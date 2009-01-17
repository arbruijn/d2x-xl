#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "error.h"
#include "text.h"
#include "u_mem.h"
#include "timer.h"
#include "gameseg.h"
#include "network.h"
#include "network_lib.h"
#include "gamesave.h"
#include "objeffects.h"
#include "fireball.h"
#include "dropobject.h"
#include "headlight.h"

#define	BASE_NET_DROP_DEPTH	8

//------------------------------------------------------------------------------

int InitObjectCount (CObject *objP)
{
	int	nFree, nTotal, i, j, bFree;
	short	nType = objP->info.nType;
	short	id = objP->info.nId;

nFree = nTotal = 0;
for (i = 0, objP = gameData.objs.init.Buffer (); i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->info.nType != nType) || (objP->info.nId != id))
		continue;
	nTotal++;
	for (bFree = 1, j = SEGMENTS [objP->info.nSegment].m_objects; j != -1; j = OBJECTS [j].info.nNextInSeg)
		if ((OBJECTS [j].info.nType == nType) && (OBJECTS [j].info.nId == id)) {
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
	int	h, i, j, bUsed,
			bUseFree,
			objCount = InitObjectCount (objP);
	short	nType = objP->info.nType;
	short	id = objP->info.nId;

// due to OBJECTS being deleted from the CObject list when picked up and recreated when dropped,
// cannot determine exact respawn CSegment, so randomly chose one from all segments where powerups
// of this nType had initially been placed in the level.
if (!objCount)		//no OBJECTS of this nType had initially been placed in the mine.
	return NULL;	//can happen with missile packs
if ((bUseFree = (objCount < 0)))
	objCount = -objCount;
h = d_rand () % objCount + 1;
for (i = 0, objP = gameData.objs.init.Buffer (); i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->info.nType != nType) || (objP->info.nId != id))
		continue;
	// if the current CSegment does not contain a powerup of the nType being looked for,
	// return that CSegment
	if (bUseFree) {
		for (bUsed = 0, j = SEGMENTS [objP->info.nSegment].m_objects; j != -1; j = OBJECTS [j].info.nNextInSeg)
			if ((OBJECTS [j].info.nType == nType) && (OBJECTS [j].info.nId == id)) {
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
//	It is assumed that the CPlayerData has all keys.
int PlayerCanOpenDoor (CSegment *segP, short nSide)
{
CWall* wallP = segP->Wall (nSide);
if (!wallP)
	return 0;						//	no CWall here.
short wallType = wallP->nType;
//	Can't open locked doors.
if (( (wallType == WALL_DOOR) && (wallP->flags & WALL_DOOR_LOCKED)) || (wallType == WALL_CLOSED))
	return 0;
return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return a CSegment %i segments away from initial CSegment.
//	Returns -1 if can't find a CSegment that distance away.

// --------------------------------------------------------------------------------------------------------------------

static int	segQueue [MAX_SEGMENTS_D2X];

int PickConnectedSegment (CObject *objP, int nMaxDepth, int *nDepthP)
{
	int			nCurDepth;
	int			nStartSeg;
	int			nHead, nTail;
	short			nSide, nChild;
	CSegment*	segP;
	CWall*		wallP;
	ubyte			bVisited [MAX_SEGMENTS_D2X];

if (!objP)
	return -1;
nStartSeg = OBJSEG (objP);
nHead =
nTail = 0;
segQueue [nHead++] = nStartSeg;

memset (bVisited, 0, gameData.segs.nSegments);

while (nTail != nHead) {
	nCurDepth = bVisited [segQueue [nTail]];
	if (nCurDepth >= nMaxDepth) {
		if (nDepthP)
			*nDepthP = nCurDepth;
		return segQueue [nTail + d_rand () % (nHead - nTail)];
		}
	segP = SEGMENTS + segQueue [nTail++];

	//	to make Random, switch a pair of entries in rndSide.
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
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
while ((nTail > 0) && (bVisited [segQueue [nTail]] == nCurDepth))
	nTail--;
if (nDepthP)
	*nDepthP = nCurDepth + 1;
return segQueue [nTail + d_rand () % (nHead - nTail + 1)];
}

//	------------------------------------------------------------------------------------------------------
//	Choose CSegment to drop a powerup in.
//	For all active net players, try to create a N CSegment path from the player.  If possible, return that
//	CSegment.  If not possible, try another player.  After a few tries, use a Random CSegment.
//	Don't drop if control center in CSegment.
int ChooseDropSegment (CObject *objP, int *pbFixedPos, int nDropState)
{
	int			nPlayer = 0;
	short			nSegment = -1;
	int			nDepth, nDropDepth;
	int			special, count;
	short			nPlayerSeg;
	CFixVector	tempv, *vPlayerPos;
	fix			nDist = 0;
	int			bUseInitSgm =
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
nDepth = BASE_NET_DROP_DEPTH + ((d_rand () * BASE_NET_DROP_DEPTH*2) >> 15);
vPlayerPos = &OBJECTS [LOCALPLAYER.nObject].info.position.vPos;
nPlayerSeg = OBJECTS [LOCALPLAYER.nObject].info.nSegment;
while (nSegment == -1) {
	if (!IsMultiGame)
		nPlayer = gameData.multiplayer.nLocalPlayer;
	else {
		nPlayer = (d_rand () * gameData.multiplayer.nPlayers) >> 15;
		count = 0;
		while ((count < gameData.multiplayer.nPlayers) &&
				 (!gameData.multiplayer.players [nPlayer].connected ||
				  (nPlayer == gameData.multiplayer.nLocalPlayer) ||
				  ((gameData.app.nGameMode & (GM_TEAM|GM_CAPTURE|GM_ENTROPY)) && (GetTeam (nPlayer) == GetTeam (gameData.multiplayer.nLocalPlayer))))) {
			nPlayer = (nPlayer + 1) % gameData.multiplayer.nPlayers;
			count++;
			}
		if (count == gameData.multiplayer.nPlayers)
			nPlayer = gameData.multiplayer.nLocalPlayer;
		}
	nSegment = PickConnectedSegment (OBJECTS + gameData.multiplayer.players [nPlayer].nObject, nDepth, &nDropDepth);
	if (nDropDepth < BASE_NET_DROP_DEPTH / 2)
		return -1;
#if TRACE
	console.printf (CON_DBG, " %d", nSegment);
#endif
	if (nSegment == -1) {
		nDepth--;
		continue;
		}
	special = SEGMENTS [nSegment].m_nType;
	if ((special == SEGMENT_IS_CONTROLCEN) ||
		 (special == SEGMENT_IS_BLOCKED) ||
		 (special == SEGMENT_IS_SKYBOX) ||
		 (special == SEGMENT_IS_GOAL_BLUE) ||
		 (special == SEGMENT_IS_GOAL_RED) ||
		 (special == SEGMENT_IS_TEAM_BLUE) ||
		 (special == SEGMENT_IS_TEAM_RED))
		nSegment = -1;
	else {	//don't drop in any children of control centers
		for (int i = 0; i < 6; i++) {
			int nChild = SEGMENTS [nSegment].m_children [i];
			if (IS_CHILD (nChild) && (SEGMENTS [nChild].m_nType == SEGMENT_IS_CONTROLCEN)) {
				nSegment = -1;
				break;
				}
			}
		}
	//bail if not far enough from original position
	if (nSegment > -1) {
		tempv = SEGMENTS [nSegment].Center ();
		nDist = FindConnectedDistance (*vPlayerPos, nPlayerSeg, tempv, nSegment, -1, WID_FLY_FLAG, 0);
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
	return (d_rand () * gameData.segs.nLastSegment) >> 15;
	}
return nSegment;
}

//	------------------------------------------------------------------------------------------------------

void DropPowerups (void)
{
	short	h = gameData.objs.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objs.dropInfo [i].nNextPowerup;
	if (!MaybeDropNetPowerup (i, gameData.objs.dropInfo [i].nPowerupType, CHECK_DROP))
		break;
	}
}

//	------------------------------------------------------------------------------------------------------

void RespawnDestroyedWeapon (short nObject)
{
	int	h = gameData.objs.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objs.dropInfo [i].nNextPowerup;
	if ((gameData.objs.dropInfo [i].nObject == nObject) &&
		 (gameData.objs.dropInfo [i].nDropTime < 0)) {
		gameData.objs.dropInfo [i].nDropTime = 0;
		MaybeDropNetPowerup (i, gameData.objs.dropInfo [i].nPowerupType, CHECK_DROP);
		}
	}
}

//	------------------------------------------------------------------------------------------------------

static int AddDropInfo (void)
{
	int	h;

if (gameData.objs.nFreeDropped < 0)
	return -1;
h = gameData.objs.nFreeDropped;
gameData.objs.nFreeDropped = gameData.objs.dropInfo [h].nNextPowerup;
gameData.objs.dropInfo [h].nPrevPowerup = gameData.objs.nLastDropped;
gameData.objs.dropInfo [h].nNextPowerup = -1;
if (gameData.objs.nLastDropped >= 0)
	gameData.objs.dropInfo [gameData.objs.nLastDropped].nNextPowerup = h;
else
	gameData.objs.nFirstDropped =
	gameData.objs.nLastDropped = h;
gameData.objs.nDropped++;
return h;
}

//	------------------------------------------------------------------------------------------------------

static void DelDropInfo (int h)
{
	int	i, j;

if (h < 0)
	return;
i = gameData.objs.dropInfo [h].nPrevPowerup;
j = gameData.objs.dropInfo [h].nNextPowerup;
if (i < 0)
	gameData.objs.nFirstDropped = j;
else
	gameData.objs.dropInfo [i].nNextPowerup = j;
if (j < 0)
	gameData.objs.nLastDropped = i;
else
	gameData.objs.dropInfo [j].nPrevPowerup = i;
gameData.objs.dropInfo [h].nNextPowerup = gameData.objs.nFreeDropped;
gameData.objs.nFreeDropped = h;
gameData.objs.nDropped--;
}

//	------------------------------------------------------------------------------------------------------
//	Drop cloak powerup if in a network game.

int MaybeDropNetPowerup (short nObject, int nPowerupType, int nDropState)
{
if (EGI_FLAG (bImmortalPowerups, 0, 0, 0) || (IsMultiGame && !IsCoopGame)) {
	short			nSegment;
	int			h, bFixedPos = 0;
	CFixVector	vNewPos;

	MultiSendWeapons (1);
#if 0
	if ((gameData.app.nGameMode & GM_NETWORK) && (nDropState < CHECK_DROP) && (nPowerupType >= 0)) {
		if (gameData.multiplayer.powerupsInMine [nPowerupType] >= gameData.multiplayer.maxPowerupsAllowed [nPowerupType])
			return 0;
		}
#endif
	if (gameData.reactor.bDestroyed || gameStates.app.bEndLevelSequence)
		return 0;
	gameData.multigame.create.nLoc = 0;
	if (gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].nSpawnDelay != 0)) {
		if (nDropState == CHECK_DROP) {
			if ((gameData.objs.dropInfo [nObject].nDropTime < 0) ||
				 (gameStates.app.nSDLTicks - gameData.objs.dropInfo [nObject].nDropTime <
				  extraGameInfo [IsMultiGame].nSpawnDelay))
				return 0;
			nDropState = EXEC_DROP;
			}
		else if (nDropState == INIT_DROP) {
			if (0 > (h = AddDropInfo ()))
				return 0;
			gameData.objs.dropInfo [h].nObject = nObject;
			gameData.objs.dropInfo [h].nPowerupType = nPowerupType;
			gameData.objs.dropInfo [h].nDropTime =
				 (extraGameInfo [IsMultiGame].nSpawnDelay < 0) ? -1 : gameStates.app.nSDLTicks;
			return 0;
			}
		if (nDropState == EXEC_DROP) {
			DelDropInfo (nObject);
			}
		}
	nObject = CallObjectCreateEgg (OBJECTS + LOCALPLAYER.nObject,
											 1, OBJ_POWERUP, nPowerupType);
	if (nObject < 0)
		return 0;
	nSegment = ChooseDropSegment (OBJECTS + nObject, &bFixedPos, nDropState);
	OBJECTS [nObject].mType.physInfo.velocity.SetZero ();
	if (bFixedPos)
		vNewPos = OBJECTS [nObject].info.position.vPos;
	else
		vNewPos = SEGMENTS [nSegment].RandomPoint ();
	MultiSendCreatePowerup (nPowerupType, nSegment, nObject, &vNewPos);
	if (!bFixedPos)
		OBJECTS [nObject].info.position.vPos = vNewPos;
	OBJECTS [nObject].RelinkToSeg (nSegment);
	/*Object*/CreateExplosion (nSegment, vNewPos, I2X (5), VCLIP_POWERUP_DISAPPEARANCE);
	return 1;
	}
return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Return true if current CSegment contains some CObject.
int SegmentContainsObject (int objType, int obj_id, int nSegment)
{
	int	nObject;

if (nSegment == -1)
	return 0;
nObject = SEGMENTS [nSegment].m_objects;
while (nObject != -1)
	if ((OBJECTS [nObject].info.nType == objType) && (OBJECTS [nObject].info.nId == obj_id))
		return 1;
	else
		nObject = OBJECTS [nObject].info.nNextInSeg;
return 0;
}

//	------------------------------------------------------------------------------------------------------

int ObjectNearbyAux (int nSegment, int objectType, int object_id, int depth)
{
	int	i, seg2;

if (depth == 0)
	return 0;
if (SegmentContainsObject (objectType, object_id, nSegment))
	return 1;
for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	seg2 = SEGMENTS [nSegment].m_children [i];
	if (seg2 != -1)
		if (ObjectNearbyAux (seg2, objectType, object_id, depth-1))
			return 1;
		}
	return 0;
}


//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
int WeaponNearby (CObject *objP, int weapon_id)
{
return ObjectNearbyAux (objP->info.nSegment, OBJ_POWERUP, weapon_id, 3);
}

//	------------------------------------------------------------------------------------------------------

void MaybeReplacePowerupWithEnergy (CObject *delObjP)
{
	int	nWeapon=-1;

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

//	Don't drop vulcan ammo if CPlayerData maxed out.
if (( (nWeapon == VULCAN_INDEX) || (delObjP->info.contains.nId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
	delObjP->info.contains.nCount = 0;
else if (( (nWeapon == GAUSS_INDEX) || (delObjP->info.contains.nId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
	delObjP->info.contains.nCount = 0;
else if (nWeapon != -1) {
	if ((PlayerHasWeapon (nWeapon, 0, -1, 1) & HAS_WEAPON_FLAG) ||
			WeaponNearby (delObjP, delObjP->info.contains.nId)) {
		if (d_rand () > 16384) {
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
		if (d_rand () > 16384) {
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
if ((delObjP->info.nCreator == BOSS_GATE_MATCEN_NUM) && (delObjP->info.contains.nId == POW_ENERGY) && (delObjP->info.contains.nType == OBJ_POWERUP)) {
#if TRACE
	console.printf (CON_DBG, "Converting energy powerup to nothing because robot %i gated in by boss.\n", OBJ_IDX (delObjP));
#endif
	delObjP->info.contains.nCount = 0;
	}

// Change multiplayer extra-lives into invulnerability
if ((gameData.app.nGameMode & GM_MULTI) && (delObjP->info.contains.nId == POW_EXTRA_LIFE))
	delObjP->info.contains.nId = POW_INVUL;
}

//------------------------------------------------------------------------------

int DropPowerup (ubyte nType, ubyte id, short owner, int num, const CFixVector& init_vel, const CFixVector& pos, short nSegment)
{
	short			nObject=-1;
	CObject		*objP;
	CFixVector	new_velocity, vNewPos;
	fix			old_mag;
   int			count;

switch (nType) {
	case OBJ_POWERUP:
		for (count = 0; count < num; count++) {
			int	rand_scale;
			new_velocity = init_vel;
			old_mag = init_vel.Mag();

			//	We want powerups to move more in network mode.
			if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
				rand_scale = 4;
				//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
				if (id == POW_EXTRA_LIFE)
					id = POW_INVUL;
				}
			else
				rand_scale = 2;
			new_velocity[X] += FixMul (old_mag+I2X (32), d_rand ()*rand_scale - 16384*rand_scale);
			new_velocity[Y] += FixMul (old_mag+I2X (32), d_rand ()*rand_scale - 16384*rand_scale);
			new_velocity[Z] += FixMul (old_mag+I2X (32), d_rand ()*rand_scale - 16384*rand_scale);
			// Give keys zero velocity so they can be tracked better in multi
			if ((gameData.app.nGameMode & GM_MULTI) &&
				 (( (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD)) || (id == POW_MONSTERBALL)))
				new_velocity.SetZero ();
			vNewPos = pos;

			if (IsMultiGame) {
				if (gameData.multigame.create.nLoc >= MAX_NET_CREATE_OBJECTS) {
					return -1;
					}
				if ((gameData.app.nGameMode & GM_NETWORK) && networkData.nStatus == NETSTAT_ENDLEVEL)
					return -1;
				}
			nObject = CreatePowerup (id, owner, nSegment, vNewPos, 0);
			if (nObject < 0) {
				Int3 ();
				return nObject;
				}
			if (IsMultiGame)
				gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
			objP = OBJECTS + nObject;
			objP->mType.physInfo.velocity = new_velocity;
			objP->mType.physInfo.drag = 512;	//1024;
			objP->mType.physInfo.mass = I2X (1);
			objP->mType.physInfo.flags = PF_BOUNCE;
			objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
			objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
			objP->rType.vClipInfo.nCurFrame = 0;

			switch (objP->info.nId) {
				case POW_CONCUSSION_1:
				case POW_CONCUSSION_4:
				case POW_SHIELD_BOOST:
				case POW_ENERGY:
					objP->info.xLifeLeft = (d_rand () + I2X (3)) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
					if (gameData.app.nGameMode & GM_MULTI)
						objP->info.xLifeLeft /= 2;
					break;
				default:
//						if (gameData.app.nGameMode & GM_MULTI)
//							objP->info.xLifeLeft = (d_rand () + I2X (3)) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
					break;
				}
			}
		break;

	case OBJ_ROBOT:
		for (count=0; count<num; count++) {
			int	rand_scale;
			new_velocity = init_vel;
			old_mag = init_vel.Mag();
			CFixVector::Normalize(new_velocity);
			//	We want powerups to move more in network mode.
			rand_scale = 2;
			new_velocity[X] += (d_rand ()-16384)*2;
			new_velocity[Y] += (d_rand ()-16384)*2;
			new_velocity[Z] += (d_rand ()-16384)*2;
			CFixVector::Normalize(new_velocity);
			new_velocity *= ((I2X (32) + old_mag) * rand_scale);
			vNewPos = pos;
			//	This is dangerous, could be outside mine.
//				vNewPos.x += (d_rand ()-16384)*8;
//				vNewPos.y += (d_rand ()-16384)*7;
//				vNewPos.z += (d_rand ()-16384)*6;
			nObject = CreateRobot (id, nSegment, vNewPos);
			if (nObject < 0) {
#if TRACE
				console.printf (1, "Can't create CObject in ObjectCreateEgg, robots.  Aborting.\n");
#endif
				Int3 ();
				return nObject;
				}
			if (gameData.app.nGameMode & GM_MULTI)
				gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
			objP = &OBJECTS [nObject];
			//Set polygon-CObject-specific data
			objP->rType.polyObjInfo.nModel = ROBOTINFO (objP->info.nId).nModel;
			objP->rType.polyObjInfo.nSubObjFlags = 0;
			//set Physics info
			objP->mType.physInfo.velocity = new_velocity;
			objP->mType.physInfo.mass = ROBOTINFO (objP->info.nId).mass;
			objP->mType.physInfo.drag = ROBOTINFO (objP->info.nId).drag;
			objP->mType.physInfo.flags |= (PF_LEVELLING);
			objP->info.xShields = ROBOTINFO (objP->info.nId).strength;
			objP->cType.aiInfo.behavior = AIB_NORMAL;
			gameData.ai.localInfo [nObject].playerAwarenessType = WEAPON_ROBOT_COLLISION;
			gameData.ai.localInfo [nObject].playerAwarenessTime = I2X (3);
			objP->cType.aiInfo.CURRENT_STATE = AIS_LOCK;
			objP->cType.aiInfo.GOAL_STATE = AIS_LOCK;
			objP->cType.aiInfo.REMOTE_OWNER = -1;
			if (ROBOTINFO (id).bossFlag)
				AddBoss (nObject);
			}
		// At JasenW's request, robots which contain robots
		// sometimes drop shields.
		if (d_rand () > 16384)
			DropPowerup (OBJ_POWERUP, POW_SHIELD_BOOST, -1, 1, init_vel, pos, nSegment);
		break;

	default:
		Error ("Error: Illegal nType (%i) in CObject spawning.\n", nType);
	}
return nObject;
}

// ----------------------------------------------------------------------------
// Returns created CObject number.
// If CObject dropped by CPlayerData, set flag.
int ObjectCreateEgg (CObject *objP)
{
	int	nObject;

if (!IsMultiGame & (objP->info.nType != OBJ_PLAYER)) {
	if (objP->info.contains.nType == OBJ_POWERUP) {
		if (objP->info.contains.nId == POW_SHIELD_BOOST) {
			if (LOCALPLAYER.shields >= I2X (100)) {
				if (d_rand () > 16384) {
#if TRACE
					console.printf (CON_DBG, "Not dropping shield!\n");
#endif
					return -1;
					}
				} 
			else  if (LOCALPLAYER.shields >= I2X (150)) {
				if (d_rand () > 8192) {
#if TRACE
					console.printf (CON_DBG, "Not dropping shield!\n");
#endif
					return -1;
					}
				}
			}
		else if (objP->info.contains.nId == POW_ENERGY) {
			if (LOCALPLAYER.energy >= I2X (100)) {
				if (d_rand () > 16384) {
#if TRACE
					console.printf (CON_DBG, "Not dropping energy!\n");
#endif
					return -1;
					}
				} 
			else if (LOCALPLAYER.energy >= I2X (150)) {
				if (d_rand () > 8192) {
#if TRACE
					console.printf (CON_DBG, "Not dropping energy!\n");
#endif
					return -1;
					}
				}
			}
		}
	}

nObject = DropPowerup (
	objP->info.contains.nType,
	 (ubyte) objP->info.contains.nId,
	 (short) (
		 ((gameData.app.nGameMode & GM_ENTROPY) &&
		 (objP->info.contains.nType == OBJ_POWERUP) &&
		 (objP->info.contains.nId == POW_ENTROPY_VIRUS) &&
		 (objP->info.nType == OBJ_PLAYER)) ?
		GetTeam (objP->info.nId) + 1 : -1
		),
	objP->info.contains.nCount,
	objP->mType.physInfo.velocity,
	objP->info.position.vPos, objP->info.nSegment);
if (nObject >= 0) {
	if (objP->info.nType == OBJ_PLAYER) {
		if (objP->info.nId == gameData.multiplayer.nLocalPlayer)
			OBJECTS [nObject].info.nFlags |= OF_PLAYER_DROPPED;
		}
	else if (objP->info.nType == OBJ_ROBOT) {
		if (objP->info.contains.nType == OBJ_POWERUP) {
			if (objP->info.contains.nId == POW_VULCAN || objP->info.contains.nId == POW_GAUSS)
				OBJECTS [nObject].cType.powerupInfo.nCount = VULCAN_WEAPON_AMMO_AMOUNT;
			else if (objP->info.contains.nId == POW_OMEGA)
				OBJECTS [nObject].cType.powerupInfo.nCount = MAX_OMEGA_CHARGE;
			}
		}
	}
return nObject;
}

// -- extern int Items_destroyed;

//	-------------------------------------------------------------------------------------------------------
//	Put count OBJECTS of nType nType (eg, powerup), id = id (eg, energy) into *objP, then drop them! Yippee!
//	Returns created CObject number.
int CallObjectCreateEgg (CObject *objP, int count, int nType, int id)
{
if (count <= 0)
	return -1;
objP->info.contains.nCount = count;
objP->info.contains.nType = nType;
objP->info.contains.nId = id;
return ObjectCreateEgg (objP);
}

//------------------------------------------------------------------------------
//creates afterburner blobs behind the specified CObject
void DropAfterburnerBlobs (CObject *objP, int count, fix xSizeScale, fix xLifeTime, CObject *pParent, int bThruster)
{
	short				i, nSegment, nThrusters;
	CObject			*blobObjP;
	tThrusterInfo	ti;

nThrusters = CalcThrusterPos (objP, &ti, 1);
for (i = 0; i < nThrusters; i++) {
	nSegment = FindSegByPos (ti.vPos[i], objP->info.nSegment, 1, 0);
	if (nSegment == -1)
		continue;
	if (!(blobObjP = /*Object*/CreateExplosion (nSegment, ti.vPos [i], xSizeScale, VCLIP_AFTERBURNER_BLOB)))
		continue;
	if (xLifeTime != -1) {
		blobObjP->rType.vClipInfo.xTotalTime = xLifeTime;
		blobObjP->rType.vClipInfo.xFrameTime = FixMulDiv (gameData.eff.vClips [0][VCLIP_AFTERBURNER_BLOB].xFrameTime,
																		  xLifeTime, blobObjP->info.xLifeLeft);
		blobObjP->info.xLifeLeft = xLifeTime;
		}
	AddChildObjectP (pParent, blobObjP);
	blobObjP->info.renderType = RT_THRUSTER;
	if (bThruster)
		blobObjP->mType.physInfo.flags |= PF_WIGGLE;
	}
}

//	-----------------------------------------------------------------------------

int MaybeDropPrimaryWeaponEgg (CObject *playerObjP, int nWeapon)
{
	int nWeaponFlag = HAS_FLAG (nWeapon);
	int nPowerup = primaryWeaponToPowerup [nWeapon];

if ((gameData.multiplayer.players [playerObjP->info.nId].primaryWeaponFlags & nWeaponFlag) &&
	 !(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].loadout.nGuns & nWeaponFlag)))
	return CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerup);
else
	return -1;
}

//	-----------------------------------------------------------------------------

void MaybeDropSecondaryWeaponEgg (CObject *playerObjP, int nWeapon, int count)
{
	int nWeaponFlag = HAS_FLAG (nWeapon);
	int nPowerup = secondaryWeaponToPowerup [nWeapon];

if (gameData.multiplayer.players [playerObjP->info.nId].secondaryWeaponFlags & nWeaponFlag) {
	int i, maxCount = ((EGI_FLAG (bDropAllMissiles, 0, 0, 0)) ? count : min(count, 3));

	for (i = 0; i < maxCount; i++)
		CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerup);
	}
}

//	-----------------------------------------------------------------------------

void MaybeDropDeviceEgg (CPlayerData *playerP, CObject *playerObjP, int nDeviceFlag, int nPowerupId)
{
if ((gameData.multiplayer.players [playerObjP->info.nId].flags & nDeviceFlag) &&
	 !(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].loadout.nDevices & nDeviceFlag)))
	CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerupId);
}

//	-----------------------------------------------------------------------------

void DropMissile1or4 (CObject *playerObjP, int nMissileIndex)
{
	int nMissiles, nPowerupId;

if ((nMissiles = gameData.multiplayer.players [playerObjP->info.nId].secondaryAmmo [nMissileIndex])) {
	nPowerupId = secondaryWeaponToPowerup [nMissileIndex];
	if (!IsMultiGame && (nMissileIndex == CONCUSSION_INDEX))
		nMissiles -= 4;	//player gets 4 concs anyway when respawning, so avoid them building up
	if (!(IsMultiGame || EGI_FLAG (bDropAllMissiles, 0, 0, 0)) && (nMissiles > 10))
		nMissiles = 10;
	if (nMissiles > 0) {
		CallObjectCreateEgg (playerObjP, nMissiles / 4, OBJ_POWERUP, nPowerupId + 1);
		CallObjectCreateEgg (playerObjP, nMissiles % 4, OBJ_POWERUP, nPowerupId);
		}
	}
}

// -- int	Items_destroyed = 0;

//	-----------------------------------------------------------------------------

void DropPlayerEggs (CObject *playerObjP)
{
if ((playerObjP->info.nType == OBJ_PLAYER) || (playerObjP->info.nType == OBJ_GHOST)) {
	int			rthresh, nFlag;
	int			nPlayerId = playerObjP->info.nId;
	short			nObject, plrObjNum = OBJ_IDX (playerObjP);
	int			nVulcanAmmo=0;
	CFixVector	vRandom;
	CPlayerData		*playerP = gameData.multiplayer.players + nPlayerId;

	// -- Items_destroyed = 0;

	// Seed the Random number generator so in net play the eggs will always
	// drop the same way
	if (IsMultiGame) {
		gameData.multigame.create.nLoc = 0;
	}

	//	If the CPlayerData had smart mines, maybe arm one of them.
	rthresh = 30000;
	while ((playerP->secondaryAmmo [SMARTMINE_INDEX] % 4 == 1) && (d_rand () < rthresh)) {
		short			nNewSeg;
		CFixVector	tvec;

		vRandom = CFixVector::Random();
		rthresh /= 2;
		tvec = playerObjP->info.position.vPos + vRandom;
		nNewSeg = FindSegByPos (tvec, playerObjP->info.nSegment, 1, 0);
		if (nNewSeg != -1)
			CreateNewWeapon (&vRandom, &tvec, nNewSeg, plrObjNum, SMARTMINE_ID, 0);
	  	}

		//	If the CPlayerData had proximity bombs, maybe arm one of them.
		if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
			rthresh = 30000;
			while ((playerP->secondaryAmmo [PROXMINE_INDEX] % 4 == 1) && (d_rand () < rthresh)) {
				short			nNewSeg;
				CFixVector	tvec;

				vRandom = CFixVector::Random();
				rthresh /= 2;
				tvec = playerObjP->info.position.vPos + vRandom;
				nNewSeg = FindSegByPos (tvec, playerObjP->info.nSegment, 1, 0);
				if (nNewSeg != -1)
					CreateNewWeapon (&vRandom, &tvec, nNewSeg, plrObjNum, PROXMINE_ID, 0);
			}
		}

		//	If the CPlayerData dies and he has powerful lasers, create the powerups here.

		if (playerP->laserLevel > MAX_LASER_LEVEL) {
			if (!(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && 
				   (extraGameInfo [IsMultiGame].loadout.nGuns & HAS_FLAG (SUPER_LASER_INDEX)))) {
				CallObjectCreateEgg (playerObjP, playerP->laserLevel - MAX_LASER_LEVEL, OBJ_POWERUP, POW_SUPERLASER);
				CallObjectCreateEgg (playerObjP, MAX_LASER_LEVEL, OBJ_POWERUP, POW_LASER);
				}
			}
		else if (playerP->laserLevel >= 1) {
			if (!(gameStates.app.bHaveExtraGameInfo [IsMultiGame] && 
				   (extraGameInfo [IsMultiGame].loadout.nGuns & (HAS_FLAG (POW_LASER) | HAS_FLAG (SUPER_LASER_INDEX)))))
				CallObjectCreateEgg (playerObjP, playerP->laserLevel, OBJ_POWERUP, POW_LASER);	// Note: laserLevel = 0 for laser level 1.
			}

		//	Drop quad laser if appropos
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_QUAD_LASERS, POW_QUADLASER);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_CLOAKED, POW_CLOAK);
		while (playerP->nInvuls--)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_INVUL);
		while (playerP->nCloaks--)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_CLOAK);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_FULLMAP, POW_FULL_MAP);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_AFTERBURNER, POW_AFTERBURNER);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_AMMO_RACK, POW_AMMORACK);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_CONVERTER, POW_CONVERTER);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_SLOWMOTION, POW_SLOWMOTION);
		MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_BULLETTIME, POW_BULLETTIME);
		if (PlayerHasHeadlight (nPlayerId) && !EGI_FLAG (headlight.bBuiltIn, 0, 1, 0) &&
			 !(gameStates.app.bHaveExtraGameInfo [1] && IsMultiGame && extraGameInfo [1].bDarkness))
			MaybeDropDeviceEgg (playerP, playerObjP, PLAYER_FLAGS_HEADLIGHT, POW_HEADLIGHT);
		// drop the other enemies flag if you have it

	playerP->nInvuls =
	playerP->nCloaks = 0;
	playerP->flags &= ~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED);
	if ((gameData.app.nGameMode & GM_CAPTURE) && (playerP->flags & PLAYER_FLAGS_FLAG))
		CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, (GetTeam (nPlayerId) == TEAM_RED) ? POW_BLUEFLAG : POW_REDFLAG);

#if !DBG
		if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))
#endif
		if (IsHoardGame ||
		    (IsEntropyGame && extraGameInfo [1].entropy.nVirusStability)) {
			// Drop hoard orbs

			int maxCount, i;
#if TRACE
			console.printf (CON_DBG, "HOARD MODE: Dropping %d orbs \n", playerP->secondaryAmmo [PROXMINE_INDEX]);
#endif
			maxCount = playerP->secondaryAmmo [PROXMINE_INDEX];
			if ((gameData.app.nGameMode & GM_HOARD) && (maxCount > 12))
				maxCount = 12;
			for (i = 0; i < maxCount; i++)
				CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_HOARD_ORB);
		}

		//Drop the vulcan, gauss, and ammo
		nVulcanAmmo = playerP->primaryAmmo [VULCAN_INDEX];
		nFlag = HAS_FLAG (VULCAN_INDEX) | HAS_FLAG (GAUSS_INDEX);
		if (extraGameInfo [IsMultiGame].loadout.nGuns & nFlag) {
			nVulcanAmmo -= GAUSS_WEAPON_AMMO_AMOUNT;
			if (nVulcanAmmo < 0)
				nVulcanAmmo = 0;
			}
		if ((int (playerP->primaryWeaponFlags & nFlag) == nFlag) && (int (extraGameInfo [IsMultiGame].loadout.nGuns & nFlag) != nFlag))
			nVulcanAmmo /= 2;		//if both vulcan & gauss, each gets half
		if ((nVulcanAmmo < VULCAN_AMMO_AMOUNT) && !(extraGameInfo [IsMultiGame].loadout.nGuns & nFlag))
			nVulcanAmmo = VULCAN_AMMO_AMOUNT;	//make sure gun has at least as much as a powerup
		nObject = MaybeDropPrimaryWeaponEgg (playerObjP, VULCAN_INDEX);
		if (nObject >= 0)
			OBJECTS [nObject].cType.powerupInfo.nCount = nVulcanAmmo;
		nObject = MaybeDropPrimaryWeaponEgg (playerObjP, GAUSS_INDEX);
		if (nObject >= 0)
			OBJECTS [nObject].cType.powerupInfo.nCount = nVulcanAmmo;

		//	Drop the rest of the primary weapons
		MaybeDropPrimaryWeaponEgg (playerObjP, SPREADFIRE_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, PLASMA_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, FUSION_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, HELIX_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, PHOENIX_INDEX);
		nObject = MaybeDropPrimaryWeaponEgg (playerObjP, OMEGA_INDEX);
		if (nObject >= 0)
			OBJECTS [nObject].cType.powerupInfo.nCount =
				(playerObjP->info.nId == gameData.multiplayer.nLocalPlayer) ? gameData.omega.xCharge [IsMultiGame] : DEFAULT_MAX_OMEGA_CHARGE;
		//	Drop the secondary weapons
		//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?
		if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
			MaybeDropSecondaryWeaponEgg (playerObjP, PROXMINE_INDEX, (playerP->secondaryAmmo [PROXMINE_INDEX])/4);
		MaybeDropSecondaryWeaponEgg (playerObjP, SMART_INDEX, playerP->secondaryAmmo [SMART_INDEX]);
		MaybeDropSecondaryWeaponEgg (playerObjP, MEGA_INDEX, playerP->secondaryAmmo [MEGA_INDEX]);
		if (!(gameData.app.nGameMode & GM_ENTROPY))
			MaybeDropSecondaryWeaponEgg (playerObjP, SMARTMINE_INDEX, (playerP->secondaryAmmo [SMARTMINE_INDEX])/4);
		MaybeDropSecondaryWeaponEgg (playerObjP, EARTHSHAKER_INDEX, playerP->secondaryAmmo [EARTHSHAKER_INDEX]);
		//	Drop the CPlayerData's missiles in packs of 1 and/or 4
		DropMissile1or4 (playerObjP, HOMING_INDEX);
		DropMissile1or4 (playerObjP, GUIDED_INDEX);
		DropMissile1or4 (playerObjP, CONCUSSION_INDEX);
		DropMissile1or4 (playerObjP, FLASHMSL_INDEX);
		DropMissile1or4 (playerObjP, MERCURY_INDEX);
		//	If CPlayerData has vulcan ammo, but no vulcan cannon, drop the ammo.
		if (!(playerP->primaryWeaponFlags & HAS_VULCAN_FLAG)) {
			int	amount = playerP->primaryAmmo [VULCAN_INDEX];
			if (amount > 200) {
#if TRACE
				console.printf (CON_DBG, "Surprising amount of vulcan ammo: %i bullets. \n", amount);
#endif
				amount = 200;
			}
			while (amount > 0) {
				CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_VULCAN_AMMO);
				amount -= VULCAN_AMMO_AMOUNT;
			}
		}

		//	Always drop a shield and energy powerup.
		if (IsMultiGame) {
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_SHIELD_BOOST);
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_ENERGY);
		}
	}
}

//	------------------------------------------------------------------------------------------------------

int ReturnFlagHome (CObject *objP)
{
	CObject	*initObjP;

if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF) {
	if (SEGMENTS [objP->info.nSegment].m_nType == ((objP->info.nId == POW_REDFLAG) ? SEGMENT_IS_GOAL_RED : SEGMENT_IS_GOAL_BLUE))
		return objP->info.nSegment;
	if ((initObjP = FindInitObject (objP))) {
	//objP->info.nSegment = initObjP->info.nSegment;
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
