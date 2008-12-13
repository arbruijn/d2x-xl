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

#include <math.h>
#include <stdlib.h>
#include "error.h"

#include "inferno.h"
#include "interp.h"
#include "hitbox.h"
#include "gr.h"
#include "ogl_color.h"
#include "hiresmodels.h"
#include "buildmodel.h"

//------------------------------------------------------------------------------

int G3CountPOFModelItems (void *modelP, short *pnSubModels, short *pnVerts, short *pnFaces, short *pnFaceVerts)
{
	ubyte *p = reinterpret_cast<ubyte*> (modelP);

G3CheckAndSwap (modelP);
for (;;) {
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int n = WORDVAL (p+2);
			(*pnVerts) += n;
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int n = WORDVAL (p+2);
			p += n * sizeof (CFixVector) + 8;
			(*pnVerts) += n;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			if (!G3CountPOFModelItems (p + WORDVAL (p+28), pnSubModels, pnVerts, pnFaces, pnFaceVerts))
				return 0;
			if (!G3CountPOFModelItems (p + WORDVAL (p+30), pnSubModels, pnVerts, pnFaces, pnFaceVerts))
				return 0;
			p += 32;
			break;


		case OP_RODBM: {
			p += 36;
			break;
			}

		case OP_SUBCALL: {
			(*pnSubModels)++;
			if (!G3CountPOFModelItems (p + WORDVAL (p+16), pnSubModels, pnVerts, pnFaces, pnFaceVerts))
				return 0;
			p += 20;
			break;
			}

		case OP_GLOW:
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

CRenderModelFace *G3AddModelFace (CRenderModel *pm, CRenderSubModel *psm, CRenderModelFace *pmf, CFixVector *pn, ubyte *p,
										CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	short				nVerts = WORDVAL (p+2);
	RenderModel::CVertex	*pmv;
	short				*pfv;
	tUVL				*uvl;
	CBitmap			*bmP;
	tRgbaColorf		baseColor;
	CFloatVector3			n, *pvn;
	short				i, j;
	ushort			c;
	char				bTextured;

if (!psm->faces)
	psm->faces = pmf;
Assert (pmf - pm->faces < pm->nFaces);
if (modelBitmaps && *modelBitmaps) {
	bTextured = 1;
	pmf->nBitmap = WORDVAL (p+28);
	bmP = modelBitmaps [pmf->nBitmap];
	if (objColorP)
		paletteManager.Game ()->ToRgbaf (bmP->AvgColorIndex (), *objColorP);
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
	if (objColorP)
		*objColorP = baseColor;
	}
pmf->nSubModel = psm - pm->subModels;
pmf->vNormal = *pn;
pmf->nIndex = pm->iFaceVert;
pmv = pm->faceVerts + pm->iFaceVert;
pvn = pm->vertNorms + pm->iFaceVert;
if (psm->nIndex < 0)
	psm->nIndex = pm->iFaceVert;
pmf->nVerts = nVerts;
if ((pmf->bGlow = (nGlow >= 0)))
	nGlow = -1;
uvl = reinterpret_cast<tUVL*> (p + 30 + (nVerts | 1) * 2);
n = pn->ToFloat3();
for (i = nVerts, pfv = WORDPTR (p+30); i; i--, pfv++, uvl++, pmv++, pvn++) {
	j = *pfv;
	Assert (pmv - pm->faceVerts < pm->nFaceVerts);
	pmv->vertex = pm->verts [j];
	pmv->texCoord.v.u = X2F (uvl->u);
	pmv->texCoord.v.v = X2F (uvl->v);
	pmv->renderColor =
	pmv->baseColor = baseColor;
	pmv->bTextured = bTextured;
	pmv->nIndex = j;
	pmv->normal = *pvn = n;
	G3SetSubModelMinMax (psm, &pmv->vertex);
	}
pm->iFaceVert += nVerts;
pm->iFace++;
psm->nFaces++;
return ++pmf;
}

//------------------------------------------------------------------------------

int G3GetPOFModelItems (void *modelP, vmsAngVec *pAnimAngles, CRenderModel *pm, int nThis, int nParent,
								int bSubObject, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	ubyte				*p = reinterpret_cast<ubyte*> (modelP);
	CRenderSubModel		*psm = pm->subModels + nThis;
	CRenderModelFace	*pmf = pm->faces + pm->iFace;
	int				nChild;
	short				nTag;

G3CheckAndSwap (modelP);
nGlow = -1;
if (bSubObject) {
	G3InitSubModelMinMax (psm);
	psm->nIndex = -1;
	psm->nParent = nParent;
	psm->nBomb = -1;
	psm->nMissile = -1;
	psm->nGun = -1;
	psm->nGunPoint = -1;
	psm->bBullets = 0;
	psm->bThruster = 0;
	psm->bGlow = 0;
	psm->bRender = 1;
	}
for (;;) {
	nTag = WORDVAL (p);
	switch (nTag) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int i, n = WORDVAL (p+2);
			CFloatVector3 *pfv = pm->verts.Buffer ();
			CFixVector *pv = VECPTR (p+4);
			for (i = n; i; i--) {
				*pfv = pv->ToFloat3();
				pfv++; pv++;
			}
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int i, n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			CFloatVector3 *pfv = pm->verts + s;
			CFixVector *pv = VECPTR (p+8);
			for (i = n; i; i--) {
				*pfv = pv->ToFloat3();
				pfv++; pv++;
			}
			p += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int nVerts = WORDVAL (p+2);
			pmf = G3AddModelFace (pm, psm, pmf, VECPTR (p+16), p, NULL, objColorP);
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int nVerts = WORDVAL (p + 2);
			pmf = G3AddModelFace (pm, psm, pmf, VECPTR (p+16), p, modelBitmaps, objColorP);
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			if (!G3GetPOFModelItems (p + WORDVAL (p+28), pAnimAngles, pm, nThis, nParent, 0, modelBitmaps, objColorP))
				return 0;
			pmf = pm->faces + pm->iFace;
			if (!G3GetPOFModelItems (p + WORDVAL (p+30), pAnimAngles, pm, nThis, nParent, 0, modelBitmaps, objColorP))
				return 0;
			pmf = pm->faces + pm->iFace;
			p += 32;
			break;

		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL:
			nChild = ++pm->iSubModel;
			pm->subModels [nChild].vOffset = *VECPTR (p+4);
			pm->subModels [nChild].nAngles = WORDVAL (p+2);
			G3InitSubModelMinMax (pm->subModels + nChild);
			if (!G3GetPOFModelItems (p + WORDVAL (p+16), pAnimAngles, pm, nChild, nThis, 1, modelBitmaps, objColorP))
				return 0;
			pmf = pm->faces + pm->iFace;
			p += 20;
			break;

		case OP_GLOW:
			nGlow = WORDVAL (p+2);
			p += 4;
			break;

		default:
			Error ("invalid polygon model\n");
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void G3SortModelFaces (CRenderModelFace *pmf, int left, int right)
{
	int	l = left,
			r = right;
	ubyte	m = pmf [(l + r) / 2].nSubModel;

do {
	while (pmf [l].nSubModel < m)
		l++;
	while (pmf [r].nSubModel > m)
		r--;
	if (l <= r) {
		if (l < r) {
			CRenderModelFace h = pmf [l];
			pmf [l] = pmf [r];
			pmf [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	G3SortModelFaces (pmf, l, right);
if (left < r)
	G3SortModelFaces (pmf, left, r);
}

//------------------------------------------------------------------------------

void G3AssignModelFaces (CRenderModel *pm)
{
	int				i;
	ubyte				nSubModel = 255;
	CRenderModelFace	*pmf;

for (pmf = pm->faces.Buffer (), i = pm->nFaces; i; i--, pmf++)
	if (pmf->nSubModel != nSubModel) {
		nSubModel = pmf->nSubModel;
		pm->subModels [nSubModel].faces = pmf;
		if (nSubModel == pm->nSubModels - 1)
			break;
		}
}

//------------------------------------------------------------------------------

int G3BuildModelFromPOF (CObject *objP, int nModel, tPolyModel *pp, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	CRenderModel	*pm = gameData.models.renderModels [0] + nModel;

if (!pp->modelData)
	return 0;
pm->nSubModels = 1;
#if DBG
HUDMessage (0, "optimizing model");
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
PrintLog ("         optimizing POF model %d\n", nModel);
if (!G3CountPOFModelItems (pp->modelData.Buffer (), &pm->nSubModels, &pm->nVerts, &pm->nFaces, &pm->nFaceVerts))
	return 0;
if (!G3AllocModel (pm))
	return 0;
G3InitSubModelMinMax (pm->subModels.Buffer ());
#if TRACE_TAGS
PrintLog ("building model for object type %d, id %d\n", objP->info.nType, objP->info.nId);
#endif
if (!G3GetPOFModelItems (pp->modelData.Buffer (), NULL, pm, 0, -1, 1, modelBitmaps, objColorP))
	return 0;
G3SortModelFaces (pm->faces.Buffer (), 0, pm->nFaces - 1);
G3AssignModelFaces (pm);
memset (pm->teamTextures, 0xFF, sizeof (pm->teamTextures));
pm->nType = pp->nType;
gameData.models.polyModels [nModel].rad = G3ModelSize (objP, pm, nModel, 0);
G3SetupModel (pm, 0, 1);
pm->iSubModel = 0;
return -1;
}

//------------------------------------------------------------------------------
//eof
