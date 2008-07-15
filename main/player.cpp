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
#include "network.h"

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

	for (i = 0; i < (int) sizeofa (nBombIds); i++)
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

	for (i = 0; i < (int) sizeofa (nMissileIds); i++)
		if (nWeapon == nMissileIds [i]) {
			*nMissiles = gameData.multiplayer.weaponStates [nPlayer].nMissiles;
			return i + 1;
			}
	}
*nMissiles = 0;
return 0;
}

//-------------------------------------------------------------------------

static inline int WIFireTicks (int nWeapon)
{
return 1000 * WI_fire_wait (nWeapon) / F1_0;
}

//-------------------------------------------------------------------------

void UpdateFiringSounds (void)
{
	tWeaponState	*wsP = gameData.multiplayer.weaponStates;
	tFiringData		*fP;
	int				bGatling, bGatlingSound, i;

bGatlingSound = (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling;
for (i = 0; i < gameData.multiplayer.nPlayers; i++, wsP++) {
	if (!IsMultiGame || gameData.multiplayer.players [i].connected) {
		bGatling = (wsP->nPrimary == VULCAN_INDEX) || (wsP->nPrimary == GAUSS_INDEX);
		fP = wsP->firing;
		if (bGatling && bGatlingSound && (fP->bSound == 1)) {
			DigiLinkSoundToObject3 (-1, (short) gameData.multiplayer.players [i].nObject, 0, 
											F1_0, 256 * F1_0, -1, -1, AddonSoundName (SND_ADDON_GATLING_SPIN), 0, SOUNDCLASS_PLAYER);
			fP->bSound = 0;
			}
		}
	}
}

//-------------------------------------------------------------------------

void UpdateFiringState (void)
{
	int	bGatling = (gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX);

if ((Controls [0].firePrimaryState != 0) || (Controls [0].firePrimaryDownCount != 0)) {
	if (gameData.weapons.firing [0].nStart <= 0) {
		gameData.weapons.firing [0].nStart = gameStates.app.nSDLTicks;
		if (bGatling) {
			if (EGI_FLAG (bGatlingSpeedUp, 1, 0, 0))
				gameData.weapons.firing [0].bSound = 1;
			else {
				gameData.weapons.firing [0].nStart -= GATLING_DELAY + 1;
				gameData.weapons.firing [0].bSound = 0;
				}
			}
		}
	gameData.weapons.firing [0].nDuration = gameStates.app.nSDLTicks - gameData.weapons.firing [0].nStart;
	gameData.weapons.firing [0].nStop = 0;
	}
else if (gameData.weapons.firing [0].nDuration) {
	gameData.weapons.firing [0].nStop = gameStates.app.nSDLTicks;
	gameData.weapons.firing [0].nDuration = 
	gameData.weapons.firing [0].nStart = 0;
	}
else if (gameData.weapons.firing [0].nStop > 0) {
	if (gameStates.app.nSDLTicks - gameData.weapons.firing [0].nStop >= GATLING_DELAY /*WIFireTicks (gameData.weapons.nPrimary) * 4 / 5*/) {
		gameData.weapons.firing [0].nStop = 0;
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
	}
}

//-------------------------------------------------------------------------

void UpdatePlayerWeaponInfo (void)
{
	int				i, bUpdate = 0;
	tWeaponState	*wsP = gameData.multiplayer.weaponStates + gameData.multiplayer.nLocalPlayer;
	tFiringData		*fP;

if (gameStates.app.bPlayerIsDead)
	gameData.weapons.firing [0].nStart = 
	gameData.weapons.firing [0].nDuration = 
	gameData.weapons.firing [0].nStop = 
	gameData.weapons.firing [1].nStart = 
	gameData.weapons.firing [1].nDuration =
	gameData.weapons.firing [1].nStop = 0;
else
	UpdateFiringState ();
if (wsP->nPrimary != gameData.weapons.nPrimary) {
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
for (i = 0, fP = wsP->firing; i < 2; i++, fP++) {
	if (fP->nStart != gameData.weapons.firing [i].nStart) {
		fP->nStart = gameData.weapons.firing [i].nStart;
		bUpdate = 1;
		}
	if (fP->nDuration != gameData.weapons.firing [i].nDuration) {
		fP->nDuration = gameData.weapons.firing [i].nDuration;
		bUpdate = 1;
		}
	if (fP->nStop != gameData.weapons.firing [i].nStop) {
		fP->nStop = gameData.weapons.firing [i].nStop;
		bUpdate = 1;
		}
	if (gameData.weapons.firing [i].bSound == 1) {
		fP->bSound = 1;
		gameData.weapons.firing [i].bSound = 0;
		}
	if (fP->bSpeedUp != EGI_FLAG (bGatlingSpeedUp, 1, 0, 0)) {
		fP->bSpeedUp = EGI_FLAG (bGatlingSpeedUp, 1, 0, 0);
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
UpdateFiringSounds ();
}

//-------------------------------------------------------------------------
