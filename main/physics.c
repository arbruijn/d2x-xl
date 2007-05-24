/* $Id: physics.c, v 1.4 2003/10/10 09:36:35 btb Exp $ */
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
static char rcsid [] = "$Id: physics.c, v 1.4 2003/10/10 09:36:35 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>

#include "joy.h"
#include "mono.h"
#include "error.h"

#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "physics.h"
#include "key.h"
#include "game.h"
#include "collide.h"
#include "fvi.h"
#include "newdemo.h"
#include "timer.h"
#include "ai.h"
#include "wall.h"
#include "switch.h"
#include "laser.h"
#include "bm.h"
#include "player.h"
#include "network.h"
#include "hudmsg.h"
#include "gameseg.h"
#include "kconfig.h"
#include "input.h"

#ifdef TACTILE
#include "tactile.h"
#endif

//Global variables for physics system
//#define _DEBUG
#define FLUID_PHYSICS	0
#define UNSTICK_OBJS		1

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
//make sure matrix is orthogonal
void CheckAndFixMatrix (vmsMatrix *m)
{
	vmsMatrix tempm;

VmVector2Matrix (&tempm, &m->fVec, &m->uVec, NULL);
*m = tempm;
}

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
//find tSide of tSegment that tPlayer is most alligned with
for (i = 0; i < 6; i++) {
	d = VmVecDot (gameData.segs.segments [objP->nSegment].sides [i].normals, &objP->position.mOrient.uVec);
	if (d > largest_d) {largest_d = d; best_side=i;}
	}
if (bFloorLeveling)
	// old way: used floor's normal as upvec
	desiredUpVec = gameData.segs.segments [objP->nSegment].sides [3].normals [0];
else  // new tPlayer leveling code: use normal of tSide closest to our up vec
	if (GetNumFaces (&gameData.segs.segments [objP->nSegment].sides [best_side])==2) {
		tSide *s = &gameData.segs.segments [objP->nSegment].sides [best_side];
		desiredUpVec.p.x = (s->normals [0].p.x + s->normals [1].p.x) / 2;
		desiredUpVec.p.y = (s->normals [0].p.y + s->normals [1].p.y) / 2;
		desiredUpVec.p.z = (s->normals [0].p.z + s->normals [1].p.z) / 2;
		VmVecNormalize (&desiredUpVec);
		}
	else
		desiredUpVec = gameData.segs.segments [objP->nSegment].sides [best_side].normals [0];
if (labs (VmVecDot (&desiredUpVec, &objP->position.mOrient.fVec)) < f1_0/2) {
	fixang save_delta_ang;
	vmsAngVec tangles;

	VmVector2Matrix (&temp_matrix, &objP->position.mOrient.fVec, &desiredUpVec, NULL);
	save_delta_ang = 
	delta_ang = VmVecDeltaAng (&objP->position.mOrient.uVec, &temp_matrix.uVec, &objP->position.mOrient.fVec);
	delta_ang += objP->mType.physInfo.turnRoll;
	if (abs (delta_ang) > DAMP_ANG) {
		vmsMatrix mRotate, new_pm;

		roll_ang = (fixang) FixMul (gameData.physics.xTime, ROLL_RATE);
		if (abs (delta_ang) < roll_ang) 
			roll_ang = delta_ang;
		else if (delta_ang < 0) 
			roll_ang = -roll_ang;
		tangles.p = tangles.h = 0;  
		tangles.b = roll_ang;
		VmAngles2Matrix (&mRotate, &tangles);
		VmMatMul (&new_pm, &objP->position.mOrient, &mRotate);
		objP->position.mOrient = new_pm;
		}
	else 
		bFloorLeveling = 0;
	}
}

//	-----------------------------------------------------------------------------------------------------------

void SetObjectTurnRoll (tObject *objP)
{
//if (!gameStates.app.bD1Mission) 
{
	fixang desired_bank = (fixang) -FixMul (objP->mType.physInfo.rotVel.p.y, TURNROLL_SCALE);
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

#define MAX_IGNORE_OBJS 100

#ifdef _DEBUG
#define EXTRADBG 1		//no extra debug when NDEBUG is on
#endif

#ifdef EXTRADBG
tObject *debugObjP=NULL;
#endif

#define XYZ(v) (v)->x, (v)->y, (v)->z


#ifdef _DEBUG
int	Total_retries=0, Total_sims=0;
int	bDontMoveAIObjects=0;
#endif

#define FT (f1_0/64)

extern int bSimpleFVI;
//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
void DoPhysicsSimRot (tObject *objP)
{
	vmsAngVec	tangles;
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
if (!(pi->rotVel.p.x || pi->rotVel.p.y || pi->rotVel.p.z || 
		pi->rotThrust.p.x || pi->rotThrust.p.y || pi->rotThrust.p.z))
	return;
if (objP->mType.physInfo.drag) {
	vmsVector	accel;
	int			nTries = gameData.physics.xTime / FT;
	fix			r = gameData.physics.xTime % FT;
	fix			k = FixDiv (r, FT);
	fix			xDrag = (objP->mType.physInfo.drag * 5) / 2;
	fix			xScale = f1_0 - xDrag;

	if (objP->mType.physInfo.flags & PF_USES_THRUST) {
		VmVecCopyScale (&accel, &objP->mType.physInfo.rotThrust, FixDiv (f1_0, objP->mType.physInfo.mass));
		while (nTries--) {
			VmVecInc (&objP->mType.physInfo.rotVel, &accel);
			VmVecScale (&objP->mType.physInfo.rotVel, xScale);
			}
		//do linear scale on remaining bit of time
		VmVecScaleInc (&objP->mType.physInfo.rotVel, &accel, k);
		VmVecScale (&objP->mType.physInfo.rotVel, f1_0 - FixMul (k, xDrag));
		}
	else if (!(objP->mType.physInfo.flags & PF_FREE_SPINNING)) {
		fix xTotalDrag = f1_0;
		while (nTries--)
			xTotalDrag = FixMul (xTotalDrag, xScale);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, f1_0 - FixMul (k, xDrag));
		VmVecScale (&objP->mType.physInfo.rotVel, xTotalDrag);
		}
	}
//now rotate tObject
//unrotate tObject for bank caused by turn
if (objP->mType.physInfo.turnRoll) {
	vmsMatrix new_pm;

	tangles.p = tangles.h = 0;
	tangles.b = -objP->mType.physInfo.turnRoll;
	VmAngles2Matrix (&mRotate, &tangles);
	VmMatMul (&new_pm, &objP->position.mOrient, &mRotate);
	objP->position.mOrient = new_pm;
	}
tangles.p = (fixang) FixMul (objP->mType.physInfo.rotVel.p.x, gameData.physics.xTime);
tangles.h = (fixang) FixMul (objP->mType.physInfo.rotVel.p.y, gameData.physics.xTime);
tangles.b = (fixang) FixMul (objP->mType.physInfo.rotVel.p.z, gameData.physics.xTime);
if (!IsMultiGame) {
	int i = (objP != gameData.objs.console) ? 0 : 1;
	if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
		tangles.p = (fix) (tangles.p / gameStates.gameplay.slowmo [i].fSpeed);
		tangles.h = (fix) (tangles.h / gameStates.gameplay.slowmo [i].fSpeed);
		tangles.b = (fix) (tangles.b / gameStates.gameplay.slowmo [i].fSpeed);
		}
	}
VmAngles2Matrix (&mRotate, &tangles);
VmMatMul (&mNewOrient, &objP->position.mOrient, &mRotate);
objP->position.mOrient = mNewOrient;
if (objP->mType.physInfo.flags & PF_TURNROLL)
	SetObjectTurnRoll (objP);
//re-rotate object for bank caused by turn
if (objP->mType.physInfo.turnRoll) {
	vmsMatrix new_pm;

	tangles.p = tangles.h = 0;
	tangles.b = objP->mType.physInfo.turnRoll;
	VmAngles2Matrix (&mRotate, &tangles);
	VmMatMul (&new_pm, &objP->position.mOrient, &mRotate);
	objP->position.mOrient = new_pm;
	}
CheckAndFixMatrix (&objP->position.mOrient);
}

//	-----------------------------------------------------------------------------------------------------------

#if UNSTICK_OBJS

int BounceObject (tObject *objP, tFVIData	hi, float fOffs, fix *pxSideDists)
{
	fix	xSideDist, xSideDists [6];
	short	nSegment;

if (!pxSideDists) {
	GetSideDistsAll (&objP->position.vPos, hi.hit.nSideSegment, xSideDists);
	pxSideDists = xSideDists;
	}
xSideDist = pxSideDists [hi.hit.nSide];
if (/*(0 <= xSideDist) && */
	 (xSideDist < objP->size - objP->size / 100)) {
#if 0
	objP->position.vPos = objP->vLastPos;
#else
#	if 0
	float fOffs;
	xSideDist = objP->size - xSideDist;
	r = ((float) xSideDist / (float) objP->size) * f2fl (objP->size);
#	endif
	objP->position.vPos.p.x += (fix) ((float) hi.hit.vNormal.p.x * fOffs);
	objP->position.vPos.p.y += (fix) ((float) hi.hit.vNormal.p.y * fOffs);
	objP->position.vPos.p.z += (fix) ((float) hi.hit.vNormal.p.z * fOffs);
#endif
	nSegment = FindSegByPoint (&objP->position.vPos, objP->nSegment);
	if ((nSegment < 0) || (nSegment > gameData.segs.nSegments)) {
		objP->position.vPos = objP->vLastPos;
		nSegment = FindSegByPoint (&objP->position.vPos, objP->nSegment);
		}
	if ((nSegment < 0) || (nSegment > gameData.segs.nSegments) || (nSegment == objP->nSegment))
		return 0;
	RelinkObject (OBJ_IDX (objP), nSegment);
#if 0//def _DEBUG
	if (objP->nType == OBJ_PLAYER)
		HUDMessage (0, "PENETRATING WALL (%d, %1.4f)", objP->size - pxSideDists [hi.hit.nSide], r);
#endif
	return 1;
	}
return 0;
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
VmVecNormalizedDirQuick (&vBump, &vCenter, &objP->position.vPos);
if (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_CONTROLCEN)
	VmVecNegate (&vBump);
VmVecScaleInc (&objP->position.vPos, &vBump, objP->size / 5);
//if moving away from seg, might move out of seg, so update
if (gameData.segs.segment2s [objP->nSegment].special == SEGMENT_IS_CONTROLCEN)
	UpdateObjectSeg (objP);
}

//	-----------------------------------------------------------------------------------------------------------

void UnstickObject (tObject *objP)
{
	tFVIData			hi;
	tVFIQuery		fq;
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
	BounceObject (objP, hi, f2fl (objP->size - VmVecDist (&objP->position.vPos, &hi.hit.vPoint)) /*0.25f*/, NULL);
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

extern tObject *monsterballP;
extern int nWeaponObj;

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
	tVFIQuery			fq;
	vmsVector			vSavePos;
	int					nSaveSeg;
	fix					xDrag;
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
	static int qqq = 0;
Assert (objP->nType != OBJ_NONE);
Assert (objP->movementType == MT_PHYSICS);
#ifdef _DEBUG
if (bDontMoveAIObjects)
	if (objP->controlType == CT_AI)
		return;
#endif
CATCH_OBJ (objP, objP->mType.physInfo.velocity.p.y == 0);
DoPhysicsSimRot (objP);
pi = &objP->mType.physInfo;
nPhysSegs = 0;
nIgnoreObjs = 0;
xSimTime = gameData.physics.xTime;
bSimpleFVI = (objP->nType != OBJ_PLAYER);
vStartPos = objP->position.vPos;
if (!(pi->velocity.p.x || pi->velocity.p.y || pi->velocity.p.z)) {
#	if UNSTICK_OBJS
	UnstickObject (objP);
#	endif
	if (objP == gameData.objs.console)
		gameData.objs.speedBoost [nObject].bBoosted = sbd.bBoosted = 0;
	if (!(pi->thrust.p.x || pi->thrust.p.y || pi->thrust.p.z))
		return;
	}


Assert (objP->mType.physInfo.brakes == 0);		//brakes not used anymore?
//if uses thrust, cannot have zero xDrag
Assert (!(objP->mType.physInfo.flags & PF_USES_THRUST) || objP->mType.physInfo.drag);
//do thrust & xDrag
if ((xDrag = objP->mType.physInfo.drag)) {
	vmsVector accel, *vel = &objP->mType.physInfo.velocity;
	fix a;

	int nTries = xSimTime / FT;
	fix r = xSimTime % FT;
	fix k = FixDiv (r, FT);
	fix d = f1_0 - xDrag;

	if (objP->mType.physInfo.flags & PF_USES_THRUST) {
		VmVecCopyScale (&accel, &objP->mType.physInfo.thrust, FixDiv (f1_0, objP->mType.physInfo.mass));
		a = (accel.p.x || accel.p.y || accel.p.z);
		if (bDoSpeedBoost && !(a || gameStates.input.bControlsSkipFrame))
			*vel = sbd.vVel;
		else {
			while (nTries--) {
				if (a)
					VmVecInc (vel, &accel);
				VmVecScale (vel, d);
			}
			//do linear scale on remaining bit of time
			VmVecScaleInc (vel, &accel, k);
			VmVecScale (vel, f1_0 - FixMul (k, xDrag));
			if (bDoSpeedBoost) {
				if (vel->p.x < sbd.vMinVel.p.x)
					vel->p.x = sbd.vMinVel.p.x;
				else if (vel->p.x > sbd.vMaxVel.p.x)
					vel->p.x = sbd.vMaxVel.p.x;
				if (vel->p.y < sbd.vMinVel.p.y)
					vel->p.y = sbd.vMinVel.p.y;
				else if (vel->p.y > sbd.vMaxVel.p.y)
					vel->p.y = sbd.vMaxVel.p.y;
				if (vel->p.z < sbd.vMinVel.p.z)
					vel->p.z = sbd.vMinVel.p.z;
				else if (vel->p.z > sbd.vMaxVel.p.z)
					vel->p.z = sbd.vMaxVel.p.z;
				}
			}
		}
	else {
		fix xTotalDrag = f1_0;
		while (nTries--)
			xTotalDrag = FixMul (xTotalDrag, d);
		//do linear scale on remaining bit of time
		xTotalDrag = FixMul (xTotalDrag, f1_0-FixMul (k, xDrag));
		VmVecScale (&objP->mType.physInfo.velocity, xTotalDrag);
		}
	}

//moveIt:

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
do {
	bRetry = 0;
	//Move the tObject
	VmVecCopyScale (
		&vFrame, 
		&objP->mType.physInfo.velocity, 
		FixMulDiv (xSimTime, xTimeScale, 100 * (nBadSeg + 1)));
	if (!IsMultiGame) {
		int i = (objP != gameData.objs.console) ? 0 : 1;
		if (gameStates.gameplay.slowmo [i].fSpeed != 1) {
			VmVecScale (&vFrame, (fix) (F1_0 / gameStates.gameplay.slowmo [i].fSpeed));
			}
		}
	if (!(vFrame.p.x || vFrame.p.y || vFrame.p.z))
		break;

retryMove:

	nTries++;
	//	If retry count is getting large, then we are trying to do something stupid.
	if (nTries > 3) {
		if (objP->nType != OBJ_PLAYER)
			break;
		if (nTries > 8) {
			if (sbd.bBoosted)
				sbd.bBoosted = 0;
			break;
			}
		}
	VmVecAdd (&vNewPos, &objP->position.vPos, &vFrame);
#if 0
	iSeg = FindSegByPoint (&vNewPos, objP->nSegment);
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
	if ((bGetPhysSegs = (objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT)))
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
		fq.startSeg = FindSegByPoint (&vNewPos, objP->nSegment);
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
#ifdef _DEBUG
	else if (fviResult == HIT_WALL)
		fviResult = FindVectorIntersection (&fq, &hi);
#endif
	//	Matt: Mike's hack.
	else if (fviResult == HIT_OBJECT) {
		tObject	*hitObjP = gameData.objs.objects + hi.hit.nObject;

		if ((hitObjP->nType == OBJ_WEAPON) && ((hitObjP->id == PROXMINE_ID) || (hitObjP->id == SMARTMINE_ID)))
			nTries--;
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
				LogErr ("Invalid segment in segment list #1\n");
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
	if (GetSegMasks (&objP->position.vPos, objP->nSegment, 0).centerMask) {	//object stuck
		int n = FindObjectSeg (objP);
		if (n == -1) {
			if (bGetPhysSegs)
				n = FindSegByPoint (&objP->vLastPos, objP->nSegment);
			if (n == -1) {
				objP->position.vPos = objP->vLastPos;
				RelinkObject (nObject, objP->nSegment);
				}
			else {
				vmsVector vCenter;
				COMPUTE_SEGMENT_CENTER_I (&vCenter, objP->nSegment);
				VmVecDec (&vCenter, &objP->position.vPos);
				if (VmVecMag (&vCenter) > F1_0) {
					VmVecNormalize (&vCenter);
					VmVecScaleFrac (&vCenter, 1, 10);
					}
				VmVecDec (&objP->position.vPos, &vCenter);
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
		actualDist = VmVecNormalizedDir (&vMoveNormal, &objP->position.vPos, &vSavePos);
		if ((fviResult == HIT_WALL) && (VmVecDot (&vMoveNormal, &vFrame) < 0)) {		//moved backwards
			//don't change position or xSimTime
			objP->position.vPos = vSavePos;
			//iSeg = objP->nSegment;		//don't change tSegment
			if (nSaveSeg != iSeg)
				RelinkObject (nObject, nSaveSeg);
			if (bDoSpeedBoost) {
				objP->position.vPos = vStartPos;
				SetSpeedBoostVelocity (nObject, -1, -1, -1, -1, -1, &vStartPos, &sbd.vDest, 0);
				VmVecCopyScale (&vFrame, &sbd.vVel, xSimTime);
				goto retryMove;
				}
			xMovedTime = 0;
			}
		else {
			attemptedDist = VmVecMag (&vFrame);
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
		VmVecSub (&vMoved, &objP->position.vPos, &vSavePos);
		xWallPart = VmVecDot (&vMoved, &hi.hit.vNormal);
#ifdef _DEBUG
		if (objP->nType == OBJ_ROBOT)
			objP = objP;
#endif
		if (xWallPart && (xMovedTime > 0) && ((xHitSpeed = -FixDiv (xWallPart, xMovedTime)) > 0)) {
			CollideObjectWithWall (objP, xHitSpeed, nWallHitSeg, nWallHitSide, &hi.hit.vPoint);
#if 0//def _DEBUG
			if (objP->nType == OBJ_PLAYER)
				HUDMessage (0, "BUMP!");
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
#if UNSTICK_OBJECT == 2
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

			Assert (gameStates.app.cheats.bBouncingWeapons || ((objP->mType.physInfo.flags & (PF_STICK | PF_BOUNCE)) != (PF_STICK | PF_BOUNCE)));	//can't be bounce and stick
			bForceFieldBounce = (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nWallHitSeg].sides [nWallHitSide].nBaseTex].flags & TMI_FORCE_FIELD);
			if (!bForceFieldBounce && (objP->mType.physInfo.flags & PF_STICK)) {		//stop moving
				AddStuckObject (objP, nWallHitSeg, nWallHitSide);
				VmVecZero (&objP->mType.physInfo.velocity);
				bObjStopped = 1;
				bRetry = 0;
				}
			else {				// Slide tObject along tWall
				int bCheckVel = 0;
				//We're constrained by a wall, so subtract wall part from velocity vector
				xWallPart = VmVecDot (&hi.hit.vNormal, &objP->mType.physInfo.velocity);
				if (bForceFieldBounce || (objP->mType.physInfo.flags & PF_BOUNCE)) {		//bounce off tWall
					xWallPart *= 2;	//Subtract out wall part twice to achieve bounce
					if (bForceFieldBounce) {
						bCheckVel = 1;				//check for max velocity
						if (objP->nType == OBJ_PLAYER)
							xWallPart *= 2;		//tPlayer bounce twice as much
						}
					if (objP->mType.physInfo.flags & PF_BOUNCES_TWICE) {
						Assert (objP->mType.physInfo.flags & PF_BOUNCE);
						if (objP->mType.physInfo.flags & PF_BOUNCED_ONCE)
							objP->mType.physInfo.flags &= ~(PF_BOUNCE | PF_BOUNCED_ONCE | PF_BOUNCES_TWICE);
						else
							objP->mType.physInfo.flags |= PF_BOUNCED_ONCE;
						}
					bBounced = 1;		//this tObject bBounced
					}
				VmVecScaleInc (&objP->mType.physInfo.velocity, &hi.hit.vNormal, -xWallPart);
				if (bCheckVel) {
					fix vel = VmVecMag (&objP->mType.physInfo.velocity);
					if (vel > MAX_OBJECT_VEL)
						VmVecScale (&objP->mType.physInfo.velocity, FixDiv (MAX_OBJECT_VEL, vel));
					}
				if (bBounced && (objP->nType == OBJ_WEAPON))
					VmVector2Matrix (&objP->position.mOrient, &objP->mType.physInfo.velocity, &objP->position.mOrient.uVec, NULL);
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
		Assert (hi.hit.nObject != -1);
		ppos0 = &gameData.objs.objects [hi.hit.nObject].position.vPos;
		ppos1 = &objP->position.vPos;
		size0 = gameData.objs.objects [hi.hit.nObject].size;
		size1 = objP->size;
		//	Calculcate the hit point between the two objects.
		Assert (size0 + size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
		if (0 && EGI_FLAG (nHitboxes, 0, 0, 0)) 
			vHitPos = hi.hit.vPoint;
		else {
			VmVecSub (&vHitPos, ppos1, ppos0);
			VmVecScaleAdd (&vHitPos, ppos0, &vHitPos, FixDiv (size0, size0 + size1));
			}
		vOldVel = objP->mType.physInfo.velocity;
		CollideTwoObjects (objP, gameData.objs.objects + hi.hit.nObject, &vHitPos);
		if (sbd.bBoosted && (objP == gameData.objs.console))
			objP->mType.physInfo.velocity = vOldVel;
		// Let object continue its movement
		if (!(objP->flags & OF_SHOULD_BE_DEAD)) {
			if ((objP->mType.physInfo.flags & PF_PERSISTENT) || 
				 (vOldVel.p.x == objP->mType.physInfo.velocity.p.x && 
				  vOldVel.p.y == objP->mType.physInfo.velocity.p.y && 
				  vOldVel.p.z == objP->mType.physInfo.velocity.p.z)) {
				//if (gameData.objs.objects [hi.hit.nObject].nType == OBJ_POWERUP)
					gameData.physics.ignoreObjs [nIgnoreObjs++] = hi.hit.nObject;
				bRetry = 1;
				}
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
		Total_retries += nTries - 1;
		Total_sims++;
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

		VmVecSub (&vMoved, &objP->position.vPos, &vStartPos);
		s = VmVecMag (&vMoved);
		VmVecScale (&vMoved, FixMulDiv (FixDiv (F1_0, gameData.physics.xTime), xTimeScale, 100));
#if 1
		if (!bDoSpeedBoost)
			objP->mType.physInfo.velocity = vMoved;
#endif
#ifdef BUMP_HACK
		if ((objP == gameData.objs.console) && 
			 !(vMoved.p.x || vMoved.p.y || vMoved.p.z) &&
			 (objP->mType.physInfo.thrust.p.x || objP->mType.physInfo.thrust.p.y || objP->mType.physInfo.thrust.p.z)) {
			DoBumpHack (objP);
			}
#endif
		}

	if (objP->mType.physInfo.flags & PF_LEVELLING)
		DoPhysicsAlignObject (objP);
	//hack to keep tPlayer from going through closed doors
	if ((objP->nType == OBJ_PLAYER) && (objP->nSegment != nOrigSegment) && 
		 (gameStates.app.cheats.bPhysics != 0xBADA55)) {
		int nSide;
		nSide = FindConnectedSide (gameData.segs.segments + objP->nSegment, gameData.segs.segments + nOrigSegment);
		if (nSide != -1) {
			if (!(WALL_IS_DOORWAY (gameData.segs.segments + nOrigSegment, nSide, NULL) & WID_FLY_FLAG)) {
				tSide *sideP;
				int	nVertex, nFaces;
				fix	dist;
				int	vertex_list [6];

				//bump tObject back
				sideP = gameData.segs.segments [nOrigSegment].sides + nSide;
				if (nOrigSegment == -1)
					Error ("nOrigSegment == -1 in physics");
				CreateAbsVertexLists (&nFaces, vertex_list, nOrigSegment, nSide);
				//let'sideP pretend this tWall is not triangulated
				nVertex = vertex_list [0];
				if (nVertex > vertex_list [1])
					nVertex = vertex_list [1];
				if (nVertex > vertex_list [2])
					nVertex = vertex_list [2];
				if (nVertex > vertex_list [3])
					nVertex = vertex_list [3];
				dist = VmDistToPlane (&vStartPos, sideP->normals, gameData.segs.vertices + nVertex);
				VmVecScaleAdd (&objP->position.vPos, &vStartPos, sideP->normals, objP->size-dist);
				UpdateObjectSeg (objP);
				}
			}
		}

//if end point not in tSegment, move tObject to last pos, or tSegment center
if (GetSegMasks (&objP->position.vPos, objP->nSegment, 0).centerMask) {
	if (FindObjectSeg (objP) == -1) {
		int n;

		if ((objP->nType == OBJ_PLAYER) && (n = FindSegByPoint (&objP->vLastPos, objP->nSegment)) != -1) {
			objP->position.vPos = objP->vLastPos;
			RelinkObject (nObject, n);
			}
		else {
			COMPUTE_SEGMENT_CENTER_I (&objP->position.vPos, objP->nSegment);
			objP->position.vPos.p.x += nObject;
			}
		if (objP->nType == OBJ_WEAPON)
			KillObject (objP);
		}
	}
CATCH_OBJ (objP, objP->mType.physInfo.velocity.p.y == 0);
#if UNSTICK_OBJS
UnstickObject (objP);
#endif
}

//	----------------------------------------------------------------
//Applies an instantaneous force on an tObject, resulting in an instantaneous
//change in velocity.

void PhysApplyForce (tObject *objP, vmsVector *vForce)
{

	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass, 
	//	in collision with crazy reactor robot thing on d2levf-s.
if (objP->mType.physInfo.mass == 0)
	return;
if (objP->movementType != MT_PHYSICS)
	return;
if (gameStates.render.automap.bDisplay && (objP == gameData.objs.console))
	return;
#ifdef TACTILE
  if (TactileStick && (obj == gameData.objs.objects + LOCALPLAYER.nObject))
	Tactile_apply_force (vForce, &objP->position.mOrient);
#endif
//Add in acceleration due to force
if (!gameData.objs.speedBoost [OBJ_IDX (objP)].bBoosted || (objP != gameData.objs.console))
	VmVecScaleInc (&objP->mType.physInfo.velocity, 
						vForce, 
						FixDiv (f1_0, objP->mType.physInfo.mass));
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
	vmsVector	*pvRotVel = &objP->mType.physInfo.rotVel;

// Make this tObject turn towards the vGoal.  Changes orientation, doesn't change direction of movement.
// If no one moves, will be facing vGoal in 1 second.

//	Detect null vector.
if (gameStates.render.automap.bDisplay && (objP == gameData.objs.console))
	return;
if ((vGoal->p.x == 0) && (vGoal->p.y == 0) && (vGoal->p.z == 0))
	return;
//	Make morph gameData.objs.objects turn more slowly.
if (objP->controlType == CT_MORPH)
	rate *= 2;

VmExtractAnglesVector (&dest_angles, vGoal);
VmExtractAnglesVector (&cur_angles, &objP->position.mOrient.fVec);
delta_p = (dest_angles.p - cur_angles.p);
delta_h = (dest_angles.h - cur_angles.h);
if (delta_p > F1_0/2) 
	delta_p = dest_angles.p - cur_angles.p - F1_0;
if (delta_p < -F1_0/2) 
	delta_p = dest_angles.p - cur_angles.p + F1_0;
if (delta_h > F1_0/2) 
	delta_h = dest_angles.h - cur_angles.h - F1_0;
if (delta_h < -F1_0/2) 
	delta_h = dest_angles.h - cur_angles.h + F1_0;
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
PhysicsSetRotVelAndSaturate (&pvRotVel->p.x, delta_p);
PhysicsSetRotVelAndSaturate (&pvRotVel->p.y, delta_h);
pvRotVel->p.z = 0;
}

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an tObject, resulting in an instantaneous
//	change in orientation.
void PhysApplyRot (tObject *objP, vmsVector *vForce)
{
	fix	xRate, xMag;

if (objP->movementType != MT_PHYSICS)
	return;
xMag = VmVecMag (vForce)/8;
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

	VmVecCopyScale (&objP->mType.physInfo.thrust, &objP->mType.physInfo.velocity, k);

}


