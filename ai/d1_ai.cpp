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
#include "fireweapon.h"
#include "collision_math.h"
#include "polymodel.h"
#include "loadgamedata.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "producers.h"
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
#include "segmath.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

#define	JOHN_CHEATS_SIZE_1	6
#define	JOHN_CHEATS_SIZE_2	6
#define	JOHN_CHEATS_SIZE_3	6

void pae_aux (int32_t nSegment, int32_t nType, int32_t level);

uint8_t	john_cheats_1 [JOHN_CHEATS_SIZE_1] = { 	KEY_P ^ 0x00 ^ 0x34,
															KEY_O ^ 0x10 ^ 0x34,
															KEY_B ^ 0x20 ^ 0x34,
															KEY_O ^ 0x30 ^ 0x34,
															KEY_Y ^ 0x40 ^ 0x34,
															KEY_S ^ 0x50 ^ 0x34 };

#define	PARALLAX	0		//	If !0, then special debugging info for Parallax eyes only enabled.

#define MIN_D 0x100

int32_t	nFlinchScaleD1 = 4;
int32_t	nAttackScaleD1 = 24;

#ifndef ANIM_RATE
#	define	ANIM_RATE		(I2X (1)/16)
#endif

#define	DELTA_ANG_SCALE	16

static uint8_t xlatD1Animation [] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

int32_t	john_cheats_index_1;		//	POBOYS		detonate reactor
int32_t	john_cheats_index_2;		//	PORGYS		high speed weapon firing
int32_t	john_cheats_index_3;		//	LUNACY		lunacy (insane behavior, rookie firing)
int32_t	john_cheats_index_4;		//	PLETCHnnn	paint robots

extern int32_t nRobotSoundVolume;

// int32_t	No_ai_flag=0;

#define	OVERALL_AGITATION_MAX	100

#define		D1_MAX_AI_CLOAK_INFO	8	//	Must be a power of 2!

#ifndef BOSS_CLOAK_DURATION
#	define	BOSS_CLOAK_DURATION	(I2X (7))
#endif
#ifndef BOSS_DEATH_DURATION
#	define	BOSS_DEATH_DURATION	(I2X (6))
#endif
#define	BOSS_DEATH_SOUND_DURATION	0x2ae14		//	2.68 seconds

//	Amount of time since the current pRobot was last processed for things such as movement.
//	It is not valid to use gameData.time.xFrame because robots do not get moved every frame.
//fix	D1_AI_proc_time;

//	---------- John: End of variables which must be saved as part of gamesave. ----------

int32_t	D1_AI_evaded=0;

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
//	18	cloak-green	; note, gating in this guy benefits pPlayer, cloak objects
//	19	vulcan
//	20	toad
//	21	4-claw
//	22	quad-laser
// 23 super boss

// uint8_t	super_boss_gate_list [] = {0, 1, 2, 9, 11, 16, 18, 19, 21, 22, 0, 9, 9, 16, 16, 18, 19, 19, 22, 22};
uint8_t	super_boss_gate_list [] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};
#define	D1_MAX_GATE_INDEX	( sizeof(super_boss_gate_list) / sizeof(super_boss_gate_list [0]) )

int32_t	D1_AI_info_enabled=0;

int32_t	Ugly_robot_cheat = 0, Ugly_robot_texture = 0;
uint8_t	Enable_john_cheat_1 = 0, Enable_john_cheat_2 = 0, Enable_john_cheat_3 = 0, Enable_john_cheat_4 = 0;

uint8_t	john_cheats_3 [2*JOHN_CHEATS_SIZE_3+1] = { KEY_Y ^ 0x67,
																KEY_E ^ 0x66,
																KEY_C ^ 0x65,
																KEY_A ^ 0x64,
																KEY_N ^ 0x63,
																KEY_U ^ 0x62,
																KEY_L ^ 0x61 };


#define	D1_MAX_AWARENESS_EVENTS	64
typedef struct awareness_event {
	int16_t 		nSegment;				// CSegment the event occurred in
	int16_t			type;					// type of event, defines behavior
	CFixVector	pos;					// absolute 3 space location of event
} awareness_event;


//	These globals are set by a call to FindHitpoint, which is a slow routine,
//	so we don't want to call it again (for this CObject) unless we have to.

#define	D1_AIS_MAX	8
#define	D1_AIE_MAX	4

//--unused-- int32_t	Processed_this_frame, LastFrameCount;
#if DBG
//	Index into this array with pLocalInfo->mode
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

//	Index into this array with pStaticInfo->behavior
char	behavior_text [6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

//	Index into this array with pStaticInfo->GOAL_STATE or pStaticInfo->CURRENT_STATE
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


int32_t	D1_AI_animation_test=0;
#endif

// Current state indicates where the pRobot current is, or has just done.
//	Transition table between states for an AI CObject.
//	 First dimension is trigger event.
//	Second dimension is current state.
//	 Third dimension is goal state.
//	Result is new goal state.
//	ERR_ means something impossible has happened.
uint8_t D1_AI_transition_table [D1_AI_MAX_EVENT][D1_AI_MAX_STATE][D1_AI_MAX_STATE] = {
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

	//	Event = AIE_COLL, pPlayer collided with pRobot
 {
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_FLIN,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FLIN},
 {	D1_AIS_ERR_,	D1_AIS_REST,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FIRE,	D1_AIS_RECO},
 {	D1_AIS_ERR_,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_LOCK,	D1_AIS_FLIN,	D1_AIS_FIRE,	D1_AIS_FIRE}
	},

	//	Event = AIE_HURT, pPlayer hurt pRobot (by firing at and hitting it)
	//	Note, this doesn't necessarily mean the pRobot JUST got hit, only that that is the most recent thing that happened.
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

uint8_t	john_cheats_2 [2*JOHN_CHEATS_SIZE_2] = { 	KEY_P ^ 0x00 ^ 0x43, 0x66,
																KEY_O ^ 0x10 ^ 0x43, 0x11,
																KEY_R ^ 0x20 ^ 0x43, 0x8,
																KEY_G ^ 0x30 ^ 0x43, 0x2,
																KEY_Y ^ 0x40 ^ 0x43, 0x0,
																KEY_S ^ 0x50 ^ 0x43 };

static CHitResult aiHitResult;

// ---------------------------------------------------------
//	On entry, gameData.botData.nTypes [1] had darn sure better be set.
//	Mallocs gameData.botData.nTypes [1] tRobotInfo structs into global tRobotInfo.
void john_cheat_func_1(int32_t key)
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
int32_t D1_AI_behavior_to_mode(int32_t behavior)
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

int32_t	Lunacy = 0;
int32_t	Diff_save = 1;

fix	primaryFiringWaitCopy [MAX_ROBOT_TYPES];
uint8_t	nRapidFireCountCopy [MAX_ROBOT_TYPES];

void do_lunacy_on(void)
{
	int32_t	i;

	if ( !Lunacy ) {
		Lunacy = 1;
		Diff_save = gameStates.app.nDifficultyLevel;
		gameStates.app.nDifficultyLevel = DIFFICULTY_LEVEL_COUNT-1;

		for (i=0; i<MAX_ROBOT_TYPES; i++) {
			primaryFiringWaitCopy [i] = gameData.botData.info [1][i].primaryFiringWait [DIFFICULTY_LEVEL_COUNT-1];
			nRapidFireCountCopy [i] = gameData.botData.info [1][i].nRapidFireCount [DIFFICULTY_LEVEL_COUNT-1];

			gameData.botData.info [1][i].primaryFiringWait [DIFFICULTY_LEVEL_COUNT-1] = gameData.botData.info [1][i].primaryFiringWait [1];
			gameData.botData.info [1][i].nRapidFireCount [DIFFICULTY_LEVEL_COUNT-1] = gameData.botData.info [1][i].nRapidFireCount [1];
		}
	}
}

void do_lunacy_off(void)
{
	int32_t	i;

	if ( Lunacy ) {
		Lunacy = 0;
		for (i=0; i<MAX_ROBOT_TYPES; i++) {
			gameData.botData.info [1][i].primaryFiringWait [DIFFICULTY_LEVEL_COUNT-1] = primaryFiringWaitCopy [i];
			gameData.botData.info [1][i].nRapidFireCount [DIFFICULTY_LEVEL_COUNT-1] = nRapidFireCountCopy [i];
		}
		gameStates.app.nDifficultyLevel = Diff_save;
	}
}

void john_cheat_func_3(int32_t key)
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
//--debug-- int32_t	Total_turns=0;
//--debug-- int32_t	Prevented_turns=0;
//--debug-- #endif

#define	D1_AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector (CFixVector *vGoal, CObject *pObj, fix rate)
{
	CFixVector	new_fVec;
	fix			dot;

	if ((pObj->info.nId == BABY_SPIDER_ID) && (pObj->info.nType == OBJ_ROBOT)) {
		pObj->TurnTowardsVector(*vGoal, rate);
		return;
	}

	new_fVec = *vGoal;

	dot = CFixVector::Dot (*vGoal, pObj->info.position.mOrient.m.dir.f);

	if (dot < (I2X (1) - gameData.time.xFrame / 2)) {
		fix	mag;
		fix	new_scale = FixDiv(gameData.time.xFrame * D1_AI_TURN_SCALE, rate);
		new_fVec *= new_scale;
		new_fVec += pObj->info.position.mOrient.m.dir.f;
		mag = CFixVector::Normalize (new_fVec);
		if (mag < I2X (1)/256) {
			new_fVec = *vGoal;		//	if degenerate vector, go right to goal
		}
	}
pObj->info.position.mOrient = CFixMatrix::CreateFR (new_fVec, pObj->info.position.mOrient.m.dir.r);
}

// --------------------------------------------------------------------------------------------------------------------
void ai_turn_randomly(CFixVector *vec_to_player, CObject *pObj, fix rate, int32_t nPrevVisibility)
{
	CFixVector	curVec;

	//	Random turning looks too stupid, so 1/4 of time, cheat.
	if (nPrevVisibility)
		if (RandShort () > 0x7400) {
			ai_turn_towards_vector (vec_to_player, pObj, rate);
			return;
		}
//--debug-- 	if (RandShort () > 0x6000)
//--debug-- 		Prevented_turns++;

	curVec = pObj->mType.physInfo.rotVel;

	curVec.v.coord.y += I2X (1)/64;

	curVec.v.coord.x += curVec.v.coord.y/6;
	curVec.v.coord.y += curVec.v.coord.z/4;
	curVec.v.coord.z += curVec.v.coord.x/10;

	if (abs(curVec.v.coord.x) > I2X (1)/8) curVec.v.coord.x /= 4;
	if (abs(curVec.v.coord.y) > I2X (1)/8) curVec.v.coord.y /= 4;
	if (abs(curVec.v.coord.z) > I2X (1)/8) curVec.v.coord.z /= 4;

	pObj->mType.physInfo.rotVel = curVec;

}

//	gameData.ai.nOverallAgitation affects:
//		Widens field of view.  Field of view is in range 0..1 (specified in bitmaps.tbl as N/360 degrees).
//			gameData.ai.nOverallAgitation/128 subtracted from field of view, making robots see wider.
//		Increases distance to which pRobot will search to create path to pPlayer by gameData.ai.nOverallAgitation/8 segments.
//		Decreases wait between fire times by gameData.ai.nOverallAgitation/64 seconds.

void john_cheat_func_4(int32_t key)
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
//	Note: Uses gameData.ai.target.vBelievedPos as pPlayer's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int32_t player_is_visible_from_object(CObject *pObj, CFixVector *pos, fix fieldOfView, CFixVector *vec_to_player)
{
	fix			dot;

	CHitQuery	hitQuery (FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_CHECK_PLAYER | FQ_VISIBILITY,
								 pos, &gameData.ai.target.vBelievedPos,
								 -1, pObj->Index (), 0, I2X (1) / 4, ++gameData.physics.bIgnoreObjFlag);

if ((*pos) == pObj->info.position.vPos)
	hitQuery.nSegment	= pObj->info.nSegment;
else {
	int32_t nSegment = FindSegByPos (*pos, pObj->info.nSegment, 1, 0);
	if (nSegment != -1)
		hitQuery.nSegment = nSegment;
	else {
		hitQuery.nSegment = pObj->info.nSegment;
		*pos = pObj->info.position.vPos;
		move_towards_segment_center (pObj);
		}
	}
#if DBG
if (hitQuery.nSegment == nDbgSeg)
	BRP;
#endif
#if DBG
if (hitQuery.nObject == nDbgObj)
	BRP;
#endif
CHitResult hitResult;
FindHitpoint (hitQuery, aiHitResult);
if (/*(hitType == HIT_NONE) ||*/ ((aiHitResult.nType == HIT_OBJECT) && (aiHitResult.nObject == LOCALPLAYER.nObject))) {
	dot = CFixVector::Dot (*vec_to_player, pObj->info.position.mOrient.m.dir.f);
	return (dot > fieldOfView - (gameData.ai.nOverallAgitation << 9)) ? 2 : 1;
	}
return 0;
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
int32_t do_silly_animation (CObject *pObj)
{
	int32_t				nObject = pObj->Index ();
	tJointPos*		jointPositions;
	int32_t				robotType, nGun, robotState, nJointPositions;
	tPolyObjInfo*	polyObjInfo = &pObj->rType.polyObjInfo;
	tAIStaticInfo*	pStaticInfo = &pObj->cType.aiInfo;
	int32_t				nGunCount, at_goal;
	int32_t				attackType;
	int32_t				nFlinchAttackScale = 1;

robotType = pObj->info.nId;
if (0 > (nGunCount = gameData.botData.info [1][robotType].nGuns))
	return 0;
attackType = gameData.botData.info [1][robotType].attackType;
robotState = xlatD1Animation [pStaticInfo->GOAL_STATE];
if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
	nFlinchAttackScale = nAttackScaleD1;
else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
	nFlinchAttackScale = nFlinchScaleD1;

at_goal = 1;
for (nGun = 0; nGun <= nGunCount; nGun++) {
	nJointPositions = RobotGetAnimState (&jointPositions, robotType, nGun, robotState);
	for (int32_t nJoint = 0; nJoint < nJointPositions; nJoint++) {
		int32_t				jointnum = jointPositions [nJoint].jointnum;

		if (jointnum >= gameData.models.polyModels [0][pObj->ModelId ()].ModelCount ())
			continue;

		CAngleVector*	jointAngles = &jointPositions [nJoint].angles;
		CAngleVector*	objAngles = &polyObjInfo->animAngles [jointnum];

		for (int32_t nAngle = 0; nAngle < 3; nAngle++) {
			if (jointAngles->v.vec [nAngle] != objAngles->v.vec [nAngle]) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].v.vec [nAngle] = jointAngles->v.vec [nAngle];
				fix delta2, deltaAngle = jointAngles->v.vec [nAngle] - objAngles->v.vec [nAngle];
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
				gameData.ai.localInfo [nObject].deltaAngles [jointnum].v.vec [nAngle] = delta2 / DELTA_ANG_SCALE;		// complete revolutions per second
				}
			}
		}

	if (at_goal) {
		tAILocalInfo* pLocalInfo = &gameData.ai.localInfo [pObj->Index ()];
		pLocalInfo->achievedState [nGun] = pLocalInfo->goalState [nGun];
		if (pLocalInfo->achievedState [nGun] == D1_AIS_RECO)
			pLocalInfo->goalState [nGun] = D1_AIS_FIRE;
		if (pLocalInfo->achievedState [nGun] == D1_AIS_FLIN)
			pLocalInfo->goalState [nGun] = D1_AIS_LOCK;
		}
	}
if (at_goal == 1) //nGunCount)
	pStaticInfo->CURRENT_STATE = pStaticInfo->GOAL_STATE;
return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-objects in an CObject towards their goals.
//	Current orientation of CObject is at:	polyObjInfo.anim_angles
//	Goal orientation of CObject is at:		aiInfo.goalAngles
//	Delta orientation of CObject is at:		aiInfo.deltaAngles
void ai_frame_animation (CObject *pObj)
{
	int32_t	nObject = pObj->Index ();
	int32_t	nJointCount;

nJointCount = gameData.models.polyModels [0][pObj->ModelId ()].ModelCount ();

for (int32_t joint = 1; joint < nJointCount; joint++) {
	fix			delta_to_goal;
	fix			scaled_delta_angle;
	CAngleVector	*pCurAngle = &pObj->rType.polyObjInfo.animAngles [joint];
	CAngleVector	*pGoalAngle = &gameData.ai.localInfo [nObject].goalAngles [joint];
	CAngleVector	*pDeltaAngle = &gameData.ai.localInfo [nObject].deltaAngles [joint];

	for (int32_t nAngle = 0; nAngle < 3; nAngle++) {
		delta_to_goal = pGoalAngle->v.vec [nAngle] - pCurAngle->v.vec [nAngle];
		if (delta_to_goal > 32767)
			delta_to_goal -= 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal += 65536;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (pDeltaAngle->v.vec [nAngle], gameData.time.xFrame) * DELTA_ANG_SCALE;
			pCurAngle->v.vec [nAngle] += (fixang) scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				pCurAngle->v.vec [nAngle] = pGoalAngle->v.vec [nAngle];
			}
		}
	}
}

// ----------------------------------------------------------------------------------
void SetNextPrimaryFireTime(tAILocalInfo *pLocalInfo, tRobotInfo *pRobotInfo)
{
	pLocalInfo->nRapidFireCount++;

if (pLocalInfo->nRapidFireCount < pRobotInfo->nRapidFireCount [gameStates.app.nDifficultyLevel])
	pLocalInfo->pNextrimaryFire = min(I2X (1)/8, pRobotInfo->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
else {
	pLocalInfo->nRapidFireCount = 0;
	pLocalInfo->pNextrimaryFire = pRobotInfo->primaryFiringWait [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the pPlayer, they attack.
//	If pPlayer is cloaked, then pRobot probably didn't actually collide, deal with that here.
void DoD1AIRobotHitAttack (CObject *pRobot, CObject *pPlayer, CFixVector *vCollision)
{
	tAILocalInfo	*pLocalInfo = &gameData.ai.localInfo [OBJECTS.Index (pRobot)];
	tRobotInfo		*pRobotInfo = &gameData.botData.info [1][pRobot->info.nId];

//#if DBG
if (!gameStates.app.cheats.bRobotsFiring)
	return;
//#endif

//	If pPlayer is dead, stop firing.
if (LOCALOBJECT->info.nType == OBJ_GHOST)
	return;

if (pRobotInfo->attackType == 1) {
	if (pLocalInfo->pNextrimaryFire <= 0) {
		if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
			if (CFixVector::Dist (OBJPOS (gameData.objData.pConsole)->vPos, pRobot->info.position.vPos) <
				 pRobot->info.xSize + gameData.objData.pConsole->info.xSize + I2X (2))
				pPlayer->CollidePlayerAndNastyRobot (pRobot, *vCollision);

		pRobot->cType.aiInfo.GOAL_STATE = D1_AIS_RECO;
		SetNextPrimaryFireTime (pLocalInfo, pRobotInfo);
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------

void ai_multi_send_robot_position (int32_t nObject, int32_t force)
{
if (IsMultiGame)
	MultiSendRobotPosition(nObject, force != -1);
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter vec_to_player is only passed now because guns which aren't on the forward vector from the
//	center of the pRobot will not fire right at the pPlayer.  We need to aim the guns at the pPlayer.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
void ai_fire_laser_at_player(CObject *pObj, CFixVector *fire_point)
{
	int32_t				nObject = pObj->Index ();
	tAILocalInfo*	pLocalInfo = &gameData.ai.localInfo [nObject];
	tRobotInfo*		pRobotInfo = &gameData.botData.info [1][pObj->info.nId];
	CFixVector		vFire;
	CFixVector		bpp_diff;

	if (!gameStates.app.cheats.bRobotsFiring)
		return;

#if DBG
	//	We should never be coming here for the green guy, as he has no laser!
	if (pRobotInfo->attackType == 1)
		Int3();	// Contact Mike: This is impossible.
#endif

	if (pObj->info.controlType == CT_MORPH)
		return;

	//	If pPlayer is exploded, stop firing.
	if (LOCALPLAYER.m_bExploded)
		return;

	//	If pPlayer is cloaked, maybe don't fire based on how long cloaked and randomness.
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	cloak_time = gameData.ai.cloakInfo [nObject % D1_MAX_AI_CLOAK_INFO].lastTime;

		if (gameData.time.xGame - cloak_time > CLOAK_TIME_MAX/4)
			if (RandShort () > FixDiv(gameData.time.xGame - cloak_time, CLOAK_TIME_MAX)/2) {
				SetNextPrimaryFireTime(pLocalInfo, pRobotInfo);
				return;
			}
	}

	//	Set position to fire at based on difficulty level.
	bpp_diff.v.coord.x = gameData.ai.target.vBelievedPos.v.coord.x + SRandShort () * (DIFFICULTY_LEVEL_COUNT-gameStates.app.nDifficultyLevel-1) * 4;
	bpp_diff.v.coord.y = gameData.ai.target.vBelievedPos.v.coord.y + SRandShort () * (DIFFICULTY_LEVEL_COUNT-gameStates.app.nDifficultyLevel-1) * 4;
	bpp_diff.v.coord.z = gameData.ai.target.vBelievedPos.v.coord.z + SRandShort () * (DIFFICULTY_LEVEL_COUNT-gameStates.app.nDifficultyLevel-1) * 4;

	//	Half the time fire at the pPlayer, half the time lead the pPlayer.
	if (RandShort () > 16384) {

		CFixVector::NormalizedDir(vFire, bpp_diff, *fire_point);

		}
	else {
		CFixVector	vPlayerDirection = bpp_diff - bpp_diff;

		// If pPlayer is not moving, fire right at him!
		//	Note: If the pRobot fires in the direction of its forward vector, this is bad because the weapon does not
		//	come out from the center of the pRobot; it comes out from the side.  So it is common for the weapon to miss
		//	its target.  Ideally, we want to point the guns at the pPlayer.  For now, just fire right at the pPlayer.
		if ((abs(vPlayerDirection.v.coord.x < 0x10000)) && (abs(vPlayerDirection.v.coord.y < 0x10000)) && (abs(vPlayerDirection.v.coord.z < 0x10000))) {

			CFixVector::NormalizedDir (vFire, bpp_diff, *fire_point);

		// Player is moving.  Determine where the pPlayer will be at the end of the next frame if he doesn't change his
		//	behavior.  Fire at exactly that point.  This isn't exactly what you want because it will probably take the laser
		//	a different amount of time to get there, since it will probably be a different distance from the pPlayer.
		//	So, that's why we write games, instead of guiding missiles...
			}
		else {
			vFire = bpp_diff - *fire_point;
			vFire *= FixMul (WI_speed (pObj->info.nId, gameStates.app.nDifficultyLevel), gameData.time.xFrame);

			vFire += vPlayerDirection;
			CFixVector::Normalize (vFire);

		}
	}

	CreateNewWeaponSimple ( &vFire, fire_point, pObj->Index (), pRobotInfo->nWeaponType, 1);

	if (IsMultiGame)
 {
		ai_multi_send_robot_position (nObject, -1);
		MultiSendRobotFire (nObject, pObj->cType.aiInfo.CURRENT_GUN, &vFire);
	}

	CreateAwarenessEvent (pObj, D1_PA_NEARBY_ROBOT_FIRED);

	SetNextPrimaryFireTime(pLocalInfo, pRobotInfo);

	//	If the boss fired, allow him to teleport very soon (right after firing, cool!), pending other factors.
	if (pObj->IsBoss ())
		gameData.bosses [0].m_nLastTeleportTime -= gameData.bosses [0].m_nTeleportInterval/2;
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
void move_towards_vector (CObject *pObj, CFixVector *vec_goal)
{
tRobotInfo *pRobotInfo = &gameData.botData.info [1][pObj->info.nId];
if (!pRobotInfo)
	return;

//	Trying to move towards pPlayer.  If forward vector much different than velocity vector,
//	bash velocity vector twice as much towards pPlayer as usual.
tPhysicsInfo& pi = &pObj->mType.physInfo;
CFixVector vel = pi.velocity;
CFixVector::Normalize (vel);
fix dot = CFixVector::Dot (vel, pObj->info.position.mOrient.m.dir.f);

if (dot < I2X (3) / 4) {
	//	This funny code is supposed to slow down the pRobot and move his velocity towards his direction
	//	more quickly than the general code
	pi.velocity.v.coord.x = pi.velocity.v.coord.x/2 + FixMul((*vec_goal).v.coord.x, gameData.time.xFrame * 32);
	pi.velocity.v.coord.y = pi.velocity.v.coord.y/2 + FixMul((*vec_goal).v.coord.y, gameData.time.xFrame * 32);
	pi.velocity.v.coord.z = pi.velocity.v.coord.z/2 + FixMul((*vec_goal).v.coord.z, gameData.time.xFrame * 32);
	} 
else {
	pi.velocity.v.coord.x += FixMul((*vec_goal).v.coord.x, gameData.time.xFrame * 64) * (gameStates.app.nDifficultyLevel + 5) / 4;
	pi.velocity.v.coord.y += FixMul((*vec_goal).v.coord.y, gameData.time.xFrame * 64) * (gameStates.app.nDifficultyLevel + 5) / 4;
	pi.velocity.v.coord.z += FixMul((*vec_goal).v.coord.z, gameData.time.xFrame * 64) * (gameStates.app.nDifficultyLevel + 5) / 4;
	}

fix speed = pi.velocity.Mag ();
fix xMaxSpeed = pObj->MaxSpeed ();

//	Green guy attacks twice as fast as he moves away.
if (pRobotInfo->attackType == 1)
	xMaxSpeed *= 2;

if (speed > xMaxSpeed) {
	pi.velocity.v.coord.x = (pi.velocity.v.coord.x * 3) / 4;
	pi.velocity.v.coord.y = (pi.velocity.v.coord.y * 3) / 4;
	pi.velocity.v.coord.z = (pi.velocity.v.coord.z * 3) / 4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_towards_player(CObject *pObj, CFixVector *vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
	move_towards_vector (pObj, vec_to_player);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means Normal slide about.  fast_flag = 0 means no evasion.
void move_around_player(CObject *pObj, CFixVector *vec_to_player, int32_t fast_flag)
{
	tPhysicsInfo&	pi = pObj->mType.physInfo;
	fix				speed;
	tRobotInfo		*pRobotInfo = &gameData.botData.info [1][pObj->info.nId];
	int32_t			nObject = pObj->Index ();
	int32_t			dir;
	int32_t			dir_change;
	fix				ft;
	CFixVector		vEvade;
	int32_t			count=0;

if (fast_flag == 0)
	return;

vEvade.SetZero ();
dir_change = 48;
ft = gameData.time.xFrame;
if (ft < I2X (1)/32) {
	dir_change *= 8;
	count += 3;
	} 
else
	while (ft < I2X (1)/4) {
		dir_change *= 2;
		ft *= 2;
		count++;
		}

dir = (gameData.app.nFrameCount + (count+1) * (nObject*8 + nObject*4 + nObject)) & dir_change;
dir >>= (4+count);

switch (dir) {
	case 0:
		vEvade.v.coord.x = FixMul((*vec_to_player).v.coord.z, gameData.time.xFrame * 32);
		vEvade.v.coord.y = FixMul((*vec_to_player).v.coord.y, gameData.time.xFrame * 32);
		vEvade.v.coord.z = FixMul(-(*vec_to_player).v.coord.x, gameData.time.xFrame * 32);
		break;
	case 1:
		vEvade.v.coord.x = FixMul(-(*vec_to_player).v.coord.z, gameData.time.xFrame * 32);
		vEvade.v.coord.y = FixMul((*vec_to_player).v.coord.y, gameData.time.xFrame * 32);
		vEvade.v.coord.z = FixMul((*vec_to_player).v.coord.x, gameData.time.xFrame * 32);
		break;
	case 2:
		vEvade.v.coord.x = FixMul(-(*vec_to_player).v.coord.y, gameData.time.xFrame * 32);
		vEvade.v.coord.y = FixMul((*vec_to_player).v.coord.x, gameData.time.xFrame * 32);
		vEvade.v.coord.z = FixMul((*vec_to_player).v.coord.z, gameData.time.xFrame * 32);
		break;
	case 3:
		vEvade.v.coord.x = FixMul((*vec_to_player).v.coord.y, gameData.time.xFrame * 32);
		vEvade.v.coord.y = FixMul(-(*vec_to_player).v.coord.x, gameData.time.xFrame * 32);
		vEvade.v.coord.z = FixMul((*vec_to_player).v.coord.z, gameData.time.xFrame * 32);
		break;
	}

	//	Note: -1 means Normal circling about the pPlayer.  > 0 means fast evasion.
if (fast_flag > 0) {
	//	Only take evasive action if looking at pPlayer.
	//	Evasion speed is scaled by percentage of shield left so wounded robots evade less effectively.
	fix dot = CFixVector::Dot (*vec_to_player, pObj->info.position.mOrient.m.dir.f);
	if ((dot > pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel]) && !(gameData.objData.pConsole->info.nFlags & PLAYER_FLAGS_CLOAKED)) {
		fix	damage_scale;

		damage_scale = FixDiv (pObj->info.xShield, pRobotInfo->strength);
		if (damage_scale > I2X (1))
			damage_scale = I2X (1);		//	Just in case...
		else if (damage_scale < 0)
			damage_scale = 0;			//	Just in case...

		vEvade *= (I2X(fast_flag) + damage_scale);
		}
	}

pi.velocity += vEvade;

speed = pi.velocity.Mag ();
if (speed > pObj->MaxSpeed ()) {
	pi.velocity.v.coord.x = (pi.velocity.v.coord.x*3)/4;
	pi.velocity.v.coord.y = (pi.velocity.v.coord.y*3)/4;
	pi.velocity.v.coord.z = (pi.velocity.v.coord.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_away_from_player(CObject *pObj, CFixVector *vec_to_player, int32_t attackType)
{
	fix				speed;
	tPhysicsInfo&	pi = pObj->mType.physInfo;
	tRobotInfo		*pRobotInfo = &gameData.botData.info [1][pObj->info.nId];
	int32_t			objref;

	pi.velocity -= *vec_to_player * (gameData.time.xFrame * 16);

if (attackType) {
	//	Get value in 0..3 to choose evasion direction.
	objref = (pObj->Index () ^ ((gameData.app.nFrameCount + 3 * pObj->Index ()) >> 5)) & 3;
	switch (objref) {
		case 0:	
			pi.velocity += pObj->info.position.mOrient.m.dir.u * ( gameData.time.xFrame << 5);	
			break;
		case 1:	
			pi.velocity += pObj->info.position.mOrient.m.dir.u * (-gameData.time.xFrame << 5);	
			break;
		case 2:	
			pi.velocity += pObj->info.position.mOrient.m.dir.r * ( gameData.time.xFrame << 5);	
			break;
		case 3:	
			pi.velocity += pObj->info.position.mOrient.m.dir.r * (-gameData.time.xFrame << 5);	
			break;
		default:	
			BRP;	//	Impossible, bogus value on objref, must be in 0..3
		}
	}

speed = pi.velocity.Mag();
if (speed > pObj->MaxSpeed ()) {
	pi.velocity.v.coord.x = (pi.velocity.v.coord.x*3)/4;
	pi.velocity.v.coord.y = (pi.velocity.v.coord.y*3)/4;
	pi.velocity.v.coord.z = (pi.velocity.v.coord.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around pPlayer.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == D1_AIM_STILL).
void ai_move_relative_to_player(CObject *pObj, tAILocalInfo *pLocalInfo, fix dist_to_player, CFixVector *vec_to_player, fix circle_distance, int32_t evade_only)
{
	CObject		*pDangerObj;
	tRobotInfo	*pRobotInfo = &gameData.botData.info [1][pObj->info.nId];

	//	See if should take avoidance.

// New way, green guys don't evade:	if ((pRobotInfo->attackType == 0) && (pObj->cType.aiInfo.nDangerLaser != -1)) {
	if (pObj->cType.aiInfo.nDangerLaser != -1) {
		pDangerObj = OBJECT (pObj->cType.aiInfo.nDangerLaser);

		if ((pDangerObj->info.nType == OBJ_WEAPON) && (pDangerObj->info.nSignature == pObj->cType.aiInfo.nDangerLaserSig)) {
			fix			dot, dist_to_laser, fieldOfView;
			CFixVector	vec_to_laser, laser_fVec;

			fieldOfView = gameData.botData.info [1][pObj->info.nId].fieldOfView [gameStates.app.nDifficultyLevel];

			vec_to_laser = pDangerObj->info.position.vPos - pObj->info.position.vPos;
			dist_to_laser = CFixVector::Normalize (vec_to_laser);
			dot = CFixVector::Dot (vec_to_laser, pObj->info.position.mOrient.m.dir.f);

			if (dot > fieldOfView) {
				fix			laser_robot_dot;
				CFixVector	laser_vec_to_robot;

				//	The laser is seen by the pRobot, see if it might hit the pRobot.
				//	Get the laser's direction.  If it's a polyobj, it can be gotten cheaply from the orientation matrix.
				if (pDangerObj->info.renderType == RT_POLYOBJ)
					laser_fVec = pDangerObj->info.position.mOrient.m.dir.f;
				else {		//	Not a polyobj, get velocity and Normalize.
					laser_fVec = pDangerObj->mType.physInfo.velocity;	//pDangerObj->info.position.mOrient.m.v.f;
					CFixVector::Normalize (laser_fVec);
				}
				laser_vec_to_robot = pObj->info.position.vPos - pDangerObj->info.position.vPos;
				CFixVector::Normalize (laser_vec_to_robot);
				laser_robot_dot = CFixVector::Dot (laser_fVec, laser_vec_to_robot);

				if ((laser_robot_dot > I2X (7) / 8) && (dist_to_laser < I2X (80))) {
					int32_t	evadeSpeed;

					D1_AI_evaded = 1;
					evadeSpeed = gameData.botData.info [1][pObj->info.nId].evadeSpeed [gameStates.app.nDifficultyLevel];

					move_around_player(pObj, vec_to_player, evadeSpeed);
				}
			}
			return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-type guys to keep coming, don't return here after evasion.
	if ((!pRobotInfo->attackType) && evade_only)
		return;

	//	If we fall out of above, then no CObject to be avoided.
	pObj->cType.aiInfo.nDangerLaser = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
	if (pRobotInfo->attackType == 1) {
		if (((pLocalInfo->pNextrimaryFire > pRobotInfo->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) && (dist_to_player < I2X (30))) || gameStates.app.bPlayerIsDead) {
			//	1/4 of time, move around pPlayer, 3/4 of time, move away from pPlayer
			if (RandShort () < 8192) {
				move_around_player(pObj, vec_to_player, -1);
			} else {
				move_away_from_player(pObj, vec_to_player, 1);
			}
		} else {
			move_towards_player(pObj, vec_to_player);
		}
	} else {
		if (dist_to_player < circle_distance)
			move_away_from_player(pObj, vec_to_player, 0);
		else if (dist_to_player < circle_distance*2)
			move_around_player(pObj, vec_to_player, -1);
		else
			move_towards_player(pObj, vec_to_player);
	}

}


//	-------------------------------------------------------------------------------------------------------------------
int32_t	Break_on_object = -1;

void do_firing_stuff(CObject *pObj, int32_t player_visibility, CFixVector *vec_to_player)
{
	if (player_visibility >= 1) {
		//	Now, if in pRobot's field of view, lock onto pPlayer
		fix	dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, *vec_to_player);
		if ((dot >= I2X (7)/8) || (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
			tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
			tAILocalInfo		*pLocalInfo = &gameData.ai.localInfo [pObj->Index ()];

			switch (pStaticInfo->GOAL_STATE) {
				case D1_AIS_NONE:
				case D1_AIS_REST:
				case D1_AIS_SRCH:
				case D1_AIS_LOCK:
					pStaticInfo->GOAL_STATE = D1_AIS_FIRE;
					if (pLocalInfo->targetAwarenessType <= D1_PA_NEARBY_ROBOT_FIRED) {
						pLocalInfo->targetAwarenessType = D1_PA_NEARBY_ROBOT_FIRED;
						pLocalInfo->targetAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
					}
					break;
			}
		} else if (dot >= I2X (1)/2) {
			tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
			switch (pStaticInfo->GOAL_STATE) {
				case D1_AIS_NONE:
				case D1_AIS_REST:
				case D1_AIS_SRCH:
					pStaticInfo->GOAL_STATE = D1_AIS_LOCK;
					break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding pRobot gets bumped or hit, he decides to find another hiding place.
void DoD1AIRobotHit (CObject *pObj, int32_t type)
{
	if (pObj->info.controlType == CT_AI) {
		if ((type == D1_PA_WEAPON_ROBOT_COLLISION) || (type == D1_PA_PLAYER_COLLISION))
			switch (pObj->cType.aiInfo.behavior) {
				case D1_AIM_HIDE:
					pObj->cType.aiInfo.SUBMODE = AISM_GOHIDE;
					break;
				case D1_AIM_STILL:
					gameData.ai.localInfo [pObj->Index ()].mode = D1_AIM_CHASE_OBJECT;
					break;
			}
	}

}
#if DBG
int32_t	Do_ai_flag=1;
#endif

#define	CHASE_TIME_LENGTH		(I2X (8))

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely player_is_visible_from_object would benefit from the
//	information of a normalized vec_to_player.
//	Return pPlayer visibility:
//		0		not visible
//		1		visible, but pRobot not looking at pPlayer (ie, on an unobstructed vector)
//		2		visible and in pRobot's field of view
//		-1		pPlayer is cloaked
//	If the pPlayer is cloaked, set vec_to_player based on time pPlayer cloaked and last uncloaked position.
//	Updates pLocalInfo->nPrevVisibility if pPlayer is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to player_visibility
void compute_vis_and_vec (CObject *pObj, CFixVector *pos, tAILocalInfo *pLocalInfo, CFixVector *vec_to_player, int32_t *player_visibility, tRobotInfo *pRobotInfo, int32_t *flag)
{
if (!*flag) {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix	delta_time, dist;
		int32_t	cloak_index = pObj->Index () % D1_MAX_AI_CLOAK_INFO;

		delta_time = gameData.time.xGame - gameData.ai.cloakInfo [cloak_index].lastTime;
		if (delta_time > I2X (2)) {
			gameData.ai.cloakInfo [cloak_index].lastTime = gameData.time.xGame;
			CFixVector randvec = CFixVector::Random();
			gameData.ai.cloakInfo [cloak_index].vLastPos += randvec * (8*delta_time);
			}

		dist = CFixVector::NormalizedDir (*vec_to_player, gameData.ai.cloakInfo [cloak_index].vLastPos, *pos);
		*player_visibility = player_is_visible_from_object (pObj, pos, pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel], vec_to_player);
		// *player_visibility = 2;

		if ((pLocalInfo->nextMiscSoundTime < gameData.time.xGame) && (pLocalInfo->pNextrimaryFire < I2X (1)) && (dist < I2X (20))) {
			pLocalInfo->nextMiscSoundTime = gameData.time.xGame + (RandShort () + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 1;
			audio.CreateSegmentSound (pRobotInfo->seeSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
			}
		}
	else {
		//	Compute expensive stuff -- vec_to_player and player_visibility
		CFixVector::NormalizedDir (*vec_to_player, gameData.ai.target.vBelievedPos, *pos);
		if (vec_to_player->IsZero ()) {
			(*vec_to_player).v.coord.x = I2X (1);
			}
		*player_visibility = player_is_visible_from_object(pObj, pos, pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel], vec_to_player);

		//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
		//	see you without killing frame rate.
		tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
		if ((*player_visibility == 2) && (pLocalInfo->nPrevVisibility != 2))
			if ((pStaticInfo->GOAL_STATE == D1_AIS_REST) || (pStaticInfo->CURRENT_STATE == D1_AIS_REST)) {
				pStaticInfo->GOAL_STATE = D1_AIS_FIRE;
				pStaticInfo->CURRENT_STATE = D1_AIS_FIRE;
				}

		if (!LOCALPLAYER.m_bExploded && (pLocalInfo->nPrevVisibility != *player_visibility) && (*player_visibility == 2)) {
			if (pLocalInfo->nPrevVisibility == 0) {
				if (pLocalInfo->timeTargetSeen + I2X (1)/2 < gameData.time.xGame) {
					audio.CreateSegmentSound (pRobotInfo->seeSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
					pLocalInfo->timeTargetSoundAttacked = gameData.time.xGame;
					pLocalInfo->nextMiscSoundTime = gameData.time.xGame + I2X (1) + RandShort ()*4;
					}
				}
			else if (pLocalInfo->timeTargetSoundAttacked + I2X (1)/4 < gameData.time.xGame) {
				audio.CreateSegmentSound (pRobotInfo->attackSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
				pLocalInfo->timeTargetSoundAttacked = gameData.time.xGame;
				}
			}

		if ((*player_visibility == 2) && (pLocalInfo->nextMiscSoundTime < gameData.time.xGame)) {
			pLocalInfo->nextMiscSoundTime = gameData.time.xGame + (RandShort () + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 2;
			audio.CreateSegmentSound (pRobotInfo->attackSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
			}
		pLocalInfo->nPrevVisibility = *player_visibility;
		}
	*flag = 1;
	if (*player_visibility)
		pLocalInfo->timeTargetSeen = gameData.time.xGame;
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move the CObject pObj to a spot in which it doesn't intersect a wall.
//	It might mean moving it outside its current CSegment.
void move_object_to_legal_spot(CObject *pObj)
{
	CFixVector	original_pos = pObj->info.position.vPos;
	int32_t			i;
	CSegment*	pSeg = SEGMENT (pObj->info.nSegment);

	for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		if (pSeg->IsPassable (i, pObj) & WID_PASSABLE_FLAG) {
			CFixVector	vSegCenter, goal_dir;

			vSegCenter = SEGMENT (pSeg->m_children [i])->Center ();
			goal_dir = vSegCenter - pObj->info.position.vPos;
			goal_dir *= pObj->info.xSize;
			pObj->info.position.vPos += goal_dir;
			if (!ObjectIntersectsWall (pObj)) {
				int32_t	nNewSeg = FindSegByPos (pObj->info.position.vPos, pObj->info.nSegment, 1, 0);

				if (nNewSeg != -1) {
					pObj->RelinkToSeg (nNewSeg);
					return;
				}
			} else
				pObj->info.position.vPos = original_pos;
		}
	}

	// Int3();		//	Darn you John, you done it again!  (But contact Mike)
	pObj->ApplyDamageToRobot(pObj->info.xShield*2, pObj->Index ());
}

// --------------------------------------------------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
void move_towards_segment_center(CObject *pObj)
{
	fix			dist_to_center;
	CFixVector	vSegCenter, goal_dir;

	vSegCenter = SEGMENT (pObj->info.nSegment)->Center ();
	goal_dir = vSegCenter - pObj->info.position.vPos;
	dist_to_center = CFixVector::Normalize (goal_dir);
	if (dist_to_center < pObj->info.xSize) {
		//	Center is nearer than the distance we want to move, so move to center.
		pObj->info.position.vPos = vSegCenter;
		if (ObjectIntersectsWall(pObj)) {
			move_object_to_legal_spot(pObj);
		}
	} else {
		int32_t	nNewSeg;
		//	Move one radii towards center.
		goal_dir *= pObj->info.xSize;
		pObj->info.position.vPos += goal_dir;
		nNewSeg = FindSegByPos (pObj->info.position.vPos, pObj->info.nSegment, 1, 0);
		if (nNewSeg == -1) {
			pObj->info.position.vPos = vSegCenter;
			move_object_to_legal_spot(pObj);
		}
	}

}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type pRobot.
//	Only brains and avoid robots can open doors.
int32_t ai_door_is_openable(CObject *pObj, CSegment *pSeg, int32_t sidenum)
{
	int32_t	nWall;

	//	The mighty console CObject can open all doors (for purposes of determining paths).
	if (pObj == gameData.objData.pConsole) {
		int32_t	nWall = pSeg->m_sides [sidenum].m_nWall;

		if (WALL (nWall)->nType == WALL_DOOR)
			return 1;
	}

	if ((pObj->info.nId == ROBOT_BRAIN) || (pObj->cType.aiInfo.behavior == D1_AIB_RUN_FROM)) {
		nWall = pSeg->m_sides [sidenum].m_nWall;

		if (nWall != -1)
			if ((WALL (nWall)->nType == WALL_DOOR) && (WALL (nWall)->keys == KEY_NONE) && !(WALL (nWall)->flags & WALL_DOOR_LOCKED))
				return 1;
	}

	return 0;
}

//--//	-----------------------------------------------------------------------------------------------------------
//--//	Return true if CObject *pObj is allowed to open door at nWall
//--int32_t door_openable_by_robot(CObject *pObj, int32_t nWall)
//--{
//--	if (pObj->info.nId == ROBOT_BRAIN)
//--		if (WALL (nWall)->keys == KEY_NONE)
//--			return 1;
//--
//--	return 0;
//--}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if a special CObject (pPlayer or control center) is in this CSegment.
int32_t special_object_in_seg (int32_t nSegment)
{
	int32_t nObject = SEGMENT (nSegment)->m_objects;

while (nObject != -1) {
	if ((OBJECT (nObject)->info.nType == OBJ_PLAYER) || (OBJECT (nObject)->info.nType == OBJ_REACTOR))
		return 1;
	nObject = OBJECT (nObject)->info.nNextInSeg;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Randomly select a CSegment attached to *pSeg, reachable by flying.
int32_t get_random_child(int32_t nSegment)
{
CSegment	*pSeg = SEGMENT (nSegment);
int32_t sidenum = (RandShort () * 6) >> 15;
while (!(pSeg->IsPassable (sidenum, NULL) & WID_PASSABLE_FLAG))
	sidenum = (RandShort () * 6) >> 15;
return pSeg->m_children [sidenum];
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if CObject created, else return false.
int32_t CreateGatedRobot (int32_t nSegment, int32_t nObjId)
{
	int32_t		nObject;
	CObject*		pObj;
	CSegment*	pSeg = SEGMENT (nSegment);
	CFixVector	vObjPos;
	tRobotInfo*	pRobotInfo = &gameData.botData.info [1][nObjId];
	int32_t		count = 0;
	fix			objsize = gameData.models.polyModels [0][pRobotInfo->nModel].Rad ();
	int32_t		default_behavior;

	FORALL_ROBOT_OBJS (pObj) {
		if (pObj->info.nCreator == BOSS_GATE_PRODUCER_NUM)
			count++;
		}
	if (count > 2 * gameStates.app.nDifficultyLevel + 3) {
		gameData.bosses [0].m_nLastGateTime = gameData.time.xGame - 3 * gameData.bosses [0].m_nGateInterval / 4;
		return 0;
		}
	vObjPos = pSeg->RandomPoint ();
	//	See if legal to place CObject here.  If not, move about in CSegment and try again.
	if (CheckObjectObjectIntersection(&vObjPos, objsize, pSeg)) {
		gameData.bosses [0].m_nLastGateTime = gameData.time.xGame - 3*gameData.bosses [0].m_nGateInterval/4;
		return 0;
		}
	nObject = CreateRobot (nObjId, nSegment, vObjPos);
	pObj = OBJECT (nObject);
	if (!pObj) {
		gameData.bosses [0].m_nLastGateTime = gameData.time.xGame - 3*gameData.bosses [0].m_nGateInterval/4;
		return -1;
		}

	//Set polygon-CObject-specific data
	pObj->rType.polyObjInfo.nModel = pRobotInfo->nModel;
	pObj->rType.polyObjInfo.nSubObjFlags = 0;
	//set Physics info
	pObj->mType.physInfo.mass = pRobotInfo->mass;
	pObj->mType.physInfo.drag = pRobotInfo->drag;
	pObj->mType.physInfo.flags |= (PF_LEVELLING);
	pObj->SetShield (pRobotInfo->strength);
	pObj->info.nCreator = BOSS_GATE_PRODUCER_NUM;	//	flag this pRobot as having been created by the boss.
	default_behavior = D1_AIB_NORMAL;
	if (nObjId == 10)						//	This is a toaster guy!
		default_behavior = D1_AIB_RUN_FROM;
	InitAIObject (pObj->Index (), default_behavior, -1 );		//	Note, -1 = CSegment this pRobot goes to to hide, should probably be something useful
	CreateExplosion (nSegment, vObjPos, I2X(10), ANIM_MORPHING_ROBOT);
	audio.CreateSegmentSound (gameData.effects.animations [0][ANIM_MORPHING_ROBOT].nSound, nSegment, 0, vObjPos, 0, I2X (1));
	pObj->MorphStart ();

	gameData.bosses [0].m_nLastGateTime = gameData.time.xGame;
	LOCALPLAYER.numRobotsLevel++;
	LOCALPLAYER.numRobotsTotal++;
	return nObject;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make CObject pObj gate in a pRobot.
//	The process of him bringing in a pRobot takes one second.
//	Then a pRobot appears somewhere near the pPlayer.
//	Return true if pRobot successfully created, else return false
int32_t gate_in_robot(int32_t type, int32_t nSegment)
{
if (!gameData.bosses.ToS ())
	return -1;
if (nSegment < 0) {
	if (!(gameData.bosses [0].m_gateSegs.Buffer () && gameData.bosses [0].m_nGateSegs))
		return -1;
	nSegment = gameData.bosses [0].m_gateSegs [(RandShort () * gameData.bosses [0].m_nGateSegs) >> 15];
	}
return CreateGatedRobot (nSegment, type);
}

// --------------------------------------------------------------------------------------------------------------------

int32_t boss_fits_in_seg (CObject *pBossObj, int32_t nSegment)
{
	CFixVector	vCenter;
	int32_t			nBossObj = pBossObj - OBJECTS;
	CSegment*	pSeg = SEGMENT (nSegment);

vCenter = SEGMENT (nSegment)->Center ();
for (int32_t nPos = 0; nPos < 9; nPos++) {
	if (!nPos) 
		pBossObj->info.position.vPos = vCenter;
	else if (pSeg->m_vertices [nPos - 1] == 0xFFFF)
		continue;
	else {
		CFixVector	vPos;
		vPos = gameData.segData.vertices [pSeg->m_vertices [nPos - 1]];
		pBossObj->info.position.vPos = CFixVector::Avg(vPos, vCenter);
		}
	OBJECT (nBossObj)->RelinkToSeg (nSegment);
	if (!ObjectIntersectsWall (pBossObj))
		return 1;
	}
return 0;
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI CObject if it is fairly aware of the pPlayer.
//	awareness_level is in 0..100.  Larger numbers indicate greater awareness (eg, 99 if firing at pPlayer).
//	In a given frame, might not get called for an CObject, or might be called more than once.
//	The fact that this routine is not called for a given CObject does not mean that CObject is not interested in the pPlayer.
//	OBJECTS are moved by physics, so they can move even if not interested in a pPlayer.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this pPlayer IS NOT allowed to move this pRobot.
//		1	this pPlayer IS allowed to move this pRobot.
int32_t ai_multiplayer_awareness(CObject *pObj, int32_t awareness_level)
{
if (!IsMultiGame)
	return 1;
if (awareness_level == 0)
	return 0;
return MultiCanControlRobot(pObj->Index (), awareness_level);
}
#if DBG
fix	Prev_boss_shield = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_boss_stuff(CObject *pObj)
{
    //  New code, fixes stupid bug which meant boss never gated in robots if > 32767 seconds played.
    if (gameData.bosses [0].m_nLastTeleportTime > gameData.time.xGame)
        gameData.bosses [0].m_nLastTeleportTime = gameData.time.xGame;

    if (gameData.bosses [0].m_nLastGateTime > gameData.time.xGame)
        gameData.bosses [0].m_nLastGateTime = gameData.time.xGame;

	if (!gameData.bosses [0].m_nDying) {
		if (pObj->cType.aiInfo.CLOAKED == 1) {
			if ((gameData.time.xGame - gameData.bosses [0].m_nCloakStartTime > BOSS_CLOAK_DURATION/3) && (gameData.bosses [0].m_nCloakEndTime - gameData.time.xGame > BOSS_CLOAK_DURATION/3) && (gameData.time.xGame - gameData.bosses [0].m_nLastTeleportTime > gameData.bosses [0].m_nTeleportInterval)) {
				if (ai_multiplayer_awareness(pObj, 98))
					TeleportBoss(pObj);
			} else if (gameData.bosses [0].m_bHitThisFrame) {
				gameData.bosses [0].m_bHitThisFrame = 0;
				gameData.bosses [0].m_nLastTeleportTime -= gameData.bosses [0].m_nTeleportInterval/4;
			}

			if (gameData.time.xGame > gameData.bosses [0].m_nCloakEndTime)
				pObj->cType.aiInfo.CLOAKED = 0;
		} else {
			if ((gameData.time.xGame - gameData.bosses [0].m_nCloakEndTime > gameData.bosses [0].m_nCloakInterval) || gameData.bosses [0].m_bHitThisFrame) {
				if (ai_multiplayer_awareness(pObj, 95))
			 {
					gameData.bosses [0].m_bHitThisFrame = 0;
					gameData.bosses [0].m_nCloakStartTime = gameData.time.xGame;
					gameData.bosses [0].m_nCloakEndTime = gameData.time.xGame+BOSS_CLOAK_DURATION;
					pObj->cType.aiInfo.CLOAKED = 1;
					if (IsMultiGame)
						MultiSendBossActions(pObj->Index (), 2, 0, 0);
				}
			}
		}
	} else
		DoBossDyingFrame (pObj);

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	(I2X (150))

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_super_boss_stuff(CObject *pObj, fix dist_to_player, int32_t player_visibility)
{
	static int32_t eclipState = 0;
	do_boss_stuff(pObj);

	// Only master pPlayer can cause gating to occur.
	if ((IsMultiGame) && !IAmGameHost())
		return;

	if ((dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE) || player_visibility || (IsMultiGame)) {
		if (gameData.time.xGame - gameData.bosses [0].m_nLastGateTime > gameData.bosses [0].m_nGateInterval/2) {
			RestartEffect(BOSS_ECLIP_NUM);
			if (eclipState == 0) {
				MultiSendBossActions(pObj->Index (), 4, 0, 0);
				eclipState = 1;
			}
		}
		else {
			StopEffect(BOSS_ECLIP_NUM);
			if (eclipState == 1) {
				MultiSendBossActions(pObj->Index (), 5, 0, 0);
				eclipState = 0;
			}
		}

		if (gameData.time.xGame - gameData.bosses [0].m_nLastGateTime > gameData.bosses [0].m_nGateInterval)
			if (ai_multiplayer_awareness(pObj, 99)) {
				int32_t	nObject;
				int32_t	randtype = (RandShort () * D1_MAX_GATE_INDEX) >> 15;

				Assert(randtype < int32_t (D1_MAX_GATE_INDEX));
				randtype = super_boss_gate_list [randtype];
				Assert(randtype < gameData.botData.nTypes [1]);

				nObject = gate_in_robot(randtype, -1);
				if ((nObject >= 0) && (IsMultiGame))
			 {
					MultiSendBossActions(pObj->Index (), 3, randtype, nObject);
					SetLocalObjNumMapping (nObject);

				}
			}
	}
}


// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this CObject should be allowed to fire at the pPlayer.
int32_t maybe_ai_do_actual_firing_stuff(CObject *pObj, tAIStaticInfo *pStaticInfo)
{
	if (IsMultiGame)
		if ((pStaticInfo->GOAL_STATE != D1_AIS_FLIN) && (pObj->info.nId != ROBOT_BRAIN))
			if (pStaticInfo->CURRENT_STATE == D1_AIS_FIRE)
				return 1;

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
void ai_do_actual_firing_stuff(CObject *pObj, tAIStaticInfo *pStaticInfo, tAILocalInfo *pLocalInfo, tRobotInfo *pRobotInfo, CFixVector *vec_to_player, fix dist_to_player, CFixVector *vGunPoint, int32_t player_visibility, int32_t object_animates)
{
	fix	dot;

if (player_visibility == 2) {
	//	Changed by mk, 01/04/94, onearm would take about 9 seconds until he can fire at you.
	// if (((!object_animates) || (pLocalInfo->achievedState [pStaticInfo->CURRENT_GUN] == D1_AIS_FIRE)) && (pLocalInfo->pNextrimaryFire <= 0)) {
	if (!object_animates || (pLocalInfo->pNextrimaryFire <= 0)) {
		dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, *vec_to_player);
		if (dot >= I2X (7)/8) {

			if (pStaticInfo->CURRENT_GUN < gameData.botData.info [1][pObj->info.nId].nGuns) {
				if (pRobotInfo->attackType == 1) {
					if (!LOCALPLAYER.m_bExploded && (dist_to_player < pObj->info.xSize + gameData.objData.pConsole->info.xSize + I2X (2))) {		// pRobotInfo->circle_distance [gameStates.app.nDifficultyLevel] + gameData.objData.pConsole->info.xSize) {
						if (!ai_multiplayer_awareness(pObj, ROBOT_FIRE_AGITATION-2))
							return;
						DoD1AIRobotHitAttack(pObj, gameData.objData.pConsole, &pObj->info.position.vPos);
					} else {
						return;
					}
				} else {
					if (vGunPoint->IsZero ()) {
						;
					} else {
						if (!ai_multiplayer_awareness(pObj, ROBOT_FIRE_AGITATION))
							return;
						ai_fire_laser_at_player(pObj, vGunPoint);
					}
				}

				//	Wants to fire, so should go into chase mode, probably.
				if ((pStaticInfo->behavior != D1_AIB_RUN_FROM) && (pStaticInfo->behavior != D1_AIB_STILL) &&
					 (pStaticInfo->behavior != D1_AIB_FOLLOW_PATH) && ((pLocalInfo->mode == D1_AIM_FOLLOW_PATH) || (pLocalInfo->mode == D1_AIM_STILL)))
					pLocalInfo->mode = D1_AIM_CHASE_OBJECT;
			}

			pStaticInfo->GOAL_STATE = D1_AIS_RECO;
			pLocalInfo->goalState [pStaticInfo->CURRENT_GUN] = D1_AIS_RECO;

			// Switch to next gun for next fire.
			pStaticInfo->CURRENT_GUN++;
			if (pStaticInfo->CURRENT_GUN >= gameData.botData.info [1][pObj->info.nId].nGuns)
				pStaticInfo->CURRENT_GUN = 0;
			}
		}
	} 
else if (WI_homingFlag (pObj->info.nId) == 1) {
	//	Robots which fire homing weapons might fire even if they don't have a bead on the pPlayer.
	if (((!object_animates) || (pLocalInfo->achievedState [pStaticInfo->CURRENT_GUN] == D1_AIS_FIRE)) &&
		 (pLocalInfo->pNextrimaryFire <= 0) && (CFixVector::Dist (aiHitResult.vPoint, pObj->info.position.vPos) > I2X (40))) {
		if (!ai_multiplayer_awareness(pObj, ROBOT_FIRE_AGITATION))
			return;
		ai_fire_laser_at_player(pObj, vGunPoint);

		pStaticInfo->GOAL_STATE = D1_AIS_RECO;
		pLocalInfo->goalState [pStaticInfo->CURRENT_GUN] = D1_AIS_RECO;

		// Switch to next gun for next fire.
		pStaticInfo->CURRENT_GUN++;
		if (pStaticInfo->CURRENT_GUN >= gameData.botData.info [1][pObj->info.nId].nGuns)
			pStaticInfo->CURRENT_GUN = 0;
	} else {
		// Switch to next gun for next fire.
		pStaticInfo->CURRENT_GUN++;
		if (pStaticInfo->CURRENT_GUN >= gameData.botData.info [1][pObj->info.nId].nGuns)
			pStaticInfo->CURRENT_GUN = 0;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

void DoD1AIFrame (CObject *pObj)
{
	int32_t				nObject = pObj->Index ();
	tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
	tAILocalInfo	*pLocalInfo = &gameData.ai.localInfo [nObject];
	fix				dist_to_player;
	CFixVector		vec_to_player;
	fix				dot;
	tRobotInfo		*pRobotInfo;
	int32_t				player_visibility=-1;
	int32_t				obj_ref;
	int32_t				object_animates;
	int32_t				new_goalState;
	int32_t				bVisAndVecComputed = 0;
	int32_t				nPrevVisibility;
	CFixVector		vGunPoint;
	CFixVector		vVisVecPos;
	bool				bHaveGunPos = false;

	if (pStaticInfo->SKIP_D1_AI_COUNT) {
		pStaticInfo->SKIP_D1_AI_COUNT--;
		return;
	}
#if DBG
if (nObject == nDbgObj)
	BRP;
#endif
if (pObj->cType.aiInfo.SKIP_AI_COUNT) {
	pObj->cType.aiInfo.SKIP_AI_COUNT--;
	if (pObj->mType.physInfo.flags & PF_USES_THRUST) {
		pObj->mType.physInfo.rotThrust.v.coord.x = 15 * pObj->mType.physInfo.rotThrust.v.coord.x / 16;
		pObj->mType.physInfo.rotThrust.v.coord.y = 15 * pObj->mType.physInfo.rotThrust.v.coord.y / 16;
		pObj->mType.physInfo.rotThrust.v.coord.z = 15 * pObj->mType.physInfo.rotThrust.v.coord.z / 16;
		if (!pObj->cType.aiInfo.SKIP_AI_COUNT)
			pObj->mType.physInfo.flags &= ~PF_USES_THRUST;
		}
	return;
	}

if (DoAnyRobotDyingFrame (pObj))
	return;

	//	Kind of a hack.  If a pRobot is flinching, but it is time for it to fire, unflinch it.
	//	Else, you can turn a big nasty pRobot into a wimp by firing flares at it.
	//	This also allows the pPlayer to see the cool flinch effect for mechs without unbalancing the game.
	if ((pStaticInfo->GOAL_STATE == D1_AIS_FLIN) && (pLocalInfo->pNextrimaryFire < 0)) {
		pStaticInfo->GOAL_STATE = D1_AIS_FIRE;
	}

	gameData.ai.target.vBelievedPos = gameData.ai.cloakInfo [nObject & (D1_MAX_AI_CLOAK_INFO-1)].vLastPos;

	if (!((pStaticInfo->behavior >= MIN_BEHAVIOR) && (pStaticInfo->behavior <= MAX_BEHAVIOR))) {
		pStaticInfo->behavior = D1_AIB_NORMAL;
	}

	Assert(pObj->info.nSegment != -1);
	Assert(pObj->info.nId < gameData.botData.nTypes [1]);

	pRobotInfo = &gameData.botData.info [1][pObj->info.nId];
	Assert(pRobotInfo->always_0xabcd == 0xabcd);
	obj_ref = nObject ^ gameData.app.nFrameCount;
	// -- if (pLocalInfo->wait_time > -I2X (8))
	// -- 	pLocalInfo->wait_time -= gameData.time.xFrame;
	if (pLocalInfo->pNextrimaryFire > -I2X (8))
		pLocalInfo->pNextrimaryFire -= gameData.time.xFrame;
	if (pLocalInfo->timeSinceProcessed < I2X (256))
		pLocalInfo->timeSinceProcessed += gameData.time.xFrame;
	nPrevVisibility = pLocalInfo->nPrevVisibility;	//	Must get this before we toast the master copy!

	//	Deal with cloaking for robots which are cloaked except just before firing.
	if (pRobotInfo->cloakType == RI_CLOAKED_EXCEPT_FIRING) {
		if (pLocalInfo->pNextrimaryFire < I2X (1)/2)
			pStaticInfo->CLOAKED = 1;
		else
			pStaticInfo->CLOAKED = 0;
		}
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
		gameData.ai.target.vBelievedPos = OBJPOS (gameData.objData.pConsole)->vPos;

	dist_to_player = CFixVector::Dist(gameData.ai.target.vBelievedPos, pObj->info.position.vPos);
	//	If this pRobot can fire, compute visibility from gun position.
	//	Don't want to compute visibility twice, as it is expensive.  (So is call to calc_vGunPoint).
	if ((pLocalInfo->pNextrimaryFire <= 0) && (dist_to_player < I2X (200)) && pRobotInfo->nGuns && !pRobotInfo->attackType) {
		bHaveGunPos = CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN) != 0;
		vVisVecPos = vGunPoint;
		}
	else {
		vVisVecPos = pObj->info.position.vPos;
		vGunPoint.SetZero ();
		}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Occasionally make non-still robots make a path to the pPlayer.  Based on agitation and distance from pPlayer.
	if ((pStaticInfo->behavior != D1_AIB_RUN_FROM) && (pStaticInfo->behavior != D1_AIB_STILL) && !(IsMultiGame))
		if (gameData.ai.nOverallAgitation > 70) {
			if ((dist_to_player < I2X (200)) && (RandShort () < gameData.time.xFrame / 4)) {
				if (RandShort () * (gameData.ai.nOverallAgitation - 40) > I2X (5)) {
					CreatePathToTarget(pObj, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					// -- show_path_and_other(pObj);
					return;
				}
			}
		}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If retry count not 0, then add it into nConsecutiveRetries.
	//	If it is 0, cut down nConsecutiveRetries.
	//	This is largely a hack to speed up physics and deal with stupid AI.  This is low level
	//	communication between systems of a sort that should not be done.
	if ((pLocalInfo->nRetryCount) && !(IsMultiGame)) {
		pLocalInfo->nConsecutiveRetries += pLocalInfo->nRetryCount;
		pLocalInfo->nRetryCount = 0;
		if (pLocalInfo->nConsecutiveRetries > 3) {
			switch (pLocalInfo->mode) {
				case D1_AIM_CHASE_OBJECT:
					CreatePathToTarget(pObj, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
					break;
				case D1_AIM_STILL:
					if (!((pStaticInfo->behavior == D1_AIB_STILL) || (pStaticInfo->behavior == D1_AIB_STATION)))	//	Behavior is still, so don't follow path.
						AttemptToResumePath (pObj);
					break;
				case D1_AIM_FOLLOW_PATH:
					if (IsMultiGame)
						pLocalInfo->mode = D1_AIM_STILL;
					else
						AttemptToResumePath(pObj);
					break;
				case D1_AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(pObj);
					pObj->mType.physInfo.velocity.v.coord.x = 0;
					pObj->mType.physInfo.velocity.v.coord.y = 0;
					pObj->mType.physInfo.velocity.v.coord.z = 0;
					CreateNSegmentPath(pObj, 5, -1);
					pLocalInfo->mode = D1_AIM_RUN_FROM_OBJECT;
					break;
				case D1_AIM_HIDE:
					move_towards_segment_center(pObj);
					pObj->mType.physInfo.velocity.v.coord.x = 0;
					pObj->mType.physInfo.velocity.v.coord.y = 0;
					pObj->mType.physInfo.velocity.v.coord.z = 0;
					if (gameData.ai.nOverallAgitation > (50 - gameStates.app.nDifficultyLevel*4))
						CreatePathToTarget(pObj, 4 + gameData.ai.nOverallAgitation/8, 1);
					else {
						CreateNSegmentPath(pObj, 5, -1);
					}
					break;
				case D1_AIM_OPEN_DOOR:
					CreateNSegmentPathToDoor(pObj, 5, -1);
					break;
				#if DBG
				case D1_AIM_FOLLOW_PATH_2:
					Int3();	//	Should never happen!
					break;
				#endif
			}
			pLocalInfo->nConsecutiveRetries = 0;
		}
	} else
		pLocalInfo->nConsecutiveRetries /= 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
 	//	If in materialization center, exit
 	if (!(IsMultiGame) && (SEGMENT (pObj->info.nSegment)->m_function == SEGMENT_FUNC_ROBOTMAKER)) {
 		AIFollowPath (pObj, 1, 1, NULL);		// 1 = pPlayer is visible, which might be a lie, but it works.
 		return;
 	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease pPlayer awareness due to the passage of time.
	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease pPlayer awareness due to the passage of time.
	if (pLocalInfo->targetAwarenessType) {
		if (pLocalInfo->targetAwarenessTime > 0) {
			pLocalInfo->targetAwarenessTime -= gameData.time.xFrame;
			if (pLocalInfo->targetAwarenessTime <= 0) {
 				pLocalInfo->targetAwarenessTime = I2X (2);	//new: 11/05/94
 				pLocalInfo->targetAwarenessType--;	//new: 11/05/94
				}
			}
		else {
			pLocalInfo->targetAwarenessType--;
			pLocalInfo->targetAwarenessTime = I2X (2);
			// pStaticInfo->GOAL_STATE = D1_AIS_REST;
			}
		}
	else
		pStaticInfo->GOAL_STATE = D1_AIS_REST;							//new: 12/13/94


	if (gameStates.app.bPlayerIsDead && (pLocalInfo->targetAwarenessType == 0))
		if ((dist_to_player < I2X (200)) && (RandShort () < gameData.time.xFrame / 8)) {
			if ((pStaticInfo->behavior != D1_AIB_STILL) && (pStaticInfo->behavior != D1_AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(pObj, 30))
					return;
				ai_multi_send_robot_position (nObject, -1);

				if (!((pLocalInfo->mode == D1_AIM_FOLLOW_PATH) && (pStaticInfo->nCurPathIndex < pStaticInfo->nPathLength-1))) {
					if (dist_to_player < I2X (30))
						CreateNSegmentPath(pObj, 5, 1);
					else
						CreatePathToTarget(pObj, 20, 1);
					}
			}
		}

	//	Make sure that if this guy got hit or bumped, then he's chasing pPlayer.
	if ((pLocalInfo->targetAwarenessType == D1_PA_WEAPON_ROBOT_COLLISION) || (pLocalInfo->targetAwarenessType >= D1_PA_PLAYER_COLLISION)) {
		if ((pStaticInfo->behavior != D1_AIB_STILL) && (pStaticInfo->behavior != D1_AIB_FOLLOW_PATH) && (pStaticInfo->behavior != D1_AIB_RUN_FROM) && (pObj->info.nId != ROBOT_BRAIN))
			pLocalInfo->mode = D1_AIM_CHASE_OBJECT;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((pStaticInfo->GOAL_STATE == D1_AIS_FLIN) && (pStaticInfo->CURRENT_STATE == D1_AIS_FLIN))
		pStaticInfo->GOAL_STATE = D1_AIS_LOCK;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Note: Should only do these two function calls for objects which animate
	if ((dist_to_player < I2X (100))) { // && !(IsMultiGame)) {
		object_animates = do_silly_animation(pObj);
		if (object_animates)
			ai_frame_animation(pObj);
		}
	else {
		//	If Object is supposed to animate, but we don't let it animate due to distance, then
		//	we must change its state, else it will never update.
		pStaticInfo->CURRENT_STATE = pStaticInfo->GOAL_STATE;
		object_animates = 0;		//	If we're not doing the animation, then should pretend it doesn't animate.
		}

	switch (gameData.botData.info [1][pObj->info.nId].bossFlag) {
		case 0:
			break;
		case 1:
			if (pStaticInfo->GOAL_STATE == D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_FIRE;
			if (pStaticInfo->CURRENT_STATE == D1_AIS_FLIN)
				pStaticInfo->CURRENT_STATE = D1_AIS_FIRE;
			dist_to_player /= 4;

			do_boss_stuff(pObj);
			dist_to_player *= 4;
			break;
#ifndef SHAREWARE
		case 2:
			if (pStaticInfo->GOAL_STATE == D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_FIRE;
			if (pStaticInfo->CURRENT_STATE == D1_AIS_FLIN)
				pStaticInfo->CURRENT_STATE = D1_AIS_FIRE;
			compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);

		 {	int32_t pv = player_visibility;
				fix	dtp = dist_to_player/4;

			//	If pPlayer cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = CFixVector::Dist(OBJPOS (gameData.objData.pConsole)->vPos, pObj->info.position.vPos)/4;
			}

			do_super_boss_stuff(pObj, dtp, pv);
			}
			break;
#endif
		default:
			Int3();	//	Bogus boss flag value.
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Time-slice, don't process all the time, purely an efficiency hack.
	//	Guys whose behavior is station and are not at their hide CSegment get processed anyway.
	if (pLocalInfo->targetAwarenessType < D1_PA_WEAPON_ROBOT_COLLISION-1) { // If pRobot got hit, he gets to attack pPlayer always!
		#if DBG
		if (Break_on_object != nObject) {	//	don't time slice if we're interested in this CObject.
		#endif
			if ((dist_to_player > I2X (250)) && (pLocalInfo->timeSinceProcessed <= I2X (2)))
				return;
			else if ((pStaticInfo->behavior != D1_AIB_STATION) || (pLocalInfo->mode != D1_AIM_FOLLOW_PATH) || (pStaticInfo->nHideSegment == pObj->info.nSegment)) {
				if ((dist_to_player > I2X (150)) && (pLocalInfo->timeSinceProcessed <= I2X (1)))
					return;
				if ((dist_to_player > I2X (100)) && (pLocalInfo->timeSinceProcessed <= I2X (1) / 2))
					return;
			}
		#if DBG
		}
		#endif
	}

	//	Reset time since processed, but skew objects so not everything processed synchronously, else
	//	we get fast frames with the occasional very slow frame.
	// D1_AI_proc_time = pLocalInfo->timeSinceProcessed;
	pLocalInfo->timeSinceProcessed = - ((nObject & 0x03) * gameData.time.xFrame) / 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Perform special ability
	switch (pObj->info.nId) {
		case ROBOT_BRAIN:
			//	Robots function nicely if behavior is Station.  This means they won't move until they
			//	can see the pPlayer, at which time they will start wandering about opening doors.
			if (gameData.objData.pConsole->info.nSegment == pObj->info.nSegment) {
				if (!ai_multiplayer_awareness(pObj, 97))
					return;
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				move_away_from_player(pObj, &vec_to_player, 0);
				ai_multi_send_robot_position (nObject, -1);
			} else if (pLocalInfo->mode != D1_AIM_STILL) {
				int32_t	r;

				r = pObj->OpenableDoorsInSegment ();
				if (r != -1) {
					pLocalInfo->mode = D1_AIM_OPEN_DOOR;
					pStaticInfo->GOALSIDE = r;
				} else if (pLocalInfo->mode != D1_AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(pObj, 50))
						return;
					CreateNSegmentPathToDoor(pObj, 8+gameStates.app.nDifficultyLevel, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position (nObject, -1);
				}
			} else {
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(pObj, 50))
						return;
					CreateNSegmentPathToDoor(pObj, 8+gameStates.app.nDifficultyLevel, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position (nObject, -1);
				}
			}
			break;
		default:
			break;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	switch (pLocalInfo->mode) {
		case D1_AIM_CHASE_OBJECT: {		// chasing pPlayer, sort of, chase if far, back off if close, circle in between
			fix	circle_distance;

			circle_distance = pRobotInfo->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objData.pConsole->info.xSize;
			//	Green guy doesn't get his circle distance boosted, else he might never attack.
			if (pRobotInfo->attackType != 1)
				circle_distance += I2X (nObject&0xf) / 2;
			compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
			if ((player_visibility < 2) && (nPrevVisibility == 2)) { // this is redundant: mk, 01/15/95: && (pLocalInfo->mode == D1_AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(pObj, 53) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
					if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
						ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
					return;
					}
				CreatePathToTarget(pObj, 8, 1);
				ai_multi_send_robot_position (nObject, -1);
				}
			else if ((player_visibility == 0) && (dist_to_player > I2X (80)) && (!(IsMultiGame))) {
				//	If pretty far from the pPlayer, pPlayer cannot be seen (obstructed) and in chase mode, switch to follow path mode.
				//	This has one desirable benefit of avoiding physics retries.
				if (pStaticInfo->behavior == D1_AIB_STATION) {
					pLocalInfo->nGoalSegment = pStaticInfo->nHideSegment;
					CreatePathToStation(pObj, 15);
					// -- show_path_and_other(pObj);
					}
				else
					CreateNSegmentPath(pObj, 5, -1);
				break;
				}

			if ((pStaticInfo->CURRENT_STATE == D1_AIS_REST) && (pStaticInfo->GOAL_STATE == D1_AIS_REST)) {
				if (player_visibility) {
					if (RandShort () < gameData.time.xFrame*player_visibility) {
						if (dist_to_player/256 < RandShort ()*player_visibility) {
							pStaticInfo->GOAL_STATE = D1_AIS_SRCH;
							pStaticInfo->CURRENT_STATE = D1_AIS_SRCH;
							}
						}
					}
				}

			if (gameData.time.xGame - pLocalInfo->timeTargetSeen > CHASE_TIME_LENGTH) {
				if (IsMultiGame && !player_visibility && (dist_to_player > I2X (70))) {
					pLocalInfo->mode = D1_AIM_STILL;
					return;
					}
				if (!ai_multiplayer_awareness(pObj, 64) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
					if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
						ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
					return;
					}
				CreatePathToTarget(pObj, 10, 1);
				// -- show_path_and_other(pObj);
				ai_multi_send_robot_position (nObject, -1);
				}
			else if ((pStaticInfo->CURRENT_STATE != D1_AIS_REST) && (pStaticInfo->GOAL_STATE != D1_AIS_REST)) {
				if (!ai_multiplayer_awareness(pObj, 70) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
					if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
						ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
					return;
					}
				ai_move_relative_to_player(pObj, pLocalInfo, dist_to_player, &vec_to_player, circle_distance, 0);
				if ((obj_ref & 1) && ((pStaticInfo->GOAL_STATE == D1_AIS_SRCH) || (pStaticInfo->GOAL_STATE == D1_AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector (&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
					else
						ai_turn_randomly(&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
					}
				if (D1_AI_evaded) {
					ai_multi_send_robot_position (nObject, 1);
					D1_AI_evaded = 0;
					}
				else
					ai_multi_send_robot_position (nObject, -1);

				do_firing_stuff(pObj, player_visibility, &vec_to_player);
				}
			break;
			}

		case D1_AIM_RUN_FROM_OBJECT:
			compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);

			if (player_visibility) {
				if (pLocalInfo->targetAwarenessType == 0)
					pLocalInfo->targetAwarenessType = D1_PA_WEAPON_ROBOT_COLLISION;

			}

			//	If in multiplayer, only do if pPlayer visible.  If not multiplayer, do always.
			if (!(IsMultiGame) || player_visibility)
				if (ai_multiplayer_awareness(pObj, 75)) {
					AIFollowPath(pObj, player_visibility, nPrevVisibility, &vec_to_player);
					ai_multi_send_robot_position (nObject, -1);
				}

			if (pStaticInfo->GOAL_STATE != D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_LOCK;
			else if (pStaticInfo->CURRENT_STATE == D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_LOCK;

			//	Bad to let run_from pRobot fire at pPlayer because it will cause a war in which it turns towards the
			//	pPlayer to fire and then towards its goal to move.
			// do_firing_stuff(pObj, player_visibility, &vec_to_player);
			//	Instead, do this:
			//	(Note, only drop if pPlayer is visible.  This prevents the bombs from being a giveaway, and
			//	also ensures that the pRobot is moving while it is dropping.  Also means fewer will be dropped.)
			if ((pLocalInfo->pNextrimaryFire <= 0) && (player_visibility)) {
				CFixVector	vFire, fire_pos;

				if (!ai_multiplayer_awareness(pObj, 75))
					return;

				vFire = pObj->info.position.mOrient.m.dir.f;
				vFire = -vFire;
				fire_pos = pObj->info.position.vPos + vFire;

				CreateNewWeaponSimple( &vFire, &fire_pos, pObj->Index (), PROXMINE_ID, 1);
				pLocalInfo->pNextrimaryFire = I2X (5);		//	Drop a proximity bomb every 5 seconds.

				if (IsMultiGame) {
					ai_multi_send_robot_position (pObj->Index (), -1);
					MultiSendRobotFire(pObj->Index (), -1, &vFire);
				}
			}
			break;

		case D1_AIM_FOLLOW_PATH: {
			int32_t	anger_level = 65;

			if (pStaticInfo->behavior == D1_AIB_STATION)
				if ((pStaticInfo->nHideIndex + pStaticInfo->nPathLength > 0) && (gameData.ai.routeSegs [pStaticInfo->nHideIndex + pStaticInfo->nPathLength - 1].nSegment == pStaticInfo->nHideSegment)) {
					anger_level = 64;
				}
			compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
			if (gameData.app.GameMode (GM_MODEM | GM_SERIAL))
				if (!player_visibility && (dist_to_player > I2X (70))) {
					pLocalInfo->mode = D1_AIM_STILL;
					return;
				}
			if (!ai_multiplayer_awareness(pObj, anger_level) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
#if 0
				if (!bHaveGunPos) {
					bHaveGunPos = CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN) != 0;
					vVisVecPos = vGunPoint;
					}
#endif
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				if (bHaveGunPos)
					ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
				return;
				}

			AIFollowPath(pObj, player_visibility, nPrevVisibility, &vec_to_player);

			if (pStaticInfo->GOAL_STATE != D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_LOCK;
			else if (pStaticInfo->CURRENT_STATE == D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_LOCK;

			if ((pStaticInfo->behavior != D1_AIB_FOLLOW_PATH) && (pStaticInfo->behavior != D1_AIB_RUN_FROM))
				do_firing_stuff(pObj, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (pStaticInfo->behavior != D1_AIB_FOLLOW_PATH) && (pStaticInfo->behavior != D1_AIB_RUN_FROM) && (pObj->info.nId != ROBOT_BRAIN)) {
				if (pRobotInfo->attackType == 0)
					pLocalInfo->mode = D1_AIM_CHASE_OBJECT;
			} else if ((player_visibility == 0) && (pStaticInfo->behavior == D1_AIB_NORMAL) && (pLocalInfo->mode == D1_AIM_FOLLOW_PATH) && (pStaticInfo->behavior != D1_AIB_RUN_FROM)) {
				pLocalInfo->mode = D1_AIM_STILL;
				pStaticInfo->nHideIndex = -1;
				pStaticInfo->nPathLength = 0;
			}

			ai_multi_send_robot_position (nObject, -1);

			break;
		}

		case D1_AIM_HIDE:
			if (!ai_multiplayer_awareness(pObj, 71) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
					ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
				return;
				}

			compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);

 			AIFollowPath(pObj, player_visibility, nPrevVisibility, &vec_to_player);

			if (pStaticInfo->GOAL_STATE != D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_LOCK;
			else if (pStaticInfo->CURRENT_STATE == D1_AIS_FLIN)
				pStaticInfo->GOAL_STATE = D1_AIS_LOCK;

			ai_multi_send_robot_position (nObject, -1);
			break;

		case D1_AIM_STILL:
			if ((dist_to_player < I2X (120) + gameStates.app.nDifficultyLevel * I2X (20)) || (pLocalInfo->targetAwarenessType >= D1_PA_WEAPON_ROBOT_COLLISION - 1)) {
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);

				//	turn towards vector if visible this time or last time, or RandShort
				// new!
				if ((player_visibility) || (nPrevVisibility) || ((RandShort () > 0x4000) && !(IsMultiGame))) {
					if (!ai_multiplayer_awareness(pObj, 71) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
						if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
							ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
						return;
						}
					ai_turn_towards_vector (&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position (nObject, -1);
				}

				do_firing_stuff(pObj, player_visibility, &vec_to_player);
				//	This is debugging code!  Remove it!  It's to make the green guy attack without doing other kinds of movement.
				if (player_visibility) {		//	Change, MK, 01/03/94 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (pRobotInfo->attackType == 1) {
						pStaticInfo->behavior = D1_AIB_NORMAL;
						if (!ai_multiplayer_awareness(pObj, 80) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
							if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
								ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
							return;
							}
						ai_move_relative_to_player(pObj, pLocalInfo, dist_to_player, &vec_to_player, 0, 0);
						if (D1_AI_evaded) {
							ai_multi_send_robot_position (nObject, 1);
							D1_AI_evaded = 0;
						}
						else
							ai_multi_send_robot_position (nObject, -1);
					} else {
						//	Robots in hover mode are allowed to evade at half Normal speed.
						if (!ai_multiplayer_awareness(pObj, 81) && maybe_ai_do_actual_firing_stuff(pObj, pStaticInfo)) {
							if (bHaveGunPos /*|| CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN)*/)
								ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
							return;
							}
						ai_move_relative_to_player(pObj, pLocalInfo, dist_to_player, &vec_to_player, 0, 1);
						if (D1_AI_evaded) {
							ai_multi_send_robot_position (nObject, -1);
							D1_AI_evaded = 0;
						}
						else
							ai_multi_send_robot_position (nObject, -1);
					}
				} else if ((pObj->info.nSegment != pStaticInfo->nHideSegment) && (dist_to_player > I2X (80)) && (!(IsMultiGame))) {
					//	If pretty far from the pPlayer, pPlayer cannot be seen (obstructed) and in chase mode, switch to follow path mode.
					//	This has one desirable benefit of avoiding physics retries.
					if (pStaticInfo->behavior == D1_AIB_STATION) {
						pLocalInfo->nGoalSegment = pStaticInfo->nHideSegment;
						CreatePathToStation(pObj, 15);
						// -- show_path_and_other(pObj);
					}
					break;
				}
			}

			break;
		case D1_AIM_OPEN_DOOR: {		// trying to open a door.
			CFixVector	vCenter, vGoal;
			Assert(pObj->info.nId == ROBOT_BRAIN);		//	Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(pObj, 62))
				return;
			vCenter = SEGMENT (pObj->info.nSegment)->SideCenter (pStaticInfo->GOALSIDE);
			vGoal = vCenter - pObj->info.position.vPos;
			CFixVector::Normalize (vGoal);
			ai_turn_towards_vector (&vGoal, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
			move_towards_vector (pObj, &vGoal);
			ai_multi_send_robot_position (nObject, -1);

			break;
		}

		default:
			pLocalInfo->mode = D1_AIM_CHASE_OBJECT;
			break;
	}		// end:	switch (pLocalInfo->mode) {

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If the pRobot can see you, increase his awareness of you.
	//	This prevents the problem of a pRobot looking right at you but doing nothing.
	// Assert(player_visibility != -1);	//	Means it didn't get initialized!
	compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
	if (player_visibility == 2)
		if (pLocalInfo->targetAwarenessType == 0)
			pLocalInfo->targetAwarenessType = D1_PA_PLAYER_COLLISION;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (!object_animates) {
		pStaticInfo->CURRENT_STATE = pStaticInfo->GOAL_STATE;
	}

	Assert(pLocalInfo->targetAwarenessType <= D1_AIE_MAX);
	Assert(pStaticInfo->CURRENT_STATE < D1_AIS_MAX);
	Assert(pStaticInfo->GOAL_STATE < D1_AIS_MAX);

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (pLocalInfo->targetAwarenessType) {
		new_goalState = D1_AI_transition_table [pLocalInfo->targetAwarenessType-1][pStaticInfo->CURRENT_STATE][pStaticInfo->GOAL_STATE];
		if (pLocalInfo->targetAwarenessType == D1_PA_WEAPON_ROBOT_COLLISION) {
			//	Decrease awareness, else this pRobot will flinch every frame.
			pLocalInfo->targetAwarenessType--;
			pLocalInfo->targetAwarenessTime = I2X (3);
		}

		if (new_goalState == D1_AIS_ERR_)
			new_goalState = D1_AIS_REST;

		if (pStaticInfo->CURRENT_STATE == D1_AIS_NONE)
			pStaticInfo->CURRENT_STATE = D1_AIS_REST;

		pStaticInfo->GOAL_STATE = new_goalState;

	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If new state = fire, then set all gun states to fire.
	if ((pStaticInfo->GOAL_STATE == D1_AIS_FIRE) ) {
		int32_t	i,nGunCount;
		nGunCount = gameData.botData.info [1][pObj->info.nId].nGuns;
		for (i=0; i<nGunCount; i++)
			pLocalInfo->goalState [i] = D1_AIS_FIRE;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his pNextrimaryFire says ok to fire, bash him there
	if ((pLocalInfo->pNextrimaryFire < 0) && (pStaticInfo->GOAL_STATE == D1_AIS_FIRE))
		pStaticInfo->CURRENT_STATE = D1_AIS_FIRE;

	if ((pStaticInfo->GOAL_STATE != D1_AIS_FLIN)  && (pObj->info.nId != ROBOT_BRAIN)) {
		switch (pStaticInfo->CURRENT_STATE) {
			case D1_AIS_NONE:
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, vec_to_player);
				if (dot >= I2X (1)/2)
					if (pStaticInfo->GOAL_STATE == D1_AIS_REST)
						pStaticInfo->GOAL_STATE = D1_AIS_SRCH;
				break;
			case D1_AIS_REST:
				if (pStaticInfo->GOAL_STATE == D1_AIS_REST) {
					compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
					if ((pLocalInfo->pNextrimaryFire <= 0) && (player_visibility)) {
						pStaticInfo->GOAL_STATE = D1_AIS_FIRE;
						}
					}
				break;

			case D1_AIS_SRCH:
				if (!ai_multiplayer_awareness(pObj, 60))
					return;
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				if (player_visibility) {
					ai_turn_towards_vector (&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position (nObject, -1);
					}
				else if (!(IsMultiGame))
					ai_turn_randomly(&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
				break;

			case D1_AIS_LOCK:
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				if (!(IsMultiGame) || (player_visibility)) {
					if (!ai_multiplayer_awareness(pObj, 68))
						return;
					if (player_visibility) {
						ai_turn_towards_vector (&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
						ai_multi_send_robot_position (nObject, -1);
						}
					else if (!(IsMultiGame))
						ai_turn_randomly(&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
						}
				break;

			case D1_AIS_FIRE:
#if 0
				if (!bHaveGunPos) {
					bHaveGunPos = CalcGunPoint (&vGunPoint, pObj, pStaticInfo->CURRENT_GUN) != 0;
					vVisVecPos = vGunPoint;
					}
#endif
				compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(pObj, ROBOT_FIRE_AGITATION-1) && IsMultiGame) {
						if (bHaveGunPos)
							ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
						return;
						}
					ai_turn_towards_vector (&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
					ai_multi_send_robot_position (nObject, -1);
					}
				else if (!(IsMultiGame)) {
					ai_turn_randomly(&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
					}
				//	Fire at pPlayer, if appropriate.
				if (bHaveGunPos)
					ai_do_actual_firing_stuff(pObj, pStaticInfo, pLocalInfo, pRobotInfo, &vec_to_player, dist_to_player, &vGunPoint, player_visibility, object_animates);
				break;

			case D1_AIS_RECO:
				if (!(obj_ref & 3)) {
					compute_vis_and_vec (pObj, &vVisVecPos, pLocalInfo, &vec_to_player, &player_visibility, pRobotInfo, &bVisAndVecComputed);
					if (player_visibility) {
						if (!ai_multiplayer_awareness(pObj, 69))
							return;
						ai_turn_towards_vector (&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
						ai_multi_send_robot_position (nObject, -1);
						} 
					else if (!(IsMultiGame)) {
						ai_turn_randomly(&vec_to_player, pObj, pRobotInfo->turnTime [gameStates.app.nDifficultyLevel], nPrevVisibility);
						}
					}
				break;

			case D1_AIS_FLIN:
				break;
			default:
				pStaticInfo->GOAL_STATE = D1_AIS_REST;
				pStaticInfo->CURRENT_STATE = D1_AIS_REST;
				break;
		}
	} // end of: if (pStaticInfo->GOAL_STATE != D1_AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		pStaticInfo->CURRENT_GUN++;
		if (pStaticInfo->CURRENT_GUN >= gameData.botData.info [1][pObj->info.nId].nGuns)
			pStaticInfo->CURRENT_GUN = 0;
	}

}

//--mk, 121094 -- // ----------------------------------------------------------------------------------
//--mk, 121094 -- void spin_robot(CObject *pRobot, CFixVector *vCollision)
//--mk, 121094 -- {
//--mk, 121094 -- 	if (vCollision->p.x != 3) {
//--mk, 121094 -- 		pRobot->physInfo.rotVel.v.c.x = 0x1235;
//--mk, 121094 -- 		pRobot->physInfo.rotVel.v.c.y = 0x2336;
//--mk, 121094 -- 		pRobot->physInfo.rotVel.v.c.z = 0x3737;
//--mk, 121094 -- 	}
//--mk, 121094 --
//--mk, 121094 -- }

//	-----------------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	int32_t	i;

	for (i=0; i<D1_MAX_AI_CLOAK_INFO; i++) {
		gameData.ai.cloakInfo [i].vLastPos = OBJPOS (gameData.objData.pConsole)->vPos;
		gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	}

	//	Make work for control centers.
	gameData.ai.target.vBelievedPos = gameData.ai.cloakInfo [0].vLastPos;

}

//	-----------------------------------------------------------------------------------
//	Returns false if awareness is considered too puny to add, else returns true.
int32_t add_awareness_event(CObject *pObj, int32_t type)
{
	//	If pPlayer cloaked and hit a pRobot, then increase awareness
	if ((type == D1_PA_WEAPON_ROBOT_COLLISION) || (type == D1_PA_WEAPON_WALL_COLLISION) || (type == D1_PA_PLAYER_COLLISION))
		ai_do_cloak_stuff();

	if (gameData.ai.nAwarenessEvents < D1_MAX_AWARENESS_EVENTS) {
		if ((type == D1_PA_WEAPON_WALL_COLLISION) || (type == D1_PA_WEAPON_ROBOT_COLLISION))
			if (pObj->info.nId == VULCAN_ID)
				if (RandShort () > 3276)
					return 0;		//	For vulcan cannon, only about 1/10 actually cause awareness

		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nSegment = pObj->info.nSegment;
		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].pos = pObj->info.position.vPos;
		gameData.ai.awarenessEvents [gameData.ai.nAwarenessEvents].nType = type;
		gameData.ai.nAwarenessEvents++;
	} else
		Assert(0);		// Hey -- Overflowed gameData.ai.awarenessEvents, make more or something
							// This just gets ignored, so you can just continue.
	return 1;

}

#if DBG
int32_t	D1_AI_dump_enable = 0;

FILE *D1_AI_dump_file = NULL;

char	D1_AI_error_message [128] = "";

// ----------------------------------------------------------------------------------
void dump_ai_objects_all()
{
#if PARALLAX
	int32_t	nObject;
	int32_t	total=0;
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

	for (nObject=0; nObject <= gameData.objData.nLastObject; nObject++) {
		CObject		*pObj = OBJECT (nObject);
		tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
		tAILocalInfo		*pLocalInfo = &gameData.ai.localInfo [nObject];
		fix			dist_to_player;

		dist_to_player = vm_vec_dist(&pObj->info.position.vPos, &OBJPOS (gameData.objData.pConsole)->vPos);

		if (pObj->info.controlType == CT_AI) {
			fprintf(D1_AI_dump_file, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n",
				nObject, pObj->info.nSegment, X2F(dist_to_player), mode_text [pLocalInfo->mode], behavior_text [pStaticInfo->behavior-0x80], pStaticInfo->nHideIndex, pStaticInfo->nPathLength);
			if (pStaticInfo->nPathLength)
				total += pStaticInfo->nPathLength;
		}
	}

	fprintf(D1_AI_dump_file, "Total path length = %4i\n", total);
#endif

}

// ----------------------------------------------------------------------------------
void force_dump_ai_objects_all(char *msg)
{
	int32_t	tsave;

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
