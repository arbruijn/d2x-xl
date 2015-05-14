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

fix AITurnTowardsVector (CFixVector *vGoal, CObject *pObj, fix rate)
{
//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
if (!ROBOTINFO (pObj))
	return 0;
if (rate == 0)
	return 0;
if ((pObj->info.nId == BABY_SPIDER_ID) && (pObj->info.nType == OBJ_ROBOT)) {
	pObj->TurnTowardsVector (*vGoal, rate);
	return CFixVector::Dot (*vGoal, pObj->info.position.mOrient.m.dir.f);
	}

CFixVector fVecNew = *vGoal;
fix dot = CFixVector::Dot (*vGoal, pObj->info.position.mOrient.m.dir.f);

if (!IsMultiGame)
	dot = (fix) (dot / gameStates.gameplay.slowmo [0].fSpeed);
if (dot < (I2X (1) - gameData.time.xFrame / 2)) {
	fix mag, new_scale = FixDiv (gameData.time.xFrame * AI_TURN_SCALE, rate);
	if (!IsMultiGame)
		new_scale = (fix) (new_scale / gameStates.gameplay.slowmo [0].fSpeed);
	fVecNew *= new_scale;
	fVecNew += pObj->info.position.mOrient.m.dir.f;
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
	fix scale = FixDiv (2 * gameStates.gameplay.seismic.nMagnitude, ROBOTINFO (pObj)->mass);
	fVecNew += vRandom * scale;
	}
pObj->info.position.mOrient = CFixMatrix::CreateFR(fVecNew, pObj->info.position.mOrient.m.dir.r);
return dot;
}

// -----------------------------------------------------------------------------
//	vGoalVec must be normalized, or close to it.
//	if bDotBased set, then speed is based on direction of movement relative to heading
void MoveTowardsVector (CObject *pObj, CFixVector *vGoalVec, int32_t bDotBased)
{
	tPhysicsInfo&	physicsInfo = pObj->mType.physInfo;
	tRobotInfo		*pRobotInfo = ROBOTINFO (pObj);

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards CPlayerData as usual.
if (!pRobotInfo)
	return;
CFixVector v = physicsInfo.velocity;
CFixVector::Normalize (v);
fix dot = CFixVector::Dot (v, pObj->info.position.mOrient.m.dir.f);

if (pRobotInfo && pRobotInfo->thief)
	dot = (I2X (1) + dot) / 2;

if (bDotBased && (dot < I2X (3) / 4)) 
	//	This funny code is supposed to slow down the robot and move his velocity towards his direction
	//	more quickly than the general code
	physicsInfo.velocity += *vGoalVec * (gameData.time.xFrame * 32);
else 
	physicsInfo.velocity += *vGoalVec * ((gameData.time.xFrame * 64) * (5 * gameStates.app.nDifficultyLevel / 4));
fix speed = physicsInfo.velocity.Mag ();
fix xMaxSpeed = pObj->MaxSpeed ();
//	Green guy attacks twice as fast as he moves away.
if ((pRobotInfo->attackType == 1) || pRobotInfo->thief || pRobotInfo->kamikaze)
	xMaxSpeed *= 2;
if (speed > xMaxSpeed)
	physicsInfo.velocity *= I2X (3) / 4;
}

// -----------------------------------------------------------------------------

void MoveAwayFromOtherRobots (CObject *pObj, CFixVector& vVecToTarget)
{
	fix				xAvoidRad = 0;
	int16_t			nAvoidObjs = 0;
	int16_t			nStartSeg = pObj->info.nSegment;
	CFixVector		vPos = pObj->info.position.vPos;
	CFixVector		vAvoidPos;

vAvoidPos.SetZero ();
if ((pObj->info.nType == OBJ_ROBOT) && !pObj->IsGuideBot ()) {
	// move out from all other robots in same segment that are too close
	CObject* pAvoidObj;
	for (int16_t nObject = SEGMENT (nStartSeg)->m_objects; nObject != -1; nObject = pAvoidObj->info.nNextInSeg) {
		pAvoidObj = OBJECT (nObject);
		if ((pAvoidObj->info.nType != OBJ_ROBOT) || (pAvoidObj->info.nSignature >= pObj->info.nSignature))
			continue;	// comparing the sigs ensures that only one of two bots tested against each other will move, keeping them from bouncing around
		fix xDist = CFixVector::Dist (vPos, pAvoidObj->info.position.vPos);
		if (xDist >= (pObj->info.xSize + pAvoidObj->info.xSize))
			continue;
		xAvoidRad += (pObj->info.xSize + pAvoidObj->info.xSize);
		vAvoidPos += pAvoidObj->info.position.vPos;
		nAvoidObjs++;
		}
	if (nAvoidObjs) {
		xAvoidRad /= nAvoidObjs;
		vAvoidPos /= I2X (nAvoidObjs);
		CSegment* pSeg = SEGMENT (nStartSeg);
		for (nAvoidObjs = 5; nAvoidObjs; nAvoidObjs--) {
			CFixVector vNewPos = CFixVector::Random ();
			vNewPos *= pObj->info.xSize;
			vNewPos += vAvoidPos;
			int16_t nDestSeg = FindSegByPos (vNewPos, nStartSeg, 0, 0);
			if (0 > nDestSeg)
				continue;
			if (nStartSeg != nDestSeg) {
				int16_t nSide = pSeg->ConnectedSide (SEGMENT (nDestSeg));
				if (0 > nSide)
					continue;
				if (!((pSeg->IsPassable (nSide, NULL) & WID_PASSABLE_FLAG) || (AIDoorIsOpenable (pObj, pSeg, nSide))))
					continue;

				CHitResult hitResult;
				CHitQuery hitQuery (0, &pObj->info.position.vPos, &vNewPos, nStartSeg, pObj->Index (), pObj->info.xSize, pObj->info.xSize);
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

void MoveTowardsPlayer (CObject *pObj, CFixVector *vVecToTarget)
//	gameData.ai.target.vDir must be normalized, or close to it.
{
MoveAwayFromOtherRobots (pObj, *vVecToTarget);
MoveTowardsVector (pObj, vVecToTarget, 1);
}

// -----------------------------------------------------------------------------
//	I am ashamed of this: fastFlag == -1 means Normal slide about.  fastFlag = 0 means no evasion.
void MoveAroundPlayer (CObject *pObj, CFixVector *vVecToTarget, int32_t fastFlag)
{
	tPhysicsInfo&	physInfo = pObj->mType.physInfo;
	int32_t			nObject = pObj->Index ();
	tRobotInfo*		pRobotInfo = ROBOTINFO (pObj);

if (!pRobotInfo)
	return;
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
	fix dot = CFixVector::Dot (gameData.ai.target.vDir, pObj->info.position.mOrient.m.dir.f);
	if ((dot > pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel]) && !TARGETOBJ->Cloaked ()) {
		fix xDamageScale = (pRobotInfo->strength) ? FixDiv (pObj->info.xShield, pRobotInfo->strength) : I2X (1);
		if (xDamageScale > I2X (1))
			xDamageScale = I2X (1);		//	Just in cased:\temp\dm_test.
		else if (xDamageScale < 0)
			xDamageScale = 0;			//	Just in cased:\temp\dm_test.
		vEvade *= (I2X (fastFlag) + xDamageScale);
		}
	}

physInfo.velocity += vEvade;
fix speed = physInfo.velocity.Mag();
if ((pObj->Index () != 1) && (speed > pObj->MaxSpeed ()))
	physInfo.velocity *= I2X (3) / 4;
}

// -----------------------------------------------------------------------------

void MoveAwayFromTarget (CObject *pObj, CFixVector *vVecToTarget, int32_t attackType)
{
	tPhysicsInfo	physicsInfo = pObj->mType.physInfo;
	fix				ft = gameData.time.xFrame * 16;

physicsInfo.velocity -= gameData.ai.target.vDir * ft;
if (attackType) {
	//	Get value in 0d:\temp\dm_test3 to choose evasion direction.
	int32_t nObjRef = ((pObj->Index ()) ^ ((gameData.app.nFrameCount + 3* (pObj->Index ())) >> 5)) & 3;
	switch (nObjRef) {
		case 0:
			physicsInfo.velocity += pObj->info.position.mOrient.m.dir.u * (gameData.time.xFrame << 5);
			break;
		case 1:
			physicsInfo.velocity += pObj->info.position.mOrient.m.dir.u * (-gameData.time.xFrame << 5);
			break;
		case 2:
			physicsInfo.velocity += pObj->info.position.mOrient.m.dir.r * (gameData.time.xFrame << 5);
			break;
		case 3:
			physicsInfo.velocity += pObj->info.position.mOrient.m.dir.r * ( -gameData.time.xFrame << 5);
			break;
		default:
			Int3 ();	//	Impossible, bogus value on nObjRef, must be in 0d:\temp\dm_test3
		}
	}

if (physicsInfo.velocity.Mag () > pObj->MaxSpeed ()) {
	physicsInfo.velocity.v.coord.x = 3 * physicsInfo.velocity.v.coord.x / 4;
	physicsInfo.velocity.v.coord.y = 3 * physicsInfo.velocity.v.coord.y / 4;
	physicsInfo.velocity.v.coord.z = 3 * physicsInfo.velocity.v.coord.z / 4;
	}
}

// -----------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag bEvadeOnly is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_IDLING).
void AIMoveRelativeToTarget (CObject *pObj, tAILocalInfo *pLocalInfo, fix xDistToTarget,
									  CFixVector *vVecToTarget, fix circleDistance, int32_t bEvadeOnly,
									  int32_t nTargetVisibility)
{
	CObject		*pDangerObj = OBJECT (pObj->cType.aiInfo.nDangerLaser);
	tRobotInfo	*pRobotInfo = ROBOTINFO (pObj);

if (!pRobotInfo)
	return;
	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((pRobotInfo->attackType == 0) && (pObj->cType.aiInfo.nDangerLaser != -1)) {
if (pDangerObj && (pDangerObj->info.nType == OBJ_WEAPON) && (pDangerObj->info.nSignature == pObj->cType.aiInfo.nDangerLaserSig)) {
	fix			dot, xDistToLaser, fieldOfView;
	CFixVector	vVecToLaser, fVecLaser;

	fieldOfView = pRobotInfo->fieldOfView [gameStates.app.nDifficultyLevel];
	vVecToLaser = pDangerObj->info.position.vPos - pObj->info.position.vPos;
	xDistToLaser = CFixVector::Normalize (vVecToLaser);
	dot = CFixVector::Dot (vVecToLaser, pObj->info.position.mOrient.m.dir.f);

	if ((dot > fieldOfView) || (pRobotInfo->companion)) {
		fix			dotLaserRobot;
		CFixVector	vLaserToRobot;

		//	The laser is seen by the robot, see if it might hit the robot.
		//	Get the laser's direction.  If it's a polyobjP, it can be gotten cheaply from the orientation matrix.
		if (pDangerObj->info.renderType == RT_POLYOBJ)
			fVecLaser = pDangerObj->info.position.mOrient.m.dir.f;
		else {		//	Not a polyobjP, get velocity and Normalize.
			fVecLaser = pDangerObj->mType.physInfo.velocity;	//pDangerObj->info.position.mOrient.m.v.f;
			CFixVector::Normalize (fVecLaser);
			}
		vLaserToRobot = pObj->info.position.vPos - pDangerObj->info.position.vPos;
		CFixVector::Normalize (vLaserToRobot);
		dotLaserRobot = CFixVector::Dot (fVecLaser, vLaserToRobot);

		if ((dotLaserRobot > I2X (7) / 8) && (xDistToLaser < I2X (80))) {
			int32_t evadeSpeed = pRobotInfo->evadeSpeed [gameStates.app.nDifficultyLevel];
			gameData.ai.bEvaded = 1;
			MoveAroundPlayer (pObj, &gameData.ai.target.vDir, evadeSpeed);
			}
		}
	return;
	}

//	If only allowed to do evade code, then done.
//	Hmm, perhaps brilliant insight.  If want claw-nType guys to keep coming, don't return here after evasion.
if (!(pRobotInfo->attackType || pRobotInfo->thief) && bEvadeOnly)
	return;
//	If we fall out of above, then no CObject to be avoided.
pObj->cType.aiInfo.nDangerLaser = -1;
//	Green guy selects move around/towards/away based on firing time, not distance.
if (pRobotInfo->attackType == 1) {
	if (!gameStates.app.bPlayerIsDead &&
		 ((pLocalInfo->pNextrimaryFire <= pRobotInfo->primaryFiringWait [gameStates.app.nDifficultyLevel]/4) ||
		  (xDistToTarget >= I2X (30))))
		MoveTowardsPlayer (pObj, &gameData.ai.target.vDir);
		//	1/4 of time, move around CPlayerData, 3/4 of time, move away from CPlayerData
	else if (RandShort () < 8192)
		MoveAroundPlayer (pObj, &gameData.ai.target.vDir, -1);
	else
		MoveAwayFromTarget (pObj, &gameData.ai.target.vDir, 1);
	}
else if (pRobotInfo->thief)
	MoveTowardsPlayer (pObj, &gameData.ai.target.vDir);
else {
	int32_t	objval = ((pObj->Index ()) & 0x0f) ^ 0x0a;

	//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
	if (pRobotInfo->kamikaze)
		MoveTowardsPlayer (pObj, &gameData.ai.target.vDir);
	else if (xDistToTarget < circleDistance)
		MoveAwayFromTarget (pObj, &gameData.ai.target.vDir, 0);
	else if ((xDistToTarget < (3+objval)*circleDistance/2) && (pLocalInfo->pNextrimaryFire > -I2X (1)))
		MoveAroundPlayer (pObj, &gameData.ai.target.vDir, -1);
	else
		if ((-pLocalInfo->pNextrimaryFire > I2X (1) + (objval << 12)) && gameData.ai.nTargetVisibility)
			//	Usually move away, but sometimes move around player.
			if ((((gameData.time.xGame >> 18) & 0x0f) ^ objval) > 4)
				MoveAwayFromTarget (pObj, &gameData.ai.target.vDir, 0);
			else
				MoveAroundPlayer (pObj, &gameData.ai.target.vDir, -1);
		else
			MoveTowardsPlayer (pObj, &gameData.ai.target.vDir);
	}
}

// -----------------------------------------------------------------------------

fix MoveObjectToLegalPoint (CObject *pObj, CFixVector *vGoal)
{
	CFixVector	vGoalDir;
	fix			xDistToGoal;

vGoalDir = *vGoal - pObj->info.position.vPos;
xDistToGoal = CFixVector::Normalize (vGoalDir);
vGoalDir *= Min (FixMul (pObj->MaxSpeed (), gameData.time.xFrame), xDistToGoal);
pObj->info.position.vPos += vGoalDir;
return xDistToGoal;
}

// -----------------------------------------------------------------------------
//	Move the CObject pObj to a spot in which it doesn't intersect a CWall.
//	It might mean moving it outside its current CSegment.
bool MoveObjectToLegalSpot (CObject *pObj, int32_t bMoveToCenter)
{
	CFixVector	vSegCenter, vOrigPos = pObj->info.position.vPos;
	CSegment*	pSeg = SEGMENT (pObj->info.nSegment);

#if DBG
if (pObj - OBJECTS == nDbgObj)
	BRP;
#endif
if (bMoveToCenter) {
	vSegCenter = SEGMENT (pObj->info.nSegment)->Center ();
	MoveObjectToLegalPoint (pObj, &vSegCenter);
	return !ObjectIntersectsWall (pObj);
	}
else {
	for (int32_t i = 0; i < SEGMENT_SIDE_COUNT; i++) {
		if (pSeg->IsPassable ((int16_t) i, pObj) & WID_PASSABLE_FLAG) {
			vSegCenter = SEGMENT (pSeg->m_children [i])->Center ();
			pObj->info.position.vPos = vSegCenter;
			if (ObjectIntersectsWall (pObj))
				pObj->info.position.vPos = vOrigPos;
			else {
				pObj->info.position.vPos = vOrigPos;
				MoveObjectToLegalPoint (pObj, &vSegCenter);
				if (ObjectIntersectsWall (pObj))
					pObj->ApplyDamageToRobot (FixMul (pObj->info.xShield / 5, gameData.time.xFrame), pObj->Index ());
				int32_t nNewSeg = FindSegByPos (pObj->info.position.vPos, pObj->info.nSegment, 1, 0);
				if ((nNewSeg != -1) && (nNewSeg != pObj->info.nSegment)) {
					pObj->RelinkToSeg (nNewSeg);
					return true;
					}
				}
			}
		}
	}

if (pObj->IsBoss ()) {
	Int3 ();		//	Note: Boss is poking outside mine.  Will try to resolve.
	TeleportBoss (pObj);
	return true;
	}
	else {
#if TRACE
		console.printf (CON_DBG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", pObj->Index ());
#endif
		pObj->ApplyDamageToRobot (FixMul (pObj->info.xShield / 5, gameData.time.xFrame), pObj->Index ());
	}
return false;
}

// -----------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsPoint (CObject *pObj, CFixVector *vGoal, fix xMinDist)
{
	fix			xDistToGoal;
	CFixVector	vGoalDir;

#if DBG
if ((nDbgSeg >= 0) && (pObj->info.nSegment == nDbgSeg))
	BRP;
if (pObj->Index () == nDbgObj)
	BRP;
#endif
vGoalDir = *vGoal - pObj->info.position.vPos;
xDistToGoal = CFixVector::Normalize (vGoalDir);
if (xDistToGoal - pObj->info.xSize <= xMinDist) {
	//	Center is nearer than the distance we want to move, so move to center.
	if (!xMinDist) {
		pObj->info.position.vPos = *vGoal;
		if (ObjectIntersectsWall (pObj))
			MoveObjectToLegalSpot (pObj, xMinDist > 0);
		}
	xDistToGoal = 0;
	}
else {
	//	Move one radius towards center.
	vGoalDir *= Min (FixMul (pObj->MaxSpeed (), gameData.time.xFrame), xDistToGoal - xMinDist);
	pObj->info.position.vPos += vGoalDir;
	int32_t nNewSeg = FindSegByPos (pObj->info.position.vPos, pObj->info.nSegment, 1, 0);
	if (nNewSeg == -1) {
		pObj->info.position.vPos = *vGoal;
		MoveObjectToLegalSpot (pObj, xMinDist > 0);
		}
	}
pObj->Unstick ();
return xDistToGoal;
}

// -----------------------------------------------------------------------------
//	Move CObject one CObject radii from current position towards CSegment center.
//	If CSegment center is nearer than 2 radii, move it to center.
fix MoveTowardsSegmentCenter (CObject *pObj)
{
CFixVector vSegCenter = SEGMENT (pObj->info.nSegment)->Center ();
return MoveTowardsPoint (pObj, &vSegCenter, 0);
}

//int32_t	Buddy_got_stuck = 0;

//	-----------------------------------------------------------------------------
// eof

