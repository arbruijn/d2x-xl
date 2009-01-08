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

#include "inferno.h"
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

#ifdef _3DFX
#include "3dfx_des.h"
#endif

CAngleVector animAngles [N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void SetRobotAngles (tRobotInfo* botInfoP, CPolyModel* modelP, CAngleVector angs [N_ANIM_STATES][MAX_SUBMODELS]);

//------------------------------------------------------------------------------

int ObjectHasShadow (CObject *objP)
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
	if (!gameData.objs.bIsMissile [objP->info.nId] && (objP->info.nId != SMALLMINE_ID))
		return 0;
	}
else if (objP->info.nType == OBJ_POWERUP) {
	if (!gameOpts->render.shadows.bPowerups)
		return 0;
	}
else if (objP->info.nType == OBJ_PLAYER) {
	if (!gameOpts->render.shadows.bPlayers)
		return 0;
	if (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED)
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

CPolyModel* GetPolyModel (CObject *objP, CFixVector *pos, int nModel, int flags)
{
	CPolyModel	*modelP = NULL;
	int			bHaveAltModel, bIsDefModel;

if (gameStates.app.bEndLevelSequence && 
	 ((nModel == gameData.endLevel.exit.nModel) || (nModel == gameData.endLevel.exit.nDestroyedModel))) {
	bHaveAltModel = 0;
	bIsDefModel = 1;
	}
else {
	bHaveAltModel = gameData.models.altPolyModels [nModel].Data ().Buffer () != NULL;
	bIsDefModel = IsDefaultModel (nModel);
	}
#if DBG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
if ((nModel >= gameData.models.nPolyModels) && !(modelP = gameData.models.modelToPOL [nModel]))
	return NULL;
// only render shadows for custom models and for standard models with a shadow proof alternative model
if (!objP)
	modelP = ((gameStates.app.bAltModels && bIsDefModel && bHaveAltModel) ? gameData.models.altPolyModels : gameData.models.polyModels) + nModel;
else if (!modelP) {
	if (!(bIsDefModel && bHaveAltModel)) {
		if (gameStates.app.bFixModels && (objP->info.nType == OBJ_ROBOT) && (gameStates.render.nShadowPass == 2))
			return NULL;
		modelP = gameData.models.polyModels + nModel;
		}
	else if (gameStates.render.nShadowPass != 2) {
		if ((gameStates.app.bAltModels || (objP->info.nType == OBJ_PLAYER)) && bHaveAltModel)
			modelP = gameData.models.altPolyModels + nModel;
		else
			modelP = gameData.models.polyModels + nModel;
		}
	else if (bHaveAltModel)
		modelP = gameData.models.altPolyModels + nModel;
	else
		return NULL;
	if ((gameStates.render.nShadowPass == 2) && (objP->info.nType == OBJ_REACTOR) && !(nModel & 1))	// use the working reactor model for rendering destroyed reactors' shadows
		modelP--;
	}
//check if should use simple model (depending on detail level chosen)
if (!(SHOW_DYN_LIGHT || SHOW_SHADOWS) && modelP->SimplerModel () && !flags && pos) {
	int	cnt = 1;
	fix depth = G3CalcPointDepth (*pos);		//gets 3d depth
	while (modelP->SimplerModel () && (depth > cnt++ * gameData.models.nSimpleModelThresholdScale * modelP->Rad ()))
		modelP = gameData.models.polyModels + modelP->SimplerModel () - 1;
	}
return modelP;
}

//------------------------------------------------------------------------------

//draw a polygon model
int DrawPolyModel (
	CObject*			objP,
	CFixVector*		pos,
	CFixMatrix*		orient,
	CAngleVector*	animAngles,
	int				nModel,
	int				flags,
	fix				light,
	fix*				glowValues,
	tBitmapIndex	altTextures [],
	tRgbaColorf*	colorP)
{
	CPolyModel*	modelP;
	int			nTextures, bHires = 0;

if ((gameStates.render.nShadowPass == 2) && !ObjectHasShadow (objP))
	return 1;
if (!(modelP = GetPolyModel (objP, pos, nModel, flags))) {
	if (!flags && (gameStates.render.nShadowPass != 2) && HaveHiresModel (nModel))
		bHires = 1;
	else
		return gameStates.render.nShadowPass == 2;
	}
if (gameStates.render.nShadowPass == 2) {
	if (!bHires) {
		G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
		G3DrawPolyModelShadow (objP, modelP->Data ().Buffer (), animAngles, nModel);
		}
	return 1;
	}
#if 1//def _DEBUG
if (nModel == nDbgModel)
	nDbgModel = nDbgModel;
#endif
nTextures = bHires ? 0 : LoadModelTextures (modelP, altTextures);
gameStates.ogl.bUseTransform = 1;
G3SetModelPoints (gameData.models.polyModelPoints.Buffer ());
gameData.render.vertP = gameData.models.fPolyModelVerts.Buffer ();
if (!flags) {	//draw entire CObject
	if (!G3RenderModel (objP, nModel, -1, modelP, gameData.models.textures.Buffer (), animAngles, NULL, light, glowValues, colorP)) {
		if (bHires)
			return 0;
#if 0//def _DEBUG
		if (objP && (objP->info.nType == OBJ_ROBOT))
			G3RenderModel (objP, nModel, -1, modelP, gameData.models.textures.Buffer (), animAngles, NULL, light, glowValues, colorP);
#endif
		if (objP && (objP->info.nType == OBJ_POWERUP)) {
			if ((objP->info.nId == POW_SMARTMINE) || (objP->info.nId == POW_PROXMINE))
				gameData.models.vScale.Set (I2X (2), I2X (2), I2X (2));
			else
				gameData.models.vScale.Set (I2X (3) / 2, I2X (3) / 2, I2X (3) / 2);
			}
		gameStates.ogl.bUseTransform = 
			(gameStates.app.bEndLevelSequence < EL_OUTSIDE) && 
			!(SHOW_DYN_LIGHT && ((RENDERPATH && gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects));
		transformation.Begin (*pos, *orient);
		G3DrawPolyModel (objP, modelP->Data ().Buffer (), gameData.models.textures.Buffer (), animAngles, NULL, light, glowValues, colorP, NULL, nModel);
		transformation.End ();
		}
	}
else {
	int i;

	//transformation.Begin (pos, orient);
	for (i = 0; flags > 0; flags >>= 1, i++)
		if (flags & 1) {
			CFixVector vOffset;

			//Assert (i < modelP->nModels);
			if (i < modelP->ModelCount ()) {
			//if submodel, rotate around its center point, not pivot point
				vOffset = CFixVector::Avg (modelP->SubModels ().mins [i], modelP->SubModels ().maxs [i]);
				vOffset.Neg();
				if (!G3RenderModel (objP, nModel, i, modelP, gameData.models.textures.Buffer (), animAngles, &vOffset, light, glowValues, colorP)) {
					if (bHires)
						return 0;
#if DBG
					G3RenderModel (objP, nModel, i, modelP, gameData.models.textures.Buffer (), animAngles, &vOffset, light, glowValues, colorP);
#endif
					transformation.Begin (vOffset);
					G3DrawPolyModel (objP, modelP->Data () + modelP->SubModels ().ptrs [i], gameData.models.textures.Buffer (),
										  animAngles, NULL, light, glowValues, colorP, NULL, nModel);
					transformation.End ();
					}
				}
			}
	//transformation.End ();
	}
gameStates.ogl.bUseTransform = 0;
gameData.render.vertP = NULL;
#if 0
{
	g3sPoint p0, p1;

transformation.Transform (&p0.p3_vec, &objP->info.position.vPos);
VmVecSub (&p1.p3_vec, &objP->info.position.vPos, &objP->mType.physInfo.velocity);
transformation.Transform (&p1.p3_vec, &p1.p3_vec);
glLineWidth (20);
glDisable (GL_TEXTURE_2D);
glBegin (GL_LINES);
glColor4d (1.0, 0.5, 0.0, 0.3);
OglVertex3x (p0.p3_vec [X], p0.p3_vec [Y], p0.p3_vec [Z]);
glColor4d (1.0, 0.5, 0.0, 0.1);
OglVertex3x (p1.p3_vec [X], p1.p3_vec [Y], p1.p3_vec [Z]);
glEnd ();
glLineWidth (1);
}
#endif
return 1;
}

//------------------------------------------------------------------------------
//compare against this size when figuring how far to place eye for picture
//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.

void DrawModelPicture (int nModel, CAngleVector *orientAngles)
{
	CFixVector	p = CFixVector::ZERO;
	CFixMatrix	o = CFixMatrix::IDENTITY;

Assert ((nModel >= 0) && (nModel < gameData.models.nPolyModels));
G3StartFrame (0, 0);
glDisable (GL_BLEND);
G3SetViewMatrix (p, o, gameStates.render.xZoom, 1);
if (gameData.models.polyModels [nModel].Rad ())
	p [Z] = FixMulDiv (DEFAULT_VIEW_DIST, gameData.models.polyModels [nModel].Rad (), BASE_MODEL_SIZE);
else
	p [Z] = DEFAULT_VIEW_DIST;
o = CFixMatrix::Create (*orientAngles);
DrawPolyModel (NULL, &p, &o, NULL, nModel, 0, I2X (1), NULL, NULL, NULL);
G3EndFrame ();
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	GrUpdate (0);
}

//------------------------------------------------------------------------------
//eof
