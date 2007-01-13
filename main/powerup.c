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

	int			h, nFrames = vcP->nFrameCount;
	fix			xTime, xFudge = (xPowerupTime * (nObject & 3)) >> 4;
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
xTime = vciP->xFrameTime - xPowerupTime + xFudge;
if (xTime < 0) {
	h = (-xTime + vcP->xFrameTime - 1) / vcP->xFrameTime;
	xTime += h * vcP->xFrameTime;
	h %= nFrames;
	if (nObject & 1) {
		vciP->nCurFrame -= h;
		if (0 > vciP->nCurFrame)
			vciP->nCurFrame = nFrames - 1;
		}
	else {
		vciP->nCurFrame += h;
		if (vciP->nCurFrame >= nFrames)
			vciP->nCurFrame = 0;
		}
	}
vciP->xFrameTime = xTime;
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
	ObjectCreateExplosion (objP->position.nSegment, &objP->position.vPos, F1_0 * 7 / 2, VCLIP_POWERUP_DISAPPEARANCE);
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
	if (ISLOCALPLAYER (nPlayer))
		PowerupBasic (15,15,7, ENERGY_SCORE, "%s %s %d",
						 TXT_ENERGY, TXT_BOOSTED_TO, f2ir (playerP->energy));
	return 1;
	} 
else if (ISLOCALPLAYER (nPlayer))
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
	if (ISLOCALPLAYER (nPlayer)) {
		PowerupBasic (0, 0, 15, SHIELD_SCORE, "%s %s %d",
							TXT_SHIELD, TXT_BOOSTED_TO, f2ir (playerP->shields));
		MultiSendShields ();
		}
	return 1;
	}
else if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
return 0;
}

//------------------------------------------------------------------------------

int PickupKey (tObject *objP, int nKey, char *pszKey, int nPlayer)
{
if (ISLOCALPLAYER (nPlayer)) {
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
if (ISLOCALPLAYER (nPlayer)) {
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
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, pszHave);
	if (!(gameData.app.nGameMode & GM_MULTI))
		bUsed = PickupEnergy (nPlayer);
	} 
else {
	playerP->flags |= nEquipment;
	if (ISLOCALPLAYER (nPlayer)) {
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
	if (ISLOCALPLAYER (nPlayer))
		PowerupBasic (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO, nPlayer);
	bUsed = 1;
	} 
else {
	max = nMaxPrimaryAmmo [VULCAN_INDEX];
	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;
	if (ISLOCALPLAYER (nPlayer))
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
	if (!bApply && (gameOpts->gameplay.bInventory && !IsMultiGame) && ((id == POW_CLOAK) || (id == POW_INVUL)))
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
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_INVULNERABLE);
	return 0;
	}
if (gameOpts->gameplay.bInventory && !IsMultiGame)
	playerP->nInvuls--;
if (ISLOCALPLAYER (nPlayer)) {
	playerP->invulnerableTime = gameData.time.xGame;
	playerP->flags |= PLAYER_FLAGS_INVULNERABLE;
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendInvul ();
	PowerupBasic (7, 14, 21, INVULNERABILITY_SCORE, "%s!", TXT_INVULNERABILITY);
	SetSpherePulse (gameData.multi.spherePulse + gameData.multi.nLocalPlayer, 0.02f, 0.5f);
	UsePowerup (-POW_INVUL);
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
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_CLOAKED);
	return 0;
	}
if (gameOpts->gameplay.bInventory && !IsMultiGame)
	playerP->nCloaks--;
if (ISLOCALPLAYER (nPlayer)) {
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

	case POW_CONCUSSION_1:
		bUsed = PickupSecondary (objP, CONCUSSION_INDEX, 1, nPlayer);
		break;

	case POW_CONCUSSION_4:
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

	case POW_QUADLASER:
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

	case POW_VULCAN:
	case POW_GAUSS: {
		int ammo = objP->cType.powerupInfo.count;

		bUsed = PickupPrimary ((objP->id == POW_VULCAN) ? VULCAN_INDEX : GAUSS_INDEX, nPlayer);

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
			if (ISLOCALPLAYER (nPlayer)) {
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

	case POW_SPREADFIRE:
		bUsed = PickupPrimary (SPREADFIRE_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_PLASMA:
		bUsed = PickupPrimary (PLASMA_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_FUSION:
		bUsed = PickupPrimary (FUSION_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_HELIX:
		bUsed = PickupPrimary (HELIX_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_PHOENIX:
		bUsed = PickupPrimary (PHOENIX_INDEX, nPlayer);
		if (!bUsed && !(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_OMEGA:
		bUsed = PickupPrimary (OMEGA_INDEX, nPlayer);
		if (bUsed)
			xOmegaCharge = objP->cType.powerupInfo.count;
		else if (!(gameData.app.nGameMode & GM_MULTI))
			bUsed = PickupEnergy (nPlayer);
		break;

	case POW_PROXMINE:
		bUsed = PickupSecondary (objP, PROXIMITY_INDEX, 4, nPlayer);
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
		bUsed = PickupSecondary (objP, SMART_MINE_INDEX, 4, nPlayer);
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

	case POW_VULCAN_AMMO:
		bUsed = PickUpVulcanAmmo (nPlayer);
		break;

	case POW_HOMINGMSL_1:
		bUsed = PickupSecondary (objP, HOMING_INDEX, 1, nPlayer);
		break;

	case POW_HOMINGMSL_4:
		bUsed = PickupSecondary (objP, HOMING_INDEX, 4, nPlayer);
		break;

	case POW_CLOAK:
		if (gameOpts->gameplay.bInventory && !IsMultiGame) {
			if (playerP->nCloaks == 255) {
				if (ISLOCALPLAYER (nPlayer))
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

	case POW_INVUL:
		if (gameOpts->gameplay.bInventory && !IsMultiGame) {
			if (playerP->nInvuls == 255) {
				if (ISLOCALPLAYER (nPlayer))
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

	case POW_SUPERLASER:
		if (playerP->laserLevel >= MAX_SUPER_LASER_LEVEL) {
			playerP->laserLevel = MAX_SUPER_LASER_LEVEL;
			HUDInitMessage (TXT_LASER_MAXEDOUT);
			} 
		else {
			ubyte nOldLevel = playerP->laserLevel;

			if (playerP->laserLevel <= MAX_LASER_LEVEL)
				playerP->laserLevel = MAX_LASER_LEVEL;
			playerP->laserLevel++;
			if (ISLOCALPLAYER (nPlayer)) {
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

	case POW_AMMORACK:
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
			if (ISLOCALPLAYER (nPlayer)) {
				if (gameOpts->gameplay.bHeadlightOn && (!EGI_FLAG (bDarkness, 0, 0) || EGI_FLAG (bHeadLights, 0, 0)))
					playerP->flags |= PLAYER_FLAGS_HEADLIGHT_ON;
				if (gameData.app.nGameMode & GM_MULTI)
					MultiSendFlags ((char) gameData.multi.nLocalPlayer);
				}
			bUsed = 1;
			}
		break;

	case POW_BLUEFLAG:
		bUsed = PickupFlag (objP, TEAM_BLUE, TEAM_RED, "BLUE FLAG!", nPlayer);
		break;

	case POW_REDFLAG:
		bUsed = PickupFlag (objP, TEAM_RED, TEAM_BLUE, "RED FLAG!", nPlayer);
		break;

	case POW_HOARD_ORB:
		if (gameData.app.nGameMode & GM_HOARD) {	
			if (playerP->secondaryAmmo [PROXIMITY_INDEX] < 12) {
				if (ISLOCALPLAYER (nPlayer)) {
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
					 ((gameData.segs.xSegments [objP->position.nSegment].owner != objP->matCenCreator) ||
					 (gameData.segs.segment2s [objP->position.nSegment].special != SEGMENT_IS_ROBOTMAKER))))
					objP->lifeleft = -1;	//make orb disappear if touched by opposing team tPlayer
				}
			else if (!extraGameInfo [1].entropy.nMaxVirusCapacity ||
						(playerP->secondaryAmmo [PROXIMITY_INDEX] < playerP->secondaryAmmo [SMART_MINE_INDEX])) {
				if (ISLOCALPLAYER (nPlayer)) {
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

if (bUsed || bSpecialUsed) {
	UsePowerup (id * (bUsed ? bUsed : bSpecialUsed));
	if (IsMultiGame)
		MultiSendWeapons (1);
	}
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
	spitter.mType.physInfo.velocity.p.x =
	spitter.mType.physInfo.velocity.p.y =
	spitter.mType.physInfo.velocity.p.z = 0;
	for (; nLeft; nLeft--) {
		i = SpitPowerup (&spitter, nType, d_rand ());
		objP = gameData.objs.objects + i;
		MultiSendCreatePowerup (nType, objP->position.nSegment, i, &objP->position.vPos);
		}
	gameData.multi.powerupsInMine [nType] += nLeft;
	memset (gameData.multi.leftoverPowerups + nObject, 0, sizeof (gameData.multi.leftoverPowerups [nObject]));
	}
}

//-----------------------------------------------------------------------------

char powerupToWeapon [MAX_POWERUP_TYPES];
char powerupToWeaponCount [MAX_POWERUP_TYPES];
char powerupClass [MAX_POWERUP_TYPES];
char powerupToObject [MAX_POWERUP_TYPES];
short powerupToModel [MAX_POWERUP_TYPES];
short weaponToModel [MAX_WEAPON_TYPES];

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
powerupToWeapon [POW_CONCUSSION_1] = CONCUSSION_INDEX;
powerupToWeapon [POW_CONCUSSION_4] = CONCUSSION_INDEX;
powerupToWeapon [POW_PROXMINE] = PROXIMITY_INDEX;
powerupToWeapon [POW_SMARTMSL] = SMART_INDEX;
powerupToWeapon [POW_MEGAMSL] = MEGA_INDEX;
powerupToWeapon [POW_FLASHMSL_1] = FLASHMSL_INDEX;
powerupToWeapon [POW_FLASHMSL_4] = FLASHMSL_INDEX;
powerupToWeapon [POW_GUIDEDMSL_1] = GUIDED_INDEX;
powerupToWeapon [POW_GUIDEDMSL_4] = GUIDED_INDEX;
powerupToWeapon [POW_SMARTMINE] = SMART_MINE_INDEX;
powerupToWeapon [POW_MERCURYMSL_1] = MERCURY_INDEX;
powerupToWeapon [POW_MERCURYMSL_4] = MERCURY_INDEX;
powerupToWeapon [POW_EARTHSHAKER] = EARTHSHAKER_INDEX;
powerupToWeapon [POW_HOMINGMSL_1] = HOMING_INDEX;
powerupToWeapon [POW_HOMINGMSL_4] = HOMING_INDEX;

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
powerupToModel [POW_FUSION] = MAX_POLYGON_MODELS - 13;
powerupToModel [POW_SUPERLASER] = MAX_POLYGON_MODELS - 14;
powerupToModel [POW_GAUSS] = MAX_POLYGON_MODELS - 15;
powerupToModel [POW_HELIX] = MAX_POLYGON_MODELS - 16;
powerupToModel [POW_PHOENIX] = MAX_POLYGON_MODELS - 17;
powerupToModel [POW_OMEGA] = MAX_POLYGON_MODELS - 18;
powerupToModel [POW_QUADLASER] = MAX_POLYGON_MODELS - 19;
powerupToModel [POW_AFTERBURNER] = MAX_POLYGON_MODELS - 20;
powerupToModel [POW_HEADLIGHT] = MAX_POLYGON_MODELS - 21;
powerupToModel [POW_AMMORACK] = MAX_POLYGON_MODELS - 22;
powerupToModel [POW_CONVERTER] = MAX_POLYGON_MODELS - 23;
powerupToModel [POW_FULL_MAP] = MAX_POLYGON_MODELS - 24;
powerupToModel [POW_CLOAK] = MAX_POLYGON_MODELS - 25;
powerupToModel [POW_INVUL] = MAX_POLYGON_MODELS - 26;
powerupToModel [POW_EXTRA_LIFE] = MAX_POLYGON_MODELS - 27;
powerupToModel [POW_KEY_BLUE] = MAX_POLYGON_MODELS - 28;
powerupToModel [POW_KEY_RED] = MAX_POLYGON_MODELS - 29;
powerupToModel [POW_KEY_GOLD] = MAX_POLYGON_MODELS - 30;

memset (weaponToModel, 0, sizeof (weaponToModel));
weaponToModel [PROXMINE_ID] = MAX_POLYGON_MODELS - 2;
weaponToModel [SMARTMINE_ID] = MAX_POLYGON_MODELS - 4;
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
