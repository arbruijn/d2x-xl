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

#include "inferno.h"
#include "error.h"
#include "input.h"
#include "network.h"
#include "multibot.h"
#include "escort.h"
#include "headlight.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
//#define _DEBUG
#if DBG
#include "string.h"
#include <time.h>
#endif

//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use gameData.time.xFrame because robots do not get moved every frame.

// ---------------------------------------------------------
//	On entry, gameData.bots.nTypes had darn sure better be set.
//	Mallocs gameData.bots.nTypes tRobotInfo structs into global gameData.bots.infoP.
void InitAISystem (void)
{
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
//	initial_mode == -1 means leave mode unchanged.
void InitAIObject (short nObject, short behavior, short nHideSegment)
{
	CObject		*objP = OBJECTS + nObject;
	tAIStaticInfo	*aiP = &objP->cType.aiInfo;
	tAILocalInfo		*ailP = gameData.ai.localInfo + nObject;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->info.nId);

Assert (nObject >= 0);
if (behavior == AIB_STATIC) {
	objP->info.controlType = CT_NONE;
	objP->info.movementType = MT_NONE;
	}
if (behavior == 0) {
	behavior = AIB_NORMAL;
	aiP->behavior = (ubyte) behavior;
	}
//	mode is now set from the Robot dialog, so this should get overwritten.
ailP->mode = AIM_IDLING;
ailP->nPrevVisibility = 0;
if (behavior != -1) {
	aiP->behavior = (ubyte) behavior;
	ailP->mode = AIBehaviorToMode (aiP->behavior);
	}
else if (!((aiP->behavior >= MIN_BEHAVIOR) && (aiP->behavior <= MAX_BEHAVIOR))) {
#if TRACE
	console.printf (CON_DBG, " [obj %i -> Normal] ", nObject);
#endif
	aiP->behavior = AIB_NORMAL;
	}
if (botInfoP->companion) {
	ailP->mode = AIM_GOTO_PLAYER;
	gameData.escort.nKillObject = -1;
	}
if (botInfoP->thief) {
	aiP->behavior = AIB_SNIPE;
	ailP->mode = AIM_THIEF_WAIT;
	}
if (botInfoP->attackType) {
	aiP->behavior = AIB_NORMAL;
	ailP->mode = AIBehaviorToMode (aiP->behavior);
	}
objP->mType.physInfo.velocity.SetZero ();
// -- ailP->waitTime = I2X (5);
ailP->playerAwarenessTime = 0;
ailP->playerAwarenessType = 0;
aiP->GOAL_STATE = AIS_SEARCH;
aiP->CURRENT_STATE = AIS_REST;
ailP->timePlayerSeen = gameData.time.xGame;
ailP->nextMiscSoundTime = gameData.time.xGame;
ailP->timePlayerSoundAttacked = gameData.time.xGame;
if ((behavior == AIB_SNIPE) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM) || (behavior == AIB_FOLLOW)) {
	aiP->nHideSegment = nHideSegment;
	ailP->nGoalSegment = nHideSegment;
	aiP->nHideIndex = -1;			// This means the path has not yet been created.
	aiP->nCurPathIndex = 0;
	}
aiP->SKIP_AI_COUNT = 0;
aiP->CLOAKED = (botInfoP->cloakType == RI_CLOAKED_ALWAYS);
objP->mType.physInfo.flags |= (PF_BOUNCE | PF_TURNROLL);
aiP->REMOTE_OWNER = -1;
aiP->bDyingSoundPlaying = 0;
aiP->xDyingStartTime = 0;
}


// ---------------------------------------------------------------------------------------------------------------------

void InitAIObjects (void)
{
	short		i, j;
	CObject*	objP;

gameData.ai.freePointSegs = gameData.ai.routeSegs.Buffer ();
gameData.bosses.Destroy ();
gameData.bosses.Create ();
for (i = j = 0, objP = OBJECTS.Buffer (); i < LEVEL_OBJECTS; i++, objP++) {
	if (objP->info.controlType == CT_AI)
		InitAIObject (i, objP->cType.aiInfo.behavior, objP->cType.aiInfo.nHideSegment);
	if ((objP->info.nType == OBJ_ROBOT) && (ROBOTINFO (objP->info.nId).bossFlag))
		gameData.bosses [j++].m_nObject = i;
	}
gameData.bosses.Setup ();
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
	Firing_wait_copy [i] = gameData.bots.infoP [i].primaryFiringWait [NDL-1];
	Firing_wait2_copy [i] = gameData.bots.infoP [i].secondaryFiringWait [NDL-1];
	RapidfireCount_copy [i] = gameData.bots.infoP [i].nRapidFireCount [NDL-1];
	gameData.bots.infoP [i].primaryFiringWait [NDL-1] = gameData.bots.infoP [i].primaryFiringWait [1];
	gameData.bots.infoP [i].secondaryFiringWait [NDL-1] = gameData.bots.infoP [i].secondaryFiringWait [1];
	gameData.bots.infoP [i].nRapidFireCount [NDL-1] = gameData.bots.infoP [i].nRapidFireCount [1];
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
	gameData.bots.infoP [i].primaryFiringWait [NDL-1] = Firing_wait_copy [i];
	gameData.bots.infoP [i].secondaryFiringWait [NDL-1] = Firing_wait2_copy [i];
	gameData.bots.infoP [i].nRapidFireCount [NDL-1] = RapidfireCount_copy [i];
	}
gameStates.app.nDifficultyLevel = nDiffSave;
}

// --------------------------------------------------------------------------------------------------------------------
//	Call this each time the CPlayerData starts a new ship.
void InitAIForShip (void)
{
for (int i = 0; i < MAX_AI_CLOAK_INFO; i++) {
	gameData.ai.cloakInfo [i].lastTime = gameData.time.xGame;
	gameData.ai.cloakInfo [i].nLastSeg = OBJSEG (gameData.objs.consoleP);
	gameData.ai.cloakInfo [i].vLastPos = OBJPOS (gameData.objs.consoleP)->vPos;
	}
}

//	-------------------------------------------------------------------------------------------------
// Initializations to be performed for all robots for a new level.
void InitRobotsForLevel (void)
{
gameData.ai.nOverallAgitation = 0;
gameStates.gameplay.bFinalBossIsDead=0;
gameData.escort.nObjNum = 0;
gameData.escort.bMayTalk = 0;
gameData.physics.xBossInvulDot = I2X (1)/4 - I2X (gameStates.app.nDifficultyLevel)/8;
for (uint i = 0; i < gameData.bosses.Count (); i++)
	gameData.bosses [i].m_nDyingStartTime = 0;
}

// ----------------------------------------------------------------------------

void InitAIFrame (void)
{
	int abState;

if (gameData.ai.nMaxAwareness < PA_PLAYER_COLLISION)
	gameData.ai.vLastPlayerPosFiredAt.SetZero ();
if (!gameData.ai.vLastPlayerPosFiredAt.IsZero())
	gameData.ai.nDistToLastPlayerPosFiredAt =
		CFixVector::Dist(gameData.ai.vLastPlayerPosFiredAt, gameData.ai.vBelievedPlayerPos);
else
	gameData.ai.nDistToLastPlayerPosFiredAt = I2X (10000);
abState = gameData.physics.xAfterburnerCharge && Controls [0].afterburnerState &&
			  (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER);
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) || HeadlightIsOn (-1) || abState)
	AIDoCloakStuff ();
gameData.ai.nMaxAwareness = 0;
}

//	---------------------------------------------------------------
// eof

