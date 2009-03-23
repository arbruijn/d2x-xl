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

#include "descent.h"
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
#include "rendermine.h"
#include "transprender.h"
#include "glare.h"
#include "sphere.h"
#include "objsmoke.h"
#include "fireball.h"
#include "objrender.h"
#include "objeffects.h"
#include "shrapnel.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "marker.h"
#include "hiresmodels.h"

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

short WeaponModel (CObject *objP)
{
	short	nModel;

if ((nModel = WeaponToModel (objP->info.nId)))
	return nModel;
return objP->rType.polyObjInfo.nModel;
}

//------------------------------------------------------------------------------

int InitAddonPowerup (CObject *objP)
{
if (objP->info.nId == POW_SLOWMOTION)
	objP->rType.vClipInfo.nClipIndex = -1;
else if (objP->info.nId == POW_BULLETTIME)
	objP->rType.vClipInfo.nClipIndex = -2;
else
	return 0;
objP->rType.vClipInfo.nCurFrame = 0;
objP->rType.vClipInfo.xTotalTime = 0;
return 1;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToPowerup (CObject *objP)
{
if (!InitAddonPowerup (objP)) {
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.vClipP [objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->info.xSize = gameData.objs.pwrUp.info [objP->info.nId].size;
	}
objP->info.controlType = CT_POWERUP;
objP->info.renderType = RT_POWERUP;
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

int ConvertHostageToModel (CObject *objP)
{
if (objP->info.renderType == RT_POLYOBJ)
	return 1;
if (gameStates.app.bNostalgia || !gameOpts->render.powerups.b3D)
	return 0;
if (!HaveReplacementModel (HOSTAGE_MODEL))
	return 0;
objP->info.renderType = RT_POLYOBJ;
objP->rType.polyObjInfo.nModel = HOSTAGE_MODEL;
objP->rType.polyObjInfo.nTexOverride = -1;
objP->mType.physInfo.rotVel.SetZero ();
memset (objP->rType.polyObjInfo.animAngles, 0, sizeof (objP->rType.polyObjInfo.animAngles));
return 1;
}

// -----------------------------------------------------------------------------

int ConvertModelToHostage (CObject *objP)
{
objP->rType.vClipInfo.nClipIndex = nHostageVClips [0];
objP->rType.vClipInfo.xFrameTime = gameData.eff.vClipP [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
objP->info.xSize = 289845;
objP->info.controlType = CT_POWERUP;
objP->info.renderType = RT_HOSTAGE;
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.drag = 512;
return 1;
}

// -----------------------------------------------------------------------------

int CObject::PowerupToWeapon (void)
{
	CAngleVector	a;
	short				nModel, nId;
	int				bHasModel = 0;

if (!SHOW_OBJ_FX)
	return 0;
if (!gameOpts->render.powerups.b3D)
	return 0;
if (info.controlType == CT_WEAPON)
	return 1;
if ((info.nType != OBJ_POWERUP) && (info.nType != OBJ_WEAPON))
	return 0;
nModel = PowerupToModel (info.nId);
if (nModel)
	nId = info.nId;
else {
	nId = PowerupToObject (info.nId);
	if (nId >= 0) {
		nModel = gameData.weapons.info [nId].nModel;
		bHasModel = 1;
		}
	}
if (!bHasModel && ((info.nType != OBJ_WEAPON) || !gameData.objs.bIsMissile [info.nId]) &&
	 !(nModel && HaveReplacementModel (nModel)))
		return 0;

if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	a[PA] = (rand () % I2X (1)) - I2X (1) / 2;
	a[BA] = (rand () % I2X (1)) - I2X (1) / 2;
	a[HA] = (rand () % I2X (1)) - I2X (1) / 2;
	info.position.mOrient = CFixMatrix::Create(a);
	}
mType.physInfo.mass = I2X (1);
mType.physInfo.drag = 512;
mType.physInfo.brakes = 0;
#if 0
if ((info.nType == OBJ_WEAPON) && gameData.objs.bIsMissile [info.nId])
#endif
 {
	mType.physInfo.rotVel[X] = 0;
	mType.physInfo.rotVel[Y] =
	mType.physInfo.rotVel[Z] = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
	}
#if 0
else {
	mType.physInfo.rotVel[X] =
	mType.physInfo.rotVel[Z] = 0;
	mType.physInfo.rotVel[Y] = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
	}
#endif
info.controlType = CT_WEAPON;
info.renderType = RT_POLYOBJ;
info.movementType = MT_PHYSICS;
mType.physInfo.flags = PF_BOUNCE | PF_FREE_SPINNING;
rType.polyObjInfo.nModel = nModel;
#if 1
if (bHasModel)
	info.xSize = gameData.models.polyModels [0][rType.polyObjInfo.nModel].Rad ();
#endif
rType.polyObjInfo.nTexOverride = -1;
info.xLifeLeft = IMMORTAL_TIME;
return 1;
}

// -----------------------------------------------------------------------------

void ConvertAllPowerupsToWeapons (void)
{
	CObject	*objP;
	//int		i;

FORALL_OBJS (objP, i)
	if (objP->info.renderType == RT_POWERUP) {
		objP->PowerupToWeapon ();
		objP->LoadTextures ();
		}
}

// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the CPlayerData had fired, "warns" the robot
void SetRobotLocationInfo (CObject *objP)
{
if (gameStates.app.bPlayerFiredLaserThisFrame != -1) {
	g3sPoint temp;

	G3TransformAndEncodePoint(&temp, objP->info.position.vPos);
	if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
		return;
	//the code below to check for CObject near the center of the screen
	//completely ignores z, which may not be good
	if ((abs (temp.p3_vec[X]) < I2X (4)) && (abs (temp.p3_vec[Y]) < I2X (4))) {
		objP->cType.aiInfo.nDangerLaser = gameStates.app.bPlayerFiredLaserThisFrame;
		objP->cType.aiInfo.nDangerLaserSig = OBJECTS [gameStates.app.bPlayerFiredLaserThisFrame].info.nSignature;
		}
	}
}

//------------------------------------------------------------------------------

fix CalcObjectLight (CObject *objP, fix *xEngineGlow)
{
	fix xLight;

if (gameStates.render.bBuildModels)
	return I2X (1);
if (IsMultiGame && netGame.BrightPlayers && (objP->info.nType == OBJ_PLAYER)) {
	xLight = I2X (1); //	If option set for bright players in netgame, brighten them
	gameOpts->ogl.bLightObjects = 0;
	}
else
	xLight = ComputeObjectLight (objP, NULL);
//make robots brighter according to robot glow field
if (objP->info.nType == OBJ_ROBOT)
	xLight += (ROBOTINFO (objP->info.nId).glow << 12);		//convert 4:4 to 16:16
else if (objP->info.nType == OBJ_WEAPON) {
	if (objP->info.nId == FLARE_ID)
		xLight += I2X (2);
	}
else if (objP->info.nType == OBJ_MARKER)
 	xLight += I2X (2);
ComputeEngineGlow (objP, xEngineGlow);
return xLight;
}

//------------------------------------------------------------------------------

static float ObjectBlobColor (CObject *objP, CBitmap *bmP, tRgbaColorf *colorP)
{
	float	fScale;

bmP->GetAvgColor (colorP);
#if DBG
if ((objP->info.nType == nDbgObjType) && ((nDbgObjId < 0) || (objP->info.nId == nDbgObjId)))
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
//draw an CObject that has one bitmap & doesn't rotate

void DrawObjectBlob (CObject *objP, int bmi0, int bmi, int iFrame, tRgbaColorf *colorP, float fAlpha)
{
	CBitmap*		bmP, * bmoP;
	tRgbaColorf	color;
	int			nType = objP->info.nType;
	int			nId = objP->info.nId;
#if 0
	int			bMuzzleFlash = 0;
#endif
	int			bAdditive = 0, bEnergy = 0, nTransp = (nType == OBJ_POWERUP) ? 3 : 2;
	fix			xSize;
	float			fScale;

if ((nType == OBJ_WEAPON) && (objP->info.nId == OMEGA_ID) && omegaLightnings.Exist ())
	return;
#if DBG
if ((nType == nDbgObjType) && ((nDbgObjId < 0) || (objP->info.nId == nDbgObjId)))
	nDbgObjType = nDbgObjType;
#endif
if (gameOpts->render.textures.bUseHires [0] || gameOpts->render.effects.bTransparent) {
	if (fAlpha) {
		bAdditive = (nType == OBJ_FIREBALL) || (nType == OBJ_EXPLOSION) || ((nType == OBJ_WEAPON) && (objP->info.nId == OMEGA_ID));
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

#if 0
if (bmi < 0) {
	PageInAddonBitmap (bmi);
	bmP = gameData.pig.tex.addonBitmaps - bmi - 1;
#if DBG
	if ((objP->rType.vClipInfo.nCurFrame < 0) || (objP->rType.vClipInfo.nCurFrame >= bmP->FrameCount ())) {
		objP->rType.vClipInfo.nCurFrame = 0;
		return;
		}
#endif
	}
else {
	LoadBitmap (bmi, 0);
	bmP = gameData.pig.tex.bitmaps [0] + bmi;
	}
#else
if (bmi < 0)
	bmP = gameData.pig.tex.addonBitmaps - bmi - 1;
else
	bmP = gameData.pig.tex.bitmaps [0] + bmi;
#endif

#if DBG
CBitmap* bmP2 = bmP;
#endif
//bmP->SetupTexture (1, 1);
if ((bmi < 0) || ((bmP->Type () == BM_TYPE_STD) && (bmoP = bmP->Override ()))) {
	//fScale = ObjectBlobColor (objP, bmP, &color);
	bmP = bmoP->SetCurFrame (iFrame);
	//fAlpha = 1;
	}
if (!bmP)
	return;
#if 0
if (!bmP->Prepared () && bmP->PrepareTexture (1, 0))
	return;
#endif
fScale = ObjectBlobColor (objP, bmP, &color);
if (colorP /*&& (bmi >= 0)*/)
	*colorP = color;
	//memcpy (colorP, gameData.pig.tex.bitmapColors + bmi, sizeof (tRgbaColorf));

xSize = objP->info.xSize;

if (nType == OBJ_POWERUP) {
	if ((bEnergy && gameOpts->render.coronas.bPowerups) || (!bEnergy && gameOpts->render.coronas.bWeapons))
		RenderPowerupCorona (objP, color.red, color.green, color.blue,
									coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	}
if ((objP->info.nType == OBJ_POWERUP) && (objP->info.nId == POW_SHIELD_BOOST) && 
	 !gameStates.app.bNostalgia && gameOpts->render.powerups.b3D && gameOpts->render.powerups.b3DShields) {
	if (objP->info.movementType != MT_SPINNING) {
		objP->info.movementType = MT_SPINNING;
		objP->mType.spinRate = objP->info.position.mOrient.UVec () * (I2X (1) / 8);
		}
	//the actual shield in the sprite texture has 3/4 of the textures size
	DrawShieldSphere (objP, 3 * color.red / 2, 3 * color.green / 2, 3 * color.blue / 2, 1.0f, 3 * objP->info.xSize / 4);	
	}
else if ((gameOpts->render.bDepthSort > 0) && (fAlpha < 1)) {
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
	if (bmP->Width () > bmP->Height ())
		transparencyRenderer.AddSprite (bmP, objP->info.position.vPos, &color, xSize, FixMulDiv (xSize, bmP->Height (), bmP->Width ()), 
												  iFrame, bAdditive, (nType == OBJ_FIREBALL) ? 10.0f : 0.0f);
	else
		transparencyRenderer.AddSprite (bmP, objP->info.position.vPos, &color, FixMulDiv (xSize, bmP->Width (), bmP->Height ()), xSize, 
												  iFrame, bAdditive, (nType == OBJ_FIREBALL) ? 10.0f : 0.0f);
	}
else {
	if (bmP->Width () > bmP->Height ())
		G3DrawBitmap (objP->info.position.vPos, xSize, FixMulDiv (xSize, bmP->Height (), bmP->Width ()), bmP, NULL, fAlpha, nTransp);
	else
		G3DrawBitmap (objP->info.position.vPos, FixMulDiv (xSize, bmP->Width (), bmP->Height ()), xSize, bmP, NULL, fAlpha, nTransp);
	}
gameData.render.nTotalSprites++;
}

//------------------------------------------------------------------------------
//draw an CObject that is a texture-mapped rod
void DrawObjectRodTexPoly (CObject *objP, tBitmapIndex bmi, int bLit, int iFrame)
{
	CBitmap *bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
	fix light;
	CFixVector delta, top_v, bot_v;
	g3sPoint top_p, bot_p;

LoadBitmap (bmi.index, 0);
if ((bmP->Type () == BM_TYPE_STD) && bmP->Override ()) {
	bmP->SetupTexture (1, gameOpts->render.bDepthSort <= 0);
	bmP = bmP->Override (iFrame);
	}
delta = objP->info.position.mOrient.UVec () * objP->info.xSize;
top_v = objP->info.position.vPos + delta;
bot_v = objP->info.position.vPos - delta;
G3TransformAndEncodePoint (&top_p, top_v);
G3TransformAndEncodePoint (&bot_p, bot_v);
if (bLit)
	light = ComputeObjectLight (objP, &top_p.p3_vec);
else
	light = I2X (1);
G3DrawRodTexPoly (bmP, &bot_p, objP->info.xSize, &top_p, objP->info.xSize, light, NULL);
}

//------------------------------------------------------------------------------

int	bLinearTMapPolyObjs = 1;

//used for robot engine glow
//function that takes the same parms as draw_tmap, but renders as flat poly
//we need this to do the cloaked effect

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	I2X (2)
#define	CLOAK_FADEOUT_DURATION_PLAYER	I2X (2)

#define	CLOAK_FADEIN_DURATION_ROBOT	I2X (1)
#define	CLOAK_FADEOUT_DURATION_ROBOT	I2X (1)

//------------------------------------------------------------------------------

int GetCloakInfo (CObject *objP, fix xCloakStartTime, fix xCloakEndTime, tCloakInfo *ciP)
{
	tCloakInfo	ci = {0, CLOAKED_FADE_LEVEL, I2X (1), I2X (1), I2X (1), 0, 0};
	int			i;

if (!(xCloakStartTime || xCloakEndTime)) {
	if (objP->info.nType == OBJ_PLAYER) {
		xCloakStartTime = gameData.multiplayer.players [objP->info.nId].cloakTime;
		xCloakEndTime = gameData.multiplayer.players [objP->info.nId].cloakTime + CLOAK_TIME_MAX;
		}
	else if (objP->info.nType == OBJ_ROBOT) {
		if (!ROBOTINFO (objP->info.nId).bossFlag) {
			xCloakStartTime = gameData.time.xGame - I2X (10);
			xCloakEndTime = gameData.time.xGame + I2X (10);
			}
		else if (0 <= (i = gameData.bosses.Find (objP->Index ()))) {
			xCloakStartTime = gameData.bosses [i].m_nCloakStartTime;
			xCloakEndTime = gameData.bosses [i].m_nCloakEndTime;
			}
		}
	}
if (xCloakStartTime != 0x7fffffff)
	ci.xTotalTime = xCloakEndTime - xCloakStartTime;
else
	ci.xTotalTime = gameData.time.xGame;
if (objP->info.nType == OBJ_PLAYER) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_PLAYER;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_PLAYER;
	}
else if (objP->info.nType == OBJ_ROBOT) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_ROBOT;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_ROBOT;
	}
else
	return 0;

ci.xDeltaTime = gameData.time.xGame - ((xCloakStartTime == 0x7fffffff) ? 0 : xCloakStartTime);
#if 0
if (ci.xDeltaTime < ci.xFadeinDuration) {	// make object transparent during second half
	ci.nFadeValue = X2I (FixDiv (ci.xFadeinDuration - ci.xDeltaTime, ci.xFadeinDuration) * CLOAKED_FADE_LEVEL);
	ci.bFading = 1;
	}
#else
// only decrease light during first half of cloak initiation time
if (ci.xDeltaTime < ci.xFadeinDuration / 2) {
	ci.xLightScale = FixDiv (ci.xFadeinDuration / 2 - ci.xDeltaTime, ci.xFadeinDuration / 2);
	ci.bFading = -1;
	}
else if (ci.xDeltaTime < ci.xFadeinDuration) {	// make object transparent during second half
	ci.nFadeValue = X2I (FixDiv (ci.xDeltaTime - ci.xFadeinDuration / 2, ci.xFadeinDuration / 2) * CLOAKED_FADE_LEVEL);
	ci.bFading = 1;
	}
#endif
else if ((xCloakStartTime == 0x7fffffff) || (gameData.time.xGame < xCloakEndTime - ci.xFadeoutDuration)) {
	static int nCloakDelta = 0, nCloakDir = 1;
	static fix xCloakTimer = 0;

	//note, if more than one cloaked CObject is visible at once, the
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
	ci.nFadeValue = X2I (FixDiv (ci.xTotalTime - ci.xFadeoutDuration / 2 - ci.xDeltaTime, ci.xFadeoutDuration / 2) * CLOAKED_FADE_LEVEL);
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
int DrawCloakedObject (CObject *objP, fix light, fix *glow, fix xCloakStartTime, fix xCloakEndTime)
{
	tObjTransformation	*posP = OBJPOS (objP);
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
	bOk = DrawPolyModel (objP, &posP->vPos, &posP->mOrient,
									reinterpret_cast<CAngleVector*> (&objP->rType.polyObjInfo.animAngles),
									objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags,
									xNewLight, glow, altTextures, NULL);
	gameData.models.nLightScale = 0;
	glow [0] = xSaveGlow;
	}
else {
	gameStates.render.bCloaked = 1;
	gameStates.render.grAlpha = GrAlpha (ci.nFadeValue);
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);	//set to black (matters for s3)
	fpDrawTexPolyMulti = G3DrawTexPolyFlat;
	bOk = DrawPolyModel (objP, &posP->vPos, &posP->mOrient,
								reinterpret_cast<CAngleVector*> (&objP->rType.polyObjInfo.animAngles),
								objP->rType.polyObjInfo.nModel, objP->rType.polyObjInfo.nSubObjFlags,
								light, glow, NULL, NULL);
	fpDrawTexPolyMulti = G3DrawTexPolyMulti;
	gameStates.render.grAlpha = 1.0f;
	gameStates.render.bCloaked = 0;
	}
return bOk;
}

//------------------------------------------------------------------------------

static inline int ObjectIsCloaked (CObject *objP)
{
if (gameStates.render.bBuildModels)
	return 0;
else if (objP->info.nType == OBJ_PLAYER)
	return (gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_CLOAKED) != 0;
else if (objP->info.nType == OBJ_ROBOT)
	return objP->cType.aiInfo.CLOAKED;
else
	return 0;
}

//------------------------------------------------------------------------------

int DrawHiresObject (CObject *objP, fix xLight, fix *xEngineGlow)
{
	float			fLight [3];
	short			nModel = 0;
	OOF::CModel	*po;

if (gameStates.render.bLoResShadows && (gameStates.render.nShadowPass == 2))
	return 0;
if (objP->info.nType == OBJ_DEBRIS)
	return 0;
else if ((objP->info.nType == OBJ_POWERUP) || (objP->info.nType == OBJ_WEAPON)) {
	if (objP->info.nType == OBJ_POWERUP)
		nModel = PowerupModel (objP->info.nId);
	else if (objP->info.nType == OBJ_WEAPON)
		nModel = WeaponModel (objP);
	if (!nModel)
		return 0;
	}
else
	nModel = objP->rType.polyObjInfo.nModel;
if (!(po = GetOOFModel (nModel)))
	return 0;
if (gameData.models.renderModels [1][nModel].m_bValid >= 0)
	return 0;
//G3RenderModel (objP, nModel, NULL, NULL, NULL, xLight, NULL, color);
fLight [0] = xLight / 65536.0f;
fLight [1] = (float) xEngineGlow [0] / 65536.0f;
fLight [2] = (float) xEngineGlow [1] / 65536.0f;
po->Render (objP, fLight, ObjectIsCloaked (objP));
return 1;
}

//------------------------------------------------------------------------------
//draw an CObject which renders as a polygon model
#define MAX_MODEL_TEXTURES 63

int DrawPolygonObject (CObject *objP, int bDepthSort, int bForce)
{
	fix	xLight;
	int	imSave = 0;
	fix	xEngineGlow [2];		//element 0 is for engine glow, 1 for headlight
	int	bBlendPolys = 0;
	int	bBrightPolys = 0;
	int	bGatling = 0;
	int	bCloaked = ObjectIsCloaked (objP);
	int	bEnergyWeapon;
	int	i, id, bOk = 0;

if (objP->info.nType == 255)
	return 0;
id = (int) objP->info.nId;
if ((id < 0) || (id == 255))
	bEnergyWeapon = id = 0;
else {
#if 0
	bGatling = ((objP->info.nType == OBJ_WEAPON) && ((id == VULCAN_ID) || (id == GAUSS_ID)));
#endif
	bEnergyWeapon = (objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [id] && !gameData.objs.bIsMissile [id];
	}
#if SHADOWS
if (!bForce && FAST_SHADOWS &&
	 !gameOpts->render.shadows.bSoft &&
	 (gameStates.render.nShadowPass == 3))
	return 1;
#endif
if (gameStates.render.bBuildModels)
	xLight = I2X (1);
else {
	xLight = CalcObjectLight (objP, xEngineGlow);
	if ((bCloaked || bEnergyWeapon) && bDepthSort && (gameStates.render.nShadowPass != 2)) {
		transparencyRenderer.AddObject (objP);
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
#if DBG
	CPolyModel* pm = gameData.models.polyModels [0] + objP->rType.polyObjInfo.nModel;
#endif
	tBitmapIndex	bm = gameData.pig.tex.bmIndex [0][objP->rType.polyObjInfo.nTexOverride],
						bmiP [MAX_MODEL_TEXTURES];

#if DBG
	Assert (pm->TextureCount () <= 12);
#endif
	for (i = 0; i < MAX_MODEL_TEXTURES; i++)		//fill whole array, in case simple model needs more
		bmiP [i] = bm;
	bOk = DrawPolyModel (objP, &objP->info.position.vPos,
									&objP->info.position.mOrient,
									reinterpret_cast<CAngleVector*> ( &objP->rType.polyObjInfo.animAngles),
									objP->rType.polyObjInfo.nModel,
									objP->rType.polyObjInfo.nSubObjFlags,
									xLight,
									xEngineGlow,
									bmiP,
									NULL);
	}
else {
	if (bCloaked) {
		if (objP->info.nType == OBJ_PLAYER)
			bOk = DrawCloakedObject (objP, xLight, xEngineGlow, gameData.multiplayer.players [id].cloakTime,
											 gameData.multiplayer.players [id].cloakTime + CLOAK_TIME_MAX);
		else if (objP->info.nType == OBJ_ROBOT) {
			if (!ROBOTINFO (id).bossFlag)
				bOk = DrawCloakedObject (objP, xLight, xEngineGlow, gameData.time.xGame - I2X (10), gameData.time.xGame + I2X (10));
			else if (0 <= (i = gameData.bosses.Find (objP->Index ())))
				bOk = DrawCloakedObject (objP, xLight, xEngineGlow, gameData.bosses [i].m_nCloakStartTime, gameData.bosses [i].m_nCloakEndTime);
			}
		}
	else {
		tBitmapIndex *bmiAltTex = (objP->rType.polyObjInfo.nAltTextures > 0) ? mpTextureIndex [objP->rType.polyObjInfo.nAltTextures - 1] : NULL;

		//	Snipers get bright when they fire.
		if (!gameStates.render.bBuildModels) {
			if ((objP->info.nType == OBJ_ROBOT) &&
				 (gameData.ai.localInfo [objP->Index ()].nextPrimaryFire < I2X (1) / 8) &&
				 (objP->cType.aiInfo.behavior == AIB_SNIPE))
				xLight = 2 * xLight + I2X (1);
			bBlendPolys = bEnergyWeapon && (gameData.weapons.info [id].nInnerModel > -1);
			bBrightPolys = bGatling || (bBlendPolys && WI_energy_usage (id));
			if (bEnergyWeapon) {
				if (gameOpts->legacy.bRender)
					gameStates.render.grAlpha = GrAlpha (FADE_LEVELS - 2);
				else
					OglBlendFunc (GL_ONE, GL_ONE);
				}
			if (bBlendPolys) {
#if 0
				fix xDistToEye = CFixVector::Dist(gameData.objs.viewerP->info.position.vPos, objP->info.position.vPos);
				if (xDistToEye < gameData.models.nSimpleModelThresholdScale * I2X (2))
#endif
					bOk = DrawPolyModel (
						objP, &objP->info.position.vPos, &objP->info.position.mOrient,
						objP->rType.polyObjInfo.animAngles,
						gameData.weapons.info [id].nInnerModel,
						objP->rType.polyObjInfo.nSubObjFlags,
						bBrightPolys ? I2X (1) : xLight,
						xEngineGlow,
						bmiAltTex,
						NULL);
				}
			if (bEnergyWeapon)
				gameStates.render.grAlpha = GrAlpha (4 * FADE_LEVELS / 5);
			else if (!bBlendPolys)
				gameStates.render.grAlpha = 1.0f;
			}
		bOk = DrawPolyModel (
			objP, &objP->info.position.vPos, &objP->info.position.mOrient,
			objP->rType.polyObjInfo.animAngles,
			objP->rType.polyObjInfo.nModel,
			objP->rType.polyObjInfo.nSubObjFlags,
			(bGatling || bBrightPolys) ? I2X (1) : xLight,
			xEngineGlow,
			bmiAltTex,
			(bGatling || bEnergyWeapon) ? gameData.weapons.color + id : NULL);
		if (!gameStates.render.bBuildModels) {
			if (!gameOpts->legacy.bRender)
				OglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gameStates.render.grAlpha = 1.0f;
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

static int RenderPlayerModel (CObject* objP, int bDepthSort, int bSpectate)
{
int bDynObjLight = (RENDERPATH && gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects;
if (automap.m_bDisplay && !(AM_SHOW_PLAYERS && AM_SHOW_PLAYER (objP->info.nId)))
	return 0;
tObjTransformation savePos;
if (bSpectate) {
	savePos = objP->info.position;
	objP->info.position = gameStates.app.playerPos;
	}
//DoObjectSmoke (objP);
DrawPolygonObject (objP, bDepthSort, 0);
gameOpts->ogl.bLightObjects = bDynObjLight;
if (!gameStates.render.bQueryCoronas) {
	RenderThrusterFlames (objP);
	RenderPlayerShield (objP);
	RenderTargetIndicator (objP, NULL);
	}
RenderTowedFlag (objP);
if (bSpectate)
	objP->info.position = savePos;
return 1;
}

// -----------------------------------------------------------------------------

static int RenderRobotModel (CObject* objP, int bDepthSort, int bSpectate)
{
if (gameStates.render.nType != 1)
	return 0;
if (automap.m_bDisplay && !AM_SHOW_ROBOTS)
	return 0;
gameData.models.vScale.SetZero ();
#if DBG
if (objP->Index () == nDbgObj)
	nDbgObj = nDbgObj;
#endif
DrawPolygonObject (objP, bDepthSort, 0);
if (!gameStates.render.bQueryCoronas && objP->info.controlType) {
	RenderThrusterFlames (objP);
	if (gameStates.render.nShadowPass != 2) {
		RenderRobotShield (objP);
		RenderTargetIndicator (objP, NULL);
		SetRobotLocationInfo (objP);
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int RenderReactorModel (CObject* objP, int bDepthSort, int bSpectate)
{
if (gameStates.render.nType != 1)
	return 0;
DrawPolygonObject (objP, bDepthSort, 0);
if (!gameStates.render.bQueryCoronas)
	RenderTargetIndicator (objP, NULL);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderWeaponModel (CObject* objP, int bDepthSort, int bSpectate)
{
if (automap.m_bDisplay && !AM_SHOW_POWERUPS (1))
	return 0;
if (!(gameStates.app.bNostalgia || gameOpts->render.powerups.b3D) && WeaponIsMine (objP->info.nId) && (objP->info.nId != SMALLMINE_ID))
	ConvertWeaponToVClip (objP);
else {
	if (gameStates.render.nType != 1)
		return 0;
	if (gameData.objs.bIsMissile [objP->info.nId]) {	//make missiles smaller during launch
		if ((objP->cType.laserInfo.parent.nType == OBJ_PLAYER) &&
			 (gameData.models.renderModels [1][108].m_bValid > 0)) {	//hires player ship
			float dt = X2F (gameData.time.xGame - objP->CreationTime ());

			if (dt < 1) {
				fix xScale = (fix) (I2X (1) + I2X (1) * dt * dt) / 2;
				gameData.models.vScale.Set (xScale, xScale, xScale);
				}
			}
		//DoObjectSmoke (objP);
		DrawPolygonObject (objP, bDepthSort, 0);
#if RENDER_HITBOX
#	if 0
		DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#	else
		RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
		RenderThrusterFlames (objP);
		gameData.models.vScale.SetZero ();
		}
	else {
#if RENDER_HITBOX
#	if 0
		DrawShieldSphere (objP, 0.66f, 0.2f, 0.0f, 0.4f);
#	else
		RenderHitbox (objP, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
		if (objP->info.nType != OBJ_WEAPON) {
			DrawPolygonObject (objP, bDepthSort, 0);
			if ((objP->info.nId != SMALLMINE_ID) && !gameStates.render.bQueryCoronas)
				RenderLightTrail (objP);
			}
		else {
			if ((objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID)) {
				if (SHOW_OBJ_FX && extraGameInfo [0].bTracers) {
					if (!gameStates.render.bQueryCoronas)
						RenderLightTrail (objP);
					gameData.models.vScale.Set (I2X (1) / 4, I2X (1) / 4, I2X (2));
					CFixVector vSavedPos = objP->info.position.vPos;
					objP->info.position.vPos += objP->info.position.mOrient.FVec ();
					DrawPolygonObject (objP, bDepthSort, 0);
					objP->info.position.vPos = vSavedPos;
					}
				}
			else {
				if ((objP->info.nId != SMALLMINE_ID) && !gameStates.render.bQueryCoronas)
					RenderLightTrail (objP);
				DrawPolygonObject (objP, bDepthSort, 0);
				}
			gameData.models.vScale.SetZero ();
			}
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int RenderPowerupModel (CObject* objP, int bDepthSort, int bSpectate)
{
if (automap.m_bDisplay && !AM_SHOW_POWERUPS (1))
	return 0;
if (!gameStates.app.bNostalgia && gameOpts->render.powerups.b3D) {
	RenderPowerupCorona (objP, 1, 1, 1, coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	if (!DrawPolygonObject (objP, bDepthSort, 0))
		ConvertWeaponToPowerup (objP);
	else {
		objP->mType.physInfo.mass = I2X (1);
		objP->mType.physInfo.drag = 512;
		if (gameOpts->render.powerups.nSpin !=
			((objP->mType.physInfo.rotVel [Y] | objP->mType.physInfo.rotVel [Z]) != 0))
			objP->mType.physInfo.rotVel [Y] =
			objP->mType.physInfo.rotVel [Z] = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
		}
#if DBG
	RenderRobotShield (objP);
#endif
	gameData.models.vScale.SetZero ();
	}
else
	ConvertWeaponToPowerup (objP);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderHostageModel (CObject* objP, int bDepthSort, int bSpectate)
{
if (gameStates.app.bNostalgia || !(gameOpts->render.powerups.b3D && DrawPolygonObject (objP, bDepthSort, 0)))
	ConvertModelToHostage (objP);
#if DBG
RenderRobotShield (objP);
#endif
return 1;
}

// -----------------------------------------------------------------------------

static int RenderPolyModel (CObject* objP, int bDepthSort, int bSpectate)
{
DrawPolygonObject (objP, bDepthSort, 0);
DrawDebrisCorona (objP);
if (IsSpawnMarkerObject (objP))
	RenderMslLockIndicator (objP);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderFireball (CObject* objP, int bForce)
{
if (!bForce && (gameStates.render.bQueryCoronas || (gameStates.render.nType != 1)))
	return 0;
if (gameStates.render.nShadowPass != 2) {
	DrawFireball (objP);
	if (objP->info.nType == OBJ_WEAPON) {
		RenderLightTrail (objP);
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int RenderExplBlast (CObject* objP, int bForce)
{
if (!bForce && (gameStates.render.bQueryCoronas || (gameStates.render.nType != 1)))
	return 0;
if (gameStates.render.nShadowPass != 2)
	DrawExplBlast (objP);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderShrapnel (CObject* objP, int bForce)
{
if (!bForce && (gameStates.render.bQueryCoronas || (gameStates.render.nType != 1)))
	return 0;
if (gameStates.render.nShadowPass != 2)
	shrapnelManager.Draw (objP);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderWeapon (CObject* objP, int bForce)
{
if (gameStates.render.nType != 1)
	return 0;
if (gameStates.render.nShadowPass != 2) {
	if (automap.m_bDisplay && !AM_SHOW_POWERUPS (1))
		return 0;
	if (objP->info.nType != OBJ_WEAPON)
		DrawWeaponVClip (objP);
	else {
		if (WeaponIsMine (objP->info.nId)) {
			if (!DoObjectSmoke (objP))
				DrawWeaponVClip (objP);
			}
		else if ((objP->info.nId != OMEGA_ID) || !(SHOW_LIGHTNINGS && gameOpts->render.lightnings.bOmega && !gameStates.app.bHaveMod)) {
			DrawWeaponVClip (objP);
			if (objP->info.nId != OMEGA_ID) {
				RenderLightTrail (objP);
				RenderMslLockIndicator (objP);
				}
			}
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int RenderHostage (CObject* objP, int bDepthSort, int bForce)
{
if (gameStates.render.nType != 1)
	return 0;
if (ConvertHostageToModel (objP))
	DrawPolygonObject (objP, bDepthSort, 0);
else if (gameStates.render.nShadowPass != 2)
	DrawHostage (objP);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderPowerup (CObject* objP, int bDepthSort, int bForce)
{
if (automap.m_bDisplay && !AM_SHOW_POWERUPS (1))
	return 0;
if (gameStates.render.nType != 1)
	return 0;
if (objP->PowerupToWeapon ()) {
	RenderPowerupCorona (objP, 1, 1, 1, coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	DrawPolygonObject (objP, bDepthSort, 0);
	}
else if (gameStates.render.nShadowPass != 2)
	DrawPowerup (objP);
return 1;
}

// -----------------------------------------------------------------------------

static int RenderLaser (CObject* objP, int bForce)
{
if (gameStates.render.bQueryCoronas && (gameStates.render.nType != 1))
	return 0;
if (gameStates.render.nShadowPass != 2) {
	RenderLaser (objP);
	if (objP->info.nType == OBJ_WEAPON)
		RenderLightTrail (objP);
	}
return 1;
}

// -----------------------------------------------------------------------------

int RenderObject (CObject *objP, int nWindow, int bForce)
{
	short			nObject = objP->Index ();
	int			bSpectate = 0, bDepthSort = RENDERPATH || (gameOpts->render.bDepthSort > 0);

int nType = objP->info.nType;
if (nType == 255) {
	objP->Die ();
	return 0;
	}
int bEmissive = (objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsWeapon [objP->info.nId] && !gameData.objs.bIsMissile [objP->info.nId];
if (bEmissive && gameStates.render.bQueryCoronas)
	return 0;
if ((gameStates.render.nShadowPass != 2) &&
	 (objP == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP) &&
	 (objP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature)) {
	return 0;
	}
#if DBG
if (nObject == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (nObject != LOCALPLAYER.nObject) {
	if (objP == gameData.objs.viewerP)
		return 0;
	 }
else if ((gameData.objs.viewerP == gameData.objs.consoleP) && !automap.m_bDisplay) {
	if ((bSpectate = (gameStates.render.bFreeCam && !nWindow)))
		;
#if DBG
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindow || (!gameStates.render.bChaseCam && (gameStates.app.bEndLevelSequence < EL_LOOKBACK)))) { //don't render ship model if neither external view nor main view
#else
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindow ||
				  ((IsMultiGame && !IsCoopGame && !EGI_FLAG (bEnableCheats, 0, 0, 0)) ||
				  (!gameStates.render.bChaseCam && (gameStates.app.bEndLevelSequence < EL_LOOKBACK))))) {
#endif
#if 0
		if (gameOpts->render.particles.bPlayers) {
			SEM_ENTER (SEM_SMOKE)
			DoPlayerSmoke (objP, -1);
			SEM_LEAVE (SEM_SMOKE)
			}
#endif
		return 0;
		}
	}
if ((nType == OBJ_NONE)/* || (nType==OBJ_CAMBOT)*/){
#if TRACE
	console.printf (1, "ERROR!!!Bogus obj %d in seg %d is rendering!\n", nObject, objP->info.nSegment);
#endif
	return 0;
	}
int mldSave = gameStates.render.detail.nMaxLinearDepth;
gameStates.render.nState = 1;
gameData.objs.color.index = 0;
gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;

switch (objP->info.renderType) {
	case RT_NONE:
		if (gameStates.render.nType != 1)
			return 0;
		break;	

	case RT_POLYOBJ:
		if (nType == OBJ_EFFECT) {
			objP->info.renderType = (objP->info.nId == SMOKE_ID) ? RT_SMOKE : RT_LIGHTNING;
			return 0;
			}
		if (gameStates.render.nType != 1)
			return 0;
		if (nType == OBJ_PLAYER) {
			if (!RenderPlayerModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		else if (nType == OBJ_ROBOT) {
			if (!RenderRobotModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		else if (nType == OBJ_WEAPON) {
			if (!RenderWeaponModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		else if (nType == OBJ_REACTOR) {
			if (!RenderReactorModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		else if (nType == OBJ_POWERUP) {
			if (!RenderPowerupModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		else if (nType == OBJ_HOSTAGE) {
			if (!RenderHostageModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		else {
			if (!RenderPolyModel (objP, bDepthSort, bSpectate))
				return 0;
			}
		break;

	case RT_MORPH:
		if (gameStates.render.nType != 1)
			return 0;
		if (gameStates.render.nShadowPass != 2)
			objP->MorphDraw ();
		break;

	case RT_THRUSTER:
		if (gameStates.render.bQueryCoronas || (gameStates.render.nType != 1))
			return 0;
		if (nWindow && (objP->mType.physInfo.flags & PF_WIGGLE))
			break;

	case RT_FIREBALL:
		if (!RenderFireball (objP, bForce))
			return 0;
		break;

	case RT_EXPLBLAST:
		if (!RenderExplBlast (objP, bForce))
			return 0;
		break;

	case RT_SHRAPNELS:
		if (!RenderShrapnel (objP, bForce))
			return 0;
		break;

	case RT_WEAPON_VCLIP:
		if (!RenderWeapon (objP, bForce))
			return 0;
		break;

	case RT_HOSTAGE:
		if (!RenderHostage (objP, bDepthSort, bForce))
			return 0;
		break;

	case RT_POWERUP:
		if (!RenderPowerup (objP, bDepthSort, bForce))
			return 0;
		break;

	case RT_LASER:
		if (!RenderLaser (objP, bForce))
			return 0;
		break;

	case RT_SMOKE:
	case RT_LIGHTNING:
		break;

	default:
		PrintLog ("Unknown renderType <%d>\n", objP->info.renderType);
	}

#ifdef NEWDEMO
if (objP->info.renderType != RT_NONE)
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		if (!gameData.demo.bWasRecorded [nObject]) {
			NDRecordRenderObject (objP);
			gameData.demo.bWasRecorded [nObject] = 1;
		}
	}
#endif
gameStates.render.detail.nMaxLinearDepth = mldSave;
gameData.render.nTotalObjects++;
OglClearError (0);
return 1;
}

//------------------------------------------------------------------------------
//eof
