/* $Id: powerup.c,v 1.7 2003/10/10 09:36:35 btb Exp $ */
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

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "mono.h"
#include "error.h"

#include "inferno.h"
#include "object.h"
#include "game.h"
#include "key.h"

#include "fireball.h"
#include "powerup.h"
#include "gauges.h"

#include "sounds.h"
#include "player.h"

#include "wall.h"
#include "text.h"
#include "weapon.h"
#include "laser.h"
#include "scores.h"
#include "multi.h"
#include "lighting.h"
#include "controls.h"
#include "hudmsg.h"
#include "kconfig.h"
#include "render.h"

#include "newdemo.h"
#include "escort.h"
#include "network.h"
#include "sphere.h"

#ifdef EDITOR
#include "gr.h"	//	for powerup outline drawing
#include "editor/editor.h"
#endif

int ReturnFlagHome (tObject *pObj);
void InvalidateEscortGoal (void);
char GetKeyValue (char);
void CheckToUsePrimary (int);
void MultiSendGotFlag (char);

//------------------------------------------------------------------------------

void UpdatePowerupClip (tVideoClip *vcP, tVClipInfo *vciP, int nObject)
{
	static fix	xPowerupTime = 0;

	int			nFrames = vcP->nFrameCount;
	fix			xFudge = (xPowerupTime * (nObject & 3)) >> 4;
	grsBitmap	*bmP;
	
xPowerupTime += gameData.time.xFrame;

if (vcP->flags & WCF_ALTFMT) {
	if (vcP->flags & WCF_INITIALIZED) {
		bmP = BM_OVERRIDE (gameData.pig.tex.bitmaps [0] + vcP->frames [0].index);
		nFrames = ((bmP->bmType != BM_TYPE_ALT) && BM_PARENT (bmP)) ? BM_FRAMECOUNT (BM_PARENT (bmP)) : BM_FRAMECOUNT (bmP);
		}
	else {
		bmP = SetupHiresAnim ((short *) vcP->frames, nFrames, -1, 0, 1, &nFrames);
		if (!bmP)
			vcP->flags &= ~WCF_ALTFMT;
		else if (!gameOpts->ogl.bGlTexMerge)
			vcP->flags &= ~WCF_ALTFMT;
		else 
			vcP->flags |= WCF_INITIALIZED;
		}
	}
vciP->xFrameTime -= xPowerupTime + xFudge;
while (vciP->xFrameTime < 0) {
	vciP->xFrameTime += vcP->xFrameTime;
	if (nObject & 1) {
		if (0 > -- (vciP->nCurFrame))
			vciP->nCurFrame = nFrames - 1;
		}
	else {
		if (++ (vciP->nCurFrame) >= nFrames)
			vciP->nCurFrame = 0;
		}
	}
xPowerupTime = 0;
}

//------------------------------------------------------------------------------

void UpdateFlagClips (void)
{
if (!gameOpts->app.bDemoData) {
	UpdatePowerupClip (gameData.pig.flags [0].vcP, &gameData.pig.flags [0].vci, 0);
	UpdatePowerupClip (gameData.pig.flags [1].vcP, &gameData.pig.flags [1].vci, 0);
	}
}

//------------------------------------------------------------------------------
//process this powerup for this frame
void DoPowerupFrame (tObject *objP)
{
//if (gameStates.app.b40fpsTick) 
	tVClipInfo	*vciP = &objP->rType.vClipInfo;
	tVideoClip	*vcP = gameData.eff.vClips [0] + vciP->nClipIndex;
	int			i = OBJ_IDX (objP);

UpdatePowerupClip (vcP, vciP, i);
if (objP->lifeleft <= 0) {
	ObjectCreateExplosion (objP->nSegment, &objP->position.vPos, F1_0 * 7 / 2, VCLIP_POWERUP_DISAPPEARANCE);
	if (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].nSound > -1)
		DigiLinkSoundToObject (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].nSound, i, 0, F1_0);
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

	GrSetColorRGB (255, 255, 255, 255);

	GrLine (blob_vertices[0], blob_vertices[1], blob_vertices[2], blob_vertices[3]);
	GrLine (blob_vertices[2], blob_vertices[3], blob_vertices[4], blob_vertices[5]);
	GrLine (blob_vertices[4], blob_vertices[5], v3x, v3y);

	GrLine (v3x, v3y, blob_vertices[0], blob_vertices[1]);
}
#endif

//------------------------------------------------------------------------------

void DrawPowerup (tObject *objP)
{
if (objP->nType == OBJ_MONSTERBALL)
	DrawMonsterball (objP, 1.0f, 0.5f, 0.0f, 0.9f);
else {
		tBitmapIndex	*frameP = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].frames;
		int				iFrame = objP->rType.vClipInfo.nCurFrame;
#ifdef EDITOR
		blob_vertices[0] = 0x80000;
#endif
	DrawObjectBlob (objP, *frameP, frameP [iFrame], iFrame, NULL, 0);
#ifdef EDITOR
	if ((gameStates.app.nFunctionMode == FMODE_EDITOR) && (CurObject_index == OBJ_IDX (objP)))
		if (blob_vertices[0] != 0x80000)
			DrawBlobOutline ();
#endif
	}
}

//------------------------------------------------------------------------------

void _CDECL_ PowerupBasic (int redAdd, int greenAdd, int blueAdd, int score, char *format, ...)
{
	char		text[120];
	va_list	args;

va_start (args, format);
vsprintf (text, format, args);
va_end (args);
PALETTE_FLASH_ADD (redAdd, greenAdd, blueAdd);
HUDInitMessage (text);
//mprintf_gameData.objs.pwrUp.Info ();
AddPointsToScore (score);
}

//------------------------------------------------------------------------------

//#ifndef RELEASE
//	Give the megawow powerup!
void DoMegaWowPowerup (int quantity)
{
	int i;

PowerupBasic (30, 0, 30, 1, "MEGA-WOWIE-ZOWIE!");
gameData.multi.players [gameData.multi.nLocalPlayer].primaryWeaponFlags = 0xffff ^ HAS_FLAG (SUPER_LASER_INDEX);		//no super laser
gameData.multi.players [gameData.multi.nLocalPlayer].secondaryWeaponFlags = 0xffff;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	gameData.multi.players [gameData.multi.nLocalPlayer].primaryAmmo[i] = VULCAN_AMMO_MAX;
for (i = 0; i < 3; i++)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo[i] = quantity;
for (i = 3; i < MAX_SECONDARY_WEAPONS; i++)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo[i] = quantity/5;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLaserLevel (gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel, MAX_LASER_LEVEL);
gameData.multi.players [gameData.multi.nLocalPlayer].energy = F1_0 * 200;
gameData.multi.players [gameData.multi.nLocalPlayer].shields = F1_0 * 200;
MultiSendShields ();
gameData.multi.players [gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_QUAD_LASERS;
gameData.multi.players [gameData.multi.nLocalPlayer].laserLevel = MAX_SUPER_LASER_LEVEL;
if (gameData.app.nGameMode & GM_HOARD)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo[PROXIMITY_INDEX] = 12;
else if (gameData.app.nGameMode & GM_ENTROPY)
	gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo[PROXIMITY_INDEX] = 15;
UpdateLaserWeaponInfo ();
}
//#endif

//------------------------------------------------------------------------------

int PickupEnergy (int nPlayer)
{
	tPlayer	*playerP = gameData.multi.players + nPlayer;

if (playerP->energy < MAX_ENERGY) {
	fix boost = 3 * F1_0 * (NDL - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	playerP->energy += boost;
	if (playerP->energy > MAX_ENERGY)
		playerP->energy = MAX_ENERGY;
	if (LOCALPLAYER (nPlayer))
		PowerupBasic (15,15,7, ENERGY_SCORE, "%s %s %d",
						 TXT_ENERGY, TXT_BOOSTED_TO, f2ir (playerP->energy));
	return 1;
	} 
else if (LOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_ENERGY);
return 0;
}

//------------------------------------------------------------------------------

int PickupShield (int nPlayer)
{
	tPlayer	*playerP = gameData.multi.players + nPlayer;

if (playerP->shields < MAX_SHIELDS) {
	fix boost = 3 * F1_0 * (NDL - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	playerP->shields += boost;
	if (playerP->shields > MAX_SHIELDS)
		playerP->shields = MAX_SHIELDS;
	if (LOCALPLAYER (nPlayer)) {
		PowerupBasic (0, 0, 15, SHIELD_SCORE, "%s %s %d",
							TXT_SHIELD, TXT_BOOSTED_TO, f2ir (playerP->shields));
		MultiSendShields ();
		}
	return 1;
	}
else if (LOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
return 0;
}

//------------------------------------------------------------------------------

int PickupKey (tObject *objP, int nKey, char *pszKey, int nPlayer)
{
if (LOCALPLAYER (nPlayer)) {
	tPlayer	*playerP = gameData.multi.players + nPlayer;

	if (playerP->flags & nKey)
		return 0;
	MultiSendPlaySound (gameData.objs.pwrUp.info [objP->id].hitSound, F1_0);
	DigiPlaySample ((short) gameData.objs.pwrUp.info[objP->id].hitSound, F1_0);
	playerP->flags |= nKey;
	PowerupBasic (15, 0, 0, KEY_SCORE, "%s %s", pszKey, TXT_ACCESS_GRANTED);
	InvalidateEscortGoal ();
	return (gameData.app.nGameMode & GM_MULTI) == 0;
	}
return 0;
}

//------------------------------------------------------------------------------

int PickupFlag (tObject *objP, int nThisTeam, int nOtherTeam, char *pszFlag, int nPlayer)
{
if (LOCALPLAYER (nPlayer)) {
	tPlayer	*playerP = gameData.multi.players + nPlayer;
	if (gameData.app.nGameMode & GM_CAPTURE) {
		if (GetTeam ((char) gameData.multi.nLocalPlayer) == nOtherTeam) {
			PowerupBasic (15, 0, 15, 0, nOtherTeam ? "RED FLAG" : "BLUE FLAG!", nPlayer);
			playerP->flags |= PLAYER_FLAGS_FLAG;
			ResetFlightPath (&gameData.pig.flags [nThisTeam].path, 10, -1);
			MultiSendGotFlag ((char) gameData.multi.nLocalPlayer);
			return 1;
			}
		if (GetTeam ((char) gameData.multi.nLocalPlayer) == nThisTeam) {
			ReturnFlagHome (objP);
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int PickupEquipment (tObject *objP, int nEquipment, char *pszHave, char *pszGot, int nPlayer)
{
	tPlayer	*playerP = gameData.multi.players + nPlayer;
	int		bUsed = 0;

if (playerP->flags & nEquipment) {
	if (LOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, pszHave);
	if (!(gameData.app.nGameMode & GM_MULTI))
		bUsed = PickupEnergy (nPlayer);
	} 
else {
	playerP->flags |= nEquipment;
	if (LOCALPLAYER (nPlayer)) {
		MultiSendPlaySound (gameData.objs.pwrUp.info [objP->id].hitSound, F1_0);
		DigiPlaySample ((short) gameData.objs.pwrUp.info [objP->id].hitSound, F1_0);
		PowerupBasic (15, 0, 15, 0, pszGot, nPlayer);
		}
	bUsed = -1;
	}
return bUsed;
}

//------------------------------------------------------------------------------

int PickUpVulcanAmmo (int nPlayer)
{
	int		bUsed=0, max;
	tPlayer	*playerP = gameData.multi.players + nPlayer;

int	pwSave = gameData.weapons.nPrimary;		
// Ugh, save selected primary weapon around the picking up of the ammo.  
// I apologize for this code.  Matthew A. Toschlog
if (PickupAmmo (CLASS_PRIMARY, VULCAN_INDEX, VULCAN_AMMO_AMOUNT, nPlayer)) {
	if (LOCALPLAYER (nPlayer))
		PowerupBasic (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO, nPlayer);
	bUsed = 1;
	} 
else {
	max = nMaxPrimaryAmmo [VULCAN_INDEX];
	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;
	if (LOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %d %s!", TXT_ALREADY_HAVE,f2i ((unsigned) VULCAN_AMMO_SCALE * (unsigned) max), TXT_VULCAN_ROUNDS);
	bUsed = 0;
	}
gameData.weapons.nPrimary = pwSave;
return bUsed;
}

//------------------------------------------------------------------------------

void UsePowerup (int id)
{
	int	bApply;

if (bApply = (id < 0))
	id = -id;
if (gameData.objs.pwrUp.info [id].hitSound > -1) {
	if (!bApply && (gameOpts->gameplay.bInventory && !IsMultiGame) && ((id == POW_CLOAK) || (id == POW_INVULNERABILITY)))
		id = POW_SHIELD_BOOST;
	if (gameData.app.nGameMode & GM_MULTI) // Added by Rob, take this out if it turns out to be not good for net games!
		MultiSendPlaySound (gameData.objs.pwrUp.info[id].hitSound, F1_0);
	DigiPlaySample ((short) gameData.objs.pwrUp.info[id].hitSound, F1_0 );
	}
MultiSendWeapons (1);
}

//------------------------------------------------------------------------------

int ApplyInvul (int bForce, int nPlayer)
{
	tPlayer *playerP = gameData.multi.players + ((nPlayer < 0) ? gameData.multi.nLocalPlayer : nPlayer);

if (!(bForce || ((gameOpts->gameplay.bInventory && !IsMultiGame) && playerP->nInvuls)))
	return 0;
if (playerP->flags & PLAYER_FLAGS_INVULNERABLE) {
	if (LOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_INVULNERABLE);
	return 0;
	}
if (gameOpts->gameplay.bInventory && !IsMultiGame)
	playerP->nInvuls--;
if (LOCALPLAYER (nPlayer)) {
	playerP->invulnerableTime = gameData.time.xGame;
	playerP->flags |= PLAYER_FLAGS_INVULNERABLE;
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendInvul ();
	PowerupBasic (7, 14, 21, INVULNERABILITY_SCORE, "%s!", TXT_INVULNERABILITY);
	SetSpherePulse (gameData.multi.spherePulse + gameData.multi.nLocalPlayer, 0.02f, 0.5f);
	UsePowerup (-POW_INVULNERABILITY);
	}
return 1;
}

//------------------------------------------------------------------------------

int ApplyCloak (int bForce, int nPlayer)
{
	tPlayer *playerP = gameData.multi.players + ((nPlayer < 0) ? gameData.multi.nLocalPlayer : nPlayer);

if (!(bForce || ((gameOpts->gameplay.bInventory && !IsMultiGame) && playerP->nCloaks)))
	return 0;
if (playerP->flags & PLAYER_FLAGS_CLOAKED) {
	if (LOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_CLOAKED);
	return 0;
	}
if (gameOpts->gameplay.bInventory && !IsMultiGame)
	playerP->nCloaks--;
if (LOCALPLAYER (nPlayer)) {
	playerP->cloakTime = gameData.time.xGame;	//	Not!changed by awareness events (like tPlayer fires laser).
	playerP->flags |= PLAYER_FLAGS_CLOAKED;
	AIDoCloakStuff ();
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendCloak ();
	PowerupBasic (-10,-10,-10, CLOAK_SCORE, "%s!", TXT_CLOAKING_DEVICE);
	UsePowerup (-POW_CLOAK);
	}
return 1;
}

//------------------------------------------------------------------------------

//	returns true if powerup consumed
int DoPowerup (tObject *objP, int nPlayer)
{
	tPlayer	*playerP;
	int		bUsed = 0;
	int		bSpecialUsed = 0;		//for when hitting vulcan cannon gets vulcan ammo
	int		bLocalPlayer;
	char		szTemp [50];
	int		id = objP->id;

if (nPlayer < 0)
	nPlayer = gameData.multi.nLocalPlayer;
playerP = gameData.multi.players + nPlayer;
bLocalPlayer = (nPlayer == gameData.multi.nLocalPlayer);
if (bLocalPlayer &&
	 ((gameStates.app.bPlayerIsDead) || 
	  (gameData.objs.console->nType == OBJ_GHOST) || 
	  (playerP->shields < 0)))
	return 0;
if (objP->cType.powerupInfo.creationTime > gameData.time.xGame)		//gametime wrapped!
	objP->cType.powerupInfo.creationTime = 0;				//allow tPlayer to pick up
if ((objP->cType.powerupInfo.flags & PF_SPAT_BY_PLAYER) && 
	 (objP->cType.powerupInfo.creationTime > 0) && 
	 (gameData.time.xGame < objP->cType.powerupInfo.creationTime+i2f (2)))
	return 0;		//not enough time elapsed
gameData.hud.bPlayerMessage = 0;	//	Prevent messages from going to HUD if -PlayerMessages switch is set
switch (objP->id) {
	case POW_EXTRA_LIFE:
		playerP->lives++;
		if (bLocalPlayer)
			PowerupBasic (15, 15, 15, 0, TXT_EXTRA_LIFE);
		bUsed = 1;
		break;

	case POW_ENERGY:
		bUsed = PickupEnergy (nPlayer);
		break;

	case POW_SHIELD_BOOST:
		bUsed = PickupShield (nPlayer);
		break;

	case POW_LASER:
		if (playerP->laserLevel >= MAX_LASER_LEVEL) {
			//playerP->laserLevel = MAX_LASER_LEVEL;
			if (bLocalPlayer)
				HUDInitMessage (TXT_MAXED_OUT, TXT_LASER);
			}
		else {
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordLaserLevel ((sbyte) playerP->laserLevel, (sbyte) playerP->laserLevel + 1);
			playerP->laserLevel++;
			PowerupBasic (10, 0, 10, LASER_SCORE, "%s %s %d", TXT_LASER, TXT_BOOSTED_TO, playerP->laserLevel+1);
			UpdateLaserWeaponInfo ();
			PickupPrimary (LASER_INDEX, nPlayer);
			bUsed = 1;
			}
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_MISSILE_1:
		bUsed = PickupSecondary (objP, CONCUSSION_INDEX, 1, nPlayer);
		break;

	case POW_MISSILE_4:
		bUsed = PickupSecondary (objP, CONCUSSION_INDEX, 4, nPlayer);
		break;

	case POW_KEY_BLUE:
		bUsed = PickupKey (objP, PLAYER_FLAGS_BLUE_KEY, TXT_BLUE, nPlayer);
		break;

	case POW_KEY_RED:
		bUsed = PickupKey (objP, PLAYER_FLAGS_RED_KEY, TXT_RED, nPlayer);
		break;

	case POW_KEY_GOLD:
		bUsed = PickupKey (objP, PLAYER_FLAGS_GOLD_KEY, TXT_YELLOW, nPlayer);
		break;

	case POW_QUAD_FIRE:
		if (!(playerP->flags & PLAYER_FLAGS_QUAD_LASERS)) {
			playerP->flags |= PLAYER_FLAGS_QUAD_LASERS;
			PowerupBasic (15, 15, 7, QUAD_FIRE_SCORE, "%s!", TXT_QUAD_LASERS);
			UpdateLaserWeaponInfo ();
			bUsed = 1;
			}
		else
			HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, TXT_QUAD_LASERS);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_VULCAN_WEAPON:
	case	POW_GAUSS_WEAPON: {
		int ammo = objP->cType.powerupInfo.count;

		bUsed = PickupPrimary ((objP->id == POW_VULCAN_WEAPON) ? VULCAN_INDEX : GAUSS_INDEX, nPlayer);

		//didn't get the weapon (because we already have it), but
		//maybe snag some of the ammo.  if single-tPlayer, grab all the ammo
		//and remove the powerup.  If multi-tPlayer take ammo in excess of
		//the amount in a powerup, and leave the rest.
		if (!bUsed)
			if ((gameData.app.nGameMode & GM_MULTI))
				ammo -= VULCAN_AMMO_AMOUNT;	//don't let take all ammo
		if (ammo > 0) {
			int nAmmoUsed = PickupAmmo (CLASS_PRIMARY, VULCAN_INDEX, ammo, nPlayer);
			objP->cType.powerupInfo.count -= nAmmoUsed;
			if (LOCALPLAYER (nPlayer)) {
				if (!bUsed && nAmmoUsed) {
					PowerupBasic (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
					bSpecialUsed = 1;
					id = POW_VULCAN_AMMO;		//set new id for making sound at end of this function
					if (objP->cType.powerupInfo.count == 0)
						bUsed = 1;		//say bUsed if all ammo taken
					}
				}
			}
		break;
		}

	case	POW_SPREADFIRE_WEAPON:
		bUsed = PickupPrimary (SPREADFIRE_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_PLASMA_WEAPON:
		bUsed = PickupPrimary (PLASMA_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_FUSION_WEAPON:
		bUsed = PickupPrimary (FUSION_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_HELIX_WEAPON:
		bUsed = PickupPrimary (HELIX_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_PHOENIX_WEAPON:
		bUsed = PickupPrimary (PHOENIX_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_OMEGA_WEAPON:
		bUsed = PickupPrimary (OMEGA_INDEX, nPlayer);
		if (bUsed)
			xOmegaCharge = objP->cType.powerupInfo.count;
		else if (!(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case	POW_PROXIMITY_WEAPON:
		bUsed = PickupSecondary (objP, PROXIMITY_INDEX, 4, nPlayer);
		break;

	case	POW_SMARTBOMB_WEAPON:
		bUsed = PickupSecondary (objP, SMART_INDEX, 1, nPlayer);
		break;

	case	POW_MEGA_WEAPON:
		bUsed = PickupSecondary (objP, MEGA_INDEX, 1, nPlayer);
		break;

	case	POW_SMISSILE1_1:
		bUsed = PickupSecondary (objP, SMISSILE1_INDEX, 1, nPlayer);
		break;

	case	POW_SMISSILE1_4:
		bUsed = PickupSecondary (objP, SMISSILE1_INDEX, 4, nPlayer);
		break;

	case	POW_GUIDED_MISSILE_1:
		bUsed = PickupSecondary (objP, GUIDED_INDEX, 1, nPlayer);
		break;

	case	POW_GUIDED_MISSILE_4:
		bUsed = PickupSecondary (objP, GUIDED_INDEX, 4, nPlayer);
		break;

	case	POW_SMART_MINE:
		bUsed = PickupSecondary (objP, SMART_MINE_INDEX, 4, nPlayer);
		break;

	case	POW_MERCURY_MISSILE_1:
		bUsed = PickupSecondary (objP, SMISSILE4_INDEX, 1, nPlayer);
		break;

	case	POW_MERCURY_MISSILE_4:
		bUsed = PickupSecondary (objP, SMISSILE4_INDEX, 4, nPlayer);
		break;

	case	POW_EARTHSHAKER_MISSILE:
		bUsed = PickupSecondary (objP, SMISSILE5_INDEX, 1, nPlayer);
		break;

	case	POW_VULCAN_AMMO:
		bUsed = PickUpVulcanAmmo (nPlayer);
		break;

	case	POW_HOMING_AMMO_1:
		bUsed = PickupSecondary (objP, HOMING_INDEX, 1, nPlayer);
		break;

	case	POW_HOMING_AMMO_4:
		bUsed = PickupSecondary (objP, HOMING_INDEX, 4, nPlayer);
		break;

	case	POW_CLOAK:
		if (gameOpts->gameplay.bInventory && !IsMultiGame) {
			if (playerP->nCloaks == 255) {
				if (LOCALPLAYER (nPlayer))
					HUDInitMessage ("%s", TXT_INVENTORY_FULL);
				}
			else {
				playerP->nCloaks++;
				bUsed = 1;
				}
			}
		else {
			bUsed = -ApplyCloak (1, nPlayer);
			}
		break;

	case	POW_INVULNERABILITY:
		if (gameOpts->gameplay.bInventory && !IsMultiGame) {
			if (playerP->nInvuls == 255) {
				if (LOCALPLAYER (nPlayer))
					HUDInitMessage ("%s", TXT_INVENTORY_FULL);
				}
			else {
				playerP->nInvuls++;
				bUsed = 1;
				}
			}
		else {
			bUsed = -ApplyInvul (1, nPlayer);
			}
		break;

#ifndef RELEASE
	case POW_MEGAWOW:
		DoMegaWowPowerup (50);
		bUsed = 1;
		break;
#endif

	case POW_FULL_MAP:
		bUsed = PickupEquipment (objP, PLAYER_FLAGS_MAP_ALL, TXT_THE_FULLMAP, TXT_GOT_FULLMAP, nPlayer) ? 1 : 0;
		break;

	case POW_CONVERTER:
		sprintf (szTemp, TXT_GOT_CONVERTER, KeyToASCII (GetKeyValue (54)));
		HUDInitMessage (szTemp);
		bUsed = PickupEquipment (objP, PLAYER_FLAGS_CONVERTER, TXT_THE_CONVERTER, szTemp, nPlayer) ? 1 : 0;
		break;

	case POW_SUPER_LASER:
		if (playerP->laserLevel >= MAX_SUPER_LASER_LEVEL) {
			playerP->laserLevel = MAX_SUPER_LASER_LEVEL;
			HUDInitMessage (TXT_LASER_MAXEDOUT);
			} 
		else {
			ubyte nOldLevel = playerP->laserLevel;

			if (playerP->laserLevel <= MAX_LASER_LEVEL)
				playerP->laserLevel = MAX_LASER_LEVEL;
			playerP->laserLevel++;
			if (LOCALPLAYER (nPlayer)) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordLaserLevel (nOldLevel, playerP->laserLevel);
				PowerupBasic (10, 0, 10, LASER_SCORE, TXT_SUPERBOOST, playerP->laserLevel + 1, nPlayer);
				UpdateLaserWeaponInfo ();
				if (gameData.weapons.nPrimary != LASER_INDEX)
				   CheckToUsePrimary (SUPER_LASER_INDEX);
				}
			bUsed = 1;
			}
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_AMMO_RACK:
		bUsed = PickupEquipment (objP, PLAYER_FLAGS_AMMO_RACK, TXT_THE_AMMORACK, TXT_GOT_AMMORACK, nPlayer) ? 1 : 0;
		break;

	case POW_AFTERBURNER:
		bUsed = PickupEquipment (objP, PLAYER_FLAGS_AFTERBURNER, TXT_THE_BURNER, TXT_GOT_BURNER, nPlayer);
		if (bUsed < 0) {
			xAfterburnerCharge = f1_0;
			bUsed = 1;
			}
		break;

	case POW_HEADLIGHT:
		sprintf (szTemp, TXT_GOT_HEADLIGHT, gameOpts->gameplay.bHeadlightOn ? TXT_ON : TXT_OFF);
		HUDInitMessage (szTemp);
		bUsed = PickupEquipment (objP, PLAYER_FLAGS_HEADLIGHT, TXT_THE_HEADLIGHT, szTemp, nPlayer);
		if (bUsed < 0) {
			if (LOCALPLAYER (nPlayer)) {
				if (gameOpts->gameplay.bHeadlightOn && (!EGI_FLAG (bDarkness, 0, 0) || EGI_FLAG (bHeadLights, 0, 0)))
					playerP->flags |= PLAYER_FLAGS_HEADLIGHT_ON;
				if (gameData.app.nGameMode & GM_MULTI)
					MultiSendFlags ((char) gameData.multi.nLocalPlayer);
				}
			bUsed = 1;
			}
		break;

	case POW_FLAG_BLUE:
		bUsed = PickupFlag (objP, TEAM_BLUE, TEAM_RED, "BLUE FLAG!", nPlayer);
		break;

	case POW_FLAG_RED:
		bUsed = PickupFlag (objP, TEAM_RED, TEAM_BLUE, "RED FLAG!", nPlayer);
		break;

	case POW_HOARD_ORB:
		if (gameData.app.nGameMode & GM_HOARD) {	
			if (playerP->secondaryAmmo [PROXIMITY_INDEX] < 12) {
				if (LOCALPLAYER (nPlayer)) {
					MultiSendGotOrb ((char) gameData.multi.nLocalPlayer);
					PowerupBasic (15, 0, 15, 0, "Orb!!!", nPlayer);
					}
				playerP->secondaryAmmo [PROXIMITY_INDEX]++;
				playerP->flags |= PLAYER_FLAGS_FLAG;
				bUsed = 1;
				}
			}
		else if (gameData.app.nGameMode & GM_ENTROPY) {
			if (objP->matCenCreator != GetTeam ((char) gameData.multi.nLocalPlayer) + 1) {
				if ((extraGameInfo [1].entropy.nVirusStability < 2) ||
						 ((extraGameInfo [1].entropy.nVirusStability < 3) && 
						 ((gameData.segs.xSegments [objP->nSegment].owner != objP->matCenCreator) ||
						 (gameData.segs.segment2s [objP->nSegment].special != SEGMENT_IS_ROBOTMAKER))))
					objP->lifeleft = -1;	//make orb disappear if touched by opposing team tPlayer
				}
			else  if (playerP->secondaryAmmo [PROXIMITY_INDEX] < 
						 playerP->secondaryAmmo [SMART_MINE_INDEX]) {
				if (LOCALPLAYER (nPlayer)) {
					MultiSendGotOrb ((char) gameData.multi.nLocalPlayer);
					PowerupBasic (15, 0, 15, 0, "Virus!!!", nPlayer);
					}
				playerP->secondaryAmmo [PROXIMITY_INDEX]++;
				playerP->flags |= PLAYER_FLAGS_FLAG;
				bUsed = 1;
				}
			}
		break;	

	default:
		break;
	}

//always say bUsed, until physics problem (getting stuck on unused powerup)
//is solved.  Note also the break statements above that are commented out
//!!	bUsed = 1;

if (bUsed || bSpecialUsed)
	UsePowerup (id * (bUsed ? bUsed : bSpecialUsed));
gameData.hud.bPlayerMessage = 1;
return bUsed;
}

//------------------------------------------------------------------------------

void SpawnLeftoverPowerups (short nObject)
{
	ubyte		nLeft = gameData.multi.leftoverPowerups [nObject].nCount;
	short		i;
	tObject	*objP;

if (nLeft) {	//leave powerups that cannot be picked up in mine
	ubyte nType = gameData.multi.leftoverPowerups [nObject].nType;
	tObject spitter = *gameData.multi.leftoverPowerups [nObject].spitterP;
	spitter.mType.physInfo.velocity.x =
	spitter.mType.physInfo.velocity.y =
	spitter.mType.physInfo.velocity.z = 0;
	for (; nLeft; nLeft--) {
		i = SpitPowerup (&spitter, nType, d_rand ());
		objP = gameData.objs.objects + i;
		MultiSendCreatePowerup (nType, objP->nSegment, i, &objP->position.vPos);
		}
	gameData.multi.powerupsInMine [nType] += nLeft;
	memset (gameData.multi.leftoverPowerups + nObject, 0, sizeof (gameData.multi.leftoverPowerups [nObject]));
	}
}

//-----------------------------------------------------------------------------

char powerupToWeapon [MAX_POWERUP_TYPES];
char powerupToWeaponCount [MAX_POWERUP_TYPES];
char powerupClass [MAX_POWERUP_TYPES];

void InitPowerupTables (void)
{
memset (powerupToWeapon, 0xff, sizeof (powerupToWeapon));

powerupToWeapon [POW_LASER] = LASER_INDEX;
powerupToWeapon [POW_VULCAN_WEAPON] = VULCAN_INDEX;
powerupToWeapon [POW_SPREADFIRE_WEAPON] = SPREADFIRE_INDEX;
powerupToWeapon [POW_PLASMA_WEAPON] = PLASMA_INDEX;
powerupToWeapon [POW_FUSION_WEAPON] = FUSION_INDEX;
powerupToWeapon [POW_GAUSS_WEAPON] = GAUSS_INDEX;
powerupToWeapon [POW_HELIX_WEAPON] = HELIX_INDEX;
powerupToWeapon [POW_PHOENIX_WEAPON] = PHOENIX_INDEX;
powerupToWeapon [POW_OMEGA_WEAPON] = OMEGA_INDEX;
powerupToWeapon [POW_SUPER_LASER] = SUPER_LASER_INDEX;
powerupToWeapon [POW_MISSILE_1] = CONCUSSION_INDEX;
powerupToWeapon [POW_MISSILE_4] = CONCUSSION_INDEX;
powerupToWeapon [POW_PROXIMITY_WEAPON] = PROXIMITY_INDEX;
powerupToWeapon [POW_SMARTBOMB_WEAPON] = SMART_INDEX;
powerupToWeapon [POW_MEGA_WEAPON] = MEGA_INDEX;
powerupToWeapon [POW_SMISSILE1_1] = SMISSILE1_INDEX;
powerupToWeapon [POW_SMISSILE1_4] = SMISSILE1_INDEX;
powerupToWeapon [POW_GUIDED_MISSILE_1] = GUIDED_INDEX;
powerupToWeapon [POW_GUIDED_MISSILE_4] = GUIDED_INDEX;
powerupToWeapon [POW_SMART_MINE] = SMART_MINE_INDEX;
powerupToWeapon [POW_MERCURY_MISSILE_1] = SMISSILE4_INDEX;
powerupToWeapon [POW_MERCURY_MISSILE_4] = SMISSILE4_INDEX;
powerupToWeapon [POW_EARTHSHAKER_MISSILE] = SMISSILE5_INDEX;
powerupToWeapon [POW_HOMING_AMMO_1] = HOMING_INDEX;
powerupToWeapon [POW_HOMING_AMMO_4] = HOMING_INDEX;

memset (powerupToWeaponCount, 0, sizeof (powerupToWeaponCount));

powerupToWeaponCount [POW_LASER] = 
powerupToWeaponCount [POW_VULCAN_WEAPON] = 
powerupToWeaponCount [POW_SPREADFIRE_WEAPON] = 
powerupToWeaponCount [POW_PLASMA_WEAPON] = 
powerupToWeaponCount [POW_FUSION_WEAPON] = 
powerupToWeaponCount [POW_GAUSS_WEAPON] = 
powerupToWeaponCount [POW_HELIX_WEAPON] = 
powerupToWeaponCount [POW_PHOENIX_WEAPON] = 
powerupToWeaponCount [POW_OMEGA_WEAPON] = 
powerupToWeaponCount [POW_SUPER_LASER] = 1;

powerupToWeaponCount [POW_CLOAK] = 
powerupToWeaponCount [POW_TURBO] = 
powerupToWeaponCount [POW_INVULNERABILITY] = 
powerupToWeaponCount [POW_FULL_MAP] = 
powerupToWeaponCount [POW_CONVERTER] = 
powerupToWeaponCount [POW_AMMO_RACK] = 
powerupToWeaponCount [POW_AFTERBURNER] = 
powerupToWeaponCount [POW_HEADLIGHT] = 1;

powerupToWeaponCount [POW_MISSILE_1] = 
powerupToWeaponCount [POW_SMARTBOMB_WEAPON] = 
powerupToWeaponCount [POW_MEGA_WEAPON] = 
powerupToWeaponCount [POW_SMISSILE1_1] = 
powerupToWeaponCount [POW_GUIDED_MISSILE_1] = 
powerupToWeaponCount [POW_MERCURY_MISSILE_1] = 
powerupToWeaponCount [POW_EARTHSHAKER_MISSILE] = 
powerupToWeaponCount [POW_HOMING_AMMO_1] = 1;

powerupToWeaponCount [POW_MISSILE_4] = 
powerupToWeaponCount [POW_PROXIMITY_WEAPON] = 
powerupToWeaponCount [POW_SMISSILE1_4] = 
powerupToWeaponCount [POW_GUIDED_MISSILE_4] = 
powerupToWeaponCount [POW_SMART_MINE] = 
powerupToWeaponCount [POW_MERCURY_MISSILE_4] = 
powerupToWeaponCount [POW_HOMING_AMMO_4] = 4;

memset (powerupClass, 0, sizeof (powerupClass));
powerupClass [POW_LASER] = 
powerupClass [POW_VULCAN_WEAPON] =
powerupClass [POW_SPREADFIRE_WEAPON] =
powerupClass [POW_PLASMA_WEAPON] =
powerupClass [POW_FUSION_WEAPON] =
powerupClass [POW_GAUSS_WEAPON] =
powerupClass [POW_HELIX_WEAPON] =
powerupClass [POW_PHOENIX_WEAPON] =
powerupClass [POW_OMEGA_WEAPON] =
powerupClass [POW_SUPER_LASER] = 1;

powerupClass [POW_MISSILE_1] = 
powerupClass [POW_MISSILE_4] = 
powerupClass [POW_PROXIMITY_WEAPON] =
powerupClass [POW_SMARTBOMB_WEAPON] =
powerupClass [POW_MEGA_WEAPON] =
powerupClass [POW_SMISSILE1_1] = 
powerupClass [POW_SMISSILE1_4] =
powerupClass [POW_GUIDED_MISSILE_1] =
powerupClass [POW_GUIDED_MISSILE_4] = 
powerupClass [POW_SMART_MINE] = 
powerupClass [POW_MERCURY_MISSILE_1] = 
powerupClass [POW_MERCURY_MISSILE_4] =
powerupClass [POW_EARTHSHAKER_MISSILE] =
powerupClass [POW_HOMING_AMMO_1] = 
powerupClass [POW_HOMING_AMMO_4] = 2;

powerupClass [POW_QUAD_FIRE] = 
powerupClass [POW_CLOAK] = 
powerupClass [POW_TURBO] = 
powerupClass [POW_INVULNERABILITY] = 
powerupClass [POW_FULL_MAP] = 
powerupClass [POW_CONVERTER] = 
powerupClass [POW_AMMO_RACK] = 
powerupClass [POW_AFTERBURNER] = 
powerupClass [POW_HEADLIGHT] = 3;
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

short PowerupsOnShips (int nPowerup)
{
	int	nType;
	short h, i, nWeapon = PowerupToWeapon (nPowerup, &nType);
	tPlayer	*playerP = gameData.multi.players;

if (nWeapon < 0)
	return 0;
for (h = i = 0; i < gameData.multi.nPlayers; i++, playerP++) {
	if ((i == gameData.multi.nLocalPlayer) && 
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
#ifndef FAST_FILE_IO
/*
 * reads n powerupType_info structs from a CFILE
 */
extern int PowerupTypeInfoReadN (powerupType_info *pti, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		pti[i].nClipIndex = CFReadInt (fp);
		pti[i].hitSound = CFReadInt (fp);
		pti[i].size = CFReadFix (fp);
		pti[i].light = CFReadFix (fp);
	}
	return i;
}
#endif

//------------------------------------------------------------------------------
//eof
