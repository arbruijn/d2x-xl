#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "text.h"
#include "u_mem.h"
#include "timer.h"
#include "findpath.h"
#include "segmath.h"
#include "network.h"
#include "network_lib.h"
#include "loadobjects.h"
#include "objeffects.h"
#include "fireball.h"
#include "dropobject.h"
#include "headlight.h"
#include "thrusterflames.h"

#if DBG
#	define	BASE_NET_DROP_DEPTH	2
#else
#	define	BASE_NET_DROP_DEPTH	8
#endif

//------------------------------------------------------------------------------

int32_t InitObjectCount (CObject *objP)
{
	int32_t	nFree, nTotal, i, j, bFree;
	int16_t	nType = objP->info.nType;
	int16_t	id = objP->info.nId;

nFree = nTotal = 0;
for (i = 0, objP = gameData.objData.init.Buffer (); i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->info.nType != nType) || (objP->info.nId != id))
		continue;
	nTotal++;
	for (bFree = 1, j = SEGMENT (objP->info.nSegment)->m_objects; j != -1; j = OBJECT (j)->info.nNextInSeg)
		if ((OBJECT (j)->info.nType == nType) && (OBJECT (j)->info.nId == id)) {
			bFree = 0;
			break;
			}
	if (bFree)
		nFree++;
	}
return nFree ? -nFree : nTotal;
}

//------------------------------------------------------------------------------

CObject *FindInitObject (CObject *objP)
{
	int32_t	h, i, j, bUsed,
			bUseFree,
			objCount = InitObjectCount (objP);
	int16_t	nType = objP->info.nType;
	int16_t	id = objP->info.nId;

// due to OBJECTS being deleted from the CObject list when picked up and recreated when dropped,
// cannot determine exact respawn CSegment, so randomly chose one from all segments where powerups
// of this nType had initially been placed in the level.
if (!objCount)		//no OBJECTS of this nType had initially been placed in the mine.
	return NULL;	//can happen with missile packs
if ((bUseFree = (objCount < 0)))
	objCount = -objCount;
h = Rand (objCount) + 1;
for (i = 0, objP = gameData.objData.init.Buffer (); i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->info.nType != nType) || (objP->info.nId != id))
		continue;
	// if the current CSegment does not contain a powerup of the nType being looked for,
	// return that CSegment
	if (bUseFree) {
		for (bUsed = 0, j = SEGMENT (objP->info.nSegment)->m_objects; j != -1; j = OBJECT (j)->info.nNextInSeg)
			if ((OBJECT (j)->info.nType == nType) && (OBJECT (j)->info.nId == id)) {
				bUsed = 1;
				break;
				}
		if (bUsed)
			continue;
		}
	if (!--h)
		return objP;
	}
return NULL;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if there is a door here and it is openable
//	It is assumed that the player has all keys.
int32_t PlayerCanOpenDoor (CSegment *segP, int16_t nSide)
{
CWall* wallP = segP->Wall (nSide);
if (!wallP)
	return 0;						//	no CWall here.
int16_t wallType = wallP->nType;
//	Can't open locked doors.
if (( (wallType == WALL_DOOR) && (wallP->flags & WALL_DOOR_LOCKED)) || (wallType == WALL_CLOSED))
	return 0;
return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return a CSegment %i segments away from initial CSegment.
//	Returns -1 if can't find a CSegment that distance away.

// --------------------------------------------------------------------------------------------------------------------

static int32_t	segQueue [MAX_SEGMENTS_D2X];

int32_t PickConnectedSegment (CObject *objP, int32_t nMaxDepth, int32_t *nDepthP)
{
	int32_t		nCurDepth;
	int32_t		nStartSeg;
	int32_t		nHead, nTail;
	int16_t		i, j, nSideCount, nSide, nChild, sideList [6];
	CSegment*	segP;
	CWall*		wallP;
	uint8_t		bVisited [MAX_SEGMENTS_D2X];

if (!objP)
	return -1;
nStartSeg = OBJSEG (objP);
nHead =
nTail = 0;
segQueue [nHead++] = nStartSeg;

memset (bVisited, 0, gameData.segData.nSegments);
bVisited [nStartSeg] = 1;

while (nTail != nHead) {
	nCurDepth = bVisited [segQueue [nTail]];
	if (nCurDepth >= nMaxDepth) {
		if (nDepthP)
			*nDepthP = nCurDepth;
		return segQueue [nTail + Rand (nHead - nTail)];
		}
	segP = SEGMENT (segQueue [nTail++]);

	//	select sides randomly
	for (i = 0, nSideCount = 0; i < SEGMENT_SIDE_COUNT; i++)
		if (segP->Side (i)->FaceCount ())
			sideList [nSideCount++] = i;
	for (i = nSideCount; i; ) {
		j = Rand (i);
		nSide = sideList [j];
		if (j < --i)
			sideList [j] = sideList [i];
		if (0 > (nChild = segP->m_children [nSide]))
			continue;
		if (bVisited [nChild])
			continue;
		wallP = segP->Wall (nSide);
		if (wallP && !PlayerCanOpenDoor (segP, nSide))
			continue;
		segQueue [nHead++] = segP->m_children [nSide];
		bVisited [nChild] = nCurDepth + 1;
		}
	}
#if TRACE
console.printf (CON_DBG, "...failed at depth %i, returning -1\n", nCurDepth);
#endif
while ((nTail > 0) && (bVisited [segQueue [nTail - 1]] == nCurDepth))
	nTail--;
if (nDepthP)
	*nDepthP = nCurDepth + 1;
return segQueue [nTail + Rand (nHead - nTail)];
}

//	------------------------------------------------------------------------------------------------------
//	Choose CSegment to drop a powerup in.
//	For all active net players, try to create a N CSegment path from the player.  If possible, return that
//	CSegment.  If not possible, try another player.  After a few tries, use a Random CSegment.
//	Don't drop if control center in CSegment.
int32_t ChooseDropSegment (CObject *objP, int32_t *pbFixedPos, int32_t nDropState)
{
	int32_t			nPlayer = 0;
	int16_t			nSegment = -1;
	int32_t			nDepth, nDropDepth;
	int32_t			count;
	int16_t			nPlayerSeg;
	CFixVector	tempv, *vPlayerPos;
	fix			nDist = 0;
	int32_t			bUseInitSgm =
						objP &&
						(EGI_FLAG (bFixedRespawns, 0, 0, 0) ||
						 (EGI_FLAG (bEnhancedCTF, 0, 0, 0) &&
						 (objP->info.nType == OBJ_POWERUP) && ((objP->info.nId == POW_BLUEFLAG) || (objP->info.nId == POW_REDFLAG))));
#if TRACE
console.printf (CON_DBG, "ChooseDropSegment:");
#endif
if (bUseInitSgm) {
	CObject *initObjP = FindInitObject (objP);
	if (initObjP) {
		*pbFixedPos = 1;
		objP->info.position.vPos = initObjP->info.position.vPos;
		objP->info.position.mOrient = initObjP->info.position.mOrient;
		return initObjP->info.nSegment;
		}
	}
if (pbFixedPos)
	*pbFixedPos = 0;
nDepth = BASE_NET_DROP_DEPTH + Rand (BASE_NET_DROP_DEPTH * 2);
vPlayerPos = &LOCALOBJECT->info.position.vPos;
nPlayerSeg = LOCALOBJECT->info.nSegment;

if (gameStates.multi.nGameType != UDP_GAME)
	gameStates.app.SRand ();
while (nSegment == -1) {
	if (!IsMultiGame)
		nPlayer = N_LOCALPLAYER;
	else {	// chose drop segment at required minimum distance from some random player
		nPlayer = Rand (N_PLAYERS);
		count = 0;
		while ((count < N_PLAYERS) &&	// make sure player is not the local player or on his team
				 (!PLAYER (nPlayer).connected ||
				  (nPlayer == N_LOCALPLAYER) ||
				  ((gameData.app.GameMode (GM_TEAM|GM_CAPTURE|GM_ENTROPY)) && (GetTeam (nPlayer) == GetTeam (N_LOCALPLAYER))))) {
			nPlayer = (nPlayer + 1) % N_PLAYERS;
			count++;
			}
		if (count == N_PLAYERS)
			nPlayer = N_LOCALPLAYER;
		}
	nSegment = PickConnectedSegment (OBJECT (PLAYER (nPlayer).nObject), nDepth, &nDropDepth);
#if 1
	if (nDropDepth < BASE_NET_DROP_DEPTH / 2)
		return -1;
#endif
#if TRACE
	console.printf (CON_DBG, " %d", nSegment);
#endif
	if (nSegment == -1) {
		nDepth--;
		continue;
		}

	CSegment* segP = SEGMENT (nSegment);
	int32_t nSegFunc = segP->m_function;
	if (segP->HasBlockedProp () ||
		 (nSegFunc == SEGMENT_FUNC_REACTOR) ||
		 (nSegFunc == SEGMENT_FUNC_GOAL_BLUE) ||
		 (nSegFunc == SEGMENT_FUNC_GOAL_RED) ||
		 (nSegFunc == SEGMENT_FUNC_TEAM_BLUE) ||
		 (nSegFunc == SEGMENT_FUNC_TEAM_RED))
		nSegment = -1;
	else {	//don't drop in any children of control centers
		for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
			int32_t nChild = segP->m_children [i];
			if (IS_CHILD (nChild) && (SEGMENT (nChild)->m_function == SEGMENT_FUNC_REACTOR)) {
				nSegment = -1;
				break;
				}
			}
		}
	//bail if not far enough from original position
	if (nSegment > -1) {
		tempv = segP->Center ();
		nDist = simpleRouter [0].PathLength (*vPlayerPos, nPlayerSeg, tempv, nSegment, -1, WID_PASSABLE_FLAG, -1);
		if ((nDist < 0) || (nDist >= I2X (20) * nDepth))
			break;
		}
	nDepth--;
	}
#if TRACE
if (nSegment != -1)
	console.printf (CON_DBG, " dist=%x\n", nDist);
#endif
if (nSegment == -1) {
#if TRACE
	console.printf (1, "Warning: Unable to find a connected CSegment.  Picking a random one.\n");
#endif
	return Rand (gameData.segData.nSegments);
	}
return nSegment;
}

//	------------------------------------------------------------------------------------------------------

void DropPowerups (void)
{
if (extraGameInfo [IsMultiGame].nSpawnDelay != 0) {
	int16_t	h = gameData.objData.nFirstDropped, i;
	while (h >= 0) {
		i = h;
		h = gameData.objData.dropInfo [i].nNextPowerup;
		if ((gameData.objData.dropInfo [i].nDropTime != 0x7FFFFFFF) && !MaybeDropNetPowerup (i, gameData.objData.dropInfo [i].nPowerupType, CHECK_DROP))
			break;
		}
	}
}

//	------------------------------------------------------------------------------------------------------

void RespawnDestroyedWeapon (int16_t nObject)
{
	int32_t	h = gameData.objData.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objData.dropInfo [i].nNextPowerup;
	if ((gameData.objData.dropInfo [i].nObject == nObject) &&
		 (gameData.objData.dropInfo [i].nDropTime < 0)) {
		gameData.objData.dropInfo [i].nDropTime = 0;
		MaybeDropNetPowerup (i, gameData.objData.dropInfo [i].nPowerupType, CHECK_DROP);
		}
	}
}

//	------------------------------------------------------------------------------------------------------

int32_t AddDropInfo (int16_t nObject, int16_t nPowerupType, int32_t nDropTime)
{
	int32_t	h;

if (gameData.objData.nFreeDropped < 0)
	return -1;
//AddPowerupInMine (nPowerupType);
h = gameData.objData.nFreeDropped;
gameData.objData.nFreeDropped = gameData.objData.dropInfo [h].nNextPowerup;
gameData.objData.dropInfo [h].nPrevPowerup = gameData.objData.nLastDropped;
gameData.objData.dropInfo [h].nNextPowerup = -1;
gameData.objData.dropInfo [h].nObject = nObject;
gameData.objData.dropInfo [h].nSignature = OBJECT (nObject)->Signature ();
gameData.objData.dropInfo [h].nPowerupType = nPowerupType;
gameData.objData.dropInfo [h].nDropTime = (nDropTime > 0) ? nDropTime : (extraGameInfo [IsMultiGame].nSpawnDelay <= 0) ? -1 : gameStates.app.nSDLTicks [0];
if (gameData.objData.nFirstDropped >= 0)
	gameData.objData.dropInfo [gameData.objData.nLastDropped].nNextPowerup = h;
else
	gameData.objData.nFirstDropped = h;
gameData.objData.nLastDropped = h;
gameData.objData.nDropped++;
return h;
}

//	------------------------------------------------------------------------------------------------------

void DelDropInfo (int32_t h)
{
	int32_t	i, j;

if (h < 0)
	return;
RemovePowerupInMine (gameData.objData.dropInfo [h].nPowerupType);
i = gameData.objData.dropInfo [h].nPrevPowerup;
j = gameData.objData.dropInfo [h].nNextPowerup;
if (i < 0)
	gameData.objData.nFirstDropped = j;
else
	gameData.objData.dropInfo [i].nNextPowerup = j;
if (j < 0)
	gameData.objData.nLastDropped = i;
else
	gameData.objData.dropInfo [j].nPrevPowerup = i;
gameData.objData.dropInfo [h].nNextPowerup = gameData.objData.nFreeDropped;
gameData.objData.nFreeDropped = h;
gameData.objData.nDropped--;
}

//	------------------------------------------------------------------------------------------------------

int32_t FindDropInfo (int32_t nSignature)
{
	int16_t	i = gameData.objData.nFirstDropped;

while (i >= 0) {
	if (gameData.objData.dropInfo [i].nSignature == nSignature)
		return i;
	i = gameData.objData.dropInfo [i].nNextPowerup;
	}
return -1;
}

//	------------------------------------------------------------------------------------------------------
//	Drop cloak powerup if in a network game.
// nObject will contain a drop list index if MaybeDropNetPowerup is called with state CHECK_DROP

int32_t MaybeDropNetPowerup (int16_t nObject, int32_t nPowerupType, int32_t nDropState)
{
if (EGI_FLAG (bImmortalPowerups, 0, 0, 0) || (IsMultiGame && !IsCoopGame)) {
#if 0
	if (IsNetworkGame && (nDropState < CHECK_DROP) && (nPowerupType >= 0)) {
		if (gameData.multiplayer.powerupsInMine [nPowerupType] >= gameData.multiplayer.maxPowerupsAllowed [nPowerupType])
			return 0;
		}
#endif
	if (gameData.reactor.bDestroyed || gameStates.app.bEndLevelSequence)
		return 0;
	gameData.multigame.create.nCount = 0;
	
	if (IsMultiGame && (extraGameInfo [IsMultiGame].nSpawnDelay != 0)) {
		if (nDropState == CHECK_DROP) {
			if ((gameData.objData.dropInfo [nObject].nDropTime < 0) ||
				 (gameStates.app.nSDLTicks [0] - gameData.objData.dropInfo [nObject].nDropTime < extraGameInfo [IsMultiGame].nSpawnDelay))
				return 0;
			nDropState = EXEC_DROP;
			}
		else if (nDropState == INIT_DROP) {
			AddDropInfo (nObject, (int16_t) nPowerupType);
			return 0;
			}
		if (nDropState == EXEC_DROP) {
			DelDropInfo (nObject);
			}
		}
	else {
		if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME) && (nDropState == INIT_DROP) && OBJECT (nObject)->IsMissile ()) {
			//if (!(MultiPowerupIs4Pack (nPowerupType + 1) && MissingPowerups (nPowerupType + 1))) 
				AddDropInfo (nObject, nPowerupType, 0x7FFFFFFF); // respawn missiles only after their destruction
			return 0;
			}
		}

	if (0 > (nObject = PrepareObjectCreateEgg (OBJECT (LOCALPLAYER.nObject), 1, OBJ_POWERUP, nPowerupType, true)))
		return 0;

	CObject* objP = OBJECT (nObject);
	int32_t bFixedPos = 0;
	int16_t nSegment = ChooseDropSegment (OBJECT (nObject), &bFixedPos, nDropState);
	if (0 > nSegment) {
		objP->Die ();
		return 0;
		}
	objP->mType.physInfo.velocity.SetZero ();

	CFixVector vNewPos;
	if (bFixedPos)
		vNewPos = objP->info.position.vPos;
	else {
		CFixVector vOffset = SEGMENT (nSegment)->Center () - vNewPos;
		CFixVector::Normalize (vOffset);
		vNewPos = SEGMENT (nSegment)->RandomPoint ();
		vNewPos += vOffset * objP->info.xSize;
		}
	nSegment = FindSegByPos (vNewPos, nSegment, 1, 0);
	MultiSendCreatePowerup (nPowerupType, nSegment, nObject, &vNewPos);
	if (!bFixedPos)
		objP->info.position.vPos = vNewPos;
	objP->RelinkToSeg (nSegment);
	CreateExplosion (objP, nSegment, vNewPos, vNewPos, I2X (5), ANIM_POWERUP_DISAPPEARANCE);
	return 1;
	}
return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Return true if current CSegment contains some CObject.
int32_t SegmentContainsObject (int32_t objType, int32_t obj_id, int32_t nSegment)
{
	int32_t	nObject;

if (nSegment == -1)
	return 0;
nObject = SEGMENT (nSegment)->m_objects;
while (nObject != -1)
	if ((OBJECT (nObject)->info.nType == objType) && (OBJECT (nObject)->info.nId == obj_id))
		return 1;
	else
		nObject = OBJECT (nObject)->info.nNextInSeg;
return 0;
}

//	------------------------------------------------------------------------------------------------------

int32_t ObjectNearbyAux (int32_t nSegment, int32_t objectType, int32_t object_id, int32_t depth)
{
if (depth == 0)
	return 0;
if (SegmentContainsObject (objectType, object_id, nSegment))
	return 1;
CSegment* segP = SEGMENT (nSegment);
for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	int16_t nChildSeg = segP->m_children [i];
	if ((nChildSeg != -1) && ObjectNearbyAux (nChildSeg, objectType, object_id, depth-1))
		return 1;
	}
return 0;
}


//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
int32_t WeaponNearby (CObject *objP, int32_t weapon_id)
{
return ObjectNearbyAux (objP->info.nSegment, OBJ_POWERUP, weapon_id, 3);
}

//	------------------------------------------------------------------------------------------------------

void MaybeReplacePowerupWithEnergy (CObject *delObjP)
{
	int32_t	nWeapon = -1;

if (delObjP->info.contains.nType != OBJ_POWERUP)
	return;

if (delObjP->info.contains.nId == POW_CLOAK) {
	if (WeaponNearby (delObjP, delObjP->info.contains.nId)) {
#if TRACE
		console.printf (CON_DBG, "Bashing cloak into nothing because there's one nearby.\n");
#endif
		delObjP->info.contains.nCount = 0;
	}
	return;
}
switch (delObjP->info.contains.nId) {
	case POW_VULCAN:
		nWeapon = VULCAN_INDEX;
		break;
	case POW_GAUSS:
		nWeapon = GAUSS_INDEX;
		break;
	case POW_SPREADFIRE:
		nWeapon = SPREADFIRE_INDEX;
		break;
	case POW_PLASMA:
		nWeapon = PLASMA_INDEX;
		break;
	case POW_FUSION:
		nWeapon = FUSION_INDEX;
		break;
	case POW_HELIX:
		nWeapon = HELIX_INDEX;
		break;
	case POW_PHOENIX:
		nWeapon = PHOENIX_INDEX;
		break;
	case POW_OMEGA:
		nWeapon = OMEGA_INDEX;
		break;
	}

//	Don't drop vulcan ammo if player maxed out.
if (( (nWeapon == VULCAN_INDEX) || (delObjP->info.contains.nId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
	delObjP->info.contains.nCount = 0;
else if (( (nWeapon == GAUSS_INDEX) || (delObjP->info.contains.nId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
	delObjP->info.contains.nCount = 0;
else if (nWeapon != -1) {
	if ((PlayerHasWeapon (nWeapon, 0, -1, 1) & HAS_WEAPON_FLAG) || WeaponNearby (delObjP, delObjP->info.contains.nId)) {
		if (RandShort () > 16384) {
			delObjP->info.contains.nType = OBJ_POWERUP;
			if (nWeapon == VULCAN_INDEX) {
				delObjP->info.contains.nId = POW_VULCAN_AMMO;
				}
			else if (nWeapon == GAUSS_INDEX) {
				delObjP->info.contains.nId = POW_VULCAN_AMMO;
				}
			else {
				delObjP->info.contains.nId = POW_ENERGY;
				}
			}
		else {
			delObjP->info.contains.nType = OBJ_POWERUP;
			delObjP->info.contains.nId = POW_SHIELD_BOOST;
			}
		}
	}
else if (delObjP->info.contains.nId == POW_QUADLASER)
	if ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) || WeaponNearby (delObjP, delObjP->info.contains.nId)) {
		if (RandShort () > 16384) {
			delObjP->info.contains.nType = OBJ_POWERUP;
			delObjP->info.contains.nId = POW_ENERGY;
			}
		else {
			delObjP->info.contains.nType = OBJ_POWERUP;
			delObjP->info.contains.nId = POW_SHIELD_BOOST;
			}
		}

//	If this robot was gated in by the boss and it now contains energy, make it contain nothing,
//	else the room gets full of energy.
if ((delObjP->info.nCreator == BOSS_GATE_PRODUCER_NUM) && (delObjP->info.contains.nId == POW_ENERGY) && (delObjP->info.contains.nType == OBJ_POWERUP)) {
#if TRACE
	console.printf (CON_DBG, "Converting energy powerup to nothing because robot %i gated in by boss.\n", OBJ_IDX (delObjP));
#endif
	delObjP->info.contains.nCount = 0;
	}

// Change multiplayer extra-lives into invulnerability
if (IsMultiGame && (delObjP->info.contains.nId == POW_EXTRA_LIFE))
	delObjP->info.contains.nId = POW_INVUL;
}

//------------------------------------------------------------------------------

int32_t DropPowerup (uint8_t nType, uint8_t nId, int16_t owner, int32_t bDropExtras, const CFixVector& vInitVel, const CFixVector& vPos, int16_t nSegment, bool bLocal)
{
	int16_t		nObject = -1;
	CObject*		objP;
	CFixVector	vNewVel, vNewPos;
	fix			xOldMag, xNewMag;

switch (nType) {
	case OBJ_POWERUP:
		if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
			return -1;
		if (nId >= MAX_WEAPON_ID)
			return -1;

		int32_t	nRandScale, nOffset;
		vNewVel = vInitVel;
		xOldMag = vInitVel.Mag();

		//	We want powerups to move more in network mode.
		if (IsMultiGame && !gameData.app.GameMode (GM_MULTI_ROBOTS)) {
			nRandScale = 4;
			//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
			if (nId == POW_EXTRA_LIFE)
				nId = POW_INVUL;
			}
		else
			nRandScale = 2;
		xNewMag = xOldMag + I2X (32);
		nOffset = 16384 * nRandScale;
		vNewVel.v.coord.x += FixMul (xNewMag, RandShort () * nRandScale - nOffset);
		vNewVel.v.coord.y += FixMul (xNewMag, RandShort () * nRandScale - nOffset);
		vNewVel.v.coord.z += FixMul (xNewMag, RandShort () * nRandScale - nOffset);
		// Give keys zero velocity so they can be tracked better in multi
		if (IsMultiGame && (((nId >= POW_KEY_BLUE) && (nId <= POW_KEY_GOLD)) || (nId == POW_MONSTERBALL)))
			vNewVel.SetZero ();
		vNewPos = vPos;

		if (IsMultiGame) {
			if (gameData.multigame.create.nCount >= MAX_NET_CREATE_OBJECTS) {
				return -1;
				}
			if (IsNetworkGame && networkData.nStatus == NETSTAT_ENDLEVEL)
				return -1;
			}
		nObject = CreatePowerup (nId, owner, nSegment, vNewPos, 0);
		objP = OBJECT (nObject);
		if (!objP) 
			return -1;
		if (IsMultiGame) {
#if 0
			if ((gameStates.multi.nGameType == UDP_GAME) && !bLocal)
				MultiSendDropPowerup (nId, nSegment, nObject, &vNewPos, &vNewVel);
#endif
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
			}
		objP->mType.physInfo.velocity = vNewVel;
		objP->mType.physInfo.drag = 512;	//1024;
		objP->mType.physInfo.mass = I2X (1);
		objP->mType.physInfo.flags = PF_BOUNCES;
		objP->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [objP->info.nId].nClipIndex;
		objP->rType.animationInfo.xFrameTime = gameData.effects.animations [0][objP->rType.animationInfo.nClipIndex].xFrameTime;
		objP->rType.animationInfo.nCurFrame = 0;

		switch (objP->info.nId) {
			case POW_CONCUSSION_1:
			case POW_CONCUSSION_4:
			case POW_SHIELD_BOOST:
			case POW_ENERGY:
				objP->SetLife ((RandShort () + I2X (3)) * 64);		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
				if (IsMultiGame)
					objP->SetLife (objP->LifeLeft () / 2);
				break;
			default:
//						if (IsMultiGame)
//							objP->SetLife ((RandShort () + I2X (3)) * 64);		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
				break;
			}
		break;

	case OBJ_ROBOT:
		vNewVel = vInitVel;
		xOldMag = vInitVel.Mag();
		CFixVector::Normalize (vNewVel);
		vNewVel.v.coord.x += SRandShort () * 2;
		vNewVel.v.coord.y += SRandShort () * 2;
		vNewVel.v.coord.z += SRandShort () * 2;
		CFixVector::Normalize (vNewVel);
		vNewVel *= ((I2X (32) + xOldMag) * 2);
		vNewPos = vPos;
		if (0 > (nObject = CreateRobot (nId, nSegment, vNewPos)))
			return nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		objP = OBJECT (nObject);
		//Set polygon-CObject-specific data
		objP->rType.polyObjInfo.nModel = ROBOTINFO (objP)->nModel;
		objP->rType.polyObjInfo.nSubObjFlags = 0;
		//set Physics info
		objP->mType.physInfo.velocity = vNewVel;
		objP->mType.physInfo.mass = ROBOTINFO (objP)->mass;
		objP->mType.physInfo.drag = ROBOTINFO (objP)->drag;
		objP->mType.physInfo.flags |= (PF_LEVELLING);
		objP->SetShield (ROBOTINFO (objP)->strength);
		objP->cType.aiInfo.behavior = AIB_NORMAL;
		gameData.ai.localInfo [nObject].targetAwarenessType = WEAPON_ROBOT_COLLISION;
		gameData.ai.localInfo [nObject].targetAwarenessTime = I2X (3);
		objP->cType.aiInfo.CURRENT_STATE = AIS_LOCK;
		objP->cType.aiInfo.GOAL_STATE = AIS_LOCK;
		objP->cType.aiInfo.REMOTE_OWNER = -1;
		if (ROBOTINFO (nId)->bossFlag)
			gameData.bosses.Add (nObject);

		// At JasenW's request, robots which contain robots sometimes drop shield.
		if (bDropExtras && (RandShort () > 16384)) {
			AddAllowedPowerup (POW_SHIELD_BOOST);
			DropPowerup (OBJ_POWERUP, POW_SHIELD_BOOST, -1, 0, vInitVel, vPos, nSegment);
			}
		break;

	default:
		PrintLog (0, "Warning: Illegal object type %d in function DropPowerup.\n", nType);
	}
return nObject;
}

// ----------------------------------------------------------------------------
// Returns created CObject number. If object dropped by player, set flag.

int32_t CObject::CreateEgg (bool bLocal, bool bUpdateLimits)
{
	int32_t	i, nObject = -1;

if ((info.nType != OBJ_PLAYER) && (info.contains.nType == OBJ_POWERUP)) {
	if (IsMultiGame) {
		if (bUpdateLimits)
			AddAllowedPowerup (info.contains.nId, info.contains.nCount);
		}
	else {
		if (info.contains.nId == POW_SHIELD_BOOST) {
			if (LOCALPLAYER.Shield () >= I2X (100)) {
				if (RandShort () > 16384) {
					return -1;
					}
				} 
			else if (LOCALPLAYER.Shield () >= I2X (150)) {
				if (RandShort () > 8192) {
					return -1;
					}
				}
			}
		else if (info.contains.nId == POW_ENERGY) {
			if (LOCALPLAYER.Energy () >= I2X (100)) {
				if (RandShort () > 16384) {
					return -1;
					}
				} 
			else if (LOCALPLAYER.Energy () >= I2X (150)) {
				if (RandShort () > 8192) {
					return -1;
					}
				}
			}
		}
	}

for (i = info.contains.nCount; i; i--) {
	nObject = DropPowerup (info.contains.nType, uint8_t (info.contains.nId), Index (), i == 1, // drop extra powerups?
								  mType.physInfo.velocity, info.position.vPos, info.nSegment, bLocal);
	CObject *objP = OBJECT (nObject);
	if (!objP)
		return -1;
	if (info.nType == OBJ_PLAYER) {
		if (info.nId == N_LOCALPLAYER)
			objP->info.nFlags |= OF_PLAYER_DROPPED;
		}
	else if (info.nType == OBJ_ROBOT) {
		if (info.contains.nType == OBJ_POWERUP) {
			if ((info.contains.nId == POW_VULCAN) || (info.contains.nId == POW_GAUSS))
				objP->cType.powerupInfo.nCount = VULCAN_WEAPON_AMMO_AMOUNT;
			else if (info.contains.nId == POW_OMEGA)
				objP->cType.powerupInfo.nCount = MAX_OMEGA_CHARGE;
			}
		}
	}
return nObject;
}

// -- extern int32_t Items_destroyed;

//	-------------------------------------------------------------------------------------------------------
//	Put count OBJECTS of nType nType (eg, powerup), id = id (eg, energy) into *objP, then drop them! Yippee!
//	Returns created CObject number.
int32_t PrepareObjectCreateEgg (CObject *objP, int32_t nCount, int32_t nType, int32_t nId, bool bLocal, bool bUpdateLimits)
{
if (nCount <= 0)
	return -1;
objP->info.contains.nCount = nCount;
objP->info.contains.nType = nType;
objP->info.contains.nId = nId;
return objP->CreateEgg (bLocal, bUpdateLimits);
}

//------------------------------------------------------------------------------
//creates afterburner blobs behind the specified CObject
void DropAfterburnerBlobs (CObject *objP, int32_t count, fix xSizeScale, fix xLifeTime, CObject *pParent, int32_t bThruster)
{
	int16_t			i, nSegment, nThrusters;
	CObject*			blobObjP;
	tThrusterInfo	ti;

nThrusters = thrusterFlames.CalcPos (objP, &ti, 1);
for (i = 0; i < nThrusters; i++) {
	nSegment = FindSegByPos (ti.vPos [i], objP->info.nSegment, 1, 0);
	if (nSegment == -1)
		continue;
	if (!(blobObjP = CreateExplosion (nSegment, ti.vPos [i], xSizeScale, ANIM_AFTERBURNER_BLOB)))
		continue;
	if (xLifeTime != -1) {
		blobObjP->rType.animationInfo.xTotalTime = xLifeTime;
		blobObjP->rType.animationInfo.xFrameTime = FixMulDiv (gameData.effects.animations [0][ANIM_AFTERBURNER_BLOB].xFrameTime,
																		  xLifeTime, blobObjP->info.xLifeLeft);
		blobObjP->SetLife (xLifeTime);
		}
	AddChildObjectP (pParent, blobObjP);
	blobObjP->info.renderType = RT_THRUSTER;
	if (bThruster)
		blobObjP->mType.physInfo.flags |= PF_WIGGLE;
	}
}

//	-----------------------------------------------------------------------------

int32_t MaybeDropPrimaryWeaponEgg (CObject *playerObjP, int32_t nWeapon)
{
	int32_t nWeaponFlag = HAS_FLAG (nWeapon);

if (!(PLAYER (playerObjP->info.nId).primaryWeaponFlags & nWeaponFlag))
	return -1;
if ((nWeapon == 4) && gameData.weapons.bTripleFusion)
	gameData.weapons.bTripleFusion = 0;
else if (gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].loadout.nGuns & nWeaponFlag))
	return -1;
return PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, primaryWeaponToPowerup [nWeapon]);
}

//	-----------------------------------------------------------------------------

void MaybeDropSecondaryWeaponEgg (CObject *playerObjP, int32_t nWeapon, int32_t count)
{
	int32_t nWeaponFlag = HAS_FLAG (nWeapon);
	int32_t nPowerup = secondaryWeaponToPowerup [0][nWeapon];

if (PLAYER (playerObjP->info.nId).secondaryWeaponFlags & nWeaponFlag) {
	int32_t i, maxCount = ((EGI_FLAG (bDropAllMissiles, 0, 0, 0)) ? count : min(count, 3));

	for (i = 0; i < maxCount; i++)
		PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerup);
	}
}

//	-----------------------------------------------------------------------------

void MaybeDropDeviceEgg (CPlayerInfo *playerP, CObject *playerObjP, int32_t nDeviceFlag, int32_t nPowerupId)
{
if ((PLAYER (playerObjP->info.nId).flags & nDeviceFlag) &&
	 !(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].loadout.nDevice & nDeviceFlag)))
	PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerupId);
}

//	-----------------------------------------------------------------------------

void DropMissile1or4 (CObject *playerObjP, int32_t nMissileIndex)
{
	int32_t nMissiles, nPowerupId;

if (0 < (nMissiles = PLAYER (playerObjP->info.nId).secondaryAmmo [nMissileIndex])) {
#if DBG
	if (nMissiles > 40)
		BRP;
#endif
	nPowerupId = secondaryWeaponToPowerup [0][nMissileIndex];
	if ((nMissileIndex == CONCUSSION_INDEX) && (playerObjP->Id () == N_LOCALPLAYER))
		nMissiles -= gameData.multiplayer.weaponStates [playerObjP->Id ()].nBuiltinMissiles;	//player gets 4 concs anyway when respawning, so avoid them building up
	if (nMissiles > 0) {
		if (!(IsMultiGame || EGI_FLAG (bDropAllMissiles, 0, 0, 0)) && (nMissiles > 10))
			nMissiles = 10;
		PrepareObjectCreateEgg (playerObjP, nMissiles / 4, OBJ_POWERUP, nPowerupId + 1);
		PrepareObjectCreateEgg (playerObjP, nMissiles % 4, OBJ_POWERUP, nPowerupId);
		}
	}
}

// -- int32_t	Items_destroyed = 0;

//	-----------------------------------------------------------------------------
//	If the player had mines, maybe arm up to 3 of them.

static void MaybeArmMines (CObject *playerObjP, CPlayerInfo* playerP, int32_t nType, int32_t nId)
{
if (gameStates.multi.nGameType == UDP_GAME) {
	int32_t nAmmo = playerP->secondaryAmmo [nType];
	if (nAmmo <= 0)
		return;
	if (nAmmo > 4)
		nAmmo = 4;
	for (nAmmo = Rand (nAmmo); nAmmo; nAmmo--) {
		CFixVector vRandom = CFixVector::Random ();
		CFixVector vDropPos = playerObjP->info.position.vPos + vRandom;
		int16_t nNewSeg = FindSegByPos (vDropPos, playerObjP->info.nSegment, 1, 0);
		if (nNewSeg == -1)
			return;
		int16_t nObject = CreateNewWeapon (&vRandom, &vDropPos, nNewSeg, OBJ_IDX (playerObjP), nId, 0);
		if (!OBJECT (nObject))
			return;
	#if 0
		if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME))
			MultiSendCreateWeapon (nObject);
	#endif
  		}
	}
else {
	for (int32_t nThreshold = 30000; (playerP->secondaryAmmo [nType] % 4 == 1) && (RandShort () < nThreshold); nThreshold /= 2) {
		CFixVector vRandom = CFixVector::Random ();
		nThreshold /= 2;
		CFixVector vDropPos = playerObjP->info.position.vPos + vRandom;
		int16_t nNewSeg = FindSegByPos (vDropPos, playerObjP->info.nSegment, 1, 0);
		if (nNewSeg == -1)
			return;
		int16_t nObject = CreateNewWeapon (&vRandom, &vDropPos, nNewSeg, OBJ_IDX (playerObjP), nId, 0);
		if (!OBJECT (nObject))
			return;
	#if 0
		if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME))
			MultiSendCreateWeapon (nObject);
	#endif
  		}
	}
}

//	-----------------------------------------------------------------------------

void DropPlayerEggs (CObject *playerObjP)
{
if (playerObjP && ((playerObjP->info.nType == OBJ_PLAYER) || (playerObjP->info.nType == OBJ_GHOST))) {
	int32_t			nPlayer = playerObjP->info.nId;
	int16_t			nObject;
	int32_t			nVulcanAmmo = 0;
	CPlayerData*	playerP = gameData.multiplayer.players + nPlayer;
	int32_t			bResetLasers = !IsMultiGame || (nPlayer != N_LOCALPLAYER);

	// Seed the Random number generator so in net play the eggs will always drop the same way
	PrintLog (1, "dropping player equipment\n");
	if (IsMultiGame) {
		gameData.multigame.create.nCount = 0;
		gameStates.app.SRand ((gameStates.multi.nGameType == UDP_GAME) ? gameStates.app.nRandSeed : 5483L);
		}
	MaybeArmMines (playerObjP, playerP, SMARTMINE_INDEX, SMARTMINE_ID);
	if (IsMultiGame && !(IsHoardGame || IsEntropyGame))
		MaybeArmMines (playerObjP, playerP, PROXMINE_INDEX, PROXMINE_ID);

	//	If the player dies and he has powerful lasers, create the powerups here.
	if (playerP->LaserLevel (1)) {
		if (!IsBuiltinWeapon (SUPER_LASER_INDEX)) {
			PrepareObjectCreateEgg (playerObjP, playerP->LaserLevel (1), OBJ_POWERUP, POW_SUPERLASER);
			PrepareObjectCreateEgg (playerObjP, MAX_LASER_LEVEL, OBJ_POWERUP, POW_LASER);
			if (bResetLasers)
				playerP->SetSuperLaser (0);
			}
		}
	if (playerP->LaserLevel (0) > 0) {
		if (!(IsBuiltinWeapon (LASER_INDEX) || IsBuiltinWeapon (SUPER_LASER_INDEX))) {
			PrepareObjectCreateEgg (playerObjP, playerP->LaserLevel (0), OBJ_POWERUP, POW_LASER);	// Note: laserLevel = 0 for laser level 1.
			if (bResetLasers)
				playerP->SetStandardLaser (0);
			}
		}

	//	Drop quad laser if appropos
	MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_QUAD_LASERS, POW_QUADLASER);
	MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_CLOAKED, POW_CLOAK);
	while (playerP->nInvuls--)
		PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_INVUL);
	while (playerP->nCloaks--)
		PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_CLOAK);
	MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_FULLMAP, POW_FULL_MAP);
	MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_AFTERBURNER, POW_AFTERBURNER);
	MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_AMMO_RACK, POW_AMMORACK);
	MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_CONVERTER, POW_CONVERTER);
	if (!IsMultiGame) {
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_SLOWMOTION, POW_SLOWMOTION);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_BULLETTIME, POW_BULLETTIME);
		}	
	if (PlayerHasHeadlight (nPlayer) && !(extraGameInfo [IsMultiGame].loadout.nDevice & PLAYER_FLAGS_HEADLIGHT) &&
		 !(gameStates.app.bHaveExtraGameInfo [1] && IsMultiGame && extraGameInfo [1].bDarkness))
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_HEADLIGHT, POW_HEADLIGHT);
	// drop the other enemies flag if you have it

	playerP->nInvuls =
	playerP->nCloaks = 0;
	playerP->flags &= ~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED);
	if ((gameData.app.GameMode (GM_CAPTURE)) && (playerP->flags & PLAYER_FLAGS_FLAG))
		PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, (GetTeam (nPlayer) == TEAM_RED) ? POW_BLUEFLAG : POW_REDFLAG);

#if !DBG
	if (gameData.app.GameMode (GM_HOARD | GM_ENTROPY))
#endif
	if (IsHoardGame || (IsEntropyGame && extraGameInfo [1].entropy.nVirusStability)) {
		// Drop hoard orbs

		int32_t maxCount, i;
#if TRACE
		console.printf (CON_DBG, "HOARD MODE: Dropping %d orbs \n", playerP->secondaryAmmo [PROXMINE_INDEX]);
#endif
		maxCount = playerP->secondaryAmmo [PROXMINE_INDEX];
		if (IsHoardGame && (maxCount > 12))
			maxCount = 12;
		for (i = 0; i < maxCount; i++)
			PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_HOARD_ORB);
		}

	//Drop the vulcan, gauss, and ammo
	nVulcanAmmo = playerP->primaryAmmo [VULCAN_INDEX] + gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed;
	if (!IsMultiGame || gameStates.app.bHaveExtraGameInfo [1]) {
		int32_t nGunObjs [2] = {-1, -1};
		int32_t nGunIds [2] = {VULCAN_INDEX, GAUSS_INDEX};
		int32_t nGunAmmo [2] = {VULCAN_WEAPON_AMMO_AMOUNT, GAUSS_WEAPON_AMMO_AMOUNT};
		int32_t i;

		gameData.multiplayer.weaponStates [nPlayer].nAmmoUsed = 0;
		if (0 < (i = nVulcanAmmo / VULCAN_CLIP_CAPACITY - 1)) {	// drop ammo in excess of presupplied Vulcan/Gauss ammo as vulcan ammo packs
			PrepareObjectCreateEgg (playerObjP, i, OBJ_POWERUP, POW_VULCAN_AMMO);
			nVulcanAmmo -= i * VULCAN_CLIP_CAPACITY;
			if (nVulcanAmmo < 0)
				nVulcanAmmo = 0;
			}
		for (i = 0; i < 2; i++) {
			if (IsBuiltinWeapon (nGunIds [i]))
				nVulcanAmmo -= nGunAmmo [i];
			else if (playerP->primaryWeaponFlags & HAS_FLAG (nGunIds [i]))
				nGunObjs [i] = MaybeDropPrimaryWeaponEgg (playerObjP, nGunIds [i]);
			}
		if ((nGunObjs [0] >= 0) && (nGunObjs [1] >= 0))
			nVulcanAmmo /= 2;
		for (i = 0; i < 2; i++) {
			if (nGunObjs [i] >= 0)
				OBJECT (nGunObjs [i])->cType.powerupInfo.nCount = nVulcanAmmo;
			}
		}
	//	Drop the rest of the primary weapons
	MaybeDropPrimaryWeaponEgg (playerObjP, SPREADFIRE_INDEX);
	MaybeDropPrimaryWeaponEgg (playerObjP, PLASMA_INDEX);
	if (gameData.weapons.bTripleFusion)
		MaybeDropPrimaryWeaponEgg (playerObjP, FUSION_INDEX);
	MaybeDropPrimaryWeaponEgg (playerObjP, FUSION_INDEX);
	MaybeDropPrimaryWeaponEgg (playerObjP, HELIX_INDEX);
	MaybeDropPrimaryWeaponEgg (playerObjP, PHOENIX_INDEX);
	nObject = MaybeDropPrimaryWeaponEgg (playerObjP, OMEGA_INDEX);
	if (nObject >= 0)
		OBJECT (nObject)->cType.powerupInfo.nCount =
			(playerObjP->info.nId == N_LOCALPLAYER) ? gameData.omega.xCharge [IsMultiGame] : DEFAULT_MAX_OMEGA_CHARGE;
	//	Drop the secondary weapons
	//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?
	if (!(gameData.app.GameMode (GM_HOARD | GM_ENTROPY)))
		MaybeDropSecondaryWeaponEgg (playerObjP, PROXMINE_INDEX, (playerP->secondaryAmmo [PROXMINE_INDEX])/4);
	MaybeDropSecondaryWeaponEgg (playerObjP, SMART_INDEX, playerP->secondaryAmmo [SMART_INDEX]);
	MaybeDropSecondaryWeaponEgg (playerObjP, MEGA_INDEX, playerP->secondaryAmmo [MEGA_INDEX]);
	if (!IsEntropyGame)
		MaybeDropSecondaryWeaponEgg (playerObjP, SMARTMINE_INDEX, (playerP->secondaryAmmo [SMARTMINE_INDEX])/4);
	MaybeDropSecondaryWeaponEgg (playerObjP, EARTHSHAKER_INDEX, playerP->secondaryAmmo [EARTHSHAKER_INDEX]);
	//	Drop the player's missiles in packs of 1 and/or 4
	DropMissile1or4 (playerObjP, HOMING_INDEX);
	DropMissile1or4 (playerObjP, GUIDED_INDEX);
	DropMissile1or4 (playerObjP, CONCUSSION_INDEX);
	DropMissile1or4 (playerObjP, FLASHMSL_INDEX);
	DropMissile1or4 (playerObjP, MERCURY_INDEX);

		//	Always drop a shield and energy powerup.
	if (IsMultiGame && !gameStates.app.bChangingShip) {
		PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_SHIELD_BOOST);
		PrepareObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_ENERGY);
		}
	PrintLog (-1);
	}
}

//	----------------------------------------------------------------------------
// Drop excess ammo when the ammo rack is stolen from the player

void DropExcessAmmo (void)
{
for (int32_t nWeapon = CONCUSSION_INDEX; nWeapon <= EARTHSHAKER_INDEX; nWeapon++) {
	int32_t nExcess = MaxSecondaryAmmo (nWeapon) - LOCALPLAYER.secondaryAmmo [nWeapon];
	if (nExcess > 0) {
		if (nExcess >= 4)
			DropSecondaryWeapon (nWeapon, nExcess / 4, 1);
		DropSecondaryWeapon (nWeapon, nExcess % 4, 1);
		}
	}
int32_t nExcess = LOCALPLAYER.primaryAmmo [VULCAN_INDEX] - nMaxPrimaryAmmo [VULCAN_INDEX];
if (nExcess > 0) {
	int32_t nClips = (nExcess + VULCAN_CLIP_CAPACITY - 1) / VULCAN_CLIP_CAPACITY;
	LOCALPLAYER.primaryAmmo [VULCAN_INDEX] -= nClips * VULCAN_CLIP_CAPACITY;
	if (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] < 0)
		LOCALPLAYER.primaryAmmo [VULCAN_INDEX] = 0;
	PrepareObjectCreateEgg (LOCALOBJECT, nClips, OBJ_POWERUP, POW_VULCAN_AMMO);
	}
}

//	------------------------------------------------------------------------------------------------------

int32_t ReturnFlagHome (CObject *objP)
{
	CObject	*initObjP;

if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF) {
	if (SEGMENT (objP->info.nSegment)->m_function == ((objP->info.nId == POW_REDFLAG) ? SEGMENT_FUNC_GOAL_RED : SEGMENT_FUNC_GOAL_BLUE))
		return objP->info.nSegment;
	if ((initObjP = FindInitObject (objP))) {
		objP->info.position.vPos = initObjP->info.position.vPos;
		objP->info.position.mOrient = initObjP->info.position.mOrient;
		objP->RelinkToSeg (initObjP->info.nSegment);
		HUDInitMessage (TXT_FLAG_RETURN);
		audio.PlaySound (SOUND_DROP_WEAPON);
		MultiSendReturnFlagHome (objP->Index ());
		}
	}
return objP->info.nSegment;
}

//------------------------------------------------------------------------------
//eof
