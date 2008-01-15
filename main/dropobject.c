#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"
#include "error.h"
#include "text.h"
#include "timer.h"
#include "gameseg.h"
#include "network.h"
#include "object.h"
#include "gamesave.h"
#include "objeffects.h"
#include "fireball.h"
#include "dropobject.h"

#define	BASE_NET_DROP_DEPTH	8

//------------------------------------------------------------------------------

int InitObjectCount (tObject *objP)
{
	int	nFree, nTotal, i, j, bFree;
	short	nType = objP->nType;
	short	id = objP->id; 

nFree = nTotal = 0;
for (i = 0, objP = gameData.objs.init; i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->nType != nType) || (objP->id != id))
		continue;
	nTotal++;
	for (bFree = 1, j = gameData.segs.segments [objP->nSegment].objects; j != -1; j = gameData.objs.objects [j].next)
		if ((gameData.objs.objects [j].nType == nType) && (gameData.objs.objects [j].id == id)) {
			bFree = 0;
			break;
			}
	if (bFree)
		nFree++;
	}
return nFree ? -nFree : nTotal;
}

//------------------------------------------------------------------------------

tObject *FindInitObject (tObject *objP)
{
	int	h, i, j, bUsed, 
			bUseFree, 
			objCount = InitObjectCount (objP);
	short	nType = objP->nType;
	short	id = objP->id; 

// due to gameData.objs.objects being deleted from the tObject list when picked up and recreated when dropped, 
// cannot determine exact respawn tSegment, so randomly chose one from all segments where powerups
// of this nType had initially been placed in the level.
if (!objCount)		//no gameData.objs.objects of this nType had initially been placed in the mine. 
	return NULL;	//can happen with missile packs
d_srand (TimerGetFixedSeconds ());
if ((bUseFree = (objCount < 0)))
	objCount = -objCount;
h = d_rand () % objCount + 1;
for (i = 0, objP = gameData.objs.init; i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->nType != nType) || (objP->id != id))
		continue;
	// if the current tSegment does not contain a powerup of the nType being looked for, 
	// return that tSegment
	if (bUseFree) {
		for (bUsed = 0, j = gameData.segs.segments [objP->nSegment].objects; j != -1; j = gameData.objs.objects [j].next)
			if ((gameData.objs.objects [j].nType == nType) && (gameData.objs.objects [j].id == id)) {
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
//	It is assumed that the tPlayer has all keys.
int PlayerCanOpenDoor (tSegment *segP, short nSide)
{
	short	nWall, wallType;

nWall = WallNumP (segP, nSide);
if (!IS_WALL (nWall))
	return 0;						//	no tWall here.
wallType = gameData.walls.walls [nWall].nType;
//	Can't open locked doors.
if (( (wallType == WALL_DOOR) && (gameData.walls.walls [nWall].flags & WALL_DOOR_LOCKED)) || (wallType == WALL_CLOSED))
	return 0;
return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return a tSegment %i segments away from initial tSegment.
//	Returns -1 if can't find a tSegment that distance away.

#define	QUEUE_SIZE	64

int PickConnectedSegment (tObject *objP, int max_depth)
{
	int		i;
	int		cur_depth;
	int		start_seg;
	int		head, tail;
	int		seg_queue [QUEUE_SIZE*2];
	sbyte		bVisited [MAX_SEGMENTS_D2X];
	sbyte		depth [MAX_SEGMENTS_D2X];
	sbyte		side_rand [MAX_SIDES_PER_SEGMENT];

	start_seg = objP->nSegment;
	head = 0;
	tail = 0;
	seg_queue [head++] = start_seg;

	memset (bVisited, 0, gameData.segs.nLastSegment+1);
	memset (depth, 0, gameData.segs.nLastSegment+1);
	cur_depth = 0;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		side_rand [i] = i;

	//	Now, randomize a bit to start, so we don't always get started in the same direction.
	for (i=0; i<4; i++) {
		int	ind1, temp;

		ind1 = (d_rand () * MAX_SIDES_PER_SEGMENT) >> 15;
		temp = side_rand [ind1];
		side_rand [ind1] = side_rand [i];
		side_rand [i] = temp;
	}


	while (tail != head) {
		int		nSide, count;
		tSegment	*segP;
		int		ind1, ind2, temp;

		if (cur_depth >= max_depth) {
			return seg_queue [tail];
		}

		segP = gameData.segs.segments + seg_queue [tail++];
		tail &= QUEUE_SIZE-1;

		//	to make random, switch a pair of entries in side_rand.
		ind1 = (d_rand () * MAX_SIDES_PER_SEGMENT) >> 15;
		ind2 = (d_rand () * MAX_SIDES_PER_SEGMENT) >> 15;
		temp = side_rand [ind1];
		side_rand [ind1] = side_rand [ind2];
		side_rand [ind2] = temp;

		count = 0;
		for (nSide=ind1; count<MAX_SIDES_PER_SEGMENT; count++) {
			short	snrand, nWall;

			if (nSide == MAX_SIDES_PER_SEGMENT)
				nSide = 0;

			snrand = side_rand [nSide];
			nWall = WallNumP (segP, snrand);
			nSide++;

			if (( (!IS_WALL (nWall)) && (segP->children [snrand] > -1)) || PlayerCanOpenDoor (segP, snrand)) {
				if (!bVisited [segP->children [snrand]]) {
					seg_queue [head++] = segP->children [snrand];
					bVisited [segP->children [snrand]] = 1;
					depth [segP->children [snrand]] = cur_depth+1;
					head &= QUEUE_SIZE-1;
					if (head > tail) {
						if (head == tail + QUEUE_SIZE-1)
							Int3 ();	//	queue overflow.  Make it bigger!
					} else
						if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
							Int3 ();	//	queue overflow.  Make it bigger!
				}
			}
		}
		if ((seg_queue [tail] < 0) || (seg_queue [tail] > gameData.segs.nLastSegment)) {
			// -- Int3 ();	//	Something bad has happened.  Queue is trashed.  --MK, 12/13/94
			return -1;
		}
		cur_depth = depth [seg_queue [tail]];
	}
#if TRACE
	con_printf (CONDBG, "...failed at depth %i, returning -1\n", cur_depth);
#endif
	return -1;
}

//	------------------------------------------------------------------------------------------------------
//	Choose tSegment to drop a powerup in.
//	For all active net players, try to create a N tSegment path from the player.  If possible, return that
//	tSegment.  If not possible, try another player.  After a few tries, use a random tSegment.
//	Don't drop if control center in tSegment.
int ChooseDropSegment (tObject *objP, int *pbFixedPos, int nDropState)
{
	int			pnum = 0;
	short			nSegment = -1;
	int			nCurDropDepth;
	int			special, count;
	short			player_seg;
	vmsVector	tempv, *player_pos;
	fix			nDist;
	int			bUseInitSgm = 
						objP &&
						(EGI_FLAG (bFixedRespawns, 0, 0, 0) || 
						 (EGI_FLAG (bEnhancedCTF, 0, 0, 0) && 
						 (objP->nType == OBJ_POWERUP) && ((objP->id == POW_BLUEFLAG) || (objP->id == POW_REDFLAG))));
#if TRACE
con_printf (CONDBG, "ChooseDropSegment:");
#endif
if (bUseInitSgm) {
	tObject *initObjP = FindInitObject (objP);
	if (initObjP) {
		*pbFixedPos = 1;
		objP->position.vPos = initObjP->position.vPos;
		objP->position.mOrient = initObjP->position.mOrient;
		return initObjP->nSegment;
		}
	}
if (pbFixedPos)
	*pbFixedPos = 0;
d_srand (TimerGetFixedSeconds ());
nCurDropDepth = BASE_NET_DROP_DEPTH + ((d_rand () * BASE_NET_DROP_DEPTH*2) >> 15);
player_pos = &gameData.objs.objects [LOCALPLAYER.nObject].position.vPos;
player_seg = gameData.objs.objects [LOCALPLAYER.nObject].nSegment;
while ((nSegment == -1) && (nCurDropDepth > BASE_NET_DROP_DEPTH/2)) {
	pnum = (d_rand () * gameData.multiplayer.nPlayers) >> 15;
	count = 0;
	while ((count < gameData.multiplayer.nPlayers) && 
				 ((gameData.multiplayer.players [pnum].connected == 0) || (pnum==gameData.multiplayer.nLocalPlayer) || ((gameData.app.nGameMode & (GM_TEAM|GM_CAPTURE|GM_ENTROPY)) && 
				 (GetTeam (pnum)==GetTeam (gameData.multiplayer.nLocalPlayer))))) {
		pnum = (pnum+1)%gameData.multiplayer.nPlayers;
		count++;
		}
	if (count == gameData.multiplayer.nPlayers) {
		//if can't valid non-tPlayer person, use the tPlayer
		pnum = gameData.multiplayer.nLocalPlayer;
		//return (d_rand () * gameData.segs.nLastSegment) >> 15;
		}
	nSegment = PickConnectedSegment (gameData.objs.objects + gameData.multiplayer.players [pnum].nObject, nCurDropDepth);
#if TRACE
	con_printf (CONDBG, " %d", nSegment);
#endif
	if (nSegment == -1) {
		nCurDropDepth--;
		continue;
		}
	special = gameData.segs.segment2s [nSegment].special;
	if ((special == SEGMENT_IS_CONTROLCEN) ||
		 (special == SEGMENT_IS_BLOCKED) ||
		 (special == SEGMENT_IS_SKYBOX) ||
		 (special == SEGMENT_IS_GOAL_BLUE) ||
		 (special == SEGMENT_IS_GOAL_RED) ||
		 (special == SEGMENT_IS_TEAM_BLUE) ||
		 (special == SEGMENT_IS_TEAM_RED))
		nSegment = -1;
	else {	//don't drop in any children of control centers
		int i;
		for (i=0;i<6;i++) {
			int ch = gameData.segs.segments [nSegment].children [i];
			if (IS_CHILD (ch) && gameData.segs.segment2s [ch].special == SEGMENT_IS_CONTROLCEN) {
				nSegment = -1;
				break;
				}
			}
		}
	//bail if not far enough from original position
	if (nSegment != -1) {
		COMPUTE_SEGMENT_CENTER_I (&tempv, nSegment);
		nDist = FindConnectedDistance (player_pos, player_seg, &tempv, nSegment, -1, WID_FLY_FLAG, 0);
		if ((nDist >= 0) && (nDist < i2f (20) * nCurDropDepth)) {
			nSegment = -1;
			}
		}
	nCurDropDepth--;
}
#if TRACE
if (nSegment != -1)
	con_printf (CONDBG, " dist=%x\n", nDist);
#endif
if (nSegment == -1) {
#if TRACE
	con_printf (1, "Warning: Unable to find a connected tSegment.  Picking a random one.\n");
#endif
	return (d_rand () * gameData.segs.nLastSegment) >> 15;
	}
else
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
if (EGI_FLAG (bImmortalPowerups, 0, 0, 0) || 
		 ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))) {
	short	nSegment;
	int h, bFixedPos = 0;
	vmsVector vNewPos;

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
	nObject = CallObjectCreateEgg (gameData.objs.objects + LOCALPLAYER.nObject, 
											 1, OBJ_POWERUP, nPowerupType);
	if (nObject < 0)
		return 0;
	nSegment = ChooseDropSegment (gameData.objs.objects + nObject, &bFixedPos, nDropState);
	if (bFixedPos)
		vNewPos = gameData.objs.objects [nObject].position.vPos;
	else
		PickRandomPointInSeg (&vNewPos, nSegment);
	MultiSendCreatePowerup (nPowerupType, nSegment, nObject, &vNewPos);
	if (!bFixedPos)
		gameData.objs.objects [nObject].position.vPos = vNewPos;
	VmVecZero (&gameData.objs.objects [nObject].mType.physInfo.velocity);
	RelinkObject (nObject, nSegment);
	ObjectCreateExplosion (nSegment, &vNewPos, i2f (5), VCLIP_POWERUP_DISAPPEARANCE);
	return 1;
	}
return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Return true if current tSegment contains some tObject.
int SegmentContainsObject (int objType, int obj_id, int nSegment)
{
	int	nObject;

if (nSegment == -1)
	return 0;
nObject = gameData.segs.segments [nSegment].objects;
while (nObject != -1)
	if ((gameData.objs.objects [nObject].nType == objType) && (gameData.objs.objects [nObject].id == obj_id))
		return 1;
	else
		nObject = gameData.objs.objects [nObject].next;
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
	seg2 = gameData.segs.segments [nSegment].children [i];
	if (seg2 != -1)
		if (ObjectNearbyAux (seg2, objectType, object_id, depth-1))
			return 1;
		}
	return 0;
}


//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
int WeaponNearby (tObject *objP, int weapon_id)
{
	return ObjectNearbyAux (objP->nSegment, OBJ_POWERUP, weapon_id, 3);
}

//	------------------------------------------------------------------------------------------------------

void MaybeReplacePowerupWithEnergy (tObject *delObjP)
{
	int	weapon_index=-1;

	if (delObjP->containsType != OBJ_POWERUP)
		return;

	if (delObjP->containsId == POW_CLOAK) {
		if (WeaponNearby (delObjP, delObjP->containsId)) {
#if TRACE
			con_printf (CONDBG, "Bashing cloak into nothing because there's one nearby.\n");
#endif
			delObjP->containsCount = 0;
		}
		return;
	}
	switch (delObjP->containsId) {
		case POW_VULCAN:		
			weapon_index = VULCAN_INDEX;	
			break;
		case POW_GAUSS:		
			weapon_index = GAUSS_INDEX;	
			break;
		case POW_SPREADFIRE:
			weapon_index = SPREADFIRE_INDEX;
			break;
		case POW_PLASMA:		
			weapon_index = PLASMA_INDEX;	
			break;
		case POW_FUSION:		
			weapon_index = FUSION_INDEX;	
			break;
		case POW_HELIX:		
			weapon_index = HELIX_INDEX;	
			break;
		case POW_PHOENIX:	
			weapon_index = PHOENIX_INDEX;	
			break;
		case POW_OMEGA:		
			weapon_index = OMEGA_INDEX;	
			break;

	}

	//	Don't drop vulcan ammo if tPlayer maxed out.
	if (( (weapon_index == VULCAN_INDEX) || (delObjP->containsId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
		delObjP->containsCount = 0;
	else if (( (weapon_index == GAUSS_INDEX) || (delObjP->containsId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
		delObjP->containsCount = 0;
	else if (weapon_index != -1) {
		if ((PlayerHasWeapon (weapon_index, 0, -1) & HAS_WEAPON_FLAG) || 
			 WeaponNearby (delObjP, delObjP->containsId)) {
			if (d_rand () > 16384) {
				delObjP->containsType = OBJ_POWERUP;
				if (weapon_index == VULCAN_INDEX) {
					delObjP->containsId = POW_VULCAN_AMMO;
				} else if (weapon_index == GAUSS_INDEX) {
					delObjP->containsId = POW_VULCAN_AMMO;
				} else {
					delObjP->containsId = POW_ENERGY;
				}
			} else {
				delObjP->containsType = OBJ_POWERUP;
				delObjP->containsId = POW_SHIELD_BOOST;
			}
		}
	} else if (delObjP->containsId == POW_QUADLASER)
		if ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) || WeaponNearby (delObjP, delObjP->containsId)) {
			if (d_rand () > 16384) {
				delObjP->containsType = OBJ_POWERUP;
				delObjP->containsId = POW_ENERGY;
			} else {
				delObjP->containsType = OBJ_POWERUP;
				delObjP->containsId = POW_SHIELD_BOOST;
			}
		}

	//	If this robot was gated in by the boss and it now contains energy, make it contain nothing, 
	//	else the room gets full of energy.
	if ((delObjP->matCenCreator == BOSS_GATE_MATCEN_NUM) && (delObjP->containsId == POW_ENERGY) && (delObjP->containsType == OBJ_POWERUP)) {
#if TRACE
		con_printf (CONDBG, "Converting energy powerup to nothing because robot %i gated in by boss.\n", OBJ_IDX (delObjP));
#endif
		delObjP->containsCount = 0;
	}

	// Change multiplayer extra-lives into invulnerability
	if ((gameData.app.nGameMode & GM_MULTI) && (delObjP->containsId == POW_EXTRA_LIFE))
	{
		delObjP->containsId = POW_INVUL;
	}
}

//------------------------------------------------------------------------------

int DropPowerup (ubyte nType, ubyte id, short owner, int num, vmsVector *init_vel, vmsVector *pos, short nSegment)
{
	short			nObject=-1;
	tObject		*objP;
	vmsVector	new_velocity, vNewPos;
	fix			old_mag;
   int			count;

switch (nType) {
	case OBJ_POWERUP:
		for (count = 0; count < num; count++) {
			int	rand_scale;
			new_velocity = *init_vel;
			old_mag = VmVecMagQuick (init_vel);

			//	We want powerups to move more in network mode.
			if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
				rand_scale = 4;
				//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
				if (id == POW_EXTRA_LIFE)
					id = POW_INVUL;
				}
			else
				rand_scale = 2;
			new_velocity.p.x += FixMul (old_mag+F1_0*32, d_rand ()*rand_scale - 16384*rand_scale);
			new_velocity.p.y += FixMul (old_mag+F1_0*32, d_rand ()*rand_scale - 16384*rand_scale);
			new_velocity.p.z += FixMul (old_mag+F1_0*32, d_rand ()*rand_scale - 16384*rand_scale);
			// Give keys zero velocity so they can be tracked better in multi
			if ((gameData.app.nGameMode & GM_MULTI) && 
				 (( (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD)) || (id == POW_MONSTERBALL)))
				VmVecZero (&new_velocity);
			vNewPos = *pos;

			if (gameData.app.nGameMode & GM_MULTI) {
				if (gameData.multigame.create.nLoc >= MAX_NET_CREATE_OBJECTS) {
#if TRACE
					con_printf (1, "Not enough slots to drop all powerups!\n");
#endif
					return -1;
					}
#if 0
				if (TooManyPowerups (id)) {
#if TRACE
					con_printf (1, "More powerups of nType %d in mine than at start of game!\n", id);
#endif
					return -1;
					}
#endif				
				if ((gameData.app.nGameMode & GM_NETWORK) && networkData.nStatus == NETSTAT_ENDLEVEL)
					return -1;
				}
			nObject = CreateObject (nType, id, owner, nSegment, &vNewPos, &vmdIdentityMatrix, gameData.objs.pwrUp.info [id].size, 
										  CT_POWERUP, MT_PHYSICS, RT_POWERUP, 0);
			if (nObject < 0) {
#if TRACE
				con_printf (1, "Can't create tObject in ObjectCreateEgg.  Aborting.\n");
#endif
				Int3 ();
				return nObject;
				}
			if (IsMultiGame)
				gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
			objP = gameData.objs.objects + nObject;
			objP->mType.physInfo.velocity = new_velocity;
			objP->mType.physInfo.drag = 512;	//1024;
			objP->mType.physInfo.mass = F1_0;
			objP->mType.physInfo.flags = PF_BOUNCE;
			objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
			objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
			objP->rType.vClipInfo.nCurFrame = 0;

			switch (objP->id) {
				case POW_CONCUSSION_1:
				case POW_CONCUSSION_4:
				case POW_SHIELD_BOOST:
				case POW_ENERGY:
					objP->lifeleft = (d_rand () + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
					if (gameData.app.nGameMode & GM_MULTI)
						objP->lifeleft /= 2;
					break;
				default:
//						if (gameData.app.nGameMode & GM_MULTI)
//							objP->lifeleft = (d_rand () + F1_0*3) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
					break;
				}
			}
		break;

	case OBJ_ROBOT:
		for (count=0; count<num; count++) {
			int	rand_scale;
			new_velocity = *init_vel;
			old_mag = VmVecMagQuick (init_vel);
			VmVecNormalizeQuick (&new_velocity);
			//	We want powerups to move more in network mode.
			rand_scale = 2;
			new_velocity.p.x += (d_rand ()-16384)*2;
			new_velocity.p.y += (d_rand ()-16384)*2;
			new_velocity.p.z += (d_rand ()-16384)*2;
			VmVecNormalizeQuick (&new_velocity);
			VmVecScale (&new_velocity, (F1_0*32 + old_mag) * rand_scale);
			vNewPos = *pos;
			//	This is dangerous, could be outside mine.
//				vNewPos.x += (d_rand ()-16384)*8;
//				vNewPos.y += (d_rand ()-16384)*7;
//				vNewPos.z += (d_rand ()-16384)*6;
			nObject = CreateObject (OBJ_ROBOT, id, -1, nSegment, &vNewPos, &vmdIdentityMatrix, 
										 gameData.models.polyModels [ROBOTINFO (id).nModel].rad, 
										 CT_AI, MT_PHYSICS, RT_POLYOBJ, 1);
			if (nObject < 0) {
#if TRACE
				con_printf (1, "Can't create tObject in ObjectCreateEgg, robots.  Aborting.\n");
#endif
				Int3 ();
				return nObject;
				}
			if (gameData.app.nGameMode & GM_MULTI)
				gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
			objP = &gameData.objs.objects [nObject];
			//Set polygon-tObject-specific data
			objP->rType.polyObjInfo.nModel = ROBOTINFO (objP->id).nModel;
			objP->rType.polyObjInfo.nSubObjFlags = 0;
			//set Physics info
			objP->mType.physInfo.velocity = new_velocity;
			objP->mType.physInfo.mass = ROBOTINFO (objP->id).mass;
			objP->mType.physInfo.drag = ROBOTINFO (objP->id).drag;
			objP->mType.physInfo.flags |= (PF_LEVELLING);
			objP->shields = ROBOTINFO (objP->id).strength;
			objP->cType.aiInfo.behavior = AIB_NORMAL;
			gameData.ai.localInfo [nObject].playerAwarenessType = PA_WEAPON_ROBOT_COLLISION;
			gameData.ai.localInfo [nObject].playerAwarenessTime = F1_0*3;
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
		Error ("Error: Illegal nType (%i) in tObject spawning.\n", nType);
	}
return nObject;
}

// ----------------------------------------------------------------------------
// Returns created tObject number.
// If tObject dropped by tPlayer, set flag.
int ObjectCreateEgg (tObject *objP)
{
	int	rval;

if (!(gameData.app.nGameMode & GM_MULTI) & (objP->nType != OBJ_PLAYER)) {
	if (objP->containsType == OBJ_POWERUP) {
		if (objP->containsId == POW_SHIELD_BOOST) {
			if (LOCALPLAYER.shields >= i2f (100)) {
				if (d_rand () > 16384) {
#if TRACE
					con_printf (CONDBG, "Not dropping shield!\n");
#endif
					return -1;
					}
				} 
			else  if (LOCALPLAYER.shields >= i2f (150)) {
				if (d_rand () > 8192) {
#if TRACE
					con_printf (CONDBG, "Not dropping shield!\n");
#endif
					return -1;
					}
				}
			} 
		else if (objP->containsId == POW_ENERGY) {
			if (LOCALPLAYER.energy >= i2f (100)) {
				if (d_rand () > 16384) {
#if TRACE
					con_printf (CONDBG, "Not dropping energy!\n");
#endif
					return -1;
					}
				} 
			else if (LOCALPLAYER.energy >= i2f (150)) {
				if (d_rand () > 8192) {
#if TRACE
					con_printf (CONDBG, "Not dropping energy!\n");
#endif
					return -1;
					}
				}
			}
		}
	}

rval = DropPowerup (
	objP->containsType, 
	 (ubyte) objP->containsId, 
	 (short) (
		 ((gameData.app.nGameMode & GM_ENTROPY) && 
		 (objP->containsType == OBJ_POWERUP) && 
		 (objP->containsId == POW_ENTROPY_VIRUS) && 
		 (objP->nType == OBJ_PLAYER)) ? 
		GetTeam (objP->id) + 1 : -1
		), 
	objP->containsCount, 
	&objP->mType.physInfo.velocity, 
	&objP->position.vPos, objP->nSegment);
if (rval >= 0) {
	if ((objP->nType == OBJ_PLAYER) && (objP->id == gameData.multiplayer.nLocalPlayer))
		gameData.objs.objects [rval].flags |= OF_PLAYER_DROPPED;
	if (objP->nType == OBJ_ROBOT && objP->containsType==OBJ_POWERUP) {
		if (objP->containsId == POW_VULCAN || objP->containsId == POW_GAUSS)
			gameData.objs.objects [rval].cType.powerupInfo.count = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (objP->containsId == POW_OMEGA)
			gameData.objs.objects [rval].cType.powerupInfo.count = MAX_OMEGA_CHARGE;
		}
	}
return rval;
}

// -- extern int Items_destroyed;

//	-------------------------------------------------------------------------------------------------------
//	Put count gameData.objs.objects of nType nType (eg, powerup), id = id (eg, energy) into *objP, then drop them! Yippee!
//	Returns created tObject number.
int CallObjectCreateEgg (tObject *objP, int count, int nType, int id)
{
if (count <= 0)
	return -1;
objP->containsCount = count;
objP->containsType = nType;
objP->containsId = id;
return ObjectCreateEgg (objP);
}

//------------------------------------------------------------------------------
//creates afterburner blobs behind the specified tObject
void DropAfterburnerBlobs (tObject *objP, int count, fix xSizeScale, fix xLifeTime, 
									tObject *pParent, int bThruster)
{
	short				i, nSegment, nThrusters;
	tObject			*blobObjP;
	tThrusterInfo	ti;

nThrusters = CalcThrusterPos (objP, &ti, 1);
for (i = 0; i < nThrusters; i++) {
	nSegment = FindSegByPoint (ti.vPos + i, objP->nSegment, 1, 0);
	if (nSegment == -1)
		continue;
	if (!(blobObjP = ObjectCreateExplosion (nSegment, ti.vPos + i, xSizeScale, VCLIP_AFTERBURNER_BLOB)))
		continue;
	if (xLifeTime != -1) {
		blobObjP->rType.vClipInfo.xTotalTime = xLifeTime;
		blobObjP->rType.vClipInfo.xFrameTime = FixMulDiv (gameData.eff.vClips [0][VCLIP_AFTERBURNER_BLOB].xFrameTime, 
																		  xLifeTime, blobObjP->lifeleft);
		blobObjP->lifeleft = xLifeTime;
		}
	AddChildObjectP (pParent, blobObjP);
	blobObjP->renderType = RT_THRUSTER;
	if (bThruster)
		blobObjP->mType.physInfo.flags |= PF_WIGGLE;
	}
}

//	------------------------------------------------------------------------------------------------------

int ReturnFlagHome (tObject *objP)
{
	tObject	*initObjP;

if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF) {
	if (gameData.segs.segment2s [objP->nSegment].special == ((objP->id == POW_REDFLAG) ? SEGMENT_IS_GOAL_RED : SEGMENT_IS_GOAL_BLUE))
		return objP->nSegment;
	if ((initObjP = FindInitObject (objP))) {
	//objP->nSegment = initObjP->nSegment;
		objP->position.vPos = initObjP->position.vPos;
		objP->position.mOrient = initObjP->position.mOrient;
		RelinkObject (OBJ_IDX (objP), initObjP->nSegment);
		HUDInitMessage (TXT_FLAG_RETURN);
		DigiPlaySample (SOUND_DROP_WEAPON, F1_0);
		MultiSendReturnFlagHome (OBJ_IDX (objP));
		}
	}
return objP->nSegment;
}

//------------------------------------------------------------------------------
//eof
