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
#include "gameseg.h"

#include "string.h"
//#define _DEBUG
#if DBG
#include <time.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Make a robot near the CPlayerData snipe.
#define	MNRS_SEG_MAX	70

void MakeNearbyRobotSnipe (void)
{
	CObject		*objP;
	tRobotInfo	*botInfoP;
	short			bfsList [MNRS_SEG_MAX];
	int			nObject, nBfsLength, i;

CreateBfsList (OBJSEG (gameData.objs.consoleP), bfsList, &nBfsLength, MNRS_SEG_MAX);
for (i = 0; i < nBfsLength; i++) {
	nObject = SEGMENTS [bfsList [i]].m_objects;
	//Assert (nObject >= 0);
	while (nObject != -1) {
		objP = OBJECTS + nObject;
		botInfoP = &ROBOTINFO (objP->info.nId);

		if ((objP->info.nType == OBJ_ROBOT) && (objP->info.nId != ROBOT_BRAIN) &&
			 (objP->cType.aiInfo.behavior != AIB_SNIPE) && 
			 (objP->cType.aiInfo.behavior != AIB_RUN_FROM) && 
			 !botInfoP->bossFlag && 
			 !botInfoP->companion) {
			objP->cType.aiInfo.behavior = AIB_SNIPE;
			gameData.ai.localInfo [nObject].mode = AIM_SNIPE_ATTACK;
			return;
			}
		nObject = objP->info.nNextInSeg;
		}
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeWait (CObject *objP, tAILocalInfo *ailP)
{
	fix xConnectedDist;

if ((gameData.ai.xDistToPlayer > I2X (50)) && (ailP->nextActionTime > 0))
	return;
ailP->nextActionTime = SNIPE_WAIT_TIME;
xConnectedDist = FindConnectedDistance (objP->info.position.vPos, objP->info.nSegment, gameData.ai.vBelievedPlayerPos, 
													 gameData.ai.nBelievedPlayerSeg, 30, WID_FLY_FLAG, 0);
if (xConnectedDist < MAX_SNIPE_DIST) {
	CreatePathToPlayer (objP, 30, 1);
	ailP->mode = AIM_SNIPE_ATTACK;
	ailP->nextActionTime = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeAttack (CObject *objP, tAILocalInfo *ailP)
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

void DoSnipeFire (CObject *objP, tAILocalInfo *ailP)
{
if (ailP->nextActionTime < 0) {
	tAIStaticInfo	*aiP = &objP->cType.aiInfo;
	CreateNSegmentPath (objP, 10 + d_rand () / 2048, OBJSEG (gameData.objs.consoleP));
	aiP->nPathLength = (aiP->nHideIndex < 0) ? 0 : SmoothPath (objP, &gameData.ai.routeSegs [aiP->nHideIndex], aiP->nPathLength);
	if (d_rand () < 8192)
		ailP->mode = AIM_SNIPE_RETREAT_BACKWARDS;
	else
		ailP->mode = AIM_SNIPE_RETREAT;
	ailP->nextActionTime = SNIPE_RETREAT_TIME;
	}
}

//	-------------------------------------------------------------------------------------------------

void DoSnipeRetreat (CObject *objP, tAILocalInfo *ailP)
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

#if defined(_WIN32) && !DBG
typedef void __fastcall tAISnipeHandler (CObject *, tAILocalInfo *);
#else
typedef void tAISnipeHandler (CObject *, tAILocalInfo *);
#endif
typedef tAISnipeHandler *pAISnipeHandler;

pAISnipeHandler aiSnipeHandlers [] = {DoSnipeAttack, DoSnipeFire, DoSnipeRetreat, DoSnipeRetreat, DoSnipeWait};

void DoSnipeFrame (CObject *objP)
{
if (gameData.ai.xDistToPlayer <= MAX_SNIPE_DIST) {
	tAILocalInfo		*ailP = gameData.ai.localInfo + objP->Index ();
	int			i = ailP->mode;

	if ((i >= AIM_SNIPE_ATTACK) && (i <= AIM_SNIPE_WAIT))
		aiSnipeHandlers [i - AIM_SNIPE_ATTACK] (objP, ailP);
	else {
		Int3 ();	//	Oops, illegal mode for snipe behavior.
		ailP->mode = AIM_SNIPE_ATTACK;
		ailP->nextActionTime = I2X (1);
		}
	}
}

//	-------------------------------------------------------------------------------------------------
