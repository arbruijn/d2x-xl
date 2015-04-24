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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "descent.h"
#include "error.h"
#include "objrender.h"
#include "key.h"
#include "input.h"
#include "fireball.h"
#include "cockpit.h"
#include "text.h"
#include "scores.h"
#include "light.h"
#include "kconfig.h"
#include "rendermine.h"
#include "newdemo.h"
#include "objeffects.h"
#include "sphere.h"

int32_t ReturnFlagHome (CObject *pObj);
void InvalidateEscortGoal (void);
void MultiSendGotFlag (uint8_t);

const char *pszPowerup [MAX_POWERUP_TYPES] = {
	"extra life",
	"energy",
	"shield",
	"laser",
	"blue key",
	"red key",
	"gold key",
	"hoard orb",
	"monsterball",
	"",
	"concussion missile",
	"concussion missile (4)",
	"quad laser",
	"vulcan gun",
	"spreadfire gun",
	"plasma gun",
	"fusion gun",
	"prox mine",
	"homing missile",
	"homing missile (4)",
	"smart missile",
	"mega missile",
	"vulcan ammo",
	"cloak",
	"turbo",
	"invul",
	"",
	"megawow",
	"gauss cannon",
	"helix cannon",
	"phoenix cannon",
	"omega cannon",
	"super laser",
	"full map",
	"converter",
	"ammo rack",
	"after burner",
	"headlight",
	"flash missile",
	"flash missile (4)",
	"guided missile",
	"guided missile (4)",
	"smart mine",
	"mercury missile",
	"mercury missile (4)",
	"earth shaker",
	"blue flag",
	"red flag",
	"slow motion",
	"bullet time"
	};

#define	MAX_INV_ITEMS	((5 - gameStates.app.nDifficultyLevel) * ((playerP->flags & PLAYER_FLAGS_AMMO_RACK) ? 2 : 1))

int32_t powerupToDevice [MAX_POWERUP_TYPES];
char powerupToWeaponCount [MAX_POWERUP_TYPES];
char powerupClass [MAX_POWERUP_TYPES];
char powerupToObject [MAX_POWERUP_TYPES];
int16_t powerupToModel [MAX_POWERUP_TYPES];
int16_t weaponToModel [MAX_WEAPON_TYPES];
uint8_t powerupType [MAX_POWERUP_TYPES];
uint8_t powerupFilter [MAX_POWERUP_TYPES];
void *pickupHandler [MAX_POWERUP_TYPES];

//------------------------------------------------------------------------------

static int32_t nDbgMinFrame = 0;

void UpdatePowerupClip (tAnimationInfo *animInfoP, tAnimationState *vciP, int32_t nObject)
{
if (animInfoP) {
	static fix	xPowerupTime = 0;
	int32_t		h, nFrames = SetupHiresVClip (animInfoP, vciP);
	fix			xTime, xFudge = (xPowerupTime * (nObject & 3)) >> 4;

	xPowerupTime += gameData.physics.xTime;
	xTime = vciP->xFrameTime - (fix) ((xPowerupTime + xFudge) / gameStates.gameplay.slowmo [0].fSpeed);
	if ((xTime < 0) && (animInfoP->xFrameTime > 0)) {
		h = (-xTime + animInfoP->xFrameTime - 1) / animInfoP->xFrameTime;
		xTime += h * animInfoP->xFrameTime;
		h %= nFrames;
		if ((nObject & 1) && (OBJECT (nObject)->info.nType != OBJ_EXPLOSION)) 
			vciP->nCurFrame -= h;
		else
			vciP->nCurFrame += h;
		if (vciP->nCurFrame < nDbgMinFrame)
			vciP->nCurFrame = nFrames - (-vciP->nCurFrame % nFrames);
		else 
			vciP->nCurFrame %= nFrames;
		}
#if DBG
	if (vciP->nCurFrame < nDbgMinFrame)
		vciP->nCurFrame = nDbgMinFrame;
#endif
	vciP->xFrameTime = xTime;
	xPowerupTime = 0;
	}
else {
	int32_t	h, nFrames;

	if (0 > (h = (gameStates.app.nSDLTicks [0] - vciP->xTotalTime))) {
		vciP->xTotalTime = gameStates.app.nSDLTicks [0];
		h = 0;
		}
	else if ((h = h / 80) && (nFrames = gameData.pig.tex.addonBitmaps [-vciP->nClipIndex - 1].FrameCount ())) { //???
		vciP->xTotalTime += h * 80;
		if (gameStates.app.nSDLTicks [0] < vciP->xTotalTime)
			vciP->xTotalTime = gameStates.app.nSDLTicks [0];
		if (nObject & 1)
			vciP->nCurFrame += h;
		else
			vciP->nCurFrame -= h;
		if (vciP->nCurFrame < 0) {
			if (!(h = -vciP->nCurFrame % nFrames))
				h = 1;
			vciP->nCurFrame = nFrames - h;
			}
		else 
			vciP->nCurFrame %= nFrames;
		}
#if DBG
	if (vciP->nCurFrame < nDbgMinFrame)
		vciP->nCurFrame = nDbgMinFrame;
#endif
	}
}

//------------------------------------------------------------------------------

void UpdateFlagClips (void)
{
if (!gameStates.app.bDemoData) {
	UpdatePowerupClip (gameData.pig.flags [0].animInfoP, &gameData.pig.flags [0].animState, 0);
	UpdatePowerupClip (gameData.pig.flags [1].animInfoP, &gameData.pig.flags [1].animState, 0);
	}
}

//------------------------------------------------------------------------------
//process this powerup for this frame
void CObject::DoPowerupFrame (void)
{
	int32_t	i = OBJ_IDX (this);
//if (gameStates.app.tick40fps.bTick) 
if (info.renderType != RT_POLYOBJ) {
	tAnimationState	*vciP = &rType.animationInfo;
	tAnimationInfo	*animInfoP = ((vciP->nClipIndex < 0) || (vciP->nClipIndex >= MAX_ANIMATIONS_D2)) ? NULL : gameData.effects.animations [0] + vciP->nClipIndex;
	UpdatePowerupClip (animInfoP, vciP, i);
	}
if (info.xLifeLeft <= 0) {
	CreateExplosion (this, info.nSegment, info.position.vPos, info.position.vPos, I2X (7) / 2, ANIM_POWERUP_DISAPPEARANCE);
	if (gameData.effects.animations [0][ANIM_POWERUP_DISAPPEARANCE].nSound > -1)
		audio.CreateObjectSound (gameData.effects.animations [0][ANIM_POWERUP_DISAPPEARANCE].nSound, SOUNDCLASS_GENERIC, i);
	}
}

//------------------------------------------------------------------------------

void DrawPowerup (CObject *objP)
{
#if DBG
//return;
#endif
if (objP->info.nType == OBJ_MONSTERBALL) {
	DrawMonsterball (objP, 1.0f, 0.5f, 0.0f, 0.9f);
	RenderMslLockIndicator (objP);
	}
else if ((objP->rType.animationInfo.nClipIndex >= -MAX_ADDON_BITMAP_FILES) && (objP->rType.animationInfo.nClipIndex < MAX_ANIMATIONS_D2)) {
	if ((objP->info.nId < MAX_POWERUP_TYPES_D2) || ((objP->info.nType == OBJ_EXPLOSION) && (objP->info.nId < MAX_ANIMATIONS_D2))) {
			tBitmapIndex*	frameP = gameData.effects.animations [0][objP->rType.animationInfo.nClipIndex].frames;
			int32_t			iFrame = objP->rType.animationInfo.nCurFrame;
		DrawObjectBitmap (objP, frameP->index, frameP [iFrame].index, iFrame, NULL, 0);
		}
	else {
		DrawObjectBitmap (objP, objP->rType.animationInfo.nClipIndex, objP->rType.animationInfo.nClipIndex, objP->rType.animationInfo.nCurFrame, NULL, 1);
		}
	}
#if DBG
else
	PrintLog (0, "invalid powerup clip index\n");
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ PowerupBasic (int32_t redAdd, int32_t greenAdd, int32_t blueAdd, int32_t score, const char *format, ...)
{
	char		text[120];
	va_list	args;

va_start (args, format);
vsprintf (text, format, args);
va_end (args);
paletteManager.BumpEffect (redAdd, greenAdd, blueAdd);
HUDInitMessage (text);
//mprintf_gameData.objs.pwrUp.Info ();
cockpit->AddPointsToScore (score);
}

//------------------------------------------------------------------------------

//#if DBG
//	Give the megawow powerup!
void DoMegaWowPowerup (int32_t quantity)
{
	int32_t i;

PowerupBasic (30, 0, 30, 1, "MEGA-WOWIE-ZOWIE!");
LOCALPLAYER.primaryWeaponFlags = 0xffff ^ HAS_FLAG (SUPER_LASER_INDEX);		//no super laser
LOCALPLAYER.secondaryWeaponFlags = 0xffff;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	LOCALPLAYER.primaryAmmo[i] = VULCAN_AMMO_MAX;
for (i = 0; i < 3; i++)
	LOCALPLAYER.secondaryAmmo[i] = quantity;
for (i = 3; i < MAX_SECONDARY_WEAPONS; i++)
	LOCALPLAYER.secondaryAmmo[i] = quantity/5;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLaserLevel (LOCALPLAYER.LaserLevel (), MAX_LASER_LEVEL);
LOCALPLAYER.SetEnergy (I2X (200));
LOCALPLAYER.SetShield (LOCALPLAYER.MaxShield ());
LOCALPLAYER.flags |= PLAYER_FLAGS_QUAD_LASERS;
LOCALPLAYER.SetSuperLaser (MAX_SUPERLASER_LEVEL - MAX_LASER_LEVEL);
if (IsHoardGame)
	LOCALPLAYER.secondaryAmmo[PROXMINE_INDEX] = 12;
else if (IsEntropyGame)
	LOCALPLAYER.secondaryAmmo[PROXMINE_INDEX] = 15;
cockpit->UpdateLaserWeaponInfo ();
}
//#endif

//------------------------------------------------------------------------------

int32_t PickupEnergyBoost (CObject *objP, int32_t nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

if (playerP->Energy () < playerP->MaxEnergy ()) {
	fix boost = I2X (3) * (NDL - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	playerP->UpdateEnergy (boost);
	if (ISLOCALPLAYER (nPlayer))
		PowerupBasic (15,15,7, ENERGY_SCORE, "%s %s %d", TXT_ENERGY, TXT_BOOSTED_TO, X2IR (playerP->energy));
	return 1;
	} 
else if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_ENERGY);
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupShieldBoost (CObject *objP, int32_t nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

if (playerP->Shield () < playerP->MaxShield ()) {
	fix boost = I2X (3) * (NDL - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	playerP->UpdateShield (boost);
	if (ISLOCALPLAYER (nPlayer)) {
		PowerupBasic (0, 0, 15, SHIELD_SCORE, "%s %s %d", TXT_SHIELD, TXT_BOOSTED_TO, X2IR (playerP->Shield ()));
		NetworkFlushData (); // will send position, shield and weapon info
		}
	OBJECT (nPlayer)->ResetDamage ();
	return 1;
	}
else if (ISLOCALPLAYER (nPlayer)) {
	if (OBJECT (N_LOCALPLAYER)->ResetDamage ())
		return 1;
	else
		HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
	}
else
	OBJECT (nPlayer)->ResetDamage ();
return 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupCloakingDevice (CObject *objP, int32_t nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (!gameOpts->gameplay.bInventory || (IsMultiGame && !IsCoopGame)) 
	return -ApplyCloak (1, nPlayer);
if (playerP->nCloaks < MAX_INV_ITEMS) {
	playerP->nCloaks++;
	PowerupBasic (0, 0, 0, 0, "%s!", TXT_CLOAKING_DEVICE);
	return 1;
	}
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s", TXT_INVENTORY_FULL);
return 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupInvulnerability (CObject *objP, int32_t nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (!gameOpts->gameplay.bInventory || (IsMultiGame && !IsCoopGame)) 
	return -ApplyInvul (1, nPlayer);
if (playerP->nInvuls < MAX_INV_ITEMS) {
	playerP->nInvuls++;
	PowerupBasic (0, 0, 0, 0, "%s!", TXT_INVULNERABILITY);
	return 1;
	}
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s", TXT_INVENTORY_FULL);
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupExtraLife (CObject *objP, int32_t nPlayer)
{
PLAYER (nPlayer).lives++;
if (nPlayer == N_LOCALPLAYER)
	PowerupBasic (15, 15, 15, 0, TXT_EXTRA_LIFE);
return 1;
}

//	-----------------------------------------------------------------------------

int32_t PickupHoardOrb (CObject *objP, int32_t nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (IsHoardGame) {
	if (playerP->secondaryAmmo [PROXMINE_INDEX] < 12) {
		if (ISLOCALPLAYER (nPlayer)) {
			MultiSendGotOrb (N_LOCALPLAYER);
			PowerupBasic (15, 0, 15, 0, "Orb!!!", nPlayer);
			}
		playerP->secondaryAmmo [PROXMINE_INDEX]++;
		playerP->flags |= PLAYER_FLAGS_FLAG;
		return 1;
		}
	}
else if (IsEntropyGame) {
	if (objP->info.nCreator != GetTeam (N_LOCALPLAYER) + 1) {
		if ((extraGameInfo [1].entropy.nVirusStability < 2) ||
			 ((extraGameInfo [1].entropy.nVirusStability < 3) && 
			 ((SEGMENT (objP->info.nSegment)->m_owner != objP->info.nCreator) ||
			 (SEGMENT (objP->info.nSegment)->m_function != SEGMENT_FUNC_ROBOTMAKER))))
			objP->Die ();	//make orb disappear if touched by opposing team CPlayerData
		}
	else if (!extraGameInfo [1].entropy.nMaxVirusCapacity ||
				(playerP->secondaryAmmo [PROXMINE_INDEX] < playerP->secondaryAmmo [SMARTMINE_INDEX])) {
		if (ISLOCALPLAYER (nPlayer)) {
			MultiSendGotOrb (N_LOCALPLAYER);
			PowerupBasic (15, 0, 15, 0, "Virus!!!", nPlayer);
			}
		playerP->secondaryAmmo [PROXMINE_INDEX]++;
		playerP->flags |= PLAYER_FLAGS_FLAG;
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupEquipment (CObject *objP, int32_t nEquipment, const char *pszHave, const char *pszGot, int32_t nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;
	int32_t		id, bUsed = 0;

if (playerP->flags & nEquipment) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, pszHave);
	if (!IsMultiGame)
		bUsed = PickupEnergyBoost (objP, nPlayer);
	} 
else {
	playerP->flags |= nEquipment;
	if (ISLOCALPLAYER (nPlayer)) {
		id = objP->info.nId;
		if (id >= MAX_POWERUP_TYPES_D2)
			id = POW_AFTERBURNER;
		MultiSendPlaySound (gameData.objData.pwrUp.info [id].hitSound, I2X (1));
		audio.PlaySound ((int16_t) gameData.objData.pwrUp.info [id].hitSound);
		PowerupBasic (15, 0, 15, 0, pszGot, nPlayer);
		}
	bUsed = -1;
	}
return bUsed;
}

//	-----------------------------------------------------------------------------

int32_t PickupHeadlight (CObject *objP, int32_t nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;
	char		szTemp [50];

sprintf (szTemp, TXT_GOT_HEADLIGHT, (EGI_FLAG (headlight.bAvailable, 0, 0, 1) && gameOpts->gameplay.bHeadlightOnWhenPickedUp) ? TXT_ON : TXT_OFF);
HUDInitMessage (szTemp);
int32_t bUsed = PickupEquipment (objP, PLAYER_FLAGS_HEADLIGHT, TXT_THE_HEADLIGHT, szTemp, nPlayer);
if (bUsed >= 0)
	return bUsed;
if (ISLOCALPLAYER (nPlayer)) {
	if (EGI_FLAG (headlight.bAvailable, 0, 0, 1)  && gameOpts->gameplay.bHeadlightOnWhenPickedUp)
		playerP->flags |= PLAYER_FLAGS_HEADLIGHT_ON;
	if IsMultiGame
		MultiSendFlags (N_LOCALPLAYER);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t PickupFullMap (CObject *objP, int32_t nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_FULLMAP, TXT_THE_FULLMAP, TXT_GOT_FULLMAP, nPlayer) ? 1 : 0;
}


//	-----------------------------------------------------------------------------

int32_t PickupConverter (CObject *objP, int32_t nPlayer)
{
	char		szTemp [50];

sprintf (szTemp, TXT_GOT_CONVERTER, KeyToASCII (controls.GetKeyValue (56)));
HUDInitMessage (szTemp);
return PickupEquipment (objP, PLAYER_FLAGS_CONVERTER, TXT_THE_CONVERTER, szTemp, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupAmmoRack (CObject *objP, int32_t nPlayer)
{
return (gameData.multiplayer.weaponStates [nPlayer].nShip != 0)
		 ? 0
		 : PickupEquipment (objP, PLAYER_FLAGS_AMMO_RACK, TXT_THE_AMMORACK, TXT_GOT_AMMORACK, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupAfterburner (CObject *objP, int32_t nPlayer)
{
	int32_t bUsed = PickupEquipment (objP, PLAYER_FLAGS_AFTERBURNER, TXT_THE_BURNER, TXT_GOT_BURNER, nPlayer);
	
if (bUsed >= 0)
	return bUsed;
gameData.physics.xAfterburnerCharge = I2X (1);
return 1;
}

//	-----------------------------------------------------------------------------

int32_t PickupSlowMotion (CObject *objP, int32_t nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_SLOWMOTION, TXT_THE_SLOWMOTION, TXT_GOT_SLOWMOTION, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupBulletTime (CObject *objP, int32_t nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_BULLETTIME, TXT_THE_BULLETTIME, TXT_GOT_BULLETTIME, nPlayer) != 0;
}

//------------------------------------------------------------------------------

int32_t PickupKey (CObject *objP, int32_t nKey, const char *pszKey, int32_t nPlayer)
{
//if (ISLOCALPLAYER (nPlayer)) 
	{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

	if (playerP->flags & nKey)
		return 0;
	if (objP) {
		MultiSendPlaySound (gameData.objData.pwrUp.info [objP->info.nId].hitSound, I2X (1));
		audio.PlaySound ((int16_t) gameData.objData.pwrUp.info [objP->info.nId].hitSound);
		}
	playerP->flags |= nKey;
	PowerupBasic (15, 0, 0, ISLOCALPLAYER (nPlayer) ? KEY_SCORE : 0, "%s %s", pszKey, TXT_ACCESS_GRANTED);
	InvalidateEscortGoal ();
	return IsMultiGame == 0;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupFlag (CObject *objP, int32_t nThisTeam, int32_t nOtherTeam, const char *pszFlag, int32_t nPlayer)
{
if (ISLOCALPLAYER (nPlayer)) {
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;
	if (gameData.app.GameMode (GM_CAPTURE)) {
		if (GetTeam (N_LOCALPLAYER) == nOtherTeam) {
			PowerupBasic (15, 0, 15, 0, nOtherTeam ? "RED FLAG!" : "BLUE FLAG!", nPlayer);
			playerP->flags |= PLAYER_FLAGS_FLAG;
			gameData.pig.flags [nThisTeam].path.Reset (10, -1);
			MultiSendGotFlag (N_LOCALPLAYER);
			return 1;
			}
		if (GetTeam (N_LOCALPLAYER) == nThisTeam) {
			ReturnFlagHome (objP);
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void UsePowerup (int32_t id)
{
	int32_t	bApply;

if ((bApply = (id < 0)))
	id = -id;
if (id >= MAX_POWERUP_TYPES_D2)
	id = POW_AFTERBURNER;
if (gameData.objData.pwrUp.info [id].hitSound > -1) {
	if (!bApply && (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame)) && ((id == POW_CLOAK) || (id == POW_INVUL)))
		id = POW_SHIELD_BOOST;
	if (IsMultiGame) // Added by Rob, take this out if it turns out to be not good for net games!
		MultiSendPlaySound (gameData.objData.pwrUp.info [id].hitSound, I2X (1));
	audio.PlaySound (int16_t (gameData.objData.pwrUp.info [id].hitSound));
	}
}

//------------------------------------------------------------------------------

int32_t ApplyInvul (int32_t bForce, int32_t nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + ((nPlayer < 0) ? N_LOCALPLAYER : nPlayer);
	int32_t bInventory = playerP->nInvuls && gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame);

if (!(bForce || bInventory))
	return 0;
if (playerP->flags & PLAYER_FLAGS_INVULNERABLE) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_INVULNERABLE);
	return 0;
	}
if (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame))
	playerP->nInvuls--;
if (ISLOCALPLAYER (nPlayer)) {
	playerP->invulnerableTime = gameData.time.xGame;
	playerP->flags |= PLAYER_FLAGS_INVULNERABLE;
	if (IsMultiGame)
		MultiSendInvul ();
	if (bInventory)
		PowerupBasic (7, 14, 21, 0, "");
	else
		PowerupBasic (7, 14, 21, INVULNERABILITY_SCORE, "%s!", TXT_INVULNERABILITY);
	SetupSpherePulse (gameData.multiplayer.spherePulse + N_LOCALPLAYER, 0.02f, 0.5f);
	UsePowerup (-POW_INVUL);
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t ApplyCloak (int32_t bForce, int32_t nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + ((nPlayer < 0) ? N_LOCALPLAYER : nPlayer);
	int32_t bInventory = playerP->nCloaks && gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame);

if (!(bForce || bInventory))
	return 0;
if (playerP->flags & PLAYER_FLAGS_CLOAKED) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_CLOAKED);
	return 0;
	}
if (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame))
	playerP->nCloaks--;
if (ISLOCALPLAYER (nPlayer)) {
	playerP->cloakTime = gameData.time.xGame;	//	Not!changed by awareness events (like CPlayerData fires laser).
	playerP->flags |= PLAYER_FLAGS_CLOAKED;
	AIDoCloakStuff ();
	if (IsMultiGame)
		MultiSendCloak ();
	if (bInventory)
		PowerupBasic (-10, -10, -10, 0, "");
	else
		PowerupBasic (-10, -10, -10, CLOAK_SCORE, "%s!", TXT_CLOAKING_DEVICE);
	UsePowerup (-POW_CLOAK);
	}
return 1;
}

//------------------------------------------------------------------------------

static inline int32_t PowerupCount (int32_t nId)
{
if ((nId == POW_CONCUSSION_4) || 
	 (nId == POW_HOMINGMSL_4) || 
	 (nId == POW_FLASHMSL_4) || 
	 (nId == POW_GUIDEDMSL_4) || 
	 (nId == POW_MERCURYMSL_4) || 
	 (nId == POW_PROXMINE) || 
	 (nId == POW_SMARTMINE))
	return 4;
return 1;
}

//------------------------------------------------------------------------------

#if defined (_WIN32) && defined (RELEASE)
typedef int32_t (__fastcall * pPickupGun) (CObject *, int32_t, int32_t);
typedef int32_t (__fastcall * pPickupMissile) (CObject *, int32_t, int32_t, int32_t);
typedef int32_t (__fastcall * pPickupEquipment) (CObject *, int32_t);
typedef int32_t (__fastcall * pPickupKey) (CObject *, int32_t, const char *, int32_t);
typedef int32_t (__fastcall * pPickupFlag) (CObject *, int32_t, int32_t, const char *, int32_t);
#else
typedef int32_t (* pPickupGun) (CObject *, int32_t, int32_t);
typedef int32_t (* pPickupMissile) (CObject *, int32_t, int32_t, int32_t);
typedef int32_t (* pPickupEquipment) (CObject *, int32_t);
typedef int32_t (* pPickupKey) (CObject *, int32_t, const char *, int32_t);
typedef int32_t (* pPickupFlag) (CObject *, int32_t, int32_t, const char *, int32_t);
#endif


//	returns true if powerup consumed
int32_t DoPowerup (CObject *objP, int32_t nPlayer)
{
if (OBSERVING)
	return 0;
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 0;
if (objP->Ignored (1, 1))
	return 0;

	CPlayerData*	playerP;
	int32_t			bUsed = 0;
	int32_t			bSpecialUsed = 0;		//for when hitting vulcan cannon gets vulcan ammo
	int32_t			bLocalPlayer;
	int32_t			nId, nType;

if (nPlayer < 0)
	nPlayer = N_LOCALPLAYER;
playerP = gameData.multiplayer.players + nPlayer;
if (SPECTATOR (OBJECT (playerP->nObject)))
	return 0;
bLocalPlayer = (nPlayer == N_LOCALPLAYER);
if (bLocalPlayer &&
	 (gameStates.app.bPlayerIsDead || 
	  (gameData.objData.consoleP->info.nType == OBJ_GHOST) || 
	  (playerP->Shield () < 0)))
	return 0;
if (objP->cType.powerupInfo.xCreationTime > gameData.time.xGame)		//gametime wrapped!
	objP->cType.powerupInfo.xCreationTime = 0;				//allow CPlayerData to pick up
if ((objP->cType.powerupInfo.nFlags & PF_SPAT_BY_PLAYER) && 
	 (objP->cType.powerupInfo.xCreationTime > 0) && 
	 (gameData.time.xGame < objP->cType.powerupInfo.xCreationTime + I2X (2)))
	return 0;		//not enough time elapsed
gameData.hud.bPlayerMessage = 0;	//	Prevent messages from going to HUD if -PlayerMessages switch is set
nId = objP->info.nId;
if ((abs (nId) >= (int32_t) sizeofa (pickupHandler)) || !pickupHandler [nId]) // unknown/unhandled powerup type
	return 0;
nType = powerupType [nId];
if (nType == POWERUP_IS_GUN) {
	bUsed = ((pPickupGun) (pickupHandler [nId])) (objP, powerupToDevice [nId], nPlayer);
	if ((bUsed < 0) && ((nId == POW_VULCAN) || (nId == POW_GAUSS))) {
		bUsed = -bUsed - 1;
		bSpecialUsed = 1;
		nId = POW_VULCAN_AMMO;
		}
	}
else if (nType == POWERUP_IS_MISSILE) {
	bUsed = ((pPickupMissile) (pickupHandler [nId])) (objP, powerupToDevice [nId], PowerupCount (nId), nPlayer);
	}
else if (nType == POWERUP_IS_KEY) {
	int32_t nKey = nId - POW_KEY_BLUE;
	bUsed = ((pPickupKey) (pickupHandler [nId])) (objP, PLAYER_FLAGS_BLUE_KEY << nKey, GAMETEXT (12 + nKey), nPlayer);
	}
else if (nType == POWERUP_IS_EQUIPMENT) {
	bUsed = ((pPickupEquipment) (pickupHandler [nId])) (objP, nPlayer);
	}
else if (nType == POWERUP_IS_FLAG) {
	int32_t nFlag = nId - POW_BLUEFLAG;
	bUsed = ((pPickupFlag) (pickupHandler [nId])) (objP, nFlag, !nFlag, GT (1077 + nFlag), nPlayer);
	}
else
	return 0;

//always say bUsed, until physics problem (getting stuck on unused powerup)
//is solved.  Note also the break statements above that are commented out
//!!	bUsed = 1;

if (bUsed || bSpecialUsed)
	UsePowerup (nId * (bUsed ? bUsed : bSpecialUsed));
gameData.hud.bPlayerMessage = 1;
return bUsed;
}

//------------------------------------------------------------------------------

int32_t SpawnPowerup (CObject *spitterP, uint8_t nId, int32_t nCount)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 0;

	int32_t			i;
	int16_t			nObject;
	CFixVector	velSave;
	CObject		*objP;

if (nCount <= 0)
	return 0;
velSave = spitterP->mType.physInfo.velocity;
spitterP->mType.physInfo.velocity.SetZero ();
for (i = nCount; i; i--) {
	nObject = SpitPowerup (spitterP, nId);
	if (nObject >= 0) {
		objP = OBJECT (nObject);
		MultiSendCreatePowerup (nId, objP->info.nSegment, nObject, &objP->info.position.vPos);
		}
	}
spitterP->mType.physInfo.velocity = velSave;
return nCount;
}

//------------------------------------------------------------------------------

void SpawnLeftoverPowerups (int16_t nObject)
{
SpawnPowerup (gameData.multiplayer.leftoverPowerups [nObject].spitterP, 
				  gameData.multiplayer.leftoverPowerups [nObject].nType,
				  gameData.multiplayer.leftoverPowerups [nObject].nCount);
memset (&gameData.multiplayer.leftoverPowerups [nObject], 0, 
		  sizeof (gameData.multiplayer.leftoverPowerups [0]));
}

//------------------------------------------------------------------------------

void CheckInventory (void)
{
	CPlayerData	*playerP = gameData.multiplayer.players + N_LOCALPLAYER;
	CObject	*objP = OBJECT (playerP->nObject);

if (SpawnPowerup (objP, POW_CLOAK, playerP->nCloaks - MAX_INV_ITEMS))
	playerP->nCloaks = MAX_INV_ITEMS;
if (SpawnPowerup (objP, POW_INVUL, playerP->nInvuls - MAX_INV_ITEMS))
	playerP->nInvuls = MAX_INV_ITEMS;
}

//-----------------------------------------------------------------------------

void InitPowerupTables (void)
{
memset (powerupToDevice, 0xff, sizeof (powerupToDevice));

powerupToDevice [POW_LASER] = LASER_INDEX;
powerupToDevice [POW_VULCAN] = VULCAN_INDEX;
powerupToDevice [POW_SPREADFIRE] = SPREADFIRE_INDEX;
powerupToDevice [POW_PLASMA] = PLASMA_INDEX;
powerupToDevice [POW_FUSION] = FUSION_INDEX;
powerupToDevice [POW_GAUSS] = GAUSS_INDEX;
powerupToDevice [POW_HELIX] = HELIX_INDEX;
powerupToDevice [POW_PHOENIX] = PHOENIX_INDEX;
powerupToDevice [POW_OMEGA] = OMEGA_INDEX;
powerupToDevice [POW_SUPERLASER] = SUPER_LASER_INDEX;
powerupToDevice [POW_CONCUSSION_1] = 
powerupToDevice [POW_CONCUSSION_4] = CONCUSSION_INDEX;
powerupToDevice [POW_PROXMINE] = PROXMINE_INDEX;
powerupToDevice [POW_HOMINGMSL_1] = 
powerupToDevice [POW_HOMINGMSL_4] = HOMING_INDEX;
powerupToDevice [POW_SMARTMSL] = SMART_INDEX;
powerupToDevice [POW_MEGAMSL] = MEGA_INDEX;
powerupToDevice [POW_FLASHMSL_1] = 
powerupToDevice [POW_FLASHMSL_4] = FLASHMSL_INDEX;
powerupToDevice [POW_GUIDEDMSL_1] = 
powerupToDevice [POW_GUIDEDMSL_4] = GUIDED_INDEX;
powerupToDevice [POW_SMARTMINE] = SMARTMINE_INDEX;
powerupToDevice [POW_MERCURYMSL_1] = 
powerupToDevice [POW_MERCURYMSL_4] = MERCURY_INDEX;
powerupToDevice [POW_EARTHSHAKER] = EARTHSHAKER_INDEX;

powerupToDevice [POW_CLOAK] = PLAYER_FLAGS_CLOAKED;
powerupToDevice [POW_INVUL] = PLAYER_FLAGS_INVULNERABLE;
powerupToDevice [POW_KEY_BLUE] = PLAYER_FLAGS_BLUE_KEY;
powerupToDevice [POW_KEY_RED] = PLAYER_FLAGS_RED_KEY;
powerupToDevice [POW_KEY_GOLD] = PLAYER_FLAGS_GOLD_KEY;
powerupToDevice [POW_FULL_MAP] = PLAYER_FLAGS_FULLMAP;
powerupToDevice [POW_AMMORACK] = PLAYER_FLAGS_AMMO_RACK;
powerupToDevice [POW_CONVERTER] = PLAYER_FLAGS_CONVERTER;
powerupToDevice [POW_QUADLASER] = PLAYER_FLAGS_QUAD_LASERS;
powerupToDevice [POW_AFTERBURNER] = PLAYER_FLAGS_AFTERBURNER;
powerupToDevice [POW_HEADLIGHT] = PLAYER_FLAGS_HEADLIGHT;
powerupToDevice [POW_SLOWMOTION] = PLAYER_FLAGS_SLOWMOTION;
powerupToDevice [POW_BULLETTIME] = PLAYER_FLAGS_BULLETTIME;
powerupToDevice [POW_BLUEFLAG] = 
powerupToDevice [POW_REDFLAG] = PLAYER_FLAGS_FLAG;

powerupToDevice [POW_VULCAN_AMMO] = POW_VULCAN_AMMO;

memset (powerupToWeaponCount, 0, sizeof (powerupToWeaponCount));

powerupToWeaponCount [POW_LASER] = 
powerupToWeaponCount [POW_VULCAN] = 
powerupToWeaponCount [POW_SPREADFIRE] = 
powerupToWeaponCount [POW_PLASMA] = 
powerupToWeaponCount [POW_FUSION] = 
powerupToWeaponCount [POW_GAUSS] = 
powerupToWeaponCount [POW_HELIX] = 
powerupToWeaponCount [POW_PHOENIX] = 
powerupToWeaponCount [POW_OMEGA] = 
powerupToWeaponCount [POW_SUPERLASER] = 1;

powerupToWeaponCount [POW_CLOAK] = 
powerupToWeaponCount [POW_TURBO] = 
powerupToWeaponCount [POW_INVUL] = 
powerupToWeaponCount [POW_FULL_MAP] = 
powerupToWeaponCount [POW_CONVERTER] = 
powerupToWeaponCount [POW_AMMORACK] = 
powerupToWeaponCount [POW_AFTERBURNER] = 
powerupToWeaponCount [POW_HEADLIGHT] = 1;

powerupToWeaponCount [POW_CONCUSSION_1] = 
powerupToWeaponCount [POW_SMARTMSL] = 
powerupToWeaponCount [POW_MEGAMSL] = 
powerupToWeaponCount [POW_FLASHMSL_1] = 
powerupToWeaponCount [POW_GUIDEDMSL_1] = 
powerupToWeaponCount [POW_MERCURYMSL_1] = 
powerupToWeaponCount [POW_EARTHSHAKER] = 
powerupToWeaponCount [POW_HOMINGMSL_1] = 1;

powerupToWeaponCount [POW_CONCUSSION_4] = 
powerupToWeaponCount [POW_PROXMINE] = 
powerupToWeaponCount [POW_FLASHMSL_4] = 
powerupToWeaponCount [POW_GUIDEDMSL_4] = 
powerupToWeaponCount [POW_SMARTMINE] = 
powerupToWeaponCount [POW_MERCURYMSL_4] = 
powerupToWeaponCount [POW_HOMINGMSL_4] = 4;

powerupToWeaponCount [POW_VULCAN_AMMO] = 1;

memset (powerupClass, 0, sizeof (powerupClass));
powerupClass [POW_LASER] = 
powerupClass [POW_VULCAN] =
powerupClass [POW_SPREADFIRE] =
powerupClass [POW_PLASMA] =
powerupClass [POW_FUSION] =
powerupClass [POW_GAUSS] =
powerupClass [POW_HELIX] =
powerupClass [POW_PHOENIX] =
powerupClass [POW_OMEGA] =
powerupClass [POW_SUPERLASER] = 1;

powerupClass [POW_CONCUSSION_1] = 
powerupClass [POW_CONCUSSION_4] = 
powerupClass [POW_PROXMINE] =
powerupClass [POW_SMARTMSL] =
powerupClass [POW_MEGAMSL] =
powerupClass [POW_FLASHMSL_1] = 
powerupClass [POW_FLASHMSL_4] =
powerupClass [POW_GUIDEDMSL_1] =
powerupClass [POW_GUIDEDMSL_4] = 
powerupClass [POW_SMARTMINE] = 
powerupClass [POW_MERCURYMSL_1] = 
powerupClass [POW_MERCURYMSL_4] =
powerupClass [POW_EARTHSHAKER] =
powerupClass [POW_HOMINGMSL_1] = 
powerupClass [POW_HOMINGMSL_4] = 2;

powerupClass [POW_QUADLASER] = 
powerupClass [POW_CLOAK] = 
//powerupClass [POW_TURBO] = 
powerupClass [POW_INVUL] = 
powerupClass [POW_FULL_MAP] = 
powerupClass [POW_CONVERTER] = 
powerupClass [POW_AMMORACK] = 
powerupClass [POW_AFTERBURNER] = 
powerupClass [POW_HEADLIGHT] = 3;

powerupClass [POW_VULCAN_AMMO] = 4;

powerupClass [POW_BLUEFLAG] = 
powerupClass [POW_REDFLAG] = 5;

powerupClass [POW_KEY_BLUE] = 
powerupClass [POW_KEY_GOLD] = 
powerupClass [POW_KEY_RED] = 6;

memset (powerupToObject, 0xff, sizeof (powerupToObject));
powerupToObject [POW_LASER] = LASER_ID;
powerupToObject [POW_VULCAN] = VULCAN_ID;
powerupToObject [POW_SPREADFIRE] = SPREADFIRE_ID;
powerupToObject [POW_PLASMA] = PLASMA_ID;
powerupToObject [POW_FUSION] = FUSION_ID;
powerupToObject [POW_GAUSS] = GAUSS_ID;
powerupToObject [POW_HELIX] = HELIX_ID;
powerupToObject [POW_PHOENIX] = PHOENIX_ID;
powerupToObject [POW_OMEGA] = OMEGA_ID;
powerupToObject [POW_SUPERLASER] = SUPERLASER_ID;
powerupToObject [POW_CONCUSSION_1] = CONCUSSION_ID;
powerupToObject [POW_PROXMINE] = PROXMINE_ID;
powerupToObject [POW_SMARTMSL] = SMARTMSL_ID;
powerupToObject [POW_MEGAMSL] = MEGAMSL_ID;
powerupToObject [POW_FLASHMSL_1] = FLASHMSL_ID;
powerupToObject [POW_GUIDEDMSL_1] = GUIDEDMSL_ID;
powerupToObject [POW_SMARTMINE] = SMINEPACK_ID;
powerupToObject [POW_MERCURYMSL_1] = MERCURYMSL_ID;
powerupToObject [POW_EARTHSHAKER] = EARTHSHAKER_ID;
powerupToObject [POW_HOMINGMSL_1] = HOMINGMSL_ID;

powerupToObject [POW_CONCUSSION_4] = CONCUSSION_ID;
powerupToObject [POW_FLASHMSL_4] = FLASHMSL_ID;
powerupToObject [POW_HOMINGMSL_4] = HOMINGMSL_ID;
powerupToObject [POW_GUIDEDMSL_4] = GUIDEDMSL_ID;
powerupToObject [POW_MERCURYMSL_4] = MERCURYMSL_ID;

memset (powerupToModel, 0, sizeof (powerupToModel));
powerupToModel [POW_PROXMINE] = MAX_POLYGON_MODELS - 1;
powerupToModel [POW_SMARTMINE] = MAX_POLYGON_MODELS - 3;
powerupToModel [POW_CONCUSSION_4] = MAX_POLYGON_MODELS - 5;
powerupToModel [POW_HOMINGMSL_4] = MAX_POLYGON_MODELS - 6;
powerupToModel [POW_FLASHMSL_4] = MAX_POLYGON_MODELS - 7;
powerupToModel [POW_GUIDEDMSL_4] = MAX_POLYGON_MODELS - 8;
powerupToModel [POW_MERCURYMSL_4] = MAX_POLYGON_MODELS - 9;
powerupToModel [POW_LASER] = MAX_POLYGON_MODELS - 10;
powerupToModel [POW_VULCAN] = MAX_POLYGON_MODELS - 11;
powerupToModel [POW_SPREADFIRE] = MAX_POLYGON_MODELS - 12;
powerupToModel [POW_PLASMA] = MAX_POLYGON_MODELS - 13;
powerupToModel [POW_FUSION] = MAX_POLYGON_MODELS - 14;
powerupToModel [POW_SUPERLASER] = MAX_POLYGON_MODELS - 15;
powerupToModel [POW_GAUSS] = MAX_POLYGON_MODELS - 16;
powerupToModel [POW_HELIX] = MAX_POLYGON_MODELS - 17;
powerupToModel [POW_PHOENIX] = MAX_POLYGON_MODELS - 18;
powerupToModel [POW_OMEGA] = MAX_POLYGON_MODELS - 19;
powerupToModel [POW_QUADLASER] = MAX_POLYGON_MODELS - 20;
powerupToModel [POW_AFTERBURNER] = MAX_POLYGON_MODELS - 21;
powerupToModel [POW_HEADLIGHT] = MAX_POLYGON_MODELS - 22;
powerupToModel [POW_AMMORACK] = MAX_POLYGON_MODELS - 23;
powerupToModel [POW_CONVERTER] = MAX_POLYGON_MODELS - 24;
powerupToModel [POW_FULL_MAP] = MAX_POLYGON_MODELS - 25;
powerupToModel [POW_CLOAK] = MAX_POLYGON_MODELS - 26;
powerupToModel [POW_INVUL] = MAX_POLYGON_MODELS - 27;
powerupToModel [POW_EXTRA_LIFE] = MAX_POLYGON_MODELS - 28;
powerupToModel [POW_KEY_BLUE] = MAX_POLYGON_MODELS - 29;
powerupToModel [POW_KEY_RED] = MAX_POLYGON_MODELS - 30;
powerupToModel [POW_KEY_GOLD] = MAX_POLYGON_MODELS - 31;
powerupToModel [POW_VULCAN_AMMO] = MAX_POLYGON_MODELS - 32;
powerupToModel [POW_SLOWMOTION] = MAX_POLYGON_MODELS - 33;
powerupToModel [POW_BULLETTIME] = MAX_POLYGON_MODELS - 34;

memset (weaponToModel, 0, sizeof (weaponToModel));
weaponToModel [PROXMINE_ID] = MAX_POLYGON_MODELS - 2;
weaponToModel [SMARTMINE_ID] = MAX_POLYGON_MODELS - 4;
weaponToModel [ROBOT_SMARTMINE_ID] = MAX_POLYGON_MODELS - 4;

pickupHandler [POW_EXTRA_LIFE] = reinterpret_cast<void*> (PickupExtraLife);
pickupHandler [POW_ENERGY] = reinterpret_cast<void*> (PickupEnergyBoost);
pickupHandler [POW_SHIELD_BOOST] = reinterpret_cast<void*> (PickupShieldBoost);
pickupHandler [POW_CLOAK] = reinterpret_cast<void*> (PickupCloakingDevice);
pickupHandler [POW_TURBO] = NULL;
pickupHandler [POW_INVUL] = reinterpret_cast<void*> (PickupInvulnerability);
pickupHandler [POW_MEGAWOW] = NULL;

pickupHandler [POW_FULL_MAP] = reinterpret_cast<void*> (PickupFullMap);
pickupHandler [POW_CONVERTER] = reinterpret_cast<void*> (PickupConverter);
pickupHandler [POW_AMMORACK] = reinterpret_cast<void*> (PickupAmmoRack);
pickupHandler [POW_AFTERBURNER] = reinterpret_cast<void*> (PickupAfterburner);
pickupHandler [POW_HEADLIGHT] = reinterpret_cast<void*> (PickupHeadlight);
pickupHandler [POW_SLOWMOTION] = reinterpret_cast<void*> (PickupSlowMotion);
pickupHandler [POW_BULLETTIME] = reinterpret_cast<void*> (PickupBulletTime);
pickupHandler [POW_HOARD_ORB] = reinterpret_cast<void*> (PickupHoardOrb);
pickupHandler [POW_MONSTERBALL] = NULL;
pickupHandler [POW_VULCAN_AMMO] = reinterpret_cast<void*> (PickupVulcanAmmo);

pickupHandler [POW_BLUEFLAG] =
pickupHandler [POW_REDFLAG] = reinterpret_cast<void*> (PickupFlag);

pickupHandler [POW_KEY_BLUE] =
pickupHandler [POW_KEY_RED] =
pickupHandler [POW_KEY_GOLD] = reinterpret_cast<void*> (PickupKey);

pickupHandler [POW_LASER] = reinterpret_cast<void*> (PickupLaser);
pickupHandler [POW_SUPERLASER] = reinterpret_cast<void*> (PickupSuperLaser);
pickupHandler [POW_QUADLASER] = reinterpret_cast<void*> (PickupQuadLaser);
pickupHandler [POW_SPREADFIRE] = 
pickupHandler [POW_PLASMA] = 
pickupHandler [POW_FUSION] = 
pickupHandler [POW_HELIX] = 
pickupHandler [POW_PHOENIX] = 
pickupHandler [POW_OMEGA] = reinterpret_cast<void*> (PickupGun);
pickupHandler [POW_VULCAN] = 
pickupHandler [POW_GAUSS] = reinterpret_cast<void*> (PickupGatlingGun);

pickupHandler [POW_PROXMINE] =
pickupHandler [POW_SMARTMINE] =
pickupHandler [POW_CONCUSSION_1] =
pickupHandler [POW_CONCUSSION_4] =
pickupHandler [POW_HOMINGMSL_1] =
pickupHandler [POW_HOMINGMSL_4] =
pickupHandler [POW_SMARTMSL] =
pickupHandler [POW_MEGAMSL] =
pickupHandler [POW_FLASHMSL_1] =
pickupHandler [POW_FLASHMSL_4] =     
pickupHandler [POW_GUIDEDMSL_1] =
pickupHandler [POW_GUIDEDMSL_4] =
pickupHandler [POW_MERCURYMSL_1] =
pickupHandler [POW_MERCURYMSL_4] =
pickupHandler [POW_EARTHSHAKER] = reinterpret_cast<void*> (PickupSecondary);

powerupType [POW_TURBO] = 
powerupType [POW_MEGAWOW] = 
powerupType [POW_MONSTERBALL] = (uint8_t) POWERUP_IS_UNDEFINED;

powerupType [POW_EXTRA_LIFE] = 
powerupType [POW_ENERGY] = 
powerupType [POW_SHIELD_BOOST] = 
powerupType [POW_CLOAK] = 
powerupType [POW_HOARD_ORB] =      
powerupType [POW_INVUL] = 
powerupType [POW_FULL_MAP] = 
powerupType [POW_CONVERTER] = 
powerupType [POW_AMMORACK] = 
powerupType [POW_AFTERBURNER] = 
powerupType [POW_HEADLIGHT] = 
powerupType [POW_SLOWMOTION] =
powerupType [POW_BULLETTIME] =
powerupType [POW_VULCAN_AMMO] = (uint8_t) POWERUP_IS_EQUIPMENT;

powerupType [POW_BLUEFLAG] =
powerupType [POW_REDFLAG] = (uint8_t) POWERUP_IS_FLAG;

powerupType [POW_KEY_BLUE] =
powerupType [POW_KEY_RED] =
powerupType [POW_KEY_GOLD] = (uint8_t) POWERUP_IS_KEY;

powerupType [POW_LASER] = 
powerupType [POW_SPREADFIRE] = 
powerupType [POW_PLASMA] = 
powerupType [POW_FUSION] = 
powerupType [POW_SUPERLASER] = 
powerupType [POW_HELIX] = 
powerupType [POW_PHOENIX] = 
powerupType [POW_OMEGA] = 
powerupType [POW_VULCAN] = 
powerupType [POW_GAUSS] = 
powerupType [POW_QUADLASER] = (uint8_t) POWERUP_IS_GUN;

powerupType [POW_CONCUSSION_1] = 
powerupType [POW_CONCUSSION_4] = 
powerupType [POW_PROXMINE] = 
powerupType [POW_HOMINGMSL_1] = 
powerupType [POW_HOMINGMSL_4] = 
powerupType [POW_SMARTMSL] = 
powerupType [POW_MEGAMSL] = 
powerupType [POW_FLASHMSL_1] = 
powerupType [POW_FLASHMSL_4] =   
powerupType [POW_GUIDEDMSL_1] = 
powerupType [POW_GUIDEDMSL_4] = 
powerupType [POW_SMARTMINE] = 
powerupType [POW_MERCURYMSL_1] = 
powerupType [POW_MERCURYMSL_4] = 
powerupType [POW_EARTHSHAKER] = (uint8_t) POWERUP_IS_MISSILE;
}

//-----------------------------------------------------------------------------

#define ENABLE_FILTER(_type,_flag) if (_flag) powerupFilter [_type] = 1;

void SetupPowerupFilter (tNetGameInfo* infoP)
{
if (!infoP)
	infoP = &netGameInfo.m_info;
memset (powerupFilter, 0, sizeof (powerupFilter));
ENABLE_FILTER (POW_INVUL, infoP->DoInvulnerability);
ENABLE_FILTER (POW_CLOAK, infoP->DoCloak);
ENABLE_FILTER (POW_KEY_BLUE, IsCoopGame);
ENABLE_FILTER (POW_KEY_RED, IsCoopGame);
ENABLE_FILTER (POW_KEY_GOLD, IsCoopGame);
ENABLE_FILTER (POW_AFTERBURNER, infoP->DoAfterburner);
ENABLE_FILTER (POW_FUSION, infoP->DoFusions);
ENABLE_FILTER (POW_PHOENIX, infoP->DoPhoenix);
ENABLE_FILTER (POW_HELIX, infoP->DoHelix);
ENABLE_FILTER (POW_MEGAMSL, infoP->DoMegas);
ENABLE_FILTER (POW_SMARTMSL, infoP->DoSmarts);
ENABLE_FILTER (POW_GAUSS, infoP->DoGauss);
ENABLE_FILTER (POW_VULCAN, infoP->DoVulcan);
ENABLE_FILTER (POW_PLASMA, infoP->DoPlasma);
ENABLE_FILTER (POW_OMEGA, infoP->DoOmega);
ENABLE_FILTER (POW_LASER, 1);
ENABLE_FILTER (POW_SUPERLASER, infoP->DoSuperLaser);
ENABLE_FILTER (POW_MONSTERBALL, (gameData.app.GameMode (GM_HOARD | GM_MONSTERBALL)));
ENABLE_FILTER (POW_PROXMINE, infoP->DoProximity || (gameData.app.GameMode (GM_HOARD | GM_ENTROPY)));
ENABLE_FILTER (POW_SMARTMINE, infoP->DoSmartMine || IsEntropyGame);
ENABLE_FILTER (POW_VULCAN_AMMO, (infoP->DoVulcan || infoP->DoGauss));
ENABLE_FILTER (POW_SPREADFIRE, infoP->DoSpread);
ENABLE_FILTER (POW_FLASHMSL_1, infoP->DoFlash);
ENABLE_FILTER (POW_FLASHMSL_4, infoP->DoFlash);
ENABLE_FILTER (POW_GUIDEDMSL_1, infoP->DoGuided);
ENABLE_FILTER (POW_GUIDEDMSL_4, infoP->DoGuided);
ENABLE_FILTER (POW_EARTHSHAKER, infoP->DoEarthShaker);
ENABLE_FILTER (POW_MERCURYMSL_1, infoP->DoMercury);
ENABLE_FILTER (POW_MERCURYMSL_4, infoP->DoMercury);
ENABLE_FILTER (POW_CONVERTER, infoP->DoConverter);
ENABLE_FILTER (POW_AMMORACK, infoP->DoAmmoRack);
ENABLE_FILTER (POW_HEADLIGHT, infoP->DoHeadlight && (!EGI_FLAG (bDarkness, 0, 0, 0) || EGI_FLAG (headlight.bAvailable, 0, 1, 0)));
ENABLE_FILTER (POW_LASER, infoP->DoLaserUpgrade);
ENABLE_FILTER (POW_CONCUSSION_1, 1);
ENABLE_FILTER (POW_CONCUSSION_4, 1);
ENABLE_FILTER (POW_HOMINGMSL_1, infoP->DoHoming);
ENABLE_FILTER (POW_HOMINGMSL_4, infoP->DoHoming);
ENABLE_FILTER (POW_QUADLASER, infoP->DoQuadLasers);
ENABLE_FILTER (POW_BLUEFLAG, (gameData.app.GameMode (GM_CAPTURE)));
ENABLE_FILTER (POW_REDFLAG, (gameData.app.GameMode (GM_CAPTURE)));
}

//-----------------------------------------------------------------------------

int32_t PowerupToDevice (int16_t nPowerup, int32_t *nType)
{
*nType = powerupClass [nPowerup];
return powerupToDevice [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupClass (int16_t nPowerup)
{
return powerupClass [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupToWeaponCount (int16_t nPowerup)
{
return powerupToWeaponCount [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupToObject (int16_t nPowerup)
{
return powerupToObject [nPowerup];
}

//-----------------------------------------------------------------------------

int16_t PowerupToModel (int16_t nPowerup)
{
return powerupToModel [nPowerup];
}

//-----------------------------------------------------------------------------

int16_t WeaponToModel (int16_t nWeapon)
{
return weaponToModel [nWeapon];
}

//-----------------------------------------------------------------------------
// powerup classes:
// 1 - guns
// 2 - missiles
// 3 - equipment
// 4 - vulcan ammo
// 5 - flags
// 6 - keys

int16_t PowerupsOnShips (int32_t nPowerup)
{
	CPlayerData*	playerP = gameData.multiplayer.players;
	int32_t			nClass, nVulcanAmmo = 0;
	int16_t			nPowerups = 0, nIndex = PowerupToDevice (nPowerup, &nClass);

if (!nClass || ((nClass < 3) && (nIndex < 0)))
	return 0;
for (int16_t i = 0; i < N_PLAYERS; i++, playerP++) {
	//if ((i == N_LOCALPLAYER) && (LOCALPLAYER.m_bExploded || gameStates.app.bPlayerIsDead))
	//	continue;
	if (playerP->Shield () < 0)
		continue; 
#if 0 //DBG
	if (!playerP->connected && (gameStates.app.nSDLTicks [0] - playerP->m_tDisconnect > 600))
#else
	if (!playerP->connected && (gameStates.app.nSDLTicks [0] - playerP->m_tDisconnect > TIMEOUT_KICK))
#endif
		continue; // wait up to three minutes for a player to reconnect before dropping him and allowing to respawn his stuff
	if (nClass == 5) {
		if ((PLAYER (i).flags & PLAYER_FLAGS_FLAG) && ((nPowerup == POW_REDFLAG) != (GetTeam (i) == TEAM_RED)))
			nPowerups++;
		}
	if (nClass == 4) 
		nVulcanAmmo += playerP->primaryAmmo [VULCAN_INDEX] + gameData.multiplayer.weaponStates [i].nAmmoUsed % VULCAN_CLIP_CAPACITY;
	else if (nClass == 3) {	// some device
		if (!(extraGameInfo [IsMultiGame].loadout.nDevice & nIndex))
			nPowerups += (playerP->flags & nIndex) != 0;
		}
	else if (nClass == 2) {	// missiles
		nPowerups += gameData.multiplayer.SecondaryAmmo (i, nIndex, 0);
		}
	else {	// guns
		if (!(extraGameInfo [IsMultiGame].loadout.nGuns & (1 << nIndex))) {
			if (nIndex == LASER_INDEX) {
				//if (!(extraGameInfo [0].loadout.nGuns & (1 << 5)))
					nPowerups += playerP->LaserLevel (0);
				}
			else if (nIndex == SUPER_LASER_INDEX) {
				nPowerups += playerP->LaserLevel (1);
				}
			else if (playerP->primaryWeaponFlags & (1 << nIndex)) {
				nPowerups++;
				if ((nIndex == FUSION_INDEX) && gameData.multiplayer.weaponStates [i].bTripleFusion)
					nPowerups++;
				}
			}
		}
	}
return (nClass == 4) ? (nVulcanAmmo + VULCAN_CLIP_CAPACITY - 1) / VULCAN_CLIP_CAPACITY : nPowerups;
} 

//------------------------------------------------------------------------------
/*
 * reads n tPowerupTypeInfo structs from a CFile
 */
extern int32_t ReadPowerupTypeInfos (tPowerupTypeInfo *pti, int32_t n, CFile& cf)
{
	int32_t i;

for (i = 0; i < n; i++) {
	pti [i].nClipIndex = cf.ReadInt ();
	pti [i].hitSound = cf.ReadInt ();
	pti [i].size = cf.ReadFix ();
	pti [i].light = cf.ReadFix ();
	}
return i;
}

//------------------------------------------------------------------------------
//eof
