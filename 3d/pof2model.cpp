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

#include "descent.h"
#include "interp.h"
#include "hitbox.h"
#include "gr.h"
#include "ogl_color.h"
#include "hiresmodels.h"
#include "buildmodel.h"

using namespace RenderModel;

//------------------------------------------------------------------------------

int32_t CModel::CountPOFModelItems (void *modelDataP, uint16_t *pnSubModels, uint16_t *pnVerts, uint16_t *pnFaces, uint16_t *pnFaceVerts)
{
	uint8_t *p = reinterpret_cast<uint8_t*> (modelDataP);

G3CheckAndSwap (modelDataP);
for (;;) {
	switch (WORDVAL (p)) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int32_t n = WORDVAL (p+2);
			if (*pnVerts < n) 
				*pnVerts = n;
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int32_t n = WORDVAL (p+2);
			int32_t s = WORDVAL (p+4);
			p += n * sizeof (CFixVector) + 8;
			if (*pnVerts < s + n) 
				*pnVerts = s + n;
			break;
			}

		case OP_FLATPOLY: {
			int32_t nVerts = WORDVAL (p+2);
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int32_t nVerts = WORDVAL (p + 2);
			(*pnFaces)++;
			(*pnFaceVerts) += nVerts;
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			if (!CountPOFModelItems (p + WORDVAL (p+28), pnSubModels, pnVerts, pnFaces, pnFaceVerts))
				return 0;
			if (!CountPOFModelItems (p + WORDVAL (p+30), pnSubModels, pnVerts, pnFaces, pnFaceVerts))
				return 0;
			p += 32;
			break;


		case OP_RODBM: {
			p += 36;
			break;
			}

		case OP_SUBCALL: {
			(*pnSubModels)++;
			if (!CountPOFModelItems (p + WORDVAL (p+16), pnSubModels, pnVerts, pnFaces, pnFaceVerts))
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

CFace* CModel::AddPOFFace (CSubModel* psm, CFace* pmf, CFixVector* pn, uint8_t* p, CArray<CBitmap*>& modelBitmaps, CFloatVector* objColorP, bool bTextured)
{
	uint16_t				nVerts = WORDVAL (p+2);
	CVertex*				pmv;
	uint16_t*				pfv;
	tUVL*					uvl;
	CFloatVector			baseColor;
	CFloatVector3		n, * pvn;
	uint16_t				i, j;
	uint16_t				c;

if (!psm->m_faces)
	psm->m_faces = pmf;
Assert (pmf - m_faces < m_nFaces);
if (bTextured) {
	pmf->m_nBitmap = WORDVAL (p+28);
	CBitmap* bmoP, *bmP = modelBitmaps [pmf->m_nBitmap];
	if (objColorP) {
		if ((bmoP = bmP->HasOverride ()))
			bmoP->GetAvgColor (objColorP);
		else if (bmP->Buffer ()) {	//don't set color if bitmap not loaded
			if (bmP->Palette ())
				bmP->Palette ()->ToRgbaf (bmP->AvgColorIndex (), *objColorP);
			else
				paletteManager.Game ()->ToRgbaf (bmP->AvgColorIndex (), *objColorP);
			}
		}
	baseColor.Red () = baseColor.Green () = baseColor.Blue () = baseColor.Alpha () = 1;
	i = (int32_t) (bmP - gameData.pig.tex.bitmaps [0]);
	pmf->m_bThruster = (i == 24) || ((i >= 1741) && (i <= 1745));
	}
else {
	bTextured = 0;
	pmf->m_nBitmap = -1;
	c = WORDVAL (p + 28);
	baseColor.Red () = (float) PAL2RGBA (((c >> 10) & 31) << 1) / 255.0f;
	baseColor.Green () = (float) PAL2RGBA (((c >> 5) & 31) << 1) / 255.0f;
	baseColor.Blue () = (float) PAL2RGBA ((c & 31) << 1) / 255.0f;
	baseColor.Alpha () = gameStates.render.grAlpha;
	if (objColorP)
		*objColorP = baseColor;
	}
pmf->m_nSubModel = psm - m_subModels;
Assert (pmf->m_nSubModel < m_nSubModels);
pmf->m_vNormal = *pn;
pmf->m_nIndex = m_iFaceVert;
pmv = m_faceVerts + m_iFaceVert;
pvn = m_vertNorms + m_iFaceVert;
if (psm->m_nVertexIndex [0] == (uint16_t) -1)
	psm->m_nVertexIndex [0] = m_iFaceVert;
pmf->m_nVerts = nVerts;
if ((pmf->m_bGlow = (nGlow >= 0)))
	nGlow = -1;
uvl = reinterpret_cast<tUVL*> (p + 30 + (nVerts | 1) * 2);
n.Assign (*pn);
Assert (m_iFaceVert + nVerts <= int32_t (m_faceVerts.Length ()));
Assert (m_iFaceVert + nVerts <= int32_t (m_vertNorms.Length ()));
for (i = nVerts, pfv = WORDPTR (p+30); i; i--, pfv++, uvl++, pmv++, pvn++) {
	j = *pfv;
	Assert (pmv - m_faceVerts < m_nFaceVerts);
	pmv->m_vertex = m_vertices [j];
	pmv->m_texCoord.v.u = X2F (uvl->u);
	pmv->m_texCoord.v.v = X2F (uvl->v);
	pmv->m_renderColor =
	pmv->m_baseColor = baseColor;
	pmv->m_bTextured = bTextured;
	pmv->m_nIndex = j;
	pmv->m_normal = *pvn = n;
	psm->SetMinMax (&pmv->m_vertex);
	}
m_iFaceVert += nVerts;
m_iFace++;
psm->m_nFaces++;
return ++pmf;
}

//------------------------------------------------------------------------------

int32_t CModel::GetPOFModelItems (void *modelDataP, CAngleVector *pAnimAngles, int32_t nThis, int32_t nParent,
											 int32_t bSubObject, CArray<CBitmap*>& modelBitmaps, CFloatVector *objColorP)
{
	uint8_t*		p = reinterpret_cast<uint8_t*> (modelDataP);
	CSubModel*	psm = m_subModels + nThis;
	CFace*		pmf = m_faces + m_iFace;
	int32_t		nChild;
	int16_t		nTag;

G3CheckAndSwap (modelDataP);
nGlow = -1;
if (bSubObject) {
	psm->InitMinMax ();
	psm->m_nVertexIndex [0] = (uint16_t) -1;
	psm->m_nParent = nParent;
	psm->m_nBomb = -1;
	psm->m_nMissile = -1;
	psm->m_nGun = -1;
	psm->m_nGunPoint = -1;
	psm->m_bBullets = 0;
	psm->m_bThruster = 0;
	psm->m_bGlow = 0;
	psm->m_bRender = 1;
	psm->m_nVertices = 0;
	}
for (;;) {
	nTag = WORDVAL (p);
	switch (nTag) {
		case OP_EOF:
			return 1;

		case OP_DEFPOINTS: {
			int32_t i, n = WORDVAL (p+2);
			int32_t h = 0;
			Assert (n <= int32_t (m_vertices.Length ()));
			CFloatVector3 *pfv = m_vertices.Buffer ();
			CFixVector *pv = VECPTR (p+4);
			for (i = n; i; i--) {
				pfv->Assign (*pv);
				pfv++; 
				pv++;
				m_vertexOwner [h].m_nOwner = (uint16_t) nThis;
				m_vertexOwner [h].m_nVertex = (uint16_t) h++;
				}
			psm->m_nVertices += n;
			p += n * sizeof (CFixVector) + 4;
			break;
			}

		case OP_DEFP_START: {
			int32_t i, n = WORDVAL (p+2);
			int32_t h = WORDVAL (p+4);
			CFloatVector3 *pfv = m_vertices + h;
			CFixVector *pv = VECPTR (p+8);
			for (i = n; i; i--) {
				pfv->Assign (*pv);
				pfv++; 
				pv++;
				m_vertexOwner [h].m_nOwner = (uint16_t) nThis;
				m_vertexOwner [h].m_nVertex = (uint16_t) h++;
				}
			psm->m_nVertices += n;
			p += n * sizeof (CFixVector) + 8;
			break;
			}

		case OP_FLATPOLY: {
			int32_t nVerts = WORDVAL (p+2);
			pmf = AddPOFFace (psm, pmf, VECPTR (p+16), p, modelBitmaps, objColorP, false);
			p += 30 + (nVerts | 1) * 2;
			break;
			}

		case OP_TMAPPOLY: {
			int32_t nVerts = WORDVAL (p + 2);
			pmf = AddPOFFace (psm, pmf, VECPTR (p+16), p, modelBitmaps, objColorP);
			p += 30 + (nVerts | 1) * 2 + nVerts * 12;
			break;
			}

		case OP_SORTNORM:
			if (!GetPOFModelItems (p + WORDVAL (p+28), pAnimAngles, nThis, nParent, 0, modelBitmaps, objColorP))
				return 0;
			pmf = m_faces + m_iFace;
			if (!GetPOFModelItems (p + WORDVAL (p+30), pAnimAngles, nThis, nParent, 0, modelBitmaps, objColorP))
				return 0;
			pmf = m_faces + m_iFace;
			p += 32;
			break;

		case OP_RODBM:
			p += 36;
			break;

		case OP_SUBCALL:
			nChild = ++m_iSubModel;
			m_subModels [nChild].m_vOffset = *VECPTR (p+4);
			m_subModels [nChild].m_nAngles = WORDVAL (p+2);
			m_subModels [nChild].InitMinMax ();
			if (!GetPOFModelItems (p + WORDVAL (p+16), pAnimAngles, nChild, nThis, 1, modelBitmaps, objColorP))
				return 0;
			pmf = m_faces + m_iFace;
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

void CModel::AssignPOFFaces (void)
{
	int32_t		i;
	uint8_t		nSubModel = 255;
	CFace*	pmf;

for (pmf = m_faces.Buffer (), i = m_nFaces; i; i--, pmf++)
	if (pmf->m_nSubModel != nSubModel) {
		nSubModel = pmf->m_nSubModel;
		m_subModels [nSubModel].m_faces = pmf;
		if (nSubModel == m_nSubModels - 1)
			break;
		}
}

//------------------------------------------------------------------------------

int32_t CModel::BuildFromPOF (CObject* objP, int32_t nModel, CPolyModel* pp, CArray<CBitmap*>& modelBitmaps, CFloatVector* objColorP)
{
if (!pp->Buffer ())
	return 0;
m_nModel = nModel;
m_nSubModels = 1;
#if DBG
HUDMessage (0, "optimizing model");
if (nModel == nDbgModel)
	BRP;
#endif
if (gameStates.app.nLogLevel > 1)
	PrintLog (1, "optimizing POF model %d\n", nModel);
if (!CountPOFModelItems (pp->Buffer (), &m_nSubModels, &m_nVerts, &m_nFaces, &m_nFaceVerts)) {
	if (gameStates.app.nLogLevel > 1)
		PrintLog (-1);
	return 0;
	}
if (!Create ()) {
	if (gameStates.app.nLogLevel > 1)
		PrintLog (-1);
	return 0;
	}
m_subModels [0].InitMinMax ();
if (!GetPOFModelItems (pp->Buffer (), NULL, 0, -1, 1, modelBitmaps, objColorP)) {
	if (gameStates.app.nLogLevel > 1)
		PrintLog (-1);
	return 0;
	}
m_faces.SortAscending ();
AssignPOFFaces ();
memset (m_teamTextures, 0xFF, sizeof (m_teamTextures));
m_nType = pp->Type ();
gameData.models.polyModels [0][nModel].SetRad (Size (objP, 0), 1);
Setup (0, 1);
m_iSubModel = 0;
if (gameStates.app.nLogLevel > 1)
	PrintLog (-1);
return -1;
}

//------------------------------------------------------------------------------
//eof
