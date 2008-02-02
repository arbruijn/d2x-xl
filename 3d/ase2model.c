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
#include "gr.h"
#include "u_mem.h"
#include "hiresmodels.h"
#include "buildmodel.h"

//------------------------------------------------------------------------------

void G3CountASEModelItems (tASEModel *pa, tG3Model *pm)
{
pm->nFaces = pa->nFaces;
pm->nSubModels = pa->nSubModels;
pm->nVerts = pa->nVerts;
pm->nFaceVerts = pa->nFaces * 3;
}

//------------------------------------------------------------------------------

void G3GetASEModelItems (int nModel, tASEModel *pa, tG3Model *pm, float fScale)
{
	tASESubModelList	*pml = pa->pSubModels;
	tASESubModel		*psa;
	tASEFace				*pfa;
	tG3SubModel			*psm;
	tG3ModelFace		*pmf = pm->pFaces;
	tG3ModelVertex		*pmv = pm->pFaceVerts;
	grsBitmap			*bmP;
	int					h, i, nFaces, iFace, nVerts = 0, nIndex = 0;
	int					bTextured;

for (pml = pa->pSubModels; pml; pml = pml->pNextModel) {
	psa = &pml->sm;
	psm = pm->pSubModels + psa->nId;
	psm->nParent = psa->nParent;
	psm->pFaces = pmf;
	psm->nFaces = nFaces = psa->nFaces;
	psm->bGlow = psa->bGlow;
	psm->bThruster = psa->bThruster;
	psm->nGunPoint = psa->nGunPoint;
	psm->nIndex = nIndex;
	VmVecFloatToFix (&psm->vOffset, (fVector *) &psa->vOffset);
	G3InitSubModelMinMax (psm);
	for (pfa = psa->pFaces, iFace = 0; iFace < nFaces; iFace++, pfa++, pmf++) {
		pmf->nIndex = nIndex;
#if 1
		i = psa->nBitmap;
#else
		i = pfa->nBitmap;
#endif
		bmP = pa->textures.pBitmaps + i;
		bTextured = !bmP->bmFlat;
		pmf->nBitmap = bTextured ? i : -1;
		pmf->nVerts = 3;
		pmf->nId = iFace;
		VmVecFloatToFix (&pmf->vNormal, (fVector *) &pfa->vNormal);
		for (i = 0; i < 3; i++, pmv++) {
			h = pfa->nVerts [i];
			if (pmv->bTextured = bTextured)
				pmv->baseColor.red =
				pmv->baseColor.green =
				pmv->baseColor.blue = 1;
			else {
				pmv->baseColor.red = (float) bmP->bmAvgRGB.red / 255.0f;
				pmv->baseColor.green = (float) bmP->bmAvgRGB.green / 255.0f;
				pmv->baseColor.blue = (float) bmP->bmAvgRGB.blue / 255.0f;
				}
			pmv->baseColor.alpha = 1;
			pmv->renderColor = pmv->baseColor;
			pmv->normal = psa->pVerts [h].normal;
			VmVecScalef ((fVector *) &pmv->vertex, (fVector *) &psa->pVerts [h].vertex, fScale);
			if (psa->pTexCoord)
				pmv->texCoord = psa->pTexCoord [pfa->nTexCoord [i]];
			h += nVerts;
			pm->pVerts [h] = pmv->vertex;
			pm->pVertNorms [h] = pmv->normal;
			pmv->nIndex = h;
			G3SetSubModelMinMax (psm, &pmv->vertex);
			nIndex++;
			}
		}
	nVerts += psa->nVerts;
	}
}

//------------------------------------------------------------------------------

int G3BuildModelFromASE (tObject *objP, int nModel)
{
	tASEModel	*pa = gameData.models.modelToASE [1][nModel];
	tG3Model		*pm;

if (!pa) {
	pa = gameData.models.modelToASE [0][nModel];
	if (!pa)
		return 0;
	}
#ifdef _DEBUG
HUDMessage (0, "optimizing model");
#endif
pm = gameData.models.g3Models [1] + nModel;
G3CountASEModelItems (pa, pm);
if (!G3AllocModel (pm))
	return 0;
G3GetASEModelItems (nModel, pa, pm, ((nModel == 108) || (nModel == 110)) ? 0.801f : 1.0f);
pm->pTextures = pa->textures.pBitmaps;
gameData.models.polyModels [nModel].rad = G3ModelSize (objP, pm, nModel, 1);
G3SetupModel (pm, 1, 0);
#if 1
G3SetGunPoints (objP, pm, nModel, 1);
#endif
return -1;
}

//------------------------------------------------------------------------------
//eof
