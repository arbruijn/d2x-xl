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
#include "object.h"
#include "objeffects.h"
#include "multi.h"

#define LIMIT_PHYSICS_FPS	0

void NDRecordGuidedEnd (void);
void DetachChildObjects (CObject *parent);
void DetachFromParent (CObject *sub);
int32_t FreeObjectSlots (int32_t nRequested);

//info on the various types of OBJECTS
#if DBG
CObject	*dbgObjP = NULL;
#endif

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

int32_t dbgObjInstances = 0;

#define	DEG90		 (I2X (1) / 4)
#define	DEG45		 (I2X (1) / 8)
#define	DEG1		 (I2X (1) / (4 * 90))

//------------------------------------------------------------------------------

CArray<uint16_t> CObject::m_weaponInfo;
CArray<uint8_t> CObject::m_bIsEquipment; 

//------------------------------------------------------------------------------

void CObject::InitTables (void)
{
m_weaponInfo.Create (MAX_WEAPONS);
m_weaponInfo.Clear (0);

m_weaponInfo [CONCUSSION_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [HOMINGMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [SMARTMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [FLASHMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [GUIDEDMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [MERCURYMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [ROBOT_CONCUSSION_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [ROBOT_HOMINGMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [ROBOT_FLASHMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [ROBOT_MERCURYMSL_ID] |= OBJ_IS_MISSILE;
m_weaponInfo [ROBOT_SMARTMSL_ID] |= OBJ_IS_MISSILE;

m_weaponInfo [MEGAMSL_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON;
m_weaponInfo [EARTHSHAKER_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON;
m_weaponInfo [EARTHSHAKER_MEGA_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON | OBJ_BOUNCES;
m_weaponInfo [ROBOT_MEGAMSL_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON;
m_weaponInfo [ROBOT_EARTHSHAKER_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON;
m_weaponInfo [ROBOT_SHAKER_MEGA_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON | OBJ_BOUNCES;
m_weaponInfo [ROBOT_MEGA_FLASHMSL_ID] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON;
m_weaponInfo [ROBOT_MEGA_FLASHMSL_ID + 1] |= OBJ_IS_MISSILE | OBJ_IS_SPLASHDMG_WEAPON;

m_weaponInfo [VULCAN_ID] |= OBJ_IS_WEAPON | OBJ_IS_GATLING_ROUND;
m_weaponInfo [GAUSS_ID] |= OBJ_IS_WEAPON | OBJ_IS_GATLING_ROUND;
m_weaponInfo [ROBOT_VULCAN_ID] |= OBJ_IS_WEAPON | OBJ_IS_GATLING_ROUND; 
m_weaponInfo [ROBOT_GAUSS_ID] |= OBJ_IS_WEAPON | OBJ_IS_GATLING_ROUND;
m_weaponInfo [ROBOT_TI_STREAM_ID] |= OBJ_IS_WEAPON;

m_weaponInfo [FLARE_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [LASER_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [LASER_ID + 1] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [LASER_ID + 2] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [LASER_ID + 3] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [SPREADFIRE_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [PLASMA_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [FUSION_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [SUPERLASER_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [SUPERLASER_ID + 1] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [HELIX_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [PHOENIX_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [OMEGA_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;

m_weaponInfo [ROBOT_BLUE_LASER_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_RED_LASER_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_GREEN_LASER_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_WHITE_LASER_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_PLASMA_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_HELIX_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_PHOENIX_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_PHASE_ENERGY_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_FAST_PHOENIX_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_LIGHT_FIREBALL_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_MEDIUM_FIREBALL_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_VERTIGO_FIREBALL_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_VERTIGO_PHOENIX_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_BLUE_ENERGY_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;
m_weaponInfo [ROBOT_WHITE_ENERGY_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_HAS_LIGHT_TRAIL;

m_weaponInfo [REACTOR_BLOB_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON;
m_weaponInfo [SMARTMSL_BLOB_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_BOUNCES;
m_weaponInfo [ROBOT_SMARTMSL_BLOB_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_BOUNCES;
m_weaponInfo [SMARTMINE_BLOB_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_BOUNCES;
m_weaponInfo [ROBOT_SMARTMINE_BLOB_ID] |= OBJ_IS_WEAPON | OBJ_IS_ENERGY_WEAPON | OBJ_BOUNCES;

m_weaponInfo [PROXMINE_ID] |= OBJ_IS_PLAYER_MINE;
m_weaponInfo [SMARTMINE_ID] |= OBJ_IS_PLAYER_MINE;
m_weaponInfo [SMALLMINE_ID] |= OBJ_IS_PLAYER_MINE;
m_weaponInfo [ROBOT_SMARTMINE_ID] |= OBJ_IS_ROBOT_MINE;

m_bIsEquipment.Create (MAX_POWERUP_TYPES);
m_bIsEquipment.Clear (0);
m_bIsEquipment [POW_EXTRA_LIFE] =
m_bIsEquipment [POW_KEY_BLUE] =
m_bIsEquipment [POW_KEY_RED] =
m_bIsEquipment [POW_KEY_GOLD] =
m_bIsEquipment [POW_FULL_MAP] =
m_bIsEquipment [POW_BLUEFLAG] =
m_bIsEquipment [POW_REDFLAG] =
m_bIsEquipment [POW_CLOAK] =
m_bIsEquipment [POW_INVUL] =
m_bIsEquipment [POW_HOARD_ORB] = 2;
m_bIsEquipment [POW_QUADLASER] =
m_bIsEquipment [POW_CONVERTER] =
m_bIsEquipment [POW_AMMORACK] =
m_bIsEquipment [POW_AFTERBURNER] =
m_bIsEquipment [POW_HEADLIGHT] =
m_bIsEquipment [POW_SLOWMOTION] =
m_bIsEquipment [POW_BULLETTIME] = 1;
}

//------------------------------------------------------------------------------

int32_t bPrintObjectInfo = 0;

tWindowRenderedData windowRenderedData [MAX_RENDERED_WINDOWS];

#if DBG
char	szObjectTypeNames [MAX_OBJECT_TYPES][10] = {
	"WALL     ",
	"FIREBALL ",
	"ROBOT    ",
	"HOSTAGE  ",
	"PLAYER   ",
	"WEAPON   ",
	"CAMERA   ",
	"POWERUP  ",
	"DEBRIS   ",
	"CNTRLCEN ",
	"FLARE    ",
	"CLUTTER  ",
	"GHOST    ",
	"LIGHT    ",
	"COOP     ",
	"MARKER   ",
	"CAMBOT   ",
	"M-BALL   ",
	"SMOKE    ",
	"EXPLOSION",
	"EFFECT   "
};
#endif

// -----------------------------------------------------------------------------

float CObject::Damage (void)
{
	float	fDmg;
	fix	xMaxShield;

if (info.nType == OBJ_PLAYER)
	fDmg = X2F (PLAYER (info.nId).Shield ()) / 100;
else if (info.nType == OBJ_ROBOT) {
	if (0 >= (xMaxShield = RobotDefaultShield (this)))
		return 1.0f;
	fDmg = X2F (info.xShield) / X2F (xMaxShield);
#if 0
	if (gameData.bots.info [0][info.nId].companion)
		fDmg /= 2;
#endif
	}
else if (info.nType == OBJ_REACTOR)
	fDmg = X2F (info.xShield) / X2F (ReactorStrength ());
else if ((info.nType == 255) || (info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED | OF_ARMAGEDDON)))
	fDmg = 0.0f;
else
	fDmg = 1.0f;
return (fDmg > 1.0f) ? 1.0f : (fDmg < 0.0f) ? 0.0f : fDmg;
}

//------------------------------------------------------------------------------

#if DBG
//set viewerP CObject to next CObject in array
void ObjectGotoNextViewer (void)
{
	CObject*	objP;

FORALL_OBJS (objP) {
	if (objP->info.nType != OBJ_NONE) {
		gameData.objs.viewerP = objP;
		return;
		}
	}
Error ("Couldn't find a viewerP CObject!");
}

//------------------------------------------------------------------------------
//set viewerP CObject to next CObject in array
void ObjectGotoPrevViewer (void)
{
	CObject*	objP;
	int32_t 	nStartObj = 0;

nStartObj = OBJ_IDX (gameData.objs.viewerP);		//get viewerP CObject number
FORALL_OBJS (objP) {
	if (--nStartObj < 0)
		nStartObj = gameData.objs.nLastObject [0];
	if (OBJECTS [nStartObj].info.nType != OBJ_NONE) {
		gameData.objs.viewerP = OBJECTS + nStartObj;
		return;
		}
	}
Error ("Couldn't find a viewerP CObject!");
}
#endif

//------------------------------------------------------------------------------

CObject *ObjFindFirstOfType (int32_t nType)
{
	CObject*	objP;

FORALL_OBJS (objP)
	if (objP->info.nType == nType)
		return (objP);
return reinterpret_cast<CObject*> (NULL);
}

//------------------------------------------------------------------------------

int32_t ObjReturnNumOfType (int32_t nType)
{
	int32_t	count = 0;
	CObject*	objP;

FORALL_OBJS (objP)
	if (objP->info.nType == nType)
		count++;
return count;
}

//------------------------------------------------------------------------------

int32_t ObjReturnNumOfTypeAndId (int32_t nType, int32_t id)
{
	int32_t	count = 0;
	CObject*	objP;

FORALL_OBJS (objP)
	if ((objP->info.nType == nType) && (objP->info.nId == id))
	 count++;
 return count;
 }

//------------------------------------------------------------------------------

void ResetPlayerObject (void)
{
//Init physics
gameData.objs.consoleP->mType.physInfo.velocity.SetZero ();
gameData.objs.consoleP->mType.physInfo.thrust.SetZero ();
gameData.objs.consoleP->mType.physInfo.rotVel.SetZero ();
gameData.objs.consoleP->mType.physInfo.rotThrust.SetZero ();
gameData.objs.consoleP->mType.physInfo.brakes = gameData.objs.consoleP->mType.physInfo.turnRoll = 0;
gameData.objs.consoleP->mType.physInfo.mass = gameData.pig.ship.player->mass;
gameData.objs.consoleP->mType.physInfo.drag = gameData.pig.ship.player->drag;
gameData.objs.consoleP->mType.physInfo.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;
//Init render info
gameData.objs.consoleP->info.renderType = (gameData.objs.consoleP->info.nType == OBJ_GHOST) ? RT_NONE : RT_POLYOBJ;
gameData.objs.consoleP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;		//what model is this?
gameData.objs.consoleP->rType.polyObjInfo.nSubObjFlags = 0;		//zero the flags
gameData.objs.consoleP->rType.polyObjInfo.nTexOverride = -1;		//no tmap override!
for (int32_t i = 0; i < MAX_SUBMODELS; i++)
	gameData.objs.consoleP->rType.polyObjInfo.animAngles [i].SetZero ();
// Clear misc
gameData.objs.consoleP->info.nFlags = 0;
}

//------------------------------------------------------------------------------

//sets up N_LOCALPLAYER & gameData.objs.consoleP
void InitMultiPlayerObject (int32_t nStage)
{
if (nStage == 0) {
	Assert ((N_LOCALPLAYER >= 0) && (N_LOCALPLAYER < MAX_PLAYERS));
	if (N_LOCALPLAYER != 0) {
		PLAYER (0) = LOCALPLAYER;
		N_LOCALPLAYER = 0;
		}
	LOCALPLAYER.SetObject (0);
	LOCALPLAYER.SetObservedPlayer (N_LOCALPLAYER);
	LOCALPLAYER.nInvuls =
	LOCALPLAYER.nCloaks = 0;
	}
else {
	gameData.objs.consoleP = OBJECTS + LOCALPLAYER.nObject;
	// at this point, SetType () must not be called, as it links the object to the internal object lists 
	// which causes list corruption during multiplayer object sync'ing
	gameData.objs.consoleP->info.nType = OBJ_PLAYER; 
	gameData.objs.consoleP->info.nId = N_LOCALPLAYER;
	gameData.objs.consoleP->info.controlType = CT_FLYING;
	gameData.objs.consoleP->info.movementType = MT_PHYSICS;
	gameStates.entropy.nTimeLastMoved = -1;
	}
}

//------------------------------------------------------------------------------
//make object0 the player, setting all relevant fields
void InitPlayerObject (void)
{
gameData.objs.consoleP->SetType (OBJ_PLAYER, false);
gameData.objs.consoleP->info.nId = 0;					//no sub-types for player
gameData.objs.consoleP->info.nSignature = 0;			//player has zero, others start at 1
gameData.objs.consoleP->SetSize (gameData.models.polyModels [0][gameData.pig.ship.player->nModel].Rad ());
gameData.objs.consoleP->info.controlType = CT_SLEW;			//default is player slewing
gameData.objs.consoleP->info.movementType = MT_PHYSICS;		//change this sometime
gameData.objs.consoleP->SetLife (IMMORTAL_TIME);
gameData.objs.consoleP->info.nAttachedObj = -1;
ResetPlayerObject ();
InitMultiPlayerObject (1);
}

//------------------------------------------------------------------------------

void CObject::Init (void)
{
info.nType = OBJ_NONE;
cType.explInfo.attached.nNext =
cType.explInfo.attached.nPrev =
cType.explInfo.attached.nParent =
info.nAttachedObj = -1;
info.nFlags = 0;
ResetSgmLinks ();
#if OBJ_LIST_TYPE == 1
ResetLinks ();
#endif
m_shots.nObject = -1;
m_shots.nSignature = -1;
m_xCreationTime =
m_xTimeLastHit = 0;
m_xTimeLastEffect = 0;
m_vStartVel.SetZero ();
}

//------------------------------------------------------------------------------

bool ObjectIsLinked (CObject *objP, int16_t nSegment)
{
if ((nSegment >= 0) && (nSegment < gameData.segs.nSegments)) {
	int16_t nObject = objP->Index ();
	for (int16_t i = SEGMENTS [nSegment].m_objects, j = -1; i >= 0; j = i, i = OBJECTS [i].info.nNextInSeg) {
		if (i == nObject) {
			objP->info.nPrevInSeg = j;
			return true;
			}
		}
	}
return false;
}

//------------------------------------------------------------------------------
//sets up the free list, init player data etc.
void InitObjects (bool bInitPlayer)
{

#if OBJ_LIST_TYPE == 1
CObject* objP, * nextObjP = NULL;
for (objP = gameData.objs.lists.all.head; objP; objP = nextObjP) {
	nextObjP = objP->Links (0).next;
	ReleaseObject (objP->Index ());
	}
#else
while (gameData.objs.nObjects)
	ReleaseObject (gameData.objs.freeList [0]);
#endif
CollideInit ();
ResetSegObjLists ();
gameData.objs.lists.Init ();
gameData.objs.InitFreeList ();
gameData.objs.consoleP =
gameData.objs.viewerP = OBJECTS.Buffer ();
if (bInitPlayer) {
	ClaimObjectSlot (0);
	gameData.objs.nObjects = 1;				//just the player
	InitPlayerObject ();
	gameData.objs.consoleP->LinkToSeg (0);	//put in the world in segment 0
	}
//gameData.objs.consoleP->Link ();
#if OBJ_LIST_TYPE == 1
gameData.objs.consoleP->ResetLinks ();
#endif
gameData.objs.nLastObject [0] = 0;
}

//------------------------------------------------------------------------------
// During sync'ing of multiplayer games and after loading savegames, object
// table entries have been used without removing them from the object free list.
// This is done here, and the affected objects are linked to the object lists.
// The object free list must have been initialized before calling this function.

void ClaimObjectSlots (void)
{
gameData.objs.nObjects = LEVEL_OBJECTS;
gameData.objs.nLastObject [0] = 0;
gameData.objs.lists.Init ();
Assert (OBJECTS [0].info.nType != OBJ_NONE);		//0 should be used

CObject* objP = &OBJECTS [0];
for (int32_t i = gameData.objs.nLastObject [0]; i; i--, objP++) {
#if OBJ_LIST_TYPE == 1
	objP->InitLinks ();
#endif
	if (objP->info.nType < MAX_OBJECT_TYPES) {
		ClaimObjectSlot (objP->Index ()); // allocate object list entry #i - should always work here
		objP->Link ();
		}
	}
}

//------------------------------------------------------------------------------
#if DBG
int32_t IsObjectInSeg (int32_t nSegment, int32_t objn)
{
	int16_t nObject, count = 0;

for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
	if (count > LEVEL_OBJECTS) {
		Int3 ();
		return count;
		}
	if (nObject == objn)
		count++;
	}
 return count;
}

//------------------------------------------------------------------------------

int32_t SearchAllSegsForObject (int32_t nObject)
{
	int32_t i;
	int32_t count = 0;

	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		count += IsObjectInSeg (i, nObject);
	}
	return count;
}

//------------------------------------------------------------------------------

void JohnsObjUnlink (int32_t nSegment, int32_t nObject)
{
	CObject  *objP = OBJECTS + nObject;

Assert (nObject != -1);
if (objP->info.nPrevInSeg == -1)
	SEGMENTS [nSegment].m_objects = objP->info.nNextInSeg;
else
	OBJECTS [objP->info.nPrevInSeg].info.nNextInSeg = objP->info.nNextInSeg;
if (objP->info.nNextInSeg != -1)
	OBJECTS [objP->info.nNextInSeg].info.nPrevInSeg = objP->info.nPrevInSeg;
}

//------------------------------------------------------------------------------

void RemoveAllObjectsBut (int32_t nSegment, int32_t nObject)
{
	int32_t i;

for (i = 0; i <= gameData.segs.nLastSegment; i++)
	if (nSegment != i)
		if (IsObjectInSeg (i, nObject))
			JohnsObjUnlink (i, nObject);
}

//------------------------------------------------------------------------------

int32_t CheckDuplicateObjects (void)
{
	int32_t	count = 0;
	CObject*	objP;

FORALL_OBJS (objP) {
	if (objP->info.nType != OBJ_NONE) {
		count = SearchAllSegsForObject (objP->Index ());
		if (count > 1) {
#if DBG
#	if TRACE
			console.printf (1, "Object %d is in %d segments!\n", objP->Index (), count);
#	endif
			Int3 ();
#endif
			RemoveAllObjectsBut (objP->info.nSegment, objP->Index ());
			return count;
			}
		}
	}
	return count;
}

//------------------------------------------------------------------------------

void ListSegObjects (int32_t nSegment)
{
	int32_t nObject, count = 0;

for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
	count++;
	if (count > LEVEL_OBJECTS)  {
		Int3 ();
		return;
		}
	}
return;
}
#endif

//------------------------------------------------------------------------------

void ResetSegObjLists (void)
{
for (int32_t i = 0; i < LEVEL_SEGMENTS; i++)
	SEGMENTS [i].m_objects = -1;
}

//------------------------------------------------------------------------------

void LinkAllObjsToSegs (void)
{
	CObject*	objP;

FORALL_OBJS (objP)
	objP->LinkToSeg (objP->info.nSegment);
}

//------------------------------------------------------------------------------

void RelinkAllObjsToSegs (void)
{
ResetSegObjLists ();
LinkAllObjsToSegs ();
}

//------------------------------------------------------------------------------

bool CheckSegObjList (CObject *objP, int16_t nObject, int16_t nFirstObj)
{
if (objP->info.nNextInSeg == nFirstObj)
	return false;
if ((nObject == nFirstObj) && (objP->info.nPrevInSeg >= 0))
	return false;
if (((objP->info.nNextInSeg >= 0) && (OBJECTS [objP->info.nNextInSeg].info.nPrevInSeg != nObject)) ||
	 ((objP->info.nPrevInSeg >= 0) && (OBJECTS [objP->info.nPrevInSeg].info.nNextInSeg != nObject)))
	return false;
return true;
}

//------------------------------------------------------------------------------

CObject::CObject ()
{
SetPrev (NULL);
SetNext (NULL);
m_nAttackRobots = -1;
}

//------------------------------------------------------------------------------

CObject::~CObject ()
{
if (Prev ())
	Prev ()->SetNext (Next ());
if (Next ())
	Next ()->SetPrev (Prev ());
SetPrev (NULL);
SetNext (NULL);
}

//------------------------------------------------------------------------------

void CObject::UnlinkFromSeg (void)
{
if (info.nSegment >= 0) {
	if (info.nPrevInSeg == -1)
		SEGMENTS [info.nSegment].m_objects = info.nNextInSeg;
	else
		OBJECTS [info.nPrevInSeg].info.nNextInSeg = info.nNextInSeg;
	if (info.nNextInSeg != -1)
		OBJECTS [info.nNextInSeg].info.nPrevInSeg = info.nPrevInSeg;
	info.nSegment = info.nNextInSeg = info.nPrevInSeg = -1;
	}
}

//------------------------------------------------------------------------------

void CObject::ToBaseObject (tBaseObject *objP)
{
//objP->info = *this;
}

//------------------------------------------------------------------------------

#if 0

void CRobotObject::ToBaseObject (tBaseObject *objP)
{
objP->info = *CObject::GetInfo ();
objP->mType.physInfo = *CPhysicsInfo::GetInfo ();
objP->cType.aiInfo = *CAIStaticInfo::GetInfo ();
objP->rType.polyObjInfo = *CPolyObjInfo::GetInfo ();
}

//------------------------------------------------------------------------------

void CPowerupObject::ToBaseObject (tBaseObject *objP)
{
objP->info = *CObject::GetInfo ();
objP->mType.physInfo = *CPhysicsInfo::GetInfo ();
objP->rType.polyObjInfo = *CPolyObjInfo::GetInfo ();
}

//------------------------------------------------------------------------------

void CWeaponObject::ToBaseObject (tBaseObject *objP)
{
objP->info = *CObject::GetInfo ();
objP->mType.physInfo = *CPhysicsInfo::GetInfo ();
objP->rType.polyObjInfo = *CPolyObjInfo::GetInfo ();
}

//------------------------------------------------------------------------------

void CLightObject::ToBaseObject (tBaseObject *objP)
{
objP->info = *CObject::GetInfo ();
objP->cType.lightInfo = *CObjLightInfo::GetInfo ();
}

//------------------------------------------------------------------------------

void CLightningObject::ToBaseObject (tBaseObject *objP)
{
objP->info = *CObject::GetInfo ();
objP->rType.lightningInfo = *CLightningInfo::GetInfo ();
}

//------------------------------------------------------------------------------

void CParticleObject::ToBaseObject (tBaseObject *objP)
{
objP->info = *CObject::GetInfo ();
objP->rType.particleInfo = *CSmokeInfo::GetInfo ();
}

#endif

//------------------------------------------------------------------------------

void CheckFreeList (void)
{
#if 0
	static uint8_t checkObjs [MAX_OBJECTS_D2X];

memset (checkObjs, 0, LEVEL_OBJECTS);
for (int32_t i = 0; i < LEVEL_OBJECTS; i++) {
	int16_t h = gameData.objs.freeList [i];
	if ((i < gameData.objs.nObjects - 1) && (OBJECTS [h].Type () >= MAX_OBJECT_TYPES))
		BRP;
	if (checkObjs [h])
		BRP;
	else
		checkObjs [h] = 1;
	}
#endif
}

//------------------------------------------------------------------------------
// Try to allocate the object with the index nObject in the object list
// This is only possible if that object is in the list of unallocated objects
// Because otherwise that object is already allocated (live)

int32_t ClaimObjectSlot (int32_t nObject)
{
for (int32_t i = gameData.objs.nObjects; i < LEVEL_OBJECTS; i++) {
	if (gameData.objs.freeList [i] == nObject) {
		if (i != gameData.objs.nObjects)
			Swap (gameData.objs.freeList [i], gameData.objs.freeList [gameData.objs.nObjects]);
		gameData.objs.freeListIndex [nObject] = gameData.objs.nObjects++;
#if DBG
		CheckFreeList ();
#endif
		if (nObject > gameData.objs.nLastObject [0]) {
			gameData.objs.nLastObject [0] = nObject;
			if (gameData.objs.nLastObject [1] < gameData.objs.nLastObject [0])
				gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];
			}
		CObject* objP = OBJECTS + nObject;
		objP->ResetSgmLinks ();
#if OBJ_LIST_TYPE == 1
		objP->SetLinkedType (OBJ_NONE);
		objP->ResetLinks ();
#endif
		return nObject;
		}
	}
return -1;
}

//------------------------------------------------------------------------------

int32_t nUnusedObjectsSlots;

//returns the number of a free object, updating gameData.objs.nLastObject [0].
//Generally, CObject::Create() should be called to get an CObject, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
int32_t AllocObject (int32_t nRequestedObject, bool bReset)
{
	int32_t nObject = (nRequestedObject < 0) ? -1 : ClaimObjectSlot (nRequestedObject);

if (nObject < 0) {
	if (gameData.objs.nObjects >= LEVEL_OBJECTS - 2) {
		FreeObjectSlots (LEVEL_OBJECTS - 10);
		CleanupObjects ();
		}
	if (gameData.objs.nObjects >= LEVEL_OBJECTS)
		return -1;
	nObject = gameData.objs.freeList [gameData.objs.nObjects];
	gameData.objs.freeListIndex [nObject] = gameData.objs.nObjects++;
	}

#if DBG
CheckFreeList ();
#endif

if (nObject > gameData.objs.nLastObject [0]) {
	gameData.objs.nLastObject [0] = nObject;
	if (gameData.objs.nLastObject [1] < gameData.objs.nLastObject [0])
		gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];
	}
CObject* objP = OBJECTS + nObject;
#if DBG
if (objP->info.nType != OBJ_NONE)
	BRP;
#endif
if (bReset) {
	memset (objP, 0, sizeof (*objP));
	objP->info.nType = OBJ_NONE;
	}
objP->ResetSgmLinks ();
#if OBJ_LIST_TYPE == 1
objP->SetLinkedType (OBJ_NONE);
objP->ResetLinks ();
#endif
objP->SetAttackMode (ROBOT_IS_HOSTILE);
objP->info.nAttachedObj =
objP->cType.explInfo.attached.nNext =
objP->cType.explInfo.attached.nPrev =
objP->cType.explInfo.attached.nParent = -1;
return nObject;
}

//------------------------------------------------------------------------------
// D2X-XL keeps several object lists where it sorts objects by certain categories.
// The following code manages these lists.

#if DBG

bool CObject::IsInList (tObjListRef& ref, int32_t nLink)
{
#if OBJ_LIST_TYPE == 1
	CObject	*listObjP;

for (listObjP = ref.head; listObjP; listObjP = listObjP->m_links [nLink].next)
	if (listObjP == this)
		return true;
return false;
#else
return true;
#endif
}

#endif

//------------------------------------------------------------------------------

void CObject::Link (tObjListRef& ref, int32_t nLink)
{
#if OBJ_LIST_TYPE == 1
	CObjListLink& link = m_links [nLink];

link.prev = ref.tail;
if (ref.tail)
	ref.tail->m_links [nLink].next = this;
else
	ref.head = this;
ref.tail = this;
link.list = &ref;
ref.nObjects++;
#endif
}

//------------------------------------------------------------------------------

void RebuildObjectLists (void)
{
#if OBJ_LIST_TYPE == 1
	static bool bRebuilding = false;

if (bRebuilding)
	return;
bRebuilding = true;
PrintLog (0, "Warning: Rebuilding corrupted object lists ...\n");

CObject* objP = OBJECTS.Buffer ();
for (int32_t nObject = gameData.objs.nLastObject [0]; nObject; nObject--, objP++) 
	objP->ResetLinks ();
gameData.objs.lists.Init ();
objP = OBJECTS.Buffer ();
for (int32_t nObject = gameData.objs.nLastObject [0]; nObject; nObject--, objP++) {
	if (objP->Type () < MAX_OBJECT_TYPES) {
		objP->Link ();
		}
	}
bRebuilding = false;
#endif
}

//------------------------------------------------------------------------------

void CObject::Unlink (tObjListRef& ref, int32_t nLink)
{
#if OBJ_LIST_TYPE == 1
	CObjListLink& link = m_links [nLink];
#if DBG
	bool bRebuild = false;
#endif

if (link.prev || link.next) {
	if (ref.nObjects <= 0) {
		link.prev = link.next = NULL;
		link.list = NULL;
		return;
		}
	if (link.next) {
#if DBG
		if (link.next->m_links [nLink].prev != this)
			bRebuild = true;
		else
#endif
			link.next->m_links [nLink].prev = link.prev;
		}
	else {
#if DBG
		if (ref.tail != this)
			bRebuild = true;
		else
#endif
			ref.tail = link.prev;
		}
	if (link.prev) {
#if DBG
		if (link.prev->m_links [nLink].next != this)
			bRebuild = true;
		else
#endif
			link.prev->m_links [nLink].next = link.next;
		}
	else {
#if DBG
		if (ref.head != this)
			bRebuild = true;
		else
#endif
			ref.head = link.next;
		}
	if (ref.head)
		ref.head->m_links [nLink].prev = NULL;
	if (ref.tail)
		ref.tail->m_links [nLink].next = NULL;
	ref.nObjects--;
	}
else if ((ref.head == this) && (ref.tail == this)) {
	ref.head = ref.tail = NULL;
	ref.nObjects = 0;
	}
#	if DBG
if (bRebuild) { //this actually means the list is corrupted -> rebuild all object lists
	RebuildObjectLists ();
	Unlink ();
	return;
	}
#	endif
link.prev = link.next = NULL;
link.list = NULL;
#endif
}

//------------------------------------------------------------------------------

#if DBG

int32_t VerifyObjLists (int32_t nObject = -1);

int32_t VerifyObjLists (int32_t nObject)
{
#if OBJ_LIST_TYPE == 1
	CObject* refObjP = (nObject < 0) ? NULL : gameData.Object (nObject);
	CObject* objP, * firstObjP = gameData.objs.lists.all.head;
	int32_t		i = 0;

if (refObjP && !refObjP->Links (0).prev  && !refObjP->Links (0).next)
	refObjP = NULL;
FORALL_OBJS (objP) {
	if (objP == refObjP)
		refObjP = NULL;
	if (objP->Type () < MAX_OBJECT_TYPES) {
		CObjListLink& link = objP->Links (0);
		if (link.prev && (link.prev->Links (0).next != objP)) {
			RebuildObjectLists ();
			return 0;
			}
		if (link.next && (link.next->Links (0).prev != objP)) {
			RebuildObjectLists ();
			return 0;
			}
		}
	if ((objP == firstObjP) && (++i > 1)) {
		RebuildObjectLists ();
		return 0;
		}
	}
if (refObjP == NULL)
	return 1;
RebuildObjectLists ();
return 0;
#else
return 1;
#endif
}

#endif

//------------------------------------------------------------------------------

void CObject::GetListsForType (uint8_t nType, tObjListRef* lists [])
{
#if OBJ_LIST_TYPE == 1
lists [0] = &gameData.objs.lists.all;
lists [1] = NULL;
if ((nType == OBJ_PLAYER) || (nType == OBJ_GHOST))
	lists [1] = &gameData.objs.lists.players;
else if (nType == OBJ_ROBOT)
	lists [1] = &gameData.objs.lists.robots;
else if (nType == OBJ_WEAPON)
	lists [1] = &gameData.objs.lists.weapons;
else if ((nType != OBJ_REACTOR) && (nType != OBJ_MONSTERBALL)) {
	if (nType == OBJ_POWERUP)
		lists [1] = &gameData.objs.lists.powerups;
	else if (nType == OBJ_EFFECT)
		lists [1] = &gameData.objs.lists.effects;
	else if (nType == OBJ_LIGHT)
		lists [1] = &gameData.objs.lists.lights;
	lists [2] = &gameData.objs.lists.statics;
	return;
	}
lists [2] = &gameData.objs.lists.actors;
#endif
}

//------------------------------------------------------------------------------

void CObject::Link (void)
{
#if OBJ_LIST_TYPE == 1
Unlink (true);
m_nLinkedType = info.nType;
tObjListRef* lists [3] = {NULL, NULL, NULL};
GetListsForType (info.nType, lists);
for (int32_t i = 0; i < 3; i++)
	if (lists [i])
		Link (*lists [i], i);
#endif
}

//------------------------------------------------------------------------------

void CObject::Unlink (bool bForce)
{
#if OBJ_LIST_TYPE == 1
if (bForce || (m_nLinkedType != OBJ_NONE)) {
	m_nLinkedType = OBJ_NONE;
	for (int32_t i = 0; i < 3; i++)
		if (m_links [i].list)
			Unlink (*m_links [i].list, i);
	}
#endif
}

//------------------------------------------------------------------------------

void CObject::Relink (uint8_t nNewType)
{
#if OBJ_LIST_TYPE == 1
tObjListRef* lists [3] = {NULL, NULL, NULL};
GetListsForType (nNewType, lists);
for (int32_t i = 0; i < 3; i++) {
	if (m_links [i].list != lists [i]) {
		if (m_links [i].list)
			Unlink (*m_links [i].list, i);
		if (lists [i])
			Link (*lists [i], i);
		}
	}
m_nLinkedType = nNewType;
#endif
}

//------------------------------------------------------------------------------

void CObject::SetType (uint8_t nNewType, bool bLink)
{
#if OBJ_LIST_TYPE == 1
if (info.nType == nNewType)
	return;

if (bLink)
	Relink (nNewType);
#endif
info.nType = nNewType;
}

//------------------------------------------------------------------------------

int32_t FreeListIndex (int32_t nObject)
{
#if DBG

int32_t i = gameData.objs.freeListIndex [nObject];
if (i >= 0) {
	if (gameData.objs.freeList [i] == nObject)
		return i;
	}	
for (i = gameData.objs.nObjects; i ; )
	if (gameData.objs.freeList [--i] == nObject)
		return gameData.objs.freeListIndex [nObject] = i;
#if DBG
for (i = gameData.objs.nObjects; i < LEVEL_OBJECTS; i++)
	if (gameData.objs.freeList [i] == nObject)
		break;
#endif
return -1;

#else

return gameData.objs.freeListIndex [nObject];

#endif
}

//------------------------------------------------------------------------------

int32_t UpdateFreeList (int32_t nObject)
{
int32_t i = FreeListIndex (nObject);
if (i < 0) {
	PrintLog (0, "FreeObject: Object list is corrupted!\n");
	return 0; 
	}
gameData.objs.freeListIndex [nObject] = -1;
if (i < --gameData.objs.nObjects) {
	Swap (gameData.objs.freeList [i], gameData.objs.freeList [gameData.objs.nObjects]);
	int16_t nObject = gameData.objs.freeList [i];
	gameData.objs.freeListIndex [nObject] = i;
	if (OBJECTS [nObject].Type () >= MAX_OBJECT_TYPES)
		BRP;
	}
#if DBG
CheckFreeList ();
#endif
return 1;
}

//------------------------------------------------------------------------------

void UpdateLastObject (int32_t nObject)
{
if ((nObject >= gameData.objs.nLastObject [0]) || (gameData.objs.nLastObject [0] < gameData.objs.nObjects - 1)) {
	gameData.objs.nLastObject [0] = 0;
	for (int32_t i = 0; i < gameData.objs.nObjects; i++)
		if (gameData.objs.nLastObject [0] < gameData.objs.freeList [i])
			gameData.objs.nLastObject [0] = gameData.objs.freeList [i];
	if (gameData.objs.nLastObject [1] < gameData.objs.nLastObject [0])
		gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];
	}
}

//------------------------------------------------------------------------------
//frees up an CObject.  Generally, ReleaseObject () should be called to get
//rid of an CObject.  This function deallocates the CObject entry after
//the CObject has been unlinked

void FreeObject (int32_t nObject)
{
if ((nObject < 0) || (nObject >= LEVEL_OBJECTS))
	return;

CObject	*objP = OBJECTS + nObject;

objP->Unlink ();
objP->info.nType = OBJ_NONE;		//unused!
objP->SetAttackMode (ROBOT_IS_HOSTILE);
DelObjChildrenN (nObject);
DelObjChildN (nObject);
gameData.objs.bWantEffect [nObject] = 0;
OBJECTS [nObject].RequestEffects (DESTROY_SMOKE | DESTROY_LIGHTNING);
lightManager.Delete (-1, -1, nObject);

if (!UpdateFreeList (nObject))
	return;
UpdateLastObject (nObject);

#if DBG
if (dbgObjP && (OBJ_IDX (dbgObjP) == nObject))
	dbgObjP = NULL;
#endif
}

//-----------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int32_t __fastcall tFreeFilter (CObject *objP);
#else
typedef int32_t tFreeFilter (CObject *objP);
#endif
typedef tFreeFilter *pFreeFilter;


int32_t FreeDebrisFilter (CObject *objP)
{
return objP->info.nType == OBJ_DEBRIS;
}


int32_t FreeFireballFilter (CObject *objP)
{
return (objP->info.nType == OBJ_FIREBALL) && (objP->cType.explInfo.nDestroyedObj == -1);
}



int32_t FreeFlareFilter (CObject *objP)
{
return (objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID);
}



int32_t FreeWeaponFilter (CObject *objP)
{
return (objP->info.nType == OBJ_WEAPON);
}

//-----------------------------------------------------------------------------

int32_t FreeCandidates (int32_t *candidateP, int32_t *nCandidateP, int32_t nToFree, pFreeFilter filterP)
{
	CObject	*objP;
	int32_t		h = *nCandidateP, i;

for (i = 0; i < h; ) {
	objP = OBJECTS + candidateP [i];
	if (!filterP (objP))
		i++;
	else {
		if (i < --h)
			candidateP [i] = candidateP [h];
		objP->Die ();
		if (!--nToFree)
			return 0;
		}
	}
*nCandidateP = h;
return nToFree;
}

//-----------------------------------------------------------------------------
//	Scan the CObject list, freeing down to nRequested OBJECTS
//	Returns number of slots freed.
int32_t FreeObjectSlots (int32_t nRequested)
{
	int32_t		i, nCandidates = 0;
	int32_t		candidates [MAX_OBJECTS_D2X];
	int32_t		nAlreadyFree, nToFree, nOrgNumToFree;

nAlreadyFree = LEVEL_OBJECTS - gameData.objs.nLastObject [0] - 1;

if (LEVEL_OBJECTS - nAlreadyFree < nRequested)
	return 0;

for (i = 0; i <= gameData.objs.nLastObject [0]; i++) {
	if (OBJECTS [i].info.nFlags & OF_SHOULD_BE_DEAD) {
		nAlreadyFree++;
		if (LEVEL_OBJECTS - nAlreadyFree < nRequested)
			return nAlreadyFree;
		}
	else
		switch (OBJECTS [i].info.nType) {
			case OBJ_NONE:
				nAlreadyFree++;
				if (LEVEL_OBJECTS - nAlreadyFree < nRequested)
					return 0;
				break;
			case OBJ_FIREBALL:
			case OBJ_EXPLOSION:
			case OBJ_WEAPON:
			case OBJ_DEBRIS:
				candidates [nCandidates++] = i;
				break;
#if 1
			default:
#else
			case OBJ_FLARE:
			case OBJ_ROBOT:
			case OBJ_HOSTAGE:
			case OBJ_PLAYER:
			case OBJ_REACTOR:
			case OBJ_CLUTTER:
			case OBJ_GHOST:
			case OBJ_LIGHT:
			case OBJ_CAMERA:
			case OBJ_POWERUP:
			case OBJ_MONSTERBALL:
			case OBJ_SMOKE:
			case OBJ_EXPLOSION:
			case OBJ_EFFECT:
#endif
				break;
			}
	}

nToFree = LEVEL_OBJECTS - nRequested - nAlreadyFree;
nOrgNumToFree = nToFree;
if (nToFree > nCandidates) {
#if TRACE
	console.printf (1, "Warning: Asked to free %i objects when there's only %i available.\n", nToFree, nCandidates);
#endif
	nToFree = nCandidates;
	}
if (!(nToFree = FreeCandidates (candidates, &nCandidates, nToFree, FreeDebrisFilter)))
	return nOrgNumToFree;
if (!(nToFree = FreeCandidates (candidates, &nCandidates, nToFree, FreeFireballFilter)))
	return nOrgNumToFree;
if (!(nToFree = FreeCandidates (candidates, &nCandidates, nToFree, FreeFlareFilter)))
	return nOrgNumToFree;
if (!(nToFree = FreeCandidates (candidates, &nCandidates, nToFree, FreeWeaponFilter)))
	return nOrgNumToFree;
return nOrgNumToFree - nToFree;
}

//------------------------------------------------------------------------------

bool CObject::IsLinkedToSeg (int16_t nSegment)
{
	int16_t nObject = OBJ_IDX (this);

for (int16_t h = SEGMENTS [nSegment].m_objects; h >= 0; h = OBJECTS [h].info.nNextInSeg)
	if (nObject == h)
		return true;
return false;
}

//-----------------------------------------------------------------------------

void CObject::LinkToSeg (int32_t nSegment)
{
if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments)) {
	nSegment = FindSegByPos (*GetPos (), 0, 0, 0);
	if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments))
		return;
	}
SetSegment (nSegment);
#if DBG
if (IsLinkedToSeg (nSegment)) {
	UnlinkFromSeg ();
	SetSegment (nSegment);
	}
#else
if (SEGMENTS [nSegment].m_objects == Index ())
	return;
#endif
SetNextInSeg (SEGMENTS [nSegment].m_objects);
#if DBG
if ((info.nNextInSeg < -1) || (info.nNextInSeg >= LEVEL_OBJECTS))
	SetNextInSeg (-1);
#endif
SetPrevInSeg (-1);
SEGMENTS [nSegment].m_objects = Index ();
if (info.nNextInSeg != -1)
	OBJECTS [info.nNextInSeg].SetPrevInSeg (Index ());
}

//--------------------------------------------------------------------
//when an CObject has moved into a new CSegment, this function unlinks it
//from its old CSegment, and links it into the new CSegment
void CObject::RelinkToSeg (int32_t nNewSeg)
{
if ((nNewSeg < 0) || (nNewSeg >= gameData.segs.nSegments))
	return;
UnlinkFromSeg ();
LinkToSeg (nNewSeg);
}

//------------------------------------------------------------------------------

void AdjustMineSpawn (void)
{
if (!IsNetworkGame)
	return;  // No need for this function in any other mode
if (!(gameData.app.GameMode (GM_HOARD | GM_ENTROPY)))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] += gameData.laser.nProximityDropped;
if (!IsEntropyGame)
	LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX] += gameData.laser.nSmartMinesDropped;
gameData.laser.nProximityDropped = 0;
gameData.laser.nSmartMinesDropped = 0;
}

//------------------------------------------------------------------------------

int32_t CObject::SoundClass (void)
{
	int16_t nType = info.nType;

if (nType == OBJ_PLAYER)
	return SOUNDCLASS_PLAYER;
if (nType == OBJ_ROBOT)
	return SOUNDCLASS_ROBOT;
if (nType == OBJ_WEAPON)
	return IsMissile () ? SOUNDCLASS_MISSILE : SOUNDCLASS_GENERIC;
return SOUNDCLASS_GENERIC;
}

//------------------------------------------------------------------------------

int32_t ObjectCount (int32_t nType)
{
	int32_t	h = 0;
	CObject*	objP;

FORALL_OBJS (objP)
	if ((objP->info.nType == nType) && !objP->IsGeometry ())
		h++;
return h;
}

//------------------------------------------------------------------------------
//called after load. Takes number of objects, and objects should be
//compressed.  resets free list, marks unused objects as unused
void ResetObjects (int32_t nObjects)
{
	CObject	*objP = OBJECTS.Buffer ();

if (!objP)
	return;

gameData.objs.nObjects = nObjects;

	int32_t	i;

for (i = 0; i < nObjects; i++, objP++)
	if (objP->info.nType != OBJ_DEBRIS)
		objP->rType.polyObjInfo.nSubObjFlags = 0;
for (; i < LEVEL_OBJECTS; i++, objP++) {
	if ((gameData.demo.nState == ND_STATE_PLAYBACK) && ((objP->info.nType == OBJ_CAMBOT) || (objP->info.nType == OBJ_EFFECT)))
		continue;
	gameData.objs.freeList [i] = i;
	objP->info.nType = OBJ_NONE;
	objP->info.nSegment =
	objP->cType.explInfo.attached.nNext =
	objP->cType.explInfo.attached.nPrev =
	objP->cType.explInfo.attached.nParent =
	objP->info.nAttachedObj = -1;
	objP->rType.polyObjInfo.nSubObjFlags = 0;
	objP->info.nFlags = 0;
	}
gameData.objs.nLastObject [0] = gameData.objs.nObjects - 1;
gameData.objs.nDebris = 0;
}

//------------------------------------------------------------------------------
//Tries to find a CSegment for an CObject, using FindSegByPos ()
int32_t CObject::FindSegment (void)
{
return FindSegByPos (info.position.vPos, info.nSegment, 1, 0);
}

//------------------------------------------------------------------------------
//If an CObject is in a CSegment, set its nSegment field and make sure it's
//properly linked.  If not in any CSegment, returns 0, else 1.
//callers should generally use FindHitpoint ()
int32_t UpdateObjectSeg (CObject * objP, bool bMove)
{
#if DBG
if (objP->Index () == nDbgObj)
	BRP;
#endif
int32_t nNewSeg = objP->FindSegment ();
if (0 > nNewSeg) {
	if (!bMove) {
#if DBG
		if (OBJ_IDX (objP) == 0)
			BRP;
#endif
		return -1;
		}
	nNewSeg = FindClosestSeg (objP->info.position.vPos);
	CSegment* segP = SEGMENTS + nNewSeg;
	CFixVector vOffset = objP->info.position.vPos - segP->Center ();
	CFixVector::Normalize (vOffset);
	objP->info.position.vPos = segP->Center () + vOffset * segP->MinRad ();
	}
if (nNewSeg == objP->info.nSegment)
	return 0;
objP->RelinkToSeg (nNewSeg);
return 1;
}

//------------------------------------------------------------------------------
//go through all OBJECTS and make sure they have the correct CSegment numbers
void FixObjectSegs (void)
{
	CObject*	objP;

FORALL_OBJS (objP) {
	if ((objP->info.nType == OBJ_NONE) || (objP->info.nType == OBJ_CAMBOT) || (objP->info.nType == OBJ_EFFECT))
		continue;
	if (!UpdateObjectSeg (objP))
		continue;
	CSegment* segP = SEGMENTS + objP->info.nSegment;
	fix xScale = segP->MinRad () - objP->info.xSize;
	if (xScale < 0)
		objP->info.position.vPos = segP->Center ();
	else {
		CFixVector	vCenter, vOffset;
		vCenter = segP->Center ();
		vOffset = objP->info.position.vPos - vCenter;
		CFixVector::Normalize (vOffset);
		objP->info.position.vPos = vCenter + vOffset * xScale;
		}
	}
}

//------------------------------------------------------------------------------
//go through all OBJECTS and make sure they have the correct size
void FixObjectSizes (void)
{
	CObject* objP;

FORALL_PLAYER_OBJS (objP)
	objP->AdjustSize ();
FORALL_ROBOT_OBJS (objP)
	objP->AdjustSize ();
}

//------------------------------------------------------------------------------

//delete OBJECTS, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if bClearAll is set, clear even proximity bombs
void ClearTransientObjects (int32_t bClearAll)
{
	CObject*	objP, * prevObjP;

for (CWeaponIterator iter (objP); objP; objP = (prevObjP ? iter.Step () : iter.Start ())) {
	prevObjP = objP;
	if ((!(gameData.weapons.info [objP->info.nId].flags & WIF_PLACABLE) && (bClearAll || ((objP->info.nId != PROXMINE_ID) && (objP->info.nId != SMARTMINE_ID)))) ||
			objP->info.nType == OBJ_FIREBALL ||	objP->info.nType == OBJ_DEBRIS || ((objP->info.nType != OBJ_NONE) && (objP->info.nFlags & OF_EXPLODING))) {
		prevObjP = iter.Back ();
		ReleaseObject (objP->Index ());
		}
	}
}

//------------------------------------------------------------------------------
//attaches an CObject, such as a fireball, to another CObject, such as a robot
void AttachObject (CObject *parentObjP, CObject *childObjP)
{
Assert (childObjP->info.nType == OBJ_FIREBALL);
Assert (childObjP->info.controlType == CT_EXPLOSION);
Assert (childObjP->cType.explInfo.attached.nNext==-1);
Assert (childObjP->cType.explInfo.attached.nPrev==-1);
Assert (parentObjP->info.nAttachedObj == -1 ||
		  OBJECTS [parentObjP->info.nAttachedObj].cType.explInfo.attached.nPrev==-1);
childObjP->cType.explInfo.attached.nNext = parentObjP->info.nAttachedObj;
if (childObjP->cType.explInfo.attached.nNext != -1)
	OBJECTS [childObjP->cType.explInfo.attached.nNext].cType.explInfo.attached.nPrev = OBJ_IDX (childObjP);
parentObjP->info.nAttachedObj = OBJ_IDX (childObjP);
childObjP->cType.explInfo.attached.nParent = OBJ_IDX (parentObjP);
childObjP->info.nFlags |= OF_ATTACHED;
Assert (childObjP->cType.explInfo.attached.nNext != OBJ_IDX (childObjP));
Assert (childObjP->cType.explInfo.attached.nPrev != OBJ_IDX (childObjP));
}

//------------------------------------------------------------------------------
//dettaches one CObject
void DetachFromParent (CObject *objP)
{
	CObject* parentObjP = gameData.Object (objP->cType.explInfo.attached.nParent);

#if DBG
if (!parentObjP)
	BRP;
#endif
if (parentObjP && (parentObjP->Type () != OBJ_NONE) && (parentObjP->info.nAttachedObj != -1)) {
	if (objP->cType.explInfo.attached.nNext != -1) {
		OBJECTS [objP->cType.explInfo.attached.nNext].cType.explInfo.attached.nPrev = objP->cType.explInfo.attached.nPrev;
		}
	if (objP->cType.explInfo.attached.nPrev != -1) {
		OBJECTS [objP->cType.explInfo.attached.nPrev].cType.explInfo.attached.nNext =
			objP->cType.explInfo.attached.nNext;
		}
	else {
		parentObjP->info.nAttachedObj = objP->cType.explInfo.attached.nNext;
		}
	}
objP->cType.explInfo.attached.nNext =
objP->cType.explInfo.attached.nPrev =
objP->cType.explInfo.attached.nParent = -1;
objP->info.nFlags &= ~OF_ATTACHED;
}

//------------------------------------------------------------------------------
//dettaches all OBJECTS from this CObject
void DetachChildObjects (CObject *parentP)
{
	int32_t i =parentP->info.nAttachedObj;

parentP->info.nAttachedObj = -1;
while (i != -1) {
	CObject* objP = gameData.Object (i);
	i = objP->cType.explInfo.attached.nNext;
	objP->cType.explInfo.attached.nNext =
	objP->cType.explInfo.attached.nPrev =
	objP->cType.explInfo.attached.nParent = -1;
	objP->info.nFlags &= ~OF_ATTACHED;
	}
}

//------------------------------------------------------------------------------

void ResetChildObjects (void)
{
	int32_t	i;

for (i = 0; i < LEVEL_OBJECTS; i++) {
	gameData.objs.childObjs [i].objIndex = -1;
	gameData.objs.childObjs [i].nextObj = i + 1;
	}
gameData.objs.childObjs [i - 1].nextObj = -1;
gameData.objs.nChildFreeList = 0;
gameData.objs.firstChild.Clear (0xff);
gameData.objs.parentObjs.Clear (0xff);
}

//------------------------------------------------------------------------------

int32_t CheckChildList (int32_t nParent)
{
	int32_t h, i, j;

if (gameData.objs.firstChild [nParent] == gameData.objs.nChildFreeList)
	return 0;
for (h = 0, i = gameData.objs.firstChild [nParent]; i >= 0; i = j, h++) {
	j = gameData.objs.childObjs [i].nextObj;
	if (j == i)
		return 0;
	if (h > gameData.objs.nLastObject [0])
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t AddChildObjectN (int32_t nParent, int32_t nChild)
{
	int32_t	h, i;

if (gameData.objs.nChildFreeList < 0)
	return 0;
h = gameData.objs.firstChild [nParent];
i = gameData.objs.firstChild [nParent] = gameData.objs.nChildFreeList;
gameData.objs.nChildFreeList = gameData.objs.childObjs [gameData.objs.nChildFreeList].nextObj;
gameData.objs.childObjs [i].nextObj = h;
gameData.objs.childObjs [i].objIndex = nChild;
gameData.objs.parentObjs [nChild] = nParent;
CheckChildList (nParent);
return 1;
}

//------------------------------------------------------------------------------

int32_t AddChildObjectP (CObject *pParent, CObject *pChild)
{
return pParent ? AddChildObjectN (OBJ_IDX (pParent), OBJ_IDX (pChild)) : 0;
//return (pParent->nType == OBJ_PLAYER) ? AddChildObjectN (OBJ_IDX (pParent), OBJ_IDX (pChild)) : 0;
}

//------------------------------------------------------------------------------

int32_t DelObjChildrenN (int32_t nParent)
{
	int32_t	i, j;

for (i = gameData.objs.firstChild [nParent]; i >= 0; i = j) {
	j = gameData.objs.childObjs [i].nextObj;
	gameData.objs.childObjs [i].nextObj = gameData.objs.nChildFreeList;
	gameData.objs.childObjs [i].objIndex = -1;
	gameData.objs.nChildFreeList = i;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t DelObjChildrenP (CObject *pParent)
{
return DelObjChildrenN (OBJ_IDX (pParent));
}

//------------------------------------------------------------------------------

int32_t DelObjChildN (int32_t nChild)
{
	int32_t	nParent, h = -1, i, j;

if (0 > (nParent = gameData.objs.parentObjs [nChild]))
	return 0;
for (i = gameData.objs.firstChild [nParent]; i >= 0; i = j) {
	j = gameData.objs.childObjs [i].nextObj;
	if (gameData.objs.childObjs [i].objIndex == nChild) {
		if (h < 0)
			gameData.objs.firstChild [nParent] = j;
		else
			gameData.objs.childObjs [h].nextObj = j;
		gameData.objs.childObjs [i].nextObj = gameData.objs.nChildFreeList;
		gameData.objs.childObjs [i].objIndex = -1;
		gameData.objs.nChildFreeList = i;
		CheckChildList (nParent);
		return 1;
		}
	h = i;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t DelObjChildP (CObject *pChild)
{
return DelObjChildN (OBJ_IDX (pChild));
}

//------------------------------------------------------------------------------

tObjectRef *GetChildObjN (int16_t nParent, tObjectRef *pChildRef)
{
	int32_t i = pChildRef ? pChildRef->nextObj : gameData.objs.firstChild [nParent];

return (i < 0) ? NULL : gameData.objs.childObjs + i;
}

//------------------------------------------------------------------------------

tObjectRef *GetChildObjP (CObject *pParent, tObjectRef *pChildRef)
{
return GetChildObjN (OBJ_IDX (pParent), pChildRef);
}

//------------------------------------------------------------------------------

CFixMatrix *CObject::View (int32_t i)
{
	tObjectViewData* viewP = gameData.objs.viewData + Index ();

if (viewP->nFrame [i] != gameData.objs.nFrameCount) {
	if (i == 0)
		viewP->mView [0] = OBJPOS (this)->mOrient.Transpose ();
	else {
		View (0);
		viewP->mView [1] = viewP->mView [0].Inverse ();
		}
	viewP->nFrame [i] = gameData.objs.nFrameCount;
	}
return &viewP->mView [i];
}

//------------------------------------------------------------------------------

void CObject::RandomBump (fix xScale, fix xForce, bool bSound)
{
	fix angle;

angle = SRandShort ();
mType.physInfo.rotVel.v.coord.x += FixMul (angle, xScale);
angle = SRandShort ();
mType.physInfo.rotVel.v.coord.z += FixMul (angle, xScale);
CFixVector vRand = CFixVector::Random ();
Bump (vRand, xForce);
if (bSound)
	audio.CreateObjectSound (SOUND_PLAYER_HIT_WALL, SOUNDCLASS_GENERIC, Index ());
}

//------------------------------------------------------------------------------

void CObject::CreateSound (int16_t nSound)
{
audio.CreateSegmentSound (nSound, info.nSegment, 0, info.position.vPos);
}

//------------------------------------------------------------------------------

int32_t CObject::OpenableDoorsInSegment (void)
{
return SEGMENTS [info.nSegment].HasOpenableDoor ();
}

//------------------------------------------------------------------------------

bool CObject::Cloaked (void)
{
if (info.nType == OBJ_PLAYER)
	return (PLAYER (info.nId).flags & PLAYER_FLAGS_CLOAKED) != 0;
else if (info.nType == OBJ_ROBOT)
	return cType.aiInfo.CLOAKED != 0;
else
	return false;
}

//------------------------------------------------------------------------------

int32_t CObject::Index (void)
{
return OBJ_IDX (this);
}

//------------------------------------------------------------------------------

CObject* CObject::Target (void)
{
return m_target ? m_target : gameData.objs.consoleP;
}

//------------------------------------------------------------------------------

CObject* CObject::Parent (void)
{
if (cType.laserInfo.parent.nObject < 0)
	return NULL;
CObject*	objP = OBJECTS + cType.laserInfo.parent.nObject;
if (objP->Signature () != cType.laserInfo.parent.nSignature)
	return NULL;
return objP;
}

//------------------------------------------------------------------------------
// return if object is a guided missile under player control (if not under 
// player control, guided missiles turn into homing missiles)

bool CObject::IsGuidedMissile (int8_t nPlayer) 
{
if (Type () != OBJ_WEAPON)
	return false;
if (Id () != GUIDEDMSL_ID)
	return false;
if (nPlayer < 0) {
	CObject* parentObjP = Parent ();
	if (!parentObjP)
		return false;
	nPlayer = parentObjP->Id ();
	}
return (this == gameData.objs.guidedMissile [nPlayer].objP) && (Signature () == gameData.objs.guidedMissile [nPlayer].nSignature);
}

//------------------------------------------------------------------------------

bool CObject::IsGuideBot (void) 
{
return (info.nType == OBJ_ROBOT) && (info.nId < MAX_ROBOT_TYPES) && ROBOTINFO (info.nId).companion;
}

//------------------------------------------------------------------------------

inline bool CObject::IsThief (void) 
{
return (info.nType == OBJ_ROBOT) && (info.nId < MAX_ROBOT_TYPES) && ROBOTINFO (info.nId).thief;
}

//------------------------------------------------------------------------------

bool CObject::IsPlayerMine (int16_t nId) 
{ 
return (m_weaponInfo [nId] & OBJ_IS_PLAYER_MINE) != 0; 
}

//------------------------------------------------------------------------------

bool CObject::IsPlayerMine (void) 
{ 
return (Type () == OBJ_WEAPON) && (Id () < m_weaponInfo.Length ()) && IsPlayerMine (Id ()); 
}

//------------------------------------------------------------------------------

bool CObject::IsGatlingRound (void) 
{ 
return IsWeapon () && ((m_weaponInfo [Id ()] & OBJ_IS_GATLING_ROUND) != 0);
}

//------------------------------------------------------------------------------

bool CObject::Bounces (void) 
{ 
#if DBG
if (!(IsWeapon () || IsMissile ()))
	return false;
if ((m_weaponInfo [Id ()] & OBJ_BOUNCES) == 0)
	return false;
if ((mType.physInfo.flags & PF_BOUNCE) == 0)
	return false;
return true;
#else
return (IsWeapon () || IsMissile ()) && ((mType.physInfo.flags & PF_BOUNCE) != 0) && ((m_weaponInfo [Id ()] & OBJ_BOUNCES) != 0);
#endif
}

//------------------------------------------------------------------------------

bool CObject::AttacksRobots (void)
{
return (IsRobot () || IsReactor ()) && EGI_FLAG (bRobotsHitRobots, 0, 0, 0) && ((gameStates.app.cheats.bRobotsKillRobots != 0) || Reprogrammed ());
}

//------------------------------------------------------------------------------

bool CObject::AttacksPlayer (void)
{
return (IsRobot () || IsReactor ()) && (gameStates.app.cheats.bRobotsFiring != 0) && !(Disarmed () || Reprogrammed ());
}

//------------------------------------------------------------------------------

bool CObject::AttacksObject (CObject* targetP)
{
return targetP->IsPlayer () ? AttacksPlayer () : AttacksRobots ();
}
		
//------------------------------------------------------------------------------

bool CObject::IsSplashDamageWeapon (void) 
{ 
return ((m_weaponInfo [Id ()] & OBJ_IS_SPLASHDMG_WEAPON) != 0);
}

//------------------------------------------------------------------------------

bool CObject::IsRobotMine (int16_t nId) 
{ 
return (m_weaponInfo [nId] & OBJ_IS_ROBOT_MINE) != 0; 
}

//------------------------------------------------------------------------------

bool CObject::IsRobotMine (void) 
{ 
return IsWeapon () && IsRobotMine (Id ()); 
}

//------------------------------------------------------------------------------

bool CObject::IsMine (int16_t nId) 
{ 
return IsPlayerMine (nId) || IsRobotMine (nId); 
}

//------------------------------------------------------------------------------

bool CObject::IsMine (void) 
{ 
return IsPlayerMine () || IsRobotMine (); 
}

//------------------------------------------------------------------------------

int32_t CObject::ModelId (bool bRaw)
{
return (bRaw || (info.nType != OBJ_PLAYER))
		 ? rType.polyObjInfo.nModel
		 : (gameData.multiplayer.weaponStates [info.nId].nShip == 255)
			? rType.polyObjInfo.nModel
			: rType.polyObjInfo.nModel + gameData.multiplayer.weaponStates [info.nId].nShip;
}

//------------------------------------------------------------------------------

CFixVector CObject::FrontPosition (void)
{
tObjTransformation* objPos = OBJPOS (this);
return objPos->vPos + objPos->mOrient.m.dir.f * info.xSize;
}

//------------------------------------------------------------------------------

fix CObject::ModelRadius (int32_t i)
{
if (info.renderType != RT_POLYOBJ)
	return info.xSize;
int32_t nId = ModelId ();
if (nId < 0)
	return info.xSize;
return gameData.models.polyModels [0][nId].Rad (i);
}

//------------------------------------------------------------------------------

fix CObject::PowerupSize (void)
{
return (fix) gameData.objs.pwrUp.info [info.nId].size;
}

//------------------------------------------------------------------------------

void CObject::DrainEnergy (void) 
{ 
tRobotInfo* botInfoP = &ROBOTINFO (info.nId);
if (botInfoP->energyDrain && LOCALPLAYER.Energy ()) {
	m_xTimeEnergyDrain = gameStates.app.nSDLTicks [0];
	LOCALPLAYER.UpdateEnergy (-I2X (botInfoP->energyDrain));
	}
}

//------------------------------------------------------------------------------

bool CObject::Appearing (bool bVisible)
{
int32_t nStage = AppearanceStage ();
return (nStage != 0) && (!bVisible || (nStage > 0)); 
}

//------------------------------------------------------------------------------

int32_t CObject::AppearanceTimer (void)
{
if (Type () != OBJ_PLAYER)
	return 0;
return gameData.multiplayer.tAppearing [Id ()][0];
}

//------------------------------------------------------------------------------

int32_t CObject::AppearanceStage (void)
{
if (Type () != OBJ_PLAYER)
	return 0;
int32_t t = AppearanceTimer ();
return (t < 0) ? -1 : (t > 0) ? 1 : 0;
}

//------------------------------------------------------------------------------

float CObject::AppearanceScale (void)
{
return Appearing () ? float (gameData.multiplayer.tAppearing [Id ()][0]) / float (gameData.multiplayer.tAppearing [Id ()][1]) : 1.0f;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if OBJ_ITERATOR_TYPE == 1

//------------------------------------------------------------------------------

CObjectIterator::CObjectIterator () : m_objP (NULL), m_nLink (0) {}

//------------------------------------------------------------------------------

CObjectIterator::CObjectIterator (CObject*& objP) 
	: m_objP (NULL), m_nLink (0)
{ 
objP = Start (); 
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Start (void)
{
m_nLink = Link ();
#if DBG
if (!m_nLink)
	BRP;
#endif
m_i = 0;
return m_objP = Head ();
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Head (void)
{
return gameData.objs.lists.all.head;
}

//------------------------------------------------------------------------------

bool CObjectIterator::Done (void)
{
return m_objP == NULL;
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Next (void)
{
return m_objP ? m_objP->Links (m_nLink).next : NULL;
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Back (void)
{
if (m_objP)
	m_objP = m_objP->Links (m_nLink).prev;
if (m_i > 0)
	m_i--;
return m_objP;
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Step (void)
{
if (Done ())
	return NULL;
m_objP = m_objP->Links (m_nLink).next;
++m_i;
#if 1 //DBG // disable code for release builds once it has proven stable
if (m_objP ? m_i >= Size () : m_i < Size ()) { // error! stop anyway
	RebuildObjectLists ();
	m_objP = NULL;
	}
#endif
return m_objP;
}

//------------------------------------------------------------------------------

CObject* CPlayerIterator::Head (void) { return gameData.objs.lists.players.head; }

CObject* CRobotIterator::Head (void) { return gameData.objs.lists.robots.head; }

CObject* CWeaponIterator::Head (void) { return gameData.objs.lists.weapons.head; }

CObject* CPowerupIterator::Head (void) { return gameData.objs.lists.powerups.head; }

CObject* CEffectIterator::Head (void) { return gameData.objs.lists.effects.head; }

CObject* CLightIterator::Head (void) { return gameData.objs.lists.lights.head; }

CObject* CActorIterator::Head (void) { return gameData.objs.lists.actors.head; }

CObject* CStaticObjectIterator::Head (void) { return gameData.objs.lists.statics.head; }

//------------------------------------------------------------------------------

int32_t CObjectIterator::Size (void) { return gameData.objs.lists.all.nObjects; }

int32_t CPlayerIterator::Size (void) { return gameData.objs.lists.players.nObjects; }

int32_t CRobotIterator::Size (void) { return gameData.objs.lists.robots.nObjects; }

int32_t CWeaponIterator::Size (void) { return gameData.objs.lists.weapons.nObjects; }

int32_t CPowerupIterator::Size (void) { return gameData.objs.lists.powerups.nObjects; }

int32_t CEffectIterator::Size (void) { return gameData.objs.lists.effects.nObjects; }

int32_t CLightIterator::Size (void) { return gameData.objs.lists.lights.nObjects; }

int32_t CActorIterator::Size (void) { return gameData.objs.lists.actors.nObjects; }

int32_t CStaticObjectIterator::Size (void) { return gameData.objs.lists.statics.nObjects; }

//------------------------------------------------------------------------------
// Despite doing the same as CObjectIterator::Start(), these c'tors are required
// because they set the virtual function pointer table up before calling Start().
// Just going via CObjectIterator's c'tor would mean that Start () would call
// the Head () and Size () functions of CObjectIterator instead of the derived class.

CPlayerIterator::CPlayerIterator (CObject*& objP) { objP = Start (); }

CRobotIterator::CRobotIterator (CObject*& objP) { objP = Start (); }

CWeaponIterator::CWeaponIterator (CObject*& objP) { objP = Start (); }

CPowerupIterator::CPowerupIterator (CObject*& objP) { objP = Start (); }

CEffectIterator::CEffectIterator (CObject*& objP) { objP = Start (); }

CLightIterator::CLightIterator (CObject*& objP) { objP = Start (); }

CActorIterator::CActorIterator (CObject*& objP) { objP = Start (); }

CStaticObjectIterator::CStaticObjectIterator (CObject*& objP) { objP = Start (); }

//------------------------------------------------------------------------------

#else // OBJ_LIST_ITERATOR

//------------------------------------------------------------------------------

CObjectIterator::CObjectIterator () 
	: m_objP (NULL), m_i (0) 
{}

CObjectIterator::CObjectIterator (CObject*& objP)
	: m_objP (NULL), m_i (0) 
{
objP = Start ();
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Start (void)
{
m_i = 0;
if (!OBJECTS.Buffer ())
	return m_objP = NULL;
if (!gameData.objs.nObjects)
	return m_objP = NULL;
m_objP = Current ();
if (Match ())
	return m_objP;
return Step ();
}

//------------------------------------------------------------------------------

bool CObjectIterator::Done (void)
{
return m_i >= gameData.objs.nObjects;
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Current (void)
{
return m_objP = OBJECTS + gameData.objs.freeList [m_i];
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Back (void)
{
if (gameData.objs.nObjects && m_i) {
	--m_i;
	return Current ();
	}
return NULL;
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Next (void)
{
if (m_i < gameData.objs.nObjects - 1) {
	++m_i;
	return Current ();
	}
return NULL;
}

//------------------------------------------------------------------------------

CObject* CObjectIterator::Step (void)
{
if (!Done ())
	while (++m_i < gameData.objs.nObjects) {
		Current ();
		if (Match ())
			return m_objP;
		}
return m_objP = NULL;
}

//------------------------------------------------------------------------------

CPlayerIterator::CPlayerIterator (CObject*& objP) { objP = Start (); }

CRobotIterator::CRobotIterator (CObject*& objP) { objP = Start (); }

CWeaponIterator::CWeaponIterator (CObject*& objP) { objP = Start (); }

CPowerupIterator::CPowerupIterator (CObject*& objP) { objP = Start (); }

CEffectIterator::CEffectIterator (CObject*& objP) { objP = Start (); }

CLightIterator::CLightIterator (CObject*& objP) { objP = Start (); }

CActorIterator::CActorIterator (CObject*& objP) { objP = Start (); }

CStaticObjectIterator::CStaticObjectIterator (CObject*& objP) { objP = Start (); }

//	-----------------------------------------------------------------------------

bool CObjectIterator::Match (void) { return true; }

bool CPlayerIterator::Match (void) { return (m_objP->Type () == OBJ_PLAYER) || (m_objP->Type () == OBJ_GHOST); }

bool CRobotIterator::Match (void) { return (m_objP->Type () == OBJ_ROBOT); }

bool CWeaponIterator::Match (void) { return (m_objP->Type () == OBJ_WEAPON); }

bool CPowerupIterator::Match (void) { return (m_objP->Type () == OBJ_POWERUP); }

bool CEffectIterator::Match (void) { return (m_objP->Type () == OBJ_EFFECT); }

bool CLightIterator::Match (void) { return (m_objP->Type () == OBJ_LIGHT); }

bool CActorIterator::Match (void) { 
	uint8_t nType = m_objP->Type ();
	return (nType == OBJ_PLAYER) || (nType == OBJ_GHOST) || (nType == OBJ_ROBOT) || (nType == OBJ_REACTOR) || (nType == OBJ_WEAPON) || (nType == OBJ_MONSTERBALL); 
	}

bool CStaticObjectIterator::Match (void) { 
	uint8_t nType = m_objP->Type ();
	return (nType == OBJ_POWERUP) || (nType == OBJ_EFFECT) || (nType == OBJ_LIGHT); 
	}

#endif // OBJ_LIST_ITERATOR

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if DBG

void CPositionTracker::Update (CFixVector& vPos, uint8_t bIdleAnimation)
{
m_positions [m_nCurPos].bIdleAnimation = bIdleAnimation; 
m_positions [m_nCurPos].vPos = vPos;
m_positions [m_nCurPos].xTime = gameStates.app.nSDLTicks [0];
m_nCurPos = (m_nCurPos + 1) % POSTRACK_MAXFRAMES;
if (m_nPosCount < POSTRACK_MAXFRAMES) 
	m_nPosCount++;
}

//------------------------------------------------------------------------------

int32_t CPositionTracker::Check (int32_t nId)
{
	CFixVector	v0, v1;
	fix			t0, t1;
	float			maxSpeed = X2F (ROBOTINFO (nId).xMaxSpeed [gameStates.app.nDifficultyLevel]);

for (int32_t i = m_nCurPos, j = 0; j < m_nPosCount; j++, i = (i + 1) % POSTRACK_MAXFRAMES) {
	if (!j) {
		v1 = m_positions [i].vPos;
		t1 = m_positions [i].xTime;
		}
	else {
		v0 = v1;
		t0 = t1;
		v1 = m_positions [i].vPos;
		t1 = m_positions [i].xTime;
		float t = float (t1 - t0);
		if (t > 0.0) {
			float speed = X2F (CFixVector::Dist (v0, v1)) / t;
			if (speed > maxSpeed * 2.0f)
				return 0; // too fast
			}
		}
	}
return 1;
}

#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//eof
