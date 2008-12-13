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

void G3CountASEModelItems (ASEModel::CModel *pa, CRenderModel *pm)
{
pm->m_nFaces = pa->nFaces;
pm->m_nSubModels = pa->nSubModels;
pm->m_nVerts = pa->nVerts;
pm->m_nFaceVerts = pa->nFaces * 3;
}

//------------------------------------------------------------------------------

void G3GetASEModelItems (int nModel, ASEModel::CModel *pa, CRenderModel *pm, float fScale)
{
	tASESubModelList		*pml = pa->subModels;
	ASEModel::CSubModel			*psa;
	ASEModel::CFace					*pfa;
	CRenderSubModel		*psm;
	CRenderModelFace		*pmf = pm->m_faces.Buffer ();
	RenderModel::CVertex	*pmv = pm->m_faceVerts.Buffer ();
	CBitmap					*bmP;
	int						h, i, nFaces, iFace, nVerts = 0, nIndex = 0;
	int						bTextured;

for (pml = pa->subModels; pml; pml = pml->pNextModel) {
	psa = &pml->sm;
	psm = pm->m_subModels + psa->nSubModel;
#if DBG
	strcpy (psm->m_szName, psa->szName);
#endif
	psm->m_m_nSubModel = ps->nSubModel;
	psm->m_nParent = psa->nParent;
	psm->m_faces = pmf;
	psm->m_nFaces = nFaces = psa->nFaces;
	psm->m_bGlow = psa->bGlow;
	psm->m_bRender = psa->bRender;
	psm->m_bThruster = psa->bThruster;
	psm->m_bWeapon = psa->bWeapon;
	psm->m_nGun = psa->nGun;
	psm->m_nBomb = psa->nBomb;
	psm->m_nMissile = psa->nMissile;
	psm->m_nType = psa->nType;
	psm->m_nWeaponPos = psa->nWeaponPos;
	psm->m_nGunPoint = psa->nGunPoint;
	psm->m_bBullets = (psa->nBullets > 0);
	psm->m_nIndex = nIndex;
	psm->m_iFrame = 0;
	psm->m_tFrame = 0;
	psm->m_nFrames = psa->bBarrel ? 32 : 0;
	psm->m_vOffset = psa->vOffset.ToFix();
	G3InitSubModelMinMax (psm);
	for (pfa = psa->faces.Buffer (), iFace = 0; iFace < nFaces; iFace++, pfa++, pmf++) {
		pmf->nIndex = nIndex;
#if 1
		i = psa->nBitmap;
#else
		i = pfa->nBitmap;
#endif
		bmP = pa->textures.m_bitmaps + i;
		bTextured = !bmP->Flat ();
		pmf->nBitmap = bTextured ? i : -1;
		pmf->nVerts = 3;
		pmf->nId = iFace;
		pmf->vNormal = pfa->vNormal.ToFix();
		for (i = 0; i < 3; i++, pmv++) {
			h = pfa->nVerts [i];
			if ((pmv->bTextured = bTextured))
				pmv->baseColor.red =
				pmv->baseColor.green =
				pmv->baseColor.blue = 1;
			else 
				bmP->GetAvgColor (&pmv->baseColor);
			pmv->baseColor.alpha = 1;
			pmv->renderColor = pmv->baseColor;
			pmv->normal = psa->verts [h].normal;
			pmv->vertex = psa->verts [h].vertex * fScale;
			if (psa->texCoord.Buffer ())
				pmv->texCoord = psa->texCoord [pfa->nTexCoord [i]];
			h += nVerts;
			pm->m_verts [h] = pmv->vertex;
			pm->m_vertNorms [h] = pmv->normal;
			pmv->nIndex = h;
			G3SetSubModelMinMax (psm, &pmv->vertex);
			nIndex++;
			}
		}
	nVerts += psa->nVerts;
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
if (!pm->m_Create ())
	return 0;
G3GetASEModelItems (nModel, pa, pm, 1.0f); //(nModel == 108) || (nModel == 110)) ? 1.145f : 1.0f);
pm->m_m_nModel = nModel;
pm->m_m_textures = pa->textures.m_bitmaps;
pm->m_m_nTextures = pa->textures.m_nBitmaps;
memset (pm->m_m_teamTextures, 0xFF, sizeof (pm->m_m_teamTextures));
for (i = 0; i < pm->m_m_nTextures; i++)
	if ((j = (int) pm->m_m_textures [i].Team ()))
		pm->m_m_teamTextures [j - 1] = i;
pm->m_m_nType = 2;
gameData.models.polyModels [nModel].rad = pm->m_Size (objP, 1);
pm->m_Setup (pm, 1, 1);
#if 1
pm->m_SetGunPoints (objP, pm, nModel, 1);
#endif
return -1;
}

//------------------------------------------------------------------------------
//eof
