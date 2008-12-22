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

RenderModel::CFace *G3AddModelFace (RenderModel::CModel *pm, RenderModel::CSubModel *psm, RenderModel::CFace *pmf, 
												CFixVector *pn, ubyte *p, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	short							nVerts = WORDVAL (p+2);
	RenderModel::CVertex*	pmv;
	short*						pfv;
	tUVL*							uvl;
	CBitmap*						bmP;
	tRgbaColorf					baseColor;
	CFloatVector3				n, * pvn;
	short							i, j;
	ushort						c;
	char							bTextured;

if (!psm->m_faces)
	psm->m_faces = pmf;
Assert (pmf - pm->m_faces < pm->m_nFaces);
if (modelBitmaps && *modelBitmaps) {
	bTextured = 1;
	pmf->m_nBitmap = WORDVAL (p+28);
	bmP = modelBitmaps [pmf->m_nBitmap];
	if (objColorP)
		paletteManager.Game ()->ToRgbaf (bmP->AvgColorIndex (), *objColorP);
	baseColor.red = baseColor.green = baseColor.blue = baseColor.alpha = 1;
	i = (int) (bmP - gameData.pig.tex.bitmaps [0]);
	pmf->m_bThruster = (i == 24) || ((i >= 1741) && (i <= 1745));
	}
else {
	bTextured = 0;
	pmf->m_nBitmap = -1;
	c = WORDVAL (p + 28);
	baseColor.red = (float) PAL2RGBA (((c >> 10) & 31) << 1) / 255.0f;
	baseColor.green = (float) PAL2RGBA (((c >> 5) & 31) << 1) / 255.0f;
	baseColor.blue = (float) PAL2RGBA ((c & 31) << 1) / 255.0f;
	baseColor.alpha = GrAlpha ();
	if (objColorP)
		*objColorP = baseColor;
	}
pmf->m_nSubModel = psm - pm->m_subModels;
pmf->m_vNormal = *pn;
pmf->m_nIndex = pm->m_iFaceVert;
pmv = pm->m_faceVerts + pm->m_iFaceVert;
pvn = pm->m_vertNorms + pm->m_iFaceVert;
if (psm->m_nIndex < 0)
	psm->m_nIndex = pm->m_iFaceVert;
pmf->m_nVerts = nVerts;
if ((pmf->m_bGlow = (nGlow >= 0)))
	nGlow = -1;
uvl = reinterpret_cast<tUVL*> (p + 30 + (nVerts | 1) * 2);
n.Assign (*pn);
for (i = nVerts, pfv = WORDPTR (p+30); i; i--, pfv++, uvl++, pmv++, pvn++) {
	j = *pfv;
	Assert (pmv - pm->m_faceVerts < pm->m_nFaceVerts);
	pmv->m_vertex = pm->m_verts [j];
	pmv->m_texCoord.v.u = X2F (uvl->u);
	pmv->m_texCoord.v.v = X2F (uvl->v);
	pmv->m_renderColor =
	pmv->m_baseColor = baseColor;
	pmv->m_bTextured = bTextured;
	pmv->m_nIndex = j;
	pmv->m_normal = *pvn = n;
	psm->SetMinMax (&pmv->m_vertex);
	}
pm->m_iFaceVert += nVerts;
pm->m_iFace++;
psm->m_nFaces++;
return ++pmf;
}

//------------------------------------------------------------------------------

int G3GetPOFModelItems (void *modelDataP, CAngleVector *pAnimAngles, RenderModel::CModel *pm, int nThis, int nParent,
								int bSubObject, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	ubyte*						p = reinterpret_cast<ubyte*> (modelDataP);
	RenderModel::CSubModel*	psm = pm->m_subModels + nThis;
	RenderModel::CFace*		pmf = pm->m_faces + pm->m_iFace;
	int							nChild;
	short							nTag;

G3CheckAndSwap (modelDataP);
nGlow = -1;
if (bSubObject) {
	psm->InitMinMax ();
	psm->m_nIndex = -1;
	psm->m_nParent = nParent;
	psm->m_nBomb = -1;
	psm->m_nMissile = -1;
	psm->m_nGun = -1;
	psm->m_nGunPoint = -1;
	psm->m_bBullets = 0;
	psm->m_bThruster = 0;
	psm->m_bGlow = 0;
	psm->m_bRender = 1;
	}
for (;;) {
	nTag = WORDVAL (p);
	switch (nTag) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int i, n = WORDVAL (p+2);
			CFloatVector3 *pfv = pm->m_verts.Buffer ();
			CFixVector *pv = VECPTR (p+4);
			for (i = n; i; i--) {
				pfv->Assign (*pv);
				pfv++; pv++;
			}
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int i, n = WORDVAL (p+2);
			int s = WORDVAL (p+4);
			CFloatVector3 *pfv = pm->m_verts + s;
			CFixVector *pv = VECPTR (p+8);
			for (i = n; i; i--) {
				pfv->Assign (*pv);
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
			pmf = pm->m_faces + pm->m_iFace;
			if (!G3GetPOFModelItems (p + WORDVAL (p+30), pAnimAngles, pm, nThis, nParent, 0, modelBitmaps, objColorP))
				return 0;
			pmf = pm->m_faces + pm->m_iFace;
			p += 32;
			break;

		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL:
			nChild = ++pm->m_iSubModel;
			pm->m_subModels [nChild].m_vOffset = *VECPTR (p+4);
			pm->m_subModels [nChild].m_nAngles = WORDVAL (p+2);
			pm->m_subModels [nChild].InitMinMax ();
			if (!G3GetPOFModelItems (p + WORDVAL (p+16), pAnimAngles, pm, nChild, nThis, 1, modelBitmaps, objColorP))
				return 0;
			pmf = pm->m_faces + pm->m_iFace;
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

void G3AssignModelFaces (RenderModel::CModel *pm)
{
	int				i;
	ubyte				nSubModel = 255;
	RenderModel::CFace	*pmf;

for (pmf = pm->m_faces.Buffer (), i = pm->m_nFaces; i; i--, pmf++)
	if (pmf->m_nSubModel != nSubModel) {
		nSubModel = pmf->m_nSubModel;
		pm->m_subModels [nSubModel].m_faces = pmf;
		if (nSubModel == pm->m_nSubModels - 1)
			break;
		}
}

//------------------------------------------------------------------------------

int G3BuildModelFromPOF (CObject *objP, int nModel, tPolyModel *pp, CBitmap **modelBitmaps, tRgbaColorf *objColorP)
{
	RenderModel::CModel	*pm = gameData.models.renderModels [0] + nModel;

if (!pp->modelData)
	return 0;
pm->m_nSubModels = 1;
#if DBG
HUDMessage (0, "optimizing model");
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
PrintLog ("         optimizing POF model %d\n", nModel);
if (!G3CountPOFModelItems (pp->modelData.Buffer (), &pm->m_nSubModels, &pm->m_nVerts, &pm->m_nFaces, &pm->m_nFaceVerts))
	return 0;
if (!G3AllocModel (pm))
	return 0;
pm->m_subModels [0].InitMinMax ();
#if TRACE_TAGS
PrintLog ("building model for object type %d, id %d\n", objP->info.nType, objP->info.nId);
#endif
if (!G3GetPOFModelItems (pp->modelData.Buffer (), NULL, pm, 0, -1, 1, modelBitmaps, objColorP))
	return 0;
pm->m_faces.SortAscending ();
G3AssignModelFaces (pm);
memset (pm->m_teamTextures, 0xFF, sizeof (pm->m_teamTextures));
pm->m_nType = pp->nType;
gameData.models.polyModels [nModel].rad = pm->Size (objP, 0);
pm->Setup (0, 1);
pm->m_iSubModel = 0;
return -1;
}

//------------------------------------------------------------------------------
//eof
