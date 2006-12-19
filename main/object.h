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

/*
 *
 * tObject system definitions
 *
 * Old Log:
 * Revision 1.6  1995/09/20  14:24:45  allender
 * swap bytes on extractshortpos
 *
 * Revision 1.5  1995/09/14  14:11:42  allender
 * FixObjectSegs returns void
 *
 * Revision 1.4  1995/08/12  12:02:44  allender
 * added flag to CreateShortPos
 *
 * Revision 1.3  1995/07/12  12:55:08  allender
 * move structures back to original form as found on PC because
 * of network play
 *
 * Revision 1.2  1995/06/19  07:55:06  allender
 * rearranged structure members for possible better alignment
 *
 * Revision 1.1  1995/05/16  16:00:40  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/31  12:24:10  john
 * I had changed nAltTextures from a pointer to a byte. This hosed old
 * saved games, so I restored it to an int.
 *
 * Revision 2.0  1995/02/27  11:26:47  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.122  1995/02/22  12:35:53  allender
 * remove anonymous unions
 *
 * Revision 1.121  1995/02/06  20:43:25  rob
 * Extern'ed Dead_player_camera so it can be reset by multi.c
 *
 * Revision 1.120  1995/02/01  16:34:07  john
 * Linted.
 *
 * Revision 1.119  1995/01/29  13:46:42  mike
 * adapt to new CreateSmallFireballOnObject prototype.
 *
 * Revision 1.118  1995/01/26  22:11:27  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 *
 * Revision 1.117  1995/01/24  12:09:29  mike
 * Boost MAX_OBJECTS from 250 to 350.
 *
 * Revision 1.116  1995/01/13  19:39:51  rob
 * Removed outdated remoteInfo structure.  (looking for cause of bugs
 *
 * Revision 1.115  1995/01/12  12:09:38  yuan
 * Added coop tObject capability.
 *
 * Revision 1.114  1994/12/15  13:04:20  mike
 * Replace Players[nLocalPlayer].timeTotal references with GameTime.
 *
 * Revision 1.113  1994/12/12  17:18:09  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 *
 * Revision 1.112  1994/12/09  14:58:42  matt
 * Added system to attach a fireball to another tObject for rendering purposes,
 * so the fireball always renders on top of (after) the tObject.
 *
 * Revision 1.111  1994/12/08  12:35:35  matt
 * Added new tObject allocation & deallocation functions so other code
 * could stop messing around with internal tObject data structures.
 *
 * Revision 1.110  1994/11/21  17:30:21  matt
 * Increased max number of objects
 *
 * Revision 1.109  1994/11/18  23:41:52  john
 * Changed some shorts to ints.
 *
 * Revision 1.108  1994/11/10  14:02:45  matt
 * Hacked in support for tPlayer ships with different textures
 *
 * Revision 1.107  1994/11/08  12:19:27  mike
 * Small explosions on objects.
 *
 * Revision 1.106  1994/10/25  10:51:17  matt
 * Vulcan cannon powerups now contain ammo count
 *
 * Revision 1.105  1994/10/21  12:19:41  matt
 * Clear transient objects when saving (& loading) games
 *
 * Revision 1.104  1994/10/21  11:25:04  mike
 * Add IMMORTAL_TIME.
 *
 * Revision 1.103  1994/10/17  21:34:54  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.102  1994/10/14  18:12:28  mike
 * Make egg dropping return tObject number.
 *
 * Revision 1.101  1994/10/12  21:07:19  matt
 * Killed unused field in tObject structure
 *
 * Revision 1.100  1994/10/12  10:38:24  mike
 * Add field OF_SILENT to obj->flags.
 *
 * Revision 1.99  1994/10/11  20:35:48  matt
 * Clear "transient" objects (weapons,explosions,etc.) when starting a level
 *
 * Revision 1.98  1994/10/03  20:56:13  rob
 * Added velocity to shortpos strucutre.
 *
 * Revision 1.97  1994/09/30  18:24:00  rob
 * Added new control nType CT_REMOTE for remote controlled objects.
 * Also added a union struct 'remoteInfo' for this nType.
 *
 * Revision 1.96  1994/09/28  09:23:05  mike
 * Prototype ObjectType_names.
 *
 * Revision 1.95  1994/09/25  23:32:37  matt
 * Changed the tObject load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * tObject structure can be changed without breaking the load/save functions.
 * As a result of this change, the localObject data can be and has been
 * incorporated into the tObject array.  Also, timeleft is now a property
 * of all objects, and the tObject structure has been otherwise cleaned up.
 *
 * Revision 1.94  1994/09/25  15:45:28  matt
 * Added OBJ_LIGHT, a nType of tObject that casts light
 * Added generalized lifeleft, and moved it to localObject
 *
 * Revision 1.93  1994/09/24  17:41:19  mike
 * Add stuff to LocalObject structure for materialization centers.
 *
 * Revision 1.92  1994/09/24  13:16:50  matt
 * Added (hacked in, really) support for overriding the bitmaps used on to
 * texture map a polygon tObject, and using a new bitmap for all the faces.
 *
 * Revision 1.91  1994/09/22  19:02:14  mike
 * Prototype functions ExtractShortPos and CreateShortPos which reside in
 * gameseg.c, but are prototyped here to prevent circular dependencies.
 *
 * Revision 1.90  1994/09/15  21:47:14  mike
 * Prototype DeadPlayerEnd().
 *
 * Revision 1.89  1994/09/15  16:34:47  mike
 * Add nDangerLaser and nDangerLaserSig to object_local to
 * enable robots to efficiently (too efficiently!) avoid tPlayer fire.
 *
 * Revision 1.88  1994/09/11  22:46:19  mike
 * Death_sequence_aborted prototyped.
 *
 * Revision 1.87  1994/09/09  20:04:30  mike
 * Add vclips for weapons.
 *
 * Revision 1.86  1994/09/09  14:20:54  matt
 * Added flag that says tObject uses thrust
 *
 * Revision 1.85  1994/09/08  14:51:32  mike
 * Make a crucial name change to a field of localObject struct.
 *
 * Revision 1.84  1994/09/07  19:16:45  mike
 * Homing missile.
 *
 * Revision 1.83  1994/09/06  17:05:43  matt
 * Added new nType for dead tPlayer
 *
 * Revision 1.82  1994/09/02  11:56:09  mike
 * Add persistency (PF_PERSISTENT) to physicsInfo.
 *
 * Revision 1.81  1994/08/28  19:10:28  mike
 * Add Player_is_dead.
 *
 * Revision 1.80  1994/08/18  15:11:44  mike
 * powerup stuff.
 *
 * Revision 1.79  1994/08/15  15:24:54  john
 * Made players know who killed them; Disabled cheat menu
 * during net tPlayer; fixed bug with not being able to turn
 * of invulnerability; Made going into edit/starting new leve
 * l drop you out of a net game; made death dialog box.
 *
 * Revision 1.78  1994/08/14  23:15:12  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 *
 * Revision 1.77  1994/08/13  14:58:27  matt
 * Finished adding support for miscellaneous objects
 *
 * Revision 1.76  1994/08/09  16:04:13  john
 * Added network players to editor.
 *
 * Revision 1.75  1994/08/03  21:06:19  matt
 * Added prototype for FixObjectSegs(), and renamed now-unused spawn_pos
 *
 * Revision 1.74  1994/08/02  12:30:27  matt
 * Added support for spinning objects
 *
 * Revision 1.73  1994/07/27  20:53:25  matt
 * Added rotational drag & thrust, so turning now has momemtum like moving
 *
 * Revision 1.72  1994/07/27  19:44:21  mike
 * Objects containing objects.
 *
 * Revision 1.71  1994/07/22  20:43:29  matt
 * Fixed flares, by adding a physics flag that makes them stick to walls.
 *
 * Revision 1.70  1994/07/21  12:42:10  mike
 * Prototype new FindObjectSeg and UpdateObjectSeg.
 *
 * Revision 1.69  1994/07/19  15:26:39  mike
 * New tAIStatic structure.
 *
 * Revision 1.68  1994/07/13  00:15:06  matt
 * Moved all (or nearly all) of the values that affect tPlayer movement to
 * bitmaps.tbl
 *
 * Revision 1.67  1994/07/12  12:40:12  matt
 * Revamped physics system
 *
 * Revision 1.66  1994/07/06  15:26:23  yuan
 * Added chase mode.
 *
 *
 *
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
#include "gr.h"

/*
 * CONSTANTS
 */

#define MAX_OBJECTS_D2  	350 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world
#define MAX_OBJECTS_D2X	   700 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world
#define MAX_OBJECTS     	3500 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world

// Object types
#define OBJ_NONE        255 // unused tObject
#define OBJ_WALL        0   // A wall... not really an tObject, but used for collisions
#define OBJ_FIREBALL    1   // a fireball, part of an explosion
#define OBJ_ROBOT       2   // an evil enemy
#define OBJ_HOSTAGE     3   // a hostage you need to rescue
#define OBJ_PLAYER      4   // the tPlayer on the console
#define OBJ_WEAPON      5   // a laser, missile, etc
#define OBJ_CAMERA      6   // a camera to slew around with
#define OBJ_POWERUP     7   // a powerup you can pick up
#define OBJ_DEBRIS      8   // a piece of robot
#define OBJ_CNTRLCEN    9   // the control center
#define OBJ_FLARE       10  // a flare
#define OBJ_CLUTTER     11  // misc objects
#define OBJ_GHOST       12  // what the tPlayer turns into when dead
#define OBJ_LIGHT       13  // a light source, & not much else
#define OBJ_COOP        14  // a cooperative tPlayer tObject.
#define OBJ_MARKER      15  // a map marker
#define OBJ_CAMBOT		16	 // a camera
#define OBJ_MONSTERBALL	17	 // a monsterball

// WARNING!! If you add a nType here, add its name to ObjectType_names
// in tObject.c
#define MAX_OBJECT_TYPES    18

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

// misc tObject flags
#define OF_EXPLODING        1   // this tObject is exploding
#define OF_SHOULD_BE_DEAD   2   // this tObject should be dead, so next time we can, we should delete this tObject.
#define OF_DESTROYED        4   // this has been killed, and is showing the dead version
#define OF_SILENT           8   // this makes no sound when it hits a wall.  Added by MK for weapons, if you extend it to other types, do it completely!
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
#define PF_STICK            0x10    // tObject sticks (stops moving) when hits wall
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
#define MAX_RENDERED_OBJECTS    50

/*
 * STRUCTURES
 */

// A compressed form for sending crucial data about via slow devices,
// such as modems and buggies.
typedef struct shortpos {
	sbyte   bytemat[9];
	short   xo,yo,zo;
	short   tSegment;
	short   velx, vely, velz;
} __pack__ shortpos;

// This is specific to the shortpos extraction routines in gameseg.c.
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
	short   nTrackGoal;         // Object this tObject is tracking.
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
	fix     xFrameTime;
	sbyte   nCurFrame;
} __pack__ tVClipInfo;

// structures for different kinds of rendering

typedef struct tPolyObjInfo {
	int     		nModel;          // which polygon model
	vmsAngVec 	animAngles[MAX_SUBMODELS]; // angles for each subobject
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
	short   		nSegment;      // tSegment number containing tObject
	short   		attachedObj;   // number of attached fireball tObject
	tPosition	position;
	fix     		size;          // 3d size of tObject - for collision detection
	fix     		shields;       // Starts at maximum, when <0, tObject dies..
	vmsVector 	last_pos;		// where tObject was last frame
	sbyte   		containsType;  // Type of tObject this tObject contains (eg, spider contains powerup)
	sbyte   		containsId;    // ID of tObject this tObject contains (eg, id = blue nType = key)
	sbyte   		containsCount; // number of objects of nType:id this tObject contains
	sbyte   		matCenCreator; // Materialization center that created this tObject, high bit set if matcen-created
	fix     		lifeleft;      // how long until goes away, or 7fff if immortal
	// movement info, determined by MOVEMENT_TYPE
	union {
		tPhysicsInfo	physInfo; // a physics tObject
		vmsVector   	spinRate; // for spinning objects
		} mType __pack__ ;
	// control info, determined by CONTROL_TYPE
	union {
		tLaserInfo      laserInfo;
		tExplosionInfo  explInfo;      // NOTE: debris uses this also
		tAIStatic       aiInfo;
		tObjLightInfo   lightInfo;     // why put this here?  Didn't know what else to do with it.
		tPowerupInfo    powerupInfo;
		} cType __pack__ ;
	// render info, determined by RENDER_TYPE
	union {
		tPolyObjInfo   polyObjInfo;      // polygon model
		tVClipInfo     vClipInfo;     // tVideoClip
		} rType __pack__ ;
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
	int     rear_view;
	int     user;
	int     numObjects;
	short   renderedObjects[MAX_RENDERED_OBJECTS];
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

extern ubyte CollisionResult[MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
// ie CollisionResult[a][b]==  what happens to a when it collides with b

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
void ResetObjects(int n_objs);

// make tObject array non-sparse
void compressObjects(void);

// Render an tObject.  Calls one of several routines based on nType
void RenderObject(tObject *obj, int nWindowNum);

// Draw a blob-nType tObject, like a fireball
void DrawObjectBlob(tObject *obj, tBitmapIndex bmi0, tBitmapIndex bmi, int iFrame,
						  tRgbColorf *color, float alpha);

// draw an tObject that is a texture-mapped rod
void DrawObjectRodTexPoly(tObject *obj, tBitmapIndex bitmap, int lighted);

// Deletes all objects that have been marked for death.
void DeleteAllObjsThatShouldBeDead();

// Toggles whether or not lock-boxes draw.
void object_toggle_lock_targets();

// move all objects for the current frame
int MoveAllObjects();     // moves all objects

// set viewer tObject to next tObject in array
void object_goto_next_viewer();

// draw target boxes for nearby robots
void object_render_targets(void);

// move an tObject for the current frame
int MoveOneObject(tObject * obj);

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
// objp->nSegment), stuff in a shortpos structure.  See typedef
// shortpos.
extern void CreateShortPos(shortpos *spp, tObject *objp, int swap_bytes);

// Extract information from a shortpos, stuff in objp->orient
// (matrix), objp->pos, objp->nSegment
extern void ExtractShortPos(tObject *objp, shortpos *spp, int swap_bytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void clear_transientObjects(int clear_all);

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
void AttachObject(tObject *parent,tObject *sub);

extern void CreateSmallFireballOnObject(tObject *objp, fix size_scale, int soundFlag);

// returns tObject number
int DropMarkerObject(vmsVector *pos, short nSegment, vmsMatrix *orient, ubyte marker_num);

extern void WakeupRenderedObjects(tObject *gmissp, int window_num);

extern void AdjustMineSpawn();

void ResetPlayerObject(void);
void StopObjectMovement (tObject *obj);
void StopPlayerMovement (void);

void DoSmokeFrame (void);
void InitObjectSmoke (void);
void ResetPlayerSmoke (void);
void ResetRobotSmoke (void);

void ObjectGotoNextViewer();
void ObjectGotoPrevViewer();
void KillPlayerSmoke (int i);


void ResetChildObjects (void);
int AddChildObjectN (int nParent, int nChild);
int AddChildObjectP (tObject *pParent, tObject *pChild);
int DelObjChildrenN (int nParent);
int DelObjChildrenP (tObject *pParent);
int DelObjChildN (int nChild);
int DelObjChildP (tObject *pChild);

tObjectRef *GetChildObjN (short nParent, tObjectRef *pChildRef);
tObjectRef *GetChildObjP (tObject *pParent, tObjectRef *pChildRef);

void RenderTargetIndicator (tObject *objP, tRgbColorf *pc);
void CalcShipThrusterPos (tObject *objP, vmsVector *vPos);

tObject *ObjFindFirstOfType (int nType);

void InitWeaponFlags (void);

extern ubyte bIsMissile [];

#define OBJ_CLOAKED(_objP)	((_objP)->ctype.aiInfo.flags [6])

#endif
