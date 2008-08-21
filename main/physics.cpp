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

#include "inferno.h"
#include "joy.h"
#include "error.h"
#include "physics.h"
#include "key.h"
#include "collide.h"
#include "timer.h"
#include "network.h"
#include "gameseg.h"
#include "kconfig.h"
#ifdef TACTILE
#	include "tactile.h"
#endif

//Global variables for physics system
//#define _DEBUG
#define UNSTICK_OBJS		0

#define ROLL_RATE 		0x2000
#define DAMP_ANG 			0x400                  //min angle to bank

#define TURNROLL_SCALE	 (0x4ec4/2)

#define MAX_OBJECT_VEL i2f (100)

#define BUMP_HACK	1	//if defined, bump tPlayer when he gets stuck

int bFloorLeveling = 0;

//	-----------------------------------------------------------------------------------------------------------

#ifdef _DEBUG

#define CATCH_OBJ(_objP,_cond)	{if (((_objP) == dbgObjP) && (_cond)) CatchDbgObj (_cond);}

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

void DoPhysicsAlignObject (tObject * objP)
{
	vmsVector	desiredUpVec;
	fixang		delta_ang, roll_ang;
	//vmsVector forvec = {0, 0, f1_0};
	vmsMatrix	temp_matrix;
	fix			d, largest_d=-f1_0;
	int			i, best_side;

best_side = 0;
// bank tPlayer according to tSegment orientation
//find tSide of tSegment that tPlayer is most aligned with
for (i = 0; i < 6; i++) {
	d = vmsVector::dot(gameData.segs.segments [objP->nSegment].sides [i].normals[0], objP->position.mOrient[UVEC]);
	if (d > largest_d) {largest_d = d; best_side=i;}
	}
if (gameOpts->gameplay.nAutoLeveling == 1) {	 // new tPlayer leveling code: use normal of tSide closest to our up vec
	if (GetNumFaces (&gameData.segs.segments [objP->nSegment].sides [best_side])==2) {
		tSide *s = &gameData.segs.segments [objP->nSegment].sides [best_side];
		desiredUpVec[X] = (s->normals[0][X] + s->normals[1][X]) / 2;
		desiredUpVec[Y] = (s->normals[0][Y] + s->normals[1][Y]) / 2;
		desiredUpVec[Z] = (s->normals[0][Z] + s->normals[1][Z]) / 2;
		vmsVector::normalize(desiredUpVec);
		}
	else
		desiredUpVec = gameData.segs.segments [objP->nSegment].sides [best_side].normals [0];
	}
else if (gameOpts->gameplay.nAutoLeveling == 2)	// old way: used floor's normal as upvec
	desiredUpVec = gameData.segs.segments [objP->nSegment].sides [3].normals [0];
else if (gameOpts->gameplay.nAutoLeveling == 3)	// mine's up vector
	desiredUpVec = (*PlayerSpawnOrient(gameData.multiplayer.nLocalPlayer))[UVEC];
else
	return;
if (labs (vmsVector::dot(desiredUpVec, objP->position.mOrient[FVEC])) < f1_0/2) {
	fixang save_delta_ang;
	vmsAngVec turnAngles;

	temp_matrix = vmsMatrix::CreateFU(objP->position.mOrient[FVEC], desiredUpVec);
//	temp_matrix = vmsMatrix::CreateFU(objP->position.mOrient[FVEC], &desiredUpVec, NULL);
	save_delta_ang =
	delta_ang = vmsVector::deltaAngle(objP->position.mOrient[UVEC], temp_matrix[UVEC], &objP->position.mOrient[FVEC]);
	delta_ang += objP->mType.physInfo.turnRoll;
	if (abs (delta_ang) > DAMP_ANG) {
		vmsMatrix mRotate, new_pm;

		roll_ang = (fixang) FixMul (gameData.physics.xTime, ROLL_RATE);
		if (abs (delta_ang) < roll_ang)
			roll_ang = delta_ang;
		else if (delta_ang < 0)
			roll_ang = -roll_ang;
		turnAngles[PA] = turnAngles[HA] = 0;
		turnAngles[BA] = roll_ang;
		mRotate = vmsMatrix::Create(turnAngles);
		// TODO MM
		new_pm = objP->position.mOrient * mRotate;
		objP->position.mOrient = new_pm;
		}
#if 0
	else
		gameOpts->gameplay.nAutoLeveling = 0;
#endif
	}
}

//	-----------------------------------------------------------------------------------------------------------

void SetObjectTurnRoll (tObject *objP)
{
//if (!gameStates.app.bD1Mission)
{
	fixang desired_bank = (fixang) -FixMul (objP->mType.physInfo.rotVel[Y], TURNROLL_SCALE);
	if (objP->mType.physInfo.turnRoll != desired_bank) {
		fixang delta_ang, max_roll;
		max_roll = (fixang) FixMul (ROLL_RATE, gameData.physics.xTime);
		delta_ang = desired_bank - objP->mType.physInfo.turnRoll;
		if (labs (delta_ang) < max_roll)
			max_roll = delta_ang;
		else
			if (delta_ang < 0)
				max_roll = -max_roll;
		objP->mType.physInfo.turnRoll += max_roll;
		}
	}
}

//list of segments went through
short physSegList [MAX_FVI_SEGS], nPhysSegs;

#ifdef _DEBUG
#define EXTRADBG 1		//no extra debug when NDEBUG is on
#endif

#ifdef EXTRADBG
tObject *debugObjP=NULL;
#endif

#ifdef _DEBUG
int	nTotalRetries=0, nTotalSims=0;
int	bDontMoveAIObjects=0;
#endif

#define FT (f1_0/64)

extern int bSimpleFVI;
//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
void DoPhysicsSimRot (tObject *objP)
{
	vmsAngVec	turnAngles;
	vmsMatrix	mRotate, mNewOrient;
	//fix			rotdrag_scale;
	tPhysicsInfo *pi;

#if 0
Assert (gameData.physics.xTime > 0); 		//Get MATT if hit this!
#else
if (gameData.physics.xTime <= 0)
	return;
#endif
pi = &objP->mType.physInfo;
if (!(pi->rotVel[X] || pi->rotVel[Y] || pi->rotVel[Z] ||
		pi->rotThrust[X] || pi->rotThrust[Y] || pi->rotThrust[Z]))
	return;
if (objP->mType.physInfo.drag) {
	vmsVector	accel;
	int			nTries = gameData.physics.xTime / FT;
	fix			r = gameData.physics.xTime % FT;
	fix			k = FixDiv (r, FT);
	fix			xDrag = (objP->mType.physInfo.drag * 5) / 2;
	fix			xScale = f1_0 - xDrag;

	if (objP == gameData.objs.console)
		xDrag = EGI_FLAG (nDrag, 0, 0, 0) * xDrag / 10;
	if (objP->mType.physInfo.flags & PF_USES_THRUST) {
		accel = objP->mType.physInfo.rotThrust * FixDiv (f1_0, objP->mType.physInfo.mass);
		while (nTries--) {
			objP->mType.physInfo.rotVel += accel;
			objP->mType.physInfo.rotVel *= xScale;
			}
		//do linear scale on remaining bit of time
		objP->mType.physInfo.rotVel += accel * k;
		if (xDrag)
			objP->mType.physInfo.rotVel *= (f1_0 - FixMul (k, xDrag));
		}
	else if (xDrag && !(objP->mType.physInfo.flags & PF_FREE_SPINNING)) {
		fix xTotalDrag = f1_0;
		while (nTries--)
			xTotalDrag = FixMul (xTotalDrag, xScale);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, f1_0 - FixMul (k, xDrag));
		objP->mType.physInfo.rotVel *= xTotalDrag;
		}
	}
//now rotate tObject
//unrotate tObject for bank caused by turn
if (objP->mType.physInfo.turnRoll) {
	vmsMatrix mOrient;

	turnAngles[PA] = turnAngles[HA] = 0;
	turnAngles[BA] = -objP->mType.physInfo.turnRoll;
	mRotate = vmsMatrix::Create(turnAngles);
	mOrient = objP->position.mOrient * mRotate;
	objP->position.mOrient = mOrient;
}
turnAngles[PA] = (fixang) FixMul (objP->mType.physInfo.rotVel[X], gameData.physics.xTime);
turnAngles[HA] = (fixang) FixMul (objP->mType.physInfo.rotVel[Y], gameData.physics.xTime);
turnAngles[BA] = (fixang) FixMul (objP->mType.physInfo.rotVel[Z], gameData.physics.xTime);
if (!IsMultiGame) {
	int i = (objP != gameData.objs.console) ? 0 : 1;
	if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
		turnAngles[PA] = (fixang) (turnAngles[PA] / gameStates.gameplay.slowmo [i].fSpeed);
		turnAngles[HA] = (fixang) (turnAngles[HA] / gameStates.gameplay.slowmo [i].fSpeed);
		turnAngles[BA] = (fixang) (turnAngles[BA] / gameStates.gameplay.slowmo [i].fSpeed);
		}
	}
mRotate = vmsMatrix::Create(turnAngles);
// TODO MM
mNewOrient = objP->position.mOrient * mRotate;
objP->position.mOrient = mNewOrient;
if (objP->mType.physInfo.flags & PF_TURNROLL)
	SetObjectTurnRoll (objP);
//re-rotate object for bank caused by turn
if (objP->mType.physInfo.turnRoll) {
	vmsMatrix m;

	turnAngles[PA] = turnAngles[HA] = 0;
	turnAngles[BA] = objP->mType.physInfo.turnRoll;
	mRotate = vmsMatrix::Create(turnAngles);
	// TODO MM
	m = objP->position.mOrient * mRotate;
	objP->position.mOrient = m;
	}
objP->position.mOrient.checkAndFix();
}

//	-----------------------------------------------------------------------------------------------------------

void DoBumpHack (tObject *objP)
{
	vmsVector vCenter, vBump;
#ifdef _DEBUG
HUDMessage (0, "BUMP HACK");
#endif
//bump tPlayer a little towards vCenter of tSegment to unstick
COMPUTE_SEGMENT_CENTER_I (&vCenter, objP->nSegment);
//HUDMessage (0, "BUMP! %d %d", d1, d2);
//don't bump tPlayer towards center of reactor tSegment
vmsVector::normalizedDir(vBump, vCenter, objP->position.vPos);
if (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_CONTROLCEN)
	vBump.neg();
objP->position.vPos += vBump * (objP->size / 5);
//if moving away from seg, might move out of seg, so update
if (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_CONTROLCEN)
	UpdateObjectSeg (objP);
}

//	-----------------------------------------------------------------------------------------------------------

#if 1 //UNSTICK_OBJS

int BounceObject (tObject *objP, tFVIData	hi, float fOffs, fix *pxSideDists)
{
	fix	xSideDist, xSideDists [6];
	short	nSegment;

if (!pxSideDists) {
	GetSideDistsAll (objP->position.vPos, hi.hit.nSideSegment, xSideDists);
	pxSideDists = xSideDists;
	}
xSideDist = pxSideDists [hi.hit.nSide];
if (/*(0 <= xSideDist) && */
	 (xSideDist < objP->size - objP->size / 100)) {
#if 0
	objP->position.vPos = objP->vLastPos;
#else
#	if 1
	float r;
	xSideDist = objP->size - xSideDist;
	r = ((float) xSideDist / (float) objP->size) * f2fl (objP->size);
#	endif
	objP->position.vPos[X] += (fix) ((float) hi.hit.vNormal[X] * fOffs);
	objP->position.vPos[Y] += (fix) ((float) hi.hit.vNormal[Y] * fOffs);
	objP->position.vPos[Z] += (fix) ((float) hi.hit.vNormal[Z] * fOffs);
#endif
	nSegment = FindSegByPos (objP->position.vPos, objP->nSegment, 1, 0);
	if ((nSegment < 0) || (nSegment > gameData.segs.nSegments)) {
		objP->position.vPos = objP->vLastPos;
		nSegment = FindSegByPos (objP->position.vPos, objP->nSegment, 1, 0);
		}
	if ((nSegment < 0) || (nSegment > gameData.segs.nSegments) || (nSegment == objP->nSegment))
		return 0;
	RelinkObject (OBJ_IDX (objP), nSegment);
#ifdef _DEBUG
	if (objP->nType == OBJ_PLAYER)
		HUDMessage (0, "PENETRATING WALL (%d, %1.4f)", objP->size - pxSideDists [hi.hit.nSide], r);
#endif
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------------------------------------

void UnstickObject (tObject *objP)
{
	tFVIData			hi;
	tFVIQuery		fq;
	int				fviResult;

if ((objP->nType == OBJ_PLAYER) &&
	 (objP->id == gameData.multiplayer.nLocalPlayer) &&
	 (gameStates.app.cheats.bPhysics == 0xBADA55))
	return;
fq.p0 = fq.p1 = &objP->position.vPos;
fq.startSeg = objP->nSegment;
fq.radP0 = 0;
fq.radP1 = objP->size;
fq.thisObjNum = OBJ_IDX (objP);
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
	BounceObject (objP, hi, f2fl (objP->size - vmsVector::dist(objP->position.vPos, hi.hit.vPoint)) /*0.25f*/, NULL);
#endif
}

#endif

//	-----------------------------------------------------------------------------------------------------------

void UpdateStats (tObject *objP, int nHitType)
{
	int	i;

if (!nHitType)
	return;
if (objP->nType != OBJ_WEAPON)
	return;
if (objP->cType.laserInfo.nParentObj != OBJ_IDX (gameData.objs.console))
	return;
switch (objP->id) {
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

//Simulate a physics tObject for this frame

void DoPhysicsSim (tObject *objP)
{
	short					nIgnoreObjs;
	int					iSeg, i;
	int					bRetry;
	int					fviResult = 0;
	vmsVector			vFrame;				//movement in this frame
	vmsVector			vNewPos, iPos;		//position after this frame
	int					nTries = 0;
	short					nObject = OBJ_IDX (objP);
	short					nWallHitSeg, nWallHitSide;
	tFVIData				hi;
	tFVIQuery			fq;
	vmsVector			vSavePos;
	int					nSaveSeg;
	fix					xSimTime, xOldSimTime, xTimeScale;
	vmsVector			vStartPos;
	int					bGetPhysSegs, bObjStopped = 0;
	fix					xMovedTime;			//how long objected moved before hit something
	vmsVector			vSaveP0, vSaveP1;
	tPhysicsInfo		*pi;
	short					nOrigSegment = objP->nSegment;
	int					nBadSeg = 0, bBounced = 0;
	tSpeedBoostData	sbd = gameData.objs.speedBoost [nObject];
	int					bDoSpeedBoost = sbd.bBoosted; // && (objP == gameData.objs.console);

Assert (objP->nType != OBJ_NONE);
Assert (objP->movementType == MT_PHYSICS);
#ifdef _DEBUG
if (bDontMoveAIObjects)
	if (objP->controlType == CT_AI)
		return;
#endif
CATCH_OBJ (objP, objP->mType.physInfo.velocity[Y] == 0);
DoPhysicsSimRot (objP);
pi = &objP->mType.physInfo;
nPhysSegs = 0;
nIgnoreObjs = 0;
xSimTime = gameData.physics.xTime;
bSimpleFVI = (objP->nType != OBJ_PLAYER);
vStartPos = objP->position.vPos;
if (pi->velocity.isZero()) {
#	if UNSTICK_OBJS
	UnstickObject (objP);
#	endif
	if (objP == gameData.objs.console)
		gameData.objs.speedBoost [nObject].bBoosted = sbd.bBoosted = 0;
	if (pi->thrust.isZero())
		return;
	}


Assert (objP->mType.physInfo.brakes == 0);		//brakes not used anymore?
//if uses thrust, cannot have zero xDrag
Assert (!(objP->mType.physInfo.flags & PF_USES_THRUST) || objP->mType.physInfo.drag);
//do thrust & xDrag
if (objP->mType.physInfo.drag) {
	vmsVector accel, &vel = objP->mType.physInfo.velocity;
	int		nTries = xSimTime / FT;
	fix		xDrag = objP->mType.physInfo.drag;
	fix		r = xSimTime % FT;
	fix		k = FixDiv (r, FT);
	fix		d = f1_0 - xDrag;
	fix		a;

	if (objP == gameData.objs.console)
		xDrag = EGI_FLAG (nDrag, 0, 0, 0) * xDrag / 10;
	if (objP->mType.physInfo.flags & PF_USES_THRUST) {
		accel = objP->mType.physInfo.thrust * FixDiv(f1_0, objP->mType.physInfo.mass);
		a = !accel.isZero();
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
				vel *= (f1_0 - FixMul (k, xDrag));
			if (bDoSpeedBoost) {
				if (vel[X] < sbd.vMinVel[X])
					vel[X] = sbd.vMinVel[X];
				else if (vel[X] > sbd.vMaxVel[X])
					vel[X] = sbd.vMaxVel[X];
				if (vel[Y] < sbd.vMinVel[Y])
					vel[Y] = sbd.vMinVel[Y];
				else if (vel[Y] > sbd.vMaxVel[Y])
					vel[Y] = sbd.vMaxVel[Y];
				if (vel[Z] < sbd.vMinVel[Z])
					vel[Z] = sbd.vMinVel[Z];
				else if (vel[Z] > sbd.vMaxVel[Z])
					vel[Z] = sbd.vMaxVel[Z];
				}
			}
		}
	else if (xDrag) {
		fix xTotalDrag = f1_0;
		while (nTries--)
			xTotalDrag = FixMul (xTotalDrag, d);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, f1_0-FixMul (k, xDrag));
		objP->mType.physInfo.velocity *= xTotalDrag;
		}
	}

//moveIt:

#ifdef _DEBUG
if ((nDbgSeg >= 0) && (objP->nSegment == nDbgSeg))
	nDbgSeg = nDbgSeg;
#endif

if (extraGameInfo [IsMultiGame].bFluidPhysics) {
	if (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_WATER)
		xTimeScale = 75;
	else if (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_LAVA)
		xTimeScale = 66;
	else
		xTimeScale = 100;
	}
else
	xTimeScale = 100;
nTries = 0;
do {
	//Move the tObject
	float fScale = !gameStates.app.bNostalgia && (IS_MISSILE (objP) && (objP->id != EARTHSHAKER_MEGA_ID) && (objP->id != ROBOT_SHAKER_MEGA_ID)) ?
						MissileSpeedScale (objP) : 1;
	bRetry = 0;
	if (fScale < 1) {
		vmsVector vStartVel = gameData.objs.vStartVel[nObject];
		vmsVector::normalize(vStartVel);
		fix xDot = vmsVector::dot(objP->position.mOrient[FVEC], vStartVel);
		vFrame = objP->mType.physInfo.velocity + gameData.objs.vStartVel[nObject];
		vFrame *= fl2f(fScale * fScale);
		vFrame += gameData.objs.vStartVel[nObject] * -(abs (xDot));
		vFrame *= FixMulDiv (xSimTime, xTimeScale, 100 * (nBadSeg + 1));
		}
	else
		vFrame = objP->mType.physInfo.velocity * FixMulDiv (xSimTime, xTimeScale, 100 * (nBadSeg + 1));
	if (!IsMultiGame) {
		int i = (objP != gameData.objs.console) ? 0 : 1;
		if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
			vFrame *= ((fix) (F1_0 / gameStates.gameplay.slowmo [i].fSpeed));
			}
		}
	if (vFrame.isZero())
		break;

retryMove:
#if 1//ndef _DEBUG
	//	If retry count is getting large, then we are trying to do something stupid.
	if (++nTries > 3) {
		if (objP->nType != OBJ_PLAYER)
			break;
		if (nTries > 8) {
			if (sbd.bBoosted)
				sbd.bBoosted = 0;
			break;
			}
		}
#endif
	vNewPos = objP->position.vPos + vFrame;
#if 0
	iSeg = FindSegByPos (&vNewPos, objP->nSegment, 1, 0);
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
	fq.p0 = &objP->position.vPos;
	fq.startSeg = objP->nSegment;
	fq.p1 = &vNewPos;
	fq.radP0 = fq.radP1 = objP->size;
	fq.thisObjNum = nObject;
	fq.ignoreObjList = gameData.physics.ignoreObjs;
	fq.flags = FQ_CHECK_OBJS;

	if (objP->nType == OBJ_WEAPON)
		fq.flags |= FQ_TRANSPOINT;
	if ((bGetPhysSegs = ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT))))
		fq.flags |= FQ_GET_SEGLIST;

	vSaveP0 = *fq.p0;
	vSaveP1 = *fq.p1;
	memset (&hi, 0, sizeof (hi));
	fviResult = FindVectorIntersection (&fq, &hi);
	UpdateStats (objP, fviResult);
	vSavePos = objP->position.vPos;			//save the tObject's position
	nSaveSeg = objP->nSegment;
#if 0//def _DEBUG
	if (objP->nType == OBJ_PLAYER)
		HUDMessage (0, "FVI: %d (%1.2f)", fviResult, f2fl (VmVecMag (&objP->mType.physInfo.velocity)));
#endif
	if (fviResult == HIT_BAD_P0) {
#ifdef _DEBUG
		static int nBadP0 = 0;
		HUDMessage (0, "BAD P0 %d", nBadP0++);
#endif
#if 0
		return;
#else
		memset (&hi, 0, sizeof (hi));
		fviResult = FindVectorIntersection (&fq, &hi);
		fq.startSeg = FindSegByPos (vNewPos, objP->nSegment, 1, 0);
		if ((fq.startSeg < 0) || (fq.startSeg == objP->nSegment)) {
			objP->position.vPos = vSavePos;
			break;
			}
		fviResult = FindVectorIntersection (&fq, &hi);
		if (fviResult == HIT_BAD_P0) {
			objP->position.vPos = vSavePos;
			break;
			}
#endif
		}
	else if (fviResult == HIT_WALL) {
#if 0//def _DEBUG
		tFVIData hiSave = hi;
		fviResult = FindVectorIntersection (&fq, &hi);
		{
		int vertList [6];
		int nFaces = CreateAbsVertexLists (vertList, hi.hit.nSideSegment, hi.hit.nSide);
		if (hi.hit.nFace >= nFaces)
			fviResult = FindVectorIntersection (&fq, &hi);
		}
#	if 0
		HUDMessage (0, "hit wall %d:%d (%d)", hi.hit.nSideSegment, hi.hit.nSide, OBJECTS->nSegment);
#	endif
#endif
#if 1 //make shots and missiles pass through skyboxes
		if (gameStates.render.bHaveSkyBox && (objP->nType == OBJ_WEAPON) && (hi.hit.nSegment >= 0)) {
			if (gameData.segs.segment2s [hi.hit.nSegment].special == SEGMENT_IS_SKYBOX) {
				short nConnSeg = SEGMENTS [hi.hit.nSegment].children [hi.hit.nSide];
				if ((nConnSeg < 0) && (objP->lifeleft > F1_0)) {	//leaving the mine
					objP->lifeleft = 0;
					objP->flags |= OF_SHOULD_BE_DEAD;
					}
				fviResult = HIT_NONE;
				}
			else if (CheckTransWall (&hi.hit.vPoint, SEGMENTS + hi.hit.nSideSegment, hi.hit.nSide, hi.hit.nFace)) {
				short nNewSeg = FindSegByPos (vNewPos, gameData.segs.skybox.segments [0], 1, 1);
				if ((nNewSeg >= 0) && (gameData.segs.segment2s [nNewSeg].special == SEGMENT_IS_SKYBOX)) {
					hi.hit.nSegment = nNewSeg;
					fviResult = HIT_NONE;
					}
				}
			}
#endif
		}
	//	Matt: Mike's hack.
	else if (fviResult == HIT_OBJECT) {
		tObject	*hitObjP = OBJECTS + hi.hit.nObject;

		if (ObjIsPlayerMine (hitObjP))
			nTries--;
#if 0//def _DEBUG
		fviResult = FindVectorIntersection (&fq, &hi);
#endif
		}

	if (bGetPhysSegs) {
		if (nPhysSegs && (physSegList [nPhysSegs-1] == hi.segList [0]))
			nPhysSegs--;
#ifdef RELEASE
		i = MAX_FVI_SEGS - nPhysSegs - 1;
		if (i > 0) {
			if (i > hi.nSegments)
				i = hi.nSegments;
			if (i < 0)
				FindVectorIntersection (&fq, &hi);
			memcpy (physSegList + nPhysSegs, hi.segList, i * sizeof (*physSegList));
			nPhysSegs += i;
			}
		else
			i = i;
#else
		for (i = 0; (i < hi.nSegments) && (nPhysSegs < MAX_FVI_SEGS-1); ) {
			if (hi.segList [i] > gameData.segs.nLastSegment)
				PrintLog ("Invalid segment in segment list #1\n");
			physSegList [nPhysSegs++] = hi.segList [i++];
			}
#endif
		}
	iPos = hi.hit.vPoint;
	iSeg = hi.hit.nSegment;
	nWallHitSide = hi.hit.nSide;
	nWallHitSeg = hi.hit.nSideSegment;
	if (iSeg == -1) {		//some sort of horrible error
		if (objP->nType == OBJ_WEAPON)
			KillObject (objP);
		break;
		}
	Assert ((fviResult != HIT_WALL) || ((nWallHitSeg > -1) && (nWallHitSeg <= gameData.segs.nLastSegment)));
	// update tObject's position and tSegment number
	objP->position.vPos = iPos;
	if (iSeg != objP->nSegment)
		RelinkObject (nObject, iSeg);
	//if start point not in tSegment, move tObject to center of tSegment
	if (GetSegMasks (objP->position.vPos, objP->nSegment, 0).centerMask) {	//object stuck
		int n = FindObjectSeg (objP);
		if (n == -1) {
			if (bGetPhysSegs)
				n = FindSegByPos (objP->vLastPos, objP->nSegment, 1, 0);
			if (n == -1) {
				objP->position.vPos = objP->vLastPos;
				RelinkObject (nObject, objP->nSegment);
				}
			else {
				vmsVector vCenter;
				COMPUTE_SEGMENT_CENTER_I (&vCenter, objP->nSegment);
				vCenter -= objP->position.vPos;
				if (vCenter.mag() > F1_0) {
					vmsVector::normalize(vCenter);
					vCenter /= 10;
					}
				objP->position.vPos -= vCenter;
				}
			if (objP->nType == OBJ_WEAPON) {
				KillObject (objP);
				return;
				}
			}
		//return;
		}

	//calulate new sim time
	{
		//vmsVector vMoved;
		vmsVector vMoveNormal;
		fix attemptedDist, actualDist;

		xOldSimTime = xSimTime;
		actualDist = vmsVector::normalizedDir(vMoveNormal, objP->position.vPos, vSavePos);
		if ((fviResult == HIT_WALL) && (vmsVector::dot(vMoveNormal, vFrame) < 0)) {		//moved backwards
			//don't change position or xSimTime
			objP->position.vPos = vSavePos;
			//iSeg = objP->nSegment;		//don't change tSegment
			if (nSaveSeg != iSeg)
				RelinkObject (nObject, nSaveSeg);
			if (bDoSpeedBoost) {
				objP->position.vPos = vStartPos;
				SetSpeedBoostVelocity (nObject, -1, -1, -1, -1, -1, &vStartPos, &sbd.vDest, 0);
				vFrame = sbd.vVel * xSimTime;
				goto retryMove;
				}
			xMovedTime = 0;
			}
		else {
			attemptedDist = vFrame.mag();
			xSimTime = FixMulDiv (xSimTime, attemptedDist - actualDist, attemptedDist);
			xMovedTime = xOldSimTime - xSimTime;
			if ((xSimTime < 0) || (xSimTime > xOldSimTime)) {
				xSimTime = xOldSimTime;
				xMovedTime = 0;
				}
			}
		}

	if (fviResult == HIT_WALL) {
		vmsVector	vMoved;
		fix			xHitSpeed, xWallPart;
		// Find hit speed

#if 0//def _DEBUG
		if (objP->nType == OBJ_PLAYER)
			HUDMessage (0, "WALL CONTACT");
		fviResult = FindVectorIntersection (&fq, &hi);
#endif
		vMoved = objP->position.vPos - vSavePos;
		xWallPart = vmsVector::dot(vMoved, hi.hit.vNormal) / gameData.collisions.hitData.nNormals;
		if (xWallPart && (xMovedTime > 0) && ((xHitSpeed = -FixDiv (xWallPart, xMovedTime)) > 0)) {
			CollideObjectWithWall (objP, xHitSpeed, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
#if 0//def _DEBUG
			if (objP->nType == OBJ_PLAYER)
				HUDMessage (0, "BUMP %1.2f (%d,%d)!", f2fl (xHitSpeed), nWallHitSeg, nWallHitSide);
#endif
			}
		else {
#if 0//def _DEBUG
			if (objP->nType == OBJ_PLAYER)
				HUDMessage (0, "SCREEEEEEEEEECH");
#endif
			ScrapeObjectOnWall (objP, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
			}
		Assert (nWallHitSeg > -1);
		Assert (nWallHitSide > -1);
#if UNSTICK_OBJS == 2
		{
		fix	xSideDists [6];
		GetSideDistsAll (&objP->position.vPos, nWallHitSeg, xSideDists);
		bRetry = BounceObject (objP, hi, 0.1f, xSideDists);
		}
#else
		bRetry = 0;
#endif
		if (!(objP->flags & OF_SHOULD_BE_DEAD) && (objP->nType != OBJ_DEBRIS)) {
			int bForceFieldBounce;		//bounce off a forcefield

			///Assert (gameStates.app.cheats.bBouncingWeapons || ((objP->mType.physInfo.flags & (PF_STICK | PF_BOUNCE)) != (PF_STICK | PF_BOUNCE)));	//can't be bounce and stick
			bForceFieldBounce = (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nWallHitSeg].sides [nWallHitSide].nBaseTex].flags & TMI_FORCE_FIELD);
			if (!bForceFieldBounce && (objP->mType.physInfo.flags & PF_STICK)) {		//stop moving
				AddStuckObject (objP, nWallHitSeg, nWallHitSide);
				objP->mType.physInfo.velocity.setZero();
				bObjStopped = 1;
				bRetry = 0;
				}
			else {				// Slide tObject along tWall
				int bCheckVel = 0;
				//We're constrained by a wall, so subtract wall part from velocity vector

				// TODO: fix this with new dot product method, without bouncing off walls
				// THIS IS DELICATE!!
				xWallPart = vmsVector::dot(hi.hit.vNormal, objP->mType.physInfo.velocity);
				if (bForceFieldBounce || (objP->mType.physInfo.flags & PF_BOUNCE)) {		//bounce off tWall
					xWallPart *= 2;	//Subtract out wall part twice to achieve bounce
					if (bForceFieldBounce) {
						bCheckVel = 1;				//check for max velocity
						if (objP->nType == OBJ_PLAYER)
							xWallPart *= 2;		//tPlayer bounce twice as much
						}
					if ((objP->mType.physInfo.flags & (PF_BOUNCE | PF_BOUNCES_TWICE)) == (PF_BOUNCE | PF_BOUNCES_TWICE)) {
						//Assert (objP->mType.physInfo.flags & PF_BOUNCE);
						if (objP->mType.physInfo.flags & PF_BOUNCED_ONCE)
							objP->mType.physInfo.flags &= ~(PF_BOUNCE | PF_BOUNCED_ONCE | PF_BOUNCES_TWICE);
						else
							objP->mType.physInfo.flags |= PF_BOUNCED_ONCE;
						}
					bBounced = 1;		//this tObject bBounced
					}
				objP->mType.physInfo.velocity += hi.hit.vNormal * (-xWallPart);
				if (bCheckVel) {
					fix vel = objP->mType.physInfo.velocity.mag();
					if (vel > MAX_OBJECT_VEL)
						objP->mType.physInfo.velocity *= (FixDiv (MAX_OBJECT_VEL, vel));
					}
				if (bBounced && (objP->nType == OBJ_WEAPON))
					objP->position.mOrient = vmsMatrix::CreateFU(objP->mType.physInfo.velocity, objP->position.mOrient[UVEC]);
					//objP->position.mOrient = vmsMatrix::CreateFU(objP->mType.physInfo.velocity, &objP->position.mOrient[UVEC], NULL);
				bRetry = 1;
				}
			}
		}
	else if (fviResult == HIT_OBJECT) {
		vmsVector	vOldVel;
		vmsVector	*ppos0, *ppos1, vHitPos;
		fix			size0, size1;
		// Mark the hit tObject so that on a retry the fvi code
		// ignores this tObject.
		//if (bSpeedBoost && (objP == gameData.objs.console))
		//	break;
#ifdef _DEBUG
		tFVIData				_hi;
		memset (&_hi, 0, sizeof (_hi));
		FindVectorIntersection (&fq, &_hi);
#endif
		Assert (hi.hit.nObject != -1);
		ppos0 = &OBJECTS [hi.hit.nObject].position.vPos;
		ppos1 = &objP->position.vPos;
		size0 = OBJECTS [hi.hit.nObject].size;
		size1 = objP->size;
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
		vOldVel = objP->mType.physInfo.velocity;
		//if (!(SPECTATOR (objP) || SPECTATOR (OBJECTS + hi.hit.nObject)))
			CollideTwoObjects (objP, OBJECTS + hi.hit.nObject, &vHitPos);
		if (sbd.bBoosted && (objP == gameData.objs.console))
			objP->mType.physInfo.velocity = vOldVel;
		// Let object continue its movement
		if (!(objP->flags & OF_SHOULD_BE_DEAD)) {
			if ((objP->mType.physInfo.flags & PF_PERSISTENT) ||
				 (vOldVel[X] == objP->mType.physInfo.velocity[X] &&
				  vOldVel[Y] == objP->mType.physInfo.velocity[Y]) &&
				  vOldVel[Z] == objP->mType.physInfo.velocity[Z]) {
				if (OBJECTS [hi.hit.nObject].nType == OBJ_POWERUP)
					nTries--;
				gameData.physics.ignoreObjs [nIgnoreObjs++] = hi.hit.nObject;
				bRetry = 1;
				}
#ifdef _DEBUG
			else
				bRetry = bRetry;
#endif
			}
		}
	else if (fviResult == HIT_NONE) {
#ifdef TACTILE
		if (TactileStick && (objP == gameData.objs.console) && !(FrameCount & 15))
			Tactile_Xvibrate_clear ();
#endif
		}
#ifdef _DEBUG
	else if (fviResult == HIT_BAD_P0) {
		Int3 ();		// Unexpected collision nType: start point not in specified tSegment.
#if TRACE
		con_printf (CONDBG, "Warning: Bad p0 in physics!!!\n");
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
if (objP->controlType == CT_AI) {
	Assert (nObject >= 0);
	if (nTries > 0) {
		gameData.ai.localInfo [nObject].nRetryCount = nTries - 1;
#ifdef _DEBUG
		nTotalRetries += nTries - 1;
		nTotalSims++;
#endif
		}
	}
	// If the ship has thrust, but the velocity is zero or the current position equals the start position
	// stored when entering this function, it has been stopped forcefully by something, so bounce it back to
	// avoid that the ship gets driven into the obstacle (most likely a wall, as that doesn't give in ;)
	if (((fviResult == HIT_WALL) || (fviResult == HIT_BAD_P0)) &&
		 !(sbd.bBoosted || bObjStopped || bBounced))	{	//Set velocity from actual movement
		vmsVector vMoved;
		fix s = FixMulDiv (FixDiv (F1_0, gameData.physics.xTime), xTimeScale, 100);

		vMoved = objP->position.vPos - vStartPos;
		s = vMoved.mag();
		vMoved *= (FixMulDiv (FixDiv (F1_0, gameData.physics.xTime), xTimeScale, 100));
#if 1
		if (!bDoSpeedBoost)
			objP->mType.physInfo.velocity = vMoved;
#endif
#ifdef BUMP_HACK
		if ((objP == gameData.objs.console) &&
			 vMoved.isZero() &&
			 !objP->mType.physInfo.thrust.isZero()) {
			DoBumpHack (objP);
			}
#endif
		}

	if (objP->mType.physInfo.flags & PF_LEVELLING)
		DoPhysicsAlignObject (objP);
	//hack to keep tPlayer from going through closed doors
	if (((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT)) && (objP->nSegment != nOrigSegment) &&
		 (gameStates.app.cheats.bPhysics != 0xBADA55)) {
		int nSide = FindConnectedSide (gameData.segs.segments + objP->nSegment, gameData.segs.segments + nOrigSegment);
		if (nSide != -1) {
			if (!(WALL_IS_DOORWAY (gameData.segs.segments + nOrigSegment, nSide, (objP->nType == OBJ_PLAYER) ? objP : NULL) & WID_FLY_FLAG)) {
				tSide *sideP;
				int	nVertex, nFaces;
				fix	dist;
				int	vertexList [6];

				//bump tObject back
				sideP = gameData.segs.segments [nOrigSegment].sides + nSide;
				if (nOrigSegment == -1)
					Error ("nOrigSegment == -1 in physics");
				nFaces = CreateAbsVertexLists (vertexList, nOrigSegment, nSide);
				//let'sideP pretend this tWall is not triangulated
				nVertex = vertexList [0];
				if (nVertex > vertexList [1])
					nVertex = vertexList [1];
				if (nVertex > vertexList [2])
					nVertex = vertexList [2];
				if (nVertex > vertexList [3])
					nVertex = vertexList [3];
				dist = vStartPos.distToPlane(sideP->normals[0], gameData.segs.vertices[nVertex]);
				objP->position.vPos = vStartPos + sideP->normals[0] * (objP->size-dist);
				UpdateObjectSeg (objP);
				}
			}
		}

//if end point not in tSegment, move tObject to last pos, or tSegment center
if (GetSegMasks (objP->position.vPos, objP->nSegment, 0).centerMask) {
	if (FindObjectSeg (objP) == -1) {
		int n;

		if (((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT)) && (n = FindSegByPos (objP->vLastPos, objP->nSegment, 1, 0)) != -1) {
			objP->position.vPos = objP->vLastPos;
			RelinkObject (nObject, n);
			}
		else {
			COMPUTE_SEGMENT_CENTER_I (&objP->position.vPos, objP->nSegment);
			objP->position.vPos[X] += nObject;
			}
		if (objP->nType == OBJ_WEAPON)
			KillObject (objP);
		}
	}
CATCH_OBJ (objP, objP->mType.physInfo.velocity[Y] == 0);
#if UNSTICK_OBJS
UnstickObject (objP);
#endif
}

//	----------------------------------------------------------------
//Applies an instantaneous force on an tObject, resulting in an instantaneous
//change in velocity.

void PhysApplyForce (tObject *objP, vmsVector *vForce)
{
#ifdef _DEBUG
	fix mag;
#endif
	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass,
	//	in collision with crazy reactor robot thing on d2levf-s.
if (objP->mType.physInfo.mass == 0)
	return;
if (objP->movementType != MT_PHYSICS)
	return;
if ((gameStates.render.automap.bDisplay && (objP == gameData.objs.console)) || SPECTATOR (objP))
	return;
#ifdef TACTILE
  if (TactileStick && (obj == OBJECTS + LOCALPLAYER.nObject))
	Tactile_apply_force (vForce, &objP->position.mOrient);
#endif
//Add in acceleration due to force
#ifdef _DEBUG
mag = objP->mType.physInfo.velocity.mag();
#endif
if (!gameData.objs.speedBoost [OBJ_IDX (objP)].bBoosted || (objP != gameData.objs.console))
	objP->mType.physInfo.velocity += *vForce * FixDiv (f1_0, objP->mType.physInfo.mass);
#ifdef _DEBUG
mag = objP->mType.physInfo.velocity.mag();
if (f2fl (mag) > 500)
	objP = objP;
#endif
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void PhysicsSetRotVelAndSaturate (fix *dest, fix delta)
{
if ((delta ^ *dest) < 0) {
	if (abs (delta) < F1_0/8) {
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
//	PhysApplyRot used to call AITurnTowardsVector until I fixed it, which broke PhysApplyRot.
void PhysicsTurnTowardsVector (vmsVector *vGoal, tObject *objP, fix rate)
{
	vmsAngVec	dest_angles, cur_angles;
	fix			delta_p, delta_h;
	vmsVector&	pvRotVel = objP->mType.physInfo.rotVel;

// Make this tObject turn towards the vGoal.  Changes orientation, doesn't change direction of movement.
// If no one moves, will be facing vGoal in 1 second.

//	Detect null vector.
if (gameStates.render.automap.bDisplay && (objP == gameData.objs.console))
	return;
if (vGoal->isZero())
	return;
//	Make morph OBJECTS turn more slowly.
if (objP->controlType == CT_MORPH)
	rate *= 2;

dest_angles = vGoal->toAnglesVec();
cur_angles = objP->position.mOrient[FVEC].toAnglesVec();
delta_p = (dest_angles[PA] - cur_angles[PA]);
delta_h = (dest_angles[HA] - cur_angles[HA]);
if (delta_p > F1_0/2)
	delta_p = dest_angles[PA] - cur_angles[PA] - F1_0;
if (delta_p < -F1_0/2)
	delta_p = dest_angles[PA] - cur_angles[PA] + F1_0;
if (delta_h > F1_0/2)
	delta_h = dest_angles[HA] - cur_angles[HA] - F1_0;
if (delta_h < -F1_0/2)
	delta_h = dest_angles[HA] - cur_angles[HA] + F1_0;
delta_p = FixDiv (delta_p, rate);
delta_h = FixDiv (delta_h, rate);
if (abs (delta_p) < F1_0/16) delta_p *= 4;
if (abs (delta_h) < F1_0/16) delta_h *= 4;
if (!IsMultiGame) {
	int i = (objP != gameData.objs.console) ? 0 : 1;
	if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
		delta_p = (fix) (delta_p / gameStates.gameplay.slowmo [i].fSpeed);
		delta_h = (fix) (delta_h / gameStates.gameplay.slowmo [i].fSpeed);
		}
	}
PhysicsSetRotVelAndSaturate(&pvRotVel[X], delta_p);
PhysicsSetRotVelAndSaturate(&pvRotVel[Y], delta_h);
pvRotVel[Z] = 0;
}

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an tObject, resulting in an instantaneous
//	change in orientation.
void PhysApplyRot (tObject *objP, vmsVector *vForce)
{
	fix	xRate, xMag;

if (objP->movementType != MT_PHYSICS)
	return;
xMag = vForce->mag()/8;
if (xMag < F1_0/256)
	xRate = 4 * F1_0;
else if (xMag < objP->mType.physInfo.mass >> 14)
	xRate = 4 * F1_0;
else {
	xRate = FixDiv (objP->mType.physInfo.mass, xMag);
	if (objP->nType == OBJ_ROBOT) {
		if (xRate < F1_0/4)
			xRate = F1_0/4;
		//	Changed by mk, 10/24/95, claw guys should not slow down when attacking!
		if (!(ROBOTINFO (objP->id).thief || ROBOTINFO (objP->id).attackType)) {
			if (objP->cType.aiInfo.SKIP_AI_COUNT * gameData.physics.xTime < 3*F1_0/4) {
				fix	tval = FixDiv (F1_0, 8 * gameData.physics.xTime);
				int	addval = f2i (tval);
				if ((d_rand () * 2) < (tval & 0xffff))
					addval++;
				objP->cType.aiInfo.SKIP_AI_COUNT += addval;
				}
			}
		}
	else {
		if (xRate < F1_0/2)
			xRate = F1_0/2;
		}
	}
//	Turn amount inversely proportional to mass.  Third parameter is seconds to do 360 turn.
PhysicsTurnTowardsVector (vForce, objP, xRate);
}


//this routine will set the thrust for an tObject to a value that will
// (hopefully) maintain the tObject's current velocity
void SetThrustFromVelocity (tObject *objP)
{
	fix k;

	Assert (objP->movementType == MT_PHYSICS);

	k = FixMulDiv (objP->mType.physInfo.mass, objP->mType.physInfo.drag, (f1_0-objP->mType.physInfo.drag));

	objP->mType.physInfo.thrust = objP->mType.physInfo.velocity * k;

}


