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
#include "error.h"
#include "newdemo.h"
#include "network.h"
#include "automap.h"
#include "paging.h"
#include "endlevel.h"
#include "light.h"
#include "lightning.h"
#include "hostage.h"
#include "interp.h"
#include "render.h"
#include "renderlib.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "flightpath.h"
#include "objsmoke.h"
#include "fireball.h"
#include "objrender.h"
#include "objeffects.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "marker.h"
#include "hiresmodels.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

int InitAddonPowerup (tObject *objP)
{
if (objP->id == POW_SLOWMOTION)
	objP->rType.vClipInfo.nClipIndex = -1;
else if (objP->id == POW_BULLETTIME)
	objP->rType.vClipInfo.nClipIndex = -2;
else
	return 0;
objP->rType.vClipInfo.nCurFrame = 0;
objP->rType.vClipInfo.xTotalTime = 0;
return 1;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToPowerup (tObject *objP)
{
if (!InitAddonPowerup (objP)) {
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.pVClips [objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->size = gameData.objs.pwrUp.info [objP->id].size;
	}
objP->controlType = CT_POWERUP;
objP->renderType = RT_POWERUP;
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

int ConvertHostageToModel (tObject *objP)
{
if (objP->renderType == RT_POLYOBJ)
	return 1;
if (gameStates.app.bNostalgia || !gameOpts->render.powerups.b3D)
	return 0;
if (!HaveReplacementModel (HOSTAGE_MODEL))
	return 0;
objP->renderType = RT_POLYOBJ;
objP->rType.polyObjInfo.nModel = HOSTAGE_MODEL;
objP->rType.polyObjInfo.nTexOverride = -1;
objP->mType.physInfo.rotVel.p.x = 
objP->mType.physInfo.rotVel.p.y = 
objP->mType.physInfo.rotVel.p.z = 0;
memset (objP->rType.polyObjInfo.animAngles, 0, sizeof (objP->rType.polyObjInfo.animAngles));
return 1;
}

// -----------------------------------------------------------------------------

int ConvertModelToHostage (tObject *objP)
{
objP->rType.vClipInfo.nClipIndex = nHostageVClips [0];
objP->rType.vClipInfo.xFrameTime = gameData.eff.pVClips [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->size = 289845;
objP->controlType = CT_POWERUP;
objP->renderType = RT_HOSTAGE;
objP->mType.physInfo.mass = F1_0;
objP->mType.physInfo.drag = 512;
return 1;
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
if ((objP->nType != OBJ_POWERUP) && (objP->nType != OBJ_WEAPON))
	return 0;
nModel = PowerupToModel (objP->id);
if (objP->id == 33)
	objP = objP;
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
	 !(nModel && HaveReplacementModel (nModel)))
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
#if 1
if (bHasModel)
	objP->size = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
#endif
objP->rType.polyObjInfo.nTexOverride = -1;
objP->lifeleft = IMMORTAL_TIME;
return 1;
}

// -----------------------------------------------------------------------------

void ConvertAllPowerupsToWeapons (void)
{
	int	i;
	tObject	*objP;

for (i = 0, objP = OBJECTS; i < gameData.objs.nLastObject; i++, objP++)
	if (objP->renderType == RT_POWERUP) {
		ConvertPowerupToWeapon (objP);
		PagingTouchObject (objP);
		}
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
	if ((abs (temp.p3_x) < F1_0 * 4) && (abs (temp.p3_y) < F1_0 * 4)) {
		objP->cType.aiInfo.nDangerLaser = gameStates.app.bPlayerFiredLaserThisFrame;
		objP->cType.aiInfo.nDangerLaserSig = gameData.objs.objects [gameStates.app.bPlayerFiredLaserThisFrame].nSignature;
		}
	}
}

//------------------------------------------------------------------------------

fix CalcObjectLight (tObject *objP, fix *xEngineGlow)
{
	fix xLight;

if (gameStates.render.bBuildModels)
	return F1_0;
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

static float ObjectBlobColor (tObject *objP, grsBitmap *bmP, tRgbaColorf *colorP)
{
	float	fScale;

colorP->red = (float) bmP->bmAvgRGB.red / 255.0f;
colorP->green = (float) bmP->bmAvgRGB.green / 255.0f;
colorP->blue = (float) bmP->bmAvgRGB.blue / 255.0f;
#ifdef _DEBUG
if ((objP->nType == nDbgObjType) && ((nDbgObjId < 0) || (objP->id == nDbgObjId)))
	nDbgObjType = nDbgObjType;
#endif
fScale = colorP->red + colorP->green + colorP->blue;
if (fScale == 0) {
	colorP->red = 
	colorP->green =
	colorP->blue = 1;
	}
return fScale;
}

//------------------------------------------------------------------------------
//draw an tObject that has one bitmap & doesn't rotate

void DrawObjectBlob (tObject *objP, int bmi0, int bmi, int iFrame, tRgbaColorf *colorP, float fAlpha)
{
	grsBitmap	*bmP;
	tRgbaColorf	color;
	int			nType = objP->nType;
	int			nId = objP->id;
#if 0
	int			bMuzzleFlash = 0;
#endif
	int			bAdditive = 0, bEnergy = 0, nTransp = (nType == OBJ_POWERUP) ? 3 : 2;
	fix			xSize;
	float			fScale;

#ifdef _DEBUG
if ((objP->nType == nDbgObjType) && ((nDbgObjId < 0) || (objP->id == nDbgObjId)))
	nDbgObjType = nDbgObjType;
#endif
if (gameOpts->render.textures.bUseHires || gameOpts->render.bTransparentEffects) {
	if (fAlpha) {
		bAdditive = (nType == OBJ_FIREBALL) || (nType == OBJ_EXPLOSION);
#if 0
		bMuzzleFlash = (nType == OBJ_FIREBALL) && ((nId == 11) || (nId == 12) || (nId == 15) || (nId == 22) || (nId == 86));
#endif
		}
	else {
		if (nType == OBJ_POWERUP) {
			if (IsEnergyPowerup (nId)) {
				fAlpha = 2.0f / 3.0f;
				bEnergy = 1;
				}
			else
				fAlpha = 1.0f;
			}
		else if ((nType != OBJ_FIREBALL) && (nType != OBJ_EXPLOSION))
			fAlpha = 1.0f;
		else {
			fAlpha = 2.0f / 3.0f;
			bAdditive = 1;
			}
		}
	}
else {
	nTransp = 3;
	fAlpha = 1.0f;
	}
if (bmi < 0) {
	PageInAddonBitmap (bmi);
	bmP = gameData.pig.tex.addonBitmaps - bmi - 1;
#ifdef _DEBUG
	if ((objP->rType.vClipInfo.nCurFrame < 0) || (objP->rType.vClipInfo.nCurFrame >= BM_FRAMECOUNT (bmP))) {
		objP->rType.vClipInfo.nCurFrame = 0;
		return;
		}
#endif
	}
else {
	PIGGY_PAGE_IN (bmi, 0);
	bmP = gameData.pig.tex.bitmaps [0] + bmi;
	}
if ((bmi < 0) || ((bmP->bmType == BM_TYPE_STD) && BM_OVERRIDE (bmP))) {
	OglLoadBmTexture (bmP, 1, nTransp = -1, gameOpts->render.bDepthSort <= 0);
	//fScale = ObjectBlobColor (objP, bmP, &color);
	bmP = BmOverride (bmP, iFrame);
	//fAlpha = 1;
	}
else {
	if (colorP && gameOpts->render.bDepthSort)
		OglLoadBmTexture (bmP, 1, nTransp, 0);
	//fScale = ObjectBlobColor (objP, bmP, &color);
	}
if (!bmP)
	return;
fScale = ObjectBlobColor (objP, bmP, &color);
if (!bmP->bmTexBuf)
	return;
if (colorP && (bmi >= 0))
	memcpy (colorP, gameData.pig.tex.bitmapColors + bmi, sizeof (tRgbaColorf));

xSize = objP->size;

if ((nType == OBJ_POWERUP) && ((bEnergy && gameOpts->render.bPowerupCoronas) || (!bEnergy && gameOpts->render.bWeaponCoronas)))
	RenderPowerupCorona (objP, color.red, color.green, color.blue, 
								coronaIntensities [gameOpts->render.nObjCoronaIntensity]);
if (gameOpts->render.bDepthSort > 0) {
	if (bAdditive) {
#if 1
		color.red = 
		color.green =
		color.blue = 0.5f;
#else
		if ((nType == OBJ_FIREBALL) && (fScale > 0)) {
			fScale = 1.0f - fScale / 6.0f;
			color.red *= fScale;
			color.green *= fScale;
			color.blue *= fScale;
			}
#endif
		}
	else
		color.red =
		color.green =
		color.blue = 1;
	color.alpha = fAlpha;
	if (bmP->bmProps.w > bmP->bmProps.h)
		RIAddSprite (bmP, &objP->position.vPos, &color, xSize, FixMulDiv (xSize, bmP->bmProps.h, bmP->bmProps.w), iFrame, bAdditive);
	else
		RIAddSprite (bmP, &objP->position.vPos, &color, FixMulDiv (xSize, bmP->bmProps.w, bmP->bmProps.h), xSize, iFrame, bAdditive);
	}
else {
	if (bmP->bmProps.w > bmP->bmProps.h)
		G3DrawBitmap (&objP->position.vPos, xSize, FixMulDiv (xSize, bmP->bmProps.h, bmP->bmProps.w), bmP, 
						  NULL, fAlpha, nTransp);
	else
		G3DrawBitmap (&objP->position.vPos, FixMulDiv (xSize, bmP->bmProps.w, bmP->bmProps.h), xSize, bmP, 
						  NULL, fAlpha, nTransp);
	}
}

//------------------------------------------------------------------------------
//draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly (tObject *objP, tBitmapIndex bmi, int bLit, int iFrame)
{
	grsBitmap *bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
	fix light;
	vmsVector delta, top_v, bot_v;
	g3sPoint top_p, bot_p;

PIGGY_PAGE_IN (bmi.index, 0);
if ((bmP->bmType == BM_TYPE_STD) && BM_OVERRIDE (bmP)) {
	OglLoadBmTexture (bmP, 1, -1, gameOpts->render.bDepthSort <= 0);
	bmP = BmOverride (bmP, iFrame);
	}
VmVecCopyScale (&delta, &objP->position.mOrient.uVec, objP->size);
VmVecAdd (&top_v, &objP->position.vPos, &delta);
VmVecSub (&bot_v, &objP->position.vPos, &delta);
G3TransformAndEncodePoint (&top_p, &top_v);
G3TransformAndEncodePoint (&bot_p, &bot_v);
if (bLit)
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

int GetCloakInfo (tObject *objP, fix xCloakStartTime, fix xCloakEndTime, tCloakInfo *ciP)
{
	tCloakInfo	ci = {0, CLOAKED_FADE_LEVEL, F1_0, F1_0, F1_0, 0, 0};
	int			i;

if (!(xCloakStartTime || xCloakEndTime)) {
	if (objP->nType == OBJ_PLAYER) {
		xCloakStartTime = gameData.multiplayer.players [objP->id].cloakTime;
		xCloakEndTime = gameData.multiplayer.players [objP->id].cloakTime + CLOAK_TIME_MAX;
		}
	else if (objP->nType == OBJ_ROBOT) {
		if (!ROBOTINFO (objP->id).bossFlag) {
			xCloakStartTime = gameData.time.xGame - F1_0 * 10;
			xCloakEndTime = gameData.time.xGame + F1_0 * 10;
			}
		else if (0 <= (i = FindBoss (OBJ_IDX (objP)))) {
			xCloakStartTime = gameData.boss [i].nCloakStartTime;
			xCloakEndTime = gameData.boss [i].nCloakEndTime;
			}
		}
	}
if (xCloakStartTime != 0x7fffffff)
	ci.xTotalTime = xCloakEndTime - xCloakStartTime;
else 
	ci.xTotalTime = gameData.time.xGame;
if (objP->nType == OBJ_PLAYER) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_PLAYER;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_PLAYER;
	}
else if (objP->nType == OBJ_ROBOT) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_ROBOT;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_ROBOT;
	}
else
	return 0;

ci.xDeltaTime = gameData.time.xGame - ((xCloakStartTime == 0x7fffffff) ? 0 : xCloakStartTime);
#if 0
if (ci.xDeltaTime < ci.xFadeinDuration) {	// make object transparent during second half
	ci.nFadeValue = f2i (FixDiv (ci.xFadeinDuration - ci.xDeltaTime, ci.xFadeinDuration) * CLOAKED_FADE_LEVEL);
	ci.bFading = 1;
	} 
#else
// only decrease light during first half of cloak initiation time
if (ci.xDeltaTime < ci.xFadeinDuration / 2) {
	ci.xLightScale = FixDiv (ci.xFadeinDuration / 2 - ci.xDeltaTime, ci.xFadeinDuration / 2);
	ci.bFading = -1;
	}
else if (ci.xDeltaTime < ci.xFadeinDuration) {	// make object transparent during second half
	ci.nFadeValue = f2i (FixDiv (ci.xDeltaTime - ci.xFadeinDuration / 2, ci.xFadeinDuration / 2) * CLOAKED_FADE_LEVEL);
	ci.bFading = 1;
	} 
#endif
else if ((xCloakStartTime == 0x7fffffff) || (gameData.time.xGame < xCloakEndTime - ci.xFadeoutDuration)) {
	static int nCloakDelta = 0, nCloakDir = 1;
	static fix xCloakTimer = 0;

	//note, if more than one cloaked tObject is visible at once, the
	//pulse rate will change!
	xCloakTimer -= gameData.time.xFrame;
	while (xCloakTimer < 0) {
		xCloakTimer += ci.xFadeoutDuration / 12;
		nCloakDelta += nCloakDir;
		if (nCloakDelta == 0 || nCloakDelta == 4)
			nCloakDir = -nCloakDir;
		}
	ci.nFadeValue = CLOAKED_FADE_LEVEL - nCloakDelta;
	} 
else if (gameData.time.xGame < xCloakEndTime - ci.xFadeoutDuration / 2) {
	ci.nFadeValue = f2i (FixDiv (ci.xTotalTime - ci.xFadeoutDuration / 2 - ci.xDeltaTime, ci.xFadeoutDuration / 2) * CLOAKED_FADE_LEVEL);
	ci.bFading = -1;
	} 
else {
	ci.xLightScale = (fix) ((float) (ci.xFadeoutDuration / 2 - (ci.xTotalTime - ci.xDeltaTime) / (float) (ci.xFadeoutDuration / 2)));
	ci.bFading = 1;
	}
if (ciP)
	*ciP = ci;
return ci.bFading;
}

//------------------------------------------------------------------------------
//do special cloaked render
int DrawCloakedObject (tObject *objP, fix light, fix *glow, fix xCloakStartTime, fix xCloakEndTime)
{
	tPosition	*posP = OBJPOS (objP);
	tCloakInfo	ci;
	int			bOk = 0;

if (gameStates.render.bQueryCoronas)
	return 1;
GetCloakInfo (objP, xCloakStartTime, xCloakEndTime, &ci);
if (ci.bFading < 0) {
	fix xNewLight, xSaveGlow;
	tBitmapIndex * altTextures = NULL;

	if (objP->rType.polyObjInfo.nAltTextures > 0)
		altTextures = mpTextureIndex [objP->rType.polyObjInfo.nAltTextures - 1];
	xNewLight = FixMul (light, ci.xLightScale);
	xSaveGlow = glow [0];
	glow [0] = FixMul (glow [0], ci.xLightScale);
	gameData.models.nLightScale = ci.xLightScale;
	bOk = DrawPolygonModel (objP, &posP->vPos, &posP->mOrient,  
									(vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
									objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags, 
									xNewLight, glow, altTextures, NULL);
	gameData.models.nLightScale = 0;
	glow [0] = xSaveGlow;
	}
else {
	gameStates.render.bCloaked = 1;
	gameStates.render.grAlpha = (float) ci.nFadeValue;
	GrSetColorRGB (0, 0, 0, 255);	//set to black (matters for s3)
	G3SetSpecialRender (DrawTexPolyFlat, NULL, NULL);		//use special flat drawer
	bOk = DrawPolygonModel (objP, &posP->vPos, &posP->mOrient, 
									(vmsAngVec *)&objP->rType.polyObjInfo.animAngles, 
									objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags, 
									light, glow, NULL, NULL);
	G3SetSpecialRender (NULL, NULL, NULL);
	gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	gameStates.render.bCloaked = 0;
	}
return bOk;
}

//------------------------------------------------------------------------------

static inline int ObjectIsCloaked (tObject *objP)
{
if (gameStates.render.bBuildModels)
	return 0;
else if (objP->nType == OBJ_PLAYER)
	return (gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_CLOAKED) != 0;
else if (objP->nType == OBJ_ROBOT)
	return objP->cType.aiInfo.CLOAKED;
else
	return 0;
}

//------------------------------------------------------------------------------

int DrawHiresObject (tObject *objP, fix xLight, fix *xEngineGlow)
{
	float			fLight [3];
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
if (!(po = OOFModel (nModel)))
	return 0;
if (gameData.models.g3Models [1][nModel].bValid >= 0)
	return 0;
//G3RenderModel (objP, nModel, NULL, NULL, NULL, xLight, NULL, color);
fLight [0] = xLight / 65536.0f;
fLight [1] = (float) xEngineGlow [0] / 65536.0f;			
fLight [2] = (float) xEngineGlow [1] / 65536.0f;			
OOF_Render (objP, po, fLight, ObjectIsCloaked (objP));
return 1;
}

//------------------------------------------------------------------------------
//draw an tObject which renders as a polygon model
int DrawPolygonObject (tObject *objP, int bDepthSort, int bForce)
{
	fix	xLight;
	int	imSave = 0;
	fix	xEngineGlow [2];		//element 0 is for engine glow, 1 for headlight
	int	bBlendPolys = 0;
	int	bBrightPolys = 0;
	int	bCloaked = ObjectIsCloaked (objP);
	int	bEnergyWeapon;
	int	i, id, bOk = 0;

if (objP->nType == 255)
	return 0;
id = (int) objP->id;
if ((id < 0) || (id == 255))
	bEnergyWeapon = id = 0;
else 
	bEnergyWeapon = (objP->nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [id] && !gameData.objs.bIsMissile [id];
#if SHADOWS
if (!bForce && FAST_SHADOWS && 
	 !gameOpts->render.shadows.bSoft && 
	 (gameStates.render.nShadowPass == 3))
	return 1;
#endif
if (gameStates.render.bBuildModels) 
	xLight = F1_0;
else {
	xLight = CalcObjectLight (objP, xEngineGlow);
	if (bCloaked && bDepthSort && (gameStates.render.nShadowPass != 2)) {
		RIAddObject (objP);
		return 1;
		}
	if (DrawHiresObject (objP, xLight, xEngineGlow))
		return 1;
	gameStates.render.bBrightObject = bEnergyWeapon;
	gameOpts->render.bDepthSort = -gameOpts->render.bDepthSort;
	imSave = gameStates.render.nInterpolationMethod;
	if (bLinearTMapPolyObjs)
		gameStates.render.nInterpolationMethod = 1;
	}
if (objP->rType.polyObjInfo.nTexOverride != -1) {
#ifdef _DEBUG
	tPolyModel *pm = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
#endif
	tBitmapIndex	bm = gameData.pig.tex.bmIndex [0][objP->rType.polyObjInfo.nTexOverride], 
						bmiP [12];

#ifdef _DEBUG
	Assert (pm->nTextures <= 12);
#endif
	for (i = 0; i < 12; i++)		//fill whole array, in case simple model needs more
		bmiP [i] = bm;
	bOk = DrawPolygonModel (objP, &objP->position.vPos, 
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
	if (bCloaked) {
		if (objP->nType == OBJ_PLAYER) 
			bOk = DrawCloakedObject (objP, xLight, xEngineGlow, gameData.multiplayer.players [id].cloakTime, 
											 gameData.multiplayer.players [id].cloakTime + CLOAK_TIME_MAX);
		else if (objP->nType == OBJ_ROBOT) {
			if (!ROBOTINFO (id).bossFlag)
				bOk = DrawCloakedObject (objP, xLight, xEngineGlow, gameData.time.xGame - F1_0 * 10, gameData.time.xGame + F1_0 * 10);
			else if (0 <= (i = FindBoss (OBJ_IDX (objP))))
				bOk = DrawCloakedObject (objP, xLight, xEngineGlow, gameData.boss [i].nCloakStartTime, gameData.boss [i].nCloakEndTime);
			}
		}
	else {
		tBitmapIndex *bmiAltTex = (objP->rType.polyObjInfo.nAltTextures > 0) ? mpTextureIndex [objP->rType.polyObjInfo.nAltTextures - 1] : NULL;

		//	Snipers get bright when they fire.
		if (!gameStates.render.bBuildModels) {
			if (gameData.ai.localInfo [OBJ_IDX (objP)].nextPrimaryFire < F1_0 / 8) {
				if (objP->cType.aiInfo.behavior == AIB_SNIPE)
					xLight = 2 * xLight + F1_0;
				}
			bBlendPolys = bEnergyWeapon && (gameData.weapons.info [id].nInnerModel > -1);
			bBrightPolys = bBlendPolys && WI_energy_usage (id);
			if (bEnergyWeapon) {
				gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS - 2.0f;
				if (!gameOpts->legacy.bRender)
					OglBlendFunc (GL_ONE, GL_ONE);
				}
			if (bBlendPolys) {
				fix xDistToEye = VmVecDistQuick (&gameData.objs.viewer->position.vPos, &objP->position.vPos);
				if (xDistToEye < gameData.models.nSimpleModelThresholdScale * F1_0*2)
					bOk = DrawPolygonModel (
						objP, &objP->position.vPos, &objP->position.mOrient, 
						(vmsAngVec *) &objP->rType.polyObjInfo.animAngles, 
						gameData.weapons.info [id].nInnerModel, 
						objP->rType.polyObjInfo.nSubObjFlags, 
						bBrightPolys ? F1_0 : xLight, 
						xEngineGlow, 
						bmiAltTex, 
						NULL);
				}
			if (bEnergyWeapon)
				gameStates.render.grAlpha = 4 * (float) GR_ACTUAL_FADE_LEVELS / 5;
			else if (!bBlendPolys)
				gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
			}
		bOk = DrawPolygonModel (
			objP, &objP->position.vPos, &objP->position.mOrient, 
			objP->rType.polyObjInfo.animAngles, 
			objP->rType.polyObjInfo.nModel, 
			objP->rType.polyObjInfo.nSubObjFlags, 
			bBrightPolys ? F1_0 : xLight, 
			xEngineGlow, 
			bmiAltTex, 
			bEnergyWeapon ? gameData.weapons.color + id : NULL);
		if (!gameStates.render.bBuildModels) {
			if (!gameOpts->legacy.bRender) {
				gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
				OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				}
			gameStates.render.grAlpha = (float) GR_ACTUAL_FADE_LEVELS;
			}
		}
	}
if (!gameStates.render.bBuildModels) {
	gameStates.render.nInterpolationMethod = imSave;
	gameStates.render.bBrightObject = 0;
	gameOpts->render.bDepthSort = -gameOpts->render.bDepthSort;
	}
return bOk;
}

// -----------------------------------------------------------------------------

bool G3DrawSphere3D  (g3sPoint *p0, int nSides, int rad);

time_t tRenderObject;

int RenderObject (tObject *objP, int nWindowNum, int bForce)
{
	short			nObject = OBJ_IDX (objP);
	int			mldSave, bSpectate = 0, bDepthSort = gameOpts->render.nPath || (gameOpts->render.bDepthSort > 0);
	tPosition	savePos;
#if 0
	float			fLight [3];
	fix			nGlow [2];
	int			oofIdx;
#endif
	time_t		t = clock ();

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
if (objP->nType == 255) {
	KillObject (objP);
	return 0;
	}
if ((objP == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer]) && 
	 (objP->nSignature == gameData.objs.guidedMissileSig [gameData.multiplayer.nLocalPlayer]) &&
	 (gameStates.render.nShadowPass != 2)) {
	tRenderObject += clock () - t;
	return 0;
	}
if (nObject != LOCALPLAYER.nObject) {
	if (objP == gameData.objs.viewer)
		return 0;
	 }
else if ((gameData.objs.viewer == gameData.objs.console) && !gameStates.render.automap.bDisplay) {
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
		tRenderObject += clock () - t;
		return 0;	
		}
	}
if ((objP->nType == OBJ_NONE)/* || (objP->nType==OBJ_CAMBOT)*/){
#if TRACE			
	con_printf (1, "ERROR!!!Bogus obj %d in seg %d is rendering!\n", nObject, objP->nSegment);
#endif
	tRenderObject += clock () - t;
	return 0;
	}
mldSave = gameStates.render.detail.nMaxLinearDepth;
gameStates.render.nState = 1;
gameData.objs.color.index = 0;
gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;
if (objP->nType == OBJ_EXPLOSION)
	objP = objP;
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
		if (objP->nType == OBJ_EFFECT) {
			objP->renderType = (objP->id == SMOKE_ID) ? RT_SMOKE : RT_LIGHTNING;
			return 0;
			}
		if (gameStates.render.nType != 1)
			return 0;
		if (objP->nType == OBJ_PLAYER) {
			int bDynObjLight = (gameOpts->render.nPath && gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects;
			if (gameStates.render.automap.bDisplay && !(AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->id)))
				return 0;
			if (bSpectate) {
				savePos = objP->position;
				objP->position = gameStates.app.playerPos;
				}
			DoObjectSmoke (objP);
			DrawPolygonObject (objP, bDepthSort, 0);
			gameOpts->ogl.bLightObjects = bDynObjLight;
			RenderThrusterFlames (objP);
			RenderPlayerShield (objP);
			RenderTargetIndicator (objP, NULL);
			RenderTowedFlag (objP);
			if (bSpectate)
				objP->position = savePos;
			}
		else if (objP->nType == OBJ_ROBOT) {
			if (gameStates.render.nType != 1)
				return 0;
			if (gameStates.render.automap.bDisplay && !AM_SHOW_ROBOTS)
				return 0;
			DoObjectSmoke (objP);
			DrawPolygonObject (objP, bDepthSort, 0);
			RenderThrusterFlames (objP);
			if (gameStates.render.nShadowPass != 2) {
				RenderRobotShield (objP);
				RenderTargetIndicator (objP, NULL);
				SetRobotLocationInfo (objP);
				}
			}
		else if (objP->nType == OBJ_WEAPON) {
			if (gameStates.render.automap.bDisplay && !AM_SHOW_POWERUPS (1))
				return 0;
			if (!(gameStates.app.bNostalgia || gameOpts->render.powerups.b3D) && WeaponIsMine (objP->id) && (objP->id != SMALLMINE_ID))
				ConvertWeaponToVClip (objP);
			else {
				if (gameStates.render.nType != 1)
					return 0;
#if 1//def RELEASE
#endif
				if (gameData.objs.bIsMissile [objP->id]) {	//make missiles smaller during launch
					if ((objP->cType.laserInfo.parentType == OBJ_PLAYER) && 
						 (gameData.models.g3Models [1][108].bValid > 0)) {	//hires player ship
						float dt = f2fl (gameData.time.xGame - gameData.objs.xCreationTime [OBJ_IDX (objP)]);
		
						if (dt < 1)
							gameData.models.nScale = (fix) (F1_0 + F1_0 * dt * dt) / 2;
						}
					DoObjectSmoke (objP);
					DrawPolygonObject (objP, bDepthSort, 0);
#if RENDER_HITBOX
#	if 0
					DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#	else
					RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
					RenderThrusterFlames (objP);
					gameData.models.nScale = 0;
					}
				else {
#if RENDER_HITBOX
#	if 0
					DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#	else
					RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
					if (objP->nType != OBJ_WEAPON)
						DrawPolygonObject (objP, bDepthSort, 0);
					if (objP->id != SMALLMINE_ID)
						RenderLightTrail (objP);
					if (objP->nType == OBJ_WEAPON) {
						DoObjectSmoke (objP);
						DrawPolygonObject (objP, bDepthSort, 0);
						}
					}
				}
			}
		else if (objP->nType == OBJ_REACTOR) {
			if (gameStates.render.nType != 1)
				return 0;
			DoObjectSmoke (objP);
			DrawPolygonObject (objP, bDepthSort, 0);
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
				RenderPowerupCorona (objP, 1, 1, 1, coronaIntensities [gameOpts->render.nObjCoronaIntensity]);
				if (!DrawPolygonObject (objP, bDepthSort, 0))
					ConvertWeaponToPowerup (objP);
				else {
					objP->mType.physInfo.mass = F1_0;
					objP->mType.physInfo.drag = 512;
					if (gameOpts->render.powerups.nSpin != 
						((objP->mType.physInfo.rotVel.p.y | objP->mType.physInfo.rotVel.p.z) != 0))
						objP->mType.physInfo.rotVel.p.y = 
						objP->mType.physInfo.rotVel.p.z = gameOpts->render.powerups.nSpin ? F1_0 / (5 - gameOpts->render.powerups.nSpin) : 0;
					}
#ifdef _DEBUG
				RenderRobotShield (objP);
#endif
				gameData.models.nScale = 0;
				}
			else
				ConvertWeaponToPowerup (objP);
			}
		else if (objP->nType == OBJ_HOSTAGE) {
			if (gameStates.app.bNostalgia || !(gameOpts->render.powerups.b3D && DrawPolygonObject (objP, bDepthSort, 0)))
				ConvertModelToHostage (objP);
#ifdef _DEBUG
			RenderRobotShield (objP);
#endif
			}
		else {
#if 1//ndef _DEBUG
			DrawPolygonObject (objP, bDepthSort, 0);
#endif
			DrawDebrisCorona (objP);
			if (IsSpawnMarkerObject (objP))
				RenderMslLockIndicator (objP);
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
			if (objP->nType != OBJ_WEAPON) 
				DrawWeaponVClip (objP); 
			else {
				if (WeaponIsMine (objP->id)) {
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
#if 0//def _DEBUG
			if (EGI_FLAG (bPlayerShield, 0, 1, 0))
				DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#endif
			}
		break;

	case RT_HOSTAGE: 
		if (gameStates.render.nType != 1)
			return 0;
		if (ConvertHostageToModel (objP))
			DrawPolygonObject (objP, bDepthSort, 0);
		else if (gameStates.render.nShadowPass != 2)
			DrawHostage (objP); 
		break;

	case RT_POWERUP:
		if (gameStates.render.automap.bDisplay && !AM_SHOW_POWERUPS (1))
			return 0;
		if (gameStates.render.nType != 1)
			return 0;
		if (ConvertPowerupToWeapon (objP)) {
			RenderPowerupCorona (objP, 1, 1, 1, coronaIntensities [gameOpts->render.nObjCoronaIntensity]);
			DrawPolygonObject (objP, bDepthSort, 0);
			}
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
	
	case RT_SMOKE:
	case RT_LIGHTNING:
		break;

	default: 
		Error ("Unknown renderType <%d>", objP->renderType);
	}
//SetNearestStaticLights (objP->nSegment, 0);

#ifdef NEWDEMO
if (objP->renderType != RT_NONE)
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		if (!gameData.demo.bWasRecorded [nObject]) {
			NDRecordRenderObject (objP);
			gameData.demo.bWasRecorded [nObject] = 1;
		}
	}
#endif
gameStates.render.detail.nMaxLinearDepth = mldSave;
tRenderObject += clock () - t;
return 1;
}

//------------------------------------------------------------------------------
//eof
