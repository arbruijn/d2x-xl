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

// --------------------------------------------------------------------------------------------------------------------

void AIIdleAnimation (tObject *objP)
{
if (gameOpts->gameplay.bIdleAnims) {
		int			h, i, j;
		tSegment		*segP = gameData.segs.segments + objP->nSegment;
		vmsVector	*vVertex, vVecToGoal, vGoal = gameData.objs.vRobotGoals [OBJ_IDX (objP)];

	for (i = 0; i < 8; i++) {
		vVertex = gameData.segs.vertices + segP->verts [i];
		if ((vGoal.p.x == vVertex->p.x) && (vGoal.p.y == vVertex->p.y) && (vGoal.p.z == vVertex->p.z))
			break;
		}
	VmVecNormalize (VmVecSub (&vVecToGoal, &vGoal, &objP->position.vPos));
	if (i == 8)
		h = 1;
	else if (AITurnTowardsVector (&vVecToGoal, objP, ROBOTINFO (objP->id).turnTime [2]) < F1_0 - F1_0 / 5) {
		if (VmVecDot (&vVecToGoal, &objP->position.mOrient.fVec) > F1_0 - F1_0 / 5)
			h = rand () % 2 == 0;
		else
			h = 0;
		}
	else if (MoveTowardsPoint (objP, &vGoal, objP->size * 3 / 2))
		h = rand () % 8 == 0;
	else
		h = 1;
	if (h && (rand () % 25 == 0)) {
		j = rand () % 8;
		if ((j == i) || (rand () % 3 == 0))
			COMPUTE_SEGMENT_CENTER_I (&vGoal, objP->nSegment);
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

int DoSillyAnimation (tObject *objP)
{
	int				nObject = OBJ_IDX (objP);
	tJointPos 		*jp_list;
	int				robotType, nGun, robotState, num_joint_positions;
	tPolyObjInfo	*polyObjInfo = &objP->rType.polyObjInfo;
	tAIStatic		*aiP = &objP->cType.aiInfo;
	int				num_guns, at_goal;
	int				attackType;
	int				flinch_attack_scale = 1;

	Assert (nObject >= 0);
	robotType = objP->id;
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
		int	joint;

		num_joint_positions = robot_get_animState (&jp_list, robotType, nGun, robotState);

		for (joint=0; joint<num_joint_positions; joint++) {
			fix			delta_angle, delta_2;
			int			jointnum = jp_list [joint].jointnum;
			vmsAngVec	*jp = &jp_list [joint].angles;
			vmsAngVec	*pObjP = &polyObjInfo->animAngles [jointnum];

			if (jointnum >= gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels) {
				Int3 ();		// Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			if (jp->p != pObjP->p) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].p = jp->p;

				delta_angle = jp->p - pObjP->p;
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

				gameData.ai.localInfo [nObject].deltaAngles [jointnum].p = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->b != pObjP->b) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].b = jp->b;

				delta_angle = jp->b - pObjP->b;
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

				gameData.ai.localInfo [nObject].deltaAngles [jointnum].b = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->h != pObjP->h) {
				if (nGun == 0)
					at_goal = 0;
				gameData.ai.localInfo [nObject].goalAngles [jointnum].h = jp->h;

				delta_angle = jp->h - pObjP->h;
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

				gameData.ai.localInfo [nObject].deltaAngles [jointnum].h = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//tAIStatic	*aiP = &objP->cType.aiInfo;
			tAILocal		*ailP = gameData.ai.localInfo + OBJ_IDX (objP);
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
//	Move all sub-gameData.objs.objects in an tObject towards their goals.
//	Current orientation of tObject is at:	polyObjInfo.animAngles
//	Goal orientation of tObject is at:		aiInfo.goalAngles
//	Delta orientation of tObject is at:		aiInfo.deltaAngles
void AIFrameAnimation (tObject *objP)
{
	int	nObject = OBJ_IDX (objP);
	int	joint;
	int	nJoints = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels;

	for (joint=1; joint<nJoints; joint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vmsAngVec	*curangp = &objP->rType.polyObjInfo.animAngles [joint];
		vmsAngVec	*goalangp = &gameData.ai.localInfo [nObject].goalAngles [joint];
		vmsAngVec	*deltaangp = &gameData.ai.localInfo [nObject].deltaAngles [joint];

		Assert (nObject >= 0);
		delta_to_goal = goalangp->p - curangp->p;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->p, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->p += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->p = goalangp->p;
		}

		delta_to_goal = goalangp->b - curangp->b;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->b, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->b += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->b = goalangp->b;
		}

		delta_to_goal = goalangp->h - curangp->h;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = FixMul (deltaangp->h, gameData.time.xFrame) * DELTA_ANG_SCALE;
			curangp->h += (fixang) scaled_delta_angle;
			if (abs (delta_to_goal) < abs (scaled_delta_angle))
				curangp->h = goalangp->h;
		}

	}

}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if tObject just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
int DoRobotDyingFrame (tObject *objP, fix StartTime, fix xRollDuration, sbyte *bDyingSoundPlaying, short deathSound, fix xExplScale, fix xSoundScale)
{
	fix	xRollVal, temp;
	fix	xSoundDuration;
	tDigiSound *soundP;

if (!xRollDuration)
	xRollDuration = F1_0/4;

xRollVal = FixDiv (gameData.time.xGame - StartTime, xRollDuration);

FixSinCos (FixMul (xRollVal, xRollVal), &temp, &objP->mType.physInfo.rotVel.p.x);
FixSinCos (xRollVal, &temp, &objP->mType.physInfo.rotVel.p.y);
FixSinCos (xRollVal-F1_0/8, &temp, &objP->mType.physInfo.rotVel.p.z);

temp = gameData.time.xGame - StartTime;
objP->mType.physInfo.rotVel.p.x = temp / 9;
objP->mType.physInfo.rotVel.p.y = temp / 5;
objP->mType.physInfo.rotVel.p.z = temp / 7;

if (gameOpts->sound.digiSampleRate) {
	soundP = gameData.pig.sound.pSounds + DigiXlatSound (deathSound);
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

void StartRobotDeathSequence (tObject *objP)
{
objP->cType.aiInfo.xDyingStartTime = gameData.time.xGame;
objP->cType.aiInfo.bDyingSoundPlaying = 0;
objP->cType.aiInfo.SKIP_AI_COUNT = 0;

}

//	----------------------------------------------------------------------

int DoAnyRobotDyingFrame (tObject *objP)
{
if (objP->cType.aiInfo.xDyingStartTime) {
	int bDeathRoll = ROBOTINFO (objP->id).bDeathRoll;
	int rval = DoRobotDyingFrame (objP, objP->cType.aiInfo.xDyingStartTime, min (bDeathRoll/2+1,6)*F1_0, &objP->cType.aiInfo.bDyingSoundPlaying, ROBOTINFO (objP->id).deathrollSound, bDeathRoll*F1_0/8, bDeathRoll*F1_0/2); 
	if (rval) {
		ExplodeObject (objP, F1_0/4);
		DigiLinkSoundToObject2 (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F2_0, F1_0*512, SOUNDCLASS_EXPLOSION);
		if ((gameData.missions.nCurrentLevel < 0) && (ROBOTINFO (objP->id).thief))
			RecreateThief (objP);
		}
	return 1;
	}
return 0;
}

//	---------------------------------------------------------------
// eof

