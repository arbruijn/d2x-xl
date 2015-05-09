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

#include <string.h>	// for memset
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "descent.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "segmath.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "rendermine.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "segmath.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "cockpit.h"
#include "text.h"
#include "sphere.h"
#include "input.h"
#include "automap.h"
#include "u_mem.h"
#include "entropy.h"
#include "objrender.h"
#include "dropobject.h"
#include "marker.h"
#include "hiresmodels.h"
#include "loadgame.h"
#include "multi.h"
#include "playerdeath.h"
#include "renderthreads.h"
#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

#define	DEG90		 (I2X (1) / 4)
#define	DEG45		 (I2X (1) / 8)
#define	DEG1		 (I2X (1) / (4 * 90))

//------------------------------------------------------------------------------

void StopObjectMovement (CObject *pObj)
{
controls [0].headingTime = 0;
controls [0].pitchTime = 0;
controls [0].bankTime = 0;
controls [0].verticalThrustTime = 0;
controls [0].sidewaysThrustTime = 0;
controls [0].forwardThrustTime = 0;
if (pObj) {
	pObj->mType.physInfo.rotThrust.SetZero ();
	pObj->mType.physInfo.thrust.SetZero ();
	pObj->mType.physInfo.velocity.SetZero ();
	pObj->mType.physInfo.rotVel.SetZero ();
	}
}

//------------------------------------------------------------------------------

void StopPlayerMovement (void)
{
if (OBJECTS.Buffer () && !(gameData.objData.speedBoost.Buffer () && gameData.objData.speedBoost [OBJ_IDX (gameData.objData.pConsole)].bBoosted)) {
	StopObjectMovement (OBJECT (LOCALPLAYER.nObject));
	memset (&gameData.physics.playerThrust, 0, sizeof (gameData.physics.playerThrust));
//	gameData.time.SetTime (I2X (1));
	gameData.objData.speedBoost [OBJ_IDX (gameData.objData.pConsole)].bBoosted = 0;
	}
}

//------------------------------------------------------------------------------

void CObject::Die (void)
{
info.nFlags |= OF_SHOULD_BE_DEAD;
#if DBG
if (Index () == nDbgObj)
	BRP;
if (this == dbgObjP)
	BRP;
#endif
if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME)) {
	if (Type () == OBJ_POWERUP) 
		RemovePowerupInMine (Id ());
	else if (Type () == OBJ_WEAPON) {
		if (IsMissile ()) {
			int32_t i = FindDropInfo (Signature ());
			if (i >= 0) {
				if (gameData.objData.dropInfo [i].nDropTime == 0x7FFFFFFF) 
					MaybeDropNetPowerup (i, gameData.objData.dropInfo [i].nPowerupType, FORCE_DROP);
				DelDropInfo (i);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void CObject::MultiDie (void)
{
info.nFlags |= OF_SHOULD_BE_DEAD;
#if DBG
if (this == dbgObjP)
	BRP;
#endif
}

//------------------------------------------------------------------------------
//process a continuously-spinning CObject
void CObject::Spin (void)
{
	CAngleVector vRot;
	CFixMatrix mRot, mOrient;

Assert (info.movementType == MT_SPINNING);
vRot = CAngleVector::Create ((fixang) FixMul (mType.spinRate.v.coord.x, gameData.time.xFrame),
									  (fixang) FixMul (mType.spinRate.v.coord.y, gameData.time.xFrame),
									  (fixang) FixMul (mType.spinRate.v.coord.z, gameData.time.xFrame));
mRot = CFixMatrix::Create (vRot);
mOrient = info.position.mOrient * mRot;
info.position.mOrient = mOrient;
info.position.mOrient.CheckAndFix();
}

// -----------------------------------------------------------------------------

void CObject::RotateCamera (void)
{
cameraManager.Rotate (this);
}

// -----------------------------------------------------------------------------

void CObject::RotateMarker (void)
{
if (m_bRotate && EGI_FLAG (bRotateMarkers, 0, 1, 0) && gameStates.app.tick40fps.bTick) {
	static time_t	t0 [MAX_DROP_SINGLE] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

	time_t t = (gameStates.app.nSDLTicks [0] - t0 [info.nId]) % 1000;
	t0 [info.nId] = gameStates.app.nSDLTicks [0];
	if (t) {
		CAngleVector a = CAngleVector::Create (0, 0, (fixang) ((float) (I2X (1) / 512) * t / 25.0f));
		CFixMatrix mRotate = CFixMatrix::Create (a);
		CFixMatrix mOrient = mRotate * info.position.mOrient;
		info.position.mOrient = mOrient;
		info.position.mOrient.CheckAndFix ();
		}
	}
}

// -----------------------------------------------------------------------------

int32_t CObject::CheckWallPhysics (void)
{
	int32_t	nType = 0;
	int16_t nObject = OBJ_IDX (this);

if (info.nType != OBJ_PLAYER)
	return 0;
CSegment* pSeg = SEGMENT (info.nSegment);
int32_t sideMask = pSeg->Masks (info.position.vPos, info.xSize).m_side;
if (sideMask) {
	int16_t		nSide;
	int32_t		bit;
	for (nSide = 0, bit = 1; nSide < SEGMENT_SIDE_COUNT; bit <<= 1, nSide++)
		if ((sideMask & bit) && (nType = ApplyWallPhysics (info.nSegment, nSide)))
			break;
	}
if (!nType)
	nType = CheckSegmentPhysics ();
// type 1,2: segment related sound
// type 2,3: sound caused by ship touching a wall
// type & 1: lava
// type & 2: water
int32_t nSoundObj;

if (nType) {
	int16_t nSound;

	if (nType == 2)
		nSound = mType.physInfo.thrust.IsZero () ? 0 : SOUND_SHIP_IN_WATER;
	else
		nSound = (nType & 1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
	nSoundObj = audio.FindObjectSound (nObject, nSound);
	if (nSound && (nSoundObj < 0)) 
		audio.CreateObjectSound (nSound, SOUNDCLASS_GENERIC, nObject, 1);
	else if (!nSound && (nSoundObj >= 0))
		audio.DeleteSoundObject (nSoundObj);
	}
else {
	for (;;) {
		if ((0 > (nSoundObj = audio.FindObjectSound (nObject, SOUND_SHIP_IN_WATER))) &&
			 (0 > (nSoundObj = audio.FindObjectSound (nObject, SOUND_SHIP_IN_WATERFALL))) &&
			 (0 > (nSoundObj = audio.FindObjectSound (nObject, SOUND_LAVAFALL_HISS))))
			break;
		audio.DeleteSoundObject (nSoundObj);
		}
	}
return nType;
}

// -----------------------------------------------------------------------------

void CObject::HandleSegmentFunction (void)
{
if ((info.nType == OBJ_PLAYER) && (N_LOCALPLAYER == info.nId)) {
	CSegment*		pSeg = SEGMENT (info.nSegment);
	CPlayerData*	pPlayer = gameData.multiplayer.players + N_LOCALPLAYER;

   if (gameData.app.GameMode (GM_CAPTURE))
		 pSeg->CheckForGoal ();
   else if (IsHoardGame)
		 pSeg->CheckForHoardGoal ();
	else if (controls [0].forwardThrustTime || controls [0].verticalThrustTime || controls [0].sidewaysThrustTime) {
		gameStates.entropy.nTimeLastMoved = -1;
		if (IsEntropyGame &&
			 ((pSeg->m_owner < 0) || (pSeg->m_owner == GetTeam (N_LOCALPLAYER) + 1))) {
			StopConquerWarning ();
			}
		}
	else if (gameStates.entropy.nTimeLastMoved < 0)
		gameStates.entropy.nTimeLastMoved = 0;

	pPlayer->UpdateShield (-pSeg->ShieldDamage (pPlayer->Shield () + 1));
	pPlayer->UpdateEnergy (-pSeg->EnergyDamage (pPlayer->energy));
	if (pPlayer->Shield () < 0)
		StartPlayerDeathSequence (this);
	else {
		pSeg->ConquerCheck ();
		//CObject* pObj = OBJECT (pPlayer->nObject);
		fix energy = pSeg->Refuel (fix (pPlayer->InitialEnergy ()) - pPlayer->Energy ());
		if (energy > 0)
			pPlayer->UpdateEnergy (energy);
		fix shield = pSeg->Repair (fix (pPlayer->InitialShield ()) - pPlayer->Shield ());
		if (shield > 0) {
			pPlayer->UpdateShield (shield);
			}
		}
	}
}

// -----------------------------------------------------------------------------

int32_t CObject::UpdateMovement (void)
{
#if DBG
if (info.nType == OBJ_REACTOR)
	BRP;
#endif
switch (info.controlType) {
	case CT_NONE:
		break;

	case CT_FLYING:
		ApplyFlightControls ();
		break;

	case CT_REPAIRCEN:
		Int3 ();	// -- hey!these are no longer supported!!-- do_repair_sequence (this);
		break;

	case CT_POWERUP:
		DoPowerupFrame ();
		break;

	case CT_MORPH:			//morph implies AI
		DoMorphFrame ();
		//NOTE: FALLS INTO AI HERE!!!!

	case CT_AI:
		//NOTE LINK TO CT_MORPH ABOVE!!!
		if (gameStates.app.bGameSuspended & SUSP_ROBOTS) {
			mType.physInfo.velocity.SetZero ();
			mType.physInfo.thrust.SetZero ();
			mType.physInfo.rotThrust.SetZero ();
			Unstick ();
			DoAnyRobotDyingFrame (this);
#if 1//!DBG
			return 1;
#endif
			}
		else if (USE_D1_AI)
			DoD1AIFrame (this);
		else
			DoAIFrame (this);
		break;

	case CT_CAMERA:
		RotateCamera ();
		break;

	case CT_WEAPON:
		UpdateWeapon ();
		break;

	case CT_EXPLOSION:
		DoExplosionSequence ();
		break;

	case CT_SLEW:
		break;	//ignore

	case CT_DEBRIS:
		DoDebrisFrame (this);
		break;

	case CT_LIGHT:
		break;		//doesn't do anything

	case CT_REMOTE:
		break;		//movement is handled in com_process_input

	case CT_CNTRLCEN:
		if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
			return 1;
		DoReactorFrame (this);
		break;

	default:
		Error ("Unknown control nType %d in CObject %i, sig/nType/id = %i/%i/%i", info.controlType, OBJ_IDX (this), info.nSignature, info.nType, info.nId);
	}
return 0;
}

// -----------------------------------------------------------------------------

static inline int32_t ShipVolume (int32_t nSpeed)
{
return I2X (1) / 16 + nSpeed / 512;
}

void CObject::UpdateShipSound (void)
{
if (!gameOpts->sound.bShip)
	return;

int32_t nObject = OBJ_IDX (this);
if (nObject != LOCALPLAYER.nObject)
	return;

int32_t nSpeed = mType.physInfo.velocity.Mag();
#if 0
nSpeed -= I2X (2);
if (nSpeed < 0)
	nSpeed = 0;
#endif
//if (gameData.multiplayer.bMoving == nSpeed)
//	return;

if (gameData.multiplayer.bMoving < 0)
	audio.CreateObjectSound (-1, SOUNDCLASS_PLAYER, OBJ_IDX (this), 1, ShipVolume (nSpeed), I2X (256), -1, -1, 
									 const_cast<char*> (addonSounds [SND_ADDON_JET_ENGINE].szSoundFile), 1, 1);
else
	audio.ChangeObjectSound (OBJ_IDX (this), ShipVolume (nSpeed));
gameData.multiplayer.bMoving = nSpeed;
}

// -----------------------------------------------------------------------------

void CObject::UpdatePosition (void)
{
if (info.nType == OBJ_MARKER)
	RotateMarker ();

switch (info.movementType) {
	case MT_NONE:
		break;								//this doesn't move

	case MT_PHYSICS:
		DoPhysicsSim ();
		lightManager.SetPos (OBJ_IDX (this));
#if 1
		RequestEffects (MOVE_LIGHTNING);
#else
		lightningManager.MoveForObject (this);
#endif
		if (info.nType == OBJ_PLAYER)
			UpdateShipSound ();
		break;	//move by physics

	case MT_SPINNING:
		Spin ();
		break;
	}
}

// -----------------------------------------------------------------------------

int32_t CObject::CheckTriggerHits (int16_t nPrevSegment)
{
		int16_t	nConnSide, i;
		int32_t	nOldLevel;

if ((info.movementType != MT_PHYSICS) || (nPrevSegment == info.nSegment))
	return 0;
nOldLevel = missionManager.nCurrentLevel;
for (i = 0; i < gameData.physics.nSegments - 1; i++) {
	if (gameData.physics.segments [i] < 0)
		continue;
	if (gameData.physics.segments [i + 1] < 0)
		continue;
#if DBG
	if (gameData.physics.segments [i] > gameData.segData.nLastSegment)
		PrintLog (0, "invalid segment in gameData.physics.segments\n");
#endif
	nConnSide = SEGMENT (gameData.physics.segments [i + 1])->ConnectedSide (SEGMENT (gameData.physics.segments [i]));
	if (nConnSide != -1)
		SEGMENT (gameData.physics.segments [i])->OperateTrigger (nConnSide, this, 0);
#if DBG
	else	// segments are not directly connected, so do binary subdivision until you find connected segments.
		PrintLog (0, "Unconnected segments %d, %d\n", gameData.physics.segments [i + 1], gameData.physics.segments [i]);
#endif
	//maybe we've gone on to the next level.  if so, bail!
	if (missionManager.nCurrentLevel != nOldLevel)
		return 1;
	}
return 0;
}

// -----------------------------------------------------------------------------

void CObject::CheckGuidedMissileThroughExit (int16_t nPrevSegment)
{
if (IsGuidedMissile (N_LOCALPLAYER)) {
	if (nPrevSegment != info.nSegment) {
		int16_t nConnSide = SEGMENT (info.nSegment)->ConnectedSide (SEGMENT (nPrevSegment));
		if (nConnSide != -1) {
			CTrigger* pTrigger = SEGMENT (nPrevSegment)->Trigger (nConnSide);
			if (pTrigger && (pTrigger->m_info.nType == TT_EXIT))
				gameData.objData.guidedMissile [N_LOCALPLAYER].pObj->UpdateLife (0);
			}
		}
	}
}

// -----------------------------------------------------------------------------

void CObject::CheckAfterburnerBlobDrop (void)
{
if (gameStates.render.bDropAfterburnerBlob) {
	Assert (this == gameData.objData.pConsole);
	DropAfterburnerBlobs (this, 2, I2X (5) / 2, -1, NULL, 0);	//	-1 means use default lifetime
	if (IsMultiGame)
		MultiSendDropBlobs (N_LOCALPLAYER);
	gameStates.render.bDropAfterburnerBlob = 0;
	}

if ((info.nType == OBJ_WEAPON) && (gameData.weapons.info [info.nId].nAfterburnerSize)) {
	int32_t	nObject, bSmoke;
	fix	vel;
	fix	delay, lifetime, nSize;

#if 1
	if (IsMissile ()) {
		if (SHOW_SMOKE && gameOpts->render.particles.bMissiles)
			return;
		if ((gameStates.app.bNostalgia || EGI_FLAG (bThrusterFlames, 1, 1, 0)) &&
			 (info.nId != MERCURYMSL_ID))
			return;
		}
#endif
	if ((vel = mType.physInfo.velocity.Mag()) > I2X (200))
		delay = I2X (1) / 16;
	else if (vel > I2X (40))
		delay = FixDiv (I2X (13), vel);
	else
		delay = DEG90;

	if ((bSmoke = SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
		nSize = I2X (3);
		lifetime = I2X (1) / 12;
		delay = 0;
		}
	else {
		nSize = I2X (gameData.weapons.info [info.nId].nAfterburnerSize) / 16;
		lifetime = 3 * delay / 2;
		if (!IsMultiGame) {
			delay /= 2;
			lifetime *= 2;
			}
		}

	nObject = OBJ_IDX (this);
	if (bSmoke ||
		 (gameData.objData.xLastAfterburnerTime [nObject] + delay < gameData.time.xGame) ||
		 (gameData.objData.xLastAfterburnerTime [nObject] > gameData.time.xGame)) {
		DropAfterburnerBlobs (this, 1, nSize, lifetime, NULL, bSmoke);
		gameData.objData.xLastAfterburnerTime [nObject] = gameData.time.xGame;
		}
	}
}

// -----------------------------------------------------------------------------

void CObject::UpdateEffects (void)
{
	bool bNeedEffect, bHaveEffect = lightningManager.GetObjectSystem (Index ()) >= 0;
	uint8_t nEffect;

if ((info.nType == OBJ_ROBOT) && gameOpts->render.lightning.bRobots) {
	if (!ROBOTINFO (info.nId)) 
		bNeedEffect = false;
	else {
		bNeedEffect = ROBOTINFO (info.nId)->energyDrain && (gameStates.app.nSDLTicks [0] - m_xTimeEnergyDrain <= 1000);
		nEffect = ROBOT_LIGHTNING;
		}
	}
else if ((info.nType == OBJ_PLAYER) && gameOpts->render.lightning.bPlayers) {
	nEffect = PLAYER_LIGHTNING;
	int32_t nType = SEGMENT (OBJSEG (this))->m_function;
	if (gameData.FusionCharge (info.nId) > I2X (2))
		bNeedEffect = true;
	else if (nType == SEGMENT_FUNC_FUELCENTER)
#if DBG
		bNeedEffect = true;
#else
		bNeedEffect = PLAYER (info.nId).energy < I2X (100);
#endif
	else if (nType == SEGMENT_FUNC_REPAIRCENTER)
#if DBG
		bNeedEffect = true;
#else
		bNeedEffect = PLAYER (info.nId).Shield () < PLAYER (info.nId).MaxShield ();
#endif
	else
		bNeedEffect = false;
	}
else
	return;
if (bHaveEffect != bNeedEffect)
	RequestEffects (bNeedEffect ? nEffect : DESTROY_LIGHTNING);
}

// -----------------------------------------------------------------------------
//move an CObject for the current frame

int32_t CObject::Update (void)
{
	int16_t	nPrevSegment = (int16_t) info.nSegment;

#if DBG
if (OBJ_IDX (this) == nDbgObj)
	BRP;
#endif
if (!FixWeaponObject (this))
	return 0;
if (info.nType == OBJ_ROBOT) {
	if (gameOpts->gameplay.bNoThief && (!IsMultiGame || IsCoopGame) && ROBOTINFO (info.nId) && ROBOTINFO (info.nId)->thief) {
#if 1
		ApplyDamageToRobot (info.xShield + I2X (1), -1);
#else
		SetShield (0);
		UpdateLife (0);
		Die ();
#endif
		}
	else {
		fix xMaxShield = RobotDefaultShield (this);
		if (info.xShield > xMaxShield)
			SetShield (xMaxShield);
		}
	}
RepairDamage ();
HandleSegmentFunction ();
if ((info.xLifeLeft != IMMORTAL_TIME) && (info.xLifeLeft != ONE_FRAME_TIME) && (gameData.physics.xTime > 0))
	info.xLifeLeft -= (fix) (gameData.physics.xTime / gameStates.gameplay.slowmo [0].fSpeed);		//...inevitable countdown towards death
gameStates.render.bDropAfterburnerBlob = 0;
if ((gameData.physics.xTime > 0) && UpdateMovement ()) {
	UpdateEffects ();
	return 1;
	}
if (info.xLifeLeft < 0) {		// We died of old age
	Die ();
	if ((info.nType == OBJ_WEAPON) && WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (info.position.vPos);
	else if (info.nType == OBJ_ROBOT)	//make robots explode
		Explode (0);
	}
if ((info.nType == OBJ_NONE) || (info.nFlags & OF_SHOULD_BE_DEAD)) {
	return 1;			//CObject has been deleted
	}
UpdatePosition ();
UpdateEffects ();
if (CheckTriggerHits (nPrevSegment))
	return 0;
CheckWallPhysics ();
CheckGuidedMissileThroughExit (nPrevSegment);
CheckAfterburnerBlobDrop ();
return 1;
}

// -----------------------------------------------------------------------------
//move all OBJECTS for the current frame

int32_t UpdateAllObjects (void)
{
PROF_START
	CObject*	pObj;

if (gameData.objData.nLastObject [0] > gameData.objData.nMaxUsedObjects)
	FreeObjectSlots (gameData.objData.nMaxUsedObjects);		//	Free all possible CObject slots.
CleanupObjects ();
if (gameOpts->gameplay.nAutoLeveling)
	gameData.objData.pConsole->mType.physInfo.flags |= PF_LEVELLING;
else
	gameData.objData.pConsole->mType.physInfo.flags &= ~PF_LEVELLING;
gameData.physics.xTime = gameData.time.xFrame;
gameData.laser.xUpdateTime %= HOMING_MSL_FRAMETIME;
gameData.laser.xUpdateTime += gameData.time.xFrame;
// Move all OBJECTS
gameStates.entropy.bConquering = 0;
UpdatePlayerOrient ();
#if USE_OPENMP //> 1
gameData.objData.update.Reset ();
#endif
++gameData.objData.nFrameCount;
FORALL_OBJS (pObj) {
#if DBG
	if (pObj->info.nType == OBJ_REACTOR)
		BRP;
#endif
	if ((pObj->info.nType != OBJ_NONE) && (pObj->info.nType != OBJ_GHOST) && !(pObj->info.nFlags & OF_SHOULD_BE_DEAD) && !pObj->Update ()) {
		PROF_END(ptObjectStates)
		return 0;
		}
#if DBG
	if (pObj->IsRobot ()) 
		pObj->CheckSpeed ();
#endif
	}

#if USE_OPENMP //> 1
#pragma omp parallel 
if (gameStates.app.bMultiThreaded) {
	int32_t h = int32_t (gameData.objData.update.ToS ());
	#pragma omp for
	for (int32_t nThread = 0; nThread < gameStates.app.nThreads; nThread++) {
		for (int32_t i = nThread; i < h; i += gameStates.app.nThreads)
			gameData.objData.update [i]->UpdateHomingWeapon (nThread);
		}
	}
#endif

PROF_END(ptObjectStates)
return 1;
}

//----------------------------------------------------------------------------------------

//	*pViewer is a pViewer, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void WakeupRenderedObjects (CObject *pViewer, int32_t nWindow)
{
//	Make sure that we are processing current data.
if (gameData.app.nFrameCount != windowRenderedData [nWindow].nFrame) {
#if TRACE
		console.printf (1, "Warning: Called WakeupRenderedObjects with a bogus window.\n");
#endif
	return;
	}

gameData.ai.nLastMissileCamera = OBJ_IDX (pViewer);
int32_t frameFilter = gameData.app.nFrameCount & 3;
for (int32_t i = windowRenderedData [nWindow].nObjects; i; ) {
	int32_t nObject = windowRenderedData [nWindow].renderedObjects [--i];
	if ((nObject & 3) == frameFilter) {
		CObject* pObj = OBJECT (nObject);
		if (pObj->info.nType == OBJ_ROBOT) {
			if (CFixVector::Dist (pViewer->info.position.vPos, pObj->info.position.vPos) < I2X (100)) {
				tAILocalInfo* pLocalInfo = gameData.ai.localInfo + nObject;
				if (pLocalInfo->targetAwarenessType == 0) {
					pObj->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
					pLocalInfo->targetAwarenessType = WEAPON_ROBOT_COLLISION;
					pLocalInfo->targetAwarenessTime = I2X (3);
					pLocalInfo->nPrevVisibility = 2;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void CleanupObjects (void)
{
	CObject*		pObj, *pPrevObj;
	int32_t		nLocalDeadPlayerObj = -1;

for (CObjectIterator iter (pObj); pObj; pObj = pPrevObj ? iter.Step () : iter.Start ()) {
	pPrevObj = pObj;
	if (pObj->info.nType == OBJ_NONE)
		continue;
	if (!(pObj->info.nFlags & OF_SHOULD_BE_DEAD))
		continue;
	if (pObj->info.nType == OBJ_PLAYER) {
		if (pObj->info.nId == N_LOCALPLAYER) {
			if (nLocalDeadPlayerObj == -1) {
				StartPlayerDeathSequence (pObj);
				nLocalDeadPlayerObj = pObj->Index ();
				}
			}
		}
	else {
		CObject* pPrevObj = iter.Back ();
		if ((pObj->info.nType == OBJ_ROBOT) ||
			 (pObj->info.nType == OBJ_DEBRIS) ||	// exploded robot
			 (pObj->info.nType == OBJ_REACTOR) ||
			 (pObj->info.nType == OBJ_POWERUP) ||
			 (pObj->info.nType == OBJ_HOSTAGE))
			ExecObjTriggers (pObj->Index (), 0);
		ReleaseObject (pObj->Index ());
		}
	}
}

//	-----------------------------------------------------------------------------------------------------------

#if DBG

void CObject::UpdateLife (fix xLife)
{
info.xLifeLeft = xLife;
if (Index () == nDbgObj)
	BRP;
}


void CObject::SetLife (fix xLife)
{
UpdateLife (m_xTotalLife = xLife);
}

#endif

//	-----------------------------------------------------------------------------------------------------------
//eof
