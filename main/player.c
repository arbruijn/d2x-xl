/* $Id: player.c,v 1.3 2003/10/10 09:36:35 btb Exp $ */

/*
 *
 * Player Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "multi.h"
#include "input.h"

#ifdef RCS
static char rcsid[] = "$Id: player.c,v 1.3 2003/10/10 09:36:35 btb Exp $";
#endif

//-------------------------------------------------------------------------
// reads a tPlayerShip structure from a CFILE
 
void PlayerShipRead (tPlayerShip *ps, CFILE *fp)
{
	int i;

ps->nModel = CFReadInt (fp);
ps->nExplVClip = CFReadInt (fp);
ps->mass = CFReadFix (fp);
ps->drag = CFReadFix (fp);
ps->maxThrust = CFReadFix (fp);
ps->reverseThrust = CFReadFix (fp);
ps->brakes = CFReadFix (fp);
ps->wiggle = CFReadFix (fp);
ps->maxRotThrust = CFReadFix (fp);
for (i = 0; i < N_PLAYER_GUNS; i++)
	CFReadVector (ps->gunPoints + i, fp);
}

//-------------------------------------------------------------------------

int EquippedPlayerGun (tObject *objP)
{
if (objP->nType == OBJ_PLAYER) {
		int		nPlayer = objP->id;
		int		nWeapon = gameData.multiplayer.weaponStates [nPlayer].nPrimary;

	return (nWeapon || (gameData.multiplayer.weaponStates [nPlayer].nLaserLevel <= MAX_LASER_LEVEL)) ? nWeapon : SUPER_LASER_INDEX;
	}
return 0;
}

//-------------------------------------------------------------------------

static int nBombIds [] = {SMART_INDEX, MEGA_INDEX, EARTHSHAKER_INDEX};

int EquippedPlayerBomb (tObject *objP)
{
if (objP->nType == OBJ_PLAYER) {
		int		nPlayer = objP->id;
		int		i, nWeapon = gameData.multiplayer.weaponStates [nPlayer].nSecondary;

	for (i = 0; i < sizeofa (nBombIds); i++)
		if (nWeapon == nBombIds [i])
			return i + 1;
	}
return 0;
}

//-------------------------------------------------------------------------

static int nMissileIds [] = {CONCUSSION_INDEX, HOMING_INDEX, FLASHMSL_INDEX, GUIDED_INDEX, MERCURY_INDEX};

int EquippedPlayerMissile (tObject *objP, int *nMissiles)
{
if (objP->nType == OBJ_PLAYER) {
		int		nPlayer = objP->id;
		int		i, nWeapon = gameData.multiplayer.weaponStates [nPlayer].nSecondary;

	for (i = 0; i < sizeofa (nMissileIds); i++)
		if (nWeapon == nMissileIds [i]) {
			*nMissiles = gameData.multiplayer.weaponStates [nPlayer].nMissiles;
			return i + 1;
			}
	}
*nMissiles = 0;
return 0;
}

//-------------------------------------------------------------------------

void UpdatePlayerWeaponInfo (void)
{
	int				i, bUpdate = 0, bGatling = (gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX);
	tWeaponState	*wsP = gameData.multiplayer.weaponStates + gameData.multiplayer.nLocalPlayer;

if (gameStates.app.bPlayerIsDead)
	gameData.weapons.firing [0].nStart = 
	gameData.weapons.firing [0].nDuration = 
	gameData.weapons.firing [0].nStop = 
	gameData.weapons.firing [1].nStart = 
	gameData.weapons.firing [1].nDuration =
	gameData.weapons.firing [1].nStop = 0;
else {
	if ((Controls [0].firePrimaryState != 0) || (Controls [0].firePrimaryDownCount != 0)) {
		if (gameData.weapons.firing [0].nStart <= 0) {
			gameData.weapons.firing [0].nStart = gameStates.app.nSDLTicks;
			if (!gameOpts->sound.bSpinup)
				gameData.weapons.firing [0].nStart -= GATLING_DELAY;
			else if (bGatling && gameOpts->sound.bGatling)
				DigiPlayWAV ("gatling-speedup.wav", F1_0);
			}
		gameData.weapons.firing [0].nDuration = gameStates.app.nSDLTicks - gameData.weapons.firing [0].nStart;
		gameData.weapons.firing [0].nStop = 0;
		if (bGatling && gameOpts->sound.bGatling && (gameData.weapons.firing [0].nDuration >= GATLING_DELAY) && (gameData.weapons.firing [0].bSound <= 0)) {
			if (gameData.weapons.nPrimary == VULCAN_INDEX)
				DigiLinkSoundToObject3 (-1, LOCALPLAYER.nObject, 1, F1_0, i2f (256), -1, -1, "vulcan-firing.wav", 1, SOUNDCLASS_PLAYER);
			else if (gameData.weapons.nPrimary == GAUSS_INDEX)
				DigiLinkSoundToObject3 (-1, LOCALPLAYER.nObject, 1, F1_0, i2f (256), -1, -1, "gauss-firing.wav", 1, SOUNDCLASS_PLAYER);
			gameData.weapons.firing [0].bSound = 1;
			}
		}
	else if (gameData.weapons.firing [0].nDuration) {
		gameData.weapons.firing [0].nStop = gameStates.app.nSDLTicks;
		gameData.weapons.firing [0].nDuration = 
		gameData.weapons.firing [0].nStart = 0;
		if (bGatling) {
			if (gameData.weapons.firing [0].bSound > 0) {
				DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
				gameData.weapons.firing [0].bSound = 0;
				}
			if (gameOpts->sound.bGatling)
				DigiPlayWAV ("gatling-slowdown.wav", F1_0);
			}
		}
	if ((Controls [0].fireSecondaryState != 0) || (Controls [0].fireSecondaryDownCount != 0)) {
		if (gameData.weapons.firing [1].nStart <= 0)
			gameData.weapons.firing [1].nStart = gameStates.app.nSDLTicks;
		gameData.weapons.firing [1].nDuration = gameStates.app.nSDLTicks - gameData.weapons.firing [1].nStart;
		gameData.weapons.firing [1].nStop = 0;
		}
	else if (gameData.weapons.firing [1].nDuration) {
		gameData.weapons.firing [1].nStop = gameStates.app.nSDLTicks;
		gameData.weapons.firing [1].nDuration = 
		gameData.weapons.firing [1].nStart = 0;
		if (bGatling && (gameData.weapons.firing [0].bSound > 0)) {
			DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
			gameData.weapons.firing [0].bSound = 0;
			}
		}
	}
if (wsP->nPrimary != gameData.weapons.nPrimary) {
	if ((gameData.weapons.firing [0].bSound > 0) && 
		 ((wsP->nPrimary == VULCAN_INDEX) || (wsP->nPrimary == GAUSS_INDEX))) {
		DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
		gameData.weapons.firing [0].bSound = 0;
		}
	wsP->nPrimary = gameData.weapons.nPrimary;
	bUpdate = 1;
	}
if (wsP->nSecondary != gameData.weapons.nSecondary) {
	wsP->nSecondary = gameData.weapons.nSecondary;
	bUpdate = 1;
	}
if (wsP->bQuadLasers != ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) != 0)) {
	wsP->bQuadLasers = ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) != 0);
	bUpdate = 1;
	}
for (i = 0; i < 2; i++) {
	if (wsP->firing [i].nStart != gameData.weapons.firing [i].nStart) {
		wsP->firing [i].nStart = gameData.weapons.firing [i].nStart;
		bUpdate = 1;
		}
	if (wsP->firing [i].nDuration != gameData.weapons.firing [i].nDuration) {
		wsP->firing [i].nDuration = gameData.weapons.firing [i].nDuration;
		bUpdate = 1;
		}
	if (wsP->firing [i].nStop != gameData.weapons.firing [i].nStop) {
		wsP->firing [i].nStop = gameData.weapons.firing [i].nStop;
		bUpdate = 1;
		}
	}
if (wsP->nMissiles != LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]) {
	wsP->nMissiles = (char) LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
	bUpdate = 1;
	}
if (wsP->nLaserLevel != LOCALPLAYER.laserLevel) {
	wsP->nLaserLevel = LOCALPLAYER.laserLevel;
	bUpdate = 1;
	}
if (wsP->bTripleFusion != gameData.weapons.bTripleFusion) {
	wsP->bTripleFusion = gameData.weapons.bTripleFusion;
	bUpdate = 1;
	}
if (wsP->nMslLaunchPos != (gameData.laser.nMissileGun & 3)) {
	wsP->nMslLaunchPos = gameData.laser.nMissileGun & 3;
	bUpdate = 1;
	}
if (wsP->xMslFireTime != gameData.missiles.xNextFireTime) {
	wsP->xMslFireTime = gameData.missiles.xNextFireTime;
	bUpdate = 1;
	}
if (bUpdate)
	MultiSendPlayerWeapons (gameData.multiplayer.nLocalPlayer);
}

//-------------------------------------------------------------------------
