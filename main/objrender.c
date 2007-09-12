/* $Id: tObject.c, v 1.9 2003/10/04 03:14:47 btb Exp $ */
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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "inferno.h"
#include "perlin.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"

#include "object.h"
#include "objsmoke.h"
#include "objrender.h"
#include "lightning.h"
#include "physics.h"
#include "slew.h"		
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "pa_enabl.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "fuelcen.h"
#include "endlevel.h"
#include "timer.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "interp.h"
#include "newdemo.h"
#include "player.h"
#include "weapon.h"
#include "network.h"
#include "newmenu.h"
#include "gauges.h"
#include "multi.h"
#include "menu.h"
#include "args.h"
#include "text.h"
#include "piggy.h"
#include "switch.h"
#include "gameseq.h"
#include "vecmat.h"
#include "particles.h"
#include "hudmsg.h"
#include "oof.h"
#include "sphere.h"
#include "globvars.h"
#ifdef TACTILE
#include "tactile.h"
#endif
#include "ogl_init.h"
#include "input.h"
#include "automap.h"
#include "u_mem.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

#define RENDER_LIGHTNING_CORONAS 0
#define RENDER_TARGET_LIGHTNING 0

#define fabsf(_f)	(float) fabs (_f)

static int nGlobalOrientation = 0;

#define IS_TRACK_GOAL(_objP)	(((_objP) == gameData.objs.trackGoals [0]) || ((_objP) == gameData.objs.trackGoals [1]))

//------------------------------------------------------------------------------

void SetRenderView (fix nEyeOffset, short *pnStartSeg);

extern vmsAngVec avZero;

//------------------------------------------------------------------------------

short PowerupModel (int nId)
{
	short nModel;

if ((nModel = PowerupToModel (nId)))
	return nModel;
if (0 > (nId = PowerupToObject (nId)))
	return 0;
return gameData.weapons.info [nId].nModel;
}

//------------------------------------------------------------------------------

short WeaponModel (tObject *objP)
{
	short	nModel;
	
if ((nModel = WeaponToModel (objP->id)))
	return nModel;
return objP->rType.polyObjInfo.nModel;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToPowerup (tObject *objP)
{
objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.pVClips [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->size = gameData.objs.pwrUp.info [objP->id].size;
objP->controlType = CT_POWERUP;
objP->renderType = RT_POWERUP;
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

int ConvertPowerupToWeapon (tObject *objP)
{
	vmsAngVec	a;
	short			nModel, nId;
	int			bHasModel = 0;

if (!SHOW_OBJ_FX)
	return 0;
if (!gameOpts->render.powerups.b3D)
	return 0;
if (objP->controlType == CT_WEAPON)
	return 1;

nModel = PowerupToModel (objP->id);
if (nModel) 
	nId = objP->id;
else {
	nId = PowerupToObject (objP->id);
	if (nId >= 0) {
		nModel = gameData.weapons.info [nId].nModel;
		bHasModel = 1;
		}
	}
if (!bHasModel && ((objP->nType != OBJ_WEAPON) || !gameData.objs.bIsMissile [objP->id]) &&
	 !(nModel && (gameData.models.modelToOOF [nModel] || gameData.models.modelToPOL [nModel])))
		return 0;

if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	a.p = (rand () % F1_0) - F1_0 / 2;
	a.b = (rand () % F1_0) - F1_0 / 2;
	a.h = (rand () % F1_0) - F1_0 / 2;
	VmAngles2Matrix (&objP->position.mOrient, &a);
	}
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
#if 0
if ((objP->nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->id]) 
#endif
	{
	objP->mType.physInfo.rotVel.p.x = 0;
	objP->mType.physInfo.rotVel.p.y = 
	objP->mType.physInfo.rotVel.p.z = gameOpts->render.powerups.nSpin ? F1_0 / (5 - gameOpts->render.powerups.nSpin) : 0;
	}
#if 0
else {
	objP->mType.physInfo.rotVel.p.x = 
	objP->mType.physInfo.rotVel.p.z = 0;
	objP->mType.physInfo.rotVel.p.y = gameOpts->render.powerups.nSpin ? F1_0 / (5 - gameOpts->render.powerups.nSpin) : 0;
	}
#endif
objP->controlType = CT_WEAPON;
objP->renderType = RT_POLYOBJ;
objP->movementType = MT_PHYSICS;
objP->mType.physInfo.flags = PF_BOUNCE | PF_FREE_SPINNING;
objP->rType.polyObjInfo.nModel = nModel;
#if 0
if (bHasModel)
	objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad, 
								gameData.weapons.info [objP->id].po_len_to_width_ratio);
#endif
objP->rType.polyObjInfo.nTexOverride = -1;
objP->lifeleft = IMMORTAL_TIME;
return 1;
}

// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the tPlayer had fired, "warns" the robot
void SetRobotLocationInfo (tObject *objP)
{
if (gameStates.app.bPlayerFiredLaserThisFrame != -1) {
	g3sPoint temp;

	G3TransformAndEncodePoint (&temp, &objP->position.vPos);
	if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
		return;
	//the code below to check for tObject near the center of the screen
	//completely ignores z, which may not be good
	if ((abs (temp.p3_x) < F1_0*4) && (abs (temp.p3_y) < F1_0*4)) {
		objP->cType.aiInfo.nDangerLaser = gameStates.app.bPlayerFiredLaserThisFrame;
		objP->cType.aiInfo.nDangerLaserSig = gameData.objs.objects [gameStates.app.bPlayerFiredLaserThisFrame].nSignature;
		}
	}
}

//------------------------------------------------------------------------------

fix CalcObjectLight (tObject *objP, fix *xEngineGlow)
{
	fix xLight;

if (IsMultiGame && netGame.BrightPlayers && (objP->nType == OBJ_PLAYER)) {
	xLight = F1_0; //	If option set for bright players in netgame, brighten them
	gameOpts->ogl.bLightObjects = 0;
	}
else
	xLight = ComputeObjectLight (objP, NULL);
//make robots brighter according to robot glow field
if (objP->nType == OBJ_ROBOT)
	xLight += (ROBOTINFO (objP->id).glow << 12);		//convert 4:4 to 16:16
else if (objP->nType == OBJ_WEAPON) {
	if (objP->id == FLARE_ID)
		xLight += F1_0 * 2;
	}
else if (objP->nType == OBJ_MARKER)
 	xLight += F1_0 * 2;
ComputeEngineGlow (objP, xEngineGlow);
return xLight;
}

//------------------------------------------------------------------------------
//draw an tObject that has one bitmap & doesn't rotate

void DrawObjectBlob (tObject *objP, tBitmapIndex bmi0, tBitmapIndex bmi, int iFrame, tRgbaColorf *color, float alpha)
{
	grsBitmap	*bmP;
	int			id;
	int			nTransp = (objP->nType == OBJ_POWERUP) ? 3 : 2;
	int			bDepthInfo = 1; // (objP->nType != OBJ_FIREBALL);
	fix			xSize;

if (gameOpts->render.bTransparentEffects) {
	if (!alpha) {
		if (objP->nType == OBJ_POWERUP) {
			id = objP->id;
			if ((id == POW_EXTRA_LIFE) ||
				 (id == POW_ENERGY) ||
				 (id == POW_SHIELD_BOOST) ||
				 (id == POW_HOARD_ORB) ||
				 (id == POW_CLOAK) ||
				 (id == POW_INVUL))
				alpha = 2.0f / 3.0f;
			else
				alpha = 1.0f;
			}
		else if ((objP->nType != OBJ_FIREBALL) && (objP->nType != OBJ_EXPLOSION))
			alpha = 1.0f;
		else {
			alpha = 2.0f / 3.0f;
			}
		}
	}
else {
	nTransp = 3;
	alpha = 1.0f;
	}
PIGGY_PAGE_IN (bmi, 0);
bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
if ((bmP->bmType == BM_TYPE_STD) && BM_OVERRIDE (bmP)) {
	OglLoadBmTexture (bmP, 1, nTransp = -1);
	bmP = BM_OVERRIDE (bmP);
	if (BM_FRAMES (bmP))
		bmP = BM_FRAMES (bmP) + iFrame;
	alpha = 1;
	}
else if (color && gameOpts->render.bDepthSort)
	OglLoadBmTexture (bmP, 1, nTransp);

if (color)
	memcpy (color, gameData.pig.tex.bitmapColors + bmi.index, sizeof (*color));

xSize = objP->size;

if (gameOpts->render.bDepthSort) {
	tRgbaColorf	color = {1, 1, 1, alpha};
	if (bmP->bm_props.w > bmP->bm_props.h)
		RIAddSprite (bmP, &objP->position.vPos, &color, xSize, FixMulDiv (xSize, bmP->bm_props.h, bmP->bm_props.w), 0);
	else
		RIAddSprite (bmP, &objP->position.vPos, &color, FixMulDiv (xSize, bmP->bm_props.w, bmP->bm_props.h), xSize, 0);
	}
else {
	if (bmP->bm_props.w > bmP->bm_props.h)
		G3DrawBitmap (&objP->position.vPos, xSize, FixMulDiv (xSize, bmP->bm_props.h, bmP->bm_props.w), bmP, 
						  NULL, alpha, nTransp, bDepthInfo);
	else	
		G3DrawBitmap (&objP->position.vPos, FixMulDiv (xSize, bmP->bm_props.w, bmP->bm_props.h), xSize, bmP, 
						  NULL, alpha, nTransp, bDepthInfo);
	}
}

//------------------------------------------------------------------------------
//draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly (tObject *objP, tBitmapIndex bmi, int lighted)
{
	grsBitmap *bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
	fix light;
	vmsVector delta, top_v, bot_v;
	g3sPoint top_p, bot_p;

PIGGY_PAGE_IN (bmi, 0);
bmP = BmOverride (bmP);
//bmP->bm_handle = bmi.index;
VmVecCopyScale (&delta, &objP->position.mOrient.uVec, objP->size);
VmVecAdd (&top_v, &objP->position.vPos, &delta);
VmVecSub (&bot_v, &objP->position.vPos, &delta);
G3TransformAndEncodePoint (&top_p, &top_v);
G3TransformAndEncodePoint (&bot_p, &bot_v);
if (lighted)
	light = ComputeObjectLight (objP, &top_p.p3_vec);
else
	light = f1_0;
#ifdef _3DFX
_3dfx_rendering_poly_obj = 1;
#endif
#ifdef PA_3DFX_VOODOO
light = f1_0;
#endif
G3DrawRodTexPoly (bmP, &bot_p, objP->size, &top_p, objP->size, light, NULL);
#ifdef _3DFX
_3dfx_rendering_poly_obj = 0;
#endif
}

//------------------------------------------------------------------------------

int	bLinearTMapPolyObjs = 1;

extern fix MaxThrust;

//used for robot engine glow
//function that takes the same parms as draw_tmap, but renders as flat poly
//we need this to do the cloaked effect

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

//------------------------------------------------------------------------------
//do special cloaked render
void DrawCloakedObject (tObject *objP, fix light, fix *glow, fix xCloakStartTime, fix xCloakEndTime)
{
	fix	xCloakDeltaTime, xTotalCloakedTime;
	fix	xLightScale = F1_0;
	int	nCloakValue = 0;
	int	bFading = 0;		//if true, bFading, else cloaking
	fix	xCloakFadeinDuration = F1_0;
	fix	xCloakFadeoutDuration = F1_0;

if (xCloakStartTime != 0x7fffffff)
	xTotalCloakedTime = xCloakEndTime - xCloakStartTime;
else 
	xTotalCloakedTime = gameData.time.xGame;
switch (objP->nType) {
	case OBJ_PLAYER:
		xCloakFadeinDuration = CLOAK_FADEIN_DURATION_PLAYER;
		xCloakFadeoutDuration = CLOAK_FADEOUT_DURATION_PLAYER;
		break;
	case OBJ_ROBOT:
		xCloakFadeinDuration = CLOAK_FADEIN_DURATION_ROBOT;
		xCloakFadeoutDuration = CLOAK_FADEOUT_DURATION_ROBOT;
		break;
	default:
		Int3 ();		//	Contact Mike: Unexpected tObject nType in DrawCloakedObject.
	}

xCloakDeltaTime = gameData.time.xGame - ((xCloakStartTime == 0x7fffffff) ? 0 : xCloakStartTime);
if (xCloakDeltaTime < xCloakFadeinDuration / 2) {
	xLightScale = FixDiv (xCloakFadeinDuration / 2 - xCloakDeltaTime, xCloakFadeinDuration / 2);
	bFading = 1;
	}
else if (xCloakDeltaTime < xCloakFadeinDuration) {
	nCloakValue = f2i (FixDiv (xCloakDeltaTime - xCloakFadeinDuration / 2, xCloakFadeinDuration / 2) * CLOAKED_FADE_LEVEL);
	} 
else if ((xCloakStartTime == 0x7fffffff) || (gameData.time.xGame < xCloakEndTime - xCloakFadeoutDuration)) {
	static int nCloakDelta = 0, nCloakDir = 1;
	static fix xCloakTimer = 0;

	//note, if more than one cloaked tObject is visible at once, the
	//pulse rate will change!
	xCloakTimer -= gameData.time.xFrame;
	while (xCloakTimer < 0) {
		xCloakTimer += xCloakFadeoutDuration / 12;
		nCloakDelta += nCloakDir;
		if (nCloakDelta == 0 || nCloakDelta == 4)
			nCloakDir = -nCloakDir;
		}
	nCloakValue = CLOAKED_FADE_LEVEL - nCloakDelta;
	} 
else if (gameData.time.xGame < xCloakEndTime - xCloakFadeoutDuration / 2) {
	nCloakValue = f2i (FixDiv (xTotalCloakedTime - xCloakFadeoutDuration / 2 - xCloakDeltaTime, xCloakFadeoutDuration / 2) * CLOAKED_FADE_LEVEL);
	} 
else {
	xLightScale = FixDiv (xCloakFadeoutDuration / 2 - (xTotalCloakedTime - xCloakDeltaTime), xCloakFadeoutDuration / 2);
	bFading = 1;
	}
if (bFading) {
	fix xNewLight, xSaveGlow;
	tBitmapIndex * nAltTextures = NULL;

	if (objP->rType.polyObjInfo.nAltTextures > 0)
		nAltTextures = MultiPlayerTextures [objP->rType.polyObjInfo.nAltTextures-1];
	xNewLight = FixMul (light, xLightScale);
	xSaveGlow = glow [0];
	glow [0] = FixMul (glow [0], xLightScale);
	DrawPolygonModel (objP, &objP->position.vPos, 
							&objP->position.mOrient, 
							(vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
							objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags, 
							xNewLight, 
							glow, 
							nAltTextures, 
							NULL);
		glow [0] = xSaveGlow;
	}
else {
	gameStates.render.grAlpha = (float) nCloakValue;
	GrSetColorRGB (0, 0, 0, 255);	//set to black (matters for s3)
	G3SetSpecialRender (DrawTexPolyFlat, NULL, NULL);		//use special flat drawer
	DrawPolygonModel (objP, &objP->position.vPos, 
						   &objP->position.mOrient, 
							(vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
							objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags, 
							light, 
							glow, 
							NULL, 
							NULL);
	G3SetSpecialRender (NULL, NULL, NULL);
	gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	}
}

//------------------------------------------------------------------------------

int DrawHiresObject (tObject *objP, fix xLight, fix *xEngineGlow)
{
	float			fLight [3];
	int			bCloaked;
	short			nModel = 0;
	tOOFObject	*po;

if (gameStates.render.bLoResShadows && (gameStates.render.nShadowPass == 2))
	return 0;
if (objP->nType == OBJ_DEBRIS)
	return 0;
else if ((objP->nType == OBJ_POWERUP) || (objP->nType == OBJ_WEAPON)) {
	if (objP->nType == OBJ_POWERUP)
		nModel = PowerupModel (objP->id);
	else if (objP->nType == OBJ_WEAPON)
		nModel = WeaponModel (objP);
	if (!nModel)
		return 0;
	}
else
	nModel = objP->rType.polyObjInfo.nModel;
if (!(po = gameData.models.modelToOOF [nModel]))
	return 0;
fLight [0] = xLight / 65536.0f;
fLight [1] = (float) xEngineGlow [0] / 65536.0f;				
fLight [2] = (float) xEngineGlow [1] / 65536.0f;				
if (objP->nType == OBJ_PLAYER)
	bCloaked = (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED) != 0;
else if (objP->nType == OBJ_ROBOT)
	bCloaked = objP->cType.aiInfo.CLOAKED;
else
	bCloaked = 0;
OOF_Render (objP, po, fLight, bCloaked);
return 1;
}

//------------------------------------------------------------------------------
//draw an tObject which renders as a polygon model
void DrawPolygonObject (tObject *objP)
{
	fix xLight;
	int imSave;
	fix xEngineGlow [2];		//element 0 is for engine glow, 1 for headlight
	int bBlendPolys = 0;
	int bBrightPolys = 0;
	int bEnergyWeapon = (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id] && !gameData.objs.bIsMissile [objP->id];
	//tRgbColorf color;

#if SHADOWS
if (FAST_SHADOWS && 
	 !gameOpts->render.shadows.bSoft && 
	 (gameStates.render.nShadowPass == 3))
	return;
#endif
xLight = CalcObjectLight (objP, xEngineGlow);
if (DrawHiresObject (objP, xLight, xEngineGlow))
	return;
gameOpts->render.bDepthSort = -gameOpts->render.bDepthSort;
imSave = gameStates.render.nInterpolationMethod;
if (bLinearTMapPolyObjs)
	gameStates.render.nInterpolationMethod = 1;
//set engine glow value
if (objP->rType.polyObjInfo.nTexOverride != -1) {
#ifdef _DEBUG
	tPolyModel *pm = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
#endif
	tBitmapIndex	bm = gameData.pig.tex.bmIndex [0][objP->rType.polyObjInfo.nTexOverride], 
						bmiP [12];
	int				i;

#ifdef _DEBUG
	Assert (pm->nTextures <= 12);
#endif
	for (i = 0; i < 12; i++)		//fill whole array, in case simple model needs more
		bmiP [i] = bm;
	DrawPolygonModel (objP, &objP->position.vPos, 
							&objP->position.mOrient, 
							(vmsAngVec *) &objP->rType.polyObjInfo.animAngles, 
							objP->rType.polyObjInfo.nModel, 
							objP->rType.polyObjInfo.nSubObjFlags, 
							xLight, 
							xEngineGlow, 
							bmiP, 
							NULL);
}
else {
	if ((objP->nType == OBJ_PLAYER) && (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED))
		DrawCloakedObject (objP, xLight, xEngineGlow, gameData.multiplayer.players [objP->id].cloakTime, 
								 gameData.multiplayer.players [objP->id].cloakTime + CLOAK_TIME_MAX);
	else if ((objP->nType == OBJ_ROBOT) && (objP->cType.aiInfo.CLOAKED)) {
		if (ROBOTINFO (objP->id).bossFlag) {
			int i = FindBoss (OBJ_IDX (objP));
			if (i >= 0)
				DrawCloakedObject (objP, xLight, xEngineGlow, gameData.boss [i].nCloakStartTime, gameData.boss [i].nCloakEndTime);
			}
		else
			DrawCloakedObject (objP, xLight, xEngineGlow, gameData.time.xGame - F1_0 * 10, gameData.time.xGame + F1_0 * 10);
		}
	else {
		tBitmapIndex *bmiAltTex = NULL;
		if (objP->rType.polyObjInfo.nAltTextures > 0)
			bmiAltTex = MultiPlayerTextures [objP->rType.polyObjInfo.nAltTextures-1];

		//	Snipers get bright when they fire.
		if (gameData.ai.localInfo [OBJ_IDX (objP)].nextPrimaryFire < F1_0/8) {
			if (objP->cType.aiInfo.behavior == AIB_SNIPE)
				xLight = 2 * xLight + F1_0;
		}
		bBlendPolys = bEnergyWeapon && (gameData.weapons.info [objP->id].nInnerModel > -1);
		bBrightPolys = bBlendPolys && WI_energy_usage (objP->id);
		if (bBlendPolys) {
			fix xDistToEye = VmVecDistQuick (&gameData.objs.viewer->position.vPos, &objP->position.vPos);
			if (!gameOpts->legacy.bRender) {
				gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS - 2.0f;
				OglBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				}
			if (xDistToEye < gameData.models.nSimpleModelThresholdScale * F1_0*2)
				DrawPolygonModel (
					objP, &objP->position.vPos, &objP->position.mOrient, 
					(vmsAngVec *) &objP->rType.polyObjInfo.animAngles, 
					gameData.weapons.info [objP->id].nInnerModel, 
					objP->rType.polyObjInfo.nSubObjFlags, 
					bBrightPolys ? F1_0 : xLight, 
					xEngineGlow, 
					bmiAltTex, 
					NULL /*gameData.weapons.color + objP->id*/);
			}
		if (bEnergyWeapon)
			gameStates.render.grAlpha = 4 * (float) GR_ACTUAL_FADE_LEVELS / 5;
		else if (!bBlendPolys)
			gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
		DrawPolygonModel (
			objP, &objP->position.vPos, &objP->position.mOrient, 
			(vmsAngVec *) &objP->rType.polyObjInfo.animAngles, 
			objP->rType.polyObjInfo.nModel, 
			objP->rType.polyObjInfo.nSubObjFlags, 
			bBrightPolys ? F1_0 : xLight, 
			xEngineGlow, 
			bmiAltTex, 
			bEnergyWeapon ? gameData.weapons.color + objP->id : NULL);
		if (bBlendPolys && !gameOpts->legacy.bRender) {
			gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
			OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
		}
	}
gameStates.render.nInterpolationMethod = imSave;
gameOpts->render.bDepthSort = -gameOpts->render.bDepthSort;
}

//------------------------------------------------------------------------------

void TransformHitboxf (tObject *objP, fVector *vertList, int iSubObj)
{

	fVector		hv;
	tHitbox		*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes + iSubObj;
	vmsVector	vMin = phb->vMin;
	vmsVector	vMax = phb->vMax;
	int			i;

for (i = 0; i < 8; i++) {
	hv.p.x = f2fl (hitBoxOffsets [i].p.x ? vMin.p.x : vMax.p.x);
	hv.p.y = f2fl (hitBoxOffsets [i].p.y ? vMin.p.y : vMax.p.y);
	hv.p.z = f2fl (hitBoxOffsets [i].p.z ? vMin.p.z : vMax.p.z);
	G3TransformPointf (vertList + i, &hv, 0);
	}
}

//------------------------------------------------------------------------------

#ifdef _DEBUG

void RenderHitbox (tObject *objP, float red, float green, float blue, float alpha)
{
	fVector		vertList [8], v;
	tHitbox		*pmhb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	int			i, j, iModel, nModels, bHit = 0;

if (!SHOW_OBJ_FX)
	return;
if (!EGI_FLAG (bRenderShield, 0, 1, 0) ||
	 ((objP->nType == OBJ_PLAYER) && (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED)))
	return;
if (!EGI_FLAG (nHitboxes, 0, 0, 0)) {
	DrawShieldSphere (objP, red, green, blue, alpha);
	return;
	}
else if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].nSubModels;
	}
glDepthFunc (GL_LEQUAL);
glEnable (GL_BLEND);
OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDisable (GL_TEXTURE_2D);
glDepthMask (0);
glColor4f (red, green, blue, alpha / 2);
G3StartInstanceMatrix (&objP->position.vPos, &objP->position.mOrient);
for (; iModel <= nModels; iModel++) {
	G3StartInstanceAngles (&pmhb [iModel].vOffset, &avZero);
	TransformHitboxf (objP, vertList, iModel);
	glBegin (GL_QUADS);
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 4; j++)
			glVertex3fv ((GLfloat *) (vertList + hitboxFaceVerts [i][j]));
		}
	glEnd ();
	glLineWidth (2);
	for (i = 0; i < 6; i++) {
		glBegin (GL_LINES);
		v.p.x = v.p.y = v.p.z = 0;
		for (j = 0; j < 4; j++) {
			glVertex3fv ((GLfloat *) (vertList + hitboxFaceVerts [i][j]));
			VmVecIncf (&v, vertList + hitboxFaceVerts [i][j]);
			}
		glEnd ();
		}
	glLineWidth (1);
	G3DoneInstance ();
	}
G3DoneInstance ();
#if 0//def _DEBUG	//display collision point
if (gameStates.app.nSDLTicks - gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].tHit < 500) {
	tObject	o;

	o.position.vPos = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].vHit;
	o.position.mOrient = objP->position.mOrient;
	o.size = F1_0 * 2;
	//SetRenderView (0, NULL);
	DrawShieldSphere (&o, 1, 0, 0, 0.33f);
	}
#endif
glDepthMask (1);
glDepthFunc (GL_LESS);
}

#endif

// -----------------------------------------------------------------------------
//	Render an tObject.  Calls one of several routines based on nType
// -----------------------------------------------------------------------------

void RenderPlayerShield (tObject *objP)
{
	int	bStencil, i = objP->id;

if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS &&
	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 1) : (gameStates.render.nShadowPass != 3)))
	return;
#endif
if (EGI_FLAG (bRenderShield, 0, 1, 0) &&
	 !(gameData.multiplayer.players [i].flags & PLAYER_FLAGS_CLOAKED)) {
	if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
		glDisable (GL_STENCIL_TEST);
	UseSpherePulse (&gameData.render.shield, gameData.multiplayer.spherePulse + i);
	if (gameData.multiplayer.players [i].flags & PLAYER_FLAGS_INVULNERABLE)
#if 0//def _DEBUG
		RenderHitbox (objP, 1.0f, 0.8f, 0.6f, 0.6f);
#else
		DrawShieldSphere (objP, 1.0f, 0.8f, 0.6f, 1.0f);
#endif
	else if (gameData.multiplayer.bWasHit [i]) {
		if (gameData.multiplayer.bWasHit [i] < 0) {
			gameData.multiplayer.bWasHit [i] = 1;
			gameData.multiplayer.nLastHitTime [i] = gameStates.app.nSDLTicks;
			SetSpherePulse (gameData.multiplayer.spherePulse + i, 0.1f, 0.5f);
			}
		else if (gameStates.app.nSDLTicks - gameData.multiplayer.nLastHitTime [i] >= 300) {
			gameData.multiplayer.bWasHit [i] = 0;
			SetSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.4f);
			}
		}
	if (gameData.multiplayer.bWasHit [i])
#if 0//def _DEBUG
		RenderHitbox (objP, 1.0f, 0.5f, 0.0f, 1.0f);
#else
		DrawShieldSphere (objP, 1.0f, 0.5f, 0.0f, 1.0f);
#endif
	else {
		if (gameData.multiplayer.spherePulse [i].fSpeed == 0.0f)
			SetSpherePulse (gameData.multiplayer.spherePulse + i, 0.02f, 0.5f);
#if 0//def _DEBUG
		RenderHitbox (objP, 0.0f, 0.5f, 1.0f, (float) f2fl (gameData.multiplayer.players [i].shields) / 400.0f);
#else
		DrawShieldSphere (objP, 0.0f, 0.5f, 1.0f, (float) f2fl (gameData.multiplayer.players [i].shields) / 100.0f);
#endif
		}
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

static inline tRgbColorf *ObjectFrameColor (tObject *objP, tRgbColorf *pc)
{
	static tRgbColorf	defaultColor = {0, 1.0f, 0};
	static tRgbColorf	botDefColor = {1.0f, 0, 0};
	static tRgbColorf	reactorDefColor = {0.5f, 0, 0.5f};
	static tRgbColorf	playerDefColors [] = {{0,1.0f,0},{0,0,1.0f},{1.0f,0,0}};

if (pc)
	return pc;
if (objP) {
	if (objP->nType == OBJ_CNTRLCEN)
		return &reactorDefColor;
	else if (objP->nType == OBJ_ROBOT) {
		if (!ROBOTINFO (objP->id).companion)
			return &botDefColor;
		}
	else if (objP->nType == OBJ_PLAYER) {
		if (IsTeamGame)
			return playerDefColors + GetTeam (objP->id) + 1;
		return playerDefColors;
		}
	}
return &defaultColor;
}

// -----------------------------------------------------------------------------

void RenderDamageIndicator (tObject *objP, tRgbColorf *pc)
{
	fVector		fPos, fVerts [4];
	float			r, r2, w;
	int			i, bStencil, bDrawArrays;

if (!SHOW_OBJ_FX)
	return;
if ((gameData.demo.nState == ND_STATE_PLAYBACK) && gameOpts->demo.bOldFormat)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (EGI_FLAG (bDamageIndicators, 0, 1, 0) &&
	 (extraGameInfo [IsMultiGame].bTargetIndicators < 2)) {
	if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
		glDisable (GL_STENCIL_TEST);
	pc = ObjectFrameColor (objP, pc);
	VmsVecToFloat (&fPos, &objP->position.vPos);
	G3TransformPointf (&fPos, &fPos, 0);
	r = f2fl (objP->size);
	r2 = r / 10;
	r = r2 * 9;
	w = 2 * r;
	fPos.p.x -= r;
	fPos.p.y += r;
	w *= ObjectDamage (objP);
	fVerts [0].p.x = fVerts [3].p.x = fPos.p.x;
	fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + w;
	fVerts [0].p.y = fVerts [1].p.y = fPos.p.y;
	fVerts [2].p.y = fVerts [3].p.y = fPos.p.y - r2;
	fVerts [0].p.z = fVerts [1].p.z = fVerts [2].p.z = fVerts [3].p.z = fPos.p.z;
	fVerts [0].p.w = fVerts [1].p.w = fVerts [2].p.w = fVerts [3].p.w = 1;
	glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
	glDisable (GL_TEXTURE_2D);
#if 1
	if (bDrawArrays = OglEnableClientState (GL_VERTEX_ARRAY)) {
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_QUADS, 0, 4);
		}
	else {
		glBegin (GL_QUADS);
		for (i = 0; i < 4; i++)
			glVertex3fv ((GLfloat *) (fVerts + i));
		glEnd ();
		}
#else
	bDrawArrays = 0;
	glBegin (GL_QUADS);
	glVertex3f (fPos.p.x, fPos.p.y, fPos.p.z);
	glVertex3f (fPos.p.x + w, fPos.p.y, fPos.p.z);
	glVertex3f (fPos.p.x + w, fPos.p.y - r2, fPos.p.z);
	glVertex3f (fPos.p.x, fPos.p.y - r2, fPos.p.z);
	glEnd ();
#endif
	w = 2 * r;
	fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + w;
	glColor3fv ((GLfloat *) pc);
	if (bDrawArrays) {
		glVertexPointer (4, GL_FLOAT, 0, fVerts);
		glDrawArrays (GL_LINE_LOOP, 0, 4);
		glDisableClientState (GL_VERTEX_ARRAY);
		}
	else {
		glBegin (GL_LINE_LOOP);
		for (i = 0; i < 4; i++)
			glVertex3fv ((GLfloat *) (fVerts + i));
		glEnd ();
		}
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

static tRgbColorf	trackGoalColor = {1, 0.5f, 0};
static int	nMslLockColor = 0;
static int	nMslLockColorIncr = -1;

void RenderMslLockIndicator (tObject *objP)
{
	#define INDICATOR_POSITIONS	60

	static tSinCosd	sinCosInd [INDICATOR_POSITIONS];
	static int			bInitSinCos = 1;
	static int			nMslLockIndPos = 0;
	static time_t		t0 = 0;

	fVector				fPos, fVerts [3];
	float					r, r2;
	int					nTgtInd, bHasDmg;

if (!EGI_FLAG (bMslLockIndicators, 0, 1, 0))
	return;
if (!IS_TRACK_GOAL (objP))
	return;
if (gameStates.app.nSDLTicks - t0 > 25) {
	t0 = gameStates.app.nSDLTicks;
	if (!nMslLockColor || (nMslLockColor == 15))
		nMslLockColorIncr = -nMslLockColorIncr;
	nMslLockColor += nMslLockColorIncr;
	trackGoalColor.green = 0.65f + (float) nMslLockColor / 100.0f;
	nMslLockIndPos = (nMslLockIndPos + 1) % INDICATOR_POSITIONS;
	}
VmsVecToFloat (&fPos, &objP->position.vPos);
G3TransformPointf (&fPos, &fPos, 0);
r = f2fl (objP->size);
r2 = r / 4;

glDisable (GL_CULL_FACE);
glDisable (GL_TEXTURE_2D);
OglEnableClientState (GL_VERTEX_ARRAY);
glColor4f (trackGoalColor.red, trackGoalColor.green, trackGoalColor.blue, 0.8f);
if (gameOpts->render.cockpit.bRotateMslLockInd) {
	fVector	rotVerts [3];
	fMatrix	m;
	int		i, j;

	if (bInitSinCos) {
		OglComputeSinCos (sizeofa (sinCosInd), sinCosInd);
		bInitSinCos = 0;
		}
	m.rVec.p.x =
	m.uVec.p.y = (float) sinCosInd [nMslLockIndPos].dCos;
	m.uVec.p.x = (float) sinCosInd [nMslLockIndPos].dSin;
	m.rVec.p.y = -m.uVec.p.x;
	m.rVec.p.z =
	m.uVec.p.z =
	m.fVec.p.x = 
	m.fVec.p.y = 0;
	m.fVec.p.z = 1;

	fVerts [0].p.z =
	fVerts [1].p.z =
	fVerts [2].p.z = 0;
	rotVerts [0].p.w = 
	rotVerts [1].p.w = 
	rotVerts [2].p.w = 1;
	fVerts [0].p.x = -r2;
	fVerts [1].p.x = +r2;
	fVerts [2].p.x = 0;
	fVerts [0].p.y = 
	fVerts [1].p.y = +r;
	fVerts [2].p.y = +r - r2;
	glVertexPointer (4, GL_FLOAT, 0, rotVerts);
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++) {
			VmVecRotatef (rotVerts + i, fVerts + i, &m);
			fVerts [i] = rotVerts [i];
			VmVecIncf (rotVerts + i, &fPos);
			}	
		glDrawArrays (GL_TRIANGLES, 0, 3);
		if (!j) {	//now rotate by 90 degrees
			m.rVec.p.x =
			m.uVec.p.y = 0;
			m.uVec.p.x = 1;
			m.rVec.p.y = -1;
			}
		}
	}
else {
	fVerts [0].p.z =
	fVerts [1].p.z =
	fVerts [2].p.z = fPos.p.z;
	fVerts [0].p.w = 
	fVerts [1].p.w = 
	fVerts [2].p.w = 1;
	fVerts [0].p.x = fPos.p.x - r2;
	fVerts [1].p.x = fPos.p.x + r2;
	fVerts [2].p.x = fPos.p.x;
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	nTgtInd = extraGameInfo [IsMultiGame].bTargetIndicators;
	bHasDmg = !EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) | (ObjectDamage (objP) < 1);
	if (!nTgtInd ||
		 ((nTgtInd == 1) && (!EGI_FLAG (bDamageIndicators, 0, 1, 0) || !bHasDmg)) ||
		 ((nTgtInd == 2) && !bHasDmg)) {
		fVerts [0].p.y = 
		fVerts [1].p.y = fPos.p.y + r;
		fVerts [2].p.y = fPos.p.y + r - r2;
		glDrawArrays (GL_TRIANGLES, 0, 3);
		}
	fVerts [0].p.y = 
	fVerts [1].p.y = fPos.p.y - r;
	fVerts [2].p.y = fPos.p.y - r + r2;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0].p.x = 
	fVerts [1].p.x = fPos.p.x + r;
	fVerts [2].p.x = fPos.p.x + r - r2;
	fVerts [0].p.y = fPos.p.y + r2;
	fVerts [1].p.y = fPos.p.y - r2;
	fVerts [2].p.y = fPos.p.y;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	fVerts [0].p.x = 
	fVerts [1].p.x = fPos.p.x - r;
	fVerts [2].p.x = fPos.p.x - r + r2;
	glDrawArrays (GL_TRIANGLES, 0, 3);
	}
glDisableClientState (GL_VERTEX_ARRAY);
glEnable (GL_CULL_FACE);
}

// -----------------------------------------------------------------------------

void RenderTargetIndicator (tObject *objP, tRgbColorf *pc)
{
	fVector		fPos, fVerts [4];
	float			r, r2, r3;
	int			i, bStencil, bDrawArrays, nPlayer = (objP->nType == OBJ_PLAYER) ? objP->id : -1;

if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
#if 0
if (!CanSeeObject (OBJ_IDX (objP), 1))
	return;
#endif
if (!EGI_FLAG (bCloakedIndicators, 0, 1, 0)) {
	if (nPlayer >= 0) {
		if (gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED)
			return;
		}
	else if (objP->nType == OBJ_ROBOT) {
		if (objP->cType.aiInfo.CLOAKED)
			return;
		}
	}
if (IsTeamGame && EGI_FLAG (bFriendlyIndicators, 0, 1, 0)) {
	if (GetTeam (nPlayer) != GetTeam (gameData.multiplayer.nLocalPlayer)) {
		if (!(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_FLAG))
			return;
		pc = ObjectFrameColor (NULL, NULL);
		}
	}
RenderMslLockIndicator (objP);
if (EGI_FLAG (bTagOnlyHitObjs, 0, 1, 0) && (ObjectDamage (objP) >= 1.0f))
	return;
if (EGI_FLAG (bTargetIndicators, 0, 1, 0)) {
	if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
		glDisable (GL_STENCIL_TEST);
	glDisable (GL_TEXTURE_2D);
	pc = (EGI_FLAG (bMslLockIndicators, 0, 1, 0) && IS_TRACK_GOAL (objP) && 
			!gameOpts->render.cockpit.bRotateMslLockInd && (extraGameInfo [IsMultiGame].bTargetIndicators != 1)) ? 
		  &trackGoalColor : ObjectFrameColor (objP, pc);
	VmsVecToFloat (&fPos, &objP->position.vPos);
	G3TransformPointf (&fPos, &fPos, 0);
	r = f2fl (objP->size);
	glColor3fv ((GLfloat *) pc);
	fVerts [0].p.w = fVerts [1].p.w = fVerts [2].p.w = fVerts [3].p.w = 1;
	glVertexPointer (4, GL_FLOAT, 0, fVerts);
	if (extraGameInfo [IsMultiGame].bTargetIndicators == 1) {	//square brackets
		r2 = r * 2 / 3;
		fVerts [0].p.x = fVerts [3].p.x = fPos.p.x - r2;
		fVerts [1].p.x = fVerts [2].p.x = fPos.p.x - r;
		fVerts [0].p.y = fVerts [1].p.y = fPos.p.y - r;
		fVerts [2].p.y = fVerts [3].p.y = fPos.p.y + r;
		fVerts [0].p.z =
		fVerts [1].p.z =
		fVerts [2].p.z =
		fVerts [3].p.z = fPos.p.z;
		if (bDrawArrays = OglEnableClientState (GL_VERTEX_ARRAY))
			glDrawArrays (GL_LINE_STRIP, 0, 4);
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv ((GLfloat *) (fVerts + i));
			glEnd ();
			}
		fVerts [0].p.x = fVerts [3].p.x = fPos.p.x + r2;
		fVerts [1].p.x = fVerts [2].p.x = fPos.p.x + r;
		if (bDrawArrays) {
			glDrawArrays (GL_LINE_STRIP, 0, 4);
			glDisableClientState (GL_VERTEX_ARRAY);
			}
		else {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i < 4; i++)
				glVertex3fv ((GLfloat *) (fVerts + i));
			glEnd ();
			}
		}
	else {	//triangle
		r2 = r / 3;
		fVerts [0].p.x = fPos.p.x - r2;
		fVerts [1].p.x = fPos.p.x + r2;
		fVerts [2].p.x = fPos.p.x;
		fVerts [0].p.y = fVerts [1].p.y = fPos.p.y + r;
		fVerts [2].p.y = fPos.p.y + r - r2;
		fVerts [0].p.z =
		fVerts [1].p.z =
		fVerts [2].p.z = fPos.p.z;
		if (bDrawArrays = OglEnableClientState (GL_VERTEX_ARRAY))
			glDrawArrays (GL_LINE_LOOP, 0, 3);
		else {
			glBegin (GL_LINE_LOOP);
			glVertex3fv ((GLfloat *) fVerts);
			glVertex3fv ((GLfloat *) (fVerts + 1));
			glVertex3fv ((GLfloat *) (fVerts + 2));
			glEnd ();
			}
		if (EGI_FLAG (bDamageIndicators, 0, 1, 0)) {
			r3 = ObjectDamage (objP);
			if (r3 < 1.0f) {
				if (r3 < 0.0f)
					r3 = 0.0f;
				fVerts [0].p.x = fPos.p.x - r2 * r3;
				fVerts [1].p.x = fPos.p.x + r2 * r3;
				fVerts [2].p.x = fPos.p.x;
				fVerts [0].p.y = fVerts [1].p.y = fPos.p.y + r - r2 * (1.0f - r3);
				//fVerts [2].p.y = fPos.p.y + r - r2;
				}
			}
		glColor4f (pc->red, pc->green, pc->blue, 2.0f / 3.0f);
		if (bDrawArrays) {
			glDrawArrays (GL_TRIANGLES, 0, 3);
			glDisableClientState (GL_VERTEX_ARRAY);
			}
		else {
			glBegin (GL_TRIANGLES);
			for (i = 0; i < 3; i++)
			glVertex3fv ((GLfloat *) (fVerts + i));
			glEnd ();
			}
		}
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
	}
RenderDamageIndicator (objP, pc);
}

// -----------------------------------------------------------------------------

void RenderTowedFlag (tObject *objP)
{
	static fVector fVerts [4] = {
		{{0.0f, 2.0f / 3.0f, 0.0f, 1.0f}},
		{{0.0f, 2.0f / 3.0f, -1.0f, 1.0f}},
		{{0.0f, -(1.0f / 3.0f), -1.0f, 1.0f}},
		{{0.0f, -(1.0f / 3.0f), 0.0f, 1.0f}}
	};

	typedef struct uv {
		float	u, v;
	} uv;

	static uv uvList [4] = {{0.0f, -0.3f}, {1.0f, -0.3f}, {1.0f, 0.7f}, {0.0f, 0.7f}};

if (gameStates.app.bNostalgia)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (IsTeamGame && (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_FLAG)) {
		vmsVector		vPos = objP->position.vPos;
		fVector			vPosf;
		tFlagData		*pf = gameData.pig.flags + !GetTeam (objP->id);
		tPathPoint		*pp = GetPathPoint (&pf->path);
		int				i, bStencil;
		float				r;
		grsBitmap		*bmP;

	if (pp) {
		if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
			glDisable (GL_STENCIL_TEST);
		OglActiveTexture (GL_TEXTURE0_ARB, 0);
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		PIGGY_PAGE_IN (pf->bmi, 0);
		bmP = gameData.pig.tex.pBitmaps + pf->vcP->frames [pf->vci.nCurFrame].index;
		if (OglBindBmTex (bmP, 1, 2))
			return;
		bmP = BmCurFrame (bmP);
		OglTexWrap (bmP->glTexture, GL_REPEAT);
		VmVecScaleInc (&vPos, &objP->position.mOrient.fVec, -objP->size);
		r = f2fl (objP->size);
		G3StartInstanceMatrix (&vPos, &pp->mOrient);
		glBegin (GL_QUADS);
		glColor3f (1.0f, 1.0f, 1.0f);
		for (i = 0; i < 4; i++) {
			vPosf.p.x = 0;
			vPosf.p.y = fVerts [i].p.y * r;
			vPosf.p.z = fVerts [i].p.z * r;
			G3TransformPointf (&vPosf, &vPosf, 0);
			glTexCoord2fv ((GLfloat *) (uvList + i));
			glVertex3fv ((GLfloat *) &vPosf);
			}
		for (i = 3; i >= 0; i--) {
			vPosf.p.x = 0;
			vPosf.p.y = fVerts [i].p.y * r;
			vPosf.p.z = fVerts [i].p.z * r;
			G3TransformPointf (&vPosf, &vPosf, 0);
			glTexCoord2fv ((GLfloat *) (uvList + i));
			glVertex3fv ((GLfloat *) &vPosf);
			}
		glEnd ();
		G3DoneInstance ();
		OGL_BINDTEX (0);
		if (bStencil)
			glEnable (GL_STENCIL_TEST);
		}
	}
}

// -----------------------------------------------------------------------------

#define	RING_SIZE		16
#define	THRUSTER_SEGS	14

static fVector	vFlame [THRUSTER_SEGS][RING_SIZE];
static int			bHaveFlame = 0;

static fVector	vRing [RING_SIZE] = {
	{{-0.5f, -0.5f, 0.0f, 1.0f}},
	{{-0.6533f, -0.2706f, 0.0f, 1.0f}},
	{{-0.7071f, 0.0f, 0.0f, 1.0f}},
	{{-0.6533f, 0.2706f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f, 1.0f}},
	{{-0.2706f, 0.6533f, 0.0f, 1.0f}},
	{{0.0f, 0.7071f, 0.0f, 1.0f}},
	{{0.2706f, 0.6533f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.0f, 1.0f}},
	{{0.6533f, 0.2706f, 0.0f, 1.0f}},
	{{0.7071f, 0.0f, 0.0f, 1.0f}},
	{{0.6533f, -0.2706f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, 0.0f, 1.0f}},
	{{0.2706f, -0.6533f, 0.0f, 1.0f}},
	{{0.0f, -0.7071f, 0.0f, 1.0f}},
	{{-0.2706f, -0.6533f, 0.0f, 1.0f}}
};

static int		nStripIdx [] = {0,15,1,14,2,13,3,12,4,11,5,10,6,9,7,8};

void CreateThrusterFlame (void)
{
if (!bHaveFlame) {
		fVector		*pv;
		int			i, j, m, n;
		double		phi, sinPhi;
		float			z = 0, 
						fScale = 2.0f / 3.0f, 
						fStep [2] = {1.0f / 4.0f, 1.0f / 3.0f};

	pv = &vFlame [0][0];
	for (i = 0, phi = 0; i < 5; i++, phi += Pi / 8, z -= fStep [0]) {
		sinPhi = (1 + sin (phi) / 2) * fScale;
		for (j = 0; j < RING_SIZE; j++, pv++) {
			pv->p.x = vRing [j].p.x * (float) sinPhi;
			pv->p.y = vRing [j].p.y * (float) sinPhi;
			pv->p.z = z;
			}
		}
	m = n = THRUSTER_SEGS - i + 1;
	for (phi = Pi / 2; i < THRUSTER_SEGS; i++, phi += Pi / 8, z -= fStep [1], m--) {
		sinPhi = (1 + sin (phi) / 2) * fScale * m / n;
		for (j = 0; j < RING_SIZE; j++, pv++) {
			pv->p.x = vRing [j].p.x * (float) sinPhi;
			pv->p.y = vRing [j].p.y * (float) sinPhi;
			pv->p.z = z;
			}
		}
	bHaveFlame = 1;
	}
}

// -----------------------------------------------------------------------------

void CalcShipThrusterPos (tObject *objP, vmsVector *vPos)
{
	tPosition	*pPos = OBJPOS (objP);

if (gameOpts->render.bHiresModels) {
	VmVecScaleAdd (vPos, &pPos->vPos, &pPos->mOrient.fVec, -objP->size);
	VmVecScaleInc (vPos, &pPos->mOrient.rVec, -(8 * objP->size / 44));
	VmVecScaleAdd (vPos + 1, vPos, &pPos->mOrient.rVec, 8 * objP->size / 22);
	}
else {
	VmVecScaleAdd (vPos, &pPos->vPos, &pPos->mOrient.fVec, -objP->size / 10 * 9);
	if (gameStates.app.bFixModels)
		VmVecScaleInc (vPos, &pPos->mOrient.uVec, objP->size / 40);
	else
		VmVecScaleInc (vPos, &pPos->mOrient.uVec, -objP->size / 20);
	vPos [1] = vPos [0];
	VmVecScaleInc (vPos, &pPos->mOrient.rVec, -8 * objP->size / 49);
	VmVecScaleInc (vPos + 1, &pPos->mOrient.rVec, 8 * objP->size / 49);
	}
}

// -----------------------------------------------------------------------------

void RenderThrusterFlames (tObject *objP)
{
	int					h, i, j, k, l, nStyle, nThrusters, bStencil, bSpectate, bTextured;
	tRgbaColorf			c [2];
	vmsVector			vPos [2], vDir [2];
	fVector				v;
	float					fSize, fLength, fSpeed, fPulse, fFade [4];
	tThrusterData		*pt = NULL;
	tPathPoint			*pp = NULL;
	tModelThrusters	*mtP = NULL;
	
	static time_t		tPulse = 0;
	static int			nPulse = 10;

if (gameStates.app.bNostalgia)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
#if 1//ndef _DEBUG
if (!EGI_FLAG (bThrusterFlames, 1, 1, 0))
	return;
#endif
if ((objP->nType == OBJ_PLAYER) && (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED))
	return;
fSpeed = f2fl (VmVecMag (&objP->mType.physInfo.velocity));
fLength = fSpeed / 60.0f + 0.5f + (float) (rand () % 100) / 1000.0f;
if (!pt || (fSpeed >= pt->fSpeed)) {
	fFade [0] = 0.95f;
	fFade [1] = 0.85f;
	fFade [2] = 0.75f;
	fFade [3] = 0.65f;
	}
else {
	fFade [0] = 0.9f;
	fFade [1] = 0.8f;
	fFade [2] = 0.7f;
	fFade [3] = 0.6f;
	}
if (pt)
	pt->fSpeed = fSpeed;
bSpectate = SPECTATOR (objP);
#if 1
if (gameStates.app.nSDLTicks - tPulse > 10) {
	tPulse = gameStates.app.nSDLTicks;
	nPulse = d_rand () % 11;
	}
fPulse = (float) nPulse / 10.0f;
if (gameOpts->render.bHiresModels && (objP->nType == OBJ_PLAYER)) {
	if (!bSpectate) {
		pt = gameData.render.thrusters + objP->id;
		pp = GetPathPoint (&pt->path);
		}
	fSize = (fLength + 1) / 2;
	nThrusters = 2;
	CalcShipThrusterPos (objP, vPos);
	}
else if (gameOpts->render.bHiresModels && (objP->nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->id]) {
	tHitbox	*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	fix		nObjRad = 2 * (phb->vMax.p.z - phb->vMin.p.z) / 3;
	if (objP->id == EARTHSHAKER_ID)
		fSize = 1.0f;
	else if ((objP->id == MEGAMSL_ID) || (objP->id == EARTHSHAKER_MEGA_ID))
		fSize = 0.8f;
	else if (objP->id == SMARTMSL_ID)
		fSize = 0.6f;
	else
		fSize = 0.5f;
	nThrusters = 1;
	if (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2)
		fLength /= 2;
	VmVecScaleAdd (vPos, &objP->position.vPos, &objP->position.mOrient.fVec, -nObjRad);
	}
else if ((objP->nType == OBJ_PLAYER) || 
			((objP->nType == OBJ_ROBOT) && !objP->cType.aiInfo.CLOAKED) || 
			((objP->nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->id])) {
	vmsMatrix	m;
	if (!bSpectate && (objP->nType == OBJ_PLAYER)) {
		pt = gameData.render.thrusters + objP->id;
		pp = GetPathPoint (&pt->path);
		}
	mtP = gameData.models.thrusters + objP->rType.polyObjInfo.nModel;
	nThrusters = mtP->nCount;
	VmCopyTransposeMatrix (&m, &objP->position.mOrient);
	for (i = 0; i < nThrusters; i++) {
		VmVecRotate (vPos + i, mtP->vPos + i, &m);
		VmVecInc (vPos + i, &objP->position.vPos);
		VmVecRotate (vDir + i, mtP->vDir + i, &m);
		}
	fSize = mtP->fSize;
	if ((objP->nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->id] && (nThrusters > 1))
		nThrusters = 1;
	}
else
	return;
#else
if (objP->nType == OBJ_ROBOT) {
	vmsMatrix	m;
	mtP = gameData.models.thrusters + objP->rType.polyObjInfo.nModel;
	nThrusters = mtP->nCount;
	VmCopyTransposeMatrix (&m, &objP->position.mOrient);
	for (i = 0; i < nThrusters; i++) {
		VmVecRotate (vPos + i, mtP->vPos + i, &m);
		VmVecInc (vPos + i, &objP->position.vPos);
		VmVecRotate (vDir + i, mtP->vDir + i, &m);
		}
	fSize = mtP->fSize;
	//mtP->nCount = 0;
	}
else if (objP->nType == OBJ_PLAYER) {
		pt = gameData.render.thrusters + objP->id;
		pp = GetPathPoint (&pt->path);

	fSize = (fLength + 1) / 2;
	nThrusters = 2;
	CalcShipThrusterPos (objP, vPos);
	}
else if (objP->nType == OBJ_WEAPON) {
	if (!gameData.objs.bIsMissile [objP->id])
		return;
	if (objP->id == EARTHSHAKER_ID)
		fSize = 1.0f;
	else if ((objP->id == MEGAMSL_ID) || (objP->id == EARTHSHAKER_MEGA_ID))
		fSize = 0.8f;
	else if (objP->id == SMARTMSL_ID)
		fSize = 0.6f;
	else
		fSize = 0.5f;
	nThrusters = 1;
	VmVecScaleAdd (vPos, &objP->position.vPos, &objP->position.mOrient.fVec, -objP->size);
	}
#endif
if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
	glDisable (GL_STENCIL_TEST);
//glDepthMask (0);
bTextured = 0;
nStyle = EGI_FLAG (bThrusterFlames, 1, 1, 0) == 2;
if (!LoadThruster ()) {
	extraGameInfo [IsMultiGame].bThrusterFlames = 2;
	glDisable (GL_TEXTURE_2D);
	}
else if (gameOpts->render.bDepthSort <= 0) {
	glEnable (GL_TEXTURE_2D);
	if (OglBindBmTex (bmpThruster [nStyle], 1, -1)) {
		extraGameInfo [IsMultiGame].bThrusterFlames = 2;
		glDisable (GL_TEXTURE_2D);
		}
	else {
		OglTexWrap (bmpThruster [nStyle]->glTexture, GL_CLAMP);
		bTextured = 1;
		}
	}
if (nThrusters > 1) {
	vmsVector vRot [2];
	for (i = 0; i < 2; i++)
		G3RotatePoint (vRot + i, vPos + i, 0);
	if (vRot [0].p.z < vRot [1].p.z) {
		vmsVector v = vPos [0];
		vPos [0] = vPos [1];
		vPos [1] = v;
		if (objP->nType == OBJ_ROBOT) {
			v = vDir [0];
			vDir [0] = vDir [1];
			vDir [1] = v;
			}
		}
	}
glEnable (GL_BLEND);
if (EGI_FLAG (bThrusterFlames, 1, 1, 0) == 1) {
		static tUVLf	uvlThruster [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};
		static tUVLf	uvlFlame [3] = {{{0,0,1}},{{1,1,1}},{{1,0,1}}};
		static fVector	vEye = {{0, 0, 0}};

		fVector	vPosf, vNormf, vFlame [3], vThruster [4], fVecf;
		float		c = 1/*0.7f + 0.03f * fPulse*/, dotFlame, dotThruster;

	glDisable (GL_CULL_FACE);
	glColor3f (c, c, c);
	fLength *= 4 * fSize;
	fSize *= 1.5f;
#if 1
	if (!mtP)
		VmsVecToFloat (&fVecf, pp ? &pp->mOrient.fVec : &objP->position.mOrient.fVec);
#endif
	for (h = 0; h < nThrusters; h++) {
		if (mtP)
			VmsVecToFloat (&fVecf, vDir + h);
		VmsVecToFloat (&vPosf, vPos + h);
		VmVecScaleAddf (vFlame + 2, &vPosf, &fVecf, -fLength);
		G3TransformPointf (vFlame + 2, vFlame + 2, 0);
		G3TransformPointf (&vPosf, &vPosf, 0);
		VmVecNormalf (&vNormf, vFlame + 2, &vPosf, &vEye);
		VmVecScaleAddf (vFlame, &vPosf, &vNormf, fSize);
		VmVecScaleAddf (vFlame + 1, &vPosf, &vNormf, -fSize);
		VmVecNormalf (&vNormf, vFlame, vFlame + 1, vFlame + 2);
		vThruster [0] = vFlame [0];
		vThruster [2] = vFlame [1];
		VmVecScaleAddf (vThruster + 1, &vPosf, &vNormf, fSize);
		VmVecScaleAddf (vThruster + 3, &vPosf, &vNormf, -fSize);
		VmVecNormalizef (&vPosf, &vPosf);
		VmVecNormalizef (&v, vFlame + 2);
		dotFlame = VmVecDotf (&vPosf, &v);
		VmVecNormalizef (&v, vThruster);
		dotThruster = VmVecDotf (&vPosf, &v);
		if (dotFlame < dotThruster) {
			if (gameOpts->render.bDepthSort > 0)
				RIAddPoly (bmpThruster [nStyle], vFlame, 3, uvlFlame, NULL, NULL, 0, 1, GL_TRIANGLES, GL_CLAMP);
			else {
				glBegin (GL_TRIANGLES);
				for (i = 0; i < 3; i++) {
					glTexCoord2fv ((GLfloat *) (uvlFlame + i));
					glVertex3fv ((GLfloat *) (vFlame + i));
					}
				glEnd ();
				}
			}
		if (gameOpts->render.bDepthSort > 0)
			RIAddPoly (bmpThruster [nStyle], vThruster, 4, uvlThruster, NULL, NULL, 0, 1, GL_QUADS, GL_CLAMP);
		else {
			glBegin (GL_QUADS);
			for (i = 0; i < 4; i++) {
				glTexCoord2fv ((GLfloat *) (uvlThruster + i));
				glVertex3fv ((GLfloat *) (vThruster + i));
				}
			}
		glEnd ();
		}
	glEnable (GL_CULL_FACE);
	}
else {
	tUVLf	uvl, uvlStep;

	CreateThrusterFlame ();
	glLineWidth (3);
	glCullFace (GL_FRONT);
	uvlStep.v.u = 1.0f / RING_SIZE;
	uvlStep.v.v = 0.5f / THRUSTER_SEGS;
	for (h = 0; h < nThrusters; h++) {
		if (bTextured) {
			float c = 1; //0.8f + 0.02f * fPulse;
			glColor3f (c, c, c); //, 0.9f);
			}
		else 
			{
			c [1].red = 0.5f + 0.05f * fPulse;
			c [1].green = 0.45f + 0.045f * fPulse;
			c [1].blue = 0.0f;
			c [1].alpha = 0.9f;
			}
		G3StartInstanceMatrix (vPos + h, (pp && !bSpectate) ? &pp->mOrient : &objP->position.mOrient);
		for (i = 0; i < THRUSTER_SEGS - 1; i++) {
#if 1
			if (!bTextured) {
				c [0] = c [1];
				c [1].red *= 0.975f;
				c [1].green *= 0.8f;
				c [1].alpha *= fFade [i / 4];
				}
			glBegin (GL_QUAD_STRIP);
			for (j = 0; j < RING_SIZE + 1; j++) {
				k = j % RING_SIZE;
				uvl.v.u = j * uvlStep.v.u;
				for (l = 0; l < 2; l++) {
					v = vFlame [i + l][k];
					v.p.x *= fSize;
					v.p.y *= fSize;
					v.p.z *= fLength;
					G3TransformPointf (&v, &v, 0);
					if (bTextured) {
						uvl.v.v = 0.25f + uvlStep.v.v * (i + l);
						glTexCoord2fv ((GLfloat *) &uvl);
						}
					else
						glColor4fv ((GLfloat *) (c + l)); // (c [l].red, c [l].green, c [l].blue, c [l].alpha);
					glVertex3fv ((GLfloat *) &v);
					}
				}
			glEnd ();
#else
			glBegin (GL_LINE_LOOP);
			glColor4f (c [1].red, c [1].green, c [1].blue, c [1].alpha);
			for (j = 0; j < RING_SIZE; j++) {
				G3TransformPointf (&v, vFlame [i] + j);
				glVertex3fv ((GLfloat *) &v);
				}
			glEnd ();
#endif
			}
		glBegin (GL_TRIANGLE_STRIP);
		for (j = 0; j < RING_SIZE; j++) {
			G3TransformPointf (&v, vFlame [0] + nStripIdx [j], 0);
			glVertex3fv ((GLfloat *) &v);
			}
		glEnd ();
		G3DoneInstance ();
		}
	glLineWidth (1);
	glCullFace (GL_BACK);
	}
glDepthMask (1);
if (bStencil)
	glEnable (GL_STENCIL_TEST);
}

// -----------------------------------------------------------------------------

typedef struct tVertRef {
	float	dot;
	int	i;
} tVertRef;

typedef struct fEdge {
	int	v0, v1;
	int	f0, f1;
	int	bContour;
	int	nPred, nSucc;	//previous and next connected contour edge
} fEdge;

// -----------------------------------------------------------------------------

inline int VmVecEqualf (fVector *v0, fVector *v1)
{
return (v0->p.x == v1->p.x) && (v0->p.y == v1->p.y) && (v0->p.z == v1->p.z);
}

static int AddEdge (fEdge *edges, int nEdges, int v0, int v1, int nFace)
{
	int	i;

for (i = 0; i < nEdges; i++, edges++)
	if (((edges->v0 == v0) && (edges->v1 == v1)) || ((edges->v0 == v1) && (edges->v1 == v0))) {
		edges->f1 = nFace;
		return nEdges;
		}
edges->v0 = v0;
edges->v1 = v1;
edges->f0 = nFace;
edges->f1 = -1;
return nEdges + 1;
}

// -----------------------------------------------------------------------------

int ObjectModelIndex (tObject *objP)
{
	tPolyModel	*pm = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
	int			i, j;

if (i = objP->rType.polyObjInfo.nSubObjFlags)
	for (j = 0; i && (j < pm->nModels); i >>= 1, j++)
		if (i & 1)
			return j;
return 0;
}

// -----------------------------------------------------------------------------

#define EXPAND_CORONA	2

void RenderLaserCorona (tObject *objP, tRgbaColorf *colorP, float alpha, float fScale)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (gameOpts->render.bObjectCoronas && LoadCorona ()) {
	int			bStencil, bDrawArrays, i;
	float			a1, a2;
	fVector		vCorona [4], vh [5], vPos, vNorm, vDir;
	tHitbox		*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
	float			fLength = f2fl (phb->vMax.p.z - phb->vMin.p.z) / 2;
	float			fRad = f2fl (phb->vMax.p.x - phb->vMin.p.x) / 2;

	static fVector	vEye = {{0, 0, 0}};
	static tUVLf	uvlCorona [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};

	if (bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	glDepthMask (0);
	glEnable (GL_TEXTURE_2D);
	if (OglBindBmTex (bmpCorona, 1, -1)) 
		return;
	OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
	colorP->alpha = alpha;
	glColor4fv ((GLfloat *) colorP);
	VmsVecToFloat (&vDir, &objP->position.mOrient.fVec);
	VmsVecToFloat (&vPos, &objP->position.vPos);
	VmVecScaleAddf (vCorona, &vPos, &vDir, fScale * fLength);
	vh [4] = vCorona [0];
	VmVecScaleAddf (vCorona + 3, &vPos, &vDir, -fScale * fLength);
	G3TransformPointf (&vPos, &vPos, 0);
	G3TransformPointf (vCorona, vCorona, 0);
	G3TransformPointf (vCorona + 3, vCorona + 3, 0);
	VmVecNormalf (&vNorm, &vPos, vCorona, &vEye);
	fScale *= fRad * 2;
	VmVecScaleIncf3 (vCorona, &vNorm, fScale);
	VmVecScaleAddf (vCorona + 1, vCorona, &vNorm, -2 * fScale);
	VmVecScaleIncf3 (vCorona + 3, &vNorm, fScale);
	VmVecScaleAddf (vCorona + 2, vCorona + 3, &vNorm, -2 * fScale);
	VmVecNormalf (&vNorm, vCorona, vCorona + 1, vCorona + 2);
	VmVecScaleAddf (vh, vCorona, vCorona + 1, 0.5f);
	VmVecScaleAddf (vh + 2, vCorona + 3, vCorona + 2, 0.5f);
	VmVecScaleAddf (vh + 1, &vPos, &vNorm, fScale);
	VmVecScaleAddf (vh + 3, &vPos, &vNorm, -fScale);
	for (i = 0; i < 4; i++)
		VmVecNormalizef (vh + i, vh + i);
	a1 = (float) fabs (VmVecDotf (vh + 2, vh));
	a2 = (float) fabs (VmVecDotf (vh + 3, vh + 1));
#if 0
	HUDMessage (0, "%1.2f %1.2f", a1, a2);
	glLineWidth (2);
	glColor4d (1,1,1,1);
	glDisable (GL_TEXTURE_2D);
	glBegin (GL_LINES);
	for (i = 1; i < 3; i++)
		glVertex3fv ((GLfloat *) (vh + i));
	glEnd ();
	glLineWidth (1);
	glColor4fv ((GLfloat *) colorP);
#endif
	if (a2 < a1) {
#if 0
		VmVecNormalf (&vNorm, vh + 1, vh + 3, vh + 4);
		VmVecScaleAddf (vh, &vPos, &vNorm, fScale);
		VmVecScaleAddf (vh + 2, &vPos, &vNorm, -fScale);
		glBegin (GL_QUADS);
		for (i = 0; i < 4; i++) {
			glTexCoord2fv ((GLfloat *) (uvlCorona + i));
			glVertex3fv ((GLfloat *) (vh + i));
			}
		glEnd ();
#	if 1
		glLineWidth (2);
		glColor4d (1,1,1,1);
		glDisable (GL_TEXTURE_2D);
		glBegin (GL_LINE_LOOP);
		for (i = 0; i < 4; i++)
			glVertex3fv ((GLfloat *) (vh + i));
		glEnd ();
		glLineWidth (1);
#	endif
#else
		fix xSize = fl2f (fScale);
		G3DrawSprite (&objP->position.vPos, xSize, xSize, bmpCorona, colorP, alpha);
#endif
		}
	else {
		if (bDrawArrays = OglEnableClientStates (1, 0)) {
			glTexCoordPointer (2, GL_FLOAT, sizeof (tUVLf), uvlCorona);
			glVertexPointer (3, GL_FLOAT, sizeof (fVector), vCorona);
			glDrawArrays (GL_QUADS, 0, 4);
			OglDisableClientStates (1, 0);
			}
		else {
			glBegin (GL_QUADS);
			for (i = 0; i < 4; i++) {
				glTexCoord2fv ((GLfloat *) (uvlCorona + i));
				glVertex3fv ((GLfloat *) (vCorona + i));
				}
			glEnd ();
			}
#if 0
		glLineWidth (2);
		glColor4d (1,1,1,1);
		glDisable (GL_TEXTURE_2D);
		glBegin (GL_LINE_LOOP);
		for (i = 0; i < 4; i++)
			glVertex3fv ((GLfloat *) (vCorona + i));
		glEnd ();
		glLineWidth (1);
#endif
		}
	glDepthMask (1);
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

void RenderObjectCorona (tObject *objP, tRgbaColorf *colorP, float alpha, fix xOffset, float fScale, int bSimple, int bViewerOffset, int bDepthSort)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if ((objP->nType == OBJ_WEAPON) && (objP->renderType == RT_POLYOBJ))
	RenderLaserCorona (objP, colorP, alpha, fScale);
else if (gameOpts->render.bObjectCoronas && LoadCorona ()) {
	int			bStencil;
	fix			xSize = (fix) (objP->size * fScale);

	static tUVLf	uvlList [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};

	vmsVector	vPos = objP->position.vPos;
	bDepthSort = bDepthSort && bSimple && (gameOpts->render.bDepthSort > 0);
	if (xOffset) {
		if (bViewerOffset) {
			vmsVector o;
			VmVecNormalize (VmVecSub (&o, &gameData.render.mine.viewerEye /*&gameData.objs.console->position.vPos*/, &vPos));
			VmVecScaleInc (&vPos, &o, xOffset);
			}
		else
			VmVecScaleInc (&vPos, &objP->position.mOrient.fVec, xOffset);
		}
	if (xSize < F1_0)
		xSize = F1_0;
	if (bDepthSort) {
		colorP->alpha = alpha;
		RIAddSprite (bmpCorona, &vPos, colorP, FixMulDiv (xSize, bmpCorona->bm_props.w, bmpCorona->bm_props.h), xSize, 0);
		return;
		}
	if (bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3))
		glDisable (GL_STENCIL_TEST);
	glDepthMask (0);
	if (bSimple) {
		G3DrawBitmap (&vPos, FixMulDiv (xSize, bmpCorona->bm_props.w, bmpCorona->bm_props.h), xSize, bmpCorona, 
						  colorP, alpha, 1, 1);
		}
	else {
		fVector	quad [4], verts [8], vCenter, vNormal, v;
		float		dot;
		int		i, j;

		glDisable (GL_CULL_FACE);
		glDepthFunc (GL_LEQUAL);
		glDepthMask (0);
		glEnable (GL_TEXTURE_2D);
		if (OglBindBmTex (bmpCorona, 1, -1)) 
			return;
		OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
		G3StartInstanceMatrix (&vPos, &objP->position.mOrient);
		TransformHitboxf (objP, verts, 0);
		for (i = 0; i < 6; i++) {
			vCenter.p.x = vCenter.p.y = vCenter.p.z = 0;
			for (j = 0; j < 4; j++) {
				quad [j] = verts [hitboxFaceVerts [i][j]];
				VmVecIncf (&vCenter, quad + j);
				}
			VmVecScalef (&vCenter, &vCenter, 0.25f);
			VmVecNormalf (&vNormal, quad, quad + 1, quad + 2);
			VmVecNormalizef (&v, &vCenter);
			dot = VmVecDotf (&vNormal, &v);
			if (dot >= 0)
				continue;
			glColor4f (colorP->red, colorP->green, colorP->blue, alpha * (float) sqrt (-dot));
			glBegin (GL_QUADS);
			for (j = 0; j < 4; j++) {
				VmVecSubf (&v, quad + j, &vCenter);
				VmVecScaleIncf3 (quad + j, &v, fScale);
 				glTexCoord2fv ((GLfloat *) (uvlList + j));
				glVertex3fv ((GLfloat *) (quad + j));
				}
			glEnd ();
			}
		G3DoneInstance ();
		glDepthFunc (GL_LESS);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_CULL_FACE);
		}

	glDepthMask (1);
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

extern vmsAngVec avZero;

void RenderShockwave (tObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if ((objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->id]) {
		vmsVector	vPos;
		int			bStencil;

	VmVecScaleAdd (&vPos, &objP->position.vPos, &objP->position.mOrient.fVec, objP->size / 2);
	if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
		glDisable (GL_STENCIL_TEST);
	if (EGI_FLAG (bShockwaves, 1, 1, 0) && 
		 (objP->mType.physInfo.velocity.p.x || objP->mType.physInfo.velocity.p.y || objP->mType.physInfo.velocity.p.z)) {
			fVector			vPosf;
			int				h, i, j, k, n;
			float				r [4], l [4], alpha;
			tRgbaColorf		*pc = gameData.weapons.color + objP->id;

		G3StartInstanceMatrix (&vPos, &objP->position.mOrient);
		glDepthMask (0);
		glDisable (GL_TEXTURE_2D);
		//glCullFace (GL_FRONT);
		glDisable (GL_CULL_FACE);		
		r [3] = f2fl (objP->size);
		if (r [3] >= 3.0f)
			r [3] /= 1.5f;
		else if (r [3] < 1)
			r [3] *= 2;
		else if (r [3] < 2)
			r [3] *= 1.5f;
		r [2] = r [3];
		r [1] = r [2] / 4.0f * 3.0f;
		r [0] = r [2] / 3;
		l [3] = (r [3] < 1.0f) ? 10.0f : 20.0f;
		l [2] = r [3] / 4;
		l [1] = -r [3] / 6;
		l [0] = -r [3] / 3;
		alpha = 0.15f;
		for (h = 0; h < 3; h++) {
			glBegin (GL_QUAD_STRIP);
			for (i = 0; i < RING_SIZE + 1; i++) {
				j = i % RING_SIZE;
				for (k = 0; k < 2; k++) {
					n = h + k;
					glColor4f (pc->red, pc->green, pc->blue, (n == 3) ? 0.0f : alpha);
					vPosf = vRing [j];
					vPosf.p.x *= r [n];
					vPosf.p.y *= r [n];
					vPosf.p.z = -l [n];
					G3TransformPointf (&vPosf, &vPosf, 0);
					glVertex3fv ((GLfloat *) &vPosf);
					}
				}
			glEnd ();
			}
		glEnable (GL_CULL_FACE);		
		for (h = 0; h < 3; h += 2) {
			glCullFace (h ? GL_FRONT : GL_BACK);
			glColor4f (pc->red, pc->green, pc->blue, h ? 0.1f : alpha);
			glBegin (GL_TRIANGLE_STRIP);
			for (j = 0; j < RING_SIZE; j++) {
				vPosf = vRing [nStripIdx [j]];
				vPosf.p.x *= r [h];
				vPosf.p.y *= r [h];
				vPosf.p.z = -l [h];
				G3TransformPointf (&vPosf, &vPosf, 0);
				glVertex3fv ((GLfloat *) &vPosf);
				}
			glEnd ();
			}
		glDepthMask (1);
		glCullFace (GL_BACK);
		G3DoneInstance ();
		}
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
	}
}

// -----------------------------------------------------------------------------

#define TRACER_WIDTH	3

void RenderTracers (tObject *objP)
{
if (!SHOW_OBJ_FX)
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (EGI_FLAG (bTracers, 0, 1, 0) &&
	 (objP->nType == OBJ_WEAPON) && ((objP->id == VULCAN_ID) || (objP->id == GAUSS_ID)
	 /*&& !gameData.objs.nTracers [objP->cType.laserInfo.nParentObj]*/)) {
#if 0
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [SUPERLASER_ID + 1].nModel;
	objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad, 
								gameData.weapons.info [objP->id].po_len_to_width_ratio) / 4;
	gameData.models.nScale = F1_0 / 4;
	DrawPolygonObject (objP);
	gameData.models.nScale = 0;
#else
		fVector			vPosf [2], vDirf;
		short				i;
		int				bStencil;
//		static short	patterns [] = {0x0603, 0x0203, 0x0103, 0x0202};

	VmsVecToFloat (vPosf, &objP->position.vPos);
	VmsVecToFloat (vPosf + 1, &objP->vLastPos);
	G3TransformPointf (vPosf, vPosf, 0);
	G3TransformPointf (vPosf + 1, vPosf + 1, 0);
	VmVecSubf (&vDirf, vPosf, vPosf + 1);
	if (!(vDirf.p.x || vDirf.p.y || vDirf.p.z)) {
		//return;
		VmsVecToFloat (vPosf + 1, &gameData.objs.objects [objP->cType.laserInfo.nParentObj].position.vPos);
		G3TransformPointf (vPosf + 1, vPosf + 1, 0);
		VmVecSubf (&vDirf, vPosf, vPosf + 1);
		if (!(vDirf.p.x || vDirf.p.y || vDirf.p.z))
			return;
		}
	if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
		glDisable (GL_STENCIL_TEST);
	glDepthMask (0);
	glEnable (GL_LINE_STIPPLE);
	glEnable (GL_BLEND);
	OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_LINE_SMOOTH);
	glLineStipple (6, 0x003F); //patterns [h]);
	vDirf.p.x *= TRACER_WIDTH / 20.0f;
	vDirf.p.y *= TRACER_WIDTH / 20.0f;
	vDirf.p.z *= TRACER_WIDTH / 20.0f;
	for (i = 1; i < 5; i++) {
		glLineWidth ((GLfloat) (TRACER_WIDTH * i));
		glBegin (GL_LINES);
		glColor4d (1, 1, 1, 0.5 / i);
		glVertex3fv ((GLfloat *) (vPosf + 1));
		glVertex3fv ((GLfloat *) vPosf);
#if 0
		VmVecDecf (vPosf, &vDirf);
		VmVecDecf (vPosf + 1, &vDirf);
#endif
		glEnd ();
		}
	glLineWidth (1);
	glDisable (GL_LINE_STIPPLE);
	glDisable (GL_LINE_SMOOTH);
	glDepthMask (1);
	if (bStencil)
		glEnable (GL_STENCIL_TEST);
#endif
	}
}

// -----------------------------------------------------------------------------

static fVector vTrailOffs [2][4] = {{{{0,0,0}},{{0,-10,-5}},{{0,-10,-50}},{{0,0,-50}}},
												{{{0,0,0}},{{0,10,-5}},{{0,10,-50}},{{0,0,-50}}}};

void RenderLightTrail (tObject *objP)
{
	tRgbaColorf		c, *pc;

if (!SHOW_OBJ_FX)
	return;
if (!gameData.objs.bIsWeapon [objP->id])
	return;
#if SHADOWS
if (SHOW_SHADOWS && (gameStates.render.nShadowPass != 1))
//	 (FAST_SHADOWS ? (gameStates.render.nShadowPass != 3) : (gameStates.render.nShadowPass != 1)))
	return;
#endif
if (objP->renderType == RT_POLYOBJ)
	pc = gameData.weapons.color + objP->id;
else {
	tRgbColorb	*pcb = VClipColor (objP);
	c.red = pcb->red / 255.0f;
	c.green = pcb->green / 255.0f;
	c.blue = pcb->blue / 255.0f;
	pc = &c;
	}

if (!gameData.objs.bIsSlowWeapon [objP->id]) {
	if (gameOpts->render.smoke.bPlasmaTrails)
		DoObjectSmoke (objP);
	else if (EGI_FLAG (bLightTrails, 1, 1, 0) && (objP->nType == OBJ_WEAPON) && 
				!gameData.objs.bIsSlowWeapon [objP->id] &&
				(objP->mType.physInfo.velocity.p.x || objP->mType.physInfo.velocity.p.y || objP->mType.physInfo.velocity.p.z) &&
				LoadCorona ()) {
			fVector			vNormf, vOffsf, vTrailVerts [4];
			int				i, bStencil, bDrawArrays, bDepthSort = (gameOpts->render.bDepthSort > 0);
			float				l, r = f2fl (objP->size);

			static fVector vEye = {{0, 0, 0}};

			static tRgbaColorf	trailColor = {0,0,0,0.33f};
			static tUVLf			uvlTrail [4] = {{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}};;
			
		if (r >= 3.0f)
			r /= 1.5f;
		else if (r < 1)
			r *= 2;
		else if (r < 2)
			r *= 1.5f;
		if (objP->renderType == RT_POLYOBJ) {
			tHitbox	*phb = gameData.models.hitboxes [objP->rType.polyObjInfo.nModel].hitboxes;
			l = f2fl (phb->vMax.p.z - phb->vMin.p.z);
			if (objP->id == FUSION_ID)
				l *= 1.5;
			}
		else
			l = 4 * r;

		VmsVecToFloat (&vOffsf, &objP->position.mOrient.fVec);
		VmsVecToFloat (vTrailVerts, &objP->position.vPos);
		VmVecScaleIncf3 (vTrailVerts, &vOffsf, l);// * -0.75f);
		VmVecScaleAddf (vTrailVerts + 2, vTrailVerts, &vOffsf, -100);
		G3TransformPointf (vTrailVerts, vTrailVerts, 0);
		G3TransformPointf (vTrailVerts + 2, vTrailVerts + 2, 0);
		VmVecSubf (&vOffsf, vTrailVerts + 2, vTrailVerts);
		VmVecScalef (&vOffsf, &vOffsf, r * 0.04f);
		VmVecNormalf (&vNormf, vTrailVerts, vTrailVerts + 2, &vEye);
		VmVecScalef (&vNormf, &vNormf, r * 4);
		VmVecAddf (vTrailVerts + 1, vTrailVerts, &vNormf);
		VmVecIncf (vTrailVerts + 1, &vOffsf);
		VmVecSubf (vTrailVerts + 3, vTrailVerts, &vNormf);
		VmVecIncf (vTrailVerts + 3, &vOffsf);
		if (bDepthSort) {
			memcpy (&trailColor, pc, 3 * sizeof (float));
			RIAddPoly (bmpCorona, vTrailVerts, 4, uvlTrail, &trailColor, NULL, 1, 0, GL_QUADS, GL_CLAMP);
			}
		else {
			bDrawArrays = OglEnableClientStates (1, 0);
			if ((bStencil = SHOW_SHADOWS && (gameStates.render.nShadowPass == 3)))
				glDisable (GL_STENCIL_TEST);
			glDisable (GL_CULL_FACE);		
			glDepthMask (0);
			glEnable (GL_TEXTURE_2D);
			if (OglBindBmTex (bmpCorona, 1, -1)) 
				return;
			OglTexWrap (bmpCorona->glTexture, GL_CLAMP);
			glColor4f (pc->red, pc->green, pc->blue, 0.5f);
			if (bDrawArrays) {
				glVertexPointer (3, GL_FLOAT, sizeof (fVector), vTrailVerts);
				glTexCoordPointer (2, GL_FLOAT, sizeof (tUVLf), uvlTrail);
				glDrawArrays (GL_QUADS, 0, 4);
				OglDisableClientStates (1, 0);
				}
			else {
				glBegin (GL_QUADS);
				for (i = 0; i < 4; i++) {
					glTexCoord3fv ((GLfloat *) (uvlTrail + i));
					glVertex3fv ((GLfloat *) (vTrailVerts + i));
					}
				glEnd ();
#if 0 // render outline
				glDisable (GL_TEXTURE_2D);
				glColor3d (1, 0, 0);
				glBegin (GL_LINE_LOOP);
				for (i = 0; i < 4; i++)
					glVertex3fv ((GLfloat *) (vTrailVerts + i));
				glEnd ();
#endif
				}
			if (bStencil)
				glEnable (GL_STENCIL_TEST);
			glEnable (GL_CULL_FACE);
			glDepthMask (1);
			}
		}
	RenderShockwave (objP);
	}
if ((objP->renderType != RT_POLYOBJ) || (objP->id == FUSION_ID))
	RenderObjectCorona (objP, pc, 0.5f, 0, 3, 1, 0, 1);
else
	RenderObjectCorona (objP, pc, 0.75f, 0, 3, 0, 0, 0);
}

// -----------------------------------------------------------------------------

void DrawDebrisCorona (tObject *objP)
{
	static	tRgbaColorf	debrisGlow = {0.66f, 0, 0, 1};
	static	tRgbaColorf	markerGlow = {0, 0.66f, 0, 1};
	static	time_t t0 = 0;

if (objP->nType == OBJ_MARKER)
	RenderObjectCorona (objP, &markerGlow, 0.75f, 0, 3, 1, 1, 0);
#ifdef _DEBUG
else if (objP->nType == OBJ_DEBRIS) {
#else
else if ((objP->nType == OBJ_DEBRIS) && gameOpts->render.nDebrisLife) {
#endif
	float	h = (float) nDebrisLife [gameOpts->render.nDebrisLife] - f2fl (objP->lifeleft);
	if (h < 0)
		h = 0;
	if (h < 10) {
		h = (10 - h) / 20.0f;
		if (gameStates.app.nSDLTicks - t0 > 50) {
			t0 = gameStates.app.nSDLTicks;
			debrisGlow.red = 0.5f + f2fl (d_rand () % (F1_0 / 4));
			debrisGlow.green = f2fl (d_rand () % (F1_0 / 4));
			}
		RenderObjectCorona (objP, &debrisGlow, h, 5 * objP->size / 2, 1.5f, 1, 1, 0);
		}
	}
}

// -----------------------------------------------------------------------------

bool G3DrawSphere3D  (g3sPoint *p0, int nSides, int rad);

int RenderObject (tObject *objP, int nWindowNum, int bForce)
{
	int			mldSave, bSpectate = 0;
	tPosition	savePos;
#if 0
	float			fLight [3];
	fix			nGlow [2];
	int			oofIdx;
#endif

#if 0//def _DEBUG
if (objP == dbgObjP) {
	objP = objP;
#if 1
	HUDMessage (0, "%1.2f %1.2f %1.2f", 
					f2fl (objP->mType.physInfo.velocity.p.x), 
					f2fl (objP->mType.physInfo.velocity.p.y), 
					f2fl (objP->mType.physInfo.velocity.p.z));
#endif
	}
#endif
if ((objP == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer]) && 
	 (objP->nSignature == gameData.objs.guidedMissileSig [gameData.multiplayer.nLocalPlayer]) &&
	 (gameStates.render.nShadowPass != 2))
	return 0;
if ((OBJ_IDX (objP) == LOCALPLAYER.nObject) &&
	 (gameData.objs.viewer == gameData.objs.console) &&
	 !gameStates.render.automap.bDisplay) {
	if ((bSpectate = (gameStates.app.bFreeCam && !nWindowNum)))
		;
		//HUDMessage (0, "%1.2f %1.2f %1.2f", f2fl (objP->position.vPos.p.x), f2fl (objP->position.vPos.p.y), f2fl (objP->position.vPos.p.z));
#ifdef _DEBUG
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindowNum || (!gameStates.render.bExternalView && (gameStates.app.bEndLevelSequence < EL_LOOKBACK)))) { //don't render ship model if neither external view nor main view
#else	 
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindowNum ||
				  ((IsMultiGame && !IsCoopGame && !EGI_FLAG (bEnableCheats, 0, 0, 0)) || 
				  (!gameStates.render.bExternalView && (gameStates.app.bEndLevelSequence < EL_LOOKBACK))))) {
#endif	 	
		if (gameOpts->render.smoke.bPlayers)
			DoPlayerSmoke (objP, -1);
		return 0;		
		}
	}
if ((objP->nType == OBJ_NONE)/* || (objP->nType==OBJ_CAMBOT)*/){
#if TRACE				
	con_printf (1, "ERROR!!!Bogus obj %d in seg %d is rendering!\n", OBJ_IDX (objP), objP->nSegment);
#endif
	return 0;
	}
mldSave = gameStates.render.detail.nMaxLinearDepth;
gameStates.render.nState = 1;
gameData.objs.color.index = 0;
gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;
#if 0//def _DEBUG
if (objP->nType == OBJ_EXPLOSION) {
	static time_t	t0 = 0;
	static int i = 0,
					nClips [] = {0, 2, 5, 7, 57, 58, 59, 60, 106};

	if (gameStates.app.nSDLTicks - t0 > 4000) {
		if (t0)
			do {
				i = (i + 1) % sizeofa (nClips);
				} while (gameData.eff.vClips [0][nClips [i]].xFrameTime <= 0);

		t0 = gameStates.app.nSDLTicks;
		objP->rType.vClipInfo.nClipIndex = nClips [i];
		}
	HUDMessage (0, "%d", nClips [i]);
	}
#endif
switch (objP->renderType) {
	case RT_NONE:	
		if (gameStates.render.nType != 1)
			return 0;
		RenderTracers (objP);
		break;		//doesn't render, like the tPlayer

	case RT_POLYOBJ:
		if (objP->nType == OBJ_SMOKE) {
			objP->renderType = RT_NONE;
			return 0;
			}
		if (gameStates.render.nType != 1)
			return 0;
		DoObjectSmoke (objP);
		if (objP->nType == OBJ_PLAYER) {
			int bDynObjLight = gameOpts->ogl.bLightObjects;
			if (gameStates.render.automap.bDisplay && !(AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->id)))
				return 0;
			if (bSpectate) {
				savePos = objP->position;
				objP->position = gameStates.app.playerPos;
				}
			DrawPolygonObject (objP);
			gameOpts->ogl.bLightObjects = bDynObjLight;
			RenderThrusterFlames (objP);
			RenderPlayerShield (objP);
			RenderTargetIndicator (objP, NULL);
			RenderTowedFlag (objP);
			if (bSpectate)
				objP->position = savePos;
			}
		else if (objP->nType == OBJ_ROBOT) {
			if (gameStates.render.automap.bDisplay && !AM_SHOW_ROBOTS)
				return 0;
			DrawPolygonObject (objP);
			RenderThrusterFlames (objP);
#if 0//def _DEBUG
			RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#else
			if (gameStates.render.nShadowPass != 2) {
				if (gameOpts->render.bRobotShields && !objP->cType.aiInfo.CLOAKED) {
					if (gameStates.app.nSDLTicks - gameData.objs.xTimeLastHit [OBJ_IDX (objP)] < 300)
						DrawShieldSphere (objP, 1.0f, 0.5f, 0, 0.5f);
					else if (ROBOTINFO (objP->id).companion)
						DrawShieldSphere (objP, 0.0f, 0.5f, 1.0f, ObjectDamage (objP) / 2);
					else
						DrawShieldSphere (objP, 0.75f, 0.0f, 0.75f, ObjectDamage (objP) / 2);
					}
#endif
				RenderTargetIndicator (objP, NULL);
				SetRobotLocationInfo (objP);
				}
			}
		else if (objP->nType == OBJ_WEAPON) {
			if (gameStates.render.automap.bDisplay && !AM_SHOW_POWERUPS (1))
				return 0;
			if (!(gameStates.app.bNostalgia || gameOpts->render.powerups.b3D) && 
				 ((objP->id == PROXMINE_ID) || (objP->id == SMARTMINE_ID)))
				ConvertWeaponToVClip (objP);
			else {

#if 1//def RELEASE
#endif
				if (gameData.objs.bIsMissile [objP->id]) {
					DrawPolygonObject (objP);
#if 0//def _DEBUG
#	if 0
					DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#	else
					RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
					RenderThrusterFlames (objP);
					}
				else {
#if 0//def _DEBUG
#	if 0
					DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#	else
					RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
					if (objP->nType != OBJ_WEAPON)
						DrawPolygonObject (objP);
					if (objP->id != SMALLMINE_ID)
						RenderLightTrail (objP);
					if (objP->nType == OBJ_WEAPON)
						DrawPolygonObject (objP);
					}
				}
			}
		else if (objP->nType == OBJ_CNTRLCEN) {
			DrawPolygonObject (objP);
			RenderTargetIndicator (objP, NULL);
			}
		else if (objP->nType == OBJ_POWERUP) {
			if (gameStates.render.automap.bDisplay && !AM_SHOW_POWERUPS (1))
				return 0;
			if (!gameStates.app.bNostalgia && gameOpts->render.powerups.b3D) {
				if ((objP->id == POW_SMARTMINE) || (objP->id == POW_PROXMINE))
					gameData.models.nScale = 2 * F1_0;
				else
					gameData.models.nScale = 3 * F1_0 / 2;
				DrawPolygonObject (objP);
				gameData.models.nScale = 0;
				objP->mType.physInfo.mass = F1_0;
				objP->mType.physInfo.drag = 512;
				if (gameOpts->render.powerups.nSpin != 
					((objP->mType.physInfo.rotVel.p.y | objP->mType.physInfo.rotVel.p.z) != 0))
					objP->mType.physInfo.rotVel.p.y = 
					objP->mType.physInfo.rotVel.p.z = gameOpts->render.powerups.nSpin ? F1_0 / (5 - gameOpts->render.powerups.nSpin) : 0;
				}
			else
				ConvertWeaponToPowerup (objP);
			}
		else {
#if 1//ndef _DEBUG
			DrawPolygonObject (objP);
#endif
			DrawDebrisCorona (objP);
			}
		break;

	case RT_MORPH:	
		if (gameStates.render.nType != 1)
			return 0;
		if (gameStates.render.nShadowPass != 2)
			MorphDrawObject (objP); 
		break;

	case RT_THRUSTER: 
		if (gameStates.render.nType != 1)
			return 0;
		if (nWindowNum && (objP->mType.physInfo.flags & PF_WIGGLE))
			break;
			
	case RT_FIREBALL: 
		if (!bForce && (gameStates.render.nType != 1))
			return 0;
		if (gameStates.render.nShadowPass != 2) {
			DrawFireball (objP); 
			if (objP->nType == OBJ_WEAPON) {
				RenderLightTrail (objP);
				}
			}
		break;

	case RT_EXPLBLAST: 
		if (!bForce && (gameStates.render.nType != 1))
			return 0;
		if (gameStates.render.nShadowPass != 2)
			DrawExplBlast (objP); 
		break;

	case RT_SHRAPNELS: 
		if (!bForce && (gameStates.render.nType != 1))
			return 0;
		if (gameStates.render.nShadowPass != 2)
			DrawShrapnels (objP); 
		break;

	case RT_WEAPON_VCLIP: 
		if (gameStates.render.nType != 1)
			return 0;
		if (gameStates.render.nShadowPass != 2) {
			if (gameStates.render.automap.bDisplay && !AM_SHOW_POWERUPS (1))
				return 0;
			if (objP->nType == OBJ_WEAPON) {
				if ((objP->id == PROXMINE_ID) || (objP->id == SMARTMINE_ID) || (objP->id == SMALLMINE_ID)) {
					if (!DoObjectSmoke (objP))
						DrawWeaponVClip (objP); 
					}	
				else if ((objP->id != OMEGA_ID) || !(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bOmega)) {
					DrawWeaponVClip (objP); 
					if (objP->id != OMEGA_ID) {
						RenderLightTrail (objP);
						RenderMslLockIndicator (objP);
						}
					}
				}
			else
				DrawWeaponVClip (objP); 
#if 0//def _DEBUG
			if (EGI_FLAG (bRenderShield, 0, 1, 0))
				DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#endif
			}
		break;

	case RT_HOSTAGE: 
		if (gameStates.render.nType != 1)
			return 0;
		if (gameStates.render.nShadowPass != 2)
			DrawHostage (objP); 
		break;

	case RT_POWERUP:
		if (gameStates.render.automap.bDisplay && !AM_SHOW_POWERUPS (1))
			return 0;
		if (gameStates.render.nType != 1)
			return 0;
		if (ConvertPowerupToWeapon (objP))
			DrawPolygonObject (objP);
		else if (gameStates.render.nShadowPass != 2)
			DrawPowerup (objP); 
		break;

	case RT_LASER: 
		if (gameStates.render.nType != 1)
			return 0;
		if (gameStates.render.nShadowPass != 2) {
			RenderLaser (objP); 
			if (objP->nType == OBJ_WEAPON)
				RenderLightTrail (objP);
			}
		break;
		
	case RT_LIGHTNING:
		break;

	default: 
		Error ("Unknown renderType <%d>", objP->renderType);
	}
SetNearestStaticLights (objP->nSegment, 0);

#ifdef NEWDEMO
if (objP->renderType != RT_NONE)
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		if (!gameData.demo.bWasRecorded [OBJ_IDX (objP)]) {
			NDRecordRenderObject (objP);
			gameData.demo.bWasRecorded [OBJ_IDX (objP)] = 1;
		}
	}
#endif
gameStates.render.detail.nMaxLinearDepth = mldSave;
return 1;
}

//------------------------------------------------------------------------------
//eof
