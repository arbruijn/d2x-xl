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

void DetachChildObjects (tObject *parent);
void DetachFromParent (tObject *sub);
int FreeObjectSlots (int nRequested);

//info on the various types of OBJECTS
#if DBG
tObject	Object_minus_one;
tObject	*dbgObjP = NULL;
#endif

#ifndef fabsf
#	define fabsf(_f)	(float) fabs (_f)
#endif

int dbgObjInstances = 0;

//------------------------------------------------------------------------------
// grsBitmap *robot_bms [MAX_ROBOT_BITMAPS];	//all bitmaps for all robots

// int robot_bm_nums [MAX_ROBOT_TYPES];		//starting bitmap num for each robot
// int robot_n_bitmaps [MAX_ROBOT_TYPES];		//how many bitmaps for each robot

// char *gameData.bots.names [MAX_ROBOT_TYPES];		//name of each robot

//--unused-- int NumRobotTypes = 0;

int bPrintObjectInfo = 0;
//@@int ObjectViewer = 0;

//tObject * SlewObject = NULL;	// Object containing slew tObject info.

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

float ObjectDamage (tObject *objP)
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
//set viewerP tObject to next tObject in array
void ObjectGotoNextViewer ()
{
	int 		i;
	tObject	*objP;

FORALL_OBJS (objP, i) {
	if (objP->info.nType != OBJ_NONE) {
		gameData.objs.viewerP = objP;
		return;
		}
	}
Error ("Couldn't find a viewerP tObject!");
}

//------------------------------------------------------------------------------
//set viewerP tObject to next tObject in array
void ObjectGotoPrevViewer ()
{
	int 		i, nStartObj = 0;
	tObject	*objP;

nStartObj = OBJ_IDX (gameData.objs.viewerP);		//get viewerP tObject number
FORALL_OBJS (objP, i) {
	if (--nStartObj < 0)
		nStartObj = gameData.objs.nLastObject [0];
	if (OBJECTS [nStartObj].info.nType != OBJ_NONE)	{
		gameData.objs.viewerP = OBJECTS + nStartObj;
		return;
		}
	}
Error ("Couldn't find a viewerP tObject!");
}
#endif

//------------------------------------------------------------------------------

tObject *ObjFindFirstOfType (int nType)
{
	int		i;
	tObject	*objP;

FORALL_OBJS (objP, i)
	if (objP->info.nType == nType)
		return (objP);
return (tObject *) NULL;
}

//------------------------------------------------------------------------------

int ObjReturnNumOfType (int nType)
{
	int		i, count = 0;
	tObject	*objP;

FORALL_OBJS (objP, i)
	if (objP->info.nType == nType)
		count++;
return count;
}

//------------------------------------------------------------------------------

int ObjReturnNumOfTypeAndId (int nType, int id)
{
	int		i, count = 0;
	tObject	*objP;

FORALL_OBJS (objP, i)
	if ((objP->info.nType == nType) && (objP->info.nId == id))
	 count++;
 return count;
 }

//------------------------------------------------------------------------------
// These variables are used to keep a list of the 3 closest robots to the viewerP.
// The code works like this: Every time render tObject is called with a polygon model,
// it finds the distance of that robot to the viewerP.  If this distance if within 10
// segments of the viewerP, it does the following: If there aren't already 3 robots in
// the closet-robots list, it just sticks that tObject into the list along with its distance.
// If the list already contains 3 robots, then it finds the robot in that list that is
// farthest from the viewerP. If that tObject is farther than the tObject currently being
// rendered, then the new tObject takes over that far tObject's slot.  *Then* after all
// OBJECTS are rendered, object_render_targets is called an it draws a target on top
// of all the OBJECTS.

//091494: #define MAX_CLOSE_ROBOTS 3
//--unused-- static int Object_draw_lock_boxes = 0;
//091494: static int Object_num_close = 0;
//091494: static tObject * Object_close_ones [MAX_CLOSE_ROBOTS];
//091494: static fix Object_closeDistance [MAX_CLOSE_ROBOTS];

//091494: set_closeObjects (tObject *objP)
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
//091494: 			// If this tObject is closer to the viewerP than
//091494: 			// the farthest in the list, replace the farthest with this tObject.
//091494: 			if (farthestDistance > dist)	{
//091494: 				Object_close_ones [farthestRobot] = obj;
//091494: 				Object_closeDistance [farthestRobot] = dist;
//091494: 			}
//091494: 		}
//091494: 	}
//091494: }


//	------------------------------------------------------------------------------------------------------------------

void CreateSmallFireballOnObject (tObject *objP, fix size_scale, int bSound)
{
	fix			size;
	vmsVector	pos, rand_vec;
	short			nSegment;

pos = objP->info.position.vPos;
rand_vec = vmsVector::Random();
rand_vec *= (objP->info.xSize / 2);
pos += rand_vec;
size = FixMul (size_scale, F1_0 / 2 + d_rand () * 4 / 2);
nSegment = FindSegByPos (pos, objP->info.nSegment, 1, 0);
if (nSegment != -1) {
	tObject *explObjP = ObjectCreateExplosion (nSegment, &pos, size, VCLIP_SMALL_EXPLOSION);
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

void CreateVClipOnObject (tObject *objP, fix size_scale, ubyte vclip_num)
{
	fix			size;
	vmsVector	pos, rand_vec;
	short			nSegment;

pos = objP->info.position.vPos;
rand_vec = vmsVector::Random();
rand_vec *= (objP->info.xSize / 2);
pos += rand_vec;
size = FixMul (size_scale, F1_0 + d_rand ()*4);
nSegment = FindSegByPos (pos, objP->info.nSegment, 1, 0);
if (nSegment != -1) {
	tObject *explObjP = ObjectCreateExplosion (nSegment, &pos, size, vclip_num);
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
//make object0 the tPlayer, setting all relevant fields
void InitPlayerObject ()
{
SetObjectType (gameData.objs.consoleP, OBJ_PLAYER);
gameData.objs.consoleP->info.nId = 0;					//no sub-types for tPlayer
gameData.objs.consoleP->info.nSignature = 0;			//tPlayer has zero, others start at 1
gameData.objs.consoleP->info.xSize = gameData.models.polyModels [gameData.pig.ship.player->nModel].rad;
gameData.objs.consoleP->info.controlType = CT_SLEW;			//default is tPlayer slewing
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
//sets up the D2_FREE list & init tPlayer & whatever else
void InitObjects (void)
{
	tObject	*objP;
	int		i;

CollideInit ();
for (i = 0, objP = OBJECTS; i < MAX_OBJECTS; i++, objP++) {
	gameData.objs.freeList [i] = i;
	objP->info.nType = OBJ_NONE;
	objP->info.nSegment =
	objP->info.nPrevInSeg =
	objP->info.nNextInSeg =
	objP->cType.explInfo.attached.nNext =
	objP->cType.explInfo.attached.nPrev =
	objP->cType.explInfo.attached.nParent =
	objP->info.nAttachedObj = -1;
	objP->info.nFlags = 0;
	memset (objP->links, 0, sizeof (objP->links));
	}
memset (gameData.segs.objects, 0xff, MAX_SEGMENTS * sizeof (short));
gameData.objs.consoleP =
gameData.objs.viewerP = OBJECTS;
InitPlayerObject ();
LinkObjToSeg (OBJ_IDX (gameData.objs.consoleP), 0);	//put in the world in segment 0
gameData.objs.nObjects = 1;						//just the tPlayer
gameData.objs.nLastObject [0] = 0;
InitIdToOOF ();
}

//------------------------------------------------------------------------------
//after calling InitObject (), the network code has grabbed specific
//object slots without allocating them.  Go through the objects and build
//the free list, then set the appropriate globals
void SpecialResetObjects (void)
{
	tObject	*objP;
	int		i;

gameData.objs.nObjects = MAX_OBJECTS;
gameData.objs.nLastObject [0] = 0;
memset (&gameData.objs.objLists, 0, sizeof (gameData.objs.objLists));
Assert (OBJECTS [0].info.nType != OBJ_NONE);		//0 should be used
for (objP = OBJECTS + MAX_OBJECTS, i = MAX_OBJECTS; i; ) {
	objP--, i--;
#ifdef _DEBUG
	if (i == nDbgObj) {
		nDbgObj = nDbgObj;
		if (objP->info.nType != OBJ_NONE)
			dbgObjInstances++;
		}
#endif
	memset (objP->links, 0, sizeof (objP->links));
	if (objP->info.nType == OBJ_NONE)
		gameData.objs.freeList [--gameData.objs.nObjects] = i;
	else {
		if (i > gameData.objs.nLastObject [0])
			gameData.objs.nLastObject [0] = i;
		LinkObject (objP);
		}
	}
}

//------------------------------------------------------------------------------
#if DBG
int IsObjectInSeg (int nSegment, int objn)
{
	short nObject, count = 0;

for (nObject = gameData.segs.objects [nSegment]; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg)	{
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
	tObject  *objP = OBJECTS + nObject;

Assert (nObject != -1);
if (objP->info.nPrevInSeg == -1)
	gameData.segs.objects [nSegment] = objP->info.nNextInSeg;
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
	for (nObject = gameData.segs.objects [nSegment]; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
		count++;
		#if DBG
		if (count > MAX_OBJECTS)	{
#if TRACE
			con_printf (1, "Object list in tSegment %d is circular.\n", nSegment);
#endif
			Int3 ();
		}
		#endif
		if (OBJECTS [nObject].info.nSegment != nSegment)	{
			#if DBG
#if TRACE
			con_printf (CONDBG, "Removing tObject %d from tSegment %d.\n", nObject, nSegment);
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
	tObject	*objP;

FORALL_OBJS (objP, i) {
	if (objP->info.nType != OBJ_NONE)	{
		count = SearchAllSegsForObject (OBJ_IDX (objP));
		if (count > 1)	{
#if DBG
#	if TRACE
			con_printf (1, "Object %d is in %d segments!\n", OBJ_IDX (objP), count);
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

void list_segObjects (int nSegment)
{
	int nObject, count = 0;

for (nObject = gameData.segs.objects [nSegment]; nObject != -1; nObject = OBJECTS [nObject].info.nNextInSeg) {
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

//link the tObject into the list for its tSegment
void LinkObjToSeg (int nObject, int nSegment)
{
	tObject *objP;

objP = OBJECTS + nObject;
if ((nSegment < 0) || (nSegment >= gameData.segs.nSegments)) {
	nSegment = FindSegByPos (objP->info.position.vPos, 0, 0, 0);
	if (nSegment < 0)
		return;
	}
#ifdef _DEBUG
if (nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
objP->info.nSegment = nSegment;
if (gameData.segs.objects [nSegment] == nObject)
	return;
objP->info.nNextInSeg = gameData.segs.objects [nSegment];
objP->info.nPrevInSeg = -1;
gameData.segs.objects [nSegment] = nObject;
if (objP->info.nNextInSeg != -1)
	OBJECTS [objP->info.nNextInSeg].info.nPrevInSeg = nObject;
if (OBJECTS [0].info.nNextInSeg == 0)
	OBJECTS [0].info.nNextInSeg = -1;
if (OBJECTS [0].info.nPrevInSeg == 0)
	OBJECTS [0].info.nPrevInSeg = -1;
}

//------------------------------------------------------------------------------

CObject::CObject ()
{
Prev () = Next () = NULL;
}

//------------------------------------------------------------------------------

CObject::~CObject ()
{
if (Prev ())
	Prev ()->Next () = Next ();
if (Next ())
	Next ()->Prev () = Next ();
Prev () = Next () = NULL;
}

//------------------------------------------------------------------------------

void CObject::Unlink (void)
{
#if 0
if (Prev () == -1)
	gameData.segs.objects [Segment ()] = Next ();
else
	OBJECTS [Prev ()].Next () = Next ();
if (Next () != -1)
	OBJECTS [Next ()].Prev () = Prev ();

Segment () = Next () = Prev () = -1;
Assert (OBJECTS [0].info.nNextInSeg != 0);
Assert (OBJECTS [0].info.nPrevInSeg != 0);
#endif
}

//------------------------------------------------------------------------------

void UnlinkObjFromSeg (tObject *objP)
{
if (objP->info.nPrevInSeg == -1)
	gameData.segs.objects [objP->info.nSegment] = objP->info.nNextInSeg;
else
	OBJECTS [objP->info.nPrevInSeg].info.nNextInSeg = objP->info.nNextInSeg;
if (objP->info.nNextInSeg != -1)
	OBJECTS [objP->info.nNextInSeg].info.nPrevInSeg = objP->info.nPrevInSeg;

objP->info.nSegment = objP->info.nNextInSeg = objP->info.nPrevInSeg = -1;
}

//------------------------------------------------------------------------------

void CObject::ToStruct (tCoreObject *objP)
{
}

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
//Generally, CObject::Create() should be called to get an tObject, since it
//fills in important fields and does the linking.
//returns -1 if no D2_FREE OBJECTS
int AllocObject (void)
{
	tObject *objP;
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
objP->info.nType = 
objP->nLinkedType = OBJ_NONE;
objP->info.nAttachedObj =
objP->cType.explInfo.attached.nNext =
objP->cType.explInfo.attached.nPrev =
objP->cType.explInfo.attached.nParent = -1;
return nObject;
}

//------------------------------------------------------------------------------

bool ObjIsInList (tObjListRef& ref, tObject *objP, int nLink)
{
	tObject	*listObjP;

for (listObjP = ref.head; listObjP; listObjP = listObjP->links [nLink].next)
	if (listObjP == objP)
		return true;
return false;
}

//------------------------------------------------------------------------------

void LinkObjToList (tObjListRef& ref, tObject *objP, int nLink)
{
	tObjListLink& link = objP->links [nLink];

if (ObjIsInList (ref, objP, nLink)) {
	PrintLog ("object %d, type %d, link %d is already linked\n", objP - gameData.objs.objects, objP->info.nType);
	return;
	}
#ifdef _DEBUG
if (objP - gameData.objs.objects == nDbgObj)
	nDbgObj = nDbgObj;
#endif
link.prev = ref.tail;
if (ref.tail)
	ref.tail->links [nLink].next = objP;
else
	ref.head = objP;
ref.tail = objP;
#if DBG
if (objP->links [nLink].next == objP)
	objP = objP;
if (objP->links [nLink].prev == objP)
	objP = objP;
#endif
ref.nObjects++;
}

//------------------------------------------------------------------------------

void UnlinkObjFromList (tObjListRef& ref, tObject *objP, int nLink)
{
	tObjListLink& link = objP->links [nLink];

if (link.prev || link.next) {
	if (ref.nObjects <= 0)
		return;
	if (link.next)
		link.next->links [nLink].prev = link.prev;
	else
		ref.tail = link.prev;
	if (link.prev)
		link.prev->links [nLink].next = link.next;
	else
		ref.head = link.next;
	link.prev = link.next = NULL;
	ref.nObjects--;
	}
else if ((ref.head == objP) || (ref.tail == objP))
		ref.head = ref.tail = NULL;
if (ObjIsInList (ref, objP, nLink)) {
	PrintLog ("object %d, type %d, link %d is still linked\n", objP - gameData.objs.objects, objP->info.nType);
	return;
	}
#if DBG
if (objP = ref.head) {
	if (objP->links [nLink].next == objP)
		objP = objP;
	if (objP->links [nLink].prev == objP)
		objP = objP;
	}
#endif
}

//------------------------------------------------------------------------------

void LinkObject (tObject *objP)
{
	ubyte nType = objP->info.nType;

#if DBG
if (objP - gameData.objs.objects == nDbgObj) {
	nDbgObj = nDbgObj;
	PrintLog ("linking object #%d, type %d\n", objP - gameData.objs.objects, nType);
	}
#endif
UnlinkObject (objP);
objP->nLinkedType = nType;
LinkObjToList (gameData.objs.objLists.all, objP, 0);
if (nType == OBJ_PLAYER)
	LinkObjToList (gameData.objs.objLists.players, objP, 1);
if (nType == OBJ_ROBOT)
	LinkObjToList (gameData.objs.objLists.robots, objP, 1);
else {
	if (nType == OBJ_WEAPON)
		LinkObjToList (gameData.objs.objLists.weapons, objP, 1);
	else if (nType == OBJ_POWERUP)
		LinkObjToList (gameData.objs.objLists.powerups, objP, 1);
	else if (nType == OBJ_EFFECT)
		LinkObjToList (gameData.objs.objLists.effects, objP, 1);
	else
		objP->links [1].prev = objP->links [1].next = NULL;
	LinkObjToList (gameData.objs.objLists.statics, objP, 2);
	return;
	}
LinkObjToList (gameData.objs.objLists.actors, objP, 2);
}

//------------------------------------------------------------------------------

void UnlinkObject (tObject *objP)
{
	ubyte nType = objP->nLinkedType;

if (nType != OBJ_NONE) {
#if DBG
	if (objP - gameData.objs.objects == nDbgObj) {
		nDbgObj = nDbgObj;
		PrintLog ("unlinking object #%d, type %d\n", objP - gameData.objs.objects, nType);
		}
#endif
	objP->nLinkedType = OBJ_NONE;
	UnlinkObjFromList (gameData.objs.objLists.all, objP, 0);
	if (nType == OBJ_PLAYER)
		UnlinkObjFromList (gameData.objs.objLists.players, objP, 1);
	else if (nType == OBJ_ROBOT)
		UnlinkObjFromList (gameData.objs.objLists.robots, objP, 1);
	else {
		if (nType == OBJ_WEAPON)
			UnlinkObjFromList (gameData.objs.objLists.weapons, objP, 1);
		else if (nType == OBJ_POWERUP)
			UnlinkObjFromList (gameData.objs.objLists.powerups, objP, 1);
		else if (nType == OBJ_EFFECT)
			UnlinkObjFromList (gameData.objs.objLists.effects, objP, 1);
		UnlinkObjFromList (gameData.objs.objLists.statics, objP, 2);
		return;
		}
	UnlinkObjFromList (gameData.objs.objLists.actors, objP, 2);
	}
}

//------------------------------------------------------------------------------

void SetObjectType (tObject *objP, ubyte nNewType)
{
if (objP->info.nType != nNewType) {
	UnlinkObject (objP);
	objP->info.nType = nNewType;
	LinkObject (objP);
	}
}

//------------------------------------------------------------------------------
//frees up an tObject.  Generally, ReleaseObject () should be called to get
//rid of an tObject.  This function deallocates the tObject entry after
//the tObject has been unlinked
void FreeObject (int nObject)
{
	tObject	*objP = OBJECTS + nObject;

#if DBG
if (nObject == nDbgObj) {
	PrintLog ("freeing object #%d\n", nObject);
	nDbgObj = nDbgObj;
	if (dbgObjInstances > 0)
		dbgObjInstances--;
	else
		nDbgObj = nDbgObj;
	}
#endif
UnlinkObject (objP);
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
DestroyObjectLightnings (OBJECTS + nObject);
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
typedef int __fastcall tFreeFilter (tObject *objP);
#else
typedef int tFreeFilter (tObject *objP);
#endif
typedef tFreeFilter *pFreeFilter;


int FreeDebrisFilter (tObject *objP)
{
return objP->info.nType == OBJ_DEBRIS;
}


int FreeFireballFilter (tObject *objP)
{
return (objP->info.nType == OBJ_FIREBALL) && (objP->cType.explInfo.nDeleteObj == -1);
}



int FreeFlareFilter (tObject *objP)
{
return (objP->info.nType == OBJ_WEAPON) && (objP->info.nId == FLARE_ID);
}



int FreeWeaponFilter (tObject *objP)
{
return (objP->info.nType == OBJ_WEAPON);
}

//-----------------------------------------------------------------------------

int FreeCandidates (int *candidateP, int *nCandidateP, int nToFree, pFreeFilter filterP)
{
	tObject	*objP;
	int		h = *nCandidateP, i;

for (i = 0; i < h; ) {
	objP = OBJECTS + candidateP [i];
	if (!filterP (objP))
		i++;
	else {
		if (i < --h)
			candidateP [i] = candidateP [h];
		KillObject (objP);
		if (!--nToFree)
			return 0;
		}
	}
*nCandidateP = h;
return nToFree;
}

//-----------------------------------------------------------------------------
//	Scan the tObject list, freeing down to nRequested OBJECTS
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
	con_printf (1, "Warning: Asked to D2_FREE %i OBJECTS, but can only D2_FREE %i.\n", nToFree, nCandidates);
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

//-----------------------------------------------------------------------------

void CObject::Link (short nSegment)
{
}

//-----------------------------------------------------------------------------

void CObject::Initialize (ubyte nType, ubyte nId, short nCreator, short nSegment, const vmsVector& vPos,
								  const vmsMatrix& mOrient, fix xSize, ubyte cType, ubyte mType, ubyte rType)
{
Signature() = gameData.objs.nNextSignature++;
Type() = nType;
Id() = nId;
LastPos() = Pos () = vPos;
Size() = xSize;
Creator() = (sbyte) nCreator;
Orient() = mOrient;
ControlType() = cType;
MovementType() = mType;
RenderType() = rType;
ContainsType() = -1;
LifeLeft() = 
	((gameData.app.nGameMode & GM_ENTROPY) && (nType == OBJ_POWERUP) && (nId == POW_HOARD_ORB) && (extraGameInfo [1].entropy.nVirusLifespan > 0)) ?
	I2X (extraGameInfo [1].entropy.nVirusLifespan) : IMMORTAL_TIME;
AttachedObj() = -1;
Shields() = 20 * F1_0;
Segment() = -1;					//set to zero by memset, above
Link (nSegment);
}

//-----------------------------------------------------------------------------

int CObject::Create (ubyte nType, ubyte nId, short nCreator, short nSegment, const vmsVector& vPos,
					      const vmsMatrix& mOrient, fix xSize, ubyte cType, ubyte mType, ubyte rType,
						   int bIgnoreLimits)
{
return -1;
}

//-----------------------------------------------------------------------------
//initialize a new tObject.  adds to the list for the given tSegment
//note that nSegment is really just a suggestion, since this routine actually
//searches for the correct tSegment
//returns the tObject number

int CreateObject (ubyte nType, ubyte nId, short nCreator, short nSegment, const vmsVector& vPos, const vmsMatrix& mOrient,
						fix xSize, ubyte cType, ubyte mType, ubyte rType)
{
	short		nObject;
	tObject	*objP;

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

//if (GetSegMasks (vPos, nSegment, 0).centerMask))
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

// Init physics info for this tObject
if (objP->info.movementType == MT_PHYSICS)
	gameData.objs.vStartVel [nObject].SetZero();
if (objP->info.renderType == RT_POLYOBJ)
	objP->rType.polyObjInfo.nTexOverride = -1;
gameData.objs.xCreationTime [nObject] = gameData.time.xGame;

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

LinkObject (objP);
LinkObjToSeg (nObject, nSegment);
return nObject;
}

//------------------------------------------------------------------------------

int CloneObject (tObject *objP)
{
	short		nObject, nSegment;
	tObject	*cloneP;
	
if (0 > (nObject = AllocObject ()))
	return -1;
cloneP = OBJECTS + nObject;
memcpy (cloneP, objP, sizeof (tObject));
cloneP->info.nCreator = -1;
cloneP->mType.physInfo.thrust.SetZero();
gameData.objs.xCreationTime [nObject] = gameData.time.xGame;
nSegment = objP->info.nSegment;
cloneP->info.nSegment = cloneP->info.nPrevInSeg = cloneP->info.nNextInSeg = -1;
memset (cloneP->links, 0, sizeof (cloneP->links));
cloneP->nLinkedType = OBJ_NONE;
LinkObject (objP);
LinkObjToSeg (nObject, nSegment);
return nObject;
}

//------------------------------------------------------------------------------

int CreateRobot (ubyte nId, short nSegment, const vmsVector& vPos)
{
return CreateObject (OBJ_ROBOT, nId, -1, nSegment, vPos, vmsMatrix::IDENTITY, gameData.models.polyModels [ROBOTINFO (nId).nModel].rad, 
							CT_AI, MT_PHYSICS, RT_POLYOBJ);
}

//------------------------------------------------------------------------------

int CreatePowerup (ubyte nId, short nCreator, short nSegment, const vmsVector& vPos, int bIgnoreLimits)
{
if (!bIgnoreLimits && TooManyPowerups ((int) nId)) {
#if 0//def _DEBUG
	HUDInitMessage ("%c%c%c%cDiscarding excess %s!", 1, 127 + 128, 64 + 128, 128, pszPowerup [nId]);
	TooManyPowerups (nId);
#endif
	return -2;
	}
short nObject = CreateObject (OBJ_POWERUP, nId, nCreator, nSegment, vPos, vmsMatrix::IDENTITY, gameData.objs.pwrUp.info [nId].size, 
										CT_POWERUP, MT_PHYSICS, RT_POWERUP);
if ((nObject >= 0) && IsMultiGame && PowerupClass (nId)) {
	gameData.multiplayer.powerupsInMine [(int) nId]++;
	if (MultiPowerupIs4Pack (nId))
		gameData.multiplayer.powerupsInMine [(int) nId - 1] += 4;
	}
return nObject;
}

//------------------------------------------------------------------------------

int CreateWeapon (ubyte nId, short nCreator, short nSegment, const vmsVector& vPos, fix xSize, ubyte rType)
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
			Error ("Invalid weapon render nType in CreateNewLaser\n");
			return -1;
		}
	}
return CreateObject (OBJ_WEAPON, nId, nCreator, nSegment, vPos, vmsMatrix::IDENTITY, xSize, CT_WEAPON, MT_PHYSICS, rType); 
}

//------------------------------------------------------------------------------

int CreateFireball (ubyte nId, short nSegment, const vmsVector& vPos, fix xSize, ubyte rType)
{
return CreateObject (OBJ_FIREBALL, nId, -1, nSegment, vPos, vmsMatrix::IDENTITY, xSize, CT_EXPLOSION, MT_NONE, rType);
}

//------------------------------------------------------------------------------

int CreateDebris (tObject *parentP, short nSubModel)
{
return CreateObject (OBJ_DEBRIS, 0, -1, parentP->info.nSegment, parentP->info.position.vPos, parentP->info.position.mOrient,
							gameData.models.polyModels [parentP->rType.polyObjInfo.nModel].subModels.rads [nSubModel],
							CT_DEBRIS, MT_PHYSICS, RT_POLYOBJ);
}

//------------------------------------------------------------------------------

int CreateCamera (tObject *parentP)
{
return CreateObject (OBJ_CAMERA, 0, -1, parentP->info.nSegment, parentP->info.position.vPos, parentP->info.position.mOrient, 0, 
							CT_NONE, MT_NONE, RT_NONE);
}

//------------------------------------------------------------------------------

int CreateLight (ubyte nId, short nSegment, const vmsVector& vPos)
{
return CreateObject (OBJ_LIGHT, nId, -1, nSegment, vPos, vmsMatrix::IDENTITY, 0, CT_LIGHT, MT_NONE, RT_NONE);
}

//------------------------------------------------------------------------------

#ifdef EDITOR
//create a copy of an tObject. returns new tObject number
int ObjectCreateCopy (int nObject, vmsVector *new_pos, int nNewSegnum)
{
	tObject *objP;
	int newObjNum = AllocObject ();

// Find next D2_FREE tObject
if (newObjNum == -1)
	return -1;
objP = OBJECTS + newObjNum;
*objP = OBJECTS [nObject];
objP->info.position.vPos = objP->info.vLastPos = *new_pos;
objP->info.nNextInSeg = objP->info.nPrevInSeg = objP->info.nSegment = -1;
LinkObject (objP);
LinkObjToSeg (newObjNum, nNewSegnum);
objP->info.nSignature = gameData.objs.nNextSignature++;
//we probably should initialize sub-structures here
return newObjNum;
}
#endif

//------------------------------------------------------------------------------

void NDRecordGuidedEnd (void);

//remove tObject from the world
void ReleaseObject (short nObject)
{
	int nParent;
	tObject *objP = OBJECTS + nObject;

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
	gameData.objs.viewerP = gameData.objs.consoleP;						//..make the tPlayer the viewerP
if (objP->info.nFlags & OF_ATTACHED)		//detach this from tObject
	DetachFromParent (objP);
if (objP->info.nAttachedObj != -1)		//detach all OBJECTS from this
	DetachChildObjects (objP);
if (objP->info.nType == OBJ_DEBRIS)
	nDebrisObjectCount--;
UnlinkObjFromSeg (OBJECTS + nObject);
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

tObject	*viewerSaveP;
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
SetObjectType (gameData.objs.consoleP, OBJ_PLAYER);
gameData.objs.consoleP->info.nFlags = nPlayerFlagsSave;
Assert ((nControlTypeSave == CT_FLYING) || (nControlTypeSave == CT_SLEW));
gameData.objs.consoleP->info.controlType = nControlTypeSave;
gameData.objs.consoleP->info.renderType = nRenderTypeSave;
LOCALPLAYER.flags &= ~PLAYER_FLAGS_INVULNERABLE;
gameStates.app.bPlayerEggsDropped = 0;
}

//------------------------------------------------------------------------------

//	Camera is less than size of tPlayer away from
void SetCameraPos (vmsVector *vCameraPos, tObject *objP)
{
	vmsVector	vPlayerCameraOffs = *vCameraPos - objP->info.position.vPos;
	int			count = 0;
	fix			xCameraPlayerDist;
	fix			xFarScale;

xCameraPlayerDist = vPlayerCameraOffs.Mag();
if (xCameraPlayerDist < xCameraToPlayerDistGoal) { // 2*objP->info.xSize) {
	//	Camera is too close to tPlayer tObject, so move it away.
	tFVIQuery	fq;
	tFVIData		hit_data;
	vmsVector	local_p1;

	if (vPlayerCameraOffs.IsZero())
		vPlayerCameraOffs[X] += F1_0/16;

	hit_data.hit.nType = HIT_WALL;
	xFarScale = F1_0;

	while ((hit_data.hit.nType != HIT_NONE) && (count++ < 6)) {
		vmsVector	closer_p1;
		vmsVector::Normalize(vPlayerCameraOffs);
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
			vPlayerCameraOffs = vmsVector::Random();
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
	vmsVector	fVec;

if (gameStates.app.bPlayerIsDead) {
	xTimeDead = gameData.time.xGame - gameStates.app.nPlayerTimeOfDeath;

	//	If unable to create camera at time of death, create now.
	if (!gameData.objs.deadPlayerCamera) {
		tObject *playerP = OBJECTS + LOCALPLAYER.nObject;
		int nObject = CreateCamera (playerP);
		if (nObject != -1)
			gameData.objs.viewerP = gameData.objs.deadPlayerCamera = OBJECTS + nObject;
		else
			Int3 ();
		}
	h = DEATH_SEQUENCE_EXPLODE_TIME - xTimeDead;
	h = max (0, h);
	gameData.objs.consoleP->mType.physInfo.rotVel = vmsVector::Create(h / 4, h / 2, h / 3);
	xCameraToPlayerDistGoal = min (xTimeDead * 8, F1_0 * 20) + gameData.objs.consoleP->info.xSize;
	SetCameraPos (&gameData.objs.deadPlayerCamera->info.position.vPos, gameData.objs.consoleP);
	fVec = gameData.objs.consoleP->info.position.vPos - gameData.objs.deadPlayerCamera->info.position.vPos;
/*
	gameData.objs.deadPlayerCamera->position.mOrient = vmsMatrix::Create(fVec, NULL, NULL);
*/
	// TODO: MatrixCreateFCheck
	gameData.objs.deadPlayerCamera->info.position.mOrient = vmsMatrix::CreateF(fVec);

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
			gameData.objs.consoleP->info.nFlags &= ~OF_SHOULD_BE_DEAD;		//don't really kill tPlayer
			gameData.objs.consoleP->info.renderType = RT_NONE;				//..just make him disappear
			SetObjectType (gameData.objs.consoleP, OBJ_GHOST);						//..and kill intersections
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

void StartPlayerDeathSequence (tObject *playerP)
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
gameStates.ogl.palAdd.red = 40;
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
PALETTE_FLASH_SET (0, 0, 0);
}

//------------------------------------------------------------------------------

void CleanupObjects (void)
{
	tObject	*objP, *nextObjP = NULL;
	int		nLocalDeadPlayerObj = -1;

for (objP = gameData.objs.objLists.all.head; objP; objP = nextObjP) {
	nextObjP = objP->links [0].next;
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
//when an tObject has moved into a new tSegment, this function unlinks it
//from its old tSegment, and links it into the new tSegment
void RelinkObjToSeg (int nObject, int nNewSegnum)
{
Assert ((nObject >= 0) && (nObject <= gameData.objs.nLastObject [0]));
Assert ((nNewSegnum <= gameData.segs.nLastSegment) && (nNewSegnum >= 0));
UnlinkObjFromSeg (OBJECTS + nObject);
LinkObjToSeg (nObject, nNewSegnum);
#if DBG
#if TRACE
if (GetSegMasks (OBJECTS [nObject].info.position.vPos,
					  OBJECTS [nObject].info.nSegment, 0).centerMask)
	con_printf (1, "RelinkObjToSeg violates seg masks.\n");
#endif
#endif
}

//--------------------------------------------------------------------
//process a continuously-spinning tObject
void SpinObject (tObject *objP)
{
	vmsAngVec rotangs;
	vmsMatrix rotmat, new_pm;

Assert (objP->info.movementType == MT_SPINNING);
rotangs = vmsAngVec::Create((fixang) FixMul (objP->mType.spinRate[X], gameData.time.xFrame),
                            (fixang) FixMul (objP->mType.spinRate[Y], gameData.time.xFrame),
                            (fixang) FixMul (objP->mType.spinRate[Z], gameData.time.xFrame));
rotmat = vmsMatrix::Create(rotangs);
// TODO MM
new_pm = objP->info.position.mOrient * rotmat;
objP->info.position.mOrient = new_pm;
objP->info.position.mOrient.CheckAndFix();
}

extern void MultiSendDropBlobs (char);
extern void FuelCenCheckForGoal (tSegment *);

//see if tWall is volatile, and if so, cause damage to tPlayer
//returns true if tPlayer is in lava
int CheckVolatileWall (tObject *objP, int nSegment, int nSide, vmsVector *hitpt);
int CheckVolatileSegment (tObject *objP, int nSegment);

//	Time at which this tObject last created afterburner blobs.

//--------------------------------------------------------------------
//reset tObject's movement info
//--------------------------------------------------------------------

void StopObjectMovement (tObject *objP)
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

void RotateCamera (tObject *objP)
{

#define	DEG90		 (F1_0 / 4)
#define	DEG45		 (F1_0 / 8)
#define	DEG1		 (F1_0 / (4 * 90))

	tCamera	*pc = gameData.cameras.cameras + gameData.objs.cameraRef [OBJ_IDX (objP)];
	fixang	curAngle = pc->curAngle;
	fixang	curDelta = pc->curDelta;

#if 1
	time_t	t0 = pc->t0;
	time_t	t = gameStates.app.nSDLTicks;

if ((t0 < 0) || (t - t0 >= 1000 / 90))
#endif
	if (objP->cType.aiInfo.behavior == AIB_NORMAL) {
		vmsAngVec	a;
		vmsMatrix	r;

		int	h = abs (curDelta);
		int	d = DEG1 / (gameOpts->render.cameras.nSpeed / 1000);
		int	s = h ? curDelta / h : 1;

	if (h != d)
		curDelta = s * d;
#if 1
	objP->mType.physInfo.brakes = (fix) t;
#endif
	if ((curAngle >= DEG45) || (curAngle <= -DEG45)) {
		if (curAngle * s - DEG45 >= curDelta * s)
			curAngle = s * DEG45 + curDelta - s;
		curDelta = -curDelta;
		}

	curAngle += curDelta;
	a[HA] = curAngle;
	a[BA] =	a[PA] = 0;
	r = vmsMatrix::Create(a);
	// TODO MM
	objP->info.position.mOrient = pc->orient * r;
	pc->curAngle = curAngle;
	pc->curDelta = curDelta;
	}
}

//--------------------------------------------------------------------

void RotateMarker (tObject *objP)
{
if (EGI_FLAG (bRotateMarkers, 0, 1, 0) && gameStates.app.tick40fps.bTick) {
	static time_t	t0 = 0;

	time_t t = (gameStates.app.nSDLTicks - t0) % 1000;
	t0 = gameStates.app.nSDLTicks;
	if (t) {
		vmsAngVec a = vmsAngVec::Create(0, 0, (fixang) ((float) (F1_0 / 512) * t / 25.0f));
		vmsMatrix mRotate = vmsMatrix::Create(a);
		vmsMatrix mOrient = mRotate * objP->info.position.mOrient;
		objP->info.position.mOrient = mOrient;
		}
	}
}

//--------------------------------------------------------------------

void CheckObjectInVolatileWall (tObject *objP)
{
	int bChkVolaSeg = 1, nType, sideMask, bUnderLavaFall = 0;
	static int nLavaFallHissPlaying [MAX_PLAYERS]={0};

if (objP->info.nType != OBJ_PLAYER)
	return;
sideMask = GetSegMasks (objP->info.position.vPos, objP->info.nSegment, objP->info.xSize).sideMask;
if (sideMask) {
	short nSide, nWall;
	int bit;
	tSide *pSide = gameData.segs.segments [objP->info.nSegment].sides;
	for (nSide = 0, bit = 1; nSide < 6; bit <<= 1, nSide++, pSide++) {
		if (!(sideMask & bit))
			continue;
		nWall = pSide->nWall;
		if (!IS_WALL (nWall))
			continue;
		if (gameData.walls.walls [nWall].nType != WALL_ILLUSION)
			continue;
		if ((nType = CheckVolatileWall (objP, objP->info.nSegment, nSide, &objP->info.position.vPos))) {
			short sound = (nType==1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
			bUnderLavaFall = 1;
			bChkVolaSeg = 0;
			if (!nLavaFallHissPlaying [objP->info.nId]) {
				DigiLinkSoundToObject3 (sound, OBJ_IDX (objP), 1, F1_0, I2X (256), -1, -1, NULL, 0, SOUNDCLASS_GENERIC);
				nLavaFallHissPlaying [objP->info.nId] = 1;
				}
			}
		}
	}
if (bChkVolaSeg) {
	if ((nType = CheckVolatileSegment (objP, objP->info.nSegment))) {
		short sound = (nType==1) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
		bUnderLavaFall = 1;
		if (!nLavaFallHissPlaying [objP->info.nId]) {
			DigiLinkSoundToObject3 (sound, OBJ_IDX (objP), 1, F1_0, I2X (256), -1, -1, NULL, 0, SOUNDCLASS_GENERIC);
			nLavaFallHissPlaying [objP->info.nId] = 1;
			}
		}
	}
if (!bUnderLavaFall && nLavaFallHissPlaying [objP->info.nId]) {
	DigiKillSoundLinkedToObject (OBJ_IDX (objP));
	nLavaFallHissPlaying [objP->info.nId] = 0;
	}
}

//--------------------------------------------------------------------

void HandleSpecialSegments (tObject *objP)
{
	fix fuel, shields;
	tSegment *segP = gameData.segs.segments + objP->info.nSegment;
	xsegment *xsegP = gameData.segs.xSegments + objP->info.nSegment;
	tPlayer *playerPP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;

if ((objP->info.nType == OBJ_PLAYER) && (gameData.multiplayer.nLocalPlayer == objP->info.nId)) {
   if (gameData.app.nGameMode & GM_CAPTURE)
		 FuelCenCheckForGoal (segP);
   else if (gameData.app.nGameMode & GM_HOARD)
		 FuelCenCheckForHoardGoal (segP);
	else if (Controls [0].forwardThrustTime || Controls [0].verticalThrustTime || Controls [0].sidewaysThrustTime) {
		gameStates.entropy.nTimeLastMoved = -1;
		if ((gameData.app.nGameMode & GM_ENTROPY) &&
			 ((xsegP->owner < 0) || (xsegP->owner == GetTeam (gameData.multiplayer.nLocalPlayer) + 1))) {
			StopConquerWarning ();
			}
		}
	else if (gameStates.entropy.nTimeLastMoved < 0)
		gameStates.entropy.nTimeLastMoved = 0;
	shields = HostileRoomDamageShields (segP, playerPP->shields + 1);
	if (shields > 0) {
		playerPP->shields -= shields;
		MultiSendShields ();
		if (playerPP->shields < 0)
			StartPlayerDeathSequence (objP);
		else
			CheckConquerRoom (xsegP);
		}
	else {
		StopConquerWarning ();
		fuel = FuelCenGiveFuel (segP, INITIAL_ENERGY - playerPP->energy);
		if (fuel > 0)
			playerPP->energy += fuel;
		shields = RepairCenGiveShields (segP, INITIAL_SHIELDS - playerPP->shields);
		if (shields > 0) {
			playerPP->shields += shields;
			MultiSendShields ();
			}
		if (!xsegP->owner)
			CheckConquerRoom (xsegP);
		}
	}
}

//--------------------------------------------------------------------

int HandleObjectControl (tObject *objP)
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
#if DBG
		if (gameStates.input.keys.pressed [KEY_PAD5])
			slew_stop ();
		if (gameStates.input.keys.pressed [KEY_NUMLOCK]) {
			slew_reset_orient ();
			* (ubyte *) 0x417 &= ~0x20;		//kill numlock
		}
		slew_frame (0);		// Does velocity addition for us.
#endif
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
		Error ("Unknown control nType %d in tObject %li, sig/nType/id = %i/%i/%i", objP->info.controlType, OBJ_IDX (objP), objP->info.nSignature, objP->info.nType, objP->info.nId);
#else
		Error ("Unknown control nType %d in tObject %i, sig/nType/id = %i/%i/%i", objP->info.controlType, OBJ_IDX (objP), objP->info.nSignature, objP->info.nType, objP->info.nId);
#endif
	}
return 0;
}

//--------------------------------------------------------------------

void UpdateShipSound (tObject *objP)
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

void HandleObjectMovement (tObject *objP)
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
		MoveObjectLightnings (objP);
		if (objP->info.nType == OBJ_PLAYER)
			UpdateShipSound (objP);
		break;	//move by physics

	case MT_SPINNING:
		SpinObject (objP);
		break;
	}
}

//--------------------------------------------------------------------

int CheckObjectHitTriggers (tObject *objP, short nPrevSegment)
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
	nConnSide = FindConnectedSide (gameData.segs.segments + physSegList [i+1], gameData.segs.segments + physSegList [i]);
	if (nConnSide != -1)
		CheckTrigger (gameData.segs.segments + physSegList [i], nConnSide, OBJ_IDX (objP), 0);
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

void CheckGuidedMissileThroughExit (tObject *objP, short nPrevSegment)
{
if ((objP == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP) &&
	 (objP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature)) {
	if (nPrevSegment != objP->info.nSegment) {
		short	nConnSide = FindConnectedSide (gameData.segs.segments + objP->info.nSegment, gameData.segs.segments + nPrevSegment);
		if (nConnSide != -1) {
			short nWall, nTrigger;
			nWall = WallNumI (nPrevSegment, nConnSide);
			if (IS_WALL (nWall)) {
				nTrigger = gameData.walls.walls [nWall].nTrigger;
				if ((nTrigger < gameData.trigs.nTriggers) &&
					 (gameData.trigs.triggers [nTrigger].nType == TT_EXIT))
					gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP->info.xLifeLeft = 0;
				}
			}
		}
	}
}

//--------------------------------------------------------------------

void CheckAfterburnerBlobDrop (tObject *objP)
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
		if (SHOW_SMOKE && gameOpts->render.smoke.bMissiles)
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

	if ((bSmoke = SHOW_SMOKE && gameOpts->render.smoke.bMissiles)) {
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

void HandleObjectEffects (tObject *objP)
{
if (objP->info.nType == OBJ_ROBOT) {
	if (ROBOTINFO (objP->info.nId).energyDrain) {
		RequestEffects (objP, ROBOT_LIGHTNINGS);
		}
	}
else if ((objP->info.nType == OBJ_PLAYER) && gameOpts->render.lightnings.bPlayers) {
	int s = gameData.segs.segment2s [objP->info.nSegment].special;
	if (s == SEGMENT_IS_FUELCEN) {
		RequestEffects (objP, PLAYER_LIGHTNINGS);
		}
	else if (s == SEGMENT_IS_REPAIRCEN) {
		RequestEffects (objP, PLAYER_LIGHTNINGS);
		}
	else
#if 1
		RequestEffects (objP, DESTROY_LIGHTNINGS);
#else
		DestroyObjectLightnings (objP);
#endif
	}
}

//--------------------------------------------------------------------
//move an tObject for the current frame

int UpdateObject (tObject * objP)
{
	short	nPrevSegment = (short) objP->info.nSegment;

#if DBG
if (OBJ_IDX (objP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (objP->info.nType == OBJ_ROBOT) {
	if (gameOpts->gameplay.bNoThief && (!IsMultiGame || IsCoopGame) && ROBOTINFO (objP->info.nId).thief) {
		objP->info.xShields = 0;
		objP->info.xLifeLeft = 0;
		KillObject (objP);
		}
	else {
		fix xMaxShields = RobotDefaultShields (objP);
		if (objP->info.xShields > xMaxShields)
			objP->info.xShields = xMaxShields;
		}
	}
objP->info.vLastPos = objP->info.position.vPos;			// Save the current position
HandleSpecialSegments (objP);
if ((objP->info.xLifeLeft != IMMORTAL_TIME) &&
	 (objP->info.xLifeLeft != ONE_FRAME_TIME)&&
	 (gameData.physics.xTime != F1_0))
	objP->info.xLifeLeft -= (fix) (gameData.physics.xTime / gameStates.gameplay.slowmo [0].fSpeed);		//...inevitable countdown towards death
gameStates.render.bDropAfterburnerBlob = 0;
if (HandleObjectControl (objP)) {
	HandleObjectEffects (objP);
	return 1;
	}
if (objP->info.xLifeLeft < 0) {		// We died of old age
	KillObject (objP);
	if ((objP->info.nType == OBJ_WEAPON) && WI_damage_radius (objP->info.nId))
		ExplodeBadassWeapon (objP, &objP->info.position.vPos);
	else if (objP->info.nType == OBJ_ROBOT)	//make robots explode
		ExplodeObject (objP, 0);
	}
if ((objP->info.nType == OBJ_NONE) || (objP->info.nFlags & OF_SHOULD_BE_DEAD)) {
	return 1;			//tObject has been deleted
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
	tObject *objP, *nextObjP;

//	CheckDuplicateObjects ();
//	RemoveIncorrectObjects ();
gameData.objs.nFrameCount++;
if (gameData.objs.nLastObject [0] > gameData.objs.nMaxUsedObjects)
	FreeObjectSlots (gameData.objs.nMaxUsedObjects);		//	Free all possible tObject slots.
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
for (objP = gameData.objs.objLists.all.head; objP; objP = nextObjP) {
	nextObjP = objP->links [0].next;
	if ((objP->info.nType != OBJ_NONE) && !(objP->info.nFlags & OF_SHOULD_BE_DEAD) && !UpdateObject (objP))
		return 0;
	i++;
	}
return 1;
}

//	-----------------------------------------------------------------------------------------------------------

int ObjectSoundClass (tObject *objP)
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
//make tObject array non-sparse
void compressObjects (void)
{
	int start_i;	//, last_i;

	//last_i = find_last_obj (MAX_OBJECTS);

	//	Note: It's proper to do < (rather than <=) gameData.objs.nLastObject [0] here because we
	//	are just removing gaps, and the last tObject can't be a gap.
	for (start_i = 0;start_i<gameData.objs.nLastObject [0];start_i++)

		if (OBJECTS [start_i].info.nType == OBJ_NONE) {

			int	segnum_copy;

			segnum_copy = OBJECTS [gameData.objs.nLastObject [0]].info.nSegment;
			UnlinkObjFromSeg (OBJECTS + gameData.objs.nLastObject [0]);
			OBJECTS [start_i] = OBJECTS [gameData.objs.nLastObject [0]];

			#ifdef EDITOR
			if (CurObject_index == gameData.objs.nLastObject [0])
				CurObject_index = start_i;
			#endif

			OBJECTS [gameData.objs.nLastObject [0]].info.nType = OBJ_NONE;

			LinkObjToSeg (start_i, segnum_copy);

			while (OBJECTS [--gameData.objs.nLastObject [0]].info.nType == OBJ_NONE);

			//last_i = find_last_obj (last_i);

		}

	ResetObjects (gameData.objs.nObjects);

}

//------------------------------------------------------------------------------

int ObjectCount (int nType)
{
	int		h = 0, i;
	tObject	*objP = OBJECTS;

FORALL_OBJS (objP, i)
	if (objP->info.nType == nType)
		h++;
return h;
}

//------------------------------------------------------------------------------

void ConvertSmokeObject (tObject *objP)
{
	int			j;
	tTrigger		*trigP;

SetObjectType (objP, OBJ_EFFECT);
objP->info.nId = SMOKE_ID;
objP->info.renderType = RT_SMOKE;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_LIFE, -1);
#if 1
j = (trigP && trigP->value) ? trigP->value : 5;
objP->rType.smokeInfo.nLife = (j * (j + 1)) / 2;
#else
objP->rType.smokeInfo.nLife = (trigP && trigP->value) ? trigP->value : 5;
#endif
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_BRIGHTNESS, -1);
objP->rType.smokeInfo.nBrightness = (trigP && trigP->value) ? trigP->value * 10 : 75;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SPEED, -1);
j = (trigP && trigP->value) ? trigP->value : 5;
#if 1
objP->rType.smokeInfo.nSpeed = (j * (j + 1)) / 2;
#else
objP->rType.smokeInfo.nSpeed = j;
#endif
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DENS, -1);
objP->rType.smokeInfo.nParts = j * ((trigP && trigP->value) ? trigP->value * 50 : STATIC_SMOKE_MAX_PARTS);
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_DRIFT, -1);
objP->rType.smokeInfo.nDrift = (trigP && trigP->value) ? j * trigP->value * 50 : objP->rType.smokeInfo.nSpeed * 50;
trigP = FindObjTrigger (OBJ_IDX (objP), TT_SMOKE_SIZE, -1);
j = (trigP && trigP->value) ? trigP->value : 5;
objP->rType.smokeInfo.nSize [0] = j + 1;
objP->rType.smokeInfo.nSize [1] = (j * (j + 1)) / 2;
}

//------------------------------------------------------------------------------

void ConvertObjects (void)
{
	tObject	*objP;
	int		i;

PrintLog ("   converting deprecated smoke objects\n");
FORALL_STATIC_OBJS (objP, i)
	if (objP->info.nType == OBJ_SMOKE)
		ConvertSmokeObject (objP);
}

//------------------------------------------------------------------------------

void SetupSmokeEffect (tObject *objP)
{
	tSmokeInfo	*psi = &objP->rType.smokeInfo;
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
psi->nParts = 90 + (nParts * psi->nLife * 3 * (1 << nSpeed)) / (11 - nParts);
if (psi->nSide > 0) {
	float faceSize = FaceSize (objP->info.nSegment, psi->nSide - 1);
	psi->nParts = (int) (psi->nParts * ((faceSize < 1) ? sqrt (faceSize) : faceSize));
	}
psi->nDrift = psi->nDrift ? nSpeed * psi->nDrift * 75 : psi->nSpeed * 50;
nSize = psi->nSize [0] ? psi->nSize [0] : 5;
psi->nSize [0] = nSize + 1;
psi->nSize [1] = (nSize * (nSize + 1)) / 2;
}

//------------------------------------------------------------------------------

void SetupEffects (void)
{
	tObject	*objP;
	int		i;

PrintLog ("   setting up effects\n");
FORALL_EFFECT_OBJS (objP, i) 
	if (objP->info.nId == SMOKE_ID)
		SetupSmokeEffect (objP);
}

//------------------------------------------------------------------------------
//called after load.  Takes number of OBJECTS,  and OBJECTS should be
//compressed.  resets D2_FREE list, marks unused OBJECTS as unused
void ResetObjects (int nObjects)
{
	int 		i;
	tObject	*objP;

gameData.objs.nObjects = nObjects;
for (i = 0, objP = OBJECTS; i < nObjects; i++, objP++)
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
//Tries to find a tSegment for an tObject, using FindSegByPos ()
int FindObjectSeg (tObject * objP)
{
return FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
}

//------------------------------------------------------------------------------
//If an tObject is in a tSegment, set its nSegment field and make sure it's
//properly linked.  If not in any tSegment, returns 0, else 1.
//callers should generally use FindVectorIntersection ()
int UpdateObjectSeg (tObject * objP)
{
	int nNewSeg;

if (0 > (nNewSeg = FindObjectSeg (objP)))
	return 0;
if (nNewSeg != objP->info.nSegment)
	RelinkObjToSeg (OBJ_IDX (objP), nNewSeg);
return 1;
}

//------------------------------------------------------------------------------
//go through all OBJECTS and make sure they have the correct tSegment numbers
void FixObjectSegs (void)
{
	tObject	*objP;
	int		i;

FORALL_OBJS (objP, i) {
	if ((objP->info.nType == OBJ_NONE) || (objP->info.nType == OBJ_CAMBOT) || (objP->info.nType == OBJ_EFFECT))
		continue;
	if (UpdateObjectSeg (objP))
		continue;
	fix xScale = MinSegRad (objP->info.nSegment) - objP->info.xSize;
	if (xScale < 0)
		COMPUTE_SEGMENT_CENTER_I (&objP->info.position.vPos, objP->info.nSegment);
	else {
		vmsVector	vCenter, vOffset;
		COMPUTE_SEGMENT_CENTER_I (&vCenter, objP->info.nSegment);
		vOffset = objP->info.position.vPos - vCenter;
		vmsVector::Normalize(vOffset);
		objP->info.position.vPos = vCenter + vOffset * xScale;
		}
	}
}

//------------------------------------------------------------------------------
//go through all OBJECTS and make sure they have the correct size
void FixObjectSizes (void)
{
	int 		i;
	tObject	*objP = OBJECTS;

FORALL_ROBOT_OBJS (objP, i)
	objP->info.xSize = gameData.models.polyModels [objP->rType.polyObjInfo.nModel].rad;
}

//------------------------------------------------------------------------------

//delete OBJECTS, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if bClearAll is set, clear even proximity bombs
void ClearTransientObjects (int bClearAll)
{
	short nObject;
	tObject *objP, *nextObjP;

for (objP = gameData.objs.objLists.weapons.head; objP; objP = nextObjP) {
	nextObjP = objP->links [1].next;
	if ((!(gameData.weapons.info [objP->info.nId].flags&WIF_PLACABLE) &&
		  (bClearAll || ((objP->info.nId != PROXMINE_ID) && (objP->info.nId != SMARTMINE_ID)))) ||
			objP->info.nType == OBJ_FIREBALL ||
			objP->info.nType == OBJ_DEBRIS ||
			((objP->info.nType != OBJ_NONE) && (objP->info.nFlags & OF_EXPLODING))) {

#if DBG
#	if TRACE
		if (objP->info.xLifeLeft > I2X (2))
			con_printf (CONDBG, "Note: Clearing tObject %d (nType=%d, id=%d) with lifeleft=%x\n",
							OBJ_IDX (objP), objP->info.nType, objP->info.nId, objP->info.xLifeLeft);
#	endif
#endif
		ReleaseObject (OBJ_IDX (objP));
		}
	#if DBG
#	if TRACE
		else if ((objP->info.nType != OBJ_NONE) && (objP->info.xLifeLeft < I2X (2)))
		con_printf (CONDBG, "Note: NOT clearing tObject %d (nType=%d, id=%d) with lifeleft=%x\n",
						OBJ_IDX (objP), objP->info.nType, objP->info.nId,	objP->info.xLifeLeft);
#	endif
#endif
	}
}

//------------------------------------------------------------------------------
//attaches an tObject, such as a fireball, to another tObject, such as a robot
void AttachObject (tObject *parentObjP, tObject *childObjP)
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
//dettaches one tObject
void DetachFromParent (tObject *sub)
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
//dettaches all OBJECTS from this tObject
void DetachChildObjects (tObject *parent)
{
while (parent->info.nAttachedObj != -1)
	DetachFromParent (OBJECTS + parent->info.nAttachedObj);
}

//------------------------------------------------------------------------------
//creates a marker tObject in the world.  returns the tObject number
int DropMarkerObject (vmsVector *pos, short nSegment, vmsMatrix *orient, ubyte nMarker)
{
	short nObject;

Assert (gameData.models.nMarkerModel != -1);
nObject = CreateObject (OBJ_MARKER, nMarker, -1, nSegment, *pos, *orient,
								gameData.models.polyModels [gameData.models.nMarkerModel].rad, CT_NONE, MT_NONE, RT_POLYOBJ);
if (nObject >= 0) {
	tObject *objP = OBJECTS + nObject;
	objP->rType.polyObjInfo.nModel = gameData.models.nMarkerModel;
	objP->mType.spinRate = objP->info.position.mOrient[UVEC] * (F1_0 / 2);
	//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all OBJECTS.
	objP->info.xLifeLeft = IMMORTAL_TIME - 1;
	}
return nObject;
}

//------------------------------------------------------------------------------

//	*viewerP is a viewerP, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void WakeupRenderedObjects (tObject *viewerP, int nWindow)
{
	int	i;

	//	Make sure that we are processing current data.
if (gameData.app.nFrameCount != windowRenderedData [nWindow].nFrame) {
#if TRACE
		con_printf (1, "Warning: Called WakeupRenderedObjects with a bogus window.\n");
#endif
	return;
	}
gameData.ai.nLastMissileCamera = OBJ_IDX (viewerP);

int nObject, frameFilter = gameData.app.nFrameCount & 3;
tObject *objP;
tAILocalInfo *ailP;

for (i = windowRenderedData [nWindow].nObjects; i; ) {
	nObject = windowRenderedData [nWindow].renderedObjects [--i];
	if ((nObject & 3) == frameFilter) {
		objP = OBJECTS + nObject;
		if (objP->info.nType == OBJ_ROBOT) {
			if (vmsVector::Dist (viewerP->info.position.vPos, objP->info.position.vPos) < F1_0*100) {
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
memset (gameData.objs.firstChild, 0xff, sizeof (*gameData.objs.firstChild) * MAX_OBJECTS);
memset (gameData.objs.parentObjs, 0xff, sizeof (*gameData.objs.parentObjs) * MAX_OBJECTS);
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

int AddChildObjectP (tObject *pParent, tObject *pChild)
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

int DelObjChildrenP (tObject *pParent)
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

int DelObjChildP (tObject *pChild)
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

tObjectRef *GetChildObjP (tObject *pParent, tObjectRef *pChildRef)
{
return GetChildObjN (OBJ_IDX (pParent), pChildRef);
}

//------------------------------------------------------------------------------

int CountPlayerObjects (int nPlayer, int nType, int nId)
{
	int		i, h = 0;
	tObject	*objP;

FORALL_OBJS (objP, i) 
	if ((objP->info.nType == nType) && (objP->info.nId == nId) &&
		 (objP->cType.laserInfo.parent.nType == OBJ_PLAYER) &&
		 (OBJECTS [objP->cType.laserInfo.parent.nObject].info.nId == nPlayer))
	h++;
return h;
}

//------------------------------------------------------------------------------

void GetPlayerSpawn (int nSpawnPos, tObject *objP)
{
	tObject	*markerP = SpawnMarkerObject (-1);

if (markerP) {
	objP->info.position = markerP->info.position;
 	RelinkObjToSeg (OBJ_IDX (objP), markerP->info.nSegment);
	}
else {
	if ((gameData.multiplayer.playerInit [nSpawnPos].nSegment < 0) || 
		 (gameData.multiplayer.playerInit [nSpawnPos].nSegment >= gameData.segs.nSegments))
	GameStartInitNetworkPlayers ();
	objP->info.position = gameData.multiplayer.playerInit [nSpawnPos].position;
 	RelinkObjToSeg (OBJ_IDX (objP), gameData.multiplayer.playerInit [nSpawnPos].nSegment);
	}
}

//------------------------------------------------------------------------------

vmsVector *PlayerSpawnPos (int nPlayer)
{
	tObject	*markerP = SpawnMarkerObject (nPlayer);

return markerP ? &markerP->info.position.vPos : &gameData.multiplayer.playerInit [nPlayer].position.vPos;
}

//------------------------------------------------------------------------------

vmsMatrix *PlayerSpawnOrient (int nPlayer)
{
	tObject	*markerP = SpawnMarkerObject (nPlayer);

return markerP ? &markerP->info.position.mOrient : &gameData.multiplayer.playerInit [nPlayer].position.mOrient;
}

//------------------------------------------------------------------------------

vmsMatrix *ObjectView (tObject *objP)
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
	tObject	o, *objP = OBJECTS;
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
o.info.position = OBJECTS->info.position;
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
