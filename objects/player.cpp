/*
 *
 * Player Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "input.h"
#include "renderlib.h"
#include "network.h"
#include "marker.h"

tShipModifier shipModifiers [MAX_SHIP_TYPES] = {
	{{1.0f, 1.0f, 1.0f}},
	{{0.9f, 0.9f, 1.1f}},
	{{1.5f, 1.2f, 0.9f}}
};

//-------------------------------------------------------------------------
// reads a CPlayerShip structure from a CFile
 
void PlayerShipRead (CPlayerShip *ps, CFile& cf)
{
ps->nModel = cf.ReadInt ();
ps->nExplVClip = cf.ReadInt ();
ps->mass = cf.ReadFix ();
ps->drag = cf.ReadFix ();
ps->maxThrust = cf.ReadFix ();
ps->reverseThrust = cf.ReadFix ();
ps->brakes = cf.ReadFix ();
ps->wiggle = cf.ReadFix ();
ps->maxRotThrust = cf.ReadFix ();
for (int32_t i = 0; i < N_PLAYER_GUNS; i++)
	cf.ReadVector (ps->gunPoints[i]);
}

//-------------------------------------------------------------------------

int32_t EquippedPlayerGun (CObject *objP)
{
if (objP->info.nType == OBJ_PLAYER) {
		int32_t		nPlayer = objP->info.nId;
		int32_t		nWeapon = gameData.multiplayer.weaponStates [nPlayer].nPrimary;

	return (nWeapon || (gameData.multiplayer.weaponStates [nPlayer].nLaserLevel <= MAX_LASER_LEVEL)) ? nWeapon : SUPER_LASER_INDEX;
	}
return 0;
}

//-------------------------------------------------------------------------

static int32_t nBombIds [] = {SMART_INDEX, MEGA_INDEX, EARTHSHAKER_INDEX};

int32_t EquippedPlayerBomb (CObject *objP)
{
if (objP->info.nType == OBJ_PLAYER) {
		int32_t		nPlayer = objP->info.nId;
		int32_t		i, nWeapon = gameData.multiplayer.weaponStates [nPlayer].nSecondary;

	for (i = 0; i < (int32_t) sizeofa (nBombIds); i++)
		if (nWeapon == nBombIds [i])
			return i + 1;
	}
return 0;
}

//-------------------------------------------------------------------------

static int32_t nMissileIds [] = {CONCUSSION_INDEX, HOMING_INDEX, FLASHMSL_INDEX, GUIDED_INDEX, MERCURY_INDEX};

int32_t EquippedPlayerMissile (CObject *objP, int32_t *nMissiles)
{
if (objP->info.nType == OBJ_PLAYER) {
		int32_t		nPlayer = objP->info.nId;
		int32_t		i, nWeapon = gameData.multiplayer.weaponStates [nPlayer].nSecondary;

	for (i = 0; i < (int32_t) sizeofa (nMissileIds); i++)
		if (nWeapon == nMissileIds [i]) {
			*nMissiles = gameData.multiplayer.weaponStates [nPlayer].nMissiles;
			return i + 1;
			}
	}
*nMissiles = 0;
return 0;
}

//-------------------------------------------------------------------------

#if 0
static inline int32_t WIFireTicks (int32_t nWeapon)
{
return 1000 * WI_fire_wait (nWeapon) / I2X (1);
}
#endif

//-------------------------------------------------------------------------

void UpdateFiringSounds (void)
{
	CWeaponState	*wsP = gameData.multiplayer.weaponStates;
	tFiringData		*fP;
	int32_t				bGatling, bGatlingSound, i;

bGatlingSound = (gameOpts->UseHiresSound () == 2) && gameOpts->sound.bGatling;
for (i = 0; i < N_PLAYERS; i++, wsP++) {
	if (!IsMultiGame || PLAYER (i).IsConnected ()) {
		bGatling = (wsP->nPrimary == VULCAN_INDEX) || (wsP->nPrimary == GAUSS_INDEX);
		fP = wsP->firing;
		if (bGatling && bGatlingSound && (fP->bSound == 1)) {
			audio.CreateObjectSound (-1, SOUNDCLASS_PLAYER, (int16_t) PLAYER (i).nObject, 0, 
											 I2X (1), I2X (256), -1, -1, AddonSoundName (SND_ADDON_GATLING_SPIN), 0);
			fP->bSound = 0;
			}
		}
	}
}

//-------------------------------------------------------------------------

void UpdateFiringState (void)
{
	int32_t	bGatling = (gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX);

if ((controls [0].firePrimaryState != 0) || (controls [0].firePrimaryDownCount != 0)) {
	if (gameData.weapons.firing [0].nStart <= 0) {
		gameData.weapons.firing [0].nStart = gameStates.app.nSDLTicks [0];
		if (bGatling) {
			if (EGI_FLAG (bGatlingSpeedUp, 1, 0, 0))
				gameData.weapons.firing [0].bSound = 1;
			else {
				gameData.weapons.firing [0].nStart -= GATLING_DELAY + 1;
				gameData.weapons.firing [0].bSound = 0;
				}
			}
		}
	gameData.weapons.firing [0].nDuration = gameStates.app.nSDLTicks [0] - gameData.weapons.firing [0].nStart;
	gameData.weapons.firing [0].nStop = 0;
	}
else if (gameData.weapons.firing [0].nDuration) {
	gameData.weapons.firing [0].nStop = gameStates.app.nSDLTicks [0];
	gameData.weapons.firing [0].nDuration = 
	gameData.weapons.firing [0].nStart = 0;
	}
else if (gameData.weapons.firing [0].nStop > 0) {
	if (gameStates.app.nSDLTicks [0] - gameData.weapons.firing [0].nStop >= GATLING_DELAY /*WIFireTicks (gameData.weapons.nPrimary) * 4 / 5*/) {
		gameData.weapons.firing [0].nStop = 0;
		}
	}
if ((controls [0].fireSecondaryState != 0) || (controls [0].fireSecondaryDownCount != 0)) {
	if (gameData.weapons.firing [1].nStart <= 0)
		gameData.weapons.firing [1].nStart = gameStates.app.nSDLTicks [0];
	gameData.weapons.firing [1].nDuration = gameStates.app.nSDLTicks [0] - gameData.weapons.firing [1].nStart;
	gameData.weapons.firing [1].nStop = 0;
	}
else if (gameData.weapons.firing [1].nDuration) {
	gameData.weapons.firing [1].nStop = gameStates.app.nSDLTicks [0];
	gameData.weapons.firing [1].nDuration = 
	gameData.weapons.firing [1].nStart = 0;
	}
}

//-------------------------------------------------------------------------

void UpdatePlayerWeaponInfo (void)
{
	CWeaponState*	wsP = gameData.multiplayer.weaponStates + N_LOCALPLAYER;
	tFiringData*	fP;

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
	}
if (wsP->nSecondary != gameData.weapons.nSecondary) {
	wsP->nSecondary = gameData.weapons.nSecondary;
	}
if (wsP->bQuadLasers != ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) != 0)) {
	wsP->bQuadLasers = ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) != 0);
	}
fP = wsP->firing;
for (int32_t i = 0; i < 2; i++, fP++) {
	if (fP->nStart != gameData.weapons.firing [i].nStart) {
		fP->nStart = gameData.weapons.firing [i].nStart;
		}
	if (fP->nDuration != gameData.weapons.firing [i].nDuration) {
		fP->nDuration = gameData.weapons.firing [i].nDuration;
		}
	if (fP->nStop != gameData.weapons.firing [i].nStop) {
		fP->nStop = gameData.weapons.firing [i].nStop;
		}
	if (gameData.weapons.firing [i].bSound == 1) {
		fP->bSound = 1;
		gameData.weapons.firing [i].bSound = 0;
		}
	if (fP->bSpeedUp != EGI_FLAG (bGatlingSpeedUp, 1, 0, 0)) {
		fP->bSpeedUp = EGI_FLAG (bGatlingSpeedUp, 1, 0, 0);
		}
	}
if (wsP->nMissiles != LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]) {
	wsP->nMissiles = (char) LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
	}
if (wsP->nLaserLevel != LOCALPLAYER.LaserLevel ()) {
	wsP->nLaserLevel = LOCALPLAYER.LaserLevel ();
	}
if (wsP->bTripleFusion != gameData.weapons.bTripleFusion) {
	wsP->bTripleFusion = gameData.weapons.bTripleFusion;
	}
if (wsP->nMslLaunchPos != (gameData.laser.nMissileGun & 3)) {
	wsP->nMslLaunchPos = gameData.laser.nMissileGun & 3;
	}
if (wsP->xMslFireTime != gameData.missiles.xNextFireTime) {
	wsP->xMslFireTime = gameData.missiles.xNextFireTime;
	}
UpdateFiringSounds ();
}

//------------------------------------------------------------------------------

void UpdatePlayerEffects (void)
{
for (int32_t nPlayer = 0; nPlayer < N_PLAYERS; nPlayer++) {
	if (gameData.multiplayer.tAppearing [nPlayer][0] < 0) {
		gameData.multiplayer.tAppearing [nPlayer][0] += gameData.time.xFrame;
		if (gameData.multiplayer.tAppearing [nPlayer][0] >= 0) {
			gameData.multiplayer.tAppearing [nPlayer][0] = 1;
			PLAYEROBJECT (nPlayer).CreateAppearanceEffect ();
			}
		}
	else if (gameData.multiplayer.tAppearing [nPlayer][0] > 0) {
		gameData.multiplayer.tAppearing [nPlayer][0] -= gameData.time.xFrame;
		if (gameData.multiplayer.tAppearing [nPlayer][0] <= 0) {
			if (nPlayer == N_LOCALPLAYER) {
				SetChaseCam (0);
				}
			gameData.multiplayer.tAppearing [nPlayer][0] = 0;
			gameData.multiplayer.bTeleport [nPlayer] = 0;
			}
		}
	}
}

//------------------------------------------------------------------------------

int32_t CountPlayerObjects (int32_t nPlayer, int32_t nType, int32_t nId)
{
	int32_t	h = 0;
	CObject*	objP;

FORALL_OBJS (objP) 
	if ((objP->info.nType == nType) && (objP->info.nId == nId) &&
		 (objP->cType.laserInfo.parent.nType == OBJ_PLAYER) &&
		 (OBJECTS [objP->cType.laserInfo.parent.nObject].info.nId == nPlayer))
	h++;
return h;
}

//------------------------------------------------------------------------------

static bool PlayerInSegment (int16_t nSegment)
{
if (nSegment < 0)
	return true;
for (int32_t i = 0; i < N_PLAYERS; i++) 
	if ((i != N_LOCALPLAYER) && (PLAYEROBJECT (i).Segment () == nSegment))
		return true;
return false;
}

//------------------------------------------------------------------------------

CFixVector *VmRandomVector (CFixVector *vRand); // from lightning.cpp

void MovePlayerToSpawnPos (int32_t nSpawnPos, CObject *objP)
{
	CObject	*markerP = markerManager.SpawnObject (-1);

if (markerP) {
	objP->info.position = markerP->info.position;
 	objP->RelinkToSeg (markerP->info.nSegment);
	}
else {
	int16_t nSegment = gameData.multiplayer.playerInit [nSpawnPos].nSegment;
	if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments))
		GameStartInitNetworkPlayers ();
	objP->info.position = gameData.multiplayer.playerInit [nSpawnPos].position;

	nSegment = gameData.multiplayer.playerInit [nSpawnPos].nSegment;

	// If the chosen spawn segment is occupied by another player,
	// try to place this player in an unoccupied adjacent segment
	if (PlayerInSegment (nSegment)) {
		CSegment* segP = SEGMENTS + nSegment;
		for (int16_t nSide = 0; nSide < 6; nSide++) {
			nSegment = segP->ChildId (nSide);
			if (!PlayerInSegment (nSegment)) {
				objP->info.position.vPos = SEGMENTS [nSegment].Center ();
			 	objP->RelinkToSeg (nSegment);
				nSegment = -1;
				break;
				}
			}
		}
	if (nSegment >= 0) {
	 	objP->RelinkToSeg (nSegment);
		// If the chosen spawn segment and all adjacent sements are occupied by another player,
		// chose a random spawn position in the segment 
		if (PlayerInSegment (nSegment)) {
			CFixVector v;
			fix r = SEGMENTS [nSegment].MinRad () / 2;
			objP->info.position.vPos = SEGMENTS [nSegment].Center () + *VmRandomVector (&v) * (r + fix (r * RandFloat (2.0f)));
			}
		}
	}
}

//------------------------------------------------------------------------------

CFixVector* PlayerSpawnPos (int32_t nPlayer)
{
	CObject	*markerP = markerManager.SpawnObject (nPlayer);

return markerP ? &markerP->info.position.vPos : &gameData.multiplayer.playerInit [nPlayer].position.vPos;
}

//------------------------------------------------------------------------------

CFixMatrix *PlayerSpawnOrient (int32_t nPlayer)
{
	CObject	*markerP = markerManager.SpawnObject (nPlayer);

return markerP ? &markerP->info.position.mOrient : &gameData.multiplayer.playerInit [nPlayer].position.mOrient;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

int32_t CPlayerData::Index (void)
{
int32_t i = int32_t (this - gameData.multiplayer.players);
return ((i < 0) || (i > MAX_PLAYERS)) ? 0 : i;
}

//-------------------------------------------------------------------------

bool CPlayerData::IsLocalPlayer (void)
{
return Index () == N_LOCALPLAYER;
}

//-------------------------------------------------------------------------

void CPlayerData::UpdateDeathTime (void) 
{
if (m_shield.Get () >= 0) {
	m_tDeath = 0;
	m_bExploded = 0;
	}
else if (!m_tDeath)
	m_tDeath = gameStates.app.nSDLTicks [0];
}

//-------------------------------------------------------------------------

fix CPlayerData::SetShield (fix s, bool bScale) 
{ 
if (m_shield.Set (s, bScale)) {
	if (OBJECTS.Buffer () && (nObject >= 0) && (IsLocalPlayer () || (nObject != LOCALPLAYER.nObject)))
		OBJECTS [nObject].SetShield (s); 
	if (IsLocalPlayer ())
		NetworkFlushData (); // will send position, shield and weapon info
	}
UpdateDeathTime ();
return shield;
}

//-------------------------------------------------------------------------

fix CPlayerData::SetEnergy (fix e, bool bScale) 
{ 
m_energy.Set (e, bScale);
return energy;
}

//-------------------------------------------------------------------------

void CPlayerData::SetObject (int16_t n)
{
nObject = n;
}

//-------------------------------------------------------------------------

CObject* CPlayerData::Object (void)
{
return (nObject < 0) ? NULL : OBJECTS + nObject;
}

//-------------------------------------------------------------------------

bool CPlayerData::WaitingForExplosion (void) 
{ 
return m_tDeath && (gameStates.app.nSDLTicks [0] - m_tDeath < 30000) && !m_bExploded && IsConnected ();  
}

//-------------------------------------------------------------------------

bool CPlayerData::WaitingForWeaponInfo (void) 
{ 
return !m_tWeaponInfo || ((gameStates.app.nSDLTicks [0] - m_tWeaponInfo > 15000) && (gameStates.app.nSDLTicks [0] - m_tWeaponInfo < 180000) && !m_bExploded && IsConnected ());
}

//-------------------------------------------------------------------------

void CPlayerData::Connect (int8_t nStatus) 
{
connected = nStatus;
if ((nStatus == CONNECT_PLAYING) && (m_nLevel == missionManager.nCurrentLevel))
	m_tDisconnect = 0;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

float CShipEnergy::Scale (void)
{
	uint8_t nShip = gameData.multiplayer.weaponStates [m_index].nShip;

return (nShip < MAX_SHIP_TYPES) ? shipModifiers [nShip].a [m_type] : 1.0f;
}

//-------------------------------------------------------------------------

time_t CShipEnergy::m_nRechargeDelays [RECHARGE_DELAY_COUNT] = {0, 1, 2, 3, 4, 5, 10, 15, 30, 60};

//-------------------------------------------------------------------------

int32_t CShipEnergy::RechargeDelayCount (void) 
{
return sizeofa (m_nRechargeDelays);
}

//-------------------------------------------------------------------------

time_t CShipEnergy::RechargeDelay (uint8_t i)
{
return m_nRechargeDelays [Clamp (i, (uint8_t) i, (uint8_t) RechargeDelayCount)];
}

//-------------------------------------------------------------------------

bool CShipEnergy::Set (fix e, bool bScale) 
{
if (bScale)
	e = (fix) FRound (e * Scale ());
if (e > Max ())
	e = Max ();
if (!m_current || (*m_current == e))
	return false;
if (*m_current > e) {
	m_toRecharge [0].Setup (RechargeDelay (extraGameInfo [IsMultiGame].nRechargeDelay));
	m_toRecharge [0].Start ();
	}
*m_current = e;
return true;
}

//-------------------------------------------------------------------------

void CShipEnergy::Recharge (void) 
{
if (!EGI_FLAG (bRechargeEnergy, false, true, false))
	return;
if (!m_current)
	return;
if (*m_current >= m_max)
	return;
if (!m_toRecharge [0].Expired (false))
	return;
m_toRecharge [0].Setup (0);
if (!m_toRecharge [1].Expired (false))
	return;
m_toRecharge [1].Setup (25);
m_toRecharge [1].Start ();
*m_current += I2X (1) / 250;
if (*m_current > m_max)
	*m_current = m_max;
}

//-------------------------------------------------------------------------

bool CShipEnergy::Set (fix e, bool bScale = true) 
{
if (bScale)
	e = (fix) FRound (e * Scale ());
if (e > Max ())
	e = Max ();
if (!m_current || (*m_current == e))
	return false;
if (*m_current > e) {
	m_toRecharge [0].Setup (extraGameInfo [IsMultiGame].nRechargeDelay);
	m_toRecharge [0].Start ();
	}
*m_current = e;
return true;
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
