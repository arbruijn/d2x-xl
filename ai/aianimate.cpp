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
#include "fireball.h"
#include "segmath.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

void AIDoRandomPatrol (CObject *pObj, tAILocalInfo *pLocalInfo)
{
#if DBG
static int bPatrols = 1;
if (!bPatrols)
	return;
if (pObj->Index () == nDbgObj)
	BRP;
#endif
if (!IsMultiGame && gameOpts->gameplay.bIdleAnims && (pLocalInfo->mode == AIM_IDLING)) {
		int32_t		h, i, j;
		CSegment		*pSeg = SEGMENT (pObj->info.nSegment);
		CFixVector	*vVertex, vVecToGoal, vGoal = gameData.objData.vRobotGoals [pObj->Index ()];
		tRobotInfo	*pRobotInfo;

	for (i = 0; i < 8; i++) {
		if (pSeg->m_vertices [i] != 0xFFFF) {
			vVertex = gameData.segData.vertices + pSeg->m_vertices [i];
			if ((vGoal.v.coord.x == (*vVertex).v.coord.x) && (vGoal.v.coord.y == (*vVertex).v.coord.y) && (vGoal.v.coord.z == (*vVertex).v.coord.z))
				break;
			}
		}
	vVecToGoal = vGoal - pObj->info.position.vPos; 
	CFixVector::Normalize (vVecToGoal);
	if (i == 8)
		h = 1;
	else if ((pRobotInfo = ROBOTINFO (pObj)) && (AITurnTowardsVector (&vVecToGoal, pObj, pRobotInfo->turnTime [2]) < I2X (1) - I2X (1) / 5)) {
		if (CFixVector::Dot (vVecToGoal, pObj->info.position.mOrient.m.dir.f) > I2X (1) - I2X (1) / 5)
			h = Rand (2) == 0;
		else
			h = 0;
		}
	else if (MoveTowardsPoint (pObj, &vGoal, pObj->info.xSize * 3 / 2)) {
#if DBG
		pObj->CheckSpeed (1);
#endif
		h = Rand (8) == 0;
		}
	else
		h = 1;
	if (h && (Rand (25) == 0)) {
		j = Rand (8);
		if ((pSeg->m_vertices [j] == 0xFFFF) || (j == i) || (Rand (3) == 0))
			vGoal = SEGMENT (pObj->info.nSegment)->Center ();
		else
			vGoal = gameData.segData.vertices [pSeg->m_vertices [j]];
		gameData.objData.vRobotGoals [pObj->Index ()] = vGoal;
		DoSillyAnimation (pObj);
		}
	}
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0

int32_t     nFlinchScale = 4;
int32_t     nAttackScale = 24;

static int8_t   xlatAnimation [] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

int32_t DoSillyAnimation (CObject *pObj)
{
	int32_t			nObject = pObj->Index ();
	tJointPos*		jointPositions;
	int32_t			robotType = pObj->info.nId, nGun, robotState, nJointPositions;
	tPolyObjInfo*	polyObjInfo = &pObj->rType.polyObjInfo;
	tAIStaticInfo*	pStaticInfo = &pObj->cType.aiInfo;
	int32_t			nGunCount, at_goal;
	int32_t			attackType;
	int32_t			nFlinchAttackScale = 1;
	tRobotInfo*		pRobotInfo = ROBOTINFO (robotType);

if (!pRobotInfo)
	return 0;
if (0 > (nGunCount = pRobotInfo->nGuns))
	return 0;
attackType = pRobotInfo->attackType;
robotState = xlatAnimation [pStaticInfo->GOAL_STATE];
if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
	nFlinchAttackScale = nAttackScale;
else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
	nFlinchAttackScale = nFlinchScale;

at_goal = 1;
for (nGun = 0; nGun <= nGunCount; nGun++) {
	nJointPositions = RobotGetAnimState (&jointPositions, robotType, nGun, robotState);
	for (int32_t nJoint = 0; nJoint < nJointPositions; nJoint++) {
		int32_t				jointnum = jointPositions [nJoint].jointnum;

		if (jointnum >= gameData.models.polyModels [0][pObj->ModelId ()].ModelCount ())
			continue;

		CAngleVector*	jointAngles = &jointPositions [nJoint].angles;
		CAngleVector*	objAngles = &polyObjInfo->animAngles [jointnum];

		for (int32_t nAngle = 0; nAngle < 3; nAngle++) {
			if (jointAngles->v.vec [nAngle] != objAngles->v.vec [nAngle]) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].v.vec [nAngle] = jointAngles->v.vec [nAngle];
				fix delta2, deltaAngle = jointAngles->v.vec [nAngle] - objAngles->v.vec [nAngle];
				if (deltaAngle >= I2X (1) / 2)
					delta2 = -ANIM_RATE;
				else if (deltaAngle >= 0)
					delta2 = ANIM_RATE;
				else if (deltaAngle >= -I2X (1) / 2)
					delta2 = -ANIM_RATE;
				else
					delta2 = ANIM_RATE;
				if (nFlinchAttackScale != 1)
					delta2 *= nFlinchAttackScale;
				gameData.ai.localInfo [nObject].deltaAngles [jointnum].v.vec [nAngle] = delta2 / DELTA_ANG_SCALE;		// complete revolutions per second
				}
			}
		}

	if (at_goal) {
		tAILocalInfo* pLocalInfo = &gameData.ai.localInfo [pObj->Index ()];
		pLocalInfo->achievedState [nGun] = pLocalInfo->goalState [nGun];
		if (pLocalInfo->achievedState [nGun] == AIS_RECOVER)
			pLocalInfo->goalState [nGun] = AIS_FIRE;
		if (pLocalInfo->achievedState [nGun] == AIS_FLINCH)
			pLocalInfo->goalState [nGun] = AIS_LOCK;
		}
	}

if (at_goal == 1) //nGunCount)
	pStaticInfo->CURRENT_STATE = pStaticInfo->GOAL_STATE;
return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-OBJECTS in an CObject towards their goals.
//	Current orientation of CObject is at:	polyObjInfo.animAngles
//	Goal orientation of CObject is at:		aiInfo.goalAngles
//	Delta orientation of CObject is at:		aiInfo.deltaAngles
void AIFrameAnimation (CObject *pObj)
{
	int32_t	nObject = pObj->Index ();
	int32_t	nJoint;
	int32_t	nJoints = gameData.models.polyModels [0][pObj->ModelId ()].ModelCount ();

for (nJoint = 1; nJoint < nJoints; nJoint++) {
	fix				deltaToGoal;
	fix				scaledDeltaAngle;
	CAngleVector*	pCurAngle = &pObj->rType.polyObjInfo.animAngles [nJoint];
	CAngleVector*	pGoalAngle = &gameData.ai.localInfo [nObject].goalAngles [nJoint];
	CAngleVector*	pDeltaAngle = &gameData.ai.localInfo [nObject].deltaAngles [nJoint];

	Assert (nObject >= 0);
	for (int32_t nAngle = 0; nAngle < 3; nAngle++) {
		deltaToGoal = pGoalAngle->v.vec [nAngle] - pCurAngle->v.vec [nAngle];
		if (deltaToGoal > 32767)
			deltaToGoal -= 65536;
		else if (deltaToGoal < -32767)
			deltaToGoal += deltaToGoal;

		if (deltaToGoal) {
			scaledDeltaAngle = FixMul (pDeltaAngle->v.vec [nAngle], gameData.time.xFrame) * DELTA_ANG_SCALE;
			pCurAngle->v.vec [nAngle] += (fixang) scaledDeltaAngle;
			if (abs (deltaToGoal) < abs (scaledDeltaAngle))
				pCurAngle->v.vec [nAngle] = pGoalAngle->v.vec [nAngle];
			}
		}
	}
}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if CObject just died.
//	scale: I2X (4) for boss, much smaller for much smaller guys
int32_t DoRobotDyingFrame (CObject *pObj, fix StartTime, fix xRollDuration, int8_t *bDyingSoundPlaying, int16_t deathSound, fix xExplScale, fix xSoundScale)
{
	fix	xRollVal, temp;
	fix	xSoundDuration;
	CSoundSample *pSound;

if (!xRollDuration)
	xRollDuration = I2X (1)/4;

xRollVal = FixDiv (gameData.time.xGame - StartTime, xRollDuration);

FixSinCos (FixMul (xRollVal, xRollVal), &temp, &pObj->mType.physInfo.rotVel.v.coord.x);
FixSinCos (xRollVal, &temp, &pObj->mType.physInfo.rotVel.v.coord.y);
FixSinCos (xRollVal-I2X (1)/8, &temp, &pObj->mType.physInfo.rotVel.v.coord.z);

temp = gameData.time.xGame - StartTime;
pObj->mType.physInfo.rotVel.v.coord.x = temp / 9;
pObj->mType.physInfo.rotVel.v.coord.y = temp / 5;
pObj->mType.physInfo.rotVel.v.coord.z = temp / 7;
if (gameOpts->sound.audioSampleRate) {
	pSound = gameData.pig.sound.pSound + audio.XlatSound (deathSound);
	xSoundDuration = FixDiv (pSound->nLength [pSound->bHires], gameOpts->sound.audioSampleRate);
	if (!xSoundDuration)
		xSoundDuration = I2X (1);
	}
else
	xSoundDuration = I2X (1);

if (StartTime + xRollDuration - xSoundDuration < gameData.time.xGame) {
	if (!*bDyingSoundPlaying) {
#if TRACE
		console.printf (CON_DBG, "Starting death sound!\n");
#endif
		*bDyingSoundPlaying = 1;
		audio.CreateObjectSound (deathSound, SOUNDCLASS_ROBOT, pObj->Index (), 0, xSoundScale, xSoundScale * 256);	//	I2X (5)12 means play twice as loud
		}
	else if (RandShort () < gameData.time.xFrame * 16) {
		CreateSmallFireballOnObject (pObj, (I2X (1) + RandShort ()) * (16 * xExplScale/I2X (1)) / 8, RandShort () < gameData.time.xFrame * 2);
		}
	}
else if (RandShort () < gameData.time.xFrame * 8) {
	CreateSmallFireballOnObject (pObj, (I2X (1)/2 + RandShort ()) * (16 * xExplScale / I2X (1)) / 8, RandShort () < gameData.time.xFrame);
	}
return (StartTime + xRollDuration < gameData.time.xGame);
}

//	----------------------------------------------------------------------

void StartRobotDeathSequence (CObject *pObj)
{
if (!pObj->cType.aiInfo.xDyingStartTime) { // if not already dying
	pObj->cType.aiInfo.xDyingStartTime = gameData.time.xGame;
	pObj->cType.aiInfo.bDyingSoundPlaying = 0;
	pObj->cType.aiInfo.SKIP_AI_COUNT = 0;
	}
}

//	----------------------------------------------------------------------

int32_t DoAnyRobotDyingFrame (CObject *pObj)
{
if (pObj->cType.aiInfo.xDyingStartTime) {
	tRobotInfo	*pRobotInfo = ROBOTINFO (pObj);
	int32_t bDeathRoll = pRobotInfo ? pRobotInfo->bDeathRoll : 0;
	int32_t rval = DoRobotDyingFrame (pObj, pObj->cType.aiInfo.xDyingStartTime, I2X (Min (bDeathRoll / 2 + 1, 6)), 
												 &pObj->cType.aiInfo.bDyingSoundPlaying, pRobotInfo ? pRobotInfo->deathrollSound : 0, 
												 I2X (bDeathRoll) / 8, I2X (bDeathRoll) / 2);
	if (rval) {
		pObj->Explode (I2X (1)/4);
		audio.CreateObjectSound (SOUND_STANDARD_EXPLOSION, SOUNDCLASS_EXPLOSION, pObj->Index (), 0, I2X (2), I2X (512));
		if (!gameOpts->gameplay.bNoThief && (missionManager.nCurrentLevel < 0) && pObj->IsThief ())
			RecreateThief (pObj);
		}
	return 1;
	}
return 0;
}

//	---------------------------------------------------------------
// eof

