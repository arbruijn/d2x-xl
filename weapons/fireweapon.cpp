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

#ifdef TACTILE
#include "tactile.h"
#endif

void BlastNearbyGlass (CObject *objP, fix damage);
void NDRecordGuidedEnd (void);

//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!!*****
//	Here are the constants you're looking for!--MK

#define	FULL_COCKPIT_OFFS 0
#define	LASER_OFFS	 (I2X (29) / 100)

static   int nMslTurnSpeeds [3] = {I2X (1) / 2, 3 * I2X (1) / 4, 5 * I2X (1) / 4};

#define	HOMINGMSL_SCALE		nMslTurnSpeeds [(IsMultiGame && !IsCoopGame) ? 2 : extraGameInfo [IsMultiGame].nMslTurnSpeed]

#define	MAX_SMART_DISTANCE	I2X (150)
#define	MAX_OBJDISTS			30

//---------------------------------------------------------------------------------

inline int HomingMslScale (void)
{
return (int) (HOMINGMSL_SCALE / sqrt (gameStates.gameplay.slowmo [0].fSpeed));
}

//---------------------------------------------------------------------------------

void TrackWeaponObject (short nObject, int nOwner)
{
if (gameData.multigame.laser.nFired [0] < sizeofa (gameData.multigame.laser.nObjects)) {
	if (gameData.multigame.laser.nFired [0] < gameData.multigame.laser.nFired [1])
		MapObjnumLocalToRemote (nObject, gameData.multigame.laser.nObjects [1][gameData.multigame.laser.nFired [0]], nOwner);
	gameData.multigame.laser.nObjects [0][gameData.multigame.laser.nFired [0]++] = nObject;
	}
}

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// player and calls the appropriate routine.

void RenderLaser (CObject *objP)
{

//	Commented out by John (sort of, typed by Mike) on 6/8/94
#if 0
	switch (objP->info.nId) {
	case WEAPON_TYPE_WEAK_LASER:
	case WEAPON_TYPE_STRONG_LASER:
	case WEAPON_TYPE_CANNON_BALL:
	case WEAPON_TYPEMSL:
		break;
	default:
		Error ("Invalid weapon nType in RenderLaser\n");
	}
#endif

switch (gameData.weapons.info [objP->info.nId].renderType) {
	case WEAPON_RENDER_LASER:
		Int3 ();	// Not supported anymore!
					//Laser_draw_one (objP->Index (), gameData.weapons.info [objP->info.nId].bitmap);
		break;
	case WEAPON_RENDER_BLOB:
		DrawObjectBitmap (objP, gameData.weapons.info [objP->info.nId].bitmap.index, gameData.weapons.info [objP->info.nId].bitmap.index, 0, NULL, 2.0f / 3.0f);
		break;
	case WEAPON_RENDER_POLYMODEL:
		break;
	case WEAPON_RENDER_VCLIP:
		Int3 ();	//	Oops, not supported, nType added by mk on 09/09/94, but not for lasers...
	default:
		Error ("Invalid weapon render nType in RenderLaser\n");
	}
}

//---------------------------------------------------------------------------------

static int LaserCreationTimeout (int nId, fix xCreationTime)
{
if (nId == PHOENIX_ID)
	return gameData.time.xGame > xCreationTime + (I2X (1) / 3) * gameStates.gameplay.slowmo [0].fSpeed;
else if (nId == GUIDEDMSL_ID)
	return gameData.time.xGame > xCreationTime + (I2X (1) / 2) * gameStates.gameplay.slowmo [0].fSpeed;
else if (CObject::IsPlayerMine (nId))
	return gameData.time.xGame > xCreationTime + (I2X (4)) * gameStates.gameplay.slowmo [0].fSpeed;
else
	return 0;
}

//---------------------------------------------------------------------------------
//	Changed by MK on 09/07/94
//	I want you to be able to blow up your own bombs.
//	AND...Your proximity bombs can blow you up if they're 2.0 seconds or more old.
//	Changed by MK on 06/06/95: Now must be 4.0 seconds old.  Much valid Net-complaining.

int LasersAreRelated (int o1, int o2)
{
	CObject	*objP1, *objP2;
	short		id1, id2;
	fix		ct1, ct2;

if ((o1 < 0) || (o2 < 0))
	return 0;
objP1 = OBJECTS + o1;
objP2 = OBJECTS + o2;
id1 = objP1->info.nId;
ct1 = objP1->cType.laserInfo.xCreationTime;
// See if o2 is the parent of o1
if (objP1->info.nType == OBJ_WEAPON)
	if ((objP1->cType.laserInfo.parent.nObject == o2) &&
		 (objP1->cType.laserInfo.parent.nSignature == objP2->info.nSignature)) {
		//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is player, they are related only if o1 < 2.0 seconds old
		if (LaserCreationTimeout (id1, ct1))
			return 0;
		return 1;
		}

id2 = objP2->info.nId;
ct2 = objP2->cType.laserInfo.xCreationTime;
// See if o1 is the parent of o2
if (objP2->info.nType == OBJ_WEAPON)
	if ((objP2->cType.laserInfo.parent.nObject == o1) &&
		 (objP2->cType.laserInfo.parent.nSignature == objP1->info.nSignature)) {
		//	o2 is a weapon, o1 is the parent of 2, so if o2 is PROXIMITY_BOMB and o1 is player, they are related only if o1 < 2.0 seconds old
		if (LaserCreationTimeout (id2, ct2))
			return 0;
		return 1;
		}

// They must both be weapons
if ((objP1->info.nType != OBJ_WEAPON) || (objP2->info.nType != OBJ_WEAPON))
	return 0;

//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
// See if they're siblings...
//	MK: 06/08/95, Don't allow prox bombs to detonate for 3/4 second.  Else too likely to get toasted by your own bomb if hit by opponent.
if (objP1->cType.laserInfo.parent.nSignature == objP2->cType.laserInfo.parent.nSignature) {
	if ((id1 != PROXMINE_ID)  && (id2 != PROXMINE_ID) && (id1 != SMARTMINE_ID) && (id2 != SMARTMINE_ID))
		return 1;
	//	If neither is older than 1/2 second, then can't blow up!
	if ((gameData.time.xGame > (ct1 + I2X (1)/2) * gameStates.gameplay.slowmo [0].fSpeed) ||
		 (gameData.time.xGame > (ct2 + I2X (1)/2) * gameStates.gameplay.slowmo [0].fSpeed))
		return 0;
	return 1;
	}

//	Anything can cause a collision with a robot super prox mine.
if (CObject::IsMine (id1) || CObject::IsMine (id2))
	return 0;
if (!COMPETITION && EGI_FLAG (bKillMissiles, 0, 0, 0) && (CObject::IsMissile (id1) || CObject::IsMissile (id2)))
	return 0;
return 1;
}

//---------------------------------------------------------------------------------
//--unused-- int Muzzle_scale=2;
void DoMuzzleStuff (int nSegment, CFixVector *pos)
{
gameData.muzzle.info [gameData.muzzle.queueIndex].createTime = TimerGetFixedSeconds ();
gameData.muzzle.info [gameData.muzzle.queueIndex].nSegment = nSegment;
gameData.muzzle.info [gameData.muzzle.queueIndex].pos = *pos;
if (++gameData.muzzle.queueIndex >= MUZZLE_QUEUE_MAX)
	gameData.muzzle.queueIndex = 0;
}

//	-----------------------------------------------------------------------------------------------------------

CFixVector *GetGunPoints (CObject *objP, int nGun)
{
if (!objP)
	return NULL;

	tGunInfo*	giP = gameData.models.gunInfo + objP->ModelId ();
	CFixVector*	vDefaultGunPoints, *vGunPoints;
	int			nDefaultGuns, nGuns;

if (objP->info.nType == OBJ_PLAYER) {
	vDefaultGunPoints = gameData.pig.ship.player->gunPoints;
	nDefaultGuns = N_PLAYER_GUNS;
	}
else if (objP->info.nType == OBJ_ROBOT) {
	vDefaultGunPoints = ROBOTINFO (objP->info.nId).gunPoints;
	nDefaultGuns = MAX_GUNS;
	}
else
	return NULL;
if (0 < (nGuns = giP->nGuns))
	vGunPoints = giP->vGunPoints;
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

CFixVector *TransformGunPoint (CObject *objP, CFixVector *vGunPoints, int nGun,
										 fix xDelay, ubyte nLaserType, CFixVector *vMuzzle, CFixMatrix *mP)
{
	int						bSpectate = SPECTATOR (objP);
	tObjTransformation*	posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
	CFixMatrix				m, *viewP;
	CFixVector				v [2];
#if FULL_COCKPIT_OFFS
	int						bLaserOffs = ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) &&
												  (objP->Index () == LOCALPLAYER.nObject));
#else
	int						bLaserOffs = 0;
#endif

if (nGun < 0) {	// use center between gunPoints nGun and nGun + 1
	*v = vGunPoints [-nGun] + vGunPoints [-nGun - 1];
//	VmVecScale (VmVecAdd (v, vGunPoints - nGun, vGunPoints - nGun - 1), I2X (1) / 2);
	*v *= (I2X (1) / 2);
}
else {
	v [0] = vGunPoints [nGun];
	if (bLaserOffs)
		v [0] += posP->mOrient.m.dir.u * LASER_OFFS;
	}
if (!mP)
	mP = &m;
if (bSpectate) {
   viewP = mP;
	*viewP = posP->mOrient.Transpose ();
}
else
   viewP = objP->View ();
v[1] = *viewP * v [0];
memcpy (mP, &posP->mOrient, sizeof (CFixMatrix));
if (nGun < 0)
	v[1] += (*mP).m.dir.u * (-2 * v->Mag ());
(*vMuzzle) = posP->vPos + v [1];
//	If supposed to fire at a delayed time (xDelay), then move this point backwards.
if (xDelay)
	*vMuzzle += mP->m.dir.f * (-FixMul (xDelay, WI_speed (nLaserType, gameStates.app.nDifficultyLevel)));
return vMuzzle;
}

//-------------- Initializes a laser after Fire is pressed -----------------

int FireWeaponDelayedWithSpread (
	CObject *objP,
	ubyte nLaserType,
	int nGun,
	fix xSpreadR,
	fix xSpreadU,
	fix xDelay,
	int bMakeSound,
	int bHarmless,
	short	nLightObj)
{
	short					nLaserSeg;
	int					nFate;
	CFixVector			vLaserPos, vLaserDir, *vGunPoints;
	int					nObject;
	CObject*				laserP;
#if FULL_COCKPIT_OFFS
	int bLaserOffs = ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) &&
							(objP->Index () == LOCALPLAYER.nObject));
#else
	int bLaserOffs = 0;
#endif
	CFixMatrix				m;
	int						bSpectate = SPECTATOR (objP);
	tObjTransformation*	posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;

#if DBG
if (nLaserType == SMARTMINE_BLOB_ID)
	nLaserType = nLaserType;
#endif
CreateAwarenessEvent (objP, PA_WEAPON_WALL_COLLISION);
// Find the initial vPosition of the laser
if (!(vGunPoints = GetGunPoints (objP, nGun)))
	return 0;
TransformGunPoint (objP, vGunPoints, nGun, xDelay, nLaserType, &vLaserPos, &m);

//--------------- Find vLaserPos and nLaserSeg ------------------
CHitQuery	hitQuery (FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS, &posP->vPos, &vLaserPos,
							 (bSpectate ? gameStates.app.nPlayerSegment : objP->info.nSegment), objP->Index (),
							 0x10, 0x10, ++gameData.physics.bIgnoreObjFlag);
CHitResult	hitResult;

nFate = FindHitpoint (hitQuery, hitResult);
nLaserSeg = hitResult.nSegment;
if (nLaserSeg == -1) {	//some sort of annoying error
	return -1;
	}
//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
if (CFixVector::Dist (vLaserPos, posP->vPos) > 3 * objP->info.xSize / 2) {
	return -1;
	}
if (nFate == HIT_WALL) {
	return -1;
	}
#if 0
//as of 12/6/94, we don't care if the laser is stuck in an object. We
//just fire away normally
if (nFate == HIT_OBJECT) {
	if (OBJECTS [hitResult.hitObject].nType == OBJ_ROBOT)
		OBJECTS [hitResult.hitObject].Die ();
	if (OBJECTS [hitResult.hitObject].nType != OBJ_POWERUP)
		return;
	}
#endif
//	Now, make laser spread out.
vLaserDir = m.m.dir.f;
if (xSpreadR || xSpreadU) {
	vLaserDir += m.m.dir.r * xSpreadR;
	vLaserDir += m.m.dir.u * xSpreadU;
	}
if (bLaserOffs)
	vLaserDir += m.m.dir.u * LASER_OFFS;
nObject = CreateNewWeapon (&vLaserDir, &vLaserPos, nLaserSeg, objP->Index (), nLaserType, bMakeSound);
//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
if (nLaserType == OMEGA_ID)
	return -1;
if (nObject == -1)
	return -1;
//TrackWeaponObject (nObject, int (objP->info.nId));
laserP = OBJECTS + nObject;
if ((nLaserType == GUIDEDMSL_ID) && gameData.multigame.bIsGuided)
	gameData.objs.guidedMissile [objP->info.nId].objP = laserP;
gameData.multigame.bIsGuided = 0;
if (CObject::IsMissile (nLaserType) && (nLaserType != GUIDEDMSL_ID)) {
	if (!gameData.objs.missileViewerP && (objP->info.nId == N_LOCALPLAYER))
		gameData.objs.missileViewerP = laserP;
	}
//	If this weapon is supposed to be silent, set that bit!
#if 0 
// the following code prevents sound from cutting out due to too many sound sources (e.g. laser bolts)
// the backside is that only one laser bolt from a round creates an impact sound
// turn on if sound cuts out during intense firefights
if (!bMakeSound)
	laserP->info.nFlags |= OF_SILENT;
#endif
//	If this weapon is supposed to be silent, set that bit!
if (bHarmless)
	laserP->info.nFlags |= OF_HARMLESS;

//	If the object firing the laser is the player, then indicate the laser object so robots can dodge.
//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
if ((objP == gameData.objs.consoleP) && !laserP->IsPlayerMine ())
	gameStates.app.bPlayerFiredLaserThisFrame = nObject;

if (gameStates.app.cheats.bHomingWeapons || gameData.weapons.info [nLaserType].homingFlag) {
	if (objP == gameData.objs.consoleP) {
		laserP->cType.laserInfo.nHomingTarget = laserP->FindVisibleHomingTarget (vLaserPos);
		gameData.multigame.laser.nTrack = laserP->cType.laserInfo.nHomingTarget;
		}
	else {// Some other player shot the homing thing
		Assert (IsMultiGame);
		laserP->cType.laserInfo.nHomingTarget = gameData.multigame.laser.nTrack;
		}
	}
lightClusterManager.Add (nObject, nLightObj);
return nObject;
}

//	-----------------------------------------------------------------------------------------------------------

void CreateFlare (CObject *objP)
{
	fix	xEnergyUsage = WI_energy_usage (FLARE_ID);

if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsage = FixMul (xEnergyUsage, I2X (gameStates.app.nDifficultyLevel+2)/4);
LOCALPLAYER.UpdateEnergy (-xEnergyUsage);
LaserPlayerFire (objP, FLARE_ID, 6, 1, 0, -1);
if (IsMultiGame) {
	gameData.multigame.laser.bFired = 1;
	gameData.multigame.laser.nGun = FLARE_ADJUST;
	gameData.multigame.laser.nFlags = 0;
	gameData.multigame.laser.nLevel = 0;
	}
}

//-------------------------------------------------------------------------------------------

static int nMslSlowDown [4] = {1, 4, 3, 2};

float MissileSpeedScale (CObject *objP)
{
	int	i = extraGameInfo [IsMultiGame].nMslStartSpeed;

if (!i)
	return 1;
return nMslSlowDown [i] * X2F (gameData.time.xGame - objP->CreationTime ());
}

//-------------------------------------------------------------------------------------------
//	Set object *objP's orientation to (or towards if I'm ambitious) its velocity.

void HomingMissileTurnTowardsVelocity (CObject *objP, CFixVector *vNormVel)
{
	CFixVector	vNewDir;
	fix 			frameTime;

frameTime = gameStates.limitFPS.bHomers ? MSEC2X (gameStates.app.tick40fps.nTime) : gameData.time.xFrame;
vNewDir = *vNormVel;
vNewDir *= ((fix) (frameTime * 16 / gameStates.gameplay.slowmo [0].fSpeed));
vNewDir += objP->info.position.mOrient.m.dir.f;
CFixVector::Normalize (vNewDir);
objP->info.position.mOrient = CFixMatrix::CreateF(vNewDir);
}

//-------------------------------------------------------------------------------------------

static inline fix HomingMslStraightTime (int id)
{
if (!CObject::IsMissile (id))
	return HOMINGMSL_STRAIGHT_TIME;
if ((id == EARTHSHAKER_MEGA_ID) || (id == ROBOT_SHAKER_MEGA_ID))
	return HOMINGMSL_STRAIGHT_TIME;
return HOMINGMSL_STRAIGHT_TIME * nMslSlowDown [(int) extraGameInfo [IsMultiGame].nMslStartSpeed];
}

//-------------------------------------------------------------------------------------------

bool CObject::RemoveWeapon (void)
{
if (LifeLeft () == ONE_FRAME_TIME) {
	if (IsMultiGame)
		SetLife (OMEGA_MULTI_LIFELEFT);
	else
		SetLife (0);
	info.renderType = RT_NONE;
	return true;
	}
if (LifeLeft () < 0) {		// We died of old age
	Die ();
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (info.position.vPos);
	return true;
	}
//delete weapons that are not moving
fix xSpeed = mType.physInfo.velocity.Mag ();
if (!((gameData.app.nFrameCount ^ info.nSignature) & 3) &&
		(info.nType == OBJ_WEAPON) && (info.nId != FLARE_ID) &&
		(gameData.weapons.info [info.nId].speed [gameStates.app.nDifficultyLevel] > 0) &&
		(xSpeed < I2X (2))) {
	ReleaseObject (Index ());
	return true;
	}
return false;
}

//-------------------------------------------------------------------------------------------
//sequence this weapon object for this _frame_ (underscores added here to aid MK in his searching!)
void CObject::UpdateHomingWeapon (int nThread)
{
for (fix xFrameTime = gameData.laser.xUpdateTime; xFrameTime >= I2X (1) / 40; xFrameTime -= I2X (1) / 40) {
	CFixVector	vVecToObject, vNewVel;
	fix			dot = I2X (1);
	fix			speed, xMaxSpeed, xDist;
	int			nObjId = info.nId;
	//	For first 1/2 second of life, missile flies straight.
	if (cType.laserInfo.xCreationTime + HomingMslStraightTime (nObjId) < gameData.time.xGame) 
		{
		//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
		if (Bounces ())
			mType.physInfo.flags &= ~PF_BOUNCE;
		//	Make sure the CObject we are tracking is still trackable.
		int nTarget = UpdateHomingTarget (cType.laserInfo.nHomingTarget, dot, nThread);
		if (nTarget != -1) {
			if (nTarget == LOCALPLAYER.nObject) {
				fix xDistToTarget = CFixVector::Dist (info.position.vPos, OBJECTS [nTarget].info.position.vPos);
				if ((xDistToTarget < LOCALPLAYER.homingObjectDist) || (LOCALPLAYER.homingObjectDist < 0))
					LOCALPLAYER.homingObjectDist = xDistToTarget;
				}
			vVecToObject = OBJECTS [nTarget].info.position.vPos - info.position.vPos;
			xDist = CFixVector::Normalize (vVecToObject);
			vNewVel = mType.physInfo.velocity;
			speed = CFixVector::Normalize (vNewVel);
			xMaxSpeed = WI_speed (info.nId, gameStates.app.nDifficultyLevel);
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
			if (gameData.weapons.info [info.nId].renderType != WEAPON_RENDER_POLYMODEL)
				vNewVel += vVecToObject;
			CFixVector::Normalize (vNewVel);
			//CFixVector vOldVel = mType.physInfo.velocity;
			CFixVector vTest = info.position.vPos + vNewVel * xDist;
			if (CanSeePoint (NULL, &info.position.vPos, &vTest, info.nSegment, 3 * info.xSize / 2))
				/*mType.physInfo.velocity = vOldVel;
			else*/ {	//	Subtract off life proportional to amount turned. For hardest turn, it will lose 2 seconds per second.
				mType.physInfo.velocity = vNewVel;
				mType.physInfo.velocity *= speed;
				dot = abs (I2X (1) - dot);
				info.xLifeLeft -= FixMul (dot * 32, I2X (1) / 40);
				}

			//	Only polygon OBJECTS have visible orientation, so only they should turn.
			if (gameData.weapons.info [info.nId].renderType == WEAPON_RENDER_POLYMODEL)
				HomingMissileTurnTowardsVelocity (this, &vNewVel);		//	vNewVel is normalized velocity.
			}
		}
	}
UpdateWeaponSpeed ();
}

//-------------------------------------------------------------------------------------------
//	Make sure weapon is not moving faster than allowed speed.

void CObject::UpdateWeaponSpeed (void)
{
fix xSpeed = mType.physInfo.velocity.Mag ();
if ((info.nType == OBJ_WEAPON) && (xSpeed > WI_speed (info.nId, gameStates.app.nDifficultyLevel))) {
	//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
	if (WI_speed (info.nId, gameStates.app.nDifficultyLevel)) {
		fix xScaleFactor = FixDiv (WI_speed (info.nId, gameStates.app.nDifficultyLevel), xSpeed);
		mType.physInfo.velocity *= xScaleFactor;
		}
	}
}

//-------------------------------------------------------------------------------------------
//sequence this weapon object for this _frame_ (underscores added here to aid MK in his searching!)
void CObject::UpdateWeapon (void)
{
	CObject*	gmObjP;
	
Assert (info.controlType == CT_WEAPON);
//	Ok, this is a big hack by MK.
//	If you want an CObject to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME
if (RemoveWeapon ())
	return;
if ((info.nType == OBJ_WEAPON) && (info.nId == FUSION_ID)) {		//always set fusion weapon to max vel
	CFixVector::Normalize (mType.physInfo.velocity);
	mType.physInfo.velocity *= (WI_speed (info.nId, gameStates.app.nDifficultyLevel));
	}
//	For homing missiles, turn towards target. (unless it's the guided missile)
if ((gameData.laser.xUpdateTime >= I2X (1) / 40) &&
	 (info.nType == OBJ_WEAPON) &&
    (gameStates.app.cheats.bHomingWeapons || WI_homingFlag (info.nId)) &&
	 !(info.nFlags & PF_HAS_BOUNCED) &&
	 !((info.nId == GUIDEDMSL_ID) &&
	   (this == (gmObjP = gameData.objs.guidedMissile [OBJECTS [cType.laserInfo.parent.nObject].info.nId].objP)) &&
		(info.nSignature == gmObjP->info.nSignature))) {
#if USE_OPENMP > 1
	if (gameStates.app.bMultiThreaded)
		gameData.objs.update.Push (this);
	else
#endif
		UpdateHomingWeapon ();
	}
else
	UpdateWeaponSpeed ();
}

//	--------------------------------------------------------------------------------------------------

void StopPrimaryFire (void)
{
#if 0
gameData.laser.xNextFireTime = gameData.time.xGame;	//	Prevents shots-to-fire from building up.
#else
if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
	gameData.laser.xNextFireTime = gameData.time.xGame + I2X (1) / 25;
else
	gameData.laser.xNextFireTime = gameData.time.xGame + WI_fire_wait (gameData.weapons.nPrimary);
#endif
gameData.laser.nGlobalFiringCount = 0;
controls.StopPrimaryFire ();
gameData.weapons.firing [0].nStart =
gameData.weapons.firing [0].nStop =
gameData.weapons.firing [0].nDuration = 0;
}

//	--------------------------------------------------------------------------------------------------

void StopSecondaryFire (void)
{
gameData.missiles.xNextFireTime = gameData.time.xGame;	//	Prevents shots-to-fire from building up.
gameData.missiles.nGlobalFiringCount = 0;
controls.StopSecondaryFire ();
}

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console player, not for network players

int LocalPlayerFireGun (void)
{
	CPlayerData*	playerP = gameData.multiplayer.players + N_LOCALPLAYER;
	CObject*			objP = &OBJECTS [playerP->nObject];
	fix				xEnergyUsed;
	int				nAmmoUsed, nPrimaryAmmo;
	int				nWeaponIndex;
	int				rVal = 0;
	int 				nRoundsPerShot = 1;
	int				bGatling = (gameData.weapons.nPrimary == GAUSS_INDEX) ? 2 : (gameData.weapons.nPrimary == VULCAN_INDEX) ? 1 : 0;
	fix				addval;
	static int		nSpreadfireToggle = 0;
	static int		nHelixOrient = 0;

if (gameStates.app.bPlayerIsDead)
	return 0;
if (gameStates.app.bD2XLevel && (SEGMENTS [objP->info.nSegment].HasNoDamageProp ()))
	return 0;
nWeaponIndex = primaryWeaponToWeaponInfo [gameData.weapons.nPrimary];
xEnergyUsed = WI_energy_usage (nWeaponIndex);
if (gameData.weapons.nPrimary == OMEGA_INDEX)
	xEnergyUsed = 0;	//	Omega consumes energy when recharging, not when firing.
if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsed = FixMul (xEnergyUsed, I2X (gameStates.app.nDifficultyLevel + 2) / 4);
//	MK, 01/26/96, Helix use 2x energy in multiplayer.  bitmaps.tbl parm should have been reduced for single player.
//if (nWeaponIndex == FUSION_ID)
//	postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), I2X (1) / 3, objP->info.xSize, 1, OBJPOS (objP)->vPos + OBJPOS (objP)->mOrient.m.dir.f * objP->info.xSize));
else if (nWeaponIndex == HELIX_ID) {
	if (IsMultiGame)
		xEnergyUsed *= 2;
	}
nAmmoUsed = WI_ammo_usage (nWeaponIndex);
addval = 2 * gameData.time.xFrame;
if (addval > I2X (1))
	addval = I2X (1);
if (!bGatling)
	nPrimaryAmmo = playerP->primaryAmmo [gameData.weapons.nPrimary];
else {
	if ((gameOpts->UseHiresSound () == 2) && gameOpts->sound.bGatling &&
		 gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bGatlingSpeedUp, 1, 0, 0) &&
		 (gameData.weapons.firing [0].nDuration < GATLING_DELAY))
		return 0;
	nPrimaryAmmo = playerP->primaryAmmo [VULCAN_INDEX];
	}
if	 ((playerP->energy < xEnergyUsed) || (nPrimaryAmmo < nAmmoUsed))
	AutoSelectWeapon (0, 1);		//	Make sure the player can fire from this weapon.

if ((gameData.laser.xLastFiredTime + 2 * gameData.time.xFrame < gameData.time.xGame) ||
	 (gameData.time.xGame < gameData.laser.xLastFiredTime))
	gameData.laser.xNextFireTime = gameData.time.xGame;
gameData.laser.xLastFiredTime = gameData.time.xGame;

while (gameData.laser.xNextFireTime <= gameData.time.xGame) {
	if	((playerP->energy >= xEnergyUsed) && (nPrimaryAmmo >= nAmmoUsed)) {
			int nLaserLevel, flags = 0;

		if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
			gameData.laser.xNextFireTime += I2X (1) / 25;
		else
			gameData.laser.xNextFireTime += WI_fire_wait (nWeaponIndex);
		nLaserLevel = LOCALPLAYER.laserLevel;
		if (gameData.weapons.nPrimary == SPREADFIRE_INDEX) {
			if (nSpreadfireToggle)
				flags |= LASER_SPREADFIRE_TOGGLED;
			nSpreadfireToggle = !nSpreadfireToggle;
			}
		if (gameData.weapons.nPrimary == HELIX_INDEX) {
			nHelixOrient++;
			flags |= ((nHelixOrient & LASER_HELIX_MASK) << LASER_HELIX_SHIFT);
			}
		if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
			flags |= LASER_QUAD;
#if 1
		int fired = FireWeapon ((short) LOCALPLAYER.nObject, (ubyte) gameData.weapons.nPrimary, nLaserLevel, flags, nRoundsPerShot);
		if (fired) {
			rVal += fired;
			if (bGatling) {
				if (nAmmoUsed > playerP->primaryAmmo [VULCAN_INDEX])
					nAmmoUsed = playerP->primaryAmmo [VULCAN_INDEX];
				playerP->primaryAmmo [VULCAN_INDEX] -= nAmmoUsed;
				gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed += nAmmoUsed;
				if (gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed >= VULCAN_AMMO_AMOUNT) {
					if (gameStates.app.bHaveExtraGameInfo [IsMultiGame])
						MaybeDropNetPowerup (-1, POW_VULCAN_AMMO, FORCE_DROP);
					gameData.multiplayer.weaponStates [N_LOCALPLAYER].nAmmoUsed -= VULCAN_AMMO_AMOUNT;
					}
				MultiSendAmmo ();
				}
			else {
				playerP->energy -= (xEnergyUsed * fired) / gameData.weapons.info [nWeaponIndex].fireCount;
				if (playerP->energy < 0)
					playerP->energy = 0;
				}
			}
#else
		rVal += FireWeapon ((short) LOCALPLAYER.nObject, (ubyte) gameData.weapons.nPrimary, nLaserLevel, flags, nRoundsPerShot);
		playerP->energy -= (xEnergyUsed * rVal) / gameData.weapons.info [nWeaponIndex].fireCount;
		if (playerP->energy < 0)
			playerP->energy = 0;
		if (rVal && ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX))) {
			if (nAmmoUsed > playerP->primaryAmmo [VULCAN_INDEX])
				playerP->primaryAmmo [VULCAN_INDEX] = 0;
			else
				playerP->primaryAmmo [VULCAN_INDEX] -= nAmmoUsed;
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
gameData.laser.nGlobalFiringCount = 0;
return rVal;
}

//	-------------------------------------------------------------------------------------------
//	if nGoalObj == -1, then create Random vector
int CreateHomingWeapon (CObject *objP, int nGoalObj, ubyte objType, int bMakeSound)
{
	short			nObject;
	CFixVector	vGoal, vRandom;

if (nGoalObj == -1)
	vGoal = CFixVector::Random ();
else { //	Create a vector towards the goal, then add some noise to it.
	CFixVector::NormalizedDir (vGoal, OBJECTS [nGoalObj].info.position.vPos, objP->info.position.vPos);
	vRandom = CFixVector::Random ();
	vGoal += vRandom * (I2X (1)/4);
	CFixVector::Normalize (vGoal);
	}
if (0 > (nObject = CreateNewWeapon (&vGoal, &objP->info.position.vPos, objP->info.nSegment, objP->Index (), objType, bMakeSound)))
	return -1;
OBJECTS [nObject].cType.laserInfo.nHomingTarget = nGoalObj;
return nObject;
}

//-----------------------------------------------------------------------------
// Create the children of a smart bomb, which is a bunch of homing missiles.
void CreateSmartChildren (CObject *objP, int nSmartChildren)
{
	tParentInfo	parent;
	int			bMakeSound;
	int			nObjects = 0;
	int			objList [MAX_OBJDISTS];
	ubyte			nBlobId, nObjType = objP->info.nType, nObjId = objP->info.nId;

if (nObjType == OBJ_WEAPON) {
	parent = objP->cType.laserInfo.parent;
	}
else if (nObjType == OBJ_ROBOT) {
	parent.nType = OBJ_ROBOT;
	parent.nObject = objP->Index ();
	}
else {
	Int3 ();	//	Hey, what kind of CObject is this!?
	parent.nType = 0;
	parent.nObject = 0;
	}

if (nObjType == OBJ_WEAPON) {
	if (gameData.weapons.info [nObjId].children < 1)
	//if ((nObjId == SMARTMSL_ID) || (nObjId == SMARTMINE_ID) || (nObjId == ROBOT_SMARTMINE_ID) || (nObjId == EARTHSHAKER_ID)) &&
		return;
	if (nObjId == EARTHSHAKER_ID)
		BlastNearbyGlass (objP, gameData.weapons.info [EARTHSHAKER_ID].strength [gameStates.app.nDifficultyLevel]);
	}
else if (nObjType != OBJ_ROBOT) // && ((nObjType != OBJ_WEAPON) || (gameData.weapons.info [nObjId].children < 1)))
	return;

int		i, nObject;
CObject	*curObjP;

if (IsMultiGame)
	d_srand (gameStates.app.nRandSeed = 8321L);
FORALL_OBJS (curObjP, nObject) {
	nObject = OBJ_IDX (curObjP);
	if (curObjP->info.nType == OBJ_PLAYER) {
		if (nObject == parent.nObject)
			continue;
		if ((parent.nType == OBJ_PLAYER) && (IsCoopGame))
			continue;
		if (IsTeamGame && (GetTeam (curObjP->info.nId) == GetTeam (OBJECTS [parent.nObject].info.nId)))
			continue;
		if (gameData.multiplayer.players [curObjP->info.nId].flags & PLAYER_FLAGS_CLOAKED)
			continue;
		}
	else if (curObjP->info.nType == OBJ_ROBOT) {
		if (curObjP->cType.aiInfo.CLOAKED)
			continue;
		if (parent.nType == OBJ_ROBOT)	//	Robot blobs can't track robots.
			continue;
		if ((parent.nType == OBJ_PLAYER) &&	ROBOTINFO (curObjP->info.nId).companion)	// Your shots won't track the buddy.
			continue;
		}
	else
		continue;
	fix dist = CFixVector::Dist (objP->info.position.vPos, curObjP->info.position.vPos);
	if (dist < MAX_SMART_DISTANCE) {
		int	oovis = ObjectToObjectVisibility (objP, curObjP, FQ_TRANSWALL);
		if (oovis) { //ObjectToObjectVisibility (objP, curObjP, FQ_TRANSWALL)) {
			objList [nObjects++] = nObject;
			if (nObjects >= MAX_OBJDISTS) {
				nObjects = MAX_OBJDISTS;
				break;
				}
			}
		}
	}
//	Get type of weapon for child from parent.
if (nObjType == OBJ_WEAPON) {
	nBlobId = gameData.weapons.info [nObjId].children;
	}
else {
	Assert (nObjType == OBJ_ROBOT);
	nBlobId = ROBOT_SMARTMSL_BLOB_ID;
	}
bMakeSound = 1;
for (i = 0; i < nSmartChildren; i++) {
	short nTarget = nObjects ? objList [(RandShort () * nObjects) >> 15] : -1;
	CreateHomingWeapon (objP, nTarget, nBlobId, bMakeSound);
	bMakeSound = 0;
	}
}

//	-------------------------------------------------------------------------------------------

//give up control of the guided missile
void ReleaseGuidedMissile (int nPlayer)
{
if (nPlayer == N_LOCALPLAYER) {
	if (!gameData.objs.guidedMissile [nPlayer].objP)
		return;
	gameData.objs.missileViewerP = gameData.objs.guidedMissile [nPlayer].objP;
	if (IsMultiGame)
	 	MultiSendGuidedInfo (gameData.objs.guidedMissile [N_LOCALPLAYER].objP, 1);
	if (gameData.demo.nState == ND_STATE_RECORDING)
	 	NDRecordGuidedEnd ();
	 }
gameData.objs.guidedMissile [nPlayer].objP = NULL;
}

//	-------------------------------------------------------------------------------------------
//parameter determines whether or not to do autoselect if have run out of ammo
//this is needed because if you drop a bomb with the B key, you don't
//want to autoselect if the bomb isn't actually selected.
void DoMissileFiring (int bAutoSelect)
{
	int		h, i, gunFlag = 0;
	short		nObject;
	ubyte		nWeaponId;
	int		nGun;
	CObject	*gmObjP = gameData.objs.guidedMissile [N_LOCALPLAYER].objP;
	CPlayerData	*playerP = gameData.multiplayer.players + N_LOCALPLAYER;

Assert (gameData.weapons.nSecondary < MAX_SECONDARY_WEAPONS);
if (gmObjP && (gmObjP->info.nSignature == gameData.objs.guidedMissile [N_LOCALPLAYER].nSignature)) {
	ReleaseGuidedMissile (N_LOCALPLAYER);
	i = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
	gameData.missiles.xNextFireTime = gameData.time.xGame + WI_fire_wait (i);
	return;
	}

if (gameStates.app.bPlayerIsDead || (playerP->secondaryAmmo [gameData.weapons.nSecondary] <= 0))
	return;

nWeaponId = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
if ((nWeaponId == PROXMINE_ID) && IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) &&
	 (CountPlayerObjects (N_LOCALPLAYER, OBJ_WEAPON, PROXMINE_ID) >= extraGameInfo [IsMultiGame].nMaxSmokeGrenades))
	return;
if (gameStates.app.cheats.bLaserRapidFire != 0xBADA55)
	gameData.missiles.xNextFireTime = gameData.time.xGame + WI_fire_wait (nWeaponId);
else
	gameData.missiles.xNextFireTime = gameData.time.xGame + I2X (1)/25;
gameData.missiles.xLastFiredTime = gameData.time.xGame;
nGun = secondaryWeaponToGunNum [gameData.weapons.nSecondary];
h = !COMPETITION && (EGI_FLAG (bDualMissileLaunch, 0, 1, 0)) ? 1 : 0;
for (i = 0; (i <= h) && (playerP->secondaryAmmo [gameData.weapons.nSecondary] > 0); i++) {
	playerP->secondaryAmmo [gameData.weapons.nSecondary]--;
	if (IsMultiGame)
		MultiSendWeapons (1);
	if (nGun == 4) {		//alternate left/right
		nGun += (gunFlag = (gameData.laser.nMissileGun & 1));
		gameData.laser.nMissileGun++;
		}
	if (0 > (nObject = LaserPlayerFire (gameData.objs.consoleP, nWeaponId, nGun, 1, 0, -1)))
		return;
	if (gameData.weapons.nSecondary == PROXMINE_INDEX) {
		if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
			if (++gameData.laser.nProximityDropped == 4) {
				gameData.laser.nProximityDropped = 0;
				MaybeDropNetPowerup (nObject, POW_PROXMINE, INIT_DROP);
				}
			}
		break; //no dual prox bomb drop
		}
	else if (gameData.weapons.nSecondary == SMARTMINE_INDEX) {
		if (!(gameData.app.nGameMode & GM_ENTROPY)) {
			if (++gameData.laser.nSmartMinesDropped == 4) {
				gameData.laser.nSmartMinesDropped = 0;
				MaybeDropNetPowerup (nObject, POW_SMARTMINE, INIT_DROP);
				}
			}
		break; //no dual smartmine drop
		}
	else if (gameData.weapons.nSecondary != CONCUSSION_INDEX)
		MaybeDropNetPowerup (nObject, secondaryWeaponToPowerup [gameData.weapons.nSecondary], INIT_DROP);
	else {
		if (gameData.multiplayer.nBuiltinMissiles)
			gameData.multiplayer.nBuiltinMissiles--;
		else
			MaybeDropNetPowerup (nObject, secondaryWeaponToPowerup [gameData.weapons.nSecondary], INIT_DROP);
		}
	if ((gameData.weapons.nSecondary == GUIDED_INDEX) || (gameData.weapons.nSecondary == SMART_INDEX))
		break;
	else if ((gameData.weapons.nSecondary == MEGA_INDEX) || (gameData.weapons.nSecondary == EARTHSHAKER_INDEX)) {
		CFixVector vForce;

	vForce.v.coord.x = - (gameData.objs.consoleP->info.position.mOrient.m.dir.f.v.coord.x << 7);
	vForce.v.coord.y = - (gameData.objs.consoleP->info.position.mOrient.m.dir.f.v.coord.y << 7);
	vForce.v.coord.z = - (gameData.objs.consoleP->info.position.mOrient.m.dir.f.v.coord.z << 7);
	gameData.objs.consoleP->ApplyForce (vForce);
	vForce.v.coord.x = (vForce.v.coord.x >> 4) + SRandShort ();
	vForce.v.coord.y = (vForce.v.coord.y >> 4) + SRandShort ();
	vForce.v.coord.z = (vForce.v.coord.z >> 4) + SRandShort ();
	gameData.objs.consoleP->ApplyRotForce (vForce);
	break; //no dual mega/smart missile launch
	}
}

if (IsMultiGame) {
	gameData.multigame.laser.bFired = 1;		//how many
	gameData.multigame.laser.nGun = gameData.weapons.nSecondary + MISSILE_ADJUST;
	gameData.multigame.laser.nFlags = gunFlag;
	gameData.multigame.laser.nLevel = 0;
	}
if (bAutoSelect)
	AutoSelectWeapon (1, 1);		//select next missile, if this one out of ammo
}

// -----------------------------------------------------------------------------

void GetPlayerMslLock (void)
{
if (gameStates.app.bPlayerIsDead)
	return;

	int			nWeapon, nObject, nGun, h, i, j;
	CFixVector	*vGunPoints, vGunPos;
	CFixMatrix	*viewP;
	CObject		*objP;

gameData.objs.trackGoals [0] =
gameData.objs.trackGoals [1] = NULL;
if ((objP = GuidedInMainView ())) {
	nObject = objP->FindVisibleHomingTarget (objP->info.position.vPos);
	gameData.objs.trackGoals [0] =
	gameData.objs.trackGoals [1] = (nObject < 0) ? NULL : OBJECTS + nObject;
	return;
	}
if (!AllowedToFireMissile (-1, 1))
	return;
if (!EGI_FLAG (bMslLockIndicators, 0, 1, 0) || COMPETITION)
	return;
if (gameStates.app.bPlayerIsDead)
	return;
nWeapon = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
if (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] <= 0)
	return;
if (!gameStates.app.cheats.bHomingWeapons &&
	 (nWeapon != HOMINGMSL_ID) && (nWeapon != MEGAMSL_ID) && (nWeapon != GUIDEDMSL_ID))
	return;
//pnt = gameData.pig.ship.player->gunPoints [nGun];
if (nWeapon == MEGAMSL_ID) {
	j = 1;
	h = 0;
	}
else {
	j = !COMPETITION && (EGI_FLAG (bDualMissileLaunch, 0, 1, 0)) ? 2 : 1;
	h = gameData.laser.nMissileGun & 1;
	}
viewP = gameData.objs.consoleP->View ();
for (i = 0; i < j; i++, h = !h) {
	nGun = secondaryWeaponToGunNum [gameData.weapons.nSecondary] + h;
	if ((vGunPoints = GetGunPoints (gameData.objs.consoleP, nGun))) {
		vGunPos = vGunPoints [nGun];
		vGunPos = *viewP * vGunPos;
		vGunPos += gameData.objs.consoleP->info.position.vPos;
		nObject = gameData.objs.consoleP->FindVisibleHomingTarget (vGunPos);
		gameData.objs.trackGoals [i] = (nObject < 0) ? NULL : OBJECTS + nObject;
		}
	}
}

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireGun (void)
{
	int i = primaryWeaponToWeaponInfo [gameData.weapons.nPrimary];

if (gameData.weapons.firing [0].nDuration)
	gameData.laser.nGlobalFiringCount += WI_fireCount (i);
if ((gameData.weapons.nPrimary == FUSION_INDEX) && gameData.laser.nGlobalFiringCount)
	ChargeFusion ();
}

//	-------------------------------------------------------------------------------------------
// eof
