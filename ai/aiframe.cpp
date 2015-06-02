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

#include "descent.h"
#include "error.h"
#include "segmath.h"
#include "multibot.h"
#include "headlight.h"
#include "visibility.h"

#include "string.h"

#if DBG
#include <time.h>
#endif

// ---------- John: These variables must be saved as part of gamesave. --------

typedef struct tAIStateInfo {
	int16_t			nObject;
	int16_t			nObjRef;
	tRobotInfo		*pRobotInfo;
	tAIStaticInfo	*pStaticInfo;
	tAILocalInfo	*pLocalInfo;
	CFixVector		vVisPos;
	int32_t			nPrevVisibility;
	int32_t			bVisAndVecComputed;
	int32_t			bHaveGunPos;
	int32_t			bMultiGame;
	int32_t			nNewGoalState;
	} tAIStateInfo;


// --------------------------------------------------------------------------------------------------------------------

static fix xAwarenessTimes [2][3] = {
 {I2X (2), I2X (3), I2X (5)},
 {I2X (4), I2X (6), I2X (10)}
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

// These globals are set by a call to FindHitpoint, which is a slow routine,
// so we don't want to call it again (for this CObject) unless we have to.

#if DBG
// Index into this array with pLocalInfo->mode
const char* pszAIMode [18] = {
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

//	Index into this array with pStaticInfo->behavior
char pszAIBehavior [6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

// Index into this array with pStaticInfo->GOAL_STATE or pStaticInfo->CURRENT_STATE
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
// Transition table between states for an AI CObject.
// First dimension is CTrigger event.
// Second dimension is current state.
// Third dimension is goal state.
// Result is new goal state.
// ERR_ means something impossible has happened.
int8_t aiTransitionTable [AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
 {
		// Event = AIE_FIRE, a nearby CObject fired
		// none     rest      srch      lock      flin      fire      recover     // CURRENT is rows, GOAL is columns
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // none
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // rest
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // search
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // lock
	 { AIS_ERROR, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECOVER }, // flinch
	 { AIS_ERROR, AIS_FIRE, AIS_FIRE, AIS_FIRE, AIS_FLINCH, AIS_FIRE, AIS_RECOVER }, // fire
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_FIRE }  // recoil
	},

	// Event = AIE_HIT, a nearby CObject was hit (or a CWall was hit)
 {
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_RECOVER},
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FLINCH},
	 { AIS_ERROR, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECOVER},
	 { AIS_ERROR, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLINCH, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_COLLIDE, player collided with robot
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

	// Event = AIE_HURT, player hurt robot (by firing at and hitting it)
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

int16_t AIRouteSeg (uint32_t i)
{
return (i < gameData.aiData.routeSegs.Length ()) ? gameData.aiData.routeSegs [i].nSegment : -1;
}


// --------------------------------------------------------------------------------------------------------------------

int32_t AINothingHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
return 0;
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIMGotoPlayerHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
MoveTowardsSegmentCenter (pObj);
CreatePathToTarget (pObj, 100, 1);
RETVAL (0)
}


int32_t AIMGotoObjectHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
gameData.escortData.nGoalObject = ESCORT_GOAL_UNSPECIFIED;
RETVAL (0)
}


int32_t AIMChaseObjectHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
CreatePathToTarget (pObj, 4 + gameData.aiData.nOverallAgitation/8 + gameStates.app.nDifficultyLevel, 1);
RETVAL (0)
}


int32_t AIMIdlingHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
if (pStateInfo->pRobotInfo->attackType)
	MoveTowardsSegmentCenter (pObj);
else if ((pStateInfo->pStaticInfo->behavior != AIB_STILL) && (pStateInfo->pStaticInfo->behavior != AIB_STATION) && (pStateInfo->pStaticInfo->behavior != AIB_FOLLOW))    // Behavior is still, so don't follow path.
	AttemptToResumePath (pObj);
RETVAL (0)
}


int32_t AIMFollowPathHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->bMultiGame)
	pStateInfo->pLocalInfo->mode = AIM_IDLING;
else
	AttemptToResumePath (pObj);
RETVAL (0)
}


int32_t AIMRunFromObjectHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
MoveTowardsSegmentCenter (pObj);
pObj->mType.physInfo.velocity.SetZero ();
CreateNSegmentPath (pObj, 5, -1);
pStateInfo->pLocalInfo->mode = AIM_RUN_FROM_OBJECT;
RETVAL (0)
}


int32_t AIMBehindHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
MoveTowardsSegmentCenter (pObj);
pObj->mType.physInfo.velocity.SetZero ();
RETVAL (0)
}


int32_t AIMOpenDoorHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
CreateNSegmentPathToDoor (pObj, 5, -1);
RETVAL (0)
}


// --------------------------------------------------------------------------------------------------------------------

int32_t AIMChaseObjectHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo *pStaticInfo = pStateInfo->pStaticInfo;
	fix circleDistance = pStateInfo->pRobotInfo->circleDistance [gameStates.app.nDifficultyLevel] + TARGETOBJ->info.xSize;

// Green guy doesn't get his circle distance boosted, else he might never attack.
if (pStateInfo->pRobotInfo->attackType != 1)
	circleDistance += I2X (pStateInfo->nObject & 0xf) / 2;
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, -MAX_CHASE_DIST);
if ((gameData.aiData.nTargetVisibility < 2) && (pStateInfo->nPrevVisibility == 2)) {
	if (!AILocalPlayerControlsRobot (pObj, 53)) {
		if (AIMaybeDoActualFiringStuff (pObj, pStaticInfo))
			AIDoActualFiringStuff (pObj, pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStaticInfo->CURRENT_GUN);
		RETVAL (1)
		}
	CreatePathToTarget (pObj, 8, 1);
	AIMultiSendRobotPos (pStateInfo->nObject, -1);
	}
else if (!gameData.aiData.nTargetVisibility && (gameData.aiData.target.xDist > MAX_CHASE_DIST) && !pStateInfo->bMultiGame) {
	// If pretty far from the player, player cannot be seen
	// (obstructed) and in chase mode, switch to follow path mode.
	// This has one desirable benefit of avoiding physics retries.
	if (pStaticInfo->behavior == AIB_STATION) {
		pStateInfo->pLocalInfo->nGoalSegment = pStaticInfo->nHideSegment;
		CreatePathToStation (pObj, 15);
		} // -- this looks like a dumb thing to do...robots following paths far away from you!else CreateNSegmentPath (pObj, 5, -1);
	RETVAL (0)
	}

if ((pStaticInfo->CURRENT_STATE == AIS_REST) && (pStaticInfo->GOAL_STATE == AIS_REST)) {
	if (gameData.aiData.nTargetVisibility &&
		 (RandShort () < gameData.timeData.xFrame * gameData.aiData.nTargetVisibility) &&
		 (gameData.aiData.target.xDist / 256 < RandShort () * gameData.aiData.nTargetVisibility)) {
		pStaticInfo->GOAL_STATE = AIS_SEARCH;
		pStaticInfo->CURRENT_STATE = AIS_SEARCH;
		}
	else
		AIDoRandomPatrol (pObj, pStateInfo->pLocalInfo);
	}

if (gameData.timeData.xGame - pStateInfo->pLocalInfo->timeTargetSeen > CHASE_TIME_LENGTH) {
	if (pStateInfo->bMultiGame)
		if (!gameData.aiData.nTargetVisibility && (gameData.aiData.target.xDist > I2X (70))) {
			pStateInfo->pLocalInfo->mode = AIM_IDLING;
			RETVAL (1)
			}
	if (!AILocalPlayerControlsRobot (pObj, 64)) {
		if (AIMaybeDoActualFiringStuff (pObj, pStaticInfo))
			AIDoActualFiringStuff (pObj, pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStaticInfo->CURRENT_GUN);
		RETVAL (1)
		}
	// -- bad idea, robots charge player they've never seen!-- CreatePathToTarget (pObj, 10, 1);
	// -- bad idea, robots charge player they've never seen!-- AIMultiSendRobotPos (pStateInfo->nObject, -1);
	}
else if ((pStaticInfo->CURRENT_STATE != AIS_REST) && (pStaticInfo->GOAL_STATE != AIS_REST)) {
	if (!AILocalPlayerControlsRobot (pObj, 70)) {
		if (AIMaybeDoActualFiringStuff (pObj, pStaticInfo))
			AIDoActualFiringStuff (pObj, pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStaticInfo->CURRENT_GUN);
		RETVAL (1)
		}
	AIMoveRelativeToTarget (pObj, pStateInfo->pLocalInfo, gameData.aiData.target.xDist, &gameData.aiData.target.vDir, circleDistance, 0, gameData.aiData.nTargetVisibility);
	if ((pStateInfo->nObjRef & 1) && ((pStaticInfo->GOAL_STATE == AIS_SEARCH) || (pStaticInfo->GOAL_STATE == AIS_LOCK))) {
		if (gameData.aiData.nTargetVisibility) // == 2)
			AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
		}
	if (gameData.aiData.bEvaded) {
		AIMultiSendRobotPos (pStateInfo->nObject, 1);
		gameData.aiData.bEvaded = 0;
		}
	else
		AIMultiSendRobotPos (pStateInfo->nObject, -1);
	DoFiringStuff (pObj, gameData.aiData.nTargetVisibility, &gameData.aiData.target.vDir);
	}
RETVAL (0)
}


int32_t AIMRunFromObjectHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo *pStaticInfo = pStateInfo->pStaticInfo;

ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
if (gameData.aiData.nTargetVisibility) {
	if (pStateInfo->pLocalInfo->targetAwarenessType == 0)
		pStateInfo->pLocalInfo->targetAwarenessType = PA_RETURN_FIRE;
	}
// If in multiplayer, only do if player visible.  If not multiplayer, do always.
if (!(pStateInfo->bMultiGame) || gameData.aiData.nTargetVisibility)
	if (AILocalPlayerControlsRobot (pObj, 75)) {
		AIFollowPath (pObj, gameData.aiData.nTargetVisibility, pStateInfo->nPrevVisibility, &gameData.aiData.target.vDir);
		AIMultiSendRobotPos (pStateInfo->nObject, -1);
		}
if (pStaticInfo->GOAL_STATE != AIS_FLINCH)
	pStaticInfo->GOAL_STATE = AIS_LOCK;
else if (pStaticInfo->CURRENT_STATE == AIS_FLINCH)
	pStaticInfo->GOAL_STATE = AIS_LOCK;

// Bad to let run_from robot fire at player because it will cause a war in which it turns towards the player
// to fire and then towards its goal to move. DoFiringStuff (pObj, gameData.aiData.nTargetVisibility, &gameData.aiData.target.vDir);
// Instead, do this:
// (Note, only drop if player is visible.  This prevents the bombs from being a giveaway, and also ensures that
// the robot is moving while it is dropping.  Also means fewer will be dropped.)
if ((pStateInfo->pLocalInfo->pNextrimaryFire <= 0) && (gameData.aiData.nTargetVisibility)) {
	CFixVector fire_vec, fire_pos;

	if (!AILocalPlayerControlsRobot (pObj, 75))
		RETVAL (1)
	fire_vec = pObj->info.position.mOrient.m.dir.f;
	fire_vec = -fire_vec;
	fire_pos = pObj->info.position.vPos + fire_vec;
	CreateNewWeaponSimple (&fire_vec, &fire_pos, pObj->Index (), (pStaticInfo->SUB_FLAGS & SUB_FLAGS_SPROX) ? ROBOT_SMARTMINE_ID : PROXMINE_ID, 1);
	pStateInfo->pLocalInfo->pNextrimaryFire = (I2X (1)/2)* (DIFFICULTY_LEVEL_COUNT+5 - gameStates.app.nDifficultyLevel);      // Drop a proximity bomb every 5 seconds.
	if (pStateInfo->bMultiGame) {
		AIMultiSendRobotPos (pObj->Index (), -1);
		MultiSendRobotFire (pObj->Index (), (pStaticInfo->SUB_FLAGS & SUB_FLAGS_SPROX) ? -2 : -1, &fire_vec);
		}
	}
RETVAL (0)
}


int32_t AIMGotoHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
AIFollowPath (pObj, 2, pStateInfo->nPrevVisibility, &gameData.aiData.target.vDir);    // Follows path as if player can see robot.
AIMultiSendRobotPos (pStateInfo->nObject, -1);
RETVAL (0)
}


int32_t AIMFollowPathHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo *pStaticInfo = pStateInfo->pStaticInfo;
	int32_t angerLevel = 65;

if (pStaticInfo->behavior == AIB_STATION)
	if ((pStaticInfo->nHideIndex >= 0) && (AIRouteSeg (uint32_t (pStaticInfo->nHideIndex + pStaticInfo->nPathLength - 1)) == pStaticInfo->nHideSegment)) {
		angerLevel = 64;
	}
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (gameData.appData.GameMode (GM_MODEM | GM_SERIAL))
	if (!gameData.aiData.nTargetVisibility && (gameData.aiData.target.xDist > I2X (70))) {
		pStateInfo->pLocalInfo->mode = AIM_IDLING;
		RETVAL (1)
		}
if (!AILocalPlayerControlsRobot (pObj, angerLevel)) {
	if (AIMaybeDoActualFiringStuff (pObj, pStaticInfo)) {
		ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, I2X (10000));
		AIDoActualFiringStuff (pObj, pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStaticInfo->CURRENT_GUN);
		}
	RETVAL (1)
	}
if (gameOpts->gameplay.nAIAggressivity && (pStateInfo->pLocalInfo->nGoalSegment == gameData.aiData.target.nBelievedSeg)) {	// destination reached
	pStateInfo->pLocalInfo->mode = AIM_IDLING;
	RETVAL (0)
	}
if (!gameData.aiData.nTargetVisibility &&	// lost player out of sight
	 (!gameOpts->gameplay.nAIAggressivity ||	// standard mode: stop immediately
	  (pStateInfo->pLocalInfo->nGoalSegment == gameData.aiData.target.nBelievedSeg) ||	// destination reached
	  (I2X (gameOpts->gameplay.nAIAggressivity - 1) < gameData.timeData.xGame - pStateInfo->pLocalInfo->timeTargetSeen) ||	// too much time passed since player was lost out of sight
	  (pStateInfo->pLocalInfo->targetAwarenessType < PA_RETURN_FIRE) ||		// not trying to return fire (any more)
	  (pStateInfo->pLocalInfo->mode != AIM_FOLLOW_PATH))) {	// not in chase mode
	pStateInfo->pLocalInfo->mode = AIM_IDLING;
	RETVAL (0)
	}
AIFollowPath (pObj, gameData.aiData.nTargetVisibility, pStateInfo->nPrevVisibility, &gameData.aiData.target.vDir);
if (pStaticInfo->GOAL_STATE != AIS_FLINCH)
	pStaticInfo->GOAL_STATE = AIS_LOCK;
else if (pStaticInfo->CURRENT_STATE == AIS_FLINCH)
	pStaticInfo->GOAL_STATE = AIS_LOCK;
if (pStaticInfo->behavior != AIB_RUN_FROM)
	DoFiringStuff (pObj, gameData.aiData.nTargetVisibility, &gameData.aiData.target.vDir);
if ((gameData.aiData.nTargetVisibility == 2) &&
		(pObj->info.nId != ROBOT_BRAIN) && !(pStateInfo->pRobotInfo->companion || pStateInfo->pRobotInfo->thief) &&
		(pStaticInfo->behavior != AIB_STILL) &&
		(pStaticInfo->behavior != AIB_SNIPE) &&
		(pStaticInfo->behavior != AIB_FOLLOW) &&
		(pStaticInfo->behavior != AIB_RUN_FROM)) {
	if (pStateInfo->pRobotInfo->attackType == 0)
		pStateInfo->pLocalInfo->mode = AIM_CHASE_OBJECT;
	// This should not just be distance based, but also time-since-player-seen based.
	}
else if ((gameData.aiData.target.xDist > MAX_PURSUIT_DIST (pStateInfo->pRobotInfo))
			&& (gameData.timeData.xGame - pStateInfo->pLocalInfo->timeTargetSeen > ((I2X (1) / 2) * (gameStates.app.nDifficultyLevel + pStateInfo->pRobotInfo->pursuit)))
			&& (gameData.aiData.nTargetVisibility == 0)
			&& (pStaticInfo->behavior == AIB_NORMAL)
			&& (pStateInfo->pLocalInfo->mode == AIM_FOLLOW_PATH)) {
	pStateInfo->pLocalInfo->mode = AIM_IDLING;
	pStaticInfo->nHideIndex = -1;
	pStaticInfo->nPathLength = 0;
	}
AIMultiSendRobotPos (pStateInfo->nObject, -1);
RETVAL (0)
}


int32_t AIMBehindHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (!AILocalPlayerControlsRobot (pObj, 71)) {
	if (AIMaybeDoActualFiringStuff (pObj, pStateInfo->pStaticInfo)) {
		ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
		AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
		}
	RETVAL (1)
	}
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
if (gameData.aiData.nTargetVisibility == 2) {
	// Get behind the player.
	// Method:
	// If gameData.aiData.target.vDir dot player_rear_vector > 0, behind is goal.
	// Else choose goal with larger dot from left, right.
	CFixVector  goal_point, vGoal, vec_to_goal, vRand;
	fix         dot;

	dot = CFixVector::Dot (OBJPOS (TARGETOBJ)->mOrient.m.dir.f, gameData.aiData.target.vDir);
	if (dot > 0) {          // Remember, we're interested in the rear vector dot being < 0.
		vGoal = OBJPOS (TARGETOBJ)->mOrient.m.dir.f;
		vGoal.Neg ();
		}
	else {
		fix dot;
		dot = CFixVector::Dot (OBJPOS (TARGETOBJ)->mOrient.m.dir.r, gameData.aiData.target.vDir);
		vGoal = OBJPOS (TARGETOBJ)->mOrient.m.dir.r;
		if (dot > 0) {
			vGoal.Neg ();
			}
		else
			;
		}
	vGoal *= (2* (TARGETOBJ->info.xSize + pObj->info.xSize + (((pStateInfo->nObject*4 + gameData.appData.nFrameCount) & 63) << 12)));
	goal_point = OBJPOS (TARGETOBJ)->vPos + vGoal;
	vRand = CFixVector::Random();
	goal_point += vRand * I2X (8);
	vec_to_goal = goal_point - pObj->info.position.vPos;
	CFixVector::Normalize (vec_to_goal);
	MoveTowardsVector (pObj, &vec_to_goal, 0);
	AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
	AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
	}
if (pStateInfo->pStaticInfo->GOAL_STATE != AIS_FLINCH)
	pStateInfo->pStaticInfo->GOAL_STATE = AIS_LOCK;
else if (pStateInfo->pStaticInfo->CURRENT_STATE == AIS_FLINCH)
	pStateInfo->pStaticInfo->GOAL_STATE = AIS_LOCK;
AIMultiSendRobotPos (pStateInfo->nObject, -1);
RETVAL (0)
}


int32_t AIMIdlingHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if ((gameData.aiData.target.xDist < MAX_WAKEUP_DIST) || (pStateInfo->pLocalInfo->targetAwarenessType >= PA_RETURN_FIRE - 1)) {
	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);

	// turn towards vector if visible this time or last time, or rand
	// new!
	if ((gameData.aiData.nTargetVisibility == 2) || (pStateInfo->nPrevVisibility == 2)) { // -- MK, 06/09/95:  || ((RandShort () > 0x4000) && !(pStateInfo->bMultiGame))) {
		if (!AILocalPlayerControlsRobot (pObj, 71)) {
			if (AIMaybeDoActualFiringStuff (pObj, pStateInfo->pStaticInfo))
				AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
			RETVAL (1)
			}
		AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
		AIMultiSendRobotPos (pStateInfo->nObject, -1);
		}
	DoFiringStuff (pObj, gameData.aiData.nTargetVisibility, &gameData.aiData.target.vDir);
	if (gameData.aiData.nTargetVisibility == 2) {  // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
		if (pStateInfo->pRobotInfo->attackType == 1) {
			pStateInfo->pStaticInfo->behavior = AIB_NORMAL;
			if (!AILocalPlayerControlsRobot (pObj, 80)) {
				if (AIMaybeDoActualFiringStuff (pObj, pStateInfo->pStaticInfo))
					AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
				RETVAL (1)
				}
			AIMoveRelativeToTarget (pObj, pStateInfo->pLocalInfo, gameData.aiData.target.xDist, &gameData.aiData.target.vDir, 0, 0, gameData.aiData.nTargetVisibility);
			if (gameData.aiData.bEvaded) {
				AIMultiSendRobotPos (pStateInfo->nObject, 1);
				gameData.aiData.bEvaded = 0;
				}
			else
				AIMultiSendRobotPos (pStateInfo->nObject, -1);
			}
		else {
			// Robots in hover mode are allowed to evade at half Normal speed.
			if (!AILocalPlayerControlsRobot (pObj, 81)) {
				if (AIMaybeDoActualFiringStuff (pObj, pStateInfo->pStaticInfo))
					AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
				RETVAL (1)
				}
			if (pStateInfo->pStaticInfo->behavior != AIB_STILL)
				AIMoveRelativeToTarget (pObj, pStateInfo->pLocalInfo, gameData.aiData.target.xDist, &gameData.aiData.target.vDir, 0, 1, gameData.aiData.nTargetVisibility);
			else
				gameData.aiData.bEvaded = 0;
			if (gameData.aiData.bEvaded) {
				AIMultiSendRobotPos (pStateInfo->nObject, -1);
				gameData.aiData.bEvaded = 0;
				}
			else
				AIMultiSendRobotPos (pStateInfo->nObject, -1);
			}
		}
	else if ((pObj->info.nSegment != pStateInfo->pStaticInfo->nHideSegment) && (gameData.aiData.target.xDist > MAX_CHASE_DIST) && !pStateInfo->bMultiGame) {
		// If pretty far from the player, player cannot be
		// seen (obstructed) and in chase mode, switch to
		// follow path mode.
		// This has one desirable benefit of avoiding physics retries.
		if (pStateInfo->pStaticInfo->behavior == AIB_STATION) {
			pStateInfo->pLocalInfo->nGoalSegment = pStateInfo->pStaticInfo->nHideSegment;
			CreatePathToStation (pObj, 15);
			}
		}
	}
RETVAL (0)
}


int32_t AIMOpenDoorHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	CFixVector vCenter, vGoal;

Assert (pObj->info.nId == ROBOT_BRAIN);     // Make sure this guy is allowed to be in this mode.
if (!AILocalPlayerControlsRobot (pObj, 62))
	RETVAL (1)
vCenter = SEGMENT (pObj->info.nSegment)->SideCenter (pStateInfo->pStaticInfo->GOALSIDE);
vGoal = vCenter - pObj->info.position.vPos;
CFixVector::Normalize (vGoal);
AITurnTowardsVector (&vGoal, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
MoveTowardsVector (pObj, &vGoal, 0);
AIMultiSendRobotPos (pStateInfo->nObject, -1);
RETVAL (0)
}


int32_t AIMSnipeHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (AILocalPlayerControlsRobot (pObj, 53)) {
	AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
	if (pStateInfo->pRobotInfo->thief)
		AIMoveRelativeToTarget (pObj, pStateInfo->pLocalInfo, gameData.aiData.target.xDist, &gameData.aiData.target.vDir, 0, 0, gameData.aiData.nTargetVisibility);
	}
RETVAL (0)
}


int32_t AIMDefaultHandler2 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
pStateInfo->pLocalInfo->mode = AIM_CHASE_OBJECT;
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AISNoneHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);
fix dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, gameData.aiData.target.vDir);
if ((dot >= I2X (1)/2) && (pStateInfo->pStaticInfo->GOAL_STATE == AIS_REST))
	pStateInfo->pStaticInfo->GOAL_STATE = AIS_SEARCH;
RETVAL (0)
}


int32_t AISRestHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->pStaticInfo->GOAL_STATE == AIS_REST) {
	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);
	if (gameData.aiData.nTargetVisibility && ReadyToFire (pStateInfo->pRobotInfo, pStateInfo->pLocalInfo))
		pStateInfo->pStaticInfo->GOAL_STATE = AIS_FIRE;
	}
RETVAL (0)
}


int32_t AISSearchHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (!AILocalPlayerControlsRobot (pObj, 60))
	RETVAL (1)
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (gameData.aiData.nTargetVisibility == 2) {
	AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
	AIMultiSendRobotPos (pStateInfo->nObject, -1);
	}
RETVAL (0)
}


int32_t AISLockHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (!pStateInfo->bMultiGame || (gameData.aiData.nTargetVisibility)) {
	if (!AILocalPlayerControlsRobot (pObj, 68))
		RETVAL (1)
	if (gameData.aiData.nTargetVisibility == 2) {   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
		AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
		AIMultiSendRobotPos (pStateInfo->nObject, -1);
		}
	}
RETVAL (0)
}


int32_t AISFireHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
if (gameData.aiData.nTargetVisibility == 2) {
	if (!AILocalPlayerControlsRobot (pObj, (ROBOT_FIRE_AGITATION - 1))) {
		if (pStateInfo->bMultiGame) {
			AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
			RETVAL (1)
			}
		}
	AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
	AIMultiSendRobotPos (pStateInfo->nObject, -1);
	}
// Fire at player, if appropriate.
if (!pStateInfo->bHaveGunPos) {
	if (pStateInfo->pLocalInfo->pNextrimaryFire <= 0)
		pStateInfo->bHaveGunPos = CalcGunPoint (&gameData.aiData.vGunPoint, pObj, pStateInfo->pStaticInfo->CURRENT_GUN);
	else
		pStateInfo->bHaveGunPos = CalcGunPoint (&gameData.aiData.vGunPoint, pObj, (pStateInfo->pRobotInfo->nGuns == 1) ? 0 : !pStateInfo->pStaticInfo->CURRENT_GUN);
	pStateInfo->vVisPos = gameData.aiData.vGunPoint;
	}
AIDoActualFiringStuff (pObj, pStateInfo->pStaticInfo, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, pStateInfo->pStaticInfo->CURRENT_GUN);
pStateInfo->bHaveGunPos = 0;
RETVAL (0)
}


int32_t AISRecoverHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->nObjRef & 3)
	RETVAL (0)
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_WAKEUP_DIST);
if (gameData.aiData.nTargetVisibility != 2)
	RETVAL (0)
if (!AILocalPlayerControlsRobot (pObj, 69))
	RETVAL (1)
AITurnTowardsVector (&gameData.aiData.target.vDir, pObj, pStateInfo->pRobotInfo->turnTime [gameStates.app.nDifficultyLevel]);
AIMultiSendRobotPos (pStateInfo->nObject, -1);
RETVAL (0)
}


int32_t AISDefaultHandler1 (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
pStateInfo->pStaticInfo->GOAL_STATE =
pStateInfo->pStaticInfo->CURRENT_STATE = AIS_REST;
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int32_t (__fastcall * pAIHandler) (CObject *pObj, tAIStateInfo *);
#else
typedef int32_t (* pAIHandler) (CObject *pObj, tAIStateInfo *);
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

int32_t AIBrainHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
// Robots function nicely if behavior is gameData.producerData.producers.  This
// means they won't move until they can see the player, at
// which time they will start wandering about opening doors.
if (pObj->info.nId != ROBOT_BRAIN)
	RETVAL (0)
if (OBJSEG (TARGETOBJ) == pObj->info.nSegment) {
	if (!AILocalPlayerControlsRobot (pObj, 97))
		RETVAL (1)
	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, I2X (10000));
	MoveAwayFromTarget (pObj, &gameData.aiData.target.vDir, 0);
	AIMultiSendRobotPos (pStateInfo->nObject, -1);
	}
else if (pStateInfo->pLocalInfo->mode != AIM_IDLING) {
	int32_t r = pObj->OpenableDoorsInSegment ();
	if (r != -1) {
		pStateInfo->pLocalInfo->mode = AIM_OPEN_DOOR;
		pStateInfo->pStaticInfo->GOALSIDE = r;
		}
	else if (pStateInfo->pLocalInfo->mode != AIM_FOLLOW_PATH) {
		if (!AILocalPlayerControlsRobot (pObj, 50))
			RETVAL (1)
		CreateNSegmentPathToDoor (pObj, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
		AIMultiSendRobotPos (pStateInfo->nObject, -1);
		}
	if (pStateInfo->pLocalInfo->nextActionTime < 0) {
		ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, I2X (10000));
		if (gameData.aiData.nTargetVisibility) {
			MakeNearbyRobotSnipe ();
			pStateInfo->pLocalInfo->nextActionTime = (DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel) * I2X (2);
			}
		}
	}
else {
	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, I2X (10000));
	if (gameData.aiData.nTargetVisibility) {
		if (!AILocalPlayerControlsRobot (pObj, 50))
			RETVAL (1)
		CreateNSegmentPathToDoor (pObj, 8+gameStates.app.nDifficultyLevel, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
		AIMultiSendRobotPos (pStateInfo->nObject, -1);
		}
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AISnipeHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->pStaticInfo->behavior == AIB_SNIPE) {
	if (pStateInfo->bMultiGame && !pStateInfo->pRobotInfo->thief) {
		pStateInfo->pStaticInfo->behavior = AIB_NORMAL;
		pStateInfo->pLocalInfo->mode = AIM_CHASE_OBJECT;
		RETVAL (1)
		}
	if (!(pStateInfo->nObjRef & 3) || pStateInfo->nPrevVisibility) {
		ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
		// If this sniper is in still mode, if he was hit or can see player, switch to snipe mode.
		if (pStateInfo->pLocalInfo->mode == AIM_IDLING)
			if (gameData.aiData.nTargetVisibility || (pStateInfo->pLocalInfo->targetAwarenessType == PA_RETURN_FIRE))
				pStateInfo->pLocalInfo->mode = AIM_SNIPE_ATTACK;
		if (!pStateInfo->pRobotInfo->thief && (pStateInfo->pLocalInfo->mode != AIM_IDLING))
			DoSnipeFrame (pObj);
		}
	else if (!pStateInfo->pRobotInfo->thief && !pStateInfo->pRobotInfo->companion)
		RETVAL (1)
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIBuddyHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->pRobotInfo->companion) {
		tAIStaticInfo	*pStaticInfo = pStateInfo->pStaticInfo;

	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, I2X (10000));
	DoEscortFrame (pObj, gameData.aiData.target.xDist, gameData.aiData.nTargetVisibility);

	if (pObj->cType.aiInfo.nDangerLaser != -1) {
		CObject *pThreat = OBJECT (pObj->cType.aiInfo.nDangerLaser);

		if (!pThreat)
			pObj->cType.aiInfo.nDangerLaser = -1;
		else if ((pThreat->info.nType == OBJ_WEAPON) && (pThreat->info.nSignature == pObj->cType.aiInfo.nDangerLaserSig)) {
			fix circleDistance = pStateInfo->pRobotInfo->circleDistance [gameStates.app.nDifficultyLevel] + TARGETOBJ->info.xSize;
			AIMoveRelativeToTarget (pObj, pStateInfo->pLocalInfo, gameData.aiData.target.xDist, &gameData.aiData.target.vDir, circleDistance, 1, gameData.aiData.nTargetVisibility);
			}
		}

	if (ReadyToFire (pStateInfo->pRobotInfo, pStateInfo->pLocalInfo)) {
		int32_t bDoStuff = 0;

		if (pObj->OpenableDoorsInSegment () != -1)
			bDoStuff = 1;
		else if (pStaticInfo->nHideIndex >= 0) {
			int16_t nSegment;
			int32_t i = pStaticInfo->nHideIndex + pStaticInfo->nCurPathIndex;
			if (((nSegment = AIRouteSeg (uint32_t (i + pStaticInfo->PATH_DIR))) >= 0) && (SEGMENT (nSegment)->HasOpenableDoor () != -1))
				bDoStuff = 1;
			else if (((nSegment = AIRouteSeg (uint32_t (i + 2 * pStaticInfo->PATH_DIR))) >= 0) && (SEGMENT (nSegment)->HasOpenableDoor () != -1))
				bDoStuff = 1;
			}
		if (!bDoStuff && (pStateInfo->pLocalInfo->mode == AIM_GOTO_PLAYER) && (gameData.aiData.target.xDist < 3 * MIN_ESCORT_DISTANCE / 2)) {
			bDoStuff = 1;
			}
		else
			;

		if (bDoStuff && (CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, gameData.aiData.target.vDir) < I2X (1) / 2)) {
			CreateNewWeaponSimple (&pObj->info.position.mOrient.m.dir.f, &pObj->info.position.vPos, pObj->Index (), FLARE_ID, 1);
			pStateInfo->pLocalInfo->pNextrimaryFire = I2X (1)/2;
			if (!gameData.escortData.bMayTalk) // If buddy not talking, make him fire flares less often.
				pStateInfo->pLocalInfo->pNextrimaryFire += RandShort ()*4;
			}
		}
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIThiefHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->pRobotInfo->thief) {
		tAIStaticInfo	*pStaticInfo = pStateInfo->pStaticInfo;

	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
	DoThiefFrame (pObj);
	if (ReadyToFire (pStateInfo->pRobotInfo, pStateInfo->pLocalInfo)) {
		int32_t bDoStuff = 0;
		if (pObj->OpenableDoorsInSegment () != -1)
			bDoStuff = 1;
		else {
			int16_t nSegment;
			int32_t i = pStaticInfo->nHideIndex + pStaticInfo->nCurPathIndex;
			if (((nSegment = AIRouteSeg (uint32_t (i + pStaticInfo->PATH_DIR))) >= 0) && (SEGMENT (nSegment)->HasOpenableDoor () != -1))
				bDoStuff = 1;
			else if (((nSegment = AIRouteSeg (uint32_t (i + 2 * pStaticInfo->PATH_DIR))) >= 0) && (SEGMENT (nSegment)->HasOpenableDoor () != -1))
				bDoStuff = 1;
			}
		if (bDoStuff) {
			// @mk, 05/08/95: Firing flare from center of CObject, this is dumb...
			CreateNewWeaponSimple (&pObj->info.position.mOrient.m.dir.f, &pObj->info.position.vPos, pObj->Index (), FLARE_ID, 1);
			pStateInfo->pLocalInfo->pNextrimaryFire = I2X (1) / 2;
			if (gameData.thiefData.nStolenItem == 0)     // If never stolen an item, fire flares less often (bad: gameData.thiefData.nStolenItem wraps, but big deal)
				pStateInfo->pLocalInfo->pNextrimaryFire += RandShort ()*4;
			}
		}
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// Make sure that if this guy got hit or bumped, then he's chasing player.

int32_t AIBumpHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
if (gameData.aiData.nTargetVisibility != 1) // Only increase visibility if unobstructed, else claw guys attack through doors.
	RETVAL (0)
if (pStateInfo->pLocalInfo->targetAwarenessType >= PA_PLAYER_COLLISION)
	;
else if ((pStateInfo->nObjRef & 3) || pStateInfo->nPrevVisibility || (gameData.aiData.target.xDist >= MAX_WAKEUP_DIST))
	RETVAL (0)
else {
	if (!HeadlightIsOn (-1)) {
		fix rval = RandShort ();
		fix sval = (gameData.aiData.target.xDist * (gameStates.app.nDifficultyLevel + 1)) / 64;
		if	((FixMul (rval, sval) >= gameData.timeData.xFrame))
			RETVAL (0)
		}
	pStateInfo->pLocalInfo->targetAwarenessType = PA_PLAYER_COLLISION;
	pStateInfo->pLocalInfo->targetAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][1];
	}
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
gameData.aiData.nTargetVisibility = 2;
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIAnimationHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
// Note: Should only do these two function calls for objects which animate
if (gameData.aiData.bEnableAnimation && (gameData.aiData.target.xDist < MAX_WAKEUP_DIST / 2)) { // && !pStateInfo->bMultiGame)) {
	gameData.aiData.bObjAnimates = DoSillyAnimation (pObj);
	if (gameData.aiData.bObjAnimates)
		AIFrameAnimation (pObj);
	}
else {
	// If Object is supposed to animate, but we don't let it animate due to distance, then
	// we must change its state, else it will never update.
	pStateInfo->pStaticInfo->CURRENT_STATE = pStateInfo->pStaticInfo->GOAL_STATE;
	gameData.aiData.bObjAnimates = 0;        // If we're not doing the animation, then should pretend it doesn't animate.
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIBossHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->pRobotInfo->bossFlag) {
		int32_t	pv;

	if (pStateInfo->pStaticInfo->GOAL_STATE == AIS_FLINCH)
		pStateInfo->pStaticInfo->GOAL_STATE = AIS_FIRE;
	if (pStateInfo->pStaticInfo->CURRENT_STATE == AIS_FLINCH)
		pStateInfo->pStaticInfo->CURRENT_STATE = AIS_FIRE;
	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
	pv = gameData.aiData.nTargetVisibility;
	// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
		pv = 0;
	DoBossStuff (pObj, pv);
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIStationaryHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
// Time-slice, don't process all the time, purely an efficiency hack.
// Guys whose behavior is station and are not at their hide segment get processed anyway.
if ((pStateInfo->pStaticInfo->behavior == AIB_SNIPE) && (pStateInfo->pLocalInfo->mode != AIM_SNIPE_WAIT))
	RETVAL (0)
if (pStateInfo->pRobotInfo->companion || pStateInfo->pRobotInfo->thief)
	RETVAL (0)
if (pStateInfo->pLocalInfo->targetAwarenessType >= PA_RETURN_FIRE - 1) // If robot got hit, he gets to attack player always!
	RETVAL (0)
if ((pStateInfo->pStaticInfo->behavior == AIB_STATION) && (pStateInfo->pLocalInfo->mode == AIM_FOLLOW_PATH) && (pStateInfo->pStaticInfo->nHideSegment != pObj->info.nSegment)) {
	if (gameData.aiData.target.xDist > MAX_SNIPE_DIST / 2) {  // station guys not at home always processed until 250 units away.
		AIDoRandomPatrol (pObj, pStateInfo->pLocalInfo);
		RETVAL (1)
		}
	}
else if ((pStateInfo->pStaticInfo->behavior != AIB_STILL) && !pStateInfo->pLocalInfo->nPrevVisibility && ((gameData.aiData.target.xDist >> 7) > pStateInfo->pLocalInfo->timeSinceProcessed)) {  // 128 units away (6.4 segments) processed after 1 second.
	AIDoRandomPatrol (pObj, pStateInfo->pLocalInfo);
	RETVAL (1)
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIProducerHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->bMultiGame || (SEGMENT (pObj->info.nSegment)->m_function != SEGMENT_FUNC_ROBOTMAKER))
	RETVAL (0)
if (!gameData.producerData.producers [SEGMENT (pObj->info.nSegment)->m_value].bEnabled)
	RETVAL (0)
AIFollowPath (pObj, 1, 1, NULL);    // 1 = player is visible, which might be a lie, but it works.
RETVAL (1)
}

// --------------------------------------------------------------------------------------------------------------------
// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.

int32_t AIApproachHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
if (pStateInfo->bMultiGame || pStateInfo->pRobotInfo->companion || pStateInfo->pRobotInfo->thief)
	RETVAL (0)
if ((pStateInfo->pStaticInfo->behavior == AIB_SNIPE) || (pStateInfo->pStaticInfo->behavior == AIB_RUN_FROM) || (pStateInfo->pStaticInfo->behavior == AIB_STILL))
	RETVAL (0)
if (gameData.aiData.nOverallAgitation < 71)
	RETVAL (0)
//if (!gameOpts->gameplay.nAIAggressivity || (pStateInfo->pLocalInfo->targetAwarenessType < PA_RETURN_FIRE) || (pStateInfo->pLocalInfo->mode == AIM_FOLLOW_PATH))
 {
	if ((gameData.aiData.target.xDist >= MAX_REACTION_DIST) || (RandShort () >= gameData.timeData.xFrame / 4))
		RETVAL (0)
	if (RandShort () * (gameData.aiData.nOverallAgitation - 40) <= I2X (5))
		RETVAL (0)
	}
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
if (gameOpts->gameplay.nAIAggressivity && (pStateInfo->pLocalInfo->mode == AIM_FOLLOW_PATH) && (pStateInfo->pLocalInfo->nGoalSegment == gameData.aiData.target.nBelievedSeg)) {
#if DBG
	if (pObj->Index () == nDbgObj)
		BRP;
#endif
	if (pObj->info.nSegment == pStateInfo->pLocalInfo->nGoalSegment) {
		pStateInfo->pLocalInfo->mode = AIM_IDLING;
		RETVAL (0)
		}
	}
CreatePathToTarget (pObj, 4 + gameData.aiData.nOverallAgitation / 8 + gameStates.app.nDifficultyLevel, 1);
RETVAL (1)
}

// --------------------------------------------------------------------------------------------------------------------

static CObject *NearestPlayerTarget (CObject* pAttacker)
{
#if 0
	int32_t		j;
	fix			curDist, minDist = MAX_WAKEUP_DIST, bestAngle = -1;
	CObject*		pCandidate, *pTarget = NULL;
	CFixVector	vPos = OBJPOS (pAttacker)->vPos;	
	CFixVector	vViewDir = pAttacker->info.position.mOrient.m.dir.f;

FORALL_PLAYER_OBJS (pCandidate, j) {
	if (pCandidate->Type () != OBJ_PLAYER) // skip ghost players
		continue;
	if (!PLAYER (pCandidate->Id ()).IsConnected ())
		continue;
	CFixVector vDir = OBJPOS (pCandidate)->vPos - vPos;
	curDist = vDir.Mag ();
	if ((curDist < MAX_WAKEUP_DIST /*/ 2*/) && (curDist < minDist) && ObjectToObjectVisibility (pAttacker, pCandidate, FQ_TRANSWALL, -1.0f)) {
		vDir /= curDist;
		fix angle = CFixVector::Dot (vViewDir, vDir);
		if (angle > bestAngle) {
			bestAngle = angle;
			pTarget = pCandidate;
			minDist = curDist;
			}
		}
	}
if (pTarget) {
	pAttacker->SetTarget (gameData.aiData.target.pObj = pTarget);
	gameData.aiData.target.vBelievedPos = OBJPOS (TARGETOBJ)->vPos;
	gameData.aiData.target.nBelievedSeg = OBJSEG (TARGETOBJ);
	CFixVector::NormalizedDir (gameData.aiData.target.vDir, gameData.aiData.target.vBelievedPos, pAttacker->info.position.vPos);
	return TARGETOBJ;
	}
#endif
return gameData.objData.pConsole;
}

// --------------------------------------------------------------------------------------------------------------------

static CObject *NearestRobotTarget (CObject* pAttacker, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	fix			curDist, minDist = MAX_WAKEUP_DIST, bestAngle = -1;
	CObject*		pCandidate, *pTarget = NULL;
	CFixVector	vPos = pAttacker->AttacksRobots ()
							 ? OBJPOS (pAttacker)->vPos	// find robot closest to this robot
							 : OBJPOS (OBJECT (N_LOCALPLAYER))->vPos;	// find robot closest to player
	CFixVector	vViewDir = pAttacker->info.position.mOrient.m.dir.f;

FORALL_ROBOT_OBJS (pCandidate) {
	if (pCandidate->Index () == pStateInfo->nObject)
		continue;
	CFixVector vDir = OBJPOS (pCandidate)->vPos - vPos;
	curDist = vDir.Mag ();
	if ((curDist < MAX_WAKEUP_DIST /*/ 2*/) && (curDist < minDist) && ObjectToObjectVisibility (pAttacker, pCandidate, FQ_TRANSWALL, -1.0f)) {
		vDir /= curDist;
		fix angle = CFixVector::Dot (vViewDir, vDir);
		if (angle > bestAngle) {
			bestAngle = angle;
			pTarget = pCandidate;
			minDist = curDist;
			}
		}
	}
if (pTarget) {
	pAttacker->SetTarget (gameData.aiData.target.pObj = pTarget);
	gameData.aiData.target.vBelievedPos = OBJPOS (TARGETOBJ)->vPos;
	gameData.aiData.target.nBelievedSeg = OBJSEG (TARGETOBJ);
	CFixVector::NormalizedDir (gameData.aiData.target.vDir, gameData.aiData.target.vBelievedPos, pAttacker->info.position.vPos);
	RETVAL (TARGETOBJ)
	}
RETVAL (NearestPlayerTarget (pAttacker))
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AITargetPosHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	CObject* pTarget = pObj->AttacksRobots () ? NearestRobotTarget (pObj, pStateInfo) : NearestPlayerTarget (pObj);

pObj->SetTarget (gameData.aiData.target.pObj = pTarget);
if ((pStateInfo->pStaticInfo->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (gameData.aiData.nLastMissileCamera != -1)) {
	gameData.aiData.target.vBelievedPos = OBJPOS (OBJECT (gameData.aiData.nLastMissileCamera))->vPos;
	RETVAL (0)
	}
if (pObj->AttacksRobots () || (!pStateInfo->pRobotInfo->thief && (RandShort () > 2 * pObj->AimDamage ()))) {
	pStateInfo->vVisPos = pObj->info.position.vPos;
	ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
	if (gameData.aiData.nTargetVisibility)
		RETVAL (0)
	}
pStateInfo->bVisAndVecComputed = 0;
if (TARGETOBJ->Type () == OBJ_ROBOT)
	gameData.aiData.target.vBelievedPos = pObj->FrontPosition ();
else if ((PLAYER (TARGETOBJ->Id ()).flags & PLAYER_FLAGS_CLOAKED) || (RandShort () > pObj->AimDamage ()))
	gameData.aiData.target.vBelievedPos = gameData.aiData.cloakInfo [pStateInfo->nObject & (MAX_AI_CLOAK_INFO - 1)].vLastPos;
else
	gameData.aiData.target.vBelievedPos = OBJPOS (TARGETOBJ)->vPos;
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AIFireHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if ((pStateInfo->nPrevVisibility || !(pStateInfo->nObjRef & 3)) && ReadyToFire (pStateInfo->pRobotInfo, pStateInfo->pLocalInfo) &&
	 ((pStateInfo->pLocalInfo->targetAwarenessType >= PA_RETURN_FIRE) ||//(pStateInfo->pStaticInfo->GOAL_STATE == AIS_FIRE) || //i.e. robot has been hit last frame
	  (gameData.aiData.target.xDist < MAX_WAKEUP_DIST)) &&
	 pStateInfo->pRobotInfo->nGuns && !pStateInfo->pRobotInfo->attackType) {
	// Since we passed ReadyToFire (), either pNextrimaryFire or nextSecondaryFire <= 0.  CalcGunPoint from relevant one.
	// If both are <= 0, we will deal with the mess in AIDoActualFiringStuff
	if (pStateInfo->pLocalInfo->pNextrimaryFire <= 0)
		pStateInfo->bHaveGunPos = CalcGunPoint (&gameData.aiData.vGunPoint, pObj, pStateInfo->pStaticInfo->CURRENT_GUN);
	else
		pStateInfo->bHaveGunPos = CalcGunPoint (&gameData.aiData.vGunPoint, pObj, (pStateInfo->pRobotInfo->nGuns == 1) ? 0 : !pStateInfo->pStaticInfo->CURRENT_GUN);
	pStateInfo->vVisPos = gameData.aiData.vGunPoint;
	}
else {
	pStateInfo->bHaveGunPos = 0;
	pStateInfo->vVisPos = pObj->info.position.vPos;
	gameData.aiData.vGunPoint.SetZero ();
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease player awareness due to the passage of time.

int32_t AIAwarenessHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo*	pStaticInfo = pStateInfo->pStaticInfo;
	tAILocalInfo*	pLocalInfo = pStateInfo->pLocalInfo;

if (pLocalInfo->targetAwarenessType) {
	if ((pLocalInfo->targetAwarenessType < PA_RETURN_FIRE) || (pStateInfo->nPrevVisibility < 2)) {
		if (pLocalInfo->targetAwarenessTime > 0) {
			pLocalInfo->targetAwarenessTime -= gameData.timeData.xFrame;
			if (pLocalInfo->targetAwarenessTime <= 0) {
				pLocalInfo->targetAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][0];   //new: 11/05/94
				pLocalInfo->targetAwarenessType--;          //new: 11/05/94
				}
			}
		else {
			pLocalInfo->targetAwarenessType--;
			pLocalInfo->targetAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][0];
			//pStaticInfo->GOAL_STATE = AIS_REST;
			}
		}
	}
else
	pStaticInfo->GOAL_STATE = AIS_REST;                     //new: 12/13/94
if (gameData.aiData.nMaxAwareness < pLocalInfo->targetAwarenessType)
	gameData.aiData.nMaxAwareness = pLocalInfo->targetAwarenessType;
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease player awareness due to the passage of time.

int32_t AIPlayerDeadHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo*	pStaticInfo = pStateInfo->pStaticInfo;
	tAILocalInfo*	pLocalInfo = pStateInfo->pLocalInfo;

if (!gameStates.app.bPlayerIsDead || pLocalInfo->targetAwarenessType)
	RETVAL (0)
if ((gameData.aiData.target.xDist >= MAX_REACTION_DIST) || (RandShort () >= gameData.timeData.xFrame / 8))
	RETVAL (0)
if ((pStaticInfo->behavior == AIB_STILL) || (pStaticInfo->behavior == AIB_RUN_FROM))
	RETVAL (0)
if (!AILocalPlayerControlsRobot (pObj, 30))
	RETVAL (1)
AIMultiSendRobotPos (pStateInfo->nObject, -1);
if (!((pLocalInfo->mode == AIM_FOLLOW_PATH) && (pStaticInfo->nCurPathIndex < pStaticInfo->nPathLength - 1)))
	if ((pStaticInfo->behavior != AIB_SNIPE) && (pStaticInfo->behavior != AIB_RUN_FROM)) {
		if (gameData.aiData.target.xDist < I2X (30))
			CreateNSegmentPath (pObj, 5, 1);
		else
			CreatePathToTarget (pObj, 20, 1);
		}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// If the robot can see you, increase his awareness of you.
// This prevents the problem of a robot looking right at you but doing nothing.
// Assert (gameData.aiData.nTargetVisibility != -1); // Means it didn't get initialized!

int32_t AIWakeupHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (!gameData.aiData.nTargetVisibility &&
	 (pStateInfo->pLocalInfo->targetAwarenessType < PA_WEAPON_WALL_COLLISION) &&
	 (gameData.aiData.target.xDist > MAX_WAKEUP_DIST)) {
	AIDoRandomPatrol (pObj, pStateInfo->pLocalInfo);
	RETVAL (1)
	}
ComputeVisAndVec (pObj, &pStateInfo->vVisPos, pStateInfo->pLocalInfo, pStateInfo->pRobotInfo, &pStateInfo->bVisAndVecComputed, MAX_REACTION_DIST);
if ((gameData.aiData.nTargetVisibility == 2) && (pStateInfo->pStaticInfo->behavior != AIB_FOLLOW) && !pStateInfo->pRobotInfo->thief) {
	if (!pStateInfo->pLocalInfo->targetAwarenessType) {
		if (pStateInfo->pStaticInfo->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
			pStateInfo->pStaticInfo->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else
			pStateInfo->pLocalInfo->targetAwarenessType = PA_PLAYER_COLLISION;
		}
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease player awareness due to the passage of time.

int32_t AINewGoalHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo*	pStaticInfo = pStateInfo->pStaticInfo;
	tAILocalInfo*	pLocalInfo = pStateInfo->pLocalInfo;

if (pLocalInfo->targetAwarenessType) {
	pStateInfo->nNewGoalState = aiTransitionTable [pLocalInfo->targetAwarenessType - 1][pStaticInfo->CURRENT_STATE][pStaticInfo->GOAL_STATE];
#if DBG
	if (pStateInfo->nNewGoalState == AIS_LOCK)
		BRP;
#endif
	if (pLocalInfo->targetAwarenessType == PA_WEAPON_ROBOT_COLLISION) {
		// Decrease awareness, else this robot will flinch every frame.
		pLocalInfo->targetAwarenessType--;
		pLocalInfo->targetAwarenessTime = xAwarenessTimes [gameOpts->gameplay.nAIAwareness][2] * (gameStates.app.nDifficultyLevel + 1);
		}
	if (pStateInfo->nNewGoalState == AIS_ERROR)
		pStateInfo->nNewGoalState = AIS_REST;
	if (pStaticInfo->CURRENT_STATE == AIS_NONE)
		pStaticInfo->CURRENT_STATE = AIS_REST;
	pStaticInfo->GOAL_STATE = pStateInfo->nNewGoalState;
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// Decrease player awareness due to the passage of time.

int32_t AIFireGunsHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
if (pStateInfo->pStaticInfo->GOAL_STATE == AIS_FIRE) {
	tAILocalInfo *pLocalInfo = pStateInfo->pLocalInfo;
	int32_t i, nGuns = pStateInfo->pRobotInfo->nGuns;
	for (i = 0; i < nGuns; i++)
		pLocalInfo->goalState [i] = AIS_FIRE;
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// If a guy hasn't animated to the firing state, but his pNextrimaryFire says ok to fire, bash him there

int32_t AIForceFireHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAIStaticInfo	*pStaticInfo = pStateInfo->pStaticInfo;

if (ReadyToFire (pStateInfo->pRobotInfo, pStateInfo->pLocalInfo) && (pStaticInfo->GOAL_STATE == AIS_FIRE))
	pStaticInfo->CURRENT_STATE = AIS_FIRE;
if ((pStaticInfo->GOAL_STATE != AIS_FLINCH) && (pObj->info.nId != ROBOT_BRAIN)) {
	if ((uint16_t) pStaticInfo->CURRENT_STATE < AIS_ERROR)
		RETVAL (aisHandler1 [pStaticInfo->CURRENT_STATE] (pObj, pStateInfo))
	else
		RETVAL (AISDefaultHandler1 (pObj, pStateInfo))
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------
// If retry count not 0, then add it into nConsecutiveRetries.
// If it is 0, cut down nConsecutiveRetries.
// This is largely a hack to speed up physics and deal with stupid
// AI.  This is low level communication between systems of a sort
// that should not be done.

int32_t AIRetryHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAILocalInfo	*pLocalInfo = pStateInfo->pLocalInfo;

if (!pLocalInfo->nRetryCount || pStateInfo->bMultiGame)
	pLocalInfo->nConsecutiveRetries /= 2;
else {
	pLocalInfo->nConsecutiveRetries += pLocalInfo->nRetryCount;
	pLocalInfo->nRetryCount = 0;
	if (pLocalInfo->nConsecutiveRetries > 3) {
		if ((uint16_t) pLocalInfo->mode <= AIM_GOTO_OBJECT)
			RETVAL (aimHandler1 [pLocalInfo->mode] (pObj, pStateInfo))
		pLocalInfo->nConsecutiveRetries = 0;
		}
	}
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

int32_t AINextFireHandler (CObject *pObj, tAIStateInfo *pStateInfo)
{
ENTER (1, 0);
	tAILocalInfo	*pLocalInfo = pStateInfo->pLocalInfo;

if (pLocalInfo->pNextrimaryFire > -I2X (8) * gameStates.gameplay.slowmo [0].fSpeed)
	pLocalInfo->pNextrimaryFire -= (fix) (gameData.timeData.xFrame / gameStates.gameplay.slowmo [0].fSpeed);
if (pStateInfo->pRobotInfo->nSecWeaponType != -1) {
	if (pLocalInfo->nextSecondaryFire > -I2X (8) * gameStates.gameplay.slowmo [0].fSpeed)
		pLocalInfo->nextSecondaryFire -= (fix) (gameData.timeData.xFrame / gameStates.gameplay.slowmo [0].fSpeed);
	}
else
	pLocalInfo->nextSecondaryFire = I2X (8);
if (pLocalInfo->timeSinceProcessed < I2X (256))
	pLocalInfo->timeSinceProcessed += gameData.timeData.xFrame;
RETVAL (0)
}

// --------------------------------------------------------------------------------------------------------------------

void DoAIFrame (CObject *pObj)
{
ENTER (1, 0);
	tAIStateInfo	si;

#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
Assert (pObj->info.nSegment != -1);
si.nObject = pObj->Index ();
si.nObjRef = si.nObject ^ gameData.appData.nFrameCount;
si.pStaticInfo = &pObj->cType.aiInfo;
si.pLocalInfo = gameData.aiData.localInfo + si.nObject;
si.bHaveGunPos = 0;
si.bVisAndVecComputed = 0;
if (!(si.pRobotInfo = ROBOTINFO (pObj)))
	RETURN
si.bMultiGame = !IsRobotGame;

#if 0 //DBG
gameData.aiData.target.pObj = NULL;
if (si.pStaticInfo->behavior == AIB_STILL)
	si.pStaticInfo = si.pStaticInfo;
#endif
gameData.aiData.nTargetVisibility = -1;
si.pLocalInfo->nextActionTime -= gameData.timeData.xFrame;

if (si.pStaticInfo->SKIP_AI_COUNT) {
	si.pStaticInfo->SKIP_AI_COUNT--;
	if (pObj->mType.physInfo.flags & PF_USES_THRUST) {
		pObj->mType.physInfo.rotThrust.v.coord.x = 15 * pObj->mType.physInfo.rotThrust.v.coord.x / 16;
		pObj->mType.physInfo.rotThrust.v.coord.y = 15 * pObj->mType.physInfo.rotThrust.v.coord.y / 16;
		pObj->mType.physInfo.rotThrust.v.coord.z = 15 * pObj->mType.physInfo.rotThrust.v.coord.z / 16;
		if (!si.pStaticInfo->SKIP_AI_COUNT)
			pObj->mType.physInfo.flags &= ~PF_USES_THRUST;
		}
	RETURN
	}

Assert (si.pRobotInfo->always_0xabcd == 0xabcd);
#if 0//DBG
if (!si.pRobotInfo->bossFlag)
	RETURN
#endif
if (DoAnyRobotDyingFrame (pObj))
	RETURN

// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
// Else, you can turn a big nasty robot into a wimp by firing flares at it.
// This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
if (((si.pStaticInfo->GOAL_STATE == AIS_FLINCH) || (si.pLocalInfo->targetAwarenessType == PA_RETURN_FIRE)) &&
	 ReadyToFire (si.pRobotInfo, si.pLocalInfo)) {
	si.pStaticInfo->GOAL_STATE = AIS_FIRE;
	}
if ((si.pStaticInfo->behavior < MIN_BEHAVIOR) || (si.pStaticInfo->behavior > MAX_BEHAVIOR))
	si.pStaticInfo->behavior = AIB_NORMAL;
//Assert (pObj->info.nId < gameData.botData.nTypes [gameStates.app.bD1Data]);
if (AINextFireHandler (pObj, &si))
	RETURN

si.nPrevVisibility = si.pLocalInfo->nPrevVisibility;    //  Must get this before we toast the master copy!
// If only awake because of a camera, make that the believed player position.
if (AITargetPosHandler (pObj, &si))
	RETURN
gameData.aiData.target.xDist = CFixVector::Dist (gameData.aiData.target.vBelievedPos, pObj->info.position.vPos);
// If this robot can fire, compute visibility from gun position.
// Don't want to compute visibility twice, as it is expensive.  (So is call to CalcGunPoint).
if (AIFireHandler (pObj, &si))
	RETURN
if (AIApproachHandler (pObj, &si))
	RETURN
if (AIRetryHandler (pObj, &si))
	RETURN
// If in materialization center, exit
if (AIProducerHandler (pObj, &si))
	RETURN
if (AIAwarenessHandler (pObj, &si))
	RETURN
if (AIPlayerDeadHandler (pObj, &si))
	RETURN
if (AIBumpHandler (pObj, &si))
	RETURN
if ((si.pStaticInfo->GOAL_STATE == AIS_FLINCH) && (si.pStaticInfo->CURRENT_STATE == AIS_FLINCH))
	si.pStaticInfo->GOAL_STATE = AIS_LOCK;
if (AIAnimationHandler (pObj, &si))
	RETURN
if (AIBossHandler (pObj, &si))
	RETURN
if (AIStationaryHandler (pObj, &si))
	RETURN
// Reset time since processed, but skew OBJECTS so not everything
// processed synchronously, else we get fast frames with the
// occasional very slow frame.
// AI_procTime = si.pLocalInfo->timeSinceProcessed;
si.pLocalInfo->timeSinceProcessed = -((si.nObject & 0x03) * gameData.timeData.xFrame) / 2;
if (AIBrainHandler (pObj, &si))
	RETURN
if (AISnipeHandler (pObj, &si))
	RETURN
if (AIBuddyHandler (pObj, &si))
	RETURN
if (AIThiefHandler (pObj, &si))
	RETURN
// More special ability stuff, but based on a property of a robot, not its ID.
if ((uint16_t) si.pLocalInfo->mode > AIM_THIEF_WAIT)
	AIMDefaultHandler2 (pObj, &si);
else if (aimHandler2 [si.pLocalInfo->mode] (pObj, &si))
	RETURN
if (AIWakeupHandler (pObj, &si))
	RETURN

if (!gameData.aiData.bObjAnimates)
	si.pStaticInfo->CURRENT_STATE = si.pStaticInfo->GOAL_STATE;

Assert (si.pLocalInfo->targetAwarenessType <= AIE_MAX);
Assert (si.pStaticInfo->CURRENT_STATE < AIS_MAX);
Assert (si.pStaticInfo->GOAL_STATE < AIS_MAX);

if (AINewGoalHandler (pObj, &si))
	RETURN
// If new state = fire, then set all gun states to fire.
if (AIFireGunsHandler (pObj, &si))
	RETURN
if (AIForceFireHandler (pObj, &si))
	RETURN
// Switch to next gun for next fire.
if (!gameData.aiData.nTargetVisibility) {
	if (++(si.pStaticInfo->CURRENT_GUN) >= si.pRobotInfo->nGuns) {
		if ((si.pRobotInfo->nGuns == 1) || (si.pRobotInfo->nSecWeaponType == -1))  // Two weapon types hack.
			si.pStaticInfo->CURRENT_GUN = 0;
		else
			si.pStaticInfo->CURRENT_GUN = 1;
		}
	AIDoRandomPatrol (pObj, si.pLocalInfo);
	}
RETURN
}

//	-------------------------------------------------------------------------------------------------
