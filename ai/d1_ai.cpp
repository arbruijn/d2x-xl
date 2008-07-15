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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <stdlib.h>

#include "inferno.h"
#include "game.h"
#include "3d.h"

#include "object.h"
#include "robot.h"
#include "render.h"
#include "error.h"
#include "d1_ai.h"
#include "laser.h"
#include "fvi.h"
#include "polyobj.h"
#include "bm.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "fuelcen.h"
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
#include "editor\editor.h"
#endif

#ifndef NDEBUG
#include "string.h"
#include <time.h>
#endif

#define	JOHN_CHEATS_SIZE_1	6
#define	JOHN_CHEATS_SIZE_2	6
#define	JOHN_CHEATS_SIZE_3	6

ubyte	john_cheats_1[JOHN_CHEATS_SIZE_1] = { 	KEY_P ^ 0x00 ^ 0x34, 
															KEY_O ^ 0x10 ^ 0x34, 
															KEY_B ^ 0x20 ^ 0x34, 
															KEY_O ^ 0x30 ^ 0x34, 
															KEY_Y ^ 0x40 ^ 0x34, 
															KEY_S ^ 0x50 ^ 0x34 };

#define	PARALLAX	0		//	If !0, then special debugging info for Parallax eyes only enabled.

#define MIN_D 0x100

int	Flinch_scale = 4;
int	Attack_scale = 24;
#define	ANIM_RATE		(F1_0/16)
#define	DELTA_ANG_SCALE	16

byte D1_Mike_to_matt_xlate[] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

int	john_cheats_index_1;		//	POBOYS		detonate reactor
int	john_cheats_index_2;		//	PORGYS		high speed weapon firing
int	john_cheats_index_3;		//	LUNACY		lunacy (insane behavior, rookie firing)
int	john_cheats_index_4;		//	PLETCHnnn	paint robots

// int	No_D1_AI_flag=0;

#define	OVERALL_AGITATION_MAX	100

#define		D1_MAX_AI_CLOAK_INFO	8	//	Must be a power of 2!

#define	BOSS_CLOAK_DURATION	(F1_0*7)
#define	BOSS_DEATH_DURATION	(F1_0*6)
#define	BOSS_DEATH_SOUND_DURATION	0x2ae14		//	2.68 seconds

//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use gameData.time.xFrame because robots do not get moved every frame.
//fix	D1_AI_proc_time;

//	---------- John: These variables must be saved as part of gamesave. ----------
int				D1_AI_initialized = 0;
int				Overall_agitation;
tAILocal			ai_local_info[MAX_OBJECTS_D2];
tAICloakInfo	ai_cloak_info[D1_MAX_AI_CLOAK_INFO];


//	---------- John: End of variables which must be saved as part of gamesave. ----------

int	D1_AI_evaded=0;

//	0	mech
//	1	green claw
//	2	spider
//	3	josh
//	4	violet
//	5	cloak vulcan
//	6	cloak mech
//	7	brain
//	8	onearm
//	9	plasma
//	10	toaster
//	11	bird
//	12	missile bird
//	13	polyhedron
//	14	baby spider
//	15	mini boss
//	16	super mech
//	17	shareware boss
//	18	cloak-green	; note, gating in this guy benefits player, cloak objects
//	19	vulcan
//	20	toad
//	21	4-claw
//	22	quad-laser
// 23 super boss

// byte	D1_Super_boss_gate_list[] = {0, 1, 2, 9, 11, 16, 18, 19, 21, 22, 0, 9, 9, 16, 16, 18, 19, 19, 22, 22};
byte	D1_Super_boss_gate_list[] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};
#define	D1_MAX_GATE_INDEX	( sizeof(D1_Super_boss_gate_list) / sizeof(D1_Super_boss_gate_list[0]) )

int	D1_AI_info_enabled=0;

int	Ugly_robot_cheat = 0, Ugly_robot_texture = 0;
byte	Enable_john_cheat_1 = 0, Enable_john_cheat_2 = 0, Enable_john_cheat_3 = 0, Enable_john_cheat_4 = 0;

ubyte	john_cheats_3[2*JOHN_CHEATS_SIZE_3+1] = { KEY_Y ^ 0x67, 
																KEY_E ^ 0x66, 
																KEY_C ^ 0x65, 
																KEY_A ^ 0x64, 
																KEY_N ^ 0x63, 
																KEY_U ^ 0x62, 
																KEY_L ^ 0x61 };


#define	D1_MAX_AWARENESS_EVENTS	64
typedef struct awareness_event {
	short 		nSegment;				// tSegment the event occurred in
	short			type;					// type of event, defines behavior
	vmsVector	pos;					// absolute 3 space location of event
} awareness_event;


//	These globals are set by a call to FindVectorIntersection, which is a slow routine,
//	so we don't want to call it again (for this tObject) unless we have to.
vmsVector	Hit_pos;
int			hitType, Hit_seg;
tFVIData		hitData;

int					Num_awareness_events = 0;
awareness_event	Awareness_events[D1_MAX_AWARENESS_EVENTS];

#define	D1_AIS_MAX	8
#define	D1_D1_AIE_MAX	4

//--unused-- int	Processed_this_frame, LastFrameCount;
#ifndef NDEBUG
//	Index into this array with ailp->mode
char	mode_text[8][9] = {
	"STILL   ",
	"WANDER  ",
	"FOL_PATH",
	"CHASE_OB",
	"RUN_FROM",
	"HIDE    ",
	"FOL_PAT2",
	"OPENDOR2"
};

//	Index into this array with aip->behavior
char	behavior_text[6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

//	Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
char	state_text[8][5] = {
	"NONE",
	"REST",
	"SRCH",
	"LOCK",
	"FLIN",
	"FIRE",
	"RECO",
	"ERR_",
};


int	D1_AI_animation_test=0;
#endif

// Current state indicates where the robot current is, or has just done.
//	Transition table between states for an AI tObject.
//	 First dimension is trigger event.
//	Second dimension is current state.
//	 Third dimension is goal state.
//	Result is new goal state.
//	ERR_ means something impossible has happened.
byte D1_AI_transition_table[D1_AI_MAX_EVENT][D1_AI_MAX_STATE][D1_AI_MAX_STATE] = {
	{
	//	Event = AIE_FIRE, a nearby tObject fired
	//	none			rest			srch			lock			flin			fire			reco				// CURRENT is rows, GOAL is columns
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	none
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	rest
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	search
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	lock
	{	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},		//	flinch
	{	D1_AIS_ERR_,	D1_AIS_FIRE,	D1_AIS_FIRE,	D1_AIS_FIRE,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	fire
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}		//	recoil
	},

	//	Event = AIE_HITT, a nearby tObject was hit (or a wall was hit)
	{
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}
	},

	//	Event = AIE_COLL, player collided with robot
	{
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},
	{	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}
	},

	//	Event = AIE_HURT, player hurt robot (by firing at and hitting it)
	//	Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN},
	{	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN}
	}
};

ubyte	john_cheats_2[2*JOHN_CHEATS_SIZE_2] = { 	KEY_P ^ 0x00 ^ 0x43, 0x66, 
																KEY_O ^ 0x10 ^ 0x43, 0x11, 
																KEY_R ^ 0x20 ^ 0x43, 0x8, 
																KEY_G ^ 0x30 ^ 0x43, 0x2, 
																KEY_Y ^ 0x40 ^ 0x43, 0x0, 
																KEY_S ^ 0x50 ^ 0x43 };

// ---------------------------------------------------------
//	On entry, N_robot_types had darn sure better be set.
//	Mallocs N_robot_types tRobotInfo structs into global tRobotInfo.
void john_cheat_func_1(int key)
{
	if (!gameStates.app.cheats.bEnabled)
		return;

if (key == (john_cheats_1[john_cheats_index_1] ^ (john_cheats_index_1 << 4) ^ 0x34)) {
	john_cheats_index_1++;
	if (john_cheats_index_1 == JOHN_CHEATS_SIZE_1)	{
		DoReactorDestroyedStuff (NULL);
		john_cheats_index_1 = 0;
		DigiPlaySample (SOUND_CHEATER, F1_0);
		}
	} 
else
	john_cheats_index_1 = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
int D1_AI_behavior_to_mode(int behavior)
{
	switch (behavior) {
		case D1_AIB_STILL:			return D1_AIM_STILL;
		case D1_AIB_NORMAL:			return D1_AIM_CHASE_OBJECT;
		case D1_AIB_HIDE:				return D1_AIM_HIDE;
		case D1_AIB_RUN_FROM:		return D1_AIM_RUN_FROM_OBJECT;
		case D1_AIB_FOLLOW_PATH:	return D1_AIM_FOLLOW_PATH;
		case D1_AIB_STATION:			return D1_AIM_STILL;
		default:	Int3();	//	Contact Mike: Error, illegal behavior type
	}

	return D1_AIM_STILL;
}

int	Lunacy = 0;
int	Diff_save = 1;

fix	primaryFiringWaitCopy[MAX_ROBOT_TYPES];
byte	nRapidFireCountCopy[MAX_ROBOT_TYPES];

void do_lunacy_on(void)
{
	int	i;

	if ( !Lunacy )	{
		Lunacy = 1;
		Diff_save = gameStates.app.nDifficultyLevel;
		gameStates.app.nDifficultyLevel = NDL-1;

		for (i=0; i<MAX_ROBOT_TYPES; i++) {
			primaryFiringWaitCopy[i] = gameData.bots.info [1][i].primaryFiringWait[NDL-1];
			nRapidFireCountCopy[i] = gameData.bots.info [1][i].nRapidFireCount[NDL-1];
	
			gameData.bots.info [1][i].primaryFiringWait[NDL-1] = gameData.bots.info [1][i].primaryFiringWait[1];
			gameData.bots.info [1][i].nRapidFireCount[NDL-1] = gameData.bots.info [1][i].nRapidFireCount[1];
		}
	}
}

void do_lunacy_off(void)
{
	int	i;

	if ( Lunacy )	{
		Lunacy = 0;
		for (i=0; i<MAX_ROBOT_TYPES; i++) {
			gameData.bots.info [1][i].primaryFiringWait[NDL-1] = primaryFiringWaitCopy[i];
			gameData.bots.info [1][i].nRapidFireCount[NDL-1] = nRapidFireCountCopy[i];
		}
		gameStates.app.nDifficultyLevel = Diff_save;
	}
}

void john_cheat_func_3(int key)
{
	if (!gameStates.app.cheats.bEnabled)
		return;

	if (key == (john_cheats_3[JOHN_CHEATS_SIZE_3 - john_cheats_index_3] ^ (0x61 + john_cheats_index_3))) {
		if (john_cheats_index_3 == 4)
			john_cheats_index_3++;
		john_cheats_index_3++;
		if (john_cheats_index_3 == JOHN_CHEATS_SIZE_3+1) {
			if (Lunacy) {
				do_lunacy_off();
				HUDInitMessage( TXT_NO_LUNACY );
			} else {
				do_lunacy_on();
				HUDInitMessage( TXT_LUNACY );
				DigiPlaySample( SOUND_CHEATER, F1_0);
			}
			john_cheats_index_3 = 0;
		}
	} else
		john_cheats_index_3 = 0;
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void set_rotvel_and_saturate(fix *dest, fix delta)
{
	if ((delta ^ *dest) < 0) {
		if (abs(delta) < F1_0/8) {
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

#define	D1_AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

extern void PhysicsTurnTowardsVector(vmsVector *goal_vector, tObject *objP, fix rate);

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector(vmsVector *goal_vector, tObject *objP, fix rate)
{
	vmsVector	new_fVec;
	fix			dot;

	if ((objP->id == BABY_SPIDER_ID) && (objP->nType == OBJ_ROBOT)) {
		PhysicsTurnTowardsVector(goal_vector, objP, rate);
		return;
	}

	new_fVec = *goal_vector;

	dot = VmVecDot(goal_vector, &objP->position.mOrient.fVec);

	if (dot < (F1_0 - gameData.time.xFrame/2)) {
		fix	mag;
		fix	new_scale = FixDiv(gameData.time.xFrame * D1_AI_TURN_SCALE, rate);
		VmVecScale(&new_fVec, new_scale);
		VmVecInc(&new_fVec, &objP->position.mOrient.fVec);
		mag = VmVecNormalize(&new_fVec);
		if (mag < F1_0/256) {
			new_fVec = *goal_vector;		//	if degenerate vector, go right to goal
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
void ai_turn_randomly(vmsVector *vec_to_player, tObject *objP, fix rate, int nPrevVisibility)
{
	vmsVector	curVec;

	//	Random turning looks too stupid, so 1/4 of time, cheat.
	if (nPrevVisibility)
		if (rand() > 0x7400) {
			ai_turn_towards_vector(vec_to_player, objP, rate);
			return;
		}
//--debug-- 	if (rand() > 0x6000)
//--debug-- 		Prevented_turns++;

	curVec = objP->mType.physInfo.rotVel;

	curVec.p.y += F1_0/64;

	curVec.p.x += curVec.p.y/6;
	curVec.p.y += curVec.p.z/4;
	curVec.p.z += curVec.p.x/10;

	if (abs(curVec.p.x) > F1_0/8) curVec.p.x /= 4;
	if (abs(curVec.p.y) > F1_0/8) curVec.p.y /= 4;
	if (abs(curVec.p.z) > F1_0/8) curVec.p.z /= 4;

	objP->mType.physInfo.rotVel = curVec;

}

//	Overall_agitation affects:
//		Widens field of view.  Field of view is in range 0..1 (specified in bitmaps.tbl as N/360 degrees).
//			Overall_agitation/128 subtracted from field of view, making robots see wider.
//		Increases distance to which robot will search to create path to player by Overall_agitation/8 segments.
//		Decreases wait between fire times by Overall_agitation/64 seconds.

void john_cheat_func_4(int key)
{
	if (!gameStates.app.cheats.bEnabled)
		return;

	switch (john_cheats_index_4) {
		case 3:
			if (key == KEY_T)
				john_cheats_index_4++;
			else
				john_cheats_index_4 = 0;
			break;

		case 1:
			if (key == KEY_L)
				john_cheats_index_4++;
			else
				john_cheats_index_4 = 0;
			break;
	
		case 2:
			if (key == KEY_E)
				john_cheats_index_4++;
			else
				john_cheats_index_4 = 0;
			break;
	
		case 0:
			if (key == KEY_P)
				john_cheats_index_4++;
			break;


		case 4:
			if (key == KEY_C)
				john_cheats_index_4++;
			else
				john_cheats_index_4 = 0;
			break;
	
		case 5:
			if (key == KEY_H)
				john_cheats_index_4++;
			else
				john_cheats_index_4 = 0;
			break;
	
		case 6:
			Ugly_robot_texture = 0;
		case 7:
		case 8:
			if ((key >= KEY_1) && (key <= KEY_0)) {
				john_cheats_index_4++;
				Ugly_robot_texture *= 10;
				if (key != KEY_0)
					Ugly_robot_texture += key - 1;
				if (john_cheats_index_4 == 9) {
					if (Ugly_robot_texture == 999)	{
						Ugly_robot_cheat = 0;
						HUDInitMessage( TXT_ROBOT_PAINTING_OFF );
					} else {
						HUDInitMessage( TXT_ROBOT_PAINTING_ON, Ugly_robot_texture );
						Ugly_robot_cheat = 0xBADA55;
					}
					john_cheats_index_4 = 0;
				}
			} else
				john_cheats_index_4 = 0;
		
			break;
		default:
			john_cheats_index_4 = 0;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from tObject, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.vBelievedPlayerPos as player's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int player_is_visible_from_object(tObject *objP, vmsVector *pos, fix fieldOfView, vmsVector *vec_to_player)
{
	fix			dot;
	tFVIQuery	fq;

	fq.p0						= pos;
	if ((pos->p.x != objP->position.vPos.p.x) || (pos->p.y != objP->position.vPos.p.y) || (pos->p.z != objP->position.vPos.p.z)) {
		int	nSegment = FindSegByPos (pos, objP->nSegment, 1, 0);
		if (nSegment == -1) {
			fq.startSeg = objP->nSegment;
			*pos = objP->position.vPos;
			move_towards_segment_center(objP);
		} else
			fq.startSeg = nSegment;
	} else
		fq.startSeg		= objP->nSegment;
	fq.p1					= &gameData.ai.vBelievedPlayerPos;
	fq.radP0				= F1_0/4;
	fq.thisObjNum		= OBJ_IDX (objP);
	fq.ignoreObjList	= NULL;
	fq.flags				= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	hitType = FindVectorIntersection(&fq,&hitData);

	Hit_pos = hitData.hit.vPoint;
	Hit_seg = hitData.hit.nSegment;

	if ((hitType == HIT_NONE) || ((hitType == HIT_OBJECT) && (hitData.hit.nObject == LOCALPLAYER.nObject))) {
		dot = VmVecDot(vec_to_player, &objP->position.mOrient.fVec);
		if (dot > fieldOfView - (Overall_agitation << 9)) {
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
int do_silly_animation(tObject *objP)
{
	int				nObject = OBJ_IDX (objP);
	tJointPos 		*jp_list;
	int				robot_type, nGun, robotState, nJointPositions;
	tPolyObjInfo	*polyObjInfo = &objP->rType.polyObjInfo;
	tAIStatic		*aip = &objP->cType.aiInfo;
	// tAILocal			*ailp = &ai_local_info[nObject];
	int				num_guns, at_goal;
	int				attackType;
	int				flinch_attack_scale = 1;

	robot_type = objP->id;
	num_guns = gameData.bots.info [1][robot_type].nGuns;
	attackType = gameData.bots.info [1][robot_type].attackType;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goalState, not GOAL_STATE.
	robotState = D1_Mike_to_matt_xlate[aip->GOAL_STATE];
	// previous_robotState = D1_Mike_to_matt_xlate[aip->CURRENT_STATE];

	if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
		flinch_attack_scale = Attack_scale;
	else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
		flinch_attack_scale = Flinch_scale;

	at_goal = 1;
	for (nGun=0; nGun <= num_guns; nGun++) {
		int	joint;

		nJointPositions = RobotGetAnimState (&jp_list, robot_type, nGun, robotState);

		for (joint=0; joint<nJointPositions; joint++) {
			fix			delta_angle, delta_2;
			int			nJoint = jp_list[joint].jointnum;
			vmsAngVec	*jp = &jp_list[joint].angles;
			vmsAngVec	*pobjp = &polyObjInfo->animAngles[nJoint];

			if (nJoint >= gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels) {
				Int3();		// Contact Mike: incompatible data, illegal nJoint, problem in pof file?
				continue;
			}
			if (jp->p != pobjp->p) {
				if (nGun == 0)
					at_goal = 0;
				ai_local_info[nObject].goalAngles[nJoint].p = jp->p;

				delta_angle = jp->p - pobjp->p;
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

				ai_local_info[nObject].deltaAngles[nJoint].p = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->b != pobjp->b) {
				if (nGun == 0)
					at_goal = 0;
				ai_local_info[nObject].goalAngles[nJoint].b = jp->b;

				delta_angle = jp->b - pobjp->b;
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

				ai_local_info[nObject].deltaAngles[nJoint].b = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->h != pobjp->h) {
				if (nGun == 0)
					at_goal = 0;
				ai_local_info[nObject].goalAngles[nJoint].h = jp->h;

				delta_angle = jp->h - pobjp->h;
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

				ai_local_info[nObject].deltaAngles[nJoint].h = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//tAIStatic	*aip = &objP->cType.aiInfo;
			tAILocal		*ailp = &ai_local_info[OBJ_IDX (objP)];
			ailp->achievedState[nGun] = ailp->goalState[nGun];
			if (ailp->achievedState[nGun] == D1_AIS_RECO)
				ailp->goalState[nGun] = D1_AIS_FIRE;

			if (ailp->achievedState[nGun] == D1_AIS_FLIN)
				ailp->goalState[nGun] = D1_AIS_LOCK;

		}
	}

	if (at_goal == 1) //num_guns)
		aip->CURRENT_STATE = aip->GOAL_STATE;

	return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-objects in an tObject towards their goals.
//	Current orientation of tObject is at:	polyObjInfo.anim_angles
//	Goal orientation of tObject is at:		aiInfo.goalAngles
//	Delta orientation of tObject is at:		aiInfo.deltaAngles
void D1_AI_frame_animation(tObject *objP)
{
	int	nObject = OBJ_IDX (objP);
	int	joint;
	int	num_joints;

	num_joints = gameData.models.polyModels[objP->rType.polyObjInfo.nModel].nModels;

	for (joint=1; joint<num_joints; joint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vmsAngVec	*curangp = &objP->rType.polyObjInfo.animAngles[joint];
		vmsAngVec	*goalangp = &ai_local_info[nObject].goalAngles[joint];
		vmsAngVec	*deltaangp = &ai_local_info[nObject].deltaAngles[joint];

#ifndef NDEBUG
if (D1_AI_animation_test) {
	printf("%i: [%7.3f %7.3f %7.3f]  [%7.3f %7.3f %7.3f]\n", joint, f2fl(curangp->p), f2fl(curangp->b), f2fl(curangp->h), f2fl(goalangp->p), f2fl(goalangp->b), f2fl(goalangp->h), f2fl(curangp->p), f2fl(curangp->b), f2fl(curangp->h));
}
#endif
		delta_to_goal = goalangp->p - curangp->p;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul(deltaangp->p, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->p += scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				curangp->p = goalangp->p;
		}

		delta_to_goal = goalangp->b - curangp->b;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul(deltaangp->b, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->b += scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				curangp->b = goalangp->b;
		}

		delta_to_goal = goalangp->h - curangp->h;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul(deltaangp->h, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->h += scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				curangp->h = goalangp->h;
		}

	}

}

// ----------------------------------------------------------------------------------
void set_nextPrimaryFire_time(tAILocal *ailp, tRobotInfo *botInfoP)
{
	ailp->nRapidFireCount++;

	if (ailp->nRapidFireCount < botInfoP->nRapidFireCount[gameStates.app.nDifficultyLevel]) {
		ailp->nextPrimaryFire = min(F1_0/8, botInfoP->primaryFiringWait[gameStates.app.nDifficultyLevel]/2);
	} else {
		ailp->nRapidFireCount = 0;
		ailp->nextPrimaryFire = botInfoP->primaryFiringWait[gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the player, they attack.
//	If player is cloaked, then robot probably didn't actually collide, deal with that here.
void do_D1_AI_robot_hit_attack(tObject *robot, tObject *player, vmsVector *collision_point)
{
	tAILocal		*ailp = &ai_local_info[robot-OBJECTS];
	tRobotInfo *botInfoP = &gameData.bots.info [1][robot->id];

//#ifndef NDEBUG
	if (!Robot_firing_enabled)
		return;
//#endif

	//	If player is dead, stop firing.
	if (OBJECTS[LOCALPLAYER.nObject].nType == OBJ_GHOST)
		return;

	if (botInfoP->attackType == 1) {
		if (ailp->nextPrimaryFire <= 0) {
			if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
				if (VmVecDist(&gameData.objs.console->position.vPos, &robot->position.vPos) < robot->size + gameData.objs.console->size + F1_0*2)
					CollidePlayerAndNastyRobot ( player, robot, collision_point );

			robot->cType.aiInfo.GOAL_STATE = D1_AIS_RECO;
			set_nextPrimaryFire_time(ailp, botInfoP);
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------

void ai_multi_send_robot_position(int nObject, int force)
{
if (IsMultiGame) 
	MultiSendRobotPosition(nObject, force != -1);
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter vec_to_player is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
void ai_fire_laser_at_player(tObject *objP, vmsVector *fire_point)
{
	int			nObject = OBJ_IDX (objP);
	tAILocal		*ailp = &ai_local_info[nObject];
	tRobotInfo	*botInfoP = &gameData.bots.info [1][objP->id];
	vmsVector	fire_vec;
	vmsVector	bpp_diff;

	if (!Robot_firing_enabled)
		return;

#ifndef NDEBUG
	//	We should never be coming here for the green guy, as he has no laser!
	if (botInfoP->attackType == 1)
		Int3();	// Contact Mike: This is impossible.
#endif

	if (objP->controlType == CT_MORPH)
		return;

	//	If player is exploded, stop firing.
	if (gameStates.app.bPlayerExploded)
		return;

	//	If player is cloaked, maybe don't fire based on how long cloaked and randomness.
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	cloak_time = ai_cloak_info[nObject % D1_MAX_AI_CLOAK_INFO].lastTime;

		if (gameData.time.xGame - cloak_time > CLOAK_TIME_MAX/4)
			if (rand() > FixDiv(gameData.time.xGame - cloak_time, CLOAK_TIME_MAX)/2) {
				set_nextPrimaryFire_time(ailp, botInfoP);
				return;
			}
	}

//--	//	Find tSegment containing laser fire position.  If the robot is straddling a tSegment, the position from
//--	//	which it fires may be in a different tSegment, which is bad news for FindVectorIntersection.  So, cast
//--	//	a ray from the tObject center (whose tSegment we know) to the laser position.  Then, in the call to Laser_create_new
//--	//	use the data returned from this call to FindVectorIntersection.
//--	//	Note that while FindVectorIntersection is pretty slow, it is not terribly slow if the destination point is
//--	//	in the same tSegment as the source point.
//--
//--	fq.p0						= &objP->position.vPos;
//--	fq.startseg				= objP->nSegment;
//--	fq.p1						= fire_point;
//--	fq.rad					= 0;
//--	fq.thisobjnum			= OBJ_IDX (objP);
//--	fq.ignore_obj_list	= NULL;
//--	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???
//--
//--	fate = FindVectorIntersection(&fq, &hit_data);
//--	if (fate != HIT_NONE)
//--		return;

	//	Set position to fire at based on difficulty level.
	bpp_diff.p.x = gameData.ai.vBelievedPlayerPos.p.x + (rand()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4;
	bpp_diff.p.y = gameData.ai.vBelievedPlayerPos.p.y + (rand()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4;
	bpp_diff.p.z = gameData.ai.vBelievedPlayerPos.p.z + (rand()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4;

	//	Half the time fire at the player, half the time lead the player.
	if (rand() > 16384) {

		VmVecNormalizedDir(&fire_vec, &bpp_diff, fire_point);

	} else {
		vmsVector	player_direction_vector;

		VmVecSub(&player_direction_vector, &bpp_diff, &bpp_diff);

		// If player is not moving, fire right at him!
		//	Note: If the robot fires in the direction of its forward vector, this is bad because the weapon does not
		//	come out from the center of the robot; it comes out from the side.  So it is common for the weapon to miss
		//	its target.  Ideally, we want to point the guns at the player.  For now, just fire right at the player.
		if ((abs(player_direction_vector.p.x < 0x10000)) && (abs(player_direction_vector.p.y < 0x10000)) && (abs(player_direction_vector.p.z < 0x10000))) {

			VmVecNormalizedDir(&fire_vec, &bpp_diff, fire_point);

		// Player is moving.  Determine where the player will be at the end of the next frame if he doesn't change his
		//	behavior.  Fire at exactly that point.  This isn't exactly what you want because it will probably take the laser
		//	a different amount of time to get there, since it will probably be a different distance from the player.
		//	So, that's why we write games, instead of guiding missiles...
		} else {
			VmVecSub(&fire_vec, &bpp_diff, fire_point);
			VmVecScale(&fire_vec,FixMul(WI_speed (objP->id, gameStates.app.nDifficultyLevel), gameData.time.xFrame));

			VmVecInc(&fire_vec, &player_direction_vector);
			VmVecNormalize(&fire_vec);

		}
	}

	CreateNewLaserEasy ( &fire_vec, fire_point, OBJ_IDX (objP), botInfoP->nWeaponType, 1);

	if (IsMultiGame)
	{
		ai_multi_send_robot_position(nObject, -1);
		MultiSendRobotFire (nObject, objP->cType.aiInfo.CURRENT_GUN, &fire_vec);
	}

	create_awareness_event(objP, D1_PA_NEARBY_ROBOT_FIRED);

	set_nextPrimaryFire_time(ailp, botInfoP);

	//	If the boss fired, allow him to teleport very soon (right after firing, cool!), pending other factors.
	if (botInfoP->bossFlag)
		gameData.boss [0].nLastTeleportTime -= gameData.boss [0].nTeleportInterval/2;
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
void move_towards_vector(tObject *objP, vmsVector *vec_goal)
{
	tPhysicsInfo	*piP = &objP->mType.physInfo;
	fix				speed, dot, xMaxSpeed;
	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->id];
	vmsVector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards player as usual.

	vel = piP->velocity;
	VmVecNormalize(&vel);
	dot = VmVecDot(&vel, &objP->position.mOrient.fVec);

	if (dot < 3*F1_0/4) {
		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
		//	more quickly than the general code
		piP->velocity.p.x = piP->velocity.p.x/2 + FixMul(vec_goal->p.x, gameData.time.xFrame*32);
		piP->velocity.p.y = piP->velocity.p.y/2 + FixMul(vec_goal->p.y, gameData.time.xFrame*32);
		piP->velocity.p.z = piP->velocity.p.z/2 + FixMul(vec_goal->p.z, gameData.time.xFrame*32);
	} else {
		piP->velocity.p.x += FixMul(vec_goal->p.x, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		piP->velocity.p.y += FixMul(vec_goal->p.y, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		piP->velocity.p.z += FixMul(vec_goal->p.z, gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
	}

	speed = VmVecMag(&piP->velocity);
	xMaxSpeed = botInfoP->xMaxSpeed[gameStates.app.nDifficultyLevel];

	//	Green guy attacks twice as fast as he moves away.
	if (botInfoP->attackType == 1)
		xMaxSpeed *= 2;

	if (speed > xMaxSpeed) {
		piP->velocity.p.x = (piP->velocity.p.x*3)/4;
		piP->velocity.p.y = (piP->velocity.p.y*3)/4;
		piP->velocity.p.z = (piP->velocity.p.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_towards_player(tObject *objP, vmsVector *vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
	move_towards_vector(objP, vec_to_player);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means normal slide about.  fast_flag = 0 means no evasion.
void move_around_player(tObject *objP, vmsVector *vec_to_player, int fast_flag)
{
	tPhysicsInfo	*piP = &objP->mType.physInfo;
	fix				speed;
	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->id];
	int				nObject = OBJ_IDX (objP);
	int				dir;
	int				dir_change;
	fix				ft;
	vmsVector		evade_vector;
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

	dir = (gameData.app.nFrameCount + (count+1) * (nObject*8 + nObject*4 + nObject)) & dir_change;
	dir >>= (4+count);

	Assert((dir >= 0) && (dir <= 3));

	switch (dir) {
		case 0:
			evade_vector.p.x = FixMul(vec_to_player->p.z, gameData.time.xFrame*32);
			evade_vector.p.y = FixMul(vec_to_player->p.y, gameData.time.xFrame*32);
			evade_vector.p.z = FixMul(-vec_to_player->p.x, gameData.time.xFrame*32);
			break;
		case 1:
			evade_vector.p.x = FixMul(-vec_to_player->p.z, gameData.time.xFrame*32);
			evade_vector.p.y = FixMul(vec_to_player->p.y, gameData.time.xFrame*32);
			evade_vector.p.z = FixMul(vec_to_player->p.x, gameData.time.xFrame*32);
			break;
		case 2:
			evade_vector.p.x = FixMul(-vec_to_player->p.y, gameData.time.xFrame*32);
			evade_vector.p.y = FixMul(vec_to_player->p.x, gameData.time.xFrame*32);
			evade_vector.p.z = FixMul(vec_to_player->p.z, gameData.time.xFrame*32);
			break;
		case 3:
			evade_vector.p.x = FixMul(vec_to_player->p.y, gameData.time.xFrame*32);
			evade_vector.p.y = FixMul(-vec_to_player->p.x, gameData.time.xFrame*32);
			evade_vector.p.z = FixMul(vec_to_player->p.z, gameData.time.xFrame*32);
			break;
	}

	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fast_flag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = VmVecDot(vec_to_player, &objP->position.mOrient.fVec);
		if ((dot > botInfoP->fieldOfView[gameStates.app.nDifficultyLevel]) && !(gameData.objs.console->flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			damage_scale = FixDiv(objP->shields, botInfoP->strength);
			if (damage_scale > F1_0)
				damage_scale = F1_0;		//	Just in case...
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in case...

			VmVecScale(&evade_vector, i2f(fast_flag) + damage_scale);
		}
	}

	piP->velocity.p.x += evade_vector.p.x;
	piP->velocity.p.y += evade_vector.p.y;
	piP->velocity.p.z += evade_vector.p.z;

	speed = VmVecMag(&piP->velocity);
	if (speed > botInfoP->xMaxSpeed[gameStates.app.nDifficultyLevel]) {
		piP->velocity.p.x = (piP->velocity.p.x*3)/4;
		piP->velocity.p.y = (piP->velocity.p.y*3)/4;
		piP->velocity.p.z = (piP->velocity.p.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
void move_away_from_player(tObject *objP, vmsVector *vec_to_player, int attackType)
{
	fix				speed;
	tPhysicsInfo	*piP = &objP->mType.physInfo;
	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->id];
	int				objref;

	piP->velocity.p.x -= FixMul(vec_to_player->p.x, gameData.time.xFrame*16);
	piP->velocity.p.y -= FixMul(vec_to_player->p.y, gameData.time.xFrame*16);
	piP->velocity.p.z -= FixMul(vec_to_player->p.z, gameData.time.xFrame*16);

	if (attackType) {
		//	Get value in 0..3 to choose evasion direction.
		objref = (OBJ_IDX (objP) ^ ((gameData.app.nFrameCount + 3*OBJ_IDX (objP)) >> 5)) & 3;

		switch (objref) {
			case 0:	VmVecScaleInc(&piP->velocity, &objP->position.mOrient.uVec, gameData.time.xFrame << 5);	break;
			case 1:	VmVecScaleInc(&piP->velocity, &objP->position.mOrient.uVec, -gameData.time.xFrame << 5);	break;
			case 2:	VmVecScaleInc(&piP->velocity, &objP->position.mOrient.rVec, gameData.time.xFrame << 5);	break;
			case 3:	VmVecScaleInc(&piP->velocity, &objP->position.mOrient.rVec, -gameData.time.xFrame << 5);	break;
			default:	Int3();	//	Impossible, bogus value on objref, must be in 0..3
		}
	}


	speed = VmVecMag(&piP->velocity);

	if (speed > botInfoP->xMaxSpeed[gameStates.app.nDifficultyLevel]) {
		piP->velocity.p.x = (piP->velocity.p.x*3)/4;
		piP->velocity.p.y = (piP->velocity.p.y*3)/4;
		piP->velocity.p.z = (piP->velocity.p.z*3)/4;
	}

//--old--	fix				speed, dot;
//--old--	tPhysicsInfo	*piP = &objP->mType.physInfo;
//--old--	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->id];
//--old--
//--old--	//	Trying to move away from player.  If forward vector much different than velocity vector,
//--old--	//	bash velocity vector twice as much away from player as usual.
//--old--	dot = VmVecDot(&piP->velocity, &objP->position.mOrient.fVec);
//--old--	if (dot > -3*F1_0/4) {
//--old--		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
//--old--		//	more quickly than the general code
//--old--		piP->velocity.p.x = piP->velocity.p.x/2 - FixMul(vec_to_player->p.x, gameData.time.xFrame*16);
//--old--		piP->velocity.p.y = piP->velocity.p.y/2 - FixMul(vec_to_player->p.y, gameData.time.xFrame*16);
//--old--		piP->velocity.p.z = piP->velocity.p.z/2 - FixMul(vec_to_player->p.z, gameData.time.xFrame*16);
//--old--	} else {
//--old--		piP->velocity.p.x -= FixMul(vec_to_player->p.x, gameData.time.xFrame*16);
//--old--		piP->velocity.p.y -= FixMul(vec_to_player->p.y, gameData.time.xFrame*16);
//--old--		piP->velocity.p.z -= FixMul(vec_to_player->p.z, gameData.time.xFrame*16);
//--old--	}
//--old--
//--old--	speed = VmVecMag(&piP->velocity);
//--old--
//--old--	if (speed > botInfoP->xMaxSpeed[gameStates.app.nDifficultyLevel]) {
//--old--		piP->velocity.p.x = (piP->velocity.p.x*3)/4;
//--old--		piP->velocity.p.y = (piP->velocity.p.y*3)/4;
//--old--		piP->velocity.p.z = (piP->velocity.p.z*3)/4;
//--old--	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == D1_AIM_STILL).
void ai_move_relative_to_player(tObject *objP, tAILocal *ailp, fix dist_to_player, vmsVector *vec_to_player, fix circle_distance, int evade_only)
{
	tObject		*dobjp;
	tRobotInfo	*botInfoP = &gameData.bots.info [1][objP->id];

	//	See if should take avoidance.

// New way, green guys don't evade:	if ((botInfoP->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
	if (objP->cType.aiInfo.nDangerLaser != -1) {
		dobjp = &OBJECTS[objP->cType.aiInfo.nDangerLaser];

		if ((dobjp->nType == OBJ_WEAPON) && (dobjp->nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
			fix			dot, dist_to_laser, fieldOfView;
			vmsVector	vec_to_laser, laser_fVec;

			fieldOfView = gameData.bots.info [1][objP->id].fieldOfView[gameStates.app.nDifficultyLevel];

			VmVecSub(&vec_to_laser, &dobjp->position.vPos, &objP->position.vPos);
			dist_to_laser = VmVecNormalize(&vec_to_laser);
			dot = VmVecDot(&vec_to_laser, &objP->position.mOrient.fVec);

			if (dot > fieldOfView) {
				fix			laser_robot_dot;
				vmsVector	laser_vec_to_robot;

				//	The laser is seen by the robot, see if it might hit the robot.
				//	Get the laser's direction.  If it's a polyobj, it can be gotten cheaply from the orientation matrix.
				if (dobjp->renderType == RT_POLYOBJ)
					laser_fVec = dobjp->position.mOrient.fVec;
				else {		//	Not a polyobj, get velocity and normalize.
					laser_fVec = dobjp->mType.physInfo.velocity;	//dobjp->position.mOrient.fVec;
					VmVecNormalize(&laser_fVec);
				}
				VmVecSub(&laser_vec_to_robot, &objP->position.vPos, &dobjp->position.vPos);
				VmVecNormalize(&laser_vec_to_robot);
				laser_robot_dot = VmVecDot(&laser_fVec, &laser_vec_to_robot);

				if ((laser_robot_dot > F1_0*7/8) && (dist_to_laser < F1_0*80)) {
					int	evadeSpeed;

					D1_AI_evaded = 1;
					evadeSpeed = gameData.bots.info [1][objP->id].evadeSpeed[gameStates.app.nDifficultyLevel];

					move_around_player(objP, vec_to_player, evadeSpeed);
				}
			}
			return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-type guys to keep coming, don't return here after evasion.
	if ((!botInfoP->attackType) && evade_only)
		return;

	//	If we fall out of above, then no tObject to be avoided.
	objP->cType.aiInfo.nDangerLaser = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
	if (botInfoP->attackType == 1) {
		if (((ailp->nextPrimaryFire > botInfoP->primaryFiringWait[gameStates.app.nDifficultyLevel]/4) && (dist_to_player < F1_0*30)) || gameStates.app.bPlayerIsDead) {
			//	1/4 of time, move around player, 3/4 of time, move away from player
			if (rand() < 8192) {
				move_around_player(objP, vec_to_player, -1);
			} else {
				move_away_from_player(objP, vec_to_player, 1);
			}
		} else {
			move_towards_player(objP, vec_to_player);
		}
	} else {
		if (dist_to_player < circle_distance)
			move_away_from_player(objP, vec_to_player, 0);
		else if (dist_to_player < circle_distance*2)
			move_around_player(objP, vec_to_player, -1);
		else
			move_towards_player(objP, vec_to_player);
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Compute a somewhat random, normalized vector.
void make_random_vector(vmsVector *vec)
{
	vec->p.x = (rand() - 16384) | 1;	// make sure we don't create null vector
	vec->p.y = rand() - 16384;
	vec->p.z = rand() - 16384;

	VmVecNormalize(vec);
}

//	-------------------------------------------------------------------------------------------------------------------
int	Break_on_object = -1;

void do_firing_stuff(tObject *objP, int player_visibility, vmsVector *vec_to_player)
{
	if (player_visibility >= 1) {
		//	Now, if in robot's field of view, lock onto player
		fix	dot = VmVecDot(&objP->position.mOrient.fVec, vec_to_player);
		if ((dot >= 7*F1_0/8) || (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
			tAIStatic	*aip = &objP->cType.aiInfo;
			tAILocal		*ailp = &ai_local_info[OBJ_IDX (objP)];

			switch (aip->GOAL_STATE) {
				case D1_AIS_NONE:
				case D1_AIS_REST:
				case D1_AIS_SRCH:
				case D1_AIS_LOCK:
					aip->GOAL_STATE = D1_AIS_FIRE;
					if (ailp->playerAwarenessType <= D1_PA_NEARBY_ROBOT_FIRED) {
						ailp->playerAwarenessType = D1_PA_NEARBY_ROBOT_FIRED;
						ailp->playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
					}
					break;
			}
		} else if (dot >= F1_0/2) {
			tAIStatic	*aip = &objP->cType.aiInfo;
			switch (aip->GOAL_STATE) {
				case D1_AIS_NONE:
				case D1_AIS_REST:
				case D1_AIS_SRCH:
					aip->GOAL_STATE = D1_AIS_LOCK;
					break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robot gets bumped or hit, he decides to find another hiding place.
void do_D1_AI_robot_hit(tObject *objP, int type)
{
	if (objP->controlType == CT_AI) {
		if ((type == D1_PA_WEAPON_ROBOT_COLLISION) || (type == D1_PA_PLAYER_COLLISION))
			switch (objP->cType.aiInfo.behavior) {
				case D1_AIM_HIDE:
					objP->cType.aiInfo.SUBMODE = AISM_GOHIDE;
					break;
				case D1_AIM_STILL:
					ai_local_info[OBJ_IDX (objP)].mode = D1_AIM_CHASE_OBJECT;
					break;
			}
	}

}
#ifndef NDEBUG
int	Do_D1_AI_flag=1;
int	Cvv_test=0;
int	Cvv_nLastTime[MAX_OBJECTS];
int	Gun_point_hack=0;
#endif

#define	CHASE_TIME_LENGTH		(F1_0*8)
#define	DEFAULT_ROBOT_SOUND_VOLUME		F1_0
int		nRobotSoundVolume=DEFAULT_ROBOT_SOUND_VOLUME;

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely player_is_visible_from_object would benefit from the
//	information of a normalized vec_to_player.
//	Return player visibility:
//		0		not visible
//		1		visible, but robot not looking at player (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		player is cloaked
//	If the player is cloaked, set vec_to_player based on time player cloaked and last uncloaked position.
//	Updates ailp->nPrevVisibility if player is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to player_visibility
void compute_vis_and_vec(tObject *objP, vmsVector *pos, tAILocal *ailp, vmsVector *vec_to_player, int *player_visibility, tRobotInfo *botInfoP, int *flag)
{
	if (!*flag) {
		if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
			fix			delta_time, dist;
			int			cloak_index = OBJ_IDX (objP) % D1_MAX_AI_CLOAK_INFO;

			delta_time = gameData.time.xGame - ai_cloak_info[cloak_index].lastTime;
			if (delta_time > F1_0*2) {
				vmsVector	randvec;

				ai_cloak_info[cloak_index].lastTime = gameData.time.xGame;
				make_random_vector(&randvec);
				VmVecScaleInc(&ai_cloak_info[cloak_index].vLastPos, &randvec, 8*delta_time );
			}

			dist = VmVecNormalizedDir(vec_to_player, &ai_cloak_info[cloak_index].vLastPos, pos);
			*player_visibility = player_is_visible_from_object(objP, pos, botInfoP->fieldOfView[gameStates.app.nDifficultyLevel], vec_to_player);
			// *player_visibility = 2;

			if ((ailp->nextMiscSoundTime < gameData.time.xGame) && (ailp->nextPrimaryFire < F1_0) && (dist < F1_0*20)) {
				ailp->nextMiscSoundTime = gameData.time.xGame + (rand() + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 1;
				DigiLinkSoundToPos( botInfoP->seeSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
			}
		} else {
			//	Compute expensive stuff -- vec_to_player and player_visibility
			VmVecNormalizedDir(vec_to_player, &gameData.ai.vBelievedPlayerPos, pos);
			if ((vec_to_player->p.x == 0) && (vec_to_player->p.y == 0) && (vec_to_player->p.z == 0)) {
				vec_to_player->p.x = F1_0;
			}
			*player_visibility = player_is_visible_from_object(objP, pos, botInfoP->fieldOfView[gameStates.app.nDifficultyLevel], vec_to_player);

			//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
			//	see you without killing frame rate.
			{
				tAIStatic	*aip = &objP->cType.aiInfo;
			if ((*player_visibility == 2) && (ailp->nPrevVisibility != 2))
				if ((aip->GOAL_STATE == D1_AIS_REST) || (aip->CURRENT_STATE == D1_AIS_REST)) {
					aip->GOAL_STATE = D1_AIS_FIRE;
					aip->CURRENT_STATE = D1_AIS_FIRE;
				}
			}

			if (!gameStates.app.bPlayerExploded && (ailp->nPrevVisibility != *player_visibility) && (*player_visibility == 2)) {
				if (ailp->nPrevVisibility == 0) {
					if (ailp->timePlayerSeen + F1_0/2 < gameData.time.xGame) {
						DigiLinkSoundToPos( botInfoP->seeSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
						ailp->timePlayerSoundAttacked = gameData.time.xGame;
						ailp->nextMiscSoundTime = gameData.time.xGame + F1_0 + rand()*4;
					}
				} else if (ailp->timePlayerSoundAttacked + F1_0/4 < gameData.time.xGame) {
					DigiLinkSoundToPos( botInfoP->attackSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
					ailp->timePlayerSoundAttacked = gameData.time.xGame;
				}
			} 

			if ((*player_visibility == 2) && (ailp->nextMiscSoundTime < gameData.time.xGame)) {
				ailp->nextMiscSoundTime = gameData.time.xGame + (rand() + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 2;
				DigiLinkSoundToPos( botInfoP->attackSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
			}
			ailp->nPrevVisibility = *player_visibility;
		}

		*flag = 1;

		if (*player_visibility) {
			ailp->timePlayerSeen = gameData.time.xGame;
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move the tObject objP to a spot in which it doesn't intersect a wall.
//	It might mean moving it outside its current tSegment.
void move_object_to_legal_spot(tObject *objP)
{
	vmsVector	original_pos = objP->position.vPos;
	int		i;
	tSegment	*segP = &gameData.segs.segments[objP->nSegment];

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		if (WALL_IS_DOORWAY(segP, i, objP) & WID_FLY_FLAG) {
			vmsVector	vSegCenter, goal_dir;
			fix			dist_to_center;

			COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->nSegment);
			VmVecSub(&goal_dir, &vSegCenter, &objP->position.vPos);
			dist_to_center = VmVecNormalize(&goal_dir);
			VmVecScale(&goal_dir, objP->size);
			VmVecInc(&objP->position.vPos, &goal_dir);
			if (!ObjectIntersectsWall(objP)) {
				int	nNewSeg = FindSegByPos (&objP->position.vPos, objP->nSegment, 1, 0);

				if (nNewSeg != -1) {
					RelinkObject(OBJ_IDX (objP), nNewSeg);
					return;
				}
			} else
				objP->position.vPos = original_pos;
		}
	}

	// Int3();		//	Darn you John, you done it again!  (But contact Mike)
	ApplyDamageToRobot(objP, objP->shields*2, OBJ_IDX (objP));
}

// --------------------------------------------------------------------------------------------------------------------
//	Move tObject one tObject radii from current position towards tSegment center.
//	If tSegment center is nearer than 2 radii, move it to center.
void move_towards_segment_center(tObject *objP)
{
	int			nSegment = objP->nSegment;
	fix			dist_to_center;
	vmsVector	vSegCenter, goal_dir;

	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->nSegment);
	VmVecSub(&goal_dir, &vSegCenter, &objP->position.vPos);
	dist_to_center = VmVecNormalize(&goal_dir);
	if (dist_to_center < objP->size) {
		//	Center is nearer than the distance we want to move, so move to center.
		objP->position.vPos = vSegCenter;
		if (ObjectIntersectsWall(objP)) {
			move_object_to_legal_spot(objP);
		}
	} else {
		int	nNewSeg;
		//	Move one radii towards center.
		VmVecScale(&goal_dir, objP->size);
		VmVecInc(&objP->position.vPos, &goal_dir);
		nNewSeg = FindSegByPos (&objP->position.vPos, objP->nSegment, 1, 0);
		if (nNewSeg == -1) {
			objP->position.vPos = vSegCenter;
			move_object_to_legal_spot(objP);
		}
	}

}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type robot.
//	Only brains and avoid robots can open doors.
int D1_AI_door_is_openable(tObject *objP, tSegment *segP, int sidenum)
{
	int	nWall;

	//	The mighty console tObject can open all doors (for purposes of determining paths).
	if (objP == gameData.objs.console) {
		int	nWall = segP->sides[sidenum].nWall;

		if (WALLS[nWall].nType == WALL_DOOR)
			return 1;
	}

	if ((objP->id == ROBOT_BRAIN) || (objP->cType.aiInfo.behavior == D1_AIB_RUN_FROM)) {
		nWall = segP->sides[sidenum].nWall;

		if (nWall != -1)
			if ((WALLS[nWall].nType == WALL_DOOR) && (WALLS[nWall].keys == KEY_NONE) && !(WALLS[nWall].flags & WALL_DOOR_LOCKED))
				return 1;
	}

	return 0;
}

//--//	-----------------------------------------------------------------------------------------------------------
//--//	Return true if tObject *objP is allowed to open door at nWall
//--int door_openable_by_robot(tObject *objP, int nWall)
//--{
//--	if (objP->id == ROBOT_BRAIN)
//--		if (WALLS[nWall].keys == KEY_NONE)
//--			return 1;
//--
//--	return 0;
//--}

//	-----------------------------------------------------------------------------------------------------------
//	Return side of openable door in tSegment, if any.  If none, return -1.
int openable_doors_in_segment(tObject *objP)
{
	int	i;
	int	nSegment = objP->nSegment;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		if (gameData.segs.segments[nSegment].sides[i].nWall != -1) {
			int	nWall = gameData.segs.segments[nSegment].sides[i].nWall;
			if ((WALLS[nWall].nType == WALL_DOOR) && (WALLS[nWall].keys == KEY_NONE) && (WALLS[nWall].state == WALL_DOOR_CLOSED) && !(WALLS[nWall].flags & WALL_DOOR_LOCKED))
				return i;
		}
	}

	return -1;

}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if a special tObject (player or control center) is in this tSegment.
int special_object_in_seg(int nSegment)
{
	int	nObject;

	nObject = gameData.segs.segments[nSegment].objList;

	while (nObject != -1) {
		if ((OBJECTS[nObject].nType == OBJ_PLAYER) || (OBJECTS[nObject].nType == OBJ_REACTOR)) {
			return 1;
		} else
			nObject = OBJECTS[nObject].next;
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Randomly select a tSegment attached to *segP, reachable by flying.
int get_random_child(int nSegment)
{
	int	sidenum;
	tSegment	*segP = &gameData.segs.segments[nSegment];

	sidenum = (rand() * 6) >> 15;

	while (!(WALL_IS_DOORWAY(segP, sidenum, NULL) & WID_FLY_FLAG))
		sidenum = (rand() * 6) >> 15;

	nSegment = segP->children[sidenum];

	return nSegment;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an tObject of size size at pos *pos intersects a (player or robot or control center) in tSegment *segP.
int CheckObjectObjectIntersection(vmsVector *pos, fix size, tSegment *segP)
{
	int		nCurObj;

	//	If this would intersect with another tObject (only check those in this tSegment), then try to move.
	nCurObj = segP->objList;
	while (nCurObj != -1) {
		tObject *curObjP = &OBJECTS[nCurObj];
		if ((curObjP->nType == OBJ_PLAYER) || (curObjP->nType == OBJ_ROBOT) || (curObjP->nType == OBJ_REACTOR)) {
			if (VmVecDist(pos, &curObjP->position.vPos) < size + curObjP->size)
				return 1;
		}
		nCurObj = curObjP->next;
	}

	return 0;

}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if tObject created, else return false.
int create_gated_robot( int nSegment, int object_id)
{
	int		nObject;
	tObject	*objP;
	tSegment	*segP = &gameData.segs.segments[nSegment];
	vmsVector	vObjPos;
	tRobotInfo	*botInfoP = &gameData.bots.info [1][object_id];
	int		i, count=0;
	fix		objsize = gameData.models.polyModels[botInfoP->nModel].rad;
	int		default_behavior;

	for (i=0; i<=gameData.objs.nLastObject [0]; i++)
		if (OBJECTS[i].nType == OBJ_ROBOT)
			if (OBJECTS[i].matCenCreator == BOSS_GATE_MATCEN_NUM)
				count++;

	if (count > 2*gameStates.app.nDifficultyLevel + 3) {
		gameData.boss [0].nLastGateTime = gameData.time.xGame - 3*gameData.boss [0].nGateInterval/4;
		return 0;
	}

	COMPUTE_SEGMENT_CENTER (&vObjPos, segP);
	PickRandomPointInSeg(&vObjPos, segP-gameData.segs.segments);

	//	See if legal to place tObject here.  If not, move about in tSegment and try again.
	if (CheckObjectObjectIntersection(&vObjPos, objsize, segP)) {
		gameData.boss [0].nLastGateTime = gameData.time.xGame - 3*gameData.boss [0].nGateInterval/4;
		return 0;
	}

	nObject = CreateObject (OBJ_ROBOT, object_id, -1, nSegment, &vObjPos, &vmdIdentityMatrix, objsize, CT_AI, MT_PHYSICS, RT_POLYOBJ, 0);

	if ( nObject < 0 ) {
		gameData.boss [0].nLastGateTime = gameData.time.xGame - 3*gameData.boss [0].nGateInterval/4;
		return -1;
	}

	objP = &OBJECTS[nObject];

	//Set polygon-tObject-specific data

	objP->rType.polyObjInfo.nModel = botInfoP->nModel;
	objP->rType.polyObjInfo.nSubObjFlags = 0;

	//set Physics info

	objP->mType.physInfo.mass = botInfoP->mass;
	objP->mType.physInfo.drag = botInfoP->drag;

	objP->mType.physInfo.flags |= (PF_LEVELLING);

	objP->shields = botInfoP->strength;
	objP->matCenCreator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.

	default_behavior = D1_AIB_NORMAL;
	if (object_id == 10)						//	This is a toaster guy!
		default_behavior = D1_AIB_RUN_FROM;

	InitAIObject (OBJ_IDX (objP), default_behavior, -1 );		//	Note, -1 = tSegment this robot goes to to hide, should probably be something useful

	ObjectCreateExplosion (nSegment, &vObjPos, i2f(10), VCLIP_MORPHING_ROBOT );
	DigiLinkSoundToPos( Vclip[VCLIP_MORPHING_ROBOT].nSound, nSegment, 0, &vObjPos, 0 , F1_0);
	MorphStart (objP);

	gameData.boss [0].nLastGateTime = gameData.time.xGame;
	LOCALPLAYER.numRobotsLevel++;
	LOCALPLAYER.numRobotsTotal++;
	return nObject;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make tObject objP gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return true if robot successfully created, else return false
int gate_in_robot(int type, int nSegment)
{
	if (nSegment < 0)
		nSegment = gameData.boss [0].gateSegs[(rand() * gameData.boss [0].nGateSegs) >> 15];

	Assert((nSegment >= 0) && (nSegment <= gameData.segments.nLastSegment));

	return create_gated_robot(nSegment, type);
}

// --------------------------------------------------------------------------------------------------------------------
int boss_fits_in_seg(tObject *boss_objp, int nSegment)
{
	vmsVector	segcenter;
	int			boss_objnum = boss_objp-OBJECTS;
	int			posnum;

	COMPUTE_SEGMENT_CENTER_I (&segcenter, nSegment);

	for (posnum=0; posnum<9; posnum++) {
		if (posnum > 0) {
			vmsVector	vertex_pos;

			Assert((posnum-1 >= 0) && (posnum-1 < 8));
			vertex_pos = gameData.segs.vertices[gameData.segs.segments[nSegment].verts[posnum-1]];
			VmVecAvg(&boss_objp->position.vPos, &vertex_pos, &segcenter);
		} else
			boss_objp->position.vPos = segcenter;

		RelinkObject(boss_objnum, nSegment);
		if (!ObjectIntersectsWall(boss_objp))
			return 1;
	}

	return 0;
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI tObject if it is fairly aware of the player.
//	awareness_level is in 0..100.  Larger numbers indicate greater awareness (eg, 99 if firing at player).
//	In a given frame, might not get called for an tObject, or might be called more than once.
//	The fact that this routine is not called for a given tObject does not mean that tObject is not interested in the player.
//	OBJECTS are moved by physics, so they can move even if not interested in a player.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this player IS NOT allowed to move this robot.
//		1	this player IS allowed to move this robot.
int ai_multiplayer_awareness(tObject *objP, int awareness_level)
{
	int	rval=1;

	if (IsMultiGame) {
		if (awareness_level == 0)
			return 0;
		rval = MultiCanRemoveRobot(OBJ_IDX (objP), awareness_level);
	}
	return rval;

}
#ifndef NDEBUG
fix	Prev_boss_shields = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_boss_stuff(tObject *objP)
{
    //  New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
    if (gameData.boss [0].nLastTeleportTime > gameData.time.xGame)
        gameData.boss [0].nLastTeleportTime = gameData.time.xGame;

    if (gameData.boss [0].nLastGateTime > gameData.time.xGame)
        gameData.boss [0].nLastGateTime = gameData.time.xGame;

	if (!gameData.boss [0].nDying) {
		if (objP->cType.aiInfo.CLOAKED == 1) {
			if ((gameData.time.xGame - gameData.boss [0].nCloakStartTime > BOSS_CLOAK_DURATION/3) && (gameData.boss [0].nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION/3) && (gameData.time.xGame - gameData.boss [0].nLastTeleportTime > gameData.boss [0].nTeleportInterval)) {
				if (ai_multiplayer_awareness(objP, 98))
					TeleportBoss(objP);
			} else if (gameData.boss [0].bHitThisFrame) {
				gameData.boss [0].bHitThisFrame = 0;
				gameData.boss [0].nLastTeleportTime -= gameData.boss [0].nTeleportInterval/4;
			}

			if (gameData.time.xGame > gameData.boss [0].nCloakEndTime)
				objP->cType.aiInfo.CLOAKED = 0;
		} else {
			if ((gameData.time.xGame - gameData.boss [0].nCloakEndTime > gameData.boss [0].nCloakInterval) || gameData.boss [0].bHitThisFrame) {
				if (ai_multiplayer_awareness(objP, 95))
				{
					gameData.boss [0].bHitThisFrame = 0;
					gameData.boss [0].nCloakStartTime = gameData.time.xGame;
					gameData.boss [0].nCloakEndTime = gameData.time.xGame+BOSS_CLOAK_DURATION;
					objP->cType.aiInfo.CLOAKED = 1;
					if (IsMultiGame)
						MultiSendBossActions(OBJ_IDX (objP), 2, 0, 0);
				}
			}
		}
	} else
		DoBossDyingFrame (objP);

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	(F1_0*150)

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_super_boss_stuff(tObject *objP, fix dist_to_player, int player_visibility)
{
	static int eclipState = 0;
	do_boss_stuff(objP);

	// Only master player can cause gating to occur.
	if ((IsMultiGame) && !NetworkIAmMaster())
		return; 

	if ((dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE) || player_visibility || (IsMultiGame)) {
		if (gameData.time.xGame - gameData.boss [0].nLastGateTime > gameData.boss [0].nGateInterval/2) {
			RestartEffect(BOSS_ECLIP_NUM);
			if (eclipState == 0) {
				MultiSendBossActions(OBJ_IDX (objP), 4, 0, 0);
				eclipState = 1;
			}
		}
		else {
			StopEffect(BOSS_ECLIP_NUM);
			if (eclipState == 1) {
				MultiSendBossActions(OBJ_IDX (objP), 5, 0, 0);
				eclipState = 0;
			}
		}

		if (gameData.time.xGame - gameData.boss [0].nLastGateTime > gameData.boss [0].nGateInterval)
			if (ai_multiplayer_awareness(objP, 99)) {
				int	nObject;
				int	randtype = (rand() * D1_MAX_GATE_INDEX) >> 15;

				Assert(randtype < D1_MAX_GATE_INDEX);
				randtype = D1_Super_boss_gate_list[randtype];
				Assert(randtype < N_robot_types);

				nObject = gate_in_robot(randtype, -1);
				if ((nObject >= 0) && (IsMultiGame))
				{
					MultiSendBossActions(OBJ_IDX (objP), 3, randtype, nObject);
					MapObjnumLocalToLocal (nObject);

				}
			}	
	}
}


// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this tObject should be allowed to fire at the player.
int maybe_D1_AI_do_actual_firing_stuff(tObject *objP, tAIStatic *aip)
{
	if (IsMultiGame)
		if ((aip->GOAL_STATE != D1_AIS_FLIN) && (objP->id != ROBOT_BRAIN))
			if (aip->CURRENT_STATE == D1_AIS_FIRE)
				return 1;

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
void D1_AI_do_actual_firing_stuff(tObject *objP, tAIStatic *aip, tAILocal *ailp, tRobotInfo *botInfoP, vmsVector *vec_to_player, fix dist_to_player, vmsVector *gun_point, int player_visibility, int object_animates)
{
	fix	dot;

	if (player_visibility == 2) {
		//	Changed by mk, 01/04/94, onearm would take about 9 seconds until he can fire at you.
		// if (((!object_animates) || (ailp->achievedState[aip->CURRENT_GUN] == D1_AIS_FIRE)) && (ailp->nextPrimaryFire <= 0)) {
		if (!object_animates || (ailp->nextPrimaryFire <= 0)) {
			dot = VmVecDot(&objP->position.mOrient.fVec, vec_to_player);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < gameData.bots.info [1][objP->id].nGuns) {
					if (botInfoP->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (dist_to_player < objP->size + gameData.objs.console->size + F1_0*2)) {		// botInfoP->circle_distance[gameStates.app.nDifficultyLevel] + gameData.objs.console->size) {
							if (!ai_multiplayer_awareness(objP, ROBOT_FIRE_AGITATION-2))
								return;
							do_D1_AI_robot_hit_attack(objP, gameData.objs.console, &objP->position.vPos);
						} else {
							return;
						}
					} else {
						if ((gun_point->p.x == 0) && (gun_point->p.y == 0) && (gun_point->p.z == 0)) {
							; 
						} else {
							if (!ai_multiplayer_awareness(objP, ROBOT_FIRE_AGITATION))
								return;
							ai_fire_laser_at_player(objP, gun_point);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ( (aip->behavior != D1_AIB_RUN_FROM) && (aip->behavior != D1_AIB_STILL) && (aip->behavior != D1_AIB_FOLLOW_PATH) && ((ailp->mode == D1_AIM_FOLLOW_PATH) || (ailp->mode == D1_AIM_STILL)))
						ailp->mode = D1_AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = D1_AIS_RECO;
				ailp->goalState[aip->CURRENT_GUN] = D1_AIS_RECO;

				// Switch to next gun for next fire.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= gameData.bots.info [1][objP->id].nGuns)
					aip->CURRENT_GUN = 0;
			}
		}
	} else if (WI_homingFlag (objP->id) == 1) {
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if (((!object_animates) || (ailp->achievedState[aip->CURRENT_GUN] == D1_AIS_FIRE)) && (ailp->nextPrimaryFire <= 0) && (VmVecDist(&Hit_pos, &objP->position.vPos) > F1_0*40)) {
			if (!ai_multiplayer_awareness(objP, ROBOT_FIRE_AGITATION))
				return;
			ai_fire_laser_at_player(objP, gun_point);

			aip->GOAL_STATE = D1_AIS_RECO;
			ailp->goalState[aip->CURRENT_GUN] = D1_AIS_RECO;

			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= gameData.bots.info [1][objP->id].nGuns)
				aip->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= gameData.bots.info [1][objP->id].nGuns)
				aip->CURRENT_GUN = 0;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
void do_D1_AI_frame(tObject *objP)
{
	int			nObject = OBJ_IDX (objP);
	tAIStatic	*aip = &objP->cType.aiInfo;
	tAILocal		*ailp = &ai_local_info[nObject];
	fix			dist_to_player;
	vmsVector	vec_to_player;
	fix			dot;
	tRobotInfo	*botInfoP;
	int			player_visibility=-1;
	int			obj_ref;
	int			object_animates;
	int			new_goalState;
	int			visibility_and_vec_computed = 0;
	int			nPrevVisibility;
	vmsVector	gun_point;
	vmsVector	vis_vec_pos;

	if (aip->SKIP_D1_AI_COUNT) {
		aip->SKIP_D1_AI_COUNT--;
		return;
	}

	//	Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	//	Else, you can turn a big nasty robot into a wimp by firing flares at it.
	//	This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == D1_AIS_FLIN) && (ailp->nextPrimaryFire < 0)) {
		aip->GOAL_STATE = D1_AIS_FIRE;
	}

	gameData.ai.vBelievedPlayerPos = ai_cloak_info[nObject & (D1_MAX_AI_CLOAK_INFO-1)].vLastPos;

	if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
		aip->behavior = D1_AIB_NORMAL;
	}

	Assert(objP->nSegment != -1);
	Assert(objP->id < N_robot_types);

	botInfoP = &gameData.bots.info [1][objP->id];
	Assert(botInfoP->always_0xabcd == 0xabcd);
	obj_ref = nObject ^ gameData.app.nFrameCount;
	// -- if (ailp->wait_time > -F1_0*8)
	// -- 	ailp->wait_time -= gameData.time.xFrame;
	if (ailp->nextPrimaryFire > -F1_0*8)
		ailp->nextPrimaryFire -= gameData.time.xFrame;
	if (ailp->timeSinceProcessed < F1_0*256)
		ailp->timeSinceProcessed += gameData.time.xFrame;
	nPrevVisibility = ailp->nPrevVisibility;	//	Must get this before we toast the master copy!

	//	Deal with cloaking for robots which are cloaked except just before firing.
	if (botInfoP->cloakType == RI_CLOAKED_EXCEPT_FIRING)
		if (ailp->nextPrimaryFire < F1_0/2)
			aip->CLOAKED = 1;
		else
			aip->CLOAKED = 0;

	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
		gameData.ai.vBelievedPlayerPos = gameData.objs.console->position.vPos;

	dist_to_player = VmVecDist(&gameData.ai.vBelievedPlayerPos, &objP->position.vPos);

	//	If this robot can fire, compute visibility from gun position.
	//	Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
	if ((ailp->nextPrimaryFire <= 0) && (dist_to_player < F1_0*200) && (botInfoP->nGuns) && !(botInfoP->attackType)) {
		CalcGunPoint (&gun_point, objP, aip->CURRENT_GUN);
		vis_vec_pos = gun_point;
	} else {
		vis_vec_pos = objP->position.vPos;
		VmVecZero(&gun_point);
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
	if ((aip->behavior != D1_AIB_RUN_FROM) && (aip->behavior != D1_AIB_STILL) && !(IsMultiGame))
		if (Overall_agitation > 70) {
			if ((dist_to_player < F1_0*200) && (rand() < gameData.time.xFrame/4)) {
				if (rand() * (Overall_agitation - 40) > F1_0*5) {
					create_path_to_player(objP, 4 + Overall_agitation/8 + gameStates.app.nDifficultyLevel, 1);
					// -- show_path_and_other(objP);
					return;
				}
			}
		}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If retry count not 0, then add it into nConsecutiveRetries.
	//	If it is 0, cut down nConsecutiveRetries.
	//	This is largely a hack to speed up physics and deal with stupid AI.  This is low level
	//	communication between systems of a sort that should not be done.
	if ((ailp->nRetryCount) && !(IsMultiGame)) {
		ailp->nConsecutiveRetries += ailp->nRetryCount;
		ailp->nRetryCount = 0;
		if (ailp->nConsecutiveRetries > 3) {
			switch (ailp->mode) {
				case D1_AIM_CHASE_OBJECT:
					create_path_to_player(objP, 4 + Overall_agitation/8 + gameStates.app.nDifficultyLevel, 1);
					break;
				case D1_AIM_STILL:
					if (!((aip->behavior == D1_AIB_STILL) || (aip->behavior == D1_AIB_STATION)))	//	Behavior is still, so don't follow path.
						attempt_to_resume_path(objP);
					break;	
				case D1_AIM_FOLLOW_PATH:
					if (IsMultiGame)
						ailp->mode = D1_AIM_STILL;
					else
						attempt_to_resume_path(objP);
					break;
				case D1_AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(objP);
					objP->mType.physInfo.velocity.p.x = 0;
					objP->mType.physInfo.velocity.p.y = 0;
					objP->mType.physInfo.velocity.p.z = 0;
					create_n_segment_path(objP, 5, -1);
					ailp->mode = D1_AIM_RUN_FROM_OBJECT;
					break;
				case D1_AIM_HIDE:
					move_towards_segment_center(objP);
					objP->mType.physInfo.velocity.p.x = 0;
					objP->mType.physInfo.velocity.p.y = 0;
					objP->mType.physInfo.velocity.p.z = 0;
					if (Overall_agitation > (50 - gameStates.app.nDifficultyLevel*4))
						create_path_to_player(objP, 4 + Overall_agitation/8, 1);
					else {
						create_n_segment_path(objP, 5, -1);
					}
					break;
				case D1_AIM_OPEN_DOOR:
					create_n_segment_path_to_door(objP, 5, -1);
					break;
				#ifndef NDEBUG
				case D1_AIM_FOLLOW_PATH_2:
					Int3();	//	Should never happen!
					break;
				#endif
			}
			ailp->nConsecutiveRetries = 0;
		}
	} else
		ailp->nConsecutiveRetries /= 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
 	//	If in materialization center, exit
 	if (!(IsMultiGame) && (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_ROBOTMAKER)) {
 		ai_follow_path(objP, 1);		// 1 = player is visible, which might be a lie, but it works.
 		return;
 	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease player awareness due to the passage of time.
//-----old, faster way from about 11/06/94-----	if (ailp->playerAwarenessType) {
//-----old, faster way from about 11/06/94-----		if (ailp->playerAwarenessTime > 0) {
//-----old, faster way from about 11/06/94-----			ailp->playerAwarenessTime -= gameData.time.xFrame;
//-----old, faster way from about 11/06/94-----			if (ailp->playerAwarenessTime <= 0) {
//-----old, faster way from about 11/06/94----- 				ailp->playerAwarenessTime = F1_0*2;
//-----old, faster way from about 11/06/94----- 				ailp->playerAwarenessType--;
//-----old, faster way from about 11/06/94-----				if (ailp->playerAwarenessType < 0) {
//-----old, faster way from about 11/06/94-----	 				aip->GOAL_STATE = D1_AIS_REST;
//-----old, faster way from about 11/06/94-----	 				ailp->playerAwarenessType = 0;
//-----old, faster way from about 11/06/94-----				}
//-----old, faster way from about 11/06/94-----
//-----old, faster way from about 11/06/94-----			}
//-----old, faster way from about 11/06/94-----		} else {
//-----old, faster way from about 11/06/94----- 			ailp->playerAwarenessType = 0;
//-----old, faster way from about 11/06/94----- 			aip->GOAL_STATE = D1_AIS_REST;
//-----old, faster way from about 11/06/94-----		}
//-----old, faster way from about 11/06/94-----	} else
//-----old, faster way from about 11/06/94-----		aip->GOAL_STATE = D1_AIS_REST;


	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease player awareness due to the passage of time.
	if (ailp->playerAwarenessType) {
		if (ailp->playerAwarenessTime > 0) {
			ailp->playerAwarenessTime -= gameData.time.xFrame;
			if (ailp->playerAwarenessTime <= 0) {
 				ailp->playerAwarenessTime = F1_0*2;	//new: 11/05/94
 				ailp->playerAwarenessType--;	//new: 11/05/94
			}
		} else {
			ailp->playerAwarenessType--;
			ailp->playerAwarenessTime = F1_0*2;
			// aip->GOAL_STATE = D1_AIS_REST;
		}
	} else
		aip->GOAL_STATE = D1_AIS_REST;							//new: 12/13/94


	if (gameStates.app.bPlayerIsDead && (ailp->playerAwarenessType == 0))
		if ((dist_to_player < F1_0*200) && (rand() < gameData.time.xFrame/8)) {
			if ((aip->behavior != D1_AIB_STILL) && (aip->behavior != D1_AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(objP, 30))
					return;
				ai_multi_send_robot_position(nObject, -1);

				if (!((ailp->mode == D1_AIM_FOLLOW_PATH) && (aip->nCurPathIndex < aip->nPathLength-1)))
					if (dist_to_player < F1_0*30)
						create_n_segment_path(objP, 5, 1);
					else
						create_path_to_player(objP, 20, 1);
			}
		}

	//	Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->playerAwarenessType == D1_PA_WEAPON_ROBOT_COLLISION) || (ailp->playerAwarenessType >= D1_PA_PLAYER_COLLISION)) {
		if ((aip->behavior != D1_AIB_STILL) && (aip->behavior != D1_AIB_FOLLOW_PATH) && (aip->behavior != D1_AIB_RUN_FROM) && (objP->id != ROBOT_BRAIN))
			ailp->mode = D1_AIM_CHASE_OBJECT;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((aip->GOAL_STATE == D1_AIS_FLIN) && (aip->CURRENT_STATE == D1_AIS_FLIN))
		aip->GOAL_STATE = D1_AIS_LOCK;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Note: Should only do these two function calls for objects which animate
	if ((dist_to_player < F1_0*100)) { // && !(IsMultiGame)) {
		object_animates = do_silly_animation(objP);
		if (object_animates)
			D1_AI_frame_animation(objP);
	} else {
		//	If Object is supposed to animate, but we don't let it animate due to distance, then
		//	we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		object_animates = 0;		//	If we're not doing the animation, then should pretend it doesn't animate.
	}

	switch (gameData.bots.info [1][objP->id].bossFlag) {
		case 0:
			break;
		case 1:
			if (aip->GOAL_STATE == D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_FIRE;
			if (aip->CURRENT_STATE == D1_AIS_FLIN)
				aip->CURRENT_STATE = D1_AIS_FIRE;
			dist_to_player /= 4;

			do_boss_stuff(objP);
			dist_to_player *= 4;
			break;
#ifndef SHAREWARE
		case 2:
			if (aip->GOAL_STATE == D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_FIRE;
			if (aip->CURRENT_STATE == D1_AIS_FLIN)
				aip->CURRENT_STATE = D1_AIS_FIRE;
			compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

			{	int pv = player_visibility;
				fix	dtp = dist_to_player/4;

			//	If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = VmVecDist(&gameData.objs.console->position.vPos, &objP->position.vPos)/4;
			}

			do_super_boss_stuff(objP, dtp, pv);
			}
			break;
#endif
		default:
			Int3();	//	Bogus boss flag value.
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Time-slice, don't process all the time, purely an efficiency hack.
	//	Guys whose behavior is station and are not at their hide tSegment get processed anyway.
	if (ailp->playerAwarenessType < D1_PA_WEAPON_ROBOT_COLLISION-1) { // If robot got hit, he gets to attack player always!
		#ifndef NDEBUG
		if (Break_on_object != nObject) {	//	don't time slice if we're interested in this tObject.
		#endif
			if ((dist_to_player > F1_0*250) && (ailp->timeSinceProcessed <= F1_0*2))
				return;
			else if (!((aip->behavior == D1_AIB_STATION) && (ailp->mode == D1_AIM_FOLLOW_PATH) && (aip->nHideSegment != objP->nSegment))) {
				if ((dist_to_player > F1_0*150) && (ailp->timeSinceProcessed <= F1_0))
					return;
				else if ((dist_to_player > F1_0*100) && (ailp->timeSinceProcessed <= F1_0/2))
					return;
			}
		#ifndef NDEBUG
		}
		#endif
	}

	//	Reset time since processed, but skew objects so not everything processed synchronously, else
	//	we get fast frames with the occasional very slow frame.
	// D1_AI_proc_time = ailp->timeSinceProcessed;
	ailp->timeSinceProcessed = - ((nObject & 0x03) * gameData.time.xFrame ) / 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Perform special ability
	switch (objP->id) {
		case ROBOT_BRAIN:
			//	Robots function nicely if behavior is Station.  This means they won't move until they
			//	can see the player, at which time they will start wandering about opening doors.
			if (gameData.objs.console->nSegment == objP->nSegment) {
				if (!ai_multiplayer_awareness(objP, 97))
					return;
				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
				move_away_from_player(objP, &vec_to_player, 0);
				ai_multi_send_robot_position(nObject, -1);
			} else if (ailp->mode != D1_AIM_STILL) {
				int	r;

				r = openable_doors_in_segment(objP);
				if (r != -1) {
					ailp->mode = D1_AIM_OPEN_DOOR;
					aip->GOALSIDE = r;
				} else if (ailp->mode != D1_AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(objP, 50))
						return;
					create_n_segment_path_to_door(objP, 8+gameStates.app.nDifficultyLevel, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(nObject, -1);
				}
			} else {
				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(objP, 50))
						return;
					create_n_segment_path_to_door(objP, 8+gameStates.app.nDifficultyLevel, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(nObject, -1);
				}
			}
			break;
		default:
			break;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	switch (ailp->mode) {
		case D1_AIM_CHASE_OBJECT: {		// chasing player, sort of, chase if far, back off if close, circle in between
			fix	circle_distance;

			circle_distance = botInfoP->circleDistance[gameStates.app.nDifficultyLevel] + gameData.objs.console->size;
			//	Green guy doesn't get his circle distance boosted, else he might never attack.
			if (botInfoP->attackType != 1)
				circle_distance += (nObject&0xf) * F1_0/2;

			compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

			//	@mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (nPrevVisibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == D1_AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(objP, 53)) {
					if (maybe_D1_AI_do_actual_firing_stuff(objP, aip))
						D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
					return;
				}
				create_path_to_player(objP, 8, 1);
				// -- show_path_and_other(objP);
				ai_multi_send_robot_position(nObject, -1);
			} else if ((player_visibility == 0) && (dist_to_player > F1_0*80) && (!(IsMultiGame))) {
				//	If pretty far from the player, player cannot be seen (obstructed) and in chase mode, switch to follow path mode.
				//	This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == D1_AIB_STATION) {
					ailp->nGoalSegment = aip->nHideSegment;
					create_path_to_station(objP, 15);
					// -- show_path_and_other(objP);
				} else
					create_n_segment_path(objP, 5, -1);
				break;
			}

			if ((aip->CURRENT_STATE == D1_AIS_REST) && (aip->GOAL_STATE == D1_AIS_REST)) {
				if (player_visibility) {
					if (rand() < gameData.time.xFrame*player_visibility) {
						if (dist_to_player/256 < rand()*player_visibility) {
							aip->GOAL_STATE = D1_AIS_SRCH;
							aip->CURRENT_STATE = D1_AIS_SRCH;
						}
					}
				}
			}

			if (gameData.time.xGame - ailp->timePlayerSeen > CHASE_TIME_LENGTH) {

				if (IsMultiGame)
					if (!player_visibility && (dist_to_player > F1_0*70)) {
						ailp->mode = D1_AIM_STILL;
						return;
					}

				if (!ai_multiplayer_awareness(objP, 64)) {
					if (maybe_D1_AI_do_actual_firing_stuff(objP, aip))
						D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
					return;
				}
				create_path_to_player(objP, 10, 1);
				// -- show_path_and_other(objP);
				ai_multi_send_robot_position(nObject, -1);
			} else if ((aip->CURRENT_STATE != D1_AIS_REST) && (aip->GOAL_STATE != D1_AIS_REST)) {
				if (!ai_multiplayer_awareness(objP, 70)) {
					if (maybe_D1_AI_do_actual_firing_stuff(objP, aip))
						D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
					return;
				}
				ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, circle_distance, 0);

				if ((obj_ref & 1) && ((aip->GOAL_STATE == D1_AIS_SRCH) || (aip->GOAL_STATE == D1_AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
					else
						ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel], nPrevVisibility);
				}

				if (D1_AI_evaded) {
					ai_multi_send_robot_position(nObject, 1);
					D1_AI_evaded = 0;
				}
				else 
					ai_multi_send_robot_position(nObject, -1);
				
				do_firing_stuff(objP, player_visibility, &vec_to_player);
			}
			break;
		}

		case D1_AIM_RUN_FROM_OBJECT:
			compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

			if (player_visibility) {
				if (ailp->playerAwarenessType == 0)
					ailp->playerAwarenessType = D1_PA_WEAPON_ROBOT_COLLISION;

			}

			//	If in multiplayer, only do if player visible.  If not multiplayer, do always.
			if (!(IsMultiGame) || player_visibility)
				if (ai_multiplayer_awareness(objP, 75)) {
					ai_follow_path(objP, player_visibility);
					ai_multi_send_robot_position(nObject, -1);
				}

			if (aip->GOAL_STATE != D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_LOCK;
			else if (aip->CURRENT_STATE == D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_LOCK;

			//	Bad to let run_from robot fire at player because it will cause a war in which it turns towards the
			//	player to fire and then towards its goal to move.
			// do_firing_stuff(objP, player_visibility, &vec_to_player);
			//	Instead, do this:
			//	(Note, only drop if player is visible.  This prevents the bombs from being a giveaway, and
			//	also ensures that the robot is moving while it is dropping.  Also means fewer will be dropped.)
			if ((ailp->nextPrimaryFire <= 0) && (player_visibility)) {
				vmsVector	fire_vec, fire_pos;

				if (!ai_multiplayer_awareness(objP, 75))
					return;

				fire_vec = objP->position.mOrient.fVec;
				VmVecNegate(&fire_vec);
				VmVecAdd(&fire_pos, &objP->position.vPos, &fire_vec);

				CreateNewLaserEasy( &fire_vec, &fire_pos, OBJ_IDX (objP), PROXMINE_ID, 1);
				ailp->nextPrimaryFire = F1_0*5;		//	Drop a proximity bomb every 5 seconds.
				
				#ifdef NETWORK
				if (IsMultiGame)
				{
					ai_multi_send_robot_position(OBJ_IDX (objP), -1);
					MultiSendRobotFire(OBJ_IDX (objP), -1, &fire_vec);
				}				  
				#endif	
			}
			break;

		case D1_AIM_FOLLOW_PATH: {
			int	anger_level = 65;

			if (aip->behavior == D1_AIB_STATION)
				if (gameData.ai.pointSegs[aip->nHideIndex + aip->nPathLength - 1].nSegment == aip->nHideSegment) {
					anger_level = 64;
				}

			compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

			if (gameData.app.nGameMode & (GM_MODEM | GM_SERIAL))
				if (!player_visibility && (dist_to_player > F1_0*70)) {
					ailp->mode = D1_AIM_STILL;
					return;
				}

			if (!ai_multiplayer_awareness(objP, anger_level)) {
				if (maybe_D1_AI_do_actual_firing_stuff(objP, aip)) {
					compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
					D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
				}
				return;
			}

			ai_follow_path(objP, player_visibility);

			if (aip->GOAL_STATE != D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_LOCK;
			else if (aip->CURRENT_STATE == D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_LOCK;

			if ((aip->behavior != D1_AIB_FOLLOW_PATH) && (aip->behavior != D1_AIB_RUN_FROM))
				do_firing_stuff(objP, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != D1_AIB_FOLLOW_PATH) && (aip->behavior != D1_AIB_RUN_FROM) && (objP->id != ROBOT_BRAIN)) {
				if (botInfoP->attackType == 0)
					ailp->mode = D1_AIM_CHASE_OBJECT;
			} else if ((player_visibility == 0) && (aip->behavior == D1_AIB_NORMAL) && (ailp->mode == D1_AIM_FOLLOW_PATH) && (aip->behavior != D1_AIB_RUN_FROM)) {
				ailp->mode = D1_AIM_STILL;
				aip->nHideIndex = -1;
				aip->nPathLength = 0;
			}

			ai_multi_send_robot_position(nObject, -1);

			break;
		}

		case D1_AIM_HIDE:
			if (!ai_multiplayer_awareness(objP, 71)) {
				if (maybe_D1_AI_do_actual_firing_stuff(objP, aip)) {
					compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
					D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
				}
				return;
			}

			compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

 			ai_follow_path(objP, player_visibility);

			if (aip->GOAL_STATE != D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_LOCK;
			else if (aip->CURRENT_STATE == D1_AIS_FLIN)
				aip->GOAL_STATE = D1_AIS_LOCK;

			ai_multi_send_robot_position(nObject, -1);
			break;

		case D1_AIM_STILL:
			if ((dist_to_player < F1_0*120+gameStates.app.nDifficultyLevel*F1_0*20) || (ailp->playerAwarenessType >= D1_PA_WEAPON_ROBOT_COLLISION-1)) {
				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

				//	turn towards vector if visible this time or last time, or rand
				// new!
				if ((player_visibility) || (nPrevVisibility) || ((rand() > 0x4000) && !(IsMultiGame))) {
					if (!ai_multiplayer_awareness(objP, 71)) {
						if (maybe_D1_AI_do_actual_firing_stuff(objP, aip))
							D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
						return;
					}
					ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(nObject, -1);
				}

				do_firing_stuff(objP, player_visibility, &vec_to_player);
				//	This is debugging code!  Remove it!  It's to make the green guy attack without doing other kinds of movement.
				if (player_visibility) {		//	Change, MK, 01/03/94 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (botInfoP->attackType == 1) {
						aip->behavior = D1_AIB_NORMAL;
						if (!ai_multiplayer_awareness(objP, 80)) {
							if (maybe_D1_AI_do_actual_firing_stuff(objP, aip))
								D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
							return;
						}
						ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, 0, 0);
						if (D1_AI_evaded) {
							ai_multi_send_robot_position(nObject, 1);
							D1_AI_evaded = 0;
						}
						else
							ai_multi_send_robot_position(nObject, -1);
					} else {
						//	Robots in hover mode are allowed to evade at half normal speed.
						if (!ai_multiplayer_awareness(objP, 81)) {
							if (maybe_D1_AI_do_actual_firing_stuff(objP, aip))
								D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
							return;
						}
						ai_move_relative_to_player(objP, ailp, dist_to_player, &vec_to_player, 0, 1);
						if (D1_AI_evaded) {
							ai_multi_send_robot_position(nObject, -1);
							D1_AI_evaded = 0;
						}
						else				
							ai_multi_send_robot_position(nObject, -1);
					}
				} else if ((objP->nSegment != aip->nHideSegment) && (dist_to_player > F1_0*80) && (!(IsMultiGame))) {
					//	If pretty far from the player, player cannot be seen (obstructed) and in chase mode, switch to follow path mode.
					//	This has one desirable benefit of avoiding physics retries.
					if (aip->behavior == D1_AIB_STATION) {
						ailp->nGoalSegment = aip->nHideSegment;
						create_path_to_station(objP, 15);
						// -- show_path_and_other(objP);
					}
					break;
				}
			}

			break;
		case D1_AIM_OPEN_DOOR: {		// trying to open a door.
			vmsVector	center_point, goal_vector;
			Assert(objP->id == ROBOT_BRAIN);		//	Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(objP, 62))
				return;
			compute_center_point_on_side(&center_point, &gameData.segs.segments[objP->nSegment], aip->GOALSIDE);
			VmVecSub(&goal_vector, &center_point, &objP->position.vPos);
			VmVecNormalize(&goal_vector);
			ai_turn_towards_vector(&goal_vector, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
			move_towards_vector(objP, &goal_vector);
			ai_multi_send_robot_position(nObject, -1);

			break;
		}

		default:
			ailp->mode = D1_AIM_CHASE_OBJECT;
			break;
	}		// end:	switch (ailp->mode) {

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If the robot can see you, increase his awareness of you.
	//	This prevents the problem of a robot looking right at you but doing nothing.
	// Assert(player_visibility != -1);	//	Means it didn't get initialized!
	compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
	if (player_visibility == 2)
		if (ailp->playerAwarenessType == 0)
			ailp->playerAwarenessType = D1_PA_PLAYER_COLLISION;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (!object_animates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
	}

	Assert(ailp->playerAwarenessType <= D1_AIE_MAX);
	Assert(aip->CURRENT_STATE < D1_AIS_MAX);
	Assert(aip->GOAL_STATE < D1_AIS_MAX);

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (ailp->playerAwarenessType) {
		new_goalState = D1_AI_transition_table[ailp->playerAwarenessType-1][aip->CURRENT_STATE][aip->GOAL_STATE];
		if (ailp->playerAwarenessType == D1_PA_WEAPON_ROBOT_COLLISION) {
			//	Decrease awareness, else this robot will flinch every frame.
			ailp->playerAwarenessType--;
			ailp->playerAwarenessTime = F1_0*3;
		}

		if (new_goalState == D1_AIS_ERR_)
			new_goalState = D1_AIS_REST;

		if (aip->CURRENT_STATE == D1_AIS_NONE)
			aip->CURRENT_STATE = D1_AIS_REST;

		aip->GOAL_STATE = new_goalState;

	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If new state = fire, then set all gun states to fire.
	if ((aip->GOAL_STATE == D1_AIS_FIRE) ) {
		int	i,num_guns;
		num_guns = gameData.bots.info [1][objP->id].nGuns;
		for (i=0; i<num_guns; i++)
			ailp->goalState[i] = D1_AIS_FIRE;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his nextPrimaryFire says ok to fire, bash him there
	if ((ailp->nextPrimaryFire < 0) && (aip->GOAL_STATE == D1_AIS_FIRE))
		aip->CURRENT_STATE = D1_AIS_FIRE;

	if ((aip->GOAL_STATE != D1_AIS_FLIN)  && (objP->id != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
			case	D1_AIS_NONE:
				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

				dot = VmVecDot(&objP->position.mOrient.fVec, &vec_to_player);
				if (dot >= F1_0/2)
					if (aip->GOAL_STATE == D1_AIS_REST)
						aip->GOAL_STATE = D1_AIS_SRCH;
				break;
			case	D1_AIS_REST:
				if (aip->GOAL_STATE == D1_AIS_REST) {
					compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
					if ((ailp->nextPrimaryFire <= 0) && (player_visibility)) {
						aip->GOAL_STATE = D1_AIS_FIRE;
					}
				}
				break;
			case	D1_AIS_SRCH:
				if (!ai_multiplayer_awareness(objP, 60))
					return;

				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

				if (player_visibility) {
					ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(nObject, -1);
				} else if (!(IsMultiGame))
					ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel], nPrevVisibility);
				break;
			case	D1_AIS_LOCK:
				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

				if (!(IsMultiGame) || (player_visibility)) {
					if (!ai_multiplayer_awareness(objP, 68))
						return;

					if (player_visibility) {
						ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
						ai_multi_send_robot_position(nObject, -1);
					} else if (!(IsMultiGame))
						ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel], nPrevVisibility);
				}
				break;
			case	D1_AIS_FIRE:
				compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);

				if (player_visibility) {
					if (!ai_multiplayer_awareness(objP, (ROBOT_FIRE_AGITATION-1))) 
					{
						if (IsMultiGame) {
							D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
							return;
						}
					}
					ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(nObject, -1);
				} else if (!(IsMultiGame)) {
					ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel], nPrevVisibility);
				}

				//	Fire at player, if appropriate.
				D1_AI_do_actual_firing_stuff(objP, aip, ailp, botInfoP, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);

				break;
			case	D1_AIS_RECO:
				if (!(obj_ref & 3)) {
					compute_vis_and_vec(objP, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, botInfoP, &visibility_and_vec_computed);
					if (player_visibility) {
						if (!ai_multiplayer_awareness(objP, 69))
							return;
						ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel]);
						ai_multi_send_robot_position(nObject, -1);
					} else if (!(IsMultiGame)) {
						ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime[gameStates.app.nDifficultyLevel], nPrevVisibility);
					}
				}
				break;
			case	D1_AIS_FLIN:
				break;
			default:
				aip->GOAL_STATE = D1_AIS_REST;
				aip->CURRENT_STATE = D1_AIS_REST;
				break;
		}
	} // end of: if (aip->GOAL_STATE != D1_AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		aip->CURRENT_GUN++;
		if (aip->CURRENT_GUN >= gameData.bots.info [1][objP->id].nGuns)
			aip->CURRENT_GUN = 0;
	}

}

//--mk, 121094 -- // ----------------------------------------------------------------------------------
//--mk, 121094 -- void spin_robot(tObject *robot, vmsVector *collision_point)
//--mk, 121094 -- {
//--mk, 121094 -- 	if (collision_point->p.x != 3) {
//--mk, 121094 -- 		robot->physInfo.rotVel.p.x = 0x1235;
//--mk, 121094 -- 		robot->physInfo.rotVel.p.y = 0x2336;
//--mk, 121094 -- 		robot->physInfo.rotVel.p.z = 0x3737;
//--mk, 121094 -- 	}
//--mk, 121094 -- 
//--mk, 121094 -- }

//	-----------------------------------------------------------------------------------
void D1_AI_do_cloak_stuff(void)
{
	int	i;

	for (i=0; i<D1_MAX_AI_CLOAK_INFO; i++) {
		ai_cloak_info[i].vLastPos = gameData.objs.console->position.vPos;
		ai_cloak_info[i].lastTime = gameData.time.xGame;
	}

	//	Make work for control centers.
	gameData.ai.vBelievedPlayerPos = ai_cloak_info[0].vLastPos;

}

//	-----------------------------------------------------------------------------------
//	Returns false if awareness is considered too puny to add, else returns true.
int add_awareness_event(tObject *objP, int type)
{
	//	If player cloaked and hit a robot, then increase awareness
	if ((type == D1_PA_WEAPON_ROBOT_COLLISION) || (type == D1_PA_WEAPON_WALL_COLLISION) || (type == D1_PA_PLAYER_COLLISION))
		D1_AI_do_cloak_stuff();

	if (Num_awareness_events < D1_MAX_AWARENESS_EVENTS) {
		if ((type == D1_PA_WEAPON_WALL_COLLISION) || (type == D1_PA_WEAPON_ROBOT_COLLISION))
			if (objP->id == VULCAN_ID)
				if (rand() > 3276)
					return 0;		//	For vulcan cannon, only about 1/10 actually cause awareness

		Awareness_events[Num_awareness_events].nSegment = objP->nSegment;
		Awareness_events[Num_awareness_events].pos = objP->position.vPos;
		Awareness_events[Num_awareness_events].nType = type;
		Num_awareness_events++;
	} else
		Assert(0);		// Hey -- Overflowed Awareness_events, make more or something
							// This just gets ignored, so you can just continue.
	return 1;

}

// ----------------------------------------------------------------------------------
//	Robots will become aware of the player based on something that occurred.
//	The tObject (probably player or weapon) which created the awareness is objP.
void create_awareness_event(tObject *objP, int type)
{
	if (add_awareness_event(objP, type)) {
		if (((rand() * (type+4)) >> 15) > 4)
			Overall_agitation++;
		if (Overall_agitation > OVERALL_AGITATION_MAX)
			Overall_agitation = OVERALL_AGITATION_MAX;
	}
}

byte	New_awareness[MAX_SEGMENTS];

// ----------------------------------------------------------------------------------
void pae_aux(int nSegment, int type, int level)
{
	int	j;

	if (New_awareness[nSegment] < type)
		New_awareness[nSegment] = type;

	// Process children.
	if (level <= 4)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (IS_CHILD(gameData.segs.segments[nSegment].children[j]))
				if (type == 4)
					pae_aux(gameData.segs.segments[nSegment].children[j], type-1, level+1);
				else
					pae_aux(gameData.segs.segments[nSegment].children[j], type, level+1);
}


// ----------------------------------------------------------------------------------
void process_awareness_events(void)
{
	int	i;

	for (i=0; i<=gameData.segments.nLastSegment; i++)
		New_awareness[i] = 0;

	for (i=0; i<Num_awareness_events; i++)
		pae_aux(Awareness_events[i].nSegment, Awareness_events[i].nType, 1);

	Num_awareness_events = 0;
}

// ----------------------------------------------------------------------------------
void set_player_awareness_all(void)
{
	int	i;

	process_awareness_events();

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (OBJECTS[i].controlType == CT_AI)
			if (New_awareness[OBJECTS[i].nSegment] > ai_local_info[i].playerAwarenessType) {
				ai_local_info[i].playerAwarenessType = New_awareness[OBJECTS[i].nSegment];
				ai_local_info[i].playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
			}
}

#ifndef NDEBUG
int	D1_AI_dump_enable = 0;

FILE *D1_AI_dump_file = NULL;

char	D1_AI_error_message[128] = "";

// ----------------------------------------------------------------------------------
void dump_D1_AI_objects_all()
{
#if PARALLAX
	int	nObject;
	int	total=0;
	time_t	time_of_day;

	time_of_day = time(NULL);

	if (!D1_AI_dump_enable)
		return;

	if (D1_AI_dump_file == NULL)
		D1_AI_dump_file = fopen("ai.out","a+t");

	fprintf(D1_AI_dump_file, "\nnum: seg distance __mode__ behav.    [velx vely velz] (Frame = %i)\n", gameData.app.nFrameCount);
	fprintf(D1_AI_dump_file, "Date & Time = %s\n", ctime(&time_of_day));

	if (D1_AI_error_message[0])
		fprintf(D1_AI_dump_file, "Error message: %s\n", D1_AI_error_message);

	for (nObject=0; nObject <= gameData.objs.nLastObject; nObject++) {
		tObject		*objP = &OBJECTS[nObject];
		tAIStatic	*aip = &objP->cType.aiInfo;
		tAILocal		*ailp = &ai_local_info[nObject];
		fix			dist_to_player;

		dist_to_player = vm_vec_dist(&objP->position.vPos, &gameData.objs.console->position.vPos);

		if (objP->controlType == CT_AI) {
			fprintf(D1_AI_dump_file, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n",
				nObject, objP->nSegment, f2fl(dist_to_player), mode_text[ailp->mode], behavior_text[aip->behavior-0x80], aip->nHideIndex, aip->nPathLength);
			if (aip->nPathLength)
				total += aip->nPathLength;
		}
	}

	fprintf(D1_AI_dump_file, "Total path length = %4i\n", total);
#endif

}

// ----------------------------------------------------------------------------------
void force_dump_D1_AI_objects_all(char *msg)
{
	int	tsave;

	tsave = D1_AI_dump_enable;

	D1_AI_dump_enable = 1;

	sprintf(D1_AI_error_message, "%s\n", msg);
	dump_D1_AI_objects_all();
	D1_AI_error_message[0] = 0;

	D1_AI_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------
void turn_off_D1_AI_dump(void)
{
	if (D1_AI_dump_file != NULL)
		fclose(D1_AI_dump_file);

	D1_AI_dump_file = NULL;
}

#endif

// ----------------------------------------------------------------------------------
//	Do things which need to get done for all AI objects each frame.
//	This includes:
//		Setting player_awareness (a fix, time in seconds which tObject is aware of player)
void do_D1_AI_frame_all(void)
{
#ifndef NDEBUG
	dump_D1_AI_objects_all();
#endif

	set_player_awareness_all();
}


//	Initializations to be performed for all robots for a new level.
void init_robots_for_level(void)
{
	Overall_agitation = 0;
}

int D1_AI_saveState( FILE * fp )
{
	fwrite( &D1_AI_initialized, sizeof(int), 1, fp );
	fwrite( &Overall_agitation, sizeof(int), 1, fp );
	fwrite( ai_local_info, sizeof(tAILocal)*MAX_OBJECTS, 1, fp );
	fwrite( gameData.ai.pointSegs, sizeof(point_seg)*D1_MAX_POINT_SEGS, 1, fp );
	fwrite( ai_cloak_info, sizeof(ai_cloak_info)*D1_MAX_AI_CLOAK_INFO, 1, fp );
	fwrite( &gameData.boss [0].nCloakStartTime, sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nCloakEndTime , sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nLastTeleportTime , sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nTeleportInterval, sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nCloakInterval, sizeof(fix), 1, fp );
	fwrite( &BOSS_CLOAK_DURATION, sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nLastGateTime, sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nGateInterval, sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nDyingStartTime, sizeof(fix), 1, fp );
	fwrite( &gameData.boss [0].nDying, sizeof(int), 1, fp );
	fwrite( &gameData.boss [0].bDyingSoundPlaying, sizeof(int), 1, fp );
	fwrite( &gameData.boss [0].bHitThisFrame, sizeof(int), 1, fp );
	fwrite( &gameData.boss [0].bHasBeenHit, sizeof(int), 1, fp );
	return 1;
}

int D1_AI_restoreState( FILE * fp )
{
	fread( &D1_AI_initialized, sizeof(int), 1, fp );
	fread( &Overall_agitation, sizeof(int), 1, fp );
	fread( ai_local_info, sizeof(tAILocal)*MAX_OBJECTS, 1, fp );
	fread( gameData.ai.pointSegs, sizeof(point_seg)*D1_MAX_POINT_SEGS, 1, fp );
	fread( ai_cloak_info, sizeof(ai_cloak_info)*D1_MAX_AI_CLOAK_INFO, 1, fp );
	fread( &gameData.boss [0].nCloakStartTime, sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nCloakEndTime , sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nLastTeleportTime , sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nTeleportInterval, sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nCloakInterval, sizeof(fix), 1, fp );
	fread( &BOSS_CLOAK_DURATION, sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nLastGateTime, sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nGateInterval, sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nDyingStartTime, sizeof(fix), 1, fp );
	fread( &gameData.boss [0].nDying, sizeof(int), 1, fp );
	fread( &gameData.boss [0].bDyingSoundPlaying, sizeof(int), 1, fp );
	fread( &gameData.boss [0].bHitThisFrame, sizeof(int), 1, fp );
	fread( &gameData.boss [0].bHasBeenHit, sizeof(int), 1, fp );
	return 1;
}

