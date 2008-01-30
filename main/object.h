/* $Id: object.h,v 1.6 2003/10/08 17:09:48 schaffner Exp $ */
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

#ifndef _OBJECT_H
#define _OBJECT_H

#include <time.h>

#include "pstypes.h"
#include "vecmat.h"
//#include "segment.h"
//#include "gameseg.h"
#include "piggy.h"
#include "aistruct.h"
#include "segment.h"
#include "gr.h"

/*
 * CONSTANTS
 */

#define MAX_OBJECTS_D2  	350 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world
#define MAX_OBJECTS_D2X	   MAX_SEGMENTS_D2X
#define MAX_OBJECTS     	MAX_SEGMENTS
#define MAX_HIT_OBJECTS		20

// Object types
#define OBJ_NONE        255 // unused tObject
#define OBJ_WALL        0   // A tWall... not really an tObject, but used for collisions
#define OBJ_FIREBALL    1   // a fireball, part of an explosion
#define OBJ_ROBOT       2   // an evil enemy
#define OBJ_HOSTAGE     3   // a hostage you need to rescue
#define OBJ_PLAYER      4   // the tPlayer on the console
#define OBJ_WEAPON      5   // a laser, missile, etc
#define OBJ_CAMERA      6   // a camera to slew around with
#define OBJ_POWERUP     7   // a powerup you can pick up
#define OBJ_DEBRIS      8   // a piece of robot
#define OBJ_REACTOR    9   // the control center
#define OBJ_FLARE       10  // a flare
#define OBJ_CLUTTER     11  // misc objects
#define OBJ_GHOST       12  // what the tPlayer turns into when dead
#define OBJ_LIGHT       13  // a light source, & not much else
#define OBJ_COOP        14  // a cooperative tPlayer tObject.
#define OBJ_MARKER      15  // a map marker
#define OBJ_CAMBOT		16	 // a camera
#define OBJ_MONSTERBALL	17	 // a monsterball
#define OBJ_SMOKE			18	 // static smoke
#define OBJ_EXPLOSION	19	 // static explosion clouds
#define OBJ_EFFECT		20	 // lightnings

// WARNING!! If you add a nType here, add its name to ObjectType_names
// in tObject.c
#define MAX_OBJECT_TYPES 21

// Result types
#define RESULT_NOTHING  0   // Ignore this collision
#define RESULT_CHECK    1   // Check for this collision

// Control types - what tells this tObject what do do
#define CT_NONE         0   // doesn't move (or change movement)
#define CT_AI           1   // driven by AI
#define CT_EXPLOSION    2   // explosion sequencer
#define CT_FLYING       4   // the tPlayer is flying
#define CT_SLEW         5   // slewing
#define CT_FLYTHROUGH   6   // the flythrough system
#define CT_WEAPON       9   // laser, etc.
#define CT_REPAIRCEN    10  // under the control of the repair center
#define CT_MORPH        11  // this tObject is being morphed
#define CT_DEBRIS       12  // this is a piece of debris
#define CT_POWERUP      13  // animating powerup blob
#define CT_LIGHT        14  // doesn't actually do anything
#define CT_REMOTE       15  // controlled by another net tPlayer
#define CT_CNTRLCEN     16  // the control center/main reactor
#define CT_CAMERA			17

// Movement types
#define MT_NONE         0   // doesn't move
#define MT_PHYSICS      1   // moves by physics
#define MT_STATIC			2	 // completely still and immoveable
#define MT_SPINNING     3   // this tObject doesn't move, just sits and spins

// Render types
#define RT_NONE         0   // does not render
#define RT_POLYOBJ      1   // a polygon model
#define RT_FIREBALL     2   // a fireball
#define RT_LASER        3   // a laser
#define RT_HOSTAGE      4   // a hostage
#define RT_POWERUP      5   // a powerup
#define RT_MORPH        6   // a robot being morphed
#define RT_WEAPON_VCLIP 7   // a weapon that renders as a tVideoClip
#define RT_THRUSTER		8	 // like afterburner, but doesn't cast light
#define RT_EXPLBLAST		9	 // white explosion light blast
#define RT_SHRAPNELS		10	 // white explosion light blast
#define RT_SMOKE			11
#define RT_LIGHTNING    12

#define SMOKE_ID			0
#define LIGHTNING_ID		1

// misc tObject flags
#define OF_EXPLODING        1   // this tObject is exploding
#define OF_SHOULD_BE_DEAD   2   // this tObject should be dead, so next time we can, we should delete this tObject.
#define OF_DESTROYED        4   // this has been killed, and is showing the dead version
#define OF_SILENT           8   // this makes no sound when it hits a tWall.  Added by MK for weapons, if you extend it to other types, do it completely!
#define OF_ATTACHED         16  // this tObject is a fireball attached to another tObject
#define OF_HARMLESS         32  // this tObject does no damage.  Added to make quad lasers do 1.5 damage as normal lasers.
#define OF_PLAYER_DROPPED   64  // this tObject was dropped by the tPlayer...
#define OF_ARMAGEDDON		 128 // destroyed by cheat

// Different Weapon ID types...
#define WEAPON_ID_LASER         0
#define WEAPON_IDMSLLE        1
#define WEAPON_ID_CANNONBALL    2

// Object Initial shields...
#define OBJECT_INITIAL_SHIELDS F1_0/2

// physics flags
#define PF_TURNROLL         0x01    // roll when turning
#define PF_LEVELLING        0x02    // level tObject with closest tSide
#define PF_BOUNCE           0x04    // bounce (not slide) when hit will
#define PF_WIGGLE           0x08    // wiggle while flying
#define PF_STICK            0x10    // tObject sticks (stops moving) when hits tWall
#define PF_PERSISTENT       0x20    // tObject keeps going even after it hits another tObject (eg, fusion cannon)
#define PF_USES_THRUST      0x40    // this tObject uses its thrust
#define PF_BOUNCED_ONCE     0x80    // Weapon has bounced once.
#define PF_FREE_SPINNING    0x100   // Drag does not apply to rotation of this tObject
#define PF_BOUNCES_TWICE    0x200   // This weapon bounces twice, then dies

#define IMMORTAL_TIME   0x3fffffff  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.
#define ONE_FRAME_TIME  0x3ffffffe  // Objects with this lifeleft will live for exactly one frame

#define MAX_VELOCITY i2f(50)

extern char szObjectTypeNames [MAX_OBJECT_TYPES][9];

// List of objects rendered last frame in order.  Created at render
// time, used by homing missiles in laser.c
#define MAX_RENDERED_OBJECTS    100

/*
 * STRUCTURES
 */

// A compressed form for sending crucial data about via slow devices,
// such as modems and buggies.
typedef struct tShortPos {
	sbyte   bytemat[9];
	short   xo,yo,zo;
	short   nSegment;
	short   velx, vely, velz;
} __pack__ tShortPos;

// This is specific to the tShortPos extraction routines in gameseg.c.
#define RELPOS_PRECISION    10
#define MATRIX_PRECISION    9
#define MATRIX_MAX          0x7f    // This is based on MATRIX_PRECISION, 9 => 0x7f

// information for physics sim for an tObject
typedef struct tPhysicsInfo {
	vmsVector	velocity;   // velocity vector of this tObject
	vmsVector	thrust;     // constant force applied to this tObject
	fix         mass;       // the mass of this tObject
	fix         drag;       // how fast this slows down
	fix         brakes;     // how much brakes applied
	vmsVector	rotVel;     // rotational velecity (angles)
	vmsVector	rotThrust;  // rotational acceleration
	fixang      turnRoll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
} __pack__ tPhysicsInfo;

// stuctures for different kinds of simulation

typedef struct tLaserInfo  {
	short   parentType;        // The nType of the parent of this tObject
	short   nParentObj;         // The tObject's parent's number
	int     nParentSig;			// The tObject's parent's nSignature...
	fix     creationTime;      // Absolute time of creation.
	short   nLastHitObj;        // For persistent weapons (survive tObject collision), tObject it most recently hit.
	short   nMslLock;         // Object this tObject is tracking.
	fix     multiplier;         // Power if this is a fusion bolt (or other super weapon to be added).
} __pack__ tLaserInfo;

typedef struct tExplosionInfo {
    fix     nSpawnTime;         // when lifeleft is < this, spawn another
    fix     nDeleteTime;        // when to delete tObject
    short   nDeleteObj;      // and what tObject to delete
    short   nAttachParent;      // explosion is attached to this tObject
    short   nPrevAttach;        // previous explosion in attach list
    short   nNextAttach;        // next explosion in attach list
} __pack__ tExplosionInfo;

typedef struct tObjLightInfo {
    fix     intensity;          // how bright the light is
} __pack__ tObjLightInfo;

#define PF_SPAT_BY_PLAYER   1   //this powerup was spat by the tPlayer

typedef struct tPowerupInfo {
	int     count;          // how many/much we pick up (vulcan cannon only?)
	fix     creationTime;  // Absolute time of creation.
	int     flags;          // spat by tPlayer?
} __pack__ tPowerupInfo;

typedef struct tVClipInfo {
	int     nClipIndex;
	fix	  xTotalTime;
	fix     xFrameTime;
	sbyte   nCurFrame;
} __pack__ tVClipInfo;

typedef struct tSmokeInfo {
	int			nLife;
	int			nSize [2];
	int			nParts;
	int			nSpeed;
	int			nDrift;
	int			nBrightness;
	tRgbaColorb	color;
	char			nSide;
} __pack__ tSmokeInfo;

typedef struct tLightningInfo {
	int			nLife;
	int			nDelay;
	int			nLength;
	int			nAmplitude;
	int			nOffset;
	short			nLightnings;
	short			nId;
	short			nTarget;
	short			nNodes;
	short			nChildren;
	short			nSteps;
	char			nAngle;
	char			nStyle;
	char			nSmoothe;
	char			bClamp;
	char			bPlasma;
	char			bSound;
	char			bRandom;
	char			bInPlane;
	tRgbaColorb color;
} __pack__ tLightningInfo;

// structures for different kinds of rendering

typedef struct tPolyObjInfo {
	int     		nModel;          // which polygon model
	vmsAngVec 	animAngles [MAX_SUBMODELS]; // angles for each subobject
	int     		nSubObjFlags;       // specify which subobjs to draw
	int     		nTexOverride;      // if this is not -1, map all face to this
	int     		nAltTextures;       // if not -1, use these textures instead
} __pack__ tPolyObjInfo;

typedef struct tPosition {
	vmsVector	vPos;				// absolute x,y,z coordinate of center of object
	vmsMatrix	mOrient;			// orientation of object in world
} tPosition;

typedef struct tObject {
	int     		nSignature;    // Every tObject ever has a unique nSignature...
	ubyte   		nType;         // what nType of tObject this is... robot, weapon, hostage, powerup, fireball
	ubyte   		id;            // which form of tObject...which powerup, robot, etc.
#ifdef WORDS_NEED_ALIGNMENT
	short   		pad;
#endif
	short   		next, prev;    // id of next and previous connected tObject in Objects, -1 = no connection
	ubyte   		controlType;   // how this tObject is controlled
	ubyte   		movementType;  // how this tObject moves
	ubyte   		renderType;    // how this tObject renders
	ubyte   		flags;         // misc flags
	short			nSegment;
	short   		attachedObj;   // number of attached fireball tObject
	tPosition	position;
	fix     		size;          // 3d size of tObject - for collision detection
	fix     		shields;       // Starts at maximum, when <0, tObject dies..
	vmsVector 	vLastPos;		// where tObject was last frame
	sbyte   		containsType;  // Type of tObject this tObject contains (eg, spider contains powerup)
	sbyte   		containsId;    // ID of tObject this tObject contains (eg, id = blue nType = key)
	sbyte   		containsCount; // number of objects of nType:id this tObject contains
	sbyte   		matCenCreator; // Materialization center that created this tObject, high bit set if matcen-created
	fix     		lifeleft;      // how long until goes away, or 7fff if immortal
	// movement info, determined by MOVEMENT_TYPE
	union {
		tPhysicsInfo	physInfo; // a physics tObject
		vmsVector   	spinRate; // for spinning objects
		} mType;
	// control info, determined by CONTROL_TYPE
	union {
		tLaserInfo		laserInfo;
		tExplosionInfo explInfo;      // NOTE: debris uses this also
		tAIStatic      aiInfo;
		tObjLightInfo  lightInfo;     // why put this here?  Didn't know what else to do with it.
		tPowerupInfo   powerupInfo;
		} cType;
	// render info, determined by RENDER_TYPE
	union {
		tPolyObjInfo   polyObjInfo;      // polygon model
		tVClipInfo     vClipInfo;     // tVideoClip
		tSmokeInfo		smokeInfo;
		tLightningInfo	lightningInfo;
		} rType;
#ifdef WORDS_NEED_ALIGNMENT
	short   nPad;
#endif
	} __pack__ tObject;

typedef struct tObjPosition {
	tPosition	position;
	short       nSegment;     // tSegment number containing tObject
	short			nSegType;		// nType of tSegment
} tObjPosition;

typedef struct {
	int     frame;
	tObject *viewer;
	int     rearView;
	int     user;
	int     numObjects;
	short   renderedObjects [MAX_RENDERED_OBJECTS];
} tWindowRenderedData;

typedef struct tObjDropInfo {
	time_t	nDropTime;
	short		nPowerupType;
	short		nPrevPowerup;
	short		nNextPowerup;
	short		nObject;
} tObjDropInfo;

typedef struct tObjectRef {
	short		objIndex;
	short		nextObj;
} tObjectRef;

#define MAX_RENDERED_WINDOWS    3

extern tWindowRenderedData windowRenderedData [MAX_RENDERED_WINDOWS];

/*
 * VARIABLES
 */

// ie gameData.objs.collisionResult[a][b]==  what happens to a when it collides with b

extern char *robot_names[];         // name of each robot

extern tObject Follow;

/*
 * FUNCTIONS
 */


// do whatever setup needs to be done
void InitObjects();

// returns tSegment number tObject is in.  Searches out from tObject's current
// seg, so this shouldn't be called if the tObject has "jumped" to a new seg
int obj_get_new_seg(tObject *obj);

// when an tObject has moved into a new tSegment, this function unlinks it
// from its old tSegment, and links it into the new tSegment
void RelinkObject(int nObject,int newsegnum);

// move an tObject from one tSegment to another. unlinks & relinks
void obj_set_new_seg(int nObject,int newsegnum);

// links an tObject into a tSegment's list of objects.
// takes tObject number and tSegment number
void LinkObject(int nObject,int nSegment);

// unlinks an tObject from a tSegment's list of objects
void UnlinkObject(int nObject);

// initialize a new tObject.  adds to the list for the given tSegment
// returns the tObject number
int CreateObject(ubyte nType, ubyte id, short owner, short nSegment, vmsVector *pos,
               vmsMatrix *orient, fix size,
               ubyte ctype, ubyte mtype, ubyte rtype, int bIgnoreLimits);

// make a copy of an tObject. returs num of new tObject
int CreateObjectCopy(int nObject, vmsVector *new_pos, int newsegnum);

// remove tObject from the world
void ReleaseObject(short nObject);

// called after load.  Takes number of objects, and objects should be
// compressed
void ResetObjects (int nObjects);
void ConvertObjects (void);
void SetupEffects (void);

// make tObject array non-sparse
void compressObjects(void);

// Draw a blob-nType tObject, like a fireball
// Deletes all objects that have been marked for death.
void DeleteAllObjsThatShouldBeDead();

// Toggles whether or not lock-boxes draw.
void object_toggle_lock_targets();

// move all objects for the current frame
int UpdateAllObjects();     // moves all objects

// set viewer tObject to next tObject in array
void object_goto_nextViewer();

// draw target boxes for nearby robots
void object_render_targets(void);

// move an tObject for the current frame
int UpdateObject(tObject * obj);

// make object0 the tPlayer, setting all relevant fields
void InitPlayerObject();

// check if tObject is in tObject->nSegment.  if not, check the adjacent
// segs.  if not any of these, returns false, else sets obj->nSegment &
// returns true callers should really use FindVectorIntersection()
// Note: this function is in gameseg.c
extern int UpdateObjectSeg(tObject *obj);


// Finds what tSegment *obj is in, returns tSegment number.  If not in
// any tSegment, returns -1.  Note: This function is defined in
// gameseg.h, but object.h depends on gameseg.h, and object.h is where
// tObject is defined...get it?
extern int FindObjectSeg(tObject * obj);

// go through all objects and make sure they have the correct tSegment
// numbers used when debugging is on
void FixObjectSegs();

// Drops objects contained in objp.
int ObjectCreateEgg(tObject *objp);

// Interface to ObjectCreateEgg, puts count objects of nType nType, id
// = id in objp and then drops them.
int CallObjectCreateEgg(tObject *objp, int count, int nType, int id);

extern void DeadPlayerEnd(void);

// Extract information from an tObject (objp->orient, objp->pos,
// objp->nSegment), stuff in a tShortPos structure.  See typedef
// tShortPos.
extern void CreateShortPos(tShortPos *spp, tObject *objp, int swap_bytes);

// Extract information from a tShortPos, stuff in objp->orient
// (matrix), objp->pos, objp->nSegment
extern void ExtractShortPos(tObject *objp, tShortPos *spp, int swap_bytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void ClearTransientObjects(int clear_all);

// returns the number of a free tObject, updating HighestObject_index.
// Generally, CreateObject() should be called to get an tObject, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
int AllocObject(void);

// frees up an tObject.  Generally, ReleaseObject() should be called to
// get rid of an tObject.  This function deallocates the tObject entry
// after the tObject has been unlinked
void FreeObject(int nObject);

// after calling initObject(), the network code has grabbed specific
// tObject slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void SpecialResetObjects(void);

// attaches an tObject, such as a fireball, to another tObject, such as
// a robot
void AttachObject(tObject *parent, tObject *sub);

extern void CreateSmallFireballOnObject(tObject *objp, fix size_scale, int soundFlag);

// returns tObject number
int DropMarkerObject(vmsVector *pos, short nSegment, vmsMatrix *orient, ubyte marker_num);

extern void WakeupRenderedObjects(tObject *gmissp, int window_num);

extern void AdjustMineSpawn();

void ResetPlayerObject(void);
void StopObjectMovement (tObject *obj);
void StopPlayerMovement (void);

int ObjectSoundClass (tObject *objP);

void ObjectGotoNextViewer();
void ObjectGotoPrevViewer();

int ObjectCount (int nType);

void ResetChildObjects (void);
int AddChildObjectN (int nParent, int nChild);
int AddChildObjectP (tObject *pParent, tObject *pChild);
int DelObjChildrenN (int nParent);
int DelObjChildrenP (tObject *pParent);
int DelObjChildN (int nChild);
int DelObjChildP (tObject *pChild);

void BuildObjectModels (void);

tObjectRef *GetChildObjN (short nParent, tObjectRef *pChildRef);
tObjectRef *GetChildObjP (tObject *pParent, tObjectRef *pChildRef);

tObject *ObjFindFirstOfType (int nType);
void InitWeaponFlags (void);
float ObjectDamage (tObject *objP);
int FindBoss (int nObject);
void InitGateIntervals (void);
int CountPlayerObjects (int nPlayer, int nType, int nId);
void FixObjectSizes (void);
void DoSlowMotionFrame (void);
vmsMatrix *ObjectView (tObject *objP);

vmsVector *PlayerSpawnPos (int nPlayer);
vmsMatrix *PlayerSpawnOrient (int nPlayer);
void GetPlayerSpawn (int nPlayer, tObject *objP);

extern ubyte bIsMissile [];

#define	OBJ_CLOAKED(_objP)	((_objP)->ctype.aiInfo.flags [6])

#define	SHOW_SHADOWS \
			(!gameStates.render.bRenderIndirect && \
			 gameStates.app.bEnableShadows && \
			 EGI_FLAG (bShadows, 0, 1, 0) && \
			 !COMPETITION)

#define	SHOW_OBJ_FX \
			(!(gameStates.app.bNostalgia || COMPETITION))

#define	IS_BOSS(_objP)		(((_objP)->nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->id).bossFlag)
#define	IS_BOSS_I(_i)		IS_BOSS (gameData.objs.objects + (_i))
#define	IS_MISSILE(_objP)	(((_objP)->nType == OBJ_WEAPON) && bIsMissile [(_objP)->id])
#define	IS_MISSILE_I(_i)	IS_MISSILE (gameData.objs.objects + (_i))

#ifdef _DEBUG
extern tObject *dbgObjP;
#endif

static inline void KillObject (tObject *objP)
{
objP->flags |= OF_SHOULD_BE_DEAD;
#ifdef _DEBUG
if (objP == dbgObjP)
	objP = objP;
#endif
}

#define SET_COLLISION(type1, type2, result) \
	gameData.objs.collisionResult [type1][type2] = result; \
	gameData.objs.collisionResult [type2][type1] = result;

#define ENABLE_COLLISION(type1, type2)		SET_COLLISION(type1, type2, RESULT_CHECK)

#define DISABLE_COLLISION(type1, type2)	SET_COLLISION(type1, type2, RESULT_NOTHING)

#define OBJECT_EXISTS(_objP)	 ((_objP) && !((_objP)->flags & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)))

//	-----------------------------------------------------------------------------------------------------------

#endif
