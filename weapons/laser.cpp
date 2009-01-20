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
#include "lightcluster.h"

#ifdef TACTILE
#include "tactile.h"
#endif

void BlastNearbyGlass (CObject *objP, fix damage);
void NDRecordGuidedEnd (void);
void NDRecordGuidedStart (void);

//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!!*****
//	Here are the constants you're looking for!--MK

#define	FULL_COCKPIT_OFFS 0
#define	LASER_OFFS	 (I2X (29) / 100)

static   int nMslTurnSpeeds [3] = {4, 2, 1};

#define	HOMINGMSL_SCALE		nMslTurnSpeeds [(IsMultiGame && !IsCoopGame) ? 2 : extraGameInfo [IsMultiGame].nMslTurnSpeed]

#define	MAX_SMART_DISTANCE	I2X (150)
#define	MAX_OBJDISTS			30

//---------------------------------------------------------------------------------

inline int HomingMslScale (void)
{
return (int) (HOMINGMSL_SCALE * sqrt (gameStates.gameplay.slowmo [0].fSpeed));
}

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// CPlayerData and calls the appropriate routine.

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
		DrawObjectBlob (objP, gameData.weapons.info [objP->info.nId].bitmap.index, gameData.weapons.info [objP->info.nId].bitmap.index, 0, NULL, 2.0f / 3.0f);
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
	return gameData.time.xGame > xCreationTime + (I2X (1) / 3) * gameStates.gameplay.slowmo [0].fSpeed;
else if (nId == GUIDEDMSL_ID)
	return gameData.time.xGame > xCreationTime + (I2X (1) / 2) * gameStates.gameplay.slowmo [0].fSpeed;
else if (WeaponIsPlayerMine (nId))
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
if (WeaponIsMine (id1) || WeaponIsMine (id2))
	return 0;
if (!COMPETITION && EGI_FLAG (bKillMissiles, 0, 0, 0) && (gameData.objs.bIsMissile [id1] || gameData.objs.bIsMissile [id2]))
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
gameData.muzzle.queueIndex++;
if (gameData.muzzle.queueIndex >= MUZZLE_QUEUE_MAX)
	gameData.muzzle.queueIndex = 0;
}

//---------------------------------------------------------------------------------
//creates a weapon CObject
int CreateWeaponObject (ubyte nWeaponType, short nSegment, CFixVector *vPosition, short nParent)
{
	int		rType = -1;
	fix		xLaserRadius = -1;
	int		nObject;
	CObject	*objP;

switch (gameData.weapons.info [nWeaponType].renderType) {
	case WEAPON_RENDER_BLOB:
		rType = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
		xLaserRadius = gameData.weapons.info [nWeaponType].blob_size;
		break;
	case WEAPON_RENDER_POLYMODEL:
		xLaserRadius = 0;	//	Filled in below.
		rType = RT_POLYOBJ;
		break;
	case WEAPON_RENDER_LASER:	// Not supported anymore
	case WEAPON_RENDER_NONE:
		rType = RT_NONE;
		xLaserRadius = I2X (1);
		break;
	case WEAPON_RENDER_VCLIP:
		rType = RT_WEAPON_VCLIP;
		xLaserRadius = gameData.weapons.info [nWeaponType].blob_size;
		break;
	default:
		Error ("Invalid weapon render nType in CreateNewWeapon\n");
		return -1;
	}

Assert (xLaserRadius != -1);
Assert (rType != -1);
nObject = CreateWeapon (nWeaponType, nParent, nSegment, *vPosition, 0, 255);
objP = OBJECTS + nObject;
if (gameData.weapons.info [nWeaponType].renderType == WEAPON_RENDER_POLYMODEL) {
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [objP->info.nId].nModel;
	objP->info.xSize = FixDiv (gameData.models.polyModels [0][objP->rType.polyObjInfo.nModel].Rad (),
								gameData.weapons.info [objP->info.nId].poLenToWidthRatio);
	}
else if (EGI_FLAG (bTracers, 0, 1, 0) && (objP->info.nId == VULCAN_ID) || (objP->info.nId == GAUSS_ID)) {
	objP->rType.polyObjInfo.nModel = gameData.weapons.info [SUPERLASER_ID + 1].nModel;
	objP->rType.polyObjInfo.nTexOverride = -1;
	objP->rType.polyObjInfo.nAltTextures = 0;
	objP->info.xSize = FixDiv (gameData.models.polyModels [0][objP->rType.polyObjInfo.nModel].Rad (),
								gameData.weapons.info [SUPERLASER_ID].poLenToWidthRatio);
	objP->info.renderType = RT_POLYOBJ;
	}
objP->mType.physInfo.mass = WI_mass (nWeaponType);
objP->mType.physInfo.drag = WI_drag (nWeaponType);
objP->mType.physInfo.thrust.SetZero ();
if (gameData.weapons.info [nWeaponType].bounce == 1)
	objP->mType.physInfo.flags |= PF_BOUNCE;
if ((gameData.weapons.info [nWeaponType].bounce == 2) || gameStates.app.cheats.bBouncingWeapons)
	objP->mType.physInfo.flags |= PF_BOUNCE + PF_BOUNCES_TWICE;
return nObject;
}

// ---------------------------------------------------------------------------------

// Initializes a laser after Fire is pressed
//	Returns CObject number.
int CreateNewWeapon (CFixVector *vDirection, CFixVector *vPosition, short nSegment,
						  short nParent, ubyte nWeaponType, int bMakeSound)
{
	int			nObject, nViewer, bBigMsl;
	CObject		*objP, *parentP = (nParent < 0) ? NULL : OBJECTS + nParent;
	CWeaponInfo	*weaponInfoP;
	fix			xParentSpeed, xWeaponSpeed;
	fix			volume;
	fix			xLaserLength = 0;

	static int	nMslSounds [2] = {SND_ADDON_MISSILE_SMALL, SND_ADDON_MISSILE_BIG};
	static int	nGatlingSounds [2] = {SND_ADDON_VULCAN, SND_ADDON_GAUSS};

Assert (nWeaponType < gameData.weapons.nTypes [0]);
if (nWeaponType >= gameData.weapons.nTypes [0])
	nWeaponType = 0;
//	Don't let homing blobs make muzzle flash.
if (parentP->info.nType == OBJ_ROBOT)
	DoMuzzleStuff (nSegment, vPosition);
else if (gameStates.app.bD2XLevel &&
			(parentP == gameData.objs.consoleP) &&
			(SEGMENTS [gameData.objs.consoleP->info.nSegment].m_nType == SEGMENT_IS_NODAMAGE))
	return -1;
#if 1
if ((nParent == LOCALPLAYER.nObject) &&
	 (nWeaponType == PROXMINE_ID) &&
	 (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
	nObject = CreatePowerup (POW_HOARD_ORB, -1, nSegment, *vPosition, 1);
	if (nObject >= 0) {
		objP = OBJECTS + nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->info.nCreator = GetTeam (gameData.multiplayer.nLocalPlayer) + 1;
		}
	return -1;
	}
#endif
nObject = CreateWeaponObject (nWeaponType, nSegment, vPosition, nParent);
if (nObject < 0)
	return -1;
#if 0
if (parentP == gameData.objs.consoleP) {
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
	parentP->SetTracers ((parentP->Tracers () + 1) % 3);
objP = OBJECTS + nObject;
objP->cType.laserInfo.parent.nObject = nParent;
objP->cType.laserInfo.parent.nType = parentP->info.nType;
objP->cType.laserInfo.parent.nSignature = parentP->info.nSignature;
//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
//	not apply to the Omega Cannon.
nViewer = OBJ_IDX (gameData.objs.viewerP);
weaponInfoP = gameData.weapons.info + nWeaponType;
if (nWeaponType == OMEGA_ID) {
	// Create orientation matrix for tracking purposes.
	int bSpectator = SPECTATOR (parentP);
	objP->info.position.mOrient = CFixMatrix::CreateFU(*vDirection, bSpectator ? gameStates.app.playerPos.mOrient.UVec () : parentP->info.position.mOrient.UVec ());
//	objP->info.position.mOrient = CFixMatrix::CreateFU(*vDirection, bSpectator ? &gameStates.app.playerPos.mOrient.UVec () : &parentP->info.position.mOrient.UVec (), NULL);
	if (((nParent != nViewer) || bSpectator) && (parentP->info.nType != OBJ_WEAPON)) {
		// Muzzle flash
		if (weaponInfoP->nFlashVClip > -1)
			CreateMuzzleFlash (objP->info.nSegment, objP->info.position.vPos, weaponInfoP->xFlashSize, weaponInfoP->nFlashVClip);
		}
	DoOmegaStuff (OBJECTS + nParent, vPosition, objP);
	return nObject;
	}
else if (nWeaponType == FUSION_ID) {
	static int nRotDir = 0;
	nRotDir = !nRotDir;
	objP->mType.physInfo.rotVel[X] =
	objP->mType.physInfo.rotVel[Y] = 0;
	objP->mType.physInfo.rotVel[Z] = nRotDir ? -I2X (1) : I2X (1);
	}
if (parentP->info.nType == OBJ_PLAYER) {
	if (nWeaponType == FUSION_ID) {
		if (gameData.fusion.xCharge <= 0)
			objP->cType.laserInfo.xScale = I2X (1);
		else if (gameData.fusion.xCharge <= I2X (4))
			objP->cType.laserInfo.xScale = I2X (1) + gameData.fusion.xCharge / 2;
		else
			objP->cType.laserInfo.xScale = I2X (4);
		}
	else if (/* (nWeaponType >= LASER_ID) &&*/ (nWeaponType <= MAX_SUPER_LASER_LEVEL) &&
				(gameData.multiplayer.players [parentP->info.nId].flags & PLAYER_FLAGS_QUAD_LASERS))
		objP->cType.laserInfo.xScale = I2X (3) / 4;
	else if (nWeaponType == GUIDEDMSL_ID) {
		if (nParent == LOCALPLAYER.nObject) {
			gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP = objP;
			gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature = objP->info.nSignature;
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
	objP->mType.physInfo.flags |= PF_BOUNCE;
if (weaponInfoP->renderType == WEAPON_RENDER_POLYMODEL)
	xLaserLength = gameData.models.polyModels [0][objP->rType.polyObjInfo.nModel].Rad () * 2;
if (nWeaponType == FLARE_ID)
	objP->mType.physInfo.flags |= PF_STICK;		//this obj sticks to walls
objP->info.xShields = WI_strength (nWeaponType, gameStates.app.nDifficultyLevel);
// Fill in laser-specific data
objP->info.xLifeLeft							= WI_lifetime (nWeaponType);
objP->cType.laserInfo.parent.nType	= parentP->info.nType;
objP->cType.laserInfo.parent.nSignature = parentP->info.nSignature;
objP->cType.laserInfo.parent.nObject	= nParent;
//	Assign nParent nType to highest level creator.  This propagates nParent nType down from
//	the original creator through weapons which create children of their own (ie, smart missile)
if (parentP->info.nType == OBJ_WEAPON) {
	int	nHighestParent = nParent;
	int	count = 0;
	CObject	*parentObjP = OBJECTS + nHighestParent;

	while ((count++ < 10) && (parentObjP->info.nType == OBJ_WEAPON)) {
		int nNextParent = parentObjP->cType.laserInfo.parent.nObject;
		if (OBJECTS [nNextParent].info.nSignature != parentObjP->cType.laserInfo.parent.nSignature)
			break;	//	Probably means nParent was killed.  Just continue.
		if (nNextParent == nHighestParent) {
			Int3 ();	//	Hmm, CObject is nParent of itself.  This would seem to be bad, no?
			break;
			}
		nHighestParent = nNextParent;
		parentObjP = OBJECTS + nHighestParent;
		objP->cType.laserInfo.parent.nObject = nHighestParent;
		objP->cType.laserInfo.parent.nType = parentObjP->info.nType;
		objP->cType.laserInfo.parent.nSignature = parentObjP->info.nSignature;
		}
	}
// Create orientation matrix so we can look from this pov
//	Homing missiles also need an orientation matrix so they know if they can make a turn.
//if ((objP->info.renderType == RT_POLYOBJ) || (WI_homingFlag (objP->info.nId)))
	objP->info.position.mOrient = CFixMatrix::CreateFU (*vDirection, parentP->info.position.mOrient.UVec ());
//	objP->info.position.mOrient = CFixMatrix::CreateFU (*vDirection, &parentP->info.position.mOrient.UVec (), NULL);
if (((nParent != nViewer) || SPECTATOR (parentP)) && (parentP->info.nType != OBJ_WEAPON)) {
	// Muzzle flash
	if (weaponInfoP->nFlashVClip > -1)
		CreateMuzzleFlash (objP->info.nSegment, objP->info.position.vPos, weaponInfoP->xFlashSize, weaponInfoP->nFlashVClip);
	}
volume = I2X (1);
if (bMakeSound && (weaponInfoP->flashSound > -1)) {
	int bGatling = (nWeaponType == VULCAN_ID) || (nWeaponType == GAUSS_ID);
	if (nParent != nViewer) {
		if (bGatling && (parentP->info.nType == OBJ_PLAYER) && (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling)
			audio.CreateSegmentSound (weaponInfoP->flashSound, objP->info.nSegment, 0, objP->info.position.vPos, 0, volume, I2X (256),
											  AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
		else
			audio.CreateSegmentSound (weaponInfoP->flashSound, objP->info.nSegment, 0, objP->info.position.vPos, 0, volume);
		}
	else {
		if (nWeaponType == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
			volume = I2X (1) / 2;
		if (bGatling && (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling)
			audio.PlaySound (-1, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER, volume, DEFAULT_PAN, 0, -1, 
								  AddonSoundName (nGatlingSounds [nWeaponType == GAUSS_ID]));
		else
			audio.PlaySound (weaponInfoP->flashSound, (nParent == nViewer) ? SOUNDCLASS_PLAYER : SOUNDCLASS_LASER, volume);
		}
	if (gameOpts->sound.bMissiles && gameData.objs.bIsMissile [nWeaponType]) {
		bBigMsl = (nWeaponType == SMARTMSL_ID) ||
					 (nWeaponType == MEGAMSL_ID) ||
					 (nWeaponType == EARTHSHAKER_ID) ||
					 (nWeaponType == ROBOT_SMARTMSL_ID) ||
					 (nWeaponType == ROBOT_MEGAMSL_ID) ||
					 (nWeaponType == ROBOT_EARTHSHAKER_ID);
		audio.CreateObjectSound (-1, SOUNDCLASS_MISSILE, nObject, 1, I2X (gameOpts->sound.xCustomSoundVolume) / 10, I2X (256), -1, -1, 
										 AddonSoundName (nMslSounds [bBigMsl]), 1);
		}
	else if (nWeaponType == FLARE_ID)
		audio.CreateObjectSound (nObject, SOUNDCLASS_GENERIC, -1, 0, I2X (1), I2X (256), -1, -1, AddonSoundName (SND_ADDON_FLARE));
	}
//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
// This also jitters the laser a bit so that it doesn't alias.
//	Don't do for weapons created by weapons.
if ((parentP->info.nType == OBJ_PLAYER) && (gameData.weapons.info [nWeaponType].renderType != WEAPON_RENDER_NONE) && (nWeaponType != FLARE_ID)) {
	CFixVector	vEndPos;
	int			nEndSeg;

	vEndPos = objP->info.position.vPos + *vDirection * (gameData.laser.nOffset + (xLaserLength / 2));
	nEndSeg = FindSegByPos (vEndPos, objP->info.nSegment, 1, 0);
	if (nEndSeg == objP->info.nSegment)
		objP->info.position.vPos = vEndPos;
	else if (nEndSeg != -1) {
		objP->info.position.vPos = vEndPos;
		objP->RelinkToSeg (nEndSeg);
		}
	}

//	Here's where to fix the problem with OBJECTS which are moving backwards imparting higher velocity to their weaponfire.
//	Find out if moving backwards.
if (!WeaponIsMine (nWeaponType))
	xParentSpeed = 0;
else {
	xParentSpeed = parentP->mType.physInfo.velocity.Mag();
	if (CFixVector::Dot (parentP->mType.physInfo.velocity, parentP->info.position.mOrient.FVec ()) < 0)
		xParentSpeed = -xParentSpeed;
	}

xWeaponSpeed = WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel);
if (weaponInfoP->speedvar != 128) {
	fix randval = I2X (1) - ((d_rand () * weaponInfoP->speedvar) >> 6);	//	Get a scale factor between speedvar% and 1.0.
	xWeaponSpeed = FixMul (xWeaponSpeed, randval);
	}
//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
if ((objP->info.nId == SMARTMSL_BLOB_ID) ||
	 (objP->info.nId == SMARTMINE_BLOB_ID) ||
	 (objP->info.nId == ROBOT_SMARTMSL_BLOB_ID) ||
	 (objP->info.nId == ROBOT_SMARTMINE_BLOB_ID) ||
	 (objP->info.nId == EARTHSHAKER_MEGA_ID))
	xWeaponSpeed /= 4;
if (WIThrust (objP->info.nId) != 0)
	xWeaponSpeed /= 2;
/*test*/objP->mType.physInfo.velocity = *vDirection * (xWeaponSpeed + xParentSpeed);
if (parentP)
	objP->SetStartVel (&parentP->mType.physInfo.velocity);
//	Set thrust
if (WIThrust (nWeaponType) != 0) {
	objP->mType.physInfo.thrust = objP->mType.physInfo.velocity;
	objP->mType.physInfo.thrust *= FixDiv (WIThrust (objP->info.nId), xWeaponSpeed + xParentSpeed);
	}
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID))
	objP->info.xLifeLeft += (d_rand () - 16384) << 2;		//	add in -2..2 seconds
return nObject;
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls CreateNewWeapon, but takes care of the CSegment and point computation for you.
int CreateNewLaserEasy (CFixVector * vDirection, CFixVector * vPosition, short parent, ubyte nWeaponType, int bMakeSound)
{
	tFVIQuery	fq;
	tFVIData		hitData;
	CObject		*parentObjP = OBJECTS + parent;
	int			fate;

	//	Find CSegment containing laser fire vPosition.  If the robot is straddling a CSegment, the vPosition from
	//	which it fires may be in a different CSegment, which is bad news for FindVectorIntersection.  So, cast
	//	a ray from the CObject center (whose CSegment we know) to the laser vPosition.  Then, in the call to CreateNewWeapon
	//	use the data returned from this call to FindVectorIntersection.
	//	Note that while FindVectorIntersection is pretty slow, it is not terribly slow if the destination point is
	//	in the same CSegment as the source point.

fq.p0					= &parentObjP->info.position.vPos;
fq.startSeg			= parentObjP->info.nSegment;
fq.p1					= vPosition;
fq.radP0				=
fq.radP1				= 0;
fq.thisObjNum		= OBJ_IDX (parentObjP);
fq.ignoreObjList	= NULL;
fq.flags				= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

fate = FindVectorIntersection (&fq, &hitData);
if (fate != HIT_NONE  || hitData.hit.nSegment==-1)
	return -1;
return CreateNewWeapon (vDirection, &hitData.hit.vPoint, (short) hitData.hit.nSegment, parent, nWeaponType, bMakeSound);
}

//	-----------------------------------------------------------------------------------------------------------

CFixVector *GetGunPoints (CObject *objP, int nGun)
{
if (!objP)
	return NULL;

	tGunInfo		*giP = gameData.models.gunInfo + objP->rType.polyObjInfo.nModel;
	CFixVector	*vDefaultGunPoints, *vGunPoints;
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
	int			bSpectate = SPECTATOR (objP);
	tObjTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;
	CFixMatrix	m, *viewP;
	CFixVector	v [2];
#if FULL_COCKPIT_OFFS
	int			bLaserOffs = ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) &&
									  (objP->Index () == LOCALPLAYER.nObject));
#else
	int			bLaserOffs = 0;
#endif

if (nGun < 0) {	// use center between gunPoints nGun and nGun + 1
	(*v) = vGunPoints[-nGun] + vGunPoints[-nGun-1];
//	VmVecScale (VmVecAdd (v, vGunPoints - nGun, vGunPoints - nGun - 1), I2X (1) / 2);
	(*v) *= (I2X (1) / 2);
}
else {
	v [0] = vGunPoints [nGun];
	if (bLaserOffs)
		v[0] += posP->mOrient.UVec () * LASER_OFFS;
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
memcpy (mP, &posP->mOrient, sizeof (CFixMatrix));
if (nGun < 0)
	v[1] += (*mP).UVec () * (-2 * v->Mag());
(*vMuzzle) = posP->vPos + v[1];
//	If supposed to fire at a delayed time (xDelay), then move this point backwards.
if (xDelay)
	*vMuzzle += (*mP).FVec ()* (-FixMul (xDelay, WI_speed (nLaserType, gameStates.app.nDifficultyLevel)));
return vMuzzle;
}

//-------------- Initializes a laser after Fire is pressed -----------------

int LaserPlayerFireSpreadDelay (
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
	short			nLaserSeg;
	int			nFate;
	CFixVector	vLaserPos, vLaserDir, *vGunPoints;
	tFVIQuery	fq;
	tFVIData		hitData;
	int			nObject;
	CObject		*laserP;
#if FULL_COCKPIT_OFFS
	int bLaserOffs = ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) &&
							(objP->Index () == LOCALPLAYER.nObject));
#else
	int bLaserOffs = 0;
#endif
	CFixMatrix			m;
	int					bSpectate = SPECTATOR (objP);
	tObjTransformation	*posP = bSpectate ? &gameStates.app.playerPos : &objP->info.position;

#if DBG
if (nLaserType == SMARTMINE_BLOB_ID)
	nLaserType = nLaserType;
#endif
CreateAwarenessEvent (objP, PA_WEAPON_WALL_COLLISION);
// Find the initial vPosition of the laser
if (!(vGunPoints = GetGunPoints (objP, nGun)))
	return 0;
#if 1
TransformGunPoint (objP, vGunPoints, nGun, xDelay, nLaserType, &vLaserPos, &m);
#else
if (nGun < 0)	// use center between gunPoints nGun and nGun + 1
	VmVecScale (VmVecAdd (&v, vGunPoints - nGun, vGunPoints - nGun - 1), I2X (1) / 2);
else {
	v = vGunPoints [nGun];
	if (bLaserOffs)
		VmVecScaleInc (&v, &posP->mOrient.UVec (), LASER_OFFS);
	}
if (bSpectate)
   VmCopyTransposeMatrix (viewP = &m, &posP->mOrient);
else
   viewP = ObjectView (objP);
VmVecRotate (&vGunPoint, &v, viewP);
memcpy (&m, &posP->mOrient, sizeof (CFixMatrix));
if (nGun < 0)
	VmVecScaleInc (&vGunPoint, &m.UVec (), -2 * VmVecMag (&v));
VmVecAdd (&vLaserPos, &posP->vPos, &vGunPoint);
//	If supposed to fire at a delayed time (xDelay), then move this point backwards.
if (xDelay)
	VmVecScaleInc (&vLaserPos, &m.FVec (), -FixMul (xDelay, WI_speed (nLaserType, gameStates.app.nDifficultyLevel)));
#endif

//	DoMuzzleStuff (objP, &Pos);

//--------------- Find vLaserPos and nLaserSeg ------------------
fq.p0					= &posP->vPos;
fq.startSeg			= bSpectate ? gameStates.app.nPlayerSegment : objP->info.nSegment;
fq.p1					= &vLaserPos;
fq.radP0				=
fq.radP1				= 0x10;
fq.thisObjNum		= objP->Index ();
fq.ignoreObjList	= NULL;
fq.flags				= FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS;
nFate = FindVectorIntersection (&fq, &hitData);
nLaserSeg = hitData.hit.nSegment;
if (nLaserSeg == -1) {	//some sort of annoying error
	return -1;
	}
//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
if (CFixVector::Dist(vLaserPos, posP->vPos) > 3 * objP->info.xSize / 2) {
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
		OBJECTS [hitData.hitObject].Die ();
	if (OBJECTS [hitData.hitObject].nType != OBJ_POWERUP)
		return;
	}
#endif
//	Now, make laser spread out.
vLaserDir = m.FVec ();
if (xSpreadR || xSpreadU) {
	vLaserDir += m.RVec () * xSpreadR;
	vLaserDir += m.UVec () * xSpreadU;
	}
if (bLaserOffs)
	vLaserDir += m.UVec () * LASER_OFFS;
nObject = CreateNewWeapon (&vLaserDir, &vLaserPos, nLaserSeg, objP->Index (), nLaserType, bMakeSound);
//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
if (nLaserType == OMEGA_ID)
	return -1;
if (nObject == -1) {
	return -1;
	}
if ((nLaserType == GUIDEDMSL_ID) && gameData.multigame.bIsGuided)
	gameData.objs.guidedMissile [objP->info.nId].objP = OBJECTS + nObject;
gameData.multigame.bIsGuided = 0;
laserP = OBJECTS + nObject;
if (gameData.objs.bIsMissile [nLaserType] && (nLaserType != GUIDEDMSL_ID)) {
	if (!gameData.objs.missileViewerP && (objP->info.nId == gameData.multiplayer.nLocalPlayer))
		gameData.objs.missileViewerP = laserP;
	}
//	If this weapon is supposed to be silent, set that bit!
if (!bMakeSound)
	laserP->info.nFlags |= OF_SILENT;
//	If this weapon is supposed to be silent, set that bit!
if (bHarmless)
	laserP->info.nFlags |= OF_HARMLESS;

//	If the object firing the laser is the CPlayerData, then indicate the laser object so robots can dodge.
//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
if ((objP == gameData.objs.consoleP) && !WeaponIsPlayerMine (laserP->info.nId))
	gameStates.app.bPlayerFiredLaserThisFrame = nObject;

if (gameStates.app.cheats.bHomingWeapons || gameData.weapons.info [nLaserType].homingFlag) {
	if (objP == gameData.objs.consoleP) {
		laserP->cType.laserInfo.nHomingTarget = FindHomingObject (&vLaserPos, laserP);
		gameData.multigame.laser.nTrack = laserP->cType.laserInfo.nHomingTarget;
		}
	else {// Some other CPlayerData shot the homing thing
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

frameTime = gameStates.limitFPS.bHomers ? SECS2X (gameStates.app.tick40fps.nTime) : gameData.time.xFrame;
vNewDir = *vNormVel;
vNewDir *= ((fix) (frameTime * 16 / gameStates.gameplay.slowmo [0].fSpeed));
vNewDir += objP->info.position.mOrient.FVec ();
CFixVector::Normalize(vNewDir);
objP->info.position.mOrient = CFixMatrix::CreateF(vNewDir);
}

//-------------------------------------------------------------------------------------------

static inline fix HomingMslStraightTime (int id)
{
if (!gameData.objs.bIsMissile [id])
	return HOMINGMSL_STRAIGHT_TIME;
if ((id == EARTHSHAKER_MEGA_ID) || (id == ROBOT_SHAKER_MEGA_ID))
	return HOMINGMSL_STRAIGHT_TIME;
return HOMINGMSL_STRAIGHT_TIME * nMslSlowDown [(int) extraGameInfo [IsMultiGame].nMslStartSpeed];
}

//-------------------------------------------------------------------------------------------
//sequence this laser CObject for this _frame_ (underscores added here to aid MK in his searching!)
void LaserDoWeaponSequence (CObject *objP)
{
	CObject	*gmObjP;
	fix		xWeaponSpeed, xScaleFactor, xDistToPlayer;

Assert (objP->info.controlType == CT_WEAPON);
//	Ok, this is a big hack by MK.
//	If you want an CObject to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME
if (objP->info.xLifeLeft == ONE_FRAME_TIME) {
	if (IsMultiGame)
		objP->info.xLifeLeft = OMEGA_MULTI_LIFELEFT;
	else
		objP->info.xLifeLeft = 0;
	objP->info.renderType = RT_NONE;
	}
if (objP->info.xLifeLeft < 0) {		// We died of old age
	objP->Die ();
	if (WI_damage_radius (objP->info.nId))
		objP->ExplodeBadassWeapon (objP->info.position.vPos);
	return;
	}
//delete weapons that are not moving
xWeaponSpeed = objP->mType.physInfo.velocity.Mag();
if (!((gameData.app.nFrameCount ^ objP->info.nSignature) & 3) &&
		(objP->info.nType == OBJ_WEAPON) && (objP->info.nId != FLARE_ID) &&
		(gameData.weapons.info [objP->info.nId].speed [gameStates.app.nDifficultyLevel] > 0) &&
		(xWeaponSpeed < I2X (2))) {
	ReleaseObject (objP->Index ());
	return;
	}
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FUSION_ID)) {		//always set fusion weapon to max vel
	CFixVector::Normalize(objP->mType.physInfo.velocity);
	objP->mType.physInfo.velocity *= (WI_speed (objP->info.nId,gameStates.app.nDifficultyLevel));
	}
//	For homing missiles, turn towards target. (unless it's the guided missile)
if ((gameOpts->legacy.bHomers || !gameStates.limitFPS.bHomers || gameStates.app.tick40fps.bTick) &&
	 (objP->info.nType == OBJ_WEAPON) &&
    (gameStates.app.cheats.bHomingWeapons || WI_homingFlag (objP->info.nId)) &&
	 !(objP->info.nFlags & PF_HAS_BOUNCED) &&
	 !((objP->info.nId == GUIDEDMSL_ID) &&
	   (objP == (gmObjP = gameData.objs.guidedMissile [OBJECTS [objP->cType.laserInfo.parent.nObject].info.nId].objP)) &&
	   (objP->info.nSignature == gmObjP->info.nSignature))) {
	CFixVector	vVecToObject, vTemp;
	fix			dot = I2X (1);
	fix			speed, xMaxSpeed;
	int			nObjId = objP->info.nId;
	//	For first 1/2 second of life, missile flies straight.
	if (objP->cType.laserInfo.xCreationTime + HomingMslStraightTime (nObjId) < gameData.time.xGame) {
		int	nHomingTarget = objP->cType.laserInfo.nHomingTarget;

		//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
		if ((nObjId == ROBOT_SMARTMINE_BLOB_ID) ||
			 (nObjId == ROBOT_SMARTMSL_BLOB_ID) ||
			 (nObjId == SMARTMINE_BLOB_ID) ||
			 (nObjId == SMARTMSL_BLOB_ID) ||
			 (nObjId == EARTHSHAKER_MEGA_ID))
			objP->mType.physInfo.flags &= ~PF_BOUNCE;

		//	Make sure the CObject we are tracking is still trackable.
		nHomingTarget = TrackHomingTarget (nHomingTarget, objP, &dot);
		if (nHomingTarget != -1) {
			if (nHomingTarget == LOCALPLAYER.nObject) {
				xDistToPlayer = CFixVector::Dist(objP->info.position.vPos, OBJECTS [nHomingTarget].info.position.vPos);
				if ((xDistToPlayer < LOCALPLAYER.homingObjectDist) || (LOCALPLAYER.homingObjectDist < 0))
					LOCALPLAYER.homingObjectDist = xDistToPlayer;
				}
			vVecToObject = OBJECTS [nHomingTarget].info.position.vPos - objP->info.position.vPos;
			CFixVector::Normalize (vVecToObject);
			vTemp = objP->mType.physInfo.velocity;
			speed = CFixVector::Normalize (vTemp);
			xMaxSpeed = WI_speed (objP->info.nId,gameStates.app.nDifficultyLevel);
			if (speed + I2X (1) < xMaxSpeed) {
				speed += FixMul (xMaxSpeed, gameData.time.xFrame / 2);
				if (speed > xMaxSpeed)
					speed = xMaxSpeed;
				}
			if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (objP->info.nId == EARTHSHAKER_MEGA_ID)) {
				fix h = (objP->info.xLifeLeft + I2X (1) - 1) / I2X (1);

				if (h > 7)
					vVecToObject *= (I2X (1) / (h - 6));
				}
			// -- dot = CFixVector::Dot (vTemp, vVecToObject);
			vVecToObject *= (I2X (1) / HomingMslScale ());
			vTemp += vVecToObject;
			//	The boss' smart children track better...
			if (gameData.weapons.info [objP->info.nId].renderType != WEAPON_RENDER_POLYMODEL)
				vTemp += vVecToObject;
			CFixVector::Normalize(vTemp);
			objP->mType.physInfo.velocity = vTemp;
			objP->mType.physInfo.velocity *= speed;

			//	Subtract off life proportional to amount turned.
			//	For hardest turn, it will lose 2 seconds per second.
			 {
				fix	absdot = abs (I2X (1) - dot);
            fix	lifelost = FixMul (absdot*32, gameData.time.xFrame);
				objP->info.xLifeLeft -= lifelost;
				}

			//	Only polygon OBJECTS have visible orientation, so only they should turn.
			if (gameData.weapons.info [objP->info.nId].renderType == WEAPON_RENDER_POLYMODEL)
				HomingMissileTurnTowardsVelocity (objP, &vTemp);		//	vTemp is normalized velocity.
			}
		}
	}
	//	Make sure weapon is not moving faster than allowed speed.
if ((objP->info.nType == OBJ_WEAPON) &&
	 (xWeaponSpeed > WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel))) {
	//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
	if (WI_speed (objP->info.nId, gameStates.app.nDifficultyLevel)) {
		xScaleFactor = FixDiv (WI_speed (objP->info.nId,gameStates.app.nDifficultyLevel), xWeaponSpeed);
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
	gameData.laser.xNextFireTime = gameData.time.xGame + I2X (1) / 25;
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
// Assumption: This is only called by the actual console CPlayerData, not for network players

int LocalPlayerFireLaser (void)
{
	CPlayerData	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
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
if (gameStates.app.bD2XLevel && (SEGMENTS [OBJECTS [playerP->nObject].info.nSegment].m_nType == SEGMENT_IS_NODAMAGE))
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
if (addval > I2X (1))
	addval = I2X (1);
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
	AutoSelectWeapon (0, 1);		//	Make sure the CPlayerData can fire from this weapon.

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
		AutoSelectWeapon (0, 1);		//	Make sure the CPlayerData can fire from this weapon.
		}
	else {
		AutoSelectWeapon (0, 1);		//	Make sure the CPlayerData can fire from this weapon.
		StopPrimaryFire ();
		break;	//	Couldn't fire weapon, so abort.
		}
	}
gameData.laser.nGlobalFiringCount = 0;
return rVal;
}

// -- #define	MAX_LIGHTNING_DISTANCE	 (I2X (30)0)
// -- #define	MAX_LIGHTNING_BLOBS		16
// -- #define	LIGHTNING_BLOB_DISTANCE	 (MAX_LIGHTNING_DISTANCE/MAX_LIGHTNING_BLOBS)
// --
// -- #define	LIGHTNING_BLOB_ID			13
// --
// -- #define	LIGHTNING_TIME		 (I2X (1)/4)
// -- #define	LIGHTNING_DELAY	 (I2X (1)/8)
// --
// -- int	Lightning_gun_num = 1;
// --
// -- fix	Lightning_startTime = -I2X (10), Lightning_lastTime;
// --
// -- //	--------------------------------------------------------------------------------------------------
// -- //	Return -1 if failed to create at least one blob.  Else return index of last blob created.
// -- int create_lightning_blobs (CFixVector *vDirection, CFixVector *start_pos, int start_segnum, int parent)
// -- {
// -- 	int			i;
// -- 	tFVIQuery	fq;
// -- 	tFVIData		hitData;
// -- 	CFixVector	vEndPos;
// -- 	CFixVector	norm_dir;
// -- 	int			fate;
// -- 	int			num_blobs;
// -- 	CFixVector	tvec;
// -- 	fix			dist_to_hit_point;
// -- 	CFixVector	point_pos, delta_pos;
// -- 	int			nObject;
// -- 	CFixVector	*gun_pos;
// -- 	CFixMatrix	m;
// -- 	CFixVector	gun_pos2;
// --
// -- 	if (LOCALPLAYER.energy > I2X (1))
// -- 		LOCALPLAYER.energy -= I2X (1);
// --
// -- 	if (LOCALPLAYER.energy <= I2X (1)) {
// -- 		LOCALPLAYER.energy = 0;
// -- 		AutoSelectWeapon (0);
// -- 		return -1;
// -- 	}
// --
// -- 	norm_dir = *vDirection;
// --
// -- 	CFixVector::Normalize(&norm_dir);
// -- 	VmVecScaleAdd (&vEndPos, start_pos, &norm_dir, MAX_LIGHTNING_DISTANCE);
// --
// -- 	fq.p0						= start_pos;
// -- 	fq.startSeg				= start_segnum;
// -- 	fq.p1						= &vEndPos;
// -- 	fq.Rad ()					= 0;
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
// -- 	VmCopyTransposeMatrix (&m,&OBJECTS [parent].info.position.mOrient);
// -- 	VmVecRotate (&gun_pos2, gun_pos, &m);
// -- 	VmVecAdd (&point_pos, &OBJECTS [parent].info.position.vPos, &gun_pos2);
// --
// -- 	delta_pos = norm_dir;
// -- 	VmVecScale (&delta_pos, dist_to_hit_point/num_blobs);
// --
// -- 	for (i=0; i<num_blobs; i++) {
// -- 		int			tPointSeg;
// -- 		CObject		*obj;
// --
// -- 		VmVecInc (&point_pos, &delta_pos);
// -- 		tPointSeg = FindSegByPos (&point_pos, start_segnum, 1, 0);
// -- 		if (tPointSeg == -1)	//	Hey, we thought we were creating points on a line, but we left the mine!
// -- 			continue;
// --
// -- 		nObject = CreateNewWeapon (vDirection, &point_pos, tPointSeg, parent, LIGHTNING_BLOB_ID, 0);
// --
// -- 		if (nObject < 0)  {
// -- 			Int3 ();
// -- 			return -1;
// -- 		}
// --
// -- 		obj = OBJECTS + nObject;
// --
// -- 		audio.PlaySound (gameData.weapons.info [objP->info.nId].flashSound);
// --
// -- 		// -- VmVecScale (&objP->mType.physInfo.velocity, I2X (1)/2);
// --
// -- 		objP->info.xLifeLeft = (LIGHTNING_TIME + LIGHTNING_DELAY)/2;
// --
// -- 	}
// --
// -- 	return nObject;
// --
// -- }
// --
// -- //	--------------------------------------------------------------------------------------------------
// -- //	Lightning Cannon.
// -- //	While being fired, creates path of blobs forward from CPlayerData until it hits something.
// -- //	Up to MAX_LIGHTNING_BLOBS blobs, spaced LIGHTNING_BLOB_DISTANCE units apart.
// -- //	When the CPlayerData releases the firing key, the blobs move forward.
// -- void lightning_frame (void)
// -- {
// -- 	if ((gameData.time.xGame - Lightning_startTime < LIGHTNING_TIME) && (gameData.time.xGame - Lightning_startTime > 0)) {
// -- 		if (gameData.time.xGame - Lightning_lastTime > LIGHTNING_DELAY) {
// -- 			create_lightning_blobs (&gameData.objs.consoleP->info.position.mOrient.FVec (), &gameData.objs.consoleP->info.position.vPos, gameData.objs.consoleP->info.nSegment, OBJ_IDX (gameData.objs.consoleP));
// -- 			Lightning_lastTime = gameData.time.xGame;
// -- 		}
// -- 	}
// -- }

//	--------------------------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int ( __fastcall * pWeaponHandler) (CObject *, int, int, int);
#else
typedef int (* pWeaponHandler) (CObject *, int, int, int);
#endif

//-------------------------------------------

int LaserHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	ubyte	nLaser = (nLevel <= MAX_LASER_LEVEL) ? LASER_ID + nLevel : SUPERLASER_ID + (nLevel - MAX_LASER_LEVEL - 1);
	short	nLightObj = lightClusterManager.Create (objP);

gameData.laser.nOffset = (I2X (2) * (d_rand () % 8)) / 8;
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

int VulcanHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
#	define VULCAN_SPREAD	(d_rand ()/8 - 32767/16)

	int bGatlingSound = gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling;

if (bGatlingSound && (gameData.weapons.firing [objP->info.nId].nDuration <= GATLING_DELAY))
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

int SpreadfireHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	short	nLightObj = lightClusterManager.Create (objP);

if (nFlags & LASER_SPREADFIRE_TOGGLED) {
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, I2X (1)/16, 0, 0, 0, nLightObj);
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, -I2X (1)/16, 0, 0, 0, nLightObj);
	}
else {
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, I2X (1)/16, 0, 0, nLightObj);
	LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, -I2X (1)/16, 0, 0, nLightObj);
	}
LaserPlayerFireSpread (objP, SPREADFIRE_ID, 6, 0, 0, 1, 0, nLightObj);
return nRoundsPerShot;
}

//-------------------------------------------

int PlasmaHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	short	nLightObj = lightClusterManager.Create (objP);

LaserPlayerFire (objP, PLASMA_ID, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, PLASMA_ID, 1, 0, 0, nLightObj);
if (nRoundsPerShot > 1) {
	nLightObj = lightClusterManager.Create (objP);

	LaserPlayerFireSpreadDelay (objP, PLASMA_ID, 0, 0, 0, gameData.time.xFrame / 2, 1, 0, nLightObj);
	LaserPlayerFireSpreadDelay (objP, PLASMA_ID, 1, 0, 0, gameData.time.xFrame / 2, 0, 0, nLightObj);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int FusionHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	CFixVector	vForce;
	short			nLightObj = lightClusterManager.Create (objP);

LaserPlayerFire (objP, FUSION_ID, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, FUSION_ID, 1, 1, 0, nLightObj);
if (EGI_FLAG (bTripleFusion, 0, 0, 0) && gameData.multiplayer.weaponStates [objP->info.nId].bTripleFusion)
#if 1
	LaserPlayerFire (objP, FUSION_ID, 6, 1, 0, nLightObj);
#else
	LaserPlayerFire (objP, FUSION_ID, -1, 1, 0, nLightObj);
#endif
nFlags = (sbyte) (gameData.fusion.xCharge >> 12);
gameData.fusion.xCharge = 0;
vForce [X] = -(objP->info.position.mOrient.FVec ()[X] << 7);
vForce [Y] = -(objP->info.position.mOrient.FVec ()[Y] << 7);
vForce [Z] = -(objP->info.position.mOrient.FVec ()[Z] << 7);
objP->ApplyForce (vForce);
vForce [X] = (vForce [X] >> 4) + d_rand () - 16384;
vForce [Y] = (vForce [Y] >> 4) + d_rand () - 16384;
vForce [Z] = (vForce [Z] >> 4) + d_rand () - 16384;
objP->ApplyRotForce (vForce);
return nRoundsPerShot;
}

//-------------------------------------------

int SuperlaserHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	ubyte nSuperLevel = 3;		//make some new kind of laser eventually
	short	nLightObj = lightClusterManager.Create (objP);

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

int GaussHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
#	define GAUSS_SPREAD		(VULCAN_SPREAD / 5)

	int			bGatlingSound = gameStates.app.bHaveExtraGameInfo [IsMultiGame] &&
										 (gameOpts->sound.bHires == 2) && gameOpts->sound.bGatling;
	tFiringData *fP = gameData.multiplayer.weaponStates [objP->info.nId].firing;

if (bGatlingSound && (fP->nDuration <= GATLING_DELAY))
	return 0;
//	Only make sound for 1/4 of vulcan bullets.
LaserPlayerFireSpread (objP, GAUSS_ID, 6, GAUSS_SPREAD, GAUSS_SPREAD,
							  (objP->info.nId != gameData.multiplayer.nLocalPlayer) || (gameData.laser.xNextFireTime > gameData.time.xGame), 0, -1);
if (nRoundsPerShot > 1) {
	LaserPlayerFireSpread (objP, GAUSS_ID, 6, GAUSS_SPREAD, GAUSS_SPREAD, 0, 0, -1);
	if (nRoundsPerShot > 2)
		LaserPlayerFireSpread (objP, GAUSS_ID, 6, GAUSS_SPREAD, GAUSS_SPREAD, 0, 0, -1);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int HelixHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	typedef struct tSpread {
		fix	r, u;
		} tSpread;

	static tSpread spreadTable [8] = {
	 {I2X (1) / 16, 0},
	 {I2X (1) / 17, I2X (1) / 42},
	 {I2X (1) / 22, I2X (1) / 22},
	 {I2X (1) / 42, I2X (1) / 17},
	 {0, I2X (1) / 16},
	 {-I2X (1) / 42, I2X (1) / 17},
	 {-I2X (1) / 22, I2X (1) / 22},
	 {-I2X (1) / 17, I2X (1) / 42}
		};

	tSpread	spread = spreadTable [(nFlags >> LASER_HELIX_SHIFT) & LASER_HELIX_MASK];
	short		nLightObj = lightClusterManager.Create (objP);

LaserPlayerFireSpread (objP, HELIX_ID, 6,  0,  0, 1, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6,  spread.r,  spread.u, 0, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6, -spread.r, -spread.u, 0, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6,  spread.r * 2,  spread.u * 2, 0, 0, nLightObj);
LaserPlayerFireSpread (objP, HELIX_ID, 6, -spread.r * 2, -spread.u * 2, 0, 0, nLightObj);
return nRoundsPerShot;
}

//-------------------------------------------

int PhoenixHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
{
	short	nLightObj = lightClusterManager.Create (objP);

LaserPlayerFire (objP, PHOENIX_ID, 0, 1, 0, nLightObj);
LaserPlayerFire (objP, PHOENIX_ID, 1, 0, 0, nLightObj);
if (nRoundsPerShot > 1) {
	nLightObj = lightClusterManager.Create (objP);
	LaserPlayerFireSpreadDelay (objP, PHOENIX_ID, 0, 0, 0, gameData.time.xFrame / 2, 1, 0, nLightObj);
	LaserPlayerFireSpreadDelay (objP, PHOENIX_ID, 1, 0, 0, gameData.time.xFrame / 2, 0, 0, nLightObj);
	}
return nRoundsPerShot;
}

//-------------------------------------------

int OmegaHandler (CObject *objP, int nLevel, int nFlags, int nRoundsPerShot)
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
//	Flags are the CPlayerData flags.  For network mode, set to 0.
//	It is assumed that this is a CPlayerData CObject (as in multiplayer), and therefore the gun positions are known.
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

if (nObjId == EARTHSHAKER_ID)
	BlastNearbyGlass (objP, gameData.weapons.info [EARTHSHAKER_ID].strength [gameStates.app.nDifficultyLevel]);

if ((nObjType == OBJ_WEAPON) &&
		((nObjId == SMARTMSL_ID) || (nObjId == SMARTMINE_ID) || (nObjId == ROBOT_SMARTMINE_ID) || (nObjId == EARTHSHAKER_ID)) &&
		(gameData.weapons.info [nObjId].children == -1))
	return;

if ((nObjType != OBJ_ROBOT) && ((nObjType != OBJ_WEAPON) || (gameData.weapons.info [nObjId].children < 1)))
	return;

int		i, nObject;
CObject	*curObjP;

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
	Assert (nBlobId != -1);		//	Hmm, missing data in bitmaps.tbl.  Need "children=NN" parameter.
	}
else {
	Assert (nObjType == OBJ_ROBOT);
	nBlobId = ROBOT_SMARTMSL_BLOB_ID;
	}
bMakeSound = 1;
for (i = 0; i < nSmartChildren; i++) {
	short nTarget = nObjects ? objList [(d_rand () * nObjects) >> 15] : -1;
	CreateHomingWeapon (objP, nTarget, nBlobId, bMakeSound);
	bMakeSound = 0;
	}
}

//	-------------------------------------------------------------------------------------------

//give up control of the guided missile
void ReleaseGuidedMissile (int nPlayer)
{
if (nPlayer == gameData.multiplayer.nLocalPlayer) {
	if (!gameData.objs.guidedMissile [nPlayer].objP)
		return;
	gameData.objs.missileViewerP = gameData.objs.guidedMissile [nPlayer].objP;
	if (IsMultiGame)
	 	MultiSendGuidedInfo (gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP, 1);
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
	CObject	*gmObjP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
	CPlayerData	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;

Assert (gameData.weapons.nSecondary < MAX_SECONDARY_WEAPONS);
if (gmObjP && (gmObjP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature)) {
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
	nObject = LaserPlayerFire (gameData.objs.consoleP, nWeaponId, nGun, 1, 0, -1);
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
	if ((gameData.weapons.nSecondary == GUIDED_INDEX) || (gameData.weapons.nSecondary == SMART_INDEX))
		break;
	else if ((gameData.weapons.nSecondary == MEGA_INDEX) || (gameData.weapons.nSecondary == EARTHSHAKER_INDEX)) {
		CFixVector vForce;

	vForce [X] = - (gameData.objs.consoleP->info.position.mOrient.FVec ()[X] << 7);
	vForce [Y] = - (gameData.objs.consoleP->info.position.mOrient.FVec ()[Y] << 7);
	vForce [Z] = - (gameData.objs.consoleP->info.position.mOrient.FVec ()[Z] << 7);
	gameData.objs.consoleP->ApplyForce (vForce);
	vForce [X] = (vForce [X] >> 4) + d_rand () - 16384;
	vForce [Y] = (vForce [Y] >> 4) + d_rand () - 16384;
	vForce [Z] = (vForce [Z] >> 4) + d_rand () - 16384;
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
	nObject = FindHomingObject (&objP->info.position.vPos, objP);
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
viewP = ObjectView (gameData.objs.consoleP);
for (i = 0; i < j; i++, h = !h) {
	nGun = secondaryWeaponToGunNum [gameData.weapons.nSecondary] + h;
	if ((vGunPoints = GetGunPoints (gameData.objs.consoleP, nGun))) {
		vGunPos = vGunPoints [nGun];
		vGunPos = *viewP * vGunPos;
		vGunPos += gameData.objs.consoleP->info.position.vPos;
		nObject = FindHomingObject (&vGunPos, gameData.objs.consoleP);
		gameData.objs.trackGoals [i] = (nObject < 0) ? NULL : OBJECTS + nObject;
		}
	}
}

//	-------------------------------------------------------------------------------------------
// eof
