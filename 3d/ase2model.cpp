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
#include "gr.h"
#include "u_mem.h"
#include "hiresmodels.h"
#include "buildmodel.h"
#include "rendermodel.h"

using namespace RenderModel;

//------------------------------------------------------------------------------

void CModel::CountASEModelItems (ASE::CModel *pa)
{
m_nFaces = pa->m_nFaces;
m_nSubModels = pa->m_nSubModels;
m_nVerts = pa->m_nVerts;
m_nFaceVerts = pa->m_nFaces * 3;
}

//------------------------------------------------------------------------------

void CModel::GetASEModelItems (int nModel, ASE::CModel *pa, float fScale)
{
	ASE::CSubModel*	psa;
	ASE::CFace*			pfa;
	CSubModel*			psm;
	CFace*				pmf = m_faces.Buffer ();
	CVertex*				pmv = m_faceVerts.Buffer ();
	CBitmap*				bmP;
	int					h, i, nFaces, iFace, nVerts = 0, nIndex = 0;
	int					bTextured;

for (psa = pa->m_subModels; psa; psa = psa->m_next) {
	psm = m_subModels + psa->m_nSubModel;
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
	psm->m_vOffset.Assign (psa->m_vOffset);
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
		pmf->m_vNormal.Assign (pfa->m_vNormal);
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
			m_verts [h] = pmv->m_vertex;
			m_vertNorms [h] = pmv->m_normal;
			pmv->m_nIndex = h;
			psm->SetMinMax (&pmv->m_vertex);
			nIndex++;
			}
		}
	nVerts += psa->m_nVerts;
	}
}

//------------------------------------------------------------------------------

int CModel::BuildFromASE (CObject *objP, int nModel)
{
	ASE::CModel*	pa = gameData.models.modelToASE [1][nModel];
	CBitmap*			bmP;
	int				i, j;

#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
if (!pa) {
	pa = gameData.models.modelToASE [0][nModel];
	if (!pa)
		return 0;
	}
#if DBG
HUDMessage (0, "optimizing model");
#endif
PrintLog ("         optimizing ASE model %d\n", nModel);
CountASEModelItems (pa);
if (!Create ())
	return 0;
GetASEModelItems (nModel, pa, 1.0f); //(nModel == 108) || (nModel == 110)) ? 1.145f : 1.0f);
m_nModel = nModel;
m_textures = pa->m_textures.m_bitmaps;
m_nTextures = pa->m_textures.m_nBitmaps;
for (i = 0, bmP = m_textures.Buffer (); i < m_nTextures; i++, bmP++) {
	pa->m_textures.m_bitmaps [i].ShareBuffer (*bmP);
	bmP->SetupTexture ();
	}
memset (m_teamTextures, 0xFF, sizeof (m_teamTextures));
for (i = 0; i < m_nTextures; i++)
	if ((j = (int) m_textures [i].Team ()))
		m_teamTextures [j - 1] = i;
m_nType = 2;
gameData.models.polyModels [0][nModel].SetRad (Size (objP, 1));
Setup (1, 1);
#if 1
SetGunPoints (objP, 1);
#endif
return -1;
}

//------------------------------------------------------------------------------
//eof
