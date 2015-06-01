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
ENTER (0, 0);

	CWeaponInfo	*pWeaponInfo = WEAPONINFO (nWeaponType);
if (!pWeaponInfo) {
	Error ("Invalid weapon render nType in CreateNewWeapon\n");
	RETURN (-1)
	}

switch (pWeaponInfo->renderType) {
	case WEAPON_RENDER_BLOB:
	case WEAPON_RENDER_POLYMODEL:
	case WEAPON_RENDER_LASER:	// Not supported anymore
	case WEAPON_RENDER_NONE:
	case WEAPON_RENDER_VCLIP:
		break;
	default:
		Error ("Invalid weapon render nType in CreateNewWeapon\n");
		RETURN (-1)
	}

int32_t nObject = CreateWeapon (nWeaponType, nParent, nSegment, *vPosition, 0, 255);
CObject *pObj = OBJECT (nObject);
if (!pObj) {
#if DBG
	if (nObject > -1)
		BRP;
#endif
	RETURN (-1)
	}
#if DBG
if (nObject == nDbgObj)
	BRP;
#endif
if (pWeaponInfo->renderType == WEAPON_RENDER_POLYMODEL) {
	pObj->rType.polyObjInfo.nModel = pWeaponInfo->nModel;
	pObj->AdjustSize (1, pWeaponInfo->poLenToWidthRatio);
	}
else if (EGI_FLAG (bTracers, 0, 1, 0) && pObj->IsGatlingRound ()) {
	pObj->rType.polyObjInfo.nModel = WEAPONINFO (SUPERLASER_ID + 1)->nModel;
	pObj->rType.polyObjInfo.nTexOverride = -1;
	pObj->rType.polyObjInfo.nAltTextures = 0;
	pObj->AdjustSize (1, WEAPONINFO (SUPERLASER_ID + 1)->poLenToWidthRatio);
	pObj->info.renderType = RT_POLYOBJ;
	}
pObj->mType.physInfo.mass = pWeaponInfo->mass;
pObj->mType.physInfo.drag = pWeaponInfo->drag;
pObj->mType.physInfo.thrust.SetZero ();
if (pWeaponInfo->bounce == 1)
	pObj->mType.physInfo.flags |= PF_BOUNCES;
if ((pWeaponInfo->bounce == 2) || gameStates.app.cheats.bBouncingWeapons)
	pObj->mType.physInfo.flags |= PF_BOUNCES + PF_BOUNCES_TWICE;
RETURN (nObject)
}

// ---------------------------------------------------------------------------------

int32_t CreateWeaponSpeed (CObject* pWeapon, bool bFix)
{
ENTER (0, 0);
	uint8_t			nWeaponType = pWeapon->Id ();
	CWeaponInfo	*pWeaponInfo = WEAPONINFO (nWeaponType);

if (!pWeaponInfo) {
	Error ("Invalid weapon render nType in CreateWeaponSpeed\n");
	RETURN (-1)
	}

	CObject			*pParent = OBJECT (pWeapon->cType.laserInfo.parent.nObject);
	fix				xParentSpeed;
	fix				xLaserLength = (pWeaponInfo->renderType == WEAPON_RENDER_POLYMODEL) ? gameData.modelData.polyModels [0][pWeapon->ModelId ()].Rad () * 2 : 0;
	CFixVector		vDir = pWeapon->Heading ();

//	Here's where to fix the problem with OBJECTS which are moving backwards imparting higher velocity to their weaponfire.
//	Find out if moving backwards.
if (!pParent || !CObject::IsMine (nWeaponType))
	xParentSpeed = 0;
else {
	xParentSpeed = pParent->mType.physInfo.velocity.Mag ();
	if (CFixVector::Dot (pParent->mType.physInfo.velocity, pParent->info.position.mOrient.m.dir.f) < 0)
		xParentSpeed = -xParentSpeed;
	}

//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
// This also jitters the laser a bit so that it doesn't alias.
//	Don't do for weapons created by weapons.
if (!bFix && pParent && (pParent->info.nType == OBJ_PLAYER) && (pWeaponInfo->renderType != WEAPON_RENDER_NONE) && (nWeaponType != FLARE_ID) && !CObject::IsMissile (nWeaponType)) {
#if 1
	pWeapon->mType.physInfo.velocity = vDir * (gameData.laserData.nOffset + xLaserLength / 2);
	if (pWeapon->Velocity ().IsZero ()) 
		PrintLog (0, "weapon object has no velocity\n");
	else 
#endif
		{
		fix xTime = gameData.physicsData.xTime;
		gameData.physicsData.xTime = -1;	// make it move the entire velocity vector
		pWeapon->Update ();
		gameData.physicsData.xTime = xTime;
		if (pWeapon->info.nFlags & OF_SHOULD_BE_DEAD) {
			ReleaseObject (pWeapon->Index ());
			RETURN (-1)
			}
		}
	}

fix xWeaponSpeed = WI_speed (pWeapon->info.nId, gameStates.app.nDifficultyLevel);
if (pWeaponInfo->speedvar != 128) {
	fix randval = I2X (1) - ((RandShort () * pWeaponInfo->speedvar) >> 6);	//	Get a scale factor between speedvar% and 1.0.
	xWeaponSpeed = FixMul (xWeaponSpeed, randval);
	}
//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
if ((nWeaponType == SMARTMSL_BLOB_ID) ||
	 (nWeaponType == SMARTMINE_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMSL_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMINE_BLOB_ID) ||
	 (nWeaponType == EARTHSHAKER_MEGA_ID))
	xWeaponSpeed /= 4;
if (WI_thrust (nWeaponType) != 0)
	xWeaponSpeed /= 2;
/*test*/pWeapon->mType.physInfo.velocity = vDir * (xWeaponSpeed + xParentSpeed);
if (pParent)
	pWeapon->SetStartVel (&pParent->mType.physInfo.velocity);
//	Set thrust
if (WI_thrust (nWeaponType) != 0) {
	pWeapon->mType.physInfo.thrust = pWeapon->mType.physInfo.velocity;
	pWeapon->mType.physInfo.thrust *= FixDiv (WI_thrust (pWeapon->info.nId), xWeaponSpeed + xParentSpeed);
	}
RETURN (pWeapon->Index ())
}

// ---------------------------------------------------------------------------------

void CreateWeaponSound (CObject* pWeapon, int32_t bMakeSound)
{
ENTER (0, 0);
if (!bMakeSound)
	LEAVE;

	uint8_t			nWeaponType = pWeapon->Id ();
	CWeaponInfo		*pWeaponInfo = WEAPONINFO (nWeaponType);

if (!pWeaponInfo || (pWeaponInfo->flashSound < 0))
	LEAVE;

	fix				volume = I2X (1);
	int32_t			nParent = pWeapon->cType.laserInfo.parent.nObject;
	CObject			*pParent = OBJECT (nParent);
	int32_t			nWeapon = pWeapon->Index ();
	int32_t			nViewer = OBJ_IDX (gameData.objData.pViewer);

	static int32_t	nMslSounds [2] = {SND_ADDON_MISSILE_SMALL, SND_ADDON_MISSILE_BIG};
	static int32_t	nGatlingSounds [2] = {SND_ADDON_VULCAN, SND_ADDON_GAUSS};

int32_t bGatling = pWeapon->IsGatlingRound ();
if (pParent && (nParent != nViewer)) {
	if (bGatling && (pParent->info.nType == OBJ_PLAYER) && (gameOpts->UseHiresSound () == 2) && gameOpts->sound.bGatling)
		audio.CreateSegmentSound (pWeaponInfo->flashSound, pWeapon->info.nSegment, 0, pWeapon->info.position.vPos, 0, volume, I2X (256),
											AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
	else
		audio.CreateSegmentSound (pWeaponInfo->flashSound, pWeapon->info.nSegment, 0, pWeapon->info.position.vPos, 0, volume);
	}
else {
	if (nWeaponType == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
		volume = I2X (1) / 2;
	if (bGatling && (gameOpts->UseHiresSound () == 2) && gameOpts->sound.bGatling)
		audio.PlaySound (-1, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER, volume, DEFAULT_PAN, 0, -1,
								AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
	else
		audio.PlaySound (pWeaponInfo->flashSound, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER, volume);
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
LEAVE
}

// ---------------------------------------------------------------------------------

// Initializes a laser after Fire is pressed
//	Returns CObject number.
int32_t CreateNewWeapon (CFixVector* vDirection, CFixVector* vPosition, int16_t nSegment, int16_t nParent, uint8_t nWeaponType, int32_t bMakeSound)
{
ENTER (0, 0);
CObject *pParent = OBJECT (nParent);
if (!pParent)
	RETURN (-1)

CWeaponInfo	*pWeaponInfo = WEAPONINFO (nWeaponType);
if (!pWeaponInfo)
	RETURN (-1)

CFixVector	vDir = *vDirection;
if (RandShort () > pParent->GunDamage ())
	RETURN (-1)

fix damage = (I2X (1) / 2 - pParent->AimDamage ()) >> 3;

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
if (nWeaponType >= gameData.weaponData.nTypes [0])
	nWeaponType = 0;
//	Don't let homing blobs make muzzle flash.
if (pParent->IsRobot ())
	DoMuzzleStuff (nSegment, vPosition);
else if (gameStates.app.bD2XLevel &&
			(pParent == gameData.objData.pConsole) &&
			(SEGMENT (gameData.objData.pConsole->info.nSegment)->HasNoDamageProp ()))
	RETURN (-1)
#if 1
if ((nParent == LOCALPLAYER.nObject) && (nWeaponType == PROXMINE_ID) && (gameData.appData.GameMode (GM_HOARD | GM_ENTROPY))) {
	int32_t nObject = CreatePowerup (POW_HOARD_ORB, -1, nSegment, *vPosition, 0);
	CObject *pObj = OBJECT (nObject);
	if (pObj) {
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nCount++] = nObject;
		pObj->rType.animationInfo.nClipIndex = gameData.objData.pwrUp.info [pObj->info.nId].nClipIndex;
		pObj->rType.animationInfo.xFrameTime = gameData.effectData.animations [0][pObj->rType.animationInfo.nClipIndex].xFrameTime;
		pObj->rType.animationInfo.nCurFrame = 0;
		pObj->info.nCreator = GetTeam (N_LOCALPLAYER) + 1;
		}
	RETURN (-1)
	}
#endif
int32_t nObject = CreateWeaponObject (nWeaponType, nSegment, vPosition, -1);
CObject *pObj = OBJECT (nObject);
if (!pObj) {
#if DBG
	if (nObject > -1)
		BRP;
#endif
	RETURN (-1)
	}
#if 0
if (pParent == gameData.objData.pConsole) {
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
			gameData.statsData.player [0].nShots [0]++;
			gameData.statsData.player [1].nShots [0]++;
			break;
		case CONCUSSION_ID:
		case FLASHMSL_ID:
		case GUIDEDMSL_ID:
		case MERCURYMSL_ID:
			gameData.statsData.player [0].nShots [1]++;
			gameData.statsData.player [1].nShots [1]++;
			break;
		}
	}
#endif
pObj->cType.laserInfo.parent.nObject = nParent;
if (pParent) {
	pObj->cType.laserInfo.parent.nType = pParent->info.nType;
	pObj->cType.laserInfo.parent.nSignature = pParent->info.nSignature;
	if (pObj->IsGatlingRound ())
		pParent->SetTracers ((pParent->Tracers () + 1) % 3);
	}
else {
	pObj->cType.laserInfo.parent.nType = -1;
	pObj->cType.laserInfo.parent.nSignature = -1;
	}
//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
//	not apply to the Omega Cannon.
int32_t nViewer = OBJ_IDX (gameData.objData.pViewer);
if (nWeaponType == OMEGA_ID) {
	// Create orientation matrix for tracking purposes.
	int32_t bSpectator = SPECTATOR (pParent);
	pObj->info.position.mOrient = CFixMatrix::CreateFU (vDir, bSpectator ? gameStates.app.playerPos.mOrient.m.dir.u : pParent->info.position.mOrient.m.dir.u);
//	pObj->info.position.mOrient = CFixMatrix::CreateFU (vDir, bSpectator ? &gameStates.app.playerPos.mOrient.m.v.u : &pParent->info.position.mOrient.m.v.u, NULL);
	if (((nParent != nViewer) || bSpectator) && (pParent->info.nType != OBJ_WEAPON)) {
		// Muzzle flash
		if ((pWeaponInfo->nFlashAnimation > -1) && ((nWeaponType != OMEGA_ID) || !gameOpts->render.lightning.bOmega || gameStates.render.bOmegaModded))
			CreateMuzzleFlash (pObj->info.nSegment, pObj->info.position.vPos, pWeaponInfo->xFlashSize, pWeaponInfo->nFlashAnimation);
		}
	DoOmegaStuff (OBJECT (nParent), vPosition, pObj);
	RETURN (nObject)
	}
else if (nWeaponType == FUSION_ID) {
	static int32_t nRotDir = 0;
	nRotDir = !nRotDir;
	pObj->mType.physInfo.rotVel.v.coord.x =
	pObj->mType.physInfo.rotVel.v.coord.y = 0;
	pObj->mType.physInfo.rotVel.v.coord.z = nRotDir ? -I2X (1) : I2X (1);
	}
if (pParent && (pParent->info.nType == OBJ_PLAYER)) {
	if (nWeaponType == FUSION_ID) {
		if (gameData.FusionCharge () <= 0)
			pObj->cType.laserInfo.xScale = I2X (1);
		else if (gameData.FusionCharge () <= I2X (4))
			pObj->cType.laserInfo.xScale = I2X (1) + gameData.FusionCharge () / 2;
		else
			pObj->cType.laserInfo.xScale = I2X (4);
		}
	else if (/* (nWeaponType >= LASER_ID) &&*/ (nWeaponType <= MAX_SUPERLASER_LEVEL) &&
				(PLAYER (pParent->info.nId).flags & PLAYER_FLAGS_QUAD_LASERS))
		pObj->cType.laserInfo.xScale = I2X (3) / 4;
	else if (nWeaponType == GUIDEDMSL_ID) {
		if (nParent == LOCALPLAYER.nObject) {
			gameData.objData.SetGuidedMissile (N_LOCALPLAYER, pObj);
			if (gameData.demoData.nState == ND_STATE_RECORDING)
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
	pObj->mType.physInfo.flags |= PF_BOUNCES;
if (nWeaponType == FLARE_ID)
	pObj->mType.physInfo.flags |= PF_STICK;		//this obj sticks to walls
pObj->info.xShield = WI_strength (nWeaponType, gameStates.app.nDifficultyLevel);
// Fill in laser-specific data
pObj->SetLife (WI_lifetime (nWeaponType));
//	Assign nParent nType to highest level creator.  This propagates nParent nType down from
//	the original creator through weapons which create children of their own (ie, smart missile)
if (pParent && (pParent->Type () == OBJ_WEAPON)) {
	int32_t		nRoot = -1, nChild = nParent;
	int32_t		count = 0;
	CObject		*pRoot = NULL, *pChild = OBJECT (nChild);

	while ((count++ < 10) && (pChild->Type () == OBJ_WEAPON)) {
		if (!(pRoot = OBJECT (nRoot = pChild->cType.laserInfo.parent.nObject)))
			break;
		if (pRoot->info.nSignature != pChild->cType.laserInfo.parent.nSignature) {
			pRoot = NULL;
			break;	//	Probably means nParent was killed.  Just continue.
			}
		if (nChild == nRoot) {
			pChild->cType.laserInfo.parent.nObject = -1;
			pRoot = NULL;
			break;
			}
		nChild = nRoot;
		pChild = pRoot;
		}
	if (pRoot) {
		pObj->cType.laserInfo.parent.nObject = nRoot;
		pObj->cType.laserInfo.parent.nType = pRoot->info.nType;
		pObj->cType.laserInfo.parent.nSignature = pRoot->info.nSignature;
		}
	}
// Create orientation matrix so we can look from this pov
//	Homing missiles also need an orientation matrix so they know if they can make a turn.
//if ((pObj->info.renderType == RT_POLYOBJ) || (WI_homingFlag (pObj->info.nId)))
	pObj->info.position.mOrient = CFixMatrix::CreateFU (vDir, pParent->info.position.mOrient.m.dir.u);
//	pObj->info.position.mOrient = CFixMatrix::CreateFU (vDir, &pParent->info.position.mOrient.m.v.u, NULL);
if (((nParent != nViewer) || SPECTATOR (pParent)) && (pParent->info.nType != OBJ_WEAPON)) {
	// Muzzle flash
	if (pWeaponInfo->nFlashAnimation > -1)
		CreateMuzzleFlash (pObj->info.nSegment, pObj->info.position.vPos, pWeaponInfo->xFlashSize, pWeaponInfo->nFlashAnimation);
	}
CreateWeaponSound (pObj, bMakeSound);
if (!CreateWeaponSpeed (pObj))
	RETURN (-1)
if ((pObj->info.nType == OBJ_WEAPON) && (pObj->info.nId == FLARE_ID))
	pObj->info.xLifeLeft += SRandShort () << 2;		//	add in -2..2 seconds

RETURN (nObject)
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls CreateNewWeapon, but takes care of the CSegment and point computation for you.
int32_t CreateNewWeaponSimple (CFixVector* vDirection, CFixVector* vPosition, int16_t parent, uint8_t nWeaponType, int32_t bMakeSound)
{
ENTER (0, 0);
	CHitResult	hitResult;
	CObject*		pParentObj = OBJECT (parent);

	//	Find segment containing laser fire position.  If the robot is straddling a segment, the position from
	//	which it fires may be in a different segment, which is bad news for FindHitpoint.  So, cast
	//	a ray from the object center (whose segment we know) to the laser position.  Then, in the call to CreateNewWeapon
	//	use the data returned from this call to FindHitpoint.
	//	Note that while FindHitpoint is pretty slow, it is not terribly slow if the destination point is
	//	in the same CSegment as the source point.

CHitQuery hitQuery (FQ_TRANSWALL | FQ_CHECK_OBJS, &pParentObj->info.position.vPos, vPosition, pParentObj->info.nSegment, OBJ_IDX (pParentObj), 0, 0, ++gameData.physicsData.bIgnoreObjFlag);
int32_t fate = FindHitpoint (hitQuery, hitResult);
if ((fate != HIT_NONE)  || (hitResult.nSegment == -1))
	RETURN (-1)

#if DBG
if (!hitResult.nSegment) {
	hitQuery.p0					= &pParentObj->info.position.vPos;
	hitQuery.nSegment			= pParentObj->info.nSegment;
	hitQuery.p1					= vPosition;
	hitQuery.radP0				=
	hitQuery.radP1				= 0;
	hitQuery.nObject			= OBJ_IDX (pParentObj);
	hitQuery.bIgnoreObjFlag	= ++gameData.physicsData.bIgnoreObjFlag;
	hitQuery.flags				= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	fate = FindHitpoint (hitQuery, hitResult);
	}
#endif

RETURN (CreateNewWeapon (vDirection, &hitResult.vPoint, (int16_t) hitResult.nSegment, parent, nWeaponType, bMakeSound))
}

// -----------------------------------------------------------------------------

bool FixWeaponObject (CObject* pObj, bool bFixType)
{
ENTER (0, 0);
if (!IsMultiGame)
	RETURN (true)
if (!pObj->IsWeapon ()) {
	if (!bFixType) 
		RETURN (true)
	pObj->SetType (OBJ_WEAPON);
	CreateWeaponSpeed (pObj, true);
	}
if (pObj->mType.physInfo.flags & PF_STICK)
	RETURN (true)
if (pObj->Velocity ().IsZero ()) {
	PrintLog (0, "weapon object has invalid velocity\n");
	CreateWeaponSpeed (pObj, true);
	if (pObj->Velocity ().IsZero ()) {
		pObj->Die ();
		RETURN (false)
		}
	}
if (pObj->LifeLeft () > WI_lifetime (pObj->Id ())) {
	PrintLog (0, "weapon object has invalid life time\n");
	pObj->Die ();
	RETURN (false)
	}

if (pObj->info.movementType != MT_PHYSICS) {
	PrintLog (0, "weapon object has invalid movement type %s\n", pObj->info.movementType);
	pObj->info.movementType = MT_PHYSICS;
	}	
if (pObj->info.controlType != CT_WEAPON) {
	PrintLog (0, "weapon object has invalid control type %s\n", pObj->info.controlType);
	pObj->info.controlType = CT_WEAPON;
	}	
RETURN (true)
}

//	-------------------------------------------------------------------------------------------
// eof
