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
#include "segmath.h"
#include "physics.h"
#include "collide.h"
#include "network.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

// -----------------------------------------------------------------------------

fix AITurnTowardsVector (CFixVector *vGoal, CObject *objP, fix rate)
{
//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
if (rate == 0)
	return 0;
if ((objP->info.nId == BABY_SPIDER_ID) && (objP->info.nType == OBJ_ROBOT)) {
	objP->TurnTowardsVector (*vGoal, rate);
	return CFixVector::Dot (*vGoal, objP->info.position.mOrient.m.dir.f);
	}

CFixVector fVecNew = *vGoal;
fix dot = CFixVector::Dot (*vGoal, objP->info.position.mOrient.m.dir.f);

if (!IsMultiGame)
	dot = (fix) (dot / gameStates.gameplay.slowmo [0].fSpeed);
if (dot < (I2X (1) - gameData.time.xFrame / 2)) {
	fix mag, new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
	if (!IsMultiGame)
		new_scale = (fix) (new_scale / gameStates.gameplay.slowmo [0].fSpeed);
	fVecNew *= new_scale;
	fVecNew += objP->info.position.mOrient.m.dir.f;
	mag = CFixVector::Normalize (fVecNew);
	if (mag < I2X (1)/256) {
#if TRACE
		console.printf (1, "Degenerate vector in AITurnTowardsVector (mag = %7.3f)\n", X2F (mag));
#endif
		fVecNew = *vGoal;		//	if degenerate vector, go right to goal
		}
	}
if (gameStates.gameplay.seismic.nMagnitude) {
	CFixVector vRandom = CFixVector::Random();
	fix scale = FixDiv (2*gameStates.gameplay.seismic.nMagnitude, ROBOTINFO (objP->info.nId).mass);
	fVecNew += vRandom * scale;
	}
objP->info.position.mOrient = CFixMatrix::CreateFR(fVecNew, objP->info.position.mOrient.m.dir.r);
return dot;
}

// -----------------------------------------------------------------------------
//	vGoalVec must be normalized, or close to it.
//	if bDotBased set, then speed is based on direction of movement relative to heading
void MoveTowardsVector (CObject *objP, CFixVector *vGoalVec, int32_t bDotBased)
{
	tPhysicsInfo&	physicsInfo = objP->mType.physInfo;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->info.nId);

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards CPlayerData as usual.

CFixVector v = physicsInfo.velocity;
CFixVector::Normalize (v);
fix dot = CFixVector::Dot (v, objP->info.position.mOrient.m.dir.f);

if (botInfoP->thief)
	dot = (I2X (1) + dot) / 2;

if (bDotBased && (dot < I2X (3) / 4)) 
	//	This funny code is supposed to slow down the robot and move his velocity towards his direction
	//	more quickly than the general code
	physicsInfo.velocity += *vGoalVec * (gameData.time.xFrame * 32);
else 
	physicsInfo.velocity += *vGoalVec * ((gameData.time.xFrame * 64) * (5 * gameStates.app.nDifficultyLevel / 4));
fix speed = physicsInfo.velocity.Mag ();
fix xMaxSpeed = botInfoP->xMaxSpeed [gameStates.app.nDifficultyLevel];
//	Green guy attacks twice as fast as he moves away.
if ((botInfoP->attackType == 1) || botInfoP->thief || botInfoP->kamikaze)
	xMaxSpeed *= 2;
if (speed > xMaxSpeed)
	physicsInfo.velocity *= I2X (3) / 4;
}

// -----------------------------------------------------------------------------

void MoveAwayFromOtherRobots (CObject *objP, CFixVector& vVecToTarget)
{
	fix				xAvoidRad = 0;
	int16_t			nAvoidObjs = 0;
	int16_t			nStartSeg = objP->info.nSegment;
	CFixVector		vPos = objP->info.position.vPos;
	CFixVector		vAvoidPos;

vAvoidPos.SetZero ();
if ((objP->info.nType == OBJ_ROBOT) && !ROBOTINFO (objP->info.nId).companion) {
	// move out from all other robots in same segment that are too close
	CObject* avoidObjP;
	for (int16_t nObject = SEGMENT (nStartSeg)->m_objects; nObject != -1; nObject = avoidObjP->info.nNextInSeg) {
		avoidObjP = OBJECT (nObject);
		if ((avoidObjP->info.nType != OBJ_ROBOT) || (avoidObjP->info.nSignature >= objP->info.nSignature))
			continue;	// comparing the sigs ensures that only one of two bots tested against each other will move, keeping them from bouncing around
		fix xDist = CFixVector::Dist (vPos, avoidObjP->info.position.vPos);
		if (xDist >= (objP->info.xSize + avoidObjP->info.xSize))
			continue;
		xAvoidRad += (objP->info.xSize + avoidObjP->info.xSize);
		vAvoidPos += avoidObjP->info.position.vPos;
		nAvoidObjs++;
		}
	if (nAvoidObjs) {
		xAvoidRad /= nAvoidObjs;
		vAvoidPos /= I2X (nAvoidObjs);
		CSegment* segP = SEGMENT (nStartSeg);
		for (nAvoidObjs = 5; nAvoidObjs; nAvoidObjs--) {
			CFixVector vNewPos = CFixVector::Random ();
			vNewPos *= objP->info.xSize;
			vNewPos += vAvoidPos;
			int16_t nDestSeg = FindSegByPos (vNewPos, nStartSeg, 0, 0);
			if (0 > nDestSeg)
				continue;
			if (nStartSeg != nDestSeg) {
				int16_t nSide = segP->ConnectedSide (SEGMENT (nDestSeg));
				if (0 > nSide)
					continue;
				if (!((segP->IsPassable (nSide, NULL) & WID_PASSABLE_FLAG) || (AIDoorIsOpenable (objP, segP, nSide))))
					continue;

				CHitResult hitResult;
				CHitQuery hitQuery (0, &objP->info.position.vPos, &vNewPos, nStartSeg, objP->Index (), objP->info.xSize, objP->info.xSize);
				int32_t hitType = FindHitpoint (hitQuery, hitResult);
				if (hitType != HIT_NONE)
					continue;
				}
			vVecToTarget = vNewPos - vAvoidPos;
			CFixVector::Normalize (vVecToTarget); //CFixVector::Avg (vVecToTarget, vNewPos);
			break;
			}
		}
	}
}

// -----------------------------------------------------------------------------

void MoveTowardsPlayer (CObject *objP, CFixVector *vVecToTarget)
//	gameData.ai.target.vDir must be normalized, or close to it.
{
MoveAwayFromOtherRobots (objP, *vVecToTarget);
MoveTowardsVector (objP, vVecToTarget, 1);
}

// -----------------------------------------------------------------------------
//	I am ashamed of this: fastFlag == -1 means Normal slide about.  fastFlag = 0 means no evasion.
void MoveAroundPlayer (CObject *objP, CFixVector *vVecToTarget, int32_t fastFlag)
{
	tPhysicsInfo&	physInfo = objP->mType.physInfo;
	tRobotInfo&		robotInfo = ROBOTINFO (objP->info.nId);
	int32_t			nObject = objP->Index ();

if (fastFlag == 0)
	return;

int32_t dirChange = 48;
fix ft = gameData.time.xFrame;
int32_t count = 0;
if (ft < I2X (1)/32) {
	dirChange *= 8;
	count += 3;
	} 
else {
	while (ft < I2X (1)/4) {
		dirChange *= 2;
		ft *= 2;
		count++;
		}
	}

int32_t dir = (gameData.app.nFrameCount + (count+1) * (nObject*8 + nObject*4 + nObject)) & dirChange;
dir >>= (4 + count);

Assert ((dir >= 0) && (dir <= 3));
ft = gameData.time.xFrame * 32;
CFixVector vEvade;
switch (dir) {
	case 0:
		vEvade.v.coord.x = FixMul (gameData.ai.target.vDir.v.coord.z, ft);
		vEvade.v.coord.y = FixMul (gameData.ai.target.vDir.v.coord.y, ft);
		vEvade.v.coord.z = FixMul (-gameData.ai.target.vDir.v.coord.x, ft);
		break;
	case 1:
		vEvade.v.coord.x = FixMul (-gameData.ai.target.vDir.v.coord.z, ft);
		vEvade.v.coord.y = FixMul (gameData.ai.target.vDir.v.coord.y, ft);
		vEvade.v.coord.z = FixMul (gameData.ai.target.vDir.v.coord.x, ft);
		break;
	case 2:
		vEvade.v.coord.x = FixMul (-gameData.ai.target.vDir.v.coord.y, ft);
		vEvade.v.coord.y = FixMul (gameData.ai.target.vDir.v.coord.x, ft);
		vEvade.v.coord.z = FixMul (gameData.ai.target.vDir.v.coord.z, ft);
		break;
	case 3:
		vEvade.v.coord.x = FixMul (gameData.ai.target.vDir.v.coord.y, ft);
		vEvade.v.coord.y = FixMul (-gameData.ai.target.vDir.v.coord.x, ft);
		vEvade.v.coord.z = FixMul (gameData.ai.target.vDir.v.coord.z, ft);
		break;
	default:
		Error ("Function MoveAroundPlayer: Bad case.");
	}

//	Note: -1 means Normal circling about the player.  > 0 means fast evasion.
if (fastFlag > 0) {
	//	Only take evasive action if looking at player.
	//	Evasion speed is scaled by percentage of shield left so wounded robots evade less effectively.
	fix dot = CFixVector::Dot (gameData.ai.target.vDir, objP->info.position.mOrient.m.dir.f);
	if ((dot > robotInfo.fieldOfView [gameStates.app.nDifficultyLevel]) && !TARGETOBJ->Cloaked ()) {
		fix xDamageScale = (robotInfo.strength) ? FixDiv (objP->info.xShield, robotInfo.strength) : I2X (1);
		if (xDamageScale > I2X (1))
			xDamageScale = I2X (1);		//	Just in cased:\temp\dm_test.
		else if (xDamageScale < 0)
			xDamageScale = 0;			//	Just in cased:\temp\dm_test.
		vEvade *= (I2X (fastFlag) + xDamageScale);
		}
	}

physInfo.velocity += vEvade;
fix speed = physInfo.velocity.Mag();
if ((objP->Index () != 1) && (speed > robotInfo.xMaxSpeed [gameStates.app.nDifficultyLevel]))
	physInfo.velocity *= I2X (3) / 4;
}

// -----------------------------------------------------------------------------

void MoveAwayFromTarget (CObject *objP, CFixVector *vVecToTarget, int32_t attackType)
{
	tPhysicsInfo	physicsInfo = objP->mType.physInfo;
	fix				ft = gameData.time.xFrame * 16;

physicsInfo.velocity -= gameData.ai.target.vDir * ft;
if (attackType) {
	//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
	int32_t nObjRef = ((objP->Index ()) ^ ((gameData.app.nFrameCount + 3* (objP->Index ())) >> 5)) & 3;
	switch (nObjRef) {
		case 0:
			physicsInfo.velocity += objP->info.position.mOrient.m.dir.u * (gameData.time.xFrame << 5);
			break;
		case 1:
			physicsInfo.velocity += objP->info.position.mOrient.m.dir.u * (-gameData.time.xFrame << 5);
			break;
		case 2:
			physicsInfo.velocity += objP->info.position.mOrient.m.dir.r * (gameData.time.xFrame << 5);
			break;
		case 3:
			physicsInfo.velocity += objP->info.position.mOrient.m.dir.r * ( -gameData.time.xFrame << 5);
			break;
		default:
			Int3 ();	//	Impossible, bogus value on nObjRef, must be in 0d:\temp\dm_test3
		}
	}

if (physicsInfo.velocity.Mag () > ROBOTINFO (objP->info.nId).xMaxSpeed [gameStates.app.nDifficultyLevel]) {
	physicsInfo.velocity.v.coord.x = 3 * physicsInfo.velocity.v.coord.x / 4;
	physicsInfo.velocity.v.coord.y = 3 * physicsInfo.velocity.v.coord.y / 4;
	physicsInfo.velocity.v.coord.z = 3 * physicsInfo.velocity.v.coord.z / 4;
	}
}

// -----------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag bEvadeOnly is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_IDLING).
void AIMoveRelativeToTarget (CObject *objP, tAILocalInfo *ailP, fix xDistToTarget,
									  CFixVector *vVecToTarget, fix circleDistance, int32_t bEvadeOnly,
									  int32_t nTargetVisibility)
{
	CObject		*dObjP;
	tRobotInfo	*botInfoP = &ROBOTINFO (objP->info.nId);

	Assert (gameData.ai.nTargetVisibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((botInfoP->attackType == 0) && (objP->cType.aiInfo.nDangerLaser != -1)) {
if (objP->cType.aiInfo.nDangerLaser != -1) {
	dObjP = OBJECT (objP->cType.aiInfo.nDangerLaser);

	if ((dObjP->info.nType == OBJ_WEAPON) && (dObjP->info.nSignature == objP->cType.aiInfo.nDangerLaserSig)) {
		fix			dot, xDistToLaser, fieldOfView;
		CFixVector	vVecToLaser, fVecLaser;

		fieldOfView = ROBOTINFO (objP->info.nId).fieldOfView [gameStates.app.nDifficultyLevel];
		vVecToLaser = dObjP->info.position.vPos - objP->info.position.vPos;
		xDistToLaser = CFixVector::Normalize (vVecToLaser);
		dot = CFixVector::Dot (vVecToLaser, objP->info.position.mOrient.m.dir.f);

		if ((dot > fieldOfView) || (botInfoP->companion)) {
			fix			dotLaserRobot;
			CFixVector	vLaserToRobot;

			//	The laser is seen by the robot, see if it might hit the robot.
			//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
			if (dObjP->info.renderType == RT_POLYOBJ)
				fVecLaser = dObjP->info.position.mOrient.m.dir.f;
			else {		//	Not a polyobjP, get velocity and Normalize.
				fVecLaser = dObjP->mType.physInfo.velocity;	//dObjP->info.position.mOrient.m.v.f;
				CFixVector::Normalize (fVecLaser);
				}
			vLaserToRobot = objP->info.position.vPos - dObjP->info.position.vPos;
			CFixVector::Normalize (vLaserToRobot);
			dotLaserRobot = CFixVector::Dot (fVecLaser, vLaserToRobot);

			if ((dotLaserRobot > I2X (7) / 8) && (xDistToLaser < I2X (80))) {
				int32_t evadeSpeed = ROBOTINFO (objP->info.nId).evadeSpeed [gameStates.app.nDifficultyLevel];
				gameData.ai.bEvaded = 1;
				MoveAroundPlayer (objP, &gameData.ai.target.vDir, evadeSpeed);
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
		  (xDistToTarget >= I2X (30))))
		MoveTowardsPlayer (objP, &gameData.ai.target.vDir);
		//	1/4 of time, move around CPlayerData, 3/4 of time, move away from CPlayerData
	else if (RandShort () < 8192)
		MoveAroundPlayer (objP, &gameData.ai.target.vDir, -1);
	else
		MoveAwayFromTarget (objP, &gameData.ai.target.vDir, 1);
	}
else if (botInfoP->thief)
	MoveTowardsPlayer (objP, &gameData.ai.target.vDir);
else {
	int32_t	objval = ((objP->Index ()) & 0x0f) ^ 0x0a;

	//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
	if (botInfoP->kamikaze)
		MoveTowardsPlayer (objP, &gameData.ai.target.vDir);
	else if (xDistToTarget < circleDistance)
		MoveAwayFromTarget (objP, &gameData.ai.target.vDir, 0);
	else if ((xDistToTarget < (3+objval)*circleDistance/2) && (ailP->nextPrimaryFire > -I2X (1)))
		MoveAroundPlayer (objP, &gameData.ai.target.vDir, -1);
	else
		if ((-ailP->nextPrimaryFire > I2X (1) + (objval << 12)) && gameData.ai.nTargetVisibility)
			//	Usually move away, but sometimes move around player.
			if ((((gameData.time.xGame >> 18) & 0x0f) ^ objval) > 4)
				MoveAwayFromTarget (objP, &gameData.ai.target.vDir, 0);
			else
				MoveAroundPlayer (objP, &gameData.ai.target.vDir, -1);
		else
			MoveTowardsPlayer (objP, &gameData.ai.target.vDir);
	}
}

// -----------------------------------------------------------------------------

fix MoveObjectToLegalPoint (CObject *objP, CFixVector *vGoal)
{
	CFixVector	vGoalDir;
	fix			xDistToGoal;

vGoalDir = *vGoal - objP->info.position.vPos;
xDistToGoal = CFixVector::Normalize (vGoalDir);
vGoalDir *= Min (FixMul (ROBOTINFO (objP->Id ()).xMaxSpeed [gameStates.app.nDifficultyLevel], gameData.time.xFrame), xDistToGoal);
objP->info.position.vPos += vGoalDir;
return xDistToGoal;
}

// -----------------------------------------------------------------------------
//	Move the CObject objP to a spot in which it doesn't intersect a CWall.
//	It might mean moving it outside its current CSegment.
bool MoveObjectToLegalSpot (CObject *objP, int32_t bMoveToCenter)
{
	CFixVector	vSegCenter, vOrigPos = objP->info.position.vPos;
	CSegment*	segP = SEGMENT (objP->info.nSegment);

#if DBG
if (objP - OBJECTS == nDbgObj)
	BRP;
#endif
if (bMoveToCenter) {
	vSegCenter = SEGMENT (objP->info.nSegment)->Center ();
	MoveObjectToLegalPoint (objP, &vSegCenter);
	return !ObjectIntersectsWall (objP);
	}
else {
	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		if (segP->IsPassable ((int16_t) i, objP) & WID_PASSABLE_FLAG) {
			vSegCenter = SEGMENT (segP->m_children [i])->Center ();
			objP->info.position.vPos = vSegCenter;
			if (ObjectIntersectsWall (objP))
				objP->info.position.vPos = vOrigPos;
			else {
				objP->info.position.vPos = vOrigPos;
				MoveObjectToLegalPoint (objP, &vSegCenter);
				if (ObjectIntersectsWall (objP))
					objP->ApplyDamageToRobot (FixMul (objP->info.xShield / 5, gameData.time.xFrame), objP->Index ());
				int32_t nNewSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
				if ((nNewSeg != -1) && (nNewSeg != objP->info.nSegment)) {
					objP->RelinkToSeg (nNewSeg);
					return true;
					}
				}
			}
		}
	}

if (ROBOTINFO (objP->info.nId).bossFlag) {
	Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
	TeleportBoss (objP);
	return true;
	}
	else {
#if TRACE
		console.printf (CON_DBG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", objP->Index ());
#endif
		objP->ApplyDamageToRobot (FixMul (objP->info.xShield / 5, gameData.time.xFrame), objP->Index ());
	}
return false;
}

// -----------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsPoint (CObject *objP, CFixVector *vGoal, fix xMinDist)
{
	fix			xDistToGoal;
	CFixVector	vGoalDir;

#if DBG
if ((nDbgSeg >= 0) && (objP->info.nSegment == nDbgSeg))
	BRP;
if (objP->Index () == nDbgObj)
	BRP;
#endif
vGoalDir = *vGoal - objP->info.position.vPos;
xDistToGoal = CFixVector::Normalize (vGoalDir);
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
	//	Move one radius towards center.
	vGoalDir *= Min (FixMul (ROBOTINFO (objP->Id ()).xMaxSpeed [gameStates.app.nDifficultyLevel], gameData.time.xFrame), xDistToGoal - xMinDist);
	objP->info.position.vPos += vGoalDir;
	int32_t nNewSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
	if (nNewSeg == -1) {
		objP->info.position.vPos = *vGoal;
		MoveObjectToLegalSpot (objP, xMinDist > 0);
		}
	}
objP->Unstick ();
return xDistToGoal;
}

// -----------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsSegmentCenter (CObject *objP)
{
CFixVector vSegCenter = SEGMENT (objP->info.nSegment)->Center ();
return MoveTowardsPoint (objP, &vSegCenter, 0);
}

//int32_t	Buddy_got_stuck = 0;

//	-----------------------------------------------------------------------------
// eof

