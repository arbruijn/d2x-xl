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
//#include "segmath.h"
#include "piggy.h"
#include "aistruct.h"
#include "segment.h"
#include "gr.h"
#include "powerup.h"

/*
 * CONSTANTS
 */

#define MAX_OBJECTS_D2  	350 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world
#define MAX_OBJECTS_D2X	   MAX_SEGMENTS_D2X
#define MAX_OBJECTS     	MAX_SEGMENTS
#define MAX_HIT_OBJECTS		20
#define MAX_EXCESS_OBJECTS 15

// Object types
#define OBJ_NONE        255 // unused CObject
#define OBJ_WALL        0   // A CWall... not really an CObject, but used for collisions
#define OBJ_FIREBALL    1   // a fireball, part of an explosion
#define OBJ_ROBOT       2   // an evil enemy
#define OBJ_HOSTAGE     3   // a hostage you need to rescue
#define OBJ_PLAYER      4   // the player on the console
#define OBJ_WEAPON      5   // a laser, missile, etc
#define OBJ_CAMERA      6   // a camera to slew around with
#define OBJ_POWERUP     7   // a powerup you can pick up
#define OBJ_DEBRIS      8   // a piece of robot
#define OBJ_REACTOR     9   // the control center
#define OBJ_FLARE       10  // a flare
#define OBJ_CLUTTER     11  // misc objects
#define OBJ_GHOST       12  // what the player turns into when dead
#define OBJ_LIGHT       13  // a light source, & not much else
#define OBJ_COOP        14  // a cooperative player CObject.
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
#define RESULT_NOTHING		0   // Ignore this collision
#define RESULT_CHECK			1   // Check for this collision

// Control types - what tells this CObject what do do
#define CT_NONE				0   // doesn't move (or change movement)
#define CT_AI					1   // driven by AI
#define CT_EXPLOSION			2   // explosion sequencer
#define CT_FLYING				4   // the player is flying
#define CT_SLEW				5   // slewing
#define CT_FLYTHROUGH		6   // the flythrough system
#define CT_WEAPON				9   // laser, etc.
#define CT_REPAIRCEN			10  // under the control of the repair center
#define CT_MORPH				11  // this CObject is being morphed
#define CT_DEBRIS				12  // this is a piece of debris
#define CT_POWERUP			13  // animating powerup blob
#define CT_LIGHT				14  // doesn't actually do anything
#define CT_REMOTE				15  // controlled by another net player
#define CT_CNTRLCEN			16  // the control center/main reactor
#define CT_WAYPOINT			17
#define CT_CAMERA				18

// Movement types
#define MT_NONE				0   // doesn't move
#define MT_PHYSICS			1   // moves by physics
#define MT_STATIC				2	 // completely still and immoveable
#define MT_SPINNING			3   // this CObject doesn't move, just sits and spins

// Render types
#define RT_NONE				0   // does not render
#define RT_POLYOBJ			1   // a polygon model
#define RT_FIREBALL			2   // a fireball
#define RT_LASER				3   // a laser
#define RT_HOSTAGE			4   // a hostage
#define RT_POWERUP			5   // a powerup
#define RT_MORPH				6   // a robot being morphed
#define RT_WEAPON_VCLIP		7   // a weapon that renders as a tVideoClip
#define RT_THRUSTER			8	 // like afterburner, but doesn't cast light
#define RT_EXPLBLAST			9	 // white explosion light blast
#define RT_SHRAPNELS			10	 // smoke trails coming from explosions
#define RT_SMOKE				11
#define RT_LIGHTNING			12
#define RT_SOUND				13
#define RT_SHOCKWAVE			14  // concentric shockwave effect

#define PARTICLE_ID			0
#define LIGHTNING_ID			1
#define SOUND_ID				2
#define WAYPOINT_ID			3

#define SINGLE_LIGHT_ID		0
#define CLUSTER_LIGHT_ID	1

// misc CObject flags
#define OF_EXPLODING        1   // this CObject is exploding
#define OF_SHOULD_BE_DEAD   2   // this CObject should be dead, so next time we can, we should delete this CObject.
#define OF_DESTROYED        4   // this has been killed, and is showing the dead version
#define OF_SILENT           8   // this makes no sound when it hits a CWall.  Added by MK for weapons, if you extend it to other types, do it completely!
#define OF_ATTACHED         16  // this CObject is a fireball attached to another CObject
#define OF_HARMLESS         32  // this CObject does no damage.  Added to make quad lasers do 1.5 damage as normal lasers.
#define OF_PLAYER_DROPPED   64  // this CObject was dropped by the player...
#define OF_ARMAGEDDON		 128 // destroyed by cheat

// Different Weapon ID types...
#define WEAPON_ID_LASER         0
#define WEAPON_IDMSLLE        1
#define WEAPON_ID_CANNONBALL    2

// Object Initial shield...
#define OBJECT_INITIAL_SHIELDS I2X (1)/2

// physics flags
#define PF_TURNROLL         0x01    // roll when turning
#define PF_LEVELLING        0x02    // level CObject with closest CSide
#define PF_BOUNCE           0x04    // bounce (not slide) when hit will
#define PF_WIGGLE           0x08    // wiggle while flying
#define PF_STICK            0x10    // CObject sticks (stops moving) when hits CWall
#define PF_PERSISTENT       0x20    // CObject keeps going even after it hits another CObject (eg, fusion cannon)
#define PF_USES_THRUST      0x40    // this CObject uses its thrust
#define PF_HAS_BOUNCED      0x80    // Weapon has bounced once.
#define PF_FREE_SPINNING    0x100   // Drag does not apply to rotation of this CObject
#define PF_BOUNCES_TWICE    0x200   // This weapon bounces twice, then dies

#define IMMORTAL_TIME   0x3fffffff  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.
#define ONE_FRAME_TIME  0x3ffffffe  // Objects with this lifeleft will live for exactly one frame

#define ROBOT_IS_HOSTILE	-1
#define ROBOT_IS_NEUTRAL	0
#define ROBOT_IS_FRIENDLY	1

#define MAX_VELOCITY I2X(50)

#define PF_SPAT_BY_PLAYER   1 //this powerup was spat by the player

extern char szObjectTypeNames [MAX_OBJECT_TYPES][10];

// List of objects rendered last frame in order.  Created at render
// time, used by homing missiles in laser.c
#define MAX_RENDERED_OBJECTS    100

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
/*
 * STRUCTURES
 */

typedef struct tLongPos {
	CFixVector			pos;
	CFixMatrix			orient;
	CFixVector			vel;
	CFixVector			rotVel;
	int16_t				nSegment;
} __pack__ tLongPos;

// A compressed form for sending crucial data about via slow devices,
// such as modems and buggies.
typedef struct tShortPos {
	int8_t		orient [9];
	int16_t		pos [3];
	int16_t		nSegment;
	int16_t		vel [3];
} __pack__ tShortPos;

class CShortPos {
	private:
		tShortPos	m_pos;
	public:
		inline int8_t GetOrient (int32_t i) { return m_pos.orient [i]; }
		inline int16_t GetPos (int32_t i) { return m_pos.pos [i]; }
		inline int16_t GetSegment (void) { return m_pos.nSegment; }
		inline int16_t GetVel (int32_t i) { return m_pos.vel [i]; }
		inline void SetOrient (int8_t orient, int32_t i) { m_pos.orient [i] = orient; }
		inline void SetPos (int16_t pos, int32_t i) { m_pos.pos [i] = pos; }
		inline void SetSegment (int16_t nSegment) { m_pos.nSegment = nSegment; }
		inline void SetVel (int16_t vel, int32_t i) { m_pos.vel [i] = vel; }
};

//	-----------------------------------------------------------------------------
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

//	-----------------------------------------------------------------------------
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
	uint16_t      flags;      // misc physics flags
} __pack__ tPhysicsInfo;

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
		inline uint16_t GetFlags (void) { return m_info.flags; }
		inline void SetVelocity (CFixVector* velocity) { m_info.velocity = *velocity; }
		inline void SetThrust (CFixVector* thrust) { m_info.thrust = *thrust; }
		inline void SetRotVel (CFixVector* rotVel) { m_info.rotVel = *rotVel; }
		inline void SetRotThrust (CFixVector* rotThrust) { m_info.rotThrust = *rotThrust; }
		inline void SetMass (fix mass) { m_info.mass = mass; }
		inline void SetDrag (fix drag) { m_info.drag = drag; }
		inline void SetBrakes (fix brakes) { m_info.brakes = brakes; }
		inline void SetTurnRoll (fixang turnRoll) { m_info.turnRoll = turnRoll; }
		inline void SetFlags (uint16_t flags) { m_info.flags = flags; }
};

//	-----------------------------------------------------------------------------
// stuctures for different kinds of simulation

typedef struct nParentInfo {
	int16_t		nType;
	int16_t		nObject;
	int32_t		nSignature;
} __pack__ tParentInfo;

typedef struct tLaserInfo  {
	tParentInfo	parent;
	fix			xCreationTime;    // Absolute time of creation.
	int16_t			nLastHitObj;      // For persistent weapons (survive CObject collision), CObject it most recently hit.
	int16_t			nHomingTarget;		// Object this CObject is tracking.
	fix			xScale;				// Power if this is a fusion bolt (or other super weapon to be added).
} __pack__ tLaserInfo;

class CLaserInfo {
	private:
		tLaserInfo	m_info;
	public:
		inline tLaserInfo* GetInfo (void) { return &m_info; };
		inline int16_t GetParentType (void) { return m_info.parent.nType; }
		inline int16_t GetParentObj (void) { return m_info.parent.nObject; }
		inline int32_t GetParentSig (void) { return m_info.parent.nSignature; }
		inline int16_t GetLastHitObj (void) { return m_info.nLastHitObj; }
		inline int16_t GetMslLock (void) { return m_info.nHomingTarget; }
		inline fix GetCreationTime (void) { return m_info.xCreationTime; }
		inline fix GetScale (void) { return m_info.xScale; }
		inline void SetParentType (int16_t nType) { m_info.parent.nType = nType; }
		inline void SetParentObj (int16_t nObject) { m_info.parent.nObject = nObject; }
		inline void SetParentSig (int32_t nSignature) { m_info.parent.nSignature = nSignature; }
		inline void SetLastHitObj (int16_t nLastHitObj) { m_info.nLastHitObj = nLastHitObj; }
		inline void SetMslLock (int16_t nHomingTarget) { m_info.nHomingTarget = nHomingTarget; }
		inline void SetCreationTime (fix xCreationTime) { m_info.xCreationTime = xCreationTime; }
		inline void SetScale (fix xScale) { m_info.xScale = xScale; }
};

//	-----------------------------------------------------------------------------

typedef struct tAttachedObjInfo {
	int16_t	nParent;	// explosion is attached to this CObject
	int16_t	nPrev;	// previous explosion in attach list
	int16_t	nNext;	// next explosion in attach list
} __pack__ tAttachedObjInfo;

class CAttachedInfo {
	private:
		tAttachedObjInfo	m_info;
	public:
		inline tAttachedObjInfo* GetInfo (void) { return &m_info; };
		inline int16_t GetParent (void) { return m_info.nParent; }
		inline int16_t GetPrevAttached (void) { return m_info.nPrev; }
		inline int16_t GetNextAttached (void) { return m_info.nNext; }
		inline void SetParent (int16_t nParent) { m_info.nParent = nParent; }
		inline void SetPrevAttached (int16_t nPrev) { m_info.nPrev = nPrev; }
		inline void SetNextAttached (int16_t nNext) { m_info.nNext = nNext; }
};

//	-----------------------------------------------------------------------------

typedef struct tExplosionInfo {
    fix     nSpawnTime;       // when lifeleft is < this, spawn another
    fix     nDeleteTime;      // when to delete CObject
    int16_t   nDeleteObj;			// and what CObject to delete
	 tAttachedObjInfo	attached;
} __pack__ tExplosionInfo;

class CExplosionInfo : public CAttachedInfo {
	private:
		tExplosionInfo	m_info;
	public:
		inline tExplosionInfo* GetInfo (void) { return &m_info; };
		inline fix GetSpawnTime (void) { return m_info.nSpawnTime; }
		inline fix GetDeleteTime (void) { return m_info.nDeleteTime; }
		inline int16_t GetDeleteObj (void) { return m_info.nDeleteObj; }
		inline void SetSpawnTime (fix nSpawnTime) { m_info.nSpawnTime = nSpawnTime; }
		inline void SetDeleteTime (fix nDeleteTime) { m_info.nDeleteTime = nDeleteTime; }
		inline void SetDeleteObj (int16_t nDeleteObj) { m_info.nDeleteObj = nDeleteObj; }
};

//	-----------------------------------------------------------------------------

typedef struct tObjLightInfo {
    fix				intensity;  // how bright the light is
	 int16_t			nSegment;
	 int16_t			nObjects;
	 CFloatVector	color;
} __pack__ tObjLightInfo;

class CObjLightInfo {
	private:
		tObjLightInfo	m_info;
	public:
		inline tObjLightInfo* GetInfo (void) { return &m_info; };
		inline fix GetIntensity (void) { return m_info.intensity; }
		inline int16_t GetSegment (void) { return m_info.nSegment; }
		inline int16_t GetObjects (void) { return m_info.nObjects; }
		inline CFloatVector* GetColor (void) { return &m_info.color; }
		inline void SetIntensity (fix intensity) { m_info.intensity = intensity; }
		inline void SetSegment (int16_t nSegment) { m_info.nSegment = nSegment; }
		inline void SetObjects (int16_t nObjects) { m_info.nObjects = nObjects; }
		inline void SetColor (CFloatVector *color) { m_info.color = *color; }
};

//	-----------------------------------------------------------------------------

typedef struct tPowerupInfo {
	int32_t     nCount;          // how many/much we pick up (vulcan cannon only?)
	fix     xCreationTime;  // Absolute time of creation.
	int32_t     nFlags;          // spat by player?
} __pack__ tPowerupInfo;

class CPowerupInfo {
	private:
		tPowerupInfo	m_info;
	public:
		inline tPowerupInfo* GetInfo (void) { return &m_info; };
		inline int32_t GetCount (void) { return m_info.nCount; }
		inline fix GetCreationTime (void) { return m_info.xCreationTime; }
		inline int32_t GetFlags (void) { return m_info.nFlags; }
		inline void SetCount (int32_t nCount) { m_info.nCount = nCount; }
		inline void SetCreationTime (fix xCreationTime) { m_info.xCreationTime = xCreationTime; }
		inline void SetFlags (int32_t nFlags) { m_info.nFlags = nFlags; }
};

//	-----------------------------------------------------------------------------

typedef struct tVideoClipInfo {
public:
	int32_t     nClipIndex;
	fix	  xTotalTime;
	fix     xFrameTime;
	int8_t   nCurFrame;
} __pack__ tVideoClipInfo;

class CVClipInfo {
	private:
		tVideoClipInfo	m_info;
	public:
		inline tVideoClipInfo* GetInfo (void) { return &m_info; };
		inline int32_t GetClipIndex (void) { return m_info.nClipIndex; }
		inline fix GetTotalTime (void) { return m_info.xTotalTime; }
		inline fix GetFrameTime (void) { return m_info.xFrameTime; }
		inline int8_t GetCurFrame (void) { return m_info.nCurFrame; }
};

//	-----------------------------------------------------------------------------

#define SMOKE_TYPE_SMOKE		0
#define SMOKE_TYPE_SPRAY		1
#define SMOKE_TYPE_BUBBLES		2
#define SMOKE_TYPE_FIRE			3
#define SMOKE_TYPE_WATERFALL	4
#define SMOKE_TYPE_RAIN			5
#define SMOKE_TYPE_SNOW			6

typedef struct tParticleInfo {
public:
	int32_t			nLife;
	int32_t			nSize [2];
	int32_t			nParts;
	int32_t			nSpeed;
	int32_t			nDrift;
	int32_t			nBrightness;
	CRGBAColor	color;
	char			nSide;
	char			nType;
	char			bEnabled;
} __pack__ tParticleInfo;

class CSmokeInfo {
	private:
		tParticleInfo	m_info;
	public:
		inline tParticleInfo* GetInfo (void) { return &m_info; };
		inline int32_t GetLife (void) { return m_info.nLife; }
		inline int32_t GetSize (int32_t i) { return m_info.nSize [i]; }
		inline int32_t GetParts (void) { return m_info.nParts; }
		inline int32_t GetSpeed (void) { return m_info.nSpeed; }
		inline int32_t GetDrift (void) { return m_info.nDrift; }
		inline int32_t GetBrightness (void) { return m_info.nBrightness; }
		inline CRGBAColor GetColor (void) { return m_info.color; }
		inline char GetSide (void) { return m_info.nSide; }
};

//	-----------------------------------------------------------------------------

typedef struct tLightningInfo {
public:
	int32_t			nLife;
	int32_t			nDelay;
	int32_t			nLength;
	int32_t			nAmplitude;
	int32_t			nOffset;
	int32_t			nWayPoint;
	int16_t			nBolts;
	int16_t			nId;
	int16_t			nTarget;
	int16_t			nNodes;
	int16_t			nChildren;
	int16_t			nFrames;
	char			nWidth;
	char			nAngle;
	char			nStyle;
	char			nSmoothe;
	char			bClamp;
	char			bGlow;
	char			bSound;
	char			bRandom;
	char			bInPlane;
	char			bEnabled;
	char			bDirection;
	CRGBAColor	color;
} __pack__ tLightningInfo;

class CLightningInfo {
	private:
		tLightningInfo	m_info;
	public:
		inline tLightningInfo* GetInfo (void) { return &m_info; };
		inline int32_t GetLife (void) { return m_info.nLife; }
		inline int32_t GetDelay (void) { return m_info.nDelay; }
		inline int32_t GetLength (void) { return m_info.nLength; }
		inline int32_t GetAmplitude (void) { return m_info.nAmplitude; }
		inline int32_t GetOffset (void) { return m_info.nOffset; }
		inline int16_t GetEmitters (void) { return m_info.nBolts; }
		inline int16_t GetId (void) { return m_info.nId; }
		inline int16_t GetTarget (void) { return m_info.nTarget; }
		inline int16_t GetNodes (void) { return m_info.nNodes; }
		inline int16_t GetChildren (void) { return m_info.nChildren; }
		inline int16_t GetFrames (void) { return m_info.nFrames; }
		inline char GetAngle (void) { return m_info.nAngle; }
		inline char GetStyle (void) { return m_info.nStyle; }
		inline char GetSmoothe (void) { return m_info.nSmoothe; }
		inline char GetClamp (void) { return m_info.bClamp; }
		inline char GetGlow (void) { return m_info.bGlow; }
		inline char GetSound (void) { return m_info.bSound; }
		inline char GetRandom (void) { return m_info.bRandom; }
		inline char GetInPlane (void) { return m_info.bInPlane; }
};


typedef struct tSoundInfo {
public:
	char	szFilename [40];
	fix	nVolume;
	char	bEnabled;
	//Mix_Chunk*	mixChunkP;
} tSoundInfo;

class CSoundInfo {
	private:
		tSoundInfo	m_info;
	public:
		inline tSoundInfo* GetInfo (void) { return &m_info; };
		inline char* Filename (void) { return m_info.szFilename; }
		inline fix Volume (void) { return (fix) FRound (float (m_info.nVolume) * I2X (1) / 10.0f); }
		//inline Mix_Chunk* SoundHandle (void) { return m_info.mixChunkP; }
		//inline void SetSoundHandle (Mix_Chunk* handle) { m_info.mixChunkP = handle; }
};

typedef struct tWayPointInfo {
public:
	int32_t	nId [2];
	int32_t	nSuccessor [2];
	int32_t	nSpeed;
} tWayPointInfo;

class CWayPointInfo {
	private:
		tWayPointInfo m_info;
	public:
		inline tWayPointInfo* GetInfo (void) { return &m_info; }
		inline int32_t Successor (void) { return m_info.nSuccessor [0]; }
		inline int32_t Predecessor (void) { return m_info.nSuccessor [1]; }
		inline void SetSuccessor (int32_t nSuccessor) { m_info.nSuccessor [0] = nSuccessor; }
		inline void SetPredecessor (int32_t nPredecessor) { m_info.nSuccessor [1] = nPredecessor; }
	};

//	-----------------------------------------------------------------------------
// structures for different kinds of rendering

typedef struct tPolyObjInfo {
public:
	int32_t     			nModel;          // which polygon model
	CAngleVector 	animAngles [MAX_SUBMODELS]; // angles for each subobject
	int32_t     			nSubObjFlags;       // specify which subobjs to draw
	int32_t     			nTexOverride;      // if this is not -1, map all face to this
	int32_t     			nAltTextures;       // if not -1, use these textures instead
} __pack__ tPolyObjInfo;

class CPolyObjInfo {
	private:
		tPolyObjInfo	m_info;
	public:
		inline tPolyObjInfo* GetInfo (void) { return &m_info; };
		inline int32_t GetModel (void) { return m_info.nModel; }
		inline CAngleVector* GetAnimAngles (int32_t i) { return m_info.animAngles + i; }
		inline int32_t GetSubObjFlags (void) { return m_info.nSubObjFlags; }
		inline int32_t GetTexOverride (void) { return m_info.nTexOverride; }
		inline int32_t GetAltTextures (void) { return m_info.nAltTextures; }
		inline void SetModel (int32_t nModel) { m_info.nModel = nModel; }
		inline void SetAnimAngles (const CAngleVector *vAngles, int32_t i) { m_info.animAngles [i] = *vAngles; }
		inline void SetSubObjFlags (int32_t nSubObjFlags) { m_info.nSubObjFlags = nSubObjFlags; }
		inline void SetTexOverride (int32_t nTexOverride) { m_info.nTexOverride = nTexOverride; }
		inline void SetAltTextures (int32_t nAltTextures) { m_info.nAltTextures = nAltTextures; }
};

//	-----------------------------------------------------------------------------

typedef struct tObjTransformation {
	CFixVector	vPos;				// absolute x,y,z coordinate of center of object
	CFixMatrix	mOrient;			// orientation of object in world
	} __pack__ tObjTransformation;

class CObjTransformation {
	private:
		tObjTransformation	m_t;

	public:
		inline CFixVector* GetPos (void) { return &m_t.vPos; }
		inline CFixMatrix* GetOrient (void) { return &m_t.mOrient; }
		inline void SetPos (const CFixVector* vPos) { m_t.vPos = *vPos; }
		inline void SetOrient (const CFixMatrix* mOrient) { m_t.mOrient = *mOrient ; }
	};

//	-----------------------------------------------------------------------------

typedef struct tObjContainerInfo {
	int8_t			nType;
	int8_t			nId;
	int8_t			nCount;
} __pack__ tObjContainerInfo;

class CObjContainerInfo {
	private:
		tObjContainerInfo	m_info;
	public:
		inline tObjContainerInfo* GetInfo (void) { return &m_info; };
		inline int8_t GetContainsType (void) { return m_info.nType; }
		inline int8_t GetContainsId (void) { return m_info.nId; }
		inline int8_t GetContainsCount (void) { return m_info.nCount; }
		inline void SetContainsType (int8_t nType) { m_info.nType = nType; }
		inline void SetContainsId (int8_t nId) { m_info.nId = nId; }
		inline void SetContainsCount (int8_t nCount) { m_info.nCount = nCount; }
};

//	-----------------------------------------------------------------------------

typedef struct tObjectInfo {
	int32_t     			nSignature;    // Every CObject ever has a unique nSignature...
	uint8_t   				nType;         // what nType of CObject this is... robot, weapon, hostage, powerup, fireball
	uint8_t   				nId;           // which form of CObject...which powerup, robot, etc.
#ifdef WORDS_NEED_ALIGNMENT
	int16_t   				pad;
#endif
	int16_t   				nNextInSeg,
								nPrevInSeg;		// id of next and previous connected CObject in Objects, -1 = no connection
	uint8_t   				controlType;   // how this CObject is controlled
	uint8_t   				movementType;  // how this CObject moves
	uint8_t   				renderType;    // how this CObject renders
	uint8_t   				nFlags;        // misc flags
	int16_t					nSegment;
	int16_t   				nAttachedObj;  // number of attached fireball CObject
	tObjTransformation	position;
	fix     					xSize;         // 3d size of CObject - for collision detection
	fix     					xShield;      // Starts at maximum, when <0, CObject dies..
	CFixVector 				vLastPos;		// where CObject was last frame
	tObjContainerInfo		contains;
	int8_t   				nCreator;		// Materialization center that created this CObject, high bit set if producer-created
	fix     					xLifeLeft;     // how long until goes away, or 7fff if immortal
} __pack__ tObjectInfo;

typedef union tObjMovementInfo {
	tPhysicsInfo			physInfo;		// a physics CObject
	CFixVector   			spinRate;		// for spinning objects
	} __pack__ tObjMovementInfo;

typedef union tObjControlInfo {
	tLaserInfo				laserInfo;
	tExplosionInfo			explInfo;      // NOTE: debris uses this also
	tAIStaticInfo			aiInfo;
	tObjLightInfo			lightInfo;     // why put this here?  Didn't know what else to do with it.
	tPowerupInfo			powerupInfo;
	tWayPointInfo			wayPointInfo;
	} __pack__ tObjControlInfo;

typedef union tObjRenderInfo {
	tPolyObjInfo			polyObjInfo;   // polygon model
	tVideoClipInfo			vClipInfo;     // tVideoClip
	tParticleInfo			particleInfo;
	tLightningInfo			lightningInfo;
	tSoundInfo				soundInfo;
	} __pack__ tObjRenderInfo;

// TODO get rid of the structs (former unions) and the union
typedef struct tBaseObject {
	tObjectInfo				info;
	tObjMovementInfo		mType;	// movement info, determined by MOVEMENT_TYPE
	tObjControlInfo		cType;	// control info, determined by CONTROL_TYPE
	tObjRenderInfo			rType;	// render info, determined by RENDER_TYPE
#ifdef WORDS_NEED_ALIGNMENT
	int16_t   				nPad;
#endif
} __pack__ tBaseObject;

//	-----------------------------------------------------------------------------

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
		inline int32_t Signature () { return info.nSignature; }
		inline uint8_t Id () { return info.nId; }
		inline fix Size () { return info.xSize; }
		inline fix Shield () { return info.xShield; }
		inline fix LifeLeft () { return info.xLifeLeft; }
		inline int16_t Segment () { return info.nSegment; }
		inline int16_t AttachedObj () { return info.nAttachedObj; }
		inline int16_t NextInSeg () { return info.nNextInSeg; }
		inline int16_t PrevInSeg () { return info.nPrevInSeg; }
		inline int8_t Creator () { return info.nCreator; }
		inline uint8_t Type () { return info.nType; }
		inline uint8_t ControlType () { return info.controlType; }
		inline uint8_t MovementType () { return info.movementType; }
		inline uint8_t RenderType () { return info.renderType; }
		inline uint8_t& Flags () { return info.nFlags; }
		inline CFixVector LastPos () { return info.vLastPos; }

		inline void SetSignature (int32_t nSignature) { info.nSignature = nSignature; }
		inline void SetKey (uint8_t nId) { info.nId = nId; }
		inline void SetShield (fix xShield) { info.xShield = xShield; }
		inline void UpdateShield (fix xShield) { info.xShield += xShield; }
		inline void SetLifeLeft (fix xLifeLeft) { info.xLifeLeft = xLifeLeft; }
		inline void SetSegment (int16_t nSegment) { info.nSegment = nSegment; }
		inline void SetAttachedObj (int16_t nAttachedObj) { info.nAttachedObj = nAttachedObj; }
		inline void SetNextInSeg (int16_t nNextInSeg) { info.nNextInSeg = nNextInSeg; }
		inline void SetPrevInSeg ( int16_t nPrevInSeg) { info.nPrevInSeg = nPrevInSeg; }
		inline void SetCreator (int8_t nCreator) { info.nCreator = nCreator; }
		inline void SetType (uint8_t nType) { info.nType = nType; }
		inline void SetId (uint8_t nId) { info.nId = nId; }
		inline void SetControlType (uint8_t controlType) { info.controlType = controlType; }
		inline void SetMovementType (uint8_t movementType) { info.movementType = movementType; }
		inline void SetRenderType (uint8_t renderType) { info.renderType = renderType; }
		inline void SetFlags (uint8_t nFlags) { info.nFlags = nFlags; }
		inline void SetLastPos (const CFixVector& vLastPos) { info.vLastPos = vLastPos; }
	};

//	-----------------------------------------------------------------------------

struct tObject;

#define OBJ_LIST_TYPE 0

typedef struct tObjListLink {
	tObject*			prev;
	tObject*			next;
} __pack__ tObjListLink;

typedef struct tShotInfo {
	int16_t			nObject;
	int32_t			nSignature;
} __pack__ tShotInfo;

typedef struct tObject : public tBaseObject {
#if OBJ_LIST_TYPE
	tObjListLink	links [3];		// link into list of objects in same category (0: all, 1: same type, 2: same class)
#endif
	uint8_t			nLinkedType;
	uint8_t			nTracers;
	fix				xCreationTime;
	fix				xTimeLastHit;
	tShotInfo		shots;
	CFixVector		vStartVel;
	CFixVector		vRenderPos;
} __pack__ tObject;

class CObject;


class CObjListLink {
	public:
		CObject	*prev, *next;
};


typedef struct tObjListRef {
	CObject*		head;
	CObject*		tail;
	int16_t		nObjects;
} __pack__ tObjListRef;


class CObjHitInfo {
	public:
		CFixVector		v [3];
		time_t			t [3];
		int32_t			i;
};

class CObjDamageInfo {
	public:
		fix				nHits [3];	// aim, drives, guns
		bool				bCritical;
		int32_t			nCritical;
		int32_t			tCritical;	// time of last critical hit
		int32_t			tShield;		// time of last non-critical hit
		int32_t			tRepaired;
};

#define MAX_WEAPONS	100

#include "collision_math.h"

#if DBG

// track object position over up to 120 frames to find out why robots are occasionally jumping!

class CPositionSnapshot {
	public:
		uint8_t		bIdleAnimation;
		fix			xTime;
		CFixVector	vPos;
};

#define POSTRACK_MAXFRAMES 120

class CPositionTracker {
	public:
		int32_t		m_nCurPos;
		int32_t		m_nPosCount;
		CPositionSnapshot m_positions [POSTRACK_MAXFRAMES];

		CPositionTracker () : m_nCurPos (0), m_nPosCount (0) {}
		void Update (CFixVector& vPos, uint8_t bIdleAnimation = 0);
		int32_t Check (int32_t nId);
};

#endif

class CObject : public CObjectInfo {
	private:
		static CArray<uint16_t>	m_weaponInfo;
		static CArray<uint8_t>	m_bIsEquipment; 

	public:
		static void InitTables (void);
		static inline bool IsWeapon (int16_t nId) { return (m_weaponInfo [nId] & OBJ_IS_WEAPON) != 0; }
		static inline bool IsEnergyWeapon (int16_t nId) { return (m_weaponInfo [nId] & OBJ_IS_ENERGY_WEAPON) != 0; }
		static inline bool HasLightTrail (int16_t nId) { return (m_weaponInfo [nId] & OBJ_HAS_LIGHT_TRAIL) != 0; }
		static inline bool IsMissile (int16_t nId) { return (m_weaponInfo [nId] & OBJ_IS_MISSILE) != 0; }
		static inline uint8_t IsEquipment (int16_t nId) { return m_bIsEquipment [nId]; }
		static bool IsPlayerMine (int16_t nId);
		static bool IsRobotMine (int16_t nId);
		static bool IsMine (int16_t nId);

		bool IsPlayerMine (void);
		bool IsRobotMine (void);
		bool IsMine (void);
		bool IsGatlingRound (void);
		bool IsSplashDamageWeapon (void);
		bool Bounces (void);
		bool AttacksRobots (void);
		bool AttacksPlayer (void);
		bool AttacksObject (CObject* targetP);
		inline void SetAttackMode (int32_t nMode) { m_nAttackRobots = nMode; }
		inline void Arm (void) { m_nAttackRobots = ROBOT_IS_HOSTILE; }
		inline void Disarm (void) { m_nAttackRobots = ROBOT_IS_NEUTRAL; }
		inline bool Disarmed (void) { return (m_nAttackRobots >= ROBOT_IS_NEUTRAL); }
		inline void Reprogram (void) { m_nAttackRobots = ROBOT_IS_FRIENDLY; }
		inline bool Reprogrammed (void) { return (m_nAttackRobots >= ROBOT_IS_FRIENDLY); }

	private:
		int16_t			m_nKey;
		CObject*			m_prev;
		CObject*			m_next;
		CObject*			m_target;
#if OBJ_LIST_TYPE
		CObjListLink	m_links [3]; // link into list of objects in same category (0: all, 1: same type, 2: same class)
		uint8_t			m_nLinkedType;
#endif
		uint8_t			m_nTracers;
		fix				m_xCreationTime;
		fix				m_xMoveTime; // move object explosions out of their origin towards the viewer during the explosion's life time
		fix				m_xMoveDist;
		fix				m_xTimeLastHit;
		fix				m_xTimeLastEffect;
		fix				m_xTimeEnergyDrain;
		int32_t			m_nAttackRobots;
		tShotInfo		m_shots;
		CFixVector		m_vStartVel;
		CFixVector		m_vOrigin;
		CFixVector		m_vRenderPos;
		CObjHitInfo		m_hitInfo;
		CObjDamageInfo	m_damage;
		bool				m_bMultiplayer;
		bool				m_bRotate;
		bool				m_bSynchronize;
		int32_t			m_nFrame;
		int32_t			m_bIgnore [2]; // ignore this object (physics: type = 0, pickup powerup: type = 1)
#if DBG
		CPositionTracker	m_posTracker;
#endif

	public:
		CObject ();
		~CObject ();

#if DBG
		inline int32_t CheckSpeed (uint8_t bIdleAnimation = 0) {
			m_posTracker.Update (Position (), bIdleAnimation);
			return m_posTracker.Check (Id ());
			}
#endif
		// initialize a new CObject.  adds to the list for the given CSegment
		// returns the CObject number
		int32_t Create (uint8_t nType, uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos,
						const CFixMatrix& mOrient, fix xSize, uint8_t cType, uint8_t mType, uint8_t rType);

		inline bool Exists (void) { return !(Flags () & (OF_EXPLODING | OF_SHOULD_BE_DEAD | OF_DESTROYED)); }
		inline bool Multiplayer (void) { return m_bMultiplayer; }
		// unlinks an CObject from a CSegment's list of objects
		void Init (void);
		void Link (void);
		void Unlink (bool bForce = false);
		void Link (tObjListRef& ref, int32_t nLink);
		void Unlink (tObjListRef& ref, int32_t nLink);
#if DBG
		bool IsInList (tObjListRef& ref, int32_t nLink);
#endif
#if OBJ_LIST_TYPE == 1
		inline void ResetLinks (void) {
			memset (m_links, 0, sizeof (m_links));
			m_nLinkedType = OBJ_NONE;
			}
		inline CObjListLink& Links (uint32_t i) { return m_links [i]; }
		inline uint8_t LinkedType (void) { return m_nLinkedType; }
		inline void SetLinkedType (uint8_t nLinkedType) { m_nLinkedType = nLinkedType; }
		inline void InitLinks (void) { memset (m_links, 0, sizeof (m_links)); }
#endif
		void SetType (uint8_t nNewType, bool bLink = true);
		void ResetSgmLinks (void) { info.nNextInSeg = info.nPrevInSeg = info.nSegment = -1; }
		void LinkToSeg (int32_t nSegment);
		void UnlinkFromSeg (void);
		void RelinkToSeg (int32_t nNewSeg);
		bool IsLinkedToSeg (int16_t nSegment);
		void Initialize (uint8_t nType, uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos,
							  const CFixMatrix& mOrient, fix xSize, uint8_t cType, uint8_t mType, uint8_t rType);
		void ToBaseObject (tBaseObject *objP);

		inline int16_t Key (void) { return m_nKey; }
		inline CObject* Prev (void) { return m_prev; }
		inline CObject* Next (void) { return m_next; }
		inline uint8_t Tracers (void) { return m_nTracers; }
		inline fix CreationTime (void) { return m_xCreationTime; }
		inline fix TimeLastHit (void) { return m_xTimeLastHit; }
		inline fix TimeLastEffect (void) { return m_xTimeLastEffect; }
		inline tShotInfo& Shots (void) { return m_shots; }
		inline CFixVector StartVel (void) { return m_vStartVel; }
		inline CFixVector RenderPos (void) { return m_vRenderPos.IsZero () ? info.position.vPos : m_vRenderPos; }

		inline void SetKey (int16_t nKey) { m_nKey = nKey; }
		inline void SetPrev (CObject* prev) { m_prev = prev; }
		inline void SetNext (CObject* next) { m_next = next; }
		inline void SetTracers (uint8_t nTracers) { m_nTracers = nTracers; }
		void SetCreationTime (fix xCreationTime = -1);
		fix LifeTime (void);
		inline void SetTimeLastHit (fix xTimeLastHit) { m_xTimeLastHit = xTimeLastHit; }
		inline void SetTimeLastEffect (fix xTimeLastEffect) { m_xTimeLastEffect = xTimeLastEffect; }
		inline void SetStartVel (CFixVector* vStartVel) { m_vStartVel = *vStartVel; }
		inline void SetRenderPos (CFixVector& vRenderPos) { m_vRenderPos = vRenderPos; }
		inline CFixVector Origin (void) { return m_vOrigin; }
		inline void SetOrigin (CFixVector vOrigin) { m_vOrigin = vOrigin; }
		inline void SetFrame (int32_t nFrame) { m_nFrame = nFrame; }
		inline int32_t Frame (void) { return m_nFrame; }
		inline void SetMoveDist (fix moveDist) { m_xMoveDist = moveDist; }
		inline void SetMoveTime (fix moveTime) { m_xMoveTime = moveTime; }

		inline fix Mass (void) { return mType.physInfo.mass; }
		inline fix Drag (void) { return mType.physInfo.drag; }
		inline CFixVector& Thrust (void) { return mType.physInfo.thrust; }
		inline CFixVector& RotThrust (void) { return mType.physInfo.rotThrust; }
		inline CFixVector& Velocity (void) { return mType.physInfo.velocity; }
		inline CFixVector& RotVelocity (void) { return mType.physInfo.rotVel; }
		inline CFixVector& Position (void) { return info.position.vPos; }
		CFixVector FrontPosition (void);
		inline CFixMatrix& Orientation (void) { return info.position.mOrient; }
		inline int16_t Segment (void) { return info.nSegment; }
#if DBG
		void SetLife (fix xLife);
#else
		inline void SetLife (fix xLife) { info.xLifeLeft = xLife; }
#endif
		inline fix LifeLeft (void) { return info.xLifeLeft; }

		inline int32_t& WayPointId (void) { return cType.wayPointInfo.nId [1]; }
		inline int32_t& NextWayPoint (void) { return cType.wayPointInfo.nSuccessor [0]; }
		inline int32_t& PrevWayPoint (void) { return cType.wayPointInfo.nSuccessor [1]; }
		inline int32_t* WayPoint (void) { 
			if (info.renderType == RT_LIGHTNING)
				return &rType.lightningInfo.nWayPoint; 
			return NULL;
			}
		inline int32_t NextWayPoint (CObject* objP) { 
			return ((info.controlType == CT_WAYPOINT) && (objP->info.renderType == RT_LIGHTNING))
					 ? cType.wayPointInfo.nSuccessor [(int32_t) objP->rType.lightningInfo.bDirection]
					 : -1;
			}

		void Read (CFile& cf);
		void LoadState (CFile& cf);
		void SaveState (CFile& cf);
		void LoadTextures (void);
		int32_t PowerupToDevice (void);
		void HandleSegmentFunction (void);
		void SetupSmoke (void);

		int32_t OpenableDoorsInSegment (void);
		int32_t CheckSegmentPhysics (void);
		int32_t CheckWallPhysics (void);
		int32_t ApplyWallPhysics (int16_t nSegment, int16_t nSide);
		void ScrapeOnWall (int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt);
		void CreateSound (int16_t nSound);

		void Die (void);
		void MultiDie (void);
		void MaybeDelete (void);

		void TurnTowardsVector (CFixVector vGoal, fix rate);
		void Wiggle (void);
		void ApplyFlightControls (void);
		void ApplyForce (CFixVector vForce);
		void ApplyRotForce (CFixVector vForce);
		void SetThrustFromVelocity (void);
		void Bump (CFixVector vForce, fix xDamage);
		void RandomBump (fix xScale, fix xForce, bool bSound = false);
		void Bump (CObject *otherObjP, CFixVector vForce, int32_t bDamage);
		void Bump (CObject *otherObjP, CFixVector vForce, CFixVector vRotForce, int32_t bDamage);
		void ApplyForceDamage (fix vForce, CObject *otherObjP);
		int32_t ApplyDamageToRobot (fix damage, int32_t nKillerObj);
		void ApplyDamageToPlayer (CObject *killerObjP, fix damage);
		void ApplyDamageToReactor (fix xDamage, int16_t nAttacker);
		int32_t ApplyDamageToClutter (fix xDamage);
		void Explode (fix delayTime);
		void ExplodePolyModel (void);
		CObject* CreateDebris (int32_t nSubObj);

		float CollisionPoint (CFloatVector* vDir, CFloatVector* vHit = NULL);

		void CollidePlayerAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt);
		void CollideRobotAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt);
		int32_t CollideWeaponAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall, CFixVector& vHitPt);
		int32_t CollideDebrisAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall, CFixVector& vHitPt);
		int32_t CollideObjectAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall, CFixVector& vHitPt);

		int32_t CollideRobotAndPlayer (CObject* playerObjP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideRobotAndReactor (CObject* reactorP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideRobotAndRobot (CObject* other, CFixVector& vHitPt, CFixVector* vNormal = NULL);

		int32_t CollidePlayerAndReactor (CObject* reactorP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollidePlayerAndPowerup (CObject* powerupP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollidePlayerAndMonsterball (CObject* monsterball, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollidePlayerAndHostage (CObject* hostageP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollidePlayerAndMarker (CObject* markerP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollidePlayerAndPlayer (CObject* other, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollidePlayerAndNastyRobot (CObject* robotP, CFixVector& vHitPt, CFixVector* vNormal = NULL);

		int32_t CollideWeaponAndRobot (CObject* robotP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideWeaponAndReactor (CObject* reactorP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideWeaponAndClutter (CObject *clutterP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideWeaponAndDebris (CObject *debrisP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideWeaponAndPlayer (CObject *playerObjP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideWeaponAndMonsterball (CObject *mBallP, CFixVector& vHitPt, CFixVector* vNormal = NULL);
		int32_t CollideWeaponAndWeapon (CObject *other, CFixVector& vHitPt, CFixVector* vNormal = NULL);

		int32_t CollideActorAndClutter (CObject* clutter, CFixVector& vHitPt, CFixVector* vNormal = NULL);

		int32_t CollideRobotAndObjProducer (void);
		int32_t CollidePlayerAndObjProducer (void);

		inline void RequestEffects (uint8_t nEffects);
		CObject* CreateExplBlast (void);
		CObject* CreateShockwave (void);
		int32_t CreateWeaponEffects (int32_t bExplBlast);
		CObject* ExplodeSplashDamage (fix damage, fix distance, fix force);
		CObject* ExplodeSplashDamagePlayer (void);
		CObject* ExplodeSplashDamageWeapon (CFixVector& vPos);
		void MaybeKillWeapon (CObject *otherObjP);
		int32_t MaybeDetonateWeapon (CObject* otherP, CFixVector& vHitPt);
		void DoExplosionSequence (void);
		void CreateAppearanceEffect (void);

		int32_t BossSpewRobot (CFixVector* vPos, int16_t objType, int32_t bObjTrigger);
		int32_t CreateGatedRobot (int16_t nSegment, uint8_t nObjId, CFixVector* vPos);

		int32_t FindSegment (void);
		void SetTurnRoll (void);
		int32_t DoPhysicsSimRot (void);
		void DoPhysicsSim (void);
		void FinishPhysicsSim (CPhysSimData& simData);
		void DoPhysicsSimOld (void);
		void Spin (void);
		int32_t Update (void);
		int32_t Index (void);
		float Damage (void);
		int32_t SoundClass (void);

		void MorphStart (void);
		void MorphDraw (void);
		void DoMorphFrame (void);
		void DoPowerupFrame (void);
		void RotateCamera (void);
		void RotateMarker (void);

		int32_t SelectHomingTarget (CFixVector& vTrackerPos);
		int32_t FindVisibleHomingTarget (CFixVector& vTrackerPos);
		int32_t FindAnyHomingTarget (CFixVector& vTrackerPos, int32_t nTargetType1, int32_t nTargetType2, int32_t nThread = 0);
		int32_t UpdateHomingTarget (int32_t nTarget, fix& dot, int32_t nThread = 0);
		int32_t MaxTrackableDist (int32_t& xBestDot);

		CFixVector RegisterHit (CFixVector vHit, int16_t nModel = -1);
		inline bool CriticalHit (void) {
			bool bCritical = m_damage.bCritical;
			m_damage.bCritical = false;	// reset when being queried
			return bCritical;
			}
		inline int32_t CritHitTime (void) { return m_damage.tCritical; }
		inline void SetDamage (fix xAim, fix xDrives, fix xGuns) {
			m_damage.nHits [0] = xAim;
			m_damage.nHits [1] = xDrives;
			m_damage.nHits [2] = xGuns;
			}

		inline bool CriticalDamage (void) { return m_damage.nHits [0] || m_damage.nHits [1] || m_damage.nHits [2]; }
		float DamageRate (void);
		fix SubSystemDamage (int32_t i);
		inline fix AimDamage (void) { return SubSystemDamage (0); }
		inline fix DriveDamage (void) { return SubSystemDamage (1); }
		inline fix GunDamage (void) { return SubSystemDamage (2); }
		inline bool ResetDamage (int32_t i) {
			if (!m_damage.nHits [i])
				return false;
			m_damage.nHits [i] = 0;
			return true;
			}
		bool ResetDamage (void);
		bool RepairDamage (int32_t i);
		void RepairDamage (void);
		int32_t TimeLastRepaired (void) { return m_damage.tRepaired; }

		bool Cloaked (void);

		inline void SetTarget (CObject* targetP) { m_target = targetP; }
		CObject* Target (void);
		CObject* Parent (void);
		void DrainEnergy (void);

		inline CObjHitInfo& HitInfo (void) { return m_hitInfo; }
		inline CFixVector HitPoint (int32_t i) { return m_hitInfo.v [i]; }

		CFixMatrix* View (int32_t i);

		void Bash (uint8_t nId);
		void BashToShield (bool bBash);
		void BashToEnergy (bool bBash);


		inline void Rotate (bool bRotate) { m_bRotate = bRotate; }
		inline bool Rotation (void) { return m_bRotate; }

		int32_t ModelId (bool bRaw = false);

		float SpeedScale (void);
		float ShieldScale (void);
		float EnergyScale (void);
		inline int32_t MaxSpeed (void) { return int32_t (60 * SpeedScale ()); }
		inline fix MaxShield (void) { return fix (I2X (100) * ShieldScale ()); }
		inline fix MaxEnergy (void) { return fix (I2X (100) * EnergyScale ()); }

		int16_t Visible (void);

		bool IsGuideBot (void);
		bool IsThief (void);
		inline bool IsPlayer (void) { return (Type () == OBJ_PLAYER); }
		inline bool IsRobot (void) { return (Type () == OBJ_ROBOT); }
		inline bool IsReactor (void) { return (Type () == OBJ_REACTOR); }
		inline bool IsPowerup (void) { return (Type () == OBJ_POWERUP); }
		inline bool IsWeapon (void) { return (Type () == OBJ_WEAPON) && (Id () < m_weaponInfo.Length ()) && IsWeapon (Id ()); }
		inline bool IsEnergyWeapon (void) { return (Type () == OBJ_WEAPON) && (Id () < m_weaponInfo.Length ()) && IsEnergyWeapon (Id ()); }
		inline bool HasLightTrail (void) { return (Type () == OBJ_WEAPON) && (Id () < m_weaponInfo.Length ()) && HasLightTrail (Id ()); }
		inline bool IsMissile (void) { return (Type () == OBJ_WEAPON) && (Id () < m_weaponInfo.Length ()) && IsMissile (Id ()); }
		inline bool IsEquipment (void) { return (Type () == OBJ_WEAPON) && (Id () < m_weaponInfo.Length ()) && IsEquipment (Id ()); }
		inline bool IsStatic (void) { return cType.aiInfo.behavior == AIB_STATIC; }
		bool Indestructible (void);
		inline bool IsGeometry (void) { return IsStatic () && Indestructible (); }
		inline void Ignore (int32_t bFlag, int32_t nType = 0) { m_bIgnore [nType] = bFlag; }
		inline bool Ignored (int32_t bFlag, int32_t nType = 0) { return m_bIgnore [nType] == bFlag; }

		inline void SetSize (fix xSize) { info.xSize = xSize; }
		inline void AdjustSize (int32_t i = 0, fix scale = 0) { 
			fix size = ModelRadius (i);
			if (!size)
				size = ModelRadius (!i);
			if (size)
				SetSize (scale ? FixDiv (size, scale) : size); 
			}

		void Verify (void);
		void VerifyPosition (void);
		inline void SetSizeFromPowerup (void) { SetSize (PowerupSize ()); }
		fix ModelRadius (int32_t i);
		fix PowerupSize (void);

		inline bool Synchronize (void) { return m_bSynchronize; }
		inline void StartSync (void) { m_bSynchronize = true; }
		inline void StopSync (void) { m_bSynchronize = false; }

	private:
		void CheckGuidedMissileThroughExit (int16_t nPrevSegment);
		void CheckAfterburnerBlobDrop (void);
		int32_t CheckTriggerHits (int16_t nPrevSegment);
		void UpdateShipSound (void);
		void UpdateEffects (void);
		int32_t UpdateMovement (void);
		void UpdatePosition (void);
		bool RemoveWeapon (void);
		void UpdateWeaponSpeed (void);
		void UpdateWeapon (void);
		void SetupRandomMovement (void);
		void SetupDebris (int32_t nSubObj, int32_t nId, int32_t nTexOverride);

		int32_t ObjectIsTrackable (int32_t nTarget, fix& xDot);
		int32_t FindTargetWindow (void);
		void AddHomingTarget (CObject* targetP, CFixVector* vTrackerPos, fix maxTrackableDist, fix& xBestDot, int32_t& nBestObj);

		void ProcessDrag (CPhysSimData& simData);
		int32_t ProcessOffset (CPhysSimData& simData);
		int32_t HandleObjectCollision (CPhysSimData& simData);
		int32_t HandleWallCollision (CPhysSimData& simData);
		int32_t HandleBadCollision (CPhysSimData& simData);
		int32_t ProcessObjectCollision (CPhysSimData& simData);
		int32_t ProcessWallCollision (CPhysSimData& simData);
		int32_t ProcessBadCollision (CPhysSimData& simData);
		int32_t UpdateSimTime (CPhysSimData& simData);
		int32_t UpdateOffset (CPhysSimData& simData);
		void FixPosition (CPhysSimData& simData);
		void ComputeMovedTime (CPhysSimData& simData);
		void UnstickFromWall (CPhysSimData& simData, CFixVector& vOldVel);
		void UnstickFromObject (CPhysSimData& simData, CFixVector& vOldVel);
		void SetupHitQuery (CHitQuery& hitQuery, int32_t nFlags, CFixVector* vNewPos = NULL);
		int32_t Bounce (CHitResult hitResult, float fOffs, fix *pxSideDists);
		void DoBumpHack (void);

	public:
		void UpdateHomingWeapon (int32_t nThread = 0);
		void Unstick (void);
	};

inline int32_t operator- (CObject* o, CArray<CObject>& a) { return a.Index (o); }

#if 0

//	-----------------------------------------------------------------------------

class CRobotObject : public CObject, public CPhysicsInfo, public CAIStaticInfo, public CPolyObjInfo {
	public:
		CRobotObject () {}
		~CRobotObject () {}
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

//	-----------------------------------------------------------------------------

class CPowerupObject : public CObject, public CPhysicsInfo, public CPolyObjInfo {
	public:
		CPowerupObject () {}
		~CPowerupObject () {}
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

//	-----------------------------------------------------------------------------

class CWeaponObject : public CObject, public CPhysicsInfo, public CPolyObjInfo {
	public:
		CWeaponObject () {}
		~CWeaponObject () {}
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

//	-----------------------------------------------------------------------------

class CLightObject : public CObject, public CObjLightInfo {
	public:
		CLightObject () {};
		~CLightObject () {};
		void Initialize (void) {};
		void ToBaseObject (tBaseObject *objP);
};

//	-----------------------------------------------------------------------------

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

//	-----------------------------------------------------------------------------

typedef struct tObjPosition {
	tObjTransformation	position;
	int16_t					nSegment;     // CSegment number containing CObject
	int16_t					nSegType;		// nType of CSegment
} tObjPosition;

class CObjPosition : public CObjTransformation {
	private:
		int16_t				m_nSegment;
		int16_t				m_nSegType;
	public:
		inline int16_t& Segment () { return m_nSegment; }
		inline int16_t& SegType () { return m_nSegType; }
};

//	-----------------------------------------------------------------------------

typedef struct tWindowRenderedData {
	int32_t     nFrame;
	CObject*		viewerP;
	int32_t     bRearView;
	int32_t     nUser;
	int32_t     nObjects;
	int16_t		renderedObjects [MAX_RENDERED_OBJECTS];
} tWindowRenderedData;

class WIndowRenderedData {
	private:
		tWindowRenderedData	m_data;
	public:
		inline int32_t& Frame () { return m_data.nFrame; }
		inline int32_t& RearView () { return m_data.bRearView; }
		inline int32_t& User () { return m_data.nUser; }
		inline int32_t& Objects () { return m_data.nObjects; }
		inline int16_t& RenderedObjects (int32_t i) { return m_data.renderedObjects [i]; }
		inline CObject *Viewer () { return m_data.viewerP; }
};

//	-----------------------------------------------------------------------------

typedef struct tObjDropInfo {
	time_t	nDropTime;
	int16_t		nPowerupType;
	int16_t		nPrevPowerup;
	int16_t		nNextPowerup;
	int16_t		nObject;
	int32_t		nSignature;
} tObjDropInfo;

class CObjDropInfo {
	private:
		tObjDropInfo	m_info;
	public:
		inline time_t& Time () { return m_info.nDropTime; }
		inline int16_t& Type () { return m_info.nPowerupType; }
		inline int16_t& Prev () { return m_info.nPrevPowerup; }
		inline int16_t& Next () { return m_info.nNextPowerup; }
		inline int16_t& Object () { return m_info.nObject; }
};

//	-----------------------------------------------------------------------------

class tObjectRef {
public:
	int16_t		objIndex;
	int16_t		nextObj;
};

//	-----------------------------------------------------------------------------

#define MAX_RENDERED_WINDOWS    3

extern tWindowRenderedData windowRenderedData [MAX_RENDERED_WINDOWS];

/*
 * VARIABLES
 */

// ie gameData.objs.collisionResult[a][b]==  what happens to a when it collides with b

extern char *robot_names[];         // name of each robot

extern CObject Follow;

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

#define OBJ_ITERATOR_TYPE	1 && OBJ_LIST_TYPE

#if OBJ_ITERATOR_TYPE

//	-----------------------------------------------------------------------------

class CObjectIterator {
	public:
		CObject*		m_objP;
		int32_t		m_nLink;

	public:
		CObjectIterator ();
		CObjectIterator (CObject*& objP);

		virtual CObject* Start (void);
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 0; }
		bool Done (void);
		CObject*Step (void);
		CObject* Current (void) { return (m_objP); }
};

//	-----------------------------------------------------------------------------

class CPlayerIterator : public CObjectIterator {
	public:
		CPlayerIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
	};

class CRobotIterator : public CObjectIterator {
	public:
		CRobotIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 1; }
	};

class CWeaponIterator : public CObjectIterator {
	public:
		CWeaponIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 1; }
	};

class CPowerupIterator : public CObjectIterator {
	public:
		CPowerupIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 1; }
	};

class CEffectIterator : public CObjectIterator {
	public:
		CEffectIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 1; }
	};

class CLightIterator : public CObjectIterator {
	public:
		CLightIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 1; }
	};

class CActorIterator : public CObjectIterator {
	public:
		CActorIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 2; }
	};

class CStaticObjectIterator : public CObjectIterator {
	public:
		CStaticObjectIterator (CObject*& objP) : CObjectIterator (objP) {}
		virtual CObject* Head (void);
		virtual int32_t Link (void) { return 2; }
	};

#else // OBJ_LIST_ITERATOR

class CObjectIterator {
	public:
		CObject*		m_objP;
		int32_t		m_i;

	public:
		CObjectIterator ();
		CObjectIterator (CObject*& objP);

		CObject* Start (void);
		bool Done (void);
		CObject*Step (void);
		CObject* Current (void) { return (m_objP); }
		int32_t Index (void) { return m_i; }
		virtual bool Match (void) { return true; }
};

//	-----------------------------------------------------------------------------

class CPlayerIterator : public CObjectIterator {
	public:
		CPlayerIterator (CObject*& objP);
		virtual bool Match (void) { return (m_objP->Type () == OBJ_PLAYER) || (m_objP->Type () == OBJ_GHOST); }
};

class CRobotIterator : public CObjectIterator {
	public:
		CRobotIterator (CObject*& objP);
		virtual bool Match (void) { return (m_objP->Type () == OBJ_ROBOT); }
};

class CWeaponIterator : public CObjectIterator {
	public:
		CWeaponIterator (CObject*& objP);
		virtual bool Match (void) { return (m_objP->Type () == OBJ_WEAPON); }
};

class CPowerupIterator : public CObjectIterator {
	public:
		CPowerupIterator (CObject*& objP);
		virtual bool Match (void) { return (m_objP->Type () == OBJ_POWERUP); }
};

class CEffectIterator : public CObjectIterator {
	public:
		CEffectIterator (CObject*& objP);
		virtual bool Match (void) { return (m_objP->Type () == OBJ_EFFECT); }
};

class CLightIterator : public CObjectIterator {
	public:
		CLightIterator (CObject*& objP);
		virtual bool Match (void) { return (m_objP->Type () == OBJ_LIGHT); }
};

class CActorIterator : public CObjectIterator {
	public:
		CActorIterator (CObject*& objP);
		virtual bool Match (void) { 
			uint8_t nType = m_objP->Type ();
			return (nType == OBJ_PLAYER) || (nType == OBJ_GHOST) || (nType == OBJ_ROBOT) || (nType == OBJ_REACTOR) || (nType == OBJ_WEAPON) || (nType == OBJ_MONSTERBALL); 
		}
};

class CStaticObjectIterator : public CObjectIterator {
	public:
		CStaticObjectIterator (CObject*& objP);
		virtual bool Match (void) { 
			uint8_t nType = m_objP->Type ();
			return (nType == OBJ_POWERUP) || (nType == OBJ_EFFECT) || (nType == OBJ_LIGHT); 
		}
};

#endif // OBJ_LIST_ITERATOR

#define FORALL_OBJS(_objP)								for (CObjectIterator objIter ((_objP)); !objIter.Done (); (_objP) = objIter.Step ())
#define FORALL_PLAYER_OBJS(_objP)					for (CPlayerIterator playerIter ((_objP)); !playerIter.Done (); (_objP) = playerIter.Step ())
#define FORALL_ROBOT_OBJS(_objP)						for (CRobotIterator robotIter ((_objP)); !robotIter.Done (); (_objP) = robotIter.Step ())
#define FORALL_WEAPON_OBJS(_objP)					for (CWeaponIterator weaponIter ((_objP)); !weaponIter.Done (); (_objP) = weaponIter.Step ())
#define FORALL_POWERUP_OBJS(_objP)					for (CPowerupIterator powerIter ((_objP)); !powerIter.Done (); (_objP) = powerIter.Step ())
#define FORALL_EFFECT_OBJS(_objP)					for (CEffectIterator effectIter ((_objP)); !effectIter.Done (); (_objP) = effectIter.Step ())
#define FORALL_LIGHT_OBJS(_objP)						for (CLightIterator lightIter ((_objP)); !lightIter.Done (); (_objP) = lightIter.Step ())
#define FORALL_ACTOR_OBJS(_objP)						for (CActorIterator actorIter ((_objP)); !actorIter.Done (); (_objP) = actorIter.Step ())
#define FORALL_STATIC_OBJS(_objP)					for (CStaticObjectIterator staticsIter ((_objP)); !staticsIter.Done (); (_objP) = staticsIter.Step ())

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

/*
 * FUNCTIONS
 */

// do whatever setup needs to be done
void InitObjects (bool bInitPlayer = true);

int32_t CreateObject (uint8_t nType, uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos, const CFixMatrix& mOrient,
					   fix xSize, uint8_t cType, uint8_t mType, uint8_t rType);
int32_t CloneObject (CObject *objP);
int32_t CreateRobot (uint8_t nId, int16_t nSegment, const CFixVector& vPos);
int32_t CreatePowerup (uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos, int32_t bIgnoreLimits, bool bForce = false);
int32_t CreateWeapon (uint8_t nId, int16_t nCreator, int16_t nSegment, const CFixVector& vPos, fix xSize, uint8_t rType);
int32_t CreateFireball (uint8_t nId, int16_t nSegment, const CFixVector& vPos, fix xSize, uint8_t rType);
int32_t CreateDebris (CObject *parentP, int16_t nSubModel);
int32_t CreateCamera (CObject *parentP);
int32_t CreateLight (uint8_t nId, int16_t nSegment, const CFixVector& vPos);
// returns CSegment number CObject is in.  Searches out from CObject's current
// seg, so this shouldn't be called if the CObject has "jumped" to a new seg
// -- unused --
//int32_t obj_get_new_seg(CObject *obj);

// when an CObject has moved into a new CSegment, this function unlinks it
// from its old CSegment, and links it into the new CSegment
void RelinkObjToSeg (int32_t nObject, int32_t nNewSeg);

void ResetSegObjLists (void);
void LinkAllObjsToSegs (void);
void RelinkAllObjsToSegs (void);
bool CheckSegObjList (CObject *objP, int16_t nObject, int16_t nFirstObj);

// move an CObject from one CSegment to another. unlinks & relinks
// -- unused --
//void obj_set_new_seg(int32_t nObject,int32_t newsegnum);

// links an CObject into a CSegment's list of objects.
// takes CObject number and CSegment number
void LinkObjToSeg(int32_t nObject,int32_t nSegment);

// unlinks an CObject from a CSegment's list of objects
void UnlinkObjFromSeg (CObject *objP);

// initialize a new CObject.  adds to the list for the given CSegment
// returns the CObject number
//int32_t CObject::Create(uint8_t nType, char id, int16_t owner, int16_t nSegment, const CFixVector& pos,
//               const CFixMatrix& orient, fix size, uint8_t ctype, uint8_t mtype, uint8_t rtype, int32_t bIgnoreLimits);

// make a copy of an CObject. returs num of new CObject
int32_t ObjectCreateCopy(int32_t nObject, CFixVector& new_pos, int32_t newsegnum);

// remove CObject from the world
int32_t FreeObjectSlots (int32_t nRequested);
void ReleaseObject(int16_t nObject);

// called after load.  Takes number of objects, and objects should be
// compressed
void ResetObjects (int32_t nObjects);
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
int32_t UpdateAllObjects();     // moves all objects

// set viewer CObject to next CObject in array
void object_goto_nextViewer();

// draw target boxes for nearby robots
void object_render_targets(void);

// move an CObject for the current frame
int32_t UpdateObject(CObject * obj);

// make object0 the player, setting all relevant fields
void InitPlayerObject(void);

// check if CObject is in CObject->nSegment.  if not, check the adjacent
// segs.  if not any of these, returns false, else sets obj->nSegment &
// returns true callers should really use FindHitpoint()
// Note: this function is in gameseg.c
int32_t UpdateObjectSeg (CObject *objP, bool bMove = true);


// go through all objects and make sure they have the correct CSegment
// numbers used when debugging is on
void FixObjectSegs (void);

void DeadPlayerEnd (void);

// Extract information from an CObject (objp->orient, objp->pos,
// objp->nSegment), stuff in a tShortPos structure.  See typedef
// tShortPos.
void CreateLongPos (tLongPos* posP, CObject* objP);
void CreateShortPos(tShortPos *posP, CObject *objp, int32_t bSwapBytes = 0);

// Extract information from a tShortPos, stuff in objp->orient
// (matrix), objp->pos, objp->nSegment
void ExtractShortPos(CObject *objp, tShortPos *spp, int32_t bSwapBytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void ClearTransientObjects(int32_t clear_all);

// returns the number of a free CObject, updating HighestObject_index.
// Generally, CObject::Create() should be called to get an CObject, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
int32_t AllocObject(int32_t nRequestedObject = -1);
int32_t ClaimObjectSlot (int32_t nObject);

// frees up an CObject.  Generally, ReleaseObject() should be called to
// get rid of an CObject.  This function deallocates the CObject entry
// after the CObject has been unlinked
void FreeObject(int32_t nObject);

// after calling initObject(), the network code has grabbed specific
// CObject slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void ClaimObjectSlots(void);

// attaches an CObject, such as a fireball, to another CObject, such as
// a robot

void CreateSmallFireballOnObject (CObject *objp, fix size_scale, int32_t soundFlag);

// returns CObject number
int32_t DropMarkerObject (CFixVector& vPos, int16_t nSegment, CFixMatrix& orient, uint8_t marker_num);

void WakeupRenderedObjects (CObject *viewerP, int32_t nWindow);

void AdjustMineSpawn (void);

void ResetPlayerObject(void);
void StopObjectMovement (CObject *obj);
void StopPlayerMovement (void);

void ObjectGotoNextViewer (void);
void ObjectGotoPrevViewer (void);

int32_t ObjectCount (int32_t nType);

void ResetChildObjects (void);
int32_t AddChildObjectN (int32_t nParent, int32_t nChild);
int32_t AddChildObjectP (CObject *pParent, CObject *pChild);
int32_t DelObjChildrenN (int32_t nParent);
int32_t DelObjChildrenP (CObject *pParent);
int32_t DelObjChildN (int32_t nChild);
int32_t DelObjChildP (CObject *pChild);

void LinkObject (CObject *objP);
void UnlinkObject (CObject *objP);

void PrepareModels (void);

tObjectRef *GetChildObjN (int16_t nParent, tObjectRef *pChildRef);
tObjectRef *GetChildObjP (CObject *pParent, tObjectRef *pChildRef);

CObject *ObjFindFirstOfType (int32_t nType);
void InitWeaponFlags (void);
int32_t CountPlayerObjects (int32_t nPlayer, int32_t nType, int32_t nId);
void FixObjectSizes (void);
void DoSlowMotionFrame (void);

CFixVector* PlayerSpawnPos (int32_t nPlayer);
CFixMatrix* PlayerSpawnOrient (int32_t nPlayer);
void GetPlayerSpawn (int32_t nPlayer, CObject *objP);
void RecreateThief (CObject *objP);
void DeadPlayerFrame (void);

void SetObjectType (CObject *objP, uint8_t nNewType);

void AttachObject (CObject *parentObjP, CObject *childObjP);
void DetachFromParent (CObject *objP);
void DetachChildObjects (CObject *parentP);

void InitMultiPlayerObject (int32_t nStage);

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

#define	OBJ_CLOAKED(_objP)	((_objP)->ctype.aiInfo.flags [6])

#define	SHOW_OBJ_FX				(gameOpts->render.effects.bEnabled && !(gameStates.app.bNostalgia || COMPETITION))

#define	SHOW_SHADOWS			(SHOW_OBJ_FX && EGI_FLAG (bShadows, 0, 1, 0))

#define	IS_BOSS(_objP)			(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).bossFlag)
#define	IS_GUIDEBOT(_objP)	(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).companion)
#define	IS_THIEF(_objP)		(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).thief)
#define	IS_BOSS(_objP)			(((_objP)->info.nType == OBJ_ROBOT) && ROBOTINFO ((_objP)->info.nId).bossFlag)
#define	IS_BOSS_I(_i)			IS_BOSS (gameData.objs.objects + (_i))
#define	IS_MISSILE(_objP)		((_objP)->IsMissile ())
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

//	-----------------------------------------------------------------------------------------------------------

int32_t SetupHiresVClip (tVideoClip *vcP, tVideoClipInfo *vciP, CBitmap* bmP = NULL);
void UpdatePowerupClip (tVideoClip *vcP, tVideoClipInfo *vciP, int32_t nObject);

//	-----------------------------------------------------------------------------------------------------------

#endif
