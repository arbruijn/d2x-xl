/* $Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: interp.c, v 1.14 2003/03/19 19:21:34 btb Exp $";
#endif

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "inferno.h"
#include "interp.h"
#include "shadows.h"
#include "hitbox.h"
#include "globvars.h"
#include "gr.h"
#include "byteswap.h"
#include "u_mem.h"
#include "console.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "network.h"
#include "render.h"
#include "gameseg.h"
#include "light.h"
#include "dynlight.h"
#include "lightning.h"
#include "renderthreads.h"
#include "hiresmodels.h"
#include "buildmodel.h"

//------------------------------------------------------------------------------

#define	G3_ALLOC(_buf,_count,_type,_fill) \
			if (((_buf) = (_type *) D2_ALLOC (_count * sizeof (_type)))) \
				memset (_buf, (char) _fill, _count * sizeof (_type)); \
			else \
				return G3FreeModelItems (pm);

//------------------------------------------------------------------------------

int G3AllocModel (tG3Model *pm)
{
G3_ALLOC (pm->pVerts, pm->nVerts, fVector3, 0);
G3_ALLOC (pm->pColor, pm->nVerts, tFaceColor, 0xff);
if (gameStates.ogl.bHaveVBOs) {
	int i;
	glGenBuffersARB (1, &pm->vboDataHandle);
	if ((i = glGetError ())) {
		glGenBuffersARB (1, &pm->vboDataHandle);
		if ((i = glGetError ())) {
#	ifdef _DEBUG
			HUDMessage (0, "glGenBuffersARB failed (%d)", i);
#	endif
			gameStates.ogl.bHaveVBOs = 0;
			return G3FreeModelItems (pm);
			}
		}
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, pm->vboDataHandle);
	if ((i = glGetError ())) {
#	ifdef _DEBUG
		HUDMessage (0, "glBindBufferARB failed (%d)", i);
#	endif
		gameStates.ogl.bHaveVBOs = 0;
		return G3FreeModelItems (pm);
		}
	glBufferDataARB (GL_ARRAY_BUFFER, pm->nFaceVerts * sizeof (tG3RenderVertex), NULL, GL_STATIC_DRAW_ARB);
	pm->pVertBuf [1] = (tG3RenderVertex *) glMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	pm->vboIndexHandle = 0;
	glGenBuffersARB (1, &pm->vboIndexHandle);
	if (pm->vboIndexHandle) {
		glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->vboIndexHandle);
		glBufferDataARB (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->nFaceVerts * sizeof (short), NULL, GL_STATIC_DRAW_ARB);
		pm->pIndex [1] = (short *) glMapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
		}
	}
G3_ALLOC (pm->pVertBuf [0], pm->nFaceVerts, tG3RenderVertex, 0);
G3_ALLOC (pm->pFaceVerts, pm->nFaceVerts, tG3ModelVertex, 0);
G3_ALLOC (pm->pVertNorms, pm->nFaceVerts, fVector3, 0);
G3_ALLOC (pm->pSubModels, pm->nSubModels, tG3SubModel, 0);
G3_ALLOC (pm->pFaces, pm->nFaces, tG3ModelFace, 0);
G3_ALLOC (pm->pIndex [0], pm->nFaceVerts, short, 0);
G3_ALLOC (pm->pSortedVerts, pm->nFaceVerts, tG3ModelVertex, 0);
return 1;
}

//------------------------------------------------------------------------------

#define G3_FREE(_p)	{if (_p) D2_FREE (_p);}

int G3FreeModelItems (tG3Model *pm)
{
G3_FREE (pm->pFaces);
G3_FREE (pm->pSubModels);
if (gameStates.ogl.bHaveVBOs && pm->vboDataHandle)
	glDeleteBuffersARB (1, &pm->vboDataHandle);
G3_FREE (pm->pVertBuf [0]);
G3_FREE (pm->pFaceVerts);
G3_FREE (pm->pColor);
G3_FREE (pm->pVertNorms);
G3_FREE (pm->pVerts);
G3_FREE (pm->pSortedVerts);
if (gameStates.ogl.bHaveVBOs && pm->vboIndexHandle)
	glDeleteBuffersARB (1, &pm->vboIndexHandle);
G3_FREE (pm->pIndex [0]);
memset (pm, 0, sizeof (*pm));
return 0;
}

//------------------------------------------------------------------------------

void G3FreeAllPolyModelItems (void)
{
	int	i, j;

PrintLog ("unloading polygon model data\n");
for (j = 0; j < 2; j++)
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		G3FreeModelItems (gameData.models.g3Models [j] + i);
POFFreeAllPolyModelItems ();
}

//------------------------------------------------------------------------------

void G3InitSubModelMinMax (tG3SubModel *psm)
{
psm->vMin.p.x = 
psm->vMin.p.y = 
psm->vMin.p.z = (float) 1e30;
psm->vMax.p.x = 
psm->vMax.p.y = 
psm->vMax.p.z = (float) -1e30;
}

//------------------------------------------------------------------------------

void G3SetSubModelMinMax (tG3SubModel *psm, fVector3 *vertexP)
{
	fVector3	v = *vertexP;

#if 0
v.p.x += f2fl (psm->vOffset.p.x);
v.p.y += f2fl (psm->vOffset.p.y);
v.p.z += f2fl (psm->vOffset.p.z);
#endif
if (psm->vMin.p.x > v.p.x)
	psm->vMin.p.x = v.p.x;
if (psm->vMin.p.y > v.p.y)
	psm->vMin.p.y = v.p.y;
if (psm->vMin.p.z > v.p.z)
	psm->vMin.p.z = v.p.z;
if (psm->vMax.p.x < v.p.x)
	psm->vMax.p.x = v.p.x;
if (psm->vMax.p.y < v.p.y)
	psm->vMax.p.y = v.p.y;
if (psm->vMax.p.z < v.p.z)
	psm->vMax.p.z = v.p.z;
}

//------------------------------------------------------------------------------

inline int G3CmpFaces (tG3ModelFace *pmf, tG3ModelFace *pm, grsBitmap *pTextures)
{
if (pTextures && (pmf->nBitmap >= 0) && (pm->nBitmap >= 0)) {
	if (pTextures [pmf->nBitmap].bmBPP < pTextures [pm->nBitmap].bmBPP)
		return -1;
	if (pTextures [pmf->nBitmap].bmBPP > pTextures [pm->nBitmap].bmBPP)
		return 1;
	}
if (pmf->nBitmap < pm->nBitmap)
	return -1;
if (pmf->nBitmap > pm->nBitmap)
	return 1;
if (pmf->nVerts < pm->nVerts)
	return -1;
if (pmf->nVerts > pm->nVerts)
	return 1;
return 0;
}

//------------------------------------------------------------------------------

void G3SortFaces (tG3SubModel *psm, int left, int right, grsBitmap *pTextures)
{
	int				l = left,
						r = right;
	tG3ModelFace	m = psm->pFaces [(l + r) / 2];
		
do {
	while (G3CmpFaces (psm->pFaces + l, &m, pTextures) < 0)
		l++;
	while (G3CmpFaces (psm->pFaces + r, &m, pTextures) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			tG3ModelFace h = psm->pFaces [l];
			psm->pFaces [l] = psm->pFaces [r];
			psm->pFaces [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	G3SortFaces (psm, l, right, pTextures);
if (left < r)
	G3SortFaces (psm, left, r, pTextures);
}

//------------------------------------------------------------------------------

void G3SortFaceVerts (tG3Model *pm, tG3SubModel *psm, tG3ModelVertex *psv)
{
	tG3ModelFace	*pmf;
	tG3ModelVertex	*pmv = pm->pFaceVerts;
	int				i, j, nIndex = psm->nIndex;

psv += nIndex;
for (i = psm->nFaces, pmf = psm->pFaces; i; i--, pmf++, psv += j) {
	j = pmf->nVerts;
	if (nIndex + j > pm->nFaceVerts)
		break;
	memcpy (psv, pmv + pmf->nIndex, j * sizeof (tG3ModelVertex));
	pmf->nIndex = nIndex;
	nIndex += j;
	}
}

//------------------------------------------------------------------------------

void G3SetupModel (tG3Model *pm, int bHires, int bSort)
{
	tG3SubModel		*psm;
	tG3ModelFace	*pfi, *pfj;
	tG3ModelVertex	*pmv, *pSortedVerts;
	fVector3			*pv, *pn;
	tTexCoord2f		*pt;
	tRgbaColorf		*pc;
	grsBitmap		*pTextures = bHires ? pm->pTextures : NULL;
	int				i, j;
	short				nId;

pm->fScale = 1;
pSortedVerts = pm->pSortedVerts;
for (i = 0, j = pm->nFaceVerts; i < j; i++)
	pm->pIndex [0][i] = i;
//sort each submodel's faces
for (i = pm->nSubModels, psm = pm->pSubModels; i; i--, psm++) {
	if (bSort) {
		G3SortFaces (psm, 0, psm->nFaces - 1, pTextures);
		G3SortFaceVerts (pm, psm, pSortedVerts);
		}
	for (nId = 0, j = psm->nFaces - 1, pfi = psm->pFaces; j; j--) {
		pfi->nId = nId;
		pfj = pfi++;
		if (G3CmpFaces (pfi, pfj, pTextures))
			nId++;
#if G3_ALLOW_TRANSPARENCY
		if (pTextures && (pTextures [pfi->nBitmap].bmProps.flags & BM_FLAG_TRANSPARENT))
			pm->bHasTransparency = 1;
#endif
		}
	pfi->nId = nId;
	}
pm->pVBVerts = (fVector3 *) pm->pVertBuf [0];
pm->pVBNormals = pm->pVBVerts + pm->nFaceVerts;
pm->pVBColor = (tRgbaColorf *) (pm->pVBNormals + pm->nFaceVerts);
pm->pVBTexCoord = (tTexCoord2f *) (pm->pVBColor + pm->nFaceVerts);
pv = pm->pVBVerts;
pn = pm->pVBNormals;
pt = pm->pVBTexCoord;
pc = pm->pVBColor;
pmv = bSort ? pSortedVerts : pm->pFaceVerts;
for (i = 0, j = pm->nFaceVerts; i < j; i++, pmv++) {
	pv [i] = pmv->vertex;
	pn [i] = pmv->normal;
	pc [i] = pmv->baseColor;
	pt [i] = pmv->texCoord;
	}
if (pm->pVertBuf [1])
	memcpy (pm->pVertBuf [1], pm->pVertBuf [0], pm->nFaceVerts * sizeof (tG3RenderVertex));
if (pm->pIndex [1])
	memcpy (pm->pIndex [1], pm->pIndex [0], pm->nFaceVerts * sizeof (short));
if (bSort)
	memcpy (pm->pFaceVerts, pSortedVerts, pm->nFaceVerts * sizeof (tG3ModelVertex));
else
	memcpy (pSortedVerts, pm->pFaceVerts, pm->nFaceVerts * sizeof (tG3ModelVertex));
pm->bValid = 1;
if (gameStates.ogl.bHaveVBOs) {
	glUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	glBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
	glUnmapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB);
	glBindBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
G3_FREE (pm->pSortedVerts);
}

//------------------------------------------------------------------------------

int G3ShiftModel (tObject *objP, int nModel, int bHires, fVector3 *vOffsetfP)
{
#if 1
return 0;
#else
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel		*psm;
	int				i;
	fVector3			vOffsetf;
	tG3ModelVertex	*pmv;

if (objP->nType == OBJ_PLAYER) {
	if (IsMultiGame && !IsCoopGame)
		return 0;
	}
else if ((objP->nType != OBJ_ROBOT) && (objP->nType != OBJ_HOSTAGE) && (objP->nType != OBJ_POWERUP))
	return 0;
if (vOffsetfP)
	vOffsetf = *vOffsetfP;
else
	VmVecFixToFloat (&vOffsetf, gameData.models.offsets + nModel);
if (!(vOffsetf.p.x || vOffsetf.p.y || vOffsetf.p.z))
	return 0;
if (vOffsetfP) {
	for (i = pm->nFaceVerts, pmv = pm->pFaceVerts; i; i--, pmv++) {
		pmv->vertex.p.x += vOffsetf.p.x;
		pmv->vertex.p.y += vOffsetf.p.y;
		pmv->vertex.p.z += vOffsetf.p.z;
		}
	}
else {
	for (i = pm->nSubModels, psm = pm->pSubModels; i; i--, psm++) {
		psm->vMin.p.x += vOffsetf.p.x;
		psm->vMin.p.y += vOffsetf.p.y;
		psm->vMin.p.z += vOffsetf.p.z;
		psm->vMax.p.x += vOffsetf.p.x;
		psm->vMax.p.y += vOffsetf.p.y;
		psm->vMax.p.z += vOffsetf.p.z;
		}
	}
return 1;
#endif
}

//------------------------------------------------------------------------------

void G3SubModelSize (tObject *objP, int nModel, int nSubModel, vmsVector *vOffset, int bHires)
{
	tG3Model		*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel	*psm = pm->pSubModels + nSubModel;
	tHitbox		*phb = (psm->nHitbox < 0) ? NULL : gameData.models.hitboxes [nModel].hitboxes + psm->nHitbox;
	vmsVector	vMin, vMax, vOffs;
	int			i, j;

if (vOffset)
	VmVecAdd (&vOffs, vOffset, &psm->vOffset);	//compute absolute offset (i.e. including offsets of all parent submodels)
else
	vOffs = psm->vOffset;
if (phb)
	phb->vOffset = vOffs;
vMin.p.x = fl2f (psm->vMin.p.x);
vMin.p.y = fl2f (psm->vMin.p.y);
vMin.p.z = fl2f (psm->vMin.p.z);
vMax.p.x = fl2f (psm->vMax.p.x);
vMax.p.y = fl2f (psm->vMax.p.y);
vMax.p.z = fl2f (psm->vMax.p.z);
VmVecAvg (&psm->vCenter, &vMin, &vMax);
if (psm->bBullets) {
	pm->bBullets = 1;
	pm->vBullets = psm->vCenter;
	}
psm->nRad = VmVecDist (&vMin, &vMax) / 2;
for (i = 0, j = pm->nSubModels, psm = pm->pSubModels; i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3SubModelSize (objP, nModel, i, &vOffs, bHires);
}

//------------------------------------------------------------------------------

static inline float G3CmpVerts (fVector3 *pv, fVector3 *pm)
{
	float h;

if ((h = (pv->p.x - pm->p.x)))
	return h;
if ((h = (pv->p.y - pm->p.y)))
	return h;
return pv->p.z - pm->p.z;
}

//------------------------------------------------------------------------------

static void G3SortModelVerts (fVector3 *vertices, short left, short right)
{
	short		l = left,
				r = right;
	fVector3	m = vertices [(l + r) / 2];

do {
	while (G3CmpVerts (vertices + l, &m) < 0)
		l++;
	while (G3CmpVerts (vertices + r, &m) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			fVector3 h = vertices [l];
			vertices [l] = vertices [r];
			vertices [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	G3SortModelVerts (vertices, l, right);
if (left < r)
	G3SortModelVerts (vertices, left, r);
}

//------------------------------------------------------------------------------

short G3FilterModelVerts (fVector3 *vertices, short nVertices)
{
	fVector3	*pi, *pj;

for (pi = vertices, pj = vertices + 1, --nVertices; nVertices; nVertices--, pj++)
	if (G3CmpVerts (pi, pj))
		*++pi = *pj;
return (short) (pi - vertices) + 1;
}

//------------------------------------------------------------------------------

fix G3ModelRad (tObject *objP, int nModel, int bHires)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel		*psm;
	tG3ModelFace	*pmf;
	tG3ModelVertex	*pmv;
	fVector3			*vertices, vCenter, vOffset, v, vMin, vMax, vi, vj;
	float				fRad = 0, r;
	short				h, i, j, k;

#ifdef _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
//first get the biggest distance between any two model vertices
if ((vertices = (fVector3 *) D2_ALLOC (pm->nFaceVerts * sizeof (fVector3)))) {
		fVector3	*pv, *pvi, *pvj;

	for (i = 0, h = pm->nSubModels, psm = pm->pSubModels, pv = vertices; i < h; i++, psm++) {
		if (psm->nHitbox > 0) {
			VmVecFixToFloat (&vOffset, &gameData.models.hitboxes [nModel].hitboxes [psm->nHitbox].vOffset);
			for (j = psm->nFaces, pmf = psm->pFaces; j; j--, pmf++) {
				for (k = pmf->nVerts, pmv = pm->pFaceVerts + pmf->nIndex; k; k--, pmv++, pv++)
					VmVecAdd (pv, &pmv->vertex, &vOffset);
				}
			}
		}
	h = (short) (pv - vertices) - 1;
	G3SortModelVerts (vertices, 0, h);
	h = G3FilterModelVerts (vertices, h);
	for (i = 0, pvi = vertices; i < h - 1; i++, pvi++)
		for (j = i + 1, pvj = vertices + j; j < h; j++, pvj++)
			if (fRad < (r = VmVecDist (pvi, pvj))) {
				fRad = r;
				vMin = *pvi;
				vMax = *pvj;
				}
	fRad /= 2;
	// then move the tentatively computed model center around so that all vertices are enclosed in the sphere
	// around the center with the radius computed above
	
	for (i = 0, pvi = vertices; i < h - 1; i++, pvi++)
		for (j = i + 1, pvj = vertices + j; j < h; j++, pvj++)
			if (fRad < (r = VmVecDist (pvi, pvj))) {
				fRad = r;
				vMin = *pvi;
				vMax = *pvj;
				}
	fRad /= 2;
	
	VmVecFixToFloat (&vCenter, gameData.models.offsets + nModel);
	for (i = h, pv = vertices; i; i--, pv++) {
		VmVecSub (&v, pv, &vCenter);
		r = VmVecMag (&v);
		if (fRad < r)
			VmVecScaleInc (&vCenter, &v, (r - fRad) / r);
		}

	D2_FREE (vertices);
	}
else {
		tG3ModelFace	*pmfi, *pmfj;
		tG3ModelVertex	*pmvi, *pmvj;
		short				l;

	h = pm->nFaces;
	for (i = 0, pmfi = pm->pFaces; i < h - 1; i++, pmfi++) {
		psm = pm->pSubModels + pmfi->nSubModel;
		VmVecFixToFloat (&vOffset, &gameData.models.hitboxes [nModel].hitboxes [psm->nHitbox].vOffset);
		for (k = pmfi->nVerts, pmvi = pm->pFaceVerts + pmfi->nIndex; k; k--, pmvi++) {
			VmVecAdd (&vi, &pmvi->vertex, &vOffset);
			for (j = i + 1, pmfj = pm->pFaces + j; j < h; j++, pmfj++) {
				for (l = pmfj->nVerts, pmvj = pm->pFaceVerts + pmfj->nIndex; l; l--, pmvj++) {
					if (pmfj->nSubModel == pmfi->nSubModel)
						VmVecAdd (&vj, &pmvj->vertex, &vOffset);
					else {
						psm = pm->pSubModels + pmfj->nSubModel;
						VmVecFixToFloat (&vj, &gameData.models.hitboxes [nModel].hitboxes [psm->nHitbox].vOffset);
						VmVecInc (&vj, &pmvj->vertex);
						}
					if (fRad < (r = VmVecDist (&vi, &vj))) {
						fRad = r;
						vMin = vi;
						vMax = vj;
						}
					}
				}
			}
		}
	fRad /= 2;
	// then move the tentatively computed model center around so that all vertices are enclosed in the sphere
	// around the center with the radius computed above
	VmVecFixToFloat (&vCenter, gameData.models.offsets + nModel);
	for (i = 0, h = pm->nSubModels, psm = pm->pSubModels; i < h; i++, psm++) {
		if (psm->nHitbox > 0) {
			VmVecFixToFloat (&vOffset, &gameData.models.hitboxes [nModel].hitboxes [psm->nHitbox].vOffset);
			//VmVecSub (&vOffset, &vCenter, &vOffset);
			for (j = psm->nFaces, pmf = psm->pFaces; j; j--, pmf++) {
				for (k = pmf->nVerts, pmv = pm->pFaceVerts + pmf->nIndex; k; k--, pmv++) {
					VmVecAdd (&v, &pmv->vertex, &vOffset);
					VmVecDec (&v, &vCenter);
					r = VmVecMag (&v);
					if (fRad < r)
						VmVecScaleInc (&vCenter, &v, (r - fRad) / r);
					}
				}
			}
		}
	}
VmVecFloatToFix (gameData.models.offsets + nModel, &vCenter);
return fl2f (fRad);
}

//------------------------------------------------------------------------------

fix G3ModelSize (tObject *objP, tG3Model *pm, int nModel, int bHires)
{
	int			nSubModels = pm->nSubModels;
	tG3SubModel	*psm;
	int			i, j;
	tHitbox		*phb = gameData.models.hitboxes [nModel].hitboxes;
	vmsVector	hv;
	fVector3		vOffset;
	double		dx, dy, dz, r;

#ifdef _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
psm = pm->pSubModels;
vOffset = psm->vMin;

j = 1;
if (nSubModels == 1) {
	psm->nHitbox = 1;
	gameData.models.hitboxes [nModel].nHitboxes = 1;
	}
else {
	for (i = nSubModels; i; i--, psm++)
		psm->nHitbox = (psm->bGlow || psm->bThruster || psm->bWeapon || (psm->nGunPoint >= 0)) ? -1 : j++;
	gameData.models.hitboxes [nModel].nHitboxes = j - 1;
	}
do {
	// initialize
	for (i = 0; i <= MAX_HITBOXES; i++) {
		phb [i].vMin.p.x = phb [i].vMin.p.y = phb [i].vMin.p.z = 0x7fffffff;
		phb [i].vMax.p.x = phb [i].vMax.p.y = phb [i].vMax.p.z = -0x7fffffff;
		phb [i].vOffset.p.x = phb [i].vOffset.p.y = phb [i].vOffset.p.z = 0;
		}
	// walk through all submodels, getting their sizes
	if (bHires) {
		for (i = 0; i < pm->nSubModels; i++)
			if (pm->pSubModels [i].nParent == -1) 
				G3SubModelSize (objP, nModel, i, NULL, 1);
		}
	else
		G3SubModelSize (objP, nModel, 0, NULL, 0);
	// determine min and max size
	for (i = 0, psm = pm->pSubModels; i < nSubModels; i++, psm++) {
		if (0 < (j = psm->nHitbox)) {
			phb [j].vMin.p.x = fl2f (psm->vMin.p.x);
			phb [j].vMin.p.y = fl2f (psm->vMin.p.y);
			phb [j].vMin.p.z = fl2f (psm->vMin.p.z);
			phb [j].vMax.p.x = fl2f (psm->vMax.p.x);
			phb [j].vMax.p.y = fl2f (psm->vMax.p.y);
			phb [j].vMax.p.z = fl2f (psm->vMax.p.z);
			dx = (phb [j].vMax.p.x - phb [j].vMin.p.x) / 2;
			dy = (phb [j].vMax.p.y - phb [j].vMin.p.y) / 2;
			dz = (phb [j].vMax.p.z - phb [j].vMin.p.z) / 2;
			r = sqrt (dx * dx + dy * dy + dz * dz) / 2;
			phb [j].vSize.p.x = (fix) dx;
			phb [j].vSize.p.y = (fix) dy;
			phb [j].vSize.p.z = (fix) dz;
			VmVecAdd (&hv, &phb [j].vMin, &phb [j].vOffset);
			if (phb [0].vMin.p.x > hv.p.x)
				phb [0].vMin.p.x = hv.p.x;
			if (phb [0].vMin.p.y > hv.p.y)
				phb [0].vMin.p.y = hv.p.y;
			if (phb [0].vMin.p.z > hv.p.z)
				phb [0].vMin.p.z = hv.p.z;
			VmVecAdd (&hv, &phb [j].vMax, &phb [j].vOffset);
			if (phb [0].vMax.p.x < hv.p.x)
				phb [0].vMax.p.x = hv.p.x;
			if (phb [0].vMax.p.y < hv.p.y)
				phb [0].vMax.p.y = hv.p.y;
			if (phb [0].vMax.p.z < hv.p.z)
				phb [0].vMax.p.z = hv.p.z;
			}
		}
	if (IsMultiGame)
		gameData.models.offsets [nModel].p.x =
		gameData.models.offsets [nModel].p.y =
		gameData.models.offsets [nModel].p.z = 0;
	else {
		gameData.models.offsets [nModel].p.x = (phb [0].vMin.p.x + phb [0].vMax.p.x) / -2;
		gameData.models.offsets [nModel].p.y = (phb [0].vMin.p.y + phb [0].vMax.p.y) / -2;
		gameData.models.offsets [nModel].p.z = (phb [0].vMin.p.z + phb [0].vMax.p.z) / -2;
		}
	} while (G3ShiftModel (objP, nModel, bHires, NULL));

psm = pm->pSubModels;
VmVecSub (&vOffset, &psm->vMin, &vOffset);
VmVecFloatToFix (gameData.models.offsets + nModel, &vOffset);
#ifdef _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
#if 1
//VmVecInc (&psm [0].vOffset, gameData.models.offsets + nModel);
#else
G3ShiftModel (objP, nModel, bHires, &vOffset);
#endif
dx = (phb [0].vMax.p.x - phb [0].vMin.p.x);
dy = (phb [0].vMax.p.y - phb [0].vMin.p.y);
dz = (phb [0].vMax.p.z - phb [0].vMin.p.z);
phb [0].vSize.p.x = (fix) dx;
phb [0].vSize.p.y = (fix) dy;
phb [0].vSize.p.z = (fix) dz;
gameData.models.offsets [nModel].p.x = (phb [0].vMax.p.x + phb [0].vMin.p.x) / 2;
gameData.models.offsets [nModel].p.y = (phb [0].vMax.p.y + phb [0].vMin.p.y) / 2;
gameData.models.offsets [nModel].p.z = (phb [0].vMax.p.z + phb [0].vMin.p.z) / 2;
//phb [0].vOffset = gameData.models.offsets [nModel];
for (i = 0; i <= j; i++)
	ComputeHitbox (nModel, i);
return G3ModelRad (objP, nModel, bHires);
}

//------------------------------------------------------------------------------

int NearestGunPoint (vmsVector *vGunPoints, vmsVector *vGunPoint, int nGuns, int *nUsedGuns)
{
	fix			xDist, xMinDist = 0x7fffffff;
	int			h = 0, i;
	vmsVector	vi, v0 = *vGunPoint;

v0.p.z = 0;
for (i = 0; i < nGuns; i++) {
	if (nUsedGuns [i])
		continue;
	vi = vGunPoints [i];
	vi.p.z = 0;
	xDist = VmVecDist (&vi, &v0);
	if (xMinDist > xDist) {
		xMinDist = xDist;
		h = i;
		}
	}
nUsedGuns [h] = 1;
return h;
}

//------------------------------------------------------------------------------

void SetShipGunPoints (tOOFObject *po, tG3Model *pm)
{
	static short nGunSubModels [] = {6, 7, 5, 4, 9, 10, 3, 3};

	tG3SubModel		*psm;
	tOOF_point		*pp;
	int				i;

for (i = 0, pp = po->gunPoints.pPoints; i < (po->gunPoints.nPoints = N_PLAYER_GUNS); i++, pp++) {
	if (nGunSubModels [i] >= pm->nSubModels)
		continue;
	pm->nGunSubModels [i] = nGunSubModels [i];
	psm = pm->pSubModels + nGunSubModels [i];
	pp->vPos.x = (psm->vMax.p.x + psm->vMin.p.x) / 2;
	if (3 == (pp->nParent = nGunSubModels [i])) {
		pp->vPos.y = (psm->vMax.p.z + 3 * psm->vMin.p.y) / 4;
		pp->vPos.z = 7 * (psm->vMax.p.z + psm->vMin.p.z) / 8;
		}
	else {
		pp->vPos.y = (psm->vMax.p.y + psm->vMin.p.y) / 2;
		if (i < 4)
      	pp->vPos.z = psm->vMax.p.z;
		else
			pp->vPos.z = (psm->vMax.p.z + psm->vMin.p.z) / 2;
		}
	}
}

//------------------------------------------------------------------------------

void SetRobotGunPoints (tOOFObject *po, tG3Model *pm)
{
	tG3SubModel		*psm;
	tOOF_point		*pp;
	int				i, j = po->gunPoints.nPoints;

for (i = 0, pp = po->gunPoints.pPoints; i < j; i++, pp++) {
	pm->nGunSubModels [i] = pp->nParent;
	psm = pm->pSubModels + pp->nParent;
	pp->vPos.x = (psm->vMax.p.x + psm->vMin.p.x) / 2;
	pp->vPos.y = (psm->vMax.p.y + psm->vMin.p.y) / 2;
  	pp->vPos.z = psm->vMax.p.z;
	}
}

//------------------------------------------------------------------------------

void G3SetGunPoints (tObject *objP, tG3Model *pm, int nModel, int bASE)
{
	vmsVector		*vGunPoints;
	int				nParent, h, i, j;

if (bASE) {
	tG3SubModel	*psm = pm->pSubModels;

	vGunPoints = gameData.models.gunInfo [nModel].vGunPoints;
	for (i = 0, j = 0; i < pm->nSubModels; i++, psm++) {
		h = psm->nGunPoint;
		if ((h >= 0) && (h < MAX_GUNS)) {
			j++;
			pm->nGunSubModels [h] = i;
			vGunPoints [h] = psm->vCenter;
			}
		}
	gameData.models.gunInfo [nModel].nGuns = j;
	}
else {
		tOOF_point		*pp;
		tOOF_subObject	*pso;
		tOOFObject		*po = gameData.models.modelToOOF [1][nModel];

	if (!po)
		po = gameData.models.modelToOOF [0][nModel];
	pp = po->gunPoints.pPoints;
	if (objP->nType == OBJ_PLAYER)
		SetShipGunPoints (po, pm); 
	else if (objP->nType == OBJ_ROBOT)
		SetRobotGunPoints (po, pm); 
	else {
		gameData.models.gunInfo [nModel].nGuns = 0;
		return;
		}
	if ((j = po->gunPoints.nPoints)) {
		if (j > MAX_GUNS)
			j = MAX_GUNS;
		gameData.models.gunInfo [nModel].nGuns = j;
		vGunPoints = gameData.models.gunInfo [nModel].vGunPoints;
		for (i = 0; i < j; i++, pp++, vGunPoints++) {
			vGunPoints->p.x = fl2f (pp->vPos.x);
			vGunPoints->p.y = fl2f (pp->vPos.y);
			vGunPoints->p.z = fl2f (pp->vPos.z);
			for (nParent = pp->nParent; nParent >= 0; nParent = pso->nParent) {
				pso = po->pSubObjects + nParent;
				VmVecInc (vGunPoints, &pm->pSubModels [nParent].vOffset);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int G3BuildModel (tObject *objP, int nModel, tPolyModel *pp, grsBitmap **modelBitmaps, tRgbaColorf *pObjColor, int bHires)
{
	tG3Model	*pm = gameData.models.g3Models [bHires] + nModel;

if (pm->bValid > 0)
	return 1;
if (pm->bValid < 0)
	return 0;
pm->bRendered = 0;
#ifdef _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
memset (pm->nGunSubModels, 0xff, sizeof (pm->nGunSubModels));
return bHires ? 
			ASEModel (nModel) ? 
				G3BuildModelFromASE (objP, nModel) : 
				G3BuildModelFromOOF (objP, nModel) : 
			G3BuildModelFromPOF (objP, nModel, pp, modelBitmaps, pObjColor);
}

//------------------------------------------------------------------------------

int G3ModelMinMax (int nModel, tHitbox *phb)
{
	tG3Model		*pm;
	tG3SubModel	*psm;
	int			i;

if (!((pm = gameData.models.g3Models [1] + nModel) || 
	   (pm = gameData.models.g3Models [0] + nModel)))
	return 0;
for (i = pm->nSubModels, psm = pm->pSubModels; i; i--, psm++) {
	if (!psm->bThruster && (psm->nGunPoint < 0)) {
		phb->vMin.p.x = fl2f (psm->vMin.p.x);
		phb->vMin.p.y = fl2f (psm->vMin.p.y);
		phb->vMin.p.z = fl2f (psm->vMin.p.z);
		phb->vMax.p.x = fl2f (psm->vMax.p.x);
		phb->vMax.p.y = fl2f (psm->vMax.p.y);
		phb->vMax.p.z = fl2f (psm->vMax.p.z);
		phb++;
		}
	}
return pm->nSubModels;
}

//------------------------------------------------------------------------------
//eof
