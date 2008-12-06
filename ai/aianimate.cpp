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
#include "fireball.h"
#include "gameseg.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
//#define _DEBUG
#if DBG
#include "string.h"
#include <time.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

void AIIdleAnimation (CObject *objP)
{
#if DBG
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (gameOpts->gameplay.bIdleAnims) {
		int			h, i, j;
		CSegment		*segP = gameData.segs.segments + objP->info.nSegment;
		vmsVector	*vVertex, vVecToGoal, vGoal = gameData.objs.vRobotGoals [OBJ_IDX (objP)];

	for (i = 0; i < 8; i++) {
		vVertex = gameData.segs.vertices + segP->verts [i];
		if ((vGoal[X] == (*vVertex)[X]) && (vGoal[Y] == (*vVertex)[Y]) && (vGoal[Z] == (*vVertex)[Z]))
			break;
		}
	vVecToGoal = vGoal - objP->info.position.vPos; vmsVector::Normalize(vVecToGoal);
	if (i == 8)
		h = 1;
	else if (AITurnTowardsVector (&vVecToGoal, objP, ROBOTINFO (objP->info.nId).turnTime [2]) < F1_0 - F1_0 / 5) {
		if (vmsVector::Dot(vVecToGoal, objP->info.position.mOrient[FVEC]) > F1_0 - F1_0 / 5)
			h = rand () % 2 == 0;
		else
			h = 0;
		}
	else if (MoveTowardsPoint (objP, &vGoal, objP->info.xSize * 3 / 2))
		h = rand () % 8 == 0;
	else
		h = 1;
	if (h && (rand () % 25 == 0)) {
		j = rand () % 8;
		if ((j == i) || (rand () % 3 == 0))
			COMPUTE_SEGMENT_CENTER_I (&vGoal, objP->info.nSegment);
		else
			vGoal = gameData.segs.vertices [segP->verts [j]];
		gameData.objs.vRobotGoals [OBJ_IDX (objP)] = vGoal;
		DoSillyAnimation (objP);
		}
	}
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
int     nFlinchScale = 4;
int     nAttackScale = 24;
sbyte   xlatAnimation [] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

int DoSillyAnimation (CObject *objP)
{
	int				nObject = OBJ_IDX (objP);
	tJointPos 		*jp_list;
	int				robotType, nGun, robotState, nJointPositions;
	tPolyObjInfo	*polyObjInfo = &objP->rType.polyObjInfo;
	tAIStaticInfo		*aiP = &objP->cType.aiInfo;
	int				num_guns, at_goal;
	int				attackType;
	int				flinch_attack_scale = 1;

	Assert (nObject >= 0);
	robotType = objP->info.nId;
	num_guns = ROBOTINFO (robotType).nGuns;
	attackType = ROBOTINFO (robotType).attackType;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goalState, not GOAL_STATE.
	robotState = xlatAnimation [aiP->GOAL_STATE];
	// previousRobotState = xlatAnimation [aiP->CURRENT_STATE];

	if (attackType) // && ((robotState == AS_FIRE) || (robotState == AS_RECOIL)))
		flinch_attack_scale = nAttackScale;
	else if ((robotState == AS_FLINCH) || (robotState == AS_RECOIL))
		flinch_attack_scale = nFlinchScale;

	at_goal = 1;
	for (nGun=0; nGun <= num_guns; nGun++) {
		int	nJoint;

		nJointPositions = RobotGetAnimState (&jp_list, robotType, nGun, robotState);

		for (nJoint = 0; nJoint < nJointPositions; nJoint++) {
			fix			delta_angle, delta_2;
			int			jointnum = jp_list [nJoint].jointnum;
			vmsAngVec	*jp = &jp_list [nJoint].angles;
			vmsAngVec	*pObjP = &polyObjInfo->animAngles [jointnum];

			if (jointnum >= gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels) {
				Int3 ();		// Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			if ((*jp)[PA] != (*pObjP)[PA]) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum][PA] = (*jp)[PA];

				delta_angle = (*jp)[PA] - (*pObjP)[PA];
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				gameData.ai.localInfo [nObject].deltaAngles [jointnum][PA] = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if ((*jp)[BA] != (*pObjP)[BA]) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum][BA] = (*jp)[BA];

				delta_angle = (*jp)[BA] - (*pObjP)[BA];
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				gameData.ai.localInfo [nObject].deltaAngles [jointnum][BA] = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if ((*jp)[HA] != (*pObjP)[HA]) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum][HA] = (*jp)[HA];

				delta_angle = (*jp)[HA] - (*pObjP)[HA];
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				gameData.ai.localInfo [nObject].deltaAngles [jointnum][HA] = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//tAIStaticInfo	*aiP = &objP->cType.aiInfo;
			tAILocalInfo		*ailP = gameData.ai.localInfo + OBJ_IDX (objP);
			ailP->achievedState [nGun] = ailP->goalState [nGun];
			if (ailP->achievedState [nGun] == AIS_RECOVER)
				ailP->goalState [nGun] = AIS_FIRE;

			if (ailP->achievedState [nGun] == AIS_FLINCH)
				ailP->goalState [nGun] = AIS_LOCK;

		}
	}

	if (at_goal == 1) //num_guns)
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
	int	nObject = OBJ_IDX (objP);
	int	nJoint;
	int	nJoints = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels;

	for (nJoint=1; nJoint<nJoints; nJoint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vmsAngVec	*curangp = &objP->rType.polyObjInfo.animAngles [nJoint];
		vmsAngVec	*goalangp = &gameData.ai.localInfo [nObject].goalAngles [nJoint];
		vmsAngVec	*deltaangp = &gameData.ai.localInfo [nObject].deltaAngles [nJoint];

		Assert (nObject >= 0);
		delta_to_goal = (*goalangp)[PA] - (*curangp)[PA];
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul ((*deltaangp)[PA], gameData.time.xFrame) * DELTA_ANG_SCALE;
			(*curangp)[PA] += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				(*curangp)[PA] = (*goalangp)[PA];
		}

		delta_to_goal = (*goalangp)[BA] - (*curangp)[BA];
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul ((*deltaangp)[BA], gameData.time.xFrame) * DELTA_ANG_SCALE;
			(*curangp)[BA] += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				(*curangp)[BA] = (*goalangp)[BA];
		}

		delta_to_goal = (*goalangp)[HA] - (*curangp)[HA];
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul ((*deltaangp)[HA], gameData.time.xFrame) * DELTA_ANG_SCALE;
			(*curangp)[HA] += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				(*curangp)[HA] = (*goalangp)[HA];
		}

	}

}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if CObject just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
int DoRobotDyingFrame (CObject *objP, fix StartTime, fix xRollDuration, sbyte *bDyingSoundPlaying, short deathSound, fix xExplScale, fix xSoundScale)
{
	fix	xRollVal, temp;
	fix	xSoundDuration;
	tDigiSound *soundP;

if (!xRollDuration)
	xRollDuration = F1_0/4;

xRollVal = FixDiv (gameData.time.xGame - StartTime, xRollDuration);

FixSinCos (FixMul (xRollVal, xRollVal), &temp, &objP->mType.physInfo.rotVel[X]);
FixSinCos (xRollVal, &temp, &objP->mType.physInfo.rotVel[Y]);
FixSinCos (xRollVal-F1_0/8, &temp, &objP->mType.physInfo.rotVel[Z]);

temp = gameData.time.xGame - StartTime;
objP->mType.physInfo.rotVel[X] = temp / 9;
objP->mType.physInfo.rotVel[Y] = temp / 5;
objP->mType.physInfo.rotVel[Z] = temp / 7;

if (gameOpts->sound.digiSampleRate) {
	soundP = gameData.pig.sound.soundP + DigiXlatSound (deathSound);
	xSoundDuration = FixDiv (soundP->nLength [soundP->bHires], gameOpts->sound.digiSampleRate);
	}
else
	xSoundDuration = F1_0;

if (StartTime + xRollDuration - xSoundDuration < gameData.time.xGame) {
	if (!*bDyingSoundPlaying) {
#if TRACE
		con_printf (CONDBG, "Starting death sound!\n");
#endif
		*bDyingSoundPlaying = 1;
		DigiLinkSoundToObject2 (deathSound, OBJ_IDX (objP), 0, xSoundScale, xSoundScale * 256, SOUNDCLASS_ROBOT);	//	F1_0*512 means play twice as loud
		}
	else if (d_rand () < gameData.time.xFrame*16)
		CreateSmallFireballOnObject (objP, (F1_0 + d_rand ()) * (16 * xExplScale/F1_0) / 8, 0);
	}
else if (d_rand () < gameData.time.xFrame * 8)
	CreateSmallFireballOnObject (objP, (F1_0/2 + d_rand ()) * (16 * xExplScale / F1_0) / 8, 1);

return (StartTime + xRollDuration < gameData.time.xGame);
}

//	----------------------------------------------------------------------

void StartRobotDeathSequence (CObject *objP)
{
objP->cType.aiInfo.xDyingStartTime = gameData.time.xGame;
objP->cType.aiInfo.bDyingSoundPlaying = 0;
objP->cType.aiInfo.SKIP_AI_COUNT = 0;

}

//	----------------------------------------------------------------------

int DoAnyRobotDyingFrame (CObject *objP)
{
if (objP->cType.aiInfo.xDyingStartTime) {
	int bDeathRoll = ROBOTINFO (objP->info.nId).bDeathRoll;
	int rval = DoRobotDyingFrame (objP, objP->cType.aiInfo.xDyingStartTime, min (bDeathRoll / 2 + 1, 6) * F1_0, 
											&objP->cType.aiInfo.bDyingSoundPlaying, ROBOTINFO (objP->info.nId).deathrollSound, 
											bDeathRoll * F1_0 / 8, bDeathRoll * F1_0 / 2);
	if (rval) {
		ExplodeObject (objP, F1_0/4);
		DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512, SOUNDCLASS_EXPLOSION);
		if ((gameData.missions.nCurrentLevel < 0) && (ROBOTINFO (objP->info.nId).thief))
			RecreateThief (objP);
		}
	return 1;
	}
return 0;
}

//	---------------------------------------------------------------
// eof

