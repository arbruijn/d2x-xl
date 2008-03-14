/* $Id: ai.c,v 1.7 2003/10/04 02:58:23 btb Exp $ */
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
#include "game.h"
#include "mono.h"
#include "3d.h"

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
#include "gauges.h"
#include "text.h"
#include "fuelcen.h"
#include "controls.h"
#include "input.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "string.h"
//#define _DEBUG
#ifdef _DEBUG
#include <time.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Make a robot near the tPlayer snipe.
#define	MNRS_SEG_MAX	70

void MakeNearbyRobotSnipe (void)
{
	tObject		*objP;
	tRobotInfo	*botInfoP;
	short			bfsList [MNRS_SEG_MAX];
	int			nObject, nBfsLength, i;

CreateBfsList (OBJSEG (gameData.objs.console), bfsList, &nBfsLength, MNRS_SEG_MAX);
for (i = 0; i < nBfsLength; i++) {
	nObject = gameData.segs.segments [bfsList [i]].objects;
	Assert (nObject >= 0);
	while (nObject != -1) {
		objP = gameData.objs.objects + nObject;
		botInfoP = &ROBOTINFO (objP->id);

		if ((objP->nType == OBJ_ROBOT) && (objP->id != ROBOT_BRAIN) &&
			 (objP->cType.aiInfo.behavior != AIB_SNIPE) && 
			 (objP->cType.aiInfo.behavior != AIB_RUN_FROM) && 
			 !botInfoP->bossFlag && 
			 !botInfoP->companion) {
			objP->cType.aiInfo.behavior = AIB_SNIPE;
			gameData.ai.localInfo [nObject].mode = AIM_SNIPE_ATTACK;
			return;
			}
		nObject = objP->next;
		}
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeWait (tObject *objP, tAILocal *ailP)
{
	fix xConnectedDist;

if ((gameData.ai.xDistToPlayer > F1_0 * 50) && (ailP->nextActionTime > 0))
	return;
ailP->nextActionTime = SNIPE_WAIT_TIME;
xConnectedDist = FindConnectedDistance (&objP->position.vPos, objP->nSegment, &gameData.ai.vBelievedPlayerPos, 
														 gameData.ai.nBelievedPlayerSeg, 30, WID_FLY_FLAG, 0);
if (xConnectedDist < MAX_SNIPE_DIST) {
	CreatePathToPlayer (objP, 30, 1);
	ailP->mode = AIM_SNIPE_ATTACK;
	ailP->nextActionTime = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeAttack (tObject *objP, tAILocal *ailP)
{
if (ailP->nextActionTime < 0) {
	ailP->mode = AIM_SNIPE_RETREAT;
	ailP->nextActionTime = SNIPE_WAIT_TIME;
	}
else {
	AIFollowPath (objP, gameData.ai.nPlayerVisibility, gameData.ai.nPlayerVisibility, &gameData.ai.vVecToPlayer);
	if (gameData.ai.nPlayerVisibility) {
		ailP->mode = AIM_SNIPE_FIRE;
		ailP->nextActionTime = SNIPE_FIRE_TIME;
		}
	else
		ailP->mode = AIM_SNIPE_ATTACK;
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeFire (tObject *objP, tAILocal *ailP)
{
if (ailP->nextActionTime < 0) {
	tAIStatic	*aiP = &objP->cType.aiInfo;
	CreateNSegmentPath (objP, 10 + d_rand () / 2048, OBJSEG (gameData.objs.console));
	aiP->nPathLength = SmoothPath (objP, &gameData.ai.pointSegs [aiP->nHideIndex], aiP->nPathLength);
	if (d_rand () < 8192)
		ailP->mode = AIM_SNIPE_RETREAT_BACKWARDS;
	else
		ailP->mode = AIM_SNIPE_RETREAT;
	ailP->nextActionTime = SNIPE_RETREAT_TIME;
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeRetreat (tObject *objP, tAILocal *ailP)
{
if (ailP->nextActionTime < 0) {
	ailP->mode = AIM_SNIPE_WAIT;
	ailP->nextActionTime = SNIPE_WAIT_TIME;
	}
else if ((gameData.ai.nPlayerVisibility == 0) || (ailP->nextActionTime > SNIPE_ABORT_RETREAT_TIME)) {
	AIFollowPath (objP, gameData.ai.nPlayerVisibility, gameData.ai.nPlayerVisibility, &gameData.ai.vVecToPlayer);
	ailP->mode = AIM_SNIPE_RETREAT_BACKWARDS;
	}
else {
	ailP->mode = AIM_SNIPE_FIRE;
	ailP->nextActionTime = SNIPE_FIRE_TIME/2;
	}
}

//	-------------------------------------------------------------------------------------------------

#ifdef _WIN32
typedef void __fastcall tAISnipeHandler (tObject *, tAILocal *);
#else
typedef void tAISnipeHandler (tObject *, tAILocal *);
#endif
typedef tAISnipeHandler *pAISnipeHandler;

pAISnipeHandler aiSnipeHandlers [] = {DoSnipeAttack, DoSnipeFire, DoSnipeRetreat, DoSnipeRetreat, DoSnipeWait};

void DoSnipeFrame (tObject *objP)
{
if (gameData.ai.xDistToPlayer <= MAX_SNIPE_DIST) {
	tAILocal		*ailP = gameData.ai.localInfo + OBJ_IDX (objP);
	int			i = ailP->mode;

	if ((i >= AIM_SNIPE_ATTACK) && (i <= AIM_SNIPE_WAIT))
		aiSnipeHandlers [i - AIM_SNIPE_ATTACK] (objP, ailP);
	else {
		Int3 ();	//	Oops, illegal mode for snipe behavior.
		ailP->mode = AIM_SNIPE_ATTACK;
		ailP->nextActionTime = F1_0;
		}
	}
}

//	-------------------------------------------------------------------------------------------------
