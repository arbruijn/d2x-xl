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

/*
 *
 * Split ai.c into two files: ai.c, ai2.c.
 *
 * Old Log:
 * Revision 1.1  1995/05/25  12:00:31  mike
 * Initial revision
 *
 *
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
#ifdef NETWORK
#include "multi.h"
#include "network.h"
#endif
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

void teleport_boss (object *objP);
int boss_fits_in_seg (object *bossObjP, int segnum);

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
//	Mallocs gameData.bots.nTypes robot_info structs into global gameData.bots.pInfo.
void InitAISystem (void)
{
#if 0
	int	i;

#if TRACE	
	con_printf (CON_DEBUG, "Trying to d_malloc %i bytes for gameData.bots.pInfo.\n", gameData.bots.nTypes * sizeof (*gameData.bots.pInfo));
#endif
	gameData.bots.pInfo = (robot_info *) d_malloc (gameData.bots.nTypes * sizeof (*gameData.bots.pInfo));
#if TRACE	
	con_printf (CON_DEBUG, "gameData.bots.pInfo = %i\n", gameData.bots.pInfo);
#endif
	for (i=0; i<gameData.bots.nTypes; i++) {
		gameData.bots.pInfo [i].field_of_view = F1_0/2;
		gameData.bots.pInfo [i].firing_wait = F1_0;
		gameData.bots.pInfo [i].turn_time = F1_0*2;
		// -- gameData.bots.pInfo [i].fire_power = F1_0;
		// -- gameData.bots.pInfo [i].shield = F1_0/2;
		gameData.bots.pInfo [i].max_speed = F1_0*10;
		gameData.bots.pInfo [i].always_0xabcd = 0xabcd;
	}
#endif

}

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
int AIBehaviorToMode (int behavior)
{
	switch (behavior) {
		case AIB_STILL:			return AIM_STILL;
		case AIB_NORMAL:			return AIM_CHASE_OBJECT;
		case AIB_BEHIND:			return AIM_BEHIND;
		case AIB_RUN_FROM:		return AIM_RUN_FROM_OBJECT;
		case AIB_SNIPE:			return AIM_STILL;	//	Changed, 09/13/95, MK, snipers are still until they see you or are hit.
		case AIB_STATION:			return AIM_STILL;
		case AIB_FOLLOW:			return AIM_FOLLOW_PATH;
		default:	Int3 ();	//	Contact Mike: Error, illegal behavior type
	}

	return AIM_STILL;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the player starts a new ship.
void AIInitBossForShip (void)
{
	gameData.boss.nHitTime = -F1_0*10;

}

// ---------------------------------------------------------------------------------------------------------------------
//	initial_mode == -1 means leave mode unchanged.
void InitAIObject (short objnum, short behavior, short hide_segment)
{
	object		*objP = gameData.objs.objects + objnum;
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo [objnum];
	robot_info	*robptr = &gameData.bots.pInfo [objP->id];

if (behavior == 0) {
	behavior = AIB_NORMAL;
	aip->behavior = (ubyte) behavior;
	}
//	mode is now set from the Robot dialog, so this should get overwritten.
ailp->mode = AIM_STILL;
ailp->previous_visibility = 0;
if (behavior != -1) {
	aip->behavior = (ubyte) behavior;
	ailp->mode = AIBehaviorToMode (aip->behavior);
	} 
else if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
#if TRACE	
	con_printf (CON_DEBUG, " [obj %i -> normal] ", objnum);
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
if (robptr->attack_type) {
	aip->behavior = AIB_NORMAL;
	ailp->mode = AIBehaviorToMode (aip->behavior);
	}
// This is astonishingly stupid! This routine gets called by matcens!KILL KILL KILL!!!gameData.ai.freePointSegs = gameData.ai.pointSegs;
VmVecZero (&objP->mtype.phys_info.velocity);
// -- ailp->wait_time = F1_0*5;
ailp->player_awareness_time = 0;
ailp->player_awareness_type = 0;
aip->GOAL_STATE = AIS_SRCH;
aip->CURRENT_STATE = AIS_REST;
ailp->time_player_seen = gameData.time.xGame;
ailp->next_misc_sound_time = gameData.time.xGame;
ailp->time_player_sound_attacked = gameData.time.xGame;
if ((behavior == AIB_SNIPE) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM) || (behavior == AIB_FOLLOW)) {
	aip->hide_segment = hide_segment;
	ailp->goal_segment = hide_segment;
	aip->hide_index = -1;			// This means the path has not yet been created.
	aip->cur_path_index = 0;
	}
aip->SKIP_AI_COUNT = 0;
aip->CLOAKED =  (robptr->cloak_type == RI_CLOAKED_ALWAYS);
objP->mtype.phys_info.flags |= (PF_BOUNCE | PF_TURNROLL);
aip->REMOTE_OWNER = -1;
aip->dying_sound_playing = 0;
aip->dying_start_time = 0;
}


extern object * CreateMorphRobot (segment *segp, vms_vector *object_pos, ubyte object_id);

// --------------------------------------------------------------------------------------------------------------------
//	Create a Buddy bot.
//	This automatically happens when you bring up the Buddy menu in a debug version.
//	It is available as a cheat in a non-debug (release) version.
void CreateBuddyBot (void)
{
	ubyte	buddy_id;
	vms_vector	object_pos;

	for (buddy_id=0; buddy_id<gameData.bots.nTypes [0]; buddy_id++)
		if (gameData.bots.info [0][buddy_id].companion)
			break;

	if (buddy_id == gameData.bots.nTypes [0]) {
#if TRACE	
		con_printf (CON_DEBUG, "Can't create Buddy.  No 'companion' bot found in gameData.bots.pInfo!\n");
#endif
		return;
	}
	COMPUTE_SEGMENT_CENTER_I (&object_pos, gameData.objs.console->segnum);
	CreateMorphRobot (gameData.segs.segments + gameData.objs.console->segnum, &object_pos, buddy_id);
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segptr.
//	Set *num_segs.
//	Boss is allowed to teleport to segments he fits in (calls ObjectIntersectsWall) and
//	he can reach from his initial position (calls FindConnectedDistance).
//	If size_check is set, then only add segment if boss can fit in it, else any segment is legal.
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
		vms_vector	original_boss_pos;
		object		*bossObjP = gameData.objs.objects + objList;
		int			head, tail;
		int			seg_queue [QUEUE_SIZE];
		//ALREADY IN RENDER.H sbyte   bVisited [MAX_SEGMENTS];
		fix			boss_size_save;
		int			nGroup;

		boss_size_save = bossObjP->size;
		// -- Causes problems!!	-- bossObjP->size = FixMul ((F1_0/4)*3, bossObjP->size);
		original_boss_seg = bossObjP->segnum;
		original_boss_pos = bossObjP->pos;
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
			short		sidenum;
			segment	*segp = gameData.segs.segments + seg_queue [tail++];

			tail &= QUEUE_SIZE-1;

			for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
				int	w, childSeg = segp->children [sidenum];

				if (( (w = WALL_IS_DOORWAY (segp, sidenum, NULL)) & WID_FLY_FLAG) || one_wall_hack) {
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
		bossObjP->pos = original_boss_pos;
		RelinkObject (objList, original_boss_seg);

	}

}

extern void InitBuddyForLevel (void);

// ---------------------------------------------------------------------------------------------------------------------
void InitAIObjects (void)
{
	short		i, j;
	object	*objP;

	gameData.ai.freePointSegs = gameData.ai.pointSegs;

	memset (gameData.boss.objList, 0xff, sizeof (gameData.boss.objList));
	for (i=j=0, objP = gameData.objs.objects; i<MAX_OBJECTS; i++, objP++) {
		if (objP->control_type == CT_AI)
			InitAIObject (i, objP->ctype.ai_info.behavior, objP->ctype.ai_info.hide_segment);
		if ((objP->type == OBJ_ROBOT) && (gameData.bots.pInfo [objP->id].boss_flag)) {
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
		Firing_wait_copy [i] = gameData.bots.pInfo [i].firing_wait [NDL-1];
		Firing_wait2_copy [i] = gameData.bots.pInfo [i].firing_wait2 [NDL-1];
		Rapidfire_count_copy [i] = gameData.bots.pInfo [i].rapidfire_count [NDL-1];

		gameData.bots.pInfo [i].firing_wait [NDL-1] = gameData.bots.pInfo [i].firing_wait [1];
		gameData.bots.pInfo [i].firing_wait2 [NDL-1] = gameData.bots.pInfo [i].firing_wait2 [1];
		gameData.bots.pInfo [i].rapidfire_count [NDL-1] = gameData.bots.pInfo [i].rapidfire_count [1];
	}

}

void DoLunacyOff (void)
{
	int	i;

	if (!gameStates.app.bLunacy)	//already off
		return;

	gameStates.app.bLunacy = 0;

	for (i=0; i<MAX_ROBOT_TYPES; i++) {
		gameData.bots.pInfo [i].firing_wait [NDL-1] = Firing_wait_copy [i];
		gameData.bots.pInfo [i].firing_wait2 [NDL-1] = Firing_wait2_copy [i];
		gameData.bots.pInfo [i].rapidfire_count [NDL-1] = Rapidfire_count_copy [i];
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

extern void physics_turn_towards_vector (vms_vector *goal_vector, object *objP, fix rate);

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector (vms_vector *goal_vector, object *objP, fix rate)
{
	vms_vector	new_fvec;
	fix			dot;

	//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
	if (rate == 0)
		return;

	if ((objP->id == BABY_SPIDER_ID) && (objP->type == OBJ_ROBOT)) {
		physics_turn_towards_vector (goal_vector, objP, rate);
		return;
	}

	new_fvec = *goal_vector;

	dot = VmVecDot (goal_vector, &objP->orient.fvec);

	if (dot < (F1_0 - gameData.time.xFrame/2)) {
		fix	mag;
		fix	new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
		VmVecScale (&new_fvec, new_scale);
		VmVecInc (&new_fvec, &objP->orient.fvec);
		mag = VmVecNormalizeQuick (&new_fvec);
		if (mag < F1_0/256) {
#if TRACE	
			con_printf (1, "Degenerate vector in ai_turn_towards_vector (mag = %7.3f)\n", f2fl (mag));
#endif
			new_fvec = *goal_vector;		//	if degenerate vector, go right to goal
		}
	}

	if (gameStates.gameplay.seismic.nMagnitude) {
		vms_vector	rand_vec;
		fix			scale;
		MakeRandomVector (&rand_vec);
		scale = FixDiv (2*gameStates.gameplay.seismic.nMagnitude, gameData.bots.pInfo [objP->id].mass);
		VmVecScaleInc (&new_fvec, &rand_vec, scale);
	}

	VmVector2Matrix (&objP->orient, &new_fvec, NULL, &objP->orient.rvec);
}

// -- unused, 08/07/95 -- // --------------------------------------------------------------------------------------------------------------------
// -- unused, 08/07/95 -- void ai_turn_randomly (vms_vector *vec_to_player, object *objP, fix rate, int previous_visibility)
// -- unused, 08/07/95 -- {
// -- unused, 08/07/95 -- 	vms_vector	curvec;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- // -- MK, 06/09/95	//	Random turning looks too stupid, so 1/4 of time, cheat.
// -- unused, 08/07/95 -- // -- MK, 06/09/95	if (previous_visibility)
// -- unused, 08/07/95 -- // -- MK, 06/09/95		if (d_rand () > 0x7400) {
// -- unused, 08/07/95 -- // -- MK, 06/09/95			ai_turn_towards_vector (vec_to_player, objP, rate);
// -- unused, 08/07/95 -- // -- MK, 06/09/95			return;
// -- unused, 08/07/95 -- // -- MK, 06/09/95		}
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- 	curvec = objP->mtype.phys_info.rotvel;
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
// -- unused, 08/07/95 -- 	objP->mtype.phys_info.rotvel = curvec;
// -- unused, 08/07/95 -- 
// -- unused, 08/07/95 -- }

//	gameData.ai.nOverallAgitation affects:
//		Widens field of view.  Field of view is in range 0d:\temp\dm_test1 (specified in bitmaps.tbl as N/360 degrees).
//			gameData.ai.nOverallAgitation/128 subtracted from field of view, making robots see wider.
//		Increases distance to which robot will search to create path to player by gameData.ai.nOverallAgitation/8 segments.
//		Decreases wait between fire times by gameData.ai.nOverallAgitation/64 seconds.


// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from object, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.vBelievedPlayerPos as player's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int ObjectCanSeePlayer (object *objP, vms_vector *pos, fix field_of_view, vms_vector *vec_to_player)
{
	fix			dot;
	fvi_query	fq;

	//	Assume that robot's gun tip is in same segment as robot's center.
	objP->ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;

	fq.p0	= pos;
	if ((pos->x != objP->pos.x) || (pos->y != objP->pos.y) || (pos->z != objP->pos.z)) {
		short segnum = FindSegByPoint (pos, objP->segnum);
		if (segnum == -1) {
			fq.startSeg = objP->segnum;
			*pos = objP->pos;
#if TRACE	
			con_printf (1, "Object %i, gun is outside mine, moving towards center.\n", OBJ_IDX (objP));
#endif
			move_towards_segment_center (objP);
			} 
		else {
			if (segnum != objP->segnum)
				objP->ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
			fq.startSeg = segnum;
			}
		}
	else
		fq.startSeg	= objP->segnum;
	fq.p1					= &gameData.ai.vBelievedPlayerPos;
	fq.rad				= F1_0/4;
	fq.thisObjNum		= OBJ_IDX (objP);
	fq.ignoreObjList	= NULL;
	fq.flags				= FQ_TRANSWALL; // -- Why were we checking gameData.objs.objects? | FQ_CHECK_OBJS;		//what about trans walls???

	gameData.ai.nHitType = FindVectorIntersection (&fq,&gameData.ai.hitData);

	gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
	gameData.ai.nHitSeg = gameData.ai.hitData.hit.nSegment;

	// -- when we stupidly checked gameData.objs.objects -- if ((gameData.ai.nHitType == HIT_NONE) || ((gameData.ai.nHitType == HIT_OBJECT) && (gameData.ai.hitData.hit_object == gameData.multi.players [gameData.multi.nLocalPlayer].objnum))) {
	if (gameData.ai.nHitType == HIT_NONE) {
		dot = VmVecDot (vec_to_player, &objP->orient.fvec);
		if (dot > field_of_view - (gameData.ai.nOverallAgitation << 9)) {
			return 2;
		} else {
			return 1;
		}
	} else {
		return 0;
	}
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
int do_silly_animation (object *objP)
{
	int				objnum = OBJ_IDX (objP);
	jointpos 		*jp_list;
	int				robot_type, nGun, robot_state, num_joint_positions;
	polyobj_info	*pobj_info = &objP->rtype.pobj_info;
	ai_static		*aip = &objP->ctype.ai_info;
	// ai_local			*ailp = &gameData.ai.localInfo [objnum];
	int				num_guns, at_goal;
	int				attack_type;
	int				flinch_attack_scale = 1;

	robot_type = objP->id;
	num_guns = gameData.bots.pInfo [robot_type].n_guns;
	attack_type = gameData.bots.pInfo [robot_type].attack_type;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goal_state, not GOAL_STATE.
	robot_state = Mike_to_matt_xlate [aip->GOAL_STATE];
	// previous_robot_state = Mike_to_matt_xlate [aip->CURRENT_STATE];

	if (attack_type) // && ((robot_state == AS_FIRE) || (robot_state == AS_RECOIL)))
		flinch_attack_scale = Attack_scale;
	else if ((robot_state == AS_FLINCH) || (robot_state == AS_RECOIL))
		flinch_attack_scale = Flinch_scale;

	at_goal = 1;
	for (nGun=0; nGun <= num_guns; nGun++) {
		int	joint;

		num_joint_positions = robot_get_anim_state (&jp_list, robot_type, nGun, robot_state);

		for (joint=0; joint<num_joint_positions; joint++) {
			fix			delta_angle, delta_2;
			int			jointnum = jp_list [joint].jointnum;
			vms_angvec	*jp = &jp_list [joint].angles;
			vms_angvec	*pObjP = &pobj_info->anim_angles [jointnum];

			if (jointnum >= gameData.models.polyModels [objP->rtype.pobj_info.model_num].n_models) {
				Int3 ();		// Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			if (jp->p != pObjP->p) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [objnum].goal_angles [jointnum].p = jp->p;

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

				gameData.ai.localInfo [objnum].delta_angles [jointnum].p = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->b != pObjP->b) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [objnum].goal_angles [jointnum].b = jp->b;

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

				gameData.ai.localInfo [objnum].delta_angles [jointnum].b = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->h != pObjP->h) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [objnum].goal_angles [jointnum].h = jp->h;

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

				gameData.ai.localInfo [objnum].delta_angles [jointnum].h = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//ai_static	*aip = &objP->ctype.ai_info;
			ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];
			ailp->achieved_state [nGun] = ailp->goal_state [nGun];
			if (ailp->achieved_state [nGun] == AIS_RECO)
				ailp->goal_state [nGun] = AIS_FIRE;

			if (ailp->achieved_state [nGun] == AIS_FLIN)
				ailp->goal_state [nGun] = AIS_LOCK;

		}
	}

	if (at_goal == 1) //num_guns)
		aip->CURRENT_STATE = aip->GOAL_STATE;

	return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-gameData.objs.objects in an object towards their goals.
//	Current orientation of object is at:	pobj_info.anim_angles
//	Goal orientation of object is at:		ai_info.goal_angles
//	Delta orientation of object is at:		ai_info.delta_angles
void ai_frame_animation (object *objP)
{
	int	objnum = OBJ_IDX (objP);
	int	joint;
	int	num_joints;

	num_joints = gameData.models.polyModels [objP->rtype.pobj_info.model_num].n_models;

	for (joint=1; joint<num_joints; joint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vms_angvec	*curangp = &objP->rtype.pobj_info.anim_angles [joint];
		vms_angvec	*goalangp = &gameData.ai.localInfo [objnum].goal_angles [joint];
		vms_angvec	*deltaangp = &gameData.ai.localInfo [objnum].delta_angles [joint];

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
void SetNextFireTime (object *objP, ai_local *ailp, robot_info *robptr, int nGun)
{
	//	For guys in snipe mode, they have a 50% shot of getting this shot in d_free.
	if ((nGun != 0) || (robptr->weapon_type2 == -1))
		if ((objP->ctype.ai_info.behavior != AIB_SNIPE) || (d_rand () > 16384))
			ailp->rapidfire_count++;

	//	Old way, 10/15/95: Continuous rapidfire if rapidfire_count set.
// -- 	if (( (robptr->weapon_type2 == -1) || (nGun != 0)) && (ailp->rapidfire_count < robptr->rapidfire_count [gameStates.app.nDifficultyLevel])) {
// -- 		ailp->next_fire = min (F1_0/8, robptr->firing_wait [gameStates.app.nDifficultyLevel]/2);
// -- 	} else {
// -- 		if ((robptr->weapon_type2 == -1) || (nGun != 0)) {
// -- 			ailp->rapidfire_count = 0;
// -- 			ailp->next_fire = robptr->firing_wait [gameStates.app.nDifficultyLevel];
// -- 		} else
// -- 			ailp->next_fire2 = robptr->firing_wait2 [gameStates.app.nDifficultyLevel];
// -- 	}

	if (( (nGun != 0) || (robptr->weapon_type2 == -1)) && (ailp->rapidfire_count < robptr->rapidfire_count [gameStates.app.nDifficultyLevel])) {
		ailp->next_fire = min (F1_0/8, robptr->firing_wait [gameStates.app.nDifficultyLevel]/2);
	} else {
		if ((robptr->weapon_type2 == -1) || (nGun != 0)) {
			ailp->next_fire = robptr->firing_wait [gameStates.app.nDifficultyLevel];
			if (ailp->rapidfire_count >= robptr->rapidfire_count [gameStates.app.nDifficultyLevel])
				ailp->rapidfire_count = 0;
		} else
			ailp->next_fire2 = robptr->firing_wait2 [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the player, they attack.
//	If player is cloaked, then robot probably didn't actually collide, deal with that here.
void DoAiRobotHitAttack (object *robot, object *playerobjP, vms_vector *collision_point)
{
	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (robot)];
	robot_info *robptr = &gameData.bots.pInfo [robot->id];

//#ifndef NDEBUG
	if (!gameStates.app.cheats.bRobotsFiring)
		return;
//#endif

	//	If player is dead, stop firing.
	if (gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum].type == OBJ_GHOST)
		return;

	if (robptr->attack_type == 1) {
		if (ailp->next_fire <= 0) {
			if (!(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED))
				if (VmVecDistQuick (&gameData.objs.console->pos, &robot->pos) < robot->size + gameData.objs.console->size + F1_0*2) {
					CollidePlayerAndNastyRobot (playerobjP, robot, collision_point);
					if (robptr->energy_drain && gameData.multi.players [gameData.multi.nLocalPlayer].energy) {
						gameData.multi.players [gameData.multi.nLocalPlayer].energy -= robptr->energy_drain * F1_0;
						if (gameData.multi.players [gameData.multi.nLocalPlayer].energy < 0)
							gameData.multi.players [gameData.multi.nLocalPlayer].energy = 0;
						// -- unused, use claw_sound in bitmaps.tbl -- DigiLinkSoundToPos (SOUND_ROBOT_SUCKED_PLAYER, playerobjP->segnum, 0, collision_point, 0, F1_0);
					}
				}

			robot->ctype.ai_info.GOAL_STATE = AIS_RECO;
			SetNextFireTime (robot, ailp, robptr, 1);	//	1 = nGun: 0 is special (uses next_fire2)
		}
	}

}

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		 (F1_0*4)
#define	MAX_LEAD_DISTANCE	 (F1_0*200)
#define	LEAD_RANGE			 (F1_0/2)

// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit player given positions, player vel, elapsed time
fix compute_lead_component (fix player_pos, fix robot_pos, fix player_vel, fix elapsed_time)
{
	return FixDiv (player_pos - robot_pos, elapsed_time) + player_vel;
}

// --------------------------------------------------------------------------------------------------------------------
//	Lead the player, returning point to fire at in vFirePoint.
//	Rules:
//		Player not cloaked
//		Player must be moving at a speed >= MIN_LEAD_SPEED
//		Player not farther away than MAX_LEAD_DISTANCE
//		dot (vector_to_player, player_direction) must be in -LEAD_RANGE,LEAD_RANGE
//		if firing a matter weapon, less leading, based on skill level.
int lead_player (object *objP, vms_vector *vFirePoint, vms_vector *vBelievedPlayerPos, int nGun, vms_vector *fire_vec)
{
	fix			dot, player_speed, dist_to_player, max_weapon_speed, projected_time;
	vms_vector	player_movement_dir, vec_to_player;
	int			nWeaponType;
	weapon_info	*wptr;
	robot_info	*robptr;

	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)
		return 0;

	player_movement_dir = gameData.objs.console->mtype.phys_info.velocity;
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
	nWeaponType = robptr->weapon_type;
	if (robptr->weapon_type2 != -1)
		if (nGun == 0)
			nWeaponType = robptr->weapon_type2;

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

	projected_time = FixDiv (dist_to_player, max_weapon_speed);

	fire_vec->x = compute_lead_component (vBelievedPlayerPos->x, vFirePoint->x, gameData.objs.console->mtype.phys_info.velocity.x, projected_time);
	fire_vec->y = compute_lead_component (vBelievedPlayerPos->y, vFirePoint->y, gameData.objs.console->mtype.phys_info.velocity.y, projected_time);
	fire_vec->z = compute_lead_component (vBelievedPlayerPos->z, vFirePoint->z, gameData.objs.console->mtype.phys_info.velocity.z, projected_time);

	VmVecNormalizeQuick (fire_vec);

	Assert (VmVecDot (fire_vec, &objP->orient.fvec) < 3*F1_0/2);

	//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
	if (VmVecDot (fire_vec, &objP->orient.fvec) < F1_0/2) {
		VmVecInc (fire_vec, &vec_to_player);
		VmVecScale (fire_vec, F1_0/2);
		if (VmVecDot (fire_vec, &objP->orient.fvec) < F1_0/2) {
			return 0;
		}
	}

	return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter vec_to_player is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
void AIFireLaserAtPlayer (object *objP, vms_vector *vFirePoint, int nGun, vms_vector *vBelievedPlayerPos)
{
	short			objnum = OBJ_IDX (objP);
	ai_local		*ailp = gameData.ai.localInfo + objnum;
	robot_info	*robptr = gameData.bots.pInfo + objP->id;
	vms_vector	fire_vec;
	vms_vector	bpp_diff;
	short			nWeaponType;
	fix			aim, dot;
	int			count;

//	If this robot is only awake because a camera woke it up, don't fire.
if (objP->ctype.ai_info.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
	return;
if (!gameStates.app.cheats.bRobotsFiring)
	return;
if (objP->control_type == CT_MORPH)
	return;
//	If player is exploded, stop firing.
if (gameStates.app.bPlayerExploded)
	return;
if (objP->ctype.ai_info.dying_start_time)
	return;		//	No firing while in death roll.
//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
//	If you try to key the boss off objP->ctype.ai_info.dying_start_time, it will hose the endlevel stuff.
if (gameData.boss.nDyingStartTime & gameData.bots.pInfo [objP->id].boss_flag)
	return;
//	If player is cloaked, maybe don't fire based on how long cloaked and randomness.
if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
	fix	xCloakTime = gameData.ai.cloakInfo [objnum % MAX_AI_CLOAK_INFO].last_time;
	if ((gameData.time.xGame - xCloakTime > CLOAK_TIME_MAX/4) &&
		 (d_rand () > FixDiv (gameData.time.xGame - xCloakTime, CLOAK_TIME_MAX)/2)) {
		SetNextFireTime (objP, ailp, robptr, nGun);
		return;
		}
	}
//	Handle problem of a robot firing through a wall because its gun tip is on the other
//	side of the wall than the robot's center.  For speed reasons, we normally only compute
//	the vector from the gun point to the player.  But we need to know whether the gun point
//	is separated from the robot's center by a wall.  If so, don't fire!
if (objP->ctype.ai_info.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
	//	Well, the gun point is in a different segment than the robot's center.
	//	This is almost always ok, but it is not ok if something solid is in between.
	int	nGunSeg = FindSegByPoint (vFirePoint, objP->segnum);
	//	See if these segments are connected, which should almost always be the case.
	short nConnSide = FindConnectedSide (&gameData.segs.segments [nGunSeg], &gameData.segs.segments [objP->segnum]);
	if (nConnSide != -1) {
		//	They are connected via nConnSide in segment objP->segnum.
		//	See if they are unobstructed.
		if (!(WALL_IS_DOORWAY (gameData.segs.segments + objP->segnum, nConnSide, NULL) & WID_FLY_FLAG)) {
			//	Can't fly through, so don't let this bot fire through!
			return;
			}
		}
	else {
		//	Well, they are not directly connected, so use FindVectorIntersection to see if they are unobstructed.
		fvi_query	fq;
		fvi_info		hit_data;
		int			fate;

		fq.startSeg				= objP->segnum;
		fq.p0						= &objP->pos;
		fq.p1						= vFirePoint;
		fq.rad					= 0;
		fq.thisObjNum			= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags					= FQ_TRANSWALL;

		fate = FindVectorIntersection (&fq, &hit_data);
		if (fate != HIT_NONE) {
			Int3 ();		//	This bot's gun is poking through a wall, so don't fire.
			move_towards_segment_center (objP);		//	And decrease chances it will happen again.
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
//	Lead the player half the time.
//	Note that when leading the player, aim is perfect.  This is probably acceptable since leading is so hacked in.
//	Problem is all robots will lead equally badly.
if (d_rand () < 16384) {
	if (lead_player (objP, vFirePoint, vBelievedPlayerPos, nGun, &fire_vec))		//	Stuff direction to fire at in vFirePoint.
		goto player_led;
}

dot = 0;
count = 0;			//	Don't want to sit in this loop foreverd:\temp\dm_test.
while ((count < 4) && (dot < F1_0/4)) {
	bpp_diff.x = vBelievedPlayerPos->x + FixMul ((d_rand ()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4, aim);
	bpp_diff.y = vBelievedPlayerPos->y + FixMul ((d_rand ()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4, aim);
	bpp_diff.z = vBelievedPlayerPos->z + FixMul ((d_rand ()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4, aim);
	VmVecNormalizedDirQuick (&fire_vec, &bpp_diff, vFirePoint);
	dot = VmVecDot (&objP->orient.fvec, &fire_vec);
	count++;
	}

player_led:

nWeaponType = robptr->weapon_type;
if ((robptr->weapon_type2 != -1) && ((nWeaponType < 0) || !nGun))
	nWeaponType = robptr->weapon_type2;
if (nWeaponType < 0)
	return;
CreateNewLaserEasy (&fire_vec, vFirePoint, OBJ_IDX (objP), (ubyte) nWeaponType, 1);
#ifdef NETWORK
if (gameData.app.nGameMode & GM_MULTI) {
	ai_multi_send_robot_position (objnum, -1);
	MultiSendRobotFire (objnum, objP->ctype.ai_info.CURRENT_GUN, &fire_vec);
}
#endif
CreateAwarenessEvent (objP, PA_NEARBY_ROBOT_FIRED);
SetNextFireTime (objP, ailp, robptr, nGun);
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
//	if dot_based set, then speed is based on direction of movement relative to heading
void move_towards_vector (object *objP, vms_vector *vec_goal, int dot_based)
{
	physics_info	*pptr = &objP->mtype.phys_info;
	fix				speed, dot, max_speed;
	robot_info		*robptr = &gameData.bots.pInfo [objP->id];
	vms_vector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards player as usual.

	vel = pptr->velocity;
	VmVecNormalizeQuick (&vel);
	dot = VmVecDot (&vel, &objP->orient.fvec);

	if (robptr->thief)
		dot = (F1_0+dot)/2;

	if (dot_based && (dot < 3*F1_0/4)) {
		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
		//	more quickly than the general code
		pptr->velocity.x = pptr->velocity.x/2 + FixMul (vec_goal->x, gameData.time.xFrame*32);
		pptr->velocity.y = pptr->velocity.y/2 + FixMul (vec_goal->y, gameData.time.xFrame*32);
		pptr->velocity.z = pptr->velocity.z/2 + FixMul (vec_goal->z, gameData.time.xFrame*32);
	} else {
		pptr->velocity.x += FixMul (vec_goal->x, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		pptr->velocity.y += FixMul (vec_goal->y, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		pptr->velocity.z += FixMul (vec_goal->z, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
	}

	speed = VmVecMagQuick (&pptr->velocity);
	max_speed = robptr->max_speed [gameStates.app.nDifficultyLevel];

	//	Green guy attacks twice as fast as he moves away.
	if ((robptr->attack_type == 1) || robptr->thief || robptr->kamikaze)
		max_speed *= 2;

	if (speed > max_speed) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_towards_player (object *objP, vms_vector *vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
move_towards_vector (objP, vec_to_player, 1);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means normal slide about.  fast_flag = 0 means no evasion.
void move_around_player (object *objP, vms_vector *vec_to_player, int fast_flag)
{
	physics_info	*pptr = &objP->mtype.phys_info;
	fix				speed;
	robot_info		*robptr = &gameData.bots.pInfo [objP->id];
	int				objnum = OBJ_IDX (objP);
	int				dir;
	int				dir_change;
	fix				ft;
	vms_vector		evade_vector;
	int				count=0;

	if (fast_flag == 0)
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

	dir = (gameData.app.nFrameCount + (count+1) * (objnum*8 + objnum*4 + objnum)) & dir_change;
	dir >>= (4+count);

	Assert ((dir >= 0) && (dir <= 3));

	switch (dir) {
		case 0:
			evade_vector.x = FixMul (vec_to_player->z, gameData.time.xFrame*32);
			evade_vector.y = FixMul (vec_to_player->y, gameData.time.xFrame*32);
			evade_vector.z = FixMul (-vec_to_player->x, gameData.time.xFrame*32);
			break;
		case 1:
			evade_vector.x = FixMul (-vec_to_player->z, gameData.time.xFrame*32);
			evade_vector.y = FixMul (vec_to_player->y, gameData.time.xFrame*32);
			evade_vector.z = FixMul (vec_to_player->x, gameData.time.xFrame*32);
			break;
		case 2:
			evade_vector.x = FixMul (-vec_to_player->y, gameData.time.xFrame*32);
			evade_vector.y = FixMul (vec_to_player->x, gameData.time.xFrame*32);
			evade_vector.z = FixMul (vec_to_player->z, gameData.time.xFrame*32);
			break;
		case 3:
			evade_vector.x = FixMul (vec_to_player->y, gameData.time.xFrame*32);
			evade_vector.y = FixMul (-vec_to_player->x, gameData.time.xFrame*32);
			evade_vector.z = FixMul (vec_to_player->z, gameData.time.xFrame*32);
			break;
		default:
			Error ("Function move_around_player: Bad case.");
	}

	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fast_flag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = VmVecDot (vec_to_player, &objP->orient.fvec);
		if ((dot > robptr->field_of_view [gameStates.app.nDifficultyLevel]) && 
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

			VmVecScale (&evade_vector, i2f (fast_flag) + damage_scale);
		}
	}

	pptr->velocity.x += evade_vector.x;
	pptr->velocity.y += evade_vector.y;
	pptr->velocity.z += evade_vector.z;

	speed = VmVecMagQuick (&pptr->velocity);
	if ((OBJ_IDX (objP) != 1) && (speed > robptr->max_speed [gameStates.app.nDifficultyLevel])) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_away_from_player (object *objP, vms_vector *vec_to_player, int attack_type)
{
	fix				speed;
	physics_info	*pptr = &objP->mtype.phys_info;
	robot_info		*robptr = &gameData.bots.pInfo [objP->id];
	int				objref;

	pptr->velocity.x -= FixMul (vec_to_player->x, gameData.time.xFrame*16);
	pptr->velocity.y -= FixMul (vec_to_player->y, gameData.time.xFrame*16);
	pptr->velocity.z -= FixMul (vec_to_player->z, gameData.time.xFrame*16);

	if (attack_type) {
		//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
		objref = ((OBJ_IDX (objP)) ^ ((gameData.app.nFrameCount + 3* (OBJ_IDX (objP))) >> 5)) & 3;

		switch (objref) {
			case 0:	VmVecScaleInc (&pptr->velocity, &objP->orient.uvec, gameData.time.xFrame << 5);	break;
			case 1:	VmVecScaleInc (&pptr->velocity, &objP->orient.uvec, -gameData.time.xFrame << 5);	break;
			case 2:	VmVecScaleInc (&pptr->velocity, &objP->orient.rvec, gameData.time.xFrame << 5);	break;
			case 3:	VmVecScaleInc (&pptr->velocity, &objP->orient.rvec, -gameData.time.xFrame << 5);	break;
			default:	Int3 ();	//	Impossible, bogus value on objref, must be in 0d:\temp\dm_test3
		}
	}


	speed = VmVecMagQuick (&pptr->velocity);

	if (speed > robptr->max_speed [gameStates.app.nDifficultyLevel]) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_STILL).
void ai_move_relative_to_player (object *objP, ai_local *ailp, fix dist_to_player, vms_vector *vec_to_player, fix circle_distance, int evade_only, int player_visibility)
{
	object		*dObjP;
	robot_info	*robptr = gameData.bots.pInfo + objP->id;

	Assert (player_visibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((robptr->attack_type == 0) && (objP->ctype.ai_info.danger_laser_num != -1)) {
if (objP->ctype.ai_info.danger_laser_num != -1) {
	dObjP = &gameData.objs.objects [objP->ctype.ai_info.danger_laser_num];

	if ((dObjP->type == OBJ_WEAPON) && (dObjP->signature == objP->ctype.ai_info.danger_laser_signature)) {
		fix			dot, dist_to_laser, field_of_view;
		vms_vector	vec_to_laser, laser_fvec;

		field_of_view = gameData.bots.pInfo [objP->id].field_of_view [gameStates.app.nDifficultyLevel];
		VmVecSub (&vec_to_laser, &dObjP->pos, &objP->pos);
		dist_to_laser = VmVecNormalizeQuick (&vec_to_laser);
		dot = VmVecDot (&vec_to_laser, &objP->orient.fvec);

		if ((dot > field_of_view) || (robptr->companion)) {
			fix			laser_robot_dot;
			vms_vector	laser_vec_to_robot;

			//	The laser is seen by the robot, see if it might hit the robot.
			//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
			if (dObjP->render_type == RT_POLYOBJ)
				laser_fvec = dObjP->orient.fvec;
			else {		//	Not a polyobjP, get velocity and normalize.
				laser_fvec = dObjP->mtype.phys_info.velocity;	//dObjP->orient.fvec;
				VmVecNormalizeQuick (&laser_fvec);
				}
			VmVecSub (&laser_vec_to_robot, &objP->pos, &dObjP->pos);
			VmVecNormalizeQuick (&laser_vec_to_robot);
			laser_robot_dot = VmVecDot (&laser_fvec, &laser_vec_to_robot);

			if ((laser_robot_dot > F1_0*7/8) && (dist_to_laser < F1_0*80)) {
				int	evade_speed = gameData.bots.pInfo [objP->id].evade_speed [gameStates.app.nDifficultyLevel];
				gameData.ai.bEvaded = 1;
				move_around_player (objP, vec_to_player, evade_speed);
				}
			}
		return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-type guys to keep coming, don't return here after evasion.
	if ((!robptr->attack_type) && (!robptr->thief) && evade_only)
		return;

	//	If we fall out of above, then no object to be avoided.
	objP->ctype.ai_info.danger_laser_num = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
if (robptr->attack_type == 1)
	if (( (ailp->next_fire > robptr->firing_wait [gameStates.app.nDifficultyLevel]/4) && (dist_to_player < F1_0*30)) || gameStates.app.bPlayerIsDead)
		//	1/4 of time, move around player, 3/4 of time, move away from player
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
	else if (dist_to_player < circle_distance)
		move_away_from_player (objP, vec_to_player, 0);
	else if ((dist_to_player < (3+objval)*circle_distance/2) && (ailp->next_fire > -F1_0))
		move_around_player (objP, vec_to_player, -1);
	else
		if ((-ailp->next_fire > F1_0 + (objval << 12)) && player_visibility)
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
void MakeRandomVector (vms_vector *vec)
{
	vec->x = (d_rand () - 16384) | 1;	// make sure we don't create null vector
	vec->y = d_rand () - 16384;
	vec->z = d_rand () - 16384;

	VmVecNormalizeQuick (vec);
}

#ifndef NDEBUG
void mprintf_animation_info (object *objP)
{
	ai_static	*aip = &objP->ctype.ai_info;
	ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];

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
	switch (ailp->player_awareness_type) {
		case AIE_FIRE: con_printf (CON_DEBUG, "FIRE "); break;
		case AIE_HITT: con_printf (CON_DEBUG, "HITT "); break;
		case AIE_COLL: con_printf (CON_DEBUG, "COLL "); break;
		case AIE_HURT: con_printf (CON_DEBUG, "HURT "); break;
	}
	con_printf (CON_DEBUG, "Next fire = %6.3f, Time = %6.3f\n", f2fl (ailp->next_fire), f2fl (ailp->player_awareness_time));
#endif
}
#endif

//	-------------------------------------------------------------------------------------------------------------------
int	Break_on_object = -1;

void do_firing_stuff (object *objP, int player_visibility, vms_vector *vec_to_player)
{
	if ((gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD) || 
		 (player_visibility >= 1)) {
		//	Now, if in robot's field of view, lock onto player
		fix	dot = VmVecDot (&objP->orient.fvec, vec_to_player);
		if ((dot >= 7*F1_0/8) || (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED)) {
			ai_static	*aip = &objP->ctype.ai_info;
			ai_local		*ailp = &gameData.ai.localInfo [OBJ_IDX (objP)];

			switch (aip->GOAL_STATE) {
				case AIS_NONE:
				case AIS_REST:
				case AIS_SRCH:
				case AIS_LOCK:
					aip->GOAL_STATE = AIS_FIRE;
					if (ailp->player_awareness_type <= PA_NEARBY_ROBOT_FIRED) {
						ailp->player_awareness_type = PA_NEARBY_ROBOT_FIRED;
						ailp->player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
					}
					break;
			}
		} else if (dot >= F1_0/2) {
			ai_static	*aip = &objP->ctype.ai_info;
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
void DoAiRobotHit (object *objP, int type)
{
	if (objP->control_type == CT_AI) {
		if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_PLAYER_COLLISION))
			switch (objP->ctype.ai_info.behavior) {
				case AIB_STILL:
				{
					int	r;

					//	Attack robots (eg, green guy) shouldn't have behavior = still.
					Assert (gameData.bots.pInfo [objP->id].attack_type == 0);

					r = d_rand ();
					//	1/8 time, charge player, 1/4 time create path, rest of time, do nothing
					if (r < 4096) {
						CreatePathToPlayer (objP, 10, 1);
						objP->ctype.ai_info.behavior = AIB_STATION;
						objP->ctype.ai_info.hide_segment = objP->segnum;
						gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_CHASE_OBJECT;
					} else if (r < 4096+8192) {
						create_n_segment_path (objP, d_rand ()/8192 + 2, -1);
						gameData.ai.localInfo [OBJ_IDX (objP)].mode = AIM_FOLLOW_PATH;
					}
					break;
				}
			}
	}

}
#ifndef NDEBUG
int	Do_ai_flag=1;
int	Cvv_test=0;
int	Cvv_last_time [MAX_OBJECTS];
int	Gun_point_hack=0;
#endif

int		Robot_sound_volume=DEFAULT_ROBOT_SOUND_VOLUME;

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely ObjectCanSeePlayer would benefit from the
//	information of a normalized vec_to_player.
//	Return player visibility:
//		0		not visible
//		1		visible, but robot not looking at player (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		player is cloaked
//	If the player is cloaked, set vec_to_player based on time player cloaked and last uncloaked position.
//	Updates ailp->previous_visibility if player is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to player_visibility
void ComputeVisAndVec (object *objP, vms_vector *pos, ai_local *ailp, vms_vector *vec_to_player, int *player_visibility, robot_info *robptr, int *flag)
{
	if (!*flag) {
		if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CLOAKED) {
			fix			delta_time, dist;
			int			cloak_index = (OBJ_IDX (objP)) % MAX_AI_CLOAK_INFO;

			delta_time = gameData.time.xGame - gameData.ai.cloakInfo [cloak_index].last_time;
			if (delta_time > F1_0*2) {
				vms_vector	randvec;

				gameData.ai.cloakInfo [cloak_index].last_time = gameData.time.xGame;
				MakeRandomVector (&randvec);
				VmVecScaleInc (&gameData.ai.cloakInfo [cloak_index].last_position, &randvec, 8*delta_time);
			}

			dist = VmVecNormalizedDirQuick (vec_to_player, &gameData.ai.cloakInfo [cloak_index].last_position, pos);
			*player_visibility = ObjectCanSeePlayer (objP, pos, robptr->field_of_view [gameStates.app.nDifficultyLevel], vec_to_player);
			// *player_visibility = 2;

			if ((ailp->next_misc_sound_time < gameData.time.xGame) && ((ailp->next_fire < F1_0) || (ailp->next_fire2 < F1_0)) && (dist < F1_0*20)) {
				ailp->next_misc_sound_time = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 1;
				DigiLinkSoundToPos (robptr->see_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
			}
		} else {
			//	Compute expensive stuff -- vec_to_player and player_visibility
			VmVecNormalizedDirQuick (vec_to_player, &gameData.ai.vBelievedPlayerPos, pos);
			if ((vec_to_player->x == 0) && (vec_to_player->y == 0) && (vec_to_player->z == 0)) {
				vec_to_player->x = F1_0;
			}
			*player_visibility = ObjectCanSeePlayer (objP, pos, robptr->field_of_view [gameStates.app.nDifficultyLevel], vec_to_player);

			//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
			//	see you without killing frame rate.
			{
				ai_static	*aip = &objP->ctype.ai_info;
			if ((*player_visibility == 2) && (ailp->previous_visibility != 2))
				if ((aip->GOAL_STATE == AIS_REST) || (aip->CURRENT_STATE == AIS_REST)) {
					aip->GOAL_STATE = AIS_FIRE;
					aip->CURRENT_STATE = AIS_FIRE;
				}
			}

			if ((ailp->previous_visibility != *player_visibility) && (*player_visibility == 2)) {
				if (ailp->previous_visibility == 0) {
					if (ailp->time_player_seen + F1_0/2 < gameData.time.xGame) {
						// -- if (gameStates.app.bPlayerExploded)
						// -- 	DigiLinkSoundToPos (robptr->taunt_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
						// -- else
							DigiLinkSoundToPos (robptr->see_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
						ailp->time_player_sound_attacked = gameData.time.xGame;
						ailp->next_misc_sound_time = gameData.time.xGame + F1_0 + d_rand ()*4;
					}
				} else if (ailp->time_player_sound_attacked + F1_0/4 < gameData.time.xGame) {
					// -- if (gameStates.app.bPlayerExploded)
					// -- 	DigiLinkSoundToPos (robptr->taunt_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
					// -- else
						DigiLinkSoundToPos (robptr->attack_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
					ailp->time_player_sound_attacked = gameData.time.xGame;
				}
			} 

			if ((*player_visibility == 2) && (ailp->next_misc_sound_time < gameData.time.xGame)) {
				ailp->next_misc_sound_time = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 2;
				// -- if (gameStates.app.bPlayerExploded)
				// -- 	DigiLinkSoundToPos (robptr->taunt_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
				// -- else
					DigiLinkSoundToPos (robptr->attack_sound, objP->segnum, 0, pos, 0 , Robot_sound_volume);
			}
			ailp->previous_visibility = *player_visibility;
		}

		*flag = 1;

		//	@mk, 09/21/95: If player view is not obstructed and awareness is at least as high as a nearby collision,
		//	act is if robot is looking at player.
		if (ailp->player_awareness_type >= PA_NEARBY_ROBOT_FIRED)
			if (*player_visibility == 1)
				*player_visibility = 2;
				
		if (*player_visibility) {
			ailp->time_player_seen = gameData.time.xGame;
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move the object objP to a spot in which it doesn't intersect a wall.
//	It might mean moving it outside its current segment.
void move_object_to_legal_spot (object *objP)
{
	vms_vector	original_pos = objP->pos;
	int		i;
	segment	*segp = &gameData.segs.segments [objP->segnum];

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		if (WALL_IS_DOORWAY (segp, (short) i, objP) & WID_FLY_FLAG) {
			vms_vector	segment_center, goal_dir;
			fix			dist_to_center;	// Value not used so far.

			COMPUTE_SEGMENT_CENTER_I (&segment_center, segp->children [i]);
			VmVecSub (&goal_dir, &segment_center, &objP->pos);
			dist_to_center = VmVecNormalizeQuick (&goal_dir);
			VmVecScale (&goal_dir, objP->size);
			VmVecInc (&objP->pos, &goal_dir);
			if (!ObjectIntersectsWall (objP)) {
				int	new_segnum = FindSegByPoint (&objP->pos, objP->segnum);

				if (new_segnum != -1) {
					RelinkObject (OBJ_IDX (objP), new_segnum);
					return;
				}
			} else
				objP->pos = original_pos;
		}
	}

	if (gameData.bots.pInfo [objP->id].boss_flag) {
		Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
		teleport_boss (objP);
	} else {
#if TRACE
		con_printf (CON_DEBUG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", OBJ_IDX (objP));
#endif
		ApplyDamageToRobot (objP, objP->shields*2, OBJ_IDX (objP));
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move object one object radii from current position towards segment center.
//	If segment center is nearer than 2 radii, move it to center.
void move_towards_segment_center (object *objP)
{
	int			segnum = objP->segnum;
	fix			dist_to_center;
	vms_vector	segment_center, goal_dir;

	COMPUTE_SEGMENT_CENTER_I (&segment_center, segnum);

	VmVecSub (&goal_dir, &segment_center, &objP->pos);
	dist_to_center = VmVecNormalizeQuick (&goal_dir);

	if (dist_to_center < objP->size) {
		//	Center is nearer than the distance we want to move, so move to center.
		objP->pos = segment_center;
#if TRACE
		con_printf (CON_DEBUG, "Object #%i moved to center of segment #%i (%7.3f %7.3f %7.3f)\n", OBJ_IDX (objP), objP->segnum, f2fl (objP->pos.x), f2fl (objP->pos.y), f2fl (objP->pos.z));
#endif
		if (ObjectIntersectsWall (objP)) {
#if TRACE
			con_printf (CON_DEBUG, "Object #%i still illegal, trying trickier move.\n");
#endif
			move_object_to_legal_spot (objP);
		}
	} else {
		int	new_segnum;
		//	Move one radii towards center.
		VmVecScale (&goal_dir, objP->size);
		VmVecInc (&objP->pos, &goal_dir);
		new_segnum = FindSegByPoint (&objP->pos, objP->segnum);
		if (new_segnum == -1) {
			objP->pos = segment_center;
			move_object_to_legal_spot (objP);
		}
	}

}

//int	Buddy_got_stuck = 0;

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type robot.
//	Brains, avoid robots, companions can open doors.
//	objP == NULL means treat as buddy.
int AIDoorIsOpenable (object *objP, segment *segp, short sidenum)
{
	short wall_num;
	wall	*wallP;

if (!IS_CHILD (segp->children [sidenum]))
	return 0;		//trap -2 (exit side)
wall_num = WallNumP (segp, sidenum);
if (!IS_WALL (wall_num))		//if there's no door at alld:\temp\dm_test.
	return 1;				//d:\temp\dm_testthen say it can't be opened
	//	The mighty console object can open all doors (for purposes of determining paths).
if (objP == gameData.objs.console) {
	if (gameData.walls.walls [wall_num].type == WALL_DOOR)
		return 1;
	}
wallP = gameData.walls.walls + wall_num;
if ((objP == NULL) || (gameData.bots.pInfo [objP->id].companion == 1)) {
	int	ailp_mode;

	if (wallP->flags & WALL_BUDDY_PROOF) {
		if ((wallP->type == WALL_DOOR) && (wallP->state == WALL_DOOR_CLOSED))
			return 0;
		else if (wallP->type == WALL_CLOSED)
			return 0;
		else if ((wallP->type == WALL_ILLUSION) && !(wallP->flags & WALL_ILLUSION_OFF))
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

	if (wallP->type == WALL_CLOSED)
		return 0;
	if (wallP->type != WALL_DOOR) /*&& (wallP->type != WALL_CLOSED))*/
		return 1;

	//	If Buddy is returning to player, don't let him think he can get through triggered doors.
	//	It's only valid to think that if the player is going to get him through.  But if he's
	//	going to the player, the player is probably on the opposite side.
	if (objP == NULL)
		ailp_mode = gameData.ai.localInfo [gameData.escort.nObjNum].mode;
	else
		ailp_mode = gameData.ai.localInfo [OBJ_IDX (objP)].mode;

	// -- if (Buddy_got_stuck) {
	if (ailp_mode == AIM_GOTO_PLAYER) {
		if ((wallP->type == WALL_BLASTABLE) && (wallP->state != WALL_BLASTED))
			return 0;
		if (wallP->type == WALL_CLOSED)
			return 0;
		if (wallP->type == WALL_DOOR) {
			if ((wallP->flags & WALL_DOOR_LOCKED) && (wallP->state == WALL_DOOR_CLOSED))
				return 0;
			}
		}
		// -- }

	if ((ailp_mode != AIM_GOTO_PLAYER) && (wallP->controlling_trigger != -1)) {
		int	clip_num = wallP->clip_num;

		if (clip_num == -1)
			return 1;
		else if (gameData.walls.pAnims [clip_num].flags & WCF_HIDDEN) {
			if (wallP->state == WALL_DOOR_CLOSED)
				return 0;
			else
				return 1;
			} 
		else
			return 1;
		}

	if (wallP->type == WALL_DOOR)  {
		if (wallP->type == WALL_BLASTABLE)
			return 1;
		else {
			int	clip_num = wallP->clip_num;

			if (clip_num == -1)
				return 1;
			//	Buddy allowed to go through secret doors to get to player.
			else if ((ailp_mode != AIM_GOTO_PLAYER) && (gameData.walls.pAnims [clip_num].flags & WCF_HIDDEN)) {
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
else if ((objP->id == ROBOT_BRAIN) || (objP->ctype.ai_info.behavior == AIB_RUN_FROM) || (objP->ctype.ai_info.behavior == AIB_SNIPE)) {
	if (IS_WALL (wall_num)) {
		if ((wallP->type == WALL_DOOR) && (wallP->keys == KEY_NONE) && !(wallP->flags & WALL_DOOR_LOCKED))
			return 1;
		else if (wallP->keys != KEY_NONE) {	//	Allow bots to open doors to which player has keys.
			if (wallP->keys & gameData.multi.players [gameData.multi.nLocalPlayer].flags)
				return 1;
			}
		}
	}
return 0;
}

//	-----------------------------------------------------------------------------------------------------------
//	Return side of openable door in segment, if any.  If none, return -1.
int openable_doors_in_segment (short segnum)
{
	ushort	i;

	if ((segnum < 0) || (segnum > gameData.segs.nLastSegment))
		return -1;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	wall_num = WallNumI (segnum, i);
		if (IS_WALL (wall_num)) {
			wall	*wallP = gameData.walls.walls + wall_num;
			if ((wallP->type == WALL_DOOR) && 
				 (wallP->keys == KEY_NONE) && 
				 (wallP->state == WALL_DOOR_CLOSED) && 
				 !(wallP->flags & WALL_DOOR_LOCKED) && 
				 !(gameData.walls.pAnims [wallP->clip_num].flags & WCF_HIDDEN))
				return i;
		}
	}

	return -1;

}

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Return true if a special object (player or control center) is in this segment.
// -- int special_object_in_seg (int segnum)
// -- {
// -- 	int	objnum;
// -- 
// -- 	objnum = gameData.segs.segments [segnum].objects;
// -- 
// -- 	while (objnum != -1) {
// -- 		if ((gameData.objs.objects [objnum].type == OBJ_PLAYER) || (gameData.objs.objects [objnum].type == OBJ_CNTRLCEN)) {
// -- 			return 1;
// -- 		} else
// -- 			objnum = gameData.objs.objects [objnum].next;
// -- 	}
// -- 
// -- 	return 0;
// -- }

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Randomly select a segment attached to *segp, reachable by flying.
// -- int get_random_child (int segnum)
// -- {
// -- 	int	sidenum;
// -- 	segment	*segp = &gameData.segs.segments [segnum];
// -- 
// -- 	sidenum = (rand () * 6) >> 15;
// -- 
// -- 	while (!(WALL_IS_DOORWAY (segp, sidenum) & WID_FLY_FLAG))
// -- 		sidenum = (rand () * 6) >> 15;
// -- 
// -- 	segnum = segp->children [sidenum];
// -- 
// -- 	return segnum;
// -- }

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an object of size size at pos *pos intersects a (player or robot or control center) in segment *segp.
int check_object_object_intersection (vms_vector *pos, fix size, segment *segp)
{
	int		curobjnum;

	//	If this would intersect with another object (only check those in this segment), then try to move.
	curobjnum = segp->objects;
	while (curobjnum != -1) {
		object *curObjP = &gameData.objs.objects [curobjnum];
		if ((curObjP->type == OBJ_PLAYER) || (curObjP->type == OBJ_ROBOT) || (curObjP->type == OBJ_CNTRLCEN)) {
			if (VmVecDistQuick (pos, &curObjP->pos) < size + curObjP->size)
				return 1;
		}
		curobjnum = curObjP->next;
	}

	return 0;

}

// --------------------------------------------------------------------------------------------------------------------
//	Return objnum if object created, else return -1.
//	If pos == NULL, pick random spot in segment.
int CreateGatedRobot (short segnum, ubyte object_id, vms_vector *pos)
{
	int			objnum, nTries = 5;
	object		*objP;
	segment		*segp = gameData.segs.segments + segnum;
	vms_vector	object_pos;
	robot_info	*robptr = &gameData.bots.pInfo [object_id];
	int			i, count=0;
	fix			objsize = gameData.models.polyModels [robptr->model_num].rad;
	ubyte			default_behavior;

	if (gameData.time.xGame - gameData.boss.nLastGateTime < gameData.boss.nGateInterval)
		return -1;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects [i].type == OBJ_ROBOT)
			if (gameData.objs.objects [i].matcen_creator == BOSS_GATE_MATCEN_NUM)
				count++;

	if (count > 2*gameStates.app.nDifficultyLevel + 6) {
		gameData.boss.nLastGateTime = gameData.time.xGame - 3*gameData.boss.nGateInterval/4;
		return -1;
	}

	COMPUTE_SEGMENT_CENTER (&object_pos, segp);
	for (;;) {
		if (!pos)
			PickRandomPointInSeg (&object_pos, SEG_IDX (segp));
		else
			object_pos = *pos;

		//	See if legal to place object here.  If not, move about in segment and try again.
		if (check_object_object_intersection (&object_pos, objsize, segp)) {
			if (!--nTries) {
				gameData.boss.nLastGateTime = gameData.time.xGame - 3*gameData.boss.nGateInterval/4;
				return -1;
				}
			pos = NULL;
			}
		else 
			break;
		}

	objnum = CreateObject (OBJ_ROBOT, object_id, -1, segnum, &object_pos, &vmdIdentityMatrix, objsize, CT_AI, MT_PHYSICS, RT_POLYOBJ, 0);

	if (objnum < 0) {
		gameData.boss.nLastGateTime = gameData.time.xGame - 3*gameData.boss.nGateInterval/4;
		return -1;
	} 
	// added lifetime increase depending on difficulty level 04/26/06 DM
	gameData.objs.objects [objnum].lifeleft = F1_0 * 30 + F0_5 * (gameStates.app.nDifficultyLevel * 15);	//	Gated in robots only live 30 seconds.

#ifdef NETWORK
	multiData.create.nObjNums [0] = objnum; // A convenient global to get objnum back to caller for multiplayer
#endif

	objP = gameData.objs.objects + objnum;

	//Set polygon-object-specific data

	objP->rtype.pobj_info.model_num = robptr->model_num;
	objP->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	objP->mtype.phys_info.mass = robptr->mass;
	objP->mtype.phys_info.drag = robptr->drag;

	objP->mtype.phys_info.flags |= (PF_LEVELLING);

	objP->shields = robptr->strength;
	objP->matcen_creator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.

	default_behavior = gameData.bots.pInfo [objP->id].behavior;
	InitAIObject (OBJ_IDX (objP), default_behavior, -1);		//	Note, -1 = segment this robot goes to to hide, should probably be something useful

	ObjectCreateExplosion (segnum, &object_pos, i2f (10), VCLIP_MORPHING_ROBOT);
	DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].sound_num, segnum, 0, &object_pos, 0 , F1_0);
	MorphStart (objP);

	gameData.boss.nLastGateTime = gameData.time.xGame;

	gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_level++;
	gameData.multi.players [gameData.multi.nLocalPlayer].num_robots_total++;

	return OBJ_IDX (objP);
}

//	----------------------------------------------------------------------------------------------------------
//	objP points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
int BossSpewRobot (object *objP, vms_vector *pos)
{
	short			objnum, segnum, objType, maxRobotTypes;
	short			boss_index, boss_id = gameData.bots.pInfo [objP->id].boss_flag;
	robot_info	*pri;

	boss_index = (boss_id >= BOSS_D2) ? boss_id - BOSS_D2 : boss_id;
	Assert ((boss_index >= 0) && (boss_index < NUM_D2_BOSSES));
	segnum = pos ? FindSegByPoint (pos, objP->segnum) : objP->segnum;
	if (segnum == -1) {
#if TRACE
		con_printf (CON_DEBUG, "Tried to spew a bot outside the mine! Aborting!\n");
#endif
		return -1;
	}	
	objType = spewBots [gameStates.app.bD1Mission][boss_index][ (maxSpewBots [boss_index] * d_rand ()) >> 15];
	if (objType == 255) {	// spawn an arbitrary robot
		maxRobotTypes = gameData.bots.nTypes [gameStates.app.bD1Mission];
		do {
				objType = d_rand () % maxRobotTypes;
			pri = gameData.bots.info [gameStates.app.bD1Mission] + objType;
			} while (pri->boss_flag ||	//well ... don't spawn another boss, huh? ;)
						pri->companion || //the buddy bot isn't exactly an enemy ... ^_^
						 (pri->score_value < 700)); //avoid spawning a ... spawn type bot
		}
	objnum = CreateGatedRobot (segnum, (ubyte) objType, pos);
	//	Make spewed robot come tumbling out as if blasted by a flash missile.
	if (objnum != -1) {
		object	*newObjP = &gameData.objs.objects [objnum];
		int		force_val;

		force_val = F1_0 / gameData.time.xFrame;

		if (force_val) {
			newObjP->ctype.ai_info.SKIP_AI_COUNT += force_val;
			newObjP->mtype.phys_info.rotthrust.x = ((d_rand () - 16384) * force_val)/16;
			newObjP->mtype.phys_info.rotthrust.y = ((d_rand () - 16384) * force_val)/16;
			newObjP->mtype.phys_info.rotthrust.z = ((d_rand () - 16384) * force_val)/16;
			newObjP->mtype.phys_info.flags |= PF_USES_THRUST;

			//	Now, give a big initial velocity to get moving away from boss.
			VmVecSub (&newObjP->mtype.phys_info.velocity, pos, &objP->pos);
			VmVecNormalizeQuick (&newObjP->mtype.phys_info.velocity);
			VmVecScale (&newObjP->mtype.phys_info.velocity, F1_0*128);
		}
	}

	return objnum;
}

// --------------------------------------------------------------------------------------------------------------------
//	Call this each time the player starts a new ship.
void InitAIForShip (void)
{
	int	i;

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		gameData.ai.cloakInfo [i].last_time = gameData.time.xGame;
		gameData.ai.cloakInfo [i].last_segment = gameData.objs.console->segnum;
		gameData.ai.cloakInfo [i].last_position = gameData.objs.console->pos;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Make object objP gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return objnum if robot successfully created, else return -1
int GateInRobot (short objnum, ubyte type, short segnum)
{
	if (segnum < 0) {
		int	i;
		
		for (i = 0; i < extraGameInfo [0].nBossCount; i++)
			if (objnum == gameData.boss.objList [i]) {
				segnum = gameData.boss.gateSegs [i][ (d_rand () * gameData.boss.nGateSegs [i]) >> 15];
				break;
				}
			}

	Assert ((segnum >= 0) && (segnum <= gameData.segs.nLastSegment));

	return CreateGatedRobot (segnum, type, NULL);
}

// --------------------------------------------------------------------------------------------------------------------
int boss_fits_in_seg (object *bossObjP, int segnum)
{
	vms_vector	segcenter;
	int			objList = OBJ_IDX (bossObjP);
	int			posnum;

	COMPUTE_SEGMENT_CENTER_I (&segcenter, segnum);

	for (posnum=0; posnum<9; posnum++) {
		if (posnum > 0) {
			vms_vector	vertex_pos;

			Assert ((posnum-1 >= 0) && (posnum-1 < 8));
			vertex_pos = gameData.segs.vertices [gameData.segs.segments [segnum].verts [posnum-1]];
			VmVecAvg (&bossObjP->pos, &vertex_pos, &segcenter);
		} else
			bossObjP->pos = segcenter;

		RelinkObject (objList, segnum);
		if (!ObjectIntersectsWall (bossObjP))
			return 1;
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
void teleport_boss (object *objP)
{
	short			i, rand_segnum = 0, rand_index, objnum;
	vms_vector	boss_dir;
	Assert (gameData.boss.nTeleportSegs [0] > 0);

	//	Pick a random segment from the list of boss-teleportable-to segments.
	objnum = OBJ_IDX (objP);
	for (i = 0; i < extraGameInfo [0].nBossCount; i++) {
		if (objnum == gameData.boss.objList [i]) {
			rand_index = (d_rand () * gameData.boss.nTeleportSegs [i]) >> 15;	
			rand_segnum = gameData.boss.teleportSegs [i][rand_index];
			Assert ((rand_segnum >= 0) && (rand_segnum <= gameData.segs.nLastSegment));
			break;
			}
		}

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendBossActions (OBJ_IDX (objP), 1, rand_segnum, 0);
#endif

	COMPUTE_SEGMENT_CENTER_I (&objP->pos, rand_segnum);
	RelinkObject (OBJ_IDX (objP), rand_segnum);

	gameData.boss.nLastTeleportTime = gameData.time.xGame;

	//	make boss point right at player
	VmVecSub (&boss_dir, &gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum].pos, &objP->pos);
	VmVector2Matrix (&objP->orient, &boss_dir, NULL, NULL);

	DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].sound_num, rand_segnum, 0, &objP->pos, 0 , F1_0);
	DigiKillSoundLinkedToObject (OBJ_IDX (objP));
	DigiLinkSoundToObject2 (gameData.bots.pInfo [objP->id].see_sound, OBJ_IDX (objP), 1, F1_0, F1_0*512);	//	F1_0*512 means play twice as loud

	//	After a teleport, boss can fire right away.
	gameData.ai.localInfo [OBJ_IDX (objP)].next_fire = 0;
	gameData.ai.localInfo [OBJ_IDX (objP)].next_fire2 = 0;

}

//	----------------------------------------------------------------------
void start_boss_death_sequence (object *objP)
{
	if (gameData.bots.pInfo [objP->id].boss_flag) {
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
//	Return true if object just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
int do_robot_dying_frame (object *objP, fix StartTime, fix roll_duration, sbyte *dying_sound_playing, short death_sound, fix expl_scale, fix sound_scale)
{
	fix	roll_val, temp;
	fix	sound_duration;

	if (!roll_duration)
		roll_duration = F1_0/4;

	roll_val = FixDiv (gameData.time.xGame - StartTime, roll_duration);

	fix_sincos (FixMul (roll_val, roll_val), &temp, &objP->mtype.phys_info.rotvel.x);
	fix_sincos (roll_val, &temp, &objP->mtype.phys_info.rotvel.y);
	fix_sincos (roll_val-F1_0/8, &temp, &objP->mtype.phys_info.rotvel.z);

	objP->mtype.phys_info.rotvel.x = (gameData.time.xGame - StartTime)/9;
	objP->mtype.phys_info.rotvel.y = (gameData.time.xGame - StartTime)/5;
	objP->mtype.phys_info.rotvel.z = (gameData.time.xGame - StartTime)/7;

	if (gameOpts->sound.digiSampleRate)
		sound_duration = FixDiv (gameData.pig.snd.pSounds [DigiXlatSound (death_sound)].length, gameOpts->sound.digiSampleRate);
	else
		sound_duration = F1_0;

	if (StartTime + roll_duration - sound_duration < gameData.time.xGame) {
		if (!*dying_sound_playing) {
#if TRACE
			con_printf (CON_DEBUG, "Starting death sound!\n");
#endif
			*dying_sound_playing = 1;
			DigiLinkSoundToObject2 (death_sound, OBJ_IDX (objP), 0, sound_scale, sound_scale*256);	//	F1_0*512 means play twice as loud
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
void StartRobotDeathSequence (object *objP)
{
	objP->ctype.ai_info.dying_start_time = gameData.time.xGame;
	objP->ctype.ai_info.dying_sound_playing = 0;
	objP->ctype.ai_info.SKIP_AI_COUNT = 0;

}

//	----------------------------------------------------------------------
void do_boss_dying_frame (object *objP)
{
	int	rval;

	rval = do_robot_dying_frame (objP, gameData.boss.nDyingStartTime, BOSS_DEATH_DURATION, 
										 &gameData.boss.bDyingSoundPlaying, 
										 gameData.bots.pInfo [objP->id].deathroll_sound, F1_0*4, F1_0*4);

	if (rval) {
		DoReactorDestroyedStuff (NULL);
		ExplodeObject (objP, F1_0/4);
		DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512);
		gameData.boss.nDying = 0;
	}
}

extern void RecreateThief (object *objP);

//	----------------------------------------------------------------------
int do_any_robot_dying_frame (object *objP)
{
	if (objP->ctype.ai_info.dying_start_time) {
		int	rval, death_roll;

		death_roll = gameData.bots.pInfo [objP->id].death_roll;
		rval = do_robot_dying_frame (objP, objP->ctype.ai_info.dying_start_time, min (death_roll/2+1,6)*F1_0, &objP->ctype.ai_info.dying_sound_playing, gameData.bots.pInfo [objP->id].deathroll_sound, death_roll*F1_0/8, death_roll*F1_0/2);

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
//	Called for an AI object if it is fairly aware of the player.
//	awareness_level is in 0d:\temp\dm_test100.  Larger numbers indicate greater awareness (eg, 99 if firing at player).
//	In a given frame, might not get called for an object, or might be called more than once.
//	The fact that this routine is not called for a given object does not mean that object is not interested in the player.
//	gameData.objs.objects are moved by physics, so they can move even if not interested in a player.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this player IS NOT allowed to move this robot.
//		1	this player IS allowed to move this robot.
int ai_multiplayer_awareness (object *objP, int awareness_level)
{
	int	rval=1;

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI) {
		if (awareness_level == 0)
			return 0;
		rval = MultiCanRemoveRobot (OBJ_IDX (objP), awareness_level);
	}
#endif

	return rval;

}

#ifndef NDEBUG
fix	Prev_boss_shields = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_boss_stuff (object *objP, int player_visibility)
{
	int	i, boss_id, boss_index, objnum, boss_alive = 0;

	boss_id = gameData.bots.pInfo [objP->id].boss_flag;
//	Assert ((boss_id >= BOSS_D2) && (boss_id < BOSS_D2 + NUM_D2_BOSSES));
	boss_index = (boss_id >= BOSS_D2) ? boss_id - BOSS_D2 : boss_id;
#ifndef NDEBUG
	if (objP->shields != Prev_boss_shields) {
#if TRACE
		con_printf (CON_DEBUG, "Boss shields = %7.3f, object %i\n", f2fl (objP->shields), OBJ_IDX (objP));
#endif
		Prev_boss_shields = objP->shields;
	}
#endif
	//	New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
	if (gameData.boss.nLastTeleportTime > gameData.time.xGame)
		gameData.boss.nLastTeleportTime = gameData.time.xGame;

	if (gameData.boss.nLastGateTime > gameData.time.xGame)
		gameData.boss.nLastGateTime = gameData.time.xGame;

	objnum = OBJ_IDX (objP);
	for (i = 0; i < extraGameInfo [0].nBossCount; i++)
		if (gameData.boss.objList [i] == objnum) {
			boss_alive = 1;
			break;
			}
	//	@mk, 10/13/95:  Reason:
	//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
	//		teleports out of there right away, and blasts player right after first door.
	if (!player_visibility && (gameData.time.xGame - gameData.boss.nHitTime > F1_0*2))
		return;

	if (boss_alive && bossProps [gameStates.app.bD1Mission][boss_index].bTeleports) {
		if (objP->ctype.ai_info.CLOAKED == 1) {
			gameData.boss.nHitTime = gameData.time.xGame;	//	Keep the cloak:teleport process going.
			if ((gameData.time.xGame - gameData.boss.nCloakStartTime > BOSS_CLOAK_DURATION/3) && 
				 (gameData.boss.nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION/3) && 
				 (gameData.time.xGame - gameData.boss.nLastTeleportTime > gameData.boss.nTeleportInterval)) {
				if (ai_multiplayer_awareness (objP, 98)) {

					teleport_boss (objP);
					if (bossProps [gameStates.app.bD1Mission][boss_index].bSpewBotsTeleport) {
#if 1
						vms_vector	spewPoint;
#	if 1
						VmVecCopyScale (&spewPoint, &objP->orient.fvec, objP->size * 2);
#	else
						VmVecCopyNormalize (&spewPoint, &objP->orient.fvec);
						VmVecScale (&spewPoint, objP->size * 2);
#	endif
						VmVecInc (&spewPoint, &objP->pos);
#endif
						if (bossProps [gameStates.app.bD1Mission][boss_index].bSpewMore && (d_rand () > 16384) &&
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
			objP->ctype.ai_info.CLOAKED = 0;
			}
		else if ((gameData.time.xGame - gameData.boss.nCloakEndTime > gameData.boss.nCloakInterval) || 
					  (gameData.time.xGame - gameData.boss.nCloakEndTime < -gameData.boss.nCloakDuration)) {
			if (ai_multiplayer_awareness (objP, 95)) {
				gameData.boss.nCloakStartTime = gameData.time.xGame;
				gameData.boss.nCloakEndTime = gameData.time.xGame+gameData.boss.nCloakDuration;
				objP->ctype.ai_info.CLOAKED = 1;
#ifdef NETWORK
				if (gameData.app.nGameMode & GM_MULTI)
					MultiSendBossActions (OBJ_IDX (objP), 2, 0, 0);
#endif
			}
		}
	}

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	 (F1_0*200)

// -- Obsolete D1 code -- // --------------------------------------------------------------------------------------------------------------------
// -- Obsolete D1 code -- //	Do special stuff for a boss.
// -- Obsolete D1 code -- void do_super_boss_stuff (object *objP, fix dist_to_player, int player_visibility)
// -- Obsolete D1 code -- {
// -- Obsolete D1 code -- 	static int eclip_state = 0;
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 	do_boss_stuff (objP, player_visibility);
// -- Obsolete D1 code -- 
// -- Obsolete D1 code -- 	// Only master player can cause gating to occur.
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
// -- Obsolete D1 code -- 			if (ai_multiplayer_awareness (objP, 99)) {
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

//int MultiCanRemoveRobot (object *objP, int awareness_level)
//{
//	return 0;
//}

void ai_multi_send_robot_position (short objnum, int force)
{
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI) 
	{
		if (force != -1)
			MultiSendRobotPosition (objnum, 1);
		else
			MultiSendRobotPosition (objnum, 0);
	}
#endif
	return;
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this object should be allowed to fire at the player.
int maybe_ai_do_actual_firing_stuff (object *objP, ai_static *aip)
{
#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI)
		if ((aip->GOAL_STATE != AIS_FLIN) && (objP->id != ROBOT_BRAIN))
			if (aip->CURRENT_STATE == AIS_FIRE)
				return 1;
#endif

	return 0;
}

vms_vector	Last_fired_upon_player_pos;

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if player is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
void ai_do_actual_firing_stuff (object *objP, ai_static *aip, ai_local *ailp, robot_info *robptr, vms_vector *vec_to_player, fix dist_to_player, vms_vector *gun_point, int player_visibility, int object_animates, int nGun)
{
	fix	dot;
	//robot_info *robptr = gameData.bots.pInfo + objP->id;

	if ((player_visibility == 2) || 
		 (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
		vms_vector	fire_pos;

		fire_pos = gameData.ai.vBelievedPlayerPos;

		//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
		//	So, fire at Last_fired_upon_player_pos instead of the player position.
		if (!robptr->attack_type && (player_visibility != 2))
			fire_pos = Last_fired_upon_player_pos;

		//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
		//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
		if (!object_animates || ready_to_fire (robptr, ailp)) {
			dot = VmVecDot (&objP->orient.fvec, vec_to_player);
			if ((dot >= 7*F1_0/8) || ((dot > F1_0/4) &&  robptr->boss_flag)) {

				if (nGun < robptr->n_guns) {
					if (robptr->attack_type == 1) {
						if (!gameStates.app.bPlayerExploded && (dist_to_player < objP->size + gameData.objs.console->size + F1_0*2)) {		// robptr->circle_distance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!ai_multiplayer_awareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAiRobotHitAttack (objP, gameData.objs.console, &objP->pos);
						} else {
							return;
						}
					} else {
						if ((gun_point->x == 0) && (gun_point->y == 0) && (gun_point->z == 0)) {
							;
						} else {
							if (!ai_multiplayer_awareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-type system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailp->next_fire <= 0) {
									AIFireLaserAtPlayer (objP, gun_point, nGun, &fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}

								if ((ailp->next_fire2 <= 0) && (robptr->weapon_type2 != -1)) {
									calc_gun_point (gun_point, objP, 0);
									AIFireLaserAtPlayer (objP, gun_point, 0, &fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}

							} else if (ailp->next_fire <= 0) {
								AIFireLaserAtPlayer (objP, gun_point, nGun, &fire_pos);
								Last_fired_upon_player_pos = fire_pos;
							}
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ((aip->behavior != AIB_RUN_FROM)
						 && (aip->behavior != AIB_STILL)
						 && (aip->behavior != AIB_SNIPE)
						 && (aip->behavior != AIB_FOLLOW)
						 && (!robptr->attack_type)
						 && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_STILL)))
						ailp->mode = AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = AIS_RECO;
				ailp->goal_state [aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.  If has 2 gun types, select gun #1, if exists.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->n_guns)
				{
					if ((robptr->n_guns == 1) || (robptr->weapon_type2 == -1))
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
				}
			}
		}
	} else if (( (!robptr->attack_type) &&
				   (gameData.weapons.info [robptr->weapon_type].homing_flag == 1)) || 
				  (( (robptr->weapon_type2 != -1) && 
				   (gameData.weapons.info [robptr->weapon_type2].homing_flag == 1)))) {
		fix dist;
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if (( (!object_animates) || (ailp->achieved_state [aip->CURRENT_GUN] == AIS_FIRE))
			 && (( (ailp->next_fire <= 0) && (aip->CURRENT_GUN != 0)) || ((ailp->next_fire2 <= 0) && (aip->CURRENT_GUN == 0)))
			 && ((dist = VmVecDistQuick (&gameData.ai.vHitPos, &objP->pos)) > F1_0*40)) {
			if (!ai_multiplayer_awareness (objP, ROBOT_FIRE_AGITATION))
				return;
			AIFireLaserAtPlayer (objP, gun_point, nGun, &gameData.ai.vBelievedPlayerPos);

			aip->GOAL_STATE = AIS_RECO;
			ailp->goal_state [aip->CURRENT_GUN] = AIS_RECO;

			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->n_guns)
				aip->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->n_guns)
				aip->CURRENT_GUN = 0;
		}
	} else {


	//	---------------------------------------------------------------

		vms_vector	vec_to_last_pos;

		if (d_rand ()/2 < FixMul (gameData.time.xFrame, (gameStates.app.nDifficultyLevel << 12) + 0x4000)) {
		if ((!object_animates || ready_to_fire (robptr, ailp)) && (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			VmVecNormalizedDirQuick (&vec_to_last_pos, &gameData.ai.vBelievedPlayerPos, &objP->pos);
			dot = VmVecDot (&objP->orient.fvec, &vec_to_last_pos);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < robptr->n_guns) {
					if (robptr->attack_type == 1) {
						if (!gameStates.app.bPlayerExploded && (dist_to_player < objP->size + gameData.objs.console->size + F1_0*2)) {		// robptr->circle_distance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!ai_multiplayer_awareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAiRobotHitAttack (objP, gameData.objs.console, &objP->pos);
						} else {
							return;
						}
					} else {
						if ((gun_point->x == 0) && (gun_point->y == 0) && (gun_point->z == 0)) {
							; 
						} else {
							if (!ai_multiplayer_awareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-type system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailp->next_fire <= 0)
									AIFireLaserAtPlayer (objP, gun_point, nGun, &Last_fired_upon_player_pos);

								if ((ailp->next_fire2 <= 0) && (robptr->weapon_type2 != -1)) {
									calc_gun_point (gun_point, objP, 0);
									AIFireLaserAtPlayer (objP, gun_point, 0, &Last_fired_upon_player_pos);
								}

							} else if (ailp->next_fire <= 0)
								AIFireLaserAtPlayer (objP, gun_point, nGun, &Last_fired_upon_player_pos);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ((aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_STILL)))
						ailp->mode = AIM_CHASE_OBJECT;
				}
				aip->GOAL_STATE = AIS_RECO;
				ailp->goal_state [aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->n_guns)
				{
					if (robptr->n_guns == 1)
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


