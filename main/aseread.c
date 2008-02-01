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
static char *pszToken;

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
D2_FREE (pm->pBitmaps);
memset (pm, 0, sizeof (*pm));
}

//------------------------------------------------------------------------------

static float FloatTok (char *delims)
{
pszToken = strtok (NULL, delims);
return pszToken ? (float) atof (pszToken) : 0;
}

//------------------------------------------------------------------------------

static int IntTok (char *delims)
{
pszToken = strtok (NULL, delims);
return pszToken ? atoi (pszToken) : 0;
}

//------------------------------------------------------------------------------

static char CharTok (char *delims)
{
pszToken = strtok (NULL, delims);
return pszToken ? *pszToken : '\0';
}

//------------------------------------------------------------------------------

static char *StrTok (char *delims)
{
pszToken = strtok (NULL, delims);
return pszToken ? pszToken : "";
}

//------------------------------------------------------------------------------

static char *ASE_ReadLine (CFILE *cfp)
{
while (!CFEoF (cfp)) {
	CFGetS (szLine, sizeof (szLine), cfp);
	strupr (szLine);
	if (pszToken = strtok (szLine, " \t"))
		return pszToken;
	}
return NULL;
}

//------------------------------------------------------------------------------

int ASE_Error (void)
{
return 0;
}

//------------------------------------------------------------------------------

static int ASE_ReadBitmap (CFILE *cfp, tASEModel *pm, grsBitmap *bmP, int nType, int bCustom)
{
	char	fn [FILENAME_LEN];

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
if ((i < 0) || (i >= pm->nBitmaps))
	return ASE_Error ();
if (CharTok (" \t") != '{')
	return ASE_Error ();
bmP = pm->pBitmaps + i;
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
		if (!ASE_ReadBitmap (cfp, pm, bmP, nType, bCustom))
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
pm->nBitmaps = IntTok (" \t");
if (!pm->nBitmaps)
	return ASE_Error ();
if (!(pm->pBitmaps = (grsBitmap *) D2_ALLOC (pm->nBitmaps * sizeof (grsBitmap))))
	return ASE_Error ();
memset (pm->pBitmaps, 0, pm->nBitmaps * sizeof (grsBitmap));
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
			psm->vOffset.v [i] = FloatTok (" \t");
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
		for (i = 0; i < 3; i++)
			pv->vertex.v [i] = FloatTok (" \t");
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
			strtok (NULL, " :");
			pf->nVerts [i] = atoi (strtok (NULL, " :"));
			}
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
		for (i = 0; i < 2; i++)
			pt->a [i] = FloatTok (" \t");
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
		for (i = 0; i < 3; i++)
			pf->vNormal.v [i] = FloatTok (" \t");
		}
	else if (!strcmp (pszToken, "*MESH_VERTEXNORMAL")) {
		if (!psm->pVerts)
			return ASE_Error ();
		i = IntTok (" \t");
		if ((i < 0) || (i >= psm->nVerts))
			return ASE_Error ();
		pv = psm->pVerts + i;
		for (i = 0; i < 3; i++)
			pv->normal.v [i] = FloatTok (" \t");
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
if (pm->nSubModels == 17)
	pm = pm;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*NODE_NAME")) {
		strcpy (psm->szName, StrTok (" \t\""));
		if (strstr (psm->szName, "$GUN-"))
			psm->nGunPoint = atoi (psm->szName + 5);
		else {
			psm->nGunPoint = -1;
			psm->bGlow = strstr (psm->szName, "GLOW") != NULL;
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

int ASE_ReadFile (char *pszFile, tASEModel *pm, short nType, int bCustom)
{
	CFILE			cf;
	int			nResult = 1;

if (!CFOpen (&cf, pszFile, gameFolders.szModelDir [nType], "rb", 0)) {
	return 0;
	}
memset (pm, 0, sizeof (*pm));
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

