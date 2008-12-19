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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "error.h"
#include "text.h"
#include "network.h"
#include "newdemo.h"
#include "gauges.h"
#include "scores.h"

//	-----------------------------------------------------------------------------

//called when a primary weapon is picked up
//returns true if actually picked up
int PickupPrimary (int nWeaponIndex, int nPlayer)
{
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;
	//ushort oldFlags = LOCALPLAYER.primaryWeaponFlags;
	ushort flag = 1 << nWeaponIndex;
	int nCutPoint;
	int nSupposedWeapon = gameData.weapons.nPrimary;
	int bTripleFusion = !gameData.multiplayer.weaponStates [nPlayer].bTripleFusion && (nWeaponIndex == FUSION_INDEX) && EGI_FLAG (bTripleFusion, 0, 0, 0);

if ((nWeaponIndex != LASER_INDEX) && (playerP->primaryWeaponFlags & flag) && !bTripleFusion) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES (nWeaponIndex));
	return 0;
	}
if (!(playerP->primaryWeaponFlags & flag))
	playerP->primaryWeaponFlags |= flag;
else if (bTripleFusion) {
	if (nPlayer == gameData.multiplayer.nLocalPlayer)
   	gameData.weapons.bTripleFusion = 1;
   else
	   gameData.multiplayer.weaponStates [nPlayer].bTripleFusion = 1;
	}
if (ISLOCALPLAYER (nPlayer)) {
	nCutPoint = POrderList (255);
	if ((gameData.weapons.nPrimary == LASER_INDEX) && 
		(playerP->laserLevel >= 4))
		nSupposedWeapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((gameOpts->gameplay.nAutoSelectWeapon == 2) && 
		 (POrderList (nWeaponIndex) < nCutPoint) && 
		 (POrderList (nWeaponIndex) < POrderList (nSupposedWeapon)))
		SelectWeapon (nWeaponIndex, 0, 0, 1);
	paletteManager.BumpEffect (7,14,21);
	if (nWeaponIndex != LASER_INDEX)
  		HUDInitMessage ("%s!", PRIMARY_WEAPON_NAMES (nWeaponIndex));
	}
return 1;
}

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int PickupSecondary (CObject *objP, int nWeaponIndex, int nAmount, int nPlayer)
{
	int		nMaxAmount;
	int		nPickedUp;
	int		nCutPoint, bEmpty = 0, bSmokeGrens;
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

if ((nWeaponIndex == PROXMINE_INDEX) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0)) {
	bSmokeGrens = 1;
	nMaxAmount = 4;
	}
else {
	bSmokeGrens = 0;
	nMaxAmount = nMaxSecondaryAmmo [nWeaponIndex];
	if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
		nMaxAmount *= 2;
	}
if (playerP->secondaryAmmo [nWeaponIndex] >= nMaxAmount) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage("%s %i %ss!", 
			TXT_ALREADY_HAVE, 
			playerP->secondaryAmmo [nWeaponIndex],
			bSmokeGrens ? TXT_SMOKE_GRENADE : SECONDARY_WEAPON_NAMES (nWeaponIndex));
	return 0;
	}
playerP->secondaryWeaponFlags |= (1 << nWeaponIndex);
playerP->secondaryAmmo [nWeaponIndex] += nAmount;
nPickedUp = nAmount;
if (playerP->secondaryAmmo [nWeaponIndex] > nMaxAmount) {
	nPickedUp = nAmount - (playerP->secondaryAmmo [nWeaponIndex] - nMaxAmount);
	playerP->secondaryAmmo [nWeaponIndex] = nMaxAmount;
	if ((nPickedUp < nAmount) && (nWeaponIndex != PROXMINE_INDEX) && (nWeaponIndex != SMARTMINE_INDEX)) {
		short nObject = objP->Index ();
		gameData.multiplayer.leftoverPowerups [nObject].nCount = nAmount - nPickedUp;
		gameData.multiplayer.leftoverPowerups [nObject].nType = secondaryWeaponToPowerup [nWeaponIndex];
		gameData.multiplayer.leftoverPowerups [nObject].spitterP = OBJECTS + playerP->nObject;
		}
	}
if (ISLOCALPLAYER (nPlayer)) {
	nCutPoint = SOrderList (255);
	bEmpty = playerP->secondaryAmmo [gameData.weapons.nSecondary] == 0;
	if (gameOpts->gameplay.nAutoSelectWeapon) {
		if (gameOpts->gameplay.nAutoSelectWeapon == 1) {
			if (bEmpty)
				SelectWeapon (nWeaponIndex, 1, 0, 1);
			}
		else if ((SOrderList (nWeaponIndex) < nCutPoint) && 
					(bEmpty || (SOrderList (nWeaponIndex) < SOrderList (gameData.weapons.nSecondary))))
			SelectWeapon (nWeaponIndex,1, 0, 1);
		else {
			//if we don't auto-select this weapon, but it's a proxbomb or smart mine,
			//we want to do a mini-auto-selection that applies to the drop bomb key
			if ((nWeaponIndex == PROXMINE_INDEX || nWeaponIndex == SMARTMINE_INDEX) &&
				!(gameData.weapons.nSecondary == PROXMINE_INDEX || gameData.weapons.nSecondary == SMARTMINE_INDEX)) {
				int cur = bLastSecondaryWasSuper [PROXMINE_INDEX] ? PROXMINE_INDEX : SMARTMINE_INDEX;
				if (SOrderList (nWeaponIndex) < SOrderList (cur))
					bLastSecondaryWasSuper[PROXMINE_INDEX] = (nWeaponIndex == SMARTMINE_INDEX);
				}
			}
		}
	//note: flash for all but concussion was 7,14,21
	if (nAmount>1) {
		paletteManager.BumpEffect (15,15,15);
		HUDInitMessage("%d %s%s", nPickedUp, bSmokeGrens ? TXT_SMOKE_GRENADES : SECONDARY_WEAPON_NAMES (nWeaponIndex), TXT_SX);
		}
	else {
		paletteManager.BumpEffect (10,10,10);
		HUDInitMessage("%s!", bSmokeGrens ? TXT_SMOKE_GRENADE : SECONDARY_WEAPON_NAMES (nWeaponIndex));
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------

//called when ammo (for the vulcan cannon) is picked up
//	Returns the amount picked up
int PickupAmmo (int classFlag, int nWeaponIndex, int ammoCount, const char *pszMsg, int nPlayer)
{
	int		nMaxAmmo, nCutPoint, nSupposedWeapon = gameData.weapons.nPrimary;
	int		nOldAmmo = classFlag;		//kill warning
	CPlayerData	*playerP = gameData.multiplayer.players + nPlayer;

Assert(classFlag==CLASS_PRIMARY && nWeaponIndex==VULCAN_INDEX);

nMaxAmmo = nMaxPrimaryAmmo [nWeaponIndex];
if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
	nMaxAmmo *= 2;
if (playerP->primaryAmmo [nWeaponIndex] == nMaxAmmo)
	return 0;
nOldAmmo = playerP->primaryAmmo [nWeaponIndex];
playerP->primaryAmmo [nWeaponIndex] += ammoCount;
if (playerP->primaryAmmo [nWeaponIndex] > nMaxAmmo) {
	ammoCount += (nMaxAmmo - playerP->primaryAmmo [nWeaponIndex]);
	playerP->primaryAmmo [nWeaponIndex] = nMaxAmmo;
	}
if ((nPlayer = gameData.multiplayer.nLocalPlayer)) {
	nCutPoint = POrderList (255);
	if ((gameData.weapons.nPrimary == LASER_INDEX) && (playerP->laserLevel >= 4))
		nSupposedWeapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((playerP->primaryWeaponFlags & (1<<nWeaponIndex)) && 
		(nWeaponIndex > gameData.weapons.nPrimary) && 
		(nOldAmmo == 0) &&
		(POrderList (nWeaponIndex) < nCutPoint) && 
		(POrderList (nWeaponIndex) < POrderList (nSupposedWeapon)))
		SelectWeapon (nWeaponIndex,0,0,1);
	}
return ammoCount;	//return amount used
}

//------------------------------------------------------------------------------

int PickupVulcanAmmo (CObject *objP, int nPlayer)
{
	int		bUsed = 0;

int	pwSave = gameData.weapons.nPrimary;	
// Ugh, save selected primary weapon around the picking up of the ammo.  
// I apologize for this code.  Matthew A. Toschlog
if (PickupAmmo (CLASS_PRIMARY, VULCAN_INDEX, VULCAN_AMMO_AMOUNT, NULL, nPlayer)) {
	if (ISLOCALPLAYER (nPlayer))
		PowerupBasic (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO, nPlayer);
	bUsed = 1;
	} 
else {
	int nMaxAmmo = nMaxPrimaryAmmo [VULCAN_INDEX];
	if (LOCALPLAYER.flags & PLAYER_FLAGS_AMMO_RACK)
		nMaxAmmo *= 2;
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %d %s!", TXT_ALREADY_HAVE,X2I ((unsigned) VULCAN_AMMO_SCALE * (unsigned) nMaxAmmo), TXT_VULCAN_ROUNDS);
	bUsed = 0;
	}
gameData.weapons.nPrimary = pwSave;
return bUsed;
}

//------------------------------------------------------------------------------

int PickupLaser (CObject *objP, int nId, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (playerP->laserLevel < MAX_LASER_LEVEL) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordLaserLevel ((sbyte) playerP->laserLevel, (sbyte) playerP->laserLevel + 1);
	playerP->laserLevel++;
	PowerupBasic (10, 0, 10, LASER_SCORE, "%s %s %d", TXT_LASER, TXT_BOOSTED_TO, playerP->laserLevel+1);
	UpdateLaserWeaponInfo ();
	PickupPrimary (LASER_INDEX, nPlayer);
	return 1;
	}
if (nPlayer == gameData.multiplayer.nLocalPlayer)
	HUDInitMessage (TXT_MAXED_OUT, TXT_LASER);
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (objP, nPlayer);
}

//------------------------------------------------------------------------------

int PickupSuperLaser (CObject *objP, int nId, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (playerP->laserLevel < MAX_SUPER_LASER_LEVEL) {
	ubyte nOldLevel = playerP->laserLevel;

	if (playerP->laserLevel <= MAX_LASER_LEVEL)
		playerP->laserLevel = MAX_LASER_LEVEL;
	playerP->laserLevel++;
	bLastPrimaryWasSuper [LASER_INDEX] = 1;
	if (ISLOCALPLAYER (nPlayer)) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordLaserLevel (nOldLevel, playerP->laserLevel);
		PowerupBasic (10, 0, 10, LASER_SCORE, TXT_SUPERBOOST, playerP->laserLevel + 1, nPlayer);
		UpdateLaserWeaponInfo ();
		if (gameData.weapons.nPrimary != LASER_INDEX)
		   CheckToUsePrimary (SUPER_LASER_INDEX);
		}
	return 1;
	}
playerP->laserLevel = MAX_SUPER_LASER_LEVEL;
if (nPlayer == gameData.multiplayer.nLocalPlayer)
	HUDInitMessage (TXT_LASER_MAXEDOUT);
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (objP, nPlayer);
}

//------------------------------------------------------------------------------

int PickupQuadLaser (CObject *objP, int nId, int nPlayer)
{
	CPlayerData *playerP = gameData.multiplayer.players + nPlayer;

if (!(playerP->flags & PLAYER_FLAGS_QUAD_LASERS)) {
	playerP->flags |= PLAYER_FLAGS_QUAD_LASERS;
	PowerupBasic (15, 15, 7, QUAD_FIRE_SCORE, "%s!", TXT_QUAD_LASERS);
	UpdateLaserWeaponInfo ();
	return 1;
	}
if (nPlayer == gameData.multiplayer.nLocalPlayer)
	HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, TXT_QUAD_LASERS);
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (objP, nPlayer);
}

//------------------------------------------------------------------------------

int PickupGun (CObject *objP, int nId, int nPlayer)
{
if (PickupPrimary (nId, nPlayer)) {
	if ((nId == OMEGA_INDEX) && (nPlayer == gameData.multiplayer.nLocalPlayer))
		gameData.omega.xCharge [IsMultiGame] = objP->cType.powerupInfo.nCount;
	return 1;
	}
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (NULL, nPlayer);
}

//	-----------------------------------------------------------------------------

int PickupGatlingGun (CObject *objP, int nId, int nPlayer)
{
	int nAmmo = objP->cType.powerupInfo.nCount;
	int bUsed = PickupPrimary (nId, nPlayer);

//didn't get the weapon (because we already have it), but
//maybe snag some of the nAmmo.  if single-CPlayerData, grab all the nAmmo
//and remove the powerup.  If multi-CPlayerData take nAmmo in excess of
//the amount in a powerup, and leave the rest.
if (!bUsed)
	if (IsMultiGame)
		nAmmo -= VULCAN_AMMO_AMOUNT;	//don't let take all nAmmo
if (nAmmo > 0) {
	int nAmmoUsed = PickupAmmo (CLASS_PRIMARY, VULCAN_INDEX, nAmmo, NULL, nPlayer);
	objP->cType.powerupInfo.nCount -= nAmmoUsed;
	if (ISLOCALPLAYER (nPlayer)) {
		if (!bUsed && nAmmoUsed) {
			PowerupBasic (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
			nId = POW_VULCAN_AMMO;		//set new id for making sound at end of this function
			return objP->cType.powerupInfo.nCount ? -1 : -2;
			}
		}
	}
return bUsed;
}

//	-----------------------------------------------------------------------------
//eof
