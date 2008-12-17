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
#define OBJ_NONE        255 // unused CObject
#define OBJ_WALL        0   // A tWall... not really an CObject, but used for collisions
#define OBJ_FIREBALL    1   // a fireball, part of an explosion
#define OBJ_ROBOT       2   // an evil enemy
#define OBJ_HOSTAGE     3   // a hostage you need to rescue
#define OBJ_PLAYER      4   // the CPlayerData on the console
#define OBJ_WEAPON      5   // a laser, missile, etc
#define OBJ_CAMERA      6   // a camera to slew around with
#define OBJ_POWERUP     7   // a powerup you can pick up
#define OBJ_DEBRIS      8   // a piece of robot
#define OBJ_REACTOR     9   // the control center
#define OBJ_FLARE       10  // a flare
#define OBJ_CLUTTER     11  // misc objects
#define OBJ_GHOST       12  // what the CPlayerData turns into when dead
#define OBJ_LIGHT       13  // a light source, & not much else
#define OBJ_COOP        14  // a cooperative CPlayerData CObject.
#define OBJ_MARKER      15  // a map marker
#define OBJ_CAMBOT		16	 // a camera
#define OBJ_MONSTERBALL	17	 // a monsterball
#define OBJ_SMOKE			18	 // static smoke
#define OBJ_EXPLOSION	19	 // static explosion particleEmitters
#define OBJ_EFFECT		20	 // lightnings

// WARNING!! If you add a nType here, add its name to ObjectType_names
// in CObject.c
#define MAX_OBJECT_TYPES 21

// Result types
#define RESULT_NOTHING  0   // Ignore this collision
#define RESULT_CHECK    1   // Check for this collision

// Control types - what tells this CObject what do do
#define CT_NONE         0   // doesn't move (or change movement)
#define CT_AI           1   // driven by AI
#define CT_EXPLOSION    2   // explosion sequencer
#define CT_FLYING       4   // the CPlayerData is flying
#define CT_SLEW         5   // slewing
#define CT_FLYTHROUGH   6   // the flythrough system
#define CT_WEAPON       9   // laser, etc.
#define CT_REPAIRCEN    10  // under the control of the repair center
#define CT_MORPH        11  // this CObject is being morphed
#define CT_DEBRIS       12  // this is a piece of debris
#define CT_POWERUP      13  // animating powerup blob
#define CT_LIGHT        14  // doesn't actually do anything
#define CT_REMOTE       15  // controlled by another net CPlayerData
#define CT_CNTRLCEN     16  // the control center/main reactor
#define CT_CAMERA			17

// Movement types
#define MT_NONE         0   // doesn't move
#define MT_PHYSICS      1   // moves by physics
#define MT_STATIC			2	 // completely still and immoveable
#define MT_SPINNING     3   // this CObject doesn't move, just sits and spins

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

#define SINGLE_LIGHT_ID		0
#define CLUSTER_LIGHT_ID	1

// misc CObject flags
#define OF_EXPLODING        1   // this CObject is exploding
#define OF_SHOULD_BE_DEAD   2   // this CObject should be dead, so next time we can, we should delete this CObject.
#define OF_DESTROYED        4   // this has been killed, and is showing the dead version
#define OF_SILENT           8   // this makes no sound when it hits a tWall.  Added by MK for weapons, if you extend it to other types, do it completely!
#define OF_ATTACHED         16  // this CObject is a fireball attached to another CObject
#define OF_HARMLESS         32  // this CObject does no damage.  Added to make quad lasers do 1.5 damage as normal lasers.
#define OF_PLAYER_DROPPED   64  // this CObject was dropped by the CPlayerData...
#define OF_ARMAGEDDON		 128 // destroyed by cheat

// Different Weapon ID types...
#define WEAPON_ID_LASER         0
#define WEAPON_IDMSLLE        1
#define WEAPON_ID_CANNONBALL    2

// Object Initial shields...
#define OBJECT_INITIAL_SHIELDS F1_0/2

// physics flags
#define PF_TURNROLL         0x01    // roll when turning
#define PF_LEVELLING        0x02    // level CObject with closest tSide
#define PF_BOUNCE           0x04    // bounce (not slide) when hit will
#define PF_WIGGLE           0x08    // wiggle while flying
#define PF_STICK            0x10    // CObject sticks (stops moving) when hits tWall
#define PF_PERSISTENT       0x20    // CObject keeps going even after it hits another CObject (eg, fusion cannon)
#define PF_USES_THRUST      0x40    // this CObject uses its thrust
#define PF_HAS_BOUNCED      0x80    // Weapon has bounced once.
#define PF_FREE_SPINNING    0x100   // Drag does not apply to rotation of this CObject
#define PF_BOUNCES_TWICE    0x200   // This weapon bounces twice, then dies

#define IMMORTAL_TIME   0x3fffffff  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.
#define ONE_FRAME_TIME  0x3ffffffe  // Objects with this lifeleft will live for exactly one frame

#define MAX_VELOCITY I2X(50)

#define PF_SPAT_BY_PLAYER   1 //this powerup was spat by the CPlayerData

extern char szObjectTypeNames [MAX_OBJECT_TYPES][10];

// List of objects rendered last frame in order.  Created at render
// time, used by homing missiles in laser.c
#define MAX_RENDERED_OBJECTS    100

/*
 * STRUCTURES
 */

// A compressed form for sending crucial data about via slow devices,
// such as modems and buggies.
typedef struct tShortPos {
	sbyte   orient [9];
	short   pos [3];
	short   nSegment;
	short   vel [3];
} tShortPos;

class CShortPos {
	private:
		tShortPos	m_pos;
	public:
		inline sbyte GetOrient (int i) { return m_pos.orient [i]; }
		inline short GetPos (int i) { return m_pos.pos [i]; }
		inline short GetSegment (void) { return m_pos.nSegment; }
		inline short GetVel (int i) { return m_pos.vel [i]; }
		inline void SetOrient (sbyte orient, int i) { m_pos.orient [i] = orient; }
		inline void SetPos (short pos, int i) { m_pos.pos [i] = pos; }
		inline void SetSegment (short nSegment) { m_pos.nSegment = nSegment; }
		inline void SetVel (short vel, int i) { m_pos.vel [i] = vel; }
};

// This is specific to the tShortPos extraction routines in gameseg.c.
#define RELPOS_PRECISION    10
#define MATRIX_PRECISION    9
#define MATRIX_MAX          0x7f    // This is based on MATRIX_PRECISION, 9 => 0x7f

#if 0
class MovementInfo { };
class PhysicsMovementInfo : public MovementInfo { };
class SpinMovementInfo    : public MovementInfo { };

class ControlInfo { };
class ControlLaserInfo : public ControlInfo { };
class ControlExplosionInfo : public ControlInfo { };
class ControlAIStaticInfo : public ControlInfo { };
class ControlLightInfo : public ControlInfo { };     // why put this here?  Didn't know what else to do with it.
class ControlPowerupInfo : public ControlInfo { };

class RenderInfo { };
class RenderPolyObjInfo : public RenderInfo { };      // polygon model
class RenderVClipInfo : public RenderInfo { };     // tVideoClip
class RenderSmokeInfo : public RenderInfo { };
class RenderLightningInfo : public RenderInfo { };
#endif

// information for physics sim for an CObject
typedef struct tPhysicsInfo {
	CFixVector	velocity;   // velocity vector of this CObject
	CFixVector	thrust;     // constant force applied to this CObject
	fix         mass;       // the mass of this CObject
	fix         drag;       // how fast this slows down
	fix         brakes;     // how much brakes applied
	CFixVector	rotVel;     // rotational velecity (angles)
	CFixVector	rotThrust;  // rotational acceleration
	fixang      turnRoll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
} tPhysicsInfo;

class CPhysicsInfo {
	private:
		tPhysicsInfo	m_info;
	public:
		inline tPhysicsInfo* GetInfo (void) { return &m_info; };
		inline CFixVector GetVelocity (void) { return m_info.velocity; }
		inline CFixVector GetThrust (void) { return m_info.thrust; }
		inline CFixVector GetRotVel (void) { return m_info.rotVel; }
		inline CFixVector GetRotThrust (void) { return m_info.rotThrust; }
		inline fix GetMass (void) { return m_info.mass; }
		inline fix GetDrag (void) { return m_info.drag; }
		inline fix GetBrakes (void) { return m_info.brakes; }
		inline fixang GetTurnRoll (void) { return m_info.turnRoll; }
		inline ushort GetFlags (void) { return m_info.flags; }
		inline void SetVelocity (CFixVector* velocity) { m_info.velocity = *velocity; }
		inline void SetThrust (CFixVector* thrust) { m_info.thrust = *thrust; }
		inline void SetRotVel (CFixVector* rotVel) { m_info.rotVel = *rotVel; }
		inline void SetRotThrust (CFixVector* rotThrust) { m_info.rotThrust = *rotThrust; }
		inline void SetMass (fix mass) { m_info.mass = mass; }
		inline void SetDrag (fix drag) { m_info.drag = drag; }
		inline void SetBrakes (fix brakes) { m_info.brakes = brakes; }
		inline void SetTurnRoll (fixang turnRoll) { m_info.turnRoll = turnRoll; }
		inline void SetFlags (ushort flags) { m_info.flags = flags; }
};
// stuctures for different kinds of simulation

typedef struct nParentInfo {
	short		nType;
	short		nObject;
	int		nSignature;
} tParentInfo;

typedef struct tLaserInfo  {
	tParentInfo	parent;
	fix			xCreationTime;      // Absolute time of creation.
	short			nLastHitObj;       // For persistent weapons (survive CObject collision), CObject it most recently hit.
	short			nHomingTarget;				// Object this CObject is tracking.
	fix			xScale;        // Power if this is a fusion bolt (or other super weapon to be added).
} tLaserInfo;

class CLaserInfo {
	private:
		tLaserInfo	m_info;
	public:
		inline tLaserInfo* GetInfo (void) { return &m_info; };
		inline short GetParentType (void) { return m_info.parent.nType; }
		inline short GetParentObj (void) { return m_info.parent.nObject; }
		inline int GetParentSig (void) { return m_info.parent.nSignature; }
		inline short GetLastHitObj (void) { return m_info.nLastHitObj; }
		inline short GetMslLock (void) { return m_info.nHomingTarget; }
		inline fix GetCreationTime (void) { return m_info.xCreationTime; }
		inline fix GetScale (void) { return m_info.xScale; }
		inline void SetParentType (short nType) { m_info.parent.nType = nType; }
		inline void SetParentObj (short nObject) { m_info.parent.nObject = nObject; }
		inline void SetParentSig (int nSignature) { m_info.parent.nSignature = nSignature; }
		inline void SetLastHitObj (short nLastHitObj) { m_info.nLastHitObj = nLastHitObj; }
		inline void SetMslLock (short nHomingTarget) { m_info.nHomingTarget = nHomingTarget; }
		inline void SetCreationTime (fix xCreationTime) { m_info.xCreationTime = xCreationTime; }
		inline void SetScale (fix xScale) { m_info.xScale = xScale; }
};

typedef struct tAttachedObjInfo {
	short	nParent;	// explosion is attached to this CObject
	short	nPrev;	// previous explosion in attach list
	short	nNext;	// next explosion in attach list
} tAttachedObjInfo;

class CAttachedInfo {
	private:
		tAttachedObjInfo	m_info;
	public:
		inline tAttachedObjInfo* GetInfo (void) { return &m_info; };
		inline short GetParent (void) { return m_info.nParent; }
		inline short GetPrevAttached (void) { return m_info.nPrev; }
		inline short GetNextAttached (void) { return m_info.nNext; }
		inline void SetParent (short nParent) { m_info.nParent = nParent; }
		inline void SetPrevAttached (short nPrev) { m_info.nPrev = nPrev; }
		inline void SetNextAttached (short nNext) { m_info.nNext = nNext; }
};

typedef struct tExplosionInfo {
    fix     nSpawnTime;       // when lifeleft is < this, spawn another
    fix     nDeleteTime;      // when to delete CObject
    short   nDeleteObj;			// and what CObject to delete
	 tAttachedObjInfo	attached;
} tExplosionInfo;

class CExplosionInfo : public CAttachedInfo {
	private:
		tExplosionInfo	m_info;
	public:
		inline tExplosionInfo* GetInfo (void) { return &m_info; };
		inline fix GetSpawnTime (void) { return m_info.nSpawnTime; }
		inline fix GetDeleteTime (void) { return m_info.nDeleteTime; }
		inline short GetDeleteObj (void) { return m_info.nDeleteObj; }
		inline void SetSpawnTime (fix nSpawnTime) { m_info.nSpawnTime = nSpawnTime; }
		inline void SetDeleteTime (fix nDeleteTime) { m_info.nDeleteTime = nDeleteTime; }
		inline void SetDeleteObj (fix nDeleteObj) { m_info.nDeleteObj = nDeleteObj; }
};

typedef struct tObjLightInfo {
    fix				intensity;  // how bright the light is
	 short			nSegment;
	 short			nObjects;
	 tRgbaColorf	color;
} tObjLightInfo;

class CObjLightInfo {
	private:
		tObjLightInfo	m_info;
	public:
		inline tObjLightInfo* GetInfo (void) { return &m_info; };
		inline fix GetIntensity (void) { return m_info.intensity; }
		inline short GetSegment (void) { return m_info.nSegment; }
		inline short GetObjects (void) { return m_info.nObjects; }
		inline tRgbaColorf* GetColor (void) { return &m_info.color; }
		inline void SetIntensity (fix intensity) { m_info.intensity = intensity; }
		inline void SetSegment (short nSegment) { m_info.nSegment = nSegment; }
		inline void SetObjects (short nObjects) { m_info.nObjects = nObjects; }
		inline void SetColor (tRgbaColorf *color) { m_info.color = *color; }
};

typedef struct tPowerupInfo {
	int     nCount;          // how many/much we pick up (vulcan cannon only?)
	fix     xCreationTime;  // Absolute time of creation.
	int     nFlags;          // spat by CPlayerData?
}  tPowerupInfo;

class CPowerupInfo {
	private:
		tPowerupInfo	m_info;
	public:
		inline tPowerupInfo* GetInfo (void) { return &m_info; };
		inline int GetCount (void) { return m_info.nCount; }
		inline fix GetCreationTime (void) { return m_info.xCreationTime; }
		inline int GetFlags (void) { return m_info.nFlags; }
		inline void SetCount (int nCount) { m_info.nCount = nCount; }
		inline void SetCreationTime (fix xCreationTime) { m_info.xCreationTime = xCreationTime; }
		inline void SetFlags (int nFlags) { m_info.nFlags = nFlags; }
};

typedef struct tVClipInfo {
public:
	int     nClipIndex;
	fix	  xTotalTime;
	fix     xFrameTime;
	sbyte   nCurFrame;
} tVClipInfo;

class CVClipInfo {
	private:
		tVClipInfo	m_info;
	public:
		inline tVClipInfo* GetInfo (void) { return &m_info; };
		inline int GetClipIndex (void) { return m_info.nClipIndex; }
		inline fix GetTotalTime (void) { return m_info.xTotalTime; }
		inline fix GetFrameTime (void) { return m_info.xFrameTime; }
		inline sbyte GetCurFrame (void) { return m_info.nCurFrame; }
};

#define SMOKE_TYPE_SMOKE	0
#define SMOKE_TYPE_SPRAY	1
#define SMOKE_TYPE_BUBBLES	2

typedef struct tParticleInfo {
public:
	int			nLife;
	int			nSize [2];
	int			nParts;
	int			nSpeed;
	int			nDrift;
	int			nBrightness;
	tRgbaColorb	color;
	char			nSide;
	char			nType;
} tParticleInfo;

class CSmokeInfo {
	private:
		tParticleInfo	m_info;
	public:
		inline tParticleInfo* GetInfo (void) { return &m_info; };
		inline int GetLife (void) { return m_info.nLife; }
		inline int GetSize (int i) { return m_info.nSize [i]; }
		inline int GetParts (void) { return m_info.nParts; }
		inline int GetSpeed (void) { return m_info.nSpeed; }
		inline int GetDrift (void) { return m_info.nDrift; }
		inline int GetBrightness (void) { return m_info.nBrightness; }
		inline tRgbaColorb GetColor (void) { return m_info.color; }
		inline char GetSide (void) { return m_info.nSide; }
};

typedef struct tLightningInfo {
public:
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
} tLightningInfo;

class CLightningInfo {
	private:
		tLightningInfo	m_info;
	public:
		inline tLightningInfo* GetInfo (void) { return &m_info; };
		inline int GetLife (void) { return m_info.nLife; }
		inline int GetDelay (void) { return m_info.nDelay; }
		inline int GetLength (void) { return m_info.nLength; }
		inline int GetAmplitude (void) { return m_info.nAmplitude; }
		inline int GetOffset (void) { return m_info.nOffset; }
		inline short GetLightnings (void) { return m_info.nLightnings; }
		inline short GetId (void) { return m_info.nId; }
		inline short GetTarget (void) { return m_info.nTarget; }
		inline short GetNodes (void) { return m_info.nNodes; }
		inline short GetChildren (void) { return m_info.nChildren; }
		inline short GetSteps (void) { return m_info.nSteps; }
		inline char GetAngle (void) { return m_info.nAngle; }
		inline char GetStyle (void) { return m_info.nStyle; }
		inline char GetSmoothe (void) { return m_info.nSmoothe; }
		inline char GetClamp (void) { return m_info.bClamp; }
		inline char GetPlasma (void) { return m_info.bPlasma; }
		inline char GetSound (void) { return m_info.bSound; }
		inline char GetRandom (void) { return m_info.bRandom; }
		inline char GetInPlane (void) { return m_info.bInPlane; }
};

// structures for different kinds of rendering

typedef struct tPolyObjInfo {
public:
	int     		nModel;          // which polygon model
	CAngleVector 	animAngles [MAX_SUBMODELS]; // angles for each subobject
	int     		nSubObjFlags;       // specify which subobjs to draw
	int     		nTexOverride;      // if this is not -1, map all face to this
	int     		nAltTextures;       // if not -1, use these textures instead
} tPolyObjInfo;

class CPolyObjInfo {
	private:
		tPolyObjInfo	m_info;
	public:
		inline tPolyObjInfo* GetInfo (void) { return &m_info; };
		inline int GetModel (void) { return m_info.nModel; }
		inline CAngleVector* GetAnimAngles (int i) { return m_info.animAngles + i; }
		inline int GetSubObjFlags (void) { return m_info.nSubObjFlags; }
		inline int GetTexOverride (void) { return m_info.nTexOverride; }
		inline int GetAltTextures (void) { return m_info.nAltTextures; }
		inline void SetModel (int nModel) { m_info.nModel = nModel; }
		inline void SetAnimAngles (const CAngleVector *vAngles, int i) { m_info.animAngles [i] = *vAngles; }
		inline void SetSubObjFlags (int nSubObjFlags) { m_info.nSubObjFlags = nSubObjFlags; }
		inline void SetTexOverride (int nTexOverride) { m_info.nTexOverride = nTexOverride; }
		inline void SetAltTextures (int nAltTextures) { m_info.nAltTextures = nAltTextures; }
};

typedef struct tObjTransformation {
	CFixVector	vPos;				// absolute x,y,z coordinate of center of object
	CFixMatrix	mOrient;			// orientation of object in world
	} tObjTransformation;

class CObjTransformation {
	private:
		tObjTransformation	m_t;

	public:
		inline CFixVector* GetPos (void) { return &m_t.vPos; }
		inline CFixMatrix* GetOrient (void) { return &m_t.mOrient; }
		inline void SetPos (const CFixVector* vPos) { m_t.vPos = *vPos; }
		inline void SetOrient (const CFixMatrix* mOrient) { m_t.mOrient = *mOrient ; }
	};

typedef struct tObjContainerInfo {
	sbyte			nType;
	sbyte			nId;
	sbyte			nCount;
} tObjContainerInfo;

class CObjContainerInfo {
	private:
		tObjContainerInfo	m_info;
	public:
		inline tObjContainerInfo* GetInfo (void) { return &m_info; };
		inline sbyte GetContainsType (void) { return m_info.nType; }
		inline sbyte GetContainsId (void) { return m_info.nId; }
		inline sbyte GetContainsCount (void) { return m_info.nCount; }
		inline void SetContainsType (sbyte nType) { m_info.nType = nType; }
		inline void SetContainsId (sbyte nId) { m_info.nId = nId; }
		inline void SetContainsCount (sbyte nCount) { m_info.nCount = nCount; }
};

typedef struct tObjectInfo {
	int     				nSignature;    // Every CObject ever has a unique nSignature...
	ubyte   				nType;         // what nType of CObject this is... robot, weapon, hostage, powerup, fireball
	ubyte   				nId;           // which form of CObject...which powerup, robot, etc.
#ifdef WORDS_NEED_ALIGNMENT
	short   				pad;
#endif
	short   				nNextInSeg, 
							nPrevInSeg;  // id of next and previous connected CObject in Objects, -1 = no connection
	ubyte   				controlType;   // how this CObject is controlled
	ubyte   				movementType;  // how this CObject moves
	ubyte   				renderType;    // how this CObject renders
	ubyte   				nFlags;        // misc flags
	short					nSegment;
	short   				nAttachedObj;  // number of attached fireball CObject
	tObjTransformation	position;
	fix     				xSize;         // 3d size of CObject - for collision detection
	fix     				xShields;      // Starts at maximum, when <0, CObject dies..
	CFixVector 			vLastPos;		// where CObject was last frame
	tObjContainerInfo	contains;
	sbyte   				nCreator; // Materialization center that created this CObject, high bit set if matcen-created
	fix     				xLifeLeft;      // how long until goes away, or 7fff if immortal
} tObjectInfo;

// TODO get rid of the structs (former unions) and the union
typedef struct tBaseObject {
	tObjectInfo			info;
	// movement info, determined by MOVEMENT_TYPE
	union {
		tPhysicsInfo	physInfo; // a physics CObject
		CFixVector   	spinRate; // for spinning objects
		} mType;
	// control info, determined by CONTROL_TYPE
	union {
		tLaserInfo		laserInfo;
		tExplosionInfo explInfo;      // NOTE: debris uses this also
		tAIStaticInfo  aiInfo;
		tObjLightInfo  lightInfo;     // why put this here?  Didn't know what else to do with it.
		tPowerupInfo   powerupInfo;
		} cType;
	// render info, determined by RENDER_TYPE
	union {
		tPolyObjInfo   polyObjInfo;      // polygon model
		tVClipInfo     vClipInfo;     // tVideoClip
		tParticleInfo	particleInfo;
		tLightningInfo	lightningInfo;
		} rType;
#ifdef WORDS_NEED_ALIGNMENT
	short   nPad;
#endif
} tBaseObject;

class CObjectInfo : public CObjTransformation, public CObjContainerInfo, public tBaseObject {
	public:
		CObjectInfo () { memset (&info, 0, sizeof (info)); }
#if 0
	private:
		tBaseObject	m_object;

	public:
		inline tBaseObject* GetInfo (void) { return &info; }; 
		inline void GetInfo (tBaseObject* infoP) { info = *infoP; }; 
#endif

	public:
		inline int Signature () { return info.nSignature; }
		inline ubyte Id () { return info.nId; }
		inline fix Size () { return info.xSize; }
		inline fix Shields () { return info.xShields; }
		inline fix LifeLeft () { return info.xLifeLeft; }
		inline short Segment () { return info.nSegment; }
		inline short AttachedObj () { return info.nAttachedObj; }
		inline short NextInSeg () { return info.nNextInSeg; }
		inline short PrevInSeg () { return info.nPrevInSeg; }
		inline sbyte Creator () { return info.nCreator; }
		inline ubyte Type () { return info.nType; }
		inline ubyte ControlType () { return info.controlType; }
		inline ubyte MovementType () { return info.movementType; }
		inline ubyte RenderType () { return info.renderType; }
		inline ubyte Flags () { return info.nFlags; }
		inline CFixVector LastPos () { return info.vLastPos; }

		inline void SetSignature (int nSignature) { info.nSignature = nSignature; }
		inline void SetId (ubyte nId) { info.nId = nId; }
		inline void SetSize (fix xSize) { info.xSize = xSize; }
		inline void SetShields (fix xShields) { info.xShields = xShields; }
		inline void SetLifeLeft (fix xLifeLeft) { info.xLifeLeft = xLifeLeft; }
		inline void SetSegment (short nSegment) { info.nSegment = nSegment; }
		inline void SetAttachedObj (short nAttachedObj) { info.nAttachedObj = nAttachedObj; }
		inline void SetNextInSeg (short nNextInSeg) { info.nNextInSeg = nNextInSeg; }
		inline void SetPrevInSeg ( short nPrevInSeg) { info.nPrevInSeg = nPrevInSeg; }
		inline void SetCreator (sbyte nCreator) { info.nCreator = nCreator; }
		inline void SetType (ubyte nType) { info.nType = nType; }
		inline void SetControlType (ubyte controlType) { info.controlType = controlType; }
		inline void SetMovementType (ubyte movementType) { info.movementType = movementType; }
		inline void SetRenderType (ubyte renderType) { info.renderType = renderType; }
		inline void SetFlags (ubyte nFlags) { info.nFlags = nFlags; }
		inline void SetLastPos (const CFixVector *vLastPos) { info.vLastPos = *vLastPos; }
};

struct tObject;

typedef struct tObjListLink {
	tObject	*prev, *next;
} tObjListLink;

typedef struct tShotInfo {
	short					nObject;
	int					nSignature;
} tShotInfo;

typedef struct tObject : public tBaseObject {
	tObjListLink	links [3];		// link into list of objects in same category (0: all, 1: same type, 2: same class)
	ubyte				nLinkedType;
	ubyte				nTracers;
	fix				xCreationTime;
	fix				xTimeLastHit;
	tShotInfo		shots;
	CFixVector		vStartVel;
} tObject;

class CObject;

class CObjListLink {
	public:
		CObject	*prev, *next;
};

typedef struct tObjListRef {
	CObject	*head, *tail;
	short		nObjects;
} tObjListRef;

class CObject : public CObjectInfo {
	private:
		short				m_nId;
		CObject			*m_prev, *m_next;
		CObjListLink	m_links [3];		// link into list of objects in same category (0: all, 1: same type, 2: same class)
		ubyte				m_nLinkedType;
		ubyte				m_nTracers;
		fix				m_xCreationTime;
		fix				m_xTimeLastHit;
		tShotInfo		m_shots;
		CFixVector		m_vStartVel;

	public:
		CObject ();
		~CObject ();
		// initialize a new CObject.  adds to the list for the given CSegment
		// returns the CObject number
		int Create (ubyte nType, ubyte nId, short nCreator, short nSegment, const CFixVector& vPos,
						const CFixMatrix& mOrient, fix xSize, ubyte cType, ubyte mType, ubyte rType);

		inline void Kill (void) { SetFlags (Flags () | OF_SHOULD_BE_DEAD); }
		inline bool Exists (void) { return !(Flags () & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)); }
		// unlinks an CObject from a CSegment's list of objects
		void Init (void);
		void Link (void);
		void Unlink (void);
		void Link (tObjListRef& ref, int nLink);
		void Unlink (tObjListRef& ref, int nLink);
#if DBG
		bool IsInList (tObjListRef& ref, int nLink);
#endif
		void SetType (ubyte nNewType);
		void LinkToSeg (int nSegment);
		void UnlinkFromSeg (void);
		void RelinkToSeg (int nNewSeg);
		bool IsLinkedToSeg (short nSegment);
		void Initialize (ubyte nType, ubyte nId, short nCreator, short nSegment, const CFixVector& vPos,
							  const CFixMatrix& mOrient, fix xSize, ubyte cType, ubyte mType, ubyte rType);
		void ToBaseObject (tBaseObject *objP);

		inline short Id (void) { return m_nId; }
		inline CObject* Prev (void) { return m_prev; }
		inline CObject* Next (void) { return m_next; }
		inline CObjListLink& Links (uint i) { return m_links [i]; }
		inline ubyte LinkedType (void) { return m_nLinkedType; }
		inline ubyte Tracers (void) { return m_nTracers; }
		inline fix CreationTime (void) { return m_xCreationTime; }
		inline fix TimeLastHit (void) { return m_xTimeLastHit; }
		inline tShotInfo& Shots (void) { return m_shots; }
		inline CFixVector StartVel (void) { return m_vStartVel; }

		inline void SetId (short nId) { m_nId = nId; }
		inline void SetPrev (CObject* prev) { m_prev = prev; }
		inline void SetNext (CObject* next) { m_next = next; }
		inline void SetLinkedType (ubyte nLinkedType) { m_nLinkedType = nLinkedType; }
		inline void SetTracers (ubyte nTracers) { m_nTracers = nTracers; }
		inline void SetCreationTime (fix xCreationTime) { m_xCreationTime = xCreationTime; }
		inline void SetTimeLastHit (fix xTimeLastHit) { m_xTimeLastHit = xTimeLastHit; }
		inline void SetStartVel (CFixVector* vStartVel) { m_vStartVel = *vStartVel; }

		inline void InitLinks (void) { memset (m_links, 0, sizeof (m_links)); }

		void Read (CFile& cf);
		//inline short Index (void) { return gameData.objs.objects.Index (this); }
};

inline int operator- (CObject* o, CArray<CObject>& a) { return a.Index (o); }

#if 0

class CRobotObject : public CObject, public CPhysicsInfo, public CAIStaticInfo, public CPolyObjInfo {
	public:
		CRobotObject () {}
		~CRobotObject () {}
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

class CPowerupObject : public CObject, public CPhysicsInfo, public CPolyObjInfo {
	public:
		CPowerupObject () {}
		~CPowerupObject () {}
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

class CWeaponObject : public CObject, public CPhysicsInfo, public CPolyObjInfo {
	public:
		CWeaponObject () {}
		~CWeaponObject () {}
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

class CLightObject : public CObject, public CObjLightInfo {
	public:
		CLightObject () {};
		~CLightObject () {};
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

class CLightningObject : public CObject, public CLightningInfo {
	public:
		CLightningObject () {};
		~CLightningObject () {};
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

class CParticleObject : public CObject, public CSmokeInfo {
	public:
		CParticleObject () {};
		~CParticleObject () {};
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

#endif


typedef struct tObjPosition {
	tObjTransformation	position;
	short						nSegment;     // CSegment number containing CObject
	short						nSegType;		// nType of CSegment
} tObjPosition;

class CObjPosition : public CObjTransformation {
	private:
		short		m_nSegment;
		short		m_nSegType;
	public:
		inline short& Segment () { return m_nSegment; }
		inline short& SegType () { return m_nSegType; }
};

typedef struct tWindowRenderedData {
	int     nFrame;
	CObject *viewerP;
	int     bRearView;
	int     nUser;
	int     nObjects;
	short   renderedObjects [MAX_RENDERED_OBJECTS];
} tWindowRenderedData;

class WIndowRenderedData {
	private:
		tWindowRenderedData	m_data;
	public:
		inline int& Frame () { return m_data.nFrame; }
		inline int& RearView () { return m_data.bRearView; }
		inline int& User () { return m_data.nUser; }
		inline int& Objects () { return m_data.nObjects; }
		inline short& RenderedObjects (int i) { return m_data.renderedObjects [i]; }
		inline CObject *Viewer () { return m_data.viewerP; }
};

typedef struct tObjDropInfo {
	time_t	nDropTime;
	short		nPowerupType;
	short		nPrevPowerup;
	short		nNextPowerup;
	short		nObject;
} tObjDropInfo;

class CObjDropInfo {
	private:
		tObjDropInfo	m_info;
	public:
		inline time_t& Time () { return m_info.nDropTime; }
		inline short& Type () { return m_info.nPowerupType; }
		inline short& Prev () { return m_info.nPrevPowerup; }
		inline short& Next () { return m_info.nNextPowerup; }
		inline short& Object () { return m_info.nObject; }
};

class tObjectRef {
public:
	short		objIndex;
	short		nextObj;
};

#define MAX_RENDERED_WINDOWS    3

extern tWindowRenderedData windowRenderedData [MAX_RENDERED_WINDOWS];

/*
 * VARIABLES
 */

// ie gameData.objs.collisionResult[a][b]==  what happens to a when it collides with b

extern char *robot_names[];         // name of each robot

extern CObject Follow;

/*
 * FUNCTIONS
 */


// do whatever setup needs to be done
void InitObjects();

int CreateObject (ubyte nType, ubyte nId, short nCreator, short nSegment, const CFixVector& vPos, const CFixMatrix& mOrient,
					   fix xSize, ubyte cType, ubyte mType, ubyte rType);
int CloneObject (CObject *objP);
int CreateRobot (ubyte nId, short nSegment, const CFixVector& vPos);
int CreatePowerup (ubyte nId, short nCreator, short nSegment, const CFixVector& vPos, int bIgnoreLimits);
int CreateWeapon (ubyte nId, short nCreator, short nSegment, const CFixVector& vPos, fix xSize, ubyte rType);
int CreateFireball (ubyte nId, short nSegment, const CFixVector& vPos, fix xSize, ubyte rType);
int CreateDebris (CObject *parentP, short nSubModel);
int CreateCamera (CObject *parentP);
int CreateLight (ubyte nId, short nSegment, const CFixVector& vPos);
// returns CSegment number CObject is in.  Searches out from CObject's current
// seg, so this shouldn't be called if the CObject has "jumped" to a new seg
// -- unused --
//int obj_get_new_seg(CObject *obj);

// when an CObject has moved into a new CSegment, this function unlinks it
// from its old CSegment, and links it into the new CSegment
void RelinkObjToSeg (int nObject, int nNewSeg);

void ResetSegObjLists (void);
void LinkAllObjsToSegs (void);
void RelinkAllObjsToSegs (void);
bool CheckSegObjList (CObject *objP, short nObject, short nFirstObj);

// move an CObject from one CSegment to another. unlinks & relinks
// -- unused --
//void obj_set_new_seg(int nObject,int newsegnum);

// links an CObject into a CSegment's list of objects.
// takes CObject number and CSegment number
void LinkObjToSeg(int nObject,int nSegment);

// unlinks an CObject from a CSegment's list of objects
void UnlinkObjFromSeg (CObject *objP);

// initialize a new CObject.  adds to the list for the given CSegment
// returns the CObject number
//int CObject::Create(ubyte nType, char id, short owner, short nSegment, const CFixVector& pos,
//               const CFixMatrix& orient, fix size, ubyte ctype, ubyte mtype, ubyte rtype, int bIgnoreLimits);

// make a copy of an CObject. returs num of new CObject
int ObjectCreateCopy(int nObject, CFixVector *new_pos, int newsegnum);

// remove CObject from the world
void ReleaseObject(short nObject);

// called after load.  Takes number of objects, and objects should be
// compressed
void ResetObjects (int nObjects);
void ConvertObjects (void);
void SetupEffects (void);

// make CObject array non-sparse
void compressObjects(void);

// Draw a blob-nType CObject, like a fireball
// Deletes all objects that have been marked for death.
void CleanupObjects();

// Toggles whether or not lock-boxes draw.
void object_toggle_lock_targets();

// move all objects for the current frame
int UpdateAllObjects();     // moves all objects

// set viewer CObject to next CObject in array
void object_goto_nextViewer();

// draw target boxes for nearby robots
void object_render_targets(void);

// move an CObject for the current frame
int UpdateObject(CObject * obj);

// make object0 the CPlayerData, setting all relevant fields
void InitPlayerObject();

// check if CObject is in CObject->nSegment.  if not, check the adjacent
// segs.  if not any of these, returns false, else sets obj->nSegment &
// returns true callers should really use FindVectorIntersection()
// Note: this function is in gameseg.c
int UpdateObjectSeg(CObject *objP, bool bMove = true);


// Finds what CSegment *obj is in, returns CSegment number.  If not in
// any CSegment, returns -1.  Note: This function is defined in
// gameseg.h, but object[HA] depends on gameseg.h, and object[HA] is where
// CObject is defined...get it?
int FindObjectSeg(CObject * obj);

// go through all objects and make sure they have the correct CSegment
// numbers used when debugging is on
void FixObjectSegs();

// Drops objects contained in objp.
int ObjectCreateEgg(CObject *objp);

// Interface to ObjectCreateEgg, puts count objects of nType nType, id
// = id in objp and then drops them.
int CallObjectCreateEgg(CObject *objp, int count, int nType, int id);

extern void DeadPlayerEnd(void);

// Extract information from an CObject (objp->orient, objp->pos,
// objp->nSegment), stuff in a tShortPos structure.  See typedef
// tShortPos.
extern void CreateShortPos(tShortPos *spp, CObject *objp, int swap_bytes);

// Extract information from a tShortPos, stuff in objp->orient
// (matrix), objp->pos, objp->nSegment
extern void ExtractShortPos(CObject *objp, tShortPos *spp, int swap_bytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void ClearTransientObjects(int clear_all);

// returns the number of a free CObject, updating HighestObject_index.
// Generally, CObject::Create() should be called to get an CObject, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
int AllocObject(void);
int InsertObject (int nObject);

// frees up an CObject.  Generally, ReleaseObject() should be called to
// get rid of an CObject.  This function deallocates the CObject entry
// after the CObject has been unlinked
void FreeObject(int nObject);

// after calling initObject(), the network code has grabbed specific
// CObject slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void SpecialResetObjects(void);

// attaches an CObject, such as a fireball, to another CObject, such as
// a robot
void AttachObject(CObject *parent, CObject *sub);

extern void CreateSmallFireballOnObject(CObject *objp, fix size_scale, int soundFlag);

// returns CObject number
int DropMarkerObject(CFixVector *pos, short nSegment, CFixMatrix *orient, ubyte marker_num);

extern void WakeupRenderedObjects(CObject *gmissp, int window_num);

extern void AdjustMineSpawn();

void ResetPlayerObject(void);
void StopObjectMovement (CObject *obj);
void StopPlayerMovement (void);

int ObjectSoundClass (CObject *objP);

void ObjectGotoNextViewer();
void ObjectGotoPrevViewer();

int ObjectCount (int nType);

void ResetChildObjects (void);
int AddChildObjectN (int nParent, int nChild);
int AddChildObjectP (CObject *pParent, CObject *pChild);
int DelObjChildrenN (int nParent);
int DelObjChildrenP (CObject *pParent);
int DelObjChildN (int nChild);
int DelObjChildP (CObject *pChild);

void LinkObject (CObject *objP);
void UnlinkObject (CObject *objP);

void BuildObjectModels (void);

tObjectRef *GetChildObjN (short nParent, tObjectRef *pChildRef);
tObjectRef *GetChildObjP (CObject *pParent, tObjectRef *pChildRef);

CObject *ObjFindFirstOfType (int nType);
void InitWeaponFlags (void);
float ObjectDamage (CObject *objP);
int FindBoss (int nObject);
void InitGateIntervals (void);
int CountPlayerObjects (int nPlayer, int nType, int nId);
void FixObjectSizes (void);
void DoSlowMotionFrame (void);
CFixMatrix *ObjectView (CObject *objP);

CFixVector *PlayerSpawnPos (int nPlayer);
CFixMatrix *PlayerSpawnOrient (int nPlayer);
void GetPlayerSpawn (int nPlayer, CObject *objP);
void RecreateThief(CObject *objP);
void DeadPlayerFrame (void);

void SetObjectType (CObject *objP, ubyte nNewType);

extern ubyte bIsMissile [];

#define	OBJ_CLOAKED(_objP)	((_objP)->ctype.aiInfo.flags [6])

#define	SHOW_SHADOWS \
			(!gameStates.render.bRenderIndirect && \
			 gameStates.app.bEnableShadows && \
			 EGI_FLAG (bShadows, 0, 1, 0) && \
			 !COMPETITION)

#define	SHOW_OBJ_FX \
			(!(gameStates.app.bNostalgia || COMPETITION))

#define	IS_BOSS(_objP)			(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).bossFlag)
#define	IS_GUIDEBOT(_objP)	(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).companion)
#define	IS_THIEF(_objP)		(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).thief)
#define	IS_BOSS(_objP)			(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).bossFlag)
#define	IS_BOSS_I(_i)			IS_BOSS (gameData.objs.objects + (_i))
#define	IS_MISSILE(_objP)		(((_objP)->info.nType == OBJ_WEAPON) && gameData.objs.bIsMissile [(_objP)->info.nId])
#define	IS_MISSILE_I(_i)		IS_MISSILE (gameData.objs.objects + (_i))

#if DBG
extern CObject *dbgObjP;
#endif

#define SET_COLLISION(type1, type2, result) \
	gameData.objs.collisionResult [type1][type2] = result; \
	gameData.objs.collisionResult [type2][type1] = result;

#define ENABLE_COLLISION(type1, type2)		SET_COLLISION(type1, type2, RESULT_CHECK)

#define DISABLE_COLLISION(type1, type2)	SET_COLLISION(type1, type2, RESULT_NOTHING)

#define OBJECT_EXISTS(_objP)	 ((_objP) && !((_objP)->Flags() & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)))

#	define FORALL_OBJSi(_objP,_i)						for ((_objP) = OBJECTS.Buffer (), (_i) = 0; (_i) <= gameData.objs.nLastObject [0]; (_i)++, (_objP)++)
#if 0
#	define FORALL_CLASS_OBJS(_type,_objP,_i)		for ((_objP) = OBJECTS, (_i) = 0; i <= gameData.objs.nLastObject [0]; (_i)++, (_objP)++) if ((_objP)->info.nType == _type)
#	define FORALL_ACTOR_OBJS(_objP,_i)				FORALL_CLASS_OBJS (OBJ_ROBOT, _objP, _i)
#	define FORALL_POWERUP_OBJS(_objP,_i)			FORALL_CLASS_OBJS (OBJ_POWERUP, _objP, _i)
#	define FORALL_WEAPON_OBJS(_objP,_i)				FORALL_CLASS_OBJS (OBJ_WEAPON, _objP, _i)
#	define FORALL_EFFECT_OBJS(_objP,_i)				FORALL_CLASS_OBJS (OBJ_EFFECT, _objP, _i)
#	define IS_OBJECT(_objP, _i)						((_i) <= gameData.objs.nLastObject [0])
#else
#	define FORALL_OBJS(_objP,_i)							for ((_objP) = gameData.objs.lists.all.head; (_objP); (_objP) = (_objP)->Links (0).next)
#	define FORALL_SUPERCLASS_OBJS(_list,_objP,_i)	for ((_objP) = (_list).head; (_objP); (_objP) = (_objP)->Links (2).next)
#	define FORALL_CLASS_OBJS(_list,_objP,_i)			for ((_objP) = (_list).head; (_objP); (_objP) = (_objP)->Links (1).next)
#	define FORALL_PLAYER_OBJS(_objP,_i)					FORALL_CLASS_OBJS (gameData.objs.lists.players, _objP, _i)
#	define FORALL_ROBOT_OBJS(_objP,_i)					FORALL_CLASS_OBJS (gameData.objs.lists.robots, _objP, _i)
#	define FORALL_POWERUP_OBJS(_objP,_i)				FORALL_CLASS_OBJS (gameData.objs.lists.powerups, _objP, _i)
#	define FORALL_WEAPON_OBJS(_objP,_i)					FORALL_CLASS_OBJS (gameData.objs.lists.weapons, _objP, _i)
#	define FORALL_EFFECT_OBJS(_objP,_i)					FORALL_CLASS_OBJS (gameData.objs.lists.effects, _objP, _i)
#	define FORALL_LIGHT_OBJS(_objP,_i)					FORALL_CLASS_OBJS (gameData.objs.lists.lights, _objP, _i)
#	define FORALL_ACTOR_OBJS(_objP,_i)					FORALL_SUPERCLASS_OBJS (gameData.objs.lists.actors, _objP, _i)
#	define FORALL_STATIC_OBJS(_objP,_i)					FORALL_SUPERCLASS_OBJS (gameData.objs.lists.statics, _objP, _i)
#	define IS_OBJECT(_objP, _i)							((_objP) != NULL)
#endif

//	-----------------------------------------------------------------------------------------------------------

static inline void KillObject (CObject *objP)
{
objP->info.nFlags |= OF_SHOULD_BE_DEAD;
#if DBG
if (objP == dbgObjP)
	objP = objP;
#endif
}

//	-----------------------------------------------------------------------------------------------------------

#endif
