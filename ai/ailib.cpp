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
#include "multi.h"
#include "network.h"
#include "loadgame.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "gameseg.h"
#include "escort.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
//#define _DEBUG
#ifdef _DEBUG
#include "string.h"
#include <time.h>
#endif

int	nRobotSoundVolume = DEFAULT_ROBOT_SOUND_VOLUME;

// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from tObject, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses gameData.ai.vBelievedPlayerPos as tPlayer's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
int ObjectCanSeePlayer (tObject *objP, vmsVector *pos, fix fieldOfView, vmsVector *vVecToPlayer)
{
	fix			dot;
	tVFIQuery	fq;

	//	Assume that robot's gun tip is in same tSegment as robot's center.
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
fq.p0	= pos;
if ((pos->p.x != objP->position.vPos.p.x) || 
	 (pos->p.y != objP->position.vPos.p.y) || 
	 (pos->p.z != objP->position.vPos.p.z)) {
	short nSegment = FindSegByPoint (pos, objP->nSegment, 1, 0);
	if (nSegment == -1) {
		fq.startSeg = objP->nSegment;
		*pos = objP->position.vPos;
#if TRACE
		con_printf (1, "Object %i, gun is outside mine, moving towards center.\n", OBJ_IDX (objP));
#endif
		MoveTowardsSegmentCenter (objP);
		} 
	else {
		if (nSegment != objP->nSegment)
			objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
		fq.startSeg = nSegment;
		}
	}
else
	fq.startSeg	= objP->nSegment;
fq.p1					= &gameData.ai.vBelievedPlayerPos;
fq.radP0				= 
fq.radP1				= F1_0 / 4;
fq.thisObjNum		= OBJ_IDX (objP);
fq.ignoreObjList	= NULL;
fq.flags				= FQ_TRANSWALL; 
gameData.ai.nHitType = FindVectorIntersection (&fq, &gameData.ai.hitData);
gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitData.hit.nSegment;
if (gameData.ai.nHitType != HIT_NONE)
	return 0;
dot = VmVecDot (vVecToPlayer, &objP->position.mOrient.fVec);
return (dot > fieldOfView - (gameData.ai.nOverallAgitation << 9)) ? 2 : 1;
}

// ------------------------------------------------------------------------------------------------------------------

int AICanFireAtPlayer (tObject *objP, vmsVector *vGun, vmsVector *vPlayer)
{
	tVFIQuery	fq;
	fix			nSize, h;
	short			nModel, ignoreObjs [2] = {OBJ_IDX (gameData.objs.console), -1};

//	Assume that robot's gun tip is in same tSegment as robot's center.
if (!(vGun->p.x || vGun->p.y || vGun->p.z))
	return 0;
if (!extraGameInfo [IsMultiGame].bRobotsHitRobots)
	return 1;
objP->cType.aiInfo.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
if ((vGun->p.x == objP->position.vPos.p.x) && 
	 (vGun->p.y == objP->position.vPos.p.y) && 
	 (vGun->p.z == objP->position.vPos.p.z))
	fq.startSeg	= objP->nSegment;
else {
	short nSegment = FindSegByPoint (vGun, objP->nSegment, 1, 0);
	if (nSegment == -1)
		return -1;
	if (nSegment != objP->nSegment)
		objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
	fq.startSeg = nSegment;
	}
h = VmVecDist (vGun, &objP->position.vPos);
h = VmVecDist (vGun, vPlayer);
nModel = objP->rType.polyObjInfo.nModel;
nSize = objP->size;
objP->rType.polyObjInfo.nModel = -1;	//make sure sphere/hitbox and not hitbox/hitbox collisions get tested
objP->size = F1_0 * 2;						//chose some meaningful small size to simulate a weapon
fq.p0					= vGun;
fq.p1					= vPlayer;
fq.radP0				= 
fq.radP1				= F1_0;
fq.thisObjNum		= OBJ_IDX (objP);
fq.ignoreObjList	= ignoreObjs;
fq.flags				= FQ_CHECK_OBJS | FQ_ANY_OBJECT | FQ_IGNORE_POWERUPS;		//what about trans walls???
gameData.ai.nHitType = FindVectorIntersection (&fq, &gameData.ai.hitData);
#ifdef _DEBUG
if (gameData.ai.nHitType == 0)
	FindVectorIntersection (&fq, &gameData.ai.hitData);
#endif
gameData.ai.vHitPos = gameData.ai.hitData.hit.vPoint;
gameData.ai.nHitSeg = gameData.ai.hitData.hit.nSegment;
objP->rType.polyObjInfo.nModel = nModel;
objP->size = nSize;
return (gameData.ai.nHitType != HIT_OBJECT);
}

// --------------------------------------------------------------------------------------------------------------------

inline void LimitPlayerVisibility (fix xMaxVisibleDist, tAILocal *ailP)
{
#if 1
if ((xMaxVisibleDist > 0) && (gameData.ai.xDistToPlayer > xMaxVisibleDist) && (ailP->playerAwarenessType < PA_RETURN_FIRE))
	gameData.ai.nPlayerVisibility = 0;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely ObjectCanSeePlayer would benefit from the
//	information of a normalized gameData.ai.vVecToPlayer.
//	Return tPlayer visibility:
//		0		not visible
//		1		visible, but robot not looking at tPlayer (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		tPlayer is cloaked
//	If the tPlayer is cloaked, set gameData.ai.vVecToPlayer based on time tPlayer cloaked and last uncloaked position.
//	Updates ailP->nPrevVisibility if tPlayer is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to gameData.ai.nPlayerVisibility
void ComputeVisAndVec (tObject *objP, vmsVector *pos, tAILocal *ailP, tRobotInfo *botInfoP, int *flag, 
							  fix xMaxVisibleDist)
{
if (*flag) {
	LimitPlayerVisibility (xMaxVisibleDist, ailP);
	}
else {
	if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
		fix			deltaTime, dist;
		int			cloak_index = (OBJ_IDX (objP)) % MAX_AI_CLOAK_INFO;

		deltaTime = gameData.time.xGame - gameData.ai.cloakInfo [cloak_index].lastTime;
		if (deltaTime > F1_0*2) {
			vmsVector	vRand;

			gameData.ai.cloakInfo [cloak_index].lastTime = gameData.time.xGame;
			MakeRandomVector (&vRand);
			VmVecScaleInc (&gameData.ai.cloakInfo [cloak_index].vLastPos, &vRand, 8*deltaTime);
			}
		dist = VmVecNormalizedDirQuick (&gameData.ai.vVecToPlayer, &gameData.ai.cloakInfo [cloak_index].vLastPos, pos);
		gameData.ai.nPlayerVisibility = ObjectCanSeePlayer (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.ai.vVecToPlayer);
		LimitPlayerVisibility (xMaxVisibleDist, ailP);
#ifdef _DEBUG
		if (gameData.ai.nPlayerVisibility == 2)
			gameData.ai.nPlayerVisibility = gameData.ai.nPlayerVisibility;
#endif
		if ((ailP->nextMiscSoundTime < gameData.time.xGame) && ((ailP->nextPrimaryFire < F1_0) || (ailP->nextSecondaryFire < F1_0)) && (dist < F1_0*20)) {
			ailP->nextMiscSoundTime = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 1;
			DigiLinkSoundToPos (botInfoP->seeSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
			}
		}
	else {
		//	Compute expensive stuff -- gameData.ai.vVecToPlayer and gameData.ai.nPlayerVisibility
		VmVecNormalizedDirQuick (&gameData.ai.vVecToPlayer, &gameData.ai.vBelievedPlayerPos, pos);
		if (!(gameData.ai.vVecToPlayer.p.x || gameData.ai.vVecToPlayer.p.y || gameData.ai.vVecToPlayer.p.z)) {
			gameData.ai.vVecToPlayer.p.x = F1_0;
			}
		gameData.ai.nPlayerVisibility = ObjectCanSeePlayer (objP, pos, botInfoP->fieldOfView [gameStates.app.nDifficultyLevel], &gameData.ai.vVecToPlayer);
		LimitPlayerVisibility (xMaxVisibleDist, ailP);
#ifdef _DEBUG
		if (gameData.ai.nPlayerVisibility == 2)
			gameData.ai.nPlayerVisibility = gameData.ai.nPlayerVisibility;
#endif
		//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
		//	see you without killing frame rate.
		{
			tAIStatic	*aiP = &objP->cType.aiInfo;
		if ((gameData.ai.nPlayerVisibility == 2) && (ailP->nPrevVisibility != 2))
			if ((aiP->GOAL_STATE == AIS_REST) || (aiP->CURRENT_STATE == AIS_REST)) {
				aiP->GOAL_STATE = AIS_FIRE;
				aiP->CURRENT_STATE = AIS_FIRE;
				}
			}

		if ((ailP->nPrevVisibility != gameData.ai.nPlayerVisibility) && (gameData.ai.nPlayerVisibility == 2)) {
			if (ailP->nPrevVisibility == 0) {
				if (ailP->timePlayerSeen + F1_0/2 < gameData.time.xGame) {
					// -- if (gameStates.app.bPlayerExploded)
					// -- 	DigiLinkSoundToPos (botInfoP->tauntSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
					// -- else
						DigiLinkSoundToPos (botInfoP->seeSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
					ailP->timePlayerSoundAttacked = gameData.time.xGame;
					ailP->nextMiscSoundTime = gameData.time.xGame + F1_0 + d_rand ()*4;
					}
				} 
			else if (ailP->timePlayerSoundAttacked + F1_0/4 < gameData.time.xGame) {
				// -- if (gameStates.app.bPlayerExploded)
				// -- 	DigiLinkSoundToPos (botInfoP->tauntSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
				// -- else
					DigiLinkSoundToPos (botInfoP->attackSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
				ailP->timePlayerSoundAttacked = gameData.time.xGame;
				}
			} 

		if ((gameData.ai.nPlayerVisibility == 2) && (ailP->nextMiscSoundTime < gameData.time.xGame)) {
			ailP->nextMiscSoundTime = gameData.time.xGame + (d_rand () + F1_0) * (7 - gameStates.app.nDifficultyLevel) / 2;
			// -- if (gameStates.app.bPlayerExploded)
			// -- 	DigiLinkSoundToPos (botInfoP->tauntSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
			// -- else
				DigiLinkSoundToPos (botInfoP->attackSound, objP->nSegment, 0, pos, 0 , nRobotSoundVolume);
			}
		ailP->nPrevVisibility = gameData.ai.nPlayerVisibility;
		}

	*flag = 1;

	//	@mk, 09/21/95: If tPlayer view is not obstructed and awareness is at least as high as a nearby collision,
	//	act is if robot is looking at player.
	if (ailP->playerAwarenessType >= PA_NEARBY_ROBOT_FIRED)
		if (gameData.ai.nPlayerVisibility == 1)
			gameData.ai.nPlayerVisibility = 2;
		
	if (gameData.ai.nPlayerVisibility) {
		ailP->timePlayerSeen = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable nType robot.
//	Brains, avoid robots, companions can open doors.
//	objP == NULL means treat as buddy.
int AIDoorIsOpenable (tObject *objP, tSegment *segP, short nSide)
{
	short nWall;
	tWall	*wallP;

if (!IS_CHILD (segP->children [nSide]))
	return 0;		//trap -2 (exit tSide)
nWall = WallNumP (segP, nSide);
if (!IS_WALL (nWall))		//if there's no door at alld:\temp\dm_test.
	return 1;				//d:\temp\dm_testthen say it can't be opened
	//	The mighty console tObject can open all doors (for purposes of determining paths).
if (objP == gameData.objs.console) {
	if (gameData.walls.walls [nWall].nType == WALL_DOOR)
		return 1;
	}
wallP = gameData.walls.walls + nWall;
if ((objP == NULL) || (ROBOTINFO (objP->id).companion == 1)) {
	int	ailp_mode;

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

	//	If Buddy is returning to tPlayer, don't let him think he can get through triggered doors.
	//	It's only valid to think that if the tPlayer is going to get him through.  But if he's
	//	going to the tPlayer, the tPlayer is probably on the opposite tSide.
	if (objP)
		ailp_mode = gameData.ai.localInfo [OBJ_IDX (objP)].mode;
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
		int	nClip = wallP->nClip;

		if (nClip == -1)
			return 1;
		else if (gameData.walls.pAnims [nClip].flags & WCF_HIDDEN) {
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
			int	nClip = wallP->nClip;

			if (nClip == -1)
				return 1;
			//	Buddy allowed to go through secret doors to get to player.
			else if ((ailp_mode != AIM_GOTO_PLAYER) && (gameData.walls.pAnims [nClip].flags & WCF_HIDDEN)) {
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
else if ((objP->id == ROBOT_BRAIN) || (objP->cType.aiInfo.behavior == AIB_RUN_FROM) || (objP->cType.aiInfo.behavior == AIB_SNIPE)) {
	if (IS_WALL (nWall)) {
		if ((wallP->nType == WALL_DOOR) && (wallP->keys == KEY_NONE) && !(wallP->flags & WALL_DOOR_LOCKED))
			return 1;
		else if (wallP->keys != KEY_NONE) {	//	Allow bots to open doors to which tPlayer has keys.
			if (wallP->keys & LOCALPLAYER.flags)
				return 1;
			}
		}
	}
return 0;
}

//	-----------------------------------------------------------------------------------------------------------
//	Return tSide of openable door in tSegment, if any.  If none, return -1.
int OpenableDoorsInSegment (short nSegment)
{
	ushort	i;

	if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment))
		return -1;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	nWall = WallNumI (nSegment, i);
		if (IS_WALL (nWall)) {
			tWall	*wallP = gameData.walls.walls + nWall;
			if ((wallP->nType == WALL_DOOR) && 
				 (wallP->keys == KEY_NONE) && 
				 (wallP->state == WALL_DOOR_CLOSED) && 
				 !(wallP->flags & WALL_DOOR_LOCKED) && 
				 !(gameData.walls.pAnims [wallP->nClip].flags & WCF_HIDDEN))
				return i;
		}
	}

	return -1;

}

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Return true if a special tObject (tPlayer or control center) is in this tSegment.
// -- int specialObject_in_seg (int nSegment)
// -- {
// -- 	int	nObject;
// -- 
// -- 	nObject = gameData.segs.segments [nSegment].objects;
// -- 
// -- 	while (nObject != -1) {
// -- 		if ((gameData.objs.objects [nObject].nType == OBJ_PLAYER) || (gameData.objs.objects [nObject].nType == OBJ_REACTOR)) {
// -- 			return 1;
// -- 		} else
// -- 			nObject = gameData.objs.objects [nObject].next;
// -- 	}
// -- 
// -- 	return 0;
// -- }

// -- // --------------------------------------------------------------------------------------------------------------------
// -- //	Randomly select a tSegment attached to *segP, reachable by flying.
// -- int get_random_child (int nSegment)
// -- {
// -- 	int	nSide;
// -- 	tSegment	*segP = &gameData.segs.segments [nSegment];
// -- 
// -- 	nSide = (rand () * 6) >> 15;
// -- 
// -- 	while (!(WALL_IS_DOORWAY (segP, nSide) & WID_FLY_FLAG))
// -- 		nSide = (rand () * 6) >> 15;
// -- 
// -- 	nSegment = segP->children [nSide];
// -- 
// -- 	return nSegment;
// -- }

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an tObject of size size at pos *pos intersects a (tPlayer or robot or control center) in tSegment *segP.
int CheckObjectObjectIntersection (vmsVector *pos, fix size, tSegment *segP)
{
	int		curobjnum;

	//	If this would intersect with another tObject (only check those in this tSegment), then try to move.
	curobjnum = segP->objects;
	while (curobjnum != -1) {
		tObject *curObjP = &gameData.objs.objects [curobjnum];
		if ((curObjP->nType == OBJ_PLAYER) || (curObjP->nType == OBJ_ROBOT) || (curObjP->nType == OBJ_REACTOR)) {
			if (VmVecDistQuick (pos, &curObjP->position.vPos) < size + curObjP->size)
				return 1;
		}
		curobjnum = curObjP->next;
	}

	return 0;

}

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI tObject if it is fairly aware of the player.
//	awarenessLevel is in 0..100.  Larger numbers indicate greater awareness (eg, 99 if firing at tPlayer).
//	In a given frame, might not get called for an tObject, or might be called more than once.
//	The fact that this routine is not called for a given tObject does not mean that tObject is not interested in the player.
//	gameData.objs.objects are moved by physics, so they can move even if not interested in a player.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this tPlayer IS NOT allowed to move this robot.
//		1	this tPlayer IS allowed to move this robot.

int AIMultiplayerAwareness (tObject *objP, int awarenessLevel)
{
if (!IsMultiGame)
	return 1;
if (!awarenessLevel)
	return 0;
return MultiCanRemoveRobot (OBJ_IDX (objP), awarenessLevel);
}

// --------------------------------------------------------------------------------------------------------------------

#define	BOSS_TO_PLAYER_GATE_DISTANCE	 (F1_0*200)

void AIMultiSendRobotPos (short nObject, int force)
{
if (IsMultiGame)
	MultiSendRobotPosition (nObject, force != -1);
}

// ----------------------------------------------------------------------------------

#ifdef _DEBUG
int Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char Ai_error_message [128] = "";

void force_dump_aiObjects_all (char *msg)
{
	int tsave;

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

