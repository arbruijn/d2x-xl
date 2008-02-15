/* $Id: ai2.c,v 1.4 2003/10/04 03:14:47 btb Exp $ */
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
static char rcsid [] = "$Id: ai2.c,v 1.4 2003/10/04 03:14:47 btb Exp $";
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "mono.h"
#include "3d.h"

#include "u_mem.h"
#include "object.h"
#include "render.h"
#include "error.h"
#include "ai.h"
#include "laser.h"
#include "fvi.h"
#include "polyobj.h"
#include "bm.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "player.h"
#include "wall.h"
#include "vclip.h"
#include "digi.h"
#include "fireball.h"
#include "morph.h"
#include "effects.h"
#include "timer.h"
#include "sounds.h"
#include "cntrlcen.h"
#include "multibot.h"
#include "multi.h"
#include "network.h"
#include "loadgame.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
//#define _DEBUG
#ifdef _DEBUG
#include "string.h"
#include <time.h>
#endif

void TeleportBoss (tObject *objP);
int BossFitsInSeg (tObject *bossObjP, int nSegment);

int     Flinch_scale = 4;
int     Attack_scale = 24;
sbyte   Mike_to_matt_xlate [] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use gameData.time.xFrame because robots do not get moved every frame.

#define	MAX_SPEW_BOT		3

int spewBots [2][NUM_D2_BOSSES][MAX_SPEW_BOT] = {
	{
	{ 38, 40, -1},
	{ 37, -1, -1},
	{ 43, 57, -1},
	{ 26, 27, 58},
	{ 59, 58, 54},
	{ 60, 61, 54},
	{ 69, 29, 24},
	{ 72, 60, 73} 
	},
	{
	{  3,  -1, -1},
	{  9,   0, -1},
	{255, 255, -1},
	{ 26,  27, 58},
	{ 59,  58, 54},
	{ 60,  61, 54},
	{ 69,  29, 24},
	{ 72,  60, 73} 
	} 
};

int	maxSpewBots [NUM_D2_BOSSES] = {2, 1, 2, 3, 3, 3,  3, 3};

// ---------------------------------------------------------
//	On entry, gameData.bots.nTypes had darn sure better be set.
//	Mallocs gameData.bots.nTypes tRobotInfo structs into global gameData.bots.pInfo.
void InitAISystem (void)
{
#if 0
	int	i;

#if TRACE
	con_printf (CONDBG, "Trying to D2_ALLOC %i bytes for gameData.bots.pInfo.\n", 
					gameData.bots.nTypes * sizeof (*gameData.bots.pInfo));
#endif
	gameData.bots.pInfo = (tRobotInfo *) D2_ALLOC (gameData.bots.nTypes * sizeof (*gameData.bots.pInfo));
#if TRACE
	con_printf (CONDBG, "gameData.bots.pInfo = %i\n", gameData.bots.pInfo);
#endif
	for (i = 0; i < gameData.bots.nTypes; i++) {
		gameData.bots.pInfo [i].fieldOfView = F1_0/2;
		gameData.bots.pInfo [i].primaryFiringWait = F1_0;
		gameData.bots.pInfo [i].turnTime = F1_0*2;
		// -- gameData.bots.pInfo [i].fire_power = F1_0;
		// -- gameData.bots.pInfo [i].shield = F1_0/2;
		gameData.bots.pInfo [i].xMaxSpeed = F1_0*10;
		gameData.bots.pInfo [i].always_0xabcd = 0xabcd;
	}
#endif

}

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
int AIBehaviorToMode (int behavior)
{
switch (behavior) {
	case AIB_STILL:		
		return AIM_IDLING;
	case AIB_NORMAL:		
		return AIM_CHASE_OBJECT;
	case AIB_BEHIND:		
		return AIM_BEHIND;
	case AIB_RUN_FROM:	
		return AIM_RUN_FROM_OBJECT;
	case AIB_SNIPE:		
		return AIM_IDLING;	//	Changed, 09/13/95, MK, snipers are still until they see you or are hit.
	case AIB_STATION:		
		return AIM_IDLING;
	case AIB_FOLLOW:		
		return AIM_FOLLOW_PATH;
	default:	Int3 ();	//	Contact Mike: Error, illegal behavior nType
	}
return AIM_IDLING;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the tPlayer starts a new ship.
void AIInitBossForShip (void)
{
	int	i;

for (i = 0; i < MAX_BOSS_COUNT; i++)
	gameData.boss [i].nHitTime = -F1_0*10;
}

// ---------------------------------------------------------------------------------------------------------------------
//	initial_mode == -1 means leave mode unchanged.
void InitAIObject (short nObject, short behavior, short nHideSegment)
{
	tObject		*objP = gameData.objs.objects + nObject;
	tAIStatic	*aip = &objP->cType.aiInfo;
	tAILocal		*ailp = gameData.ai.localInfo + nObject;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->id);

Assert (nObject >= 0);
if (behavior == AIB_STATIC) {
	objP->controlType = CT_NONE;
	objP->movementType = MT_NONE;
	}
if (behavior == 0) {
	behavior = AIB_NORMAL;
	aip->behavior = (ubyte) behavior;
	}
//	mode is now set from the Robot dialog, so this should get overwritten.
ailp->mode = AIM_IDLING;
ailp->nPrevVisibility = 0;
if (behavior != -1) {
	aip->behavior = (ubyte) behavior;
	ailp->mode = AIBehaviorToMode (aip->behavior);
	} 
else if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
#if TRACE
	con_printf (CONDBG, " [obj %i -> normal] ", nObject);
#endif
	aip->behavior = AIB_NORMAL;
	}
if (botInfoP->companion) {
	ailp->mode = AIM_GOTO_PLAYER;
	gameData.escort.nKillObject = -1;
	}
if (botInfoP->thief) {
	aip->behavior = AIB_SNIPE;
	ailp->mode = AIM_THIEF_WAIT;
	}
if (botInfoP->attackType) {
	aip->behavior = AIB_NORMAL;
	ailp->mode = AIBehaviorToMode (aip->behavior);
	}
VmVecZero (&objP->mType.physInfo.velocity);
// -- ailp->waitTime = F1_0*5;
ailp->playerAwarenessTime = 0;
ailp->playerAwarenessType = 0;
aip->GOAL_STATE = AIS_SRCH;
aip->CURRENT_STATE = AIS_REST;
ailp->timePlayerSeen = gameData.time.xGame;
ailp->nextMiscSoundTime = gameData.time.xGame;
ailp->timePlayerSoundAttacked = gameData.time.xGame;
if ((behavior == AIB_SNIPE) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM) || (behavior == AIB_FOLLOW)) {
	aip->nHideSegment = nHideSegment;
	ailp->nGoalSegment = nHideSegment;
	aip->nHideIndex = -1;			// This means the path has not yet been created.
	aip->nCurPathIndex = 0;
	}
aip->SKIP_AI_COUNT = 0;
aip->CLOAKED = (botInfoP->cloakType == RI_CLOAKED_ALWAYS);
objP->mType.physInfo.flags |= (PF_BOUNCE | PF_TURNROLL);
aip->REMOTE_OWNER = -1;
aip->bDyingSoundPlaying = 0;
aip->xDyingStartTime = 0;
}


extern tObject * CreateMorphRobot (tSegment *segP, vmsVector *vObjPos, ubyte nObjId);

// --------------------------------------------------------------------------------------------------------------------
//	Create a Buddy bot.
//	This automatically happens when you bring up the Buddy menu in a debug version.
//	It is available as a cheat in a non-debug (release) version.
void CreateBuddyBot (void)
{
	ubyte	buddy_id;
	vmsVector	vObjPos;

for (buddy_id = 0; buddy_id < gameData.bots.nTypes [0]; buddy_id++)
	if (gameData.bots.info [0][buddy_id].companion)
		break;

	if (buddy_id == gameData.bots.nTypes [0]) {
#if TRACE
		con_printf (CONDBG, "Can't create Buddy.  No 'companion' bot found in gameData.bots.pInfo!\n");
#endif
		return;
	}
	COMPUTE_SEGMENT_CENTER_I (&vObjPos, OBJSEG (gameData.objs.console));
	CreateMorphRobot (gameData.segs.segments + OBJSEG (gameData.objs.console), &vObjPos, buddy_id);
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segListP.
//	Set *segCountP.
//	Boss is allowed to teleport to segments he fits in (calls ObjectIntersectsWall) and
//	he can reach from his initial position (calls FindConnectedDistance).
//	If bSizeCheck is set, then only add tSegment if boss can fit in it, else any segment is legal.
//	bOneWallHack added by MK, 10/13/95: A mega-hack! Set to !0 to ignore the 
void InitBossSegments (int objList, short segListP [], short *segCountP, int bSizeCheck, int bOneWallHack)
{
	tSegment		*segP;
	tObject		*bossObjP = gameData.objs.objects + objList;
	vmsVector	vBossHomePos;
	int			nBossHomeSeg;
	int			head, tail, w, childSeg;
	int			nGroup, nSide, nSegments;
	int			seqQueue [QUEUE_SIZE];
	fix			xBossSizeSave;

#ifdef EDITOR
nSelectedSegs = 0;
#endif
//	See if there is a boss.  If not, quick out.
nSegments = 0;
xBossSizeSave = bossObjP->size;
// -- Causes problems!!	-- bossObjP->size = FixMul ((F1_0/4)*3, bossObjP->size);
nBossHomeSeg = bossObjP->nSegment;
vBossHomePos = bossObjP->position.vPos;
nGroup = gameData.segs.xSegments [nBossHomeSeg].group;
head = 
tail = 0;
seqQueue [head++] = nBossHomeSeg;
segListP [nSegments++] = nBossHomeSeg;
#ifdef EDITOR
Selected_segs [nSelectedSegs++] = nBossHomeSeg;
#endif

memset (gameData.render.mine.bVisited, 0, gameData.segs.nSegments);

while (tail != head) {
	segP = gameData.segs.segments + seqQueue [tail++];
	tail &= QUEUE_SIZE-1;
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
		childSeg = segP->children [nSide];
		if (!bOneWallHack) {
			w = WALL_IS_DOORWAY (segP, nSide, NULL);
			if (!(w & WID_FLY_FLAG))
				continue;
			}
		//	If we get here and w == WID_WALL, then we want to process through this tWall, else not.
		if (!IS_CHILD (childSeg))
			continue;
		if (bOneWallHack)
			bOneWallHack--;
		if (gameData.render.mine.bVisited [childSeg])
			continue;
		if (nGroup != gameData.segs.xSegments [childSeg].group)
			continue;
		seqQueue [head++] = childSeg;
		gameData.render.mine.bVisited [childSeg] = 1;
		head &= QUEUE_SIZE - 1;
		if (head > tail) {
			if (head == tail + QUEUE_SIZE-1)
				goto errorExit;	//	queue overflow.  Make it bigger!
			}
		else if (head + QUEUE_SIZE == tail + QUEUE_SIZE - 1)
			goto errorExit;	//	queue overflow.  Make it bigger!
		if (bSizeCheck && !BossFitsInSeg (bossObjP, childSeg))
			continue;
		if (nSegments >= MAX_BOSS_TELEPORT_SEGS - 1)
			goto errorExit;
		segListP [nSegments++] = childSeg;
#ifdef EDITOR
		Selected_segs [nSelectedSegs++] = childSeg;
#endif
		}
	}

errorExit:

bossObjP->size = xBossSizeSave;
bossObjP->position.vPos = vBossHomePos;
RelinkObject (objList, nBossHomeSeg);
*segCountP = nSegments;
}

extern void InitBuddyForLevel (void);

// ---------------------------------------------------------------------------------------------------------------------

void InitBossData (int i, int nObject)
{
if (nObject >= 0)
	gameData.boss [i].nObject = nObject;
else if (gameData.boss [i].nObject < 0)
	return;
InitBossSegments (gameData.boss [i].nObject, gameData.boss [i].gateSegs, &gameData.boss [i].nGateSegs, 0, 0);
InitBossSegments (gameData.boss [i].nObject, gameData.boss [i].teleportSegs, &gameData.boss [i].nTeleportSegs, 1, 0);
if (gameData.boss [i].nTeleportSegs == 1)
	InitBossSegments (gameData.boss [i].nObject, gameData.boss [i].teleportSegs, &gameData.boss [i].nTeleportSegs, 1, 1);
gameData.boss [i].bDyingSoundPlaying = 0;
gameData.boss [i].nDying = 0;
gameData.boss [i].nGateInterval = F1_0 * 4 - gameStates.app.nDifficultyLevel * i2f (2) / 3;
if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) {
	gameData.boss [i].nTeleportInterval = F1_0*10;
	gameData.boss [i].nCloakInterval = F1_0*15;					//	Time between cloaks
	} 
else {
	gameData.boss [i].nTeleportInterval = F1_0*7;
	gameData.boss [i].nCloakInterval = F1_0*10;					//	Time between cloaks
	}
}

// ---------------------------------------------------------------------------------------------------------------------

int AddBoss (int nObject)
{
	int	i = FindBoss (nObject);

if (i >= 0)
	return i;
i = extraGameInfo [0].nBossCount++;
if (i >= MAX_BOSS_COUNT)
	return -1;
InitBossData (i, nObject);
return i;
}

// ---------------------------------------------------------------------------------------------------------------------

void RemoveBoss (int i)
{
extraGameInfo [0].nBossCount--;
if (i < BOSS_COUNT)
	gameData.boss [i] = gameData.boss [BOSS_COUNT];
memset (gameData.boss + BOSS_COUNT, 0, sizeof (gameData.boss [BOSS_COUNT]));
}

// ---------------------------------------------------------------------------------------------------------------------

void InitAIObjects (void)
{
	short		h, i, j;
	tObject	*objP;

	gameData.ai.freePointSegs = gameData.ai.pointSegs;

for (i = 0; i < MAX_BOSS_COUNT; i++) {
	gameData.boss [i].nObject = -1;
#ifdef _DEBUG
//	gameData.boss [i].xPrevShields = -1;
#endif
	}
for (i = j = 0, objP = gameData.objs.objects; i < MAX_OBJECTS; i++, objP++) {
	if (objP->controlType == CT_AI)
		InitAIObject (i, objP->cType.aiInfo.behavior, objP->cType.aiInfo.nHideSegment);
	if ((objP->nType == OBJ_ROBOT) && (ROBOTINFO (objP->id).bossFlag))
		gameData.boss [j++].nObject = i;
	}
for (h = BOSS_COUNT, i = 0; i < h; i++)
	InitBossData (i, -1);
gameData.ai.bInitialized = 1;
AIDoCloakStuff ();
InitBuddyForLevel ();
}

// ---------------------------------------------------------------------------------------------------------------------

int		nDiffSave = 1;
fix		Firing_wait_copy [MAX_ROBOT_TYPES];
fix		Firing_wait2_copy [MAX_ROBOT_TYPES];
sbyte		RapidfireCount_copy [MAX_ROBOT_TYPES];

void DoLunacyOn (void)
{
	int	i;

if (gameStates.app.bLunacy)	//already on
	return;
gameStates.app.bLunacy = 1;
nDiffSave = gameStates.app.nDifficultyLevel;
gameStates.app.nDifficultyLevel = NDL-1;
for (i = 0; i < MAX_ROBOT_TYPES; i++) {
	Firing_wait_copy [i] = gameData.bots.pInfo [i].primaryFiringWait [NDL-1];
	Firing_wait2_copy [i] = gameData.bots.pInfo [i].secondaryFiringWait [NDL-1];
	RapidfireCount_copy [i] = gameData.bots.pInfo [i].nRapidFireCount [NDL-1];
	gameData.bots.pInfo [i].primaryFiringWait [NDL-1] = gameData.bots.pInfo [i].primaryFiringWait [1];
	gameData.bots.pInfo [i].secondaryFiringWait [NDL-1] = gameData.bots.pInfo [i].secondaryFiringWait [1];
	gameData.bots.pInfo [i].nRapidFireCount [NDL-1] = gameData.bots.pInfo [i].nRapidFireCount [1];
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void DoLunacyOff (void)
{
	int	i;

if (!gameStates.app.bLunacy)	//already off
	return;
gameStates.app.bLunacy = 0;
for (i = 0; i < MAX_ROBOT_TYPES; i++) {
	gameData.bots.pInfo [i].primaryFiringWait [NDL-1] = Firing_wait_copy [i];
	gameData.bots.pInfo [i].secondaryFiringWait [NDL-1] = Firing_wait2_copy [i];
	gameData.bots.pInfo [i].nRapidFireCount [NDL-1] = RapidfireCount_copy [i];
	}
gameStates.app.nDifficultyLevel = nDiffSave;
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void SetRotVelAndSaturate (fix *dest, fix delta)
{
if ((delta ^ *dest) < 0) {
	if (abs (delta) < F1_0/8) {
		*dest = delta/4;
		} 
	else
		*dest = delta;
	}
else {
	*dest = delta;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//--debug-- #ifdef _DEBUG
//--debug-- int	Total_turns=0;
//--debug-- int	Prevented_turns=0;
//--debug-- #endif

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14
#define	FIRE_AT_NEARBY_PLAYER_THRESHOLD	 (F1_0 * 40) //(F1_0*40)

void PhysicsTurnTowardsVector (vmsVector *vGoal, tObject *objP, fix rate);

//-------------------------------------------------------------------------------------------

fix AITurnTowardsVector (vmsVector *vGoal, tObject *objP, fix rate)
{
	vmsVector	new_fvec;
	fix			dot;

	//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
if (rate == 0)
	return 0;
if ((objP->id == BABY_SPIDER_ID) && (objP->nType == OBJ_ROBOT)) {
	PhysicsTurnTowardsVector (vGoal, objP, rate);
	return VmVecDot (vGoal, &objP->position.mOrient.fVec);
	}
new_fvec = *vGoal;
dot = VmVecDot (vGoal, &objP->position.mOrient.fVec);
if (!IsMultiGame)
	dot = (fix) (dot / gameStates.gameplay.slowmo [0].fSpeed);
if (dot < (F1_0 - gameData.time.xFrame/2)) {
	fix mag, new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
	if (!IsMultiGame)
		new_scale = (fix) (new_scale / gameStates.gameplay.slowmo [0].fSpeed);
	VmVecScale (&new_fvec, new_scale);
	VmVecInc (&new_fvec, &objP->position.mOrient.fVec);
	mag = VmVecNormalizeQuick (&new_fvec);
	if (mag < F1_0/256) {
#if TRACE
		con_printf (1, "Degenerate vector in AITurnTowardsVector (mag = %7.3f)\n", f2fl (mag));
#endif
		new_fvec = *vGoal;		//	if degenerate vector, go right to goal
		}
	}
if (gameStates.gameplay.seismic.nMagnitude) {
	vmsVector	rand_vec;
	fix			scale;
	MakeRandomVector (&rand_vec);
	scale = FixDiv (2*gameStates.gameplay.seismic.nMagnitude, ROBOTINFO (objP->id).mass);
	VmVecScaleInc (&new_fvec, &rand_vec, scale);
	}
VmVector2Matrix (&objP->position.mOrient, &new_fvec, NULL, &objP->position.mOrient.rVec);
return dot;
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from tObject, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.vBelievedPlayerPos as tPlayer's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int ObjectCanSeePlayer (tObject *objP, vmsVector *pos, fix fieldOfView, vmsVector *vVecToPlayer)
{
	fix			dot;
	tVFIQuery	fq;

	//	Assume that robot's gun tip is in same tSegment as robot's center.
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
fq.p0	= pos;
if ((pos->p.x != objP->position.vPos.p.x) || 
	 (pos->p.y != objP->position.vPos.p.y) || 
	 (pos->p.z != objP->position.vPos.p.z)) {
	short nSegment = FindSegByPoint (pos, objP->nSegment, 1, 0);
	if (nSegment == -1) {
		fq.startSeg = objP->nSegment;
		*pos = objP->position.vPos;
#if TRACE
		con_printf (1, "Object %i, gun is outside mine, moving towards center.\n", OBJ_IDX (objP));
#endif
		MoveTowardsSegmentCenter (objP);
		} 
	else {
		if (nSegment != objP->nSegment)
			objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
		fq.startSeg = nSegment;
		}
	}
else
	fq.startSeg	= objP->nSegment;
fq.p1					= &gameData.ai.vBelievedPlayerPos;
fq.radP0				= 
fq.radP1				= F1_0 / 4;
fq.thisObjNum		= OBJ_IDX (objP);
fq.ignoreObjList	= NULL;
fq.flags				= FQ_TRANSWALL; 
gameData.ai.nHitType = FindVectorIntersection (&fq, &gameData.ai.hitData);
gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitData.hit.nSegment;
if (gameData.ai.nHitType != HIT_NONE)
	return 0;
dot = VmVecDot (vVecToPlayer, &objP->position.mOrient.fVec);
return (dot > fieldOfView - (gameData.ai.nOverallAgitation << 9)) ? 2 : 1;
}

// ------------------------------------------------------------------------------------------------------------------

int AICanFireAtPlayer (tObject *objP, vmsVector *vGun, vmsVector *vPlayer)
{
	tVFIQuery	fq;
	fix			nSize, h;
	short			nModel, ignoreObjs [2] = {OBJ_IDX (gameData.objs.console), -1};

//	Assume that robot's gun tip is in same tSegment as robot's center.
if (!(vGun->p.x || vGun->p.y || vGun->p.z))
	return 0;
if (!extraGameInfo [IsMultiGame].bRobotsHitRobots)
	return 1;
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
if ((vGun->p.x == objP->position.vPos.p.x) && 
	 (vGun->p.y == objP->position.vPos.p.y) && 
	 (vGun->p.z == objP->position.vPos.p.z))
	fq.startSeg	= objP->nSegment;
else {
	short nSegment = FindSegByPoint (vGun, objP->nSegment, 1, 0);
	if (nSegment == -1)
		return -1;
	if (nSegment != objP->nSegment)
		objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
	fq.startSeg = nSegment;
	}
h = VmVecDist (vGun, &objP->position.vPos);
h = VmVecDist (vGun, vPlayer);
nModel = objP->rType.polyObjInfo.nModel;
nSize = objP->size;
objP->rType.polyObjInfo.nModel = -1;	//make sure sphere/hitbox and not hitbox/hitbox collisions get tested
objP->size = F1_0 * 2;						//chose some meaningful small size to simulate a weapon
fq.p0					= vGun;
fq.p1					= vPlayer;
fq.radP0				= 
fq.radP1				= F1_0;
fq.thisObjNum		= OBJ_IDX (objP);
fq.ignoreObjList	= ignoreObjs;
fq.flags				= FQ_CHECK_OBJS | FQ_ANY_OBJECT | FQ_IGNORE_POWERUPS;		//what about trans walls???
gameData.ai.nHitType = FindVectorIntersection (&fq, &gameData.ai.hitData);
#ifdef _DEBUG
if (gameData.ai.nHitType == 0)
	FindVectorIntersection (&fq, &gameData.ai.hitData);
#endif
gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitData.hit.nSegment;
objP->rType.polyObjInfo.nModel = nModel;
objP->size = nSize;
return (gameData.ai.nHitType != HIT_OBJECT);
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
int DoSillyAnimation (tObject *objP)
{
	int				nObject = OBJ_IDX (objP);
	tJointPos 		*jp_list;
	int				robotType, nGun, robotState, num_joint_positions;
	tPolyObjInfo	*polyObjInfo = &objP->rType.polyObjInfo;
	tAIStatic		*aip = &objP->cType.aiInfo;
	int				num_guns, at_goal;
	int				attackType;
	int				flinch_attack_scale = 1;

	Assert (nObject >= 0);
	robotType = objP->id;
	num_guns = ROBOTINFO (robotType).nGuns;
	attackType = ROBOTINFO (robotType).attackType;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goalState, not GOAL_STATE.
	robotState = Mike_to_matt_xlate [aip->GOAL_STATE];
	// previousRobotState = Mike_to_matt_xlate [aip->CURRENT_STATE];

	if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
		flinch_attack_scale = Attack_scale;
	else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
		flinch_attack_scale = Flinch_scale;

	at_goal = 1;
	for (nGun=0; nGun <= num_guns; nGun++) {
		int	joint;

		num_joint_positions = robot_get_animState (&jp_list, robotType, nGun, robotState);

		for (joint=0; joint<num_joint_positions; joint++) {
			fix			delta_angle, delta_2;
			int			jointnum = jp_list [joint].jointnum;
			vmsAngVec	*jp = &jp_list [joint].angles;
			vmsAngVec	*pObjP = &polyObjInfo->animAngles [jointnum];

			if (jointnum >= gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels) {
				Int3 ();		// Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			if (jp->p != pObjP->p) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].p = jp->p;

				delta_angle = jp->p - pObjP->p;
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				gameData.ai.localInfo [nObject].deltaAngles [jointnum].p = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->b != pObjP->b) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].b = jp->b;

				delta_angle = jp->b - pObjP->b;
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				gameData.ai.localInfo [nObject].deltaAngles [jointnum].b = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->h != pObjP->h) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].h = jp->h;

				delta_angle = jp->h - pObjP->h;
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				gameData.ai.localInfo [nObject].deltaAngles [jointnum].h = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//tAIStatic	*aip = &objP->cType.aiInfo;
			tAILocal		*ailp = gameData.ai.localInfo + OBJ_IDX (objP);
			ailp->achievedState [nGun] = ailp->goalState [nGun];
			if (ailp->achievedState [nGun] == AIS_RECO)
				ailp->goalState [nGun] = AIS_FIRE;

			if (ailp->achievedState [nGun] == AIS_FLIN)
				ailp->goalState [nGun] = AIS_LOCK;

		}
	}

	if (at_goal == 1) //num_guns)
		aip->CURRENT_STATE = aip->GOAL_STATE;

	return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-gameData.objs.objects in an tObject towards their goals.
//	Current orientation of tObject is at:	polyObjInfo.animAngles
//	Goal orientation of tObject is at:		aiInfo.goalAngles
//	Delta orientation of tObject is at:		aiInfo.deltaAngles
void AIFrameAnimation (tObject *objP)
{
	int	nObject = OBJ_IDX (objP);
	int	joint;
	int	num_joints;

	num_joints = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels;

	for (joint=1; joint<num_joints; joint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vmsAngVec	*curangp = &objP->rType.polyObjInfo.animAngles [joint];
		vmsAngVec	*goalangp = &gameData.ai.localInfo [nObject].goalAngles [joint];
		vmsAngVec	*deltaangp = &gameData.ai.localInfo [nObject].deltaAngles [joint];

		Assert (nObject >= 0);
		delta_to_goal = goalangp->p - curangp->p;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->p, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->p += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->p = goalangp->p;
		}

		delta_to_goal = goalangp->b - curangp->b;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->b, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->b += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->b = goalangp->b;
		}

		delta_to_goal = goalangp->h - curangp->h;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->h, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->h += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->h = goalangp->h;
		}

	}

}

// ----------------------------------------------------------------------------------
void SetNextFireTime (tObject *objP, tAILocal *ailp, tRobotInfo *botInfoP, int nGun)
{
	//	For guys in snipe mode, they have a 50% shot of getting this shot in D2_FREE.
if ((nGun != 0) || (botInfoP->nSecWeaponType == -1))
	if ((objP->cType.aiInfo.behavior != AIB_SNIPE) || (d_rand () > 16384))
		ailp->nRapidFireCount++;
if (((nGun != 0) || (botInfoP->nSecWeaponType == -1)) && (ailp->nRapidFireCount < botInfoP->nRapidFireCount [gameStates.app.nDifficultyLevel])) {
	ailp->nextPrimaryFire = min (F1_0/8, botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
	}
else {
	if ((botInfoP->nSecWeaponType == -1) || (nGun != 0)) {
		ailp->nextPrimaryFire = botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel];
		if (ailp->nRapidFireCount >= botInfoP->nRapidFireCount [gameStates.app.nDifficultyLevel])
			ailp->nRapidFireCount = 0;
		}
	else
		ailp->nextSecondaryFire = botInfoP->secondaryFiringWait [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the tPlayer, they attack.
//	If tPlayer is cloaked, then robot probably didn't actually collide, deal with that here.
void DoAiRobotHitAttack (tObject *robot, tObject *playerobjP, vmsVector *vCollision)
{
	tAILocal		*ailp = gameData.ai.localInfo + OBJ_IDX (robot);
	tRobotInfo	*botInfoP = &ROBOTINFO (robot->id);

if (!gameStates.app.cheats.bRobotsFiring)
	return;
//	If tPlayer is dead, stop firing.
if (gameData.objs.objects [LOCALPLAYER.nObject].nType == OBJ_GHOST)
	return;
if (botInfoP->attackType != 1)
	return;
if (ailp->nextPrimaryFire > 0)
	return;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
	if (VmVecDistQuick (&OBJPOS (gameData.objs.console)->vPos, &robot->position.vPos) < 
		 robot->size + gameData.objs.console->size + F1_0 * 2) {
		CollidePlayerAndNastyRobot (playerobjP, robot, vCollision);
		if (botInfoP->energyDrain && LOCALPLAYER.energy) {
			LOCALPLAYER.energy -= botInfoP->energyDrain * F1_0;
			if (LOCALPLAYER.energy < 0)
				LOCALPLAYER.energy = 0;
			}
		}
	}
robot->cType.aiInfo.GOAL_STATE = AIS_RECO;
SetNextFireTime (robot, ailp, botInfoP, 1);	//	1 = nGun: 0 is special (uses nextSecondaryFire)
}

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		 (F1_0*4)
#define	MAX_LEAD_DISTANCE	 (F1_0*200)
#define	LEAD_RANGE			 (F1_0/2)

// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit tPlayer given positions, tPlayer vel, elapsed time
inline fix ComputeLeadComponent (fix player_pos, fix robot_pos, fix player_vel, fix elapsedTime)
{
return FixDiv (player_pos - robot_pos, elapsedTime) + player_vel;
}

// --------------------------------------------------------------------------------------------------------------------
//	Lead the tPlayer, returning point to fire at in vFirePoint.
//	Rules:
//		Player not cloaked
//		Player must be moving at a speed >= MIN_LEAD_SPEED
//		Player not farther away than MAX_LEAD_DISTANCE
//		dot (vector_to_player, player_direction) must be in -LEAD_RANGE,LEAD_RANGE
//		if firing a matter weapon, less leading, based on skill level.
int LeadPlayer (tObject *objP, vmsVector *vFirePoint, vmsVector *vBelievedPlayerPos, int nGun, vmsVector *fire_vec)
{
	fix			dot, player_speed, xDistToPlayer, max_weapon_speed, projectedTime;
	vmsVector	player_movement_dir, vVecToPlayer;
	int			nWeaponType;
	tWeaponInfo	*wptr;
	tRobotInfo	*botInfoP;

	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
		return 0;

	player_movement_dir = gameData.objs.console->mType.physInfo.velocity;
	player_speed = VmVecNormalizeQuick (&player_movement_dir);

	if (player_speed < MIN_LEAD_SPEED)
		return 0;

	VmVecSub (&vVecToPlayer, vBelievedPlayerPos, vFirePoint);
	xDistToPlayer = VmVecNormalizeQuick (&vVecToPlayer);
	if (xDistToPlayer > MAX_LEAD_DISTANCE)
		return 0;

	dot = VmVecDot (&vVecToPlayer, &player_movement_dir);

	if ((dot < -LEAD_RANGE) || (dot > LEAD_RANGE))
		return 0;

	//	Looks like it might be worth trying to lead the player.
	botInfoP = &ROBOTINFO (objP->id);
	nWeaponType = botInfoP->nWeaponType;
	if (botInfoP->nSecWeaponType != -1)
		if (nGun == 0)
			nWeaponType = botInfoP->nSecWeaponType;

	wptr = gameData.weapons.info + nWeaponType;
	max_weapon_speed = wptr->speed [gameStates.app.nDifficultyLevel];
	if (max_weapon_speed < F1_0)
		return 0;

	//	Matter weapons:
	//	At Rookie or Trainee, don't lead at all.
	//	At higher skill levels, don't lead as well.  Accomplish this by screwing up max_weapon_speed.
	if (wptr->matter)
	{
		if (gameStates.app.nDifficultyLevel <= 1)
			return 0;
		else
			max_weapon_speed *= (NDL-gameStates.app.nDifficultyLevel);
	}

	projectedTime = FixDiv (xDistToPlayer, max_weapon_speed);

	fire_vec->p.x = ComputeLeadComponent (vBelievedPlayerPos->p.x, vFirePoint->p.x, gameData.objs.console->mType.physInfo.velocity.p.x, projectedTime);
	fire_vec->p.y = ComputeLeadComponent (vBelievedPlayerPos->p.y, vFirePoint->p.y, gameData.objs.console->mType.physInfo.velocity.p.y, projectedTime);
	fire_vec->p.z = ComputeLeadComponent (vBelievedPlayerPos->p.z, vFirePoint->p.z, gameData.objs.console->mType.physInfo.velocity.p.z, projectedTime);

	VmVecNormalizeQuick (fire_vec);

	Assert (VmVecDot (fire_vec, &objP->position.mOrient.fVec) < 3*F1_0/2);

	//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
	if (VmVecDot (fire_vec, &objP->position.mOrient.fVec) < F1_0/2) {
		VmVecInc (fire_vec, &vVecToPlayer);
		VmVecScale (fire_vec, F1_0/2);
		if (VmVecDot (fire_vec, &objP->position.mOrient.fVec) < F1_0/2) {
			return 0;
		}
	}

	return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter gameData.ai.vVecToPlayer is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter gameData.ai.vVecToPlayer should not be necessary.
void AIFireLaserAtPlayer (tObject *objP, vmsVector *vFirePoint, int nGun, vmsVector *vBelievedPlayerPos)
{
	short			nObject = OBJ_IDX (objP);
	tAILocal		*ailp = gameData.ai.localInfo + nObject;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->id);
	vmsVector	fire_vec;
	vmsVector	bpp_diff;
	short			nWeaponType;
	fix			aim, dot;
	int			count, i;

Assert (nObject >= 0);
//	If this robot is only awake because a camera woke it up, don't fire.
if (objP->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
	return;
if (!gameStates.app.cheats.bRobotsFiring)
	return;
if (objP->controlType == CT_MORPH)
	return;
//	If player is exploded, stop firing.
if (gameStates.app.bPlayerExploded)
	return;
if (objP->cType.aiInfo.xDyingStartTime)
	return;		//	No firing while in death roll.
//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
//	If you try to key the boss off objP->cType.aiInfo.xDyingStartTime, it will hose the endlevel stuff.
if (ROBOTINFO (objP->id).bossFlag) {
	i = FindBoss (nObject);
	if (gameData.boss [i].nDyingStartTime)
		return;
	}
//	If tPlayer is cloaked, maybe don't fire based on how long cloaked and randomness.
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
	fix	xCloakTime = gameData.ai.cloakInfo [nObject % MAX_AI_CLOAK_INFO].lastTime;
	if ((gameData.time.xGame - xCloakTime > CLOAK_TIME_MAX/4) &&
		 (d_rand () > FixDiv (gameData.time.xGame - xCloakTime, CLOAK_TIME_MAX)/2)) {
		SetNextFireTime (objP, ailp, botInfoP, nGun);
		return;
		}
	}
//	Handle problem of a robot firing through a tWall because its gun tip is on the other
//	tSide of the tWall than the robot's center.  For speed reasons, we normally only compute
//	the vector from the gun point to the player.  But we need to know whether the gun point
//	is separated from the robot's center by a tWall.  If so, don't fire!
if (objP->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
	//	Well, the gun point is in a different tSegment than the robot's center.
	//	This is almost always ok, but it is not ok if something solid is in between.
	int	nGunSeg = FindSegByPoint (vFirePoint, objP->nSegment, 1, 0);
	//	See if these segments are connected, which should almost always be the case.
	short nConnSide = FindConnectedSide (&gameData.segs.segments [nGunSeg], &gameData.segs.segments [objP->nSegment]);
	if (nConnSide != -1) {
		//	They are connected via nConnSide in tSegment objP->nSegment.
		//	See if they are unobstructed.
		if (!(WALL_IS_DOORWAY (gameData.segs.segments + objP->nSegment, nConnSide, NULL) & WID_FLY_FLAG)) {
			//	Can't fly through, so don't let this bot fire through!
			return;
			}
		}
	else {
		//	Well, they are not directly connected, so use FindVectorIntersection to see if they are unobstructed.
		tVFIQuery	fq;
		tFVIData		hit_data;
		int			fate;

		fq.startSeg			= objP->nSegment;
		fq.p0					= &objP->position.vPos;
		fq.p1					= vFirePoint;
		fq.radP0				= 
		fq.radP1				= 0;
		fq.thisObjNum		= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags				= FQ_TRANSWALL;

		fate = FindVectorIntersection (&fq, &hit_data);
		if (fate != HIT_NONE) {
			Int3 ();		//	This bot's gun is poking through a tWall, so don't fire.
			MoveTowardsSegmentCenter (objP);		//	And decrease chances it will happen again.
			return;
			}
		}
	}
//	Set position to fire at based on difficulty level and robot's aiming ability
aim = FIRE_K*F1_0 - (FIRE_K-1)* (botInfoP->aim << 8);	//	F1_0 in bitmaps.tbl = same as used to be.  Worst is 50% more error.
//	Robots aim more poorly during seismic disturbance.
if (gameStates.gameplay.seismic.nMagnitude) {
	fix temp = F1_0 - abs (gameStates.gameplay.seismic.nMagnitude);
	if (temp < F1_0/2)
		temp = F1_0/2;
	aim = FixMul (aim, temp);
	}
//	Lead the tPlayer half the time.
//	Note that when leading the tPlayer, aim is perfect.  This is probably acceptable since leading is so hacked in.
//	Problem is all robots will lead equally badly.
if (d_rand () < 16384) {
	if (LeadPlayer (objP, vFirePoint, vBelievedPlayerPos, nGun, &fire_vec))		//	Stuff direction to fire at in vFirePoint.
		goto player_led;
}

dot = 0;
count = 0;			//	Don't want to sit in this loop foreverd:\temp\dm_test.
i = (NDL - gameStates.app.nDifficultyLevel - 1) * 4;
while ((count < 4) && (dot < F1_0/4)) {
	bpp_diff.p.x = vBelievedPlayerPos->p.x + FixMul ((d_rand ()-16384) * i, aim);
	bpp_diff.p.y = vBelievedPlayerPos->p.y + FixMul ((d_rand ()-16384) * i, aim);
	bpp_diff.p.z = vBelievedPlayerPos->p.z + FixMul ((d_rand ()-16384) * i, aim);
	VmVecNormalizedDirQuick (&fire_vec, &bpp_diff, vFirePoint);
	dot = VmVecDot (&objP->position.mOrient.fVec, &fire_vec);
	count++;
	}

player_led:

nWeaponType = botInfoP->nWeaponType;
if ((botInfoP->nSecWeaponType != -1) && ((nWeaponType < 0) || !nGun))
	nWeaponType = botInfoP->nSecWeaponType;
if (nWeaponType < 0)
	return;
#if 0
{
static int qqq = 0;
HUDMessage (0, "FireAtPlayer %d", ++qqq);
}
#endif
CreateNewLaserEasy (&fire_vec, vFirePoint, OBJ_IDX (objP), (ubyte) nWeaponType, 1);
if (IsMultiGame) {
	AIMultiSendRobotPos (nObject, -1);
	MultiSendRobotFire (nObject, objP->cType.aiInfo.CURRENT_GUN, &fire_vec);
	}
#if 1
if (++(objP->cType.aiInfo.CURRENT_GUN) >= botInfoP->nGuns) {
	if ((botInfoP->nGuns == 1) || (botInfoP->nSecWeaponType == -1))
		objP->cType.aiInfo.CURRENT_GUN = 0;
	else
		objP->cType.aiInfo.CURRENT_GUN = 1;
	}
#endif
CreateAwarenessEvent (objP, PA_NEARBY_ROBOT_FIRED);
SetNextFireTime (objP, ailp, botInfoP, nGun);
}

// --------------------------------------------------------------------------------------------------------------------
//	vGoalVec must be normalized, or close to it.
//	if bDotBased set, then speed is based on direction of movement relative to heading
void MoveTowardsVector (tObject *objP, vmsVector *vGoalVec, int bDotBased)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed, dot, xMaxSpeed, t, d;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->id);
	vmsVector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards tPlayer as usual.

vel = pptr->velocity;
VmVecNormalizeQuick (&vel);
dot = VmVecDot (&vel, &objP->position.mOrient.fVec);

if (botInfoP->thief)
	dot = (F1_0+dot)/2;

if (bDotBased && (dot < 3*F1_0/4)) {
	//	This funny code is supposed to slow down the robot and move his velocity towards his direction
	//	more quickly than the general code
	t = gameData.time.xFrame * 32;
	pptr->velocity.p.x = pptr->velocity.p.x/2 + FixMul (vGoalVec->p.x, t);
	pptr->velocity.p.y = pptr->velocity.p.y/2 + FixMul (vGoalVec->p.y, t);
	pptr->velocity.p.z = pptr->velocity.p.z/2 + FixMul (vGoalVec->p.z, t);
	} 
else {
	t = gameData.time.xFrame * 64;
	d = (gameStates.app.nDifficultyLevel + 5) / 4;
	pptr->velocity.p.x += FixMul (vGoalVec->p.x, t) * d;
	pptr->velocity.p.y += FixMul (vGoalVec->p.y, t) * d;
	pptr->velocity.p.z += FixMul (vGoalVec->p.z, t) * d;
	}
speed = VmVecMagQuick (&pptr->velocity);
xMaxSpeed = botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel];
//	Green guy attacks twice as fast as he moves away.
if ((botInfoP->attackType == 1) || botInfoP->thief || botInfoP->kamikaze)
	xMaxSpeed *= 2;
if (speed > xMaxSpeed) {
	pptr->velocity.p.x = (pptr->velocity.p.x * 3) / 4;
	pptr->velocity.p.y = (pptr->velocity.p.y * 3) / 4;
	pptr->velocity.p.z = (pptr->velocity.p.z * 3) / 4;
	}
}

// --------------------------------------------------------------------------------------------------------------------

void MoveTowardsPlayer (tObject *objP, vmsVector *vVecToPlayer)
//	gameData.ai.vVecToPlayer must be normalized, or close to it.
{
MoveTowardsVector (objP, vVecToPlayer, 1);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fastFlag == -1 means normal slide about.  fastFlag = 0 means no evasion.
void MoveAroundPlayer (tObject *objP, vmsVector *vVecToPlayer, int fastFlag)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->id);
	int				nObject = OBJ_IDX (objP);
	int				dir;
	int				dir_change;
	fix				ft;
	vmsVector		vEvade;
	int				count=0;

	if (fastFlag == 0)
		return;

	dir_change = 48;
	ft = gameData.time.xFrame;
	if (ft < F1_0/32) {
		dir_change *= 8;
		count += 3;
	} else
		while (ft < F1_0/4) {
			dir_change *= 2;
			ft *= 2;
			count++;
		}

	dir = (gameData.app.nFrameCount + (count+1) * (nObject*8 + nObject*4 + nObject)) & dir_change;
	dir >>= (4+count);

	Assert ((dir >= 0) && (dir <= 3));
	ft = gameData.time.xFrame * 32;
	switch (dir) {
		case 0:
			vEvade.p.x = FixMul (gameData.ai.vVecToPlayer.p.z, ft);
			vEvade.p.y = FixMul (gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.z = FixMul (-gameData.ai.vVecToPlayer.p.x, ft);
			break;
		case 1:
			vEvade.p.x = FixMul (-gameData.ai.vVecToPlayer.p.z, ft);
			vEvade.p.y = FixMul (gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.z = FixMul (gameData.ai.vVecToPlayer.p.x, ft);
			break;
		case 2:
			vEvade.p.x = FixMul (-gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.y = FixMul (gameData.ai.vVecToPlayer.p.x, ft);
			vEvade.p.z = FixMul (gameData.ai.vVecToPlayer.p.z, ft);
			break;
		case 3:
			vEvade.p.x = FixMul (gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.y = FixMul (-gameData.ai.vVecToPlayer.p.x, ft);
			vEvade.p.z = FixMul (gameData.ai.vVecToPlayer.p.z, ft);
			break;
		default:
			Error ("Function MoveAroundPlayer: Bad case.");
	}

	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fastFlag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = VmVecDot (&gameData.ai.vVecToPlayer, &objP->position.mOrient.fVec);
		if ((dot > botInfoP->fieldOfView [gameStates.app.nDifficultyLevel]) && 
			 !(gameData.objs.console->flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			if (botInfoP->strength)
				damage_scale = FixDiv (objP->shields, botInfoP->strength);
			else
				damage_scale = F1_0;
			if (damage_scale > F1_0)
				damage_scale = F1_0;		//	Just in cased:\temp\dm_test.
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in cased:\temp\dm_test.

			VmVecScale (&vEvade, i2f (fastFlag) + damage_scale);
		}
	}

	pptr->velocity.p.x += vEvade.p.x;
	pptr->velocity.p.y += vEvade.p.y;
	pptr->velocity.p.z += vEvade.p.z;

	speed = VmVecMagQuick (&pptr->velocity);
	if ((OBJ_IDX (objP) != 1) && (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel])) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void MoveAwayFromPlayer (tObject *objP, vmsVector *vVecToPlayer, int attackType)
{
	fix				speed;
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->id);
	int				objref;
	fix				ft = gameData.time.xFrame * 16;

	pptr->velocity.p.x -= FixMul (gameData.ai.vVecToPlayer.p.x, ft);
	pptr->velocity.p.y -= FixMul (gameData.ai.vVecToPlayer.p.y, ft);
	pptr->velocity.p.z -= FixMul (gameData.ai.vVecToPlayer.p.z, ft);

	if (attackType) {
		//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
		objref = ((OBJ_IDX (objP)) ^ ((gameData.app.nFrameCount + 3* (OBJ_IDX (objP))) >> 5)) & 3;

		switch (objref) {
			case 0:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.uVec, gameData.time.xFrame << 5);
				break;
			case 1:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.uVec, -gameData.time.xFrame << 5);
				break;
			case 2:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.rVec, gameData.time.xFrame << 5);
				break;
			case 3:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.rVec, -gameData.time.xFrame << 5);
				break;
			default:
				Int3 ();	//	Impossible, bogus value on objref, must be in 0d:\temp\dm_test3
		}
	}


	speed = VmVecMagQuick (&pptr->velocity);

	if (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag bEvadeOnly is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_IDLING).
void AIMoveRelativeToPlayer (tObject *objP, tAILocal *ailp, fix xDistToPlayer, 
									  vmsVector *vVecToPlayer, fix circleDistance, int bEvadeOnly, 
									  int nPlayerVisibility)
{
	tObject		*dObjP;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->id);

	Assert (gameData.ai.nPlayerVisibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((botInfoP->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
if (objP->cType.aiInfo.nDangerLaser != -1) {
	dObjP = &gameData.objs.objects [objP->cType.aiInfo.nDangerLaser];

	if ((dObjP->nType == OBJ_WEAPON) && (dObjP->nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
		fix			dot, xDistToLaser, fieldOfView;
		vmsVector	vVecToLaser, fVecLaser;

		fieldOfView = ROBOTINFO (objP->id).fieldOfView [gameStates.app.nDifficultyLevel];
		VmVecSub (&vVecToLaser, &dObjP->position.vPos, &objP->position.vPos);
		xDistToLaser = VmVecNormalizeQuick (&vVecToLaser);
		dot = VmVecDot (&vVecToLaser, &objP->position.mOrient.fVec);

		if ((dot > fieldOfView) || (botInfoP->companion)) {
			fix			dotLaserRobot;
			vmsVector	vLaserToRobot;

			//	The laser is seen by the robot, see if it might hit the robot.
			//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
			if (dObjP->renderType == RT_POLYOBJ)
				fVecLaser = dObjP->position.mOrient.fVec;
			else {		//	Not a polyobjP, get velocity and normalize.
				fVecLaser = dObjP->mType.physInfo.velocity;	//dObjP->position.mOrient.fVec;
				VmVecNormalizeQuick (&fVecLaser);
				}
			VmVecSub (&vLaserToRobot, &objP->position.vPos, &dObjP->position.vPos);
			VmVecNormalizeQuick (&vLaserToRobot);
			dotLaserRobot = VmVecDot (&fVecLaser, &vLaserToRobot);

			if ((dotLaserRobot > F1_0*7/8) && (xDistToLaser < F1_0*80)) {
				int evadeSpeed = ROBOTINFO (objP->id).evadeSpeed [gameStates.app.nDifficultyLevel];
				gameData.ai.bEvaded = 1;
				MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, evadeSpeed);
				}
			}
		return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-nType guys to keep coming, don't return here after evasion.
	if (!(botInfoP->attackType || botInfoP->thief) && bEvadeOnly)
		return;

	//	If we fall out of above, then no tObject to be avoided.
	objP->cType.aiInfo.nDangerLaser = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
if (botInfoP->attackType == 1)
	if (((ailp->nextPrimaryFire > botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) && (gameData.ai.xDistToPlayer < F1_0*30)) || gameStates.app.bPlayerIsDead)
		//	1/4 of time, move around tPlayer, 3/4 of time, move away from tPlayer
		if (d_rand () < 8192)
			MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, -1);
		else
			MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 1);
	else
		MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
else if (botInfoP->thief)
	MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
else {
	int	objval = ((OBJ_IDX (objP)) & 0x0f) ^ 0x0a;

	//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
	if (botInfoP->kamikaze)
		MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
	else if (gameData.ai.xDistToPlayer < circleDistance)
		MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 0);
	else if ((gameData.ai.xDistToPlayer < (3+objval)*circleDistance/2) && (ailp->nextPrimaryFire > -F1_0))
		MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, -1);
	else
		if ((-ailp->nextPrimaryFire > F1_0 + (objval << 12)) && gameData.ai.nPlayerVisibility)
			//	Usually move away, but sometimes move around player.
			if ((((gameData.time.xGame >> 18) & 0x0f) ^ objval) > 4) 
				MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 0);
			else
				MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, -1);
		else
			MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Compute a somewhat random, normalized vector.
void MakeRandomVector (vmsVector *vec)
{
	vec->p.x = (d_rand () - 16384) | 1;	// make sure we don't create null vector
	vec->p.y = d_rand () - 16384;
	vec->p.z = d_rand () - 16384;

	VmVecNormalizeQuick (vec);
}

#ifdef _DEBUG
void mprintf_animation_info (tObject *objP)
{
	tAIStatic	*aip = &objP->cType.aiInfo;
	tAILocal		*ailp = gameData.ai.localInfo + OBJ_IDX (objP);

	if (!gameData.ai.bInfoEnabled)
		return;
#if TRACE
	con_printf (CONDBG, "Goal = ");
	switch (aip->GOAL_STATE) {
		case AIS_NONE:	con_printf (CONDBG, "NONE ");	break;
		case AIS_REST:	con_printf (CONDBG, "REST ");	break;
		case AIS_SRCH:	con_printf (CONDBG, "SRCH ");	break;
		case AIS_LOCK:	con_printf (CONDBG, "LOCK ");	break;
		case AIS_FLIN:	con_printf (CONDBG, "FLIN ");	break;
		case AIS_FIRE:	con_printf (CONDBG, "FIRE ");	break;
		case AIS_RECO:	con_printf (CONDBG, "RECO ");	break;
		case AIS_ERR_:	con_printf (CONDBG, "ERR_ ");	break;
	}
	con_printf (CONDBG, " Cur = ");
	switch (aip->CURRENT_STATE) {
		case AIS_NONE:	con_printf (CONDBG, "NONE ");	break;
		case AIS_REST:	con_printf (CONDBG, "REST ");	break;
		case AIS_SRCH:	con_printf (CONDBG, "SRCH ");	break;
		case AIS_LOCK:	con_printf (CONDBG, "LOCK ");	break;
		case AIS_FLIN:	con_printf (CONDBG, "FLIN ");	break;
		case AIS_FIRE:	con_printf (CONDBG, "FIRE ");	break;
		case AIS_RECO:	con_printf (CONDBG, "RECO ");	break;
		case AIS_ERR_:	con_printf (CONDBG, "ERR_ ");	break;
	}
	con_printf (CONDBG, " Aware = ");
	switch (ailp->playerAwarenessType) {
		case AIE_FIRE: con_printf (CONDBG, "FIRE "); break;
		case AIE_HITT: con_printf (CONDBG, "HITT "); break;
		case AIE_COLL: con_printf (CONDBG, "COLL "); break;
		case AIE_HURT: con_printf (CONDBG, "HURT "); break;
	}
	con_printf (CONDBG, "Next fire = %6.3f, Time = %6.3f\n", f2fl (ailp->nextPrimaryFire), f2fl (ailp->playerAwarenessTime));
#endif
}
#endif

//	-------------------------------------------------------------------------------------------------------------------

int	nBreakOnObject = -1;

void DoFiringStuff (tObject *objP, int nPlayerVisibility, vmsVector *vVecToPlayer)
{
if ((gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD) || 
	 (gameData.ai.nPlayerVisibility >= 1)) {
	//	Now, if in robot's field of view, lock onto tPlayer
	fix	dot = VmVecDot (&objP->position.mOrient.fVec, &gameData.ai.vVecToPlayer);
	if ((dot >= 7 * F1_0 / 8) || (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
		tAIStatic	*aip = &objP->cType.aiInfo;
		tAILocal		*ailp = gameData.ai.localInfo + OBJ_IDX (objP);

		switch (aip->GOAL_STATE) {
			case AIS_NONE:
			case AIS_REST:
			case AIS_SRCH:
			case AIS_LOCK:
				aip->GOAL_STATE = AIS_FIRE;
				if (ailp->playerAwarenessType <= PA_NEARBY_ROBOT_FIRED) {
					ailp->playerAwarenessType = PA_NEARBY_ROBOT_FIRED;
					ailp->playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
					}
				break;
			}
		} 
	else if (dot >= F1_0/2) {
		tAIStatic	*aip = &objP->cType.aiInfo;
		switch (aip->GOAL_STATE) {
			case AIS_NONE:
			case AIS_REST:
			case AIS_SRCH:
				aip->GOAL_STATE = AIS_LOCK;
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robot gets bumped or hit, he decides to find another hiding place.
void DoAiRobotHit (tObject *objP, int nType)
{
	int	r;

if (objP->controlType != CT_AI)
	return;
if ((nType != PA_WEAPON_ROBOT_COLLISION) && (nType != PA_PLAYER_COLLISION))
	return;
if (objP->cType.aiInfo.behavior == AIB_STILL)
	return;
r = d_rand ();
//	Attack robots (eg, green guy) shouldn't have behavior = still.
//Assert (ROBOTINFO (objP->id).attackType == 0);
//	1/8 time, charge tPlayer, 1/4 time create path, rest of time, do nothing
if (r < 4096) {
	CreatePathToPlayer (objP, 10, 1);
	objP->cType.aiInfo.behavior = AIB_STATION;
	objP->cType.aiInfo.nHideSegment = objP->nSegment;
	gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_CHASE_OBJECT;
	}
else if (r < 4096 + 8192) {
	CreateNSegmentPath (objP, d_rand () / 8192 + 2, -1);
	gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_FOLLOW_PATH;
	}
}

#ifdef _DEBUG
int	bDoAIFlag=1;
int	Cvv_test=0;
int	Cvv_lastTime [MAX_OBJECTS_D2X];
int	Gun_point_hack=0;
#endif

int		RobotSoundVolume=DEFAULT_ROBOT_SOUND_VOLUME;

// --------------------------------------------------------------------------------------------------------------------

inline void LimitPlayerVisibility (fix xMaxVisibleDist, tAILocal *ailp)
{
#if 1
if ((xMaxVisibleDist > 0) && (gameData.ai.xDistToPlayer > xMaxVisibleDist) && (ailp->playerAwarenessType < PA_RETURN_FIRE))
	gameData.ai.nPlayerVisibility = 0;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely ObjectCanSeePlayer would benefit from the
//	information of a normalized gameData.ai.vVecToPlayer.
//	Return tPlayer visibility:
//		0		not visible
//		1		visible, but robot not looking at tPlayer (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		tPlayer is cloaked
//	If the tPlayer is cloaked, set gameData.ai.vVecToPlayer based on time tPlayer cloaked and last uncloaked position.
//	Updates ailp->nPrevVisibility if tPlayer is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to gameData.ai.nPlayerVisibility
void ComputeVisAndVec (tObject *objP, vmsVector *pos, tAILocal *ailp, tRobotInfo *botInfoP, int *flag, 
							  fix xMaxVisibleDist)
{
if (*flag) {
	LimitPlayerVisibility (xMaxVisibleDist, ailp);
	}
else {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix			deltaTime, dist;
		int			cloak_index = (OBJ_IDX (objP)) % MAX_AI_CLOAK_INFO;

		deltaTime = gameData.time.xGame - gameData.ai.cloakInfo [cloak_index].lastTime;
		if (deltaTime > F1_0*2) {
			vmsVector	vRand;

			gameData.ai.cloakInfo [cloak_index].lastTime = gameData.time.xGame;
			MakeRandomVector (&vRand);
			VmVecScaleInc (&gameData.ai.cloakInfo [cloak_index].vLastPos, &vRand, 8*deltaTime);
			}
		dist = VmVecNormalizedDirQuick (&gameData.ai.vVecToPlayer, &gameData.ai.cloakInfo [cloak_index].vLastPos, pos);
		gameData.ai.nPlayerVisibility = ObjectCanSeePlayer (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.ai.vVecToPlayer);
		LimitPlayerVisibility (xMaxVisibleDist, ailp);
#ifdef _DEBUG
		if (gameData.ai.nPlayerVisibility == 2)
			gameData.ai.nPlayerVisibility = gameData.ai.nPlayerVisibility;
#endif
		if ((ailp->nextMiscSoundTime < gameData.time.xGame) && ((ailp->nextPrimaryFire < F1_0) || (ailp->nextSecondaryFire < F1_0)) && (dist < F1_0*20)) {
			ailp->nextMiscSoundTime = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 1;
			DigiLinkSoundToPos (botInfoP->seeSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
			}
		}
	else {
		//	Compute expensive stuff -- gameData.ai.vVecToPlayer and gameData.ai.nPlayerVisibility
		VmVecNormalizedDirQuick (&gameData.ai.vVecToPlayer, &gameData.ai.vBelievedPlayerPos, pos);
		if (!(gameData.ai.vVecToPlayer.p.x || gameData.ai.vVecToPlayer.p.y || gameData.ai.vVecToPlayer.p.z)) {
			gameData.ai.vVecToPlayer.p.x = F1_0;
			}
		gameData.ai.nPlayerVisibility = ObjectCanSeePlayer (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.ai.vVecToPlayer);
		LimitPlayerVisibility (xMaxVisibleDist, ailp);
#ifdef _DEBUG
		if (gameData.ai.nPlayerVisibility == 2)
			gameData.ai.nPlayerVisibility = gameData.ai.nPlayerVisibility;
#endif
		//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
		//	see you without killing frame rate.
		{
			tAIStatic	*aip = &objP->cType.aiInfo;
		if ((gameData.ai.nPlayerVisibility == 2) && (ailp->nPrevVisibility != 2))
			if ((aip->GOAL_STATE == AIS_REST) || (aip->CURRENT_STATE == AIS_REST)) {
				aip->GOAL_STATE = AIS_FIRE;
				aip->CURRENT_STATE = AIS_FIRE;
				}
			}

		if ((ailp->nPrevVisibility != gameData.ai.nPlayerVisibility) && (gameData.ai.nPlayerVisibility == 2)) {
			if (ailp->nPrevVisibility == 0) {
				if (ailp->timePlayerSeen + F1_0/2 < gameData.time.xGame) {
					// -- if (gameStates.app.bPlayerExploded)
					// -- 	DigiLinkSoundToPos (botInfoP->tauntSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
					// -- else
						DigiLinkSoundToPos (botInfoP->seeSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
					ailp->timePlayerSoundAttacked = gameData.time.xGame;
					ailp->nextMiscSoundTime = gameData.time.xGame + F1_0 + d_rand ()*4;
					}
				} 
			else if (ailp->timePlayerSoundAttacked + F1_0/4 < gameData.time.xGame) {
				// -- if (gameStates.app.bPlayerExploded)
				// -- 	DigiLinkSoundToPos (botInfoP->tauntSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
				// -- else
					DigiLinkSoundToPos (botInfoP->attackSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
				ailp->timePlayerSoundAttacked = gameData.time.xGame;
				}
			} 

		if ((gameData.ai.nPlayerVisibility == 2) && (ailp->nextMiscSoundTime < gameData.time.xGame)) {
			ailp->nextMiscSoundTime = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 2;
			// -- if (gameStates.app.bPlayerExploded)
			// -- 	DigiLinkSoundToPos (botInfoP->tauntSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
			// -- else
				DigiLinkSoundToPos (botInfoP->attackSound, objP->nSegment, 0, pos, 0 , RobotSoundVolume);
			}
		ailp->nPrevVisibility = gameData.ai.nPlayerVisibility;
		}

	*flag = 1;

	//	@mk, 09/21/95: If tPlayer view is not obstructed and awareness is at least as high as a nearby collision,
	//	act is if robot is looking at player.
	if (ailp->playerAwarenessType >= PA_NEARBY_ROBOT_FIRED)
		if (gameData.ai.nPlayerVisibility == 1)
			gameData.ai.nPlayerVisibility = 2;
		
	if (gameData.ai.nPlayerVisibility) {
		ailp->timePlayerSeen = gameData.time.xGame;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

fix MoveObjectToLegalPoint (tObject *objP, vmsVector *vGoal)
{
	vmsVector	vGoalDir;
	fix			xDistToGoal;

VmVecSub (&vGoalDir, vGoal, &objP->position.vPos);
xDistToGoal = VmVecNormalizeQuick (&vGoalDir);
VmVecScale (&vGoalDir, objP->size / 2);
VmVecInc (&objP->position.vPos, &vGoalDir);
return xDistToGoal;
}

// --------------------------------------------------------------------------------------------------------------------
//	Move the tObject objP to a spot in which it doesn't intersect a tWall.
//	It might mean moving it outside its current tSegment.
void MoveObjectToLegalSpot (tObject *objP, int bMoveToCenter)
{
	vmsVector	vSegCenter, vOrigPos = objP->position.vPos;
	int			i;
	tSegment		*segP = gameData.segs.segments + objP->nSegment;

if (bMoveToCenter) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->nSegment);
	MoveObjectToLegalPoint (objP, &vSegCenter);
	return;
	}
else {
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
		if (WALL_IS_DOORWAY (segP, (short) i, objP) & WID_FLY_FLAG) {
			COMPUTE_SEGMENT_CENTER_I (&vSegCenter, segP->children [i]);
			MoveObjectToLegalPoint (objP, &vSegCenter);
			if (ObjectIntersectsWall (objP))
				objP->position.vPos = vOrigPos;
			else {
				int nNewSeg = FindSegByPoint (&objP->position.vPos, objP->nSegment, 1, 0);
				if (nNewSeg != -1) {
					RelinkObject (OBJ_IDX (objP), nNewSeg);
					return;
					}
				}
			}
		}
	}

if (ROBOTINFO (objP->id).bossFlag) {
	Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
	TeleportBoss (objP);
	}
	else {
#if TRACE
		con_printf (CONDBG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", OBJ_IDX (objP));
#endif
		ApplyDamageToRobot (objP, objP->shields*2, OBJ_IDX (objP));
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move tObject one tObject radii from current position towards tSegment center.
//	If tSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsPoint (tObject *objP, vmsVector *vGoal, fix xMinDist)
{
	fix			xDistToGoal;
	vmsVector	vGoalDir;

VmVecSub (&vGoalDir, vGoal, &objP->position.vPos);
xDistToGoal = VmVecNormalizeQuick (&vGoalDir);
if (xDistToGoal - objP->size <= xMinDist) {
	//	Center is nearer than the distance we want to move, so move to center.
	if (!xMinDist) {
		objP->position.vPos = *vGoal;
		if (ObjectIntersectsWall (objP))
			MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	xDistToGoal = 0;
	} 
else {
	int	nNewSeg;
	fix	xRemDist = xDistToGoal - xMinDist, 
			xScale = (objP->size < xRemDist) ? objP->size : xRemDist;

	if (xMinDist)
		xScale /= 20;
	//	Move one radius towards center.
	VmVecScale (&vGoalDir, xScale);
	VmVecInc (&objP->position.vPos, &vGoalDir);
	nNewSeg = FindSegByPoint (&objP->position.vPos, objP->nSegment, 1, 0);
	if (nNewSeg == -1) {
		objP->position.vPos = *vGoal;
		MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	}
return xDistToGoal;
}

// --------------------------------------------------------------------------------------------------------------------
//	Move tObject one tObject radii from current position towards tSegment center.
//	If tSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsSegmentCenter (tObject *objP)
{
	vmsVector	vSegCenter;

COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->nSegment);
return MoveTowardsPoint (objP, &vSegCenter, 0);
}

//int	Buddy_got_stuck = 0;

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable nType robot.
//	Brains, avoid robots, companions can open doors.
//	objP == NULL means treat as buddy.
int AIDoorIsOpenable (tObject *objP, tSegment *segP, short nSide)
{
	short nWall;
	tWall	*wallP;

if (!IS_CHILD (segP->children [nSide]))
	return 0;		//trap -2 (exit tSide)
nWall = WallNumP (segP, nSide);
if (!IS_WALL (nWall))		//if there's no door at alld:\temp\dm_test.
	return 1;				//d:\temp\dm_testthen say it can't be opened
	//	The mighty console tObject can open all doors (for purposes of determining paths).
if (objP == gameData.objs.console) {
	if (gameData.walls.walls [nWall].nType == WALL_DOOR)
		return 1;
	}
wallP = gameData.walls.walls + nWall;
if ((objP == NULL) || (ROBOTINFO (objP->id).companion == 1)) {
	int	ailp_mode;

	if (wallP->flags & WALL_BUDDY_PROOF) {
		if ((wallP->nType == WALL_DOOR) && (wallP->state == WALL_DOOR_CLOSED))
			return 0;
		else if (wallP->nType == WALL_CLOSED)
			return 0;
		else if ((wallP->nType == WALL_ILLUSION) && !(wallP->flags & WALL_ILLUSION_OFF))
			return 0;
		}
		
	if (wallP->keys != KEY_NONE) {
		if (wallP->keys == KEY_BLUE)
			return (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY);
		else if (wallP->keys == KEY_GOLD)
			return (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY);
		else if (wallP->keys == KEY_RED)
			return (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY);
		}

	if (wallP->nType == WALL_CLOSED)
		return 0;
	if (wallP->nType != WALL_DOOR) /*&& (wallP->nType != WALL_CLOSED))*/
		return 1;

	//	If Buddy is returning to tPlayer, don't let him think he can get through triggered doors.
	//	It's only valid to think that if the tPlayer is going to get him through.  But if he's
	//	going to the tPlayer, the tPlayer is probably on the opposite tSide.
	if (objP)
		ailp_mode = gameData.ai.localInfo [OBJ_IDX (objP)].mode;
	else if (gameData.escort.nObjNum >= 0)
		ailp_mode = gameData.ai.localInfo [gameData.escort.nObjNum].mode;
	else
		ailp_mode = 0;

	// -- if (Buddy_got_stuck) {
	if (ailp_mode == AIM_GOTO_PLAYER) {
		if ((wallP->nType == WALL_BLASTABLE) && (wallP->state != WALL_BLASTED))
			return 0;
		if (wallP->nType == WALL_CLOSED)
			return 0;
		if (wallP->nType == WALL_DOOR) {
			if ((wallP->flags & WALL_DOOR_LOCKED) && (wallP->state == WALL_DOOR_CLOSED))
				return 0;
			}
		}
		// -- }

	if ((ailp_mode != AIM_GOTO_PLAYER) && (wallP->controllingTrigger != -1)) {
		int	nClip = wallP->nClip;

		if (nClip == -1)
			return 1;
		else if (gameData.walls.pAnims [nClip].flags & WCF_HIDDEN) {
			if (wallP->state == WALL_DOOR_CLOSED)
				return 0;
			else
				return 1;
			} 
		else
			return 1;
		}

	if (wallP->nType == WALL_DOOR)  {
		if (wallP->nType == WALL_BLASTABLE)
			return 1;
		else {
			int	nClip = wallP->nClip;

			if (nClip == -1)
				return 1;
			//	Buddy allowed to go through secret doors to get to player.
			else if ((ailp_mode != AIM_GOTO_PLAYER) && (gameData.walls.pAnims [nClip].flags & WCF_HIDDEN)) {
				if (wallP->state == WALL_DOOR_CLOSED)
					return 0;
				else
					return 1;
				} 
			else
				return 1;
			}
		}
	}
else if ((objP->id == ROBOT_BRAIN) || (objP->cType.aiInfo.behavior == AIB_RUN_FROM) || (objP->cType.aiInfo.behavior == AIB_SNIPE)) {
	if (IS_WALL (nWall)) {
		if ((wallP->nType == WALL_DOOR) && (wallP->keys == KEY_NONE) && !(wallP->flags & WALL_DOOR_LOCKED))
			return 1;
		else if (wallP->keys != KEY_NONE) {	//	Allow bots to open doors to which tPlayer has keys.
			if (wallP->keys & LOCALPLAYER.flags)
				return 1;
			}
		}
	}
return 0;
}

//	-----------------------------------------------------------------------------------------------------------
//	Return tSide of openable door in tSegment, if any.  If none, return -1.
int OpenableDoorsInSegment (short nSegment)
{
	ushort	i;

	if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment))
		return -1;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	nWall = WallNumI (nSegment, i);
		if (IS_WALL (nWall)) {
			tWall	*wallP = gameData.walls.walls + nWall;
			if ((wallP->nType == WALL_DOOR) && 
				 (wallP->keys == KEY_NONE) && 
				 (wallP->state == WALL_DOOR_CLOSED) && 
				 !(wallP->flags & WALL_DOOR_LOCKED) && 
				 !(gameData.walls.pAnims [wallP->nClip].flags & WCF_HIDDEN))
				return i;
		}
	}

	return -1;

}

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Return true if a special tObject (tPlayer or control center) is in this tSegment.
// -- int specialObject_in_seg (int nSegment)
// -- {
// -- 	int	nObject;
// -- 
// -- 	nObject = gameData.segs.segments [nSegment].objects;
// -- 
// -- 	while (nObject != -1) {
// -- 		if ((gameData.objs.objects [nObject].nType == OBJ_PLAYER) || (gameData.objs.objects [nObject].nType == OBJ_REACTOR)) {
// -- 			return 1;
// -- 		} else
// -- 			nObject = gameData.objs.objects [nObject].next;
// -- 	}
// -- 
// -- 	return 0;
// -- }

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Randomly select a tSegment attached to *segP, reachable by flying.
// -- int get_random_child (int nSegment)
// -- {
// -- 	int	nSide;
// -- 	tSegment	*segP = &gameData.segs.segments [nSegment];
// -- 
// -- 	nSide = (rand () * 6) >> 15;
// -- 
// -- 	while (!(WALL_IS_DOORWAY (segP, nSide) & WID_FLY_FLAG))
// -- 		nSide = (rand () * 6) >> 15;
// -- 
// -- 	nSegment = segP->children [nSide];
// -- 
// -- 	return nSegment;
// -- }

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an tObject of size size at pos *pos intersects a (tPlayer or robot or control center) in tSegment *segP.
int CheckObjectObjectIntersection (vmsVector *pos, fix size, tSegment *segP)
{
	int		curobjnum;

	//	If this would intersect with another tObject (only check those in this tSegment), then try to move.
	curobjnum = segP->objects;
	while (curobjnum != -1) {
		tObject *curObjP = &gameData.objs.objects [curobjnum];
		if ((curObjP->nType == OBJ_PLAYER) || (curObjP->nType == OBJ_ROBOT) || (curObjP->nType == OBJ_REACTOR)) {
			if (VmVecDistQuick (pos, &curObjP->position.vPos) < size + curObjP->size)
				return 1;
		}
		curobjnum = curObjP->next;
	}

	return 0;

}

// --------------------------------------------------------------------------------------------------------------------
//	Return nObject if tObject created, else return -1.
//	If pos == NULL, pick random spot in tSegment.
int CreateGatedRobot (tObject *bossObjP, short nSegment, ubyte nObjId, vmsVector *pos)
{
	int			nObject, nTries = 5;
	tObject		*objP;
	tSegment		*segP = gameData.segs.segments + nSegment;
	vmsVector	vObjPos;
	tRobotInfo	*botInfoP = &ROBOTINFO (nObjId);
	int			i, nBoss, count = 0;
	fix			objsize = gameData.models.polyModels [botInfoP->nModel].rad;
	ubyte			default_behavior;

if (gameStates.gameplay.bNoBotAI)
	return -1;
nBoss = FindBoss (OBJ_IDX (bossObjP));
if (nBoss < 0)
	return -1;
if (gameData.time.xGame - gameData.boss [nBoss].nLastGateTime < gameData.boss [nBoss].nGateInterval)
	return -1;
for (i = 0; i <= gameData.objs.nLastObject; i++)
	if ((gameData.objs.objects [i].nType == OBJ_ROBOT) &&
		 (gameData.objs.objects [i].matCenCreator == BOSS_GATE_MATCEN_NUM))
		count++;
if (count > 2 * gameStates.app.nDifficultyLevel + 6) {
	gameData.boss [nBoss].nLastGateTime = gameData.time.xGame - 3 * gameData.boss [nBoss].nGateInterval / 4;
	return -1;
	}
COMPUTE_SEGMENT_CENTER (&vObjPos, segP);
for (;;) {
	if (!pos)
		PickRandomPointInSeg (&vObjPos, SEG_IDX (segP));
	else
		vObjPos = *pos;

	//	See if legal to place tObject here.  If not, move about in tSegment and try again.
	if (CheckObjectObjectIntersection (&vObjPos, objsize, segP)) {
		if (!--nTries) {
			gameData.boss [nBoss].nLastGateTime = gameData.time.xGame - 3 * gameData.boss [nBoss].nGateInterval / 4;
			return -1;
			}
		pos = NULL;
		}
	else 
		break;
	}
nObject = CreateObject (OBJ_ROBOT, nObjId, -1, nSegment, &vObjPos, &vmdIdentityMatrix, objsize, CT_AI, MT_PHYSICS, RT_POLYOBJ, 0);
if (nObject < 0) {
	gameData.boss [nBoss].nLastGateTime = gameData.time.xGame - 3 * gameData.boss [nBoss].nGateInterval / 4;
	return -1;
	} 
// added lifetime increase depending on difficulty level 04/26/06 DM
gameData.multigame.create.nObjNums [0] = nObject; // A convenient global to get nObject back to caller for multiplayer
objP = gameData.objs.objects + nObject;
objP->lifeleft = F1_0 * 30 + F0_5 * (gameStates.app.nDifficultyLevel * 15);	//	Gated in robots only live 30 seconds.
//Set polygon-tObject-specific data
objP->rType.polyObjInfo.nModel = botInfoP->nModel;
objP->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
objP->mType.physInfo.mass = botInfoP->mass;
objP->mType.physInfo.drag = botInfoP->drag;
objP->mType.physInfo.flags |= (PF_LEVELLING);
objP->shields = botInfoP->strength;
objP->matCenCreator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.
default_behavior = ROBOTINFO (objP->id).behavior;
InitAIObject (OBJ_IDX (objP), default_behavior, -1);		//	Note, -1 = tSegment this robot goes to to hide, should probably be something useful
ObjectCreateExplosion (nSegment, &vObjPos, i2f (10), VCLIP_MORPHING_ROBOT);
DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nSegment, 0, &vObjPos, 0 , F1_0);
MorphStart (objP);
gameData.boss [nBoss].nLastGateTime = gameData.time.xGame;
LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
return OBJ_IDX (objP);
}

//	----------------------------------------------------------------------------------------------------------
//	objP points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
int BossSpewRobot (tObject *objP, vmsVector *pos, short objType, int bObjTrigger)
{
	short			nObject, nSegment, maxRobotTypes;
	short			nBossIndex, nBossId = ROBOTINFO (objP->id).bossFlag;
	tRobotInfo	*pri;
	vmsVector	vPos;

if (!bObjTrigger && FindObjTrigger (OBJ_IDX (objP), TT_SPAWN_BOT, -1))
	return -1;
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
Assert ((nBossIndex >= 0) && (nBossIndex < NUM_D2_BOSSES));
nSegment = pos ? FindSegByPoint (pos, objP->nSegment, 1, 0) : objP->nSegment;
if (nSegment == -1) {
#if TRACE
	con_printf (CONDBG, "Tried to spew a bot outside the mine! Aborting!\n");
#endif
	return -1;
	}
if (!pos) {
	pos = &vPos;
	COMPUTE_SEGMENT_CENTER_I (pos, nSegment);
	}
if (objType < 0)
	objType = spewBots [gameStates.app.bD1Mission][nBossIndex][(maxSpewBots [nBossIndex] * d_rand ()) >> 15];
if (objType == 255) {	// spawn an arbitrary robot
	maxRobotTypes = gameData.bots.nTypes [gameStates.app.bD1Mission];
	do {
		objType = d_rand () % maxRobotTypes;
		pri = gameData.bots.info [gameStates.app.bD1Mission] + objType;
		} while (pri->bossFlag ||	//well ... don't spawn another boss, huh? ;)
					pri->companion || //the buddy bot isn't exactly an enemy ... ^_^
					(pri->scoreValue < 700)); //avoid spawning a ... spawn nType bot
	}
nObject = CreateGatedRobot (objP, nSegment, (ubyte) objType, pos);
//	Make spewed robot come tumbling out as if blasted by a flash missile.
if (nObject != -1) {
	tObject	*newObjP = gameData.objs.objects + nObject;
	int		force_val = F1_0 / (gameData.time.xFrame ? gameData.time.xFrame : 1);
	if (force_val) {
		newObjP->cType.aiInfo.SKIP_AI_COUNT += force_val;
		newObjP->mType.physInfo.rotThrust.p.x = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.rotThrust.p.y = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.rotThrust.p.z = ((d_rand () - 16384) * force_val)/16;
		newObjP->mType.physInfo.flags |= PF_USES_THRUST;

		//	Now, give a big initial velocity to get moving away from boss.
		VmVecSub (&newObjP->mType.physInfo.velocity, pos, &objP->position.vPos);
		VmVecNormalizeQuick (&newObjP->mType.physInfo.velocity);
		VmVecScale (&newObjP->mType.physInfo.velocity, F1_0*128);
		}
	}
return nObject;
}

// --------------------------------------------------------------------------------------------------------------------
//	Call this each time the tPlayer starts a new ship.
void InitAIForShip (void)
{
	int	i;

for (i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	gameData.ai.cloakInfo [i].nLastSeg = OBJSEG (gameData.objs.console);
	gameData.ai.cloakInfo [i].vLastPos = OBJPOS (gameData.objs.console)->vPos;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Make tObject objP gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return nObject if robot successfully created, else return -1
int GateInRobot (short nObject, ubyte nType, short nSegment)
{
if (nSegment < 0) {
	int i = FindBoss (nObject);
	if (i < 0)
		return -1;
	nSegment = gameData.boss [i].gateSegs [(d_rand () * gameData.boss [i].nGateSegs) >> 15];
	}
Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));
return CreateGatedRobot (gameData.objs.objects + nObject, nSegment, nType, NULL);
}

// --------------------------------------------------------------------------------------------------------------------

int BossFitsInSeg (tObject *bossObjP, int nSegment)
{
	int			nObject = OBJ_IDX (bossObjP);
	int			nPos;
	vmsVector	vSegCenter, vVertPos;

gameData.collisions.nSegsVisited = 0;
COMPUTE_SEGMENT_CENTER_I (&vSegCenter, nSegment);
for (nPos = 0; nPos < 9; nPos++) {
	if (!nPos)
		bossObjP->position.vPos = vSegCenter;
	else {
		vVertPos = gameData.segs.vertices [gameData.segs.segments [nSegment].verts [nPos-1]];
		VmVecAvg (&bossObjP->position.vPos, &vVertPos, &vSegCenter);
		} 
	RelinkObject (nObject, nSegment);
	if (!ObjectIntersectsWall (bossObjP))
		return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int IsValidTeleportDest (vmsVector *vPos, int nMinDist)
{
	tObject		*objP = gameData.objs.objects;
	int			i;
	vmsVector	vOffs;
	fix			xDist;

for (i = gameData.objs.nLastObject; i; i--, objP++) {
	if ((objP->nType == OBJ_ROBOT) || (objP->nType == OBJ_PLAYER)) {
		xDist = VmVecMag (VmVecSub (&vOffs, vPos, &objP->position.vPos));
		if (xDist > ((nMinDist + objP->size) * 3 / 2))
			return 1;
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

void TeleportBoss (tObject *objP)
{
	short			i, nAttempts = 5, nRandSeg = 0, nRandIndex, nObject = OBJ_IDX (objP);
	vmsVector	vBossDir, vNewPos;

//	Pick a random tSegment from the list of boss-teleportable-to segments.
Assert (nObject >= 0);
i = FindBoss (nObject);
if (i < 0)
	return;
//Assert (gameData.boss [i].nTeleportSegs > 0);
if (gameData.boss [i].nTeleportSegs <= 0)
	return;
do {
	nRandIndex = (d_rand () * gameData.boss [i].nTeleportSegs) >> 15;
	nRandSeg = gameData.boss [i].teleportSegs [nRandIndex];
	Assert ((nRandSeg >= 0) && (nRandSeg <= gameData.segs.nLastSegment));
	if (IsMultiGame)
		MultiSendBossActions (nObject, 1, nRandSeg, 0);
	COMPUTE_SEGMENT_CENTER_I (&vNewPos, nRandSeg);
	if (IsValidTeleportDest (&vNewPos, objP->size))
		break;
	}
	while (--nAttempts);
if (!nAttempts)
	return;
RelinkObject (nObject, nRandSeg);
gameData.boss [i].nLastTeleportTime = gameData.time.xGame;
//	make boss point right at tPlayer
objP->position.vPos = vNewPos;
VmVecSub (&vBossDir, &gameData.objs.objects [LOCALPLAYER.nObject].position.vPos, &vNewPos);
VmVector2Matrix (&objP->position.mOrient, &vBossDir, NULL, NULL);
DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nRandSeg, 0, &objP->position.vPos, 0 , F1_0);
DigiKillSoundLinkedToObject (nObject);
DigiLinkSoundToObject2 (ROBOTINFO (objP->id).seeSound, OBJ_IDX (objP), 1, F1_0, F1_0*512, SOUNDCLASS_ROBOT);	//	F1_0*512 means play twice as loud
//	After a teleport, boss can fire right away.
gameData.ai.localInfo [nObject].nextPrimaryFire = 0;
gameData.ai.localInfo [nObject].nextSecondaryFire = 0;
}

//	----------------------------------------------------------------------

void StartBossDeathSequence (tObject *objP)
{
if (ROBOTINFO (objP->id).bossFlag) {
	int	nObject = OBJ_IDX (objP),
			i = FindBoss (nObject);

	if (i < 0)
		return;
	gameData.boss [i].nDying = nObject;
	gameData.boss [i].nDyingStartTime = gameData.time.xGame;
	}
}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if tObject just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
int DoRobotDyingFrame (tObject *objP, fix StartTime, fix xRollDuration, sbyte *bDyingSoundPlaying, short deathSound, fix xExplScale, fix xSoundScale)
{
	fix	xRollVal, temp;
	fix	xSoundDuration;
	tDigiSound *soundP;

if (!xRollDuration)
	xRollDuration = F1_0/4;

xRollVal = FixDiv (gameData.time.xGame - StartTime, xRollDuration);

FixSinCos (FixMul (xRollVal, xRollVal), &temp, &objP->mType.physInfo.rotVel.p.x);
FixSinCos (xRollVal, &temp, &objP->mType.physInfo.rotVel.p.y);
FixSinCos (xRollVal-F1_0/8, &temp, &objP->mType.physInfo.rotVel.p.z);

objP->mType.physInfo.rotVel.p.x = (gameData.time.xGame - StartTime)/9;
objP->mType.physInfo.rotVel.p.y = (gameData.time.xGame - StartTime)/5;
objP->mType.physInfo.rotVel.p.z = (gameData.time.xGame - StartTime)/7;

if (gameOpts->sound.digiSampleRate) {
	soundP = gameData.pig.sound.pSounds + DigiXlatSound (deathSound);
	xSoundDuration = FixDiv (soundP->nLength [soundP->bHires], gameOpts->sound.digiSampleRate);
	}
else
	xSoundDuration = F1_0;

if (StartTime + xRollDuration - xSoundDuration < gameData.time.xGame) {
	if (!*bDyingSoundPlaying) {
#if TRACE
		con_printf (CONDBG, "Starting death sound!\n");
#endif
		*bDyingSoundPlaying = 1;
		DigiLinkSoundToObject2 (deathSound, OBJ_IDX (objP), 0, xSoundScale, xSoundScale * 256, SOUNDCLASS_ROBOT);	//	F1_0*512 means play twice as loud
		}
	else if (d_rand () < gameData.time.xFrame*16)
		CreateSmallFireballOnObject (objP, (F1_0 + d_rand ()) * (16 * xExplScale/F1_0) / 8, 0);
	}
else if (d_rand () < gameData.time.xFrame * 8)
	CreateSmallFireballOnObject (objP, (F1_0/2 + d_rand ()) * (16 * xExplScale / F1_0) / 8, 1);

return (StartTime + xRollDuration < gameData.time.xGame);
}

//	----------------------------------------------------------------------

void StartRobotDeathSequence (tObject *objP)
{
objP->cType.aiInfo.xDyingStartTime = gameData.time.xGame;
objP->cType.aiInfo.bDyingSoundPlaying = 0;
objP->cType.aiInfo.SKIP_AI_COUNT = 0;

}

//	----------------------------------------------------------------------

void DoBossDyingFrame (tObject *objP)
{
	int	rval, i = FindBoss (OBJ_IDX (objP));

if (i < 0)
	return;
rval = DoRobotDyingFrame (objP, gameData.boss [i].nDyingStartTime, BOSS_DEATH_DURATION, 
								 &gameData.boss [i].bDyingSoundPlaying, 
								 ROBOTINFO (objP->id).deathrollSound, F1_0*4, F1_0*4);
if (rval) {
	RemoveBoss (i);
	DoReactorDestroyedStuff (NULL);
	ExplodeObject (objP, F1_0/4);
	DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512, SOUNDCLASS_EXPLOSION);
	}
}

extern void RecreateThief (tObject *objP);

//	----------------------------------------------------------------------

int DoAnyRobotDyingFrame (tObject *objP)
{
if (objP->cType.aiInfo.xDyingStartTime) {
	int bDeathRoll = ROBOTINFO (objP->id).bDeathRoll;
	int rval = DoRobotDyingFrame (objP, objP->cType.aiInfo.xDyingStartTime, min (bDeathRoll/2+1,6)*F1_0, &objP->cType.aiInfo.bDyingSoundPlaying, ROBOTINFO (objP->id).deathrollSound, bDeathRoll*F1_0/8, bDeathRoll*F1_0/2); 
	if (rval) {
		ExplodeObject (objP, F1_0/4);
		DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512, SOUNDCLASS_EXPLOSION);
		if ((gameData.missions.nCurrentLevel < 0) && (ROBOTINFO (objP->id).thief))
			RecreateThief (objP);
		}
	return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI tObject if it is fairly aware of the player.
//	awarenessLevel is in 0d:\temp\dm_test100.  Larger numbers indicate greater awareness (eg, 99 if firing at tPlayer).
//	In a given frame, might not get called for an tObject, or might be called more than once.
//	The fact that this routine is not called for a given tObject does not mean that tObject is not interested in the player.
//	gameData.objs.objects are moved by physics, so they can move even if not interested in a player.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this tPlayer IS NOT allowed to move this robot.
//		1	this tPlayer IS allowed to move this robot.

int AIMultiplayerAwareness (tObject *objP, int awarenessLevel)
{
if (!IsMultiGame)
	return 1;
if (!awarenessLevel)
	return 0;
return MultiCanRemoveRobot (OBJ_IDX (objP), awarenessLevel);
}

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void DoBossStuff (tObject *objP, int nPlayerVisibility)
{
	int	i, nBossId, nBossIndex, nObject;

nObject = OBJ_IDX (objP);
i = FindBoss (nObject);
if (i < 0)
	return;
nBossId = ROBOTINFO (objP->id).bossFlag;
//	Assert ((nBossId >= BOSS_D2) && (nBossId < BOSS_D2 + NUM_D2_BOSSES));
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
#ifdef _DEBUG
if (objP->shields != gameData.boss [i].xPrevShields) {
#if TRACE
	con_printf (CONDBG, "Boss shields = %7.3f, tObject %i\n", f2fl (objP->shields), OBJ_IDX (objP));
#endif
	gameData.boss [i].xPrevShields = objP->shields;
	}
#endif
	//	New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
if (gameData.boss [i].nLastTeleportTime > gameData.time.xGame)
	gameData.boss [i].nLastTeleportTime = gameData.time.xGame;

if (gameData.boss [i].nLastGateTime > gameData.time.xGame)
	gameData.boss [i].nLastGateTime = gameData.time.xGame;

//	@mk, 10/13/95:  Reason:
//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
//		teleports out of there right away, and blasts tPlayer right after first door.
if (!gameData.ai.nPlayerVisibility && (gameData.time.xGame - gameData.boss [i].nHitTime > F1_0*2))
	return;

if (bossProps [gameStates.app.bD1Mission][nBossIndex].bTeleports) {
	if (objP->cType.aiInfo.CLOAKED == 1) {
		gameData.boss [i].nHitTime = gameData.time.xGame;	//	Keep the cloak:teleport process going.
		if ((gameData.time.xGame - gameData.boss [i].nCloakStartTime > BOSS_CLOAK_DURATION/3) && 
			 (gameData.boss [i].nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION/3) && 
			 (gameData.time.xGame - gameData.boss [i].nLastTeleportTime > gameData.boss [i].nTeleportInterval)) {
			if (AIMultiplayerAwareness (objP, 98)) {
				TeleportBoss (objP);
				if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewBotsTeleport) {
					vmsVector	spewPoint;
					VmVecCopyScale (&spewPoint, &objP->position.mOrient.fVec, objP->size * 2);
					VmVecInc (&spewPoint, &objP->position.vPos);
					if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewMore && (d_rand () > 16384) &&
						 (BossSpewRobot (objP, &spewPoint, -1, 0) != -1))
						gameData.boss [i].nLastGateTime = gameData.time.xGame - gameData.boss [i].nGateInterval - 1;	//	Force allowing spew of another bot.
					BossSpewRobot (objP, &spewPoint, -1, 0);
					}
				}
			} 
		else if (gameData.time.xGame - gameData.boss [i].nHitTime > F1_0*2) {
			gameData.boss [i].nLastTeleportTime -= gameData.boss [i].nTeleportInterval/4;
			}
	if (!gameData.boss [i].nCloakDuration)
		gameData.boss [i].nCloakDuration = BOSS_CLOAK_DURATION;
	if ((gameData.time.xGame > gameData.boss [i].nCloakEndTime) || 
			(gameData.time.xGame < gameData.boss [i].nCloakStartTime))
		objP->cType.aiInfo.CLOAKED = 0;
		}
	else if ((gameData.time.xGame - gameData.boss [i].nCloakEndTime > gameData.boss [i].nCloakInterval) || 
				(gameData.time.xGame - gameData.boss [i].nCloakEndTime < -gameData.boss [i].nCloakDuration)) {
		if (AIMultiplayerAwareness (objP, 95)) {
			gameData.boss [i].nCloakStartTime = gameData.time.xGame;
			gameData.boss [i].nCloakEndTime = gameData.time.xGame + gameData.boss [i].nCloakDuration;
			objP->cType.aiInfo.CLOAKED = 1;
			if (IsMultiGame)
				MultiSendBossActions (OBJ_IDX (objP), 2, 0, 0);
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

#define	BOSS_TO_PLAYER_GATE_DISTANCE	 (F1_0*200)

void AIMultiSendRobotPos (short nObject, int force)
{
if (IsMultiGame)
	MultiSendRobotPosition (nObject, force != -1);
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this tObject should be allowed to fire at the player.
int AIMaybeDoActualFiringStuff (tObject *objP, tAIStatic *aip)
{
if (IsMultiGame &&
	 (aip->GOAL_STATE != AIS_FLIN) && (objP->id != ROBOT_BRAIN) &&
	 (aip->CURRENT_STATE == AIS_FIRE))
	return 1;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if tPlayer is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
void AIDoActualFiringStuff (tObject *objP, tAIStatic *aip, tAILocal *ailp, tRobotInfo *botInfoP, int nGun)
{
	fix	dot;

if ((gameData.ai.nPlayerVisibility == 2) || 
	 (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
	vmsVector vFirePos = gameData.ai.vBelievedPlayerPos;

		//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
		//	So, fire at gameData.ai.vLastPlayerPosFiredAt instead of the tPlayer position.
	if (!botInfoP->attackType && (gameData.ai.nPlayerVisibility != 2))
		vFirePos = gameData.ai.vLastPlayerPosFiredAt;

		//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
		//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
		if (!gameData.ai.bObjAnimates || ReadyToFire (botInfoP, ailp)) {
			dot = VmVecDot (&objP->position.mOrient.fVec, &gameData.ai.vVecToPlayer);
			if ((dot >= 7 * F1_0 / 8) || ((dot > F1_0 / 4) &&  botInfoP->bossFlag)) {
				if (nGun < botInfoP->nGuns) {
					if (botInfoP->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (gameData.ai.xDistToPlayer < objP->size + gameData.objs.console->size + F1_0*2)) {		// botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAiRobotHitAttack (objP, gameData.objs.console, &objP->position.vPos);
							}
						else 
							return;
						}
					else {
#if 1
						if (AICanFireAtPlayer (objP, &gameData.ai.vGunPoint, &vFirePos)) {
#else
						if (gameData.ai.vGunPoint.p.x || gameData.ai.vGunPoint.p.y || gameData.ai.vGunPoint.p.z) {
#endif
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailp->nextPrimaryFire <= 0) {
									AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &vFirePos);
									gameData.ai.vLastPlayerPosFiredAt = vFirePos;
									}
								if ((ailp->nextSecondaryFire <= 0) && (botInfoP->nSecWeaponType != -1)) {
									CalcGunPoint (&gameData.ai.vGunPoint, objP, 0);
									AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, 0, &vFirePos);
									gameData.ai.vLastPlayerPosFiredAt = vFirePos;
									}
								}
							else if (ailp->nextPrimaryFire <= 0) {
								AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &vFirePos);
								gameData.ai.vLastPlayerPosFiredAt = vFirePos;
								}
							}
						}

					//	Wants to fire, so should go into chase mode, probably.
					if ((aip->behavior != AIB_RUN_FROM)
						 && (aip->behavior != AIB_STILL)
						 && (aip->behavior != AIB_SNIPE)
						 && (aip->behavior != AIB_FOLLOW)
						 && !botInfoP->attackType
						 && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_IDLING)))
						ailp->mode = AIM_CHASE_OBJECT;
					}

				aip->GOAL_STATE = AIS_RECO;
				ailp->goalState [aip->CURRENT_GUN] = AIS_RECO;
#if 0
				// Switch to next gun for next fire.  If has 2 gun types, select gun #1, if exists.
				if (++(aip->CURRENT_GUN) >= botInfoP->nGuns) {
					if ((botInfoP->nGuns == 1) || (botInfoP->nSecWeaponType == -1))
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
					}
#endif
				}
			}
		}
	else if (((!botInfoP->attackType) &&
			   (gameData.weapons.info [botInfoP->nWeaponType].homingFlag == 1)) || 
			  (((botInfoP->nSecWeaponType != -1) && 
				(gameData.weapons.info [botInfoP->nSecWeaponType].homingFlag == 1)))) {
		fix dist;
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if ((!gameData.ai.bObjAnimates || (ailp->achievedState [aip->CURRENT_GUN] == AIS_FIRE))
			 && (((ailp->nextPrimaryFire <= 0) && (aip->CURRENT_GUN != 0)) || 
				  ((ailp->nextSecondaryFire <= 0) && (aip->CURRENT_GUN == 0)))
			 && ((dist = VmVecDistQuick (&gameData.ai.vHitPos, &objP->position.vPos)) > F1_0*40)) {
			if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
				return;
			AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &gameData.ai.vBelievedPlayerPos);
			aip->GOAL_STATE = AIS_RECO;
			ailp->goalState [aip->CURRENT_GUN] = AIS_RECO;
			// Switch to next gun for next fire.
			if (++(aip->CURRENT_GUN) >= botInfoP->nGuns)
				aip->CURRENT_GUN = 0;
			}
		else {
			// Switch to next gun for next fire.
			if (++(aip->CURRENT_GUN) >= botInfoP->nGuns)
				aip->CURRENT_GUN = 0;
			}
		}
	else {	//	---------------------------------------------------------------
		vmsVector	vec_to_last_pos;

		if (d_rand ()/2 < FixMul (gameData.time.xFrame, (gameStates.app.nDifficultyLevel << 12) + 0x4000)) {
		if ((!gameData.ai.bObjAnimates || ReadyToFire (botInfoP, ailp)) && (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			VmVecNormalizedDirQuick (&vec_to_last_pos, &gameData.ai.vBelievedPlayerPos, &objP->position.vPos);
			dot = VmVecDot (&objP->position.mOrient.fVec, &vec_to_last_pos);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < botInfoP->nGuns) {
					if (botInfoP->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (gameData.ai.xDistToPlayer < objP->size + gameData.objs.console->size + F1_0*2)) {		// botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAiRobotHitAttack (objP, gameData.objs.console, &objP->position.vPos);
							}
						else
							return;
						}
					else {
						if ((&gameData.ai.vGunPoint.p.x == 0) && (&gameData.ai.vGunPoint.p.y == 0) && (&gameData.ai.vGunPoint.p.z == 0))
							; 
						else {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailp->nextPrimaryFire <= 0)
									AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &gameData.ai.vLastPlayerPosFiredAt);

								if ((ailp->nextSecondaryFire <= 0) && (botInfoP->nSecWeaponType != -1)) {
									CalcGunPoint (&gameData.ai.vGunPoint, objP, 0);
									AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, 0, &gameData.ai.vLastPlayerPosFiredAt);
									}
								} 
							else if (ailp->nextPrimaryFire <= 0)
								AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &gameData.ai.vLastPlayerPosFiredAt);
							}
						}
					//	Wants to fire, so should go into chase mode, probably.
					if ((aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_IDLING)))
						ailp->mode = AIM_CHASE_OBJECT;
					}
				aip->GOAL_STATE = AIS_RECO;
				ailp->goalState [aip->CURRENT_GUN] = AIS_RECO;
				// Switch to next gun for next fire.
				if (++(aip->CURRENT_GUN) >= botInfoP->nGuns) {
					if (botInfoP->nGuns == 1)
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
					}
				}
			}
		}
	}
}

//	---------------------------------------------------------------
// eof

