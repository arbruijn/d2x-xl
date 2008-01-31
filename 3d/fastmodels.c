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

extern tFaceColor tMapColor;

#if DBG_SHADOWS
extern int bShadowTest;
extern int bFrontCap;
extern int bRearCap;
extern int bShadowVolume;
extern int bFrontFaces;
extern int bBackFaces;
extern int bSWCulling;
#endif
#if SHADOWS
extern int bZPass;
#endif

#define G3_DRAW_ARRAYS				1
#define G3_DRAW_SUBMODELS			1
#define G3_FAST_MODELS				1
#define G3_DRAW_RANGE_ELEMENTS	1
#define G3_SW_SCALING				0
#define G3_HW_LIGHTING				1
#define G3_USE_VBOS					1 //G3_HW_LIGHTING
#define G3_ALLOW_TRANSPARENCY		1

#define G3_BUFFER_OFFSET(_i)	(GLvoid *) ((char *) NULL + (_i))

//------------------------------------------------------------------------------

#define G3_FREE(_p)	{if (_p) D2_FREE (_p);}

int G3FreeModelItems (tG3Model *pm)
{
G3_FREE (pm->pFaces);
G3_FREE (pm->pSubModels);
if (gameStates.ogl.bHaveVBOs && pm->vboDataHandle)
	glDeleteBuffers (1, &pm->vboDataHandle);
G3_FREE (pm->pVertBuf [0]);
G3_FREE (pm->pFaceVerts);
G3_FREE (pm->pColor);
G3_FREE (pm->pVertNorms);
G3_FREE (pm->pVerts);
G3_FREE (pm->pSortedVerts);
if (gameStates.ogl.bHaveVBOs && pm->vboIndexHandle)
	glDeleteBuffers (1, &pm->vboIndexHandle);
G3_FREE (pm->pIndex [0]);
memset (pm, 0, sizeof (*pm));
return 0;
}

//------------------------------------------------------------------------------

void G3FreeAllPolyModelItems (void)
{
	int	i, j;

for (j = 0; j < 2; j++)
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		G3FreeModelItems (gameData.models.g3Models [j] + i);
POFFreeAllPolyModelItems ();
}

//------------------------------------------------------------------------------

bool G3CountModelItems (void *modelP, short *pnSubModels, short *pnVerts, short *pnFaces, short *pnFaceVerts)
{
	ubyte *p = modelP;

G3CheckAndSwap (modelP);
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			(*pnVerts) += n;
			p += n * sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			p += n * sizeof (vmsVector) + 8;
			(*pnVerts) += n;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + ((nVerts & ~ 1) + 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + ((nVerts & ~1) + 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			G3CountModelItems (p + WORDVAL (p+28), pnSubModels, pnVerts, pnFaces, pnFaceVerts);
			G3CountModelItems (p + WORDVAL (p+30), pnSubModels, pnVerts, pnFaces, pnFaceVerts);
			p += 32;
			break;


		case OP_RODBM: {
			p += 36;
			break;
			}

		case OP_SUBCALL: {
			(*pnSubModels)++;
			G3CountModelItems (p + WORDVAL (p+16), pnSubModels, pnVerts, pnFaces, pnFaceVerts);
			p += 20;
			break;
			}

		case OP_GLOW:
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
	}
return 1;
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

tG3ModelFace *G3AddModelFace (tG3Model *pm, tG3SubModel *psm, tG3ModelFace *pmf, vmsVector *pn, ubyte *p, 
										grsBitmap **modelBitmaps, tRgbaColorf *pObjColor)
{
	short				nVerts = WORDVAL (p+2);
	tG3ModelVertex	*pmv;
	short				*pfv;
	tUVL				*uvl;
	grsBitmap		*bmP;
	tRgbaColorf		baseColor;
	fVector3			n, *pvn;
	short				i, j;
	ushort			c;
	char				bTextured;

Assert (pmf - pm->pFaces < pm->nFaces);
if (modelBitmaps && *modelBitmaps) {
	bTextured = 1;
	pmf->nBitmap = WORDVAL (p+28);
	bmP = modelBitmaps [pmf->nBitmap];
	if (pObjColor) {
		ubyte c = bmP->bmAvgColor;
		pObjColor->red = CPAL2Tr (gamePalette, c);
		pObjColor->green = CPAL2Tg (gamePalette, c);
		pObjColor->blue = CPAL2Tb (gamePalette, c);
		}
	baseColor.red = baseColor.green = baseColor.blue = baseColor.alpha = 1;
	i = (int) (bmP - gameData.pig.tex.bitmaps [0]);
	pmf->bThruster = (i == 24) || ((i >= 1741) && (i <= 1745));
	}
else {
	bTextured = 0;
	pmf->nBitmap = -1;
	c = WORDVAL (p + 28);
	baseColor.red = (float) PAL2RGBA (((c >> 10) & 31) << 1) / 255.0f;
	baseColor.green = (float) PAL2RGBA (((c >> 5) & 31) << 1) / 255.0f;
	baseColor.blue = (float) PAL2RGBA ((c & 31) << 1) / 255.0f;
	baseColor.alpha = GrAlpha ();
	if (pObjColor)
		*pObjColor = baseColor;
	}
pmf->vNormal = *pn;
pmf->nIndex = pm->iFaceVert;
pmv = pm->pFaceVerts + pm->iFaceVert;
pvn = pm->pVertNorms + pm->iFaceVert;
if (psm->nIndex < 0)
	psm->nIndex = pm->iFaceVert;
pmf->nVerts = nVerts;
if ((pmf->bGlow = (nGlow >= 0)))
	nGlow = -1;
uvl = (tUVL *) (p + 30 + (nVerts | 1) * 2);
VmVecFixToFloat3 (&n, pn);
for (i = nVerts, pfv = WORDPTR (p+30); i; i--, pfv++, uvl++, pmv++, pvn++) {
	j = *pfv;
	pmv->vertex = pm->pVerts [j];
	pmv->texCoord.v.u = f2fl (uvl->u);
	pmv->texCoord.v.v = f2fl (uvl->v);
	pmv->renderColor =
	pmv->baseColor = baseColor;
	pmv->bTextured = bTextured;
	pmv->nIndex = j;
	pmv->normal = *pvn = n;
	G3SetSubModelMinMax (psm, &pmv->vertex);
	}
pm->iFaceVert += nVerts;
pmf++;
pm->iFace++;
psm->nFaces++;
return pmf;
}

//------------------------------------------------------------------------------

bool G3GetModelItems (void *modelP, vmsAngVec *pAnimAngles, tG3Model *pm, int nThis, int nParent, 
							 grsBitmap **modelBitmaps, tRgbaColorf *pObjColor)
{
	ubyte				*p = modelP;
	tG3SubModel		*psm = pm->pSubModels + nThis;
	tG3ModelFace	*pmf = pm->pFaces + pm->iFace;
	int				nChild;

G3CheckAndSwap (modelP);
nGlow = -1;
if (!psm->pFaces) {
	psm->pFaces = pmf;
	psm->nIndex = -1;
	psm->nParent = nParent;
	}
for (;;)
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int i, n = WORDVAL (p+2);
			fVector3 *pfv = pm->pVerts;
			vmsVector *pv = VECPTR(p+4);
			for (i = n; i; i--)
				VmVecFixToFloat3 (pfv++, pv++);
			p += n * sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int i, n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			fVector3 *pfv = pm->pVerts + s;
			vmsVector *pv = VECPTR(p+8);
			for (i = n; i; i--)
				VmVecFixToFloat3 (pfv++, pv++);
			p += n * sizeof (vmsVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			pmf = G3AddModelFace (pm, psm, pmf, VECPTR (p+16), p, NULL, pObjColor);
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			pmf = G3AddModelFace (pm, psm, pmf, VECPTR (p+16), p, modelBitmaps, pObjColor);
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			G3GetModelItems (p + WORDVAL (p+28), pAnimAngles, pm, nThis, nParent, modelBitmaps, pObjColor);
			pmf = pm->pFaces + pm->iFace;
			G3GetModelItems (p + WORDVAL (p+30), pAnimAngles, pm, nThis, nParent, modelBitmaps, pObjColor);
			pmf = pm->pFaces + pm->iFace;
			p += 32;
			break;

		case OP_RODBM:
			p+=36;
			break;

		case OP_SUBCALL: {
			nChild = ++pm->iSubModel;
			pm->pSubModels [nChild].vOffset = *VECPTR (p+4);
			pm->pSubModels [nChild].nAngles = WORDVAL (p+2);
			G3InitSubModelMinMax (pm->pSubModels + nChild);
			G3GetModelItems (p + WORDVAL (p+16), pAnimAngles, pm, nChild, nThis, modelBitmaps, pObjColor);
			pmf = pm->pFaces + pm->iFace;
			p += 20;
			break;
			}

		case OP_GLOW:
			nGlow = WORDVAL (p+2);
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
		}
return 1;
}

//------------------------------------------------------------------------------

inline int G3CmpFaces (tG3ModelFace *pmf, tG3ModelFace *pm, grsBitmap *pTextures)
{
if (pTextures) {
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

#define	G3_ALLOC(_buf,_count,_type,_fill) \
			if (((_buf) = (_type *) D2_ALLOC (_count * sizeof (_type)))) \
				memset (_buf, (char) _fill, _count * sizeof (_type)); \
			else \
				return G3FreeModelItems (pm);

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

int G3AllocModel (tG3Model *pm)
{
G3_ALLOC (pm->pVerts, pm->nVerts, fVector3, 0);
G3_ALLOC (pm->pColor, pm->nVerts, tFaceColor, 0xff);
if (gameStates.ogl.bHaveVBOs) {
	int i;
	glGenBuffers (1, &pm->vboDataHandle);
	if ((i = glGetError ())) {
		glGenBuffers (1, &pm->vboDataHandle);
		if ((i = glGetError ())) {
#	ifdef _DEBUG
			HUDMessage (0, "glGenBuffers failed (%d)", i);
#	endif
			gameStates.ogl.bHaveVBOs = 0;
			return G3FreeModelItems (pm);
			}
		}
	glBindBuffer (GL_ARRAY_BUFFER_ARB, pm->vboDataHandle);
	if ((i = glGetError ())) {
#	ifdef _DEBUG
		HUDMessage (0, "glBindBuffer failed (%d)", i);
#	endif
		gameStates.ogl.bHaveVBOs = 0;
		return G3FreeModelItems (pm);
		}
	glBufferData (GL_ARRAY_BUFFER, pm->nFaceVerts * sizeof (tG3RenderVertex), NULL, GL_STATIC_DRAW_ARB);
	pm->pVertBuf [1] = (tG3RenderVertex *) glMapBuffer (GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	pm->vboIndexHandle = 0;
	glGenBuffers (1, &pm->vboIndexHandle);
	if (pm->vboIndexHandle) {
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->vboIndexHandle);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->nFaceVerts * sizeof (short), NULL, GL_STATIC_DRAW_ARB);
		pm->pIndex [1] = (short *) glMapBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
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

void G3SetupModel (tG3Model *pm, int bHires)
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
	G3SortFaces (psm, 0, psm->nFaces - 1, pTextures);
	G3SortFaceVerts (pm, psm, pSortedVerts);
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
pmv = pSortedVerts;
for (i = pm->nFaceVerts; i; i--, pt++, pc++, pv++, pn++, pmv++) {
	*pv = pmv->vertex;
	*pn = pmv->normal;
	*pc = pmv->baseColor;
	*pt = pmv->texCoord;
	}
if (pm->pVertBuf [1])
	memcpy (pm->pVertBuf [1], pm->pVertBuf [0], pm->nFaceVerts * sizeof (tG3RenderVertex));
if (pm->pIndex [1])
	memcpy (pm->pIndex [1], pm->pIndex [0], pm->nFaceVerts * sizeof (short));
memcpy (pm->pFaceVerts, pSortedVerts, pm->nFaceVerts * sizeof (tG3ModelVertex));
pm->bValid = 1;
if (gameStates.ogl.bHaveVBOs) {
	glUnmapBuffer (GL_ARRAY_BUFFER_ARB);
	glBindBuffer (GL_ARRAY_BUFFER_ARB, 0);
	glUnmapBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB);
	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
G3_FREE (pm->pSortedVerts);
}

//------------------------------------------------------------------------------

int G3CountOOFModelItems (tOOFObject *po, tG3Model *pm)
{
	tOOF_subObject	*pso;
	tOOF_face		*pf;
	int				i, j;

i = po->nSubObjects;
pm->nSubModels = i;
pm->nFaces = 0;
pm->nVerts = 0;
pm->nFaceVerts = 0;
for (pso = po->pSubObjects; i; i--, pso++) {
	j = pso->faces.nFaces;
	pm->nFaces += j;
	pm->nVerts += pso->nVerts;
	for (pf = pso->faces.pFaces; j; j--, pf++)
		pm->nFaceVerts += pf->nVerts;
	}
return 1;
}

//------------------------------------------------------------------------------

int G3GetOOFModelItems (int nModel, tOOFObject *po, tG3Model *pm)
{
	tOOF_subObject	*pso;
	tOOF_face		*pof;
	tOOF_faceVert	*pfv;
	tG3SubModel		*psm;
	fVector3			*pvn = pm->pVertNorms, vNormal;
	tG3ModelVertex	*pmv = pm->pFaceVerts;
	tG3ModelFace	*pmf = pm->pFaces;
	int				h, i, j, n, nIndex = 0;

for (i = po->nSubObjects, pso = po->pSubObjects, psm = pm->pSubModels; i; i--, pso++, psm++) {
	psm->nParent = pso->nParent;
	if (psm->nParent < 0)
		pm->iSubModel = (short) (psm - pm->pSubModels);
	psm->vOffset.p.x = fl2f (pso->vOffset.x);
	psm->vOffset.p.y = fl2f (pso->vOffset.y);
	psm->vOffset.p.z = fl2f (pso->vOffset.z);
	psm->nAngles = 0;
	j = pso->faces.nFaces;
	psm->nIndex = nIndex;
	psm->nFaces = j;
	psm->pFaces = pmf;
	for (pof = pso->faces.pFaces; j; j--, pof++, pmf++) {
		pmf->nIndex = nIndex;
		pmf->bThruster = 0;
		pmf->bGlow = 0;
		n = pof->nVerts;
		pmf->nVerts = n;
		if (pof->bTextured)
			pmf->nBitmap = pof->texProps.nTexId;
		else
			pmf->nBitmap = -1;
		pfv = pof->pVerts;
		h = pfv->nIndex;
		if (nModel > 200) {
			VmVecNormalf ((fVector *) &vNormal, 
							  (fVector *) (pso->pvVerts + pfv [0].nIndex), 
							  (fVector *) (pso->pvVerts + pfv [1].nIndex), 
							  (fVector *) (pso->pvVerts + pfv [2].nIndex));
			}
		else
			memcpy (&vNormal, &pof->vNormal, sizeof (fVector3));
		for (; n; n--, pfv++, pmv++, pvn++) {
			h = pfv->nIndex;
			pmv->nIndex = h;
			pmv->texCoord.v.u = pfv->fu;
			pmv->texCoord.v.v = pfv->fv;
			pmv->normal = vNormal;
			memcpy (pm->pVerts + h, pso->pvVerts + h, sizeof (fVector3));
			memcpy (&pmv->vertex, pso->pvVerts + h, sizeof (fVector3));
			G3SetSubModelMinMax (psm, &pmv->vertex);
			*pvn = vNormal;
			if ((pmv->bTextured = pof->bTextured))
				pmv->baseColor.red = 
				pmv->baseColor.green =
				pmv->baseColor.blue = 1.0f;
			else {
				pmv->baseColor.red = (float) pof->texProps.color.r / 255.0f;
				pmv->baseColor.green = (float) pof->texProps.color.g / 255.0f;
				pmv->baseColor.blue = (float) pof->texProps.color.b / 255.0f;
				}
			pmv->baseColor.alpha = 1.0f;
			nIndex++;
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int G3ShiftModel (tObject *objP, int nModel, int bHires)
{
#if 0
return 0;
#else
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel		*psm;
	int				i;
	fVector			vOffset;
	tG3ModelVertex	*pmv;

#if 0
if (bHires)
	return 0;
#endif
if (IsMultiGame)
	return 0;
if ((objP->nType != OBJ_PLAYER) && (objP->nType != OBJ_ROBOT))
	return 0;
VmVecFixToFloat (&vOffset, gameData.models.offsets + nModel);
if (!(vOffset.p.x || vOffset.p.y || vOffset.p.z))
	return 0;
for (i = pm->nFaceVerts, pmv = pm->pFaceVerts; i; i--, pmv++) {
	pmv->vertex.p.x += vOffset.p.x;
	pmv->vertex.p.y += vOffset.p.y;
	pmv->vertex.p.z += vOffset.p.z;
	}
for (i = pm->nSubModels, psm = pm->pSubModels; i; i--, psm++) {
	psm->vMin.p.x += vOffset.p.x;
	psm->vMin.p.y += vOffset.p.y;
	psm->vMin.p.z += vOffset.p.z;
	psm->vMax.p.x += vOffset.p.x;
	psm->vMax.p.y += vOffset.p.y;
	psm->vMax.p.z += vOffset.p.z;
	}
return 1;
#endif
}

//------------------------------------------------------------------------------

void G3SubModelSize (tObject *objP, int nModel, int nSubModel, vmsVector *vOffset, int bHires)
{
	tG3Model		*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel	*psm = pm->pSubModels + nSubModel;
	tHitbox		*phb = gameData.models.hitboxes [nModel].hitboxes + nSubModel + 1;
	int			i, j;

if (vOffset)
	VmVecAdd (&phb->vOffset, vOffset, &psm->vOffset);	//compute absolute offset (i.e. including offsets of all parent submodels)
else
	phb->vOffset = psm->vOffset;
phb->vMin.p.x = fl2f (psm->vMin.p.x);
phb->vMin.p.y = fl2f (psm->vMin.p.y);
phb->vMin.p.z = fl2f (psm->vMin.p.z);
phb->vMax.p.x = fl2f (psm->vMax.p.x);
phb->vMax.p.y = fl2f (psm->vMax.p.y);
phb->vMax.p.z = fl2f (psm->vMax.p.z);
phb->vSize.p.x = (phb->vMax.p.x - phb->vMin.p.x) / 2;
phb->vSize.p.y = (phb->vMax.p.y - phb->vMin.p.y) / 2;
phb->vSize.p.z = (phb->vMax.p.z - phb->vMin.p.z) / 2;
for (i = 0, j = pm->nSubModels, psm = pm->pSubModels; i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3SubModelSize (objP, nModel, i, &phb->vOffset, bHires);
}

//------------------------------------------------------------------------------

fix G3ModelSize (tObject *objP, tG3Model *pm, int nModel, int bHires)
{
	int			nSubModels = pm->nSubModels;
	tG3SubModel	*psm;
	int			i;
	tHitbox		*phb = gameData.models.hitboxes [nModel].hitboxes;
	vmsVector	hv;
	double		dx, dy, dz;

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
				G3SubModelSize (objP, nModel, i, NULL, bHires);
		}
	else
		G3SubModelSize (objP, nModel, 0, NULL, bHires);
	// determine min and max size
	for (i = 1, psm = pm->pSubModels; i <= nSubModels; i++, psm++) {
		phb [i].vMin.p.x = fl2f (psm->vMin.p.x);
		phb [i].vMin.p.y = fl2f (psm->vMin.p.y);
		phb [i].vMin.p.z = fl2f (psm->vMin.p.z);
		phb [i].vMax.p.x = fl2f (psm->vMax.p.x);
		phb [i].vMax.p.y = fl2f (psm->vMax.p.y);
		phb [i].vMax.p.z = fl2f (psm->vMax.p.z);
		dx = (phb [i].vMax.p.x - phb [i].vMin.p.x) / 2;
		dy = (phb [i].vMax.p.y - phb [i].vMin.p.y) / 2;
		dz = (phb [i].vMax.p.z - phb [i].vMin.p.z) / 2;
		phb [i].vSize.p.x = (fix) dx;
		phb [i].vSize.p.y = (fix) dy;
		phb [i].vSize.p.z = (fix) dz;
		VmVecAdd (&hv, &phb [i].vMin, &phb [i].vOffset);
		if (phb [0].vMin.p.x > hv.p.x)
			phb [0].vMin.p.x = hv.p.x;
		if (phb [0].vMin.p.y > hv.p.y)
			phb [0].vMin.p.y = hv.p.y;
		if (phb [0].vMin.p.z > hv.p.z)
			phb [0].vMin.p.z = hv.p.z;
		VmVecAdd (&hv, &phb [i].vMax, &phb [i].vOffset);
		if (phb [0].vMax.p.x < hv.p.x)
			phb [0].vMax.p.x = hv.p.x;
		if (phb [0].vMax.p.y < hv.p.y)
			phb [0].vMax.p.y = hv.p.y;
		if (phb [0].vMax.p.z < hv.p.z)
			phb [0].vMax.p.z = hv.p.z;
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
	} while (G3ShiftModel (objP, nModel, bHires));
dx = (phb [0].vMax.p.x - phb [0].vMin.p.x);
dy = (phb [0].vMax.p.y - phb [0].vMin.p.y);
dz = (phb [0].vMax.p.z - phb [0].vMin.p.z);
phb [0].vSize.p.x = (fix) dx / 2;
phb [0].vSize.p.y = (fix) dy / 2;
phb [0].vSize.p.z = (fix) dz / 2;
gameData.models.hitboxes [nModel].nSubModels = nSubModels;
for (i = 0; i <= nSubModels; i++)
	ComputeHitbox (nModel, i);
return (fix) (sqrt (dx * dx + dy * dy + dz + dz) / 2);
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
	static int nParents [] = {6, 7, 5, 4, 9, 10, 3, 3};

	tG3SubModel		*psm;
	tOOF_point		*pp;
	int				i;

for (i = 0, pp = po->gunPoints.pPoints; i < (po->gunPoints.nPoints = N_PLAYER_GUNS); i++, pp++) {
	if (nParents [i] >= pm->nSubModels)
		continue;
	psm = pm->pSubModels + nParents [i];
	pp->vPos.x = (psm->vMax.p.x + psm->vMin.p.x) / 2;
	if (3 == (pp->nParent = nParents [i])) {
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
	psm = pm->pSubModels + pp->nParent;
	pp->vPos.x = (psm->vMax.p.x + psm->vMin.p.x) / 2;
	pp->vPos.y = (psm->vMax.p.y + psm->vMin.p.y) / 2;
  	pp->vPos.z = psm->vMax.p.z;
	}
}

//------------------------------------------------------------------------------

void G3SetGunPoints (tObject *objP, tG3Model *pm, int nModel)
{
	vmsVector		*vGunPoints;
	tOOFObject		*po = gameData.models.oofModels [1][nModel];
	tOOF_subObject	*pso;
	tOOF_point		*pp;
	int				nParent, i, j;

if (!po)
	po = gameData.models.oofModels [0][nModel];
pp = po->gunPoints.pPoints;
if (objP->nType == OBJ_PLAYER)
	SetShipGunPoints (po, pm); 
else if (objP->nType == OBJ_ROBOT)
	SetRobotGunPoints (po, pm); 
else {
	gameData.models.gunInfo [nModel].nGuns = 0;
	return;
	}
if (j = po->gunPoints.nPoints) {
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

//------------------------------------------------------------------------------

int G3BuildModelFromOOF (tObject *objP, int nModel)
{
	tOOFObject	*po = gameData.models.oofModels [1][nModel];
	tG3Model		*pm;

if (!po) {
	po = gameData.models.oofModels [0][nModel];
	if (!po)
		return 0;
	}
#ifdef _DEBUG
HUDMessage (0, "optimizing model");
#endif
pm = gameData.models.g3Models [1] + nModel;
G3CountOOFModelItems (po, pm);
if (!G3AllocModel (pm))
	return 0;
G3InitSubModelMinMax (pm->pSubModels);
G3GetOOFModelItems (nModel, po, pm);
pm->pTextures = po->textures.pBitmaps;
gameData.models.polyModels [nModel].rad = G3ModelSize (objP, pm, nModel, 1);
G3SetupModel (pm, 1);
#if 1
G3SetGunPoints (objP, pm, nModel);
#endif
return -1;
}

//------------------------------------------------------------------------------

int G3BuildModel (tObject *objP, int nModel, tPolyModel *pp, grsBitmap **modelBitmaps, tRgbaColorf *pObjColor, int bHires)
{
	tG3Model	*pm = gameData.models.g3Models [bHires] + nModel;

if (pm->bValid > 0)
	return 1;
if (pm->bValid < 0)
	return 0;
if (bHires)
	return G3BuildModelFromOOF (objP, nModel);
if (!pp->modelData)
	return 0;
pm->nSubModels = 1;
#ifdef _DEBUG
HUDMessage (0, "optimizing model");
#endif
G3CountModelItems (pp->modelData, &pm->nSubModels, &pm->nVerts, &pm->nFaces, &pm->nFaceVerts);
if (!G3AllocModel (pm))
	return 0;
G3InitSubModelMinMax (pm->pSubModels);
G3GetModelItems (pp->modelData, NULL, pm, 0, -1, modelBitmaps, pObjColor);
gameData.models.polyModels [nModel].rad = G3ModelSize (objP, pm, nModel, 0);
G3SetupModel (pm, 0);
pm->iSubModel = 0;
return -1;
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
for (i = pm->nSubModels, psm = pm->pSubModels; i; i--, psm++, phb++) {
	phb->vMin.p.x = fl2f (psm->vMin.p.x);
	phb->vMin.p.y = fl2f (psm->vMin.p.y);
	phb->vMin.p.z = fl2f (psm->vMin.p.z);
	phb->vMax.p.x = fl2f (psm->vMax.p.x);
	phb->vMax.p.y = fl2f (psm->vMax.p.y);
	phb->vMax.p.z = fl2f (psm->vMax.p.z);
	}
return pm->nSubModels;
}

//------------------------------------------------------------------------------

#if !G3_HW_LIGHTING

typedef struct tG3ThreadInfo {
	tObject		*objP;
	tG3Model		*pm;
	tThreadInfo	ti [2];
} tG3ThreadInfo;

static tG3ThreadInfo g3ti;

#endif

//------------------------------------------------------------------------------

void G3DynLightModel (tObject *objP, tG3Model *pm, short iVerts, short nVerts, short iFaceVerts, short nFaceVerts)
{
	fVector			vPos, vVertex;
	fVector3			*pv, *pn;
	tG3ModelVertex	*pmv;
	tFaceColor		*pc;
	float				fAlpha = GrAlpha ();
	int				h, i, bEmissive = (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];

if (!gameStates.render.bBrightObject) {
	VmVecFixToFloat (&vPos, &objP->position.vPos);
	for (i = iVerts, pv = pm->pVerts + iVerts, pn = pm->pVertNorms + iVerts, pc = pm->pColor + iVerts; 
		  i < nVerts; 
		  i++, pv++, pn++, pc++) {
		pc->index = 0;
		VmVecAddf (&vVertex, &vPos, (fVector *) pv);
		G3VertexColor ((fVector *) pn, &vVertex, i, pc, NULL, 1, 0, 0);
		}
	}
for (i = iFaceVerts, h = iFaceVerts, pmv = pm->pFaceVerts + iFaceVerts; i < nFaceVerts; i++, h++, pmv++) {
	if (gameStates.render.bBrightObject || bEmissive) {
		pm->pVBColor [h] = pmv->baseColor;
		pm->pVBColor [h].alpha = fAlpha;
		}
	else if (pmv->bTextured)
		pm->pVBColor [h] = pm->pColor [pmv->nIndex].color;
	else {
		pc = pm->pColor + pmv->nIndex;
		pm->pVBColor [h].red = pmv->baseColor.red * pc->color.red;
		pm->pVBColor [h].green = pmv->baseColor.green * pc->color.green;
		pm->pVBColor [h].blue = pmv->baseColor.blue * pc->color.blue;
		pm->pVBColor [h].alpha = pmv->baseColor.alpha;
		}
	}
}

//------------------------------------------------------------------------------

void G3LightModel (tObject *objP, int nModel, fix xModelLight, fix *xGlowValues, int bHires)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3ModelVertex	*pmv;
	tG3ModelFace	*pmf;
	tRgbaColorf		baseColor, *colorP;
	float				fLight, fAlpha = (float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS; 
	int				h, i, j, l;
	int				bEmissive = (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];

#ifdef _DEBUG
if (OBJ_IDX (objP) == nDbgObj)
	objP = objP;
#endif
#if 0
if (xModelLight > F1_0)
	xModelLight = F1_0;
#endif
if (SHOW_DYN_LIGHT && (gameOpts->ogl.bObjLighting || 
    (gameOpts->ogl.bLightObjects && (gameOpts->ogl.bLightPowerups || (objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT))))) {
	tiRender.objP = objP;
	tiRender.pm = pm;
	if (!RunRenderThreads (rtPolyModel))
		G3DynLightModel (objP, pm, 0, pm->nVerts, 0, pm->nFaceVerts);
	}
else {
	if (gameData.objs.color.index)
		baseColor = gameData.objs.color.color;
	else
		baseColor.red = baseColor.green = baseColor.blue = 1;
	baseColor.alpha = fAlpha;
	for (i = pm->nFaces, pmf = pm->pFaces; i; i--, pmf++) {
		if (pmf->nBitmap >= 0)
			colorP = &baseColor;
		else
			colorP = NULL;
		if (pmf->bGlow) 
			l = xGlowValues [nGlow];
		else if (bEmissive)
			l = F1_0;
		else {
			l = -VmVecDot (&viewInfo.view [0].fVec, &pmf->vNormal);
			l = 3 * f1_0 / 4 + l / 4;
			l = FixMul (l, xModelLight);
			}
		fLight = f2fl (l);
		for (j = pmf->nVerts, h = pmf->nIndex, pmv = pm->pFaceVerts + pmf->nIndex; j; j--, h++, pmv++) {
#if G3_DRAW_ARRAYS
			if (colorP) {
				pm->pVBColor [h].red = colorP->red * fLight;
				pm->pVBColor [h].green = colorP->green * fLight;
				pm->pVBColor [h].blue = colorP->blue * fLight;
				pm->pVBColor [h].alpha = fAlpha;
				}
			else {
				pm->pVBColor [h].red = pmv->baseColor.red * fLight;
				pm->pVBColor [h].green = pmv->baseColor.green * fLight;
				pm->pVBColor [h].blue = pmv->baseColor.blue * fLight;
				pm->pVBColor [h].alpha = fAlpha;
				}
#else
			if (colorP) {
				pmv->renderColor.red = colorP->red * fLight;
				pmv->renderColor.green = colorP->green * fLight;
				pmv->renderColor.blue = colorP->blue * fLight;
				pmv->renderColor.alpha = fAlpha;
				}
			else {
				pmv->renderColor.red = pmv->baseColor.red * fLight;
				pmv->renderColor.green = pmv->baseColor.green * fLight;
				pmv->renderColor.blue = pmv->baseColor.blue * fLight;
				pmv->renderColor.alpha = fAlpha;
				}
#endif
			}
		}
	}
}

//------------------------------------------------------------------------------

#if G3_SW_SCALING

void G3ScaleModel (int nModel, int bHires)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	float				fScale = gameData.models.nScale ? f2fl (gameData.models.nScale) : 1;
	int				i;
	fVector3			*pv;
	tG3ModelVertex	*pmv;

if (pm->fScale == fScale)
	return;
fScale /= pm->fScale;
for (i = pm->nVerts, pv = pm->pVerts; i; i--, pv++) {
	pv->p.x *= fScale;
	pv->p.y *= fScale;
	pv->p.z *= fScale;
	}
for (i = pm->nFaceVerts, pmv = pm->pFaceVerts; i; i--, pmv++)
	pmv->vertex = pm->pVerts [pmv->nIndex];
pm->fScale *= fScale;
}

#endif

//------------------------------------------------------------------------------

void G3GetThrusterPos (short nModel, tG3ModelFace *pmf, vmsVector *vOffset, int bHires)
{
	tG3Model				*pm = gameData.models.g3Models [bHires] + nModel;
	tG3ModelVertex		*pmv;
	fVector				v = {{0,0,0}}, vn, vo, vForward = {{0,0,1}};
	tModelThrusters	*mtP = gameData.models.thrusters + nModel;
	int					i, j;
	float					h, nSize;

if (mtP->nCount >= 2)
	return;
VmVecFixToFloat (&vn, &pmf->vNormal);
if (VmVecDotf (&vn, &vForward) > -1.0f / 3.0f)
	return;
for (i = 0, j = pmf->nVerts, pmv = pm->pFaceVerts + pmf->nIndex; i < j; i++)
	VmVecIncf (&v, (fVector *) &pmv [i].vertex);
v.p.x /= j;
v.p.y /= j;
v.p.z /= j;
v.p.z -= 1.0f / 16.0f;
#if 0
G3TransformPoint (&v, &v, 0);
#else
if (vOffset) {
	VmVecFixToFloat (&vo, vOffset);
	VmVecIncf (&v, &vo);
	}
#endif
if (mtP->nCount && (v.p.x == mtP->vPos [0].p.x) && (v.p.y == mtP->vPos [0].p.y) && (v.p.z == mtP->vPos [0].p.z))
	return;
mtP->vPos [mtP->nCount].p.x = fl2f (v.p.x);
mtP->vPos [mtP->nCount].p.y = fl2f (v.p.y);
mtP->vPos [mtP->nCount].p.z = fl2f (v.p.z);
if (vOffset)
	VmVecDecf (&v, &vo);
mtP->vDir [mtP->nCount] = pmf->vNormal;
VmVecNegate (mtP->vDir + mtP->nCount);
if (!mtP->nCount++) {
	for (i = 0, nSize = 1000000000; i < j; i++)
		if (nSize > (h = VmVecDistf (&v, (fVector *) &pmv [i].vertex)))
			nSize = h;
	mtP->fSize = nSize;// * 1.25f;
	}
}

//------------------------------------------------------------------------------

void G3DrawSubModel (tObject *objP, short nModel, short nSubModel, short nExclusive, grsBitmap **modelBitmaps, 
						   vmsAngVec *pAnimAngles, vmsVector *vOffset, int bHires, int bUseVBO, int nPass, int bTransparency)
{
	tG3Model			*pm = gameData.models.g3Models [bHires] + nModel;
	tG3SubModel		*psm = pm->pSubModels + nSubModel;
	tG3ModelFace	*pmf;
	grsBitmap		*bmP = NULL;
	vmsAngVec		*va = pAnimAngles ? pAnimAngles + psm->nAngles : &avZero;
	vmsVector		vo;
	int				i, j, bTransparent, bTextured = !(gameStates.render.bCloaked /*|| nPass*/);
	short				nId, nFaceVerts, nVerts, nIndex, nBitmap = -1;

// set the translation
vo = psm->vOffset;
if (gameData.models.nScale)
	VmVecScale (&vo, gameData.models.nScale);
if (vOffset && (nExclusive < 0)) {
	G3StartInstanceAngles (&vo, va);
	VmVecInc (&vo, vOffset);
	}
#if G3_DRAW_SUBMODELS
// render any dependent submodels
for (i = 0, j = pm->nSubModels, psm = pm->pSubModels; i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3DrawSubModel (objP, nModel, i, nExclusive, modelBitmaps, pAnimAngles, &vo, bHires, bUseVBO, nPass, bTransparency);
#endif
// render the faces
if ((nExclusive < 0) || (nSubModel == nExclusive)) {
	if (vOffset && (nSubModel == nExclusive))
		G3StartInstanceMatrix (vOffset, NULL);
	glDisable (GL_TEXTURE_2D);
	if (gameStates.render.bCloaked)
		glColor4f (0, 0, 0, GrAlpha ());
	for (psm = pm->pSubModels + nSubModel, i = psm->nFaces, pmf = psm->pFaces; i; ) {
		if (bTextured && (nBitmap != pmf->nBitmap)) {
			if (0 > (nBitmap = pmf->nBitmap)) 
				glDisable (GL_TEXTURE_2D);
			else {
				bmP = bHires ? pm->pTextures + nBitmap : modelBitmaps [nBitmap];
				glActiveTexture (GL_TEXTURE0);
				glClientActiveTexture (GL_TEXTURE0);
				glEnable (GL_TEXTURE_2D);
				bmP = BmOverride (bmP, -1);
				if (BM_FRAMES (bmP))
					bmP = BM_CURFRAME (bmP);
				if (OglBindBmTex (bmP, 1, 3))
					continue;
				OglTexWrap (bmP->glTexture, GL_REPEAT);
				}
			}
		nIndex = pmf->nIndex;
#if G3_DRAW_ARRAYS
#	if G3_ALLOW_TRANSPARENCY
		if (bHires) {
			bTransparent = bmP && ((bmP->bmProps.flags & BM_FLAG_TRANSPARENT) != 0);
			if (bTransparent != bTransparency) {
				if (bTransparent)
					pm->bHasTransparency = 1;
				pmf++;
				i--;
				continue;
				}
			}
#	endif
#if G3_DRAW_ARRAYS
		if ((nFaceVerts = pmf->nVerts) > 4) {
#else
		if ((nFaceVerts = pmf->nVerts) > 0) {
#endif
			if (!nPass && pmf->bThruster)
				G3GetThrusterPos (nModel, pmf, &vo, bHires);
			nVerts = nFaceVerts;
			pmf++;
			i--;
			}
		else { 
			nId = pmf->nId;
			nVerts = 0;
			do {
				if (!nPass && pmf->bThruster)
					G3GetThrusterPos (nModel, pmf, &vo, bHires);
				nVerts += nFaceVerts;
				pmf++;
				i--;
				} while (i && (pmf->nId == nId));
			}
#else
		nFaceVerts = pmf->nVerts;
		if (pmf->bThruster)
			G3GetThrusterPos (nModel, pmf, vOffset, bHires);
		nVerts = nFaceVerts;
		pmf++;
		i--;
#endif
#ifdef _DEBUG
		if (nFaceVerts == 8)
			nFaceVerts = nFaceVerts;
		if (nIndex + nVerts > pm->nFaceVerts)
			break;
#endif
#if G3_DRAW_ARRAYS
#	if G3_DRAW_RANGE_ELEMENTS
		if (glDrawRangeElements)
			if (bUseVBO)
				glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
											0, nVerts - 1, nVerts, GL_UNSIGNED_SHORT, 
											G3_BUFFER_OFFSET (nIndex * sizeof (short)));
			else
				glDrawRangeElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
											nIndex, nIndex + nVerts - 1, nVerts, GL_UNSIGNED_SHORT, 
											pm->pIndex [0] + nIndex);

		else
			if (bUseVBO)
				glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
									nVerts, GL_UNSIGNED_SHORT, G3_BUFFER_OFFSET (nIndex * sizeof (short)));
			else
				glDrawElements ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, 
									nVerts, GL_UNSIGNED_SHORT, pm->pIndex + nIndex);
#	else
		glDrawArrays ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN, nIndex, nVerts);
#	endif
#else
		{
		tG3ModelVertex	*pmv = pm->pFaceVerts + nIndex;
		glBegin ((nFaceVerts == 3) ? GL_TRIANGLES : (nFaceVerts == 4) ? GL_QUADS : GL_TRIANGLE_FAN);
		for (j = nVerts; j; j--, pmv++) {
			if (!gameStates.render.bCloaked) {
				glTexCoord2fv ((GLfloat *) &pmv->texCoord);
				glColor4fv ((GLfloat *) (pm->pVBColor + pmv->nIndex));
				}
			if (gameOpts->ogl.bObjLighting)
				glNormal3fv ((GLfloat *) &pmv->normal);
			glVertex3fv ((GLfloat *) &pmv->vertex);
			}
		glEnd ();
		pmv -= nVerts;
#	if 0
		glDisable (GL_TEXTURE_2D);
		glBegin (GL_LINE_LOOP);
		for (j = nVerts; j; j--, pmv++) {
			if (!gameStates.render.bCloaked) {
				glTexCoord2fv ((GLfloat *) &pmv->texCoord);
				glColor4fv ((GLfloat *) (pm->pVBColor + pmv->nIndex));
				}
			if (gameOpts->ogl.bObjLighting)
				glNormal3fv ((GLfloat *) &pmv->normal);
			glVertex3fv ((GLfloat *) &pmv->vertex);
			}
		glEnd ();
		if (bmP)
			glEnable (GL_TEXTURE_2D);
#	endif
		}
#endif
		}
	}
if ((nExclusive < 0) || (nSubModel == nExclusive))
	G3DoneInstance ();
}

//------------------------------------------------------------------------------

void G3DrawModel (tObject *objP, short nModel, short nSubModel, grsBitmap **modelBitmaps, 
						vmsAngVec *pAnimAngles, vmsVector *vOffset, int bHires, int bUseVBO, int bTransparency)
{
	tG3Model			*pm;
	tShaderLight	*psl;
	int				nPass, iLightSource = 0, iLight, nLights;
	int				bEmissive = objP && (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];
	int				bLighting = SHOW_DYN_LIGHT && gameOpts->ogl.bObjLighting && !(gameStates.render.bQueryCoronas || gameStates.render.bCloaked || bEmissive);
	GLenum			hLight;

if (bLighting) {
	nLights = gameData.render.lights.dynamic.shader.nActiveLights [0];
	OglEnableLighting (0); 
	}
else
	nLights = 1;
glEnable (GL_BLEND);
if (bEmissive)
	glBlendFunc (GL_ONE, GL_ONE);
else if (gameStates.render.bCloaked)
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
else if (bTransparency) {
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (0);
	}
else
	glBlendFunc (GL_ONE, GL_ZERO);
for (nPass = 0; nLights || !nPass; nPass++) {
	if (bLighting) {
		if (nPass) {
			glBlendFunc (GL_ONE, GL_ONE);
			glDepthMask (0);
			}
		OglSetupTransform (1);
		for (iLight = 0; (iLight < 8) && nLights; iLight++, nLights--, iLightSource++) { 
			psl = gameData.render.lights.dynamic.shader.activeLights [0][iLightSource];
			hLight = GL_LIGHT0 + iLight;
			glEnable (hLight);
//			sprintf (szLightSources + strlen (szLightSources), "%d ", (psl->nObject >= 0) ? -psl->nObject : psl->nSegment);
			glLightfv (hLight, GL_POSITION, (GLfloat *) (psl->pos));
			glLightfv (hLight, GL_DIFFUSE, (GLfloat *) &(psl->color));
			glLightfv (hLight, GL_SPECULAR, (GLfloat *) &(psl->color));
			glLightf (hLight, GL_CONSTANT_ATTENUATION, 0.1f / psl->brightness);
			glLightf (hLight, GL_LINEAR_ATTENUATION, 0.1f / psl->brightness);
			glLightf (hLight, GL_QUADRATIC_ATTENUATION, 0.01f / psl->brightness);
			if (psl->bSpot) {
#if 0
				psl = psl;
#else
				glLighti (hLight, GL_SPOT_EXPONENT, 12);
				glLighti (hLight, GL_SPOT_CUTOFF, 25);
				glLightfv (hLight, GL_SPOT_DIRECTION, (GLfloat *) &psl->dir);
#endif
				}
			}
		OglResetTransform (1);
		for (; iLight < 8; iLight++)
			glDisable (GL_LIGHT0 + iLight);
		}
	if (nSubModel < 0)
		G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	pm = gameData.models.g3Models [bHires] + nModel;
	if (bHires) {
		int i;
		for (i = 0; i < pm->nSubModels; i++)
			if (pm->pSubModels [i].nParent == -1) 
				G3DrawSubModel (objP, nModel, i, nSubModel, modelBitmaps, pAnimAngles, (nSubModel < 0) ? &pm->pSubModels->vOffset : vOffset, bHires, bUseVBO, nPass, bTransparency);
		}
	else
		G3DrawSubModel (objP, nModel, 0, nSubModel, modelBitmaps, pAnimAngles, (nSubModel < 0) ? &pm->pSubModels->vOffset : vOffset, bHires, bUseVBO, nPass, bTransparency);
	if (nSubModel < 0)
		G3DoneInstance ();
	if (!bLighting)
		break;
	}
if (bLighting) {
	OglDisableLighting ();
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask (1);
	}
//HUDMessage (0, "%s", szLightSources);
}

//------------------------------------------------------------------------------

void G3RenderDamageLightnings (tObject *objP, short nModel, short nSubModel, 
										 vmsAngVec *pAnimAngles, vmsVector *vOffset, int bHires)
{
	tG3Model			*pm;
	tG3SubModel		*psm;
	tG3ModelFace	*pmf;
	vmsAngVec		*va;
	vmsVector		vo;
	int				i, j;

pm = gameData.models.g3Models [bHires] + nModel;
if (pm->bValid < 1) {
	if (!bHires)
		return;
	pm = gameData.models.g3Models [0] + nModel;
	if (pm->bValid < 1)
		return;
	}
psm = pm->pSubModels + nSubModel;
va = pAnimAngles ? pAnimAngles + psm->nAngles : &avZero;
if (!(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bDamage))
	return;
if (!objP || (ObjectDamage (objP) > 0.5f))
	return;
// set the translation
vo = psm->vOffset;
if (gameData.models.nScale)
	VmVecScale (&vo, gameData.models.nScale);
if (vOffset) {
	G3StartInstanceAngles (&vo, va);
	VmVecInc (&vo, vOffset);
	}
// render any dependent submodels
for (i = 0, j = pm->nSubModels, psm = pm->pSubModels; i < j; i++, psm++)
	if (psm->nParent == nSubModel)
		G3RenderDamageLightnings (objP, nModel, i, pAnimAngles, &vo, bHires);
// render the lightnings
for (psm = pm->pSubModels + nSubModel, i = psm->nFaces, pmf = psm->pFaces; i; i--, pmf++) 
	RenderDamageLightnings (objP, NULL, pm->pFaceVerts + pmf->nIndex, pmf->nVerts);
if (vOffset)
	G3DoneInstance ();
}

//------------------------------------------------------------------------------

int G3HaveModel (int nModel)
{
return (gameData.models.g3Models [0][nModel].bValid != 0) || (gameData.models.g3Models [1][nModel].bValid != 0);
}

//------------------------------------------------------------------------------

int G3RenderModel (tObject *objP, short nModel, short nSubModel, tPolyModel *pp, grsBitmap **modelBitmaps, 
						 vmsAngVec *pAnimAngles, vmsVector *vOffset, fix xModelLight, fix *xGlowValues, tRgbaColorf *pObjColor)
{
	tG3Model	*pm = gameData.models.g3Models [1] + nModel;
	int		i, bHires = 1, bUseVBO = gameStates.ogl.bHaveVBOs && gameOpts->ogl.bObjLighting;

if (!objP)
	return 0;
if (gameStates.render.bQueryCoronas && gameStates.render.bCloaked)
	return 0;
#if G3_FAST_MODELS
if (!gameOpts->render.nPath)
#endif
	{
	gameData.models.g3Models [0][nModel].bValid =
	gameData.models.g3Models [1][nModel].bValid = -1;
	return 0;
	}
if (pm->bValid < 1) {
	if (pm->bValid) {
		i = 0;
		bHires = 0;
		}
	else {
		i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 1);
		if (i < 0)	//successfully built new model
			return gameStates.render.bBuildModels;
		pm->bValid = -1;
		}
	pm = gameData.models.g3Models [0] + nModel;
	if (pm->bValid < 0)
		return 0;
	if (!(i || pm->bValid)) {
		i = G3BuildModel (objP, nModel, pp, modelBitmaps, pObjColor, 0);
		if (i <= 0) {
			if (!i)
				pm->bValid = -1;
			return gameStates.render.bBuildModels;
			}
		}
	}
if (!(gameStates.render.bCloaked ? 
	   G3EnableClientStates (0, 0, 0, GL_TEXTURE0) : 
		G3EnableClientStates (1, 1, gameOpts->ogl.bObjLighting, GL_TEXTURE0)))
	return 0;
if (bUseVBO) {
	int i;
	glBindBuffer (GL_ARRAY_BUFFER_ARB, pm->vboDataHandle);
	if ((i = glGetError ())) {
		glBindBuffer (GL_ARRAY_BUFFER_ARB, pm->vboDataHandle);
		if ((i = glGetError ()))
			return 0;
		}
	}
else {
	pm->pVBVerts = (fVector3 *) pm->pVertBuf [0];
	pm->pVBNormals = pm->pVBVerts + pm->nFaceVerts;
	pm->pVBColor = (tRgbaColorf *) (pm->pVBNormals + pm->nFaceVerts);
	pm->pVBTexCoord = (tTexCoord2f *) (pm->pVBColor + pm->nFaceVerts);
	}
#if G3_SW_SCALING
G3ScaleModel (nModel);
#else
if (bHires)
	gameData.models.nScale = 0;
#endif
if (!(gameOpts->ogl.bObjLighting || gameStates.render.bQueryCoronas || gameStates.render.bCloaked))
	G3LightModel (objP, nModel, xModelLight, xGlowValues, bHires);
if (bUseVBO) {
	if (!gameStates.render.bCloaked) {
		glNormalPointer (GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->nFaceVerts * sizeof (fVector3)));
		glColorPointer (4, GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->nFaceVerts * 2 * sizeof (fVector3)));
		glTexCoordPointer (2, GL_FLOAT, 0, G3_BUFFER_OFFSET (pm->nFaceVerts * ((2 * sizeof (fVector3) + sizeof (tRgbaColorf)))));
		}
	glVertexPointer (3, GL_FLOAT, 0, G3_BUFFER_OFFSET (0));
	if (pm->vboIndexHandle)
		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, pm->vboIndexHandle);
	}
else 
	{
	if (!gameStates.render.bCloaked) {
		glTexCoordPointer (2, GL_FLOAT, 0, pm->pVBTexCoord);
		if (gameOpts->ogl.bObjLighting)
			glNormalPointer (GL_FLOAT, 0, pm->pVBNormals);
		glColorPointer (4, GL_FLOAT, 0, pm->pVBColor);
		}
	glVertexPointer (3, GL_FLOAT, 0, pm->pVBVerts);
	}
G3DrawModel (objP, nModel, nSubModel, modelBitmaps, pAnimAngles, vOffset, bHires, bUseVBO, 0);
if ((objP->nType != OBJ_DEBRIS) && bHires && pm->bHasTransparency)
	G3DrawModel (objP, nModel, nSubModel, modelBitmaps, pAnimAngles, vOffset, bHires, bUseVBO, 1);
glDisable (GL_TEXTURE_2D);
glBindBuffer (GL_ARRAY_BUFFER_ARB, 0);
glBindBuffer (GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
if (gameStates.render.bCloaked)
	G3DisableClientStates (0, 0, 0, -1);
else
	G3DisableClientStates (1, 1, gameOpts->ogl.bObjLighting, -1);
if (objP && ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT) || (objP->nType == OBJ_REACTOR))) {
	G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
	G3RenderDamageLightnings (objP, nModel, 0, pAnimAngles, NULL, bHires);
	G3DoneInstance ();
	}
return 1;
}

//------------------------------------------------------------------------------
//eof
