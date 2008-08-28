#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "inferno.h"
#include "u_mem.h"
#include "error.h"
#include "light.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "render.h"
#include "strutil.h"
#include "interp.h"
#include "ase.h"

static char	szLine [1024];
static char	szLineBackup [1024];
static int nLine = 0;
static CFILE *aseFile = NULL;
static char *pszToken = NULL;
static int bErrMsg = 0;

#define ASE_ROTATE_MODEL	1
#define ASE_FLIP_TEXCOORD	1

//------------------------------------------------------------------------------

int ASE_Error(const char *pszMsg)
{
if (!bErrMsg) {
	if (pszMsg)
		PrintLog ("   %s: error in line %d (%s)\n", aseFile->filename, nLine, pszMsg);
	else
		PrintLog ("   %s: error in line %d\n", aseFile->filename, nLine);
	bErrMsg = 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReleaseTextures (void)
{
	tASEModel	*pm;
	int			bCustom, i;

PrintLog ("releasing ASE model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, pm = gameData.models.aseModels [bCustom]; i; i--, pm++)
		ReleaseModelTextures (&pm->textures);
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReloadTextures (void)
{
	tASEModel	*pm;
	int			bCustom, i;

PrintLog ("reloading ASE model textures\n");
for (bCustom = 0; bCustom < 2; bCustom++)
	for (i = gameData.models.nHiresModels, pm = gameData.models.aseModels [bCustom]; i; i--, pm++)
		if (!ReadModelTextures (&pm->textures, pm->nType, bCustom)) {
			ASE_FreeModel (pm);
			return 0;
			}
return 1;
}

//------------------------------------------------------------------------------

int ASE_FreeTextures (tASEModel *pm)
{
FreeModelTextures (&pm->textures);
return 0;
}
//------------------------------------------------------------------------------

void ASE_FreeSubModel (tASESubModel *psm)
{
D2_FREE (psm->pFaces);
D2_FREE (psm->pVerts);
D2_FREE (psm->pTexCoord);
}

//------------------------------------------------------------------------------

void ASE_FreeModel (tASEModel *pm)
{
	tASESubModelList	*pml, *h;

for (pml = pm->pSubModels; pml; ) {
	ASE_FreeSubModel (&pml->sm);
	h = pml;
	pml = pml->pNextModel;
	D2_FREE (h);
	}
ASE_FreeTextures (pm);
memset (pm, 0, sizeof (*pm));
}

//------------------------------------------------------------------------------

static float FloatTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ("missing data");
return pszToken ? (float) atof (pszToken) : 0;
}

//------------------------------------------------------------------------------

static int IntTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ("missing data");
return pszToken ? atoi (pszToken) : 0;
}

//------------------------------------------------------------------------------

static char CharTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ("missing data");
return pszToken ? *pszToken : '\0';
}

//------------------------------------------------------------------------------

static char *StrTok (const char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ("missing data");
return pszToken ? pszToken : (char *) "";
}

//------------------------------------------------------------------------------

void ASE_ReadVector (CFILE *cfP, fVector3 *pv)
{
#if ASE_ROTATE_MODEL
	float x = FloatTok (" \t");
	float z = -FloatTok (" \t");
	float y = FloatTok (" \t");
	*pv = fVector3::Create(x, y, z);
#else	// need to rotate model for Descent
	int	i;

for (i = 0; i < 3; i++)
	pv [i] = FloatTok (" \t");
#endif
}

//------------------------------------------------------------------------------

static char *ASE_ReadLine (CFILE *cfP)
{
while (!CFEoF (cfP)) {
	CFGetS (szLine, sizeof (szLine), cfP);
	nLine++;
	strcpy (szLineBackup, szLine);
	strupr (szLine);
	if ((pszToken = strtok (szLine, " \t")))
		return pszToken;
	}
return NULL;
}

//------------------------------------------------------------------------------

static int ASE_ReadTexture (CFILE *cfP, tASEModel *pm, int nBitmap, int nType, int bCustom)
{
	grsBitmap	*bmP = pm->textures.pBitmaps + nBitmap;
	char			fn [FILENAME_LEN], *ps;
	int			l;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
bmP->bmFlat = 0;
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*BITMAP")) {
		if (bmP->bmTexBuf)	//duplicate
			return ASE_Error ("duplicate item");
		*fn = '\001';
		CFSplitPath (StrTok ("\""), NULL, fn + 1, NULL);
		if (!ReadModelTGA (strlwr (fn), bmP, nType, bCustom))
			return ASE_Error ("texture not found");
		l = (int) strlen (fn) + 1;
		if (!(pm->textures.pszNames [nBitmap] = (char *) D2_ALLOC (l)))
			return ASE_Error ("out of memory");
		memcpy (pm->textures.pszNames [nBitmap], fn, l);
		if ((ps = strstr (fn, "color")))
			pm->textures.nTeam [nBitmap] = atoi (ps + 5) + 1;
		else
			pm->textures.nTeam [nBitmap] = 0;
		bmP->bmTeam = pm->textures.nTeam [nBitmap];
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadMaterial (CFILE *cfP, tASEModel *pm, int nType, int bCustom)
{
	int			i;
	grsBitmap	*bmP;

i = IntTok (" \t");
if ((i < 0) || (i >= pm->textures.nBitmaps))
	return ASE_Error ("invalid bitmap number");
if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
bmP = pm->textures.pBitmaps + i;
bmP->bmFlat = 1;
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERAL_DIFFUSE")) {
		bmP->bmAvgRGB.red = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		bmP->bmAvgRGB.green = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		bmP->bmAvgRGB.blue = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		}
	else if (!strcmp (pszToken, "*MAP_DIFFUSE")) {
		if (!ASE_ReadTexture (cfP, pm, i, nType, bCustom))
			return ASE_Error (NULL);
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadMaterialList (CFILE *cfP, tASEModel *pm, int nType, int bCustom)
{
if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
if (!(pszToken = ASE_ReadLine (cfP)))
	return ASE_Error ("unexpected end of file");
if (strcmp (pszToken, "*MATERIAL_COUNT"))
	return ASE_Error ("material count missing");
pm->textures.nBitmaps = IntTok (" \t");
if (!pm->textures.nBitmaps)
	return ASE_Error ("no bitmaps specified");
if (!(pm->textures.pBitmaps = (grsBitmap *) D2_ALLOC (pm->textures.nBitmaps * sizeof (grsBitmap))))
	return ASE_Error ("out of memory");
if (!(pm->textures.pszNames = (char **) D2_ALLOC (pm->textures.nBitmaps * sizeof (char *))))
	return ASE_Error ("out of memory");
if (!(pm->textures.nTeam = (ubyte *) D2_ALLOC (pm->textures.nBitmaps * sizeof (ubyte))))
	return ASE_Error ("out of memory");
memset (pm->textures.pBitmaps, 0, pm->textures.nBitmaps * sizeof (grsBitmap));
memset (pm->textures.pszNames, 0, pm->textures.nBitmaps * sizeof (char *));
memset (pm->textures.nTeam, 0, pm->textures.nBitmaps * sizeof (ubyte));
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERIAL")) {
		if (!ASE_ReadMaterial (cfP, pm, nType, bCustom))
			return ASE_Error (NULL);
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadNode (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*TM_POS")) {
		for (i = 0; i < 3; i++)
			psm->vOffset[i] = 0; //FloatTok (" \t");
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadMeshVertexList (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEVertex		*pv;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_VERTEX")) {
		if (!psm->pVerts)
			return ASE_Error ("no vertices found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nVerts))
			return ASE_Error ("invalid vertex number");
		pv = psm->pVerts + i;
		ASE_ReadVector (cfP, &pv->vertex);
		}	
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadMeshFaceList (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEFace			*pf;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACE")) {
		if (!psm->pFaces)
			return ASE_Error ("no faces found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nFaces))
			return ASE_Error ("invalid face number");
		pf = psm->pFaces + i;
		for (i = 0; i < 3; i++) {
			strtok (NULL, " :\t");
			pf->nVerts [i] = IntTok (" :\t");
			}
		do {
			pszToken = StrTok (" :\t");
			if (!*pszToken)
				return ASE_Error ("unexpected end of file");
			} while (strcmp (pszToken, "*MESH_MTLID"));
		pf->nBitmap = IntTok (" ");
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadVertexTexCoord (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tTexCoord2f		*pt;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TVERT")) {
		if (!psm->pTexCoord)
			return ASE_Error ("no texture coordinates found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nTexCoord))
			return ASE_Error ("invalid texture coordinate number");
		pt = psm->pTexCoord + i;
#if ASE_FLIP_TEXCOORD
		pt->v.u = FloatTok (" \t");
		pt->v.v = -FloatTok (" \t");
#else
		for (i = 0; i < 2; i++)
			pt->a [i] = FloatTok (" \t");
#endif
		}	
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadFaceTexCoord (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEFace			*pf;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TFACE")) {
		if (!psm->pFaces)
			return ASE_Error ("no faces found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nFaces))
			return ASE_Error ("invalid face number");
		pf = psm->pFaces + i;
		for (i = 0; i < 3; i++)
			pf->nTexCoord [i] = IntTok (" \t");
		}	
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadMeshNormals (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEFace			*pf;
	tASEVertex		*pv;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACENORMAL")) {
		if (!psm->pFaces)
			return ASE_Error ("no faces found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nFaces))
			return ASE_Error ("invalid face number");
		pf = psm->pFaces + i;
		ASE_ReadVector (cfP, &pf->vNormal);
		}
	else if (!strcmp (pszToken, "*MESH_VERTEXNORMAL")) {
		if (!psm->pVerts)
			return ASE_Error ("no vertices found");
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nVerts))
			return ASE_Error ("invalid vertex number");
		pv = psm->pVerts + i;
		ASE_ReadVector (cfP, &pv->normal);
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadMesh (CFILE *cfP, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_NUMVERTEX")) {
		if (psm->pVerts)
			return ASE_Error ("duplicate vertex list");
		psm->nVerts = IntTok (" \t");
		if (!psm->nVerts)
			return ASE_Error ("no vertices found");
		pm->nVerts += psm->nVerts;
		if (!(psm->pVerts = (tASEVertex *) D2_ALLOC (psm->nVerts * sizeof (tASEVertex))))
			return ASE_Error ("out of memory");
		memset (psm->pVerts, 0, psm->nVerts * sizeof (tASEVertex));
		}
	else if (!strcmp (pszToken, "*MESH_NUMTVERTEX")) {
		if (psm->pTexCoord)
			return ASE_Error ("no texture coordinates found");
		psm->nTexCoord = IntTok (" \t");
		if (psm->nTexCoord) {
			if (!(psm->pTexCoord = (tTexCoord2f *) D2_ALLOC (psm->nTexCoord * sizeof (tTexCoord2f))))
				return ASE_Error ("out of memory");
			}
		}
	else if (!strcmp (pszToken, "*MESH_NUMFACES")) {
		if (psm->pFaces)
			return ASE_Error ("no faces found");
		psm->nFaces = IntTok (" \t");
		if (!psm->nFaces)
			return ASE_Error ("no faces specified");
		pm->nFaces += psm->nFaces;
		if (!(psm->pFaces = (tASEFace *) D2_ALLOC (psm->nFaces * sizeof (tASEFace))))
			return ASE_Error ("out of memory");
		memset (psm->pFaces, 0, psm->nFaces * sizeof (tASEFace));
		}
	else if (!strcmp (pszToken, "*MESH_VERTEX_LIST")) {
		if (!ASE_ReadMeshVertexList (cfP, pm))
			return ASE_Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_FACE_LIST")) {
		if (!ASE_ReadMeshFaceList (cfP, pm))
			return ASE_Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_NORMALS")) {
		if (!ASE_ReadMeshNormals (cfP, pm))
			return ASE_Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_TVERTLIST")) {
		if (!ASE_ReadVertexTexCoord (cfP, pm))
			return ASE_Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH_TFACELIST")) {
		if (!ASE_ReadFaceTexCoord (cfP, pm))
			return ASE_Error (NULL);
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

static int ASE_ReadSubModel (CFILE *cfP, tASEModel *pm)
{
	tASESubModelList	*pml;
	tASESubModel		*psm;

if (CharTok (" \t") != '{')
	return ASE_Error ("syntax error");
if (!(pml = (tASESubModelList *) D2_ALLOC (sizeof (tASESubModelList))))
	return ASE_Error ("out of memory");
memset (pml, 0, sizeof (*pml));
pml->pNextModel = pm->pSubModels;
pm->pSubModels = pml;
psm = &pm->pSubModels->sm;
psm->nId = pm->nSubModels++;
psm->nBomb = -1;
psm->nMissile = -1;
psm->nGun = -1;
psm->nGunPoint = -1;
psm->nBullets = -1;
psm->bRender = 1;
psm->nType = 0;
psm->bBarrel = 0;
while ((pszToken = ASE_ReadLine (cfP))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*NODE_NAME")) {
		strcpy (psm->szName, StrTok (" \t\""));
		if (strstr (psm->szName, "$GUNPNT"))
			psm->nGunPoint = atoi (psm->szName + 8);
		if (strstr (psm->szName, "$BULLETS"))
			psm->nBullets = 1;
		else if (strstr (psm->szName, "GLOW") != NULL) 
			psm->bGlow = 1;
		else if (strstr (psm->szName, "$DUMMY") != NULL)
			psm->bRender = 0;
		else if (strstr (psm->szName, "$THRUSTER") != NULL)
			psm->bThruster = 1;
		else if (strstr (psm->szName, "$WINGTIP") != NULL) {
			psm->bWeapon = 1;
			psm->nGun = 0;
			psm->nBomb =
			psm->nMissile = -1;
			psm->nType = atoi (psm->szName + 8) + 1;
			}
		else if (strstr (psm->szName, "$GUN") != NULL) {
			psm->bWeapon = 1;
			psm->nGun = atoi (psm->szName + 4) + 1;
			psm->nWeaponPos = atoi (psm->szName + 6) + 1;
			psm->nBomb =
			psm->nMissile = -1;
			}
		else if (strstr (psm->szName, "$BARREL") != NULL) {
			psm->bWeapon = 1;
			psm->nGun = atoi (psm->szName + 7) + 1;
			psm->nWeaponPos = atoi (psm->szName + 9) + 1;
			psm->nBomb =
			psm->nMissile = -1;
			psm->bBarrel = 1;
			}
		else if (strstr (psm->szName, "$MISSILE") != NULL) {
			psm->bWeapon = 1;
			psm->nMissile = atoi (psm->szName + 8) + 1;
			psm->nWeaponPos = atoi (psm->szName + 10) + 1;
			psm->nGun =
			psm->nBomb = -1;
			}
		else if (strstr (psm->szName, "$BOMB") != NULL) {
			psm->bWeapon = 1;
			psm->nBomb = atoi (psm->szName + 6) + 1;
			psm->nGun =
			psm->nMissile = -1;
			}
		}
	else if (!strcmp (pszToken, "*NODE_PARENT")) {
		strcpy (psm->szParent, StrTok (" \t\""));
		}
	if (!strcmp (pszToken, "*NODE_TM")) {
		if (!ASE_ReadNode (cfP, pm))
			return ASE_Error (NULL);
		}
	else if (!strcmp (pszToken, "*MESH")) {
		if (!ASE_ReadMesh (cfP, pm))
			return ASE_Error (NULL);
		}
	else if (!strcmp (pszToken, "*MATERIAL_REF")) {
		psm->nBitmap = IntTok (" \t");
		}
	}
return ASE_Error ("unexpected end of file");
}

//------------------------------------------------------------------------------

int ASE_FindSubModel (tASEModel *pm, char *pszName)
{
	tASESubModelList	*pml;

for (pml = pm->pSubModels; pml; pml = pml->pNextModel)
	if (!strcmp (pml->sm.szName, pszName))
		return pml->sm.nId;
return -1;
}

//------------------------------------------------------------------------------

void ASE_LinkSubModels (tASEModel *pm)
{
	tASESubModelList	*pml;

for (pml = pm->pSubModels; pml; pml = pml->pNextModel)
	pml->sm.nParent = ASE_FindSubModel (pm, pml->sm.szParent);
}

//------------------------------------------------------------------------------

int ASE_ReadFile (char *pszFile, tASEModel *pm, short nModel, short nType, int bCustom)
{
	CFILE			cf;
	int			nResult = 1;

if (!CFOpen (&cf, pszFile, gameFolders.szModelDir [nType], "rb", 0)) {
	return 0;
	}
bErrMsg = 0;
aseFile = &cf;
memset (pm, 0, sizeof (*pm));
pm->nModel = nModel;
pm->nType = nType;
#ifdef _DEBUG
nLine = 0;
#endif
while ((pszToken = ASE_ReadLine (&cf))) {
	if (!strcmp (pszToken, "*MATERIAL_LIST")) {
		if (!(nResult = ASE_ReadMaterialList (&cf, pm, nType, bCustom)))
			break;
		}
	else if (!strcmp (pszToken, "*GEOMOBJECT")) {
		if (!(nResult = ASE_ReadSubModel (&cf, pm)))
			break;
		}
	}
CFClose (&cf);
if (!nResult)
	ASE_FreeModel (pm);
else {
	ASE_LinkSubModels (pm);
	gameData.models.bHaveHiresModel [pm - gameData.models.aseModels [bCustom]] = 1;
	}
aseFile = NULL;
return nResult;
}


//------------------------------------------------------------------------------
//eof

