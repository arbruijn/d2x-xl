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
#include "hiresmodels.h"
#include "buildmodel.h"

//------------------------------------------------------------------------------

int G3CountOOFModelItems (OOF::CModel *po, RenderModel::CModel *pm)
{
	OOF::CSubModel	*pso;
	OOF::CFace		*pf;
	int						i, j;

i = po->m_nSubModels;
pm->m_nSubModels = i;
pm->m_nFaces = 0;
pm->m_nVerts = 0;
pm->m_nFaceVerts = 0;
for (pso = po->m_subModels.Buffer (); i; i--, pso++) {
	j = pso->m_faces.m_nFaces;
	pm->m_nFaces += j;
	pm->m_nVerts += pso->m_nVerts;
	for (pf = pso->m_faces.m_list.Buffer (); j; j--, pf++)
		pm->m_nFaceVerts += pf->m_nVerts;
	}
return 1;
}

//------------------------------------------------------------------------------

int G3GetOOFModelItems (int nModel, OOF::CModel *po, RenderModel::CModel *pm, float fScale)
{
	OOF::CSubModel*		pso;
	OOF::CFace*			pof;
	OOF::CFaceVert*		pfv;
	RenderModel::CSubModel*	psm;
	CFloatVector3*				pvn = pm->m_vertNorms.Buffer (), vNormal;
	RenderModel::CVertex*	pmv = pm->m_faceVerts.Buffer ();
	RenderModel::CFace*		pmf = pm->m_faces.Buffer ();
	int							h, i, j, n, nIndex = 0;

for (i = po->m_nSubModels, pso = po->m_subModels.Buffer (), psm = pm->m_subModels.Buffer (); i; i--, pso++, psm++) {
	psm->m_nParent = pso->m_nParent;
	if (psm->m_nParent < 0)
		pm->m_iSubModel = (short) (psm - pm->m_subModels);
	psm->m_vOffset [X] = F2X (pso->m_vOffset [X] * fScale);
	psm->m_vOffset [Y] = F2X (pso->m_vOffset [Y] * fScale);
	psm->m_vOffset [Z] = F2X (pso->m_vOffset [Z] * fScale);
	psm->m_nAngles = 0;
	psm->m_nBomb = -1;
	psm->m_nMissile = -1;
	psm->m_nGun = -1;
	psm->m_nGunPoint = -1;
	psm->m_bBullets = 0;
	psm->m_bThruster = 0;
	psm->m_bGlow = 0;
	psm->m_bRender = 1;
	j = pso->m_faces.m_nFaces;
	psm->m_nIndex = nIndex;
	psm->m_nFaces = j;
	psm->m_faces = pmf;
	psm->InitMinMax ();
	for (pof = pso->m_faces.m_list.Buffer (); j; j--, pof++, pmf++) {
		pmf->m_nIndex = nIndex;
		pmf->m_bThruster = 0;
		pmf->m_bGlow = 0;
		n = pof->m_nVerts;
		pmf->m_nVerts = n;
		if (pof->m_bTextured)
			pmf->m_nBitmap = pof->m_texProps.nTexId;
		else
			pmf->m_nBitmap = -1;
		pfv = pof->m_verts;
		h = pfv->m_nIndex;
		if (nModel > 200) {
			vNormal = CFloatVector3::Normal (*pso->m_verts [pfv [0].m_nIndex].V3 (), 
														*pso->m_verts [pfv [1].m_nIndex].V3 (), 
														*pso->m_verts [pfv [2].m_nIndex].V3 ());
			}
		else
			vNormal.Assign (pof->m_vNormal);
		for (; n; n--, pfv++, pmv++, pvn++) {
			h = pfv->m_nIndex;
			pmv->m_nIndex = h;
			pmv->m_texCoord.v.u = pfv->m_fu;
			pmv->m_texCoord.v.v = pfv->m_fv;
			pmv->m_normal = vNormal;
			*reinterpret_cast<CFloatVector*> (pm->m_verts + h) = *reinterpret_cast<CFloatVector*> (pso->m_verts + h) * fScale;
			pmv->m_vertex = pm->m_verts [h];
			psm->SetMinMax (&pmv->m_vertex);
			*pvn = vNormal;
			if ((pmv->m_bTextured = pof->m_bTextured))
				pmv->m_baseColor.red =
				pmv->m_baseColor.green =
				pmv->m_baseColor.blue = 1.0f;
			else {
				pmv->m_baseColor.red = (float) pof->m_texProps.color.red / 255.0f;
				pmv->m_baseColor.green = (float) pof->m_texProps.color.green / 255.0f;
				pmv->m_baseColor.blue = (float) pof->m_texProps.color.blue / 255.0f;
				}
			pmv->m_baseColor.alpha = 1.0f;
			nIndex++;
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int G3BuildModelFromOOF (CObject *objP, int nModel)
{
	OOF::CModel	*po = gameData.models.modelToOOF [1][nModel];
	RenderModel::CModel		*pm;

if (!po) {
	po = gameData.models.modelToOOF [0][nModel];
	if (!po)
		return 0;
	}
#if DBG
HUDMessage (0, "optimizing model");
#endif
PrintLog ("         optimizing OOF model %d\n", nModel);
pm = gameData.models.renderModels [1] + nModel;
G3CountOOFModelItems (po, pm);
if (!G3AllocModel (pm))
	return 0;
G3GetOOFModelItems (nModel, po, pm, /*((nModel == 108) || (nModel == 110)) ? 0.805f :*/ 1.0f);
pm->m_textures = po->m_textures.m_bitmaps;
memset (pm->m_teamTextures, 0xFF, sizeof (pm->m_teamTextures));
pm->m_nType = -1;
gameData.models.polyModels [nModel].rad = pm->Size (objP, 1);
pm->Setup (1, 1);
#if 1
pm->SetGunPoints (objP, 0);
#endif
return -1;
}

//------------------------------------------------------------------------------
//eof
