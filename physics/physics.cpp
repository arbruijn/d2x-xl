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

#include "descent.h"
#include "joy.h"
#include "error.h"
#include "physics.h"
#include "key.h"
#include "collide.h"
#include "timer.h"
#include "network.h"
#include "gameseg.h"
#include "kconfig.h"
#include "automap.h"
#ifdef TACTILE
#	include "tactile.h"
#endif

//Global variables for physics system
//#define _DEBUG
#define UNSTICK_OBJS		0

#define ROLL_RATE 		0x2000
#define DAMP_ANG 			0x400                  //min angle to bank

#define TURNROLL_SCALE	 (0x4ec4/2)

#define MAX_OBJECT_VEL I2X (100)

#define BUMP_HACK	1	//if defined, bump CPlayerData when he gets stuck

int bFloorLeveling = 0;

//	-----------------------------------------------------------------------------------------------------------

#if DBG

#define CATCH_OBJ(_objP,_cond) {if (((_objP) == dbgObjP) && (_cond)) CatchDbgObj (_cond);}

int CatchDbgObj (int cond)
{
if (cond)
	return 1;
return 0;
}

#else

#define CATCH_OBJ(_objP,_cond)

#endif

//	-----------------------------------------------------------------------------------------------------------

void DoPhysicsAlignObject (CObject * objP)
{
	CFixVector	desiredUpVec;
	fixang		delta_ang, roll_ang;
	//CFixVector forvec = {0, 0, I2X (1)};
	CFixMatrix	temp_matrix;
	fix			d, largest_d=-I2X (1);
	int			i, nBestSide;

nBestSide = 0;
// bank CPlayerData according to CSegment orientation
//find CSide of CSegment that CPlayerData is most aligned with
for (i = 0; i < 6; i++) {
	d = CFixVector::Dot (SEGMENTS [objP->info.nSegment].m_sides [i].m_normals [0], objP->info.position.mOrient.UVec ());
	if (d > largest_d) {largest_d = d; nBestSide=i;}
	}
if (gameOpts->gameplay.nAutoLeveling == 1) {	 // new CPlayerData leveling code: use Normal of CSide closest to our up vec
	CSide* sideP = SEGMENTS [objP->info.nSegment].m_sides + nBestSide;
	if (sideP->FaceCount () == 2) {
		desiredUpVec = CFixVector::Avg (sideP->m_normals [0], sideP->m_normals [1]);
		CFixVector::Normalize (desiredUpVec);
		}
	else
		desiredUpVec = SEGMENTS [objP->info.nSegment].m_sides [nBestSide].m_normals [0];
	}
else if (gameOpts->gameplay.nAutoLeveling == 2)	// old way: used floor's Normal as upvec
	desiredUpVec = SEGMENTS [objP->info.nSegment].m_sides [3].m_normals [0];
else if (gameOpts->gameplay.nAutoLeveling == 3)	// mine's up vector
	desiredUpVec = (*PlayerSpawnOrient(gameData.multiplayer.nLocalPlayer)).UVec ();
else
	return;
if (labs (CFixVector::Dot (desiredUpVec, objP->info.position.mOrient.FVec ())) < I2X (1)/2) {
	fixang save_delta_ang;
	CAngleVector turnAngles;

	temp_matrix = CFixMatrix::CreateFU(objP->info.position.mOrient.FVec (), desiredUpVec);
//	temp_matrix = CFixMatrix::CreateFU(objP->info.position.mOrient.FVec (), &desiredUpVec, NULL);
	save_delta_ang =
	delta_ang = CFixVector::DeltaAngle(objP->info.position.mOrient.UVec (), temp_matrix.UVec (), &objP->info.position.mOrient.FVec ());
	delta_ang += objP->mType.physInfo.turnRoll;
	if (abs (delta_ang) > DAMP_ANG) {
		CFixMatrix mRotate, new_pm;

		roll_ang = (fixang) FixMul (gameData.physics.xTime, ROLL_RATE);
		if (abs (delta_ang) < roll_ang)
			roll_ang = delta_ang;
		else if (delta_ang < 0)
			roll_ang = -roll_ang;
		turnAngles [PA] = turnAngles [HA] = 0;
		turnAngles [BA] = roll_ang;
		mRotate = CFixMatrix::Create(turnAngles);
		new_pm = objP->info.position.mOrient * mRotate;
		objP->info.position.mOrient = new_pm;
		}
#if 0
	else
		gameOpts->gameplay.nAutoLeveling = 0;
#endif
	}
}

//	-----------------------------------------------------------------------------------------------------------

void CObject::SetTurnRoll (void)
{
//if (!gameStates.app.bD1Mission)
{
	fixang desired_bank = (fixang) -FixMul (mType.physInfo.rotVel [Y], TURNROLL_SCALE);
	if (mType.physInfo.turnRoll != desired_bank) {
		fixang delta_ang, max_roll;
		max_roll = (fixang) FixMul (ROLL_RATE, gameData.physics.xTime);
		delta_ang = desired_bank - mType.physInfo.turnRoll;
		if (labs (delta_ang) < max_roll)
			max_roll = delta_ang;
		else
			if (delta_ang < 0)
				max_roll = -max_roll;
		mType.physInfo.turnRoll += max_roll;
		}
	}
}

#if DBG
#define EXTRADBG 1		//no extra debug when NDEBUG is on
#endif

#ifdef EXTRADBG
CObject *debugObjP=NULL;
#endif

#if DBG
int	nTotalRetries=0, nTotalSims=0;
int	bDontMoveAIObjects=0;
#endif

#define FT (I2X (1)/64)

extern int bSimpleFVI;
//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
void CObject::DoPhysicsSimRot (void)
{
	CAngleVector	turnAngles;
	CFixMatrix		mRotate, mNewOrient;
	tPhysicsInfo*	pi;

if (gameData.physics.xTime <= 0)
	return;
pi = &mType.physInfo;
if (!(pi->rotVel [X] || pi->rotVel [Y] || pi->rotVel [Z] ||
		pi->rotThrust [X] || pi->rotThrust [Y] || pi->rotThrust [Z]))
	return;
if (mType.physInfo.drag) {
	CFixVector	accel;
	int			nTries = gameData.physics.xTime / FT;
	fix			r = gameData.physics.xTime % FT;
	fix			k = FixDiv (r, FT);
	fix			xDrag = (mType.physInfo.drag * 5) / 2;
	fix			xScale = I2X (1) - xDrag;

	if (this == gameData.objs.consoleP)
		xDrag = EGI_FLAG (nDrag, 0, 0, 0) * xDrag / 10;
	if (mType.physInfo.flags & PF_USES_THRUST) {
		accel = mType.physInfo.rotThrust * FixDiv (I2X (1), mType.physInfo.mass);
		while (nTries--) {
			mType.physInfo.rotVel += accel;
			mType.physInfo.rotVel *= xScale;
			}
		//do linear scale on remaining bit of time
		mType.physInfo.rotVel += accel * k;
		if (xDrag)
			mType.physInfo.rotVel *= (I2X (1) - FixMul (k, xDrag));
		}
	else if (xDrag && !(mType.physInfo.flags & PF_FREE_SPINNING)) {
		fix xTotalDrag = I2X (1);
		while (nTries--)
			xTotalDrag = FixMul (xTotalDrag, xScale);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, I2X (1) - FixMul (k, xDrag));
		mType.physInfo.rotVel *= xTotalDrag;
		}
	}
//now rotate CObject
//unrotate CObject for bank caused by turn
if (mType.physInfo.turnRoll) {
	CFixMatrix mOrient;

	turnAngles [PA] = turnAngles [HA] = 0;
	turnAngles [BA] = -mType.physInfo.turnRoll;
	mRotate = CFixMatrix::Create (turnAngles);
	mOrient = info.position.mOrient * mRotate;
	info.position.mOrient = mOrient;
}
turnAngles [PA] = (fixang) FixMul (mType.physInfo.rotVel [X], gameData.physics.xTime);
turnAngles [HA] = (fixang) FixMul (mType.physInfo.rotVel [Y], gameData.physics.xTime);
turnAngles [BA] = (fixang) FixMul (mType.physInfo.rotVel [Z], gameData.physics.xTime);
if (!IsMultiGame) {
	int i = (this != gameData.objs.consoleP) ? 0 : 1;
	if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
		turnAngles [PA] = (fixang) (turnAngles [PA] / gameStates.gameplay.slowmo [i].fSpeed);
		turnAngles [HA] = (fixang) (turnAngles [HA] / gameStates.gameplay.slowmo [i].fSpeed);
		turnAngles [BA] = (fixang) (turnAngles [BA] / gameStates.gameplay.slowmo [i].fSpeed);
		}
	}
mRotate = CFixMatrix::Create (turnAngles);
mNewOrient = info.position.mOrient * mRotate;
info.position.mOrient = mNewOrient;
if (mType.physInfo.flags & PF_TURNROLL)
	SetTurnRoll ();
//re-rotate object for bank caused by turn
if (mType.physInfo.turnRoll) {
	CFixMatrix m;

	turnAngles [PA] = turnAngles [HA] = 0;
	turnAngles [BA] = mType.physInfo.turnRoll;
	mRotate = CFixMatrix::Create(turnAngles);
	m = info.position.mOrient * mRotate;
	info.position.mOrient = m;
	}
info.position.mOrient.CheckAndFix ();
}

//	-----------------------------------------------------------------------------------------------------------

void DoBumpHack (CObject *objP)
{
	CFixVector vCenter, vBump;

//bump CPlayerData a little towards vCenter of CSegment to unstick
vCenter = SEGMENTS [objP->info.nSegment].Center ();
//don't bump CPlayerData towards center of reactor CSegment
CFixVector::NormalizedDir (vBump, vCenter, objP->info.position.vPos);
if (SEGMENTS [objP->info.nSegment].m_nType == SEGMENT_IS_CONTROLCEN)
	vBump.Neg();
objP->info.position.vPos += vBump * (objP->info.xSize / 5);
//if moving away from seg, might move out of seg, so update
if (SEGMENTS [objP->info.nSegment].m_nType == SEGMENT_IS_CONTROLCEN)
	UpdateObjectSeg (objP);
}

//	-----------------------------------------------------------------------------------------------------------

#if 1 //UNSTICK_OBJS

int BounceObject (CObject *objP, tFVIData	hi, float fOffs, fix *pxSideDists)
{
	fix	xSideDist, xSideDists [6];
	short	nSegment;

if (!pxSideDists) {
	SEGMENTS [hi.hit.nSideSegment].GetSideDists (objP->info.position.vPos, xSideDists, 1);
	pxSideDists = xSideDists;
	}
xSideDist = pxSideDists [hi.hit.nSide];
if (/*(0 <= xSideDist) && */
	 (xSideDist < objP->info.xSize - objP->info.xSize / 100)) {
#if 0
	objP->info.position.vPos = objP->info.vLastPos;
#else
#	if 1
	float r;
	xSideDist = objP->info.xSize - xSideDist;
	r = ((float) xSideDist / (float) objP->info.xSize) * X2F (objP->info.xSize);
#	endif
	objP->info.position.vPos [X] += (fix) ((float) hi.hit.vNormal [X] * fOffs);
	objP->info.position.vPos [Y] += (fix) ((float) hi.hit.vNormal [Y] * fOffs);
	objP->info.position.vPos [Z] += (fix) ((float) hi.hit.vNormal [Z] * fOffs);
#endif
	nSegment = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
	if ((nSegment < 0) || (nSegment > gameData.segs.nSegments)) {
		objP->info.position.vPos = objP->info.vLastPos;
		nSegment = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
		}
	if ((nSegment < 0) || (nSegment > gameData.segs.nSegments) || (nSegment == objP->info.nSegment))
		return 0;
	objP->RelinkToSeg (nSegment);
#if DBG
	if (objP->info.nType == OBJ_PLAYER)
		HUDMessage (0, "PENETRATING WALL (%d, %1.4f)", objP->info.xSize - pxSideDists [hi.hit.nSide], r);
#endif
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------------------------------------

void UnstickObject (CObject *objP)
{
	tFVIData			hi;
	tFVIQuery		fq;
	int				fviResult;

if ((objP->info.nType == OBJ_PLAYER) &&
	 (objP->info.nId == gameData.multiplayer.nLocalPlayer) &&
	 (gameStates.app.cheats.bPhysics == 0xBADA55))
	return;
fq.p0 = fq.p1 = &objP->info.position.vPos;
fq.startSeg = objP->info.nSegment;
fq.radP0 = 0;
fq.radP1 = objP->info.xSize;
fq.thisObjNum = objP->Index ();
fq.ignoreObjList = NULL;
fq.flags = 0;
fviResult = FindVectorIntersection (&fq, &hi);
if (fviResult == HIT_WALL)
#if 1
#	if 0
	DoBumpHack (objP);
#	else
	BounceObject (objP, hi, 0.1f, NULL);
#	endif
#else
	BounceObject (objP, hi, X2F (objP->info.xSize - VmVecDist (&objP->info.position.vPos, &hi.hit.vPoint)) /*0.25f*/, NULL);
#endif
}

#endif

//	-----------------------------------------------------------------------------------------------------------

void UpdateStats (CObject *objP, int nHitType)
{
	int	i;

if (!nHitType)
	return;
if (objP->info.nType != OBJ_WEAPON)
	return;
if (objP->cType.laserInfo.parent.nObject != OBJ_IDX (gameData.objs.consoleP))
	return;
switch (objP->info.nId) {
	case FUSION_ID:
		if (nHitType == HIT_OBJECT)
			return;
		if (objP->cType.laserInfo.nLastHitObj > 0)
			nHitType = HIT_OBJECT;
	case LASER_ID:
	case LASER_ID + 1:
	case LASER_ID + 2:
	case LASER_ID + 3:
	case VULCAN_ID:
	case SPREADFIRE_ID:
	case PLASMA_ID:
	case SUPERLASER_ID:
	case SUPERLASER_ID + 1:
	case GAUSS_ID:
	case HELIX_ID:
	case PHOENIX_ID:
		i = 0;
		break;
	case CONCUSSION_ID:
	case FLASHMSL_ID:
	case GUIDEDMSL_ID:
	case MERCURYMSL_ID:
		i = 1;
		break;
	default:
		return;
	}
if (nHitType == HIT_WALL) {
	gameData.stats.player [0].nMisses [i]++;
	gameData.stats.player [1].nMisses [i]++;
	}
else if (nHitType == HIT_OBJECT) {
	gameData.stats.player [0].nHits [i]++;
	gameData.stats.player [1].nHits [i]++;
	}
else
	return;
}

//	-----------------------------------------------------------------------------------------------------------

//Simulate a physics CObject for this frame

void CObject::DoPhysicsSim (void)
{
	short					nIgnoreObjs;
	int					iSeg, i;
	int					bRetry;
	int					fviResult = 0;
	CFixVector			vFrame;				//movement in this frame
	CFixVector			vNewPos, iPos;		//position after this frame
	int					nTries = 0;
	short					nObject = OBJ_IDX (this);
	short					nWallHitSeg, nWallHitSide;
	tFVIData				hi;
	tFVIQuery			fq;
	CFixVector			vSavePos;
	int					nSaveSeg;
	fix					xSimTime, xOldSimTime, xTimeScale;
	CFixVector			vStartPos;
	int					bGetPhysSegs, bObjStopped = 0;
	fix					xMovedTime;			//how long objected moved before hit something
	CFixVector			vSaveP0, vSaveP1;
	tPhysicsInfo		*pi;
	short					nOrigSegment = info.nSegment;
	int					nBadSeg = 0, bBounced = 0;
	tSpeedBoostData	sbd = gameData.objs.speedBoost [nObject];
	int					bDoSpeedBoost = sbd.bBoosted; // && (this == gameData.objs.consoleP);

Assert (info.nType != OBJ_NONE);
Assert (info.movementType == MT_PHYSICS);
#if DBG
if (Index () == nDbgObj)
	nDbgObj = nDbgObj;
if (bDontMoveAIObjects)
	if (info.controlType == CT_AI)
		return;
#endif
CATCH_OBJ (this, mType.physInfo.velocity [Y] == 0);
DoPhysicsSimRot ();
pi = &mType.physInfo;
gameData.physics.nSegments = 0;
nIgnoreObjs = 0;
xSimTime = gameData.physics.xTime;
bSimpleFVI = (info.nType != OBJ_PLAYER);
vStartPos = info.position.vPos;
if (pi->velocity.IsZero()) {
#	if UNSTICK_OBJS
	UnstickObject (this);
#	endif
	if (this == gameData.objs.consoleP)
		gameData.objs.speedBoost [nObject].bBoosted = sbd.bBoosted = 0;
	if (pi->thrust.IsZero())
		return;
	}

#if DBG
if (Index () == nDbgObj)
	nDbgObj = nDbgObj;
#endif

Assert (mType.physInfo.brakes == 0);		//brakes not used anymore?
//if uses thrust, cannot have zero xDrag
Assert (!(mType.physInfo.flags & PF_USES_THRUST) || mType.physInfo.drag);
//do thrust & xDrag
if (mType.physInfo.drag) {
	CFixVector accel, &vel = mType.physInfo.velocity;
	int		nTries = xSimTime / FT;
	fix		xDrag = mType.physInfo.drag;
	fix		r = xSimTime % FT;
	fix		k = FixDiv (r, FT);
	fix		d = I2X (1) - xDrag;
	fix		a;

	if (this == gameData.objs.consoleP)
		xDrag = EGI_FLAG (nDrag, 0, 0, 0) * xDrag / 10;
	if (mType.physInfo.flags & PF_USES_THRUST) {
		accel = mType.physInfo.thrust * FixDiv (I2X (1), mType.physInfo.mass);
		a = !accel.IsZero();
		if (bDoSpeedBoost && !(a || gameStates.input.bControlsSkipFrame))
			vel = sbd.vVel;
		else {
			while (nTries--) {
				if (a)
					vel += accel;
				vel *= d;
			}
			//do linear scale on remaining bit of time
			vel += accel * k;
			if (xDrag)
				vel *= (I2X (1) - FixMul (k, xDrag));
			if (bDoSpeedBoost) {
				if (vel [X] < sbd.vMinVel [X])
					vel [X] = sbd.vMinVel [X];
				else if (vel [X] > sbd.vMaxVel [X])
					vel [X] = sbd.vMaxVel [X];
				if (vel [Y] < sbd.vMinVel [Y])
					vel [Y] = sbd.vMinVel [Y];
				else if (vel [Y] > sbd.vMaxVel [Y])
					vel [Y] = sbd.vMaxVel [Y];
				if (vel [Z] < sbd.vMinVel [Z])
					vel [Z] = sbd.vMinVel [Z];
				else if (vel [Z] > sbd.vMaxVel [Z])
					vel [Z] = sbd.vMaxVel [Z];
				}
			}
		}
	else if (xDrag) {
		fix xTotalDrag = I2X (1);
		while (nTries--)
			xTotalDrag = FixMul (xTotalDrag, d);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, I2X (1)-FixMul (k, xDrag));
		mType.physInfo.velocity *= xTotalDrag;
		}
	}

//moveIt:

#if DBG
if ((nDbgSeg >= 0) && (info.nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif

if (extraGameInfo [IsMultiGame].bFluidPhysics) {
	if (SEGMENTS [info.nSegment].m_nType == SEGMENT_IS_WATER)
		xTimeScale = 75;
	else if (SEGMENTS [info.nSegment].m_nType == SEGMENT_IS_LAVA)
		xTimeScale = 66;
	else
		xTimeScale = 100;
	}
else
	xTimeScale = 100;
nTries = 0;
do {
	//Move the CObject
	float fScale = !gameStates.app.bNostalgia && (IS_MISSILE (this) && (info.nId != EARTHSHAKER_MEGA_ID) && (info.nId != ROBOT_SHAKER_MEGA_ID)) ?
						MissileSpeedScale (this) : 1;
	bRetry = 0;
	if (fScale < 1) {
		CFixVector vStartVel = StartVel ();
		CFixVector::Normalize (vStartVel);
		fix xDot = CFixVector::Dot (info.position.mOrient.FVec (), vStartVel);
		vFrame = mType.physInfo.velocity + StartVel ();
		vFrame *= F2X (fScale * fScale);
		vFrame += StartVel () * ((xDot > 0) ? -xDot : xDot);
		vFrame *= FixMulDiv (xSimTime, xTimeScale, 100 * (nBadSeg + 1));
		}
	else
		vFrame = mType.physInfo.velocity * FixMulDiv (xSimTime, xTimeScale, 100 * (nBadSeg + 1));
	if (!IsMultiGame) {
		int i = (this != gameData.objs.consoleP) ? 0 : 1;
		if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
			vFrame *= (fix) (I2X (1) / gameStates.gameplay.slowmo [i].fSpeed);
			}
		}
	if (vFrame.IsZero())
		break;

retryMove:
#if 1//ndef _DEBUG
	//	If retry count is getting large, then we are trying to do something stupid.
	if (++nTries > 3) {
		if (info.nType != OBJ_PLAYER)
			break;
		if (nTries > 8) {
			if (sbd.bBoosted)
				sbd.bBoosted = 0;
			break;
			}
		}
#endif
	vNewPos = info.position.vPos + vFrame;
#if 0
	iSeg = FindSegByPos (&vNewPos, info.nSegment, 1, 0);
	if (iSeg < 0) {
#if 0//def _DEBUG
		static int nBadSegs = 0;

		HUDMessage (0, "bad dest seg %d", nBadSegs++);
#endif
		nBadSeg++;
		bRetry = 1;
		continue;
		}
#endif
	gameData.physics.ignoreObjs [nIgnoreObjs] = -1;
	fq.p0 = &info.position.vPos;
	fq.startSeg = info.nSegment;
	fq.p1 = &vNewPos;
	fq.radP0 = fq.radP1 = info.xSize;
	fq.thisObjNum = nObject;
	fq.ignoreObjList = gameData.physics.ignoreObjs.Buffer ();
	fq.flags = FQ_CHECK_OBJS;

	if (info.nType == OBJ_WEAPON)
		fq.flags |= FQ_TRANSPOINT;
	if ((bGetPhysSegs = ((info.nType == OBJ_PLAYER) || (info.nType == OBJ_ROBOT))))
		fq.flags |= FQ_GET_SEGLIST;

	vSaveP0 = *fq.p0;
	vSaveP1 = *fq.p1;
	memset (&hi, 0, sizeof (hi));
	fviResult = FindVectorIntersection (&fq, &hi);
	UpdateStats (this, fviResult);
	vSavePos = info.position.vPos;			//save the CObject's position
	nSaveSeg = info.nSegment;
#if 0//def _DEBUG
	if (info.nType == OBJ_PLAYER)
		HUDMessage (0, "FVI: %d (%1.2f)", fviResult, X2F (VmVecMag (&mType.physInfo.velocity)));
#endif
	if (fviResult == HIT_BAD_P0) {
#if DBG
		static int nBadP0 = 0;
		HUDMessage (0, "BAD P0 %d", nBadP0++);
#endif
#if 0
		return;
#else
		memset (&hi, 0, sizeof (hi));
		fviResult = FindVectorIntersection (&fq, &hi);
		fq.startSeg = FindSegByPos (vNewPos, info.nSegment, 1, 0);
		if ((fq.startSeg < 0) || (fq.startSeg == info.nSegment)) {
			info.position.vPos = vSavePos;
			break;
			}
		fviResult = FindVectorIntersection (&fq, &hi);
		if (fviResult == HIT_BAD_P0) {
			info.position.vPos = vSavePos;
			break;
			}
#endif
		}
	else if (fviResult == HIT_WALL) {
		if (gameStates.render.bHaveSkyBox && (info.nType == OBJ_WEAPON) && (hi.hit.nSegment >= 0)) {
			if (SEGMENTS [hi.hit.nSegment].m_nType == SEGMENT_IS_SKYBOX) {
				short nConnSeg = SEGMENTS [hi.hit.nSegment].m_children [hi.hit.nSide];
				if ((nConnSeg < 0) && (info.xLifeLeft > I2X (1))) {	//leaving the mine
					info.xLifeLeft = 0;
					info.nFlags |= OF_SHOULD_BE_DEAD;
					}
				fviResult = HIT_NONE;
				}
			else if (SEGMENTS [hi.hit.nSideSegment].CheckForTranspPixel (hi.hit.vPoint, hi.hit.nSide, hi.hit.nFace)) {
				short nNewSeg = FindSegByPos (vNewPos, gameData.segs.skybox [0], 1, 1);
				if ((nNewSeg >= 0) && (SEGMENTS [nNewSeg].m_nType == SEGMENT_IS_SKYBOX)) {
					hi.hit.nSegment = nNewSeg;
					fviResult = HIT_NONE;
					}
				}
			}
		}
	//	Matt: Mike's hack.
	else if (fviResult == HIT_OBJECT) {
		CObject	*hitObjP = OBJECTS + hi.hit.nObject;

		if (ObjIsPlayerMine (hitObjP))
			nTries--;
#if 0//def _DEBUG
		fviResult = FindVectorIntersection (&fq, &hi);
#endif
		}

	if (bGetPhysSegs) {
		if (gameData.physics.nSegments && (gameData.physics.segments [gameData.physics.nSegments-1] == hi.segList [0]))
			gameData.physics.nSegments--;
#if 1 //!DBG
		i = MAX_FVI_SEGS - gameData.physics.nSegments - 1;
		if (i > 0) {
			if (i > hi.nSegments)
				i = hi.nSegments;
#	if DBG
			if (i <= 0)
				FindVectorIntersection (&fq, &hi);
#	endif
			if (i > 0)
				memcpy (gameData.physics.segments + gameData.physics.nSegments, hi.segList, i * sizeof (gameData.physics.segments [0]));
			gameData.physics.nSegments += i;
			}
#else
		for (i = 0; (i < hi.nSegments) && (gameData.physics.nSegments < MAX_FVI_SEGS-1); ) {
			if (hi.segList [i] > gameData.segs.nLastSegment)
				PrintLog ("Invalid segment in segment list #1\n");
			gameData.physics.segments [gameData.physics.nSegments++] = hi.segList [i++];
			}
#endif
		}
	iPos = hi.hit.vPoint;
	iSeg = hi.hit.nSegment;
	nWallHitSide = hi.hit.nSide;
	nWallHitSeg = hi.hit.nSideSegment;
	if (iSeg == -1) {		//some sort of horrible error
		if (info.nType == OBJ_WEAPON)
			Die ();
		break;
		}
	Assert ((fviResult != HIT_WALL) || ((nWallHitSeg > -1) && (nWallHitSeg <= gameData.segs.nLastSegment)));
	// update CObject's position and CSegment number
	info.position.vPos = iPos;
	if (iSeg != info.nSegment)
		OBJECTS [nObject].RelinkToSeg (iSeg);
	//if start point not in CSegment, move CObject to center of CSegment
	if (SEGMENTS [info.nSegment].Masks (info.position.vPos, 0).m_center) {	//object stuck
		int n = FindSegment ();
		if (n == -1) {
			if (bGetPhysSegs)
				n = FindSegByPos (info.vLastPos, info.nSegment, 1, 0);
			if (n == -1) {
				info.position.vPos = info.vLastPos;
				OBJECTS [nObject].RelinkToSeg (info.nSegment);
				}
			else {
				CFixVector vCenter;
				vCenter = SEGMENTS [info.nSegment].Center ();
				vCenter -= info.position.vPos;
				if (vCenter.Mag() > I2X (1)) {
					CFixVector::Normalize(vCenter);
					vCenter /= 10;
					}
				info.position.vPos -= vCenter;
				}
			if (info.nType == OBJ_WEAPON) {
				Die ();
				return;
				}
			}
		//return;
		}

	//calulate new sim time
 {
		//CFixVector vMoved;
		CFixVector vMoveNormal;
		fix attemptedDist, actualDist;

		xOldSimTime = xSimTime;
		actualDist = CFixVector::NormalizedDir (vMoveNormal, info.position.vPos, vSavePos);
		if ((fviResult == HIT_WALL) && (CFixVector::Dot (vMoveNormal, vFrame) < 0)) {		//moved backwards
			//don't change position or xSimTime
			info.position.vPos = vSavePos;
			//iSeg = info.nSegment;		//don't change CSegment
			if (nSaveSeg != iSeg)
				OBJECTS [nObject].RelinkToSeg (nSaveSeg);
			if (bDoSpeedBoost) {
				info.position.vPos = vStartPos;
				SetSpeedBoostVelocity (nObject, -1, -1, -1, -1, -1, &vStartPos, &sbd.vDest, 0);
				vFrame = sbd.vVel * xSimTime;
				goto retryMove;
				}
			xMovedTime = 0;
			}
		else {
			attemptedDist = vFrame.Mag();
			xSimTime = FixMulDiv (xSimTime, attemptedDist - actualDist, attemptedDist);
			xMovedTime = xOldSimTime - xSimTime;
			if ((xSimTime < 0) || (xSimTime > xOldSimTime)) {
				xSimTime = xOldSimTime;
				xMovedTime = 0;
				}
			}
		}

	if (fviResult == HIT_WALL) {
		CFixVector	vMoved;
		fix			xHitSpeed, xWallPart;
		// Find hit speed

#if 0//def _DEBUG
		if (info.nType == OBJ_PLAYER)
			HUDMessage (0, "WALL CONTACT");
		fviResult = FindVectorIntersection (&fq, &hi);
#endif
		vMoved = info.position.vPos - vSavePos;
		xWallPart = CFixVector::Dot (vMoved, hi.hit.vNormal) / gameData.collisions.hitData.nNormals;
		if (xWallPart && (xMovedTime > 0) && ((xHitSpeed = -FixDiv (xWallPart, xMovedTime)) > 0)) {
			CollideObjectAndWall (xHitSpeed, nWallHitSeg, nWallHitSide, hi.hit.vPoint);
#if 0//def _DEBUG
			if (info.nType == OBJ_PLAYER)
				HUDMessage (0, "BUMP %1.2f (%d,%d)!", X2F (xHitSpeed), nWallHitSeg, nWallHitSide);
#endif
			}
		else {
#if 0//def _DEBUG
			if (info.nType == OBJ_PLAYER)
				HUDMessage (0, "SCREEEEEEEEEECH");
#endif
			ScrapeOnWall (nWallHitSeg, nWallHitSide, hi.hit.vPoint);
			}
		Assert (nWallHitSeg > -1);
		Assert (nWallHitSide > -1);
#if UNSTICK_OBJS == 2
	 {
		fix	xSideDists [6];
		SEGMENTS [nWallHitSeg].GetSideDists (&info.position.vPos, xSideDists);
		bRetry = BounceObject (this, hi, 0.1f, xSideDists);
		}
#else
		bRetry = 0;
#endif
		if (!(info.nFlags & OF_SHOULD_BE_DEAD) && (info.nType != OBJ_DEBRIS)) {
			int bForceFieldBounce;		//bounce off a forcefield

			///Assert (gameStates.app.cheats.bBouncingWeapons || ((mType.physInfo.flags & (PF_STICK | PF_BOUNCE)) != (PF_STICK | PF_BOUNCE)));	//can't be bounce and stick
			bForceFieldBounce = (gameData.pig.tex.tMapInfoP [SEGMENTS [nWallHitSeg].m_sides [nWallHitSide].m_nBaseTex].flags & TMI_FORCE_FIELD);
			if (!bForceFieldBounce && (mType.physInfo.flags & PF_STICK)) {		//stop moving
				AddStuckObject (this, nWallHitSeg, nWallHitSide);
				mType.physInfo.velocity.SetZero ();
				bObjStopped = 1;
				bRetry = 0;
				}
			else {				// Slide CObject along CWall
				int bCheckVel = 0;
				//We're constrained by a wall, so subtract wall part from velocity vector

				xWallPart = CFixVector::Dot (hi.hit.vNormal, mType.physInfo.velocity);
				if (bForceFieldBounce || (mType.physInfo.flags & PF_BOUNCE)) {		//bounce off CWall
					xWallPart *= 2;	//Subtract out wall part twice to achieve bounce
					if (bForceFieldBounce) {
						bCheckVel = 1;				//check for max velocity
						if (info.nType == OBJ_PLAYER)
							xWallPart *= 2;		//CPlayerData bounce twice as much
						}
					if ((mType.physInfo.flags & (PF_BOUNCE | PF_BOUNCES_TWICE)) == (PF_BOUNCE | PF_BOUNCES_TWICE)) {
						//Assert (mType.physInfo.flags & PF_BOUNCE);
						if (mType.physInfo.flags & PF_HAS_BOUNCED)
							mType.physInfo.flags &= ~(PF_BOUNCE | PF_HAS_BOUNCED | PF_BOUNCES_TWICE);
						else
							mType.physInfo.flags |= PF_HAS_BOUNCED;
						}
					bBounced = 1;		//this CObject bBounced
					}
				mType.physInfo.velocity += hi.hit.vNormal * (-xWallPart);
				if (bCheckVel) {
					fix vel = mType.physInfo.velocity.Mag();
					if (vel > MAX_OBJECT_VEL)
						mType.physInfo.velocity *= (FixDiv (MAX_OBJECT_VEL, vel));
					}
				if (bBounced && (info.nType == OBJ_WEAPON))
					info.position.mOrient = CFixMatrix::CreateFU(mType.physInfo.velocity, info.position.mOrient.UVec ());
					//info.position.mOrient = CFixMatrix::CreateFU(mType.physInfo.velocity, &info.position.mOrient.UVec (), NULL);
				bRetry = 1;
				}
			}
		}
	else if (fviResult == HIT_OBJECT) {
		CFixVector	vOldVel;
		CFixVector	*ppos0, *ppos1, vHitPos;
		fix			size0, size1;
		// Mark the hit CObject so that on a retry the fvi code
		// ignores this CObject.
		//if (bSpeedBoost && (this == gameData.objs.consoleP))
		//	break;
#if DBG
		tFVIData				_hi;
		memset (&_hi, 0, sizeof (_hi));
		FindVectorIntersection (&fq, &_hi);
#endif
		Assert (hi.hit.nObject != -1);
		ppos0 = &OBJECTS [hi.hit.nObject].info.position.vPos;
		ppos1 = &info.position.vPos;
		size0 = OBJECTS [hi.hit.nObject].info.xSize;
		size1 = info.xSize;
		//	Calculate the hit point between the two objects.
		Assert (size0 + size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
#if 0
		if (EGI_FLAG (nHitboxes, 0, 0, 0))
			vHitPos = hi.hit.vPoint;
		else
#endif
		 {
			vHitPos = *ppos1 - *ppos0;
			vHitPos = *ppos0 + vHitPos * FixDiv(size0, size0 + size1);
			}
		vOldVel = mType.physInfo.velocity;
		//if (!(SPECTATOR (this) || SPECTATOR (OBJECTS + hi.hit.nObject)))
			CollideTwoObjects (this, OBJECTS + hi.hit.nObject, vHitPos);
		if (sbd.bBoosted && (this == gameData.objs.consoleP))
			mType.physInfo.velocity = vOldVel;
		// Let object continue its movement
		if (!(info.nFlags & OF_SHOULD_BE_DEAD)) {
			if ((mType.physInfo.flags & PF_PERSISTENT) ||
				 (vOldVel [X] == mType.physInfo.velocity [X] &&
				  vOldVel [Y] == mType.physInfo.velocity [Y]) &&
				  vOldVel [Z] == mType.physInfo.velocity [Z]) {
				if (OBJECTS [hi.hit.nObject].info.nType == OBJ_POWERUP)
					nTries--;
				gameData.physics.ignoreObjs [nIgnoreObjs++] = hi.hit.nObject;
				bRetry = 1;
				}
#if DBG
			else
				bRetry = bRetry;
#endif
			}
		}
	else if (fviResult == HIT_NONE) {
#ifdef TACTILE
		if (TactileStick && (this == gameData.objs.consoleP) && !(FrameCount & 15))
			Tactile_Xvibrate_clear ();
#endif
		}
#if DBG
	else if (fviResult == HIT_BAD_P0) {
		Int3 ();		// Unexpected collision nType: start point not in specified CSegment.
#if TRACE
		console.printf (CON_DBG, "Warning: Bad p0 in physics!!!\n");
#endif
		}
	else {
		// Unknown collision nType returned from FindVectorIntersection!!
		Int3 ();
		break;
		}
#endif
	} while (bRetry);
	//	Pass retry nTries info to AI.
if (info.controlType == CT_AI) {
	Assert (nObject >= 0);
	if (nTries > 0) {
		gameData.ai.localInfo [nObject].nRetryCount = nTries - 1;
#if DBG
		nTotalRetries += nTries - 1;
		nTotalSims++;
#endif
		}
	}
	// If the ship has thrust, but the velocity is zero or the current position equals the start position
	// stored when entering this function, it has been stopped forcefully by something, so bounce it back to
	// avoid that the ship gets driven into the obstacle (most likely a wall, as that doesn't give in ;)
	if (((fviResult == HIT_WALL) || (fviResult == HIT_BAD_P0)) &&
		 !(sbd.bBoosted || bObjStopped || bBounced)) {	//Set velocity from actual movement
		CFixVector vMoved;
		fix s = FixMulDiv (FixDiv (I2X (1), gameData.physics.xTime), xTimeScale, 100);

		vMoved = info.position.vPos - vStartPos;
		s = vMoved.Mag();
		vMoved *= (FixMulDiv (FixDiv (I2X (1), gameData.physics.xTime), xTimeScale, 100));
#if 1
		if (!bDoSpeedBoost)
			mType.physInfo.velocity = vMoved;
#endif
#ifdef BUMP_HACK
		if ((this == gameData.objs.consoleP) &&
			 vMoved.IsZero() &&
			 !mType.physInfo.thrust.IsZero()) {
			DoBumpHack (this);
			}
#endif
		}

	if (mType.physInfo.flags & PF_LEVELLING)
		DoPhysicsAlignObject (this);
	//hack to keep CPlayerData from going through closed doors
	if (((info.nType == OBJ_PLAYER) || (info.nType == OBJ_ROBOT)) && (info.nSegment != nOrigSegment) &&
		 (gameStates.app.cheats.bPhysics != 0xBADA55)) {
		int nSide = SEGMENTS [info.nSegment].ConnectedSide (SEGMENTS + nOrigSegment);
		if (nSide != -1) {
			if (!(SEGMENTS [nOrigSegment].IsDoorWay (nSide, (info.nType == OBJ_PLAYER) ? this : NULL) & WID_FLY_FLAG)) {
				CSide *sideP;
				fix	dist;

				//bump CObject back
				sideP = SEGMENTS [nOrigSegment].m_sides + nSide;
				if (nOrigSegment == -1)
					Error ("nOrigSegment == -1 in physics");
				dist = vStartPos.DistToPlane (sideP->m_normals [0], gameData.segs.vertices [sideP->m_nMinVertex [0]]);
				info.position.vPos = vStartPos + sideP->m_normals [0] * (info.xSize - dist);
				UpdateObjectSeg (this);
				}
			}
		}

//if end point not in CSegment, move CObject to last pos, or CSegment center
if (SEGMENTS [info.nSegment].Masks (info.position.vPos, 0).m_center) {
	if (FindSegment () == -1) {
		int n;

		if (((info.nType == OBJ_PLAYER) || (info.nType == OBJ_ROBOT)) && 
			 (n = FindSegByPos (info.vLastPos, info.nSegment, 1, 0)) != -1) {
			info.position.vPos = info.vLastPos;
			OBJECTS [nObject].RelinkToSeg (n);
			}
		else {
			info.position.vPos = SEGMENTS [info.nSegment].Center ();
			info.position.vPos [X] += nObject;
			}
		if (info.nType == OBJ_WEAPON)
			Die ();
		}
	}
CATCH_OBJ (this, mType.physInfo.velocity [Y] == 0);
#if UNSTICK_OBJS
UnstickObject (this);
#endif
}

//	----------------------------------------------------------------
//Applies an instantaneous force on an CObject, resulting in an instantaneous
//change in velocity.

void CObject::ApplyForce (CFixVector vForce)
{
	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass,
	//	in collision with crazy reactor robot thing on d2levf-s.
if (mType.physInfo.mass == 0)
	return;
if (info.movementType != MT_PHYSICS)
	return;
if ((automap.m_bDisplay && (this == gameData.objs.consoleP)) || SPECTATOR (this))
	return;
#ifdef TACTILE
  if (TactileStick && (obj == OBJECTS + LOCALPLAYER.nObject))
	Tactile_apply_force (vForce, &info.position.mOrient);
#endif
//Add in acceleration due to force
if (!gameData.objs.speedBoost [OBJ_IDX (this)].bBoosted || (this != gameData.objs.consoleP))
	mType.physInfo.velocity += vForce * FixDiv (I2X (1), mType.physInfo.mass);
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void PhysicsSetRotVelAndSaturate (fix *dest, fix delta)
{
if ((delta ^ *dest) < 0) {
	if (abs (delta) < I2X (1)/8) {
		*dest = delta/4;
		}
	else
		*dest = delta;
		}
else {
	*dest = delta;
	}
}

//	------------------------------------------------------------------------------------------------------
//	Note: This is the old AITurnTowardsVector code.
//	ApplyRotForce used to call AITurnTowardsVector until I fixed it, which broke ApplyRotForce.
void CObject::TurnTowardsVector (CFixVector vGoal, fix rate)
{
	CAngleVector	dest_angles, cur_angles;
	fix				delta_p, delta_h;
	CFixVector&		pvRotVel = mType.physInfo.rotVel;

// Make this CObject turn towards the vGoal.  Changes orientation, doesn't change direction of movement.
// If no one moves, will be facing vGoal in 1 second.

//	Detect null vector.
if (automap.m_bDisplay && (this == gameData.objs.consoleP))
	return;
if (vGoal.IsZero())
	return;
//	Make morph OBJECTS turn more slowly.
if (info.controlType == CT_MORPH)
	rate *= 2;

dest_angles = vGoal.ToAnglesVec();
cur_angles = info.position.mOrient.FVec ().ToAnglesVec();
delta_p = (dest_angles [PA] - cur_angles [PA]);
delta_h = (dest_angles [HA] - cur_angles [HA]);
if (delta_p > I2X (1)/2)
	delta_p = dest_angles [PA] - cur_angles [PA] - I2X (1);
if (delta_p < -I2X (1)/2)
	delta_p = dest_angles [PA] - cur_angles [PA] + I2X (1);
if (delta_h > I2X (1)/2)
	delta_h = dest_angles [HA] - cur_angles [HA] - I2X (1);
if (delta_h < -I2X (1)/2)
	delta_h = dest_angles [HA] - cur_angles [HA] + I2X (1);
delta_p = FixDiv (delta_p, rate);
delta_h = FixDiv (delta_h, rate);
if (abs (delta_p) < I2X (1)/16) delta_p *= 4;
if (abs (delta_h) < I2X (1)/16) delta_h *= 4;
if (!IsMultiGame) {
	int i = (this != gameData.objs.consoleP) ? 0 : 1;
	if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
		delta_p = (fix) (delta_p / gameStates.gameplay.slowmo [i].fSpeed);
		delta_h = (fix) (delta_h / gameStates.gameplay.slowmo [i].fSpeed);
		}
	}
PhysicsSetRotVelAndSaturate(&pvRotVel [X], delta_p);
PhysicsSetRotVelAndSaturate(&pvRotVel [Y], delta_h);
pvRotVel [Z] = 0;
}

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an CObject, resulting in an instantaneous
//	change in orientation.
void CObject::ApplyRotForce (CFixVector vForce)
{
	fix	xRate, xMag;

if (info.movementType != MT_PHYSICS)
	return;
xMag = vForce.Mag() / 8;
if (xMag < I2X (1) / 256)
	xRate = I2X (4);
else if (xMag < mType.physInfo.mass >> 14)
	xRate = I2X (4);
else {
	xRate = FixDiv (mType.physInfo.mass, xMag);
	if (info.nType == OBJ_ROBOT) {
		if (xRate < I2X (1)/4)
			xRate = I2X (1)/4;
		//	Changed by mk, 10/24/95, claw guys should not slow down when attacking!
		if (!(ROBOTINFO (info.nId).thief || ROBOTINFO (info.nId).attackType)) {
			if (cType.aiInfo.SKIP_AI_COUNT * gameData.physics.xTime < I2X (3)/4) {
				fix	tval = FixDiv (I2X (1), 8 * gameData.physics.xTime);
				int	addval = X2I (tval);
				if ((d_rand () * 2) < (tval & 0xffff))
					addval++;
				cType.aiInfo.SKIP_AI_COUNT += addval;
				}
			}
		}
	else {
		if (xRate < I2X (1)/2)
			xRate = I2X (1)/2;
		}
	}
//	Turn amount inversely proportional to mass.  Third parameter is seconds to do 360 turn.
TurnTowardsVector (vForce, xRate);
}


//this routine will set the thrust for an CObject to a value that will
// (hopefully) maintain the CObject's current velocity
void CObject::SetThrustFromVelocity (void)
{
fix k = FixMulDiv (mType.physInfo.mass, mType.physInfo.drag, (I2X (1) - mType.physInfo.drag));
mType.physInfo.thrust = mType.physInfo.velocity * k;

}


