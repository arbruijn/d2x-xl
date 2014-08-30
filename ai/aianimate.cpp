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

void AIDoRandomPatrol (CObject *objP, tAILocalInfo *ailP)
{
#if DBG
static int bPatrols = 1;
if (!bPatrols)
	return;
if (objP->Index () == nDbgObj)
	BRP;
#endif
if (!IsMultiGame && gameOpts->gameplay.bIdleAnims && (ailP->mode == AIM_IDLING)) {
		int32_t		h, i, j;
		CSegment*	segP = SEGMENTS + objP->info.nSegment;
		CFixVector*	vVertex, vVecToGoal, vGoal = gameData.objs.vRobotGoals [objP->Index ()];

	for (i = 0; i < 8; i++) {
		if (segP->m_vertices [i] != 0xFFFF) {
			vVertex = gameData.segs.vertices + segP->m_vertices [i];
			if ((vGoal.v.coord.x == (*vVertex).v.coord.x) && (vGoal.v.coord.y == (*vVertex).v.coord.y) && (vGoal.v.coord.z == (*vVertex).v.coord.z))
				break;
			}
		}
	vVecToGoal = vGoal - objP->info.position.vPos; 
	CFixVector::Normalize (vVecToGoal);
	if (i == 8)
		h = 1;
	else if (AITurnTowardsVector (&vVecToGoal, objP, ROBOTINFO (objP->info.nId).turnTime [2]) < I2X (1) - I2X (1) / 5) {
		if (CFixVector::Dot (vVecToGoal, objP->info.position.mOrient.m.dir.f) > I2X (1) - I2X (1) / 5)
			h = Rand (2) == 0;
		else
			h = 0;
		}
	else if (MoveTowardsPoint (objP, &vGoal, objP->info.xSize * 3 / 2)) {
#if DBG
		objP->CheckSpeed (1);
#endif
		h = Rand (8) == 0;
		}
	else
		h = 1;
	if (h && (Rand (25) == 0)) {
		j = Rand (8);
		if ((segP->m_vertices [j] == 0xFFFF) || (j == i) || (Rand (3) == 0))
			vGoal = SEGMENTS [objP->info.nSegment].Center ();
		else
			vGoal = gameData.segs.vertices [segP->m_vertices [j]];
		gameData.objs.vRobotGoals [objP->Index ()] = vGoal;
		DoSillyAnimation (objP);
		}
	}
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0

int32_t     nFlinchScale = 4;
int32_t     nAttackScale = 24;

static int8_t   xlatAnimation [] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

int32_t DoSillyAnimation (CObject *objP)
{
	int32_t			nObject = objP->Index ();
	tJointPos*		jointPositions;
	int32_t			robotType, nGun, robotState, nJointPositions;
	tPolyObjInfo*	polyObjInfo = &objP->rType.polyObjInfo;
	tAIStaticInfo*	aiP = &objP->cType.aiInfo;
	int32_t			nGunCount, at_goal;
	int32_t			attackType;
	int32_t			nFlinchAttackScale = 1;

robotType = objP->info.nId;
if (0 > (nGunCount = ROBOTINFO (robotType).nGuns))
	return 0;
attackType = ROBOTINFO (robotType).attackType;
robotState = xlatAnimation [aiP->GOAL_STATE];
if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
	nFlinchAttackScale = nAttackScale;
else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
	nFlinchAttackScale = nFlinchScale;

at_goal = 1;
for (nGun = 0; nGun <= nGunCount; nGun++) {
	nJointPositions = RobotGetAnimState (&jointPositions, robotType, nGun, robotState);
	for (int32_t nJoint = 0; nJoint < nJointPositions; nJoint++) {
		int32_t				jointnum = jointPositions [nJoint].jointnum;

		if (jointnum >= gameData.models.polyModels [0][objP->ModelId ()].ModelCount ())
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
		tAILocalInfo* ailP = &gameData.ai.localInfo [objP->Index ()];
		ailP->achievedState [nGun] = ailP->goalState [nGun];
		if (ailP->achievedState [nGun] == AIS_RECOVER)
			ailP->goalState [nGun] = AIS_FIRE;
		if (ailP->achievedState [nGun] == AIS_FLINCH)
			ailP->goalState [nGun] = AIS_LOCK;
		}
	}

if (at_goal == 1) //nGunCount)
	aiP->CURRENT_STATE = aiP->GOAL_STATE;
return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-OBJECTS in an CObject towards their goals.
//	Current orientation of CObject is at:	polyObjInfo.animAngles
//	Goal orientation of CObject is at:		aiInfo.goalAngles
//	Delta orientation of CObject is at:		aiInfo.deltaAngles
void AIFrameAnimation (CObject *objP)
{
	int32_t	nObject = objP->Index ();
	int32_t	nJoint;
	int32_t	nJoints = gameData.models.polyModels [0][objP->ModelId ()].ModelCount ();

for (nJoint = 1; nJoint < nJoints; nJoint++) {
	fix				deltaToGoal;
	fix				scaledDeltaAngle;
	CAngleVector*	curAngP = &objP->rType.polyObjInfo.animAngles [nJoint];
	CAngleVector*	goalAngP = &gameData.ai.localInfo [nObject].goalAngles [nJoint];
	CAngleVector*	deltaAngP = &gameData.ai.localInfo [nObject].deltaAngles [nJoint];

	Assert (nObject >= 0);
	for (int32_t nAngle = 0; nAngle < 3; nAngle++) {
		deltaToGoal = goalAngP->v.vec [nAngle] - curAngP->v.vec [nAngle];
		if (deltaToGoal > 32767)
			deltaToGoal -= 65536;
		else if (deltaToGoal < -32767)
			deltaToGoal += deltaToGoal;

		if (deltaToGoal) {
			scaledDeltaAngle = FixMul (deltaAngP->v.vec [nAngle], gameData.time.xFrame) * DELTA_ANG_SCALE;
			curAngP->v.vec [nAngle] += (fixang) scaledDeltaAngle;
			if (abs (deltaToGoal) < abs (scaledDeltaAngle))
				curAngP->v.vec [nAngle] = goalAngP->v.vec [nAngle];
			}
		}
	}
}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if CObject just died.
//	scale: I2X (4) for boss, much smaller for much smaller guys
int32_t DoRobotDyingFrame (CObject *objP, fix StartTime, fix xRollDuration, int8_t *bDyingSoundPlaying, int16_t deathSound, fix xExplScale, fix xSoundScale)
{
	fix	xRollVal, temp;
	fix	xSoundDuration;
	CSoundSample *soundP;

if (!xRollDuration)
	xRollDuration = I2X (1)/4;

xRollVal = FixDiv (gameData.time.xGame - StartTime, xRollDuration);

FixSinCos (FixMul (xRollVal, xRollVal), &temp, &objP->mType.physInfo.rotVel.v.coord.x);
FixSinCos (xRollVal, &temp, &objP->mType.physInfo.rotVel.v.coord.y);
FixSinCos (xRollVal-I2X (1)/8, &temp, &objP->mType.physInfo.rotVel.v.coord.z);

temp = gameData.time.xGame - StartTime;
objP->mType.physInfo.rotVel.v.coord.x = temp / 9;
objP->mType.physInfo.rotVel.v.coord.y = temp / 5;
objP->mType.physInfo.rotVel.v.coord.z = temp / 7;
if (gameOpts->sound.audioSampleRate) {
	soundP = gameData.pig.sound.soundP + audio.XlatSound (deathSound);
	xSoundDuration = FixDiv (soundP->nLength [soundP->bHires], gameOpts->sound.audioSampleRate);
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
		audio.CreateObjectSound (deathSound, SOUNDCLASS_ROBOT, objP->Index (), 0, xSoundScale, xSoundScale * 256);	//	I2X (5)12 means play twice as loud
		}
	else if (RandShort () < gameData.time.xFrame * 16) {
		CreateSmallFireballOnObject (objP, (I2X (1) + RandShort ()) * (16 * xExplScale/I2X (1)) / 8, RandShort () < gameData.time.xFrame * 2);
		}
	}
else if (RandShort () < gameData.time.xFrame * 8) {
	CreateSmallFireballOnObject (objP, (I2X (1)/2 + RandShort ()) * (16 * xExplScale / I2X (1)) / 8, RandShort () < gameData.time.xFrame);
	}
return (StartTime + xRollDuration < gameData.time.xGame);
}

//	----------------------------------------------------------------------

void StartRobotDeathSequence (CObject *objP)
{
if (!objP->cType.aiInfo.xDyingStartTime) { // if not already dying
	objP->cType.aiInfo.xDyingStartTime = gameData.time.xGame;
	objP->cType.aiInfo.bDyingSoundPlaying = 0;
	objP->cType.aiInfo.SKIP_AI_COUNT = 0;
	}
}

//	----------------------------------------------------------------------

int32_t DoAnyRobotDyingFrame (CObject *objP)
{
if (objP->cType.aiInfo.xDyingStartTime) {
	int32_t bDeathRoll = ROBOTINFO (objP->info.nId).bDeathRoll;
	int32_t rval = DoRobotDyingFrame (objP, objP->cType.aiInfo.xDyingStartTime, I2X (Min (bDeathRoll / 2 + 1, 6)), 
											&objP->cType.aiInfo.bDyingSoundPlaying, ROBOTINFO (objP->info.nId).deathrollSound, 
											I2X (bDeathRoll) / 8, I2X (bDeathRoll) / 2);
	if (rval) {
		objP->Explode (I2X (1)/4);
		audio.CreateObjectSound (SOUND_STANDARD_EXPLOSION, SOUNDCLASS_EXPLOSION, objP->Index (), 0, I2X (2), I2X (512));
		if (!gameOpts->gameplay.bNoThief && (missionManager.nCurrentLevel < 0) && (ROBOTINFO (objP->info.nId).thief))
			RecreateThief (objP);
		}
	return 1;
	}
return 0;
}

//	---------------------------------------------------------------
// eof

