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
#ifdef TACTILE
#	include "tactile.h"
#endif
#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

#define	DEG90		 (I2X (1) / 4)
#define	DEG45		 (I2X (1) / 8)
#define	DEG1		 (I2X (1) / (4 * 90))

//------------------------------------------------------------------------------

void StopObjectMovement (CObject *objP)
{
controls [0].headingTime = 0;
controls [0].pitchTime = 0;
controls [0].bankTime = 0;
controls [0].verticalThrustTime = 0;
controls [0].sidewaysThrustTime = 0;
controls [0].forwardThrustTime = 0;
if (objP) {
	objP->mType.physInfo.rotThrust.SetZero ();
	objP->mType.physInfo.thrust.SetZero ();
	objP->mType.physInfo.velocity.SetZero ();
	objP->mType.physInfo.rotVel.SetZero ();
	}
}

//------------------------------------------------------------------------------

void StopPlayerMovement (void)
{
if (OBJECTS.Buffer () && !(gameData.objs.speedBoost.Buffer () && gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted)) {
	StopObjectMovement (OBJECTS + LOCALPLAYER.nObject);
	memset (&gameData.physics.playerThrust, 0, sizeof (gameData.physics.playerThrust));
//	gameData.time.SetTime (I2X (1));
	gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted = 0;
	}
}

//------------------------------------------------------------------------------

void CObject::Die (void)
{
info.nFlags |= OF_SHOULD_BE_DEAD;
#if DBG
if (Index () == nDbgObj)
	nDbgObj = nDbgObj;
if (this == dbgObjP)
	dbgObjP = dbgObjP;
#endif
if (IsMultiGame && (gameStates.multi.nGameType == UDP_GAME) && (Type () == OBJ_POWERUP)) {
	if (!IsMissile ()) 
		RemovePowerupInMine (Id ());
	else {
		int i = FindDropInfo (Signature ());
		if (i >= 0)
			DelDropInfo (i);
#if DBG
		else
			FindDropInfo (Signature ());
#endif
	}
}

//------------------------------------------------------------------------------

void CObject::MultiDie (void)
{
info.nFlags |= OF_SHOULD_BE_DEAD;
#if DBG
if (this == dbgObjP)
	dbgObjP = dbgObjP;
#endif
}

//------------------------------------------------------------------------------
//process a continuously-spinning CObject
void CObject::Spin (void)
{
	CAngleVector vRot;
	CFixMatrix mRot, mOrient;

Assert (info.movementType == MT_SPINNING);
vRot = CAngleVector::Create((fixang) FixMul (mType.spinRate.v.coord.x, gameData.time.xFrame),
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

int CObject::CheckWallPhysics (void)
{
	int	nType = 0, sideMask, nSoundObj;
	short nObject = OBJ_IDX (this);

if (info.nType != OBJ_PLAYER)
	return 0;
sideMask = SEGMENTS [info.nSegment].Masks (info.position.vPos, info.xSize).m_side;
if (sideMask) {
	short		nSide;
	int		bit;
	for (nSide = 0, bit = 1; nSide < 6; bit <<= 1, nSide++)
		if ((sideMask & bit) && (nType = ApplyWallPhysics (info.nSegment, nSide)))
			break;
	}
if (!nType)
	nType = CheckSegmentPhysics ();
// type 1,2: segment related sound
// type 2,3: sound caused by ship touching a wall
// type & 1: lava
// type & 2: water
if (nType) {
	short nSound;

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
	CSegment*		segP = SEGMENTS + info.nSegment;
	CPlayerData*	playerP = gameData.multiplayer.players + N_LOCALPLAYER;

   if (gameData.app.GameMode (GM_CAPTURE))
		 segP->CheckForGoal ();
   else if (IsHoardGame)
		 segP->CheckForHoardGoal ();
	else if (controls [0].forwardThrustTime || controls [0].verticalThrustTime || controls [0].sidewaysThrustTime) {
		gameStates.entropy.nTimeLastMoved = -1;
		if (IsEntropyGame &&
			 ((segP->m_owner < 0) || (segP->m_owner == GetTeam (N_LOCALPLAYER) + 1))) {
			StopConquerWarning ();
			}
		}
	else if (gameStates.entropy.nTimeLastMoved < 0)
		gameStates.entropy.nTimeLastMoved = 0;

	playerP->UpdateShield (-segP->ShieldDamage (playerP->Shield () + 1));
	playerP->UpdateEnergy (-segP->EnergyDamage (playerP->energy));
	if (playerP->Shield () < 0)
		StartPlayerDeathSequence (this);
	else {
		segP->ConquerCheck ();
		//CObject* objP = OBJECTS + playerP->nObject;
		fix energy = segP->Refuel (fix (playerP->InitialEnergy ()) - playerP->Energy ());
		if (energy > 0)
			playerP->UpdateEnergy (energy);
		fix shield = segP->Repair (fix (playerP->InitialShield ()) - playerP->Shield ());
		if (shield > 0) {
			playerP->UpdateShield (shield);
			}
		}
	}
}

// -----------------------------------------------------------------------------

int CObject::UpdateMovement (void)
{
switch (info.controlType) {
	case CT_NONE:
		break;

	case CT_FLYING:
		ApplyFlightControls ();
		break;

	case CT_REPAIRCEN:
		Int3 ();	// -- hey!these are no longer supported!!-- do_repair_sequence (this); break;

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

static inline int ShipVolume (int nSpeed)
{
return I2X (1) / 16 + nSpeed / 512;
}

void CObject::UpdateShipSound (void)
{
if (!gameOpts->sound.bShip)
	return;

int nObject = OBJ_IDX (this);
if (nObject != LOCALPLAYER.nObject)
	return;

int nSpeed = mType.physInfo.velocity.Mag();
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

int CObject::CheckTriggerHits (short nPrevSegment)
{
		short	nConnSide, i;
		int	nOldLevel;

if ((info.movementType != MT_PHYSICS) || (nPrevSegment == info.nSegment))
	return 0;
nOldLevel = missionManager.nCurrentLevel;
for (i = 0; i < gameData.physics.nSegments - 1; i++) {
	if (gameData.physics.segments [i] < 0)
		continue;
	if (gameData.physics.segments [i + 1] < 0)
		continue;
#if DBG
	if (gameData.physics.segments [i] > gameData.segs.nLastSegment)
		PrintLog (0, "invalid segment in gameData.physics.segments\n");
#endif
	nConnSide = SEGMENTS [gameData.physics.segments [i + 1]].ConnectedSide (SEGMENTS + gameData.physics.segments [i]);
	if (nConnSide != -1)
		SEGMENTS [gameData.physics.segments [i]].OperateTrigger (nConnSide, this, 0);
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

void CObject::CheckGuidedMissileThroughExit (short nPrevSegment)
{
if ((this == gameData.objs.guidedMissile [N_LOCALPLAYER].objP) &&
	 (info.nSignature == gameData.objs.guidedMissile [N_LOCALPLAYER].nSignature)) {
	if (nPrevSegment != info.nSegment) {
		short	nConnSide = SEGMENTS [info.nSegment].ConnectedSide (SEGMENTS + nPrevSegment);
		if (nConnSide != -1) {
			CTrigger* trigP = SEGMENTS [nPrevSegment].Trigger (nConnSide);
			if (trigP && (trigP->m_info.nType == TT_EXIT))
				gameData.objs.guidedMissile [N_LOCALPLAYER].objP->SetLife (0);
			}
		}
	}
}

// -----------------------------------------------------------------------------

void CObject::CheckAfterburnerBlobDrop (void)
{
if (gameStates.render.bDropAfterburnerBlob) {
	Assert (this == gameData.objs.consoleP);
	DropAfterburnerBlobs (this, 2, I2X (5) / 2, -1, NULL, 0);	//	-1 means use default lifetime
	if (IsMultiGame)
		MultiSendDropBlobs ((char) N_LOCALPLAYER);
	gameStates.render.bDropAfterburnerBlob = 0;
	}

if ((info.nType == OBJ_WEAPON) && (gameData.weapons.info [info.nId].nAfterburnerSize)) {
	int	nObject, bSmoke;
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
		 (gameData.objs.xLastAfterburnerTime [nObject] + delay < gameData.time.xGame) ||
		 (gameData.objs.xLastAfterburnerTime [nObject] > gameData.time.xGame)) {
		DropAfterburnerBlobs (this, 1, nSize, lifetime, NULL, bSmoke);
		gameData.objs.xLastAfterburnerTime [nObject] = gameData.time.xGame;
		}
	}
}

// -----------------------------------------------------------------------------

void CObject::UpdateEffects (void)
{
	bool bNeedEffect, bHaveEffect = lightningManager.GetObjectSystem (Index ()) >= 0;
	ubyte nEffect;

if ((info.nType == OBJ_ROBOT) && gameOpts->render.lightning.bRobots) {
	bNeedEffect = ROBOTINFO (info.nId).energyDrain && (gameStates.app.nSDLTicks [0] - m_xTimeEnergyDrain <= 1000);
	nEffect = ROBOT_LIGHTNING;
	}
else if ((info.nType == OBJ_PLAYER) && gameOpts->render.lightning.bPlayers) {
	nEffect = PLAYER_LIGHTNING;
	int nType = SEGMENTS [OBJSEG (this)].m_function;
	if (gameData.FusionCharge (info.nId) > I2X (2))
		bNeedEffect = true;
	else if (nType == SEGMENT_FUNC_FUELCEN)
#if DBG
		bNeedEffect = true;
#else
		bNeedEffect = gameData.multiplayer.players [info.nId].energy < I2X (100);
#endif
	else if (nType == SEGMENT_FUNC_REPAIRCEN)
#if DBG
		bNeedEffect = true;
#else
		bNeedEffect = gameData.multiplayer.players [info.nId].Shield () < gameData.multiplayer.players [info.nId].MaxShield ();
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

int CObject::Update (void)
{
	short	nPrevSegment = (short) info.nSegment;

#if DBG
if ((info.nType == OBJ_WEAPON) && (info.nId == SMARTMINE_BLOB_ID)) {
	nDbgObj = OBJ_IDX (this);
	nDbgObj = nDbgObj;
	}
if (OBJ_IDX (this) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (info.nType == OBJ_ROBOT) {
	if (gameOpts->gameplay.bNoThief && (!IsMultiGame || IsCoopGame) && ROBOTINFO (info.nId).thief) {
#if 1
		ApplyDamageToRobot (info.xShield + I2X (1), -1);
#else
		SetShield (0);
		SetLife (0);
		Die ();
#endif
		}
	else {
		fix xMaxShield = RobotDefaultShield (this);
		if (info.xShield > xMaxShield)
			SetShield (xMaxShield);
		}
	}
info.vLastPos = info.position.vPos;			// Save the current position
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

int UpdateAllObjects (void)
{
PROF_START
	CObject*	objP, * nextObjP;

if (gameData.objs.nLastObject [0] > gameData.objs.nMaxUsedObjects)
	FreeObjectSlots (gameData.objs.nMaxUsedObjects);		//	Free all possible CObject slots.
CleanupObjects ();
if (gameOpts->gameplay.nAutoLeveling)
	gameData.objs.consoleP->mType.physInfo.flags |= PF_LEVELLING;
else
	gameData.objs.consoleP->mType.physInfo.flags &= ~PF_LEVELLING;
gameData.physics.xTime = gameData.time.xFrame;
gameData.laser.xUpdateTime %= I2X (1) / 40;
gameData.laser.xUpdateTime += gameData.time.xFrame;
// Move all OBJECTS
gameStates.entropy.bConquering = 0;
UpdatePlayerOrient ();
//WaitForEffectsThread ();
#if USE_OPENMP //> 1
gameData.objs.update.Reset ();
#endif
++gameData.objs.nFrameCount;
for (objP = gameData.objs.lists.all.head; objP; objP = nextObjP) {
	nextObjP = objP->Links (0).next;
	if ((objP->info.nType != OBJ_NONE) && (objP->info.nType != OBJ_GHOST) && !(objP->info.nFlags & OF_SHOULD_BE_DEAD) && !objP->Update ()) {
		return 0;
		PROF_END(ptUpdateObjects)
		}
	}

#if USE_OPENMP //> 1
#pragma omp parallel 
if (gameStates.app.bMultiThreaded) {
	int h = int (gameData.objs.update.ToS ());
	#pragma omp for
	for (int nThread = 0; nThread < gameStates.app.nThreads; nThread++) {
		for (int i = nThread; i < h; i += gameStates.app.nThreads)
			gameData.objs.update [i]->UpdateHomingWeapon (nThread);
		}
	}
#endif

PROF_END(ptUpdateObjects)
return 1;
}

//----------------------------------------------------------------------------------------

//	*viewerP is a viewerP, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void WakeupRenderedObjects (CObject *viewerP, int nWindow)
{
//	Make sure that we are processing current data.
if (gameData.app.nFrameCount != windowRenderedData [nWindow].nFrame) {
#if TRACE
		console.printf (1, "Warning: Called WakeupRenderedObjects with a bogus window.\n");
#endif
	return;
	}

gameData.ai.nLastMissileCamera = OBJ_IDX (viewerP);
int frameFilter = gameData.app.nFrameCount & 3;
for (int i = windowRenderedData [nWindow].nObjects; i; ) {
	int nObject = windowRenderedData [nWindow].renderedObjects [--i];
	if ((nObject & 3) == frameFilter) {
		CObject* objP = OBJECTS + nObject;
		if (objP->info.nType == OBJ_ROBOT) {
			if (CFixVector::Dist (viewerP->info.position.vPos, objP->info.position.vPos) < I2X (100)) {
				tAILocalInfo* ailP = gameData.ai.localInfo + nObject;
				if (ailP->targetAwarenessType == 0) {
					objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
					ailP->targetAwarenessType = WEAPON_ROBOT_COLLISION;
					ailP->targetAwarenessTime = I2X (3);
					ailP->nPrevVisibility = 2;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void CleanupObjects (void)
{
	CObject	*objP, *nextObjP = NULL;
	int		nLocalDeadPlayerObj = -1;

for (objP = gameData.objs.lists.all.head; objP; objP = nextObjP) {
	nextObjP = objP->Links (0).next;
	if (objP->info.nType == OBJ_NONE)
		continue;
	if (!(objP->info.nFlags & OF_SHOULD_BE_DEAD))
		continue;
	//Assert ((objP->info.nType != OBJ_FIREBALL) || (objP->cType.explInfo.nDeleteTime == -1));
	if (objP->info.nType != OBJ_PLAYER)
		ReleaseObject (objP->Index ());
	else {
		if (objP->info.nId == N_LOCALPLAYER) {
			if (nLocalDeadPlayerObj == -1) {
				StartPlayerDeathSequence (objP);
				nLocalDeadPlayerObj = objP->Index ();
				}
			else
				Int3 ();
			}
		}
	}
}

//	-----------------------------------------------------------------------------------------------------------

#if DBG
void CObject::SetLife (fix xLife)
{
info.xLifeLeft = xLife;
if (Index () == nDbgObj)
	nDbgObj = nDbgObj;
}
#endif

//	-----------------------------------------------------------------------------------------------------------
//eof
