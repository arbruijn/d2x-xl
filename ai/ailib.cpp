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
//	Note: Uses gameData.aiData.target.vBelievedPos as player's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int32_t AICanSeeTarget (CObject *pObj, CFixVector *vPos, fix fieldOfView, CFixVector *vVecToTarget)
{
ENTER (1, 0);
	CHitQuery	hitQuery (FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_VISIBILITY | (pObj->AttacksRobots () ? FQ_ANY_OBJECT : FQ_CHECK_PLAYER),
								 vPos, &gameData.aiData.target.vBelievedPos, -1, pObj->Index (), I2X (1) / 4, I2X (1) / 4, ++gameData.physicsData.bIgnoreObjFlag);

//	Assume that robot's gun tip is in same CSegment as robot's center.
pObj->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
if ((*vPos) != pObj->info.position.vPos) {
	int16_t nSegment = FindSegByPos (*vPos, pObj->info.nSegment, 1, 0);
	if (nSegment == -1) {
		hitQuery.nSegment = pObj->info.nSegment;
		*vPos = pObj->info.position.vPos;
#if TRACE
		console.printf (1, "Object %i, gun is outside mine, moving towards center.\n", pObj->Index ());
#endif
		MoveTowardsSegmentCenter (pObj);
		}
	else {
		if (nSegment != pObj->info.nSegment)
			pObj->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
		hitQuery.nSegment = nSegment;
		}
	}
else
	hitQuery.nSegment	= pObj->info.nSegment;

gameData.aiData.nHitType = FindHitpoint (hitQuery, gameData.aiData.hitResult, 0);
gameData.aiData.vHitPos = gameData.aiData.hitResult.vPoint;
gameData.aiData.nHitSeg = gameData.aiData.hitResult.nSegment;
if ((gameData.aiData.nHitType != HIT_OBJECT) || (gameData.aiData.hitResult.nObject != TARGETOBJ->Index ()))
	RETVAL (0)
fix dot = CFixVector::Dot (*vVecToTarget, pObj->info.position.mOrient.m.dir.f);
if (dot > fieldOfView - (gameData.aiData.nOverallAgitation << 9))
	RETVAL (2)
if (gameOpts->gameplay.nAIAggressivity) {	// player visible at raised AI aggressivity when having headlight on
	 if ((TARGETOBJ->info.nType == OBJ_PLAYER) && 
		  (TARGETOBJ->info.nId == N_LOCALPLAYER) && 
		  HeadlightIsOn (-1))
		RETVAL (2)
	if (!TARGETOBJ->Cloaked () &&
		 (CFixVector::Dist (*vPos, OBJPOS (TARGETOBJ)->vPos) < 
		 ((3 + gameOpts->gameplay.nAIAggressivity) * (pObj->info.xSize + TARGETOBJ->info.xSize)) / 2))
		RETVAL (2)
	}
RETVAL (1)
}

// ------------------------------------------------------------------------------------------------------------------

int32_t AICanFireAtTarget (CObject *pObj, CFixVector *vGun, CFixVector *vTarget)
{
ENTER (1, 0);
	fix			nSize;
	int16_t		nModel;

//	Assume that robot's gun tip is in same CSegment as robot's center.
if (vGun->IsZero ())
	RETVAL (0)
if (!extraGameInfo [IsMultiGame].bRobotsHitRobots)
	RETVAL (1)
pObj->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;

CHitQuery hitQuery (FQ_CHECK_OBJS | FQ_ANY_OBJECT | FQ_IGNORE_POWERUPS | FQ_TRANSPOINT | FQ_VISIBILITY,
						  vGun, vTarget, -1, pObj->Index (), I2X (1), I2X (1), ++gameData.physicsData.bIgnoreObjFlag);

if (*vGun == pObj->Position ())
	hitQuery.nSegment	= pObj->info.nSegment;
else {
	int16_t nSegment = FindSegByPos (*vGun, pObj->info.nSegment, 1, 0);
	if (nSegment == -1)
		return -1;
	if (nSegment != pObj->info.nSegment)
		pObj->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
	hitQuery.nSegment = nSegment;
	}
nModel = pObj->rType.polyObjInfo.nModel;
nSize = pObj->info.xSize;
pObj->rType.polyObjInfo.nModel = -1;	//make sure sphere/hitbox and not hitbox/hitbox collisions get tested
pObj->SetSize (I2X (2));					//chose some meaningful small size to simulate a weapon

gameData.aiData.nHitType = FindHitpoint (hitQuery, gameData.aiData.hitResult, 0);
#if DBG
if (gameData.aiData.nHitType == 0)
	FindHitpoint (hitQuery, gameData.aiData.hitResult, 0);
#endif
pObj->rType.polyObjInfo.nModel = nModel;
gameData.aiData.vHitPos = gameData.aiData.hitResult.vPoint;
gameData.aiData.nHitSeg = gameData.aiData.hitResult.nSegment;
pObj->rType.polyObjInfo.nModel = nModel;
pObj->SetSize (nSize);
RETVAL ((gameData.aiData.nHitType == HIT_NONE) || 
		  ((gameData.aiData.nHitType == HIT_OBJECT) && pObj->Target () && (pObj->Target ()->Index () == gameData.aiData.hitResult.nObject)))
}

// --------------------------------------------------------------------------------------------------------------------

inline void LimitTargetVisibility (fix xMaxVisibleDist, tAILocalInfo *pLocalInfo)
{
#if 1
if ((xMaxVisibleDist > 0) && (gameData.aiData.target.xDist > xMaxVisibleDist) && (pLocalInfo->targetAwarenessType < PA_RETURN_FIRE))
	gameData.aiData.nTargetVisibility = 0;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely AICanSeeTarget would benefit from the
//	information of a normalized gameData.aiData.target.vDir.
//	Return player visibility:
//		0		not visible
//		1		visible, but robot not looking at player (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		player is cloaked
//	If the player is cloaked, set gameData.aiData.target.vDir based on time player cloaked and last uncloaked position.
//	Updates pLocalInfo->nPrevVisibility if player is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to gameData.aiData.nTargetVisibility
void ComputeVisAndVec (CObject *pObj, CFixVector *pos, tAILocalInfo *pLocalInfo, tRobotInfo *pRobotInfo, int32_t *flag, fix xMaxVisibleDist)
{
ENTER (1, 0);
if (*flag)
	LimitTargetVisibility (xMaxVisibleDist, pLocalInfo);
else if (!TARGETOBJ->Appearing ()) {
	if (TARGETOBJ->Cloaked ()) {
		fix			deltaTime, dist;
		int32_t			nCloakIndex = (pObj->Index ()) % MAX_AI_CLOAK_INFO;

		deltaTime = gameData.timeData.xGame - gameData.aiData.cloakInfo [nCloakIndex].lastTime;
		if (deltaTime > I2X (2)) {
			CFixVector	vRand;

			gameData.aiData.cloakInfo [nCloakIndex].lastTime = gameData.timeData.xGame;
			vRand = CFixVector::Random ();
			gameData.aiData.cloakInfo [nCloakIndex].vLastPos += vRand * (8 * deltaTime);
			}
		dist = CFixVector::NormalizedDir (gameData.aiData.target.vDir, gameData.aiData.cloakInfo [nCloakIndex].vLastPos, *pos);
		gameData.aiData.nTargetVisibility = AICanSeeTarget (pObj, pos, pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.aiData.target.vDir);
		LimitTargetVisibility (xMaxVisibleDist, pLocalInfo);
#if DBG
		if (gameData.aiData.nTargetVisibility == 2)
			gameData.aiData.nTargetVisibility = gameData.aiData.nTargetVisibility;
#endif
		if ((pLocalInfo->nextMiscSoundTime < gameData.timeData.xGame) && ((pLocalInfo->pNextrimaryFire < I2X (1)) || (pLocalInfo->nextSecondaryFire < I2X (1))) && (dist < I2X (20))) {
			pLocalInfo->nextMiscSoundTime = gameData.timeData.xGame + (RandShort () + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 1;
			audio.CreateSegmentSound (pRobotInfo->seeSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
			}
		}
	else {
		//	Compute expensive stuff -- gameData.aiData.target.vDir and gameData.aiData.nTargetVisibility
		CFixVector::NormalizedDir (gameData.aiData.target.vDir, gameData.aiData.target.vBelievedPos, *pos);
		if (gameData.aiData.target.vDir.IsZero ()) {
			gameData.aiData.target.vDir.v.coord.x = I2X (1);
			}
		gameData.aiData.nTargetVisibility = AICanSeeTarget (pObj, pos, pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.aiData.target.vDir);
		LimitTargetVisibility (xMaxVisibleDist, pLocalInfo);
#if DBG
		if (gameData.aiData.nTargetVisibility == 2)
			gameData.aiData.nTargetVisibility = gameData.aiData.nTargetVisibility;
#endif
		//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
		//	see you without killing frame rate.
		tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
		if ((gameData.aiData.nTargetVisibility == 2) && (pLocalInfo->nPrevVisibility != 2))
			if ((pStaticInfo->GOAL_STATE == AIS_REST) || (pStaticInfo->CURRENT_STATE == AIS_REST)) {
				pStaticInfo->GOAL_STATE = AIS_FIRE;
				pStaticInfo->CURRENT_STATE = AIS_FIRE;
				}

		if ((pLocalInfo->nPrevVisibility != gameData.aiData.nTargetVisibility) && (gameData.aiData.nTargetVisibility == 2)) {
			if (pLocalInfo->nPrevVisibility == 0) {
				if (pLocalInfo->timeTargetSeen + I2X (1)/2 < gameData.timeData.xGame) {
					// -- if (LOCALPLAYER.m_bExploded)
					// -- 	audio.CreateSegmentSound (pRobotInfo->tauntSound, pObj->info.nSegment, 0, pos, 0 , nRobotSoundVolume);
					// -- else
						audio.CreateSegmentSound (pRobotInfo->seeSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
					pLocalInfo->timeTargetSoundAttacked = gameData.timeData.xGame;
					pLocalInfo->nextMiscSoundTime = gameData.timeData.xGame + I2X (1) + RandShort ()*4;
					}
				}
			else if (pLocalInfo->timeTargetSoundAttacked + I2X (1) / 4 < gameData.timeData.xGame) {
				// -- if (LOCALPLAYER.m_bExploded)
				// -- 	audio.CreateSegmentSound (pRobotInfo->tauntSound, pObj->info.nSegment, 0, pos, 0 , nRobotSoundVolume);
				// -- else
					audio.CreateSegmentSound (pRobotInfo->attackSound, pObj->info.nSegment, 0, *pos, 0, nRobotSoundVolume);
				pLocalInfo->timeTargetSoundAttacked = gameData.timeData.xGame;
				}
			}

		if ((gameData.aiData.nTargetVisibility == 2) && (pLocalInfo->nextMiscSoundTime < gameData.timeData.xGame)) {
			pLocalInfo->nextMiscSoundTime = gameData.timeData.xGame + (RandShort () + I2X (1)) * (7 - gameStates.app.nDifficultyLevel) / 2;
			// -- if (LOCALPLAYER.m_bExploded)
			// -- 	audio.CreateSegmentSound (pRobotInfo->tauntSound, pObj->info.nSegment, 0, pos, 0 , nRobotSoundVolume);
			// -- else
				audio.CreateSegmentSound (pRobotInfo->attackSound, pObj->info.nSegment, 0, *pos, 0 , nRobotSoundVolume);
			}
		pLocalInfo->nPrevVisibility = gameData.aiData.nTargetVisibility;
		}

	*flag = 1;

	//	@mk, 09/21/95: If player view is not obstructed and awareness is at least as high as a nearby collision,
	//	act is if robot is looking at player.
	if (pLocalInfo->targetAwarenessType >= PA_NEARBY_ROBOT_FIRED)
		if (gameData.aiData.nTargetVisibility == 1)
			gameData.aiData.nTargetVisibility = 2;
	if (gameData.aiData.nTargetVisibility)
		pLocalInfo->timeTargetSeen = gameData.timeData.xGame;
	}
#if 0 //DBG
if (pObj->Index () == nDbgObj) {
	HUDMessage (0, "vis: %d", gameData.aiData.nTargetVisibility);
	}
#endif
RETURN
}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable nType robot.
//	Brains, avoid robots, companions can open doors.
//	pObj == NULL means treat as buddy.
int32_t AIDoorIsOpenable (CObject *pObj, CSegment *pSeg, int16_t nSide)
{
ENTER (1, 0);
	CWall	*pWall;

if (!IS_CHILD (pSeg->m_children [nSide]))
	RETVAL (0)		//trap -2 (exit CSide)
if (!(pWall = pSeg->Wall (nSide)))
	RETVAL (1)				//d:\temp\dm_testthen say it can't be opened
	//	The mighty console CObject can open all doors (for purposes of determining paths).
if (pObj == gameData.objData.pConsole) {
	if (pWall->nType == WALL_DOOR)
		RETVAL (1)
	}
if ((pObj == NULL) || pObj->IsGuideBot ()) {
	int32_t	ailp_mode;

	if (pWall->flags & WALL_BUDDY_PROOF) {
		if ((pWall->nType == WALL_DOOR) && (pWall->state == WALL_DOOR_CLOSED))
			RETVAL (0)
		else if (pWall->nType == WALL_CLOSED)
			RETVAL (0)
		else if ((pWall->nType == WALL_ILLUSION) && !(pWall->flags & WALL_ILLUSION_OFF))
			RETVAL (0)
		}

	if (pWall->keys != KEY_NONE) {
		if (pWall->keys == KEY_BLUE)
			return (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY);
		else if (pWall->keys == KEY_GOLD)
			return (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY);
		else if (pWall->keys == KEY_RED)
			return (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY);
		}

	if (pWall->nType == WALL_CLOSED)
		RETVAL (0)
	if (pWall->nType != WALL_DOOR) /*&& (pWall->nType != WALL_CLOSED))*/
		RETVAL (1)

	//	If Buddy is returning to player, don't let him think he can get through triggered doors.
	//	It's only valid to think that if the player is going to get him through.  But if he's
	//	going to the player, the player is probably on the opposite CSide.
	if (pObj)
		ailp_mode = gameData.aiData.localInfo [pObj->Index ()].mode;
	else if (gameData.escortData.nObjNum >= 0)
		ailp_mode = gameData.aiData.localInfo [gameData.escortData.nObjNum].mode;
	else
		ailp_mode = 0;

	// -- if (Buddy_got_stuck) {
	if (ailp_mode == AIM_GOTO_PLAYER) {
		if ((pWall->nType == WALL_BLASTABLE) && (pWall->state != WALL_BLASTED))
			RETVAL (0)
		if (pWall->nType == WALL_CLOSED)
			RETVAL (0)
		if (pWall->nType == WALL_DOOR) {
			if ((pWall->flags & WALL_DOOR_LOCKED) && (pWall->state == WALL_DOOR_CLOSED))
				RETVAL (0)
			}
		}
		// -- }

	if ((ailp_mode != AIM_GOTO_PLAYER) && (pWall->controllingTrigger != -1)) {
		int32_t	nClip = pWall->nClip;

		if (nClip == -1)
			RETVAL (1)
		else if (gameData.wallData.pAnim [nClip].flags & WCF_HIDDEN) {
			if (pWall->state == WALL_DOOR_CLOSED)
				RETVAL (0)
			else
				RETVAL (1)
			}
		else
			RETVAL (1)
		}

	if (pWall->nType == WALL_DOOR)  {
		if (pWall->nType == WALL_BLASTABLE)
			RETVAL (1)
		else {
			int32_t	nClip = pWall->nClip;

			if (nClip == -1)
				RETVAL (1)
			//	Buddy allowed to go through secret doors to get to player.
			else if (0 && (ailp_mode != AIM_GOTO_PLAYER) && (gameData.wallData.pAnim [nClip].flags & WCF_HIDDEN)) {
				if (pWall->state == WALL_DOOR_CLOSED)
					RETVAL (0)
				else
					RETVAL (1)
				}
			else
				RETVAL (1)
			}
		}
	}
else if ((pObj->info.nId == ROBOT_BRAIN) || (pObj->cType.aiInfo.behavior == AIB_RUN_FROM) || (pObj->cType.aiInfo.behavior == AIB_SNIPE)) {
	if (pWall) {
		if ((pWall->nType == WALL_DOOR) && (pWall->keys == KEY_NONE) && !(pWall->flags & WALL_DOOR_LOCKED))
			RETVAL (1)
		else if (pWall->keys != KEY_NONE) {	//	Allow bots to open doors to which player has keys.
			if (pWall->keys & LOCALPLAYER.flags)
				RETVAL (1)
			}
		}
	}
RETVAL (0)
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
// -- //	Randomly select a CSegment attached to *pSeg, reachable by flying.
// -- int32_t get_random_child (int32_t nSegment)
// -- {
// -- 	int32_t	nSide;
// -- 	CSegment	*pSeg = SEGMENT (nSegment);
// --
// -- 	nSide = (rand () * 6) >> 15;
// --
// -- 	while (!(pSeg->IsPassable (nSide) & WID_PASSABLE_FLAG))
// -- 		nSide = (rand () * 6) >> 15;
// --
// -- 	nSegment = pSeg->m_children [nSide];
// --
// -- 	return nSegment;
// -- }

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an CObject of size size at pos *pos intersects a (player or robot or control center) in CSegment *pSeg.
int32_t CheckObjectObjectIntersection (CFixVector *pos, fix size, CSegment *pSeg)
{
//	If this would intersect with another CObject (only check those in this CSegment), then try to move.
ENTER (1, 0);
int16_t nObject = pSeg->m_objects;
while (nObject != -1) {
	CObject *pObj = OBJECT (nObject);
	if (!pObj)
		RETVAL (0)
	if ((pObj->info.nType == OBJ_PLAYER) || (pObj->info.nType == OBJ_ROBOT) || (pObj->info.nType == OBJ_REACTOR)) {
		if (CFixVector::Dist (*pos, pObj->info.position.vPos) < size + pObj->info.xSize)
			RETVAL (1)
		}
	nObject = pObj->info.nNextInSeg;
	}
RETVAL (0)
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

int32_t AILocalPlayerControlsRobot (CObject *pObj, int32_t awarenessLevel)
{
ENTER (1, 0);
if (!IsMultiGame)
	RETVAL (1)
if (!awarenessLevel)
	RETVAL (0)
RETVAL (MultiCanControlRobot (pObj->Index (), awarenessLevel))
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

