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

#include "inferno.h"
#include "error.h"
#include "objrender.h"
#include "key.h"
#include "fireball.h"
#include "gauges.h"
#include "text.h"
#include "scores.h"
#include "light.h"
#include "kconfig.h"
#include "render.h"
#include "newdemo.h"
#include "sphere.h"

#ifdef EDITOR
#include "gr.h"	//	for powerup outline drawing
#include "editor/editor.h"
#endif
//#define _DEBUG
int ReturnFlagHome (CObject *pObj);
void InvalidateEscortGoal (void);
char GetKeyValue (char);
void MultiSendGotFlag (char);

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

char powerupToWeapon [MAX_POWERUP_TYPES];
char powerupToWeaponCount [MAX_POWERUP_TYPES];
char powerupClass [MAX_POWERUP_TYPES];
char powerupToObject [MAX_POWERUP_TYPES];
short powerupToModel [MAX_POWERUP_TYPES];
short weaponToModel [MAX_WEAPON_TYPES];
ubyte powerupType [MAX_POWERUP_TYPES];
void *pickupHandler [MAX_POWERUP_TYPES];

//------------------------------------------------------------------------------

void UpdatePowerupClip (tVideoClip *vcP, tVClipInfo *vciP, int nObject)
{
if (vcP) {
	static fix	xPowerupTime = 0;
	int			h, nFrames = SetupHiresVClip (vcP, vciP);
	fix			xTime, xFudge = (xPowerupTime * (nObject & 3)) >> 4;

	xPowerupTime += gameData.physics.xTime;
	xTime = vciP->xFrameTime - (fix) ((xPowerupTime + xFudge) / gameStates.gameplay.slowmo [0].fSpeed);
	if ((xTime < 0) && (vcP->xFrameTime > 0)) {
		h = (-xTime + vcP->xFrameTime - 1) / vcP->xFrameTime;
		xTime += h * vcP->xFrameTime;
		h %= nFrames;
		if ((nObject & 1) && (OBJECTS [nObject].info.nType != OBJ_EXPLOSION)) 
			vciP->nCurFrame -= h;
		else
			vciP->nCurFrame += h;
		if (vciP->nCurFrame < 0)
			vciP->nCurFrame = nFrames - (-vciP->nCurFrame % nFrames);
		else 
			vciP->nCurFrame %= nFrames;
		}
	vciP->xFrameTime = xTime;
	xPowerupTime = 0;
	}
else {
	int	h, nFrames;

	if (0 > (h = (gameStates.app.nSDLTicks - vciP->xTotalTime))) {
		vciP->xTotalTime = gameStates.app.nSDLTicks;
		h = 0;
		}
	else if ((h = h / 80) && (nFrames = gameData.pig.tex.addonBitmaps [-vciP->nClipIndex - 1].FrameCount ())) { //???
		vciP->xTotalTime += h * 80;
		if (gameStates.app.nSDLTicks < vciP->xTotalTime)
			vciP->xTotalTime = gameStates.app.nSDLTicks;
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
	}
}

//------------------------------------------------------------------------------

void UpdateFlagClips (void)
{
if (!gameStates.app.bDemoData) {
	UpdatePowerupClip (gameData.pig.flags [0].vcP, &gameData.pig.flags [0].vci, 0);
	UpdatePowerupClip (gameData.pig.flags [1].vcP, &gameData.pig.flags [1].vci, 0);
	}
}

//------------------------------------------------------------------------------
//process this powerup for this frame
void DoPowerupFrame (CObject *objP)
{
//if (gameStates.app.tick40fps.bTick) 
	tVClipInfo	*vciP = &objP->rType.vClipInfo;
	tVideoClip	*vcP = (vciP->nClipIndex < 0) ? NULL : gameData.eff.vClips [0] + vciP->nClipIndex;
	int			i = OBJ_IDX (objP);

if (objP->info.renderType != RT_POLYOBJ)
	UpdatePowerupClip (vcP, vciP, i);
if (objP->info.xLifeLeft <= 0) {
	ObjectCreateExplosion (objP->info.nSegment, &objP->info.position.vPos, F1_0 * 7 / 2, VCLIP_POWERUP_DISAPPEARANCE);
	if (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].nSound > -1)
		DigiLinkSoundToObject (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].nSound, i, 0, F1_0, SOUNDCLASS_GENERIC);
	}
}

#ifdef EDITOR
extern fix blob_vertices[];

//------------------------------------------------------------------------------
//	blob_vertices has 3 vertices in it, 4th must be computed
void DrawBlobOutline (void)
{
	fix	v3x, v3y;

	v3x = blob_vertices[4] - blob_vertices[2] + blob_vertices[0];
	v3y = blob_vertices[5] - blob_vertices[3] + blob_vertices[1];

	CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);

	GrLine (blob_vertices[0], blob_vertices[1], blob_vertices[2], blob_vertices[3]);
	GrLine (blob_vertices[2], blob_vertices[3], blob_vertices[4], blob_vertices[5]);
	GrLine (blob_vertices[4], blob_vertices[5], v3x, v3y);

	GrLine (v3x, v3y, blob_vertices[0], blob_vertices[1]);
}
#endif

//------------------------------------------------------------------------------

void DrawPowerup (CObject *objP)
{
#if DBG
//return;
#endif
if (objP->info.nType == OBJ_MONSTERBALL)
	DrawMonsterball (objP, 1.0f, 0.5f, 0.0f, 0.9f);
else if ((objP->info.nId < MAX_POWERUP_TYPES_D2) || ((objP->info.nType == OBJ_EXPLOSION) && (objP->info.nId < VCLIP_MAXNUM))) {
		tBitmapIndex	*frameP = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].frames;
		int				iFrame = objP->rType.vClipInfo.nCurFrame;
#ifdef EDITOR
		blob_vertices[0] = 0x80000;
#endif
	DrawObjectBlob (objP, frameP->index, frameP [iFrame].index, iFrame, NULL, 0);
#ifdef EDITOR
	if ((gameStates.app.nFunctionMode == FMODE_EDITOR) && (CurObject_index == OBJ_IDX (objP)))
		if (blob_vertices[0] != 0x80000)
			DrawBlobOutline ();
#endif
	}
else {
	DrawObjectBlob (objP, objP->rType.vClipInfo.nClipIndex, objP->rType.vClipInfo.nClipIndex, objP->rType.vClipInfo.nCurFrame, NULL, 1);
	}
}

//------------------------------------------------------------------------------

void _CDECL_ PowerupBasic (int redAdd, int greenAdd, int blueAdd, int score, const char *format, ...)
{
	char		text[120];
	va_list	args;

va_start (args, format);
vsprintf (text, format, args);
va_end (args);
paletteManager.BumpEffect (redAdd, greenAdd, blueAdd);
HUDInitMessage (text);
//mprintf_gameData.objs.pwrUp.Info ();
AddPointsToScore (score);
}

//------------------------------------------------------------------------------

//#if DBG
//	Give the megawow powerup!
void DoMegaWowPowerup (int quantity)
{
	int i;

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
	NDRecordLaserLevel (LOCALPLAYER.laserLevel, MAX_LASER_LEVEL);
LOCALPLAYER.energy = F1_0 * 200;
LOCALPLAYER.shields = F1_0 * 200;
MultiSendShields ();
LOCALPLAYER.flags |= PLAYER_FLAGS_QUAD_LASERS;
LOCALPLAYER.laserLevel = MAX_SUPER_LASER_LEVEL;
if (gameData.app.nGameMode & GM_HOARD)
	LOCALPLAYER.secondaryAmmo[PROXMINE_INDEX] = 12;
else if (gameData.app.nGameMode & GM_ENTROPY)
	LOCALPLAYER.secondaryAmmo[PROXMINE_INDEX] = 15;
UpdateLaserWeaponInfo ();
}
//#endif

//------------------------------------------------------------------------------

int PickupEnergyBoost (CObject *objP, int nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

if (playerP->energy < MAX_ENERGY) {
	fix boost = 3 * F1_0 * (NDL - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	playerP->energy += boost;
	if (playerP->energy > MAX_ENERGY)
		playerP->energy = MAX_ENERGY;
	if (ISLOCALPLAYER (nPlayer))
		PowerupBasic (15,15,7, ENERGY_SCORE, "%s %s %d",
						 TXT_ENERGY, TXT_BOOSTED_TO, X2IR (playerP->energy));
	return 1;
	} 
else if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_ENERGY);
return 0;
}

//------------------------------------------------------------------------------

int PickupShieldBoost (CObject *objP, int nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

if (playerP->shields < MAX_SHIELDS) {
	fix boost = 3 * F1_0 * (NDL - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	playerP->shields += boost;
	if (playerP->shields > MAX_SHIELDS)
		playerP->shields = MAX_SHIELDS;
	if (ISLOCALPLAYER (nPlayer)) {
		PowerupBasic (0, 0, 15, SHIELD_SCORE, "%s %s %d",
							TXT_SHIELD, TXT_BOOSTED_TO, X2IR (playerP->shields));
		MultiSendShields ();
		}
	return 1;
	}
else if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
return 0;
}

//	-----------------------------------------------------------------------------

int PickupCloakingDevice (CObject *objP, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (!gameOpts->gameplay.bInventory || (IsMultiGame && !IsCoopGame)) 
	return -ApplyCloak (1, nPlayer);
if (playerP->nCloaks < MAX_INV_ITEMS) {
	playerP->nCloaks++;
	return 1;
	}
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s", TXT_INVENTORY_FULL);
return 0;
}

//	-----------------------------------------------------------------------------

int PickupInvulnerability (CObject *objP, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (!gameOpts->gameplay.bInventory || (IsMultiGame && !IsCoopGame)) 
	return -ApplyInvul (1, nPlayer);
if (playerP->nInvuls < MAX_INV_ITEMS) {
	playerP->nInvuls++;
	return 1;
	}
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s", TXT_INVENTORY_FULL);
return 0;
}

//------------------------------------------------------------------------------

int PickupExtraLife (CObject *objP, int nPlayer)
{
gameData.multiplayer.players [nPlayer].lives++;
if (nPlayer == gameData.multiplayer.nLocalPlayer)
	PowerupBasic (15, 15, 15, 0, TXT_EXTRA_LIFE);
return 1;
}

//	-----------------------------------------------------------------------------

int PickupHoardOrb (CObject *objP, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (IsHoardGame) {
	if (playerP->secondaryAmmo [PROXMINE_INDEX] < 12) {
		if (ISLOCALPLAYER (nPlayer)) {
			MultiSendGotOrb ((char) gameData.multiplayer.nLocalPlayer);
			PowerupBasic (15, 0, 15, 0, "Orb!!!", nPlayer);
			}
		playerP->secondaryAmmo [PROXMINE_INDEX]++;
		playerP->flags |= PLAYER_FLAGS_FLAG;
		return 1;
		}
	}
else if (IsEntropyGame) {
	if (objP->info.nCreator != GetTeam ((char) gameData.multiplayer.nLocalPlayer) + 1) {
		if ((extraGameInfo [1].entropy.nVirusStability < 2) ||
			 ((extraGameInfo [1].entropy.nVirusStability < 3) && 
			 ((gameData.segs.xSegments [objP->info.nSegment].owner != objP->info.nCreator) ||
			 (gameData.segs.segment2s [objP->info.nSegment].m_special != SEGMENT_IS_ROBOTMAKER))))
			objP->info.xLifeLeft = -1;	//make orb disappear if touched by opposing team CPlayerData
		}
	else if (!extraGameInfo [1].entropy.nMaxVirusCapacity ||
				(playerP->secondaryAmmo [PROXMINE_INDEX] < playerP->secondaryAmmo [SMARTMINE_INDEX])) {
		if (ISLOCALPLAYER (nPlayer)) {
			MultiSendGotOrb ((char) gameData.multiplayer.nLocalPlayer);
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

int PickupEquipment (CObject *objP, int nEquipment, const char *pszHave, const char *pszGot, int nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;
	int		id, bUsed = 0;

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
		MultiSendPlaySound (gameData.objs.pwrUp.info [id].hitSound, F1_0);
		DigiPlaySample ((short) gameData.objs.pwrUp.info [id].hitSound, F1_0);
		PowerupBasic (15, 0, 15, 0, pszGot, nPlayer);
		}
	bUsed = -1;
	}
return bUsed;
}

//	-----------------------------------------------------------------------------

int PickupHeadlight (CObject *objP, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;
	char		szTemp [50];

sprintf (szTemp, TXT_GOT_HEADLIGHT, (EGI_FLAG (headlight.bAvailable, 0, 0, 1) && gameOpts->gameplay.bHeadlightOnWhenPickedUp) ? TXT_ON : TXT_OFF);
HUDInitMessage (szTemp);
int bUsed = PickupEquipment (objP, PLAYER_FLAGS_HEADLIGHT, TXT_THE_HEADLIGHT, szTemp, nPlayer);
if (bUsed >= 0)
	return bUsed;
if (ISLOCALPLAYER (nPlayer)) {
	if (EGI_FLAG (headlight.bAvailable, 0, 0, 1)  && gameOpts->gameplay.bHeadlightOnWhenPickedUp)
		playerP->flags |= PLAYER_FLAGS_HEADLIGHT_ON;
	if IsMultiGame
		MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int PickupFullMap (CObject *objP, int nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_FULLMAP, TXT_THE_FULLMAP, TXT_GOT_FULLMAP, nPlayer) ? 1 : 0;
}


//	-----------------------------------------------------------------------------

int PickupConverter (CObject *objP, int nPlayer)
{
	char		szTemp [50];

sprintf (szTemp, TXT_GOT_CONVERTER, KeyToASCII (GetKeyValue (54)));
HUDInitMessage (szTemp);
return PickupEquipment (objP, PLAYER_FLAGS_CONVERTER, TXT_THE_CONVERTER, szTemp, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int PickupAmmoRack (CObject *objP, int nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_AMMO_RACK, TXT_THE_AMMORACK, TXT_GOT_AMMORACK, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int PickupAfterburner (CObject *objP, int nPlayer)
{
	int bUsed = PickupEquipment (objP, PLAYER_FLAGS_AFTERBURNER, TXT_THE_BURNER, TXT_GOT_BURNER, nPlayer);
	
if (bUsed >= 0)
	return bUsed;
gameData.physics.xAfterburnerCharge = f1_0;
return 1;
}

//	-----------------------------------------------------------------------------

int PickupSlowMotion (CObject *objP, int nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_SLOWMOTION, TXT_THE_SLOWMOTION, TXT_GOT_SLOWMOTION, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int PickupBulletTime (CObject *objP, int nPlayer)
{
return PickupEquipment (objP, PLAYER_FLAGS_BULLETTIME, TXT_THE_BULLETTIME, TXT_GOT_BULLETTIME, nPlayer) != 0;
}

//------------------------------------------------------------------------------

int PickupKey (CObject *objP, int nKey, const char *pszKey, int nPlayer)
{
if (ISLOCALPLAYER (nPlayer)) {
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

	if (playerP->flags & nKey)
		return 0;
	MultiSendPlaySound (gameData.objs.pwrUp.info [objP->info.nId].hitSound, F1_0);
	DigiPlaySample ((short) gameData.objs.pwrUp.info[objP->info.nId].hitSound, F1_0);
	playerP->flags |= nKey;
	PowerupBasic (15, 0, 0, KEY_SCORE, "%s %s", pszKey, TXT_ACCESS_GRANTED);
	InvalidateEscortGoal ();
	return IsMultiGame == 0;
	}
return 0;
}

//------------------------------------------------------------------------------

int PickupFlag (CObject *objP, int nThisTeam, int nOtherTeam, const char *pszFlag, int nPlayer)
{
if (ISLOCALPLAYER (nPlayer)) {
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;
	if (gameData.app.nGameMode & GM_CAPTURE) {
		if (GetTeam ((char) gameData.multiplayer.nLocalPlayer) == nOtherTeam) {
			PowerupBasic (15, 0, 15, 0, nOtherTeam ? reinterpret_cast<char*> ("RED FLAG!") : reinterpret_cast<char*> ("BLUE FLAG!"), nPlayer);
			playerP->flags |= PLAYER_FLAGS_FLAG;
			gameData.pig.flags [nThisTeam].path.Reset (10, -1);
			MultiSendGotFlag ((char) gameData.multiplayer.nLocalPlayer);
			return 1;
			}
		if (GetTeam ((char) gameData.multiplayer.nLocalPlayer) == nThisTeam) {
			ReturnFlagHome (objP);
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void UsePowerup (int id)
{
	int	bApply;

if ((bApply = (id < 0)))
	id = -id;
if (id >= MAX_POWERUP_TYPES_D2)
	id = POW_AFTERBURNER;
if (gameData.objs.pwrUp.info [id].hitSound > -1) {
	if (!bApply && (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame)) && ((id == POW_CLOAK) || (id == POW_INVUL)))
		id = POW_SHIELD_BOOST;
	if (IsMultiGame) // Added by Rob, take this out if it turns out to be not good for net games!
		MultiSendPlaySound (gameData.objs.pwrUp.info [id].hitSound, F1_0);
	DigiPlaySample ((short) gameData.objs.pwrUp.info [id].hitSound, F1_0);
	}
MultiSendWeapons (1);
}

//------------------------------------------------------------------------------

int ApplyInvul (int bForce, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + ((nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer);

if (!(bForce || ((gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame)) && playerP->nInvuls)))
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
	if IsMultiGame
		MultiSendInvul ();
	PowerupBasic (7, 14, 21, INVULNERABILITY_SCORE, "%s!", TXT_INVULNERABILITY);
	SetupSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
	UsePowerup (-POW_INVUL);
	}
return 1;
}

//------------------------------------------------------------------------------

int ApplyCloak (int bForce, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + ((nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer);

if (!(bForce || ((gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame)) && playerP->nCloaks)))
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
	if IsMultiGame
		MultiSendCloak ();
	PowerupBasic (-10,-10,-10, CLOAK_SCORE, "%s!", TXT_CLOAKING_DEVICE);
	UsePowerup (-POW_CLOAK);
	}
return 1;
}

//------------------------------------------------------------------------------

static inline int PowerupCount (int nId)
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

#if defined (_WIN32) && !defined (_DEBUG)
typedef int (__fastcall * pPickupGun) (CObject *, int, int);
typedef int (__fastcall * pPickupMissile) (CObject *, int, int, int);
typedef int (__fastcall * pPickupEquipment) (CObject *, int);
typedef int (__fastcall * pPickupKey) (CObject *, int, const char *, int);
typedef int (__fastcall * pPickupFlag) (CObject *, int, int, const char *, int);
#else
typedef int (* pPickupGun) (CObject *, int, int);
typedef int (* pPickupMissile) (CObject *, int, int, int);
typedef int (* pPickupEquipment) (CObject *, int);
typedef int (* pPickupKey) (CObject *, int, const char *, int);
typedef int (* pPickupFlag) (CObject *, int, int, const char *, int);
#endif


//	returns true if powerup consumed
int DoPowerup (CObject *objP, int nPlayer)
{
	CPlayerData	*playerP;
	int		bUsed = 0;
	int		bSpecialUsed = 0;		//for when hitting vulcan cannon gets vulcan ammo
	int		bLocalPlayer;
	int		nId, nType;

if (nPlayer < 0)
	nPlayer = gameData.multiplayer.nLocalPlayer;
playerP = gameData.multiplayer.players + nPlayer;
if (SPECTATOR (OBJECTS + playerP->nObject))
	return 0;
bLocalPlayer = (nPlayer == gameData.multiplayer.nLocalPlayer);
if (bLocalPlayer &&
	 (gameStates.app.bPlayerIsDead || 
	  (gameData.objs.consoleP->info.nType == OBJ_GHOST) || 
	  (playerP->shields < 0)))
	return 0;
if (objP->cType.powerupInfo.xCreationTime > gameData.time.xGame)		//gametime wrapped!
	objP->cType.powerupInfo.xCreationTime = 0;				//allow CPlayerData to pick up
if ((objP->cType.powerupInfo.nFlags & PF_SPAT_BY_PLAYER) && 
	 (objP->cType.powerupInfo.xCreationTime > 0) && 
	 (gameData.time.xGame < objP->cType.powerupInfo.xCreationTime + I2X (2)))
	return 0;		//not enough time elapsed
gameData.hud.bPlayerMessage = 0;	//	Prevent messages from going to HUD if -PlayerMessages switch is set
nId = objP->info.nId;
#if 1

nType = powerupType [nId];
if (nType == POWERUP_IS_GUN) {
	bUsed = ((pPickupGun) (pickupHandler [nId])) (objP, powerupToWeapon [nId], nPlayer);
	if ((bUsed < 0) && ((nId == POW_VULCAN) || (nId == POW_GAUSS))) {
		bUsed = -bUsed - 1;
		bSpecialUsed = 1;
		nId = POW_VULCAN_AMMO;
		}
	}
else if (nType == POWERUP_IS_MISSILE) {
	bUsed = ((pPickupMissile) (pickupHandler [nId])) (objP, powerupToWeapon [nId], PowerupCount (nId), nPlayer);
	}
else if (nType == POWERUP_IS_KEY) {
	int nKey = nId - POW_KEY_BLUE;
	bUsed = ((pPickupKey) (pickupHandler [nId])) (objP, PLAYER_FLAGS_BLUE_KEY << nKey, GAMETEXT (12 + nKey), nPlayer);
	}
else if (nType == POWERUP_IS_EQUIPMENT) {
	bUsed = ((pPickupEquipment) (pickupHandler [nId])) (objP, nPlayer);
	}
else if (nType == POWERUP_IS_FLAG) {
	int nFlag = nId - POW_BLUEFLAG;
	bUsed = ((pPickupFlag) (pickupHandler [nId])) (objP, nFlag, !nFlag, GT (1077 + nFlag), nPlayer);
	}
else
	return 0;

#else

switch (objP->nId) {
	case POW_EXTRA_LIFE:
		bUsed = PickupExtraLife (objP, nPlayer);
		break;

	case POW_ENERGY:
		bUsed = PickupEnergyBoost (objP, nPlayer);
		break;

	case POW_SHIELD_BOOST:
		bUsed = PickupShieldBoost (objP, nPlayer);
		break;

	case POW_LASER:
		bUsed = PickupLaser (objP, 0, 1, NULL, nPlayer);
		break;

	case POW_QUADLASER:
		bUsed = PickupQuadLaser (objP, 0, 1, NULL, nPlayer);
		break;

	case POW_SUPERLASER:
		bUsed = PickupSuperLaser (objP, 0, 1, NULL, nPlayer);
		break;

	case POW_VULCAN:
		if (0 > (bUsed = PickupGatlingGun (objP, VULCAN_INDEX, 1, NULL, nPlayer))) {
			bUsed = -bUsed - 1;
			bSpecialUsed = 1;
			nId = POW_VULCAN_AMMO;
			}
		break;

	case POW_GAUSS: 
		if (0 > (bUsed = PickupGatlingGun (objP, GAUSS_INDEX, 1, NULL, nPlayer))) {
			bUsed = -bUsed - 1;
			bSpecialUsed = 1;
			nId = POW_VULCAN_AMMO;
			}
		break;

	case POW_SPREADFIRE:
		bUsed = PickupGun (SPREADFIRE_INDEX, nPlayer);
		break;

	case POW_PLASMA:
		bUsed = PickupGun (PLASMA_INDEX, nPlayer);
		break;

	case POW_FUSION:
		bUsed = PickupGun (FUSION_INDEX, nPlayer);
		break;

	case POW_HELIX:
		bUsed = PickupGun (HELIX_INDEX, nPlayer);
		break;

	case POW_PHOENIX:
		bUsed = PickupGun (PHOENIX_INDEX, nPlayer);
		break;

	case POW_OMEGA:
		bUsed = PickupGun (OMEGA_INDEX, nPlayer);
		break;

	case POW_CONCUSSION_1:
		bUsed = PickupSecondary (objP, CONCUSSION_INDEX, 1, NULL, nPlayer);
		break;

	case POW_CONCUSSION_4:
		bUsed = PickupSecondary (objP, CONCUSSION_INDEX, 4, NULL, nPlayer);
		break;

	case POW_HOMINGMSL_1:
		bUsed = PickupSecondary (objP, HOMING_INDEX, 1, nPlayer);
		break;

	case POW_HOMINGMSL_4:
		bUsed = PickupSecondary (objP, HOMING_INDEX, 4, nPlayer);
		break;

	case POW_PROXMINE:
		bUsed = PickupSecondary (objP, PROXMINE_INDEX, 4, nPlayer);
		break;

	case POW_SMARTMSL:
		bUsed = PickupSecondary (objP, SMART_INDEX, 1, nPlayer);
		break;

	case POW_MEGAMSL:
		bUsed = PickupSecondary (objP, MEGA_INDEX, 1, nPlayer);
		break;

	case POW_FLASHMSL_1:
		bUsed = PickupSecondary (objP, FLASHMSL_INDEX, 1, nPlayer);
		break;

	case POW_FLASHMSL_4:
		bUsed = PickupSecondary (objP, FLASHMSL_INDEX, 4, nPlayer);
		break;

	case POW_GUIDEDMSL_1:
		bUsed = PickupSecondary (objP, GUIDED_INDEX, 1, nPlayer);
		break;

	case POW_GUIDEDMSL_4:
		bUsed = PickupSecondary (objP, GUIDED_INDEX, 4, nPlayer);
		break;

	case POW_SMARTMINE:
		bUsed = PickupSecondary (objP, SMARTMINE_INDEX, 4, nPlayer);
		break;

	case POW_MERCURYMSL_1:
		bUsed = PickupSecondary (objP, MERCURY_INDEX, 1, nPlayer);
		break;

	case POW_MERCURYMSL_4:
		bUsed = PickupSecondary (objP, MERCURY_INDEX, 4, nPlayer);
		break;

	case POW_EARTHSHAKER:
		bUsed = PickupSecondary (objP, EARTHSHAKER_INDEX, 1, nPlayer);
		break;

	case POW_KEY_BLUE:
		bUsed = PickupKey (objP, PLAYER_FLAGS_BLUE_KEY, 1, TXT_BLUE, nPlayer);
		break;

	case POW_KEY_RED:
		bUsed = PickupKey (objP, PLAYER_FLAGS_RED_KEY, 1, TXT_RED, nPlayer);
		break;

	case POW_KEY_GOLD:
		bUsed = PickupKey (objP, PLAYER_FLAGS_GOLD_KEY, 1, TXT_YELLOW, nPlayer);
		break;

	case POW_VULCAN_AMMO:
		bUsed = PickUpVulcanAmmo (objP, nPlayer);
		break;

	case POW_CLOAK:
		bUsed = PickupCloakingDevice (objP, 0, 1, NULL, nPlayer);
		break;

	case POW_INVUL:
		bUsed = PickupInvulnerability (objP, 0, 1, NULL, nPlayer);
		break;

#if DBG
	case POW_MEGAWOW:
		DoMegaWowPowerup (50);
		bUsed = 1;
		break;
#endif

	case POW_FULL_MAP:
		bUsed = PickupFullMap (nPlayer);
		break;

	case POW_CONVERTER:
		bUsed = PickupConverter (nPlayer);
		break;

	case POW_AMMORACK:
		bUsed = PickupAmmoRack (nPlayer);
		break;

	case POW_AFTERBURNER:
		bUsed = PickupAfterburner (nPlayer);
		break;

	case POW_SLOWMOTION:
		bUsed = PickupSlowMotion (nPlayer);
		break;

	case POW_BULLETTIME:
		bUsed = PickupBulletTime (nPlayer);
		break;

	case POW_HEADLIGHT:
		bUsed = PickupHeadlight (nPlayer);
		break;

	case POW_BLUEFLAG:
		bUsed = PickupFlag (objP, TEAM_BLUE, TEAM_RED, "BLUE FLAG!", nPlayer);
		break;

	case POW_REDFLAG:
		bUsed = PickupFlag (objP, TEAM_RED, TEAM_BLUE, "RED FLAG!", nPlayer);
		break;

	case POW_HOARD_ORB:
		bUsed = PickupHoardOrb (objP, 0, 1, nPlayer);
		break;

	default:
		break;
	}

#endif
//always say bUsed, until physics problem (getting stuck on unused powerup)
//is solved.  Note also the break statements above that are commented out
//!!	bUsed = 1;

if (bUsed || bSpecialUsed) {
	UsePowerup (nId * (bUsed ? bUsed : bSpecialUsed));
	if (IsMultiGame)
		MultiSendWeapons (1);
	}
gameData.hud.bPlayerMessage = 1;
return bUsed;
}

//------------------------------------------------------------------------------

int SpawnPowerup (CObject *spitterP, ubyte nId, int nCount)
{
	int			i;
	short			nObject;
	CFixVector	velSave;
	CObject		*objP;

if (nCount <= 0)
	return 0;
gameData.multiplayer.powerupsInMine [nId] += nCount;
velSave = spitterP->mType.physInfo.velocity;
spitterP->mType.physInfo.velocity.SetZero();
for (i = nCount; i; i--) {
	nObject = SpitPowerup (spitterP, nId, d_rand ());
	objP = OBJECTS + nObject;
	MultiSendCreatePowerup (nId, objP->info.nSegment, nObject, &objP->info.position.vPos);
	}
spitterP->mType.physInfo.velocity = velSave;
return nCount;
}

//------------------------------------------------------------------------------

void SpawnLeftoverPowerups (short nObject)
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
	CPlayerData	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
	CObject	*objP = OBJECTS + playerP->nObject;

if (SpawnPowerup (objP, POW_CLOAK, playerP->nCloaks - MAX_INV_ITEMS))
	playerP->nCloaks = MAX_INV_ITEMS;
if (SpawnPowerup (objP, POW_INVUL, playerP->nInvuls - MAX_INV_ITEMS))
	playerP->nInvuls = MAX_INV_ITEMS;
}

//-----------------------------------------------------------------------------

void InitPowerupTables (void)
{
memset (powerupToWeapon, 0xff, sizeof (powerupToWeapon));

powerupToWeapon [POW_LASER] = LASER_INDEX;
powerupToWeapon [POW_VULCAN] = VULCAN_INDEX;
powerupToWeapon [POW_SPREADFIRE] = SPREADFIRE_INDEX;
powerupToWeapon [POW_PLASMA] = PLASMA_INDEX;
powerupToWeapon [POW_FUSION] = FUSION_INDEX;
powerupToWeapon [POW_GAUSS] = GAUSS_INDEX;
powerupToWeapon [POW_HELIX] = HELIX_INDEX;
powerupToWeapon [POW_PHOENIX] = PHOENIX_INDEX;
powerupToWeapon [POW_OMEGA] = OMEGA_INDEX;
powerupToWeapon [POW_SUPERLASER] = SUPER_LASER_INDEX;
powerupToWeapon [POW_CONCUSSION_1] = 
powerupToWeapon [POW_CONCUSSION_4] = CONCUSSION_INDEX;
powerupToWeapon [POW_PROXMINE] = PROXMINE_INDEX;
powerupToWeapon [POW_HOMINGMSL_1] = 
powerupToWeapon [POW_HOMINGMSL_4] = HOMING_INDEX;
powerupToWeapon [POW_SMARTMSL] = SMART_INDEX;
powerupToWeapon [POW_MEGAMSL] = MEGA_INDEX;
powerupToWeapon [POW_FLASHMSL_1] = 
powerupToWeapon [POW_FLASHMSL_4] = FLASHMSL_INDEX;
powerupToWeapon [POW_GUIDEDMSL_1] = 
powerupToWeapon [POW_GUIDEDMSL_4] = GUIDED_INDEX;
powerupToWeapon [POW_SMARTMINE] = SMARTMINE_INDEX;
powerupToWeapon [POW_MERCURYMSL_1] = 
powerupToWeapon [POW_MERCURYMSL_4] = MERCURY_INDEX;
powerupToWeapon [POW_EARTHSHAKER] = EARTHSHAKER_INDEX;

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
powerupClass [POW_TURBO] = 
powerupClass [POW_INVUL] = 
powerupClass [POW_FULL_MAP] = 
powerupClass [POW_CONVERTER] = 
powerupClass [POW_AMMORACK] = 
powerupClass [POW_AFTERBURNER] = 
powerupClass [POW_HEADLIGHT] = 3;

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
powerupType [POW_MONSTERBALL] = (ubyte) POWERUP_IS_UNDEFINED;

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
powerupType [POW_VULCAN_AMMO] = (ubyte) POWERUP_IS_EQUIPMENT;

powerupType [POW_BLUEFLAG] =
powerupType [POW_REDFLAG] = (ubyte) POWERUP_IS_FLAG;

powerupType [POW_KEY_BLUE] =
powerupType [POW_KEY_RED] =
powerupType [POW_KEY_GOLD] = (ubyte) POWERUP_IS_KEY;

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
powerupType [POW_QUADLASER] = (ubyte) POWERUP_IS_GUN;

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
powerupType [POW_EARTHSHAKER] = (ubyte) POWERUP_IS_MISSILE;

}

//-----------------------------------------------------------------------------

char PowerupToWeapon (short nPowerup, int *nType)
{
*nType = powerupClass [nPowerup];
return powerupToWeapon [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupClass (short nPowerup)
{
return powerupClass [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupToWeaponCount (short nPowerup)
{
return powerupToWeaponCount [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupToObject (short nPowerup)
{
return powerupToObject [nPowerup];
}

//-----------------------------------------------------------------------------

short PowerupToModel (short nPowerup)
{
return powerupToModel [nPowerup];
}

//-----------------------------------------------------------------------------

short WeaponToModel (short nWeapon)
{
return weaponToModel [nWeapon];
}

//-----------------------------------------------------------------------------

short PowerupsOnShips (int nPowerup)
{
	int	nType;
	short h, i, nWeapon = PowerupToWeapon (nPowerup, &nType);
	CPlayerData	*playerP = gameData.multiplayer.players;

if (nWeapon < 0)
	return 0;
for (h = i = 0; i < gameData.multiplayer.nPlayers; i++, playerP++) {
	if ((i == gameData.multiplayer.nLocalPlayer) && 
		 (gameStates.app.bPlayerExploded || gameStates.app.bPlayerIsDead))
		continue;
	if (playerP->shields < 0)
		continue;
	if (!playerP->connected)
		continue;
	if (nType == 2)
		h += playerP->secondaryAmmo [nWeapon];
	else {
		if (nWeapon == LASER_INDEX) {
			if (playerP->laserLevel > MAX_LASER_LEVEL)
				h = MAX_LASER_LEVEL + 1;
			else
				h += playerP->laserLevel + 1;
			}
		else if (nWeapon == SUPER_LASER_INDEX) {
			if (playerP->laserLevel > MAX_LASER_LEVEL)
				h += playerP->laserLevel - MAX_LASER_LEVEL;
			}
		else if (playerP->primaryWeaponFlags & (1 << nWeapon))
			h++;
		}
	}
return h;
} 

//------------------------------------------------------------------------------
#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads n tPowerupTypeInfo structs from a CFile
 */
extern int ReadPowerupTypeInfos (tPowerupTypeInfo *pti, int n, CFile& cf)
{
	int i;

for (i = 0; i < n; i++) {
	pti [i].nClipIndex = cf.ReadInt ();
	pti [i].hitSound = cf.ReadInt ();
	pti [i].size = cf.ReadFix ();
	pti [i].light = cf.ReadFix ();
	}
return i;
}
#endif

//------------------------------------------------------------------------------
//eof
