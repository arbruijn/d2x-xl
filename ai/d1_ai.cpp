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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "descent.h"
#include "game.h"
#include "3d.h"

#include "object.h"
#include "robot.h"
#include "rendermine.h"
#include "error.h"
#include "d1_ai.h"
#include "laser.h"
#include "collision_math.h"
#include "polymodel.h"
#include "loadgamedata.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "fuelcen.h"
#include "player.h"
#include "wall.h"
#include "vclip.h"
#include "audio.h"
#include "fireball.h"
#include "morph.h"
#include "effects.h"
#include "timer.h"
#include "sounds.h"
#include "reactor.h"
#include "multibot.h"
#include "multi.h"
#include "network.h"
#include "loadgame.h"
#include "key.h"
#include "powerup.h"
#include "cockpit.h"
#include "text.h"
#include "gameseg.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

#define	JOHN_CHEATS_SIZE_1	6
#define	JOHN_CHEATS_SIZE_2	6
#define	JOHN_CHEATS_SIZE_3	6

void pae_aux (int nSegment, int nType, int level);

ubyte	john_cheats_1 [JOHN_CHEATS_SIZE_1] = { 	KEY_P ^ 0x00 ^ 0x34,
															KEY_O ^ 0x10 ^ 0x34,
															KEY_B ^ 0x20 ^ 0x34,
															KEY_O ^ 0x30 ^ 0x34,
															KEY_Y ^ 0x40 ^ 0x34,
															KEY_S ^ 0x50 ^ 0x34 };

#define	PARALLAX	0		//	If !0, then special debugging info for Parallax eyes only enabled.

#define MIN_D 0x100

int	nFlinchScaleD1 = 4;
int	nAttackScaleD1 = 24;

#ifndef ANIM_RATE
#	define	ANIM_RATE		(I2X (1)/16)
#endif

#define	DELTA_ANG_SCALE	16

static ubyte xlatD1Animation [] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

int	john_cheats_index_1;		//	POBOYS		detonate reactor
int	john_cheats_index_2;		//	PORGYS		high speed weapon firing
int	john_cheats_index_3;		//	LUNACY		lunacy (insane behavior, rookie firing)
int	john_cheats_index_4;		//	PLETCHnnn	paint robots

extern int nRobotSoundVolume;

// int	No_ai_flag=0;

#define	OVERALL_AGITATION_MAX	100

#define		D1_MAX_AI_CLOAK_INFO	8	//	Must be a power of 2!

#ifndef BOSS_CLOAK_DURATION
#	define	BOSS_CLOAK_DURATION	(I2X (7))
#endif
#ifndef BOSS_DEATH_DURATION
#	define	BOSS_DEATH_DURATION	(I2X (6))
#endif
#define	BOSS_DEATH_SOUND_DURATION	0x2ae14		//	2.68 seconds

//	Amount of time since the current robotP was last processed for things such as movement.
//	It is not valid to use gameData.time.xFrame because robots do not get moved every frame.
//fix	D1_AI_proc_time;

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
//	18	cloak-green	; note, gating in this guy benefits playerP, cloak objects
//	19	vulcan
//	20	toad
//	21	4-claw
//	22	quad-laser
// 23 super boss

// ubyte	super_boss_gate_list [] = {0, 1, 2, 9, 11, 16, 18, 19, 21, 22, 0, 9, 9, 16, 16, 18, 19, 19, 22, 22};
ubyte	super_boss_gate_list [] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};
#define	D1_MAX_GATE_INDEX	( sizeof(super_boss_gate_list) / sizeof(super_boss_gate_list [0]) )

int	D1_AI_info_enabled=0;

int	Ugly_robot_cheat = 0, Ugly_robot_texture = 0;
ubyte	Enable_john_cheat_1 = 0, Enable_john_cheat_2 = 0, Enable_john_cheat_3 = 0, Enable_john_cheat_4 = 0;

ubyte	john_cheats_3 [2*JOHN_CHEATS_SIZE_3+1] = { KEY_Y ^ 0x67,
																KEY_E ^ 0x66,
																KEY_C ^ 0x65,
																KEY_A ^ 0x64,
																KEY_N ^ 0x63,
																KEY_U ^ 0x62,
																KEY_L ^ 0x61 };


#define	D1_MAX_AWARENESS_EVENTS	64
typedef struct awareness_event {
	short 		nSegment;				// CSegment the event occurred in
	short			type;					// type of event, defines behavior
	CFixVector	pos;					// absolute 3 space location of event
} awareness_event;


//	These globals are set by a call to FindHitpoint, which is a slow routine,
//	so we don't want to call it again (for this CObject) unless we have to.
CFixVector	Hit_pos;
int			hitType, Hit_seg;
tCollisionData		hitData;

#define	D1_AIS_MAX	8
#define	D1_AIE_MAX	4

//--unused-- int	Processed_this_frame, LastFrameCount;
#if DBG
//	Index into this array with ailP->mode
char	mode_text [8][9] = {
	"STILL   ",
	"WANDER  ",
	"FOL_PATH",
	"CHASE_OB",
	"RUN_FROM",
	"HIDE    ",
	"FOL_PAT2",
	"OPENDOR2"
};

//	Index into this array with aiP->behavior
char	behavior_text [6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

//	Index into this array with aiP->GOAL_STATE or aiP->CURRENT_STATE
char	state_text [8][5] = {
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

// Current state indicates where the robotP current is, or has just done.
//	Transition table between states for an AI CObject.
//	 First dimension is trigger event.
//	Second dimension is current state.
//	 Third dimension is goal state.
//	Result is new goal state.
//	ERR_ means something impossible has happened.
ubyte D1_AI_transition_table [D1_AI_MAX_EVENT][D1_AI_MAX_STATE][D1_AI_MAX_STATE] = {
 {
	//	Event = AIE_FIRE, a nearby CObject fired
	//	none			rest			srch			lock			flin			fire			reco				// CURRENT is rows, GOAL is columns
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	none
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	rest
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	search
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	lock
 {	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},		//	flinch
 {	D1_AIS_ERR_,	D1_AIS_FIRE,	D1_AIS_FIRE,	D1_AIS_FIRE,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},		//	fire
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}		//	recoil
	},

	//	Event = AIE_HITT, a nearby CObject was hit (or a wall was hit)
 {
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FLIN},
 {	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}
	},

	//	Event = AIE_COLL, playerP collided with robotP
 {
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FLIN},
 {	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}
	},

	//	Event = AIE_HURT, playerP hurt robotP (by firing at and hitting it)
	//	Note, this doesn't necessarily mean the robotP JUST got hit, only that that is the most recent thing that happened.
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

ubyte	john_cheats_2 [2*JOHN_CHEATS_SIZE_2] = { 	KEY_P ^ 0x00 ^ 0x43, 0x66,
																KEY_O ^ 0x10 ^ 0x43, 0x11,
																KEY_R ^ 0x20 ^ 0x43, 0x8,
																KEY_G ^ 0x30 ^ 0x43, 0x2,
																KEY_Y ^ 0x40 ^ 0x43, 0x0,
																KEY_S ^ 0x50 ^ 0x43 };

// ---------------------------------------------------------
//	On entry, gameData.bots.nTypes [1] had darn sure better be set.
//	Mallocs gameData.bots.nTypes [1] tRobotInfo structs into global tRobotInfo.
void john_cheat_func_1(int key)
{
	if (!gameStates.app.cheats.bEnabled)
		return;

if (key == (john_cheats_1 [john_cheats_index_1] ^ (john_cheats_index_1 << 4) ^ 0x34)) {
	john_cheats_index_1++;
	if (john_cheats_index_1 == JOHN_CHEATS_SIZE_1) {
		DoReactorDestroyedStuff (NULL);
		john_cheats_index_1 = 0;
		audio.PlaySound (SOUND_CHEATER);
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

fix	primaryFiringWaitCopy [MAX_ROBOT_TYPES];
ubyte	nRapidFireCountCopy [MAX_ROBOT_TYPES];

void do_lunacy_on(void)
{
	int	i;

	if ( !Lunacy ) {
		Lunacy = 1;
		Diff_save = gameStates.app.nDifficultyLevel;
		gameStates.app.nDifficultyLevel = NDL-1;

		for (i=0; i<MAX_ROBOT_TYPES; i++) {
			primaryFiringWaitCopy [i] = gameData.bots.info [1][i].primaryFiringWait [NDL-1];
			nRapidFireCountCopy [i] = gameData.bots.info [1][i].nRapidFireCount [NDL-1];

			gameData.bots.info [1][i].primaryFiringWait [NDL-1] = gameData.bots.info [1][i].primaryFiringWait [1];
			gameData.bots.info [1][i].nRapidFireCount [NDL-1] = gameData.bots.info [1][i].nRapidFireCount [1];
		}
	}
}

void do_lunacy_off(void)
{
	int	i;

	if ( Lunacy ) {
		Lunacy = 0;
		for (i=0; i<MAX_ROBOT_TYPES; i++) {
			gameData.bots.info [1][i].primaryFiringWait [NDL-1] = primaryFiringWaitCopy [i];
			gameData.bots.info [1][i].nRapidFireCount [NDL-1] = nRapidFireCountCopy [i];
		}
		gameStates.app.nDifficultyLevel = Diff_save;
	}
}

void john_cheat_func_3(int key)
{
	if (!gameStates.app.cheats.bEnabled)
		return;

	if (key == (john_cheats_3 [JOHN_CHEATS_SIZE_3 - john_cheats_index_3] ^ (0x61 + john_cheats_index_3))) {
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
				audio.PlaySound (SOUND_CHEATER);
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
		if (abs(delta) < I2X (1)/8) {
			*dest = delta/4;
		} else
			*dest = delta;
	} else {
		*dest = delta;
	}
}

//--debug-- #if DBG
//--debug-- int	Total_turns=0;
//--debug-- int	Prevented_turns=0;
//--debug-- #endif

#define	D1_AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector(CFixVector *vGoal, CObject *objP, fix rate)
{
	CFixVector	new_fVec;
	fix			dot;

	if ((objP->info.nId == BABY_SPIDER_ID) && (objP->info.nType == OBJ_ROBOT)) {
		objP->TurnTowardsVector(*vGoal, rate);
		return;
	}

	new_fVec = *vGoal;

	dot = CFixVector::Dot (*vGoal, objP->info.position.mOrient.FVec ());

	if (dot < (I2X (1) - gameData.time.xFrame/2)) {
		fix	mag;
		fix	new_scale = FixDiv(gameData.time.xFrame * D1_AI_TURN_SCALE, rate);
		new_fVec *= new_scale;
		new_fVec += objP->info.position.mOrient.FVec ();
		mag = CFixVector::Normalize (new_fVec);
		if (mag < I2X (1)/256) {
			new_fVec = *vGoal;		//	if degenerate vector, go right to goal
		}
	}
objP->info.position.mOrient = CFixMatrix::CreateFR (new_fVec, objP->info.position.mOrient.RVec ());
}

// --------------------------------------------------------------------------------------------------------------------
void ai_turn_randomly(CFixVector *vec_to_player, CObject *objP, fix rate, int nPrevVisibility)
{
	CFixVector	curVec;

	//	Random turning looks too stupid, so 1/4 of time, cheat.
	if (nPrevVisibility)
		if (d_rand() > 0x7400) {
			ai_turn_towards_vector(vec_to_player, objP, rate);
			return;
		}
//--debug-- 	if (d_rand() > 0x6000)
//--debug-- 		Prevented_turns++;

	curVec = objP->mType.physInfo.rotVel;

	curVec [Y] += I2X (1)/64;

	curVec [X] += curVec [Y]/6;
	curVec [Y] += curVec [Z]/4;
	curVec [Z] += curVec [X]/10;

	if (abs(curVec [X]) > I2X (1)/8) curVec [X] /= 4;
	if (abs(curVec [Y]) > I2X (1)/8) curVec [Y] /= 4;
	if (abs(curVec [Z]) > I2X (1)/8) curVec [Z] /= 4;

	objP->mType.physInfo.rotVel = curVec;

}

//	gameData.ai.nOverallAgitation affects:
//		Widens field of view.  Field of view is in range 0..1 (specified in bitmaps.tbl as N/360 degrees).
//			gameData.ai.nOverallAgitation/128 subtracted from field of view, making robots see wider.
//		Increases distance to which robotP will search to create path to playerP by gameData.ai.nOverallAgitation/8 segments.
//		Decreases wait between fire times by gameData.ai.nOverallAgitation/64 seconds.

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
					if (Ugly_robot_texture == 999) {
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
//		0		Player is not visible from CObject, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.vBelievedTargetPos as playerP's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int player_is_visible_from_object(CObject *objP, CFixVector *pos, fix fieldOfView, CFixVector *vec_to_player)
{
	fix			dot;
	tCollisionQuery	fq;

fq.p0 = pos;
if ((*pos) == objP->info.position.vPos)
	fq.startSeg	= objP->info.nSegment;
else {
	int nSegment = FindSegByPos (*pos, objP->info.nSegment, 1, 0);
	if (nSegment != -1) 
		fq.startSeg = nSegment;
	else {
		fq.startSeg = objP->info.nSegment;
		*pos = objP->info.position.vPos;
		move_towards_segment_center (objP);
		} 
	} 
#if DBG
if (fq.startSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
fq.p1					= &gameData.ai.vBelievedTargetPos;
fq.radP0				= 0;
fq.radP1				= I2X (1) / 4;
fq.thisObjNum		= objP->Index ();
fq.ignoreObjList	= NULL;
fq.flags				= FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_CHECK_PLAYER;		//what about trans walls???
fq.bCheckVisibility = true;
#if DBG
if (fq.thisObjNum == nDbgObj)
	nDbgObj = nDbgObj;
#endif
hitType = FindHitpoint (&fq, &hitData);
Hit_pos = hitData.hit.vPoint;
Hit_seg = hitData.hit.nSegment;
if (/*(hitType == HIT_NONE) ||*/ ((hitType == HIT_OBJECT) && (hitData.hit.nObject == LOCALPLAYER.nObject))) {
	dot = CFixVector::Dot (*vec_to_player, objP->info.position.mOrient.FVec ());
	return (dot > fieldOfView - (gameData.ai.nOverallAgitation << 9)) ? 2 : 1;
	} 
return 0;
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
int do_silly_animation (CObject *objP)
{
	int				nObject = objP->Index ();
	tJointPos*		jointPositions;
	int				robotType, nGun, robotState, nJointPositions;
	tPolyObjInfo*	polyObjInfo = &objP->rType.polyObjInfo;
	tAIStaticInfo*	aiP = &objP->cType.aiInfo;
	int				nGunCount, at_goal;
	int				attackType;
	int				nFlinchAttackScale = 1;

robotType = objP->info.nId;
if (0 > (nGunCount = gameData.bots.info [1][robotType].nGuns))
	return 0;
attackType = gameData.bots.info [1][robotType].attackType;
robotState = xlatD1Animation [aiP->GOAL_STATE];
if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
	nFlinchAttackScale = nAttackScaleD1;
else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
	nFlinchAttackScale = nFlinchScaleD1;

at_goal = 1;
for (nGun = 0; nGun <= nGunCount; nGun++) {
	nJointPositions = RobotGetAnimState (&jointPositions, robotType, nGun, robotState);
	for (int nJoint = 0; nJoint < nJointPositions; nJoint++) {
		int				jointnum = jointPositions [nJoint].jointnum;

		if (jointnum >= gameData.models.polyModels [0][objP->rType.polyObjInfo.nModel].ModelCount ())
			continue;

		CAngleVector*	jointAngles = &jointPositions [nJoint].angles;
		CAngleVector*	objAngles = &polyObjInfo->animAngles [jointnum];

		for (int nAngle = 0; nAngle < 3; nAngle++) {
			if ((*jointAngles)[nAngle] != (*objAngles)[nAngle]) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum][nAngle] = (*jointAngles)[nAngle];
				fix delta2, deltaAngle = (*jointAngles)[nAngle] - (*objAngles)[nAngle];
				if (deltaAngle >= I2X (1) / 2)
					delta2 = -ANIM_RATE;
				else if (deltaAngle >= 0)
					delta2 = ANIM_RATE;
				else if (deltaAngle >= -I2X (1) / 2)
					delta2 = -ANIM_RATE;
				else
					delta2 = ANIM_RATE;
				if (nFlinchAttackScale != 1)
					delta2 *= nFlinchAttackScale;
				gameData.ai.localInfo [nObject].deltaAngles [jointnum][nAngle] = delta2 / DELTA_ANG_SCALE;		// complete revolutions per second
				}
			}
		}	

	if (at_goal) {
		tAILocalInfo* ailP = &gameData.ai.localInfo [objP->Index ()];
		ailP->achievedState [nGun] = ailP->goalState [nGun];
		if (ailP->achievedState [nGun] == D1_AIS_RECO)
			ailP->goalState [nGun] = D1_AIS_FIRE;
		if (ailP->achievedState [nGun] == D1_AIS_FLIN)
			ailP->goalState [nGun] = D1_AIS_LOCK;
		}
	}
if (at_goal == 1) //nGunCount)
	aiP->CURRENT_STATE = aiP->GOAL_STATE;
return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-objects in an CObject towards their goals.
//	Current orientation of CObject is at:	polyObjInfo.anim_angles
//	Goal orientation of CObject is at:		aiInfo.goalAngles
//	Delta orientation of CObject is at:		aiInfo.deltaAngles
void ai_frame_animation (CObject *objP)
{
	int	nObject = objP->Index ();
	int	nJointCount;

nJointCount = gameData.models.polyModels [0][objP->rType.polyObjInfo.nModel].ModelCount ();

for (int joint = 1; joint < nJointCount; joint++) {
	fix			delta_to_goal;
	fix			scaled_delta_angle;
	CAngleVector	*curangp = &objP->rType.polyObjInfo.animAngles [joint];
	CAngleVector	*goalangp = &gameData.ai.localInfo [nObject].goalAngles [joint];
	CAngleVector	*deltaangp = &gameData.ai.localInfo [nObject].deltaAngles [joint];

	for (int nAngle = 0; nAngle < 3; nAngle++) {
		delta_to_goal = (*goalangp) [nAngle] - (*curangp) [nAngle];
		if (delta_to_goal > 32767)
			delta_to_goal -= 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal += 65536;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul((*deltaangp) [nAngle], gameData.time.xFrame) * DELTA_ANG_SCALE;
			(*curangp) [nAngle] += (fixang) scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				(*curangp) [nAngle] = (*goalangp) [nAngle];
			}
		}
	}
}

// ----------------------------------------------------------------------------------
void SetNextPrimaryFireTime(tAILocalInfo *ailP, tRobotInfo *botInfoP)
{
	ailP->nRapidFireCount++;

if (ailP->nRapidFireCount < botInfoP->nRapidFireCount [gameStates.app.nDifficultyLevel]) 
	ailP->nextPrimaryFire = min(I2X (1)/8, botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
else {
	ailP->nRapidFireCount = 0;
	ailP->nextPrimaryFire = botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the playerP, they attack.
//	If playerP is cloaked, then robotP probably didn't actually collide, deal with that here.
void DoD1AIRobotHitAttack(CObject *robotP, CObject *playerP, CFixVector *vCollision)
{
	tAILocalInfo	*ailP = &gameData.ai.localInfo [OBJECTS.Index (robotP)];
	tRobotInfo		*botInfoP = &gameData.bots.info [1][robotP->info.nId];

//#if DBG
	if (!gameStates.app.cheats.bRobotsFiring)
		return;
//#endif

	//	If playerP is dead, stop firing.
	if (OBJECTS [LOCALPLAYER.nObject].info.nType == OBJ_GHOST)
		return;

	if (botInfoP->attackType == 1) {
		if (ailP->nextPrimaryFire <= 0) {
			if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
				if (CFixVector::Dist (OBJPOS (gameData.objs.consoleP)->vPos, robotP->info.position.vPos) <
					 robotP->info.xSize + gameData.objs.consoleP->info.xSize + I2X (2))
					playerP->CollidePlayerAndNastyRobot (robotP, *vCollision);

			robotP->cType.aiInfo.GOAL_STATE = D1_AIS_RECO;
			SetNextPrimaryFireTime (ailP, botInfoP);
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
//	center of the robotP will not fire right at the playerP.  We need to aim the guns at the playerP.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
void ai_fire_laser_at_player(CObject *objP, CFixVector *fire_point)
{
	int				nObject = objP->Index ();
	tAILocalInfo*	ailP = &gameData.ai.localInfo [nObject];
	tRobotInfo*		botInfoP = &gameData.bots.info [1][objP->info.nId];
	CFixVector		vFire;
	CFixVector		bpp_diff;

	if (!gameStates.app.cheats.bRobotsFiring)
		return;

#if DBG
	//	We should never be coming here for the green guy, as he has no laser!
	if (botInfoP->attackType == 1)
		Int3();	// Contact Mike: This is impossible.
#endif

	if (objP->info.controlType == CT_MORPH)
		return;

	//	If playerP is exploded, stop firing.
	if (gameStates.app.bPlayerExploded)
		return;

	//	If playerP is cloaked, maybe don't fire based on how long cloaked and randomness.
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	cloak_time = gameData.ai.cloakInfo [nObject % D1_MAX_AI_CLOAK_INFO].lastTime;

		if (gameData.time.xGame - cloak_time > CLOAK_TIME_MAX/4)
			if (d_rand() > FixDiv(gameData.time.xGame - cloak_time, CLOAK_TIME_MAX)/2) {
				SetNextPrimaryFireTime(ailP, botInfoP);
				return;
			}
	}

	//	Set position to fire at based on difficulty level.
	bpp_diff [X] = gameData.ai.vBelievedTargetPos [X] + (d_rand()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4;
	bpp_diff [Y] = gameData.ai.vBelievedTargetPos [Y] + (d_rand()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4;
	bpp_diff [Z] = gameData.ai.vBelievedTargetPos [Z] + (d_rand()-16384) * (NDL-gameStates.app.nDifficultyLevel-1) * 4;

	//	Half the time fire at the playerP, half the time lead the playerP.
	if (d_rand() > 16384) {

		CFixVector::NormalizedDir(vFire, bpp_diff, *fire_point);

		}
	else {
		CFixVector	vPlayerDirection = bpp_diff - bpp_diff;

		// If playerP is not moving, fire right at him!
		//	Note: If the robotP fires in the direction of its forward vector, this is bad because the weapon does not
		//	come out from the center of the robotP; it comes out from the side.  So it is common for the weapon to miss
		//	its target.  Ideally, we want to point the guns at the playerP.  For now, just fire right at the playerP.
		if ((abs(vPlayerDirection [X] < 0x10000)) && (abs(vPlayerDirection [Y] < 0x10000)) && (abs(vPlayerDirection [Z] < 0x10000))) {

			CFixVector::NormalizedDir (vFire, bpp_diff, *fire_point);

		// Player is moving.  Determine where the playerP will be at the end of the next frame if he doesn't change his
		//	behavior.  Fire at exactly that point.  This isn't exactly what you want because it will probably take the laser
		//	a different amount of time to get there, since it will probably be a different distance from the playerP.
		//	So, that's why we write games, instead of guiding missiles...
			} 
		else {
			vFire = bpp_diff - *fire_point;
			vFire *= FixMul (WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel), gameData.time.xFrame);

			vFire += vPlayerDirection;
			CFixVector::Normalize (vFire);

		}
	}

	CreateNewLaserEasy ( &vFire, fire_point, objP->Index (), botInfoP->nWeaponType, 1);

	if (IsMultiGame)
 {
		ai_multi_send_robot_position(nObject, -1);
		MultiSendRobotFire (nObject, objP->cType.aiInfo.CURRENT_GUN, &vFire);
	}

	CreateAwarenessEvent (objP, D1_PA_NEARBY_ROBOT_FIRED);

	SetNextPrimaryFireTime(ailP, botInfoP);

	//	If the boss fired, allow him to teleport very soon (right after firing, cool!), pending other factors.
	if (botInfoP->bossFlag)
		gameData.bosses [0].m_nLastTeleportTime -= gameData.bosses [0].m_nTeleportInterval/2;
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
void move_towards_vector(CObject *objP, CFixVector *vec_goal)
{
	tPhysicsInfo	*piP = &objP->mType.physInfo;
	fix				speed, dot, xMaxSpeed;
	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->info.nId];
	CFixVector		vel;

	//	Trying to move towards playerP.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards playerP as usual.

	vel = piP->velocity;
	CFixVector::Normalize (vel);
	dot = CFixVector::Dot (vel, objP->info.position.mOrient.FVec ());

	if (dot < I2X (3)/4) {
		//	This funny code is supposed to slow down the robotP and move his velocity towards his direction
		//	more quickly than the general code
		piP->velocity [X] = piP->velocity [X]/2 + FixMul((*vec_goal) [X], gameData.time.xFrame*32);
		piP->velocity [Y] = piP->velocity [Y]/2 + FixMul((*vec_goal) [Y], gameData.time.xFrame*32);
		piP->velocity [Z] = piP->velocity [Z]/2 + FixMul((*vec_goal) [Z], gameData.time.xFrame*32);
	} else {
		piP->velocity [X] += FixMul((*vec_goal) [X], gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		piP->velocity [Y] += FixMul((*vec_goal) [Y], gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
		piP->velocity [Z] += FixMul((*vec_goal) [Z], gameData.time.xFrame*64) * (gameStates.app.nDifficultyLevel+5)/4;
	}

	speed = piP->velocity.Mag();
	xMaxSpeed = botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel];

	//	Green guy attacks twice as fast as he moves away.
	if (botInfoP->attackType == 1)
		xMaxSpeed *= 2;

	if (speed > xMaxSpeed) {
		piP->velocity [X] = (piP->velocity [X]*3)/4;
		piP->velocity [Y] = (piP->velocity [Y]*3)/4;
		piP->velocity [Z] = (piP->velocity [Z]*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_towards_player(CObject *objP, CFixVector *vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
	move_towards_vector(objP, vec_to_player);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means Normal slide about.  fast_flag = 0 means no evasion.
void move_around_player(CObject *objP, CFixVector *vec_to_player, int fast_flag)
{
	tPhysicsInfo	*piP = &objP->mType.physInfo;
	fix				speed;
	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->info.nId];
	int				nObject = objP->Index ();
	int				dir;
	int				dir_change;
	fix				ft;
	CFixVector		vEvade;
	int				count=0;

	if (fast_flag == 0)
		return;

	vEvade.SetZero ();
	dir_change = 48;
	ft = gameData.time.xFrame;
	if (ft < I2X (1)/32) {
		dir_change *= 8;
		count += 3;
	} else
		while (ft < I2X (1)/4) {
			dir_change *= 2;
			ft *= 2;
			count++;
		}

	dir = (gameData.app.nFrameCount + (count+1) * (nObject*8 + nObject*4 + nObject)) & dir_change;
	dir >>= (4+count);

	Assert((dir >= 0) && (dir <= 3));

	switch (dir) {
			case 0:
				vEvade [X] = FixMul((*vec_to_player) [Z], gameData.time.xFrame*32);
				vEvade [Y] = FixMul((*vec_to_player) [Y], gameData.time.xFrame*32);
				vEvade [Z] = FixMul(-(*vec_to_player) [X], gameData.time.xFrame*32);
				break;
			case 1:
				vEvade [X] = FixMul(-(*vec_to_player) [Z], gameData.time.xFrame*32);
				vEvade [Y] = FixMul((*vec_to_player) [Y], gameData.time.xFrame*32);
				vEvade [Z] = FixMul((*vec_to_player) [X], gameData.time.xFrame*32);
				break;
			case 2:
				vEvade [X] = FixMul(-(*vec_to_player) [Y], gameData.time.xFrame*32);
				vEvade [Y] = FixMul((*vec_to_player) [X], gameData.time.xFrame*32);
				vEvade [Z] = FixMul((*vec_to_player) [Z], gameData.time.xFrame*32);
				break;
			case 3:
				vEvade [X] = FixMul((*vec_to_player) [Y], gameData.time.xFrame*32);
				vEvade [Y] = FixMul(-(*vec_to_player) [X], gameData.time.xFrame*32);
				vEvade [Z] = FixMul((*vec_to_player) [Z], gameData.time.xFrame*32);
				break;
		}

	//	Note: -1 means Normal circling about the playerP.  > 0 means fast evasion.
	if (fast_flag > 0) {
		fix	dot;

		//	Only take evasive action if looking at playerP.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = CFixVector::Dot (*vec_to_player, objP->info.position.mOrient.FVec ());
		if ((dot > botInfoP->fieldOfView [gameStates.app.nDifficultyLevel]) && !(gameData.objs.consoleP->info.nFlags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			damage_scale = FixDiv (objP->info.xShields, botInfoP->strength);
			if (damage_scale > I2X (1))
				damage_scale = I2X (1);		//	Just in case...
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in case...

			vEvade *= (I2X(fast_flag) + damage_scale);
		}
	}

	piP->velocity += vEvade;

	speed = piP->velocity.Mag();
	if (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
		piP->velocity [X] = (piP->velocity [X]*3)/4;
		piP->velocity [Y] = (piP->velocity [Y]*3)/4;
		piP->velocity [Z] = (piP->velocity [Z]*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
void move_away_from_player(CObject *objP, CFixVector *vec_to_player, int attackType)
{
	fix				speed;
	tPhysicsInfo	*piP = &objP->mType.physInfo;
	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->info.nId];
	int				objref;

	piP->velocity -= *vec_to_player * (gameData.time.xFrame*16);

	if (attackType) {
		//	Get value in 0..3 to choose evasion direction.
		objref = (objP->Index () ^ ((gameData.app.nFrameCount + 3*objP->Index ()) >> 5)) & 3;

		switch (objref) {
			case 0:	piP->velocity += objP->info.position.mOrient.UVec () * ( gameData.time.xFrame << 5);	break;
			case 1:	piP->velocity += objP->info.position.mOrient.UVec () * (-gameData.time.xFrame << 5);	break;
			case 2:	piP->velocity += objP->info.position.mOrient.RVec () * ( gameData.time.xFrame << 5);	break;
			case 3:	piP->velocity += objP->info.position.mOrient.RVec () * (-gameData.time.xFrame << 5);	break;
			default:	Int3();	//	Impossible, bogus value on objref, must be in 0..3
		}
	}


	speed = piP->velocity.Mag();

	if (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
		piP->velocity [X] = (piP->velocity [X]*3)/4;
		piP->velocity [Y] = (piP->velocity [Y]*3)/4;
		piP->velocity [Z] = (piP->velocity [Z]*3)/4;
	}

//--old--	fix				speed, dot;
//--old--	tPhysicsInfo	*piP = &objP->mType.physInfo;
//--old--	tRobotInfo		*botInfoP = &gameData.bots.info [1][objP->info.nId];
//--old--
//--old--	//	Trying to move away from playerP.  If forward vector much different than velocity vector,
//--old--	//	bash velocity vector twice as much away from playerP as usual.
//--old--	dot = VmVecDot (&piP->velocity, &objP->info.position.mOrient.FVec ());
//--old--	if (dot > -I2X (3)/4) {
//--old--		//	This funny code is supposed to slow down the robotP and move his velocity towards his direction
//--old--		//	more quickly than the general code
//--old--		piP->velocity [X] = piP->velocity [X]/2 - FixMul(vec_to_player->p.x, gameData.time.xFrame*16);
//--old--		piP->velocity [Y] = piP->velocity [Y]/2 - FixMul(vec_to_player->p.y, gameData.time.xFrame*16);
//--old--		piP->velocity [Z] = piP->velocity [Z]/2 - FixMul(vec_to_player->p.z, gameData.time.xFrame*16);
//--old--	} else {
//--old--		piP->velocity [X] -= FixMul(vec_to_player->p.x, gameData.time.xFrame*16);
//--old--		piP->velocity [Y] -= FixMul(vec_to_player->p.y, gameData.time.xFrame*16);
//--old--		piP->velocity [Z] -= FixMul(vec_to_player->p.z, gameData.time.xFrame*16);
//--old--	}
//--old--
//--old--	speed = VmVecMag(&piP->velocity);
//--old--
//--old--	if (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
//--old--		piP->velocity [X] = (piP->velocity [X]*3)/4;
//--old--		piP->velocity [Y] = (piP->velocity [Y]*3)/4;
//--old--		piP->velocity [Z] = (piP->velocity [Z]*3)/4;
//--old--	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around playerP.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == D1_AIM_STILL).
void ai_move_relative_to_player(CObject *objP, tAILocalInfo *ailP, fix dist_to_player, CFixVector *vec_to_player, fix circle_distance, int evade_only)
{
	CObject		*dangerObjP;
	tRobotInfo	*botInfoP = &gameData.bots.info [1][objP->info.nId];

	//	See if should take avoidance.

// New way, green guys don't evade:	if ((botInfoP->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
	if (objP->cType.aiInfo.nDangerLaser != -1) {
		dangerObjP = &OBJECTS [objP->cType.aiInfo.nDangerLaser];

		if ((dangerObjP->info.nType == OBJ_WEAPON) && (dangerObjP->info.nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
			fix			dot, dist_to_laser, fieldOfView;
			CFixVector	vec_to_laser, laser_fVec;

			fieldOfView = gameData.bots.info [1][objP->info.nId].fieldOfView [gameStates.app.nDifficultyLevel];

			vec_to_laser = dangerObjP->info.position.vPos - objP->info.position.vPos;
			dist_to_laser = CFixVector::Normalize (vec_to_laser);
			dot = CFixVector::Dot (vec_to_laser, objP->info.position.mOrient.FVec ());

			if (dot > fieldOfView) {
				fix			laser_robot_dot;
				CFixVector	laser_vec_to_robot;

				//	The laser is seen by the robotP, see if it might hit the robotP.
				//	Get the laser's direction.  If it's a polyobj, it can be gotten cheaply from the orientation matrix.
				if (dangerObjP->info.renderType == RT_POLYOBJ)
					laser_fVec = dangerObjP->info.position.mOrient.FVec ();
				else {		//	Not a polyobj, get velocity and Normalize.
					laser_fVec = dangerObjP->mType.physInfo.velocity;	//dangerObjP->info.position.mOrient.FVec ();
					CFixVector::Normalize (laser_fVec);
				}
				laser_vec_to_robot = objP->info.position.vPos - dangerObjP->info.position.vPos;
				CFixVector::Normalize (laser_vec_to_robot);
				laser_robot_dot = CFixVector::Dot (laser_fVec, laser_vec_to_robot);

				if ((laser_robot_dot > I2X (7) / 8) && (dist_to_laser < I2X (80))) {
					int	evadeSpeed;

					D1_AI_evaded = 1;
					evadeSpeed = gameData.bots.info [1][objP->info.nId].evadeSpeed [gameStates.app.nDifficultyLevel];

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

	//	If we fall out of above, then no CObject to be avoided.
	objP->cType.aiInfo.nDangerLaser = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
	if (botInfoP->attackType == 1) {
		if (((ailP->nextPrimaryFire > botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) && (dist_to_player < I2X (30))) || gameStates.app.bPlayerIsDead) {
			//	1/4 of time, move around playerP, 3/4 of time, move away from playerP
			if (d_rand() < 8192) {
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


//	-------------------------------------------------------------------------------------------------------------------
int	Break_on_object = -1;

void do_firing_stuff(CObject *objP, int player_visibility, CFixVector *vec_to_player)
{
	if (player_visibility >= 1) {
		//	Now, if in robotP's field of view, lock onto playerP
		fix	dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), *vec_to_player);
		if ((dot >= I2X (7)/8) || (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
			tAIStaticInfo	*aiP = &objP->cType.aiInfo;
			tAILocalInfo		*ailP = &gameData.ai.localInfo [objP->Index ()];

			switch (aiP->GOAL_STATE) {
				case D1_AIS_NONE:
				case D1_AIS_REST:
				case D1_AIS_SRCH:
				case D1_AIS_LOCK:
					aiP->GOAL_STATE = D1_AIS_FIRE;
					if (ailP->targetAwarenessType <= D1_PA_NEARBY_ROBOT_FIRED) {
						ailP->targetAwarenessType = D1_PA_NEARBY_ROBOT_FIRED;
						ailP->targetAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
					}
					break;
			}
		} else if (dot >= I2X (1)/2) {
			tAIStaticInfo	*aiP = &objP->cType.aiInfo;
			switch (aiP->GOAL_STATE) {
				case D1_AIS_NONE:
				case D1_AIS_REST:
				case D1_AIS_SRCH:
					aiP->GOAL_STATE = D1_AIS_LOCK;
					break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robotP gets bumped or hit, he decides to find another hiding place.
void DoD1AIRobotHit (CObject *objP, int type)
{
	if (objP->info.controlType == CT_AI) {
		if ((type == D1_PA_WEAPON_ROBOT_COLLISION) || (type == D1_PA_PLAYER_COLLISION))
			switch (objP->cType.aiInfo.behavior) {
				case D1_AIM_HIDE:
					objP->cType.aiInfo.SUBMODE = AISM_GOHIDE;
					break;
				case D1_AIM_STILL:
					gameData.ai.localInfo [objP->Index ()].mode = D1_AIM_CHASE_OBJECT;
					break;
			}
	}

}
#if DBG
int	Do_ai_flag=1;
#endif

#define	CHASE_TIME_LENGTH		(I2X (8))

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely player_is_visible_from_object would benefit from the
//	information of a normalized vec_to_player.
//	Return playerP visibility:
//		0		not visible
//		1		visible, but robotP not looking at playerP (ie, on an unobstructed vector)
//		2		visible and in robotP's field of view
//		-1		playerP is cloaked
//	If the playerP is cloaked, set vec_to_player based on time playerP cloaked and last uncloaked position.
//	Updates ailP->nPrevVisibility if playerP is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to player_visibility
void compute_vis_and_vec(CObject *objP, CFixVector *pos, tAILocalInfo *ailP, CFixVector *vec_to_player, int *player_visibility, tRobotInfo *botInfoP, int *flag)
{
if (!*flag) {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	delta_time, dist;
		int	cloak_index = objP->Index () % D1_MAX_AI_CLOAK_INFO;

		delta_time = gameData.time.xGame - gameData.ai.cloakInfo [cloak_index].lastTime;
		if (delta_time > I2X (2)) {
			gameData.ai.cloakInfo [cloak_index].lastTime = gameData.time.xGame;
			CFixVector randvec = CFixVector::Random();
			gameData.ai.cloakInfo [cloak_index].vLastPos += randvec * (8*delta_time);
			}

		dist = CFixVector::NormalizedDir (*vec_to_player, gameData.ai.cloakInfo [cloak_index].vLastPos, *pos);
		*player_visibility = player_is_visible_from_object (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], vec_to_player);
		// *player_visibility = 2;

		if ((ailP->nextMiscSoundTime < gameData.time.xGame) && (ailP->nextPrimaryFire < I2X (1)) && (dist < I2X (20))) {
			ailP->nextMiscSoundTime = gameData.time.xGame + (d_rand() + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 1;
			audio.CreateSegmentSound (botInfoP->seeSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
			}
		} 
	else {
		//	Compute expensive stuff -- vec_to_player and player_visibility
		CFixVector::NormalizedDir (*vec_to_player, gameData.ai.vBelievedTargetPos, *pos);
		if (vec_to_player->IsZero ()) {
			(*vec_to_player) [X] = I2X (1);
			}
		*player_visibility = player_is_visible_from_object(objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], vec_to_player);

		//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
		//	see you without killing frame rate.
		tAIStaticInfo	*aiP = &objP->cType.aiInfo;
		if ((*player_visibility == 2) && (ailP->nPrevVisibility != 2))
			if ((aiP->GOAL_STATE == D1_AIS_REST) || (aiP->CURRENT_STATE == D1_AIS_REST)) {
				aiP->GOAL_STATE = D1_AIS_FIRE;
				aiP->CURRENT_STATE = D1_AIS_FIRE;
				}

		if (!gameStates.app.bPlayerExploded && (ailP->nPrevVisibility != *player_visibility) && (*player_visibility == 2)) {
			if (ailP->nPrevVisibility == 0) {
				if (ailP->timeTargetSeen + I2X (1)/2 < gameData.time.xGame) {
					audio.CreateSegmentSound (botInfoP->seeSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
					ailP->timeTargetSoundAttacked = gameData.time.xGame;
					ailP->nextMiscSoundTime = gameData.time.xGame + I2X (1) + d_rand()*4;
					}
				} 
			else if (ailP->timeTargetSoundAttacked + I2X (1)/4 < gameData.time.xGame) {
				audio.CreateSegmentSound (botInfoP->attackSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
				ailP->timeTargetSoundAttacked = gameData.time.xGame;
				}
			}

		if ((*player_visibility == 2) && (ailP->nextMiscSoundTime < gameData.time.xGame)) {
			ailP->nextMiscSoundTime = gameData.time.xGame + (d_rand() + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 2;
			audio.CreateSegmentSound (botInfoP->attackSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
			}
		ailP->nPrevVisibility = *player_visibility;
		}
	*flag = 1;
	if (*player_visibility)
		ailP->timeTargetSeen = gameData.time.xGame;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move the CObject objP to a spot in which it doesn't intersect a wall.
//	It might mean moving it outside its current CSegment.
void move_object_to_legal_spot(CObject *objP)
{
	CFixVector	original_pos = objP->info.position.vPos;
	int			i;
	CSegment*	segP = SEGMENTS + objP->info.nSegment;

	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
		if (segP->IsDoorWay (i, objP) & WID_FLY_FLAG) {
			CFixVector	vSegCenter, goal_dir;
			fix			dist_to_center;

			vSegCenter = SEGMENTS [objP->info.nSegment].Center ();
			goal_dir = vSegCenter - objP->info.position.vPos;
			dist_to_center = CFixVector::Normalize (goal_dir);
			goal_dir *= objP->info.xSize;
			objP->info.position.vPos += goal_dir;
			if (!ObjectIntersectsWall(objP)) {
				int	nNewSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);

				if (nNewSeg != -1) {
					objP->RelinkToSeg (nNewSeg);
					return;
				}
			} else
				objP->info.position.vPos = original_pos;
		}
	}

	// Int3();		//	Darn you John, you done it again!  (But contact Mike)
	objP->ApplyDamageToRobot(objP->info.xShields*2, objP->Index ());
}

// --------------------------------------------------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
void move_towards_segment_center(CObject *objP)
{
	fix			dist_to_center;
	CFixVector	vSegCenter, goal_dir;

	vSegCenter = SEGMENTS [objP->info.nSegment].Center ();
	goal_dir = vSegCenter - objP->info.position.vPos;
	dist_to_center = CFixVector::Normalize (goal_dir);
	if (dist_to_center < objP->info.xSize) {
		//	Center is nearer than the distance we want to move, so move to center.
		objP->info.position.vPos = vSegCenter;
		if (ObjectIntersectsWall(objP)) {
			move_object_to_legal_spot(objP);
		}
	} else {
		int	nNewSeg;
		//	Move one radii towards center.
		goal_dir *= objP->info.xSize;
		objP->info.position.vPos += goal_dir;
		nNewSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
		if (nNewSeg == -1) {
			objP->info.position.vPos = vSegCenter;
			move_object_to_legal_spot(objP);
		}
	}

}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type robotP.
//	Only brains and avoid robots can open doors.
int ai_door_is_openable(CObject *objP, CSegment *segP, int sidenum)
{
	int	nWall;

	//	The mighty console CObject can open all doors (for purposes of determining paths).
	if (objP == gameData.objs.consoleP) {
		int	nWall = segP->m_sides [sidenum].m_nWall;

		if (WALLS [nWall].nType == WALL_DOOR)
			return 1;
	}

	if ((objP->info.nId == ROBOT_BRAIN) || (objP->cType.aiInfo.behavior == D1_AIB_RUN_FROM)) {
		nWall = segP->m_sides [sidenum].m_nWall;

		if (nWall != -1)
			if ((WALLS [nWall].nType == WALL_DOOR) && (WALLS [nWall].keys == KEY_NONE) && !(WALLS [nWall].flags & WALL_DOOR_LOCKED))
				return 1;
	}

	return 0;
}

//--//	-----------------------------------------------------------------------------------------------------------
//--//	Return true if CObject *objP is allowed to open door at nWall
//--int door_openable_by_robot(CObject *objP, int nWall)
//--{
//--	if (objP->info.nId == ROBOT_BRAIN)
//--		if (WALLS [nWall].keys == KEY_NONE)
//--			return 1;
//--
//--	return 0;
//--}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if a special CObject (playerP or control center) is in this CSegment.
int special_object_in_seg (int nSegment)
{
	int nObject = SEGMENTS [nSegment].m_objects;

while (nObject != -1) {
	if ((OBJECTS [nObject].info.nType == OBJ_PLAYER) || (OBJECTS [nObject].info.nType == OBJ_REACTOR))
		return 1;
	nObject = OBJECTS [nObject].info.nNextInSeg;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Randomly select a CSegment attached to *segP, reachable by flying.
int get_random_child(int nSegment)
{
CSegment	*segP = SEGMENTS + nSegment;
int sidenum = (d_rand() * 6) >> 15;
while (!(segP->IsDoorWay (sidenum, NULL) & WID_FLY_FLAG))
	sidenum = (d_rand() * 6) >> 15;
return segP->m_children [sidenum];
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if CObject created, else return false.
int CreateGatedRobot (int nSegment, int nObjId)
{
	int			nObject;
	CObject		*objP;
	CSegment		*segP = &SEGMENTS [nSegment];
	CFixVector	vObjPos;
	tRobotInfo	*botInfoP = &gameData.bots.info [1][nObjId];
	int			count = 0;
	//int			i;
	fix			objsize = gameData.models.polyModels [0][botInfoP->nModel].Rad ();
	int			default_behavior;

	FORALL_ROBOT_OBJS (objP, i) {
		if (objP->info.nCreator == BOSS_GATE_MATCEN_NUM)
			count++;
		}
	if (count > 2 * gameStates.app.nDifficultyLevel + 3) {
		gameData.bosses [0].m_nLastGateTime = gameData.time.xGame - 3 * gameData.bosses [0].m_nGateInterval / 4;
		return 0;
		}
	vObjPos = segP->RandomPoint ();
	//	See if legal to place CObject here.  If not, move about in CSegment and try again.
	if (CheckObjectObjectIntersection(&vObjPos, objsize, segP)) {
		gameData.bosses [0].m_nLastGateTime = gameData.time.xGame - 3*gameData.bosses [0].m_nGateInterval/4;
		return 0;
		}
	nObject = CreateRobot (nObjId, nSegment, vObjPos);
	if (nObject < 0) {
		gameData.bosses [0].m_nLastGateTime = gameData.time.xGame - 3*gameData.bosses [0].m_nGateInterval/4;
		return -1;
		}

	objP = &OBJECTS [nObject];
	//Set polygon-CObject-specific data
	objP->rType.polyObjInfo.nModel = botInfoP->nModel;
	objP->rType.polyObjInfo.nSubObjFlags = 0;
	//set Physics info
	objP->mType.physInfo.mass = botInfoP->mass;
	objP->mType.physInfo.drag = botInfoP->drag;
	objP->mType.physInfo.flags |= (PF_LEVELLING);
	objP->info.xShields = botInfoP->strength;
	objP->info.nCreator = BOSS_GATE_MATCEN_NUM;	//	flag this robotP as having been created by the boss.
	default_behavior = D1_AIB_NORMAL;
	if (nObjId == 10)						//	This is a toaster guy!
		default_behavior = D1_AIB_RUN_FROM;
	InitAIObject (objP->Index (), default_behavior, -1 );		//	Note, -1 = CSegment this robotP goes to to hide, should probably be something useful
	/*Object*/CreateExplosion (nSegment, vObjPos, I2X(10), VCLIP_MORPHING_ROBOT);
	audio.CreateSegmentSound (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nSegment, 0, vObjPos, 0, I2X (1));
	objP->MorphStart ();

	gameData.bosses [0].m_nLastGateTime = gameData.time.xGame;
	LOCALPLAYER.numRobotsLevel++;
	LOCALPLAYER.numRobotsTotal++;
	return nObject;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make CObject objP gate in a robotP.
//	The process of him bringing in a robotP takes one second.
//	Then a robotP appears somewhere near the playerP.
//	Return true if robotP successfully created, else return false
int gate_in_robot(int type, int nSegment)
{
if (!gameData.bosses.ToS ())
	return -1;
if (nSegment < 0) {
	if (!(gameData.bosses [0].m_gateSegs.Buffer () && gameData.bosses [0].m_nGateSegs))
		return -1;
	nSegment = gameData.bosses [0].m_gateSegs [(d_rand () * gameData.bosses [0].m_nGateSegs) >> 15];
	}
return CreateGatedRobot (nSegment, type);
}

// --------------------------------------------------------------------------------------------------------------------
int boss_fits_in_seg(CObject *bossObjP, int nSegment)
{
	CFixVector	vCenter;
	int			nBossObj = bossObjP - OBJECTS;
	int			posnum;

vCenter = SEGMENTS [nSegment].Center ();
for (posnum=0; posnum<9; posnum++) {
	if (posnum > 0) {
		CFixVector	vertex_pos;

		Assert((posnum-1 >= 0) && (posnum-1 < 8));
		vertex_pos = gameData.segs.vertices [SEGMENTS [nSegment].m_verts [posnum-1]];
		bossObjP->info.position.vPos = CFixVector::Avg(vertex_pos, vCenter);
	} else
		bossObjP->info.position.vPos = vCenter;

	OBJECTS [nBossObj].RelinkToSeg(nSegment);
	if (!ObjectIntersectsWall(bossObjP))
		return 1;
	}
return 0;
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI CObject if it is fairly aware of the playerP.
//	awareness_level is in 0..100.  Larger numbers indicate greater awareness (eg, 99 if firing at playerP).
//	In a given frame, might not get called for an CObject, or might be called more than once.
//	The fact that this routine is not called for a given CObject does not mean that CObject is not interested in the playerP.
//	OBJECTS are moved by physics, so they can move even if not interested in a playerP.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this playerP IS NOT allowed to move this robotP.
//		1	this playerP IS allowed to move this robotP.
int ai_multiplayer_awareness(CObject *objP, int awareness_level)
{
	int	rval=1;

	if (IsMultiGame) {
		if (awareness_level == 0)
			return 0;
		rval = MultiCanRemoveRobot(objP->Index (), awareness_level);
	}
	return rval;

}
#if DBG
fix	Prev_boss_shields = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_boss_stuff(CObject *objP)
{
    //  New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
    if (gameData.bosses [0].m_nLastTeleportTime > gameData.time.xGame)
        gameData.bosses [0].m_nLastTeleportTime = gameData.time.xGame;

    if (gameData.bosses [0].m_nLastGateTime > gameData.time.xGame)
        gameData.bosses [0].m_nLastGateTime = gameData.time.xGame;

	if (!gameData.bosses [0].m_nDying) {
		if (objP->cType.aiInfo.CLOAKED == 1) {
			if ((gameData.time.xGame - gameData.bosses [0].m_nCloakStartTime > BOSS_CLOAK_DURATION/3) && (gameData.bosses [0].m_nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION/3) && (gameData.time.xGame - gameData.bosses [0].m_nLastTeleportTime > gameData.bosses [0].m_nTeleportInterval)) {
				if (ai_multiplayer_awareness(objP, 98))
					TeleportBoss(objP);
			} else if (gameData.bosses [0].m_bHitThisFrame) {
				gameData.bosses [0].m_bHitThisFrame = 0;
				gameData.bosses [0].m_nLastTeleportTime -= gameData.bosses [0].m_nTeleportInterval/4;
			}

			if (gameData.time.xGame > gameData.bosses [0].m_nCloakEndTime)
				objP->cType.aiInfo.CLOAKED = 0;
		} else {
			if ((gameData.time.xGame - gameData.bosses [0].m_nCloakEndTime > gameData.bosses [0].m_nCloakInterval) || gameData.bosses [0].m_bHitThisFrame) {
				if (ai_multiplayer_awareness(objP, 95))
			 {
					gameData.bosses [0].m_bHitThisFrame = 0;
					gameData.bosses [0].m_nCloakStartTime = gameData.time.xGame;
					gameData.bosses [0].m_nCloakEndTime = gameData.time.xGame+BOSS_CLOAK_DURATION;
					objP->cType.aiInfo.CLOAKED = 1;
					if (IsMultiGame)
						MultiSendBossActions(objP->Index (), 2, 0, 0);
				}
			}
		}
	} else
		DoBossDyingFrame (objP);

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	(I2X (150))

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_super_boss_stuff(CObject *objP, fix dist_to_player, int player_visibility)
{
	static int eclipState = 0;
	do_boss_stuff(objP);

	// Only master playerP can cause gating to occur.
	if ((IsMultiGame) && !NetworkIAmMaster())
		return;

	if ((dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE) || player_visibility || (IsMultiGame)) {
		if (gameData.time.xGame - gameData.bosses [0].m_nLastGateTime > gameData.bosses [0].m_nGateInterval/2) {
			RestartEffect(BOSS_ECLIP_NUM);
			if (eclipState == 0) {
				MultiSendBossActions(objP->Index (), 4, 0, 0);
				eclipState = 1;
			}
		}
		else {
			StopEffect(BOSS_ECLIP_NUM);
			if (eclipState == 1) {
				MultiSendBossActions(objP->Index (), 5, 0, 0);
				eclipState = 0;
			}
		}

		if (gameData.time.xGame - gameData.bosses [0].m_nLastGateTime > gameData.bosses [0].m_nGateInterval)
			if (ai_multiplayer_awareness(objP, 99)) {
				int	nObject;
				int	randtype = (d_rand() * D1_MAX_GATE_INDEX) >> 15;

				Assert(randtype < D1_MAX_GATE_INDEX);
				randtype = super_boss_gate_list [randtype];
				Assert(randtype < gameData.bots.nTypes [1]);

				nObject = gate_in_robot(randtype, -1);
				if ((nObject >= 0) && (IsMultiGame))
			 {
					MultiSendBossActions(objP->Index (), 3, randtype, nObject);
					MapObjnumLocalToLocal (nObject);

				}
			}
	}
}


// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this CObject should be allowed to fire at the playerP.
int maybe_ai_do_actual_firing_stuff(CObject *objP, tAIStaticInfo *aiP)
{
	if (IsMultiGame)
		if ((aiP->GOAL_STATE != D1_AIS_FLIN) && (objP->info.nId != ROBOT_BRAIN))
			if (aiP->CURRENT_STATE == D1_AIS_FIRE)
				return 1;

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
void ai_do_actual_firing_stuff(CObject *objP, tAIStaticInfo *aiP, tAILocalInfo *ailP, tRobotInfo *botInfoP, CFixVector *vec_to_player, fix dist_to_player, CFixVector *vGunPoint, int player_visibility, int object_animates)
{
	fix	dot;

	if (player_visibility == 2) {
		//	Changed by mk, 01/04/94, onearm would take about 9 seconds until he can fire at you.
		// if (((!object_animates) || (ailP->achievedState [aiP->CURRENT_GUN] == D1_AIS_FIRE)) && (ailP->nextPrimaryFire <= 0)) {
		if (!object_animates || (ailP->nextPrimaryFire <= 0)) {
			dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), *vec_to_player);
			if (dot >= I2X (7)/8) {

				if (aiP->CURRENT_GUN < gameData.bots.info [1][objP->info.nId].nGuns) {
					if (botInfoP->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (dist_to_player < objP->info.xSize + gameData.objs.consoleP->info.xSize + I2X (2))) {		// botInfoP->circle_distance [gameStates.app.nDifficultyLevel] + gameData.objs.consoleP->info.xSize) {
							if (!ai_multiplayer_awareness(objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoD1AIRobotHitAttack(objP, gameData.objs.consoleP, &objP->info.position.vPos);
						} else {
							return;
						}
					} else {
						if (vGunPoint->IsZero ()) {
							;
						} else {
							if (!ai_multiplayer_awareness(objP, ROBOT_FIRE_AGITATION))
								return;
							ai_fire_laser_at_player(objP, vGunPoint);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ( (aiP->behavior != D1_AIB_RUN_FROM) && (aiP->behavior != D1_AIB_STILL) && (aiP->behavior != D1_AIB_FOLLOW_PATH) && ((ailP->mode == D1_AIM_FOLLOW_PATH) || (ailP->mode == D1_AIM_STILL)))
						ailP->mode = D1_AIM_CHASE_OBJECT;
				}

				aiP->GOAL_STATE = D1_AIS_RECO;
				ailP->goalState [aiP->CURRENT_GUN] = D1_AIS_RECO;

				// Switch to next gun for next fire.
				aiP->CURRENT_GUN++;
				if (aiP->CURRENT_GUN >= gameData.bots.info [1][objP->info.nId].nGuns)
					aiP->CURRENT_GUN = 0;
			}
		}
	} else if (WI_homingFlag (objP->info.nId) == 1) {
		//	Robots which fire homing weapons might fire even if they don't have a bead on the playerP.
		if (((!object_animates) || (ailP->achievedState [aiP->CURRENT_GUN] == D1_AIS_FIRE)) && (ailP->nextPrimaryFire <= 0) && (CFixVector::Dist(Hit_pos, objP->info.position.vPos) > I2X (40))) {
			if (!ai_multiplayer_awareness(objP, ROBOT_FIRE_AGITATION))
				return;
			ai_fire_laser_at_player(objP, vGunPoint);

			aiP->GOAL_STATE = D1_AIS_RECO;
			ailP->goalState [aiP->CURRENT_GUN] = D1_AIS_RECO;

			// Switch to next gun for next fire.
			aiP->CURRENT_GUN++;
			if (aiP->CURRENT_GUN >= gameData.bots.info [1][objP->info.nId].nGuns)
				aiP->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aiP->CURRENT_GUN++;
			if (aiP->CURRENT_GUN >= gameData.bots.info [1][objP->info.nId].nGuns)
				aiP->CURRENT_GUN = 0;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
void DoD1AIFrame (CObject *objP)
{
	int				nObject = objP->Index ();
	tAIStaticInfo	*aiP = &objP->cType.aiInfo;
	tAILocalInfo	*ailP = &gameData.ai.localInfo [nObject];
	fix				dist_to_player;
	CFixVector		vec_to_player;
	fix				dot;
	tRobotInfo		*botInfoP;
	int				player_visibility=-1;
	int				obj_ref;
	int				object_animates;
	int				new_goalState;
	int				bVisAndVecComputed = 0;
	int				nPrevVisibility;
	CFixVector		vGunPoint;
	CFixVector		vVisVecPos;

	if (aiP->SKIP_D1_AI_COUNT) {
		aiP->SKIP_D1_AI_COUNT--;
		return;
	}
#if DBG
if (nObject == nDbgObj)
	nDbgObj = nDbgObj;
#endif
	//	Kind of a hack.  If a robotP is flinching, but it is time for it to fire, unflinch it.
	//	Else, you can turn a big nasty robotP into a wimp by firing flares at it.
	//	This also allows the playerP to see the cool flinch effect for mechs without unbalancing the game.
	if ((aiP->GOAL_STATE == D1_AIS_FLIN) && (ailP->nextPrimaryFire < 0)) {
		aiP->GOAL_STATE = D1_AIS_FIRE;
	}

	gameData.ai.vBelievedTargetPos = gameData.ai.cloakInfo [nObject & (D1_MAX_AI_CLOAK_INFO-1)].vLastPos;

	if (!((aiP->behavior >= MIN_BEHAVIOR) && (aiP->behavior <= MAX_BEHAVIOR))) {
		aiP->behavior = D1_AIB_NORMAL;
	}

	Assert(objP->info.nSegment != -1);
	Assert(objP->info.nId < gameData.bots.nTypes [1]);

	botInfoP = &gameData.bots.info [1][objP->info.nId];
	Assert(botInfoP->always_0xabcd == 0xabcd);
	obj_ref = nObject ^ gameData.app.nFrameCount;
	// -- if (ailP->wait_time > -I2X (8))
	// -- 	ailP->wait_time -= gameData.time.xFrame;
	if (ailP->nextPrimaryFire > -I2X (8))
		ailP->nextPrimaryFire -= gameData.time.xFrame;
	if (ailP->timeSinceProcessed < I2X (256))
		ailP->timeSinceProcessed += gameData.time.xFrame;
	nPrevVisibility = ailP->nPrevVisibility;	//	Must get this before we toast the master copy!

	//	Deal with cloaking for robots which are cloaked except just before firing.
	if (botInfoP->cloakType == RI_CLOAKED_EXCEPT_FIRING) {
		if (ailP->nextPrimaryFire < I2X (1)/2)
			aiP->CLOAKED = 1;
		else
			aiP->CLOAKED = 0;
		}
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
		gameData.ai.vBelievedTargetPos = OBJPOS (gameData.objs.consoleP)->vPos;

	dist_to_player = CFixVector::Dist(gameData.ai.vBelievedTargetPos, objP->info.position.vPos);
	//	If this robotP can fire, compute visibility from gun position.
	//	Don't want to compute visibility twice, as it is expensive.  (So is call to calc_vGunPoint).
	if ((ailP->nextPrimaryFire <= 0) && (dist_to_player < I2X (200)) && (botInfoP->nGuns) && !(botInfoP->attackType)) {
		CalcGunPoint (&vGunPoint, objP, aiP->CURRENT_GUN);
		vVisVecPos = vGunPoint;
		}
	else {
		vVisVecPos = objP->info.position.vPos;
		vGunPoint.SetZero ();
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Occasionally make non-still robots make a path to the playerP.  Based on agitation and distance from playerP.
	if ((aiP->behavior != D1_AIB_RUN_FROM) && (aiP->behavior != D1_AIB_STILL) && !(IsMultiGame))
		if (gameData.ai.nOverallAgitation > 70) {
			if ((dist_to_player < I2X (200)) && (d_rand() < gameData.time.xFrame/4)) {
				if (d_rand() * (gameData.ai.nOverallAgitation - 40) > I2X (5)) {
					CreatePathToPlayer(objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
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
	if ((ailP->nRetryCount) && !(IsMultiGame)) {
		ailP->nConsecutiveRetries += ailP->nRetryCount;
		ailP->nRetryCount = 0;
		if (ailP->nConsecutiveRetries > 3) {
			switch (ailP->mode) {
				case D1_AIM_CHASE_OBJECT:
					CreatePathToPlayer(objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					break;
				case D1_AIM_STILL:
					if (!((aiP->behavior == D1_AIB_STILL) || (aiP->behavior == D1_AIB_STATION)))	//	Behavior is still, so don't follow path.
						AttemptToResumePath (objP);
					break;
				case D1_AIM_FOLLOW_PATH:
					if (IsMultiGame)
						ailP->mode = D1_AIM_STILL;
					else
						AttemptToResumePath(objP);
					break;
				case D1_AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(objP);
					objP->mType.physInfo.velocity [X] = 0;
					objP->mType.physInfo.velocity [Y] = 0;
					objP->mType.physInfo.velocity [Z] = 0;
					CreateNSegmentPath(objP, 5, -1);
					ailP->mode = D1_AIM_RUN_FROM_OBJECT;
					break;
				case D1_AIM_HIDE:
					move_towards_segment_center(objP);
					objP->mType.physInfo.velocity [X] = 0;
					objP->mType.physInfo.velocity [Y] = 0;
					objP->mType.physInfo.velocity [Z] = 0;
					if (gameData.ai.nOverallAgitation > (50 - gameStates.app.nDifficultyLevel*4))
						CreatePathToPlayer(objP, 4 + gameData.ai.nOverallAgitation/8, 1);
					else {
						CreateNSegmentPath(objP, 5, -1);
					}
					break;
				case D1_AIM_OPEN_DOOR:
					CreateNSegmentPathToDoor(objP, 5, -1);
					break;
				#if DBG
				case D1_AIM_FOLLOW_PATH_2:
					Int3();	//	Should never happen!
					break;
				#endif
			}
			ailP->nConsecutiveRetries = 0;
		}
	} else
		ailP->nConsecutiveRetries /= 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
 	//	If in materialization center, exit
 	if (!(IsMultiGame) && (SEGMENTS [objP->info.nSegment].m_nType == SEGMENT_IS_ROBOTMAKER)) {
 		AIFollowPath (objP, 1, 1, NULL);		// 1 = playerP is visible, which might be a lie, but it works.
 		return;
 	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease playerP awareness due to the passage of time.
	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease playerP awareness due to the passage of time.
	if (ailP->targetAwarenessType) {
		if (ailP->targetAwarenessTime > 0) {
			ailP->targetAwarenessTime -= gameData.time.xFrame;
			if (ailP->targetAwarenessTime <= 0) {
 				ailP->targetAwarenessTime = I2X (2);	//new: 11/05/94
 				ailP->targetAwarenessType--;	//new: 11/05/94
				}
			}
		else {
			ailP->targetAwarenessType--;
			ailP->targetAwarenessTime = I2X (2);
			// aiP->GOAL_STATE = D1_AIS_REST;
			}
		}
	else
		aiP->GOAL_STATE = D1_AIS_REST;							//new: 12/13/94


	if (gameStates.app.bPlayerIsDead && (ailP->targetAwarenessType == 0))
		if ((dist_to_player < I2X (200)) && (d_rand() < gameData.time.xFrame/8)) {
			if ((aiP->behavior != D1_AIB_STILL) && (aiP->behavior != D1_AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(objP, 30))
					return;
				ai_multi_send_robot_position(nObject, -1);

				if (!((ailP->mode == D1_AIM_FOLLOW_PATH) && (aiP->nCurPathIndex < aiP->nPathLength-1))) {
					if (dist_to_player < I2X (30))
						CreateNSegmentPath(objP, 5, 1);
					else
						CreatePathToPlayer(objP, 20, 1);
					}
			}
		}

	//	Make sure that if this guy got hit or bumped, then he's chasing playerP.
	if ((ailP->targetAwarenessType == D1_PA_WEAPON_ROBOT_COLLISION) || (ailP->targetAwarenessType >= D1_PA_PLAYER_COLLISION)) {
		if ((aiP->behavior != D1_AIB_STILL) && (aiP->behavior != D1_AIB_FOLLOW_PATH) && (aiP->behavior != D1_AIB_RUN_FROM) && (objP->info.nId != ROBOT_BRAIN))
			ailP->mode = D1_AIM_CHASE_OBJECT;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((aiP->GOAL_STATE == D1_AIS_FLIN) && (aiP->CURRENT_STATE == D1_AIS_FLIN))
		aiP->GOAL_STATE = D1_AIS_LOCK;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Note: Should only do these two function calls for objects which animate
	if ((dist_to_player < I2X (100))) { // && !(IsMultiGame)) {
		object_animates = do_silly_animation(objP);
		if (object_animates)
			ai_frame_animation(objP);
		}
	else {
		//	If Object is supposed to animate, but we don't let it animate due to distance, then
		//	we must change its state, else it will never update.
		aiP->CURRENT_STATE = aiP->GOAL_STATE;
		object_animates = 0;		//	If we're not doing the animation, then should pretend it doesn't animate.
		}

	switch (gameData.bots.info [1][objP->info.nId].bossFlag) {
		case 0:
			break;
		case 1:
			if (aiP->GOAL_STATE == D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_FIRE;
			if (aiP->CURRENT_STATE == D1_AIS_FLIN)
				aiP->CURRENT_STATE = D1_AIS_FIRE;
			dist_to_player /= 4;

			do_boss_stuff(objP);
			dist_to_player *= 4;
			break;
#ifndef SHAREWARE
		case 2:
			if (aiP->GOAL_STATE == D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_FIRE;
			if (aiP->CURRENT_STATE == D1_AIS_FLIN)
				aiP->CURRENT_STATE = D1_AIS_FIRE;
			compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

		 {	int pv = player_visibility;
				fix	dtp = dist_to_player/4;

			//	If playerP cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = CFixVector::Dist(OBJPOS (gameData.objs.consoleP)->vPos, objP->info.position.vPos)/4;
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
	//	Guys whose behavior is station and are not at their hide CSegment get processed anyway.
	if (ailP->targetAwarenessType < D1_PA_WEAPON_ROBOT_COLLISION-1) { // If robotP got hit, he gets to attack playerP always!
		#if DBG
		if (Break_on_object != nObject) {	//	don't time slice if we're interested in this CObject.
		#endif
			if ((dist_to_player > I2X (250)) && (ailP->timeSinceProcessed <= I2X (2)))
				return;
			else if ((aiP->behavior != D1_AIB_STATION) || (ailP->mode != D1_AIM_FOLLOW_PATH) || (aiP->nHideSegment == objP->info.nSegment)) {
				if ((dist_to_player > I2X (150)) && (ailP->timeSinceProcessed <= I2X (1)))
					return;
				if ((dist_to_player > I2X (100)) && (ailP->timeSinceProcessed <= I2X (1)/2))
					return;
			}
		#if DBG
		}
		#endif
	}

	//	Reset time since processed, but skew objects so not everything processed synchronously, else
	//	we get fast frames with the occasional very slow frame.
	// D1_AI_proc_time = ailP->timeSinceProcessed;
	ailP->timeSinceProcessed = - ((nObject & 0x03) * gameData.time.xFrame ) / 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Perform special ability
	switch (objP->info.nId) {
		case ROBOT_BRAIN:
			//	Robots function nicely if behavior is Station.  This means they won't move until they
			//	can see the playerP, at which time they will start wandering about opening doors.
			if (gameData.objs.consoleP->info.nSegment == objP->info.nSegment) {
				if (!ai_multiplayer_awareness(objP, 97))
					return;
				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
				move_away_from_player(objP, &vec_to_player, 0);
				ai_multi_send_robot_position(nObject, -1);
			} else if (ailP->mode != D1_AIM_STILL) {
				int	r;

				r = objP->OpenableDoorsInSegment ();
				if (r != -1) {
					ailP->mode = D1_AIM_OPEN_DOOR;
					aiP->GOALSIDE = r;
				} else if (ailP->mode != D1_AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(objP, 50))
						return;
					CreateNSegmentPathToDoor(objP, 8+gameStates.app.nDifficultyLevel, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(nObject, -1);
				}
			} else {
				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(objP, 50))
						return;
					CreateNSegmentPathToDoor(objP, 8+gameStates.app.nDifficultyLevel, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(nObject, -1);
				}
			}
			break;
		default:
			break;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	switch (ailP->mode) {
		case D1_AIM_CHASE_OBJECT: {		// chasing playerP, sort of, chase if far, back off if close, circle in between
			fix	circle_distance;

			circle_distance = botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.consoleP->info.xSize;
			//	Green guy doesn't get his circle distance boosted, else he might never attack.
			if (botInfoP->attackType != 1)
				circle_distance += I2X (nObject&0xf) / 2;

			compute_vis_and_vec (objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

			//	@mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (nPrevVisibility == 2)) { // this is redundant: mk, 01/15/95: && (ailP->mode == D1_AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(objP, 53)) {
					if (maybe_ai_do_actual_firing_stuff(objP, aiP))
						ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
					return;
				}
				CreatePathToPlayer(objP, 8, 1);
				// -- show_path_and_other(objP);
				ai_multi_send_robot_position(nObject, -1);
			} else if ((player_visibility == 0) && (dist_to_player > I2X (80)) && (!(IsMultiGame))) {
				//	If pretty far from the playerP, playerP cannot be seen (obstructed) and in chase mode, switch to follow path mode.
				//	This has one desirable benefit of avoiding physics retries.
				if (aiP->behavior == D1_AIB_STATION) {
					ailP->nGoalSegment = aiP->nHideSegment;
					CreatePathToStation(objP, 15);
					// -- show_path_and_other(objP);
				} else
					CreateNSegmentPath(objP, 5, -1);
				break;
			}

			if ((aiP->CURRENT_STATE == D1_AIS_REST) && (aiP->GOAL_STATE == D1_AIS_REST)) {
				if (player_visibility) {
					if (d_rand() < gameData.time.xFrame*player_visibility) {
						if (dist_to_player/256 < d_rand()*player_visibility) {
							aiP->GOAL_STATE = D1_AIS_SRCH;
							aiP->CURRENT_STATE = D1_AIS_SRCH;
						}
					}
				}
			}

			if (gameData.time.xGame - ailP->timeTargetSeen > CHASE_TIME_LENGTH) {

				if (IsMultiGame)
					if (!player_visibility && (dist_to_player > I2X (70))) {
						ailP->mode = D1_AIM_STILL;
						return;
					}

				if (!ai_multiplayer_awareness(objP, 64)) {
					if (maybe_ai_do_actual_firing_stuff(objP, aiP))
						ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
					return;
				}
				CreatePathToPlayer(objP, 10, 1);
				// -- show_path_and_other(objP);
				ai_multi_send_robot_position(nObject, -1);
			} else if ((aiP->CURRENT_STATE != D1_AIS_REST) && (aiP->GOAL_STATE != D1_AIS_REST)) {
				if (!ai_multiplayer_awareness(objP, 70)) {
					if (maybe_ai_do_actual_firing_stuff(objP, aiP))
						ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
					return;
				}
				ai_move_relative_to_player(objP, ailP, dist_to_player, &vec_to_player, circle_distance, 0);

				if ((obj_ref & 1) && ((aiP->GOAL_STATE == D1_AIS_SRCH) || (aiP->GOAL_STATE == D1_AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
					else
						ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
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
			compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

			if (player_visibility) {
				if (ailP->targetAwarenessType == 0)
					ailP->targetAwarenessType = D1_PA_WEAPON_ROBOT_COLLISION;

			}

			//	If in multiplayer, only do if playerP visible.  If not multiplayer, do always.
			if (!(IsMultiGame) || player_visibility)
				if (ai_multiplayer_awareness(objP, 75)) {
					AIFollowPath(objP, player_visibility, nPrevVisibility, &vec_to_player);
					ai_multi_send_robot_position(nObject, -1);
				}

			if (aiP->GOAL_STATE != D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_LOCK;
			else if (aiP->CURRENT_STATE == D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_LOCK;

			//	Bad to let run_from robotP fire at playerP because it will cause a war in which it turns towards the
			//	playerP to fire and then towards its goal to move.
			// do_firing_stuff(objP, player_visibility, &vec_to_player);
			//	Instead, do this:
			//	(Note, only drop if playerP is visible.  This prevents the bombs from being a giveaway, and
			//	also ensures that the robotP is moving while it is dropping.  Also means fewer will be dropped.)
			if ((ailP->nextPrimaryFire <= 0) && (player_visibility)) {
				CFixVector	vFire, fire_pos;

				if (!ai_multiplayer_awareness(objP, 75))
					return;

				vFire = objP->info.position.mOrient.FVec ();
				vFire = -vFire;
				fire_pos = objP->info.position.vPos + vFire;

				CreateNewLaserEasy( &vFire, &fire_pos, objP->Index (), PROXMINE_ID, 1);
				ailP->nextPrimaryFire = I2X (5);		//	Drop a proximity bomb every 5 seconds.

				#ifdef NETWORK
				if (IsMultiGame)
			 {
					ai_multi_send_robot_position(objP->Index (), -1);
					MultiSendRobotFire(objP->Index (), -1, &vFire);
				}
				#endif
			}
			break;

		case D1_AIM_FOLLOW_PATH: {
			int	anger_level = 65;

			if (aiP->behavior == D1_AIB_STATION)
				if ((aiP->nHideIndex + aiP->nPathLength > 0) && (gameData.ai.routeSegs [aiP->nHideIndex + aiP->nPathLength - 1].nSegment == aiP->nHideSegment)) {
					anger_level = 64;
				}

			compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

			if (gameData.app.nGameMode & (GM_MODEM | GM_SERIAL))
				if (!player_visibility && (dist_to_player > I2X (70))) {
					ailP->mode = D1_AIM_STILL;
					return;
				}

			if (!ai_multiplayer_awareness(objP, anger_level)) {
				if (maybe_ai_do_actual_firing_stuff(objP, aiP)) {
					compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
					ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
				}
				return;
			}

			AIFollowPath(objP, player_visibility, nPrevVisibility, &vec_to_player);

			if (aiP->GOAL_STATE != D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_LOCK;
			else if (aiP->CURRENT_STATE == D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_LOCK;

			if ((aiP->behavior != D1_AIB_FOLLOW_PATH) && (aiP->behavior != D1_AIB_RUN_FROM))
				do_firing_stuff(objP, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (aiP->behavior != D1_AIB_FOLLOW_PATH) && (aiP->behavior != D1_AIB_RUN_FROM) && (objP->info.nId != ROBOT_BRAIN)) {
				if (botInfoP->attackType == 0)
					ailP->mode = D1_AIM_CHASE_OBJECT;
			} else if ((player_visibility == 0) && (aiP->behavior == D1_AIB_NORMAL) && (ailP->mode == D1_AIM_FOLLOW_PATH) && (aiP->behavior != D1_AIB_RUN_FROM)) {
				ailP->mode = D1_AIM_STILL;
				aiP->nHideIndex = -1;
				aiP->nPathLength = 0;
			}

			ai_multi_send_robot_position(nObject, -1);

			break;
		}

		case D1_AIM_HIDE:
			if (!ai_multiplayer_awareness(objP, 71)) {
				if (maybe_ai_do_actual_firing_stuff(objP, aiP)) {
					compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
					ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
				}
				return;
			}

			compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

 			AIFollowPath(objP, player_visibility, nPrevVisibility, &vec_to_player);

			if (aiP->GOAL_STATE != D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_LOCK;
			else if (aiP->CURRENT_STATE == D1_AIS_FLIN)
				aiP->GOAL_STATE = D1_AIS_LOCK;

			ai_multi_send_robot_position(nObject, -1);
			break;

		case D1_AIM_STILL:
			if ((dist_to_player < I2X (120) + gameStates.app.nDifficultyLevel * I2X (20)) || (ailP->targetAwarenessType >= D1_PA_WEAPON_ROBOT_COLLISION - 1)) {
				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

				//	turn towards vector if visible this time or last time, or d_rand
				// new!
				if ((player_visibility) || (nPrevVisibility) || ((d_rand() > 0x4000) && !(IsMultiGame))) {
					if (!ai_multiplayer_awareness(objP, 71)) {
						if (maybe_ai_do_actual_firing_stuff(objP, aiP))
							ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
						return;
					}
					ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(nObject, -1);
				}

				do_firing_stuff(objP, player_visibility, &vec_to_player);
				//	This is debugging code!  Remove it!  It's to make the green guy attack without doing other kinds of movement.
				if (player_visibility) {		//	Change, MK, 01/03/94 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (botInfoP->attackType == 1) {
						aiP->behavior = D1_AIB_NORMAL;
						if (!ai_multiplayer_awareness(objP, 80)) {
							if (maybe_ai_do_actual_firing_stuff(objP, aiP))
								ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
							return;
						}
						ai_move_relative_to_player(objP, ailP, dist_to_player, &vec_to_player, 0, 0);
						if (D1_AI_evaded) {
							ai_multi_send_robot_position(nObject, 1);
							D1_AI_evaded = 0;
						}
						else
							ai_multi_send_robot_position(nObject, -1);
					} else {
						//	Robots in hover mode are allowed to evade at half Normal speed.
						if (!ai_multiplayer_awareness(objP, 81)) {
							if (maybe_ai_do_actual_firing_stuff(objP, aiP))
								ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
							return;
						}
						ai_move_relative_to_player(objP, ailP, dist_to_player, &vec_to_player, 0, 1);
						if (D1_AI_evaded) {
							ai_multi_send_robot_position(nObject, -1);
							D1_AI_evaded = 0;
						}
						else
							ai_multi_send_robot_position(nObject, -1);
					}
				} else if ((objP->info.nSegment != aiP->nHideSegment) && (dist_to_player > I2X (80)) && (!(IsMultiGame))) {
					//	If pretty far from the playerP, playerP cannot be seen (obstructed) and in chase mode, switch to follow path mode.
					//	This has one desirable benefit of avoiding physics retries.
					if (aiP->behavior == D1_AIB_STATION) {
						ailP->nGoalSegment = aiP->nHideSegment;
						CreatePathToStation(objP, 15);
						// -- show_path_and_other(objP);
					}
					break;
				}
			}

			break;
		case D1_AIM_OPEN_DOOR: {		// trying to open a door.
			CFixVector	vCenter, vGoal;
			Assert(objP->info.nId == ROBOT_BRAIN);		//	Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(objP, 62))
				return;
			vCenter = SEGMENTS [objP->info.nSegment].SideCenter (aiP->GOALSIDE);
			vGoal = vCenter - objP->info.position.vPos;
			CFixVector::Normalize (vGoal);
			ai_turn_towards_vector(&vGoal, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
			move_towards_vector(objP, &vGoal);
			ai_multi_send_robot_position(nObject, -1);

			break;
		}

		default:
			ailP->mode = D1_AIM_CHASE_OBJECT;
			break;
	}		// end:	switch (ailP->mode) {

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If the robotP can see you, increase his awareness of you.
	//	This prevents the problem of a robotP looking right at you but doing nothing.
	// Assert(player_visibility != -1);	//	Means it didn't get initialized!
	compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
	if (player_visibility == 2)
		if (ailP->targetAwarenessType == 0)
			ailP->targetAwarenessType = D1_PA_PLAYER_COLLISION;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (!object_animates) {
		aiP->CURRENT_STATE = aiP->GOAL_STATE;
	}

	Assert(ailP->targetAwarenessType <= D1_AIE_MAX);
	Assert(aiP->CURRENT_STATE < D1_AIS_MAX);
	Assert(aiP->GOAL_STATE < D1_AIS_MAX);

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (ailP->targetAwarenessType) {
		new_goalState = D1_AI_transition_table [ailP->targetAwarenessType-1][aiP->CURRENT_STATE][aiP->GOAL_STATE];
		if (ailP->targetAwarenessType == D1_PA_WEAPON_ROBOT_COLLISION) {
			//	Decrease awareness, else this robotP will flinch every frame.
			ailP->targetAwarenessType--;
			ailP->targetAwarenessTime = I2X (3);
		}

		if (new_goalState == D1_AIS_ERR_)
			new_goalState = D1_AIS_REST;

		if (aiP->CURRENT_STATE == D1_AIS_NONE)
			aiP->CURRENT_STATE = D1_AIS_REST;

		aiP->GOAL_STATE = new_goalState;

	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If new state = fire, then set all gun states to fire.
	if ((aiP->GOAL_STATE == D1_AIS_FIRE) ) {
		int	i,nGunCount;
		nGunCount = gameData.bots.info [1][objP->info.nId].nGuns;
		for (i=0; i<nGunCount; i++)
			ailP->goalState [i] = D1_AIS_FIRE;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his nextPrimaryFire says ok to fire, bash him there
	if ((ailP->nextPrimaryFire < 0) && (aiP->GOAL_STATE == D1_AIS_FIRE))
		aiP->CURRENT_STATE = D1_AIS_FIRE;

	if ((aiP->GOAL_STATE != D1_AIS_FLIN)  && (objP->info.nId != ROBOT_BRAIN)) {
		switch (aiP->CURRENT_STATE) {
			case	D1_AIS_NONE:
				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

				dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), vec_to_player);
				if (dot >= I2X (1)/2)
					if (aiP->GOAL_STATE == D1_AIS_REST)
						aiP->GOAL_STATE = D1_AIS_SRCH;
				break;
			case	D1_AIS_REST:
				if (aiP->GOAL_STATE == D1_AIS_REST) {
					compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
					if ((ailP->nextPrimaryFire <= 0) && (player_visibility)) {
						aiP->GOAL_STATE = D1_AIS_FIRE;
					}
				}
				break;
			case	D1_AIS_SRCH:
				if (!ai_multiplayer_awareness(objP, 60))
					return;

				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

				if (player_visibility) {
					ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(nObject, -1);
				} else if (!(IsMultiGame))
					ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
				break;
			case	D1_AIS_LOCK:
				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

				if (!(IsMultiGame) || (player_visibility)) {
					if (!ai_multiplayer_awareness(objP, 68))
						return;

					if (player_visibility) {
						ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
						ai_multi_send_robot_position(nObject, -1);
					} else if (!(IsMultiGame))
						ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
				}
				break;
			case	D1_AIS_FIRE:
				compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);

				if (player_visibility) {
					if (!ai_multiplayer_awareness(objP, (ROBOT_FIRE_AGITATION-1)))
				 {
						if (IsMultiGame) {
							ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
							return;
						}
					}
					ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position(nObject, -1);
				} else if (!(IsMultiGame)) {
					ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
				}

				//	Fire at playerP, if appropriate.
				ai_do_actual_firing_stuff(objP, aiP, ailP, botInfoP, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);

				break;
			case	D1_AIS_RECO:
				if (!(obj_ref & 3)) {
					compute_vis_and_vec(objP, &vVisVecPos, ailP, &vec_to_player, &player_visibility, botInfoP, &bVisAndVecComputed);
					if (player_visibility) {
						if (!ai_multiplayer_awareness(objP, 69))
							return;
						ai_turn_towards_vector(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
						ai_multi_send_robot_position(nObject, -1);
					} else if (!(IsMultiGame)) {
						ai_turn_randomly(&vec_to_player, objP, botInfoP->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
					}
				}
				break;
			case	D1_AIS_FLIN:
				break;
			default:
				aiP->GOAL_STATE = D1_AIS_REST;
				aiP->CURRENT_STATE = D1_AIS_REST;
				break;
		}
	} // end of: if (aiP->GOAL_STATE != D1_AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		aiP->CURRENT_GUN++;
		if (aiP->CURRENT_GUN >= gameData.bots.info [1][objP->info.nId].nGuns)
			aiP->CURRENT_GUN = 0;
	}

}

//--mk, 121094 -- // ----------------------------------------------------------------------------------
//--mk, 121094 -- void spin_robot(CObject *robotP, CFixVector *vCollision)
//--mk, 121094 -- {
//--mk, 121094 -- 	if (vCollision->p.x != 3) {
//--mk, 121094 -- 		robotP->physInfo.rotVel [X] = 0x1235;
//--mk, 121094 -- 		robotP->physInfo.rotVel [Y] = 0x2336;
//--mk, 121094 -- 		robotP->physInfo.rotVel [Z] = 0x3737;
//--mk, 121094 -- 	}
//--mk, 121094 --
//--mk, 121094 -- }

//	-----------------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	int	i;

	for (i=0; i<D1_MAX_AI_CLOAK_INFO; i++) {
		gameData.ai.cloakInfo [i].vLastPos = OBJPOS (gameData.objs.consoleP)->vPos;
		gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	}

	//	Make work for control centers.
	gameData.ai.vBelievedTargetPos = gameData.ai.cloakInfo [0].vLastPos;

}

//	-----------------------------------------------------------------------------------
//	Returns false if awareness is considered too puny to add, else returns true.
int add_awareness_event(CObject *objP, int type)
{
	//	If playerP cloaked and hit a robotP, then increase awareness
	if ((type == D1_PA_WEAPON_ROBOT_COLLISION) || (type == D1_PA_WEAPON_WALL_COLLISION) || (type == D1_PA_PLAYER_COLLISION))
		ai_do_cloak_stuff();

	if (gameData.ai.nAwarenessEvents < D1_MAX_AWARENESS_EVENTS) {
		if ((type == D1_PA_WEAPON_WALL_COLLISION) || (type == D1_PA_WEAPON_ROBOT_COLLISION))
			if (objP->info.nId == VULCAN_ID)
				if (d_rand() > 3276)
					return 0;		//	For vulcan cannon, only about 1/10 actually cause awareness

		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nSegment = objP->info.nSegment;
		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].pos = objP->info.position.vPos;
		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nType = type;
		gameData.ai.nAwarenessEvents++;
	} else
		Assert(0);		// Hey -- Overflowed gameData.ai.awarenessEvents, make more or something
							// This just gets ignored, so you can just continue.
	return 1;

}

#if DBG
int	D1_AI_dump_enable = 0;

FILE *D1_AI_dump_file = NULL;

char	D1_AI_error_message [128] = "";

// ----------------------------------------------------------------------------------
void dump_ai_objects_all()
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

	if (D1_AI_error_message [0])
		fprintf(D1_AI_dump_file, "Error message: %s\n", D1_AI_error_message);

	for (nObject=0; nObject <= gameData.objs.nLastObject; nObject++) {
		CObject		*objP = &OBJECTS [nObject];
		tAIStaticInfo	*aiP = &objP->cType.aiInfo;
		tAILocalInfo		*ailP = &gameData.ai.localInfo [nObject];
		fix			dist_to_player;

		dist_to_player = vm_vec_dist(&objP->info.position.vPos, &OBJPOS (gameData.objs.consoleP)->vPos);

		if (objP->info.controlType == CT_AI) {
			fprintf(D1_AI_dump_file, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n",
				nObject, objP->info.nSegment, X2F(dist_to_player), mode_text [ailP->mode], behavior_text [aiP->behavior-0x80], aiP->nHideIndex, aiP->nPathLength);
			if (aiP->nPathLength)
				total += aiP->nPathLength;
		}
	}

	fprintf(D1_AI_dump_file, "Total path length = %4i\n", total);
#endif

}

// ----------------------------------------------------------------------------------
void force_dump_ai_objects_all(char *msg)
{
	int	tsave;

	tsave = D1_AI_dump_enable;

	D1_AI_dump_enable = 1;

	sprintf(D1_AI_error_message, "%s\n", msg);
	dump_ai_objects_all();
	D1_AI_error_message [0] = 0;

	D1_AI_dump_enable = tsave;
}

#endif

// ----------------------------------------------------------------------------------
//eof
