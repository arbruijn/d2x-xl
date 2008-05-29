/* $Id: weapon.c,v 1.9 2003/10/11 09:28:38 btb Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: weapon.c,v 1.9 2003/10/11 09:28:38 btb Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "weapon.h"
#include "error.h"
#include "sounds.h"
#include "text.h"
#include "multi.h"
#include "network.h"

//	-----------------------------------------------------------------------------

//called when a primary weapon is picked up
//returns true if actually picked up
int PickupPrimary (int nWeaponIndex, int nPlayer)
{
	tPlayer	*playerP = gameData.multiplayer.players + nPlayer;
	//ushort oldFlags = LOCALPLAYER.primaryWeaponFlags;
	ushort flag = 1 << nWeaponIndex;
	int cutpoint;
	int supposed_weapon = gameData.weapons.nPrimary;
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
	cutpoint = POrderList (255);
	if ((gameData.weapons.nPrimary == LASER_INDEX) && 
		(playerP->laserLevel >= 4))
		supposed_weapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((gameOpts->gameplay.nAutoSelectWeapon == 2) && 
		 (POrderList (nWeaponIndex) < cutpoint) && 
		 (POrderList (nWeaponIndex) < POrderList (supposed_weapon)))
		SelectWeapon (nWeaponIndex, 0, 0, 1);
	PALETTE_FLASH_ADD (7,14,21);
	if (nWeaponIndex != LASER_INDEX)
  		HUDInitMessage ("%s!", PRIMARY_WEAPON_NAMES (nWeaponIndex));
	}
return 1;
}

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int PickupSecondary (tObject *objP, int nWeaponIndex, int count, int nPlayer)
{
	int		max;
	int		nPickedUp;
	int		cutpoint, bEmpty = 0, bSmokeGrens;
	tPlayer	*playerP = gameData.multiplayer.players + nPlayer;

if ((nWeaponIndex == PROXMINE_INDEX) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0)) {
	bSmokeGrens = 1;
	max = 4;
	}
else {
	bSmokeGrens = 0;
	max = nMaxSecondaryAmmo [nWeaponIndex];
	if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
		max *= 2;
	}
if (playerP->secondaryAmmo [nWeaponIndex] >= max) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage("%s %i %ss!", 
			TXT_ALREADY_HAVE, 
			playerP->secondaryAmmo [nWeaponIndex],
			bSmokeGrens ? TXT_SMOKE_GRENADE : SECONDARY_WEAPON_NAMES (nWeaponIndex));
	return 0;
	}
playerP->secondaryWeaponFlags |= (1 << nWeaponIndex);
playerP->secondaryAmmo [nWeaponIndex] += count;
nPickedUp = count;
if (playerP->secondaryAmmo [nWeaponIndex] > max) {
	nPickedUp = count - (playerP->secondaryAmmo [nWeaponIndex] - max);
	playerP->secondaryAmmo [nWeaponIndex] = max;
	if ((nPickedUp < count) && (nWeaponIndex != PROXMINE_INDEX) && (nWeaponIndex != SMARTMINE_INDEX)) {
		short nObject = OBJ_IDX (objP);
		gameData.multiplayer.leftoverPowerups [nObject].nCount = count - nPickedUp;
		gameData.multiplayer.leftoverPowerups [nObject].nType = secondaryWeaponToPowerup [nWeaponIndex];
		gameData.multiplayer.leftoverPowerups [nObject].spitterP = OBJECTS + playerP->nObject;
		}
	}
if (ISLOCALPLAYER (nPlayer)) {
	cutpoint = SOrderList (255);
	bEmpty = playerP->secondaryAmmo [gameData.weapons.nSecondary] == 0;
	if (gameOpts->gameplay.nAutoSelectWeapon) {
		if (gameOpts->gameplay.nAutoSelectWeapon == 1) {
			if (bEmpty)
				SelectWeapon (nWeaponIndex, 1, 0, 1);
			}
		else if ((SOrderList (nWeaponIndex) < cutpoint) && 
					(bEmpty || (SOrderList (nWeaponIndex) < SOrderList (gameData.weapons.nSecondary))))
			SelectWeapon(nWeaponIndex,1, 0, 1);
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
	if (count>1) {
		PALETTE_FLASH_ADD (15,15,15);
		HUDInitMessage("%d %s%s", nPickedUp, bSmokeGrens ? TXT_SMOKE_GRENADES : SECONDARY_WEAPON_NAMES (nWeaponIndex), TXT_SX);
		}
	else {
		PALETTE_FLASH_ADD (10,10,10);
		HUDInitMessage("%s!", bSmokeGrens ? TXT_SMOKE_GRENADE : SECONDARY_WEAPON_NAMES (nWeaponIndex));
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------

//called when ammo (for the vulcan cannon) is picked up
//	Returns the amount picked up
int PickupAmmo (int classFlag,int nWeaponIndex,int ammoCount, int nPlayer)
{
	int		max,cutpoint,supposed_weapon=gameData.weapons.nPrimary;
	int		old_ammo=classFlag;		//kill warning
	tPlayer	*playerP = gameData.multiplayer.players + nPlayer;

Assert(classFlag==CLASS_PRIMARY && nWeaponIndex==VULCAN_INDEX);

max = nMaxPrimaryAmmo [nWeaponIndex];
if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
	max *= 2;
if (playerP->primaryAmmo [nWeaponIndex] == max)
	return 0;
old_ammo = playerP->primaryAmmo [nWeaponIndex];
playerP->primaryAmmo [nWeaponIndex] += ammoCount;
if (playerP->primaryAmmo [nWeaponIndex] > max) {
	ammoCount += (max - playerP->primaryAmmo [nWeaponIndex]);
	playerP->primaryAmmo [nWeaponIndex] = max;
	}
if ((nPlayer = gameData.multiplayer.nLocalPlayer)) {
	cutpoint = POrderList (255);
	if ((gameData.weapons.nPrimary == LASER_INDEX) && (playerP->laserLevel >= 4))
		supposed_weapon = SUPER_LASER_INDEX;  // allotment for stupid way of doing super laser
	if ((playerP->primaryWeaponFlags & (1<<nWeaponIndex)) && 
		(nWeaponIndex > gameData.weapons.nPrimary) && 
		(old_ammo == 0) &&
		(POrderList (nWeaponIndex) < cutpoint) && 
		(POrderList (nWeaponIndex) < POrderList (supposed_weapon)))
		SelectWeapon(nWeaponIndex,0,0,1);
	}
return ammoCount;	//return amount used
}

//	-----------------------------------------------------------------------------
//eof
