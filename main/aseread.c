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

typedef struct tASEVertex {
	fVector3					vertex;
	fVector3					normal;
} tASEVertex;

typedef struct tASEFace {
	fVector3					vNormal;
	int						nVerts [3];	// indices of vertices 
	int						nTexCoord [3];
} tASEFace;

typedef struct tASESubModel {
	char						szName [256];
	char						szParent [256];
	int						nParent;
	int						nBitmap;
	int						nFaces;
	int						nVerts;
	int						nTexCoord;
	int						nIndex;
	fVector3					vOffset;
	tASEFace					*pFaces;
	tASEVertex				*pVerts;
	tTexCoord2f				*pTexCoord;
} tASESubModel;

typedef struct tSubModelList {
	struct tSubModelList *pNextModel;
	tASESubModel			sm;
} tSubModelList;

typedef struct tASEModel {
	int				nBitmaps;
	grsBitmap		*pBitmaps;
	tSubModelList	*pSubModels;
	int				nSubModels;
	int				nVerts;
	int				nFaces;
} tASEModel;

static char	szLine [1024];

//------------------------------------------------------------------------------

float FloatTok (char *delims)
{
	char	*pszToken = strtok (NULL, delims);

return pszToken ? (float) atof (pszToken) : 0;
}

//------------------------------------------------------------------------------

int IntTok (char *delims)
{
	char	*pszToken = strtok (NULL, delims);

return pszToken ? atoi (pszToken) : 0;
}

//------------------------------------------------------------------------------

char CharTok (char *delims)
{
	char	*pszToken = strtok (NULL, delims);

return pszToken ? *pszToken : '\0';
}

//------------------------------------------------------------------------------

char *StrTok (char *delims)
{
	char	*pszToken = strtok (NULL, delims);

return pszToken ? pszToken : "";
}

//------------------------------------------------------------------------------

char *ASE_ReadLine (CFILE *cfp)
{
	char	*pszToken;

while (!CFEoF (cfp)) {
	CFGetS (szLine, sizeof (szLine), cfp);
	if (pszToken = strtok (szLine, " "))
		return pszToken;
	}
return NULL;
}

//------------------------------------------------------------------------------

int ASE_ReadBitmap (CFILE *cfp, tASEModel *pm, grsBitmap *bmP, int nType, int bCustom)
{
	char	*pszToken;

if (CharTok (" ") != '{')
	return 0;
bmP->bmFlat = 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*BITMAP")) {
		if (bmP->bmTexBuf)	//duplicate
			return 0;
		if (!OOF_ReadTGA (StrTok (" \""), bmP, nType, bCustom))
			return 0;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMaterial (CFILE *cfp, tASEModel *pm, int nType, int bCustom)
{
	int			i;
	grsBitmap	*bmP;
	char			*pszToken = strtok (szLine, " ");

i = IntTok (" ");
if ((i < 0) || (i >= pm->nBitmaps))
	return 0;
if (CharTok (" ") != '{')
	return 0;
bmP = pm->pBitmaps + i;
bmP->bmFlat = 1;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERAL_DIFFUSE")) {
		bmP->bmAvgRGB.red = (ubyte) (FloatTok (" ") * 255 + 0.5);
		bmP->bmAvgRGB.green = (ubyte) (FloatTok (" ") * 255 + 0.5);
		bmP->bmAvgRGB.blue = (ubyte) (FloatTok (" ") * 255 + 0.5);
		}
	else if (!strcmp (pszToken, "*MAP_DIFFUSE")) {
		if (!ASE_ReadBitmap (cfp, pm, bmP, nType, bCustom))
			return 0;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMaterialList (CFILE *cfp, tASEModel *pm, int nType, int bCustom)
{
	char	*pszToken;

if (CharTok (" ") != '{')
	return 0;
CFGetS (szLine, sizeof (szLine), cfp);
if (!strcmp (strtok (szLine, " "), "*MATERIAL_COUNT"))
	return 0;
pm->nBitmaps = atoi (strtok (szLine, " "));
if (!pm->nBitmaps)
	return 0;
if (!(pm->pBitmaps = (grsBitmap *) D2_ALLOC (pm->nBitmaps * sizeof (grsBitmap))))
	return 0;
memset (pm->pBitmaps, 0, pm->nBitmaps * sizeof (grsBitmap));
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MATERIAL")) {
		if (!ASE_ReadMaterial (cfp, pm, nType, bCustom))
			return 0;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadNode (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	int	i;
	char	*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*NODE_NAME")) {
		strcpy (pml->sm.szName, StrTok (" \""));
		}
	if (!strcmp (pszToken, "*NODE_PARENT")) {
		strcpy (pml->sm.szParent, StrTok (" \""));
		}
	else if (!strcmp (pszToken, "*TM_POS")) {
		for (i = 0; i < 3; i++)
			pml->sm.vOffset.v [i] = FloatTok (" ");
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMeshVertexList (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	tASEVertex	*pv;
	int			i;
	char			*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_VERTEX")) {
		pv = pml->sm.pVerts + IntTok (" ");
		for (i = 0; i < 3; i++)
			pv->vertex.v [i] = FloatTok (" ");
		}	
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMeshFace (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	tASEFace	*pf;
	int		i;
	char		*pszToken;

if (CharTok (" ") != '{')
	return 0;
pf = pml->sm.pFaces + IntTok (" ");
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACE")) {
		for (i = 0; i < 3; i++) {
			strtok (NULL, " :");
			pf->nVerts [i] = atoi (strtok (NULL, " :"));
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMeshFaceList (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	char	*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACE")) {
		if (!ASE_ReadMeshFace (cfp, pm, pml))
			return 0;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadVertexTexCoord (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	tTexCoord2f	*pt;
	int			i;
	char			*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TVERT")) {
		pt = pml->sm.pTexCoord + IntTok (" ");
		for (i = 0; i < 2; i++)
			pt->a [i] = FloatTok (" ");
		}	
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadFaceTexCoord (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	tASEFace	*pf;
	int		i;
	char		*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_TFACE")) {
		pf = pml->sm.pFaces + IntTok (" ");
		for (i = 0; i < 3; i++)
			pf->nTexCoord [i] = IntTok (" ");
		}	
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMeshNormals (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	tASEFace		*pf;
	tASEVertex	*pv;
	int			i;
	char			*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_FACENORMAL")) {
		pf = pml->sm.pFaces + IntTok (" ");
		for (i = 0; i < 3; i++)
			pf->vNormal.v [i] = FloatTok (" ");
		}
	else if (!strcmp (pszToken, "*MESH_VERTEXNORMAL")) {
		pv = pml->sm.pVerts + IntTok (" ");
		for (i = 0; i < 3; i++)
			pv->vertex.v [i] = FloatTok (" ");
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadMesh (CFILE *cfp, tASEModel *pm, tSubModelList *pml)
{
	char	*pszToken;

if (CharTok (" ") != '{')
	return 0;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*MESH_NUMVERTEX")) {
		pml->sm.nVerts = IntTok (" ");
		if (!pml->sm.nVerts)
			return 0;
		pm->nVerts += pml->sm.nVerts;
		if (!(pml->sm.pVerts = (tASEVertex *) D2_ALLOC (pml->sm.nVerts * sizeof (tASEVertex))))
			return 0;
		memset (pml->sm.pVerts, 0, pml->sm.nVerts * sizeof (tASEVertex));
		}
	else if (!strcmp (pszToken, "*MESH_NUMTVERTEX")) {
		pml->sm.nTexCoord = IntTok (" ");
		if (pml->sm.nTexCoord) {
			if (!(pml->sm.pTexCoord = (tTexCoord2f *) D2_ALLOC (pml->sm.nVerts * sizeof (tTexCoord2f))))
				return 0;
			}
		}
	else if (!strcmp (pszToken, "*MESH_NUMFACES")) {
		pml->sm.nFaces = IntTok (" ");
		if (!pml->sm.nFaces)
			return 0;
		pm->nFaces += pml->sm.nFaces;
		if (!(pml->sm.pFaces = (tASEFace *) D2_ALLOC (pml->sm.nFaces * sizeof (tASEFace))))
			return 0;
		memset (pml->sm.pFaces, 0, pml->sm.nFaces * sizeof (tASEFace));
		}
	else if (!strcmp (pszToken, "*MESH_VERTEX_LIST")) {
		if (!ASE_ReadMeshVertexList (cfp, pm, pml))
			return 0;
		}
	else if (!strcmp (pszToken, "*MESH_FACE_LIST")) {
		if (!ASE_ReadMeshFaceList (cfp, pm, pml))
			return 0;
		}
	else if (!strcmp (pszToken, "*MESH_NORMALS")) {
		if (!ASE_ReadMeshNormals (cfp, pm, pml))
			return 0;
		}
	else if (!strcmp (pszToken, "*MESH_TVERTLIST")) {
		if (!ASE_ReadVertexTexCoord (cfp, pm, pml))
			return 0;
		}
	else if (!strcmp (pszToken, "*MESH_TFACELIST")) {
		if (!ASE_ReadFaceTexCoord (cfp, pm, pml))
			return 0;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadSubModel (CFILE *cfp, tASEModel *pm)
{
	tSubModelList	*pml;
	char				*pszToken;
	
if (CharTok (" ") != '{')
	return 0;
if (!(pml = (tSubModelList *) D2_ALLOC (sizeof (tSubModelList))))
	return 0;
memset (pml, 0, sizeof (*pml));
pml->pNextModel = pm->pSubModels;
pm->pSubModels = pml;
pm->nSubModels++;
while ((pszToken = ASE_ReadLine (cfp))) {
	if (*pszToken == '}')
		return 1;
	if (!strcmp (pszToken, "*NODE_TM")) {
		if (!ASE_ReadNode (cfp, pm, pml))
			return 0;
		}
	else if (!strcmp (pszToken, "*MESH")) {
		if (!ASE_ReadMesh (cfp, pm, pml))
			return 0;
		}
	else if (!strcmp (pszToken, "*MATERIAL_REF")) {
		pml->sm.nBitmap = IntTok (" ");
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int ASE_ReadFile (char *pszFile, tG3Model *pm, short nType, int bCustom)
{
	CFILE			cf;
	tASEModel	am;
	char			*pszToken;
	int			nResult = 1;

if (!CFOpen (&cf, pszFile, gameFolders.szModelDir [nType], "rb", 0)) {
	return 0;
	}
memset (&am, 0, sizeof (am));
while ((pszToken = ASE_ReadLine (&cf))) {
	if (!strcmp (pszToken, "*MATERIAL_LIST")) {
		if (!(nResult = ASE_ReadMaterialList (&cf, &am, nType, bCustom)))
			break;
		}
	else if (!strcmp (pszToken, "*GEOMOBJECT")) {
		if (!(nResult = ASE_ReadSubModel (&cf, &am)))
			break;
		}
	}
CFClose (&cf);
gameData.models.bHaveHiresModel [pm - gameData.models.g3Models [1]] = 1;
return 1;
}

//------------------------------------------------------------------------------
//eof

