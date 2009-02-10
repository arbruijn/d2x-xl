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
#include "gameseg.h"
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
#include "gameseg.h"
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
#ifdef TACTILE
#	include "tactile.h"
#endif

//------------------------------------------------------------------------------

void CObject::Initialize (ubyte nType, ubyte nId, short nCreator, short nSegment, const CFixVector& vPos,
								  const CFixMatrix& mOrient, fix xSize, ubyte cType, ubyte mType, ubyte rType)
{
SetSignature (gameData.objs.nNextSignature++);
SetType (nType);
SetId (nId);
SetLastPos (vPos);
SetSize (xSize);
SetCreator ((sbyte) nCreator);
SetOrient (&mOrient);
SetControlType (cType);
SetMovementType (mType);
SetRenderType (rType);
SetContainsType (-1);
SetLifeLeft (
	 ((gameData.app.nGameMode & GM_ENTROPY) &&  (nType == OBJ_POWERUP) &&  (nId == POW_HOARD_ORB) &&  (extraGameInfo [1].entropy.nVirusLifespan > 0)) ?
		I2X (extraGameInfo [1].entropy.nVirusLifespan) : IMMORTAL_TIME);
SetAttachedObj (-1);
SetShields (I2X (20));
SetSegment (-1);					//set to zero by memset, above
LinkToSeg (nSegment);
}

//-----------------------------------------------------------------------------

int CObject::Create (ubyte nType, ubyte nId, short nCreator, short nSegment, 
							const CFixVector& vPos, const CFixMatrix& mOrient,
							fix xSize, ubyte cType, ubyte mType, ubyte rType)
{
#if DBG
if (nType == OBJ_WEAPON) {
	nType = nType;
	if ((nCreator >= 0) && (OBJECTS [nCreator].info.nType == OBJ_ROBOT))
		nType = nType;
	if (nId == FLARE_ID)
		nType = nType;
	if (gameData.objs.bIsMissile [(int) nId])
		nType = nType;
	}
else if (nType == OBJ_ROBOT) {
#if 0
	if (ROBOTINFO ((int) nId).bossFlag && (BOSS_COUNT >= MAX_BOSS_COUNT))
		return -1;
#endif
	}
else if (nType == OBJ_HOSTAGE)
	nType = nType;
else if (nType == OBJ_FIREBALL)
	nType = nType;
else if (nType == OBJ_REACTOR)
	nType = nType;
else if (nType == OBJ_DEBRIS)
	nType = nType;
else if (nType == OBJ_MARKER)
	nType = nType;
else if (nType == OBJ_PLAYER)
	nType = nType;
else if (nType == OBJ_POWERUP)
	nType = nType;
#endif

SetSegment (FindSegByPos (vPos, nSegment, 1, 0));
if ((Segment () < 0) || (Segment () > gameData.segs.nLastSegment))
	return -1;

if (nType == OBJ_DEBRIS) {
	if (gameData.objs.nDebris >= gameStates.render.detail.nMaxDebrisObjects)
		return -1;
	}

// Zero out object structure to keep weird bugs from happening in uninitialized fields.
m_nId = OBJ_IDX (this);
SetSignature (gameData.objs.nNextSignature++);
SetType (nType);
SetId (nId);
SetLastPos (vPos);
SetPos (&vPos);
SetSize (xSize);
SetCreator ((sbyte) nCreator);
SetOrient (&mOrient);
SetControlType (cType);
SetMovementType (mType);
SetRenderType (rType);
SetContainsType (-1);
SetLifeLeft (
	((gameData.app.nGameMode & GM_ENTROPY) && (nType == OBJ_POWERUP) && (nId == POW_HOARD_ORB) && (extraGameInfo [1].entropy.nVirusLifespan > 0)) ?
	I2X (extraGameInfo [1].entropy.nVirusLifespan) : IMMORTAL_TIME);
SetAttachedObj (-1);
m_xCreationTime = gameData.time.xGame;
#if 0
if (GetControlType () == CT_POWERUP)
	CPowerupInfo::SetCount (1);
// Init physics info for this CObject
if (GetMovementType () == MT_PHYSICS)
	m_vStartVel.SetZero ();
if (GetRenderType () == RT_POLYOBJ)
	CPolyObjInfo::SetTexOverride (-1);

if (GetType () == OBJ_WEAPON) {
	CPhysicsInfo::SetFlags (CPhysInfo.GetFlags () | WI_persistent (m_info.nId) * PF_PERSISTENT);
	CLaserInfo::SetCreationTime (gameData.time.xGame);
	CLaserInfo::SetLastHitObj (0);
	CLaserInfo::SetScale (I2X (1));
	}
else if (GetType () == OBJ_DEBRIS)
	gameData.objs.nDebris++;
if (GetControlType () == CT_POWERUP)
	CPowerupInfo::SetCreationTime (gameData.time.xGame);
else if (GetControlType () == CT_EXPLOSION) {
	CAttachedInfo::SetPrev (-1);
	CAttachedInfo::SetNext (-1);
	CAttachedInfo::SetParent (-1);
	}
#endif
Link ();
LinkToSeg (nSegment);
return m_nId;
}

//-----------------------------------------------------------------------------
//initialize a new CObject.  adds to the list for the given CSegment
//note that nSegment is really just a suggestion, since this routine actually
//searches for the correct CSegment
//returns the CObject number

int CreateObject (ubyte nType, ubyte nId, short nCreator, short nSegment, const CFixVector& vPos, const CFixMatrix& mOrient,
						fix xSize, ubyte cType, ubyte mType, ubyte rType)
{
	short		nObject;
	CObject	*objP;

#if DBG
if (nType == OBJ_WEAPON) {
	nType = nType;
	if ((nCreator >= 0) && (OBJECTS [nCreator].info.nType == OBJ_ROBOT))
		nType = nType;
	if (nId == FLARE_ID)
		nType = nType;
	if (gameData.objs.bIsMissile [(int) nId])
		nType = nType;
	}
else if (nType == OBJ_ROBOT) {
#if 0
	if (ROBOTINFO ((int) nId).bossFlag && (BOSS_COUNT >= MAX_BOSS_COUNT))
		return -1;
#endif
	}
else if (nType == OBJ_HOSTAGE)
	nType = nType;
else if (nType == OBJ_FIREBALL)
	nType = nType;
else if (nType == OBJ_REACTOR)
	nType = nType;
else if (nType == OBJ_DEBRIS)
	nType = nType;
else if (nType == OBJ_MARKER)
	nType = nType;
else if (nType == OBJ_PLAYER)
	nType = nType;
else if (nType == OBJ_POWERUP)
	nType = nType;
#endif

//if (GetSegMasks (vPos, nSegment, 0).m_center))
if (nSegment < -1)
	nSegment = -nSegment - 2;
else
	nSegment = FindSegByPos (vPos, nSegment, 1, 0);
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment))
	return -1;

if (nType == OBJ_DEBRIS) {
	if (gameData.objs.nDebris >= gameStates.render.detail.nMaxDebrisObjects)
		return -1;
	}

// Find next free object
if (0 > (nObject = AllocObject ()))
	return -1;
objP = OBJECTS + nObject;
objP->SetId (nObject);
// Zero out object structure to keep weird bugs from happening in uninitialized fields.
objP->info.nSignature = gameData.objs.nNextSignature++;
objP->info.nType = nType;
objP->info.nId = nId;
objP->info.vLastPos = 
objP->info.position.vPos = vPos;
objP->info.xSize = xSize;
objP->info.nCreator = (sbyte) nCreator;
objP->info.position.mOrient = mOrient;
objP->info.controlType = cType;
objP->info.movementType = mType;
objP->info.renderType = rType;
objP->info.contains.nType = -1;
objP->info.xLifeLeft = 
	((gameData.app.nGameMode & GM_ENTROPY) && (nType == OBJ_POWERUP) && (nId == POW_HOARD_ORB) && (extraGameInfo [1].entropy.nVirusLifespan > 0)) ?
	I2X (extraGameInfo [1].entropy.nVirusLifespan) : IMMORTAL_TIME;
objP->info.nAttachedObj = -1;
if (objP->info.controlType == CT_POWERUP)
	objP->cType.powerupInfo.nCount = 1;

// Init physics info for this CObject
if (objP->info.movementType == MT_PHYSICS)
	objP->SetStartVel ((CFixVector*) &CFixVector::ZERO);
if (objP->info.renderType == RT_POLYOBJ)
	objP->rType.polyObjInfo.nTexOverride = -1;
objP->SetCreationTime (gameData.time.xGame);

if (objP->info.nType == OBJ_WEAPON) {
	Assert (objP->info.controlType == CT_WEAPON);
	objP->mType.physInfo.flags |= WI_persistent (objP->info.nId) * PF_PERSISTENT;
	objP->cType.laserInfo.xCreationTime = gameData.time.xGame;
	objP->cType.laserInfo.nLastHitObj = 0;
	objP->cType.laserInfo.xScale = I2X (1);
	}
else if (objP->info.nType == OBJ_DEBRIS)
	gameData.objs.nDebris++;
if (objP->info.controlType == CT_POWERUP)
	objP->cType.powerupInfo.xCreationTime = gameData.time.xGame;
else if (objP->info.controlType == CT_EXPLOSION)
	objP->cType.explInfo.attached.nNext =
	objP->cType.explInfo.attached.nPrev =
	objP->cType.explInfo.attached.nParent = -1;

objP->Link ();
objP->LinkToSeg (nSegment);
return nObject;
}

//------------------------------------------------------------------------------

int CloneObject (CObject *objP)
{
	short		nObject, nSegment;
	int		nSignature;
	CObject	*cloneP;
	
if (0 > (nObject = AllocObject ()))
	return -1;
cloneP = OBJECTS + nObject;
nSignature = cloneP->info.nSignature;
memcpy (cloneP, objP, sizeof (CObject));
cloneP->info.nSignature = nSignature;
cloneP->info.nCreator = -1;
cloneP->mType.physInfo.thrust.SetZero ();
cloneP->SetCreationTime (gameData.time.xGame);
nSegment = objP->info.nSegment;
cloneP->info.nSegment = 
cloneP->info.nPrevInSeg = 
cloneP->info.nNextInSeg = -1;
cloneP->InitLinks ();
cloneP->SetLinkedType (OBJ_NONE);
objP->Link ();
objP->LinkToSeg (nSegment);
return nObject;
}

//------------------------------------------------------------------------------

int CreateRobot (ubyte nId, short nSegment, const CFixVector& vPos)
{
return CreateObject (OBJ_ROBOT, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, gameData.models.polyModels [0][ROBOTINFO (nId).nModel].Rad (), 
							CT_AI, MT_PHYSICS, RT_POLYOBJ);
}

//------------------------------------------------------------------------------

int CreatePowerup (ubyte nId, short nCreator, short nSegment, const CFixVector& vPos, int bIgnoreLimits)
{
if (!bIgnoreLimits && TooManyPowerups ((int) nId)) {
#if 0//def _DEBUG
	HUDInitMessage ("%c%c%c%cDiscarding excess %s!", 1, 127 + 128, 64 + 128, 128, pszPowerup [nId]);
	TooManyPowerups (nId);
#endif
	return -2;
	}
short nObject = CreateObject (OBJ_POWERUP, nId, nCreator, nSegment, vPos, CFixMatrix::IDENTITY, gameData.objs.pwrUp.info [nId].size, 
										CT_POWERUP, MT_PHYSICS, RT_POWERUP);
if ((nObject >= 0) && IsMultiGame && PowerupClass (nId)) {
	gameData.multiplayer.powerupsInMine [(int) nId]++;
	if (MultiPowerupIs4Pack (nId))
		gameData.multiplayer.powerupsInMine [(int) nId - 1] += 4;
	}
return nObject;
}

//------------------------------------------------------------------------------

int CreateWeapon (ubyte nId, short nCreator, short nSegment, const CFixVector& vPos, fix xSize, ubyte rType)
{
if (rType == 255) {
	switch (gameData.weapons.info [nId].renderType) {
		case WEAPON_RENDER_BLOB:
			rType = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
			xSize = gameData.weapons.info [nId].blob_size;
			break;
		case WEAPON_RENDER_POLYMODEL:
			xSize = 0;	//	Filled in below.
			rType = RT_POLYOBJ;
			break;
		case WEAPON_RENDER_NONE:
			rType = RT_NONE;
			xSize = I2X (1);
			break;
		case WEAPON_RENDER_VCLIP:
			rType = RT_WEAPON_VCLIP;
			xSize = gameData.weapons.info [nId].blob_size;
			break;
		default:
			Error ("Invalid weapon render nType in CreateNewWeapon\n");
			return -1;
		}
	}
return CreateObject (OBJ_WEAPON, nId, nCreator, nSegment, vPos, CFixMatrix::IDENTITY, xSize, CT_WEAPON, MT_PHYSICS, rType); 
}

//------------------------------------------------------------------------------

int CreateFireball (ubyte nId, short nSegment, const CFixVector& vPos, fix xSize, ubyte rType)
{
return CreateObject (OBJ_FIREBALL, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, xSize, CT_EXPLOSION, MT_NONE, rType);
}

//------------------------------------------------------------------------------

int CreateDebris (CObject *parentP, short nSubModel)
{
return CreateObject (OBJ_DEBRIS, 0, -1, parentP->info.nSegment, parentP->info.position.vPos, parentP->info.position.mOrient,
							gameData.models.polyModels [0][parentP->rType.polyObjInfo.nModel].SubModels ().rads [nSubModel],
							CT_DEBRIS, MT_PHYSICS, RT_POLYOBJ);
}

//------------------------------------------------------------------------------

int CreateCamera (CObject *parentP)
{
return CreateObject (OBJ_CAMERA, 0, -1, parentP->info.nSegment, parentP->info.position.vPos, parentP->info.position.mOrient, 0, 
							CT_NONE, MT_NONE, RT_NONE);
}

//------------------------------------------------------------------------------

int CreateLight (ubyte nId, short nSegment, const CFixVector& vPos)
{
return CreateObject (OBJ_LIGHT, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, 0, CT_LIGHT, MT_NONE, RT_NONE);
}

//------------------------------------------------------------------------------

void CreateSmallFireballOnObject (CObject *objP, fix size_scale, int bSound)
{
	fix			size;
	CFixVector	vPos, vRand;
	short			nSegment;

vPos = objP->info.position.vPos;
vRand = CFixVector::Random();
vRand *= (objP->info.xSize / 2);
vPos += vRand;
size = FixMul (size_scale, I2X (1) / 2 + d_rand () * 4 / 2);
nSegment = FindSegByPos (vPos, objP->info.nSegment, 1, 0);
if (nSegment != -1) {
	CObject *explObjP = /*Object*/CreateExplosion (nSegment, vPos, size, VCLIP_SMALL_EXPLOSION);
	if (!explObjP)
		return;
	AttachObject (objP, explObjP);
	if (d_rand () < 8192) {
		fix	vol = I2X (1) / 2;
		if (objP->info.nType == OBJ_ROBOT)
			vol *= 2;
		else if (bSound)
			audio.CreateObjectSound (SOUND_EXPLODING_WALL, SOUNDCLASS_EXPLOSION, objP->Index (), 0, vol);
		}
	}
}

//------------------------------------------------------------------------------


void CreateVClipOnObject (CObject *objP, fix xScale, ubyte nVClip)
{
	fix			xSize;
	CFixVector	vPos, vRand;
	short			nSegment;

vPos = objP->info.position.vPos;
vRand = CFixVector::Random();
vRand *= (objP->info.xSize / 2);
vPos += vRand;
xSize = FixMul (xScale, I2X (1) + d_rand ()*4);
nSegment = FindSegByPos (vPos, objP->info.nSegment, 1, 0);
if (nSegment != -1) {
	CObject *explObjP = /*Object*/CreateExplosion (nSegment, vPos, xSize, nVClip);
	if (!explObjP)
		return;

	explObjP->info.movementType = MT_PHYSICS;
	explObjP->mType.physInfo.velocity [X] = objP->mType.physInfo.velocity [X] / 2;
	explObjP->mType.physInfo.velocity [Y] = objP->mType.physInfo.velocity [Y] / 2;
	explObjP->mType.physInfo.velocity [Z] = objP->mType.physInfo.velocity [Z] / 2;
	}
}

//------------------------------------------------------------------------------
//creates a marker CObject in the world.  returns the CObject number
int DropMarkerObject (CFixVector& vPos, short nSegment, CFixMatrix& orient, ubyte nMarker)
{
	short nObject;

Assert (gameData.models.nMarkerModel != -1);
nObject = CreateObject (OBJ_MARKER, nMarker, -1, nSegment, vPos, orient,
								gameData.models.polyModels [0][gameData.models.nMarkerModel].Rad (), CT_NONE, MT_NONE, RT_POLYOBJ);
if (nObject >= 0) {
	CObject *objP = OBJECTS + nObject;
	objP->rType.polyObjInfo.nModel = gameData.models.nMarkerModel;
	objP->mType.spinRate = objP->info.position.mOrient.UVec () * (I2X (1) / 2);
	//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all OBJECTS.
	objP->info.xLifeLeft = IMMORTAL_TIME;
	}
return nObject;
}

//------------------------------------------------------------------------------

//remove CObject from the world
void ReleaseObject (short nObject)
{
if (!nObject)
	return;

	int nParent;
	CObject *objP = OBJECTS + nObject;

if (objP->info.nType == OBJ_WEAPON) {
	RespawnDestroyedWeapon (nObject);
	if (objP->info.nId == GUIDEDMSL_ID) {
		nParent = OBJECTS [objP->cType.laserInfo.parent.nObject].info.nId;
		if (nParent != gameData.multiplayer.nLocalPlayer)
			gameData.objs.guidedMissile [nParent].objP = NULL;
		else if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordGuidedEnd ();
		}
	}
if (objP == gameData.objs.viewerP)		//deleting the viewerP?
	gameData.objs.viewerP = gameData.objs.consoleP;						//..make the CPlayerData the viewerP
if (objP->info.nFlags & OF_ATTACHED)		//detach this from CObject
	DetachFromParent (objP);
if (objP->info.nAttachedObj != -1)		//detach all OBJECTS from this
	DetachChildObjects (objP);
if (objP->info.nType == OBJ_DEBRIS)
	gameData.objs.nDebris--;
OBJECTS [nObject].UnlinkFromSeg ();
Assert (OBJECTS [0].info.nNextInSeg != 0);
if ((objP->info.nType == OBJ_ROBOT) || 
	 (objP->info.nType == OBJ_REACTOR) || 
	 (objP->info.nType == OBJ_POWERUP) || 
	 (objP->info.nType == OBJ_HOSTAGE))
	ExecObjTriggers (nObject, 0);
objP->info.nType = OBJ_NONE;		//unused!
objP->info.nSignature = -1;
objP->info.nSegment = -1;				// zero it!
FreeObject (nObject);
SpawnLeftoverPowerups (nObject);
}

//------------------------------------------------------------------------------
//eof
