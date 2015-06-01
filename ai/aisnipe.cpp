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
#include "findpath.h"

#include "string.h"

#if DBG
#include <time.h>
#endif

//	-----------------------------------------------------------------------------
// Make a robot near the player snipe.
#define	MNRS_SEG_MAX	70

void MakeNearbyRobotSnipe (void)
{
ENTER (1, 0);
	CObject		*pObj;
	int16_t		bfsList [MNRS_SEG_MAX];
	int32_t		nObject, nBfsLength, i;

CreateBfsList (OBJSEG (TARGETOBJ), bfsList, &nBfsLength, MNRS_SEG_MAX);
for (i = 0; i < nBfsLength; i++) {
	for (nObject = SEGMENT (bfsList [i])->m_objects; nObject >= 0; nObject = pObj->info.nNextInSeg) {
		pObj = OBJECT (nObject);
		if (pObj->info.nType != OBJ_ROBOT)  
			continue;
		if ((pObj->info.nId == ROBOT_BRAIN) || (pObj->info.nId == 255))
			continue;
		if ((pObj->cType.aiInfo.behavior == AIB_SNIPE) || (pObj->cType.aiInfo.behavior == AIB_RUN_FROM))
			continue;
		if (pObj->IsBoss () || pObj->IsGuideBot ())
			continue;
		pObj->cType.aiInfo.behavior = AIB_SNIPE;
		gameData.aiData.localInfo [nObject].mode = AIM_SNIPE_ATTACK;
		LEAVE
		}
	}
LEAVE
}

//	-----------------------------------------------------------------------------

void DoSnipeWait (CObject *pObj, tAILocalInfo *pLocalInfo)
{
ENTER (1, 0);
if ((gameData.aiData.target.xDist > I2X (50)) && (pLocalInfo->nextActionTime > 0))
	LEAVE
pLocalInfo->nextActionTime = SNIPE_WAIT_TIME;
fix xConnectedDist = simpleRouter [0].PathLength (pObj->info.position.vPos, pObj->info.nSegment, 
																  gameData.aiData.target.vBelievedPos, gameData.aiData.target.nBelievedSeg, 
															     30, WID_PASSABLE_FLAG, 1);
if (xConnectedDist < MAX_SNIPE_DIST) {
	CreatePathToTarget (pObj, 30, 1);
	pLocalInfo->mode = AIM_SNIPE_ATTACK;
	pLocalInfo->nextActionTime = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
	}
LEAVE
}

//	-----------------------------------------------------------------------------

void DoSnipeAttack (CObject *pObj, tAILocalInfo *pLocalInfo)
{
ENTER (1, 0);
if (pLocalInfo->nextActionTime < 0) {
	pLocalInfo->mode = AIM_SNIPE_RETREAT;
	pLocalInfo->nextActionTime = SNIPE_WAIT_TIME;
	}
else {
	AIFollowPath (pObj, gameData.aiData.nTargetVisibility, gameData.aiData.nTargetVisibility, &gameData.aiData.target.vDir);
	if (gameData.aiData.nTargetVisibility) {
		pLocalInfo->mode = AIM_SNIPE_FIRE;
		pLocalInfo->nextActionTime = SNIPE_FIRE_TIME;
		}
	else
		pLocalInfo->mode = AIM_SNIPE_ATTACK;
	}
LEAVE
}

//	-----------------------------------------------------------------------------

void DoSnipeFire (CObject *pObj, tAILocalInfo *pLocalInfo)
{
ENTER (1, 0);
if (pLocalInfo->nextActionTime < 0) {
	tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
	CreateNSegmentPath (pObj, 10 + RandShort () / 2048, OBJSEG (TARGETOBJ));
	pStaticInfo->nPathLength = (pStaticInfo->nHideIndex < 0) ? 0 : SmoothPath (pObj, &gameData.aiData.routeSegs [pStaticInfo->nHideIndex], pStaticInfo->nPathLength);
	if (RandShort () < 8192)
		pLocalInfo->mode = AIM_SNIPE_RETREAT_BACKWARDS;
	else
		pLocalInfo->mode = AIM_SNIPE_RETREAT;
	pLocalInfo->nextActionTime = SNIPE_RETREAT_TIME;
	}
LEAVE
}

//	-----------------------------------------------------------------------------

void DoSnipeRetreat (CObject *pObj, tAILocalInfo *pLocalInfo)
{
ENTER (1, 0);
if (pLocalInfo->nextActionTime < 0) {
	pLocalInfo->mode = AIM_SNIPE_WAIT;
	pLocalInfo->nextActionTime = SNIPE_WAIT_TIME;
	}
else if ((gameData.aiData.nTargetVisibility == 0) || (pLocalInfo->nextActionTime > SNIPE_ABORT_RETREAT_TIME)) {
	AIFollowPath (pObj, gameData.aiData.nTargetVisibility, gameData.aiData.nTargetVisibility, &gameData.aiData.target.vDir);
	pLocalInfo->mode = AIM_SNIPE_RETREAT_BACKWARDS;
	}
else {
	pLocalInfo->mode = AIM_SNIPE_FIRE;
	pLocalInfo->nextActionTime = SNIPE_FIRE_TIME/2;
	}
LEAVE
}

//	-----------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef void __fastcall tAISnipeHandler (CObject *, tAILocalInfo *);
#else
typedef void tAISnipeHandler (CObject *, tAILocalInfo *);
#endif
typedef tAISnipeHandler *pAISnipeHandler;

pAISnipeHandler aiSnipeHandlers [] = {DoSnipeAttack, DoSnipeFire, DoSnipeRetreat, DoSnipeRetreat, DoSnipeWait};

void DoSnipeFrame (CObject *pObj)
{
ENTER (1, 0);
if (gameData.aiData.target.xDist <= MAX_SNIPE_DIST) {
	tAILocalInfo		*pLocalInfo = gameData.aiData.localInfo + pObj->Index ();
	int32_t			i = pLocalInfo->mode;

	if ((i >= AIM_SNIPE_ATTACK) && (i <= AIM_SNIPE_WAIT))
		aiSnipeHandlers [i - AIM_SNIPE_ATTACK] (pObj, pLocalInfo);
	else {
		Int3 ();	//	Oops, illegal mode for snipe behavior.
		pLocalInfo->mode = AIM_SNIPE_ATTACK;
		pLocalInfo->nextActionTime = I2X (1);
		}
	}
LEAVE
}

//	-----------------------------------------------------------------------------
// eof
