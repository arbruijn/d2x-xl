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
#include "../effects/glow.h"
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
#include "thrusterflames.h"
#include "renderthreads.h"

//------------------------------------------------------------------------------

int16_t PowerupModel (int32_t nId)
{
	int16_t nModel;

if ((nModel = PowerupToModel (nId)))
	return nModel;
if (0 > (nId = PowerupToObject (nId)))
	return 0;
return gameData.weapons.info [nId].nModel;
}

//------------------------------------------------------------------------------

int16_t WeaponModel (CObject *pObj)
{
	int16_t	nModel;

if ((nModel = WeaponToModel (pObj->info.nId)))
	return nModel;
return pObj->ModelId ();
}

//------------------------------------------------------------------------------

int32_t InitAddonPowerup (CObject *pObj)
{
if (pObj->info.nId == POW_SLOWMOTION)
	pObj->rType.animationInfo.nClipIndex = -1;
else if (pObj->info.nId == POW_BULLETTIME)
	pObj->rType.animationInfo.nClipIndex = -2;
else
	return 0;
pObj->rType.animationInfo.nCurFrame = 0;
pObj->rType.animationInfo.xTotalTime = 0;
return 1;
}

// -----------------------------------------------------------------------------

void ConvertWeaponToPowerup (CObject *pObj)
{
if (!InitAddonPowerup (pObj)) {
	pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
	pObj->rType.animationInfo.xFrameTime = gameData.effects.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
	pObj->rType.animationInfo.nCurFrame = 0;
	pObj->SetSizeFromPowerup ();
	}
pObj->info.controlType = CT_POWERUP;
pObj->info.renderType = RT_POWERUP;
pObj->mType.physInfo.mass = I2X (1);
pObj->mType.physInfo.drag = 512;
}

// -----------------------------------------------------------------------------

int32_t ConvertHostageToModel (CObject *pObj)
{
if (pObj->info.renderType == RT_POLYOBJ)
	return 1;
if (gameStates.app.bNostalgia || !gameOpts->Use3DPowerups ())
	return 0;
if (!HaveReplacementModel (HOSTAGE_MODEL))
	return 0;
pObj->info.renderType = RT_POLYOBJ;
pObj->rType.polyObjInfo.nModel = HOSTAGE_MODEL;
pObj->rType.polyObjInfo.nTexOverride = -1;
pObj->mType.physInfo.rotVel.SetZero ();
memset (pObj->rType.polyObjInfo.animAngles, 0, sizeof (pObj->rType.polyObjInfo.animAngles));
return 1;
}

// -----------------------------------------------------------------------------

int32_t ConvertModelToHostage (CObject *pObj)
{
pObj->rType.animationInfo.nClipIndex = nHostageVClips [0];
pObj->rType.animationInfo.xFrameTime = gameData.effects.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
pObj->rType.animationInfo.nCurFrame = 0;
pObj->SetSize (289845);
pObj->info.controlType = CT_POWERUP;
pObj->info.renderType = RT_HOSTAGE;
pObj->mType.physInfo.mass = I2X (1);
pObj->mType.physInfo.drag = 512;
return 1;
}

// -----------------------------------------------------------------------------

int32_t CObject::PowerupToDevice (void)
{

if (gameStates.app.bNostalgia)
	return 0;
if (!gameOpts->Use3DPowerups ())
	return 0;
if (info.controlType == CT_WEAPON)
	return 1;
if ((info.nType != OBJ_POWERUP) && (info.nType != OBJ_WEAPON))
	return 0;

int32_t bHasModel = 0;
int16_t nId, nModel = PowerupToModel (info.nId);

if (nModel)
	nId = info.nId;
else {
	nId = PowerupToObject (info.nId);
	if (nId >= 0) {
		nModel = gameData.weapons.info [nId].nModel;
		bHasModel = 1;
		}
	}
if (!bHasModel && !IsMissile () && !(nModel && HaveReplacementModel (nModel)))
		return 0;

#if 0 //DBG
Orientation () = gameData.objData.pConsole->Orientation ();
mType.physInfo.rotVel.v.coord.x =
mType.physInfo.rotVel.v.coord.y =
mType.physInfo.rotVel.v.coord.z = 0;
#else
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	CAngleVector a;
	a.v.coord.p = 2 * SRandShort ();
	a.v.coord.b = 2 * SRandShort ();
	a.v.coord.h = 2 * SRandShort ();
	info.position.mOrient = CFixMatrix::Create(a);
	}
mType.physInfo.rotVel.v.coord.x = 0;
mType.physInfo.rotVel.v.coord.y =
mType.physInfo.rotVel.v.coord.z = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
#endif
mType.physInfo.mass = I2X (1);
mType.physInfo.drag = 512;
mType.physInfo.brakes = 0;
info.controlType = CT_WEAPON;
info.renderType = RT_POLYOBJ;
info.movementType = MT_PHYSICS;
mType.physInfo.flags = PF_BOUNCES | PF_FREE_SPINNING;
rType.polyObjInfo.nModel = nModel;
if (bHasModel)
	AdjustSize ();
rType.polyObjInfo.nTexOverride = -1;
SetLife (IMMORTAL_TIME);
return 1;
}

// -----------------------------------------------------------------------------

void ConvertAllPowerupsToWeapons (void)
{
	CObject* pObj;

FORALL_OBJS (pObj)
	if (pObj->info.renderType == RT_POWERUP) {
		pObj->PowerupToDevice ();
		pObj->LoadTextures ();
		}
}

// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
void SetRobotLocationInfo (CObject *pObj)
{
if (gameStates.app.bPlayerFiredLaserThisFrame != -1) {
	CRenderPoint temp;

	temp.TransformAndEncode (pObj->info.position.vPos);
	if (temp.Behind ())		//robot behind the screen
		return;
	//the code below to check for CObject near the center of the screen
	//completely ignores z, which may not be good
	if ((abs (temp.ViewPos ().v.coord.x) < I2X (4)) && (abs (temp.ViewPos ().v.coord.y) < I2X (4))) {
		pObj->cType.aiInfo.nDangerLaser = gameStates.app.bPlayerFiredLaserThisFrame;
		pObj->cType.aiInfo.nDangerLaserSig = OBJECT (gameStates.app.bPlayerFiredLaserThisFrame)->info.nSignature;
		}
	}
}

//------------------------------------------------------------------------------

fix CalcObjectLight (CObject *pObj, fix *xEngineGlow)
{
	fix xLight;

if (gameStates.render.bBuildModels)
	return I2X (1);
if (IsMultiGame && netGameInfo.m_info.BrightPlayers && (pObj->info.nType == OBJ_PLAYER)) {
	xLight = I2X (1); //	If option set for bright players in netgame, brighten them
	gameOpts->ogl.bLightObjects = 0;
	}
else
	xLight = ComputeObjectLight (pObj, NULL);
//make robots brighter according to robot glow field
if (pObj->info.nType == OBJ_ROBOT)
	xLight += ROBOTINFO (pObj) ? (ROBOTINFO (pObj)->glow << 12) : 0;		//convert 4:4 to 16:16
else if (pObj->info.nType == OBJ_WEAPON) {
	if (pObj->info.nId == FLARE_ID)
		xLight += I2X (2);
	}
else if (pObj->info.nType == OBJ_MARKER)
 	xLight += I2X (2);
ComputeEngineGlow (pObj, xEngineGlow);
return xLight;
}

//------------------------------------------------------------------------------

static float ObjectBlobColor (CObject *pObj, CBitmap *pBm, CFloatVector *pColor, bool bMaxOut = false)
{
	float	fScale;

pBm->GetAvgColor (pColor);
#if DBG
if ((pObj->info.nType == nDbgObjType) && ((nDbgObjId < 0) || (pObj->info.nId == nDbgObjId)))
	BRP;
#endif
if (bMaxOut) {
	float h = pColor->Red ();
	if (h < pColor->Green ())
		h = pColor->Green ();
	if (h < pColor->Blue ())
		h = pColor->Blue ();
	if ((h > 0.0f) && (h < 1.0f)) {
		pColor->Red () /= h;
		pColor->Green () /= h;
		pColor->Blue () /= h;
		}
	}
fScale = pColor->Red () + pColor->Green () + pColor->Blue ();
if (fScale == 0) {
	pColor->Red () =
	pColor->Green () =
	pColor->Blue () = 1;
	}
return fScale;
}

//------------------------------------------------------------------------------

static int32_t DrawShield3D (CObject* pObj, CFloatVector& color)
{
if ((pObj->mType.physInfo.velocity.IsZero ()) && (pObj->info.movementType != MT_SPINNING)) {
	pObj->info.movementType = MT_SPINNING;
	pObj->mType.spinRate = pObj->info.position.mOrient.m.dir.u * (I2X (1) / 8);
	}
//the actual shield in the sprite texture has 3/4 of the textures size
return DrawShieldSphere (pObj, color.Red (), color.Green (), color.Blue (), 1.0f, 0, 3 * pObj->info.xSize / 4);
}

//------------------------------------------------------------------------------
//draw an CObject that has one bitmap & doesn't rotate

void DrawObjectBitmap (CObject *pObj, int32_t bmi0, int32_t bmi, int32_t iFrame, CFloatVector *pColor, float fAlpha)
{
	int32_t nType = pObj->info.nType;

if ((nType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
	return;

	CBitmap*			pBm;
	CFloatVector	color;
	int32_t			nId = pObj->info.nId;
#if 0
	int32_t			bMuzzleFlash = 0;
#endif
	int32_t			bAdditive = 0, bEnergy = 0;
	fix				xSize;

if ((nType == OBJ_WEAPON) && (pObj->info.nId == OMEGA_ID) && omegaLightning.Exist ())
	return;
#if DBG
if ((nType == nDbgObjType) && ((nDbgObjId < 0) || (pObj->info.nId == nDbgObjId)))
	BRP;
#endif
if (gameOpts->render.textures.bUseHires [0] || gameOpts->render.effects.bTransparent) {
	if (fAlpha) {
		bAdditive = ((nType == OBJ_FIREBALL) || (nType == OBJ_EXPLOSION)) ? /*glowRenderer.Available (GLOW_SPRITES) ? 2 :*/ 1 : ((nType == OBJ_WEAPON) && (pObj->info.nId == OMEGA_ID)) ? 2 : 0;
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
			fAlpha = 1.0f; //2.0f / 3.0f;
			bAdditive = 1;
			}
		}
	}
else {
	fAlpha = 1.0f;
	}

if (bmi < 0) {
	if (-bmi - 1 >= int32_t (gameData.pig.tex.addonBitmaps.Length ()))
		return;
	pBm = &gameData.pig.tex.addonBitmaps [-bmi - 1];
	pBm = pBm->SetCurFrame (iFrame);
	}
else {
	CBitmap* pBmo;

	pBm = gameData.pig.tex.bitmaps [0] + bmi;
	if ((pBm->Type () == BM_TYPE_STD) && (pBmo = pBm->Override ()))
		pBm = pBmo->SetCurFrame (iFrame);
	}
if (!pBm || pBm->Bind (1))
	return;

bool b3DShield = ((pObj->Type () == OBJ_POWERUP) && ((pObj->Id () == POW_SHIELD_BOOST) || (pObj->Id () == POW_HOARD_ORB)) &&
					   gameOpts->Use3DPowerups () && gameOpts->render.powerups.b3DShields);

ObjectBlobColor (pObj, pBm, &color, b3DShield);
if (pColor /*&& (bmi >= 0)*/)
	*pColor = color;
	//memcpy (pColor, gameData.pig.tex.bitmapColors + bmi, sizeof (CFloatVector));

xSize = pObj->info.xSize;

if (b3DShield && DrawShield3D (pObj, color)) {
	if ((nType == OBJ_POWERUP) && ((bEnergy && gameOpts->render.coronas.bPowerups) || (!bEnergy && gameOpts->render.coronas.bWeapons)))
		RenderPowerupCorona (pObj, color.Red (), color.Green (), color.Blue (), coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
		}
else {
	if ((nType == OBJ_POWERUP) && ((bEnergy && gameOpts->render.coronas.bPowerups) || (!bEnergy && gameOpts->render.coronas.bWeapons)))
		RenderPowerupCorona (pObj, color.Red (), color.Green (), color.Blue (), coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	if (fAlpha < 1) {
		if (bAdditive) {
			color.Red () =
			color.Green () =
			color.Blue () = 0.5f;
			}
		else
			color.Red () = color.Green () = color.Blue () = 1.0f;
		color.Alpha () = fAlpha;
		CFixVector vPos = pObj->info.position.vPos;
		if (pBm->Width () > pBm->Height ())
			transparencyRenderer.AddSprite (pBm, vPos, &color, xSize, FixMulDiv (xSize, pBm->Height (), pBm->Width ()),
													  iFrame, bAdditive, (nType == OBJ_FIREBALL) ? 10.0f : 0.0f);
		else
			transparencyRenderer.AddSprite (pBm, vPos, &color, FixMulDiv (xSize, pBm->Width (), pBm->Height ()), xSize,
													  iFrame, bAdditive, (nType == OBJ_FIREBALL) ? 10.0f : 0.0f);
		}
	else {
		if (pBm->Width () > pBm->Height ())
			ogl.RenderBitmap (pBm, pObj->info.position.vPos, xSize, FixMulDiv (xSize, pBm->Height (), pBm->Width ()), NULL, fAlpha, bAdditive);
		else
			ogl.RenderBitmap (pBm, pObj->info.position.vPos, FixMulDiv (xSize, pBm->Width (), pBm->Height ()), xSize, NULL, fAlpha, bAdditive);
		}
	gameData.render.nTotalSprites++;
	}

}

//------------------------------------------------------------------------------

int32_t	bLinearTMapPolyObjs = 1;

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

int32_t GetCloakInfo (CObject *pObj, fix xCloakStartTime, fix xCloakEndTime, tCloakInfo *ciP)
{
	tCloakInfo	ci = {0, CLOAKED_FADE_LEVEL, I2X (1), I2X (1), I2X (1), 0, 0};
	int32_t		i;

if (!xCloakStartTime && !xCloakEndTime) {
	if (pObj->info.nType == OBJ_PLAYER) {
		xCloakStartTime = PLAYER (pObj->info.nId).cloakTime;
		xCloakEndTime = PLAYER (pObj->info.nId).cloakTime + CLOAK_TIME_MAX;
		}
	else if (pObj->info.nType == OBJ_ROBOT) {
		if (!pObj->IsBoss ()) {
			xCloakStartTime = gameData.time.xGame - I2X (10);
			xCloakEndTime = gameData.time.xGame + I2X (10);
			}
		else if (0 <= (i = gameData.bosses.Find (pObj->Index ()))) {
			xCloakStartTime = gameData.bosses [i].m_nCloakStartTime;
			xCloakEndTime = gameData.bosses [i].m_nCloakEndTime;
			}
		}
	}

if (xCloakStartTime != 0x7fffffff)
	ci.xTotalTime = xCloakEndTime - xCloakStartTime;
else
	ci.xTotalTime = gameData.time.xGame;
if (pObj->info.nType == OBJ_PLAYER) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_PLAYER;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_PLAYER;
	}
else if (pObj->info.nType == OBJ_ROBOT) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_ROBOT;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_ROBOT;
	}
else
	return 0;

ci.xDeltaTime = (xCloakStartTime == 0x7fffffff) ? gameData.time.xGame : gameData.time.xGame - xCloakStartTime;

// only decrease light during first half of cloak initiation time
if (ci.xDeltaTime < ci.xFadeinDuration / 2) {
	ci.xLightScale = FixDiv (ci.xFadeinDuration / 2 - ci.xDeltaTime, ci.xFadeinDuration / 2);
	ci.bFading = -1;
	}
else if (ci.xDeltaTime < ci.xFadeinDuration) // make object transparent during second half
	ci.nFadeValue = X2I (FixDiv (ci.xDeltaTime - ci.xFadeinDuration / 2, ci.xFadeinDuration / 2) * CLOAKED_FADE_LEVEL);
else if ((xCloakStartTime == 0x7fffffff) || (gameData.time.xGame < xCloakEndTime - ci.xFadeoutDuration)) { // fully cloaked
	static int32_t nCloakDelta = 0, nCloakDir = 1;
	static fix xCloakTimer = 0;

	//note, if more than one cloaked CObject is visible at once, the pulse rate will change!
	xCloakTimer -= gameData.time.xFrame;
	while (xCloakTimer < 0) {
		xCloakTimer += ci.xFadeoutDuration / 12;
		nCloakDelta += nCloakDir;
		if (nCloakDelta == 0 || nCloakDelta == 4)
			nCloakDir = -nCloakDir;
		}
	ci.nFadeValue = CLOAKED_FADE_LEVEL - nCloakDelta;
	}
else if (gameData.time.xGame < xCloakEndTime - ci.xFadeoutDuration / 2)
	ci.nFadeValue = X2I (FixDiv (ci.xTotalTime - ci.xFadeoutDuration / 2 - ci.xDeltaTime, ci.xFadeoutDuration / 2) * CLOAKED_FADE_LEVEL);
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
int32_t DrawCloakedObject (CObject *pObj, fix light, fix *glow, fix xCloakStartTime, fix xCloakEndTime)
{
	tObjTransformation*	pPos = OBJPOS (pObj);
	tCloakInfo				ci;
	int32_t					bOk = 0;

if (GetCloakInfo (pObj, xCloakStartTime, xCloakEndTime, &ci) != 0) {
	fix xNewLight, xSaveGlow;
	tBitmapIndex * altTextures = NULL;

	if (pObj->rType.polyObjInfo.nAltTextures > 0)
		altTextures = mpTextureIndex [pObj->rType.polyObjInfo.nAltTextures - 1];
	xNewLight = FixMul (light, ci.xLightScale);
	xSaveGlow = glow [0];
	glow [0] = FixMul (glow [0], ci.xLightScale);
	gameData.models.nLightScale = ci.xLightScale;
	bOk = DrawPolyModel (pObj, &pPos->vPos, &pPos->mOrient,
								reinterpret_cast<CAngleVector*> (&pObj->rType.polyObjInfo.animAngles),
								pObj->ModelId (true), pObj->rType.polyObjInfo.nSubObjFlags,
								xNewLight, glow, altTextures, NULL);
	gameData.models.nLightScale = 0;
	glow [0] = xSaveGlow;
	}
else {
	gameStates.render.bCloaked = 1;
	gameStates.render.grAlpha = GrAlpha (ci.nFadeValue);
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);	//set to black (matters for s3)
	fpDrawTexPolyMulti = G3DrawTexPolyFlat;
	bOk = DrawPolyModel (pObj, &pPos->vPos, &pPos->mOrient,
								reinterpret_cast<CAngleVector*> (&pObj->rType.polyObjInfo.animAngles),
								pObj->ModelId (true), pObj->rType.polyObjInfo.nSubObjFlags,
								light, glow, NULL, NULL);
	fpDrawTexPolyMulti = G3DrawTexPolyMulti;
	gameStates.render.grAlpha = 1.0f;
	gameStates.render.bCloaked = 0;
	}
return bOk;
}

//------------------------------------------------------------------------------

static inline int32_t ObjectIsCloaked (CObject *pObj)
{
return gameStates.render.bBuildModels ? 0 : pObj->Cloaked ();
}

//------------------------------------------------------------------------------

int32_t DrawHiresObject (CObject *pObj, fix xLight, fix *xEngineGlow)
{
	float			fLight [3];
	int16_t			nModel = 0;
	OOF::CModel	*po;

#if 0
if (gameStates.render.bLoResShadows && (gameStates.render.nShadowPass == 2))
	return 0;
#endif
if (pObj->info.nType == OBJ_DEBRIS)
	return 0;
else if ((pObj->info.nType == OBJ_POWERUP) || (pObj->info.nType == OBJ_WEAPON)) {
	if (pObj->info.nType == OBJ_POWERUP)
		nModel = PowerupModel (pObj->info.nId);
	else if (pObj->info.nType == OBJ_WEAPON)
		nModel = WeaponModel (pObj);
	if (!nModel)
		return 0;
	}
else
	nModel = pObj->ModelId ();
if (!(po = GetOOFModel (nModel)))
	return 0;
if (gameData.models.renderModels [1][nModel].m_bValid >= 0)
	return 0;
//G3RenderModel (pObj, nModel, NULL, NULL, NULL, xLight, NULL, color);
fLight [0] = xLight / 65536.0f;
fLight [1] = (float) xEngineGlow [0] / 65536.0f;
fLight [2] = (float) xEngineGlow [1] / 65536.0f;
po->Render (pObj, fLight, ObjectIsCloaked (pObj));
return 1;
}

//------------------------------------------------------------------------------
//draw an CObject which renders as a polygon model
#define MAX_MODEL_TEXTURES 63

int32_t DrawPolygonObject (CObject *pObj, int32_t bForce)
{
	fix	xLight;
	int32_t	imSave = 0;
	fix	xEngineGlow [2];		//element 0 is for engine glow, 1 for headlight
	int32_t	bBlendPolys = 0;
	int32_t	bBrightPolys = 0;
	int32_t	bGatling = 0;
	int32_t	bCloaked = ObjectIsCloaked (pObj);
	int32_t	bEnergyWeapon;
	int32_t	i, id, bOk = 0;

if (pObj->info.nType == 255)
	return 0;
#if DBG
if ((pObj->info.nType != OBJ_PLAYER) && (pObj->info.position.vPos == OBJECT (0)->info.position.vPos))
	BRP;
#endif
id = (int32_t) pObj->info.nId;
if ((id < 0) || (id == 255))
	bEnergyWeapon = id = 0;
else {
	bEnergyWeapon = pObj->IsEnergyProjectile ();
	}
if (!bForce && FAST_SHADOWS && !gameOpts->render.shadows.bSoft && (gameStates.render.nShadowPass == 3))
	return 1;
if (gameStates.render.bBuildModels)
	xLight = I2X (1);
else {
	xLight = CalcObjectLight (pObj, xEngineGlow);
	if ((gameStates.render.nType != RENDER_TYPE_TRANSPARENCY) && (bCloaked || bEnergyWeapon) && (gameStates.render.nShadowPass != 2)) {
		transparencyRenderer.AddObject (pObj);
		return 1;
		}
	if (DrawHiresObject (pObj, xLight, xEngineGlow))
		return 1;
	gameStates.render.bBrightObject = bEnergyWeapon;
	imSave = gameStates.render.nInterpolationMethod;
	if (bLinearTMapPolyObjs)
		gameStates.render.nInterpolationMethod = 1;
	}
if (pObj->rType.polyObjInfo.nTexOverride != -1) {
#if DBG
	CPolyModel* pm = gameData.models.polyModels [0] + pObj->ModelId ();
#endif
	tBitmapIndex	bm = gameData.pig.tex.bmIndex [0][pObj->rType.polyObjInfo.nTexOverride],
						pBmIndex [MAX_MODEL_TEXTURES];

#if DBG
	Assert (pm->TextureCount () <= 12);
#endif
	for (i = 0; i < MAX_MODEL_TEXTURES; i++)		//fill whole array, in case simple model needs more
		pBmIndex [i] = bm;
	bOk = DrawPolyModel (pObj, &pObj->info.position.vPos,
								&pObj->info.position.mOrient,
								reinterpret_cast<CAngleVector*> (&pObj->rType.polyObjInfo.animAngles),
								pObj->ModelId (),
								pObj->rType.polyObjInfo.nSubObjFlags,
								xLight,
								xEngineGlow,
								pBmIndex,
								NULL);
	}
else {
	if (bCloaked) {
		if (pObj->info.nType == OBJ_PLAYER)
			bOk = DrawCloakedObject (pObj, xLight, xEngineGlow, PLAYER (id).cloakTime, PLAYER (id).cloakTime + CLOAK_TIME_MAX);
		else if ((pObj->info.nType == OBJ_ROBOT) && ROBOTINFO (id)) {
			if (!ROBOTINFO (id)->bossFlag)
				bOk = DrawCloakedObject (pObj, xLight, xEngineGlow, gameData.time.xGame - I2X (10), gameData.time.xGame + I2X (10));
			else if (0 <= (i = gameData.bosses.Find (pObj->Index ())))
				bOk = DrawCloakedObject (pObj, xLight, xEngineGlow, gameData.bosses [i].m_nCloakStartTime, gameData.bosses [i].m_nCloakEndTime);
			}
		}
	else {
		tBitmapIndex *bmiAltTex = (pObj->rType.polyObjInfo.nAltTextures > 0) ? mpTextureIndex [pObj->rType.polyObjInfo.nAltTextures - 1] : NULL;

		//	Snipers get bright when they fire.
		if (!gameStates.render.bBuildModels) {
			if ((pObj->info.nType == OBJ_ROBOT) &&
				 (gameData.ai.localInfo [pObj->Index ()].pNextrimaryFire < I2X (1) / 8) &&
				 (pObj->cType.aiInfo.behavior == AIB_SNIPE))
				xLight = 2 * xLight + I2X (1);
			bBlendPolys = bEnergyWeapon && (gameData.weapons.info [id].nInnerModel > -1);
			bBrightPolys = bGatling || (bBlendPolys && WI_energy_usage (id));
			if (bEnergyWeapon) {
				if (gameOpts->legacy.bRender)
					gameStates.render.grAlpha = GrAlpha (FADE_LEVELS - 2);
				else {
					ogl.SetBlendMode (OGL_BLEND_ADD);
					glowRenderer.Begin (GLOW_OBJECTS, 2, false, 1.0f);
					}
				}
			if (bBlendPolys) {
#if 0
				fix xDistToEye = CFixVector::Dist(gameData.objData.pViewer->info.position.vPos, pObj->info.position.vPos);
				if (xDistToEye < gameData.models.nSimpleModelThresholdScale * I2X (2))
#endif
					bOk = DrawPolyModel (pObj, &pObj->info.position.vPos, &pObj->info.position.mOrient,
												pObj->rType.polyObjInfo.animAngles,
												gameData.weapons.info [id].nInnerModel,
												pObj->rType.polyObjInfo.nSubObjFlags,
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
		bOk = DrawPolyModel (pObj, &pObj->info.position.vPos, &pObj->info.position.mOrient,
									pObj->rType.polyObjInfo.animAngles,
									pObj->ModelId (true),
									pObj->rType.polyObjInfo.nSubObjFlags,
									(bGatling || bBrightPolys) ? I2X (1) : xLight,
									xEngineGlow,
									bmiAltTex,
									(bGatling || bEnergyWeapon) ? gameData.weapons.color + id : NULL);
		if (!gameStates.render.bBuildModels) {
			if (!gameOpts->legacy.bRender)
				ogl.SetBlendMode (OGL_BLEND_ALPHA);
			gameStates.render.grAlpha = 1.0f;
			}
		if (bEnergyWeapon)
			glowRenderer.Done (GLOW_OBJECTS);
		}
	}
if (!gameStates.render.bBuildModels) {
	gameStates.render.nInterpolationMethod = imSave;
	gameStates.render.bBrightObject = 0;
	}
return bOk;
}

// -----------------------------------------------------------------------------

static int32_t RenderPlayerModel (CObject* pObj, int32_t bSpectate)
{
if (pObj->AppearanceStage () < 0)
	return 0;
if (automap.Active () && !(OBSERVING || (AM_SHOW_PLAYERS && AM_SHOW_PLAYER (pObj->info.nId))))
	return 0;
tObjTransformation savePos;
if (bSpectate) {
	savePos = pObj->info.position;
	pObj->info.position = gameStates.app.playerPos;
	}
	if (pObj->Appearing ()) {
		fix xScale = F2X (Min (1.0f, (float) pow (1.0f - pObj->AppearanceScale (), 0.25f)));
		gameData.models.vScale.Set (xScale, xScale, xScale);
		}
if (!DrawPolygonObject (pObj, 0)) {
	gameData.models.vScale.SetZero ();
	return 0;
	}
gameData.models.vScale.SetZero ();
gameOpts->ogl.bLightObjects = (gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects;
thrusterFlames.Render (pObj);
RenderPlayerShield (pObj);
RenderTargetIndicator (pObj, NULL);
RenderTowedFlag (pObj);
if (bSpectate)
	pObj->info.position = savePos;
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderRobotModel (CObject* pObj, int32_t bSpectate)
{
if (automap.Active () && !AM_SHOW_ROBOTS)
	return 0;
gameData.models.vScale.SetZero ();
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
#if 1 //!RENDER_HITBOX
if (!DrawPolygonObject (pObj, 0))
	return 0;
#endif
if (pObj->info.controlType) {
	thrusterFlames.Render (pObj);
	if (gameStates.render.nShadowPass != 2) {
		RenderRobotShield (pObj);
		RenderTargetIndicator (pObj, NULL);
		SetRobotLocationInfo (pObj);
		}
	}
#if RENDER_HITBOX
else 
	RenderHitbox (pObj, 0.5f, 0.0f, 0.6f, 0.4f);
#endif
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderReactorModel (CObject* pObj, int32_t bSpectate)
{
if (!DrawPolygonObject (pObj, 0))
	return 0;
if ((gameStates.render.nShadowPass != 2)) {
	RenderRobotShield (pObj);
	RenderTargetIndicator (pObj, NULL);
	}
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderWeaponModel (CObject* pObj, int32_t bSpectate)
{
if (automap.Active () && !AM_SHOW_POWERUPS (1))
	return 0;
if (!(gameStates.app.bNostalgia || gameOpts->Use3DPowerups ()) && pObj->IsMine () && (pObj->info.nId != SMALLMINE_ID))
	ConvertWeaponToVClip (pObj);
else {
	if (pObj->IsMissile ()) {	//make missiles smaller during launch
		if ((pObj->cType.laserInfo.parent.nType == OBJ_PLAYER) &&
			 (gameData.models.renderModels [1][pObj->ModelId ()].m_bValid > 0)) {	//hires player ship
			float dt = X2F (gameData.time.xGame - pObj->CreationTime ());

			if (dt < 1) {
				fix xScale = (fix) (I2X (1) + I2X (1) * dt * dt) / 2;
				gameData.models.vScale.Set (xScale, xScale, xScale);
				}
			}
		//DoObjectSmoke (pObj);
		if (DrawPolygonObject (pObj, 0)) {
#if RENDER_HITBOX
#	if 0
			DrawShieldSphere (pObj, 0.66f, 0.2f, 0.0f, 0.4f, 0);
#	else
			RenderHitbox (pObj, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
			thrusterFlames.Render (pObj);
			}
		gameData.models.vScale.SetZero ();
		}
	else {
#if RENDER_HITBOX
#	if 0
		DrawShieldSphere (pObj, 0.66f, 0.2f, 0.0f, 0.4f, 0);
#	else
		RenderHitbox (pObj, 0.5f, 0.0f, 0.6f, 0.4f);
#	endif
#endif
		if (pObj->info.nType != OBJ_WEAPON) {
			if (!DrawPolygonObject (pObj, 0))
				return 0;
			if (pObj->info.nId != SMALLMINE_ID)
				RenderLightTrail (pObj);
			}
		else {
			if (pObj->IsGatlingRound ()) {
				if (SHOW_OBJ_FX && extraGameInfo [0].bTracers) {
					//RenderLightTrail (pObj);
					CFixVector vSavedPos = pObj->info.position.vPos;
					gameData.models.vScale.Set (I2X (1) / 4, I2X (1) / 4, I2X (3) / 2);
					pObj->info.position.vPos += pObj->info.position.mOrient.m.dir.f;
					DrawPolygonObject (pObj, 0);
					pObj->info.position.vPos = vSavedPos;
					}
				}
			else {
				if (pObj->info.nId != SMALLMINE_ID)
					RenderLightTrail (pObj);
				DrawPolygonObject (pObj, 0);
				}
			gameData.models.vScale.SetZero ();
			}
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderPowerupModel (CObject* pObj, int32_t bSpectate)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 0;
if (automap.Active () && !AM_SHOW_POWERUPS (1))
	return 0;
if (!gameStates.app.bNostalgia && gameOpts->Use3DPowerups ()) {
	RenderPowerupCorona (pObj, 1, 1, 1, coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	if (!DrawPolygonObject (pObj, 0))
		ConvertWeaponToPowerup (pObj);
	else {
		pObj->mType.physInfo.mass = I2X (1);
		pObj->mType.physInfo.drag = 512;
		if (gameOpts->render.powerups.nSpin !=
			((pObj->mType.physInfo.rotVel.v.coord.y | pObj->mType.physInfo.rotVel.v.coord.z) != 0))
			pObj->mType.physInfo.rotVel.v.coord.y =
			pObj->mType.physInfo.rotVel.v.coord.z = gameOpts->render.powerups.nSpin ? I2X (1) / (5 - gameOpts->render.powerups.nSpin) : 0;
		}
#if DBG
	RenderRobotShield (pObj);
#endif
	gameData.models.vScale.SetZero ();
	}
else
	ConvertWeaponToPowerup (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderHostageModel (CObject* pObj, int32_t bSpectate)
{
if (gameStates.app.bNostalgia || !(gameOpts->Use3DPowerups () && DrawPolygonObject (pObj, 0)))
	ConvertModelToHostage (pObj);
#if DBG
RenderRobotShield (pObj);
#endif
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderPolyModel (CObject* pObj, int32_t bSpectate)
{
DrawPolygonObject (pObj, 0);
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) 
	DrawDebrisCorona (pObj);
if (markerManager.IsSpawnObject (pObj))
	RenderMslLockIndicator (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderFireball (CObject* pObj, int32_t bForce)
{
if (gameStates.render.nShadowPass != 2) {
	DrawFireball (pObj);
	if (pObj->info.nType == OBJ_WEAPON) {
		RenderLightTrail (pObj);
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderExplBlast (CObject* pObj, int32_t bForce)
{
if (gameStates.render.nShadowPass != 2)
	DrawExplBlast (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderShockwave (CObject* pObj, int32_t bForce)
{
if (gameStates.render.nShadowPass != 2)
	DrawShockwave (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderShrapnel (CObject* pObj, int32_t bForce)
{
if (gameStates.render.nShadowPass != 2)
	shrapnelManager.Draw (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderWeapon (CObject* pObj, int32_t bForce)
{
if (gameStates.render.nShadowPass != 2) {
	if (automap.Active () && !AM_SHOW_POWERUPS (1))
		return 0;
	if (pObj->info.nType != OBJ_WEAPON)
		DrawWeaponVClip (pObj);
	else {
		if (pObj->IsMine ()) {
			if (!DoObjectSmoke (pObj))
				DrawWeaponVClip (pObj);
			}
		else if ((pObj->info.nId != OMEGA_ID) || !(SHOW_LIGHTNING (1) && gameOpts->render.lightning.bOmega && !gameStates.render.bOmegaModded)) {
			DrawWeaponVClip (pObj);
			if (pObj->info.nId != OMEGA_ID) {
				RenderLightTrail (pObj);
				RenderMslLockIndicator (pObj);
				}
			}
		}
	}
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderHostage (CObject* pObj, int32_t bForce)
{
if (ConvertHostageToModel (pObj))
	DrawPolygonObject (pObj, 0);
else if (gameStates.render.nShadowPass != 2)
	DrawHostage (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderPowerup (CObject* pObj, int32_t bForce)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 0;
if (automap.Active () && !AM_SHOW_POWERUPS (1))
	return 0;
if (pObj->PowerupToDevice ()) {
	RenderPowerupCorona (pObj, 1, 1, 1, coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	DrawPolygonObject (pObj, 0);
	}
else if (gameStates.render.nShadowPass != 2)
	DrawPowerup (pObj);
return 1;
}

// -----------------------------------------------------------------------------

static int32_t RenderLaser (CObject* pObj, int32_t bForce)
{
if (gameStates.render.nShadowPass != 2) {
	RenderLaser (pObj);
	if (pObj->info.nType == OBJ_WEAPON)
		RenderLightTrail (pObj);
	}
return 1;
}

// -----------------------------------------------------------------------------

int32_t RenderObject (CObject *pObj, int32_t nWindow, int32_t bForce)
{
	int16_t			nObject = pObj->Index ();
	int32_t			bSpectate = 0;

int32_t nType = pObj->info.nType;
if (nType == 255) {
	pObj->Die ();
	return 0;
	}
if ((nType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
	return 0;
if ((gameStates.render.nShadowPass != 2) && pObj->IsGuidedMissile ()) {
	return 0;
	}
#if DBG
if (nObject == nDbgObj)
	BRP;
#endif
if (nObject != LOCALPLAYER.nObject) {
	if (pObj == gameData.objData.pViewer)
		return 0;
	 }
else if ((gameData.objData.pViewer == gameData.objData.pConsole) && !automap.Active ()) {
	if ((bSpectate = ((gameStates.render.bFreeCam > 0) && !nWindow)))
		;
#if DBG
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindow || (!gameStates.render.bChaseCam && (gameStates.app.bEndLevelSequence < EL_LOOKBACK)))) { //don't render ship model if neither external view nor main view
#else
	 else if ((gameStates.render.nShadowPass != 2) && !gameStates.app.bPlayerIsDead &&
				 (nWindow ||
				  ((IsMultiGame && !IsCoopGame && (!EGI_FLAG (bEnableCheats, 0, 0, 0) || COMPETITION)) ||
				  (!gameStates.render.bChaseCam && (gameStates.app.bEndLevelSequence < EL_LOOKBACK))))) {
#endif
#if 0
		if (gameOpts->render.particles.bPlayers) {
			SEM_ENTER (SEM_SMOKE)
			DoPlayerSmoke (pObj, -1);
			SEM_LEAVE (SEM_SMOKE)
			}
#endif
		return 0;
		}
	}
if ((nType == OBJ_NONE)/* || (nType==OBJ_CAMBOT)*/){
#if TRACE
	console.printf (1, "ERROR!!!Bogus obj %d in seg %d is rendering!\n", nObject, pObj->info.nSegment);
#endif
	return 0;
	}
int32_t mldSave = gameStates.render.detail.nMaxLinearDepth;
gameStates.render.nState = 1;
gameData.objData.color.index = 0;
gameStates.render.detail.nMaxLinearDepth = gameStates.render.detail.nMaxLinearDepthObjects;

switch (pObj->info.renderType) {
	case RT_NONE:
		break;

	case RT_POLYOBJ:
		if (nType == OBJ_EFFECT) {
			pObj->info.renderType = (pObj->info.nId == PARTICLE_ID) ? RT_PARTICLES : (pObj->info.nId == LIGHTNING_ID) ? RT_LIGHTNING : RT_SOUND;
			return 0;
			}
		if (nType == OBJ_PLAYER) {
			if (!RenderPlayerModel (pObj, bSpectate))
				return 0;
			}
		else if (nType == OBJ_ROBOT) {
			if (!RenderRobotModel (pObj, bSpectate))
				return 0;
			}
		else if (nType == OBJ_WEAPON) {
			if (!RenderWeaponModel (pObj, bSpectate))
				return 0;
			}
		else if (nType == OBJ_REACTOR) {
			if (!RenderReactorModel (pObj, bSpectate))
				return 0;
			}
		else if (nType == OBJ_POWERUP) {
			if (!RenderPowerupModel (pObj, bSpectate))
				return 0;
			}
		else if (nType == OBJ_HOSTAGE) {
			if (!RenderHostageModel (pObj, bSpectate))
				return 0;
			}
		else {
			if (!RenderPolyModel (pObj, bSpectate))
				return 0;
			}
		break;

	case RT_MORPH:
		if (gameStates.render.nShadowPass != 2)
			pObj->MorphDraw ();
		break;

	case RT_THRUSTER:
		if (nWindow && (pObj->mType.physInfo.flags & PF_WIGGLE))
			break;

	case RT_FIREBALL:
		if (!RenderFireball (pObj, bForce))
			return 0;
		break;

	case RT_EXPLBLAST:
		if (!RenderExplBlast (pObj, bForce))
			return 0;
		break;

	case RT_SHOCKWAVE:
		if (!RenderShockwave (pObj, bForce))
			return 0;
		break;

	case RT_SHRAPNELS:
		if (!RenderShrapnel (pObj, bForce))
			return 0;
		break;

	case RT_WEAPON_VCLIP:
		if (!RenderWeapon (pObj, bForce))
			return 0;
		break;

	case RT_HOSTAGE:
		if (!RenderHostage (pObj, bForce))
			return 0;
		break;

	case RT_POWERUP:
		if (!RenderPowerup (pObj, bForce))
			return 0;
		break;

	case RT_LASER:
		if (!RenderLaser (pObj, bForce))
			return 0;
		break;

	case RT_PARTICLES:
	case RT_LIGHTNING:
	case RT_SOUND:
		return 0;
		break;

	default:
		PrintLog (0, "Unknown renderType <%d>\n", pObj->info.renderType);
	}

if (pObj->info.renderType != RT_NONE)
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		if (!gameData.demo.bWasRecorded [nObject]) {
			NDRecordRenderObject (pObj);
			gameData.demo.bWasRecorded [nObject] = 1;
		}
	}

gameStates.render.detail.nMaxLinearDepth = mldSave;
gameData.render.nTotalObjects++;
ogl.ClearError (0);
return 1;
}

//------------------------------------------------------------------------------
//eof
