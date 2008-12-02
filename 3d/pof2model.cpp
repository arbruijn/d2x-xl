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
	ubyte *p = (ubyte *) modelP;

G3CheckAndSwap (modelP);
for (;;) {
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

tG3ModelFace *G3AddModelFace (tG3Model *pm, tG3SubModel *psm, tG3ModelFace *pmf, vmsVector *pn, ubyte *p,
										CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	short				nVerts = WORDVAL (p+2);
	tG3ModelVertex	*pmv;
	short				*pfv;
	tUVL				*uvl;
	CBitmap		*bmP;
	tRgbaColorf		baseColor;
	fVector3			n, *pvn;
	short				i, j;
	ushort			c;
	char				bTextured;

if (!psm->pFaces)
	psm->pFaces = pmf;
Assert (pmf - pm->pFaces < pm->nFaces);
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
pmf->nSubModel = psm - pm->pSubModels;
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
n = pn->ToFloat3();
for (i = nVerts, pfv = WORDPTR (p+30); i; i--, pfv++, uvl++, pmv++, pvn++) {
	j = *pfv;
	Assert (pmv - pm->pFaceVerts < pm->nFaceVerts);
	pmv->vertex = pm->pVerts [j];
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

int G3GetPOFModelItems (void *modelP, vmsAngVec *pAnimAngles, tG3Model *pm, int nThis, int nParent,
								int bSubObject, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	ubyte				*p = (ubyte *) modelP;
	tG3SubModel		*psm = pm->pSubModels + nThis;
	tG3ModelFace	*pmf = pm->pFaces + pm->iFace;
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
			fVector3 *pfv = pm->pVerts;
			vmsVector *pv = VECPTR (p+4);
			for (i = n; i; i--) {
				*pfv = pv->ToFloat3();
				pfv++; pv++;
			}
			p += n * sizeof (vmsVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int i, n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			fVector3 *pfv = pm->pVerts + s;
			vmsVector *pv = VECPTR (p+8);
			for (i = n; i; i--) {
				*pfv = pv->ToFloat3();
				pfv++; pv++;
			}
			p += n * sizeof (vmsVector) + 8;
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
			pmf = pm->pFaces + pm->iFace;
			if (!G3GetPOFModelItems (p + WORDVAL (p+30), pAnimAngles, pm, nThis, nParent, 0, modelBitmaps, objColorP))
				return 0;
			pmf = pm->pFaces + pm->iFace;
			p += 32;
			break;

		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL:
			nChild = ++pm->iSubModel;
			pm->pSubModels [nChild].vOffset = *VECPTR (p+4);
			pm->pSubModels [nChild].nAngles = WORDVAL (p+2);
			G3InitSubModelMinMax (pm->pSubModels + nChild);
			if (!G3GetPOFModelItems (p + WORDVAL (p+16), pAnimAngles, pm, nChild, nThis, 1, modelBitmaps, objColorP))
				return 0;
			pmf = pm->pFaces + pm->iFace;
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

void G3SortModelFaces (tG3ModelFace *pmf, int left, int right)
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
			tG3ModelFace h = pmf [l];
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

void G3AssignModelFaces (tG3Model *pm)
{
	int				i;
	ubyte				nSubModel = 255;
	tG3ModelFace	*pmf;

for (pmf = pm->pFaces, i = pm->nFaces; i; i--, pmf++)
	if (pmf->nSubModel != nSubModel) {
		nSubModel = pmf->nSubModel;
		pm->pSubModels [nSubModel].pFaces = pmf;
		if (nSubModel == pm->nSubModels - 1)
			break;
		}
}

//------------------------------------------------------------------------------

int G3BuildModelFromPOF (tObject *objP, int nModel, tPolyModel *pp, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	tG3Model	*pm = gameData.models.g3Models [0] + nModel;

if (!pp->modelData)
	return 0;
pm->nSubModels = 1;
#if DBG
HUDMessage (0, "optimizing model");
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
PrintLog ("         optimizing POF model %d\n", nModel);
if (!G3CountPOFModelItems (pp->modelData, &pm->nSubModels, &pm->nVerts, &pm->nFaces, &pm->nFaceVerts))
	return 0;
if (!G3AllocModel (pm))
	return 0;
G3InitSubModelMinMax (pm->pSubModels);
#if TRACE_TAGS
PrintLog ("building model for object type %d, id %d\n", objP->info.nType, objP->info.nId);
#endif
if (!G3GetPOFModelItems (pp->modelData, NULL, pm, 0, -1, 1, modelBitmaps, objColorP))
	return 0;
G3SortModelFaces (pm->pFaces, 0, pm->nFaces - 1);
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
