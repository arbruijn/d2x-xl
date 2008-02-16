#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif
//#include  "oof.h
#include "inferno.h"
#include "cfile.h"
#include "args.h"
#include "u_mem.h"
#include "gr.h"
#include "error.h"
#include "globvars.h"
#include "3d.h"
#include "light.h"
#include "dynlight.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "vecmat.h"
#include "render.h"
#include "strutil.h"
#include "hudmsg.h"
#include "tga.h"
#include "interp.h"
#include "ase.h"

static char	szLine [1024];
#ifdef _DEBUG
static char	szLineBackup [1024];
static int nLine = 0;
#endif
static char *pszToken;

#define ASE_ROTATE_MODEL	1
#define ASE_FLIP_TEXCOORD	1

//------------------------------------------------------------------------------

int ASE_Error (void)
{
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReleaseTextures (void)
{
	tASEModel	*pm;
	int			bCustom, i;

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

static float FloatTok (char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ();
return pszToken ? (float) atof (pszToken) : 0;
}

//------------------------------------------------------------------------------

static int IntTok (char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ();
return pszToken ? atoi (pszToken) : 0;
}

//------------------------------------------------------------------------------

static char CharTok (char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ();
return pszToken ? *pszToken : '\0';
}

//------------------------------------------------------------------------------

static char *StrTok (char *delims)
{
pszToken = strtok (NULL, delims);
if (!(pszToken && *pszToken))
	ASE_Error ();
return pszToken ? pszToken : "";
}

//------------------------------------------------------------------------------

void ASE_ReadVector (CFILE *cfp, fVector3 *pv)
{
#if ASE_ROTATE_MODEL
pv->p.x = FloatTok (" \t");
pv->p.z = -FloatTok (" \t");
pv->p.y = FloatTok (" \t");
#else	// need to rotate model for Descent
	int	i;

for (i = 0; i < 3; i++)
	pv [i] = FloatTok (" \t");
#endif
}

//------------------------------------------------------------------------------

static char *ASE_ReadLine (CFILE *cfp)
{
while (!CFEoF (cfp)) {
	CFGetS (szLine, sizeof (szLine), cfp);
#ifdef _DEBUG
	nLine++;
	strcpy (szLineBackup, szLine);
#endif
	strupr (szLine);
	if ((pszToken = strtok (szLine, " \t")))
		return pszToken;
	}
return NULL;
}

//------------------------------------------------------------------------------

static int ASE_ReadTexture (CFILE *cfp, tASEModel *pm, int nBitmap, int nType, int bCustom)
{
	grsBitmap	*bmP = pm->textures.pBitmaps + nBitmap;
	char			fn [FILENAME_LEN];
	int			l;

if (CharTok (" \t") != '{')
	return ASE_Error ();
bmP->bmFlat = 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*BITMAP")) {
		if (bmP->bmTexBuf)	//duplicate
			return ASE_Error ();
		*fn = '\001';
		CFSplitPath (StrTok (" \t\""), NULL, fn + 1, NULL);
		if (!ReadModelTGA (strlwr (fn), bmP, nType, bCustom))
			return ASE_Error ();
		l = (int) strlen (fn) + 1;
		if (!(pm->textures.pszNames [nBitmap] = D2_ALLOC (l)))
			return ASE_Error ();
		memcpy (pm->textures.pszNames [nBitmap], fn, l);
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadMaterial (CFILE *cfp, tASEModel *pm, int nType, int bCustom)
{
	int			i;
	grsBitmap	*bmP;

i = IntTok (" \t");
if ((i < 0) || (i >= pm->textures.nBitmaps))
	return ASE_Error ();
if (CharTok (" \t") != '{')
	return ASE_Error ();
bmP = pm->textures.pBitmaps + i;
bmP->bmFlat = 1;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERAL_DIFFUSE")) {
		bmP->bmAvgRGB.red = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		bmP->bmAvgRGB.green = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		bmP->bmAvgRGB.blue = (ubyte) (FloatTok (" \t") * 255 + 0.5);
		}
	else if (!strcmp (pszToken, "*MAP_DIFFUSE")) {
		if (!ASE_ReadTexture (cfp, pm, i, nType, bCustom))
			return ASE_Error ();
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadMaterialList (CFILE *cfp, tASEModel *pm, int nType, int bCustom)
{
if (CharTok (" \t") != '{')
	return ASE_Error ();
if (!(pszToken = ASE_ReadLine (cfp)))
	return ASE_Error ();
if (strcmp (pszToken, "*MATERIAL_COUNT"))
	return ASE_Error ();
pm->textures.nBitmaps = IntTok (" \t");
if (!pm->textures.nBitmaps)
	return ASE_Error ();
if (!(pm->textures.pBitmaps = (grsBitmap *) D2_ALLOC (pm->textures.nBitmaps * sizeof (grsBitmap))))
	return ASE_Error ();
if (!(pm->textures.pszNames = (char **) D2_ALLOC (pm->textures.nBitmaps * sizeof (char *))))
	return ASE_Error ();
memset (pm->textures.pBitmaps, 0, pm->textures.nBitmaps * sizeof (grsBitmap));
memset (pm->textures.pszNames, 0, pm->textures.nBitmaps * sizeof (char *));
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERIAL")) {
		if (!ASE_ReadMaterial (cfp, pm, nType, bCustom))
			return ASE_Error ();
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadNode (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*TM_POS")) {
		for (i = 0; i < 3; i++)
			psm->vOffset.v [i] = 0; //FloatTok (" \t");
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadMeshVertexList (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEVertex		*pv;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_VERTEX")) {
		if (!psm->pVerts)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nVerts))
			return ASE_Error ();
		pv = psm->pVerts + i;
		ASE_ReadVector (cfp, &pv->vertex);
		}	
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadMeshFaceList (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEFace			*pf;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACE")) {
		if (!psm->pFaces)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nFaces))
			return ASE_Error ();
		pf = psm->pFaces + i;
		for (i = 0; i < 3; i++) {
			strtok (NULL, " :\t");
			pf->nVerts [i] = IntTok (" :\t");
			}
		do {
			pszToken = StrTok (" :\t");
			if (!*pszToken)
				return ASE_Error ();
			} while (strcmp (pszToken, "*MESH_MTLID"));
		pf->nBitmap = IntTok (" ");
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadVertexTexCoord (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tTexCoord2f		*pt;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TVERT")) {
		if (!psm->pTexCoord)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nTexCoord))
			return ASE_Error ();
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
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadFaceTexCoord (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEFace			*pf;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TFACE")) {
		if (!psm->pFaces)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nFaces))
			return ASE_Error ();
		pf = psm->pFaces + i;
		for (i = 0; i < 3; i++)
			pf->nTexCoord [i] = IntTok (" \t");
		}	
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadMeshNormals (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;
	tASEFace			*pf;
	tASEVertex		*pv;
	int				i;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACENORMAL")) {
		if (!psm->pFaces)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nFaces))
			return ASE_Error ();
		pf = psm->pFaces + i;
		ASE_ReadVector (cfp, &pf->vNormal);
		}
	else if (!strcmp (pszToken, "*MESH_VERTEXNORMAL")) {
		if (!psm->pVerts)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nVerts))
			return ASE_Error ();
		pv = psm->pVerts + i;
		ASE_ReadVector (cfp, &pv->normal);
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadMesh (CFILE *cfp, tASEModel *pm)
{
	tASESubModel	*psm = &pm->pSubModels->sm;

if (CharTok (" \t") != '{')
	return ASE_Error ();
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_NUMVERTEX")) {
		if (psm->pVerts)
			return ASE_Error ();
		psm->nVerts = IntTok (" \t");
		if (!psm->nVerts)
			return ASE_Error ();
		pm->nVerts += psm->nVerts;
		if (!(psm->pVerts = (tASEVertex *) D2_ALLOC (psm->nVerts * sizeof (tASEVertex))))
			return ASE_Error ();
		memset (psm->pVerts, 0, psm->nVerts * sizeof (tASEVertex));
		}
	else if (!strcmp (pszToken, "*MESH_NUMTVERTEX")) {
		if (psm->pTexCoord)
			return ASE_Error ();
		psm->nTexCoord = IntTok (" \t");
		if (psm->nTexCoord) {
			if (!(psm->pTexCoord = (tTexCoord2f *) D2_ALLOC (psm->nTexCoord * sizeof (tTexCoord2f))))
				return ASE_Error ();
			}
		}
	else if (!strcmp (pszToken, "*MESH_NUMFACES")) {
		if (psm->pFaces)
			return ASE_Error ();
		psm->nFaces = IntTok (" \t");
		if (!psm->nFaces)
			return ASE_Error ();
		pm->nFaces += psm->nFaces;
		if (!(psm->pFaces = (tASEFace *) D2_ALLOC (psm->nFaces * sizeof (tASEFace))))
			return ASE_Error ();
		memset (psm->pFaces, 0, psm->nFaces * sizeof (tASEFace));
		}
	else if (!strcmp (pszToken, "*MESH_VERTEX_LIST")) {
		if (!ASE_ReadMeshVertexList (cfp, pm))
			return ASE_Error ();
		}
	else if (!strcmp (pszToken, "*MESH_FACE_LIST")) {
		if (!ASE_ReadMeshFaceList (cfp, pm))
			return ASE_Error ();
		}
	else if (!strcmp (pszToken, "*MESH_NORMALS")) {
		if (!ASE_ReadMeshNormals (cfp, pm))
			return ASE_Error ();
		}
	else if (!strcmp (pszToken, "*MESH_TVERTLIST")) {
		if (!ASE_ReadVertexTexCoord (cfp, pm))
			return ASE_Error ();
		}
	else if (!strcmp (pszToken, "*MESH_TFACELIST")) {
		if (!ASE_ReadFaceTexCoord (cfp, pm))
			return ASE_Error ();
		}
	}
return ASE_Error ();
}

//------------------------------------------------------------------------------

static int ASE_ReadSubModel (CFILE *cfp, tASEModel *pm)
{
	tASESubModelList	*pml;
	tASESubModel		*psm;

if (CharTok (" \t") != '{')
	return ASE_Error ();
if (!(pml = (tASESubModelList *) D2_ALLOC (sizeof (tASESubModelList))))
	return ASE_Error ();
memset (pml, 0, sizeof (*pml));
pml->pNextModel = pm->pSubModels;
pm->pSubModels = pml;
psm = &pm->pSubModels->sm;
psm->nId = pm->nSubModels++;
psm->nBomb = -1;
psm->nMissile = -1;
psm->nGun = -1;
psm->nGunPoint = -1;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*NODE_NAME")) {
		strcpy (psm->szName, StrTok (" \t\""));
		if (strstr (psm->szName, "$GUNPNT"))
			psm->nGunPoint = atoi (psm->szName + 8);
		else if (strstr (psm->szName, "GLOW") != NULL) 
			psm->bGlow = 1;
		else if (strstr (psm->szName, "$THRUSTER") != NULL)
			psm->bThruster = 1;
		else if (strstr (psm->szName, "$WINGTIP") != NULL) {
			psm->bWeapon = 1;
			psm->nGun = 0;
			psm->nBomb =
			psm->nMissile = -1;
			}
		else if (strstr (psm->szName, "$STDLAS") != NULL) {
			psm->bWeapon = 1;
			psm->nGun = -1;
			psm->nBomb =
			psm->nMissile = -1;
			}
		else if (strstr (psm->szName, "$GUN") != NULL) {
			psm->bWeapon = 1;
			psm->nGun = atoi (psm->szName + 4) + 1;
			psm->nWeaponPos = atoi (psm->szName + 6) + 1;
			psm->nBomb =
			psm->nMissile = -1;
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
		if (!ASE_ReadNode (cfp, pm))
			return ASE_Error ();
		}
	else if (!strcmp (pszToken, "*MESH")) {
		if (!ASE_ReadMesh (cfp, pm))
			return ASE_Error ();
		}
	else if (!strcmp (pszToken, "*MATERIAL_REF")) {
		psm->nBitmap = IntTok (" \t");
		}
	}
return ASE_Error ();
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
return nResult;
}


//------------------------------------------------------------------------------
//eof

