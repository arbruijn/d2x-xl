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

//#define PSX_BUILD_TOOLS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"
#include "descent.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "objrender.h"
#include "loadgeometry.h"
#include "error.h"
#include "segmath.h"
#include "game.h"
#include "piggy.h"
#include "texmerge.h"
#include "polymodel.h"
#include "vclip.h"
#include "effects.h"
#include "fireball.h"
#include "weapon.h"
#include "palette.h"
#include "timer.h"
#include "text.h"
#include "reactor.h"
#include "cockpit.h"
#include "powerup.h"
#include "producers.h"
#include "mission.h"
#include "menu.h"
#include "menu.h"
#include "loadobjects.h"
#include "gamepal.h"
#include "sphere.h"

//------------------------------------------------------------------------------

void LoadAnimationTextures (tAnimationInfo* vc, int32_t bD1)
{
for (int32_t i = 0; i < vc->nFrameCount; i++)
	LoadTexture (vc->frames [i].index, i, bD1);
}

//------------------------------------------------------------------------------

void LoadWallEffectTextures (int32_t nTexture)
{
	tEffectInfo*	effectInfoP = gameData.effects.effectP.Buffer ();

for (int32_t i = gameData.effects.nEffects [gameStates.app.bD1Data]; i; i--, effectInfoP++) {
	if (effectInfoP->changing.nWallTexture == nTexture) {
		LoadAnimationTextures (&effectInfoP->animationInfo, gameStates.app.bD1Data);
		if (effectInfoP->destroyed.nTexture >= 0)
			LoadTexture (gameData.pig.tex.bmIndexP [effectInfoP->destroyed.nTexture].index, 0, gameStates.app.bD1Data);	//use this bitmap when monitor destroyed
		if (effectInfoP->destroyed.nAnimation >= 0)
			LoadAnimationTextures (&gameData.effects.vClipP [effectInfoP->destroyed.nAnimation], gameStates.app.bD1Data);		  //what animation to play when exploding
		if (effectInfoP->destroyed.nEffect >= 0)
			LoadAnimationTextures (&gameData.effects.effectP [effectInfoP->destroyed.nEffect].animationInfo, gameStates.app.bD1Data); //what effect to play when exploding
		if (effectInfoP->nCriticalAnimation >= 0)
			LoadAnimationTextures (&gameData.effects.effectP [effectInfoP->nCriticalAnimation].animationInfo, gameStates.app.bD1Data); //what effect to play when mine critical
		}
	}
}

//------------------------------------------------------------------------------

void LoadObjectEffectTextures (int32_t nTexture)
{
	tEffectInfo *effectInfoP = gameData.effects.effectP.Buffer ();

for (int32_t i = gameData.effects.nEffects [gameStates.app.bD1Data]; i; i--, effectInfoP++)
	if (effectInfoP->changing.nObjectTexture == nTexture)
		LoadAnimationTextures (&effectInfoP->animationInfo, 0);
}

//------------------------------------------------------------------------------

void LoadModelTextures (int32_t nModel)
{
	CPolyModel*	modelP = gameData.models.polyModels [0] + nModel;
	uint16_t*		pi = gameData.pig.tex.objBmIndexP + modelP->FirstTexture ();
	int32_t			j;

for (int32_t h = modelP->TextureCount (), i = 0; i < h; i++, pi++) {
	j = *pi;
	LoadTexture (gameData.pig.tex.objBmIndex [j].index, 0 /*i*/, 0); // don't pass frame number to avoid checking for hires replacement textures
	LoadObjectEffectTextures (j);
	}
}

//------------------------------------------------------------------------------

void LoadWeaponTextures (int32_t nWeaponType)
{
if ((nWeaponType < 0) || (nWeaponType > gameData.weapons.nTypes [0])) 
	return;
if (gameData.weapons.info [nWeaponType].picture.index)
	LoadTexture (gameData.weapons.info [nWeaponType].hiresPicture.index, 0, 0);
if (gameData.weapons.info [nWeaponType].nFlashAnimation >= 0)
	LoadAnimationTextures (&gameData.effects.animations [0][gameData.weapons.info [nWeaponType].nFlashAnimation], 0);
if (gameData.weapons.info [nWeaponType].nWallHitAnimation >= 0)
	LoadAnimationTextures (&gameData.effects.animations [0][gameData.weapons.info [nWeaponType].nWallHitAnimation], 0);
if (WI_damage_radius (nWeaponType)) {
	// Robot_hit_vclips are actually badass_vclips
	if (gameData.weapons.info [nWeaponType].nRobotHitAnimation >= 0)
		LoadAnimationTextures (&gameData.effects.animations [0][gameData.weapons.info [nWeaponType].nRobotHitAnimation], 0);
	}
switch (gameData.weapons.info [nWeaponType].renderType) {
	case WEAPON_RENDER_VCLIP:
		if (gameData.weapons.info [nWeaponType].nAnimationIndex >= 0)
			LoadAnimationTextures (&gameData.effects.animations [0][gameData.weapons.info [nWeaponType].nAnimationIndex], 0);
		break;

	case WEAPON_RENDER_NONE:
		break;

	case WEAPON_RENDER_POLYMODEL:
		LoadModelTextures (gameData.weapons.info [nWeaponType].nModel);
		break;

	case WEAPON_RENDER_BLOB:
		LoadTexture (gameData.weapons.info [nWeaponType].bitmap.index, 0, 0);
		break;
	}
}

//------------------------------------------------------------------------------

int8_t superBossGateTypeList [13] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22};

void LoadRobotTextures (int32_t robotIndex)
{
	int32_t i;

// Page in robotIndex
LoadModelTextures (ROBOTINFO (robotIndex).nModel);
if (ROBOTINFO (robotIndex).nExp1VClip >= 0)
	LoadAnimationTextures (&gameData.effects.animations [0][ROBOTINFO (robotIndex).nExp1VClip], 0);
if (ROBOTINFO (robotIndex).nExp2VClip >= 0)
	LoadAnimationTextures (&gameData.effects.animations [0][ROBOTINFO (robotIndex).nExp2VClip], 0);
// Page in his weapons
LoadWeaponTextures (ROBOTINFO (robotIndex).nWeaponType);
// A super-boss can gate in robots...
if (ROBOTINFO (robotIndex).bossFlag == 2) {
	for (i = 0; i < 13; i++)
		LoadRobotTextures (superBossGateTypeList [i]);
	LoadAnimationTextures (&gameData.effects.animations [0][ANIM_MORPHING_ROBOT], 0);
	}
}

//------------------------------------------------------------------------------

void CObject::LoadTextures (void)
{
	int32_t v;

switch (info.renderType) {
	case RT_NONE:
		break;		//doesn't render, like the player

	case RT_POLYOBJ:
		if (rType.polyObjInfo.nTexOverride == -1)
			LoadModelTextures (ModelId ());
		else
			LoadTexture (gameData.pig.tex.bmIndex [0][rType.polyObjInfo.nTexOverride].index, 0, 0);
		break;

	case RT_POWERUP:
		if (PowerupToDevice ())
			LoadTextures ();
		else if ((info.nId >= MAX_POWERUP_TYPES_D2) && (rType.animationInfo.nClipIndex >= gameData.effects.nClips [0]))
			rType.animationInfo.nClipIndex = -MAX_ADDON_BITMAP_FILES - 1;
		break;

	case RT_MORPH:
	case RT_FIREBALL:
	case RT_THRUSTER:
	case RT_WEAPON_VCLIP: 
		break;

	case RT_HOSTAGE:
		LoadAnimationTextures (gameData.effects.animations [0] + rType.animationInfo.nClipIndex, 0);
		break;

	case RT_LASER: 
		break;
 	}

switch (info.nType) {
	case OBJ_PLAYER:
		v = GetExplosionVClip (this, 0);
		if (v>= 0)
			LoadAnimationTextures (gameData.effects.animations [0] + v, 0);
		break;

	case OBJ_ROBOT:
		LoadRobotTextures (info.nId);
		break;

	case OBJ_REACTOR:
		LoadWeaponTextures (CONTROLCEN_WEAPON_NUM);
		if (gameData.models.nDeadModels [ModelId ()] != -1)
			LoadModelTextures (gameData.models.nDeadModels [ModelId ()]);
		break;
	}
}

//------------------------------------------------------------------------------

void CSide::LoadTextures (void)
{
#if DBG
if (m_nBaseTex == nDbgTexture)
	BRP;
#endif
if (!FaceCount ())
	return;
LoadWallEffectTextures (m_nBaseTex);
if (m_nOvlTex) {
	LoadTexture (gameData.pig.tex.bmIndexP [m_nOvlTex].index, 0, gameStates.app.bD1Data);
	LoadWallEffectTextures (m_nOvlTex);
	}
LoadTexture (gameData.pig.tex.bmIndexP [m_nBaseTex].index, 0, gameStates.app.bD1Data);
}

//------------------------------------------------------------------------------

void CSegment::LoadSideTextures (int32_t nSide)
{
#if DBG
if ((Index () == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	BRP;
#endif
if (!(IsPassable (nSide, NULL) & WID_VISIBLE_FLAG))
	return;
m_sides [nSide].LoadTextures ();
}

//------------------------------------------------------------------------------

void CSegment::LoadBotGenTextures (void)
{
	int32_t		i;
	uint32_t		flags;
	int32_t		robotIndex;

if (m_function != SEGMENT_FUNC_ROBOTMAKER)
	return;
LoadAnimationTextures (&gameData.effects.animations [0][ANIM_MORPHING_ROBOT], 0);
if (!gameData.producers.robotMakers [m_nObjProducer].objFlags)
	return;
for (i = 0; i < 2; i++, robotIndex += 32) {
	robotIndex = i * 32;
	for (flags = gameData.producers.robotMakers [m_nObjProducer].objFlags [i]; flags; flags >>= 1, robotIndex++)
		if (flags & 1)
			LoadRobotTextures (robotIndex);
	}
}

//------------------------------------------------------------------------------

void LoadObjectTextures (int32_t nType)
{
	CObject*	objP;

FORALL_OBJS (objP)
	if ((nType < 0) || (objP->info.nType == nType))
		objP->LoadTextures ();
}

//------------------------------------------------------------------------------

void CSegment::LoadTextures (void)
{
	int16_t			nSide, nObject;

#if DBG
if (Index () == nDbgSeg)
	BRP;
#endif
if (m_function == SEGMENT_FUNC_ROBOTMAKER)
	LoadBotGenTextures ();
for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) 
	m_sides [nSide].LoadTextures ();
for (nObject = m_objects; nObject != -1; nObject = gameData.Object (nObject)->info.nNextInSeg)
	gameData.Object (nObject)->LoadTextures ();
}

//------------------------------------------------------------------------------

void CWall::LoadTextures (void)
{
if (nClip>= 0) {
	tWallEffect* anim = gameData.walls.animP + nClip;
	for (int32_t j = 0; j < anim->nFrameCount; j++)
		LoadTexture (gameData.pig.tex.bmIndexP [anim->frames [j]].index, j, gameStates.app.bD1Data);
	}
}

//------------------------------------------------------------------------------

void LoadWallTextures (void)
{
for (int32_t i = 0; i < gameData.walls.nWalls; i++)
	gameData.Wall (i)->LoadTextures ();
}

//------------------------------------------------------------------------------

void LoadSegmentTextures (void)
{
for (int32_t i = 0; i < gameData.segs.nSegments; i++)
	gameData.Segment (i)->LoadTextures ();
}

//------------------------------------------------------------------------------

void LoadPowerupTextures (void)
{
for (int32_t i = 0; i < gameData.objs.pwrUp.nTypes; i++)
	if (gameData.objs.pwrUp.info [i].nClipIndex >= 0)
		LoadAnimationTextures (&gameData.effects.animations [0][gameData.objs.pwrUp.info [i].nClipIndex], 0);
}

//------------------------------------------------------------------------------

void LoadWeaponTextures (void)
{
for (int32_t i = 0; i < gameData.weapons.nTypes [0]; i++)
	LoadWeaponTextures (i);
}

//------------------------------------------------------------------------------

void LoadGaugeTextures (void)
{
for (int32_t i = 0; i < MAX_GAUGE_BMS; i++)
	if (gameData.cockpit.gauges [1][i].index)
		LoadTexture (gameData.cockpit.gauges [1][i].index, 0, 0);
}

//------------------------------------------------------------------------------

void PageInAddonBitmap (int32_t bmi)
{
if ((bmi < 0) && (bmi >= -MAX_ADDON_BITMAP_FILES)) {
	PageInBitmap (&gameData.pig.tex.addonBitmaps [-bmi - 1], szAddonTextures [-bmi - 1], bmi, 0, true);
	}
}

//------------------------------------------------------------------------------

void LoadAddonTextures (void)
{
for (int32_t i = 0; i < MAX_ADDON_BITMAP_FILES; i++)
	PageInAddonBitmap (-i - 1);
}

//------------------------------------------------------------------------------

void LoadAllTextures (void)
{
StopTime ();
#if 0
int32_t bBlackScreen = paletteManager.EffectDisabled ();
if (paletteManager.EffectDisabled ()) {
	CCanvas::Current ()->Clear (BLACK_RGBA);
	//paletteManager.ResumeEffect ();
	}
#endif
LoadSegmentTextures ();
LoadWallTextures ();
LoadPowerupTextures ();
LoadWeaponTextures ();
LoadPowerupTextures ();
LoadGaugeTextures ();
LoadAnimationTextures (&gameData.effects.animations [0][ANIM_PLAYER_APPEARANCE], 0);
LoadAnimationTextures (&gameData.effects.animations [0][ANIM_POWERUP_DISAPPEARANCE], 0);
LoadAddonTextures ();
#if 0
if (bBlackScreen) {
	paletteManager.ClearEffect ();
	CCanvas::Current ()->Clear (BLACK_RGBA);
	}
#endif
StartTime (0);
}

//------------------------------------------------------------------------------

int32_t PagingGaugeSize (void)
{
return PROGRESS_STEPS (gameData.segs.nSegments) + 
		 PROGRESS_STEPS (gameFileInfo.walls.count) +
		 PROGRESS_STEPS (gameData.objs.pwrUp.nTypes) * 2 +
		 PROGRESS_STEPS (gameData.weapons.nTypes [0]) + 
		 PROGRESS_STEPS (MAX_GAUGE_BMS);
}

//------------------------------------------------------------------------------

static int32_t nTouchSeg = 0;
static int32_t nTouchWall = 0;
static int32_t nTouchWeapon = 0;
static int32_t nTouchPowerup1 = 0;
static int32_t nTouchPowerup2 = 0;
static int32_t nTouchGauge = 0;

static int32_t LoadTexturesPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	int32_t	i;

//paletteManager.ResumeEffect ();
if (nTouchSeg < gameData.segs.nSegments) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchSeg < gameData.segs.nSegments); i++) {
#if DBG
		if (nTouchSeg == nDbgSeg)
			BRP;
#endif
		gameData.Segment (nTouchSeg++)->LoadTextures ();
		}
	}
else if (nTouchWall < gameData.walls.nWalls) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchWall < gameData.walls.nWalls); i++)
		gameData.Wall (nTouchWall++)->LoadTextures ();
	}
else if (nTouchPowerup1 < gameData.objs.pwrUp.nTypes) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchPowerup1 < gameData.objs.pwrUp.nTypes); i++, nTouchPowerup1++)
		if (gameData.objs.pwrUp.info [nTouchPowerup1].nClipIndex>= 0)
			LoadAnimationTextures (&gameData.effects.animations [0][gameData.objs.pwrUp.info [nTouchPowerup1].nClipIndex], 0);
	}
else if (nTouchWeapon < gameData.weapons.nTypes [0]) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchWeapon < gameData.weapons.nTypes [0]); i++)
		LoadWeaponTextures (nTouchWeapon++);
	}
else if (nTouchPowerup2 < gameData.objs.pwrUp.nTypes) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchPowerup2 < gameData.objs.pwrUp.nTypes); i++, nTouchPowerup2++)
		if (gameData.objs.pwrUp.info [nTouchPowerup2].nClipIndex>= 0)
			LoadAnimationTextures (&gameData.effects.animations [0][gameData.objs.pwrUp.info [nTouchPowerup2].nClipIndex], 0);
	}
else if (nTouchGauge < MAX_GAUGE_BMS) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchGauge < MAX_GAUGE_BMS); i++, nTouchGauge++)
		if (gameData.cockpit.gauges [1][nTouchGauge].index)
			LoadTexture (gameData.cockpit.gauges [1][nTouchGauge].index, 0, 0);
	}
else {
	LoadAnimationTextures (&gameData.effects.animations [0][ANIM_PLAYER_APPEARANCE], 0);
	LoadAnimationTextures (&gameData.effects.animations [0][ANIM_POWERUP_DISAPPEARANCE], 0);
	LoadAddonTextures ();
	key = -2;
	//paletteManager.ResumeEffect ();
	return nCurItem;
	}
menu [0].Value ()++;
menu [0].Rebuild ();
key = 0;
//paletteManager.ResumeEffect ();
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t nBitmaps = 0;

void LoadLevelTextures (void)
{
nBitmaps = 0;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
		int32_t	i = LoadMineGaugeSize ();

	nTouchSeg = 0;
	nTouchWall = 0;
	nTouchWeapon = 0;
	nTouchPowerup1 = 0;
	nTouchPowerup2 = 0;
	nTouchGauge = 0;
	ProgressBar (TXT_LOADING, i, i + PagingGaugeSize () + SortLightsGaugeSize () + SegDistGaugeSize (), LoadTexturesPoll); 
	}
else
	LoadAllTextures ();
}

//------------------------------------------------------------------------------
