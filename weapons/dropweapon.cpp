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
int32_t SpitPowerup (CObject *pSpitter, uint8_t id, int32_t seed)
{
ENTER (0, 0);
	int16_t					nObject;
	CObject*					pObj;
	CFixVector				newVelocity, newPos;
	tObjTransformation	*pPos = OBJPOS (pSpitter);

if (seed > 0)
	gameStates.app.SRand (seed);
newVelocity = pSpitter->mType.physInfo.velocity + pSpitter->info.position.mOrient.m.dir.f * I2X (SPIT_SPEED);
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
newPos = pPos->vPos + pPos->mOrient.m.dir.f * pSpitter->info.xSize;
if (IsMultiGame && (gameData.multigame.create.nCount >= MAX_NET_CREATE_OBJECTS))
	RETVAL (-1)
nObject = CreatePowerup (id, int16_t (GetTeam (N_LOCALPLAYER) + 1), int16_t (OBJSEG (pSpitter)), newPos, 0);
pObj = OBJECT (nObject);
if (!pObj)
	RETVAL (-1)
pObj->mType.physInfo.velocity = newVelocity;
pObj->mType.physInfo.drag = 512;	//1024;
pObj->mType.physInfo.mass = I2X (1);
pObj->mType.physInfo.flags = PF_BOUNCES;
pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
pObj->rType.animationInfo.xFrameTime = gameData.effectData.vClipP [pObj->rType.animationInfo.nClipIndex].xFrameTime;
pObj->rType.animationInfo.nCurFrame = 0;
if (pSpitter == gameData.objData.pConsole)
	pObj->cType.powerupInfo.nFlags |= PF_SPAT_BY_PLAYER;
switch (pObj->info.nId) {
	case POW_CONCUSSION_1:
	case POW_CONCUSSION_4:
	case POW_SHIELD_BOOST:
	case POW_ENERGY:
		pObj->SetLife ((RandShort () + I2X (3)) * 64);		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
		if (IsMultiGame)
			pObj->info.xLifeLeft /= 2;
		break;
	default:
		//if (IsMultiGame)
		//	pObj->info.xLifeLeft = (RandShort () + I2X (3)) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
		break;
	}
RETVAL (nObject)
}

//	-----------------------------------------------------------------------------

static inline int32_t IsBuiltInDevice (int32_t nDeviceFlag)
{
return gameStates.app.bHaveExtraGameInfo [IsMultiGame] && ((extraGameInfo [IsMultiGame].loadout.nDevice & nDeviceFlag) != 0);
}

//	-----------------------------------------------------------------------------

void DropCurrentWeapon (void)
{
ENTER (0, 0);
	int32_t	nObject = -1,
				ammo = 0;

if (IsMultiGame)
	gameStates.app.SRand ();

if (gameData.weaponData.nPrimary == 0) {	//special laser drop handling
	if ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) && !IsBuiltInDevice (PLAYER_FLAGS_QUAD_LASERS)) {
		LOCALPLAYER.flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		nObject = SpitPowerup (gameData.objData.pConsole, POW_QUADLASER);
		if (nObject < 0) {
			LOCALPLAYER.flags |= PLAYER_FLAGS_QUAD_LASERS;
			RETURN
			}
		HUDInitMessage(TXT_DROP_QLASER);
		}
	else if (!IsBuiltinWeapon (SUPER_LASER_INDEX) && LOCALPLAYER.DropSuperLaser ()) {
		nObject = SpitPowerup (gameData.objData.pConsole, POW_SUPERLASER);
		if (nObject < 0) {
			LOCALPLAYER.AddSuperLaser ();
			RETURN
			}
		HUDInitMessage (TXT_DROP_SLASER);
		}
	else if (!IsBuiltinWeapon (SUPER_LASER_INDEX) && LOCALPLAYER.DropStandardLaser ()) {
		nObject = SpitPowerup (gameData.objData.pConsole, POW_LASER);
		if (nObject < 0) {
			LOCALPLAYER.AddStandardLaser ();
			RETURN
			}
		HUDInitMessage (TXT_DROP_LASER);
		}
	}
else {
	if ((gameData.weaponData.nPrimary == 4) && gameData.weaponData.bTripleFusion) {
		gameData.weaponData.bTripleFusion = 0;
		nObject = SpitPowerup (gameData.objData.pConsole, primaryWeaponToPowerup [gameData.weaponData.nPrimary]);
		}
	else if (gameData.weaponData.nPrimary && !IsBuiltinWeapon (gameData.weaponData.nPrimary)) { //if selected weapon was not the laser
		LOCALPLAYER.primaryWeaponFlags &= (~(1 << gameData.weaponData.nPrimary));
		nObject = SpitPowerup (gameData.objData.pConsole, primaryWeaponToPowerup [gameData.weaponData.nPrimary]);
		}
	if (nObject < 0) {	// couldn't drop
		if (gameData.weaponData.nPrimary) 	//if selected weapon was not the laser
			LOCALPLAYER.primaryWeaponFlags |= (1 << gameData.weaponData.nPrimary);
		RETURN
		}
	HUDInitMessage (TXT_DROP_WEAPON, PRIMARY_WEAPON_NAMES (gameData.weaponData.nPrimary));
	}
audio.PlaySound (SOUND_DROP_WEAPON);
if ((gameData.weaponData.nPrimary == VULCAN_INDEX) || (gameData.weaponData.nPrimary == GAUSS_INDEX)) {
	//if it's one of these, drop some ammo with the weapon
	ammo = LOCALPLAYER.primaryAmmo [VULCAN_INDEX];
	if ((LOCALPLAYER.primaryWeaponFlags & HAS_FLAG (VULCAN_INDEX)) && (gameData.weaponData.nPrimary == GAUSS_INDEX))
		ammo /= 2;		//if both vulcan & gauss, drop half
	LOCALPLAYER.primaryAmmo [VULCAN_INDEX] -= ammo;
	if (nObject >= 0)
		OBJECT (nObject)->cType.powerupInfo.nCount = ammo;
	}
if (gameData.weaponData.nPrimary == OMEGA_INDEX) {
	//dropped weapon has current energy
	if (nObject >= 0)
		OBJECT (nObject)->cType.powerupInfo.nCount = gameData.omegaData.xCharge [IsMultiGame];
	}
if (IsMultiGame)
	MultiSendDropWeapon (nObject);
if (gameData.weaponData.nPrimary) //if selected weapon was not the laser
	AutoSelectWeapon (0, 0);
RETURN
}

//	-----------------------------------------------------------------------------

extern void DropOrb (void);

void DropSecondaryWeapon (int32_t nWeapon, int32_t nAmount, int32_t bSilent)
{
ENTER (0, 0);
if (nWeapon < 0)
	nWeapon = gameData.weaponData.nSecondary;
if ((LOCALPLAYER.secondaryAmmo [nWeapon] == 0) || 
	 (IsMultiGame && (nWeapon == 0) && (LOCALPLAYER.secondaryAmmo [nWeapon] <= gameData.multiplayer.weaponStates [N_LOCALPLAYER].nBuiltinMissiles))) {
	if (!bSilent)
		HUDInitMessage (TXT_CANT_DROP_SEC);
	RETURN
	}

int32_t nPowerup = secondaryWeaponToPowerup [0][nWeapon];
int32_t bHoardEntropy = (gameData.appData.GameMode (GM_HOARD | GM_ENTROPY)) != 0;
int32_t bMine = (nPowerup == POW_PROXMINE) || (nPowerup == POW_SMARTMINE);

if (!bHoardEntropy && bMine && LOCALPLAYER.secondaryAmmo [nWeapon] < 4) {
	HUDInitMessage(TXT_DROP_NEED4);
	RETURN
	}
if (bHoardEntropy) {
	DropOrb ();
	RETURN
	}

int32_t nItems = nAmount;
if (bMine)
	LOCALPLAYER.secondaryAmmo [nWeapon] -= 4;
else {
	LOCALPLAYER.secondaryAmmo [nWeapon] -= nAmount;
	if ((nAmount % 4 == 0) && (secondaryWeaponToPowerup [1][nWeapon] > -1)) { // amount is multiple of four and four pack of this powerup available
		nPowerup = secondaryWeaponToPowerup [1][nWeapon];
		nItems /= 4;
		}
	}

for (int32_t i = 0; i < nItems; i++) {
	int32_t nObject = SpitPowerup (gameData.objData.pConsole, nPowerup);
	if (nObject == -1) { // can't put any more objects in the mine
		if (bMine)
			LOCALPLAYER.secondaryAmmo [nWeapon] += 4;
		else
			LOCALPLAYER.secondaryAmmo [nWeapon] += nAmount;
		RETURN
		}
	if (!bSilent) {
		HUDInitMessage (TXT_DROP_WEAPON, SECONDARY_WEAPON_NAMES (gameData.weaponData.nSecondary));
		audio.PlaySound (SOUND_DROP_WEAPON);
		}
	if (IsMultiGame)
		MultiSendDropWeapon (nObject);
	if (LOCALPLAYER.secondaryAmmo [nWeapon] == 0) {
		LOCALPLAYER.secondaryWeaponFlags &= (~(1 << gameData.weaponData.nSecondary));
		AutoSelectWeapon (1, 0);
		}
	}
RETURN
}

//	-----------------------------------------------------------------------------
//eof
