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

#include "descent.h"
#include "fireweapon.h"
#include "weapon.h"
#include "error.h"
#include "sounds.h"
#include "text.h"
#include "network.h"
#include "hudmsgs.h"

//	-----------------------------------------------------------------------------

#define SPIT_SPEED 20

//this function is for when the player intentionally drops a powerup
//this function is based on DropPowerup()
int SpitPowerup (CObject *spitterP, ubyte id, int seed)
{
	short			nObject;
	CObject		*objP;
	CFixVector	newVelocity, newPos;
	tObjTransformation	*posP = OBJPOS (spitterP);

d_srand (gameStates.app.nRandSeed = (seed < 0) ? RandShort () : seed);
newVelocity = spitterP->mType.physInfo.velocity + spitterP->info.position.mOrient.m.dir.f * I2X (SPIT_SPEED);
newVelocity.v.coord.x += (SRandShort ()) * SPIT_SPEED * 2;
newVelocity.v.coord.y += (SRandShort ()) * SPIT_SPEED * 2;
newVelocity.v.coord.z += (SRandShort ()) * SPIT_SPEED * 2;
// Give keys zero velocity so they can be tracked better in multi
if (IsMultiGame && (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD))
	newVelocity.SetZero ();
//there's a piece of code which lets the player pick up a powerup if
//the distance between him and the powerup is less than 2 time their
//combined radii.  So we need to create powerups pretty far out from
//the player.
newPos = posP->vPos + posP->mOrient.m.dir.f * spitterP->info.xSize;
if (IsMultiGame && (gameData.multigame.create.nCount >= MAX_NET_CREATE_OBJECTS))
	return -1;
nObject = CreatePowerup (id, short (GetTeam (N_LOCALPLAYER) + 1), short (OBJSEG (spitterP)), newPos, 0);
if (nObject < 0) {
	Int3();
	return nObject;
	}
objP = OBJECTS + nObject;
objP->mType.physInfo.velocity = newVelocity;
objP->mType.physInfo.drag = 512;	//1024;
objP->mType.physInfo.mass = I2X (1);
objP->mType.physInfo.flags = PF_BOUNCE;
objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
objP->rType.vClipInfo.xFrameTime = gameData.eff.vClipP [objP->rType.vClipInfo.nClipIndex].xFrameTime;
objP->rType.vClipInfo.nCurFrame = 0;
if (spitterP == gameData.objs.consoleP)
	objP->cType.powerupInfo.nFlags |= PF_SPAT_BY_PLAYER;
switch (objP->info.nId) {
	case POW_CONCUSSION_1:
	case POW_CONCUSSION_4:
	case POW_SHIELD_BOOST:
	case POW_ENERGY:
		objP->SetLife ((RandShort () + I2X (3)) * 64);		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
		if (IsMultiGame)
			objP->info.xLifeLeft /= 2;
		break;
	default:
		//if (IsMultiGame)
		//	objP->info.xLifeLeft = (RandShort () + I2X (3)) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
		break;
	}
MultiSendWeapons (1);
return nObject;
}

//	-----------------------------------------------------------------------------

static inline int IsBuiltInDevice (int nDeviceFlag)
{
return gameStates.app.bHaveExtraGameInfo [IsMultiGame] && ((extraGameInfo [IsMultiGame].loadout.nDevice & nDeviceFlag) != 0);
}

//	-----------------------------------------------------------------------------

void DropCurrentWeapon (void)
{
	int	nObject = -1,
			ammo = 0;

if (gameData.weapons.nPrimary == 0) {	//special laser drop handling
	if ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) && !IsBuiltInDevice (PLAYER_FLAGS_QUAD_LASERS)) {
		LOCALPLAYER.flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		nObject = SpitPowerup (gameData.objs.consoleP, POW_QUADLASER);
		if (nObject < 0) {
			LOCALPLAYER.flags |= PLAYER_FLAGS_QUAD_LASERS;
			return;
			}
		HUDInitMessage(TXT_DROP_QLASER);
		}
	else if (!IsBuiltinWeapon (SUPER_LASER_INDEX) && LOCALPLAYER.DropSuperLaser ()) {
		nObject = SpitPowerup (gameData.objs.consoleP, POW_SUPERLASER);
		if (nObject < 0) {
			LOCALPLAYER.AddSuperLaser ();
			return;
			}
		HUDInitMessage (TXT_DROP_SLASER);
		}
	else if (!IsBuiltinWeapon (SUPER_LASER_INDEX) && LOCALPLAYER.DropStandardLaser ()) {
		nObject = SpitPowerup (gameData.objs.consoleP, POW_SUPERLASER);
		if (nObject < 0) {
			LOCALPLAYER.AddStandardLaser ();
			return;
			}
		HUDInitMessage (TXT_DROP_LASER);
		}
	}
else {
	if ((gameData.weapons.nPrimary == 4) && gameData.weapons.bTripleFusion) {
		gameData.weapons.bTripleFusion = 0;
		nObject = SpitPowerup (gameData.objs.consoleP, primaryWeaponToPowerup [gameData.weapons.nPrimary]);
		}
	else if (gameData.weapons.nPrimary && !IsBuiltinWeapon (gameData.weapons.nPrimary)) { //if selected weapon was not the laser
		LOCALPLAYER.primaryWeaponFlags &= (~(1 << gameData.weapons.nPrimary));
		nObject = SpitPowerup (gameData.objs.consoleP, primaryWeaponToPowerup [gameData.weapons.nPrimary]);
		}
	if (nObject < 0) {	// couldn't drop
		if (gameData.weapons.nPrimary) 	//if selected weapon was not the laser
			LOCALPLAYER.primaryWeaponFlags |= (1 << gameData.weapons.nPrimary);
		return;
		}
	HUDInitMessage (TXT_DROP_WEAPON, PRIMARY_WEAPON_NAMES (gameData.weapons.nPrimary));
	}
audio.PlaySound (SOUND_DROP_WEAPON);
if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
	//if it's one of these, drop some ammo with the weapon
	ammo = LOCALPLAYER.primaryAmmo [VULCAN_INDEX];
	if ((LOCALPLAYER.primaryWeaponFlags & HAS_FLAG (VULCAN_INDEX)) && (gameData.weapons.nPrimary == GAUSS_INDEX))
		ammo /= 2;		//if both vulcan & gauss, drop half
	LOCALPLAYER.primaryAmmo [VULCAN_INDEX] -= ammo;
	if (nObject >= 0)
		OBJECTS [nObject].cType.powerupInfo.nCount = ammo;
	}
if (gameData.weapons.nPrimary == OMEGA_INDEX) {
	//dropped weapon has current energy
	if (nObject >= 0)
		OBJECTS [nObject].cType.powerupInfo.nCount = gameData.omega.xCharge [IsMultiGame];
	}
if (IsMultiGame) {
	MultiSendDropWeapon (nObject);
	MultiSendWeapons (1);
	}
if (gameData.weapons.nPrimary) //if selected weapon was not the laser
	AutoSelectWeapon (0, 0);
}

//	-----------------------------------------------------------------------------

extern void DropOrb (void);

void DropSecondaryWeapon (int nWeapon)
{
if (nWeapon < 0)
	nWeapon = gameData.weapons.nSecondary;
if ((LOCALPLAYER.secondaryAmmo [nWeapon] == 0) || 
	 (IsMultiGame && (nWeapon == 0) && (LOCALPLAYER.secondaryAmmo [nWeapon] <= gameData.multiplayer.nBuiltinMissiles))) {
	HUDInitMessage (TXT_CANT_DROP_SEC);
	return;
	}

int nPowerup = secondaryWeaponToPowerup [nWeapon];
int bHoardEntropy = (gameData.app.GameMode (GM_HOARD | GM_ENTROPY)) != 0;
int bMine = (nPowerup == POW_PROXMINE) || (nPowerup == POW_SMARTMINE);

if (!bHoardEntropy && bMine && LOCALPLAYER.secondaryAmmo [nWeapon] < 4) {
	HUDInitMessage(TXT_DROP_NEED4);
	return;
	}
if (bHoardEntropy) {
	DropOrb ();
	return;
	}
if (bMine)
	LOCALPLAYER.secondaryAmmo [nWeapon] -= 4;
else
	LOCALPLAYER.secondaryAmmo [nWeapon]--;

int nObject = SpitPowerup (gameData.objs.consoleP, nPowerup);

if (nObject == -1) {
	if (bMine)
		LOCALPLAYER.secondaryAmmo [nWeapon] += 4;
	else
		LOCALPLAYER.secondaryAmmo [nWeapon]++;
	return;
	}
HUDInitMessage (TXT_DROP_WEAPON, SECONDARY_WEAPON_NAMES (gameData.weapons.nSecondary));
audio.PlaySound (SOUND_DROP_WEAPON);
if (IsMultiGame) {
	MultiSendDropWeapon (nObject);
	MultiSendWeapons (1);
	}
if (LOCALPLAYER.secondaryAmmo [nWeapon] == 0) {
	LOCALPLAYER.secondaryWeaponFlags &= (~(1<<gameData.weapons.nSecondary));
	AutoSelectWeapon (1, 0);
	}
}

//	-----------------------------------------------------------------------------
//eof
