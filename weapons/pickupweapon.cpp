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
COPYRIGHT 1993	999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "descent.h"
#include "error.h"
#include "text.h"
#include "network.h"
#include "newdemo.h"
#include "cockpit.h"
#include "scores.h"

//	-----------------------------------------------------------------------------

static void DuplicateWeaponMsg (int32_t nWeaponIndex, int32_t nPlayer)
{
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES (nWeaponIndex));
}

//	-----------------------------------------------------------------------------

//called when a primary weapon is picked up
//returns true if actually picked up
int32_t PickupPrimary (int32_t nWeaponIndex, int32_t nPlayer)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;
	//uint16_t oldFlags = LOCALPLAYER.primaryWeaponFlags;
	uint16_t flag = 1 << nWeaponIndex;
	int32_t nCutPoint;
	int32_t nSupposedWeapon = gameData.weaponData.nPrimary;

if (nWeaponIndex == FUSION_INDEX) {
	if (gameData.multiplayer.weaponStates [nPlayer].nShip == 1)
		return 0;
	if (pPlayer->primaryWeaponFlags & flag) {
		if (!EGI_FLAG (bTripleFusion, 0, 0, 0)) {
			HUDInitMessage (TXT_MAXED_OUT, TXT_FUSION);
			return 0; // tri-fusion not allowed
			}
		if (gameData.multiplayer.weaponStates [nPlayer].nShip != 2) {
			HUDInitMessage (TXT_MAXED_OUT, TXT_FUSION);
			return 0; // tri-fusion only allowed on heavy fighter
			}
		if (gameData.multiplayer.weaponStates [nPlayer].bTripleFusion) {
			DuplicateWeaponMsg (nWeaponIndex, nPlayer);
			return 0; // already has tri-fusion
			}
		if (nPlayer == N_LOCALPLAYER)
   		gameData.weaponData.bTripleFusion = 1;
		else
			gameData.multiplayer.weaponStates [nPlayer].bTripleFusion = 1;
		}
	}
else if (nWeaponIndex != LASER_INDEX) {
	if (pPlayer->primaryWeaponFlags & flag) {
		DuplicateWeaponMsg (nWeaponIndex, nPlayer);
		return 0;
		}
	}
pPlayer->primaryWeaponFlags |= flag;

if (ISLOCALPLAYER (nPlayer)) {
	nCutPoint = POrderList (255);
	if ((gameData.weaponData.nPrimary == LASER_INDEX) && pPlayer->HasSuperLaser ())
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

int32_t MaxSecondaryAmmo (int32_t nWeapon)
{
int32_t nMaxAmount = nMaxSecondaryAmmo [nWeapon];
if (gameData.multiplayer.weaponStates [N_LOCALPLAYER].nShip == 1) {
	if (nWeapon == EARTHSHAKER_INDEX)
		nMaxAmount /= 2;
	else
		nMaxAmount = 4 * nMaxAmount / 5;
	}
else if (gameData.multiplayer.weaponStates [N_LOCALPLAYER].nShip == 2) 
	nMaxAmount *= 2;
else {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_AMMO_RACK) {
		if (gameStates.app.bNostalgia)
			nMaxAmount *= 2;
		else
			nMaxAmount = 3 * nMaxAmount / 2;
		}
	}
if (IsMultiGame && !IsCoopGame && gameStates.app.bHaveExtraGameInfo [1] && (nMaxAmount > extraGameInfo [1].loadout.nMissiles [nWeapon]))
	nMaxAmount = extraGameInfo [1].loadout.nMissiles [nWeapon];
return nMaxAmount;
}

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int32_t PickupSecondary (CObject *pObj, int32_t nWeaponIndex, int32_t nAmount, int32_t nPlayer)
{
	int32_t		nMaxAmount;
	int32_t		nPickedUp;
	int32_t		nCutPoint, bEmpty = 0, bSmokeGrens;
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;

if ((nWeaponIndex == PROXMINE_INDEX) && IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0)) {
	bSmokeGrens = 1;
	nMaxAmount = 4;
	}
else {
	bSmokeGrens = 0;
	nMaxAmount = MaxSecondaryAmmo (nWeaponIndex);
	}
if (pPlayer->secondaryAmmo [nWeaponIndex] >= nMaxAmount) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage("%s %i %ss!", 
			TXT_ALREADY_HAVE, 
			pPlayer->secondaryAmmo [nWeaponIndex],
			bSmokeGrens ? TXT_SMOKE_GRENADE : SECONDARY_WEAPON_NAMES (nWeaponIndex));
	return 0;
	}
pPlayer->secondaryWeaponFlags |= (1 << nWeaponIndex);
pPlayer->secondaryAmmo [nWeaponIndex] += nAmount;
nPickedUp = nAmount;
if (pPlayer->secondaryAmmo [nWeaponIndex] > nMaxAmount) {
	nPickedUp = nAmount - (pPlayer->secondaryAmmo [nWeaponIndex] - nMaxAmount);
	pPlayer->secondaryAmmo [nWeaponIndex] = nMaxAmount;
	if ((nPickedUp < nAmount) && (nWeaponIndex != PROXMINE_INDEX) && (nWeaponIndex != SMARTMINE_INDEX)) {
		int16_t nObject = pObj->Index ();
		gameData.multiplayer.leftoverPowerups [nObject].nCount = nAmount - nPickedUp;
		gameData.multiplayer.leftoverPowerups [nObject].nType = secondaryWeaponToPowerup [0][nWeaponIndex];
		gameData.multiplayer.leftoverPowerups [nObject].pSpitter = OBJECT (pPlayer->nObject);
		}
	}
if (ISLOCALPLAYER (nPlayer)) {
	nCutPoint = SOrderList (255);
	bEmpty = pPlayer->secondaryAmmo [gameData.weaponData.nSecondary] == 0;
	if (gameOpts->gameplay.nAutoSelectWeapon) {
		if (gameOpts->gameplay.nAutoSelectWeapon == 1) {
			if (bEmpty)
				SelectWeapon (nWeaponIndex, 1, 0, 1);
			}
		else if ((SOrderList (nWeaponIndex) < nCutPoint) && 
					(bEmpty || (SOrderList (nWeaponIndex) < SOrderList (gameData.weaponData.nSecondary))))
			SelectWeapon (nWeaponIndex,1, 0, 1);
		else {
			//if we don't auto-select this weapon, but it's a proxbomb or smart mine,
			//we want to do a mini-auto-selection that applies to the drop bomb key
			if ((nWeaponIndex == PROXMINE_INDEX || nWeaponIndex == SMARTMINE_INDEX) &&
				!(gameData.weaponData.nSecondary == PROXMINE_INDEX || gameData.weaponData.nSecondary == SMARTMINE_INDEX)) {
				int32_t cur = bLastSecondaryWasSuper [PROXMINE_INDEX] ? PROXMINE_INDEX : SMARTMINE_INDEX;
				if (SOrderList (nWeaponIndex) < SOrderList (cur))
					bLastSecondaryWasSuper [PROXMINE_INDEX] = (nWeaponIndex == SMARTMINE_INDEX);
				}
			}
		}
	//note: flash for all but concussion was 7,14,21
	if (nAmount > 1) {
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
int32_t PickupAmmo (int32_t classFlag, int32_t nWeaponIndex, int32_t ammoCount, const char *pszMsg, int32_t nPlayer)
{
	int32_t			nMaxAmmo, nCutPoint, nSupposedWeapon = gameData.weaponData.nPrimary;
	int32_t			nOldAmmo = classFlag;		//kill warning
	CPlayerData*	pPlayer = gameData.multiplayer.players + nPlayer;

nMaxAmmo = nMaxPrimaryAmmo [nWeaponIndex];
if (pPlayer->flags & PLAYER_FLAGS_AMMO_RACK)
	nMaxAmmo *= 2;
nMaxAmmo -= pPlayer->primaryAmmo [nWeaponIndex];
if (ammoCount > nMaxAmmo) {
	if (!nMaxAmmo/* || (nWeaponIndex == VULCAN_INDEX)*/)	// only pick up Vulcan ammo if player can take the entire clip
		return 0;
	ammoCount = nMaxAmmo;
	}
nOldAmmo = pPlayer->primaryAmmo [nWeaponIndex];
pPlayer->primaryAmmo [nWeaponIndex] += ammoCount;
if ((nPlayer = N_LOCALPLAYER)) {
	nCutPoint = POrderList (255);
	if ((gameData.weaponData.nPrimary == LASER_INDEX) && pPlayer->HasSuperLaser ())
		nSupposedWeapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((pPlayer->primaryWeaponFlags & (1<<nWeaponIndex)) && 
		(nWeaponIndex > gameData.weaponData.nPrimary) && 
		(nOldAmmo == 0) &&
		(POrderList (nWeaponIndex) < nCutPoint) && 
		(POrderList (nWeaponIndex) < POrderList (nSupposedWeapon)))
		SelectWeapon (nWeaponIndex,0,0,1);
	}
return ammoCount;	//return amount used
}

//------------------------------------------------------------------------------

int32_t PickupVulcanAmmo (CObject *pObj, int32_t nPlayer)
{
	int32_t	pwSave = gameData.weaponData.nPrimary;	
	int32_t	nAmmo = gameStates.app.bNostalgia ? VULCAN_CLIP_CAPACITY : pObj->cType.powerupInfo.nCount; // ammo available in this clip
	int32_t	nUsed = PickupAmmo (CLASS_PRIMARY, VULCAN_INDEX, nAmmo, NULL, nPlayer); // what the player actually took this time

// Ugh, save selected primary weapon around the picking up of the ammo.  
// I apologize for this code.  Matthew A. Toschlog
if (nUsed) {
	if (ISLOCALPLAYER (nPlayer))
		PickupEffect (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO, nPlayer);
	MultiSendAmmo ();
	pObj->cType.powerupInfo.nCount -= nUsed;
	MultiSendAmmoUpdate (pObj->Index ());
	gameData.weaponData.nPrimary = pwSave;
	return nUsed >= nAmmo;
	} 
else {
	int32_t nMaxAmmo = nMaxPrimaryAmmo [VULCAN_INDEX];
	if (LOCALPLAYER.flags & PLAYER_FLAGS_AMMO_RACK)
		nMaxAmmo *= 2;
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %d %s!", TXT_ALREADY_HAVE,X2I ((uint32_t) VULCAN_AMMO_SCALE * (uint32_t) nMaxAmmo), TXT_VULCAN_ROUNDS);
	gameData.weaponData.nPrimary = pwSave;
	return 0;
	}
}

//------------------------------------------------------------------------------

int32_t PickupLaser (CObject *pObj, int32_t nId, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;

if (pPlayer->AddStandardLaser ()) {
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordLaserLevel ((int8_t) pPlayer->LaserLevel (0) - 1, (int8_t) pPlayer->LaserLevel (0));
	PickupEffect (10, 0, 10, LASER_SCORE, "%s %s %d", TXT_LASER, TXT_BOOSTED_TO, pPlayer->LaserLevel () + 1);
	cockpit->UpdateLaserWeaponInfo ();
	PickupPrimary (LASER_INDEX, nPlayer);
	return 1;
	}
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_MAXED_OUT, TXT_LASER);
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (pObj, nPlayer);
}

//------------------------------------------------------------------------------

int32_t PickupSuperLaser (CObject *pObj, int32_t nId, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;

if (pPlayer->AddSuperLaser ()) {
	bLastPrimaryWasSuper [LASER_INDEX] = 1;
	if (ISLOCALPLAYER (nPlayer)) {
		if (gameData.demoData.nState == ND_STATE_RECORDING)
			NDRecordLaserLevel (pPlayer->LaserLevel () - 1, pPlayer->LaserLevel ());
		PickupEffect (10, 0, 10, LASER_SCORE, TXT_SUPERBOOST, pPlayer->LaserLevel () + 1, nPlayer);
		cockpit->UpdateLaserWeaponInfo ();
		if (gameData.weaponData.nPrimary != LASER_INDEX)
		   CheckToUsePrimary (SUPER_LASER_INDEX);
		}
	return 1;
	}
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage (TXT_LASER_MAXEDOUT);
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (pObj, nPlayer);
}

//------------------------------------------------------------------------------

int32_t PickupQuadLaser (CObject *pObj, int32_t nId, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;

if (!(pPlayer->flags & PLAYER_FLAGS_QUAD_LASERS)) {
	pPlayer->flags |= PLAYER_FLAGS_QUAD_LASERS;
	PickupEffect (15, 15, 7, QUAD_FIRE_SCORE, "%s!", TXT_QUAD_LASERS);
	cockpit->UpdateLaserWeaponInfo ();
	return 1;
	}
if (nPlayer == N_LOCALPLAYER)
	HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, TXT_QUAD_LASERS);
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (pObj, nPlayer);
}

//------------------------------------------------------------------------------

int32_t PickupGun (CObject *pObj, int32_t nId, int32_t nPlayer)
{
if (PickupPrimary (nId, nPlayer)) {
	if ((nId == OMEGA_INDEX) && (nPlayer == N_LOCALPLAYER))
		gameData.omegaData.xCharge [IsMultiGame] = pObj->cType.powerupInfo.nCount;
	return 1;
	}
if (IsMultiGame)
	return 0;
return PickupEnergyBoost (NULL, nPlayer);
}

//	-----------------------------------------------------------------------------

int32_t PickupGatlingGun (CObject *pObj, int32_t nId, int32_t nPlayer)
{
	int32_t nAmmo = pObj->cType.powerupInfo.nCount;
	int32_t bPickedUp = PickupPrimary (nId, nPlayer);

//didn't get the weapon (because we already have it), but
//maybe snag some of the nAmmo.  if singleplayer, grab all the ammo
//and remove the powerup.  If multiplayer take ammo in excess of
//the amount in a powerup, and leave the rest.
if (!bPickedUp && IsMultiGame)
	nAmmo -= VULCAN_CLIP_CAPACITY;	//don't let take all ammo
if (nAmmo > 0) {
	int32_t nAmmoUsed = PickupAmmo (CLASS_PRIMARY, VULCAN_INDEX, nAmmo, NULL, nPlayer);
	pObj->cType.powerupInfo.nCount -= nAmmoUsed;
	if (ISLOCALPLAYER (nPlayer)) {
		if (!bPickedUp && nAmmoUsed) {
			PickupEffect (7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
			return pObj->cType.powerupInfo.nCount ? -1 : -2;
			}
		}
	}
return bPickedUp;
}

//	-----------------------------------------------------------------------------
//eof
