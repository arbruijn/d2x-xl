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

int32_t MultiPowerupIs4Pack (int32_t);

#if DBG
int32_t nDbgPowerup = -1;
#endif

//------------------------------------------------------------------------------

void CObject::SetCreationTime (fix xCreationTime) 
{ 
m_xCreationTime = ((xCreationTime < 0) ? gameData.timeData.xGame : xCreationTime); 
}

//------------------------------------------------------------------------------

fix CObject::LifeTime (void) 
{ 
return gameData.timeData.xGame - m_xCreationTime; 
}

//------------------------------------------------------------------------------

void CObject::Initialize (uint8_t nType, uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos,
								  const CFixMatrix& mOrient, fix xSize, uint8_t cType, uint8_t mType, uint8_t rType)
{
SetSignature (gameData.objData.nNextSignature++);
SetType (nType);
SetKey (nId);
SetLastPos (vPos);
SetSize (xSize);
SetCreator ((int8_t) nCreator);
SetOrient (&mOrient);
SetControlType (cType);
SetMovementType (mType);
SetRenderType (rType);
SetContainsType (-1);
SetLifeLeft (
	 (IsEntropyGame &&  (nType == OBJ_POWERUP) &&  (nId == POW_HOARD_ORB) &&  (extraGameInfo [1].entropy.nVirusLifespan > 0)) ?
		I2X (extraGameInfo [1].entropy.nVirusLifespan) : IMMORTAL_TIME);
SetAttachedObj (-1);
SetShield (I2X (20));
SetSegment (-1);					//set to zero by memset, above
LinkToSeg (nSegment);
}

//-----------------------------------------------------------------------------

int32_t CObject::Create (uint8_t nType, uint8_t nId, int16_t nCreator, int16_t nSegment,
								 const CFixVector& vPos, const CFixMatrix& mOrient,
								 fix xSize, uint8_t cType, uint8_t mType, uint8_t rType)
{
ENTER (0, 0);
#if DBG
if (nType == OBJ_WEAPON) {
	nType = nType;
	if ((nCreator >= 0) && (OBJECT (nCreator)->info.nType == OBJ_ROBOT))
		nType = nType;
	if (nId == FLARE_ID)
		nType = nType;
	if (IsMissile ((int32_t) nId))
		nType = nType;
	}
else if (nType == OBJ_ROBOT) {
#if 0
	if (ROBOTINFO ((int32_t) nId) && ROBOTINFO ((int32_t) nId)->bossFlag && (BOSS_COUNT >= MAX_BOSS_COUNT))
		RETURN (-1)
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
if ((Segment () < 0) || (Segment () > gameData.segData.nLastSegment))
	RETURN (-1)

if (nType == OBJ_DEBRIS) {
	if (!gameOpts->render.effects.nShrapnels && (gameData.objData.nDebris >= gameStates.render.detail.nMaxDebrisObjects))
		RETURN (-1)
	}

// Zero out object structure to keep weird bugs from happening in uninitialized fields.
SetKey (OBJ_IDX (this));
SetSignature (gameData.objData.nNextSignature++);
SetType (nType);
SetKey (nId);
SetLastPos (vPos);
SetPos (&vPos);
SetSize (xSize);
SetCreator ((int8_t) nCreator);
SetOrient (&mOrient);
SetControlType (cType);
SetMovementType (mType);
SetRenderType (rType);
SetContainsType (-1);
SetLifeLeft (
	(IsEntropyGame && (nType == OBJ_POWERUP) && (nId == POW_HOARD_ORB) && (extraGameInfo [1].entropy.nVirusLifespan > 0)) ?
	I2X (extraGameInfo [1].entropy.nVirusLifespan) : IMMORTAL_TIME);
SetAttachedObj (-1);
m_xCreationTime = gameData.timeData.xGame;
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
	CLaserInfo::SetCreationTime (gameData.timeData.xGame);
	CLaserInfo::SetLastHitObj (0);
	CLaserInfo::SetScale (I2X (1));
	}
else if (GetType () == OBJ_DEBRIS)
	gameData.objData.nDebris++;
if (GetControlType () == CT_POWERUP)
	CPowerupInfo::SetCreationTime (gameData.timeData.xGame);
else if (GetControlType () == CT_EXPLOSION) {
	CAttachedInfo::SetPrev (-1);
	CAttachedInfo::SetNext (-1);
	CAttachedInfo::SetParent (-1);
	}
#endif
Link ();
LinkToSeg (nSegment);
RETURN (Key ())
}

//-----------------------------------------------------------------------------
//initialize a new CObject.  adds to the list for the given CSegment
//note that nSegment is really just a suggestion, since this routine actually
//searches for the correct CSegment
//returns the CObject number

void UpdateLastObject (int32_t nObject);

int32_t CreateObject (uint8_t nType, uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos, const CFixMatrix& mOrient, fix xSize, uint8_t cType, uint8_t mType, uint8_t rType)
{
ENTER (0, 0);
	int16_t	nObject;
	CObject	*pObj;

#if DBG
if (nType == OBJ_WEAPON) {
	BRP;
	if ((nCreator >= 0) && (OBJECT (nCreator)->info.nType == OBJ_ROBOT)) {
		BRP;
		if ((nDbgSeg >= 0) && (nSegment == nDbgSeg))
			BRP;
		}
	if (nId == FLARE_ID)
		BRP;
	if (nId == EARTHSHAKER_MEGA_ID)
		BRP;
	if (CObject::IsMissile ((int32_t) nId))
		BRP;
	else
		BRP;
	}
else if (nType == OBJ_ROBOT) {
	BRP;
#if 0
	if (ROBOTINFO ((int32_t) nId) && ROBOTINFO ((int32_t) nId)->bossFlag && (BOSS_COUNT >= MAX_BOSS_COUNT))
		RETURN (-1)
#endif
	}
else if (nType == OBJ_HOSTAGE)
	BRP;
else if (nType == OBJ_FIREBALL)
	BRP;
else if (nType == OBJ_REACTOR)
	BRP;
else if (nType == OBJ_DEBRIS)
	BRP;
else if (nType == OBJ_MARKER)
	BRP;
else if (nType == OBJ_PLAYER)
	BRP;
else if (nType == OBJ_POWERUP) {
	BRP;
	if (nId == POW_MONSTERBALL)
		BRP;
	if (nId == POW_VULCAN_AMMO) // unknown powerup type
		BRP;
	}
#endif

//if (GetSegMasks (vPos, nSegment, 0).m_center))
if (nSegment < -1)
	nSegment = -nSegment - 2;
else
	nSegment = FindSegByPos (vPos, nSegment, 1, 0);
if ((nSegment < 0) || (nSegment > gameData.segData.nLastSegment))
	RETURN (-1)

if (nType == OBJ_DEBRIS) {
	if (!gameOpts->render.effects.nShrapnels && (gameData.objData.nDebris >= gameStates.render.detail.nMaxDebrisObjects))
		RETURN (-1)
	}

// Find next free object
nObject = AllocObject ();
pObj = OBJECT (nObject);
if (!pObj) {
#if DBG
	if (nObject > -1)
		UpdateLastObject (0x7fffffff);
#endif
	RETURN (-1)
	}
pObj->SetKey (nObject);
pObj->SetCreationTime ();
// Zero out object structure to keep weird bugs from happening in uninitialized fields.
pObj->info.nSignature = gameData.objData.nNextSignature++;
pObj->info.nType = nType;
pObj->info.nId = nId;
pObj->info.vLastPos =
pObj->info.position.vPos = vPos;
pObj->SetOrigin (vPos);
pObj->SetSize (xSize);
pObj->info.nCreator = int8_t (nCreator);
pObj->SetLife (IMMORTAL_TIME);

CObject* pCreator = OBJECT (nCreator);

if (IsMultiGame && IsEntropyGame && (nType == OBJ_POWERUP) && (nId == POW_ENTROPY_VIRUS)) {
	if (pCreator && (pCreator->info.nType == OBJ_PLAYER))
		pObj->info.nCreator = int8_t (GetTeam (OBJECT (nCreator)->info.nId) + 1);
	if (extraGameInfo [1].entropy.nVirusLifespan > 0)
		pObj->SetLife (I2X (extraGameInfo [1].entropy.nVirusLifespan));
	}
pObj->info.position.mOrient = mOrient;
pObj->info.controlType = cType;
pObj->info.movementType = mType;
pObj->info.renderType = rType;
pObj->info.contains.nType = -1;
pObj->info.nAttachedObj = -1;
if (pObj->info.controlType == CT_POWERUP)
	pObj->cType.powerupInfo.nCount = 1;

// Init physics info for this CObject
if (pObj->info.movementType == MT_PHYSICS)
	pObj->SetStartVel ((CFixVector*) &CFixVector::ZERO);
if (pObj->info.renderType == RT_POLYOBJ)
	pObj->rType.polyObjInfo.nTexOverride = -1;
pObj->SetCreationTime (gameData.timeData.xGame);

if (pObj->info.nType == OBJ_WEAPON) {
	Assert (pObj->info.controlType == CT_WEAPON);
	pObj->mType.physInfo.flags |= WI_persistent (pObj->info.nId) * PF_PERSISTENT;
	pObj->cType.laserInfo.xCreationTime = gameData.timeData.xGame;
	pObj->cType.laserInfo.nLastHitObj = 0;
	pObj->cType.laserInfo.xScale = I2X (1);
	}
else if (pObj->info.nType == OBJ_DEBRIS)
	gameData.objData.nDebris++;
if (pObj->info.controlType == CT_POWERUP)
	pObj->cType.powerupInfo.xCreationTime = gameData.timeData.xGame;
else if (pObj->info.controlType == CT_EXPLOSION)
	pObj->cType.explInfo.attached.nNext =
	pObj->cType.explInfo.attached.nPrev =
	pObj->cType.explInfo.attached.nParent = -1;

pObj->Arm ();
pObj->ResetSgmLinks ();
#if OBJ_LIST_TYPE == 1
pObj->ResetLinks ();
pObj->Link ();
#endif
pObj->LinkToSeg (nSegment);
pObj->StopSync ();

memset (&pObj->HitInfo (), 0, sizeof (CObjHitInfo));
#if 1
if (IsMultiGame && IsCoopGame && 
	 (nType == OBJ_WEAPON) && CObject::IsMissile (int16_t (nId)) && pCreator && (pCreator->info.nType == OBJ_PLAYER)) {
	extern char powerupToObject [MAX_POWERUP_TYPES];

	for (int32_t i = 0; i < MAX_POWERUP_TYPES; i++) {
		if (powerupToObject [i] == nId)
			gameData.multiplayer.maxPowerupsAllowed [i]--;
		}
	}
#endif
pObj->ResetDamage ();
pObj->SetTarget (NULL);
RETURN (nObject)
}

//------------------------------------------------------------------------------

int32_t CloneObject (CObject *pObj)
{
ENTER (0, 0);
int16_t nObject = AllocObject ();
CObject *pClone = OBJECT (nObject);
if (!pClone)
	RETURN (-1)
int32_t nSignature = pClone->info.nSignature;
memcpy (pClone, pObj, sizeof (CObject));
pClone->info.nSignature = nSignature;
pClone->info.nCreator = -1;
pClone->mType.physInfo.thrust.SetZero ();
pClone->SetCreationTime (gameData.timeData.xGame);
int16_t nSegment = pObj->info.nSegment;
pClone->info.nSegment =
pClone->info.nPrevInSeg =
pClone->info.nNextInSeg = -1;
#if OBJ_LIST_TYPE == 1
pClone->InitLinks ();
pClone->SetLinkedType (OBJ_NONE);
#endif
pObj->Link ();
pObj->LinkToSeg (nSegment);
RETURN (nObject)
}

//------------------------------------------------------------------------------

int32_t CreateRobot (uint8_t nId, int16_t nSegment, const CFixVector& vPos)
{
ENTER (0, 0);
tRobotInfo* pRobotInfo = ROBOTINFO (nId);
if (!pRobotInfo) {
	PrintLog (0, "Trying to create non-existent robot (type %d)\n", nId);
	RETURN (-1)
	}
RETURN (CreateObject (OBJ_ROBOT, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, gameData.modelData.polyModels [0][pRobotInfo->nModel].Rad (), CT_AI, MT_PHYSICS, RT_POLYOBJ))
}

//------------------------------------------------------------------------------

int32_t PowerupsInMine (int32_t nPowerup)
{
ENTER (0, 0);
	int32_t nCount = 0;

if (MultiPowerupIs4Pack (nPowerup))
	nCount = PowerupsInMine (nPowerup - 1) / 4;
else {
#if DBG
	if (nPowerup == nDbgPowerup)
		BRP;
#endif
	nCount = gameData.multiplayer.powerupsInMine [nPowerup];
	if (gameStates.multi.nGameType == UDP_GAME)
		nCount += PowerupsOnShips (nPowerup);
	if (nPowerup == POW_VULCAN_AMMO) {
		int32_t	nAmmo = 0;
		CObject* pObj;
		FORALL_POWERUP_OBJS (pObj) {
			if ((pObj->Id () == POW_VULCAN) || (pObj->Id () == POW_GAUSS))
				nAmmo += pObj->cType.powerupInfo.nCount;
			}
		nCount += (nAmmo + VULCAN_CLIP_CAPACITY - 1) / VULCAN_CLIP_CAPACITY;
		}
	else if ((nPowerup == POW_PROXMINE) || (nPowerup == POW_SMARTMINE)) {
		int32_t nMines = 0;
		int32_t	nId = (nPowerup == POW_PROXMINE) ? PROXMINE_ID : SMARTMINE_ID;
		CObject* pObj;
		FORALL_WEAPON_OBJS (pObj) {
			if (pObj->Id () == nId)
				nMines++;
			}
		nCount += (nMines + 3) / 4;
		}
	}
RETURN (nCount)
}

//-----------------------------------------------------------------------------

void SetAllowedPowerup (int32_t nPowerup, uint32_t nCount)
{
ENTER (0, 0);
if (nCount && powerupFilter [nPowerup]) {
#if DBG
	if (nPowerup == nDbgPowerup)
		BRP;
#endif
	if (MultiPowerupIs4Pack (nPowerup))
		SetAllowedPowerup (nPowerup - 1, nCount * 4);
	else if (MultiPowerupIs4Pack (nPowerup + 1)) {
		gameData.multiplayer.maxPowerupsAllowed [nPowerup] = 4 * nCount;
		gameData.multiplayer.maxPowerupsAllowed [nPowerup + 1] = gameData.multiplayer.maxPowerupsAllowed [nPowerup - 1] / 4;
		}
	else {
		gameData.multiplayer.maxPowerupsAllowed [nPowerup] = nCount;
		if ((nPowerup == POW_VULCAN) || (nPowerup == POW_GAUSS))
			gameData.multiplayer.maxPowerupsAllowed [POW_VULCAN_AMMO] = 2 * nCount; // add vulcan ammo for each gatling type gun
		}
	}
LEAVE
}

//-----------------------------------------------------------------------------

void AddAllowedPowerup (int32_t nPowerup, uint32_t nCount)
{
ENTER (0, 0);
if (nCount && powerupFilter [nPowerup]) {
#if DBG
	if (nPowerup == nDbgPowerup)
		BRP;
#endif
	if (MultiPowerupIs4Pack (nPowerup)) 
		AddAllowedPowerup (nPowerup - 1, nCount * 4);
	else {
		gameData.multiplayer.maxPowerupsAllowed [nPowerup] += nCount;
		if ((nPowerup == POW_VULCAN) || (nPowerup == POW_GAUSS))
			gameData.multiplayer.maxPowerupsAllowed [POW_VULCAN_AMMO] += 2 * nCount;
		if (MultiPowerupIs4Pack (nPowerup + 1))
			gameData.multiplayer.maxPowerupsAllowed [nPowerup + 1] = gameData.multiplayer.maxPowerupsAllowed [nPowerup] / 4;
		}
	}
LEAVE
}

//-----------------------------------------------------------------------------

void RemoveAllowedPowerup (int32_t nPowerup, uint32_t nCount)
{
ENTER (0, 0);
if (nCount && powerupFilter [nPowerup]) {
#if DBG
	if (nPowerup == nDbgPowerup)
		BRP;
#endif
	uint16_t h = gameData.multiplayer.maxPowerupsAllowed [nPowerup];
	if (MultiPowerupIs4Pack (nPowerup))
		RemoveAllowedPowerup (nPowerup - 1, nCount * 4);
	else {
		gameData.multiplayer.maxPowerupsAllowed [nPowerup] -= nCount;
		if ((nPowerup == POW_VULCAN) || (nPowerup == POW_GAUSS))
			gameData.multiplayer.maxPowerupsAllowed [POW_VULCAN_AMMO] -= 2;
		if (gameData.multiplayer.maxPowerupsAllowed [nPowerup] > h) // overflow
			gameData.multiplayer.maxPowerupsAllowed [nPowerup] = 0;
		if (MultiPowerupIs4Pack (nPowerup + 1))
			gameData.multiplayer.maxPowerupsAllowed [nPowerup + 1] = gameData.multiplayer.maxPowerupsAllowed [nPowerup] / 4;
		}
	}
LEAVE
}

//-----------------------------------------------------------------------------

void AddPowerupInMine (int32_t nPowerup, uint32_t nCount, bool bIncreaseLimit)
{
ENTER (0, 0);
if (nCount && powerupFilter [nPowerup]) {
#if DBG
	if (nPowerup == nDbgPowerup)
		BRP;
#endif
	if (MultiPowerupIs4Pack (nPowerup))
		AddPowerupInMine (nPowerup - 1, nCount * 4, bIncreaseLimit);
	else {
		gameData.multiplayer.powerupsInMine [nPowerup] += nCount;
		if (MultiPowerupIs4Pack (nPowerup + 1))
			gameData.multiplayer.powerupsInMine [nPowerup + 1] = gameData.multiplayer.powerupsInMine [nPowerup] / 4;
		if (bIncreaseLimit)
			AddAllowedPowerup (nPowerup, nCount);
		}
	}
LEAVE
}

//-----------------------------------------------------------------------------

void RemovePowerupInMine (int32_t nPowerup, uint32_t nCount)
{
ENTER (0, 0);
if (nCount && powerupFilter [nPowerup]) {
#if DBG
	if (nPowerup == nDbgPowerup)
		BRP;
#endif
	if (MultiPowerupIs4Pack (nPowerup))
		RemovePowerupInMine (nPowerup - 1, nCount * 4);
	else {
		if (gameData.multiplayer.powerupsInMine [nPowerup] <= nCount)
			gameData.multiplayer.powerupsInMine [nPowerup] = 0;
		else 
			gameData.multiplayer.powerupsInMine [nPowerup] -= nCount;
		if (MultiPowerupIs4Pack (nPowerup + 1))
			gameData.multiplayer.powerupsInMine [nPowerup + 1] = gameData.multiplayer.powerupsInMine [nPowerup] / 4;
		}	
	}
LEAVE
}

//------------------------------------------------------------------------------

int32_t MissingPowerups (int32_t nPowerup, int32_t bBreakDown)
{
return powerupFilter [nPowerup] ? gameData.multiplayer.maxPowerupsAllowed [nPowerup] - PowerupsInMine (nPowerup) : 0;
}

//------------------------------------------------------------------------------

static inline int32_t TooManyPowerups (int32_t nPowerup)
{
#if DBG
if (nPowerup == nDbgPowerup)
	BRP;
#endif
if (!IsMultiGame)
	return 0;
if (!PowerupClass (nPowerup))
	return 0;
if (PowerupsInMine (nPowerup) < gameData.multiplayer.maxPowerupsAllowed [nPowerup])
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int32_t CreatePowerup (uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos, int32_t bIgnoreLimits, bool bForce)
{
ENTER (0, 0);
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	RETURN (-1)
if (nId >= MAX_POWERUP_TYPES) {
	PrintLog (0, "Trying to create non-existent powerup (type %d)\n", nId);
	RETURN (-1)
	}
if (!bIgnoreLimits && TooManyPowerups ((int32_t) nId)) {
#if 1 //DBG
	PrintLog (0, "Deleting excess powerup %d (in mine: %d, on ships: %d, max: %d)\n", 
				 nId, gameData.multiplayer.powerupsInMine [nId], PowerupsOnShips (nId), gameData.multiplayer.maxPowerupsAllowed [nId]);
#if DBG
	HUDInitMessage ("%c%c%c%cDiscarding excess %s!", 1, 127 + 128, 64 + 128, 128, pszPowerup [nId]);
#endif
	TooManyPowerups (nId);
#endif
	RETURN (-2)
	}
if (gameStates.gameplay.bMineMineCheat && !bForce && (CObject::IsEquipment (nId) < 2))
	RETURN (-1)
int16_t nObject = CreateObject (OBJ_POWERUP, nId, nCreator, nSegment, vPos, CFixMatrix::IDENTITY, gameData.objData.pwrUp.info [nId].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
if (nObject >= 0) {
	if (nId == POW_VULCAN_AMMO)
		OBJECT (nObject)->cType.powerupInfo.nCount = VULCAN_CLIP_CAPACITY;
	if (IsMultiGame && PowerupClass (nId))
		AddPowerupInMine ((int32_t) nId);
	}
RETURN (nObject)
}

//------------------------------------------------------------------------------

int32_t CreateWeapon (uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos, fix xSize, uint8_t rType)
{
ENTER (0, 0);
if (rType == 255) {
	CWeaponInfo *pWeaponInfo = WEAPONINFO (nId);
	if (!pWeaponInfo)
		RETURN (-1)
	switch (pWeaponInfo->renderType) {
		case WEAPON_RENDER_BLOB:
			xSize = pWeaponInfo->xBlobSize;
			rType = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
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
			xSize = pWeaponInfo->xBlobSize;
			rType = RT_WEAPON_VCLIP;
			break;
		default:
			PrintLog (0, "Error: Invalid weapon render nType in CreateNewWeapon\n");
			RETURN (-1)
		}
	}
RETURN (CreateObject (OBJ_WEAPON, nId, nCreator, nSegment, vPos, CFixMatrix::IDENTITY, xSize, CT_WEAPON, MT_PHYSICS, rType))
}

//------------------------------------------------------------------------------

int32_t CreateFireball (uint8_t nId, int16_t nSegment, const CFixVector& vPos, fix xSize, uint8_t rType)
{
ENTER (0, 0);
RETURN (CreateObject (OBJ_FIREBALL, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, xSize, CT_EXPLOSION, MT_NONE, rType))
}

//------------------------------------------------------------------------------

int32_t CreateDebris (CObject *pParent, int16_t nSubModel)
{
ENTER (0, 0);
RETURN (CreateObject (OBJ_DEBRIS, 0, -1, pParent->info.nSegment, pParent->info.position.vPos, pParent->info.position.mOrient,
							 gameData.modelData.polyModels [0][pParent->ModelId ()].SubModels ().rads [nSubModel],
							 CT_DEBRIS, MT_PHYSICS, RT_POLYOBJ))
}

//------------------------------------------------------------------------------

int32_t CreateCamera (CObject *pParent)
{
ENTER (0, 0);
RETURN (CreateObject (OBJ_CAMERA, 0, -1, pParent->info.nSegment, pParent->info.position.vPos, pParent->info.position.mOrient, 0, CT_NONE, MT_NONE, RT_NONE))
}

//------------------------------------------------------------------------------

int32_t CreateLight (uint8_t nId, int16_t nSegment, const CFixVector& vPos)
{
ENTER (0, 0);
RETURN (CreateObject (OBJ_LIGHT, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, 0, CT_LIGHT, MT_NONE, RT_NONE))
}

//------------------------------------------------------------------------------

void CreateSmallFireballOnObject (CObject *pObj, fix size_scale, int32_t bSound)
{
ENTER (0, 0);
	fix			size;
	CFixVector	vPos, vRand;
	int16_t		nSegment;

vPos = OBJPOS (pObj)->vPos;
vRand = CFixVector::Random();
vRand *= (pObj->info.xSize / 2);
vPos += vRand;
size = FixMul (size_scale, I2X (1) / 2 + RandShort () * 4 / 2);
nSegment = FindSegByPos (vPos, pObj->info.nSegment, 1, 0);
if (nSegment != -1) {
	CObject *pExplObj = CreateExplosion (nSegment, vPos, size, ANIM_SMALL_EXPLOSION);
	if (!pExplObj)
		LEAVE;
	AttachObject (pObj, pExplObj);
	if (bSound || (RandShort () < 8192)) {
		fix vol = I2X (1) / 2;
		if (pObj->info.nType == OBJ_ROBOT)
			vol *= 2;
		audio.CreateObjectSound (SOUND_EXPLODING_WALL, SOUNDCLASS_EXPLOSION, pObj->Index (), 0, vol);
		}
	}
LEAVE
}

//------------------------------------------------------------------------------

void CreateVClipOnObject (CObject *pObj, fix xScale, uint8_t nVClip)
{
ENTER (0, 0);
	fix			xSize;
	CFixVector	vPos, vRand;
	int16_t		nSegment;

vPos = OBJPOS (pObj)->vPos;
vRand = CFixVector::Random();
vRand *= (pObj->info.xSize / 2);
vPos += vRand;
xSize = FixMul (xScale, I2X (1) + RandShort ()*4);
nSegment = FindSegByPos (vPos, pObj->info.nSegment, 1, 0);
if (nSegment != -1) {
	CObject *pExplObj = CreateExplosion (nSegment, vPos, xSize, nVClip);
	if (!pExplObj)
		LEAVE;

	pExplObj->info.movementType = MT_PHYSICS;
	pExplObj->mType.physInfo.velocity = pObj->mType.physInfo.velocity * (I2X (1) / 2);
	}
LEAVE
}

//------------------------------------------------------------------------------
//creates a marker CObject in the world.  returns the CObject number
int32_t DropMarkerObject (CFixVector& vPos, int16_t nSegment, CFixMatrix& orient, uint8_t nMarker)
{
ENTER (0, 0);
	int16_t nObject;

//Assert (gameData.modelData.nMarkerModel != -1);
nObject = CreateObject (OBJ_MARKER, nMarker, -1, nSegment, vPos, orient,
								gameData.modelData.polyModels [0][gameData.modelData.nMarkerModel].Rad (), CT_NONE, MT_NONE, RT_POLYOBJ);
CObject *pObj = OBJECT (nObject);
if (!pObj)
	RETURN (-1)
pObj->rType.polyObjInfo.nModel = gameData.modelData.nMarkerModel;
pObj->mType.spinRate = pObj->info.position.mOrient.m.dir.u * (I2X (1) / 2);
//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all OBJECTS.
pObj->SetLife (IMMORTAL_TIME);
RETURN (nObject)
}

//------------------------------------------------------------------------------
//eof
