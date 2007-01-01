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
#include "gameseq.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif

#ifndef NDEBUG
#include "string.h"
#include <time.h>
#endif

void teleport_boss (tObject *objP);
int boss_fits_in_seg (tObject *bossObjP, int nSegment);

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
	con_printf (CON_DEBUG, "Trying to d_malloc %i bytes for gameData.bots.pInfo.\n", gameData.bots.nTypes * sizeof (*gameData.bots.pInfo));
#endif
	gameData.bots.pInfo = (tRobotInfo *) d_malloc (gameData.bots.nTypes * sizeof (*gameData.bots.pInfo));
#if TRACE	
	con_printf (CON_DEBUG, "gameData.bots.pInfo = %i\n", gameData.bots.pInfo);
#endif
	for (i=0; i<gameData.bots.nTypes; i++) {
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
		case AIB_IDLING:			return AIM_IDLING;
		case AIB_NORMAL:			return AIM_CHASE_OBJECT;
		case AIB_BEHIND:			return AIM_BEHIND;
		case AIB_RUN_FROM:		return AIM_RUN_FROM_OBJECT;
		case AIB_SNIPE:			return AIM_IDLING;	//	Changed, 09/13/95, MK, snipers are still until they see you or are hit.
		case AIB_STATION:			return AIM_IDLING;
		case AIB_FOLLOW:			return AIM_FOLLOW_PATH;
		default:	Int3 ();	//	Contact Mike: Error, illegal behavior nType
	}

	return AIM_IDLING;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the tPlayer starts a new ship.
void AIInitBossForShip (void)
{
	gameData.boss.nHitTime = -F1_0*10;

}

// ---------------------------------------------------------------------------------------------------------------------
//	initial_mode == -1 means leave mode unchanged.
void InitAIObject (short nObject, short behavior, short nHideSegment)
{
	tObject		*objP = gameData.objs.objects + nObject;
	tAIStatic	*aip = &objP->cType.aiInfo;
	tAILocal		*ailp = &gameData.ai.localInfo [nObject];
	tRobotInfo	*robptr = &gameData.bots.pInfo [objP->id];

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
	con_printf (CON_DEBUG, " [obj %i -> normal] ", nObject);
#endif
	aip->behavior = AIB_NORMAL;
	}
if (robptr->companion) {
	ailp->mode = AIM_GOTO_PLAYER;
	gameData.escort.nKillObject = -1;
	}
if (robptr->thief) {
	aip->behavior = AIB_SNIPE;
	ailp->mode = AIM_THIEF_WAIT;
	}
if (robptr->attackType) {
	aip->behavior = AIB_NORMAL;
	ailp->mode = AIBehaviorToMode (aip->behavior);
	}
// This is astonishingly stupid! This routine gets called by matcens!KILL KILL KILL!!!gameData.ai.freePointSegs = gameData.ai.pointSegs;
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
aip->CLOAKED =  (robptr->cloakType == RI_CLOAKED_ALWAYS);
objP->mType.physInfo.flags |= (PF_BOUNCE | PF_TURNROLL);
aip->REMOTE_OWNER = -1;
aip->bDyingSoundPlaying = 0;
aip->xDyingStartTime = 0;
}


extern tObject * CreateMorphRobot (tSegment *segP, vmsVector *object_pos, ubyte object_id);

// --------------------------------------------------------------------------------------------------------------------
//	Create a Buddy bot.
//	This automatically happens when you bring up the Buddy menu in a debug version.
//	It is available as a cheat in a non-debug (release) version.
void CreateBuddyBot (void)
{
	ubyte	buddy_id;
	vmsVector	object_pos;

	for (buddy_id=0; buddy_id<gameData.bots.nTypes [0]; buddy_id++)
		if (gameData.bots.info [0][buddy_id].companion)
			break;

	if (buddy_id == gameData.bots.nTypes [0]) {
#if TRACE	
		con_printf (CON_DEBUG, "Can't create Buddy.  No 'companion' bot found in gameData.bots.pInfo!\n");
#endif
		return;
	}
	COMPUTE_SEGMENT_CENTER_I (&object_pos, gameData.objs.console->position.nSegment);
	CreateMorphRobot (gameData.segs.segments + gameData.objs.console->position.nSegment, &object_pos, buddy_id);
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segptr.
//	Set *num_segs.
//	Boss is allowed to teleport to segments he fits in (calls ObjectIntersectsWall) and
//	he can reach from his initial position (calls FindConnectedDistance).
//	If size_check is set, then only add tSegment if boss can fit in it, else any tSegment is legal.
//	one_wall_hack added by MK, 10/13/95: A mega-hack! Set to !0 to ignore the 
void InitBossSegments (int objList, short segptr [], short *num_segs, int size_check, int one_wall_hack)
{
#ifdef EDITOR
	N_selected_segs = 0;
#endif


if (size_check)
#if TRACE	
	con_printf (CON_DEBUG, "Boss fits in segments:\n");
#endif
	//	See if there is a boss.  If not, quick out.
	*num_segs = 0;
	{
		int			original_boss_seg;
		vmsVector	original_boss_pos;
		tObject		*bossObjP = gameData.objs.objects + objList;
		int			head, tail;
		int			seg_queue [QUEUE_SIZE];
		//ALREADY IN RENDER.H sbyte   bVisited [MAX_SEGMENTS];
		fix			boss_size_save;
		int			nGroup;

		boss_size_save = bossObjP->size;
		// -- Causes problems!!	-- bossObjP->size = FixMul ((F1_0/4)*3, bossObjP->size);
		original_boss_seg = bossObjP->position.nSegment;
		original_boss_pos = bossObjP->position.vPos;
		nGroup = gameData.segs.xSegments [original_boss_seg].group;
		head = 0;
		tail = 0;
		seg_queue [head++] = original_boss_seg;

		segptr [ (*num_segs)++] = original_boss_seg;
#if TRACE	
		con_printf (CON_DEBUG, "%4i ", original_boss_seg);
#endif
		#ifdef EDITOR
		Selected_segs [N_selected_segs++] = original_boss_seg;
		#endif

		memset (bVisited, 0, gameData.segs.nSegments);

		while (tail != head) {
			short		nSide;
			tSegment	*segP = gameData.segs.segments + seg_queue [tail++];

			tail &= QUEUE_SIZE-1;

			for (nSide=0; nSide<MAX_SIDES_PER_SEGMENT; nSide++) {
				int	w, childSeg = segP->children [nSide];

				if (( (w = WALL_IS_DOORWAY (segP, nSide, NULL)) & WID_FLY_FLAG) || one_wall_hack) {
					//	If we get here and w == WID_WALL, then we want to process through this wall, else not.
					if (IS_CHILD (childSeg)) {
						if (one_wall_hack)
							one_wall_hack--;
					} else
						continue;

					if ((nGroup == gameData.segs.xSegments [childSeg].group) && (bVisited [childSeg] == 0)) {
						seg_queue [head++] = childSeg;
						bVisited [childSeg] = 1;
						head &= QUEUE_SIZE-1;
						if (head > tail) {
							if (head == tail + QUEUE_SIZE-1)
								Int3 ();	//	queue overflow.  Make it bigger!
						} else
							if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
								Int3 ();	//	queue overflow.  Make it bigger!
	
						if ((!size_check) || boss_fits_in_seg (bossObjP, childSeg)) {
							segptr [ (*num_segs)++] = childSeg;
							if (size_check) {
#if TRACE	
								con_printf (CON_DEBUG, "%4i ", childSeg);
#endif
							}								
							#ifdef EDITOR
							Selected_segs [N_selected_segs++] = childSeg;
							#endif
							if (*num_segs >= MAX_BOSS_TELEPORT_SEGS) {
#if TRACE	
								con_printf (1, "Warning: Too many boss teleport segments.  Found %i after searching %i/%i segments.\n", MAX_BOSS_TELEPORT_SEGS, childSeg, gameData.segs.nLastSegment+1);
#endif
								tail = head;
							}
						}
					}
				}
			}

		}

		bossObjP->size = boss_size_save;
		bossObjP->position.vPos = original_boss_pos;
		RelinkObject (objList, original_boss_seg);

	}

}

extern void InitBuddyForLevel (void);

// ---------------------------------------------------------------------------------------------------------------------
void InitAIObjects (void)
{
	short		i, j;
	tObject	*objP;

	gameData.ai.freePointSegs = gameData.ai.pointSegs;

	memset (gameData.boss.objList, 0xff, sizeof (gameData.boss.objList));
	for (i=j=0, objP = gameData.objs.objects; i<MAX_OBJECTS; i++, objP++) {
		if (objP->controlType == CT_AI)
			InitAIObject (i, objP->cType.aiInfo.behavior, objP->cType.aiInfo.nHideSegment);
		if ((objP->nType == OBJ_ROBOT) && (gameData.bots.pInfo [objP->id].bossFlag)) {
			if (j)		//	There are two bosses in this mine! i and gameData.boss.objList!
				Int3 ();			//do int3 here instead of assert so museum will work
			gameData.boss.objList [j++] = i;
		}
	}

	for (i = 0; i < extraGameInfo [0].nBossCount; i++) {
		if (gameData.boss.objList [i] < 0)
			continue;
		InitBossSegments (gameData.boss.objList [i], gameData.boss.gateSegs [i], &gameData.boss.nGateSegs [i], 0, 0);
		InitBossSegments (gameData.boss.objList [i], gameData.boss.teleportSegs [i], &gameData.boss.nTeleportSegs [i], 1, 0);
		if (gameData.boss.nTeleportSegs [i] == 1)
			InitBossSegments (gameData.boss.objList [i], gameData.boss.teleportSegs [i], &gameData.boss.nTeleportSegs [i], 1, 1);
		}

	gameData.boss.bDyingSoundPlaying = 0;
	gameData.boss.nDying = 0;
	// -- unused!MK, 10/21/95 -- Boss_been_hit = 0;
	gameData.boss.nGateInterval = F1_0*4 - gameStates.app.nDifficultyLevel*i2f (2)/3;

	gameData.ai.bInitialized = 1;

	AIDoCloakStuff ();

	InitBuddyForLevel ();

	if (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) {
		gameData.boss.nTeleportInterval = F1_0*10;
		gameData.boss.nCloakInterval = F1_0*15;					//	Time between cloaks
	} else {
		gameData.boss.nTeleportInterval = F1_0*7;
		gameData.boss.nCloakInterval = F1_0*10;					//	Time between cloaks
	}
}

int	nDiffSave = 1;

fix     Firing_wait_copy [MAX_ROBOT_TYPES];
fix     Firing_wait2_copy [MAX_ROBOT_TYPES];
sbyte   Rapidfire_count_copy [MAX_ROBOT_TYPES];

void DoLunacyOn (void)
{
	int	i;

	if (gameStates.app.bLunacy)	//already on
		return;

	gameStates.app.bLunacy = 1;

	nDiffSave = gameStates.app.nDifficultyLevel;
	gameStates.app.nDifficultyLevel = NDL-1;

	for (i=0; i<MAX_ROBOT_TYPES; i++) {
		Firing_wait_copy [i] = gameData.bots.pInfo [i].primaryFiringWait [NDL-1];
		Firing_wait2_copy [i] = gameData.bots.pInfo [i].secondaryFiringWait [NDL-1];
		Rapidfire_count_copy [i] = gameData.bots.pInfo [i].nRapidFireCount [NDL-1];

		gameData.bots.pInfo [i].primaryFiringWait [NDL-1] = gameData.bots.pInfo [i].primaryFiringWait [1];
		gameData.bots.pInfo [i].secondaryFiringWait [NDL-1] = gameData.bots.pInfo [i].secondaryFiringWait [1];
		gameData.bots.pInfo [i].nRapidFireCount [NDL-1] = gameData.bots.pInfo [i].nRapidFireCount [1];
	}

}

void DoLunacyOff (void)
{
	int	i;

	if (!gameStates.app.bLunacy)	//already off
		return;

	gameStates.app.bLunacy = 0;

	for (i=0; i<MAX_ROBOT_TYPES; i++) {
		gameData.bots.pInfo [i].primaryFiringWait [NDL-1] = Firing_wait_copy [i];
		gameData.bots.pInfo [i].secondaryFiringWait [NDL-1] = Firing_wait2_copy [i];
		gameData.bots.pInfo [i].nRapidFireCount [NDL-1] = Rapidfire_count_copy [i];
	}

	gameStates.app.nDifficultyLevel = nDiffSave;
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void set_rotvel_and_saturate (fix *dest, fix delta)
{
	if ((delta ^ *dest) < 0) {
		if (abs (delta) < F1_0/8) {
			*dest = delta/4;
		} else
			*dest = delta;
	} else {
		*dest = delta;
	}
}

//--debug-- #ifndef NDEBUG
//--debug-- int	Total_turns=0;
//--debug-- int	Prevented_turns=0;
//--debug-- #endif

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14
#define	FIRE_AT_NEARBY_PLAYER_THRESHOLD	 (F1_0*40)

extern void PhysicsTurnTowardsVector (vmsVector *vGoal, tObject *objP, fix rate);

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
if (dot < (F1_0 - gameData.time.xFrame/2)) {
	fix	mag;
	fix	new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
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
	scale = FixDiv (2*gameStates.gameplay.seismic.nMagnitude, gameData.bots.pInfo [objP->id].mass);
	VmVecScaleInc (&new_fvec, &rand_vec, scale);
	}
VmVector2Matrix (&objP->position.mOrient, &new_fvec, NULL, &objP->position.mOrient.rVec);
return dot;
}

// -- unused, 08/07/95 -- // --------------------------------------------------------------------------------------------------------------------
// -- unused, 08/07/95 -- void ai_turn_randomly (vmsVector *vec_to_player, tObject *objP, fix rate, int nPrevVisibility)
// -- unused, 08/07/95 -- {
// -- unused, 08/07/95 -- 	vmsVector	curvec;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- // -- MK, 06/09/95	//	Random turning looks too stupid, so 1/4 of time, cheat.
// -- unused, 08/07/95 -- // -- MK, 06/09/95	if (nPrevVisibility)
// -- unused, 08/07/95 -- // -- MK, 06/09/95		if (d_rand () > 0x7400) {
// -- unused, 08/07/95 -- // -- MK, 06/09/95			AITurnTowardsVector (vec_to_player, objP, rate);
// -- unused, 08/07/95 -- // -- MK, 06/09/95			return;
// -- unused, 08/07/95 -- // -- MK, 06/09/95		}
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- 	curvec = objP->mType.physInfo.rotVel;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- 	curvec.y += F1_0/64;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- 	curvec.x += curvec.y/6;
// -- unused, 08/07/95 -- 	curvec.y += curvec.z/4;
// -- unused, 08/07/95 -- 	curvec.z += curvec.x/10;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- 	if (abs (curvec.x) > F1_0/8) curvec.x /= 4;
// -- unused, 08/07/95 -- 	if (abs (curvec.y) > F1_0/8) curvec.y /= 4;
// -- unused, 08/07/95 -- 	if (abs (curvec.z) > F1_0/8) curvec.z /= 4;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- 	objP->mType.physInfo.rotVel = curvec;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- }

//	gameData.ai.nOverallAgitation affects:
//		Widens field of view.  Field of view is in range 0d:\temp\dm_test1 (specified in bitmaps.tbl as N/360 degrees).
//			gameData.ai.nOverallAgitation/128 subtracted from field of view, making robots see wider.
//		Increases distance to which robot will search to create path to tPlayer by gameData.ai.nOverallAgitation/8 segments.
//		Decreases wait between fire times by gameData.ai.nOverallAgitation/64 seconds.


// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from tObject, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.vBelievedPlayerPos as tPlayer's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int ObjectCanSeePlayer (tObject *objP, vmsVector *pos, fix fieldOfView, vmsVector *vec_to_player)
{
	fix			dot;
	fvi_query	fq;

	//	Assume that robot's gun tip is in same tSegment as robot's center.
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
fq.p0	= pos;
if ((pos->p.x != objP->position.vPos.p.x) || 
	 (pos->p.y != objP->position.vPos.p.y) || 
	 (pos->p.z != objP->position.vPos.p.z)) {
	short nSegment = FindSegByPoint (pos, objP->position.nSegment);
	if (nSegment == -1) {
		fq.startSeg = objP->position.nSegment;
		*pos = objP->position.vPos;
#if TRACE	
		con_printf (1, "Object %i, gun is outside mine, moving towards center.\n", OBJ_IDX (objP));
#endif
		MoveTowardsSegmentCenter (objP);
		} 
	else {
		if (nSegment != objP->position.nSegment)
			objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
		fq.startSeg = nSegment;
		}
	}
else
	fq.startSeg	= objP->position.nSegment;
fq.p1					= &gameData.ai.vBelievedPlayerPos;
fq.rad				= F1_0/4;
fq.thisObjNum		= OBJ_IDX (objP);
fq.ignoreObjList	= NULL;
fq.flags				= FQ_TRANSWALL; // -- Why were we checking gameData.objs.objects? | FQ_CHECK_OBJS;		//what about trans walls???
gameData.ai.nHitType = FindVectorIntersection (&fq,&gameData.ai.hitData);
gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitData.hit.nSegment;
// -- when we stupidly checked gameData.objs.objects -- if ((gameData.ai.nHitType == HIT_NONE) || ((gameData.ai.nHitType == HIT_OBJECT) && (gameData.ai.hitData.hitObject == gameData.multi.players [gameData.multi.nLocalPlayer].nObject))) {
if (gameData.ai.nHitType != HIT_NONE)
	return 0;
dot = VmVecDot (vec_to_player, &objP->position.mOrient.fVec);
return (dot > fieldOfView - (gameData.ai.nOverallAgitation << 9)) ? 2 : 1;
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
int DoSillyAnimation (tObject *objP)
{
	int				nObject = OBJ_IDX (objP);
	tJointPos 		*jp_list;
	int				robotType, nGun, robot_state, num_joint_positions;
	tPolyObjInfo	*polyObjInfo = &objP->rType.polyObjInfo;
	tAIStatic		*aip = &objP->cType.aiInfo;
	int				num_guns, at_goal;
	int				attackType;
	int				flinch_attack_scale = 1;

	robotType = objP->id;
	num_guns = gameData.bots.pInfo [robotType].nGuns;
	attackType = gameData.bots.pInfo [robotType].attackType;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goalState, not GOAL_STATE.
	robot_state = Mike_to_matt_xlate [aip->GOAL_STATE];
	// previousRobot_state = Mike_to_matt_xlate [aip->CURRENT_STATE];

	if (attackType) // && ((robot_state == AS_FIRE) || (robot_state == AS_RECOIL)))
		flinch_attack_scale = Attack_scale;
	else if ((robot_state == AS_FLINCH) || (robot_state == AS_RECOIL))
		flinch_attack_scale = Flinch_scale;

	at_goal = 1;
	for (nGun=0; nGun <= num_guns; nGun++) {
		int	joint;

		num_joint_positions = robot_get_anim_state (&jp_list, robotType, nGun, robot_state);

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
			tAILocal		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
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
void ai_frame_animation (tObject *objP)
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

		delta_to_goal = goalangp->p - curangp->p;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->p, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->p += scaled_delta_angle;
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
			curangp->b += scaled_delta_angle;
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
			curangp->h += scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->h = goalangp->h;
		}

	}

}

// ----------------------------------------------------------------------------------
void SetNextFireTime (tObject *objP, tAILocal *ailp, tRobotInfo *robptr, int nGun)
{
	//	For guys in snipe mode, they have a 50% shot of getting this shot in d_free.
	if ((nGun != 0) || (robptr->nSecWeaponType == -1))
		if ((objP->cType.aiInfo.behavior != AIB_SNIPE) || (d_rand () > 16384))
			ailp->nRapidFireCount++;

	//	Old way, 10/15/95: Continuous rapidfire if nRapidFireCount set.
// -- 	if (( (robptr->nSecWeaponType == -1) || (nGun != 0)) && (ailp->nRapidFireCount < robptr->nRapidFireCount [gameStates.app.nDifficultyLevel])) {
// -- 		ailp->nextPrimaryFire = min (F1_0/8, robptr->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
// -- 	} else {
// -- 		if ((robptr->nSecWeaponType == -1) || (nGun != 0)) {
// -- 			ailp->nRapidFireCount = 0;
// -- 			ailp->nextPrimaryFire = robptr->primaryFiringWait [gameStates.app.nDifficultyLevel];
// -- 		} else
// -- 			ailp->nextSecondaryFire = robptr->secondaryFiringWait [gameStates.app.nDifficultyLevel];
// -- 	}

	if (( (nGun != 0) || (robptr->nSecWeaponType == -1)) && (ailp->nRapidFireCount < robptr->nRapidFireCount [gameStates.app.nDifficultyLevel])) {
		ailp->nextPrimaryFire = min (F1_0/8, robptr->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
	} else {
		if ((robptr->nSecWeaponType == -1) || (nGun != 0)) {
			ailp->nextPrimaryFire = robptr->primaryFiringWait [gameStates.app.nDifficultyLevel];
			if (ailp->nRapidFireCount >= robptr->nRapidFireCount [gameStates.app.nDifficultyLevel])
				ailp->nRapidFireCount = 0;
		} else
			ailp->nextSecondaryFire = robptr->secondaryFiringWait [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the tPlayer, they attack.
//	If tPlayer is cloaked, then robot probably didn't actually collide, deal with that here.
void DoAiRobotHitAttack (tObject *robot, tObject *playerobjP, vmsVector *collision_point)
{
	tAILocal		*ailp = &gameData.ai.localInfo [OBJ_IDX (robot)];
	tRobotInfo *robptr = &gameData.bots.pInfo [robot->id];

//#ifndef NDEBUG
	if (!gameStates.app.cheats.bRobotsFiring)
		return;
//#endif

	//	If tPlayer is dead, stop firing.
	if (gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].nType == OBJ_GHOST)
		return;

	if (robptr->attackType == 1) {
		if (ailp->nextPrimaryFire <= 0) {
			if (!(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED))
				if (VmVecDistQuick (&gameData.objs.console->position.vPos, &robot->position.vPos) < robot->size + gameData.objs.console->size + F1_0*2) {
					CollidePlayerAndNastyRobot (playerobjP, robot, collision_point);
					if (robptr->energyDrain && gameData.multi.players [gameData.multi.nLocalPlayer].energy) {
						gameData.multi.players [gameData.multi.nLocalPlayer].energy -= robptr->energyDrain * F1_0;
						if (gameData.multi.players [gameData.multi.nLocalPlayer].energy < 0)
							gameData.multi.players [gameData.multi.nLocalPlayer].energy = 0;
						// -- unused, use clawSound in bitmaps.tbl -- DigiLinkSoundToPos (SOUND_ROBOT_SUCKED_PLAYER, playerobjP->nSegment, 0, collision_point, 0, F1_0);
					}
				}

			robot->cType.aiInfo.GOAL_STATE = AIS_RECO;
			SetNextFireTime (robot, ailp, robptr, 1);	//	1 = nGun: 0 is special (uses nextSecondaryFire)
		}
	}

}

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		 (F1_0*4)
#define	MAX_LEAD_DISTANCE	 (F1_0*200)
#define	LEAD_RANGE			 (F1_0/2)

// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit tPlayer given positions, tPlayer vel, elapsed time
fix compute_lead_component (fix player_pos, fix robot_pos, fix player_vel, fix elapsedTime)
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
int lead_player (tObject *objP, vmsVector *vFirePoint, vmsVector *vBelievedPlayerPos, int nGun, vmsVector *fire_vec)
{
	fix			dot, player_speed, dist_to_player, max_weapon_speed, projectedTime;
	vmsVector	player_movement_dir, vec_to_player;
	int			nWeaponType;
	tWeaponInfo	*wptr;
	tRobotInfo	*robptr;

	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)
		return 0;

	player_movement_dir = gameData.objs.console->mType.physInfo.velocity;
	player_speed = VmVecNormalizeQuick (&player_movement_dir);

	if (player_speed < MIN_LEAD_SPEED)
		return 0;

	VmVecSub (&vec_to_player, vBelievedPlayerPos, vFirePoint);
	dist_to_player = VmVecNormalizeQuick (&vec_to_player);
	if (dist_to_player > MAX_LEAD_DISTANCE)
		return 0;

	dot = VmVecDot (&vec_to_player, &player_movement_dir);

	if ((dot < -LEAD_RANGE) || (dot > LEAD_RANGE))
		return 0;

	//	Looks like it might be worth trying to lead the player.
	robptr = &gameData.bots.pInfo [objP->id];
	nWeaponType = robptr->nWeaponType;
	if (robptr->nSecWeaponType != -1)
		if (nGun == 0)
			nWeaponType = robptr->nSecWeaponType;

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

	projectedTime = FixDiv (dist_to_player, max_weapon_speed);

	fire_vec->p.x = compute_lead_component (vBelievedPlayerPos->p.x, vFirePoint->p.x, gameData.objs.console->mType.physInfo.velocity.p.x, projectedTime);
	fire_vec->p.y = compute_lead_component (vBelievedPlayerPos->p.y, vFirePoint->p.y, gameData.objs.console->mType.physInfo.velocity.p.y, projectedTime);
	fire_vec->p.z = compute_lead_component (vBelievedPlayerPos->p.z, vFirePoint->p.z, gameData.objs.console->mType.physInfo.velocity.p.z, projectedTime);

	VmVecNormalizeQuick (fire_vec);

	Assert (VmVecDot (fire_vec, &objP->position.mOrient.fVec) < 3*F1_0/2);

	//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
	if (VmVecDot (fire_vec, &objP->position.mOrient.fVec) < F1_0/2) {
		VmVecInc (fire_vec, &vec_to_player);
		VmVecScale (fire_vec, F1_0/2);
		if (VmVecDot (fire_vec, &objP->position.mOrient.fVec) < F1_0/2) {
			return 0;
		}
	}

	return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter vec_to_player is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
void AIFireLaserAtPlayer (tObject *objP, vmsVector *vFirePoint, int nGun, vmsVector *vBelievedPlayerPos)
{
	short			nObject = OBJ_IDX (objP);
	tAILocal		*ailp = gameData.ai.localInfo + nObject;
	tRobotInfo	*robptr = gameData.bots.pInfo + objP->id;
	vmsVector	fire_vec;
	vmsVector	bpp_diff;
	short			nWeaponType;
	fix			aim, dot;
	int			count;

//	If this robot is only awake because a camera woke it up, don't fire.
if (objP->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
	return;
if (!gameStates.app.cheats.bRobotsFiring)
	return;
if (objP->controlType == CT_MORPH)
	return;
//	If tPlayer is exploded, stop firing.
if (gameStates.app.bPlayerExploded)
	return;
if (objP->cType.aiInfo.xDyingStartTime)
	return;		//	No firing while in death roll.
//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
//	If you try to key the boss off objP->cType.aiInfo.xDyingStartTime, it will hose the endlevel stuff.
if (gameData.boss.nDyingStartTime & gameData.bots.pInfo [objP->id].bossFlag)
	return;
//	If tPlayer is cloaked, maybe don't fire based on how long cloaked and randomness.
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
	fix	xCloakTime = gameData.ai.cloakInfo [nObject % MAX_AI_CLOAK_INFO].lastTime;
	if ((gameData.time.xGame - xCloakTime > CLOAK_TIME_MAX/4) &&
		 (d_rand () > FixDiv (gameData.time.xGame - xCloakTime, CLOAK_TIME_MAX)/2)) {
		SetNextFireTime (objP, ailp, robptr, nGun);
		return;
		}
	}
//	Handle problem of a robot firing through a wall because its gun tip is on the other
//	tSide of the wall than the robot's center.  For speed reasons, we normally only compute
//	the vector from the gun point to the player.  But we need to know whether the gun point
//	is separated from the robot's center by a wall.  If so, don't fire!
if (objP->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
	//	Well, the gun point is in a different tSegment than the robot's center.
	//	This is almost always ok, but it is not ok if something solid is in between.
	int	nGunSeg = FindSegByPoint (vFirePoint, objP->position.nSegment);
	//	See if these segments are connected, which should almost always be the case.
	short nConnSide = FindConnectedSide (&gameData.segs.segments [nGunSeg], &gameData.segs.segments [objP->position.nSegment]);
	if (nConnSide != -1) {
		//	They are connected via nConnSide in tSegment objP->position.nSegment.
		//	See if they are unobstructed.
		if (!(WALL_IS_DOORWAY (gameData.segs.segments + objP->position.nSegment, nConnSide, NULL) & WID_FLY_FLAG)) {
			//	Can't fly through, so don't let this bot fire through!
			return;
			}
		}
	else {
		//	Well, they are not directly connected, so use FindVectorIntersection to see if they are unobstructed.
		fvi_query	fq;
		fvi_info		hit_data;
		int			fate;

		fq.startSeg				= objP->position.nSegment;
		fq.p0						= &objP->position.vPos;
		fq.p1						= vFirePoint;
		fq.rad					= 0;
		fq.thisObjNum			= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags					= FQ_TRANSWALL;

		fate = FindVectorIntersection (&fq, &hit_data);
		if (fate != HIT_NONE) {
			Int3 ();		//	This bot's gun is poking through a wall, so don't fire.
			MoveTowardsSegmentCenter (objP);		//	And decrease chances it will happen again.
			return;
			}
		}
	}
//	Set position to fire at based on difficulty level and robot's aiming ability
aim = FIRE_K*F1_0 - (FIRE_K-1)* (robptr->aim << 8);	//	F1_0 in bitmaps.tbl = same as used to be.  Worst is 50% more error.
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
	if (lead_player (objP, vFirePoint, vBelievedPlayerPos, nGun, &fire_vec))		//	Stuff direction to fire at in vFirePoint.
		goto player_led;
}

dot = 0;
count = 0;			//	Don't want to sit in this loop foreverd:\temp\dm_test.
while ((count < 4) && (dot < F1_0/4)) {
	bpp_diff.p.x = vBelievedPlayerPos->p.x + FixMul ((d_rand ()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4, aim);
	bpp_diff.p.y = vBelievedPlayerPos->p.y + FixMul ((d_rand ()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4, aim);
	bpp_diff.p.z = vBelievedPlayerPos->p.z + FixMul ((d_rand ()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4, aim);
	VmVecNormalizedDirQuick (&fire_vec, &bpp_diff, vFirePoint);
	dot = VmVecDot (&objP->position.mOrient.fVec, &fire_vec);
	count++;
	}

player_led:

nWeaponType = robptr->nWeaponType;
if ((robptr->nSecWeaponType != -1) && ((nWeaponType < 0) || !nGun))
	nWeaponType = robptr->nSecWeaponType;
if (nWeaponType < 0)
	return;
CreateNewLaserEasy (&fire_vec, vFirePoint, OBJ_IDX (objP), (ubyte) nWeaponType, 1);
if (gameData.app.nGameMode & GM_MULTI) {
	ai_multi_sendRobot_position (nObject, -1);
	MultiSendRobotFire (nObject, objP->cType.aiInfo.CURRENT_GUN, &fire_vec);
	}
CreateAwarenessEvent (objP, PA_NEARBY_ROBOT_FIRED);
SetNextFireTime (objP, ailp, robptr, nGun);
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
//	if dot_based set, then speed is based on direction of movement relative to heading
void move_towards_vector (tObject *objP, vmsVector *vec_goal, int dot_based)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed, dot, xMaxSpeed;
	tRobotInfo		*robptr = &gameData.bots.pInfo [objP->id];
	vmsVector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards tPlayer as usual.

	vel = pptr->velocity;
	VmVecNormalizeQuick (&vel);
	dot = VmVecDot (&vel, &objP->position.mOrient.fVec);

	if (robptr->thief)
		dot = (F1_0+dot)/2;

	if (dot_based && (dot < 3*F1_0/4)) {
		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
		//	more quickly than the general code
		pptr->velocity.p.x = pptr->velocity.p.x/2 + FixMul (vec_goal->p.x, gameData.time.xFrame*32);
		pptr->velocity.p.y = pptr->velocity.p.y/2 + FixMul (vec_goal->p.y, gameData.time.xFrame*32);
		pptr->velocity.p.z = pptr->velocity.p.z/2 + FixMul (vec_goal->p.z, gameData.time.xFrame*32);
	} else {
		pptr->velocity.p.x += FixMul (vec_goal->p.x, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		pptr->velocity.p.y += FixMul (vec_goal->p.y, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		pptr->velocity.p.z += FixMul (vec_goal->p.z, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
	}

	speed = VmVecMagQuick (&pptr->velocity);
	xMaxSpeed = robptr->xMaxSpeed [gameStates.app.nDifficultyLevel];

	//	Green guy attacks twice as fast as he moves away.
	if ((robptr->attackType == 1) || robptr->thief || robptr->kamikaze)
		xMaxSpeed *= 2;

	if (speed > xMaxSpeed) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_towards_player (tObject *objP, vmsVector *vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
move_towards_vector (objP, vec_to_player, 1);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fastFlag == -1 means normal slide about.  fastFlag = 0 means no evasion.
void move_around_player (tObject *objP, vmsVector *vec_to_player, int fastFlag)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed;
	tRobotInfo		*robptr = &gameData.bots.pInfo [objP->id];
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

	switch (dir) {
		case 0:
			vEvade.p.x = FixMul (vec_to_player->p.z, gameData.time.xFrame*32);
			vEvade.p.y = FixMul (vec_to_player->p.y, gameData.time.xFrame*32);
			vEvade.p.z = FixMul (-vec_to_player->p.x, gameData.time.xFrame*32);
			break;
		case 1:
			vEvade.p.x = FixMul (-vec_to_player->p.z, gameData.time.xFrame*32);
			vEvade.p.y = FixMul (vec_to_player->p.y, gameData.time.xFrame*32);
			vEvade.p.z = FixMul (vec_to_player->p.x, gameData.time.xFrame*32);
			break;
		case 2:
			vEvade.p.x = FixMul (-vec_to_player->p.y, gameData.time.xFrame*32);
			vEvade.p.y = FixMul (vec_to_player->p.x, gameData.time.xFrame*32);
			vEvade.p.z = FixMul (vec_to_player->p.z, gameData.time.xFrame*32);
			break;
		case 3:
			vEvade.p.x = FixMul (vec_to_player->p.y, gameData.time.xFrame*32);
			vEvade.p.y = FixMul (-vec_to_player->p.x, gameData.time.xFrame*32);
			vEvade.p.z = FixMul (vec_to_player->p.z, gameData.time.xFrame*32);
			break;
		default:
			Error ("Function move_around_player: Bad case.");
	}

	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fastFlag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = VmVecDot (vec_to_player, &objP->position.mOrient.fVec);
		if ((dot > robptr->fieldOfView [gameStates.app.nDifficultyLevel]) && 
			 !(gameData.objs.console->flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			if (robptr->strength)
				damage_scale = FixDiv (objP->shields, robptr->strength);
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
	if ((OBJ_IDX (objP) != 1) && (speed > robptr->xMaxSpeed [gameStates.app.nDifficultyLevel])) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_away_from_player (tObject *objP, vmsVector *vec_to_player, int attackType)
{
	fix				speed;
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	tRobotInfo		*robptr = &gameData.bots.pInfo [objP->id];
	int				objref;

	pptr->velocity.p.x -= FixMul (vec_to_player->p.x, gameData.time.xFrame*16);
	pptr->velocity.p.y -= FixMul (vec_to_player->p.y, gameData.time.xFrame*16);
	pptr->velocity.p.z -= FixMul (vec_to_player->p.z, gameData.time.xFrame*16);

	if (attackType) {
		//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
		objref = ((OBJ_IDX (objP)) ^ ((gameData.app.nFrameCount + 3* (OBJ_IDX (objP))) >> 5)) & 3;

		switch (objref) {
			case 0:	VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.uVec, gameData.time.xFrame << 5);	break;
			case 1:	VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.uVec, -gameData.time.xFrame << 5);	break;
			case 2:	VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.rVec, gameData.time.xFrame << 5);	break;
			case 3:	VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.rVec, -gameData.time.xFrame << 5);	break;
			default:	Int3 ();	//	Impossible, bogus value on objref, must be in 0d:\temp\dm_test3
		}
	}


	speed = VmVecMagQuick (&pptr->velocity);

	if (speed > robptr->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_IDLING).
void ai_move_relative_to_player (tObject *objP, tAILocal *ailp, fix dist_to_player, vmsVector *vec_to_player, fix circleDistance, int evade_only, int player_visibility)
{
	tObject		*dObjP;
	tRobotInfo	*robptr = gameData.bots.pInfo + objP->id;

	Assert (player_visibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((robptr->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
if (objP->cType.aiInfo.nDangerLaser != -1) {
	dObjP = &gameData.objs.objects [objP->cType.aiInfo.nDangerLaser];

	if ((dObjP->nType == OBJ_WEAPON) && (dObjP->nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
		fix			dot, dist_to_laser, fieldOfView;
		vmsVector	vec_to_laser, laser_fvec;

		fieldOfView = gameData.bots.pInfo [objP->id].fieldOfView [gameStates.app.nDifficultyLevel];
		VmVecSub (&vec_to_laser, &dObjP->position.vPos, &objP->position.vPos);
		dist_to_laser = VmVecNormalizeQuick (&vec_to_laser);
		dot = VmVecDot (&vec_to_laser, &objP->position.mOrient.fVec);

		if ((dot > fieldOfView) || (robptr->companion)) {
			fix			laserRobot_dot;
			vmsVector	laser_vec_toRobot;

			//	The laser is seen by the robot, see if it might hit the robot.
			//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
			if (dObjP->renderType == RT_POLYOBJ)
				laser_fvec = dObjP->position.mOrient.fVec;
			else {		//	Not a polyobjP, get velocity and normalize.
				laser_fvec = dObjP->mType.physInfo.velocity;	//dObjP->position.mOrient.fVec;
				VmVecNormalizeQuick (&laser_fvec);
				}
			VmVecSub (&laser_vec_toRobot, &objP->position.vPos, &dObjP->position.vPos);
			VmVecNormalizeQuick (&laser_vec_toRobot);
			laserRobot_dot = VmVecDot (&laser_fvec, &laser_vec_toRobot);

			if ((laserRobot_dot > F1_0*7/8) && (dist_to_laser < F1_0*80)) {
				int	evadeSpeed = gameData.bots.pInfo [objP->id].evadeSpeed [gameStates.app.nDifficultyLevel];
				gameData.ai.bEvaded = 1;
				move_around_player (objP, vec_to_player, evadeSpeed);
				}
			}
		return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-nType guys to keep coming, don't return here after evasion.
	if ((!robptr->attackType) && (!robptr->thief) && evade_only)
		return;

	//	If we fall out of above, then no tObject to be avoided.
	objP->cType.aiInfo.nDangerLaser = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
if (robptr->attackType == 1)
	if (( (ailp->nextPrimaryFire > robptr->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) && (dist_to_player < F1_0*30)) || gameStates.app.bPlayerIsDead)
		//	1/4 of time, move around tPlayer, 3/4 of time, move away from tPlayer
		if (d_rand () < 8192)
			move_around_player (objP, vec_to_player, -1);
		else
			move_away_from_player (objP, vec_to_player, 1);
	else
		move_towards_player (objP, vec_to_player);
else if (robptr->thief)
	move_towards_player (objP, vec_to_player);
else {
	int	objval = ((OBJ_IDX (objP)) & 0x0f) ^ 0x0a;

	//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
	if (robptr->kamikaze)
		move_towards_player (objP, vec_to_player);
	else if (dist_to_player < circleDistance)
		move_away_from_player (objP, vec_to_player, 0);
	else if ((dist_to_player < (3+objval)*circleDistance/2) && (ailp->nextPrimaryFire > -F1_0))
		move_around_player (objP, vec_to_player, -1);
	else
		if ((-ailp->nextPrimaryFire > F1_0 + (objval << 12)) && player_visibility)
			//	Usually move away, but sometimes move around player.
			if (( ((gameData.time.xGame >> 18) & 0x0f) ^ objval) > 4) 
				move_away_from_player (objP, vec_to_player, 0);
			else
				move_around_player (objP, vec_to_player, -1);
		else
			move_towards_player (objP, vec_to_player);
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

#ifndef NDEBUG
void mprintf_animation_info (tObject *objP)
{
	tAIStatic	*aip = &objP->cType.aiInfo;
	tAILocal		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];

	if (!gameData.ai.bInfoEnabled)
		return;
#if TRACE	
	con_printf (CON_DEBUG, "Goal = ");
	switch (aip->GOAL_STATE) {
		case AIS_NONE:	con_printf (CON_DEBUG, "NONE ");	break;
		case AIS_REST:	con_printf (CON_DEBUG, "REST ");	break;
		case AIS_SRCH:	con_printf (CON_DEBUG, "SRCH ");	break;
		case AIS_LOCK:	con_printf (CON_DEBUG, "LOCK ");	break;
		case AIS_FLIN:	con_printf (CON_DEBUG, "FLIN ");	break;
		case AIS_FIRE:	con_printf (CON_DEBUG, "FIRE ");	break;
		case AIS_RECO:	con_printf (CON_DEBUG, "RECO ");	break;
		case AIS_ERR_:	con_printf (CON_DEBUG, "ERR_ ");	break;
	}
	con_printf (CON_DEBUG, " Cur = ");
	switch (aip->CURRENT_STATE) {
		case AIS_NONE:	con_printf (CON_DEBUG, "NONE ");	break;
		case AIS_REST:	con_printf (CON_DEBUG, "REST ");	break;
		case AIS_SRCH:	con_printf (CON_DEBUG, "SRCH ");	break;
		case AIS_LOCK:	con_printf (CON_DEBUG, "LOCK ");	break;
		case AIS_FLIN:	con_printf (CON_DEBUG, "FLIN ");	break;
		case AIS_FIRE:	con_printf (CON_DEBUG, "FIRE ");	break;
		case AIS_RECO:	con_printf (CON_DEBUG, "RECO ");	break;
		case AIS_ERR_:	con_printf (CON_DEBUG, "ERR_ ");	break;
	}
	con_printf (CON_DEBUG, " Aware = ");
	switch (ailp->playerAwarenessType) {
		case AIE_FIRE: con_printf (CON_DEBUG, "FIRE "); break;
		case AIE_HITT: con_printf (CON_DEBUG, "HITT "); break;
		case AIE_COLL: con_printf (CON_DEBUG, "COLL "); break;
		case AIE_HURT: con_printf (CON_DEBUG, "HURT "); break;
	}
	con_printf (CON_DEBUG, "Next fire = %6.3f, Time = %6.3f\n", f2fl (ailp->nextPrimaryFire), f2fl (ailp->playerAwarenessTime));
#endif
}
#endif

//	-------------------------------------------------------------------------------------------------------------------
int	Break_onObject = -1;

void do_firing_stuff (tObject *objP, int player_visibility, vmsVector *vec_to_player)
{
	if ((gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD) || 
		 (player_visibility >= 1)) {
		//	Now, if in robot's field of view, lock onto tPlayer
		fix	dot = VmVecDot (&objP->position.mOrient.fVec, vec_to_player);
		if ((dot >= 7*F1_0/8) || (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)) {
			tAIStatic	*aip = &objP->cType.aiInfo;
			tAILocal		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];

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
		} else if (dot >= F1_0/2) {
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
	if (objP->controlType == CT_AI) {
		if ((nType == PA_WEAPON_ROBOT_COLLISION) || (nType == PA_PLAYER_COLLISION))
			switch (objP->cType.aiInfo.behavior) {
				case AIB_IDLING:
				{
					int	r;

					//	Attack robots (eg, green guy) shouldn't have behavior = still.
					Assert (gameData.bots.pInfo [objP->id].attackType == 0);

					r = d_rand ();
					//	1/8 time, charge tPlayer, 1/4 time create path, rest of time, do nothing
					if (r < 4096) {
						CreatePathToPlayer (objP, 10, 1);
						objP->cType.aiInfo.behavior = AIB_STATION;
						objP->cType.aiInfo.nHideSegment = objP->position.nSegment;
						gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_CHASE_OBJECT;
					} else if (r < 4096+8192) {
						CreateNSegmentPath (objP, d_rand ()/8192 + 2, -1);
						gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_FOLLOW_PATH;
					}
					break;
				}
			}
	}

}
#ifndef NDEBUG
int	Do_aiFlag=1;
int	Cvv_test=0;
int	Cvv_lastTime [MAX_OBJECTS];
int	Gun_point_hack=0;
#endif

int		RobotSoundVolume=DEFAULT_ROBOT_SOUND_VOLUME;

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely ObjectCanSeePlayer would benefit from the
//	information of a normalized vec_to_player.
//	Return tPlayer visibility:
//		0		not visible
//		1		visible, but robot not looking at tPlayer (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		tPlayer is cloaked
//	If the tPlayer is cloaked, set vec_to_player based on time tPlayer cloaked and last uncloaked position.
//	Updates ailp->nPrevVisibility if tPlayer is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to player_visibility
void ComputeVisAndVec (tObject *objP, vmsVector *pos, tAILocal *ailp, vmsVector *vec_to_player, int *player_visibility, tRobotInfo *robptr, int *flag)
{
	if (!*flag) {
		if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
			fix			deltaTime, dist;
			int			cloak_index = (OBJ_IDX (objP)) % MAX_AI_CLOAK_INFO;

			deltaTime = gameData.time.xGame - gameData.ai.cloakInfo [cloak_index].lastTime;
			if (deltaTime > F1_0*2) {
				vmsVector	randvec;

				gameData.ai.cloakInfo [cloak_index].lastTime = gameData.time.xGame;
				MakeRandomVector (&randvec);
				VmVecScaleInc (&gameData.ai.cloakInfo [cloak_index].last_position, &randvec, 8*deltaTime);
			}

			dist = VmVecNormalizedDirQuick (vec_to_player, &gameData.ai.cloakInfo [cloak_index].last_position, pos);
			*player_visibility = ObjectCanSeePlayer (objP, pos, robptr->fieldOfView [gameStates.app.nDifficultyLevel], vec_to_player);
			// *player_visibility = 2;

			if ((ailp->nextMiscSoundTime < gameData.time.xGame) && ((ailp->nextPrimaryFire < F1_0) || (ailp->nextSecondaryFire < F1_0)) && (dist < F1_0*20)) {
				ailp->nextMiscSoundTime = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 1;
				DigiLinkSoundToPos (robptr->seeSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
			}
		} else {
			//	Compute expensive stuff -- vec_to_player and player_visibility
			VmVecNormalizedDirQuick (vec_to_player, &gameData.ai.vBelievedPlayerPos, pos);
			if ((vec_to_player->p.x == 0) && (vec_to_player->p.y == 0) && (vec_to_player->p.z == 0)) {
				vec_to_player->p.x = F1_0;
			}
			*player_visibility = ObjectCanSeePlayer (objP, pos, robptr->fieldOfView [gameStates.app.nDifficultyLevel], vec_to_player);

			//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
			//	see you without killing frame rate.
			{
				tAIStatic	*aip = &objP->cType.aiInfo;
			if ((*player_visibility == 2) && (ailp->nPrevVisibility != 2))
				if ((aip->GOAL_STATE == AIS_REST) || (aip->CURRENT_STATE == AIS_REST)) {
					aip->GOAL_STATE = AIS_FIRE;
					aip->CURRENT_STATE = AIS_FIRE;
				}
			}

			if ((ailp->nPrevVisibility != *player_visibility) && (*player_visibility == 2)) {
				if (ailp->nPrevVisibility == 0) {
					if (ailp->timePlayerSeen + F1_0/2 < gameData.time.xGame) {
						// -- if (gameStates.app.bPlayerExploded)
						// -- 	DigiLinkSoundToPos (robptr->tauntSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
						// -- else
							DigiLinkSoundToPos (robptr->seeSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
						ailp->timePlayerSoundAttacked = gameData.time.xGame;
						ailp->nextMiscSoundTime = gameData.time.xGame + F1_0 + d_rand ()*4;
					}
				} else if (ailp->timePlayerSoundAttacked + F1_0/4 < gameData.time.xGame) {
					// -- if (gameStates.app.bPlayerExploded)
					// -- 	DigiLinkSoundToPos (robptr->tauntSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
					// -- else
						DigiLinkSoundToPos (robptr->attackSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
					ailp->timePlayerSoundAttacked = gameData.time.xGame;
				}
			} 

			if ((*player_visibility == 2) && (ailp->nextMiscSoundTime < gameData.time.xGame)) {
				ailp->nextMiscSoundTime = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 2;
				// -- if (gameStates.app.bPlayerExploded)
				// -- 	DigiLinkSoundToPos (robptr->tauntSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
				// -- else
					DigiLinkSoundToPos (robptr->attackSound, objP->position.nSegment, 0, pos, 0 , RobotSoundVolume);
			}
			ailp->nPrevVisibility = *player_visibility;
		}

		*flag = 1;

		//	@mk, 09/21/95: If tPlayer view is not obstructed and awareness is at least as high as a nearby collision,
		//	act is if robot is looking at player.
		if (ailp->playerAwarenessType >= PA_NEARBY_ROBOT_FIRED)
			if (*player_visibility == 1)
				*player_visibility = 2;
				
		if (*player_visibility) {
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
//	Move the tObject objP to a spot in which it doesn't intersect a wall.
//	It might mean moving it outside its current tSegment.
void MoveObjectToLegalSpot (tObject *objP, int bMoveToCenter)
{
	vmsVector	vSegCenter, vOrigPos = objP->position.vPos;
	int			i;
	tSegment		*segP = gameData.segs.segments + objP->position.nSegment;

if (bMoveToCenter) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->position.nSegment);
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
				int nNewSeg = FindSegByPoint (&objP->position.vPos, objP->position.nSegment);
				if (nNewSeg != -1) {
					RelinkObject (OBJ_IDX (objP), nNewSeg);
					return;
					}
				}
			}
		}
	}

if (gameData.bots.pInfo [objP->id].bossFlag) {
	Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
	teleport_boss (objP);
	}
	else {
#if TRACE
		con_printf (CON_DEBUG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", OBJ_IDX (objP));
#endif
		ApplyDamageToRobot (objP, objP->shields*2, OBJ_IDX (objP));
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move tObject one tObject radii from current position towards tSegment center.
//	If tSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsPoint (tObject *objP, vmsVector *vGoal, fix xMinDist)
{
	int			nSegment = objP->position.nSegment;
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
	//	Move one radii towards center.
	VmVecScale (&vGoalDir, xScale);
	VmVecInc (&objP->position.vPos, &vGoalDir);
	nNewSeg = FindSegByPoint (&objP->position.vPos, objP->position.nSegment);
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

COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->position.nSegment);
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
	wall	*wallP;

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
if ((objP == NULL) || (gameData.bots.pInfo [objP->id].companion == 1)) {
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
			return (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_BLUE_KEY);
		else if (wallP->keys == KEY_GOLD)
			return (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_GOLD_KEY);
		else if (wallP->keys == KEY_RED)
			return (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_RED_KEY);
		}

	if (wallP->nType == WALL_CLOSED)
		return 0;
	if (wallP->nType != WALL_DOOR) /*&& (wallP->nType != WALL_CLOSED))*/
		return 1;

	//	If Buddy is returning to tPlayer, don't let him think he can get through triggered doors.
	//	It's only valid to think that if the tPlayer is going to get him through.  But if he's
	//	going to the tPlayer, the tPlayer is probably on the opposite tSide.
	if (objP == NULL)
		ailp_mode = gameData.ai.localInfo [gameData.escort.nObjNum].mode;
	else
		ailp_mode = gameData.ai.localInfo [OBJ_IDX (objP)].mode;

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
			if (wallP->keys & gameData.multi.players [gameData.multi.nLocalPlayer].flags)
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
			wall	*wallP = gameData.walls.walls + nWall;
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
// -- 		if ((gameData.objs.objects [nObject].nType == OBJ_PLAYER) || (gameData.objs.objects [nObject].nType == OBJ_CNTRLCEN)) {
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
int checkObjectObject_intersection (vmsVector *pos, fix size, tSegment *segP)
{
	int		curobjnum;

	//	If this would intersect with another tObject (only check those in this tSegment), then try to move.
	curobjnum = segP->objects;
	while (curobjnum != -1) {
		tObject *curObjP = &gameData.objs.objects [curobjnum];
		if ((curObjP->nType == OBJ_PLAYER) || (curObjP->nType == OBJ_ROBOT) || (curObjP->nType == OBJ_CNTRLCEN)) {
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
int CreateGatedRobot (short nSegment, ubyte object_id, vmsVector *pos)
{
	int			nObject, nTries = 5;
	tObject		*objP;
	tSegment		*segP = gameData.segs.segments + nSegment;
	vmsVector	object_pos;
	tRobotInfo	*robptr = &gameData.bots.pInfo [object_id];
	int			i, count=0;
	fix			objsize = gameData.models.polyModels [robptr->nModel].rad;
	ubyte			default_behavior;

	if (gameData.time.xGame - gameData.boss.nLastGateTime < gameData.boss.nGateInterval)
		return -1;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects [i].nType == OBJ_ROBOT)
			if (gameData.objs.objects [i].matCenCreator == BOSS_GATE_MATCEN_NUM)
				count++;

	if (count > 2*gameStates.app.nDifficultyLevel + 6) {
		gameData.boss.nLastGateTime = gameData.time.xGame - 3*gameData.boss.nGateInterval/4;
		return -1;
	}

	COMPUTE_SEGMENT_CENTER (&object_pos, segP);
	for (;;) {
		if (!pos)
			PickRandomPointInSeg (&object_pos, SEG_IDX (segP));
		else
			object_pos = *pos;

		//	See if legal to place tObject here.  If not, move about in tSegment and try again.
		if (checkObjectObject_intersection (&object_pos, objsize, segP)) {
			if (!--nTries) {
				gameData.boss.nLastGateTime = gameData.time.xGame - 3*gameData.boss.nGateInterval/4;
				return -1;
				}
			pos = NULL;
			}
		else 
			break;
		}

	nObject = CreateObject (OBJ_ROBOT, object_id, -1, nSegment, &object_pos, &vmdIdentityMatrix, objsize, CT_AI, MT_PHYSICS, RT_POLYOBJ, 0);
	if (nObject < 0) {
		gameData.boss.nLastGateTime = gameData.time.xGame - 3*gameData.boss.nGateInterval/4;
		return -1;
		} 
	// added lifetime increase depending on difficulty level 04/26/06 DM
	gameData.objs.objects [nObject].lifeleft = F1_0 * 30 + F0_5 * (gameStates.app.nDifficultyLevel * 15);	//	Gated in robots only live 30 seconds.
	multiData.create.nObjNums [0] = nObject; // A convenient global to get nObject back to caller for multiplayer
	objP = gameData.objs.objects + nObject;
	//Set polygon-tObject-specific data
	objP->rType.polyObjInfo.nModel = robptr->nModel;
	objP->rType.polyObjInfo.nSubObjFlags = 0;
	//set Physics info
	objP->mType.physInfo.mass = robptr->mass;
	objP->mType.physInfo.drag = robptr->drag;
	objP->mType.physInfo.flags |= (PF_LEVELLING);
	objP->shields = robptr->strength;
	objP->matCenCreator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.
	default_behavior = gameData.bots.pInfo [objP->id].behavior;
	InitAIObject (OBJ_IDX (objP), default_behavior, -1);		//	Note, -1 = tSegment this robot goes to to hide, should probably be something useful
	ObjectCreateExplosion (nSegment, &object_pos, i2f (10), VCLIP_MORPHING_ROBOT);
	DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nSegment, 0, &object_pos, 0 , F1_0);
	MorphStart (objP);

	gameData.boss.nLastGateTime = gameData.time.xGame;

	gameData.multi.players [gameData.multi.nLocalPlayer].numRobotsLevel++;
	gameData.multi.players [gameData.multi.nLocalPlayer].numRobotsTotal++;

	return OBJ_IDX (objP);
}

//	----------------------------------------------------------------------------------------------------------
//	objP points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
int BossSpewRobot (tObject *objP, vmsVector *pos)
{
	short			nObject, nSegment, objType, maxRobotTypes;
	short			nBossIndex, nBossId = gameData.bots.pInfo [objP->id].bossFlag;
	tRobotInfo	*pri;

	nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
	Assert ((nBossIndex >= 0) && (nBossIndex < NUM_D2_BOSSES));
	nSegment = pos ? FindSegByPoint (pos, objP->position.nSegment) : objP->position.nSegment;
	if (nSegment == -1) {
#if TRACE
		con_printf (CON_DEBUG, "Tried to spew a bot outside the mine! Aborting!\n");
#endif
		return -1;
	}	
	objType = spewBots [gameStates.app.bD1Mission][nBossIndex][ (maxSpewBots [nBossIndex] * d_rand ()) >> 15];
	if (objType == 255) {	// spawn an arbitrary robot
		maxRobotTypes = gameData.bots.nTypes [gameStates.app.bD1Mission];
		do {
				objType = d_rand () % maxRobotTypes;
			pri = gameData.bots.info [gameStates.app.bD1Mission] + objType;
			} while (pri->bossFlag ||	//well ... don't spawn another boss, huh? ;)
						pri->companion || //the buddy bot isn't exactly an enemy ... ^_^
						 (pri->scoreValue < 700)); //avoid spawning a ... spawn nType bot
		}
	nObject = CreateGatedRobot (nSegment, (ubyte) objType, pos);
	//	Make spewed robot come tumbling out as if blasted by a flash missile.
	if (nObject != -1) {
		tObject	*newObjP = &gameData.objs.objects [nObject];
		int		force_val;

		force_val = F1_0 / gameData.time.xFrame;

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

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
		gameData.ai.cloakInfo [i].last_segment = gameData.objs.console->position.nSegment;
		gameData.ai.cloakInfo [i].last_position = gameData.objs.console->position.vPos;
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
		int	i;
		
		for (i = 0; i < extraGameInfo [0].nBossCount; i++)
			if (nObject == gameData.boss.objList [i]) {
				nSegment = gameData.boss.gateSegs [i][ (d_rand () * gameData.boss.nGateSegs [i]) >> 15];
				break;
				}
			}

	Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));

	return CreateGatedRobot (nSegment, nType, NULL);
}

// --------------------------------------------------------------------------------------------------------------------
int boss_fits_in_seg (tObject *bossObjP, int nSegment)
{
	vmsVector	segcenter;
	int			objList = OBJ_IDX (bossObjP);
	int			posnum;

	COMPUTE_SEGMENT_CENTER_I (&segcenter, nSegment);

	for (posnum=0; posnum<9; posnum++) {
		if (posnum > 0) {
			vmsVector	vertex_pos;

			Assert ((posnum-1 >= 0) && (posnum-1 < 8));
			vertex_pos = gameData.segs.vertices [gameData.segs.segments [nSegment].verts [posnum-1]];
			VmVecAvg (&bossObjP->position.vPos, &vertex_pos, &segcenter);
		} else
			bossObjP->position.vPos = segcenter;

		RelinkObject (objList, nSegment);
		if (!ObjectIntersectsWall (bossObjP))
			return 1;
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
void teleport_boss (tObject *objP)
{
	short			i, rand_segnum = 0, rand_index, nObject;
	vmsVector	boss_dir;
	Assert (gameData.boss.nTeleportSegs [0] > 0);

	//	Pick a random tSegment from the list of boss-teleportable-to segments.
	nObject = OBJ_IDX (objP);
	for (i = 0; i < extraGameInfo [0].nBossCount; i++) {
		if (nObject == gameData.boss.objList [i]) {
			rand_index = (d_rand () * gameData.boss.nTeleportSegs [i]) >> 15;	
			rand_segnum = gameData.boss.teleportSegs [i][rand_index];
			Assert ((rand_segnum >= 0) && (rand_segnum <= gameData.segs.nLastSegment));
			break;
			}
		}

	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendBossActions (OBJ_IDX (objP), 1, rand_segnum, 0);
	COMPUTE_SEGMENT_CENTER_I (&objP->position.vPos, rand_segnum);
	RelinkObject (OBJ_IDX (objP), rand_segnum);

	gameData.boss.nLastTeleportTime = gameData.time.xGame;

	//	make boss point right at tPlayer
	VmVecSub (&boss_dir, &gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].position.vPos, &objP->position.vPos);
	VmVector2Matrix (&objP->position.mOrient, &boss_dir, NULL, NULL);

	DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, rand_segnum, 0, &objP->position.vPos, 0 , F1_0);
	DigiKillSoundLinkedToObject (OBJ_IDX (objP));
	DigiLinkSoundToObject2 (gameData.bots.pInfo [objP->id].seeSound, OBJ_IDX (objP), 1, F1_0, F1_0*512);	//	F1_0*512 means play twice as loud

	//	After a teleport, boss can fire right away.
	gameData.ai.localInfo [OBJ_IDX (objP)].nextPrimaryFire = 0;
	gameData.ai.localInfo [OBJ_IDX (objP)].nextSecondaryFire = 0;

}

//	----------------------------------------------------------------------
void start_boss_death_sequence (tObject *objP)
{
	if (gameData.bots.pInfo [objP->id].bossFlag) {
		int	i;

		gameData.boss.nDying = OBJ_IDX (objP);
		gameData.boss.nDyingStartTime = gameData.time.xGame;
		--extraGameInfo [0].nBossCount;
		for (i = 0; i <= extraGameInfo [0].nBossCount; i++) {
			if (gameData.boss.objList [i] == gameData.boss.nDying) {
				if (i < extraGameInfo [0].nBossCount)
					gameData.boss.objList [i] = gameData.boss.objList [extraGameInfo [0].nBossCount];
				break;
				}
			}
	}

}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if tObject just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
int DoRobotDyingFrame (tObject *objP, fix StartTime, fix roll_duration, sbyte *bDyingSoundPlaying, short deathSound, fix expl_scale, fix sound_scale)
{
	fix	roll_val, temp;
	fix	sound_duration;

	if (!roll_duration)
		roll_duration = F1_0/4;

	roll_val = FixDiv (gameData.time.xGame - StartTime, roll_duration);

	FixSinCos (FixMul (roll_val, roll_val), &temp, &objP->mType.physInfo.rotVel.p.x);
	FixSinCos (roll_val, &temp, &objP->mType.physInfo.rotVel.p.y);
	FixSinCos (roll_val-F1_0/8, &temp, &objP->mType.physInfo.rotVel.p.z);

	objP->mType.physInfo.rotVel.p.x = (gameData.time.xGame - StartTime)/9;
	objP->mType.physInfo.rotVel.p.y = (gameData.time.xGame - StartTime)/5;
	objP->mType.physInfo.rotVel.p.z = (gameData.time.xGame - StartTime)/7;

	if (gameOpts->sound.digiSampleRate)
		sound_duration = FixDiv (gameData.pig.snd.pSounds [DigiXlatSound (deathSound)].length, gameOpts->sound.digiSampleRate);
	else
		sound_duration = F1_0;

	if (StartTime + roll_duration - sound_duration < gameData.time.xGame) {
		if (!*bDyingSoundPlaying) {
#if TRACE
			con_printf (CON_DEBUG, "Starting death sound!\n");
#endif
			*bDyingSoundPlaying = 1;
			DigiLinkSoundToObject2 (deathSound, OBJ_IDX (objP), 0, sound_scale, sound_scale*256);	//	F1_0*512 means play twice as loud
		} else if (d_rand () < gameData.time.xFrame*16)
			CreateSmallFireballOnObject (objP, (F1_0 + d_rand ()) * (16 * expl_scale/F1_0)/8, 0);
	} else if (d_rand () < gameData.time.xFrame*8)
		CreateSmallFireballOnObject (objP, (F1_0/2 + d_rand ()) * (16 * expl_scale/F1_0)/8, 1);

	if (StartTime + roll_duration < gameData.time.xGame)
		return 1;
	else
		return 0;
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
	int	rval;

rval = DoRobotDyingFrame (objP, gameData.boss.nDyingStartTime, BOSS_DEATH_DURATION, 
								 &gameData.boss.bDyingSoundPlaying, 
								 gameData.bots.pInfo [objP->id].deathrollSound, F1_0*4, F1_0*4);
if (rval) {
	DoReactorDestroyedStuff (NULL);
	ExplodeObject (objP, F1_0/4);
	DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512);
	gameData.boss.nDying = 0;
	}
}

extern void RecreateThief (tObject *objP);

//	----------------------------------------------------------------------

int DoAnyRobotDyingFrame (tObject *objP)
{
if (objP->cType.aiInfo.xDyingStartTime) {
	int bDeathRoll = gameData.bots.pInfo [objP->id].bDeathRoll;
	int rval = DoRobotDyingFrame (objP, objP->cType.aiInfo.xDyingStartTime, min (bDeathRoll/2+1,6)*F1_0, &objP->cType.aiInfo.bDyingSoundPlaying, gameData.bots.pInfo [objP->id].deathrollSound, bDeathRoll*F1_0/8, bDeathRoll*F1_0/2); 
	if (rval) {
		ExplodeObject (objP, F1_0/4);
		DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512);
		if ((gameData.missions.nCurrentLevel < 0) && (gameData.bots.pInfo [objP->id].thief))
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
	int	rval=1;

if (gameData.app.nGameMode & GM_MULTI) {
	if (awarenessLevel == 0)
		return 0;
	rval = MultiCanRemoveRobot (OBJ_IDX (objP), awarenessLevel);
}
return rval;
}

#ifndef NDEBUG
fix	Prev_boss_shields = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void DoBossStuff (tObject *objP, int player_visibility)
{
	int	i, nBossId, nBossIndex, nObject, bBossAlive = 0;

nBossId = gameData.bots.pInfo [objP->id].bossFlag;
//	Assert ((nBossId >= BOSS_D2) && (nBossId < BOSS_D2 + NUM_D2_BOSSES));
nBossIndex = (nBossId >= BOSS_D2) ? nBossId - BOSS_D2 : nBossId;
#ifndef NDEBUG
if (objP->shields != Prev_boss_shields) {
#if TRACE
	con_printf (CON_DEBUG, "Boss shields = %7.3f, tObject %i\n", f2fl (objP->shields), OBJ_IDX (objP));
#endif
	Prev_boss_shields = objP->shields;
	}
#endif
	//	New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
if (gameData.boss.nLastTeleportTime > gameData.time.xGame)
	gameData.boss.nLastTeleportTime = gameData.time.xGame;

if (gameData.boss.nLastGateTime > gameData.time.xGame)
	gameData.boss.nLastGateTime = gameData.time.xGame;

nObject = OBJ_IDX (objP);
for (i = 0; i < extraGameInfo [0].nBossCount; i++)
	if (gameData.boss.objList [i] == nObject) {
		bBossAlive = 1;
		break;
		}
//	@mk, 10/13/95:  Reason:
//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
//		teleports out of there right away, and blasts tPlayer right after first door.
if (!player_visibility && (gameData.time.xGame - gameData.boss.nHitTime > F1_0*2))
	return;

if (bBossAlive && bossProps [gameStates.app.bD1Mission][nBossIndex].bTeleports) {
	if (objP->cType.aiInfo.CLOAKED == 1) {
		gameData.boss.nHitTime = gameData.time.xGame;	//	Keep the cloak:teleport process going.
		if ((gameData.time.xGame - gameData.boss.nCloakStartTime > BOSS_CLOAK_DURATION/3) && 
				(gameData.boss.nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION/3) && 
				(gameData.time.xGame - gameData.boss.nLastTeleportTime > gameData.boss.nTeleportInterval)) {
			if (AIMultiplayerAwareness (objP, 98)) {

				teleport_boss (objP);
				if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewBotsTeleport) {
#if 1
					vmsVector	spewPoint;
#	if 1
					VmVecCopyScale (&spewPoint, &objP->position.mOrient.fVec, objP->size * 2);
#	else
					VmVecCopyNormalize (&spewPoint, &objP->position.mOrient.fVec);
					VmVecScale (&spewPoint, objP->size * 2);
#	endif
					VmVecInc (&spewPoint, &objP->position.vPos);
#endif
					if (bossProps [gameStates.app.bD1Mission][nBossIndex].bSpewMore && (d_rand () > 16384) &&
							(BossSpewRobot (objP, &spewPoint) != -1))
						gameData.boss.nLastGateTime = gameData.time.xGame - gameData.boss.nGateInterval - 1;	//	Force allowing spew of another bot.
					BossSpewRobot (objP, &spewPoint);
					}
				}
			} 
		else if (gameData.time.xGame - gameData.boss.nHitTime > F1_0*2) {
			gameData.boss.nLastTeleportTime -= gameData.boss.nTeleportInterval/4;
			}
	if (!gameData.boss.nCloakDuration)
		gameData.boss.nCloakDuration = BOSS_CLOAK_DURATION;
	if ((gameData.time.xGame > gameData.boss.nCloakEndTime) || 
			(gameData.time.xGame < gameData.boss.nCloakStartTime))
		objP->cType.aiInfo.CLOAKED = 0;
		}
	else if ((gameData.time.xGame - gameData.boss.nCloakEndTime > gameData.boss.nCloakInterval) || 
					(gameData.time.xGame - gameData.boss.nCloakEndTime < -gameData.boss.nCloakDuration)) {
		if (AIMultiplayerAwareness (objP, 95)) {
			gameData.boss.nCloakStartTime = gameData.time.xGame;
			gameData.boss.nCloakEndTime = gameData.time.xGame+gameData.boss.nCloakDuration;
			objP->cType.aiInfo.CLOAKED = 1;
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendBossActions (OBJ_IDX (objP), 2, 0, 0);
			}
		}
	}

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	 (F1_0*200)

// -- Obsolete D1 code -- // --------------------------------------------------------------------------------------------------------------------
// -- Obsolete D1 code -- //	Do special stuff for a boss.
// -- Obsolete D1 code -- void do_super_boss_stuff (tObject *objP, fix dist_to_player, int player_visibility)
// -- Obsolete D1 code -- {
// -- Obsolete D1 code -- 	static int eclip_state = 0;
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 	DoBossStuff (objP, player_visibility);
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 	// Only master tPlayer can cause gating to occur.
// -- Obsolete D1 code -- 	if ((gameData.app.nGameMode & GM_MULTI) && !NetworkIAmMaster ())
// -- Obsolete D1 code -- 		return; 
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 	if ((dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE) || player_visibility || (gameData.app.nGameMode & GM_MULTI)) {
// -- Obsolete D1 code -- 		if (gameData.time.xGame - gameData.boss.nLastGateTime > gameData.boss.nGateInterval/2) {
// -- Obsolete D1 code -- 			RestartEffect (BOSS_ECLIP_NUM);
// -- Obsolete D1 code -- 			if (eclip_state == 0) {
// -- Obsolete D1 code -- 				MultiSendBossActions (OBJ_IDX (objP), 4, 0, 0);
// -- Obsolete D1 code -- 				eclip_state = 1;
// -- Obsolete D1 code -- 			}
// -- Obsolete D1 code -- 		}
// -- Obsolete D1 code -- 		else {
// -- Obsolete D1 code -- 			StopEffect (BOSS_ECLIP_NUM);
// -- Obsolete D1 code -- 			if (eclip_state == 1) {
// -- Obsolete D1 code -- 				MultiSendBossActions (OBJ_IDX (objP), 5, 0, 0);
// -- Obsolete D1 code -- 				eclip_state = 0;
// -- Obsolete D1 code -- 			}
// -- Obsolete D1 code -- 		}
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 		if (gameData.time.xGame - gameData.boss.nLastGateTime > gameData.boss.nGateInterval)
// -- Obsolete D1 code -- 			if (AIMultiplayerAwareness (objP, 99)) {
// -- Obsolete D1 code -- 				int	rtval;
// -- Obsolete D1 code -- 				int	randtype = (d_rand () * MAX_GATE_INDEX) >> 15;
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 				Assert (randtype < MAX_GATE_INDEX);
// -- Obsolete D1 code -- 				randtype = Super_boss_gate_list [randtype];
// -- Obsolete D1 code -- 				Assert (randtype < gameData.bots.nTypes);
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 				rtval = GateInRobot (randtype, -1);
// -- Obsolete D1 code -- 				if ((rtval != -1) && (gameData.app.nGameMode & GM_MULTI))
// -- Obsolete D1 code -- 				{
// -- Obsolete D1 code -- 					MultiSendBossActions (OBJ_IDX (objP), 3, randtype, multiData.create.nObjNums [0]);
// -- Obsolete D1 code -- 					MapObjnumLocalToLocal (multiData.create.nObjNums [0]);
// -- Obsolete D1 code -- 				}
// -- Obsolete D1 code -- 			}	
// -- Obsolete D1 code -- 	}
// -- Obsolete D1 code -- }

//int MultiCanRemoveRobot (tObject *objP, int awarenessLevel)
//{
//	return 0;
//}

void ai_multi_sendRobot_position (short nObject, int force)
{
if (gameData.app.nGameMode & GM_MULTI) {
	if (force != -1)
		MultiSendRobotPosition (nObject, 1);
	else
		MultiSendRobotPosition (nObject, 0);
	}
return;
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this tObject should be allowed to fire at the player.
int maybe_ai_do_actual_firing_stuff (tObject *objP, tAIStatic *aip)
{
if (gameData.app.nGameMode & GM_MULTI)
	if ((aip->GOAL_STATE != AIS_FLIN) && (objP->id != ROBOT_BRAIN))
		if (aip->CURRENT_STATE == AIS_FIRE)
			return 1;
return 0;
}

vmsVector	Last_fired_upon_player_pos;

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if tPlayer is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
void ai_do_actual_firing_stuff (tObject *objP, tAIStatic *aip, tAILocal *ailp, tRobotInfo *robptr, vmsVector *vec_to_player, fix dist_to_player, vmsVector *gun_point, int player_visibility, int object_animates, int nGun)
{
	fix	dot;
	//tRobotInfo *robptr = gameData.bots.pInfo + objP->id;

	if ((player_visibility == 2) || 
		 (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
		vmsVector	fire_pos;

		fire_pos = gameData.ai.vBelievedPlayerPos;

		//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
		//	So, fire at Last_fired_upon_player_pos instead of the tPlayer position.
		if (!robptr->attackType && (player_visibility != 2))
			fire_pos = Last_fired_upon_player_pos;

		//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
		//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
		if (!object_animates || ready_to_fire (robptr, ailp)) {
			dot = VmVecDot (&objP->position.mOrient.fVec, vec_to_player);
			if ((dot >= 7*F1_0/8) || ((dot > F1_0/4) &&  robptr->bossFlag)) {

				if (nGun < robptr->nGuns) {
					if (robptr->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (dist_to_player < objP->size + gameData.objs.console->size + F1_0*2)) {		// robptr->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAiRobotHitAttack (objP, gameData.objs.console, &objP->position.vPos);
						} else {
							return;
						}
					} else {
						if ((gun_point->p.x == 0) && (gun_point->p.y == 0) && (gun_point->p.z == 0)) {
							;
						} else {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailp->nextPrimaryFire <= 0) {
									AIFireLaserAtPlayer (objP, gun_point, nGun, &fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}

								if ((ailp->nextSecondaryFire <= 0) && (robptr->nSecWeaponType != -1)) {
									CalcGunPoint (gun_point, objP, 0);
									AIFireLaserAtPlayer (objP, gun_point, 0, &fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}

							} else if (ailp->nextPrimaryFire <= 0) {
								AIFireLaserAtPlayer (objP, gun_point, nGun, &fire_pos);
								Last_fired_upon_player_pos = fire_pos;
							}
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ((aip->behavior != AIB_RUN_FROM)
						 && (aip->behavior != AIB_IDLING)
						 && (aip->behavior != AIB_SNIPE)
						 && (aip->behavior != AIB_FOLLOW)
						 && (!robptr->attackType)
						 && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_IDLING)))
						ailp->mode = AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = AIS_RECO;
				ailp->goalState [aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.  If has 2 gun types, select gun #1, if exists.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->nGuns)
				{
					if ((robptr->nGuns == 1) || (robptr->nSecWeaponType == -1))
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
				}
			}
		}
	} else if (( (!robptr->attackType) &&
				   (gameData.weapons.info [robptr->nWeaponType].homingFlag == 1)) || 
				  (( (robptr->nSecWeaponType != -1) && 
				   (gameData.weapons.info [robptr->nSecWeaponType].homingFlag == 1)))) {
		fix dist;
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if (( (!object_animates) || (ailp->achievedState [aip->CURRENT_GUN] == AIS_FIRE))
			 && (( (ailp->nextPrimaryFire <= 0) && (aip->CURRENT_GUN != 0)) || ((ailp->nextSecondaryFire <= 0) && (aip->CURRENT_GUN == 0)))
			 && ((dist = VmVecDistQuick (&gameData.ai.vHitPos, &objP->position.vPos)) > F1_0*40)) {
			if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
				return;
			AIFireLaserAtPlayer (objP, gun_point, nGun, &gameData.ai.vBelievedPlayerPos);

			aip->GOAL_STATE = AIS_RECO;
			ailp->goalState [aip->CURRENT_GUN] = AIS_RECO;

			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->nGuns)
				aip->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->nGuns)
				aip->CURRENT_GUN = 0;
		}
	} else {


	//	---------------------------------------------------------------

		vmsVector	vec_to_last_pos;

		if (d_rand ()/2 < FixMul (gameData.time.xFrame, (gameStates.app.nDifficultyLevel << 12) + 0x4000)) {
		if ((!object_animates || ready_to_fire (robptr, ailp)) && (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			VmVecNormalizedDirQuick (&vec_to_last_pos, &gameData.ai.vBelievedPlayerPos, &objP->position.vPos);
			dot = VmVecDot (&objP->position.mOrient.fVec, &vec_to_last_pos);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < robptr->nGuns) {
					if (robptr->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (dist_to_player < objP->size + gameData.objs.console->size + F1_0*2)) {		// robptr->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAiRobotHitAttack (objP, gameData.objs.console, &objP->position.vPos);
						} else {
							return;
						}
					} else {
						if ((gun_point->p.x == 0) && (gun_point->p.y == 0) && (gun_point->p.z == 0)) {
							; 
						} else {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailp->nextPrimaryFire <= 0)
									AIFireLaserAtPlayer (objP, gun_point, nGun, &Last_fired_upon_player_pos);

								if ((ailp->nextSecondaryFire <= 0) && (robptr->nSecWeaponType != -1)) {
									CalcGunPoint (gun_point, objP, 0);
									AIFireLaserAtPlayer (objP, gun_point, 0, &Last_fired_upon_player_pos);
								}

							} else if (ailp->nextPrimaryFire <= 0)
								AIFireLaserAtPlayer (objP, gun_point, nGun, &Last_fired_upon_player_pos);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ((aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_IDLING) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_IDLING)))
						ailp->mode = AIM_CHASE_OBJECT;
				}
				aip->GOAL_STATE = AIS_RECO;
				ailp->goalState [aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->nGuns)
				{
					if (robptr->nGuns == 1)
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
				}
			}
		}
		}


	//	---------------------------------------------------------------


	}

}


