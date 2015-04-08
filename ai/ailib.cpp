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
#include "segmath.h"
#include "network.h"
#include "multibot.h"
#include "headlight.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

int32_t	nRobotSoundVolume = DEFAULT_ROBOT_SOUND_VOLUME;

// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from object, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.target.vBelievedPos as player's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int32_t AICanSeeTarget (CObject *objP, CFixVector *vPos, fix fieldOfView, CFixVector *vVecToTarget)
{
	fix			dot;
	CHitQuery	hitQuery (FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_VISIBILITY | (objP->AttacksRobots () ? FQ_ANY_OBJECT : FQ_CHECK_PLAYER),
								 vPos, &gameData.ai.target.vBelievedPos, -1, objP->Index (), I2X (1) / 4, I2X (1) / 4, ++gameData.physics.bIgnoreObjFlag);

//	Assume that robot's gun tip is in same CSegment as robot's center.
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
if ((*vPos) != objP->info.position.vPos) {
	int16_t nSegment = FindSegByPos (*vPos, objP->info.nSegment, 1, 0);
	if (nSegment == -1) {
		hitQuery.nSegment = objP->info.nSegment;
		*vPos = objP->info.position.vPos;
#if TRACE
		console.printf (1, "Object %i, gun is outside mine, moving towards center.\n", objP->Index ());
#endif
		MoveTowardsSegmentCenter (objP);
		}
	else {
		if (nSegment != objP->info.nSegment)
			objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
		hitQuery.nSegment = nSegment;
		}
	}
else
	hitQuery.nSegment	= objP->info.nSegment;

gameData.ai.nHitType = FindHitpoint (hitQuery, gameData.ai.hitResult);
gameData.ai.vHitPos = gameData.ai.hitResult.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitResult.nSegment;
if ((gameData.ai.nHitType != HIT_OBJECT) || (gameData.ai.hitResult.nObject != TARGETOBJ->Index ()))
	return 0;
dot = CFixVector::Dot (*vVecToTarget, objP->info.position.mOrient.m.dir.f);
if (dot > fieldOfView - (gameData.ai.nOverallAgitation << 9))
	return 2;
if (gameOpts->gameplay.nAIAggressivity) {	// player visible at raised AI aggressivity when having headlight on
	 if ((TARGETOBJ->info.nType == OBJ_PLAYER) && 
		  (TARGETOBJ->info.nId == N_LOCALPLAYER) && 
		  HeadlightIsOn (-1))
		return 2;
	if (!TARGETOBJ->Cloaked () &&
		 (CFixVector::Dist (*vPos, OBJPOS (TARGETOBJ)->vPos) < 
		 ((3 + gameOpts->gameplay.nAIAggressivity) * (objP->info.xSize + TARGETOBJ->info.xSize)) / 2))
		return 2;
	}
return 1;
}

// ------------------------------------------------------------------------------------------------------------------

int32_t AICanFireAtTarget (CObject *objP, CFixVector *vGun, CFixVector *vTarget)
{
	fix			nSize;
	int16_t			nModel;

//	Assume that robot's gun tip is in same CSegment as robot's center.
if (vGun->IsZero ())
	return 0;
if (!extraGameInfo [IsMultiGame].bRobotsHitRobots)
	return 1;
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;

CHitQuery hitQuery (FQ_CHECK_OBJS | FQ_ANY_OBJECT | FQ_IGNORE_POWERUPS | FQ_TRANSPOINT | FQ_VISIBILITY,
						  vGun, vTarget, -1, objP->Index (), I2X (1), I2X (1), ++gameData.physics.bIgnoreObjFlag);

if (*vGun == objP->Position ())
	hitQuery.nSegment	= objP->info.nSegment;
else {
	int16_t nSegment = FindSegByPos (*vGun, objP->info.nSegment, 1, 0);
	if (nSegment == -1)
		return -1;
	if (nSegment != objP->info.nSegment)
		objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
	hitQuery.nSegment = nSegment;
	}
nModel = objP->rType.polyObjInfo.nModel;
nSize = objP->info.xSize;
objP->rType.polyObjInfo.nModel = -1;	//make sure sphere/hitbox and not hitbox/hitbox collisions get tested
objP->SetSize (I2X (2));					//chose some meaningful small size to simulate a weapon

gameData.ai.nHitType = FindHitpoint (hitQuery, gameData.ai.hitResult);
#if DBG
if (gameData.ai.nHitType == 0)
	FindHitpoint (hitQuery, gameData.ai.hitResult);
#endif
objP->rType.polyObjInfo.nModel = nModel;
gameData.ai.vHitPos = gameData.ai.hitResult.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitResult.nSegment;
objP->rType.polyObjInfo.nModel = nModel;
objP->SetSize (nSize);
return (gameData.ai.nHitType == HIT_NONE) || 
		 ((gameData.ai.nHitType == HIT_OBJECT) && objP->Target () && (objP->Target ()->Index () == gameData.ai.hitResult.nObject));
}

// --------------------------------------------------------------------------------------------------------------------

inline void LimitTargetVisibility (fix xMaxVisibleDist, tAILocalInfo *ailP)
{
#if 1
if ((xMaxVisibleDist > 0) && (gameData.ai.target.xDist > xMaxVisibleDist) && (ailP->targetAwarenessType < PA_RETURN_FIRE))
	gameData.ai.nTargetVisibility = 0;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely AICanSeeTarget would benefit from the
//	information of a normalized gameData.ai.target.vDir.
//	Return player visibility:
//		0		not visible
//		1		visible, but robot not looking at player (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		player is cloaked
//	If the player is cloaked, set gameData.ai.target.vDir based on time player cloaked and last uncloaked position.
//	Updates ailP->nPrevVisibility if player is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to gameData.ai.nTargetVisibility
void ComputeVisAndVec (CObject *objP, CFixVector *pos, tAILocalInfo *ailP, tRobotInfo *botInfoP, int32_t *flag, fix xMaxVisibleDist)
{
if (*flag)
	LimitTargetVisibility (xMaxVisibleDist, ailP);
else if (!TARGETOBJ->Appearing ()) {
	if (TARGETOBJ->Cloaked ()) {
		fix			deltaTime, dist;
		int32_t			nCloakIndex = (objP->Index ()) % MAX_AI_CLOAK_INFO;

		deltaTime = gameData.time.xGame - gameData.ai.cloakInfo [nCloakIndex].lastTime;
		if (deltaTime > I2X (2)) {
			CFixVector	vRand;

			gameData.ai.cloakInfo [nCloakIndex].lastTime = gameData.time.xGame;
			vRand = CFixVector::Random ();
			gameData.ai.cloakInfo [nCloakIndex].vLastPos += vRand * (8 * deltaTime);
			}
		dist = CFixVector::NormalizedDir (gameData.ai.target.vDir, gameData.ai.cloakInfo [nCloakIndex].vLastPos, *pos);
		gameData.ai.nTargetVisibility = AICanSeeTarget (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.ai.target.vDir);
		LimitTargetVisibility (xMaxVisibleDist, ailP);
#if DBG
		if (gameData.ai.nTargetVisibility == 2)
			gameData.ai.nTargetVisibility = gameData.ai.nTargetVisibility;
#endif
		if ((ailP->nextMiscSoundTime < gameData.time.xGame) && ((ailP->nextPrimaryFire < I2X (1)) || (ailP->nextSecondaryFire < I2X (1))) && (dist < I2X (20))) {
			ailP->nextMiscSoundTime = gameData.time.xGame + (RandShort () + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 1;
			audio.CreateSegmentSound (botInfoP->seeSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
			}
		}
	else {
		//	Compute expensive stuff -- gameData.ai.target.vDir and gameData.ai.nTargetVisibility
		CFixVector::NormalizedDir (gameData.ai.target.vDir, gameData.ai.target.vBelievedPos, *pos);
		if (gameData.ai.target.vDir.IsZero ()) {
			gameData.ai.target.vDir.v.coord.x = I2X (1);
			}
		gameData.ai.nTargetVisibility = AICanSeeTarget (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.ai.target.vDir);
		LimitTargetVisibility (xMaxVisibleDist, ailP);
#if DBG
		if (gameData.ai.nTargetVisibility == 2)
			gameData.ai.nTargetVisibility = gameData.ai.nTargetVisibility;
#endif
		//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
		//	see you without killing frame rate.
		tAIStaticInfo	*aiP = &objP->cType.aiInfo;
		if ((gameData.ai.nTargetVisibility == 2) && (ailP->nPrevVisibility != 2))
			if ((aiP->GOAL_STATE == AIS_REST) || (aiP->CURRENT_STATE == AIS_REST)) {
				aiP->GOAL_STATE = AIS_FIRE;
				aiP->CURRENT_STATE = AIS_FIRE;
				}

		if ((ailP->nPrevVisibility != gameData.ai.nTargetVisibility) && (gameData.ai.nTargetVisibility == 2)) {
			if (ailP->nPrevVisibility == 0) {
				if (ailP->timeTargetSeen + I2X (1)/2 < gameData.time.xGame) {
					// -- if (LOCALPLAYER.m_bExploded)
					// -- 	audio.CreateSegmentSound (botInfoP->tauntSound, objP->info.nSegment, 0, pos, 0 , nRobotSoundVolume);
					// -- else
						audio.CreateSegmentSound (botInfoP->seeSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
					ailP->timeTargetSoundAttacked = gameData.time.xGame;
					ailP->nextMiscSoundTime = gameData.time.xGame + I2X (1) + RandShort ()*4;
					}
				}
			else if (ailP->timeTargetSoundAttacked + I2X (1) / 4 < gameData.time.xGame) {
				// -- if (LOCALPLAYER.m_bExploded)
				// -- 	audio.CreateSegmentSound (botInfoP->tauntSound, objP->info.nSegment, 0, pos, 0 , nRobotSoundVolume);
				// -- else
					audio.CreateSegmentSound (botInfoP->attackSound, objP->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
				ailP->timeTargetSoundAttacked = gameData.time.xGame;
				}
			}

		if ((gameData.ai.nTargetVisibility == 2) && (ailP->nextMiscSoundTime < gameData.time.xGame)) {
			ailP->nextMiscSoundTime = gameData.time.xGame + (RandShort () + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 2;
			// -- if (LOCALPLAYER.m_bExploded)
			// -- 	audio.CreateSegmentSound (botInfoP->tauntSound, objP->info.nSegment, 0, pos, 0 , nRobotSoundVolume);
			// -- else
				audio.CreateSegmentSound (botInfoP->attackSound, objP->info.nSegment, 0, *pos, 0 , nRobotSoundVolume);
			}
		ailP->nPrevVisibility = gameData.ai.nTargetVisibility;
		}

	*flag = 1;

	//	@mk, 09/21/95: If player view is not obstructed and awareness is at least as high as a nearby collision,
	//	act is if robot is looking at player.
	if (ailP->targetAwarenessType >= PA_NEARBY_ROBOT_FIRED)
		if (gameData.ai.nTargetVisibility == 1)
			gameData.ai.nTargetVisibility = 2;
	if (gameData.ai.nTargetVisibility)
		ailP->timeTargetSeen = gameData.time.xGame;
	}
#if 0 //DBG
if (objP->Index () == nDbgObj) {
	HUDMessage (0, "vis: %d", gameData.ai.nTargetVisibility);
	}
#endif
}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable nType robot.
//	Brains, avoid robots, companions can open doors.
//	objP == NULL means treat as buddy.
int32_t AIDoorIsOpenable (CObject *objP, CSegment *segP, int16_t nSide)
{
	CWall	*wallP;

if (!IS_CHILD (segP->m_children [nSide]))
	return 0;		//trap -2 (exit CSide)
if (!(wallP = segP->Wall (nSide)))
	return 1;				//d:\temp\dm_testthen say it can't be opened
	//	The mighty console CObject can open all doors (for purposes of determining paths).
if (objP == gameData.objs.consoleP) {
	if (wallP->nType == WALL_DOOR)
		return 1;
	}
if ((objP == NULL) || (ROBOTINFO (objP->info.nId).companion == 1)) {
	int32_t	ailp_mode;

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

	//	If Buddy is returning to player, don't let him think he can get through triggered doors.
	//	It's only valid to think that if the player is going to get him through.  But if he's
	//	going to the player, the player is probably on the opposite CSide.
	if (objP)
		ailp_mode = gameData.ai.localInfo [objP->Index ()].mode;
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
		int32_t	nClip = wallP->nClip;

		if (nClip == -1)
			return 1;
		else if (gameData.walls.animP [nClip].flags & WCF_HIDDEN) {
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
			int32_t	nClip = wallP->nClip;

			if (nClip == -1)
				return 1;
			//	Buddy allowed to go through secret doors to get to player.
			else if ((ailp_mode != AIM_GOTO_PLAYER) && (gameData.walls.animP [nClip].flags & WCF_HIDDEN)) {
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
else if ((objP->info.nId == ROBOT_BRAIN) || (objP->cType.aiInfo.behavior == AIB_RUN_FROM) || (objP->cType.aiInfo.behavior == AIB_SNIPE)) {
	if (wallP) {
		if ((wallP->nType == WALL_DOOR) && (wallP->keys == KEY_NONE) && !(wallP->flags & WALL_DOOR_LOCKED))
			return 1;
		else if (wallP->keys != KEY_NONE) {	//	Allow bots to open doors to which player has keys.
			if (wallP->keys & LOCALPLAYER.flags)
				return 1;
			}
		}
	}
return 0;
}

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Return true if a special CObject (player or control center) is in this CSegment.
// -- int32_t specialObject_in_seg (int32_t nSegment)
// -- {
// -- 	int32_t	nObject;
// --
// -- 	nObject = SEGMENT (nSegment)->m_objects;
// --
// -- 	while (nObject != -1) {
// -- 		if ((OBJECT (nObject)->nType == OBJ_PLAYER) || (OBJECT (nObject)->nType == OBJ_REACTOR)) {
// -- 			return 1;
// -- 		} else
// -- 			nObject = OBJECT (nObject)->next;
// -- 	}
// --
// -- 	return 0;
// -- }

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Randomly select a CSegment attached to *segP, reachable by flying.
// -- int32_t get_random_child (int32_t nSegment)
// -- {
// -- 	int32_t	nSide;
// -- 	CSegment	*segP = SEGMENT (nSegment);
// --
// -- 	nSide = (rand () * 6) >> 15;
// --
// -- 	while (!(segP->IsPassable (nSide) & WID_PASSABLE_FLAG))
// -- 		nSide = (rand () * 6) >> 15;
// --
// -- 	nSegment = segP->m_children [nSide];
// --
// -- 	return nSegment;
// -- }

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an CObject of size size at pos *pos intersects a (player or robot or control center) in CSegment *segP.
int32_t CheckObjectObjectIntersection (CFixVector *pos, fix size, CSegment *segP)
{
//	If this would intersect with another CObject (only check those in this CSegment), then try to move.
int16_t nObject = segP->m_objects;
CObject *objP;
while (nObject != -1) {
	objP = OBJECT (nObject);
	if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_REACTOR)) {
		if (CFixVector::Dist (*pos, objP->info.position.vPos) < size + objP->info.xSize)
			return 1;
		}
	nObject = objP->info.nNextInSeg;
	}
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI CObject if it is fairly aware of the player.
//	awarenessLevel is in 0..100.  Larger numbers indicate greater awareness (eg, 99 if firing at player).
//	In a given frame, might not get called for an CObject, or might be called more than once.
//	The fact that this routine is not called for a given CObject does not mean that CObject is not interested in the player.
//	OBJECTS are moved by physics, so they can move even if not interested in a player.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this player IS NOT allowed to move this robot.
//		1	this player IS allowed to move this robot.

int32_t AILocalPlayerControlsRobot (CObject *objP, int32_t awarenessLevel)
{
if (!IsMultiGame)
	return 1;
if (!awarenessLevel)
	return 0;
return MultiCanControlRobot (objP->Index (), awarenessLevel);
}

// --------------------------------------------------------------------------------------------------------------------

#define	BOSS_TO_PLAYER_GATE_DISTANCE	 (I2X (200))

void AIMultiSendRobotPos (int16_t nObject, int32_t force)
{
if (IsMultiGame)
	MultiSendRobotPosition (nObject, force != -1);
}

// ----------------------------------------------------------------------------------

#if DBG
int32_t Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char Ai_error_message [128] = "";

void force_dump_aiObjects_all (char *msg)
{
	int32_t tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf (Ai_error_message, "%s\n", msg);
	//dump_aiObjects_all ();
	Ai_error_message [0] = 0;

	Ai_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------

void turn_off_ai_dump (void)
{
	if (Ai_dump_file != NULL)
		fclose (Ai_dump_file);

	Ai_dump_file = NULL;
}

#endif

//	---------------------------------------------------------------
// eof

