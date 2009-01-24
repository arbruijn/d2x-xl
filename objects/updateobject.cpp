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

#include "inferno.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"
#include "lightning.h"
#include "objsmoke.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "fireball.h"
#include "error.h"
#include "endlevel.h"
#include "timer.h"
#include "gameseg.h"
#include "collide.h"
#include "dynlight.h"
#include "interp.h"
#include "newdemo.h"
#include "gauges.h"
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
Controls [0].headingTime = 0;
Controls [0].pitchTime = 0;
Controls [0].bankTime = 0;
Controls [0].verticalThrustTime = 0;
Controls [0].sidewaysThrustTime = 0;
Controls [0].forwardThrustTime = 0;
objP->mType.physInfo.rotThrust.SetZero ();
objP->mType.physInfo.thrust.SetZero ();
objP->mType.physInfo.velocity.SetZero ();
objP->mType.physInfo.rotVel.SetZero ();
}

//------------------------------------------------------------------------------

void StopPlayerMovement (void)
{
if (!gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted) {
	StopObjectMovement (OBJECTS + LOCALPLAYER.nObject);
	memset (&gameData.physics.playerThrust, 0, sizeof (gameData.physics.playerThrust));
//	gameData.time.xFrame = I2X (1);
	gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted = 0;
	}
}

//------------------------------------------------------------------------------

void CObject::Die (void)
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
	CAngleVector rotangs;
	CFixMatrix rotmat, new_pm;

Assert (info.movementType == MT_SPINNING);
rotangs = CAngleVector::Create((fixang) FixMul (mType.spinRate [X], gameData.time.xFrame),
										 (fixang) FixMul (mType.spinRate [Y], gameData.time.xFrame),
										 (fixang) FixMul (mType.spinRate [Z], gameData.time.xFrame));
rotmat = CFixMatrix::Create (rotangs);
new_pm = info.position.mOrient * rotmat;
info.position.mOrient = new_pm;
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
if (EGI_FLAG (bRotateMarkers, 0, 1, 0) && gameStates.app.tick40fps.bTick) {
	static time_t	t0 = 0;

	time_t t = (gameStates.app.nSDLTicks - t0) % 1000;
	t0 = gameStates.app.nSDLTicks;
	if (t) {
		CAngleVector a = CAngleVector::Create(0, 0, (fixang) ((float) (I2X (1) / 512) * t / 25.0f));
		CFixMatrix mRotate = CFixMatrix::Create(a);
		CFixMatrix mOrient = mRotate * info.position.mOrient;
		info.position.mOrient = mOrient;
		}
	}
}

// -----------------------------------------------------------------------------

int CObject::CheckWallPhysics (void)
{
	int	nType = 0, sideMask;

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
	HUDMessage (0, "Thrust: %d %d %d", mType.physInfo.thrust [X], mType.physInfo.thrust [Y], mType.physInfo.thrust [Z]);
	if (nType == 2)
		nSound = mType.physInfo.thrust.IsZero () ? 0 : SOUND_SHIP_IN_WATER;
	else
		nSound = (nType & 1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
	if (nSound != gameData.objs.nSoundPlaying [info.nId]) {
		if (gameData.objs.nSoundPlaying [info.nId])
			audio.DestroyObjectSound (OBJ_IDX (this));
		if (nSound)
			audio.CreateObjectSound (nSound, SOUNDCLASS_GENERIC, OBJ_IDX (this), 1);
		gameData.objs.nSoundPlaying [info.nId] = nSound;
		}
	}
else if (gameData.objs.nSoundPlaying [info.nId]) {
	audio.DestroyObjectSound (OBJ_IDX (this));
	gameData.objs.nSoundPlaying [info.nId] = 0;
	}
return nType;
}

// -----------------------------------------------------------------------------

void CObject::HandleSpecialSegment (void)
{
if ((info.nType == OBJ_PLAYER) && (gameData.multiplayer.nLocalPlayer == info.nId)) {
	CSegment*		segP = SEGMENTS + info.nSegment;
	CPlayerData*	playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
	fix				shields, fuel;

   if (gameData.app.nGameMode & GM_CAPTURE)
		 segP->CheckForGoal ();
   else if (gameData.app.nGameMode & GM_HOARD)
		 segP->CheckForHoardGoal ();
	else if (Controls [0].forwardThrustTime || Controls [0].verticalThrustTime || Controls [0].sidewaysThrustTime) {
		gameStates.entropy.nTimeLastMoved = -1;
		if ((gameData.app.nGameMode & GM_ENTROPY) &&
			 ((segP->m_owner < 0) || (segP->m_owner == GetTeam (gameData.multiplayer.nLocalPlayer) + 1))) {
			StopConquerWarning ();
			}
		}
	else if (gameStates.entropy.nTimeLastMoved < 0)
		gameStates.entropy.nTimeLastMoved = 0;

	shields = segP->Damage (playerP->shields + 1);
	if (shields > 0) {
		playerP->shields -= shields;
		MultiSendShields ();
		if (playerP->shields < 0)
			StartPlayerDeathSequence (this);
		else
			segP->ConquerCheck ();
		}
	else {
		StopConquerWarning ();
		fuel = segP->Refuel (INITIAL_ENERGY - playerP->energy);
		if (fuel > 0)
			playerP->energy += fuel;
		shields = segP->Repair (INITIAL_SHIELDS - playerP->shields);
		if (shields > 0) {
			playerP->shields += shields;
			MultiSendShields ();
			}
		if (!segP->m_owner)
			segP->ConquerCheck ();
		}
	}
}

// -----------------------------------------------------------------------------

int CObject::UpdateControl (void)
{
switch (info.controlType) {
	case CT_NONE:
		break;

	case CT_FLYING:
		ReadFlyingControls (this);
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
		if (gameStates.gameplay.bNoBotAI || (gameStates.app.bGameSuspended & SUSP_ROBOTS)) {
			mType.physInfo.velocity.SetZero ();
			mType.physInfo.thrust.SetZero ();
			mType.physInfo.rotThrust.SetZero ();
			DoAnyRobotDyingFrame (this);
#if 1//ndef _DEBUG
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
		LaserDoWeaponSequence (this);
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
		if (gameStates.gameplay.bNoBotAI)
			return 1;
		DoReactorFrame (this);
		break;

	default:
		Error ("Unknown control nType %d in CObject %i, sig/nType/id = %i/%i/%i", info.controlType, OBJ_IDX (this), info.nSignature, info.nType, info.nId);
	}
return 0;
}

// -----------------------------------------------------------------------------

void CObject::UpdateShipSound (void)
{
if (!gameOpts->sound.bShip)
	return;

int nObject = OBJ_IDX (this);
if (nObject != LOCALPLAYER.nObject)
	return;

int nSpeed = mType.physInfo.velocity.Mag();
nSpeed -= I2X (2);
if (nSpeed < 0)
	nSpeed = 0;
if (gameData.multiplayer.bMoving == nSpeed)
	return;

if (gameData.multiplayer.bMoving < 0)
	audio.CreateObjectSound (-1, SOUNDCLASS_PLAYER, OBJ_IDX (this), 1, I2X (1) / 64 + nSpeed / 256, I2X (256), -1, -1, "missileflight-small.wav", 1);
else
	audio.ChangeObjectSound (OBJ_IDX (this), I2X (1) / 64 + nSpeed / 256);
gameData.multiplayer.bMoving = nSpeed;
}

// -----------------------------------------------------------------------------

void CObject::UpdateMovement (void)
{
if (info.nType == OBJ_MARKER)
	RotateMarker ();

switch (info.movementType) {
	case MT_NONE:
		break;								//this doesn't move

	case MT_PHYSICS:
		DoPhysicsSim ();
		lightManager.SetPos (OBJ_IDX (this));
		lightningManager.MoveForObject (this);
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
nOldLevel = gameData.missions.nCurrentLevel;
for (i = 0; i < gameData.physics.nSegments - 1; i++) {
#if DBG
	if (gameData.physics.segments [i] > gameData.segs.nLastSegment)
		PrintLog ("invalid segment in gameData.physics.segments\n");
#endif
	nConnSide = SEGMENTS [gameData.physics.segments [i + 1]].ConnectedSide (SEGMENTS + gameData.physics.segments [i]);
	if (nConnSide != -1)
		SEGMENTS [gameData.physics.segments [i]].OperateTrigger (nConnSide, this, 0);
#if DBG
	else	// segments are not directly connected, so do binary subdivision until you find connected segments.
		PrintLog ("UNCONNECTED SEGMENTS %d, %d\n", gameData.physics.segments [i+1], gameData.physics.segments [i]);
#endif
	//maybe we've gone on to the next level.  if so, bail!
	if (gameData.missions.nCurrentLevel != nOldLevel)
		return 1;
	}
return 0;
}

// -----------------------------------------------------------------------------

void CObject::CheckGuidedMissileThroughExit (short nPrevSegment)
{
if ((this == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP) &&
	 (info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature)) {
	if (nPrevSegment != info.nSegment) {
		short	nConnSide = SEGMENTS [info.nSegment].ConnectedSide (SEGMENTS + nPrevSegment);
		if (nConnSide != -1) {
			CTrigger* trigP = SEGMENTS [nPrevSegment].Trigger (nConnSide);
			if (trigP && (trigP->nType == TT_EXIT))
				gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP->info.xLifeLeft = 0;
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
		MultiSendDropBlobs ((char) gameData.multiplayer.nLocalPlayer);
	gameStates.render.bDropAfterburnerBlob = 0;
	}

if ((info.nType == OBJ_WEAPON) && (gameData.weapons.info [info.nId].nAfterburnerSize)) {
	int	nObject, bSmoke;
	fix	vel;
	fix	delay, lifetime, nSize;

#if 1
	if ((info.nType == OBJ_WEAPON) && gameData.objs.bIsMissile [info.nId]) {
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
if (info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (info.nId).energyDrain) {
		RequestEffects (ROBOT_LIGHTNINGS);
		}
	}
else if ((info.nType == OBJ_PLAYER) && gameOpts->render.lightnings.bPlayers) {
	int nType = SEGMENTS [info.nSegment].m_nType;
	if (nType == SEGMENT_IS_FUELCEN)
		RequestEffects (PLAYER_LIGHTNINGS);
	else if (nType == SEGMENT_IS_REPAIRCEN)
		RequestEffects (PLAYER_LIGHTNINGS);
	else
		RequestEffects (DESTROY_LIGHTNINGS);
	}
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
		info.xShields = 0;
		info.xLifeLeft = 0;
		Die ();
		}
	else {
		fix xMaxShields = RobotDefaultShields (this);
		if (info.xShields > xMaxShields)
			info.xShields = xMaxShields;
		}
	}
info.vLastPos = info.position.vPos;			// Save the current position
HandleSpecialSegment ();
if ((info.xLifeLeft != IMMORTAL_TIME) &&
	 (info.xLifeLeft != ONE_FRAME_TIME) &&
	 (gameData.physics.xTime != I2X (1)))
	info.xLifeLeft -= (fix) (gameData.physics.xTime / gameStates.gameplay.slowmo [0].fSpeed);		//...inevitable countdown towards death
gameStates.render.bDropAfterburnerBlob = 0;
if (UpdateControl ()) {
	UpdateEffects ();
	return 1;
	}
if (info.xLifeLeft < 0) {		// We died of old age
	Die ();
	if ((info.nType == OBJ_WEAPON) && WI_damage_radius (info.nId))
		ExplodeBadassWeapon (info.position.vPos);
	else if (info.nType == OBJ_ROBOT)	//make robots explode
		Explode (0);
	}
if ((info.nType == OBJ_NONE) || (info.nFlags & OF_SHOULD_BE_DEAD)) {
	return 1;			//CObject has been deleted
	}
UpdateMovement ();
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
	int i;
	CObject *objP, *nextObjP;

gameData.objs.nFrameCount++;
if (gameData.objs.nLastObject [0] > gameData.objs.nMaxUsedObjects)
	FreeObjectSlots (gameData.objs.nMaxUsedObjects);		//	Free all possible CObject slots.
#if LIMIT_PHYSICS_FPS
if (!gameStates.app.tick60fps.bTick)
	return 1;
gameData.physics.xTime = SECS2X (gameStates.app.tick60fps.nTime);
#else
gameData.physics.xTime = gameData.time.xFrame;
#endif
CleanupObjects ();
if (gameOpts->gameplay.nAutoLeveling)
	gameData.objs.consoleP->mType.physInfo.flags |= PF_LEVELLING;
else
	gameData.objs.consoleP->mType.physInfo.flags &= ~PF_LEVELLING;

// Move all OBJECTS
gameStates.entropy.bConquering = 0;
UpdatePlayerOrient ();
i = 0;
for (objP = gameData.objs.lists.all.head; objP; objP = nextObjP) {
	nextObjP = objP->Links (0).next;
	if ((objP->info.nType != OBJ_NONE) && (objP->info.nType != OBJ_GHOST) && !(objP->info.nFlags & OF_SHOULD_BE_DEAD) && !objP->Update ())
		return 0;
	i++;
	}
return 1;
}

//----------------------------------------------------------------------------------------

//	*viewerP is a viewerP, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void WakeupRenderedObjects (CObject *viewerP, int nWindow)
{
	int	i;

	//	Make sure that we are processing current data.
if (gameData.app.nFrameCount != windowRenderedData [nWindow].nFrame) {
#if TRACE
		console.printf (1, "Warning: Called WakeupRenderedObjects with a bogus window.\n");
#endif
	return;
	}
gameData.ai.nLastMissileCamera = OBJ_IDX (viewerP);

int nObject, frameFilter = gameData.app.nFrameCount & 3;
CObject *objP;
tAILocalInfo *ailP;

for (i = windowRenderedData [nWindow].nObjects; i; ) {
	nObject = windowRenderedData [nWindow].renderedObjects [--i];
	if ((nObject & 3) == frameFilter) {
		objP = OBJECTS + nObject;
		if (objP->info.nType == OBJ_ROBOT) {
			if (CFixVector::Dist (viewerP->info.position.vPos, objP->info.position.vPos) < I2X (100)) {
				ailP = gameData.ai.localInfo + nObject;
				if (ailP->playerAwarenessType == 0) {
					objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
					ailP->playerAwarenessType = WEAPON_ROBOT_COLLISION;
					ailP->playerAwarenessTime = I2X (3);
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
	Assert ((objP->info.nType != OBJ_FIREBALL) || (objP->cType.explInfo.nDeleteTime == -1));
	if (objP->info.nType != OBJ_PLAYER)
		ReleaseObject (objP->Index ());
	else {
		if (objP->info.nId == gameData.multiplayer.nLocalPlayer) {
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
//eof
