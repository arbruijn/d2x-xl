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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "descent.h"
#include "objrender.h"
#include "lightning.h"
#include "trackobject.h"
#include "omega.h"
#include "segpoint.h"
#include "error.h"
#include "key.h"
#include "texmap.h"
#include "textures.h"
#include "rendermine.h"
#include "fireball.h"
#include "newdemo.h"
#include "timer.h"
#include "physics.h"
#include "segmath.h"
#include "input.h"
#include "dropobject.h"
#include "lightcluster.h"
#include "visibility.h"
#include "postprocessing.h"

void BlastNearbyGlass (CObject *pObj, fix damage);
void NDRecordGuidedEnd (void);

//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!!*****
//	Here are the constants you're looking for!--MK

#define	FULL_COCKPIT_OFFS 0
#define	LASER_OFFS	 (I2X (29) / 100)

#define	MAX_SMART_DISTANCE	I2X (150)
#define	MAX_TARGET_OBJS		30

//---------------------------------------------------------------------------------

void TrackWeaponObject (int16_t nObject, int32_t nOwner)
{
if (gameData.multigame.weapon.nFired [0] < sizeofa (gameData.multigame.weapon.nObjects)) {
	if (gameData.multigame.weapon.nFired [0] < gameData.multigame.weapon.nFired [1])
		SetObjNumMapping (nObject, gameData.multigame.weapon.nObjects [1][gameData.multigame.weapon.nFired [0]], nOwner);
	gameData.multigame.weapon.nObjects [0][gameData.multigame.weapon.nFired [0]++] = nObject;
	}
}

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// player and calls the appropriate routine.

void RenderLaser (CObject *pObj)
{

//	Commented out by John (sort of, typed by Mike) on 6/8/94
#if 0
	switch (pObj->info.nId) {
	case WEAPON_TYPE_WEAK_LASER:
	case WEAPON_TYPE_STRONG_LASER:
	case WEAPON_TYPE_CANNON_BALL:
	case WEAPON_TYPEMSL:
		break;
	default:
		Error ("Invalid weapon nType in RenderLaser\n");
	}
#endif

CWeaponInfo *pWeaponInfo = WEAPONINFO (pObj);

if (!pWeaponInfo) {
	Error ("Invalid weapon type %d in RenderLaser (object type: %d)\n", pObj->Id (), pObj->Type ());
	return;
	}

switch (pWeaponInfo->renderType) {
	case WEAPON_RENDER_LASER:
		// Not supported
		break;
	case WEAPON_RENDER_BLOB:
		DrawObjectBitmap (pObj, pWeaponInfo->bitmap.index, pWeaponInfo->bitmap.index, 0, NULL, 2.0f / 3.0f);
		break;
	case WEAPON_RENDER_POLYMODEL:
		break;
	case WEAPON_RENDER_VCLIP:
		Int3 ();	//	Oops, not supported, nType added by mk on 09/09/94, but not for lasers...
	default:
		Error ("Invalid weapon render type %d in RenderLaser\n", pWeaponInfo->renderType);
	}
}

//---------------------------------------------------------------------------------

static int32_t LaserCreationTimeout (int32_t nId, fix xCreationTime)
{
if (nId == PHOENIX_ID)
	return gameData.timeData.xGame > xCreationTime + (I2X (1) / 3) * gameStates.gameplay.slowmo [0].fSpeed;
else if (nId == GUIDEDMSL_ID)
	return gameData.timeData.xGame > xCreationTime + (I2X (1) / 2) * gameStates.gameplay.slowmo [0].fSpeed;
else if (CObject::IsPlayerMine (nId))
	return gameData.timeData.xGame > xCreationTime + (I2X (4)) * gameStates.gameplay.slowmo [0].fSpeed;
else
	return 0;
}

//---------------------------------------------------------------------------------
//	Changed by MK on 09/07/94
//	I want you to be able to blow up your own bombs.
//	AND...Your proximity bombs can blow you up if they're 2.0 seconds or more old.
//	Changed by MK on 06/06/95: Now must be 4.0 seconds old.  Much valid Net-complaining.

int32_t LasersAreRelated (int32_t o1, int32_t o2)
{
ENTER (0, 0);
	CObject	*pObj1, *pObj2;
	int16_t	id1, id2;
	fix		ct1, ct2;

if ((o1 < 0) || (o2 < 0))
	RETVAL (0)
pObj1 = OBJECT (o1);
pObj2 = OBJECT (o2);
id1 = pObj1->info.nId;
ct1 = pObj1->cType.laserInfo.xCreationTime;
// See if o2 is the parent of o1
if (pObj1->info.nType == OBJ_WEAPON)
	if ((pObj1->cType.laserInfo.parent.nObject == o2) &&
		 (pObj1->cType.laserInfo.parent.nSignature == pObj2->info.nSignature)) {
		//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is player, they are related only if o1 < 2.0 seconds old
		if (LaserCreationTimeout (id1, ct1))
			RETVAL (0)
		RETVAL (1)
		}

id2 = pObj2->info.nId;
ct2 = pObj2->cType.laserInfo.xCreationTime;
// See if o1 is the parent of o2
if (pObj2->info.nType == OBJ_WEAPON)
	if ((pObj2->cType.laserInfo.parent.nObject == o1) &&
		 (pObj2->cType.laserInfo.parent.nSignature == pObj1->info.nSignature)) {
		//	o2 is a weapon, o1 is the parent of 2, so if o2 is PROXIMITY_BOMB and o1 is player, they are related only if o1 < 2.0 seconds old
		if (LaserCreationTimeout (id2, ct2))
			RETVAL (0)
		RETVAL (1)
		}

// They must both be weapons
if ((pObj1->info.nType != OBJ_WEAPON) || (pObj2->info.nType != OBJ_WEAPON))
	RETVAL (0)

//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
// See if they're siblings...
//	MK: 06/08/95, Don't allow prox bombs to detonate for 3/4 second.  Else too likely to get toasted by your own bomb if hit by opponent.
if (pObj1->cType.laserInfo.parent.nSignature == pObj2->cType.laserInfo.parent.nSignature) {
	if ((id1 != PROXMINE_ID)  && (id2 != PROXMINE_ID) && (id1 != SMARTMINE_ID) && (id2 != SMARTMINE_ID))
		RETVAL (1)
	//	If neither is older than 1/2 second, then can't blow up!
	if ((gameData.timeData.xGame > (ct1 + I2X (1)/2) * gameStates.gameplay.slowmo [0].fSpeed) ||
		 (gameData.timeData.xGame > (ct2 + I2X (1)/2) * gameStates.gameplay.slowmo [0].fSpeed))
		RETVAL (0)
	RETVAL (1)
	}

//	Anything can cause a collision with a robot super prox mine.
if (CObject::IsMine (id1) || CObject::IsMine (id2))
	RETVAL (0)
if (!COMPETITION && EGI_FLAG (bKillMissiles, 0, 0, 0) && (CObject::IsMissile (id1) || CObject::IsMissile (id2)))
	RETVAL (0)
RETVAL (1)
}

//---------------------------------------------------------------------------------
//--unused-- int32_t Muzzle_scale=2;
void DoMuzzleStuff (int32_t nSegment, CFixVector *pos)
{
gameData.muzzleData.info [gameData.muzzleData.queueIndex].createTime = TimerGetFixedSeconds ();
gameData.muzzleData.info [gameData.muzzleData.queueIndex].nSegment = nSegment;
gameData.muzzleData.info [gameData.muzzleData.queueIndex].pos = *pos;
if (++gameData.muzzleData.queueIndex >= MUZZLE_QUEUE_MAX)
	gameData.muzzleData.queueIndex = 0;
}

//	-----------------------------------------------------------------------------------------------------------

CFixVector *GetGunPoints (CObject *pObj, int32_t nGun)
{
if (!pObj)
	return NULL;

	tGunInfo*	pGunInfo = gameData.modelData.gunInfo + pObj->ModelId ();
	CFixVector*	vDefaultGunPoints, *vGunPoints;
	int32_t		nDefaultGuns, nGuns;

if (pObj->info.nType == OBJ_PLAYER) {
	vDefaultGunPoints = gameData.pigData.ship.player->gunPoints;
	nDefaultGuns = N_PLAYER_GUNS;
	}
else if (pObj->info.nType == OBJ_ROBOT) {
	vDefaultGunPoints = ROBOTINFO (pObj) ? ROBOTINFO (pObj)->gunPoints : 0;
	nDefaultGuns = MAX_GUNS;
	}
else
	return NULL;
if (0 < (nGuns = pGunInfo->nGuns))
	vGunPoints = pGunInfo->vGunPoints;
else {
	if (!(nGuns = nDefaultGuns))
		return NULL;
	vGunPoints = vDefaultGunPoints;
	}
if (abs (nGun) >= nDefaultGuns)
	nGun = (nGun < 0) ? -(nDefaultGuns - 1) : nDefaultGuns - 1;
return vGunPoints;
}

//-------------- Initializes a laser after Fire is pressed -----------------

CFixVector *TransformGunPoint (CObject *pObj, CFixVector *vGunPoints, int32_t nGun, fix xDelay, uint8_t nLaserType, CFixVector *vMuzzle, CFixMatrix *pMatrix)
{
	int32_t					bSpectate = SPECTATOR (pObj);
	tObjTransformation*	pPos = bSpectate ? &gameStates.app.playerPos : &pObj->info.position;
	CFixMatrix				m, *pView;
	CFixVector				v [2];
#if FULL_COCKPIT_OFFS
	int32_t					bLaserOffs = ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) &&
												  (pObj->Index () == LOCALPLAYER.nObject));
#else
	int32_t					bLaserOffs = 0;
#endif

if (nGun < 0) {	// use center between gunPoints nGun and nGun + 1
	*v = vGunPoints [-nGun] + vGunPoints [-nGun - 1];
//	VmVecScale (VmVecAdd (v, vGunPoints - nGun, vGunPoints - nGun - 1), I2X (1) / 2);
	*v *= (I2X (1) / 2);
}
else {
	v [0] = vGunPoints [nGun];
	if (bLaserOffs)
		v [0] += pPos->mOrient.m.dir.u * LASER_OFFS;
	}
if (!pMatrix)
	pMatrix = &m;
if (bSpectate) {
   pView = pMatrix;
	*pView = pPos->mOrient.Transpose ();
}
else
   pView = pObj->View (0);
v[1] = *pView * v [0];
memcpy (pMatrix, &pPos->mOrient, sizeof (CFixMatrix));
if (nGun < 0)
	v[1] += (*pMatrix).m.dir.u * (-2 * v->Mag ());
(*vMuzzle) = pPos->vPos + v [1];
//	If supposed to fire at a delayed time (xDelay), then move this point backwards.
if (xDelay)
	*vMuzzle += pMatrix->m.dir.f * (-FixMul (xDelay, WI_Speed (nLaserType, gameStates.app.nDifficultyLevel)));
return vMuzzle;
}

//-------------- Initializes a laser after Fire is pressed -----------------

int32_t FireWeaponDelayedWithSpread (
	CObject*	pObj,
	uint8_t	nLaserType,
	int32_t	nGun,
	fix		xSpreadR,
	fix		xSpreadU,
	fix		xDelay,
	int32_t	bMakeSound,
	int32_t	bHarmless,
	int16_t	nLightObj)
{
ENTER (0, 0);
	int32_t					nPlayer = pObj->Id ();
	int16_t					nLaserSeg;
	int32_t					nFate;
	CFixVector				vLaserPos, vLaserDir, *vGunPoints;
	int32_t					nObject;
	CObject*					pWeapon;
#if FULL_COCKPIT_OFFS
	int32_t					bLaserOffs = ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) && (pObj->Index () == LOCALPLAYER.nObject));
#else
	int32_t bLaserOffs = 0;
#endif
	CFixMatrix				m;
	int32_t					bSpectate = SPECTATOR (pObj);
	tObjTransformation*	pPos = bSpectate ? &gameStates.app.playerPos : &pObj->info.position;

#if DBG
if (nLaserType == SMARTMINE_BLOB_ID)
	nLaserType = nLaserType;
#endif
CreateAwarenessEvent (pObj, PA_WEAPON_WALL_COLLISION);
// Find the initial vPosition of the laser
if (!(vGunPoints = GetGunPoints (pObj, nGun)))
	RETVAL (0)
TransformGunPoint (pObj, vGunPoints, nGun, xDelay, nLaserType, &vLaserPos, &m);

//--------------- Find vLaserPos and nLaserSeg ------------------
CHitQuery	hitQuery (FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS, &pPos->vPos, &vLaserPos,
							 (bSpectate ? gameStates.app.nPlayerSegment : pObj->info.nSegment), pObj->Index (),
							 0x10, 0x10, ++gameData.physicsData.bIgnoreObjFlag);
CHitResult	hitResult;

nFate = FindHitpoint (hitQuery, hitResult);
nLaserSeg = hitResult.nSegment;
if (nLaserSeg == -1) {	//some sort of annoying error
	RETVAL (-1)
	}
//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
if (CFixVector::Dist (vLaserPos, pPos->vPos) > 3 * pObj->info.xSize / 2) {
	RETVAL (-1)
	}
if (nFate == HIT_WALL) {
	RETVAL (-1)
	}
//	Now, make laser spread out.
vLaserDir = m.m.dir.f;
if (xSpreadR || xSpreadU) {
	vLaserDir += m.m.dir.r * xSpreadR;
	vLaserDir += m.m.dir.u * xSpreadU;
	}
if (bLaserOffs)
	vLaserDir += m.m.dir.u * LASER_OFFS;
nObject = CreateNewWeapon (&vLaserDir, &vLaserPos, nLaserSeg, pObj->Index (), nLaserType, bMakeSound);
pWeapon = OBJECT (nObject);
if (!pWeapon) {
#if DBG
	if (nObject > -1)
		BRP;
#endif
	RETVAL (-1)
	}
//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
if (nLaserType == OMEGA_ID)
	RETVAL (-1)
#if 0 // debug stuff
TrackWeaponObject (nObject, int32_t (nPlayer));
#endif
if ((nLaserType == GUIDEDMSL_ID) && gameData.multigame.bIsGuided)
	gameData.objData.SetGuidedMissile (nPlayer, pWeapon);
gameData.multigame.bIsGuided = 0;
if (CObject::IsMissile (nLaserType) && (nLaserType != GUIDEDMSL_ID)) {
	if (!gameData.objData.pMissileViewer && (nPlayer == N_LOCALPLAYER))
		gameData.objData.pMissileViewer = pWeapon;
	}
//	If this weapon is supposed to be silent, set that bit!
#if 0 
// the following code prevents sound from cutting out due to too many sound sources (e.g. laser bolts)
// the backside is that only one laser bolt from a round creates an impact sound
// turn on if sound cuts out during intense firefights
if (!bMakeSound)
	pWeapon->info.nFlags |= OF_SILENT;
#endif
//	If this weapon is supposed to be silent, set that bit!
if (bHarmless)
	pWeapon->info.nFlags |= OF_HARMLESS;

//	If the object firing the laser is the player, then indicate the laser object so robots can dodge.
//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
if ((nPlayer == N_LOCALPLAYER) && !pWeapon->IsPlayerMine ())
	gameStates.app.bPlayerFiredLaserThisFrame = nObject;

if (gameStates.app.cheats.bHomingWeapons || WI_HomingFlag (nLaserType)) {
	if (nPlayer == N_LOCALPLAYER) {
		pWeapon->cType.laserInfo.nHomingTarget = pWeapon->FindVisibleHomingTarget (vLaserPos, MAX_THREADS);
		gameData.multigame.weapon.nTrack = pWeapon->cType.laserInfo.nHomingTarget;
		}
	else {// Some other player shot the homing thing
		Assert (IsMultiGame);
		pWeapon->cType.laserInfo.nHomingTarget = gameData.multigame.weapon.nTrack;
		}
	}
lightClusterManager.Add (nObject, nLightObj);
RETVAL (nObject)
}

//	-----------------------------------------------------------------------------------------------------------

void CreateFlare (CObject *pObj)
{
ENTER (0, 0);
	fix	xEnergyUsage = WI_EnergyUsage (FLARE_ID);

if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsage = FixMul (xEnergyUsage, I2X (gameStates.app.nDifficultyLevel+2)/4);
LOCALPLAYER.UpdateEnergy (-xEnergyUsage);
LaserPlayerFire (pObj, FLARE_ID, 6, 1, 0, -1);
if (IsMultiGame) {
	gameData.multigame.weapon.bFired = 1;
	gameData.multigame.weapon.nGun = FLARE_ADJUST;
	gameData.multigame.weapon.nFlags = 0;
	gameData.multigame.weapon.nLevel = 0;
	MultiSendFire ();
	}
RETURN
}

//-------------------------------------------------------------------------------------------

static int32_t nMslSlowDown [4] = {1, 4, 3, 2};

float MissileSpeedScale (CObject *pObj)
{
	int32_t	i = extraGameInfo [IsMultiGame].nMslStartSpeed;

if (!i)
	return 1;
return nMslSlowDown [i] * X2F (gameData.timeData.xGame - pObj->CreationTime ());
}

//-------------------------------------------------------------------------------------------
//	Set object *pObj's orientation to (or towards if I'm ambitious) its velocity.

void HomingMissileTurnTowardsVelocity (CObject *pObj, CFixVector *vNormVel)
{
	CFixVector	vNewDir;
	fix 			frameTime;

frameTime = gameStates.limitFPS.bHomers ? MSEC2X (HOMING_WEAPON_FRAMETIME) : gameData.timeData.xFrame;
vNewDir = *vNormVel;
vNewDir *= ((fix) (frameTime * 16 / gameStates.gameplay.slowmo [0].fSpeed));
vNewDir += pObj->info.position.mOrient.m.dir.f;
CFixVector::Normalize (vNewDir);
pObj->info.position.mOrient = CFixMatrix::CreateF(vNewDir);
}

//-------------------------------------------------------------------------------------------

static inline fix HomingMslStraightTime (int32_t id)
{
if (!CObject::IsMissile (id))
	return HOMINGMSL_STRAIGHT_TIME;
if ((id == EARTHSHAKER_MEGA_ID) || (id == ROBOT_SHAKER_MEGA_ID))
	return HOMINGMSL_STRAIGHT_TIME;
return HOMINGMSL_STRAIGHT_TIME * nMslSlowDown [(int32_t) extraGameInfo [IsMultiGame].nMslStartSpeed];
}

//-------------------------------------------------------------------------------------------

bool CObject::RemoveWeapon (void)
{
ENTER (0, 0);
if (LifeLeft () == ONE_FRAME_TIME) {
	UpdateLife (IsMultiGame ? OMEGA_MULTI_LIFELEFT : 0);
	info.renderType = RT_NONE;
	RETVAL (true)
	}
if (LifeLeft () < 0) {		// We died of old age
	Die ();
	if (WI_DamageRadius (info.nId))
		ExplodeSplashDamageWeapon (info.position.vPos);
	RETVAL (true)
	}
//delete weapons that are not moving
fix xSpeed = mType.physInfo.velocity.Mag ();
if (!((gameData.appData.nFrameCount ^ info.nSignature) & 3) &&
		(info.nType == OBJ_WEAPON) && (info.nId != FLARE_ID) &&
		(WI_Speed (info.nId, gameStates.app.nDifficultyLevel) > 0) &&
		(xSpeed < I2X (2))) {
	ReleaseObject (Index ());
	RETVAL (true)
	}
RETVAL (false)
}

//-------------------------------------------------------------------------------------------
//sequence this weapon object for this _frame_ (underscores added here to aid MK in his searching!)
// This function is only called every 25 ms max. (-> updating at 40 fps or less) 

void CObject::UpdateHomingWeapon (int32_t nThread)
{
ENTER (0, 0);
for (fix xFrameTime = gameData.laserData.xUpdateTime; xFrameTime >= HOMING_WEAPON_FRAMETIME; xFrameTime -= HOMING_WEAPON_FRAMETIME) {
	CFixVector	vVecToObject, vNewVel;
	fix			dot = I2X (1);
	fix			speed, xMaxSpeed, xDist;
	int32_t		nObjId = info.nId;
	//	For first 1/2 second of life, missile flies straight.
	if (cType.laserInfo.xCreationTime + HomingMslStraightTime (nObjId) < gameData.timeData.xGame) {
		//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
		if (Bounces ())
			mType.physInfo.flags &= ~PF_BOUNCES;
		//	Make sure the CObject we are tracking is still trackable.
		int32_t nTarget = UpdateHomingTarget (cType.laserInfo.nHomingTarget, dot, nThread);
		CObject *pTarget = OBJECT (nTarget);
		if (pTarget) {
			if (nTarget == LOCALPLAYER.nObject) {
				fix xDistToTarget = CFixVector::Dist (info.position.vPos, OBJPOS (pTarget)->vPos);
				if ((xDistToTarget < LOCALPLAYER.homingObjectDist) || (LOCALPLAYER.homingObjectDist < 0))
					LOCALPLAYER.homingObjectDist = xDistToTarget;
				}
			vVecToObject = OBJPOS (pTarget)->vPos - info.position.vPos;
			xDist = CFixVector::Normalize (vVecToObject);
			vNewVel = mType.physInfo.velocity;
			speed = CFixVector::Normalize (vNewVel);
			xMaxSpeed = WI_Speed (info.nId, gameStates.app.nDifficultyLevel);
			if (speed + I2X (1) < xMaxSpeed) {
				speed += FixMul (xMaxSpeed, I2X (1) / 80);
				if (speed > xMaxSpeed)
					speed = xMaxSpeed;
				}
			if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (info.nId == EARTHSHAKER_MEGA_ID)) {
				fix h = (info.xLifeLeft + I2X (1) - 1) / I2X (1);

				if (h > 7)
					vVecToObject *= (I2X (1) / (h - 6));
				}
#if 0
			vVecToObject *= HomingMslScale ();
#endif
			vNewVel += vVecToObject;
			//	The boss' smart children track better...
			CWeaponInfo *pWeaponInfo = WEAPONINFO (info.nId);
			if (!pWeaponInfo || (pWeaponInfo->renderType != WEAPON_RENDER_POLYMODEL))
				vNewVel += vVecToObject;
			CFixVector::Normalize (vNewVel);
			//CFixVector vOldVel = mType.physInfo.velocity;
			CFixVector vTest = info.position.vPos + vNewVel * xDist;
			if (CanSeePoint (this, &info.position.vPos, &vTest, info.nSegment, 3 * info.xSize / 2, -1.0f, nThread))
				/*mType.physInfo.velocity = vOldVel;
			else*/ {	//	Subtract off life proportional to amount turned. For hardest turn, it will lose 2 seconds per second.
				mType.physInfo.velocity = vNewVel;
				mType.physInfo.velocity *= speed;
				dot = abs (I2X (1) - dot);
				info.xLifeLeft -= FixMul (dot * 32, I2X (1) / 40);
				//	Only polygon OBJECTS have visible orientation, so only they should turn.
				if (pWeaponInfo && (pWeaponInfo->renderType == WEAPON_RENDER_POLYMODEL))
					HomingMissileTurnTowardsVelocity (this, &vNewVel);		//	vNewVel is normalized velocity.
				}
			}
		}
	}
UpdateWeaponSpeed ();
RETURN
}

//-------------------------------------------------------------------------------------------
//	Make sure weapon is not moving faster than allowed speed.

void CObject::UpdateWeaponSpeed (void)
{
fix xSpeed = mType.physInfo.velocity.Mag ();
if ((info.nType == OBJ_WEAPON) && (xSpeed > WI_Speed (info.nId, gameStates.app.nDifficultyLevel))) {
	//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
	if (WI_Speed (info.nId, gameStates.app.nDifficultyLevel)) {
		fix xScaleFactor = FixDiv (WI_Speed (info.nId, gameStates.app.nDifficultyLevel), xSpeed);
		mType.physInfo.velocity *= xScaleFactor;
		}
	}
}

//-------------------------------------------------------------------------------------------
//sequence this weapon object for this _frame_ (underscores added here to aid MK in his searching!)
void CObject::UpdateWeapon (void)
{
Assert (info.controlType == CT_WEAPON);
//	Ok, this is a big hack by MK.
//	If you want an CObject to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME
if (RemoveWeapon ())
	return;
if (info.nType == OBJ_WEAPON) {
	if (info.nId == FUSION_ID) {		//always set fusion weapon to max vel
		CFixVector::Normalize (mType.physInfo.velocity);
		mType.physInfo.velocity *= (WI_Speed (info.nId, gameStates.app.nDifficultyLevel));
		}
	//	For homing missiles, turn towards target. (unless it's a guided missile still under player control)
	if ((gameOpts->legacy.bHomers || (gameData.laserData.xUpdateTime >= HOMING_WEAPON_FRAMETIME)) && // limit update frequency to 40 fps
	    (gameStates.app.cheats.bHomingWeapons || WI_HomingFlag (info.nId)) &&
		 !(info.nFlags & PF_BOUNCED_ONCE) &&
		!IsGuidedMissile ()) {
#if USE_OPENMP //> 1
		if (gameStates.app.bMultiThreaded)
			gameData.objData.update.Push (this);
		else
#endif
			UpdateHomingWeapon ();
		return;
		}
	}
UpdateWeaponSpeed ();
}

//	--------------------------------------------------------------------------------------------------

void StopPrimaryFire (void)
{
#if 0
gameData.laserData.xNextFireTime = gameData.timeData.xGame;	//	Prevents shots-to-fire from building up.
#else
if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
	gameData.laserData.xNextFireTime = gameData.timeData.xGame + I2X (1) / 25;
else
	gameData.laserData.xNextFireTime = gameData.timeData.xGame + WI_FireWait (gameData.weaponData.nPrimary);
#endif
gameData.laserData.nGlobalFiringCount = 0;
controls.StopPrimaryFire ();
gameData.weaponData.firing [0].nStart =
gameData.weaponData.firing [0].nStop =
gameData.weaponData.firing [0].nDuration = 0;
}

//	--------------------------------------------------------------------------------------------------

void StopSecondaryFire (void)
{
gameData.missileData.xNextFireTime = gameData.timeData.xGame;	//	Prevents shots-to-fire from building up.
gameData.missileData.nGlobalFiringCount = 0;
controls.StopSecondaryFire ();
}

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console player, not for network players

int32_t LocalPlayerFireGun (void)
{
ENTER (0, 0);
	CPlayerData*	pPlayer = gameData.multiplayer.players + N_LOCALPLAYER;
	CObject*			pObj = OBJECT (pPlayer->nObject);
	fix				xEnergyUsed;
	int32_t			nAmmoUsed, nPrimaryAmmo;
	int32_t			nWeaponIndex;
	int32_t			rVal = 0;
	int32_t 			nRoundsPerShot = 1;
	int32_t			bGatling = (gameData.weaponData.nPrimary == GAUSS_INDEX) ? 2 : (gameData.weaponData.nPrimary == VULCAN_INDEX) ? 1 : 0;
	fix				addval;
	static int32_t	nSpreadfireToggle = 0;
	static int32_t	nHelixOrient = 0;

if (gameStates.app.bPlayerIsDead || OBSERVING)
	RETVAL (0)
if (gameStates.app.bD2XLevel && (SEGMENT (pObj->info.nSegment)->HasNoDamageProp ()))
	RETVAL (0)
nWeaponIndex = primaryWeaponToWeaponInfo [gameData.weaponData.nPrimary];
xEnergyUsed = WI_EnergyUsage (nWeaponIndex);
if (gameData.weaponData.nPrimary == OMEGA_INDEX)
	xEnergyUsed = 0;	//	Omega consumes energy when recharging, not when firing.
if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsed = FixMul (xEnergyUsed, I2X (gameStates.app.nDifficultyLevel + 2) / 4);
//	MK, 01/26/96, Helix use 2x energy in multiplayer.  bitmaps.tbl parm should have been reduced for single player.
//if (nWeaponIndex == FUSION_ID)
//	postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), I2X (1) / 3, pObj->info.xSize, 1, OBJPOS (pObj)->vPos + OBJPOS (pObj)->mOrient.m.dir.f * pObj->info.xSize));
else if (nWeaponIndex == HELIX_ID) {
	if (IsMultiGame)
		xEnergyUsed *= 2;
	}
nAmmoUsed = WI_AmmoUsage (nWeaponIndex);
addval = 2 * gameData.timeData.xFrame;
if (addval > I2X (1))
	addval = I2X (1);
if (!bGatling)
	nPrimaryAmmo = pPlayer->primaryAmmo [gameData.weaponData.nPrimary];
else {
	if ((gameOpts->UseHiresSound () == 2) && /*gameOpts->sound.bGatling &&*/
		 gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bGatlingSpeedUp, 1, 0, 0) &&
		 (gameData.weaponData.firing [0].nDuration < GATLING_DELAY))
		RETVAL (0)
	nPrimaryAmmo = pPlayer->primaryAmmo [VULCAN_INDEX];
	}
if	 ((pPlayer->energy < xEnergyUsed) || (nPrimaryAmmo < nAmmoUsed))
	AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.

if ((gameData.laserData.xLastFiredTime + 2 * gameData.timeData.xFrame < gameData.timeData.xGame) ||
	 (gameData.timeData.xGame < gameData.laserData.xLastFiredTime))
	gameData.laserData.xNextFireTime = gameData.timeData.xGame;
gameData.laserData.xLastFiredTime = gameData.timeData.xGame;

while (gameData.laserData.xNextFireTime <= gameData.timeData.xGame) {
	if	((pPlayer->energy >= xEnergyUsed) && (nPrimaryAmmo >= nAmmoUsed)) {
			int32_t nLaserLevel, flags = 0;

		if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
			gameData.laserData.xNextFireTime += I2X (1) / 25;
		else
			gameData.laserData.xNextFireTime += WI_FireWait (nWeaponIndex);
		nLaserLevel = LOCALPLAYER.LaserLevel ();
		if (gameData.weaponData.nPrimary == SPREADFIRE_INDEX) {
			if (nSpreadfireToggle)
				flags |= LASER_SPREADFIRE_TOGGLED;
			nSpreadfireToggle = !nSpreadfireToggle;
			}
		if (gameData.weaponData.nPrimary == HELIX_INDEX) {
			nHelixOrient++;
			flags |= ((nHelixOrient & LASER_HELIX_MASK) << LASER_HELIX_SHIFT);
			}
		if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
			flags |= LASER_QUAD;
#if 1
		int32_t fired = FireWeapon ((int16_t) LOCALPLAYER.nObject, (uint8_t) gameData.weaponData.nPrimary, nLaserLevel, flags, nRoundsPerShot);
		if (fired) {
			rVal += fired;
			if (bGatling) {
				if (nAmmoUsed > pPlayer->primaryAmmo [VULCAN_INDEX])
					nAmmoUsed = pPlayer->primaryAmmo [VULCAN_INDEX];
				pPlayer->primaryAmmo [VULCAN_INDEX] -= nAmmoUsed;
				gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed += nAmmoUsed;
				if (gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed >= VULCAN_CLIP_CAPACITY) {
					if (gameStates.app.bHaveExtraGameInfo [IsMultiGame])
						MaybeDropNetPowerup (-1, POW_VULCAN_AMMO, FORCE_DROP);
					gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed -= VULCAN_CLIP_CAPACITY;
					}
				MultiSendAmmo ();
				}
			else {
				pPlayer->UpdateEnergy (-(xEnergyUsed * fired) / WI_FireCount (nWeaponIndex));
				}
			}
#else
		rVal += FireWeapon ((int16_t) LOCALPLAYER.nObject, (uint8_t) gameData.weaponData.nPrimary, nLaserLevel, flags, nRoundsPerShot);
		pPlayer->UpdateEnergy (-(xEnergyUsed * rVal) / WI_FireCount (nWeaponIndex));
		if (rVal && ((gameData.weaponData.nPrimary == VULCAN_INDEX) || (gameData.weaponData.nPrimary == GAUSS_INDEX))) {
			if (nAmmoUsed > pPlayer->primaryAmmo [VULCAN_INDEX])
				pPlayer->primaryAmmo [VULCAN_INDEX] = 0;
			else
				pPlayer->primaryAmmo [VULCAN_INDEX] -= nAmmoUsed;
			}
#endif
		AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.
		}
	else {
		AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.
		StopPrimaryFire ();
		break;	//	Couldn't fire weapon, so abort.
		}
	}
gameData.laserData.nGlobalFiringCount = 0;
RETVAL (rVal)
}

//	-------------------------------------------------------------------------------------------
//	if nGoalObj == -1, then create Random vector
int32_t CreateHomingWeapon (CObject *pObj, int32_t nGoalObj, uint8_t objType, int32_t bMakeSound)
{
ENTER (0, 0);
	int16_t		nObject;
	CFixVector	vGoal, vRandom;
	CObject		*pGoalObj = OBJECT (nGoalObj);

if (!pGoalObj)
	vGoal = CFixVector::Random ();
else { //	Create a vector towards the goal, then add some noise to it.
	CFixVector::NormalizedDir (vGoal, pGoalObj->info.position.vPos, pObj->info.position.vPos);
	vRandom = CFixVector::Random ();
	vGoal += vRandom * (I2X (1)/4);
	CFixVector::Normalize (vGoal);
	}
nObject = CreateNewWeapon (&vGoal, &pObj->info.position.vPos, pObj->info.nSegment, pObj->Index (), objType, bMakeSound);
pObj = OBJECT (nObject);
if (!pObj)
	RETVAL (-1)
pObj->cType.laserInfo.nHomingTarget = nGoalObj;
RETVAL (nObject)
}

//-----------------------------------------------------------------------------
// Create the children of a smart bomb, which is a bunch of homing missiles.
void CreateSmartChildren (CObject *pObj, int32_t nSmartChildren)
{
ENTER (0, 0);
	tParentInfo		parent;
	int32_t			bMakeSound;
	int32_t			nObjects = 0;
	int32_t			targetList [MAX_TARGET_OBJS];
	uint8_t			nBlobId, 
						nObjType = pObj->info.nType, 
						nObjId = pObj->info.nId;

if (nObjType == OBJ_WEAPON) {
	parent = pObj->cType.laserInfo.parent;
	}
else if (nObjType == OBJ_ROBOT) {
	parent.nType = OBJ_ROBOT;
	parent.nObject = pObj->Index ();
	}
else {
	Int3 ();	//	Hey, what kind of CObject is this!?
	parent.nType = 0;
	parent.nObject = 0;
	}

CWeaponInfo *pWeaponInfo = WEAPONINFO (nObjId);

if (nObjType == OBJ_WEAPON) {
	if (!pWeaponInfo || (pWeaponInfo->children < 1))
		RETURN
	if (nObjId == EARTHSHAKER_ID)
		BlastNearbyGlass (pObj, WI_Strength (EARTHSHAKER_ID, gameStates.app.nDifficultyLevel));
	}
else if (nObjType != OBJ_ROBOT) 
	RETURN

CObject	*pTarget;
int32_t	i, nObject;

if (IsMultiGame)
	gameStates.app.SRand (8321L);

FORALL_OBJS (pTarget) {
	nObject = OBJ_IDX (pTarget);
	if (pTarget->info.nType == OBJ_PLAYER) {
		if (nObject == parent.nObject)
			continue;
		if ((parent.nType == OBJ_PLAYER) && (IsCoopGame))
			continue;
		if (IsTeamGame && (GetTeam (pTarget->info.nId) == GetTeam (OBJECT (parent.nObject)->info.nId)))
			continue;
		if (PLAYER (pTarget->info.nId).flags & PLAYER_FLAGS_CLOAKED)
			continue;
		}
	else if (pTarget->info.nType == OBJ_ROBOT) {
		if (pTarget->cType.aiInfo.CLOAKED)
			continue;
		if (parent.nType == OBJ_ROBOT)	//	Robot blobs can't track robots.
			continue;
		if ((parent.nType == OBJ_PLAYER) &&	pTarget->IsGuideBot ())	// Your shots won't track the buddy.
			continue;
		}
	else
		continue;
	fix dist = CFixVector::Dist (pObj->info.position.vPos, pTarget->info.position.vPos);
	if ((dist < MAX_SMART_DISTANCE) && ObjectToObjectVisibility (pObj, pTarget, FQ_TRANSWALL, -1.0f)) {
		targetList [nObjects++] = nObject;
		if (nObjects >= MAX_TARGET_OBJS)
			break;
		}
	}
//	Get type of weapon for child from parent.
if (nObjType == OBJ_WEAPON) {
	if (!pWeaponInfo)
		RETURN
	nBlobId = pWeaponInfo->children;
	}
else {
	Assert (nObjType == OBJ_ROBOT);
	nBlobId = ROBOT_SMARTMSL_BLOB_ID;
	}

bMakeSound = 1;
for (i = 0; i < nSmartChildren; i++) {
	int16_t nTarget = nObjects ? targetList [Rand (nObjects)] : -1;
	CreateHomingWeapon (pObj, nTarget, nBlobId, bMakeSound);
	bMakeSound = 0;
	}
RETURN
}

//	-------------------------------------------------------------------------------------------

//give up control of the guided missile
void ReleaseGuidedMissile (int32_t nPlayer)
{
ENTER (0, 0);
if (nPlayer == N_LOCALPLAYER) {
	CObject* gmObjP = gameData.objData.GetGuidedMissile (nPlayer);
	if (!gmObjP)
		RETURN
	gameData.objData.pMissileViewer = gmObjP;
	if (IsMultiGame)
	 	MultiSendGuidedInfo (gmObjP, 1);
	if (gameData.demoData.nState == ND_STATE_RECORDING)
	 	NDRecordGuidedEnd ();
	 }
gameData.objData.SetGuidedMissile (nPlayer, NULL);
RETURN
}

//	-------------------------------------------------------------------------------------------
//parameter determines whether or not to do autoselect if have run out of ammo
//this is needed because if you drop a bomb with the B key, you don't
//want to autoselect if the bomb isn't actually selected.
void DoMissileFiring (int32_t bAutoSelect)
{
ENTER (0, 0);
	int32_t			i, gunFlag = 0;
	CPlayerData*	pPlayer = gameData.multiplayer.players + N_LOCALPLAYER;

Assert (gameData.weaponData.nSecondary < MAX_SECONDARY_WEAPONS);
if (gameData.objData.HasGuidedMissile (N_LOCALPLAYER)) {
	ReleaseGuidedMissile (N_LOCALPLAYER);
	i = secondaryWeaponToWeaponInfo [gameData.weaponData.nSecondary];
	gameData.missileData.xNextFireTime = gameData.timeData.xGame + WI_FireWait (i);
	RETURN
	}

if (gameStates.app.bPlayerIsDead || OBSERVING || (pPlayer->secondaryAmmo [gameData.weaponData.nSecondary] <= 0))
	RETURN

uint8_t nWeaponId = secondaryWeaponToWeaponInfo [gameData.weaponData.nSecondary];
if ((nWeaponId == PROXMINE_ID) && IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) &&
	 (CountPlayerObjects (N_LOCALPLAYER, OBJ_WEAPON, PROXMINE_ID) >= extraGameInfo [IsMultiGame].nMaxSmokeGrenades))
	RETURN
if (gameStates.app.cheats.bLaserRapidFire != 0xBADA55)
	gameData.missileData.xNextFireTime = gameData.timeData.xGame + WI_FireWait (nWeaponId);
else
	gameData.missileData.xNextFireTime = gameData.timeData.xGame + I2X (1)/25;
gameData.missileData.xLastFiredTime = gameData.timeData.xGame;
int32_t nGun = secondaryWeaponToGunNum [gameData.weaponData.nSecondary];
int32_t h = !COMPETITION && (EGI_FLAG (bDualMissileLaunch, 0, 1, 0)) ? 1 : 0;
for (i = 0; (i <= h) && (pPlayer->secondaryAmmo [gameData.weaponData.nSecondary] > 0); i++) {
	pPlayer->secondaryAmmo [gameData.weaponData.nSecondary]--;
	if (nGun == 4) {		//alternate left/right
		nGun += (gunFlag = (gameData.laserData.nMissileGun & 1));
		gameData.laserData.nMissileGun++;
		}
	int32_t nObject = LaserPlayerFire (gameData.objData.pConsole, nWeaponId, nGun, 1, 0, -1);
	if (0 > nObject)
		RETURN
	if (gameData.weaponData.nSecondary == PROXMINE_INDEX) {
		if (!gameData.appData.GameMode (GM_HOARD | GM_ENTROPY)) {
			if (++gameData.laserData.nProximityDropped == 4) {
				gameData.laserData.nProximityDropped = 0;
				MaybeDropNetPowerup (nObject, POW_PROXMINE, INIT_DROP);
				}
			}
		break; //no dual prox bomb drop
		}
	else if (gameData.weaponData.nSecondary == SMARTMINE_INDEX) {
		if (!IsEntropyGame) {
			if (++gameData.laserData.nSmartMinesDropped == 4) {
				gameData.laserData.nSmartMinesDropped = 0;
				MaybeDropNetPowerup (nObject, POW_SMARTMINE, INIT_DROP);
				}
			}
		break; //no dual smartmine drop
		}
	else if (gameData.weaponData.nSecondary != CONCUSSION_INDEX)
		MaybeDropNetPowerup (nObject, secondaryWeaponToPowerup [0][gameData.weaponData.nSecondary], INIT_DROP);
	else {
		if (gameData.multiplayer.weaponStates [N_LOCALPLAYER].nBuiltinMissiles)
			gameData.multiplayer.weaponStates [N_LOCALPLAYER].nBuiltinMissiles--;
		else
			MaybeDropNetPowerup (nObject, secondaryWeaponToPowerup [0][gameData.weaponData.nSecondary], INIT_DROP);
		}
	if ((gameData.weaponData.nSecondary == GUIDED_INDEX) || (gameData.weaponData.nSecondary == SMART_INDEX))
		break;
	else if ((gameData.weaponData.nSecondary == MEGA_INDEX) || (gameData.weaponData.nSecondary == EARTHSHAKER_INDEX)) {
		CFixVector vForce;

	vForce.v.coord.x = - (gameData.objData.pConsole->info.position.mOrient.m.dir.f.v.coord.x << 7);
	vForce.v.coord.y = - (gameData.objData.pConsole->info.position.mOrient.m.dir.f.v.coord.y << 7);
	vForce.v.coord.z = - (gameData.objData.pConsole->info.position.mOrient.m.dir.f.v.coord.z << 7);
	gameData.objData.pConsole->ApplyForce (vForce);
	vForce.v.coord.x = (vForce.v.coord.x >> 4) + SRandShort ();
	vForce.v.coord.y = (vForce.v.coord.y >> 4) + SRandShort ();
	vForce.v.coord.z = (vForce.v.coord.z >> 4) + SRandShort ();
	gameData.objData.pConsole->ApplyRotForce (vForce);
	break; //no dual mega/smart missile launch
	}
}

if (IsMultiGame) {
	gameData.multigame.weapon.bFired = 1;		//how many
	gameData.multigame.weapon.nGun = gameData.weaponData.nSecondary + MISSILE_ADJUST;
	gameData.multigame.weapon.nFlags = gunFlag;
	gameData.multigame.weapon.nLevel = 0;
	MultiSendFire ();
	}
if (bAutoSelect)
	AutoSelectWeapon (1, 1);		//select next missile, if this one out of ammo
RETURN
}

// -----------------------------------------------------------------------------

void GetPlayerMslLock (void)
{
ENTER (0, 0);
if (gameStates.app.bPlayerIsDead || OBSERVING)
	RETURN

	int32_t		nWeapon, nObject, nGun, h, i, j;
	CFixVector	*vGunPoints, vGunPos;
	CFixMatrix	*pView;
	CObject		*pObj;

gameData.objData.trackGoals [0] =
gameData.objData.trackGoals [1] = NULL;
if ((pObj = GuidedInMainView ())) {
	nObject = pObj->FindVisibleHomingTarget (pObj->info.position.vPos, MAX_THREADS);
	gameData.objData.trackGoals [0] =
	gameData.objData.trackGoals [1] = (nObject < 0) ? NULL : OBJECT (nObject);
	RETURN
	}
if (!AllowedToFireMissile (-1, 1))
	RETURN
if (!EGI_FLAG (bMslLockIndicators, 0, 1, 0) || COMPETITION)
	RETURN
nWeapon = secondaryWeaponToWeaponInfo [gameData.weaponData.nSecondary];
if (LOCALPLAYER.secondaryAmmo [gameData.weaponData.nSecondary] <= 0)
	RETURN
if (!gameStates.app.cheats.bHomingWeapons &&
	 (nWeapon != HOMINGMSL_ID) && (nWeapon != MEGAMSL_ID) && (nWeapon != GUIDEDMSL_ID))
	RETURN
//pnt = gameData.pigData.ship.player->gunPoints [nGun];
if (nWeapon == MEGAMSL_ID) {
	j = 1;
	h = 0;
	}
else {
	j = !COMPETITION && (EGI_FLAG (bDualMissileLaunch, 0, 1, 0)) ? 2 : 1;
	h = gameData.laserData.nMissileGun & 1;
	}
pView = gameData.objData.pConsole->View (0);
for (i = 0; i < j; i++, h = !h) {
	nGun = secondaryWeaponToGunNum [gameData.weaponData.nSecondary] + h;
	if ((vGunPoints = GetGunPoints (gameData.objData.pConsole, nGun))) {
		vGunPos = vGunPoints [nGun];
		vGunPos = *pView * vGunPos;
		vGunPos += gameData.objData.pConsole->info.position.vPos;
		nObject = gameData.objData.pConsole->FindVisibleHomingTarget (vGunPos, MAX_THREADS);
		gameData.objData.trackGoals [i] = (nObject < 0) ? NULL : OBJECT (nObject);
		}
	}
RETURN
}

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireGun (void)
{
ENTER (0, 0);
	int32_t i = primaryWeaponToWeaponInfo [gameData.weaponData.nPrimary];

if (gameData.weaponData.firing [0].nDuration)
	gameData.laserData.nGlobalFiringCount += WI_FireCount (i);
if ((gameData.weaponData.nPrimary == FUSION_INDEX) && gameData.laserData.nGlobalFiringCount)
	ChargeFusion ();
RETURN
}

//	-------------------------------------------------------------------------------------------
// eof
