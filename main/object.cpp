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
#ifdef TACTILE
#	include "tactile.h"
#endif
#ifdef EDITOR
#	include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

#define LIMIT_PHYSICS_FPS	0

void DetachChildObjects (CObject *parent);
void DetachFromParent (CObject *sub);
int FreeObjectSlots (int nRequested);

//info on the various types of OBJECTS
#if DBG
CObject	Object_minus_one;
CObject	*dbgObjP = NULL;
#endif

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

int dbgObjInstances = 0;

#define	DEG90		 (F1_0 / 4)
#define	DEG45		 (F1_0 / 8)
#define	DEG1		 (F1_0 / (4 * 90))

//------------------------------------------------------------------------------
// CBitmap *robot_bms [MAX_ROBOT_BITMAPS];	//all bitmaps for all robots

// int robot_bm_nums [MAX_ROBOT_TYPES];		//starting bitmap num for each robot
// int robot_n_bitmaps [MAX_ROBOT_TYPES];		//how many bitmaps for each robot

// char *gameData.bots.names [MAX_ROBOT_TYPES];		//name of each robot

//--unused-- int NumRobotTypes = 0;

int bPrintObjectInfo = 0;
//@@int ObjectViewer = 0;

//CObject * SlewObject = NULL;	// Object containing slew CObject info.

//--unused-- int Player_controllerType = 0;

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

float ObjectDamage (CObject *objP)
{
	float	fDmg;
	fix	xMaxShields;

if (objP->info.nType == OBJ_PLAYER)
	fDmg = X2F (gameData.multiplayer.players [objP->info.nId].shields) / 100;
else if (objP->info.nType == OBJ_ROBOT) {
	xMaxShields = RobotDefaultShields (objP);
	fDmg = X2F (objP->info.xShields) / X2F (xMaxShields);
#if 0
	if (gameData.bots.info [0][objP->info.nId].companion)
		fDmg /= 2;
#endif
	}
else if (objP->info.nType == OBJ_REACTOR)
	fDmg = X2F (objP->info.xShields) / X2F (ReactorStrength ());
else if ((objP->info.nType == 255) || (objP->info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED | OF_ARMAGEDDON)))
	fDmg = 0.0f;
else
	fDmg = 1.0f;
return (fDmg > 1.0f) ? 1.0f : (fDmg < 0.0f) ? 0.0f : fDmg;
}

// -----------------------------------------------------------------------------

int FindBoss (int nBossObj)
{
	int	i, j = BOSS_COUNT;

for (i = 0; i < j; i++)
	if (gameData.boss [i].nObject == nBossObj)
		return i;
return -1;
}

//------------------------------------------------------------------------------

void InitGateIntervals (void)
{
	int	i;

for (i = 0; i < MAX_BOSS_COUNT; i++)
	gameData.boss [i].nGateInterval = F1_0 * 4 - gameStates.app.nDifficultyLevel * I2X (2) / 3;
}

//------------------------------------------------------------------------------

#if DBG
//set viewerP CObject to next CObject in array
void ObjectGotoNextViewer ()
{
	int 		i;
	CObject	*objP;

FORALL_OBJS (objP, i) {
	if (objP->info.nType != OBJ_NONE) {
		gameData.objs.viewerP = objP;
		return;
		}
	}
Error ("Couldn't find a viewerP CObject!");
}

//------------------------------------------------------------------------------
//set viewerP CObject to next CObject in array
void ObjectGotoPrevViewer ()
{
	int 		i, nStartObj = 0;
	CObject	*objP;

nStartObj = OBJ_IDX (gameData.objs.viewerP);		//get viewerP CObject number
FORALL_OBJS (objP, i) {
	if (--nStartObj < 0)
		nStartObj = gameData.objs.nLastObject [0];
	if (OBJECTS [nStartObj].info.nType != OBJ_NONE)	{
		gameData.objs.viewerP = OBJECTS + nStartObj;
		return;
		}
	}
Error ("Couldn't find a viewerP CObject!");
}
#endif

//------------------------------------------------------------------------------

CObject *ObjFindFirstOfType (int nType)
{
	int		i;
	CObject	*objP;

FORALL_OBJS (objP, i)
	if (objP->info.nType == nType)
		return (objP);
return reinterpret_cast<CObject*> (NULL);
}

//------------------------------------------------------------------------------

int ObjReturnNumOfType (int nType)
{
	int		i, count = 0;
	CObject	*objP;

FORALL_OBJS (objP, i)
	if (objP->info.nType == nType)
		count++;
return count;
}

//------------------------------------------------------------------------------

int ObjReturnNumOfTypeAndId (int nType, int id)
{
	int		i, count = 0;
	CObject	*objP;

FORALL_OBJS (objP, i)
	if ((objP->info.nType == nType) && (objP->info.nId == id))
	 count++;
 return count;
 }

//------------------------------------------------------------------------------
// These variables are used to keep a list of the 3 closest robots to the viewerP.
// The code works like this: Every time render CObject is called with a polygon model,
// it finds the distance of that robot to the viewerP.  If this distance if within 10
// segments of the viewerP, it does the following: If there aren't already 3 robots in
// the closet-robots list, it just sticks that CObject into the list along with its distance.
// If the list already contains 3 robots, then it finds the robot in that list that is
// farthest from the viewerP. If that CObject is farther than the CObject currently being
// rendered, then the new CObject takes over that far CObject's slot.  *Then* after all
// OBJECTS are rendered, object_render_targets is called an it draws a target on top
// of all the OBJECTS.

//091494: #define MAX_CLOSE_ROBOTS 3
//--unused-- static int Object_draw_lock_boxes = 0;
//091494: static int Object_num_close = 0;
//091494: static CObject * Object_close_ones [MAX_CLOSE_ROBOTS];
//091494: static fix Object_closeDistance [MAX_CLOSE_ROBOTS];

//091494: set_closeObjects (CObject *objP)
//091494: {
//091494: 	fix dist;
//091494:
//091494: 	if ((objP->info.nType != OBJ_ROBOT) || (Object_draw_lock_boxes == 0))
//091494: 		return;
//091494:
//091494: 	// The following code keeps a list of the 10 closest robots to the
//091494: 	// viewerP.  See comments in front of this function for how this works.
//091494: 	dist = VmVecDist (&objP->info.position.vPos, &gameData.objs.viewerP->info.position.vPos);
//091494: 	if (dist < I2X (20*10))	{
//091494: 		if (Object_num_close < MAX_CLOSE_ROBOTS)	{
//091494: 			Object_close_ones [Object_num_close] = obj;
//091494: 			Object_closeDistance [Object_num_close] = dist;
//091494: 			Object_num_close++;
//091494: 		} else {
//091494: 			int i, farthestRobot;
//091494: 			fix farthestDistance;
//091494: 			// Find the farthest robot in the list
//091494: 			farthestRobot = 0;
//091494: 			farthestDistance = Object_closeDistance [0];
//091494: 			for (i=1; i<Object_num_close; i++)	{
//091494: 				if (Object_closeDistance [i] > farthestDistance)	{
//091494: 					farthestDistance = Object_closeDistance [i];
//091494: 					farthestRobot = i;
//091494: 				}
//091494: 			}
//091494: 			// If this CObject is closer to the viewerP than
//091494: 			// the farthest in the list, replace the farthest with this CObject.
//091494: 			if (farthestDistance > dist)	{
//091494: 				Object_close_ones [farthestRobot] = obj;
//091494: 				Object_closeDistance [farthestRobot] = dist;
//091494: 			}
//091494: 		}
//091494: 	}
//091494: }


//	------------------------------------------------------------------------------------------------------------------

void CreateSmallFireballOnObject (CObject *objP, fix size_scale, int bSound)
{
	fix			size;
	CFixVector	pos, rand_vec;
	short			nSegment;

pos = objP->info.position.vPos;
rand_vec = CFixVector::Random();
rand_vec *= (objP->info.xSize / 2);
pos += rand_vec;
size = FixMul (size_scale, F1_0 / 2 + d_rand () * 4 / 2);
nSegment = FindSegByPos (pos, objP->info.nSegment, 1, 0);
if (nSegment != -1) {
	CObject *explObjP = ObjectCreateExplosion (nSegment, &pos, size, VCLIP_SMALL_EXPLOSION);
	if (!explObjP)
		return;
	AttachObject (objP, explObjP);
	if (d_rand () < 8192) {
		fix	vol = F1_0 / 2;
		if (objP->info.nType == OBJ_ROBOT)
			vol *= 2;
		else if (bSound)
			DigiLinkSoundToObject (SOUND_EXPLODING_WALL, OBJ_IDX (objP), 0, vol, SOUNDCLASS_EXPLOSION);
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------

void CreateVClipOnObject (CObject *objP, fix size_scale, ubyte vclip_num)
{
	fix			size;
	CFixVector	pos, rand_vec;
	short			nSegment;

pos = objP->info.position.vPos;
rand_vec = CFixVector::Random();
rand_vec *= (objP->info.xSize / 2);
pos += rand_vec;
size = FixMul (size_scale, F1_0 + d_rand ()*4);
nSegment = FindSegByPos (pos, objP->info.nSegment, 1, 0);
if (nSegment != -1) {
	CObject *explObjP = ObjectCreateExplosion (nSegment, &pos, size, vclip_num);
	if (!explObjP)
		return;

	explObjP->info.movementType = MT_PHYSICS;
	explObjP->mType.physInfo.velocity[X] = objP->mType.physInfo.velocity[X] / 2;
	explObjP->mType.physInfo.velocity[Y] = objP->mType.physInfo.velocity[Y] / 2;
	explObjP->mType.physInfo.velocity[Z] = objP->mType.physInfo.velocity[Z] / 2;
	}
}

//------------------------------------------------------------------------------

void ResetPlayerObject (void)
{
	int i;

//Init physics
gameData.objs.consoleP->mType.physInfo.velocity.SetZero();
gameData.objs.consoleP->mType.physInfo.thrust.SetZero();
gameData.objs.consoleP->mType.physInfo.rotVel.SetZero();
gameData.objs.consoleP->mType.physInfo.rotThrust.SetZero();
gameData.objs.consoleP->mType.physInfo.brakes = gameData.objs.consoleP->mType.physInfo.turnRoll = 0;
gameData.objs.consoleP->mType.physInfo.mass = gameData.pig.ship.player->mass;
gameData.objs.consoleP->mType.physInfo.drag = gameData.pig.ship.player->drag;
gameData.objs.consoleP->mType.physInfo.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;
//Init render info
gameData.objs.consoleP->info.renderType = RT_POLYOBJ;
gameData.objs.consoleP->rType.polyObjInfo.nModel = gameData.pig.ship.player->nModel;		//what model is this?
gameData.objs.consoleP->rType.polyObjInfo.nSubObjFlags = 0;		//zero the flags
gameData.objs.consoleP->rType.polyObjInfo.nTexOverride = -1;		//no tmap override!
for (i = 0; i < MAX_SUBMODELS; i++)
	gameData.objs.consoleP->rType.polyObjInfo.animAngles[i].SetZero();
// Clear misc
gameData.objs.consoleP->info.nFlags = 0;
}

//------------------------------------------------------------------------------
//make object0 the CPlayerData, setting all relevant fields
void InitPlayerObject ()
{
gameData.objs.consoleP->SetType (OBJ_PLAYER);
gameData.objs.consoleP->info.nId = 0;					//no sub-types for CPlayerData
gameData.objs.consoleP->info.nSignature = 0;			//CPlayerData has zero, others start at 1
gameData.objs.consoleP->info.xSize = gameData.models.polyModels [gameData.pig.ship.player->nModel].rad;
gameData.objs.consoleP->info.controlType = CT_SLEW;			//default is CPlayerData slewing
gameData.objs.consoleP->info.movementType = MT_PHYSICS;		//change this sometime
gameData.objs.consoleP->info.xLifeLeft = IMMORTAL_TIME;
gameData.objs.consoleP->info.nAttachedObj = -1;
ResetPlayerObject ();
}

//------------------------------------------------------------------------------

void InitIdToOOF (void)
{
memset (gameData.objs.idToOOF, 0, sizeof (gameData.objs.idToOOF));
gameData.objs.idToOOF [MEGAMSL_ID] = OOF_MEGA;
}

//------------------------------------------------------------------------------

void CObject::Init (void)
{
info.nType = OBJ_NONE;
info.nSegment =
info.nPrevInSeg =
info.nNextInSeg =
cType.explInfo.attached.nNext =
cType.explInfo.attached.nPrev =
cType.explInfo.attached.nParent =
info.nAttachedObj = -1;
info.nFlags = 0;
memset (m_links, 0, sizeof (m_links));
m_shots.nObject = -1;
m_shots.nSignature = -1;
m_xCreationTime =
m_xTimeLastHit = 0;
m_vStartVel.SetZero ();
}

//------------------------------------------------------------------------------
//sets up the free list & init CPlayerData & whatever else
void InitObjects (void)
{
	CObject	*objP;
	int		i;

CollideInit ();
for (i = 0, objP = OBJECTS.Buffer (); i < MAX_OBJECTS; i++, objP++) {
	gameData.objs.freeList [i] = i;
	objP->Init ();
	}
ResetSegObjLists ();
gameData.objs.consoleP =
gameData.objs.viewerP = OBJECTS.Buffer ();
InitPlayerObject ();
gameData.objs.consoleP->LinkToSeg (0);	//put in the world in segment 0
gameData.objs.nObjects = 1;						//just the CPlayerData
gameData.objs.nLastObject [0] = 0;
InitIdToOOF ();
}

//------------------------------------------------------------------------------
//after calling InitObject (), the network code has grabbed specific
//object slots without allocating them.  Go through the objects and build
//the free list, then set the appropriate globals
void SpecialResetObjects (void)
{
	CObject	*objP;
	int		i;

gameData.objs.nObjects = MAX_OBJECTS;
gameData.objs.nLastObject [0] = 0;
memset (&gameData.objs.lists, 0, sizeof (gameData.objs.lists));
Assert (OBJECTS [0].info.nType != OBJ_NONE);		//0 should be used
for (objP = OBJECTS.Buffer () + MAX_OBJECTS, i = MAX_OBJECTS; i; ) {
	objP--, i--;
#if DBG
	if (i == nDbgObj) {
		nDbgObj = nDbgObj;
		if (objP->info.nType != OBJ_NONE)
			dbgObjInstances++;
		}
#endif
	objP->InitLinks ();
	if (objP->info.nType == OBJ_NONE)
		gameData.objs.freeList [--gameData.objs.nObjects] = i;
	else {
		if (i > gameData.objs.nLastObject [0])
			gameData.objs.nLastObject [0] = i;
		objP->Link ();
		}
	}
}

//------------------------------------------------------------------------------
#if DBG
int IsObjectInSeg (int nSegment, int objn)
{
	short nObject, count = 0;

for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg)	{
	if (count > MAX_OBJECTS) {
		Int3 ();
		return count;
		}
	if (nObject==objn)
		count++;
	}
 return count;
}

//------------------------------------------------------------------------------

int SearchAllSegsForObject (int nObject)
{
	int i;
	int count = 0;

	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		count += IsObjectInSeg (i, nObject);
	}
	return count;
}

//------------------------------------------------------------------------------

void JohnsObjUnlink (int nSegment, int nObject)
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

void RemoveIncorrectObjects ()
{
	int nSegment, nObject, count;

for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
	count = 0;
	for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
		count++;
		#if DBG
		if (count > MAX_OBJECTS)	{
#if TRACE
			console.printf (1, "Object list in CSegment %d is circular.\n", nSegment);
#endif
			Int3 ();
		}
		#endif
		if (OBJECTS [nObject].info.nSegment != nSegment)	{
			#if DBG
#if TRACE
			console.printf (CON_DBG, "Removing CObject %d from CSegment %d.\n", nObject, nSegment);
#endif
			Int3 ();
			#endif
			JohnsObjUnlink (nSegment, nObject);
			}
		}
	}
}

//------------------------------------------------------------------------------

void RemoveAllObjectsBut (int nSegment, int nObject)
{
	int i;

for (i = 0; i <= gameData.segs.nLastSegment; i++)
	if (nSegment != i)
		if (IsObjectInSeg (i, nObject))
			JohnsObjUnlink (i, nObject);
}

//------------------------------------------------------------------------------

int CheckDuplicateObjects (void)
{
	int 		i, count = 0;
	CObject	*objP;

FORALL_OBJS (objP, i) {
	if (objP->info.nType != OBJ_NONE)	{
		count = SearchAllSegsForObject (OBJ_IDX (objP));
		if (count > 1)	{
#if DBG
#	if TRACE
			console.printf (1, "Object %d is in %d segments!\n", OBJ_IDX (objP), count);
#	endif
			Int3 ();
#endif
			RemoveAllObjectsBut (objP->info.nSegment, OBJ_IDX (objP));
			return count;
			}
		}
	}
	return count;
}

//------------------------------------------------------------------------------

void ListSegObjects (int nSegment)
{
	int nObject, count = 0;

for (nObject = SEGMENTS [nSegment].m_objects; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
	count++;
	if (count > MAX_OBJECTS) 	{
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
	int	i, j = gameData.segs.nSegments ? gameData.segs.nSegments : MAX_SEGMENTS;

for (i = 0; i < j; i++)
	SEGMENTS [i].m_objects = -1;
}

//------------------------------------------------------------------------------

void LinkAllObjsToSegs (void)
{
	CObject	*objP;
	int		i;

FORALL_OBJS (objP, i)
	objP->LinkToSeg (objP->info.nSegment);
}

//------------------------------------------------------------------------------

void RelinkAllObjsToSegs (void)
{
ResetSegObjLists ();
LinkAllObjsToSegs ();
}

//------------------------------------------------------------------------------

bool CheckSegObjList (CObject *objP, short nObject, short nFirstObj)
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
if (info.nPrevInSeg == -1)
	SEGMENTS [info.nSegment].m_objects = info.nNextInSeg;
else
	OBJECTS [info.nPrevInSeg].info.nNextInSeg = info.nNextInSeg;
if (info.nNextInSeg != -1)
	OBJECTS [info.nNextInSeg].info.nPrevInSeg = info.nPrevInSeg;
info.nSegment = info.nNextInSeg = info.nPrevInSeg = -1;
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

int InsertObject (int nObject)
{
for (int i = gameData.objs.nObjects; i < MAX_OBJECTS; i++)
	if (gameData.objs.freeList [i] == nObject) {
		gameData.objs.freeList [i] = gameData.objs.freeList [gameData.objs.nObjects++];
		if (nObject > gameData.objs.nLastObject [0]) {
			gameData.objs.nLastObject [0] = nObject;
			if (gameData.objs.nLastObject [1] < gameData.objs.nLastObject [0])
				gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];
		}
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int nDebrisObjectCount = 0;
int nUnusedObjectsSlots;

//returns the number of a free object, updating gameData.objs.nLastObject [0].
//Generally, CObject::Create() should be called to get an CObject, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
int AllocObject (void)
{
	CObject *objP;
	int		nObject;

if (gameData.objs.nObjects >= MAX_OBJECTS - 2) {
	FreeObjectSlots (MAX_OBJECTS - 10);
	CleanupObjects ();
	}
if (gameData.objs.nObjects >= MAX_OBJECTS)
	return -1;
nObject = gameData.objs.freeList [gameData.objs.nObjects++];
#if DBG
if (nObject == nDbgObj) {
	PrintLog ("allocating object #%d\n", nObject);
	nDbgObj = nDbgObj;
	if (dbgObjInstances++ > 0)
		nDbgObj = nDbgObj;
	}
#endif
if (nObject > gameData.objs.nLastObject [0]) {
	gameData.objs.nLastObject [0] = nObject;
	if (gameData.objs.nLastObject [1] < gameData.objs.nLastObject [0])
		gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];
	}
objP = OBJECTS + nObject;
#if DBG
if (objP->info.nType != OBJ_NONE)
	objP = objP;
#endif
memset (objP, 0, sizeof (*objP));
objP->info.nType = OBJ_NONE;
objP->SetLinkedType (OBJ_NONE);
objP->info.nAttachedObj =
objP->cType.explInfo.attached.nNext =
objP->cType.explInfo.attached.nPrev =
objP->cType.explInfo.attached.nParent = -1;
return nObject;
}

//------------------------------------------------------------------------------

#if DBG

bool CObject::IsInList (tObjListRef& ref, int nLink)
{
	CObject	*listObjP;

for (listObjP = ref.head; listObjP; listObjP = listObjP->m_links [nLink].next)
	if (listObjP == this)
		return true;
return false;
}

#endif

//------------------------------------------------------------------------------

void CObject::Link (tObjListRef& ref, int nLink)
{
	CObjListLink& link = m_links [nLink];

#if DBG
if (IsInList (ref, nLink)) {
	//PrintLog ("object %d, type %d, link %d is already linked\n", objP - gameData.objs.objects, objP->info.nType);
	return;
	}
if (this - gameData.objs.objects == nDbgObj)
	nDbgObj = nDbgObj;
#endif
link.prev = ref.tail;
if (ref.tail)
	ref.tail->m_links [nLink].next = this;
else
	ref.head = this;
ref.tail = this;
ref.nObjects++;
}

//------------------------------------------------------------------------------

void CObject::Unlink (tObjListRef& ref, int nLink)
{
	CObjListLink& link = m_links [nLink];

if (link.prev || link.next) {
	if (ref.nObjects <= 0)
		return;
	if (link.next)
		link.next->m_links [nLink].prev = link.prev;
	else
		ref.tail = link.prev;
	if (link.prev)
		link.prev->m_links [nLink].next = link.next;
	else
		ref.head = link.next;
	link.prev = link.next = NULL;
	ref.nObjects--;
	}
else if ((ref.head == this) && (ref.tail == this))
		ref.head = ref.tail = NULL;
else if (ref.head == this) { //this actually means the list is corrupted
	if (!ref.tail)
		ref.head = NULL;
	else
		for (ref.head = ref.tail; ref.head->m_links [nLink].prev; ref.head = ref.head->m_links [nLink].prev)
			;
	}
else if (ref.tail == this) { //this actually means the list is corrupted
	if (!ref.head)
		ref.head = NULL;
	else
		for (ref.tail = ref.head; ref.tail->m_links [nLink].next; ref.tail = ref.tail->m_links [nLink].next)
			;
	}
#if DBG
if (IsInList (ref, nLink)) {
	PrintLog ("object %d, type %d, link %d is still linked\n", this - gameData.objs.objects, info.nType);
	return;
	}
CObject *objP;
if (objP = ref.head) {
	if (objP->Links (nLink).next == objP)
		nDbgObj = nDbgObj;
	if (objP->Links (nLink).prev == objP)
		nDbgObj = nDbgObj;
	}
#endif
}

//------------------------------------------------------------------------------

void CObject::Link (void)
{
	ubyte nType = info.nType;

#if DBG
if (this - gameData.objs.objects == nDbgObj) {
	nDbgObj = nDbgObj;
	//PrintLog ("linking object #%d, type %d\n", objP - gameData.objs.objects, nType);
	}
#endif
Unlink ();
m_nLinkedType = nType;
Link (gameData.objs.lists.all, 0);
if (nType == OBJ_PLAYER)
	Link (gameData.objs.lists.players, 1);
else if (nType == OBJ_ROBOT)
	Link (gameData.objs.lists.robots, 1);
else if (nType != OBJ_REACTOR) {
	if (nType == OBJ_WEAPON)
		Link (gameData.objs.lists.weapons, 1);
	else if (nType == OBJ_POWERUP)
		Link (gameData.objs.lists.powerups, 1);
	else if (nType == OBJ_EFFECT)
		Link (gameData.objs.lists.effects, 1);
	else if (nType == OBJ_LIGHT)
		Link (gameData.objs.lists.lights, 1);
	else
		m_links [1].prev = m_links [1].next = NULL;
#ifdef _DEBUG
	if (nType == OBJ_CAMBOT)
		nType = nType;
#endif
	Link (gameData.objs.lists.statics, 2);
	return;
	}
Link (gameData.objs.lists.actors, 2);
}

//------------------------------------------------------------------------------

void CObject::Unlink (void)
{
	ubyte nType = m_nLinkedType;

if (nType != OBJ_NONE) {
#if DBG
	if (this - gameData.objs.objects == nDbgObj) {
		nDbgObj = nDbgObj;
		//PrintLog ("unlinking object #%d, type %d\n", objP - gameData.objs.objects, nType);
		}
#endif
	m_nLinkedType = OBJ_NONE;
	Unlink (gameData.objs.lists.all, 0);
	if (nType == OBJ_PLAYER)
		Unlink (gameData.objs.lists.players, 1);
	else if (nType == OBJ_ROBOT)
		Unlink (gameData.objs.lists.robots, 1);
	else if (nType != OBJ_REACTOR) {
		if (nType == OBJ_WEAPON)
			Unlink (gameData.objs.lists.weapons, 1);
		else if (nType == OBJ_POWERUP)
			Unlink (gameData.objs.lists.powerups, 1);
		else if (nType == OBJ_EFFECT)
			Unlink (gameData.objs.lists.effects, 1);
		else if (nType == OBJ_LIGHT)
			Unlink (gameData.objs.lists.lights, 1);
		Unlink (gameData.objs.lists.statics, 2);
		return;
		}
	Unlink (gameData.objs.lists.actors, 2);
	}
}

//------------------------------------------------------------------------------

void CObject::SetType (ubyte nNewType)
{
if (info.nType != nNewType) {
	Unlink ();
	info.nType = nNewType;
	Link ();
	}
}

//------------------------------------------------------------------------------
//frees up an CObject.  Generally, ReleaseObject () should be called to get
//rid of an CObject.  This function deallocates the CObject entry after
//the CObject has been unlinked
void FreeObject (int nObject)
{
	CObject	*objP = OBJECTS + nObject;

#if DBG
if (nObject == nDbgObj) {
	//PrintLog ("freeing object #%d\n", nObject);
	nDbgObj = nDbgObj;
	if (dbgObjInstances > 0)
		dbgObjInstances--;
	else
		nDbgObj = nDbgObj;
	}
#endif
objP->Unlink ();
DelObjChildrenN (nObject);
DelObjChildN (nObject);
gameData.objs.bWantEffect [nObject] = 0;
#if 1
RequestEffects (OBJECTS + nObject, DESTROY_SMOKE | DESTROY_LIGHTNINGS);
#else
SEM_ENTER (SEM_SMOKE)
KillObjectSmoke (nObject);
SEM_LEAVE (SEM_SMOKE)
SEM_ENTER (SEM_LIGHTNINGS)
lightningManager.DestroyForObject (OBJECTS + nObject);
SEM_LEAVE (SEM_LIGHTNINGS)
#endif
RemoveDynLight (-1, -1, nObject);
gameData.objs.freeList [--gameData.objs.nObjects] = nObject;
Assert (gameData.objs.nObjects >= 0);
if (nObject == gameData.objs.nLastObject [0])
	while (OBJECTS [--gameData.objs.nLastObject [0]].info.nType == OBJ_NONE);
#if DBG
if (dbgObjP && (OBJ_IDX (dbgObjP) == nObject))
	dbgObjP = NULL;
#endif
}

//-----------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int __fastcall tFreeFilter (CObject *objP);
#else
typedef int tFreeFilter (CObject *objP);
#endif
typedef tFreeFilter *pFreeFilter;


int FreeDebrisFilter (CObject *objP)
{
return objP->info.nType == OBJ_DEBRIS;
}


int FreeFireballFilter (CObject *objP)
{
return (objP->info.nType == OBJ_FIREBALL) && (objP->cType.explInfo.nDeleteObj == -1);
}



int FreeFlareFilter (CObject *objP)
{
return (objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID);
}



int FreeWeaponFilter (CObject *objP)
{
return (objP->info.nType == OBJ_WEAPON);
}

//-----------------------------------------------------------------------------

int FreeCandidates (int *candidateP, int *nCandidateP, int nToFree, pFreeFilter filterP)
{
	CObject	*objP;
	int		h = *nCandidateP, i;

for (i = 0; i < h; ) {
	objP = OBJECTS + candidateP [i];
	if (!filterP (objP))
		i++;
	else {
		if (i < --h)
			candidateP [i] = candidateP [h];
		objP->Kill ();
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
int FreeObjectSlots (int nRequested)
{
	int		i, nCandidates = 0;
	int		candidates [MAX_OBJECTS_D2X];
	int		nAlreadyFree, nToFree, nOrgNumToFree;

nAlreadyFree = MAX_OBJECTS - gameData.objs.nLastObject [0] - 1;

if (MAX_OBJECTS - nAlreadyFree < nRequested)
	return 0;

for (i = 0; i <= gameData.objs.nLastObject [0]; i++) {
	if (OBJECTS [i].info.nFlags & OF_SHOULD_BE_DEAD) {
		nAlreadyFree++;
		if (MAX_OBJECTS - nAlreadyFree < nRequested)
			return nAlreadyFree;
		}
	else
		switch (OBJECTS [i].info.nType) {
			case OBJ_NONE:
				nAlreadyFree++;
				if (MAX_OBJECTS - nAlreadyFree < nRequested)
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

nToFree = MAX_OBJECTS - nRequested - nAlreadyFree;
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

bool CObject::IsLinkedToSeg (short nSegment)
{
	short nObject = OBJ_IDX (this);

for (short h = SEGMENTS [nSegment].m_objects; h >= 0; h = OBJECTS [h].info.nNextInSeg)
	if (nObject == h)
		return true;
return false;
}

//-----------------------------------------------------------------------------

void CObject::LinkToSeg (int nSegment)
{
if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments)) {
	nSegment = FindSegByPos (*GetPos (), 0, 0, 0);
	if (nSegment < 0)
		return;
	}
SetSegment (nSegment);
#if DBG
if (IsLinkedToSeg (nSegment))
	UnlinkFromSeg ();
#else
if (SEGMENTS [nSegment].m_objects == nObject)
	return;
#endif
SetNextInSeg (SEGMENTS [nSegment].m_objects);
SetPrevInSeg (-1);
SEGMENTS [nSegment].m_objects = OBJ_IDX (this);
if (info.nNextInSeg != -1)
	OBJECTS [info.nNextInSeg].info.nPrevInSeg = OBJ_IDX (this);
}

//------------------------------------------------------------------------------

void CObject::Initialize (ubyte nType, ubyte nId, short nCreator, short nSegment, const CFixVector& vPos,
								  const CFixMatrix& mOrient, fix xSize, ubyte cType, ubyte mType, ubyte rType)
{
SetSignature (gameData.objs.nNextSignature++);
SetType (nType);
SetId (nId);
SetLastPos (&vPos);
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
SetShields (20 * F1_0);
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
	if (ROBOTINFO ((int) nId).bossFlag && (BOSS_COUNT >= MAX_BOSS_COUNT))
		return -1;
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
	if (nDebrisObjectCount >= gameStates.render.detail.nMaxDebrisObjects)
		return -1;
	}

// Zero out object structure to keep weird bugs from happening in uninitialized fields.
m_nId = OBJ_IDX (this);
SetSignature (gameData.objs.nNextSignature++);
SetType (nType);
SetId (nId);
SetLastPos (&vPos);
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
	m_vStartVel.SetZero();
if (GetRenderType () == RT_POLYOBJ)
	CPolyObjInfo::SetTexOverride (-1);

if (GetType () == OBJ_WEAPON) {
	CPhysicsInfo::SetFlags (CPhysInfo.GetFlags () | WI_persistent (m_info.nId) * PF_PERSISTENT);
	CLaserInfo::SetCreationTime (gameData.time.xGame);
	CLaserInfo::SetLastHitObj (0);
	CLaserInfo::SetScale (F1_0);
	}
else if (GetType () == OBJ_DEBRIS)
	nDebrisObjectCount++;
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
	if (ROBOTINFO ((int) nId).bossFlag && (BOSS_COUNT >= MAX_BOSS_COUNT))
		return -1;
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
nSegment = FindSegByPos (vPos, nSegment, 1, 0);
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment))
	return -1;

if (nType == OBJ_DEBRIS) {
	if (nDebrisObjectCount >= gameStates.render.detail.nMaxDebrisObjects)
		return -1;
	}

// Find next free object
if (0 > (nObject = AllocObject ()))
	return -1;
if (!nObject)
	nObject = nObject;
objP = OBJECTS + nObject;
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
	objP->cType.laserInfo.xScale = F1_0;
	}
else if (objP->info.nType == OBJ_DEBRIS)
	nDebrisObjectCount++;
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
cloneP->mType.physInfo.thrust.SetZero();
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
return CreateObject (OBJ_ROBOT, nId, -1, nSegment, vPos, CFixMatrix::IDENTITY, gameData.models.polyModels [ROBOTINFO (nId).nModel].rad, 
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
	switch (gameData.weapons.info [nId].renderType)	{
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
			xSize = F1_0;
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
							gameData.models.polyModels [parentP->rType.polyObjInfo.nModel].subModels.rads [nSubModel],
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

#ifdef EDITOR
//create a copy of an CObject. returns new CObject number
int ObjectCreateCopy (int nObject, CFixVector *new_pos, int nNewSegnum)
{
	CObject *objP;
	int newObjNum = AllocObject ();

// Find next D2_FREE CObject
if (newObjNum == -1)
	return -1;
objP = OBJECTS + newObjNum;
*objP = OBJECTS [nObject];
objP->info.position.vPos = objP->info.vLastPos = *new_pos;
objP->info.nNextInSeg = objP->info.nPrevInSeg = objP->info.nSegment = -1;
objP->Link ();
objP->LinkToSeg (nNewSegnum);
objP->info.nSignature = gameData.objs.nNextSignature++;
//we probably should initialize sub-structures here
return newObjNum;
}
#endif

//------------------------------------------------------------------------------

void NDRecordGuidedEnd (void);

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
	nDebrisObjectCount--;
OBJECTS [nObject].UnlinkFromSeg ();
Assert (OBJECTS [0].info.nNextInSeg != 0);
if ((objP->info.nType == OBJ_ROBOT) || (objP->info.nType == OBJ_REACTOR))
	ExecObjTriggers (nObject, 0);
objP->info.nType = OBJ_NONE;		//unused!
objP->info.nSignature = -1;
objP->info.nSegment = -1;				// zero it!
FreeObject (nObject);
SpawnLeftoverPowerups (nObject);
}

//------------------------------------------------------------------------------

#define	DEATH_SEQUENCE_EXPLODE_TIME	 (F1_0*2)

CObject	*viewerSaveP;
int		nPlayerFlagsSave;
fix		xCameraToPlayerDistGoal=F1_0*4;
ubyte		nControlTypeSave, nRenderTypeSave;

//------------------------------------------------------------------------------

void DeadPlayerEnd (void)
{
if (!gameStates.app.bPlayerIsDead)
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordRestoreCockpit ();
gameStates.app.bPlayerIsDead = 0;
gameStates.app.bPlayerExploded = 0;
if (gameData.objs.deadPlayerCamera) {
	ReleaseObject (OBJ_IDX (gameData.objs.deadPlayerCamera));
	gameData.objs.deadPlayerCamera = NULL;
	}
SelectCockpit (gameStates.render.cockpit.nModeSave);
gameStates.render.cockpit.nModeSave = -1;
gameData.objs.viewerP = viewerSaveP;
gameData.objs.consoleP->SetType (OBJ_PLAYER);
gameData.objs.consoleP->info.nFlags = nPlayerFlagsSave;
Assert ((nControlTypeSave == CT_FLYING) || (nControlTypeSave == CT_SLEW));
gameData.objs.consoleP->info.controlType = nControlTypeSave;
gameData.objs.consoleP->info.renderType = nRenderTypeSave;
LOCALPLAYER.flags &= ~PLAYER_FLAGS_INVULNERABLE;
gameStates.app.bPlayerEggsDropped = 0;
}

//------------------------------------------------------------------------------

//	Camera is less than size of CPlayerData away from
void SetCameraPos (CFixVector *vCameraPos, CObject *objP)
{
	CFixVector	vPlayerCameraOffs = *vCameraPos - objP->info.position.vPos;
	int			count = 0;
	fix			xCameraPlayerDist;
	fix			xFarScale;

xCameraPlayerDist = vPlayerCameraOffs.Mag();
if (xCameraPlayerDist < xCameraToPlayerDistGoal) { // 2*objP->info.xSize) {
	//	Camera is too close to CPlayerData CObject, so move it away.
	tFVIQuery	fq;
	tFVIData		hit_data;
	CFixVector	local_p1;

	if (vPlayerCameraOffs.IsZero())
		vPlayerCameraOffs[X] += F1_0/16;

	hit_data.hit.nType = HIT_WALL;
	xFarScale = F1_0;

	while ((hit_data.hit.nType != HIT_NONE) && (count++ < 6)) {
		CFixVector	closer_p1;
		CFixVector::Normalize(vPlayerCameraOffs);
		vPlayerCameraOffs *= xCameraToPlayerDistGoal;

		fq.p0 = &objP->info.position.vPos;
		closer_p1 = objP->info.position.vPos + vPlayerCameraOffs;	//	This is the actual point we want to put the camera at.
		vPlayerCameraOffs *= xFarScale;				//	...but find a point 50% further away...
		local_p1 = objP->info.position.vPos + vPlayerCameraOffs;		//	...so we won't have to do as many cuts.

		fq.p1					= &local_p1;
		fq.startSeg			= objP->info.nSegment;
		fq.radP0				=
		fq.radP1				= 0;
		fq.thisObjNum		= OBJ_IDX (objP);
		fq.ignoreObjList	= NULL;
		fq.flags				= 0;
		FindVectorIntersection (&fq, &hit_data);

		if (hit_data.hit.nType == HIT_NONE)
			*vCameraPos = closer_p1;
		else {
			vPlayerCameraOffs = CFixVector::Random();
			xFarScale = 3*F1_0 / 2;
			}
		}
	}
}

//------------------------------------------------------------------------------

extern int nProximityDropped, nSmartminesDropped;

void DeadPlayerFrame (void)
{
	fix			xTimeDead, h;
	CFixVector	fVec;

if (gameStates.app.bPlayerIsDead) {
	xTimeDead = gameData.time.xGame - gameStates.app.nPlayerTimeOfDeath;

	//	If unable to create camera at time of death, create now.
	if (!gameData.objs.deadPlayerCamera) {
		CObject *playerP = OBJECTS + LOCALPLAYER.nObject;
		int nObject = CreateCamera (playerP);
		if (nObject != -1)
			gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
		else
			Int3 ();
		}
	h = DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead;
	h = max (0, h);
	gameData.objs.consoleP->mType.physInfo.rotVel = CFixVector::Create(h / 4, h / 2, h / 3);
	xCameraToPlayerDistGoal = min (xTimeDead * 8, F1_0 * 20) + gameData.objs.consoleP->info.xSize;
	SetCameraPos (&gameData.objs.deadPlayerCamera->info.position.vPos, gameData.objs.consoleP);
	fVec = gameData.objs.consoleP->info.position.vPos - gameData.objs.deadPlayerCamera->info.position.vPos;
/*
	gameData.objs.deadPlayerCamera->position.mOrient = CFixMatrix::Create(fVec, NULL, NULL);
*/
	// TODO: MatrixCreateFCheck
	gameData.objs.deadPlayerCamera->info.position.mOrient = CFixMatrix::CreateF(fVec);

	if (xTimeDead > DEATH_SEQUENCE_EXPLODE_TIME) {
		if (!gameStates.app.bPlayerExploded) {
		if (LOCALPLAYER.hostages.nOnBoard > 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_2, LOCALPLAYER.hostages.nOnBoard);
		else if (LOCALPLAYER.hostages.nOnBoard == 1)
			HUDInitMessage (TXT_SHIP_DESTROYED_1);
		else
			HUDInitMessage (TXT_SHIP_DESTROYED_0);

#ifdef TACTILE
			if (TactileStick)
				ClearForces ();
#endif
			gameStates.app.bPlayerExploded = 1;
			if (gameData.app.nGameMode & GM_NETWORK) {
				AdjustMineSpawn ();
				MultiCapObjects ();
				}
			DropPlayerEggs (gameData.objs.consoleP);
			gameStates.app.bPlayerEggsDropped = 1;
			if (IsMultiGame)
				MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
			ExplodeBadassPlayer (gameData.objs.consoleP);
			//is this next line needed, given the badass call above?
			ExplodeObject (gameData.objs.consoleP, 0);
			gameData.objs.consoleP->info.nFlags &= ~OF_SHOULD_BE_DEAD;		//don't really kill CPlayerData
			gameData.objs.consoleP->info.renderType = RT_NONE;				//..just make him disappear
			gameData.objs.consoleP->SetType (OBJ_GHOST);						//..and kill intersections
			LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#if 0
			if (gameOpts->gameplay.bFastRespawn)
				gameStates.app.bDeathSequenceAborted = 1;
#endif
			}
		}
	else {
		if (d_rand () < gameData.time.xFrame * 4) {
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendCreateExplosion (gameData.multiplayer.nLocalPlayer);
			CreateSmallFireballOnObject (gameData.objs.consoleP, F1_0, 1);
			}
		}
	if (gameStates.app.bDeathSequenceAborted) { //xTimeDead > DEATH_SEQUENCE_LENGTH) {
		StopPlayerMovement ();
		gameStates.app.bEnterGame = 2;
		if (!gameStates.app.bPlayerEggsDropped) {
			if (gameData.app.nGameMode & GM_NETWORK) {
				AdjustMineSpawn ();
				MultiCapObjects ();
				}
			DropPlayerEggs (gameData.objs.consoleP);
			gameStates.app.bPlayerEggsDropped = 1;
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendPlayerExplode (MULTI_PLAYER_EXPLODE);
			}
		DoPlayerDead ();		//kill_playerP ();
		}
	}
}

//------------------------------------------------------------------------------

void AdjustMineSpawn ()
{
if (!(gameData.app.nGameMode & GM_NETWORK))
	return;  // No need for this function in any other mode
if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]+=nProximityDropped;
if (!(gameData.app.nGameMode & GM_ENTROPY))
	LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX]+=nSmartminesDropped;
nProximityDropped = 0;
nSmartminesDropped = 0;
}



int nKilledInFrame = -1;
short nKilledObjNum = -1;
extern char bMultiSuicide;

//	------------------------------------------------------------------------

void StartPlayerDeathSequence (CObject *playerP)
{
	int	nObject;

Assert (playerP == gameData.objs.consoleP);
gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted = 0;
if (gameStates.app.bPlayerIsDead)
	return;
if (gameData.objs.deadPlayerCamera) {
	ReleaseObject (OBJ_IDX (gameData.objs.deadPlayerCamera));
	gameData.objs.deadPlayerCamera = NULL;
	}
StopConquerWarning ();
//Assert (gameStates.app.bPlayerIsDead == 0);
//Assert (gameData.objs.deadPlayerCamera == NULL);
ResetRearView ();
if (!(gameData.app.nGameMode & GM_MULTI))
	HUDClearMessages ();
nKilledInFrame = gameData.app.nFrameCount;
nKilledObjNum = OBJ_IDX (playerP);
gameStates.app.bDeathSequenceAborted = 0;
if (gameData.app.nGameMode & GM_MULTI) {
	MultiSendKill (LOCALPLAYER.nObject);
//		If Hoard, increase number of orbs by 1
//    Only if you haven't killed yourself
//		This prevents cheating
	if (gameData.app.nGameMode & GM_HOARD)
		if (!bMultiSuicide)
			if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]<12)
				LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]++;
	}
paletteManager.SetRedEffect (40);
gameStates.app.bPlayerIsDead = 1;
#ifdef TACTILE
   if (TactileStick)
	Buffeting (70);
#endif
//LOCALPLAYER.flags &= ~ (PLAYER_FLAGS_AFTERBURNER);
playerP->mType.physInfo.rotThrust.SetZero();
playerP->mType.physInfo.thrust.SetZero();
gameStates.app.nPlayerTimeOfDeath = gameData.time.xGame;
nObject = CreateCamera (playerP);
viewerSaveP = gameData.objs.viewerP;
if (nObject != -1)
	gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
else {
	Int3 ();
	gameData.objs.deadPlayerCamera = NULL;
	}
if (gameStates.render.cockpit.nModeSave == -1)		//if not already saved
	gameStates.render.cockpit.nModeSave = gameStates.render.cockpit.nMode;
SelectCockpit (CM_LETTERBOX);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordLetterbox ();
nPlayerFlagsSave = playerP->info.nFlags;
nControlTypeSave = playerP->info.controlType;
nRenderTypeSave = playerP->info.renderType;
playerP->info.nFlags &= ~OF_SHOULD_BE_DEAD;
//	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
playerP->info.controlType = CT_NONE;
if (!gameStates.entropy.bExitSequence) {
	playerP->info.xShields = F1_0*1000;
	MultiSendShields ();
	}
paletteManager.SetEffect (0, 0, 0);
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
		ReleaseObject (OBJ_IDX (objP));
	else {
		if (objP->info.nId == gameData.multiplayer.nLocalPlayer) {
			if (nLocalDeadPlayerObj == -1) {
				StartPlayerDeathSequence (objP);
				nLocalDeadPlayerObj = OBJ_IDX (objP);
				}
			else
				Int3 ();
			}
		}
	}
}

//--------------------------------------------------------------------
//when an CObject has moved into a new CSegment, this function unlinks it
//from its old CSegment, and links it into the new CSegment
void CObject::RelinkToSeg (int nNewSeg)
{
#if DBG
	short nObject = OBJ_IDX (this);
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0])) {
	PrintLog ("invalid object in RelinkObjToSeg\r\n");
	return;
	}
if ((nNewSeg < 0) || (nNewSeg > gameData.segs.nLastSegment)) {
	PrintLog ("invalid segment in RelinkObjToSeg\r\n");
	return;
	}
#endif
UnlinkFromSeg ();
LinkToSeg (nNewSeg);
#if DBG
#if TRACE
if (SEGMENTS [info.nSegment].SideMasks (info.position.vPos, 0).m_center)
	console.printf (1, "CObject::RelinkToSeg violates seg masks.\n");
#endif
#endif
}

//--------------------------------------------------------------------
//process a continuously-spinning CObject
void SpinObject (CObject *objP)
{
	CAngleVector rotangs;
	CFixMatrix rotmat, new_pm;

Assert (objP->info.movementType == MT_SPINNING);
rotangs = CAngleVector::Create((fixang) FixMul (objP->mType.spinRate[X], gameData.time.xFrame),
                            (fixang) FixMul (objP->mType.spinRate[Y], gameData.time.xFrame),
                            (fixang) FixMul (objP->mType.spinRate[Z], gameData.time.xFrame));
rotmat = CFixMatrix::Create(rotangs);
// TODO MM
new_pm = objP->info.position.mOrient * rotmat;
objP->info.position.mOrient = new_pm;
objP->info.position.mOrient.CheckAndFix();
}

extern void MultiSendDropBlobs (char);
extern void FuelCenCheckForGoal (CSegment *);

//see if CWall is volatile, and if so, cause damage to CPlayerData
//returns true if CPlayerData is in lava
int CheckVolatileWall (CObject *objP, int nSegment, int nSide, CFixVector *hitpt);
int CheckVolatileSegment (CObject *objP, int nSegment);

//	Time at which this CObject last created afterburner blobs.

//--------------------------------------------------------------------
//reset CObject's movement info
//--------------------------------------------------------------------

void StopObjectMovement (CObject *objP)
{
Controls [0].headingTime = 0;
Controls [0].pitchTime = 0;
Controls [0].bankTime = 0;
Controls [0].verticalThrustTime = 0;
Controls [0].sidewaysThrustTime = 0;
Controls [0].forwardThrustTime = 0;
objP->mType.physInfo.rotThrust.SetZero();
objP->mType.physInfo.thrust.SetZero();
objP->mType.physInfo.velocity.SetZero();
objP->mType.physInfo.rotVel.SetZero();
}

//--------------------------------------------------------------------

void StopPlayerMovement (void)
{
if (!gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted) {
	StopObjectMovement (OBJECTS + LOCALPLAYER.nObject);
	memset (&gameData.physics.playerThrust, 0, sizeof (gameData.physics.playerThrust));
//	gameData.time.xFrame = F1_0;
	gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted = 0;
	}
}

//--------------------------------------------------------------------

void RotateCamera (CObject *objP)
{
cameraManager.Rotate (objP);
}

//--------------------------------------------------------------------

void RotateMarker (CObject *objP)
{
if (EGI_FLAG (bRotateMarkers, 0, 1, 0) && gameStates.app.tick40fps.bTick) {
	static time_t	t0 = 0;

	time_t t = (gameStates.app.nSDLTicks - t0) % 1000;
	t0 = gameStates.app.nSDLTicks;
	if (t) {
		CAngleVector a = CAngleVector::Create(0, 0, (fixang) ((float) (F1_0 / 512) * t / 25.0f));
		CFixMatrix mRotate = CFixMatrix::Create(a);
		CFixMatrix mOrient = mRotate * objP->info.position.mOrient;
		objP->info.position.mOrient = mOrient;
		}
	}
}

//--------------------------------------------------------------------

int CObject::CheckWallPhysics (short nSegment, short nSide)
{
	int			bCheck = 1, nType, sideMask, bUnderLavaFall = 0;
	CSegment*	segP = SEGMENTS + nSegment;

	static bool bPlayingSound [MAX_PLAYERS] = {false, false, false, false, false, false, false, false};

if (info.nType != OBJ_PLAYER)
	return 0;
sideMask = segP->SideMasks (info.position.vPos, info.xSize).m_side;
if (sideMask) {
	short		nSide;
	int		bit;
	CSide*	sideP = segP->m_sides;
	for (nSide = 0, bit = 1; nSide < 6; bit <<= 1, nSide++, sideP++) {
		if (!(sideMask & bit))
			continue;
		if ((nType = ApplyWallPhysics (nSegment, nSide)))
			break;
		}
	}
if (!nType)
	nType = CheckSegmentPhysics ();
if (nType) {
	if (!bPlayingSound [info.nId]) {
		short sound = (nType == 1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
		DigiLinkSoundToObject3 (sound, OBJ_IDX (objP), 1, F1_0, I2X (256), -1, -1, NULL, 0, SOUNDCLASS_GENERIC);
		bPlayingSound [info.nId] = 1;
		}
	}
else if (bPlayingSound [info.nId]) {
	DigiKillSoundLinkedToObject (OBJ_IDX (objP));
	bPlayingSound [info.nId] = 0;
	}
return nType;
}

//--------------------------------------------------------------------

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

//--------------------------------------------------------------------

int HandleObjectControl (CObject *objP)
{
switch (objP->info.controlType) {
	case CT_NONE:
		break;

	case CT_FLYING:
		ReadFlyingControls (objP);
		break;

	case CT_REPAIRCEN:
		Int3 ();	// -- hey!these are no longer supported!!-- do_repair_sequence (objP); break;

	case CT_POWERUP:
		DoPowerupFrame (objP);
		break;

	case CT_MORPH:			//morph implies AI
		DoMorphFrame (objP);
		//NOTE: FALLS INTO AI HERE!!!!

	case CT_AI:
		//NOTE LINK TO CT_MORPH ABOVE!!!
		if (gameStates.gameplay.bNoBotAI || (gameStates.app.bGameSuspended & SUSP_ROBOTS)) {
			objP->mType.physInfo.velocity.SetZero();
			objP->mType.physInfo.thrust.SetZero();
			objP->mType.physInfo.rotThrust.SetZero();
			DoAnyRobotDyingFrame (objP);
#if 1//ndef _DEBUG
			return 1;
#endif
			}
		else if (USE_D1_AI)
			DoD1AIFrame (objP);
		else
			DoAIFrame(objP);
		break;

	case CT_CAMERA:
		RotateCamera (objP);
		break;

	case CT_WEAPON:
		LaserDoWeaponSequence (objP);
		break;

	case CT_EXPLOSION:
		DoExplosionSequence (objP);
		break;

	case CT_SLEW:
		break;	//ignore

	case CT_DEBRIS:
		DoDebrisFrame (objP);
		break;

	case CT_LIGHT:
		break;		//doesn't do anything

	case CT_REMOTE:
		break;		//movement is handled in com_process_input

	case CT_CNTRLCEN:
		if (gameStates.gameplay.bNoBotAI)
			return 1;
		DoReactorFrame (objP);
		break;

	default:
#ifdef __DJGPP__
		Error ("Unknown control nType %d in CObject %li, sig/nType/id = %i/%i/%i", objP->info.controlType, OBJ_IDX (objP), objP->info.nSignature, objP->info.nType, objP->info.nId);
#else
		Error ("Unknown control nType %d in CObject %i, sig/nType/id = %i/%i/%i", objP->info.controlType, OBJ_IDX (objP), objP->info.nSignature, objP->info.nType, objP->info.nId);
#endif
	}
return 0;
}

//--------------------------------------------------------------------

void UpdateShipSound (CObject *objP)
{
	int	nSpeed = objP->mType.physInfo.velocity.Mag();
	int	nObject = OBJ_IDX (objP);

if (!gameOpts->sound.bShip)
	return;
if (nObject != LOCALPLAYER.nObject)
	return;
if (gameData.multiplayer.bMoving == nSpeed)
	return;
nSpeed -= 2 * F1_0;
if (nSpeed < 0)
	nSpeed = 0;

if (gameData.multiplayer.bMoving < 0)
	DigiLinkSoundToObject3 (-1, OBJ_IDX (objP), 1, F1_0 / 64 + nSpeed / 256, I2X (256), -1, -1, "missileflight-small.wav", 1, SOUNDCLASS_PLAYER);
else
	DigiChangeSoundLinkedToObject (OBJ_IDX (objP), F1_0 / 64 + nSpeed / 256);
gameData.multiplayer.bMoving = nSpeed;
}

//--------------------------------------------------------------------

void HandleObjectMovement (CObject *objP)
{
if (objP->info.nType == OBJ_MARKER)
	RotateMarker (objP);

switch (objP->info.movementType) {
	case MT_NONE:
		break;								//this doesn't move

	case MT_PHYSICS:
#if DBG
		if (objP->info.nType != OBJ_PLAYER)
			objP = objP;
#endif
		DoPhysicsSim (objP);
		SetDynLightPos (OBJ_IDX (objP));
		lightningManager.MoveForObject (objP);
		if (objP->info.nType == OBJ_PLAYER)
			UpdateShipSound (objP);
		break;	//move by physics

	case MT_SPINNING:
		SpinObject (objP);
		break;
	}
}

//--------------------------------------------------------------------

int CheckObjectHitTriggers (CObject *objP, short nPrevSegment)
{
		short	nConnSide, i;
		int	nOldLevel;

if ((objP->info.movementType != MT_PHYSICS) || (nPrevSegment == objP->info.nSegment))
	return 0;
nOldLevel = gameData.missions.nCurrentLevel;
for (i = 0; i < nPhysSegs - 1; i++) {
#if DBG
	if (physSegList [i] > gameData.segs.nLastSegment)
		PrintLog ("invalid segment in physSegList\n");
#endif
	nConnSide = ConnectedSide (SEGMENTS + physSegList [i+1], SEGMENTS + physSegList [i]);
	if (nConnSide != -1)
		CheckTrigger (SEGMENTS + physSegList [i], nConnSide, OBJ_IDX (objP), 0);
#if DBG
	else	// segments are not directly connected, so do binary subdivision until you find connected segments.
		PrintLog ("UNCONNECTED SEGMENTS %d, %d\n", physSegList [i+1], physSegList [i]);
#endif
	//maybe we've gone on to the next level.  if so, bail!
	if (gameData.missions.nCurrentLevel != nOldLevel)
		return 1;
	}
return 0;
}

//--------------------------------------------------------------------

void CheckGuidedMissileThroughExit (CObject *objP, short nPrevSegment)
{
if ((objP == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP) &&
	 (objP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature)) {
	if (nPrevSegment != objP->info.nSegment) {
		short	nConnSide = ConnectedSide (SEGMENTS + objP->info.nSegment, SEGMENTS + nPrevSegment);
		if (nConnSide != -1) {
			CTrigger* trigP = SEGMENTS [nPrevSegment].Trigger (nConnSide);
			if (trigP && (trigP->nType == TT_EXIT))
				gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP->info.xLifeLeft = 0;
			}
		}
	}
}

//--------------------------------------------------------------------

void CheckAfterburnerBlobDrop (CObject *objP)
{
if (gameStates.render.bDropAfterburnerBlob) {
	Assert (objP == gameData.objs.consoleP);
	DropAfterburnerBlobs (objP, 2, I2X (5) / 2, -1, NULL, 0);	//	-1 means use default lifetime
	if (IsMultiGame)
		MultiSendDropBlobs ((char) gameData.multiplayer.nLocalPlayer);
	gameStates.render.bDropAfterburnerBlob = 0;
	}

if ((objP->info.nType == OBJ_WEAPON) && (gameData.weapons.info [objP->info.nId].afterburner_size)) {
	int	nObject, bSmoke;
	fix	vel;
	fix	delay, lifetime, nSize;

#if 1
	if ((objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->info.nId]) {
		if (SHOW_SMOKE && gameOpts->render.particles.bMissiles)
			return;
		if ((gameStates.app.bNostalgia || EGI_FLAG (bThrusterFlames, 1, 1, 0)) &&
			 (objP->info.nId != MERCURYMSL_ID))
			return;
		}
#endif
	if ((vel = objP->mType.physInfo.velocity.Mag()) > F1_0 * 200)
		delay = F1_0 / 16;
	else if (vel > F1_0 * 40)
		delay = FixDiv (F1_0 * 13, vel);
	else
		delay = DEG90;

	if ((bSmoke = SHOW_SMOKE && gameOpts->render.particles.bMissiles)) {
		nSize = F1_0 * 3;
		lifetime = F1_0 / 12;
		delay = 0;
		}
	else {
		nSize = I2X (gameData.weapons.info [objP->info.nId].afterburner_size) / 16;
		lifetime = 3 * delay / 2;
		if (!IsMultiGame) {
			delay /= 2;
			lifetime *= 2;
			}
		}

	nObject = OBJ_IDX (objP);
	if (bSmoke ||
		 (gameData.objs.xLastAfterburnerTime [nObject] + delay < gameData.time.xGame) ||
		 (gameData.objs.xLastAfterburnerTime [nObject] > gameData.time.xGame)) {
		DropAfterburnerBlobs (objP, 1, nSize, lifetime, NULL, bSmoke);
		gameData.objs.xLastAfterburnerTime [nObject] = gameData.time.xGame;
		}
	}
}

//--------------------------------------------------------------------

void HandleObjectEffects (CObject *objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->info.nId).energyDrain) {
		RequestEffects (objP, ROBOT_LIGHTNINGS);
		}
	}
else if ((objP->info.nType == OBJ_PLAYER) && gameOpts->render.lightnings.bPlayers) {
	int s = SEGMENTS [objP->info.nSegment].m_nType;
	if (s == SEGMENT_IS_FUELCEN) {
		RequestEffects (objP, PLAYER_LIGHTNINGS);
		}
	else if (s == SEGMENT_IS_REPAIRCEN) {
		RequestEffects (objP, PLAYER_LIGHTNINGS);
		}
	else
		RequestEffects (objP, DESTROY_LIGHTNINGS);
	}
}

//--------------------------------------------------------------------
//move an CObject for the current frame

int UpdateObject (CObject * objP)
{
	short	nPrevSegment = (short) objP->info.nSegment;

#if DBG
if ((objP->info.nType == OBJ_WEAPON) && (objP->info.nId == SMARTMINE_BLOB_ID)) {
	nDbgObj = OBJ_IDX (objP);
	nDbgObj = nDbgObj;
	}
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (objP->info.nType == OBJ_ROBOT) {
	if (gameOpts->gameplay.bNoThief && (!IsMultiGame || IsCoopGame) && ROBOTINFO (objP->info.nId).thief) {
		objP->info.xShields = 0;
		objP->info.xLifeLeft = 0;
		objP->Kill ();
		}
	else {
		fix xMaxShields = RobotDefaultShields (objP);
		if (objP->info.xShields > xMaxShields)
			objP->info.xShields = xMaxShields;
		}
	}
objP->info.vLastPos = objP->info.position.vPos;			// Save the current position
objP->HandleSpecialSegment ();
if ((objP->info.xLifeLeft != IMMORTAL_TIME) &&
	 (objP->info.xLifeLeft != ONE_FRAME_TIME) &&
	 (gameData.physics.xTime != F1_0))
	objP->info.xLifeLeft -= (fix) (gameData.physics.xTime / gameStates.gameplay.slowmo [0].fSpeed);		//...inevitable countdown towards death
gameStates.render.bDropAfterburnerBlob = 0;
if (HandleObjectControl (objP)) {
	HandleObjectEffects (objP);
	return 1;
	}
if (objP->info.xLifeLeft < 0) {		// We died of old age
	objP->Kill ();
	if ((objP->info.nType == OBJ_WEAPON) && WI_damage_radius (objP->info.nId))
		ExplodeBadassWeapon (objP, &objP->info.position.vPos);
	else if (objP->info.nType == OBJ_ROBOT)	//make robots explode
		ExplodeObject (objP, 0);
	}
if ((objP->info.nType == OBJ_NONE) || (objP->info.nFlags & OF_SHOULD_BE_DEAD)) {
	return 1;			//CObject has been deleted
	}
HandleObjectMovement (objP);
HandleObjectEffects (objP);
if (CheckObjectHitTriggers (objP, nPrevSegment))
	return 0;
CheckObjectInVolatileWall (objP);
CheckGuidedMissileThroughExit (objP, nPrevSegment);
CheckAfterburnerBlobDrop (objP);
return 1;
}

//--------------------------------------------------------------------
//move all OBJECTS for the current frame
int UpdateAllObjects ()
{
	int i;
	CObject *objP, *nextObjP;

//	CheckDuplicateObjects ();
//	RemoveIncorrectObjects ();
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
	if ((objP->info.nType != OBJ_NONE) && !(objP->info.nFlags & OF_SHOULD_BE_DEAD) && !UpdateObject (objP))
		return 0;
	i++;
	}
return 1;
}

//	-----------------------------------------------------------------------------------------------------------

int ObjectSoundClass (CObject *objP)
{
	short nType = objP->info.nType;

if (nType == OBJ_PLAYER)
	return SOUNDCLASS_PLAYER;
if (nType == OBJ_ROBOT)
	return SOUNDCLASS_ROBOT;
if (nType == OBJ_WEAPON)
	return gameData.objs.bIsMissile [objP->info.nId] ? SOUNDCLASS_MISSILE : SOUNDCLASS_GENERIC;
return SOUNDCLASS_GENERIC;
}

//	-----------------------------------------------------------------------------------------------------------
//make CObject array non-sparse
void compressObjects (void)
{
	int start_i;	//, last_i;

	//last_i = find_last_obj (MAX_OBJECTS);

	//	Note: It's proper to do < (rather than <=) gameData.objs.nLastObject [0] here because we
	//	are just removing gaps, and the last CObject can't be a gap.
	for (start_i = 0;start_i<gameData.objs.nLastObject [0];start_i++)

		if (OBJECTS [start_i].info.nType == OBJ_NONE) {

			int	segnum_copy;

			segnum_copy = OBJECTS [gameData.objs.nLastObject [0]].info.nSegment;
			OBJECTS [gameData.objs.nLastObject [0]].UnlinkFromSeg ();
			OBJECTS [start_i] = OBJECTS [gameData.objs.nLastObject [0]];

			#ifdef EDITOR
			if (CurObject_index == gameData.objs.nLastObject [0])
				CurObject_index = start_i;
			#endif

			OBJECTS [gameData.objs.nLastObject [0]].info.nType = OBJ_NONE;

			OBJECTS [start_i].LinkToSeg (segnum_copy);

			while (OBJECTS [--gameData.objs.nLastObject [0]].info.nType == OBJ_NONE);

			//last_i = find_last_obj (last_i);

		}

	ResetObjects (gameData.objs.nObjects);

}

//------------------------------------------------------------------------------

int ObjectCount (int nType)
{
	int		h = 0, i;
	CObject	*objP = OBJECTS.Buffer ();

FORALL_OBJS (objP, i)
	if (objP->info.nType == nType)
		h++;
return h;
}

//------------------------------------------------------------------------------

void ConvertSmokeObject (CObject *objP)
{
	int			j;
	CTrigger		*trigP;

objP->SetType (OBJ_EFFECT);
objP->info.nId = SMOKE_ID;
objP->info.renderType = RT_SMOKE;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_LIFE, -1);
#if 1
j = (trigP && trigP->value) ? trigP->value : 5;
objP->rType.particleInfo.nLife = (j * (j + 1)) / 2;
#else
objP->rType.particleInfo.nLife = (trigP && trigP->value) ? trigP->value : 5;
#endif
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_BRIGHTNESS, -1);
objP->rType.particleInfo.nBrightness = (trigP && trigP->value) ? trigP->value * 10 : 75;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SPEED, -1);
j = (trigP && trigP->value) ? trigP->value : 5;
#if 1
objP->rType.particleInfo.nSpeed = (j * (j + 1)) / 2;
#else
objP->rType.particleInfo.nSpeed = j;
#endif
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DENS, -1);
objP->rType.particleInfo.nParts = j * ((trigP && trigP->value) ? trigP->value * 50 : STATIC_SMOKE_MAX_PARTS);
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DRIFT, -1);
objP->rType.particleInfo.nDrift = (trigP && trigP->value) ? j * trigP->value * 50 : objP->rType.particleInfo.nSpeed * 50;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SIZE, -1);
j = (trigP && trigP->value) ? trigP->value : 5;
objP->rType.particleInfo.nSize [0] = j + 1;
objP->rType.particleInfo.nSize [1] = (j * (j + 1)) / 2;
}

//------------------------------------------------------------------------------

void ConvertObjects (void)
{
	CObject	*objP;
	int		i;

PrintLog ("   converting deprecated smoke objects\n");
FORALL_STATIC_OBJS (objP, i)
	if (objP->info.nType == OBJ_SMOKE)
		ConvertSmokeObject (objP);
}

//------------------------------------------------------------------------------

void SetupSmokeEffect (CObject *objP)
{
	tParticleInfo	*psi = &objP->rType.particleInfo;
	int			nLife, nSpeed, nParts, nSize;

objP->info.renderType = RT_SMOKE;
nLife = psi->nLife ? psi->nLife : 5;
#if 1
psi->nLife = (nLife * (nLife + 1)) / 2;
#else
psi->nLife = psi->value ? psi->value : 5;
#endif
psi->nBrightness = psi->nBrightness ? psi->nBrightness * 10 : 50;
if (!(nSpeed = psi->nSpeed))
	nSpeed = 5;
#if 1
psi->nSpeed = (nSpeed * (nSpeed + 1)) / 2;
#else
psi->nSpeed = i;
#endif
if (!(nParts = psi->nParts))
	nParts = 5;
psi->nDrift = psi->nDrift ? nSpeed * psi->nDrift * 75 : psi->nSpeed * 50;
nSize = psi->nSize [0] ? psi->nSize [0] : 5;
psi->nSize [0] = nSize + 1;
psi->nSize [1] = (nSize * (nSize + 1)) / 2;
psi->nParts = 90 + (nParts * psi->nLife * 3 * (1 << nSpeed)) / (11 - nParts);
if (psi->nSide > 0) {
	float faceSize = FaceSize (objP->info.nSegment, psi->nSide - 1);
	psi->nParts = (int) (psi->nParts * ((faceSize < 1) ? sqrt (faceSize) : faceSize));
	if (gameData.segs.nLevelVersion >= 18) {
		if (psi->nType == SMOKE_TYPE_SPRAY)
			psi->nParts *= 4;
		}
	else if ((gameData.segs.nLevelVersion < 18) && IsWaterTexture (SEGMENTS [objP->info.nSegment].m_sides [psi->nSide - 1].m_nBaseTex)) {
		psi->nParts *= 4;
		//psi->nSize [1] /= 2;
		}
	}
#if 1
if (psi->nType == SMOKE_TYPE_BUBBLES) {
	psi->nParts *= 2;
	}
#endif
}

//------------------------------------------------------------------------------

void SetupEffects (void)
{
	CObject	*objP;
	int		i;

PrintLog ("   setting up effects\n");
FORALL_EFFECT_OBJS (objP, i) 
	if (objP->info.nId == SMOKE_ID)
		SetupSmokeEffect (objP);
}

//------------------------------------------------------------------------------
//called after load. Takes number of objects, and objects should be
//compressed.  resets free list, marks unused objects as unused
void ResetObjects (int nObjects)
{
	int 		i;
	CObject	*objP;

gameData.objs.nObjects = nObjects;
for (i = 0, objP = OBJECTS.Buffer (); i < nObjects; i++, objP++)
	if (objP->info.nType != OBJ_DEBRIS)
		objP->rType.polyObjInfo.nSubObjFlags = 0;
for (; i < MAX_OBJECTS; i++, objP++) {
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
nDebrisObjectCount = 0;
}

//------------------------------------------------------------------------------
//Tries to find a CSegment for an CObject, using FindSegByPos ()
int FindObjectSeg (CObject * objP)
{
return FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
}

//------------------------------------------------------------------------------
//If an CObject is in a CSegment, set its nSegment field and make sure it's
//properly linked.  If not in any CSegment, returns 0, else 1.
//callers should generally use FindVectorIntersection ()
int UpdateObjectSeg (CObject * objP, bool bMove)
{
	int nNewSeg;

#if DBG
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (0 > (nNewSeg = FindObjectSeg (objP))) {
	if (!bMove)
		return 0;
	nNewSeg = FindClosestSeg (objP->info.position.vPos);
	CSegment* segP = SEGMENTS + nNewSeg;
	CFixVector vOffset = objP->info.position.vPos - segP->Center ();
	CFixVector::Normalize (vOffset);
	objP->info.position.vPos = segP->Center () + vOffset * segP->MinRad ();
	}
if (nNewSeg != objP->info.nSegment)
	objP->RelinkToSeg (nNewSeg);
return 1;
}

//------------------------------------------------------------------------------
//go through all OBJECTS and make sure they have the correct CSegment numbers
void FixObjectSegs (void)
{
	CObject	*objP;
	int		i;

FORALL_OBJS (objP, i) {
	if ((objP->info.nType == OBJ_NONE) || (objP->info.nType == OBJ_CAMBOT) || (objP->info.nType == OBJ_EFFECT))
		continue;
	if (UpdateObjectSeg (objP))
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
	int 		i;
	CObject	*objP = OBJECTS.Buffer ();

FORALL_ROBOT_OBJS (objP, i)
	objP->info.xSize = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
}

//------------------------------------------------------------------------------

//delete OBJECTS, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if bClearAll is set, clear even proximity bombs
void ClearTransientObjects (int bClearAll)
{
	CObject *objP, *nextObjP;

for (objP = gameData.objs.lists.weapons.head; objP; objP = nextObjP) {
	nextObjP = objP->Links (1).next;
	if ((!(gameData.weapons.info [objP->info.nId].flags & WIF_PLACABLE) &&
		  (bClearAll || ((objP->info.nId != PROXMINE_ID) && (objP->info.nId != SMARTMINE_ID)))) ||
			objP->info.nType == OBJ_FIREBALL ||
			objP->info.nType == OBJ_DEBRIS ||
			((objP->info.nType != OBJ_NONE) && (objP->info.nFlags & OF_EXPLODING))) {

#if DBG
#	if TRACE
		if (objP->info.xLifeLeft > I2X (2))
			console.printf (CON_DBG, "Note: Clearing CObject %d (nType=%d, id=%d) with lifeleft=%x\n",
							OBJ_IDX (objP), objP->info.nType, objP->info.nId, objP->info.xLifeLeft);
#	endif
#endif
		ReleaseObject (OBJ_IDX (objP));
		}
	#if DBG
#	if TRACE
		else if ((objP->info.nType != OBJ_NONE) && (objP->info.xLifeLeft < I2X (2)))
		console.printf (CON_DBG, "Note: NOT clearing CObject %d (nType=%d, id=%d) with lifeleft=%x\n",
						OBJ_IDX (objP), objP->info.nType, objP->info.nId,	objP->info.xLifeLeft);
#	endif
#endif
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
void DetachFromParent (CObject *sub)
{
if ((OBJECTS [sub->cType.explInfo.attached.nParent].info.nType != OBJ_NONE) &&
	 (OBJECTS [sub->cType.explInfo.attached.nParent].info.nAttachedObj != -1)) {
	if (sub->cType.explInfo.attached.nNext != -1) {
		OBJECTS [sub->cType.explInfo.attached.nNext].cType.explInfo.attached.nPrev = sub->cType.explInfo.attached.nPrev;
		}
	if (sub->cType.explInfo.attached.nPrev != -1) {
		OBJECTS [sub->cType.explInfo.attached.nPrev].cType.explInfo.attached.nNext =
			sub->cType.explInfo.attached.nNext;
		}
	else {
		OBJECTS [sub->cType.explInfo.attached.nParent].info.nAttachedObj = sub->cType.explInfo.attached.nNext;
		}
	}
sub->cType.explInfo.attached.nNext =
sub->cType.explInfo.attached.nPrev =
sub->cType.explInfo.attached.nParent = -1;
sub->info.nFlags &= ~OF_ATTACHED;
}

//------------------------------------------------------------------------------
//dettaches all OBJECTS from this CObject
void DetachChildObjects (CObject *parent)
{
while (parent->info.nAttachedObj != -1)
	DetachFromParent (OBJECTS + parent->info.nAttachedObj);
}

//------------------------------------------------------------------------------
//creates a marker CObject in the world.  returns the CObject number
int DropMarkerObject (CFixVector *pos, short nSegment, CFixMatrix *orient, ubyte nMarker)
{
	short nObject;

Assert (gameData.models.nMarkerModel != -1);
nObject = CreateObject (OBJ_MARKER, nMarker, -1, nSegment, *pos, *orient,
								gameData.models.polyModels [gameData.models.nMarkerModel].rad, CT_NONE, MT_NONE, RT_POLYOBJ);
if (nObject >= 0) {
	CObject *objP = OBJECTS + nObject;
	objP->rType.polyObjInfo.nModel = gameData.models.nMarkerModel;
	objP->mType.spinRate = objP->info.position.mOrient.UVec () * (F1_0 / 2);
	//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all OBJECTS.
	objP->info.xLifeLeft = IMMORTAL_TIME;
	}
return nObject;
}

//------------------------------------------------------------------------------

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
			if (CFixVector::Dist (viewerP->info.position.vPos, objP->info.position.vPos) < F1_0*100) {
				ailP = gameData.ai.localInfo + nObject;
				if (ailP->playerAwarenessType == 0) {
					objP->cType.aiInfo.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
					ailP->playerAwarenessType = WEAPON_ROBOT_COLLISION;
					ailP->playerAwarenessTime = F1_0*3;
					ailP->nPrevVisibility = 2;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void ResetChildObjects (void)
{
	int	i;

for (i = 0; i < MAX_OBJECTS; i++) {
	gameData.objs.childObjs [i].objIndex = -1;
	gameData.objs.childObjs [i].nextObj = i + 1;
	}
gameData.objs.childObjs [i - 1].nextObj = -1;
gameData.objs.nChildFreeList = 0;
gameData.objs.firstChild.Clear (0xff);
gameData.objs.parentObjs.Clear (0xff);
}

//------------------------------------------------------------------------------

int CheckChildList (int nParent)
{
	int h, i, j;

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

int AddChildObjectN (int nParent, int nChild)
{
	int	h, i;

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

int AddChildObjectP (CObject *pParent, CObject *pChild)
{
return pParent ? AddChildObjectN (OBJ_IDX (pParent), OBJ_IDX (pChild)) : 0;
//return (pParent->nType == OBJ_PLAYER) ? AddChildObjectN (OBJ_IDX (pParent), OBJ_IDX (pChild)) : 0;
}

//------------------------------------------------------------------------------

int DelObjChildrenN (int nParent)
{
	int	i, j;

for (i = gameData.objs.firstChild [nParent]; i >= 0; i = j) {
	j = gameData.objs.childObjs [i].nextObj;
	gameData.objs.childObjs [i].nextObj = gameData.objs.nChildFreeList;
	gameData.objs.childObjs [i].objIndex = -1;
	gameData.objs.nChildFreeList = i;
	}
return 1;
}

//------------------------------------------------------------------------------

int DelObjChildrenP (CObject *pParent)
{
return DelObjChildrenN (OBJ_IDX (pParent));
}

//------------------------------------------------------------------------------

int DelObjChildN (int nChild)
{
	int	nParent, h = -1, i, j;

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

int DelObjChildP (CObject *pChild)
{
return DelObjChildN (OBJ_IDX (pChild));
}

//------------------------------------------------------------------------------

tObjectRef *GetChildObjN (short nParent, tObjectRef *pChildRef)
{
	int i = pChildRef ? pChildRef->nextObj : gameData.objs.firstChild [nParent];

return (i < 0) ? NULL : gameData.objs.childObjs + i;
}

//------------------------------------------------------------------------------

tObjectRef *GetChildObjP (CObject *pParent, tObjectRef *pChildRef)
{
return GetChildObjN (OBJ_IDX (pParent), pChildRef);
}

//------------------------------------------------------------------------------

int CountPlayerObjects (int nPlayer, int nType, int nId)
{
	int		i, h = 0;
	CObject	*objP;

FORALL_OBJS (objP, i) 
	if ((objP->info.nType == nType) && (objP->info.nId == nId) &&
		 (objP->cType.laserInfo.parent.nType == OBJ_PLAYER) &&
		 (OBJECTS [objP->cType.laserInfo.parent.nObject].info.nId == nPlayer))
	h++;
return h;
}

//------------------------------------------------------------------------------

void GetPlayerSpawn (int nSpawnPos, CObject *objP)
{
	CObject	*markerP = SpawnMarkerObject (-1);

if (markerP) {
	objP->info.position = markerP->info.position;
 	objP->RelinkToSeg (markerP->info.nSegment);
	}
else {
	if ((gameData.multiplayer.playerInit [nSpawnPos].nSegment < 0) || 
		 (gameData.multiplayer.playerInit [nSpawnPos].nSegment >= gameData.segs.nSegments))
	GameStartInitNetworkPlayers ();
	objP->info.position = gameData.multiplayer.playerInit [nSpawnPos].position;
 	objP->RelinkToSeg (gameData.multiplayer.playerInit [nSpawnPos].nSegment);
	}
}

//------------------------------------------------------------------------------

CFixVector *PlayerSpawnPos (int nPlayer)
{
	CObject	*markerP = SpawnMarkerObject (nPlayer);

return markerP ? &markerP->info.position.vPos : &gameData.multiplayer.playerInit [nPlayer].position.vPos;
}

//------------------------------------------------------------------------------

CFixMatrix *PlayerSpawnOrient (int nPlayer)
{
	CObject	*markerP = SpawnMarkerObject (nPlayer);

return markerP ? &markerP->info.position.mOrient : &gameData.multiplayer.playerInit [nPlayer].position.mOrient;
}

//------------------------------------------------------------------------------

CFixMatrix *ObjectView (CObject *objP)
{
	tObjectViewData	*viewP = gameData.objs.viewData + OBJ_IDX (objP);

if (viewP->nFrame != gameData.objs.nFrameCount) {
	viewP->mView = OBJPOS (objP)->mOrient.Transpose();
	viewP->nFrame = gameStates.render.nFrameCount;
	}
return &viewP->mView;
}

//------------------------------------------------------------------------------

#define BUILD_ALL_MODELS 0

void BuildObjectModels (void)
{
	int      h, i, j;
	CObject	o, *objP = OBJECTS.Buffer ();
	const char		*pszHires;

if (!RENDERPATH)
	return;
if (!gameData.objs.nLastObject [0])
	return;
PrintLog ("   building optimized polygon model data\n");
gameStates.render.nType = 1;
gameStates.render.nShadowPass = 1;
gameStates.render.bBuildModels = 1;
h = 0;
#if !BUILD_ALL_MODELS
for (i = 0; i <= gameData.objs.nLastObject [0]; i++, objP++) {
	if ((objP->info.nSegment >= 0) && (objP->info.nType != 255) && (objP->info.renderType == RT_POLYOBJ) &&
		 !G3HaveModel (objP->rType.polyObjInfo.nModel)) {
		PrintLog ("      building model %d\n", objP->rType.polyObjInfo.nModel);
#if DBG
		if (objP->rType.polyObjInfo.nModel == nDbgModel)
			nDbgModel = nDbgModel;
#endif
		if (DrawPolygonObject (objP, 1, 0))
			h++;
		}
	}
#endif
memset (&o, 0, sizeof (o));
o.info.nType = OBJ_WEAPON;
o.info.position = OBJECTS [0].info.position;
o.rType.polyObjInfo.nTexOverride = -1;
PrintLog ("   building optimized replacement model data\n");
#if BUILD_ALL_MODELS
j = 0;
for (i = 0; i < MAX_POLYGON_MODELS; i++) {
	o.rType.polyObjInfo.nModel = i; //replacementModels [i].nModel;
#else
j = ReplacementModelCount ();
for (i = 0; i < j; i++)
	if ((pszHires = replacementModels [i].pszHires) && (strstr (pszHires, "laser") == pszHires))
		break;
for (tReplacementModel *rmP = replacementModels + i; i < j; i++, rmP++) {
#endif
	if ((pszHires = rmP->pszHires)) {
		if (strstr (pszHires, "pminepack") == pszHires)
			o.info.nType = OBJ_POWERUP;
		else if (strstr (pszHires, "hostage") == pszHires)
			o.info.nType = OBJ_HOSTAGE;
		}
	o.info.nId = (ubyte) rmP->nId;
	o.rType.polyObjInfo.nModel = rmP->nModel;
	if (!G3HaveModel (o.rType.polyObjInfo.nModel)) {
#if DBG
		if (o.rType.polyObjInfo.nModel == nDbgModel)
			nDbgModel = nDbgModel;
#endif
		PrintLog ("      building model %d (%s)\n", o.rType.polyObjInfo.nModel, pszHires ? pszHires : "n/a");
		if (DrawPolygonObject (&o, 1, 0))
			h++;
		}
	if (o.info.nType == OBJ_HOSTAGE)
		o.info.nType = OBJ_POWERUP;
	}
gameStates.render.bBuildModels = 0;
PrintLog ("   saving optimized polygon model data\n", h);
SaveModelData ();
PrintLog ("   finished building optimized polygon model data (%d models converted)\n", h);
}

//------------------------------------------------------------------------------

inline int CObject::OpenableDoorsInSegment (void)
{
return SEGMENTS [info.nSegment].HasOpenableDoors ();
}

//------------------------------------------------------------------------------

void InitWeaponFlags (void)
{
memset (gameData.objs.bIsMissile, 0, sizeof (gameData.objs.bIsMissile));
gameData.objs.bIsMissile [CONCUSSION_ID] =
gameData.objs.bIsMissile [HOMINGMSL_ID] =
gameData.objs.bIsMissile [SMARTMSL_ID] =
gameData.objs.bIsMissile [MEGAMSL_ID] =
gameData.objs.bIsMissile [FLASHMSL_ID] =
gameData.objs.bIsMissile [GUIDEDMSL_ID] =
gameData.objs.bIsMissile [MERCURYMSL_ID] =
gameData.objs.bIsMissile [EARTHSHAKER_ID] =
gameData.objs.bIsMissile [EARTHSHAKER_MEGA_ID] =
gameData.objs.bIsMissile [ROBOT_CONCUSSION_ID] =
gameData.objs.bIsMissile [ROBOT_HOMINGMSL_ID] =
gameData.objs.bIsMissile [ROBOT_FLASHMSL_ID] =
gameData.objs.bIsMissile [ROBOT_MERCURYMSL_ID] =
gameData.objs.bIsMissile [ROBOT_MEGA_FLASHMSL_ID] =
gameData.objs.bIsMissile [ROBOT_SMARTMSL_ID] =
gameData.objs.bIsMissile [ROBOT_MEGAMSL_ID] =
gameData.objs.bIsMissile [ROBOT_EARTHSHAKER_ID] =
gameData.objs.bIsMissile [ROBOT_SHAKER_MEGA_ID] = 1;

memset (gameData.objs.bIsWeapon, 0, sizeof (gameData.objs.bIsWeapon));
gameData.objs.bIsWeapon [VULCAN_ID] =
gameData.objs.bIsWeapon [GAUSS_ID] =
gameData.objs.bIsWeapon [ROBOT_VULCAN_ID] = 1;
gameData.objs.bIsWeapon [LASER_ID] =
gameData.objs.bIsWeapon [LASER_ID + 1] =
gameData.objs.bIsWeapon [LASER_ID + 2] =
gameData.objs.bIsWeapon [LASER_ID + 3] =
gameData.objs.bIsWeapon [REACTOR_BLOB_ID] =
gameData.objs.bIsWeapon [ROBOT_LIGHT_FIREBALL_ID] =
gameData.objs.bIsWeapon [SMARTMSL_BLOB_ID] =
gameData.objs.bIsWeapon [SMARTMINE_BLOB_ID] =
gameData.objs.bIsWeapon [ROBOT_SMARTMINE_BLOB_ID] =
gameData.objs.bIsWeapon [FLARE_ID] =
gameData.objs.bIsWeapon [SPREADFIRE_ID] =
gameData.objs.bIsWeapon [PLASMA_ID] =
gameData.objs.bIsWeapon [FUSION_ID] =
gameData.objs.bIsWeapon [SUPERLASER_ID] =
gameData.objs.bIsWeapon [SUPERLASER_ID + 1] =
gameData.objs.bIsWeapon [HELIX_ID] =
gameData.objs.bIsWeapon [PHOENIX_ID] =
gameData.objs.bIsWeapon [OMEGA_ID] =
gameData.objs.bIsWeapon [ROBOT_PLASMA_ID] =
gameData.objs.bIsWeapon [ROBOT_MEDIUM_FIREBALL_ID] =
gameData.objs.bIsWeapon [ROBOT_SMARTMSL_BLOB_ID] =
gameData.objs.bIsWeapon [ROBOT_TI_STREAM_ID] =
gameData.objs.bIsWeapon [ROBOT_PHOENIX_ID] =
gameData.objs.bIsWeapon [ROBOT_FAST_PHOENIX_ID] =
gameData.objs.bIsWeapon [ROBOT_PHASE_ENERGY_ID] =
gameData.objs.bIsWeapon [ROBOT_MEGA_FLASHMSL_ID] =
gameData.objs.bIsWeapon [ROBOT_MEGA_FLASHMSL_ID + 1] =
gameData.objs.bIsWeapon [ROBOT_VERTIGO_FIREBALL_ID] =
gameData.objs.bIsWeapon [ROBOT_VERTIGO_PHOENIX_ID] =
gameData.objs.bIsWeapon [ROBOT_HELIX_ID] =
gameData.objs.bIsWeapon [ROBOT_BLUE_ENERGY_ID] =
gameData.objs.bIsWeapon [ROBOT_WHITE_ENERGY_ID] =
gameData.objs.bIsWeapon [ROBOT_BLUE_LASER_ID] =
gameData.objs.bIsWeapon [ROBOT_RED_LASER_ID] =
gameData.objs.bIsWeapon [ROBOT_GREEN_LASER_ID] =
gameData.objs.bIsWeapon [ROBOT_WHITE_LASER_ID] = 1;

memset (gameData.objs.bIsSlowWeapon, 0, sizeof (gameData.objs.bIsSlowWeapon));
gameData.objs.bIsSlowWeapon [VULCAN_ID] =
gameData.objs.bIsSlowWeapon [GAUSS_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_VULCAN_ID] = 1;
gameData.objs.bIsSlowWeapon [REACTOR_BLOB_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_TI_STREAM_ID] =
gameData.objs.bIsSlowWeapon [SMARTMINE_BLOB_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_SMARTMINE_BLOB_ID] =
gameData.objs.bIsSlowWeapon [SMARTMSL_BLOB_ID] =
gameData.objs.bIsSlowWeapon [ROBOT_SMARTMSL_BLOB_ID] = 1;
}

//------------------------------------------------------------------------------
//eof
