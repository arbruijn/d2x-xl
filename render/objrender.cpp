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
#include "addon_bitmaps.h"
#include "renderthreads.h"

//------------------------------------------------------------------------------

int16_t PowerupModel (int32_t nId)
{
ENTER (0, 0);
	int16_t nModel;

if ((nModel = PowerupToModel (nId)))
	RETVAL (nModel)
if (0 > (nId = PowerupToObject (nId)))
	RETVAL (-1)
CWeaponInfo *pWeaponInfo = WEAPONINFO (nId);
RETVAL (pWeaponInfo ? pWeaponInfo->nModel : -1)
}

//------------------------------------------------------------------------------

int16_t WeaponModel (CObject *pObj)
{
ENTER (0, 0);
	int16_t	nModel = WeaponToModel (pObj->info.nId);

RETVAL (nModel ? nModel : pObj->ModelId ())
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
	pObj->rType.animationInfo.xFrameTime = gameData.effectData.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
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
ENTER (0, 0);
if (pObj->info.renderType == RT_POLYOBJ)
	RETVAL (1)
if (gameStates.app.bNostalgia || !gameOpts->Use3DPowerups ())
	RETVAL (0)
if (!HaveReplacementModel (HOSTAGE_MODEL))
	RETVAL (0)
pObj->info.renderType = RT_POLYOBJ;
pObj->rType.polyObjInfo.nModel = HOSTAGE_MODEL;
pObj->rType.polyObjInfo.nTexOverride = -1;
pObj->mType.physInfo.rotVel.SetZero ();
memset (pObj->rType.polyObjInfo.animAngles, 0, sizeof (pObj->rType.polyObjInfo.animAngles));
RETVAL (1)
}

// -----------------------------------------------------------------------------

int32_t ConvertModelToHostage (CObject *pObj)
{
ENTER (0, 0);
pObj->rType.animationInfo.nClipIndex = nHostageVClips [0];
pObj->rType.animationInfo.xFrameTime = gameData.effectData.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
pObj->rType.animationInfo.nCurFrame = 0;
pObj->SetSize (289845);
pObj->info.controlType = CT_POWERUP;
pObj->info.renderType = RT_HOSTAGE;
pObj->mType.physInfo.mass = I2X (1);
pObj->mType.physInfo.drag = 512;
RETVAL (1)
}

// -----------------------------------------------------------------------------

void SetupSpin (CObject *pObj, bool bOrient)
{
ENTER (0, 0);
if (gameOpts->render.bPowerupSpinType)
	pObj->mType.physInfo.rotVel.SetZero ();
else {
	if (bOrient && (gameData.demoData.nState != ND_STATE_PLAYBACK)) {
		if (gameData.objData.pViewer)
			pObj->info.position.mOrient = gameData.objData.pViewer->Orientation ();
		else {
			CAngleVector a;
			a.Set (2 * SRandShort (), 2 * SRandShort (), 2 * SRandShort ());
			pObj->info.position.mOrient = CFixMatrix::Create (a);
			}
		}

	static CFixVector vSpin = { I2X (1) / 4, I2X (1) / 4, I2X (1) / 4 };

	if (gameOpts->render.powerups.nSpin)
		pObj->mType.physInfo.rotVel = vSpin;
	else
		pObj->mType.physInfo.rotVel.SetZero ();
	}
RETURN
}

// -----------------------------------------------------------------------------
// Make 3D powerups spin like their 2D counter parts: At an angle of 45 deg around their vertical axis
// Unfortunately, the 3D powerup models have various different orientations that need to be adjusted.
// That sucks. 

void UpdateSpin (CObject *pObj)
{
	CAngleVector	a;
	fix				xRotAngle = (I2X (1) * (SDL_GetTicks () % 2001)) / 2000;

switch (pObj->Id ()) {
	case POW_EXTRA_LIFE:
	case POW_ENERGY:
	case POW_SHIELD_BOOST:
	case POW_HOARD_ORB:
	case POW_CLOAK:
	case POW_INVUL:
		a.Set (0, 0, xRotAngle); // don't tilt
		break;

	case POW_MONSTERBALL:
		return;

	case POW_VULCAN_AMMO:
		a.Set (0, -I2X (1) / 12, xRotAngle); // flipped and pointing down
		break;

	case POW_GAUSS:
		a.Set (-I2X (1) / 3, 0, int16_t ((xRotAngle + I2X (3) / 4) % I2X (1))); // rotated
		break;

	case POW_HOMINGMSL_1:
	case POW_HOMINGMSL_4:
	case POW_SMARTMSL:
	case POW_MEGAMSL:
	case POW_CONCUSSION_1:
	case POW_CONCUSSION_4:
	case POW_FLASHMSL_1:
	case POW_FLASHMSL_4:
	case POW_GUIDEDMSL_1:
	case POW_GUIDEDMSL_4:
	case POW_MERCURYMSL_1:
	case POW_EARTHSHAKER:
		a.Set (-I2X (1) / 12, 0, xRotAngle); // pointing down
		break;

	case POW_AFTERBURNER:
	case POW_AMMORACK:
		a.Set (I2X (1) / 6, -I2X (1) / 24, xRotAngle); // pointing down
		break;

	case POW_KEY_BLUE:
	case POW_KEY_RED:
	case POW_KEY_GOLD:
		a.Set (0, -I2X (1) / 4, xRotAngle); // pointing down
		break;


	case POW_HEADLIGHT:
		a.Set (I2X (1) / 12, -I2X (1) / 64, int16_t ((xRotAngle + I2X (3) / 8) % I2X (1)));
		break;

	case POW_MERCURYMSL_4:
	case POW_QUADLASER:
		a.Set (I2X (1) / 12, 0, int16_t ((xRotAngle + I2X (1) / 2) % I2X (1)));
		break;
#if 0
	case POW_LASER:
	case POW_VULCAN:
	case POW_SPREADFIRE:
	case POW_PLASMA:
	case POW_FUSION:
	case POW_SUPERLASER:
	case POW_HELIX:
	case POW_PHOENIX:
	case POW_OMEGA:
	case POW_PROXMINE:
	case POW_SMARTMINE:
	case POW_BLUEFLAG:
	case POW_REDFLAG:
	case POW_FULL_MAP:
	case POW_CONVERTER:
	case POW_SLOWMOTION:
	case POW_BULLETTIME:
	case POW_QUADLASER:
#endif
	default:
		a.Set (I2X (1) / 6, 0, xRotAngle);
		break;
	}

#if 1
pObj->info.position.mOrient = CFixMatrix::Create (a);
#else
// Make powerups align to player orientation. Makes them look quite fake.
CFixMatrix mOrient = CFixMatrix::Create (a);
pObj->info.position.mOrient = gameData.objData.pViewer->Orientation () * mOrient;
#endif
pObj->mType.physInfo.rotVel.SetZero (); // better safe than sorry ...
}

// -----------------------------------------------------------------------------

int32_t CObject::PowerupToDevice (void)
{
ENTER (0, 0);
if (gameStates.app.bNostalgia ||
	(gameData.demoData.nState == ND_STATE_PLAYBACK && gameData.demoData.bUseShortPos)) // shortpos -> non-xl demo
	RETVAL (0)
if (!gameOpts->Use3DPowerups ())
	RETVAL (0)
if (info.controlType == CT_WEAPON)
	RETVAL (1)
if ((info.nType != OBJ_POWERUP) && (info.nType != OBJ_WEAPON))
	RETVAL (0)

int32_t bHasModel = 0;
int16_t nId, nModel = PowerupToModel (info.nId);

#if DBG
if (Index () == nDbgObj)
	BRP;
#endif

if (nModel)
	nId = info.nId;
else {
	nId = PowerupToObject (info.nId);
	if (nId >= 0) {
		CWeaponInfo *pWeaponInfo = WEAPONINFO (nId);
		if (pWeaponInfo) {
			nModel = pWeaponInfo->nModel;
			bHasModel = (nModel >= 0);
			}
		}
	}
if (!bHasModel && !IsMissile () && !(nModel && HaveReplacementModel (nModel)))
		RETVAL (0)

#if 0 //DBG
Orientation () = gameData.objData.pConsole->Orientation ();
mType.physInfo.rotVel.v.coord.x =
mType.physInfo.rotVel.v.coord.y =
mType.physInfo.rotVel.v.coord.z = 0;
#else
SetupSpin (this, true);
#endif
#if DBG
if (Index () == nDbgObj)
	BRP;
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
RETVAL (1)
}

// -----------------------------------------------------------------------------

void ConvertAllPowerupsToWeapons (void)
{
ENTER (0, 0);
	CObject* pObj;

FORALL_OBJS (pObj)
	if (pObj->info.renderType == RT_POWERUP) {
		pObj->PowerupToDevice ();
		pObj->LoadTextures ();
		}
RETURN
}

// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
void SetRobotLocationInfo (CObject *pObj)
{
if (pObj && (gameStates.app.bPlayerFiredLaserThisFrame != -1)) {
	CFixVector vPos;
	#if 0
	CRenderPoint temp;

	temp.TransformAndEncode (pObj->info.position.vPos);
	if (temp.Behind ())		//robot behind the screen
		return;

	vPos = temp.ViewPos ();
	#else
	CFixMatrix m = transformation.m_info.view [1];
	m.Scale (transformation.m_info.scale);
	vPos = m * (pObj->info.position.vPos - transformation.m_info.pos);

	if (vPos.v.coord.z <= 0) //robot behind the screen
		return;
	#endif

	//the code below to check for CObject near the center of the screen
	//completely ignores z, which may not be good
	if ((abs (vPos.v.coord.x) < I2X (4)) && (abs (vPos.v.coord.y) < I2X (4))) {
		pObj->cType.aiInfo.nDangerLaser = gameStates.app.bPlayerFiredLaserThisFrame;
		pObj->cType.aiInfo.nDangerLaserSig = OBJECT (gameStates.app.bPlayerFiredLaserThisFrame)->info.nSignature;
		}
	}
}

//------------------------------------------------------------------------------

fix CalcObjectLight (CObject *pObj, fix *xEngineGlow)
{
ENTER (0, 0);

	fix xLight;

if (gameStates.render.bBuildModels)
	RETVAL (I2X (1))
if (!pObj)
	RETVAL (0)
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
RETVAL (xLight)
}

//------------------------------------------------------------------------------

static float ObjectBlobColor (CObject *pObj, CBitmap *pBm, CFloatVector *pColor, float fBrightness = 0.0f)
{
	float	fScale;

pBm->GetAvgColor (pColor);
#if DBG
if ((pObj->info.nType == nDbgObjType) && ((nDbgObjId < 0) || (pObj->info.nId == nDbgObjId)))
	BRP;
#endif
if (fBrightness > 0.0f) {
	float h = fBrightness / Max (pColor->Red (), Max (pColor->Green (), pColor->Blue ()));
	pColor->Red () *= h;
	pColor->Green () *= h;
	pColor->Blue () *= h;
	}
fScale = pColor->Red () + pColor->Green () + pColor->Blue ();
if (fScale == 0.0f) {
	pColor->Red () =
	pColor->Green () =
	pColor->Blue () = 1.0f;
	}
return fScale;
}

//------------------------------------------------------------------------------

static CBitmap *PowerupIcon (CObject *pObj)
{
switch (pObj->Id ()) {
	case POW_SHIELD_BOOST:
		return NULL;
	case POW_HOARD_ORB:
	case POW_EXTRA_LIFE:
		return pyroIcon.Bitmap ();
	case POW_INVUL:
		return invulIcon.Bitmap ();
	case POW_CLOAK:
		return cloakIcon.Bitmap ();
	default:
		return NULL;
	}
}

//------------------------------------------------------------------------------

#define ICON_IN_FRONT 0

static int32_t DrawPowerupSphere (CObject* pObj, CBitmap *pBm, CFloatVector& color)
{
ENTER (0, 0);
if (!gameData.renderData.shield)
	RETVAL (0)

	static CPulseData powerupPulse;

if ((pObj->mType.physInfo.velocity.IsZero ()) && (pObj->info.movementType != MT_SPINNING)) {
	pObj->info.movementType = MT_SPINNING;
	pObj->mType.spinRate = pObj->info.position.mOrient.m.dir.u * (I2X (1) / 8);
	}
//the actual shield in the sprite texture has 3/4 of the textures size
gameData.renderData.shield->SetupSurface (&powerupPulse, ((pObj->Id () == POW_SHIELD_BOOST) && !SHOW_LIGHTNING (3)) ? shield [1].Bitmap () : shield [2].Bitmap ());
#if ICON_IN_FRONT
if (pBm)
	transparencyRenderer.AddSprite (pBm, pObj->Position (), &color, 2 * pObj->info.xSize / 3, 2 * pObj->info.xSize / 3, 0, 0, 0.0f);
#endif
// alpha < 0 will cause the sphere to have an outline, additive < 0 will cause the sphere to be blurred
int32_t h = (pObj->Id () == POW_SHIELD_BOOST)
				? DrawShieldSphere (pObj, color.Red (), color.Green (), color.Blue (), 1.0f, 0, 3 * pObj->info.xSize / 4) 
				: DrawShieldSphere (pObj, color.Red (), color.Green (), color.Blue (), -1.0f, 0, 3 * pObj->info.xSize / 4); 
#if !ICON_IN_FRONT
if (h && pBm)
	transparencyRenderer.AddSprite (pBm, pObj->Position (), &color, 2 * pObj->info.xSize / 3, 2 * pObj->info.xSize / 3, 0, 0, 0.0f);
#endif
RETVAL (h)
}

//------------------------------------------------------------------------------
//draw an CObject that has one bitmap & doesn't rotate

void DrawObjectBitmap (CObject *pObj, int32_t bmi0, int32_t bmi, int32_t iFrame, CFloatVector *pColor, float fAlpha)
{
ENTER (0, 0);
	int32_t nType = pObj->info.nType;

if ((nType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
	RETURN

	CBitmap*			pBm;
	CFloatVector	color;
	int32_t			nId = pObj->info.nId;
#if 0
	int32_t			bMuzzleFlash = 0;
#endif
	int32_t			bAdditive = 0, bEnergy = 0;
	fix				xSize;

if ((nType == OBJ_WEAPON) && (pObj->info.nId == OMEGA_ID) && omegaLightning.Exist ())
	RETURN
#if DBG
if ((nType == nDbgObjType) && ((nDbgObjId < 0) || (pObj->info.nId == nDbgObjId)))
	BRP;
#endif
if ((gameOpts->render.textures.bUseHires [0] || gameOpts->render.effects.bTransparent) && gameOpts->render.nImageQuality) {
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
	if (-bmi - 1 >= int32_t (gameData.pigData.tex.addonBitmaps.Length ()))
		RETURN
	pBm = &gameData.pigData.tex.addonBitmaps [-bmi - 1];
	pBm = pBm->SetCurFrame (iFrame);
	}
else {
	CBitmap* pBmo;

	pBm = gameData.pigData.tex.bitmaps [0] + bmi;
	if ((pBm->Type () == BM_TYPE_STD) && (pBmo = pBm->Override ()))
		pBm = pBmo->SetCurFrame (iFrame);
	}
if (!pBm || pBm->Bind (1))
	RETURN

bool b3DShield = ((pObj->Type () == OBJ_POWERUP) && 
					   ((pObj->Id () == POW_SHIELD_BOOST) || (pObj->Id () == POW_INVUL) || (pObj->Id () == POW_CLOAK) || (pObj->Id () == POW_EXTRA_LIFE) || (pObj->Id () == POW_HOARD_ORB)) &&
					   gameOpts->Use3DPowerups () && gameOpts->render.powerups.b3DShields);

ObjectBlobColor (pObj, pBm, &color, b3DShield ? 0.75f : 0.0f);
if (pColor /*&& (bmi >= 0)*/)
	*pColor = color;
	//memcpy (pColor, gameData.pigData.tex.bitmapColors + bmi, sizeof (CFloatVector));

xSize = pObj->info.xSize;

if ((nType == OBJ_POWERUP) && ((bEnergy && gameOpts->render.coronas.bPowerups) || (!bEnergy && gameOpts->render.coronas.bWeapons)))
	RenderPowerupCorona (pObj, color.Red (), color.Green (), color.Blue (), coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
if (b3DShield && DrawPowerupSphere (pObj, PowerupIcon (pObj), color)) {
	//if ((nType == OBJ_POWERUP) && ((bEnergy && gameOpts->render.coronas.bPowerups) || (!bEnergy && gameOpts->render.coronas.bWeapons)))
	//	RenderPowerupCorona (pObj, color.Red (), color.Green (), color.Blue (), coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
		}
else {
	//if ((nType == OBJ_POWERUP) && ((bEnergy && gameOpts->render.coronas.bPowerups) || (!bEnergy && gameOpts->render.coronas.bWeapons)))
	//	RenderPowerupCorona (pObj, color.Red (), color.Green (), color.Blue (), coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
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
	gameData.renderData.nTotalSprites++;
	}
RETURN
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
ENTER (0, 0);
	tCloakInfo	ci = {0, CLOAKED_FADE_LEVEL, I2X (1), I2X (1), I2X (1), 0, 0};
	int32_t		i;

if (!xCloakStartTime && !xCloakEndTime) {
	if (pObj->info.nType == OBJ_PLAYER) {
		xCloakStartTime = PLAYER (pObj->info.nId).cloakTime;
		xCloakEndTime = PLAYER (pObj->info.nId).cloakTime + CLOAK_TIME_MAX;
		}
	else if (pObj->info.nType == OBJ_ROBOT) {
		if (!pObj->IsBoss ()) {
			xCloakStartTime = gameData.timeData.xGame - I2X (10);
			xCloakEndTime = gameData.timeData.xGame + I2X (10);
			}
		else if (0 <= (i = gameData.bossData.Find (pObj->Index ()))) {
			xCloakStartTime = gameData.bossData [i].m_nCloakStartTime;
			xCloakEndTime = gameData.bossData [i].m_nCloakEndTime;
			}
		}
	}

if (xCloakStartTime != 0x7fffffff)
	ci.xTotalTime = xCloakEndTime - xCloakStartTime;
else
	ci.xTotalTime = gameData.timeData.xGame;
if (pObj->info.nType == OBJ_PLAYER) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_PLAYER;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_PLAYER;
	}
else if (pObj->info.nType == OBJ_ROBOT) {
	ci.xFadeinDuration = CLOAK_FADEIN_DURATION_ROBOT;
	ci.xFadeoutDuration = CLOAK_FADEOUT_DURATION_ROBOT;
	}
else
	RETVAL (0)

ci.xDeltaTime = (xCloakStartTime == 0x7fffffff) ? gameData.timeData.xGame : gameData.timeData.xGame - xCloakStartTime;

// only decrease light during first half of cloak initiation time
if (ci.xDeltaTime < ci.xFadeinDuration / 2) {
	ci.xLightScale = FixDiv (ci.xFadeinDuration / 2 - ci.xDeltaTime, ci.xFadeinDuration / 2);
	ci.bFading = -1;
	}
else if (ci.xDeltaTime < ci.xFadeinDuration) // make object transparent during second half
	ci.nFadeValue = X2I (FixDiv (ci.xDeltaTime - ci.xFadeinDuration / 2, ci.xFadeinDuration / 2) * CLOAKED_FADE_LEVEL);
else if ((xCloakStartTime == 0x7fffffff) || (gameData.timeData.xGame < xCloakEndTime - ci.xFadeoutDuration)) { // fully cloaked
	static int32_t nCloakDelta = 0, nCloakDir = 1;
	static fix xCloakTimer = 0;

	//note, if more than one cloaked CObject is visible at once, the pulse rate will change!
	xCloakTimer -= gameData.timeData.xFrame;
	while (xCloakTimer < 0) {
		xCloakTimer += ci.xFadeoutDuration / 12;
		nCloakDelta += nCloakDir;
		if (nCloakDelta == 0 || nCloakDelta == 4)
			nCloakDir = -nCloakDir;
		}
	ci.nFadeValue = CLOAKED_FADE_LEVEL - nCloakDelta;
	}
else if (gameData.timeData.xGame < xCloakEndTime - ci.xFadeoutDuration / 2)
	ci.nFadeValue = X2I (FixDiv (ci.xTotalTime - ci.xFadeoutDuration / 2 - ci.xDeltaTime, ci.xFadeoutDuration / 2) * CLOAKED_FADE_LEVEL);
else {
	ci.xLightScale = (fix) ((float) (ci.xFadeoutDuration / 2 - (ci.xTotalTime - ci.xDeltaTime) / (float) (ci.xFadeoutDuration / 2)));
	ci.bFading = 1;
	}
if (ciP)
	*ciP = ci;
RETVAL (ci.bFading)
}

//------------------------------------------------------------------------------
//do special cloaked render
int32_t DrawCloakedObject (CObject *pObj, fix light, fix *glow, fix xCloakStartTime, fix xCloakEndTime)
{
ENTER (0, 0);
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
	gameData.modelData.nLightScale = ci.xLightScale;
	bOk = DrawPolyModel (pObj, &pPos->vPos, &pPos->mOrient,
								reinterpret_cast<CAngleVector*> (&pObj->rType.polyObjInfo.animAngles),
								pObj->ModelId (true), pObj->rType.polyObjInfo.nSubObjFlags,
								xNewLight, glow, altTextures, NULL);
	gameData.modelData.nLightScale = 0;
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
RETVAL (bOk)
}

//------------------------------------------------------------------------------

static inline int32_t ObjectIsCloaked (CObject *pObj)
{
return gameStates.render.bBuildModels ? 0 : pObj->Cloaked ();
}

//------------------------------------------------------------------------------

int32_t DrawHiresObject (CObject *pObj, fix xLight, fix *xEngineGlow)
{
ENTER (0, 0);
	float			fLight [3];
	int16_t		nModel = 0;
	OOF::CModel	*po;

#if 0
if (gameStates.render.bLoResShadows && (gameStates.render.nShadowPass == 2))
	RETVAL (0)
#endif
if (pObj->info.nType == OBJ_DEBRIS)
	RETVAL (0)
else if ((pObj->info.nType == OBJ_POWERUP) || (pObj->info.nType == OBJ_WEAPON)) {
	if (pObj->info.nType == OBJ_POWERUP)
		nModel = PowerupModel (pObj->info.nId);
	else if (pObj->info.nType == OBJ_WEAPON)
		nModel = WeaponModel (pObj);
	if (!nModel)
		RETVAL (0)
	}
else
	nModel = pObj->ModelId ();
if (!(po = GetOOFModel (nModel)))
	RETVAL (0)
if (gameData.modelData.renderModels [1][nModel].m_bValid >= 0)
	RETVAL (0)
//G3RenderModel (pObj, nModel, NULL, NULL, NULL, xLight, NULL, color);
fLight [0] = xLight / 65536.0f;
fLight [1] = (float) xEngineGlow [0] / 65536.0f;
fLight [2] = (float) xEngineGlow [1] / 65536.0f;
po->Render (pObj, fLight, ObjectIsCloaked (pObj));
RETVAL (1)
}

//------------------------------------------------------------------------------
//draw an CObject which renders as a polygon model
#define MAX_MODEL_TEXTURES 63

int32_t DrawPolygonObject (CObject *pObj, int32_t bForce)
{
ENTER (0, 0);
	fix		xLight;
	int32_t	imSave = 0;
	fix		xEngineGlow [2];		//element 0 is for engine glow, 1 for headlight
	int32_t	bBlendPolys = 0;
	int32_t	bBrightPolys = 0;
	int32_t	bGatling = 0;
	int32_t	bCloaked = ObjectIsCloaked (pObj);
	int32_t	bEnergyWeapon;
	int32_t	i, id, bOk = 0;

if (pObj->info.nType == 255)
	RETVAL (0)
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
	RETVAL (1)
if (gameStates.render.bBuildModels)
	xLight = I2X (1);
else {
	xLight = CalcObjectLight (pObj, xEngineGlow);
	if ((gameStates.render.nType != RENDER_TYPE_TRANSPARENCY) && (bCloaked || bEnergyWeapon) && (gameStates.render.nShadowPass != 2)) {
		transparencyRenderer.AddObject (pObj);
		RETVAL (1)
		}
	if (DrawHiresObject (pObj, xLight, xEngineGlow))
		RETVAL (1)
	gameStates.render.bBrightObject = bEnergyWeapon;
	imSave = gameStates.render.nInterpolationMethod;
	if (bLinearTMapPolyObjs)
		gameStates.render.nInterpolationMethod = 1;
	}
if (pObj->rType.polyObjInfo.nTexOverride != -1) {
#if DBG
	CPolyModel* pm = gameData.modelData.polyModels [0] + pObj->ModelId ();
#endif
	tBitmapIndex	bm = gameData.pigData.tex.bmIndex [0][pObj->rType.polyObjInfo.nTexOverride],
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
				bOk = DrawCloakedObject (pObj, xLight, xEngineGlow, gameData.timeData.xGame - I2X (10), gameData.timeData.xGame + I2X (10));
			else if (0 <= (i = gameData.bossData.Find (pObj->Index ())))
				bOk = DrawCloakedObject (pObj, xLight, xEngineGlow, gameData.bossData [i].m_nCloakStartTime, gameData.bossData [i].m_nCloakEndTime);
			}
		}
	else {
		tBitmapIndex *bmiAltTex = (pObj->rType.polyObjInfo.nAltTextures > 0) ? mpTextureIndex [pObj->rType.polyObjInfo.nAltTextures - 1] : NULL;

		//	Snipers get bright when they fire.
		if (!gameStates.render.bBuildModels) {
			if ((pObj->info.nType == OBJ_ROBOT) &&
				 (gameData.aiData.localInfo [pObj->Index ()].pNextrimaryFire < I2X (1) / 8) &&
				 (pObj->cType.aiInfo.behavior == AIB_SNIPE))
				xLight = 2 * xLight + I2X (1);
			if (pObj->IsWeapon ()) {
				CWeaponInfo *pWeaponInfo = WEAPONINFO (id);
				bBlendPolys = bEnergyWeapon && pWeaponInfo && (pWeaponInfo->nInnerModel > -1);
				bBrightPolys = bGatling || (bBlendPolys && WI_EnergyUsage (id));
				if (bEnergyWeapon) {
					if (gameOpts->legacy.bRender)
						gameStates.render.grAlpha = GrAlpha (FADE_LEVELS - 2);
					else {
						ogl.SetBlendMode (OGL_BLEND_ADD);
						glowRenderer.Begin (GLOW_OBJECTS, 2, false, 1.0f);
						}
					}
				if (bBlendPolys) {
					bOk = DrawPolyModel (pObj, &pObj->info.position.vPos, &pObj->info.position.mOrient,
												pObj->rType.polyObjInfo.animAngles,
												pWeaponInfo ? pWeaponInfo->nInnerModel : -1,
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
			}
		bOk = DrawPolyModel (pObj, &pObj->info.position.vPos, &pObj->info.position.mOrient,
									pObj->rType.polyObjInfo.animAngles,
									pObj->ModelId (true),
									pObj->rType.polyObjInfo.nSubObjFlags,
									(bGatling || bBrightPolys) ? I2X (1) : xLight,
									xEngineGlow,
									bmiAltTex,
									(bGatling || bEnergyWeapon) ? gameData.weaponData.color + id : NULL);
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
RETVAL (bOk)
}

// -----------------------------------------------------------------------------

static int32_t RenderPlayerModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
if (pObj->AppearanceStage () < 0)
	RETVAL (0)
if (automap.Active () && !(OBSERVING || (AM_SHOW_PLAYERS && AM_SHOW_PLAYER (pObj->info.nId))))
	RETVAL (0)
tObjTransformation savePos;
if (bSpectate) {
	savePos = pObj->info.position;
	pObj->info.position = gameStates.app.playerPos;
	}
	if (pObj->Appearing ()) {
		fix xScale = F2X (Min (1.0f, (float) pow (1.0f - pObj->AppearanceScale (), 0.25f)));
		gameData.modelData.vScale.Set (xScale, xScale, xScale);
		}
if (!DrawPolygonObject (pObj, 0)) {
	gameData.modelData.vScale.SetZero ();
	RETVAL (0)
	}
gameData.modelData.vScale.SetZero ();
gameOpts->ogl.bLightObjects = (gameOpts->ogl.bObjLighting) || gameOpts->ogl.bLightObjects;
thrusterFlames.Render (pObj);
RenderPlayerShield (pObj);
RenderTargetIndicator (pObj, NULL);
RenderTowedFlag (pObj);
if (bSpectate)
	pObj->info.position = savePos;
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderRobotModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
if (automap.Active () && !AM_SHOW_ROBOTS)
	RETVAL (0)
gameData.modelData.vScale.SetZero ();
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
#if 1 //!RENDER_HITBOX
if (!DrawPolygonObject (pObj, 0))
	RETVAL (0)
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
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderReactorModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
if (!DrawPolygonObject (pObj, 0))
	RETVAL (0)
if ((gameStates.render.nShadowPass != 2)) {
	RenderRobotShield (pObj);
	RenderTargetIndicator (pObj, NULL);
	}
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderWeaponModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
if (automap.Active () && !AM_SHOW_POWERUPS (1))
	RETVAL (0)
if (!(gameStates.app.bNostalgia || gameOpts->Use3DPowerups ()) && pObj->IsMine () && (pObj->info.nId != SMALLMINE_ID))
	ConvertWeaponToVClip (pObj);
else {
	if (pObj->IsMissile ()) {	//make missiles smaller during launch
		if ((pObj->cType.laserInfo.parent.nType == OBJ_PLAYER) &&
			 (gameData.modelData.renderModels [1][pObj->ModelId ()].m_bValid > 0)) {	//hires player ship
			float dt = X2F (gameData.timeData.xGame - pObj->CreationTime ());

			if (dt < 1) {
				fix xScale = (fix) (I2X (1) + I2X (1) * dt * dt) / 2;
				gameData.modelData.vScale.Set (xScale, xScale, xScale);
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
		gameData.modelData.vScale.SetZero ();
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
				RETVAL (0)
			if (pObj->info.nId != SMALLMINE_ID)
				RenderLightTrail (pObj);
			}
		else {
			if (pObj->IsGatlingRound ()) {
				if (SHOW_OBJ_FX && extraGameInfo [0].bTracers) {
					//RenderLightTrail (pObj);
					CFixVector vSavedPos = pObj->info.position.vPos;
					gameData.modelData.vScale.Set (I2X (1) / 4, I2X (1) / 4, I2X (3) / 2);
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
			gameData.modelData.vScale.SetZero ();
			}
		}
	}
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderPowerupModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	RETVAL (0)
if (automap.Active () && !AM_SHOW_POWERUPS (1))
	RETVAL (0)
if (!gameStates.app.bNostalgia && gameOpts->Use3DPowerups ()) {
	RenderPowerupCorona (pObj, 1, 1, 1, coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	if (!DrawPolygonObject (pObj, 0))
		ConvertWeaponToPowerup (pObj);
	else {
		pObj->mType.physInfo.mass = I2X (1);
		pObj->mType.physInfo.drag = 512;
		int32_t bSpinning = !pObj->mType.physInfo.rotVel.IsZero ();
		if (gameOpts->render.powerups.nSpin !=	bSpinning) 
			SetupSpin (pObj, false);
		if (gameOpts->render.bPowerupSpinType)
			UpdateSpin (pObj);
		}
#if DBG
	RenderRobotShield (pObj);
#endif
	gameData.modelData.vScale.SetZero ();
	}
else
	ConvertWeaponToPowerup (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderHostageModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
if (gameStates.app.bNostalgia || !(gameOpts->Use3DPowerups () && DrawPolygonObject (pObj, 0)))
	ConvertModelToHostage (pObj);
#if DBG
RenderRobotShield (pObj);
#endif
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderPolyModel (CObject* pObj, int32_t bSpectate)
{
ENTER (0, 0);
DrawPolygonObject (pObj, 0);
if (!(SHOW_SMOKE && gameOpts->render.particles.bDebris)) 
	DrawDebrisCorona (pObj);
if (markerManager.IsSpawnObject (pObj))
	RenderMslLockIndicator (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderFireball (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.render.nShadowPass != 2) {
	DrawFireball (pObj);
	if (pObj->info.nType == OBJ_WEAPON) {
		RenderLightTrail (pObj);
		}
	}
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderExplBlast (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.render.nShadowPass != 2)
	DrawExplBlast (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderShockwave (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.render.nShadowPass != 2)
	DrawShockwave (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderShrapnel (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.render.nShadowPass != 2)
	shrapnelManager.Draw (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderWeapon (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.render.nShadowPass != 2) {
	if (automap.Active () && !AM_SHOW_POWERUPS (1))
		RETVAL (0)
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
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderHostage (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (ConvertHostageToModel (pObj))
	DrawPolygonObject (pObj, 0);
else if (gameStates.render.nShadowPass != 2)
	DrawHostage (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderPowerup (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	RETVAL (0)
if (automap.Active () && !AM_SHOW_POWERUPS (1))
	RETVAL (0)
if (pObj->PowerupToDevice ()) {
	RenderPowerupCorona (pObj, 1, 1, 1, coronaIntensities [gameOpts->render.coronas.nObjIntensity]);
	DrawPolygonObject (pObj, 0);
	}
else if (gameStates.render.nShadowPass != 2)
	DrawPowerup (pObj);
RETVAL (1)
}

// -----------------------------------------------------------------------------

static int32_t RenderLaser (CObject* pObj, int32_t bForce)
{
ENTER (0, 0);
if (gameStates.render.nShadowPass != 2) {
	RenderLaser (pObj);
	if (pObj->info.nType == OBJ_WEAPON)
		RenderLightTrail (pObj);
	}
RETVAL (1)
}

// -----------------------------------------------------------------------------

int32_t RenderObject (CObject *pObj, int32_t nWindow, int32_t bForce)
{
ENTER (0, 0);
	int16_t			nObject = pObj->Index ();
	int32_t			bSpectate = 0;

int32_t nType = pObj->info.nType;
if (nType == 255) {
	pObj->Die ();
	RETVAL (0)
	}
if ((nType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
	RETVAL (0)
if ((gameStates.render.nShadowPass != 2) && pObj->IsGuidedMissile ()) {
	RETVAL (0)
	}
#if DBG
if (nObject == nDbgObj)
	BRP;
#endif
if (nObject != LOCALPLAYER.nObject) {
	if (pObj == gameData.objData.pViewer)
		RETVAL (0)
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
		RETVAL (0)
		}
	}
if ((nType == OBJ_NONE)/* || (nType==OBJ_CAMBOT)*/){
#if TRACE
	console.printf (1, "ERROR!!!Bogus obj %d in seg %d is rendering!\n", nObject, pObj->info.nSegment);
#endif
	RETVAL (0)
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
			RETVAL (0)
			}
		if (nType == OBJ_PLAYER) {
			if (!RenderPlayerModel (pObj, bSpectate))
				RETVAL (0)
			}
		else if (nType == OBJ_ROBOT) {
			if (!RenderRobotModel (pObj, bSpectate))
				RETVAL (0)
			}
		else if (nType == OBJ_WEAPON) {
			if (!RenderWeaponModel (pObj, bSpectate))
				RETVAL (0)
			}
		else if (nType == OBJ_REACTOR) {
			if (!RenderReactorModel (pObj, bSpectate))
				RETVAL (0)
			}
		else if (nType == OBJ_POWERUP) {
			if (!RenderPowerupModel (pObj, bSpectate))
				RETVAL (0)
			}
		else if (nType == OBJ_HOSTAGE) {
			if (!RenderHostageModel (pObj, bSpectate))
				RETVAL (0)
			}
		else {
			if (!RenderPolyModel (pObj, bSpectate))
				RETVAL (0)
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
			RETVAL (0)
		break;

	case RT_EXPLBLAST:
		if (!RenderExplBlast (pObj, bForce))
			RETVAL (0)
		break;

	case RT_SHOCKWAVE:
		if (!RenderShockwave (pObj, bForce))
			RETVAL (0)
		break;

	case RT_SHRAPNELS:
		if (!RenderShrapnel (pObj, bForce))
			RETVAL (0)
		break;

	case RT_WEAPON_VCLIP:
		if (!RenderWeapon (pObj, bForce))
			RETVAL (0)
		break;

	case RT_HOSTAGE:
		if (!RenderHostage (pObj, bForce))
			RETVAL (0)
		break;

	case RT_POWERUP:
		if (!RenderPowerup (pObj, bForce))
			RETVAL (0)
		break;

	case RT_LASER:
		if (!RenderLaser (pObj, bForce))
			RETVAL (0)
		break;

	case RT_PARTICLES:
	case RT_LIGHTNING:
	case RT_SOUND:
		RETVAL (0)
		break;

	default:
		PrintLog (0, "Unknown renderType <%d>\n", pObj->info.renderType);
	}

if (pObj->info.renderType != RT_NONE)
	if (gameData.demoData.nState == ND_STATE_RECORDING) {
		if (!gameData.demoData.bWasRecorded [nObject]) {
			NDRecordRenderObject (pObj);
			gameData.demoData.bWasRecorded [nObject] = 1;
		}
	}

gameStates.render.detail.nMaxLinearDepth = mldSave;
gameData.renderData.nTotalObjects++;
ogl.ClearError (0);
RETVAL (1)
}

//------------------------------------------------------------------------------
//eof
