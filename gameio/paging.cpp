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
#include "fuelcen.h"
#include "mission.h"
#include "menu.h"
#include "menu.h"
#include "loadobjects.h"
#include "gamepal.h"
#include "sphere.h"

//------------------------------------------------------------------------------

void LoadVClipTextures (tVideoClip* vc, int bD1)
{
for (int i = 0; i < vc->nFrameCount; i++)
	LoadTexture (vc->frames [i].index, bD1);
}

//------------------------------------------------------------------------------

void LoadWallEffectTextures (int nTexture)
{
	tEffectClip*	ecP = gameData.effects.effectP.Buffer ();

for (int i = gameData.effects.nEffects [gameStates.app.bD1Data]; i; i--, ecP++) {
	if (ecP->changingWallTexture == nTexture) {
		LoadVClipTextures (&ecP->vClipInfo, gameStates.app.bD1Data);
		if (ecP->nDestBm >= 0)
			LoadTexture (gameData.pig.tex.bmIndexP [ecP->nDestBm].index, gameStates.app.bD1Data);	//use this bitmap when monitor destroyed
		if (ecP->nDestVClip >= 0)
			LoadVClipTextures (&gameData.effects.vClipP [ecP->nDestVClip], gameStates.app.bD1Data);		  //what tVideoClip to play when exploding
		if (ecP->nDestEClip >= 0)
			LoadVClipTextures (&gameData.effects.effectP [ecP->nDestEClip].vClipInfo, gameStates.app.bD1Data); //what tEffectClip to play when exploding
		if (ecP->nCritClip >= 0)
			LoadVClipTextures (&gameData.effects.effectP [ecP->nCritClip].vClipInfo, gameStates.app.bD1Data); //what tEffectClip to play when mine critical
		}
	}
}

//------------------------------------------------------------------------------

void LoadObjectEffectTextures (int nTexture)
{
	tEffectClip *ecP = gameData.effects.effectP.Buffer ();

for (int i = gameData.effects.nEffects [gameStates.app.bD1Data]; i; i--, ecP++)
	if (ecP->changingObjectTexture == nTexture)
		LoadVClipTextures (&ecP->vClipInfo, 0);
}

//------------------------------------------------------------------------------

void LoadModelTextures (int nModel)
{
	CPolyModel*	modelP = gameData.models.polyModels [0] + nModel;
	ushort*		pi = gameData.pig.tex.objBmIndexP + modelP->FirstTexture ();
	int			j;

for (int i = modelP->TextureCount (); i; i--, pi++) {
	j = *pi;
	LoadTexture (gameData.pig.tex.objBmIndex [j].index, 0);
	LoadObjectEffectTextures (j);
	}
}

//------------------------------------------------------------------------------

void LoadWeaponTextures (int nWeaponType)
{
if ((nWeaponType < 0) || (nWeaponType > gameData.weapons.nTypes [0])) 
	return;
if (gameData.weapons.info [nWeaponType].picture.index)
	LoadTexture (gameData.weapons.info [nWeaponType].hiresPicture.index, 0);
if (gameData.weapons.info [nWeaponType].nFlashVClip >= 0)
	LoadVClipTextures (&gameData.effects.vClips [0][gameData.weapons.info [nWeaponType].nFlashVClip], 0);
if (gameData.weapons.info [nWeaponType].nWallHitVClip >= 0)
	LoadVClipTextures (&gameData.effects.vClips [0][gameData.weapons.info [nWeaponType].nWallHitVClip], 0);
if (WI_damage_radius (nWeaponType)) {
	// Robot_hit_vclips are actually badass_vclips
	if (gameData.weapons.info [nWeaponType].nRobotHitVClip >= 0)
		LoadVClipTextures (&gameData.effects.vClips [0][gameData.weapons.info [nWeaponType].nRobotHitVClip], 0);
	}
switch (gameData.weapons.info [nWeaponType].renderType) {
	case WEAPON_RENDER_VCLIP:
		if (gameData.weapons.info [nWeaponType].nVClipIndex >= 0)
			LoadVClipTextures (&gameData.effects.vClips [0][gameData.weapons.info [nWeaponType].nVClipIndex], 0);
		break;

	case WEAPON_RENDER_NONE:
		break;

	case WEAPON_RENDER_POLYMODEL:
		LoadModelTextures (gameData.weapons.info [nWeaponType].nModel);
		break;

	case WEAPON_RENDER_BLOB:
		LoadTexture (gameData.weapons.info [nWeaponType].bitmap.index, 0);
		break;
	}
}

//------------------------------------------------------------------------------

sbyte superBossGateTypeList [13] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22};

void LoadRobotTextures (int robotIndex)
{
	int i;

// Page in robotIndex
LoadModelTextures (ROBOTINFO (robotIndex).nModel);
if (ROBOTINFO (robotIndex).nExp1VClip >= 0)
	LoadVClipTextures (&gameData.effects.vClips [0][ROBOTINFO (robotIndex).nExp1VClip], 0);
if (ROBOTINFO (robotIndex).nExp2VClip >= 0)
	LoadVClipTextures (&gameData.effects.vClips [0][ROBOTINFO (robotIndex).nExp2VClip], 0);
// Page in his weapons
LoadWeaponTextures (ROBOTINFO (robotIndex).nWeaponType);
// A super-boss can gate in robots...
if (ROBOTINFO (robotIndex).bossFlag == 2) {
	for (i = 0; i < 13; i++)
		LoadRobotTextures (superBossGateTypeList [i]);
	LoadVClipTextures (&gameData.effects.vClips [0][VCLIP_MORPHING_ROBOT], 0);
	}
}

//------------------------------------------------------------------------------

void CObject::LoadTextures (void)
{
	int v;

switch (info.renderType) {
	case RT_NONE:
		break;		//doesn't render, like the player

	case RT_POLYOBJ:
		if (rType.polyObjInfo.nTexOverride == -1)
			LoadModelTextures (ModelId ());
		else
			LoadTexture (gameData.pig.tex.bmIndex [0][rType.polyObjInfo.nTexOverride].index, 0);
		break;

	case RT_POWERUP:
		if (PowerupToDevice ())
			LoadTextures ();
		else if ((info.nId >= MAX_POWERUP_TYPES_D2) && (rType.vClipInfo.nClipIndex >= gameData.effects.nClips [0]))
			rType.vClipInfo.nClipIndex = -MAX_ADDON_BITMAP_FILES - 1;
		break;

	case RT_MORPH:
	case RT_FIREBALL:
	case RT_THRUSTER:
	case RT_WEAPON_VCLIP: 
		break;

	case RT_HOSTAGE:
		LoadVClipTextures (gameData.effects.vClips [0] + rType.vClipInfo.nClipIndex, 0);
		break;

	case RT_LASER: 
		break;
 	}

switch (info.nType) {
	case OBJ_PLAYER:
		v = GetExplosionVClip (this, 0);
		if (v>= 0)
			LoadVClipTextures (gameData.effects.vClips [0] + v, 0);
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
	nDbgTexture = nDbgTexture;
#endif
LoadWallEffectTextures (m_nBaseTex);
if (m_nOvlTex) {
	LoadTexture (gameData.pig.tex.bmIndexP [m_nOvlTex].index, gameStates.app.bD1Data);
	LoadWallEffectTextures (m_nOvlTex);
	}
LoadTexture (gameData.pig.tex.bmIndexP [m_nBaseTex].index, gameStates.app.bD1Data);
}

//------------------------------------------------------------------------------

void CSegment::LoadSideTextures (int nSide)
{
#if DBG
if ((Index () == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	nDbgSeg = nDbgSeg;
#endif
if (!(IsDoorWay (nSide, NULL) & WID_VISIBLE_FLAG))
	return;
m_sides [nSide].LoadTextures ();
}

//------------------------------------------------------------------------------

void CSegment::LoadBotGenTextures (void)
{
	int		i;
	uint		flags;
	int		robotIndex;

if (m_function != SEGMENT_FUNC_ROBOTMAKER)
	return;
LoadVClipTextures (&gameData.effects.vClips [0][VCLIP_MORPHING_ROBOT], 0);
if (!gameData.matCens.botGens [m_nMatCen].objFlags)
	return;
for (i = 0; i < 2; i++, robotIndex += 32) {
	robotIndex = i * 32;
	for (flags = gameData.matCens.botGens [m_nMatCen].objFlags [i]; flags; flags >>= 1, robotIndex++)
		if (flags & 1)
			LoadRobotTextures (robotIndex);
	}
}

//------------------------------------------------------------------------------

void LoadObjectTextures (int nType)
{
	//int		i;
	CObject	*objP;

FORALL_OBJS (objP, i)
	if ((nType < 0) || (objP->info.nType == nType))
		objP->LoadTextures ();
}

//------------------------------------------------------------------------------

void CSegment::LoadTextures (void)
{
	short			nSide, nObject;

#if DBG
if (Index () == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
if (m_function == SEGMENT_FUNC_ROBOTMAKER)
	LoadBotGenTextures ();
for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) 
	m_sides [nSide].LoadTextures ();
for (nObject = m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg)
	OBJECTS [nObject].LoadTextures ();
}

//------------------------------------------------------------------------------

void CWall::LoadTextures (void)
{
if (nClip>= 0) {
	tWallClip* anim = gameData.walls.animP + nClip;
	for (int j = 0; j < anim->nFrameCount; j++)
		LoadTexture (gameData.pig.tex.bmIndexP [anim->frames [j]].index, gameStates.app.bD1Data);
	}
}

//------------------------------------------------------------------------------

void LoadWallTextures (void)
{
for (int i = 0; i < gameData.walls.nWalls; i++)
	WALLS [i].LoadTextures ();
}

//------------------------------------------------------------------------------

void LoadSegmentTextures (void)
{
for (int i = 0; i < gameData.segs.nSegments; i++)
	SEGMENTS [i].LoadTextures ();
}

//------------------------------------------------------------------------------

void LoadPowerupTextures (void)
{
for (int i = 0; i < gameData.objs.pwrUp.nTypes; i++)
	if (gameData.objs.pwrUp.info [i].nClipIndex>= 0)
		LoadVClipTextures (&gameData.effects.vClips [0][gameData.objs.pwrUp.info [i].nClipIndex], 0);
}

//------------------------------------------------------------------------------

void LoadWeaponTextures (void)
{
for (int i = 0; i < gameData.weapons.nTypes [0]; i++)
	LoadWeaponTextures (i);
}

//------------------------------------------------------------------------------

void LoadGaugeTextures (void)
{
for (int i = 0; i < MAX_GAUGE_BMS; i++)
	if (gameData.cockpit.gauges [1][i].index)
		LoadTexture (gameData.cockpit.gauges [1][i].index, 0);
}

//------------------------------------------------------------------------------

void PageInAddonBitmap (int bmi)
{
if ((bmi < 0) && (bmi >= -MAX_ADDON_BITMAP_FILES)) {
	PageInBitmap (&gameData.pig.tex.addonBitmaps [-bmi - 1], szAddonTextures [-bmi - 1], bmi, 0, true);
	}
}

//------------------------------------------------------------------------------

void LoadAddonTextures (void)
{
for (int i = 0; i < MAX_ADDON_BITMAP_FILES; i++)
	PageInAddonBitmap (-i - 1);
}

//------------------------------------------------------------------------------

void LoadAllTextures (void)
{
StopTime ();
#if 0
int bBlackScreen = paletteManager.EffectDisabled ();
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
LoadVClipTextures (&gameData.effects.vClips [0][VCLIP_PLAYER_APPEARANCE], 0);
LoadVClipTextures (&gameData.effects.vClips [0][VCLIP_POWERUP_DISAPPEARANCE], 0);
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

int PagingGaugeSize (void)
{
return PROGRESS_STEPS (gameData.segs.nSegments) + 
		 PROGRESS_STEPS (gameFileInfo.walls.count) +
		 PROGRESS_STEPS (gameData.objs.pwrUp.nTypes) * 2 +
		 PROGRESS_STEPS (gameData.weapons.nTypes [0]) + 
		 PROGRESS_STEPS (MAX_GAUGE_BMS);
}

//------------------------------------------------------------------------------

static int nTouchSeg = 0;
static int nTouchWall = 0;
static int nTouchWeapon = 0;
static int nTouchPowerup1 = 0;
static int nTouchPowerup2 = 0;
static int nTouchGauge = 0;

static int LoadTexturesPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int	i;

//paletteManager.ResumeEffect ();
if (nTouchSeg < gameData.segs.nSegments) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchSeg < gameData.segs.nSegments); i++) {
#if DBG
		if (nTouchSeg == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		SEGMENTS [nTouchSeg++].LoadTextures ();
		}
	}
else if (nTouchWall < gameData.walls.nWalls) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchWall < gameData.walls.nWalls); i++)
		WALLS [nTouchWall++].LoadTextures ();
	}
else if (nTouchPowerup1 < gameData.objs.pwrUp.nTypes) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchPowerup1 < gameData.objs.pwrUp.nTypes); i++, nTouchPowerup1++)
		if (gameData.objs.pwrUp.info [nTouchPowerup1].nClipIndex>= 0)
			LoadVClipTextures (&gameData.effects.vClips [0][gameData.objs.pwrUp.info [nTouchPowerup1].nClipIndex], 0);
	}
else if (nTouchWeapon < gameData.weapons.nTypes [0]) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchWeapon < gameData.weapons.nTypes [0]); i++)
		LoadWeaponTextures (nTouchWeapon++);
	}
else if (nTouchPowerup2 < gameData.objs.pwrUp.nTypes) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchPowerup2 < gameData.objs.pwrUp.nTypes); i++, nTouchPowerup2++)
		if (gameData.objs.pwrUp.info [nTouchPowerup2].nClipIndex>= 0)
			LoadVClipTextures (&gameData.effects.vClips [0][gameData.objs.pwrUp.info [nTouchPowerup2].nClipIndex], 0);
	}
else if (nTouchGauge < MAX_GAUGE_BMS) {
	for (i = 0; (i < PROGRESS_INCR) && (nTouchGauge < MAX_GAUGE_BMS); i++, nTouchGauge++)
		if (gameData.cockpit.gauges [1][nTouchGauge].index)
			LoadTexture (gameData.cockpit.gauges [1][nTouchGauge].index, 0);
	}
else {
	LoadVClipTextures (&gameData.effects.vClips [0][VCLIP_PLAYER_APPEARANCE], 0);
	LoadVClipTextures (&gameData.effects.vClips [0][VCLIP_POWERUP_DISAPPEARANCE], 0);
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

int nBitmaps = 0;

void LoadLevelTextures (void)
{
nBitmaps = 0;
if (gameStates.app.bProgressBars && gameOpts->menus.nStyle) {
		int	i = LoadMineGaugeSize ();

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
