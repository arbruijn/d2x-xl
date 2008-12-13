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
#include "gr.h"
#include "u_mem.h"
#include "hiresmodels.h"
#include "buildmodel.h"
#include "rendermodel.h"

//------------------------------------------------------------------------------

void G3CountASEModelItems (ASEModel::CModel *pa, RenderModel::CModel *pm)
{
pm->m_nFaces = pa->m_nFaces;
pm->m_nSubModels = pa->m_nSubModels;
pm->m_nVerts = pa->m_nVerts;
pm->m_nFaceVerts = pa->m_nFaces * 3;
}

//------------------------------------------------------------------------------

void G3GetASEModelItems (int nModel, ASEModel::CModel *pa, RenderModel::CModel *pm, float fScale)
{
	ASEModel::CSubModel*		psa;
	ASEModel::CFace*			pfa;
	RenderModel::CSubModel*	psm;
	RenderModel::CFace*		pmf = pm->m_faces.Buffer ();
	RenderModel::CVertex*	pmv = pm->m_faceVerts.Buffer ();
	CBitmap*						bmP;
	int							h, i, nFaces, iFace, nVerts = 0, nIndex = 0;
	int							bTextured;

for (psa = pa->m_subModels; psa; psa = psa->m_next) {
	psm = pm->m_subModels + psa->m_nSubModel;
#if DBG
	strcpy (psm->m_szName, psa->m_szName);
#endif
	psm->m_nSubModel = psa->m_nSubModel;
	psm->m_nParent = psa->m_nParent;
	psm->m_faces = pmf;
	psm->m_nFaces = nFaces = psa->m_nFaces;
	psm->m_bGlow = psa->m_bGlow;
	psm->m_bRender = psa->m_bRender;
	psm->m_bThruster = psa->m_bThruster;
	psm->m_bWeapon = psa->m_bWeapon;
	psm->m_nGun = psa->m_nGun;
	psm->m_nBomb = psa->m_nBomb;
	psm->m_nMissile = psa->m_nMissile;
	psm->m_nType = psa->m_nType;
	psm->m_nWeaponPos = psa->m_nWeaponPos;
	psm->m_nGunPoint = psa->m_nGunPoint;
	psm->m_bBullets = (psa->m_nBullets > 0);
	psm->m_nIndex = nIndex;
	psm->m_iFrame = 0;
	psm->m_tFrame = 0;
	psm->m_nFrames = psa->m_bBarrel ? 32 : 0;
	psm->m_vOffset = psa->m_vOffset.ToFix();
	psm->InitMinMax ();
	for (pfa = psa->m_faces.Buffer (), iFace = 0; iFace < nFaces; iFace++, pfa++, pmf++) {
		pmf->m_nIndex = nIndex;
#if 1
		i = psa->m_nBitmap;
#else
		i = pfa->m_nBitmap;
#endif
		bmP = pa->m_textures.m_bitmaps + i;
		bTextured = !bmP->Flat ();
		pmf->m_nBitmap = bTextured ? i : -1;
		pmf->m_nVerts = 3;
		pmf->m_nId = iFace;
		pmf->m_vNormal = pfa->m_vNormal.ToFix();
		for (i = 0; i < 3; i++, pmv++) {
			h = pfa->m_nVerts [i];
			if ((pmv->m_bTextured = bTextured))
				pmv->m_baseColor.red =
				pmv->m_baseColor.green =
				pmv->m_baseColor.blue = 1;
			else 
				bmP->GetAvgColor (&pmv->m_baseColor);
			pmv->m_baseColor.alpha = 1;
			pmv->m_renderColor = pmv->m_baseColor;
			pmv->m_normal = psa->m_verts [h].m_normal;
			pmv->m_vertex = psa->m_verts [h].m_vertex * fScale;
			if (psa->m_texCoord.Buffer ())
				pmv->m_texCoord = psa->m_texCoord [pfa->m_nTexCoord [i]];
			h += nVerts;
			pm->m_verts [h] = pmv->m_vertex;
			pm->m_vertNorms [h] = pmv->m_normal;
			pmv->m_nIndex = h;
			psm->SetMinMax (&pmv->m_vertex);
			nIndex++;
			}
		}
	nVerts += psa->m_nVerts;
	}
}

//------------------------------------------------------------------------------

int G3BuildModelFromASE (CObject *objP, int nModel)
{
	ASEModel::CModel*				pa = gameData.models.modelToASE [1][nModel];
	RenderModel::CModel*	pm;
	int						i, j;

if (!pa) {
	pa = gameData.models.modelToASE [0][nModel];
	if (!pa)
		return 0;
	}
#if DBG
HUDMessage (0, "optimizing model");
#endif
PrintLog ("         optimizing ASE model %d\n", nModel);
pm = gameData.models.renderModels [1] + nModel;
G3CountASEModelItems (pa, pm);
if (!pm->Create ())
	return 0;
G3GetASEModelItems (nModel, pa, pm, 1.0f); //(nModel == 108) || (nModel == 110)) ? 1.145f : 1.0f);
pm->m_nModel = nModel;
pm->m_textures = pa->m_textures.m_bitmaps;
pm->m_nTextures = pa->m_textures.m_nBitmaps;
memset (pm->m_teamTextures, 0xFF, sizeof (pm->m_teamTextures));
for (i = 0; i < pm->m_nTextures; i++)
	if ((j = (int) pm->m_textures [i].Team ()))
		pm->m_teamTextures [j - 1] = i;
pm->m_nType = 2;
gameData.models.polyModels [nModel].rad = pm->Size (objP, 1);
pm->Setup (1, 1);
#if 1
pm->SetGunPoints (objP, 1);
#endif
return -1;
}

//------------------------------------------------------------------------------
//eof
