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

void NDRecordGuidedStart (void);

//---------------------------------------------------------------------------------
//creates a weapon CObject
int32_t CreateWeaponObject (uint8_t nWeaponType, int16_t nSegment, CFixVector *vPosition, int16_t nParent)
{
	int32_t	nObject;
	CObject	*objP;

switch (gameData.weapons.info [nWeaponType].renderType) {
	case WEAPON_RENDER_BLOB:
	case WEAPON_RENDER_POLYMODEL:
	case WEAPON_RENDER_LASER:	// Not supported anymore
	case WEAPON_RENDER_NONE:
	case WEAPON_RENDER_VCLIP:
		break;
	default:
		Error ("Invalid weapon render nType in CreateNewWeapon\n");
		return -1;
	}

nObject = CreateWeapon (nWeaponType, nParent, nSegment, *vPosition, 0, 255);
objP = OBJECT (nObject);
if (!objP) {
#if DBG
	if (nObject > -1)
		BRP;
#endif
	return -1;
	}
#if DBG
if (nObject == nDbgObj)
	BRP;
#endif
if (gameData.weapons.info [nWeaponType].renderType == WEAPON_RENDER_POLYMODEL) {
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->info.nId].nModel;
	objP->AdjustSize (1, gameData.weapons.info [objP->info.nId].poLenToWidthRatio);
	}
else if (EGI_FLAG (bTracers, 0, 1, 0) && objP->IsGatlingRound ()) {
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [SUPERLASER_ID + 1].nModel;
	objP->rType.polyObjInfo.nTexOverride = -1;
	objP->rType.polyObjInfo.nAltTextures = 0;
	objP->AdjustSize (1, gameData.weapons.info [SUPERLASER_ID].poLenToWidthRatio);
	objP->info.renderType = RT_POLYOBJ;
	}
objP->mType.physInfo.mass = WI_mass (nWeaponType);
objP->mType.physInfo.drag = WI_drag (nWeaponType);
objP->mType.physInfo.thrust.SetZero ();
if (gameData.weapons.info [nWeaponType].bounce == 1)
	objP->mType.physInfo.flags |= PF_BOUNCES;
if ((gameData.weapons.info [nWeaponType].bounce == 2) || gameStates.app.cheats.bBouncingWeapons)
	objP->mType.physInfo.flags |= PF_BOUNCES + PF_BOUNCES_TWICE;
return nObject;
}

// ---------------------------------------------------------------------------------

int32_t CreateWeaponSpeed (CObject* weaponP, bool bFix)
{
	uint8_t			nWeaponType = weaponP->Id ();
	CWeaponInfo&	weaponInfo = gameData.weapons.info [nWeaponType];
	CObject			*parentP = OBJECT (weaponP->cType.laserInfo.parent.nObject);
	fix				xParentSpeed;
	fix				xLaserLength = (weaponInfo.renderType == WEAPON_RENDER_POLYMODEL) ? gameData.models.polyModels [0][weaponP->ModelId ()].Rad () * 2 : 0;
	CFixVector		vDir = weaponP->Heading ();

//	Here's where to fix the problem with OBJECTS which are moving backwards imparting higher velocity to their weaponfire.
//	Find out if moving backwards.
if (!parentP || !CObject::IsMine (nWeaponType))
	xParentSpeed = 0;
else {
	xParentSpeed = parentP->mType.physInfo.velocity.Mag ();
	if (CFixVector::Dot (parentP->mType.physInfo.velocity, parentP->info.position.mOrient.m.dir.f) < 0)
		xParentSpeed = -xParentSpeed;
	}

//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
// This also jitters the laser a bit so that it doesn't alias.
//	Don't do for weapons created by weapons.
if (!bFix && parentP && (parentP->info.nType == OBJ_PLAYER) && (gameData.weapons.info [nWeaponType].renderType != WEAPON_RENDER_NONE) && (nWeaponType != FLARE_ID) && !CObject::IsMissile (nWeaponType)) {
#if 1
	weaponP->mType.physInfo.velocity = vDir * (gameData.laser.nOffset + xLaserLength / 2);
	if (weaponP->Velocity ().IsZero ()) 
		PrintLog (0, "weapon parent has no velocity\n");
	else {
		fix xTime = gameData.physics.xTime;
		gameData.physics.xTime = -1;	// make it move the entire velocity vector
		weaponP->Update ();
		gameData.physics.xTime = xTime;
		if (weaponP->info.nFlags & OF_SHOULD_BE_DEAD) {
			ReleaseObject (weaponP->Index ());
			return -1;
			}
		}
#endif
	}

fix xWeaponSpeed = WI_speed (weaponP->info.nId, gameStates.app.nDifficultyLevel);
if (weaponInfo.speedvar != 128) {
	fix randval = I2X (1) - ((RandShort () * weaponInfo.speedvar) >> 6);	//	Get a scale factor between speedvar% and 1.0.
	xWeaponSpeed = FixMul (xWeaponSpeed, randval);
	}
//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
if ((nWeaponType == SMARTMSL_BLOB_ID) ||
	 (nWeaponType == SMARTMINE_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMSL_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMINE_BLOB_ID) ||
	 (nWeaponType == EARTHSHAKER_MEGA_ID))
	xWeaponSpeed /= 4;
if (WIThrust (nWeaponType) != 0)
	xWeaponSpeed /= 2;
/*test*/weaponP->mType.physInfo.velocity = vDir * (xWeaponSpeed + xParentSpeed);
if (parentP)
	weaponP->SetStartVel (&parentP->mType.physInfo.velocity);
//	Set thrust
if (WIThrust (nWeaponType) != 0) {
	weaponP->mType.physInfo.thrust = weaponP->mType.physInfo.velocity;
	weaponP->mType.physInfo.thrust *= FixDiv (WIThrust (weaponP->info.nId), xWeaponSpeed + xParentSpeed);
	}
return weaponP->Index ();
}

// ---------------------------------------------------------------------------------

void CreateWeaponSound (CObject* weaponP, int32_t bMakeSound)
{
	uint8_t			nWeaponType = weaponP->Id ();
	CWeaponInfo&	weaponInfo = gameData.weapons.info [nWeaponType];
	fix				volume = I2X (1);
	int32_t			nParent = weaponP->cType.laserInfo.parent.nObject;
	CObject			*parentP = OBJECT (nParent);
	int32_t			nWeapon = weaponP->Index ();
	int32_t			nViewer = OBJ_IDX (gameData.objData.viewerP);

	static int32_t	nMslSounds [2] = {SND_ADDON_MISSILE_SMALL, SND_ADDON_MISSILE_BIG};
	static int32_t	nGatlingSounds [2] = {SND_ADDON_VULCAN, SND_ADDON_GAUSS};

if (bMakeSound && (weaponInfo.flashSound > -1)) {
	int32_t bGatling = weaponP->IsGatlingRound ();
	if (parentP && (nParent != nViewer)) {
		if (bGatling && (parentP->info.nType == OBJ_PLAYER) && (gameOpts->UseHiresSound () == 2) && gameOpts->sound.bGatling)
			audio.CreateSegmentSound (weaponInfo.flashSound, weaponP->info.nSegment, 0, weaponP->info.position.vPos, 0, volume, I2X (256),
											  AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
		else
			audio.CreateSegmentSound (weaponInfo.flashSound, weaponP->info.nSegment, 0, weaponP->info.position.vPos, 0, volume);
		}
	else {
		if (nWeaponType == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
			volume = I2X (1) / 2;
		if (bGatling && (gameOpts->UseHiresSound () == 2) && gameOpts->sound.bGatling)
			audio.PlaySound (-1, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER, volume, DEFAULT_PAN, 0, -1,
								  AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
		else
			audio.PlaySound (weaponInfo.flashSound, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER, volume);
		}
	if (gameOpts->sound.bMissiles && CObject::IsMissile (nWeaponType)) {
		int32_t bBigMsl = (nWeaponType == SMARTMSL_ID) ||
								(nWeaponType == MEGAMSL_ID) ||
								(nWeaponType == EARTHSHAKER_ID) ||
								(nWeaponType == ROBOT_SMARTMSL_ID) ||
								(nWeaponType == ROBOT_MEGAMSL_ID) ||
								(nWeaponType == ROBOT_EARTHSHAKER_ID);
		audio.CreateObjectSound (-1, SOUNDCLASS_MISSILE, nWeapon, 1, I2X (gameOpts->sound.xCustomSoundVolume) / 10, I2X (256), -1, -1,
										 AddonSoundName (nMslSounds [bBigMsl]), 1);
		}
	else if (nWeaponType == FLARE_ID)
		audio.CreateObjectSound (nWeapon, SOUNDCLASS_GENERIC, -1, 0, I2X (1), I2X (256), -1, -1, AddonSoundName (SND_ADDON_FLARE));
	}
}

// ---------------------------------------------------------------------------------

// Initializes a laser after Fire is pressed
//	Returns CObject number.
int32_t CreateNewWeapon (CFixVector* vDirection, CFixVector* vPosition, int16_t nSegment, int16_t nParent, uint8_t nWeaponType, int32_t bMakeSound)
{
	CObject		*objP, *parentP = OBJECT (nParent);

if (!parentP)
	return -1;

	int32_t		nObject, nViewer;
	CWeaponInfo	weaponInfo = gameData.weapons.info [nWeaponType];
	fix			xLaserLength = 0;
	CFixVector	vDir = *vDirection;

if (RandShort () > parentP->GunDamage ())
	return -1;

	fix			damage = (I2X (1) / 2 - parentP->AimDamage ()) >> 3;

if (damage > 0) {
	CAngleVector	v;
	CFixMatrix		m;

	v.v.coord.p = fixang (damage / 2 - Rand (damage));
	v.v.coord.b = 0;
	v.v.coord.h = fixang (damage / 2 - Rand (damage));
	m = CFixMatrix::Create (v);
	vDir = m * vDir;
	CFixVector::Normalize (vDir);
	}

//this is a horrible hack.  guided missile stuff should not be
//handled in the middle of a routine that is dealing with the player
if (nWeaponType >= gameData.weapons.nTypes [0])
	nWeaponType = 0;
//	Don't let homing blobs make muzzle flash.
if (parentP->IsRobot ())
	DoMuzzleStuff (nSegment, vPosition);
else if (gameStates.app.bD2XLevel &&
			(parentP == gameData.objData.consoleP) &&
			(SEGMENT (gameData.objData.consoleP->info.nSegment)->HasNoDamageProp ()))
	return -1;
#if 1
if ((nParent == LOCALPLAYER.nObject) && (nWeaponType == PROXMINE_ID) && (gameData.app.GameMode (GM_HOARD | GM_ENTROPY))) {
	nObject = CreatePowerup (POW_HOARD_ORB, -1, nSegment, *vPosition, 0);
	objP = OBJECT (nObject);
	if (objP) {
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		objP->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [objP->info.nId].nClipIndex;
		objP->rType.animationInfo.xFrameTime = gameData.effects.animations [0][objP->rType.animationInfo.nClipIndex].xFrameTime;
		objP->rType.animationInfo.nCurFrame = 0;
		objP->info.nCreator = GetTeam (N_LOCALPLAYER) + 1;
		}
	return -1;
	}
#endif
nObject = CreateWeaponObject (nWeaponType, nSegment, vPosition, -1);
objP = OBJECT (nObject);
if (!objP) {
#if DBG
	if (nObject > -1)
		BRP;
#endif
	return -1;
	}
#if 0
if (parentP == gameData.objData.consoleP) {
	switch (nWeaponType) {
		case LASER_ID:
		case LASER_ID + 1:
		case LASER_ID + 2:
		case LASER_ID + 3:
		case VULCAN_ID:
		case SPREADFIRE_ID:
		case PLASMA_ID:
		case FUSION_ID:
		case SUPERLASER_ID:
		case SUPERLASER_ID + 1:
		case GAUSS_ID:
		case HELIX_ID:
		case PHOENIX_ID:
			gameData.stats.player [0].nShots [0]++;
			gameData.stats.player [1].nShots [0]++;
			break;
		case CONCUSSION_ID:
		case FLASHMSL_ID:
		case GUIDEDMSL_ID:
		case MERCURYMSL_ID:
			gameData.stats.player [0].nShots [1]++;
			gameData.stats.player [1].nShots [1]++;
			break;
		}
	}
#endif
objP->cType.laserInfo.parent.nObject = nParent;
if (parentP) {
	objP->cType.laserInfo.parent.nType = parentP->info.nType;
	objP->cType.laserInfo.parent.nSignature = parentP->info.nSignature;
	if (objP->IsGatlingRound ())
		parentP->SetTracers ((parentP->Tracers () + 1) % 3);
	}
else {
	objP->cType.laserInfo.parent.nType = -1;
	objP->cType.laserInfo.parent.nSignature = -1;
	}
//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
//	not apply to the Omega Cannon.
nViewer = OBJ_IDX (gameData.objData.viewerP);
if (nWeaponType == OMEGA_ID) {
	// Create orientation matrix for tracking purposes.
	int32_t bSpectator = SPECTATOR (parentP);
	objP->info.position.mOrient = CFixMatrix::CreateFU (vDir, bSpectator ? gameStates.app.playerPos.mOrient.m.dir.u : parentP->info.position.mOrient.m.dir.u);
//	objP->info.position.mOrient = CFixMatrix::CreateFU (vDir, bSpectator ? &gameStates.app.playerPos.mOrient.m.v.u : &parentP->info.position.mOrient.m.v.u, NULL);
	if (((nParent != nViewer) || bSpectator) && (parentP->info.nType != OBJ_WEAPON)) {
		// Muzzle flash
		if ((weaponInfo.nFlashAnimation > -1) && ((nWeaponType != OMEGA_ID) || !gameOpts->render.lightning.bOmega || gameStates.render.bOmegaModded))
			CreateMuzzleFlash (objP->info.nSegment, objP->info.position.vPos, weaponInfo.xFlashSize, weaponInfo.nFlashAnimation);
		}
	DoOmegaStuff (OBJECT (nParent), vPosition, objP);
	return nObject;
	}
else if (nWeaponType == FUSION_ID) {
	static int32_t nRotDir = 0;
	nRotDir = !nRotDir;
	objP->mType.physInfo.rotVel.v.coord.x =
	objP->mType.physInfo.rotVel.v.coord.y = 0;
	objP->mType.physInfo.rotVel.v.coord.z = nRotDir ? -I2X (1) : I2X (1);
	}
if (parentP && (parentP->info.nType == OBJ_PLAYER)) {
	if (nWeaponType == FUSION_ID) {
		if (gameData.FusionCharge () <= 0)
			objP->cType.laserInfo.xScale = I2X (1);
		else if (gameData.FusionCharge () <= I2X (4))
			objP->cType.laserInfo.xScale = I2X (1) + gameData.FusionCharge () / 2;
		else
			objP->cType.laserInfo.xScale = I2X (4);
		}
	else if (/* (nWeaponType >= LASER_ID) &&*/ (nWeaponType <= MAX_SUPERLASER_LEVEL) &&
				(PLAYER (parentP->info.nId).flags & PLAYER_FLAGS_QUAD_LASERS))
		objP->cType.laserInfo.xScale = I2X (3) / 4;
	else if (nWeaponType == GUIDEDMSL_ID) {
		if (nParent == LOCALPLAYER.nObject) {
			gameData.objData.SetGuidedMissile (N_LOCALPLAYER, objP);
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordGuidedStart ();
			}
		}
	}

//	Make children of smart bomb bounce so if they hit a CWall right away, they
//	won't detonate.  The frame xInterval code will clear this bit after 1/2 second.
if ((nWeaponType == SMARTMSL_BLOB_ID) ||
	 (nWeaponType == SMARTMINE_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMSL_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMINE_BLOB_ID) ||
	 (nWeaponType == EARTHSHAKER_MEGA_ID))
	objP->mType.physInfo.flags |= PF_BOUNCES;
if (weaponInfo.renderType == WEAPON_RENDER_POLYMODEL)
	xLaserLength = gameData.models.polyModels [0][objP->ModelId ()].Rad () * 2;
if (nWeaponType == FLARE_ID)
	objP->mType.physInfo.flags |= PF_STICK;		//this obj sticks to walls
objP->info.xShield = WI_strength (nWeaponType, gameStates.app.nDifficultyLevel);
// Fill in laser-specific data
objP->SetLife (WI_lifetime (nWeaponType));
//	Assign nParent nType to highest level creator.  This propagates nParent nType down from
//	the original creator through weapons which create children of their own (ie, smart missile)
if (parentP && (parentP->info.nType == OBJ_WEAPON)) {
	int32_t		nRoot = -1, nChild = nParent;
	int32_t		count = 0;
	CObject		*rootP = NULL, *childP = OBJECT (nChild);

	while ((count++ < 10) && (childP->IsWeapon ())) {
		if (!(rootP = OBJECT (nRoot = childP->cType.laserInfo.parent.nObject)))
			break;
		if (rootP->info.nSignature != childP->cType.laserInfo.parent.nSignature) {
			rootP = NULL;
			break;	//	Probably means nParent was killed.  Just continue.
			}
		if (nChild == nRoot) {
			childP->cType.laserInfo.parent.nObject = -1;
			rootP = NULL;
			break;
			}
		nChild = nRoot;
		childP = rootP;
		}
	if (rootP) {
		objP->cType.laserInfo.parent.nObject = nRoot;
		objP->cType.laserInfo.parent.nType = rootP->info.nType;
		objP->cType.laserInfo.parent.nSignature = rootP->info.nSignature;
		}
	}
// Create orientation matrix so we can look from this pov
//	Homing missiles also need an orientation matrix so they know if they can make a turn.
//if ((objP->info.renderType == RT_POLYOBJ) || (WI_homingFlag (objP->info.nId)))
	objP->info.position.mOrient = CFixMatrix::CreateFU (vDir, parentP->info.position.mOrient.m.dir.u);
//	objP->info.position.mOrient = CFixMatrix::CreateFU (vDir, &parentP->info.position.mOrient.m.v.u, NULL);
if (((nParent != nViewer) || SPECTATOR (parentP)) && (parentP->info.nType != OBJ_WEAPON)) {
	// Muzzle flash
	if (weaponInfo.nFlashAnimation > -1)
		CreateMuzzleFlash (objP->info.nSegment, objP->info.position.vPos, weaponInfo.xFlashSize, weaponInfo.nFlashAnimation);
	}
CreateWeaponSound (objP, bMakeSound);
if (!CreateWeaponSpeed (objP))
	return -1;
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID))
	objP->info.xLifeLeft += SRandShort () << 2;		//	add in -2..2 seconds

return nObject;
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls CreateNewWeapon, but takes care of the CSegment and point computation for you.
int32_t CreateNewWeaponSimple (CFixVector* vDirection, CFixVector* vPosition, int16_t parent, uint8_t nWeaponType, int32_t bMakeSound)
{
	CHitResult	hitResult;
	CObject*		parentObjP = OBJECT (parent);
	int32_t			fate;

	//	Find segment containing laser fire position.  If the robot is straddling a segment, the position from
	//	which it fires may be in a different segment, which is bad news for FindHitpoint.  So, cast
	//	a ray from the object center (whose segment we know) to the laser position.  Then, in the call to CreateNewWeapon
	//	use the data returned from this call to FindHitpoint.
	//	Note that while FindHitpoint is pretty slow, it is not terribly slow if the destination point is
	//	in the same CSegment as the source point.

CHitQuery hitQuery (FQ_TRANSWALL | FQ_CHECK_OBJS, &parentObjP->info.position.vPos, vPosition, parentObjP->info.nSegment, OBJ_IDX (parentObjP), 0, 0, ++gameData.physics.bIgnoreObjFlag);
fate = FindHitpoint (hitQuery, hitResult);
if ((fate != HIT_NONE)  || (hitResult.nSegment == -1))
	return -1;

#if DBG
if (!hitResult.nSegment) {
	hitQuery.p0					= &parentObjP->info.position.vPos;
	hitQuery.nSegment			= parentObjP->info.nSegment;
	hitQuery.p1					= vPosition;
	hitQuery.radP0				=
	hitQuery.radP1				= 0;
	hitQuery.nObject			= OBJ_IDX (parentObjP);
	hitQuery.bIgnoreObjFlag	= ++gameData.physics.bIgnoreObjFlag;
	hitQuery.flags				= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	fate = FindHitpoint (hitQuery, hitResult);
	}
#endif

return CreateNewWeapon (vDirection, &hitResult.vPoint, (int16_t) hitResult.nSegment, parent, nWeaponType, bMakeSound);
}

// -----------------------------------------------------------------------------

bool FixWeaponObject (CObject* objP, bool bFixType)
{
if (!IsMultiGame)
	return true;
if (!objP->IsWeapon ()) {
	if (!bFixType) 
		return true;
	objP->SetType (OBJ_WEAPON);
	CreateWeaponSpeed (objP, true);
	}
if (objP->mType.physInfo.flags & PF_STICK)
	return true;
if (objP->Velocity ().IsZero ()) {
	PrintLog (0, "weapon object has invalid velocity\n");
	CreateWeaponSpeed (objP, true);
	if (objP->Velocity ().IsZero ()) {
		objP->Die ();
		return false;
		}
	}
if (objP->LifeLeft () > WI_lifetime (objP->Id ())) {
	PrintLog (0, "weapon object has invalid life time\n");
	objP->Die ();
	return false;
	}

if (objP->info.movementType != MT_PHYSICS) {
	PrintLog (0, "weapon object has invalid movement type %s\n", objP->info.movementType);
	objP->info.movementType = MT_PHYSICS;
	}	
if (objP->info.controlType != CT_WEAPON) {
	PrintLog (0, "weapon object has invalid control type %s\n", objP->info.controlType);
	objP->info.controlType = CT_WEAPON;
	}	
return true;
}

//	-------------------------------------------------------------------------------------------
// eof
