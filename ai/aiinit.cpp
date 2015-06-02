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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "descent.h"
#include "error.h"
#include "input.h"
#include "network.h"
#include "multibot.h"
#include "escort.h"
#include "headlight.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use gameData.timeData.xFrame because robots do not get moved every frame.

// ---------------------------------------------------------
//	On entry, gameData.botData.nTypes had darn sure better be set.
//	Mallocs gameData.botData.nTypes tRobotInfo structs into global gameData.botData.pInfo.
void InitAISystem (void)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
int32_t AIBehaviorToMode (int32_t behavior)
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
//	initial_mode == -1 means leave mode unchanged.
void InitAIObject (int16_t nObject, int16_t behavior, int16_t nHideSegment)
{
ENTER (1, 0);
	CObject			*pObj = OBJECT (nObject);
	tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
	tAILocalInfo	*pLocalInfo = gameData.aiData.localInfo + nObject;
	tRobotInfo		*pRobotInfo = ROBOTINFO (pObj);

#if DBG
if (!pObj->IsRobot ())
	BRP;
#endif
if (!pRobotInfo)
	RETURN
if (behavior == AIB_STATIC) {
	pObj->info.controlType = CT_NONE;
	pObj->info.movementType = MT_NONE;
	}
if (behavior == 0) {
	behavior = AIB_NORMAL;
	pStaticInfo->behavior = (uint8_t) behavior;
	}
//	mode is now set from the Robot dialog, so this should get overwritten.
pLocalInfo->mode = AIM_IDLING;
pLocalInfo->nPrevVisibility = 0;
if (behavior != -1) {
	pStaticInfo->behavior = (uint8_t) behavior;
	pLocalInfo->mode = AIBehaviorToMode (pStaticInfo->behavior);
	}
else if (!((pStaticInfo->behavior >= MIN_BEHAVIOR) && (pStaticInfo->behavior <= MAX_BEHAVIOR))) {
#if TRACE
	console.printf (CON_DBG, " [obj %i -> Normal] ", nObject);
#endif
	pStaticInfo->behavior = AIB_NORMAL;
	}
if (pRobotInfo->companion) {
	pLocalInfo->mode = AIM_GOTO_PLAYER;
	gameData.escortData.nKillObject = -1;
	}
if (pRobotInfo->thief) {
	pStaticInfo->behavior = AIB_SNIPE;
	pLocalInfo->mode = AIM_THIEF_WAIT;
	}
if (pRobotInfo->attackType) {
	pStaticInfo->behavior = AIB_NORMAL;
	pLocalInfo->mode = AIBehaviorToMode (pStaticInfo->behavior);
	}
pObj->mType.physInfo.velocity.SetZero ();
// -- pLocalInfo->waitTime = I2X (5);
pLocalInfo->targetAwarenessTime = 0;
pLocalInfo->targetAwarenessType = 0;
pStaticInfo->GOAL_STATE = AIS_SEARCH;
pStaticInfo->CURRENT_STATE = AIS_REST;
pLocalInfo->timeTargetSeen = gameData.timeData.xGame;
pLocalInfo->nextMiscSoundTime = gameData.timeData.xGame;
pLocalInfo->timeTargetSoundAttacked = gameData.timeData.xGame;
if ((behavior == AIB_SNIPE) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM) || (behavior == AIB_FOLLOW)) {
	pStaticInfo->nHideSegment = nHideSegment;
	pLocalInfo->nGoalSegment = nHideSegment;
	pStaticInfo->nHideIndex = -1;			// This means the path has not yet been created.
	pStaticInfo->nPathLength = 0;
	pStaticInfo->nCurPathIndex = 0;
	}
pStaticInfo->SKIP_AI_COUNT = 0;
pStaticInfo->CLOAKED = (pRobotInfo->cloakType == RI_CLOAKED_ALWAYS);
pObj->mType.physInfo.flags |= (PF_BOUNCES | PF_TURNROLL);
pStaticInfo->REMOTE_OWNER = -1;
pStaticInfo->bDyingSoundPlaying = 0;
pStaticInfo->xDyingStartTime = 0;
RETURN
}


// ---------------------------------------------------------------------------------------------------------------------

void InitAIObjects (void)
{
ENTER (1, 0);
	int16_t		nBosses = 0;
	CObject		*pObj;

gameData.aiData.target.pObj = NULL;
gameData.aiData.freePointSegs = gameData.aiData.routeSegs.Buffer ();
FORALL_OBJS (pObj) {
	if (pObj->IsRobot ()) {
		if (pObj->info.controlType == CT_AI)
			InitAIObject (pObj->Index (), pObj->cType.aiInfo.behavior, pObj->cType.aiInfo.nHideSegment);
		if (pObj->IsBoss ()) {
			if (nBosses < (int32_t) gameData.bossData.ToS () || gameData.bossData.Grow ())
				gameData.bossData [nBosses++].Setup (pObj->Index ());
			}
		}
	}

int32_t i = gameData.bossData.Count () - nBosses;
if (0 < i) {
	gameData.bossData.Shrink (uint32_t (i));
	extraGameInfo [0].nBossCount [0] -= i;
	extraGameInfo [0].nBossCount [1] -= i;
	}
gameData.aiData.bInitialized = 1;
AIDoCloakStuff ();
InitBuddyForLevel ();
RETURN
}

// ---------------------------------------------------------------------------------------------------------------------

int32_t		nDiffSave = 1;
fix		Firing_wait_copy [MAX_ROBOT_TYPES];
fix		Firing_wait2_copy [MAX_ROBOT_TYPES];
int8_t		RapidfireCount_copy [MAX_ROBOT_TYPES];

void DoLunacyOn (void)
{
ENTER (1, 0);
if (gameStates.app.bLunacy)	//already on
	RETURN
gameStates.app.bLunacy = 1;
nDiffSave = gameStates.app.nDifficultyLevel;
gameStates.app.nDifficultyLevel = DIFFICULTY_LEVEL_COUNT-1;
for (int32_t i = 0; i < MAX_ROBOT_TYPES; i++) {
	Firing_wait_copy [i] = gameData.botData.pInfo [i].primaryFiringWait [DIFFICULTY_LEVEL_COUNT-1];
	Firing_wait2_copy [i] = gameData.botData.pInfo [i].secondaryFiringWait [DIFFICULTY_LEVEL_COUNT-1];
	RapidfireCount_copy [i] = gameData.botData.pInfo [i].nRapidFireCount [DIFFICULTY_LEVEL_COUNT-1];
	gameData.botData.pInfo [i].primaryFiringWait [DIFFICULTY_LEVEL_COUNT-1] = gameData.botData.pInfo [i].primaryFiringWait [1];
	gameData.botData.pInfo [i].secondaryFiringWait [DIFFICULTY_LEVEL_COUNT-1] = gameData.botData.pInfo [i].secondaryFiringWait [1];
	gameData.botData.pInfo [i].nRapidFireCount [DIFFICULTY_LEVEL_COUNT-1] = gameData.botData.pInfo [i].nRapidFireCount [1];
	}
RETURN
}

// ---------------------------------------------------------------------------------------------------------------------

void DoLunacyOff (void)
{
ENTER (1, 0);
if (!gameStates.app.bLunacy)	//already off
	RETURN
gameStates.app.bLunacy = 0;
for (int32_t i = 0; i < MAX_ROBOT_TYPES; i++) {
	gameData.botData.pInfo [i].primaryFiringWait [DIFFICULTY_LEVEL_COUNT-1] = Firing_wait_copy [i];
	gameData.botData.pInfo [i].secondaryFiringWait [DIFFICULTY_LEVEL_COUNT-1] = Firing_wait2_copy [i];
	gameData.botData.pInfo [i].nRapidFireCount [DIFFICULTY_LEVEL_COUNT-1] = RapidfireCount_copy [i];
	}
gameStates.app.nDifficultyLevel = nDiffSave;
RETURN
}

// --------------------------------------------------------------------------------------------------------------------
//	Call this each time the player starts a new ship.
void InitAIForShip (void)
{
ENTER (1, 0);
for (int32_t i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.aiData.cloakInfo [i].lastTime = gameData.timeData.xGame;
	if (gameData.objData.pConsole) {
		gameData.aiData.cloakInfo [i].nLastSeg = OBJSEG (gameData.objData.pConsole);
		gameData.aiData.cloakInfo [i].vLastPos = OBJPOS (gameData.objData.pConsole)->vPos;
		}
	else {
		gameData.aiData.cloakInfo [i].nLastSeg = -1;
		gameData.aiData.cloakInfo [i].vLastPos = CFixVector::ZERO;
		}
	}
RETURN
}

//	-------------------------------------------------------------------------------------------------
// Initializations to be performed for all robots for a new level.
void InitRobotsForLevel (void)
{
ENTER (1, 0);
gameData.aiData.nOverallAgitation = 0;
gameStates.gameplay.bFinalBossIsDead = 0;
gameData.escortData.nObjNum = 0;
gameData.escortData.bMayTalk = 0;
gameData.physicsData.xBossInvulDot = I2X (1)/4 - I2X (gameStates.app.nDifficultyLevel)/8;
for (uint32_t i = 0; i < gameData.bossData.Count (); i++)
	gameData.bossData [i].m_nDyingStartTime = 0;
RETURN
}

// ----------------------------------------------------------------------------

void InitAIFrame (void)
{
ENTER (1, 0);
	int32_t abState;

if (gameData.aiData.nMaxAwareness < PA_PLAYER_COLLISION)
	gameData.aiData.target.vLastPosFiredAt.SetZero ();
if (!gameData.aiData.target.vLastPosFiredAt.IsZero ())
	gameData.aiData.target.nDistToLastPosFiredAt =
		CFixVector::Dist (gameData.aiData.target.vLastPosFiredAt, gameData.aiData.target.vBelievedPos);
else
	gameData.aiData.target.nDistToLastPosFiredAt = I2X (10000);
abState = gameData.physicsData.xAfterburnerCharge && controls [0].afterburnerState &&
			  (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER);
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) || HeadlightIsOn (-1) || abState)
	AIDoCloakStuff ();
gameData.aiData.nMaxAwareness = 0;
RETURN
}

//	---------------------------------------------------------------
// eof

