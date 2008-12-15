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
#include "gameseg.h"
#include "physics.h"
#include "collide.h"
#include "network.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
//#define _DEBUG
#if DBG
#include "string.h"
#include <time.h>
#endif

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

//-------------------------------------------------------------------------------------------

fix AITurnTowardsVector (CFixVector *vGoal, CObject *objP, fix rate)
{
	CFixVector	new_fvec;
	fix			dot;

	//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
if (rate == 0)
	return 0;
if ((objP->info.nId == BABY_SPIDER_ID) && (objP->info.nType == OBJ_ROBOT)) {
	PhysicsTurnTowardsVector (vGoal, objP, rate);
	return CFixVector::Dot(*vGoal, objP->info.position.mOrient.FVec ());
	}
new_fvec = *vGoal;
dot = CFixVector::Dot(*vGoal, objP->info.position.mOrient.FVec ());
if (!IsMultiGame)
	dot = (fix) (dot / gameStates.gameplay.slowmo [0].fSpeed);
if (dot < (F1_0 - gameData.time.xFrame/2)) {
	fix mag, new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
	if (!IsMultiGame)
		new_scale = (fix) (new_scale / gameStates.gameplay.slowmo [0].fSpeed);
	new_fvec *= new_scale;
	new_fvec += objP->info.position.mOrient.FVec ();
	mag = CFixVector::Normalize(new_fvec);
	if (mag < F1_0/256) {
#if TRACE
		con_printf (1, "Degenerate vector in AITurnTowardsVector (mag = %7.3f)\n", X2F (mag));
#endif
		new_fvec = *vGoal;		//	if degenerate vector, go right to goal
		}
	}
if (gameStates.gameplay.seismic.nMagnitude) {
	CFixVector	rand_vec;
	fix			scale;
	rand_vec = CFixVector::Random();
	scale = FixDiv (2*gameStates.gameplay.seismic.nMagnitude, ROBOTINFO (objP->info.nId).mass);
	new_fvec += rand_vec * scale;
	}
objP->info.position.mOrient = CFixMatrix::CreateFR(new_fvec, objP->info.position.mOrient.RVec ());
return dot;
}

// --------------------------------------------------------------------------------------------------------------------
//	vGoalVec must be normalized, or close to it.
//	if bDotBased set, then speed is based on direction of movement relative to heading
void MoveTowardsVector (CObject *objP, CFixVector *vGoalVec, int bDotBased)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed, dot, xMaxSpeed, t, d;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->info.nId);
	CFixVector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards CPlayerData as usual.

vel = pptr->velocity;
CFixVector::Normalize(vel);
dot = CFixVector::Dot(vel, objP->info.position.mOrient.FVec ());

if (botInfoP->thief)
	dot = (F1_0+dot)/2;

if (bDotBased && (dot < 3*F1_0/4)) {
	//	This funny code is supposed to slow down the robot and move his velocity towards his direction
	//	more quickly than the general code
	t = gameData.time.xFrame * 32;
	pptr->velocity[X] = pptr->velocity[X]/2 + FixMul ((*vGoalVec)[X], t);
	pptr->velocity[Y] = pptr->velocity[Y]/2 + FixMul ((*vGoalVec)[Y], t);
	pptr->velocity[Z] = pptr->velocity[Z]/2 + FixMul ((*vGoalVec)[Z], t);
	}
else {
	t = gameData.time.xFrame * 64;
	d = (gameStates.app.nDifficultyLevel + 5) / 4;
	pptr->velocity[X] += FixMul ((*vGoalVec)[X], t) * d;
	pptr->velocity[Y] += FixMul ((*vGoalVec)[Y], t) * d;
	pptr->velocity[Z] += FixMul ((*vGoalVec)[Z], t) * d;
	}
speed = pptr->velocity.Mag();
xMaxSpeed = botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel];
//	Green guy attacks twice as fast as he moves away.
if ((botInfoP->attackType == 1) || botInfoP->thief || botInfoP->kamikaze)
	xMaxSpeed *= 2;
if (speed > xMaxSpeed) {
	pptr->velocity[X] = (pptr->velocity[X] * 3) / 4;
	pptr->velocity[Y] = (pptr->velocity[Y] * 3) / 4;
	pptr->velocity[Z] = (pptr->velocity[Z] * 3) / 4;
	}
}

// --------------------------------------------------------------------------------------------------------------------

void MoveAwayFromOtherRobots (CObject *objP, CFixVector& vVecToPlayer)
{
	CFixVector		vPos, vAvoidPos, vNewPos;
	fix				xDist, xAvoidRad;
	short				nStartSeg, nDestSeg, nObject, nSide, nAvoidObjs;
	CObject			*avoidObjP;
	CSegment			*segP;
	tFVIQuery		fq;
	tFVIData			hitData;
	int				hitType;

vAvoidPos.SetZero ();
xAvoidRad = 0;
nAvoidObjs = 0;
nStartSeg = objP->info.nSegment;
vPos = objP->info.position.vPos;
if ((objP->info.nType == OBJ_ROBOT) && !ROBOTINFO (objP->info.nId).companion) {
	// move out from all other robots in same segment that are too close
	for (nObject = SEGMENTS [nStartSeg].objects; nObject != -1; nObject = avoidObjP->info.nNextInSeg) {
		avoidObjP = OBJECTS + nObject;
		if ((avoidObjP->info.nType != OBJ_ROBOT) || (avoidObjP->info.nSignature >= objP->info.nSignature))
			continue;	// comparing the sigs ensures that only one of two bots tested against each other will move, keeping them from bouncing around
		xDist = CFixVector::Dist (vPos, avoidObjP->info.position.vPos);
		if (xDist >= (objP->info.xSize + avoidObjP->info.xSize))
			continue;
		xAvoidRad += (objP->info.xSize + avoidObjP->info.xSize);
		vAvoidPos += avoidObjP->info.position.vPos;
		nAvoidObjs++;
		}
	if (nAvoidObjs) {
		xAvoidRad /= nAvoidObjs;
		vAvoidPos /= nAvoidObjs * F1_0;
		segP = SEGMENTS + nStartSeg;
		for (nAvoidObjs = 5; nAvoidObjs; nAvoidObjs--) {
			vNewPos = CFixVector::Random ();
			vNewPos *= objP->info.xSize;
			vNewPos += vAvoidPos;
			if (0 > (nDestSeg = FindSegByPos (vNewPos, nStartSeg, 0, 0)))
				continue;
			if (nStartSeg != nDestSeg) {
				if (0 > (nSide = FindConnectedSide (segP, SEGMENTS + nDestSeg)))
					continue;
				if (!((WALL_IS_DOORWAY (segP, nSide, NULL) & WID_FLY_FLAG) || (AIDoorIsOpenable (objP, segP, nSide))))
					continue;
				fq.p0					= &objP->info.position.vPos;
				fq.startSeg			= nStartSeg;
				fq.p1					= &vNewPos;
				fq.radP0				=
				fq.radP1				= objP->info.xSize;
				fq.thisObjNum		= OBJ_IDX (objP);
				fq.ignoreObjList	= NULL;
				fq.flags				= 0;
				hitType = FindVectorIntersection (&fq, &hitData);
				if (hitType != HIT_NONE)
					continue;
				}
			vVecToPlayer = vNewPos - vAvoidPos;
			CFixVector::Normalize (vVecToPlayer); //CFixVector::Avg (vVecToPlayer, vNewPos);
			break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

void MoveTowardsPlayer (CObject *objP, CFixVector *vVecToPlayer)
//	gameData.ai.vVecToPlayer must be normalized, or close to it.
{
MoveAwayFromOtherRobots (objP, *vVecToPlayer);
MoveTowardsVector (objP, vVecToPlayer, 1);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fastFlag == -1 means Normal slide about.  fastFlag = 0 means no evasion.
void MoveAroundPlayer (CObject *objP, CFixVector *vVecToPlayer, int fastFlag)
{
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	fix				speed;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->info.nId);
	int				nObject = OBJ_IDX (objP);
	int				dir;
	int				dir_change;
	fix				ft;
	CFixVector		vEvade;
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
			vEvade[X] = FixMul (gameData.ai.vVecToPlayer[Z], ft);
			vEvade[Y] = FixMul (gameData.ai.vVecToPlayer[Y], ft);
			vEvade[Z] = FixMul (-gameData.ai.vVecToPlayer[X], ft);
			break;
		case 1:
			vEvade[X] = FixMul (-gameData.ai.vVecToPlayer[Z], ft);
			vEvade[Y] = FixMul (gameData.ai.vVecToPlayer[Y], ft);
			vEvade[Z] = FixMul (gameData.ai.vVecToPlayer[X], ft);
			break;
		case 2:
			vEvade[X] = FixMul (-gameData.ai.vVecToPlayer[Y], ft);
			vEvade[Y] = FixMul (gameData.ai.vVecToPlayer[X], ft);
			vEvade[Z] = FixMul (gameData.ai.vVecToPlayer[Z], ft);
			break;
		case 3:
			vEvade[X] = FixMul (gameData.ai.vVecToPlayer[Y], ft);
			vEvade[Y] = FixMul (-gameData.ai.vVecToPlayer[X], ft);
			vEvade[Z] = FixMul (gameData.ai.vVecToPlayer[Z], ft);
			break;
		default:
			Error ("Function MoveAroundPlayer: Bad case.");
	}

	//	Note: -1 means Normal circling about the player.  > 0 means fast evasion.
	if (fastFlag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = CFixVector::Dot(gameData.ai.vVecToPlayer, objP->info.position.mOrient.FVec ());
		if ((dot > botInfoP->fieldOfView [gameStates.app.nDifficultyLevel]) &&
			 !(gameData.objs.consoleP->info.nFlags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			if (botInfoP->strength)
				damage_scale = FixDiv (objP->info.xShields, botInfoP->strength);
			else
				damage_scale = F1_0;
			if (damage_scale > F1_0)
				damage_scale = F1_0;		//	Just in cased:\temp\dm_test.
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in cased:\temp\dm_test.

			vEvade *= (I2X (fastFlag) + damage_scale);
		}
	}

	pptr->velocity[X] += vEvade[X];
	pptr->velocity[Y] += vEvade[Y];
	pptr->velocity[Z] += vEvade[Z];

	speed = pptr->velocity.Mag();
	if ((OBJ_IDX (objP) != 1) && (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel])) {
		pptr->velocity[X] = (pptr->velocity[X]*3)/4;
		pptr->velocity[Y] = (pptr->velocity[Y]*3)/4;
		pptr->velocity[Z] = (pptr->velocity[Z]*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void MoveAwayFromPlayer (CObject *objP, CFixVector *vVecToPlayer, int attackType)
{
	fix				speed;
	tPhysicsInfo	*pptr = &objP->mType.physInfo;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->info.nId);
	int				objref;
	fix				ft = gameData.time.xFrame * 16;

	pptr->velocity -= gameData.ai.vVecToPlayer * ft;

	if (attackType) {
		//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
		objref = ((OBJ_IDX (objP)) ^ ((gameData.app.nFrameCount + 3* (OBJ_IDX (objP))) >> 5)) & 3;

		switch (objref) {
			case 0:
				pptr->velocity += objP->info.position.mOrient.UVec () * (gameData.time.xFrame << 5);
				break;
			case 1:
				pptr->velocity += objP->info.position.mOrient.UVec () * (-gameData.time.xFrame << 5);
				break;
			case 2:
				pptr->velocity += objP->info.position.mOrient.RVec () * (gameData.time.xFrame << 5);
				break;
			case 3:
				pptr->velocity += objP->info.position.mOrient.RVec () * ( -gameData.time.xFrame << 5);
				break;
			default:
				Int3 ();	//	Impossible, bogus value on objref, must be in 0d:\temp\dm_test3
		}
	}


	speed = pptr->velocity.Mag();

	if (speed > botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel]) {
		pptr->velocity[X] = (pptr->velocity[X]*3)/4;
		pptr->velocity[Y] = (pptr->velocity[Y]*3)/4;
		pptr->velocity[Z] = (pptr->velocity[Z]*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag bEvadeOnly is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_IDLING).
void AIMoveRelativeToPlayer (CObject *objP, tAILocalInfo *ailP, fix xDistToPlayer,
									  CFixVector *vVecToPlayer, fix circleDistance, int bEvadeOnly,
									  int nPlayerVisibility)
{
	CObject		*dObjP;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->info.nId);

	Assert (gameData.ai.nPlayerVisibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((botInfoP->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
if (objP->cType.aiInfo.nDangerLaser != -1) {
	dObjP = &OBJECTS [objP->cType.aiInfo.nDangerLaser];

	if ((dObjP->info.nType == OBJ_WEAPON) && (dObjP->info.nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
		fix			dot, xDistToLaser, fieldOfView;
		CFixVector	vVecToLaser, fVecLaser;

		fieldOfView = ROBOTINFO (objP->info.nId).fieldOfView [gameStates.app.nDifficultyLevel];
		vVecToLaser = dObjP->info.position.vPos - objP->info.position.vPos;
		xDistToLaser = CFixVector::Normalize(vVecToLaser);
		dot = CFixVector::Dot(vVecToLaser, objP->info.position.mOrient.FVec ());

		if ((dot > fieldOfView) || (botInfoP->companion)) {
			fix			dotLaserRobot;
			CFixVector	vLaserToRobot;

			//	The laser is seen by the robot, see if it might hit the robot.
			//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
			if (dObjP->info.renderType == RT_POLYOBJ)
				fVecLaser = dObjP->info.position.mOrient.FVec ();
			else {		//	Not a polyobjP, get velocity and Normalize.
				fVecLaser = dObjP->mType.physInfo.velocity;	//dObjP->info.position.mOrient.FVec ();
				CFixVector::Normalize(fVecLaser);
				}
			vLaserToRobot = objP->info.position.vPos - dObjP->info.position.vPos;
			CFixVector::Normalize(vLaserToRobot);
			dotLaserRobot = CFixVector::Dot(fVecLaser, vLaserToRobot);

			if ((dotLaserRobot > F1_0*7/8) && (xDistToLaser < F1_0*80)) {
				int evadeSpeed = ROBOTINFO (objP->info.nId).evadeSpeed [gameStates.app.nDifficultyLevel];
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
//	If we fall out of above, then no CObject to be avoided.
objP->cType.aiInfo.nDangerLaser = -1;
//	Green guy selects move around/towards/away based on firing time, not distance.
if (botInfoP->attackType == 1) {
	if (!gameStates.app.bPlayerIsDead &&
		 ((ailP->nextPrimaryFire <= botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) ||
		  (gameData.ai.xDistToPlayer >= F1_0 * 30)))
		MoveTowardsPlayer (objP, &gameData.ai.vVecToPlayer);
		//	1/4 of time, move around CPlayerData, 3/4 of time, move away from CPlayerData
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

fix MoveObjectToLegalPoint (CObject *objP, CFixVector *vGoal)
{
	CFixVector	vGoalDir;
	fix			xDistToGoal;

vGoalDir = *vGoal - objP->info.position.vPos;
xDistToGoal = CFixVector::Normalize(vGoalDir);
vGoalDir *= (objP->info.xSize / 2);
objP->info.position.vPos += vGoalDir;
return xDistToGoal;
}

// --------------------------------------------------------------------------------------------------------------------
//	Move the CObject objP to a spot in which it doesn't intersect a tWall.
//	It might mean moving it outside its current CSegment.
void MoveObjectToLegalSpot (CObject *objP, int bMoveToCenter)
{
	CFixVector	vSegCenter, vOrigPos = objP->info.position.vPos;
	int			i;
	CSegment		*segP = gameData.segs.segments + objP->info.nSegment;

if (bMoveToCenter) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->info.nSegment);
	MoveObjectToLegalPoint (objP, &vSegCenter);
	return;
	}
else {
	for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
		if (WALL_IS_DOORWAY (segP, (short) i, objP) & WID_FLY_FLAG) {
			COMPUTE_SEGMENT_CENTER_I (&vSegCenter, segP->children [i]);
			MoveObjectToLegalPoint (objP, &vSegCenter);
			if (ObjectIntersectsWall (objP))
				objP->info.position.vPos = vOrigPos;
			else {
				int nNewSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
				if (nNewSeg != -1) {
					objP->RelinkToSeg (nNewSeg);
					return;
					}
				}
			}
		}
	}

if (ROBOTINFO (objP->info.nId).bossFlag) {
	Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
	TeleportBoss (objP);
	}
	else {
#if TRACE
		con_printf (CONDBG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", OBJ_IDX (objP));
#endif
		ApplyDamageToRobot (objP, objP->info.xShields*2, OBJ_IDX (objP));
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsPoint (CObject *objP, CFixVector *vGoal, fix xMinDist)
{
	fix			xDistToGoal;
	CFixVector	vGoalDir;

#if DBG
if ((nDbgSeg >= 0) && (objP->info.nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif
vGoalDir = *vGoal - objP->info.position.vPos;
xDistToGoal = CFixVector::Normalize(vGoalDir);
if (xDistToGoal - objP->info.xSize <= xMinDist) {
	//	Center is nearer than the distance we want to move, so move to center.
	if (!xMinDist) {
		objP->info.position.vPos = *vGoal;
		if (ObjectIntersectsWall (objP))
			MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	xDistToGoal = 0;
	}
else {
	int	nNewSeg;
	fix	xRemDist = xDistToGoal - xMinDist,
			xScale = (objP->info.xSize < xRemDist) ? objP->info.xSize : xRemDist;

	if (xMinDist)
		xScale /= 20;
	//	Move one radius towards center.
	vGoalDir *= xScale;
	objP->info.position.vPos += vGoalDir;
	nNewSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
	if (nNewSeg == -1) {
		objP->info.position.vPos = *vGoal;
		MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	}
UnstickObject (objP);
return xDistToGoal;
}

// --------------------------------------------------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsSegmentCenter (CObject *objP)
{
	CFixVector	vSegCenter;

COMPUTE_SEGMENT_CENTER_I (&vSegCenter, objP->info.nSegment);
return MoveTowardsPoint (objP, &vSegCenter, 0);
}

//int	Buddy_got_stuck = 0;

//	---------------------------------------------------------------
// eof

