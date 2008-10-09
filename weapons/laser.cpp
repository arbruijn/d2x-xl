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

#include "inferno.h"
#include "objrender.h"
#include "lightning.h"
#include "trackobject.h"
#include "omega.h"
#include "segpoint.h"
#include "error.h"
#include "key.h"
#include "texmap.h"
#include "textures.h"
#include "render.h"
#include "fireball.h"
#include "newdemo.h"
#include "timer.h"
#include "physics.h"
#include "gameseg.h"
#include "input.h"
#include "dropobject.h"

#ifdef TACTILE
#include "tactile.h"
#endif

void BlastNearbyGlass (tObject *objP, fix damage);
void NDRecordGuidedEnd (void);
void NDRecordGuidedStart (void);

//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!!*****
//	Here are the constants you're looking for!--MK

#define	FULL_COCKPIT_OFFS 0
#define	LASER_OFFS	 ((F1_0 * 29) / 100)

static   int nMslTurnSpeeds [3] = {4, 2, 1};

#define	HOMINGMSL_SCALE		nMslTurnSpeeds [(IsMultiGame && !IsCoopGame) ? 2 : extraGameInfo [IsMultiGame].nMslTurnSpeed]

#define	MAX_SMART_DISTANCE	 (F1_0*150)
#define	MAX_OBJDISTS			30

//---------------------------------------------------------------------------------

inline int HomingMslScale (void)
{
return (int) (HOMINGMSL_SCALE * sqrt (gameStates.gameplay.slowmo [0].fSpeed));
}

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// tPlayer and calls the appropriate routine.

void RenderLaser (tObject *objP)
{

//	Commented out by John (sort of, typed by Mike) on 6/8/94
#if 0
	switch (objP->id)	{
	case WEAPON_TYPE_WEAK_LASER:
	case WEAPON_TYPE_STRONG_LASER:
	case WEAPON_TYPE_CANNON_BALL:
	case WEAPON_TYPEMSL:
		break;
	default:
		Error ("Invalid weapon nType in RenderLaser\n");
	}
#endif

switch (gameData.weapons.info [objP->id].renderType)	{
	case WEAPON_RENDER_LASER:
		Int3 ();	// Not supported anymore!
					//Laser_draw_one (OBJ_IDX (objP), gameData.weapons.info [objP->id].bitmap);
		break;
	case WEAPON_RENDER_BLOB:
		DrawObjectBlob (objP, gameData.weapons.info [objP->id].bitmap.index, gameData.weapons.info [objP->id].bitmap.index, 0, NULL, 2.0f / 3.0f);
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

int LaserCreationTimeout (int nId, fix xCreationTime)
{
if (nId == PHOENIX_ID)
	return gameData.time.xGame > xCreationTime + (F1_0 / 3) * gameStates.gameplay.slowmo [0].fSpeed;
else if (nId == PHOENIX_ID)
	return gameData.time.xGame > xCreationTime + (F1_0 / 2) * gameStates.gameplay.slowmo [0].fSpeed;
else if (WeaponIsPlayerMine (nId))
	return gameData.time.xGame > xCreationTime + (F1_0 * 4) * gameStates.gameplay.slowmo [0].fSpeed;
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
	tObject	*objP1, *objP2;
	short		id1, id2;
	fix		ct1, ct2;

if ((o1 < 0) || (o2 < 0))
	return 0;
objP1 = OBJECTS + o1;
objP2 = OBJECTS + o2;
id1 = objP1->id;
ct1 = objP1->cType.laserInfo.creationTime;
// See if o2 is the parent of o1
if (objP1->nType == OBJ_WEAPON)
	if ((objP1->cType.laserInfo.nParentObj == o2) &&
		 (objP1->cType.laserInfo.nParentSig == objP2->nSignature)) {
		//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is tPlayer, they are related only if o1 < 2.0 seconds old
		if (LaserCreationTimeout (id1, ct1))
			return 0;
		return 1;
		}

id2 = objP2->id;
ct2 = objP2->cType.laserInfo.creationTime;
// See if o1 is the parent of o2
if (objP2->nType == OBJ_WEAPON)
	if ((objP2->cType.laserInfo.nParentObj == o1) &&
		 (objP2->cType.laserInfo.nParentSig == objP1->nSignature)) {
		//	o2 is a weapon, o1 is the parent of 2, so if o2 is PROXIMITY_BOMB and o1 is tPlayer, they are related only if o1 < 2.0 seconds old
		if (LaserCreationTimeout (id2, ct2))
			return 0;
		return 1;
		}

// They must both be weapons
if ((objP1->nType != OBJ_WEAPON) || (objP2->nType != OBJ_WEAPON))
	return 0;

//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
// See if they're siblings...
//	MK: 06/08/95, Don't allow prox bombs to detonate for 3/4 second.  Else too likely to get toasted by your own bomb if hit by opponent.
if (objP1->cType.laserInfo.nParentSig == objP2->cType.laserInfo.nParentSig) {
	if ((id1 != PROXMINE_ID)  && (id2 != PROXMINE_ID) && (id1 != SMARTMINE_ID) && (id2 != SMARTMINE_ID))
		return 1;
	//	If neither is older than 1/2 second, then can't blow up!
	if ((gameData.time.xGame > (ct1 + F1_0/2) * gameStates.gameplay.slowmo [0].fSpeed) ||
		 (gameData.time.xGame > (ct2 + F1_0/2) * gameStates.gameplay.slowmo [0].fSpeed))
		return 0;
	return 1;
	}

//	Anything can cause a collision with a robot super prox mine.
if (WeaponIsMine (id1) || WeaponIsMine (id2))
	return 0;
if (!COMPETITION && EGI_FLAG (bKillMissiles, 0, 0, 0) && (gameData.objs.bIsMissile [id1] || gameData.objs.bIsMissile [id2]))
	return 0;
return 1;
}

//---------------------------------------------------------------------------------
//--unused-- int Muzzle_scale=2;
void DoMuzzleStuff (int nSegment, vmsVector *pos)
{
gameData.muzzle.info [gameData.muzzle.queueIndex].createTime = TimerGetFixedSeconds ();
gameData.muzzle.info [gameData.muzzle.queueIndex].nSegment = nSegment;
gameData.muzzle.info [gameData.muzzle.queueIndex].pos = *pos;
gameData.muzzle.queueIndex++;
if (gameData.muzzle.queueIndex >= MUZZLE_QUEUE_MAX)
	gameData.muzzle.queueIndex = 0;
}

//---------------------------------------------------------------------------------
//creates a weapon tObject
int CreateWeaponObject (ubyte nWeaponType, short nSegment, vmsVector *vPosition, short nParent)
{
	int		rType = -1;
	fix		xLaserRadius = -1;
	int		nObject;
	tObject	*objP;

switch (gameData.weapons.info [nWeaponType].renderType)	{
	case WEAPON_RENDER_BLOB:
		rType = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
		xLaserRadius = gameData.weapons.info [nWeaponType].blob_size;
		break;
	case WEAPON_RENDER_POLYMODEL:
		xLaserRadius = 0;	//	Filled in below.
		rType = RT_POLYOBJ;
		break;
	case WEAPON_RENDER_LASER:
		Int3 (); 	// Not supported anymore
		break;
	case WEAPON_RENDER_NONE:
		rType = RT_NONE;
		xLaserRadius = F1_0;
		break;
	case WEAPON_RENDER_VCLIP:
		rType = RT_WEAPON_VCLIP;
		xLaserRadius = gameData.weapons.info [nWeaponType].blob_size;
		break;
	default:
		Error ("Invalid weapon render nType in CreateNewLaser\n");
		return -1;
	}

Assert (xLaserRadius != -1);
Assert (rType != -1);
nObject = tObject::Create((ubyte) OBJ_WEAPON, nWeaponType, nParent, nSegment, *vPosition, vmsMatrix::IDENTITY, xLaserRadius, (ubyte) CT_WEAPON, (ubyte) MT_PHYSICS, (ubyte) rType, 1);
objP = OBJECTS + nObject;
if (gameData.weapons.info [nWeaponType].renderType == WEAPON_RENDER_POLYMODEL) {
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->id].nModel;
	objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad,
								gameData.weapons.info [objP->id].po_len_to_width_ratio);
	}
else if (EGI_FLAG (bTracers, 0, 1, 0) && (objP->id == VULCAN_ID) || (objP->id == GAUSS_ID)) {
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [SUPERLASER_ID + 1].nModel;
	objP->rType.polyObjInfo.nTexOverride = -1;
	objP->rType.polyObjInfo.nAltTextures = 0;
	objP->size = FixDiv (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad,
								gameData.weapons.info [SUPERLASER_ID].po_len_to_width_ratio);
	objP->renderType = RT_POLYOBJ;
	}
objP->mType.physInfo.mass = WI_mass (nWeaponType);
objP->mType.physInfo.drag = WI_drag (nWeaponType);
objP->mType.physInfo.thrust.SetZero();
if (gameData.weapons.info [nWeaponType].bounce == 1)
	objP->mType.physInfo.flags |= PF_BOUNCE;
if ((gameData.weapons.info [nWeaponType].bounce == 2) || gameStates.app.cheats.bBouncingWeapons)
	objP->mType.physInfo.flags |= PF_BOUNCE + PF_BOUNCES_TWICE;
return nObject;
}

// ---------------------------------------------------------------------------------

// Initializes a laser after Fire is pressed
//	Returns tObject number.
int CreateNewLaser (vmsVector *vDirection, vmsVector *vPosition, short nSegment,
						  short nParent, ubyte nWeaponType, int bMakeSound)
{
	int			nObject, nViewer, bBigMsl;
	tObject		*objP, *parentP = (nParent < 0) ? NULL : OBJECTS + nParent;
	tWeaponInfo	*weaponInfoP;
	fix			xParentSpeed, xWeaponSpeed;
	fix			volume;
	fix			xLaserLength = 0;

	static int	nMslSounds [2] = {SND_ADDON_MISSILE_SMALL, SND_ADDON_MISSILE_BIG};
	static int	nGatlingSounds [2] = {SND_ADDON_VULCAN, SND_ADDON_GAUSS};

Assert (nWeaponType < gameData.weapons.nTypes [0]);
if (nWeaponType >= gameData.weapons.nTypes [0])
	nWeaponType = 0;
//	Don't let homing blobs make muzzle flash.
if (parentP->nType == OBJ_ROBOT)
	DoMuzzleStuff (nSegment, vPosition);
else if (gameStates.app.bD2XLevel &&
			(parentP == gameData.objs.console) &&
			(gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
	return -1;
#if 1
if ((nParent == LOCALPLAYER.nObject) &&
	 (nWeaponType == PROXMINE_ID) &&
	 (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
	nObject = tObject::Create(OBJ_POWERUP, POW_HOARD_ORB, -1, nSegment, *vPosition, vmsMatrix::IDENTITY,
									gameData.objs.pwrUp.info [POW_HOARD_ORB].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
	if (nObject >= 0) {
		objP = OBJECTS + nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->matCenCreator = GetTeam (gameData.multiplayer.nLocalPlayer) + 1;
		}
	return -1;
	}
#endif
nObject = CreateWeaponObject (nWeaponType, nSegment, vPosition, nParent);
if (nObject < 0)
	return -1;
#if 0
if (parentP == gameData.objs.console) {
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
if ((nWeaponType == VULCAN_ID) || (nWeaponType == GAUSS_ID))
	gameData.objs.nTracers [nParent] = (gameData.objs.nTracers [nParent] + 1) % 3;
objP = OBJECTS + nObject;
objP->cType.laserInfo.nParentObj = nParent;
objP->cType.laserInfo.parentType = parentP->nType;
objP->cType.laserInfo.nParentSig = parentP->nSignature;
//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
//	not apply to the Omega Cannon.
nViewer = OBJ_IDX (gameData.objs.viewer);
weaponInfoP = gameData.weapons.info + nWeaponType;
if (nWeaponType == OMEGA_ID) {
	// Create orientation matrix for tracking purposes.
	int bSpectator = SPECTATOR (parentP);
	objP->position.mOrient = vmsMatrix::CreateFU(*vDirection, bSpectator ? gameStates.app.playerPos.mOrient[UVEC] : parentP->position.mOrient[UVEC]);
//	objP->position.mOrient = vmsMatrix::CreateFU(*vDirection, bSpectator ? &gameStates.app.playerPos.mOrient[UVEC] : &parentP->position.mOrient[UVEC], NULL);
	if (((nParent != nViewer) || bSpectator) && (parentP->nType != OBJ_WEAPON)) {
		// Muzzle flash
		if (weaponInfoP->nFlashVClip > -1)
			ObjectCreateMuzzleFlash (objP->nSegment, &objP->position.vPos, weaponInfoP->xFlashSize, weaponInfoP->nFlashVClip);
		}
	DoOmegaStuff (OBJECTS + nParent, vPosition, objP);
	return nObject;
	}
else if (nWeaponType == FUSION_ID) {
	static int nRotDir = 0;
	nRotDir = !nRotDir;
	objP->mType.physInfo.rotVel[X] =
	objP->mType.physInfo.rotVel[Y] = 0;
	objP->mType.physInfo.rotVel[Z] = nRotDir ? -F1_0 : F1_0;
	}
if (parentP->nType == OBJ_PLAYER) {
	if (nWeaponType == FUSION_ID) {
		if (gameData.fusion.xCharge <= 0)
			objP->cType.laserInfo.multiplier = F1_0;
		else if (gameData.fusion.xCharge <= 4 * F1_0)
			objP->cType.laserInfo.multiplier = F1_0 + gameData.fusion.xCharge / 2;
		else
			objP->cType.laserInfo.multiplier = 4 * F1_0;
		}
	else if (/* (nWeaponType >= LASER_ID) &&*/ (nWeaponType <= MAX_SUPER_LASER_LEVEL) &&
				(gameData.multiplayer.players [parentP->id].flags & PLAYER_FLAGS_QUAD_LASERS))
		objP->cType.laserInfo.multiplier = 3 * F1_0 / 4;
	else if (nWeaponType == GUIDEDMSL_ID) {
		if (nParent == LOCALPLAYER.nObject) {
			gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP = objP;
			gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature = objP->nSignature;
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordGuidedStart ();
			}
		}
	}

//	Make children of smart bomb bounce so if they hit a tWall right away, they
//	won't detonate.  The frame xInterval code will clear this bit after 1/2 second.
if ((nWeaponType == SMARTMSL_BLOB_ID) ||
	 (nWeaponType == SMARTMINE_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMSL_BLOB_ID) ||
	 (nWeaponType == ROBOT_SMARTMINE_BLOB_ID) ||
	 (nWeaponType == EARTHSHAKER_MEGA_ID))
	objP->mType.physInfo.flags |= PF_BOUNCE;
if (weaponInfoP->renderType == WEAPON_RENDER_POLYMODEL)
	xLaserLength = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad * 2;
if (nWeaponType == FLARE_ID)
	objP->mType.physInfo.flags |= PF_STICK;		//this obj sticks to walls
objP->shields = WI_strength (nWeaponType, gameStates.app.nDifficultyLevel);
// Fill in laser-specific data
objP->lifeleft							= WI_lifetime (nWeaponType);
objP->cType.laserInfo.parentType	= parentP->nType;
objP->cType.laserInfo.nParentSig = parentP->nSignature;
objP->cType.laserInfo.nParentObj	= nParent;
//	Assign nParent nType to highest level creator.  This propagates nParent nType down from
//	the original creator through weapons which create children of their own (ie, smart missile)
if (parentP->nType == OBJ_WEAPON) {
	int	nHighestParent = nParent;
	int	count = 0;
	tObject	*parentObjP = OBJECTS + nHighestParent;

	while ((count++ < 10) && (parentObjP->nType == OBJ_WEAPON)) {
		int nNextParent = parentObjP->cType.laserInfo.nParentObj;
		if (OBJECTS [nNextParent].nSignature != parentObjP->cType.laserInfo.nParentSig)
			break;	//	Probably means nParent was killed.  Just continue.
		if (nNextParent == nHighestParent) {
			Int3 ();	//	Hmm, tObject is nParent of itself.  This would seem to be bad, no?
			break;
			}
		nHighestParent = nNextParent;
		parentObjP = OBJECTS + nHighestParent;
		objP->cType.laserInfo.nParentObj	= nHighestParent;
		objP->cType.laserInfo.parentType	= parentObjP->nType;
		objP->cType.laserInfo.nParentSig = parentObjP->nSignature;
		}
	}
// Create orientation matrix so we can look from this pov
//	Homing missiles also need an orientation matrix so they know if they can make a turn.
//if ((objP->renderType == RT_POLYOBJ) || (WI_homingFlag (objP->id)))
	objP->position.mOrient = vmsMatrix::CreateFU(*vDirection, parentP->position.mOrient[UVEC]);
//	objP->position.mOrient = vmsMatrix::CreateFU(*vDirection, &parentP->position.mOrient[UVEC], NULL);
if (((nParent != nViewer) || SPECTATOR (parentP)) && (parentP->nType != OBJ_WEAPON)) {
	// Muzzle flash
	if (weaponInfoP->nFlashVClip > -1)
		ObjectCreateMuzzleFlash (objP->nSegment, &objP->position.vPos, weaponInfoP->xFlashSize,
										 weaponInfoP->nFlashVClip);
	}
volume = F1_0;
if (bMakeSound && (weaponInfoP->flashSound > -1)) {
	int bGatling = (nWeaponType == VULCAN_ID) || (nWeaponType == GAUSS_ID);
	if (nParent != nViewer) {
		if (bGatling && (parentP->nType == OBJ_PLAYER) && (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling)
			DigiLinkSoundToPos2 (weaponInfoP->flashSound, objP->nSegment, 0, &objP->position.vPos, 0, volume, F1_0 * 256,
										AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
		else
			DigiLinkSoundToPos (weaponInfoP->flashSound, objP->nSegment, 0, &objP->position.vPos, 0, volume);
		}
	else {
		if (nWeaponType == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
			volume = F1_0 / 2;
		if (bGatling && (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling)
			DigiPlaySampleClass (-1, AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]), volume, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER);
		else
			DigiPlaySampleClass (weaponInfoP->flashSound, NULL, volume, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER);
		}
	if (gameOpts->sound.bMissiles && gameData.objs.bIsMissile [nWeaponType]) {
		bBigMsl = (nWeaponType == SMARTMSL_ID) ||
					 (nWeaponType == MEGAMSL_ID) ||
					 (nWeaponType == EARTHSHAKER_ID) ||
					 (nWeaponType == ROBOT_SMARTMSL_ID) ||
					 (nWeaponType == ROBOT_MEGAMSL_ID) ||
					 (nWeaponType == ROBOT_EARTHSHAKER_ID);
		DigiLinkSoundToObject3 (-1, nObject, 1, (gameOpts->sound.xCustomSoundVolume * F1_0) / 10, I2X (256), -1, -1, 
										AddonSoundName (nMslSounds [bBigMsl]), 1, SOUNDCLASS_MISSILE);
		}
	else if (nWeaponType == FLARE_ID)
		DigiSetObjectSound (nObject, -1, AddonSoundName (SND_ADDON_FLARE));
	}
//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
// This also jitters the laser a bit so that it doesn't alias.
//	Don't do for weapons created by weapons.
if ((parentP->nType == OBJ_PLAYER) && (gameData.weapons.info [nWeaponType].renderType != WEAPON_RENDER_NONE) && (nWeaponType != FLARE_ID)) {
	vmsVector	vEndPos;
	int			nEndSeg;

	vEndPos = objP->position.vPos + *vDirection * (gameData.laser.nOffset + (xLaserLength / 2));
	nEndSeg = FindSegByPos (vEndPos, objP->nSegment, 1, 0);
	if (nEndSeg == objP->nSegment)
		objP->position.vPos = vEndPos;
	else if (nEndSeg != -1) {
		objP->position.vPos = vEndPos;
		RelinkObject (OBJ_IDX (objP), nEndSeg);
		}
	}

//	Here's where to fix the problem with OBJECTS which are moving backwards imparting higher velocity to their weaponfire.
//	Find out if moving backwards.
if (!WeaponIsMine (nWeaponType))
	xParentSpeed = 0;
else {
	xParentSpeed = parentP->mType.physInfo.velocity.Mag();
	if (vmsVector::Dot(parentP->mType.physInfo.velocity,
						parentP->position.mOrient[FVEC]) < 0)
		xParentSpeed = -xParentSpeed;
	}

xWeaponSpeed = WI_speed (objP->id, gameStates.app.nDifficultyLevel);
if (weaponInfoP->speedvar != 128) {
	fix randval = F1_0 - ((d_rand () * weaponInfoP->speedvar) >> 6);	//	Get a scale factor between speedvar% and 1.0.
	xWeaponSpeed = FixMul (xWeaponSpeed, randval);
	}
//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
if ((objP->id == SMARTMSL_BLOB_ID) ||
	 (objP->id == SMARTMINE_BLOB_ID) ||
	 (objP->id == ROBOT_SMARTMSL_BLOB_ID) ||
	 (objP->id == ROBOT_SMARTMINE_BLOB_ID) ||
	 (objP->id == EARTHSHAKER_MEGA_ID))
	xWeaponSpeed /= 4;
if (WIThrust (objP->id) != 0)
	xWeaponSpeed /= 2;
/*test*/objP->mType.physInfo.velocity = *vDirection * (xWeaponSpeed + xParentSpeed);
if (parentP)
	gameData.objs.vStartVel [nObject] = parentP->mType.physInfo.velocity;
//	Set thrust
if (WIThrust (nWeaponType) != 0) {
	objP->mType.physInfo.thrust = objP->mType.physInfo.velocity;
	objP->mType.physInfo.thrust *= FixDiv (WIThrust (objP->id), xWeaponSpeed + xParentSpeed);
	}
if ((objP->nType == OBJ_WEAPON) && (objP->id == FLARE_ID))
	objP->lifeleft += (d_rand () - 16384) << 2;		//	add in -2..2 seconds
return nObject;
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls CreateNewLaser, but takes care of the tSegment and point computation for you.
int CreateNewLaserEasy (vmsVector * vDirection, vmsVector * vPosition, short parent, ubyte nWeaponType, int bMakeSound)
{
	tFVIQuery	fq;
	tFVIData		hitData;
	tObject		*parentObjP = OBJECTS + parent;
	int			fate;

	//	Find tSegment containing laser fire vPosition.  If the robot is straddling a tSegment, the vPosition from
	//	which it fires may be in a different tSegment, which is bad news for FindVectorIntersection.  So, cast
	//	a ray from the tObject center (whose tSegment we know) to the laser vPosition.  Then, in the call to CreateNewLaser
	//	use the data returned from this call to FindVectorIntersection.
	//	Note that while FindVectorIntersection is pretty slow, it is not terribly slow if the destination point is
	//	in the same tSegment as the source point.

fq.p0					= &parentObjP->position.vPos;
fq.startSeg			= parentObjP->nSegment;
fq.p1					= vPosition;
fq.radP0				=
fq.radP1				= 0;
fq.thisObjNum		= OBJ_IDX (parentObjP);
fq.ignoreObjList	= NULL;
fq.flags				= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

fate = FindVectorIntersection (&fq, &hitData);
if (fate != HIT_NONE  || hitData.hit.nSegment==-1)
	return -1;
return CreateNewLaser (vDirection, &hitData.hit.vPoint, (short) hitData.hit.nSegment, parent, nWeaponType, bMakeSound);
}

//	-----------------------------------------------------------------------------------------------------------

vmsVector *GetGunPoints (tObject *objP, int nGun)
{
if (!objP)
	return NULL;

	tGunInfo		*giP = gameData.models.gunInfo + objP->rType.polyObjInfo.nModel;
	vmsVector	*vDefaultGunPoints, *vGunPoints;
	int			nDefaultGuns, nGuns;

if (objP->nType == OBJ_PLAYER) {
	vDefaultGunPoints = gameData.pig.ship.player->gunPoints;
	nDefaultGuns = N_PLAYER_GUNS;
	}
else if (objP->nType == OBJ_ROBOT) {
	vDefaultGunPoints = ROBOTINFO (objP->id).gunPoints;
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

vmsVector *TransformGunPoint (tObject *objP, vmsVector *vGunPoints, int nGun,
										fix xDelay, ubyte nLaserType, vmsVector *vMuzzle, vmsMatrix *mP)
{
	int			bSpectate = SPECTATOR (objP);
	tTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->position;
	vmsMatrix	m, *viewP;
	vmsVector	v [2];
#if FULL_COCKPIT_OFFS
	int			bLaserOffs = ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) &&
									  (OBJ_IDX (objP) == LOCALPLAYER.nObject));
#else
	int			bLaserOffs = 0;
#endif

if (nGun < 0) {	// use center between gunPoints nGun and nGun + 1
	(*v) = vGunPoints[-nGun] + vGunPoints[-nGun-1];
//	VmVecScale (VmVecAdd (v, vGunPoints - nGun, vGunPoints - nGun - 1), F1_0 / 2);
	(*v) *= (F1_0 / 2);
}
else {
	v [0] = vGunPoints [nGun];
	if (bLaserOffs)
		v[0] += posP->mOrient[UVEC] * LASER_OFFS;
	}
if (!mP)
	mP = &m;
if (bSpectate) {
   viewP = mP;
	*viewP = posP->mOrient.Transpose();
}
else
   viewP = ObjectView (objP);
v[1] = *viewP * v[0];
memcpy (mP, &posP->mOrient, sizeof (vmsMatrix));
if (nGun < 0)
	v[1] += (*mP)[UVEC] * (-2 * v->Mag());
(*vMuzzle) = posP->vPos + v[1];
//	If supposed to fire at a delayed time (xDelay), then move this point backwards.
if (xDelay)
	*vMuzzle += (*mP)[FVEC]* (-FixMul (xDelay, WI_speed (nLaserType, gameStates.app.nDifficultyLevel)));
return vMuzzle;
}

//-------------- Initializes a laser after Fire is pressed -----------------

int LaserPlayerFireSpreadDelay (
	tObject *objP,
	ubyte nLaserType,
	int nGun,
	fix xSpreadR,
	fix xSpreadU,
	fix xDelay,
	int bMakeSound,
	int bHarmless,
	short	nLightObj)
{
	short			nLaserSeg;
	int			nFate;
	vmsVector	vLaserPos, vLaserDir, *vGunPoints;
	tFVIQuery	fq;
	tFVIData		hitData;
	int			nObject;
	tObject		*laserP;
#if FULL_COCKPIT_OFFS
	int bLaserOffs = ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) &&
							(OBJ_IDX (objP) == LOCALPLAYER.nObject));
#else
	int bLaserOffs = 0;
#endif
	vmsMatrix	m;
	int			bSpectate = SPECTATOR (objP);
	tTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->position;

CreateAwarenessEvent (objP, PA_WEAPON_WALL_COLLISION);
// Find the initial vPosition of the laser
if (!(vGunPoints = GetGunPoints (objP, nGun)))
	return 0;
#if 1
TransformGunPoint (objP, vGunPoints, nGun, xDelay, nLaserType, &vLaserPos, &m);
#else
if (nGun < 0)	// use center between gunPoints nGun and nGun + 1
	VmVecScale (VmVecAdd (&v, vGunPoints - nGun, vGunPoints - nGun - 1), F1_0 / 2);
else {
	v = vGunPoints [nGun];
	if (bLaserOffs)
		VmVecScaleInc (&v, &posP->mOrient[UVEC], LASER_OFFS);
	}
if (bSpectate)
   VmCopyTransposeMatrix (viewP = &m, &posP->mOrient);
else
   viewP = ObjectView (objP);
VmVecRotate (&vGunPoint, &v, viewP);
memcpy (&m, &posP->mOrient, sizeof (vmsMatrix));
if (nGun < 0)
	VmVecScaleInc (&vGunPoint, &m[UVEC], -2 * VmVecMag (&v));
VmVecAdd (&vLaserPos, &posP->vPos, &vGunPoint);
//	If supposed to fire at a delayed time (xDelay), then move this point backwards.
if (xDelay)
	VmVecScaleInc (&vLaserPos, &m[FVEC], -FixMul (xDelay, WI_speed (nLaserType, gameStates.app.nDifficultyLevel)));
#endif

//	DoMuzzleStuff (objP, &Pos);

//--------------- Find vLaserPos and nLaserSeg ------------------
fq.p0					= &posP->vPos;
fq.startSeg			= bSpectate ? gameStates.app.nPlayerSegment : objP->nSegment;
fq.p1					= &vLaserPos;
fq.radP0				=
fq.radP1				= 0x10;
fq.thisObjNum		= OBJ_IDX (objP);
fq.ignoreObjList	= NULL;
fq.flags				= FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS;
nFate = FindVectorIntersection (&fq, &hitData);
nLaserSeg = hitData.hit.nSegment;
if (nLaserSeg == -1) {	//some sort of annoying error
	return -1;
	}
//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
if (vmsVector::Dist(vLaserPos, posP->vPos) > 3 * objP->size / 2) {
	return -1;
	}
if (nFate == HIT_WALL)  {
	return -1;
	}
#if 0
//as of 12/6/94, we don't care if the laser is stuck in an object. We
//just fire away normally
if (nFate == HIT_OBJECT) {
	if (OBJECTS [hitData.hitObject].nType == OBJ_ROBOT)
		KillObject (OBJECTS + hitData.hitObject);
	if (OBJECTS [hitData.hitObject].nType != OBJ_POWERUP)
		return;
	}
#endif
//	Now, make laser spread out.
vLaserDir = m[FVEC];
if (xSpreadR || xSpreadU) {
	vLaserDir += m[RVEC] * xSpreadR;
	vLaserDir += m[UVEC] * xSpreadU;
	}
if (bLaserOffs)
	vLaserDir += m[UVEC] * LASER_OFFS;
nObject = CreateNewLaser (&vLaserDir, &vLaserPos, nLaserSeg, OBJ_IDX (objP), nLaserType, bMakeSound);
//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
if (nLaserType == OMEGA_ID)
	return -1;
if (nObject == -1) {
	return -1;
	}
if ((nLaserType == GUIDEDMSL_ID) && gameData.multigame.bIsGuided)
	gameData.objs.guidedMissile [objP->id].objP = OBJECTS + nObject;
gameData.multigame.bIsGuided = 0;
laserP = OBJECTS + nObject;
if (gameData.objs.bIsMissile [nLaserType] && (nLaserType != GUIDEDMSL_ID)) {
	if (!gameData.objs.missileViewer && (objP->id == gameData.multiplayer.nLocalPlayer))
		gameData.objs.missileViewer = laserP;
	}
//	If this weapon is supposed to be silent, set that bit!
if (!bMakeSound)
	laserP->flags |= OF_SILENT;
//	If this weapon is supposed to be silent, set that bit!
if (bHarmless)
	laserP->flags |= OF_HARMLESS;

//	If the object firing the laser is the tPlayer, then indicate the laser object so robots can dodge.
//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
if ((objP == gameData.objs.console) && !WeaponIsPlayerMine (laserP->id))
	gameStates.app.bPlayerFiredLaserThisFrame = nObject;

if (gameStates.app.cheats.bHomingWeapons || gameData.weapons.info [nLaserType].homingFlag) {
	if (objP == gameData.objs.console) {
		laserP->cType.laserInfo.nMslLock = FindHomingObject (&vLaserPos, laserP);
		gameData.multigame.laser.nTrack = laserP->cType.laserInfo.nMslLock;
		}
	else {// Some other tPlayer shot the homing thing
		Assert (IsMultiGame);
		laserP->cType.laserInfo.nMslLock = gameData.multigame.laser.nTrack;
		}
	}
gameData.objs.lightObjs [nObject].nObject = nLightObj;
if (nLightObj >= 0) {
	gameData.objs.lightObjs [nObject].nSignature = OBJECTS [nLightObj].nSignature;
	OBJECTS [nLightObj].cType.lightInfo.nObjects++;
	}
return nObject;
}

//	-----------------------------------------------------------------------------------------------------------

void CreateFlare (tObject *objP)
{
	fix	xEnergyUsage = WI_energy_usage (FLARE_ID);

if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsage = FixMul (xEnergyUsage, I2X (gameStates.app.nDifficultyLevel+2)/4);
LOCALPLAYER.energy -= xEnergyUsage;
if (LOCALPLAYER.energy <= 0) {
	LOCALPLAYER.energy = 0;
	}
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

float MissileSpeedScale (tObject *objP)
{
	int	i = extraGameInfo [IsMultiGame].nMslStartSpeed;

if (!i)
	return 1;
return nMslSlowDown [i] * X2F (gameData.time.xGame - gameData.objs.xCreationTime [OBJ_IDX (objP)]);
}

//-------------------------------------------------------------------------------------------
//	Set object *objP's orientation to (or towards if I'm ambitious) its velocity.

void HomingMissileTurnTowardsVelocity (tObject *objP, vmsVector *vNormVel)
{
	vmsVector	vNewDir;
	fix 			frameTime;

frameTime = gameStates.limitFPS.bHomers ? SECS2X (gameStates.app.tick40fps.nTime) : gameData.time.xFrame;
vNewDir = *vNormVel;
vNewDir *= ((fix) (frameTime * 16 / gameStates.gameplay.slowmo [0].fSpeed));
vNewDir += objP->position.mOrient[FVEC];
vmsVector::Normalize(vNewDir);
/*
objP->position.mOrient = vmsMatrix::Create(vNewDir, NULL, NULL);
*/
// TODO: MatrixCreateFCheck
objP->position.mOrient = vmsMatrix::CreateF(vNewDir);
}

//-------------------------------------------------------------------------------------------

static inline fix HomingMslStraightTime (int id)
{
	int i = ((id == EARTHSHAKER_MEGA_ID) || (id == ROBOT_SHAKER_MEGA_ID)) ? 1 : nMslSlowDown [(int) extraGameInfo [IsMultiGame].nMslStartSpeed];

return i * HOMINGMSL_STRAIGHT_TIME;
}

//-------------------------------------------------------------------------------------------
//sequence this laser tObject for this _frame_ (underscores added here to aid MK in his searching!)
void LaserDoWeaponSequence (tObject *objP)
{
	tObject	*gmObjP;
	fix		xWeaponSpeed, xScaleFactor, xDistToPlayer;

Assert (objP->controlType == CT_WEAPON);
//	Ok, this is a big hack by MK.
//	If you want an tObject to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME
if (objP->lifeleft == ONE_FRAME_TIME) {
	if (IsMultiGame)
		objP->lifeleft = OMEGA_MULTI_LIFELEFT;
	else
		objP->lifeleft = 0;
	objP->renderType = RT_NONE;
	}
if (objP->lifeleft < 0) {		// We died of old age
	KillObject (objP);
	if (WI_damage_radius (objP->id))
		ExplodeBadassWeapon (objP,&objP->position.vPos);
	return;
	}
//delete weapons that are not moving
xWeaponSpeed = objP->mType.physInfo.velocity.Mag();
if (!((gameData.app.nFrameCount ^ objP->nSignature) & 3) &&
		(objP->nType == OBJ_WEAPON) && (objP->id != FLARE_ID) &&
		(gameData.weapons.info [objP->id].speed [gameStates.app.nDifficultyLevel] > 0) &&
		(xWeaponSpeed < F2_0)) {
	ReleaseObject (OBJ_IDX (objP));
	return;
	}
if ((objP->nType == OBJ_WEAPON) && (objP->id == FUSION_ID)) {		//always set fusion weapon to max vel
	vmsVector::Normalize(objP->mType.physInfo.velocity);
	objP->mType.physInfo.velocity *= (WI_speed (objP->id,gameStates.app.nDifficultyLevel));
	}
//	For homing missiles, turn towards target. (unless it's the guided missile)
if ((gameOpts->legacy.bHomers || !gameStates.limitFPS.bHomers || gameStates.app.tick40fps.bTick) &&
	 (objP->nType == OBJ_WEAPON) &&
    (gameStates.app.cheats.bHomingWeapons || WI_homingFlag (objP->id)) &&
	 !((objP->id == GUIDEDMSL_ID) &&
	   (objP == (gmObjP = gameData.objs.guidedMissile [OBJECTS [objP->cType.laserInfo.nParentObj].id].objP)) &&
	   (objP->nSignature == gmObjP->nSignature))) {
	vmsVector	vVecToObject, vTemp;
	fix			dot = F1_0;
	fix			speed, xMaxSpeed;
	int			id = objP->id;
	//	For first 1/2 second of life, missile flies straight.
	if (objP->cType.laserInfo.creationTime + HomingMslStraightTime (id) < gameData.time.xGame) {
		int	nMslLock = objP->cType.laserInfo.nMslLock;

		//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
		if ((id == ROBOT_SMARTMINE_BLOB_ID) ||
			 (id == ROBOT_SMARTMSL_BLOB_ID) ||
			 (id == SMARTMINE_BLOB_ID) ||
			 (id == SMARTMSL_BLOB_ID) ||
			 (id == EARTHSHAKER_MEGA_ID))
			objP->mType.physInfo.flags &= ~PF_BOUNCE;

		//	Make sure the tObject we are tracking is still trackable.
		nMslLock = TrackMslLock (nMslLock, objP, &dot);
		if (nMslLock == LOCALPLAYER.nObject) {
			xDistToPlayer = vmsVector::Dist(objP->position.vPos, OBJECTS [nMslLock].position.vPos);
			if ((xDistToPlayer < LOCALPLAYER.homingObjectDist) || (LOCALPLAYER.homingObjectDist < 0))
				LOCALPLAYER.homingObjectDist = xDistToPlayer;

			}
		if (nMslLock != -1) {
			vVecToObject = OBJECTS [nMslLock].position.vPos - objP->position.vPos;
			vmsVector::Normalize(vVecToObject);
			vTemp = objP->mType.physInfo.velocity;
			speed = vmsVector::Normalize(vTemp);
			xMaxSpeed = WI_speed (objP->id,gameStates.app.nDifficultyLevel);
			if (speed + F1_0 < xMaxSpeed) {
				speed += FixMul (xMaxSpeed, gameData.time.xFrame / 2);
				if (speed > xMaxSpeed)
					speed = xMaxSpeed;
				}
			if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (objP->id == EARTHSHAKER_MEGA_ID)) {
				fix	h = (objP->lifeleft + F1_0 - 1) / F1_0;

				if (h > 7)
					vVecToObject *= (F1_0 / (h - 6));
				}
			// -- dot = vmsVector::Dot(vTemp, vVecToObject);
			vVecToObject *= (F1_0 / HomingMslScale ());
			vTemp += vVecToObject;
			//	The boss' smart children track better...
			if (gameData.weapons.info [objP->id].renderType != WEAPON_RENDER_POLYMODEL)
				vTemp += vVecToObject;
			vmsVector::Normalize(vTemp);
			objP->mType.physInfo.velocity = vTemp;
			objP->mType.physInfo.velocity *= speed;

			//	Subtract off life proportional to amount turned.
			//	For hardest turn, it will lose 2 seconds per second.
				{
				fix	absdot = abs (F1_0 - dot);
            fix	lifelost = FixMul (absdot*32, gameData.time.xFrame);
				objP->lifeleft -= lifelost;
				}

			//	Only polygon OBJECTS have visible orientation, so only they should turn.
			if (gameData.weapons.info [objP->id].renderType == WEAPON_RENDER_POLYMODEL)
				HomingMissileTurnTowardsVelocity (objP, &vTemp);		//	vTemp is normalized velocity.
			}
		}
	}
	//	Make sure weapon is not moving faster than allowed speed.
if ((objP->nType == OBJ_WEAPON) &&
	 (xWeaponSpeed > WI_speed (objP->id, gameStates.app.nDifficultyLevel))) {
	//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
	if (WI_speed (objP->id, gameStates.app.nDifficultyLevel)) {
		xScaleFactor = FixDiv (WI_speed (objP->id,gameStates.app.nDifficultyLevel), xWeaponSpeed);
		objP->mType.physInfo.velocity *= xScaleFactor;
		}
	}
}

//	--------------------------------------------------------------------------------------------------

void StopPrimaryFire (void)
{
#if 0
gameData.laser.xNextFireTime = gameData.time.xGame;	//	Prevents shots-to-fire from building up.
#else
if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
	gameData.laser.xNextFireTime = gameData.time.xGame + F1_0 / 25;
else
	gameData.laser.xNextFireTime = gameData.time.xGame + WI_fire_wait (gameData.weapons.nPrimary);
#endif
gameData.laser.nGlobalFiringCount = 0;
Controls [0].firePrimaryState = 0;
Controls [0].firePrimaryDownCount = 0;
gameData.weapons.firing [0].nStart =
gameData.weapons.firing [0].nStop =
gameData.weapons.firing [0].nDuration = 0;
}

//	--------------------------------------------------------------------------------------------------

void StopSecondaryFire (void)
{
gameData.missiles.xNextFireTime = gameData.time.xGame;	//	Prevents shots-to-fire from building up.
gameData.missiles.nGlobalFiringCount = 0;
Controls [0].fireSecondaryState = 0;
Controls [0].fireSecondaryDownCount = 0;
}

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console tPlayer, not for network players

int LocalPlayerFireLaser (void)
{
	tPlayer	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
	fix		xEnergyUsed;
	int		nAmmoUsed, nPrimaryAmmo;
	int		nWeaponIndex;
	int		rVal = 0;
	int 		nRoundsPerShot = 1;
	fix		addval;
	static int nSpreadfireToggle = 0;
	static int nHelixOrient = 0;

if (gameStates.app.bPlayerIsDead)
	return 0;
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [OBJECTS [playerP->nObject].nSegment].special == SEGMENT_IS_NODAMAGE))
	return 0;
nWeaponIndex = primaryWeaponToWeaponInfo [gameData.weapons.nPrimary];
xEnergyUsed = WI_energy_usage (nWeaponIndex);
if (gameData.weapons.nPrimary == OMEGA_INDEX)
	xEnergyUsed = 0;	//	Omega consumes energy when recharging, not when firing.
if (gameStates.app.nDifficultyLevel < 2)
	xEnergyUsed = FixMul (xEnergyUsed, I2X (gameStates.app.nDifficultyLevel+2)/4);
//	MK, 01/26/96, Helix use 2x energy in multiplayer.  bitmaps.tbl parm should have been reduced for single player.
if (nWeaponIndex == HELIX_INDEX)
	if (IsMultiGame)
		xEnergyUsed *= 2;
nAmmoUsed = WI_ammo_usage (nWeaponIndex);
addval = 2 * gameData.time.xFrame;
if (addval > F1_0)
	addval = F1_0;
if ((gameData.weapons.nPrimary != VULCAN_INDEX) && (gameData.weapons.nPrimary != GAUSS_INDEX))
	nPrimaryAmmo = playerP->primaryAmmo [gameData.weapons.nPrimary];
else {
	if ((gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling &&
		 gameStates.app.bHaveExtraGameInfo [IsMultiGame] && EGI_FLAG (bGatlingSpeedUp, 1, 0, 0) &&
		 (gameData.weapons.firing [0].nDuration < GATLING_DELAY))
		return 0;
	nPrimaryAmmo = playerP->primaryAmmo [VULCAN_INDEX];
	}
if	 ((playerP->energy < xEnergyUsed) || (nPrimaryAmmo < nAmmoUsed))
	AutoSelectWeapon (0, 1);		//	Make sure the tPlayer can fire from this weapon.

if ((gameData.laser.xLastFiredTime + 2 * gameData.time.xFrame < gameData.time.xGame) ||
	 (gameData.time.xGame < gameData.laser.xLastFiredTime))
	gameData.laser.xNextFireTime = gameData.time.xGame;
gameData.laser.xLastFiredTime = gameData.time.xGame;

while (gameData.laser.xNextFireTime <= gameData.time.xGame) {
	if	((playerP->energy >= xEnergyUsed) && (nPrimaryAmmo >= nAmmoUsed)) {
			int nLaserLevel, flags = 0;

		if (gameStates.app.cheats.bLaserRapidFire == 0xBADA55)
			gameData.laser.xNextFireTime += F1_0 / 25;
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
		rVal += LaserFireObject ((short) LOCALPLAYER.nObject, (ubyte) gameData.weapons.nPrimary, nLaserLevel, flags, nRoundsPerShot);
		playerP->energy -= (xEnergyUsed * rVal) / gameData.weapons.info [nWeaponIndex].fireCount;
		if (playerP->energy < 0)
			playerP->energy = 0;
		if (rVal && ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX))) {
			if (nAmmoUsed > playerP->primaryAmmo [VULCAN_INDEX])
				playerP->primaryAmmo [VULCAN_INDEX] = 0;
			else
				playerP->primaryAmmo [VULCAN_INDEX] -= nAmmoUsed;
			}
		AutoSelectWeapon (0, 1);		//	Make sure the tPlayer can fire from this weapon.
		}
	else {
		AutoSelectWeapon (0, 1);		//	Make sure the tPlayer can fire from this weapon.
		StopPrimaryFire ();
		break;	//	Couldn't fire weapon, so abort.
		}
	}
gameData.laser.nGlobalFiringCount = 0;
return rVal;
}

// -- #define	MAX_LIGHTNING_DISTANCE	 (F1_0*300)
// -- #define	MAX_LIGHTNING_BLOBS		16
// -- #define	LIGHTNING_BLOB_DISTANCE	 (MAX_LIGHTNING_DISTANCE/MAX_LIGHTNING_BLOBS)
// --
// -- #define	LIGHTNING_BLOB_ID			13
// --
// -- #define	LIGHTNING_TIME		 (F1_0/4)
// -- #define	LIGHTNING_DELAY	 (F1_0/8)
// --
// -- int	Lightning_gun_num = 1;
// --
// -- fix	Lightning_startTime = -F1_0*10, Lightning_lastTime;
// --
// -- //	--------------------------------------------------------------------------------------------------
// -- //	Return -1 if failed to create at least one blob.  Else return index of last blob created.
// -- int create_lightning_blobs (vmsVector *vDirection, vmsVector *start_pos, int start_segnum, int parent)
// -- {
// -- 	int			i;
// -- 	tFVIQuery	fq;
// -- 	tFVIData		hitData;
// -- 	vmsVector	vEndPos;
// -- 	vmsVector	norm_dir;
// -- 	int			fate;
// -- 	int			num_blobs;
// -- 	vmsVector	tvec;
// -- 	fix			dist_to_hit_point;
// -- 	vmsVector	point_pos, delta_pos;
// -- 	int			nObject;
// -- 	vmsVector	*gun_pos;
// -- 	vmsMatrix	m;
// -- 	vmsVector	gun_pos2;
// --
// -- 	if (LOCALPLAYER.energy > F1_0)
// -- 		LOCALPLAYER.energy -= F1_0;
// --
// -- 	if (LOCALPLAYER.energy <= F1_0) {
// -- 		LOCALPLAYER.energy = 0;
// -- 		AutoSelectWeapon (0);
// -- 		return -1;
// -- 	}
// --
// -- 	norm_dir = *vDirection;
// --
// -- 	vmsVector::Normalize(&norm_dir);
// -- 	VmVecScaleAdd (&vEndPos, start_pos, &norm_dir, MAX_LIGHTNING_DISTANCE);
// --
// -- 	fq.p0						= start_pos;
// -- 	fq.startSeg				= start_segnum;
// -- 	fq.p1						= &vEndPos;
// -- 	fq.rad					= 0;
// -- 	fq.thisObjNum			= parent;
// -- 	fq.ignoreObjList	= NULL;
// -- 	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;
// --
// -- 	fate = FindVectorIntersection (&fq, &hitData);
// -- 	if (hitData.hit.nSegment == -1) {
// -- 		return -1;
// -- 	}
// --
// -- 	dist_to_hit_point = VmVecMag (VmVecSub (&tvec, &hitData.hit.vPoint, start_pos);
// -- 	num_blobs = dist_to_hit_point/LIGHTNING_BLOB_DISTANCE;
// --
// -- 	if (num_blobs > MAX_LIGHTNING_BLOBS)
// -- 		num_blobs = MAX_LIGHTNING_BLOBS;
// --
// -- 	if (num_blobs < MAX_LIGHTNING_BLOBS/4)
// -- 		num_blobs = MAX_LIGHTNING_BLOBS/4;
// --
// -- 	// Find the initial vPosition of the laser
// -- 	gun_pos = &gameData.pig.ship.player->gunPoints [Lightning_gun_num];
// -- 	VmCopyTransposeMatrix (&m,&OBJECTS [parent].position.mOrient);
// -- 	VmVecRotate (&gun_pos2, gun_pos, &m);
// -- 	VmVecAdd (&point_pos, &OBJECTS [parent].position.vPos, &gun_pos2);
// --
// -- 	delta_pos = norm_dir;
// -- 	VmVecScale (&delta_pos, dist_to_hit_point/num_blobs);
// --
// -- 	for (i=0; i<num_blobs; i++) {
// -- 		int			tPointSeg;
// -- 		tObject		*obj;
// --
// -- 		VmVecInc (&point_pos, &delta_pos);
// -- 		tPointSeg = FindSegByPos (&point_pos, start_segnum, 1, 0);
// -- 		if (tPointSeg == -1)	//	Hey, we thought we were creating points on a line, but we left the mine!
// -- 			continue;
// --
// -- 		nObject = CreateNewLaser (vDirection, &point_pos, tPointSeg, parent, LIGHTNING_BLOB_ID, 0);
// --
// -- 		if (nObject < 0) 	{
// -- 			Int3 ();
// -- 			return -1;
// -- 		}
// --
// -- 		obj = OBJECTS + nObject;
// --
// -- 		DigiPlaySample (gameData.weapons.info [objP->id].flashSound, F1_0);
// --
// -- 		// -- VmVecScale (&objP->mType.physInfo.velocity, F1_0/2);
// --
// -- 		objP->lifeleft = (LIGHTNING_TIME + LIGHTNING_DELAY)/2;
// --
// -- 	}
// --
// -- 	return nObject;
// --
// -- }
// --
// -- //	--------------------------------------------------------------------------------------------------
// -- //	Lightning Cannon.
// -- //	While being fired, creates path of blobs forward from tPlayer until it hits something.
// -- //	Up to MAX_LIGHTNING_BLOBS blobs, spaced LIGHTNING_BLOB_DISTANCE units apart.
// -- //	When the tPlayer releases the firing key, the blobs move forward.
// -- void lightning_frame (void)
// -- {
// -- 	if ((gameData.time.xGame - Lightning_startTime < LIGHTNING_TIME) && (gameData.time.xGame - Lightning_startTime > 0)) {
// -- 		if (gameData.time.xGame - Lightning_lastTime > LIGHTNING_DELAY) {
// -- 			create_lightning_blobs (&gameData.objs.console->position.mOrient[FVEC], &gameData.objs.console->position.vPos, gameData.objs.console->nSegment, OBJ_IDX (gameData.objs.console));
// -- 			Lightning_lastTime = gameData.time.xGame;
// -- 		}
// -- 	}
// -- }

//	-----------------------------------------------------------------------------------------------------------

short CreateClusterLight (tObject *objP)
{
if (!gameStates.render.bClusterLights)
	return -1;
short nObject = tObject::Create(OBJ_LIGHT, CLUSTER_LIGHT_ID, -1, objP->nSegment, OBJPOS (objP)->vPos, vmsMatrix::IDENTITY, 0, CT_LIGHT, MT_NONE, RT_NONE, 1);
if (nObject >= 0)
	OBJECTS [nObject].lifeleft = IMMORTAL_TIME;
return nObject;
}

//	--------------------------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int ( __fastcall * pWeaponHandler) (tObject *, int, int, int);
#else
typedef int (* pWeaponHandler) (tObject *, int, int, int);
#endif

//-------------------------------------------

int LaserHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	ubyte	nLaser = (nLevel <= MAX_LASER_LEVEL) ? LASER_ID + nLevel : SUPERLASER_ID + (nLevel - MAX_LASER_LEVEL - 1);
	short	nLightObj = CreateClusterLight (objP);

gameData.laser.nOffset = (F1_0 * 2 * (d_rand () % 8)) / 8;
LaserPlayerFire (objP, nLaser, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, nLaser, 1, 0, 0, nLightObj);
if (nFlags & LASER_QUAD) {
	//	hideous system to make quad laser 1.5x powerful as Normal laser, make every other quad laser bolt bHarmless
	LaserPlayerFire (objP, nLaser, 2, 0, 0, nLightObj);
	LaserPlayerFire (objP, nLaser, 3, 0, 0, nLightObj);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int VulcanHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
#	define VULCAN_SPREAD	(d_rand ()/8 - 32767/16)

	int bGatlingSound = gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling;

if (bGatlingSound && (gameData.weapons.firing [objP->id].nDuration <= GATLING_DELAY))
	return 0;
//	Only make sound for 1/4 of vulcan bullets.
LaserPlayerFireSpread (objP, VULCAN_ID, 6, VULCAN_SPREAD, VULCAN_SPREAD, 1, 0, -1);
if (nRoundsPerShot > 1) {
	LaserPlayerFireSpread (objP, VULCAN_ID, 6, VULCAN_SPREAD, VULCAN_SPREAD, 0, 0, -1);
	if (nRoundsPerShot > 2)
		LaserPlayerFireSpread (objP, VULCAN_ID, 6, VULCAN_SPREAD, VULCAN_SPREAD, 0, 0, -1);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int SpreadfireHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	short	nLightObj = CreateClusterLight (objP);

if (nFlags & LASER_SPREADFIRE_TOGGLED) {
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, F1_0/16, 0, 0, 0, nLightObj);
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, -F1_0/16, 0, 0, 0, nLightObj);
	}
else {
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, F1_0/16, 0, 0, nLightObj);
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, -F1_0/16, 0, 0, nLightObj);
	}
LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, 0, 1, 0, nLightObj);
return nRoundsPerShot;
}

//-------------------------------------------

int PlasmaHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	short	nLightObj = CreateClusterLight (objP);

LaserPlayerFire (objP, PLASMA_ID, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, PLASMA_ID, 1, 0, 0, nLightObj);
if (nRoundsPerShot > 1) {
	nLightObj = CreateClusterLight (objP);

	LaserPlayerFireSpreadDelay (objP, PLASMA_ID, 0, 0, 0, gameData.time.xFrame / 2, 1, 0, nLightObj);
	LaserPlayerFireSpreadDelay (objP, PLASMA_ID, 1, 0, 0, gameData.time.xFrame / 2, 0, 0, nLightObj);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int FusionHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	vmsVector	vForce;
	short			nLightObj = CreateClusterLight (objP);

LaserPlayerFire (objP, FUSION_ID, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, FUSION_ID, 1, 1, 0, nLightObj);
if (EGI_FLAG (bTripleFusion, 0, 0, 0) && gameData.multiplayer.weaponStates [objP->id].bTripleFusion)
#if 1
	LaserPlayerFire (objP, FUSION_ID, 6, 1, 0, nLightObj);
#else
	LaserPlayerFire (objP, FUSION_ID, -1, 1, 0, nLightObj);
#endif
nFlags = (sbyte) (gameData.fusion.xCharge >> 12);
gameData.fusion.xCharge = 0;
vForce[X] = -(objP->position.mOrient[FVEC][X] << 7);
vForce[Y] = -(objP->position.mOrient[FVEC][Y] << 7);
vForce[Z] = -(objP->position.mOrient[FVEC][Z] << 7);
PhysApplyForce (objP, &vForce);
vForce[X] = (vForce[X] >> 4) + d_rand () - 16384;
vForce[Y] = (vForce[Y] >> 4) + d_rand () - 16384;
vForce[Z] = (vForce[Z] >> 4) + d_rand () - 16384;
PhysApplyRot (objP, &vForce);
return nRoundsPerShot;
}

//-------------------------------------------

int SuperlaserHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	ubyte nSuperLevel = 3;		//make some new kind of laser eventually
	short	nLightObj = CreateClusterLight (objP);

LaserPlayerFire (objP, nSuperLevel, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, nSuperLevel, 1, 0, 0, nLightObj);

if (nFlags & LASER_QUAD) {
	//	hideous system to make quad laser 1.5x powerful as Normal laser, make every other quad laser bolt bHarmless
	LaserPlayerFire (objP, nSuperLevel, 2, 0, 0, nLightObj);
	LaserPlayerFire (objP, nSuperLevel, 3, 0, 0, nLightObj);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int GaussHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
#	define GAUSS_SPREAD		(VULCAN_SPREAD / 5)

	int			bGatlingSound = gameStates.app.bHaveExtraGameInfo [IsMultiGame] &&
										 (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling;
	tFiringData *fP = gameData.multiplayer.weaponStates [objP->id].firing;

if (bGatlingSound && (fP->nDuration <= GATLING_DELAY))
	return 0;
//	Only make sound for 1/4 of vulcan bullets.
LaserPlayerFireSpread (objP, GAUSS_ID, 6, GAUSS_SPREAD, GAUSS_SPREAD,
							  (objP->id != gameData.multiplayer.nLocalPlayer) || (gameData.laser.xNextFireTime > gameData.time.xGame), 0, -1);
if (nRoundsPerShot > 1) {
	LaserPlayerFireSpread (objP, GAUSS_ID, 6, GAUSS_SPREAD, GAUSS_SPREAD, 0, 0, -1);
	if (nRoundsPerShot > 2)
		LaserPlayerFireSpread (objP, GAUSS_ID, 6, GAUSS_SPREAD, GAUSS_SPREAD, 0, 0, -1);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int HelixHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	typedef struct tSpread {
		fix	r, u;
		} tSpread;

	static tSpread spreadTable [8] = {
		{F1_0 / 16, 0},
		{F1_0 / 17, F1_0 / 42},
		{F1_0 / 22, F1_0 / 22},
		{F1_0 / 42, F1_0 / 17},
		{0, F1_0 / 16},
		{-F1_0 / 42, F1_0 / 17},
		{-F1_0 / 22, F1_0 / 22},
		{-F1_0 / 17, F1_0 / 42}
		};

	tSpread	spread = spreadTable [(nFlags >> LASER_HELIX_SHIFT) & LASER_HELIX_MASK];
	short		nLightObj = CreateClusterLight (objP);

LaserPlayerFireSpread (objP, HELIX_ID, 6,  0,  0, 1, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6,  spread.r,  spread.u, 0, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6, -spread.r, -spread.u, 0, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6,  spread.r * 2,  spread.u * 2, 0, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6, -spread.r * 2, -spread.u * 2, 0, 0, nLightObj);
return nRoundsPerShot;
}

//-------------------------------------------

int PhoenixHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	short	nLightObj = CreateClusterLight (objP);

LaserPlayerFire (objP, PHOENIX_ID, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, PHOENIX_ID, 1, 0, 0, nLightObj);
if (nRoundsPerShot > 1) {
	nLightObj = CreateClusterLight (objP);
	LaserPlayerFireSpreadDelay (objP, PHOENIX_ID, 0, 0, 0, gameData.time.xFrame / 2, 1, 0, nLightObj);
	LaserPlayerFireSpreadDelay (objP, PHOENIX_ID, 1, 0, 0, gameData.time.xFrame / 2, 0, 0, nLightObj);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int OmegaHandler (tObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
LaserPlayerFire (objP, OMEGA_ID, 6, 1, 0, -1);
return nRoundsPerShot;
}

//-------------------------------------------

pWeaponHandler weaponHandlers [] = {
	LaserHandler,
	VulcanHandler,
	SpreadfireHandler,
	PlasmaHandler,
	FusionHandler,
	SuperlaserHandler,
	GaussHandler,
	HelixHandler,
	PhoenixHandler,
	OmegaHandler
	};



//	--------------------------------------------------------------------------------------------------
//	Object "nObject" fires weapon "weapon_num" of level "level". (Right now (9/24/94) level is used only for nType 0 laser.
//	Flags are the tPlayer flags.  For network mode, set to 0.
//	It is assumed that this is a tPlayer tObject (as in multiplayer), and therefore the gun positions are known.
//	Returns number of times a weapon was fired.  This is typically 1, but might be more for low frame rates.
//	More than one shot is fired with a pseudo-delay so that players on show machines can fire (for themselves
//	or other players) often enough for things like the vulcan cannon.

int LaserFireObject (short nObject, ubyte nWeapon, int nLevel, int nFlags, int nRoundsPerShot)
{
if (nWeapon > OMEGA_INDEX) {
	gameData.weapons.nPrimary = 0;
	nRoundsPerShot = 0;
	}
else
	nRoundsPerShot = weaponHandlers [nWeapon] (OBJECTS + nObject, nLevel, nFlags, nRoundsPerShot);
if (IsMultiGame && (nObject == LOCALPLAYER.nObject)) {
	gameData.multigame.laser.bFired = nRoundsPerShot;
	gameData.multigame.laser.nGun = nWeapon;
	gameData.multigame.laser.nFlags = nFlags;
	gameData.multigame.laser.nLevel = nLevel;
	}
return nRoundsPerShot;
}

//	-------------------------------------------------------------------------------------------
//	if nGoalObj == -1, then create Random vector
int CreateHomingMissile (tObject *objP, int nGoalObj, ubyte objType, int bMakeSound)
{
	short			nObject;
	vmsVector	vGoal;
	vmsVector	random_vector;
	//vmsVector	vGoalPos;

	if (nGoalObj == -1) {
		vGoal = vmsVector::Random();
	} else {
		vmsVector::NormalizedDir(vGoal, OBJECTS [nGoalObj].position.vPos, objP->position.vPos);
		random_vector = vmsVector::Random();
		vGoal += random_vector * (F1_0/4);
		vmsVector::Normalize(vGoal);
	}

	//	Create a vector towards the goal, then add some noise to it.
	nObject = CreateNewLaser (&vGoal, &objP->position.vPos, objP->nSegment,
									  OBJ_IDX (objP), objType, bMakeSound);
	if (nObject == -1)
		return -1;

	// Fixed to make sure the right person gets credit for the kill

//	OBJECTS [nObject].cType.laserInfo.nParentObj = objP->cType.laserInfo.nParentObj;
//	OBJECTS [nObject].cType.laserInfo.parentType = objP->cType.laserInfo.parentType;
//	OBJECTS [nObject].cType.laserInfo.nParentSig = objP->cType.laserInfo.nParentSig;

	OBJECTS [nObject].cType.laserInfo.nMslLock = nGoalObj;

	return nObject;
}

//-----------------------------------------------------------------------------
// Create the children of a smart bomb, which is a bunch of homing missiles.
void CreateSmartChildren (tObject *objP, int nSmartChildren)
{
	int	parentType, nParentObj;
	int	bMakeSound;
	int	nObjects = 0;
	int	objList [MAX_OBJDISTS];
	ubyte	nBlobId;

if (objP->nType == OBJ_WEAPON) {
	parentType = objP->cType.laserInfo.parentType;
	nParentObj = objP->cType.laserInfo.nParentObj;
	}
else if (objP->nType == OBJ_ROBOT) {
	parentType = OBJ_ROBOT;
	nParentObj = OBJ_IDX (objP);
	}
else {
	Int3 ();	//	Hey, what kind of tObject is this!?
	parentType = 0;
	nParentObj = 0;
	}

if (objP->id == EARTHSHAKER_ID)
	BlastNearbyGlass (objP, gameData.weapons.info [EARTHSHAKER_ID].strength [gameStates.app.nDifficultyLevel]);

if ((objP->nType == OBJ_WEAPON) &&
		((objP->id == SMARTMSL_ID) || (objP->id == SMARTMINE_ID) || (objP->id == ROBOT_SMARTMINE_ID) || (objP->id == EARTHSHAKER_ID)) &&
		(gameData.weapons.info [objP->id].children == -1))
	return;

if (((objP->nType == OBJ_WEAPON) && (gameData.weapons.info [objP->id].children != -1)) || (objP->nType == OBJ_ROBOT)) {
	int	i, nObject;
	tObject	*curObjP;

	if (IsMultiGame)
		d_srand (8321L);

	for (nObject = 0, curObjP = OBJECTS; nObject <= gameData.objs.nLastObject [0]; nObject++, curObjP++) {
		if ((((curObjP->nType == OBJ_ROBOT) && (!curObjP->cType.aiInfo.CLOAKED)) || (curObjP->nType == OBJ_PLAYER)) && (nObject != nParentObj)) {
			fix	dist;
			if (curObjP->nType == OBJ_PLAYER) {
				if ((parentType == OBJ_PLAYER) && (IsCoopGame))
					continue;
				if (IsTeamGame && (GetTeam (curObjP->id) == GetTeam (OBJECTS [nParentObj].id)))
					continue;
				if (gameData.multiplayer.players [curObjP->id].flags & PLAYER_FLAGS_CLOAKED)
					continue;
				}

			//	Robot blobs can't track robots.
			if (curObjP->nType == OBJ_ROBOT) {
				if (parentType == OBJ_ROBOT)
					continue;
				//	Your shots won't track the buddy.
				if (parentType == OBJ_PLAYER)
					if (ROBOTINFO (curObjP->id).companion)
						continue;
				}
			dist = vmsVector::Dist(objP->position.vPos, curObjP->position.vPos);
			if (dist < MAX_SMART_DISTANCE) {
				int	oovis = ObjectToObjectVisibility (objP, curObjP, FQ_TRANSWALL);
				if (oovis) { //ObjectToObjectVisibility (objP, curObjP, FQ_TRANSWALL)) {
					objList [nObjects] = nObject;
					nObjects++;
					if (nObjects >= MAX_OBJDISTS) {
						nObjects = MAX_OBJDISTS;
						break;
						}
					}
				}
			}
		}

	//	Get nType of weapon for child from parent.
	if (objP->nType == OBJ_WEAPON) {
		nBlobId = gameData.weapons.info [objP->id].children;
		Assert (nBlobId != -1);		//	Hmm, missing data in bitmaps.tbl.  Need "children=NN" parameter.
		}
	else {
		Assert (objP->nType == OBJ_ROBOT);
		nBlobId = ROBOT_SMARTMSL_BLOB_ID;
		}
	bMakeSound = 1;
	for (i = 0; i < nSmartChildren; i++) {
		short nObject = (nObjects==0) ? -1 : objList [(d_rand () * nObjects) >> 15];
		CreateHomingMissile (objP, nObject, nBlobId, bMakeSound);
		bMakeSound = 0;
		}
	}
}

//	-------------------------------------------------------------------------------------------

//give up control of the guided missile
void ReleaseGuidedMissile (int nPlayer)
{
if (nPlayer == gameData.multiplayer.nLocalPlayer) {
	if (!gameData.objs.guidedMissile [nPlayer].objP)
		return;
	gameData.objs.missileViewer = gameData.objs.guidedMissile [nPlayer].objP;
	if (IsMultiGame)
	 	MultiSendGuidedInfo (gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP, 1);
	if (gameData.demo.nState == ND_STATE_RECORDING)
	 	NDRecordGuidedEnd ();
	 }
gameData.objs.guidedMissile [nPlayer].objP = NULL;
}

int nProximityDropped = 0, nSmartminesDropped = 0;

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
	tObject	*gmObjP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
	tPlayer	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;

Assert (gameData.weapons.nSecondary < MAX_SECONDARY_WEAPONS);
if (gmObjP && (gmObjP->nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature)) {
	ReleaseGuidedMissile (gameData.multiplayer.nLocalPlayer);
	i = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
	gameData.missiles.xNextFireTime = gameData.time.xGame + WI_fire_wait (i);
	return;
	}

if (gameStates.app.bPlayerIsDead || (playerP->secondaryAmmo [gameData.weapons.nSecondary] <= 0))
	return;

nWeaponId = secondaryWeaponToWeaponInfo [gameData.weapons.nSecondary];
if ((nWeaponId == PROXMINE_ID) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0) &&
	 (CountPlayerObjects (gameData.multiplayer.nLocalPlayer, OBJ_WEAPON, PROXMINE_ID) >= extraGameInfo [IsMultiGame].nMaxSmokeGrenades))
	return;
if (gameStates.app.cheats.bLaserRapidFire != 0xBADA55)
	gameData.missiles.xNextFireTime = gameData.time.xGame + WI_fire_wait (nWeaponId);
else
	gameData.missiles.xNextFireTime = gameData.time.xGame + F1_0/25;
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
	nObject = LaserPlayerFire (gameData.objs.console, nWeaponId, nGun, 1, 0, -1);
	if (gameData.weapons.nSecondary == PROXMINE_INDEX) {
		if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
			if (++nProximityDropped == 4) {
				nProximityDropped = 0;
				MaybeDropNetPowerup (nObject, POW_PROXMINE, INIT_DROP);
				}
			}
		break; //no dual prox bomb drop
		}
	else if (gameData.weapons.nSecondary == SMARTMINE_INDEX) {
		if (!(gameData.app.nGameMode & GM_ENTROPY)) {
			if (++nSmartminesDropped == 4) {
				nSmartminesDropped = 0;
				MaybeDropNetPowerup (nObject, POW_SMARTMINE, INIT_DROP);
				}
			}
		break; //no dual smartmine drop
		}
	else if (gameData.weapons.nSecondary != CONCUSSION_INDEX)
		MaybeDropNetPowerup (nObject, secondaryWeaponToPowerup [gameData.weapons.nSecondary], INIT_DROP);
	if ((gameData.weapons.nSecondary == GUIDED_INDEX) || (gameData.weapons.nSecondary == SMART_INDEX))
		break;
	else if ((gameData.weapons.nSecondary == MEGA_INDEX) || (gameData.weapons.nSecondary == EARTHSHAKER_INDEX)) {
		vmsVector vForce;

	vForce[X] = - (gameData.objs.console->position.mOrient[FVEC][X] << 7);
	vForce[Y] = - (gameData.objs.console->position.mOrient[FVEC][Y] << 7);
	vForce[Z] = - (gameData.objs.console->position.mOrient[FVEC][Z] << 7);
	PhysApplyForce (gameData.objs.console, &vForce);
	vForce[X] = (vForce[X] >> 4) + d_rand () - 16384;
	vForce[Y] = (vForce[Y] >> 4) + d_rand () - 16384;
	vForce[Z] = (vForce[Z] >> 4) + d_rand () - 16384;
	PhysApplyRot (gameData.objs.console, &vForce);
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
	int			nWeapon, nObject, nGun, h, i, j;
	vmsVector	*vGunPoints, vGunPos;
	vmsMatrix	*viewP;
	tObject		*objP;

gameData.objs.trackGoals [0] =
gameData.objs.trackGoals [1] = NULL;
if ((objP = GuidedInMainView ())) {
	nObject = FindHomingObject (&objP->position.vPos, objP);
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
viewP = ObjectView (gameData.objs.console);
for (i = 0; i < j; i++, h = !h) {
	nGun = secondaryWeaponToGunNum [gameData.weapons.nSecondary] + h;
	if ((vGunPoints = GetGunPoints (gameData.objs.console, nGun))) {
		vGunPos = vGunPoints [nGun];
		vGunPos = *viewP * vGunPos;
		vGunPos += gameData.objs.console->position.vPos;
		nObject = FindHomingObject (&vGunPos, gameData.objs.console);
		gameData.objs.trackGoals [i] = (nObject < 0) ? NULL : OBJECTS + nObject;
		}
	}
}

//	-------------------------------------------------------------------------------------------
// eof
