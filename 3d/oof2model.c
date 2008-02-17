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
#include "hitbox.h"
#include "gr.h"
#include "hiresmodels.h"
#include "buildmodel.h"

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

int G3GetOOFModelItems (int nModel, tOOFObject *po, tG3Model *pm, float fScale)
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
	psm->vOffset.p.x = fl2f (pso->vOffset.x * fScale);
	psm->vOffset.p.y = fl2f (pso->vOffset.y * fScale);
	psm->vOffset.p.z = fl2f (pso->vOffset.z * fScale);
	psm->nAngles = 0;
	psm->nGunPoint = -1;
	psm->bThruster = 0;
	psm->bGlow = 0;
	j = pso->faces.nFaces;
	psm->nIndex = nIndex;
	psm->nFaces = j;
	psm->pFaces = pmf;
	G3InitSubModelMinMax (psm);
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
			VmVecScalef ((fVector *) (pm->pVerts + h), (fVector *) (pso->pvVerts + h), fScale);
			pmv->vertex = pm->pVerts [h];
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

int G3BuildModelFromOOF (tObject *objP, int nModel)
{
	tOOFObject	*po = gameData.models.modelToOOF [1][nModel];
	tG3Model		*pm;

if (!po) {
	po = gameData.models.modelToOOF [0][nModel];
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
G3GetOOFModelItems (nModel, po, pm, /*((nModel == 108) || (nModel == 110)) ? 0.805f :*/ 1.0f);
pm->pTextures = po->textures.pBitmaps;
memset (pm->teamTextures, 0xFF, sizeof (pm->teamTextures));
gameData.models.polyModels [nModel].rad = G3ModelSize (objP, pm, nModel, 1);
G3SetupModel (pm, 1, 1);
#if 1
G3SetGunPoints (objP, pm, nModel, 0);
#endif
return -1;
}

//------------------------------------------------------------------------------
//eof
