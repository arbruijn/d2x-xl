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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "inferno.h"
#include "error.h"
#include "gameseg.h"
#include "multibot.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "string.h"
//#define _DEBUG
#ifdef _DEBUG
#include <time.h>
#endif

// ---------- John: These variables must be saved as part of gamesave. --------

typedef struct tAIStateInfo {
	short			nObject;
	short			nObjRef;
	tRobotInfo	*botInfoP;
	tAIStatic	*aiP;
	tAILocal		*ailP;
	vmsVector	vVisPos;
	int			nPrevVisibility;
	int			bVisAndVecComputed;
	int			bHaveGunPos;
	int			bMultiGame;
	int			nNewGoalState;
	} tAIStateInfo;


// --------------------------------------------------------------------------------------------------------------------

static fix xAwarenessTimes [2][3] = {
	{F1_0 * 2, F1_0 * 3, F1_0 * 5},
	{F1_0 * 4, F1_0 * 6, F1_0 * 10}
	};

tBossProps bossProps [2][NUM_D2_BOSSES] = {
	{
	{1,0,1,0,0,0,0,0},
	{1,1,1,0,0,0,0,0},
	{1,0,1,1,0,1,0,0},
	{1,0,1,1,0,1,0,0},
	{1,0,0,1,0,0,1,0},
	{1,0,1,1,0,0,1,1},
	{1,0,1,0,0,0,1,0},
	{1,0,1,1,0,0,0,1}
	},
	{
	{1,0,1,0,0,0,0,0},
	{1,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,0,0},
	{1,0,1,1,0,1,0,0},
	{1,0,0,1,0,0,1,0},
	{1,0,1,1,0,0,1,1},
	{1,0,1,0,0,0,1,0},
	{1,0,1,1,0,0,0,1}
	}
};

// These globals are set by a call to FindVectorIntersection, which is a slow routine,
// so we don't want to call it again (for this tObject) unless we have to.

#ifdef _DEBUG
// Index into this array with ailP->mode
char *pszAIMode [18] = {
	"STILL",
	"WANDER",
	"FOL_PATH",
	"CHASE_OBJ",
	"RUN_FROM",
	"BEHIND",
	"FOL_PATH2",
	"OPEN_DOOR",
	"GOTO_PLR",
	"GOTO_OBJ",
	"SN_ATT",
	"SN_FIRE",
	"SN_RETR",
	"SN_RTBK",
	"SN_WAIT",
	"TH_ATTACK",
	"TH_RETREAT",
	"TH_WAIT",
};

//	Index into this array with aiP->behavior
char pszAIBehavior [6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

// Index into this array with aiP->GOAL_STATE or aiP->CURRENT_STATE
char pszAIState [8][5] = {
	"NONE",
	"REST",
	"SRCH",
	"LOCK",
	"FLIN",
	"FIRE",
	"RECO",
	"ERR ",
};


#endif

// Current state indicates where the robot current is, or has just done.
// Transition table between states for an AI tObject.
// First dimension is tTrigger event.
// Second dimension is current state.
// Third dimension is goal state.
// Result is new goal state.
// ERR_ means something impossible has happened.
sbyte aiTransitionTable [AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
	{
		// Event = AIE_FIRE, a nearby tObject fired
		// none     rest      srch      lock      flin      fire      recover     // CURRENT is rows, GOAL is columns
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // none
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // rest
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // search
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // lock
		{ AIS_ERROR, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECOVER }, // flinch
		{ AIS_ERROR, AIS_FIRE, AIS_FIRE, AIS_FIRE, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // fire
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_FIRE }  // recoil
	},

	// Event = AIE_HIT, a nearby tObject was hit (or a tWall was hit)
	{
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_COLLIDE, tPlayer collided with robot
	{
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_LOCK, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_RETF, robot fires back after having been hit by player
	{
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_LOCK, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECOVER},
		{ AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_HURT, tPlayer hurt robot (by firing at and hitting it)
	// Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH},
		{ AIS_ERROR, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH, AIS_FLINCH}
	}
};


// --------------------------------------------------------------------------------------------------------------------

int AINothingHandler (tObject *objP, tAIStateInfo *siP)
{
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIMGotoPlayerHandler1 (tObject *objP, tAIStateInfo *siP)
{
MoveTowardsSegmentCenter (objP);
CreatePathToPlayer (objP, 100, 1);
return 0;
}


int AIMGotoObjectHandler1 (tObject *objP, tAIStateInfo *siP)
{
gameData.escort.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
return 0;
}


int AIMChaseObjectHandler1 (tObject *objP, tAIStateInfo *siP)
{
CreatePathToPlayer (objP, 4 + gameData.ai.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
return 0;
}


int AIMIdlingHandler1 (tObject *objP, tAIStateInfo *siP)
{
if (siP->botInfoP->attackType)
	MoveTowardsSegmentCenter (objP);
else if ((siP->aiP->behavior != AIB_STILL) && (siP->aiP->behavior != AIB_STATION) && (siP->aiP->behavior != AIB_FOLLOW))    // Behavior is still, so don't follow path.
	AttemptToResumePath (objP);
return 0;
}


int AIMFollowPathHandler1 (tObject *objP, tAIStateInfo *siP)
{
if (siP->bMultiGame)
	siP->ailP->mode = AIM_IDLING;
else
	AttemptToResumePath (objP);
return 0;
}


int AIMRunFromObjectHandler1 (tObject *objP, tAIStateInfo *siP)
{
MoveTowardsSegmentCenter (objP);
objP->mType.physInfo.velocity.setZero();
CreateNSegmentPath (objP, 5, -1);
siP->ailP->mode = AIM_RUN_FROM_OBJECT;
return 0;
}


int AIMBehindHandler1 (tObject *objP, tAIStateInfo *siP)
{
MoveTowardsSegmentCenter (objP);
objP->mType.physInfo.velocity.setZero();
return 0;
}


int AIMOpenDoorHandler1 (tObject *objP, tAIStateInfo *siP)
{
CreateNSegmentPathToDoor (objP, 5, -1);
return 0;
}


// --------------------------------------------------------------------------------------------------------------------

int AIMChaseObjectHandler2 (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic *aiP = siP->aiP;
	fix circleDistance = siP->botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size;

// Green guy doesn't get his circle distance boosted, else he might never attack.
if (siP->botInfoP->attackType != 1)
	circleDistance += (siP->nObject & 0xf) * F1_0 / 2;
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, -MAX_CHASE_DIST);
if ((gameData.ai.nPlayerVisibility < 2) && (siP->nPrevVisibility == 2)) {
	if (!AIMultiplayerAwareness (objP, 53)) {
		if (AIMaybeDoActualFiringStuff (objP, aiP))
			AIDoActualFiringStuff (objP, aiP, siP->ailP, siP->botInfoP, aiP->CURRENT_GUN);
		return 1;
		}
	CreatePathToPlayer (objP, 8, 1);
	AIMultiSendRobotPos (siP->nObject, -1);
	}
else if (!gameData.ai.nPlayerVisibility && (gameData.ai.xDistToPlayer > MAX_CHASE_DIST) && !siP->bMultiGame) {
	// If pretty far from the tPlayer, tPlayer cannot be seen
	// (obstructed) and in chase mode, switch to follow path mode.
	// This has one desirable benefit of avoiding physics retries.
	if (aiP->behavior == AIB_STATION) {
		siP->ailP->nGoalSegment = aiP->nHideSegment;
		CreatePathToStation (objP, 15);
		} // -- this looks like a dumb thing to do...robots following paths far away from you!else CreateNSegmentPath (objP, 5, -1);
	return 0;
	}

if ((aiP->CURRENT_STATE == AIS_REST) && (aiP->GOAL_STATE == AIS_REST)) {
	if (gameData.ai.nPlayerVisibility &&
			(d_rand () < gameData.time.xFrame * gameData.ai.nPlayerVisibility) &&
			(gameData.ai.xDistToPlayer / 256 < d_rand () * gameData.ai.nPlayerVisibility)) {
		aiP->GOAL_STATE = AIS_SEARCH;
		aiP->CURRENT_STATE = AIS_SEARCH;
		}
	else
		AIIdleAnimation (objP);
	}

if (gameData.time.xGame - siP->ailP->timePlayerSeen > CHASE_TIME_LENGTH) {
	if (siP->bMultiGame)
		if (!gameData.ai.nPlayerVisibility && (gameData.ai.xDistToPlayer > F1_0 * 70)) {
			siP->ailP->mode = AIM_IDLING;
			return 1;
			}
	if (!AIMultiplayerAwareness (objP, 64)) {
		if (AIMaybeDoActualFiringStuff (objP, aiP))
			AIDoActualFiringStuff (objP, aiP, siP->ailP, siP->botInfoP, aiP->CURRENT_GUN);
		return 1;
		}
	// -- bad idea, robots charge tPlayer they've never seen!-- CreatePathToPlayer (objP, 10, 1);
	// -- bad idea, robots charge tPlayer they've never seen!-- AIMultiSendRobotPos (siP->nObject, -1);
	}
else if ((aiP->CURRENT_STATE != AIS_REST) && (aiP->GOAL_STATE != AIS_REST)) {
	if (!AIMultiplayerAwareness (objP, 70)) {
		if (AIMaybeDoActualFiringStuff (objP, aiP))
			AIDoActualFiringStuff (objP, aiP, siP->ailP, siP->botInfoP, aiP->CURRENT_GUN);
		return 1;
		}
	AIMoveRelativeToPlayer (objP, siP->ailP, gameData.ai.xDistToPlayer, &gameData.ai.vVecToPlayer, circleDistance, 0, gameData.ai.nPlayerVisibility);
	if ((siP->nObjRef & 1) && ((aiP->GOAL_STATE == AIS_SEARCH) || (aiP->GOAL_STATE == AIS_LOCK))) {
		if (gameData.ai.nPlayerVisibility) // == 2)
			AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
		}
	if (gameData.ai.bEvaded) {
		AIMultiSendRobotPos (siP->nObject, 1);
		gameData.ai.bEvaded = 0;
		}
	else
		AIMultiSendRobotPos (siP->nObject, -1);
	DoFiringStuff (objP, gameData.ai.nPlayerVisibility, &gameData.ai.vVecToPlayer);
	}
return 0;
}


int AIMRunFromObjectHandler2 (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic *aiP = siP->aiP;

ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
if (gameData.ai.nPlayerVisibility) {
	if (siP->ailP->playerAwarenessType == 0)
		siP->ailP->playerAwarenessType = PA_RETURN_FIRE;
	}
// If in multiplayer, only do if tPlayer visible.  If not multiplayer, do always.
if (!(siP->bMultiGame) || gameData.ai.nPlayerVisibility)
	if (AIMultiplayerAwareness (objP, 75)) {
		AIFollowPath (objP, gameData.ai.nPlayerVisibility, siP->nPrevVisibility, &gameData.ai.vVecToPlayer);
		AIMultiSendRobotPos (siP->nObject, -1);
		}
if (aiP->GOAL_STATE != AIS_FLINCH)
	aiP->GOAL_STATE = AIS_LOCK;
else if (aiP->CURRENT_STATE == AIS_FLINCH)
	aiP->GOAL_STATE = AIS_LOCK;

// Bad to let run_from robot fire at tPlayer because it will cause a war in which it turns towards the tPlayer
// to fire and then towards its goal to move. DoFiringStuff (objP, gameData.ai.nPlayerVisibility, &gameData.ai.vVecToPlayer);
// Instead, do this:
// (Note, only drop if tPlayer is visible.  This prevents the bombs from being a giveaway, and also ensures that
// the robot is moving while it is dropping.  Also means fewer will be dropped.)
if ((siP->ailP->nextPrimaryFire <= 0) && (gameData.ai.nPlayerVisibility)) {
	vmsVector fire_vec, fire_pos;

	if (!AIMultiplayerAwareness (objP, 75))
		return 1;
	fire_vec = objP->position.mOrient[FVEC];
	fire_vec = -fire_vec;
	fire_pos = objP->position.vPos + fire_vec;
	CreateNewLaserEasy (&fire_vec, &fire_pos, OBJ_IDX (objP), (aiP->SUB_FLAGS & SUB_FLAGS_SPROX) ? ROBOT_SMARTMINE_ID : PROXMINE_ID, 1);
	siP->ailP->nextPrimaryFire = (F1_0/2)* (NDL+5 - gameStates.app.nDifficultyLevel);      // Drop a proximity bomb every 5 seconds.
	if (siP->bMultiGame) {
		AIMultiSendRobotPos (OBJ_IDX (objP), -1);
		MultiSendRobotFire (OBJ_IDX (objP), (aiP->SUB_FLAGS & SUB_FLAGS_SPROX) ? -2 : -1, &fire_vec);
		}
	}
return 0;
}


int AIMGotoHandler2 (tObject *objP, tAIStateInfo *siP)
{
AIFollowPath (objP, 2, siP->nPrevVisibility, &gameData.ai.vVecToPlayer);    // Follows path as if tPlayer can see robot.
AIMultiSendRobotPos (siP->nObject, -1);
return 0;
}


int AIMFollowPathHandler2 (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic *aiP = siP->aiP;
	int angerLevel = 65;

if (aiP->behavior == AIB_STATION)
	if (gameData.ai.pointSegs [aiP->nHideIndex + aiP->nPathLength - 1].nSegment == aiP->nHideSegment) {
		angerLevel = 64;
	}
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (gameData.app.nGameMode & (GM_MODEM | GM_SERIAL))
	if (!gameData.ai.nPlayerVisibility && (gameData.ai.xDistToPlayer > F1_0 * 70)) {
		siP->ailP->mode = AIM_IDLING;
		return 1;
		}
if (!AIMultiplayerAwareness (objP, angerLevel)) {
	if (AIMaybeDoActualFiringStuff (objP, aiP)) {
		ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, F1_0 * 10000);
		AIDoActualFiringStuff (objP, aiP, siP->ailP, siP->botInfoP, aiP->CURRENT_GUN);
		}
	return 1;
	}
if (!gameData.ai.nPlayerVisibility &&
	 ((siP->ailP->playerAwarenessType < PA_RETURN_FIRE) || (siP->ailP->mode != AIM_FOLLOW_PATH) ||
	 (siP->ailP->nGoalSegment != gameData.ai.nBelievedPlayerSeg))) {
	siP->ailP->mode = AIM_IDLING;
	return 1;
	}
AIFollowPath (objP, gameData.ai.nPlayerVisibility, siP->nPrevVisibility, &gameData.ai.vVecToPlayer);
if (aiP->GOAL_STATE != AIS_FLINCH)
	aiP->GOAL_STATE = AIS_LOCK;
else if (aiP->CURRENT_STATE == AIS_FLINCH)
	aiP->GOAL_STATE = AIS_LOCK;
if (aiP->behavior != AIB_RUN_FROM)
	DoFiringStuff (objP, gameData.ai.nPlayerVisibility, &gameData.ai.vVecToPlayer);
if ((gameData.ai.nPlayerVisibility == 2) &&
		(aiP->behavior != AIB_STILL) && (aiP->behavior != AIB_SNIPE) && (aiP->behavior != AIB_FOLLOW) && (aiP->behavior != AIB_RUN_FROM) && (objP->id != ROBOT_BRAIN) &&
		!(siP->botInfoP->companion || siP->botInfoP->thief)) {
	if (siP->botInfoP->attackType == 0)
		siP->ailP->mode = AIM_CHASE_OBJECT;
	// This should not just be distance based, but also time-since-tPlayer-seen based.
	}
else if ((gameData.ai.xDistToPlayer > MAX_PURSUIT_DIST (siP->botInfoP))
			&& (gameData.time.xGame - siP->ailP->timePlayerSeen > ((F1_0 / 2) * (gameStates.app.nDifficultyLevel + siP->botInfoP->pursuit)))
			&& (gameData.ai.nPlayerVisibility == 0)
			&& (aiP->behavior == AIB_NORMAL)
			&& (siP->ailP->mode == AIM_FOLLOW_PATH)) {
	siP->ailP->mode = AIM_IDLING;
	aiP->nHideIndex = -1;
	aiP->nPathLength = 0;
	}
AIMultiSendRobotPos (siP->nObject, -1);
return 0;
}


int AIMBehindHandler2 (tObject *objP, tAIStateInfo *siP)
{
if (!AIMultiplayerAwareness (objP, 71)) {
	if (AIMaybeDoActualFiringStuff (objP, siP->aiP)) {
		ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
		AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
		}
	return 1;
	}
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
if (gameData.ai.nPlayerVisibility == 2) {
	// Get behind the player.
	// Method:
	// If gameData.ai.vVecToPlayer dot player_rear_vector > 0, behind is goal.
	// Else choose goal with larger dot from left, right.
	vmsVector  goal_point, vGoal, vec_to_goal, vRand;
	fix         dot;

	dot = vmsVector::dot(OBJPOS (gameData.objs.console)->mOrient[FVEC], gameData.ai.vVecToPlayer);
	if (dot > 0) {          // Remember, we're interested in the rear vector dot being < 0.
		vGoal = OBJPOS (gameData.objs.console)->mOrient[FVEC];
		vGoal = -vGoal;
		}
	else {
		fix dot;
		dot = vmsVector::dot(OBJPOS (gameData.objs.console)->mOrient[RVEC], gameData.ai.vVecToPlayer);
		vGoal = OBJPOS (gameData.objs.console)->mOrient[RVEC];
		if (dot > 0) {
			vGoal = -vGoal;
			}
		else
			;
		}
	vGoal *= (2* (gameData.objs.console->size + objP->size + (((siP->nObject*4 + gameData.app.nFrameCount) & 63) << 12)));
	goal_point = OBJPOS (gameData.objs.console)->vPos + vGoal;
	vRand = vmsVector::Random();
	goal_point += vRand * (F1_0 * 8);
	vec_to_goal = goal_point - objP->position.vPos;
	vmsVector::normalize(vec_to_goal);
	MoveTowardsVector (objP, &vec_to_goal, 0);
	AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
	AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
	}
if (siP->aiP->GOAL_STATE != AIS_FLINCH)
	siP->aiP->GOAL_STATE = AIS_LOCK;
else if (siP->aiP->CURRENT_STATE == AIS_FLINCH)
	siP->aiP->GOAL_STATE = AIS_LOCK;
AIMultiSendRobotPos (siP->nObject, -1);
return 0;
}


int AIMIdlingHandler2 (tObject *objP, tAIStateInfo *siP)
{
if ((gameData.ai.xDistToPlayer < MAX_WAKEUP_DIST) || (siP->ailP->playerAwarenessType >= PA_RETURN_FIRE - 1)) {
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);

	// turn towards vector if visible this time or last time, or rand
	// new!
	if ((gameData.ai.nPlayerVisibility == 2) || (siP->nPrevVisibility == 2)) { // -- MK, 06/09/95:  || ((d_rand () > 0x4000) && !(siP->bMultiGame))) {
		if (!AIMultiplayerAwareness (objP, 71)) {
			if (AIMaybeDoActualFiringStuff (objP, siP->aiP))
				AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
			return 1;
			}
		AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
		AIMultiSendRobotPos (siP->nObject, -1);
		}
	DoFiringStuff (objP, gameData.ai.nPlayerVisibility, &gameData.ai.vVecToPlayer);
	if (gameData.ai.nPlayerVisibility == 2) {  // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
		if (siP->botInfoP->attackType == 1) {
			siP->aiP->behavior = AIB_NORMAL;
			if (!AIMultiplayerAwareness (objP, 80)) {
				if (AIMaybeDoActualFiringStuff (objP, siP->aiP))
					AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
				return 1;
				}
			AIMoveRelativeToPlayer (objP, siP->ailP, gameData.ai.xDistToPlayer, &gameData.ai.vVecToPlayer, 0, 0, gameData.ai.nPlayerVisibility);
			if (gameData.ai.bEvaded) {
				AIMultiSendRobotPos (siP->nObject, 1);
				gameData.ai.bEvaded = 0;
				}
			else
				AIMultiSendRobotPos (siP->nObject, -1);
			}
		else {
			// Robots in hover mode are allowed to evade at half normal speed.
			if (!AIMultiplayerAwareness (objP, 81)) {
				if (AIMaybeDoActualFiringStuff (objP, siP->aiP))
					AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
				return 1;
				}
			if (siP->aiP->behavior != AIB_STILL)
				AIMoveRelativeToPlayer (objP, siP->ailP, gameData.ai.xDistToPlayer, &gameData.ai.vVecToPlayer, 0, 1, gameData.ai.nPlayerVisibility);
			else
				gameData.ai.bEvaded = 0;
			if (gameData.ai.bEvaded) {
				AIMultiSendRobotPos (siP->nObject, -1);
				gameData.ai.bEvaded = 0;
				}
			else
				AIMultiSendRobotPos (siP->nObject, -1);
			}
		}
	else if ((objP->nSegment != siP->aiP->nHideSegment) && (gameData.ai.xDistToPlayer > MAX_CHASE_DIST) && !siP->bMultiGame) {
		// If pretty far from the tPlayer, tPlayer cannot be
		// seen (obstructed) and in chase mode, switch to
		// follow path mode.
		// This has one desirable benefit of avoiding physics retries.
		if (siP->aiP->behavior == AIB_STATION) {
			siP->ailP->nGoalSegment = siP->aiP->nHideSegment;
			CreatePathToStation (objP, 15);
			}
		}
	}
return 0;
}


int AIMOpenDoorHandler2 (tObject *objP, tAIStateInfo *siP)
{
	vmsVector vCenter, vGoal;

Assert (objP->id == ROBOT_BRAIN);     // Make sure this guy is allowed to be in this mode.
if (!AIMultiplayerAwareness (objP, 62))
	return 1;
COMPUTE_SIDE_CENTER (&vCenter, gameData.segs.segments + objP->nSegment, siP->aiP->GOALSIDE);
vGoal = vCenter - objP->position.vPos;
vmsVector::normalize(vGoal);
AITurnTowardsVector (&vGoal, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
MoveTowardsVector (objP, &vGoal, 0);
AIMultiSendRobotPos (siP->nObject, -1);
return 0;
}


int AIMSnipeHandler2 (tObject *objP, tAIStateInfo *siP)
{
if (AIMultiplayerAwareness (objP, 53)) {
	AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
	if (siP->botInfoP->thief)
		AIMoveRelativeToPlayer (objP, siP->ailP, gameData.ai.xDistToPlayer, &gameData.ai.vVecToPlayer, 0, 0, gameData.ai.nPlayerVisibility);
	}
return 0;
}


int AIMDefaultHandler2 (tObject *objP, tAIStateInfo *siP)
{
siP->ailP->mode = AIM_CHASE_OBJECT;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AISNoneHandler1 (tObject *objP, tAIStateInfo *siP)
{
	fix	dot;

ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);
dot = vmsVector::dot(objP->position.mOrient[FVEC], gameData.ai.vVecToPlayer);
if ((dot >= F1_0/2) && (siP->aiP->GOAL_STATE == AIS_REST))
	siP->aiP->GOAL_STATE = AIS_SEARCH;
return 0;
}


int AISRestHandler1 (tObject *objP, tAIStateInfo *siP)
{
if (siP->aiP->GOAL_STATE == AIS_REST) {
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);
	if (gameData.ai.nPlayerVisibility && ReadyToFire (siP->botInfoP, siP->ailP))
		siP->aiP->GOAL_STATE = AIS_FIRE;
	}
return 0;
}


int AISSearchHandler1 (tObject *objP, tAIStateInfo *siP)
{
if (!AIMultiplayerAwareness (objP, 60))
	return 1;
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (gameData.ai.nPlayerVisibility == 2) {
	AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
	AIMultiSendRobotPos (siP->nObject, -1);
	}
return 0;
}


int AISLockHandler1 (tObject *objP, tAIStateInfo *siP)
{
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (!siP->bMultiGame || (gameData.ai.nPlayerVisibility)) {
	if (!AIMultiplayerAwareness (objP, 68))
		return 1;
	if (gameData.ai.nPlayerVisibility == 2) {   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
		AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
		AIMultiSendRobotPos (siP->nObject, -1);
		}
	}
return 0;
}


int AISFireHandler1 (tObject *objP, tAIStateInfo *siP)
{
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
if (gameData.ai.nPlayerVisibility == 2) {
	if (!AIMultiplayerAwareness (objP, (ROBOT_FIRE_AGITATION - 1))) {
		if (siP->bMultiGame) {
			AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
			return 1;
			}
		}
	AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
	AIMultiSendRobotPos (siP->nObject, -1);
	}
// Fire at tPlayer, if appropriate.
if (!siP->bHaveGunPos) {
	if (siP->ailP->nextPrimaryFire <= 0)
		siP->bHaveGunPos = CalcGunPoint (&gameData.ai.vGunPoint, objP, siP->aiP->CURRENT_GUN);
	else
		siP->bHaveGunPos = CalcGunPoint (&gameData.ai.vGunPoint, objP, (ROBOTINFO (objP->id).nGuns == 1) ? 0 : !siP->aiP->CURRENT_GUN);
	siP->vVisPos = gameData.ai.vGunPoint;
	}
AIDoActualFiringStuff (objP, siP->aiP, siP->ailP, siP->botInfoP, siP->aiP->CURRENT_GUN);
siP->bHaveGunPos = 0;
return 0;
}


int AISRecoverHandler1 (tObject *objP, tAIStateInfo *siP)
{
if (siP->nObjRef & 3)
	return 0;
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (gameData.ai.nPlayerVisibility != 2)
	return 0;
if (!AIMultiplayerAwareness (objP, 69))
	return 1;
AITurnTowardsVector (&gameData.ai.vVecToPlayer, objP, siP->botInfoP->turnTime [gameStates.app.nDifficultyLevel]);
AIMultiSendRobotPos (siP->nObject, -1);
return 0;
}


int AISDefaultHandler1 (tObject *objP, tAIStateInfo *siP)
{
siP->aiP->GOAL_STATE =
siP->aiP->CURRENT_STATE = AIS_REST;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

#if defined(_WIN32) && defined(RELEASE)
typedef int (__fastcall * pAIHandler) (tObject *objP, tAIStateInfo *);
#else
typedef int (* pAIHandler) (tObject *objP, tAIStateInfo *);
#endif

static pAIHandler aimHandler1 [] = {
	AIMIdlingHandler1, AINothingHandler, AIMFollowPathHandler1, AIMChaseObjectHandler1, AIMRunFromObjectHandler1,
	AIMBehindHandler1, AINothingHandler, AIMOpenDoorHandler1, AIMGotoPlayerHandler1, AIMGotoObjectHandler1};

static pAIHandler aimHandler2 [] = {
	AIMIdlingHandler2, AINothingHandler, AIMFollowPathHandler2, AIMChaseObjectHandler2, AIMRunFromObjectHandler2,
	AIMBehindHandler2, AINothingHandler, AIMOpenDoorHandler2, AIMGotoHandler2, AIMGotoHandler2,
	AIMSnipeHandler2, AIMSnipeHandler2, AINothingHandler, AIMSnipeHandler2, AINothingHandler,
	AINothingHandler,	AINothingHandler, AINothingHandler};

static pAIHandler aisHandler1 [] = {
	AISNoneHandler1, AISRestHandler1, AISSearchHandler1, AISLockHandler1,
	AINothingHandler, AISFireHandler1, AISRecoverHandler1, AINothingHandler};

// --------------------------------------------------------------------------------------------------------------------

int AIBrainHandler (tObject *objP, tAIStateInfo *siP)
{
// Robots function nicely if behavior is gameData.matCens.fuelCenters.  This
// means they won't move until they can see the tPlayer, at
// which time they will start wandering about opening doors.
if (objP->id != ROBOT_BRAIN)
	return 0;
if (OBJSEG (gameData.objs.console) == objP->nSegment) {
	if (!AIMultiplayerAwareness (objP, 97))
		return 1;
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, F1_0 * 10000);
	MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 0);
	AIMultiSendRobotPos (siP->nObject, -1);
	}
else if (siP->ailP->mode != AIM_IDLING) {
	int r = OpenableDoorsInSegment (objP->nSegment);
	if (r != -1) {
		siP->ailP->mode = AIM_OPEN_DOOR;
		siP->aiP->GOALSIDE = r;
		}
	else if (siP->ailP->mode != AIM_FOLLOW_PATH) {
		if (!AIMultiplayerAwareness (objP, 50))
			return 1;
		CreateNSegmentPathToDoor (objP, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
		AIMultiSendRobotPos (siP->nObject, -1);
		}
	if (siP->ailP->nextActionTime < 0) {
		ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, F1_0 * 10000);
		if (gameData.ai.nPlayerVisibility) {
			MakeNearbyRobotSnipe ();
			siP->ailP->nextActionTime = (NDL - gameStates.app.nDifficultyLevel) * 2*F1_0;
			}
		}
	}
else {
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, F1_0 * 10000);
	if (gameData.ai.nPlayerVisibility) {
		if (!AIMultiplayerAwareness (objP, 50))
			return 1;
		CreateNSegmentPathToDoor (objP, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
		AIMultiSendRobotPos (siP->nObject, -1);
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AISnipeHandler (tObject *objP, tAIStateInfo *siP)
{
if (siP->aiP->behavior == AIB_SNIPE) {
	if (siP->bMultiGame && !siP->botInfoP->thief) {
		siP->aiP->behavior = AIB_NORMAL;
		siP->ailP->mode = AIM_CHASE_OBJECT;
		return 1;
		}
	if (!(siP->nObjRef & 3) || siP->nPrevVisibility) {
		ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
		// If this sniper is in still mode, if he was hit or can see tPlayer, switch to snipe mode.
		if (siP->ailP->mode == AIM_IDLING)
			if (gameData.ai.nPlayerVisibility || (siP->ailP->playerAwarenessType == PA_RETURN_FIRE))
				siP->ailP->mode = AIM_SNIPE_ATTACK;
		if (!siP->botInfoP->thief && (siP->ailP->mode != AIM_IDLING))
			DoSnipeFrame (objP);
		}
	else if (!siP->botInfoP->thief && !siP->botInfoP->companion)
		return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIBuddyHandler (tObject *objP, tAIStateInfo *siP)
{
if (siP->botInfoP->companion) {
		tAIStatic	*aiP = siP->aiP;

	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, F1_0 * 10000);
	DoEscortFrame (objP, gameData.ai.xDistToPlayer, gameData.ai.nPlayerVisibility);

	if (objP->cType.aiInfo.nDangerLaser != -1) {
		tObject *dObjP = &OBJECTS [objP->cType.aiInfo.nDangerLaser];

		if ((dObjP->nType == OBJ_WEAPON) && (dObjP->nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
			fix circleDistance = siP->botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.console->size;
			AIMoveRelativeToPlayer (objP, siP->ailP, gameData.ai.xDistToPlayer, &gameData.ai.vVecToPlayer, circleDistance, 1, gameData.ai.nPlayerVisibility);
			}
		}

	if (ReadyToFire (siP->botInfoP, siP->ailP)) {
		int bDoStuff = 0;

		if (OpenableDoorsInSegment (objP->nSegment) != -1)
			bDoStuff = 1;
		else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aiP->nHideIndex + aiP->nCurPathIndex + aiP->PATH_DIR].nSegment) != -1)
			bDoStuff = 1;
		else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aiP->nHideIndex + aiP->nCurPathIndex + 2*aiP->PATH_DIR].nSegment) != -1)
			bDoStuff = 1;
		else if ((siP->ailP->mode == AIM_GOTO_PLAYER) && (gameData.ai.xDistToPlayer < 3*MIN_ESCORT_DISTANCE/2) ) {
			bDoStuff = 1;
			}
		else
			;

		if (bDoStuff && (vmsVector::dot(objP->position.mOrient[FVEC], gameData.ai.vVecToPlayer) < F1_0 / 2)) {
			CreateNewLaserEasy (&objP->position.mOrient[FVEC], &objP->position.vPos, OBJ_IDX (objP), FLARE_ID, 1);
			siP->ailP->nextPrimaryFire = F1_0/2;
			if (!gameData.escort.bMayTalk) // If buddy not talking, make him fire flares less often.
				siP->ailP->nextPrimaryFire += d_rand ()*4;
			}
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIThiefHandler (tObject *objP, tAIStateInfo *siP)
{
if (siP->botInfoP->thief) {
		tAIStatic	*aiP = siP->aiP;

	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
	DoThiefFrame (objP);
	if (ReadyToFire (siP->botInfoP, siP->ailP)) {
		int bDoStuff = 0;
		if (OpenableDoorsInSegment (objP->nSegment) != -1)
			bDoStuff = 1;
		else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aiP->nHideIndex + aiP->nCurPathIndex + aiP->PATH_DIR].nSegment) != -1)
			bDoStuff = 1;
		else if (OpenableDoorsInSegment ((short) gameData.ai.pointSegs [aiP->nHideIndex + aiP->nCurPathIndex + 2*aiP->PATH_DIR].nSegment) != -1)
			bDoStuff = 1;
		if (bDoStuff) {
			// @mk, 05/08/95: Firing flare from center of tObject, this is dumb...
			CreateNewLaserEasy (&objP->position.mOrient[FVEC], &objP->position.vPos, OBJ_IDX (objP), FLARE_ID, 1);
			siP->ailP->nextPrimaryFire = F1_0 / 2;
			if (gameData.thief.nStolenItem == 0)     // If never stolen an item, fire flares less often (bad: gameData.thief.nStolenItem wraps, but big deal)
				siP->ailP->nextPrimaryFire += d_rand ()*4;
			}
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Make sure that if this guy got hit or bumped, then he's chasing player.

int AIBumpHandler (tObject *objP, tAIStateInfo *siP)
{
#ifdef _DEBUG
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (siP->ailP->playerAwarenessType >= PA_PLAYER_COLLISION) {
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
	if (gameData.ai.nPlayerVisibility == 1) // Only increase visibility if unobstructed, else claw guys attack through doors.
		gameData.ai.nPlayerVisibility = 2;
	}
else if (!(siP->nObjRef & 3) && !siP->nPrevVisibility && (gameData.ai.xDistToPlayer < MAX_WAKEUP_DIST)) {
	fix rval = d_rand ();
	fix sval = (gameData.ai.xDistToPlayer * (gameStates.app.nDifficultyLevel + 1)) / 64;
	if ((FixMul (rval, sval) < gameData.time.xFrame) || HeadlightIsOn (-1)) {
		siP->ailP->playerAwarenessType = PA_PLAYER_COLLISION;
		siP->ailP->playerAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][1];
		ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
		if (gameData.ai.nPlayerVisibility == 1)
			gameData.ai.nPlayerVisibility = 2;
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIAnimationHandler (tObject *objP, tAIStateInfo *siP)
{
// Note: Should only do these two function calls for objects which animate
if (gameData.ai.bEnableAnimation && (gameData.ai.xDistToPlayer < MAX_WAKEUP_DIST / 2)) { // && !siP->bMultiGame)) {
	gameData.ai.bObjAnimates = DoSillyAnimation (objP);
	if (gameData.ai.bObjAnimates)
		AIFrameAnimation (objP);
	}
else {
	// If Object is supposed to animate, but we don't let it animate due to distance, then
	// we must change its state, else it will never update.
	siP->aiP->CURRENT_STATE = siP->aiP->GOAL_STATE;
	gameData.ai.bObjAnimates = 0;        // If we're not doing the animation, then should pretend it doesn't animate.
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIBossHandler (tObject *objP, tAIStateInfo *siP)
{
if (siP->botInfoP->bossFlag) {
		int	pv;
		fix	dtp = gameData.ai.xDistToPlayer / 4;

	if (siP->aiP->GOAL_STATE == AIS_FLINCH)
		siP->aiP->GOAL_STATE = AIS_FIRE;
	if (siP->aiP->CURRENT_STATE == AIS_FLINCH)
		siP->aiP->CURRENT_STATE = AIS_FIRE;
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
	pv = gameData.ai.nPlayerVisibility;
	// If tPlayer cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		pv = 0;
		dtp = vmsVector::dist(OBJPOS (gameData.objs.console)->vPos, objP->position.vPos) / 4;
		}
	DoBossStuff (objP, pv);
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIStationaryHandler (tObject *objP, tAIStateInfo *siP)
{
// Time-slice, don't process all the time, purely an efficiency hack.
// Guys whose behavior is station and are not at their hide segment get processed anyway.
if ((siP->aiP->behavior == AIB_SNIPE) && (siP->ailP->mode != AIM_SNIPE_WAIT))
	return 0;
if (siP->botInfoP->companion || siP->botInfoP->thief)
	return 0;
if (siP->ailP->playerAwarenessType >= PA_RETURN_FIRE - 1) // If robot got hit, he gets to attack tPlayer always!
	return 0;
if ((siP->aiP->behavior == AIB_STATION) && (siP->ailP->mode == AIM_FOLLOW_PATH) && (siP->aiP->nHideSegment != objP->nSegment)) {
	if (gameData.ai.xDistToPlayer > MAX_SNIPE_DIST / 2) {  // station guys not at home always processed until 250 units away.
		AIIdleAnimation (objP);
		return 1;
		}
	}
else if ((siP->aiP->behavior != AIB_STILL) && !siP->ailP->nPrevVisibility && ((gameData.ai.xDistToPlayer >> 7) > siP->ailP->timeSinceProcessed)) {  // 128 units away (6.4 segments) processed after 1 second.
	AIIdleAnimation (objP);
	return 1;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIMatCenHandler (tObject *objP, tAIStateInfo *siP)
{
if (siP->bMultiGame || (gameData.segs.segment2s [objP->nSegment].special != SEGMENT_IS_ROBOTMAKER))
	return 0;
if (!gameData.matCens.fuelCenters [gameData.segs.segment2s [objP->nSegment].value].bEnabled)
	return 0;
AIFollowPath (objP, 1, 1, NULL);    // 1 = tPlayer is visible, which might be a lie, but it works.
return 1;
}

// --------------------------------------------------------------------------------------------------------------------
// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.

int AIApproachHandler (tObject *objP, tAIStateInfo *siP)
{
#ifdef _DEBUG
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (siP->bMultiGame || siP->botInfoP->companion || siP->botInfoP->thief)
	return 0;
if ((siP->aiP->behavior == AIB_SNIPE) || (siP->aiP->behavior == AIB_RUN_FROM) || (siP->aiP->behavior == AIB_STILL))
	return 0;
if (gameData.ai.nOverallAgitation < 71)
	return 0;
if (siP->ailP->playerAwarenessType < PA_RETURN_FIRE) {
	if ((gameData.ai.xDistToPlayer >= MAX_REACTION_DIST) || (d_rand () >= gameData.time.xFrame/4))
		return 0;
	if (d_rand () * (gameData.ai.nOverallAgitation - 40) <= F1_0 * 5)
		return 0;
	}
if ((siP->ailP->mode == AIM_FOLLOW_PATH) && (siP->ailP->nGoalSegment == gameData.ai.nBelievedPlayerSeg)) {
#ifdef _DEBUG
	if (OBJ_IDX (objP) == nDbgObj)
		nDbgObj = nDbgObj;
#endif
	if (objP->nSegment == siP->ailP->nGoalSegment)
		siP->ailP->mode = AIM_IDLING;
	return 0;
	}
CreatePathToPlayer (objP, 4 + gameData.ai.nOverallAgitation / 8 + gameStates.app.nDifficultyLevel, 1);
return 1;
}

// --------------------------------------------------------------------------------------------------------------------

int AIPlayerPosHandler (tObject *objP, tAIStateInfo *siP)
{
if ((siP->aiP->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (gameData.ai.nLastMissileCamera != -1)) {
	gameData.ai.vBelievedPlayerPos = OBJECTS [gameData.ai.nLastMissileCamera].position.vPos;
	return 0;
	}
if (gameStates.app.cheats.bRobotsKillRobots) {
	siP->vVisPos = objP->position.vPos;
	ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
	if (gameData.ai.nPlayerVisibility) {
		int j, nMinObj = -1;
		fix curDist, minDist = MAX_WAKEUP_DIST;

		for (j = 0; j <= gameData.objs.nLastObject [0]; j++)
			if ((OBJECTS [j].nType == OBJ_ROBOT) && (j != siP->nObject)) {
				curDist = vmsVector::dist(objP->position.vPos, OBJECTS [j].position.vPos);
				if ((curDist < MAX_WAKEUP_DIST / 2) && (curDist < minDist) &&
					 ObjectToObjectVisibility (objP, OBJECTS + j, FQ_TRANSWALL)) {
					nMinObj = j;
					minDist = curDist;
					}
				}
		if (nMinObj >= 0) {
			gameData.ai.vBelievedPlayerPos = OBJECTS [nMinObj].position.vPos;
			gameData.ai.nBelievedPlayerSeg = OBJECTS [nMinObj].nSegment;
			vmsVector::normalizedDir(gameData.ai.vVecToPlayer, gameData.ai.vBelievedPlayerPos, objP->position.vPos);
			return 0;
			}
		}
	}
siP->bVisAndVecComputed = 0;
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
	gameData.ai.vBelievedPlayerPos = gameData.ai.cloakInfo [siP->nObject & (MAX_AI_CLOAK_INFO-1)].vLastPos;
else
	gameData.ai.vBelievedPlayerPos = OBJPOS (gameData.objs.console)->vPos;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AIFireHandler (tObject *objP, tAIStateInfo *siP)
{
if ((siP->nPrevVisibility || !(siP->nObjRef & 3)) && ReadyToFire (siP->botInfoP, siP->ailP) &&
		((siP->ailP->playerAwarenessType >= PA_RETURN_FIRE) ||//(siP->aiP->GOAL_STATE == AIS_FIRE) || //i.e. robot has been hit last frame
		(gameData.ai.xDistToPlayer < MAX_WAKEUP_DIST)) &&
		siP->botInfoP->nGuns && !siP->botInfoP->attackType) {
	// Since we passed ReadyToFire (), either nextPrimaryFire or nextSecondaryFire <= 0.  CalcGunPoint from relevant one.
	// If both are <= 0, we will deal with the mess in AIDoActualFiringStuff
	if (siP->ailP->nextPrimaryFire <= 0)
		siP->bHaveGunPos = CalcGunPoint (&gameData.ai.vGunPoint, objP, siP->aiP->CURRENT_GUN);
	else
		siP->bHaveGunPos = CalcGunPoint (&gameData.ai.vGunPoint, objP,
												(ROBOTINFO (objP->id).nGuns == 1) ? 0 : !siP->aiP->CURRENT_GUN);
	siP->vVisPos = gameData.ai.vGunPoint;
	}
else {
	siP->vVisPos = objP->position.vPos;
	gameData.ai.vGunPoint.setZero();
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease tPlayer awareness due to the passage of time.

int AIAwarenessHandler (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic	*aiP = siP->aiP;
	tAILocal		*ailP = siP->ailP;

if (ailP->playerAwarenessType) {
	if ((ailP->playerAwarenessType < PA_RETURN_FIRE) || (siP->nPrevVisibility < 2)) {
		if (ailP->playerAwarenessTime > 0) {
			ailP->playerAwarenessTime -= gameData.time.xFrame;
			if (ailP->playerAwarenessTime <= 0) {
				ailP->playerAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][0];   //new: 11/05/94
				ailP->playerAwarenessType--;          //new: 11/05/94
				}
			}
		else {
			ailP->playerAwarenessType--;
			ailP->playerAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][0];
			//aiP->GOAL_STATE = AIS_REST;
			}
		}
	}
else
	aiP->GOAL_STATE = AIS_REST;                     //new: 12/13/94
if (gameData.ai.nMaxAwareness < ailP->playerAwarenessType)
	gameData.ai.nMaxAwareness = ailP->playerAwarenessType;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease tPlayer awareness due to the passage of time.

int AIPlayerDeadHandler (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic	*aiP = siP->aiP;
	tAILocal		*ailP = siP->ailP;

if (!gameStates.app.bPlayerIsDead || ailP->playerAwarenessType)
	return 0;
if ((gameData.ai.xDistToPlayer >= MAX_REACTION_DIST) || (d_rand () >= gameData.time.xFrame / 8))
	return 0;
if ((aiP->behavior == AIB_STILL) || (aiP->behavior == AIB_RUN_FROM))
	return 0;
if (!AIMultiplayerAwareness (objP, 30))
	return 1;
AIMultiSendRobotPos (siP->nObject, -1);
if (!((ailP->mode == AIM_FOLLOW_PATH) && (aiP->nCurPathIndex < aiP->nPathLength-1)))
	if ((aiP->behavior != AIB_SNIPE) && (aiP->behavior != AIB_RUN_FROM)) {
		if (gameData.ai.xDistToPlayer < F1_0 * 30)
			CreateNSegmentPath (objP, 5, 1);
		else
			CreatePathToPlayer (objP, 20, 1);
		}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// If the robot can see you, increase his awareness of you.
// This prevents the problem of a robot looking right at you but doing nothing.
// Assert (gameData.ai.nPlayerVisibility != -1); // Means it didn't get initialized!

int AIWakeupHandler (tObject *objP, tAIStateInfo *siP)
{
if (!gameData.ai.nPlayerVisibility &&
	 (siP->ailP->playerAwarenessType < PA_WEAPON_WALL_COLLISION) &&
	 (gameData.ai.xDistToPlayer > MAX_WAKEUP_DIST)) {
	AIIdleAnimation (objP);
	return 1;
	}
ComputeVisAndVec (objP, &siP->vVisPos, siP->ailP, siP->botInfoP, &siP->bVisAndVecComputed, MAX_REACTION_DIST);
if ((gameData.ai.nPlayerVisibility == 2) && (siP->aiP->behavior != AIB_FOLLOW) && !siP->botInfoP->thief) {
	if (!siP->ailP->playerAwarenessType) {
		if (siP->aiP->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
			siP->aiP->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else
			siP->ailP->playerAwarenessType = PA_PLAYER_COLLISION;
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease tPlayer awareness due to the passage of time.

int AINewGoalHandler (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic	*aiP = siP->aiP;
	tAILocal		*ailP = siP->ailP;

if (ailP->playerAwarenessType) {
	siP->nNewGoalState = aiTransitionTable [ailP->playerAwarenessType - 1][aiP->CURRENT_STATE][aiP->GOAL_STATE];
#ifdef _DEBUG
	if (siP->nNewGoalState == AIS_LOCK)
		siP->nNewGoalState = siP->nNewGoalState;
#endif
	if (ailP->playerAwarenessType == PA_WEAPON_ROBOT_COLLISION) {
		// Decrease awareness, else this robot will flinch every frame.
		ailP->playerAwarenessType--;
		ailP->playerAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][2] * (gameStates.app.nDifficultyLevel + 1);
		}
	if (siP->nNewGoalState == AIS_ERROR)
		siP->nNewGoalState = AIS_REST;
	if (aiP->CURRENT_STATE == AIS_NONE)
		aiP->CURRENT_STATE = AIS_REST;
	aiP->GOAL_STATE = siP->nNewGoalState;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease tPlayer awareness due to the passage of time.

int AIFireGunsHandler (tObject *objP, tAIStateInfo *siP)
{
if (siP->aiP->GOAL_STATE == AIS_FIRE) {
	tAILocal	*ailP = siP->ailP;
	int		i, nGuns = siP->botInfoP->nGuns;
	for (i = 0; i < nGuns; i++)
		ailP->goalState [i] = AIS_FIRE;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// If a guy hasn't animated to the firing state, but his nextPrimaryFire says ok to fire, bash him there

int AIForceFireHandler (tObject *objP, tAIStateInfo *siP)
{
	tAIStatic	*aiP = siP->aiP;

if (ReadyToFire (siP->botInfoP, siP->ailP) && (aiP->GOAL_STATE == AIS_FIRE))
	aiP->CURRENT_STATE = AIS_FIRE;
if ((aiP->GOAL_STATE != AIS_FLINCH) && (objP->id != ROBOT_BRAIN)) {
	if ((ushort) aiP->CURRENT_STATE < AIS_ERROR)
		return aisHandler1 [aiP->CURRENT_STATE] (objP, siP);
	else
		return AISDefaultHandler1 (objP, siP);
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// If retry count not 0, then add it into nConsecutiveRetries.
// If it is 0, cut down nConsecutiveRetries.
// This is largely a hack to speed up physics and deal with stupid
// AI.  This is low level communication between systems of a sort
// that should not be done.

int AIRetryHandler (tObject *objP, tAIStateInfo *siP)
{
	tAILocal	*ailP = siP->ailP;

if (!ailP->nRetryCount || siP->bMultiGame)
	ailP->nConsecutiveRetries /= 2;
else {
	ailP->nConsecutiveRetries += ailP->nRetryCount;
	ailP->nRetryCount = 0;
	if (ailP->nConsecutiveRetries > 3) {
		if ((ushort) ailP->mode <= AIM_GOTO_OBJECT)
			return aimHandler1 [ailP->mode] (objP, siP);
		ailP->nConsecutiveRetries = 0;
		}
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int AINextFireHandler (tObject *objP, tAIStateInfo *siP)
{
	tAILocal	*ailP = siP->ailP;

if (ailP->nextPrimaryFire > -F1_0 * 8 * gameStates.gameplay.slowmo [0].fSpeed)
	ailP->nextPrimaryFire -= (fix) (gameData.time.xFrame / gameStates.gameplay.slowmo [0].fSpeed);
if (siP->botInfoP->nSecWeaponType != -1) {
	if (ailP->nextSecondaryFire > -F1_0 * 8 * gameStates.gameplay.slowmo [0].fSpeed)
		ailP->nextSecondaryFire -= (fix) (gameData.time.xFrame / gameStates.gameplay.slowmo [0].fSpeed);
	}
else
	ailP->nextSecondaryFire = F1_0 * 8;
if (ailP->timeSinceProcessed < F1_0 * 256)
	ailP->timeSinceProcessed += gameData.time.xFrame;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

void DoAIFrame (tObject *objP)
{
	tAIStateInfo	si;

#ifdef _DEBUG
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
Assert (objP->nSegment != -1);
si.nObject = OBJ_IDX (objP);
si.nObjRef = si.nObject ^ gameData.app.nFrameCount;
si.aiP = &objP->cType.aiInfo;
si.ailP = gameData.ai.localInfo + si.nObject;
si.bHaveGunPos = 0;
si.bVisAndVecComputed = 0;
si.botInfoP = &ROBOTINFO (objP->id);
si.bMultiGame = !IsRobotGame;

#ifdef _DEBUG
if (si.aiP->behavior == AIB_STILL)
	si.aiP = si.aiP;
#endif
gameData.ai.nPlayerVisibility = -1;
si.ailP->nextActionTime -= gameData.time.xFrame;

if (si.aiP->SKIP_AI_COUNT) {
	si.aiP->SKIP_AI_COUNT--;
	if (objP->mType.physInfo.flags & PF_USES_THRUST) {
		objP->mType.physInfo.rotThrust[X] = 15 * objP->mType.physInfo.rotThrust[X] / 16;
		objP->mType.physInfo.rotThrust[Y] = 15 * objP->mType.physInfo.rotThrust[Y] / 16;
		objP->mType.physInfo.rotThrust[Z] = 15 * objP->mType.physInfo.rotThrust[Z] / 16;
		if (!si.aiP->SKIP_AI_COUNT)
			objP->mType.physInfo.flags &= ~PF_USES_THRUST;
		}
	goto funcExit;
	}

Assert (si.botInfoP->always_0xabcd == 0xabcd);
#if 0//def _DEBUG
if (!si.botInfoP->bossFlag)
	goto funcExit;
#endif
if (DoAnyRobotDyingFrame (objP))
	goto funcExit;

// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
// Else, you can turn a big nasty robot into a wimp by firing flares at it.
// This also allows the tPlayer to see the cool flinch effect for mechs without unbalancing the game.
if (((si.aiP->GOAL_STATE == AIS_FLINCH) || (si.ailP->playerAwarenessType == PA_RETURN_FIRE)) &&
	 ReadyToFire (si.botInfoP, si.ailP)) {
	si.aiP->GOAL_STATE = AIS_FIRE;
	}
if ((si.aiP->behavior < MIN_BEHAVIOR) || (si.aiP->behavior > MAX_BEHAVIOR))
		si.aiP->behavior = AIB_NORMAL;
//Assert (objP->id < gameData.bots.nTypes [gameStates.app.bD1Data]);
if (AINextFireHandler (objP, &si))
	goto funcExit;

si.nPrevVisibility = si.ailP->nPrevVisibility;    //  Must get this before we toast the master copy!
// If only awake because of a camera, make that the believed tPlayer position.
if (AIPlayerPosHandler (objP, &si))
	goto funcExit;
gameData.ai.xDistToPlayer = vmsVector::dist(gameData.ai.vBelievedPlayerPos, objP->position.vPos);
// If this robot can fire, compute visibility from gun position.
// Don't want to compute visibility twice, as it is expensive.  (So is call to CalcGunPoint).
if (AIFireHandler (objP, &si))
	goto funcExit;
if (AIApproachHandler (objP, &si))
	goto funcExit;
if (AIRetryHandler (objP, &si))
	goto funcExit;
// If in materialization center, exit
if (AIMatCenHandler (objP, &si))
	goto funcExit;
if (AIAwarenessHandler (objP, &si))
	goto funcExit;
if (AIPlayerDeadHandler (objP, &si))
	goto funcExit;
if (AIBumpHandler (objP, &si))
	goto funcExit;
if ((si.aiP->GOAL_STATE == AIS_FLINCH) && (si.aiP->CURRENT_STATE == AIS_FLINCH))
	si.aiP->GOAL_STATE = AIS_LOCK;
if (AIAnimationHandler (objP, &si))
	goto funcExit;
if (AIBossHandler (objP, &si))
	goto funcExit;
if (AIStationaryHandler (objP, &si))
	goto funcExit;
// Reset time since processed, but skew OBJECTS so not everything
// processed synchronously, else we get fast frames with the
// occasional very slow frame.
// AI_procTime = si.ailP->timeSinceProcessed;
si.ailP->timeSinceProcessed = -((si.nObject & 0x03) * gameData.time.xFrame) / 2;
if (AIBrainHandler (objP, &si))
	goto funcExit;
if (AISnipeHandler (objP, &si))
	goto funcExit;
if (AIBuddyHandler (objP, &si))
	goto funcExit;
if (AIThiefHandler (objP, &si))
	goto funcExit;
// More special ability stuff, but based on a property of a robot, not its ID.
if ((ushort) si.ailP->mode > AIM_THIEF_WAIT)
	AIMDefaultHandler2 (objP, &si);
else if (aimHandler2 [si.ailP->mode] (objP, &si))
	goto funcExit;
if (AIWakeupHandler (objP, &si))
	goto funcExit;

if (!gameData.ai.bObjAnimates)
	si.aiP->CURRENT_STATE = si.aiP->GOAL_STATE;

Assert (si.ailP->playerAwarenessType <= AIE_MAX);
Assert (si.aiP->CURRENT_STATE < AIS_MAX);
Assert (si.aiP->GOAL_STATE < AIS_MAX);

if (AINewGoalHandler (objP, &si))
	goto funcExit;
// If new state = fire, then set all gun states to fire.
if (AIFireGunsHandler (objP, &si))
	goto funcExit;
if (AIForceFireHandler (objP, &si))
	goto funcExit;
// Switch to next gun for next fire.
if (!gameData.ai.nPlayerVisibility) {
	if (++(si.aiP->CURRENT_GUN) >= ROBOTINFO (objP->id).nGuns) {
		if ((si.botInfoP->nGuns == 1) || (si.botInfoP->nSecWeaponType == -1))  // Two weapon types hack.
			si.aiP->CURRENT_GUN = 0;
		else
			si.aiP->CURRENT_GUN = 1;
		}
	}

funcExit:

#if 0//def _DEBUG
HUDMessage (0, "%s %s %d %d %d",
				pszAIState [si.aiP->flags [1]], pszAIState [si.aiP->flags [2]],
				gameData.ai.xDistToPlayer / F1_0,
				si.ailP->playerAwarenessType, si.ailP->playerAwarenessTime);
#else
	;
#endif
}

//	-------------------------------------------------------------------------------------------------
