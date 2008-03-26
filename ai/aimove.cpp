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
#include "reactor.h"
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

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

//-------------------------------------------------------------------------------------------

fix AITurnTowardsVector (vmsVector *vGoal, tObject *objP, fix rate)
{
	vmsVector	new_fvec;
	fix			dot;

	//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
if (rate == 0)
	return 0;
if ((objP->id == BABY_SPIDER_ID) && (objP->nType == OBJ_ROBOT)) {
	PhysicsTurnTowardsVector (vGoal, objP, rate);
	return VmVecDot (vGoal, &objP->position.mOrient.fVec);
	}
new_fvec = *vGoal;
dot = VmVecDot (vGoal, &objP->position.mOrient.fVec);
if (!IsMultiGame)
	dot = (fix) (dot / gameStates.gameplay.slowmo [0].fSpeed);
if (dot < (F1_0 - gameData.time.xFrame/2)) {
	fix mag, new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
	if (!IsMultiGame)
		new_scale = (fix) (new_scale / gameStates.gameplay.slowmo [0].fSpeed);
	VmVecScale (&new_fvec, new_scale);
	VmVecInc (&new_fvec, &objP->position.mOrient.fVec);
	mag = VmVecNormalize (&new_fvec);
	if (mag < F1_0/256) {
#if TRACE
		con_printf (1, "Degenerate vector in AITurnTowardsVector (mag = %7.3f)\n", f2fl (mag));
#endif
		new_fvec = *vGoal;		//	if degenerate vector, go right to goal
		}
	}
if (gameStates.gameplay.seismic.nMagnitude) {
	vmsVector	rand_vec;
	fix			scale;
	MakeRandomVector (&rand_vec);
	scale = FixDiv (2*gameStates.gameplay.seismic.nMagnitude, ROBOTINFO (objP->id).mass);
	VmVecScaleInc (&new_fvec, &rand_vec, scale);
	}
VmVector2Matrix (&objP->position.mOrient, &new_fvec, NULL, &objP->position.mOrient.rVec);
return dot;
}

// --------------------------------------------------------------------------------------------------------------------
//	vGoalVec must be normalized, or close to it.
//	if bDotBased set, then speed is based on direction of movement relative to heading
void MoveTowardsVector (tObject *objP, vmsVector *vGoalVec, int bDotBased)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed, dot, xMaxSpeed, t, d;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->id);
	vmsVector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards tPlayer as usual.

vel = pptr->velocity;
VmVecNormalize (&vel);
dot = VmVecDot (&vel, &objP->position.mOrient.fVec);

if (botInfoP->thief)
	dot = (F1_0+dot)/2;

if (bDotBased && (dot < 3*F1_0/4)) {
	//	This funny code is supposed to slow down the robot and move his velocity towards his direction
	//	more quickly than the general code
	t = gameData.time.xFrame * 32;
	pptr->velocity.p.x = pptr->velocity.p.x/2 + FixMul (vGoalVec->p.x, t);
	pptr->velocity.p.y = pptr->velocity.p.y/2 + FixMul (vGoalVec->p.y, t);
	pptr->velocity.p.z = pptr->velocity.p.z/2 + FixMul (vGoalVec->p.z, t);
	} 
else {
	t = gameData.time.xFrame * 64;
	d = (gameStates.app.nDifficultyLevel + 5) / 4;
	pptr->velocity.p.x += FixMul (vGoalVec->p.x, t) * d;
	pptr->velocity.p.y += FixMul (vGoalVec->p.y, t) * d;
	pptr->velocity.p.z += FixMul (vGoalVec->p.z, t) * d;
	}
speed = VmVecMagQuick (&pptr->velocity);
xMaxSpeed = botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel];
//	Green guy attacks twice as fast as he moves away.
if ((botInfoP->attackType == 1) || botInfoP->thief || botInfoP->kamikaze)
	xMaxSpeed *= 2;
if (speed > xMaxSpeed) {
	pptr->velocity.p.x = (pptr->velocity.p.x * 3) / 4;
	pptr->velocity.p.y = (pptr->velocity.p.y * 3) / 4;
	pptr->velocity.p.z = (pptr->velocity.p.z * 3) / 4;
	}
}

// --------------------------------------------------------------------------------------------------------------------

void MoveTowardsPlayer (tObject *objP, vmsVector *vVecToPlayer)
//	gameData.ai.vVecToPlayer must be normalized, or close to it.
{
MoveTowardsVector (objP, vVecToPlayer, 1);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fastFlag == -1 means normal slide about.  fastFlag = 0 means no evasion.
void MoveAroundPlayer (tObject *objP, vmsVector *vVecToPlayer, int fastFlag)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->id);
	int				nObject = OBJ_IDX (objP);
	int				dir;
	int				dir_change;
	fix				ft;
	vmsVector		vEvade;
	int				count=0;

	if (fastFlag == 0)
		return;

	dir_change = 48;
	ft = gameData.time.xFrame;
	if (ft < F1_0/32) {
		dir_change *= 8;
		count += 3;
	} else
		while (ft < F1_0/4) {
			dir_change *= 2;
			ft *= 2;
			count++;
		}

	dir = (gameData.app.nFrameCount + (count+1) * (nObject*8 + nObject*4 + nObject)) & dir_change;
	dir >>= (4+count);

	Assert ((dir >= 0) && (dir <= 3));
	ft = gameData.time.xFrame * 32;
	switch (dir) {
		case 0:
			vEvade.p.x = FixMul (gameData.ai.vVecToPlayer.p.z, ft);
			vEvade.p.y = FixMul (gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.z = FixMul (-gameData.ai.vVecToPlayer.p.x, ft);
			break;
		case 1:
			vEvade.p.x = FixMul (-gameData.ai.vVecToPlayer.p.z, ft);
			vEvade.p.y = FixMul (gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.z = FixMul (gameData.ai.vVecToPlayer.p.x, ft);
			break;
		case 2:
			vEvade.p.x = FixMul (-gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.y = FixMul (gameData.ai.vVecToPlayer.p.x, ft);
			vEvade.p.z = FixMul (gameData.ai.vVecToPlayer.p.z, ft);
			break;
		case 3:
			vEvade.p.x = FixMul (gameData.ai.vVecToPlayer.p.y, ft);
			vEvade.p.y = FixMul (-gameData.ai.vVecToPlayer.p.x, ft);
			vEvade.p.z = FixMul (gameData.ai.vVecToPlayer.p.z, ft);
			break;
		default:
			Error ("Function MoveAroundPlayer: Bad case.");
	}

	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fastFlag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = VmVecDot (&gameData.ai.vVecToPlayer, &objP->position.mOrient.fVec);
		if ((dot > botInfoP->fieldOfView [gameStates.app.nDifficultyLevel]) && 
			 !(gameData.objs.console->flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			if (botInfoP->strength)
				damage_scale = FixDiv (objP->shields, botInfoP->strength);
			else
				damage_scale = F1_0;
			if (damage_scale > F1_0)
				damage_scale = F1_0;		//	Just in cased:\temp\dm_test.
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in cased:\temp\dm_test.

			VmVecScale (&vEvade, i2f (fastFlag) + damage_scale);
		}
	}

	pptr->velocity.p.x += vEvade.p.x;
	pptr->velocity.p.y += vEvade.p.y;
	pptr->velocity.p.z += vEvade.p.z;

	speed = VmVecMagQuick (&pptr->velocity);
	if ((OBJ_IDX (objP) != 1) && (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel])) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void MoveAwayFromPlayer (tObject *objP, vmsVector *vVecToPlayer, int attackType)
{
	fix				speed;
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->id);
	int				objref;
	fix				ft = gameData.time.xFrame * 16;

	pptr->velocity.p.x -= FixMul (gameData.ai.vVecToPlayer.p.x, ft);
	pptr->velocity.p.y -= FixMul (gameData.ai.vVecToPlayer.p.y, ft);
	pptr->velocity.p.z -= FixMul (gameData.ai.vVecToPlayer.p.z, ft);

	if (attackType) {
		//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
		objref = ((OBJ_IDX (objP)) ^ ((gameData.app.nFrameCount + 3* (OBJ_IDX (objP))) >> 5)) & 3;

		switch (objref) {
			case 0:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.uVec, gameData.time.xFrame << 5);
				break;
			case 1:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.uVec, -gameData.time.xFrame << 5);
				break;
			case 2:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.rVec, gameData.time.xFrame << 5);
				break;
			case 3:
				VmVecScaleInc (&pptr->velocity, &objP->position.mOrient.rVec, -gameData.time.xFrame << 5);
				break;
			default:
				Int3 ();	//	Impossible, bogus value on objref, must be in 0d:\temp\dm_test3
		}
	}


	speed = VmVecMagQuick (&pptr->velocity);

	if (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
		pptr->velocity.p.x = (pptr->velocity.p.x*3)/4;
		pptr->velocity.p.y = (pptr->velocity.p.y*3)/4;
		pptr->velocity.p.z = (pptr->velocity.p.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag bEvadeOnly is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_IDLING).
void AIMoveRelativeToPlayer (tObject *objP, tAILocal *ailP, fix xDistToPlayer, 
									  vmsVector *vVecToPlayer, fix circleDistance, int bEvadeOnly, 
									  int nPlayerVisibility)
{
	tObject		*dObjP;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->id);

	Assert (gameData.ai.nPlayerVisibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((botInfoP->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
if (objP->cType.aiInfo.nDangerLaser != -1) {
	dObjP = &gameData.objs.objects [objP->cType.aiInfo.nDangerLaser];

	if ((dObjP->nType == OBJ_WEAPON) && (dObjP->nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
		fix			dot, xDistToLaser, fieldOfView;
		vmsVector	vVecToLaser, fVecLaser;

		fieldOfView = ROBOTINFO (objP->id).fieldOfView [gameStates.app.nDifficultyLevel];
		VmVecSub (&vVecToLaser, &dObjP->position.vPos, &objP->position.vPos);
		xDistToLaser = VmVecNormalize (&vVecToLaser);
		dot = VmVecDot (&vVecToLaser, &objP->position.mOrient.fVec);

		if ((dot > fieldOfView) || (botInfoP->companion)) {
			fix			dotLaserRobot;
			vmsVector	vLaserToRobot;

			//	The laser is seen by the robot, see if it might hit the robot.
			//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
			if (dObjP->renderType == RT_POLYOBJ)
				fVecLaser = dObjP->position.mOrient.fVec;
			else {		//	Not a polyobjP, get velocity and normalize.
				fVecLaser = dObjP->mType.physInfo.velocity;	//dObjP->position.mOrient.fVec;
				VmVecNormalize (&fVecLaser);
				}
			VmVecSub (&vLaserToRobot, &objP->position.vPos, &dObjP->position.vPos);
			VmVecNormalize (&vLaserToRobot);
			dotLaserRobot = VmVecDot (&fVecLaser, &vLaserToRobot);

			if ((dotLaserRobot > F1_0*7/8) && (xDistToLaser < F1_0*80)) {
				int evadeSpeed = ROBOTINFO (objP->id).evadeSpeed [gameStates.app.nDifficultyLevel];
				gameData.ai.bEvaded = 1;
				MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, evadeSpeed);
				}
			}
		return;
		}
	}

//	If only allowed to do evade code, then done.
//	Hmm, perhaps brilliant insight.  If want claw-nType guys to keep coming, don't return here after evasion.
if (!(botInfoP->attackType || botInfoP->thief) && bEvadeOnly)
	return;
//	If we fall out of above, then no tObject to be avoided.
objP->cType.aiInfo.nDangerLaser = -1;
//	Green guy selects move around/towards/away based on firing time, not distance.
if (botInfoP->attackType == 1) {
	if (!gameStates.app.bPlayerIsDead &&
		 ((ailP->nextPrimaryFire <= botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) || 
		  (gameData.ai.xDistToPlayer >= F1_0 * 30)))
		MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
		//	1/4 of time, move around tPlayer, 3/4 of time, move away from tPlayer
	else if (d_rand () < 8192)
		MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, -1);
	else
		MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 1);
	}
else if (botInfoP->thief)
	MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
else {
	int	objval = ((OBJ_IDX (objP)) & 0x0f) ^ 0x0a;

	//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
	if (botInfoP->kamikaze)
		MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
	else if (gameData.ai.xDistToPlayer < circleDistance)
		MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 0);
	else if ((gameData.ai.xDistToPlayer < (3+objval)*circleDistance/2) && (ailP->nextPrimaryFire > -F1_0))
		MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, -1);
	else
		if ((-ailP->nextPrimaryFire > F1_0 + (objval << 12)) && gameData.ai.nPlayerVisibility)
			//	Usually move away, but sometimes move around player.
			if ((((gameData.time.xGame >> 18) & 0x0f) ^ objval) > 4) 
				MoveAwayFromPlayer (objP, &gameData.ai.vVecToPlayer, 0);
			else
				MoveAroundPlayer (objP, &gameData.ai.vVecToPlayer, -1);
		else
			MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
	}
}

// --------------------------------------------------------------------------------------------------------------------

fix MoveObjectToLegalPoint (tObject *objP, vmsVector *vGoal)
{
	vmsVector	vGoalDir;
	fix			xDistToGoal;

VmVecSub (&vGoalDir, vGoal, &objP->position.vPos);
xDistToGoal = VmVecNormalize (&vGoalDir);
VmVecScale (&vGoalDir, objP->size / 2);
VmVecInc (&objP->position.vPos, &vGoalDir);
return xDistToGoal;
}

// --------------------------------------------------------------------------------------------------------------------
//	Move the tObject objP to a spot in which it doesn't intersect a tWall.
//	It might mean moving it outside its current tSegment.
void MoveObjectToLegalSpot (tObject *objP, int bMoveToCenter)
{
	vmsVector	vSegCenter, vOrigPos = objP->position.vPos;
	int			i;
	tSegment		*segP = gameData.segs.segments + objP->nSegment;

if (bMoveToCenter) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->nSegment);
	MoveObjectToLegalPoint (objP, &vSegCenter);
	return;
	}
else {
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
		if (WALL_IS_DOORWAY (segP, (short) i, objP) & WID_FLY_FLAG) {
			COMPUTE_SEGMENT_CENTER_I (&vSegCenter, segP->children [i]);
			MoveObjectToLegalPoint (objP, &vSegCenter);
			if (ObjectIntersectsWall (objP))
				objP->position.vPos = vOrigPos;
			else {
				int nNewSeg = FindSegByPoint (&objP->position.vPos, objP->nSegment, 1, 0);
				if (nNewSeg != -1) {
					RelinkObject (OBJ_IDX (objP), nNewSeg);
					return;
					}
				}
			}
		}
	}

if (ROBOTINFO (objP->id).bossFlag) {
	Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
	TeleportBoss (objP);
	}
	else {
#if TRACE
		con_printf (CONDBG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", OBJ_IDX (objP));
#endif
		ApplyDamageToRobot (objP, objP->shields*2, OBJ_IDX (objP));
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move tObject one tObject radii from current position towards tSegment center.
//	If tSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsPoint (tObject *objP, vmsVector *vGoal, fix xMinDist)
{
	fix			xDistToGoal;
	vmsVector	vGoalDir;

VmVecSub (&vGoalDir, vGoal, &objP->position.vPos);
xDistToGoal = VmVecNormalize (&vGoalDir);
if (xDistToGoal - objP->size <= xMinDist) {
	//	Center is nearer than the distance we want to move, so move to center.
	if (!xMinDist) {
		objP->position.vPos = *vGoal;
		if (ObjectIntersectsWall (objP))
			MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	xDistToGoal = 0;
	} 
else {
	int	nNewSeg;
	fix	xRemDist = xDistToGoal - xMinDist, 
			xScale = (objP->size < xRemDist) ? objP->size : xRemDist;

	if (xMinDist)
		xScale /= 20;
	//	Move one radius towards center.
	VmVecScale (&vGoalDir, xScale);
	VmVecInc (&objP->position.vPos, &vGoalDir);
	nNewSeg = FindSegByPoint (&objP->position.vPos, objP->nSegment, 1, 0);
	if (nNewSeg == -1) {
		objP->position.vPos = *vGoal;
		MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	}
return xDistToGoal;
}

// --------------------------------------------------------------------------------------------------------------------
//	Move tObject one tObject radii from current position towards tSegment center.
//	If tSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsSegmentCenter (tObject *objP)
{
	vmsVector	vSegCenter;

COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->nSegment);
return MoveTowardsPoint (objP, &vSegCenter, 0);
}

//int	Buddy_got_stuck = 0;

//	---------------------------------------------------------------
// eof

