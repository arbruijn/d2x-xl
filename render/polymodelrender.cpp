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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "descent.h"
#include "interp.h"
#include "error.h"
#include "u_mem.h"
#include "args.h"
#include "gamepal.h"
#include "network.h"
#include "strutil.h"
#include "hiresmodels.h"
#include "texmap.h"
#include "textures.h"
#include "light.h"
#include "dynlight.h"
#include "buildmodel.h"
#include "hitbox.h"
#include "paging.h"
#include "ogl_lib.h"

//------------------------------------------------------------------------------

#define BASE_MODEL_SIZE		0x28000
#define DEFAULT_VIEW_DIST	0x60000

//------------------------------------------------------------------------------

int32_t ObjectHasShadow (CObject *objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (!gameOpts->render.shadows.bRobots)
		return 0;
	if (objP->cType.aiInfo.CLOAKED)
		return 0;
	}
else if (objP->info.nType == OBJ_WEAPON) {
	if (!gameOpts->render.shadows.bMissiles)
		return 0;
	if (!objP->IsMissile () && (objP->info.nId != SMALLMINE_ID))
		return 0;
	}
else if (objP->info.nType == OBJ_POWERUP) {
	if (!gameOpts->render.shadows.bPowerups)
		return 0;
	}
else if (objP->info.nType == OBJ_PLAYER) {
	if (!gameOpts->render.shadows.bPlayers)
		return 0;
	if (PLAYER (objP->info.nId).flags & PLAYER_FLAGS_CLOAKED)
		return 0;
	}
else if (objP->info.nType == OBJ_REACTOR) {
	if (!gameOpts->render.shadows.bReactors)
		return 0;
	}
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

CPolyModel* GetPolyModel (CObject *objP, CFixVector *pos, int32_t nModel, int32_t flags, int32_t* bCustomModel)
{
	CPolyModel	*modelP = NULL;
	int32_t		bHaveAltModel, bIsDefModel;

#if DBG
if (nModel == nDbgModel)
	BRP;
#endif
if (nModel < 0)
	return NULL;
bCustomModel = 0;
if (gameStates.app.bEndLevelSequence && 
	 ((nModel == gameData.endLevel.exit.nModel) || (nModel == gameData.endLevel.exit.nDestroyedModel))) {
	bHaveAltModel = 0;
	bIsDefModel = 1;
	}
else {
	bHaveAltModel = gameData.models.polyModels [2][nModel].Data () != NULL;
	bIsDefModel = IsDefaultModel (nModel);
	}
#if DBG
if (nModel == nDbgModel)
	BRP;
#endif
if ((nModel >= gameData.models.nPolyModels) && !(modelP = gameData.models.modelToPOL [nModel]))
	return NULL;
// only render shadows for custom models and for standard models with a shadow proof alternative model
if (!objP)
	modelP = ((gameStates.app.bAltModels && bIsDefModel && bHaveAltModel) ? gameData.models.polyModels [2] : gameData.models.polyModels [0]) + nModel;
else if (!modelP) {
	if (!(bIsDefModel && bHaveAltModel)) {
		if (gameStates.app.bFixModels && (objP->info.nType == OBJ_ROBOT) && (gameStates.render.nShadowPass == 2))
			return NULL;
		modelP = gameData.models.polyModels [0] + nModel;
		if (bCustomModel)
			*bCustomModel = 1;
		}
	else if (gameStates.render.nShadowPass != 2) {
		if ((gameStates.app.bAltModels || (objP->info.nType == OBJ_PLAYER)) && bHaveAltModel)
			modelP = gameData.models.polyModels [2] + nModel;
		else
			modelP = gameData.models.polyModels [0] + nModel;
		}
	else if (bHaveAltModel)
		modelP = gameData.models.polyModels [2] + nModel;
	else
		return NULL;
	if ((gameStates.render.nShadowPass == 2) && (objP->info.nType == OBJ_REACTOR) && !(nModel & 1))	// use the working reactor model for rendering destroyed reactors' shadows
		modelP--;
	}
//check if should use simple model (depending on detail level chosen)
if (!(SHOW_DYN_LIGHT || SHOW_SHADOWS) && modelP->SimplerModel () && !flags && pos) {
	int32_t	cnt = 1;
	fix depth = G3CalcPointDepth (*pos);		//gets 3d depth
	while (modelP->SimplerModel () && (depth > cnt++ * gameData.models.nSimpleModelThresholdScale * modelP->Rad ()))
		modelP = gameData.models.polyModels [0] + modelP->SimplerModel () - 1;
	}
return modelP;
}

//------------------------------------------------------------------------------

//draw a polygon model
int32_t DrawPolyModel (
	CObject*			objP,
	CFixVector*		pos,
	CFixMatrix*		orient,
	CAngleVector*	animAngles,
	int32_t			nModel,
	int32_t			flags,
	fix				light,
	fix*				glowValues,
	tBitmapIndex	altTextures [],
	CFloatVector*	colorP)
{
	CPolyModel*	modelP;
	int32_t			bHires = 0, bCustomModel = 0;

#if 0 //DBG
if (!gameStates.render.bBuildModels) {
	glColor3f (0,0,0);
	ogl.RenderScreenQuad (0);
	return 0;
	}
#endif

if (nModel < 0)
	return 1;
#if !MAX_SHADOWMAPS
if ((gameStates.render.nShadowPass == 2) && !ObjectHasShadow (objP))
	return 1;
if (!(modelP = GetPolyModel (objP, pos, nModel, flags, &bCustomModel))) {
	if (!(bCustomModel || flags) && (gameStates.render.nShadowPass != 2) && HaveHiresModel (nModel))
		bHires = 1;
	else
		return gameStates.render.nShadowPass == 2;
	}
if (gameStates.render.nShadowPass == 2) {
	if (!bHires) {
		G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
		G3DrawPolyModelShadow (objP, modelP->Data (), animAngles, nModel);
		}
	return 1;
	}
#else
if (!(modelP = GetPolyModel (objP, pos, nModel, flags, &bCustomModel))) {
	if (!flags && HaveHiresModel (nModel))
		bHires = 1;
	else
		return 0;
	}
#endif

#if 1//DBG
if (nModel == nDbgModel)
	BRP;
#endif

if (!bHires)
	modelP->LoadTextures (altTextures);
G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
gameData.render.vertP = gameData.models.fPolyModelVerts.Buffer ();
ogl.SetTransform (1);
if (!flags) {	//draw entire object
	if (!gameStates.app.bNostalgia && G3RenderModel (objP, bCustomModel ? -nModel : nModel, -1, modelP, gameData.models.textures, animAngles, NULL, light, glowValues, colorP)) {
		ogl.SetTransform (0);
		gameData.render.vertP = NULL;
		return 1;
		}
	if (bHires) {
		ogl.SetTransform (0);
		gameData.render.vertP = NULL;
		return 0;
		}
	if (objP && (objP->info.nType == OBJ_POWERUP)) {
		if ((objP->info.nId == POW_SMARTMINE) || (objP->info.nId == POW_PROXMINE))
			gameData.models.vScale.Set (I2X (2), I2X (2), I2X (2));
		else
			gameData.models.vScale.Set (I2X (3) / 2, I2X (3) / 2, I2X (3) / 2);
		}
	ogl.SetTransform ((gameStates.app.bEndLevelSequence < EL_OUTSIDE) && 
							!(SHOW_DYN_LIGHT && (gameOpts->ogl.bObjLighting || gameOpts->ogl.bLightObjects)));
	transformation.Begin (*pos, *orient);
	G3DrawPolyModel (objP, modelP->Data (), gameData.models.textures, animAngles, NULL, light, glowValues, colorP, NULL, nModel);
	transformation.End ();
	}
else {	
	CFixVector vOffset;

	for (int32_t i = 0; flags > 0; flags >>= 1, i++) {
		if ((flags & 1) && (i < modelP->ModelCount ())) {
			//if submodel, rotate around its center point, not pivot point
			vOffset = CFixVector::Avg (modelP->SubModels ().mins [i], modelP->SubModels ().maxs [i]);
			vOffset.Neg ();
			if (!G3RenderModel (objP, nModel, i, modelP, gameData.models.textures, animAngles, &vOffset, light, glowValues, colorP)) {
				if (bHires) {
					ogl.SetTransform (0);
					gameData.render.vertP = NULL;
					return 0;
					}
#if DBG
				G3RenderModel (objP, nModel, i, modelP, gameData.models.textures, animAngles, &vOffset, light, glowValues, colorP);
#endif
				transformation.Begin (vOffset);
				G3DrawPolyModel (objP, modelP->Data () + modelP->SubModels ().ptrs [i], gameData.models.textures,
									  animAngles, NULL, light, glowValues, colorP, NULL, nModel);
				transformation.End ();
				}
			}
		}
	}
ogl.SetTransform (0);
gameData.render.vertP = NULL;
return 1;
}

//------------------------------------------------------------------------------
//compare against this size when figuring how far to place eye for picture
//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.

void DrawModelPicture (int32_t nModel, CAngleVector *orientAngles)
{
	CFixVector	p = CFixVector::ZERO;
	CFixMatrix	o = CFixMatrix::IDENTITY;

Assert ((nModel >= 0) && (nModel < gameData.models.nPolyModels));
G3StartFrame (transformation, 0, 0, 0);
ogl.SetBlending (false);
SetupTransformation (transformation, p, o, gameStates.render.xZoom, 1);

if (gameData.models.polyModels [0][nModel].Rad ())
	p.v.coord.z = FixMulDiv (DEFAULT_VIEW_DIST, gameData.models.polyModels [0][nModel].Rad (), BASE_MODEL_SIZE);
else
	p.v.coord.z = DEFAULT_VIEW_DIST;
o = CFixMatrix::Create (*orientAngles);
DrawPolyModel (NULL, &p, &o, NULL, nModel, 0, I2X (1), NULL, NULL, NULL);
G3EndFrame (transformation, 0);
if (ogl.m_states.nDrawBuffer != GL_BACK)
	ogl.Update (0);
}

//------------------------------------------------------------------------------
//eof
