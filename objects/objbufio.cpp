// Convert object data from/to a buffer compatible with a packed object struct
// Only supports network sync'ed objects: robot, hostage, player, weapon, powerup, reactor, ghost, marker
#include <stdint.h>
#include <string.h>
#include "descent.h"
#include "object.h"
#include "error.h"
#include "objbufio.h"

#define MAX_SUBMODELS 10
#define MAX_AI_FLAGS 11

typedef uint8_t ubyte;
typedef int8_t sbyte;
typedef ubyte pint[4], puint[4], pfix[4], pfloat[4], prgba[4];
typedef ubyte pshort[2], pushort[2], pfixang[2];
typedef struct pvector { pfix x, y, z; } pvector;
typedef struct pangvec { pfixang p, b, h; } pangvec;
typedef struct pmatrix { pvector rvec, uvec, fvec; } pmatrix;

typedef struct tPhysicsInfoPacked {
	pvector  velocity;
	pvector  thrust;
	pfix     mass;
	pfix     drag;
	pfix     brakes;
	pvector  rotVel;
	pvector  rotThrust;
	pfixang  turnRoll;
	pushort  flags;
} tPhysicsInfoPacked;

typedef struct tLaserInfoPacked {
	pshort   parentType;
	pshort   parentNum;
	pint     parentSignature;
	pfix     xCreationTime;
	pshort   nLastHitObj;
	pshort   nHomingTarget;
	pfix     xScale;
} tLaserInfoPacked;

typedef struct tExplosionInfoPacked {
    pfix     nSpawnTime;
    pfix     nDeleteTime;
    pshort   nDestroyedObj;
    pshort   attachParent;
    pshort   attachPrev;
    pshort   attachNext;
} tExplosionInfoPacked;

typedef struct tAIStaticInfoPacked {
	ubyte   behavior;
	sbyte   flags[MAX_AI_FLAGS];
	pshort  nHideSegment;
	pshort  nHideIndex;
	pshort  nPathLength;
	sbyte	nCurPathIndex;
	sbyte	bDyingSoundPlaying;
	pshort	nDangerLaser;
	pint	nDangerLaserSig;
	pfix		xDyingStartTime;
} tAIStaticInfoPacked;

typedef struct tObjLightInfoPacked {
    pfix     intensity;
	pshort	 nSegment; // XL extension
	pshort	 nObjects; // XL extension
	pfloat	 color; // XL extension
} tObjLightInfoPacked;

typedef struct tPowerupInfoPacked {
	pint     nCount;
	pfix     xCreationTime;
	pint     nFlags;
} tPowerupInfoPacked;

typedef struct tAnimationStatePacked {
	pint     nClipIndex;
	pfix     xTotalTime; // XL extension
	pfix     xFrameTime;
	sbyte   nCurFrame;
} tAnimationStatePacked;

typedef struct tAnimationStateCompat {
	pint     nClipIndex;
	pfix     xFrameTime;
	sbyte   nCurFrame;
} tAnimationStateCompat;

typedef struct tParticleInfoPacked {
	pint		nLife;
	pint		nSize [2];
	pint		nParts;
	pint		nSpeed;
	pint		nDrift;
	pint		nBrightness;
	prgba		color;
	sbyte		nSide;
	sbyte		nType;
	sbyte		bEnabled;
} tParticleInfoPacked;

typedef struct tLightningInfoPacked {
	pint		nLife;
	pint		nDelay;
	pint		nLength;
	pint		nAmplitude;
	pint		nOffset;
	pint		nWayPoint;
	pshort		nBolts;
	pshort		nId;
	pshort		nTarget;
	pshort		nNodes;
	pshort		nChildren;
	pshort		nFrames;
	sbyte			nWidth;
	sbyte			nAngle;
	sbyte			nStyle;
	sbyte			nSmoothe;
	sbyte			bClamp;
	sbyte			bGlow;
	sbyte			bBlur;
	sbyte			bSound;
	sbyte			bRandom;
	sbyte			bInPlane;
	sbyte			bEnabled;
	sbyte			bDirection;
	prgba		color;
} tLightningInfoPacked;

typedef struct tSoundInfoPacked {
	char	szFilename [40];
	pfix	nVolume;
	char	bEnabled;
} tSoundInfoPacked;

typedef struct tWayPointInfoPacked {
	pint	nId [2];
	pint	nSuccessor [2];
	pint	nSpeed;
} tWayPointInfoPacked;

typedef struct tPolyobjInfoPacked {
	pint     nModel;
	pangvec  animAngles[MAX_SUBMODELS];
	pint     nSubObjFlags;
	pint     nTexOverride;
	pint     nAltTextures;
} tPolyobjInfoPacked;

typedef struct objectPacked {
	pint    nSignature;
	ubyte   type;
	ubyte   id;
	pshort  next,prev;
	ubyte   controlType;
	ubyte   movementType;
	ubyte   renderType;
	ubyte   flags;
	pshort  nSegment;
	pshort  attached_obj;
	pvector pos;
	pmatrix orient;
	pfix    xSize;
	pfix    xShield;
	pvector vLastPos;
	sbyte   containsType;
	sbyte   containsId;
	sbyte   containsCount;
	sbyte   nCreator;
	pfix    xLifeLeft;

	union {
		tPhysicsInfoPacked   physInfo;
		pvector              spinRate;
	} mType;

	union {
		tLaserInfoPacked     laserInfo;
		tExplosionInfoPacked explInfo;
		tAIStaticInfoPacked  aiInfo;
		tObjLightInfoPacked  lightInfo;
		tPowerupInfoPacked   powerupInfo;
		tWayPointInfoPacked  wayPointInfo;
	} cType;

	union {
		tPolyobjInfoPacked      polyObjInfo;
		tAnimationStatePacked   animationInfo;
		tAnimationStateCompat   animationInfoCompat;
		tParticleInfoPacked		particleInfo;
		tLightningInfoPacked	lightningInfo;
		tSoundInfoPacked		soundInfo;
	} rType;
} tObjectPacked;

#define COPY_B(a, b) a = b
#define COPY_S(a, b) memcpy(&a, &b, sizeof(a))
#define COPY_I(a, b) memcpy(&a, &b, sizeof(a))
#define COPY_F(a, b) memcpy(&a, &b, sizeof(a))

void ObjectWriteToBuffer(CObject *obj, void *buffer)
{
	tObjectPacked *objPacked = (tObjectPacked *)buffer;
	memset(objPacked, 0, sizeof(*objPacked));
	COPY_I(objPacked->nSignature    , obj->info.nSignature);
	COPY_B(objPacked->type          , obj->info.nType);
	COPY_B(objPacked->id            , obj->info.nId);
	COPY_S(objPacked->next          , obj->info.nNextInSeg);
	COPY_S(objPacked->prev          , obj->info.nPrevInSeg);
	COPY_B(objPacked->controlType   , obj->info.controlType);
	COPY_B(objPacked->movementType  , obj->info.movementType);
	COPY_B(objPacked->renderType    , obj->info.renderType);
	COPY_B(objPacked->flags         , obj->info.nFlags);
	COPY_S(objPacked->nSegment      , obj->info.nSegment);
	COPY_S(objPacked->attached_obj  , obj->info.nAttachedObj);
	COPY_I(objPacked->pos.x         , obj->info.position.vPos.v.coord.x);
	COPY_I(objPacked->pos.y         , obj->info.position.vPos.v.coord.y);
	COPY_I(objPacked->pos.z         , obj->info.position.vPos.v.coord.z);
	COPY_I(objPacked->orient.rvec.x , obj->info.position.mOrient.m.dir.r.v.coord.x);
	COPY_I(objPacked->orient.rvec.y , obj->info.position.mOrient.m.dir.r.v.coord.y);
	COPY_I(objPacked->orient.rvec.z , obj->info.position.mOrient.m.dir.r.v.coord.z);
	COPY_I(objPacked->orient.fvec.x , obj->info.position.mOrient.m.dir.f.v.coord.x);
	COPY_I(objPacked->orient.fvec.y , obj->info.position.mOrient.m.dir.f.v.coord.y);
	COPY_I(objPacked->orient.fvec.z , obj->info.position.mOrient.m.dir.f.v.coord.z);
	COPY_I(objPacked->orient.uvec.x , obj->info.position.mOrient.m.dir.u.v.coord.x);
	COPY_I(objPacked->orient.uvec.y , obj->info.position.mOrient.m.dir.u.v.coord.y);
	COPY_I(objPacked->orient.uvec.z , obj->info.position.mOrient.m.dir.u.v.coord.z);
	COPY_I(objPacked->xSize         , obj->info.xSize);
	COPY_I(objPacked->xShield       , obj->info.xShield);
	COPY_I(objPacked->vLastPos.x    , obj->info.vLastPos.v.coord.x);
	COPY_I(objPacked->vLastPos.y    , obj->info.vLastPos.v.coord.y);
	COPY_I(objPacked->vLastPos.z    , obj->info.vLastPos.v.coord.z);
	COPY_B(objPacked->containsType  , obj->info.contains.nType);
	COPY_B(objPacked->containsId    , obj->info.contains.nId);
	COPY_B(objPacked->containsCount , obj->info.contains.nCount);
	COPY_B(objPacked->nCreator      , obj->info.nCreator);
	COPY_I(objPacked->xLifeLeft     , obj->info.xLifeLeft);

	switch (obj->info.movementType)
	{
		case MT_NONE:
			break;

		case MT_PHYSICS:
			COPY_I(objPacked->mType.physInfo.velocity.x  , obj->mType.physInfo.velocity.v.coord.x);
			COPY_I(objPacked->mType.physInfo.velocity.y  , obj->mType.physInfo.velocity.v.coord.y);
			COPY_I(objPacked->mType.physInfo.velocity.z  , obj->mType.physInfo.velocity.v.coord.z);
			COPY_I(objPacked->mType.physInfo.thrust.x    , obj->mType.physInfo.thrust.v.coord.x);
			COPY_I(objPacked->mType.physInfo.thrust.y    , obj->mType.physInfo.thrust.v.coord.y);
			COPY_I(objPacked->mType.physInfo.thrust.z    , obj->mType.physInfo.thrust.v.coord.z);
			COPY_I(objPacked->mType.physInfo.mass        , obj->mType.physInfo.mass);
			COPY_I(objPacked->mType.physInfo.drag        , obj->mType.physInfo.drag);
			COPY_I(objPacked->mType.physInfo.brakes      , obj->mType.physInfo.brakes);
			COPY_I(objPacked->mType.physInfo.rotVel.x    , obj->mType.physInfo.rotVel.v.coord.x);
			COPY_I(objPacked->mType.physInfo.rotVel.y    , obj->mType.physInfo.rotVel.v.coord.y);
			COPY_I(objPacked->mType.physInfo.rotVel.z    , obj->mType.physInfo.rotVel.v.coord.z);
			COPY_I(objPacked->mType.physInfo.rotThrust.x , obj->mType.physInfo.rotThrust.v.coord.x);
			COPY_I(objPacked->mType.physInfo.rotThrust.y , obj->mType.physInfo.rotThrust.v.coord.y);
			COPY_I(objPacked->mType.physInfo.rotThrust.z , obj->mType.physInfo.rotThrust.v.coord.z);
			COPY_S(objPacked->mType.physInfo.turnRoll    , obj->mType.physInfo.turnRoll);
			COPY_S(objPacked->mType.physInfo.flags       , obj->mType.physInfo.flags);
			break;

		case MT_SPINNING:
			COPY_I(objPacked->mType.spinRate.x , obj->mType.spinRate.v.coord.x);
			COPY_I(objPacked->mType.spinRate.y , obj->mType.spinRate.v.coord.y);
			COPY_I(objPacked->mType.spinRate.z , obj->mType.spinRate.v.coord.z);
			break;

		default:
			PrintLog(0, "unknown movement type %d obj type %d\n", obj->info.movementType, obj->info.nType);
	}

	switch (obj->info.controlType)
	{
		case CT_NONE:
		case CT_FLYING:
		case CT_CNTRLCEN:
		case CT_REMOTE:
		case CT_SLEW:
		case CT_DEBRIS:
			break;

		case CT_WEAPON:
			COPY_S(objPacked->cType.laserInfo.parentType      , obj->cType.laserInfo.parent.nType);
			COPY_S(objPacked->cType.laserInfo.parentNum       , obj->cType.laserInfo.parent.nObject);
			COPY_I(objPacked->cType.laserInfo.parentSignature , obj->cType.laserInfo.parent.nSignature);
			COPY_I(objPacked->cType.laserInfo.xCreationTime   , obj->cType.laserInfo.xCreationTime);
			COPY_S(objPacked->cType.laserInfo.nLastHitObj     , obj->cType.laserInfo.nLastHitObj);
			COPY_S(objPacked->cType.laserInfo.nHomingTarget   , obj->cType.laserInfo.nHomingTarget);
			COPY_I(objPacked->cType.laserInfo.xScale          , obj->cType.laserInfo.xScale);
			break;

		case CT_EXPLOSION:
			COPY_I(objPacked->cType.explInfo.nSpawnTime    , obj->cType.explInfo.nSpawnTime);
			COPY_I(objPacked->cType.explInfo.nDeleteTime   , obj->cType.explInfo.nDeleteTime);
			COPY_S(objPacked->cType.explInfo.nDestroyedObj , obj->cType.explInfo.nDestroyedObj);
			COPY_S(objPacked->cType.explInfo.attachParent  , obj->cType.explInfo.attached.nParent);
			COPY_S(objPacked->cType.explInfo.attachPrev    , obj->cType.explInfo.attached.nPrev);
			COPY_S(objPacked->cType.explInfo.attachNext    , obj->cType.explInfo.attached.nNext);
			break;

		case CT_AI:
		case CT_MORPH:
		case CT_CAMERA:
		{
			int i;
			COPY_B(objPacked->cType.aiInfo.behavior           , obj->cType.aiInfo.behavior);
			for (i = 0; i < MAX_AI_FLAGS; i++)
				COPY_B(objPacked->cType.aiInfo.flags[i]       , obj->cType.aiInfo.flags[i]);
			COPY_S(objPacked->cType.aiInfo.nHideSegment       , obj->cType.aiInfo.nHideSegment);
			COPY_S(objPacked->cType.aiInfo.nHideIndex         , obj->cType.aiInfo.nHideIndex);
			COPY_S(objPacked->cType.aiInfo.nPathLength        , obj->cType.aiInfo.nPathLength);
			COPY_B(objPacked->cType.aiInfo.nCurPathIndex      , obj->cType.aiInfo.nCurPathIndex);
			COPY_B(objPacked->cType.aiInfo.bDyingSoundPlaying , obj->cType.aiInfo.bDyingSoundPlaying);
			COPY_S(objPacked->cType.aiInfo.nDangerLaser       , obj->cType.aiInfo.nDangerLaser);
			COPY_I(objPacked->cType.aiInfo.nDangerLaserSig    , obj->cType.aiInfo.nDangerLaserSig);
			COPY_I(objPacked->cType.aiInfo.xDyingStartTime    , obj->cType.aiInfo.xDyingStartTime);
			break;
		}

		case CT_POWERUP:
			COPY_I(objPacked->cType.powerupInfo.nCount         , obj->cType.powerupInfo.nCount);
			COPY_I(objPacked->cType.powerupInfo.xCreationTime  , obj->cType.powerupInfo.xCreationTime);
			COPY_I(objPacked->cType.powerupInfo.nFlags         , obj->cType.powerupInfo.nFlags);
			break;

		case CT_LIGHT:
		    COPY_I(objPacked->cType.lightInfo.intensity, obj->cType.lightInfo.intensity);
			COPY_S(objPacked->cType.lightInfo.nSegment, obj->cType.lightInfo.nSegment);
			COPY_S(objPacked->cType.lightInfo.nObjects, obj->cType.lightInfo.nObjects);
			COPY_F(objPacked->cType.lightInfo.color, obj->cType.lightInfo.color);
			break;

		case CT_WAYPOINT:
			COPY_I(objPacked->cType.wayPointInfo.nId [0], obj->cType.wayPointInfo.nId [0]);
			COPY_I(objPacked->cType.wayPointInfo.nId [1], obj->cType.wayPointInfo.nId [1]);
			COPY_I(objPacked->cType.wayPointInfo.nSuccessor [0], obj->cType.wayPointInfo.nSuccessor [0]);
			COPY_I(objPacked->cType.wayPointInfo.nSuccessor [1], obj->cType.wayPointInfo.nSuccessor [1]);
			COPY_I(objPacked->cType.wayPointInfo.nSpeed, obj->cType.wayPointInfo.nSpeed);
			break;

		default:
			PrintLog(0, "unknown control type %d obj type %d\n", obj->info.controlType, obj->info.nType);
	}

	switch (obj->info.renderType)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->info.renderType == RT_NONE && obj->info.nType != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if renderType == RT_NONE at this time.
				break;
			COPY_I(objPacked->rType.polyObjInfo.nModel              , obj->rType.polyObjInfo.nModel);
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				COPY_S(objPacked->rType.polyObjInfo.animAngles[i].p , obj->rType.polyObjInfo.animAngles[i].v.coord.p);
				COPY_S(objPacked->rType.polyObjInfo.animAngles[i].b , obj->rType.polyObjInfo.animAngles[i].v.coord.b);
				COPY_S(objPacked->rType.polyObjInfo.animAngles[i].h , obj->rType.polyObjInfo.animAngles[i].v.coord.h);
			}
			COPY_I(objPacked->rType.polyObjInfo.nSubObjFlags        , obj->rType.polyObjInfo.nSubObjFlags);
			COPY_I(objPacked->rType.polyObjInfo.nTexOverride        , obj->rType.polyObjInfo.nTexOverride);
			COPY_I(objPacked->rType.polyObjInfo.nAltTextures        , obj->rType.polyObjInfo.nAltTextures);
			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
		case RT_THRUSTER:
		case RT_EXPLBLAST:
		case RT_SHRAPNELS:
		case RT_SHOCKWAVE:
			COPY_I(objPacked->rType.animationInfo.nClipIndex , obj->rType.animationInfo.nClipIndex);
			COPY_I(objPacked->rType.animationInfo.xTotalTime , obj->rType.animationInfo.xTotalTime);
			COPY_I(objPacked->rType.animationInfo.xFrameTime , obj->rType.animationInfo.xFrameTime);
			COPY_B(objPacked->rType.animationInfo.nCurFrame  , obj->rType.animationInfo.nCurFrame);
			break;

		case RT_LASER:
			break;

		case RT_PARTICLES:
			COPY_I(objPacked->rType.particleInfo.nLife , obj->rType.particleInfo.nLife);
			COPY_I(objPacked->rType.particleInfo.nSize [0] , obj->rType.particleInfo.nSize [0]);
			COPY_I(objPacked->rType.particleInfo.nSize [1] , obj->rType.particleInfo.nSize [1]);
			COPY_I(objPacked->rType.particleInfo.nParts , obj->rType.particleInfo.nParts);
			COPY_I(objPacked->rType.particleInfo.nSpeed , obj->rType.particleInfo.nSpeed);
			COPY_I(objPacked->rType.particleInfo.nDrift , obj->rType.particleInfo.nDrift);
			COPY_I(objPacked->rType.particleInfo.nBrightness , obj->rType.particleInfo.nBrightness);
			COPY_I(objPacked->rType.particleInfo.color , obj->rType.particleInfo.color);
			COPY_B(objPacked->rType.particleInfo.nSide , obj->rType.particleInfo.nSide);
			COPY_B(objPacked->rType.particleInfo.nType , obj->rType.particleInfo.nType);
			COPY_B(objPacked->rType.particleInfo.bEnabled , obj->rType.particleInfo.bEnabled);
			break;

		case RT_LIGHTNING:
			COPY_I(objPacked->rType.lightningInfo.nLife , obj->rType.lightningInfo.nLife);
			COPY_I(objPacked->rType.lightningInfo.nDelay , obj->rType.lightningInfo.nDelay);
			COPY_I(objPacked->rType.lightningInfo.nLength , obj->rType.lightningInfo.nLength);
			COPY_I(objPacked->rType.lightningInfo.nAmplitude , obj->rType.lightningInfo.nAmplitude);
			COPY_I(objPacked->rType.lightningInfo.nOffset , obj->rType.lightningInfo.nOffset);
			COPY_S(objPacked->rType.lightningInfo.nBolts , obj->rType.lightningInfo.nBolts);
			COPY_S(objPacked->rType.lightningInfo.nId , obj->rType.lightningInfo.nId);
			COPY_S(objPacked->rType.lightningInfo.nTarget , obj->rType.lightningInfo.nTarget);
			COPY_S(objPacked->rType.lightningInfo.nNodes , obj->rType.lightningInfo.nNodes);
			COPY_S(objPacked->rType.lightningInfo.nChildren , obj->rType.lightningInfo.nChildren);
			COPY_S(objPacked->rType.lightningInfo.nFrames , obj->rType.lightningInfo.nFrames);
			COPY_B(objPacked->rType.lightningInfo.nWidth , obj->rType.lightningInfo.nWidth);
			COPY_B(objPacked->rType.lightningInfo.nAngle , obj->rType.lightningInfo.nAngle);
			COPY_B(objPacked->rType.lightningInfo.nStyle , obj->rType.lightningInfo.nStyle);
			COPY_B(objPacked->rType.lightningInfo.nSmoothe , obj->rType.lightningInfo.nSmoothe);
			COPY_B(objPacked->rType.lightningInfo.bClamp , obj->rType.lightningInfo.bClamp);
			COPY_B(objPacked->rType.lightningInfo.bGlow , obj->rType.lightningInfo.bGlow);
			COPY_B(objPacked->rType.lightningInfo.bSound , obj->rType.lightningInfo.bSound);
			COPY_B(objPacked->rType.lightningInfo.bRandom , obj->rType.lightningInfo.bRandom);
			COPY_B(objPacked->rType.lightningInfo.bInPlane , obj->rType.lightningInfo.bInPlane);
			COPY_I(objPacked->rType.lightningInfo.color , obj->rType.lightningInfo.color);
			COPY_B(objPacked->rType.lightningInfo.bEnabled , obj->rType.lightningInfo.bEnabled);
			break;

		case RT_SOUND:
			memcpy (objPacked->rType.soundInfo.szFilename, obj->rType.soundInfo.szFilename, sizeof (objPacked->rType.soundInfo.szFilename));
			objPacked->rType.soundInfo.szFilename [sizeof (objPacked->rType.soundInfo.szFilename) - 1] = '\0';
			COPY_I(objPacked->rType.soundInfo.nVolume , obj->rType.soundInfo.nVolume);
			COPY_B(objPacked->rType.soundInfo.bEnabled , obj->rType.soundInfo.bEnabled);
			break;

		default:
			PrintLog(0, "unknown render type %d obj type %d\n", obj->info.renderType, obj->info.nType);
	}
}

#undef COPY_S
#undef COPY_I
#undef COPY_F
#define COPY_S(a, b) memcpy(&a, &b, sizeof(a))
#define COPY_I(a, b) memcpy(&a, &b, sizeof(a))
#define COPY_F(a, b) memcpy(&a, &b, sizeof(a))

void ObjectReadFromBuffer(CObject *obj, void *buffer, bool compat)
{
	tObjectPacked *objPacked = (tObjectPacked *)buffer;
	COPY_I(obj->info.nSignature        , objPacked->nSignature);
	COPY_B(obj->info.nType             , objPacked->type);
	COPY_B(obj->info.nId               , objPacked->id);
	COPY_S(obj->info.nNextInSeg        , objPacked->next);
	COPY_S(obj->info.nPrevInSeg        , objPacked->prev);
	COPY_B(obj->info.controlType       , objPacked->controlType);
	COPY_B(obj->info.movementType      , objPacked->movementType);
	COPY_B(obj->info.renderType        , objPacked->renderType);
	COPY_B(obj->info.nFlags            , objPacked->flags);
	COPY_S(obj->info.nSegment          , objPacked->nSegment);
	COPY_S(obj->info.nAttachedObj      , objPacked->attached_obj);
	COPY_I(obj->info.position.vPos.v.coord.x         , objPacked->pos.x);
	COPY_I(obj->info.position.vPos.v.coord.y         , objPacked->pos.y);
	COPY_I(obj->info.position.vPos.v.coord.z         , objPacked->pos.z);
	COPY_I(obj->info.position.mOrient.m.dir.r.v.coord.x , objPacked->orient.rvec.x);
	COPY_I(obj->info.position.mOrient.m.dir.r.v.coord.y , objPacked->orient.rvec.y);
	COPY_I(obj->info.position.mOrient.m.dir.r.v.coord.z , objPacked->orient.rvec.z);
	COPY_I(obj->info.position.mOrient.m.dir.f.v.coord.x , objPacked->orient.fvec.x);
	COPY_I(obj->info.position.mOrient.m.dir.f.v.coord.y , objPacked->orient.fvec.y);
	COPY_I(obj->info.position.mOrient.m.dir.f.v.coord.z , objPacked->orient.fvec.z);
	COPY_I(obj->info.position.mOrient.m.dir.u.v.coord.x , objPacked->orient.uvec.x);
	COPY_I(obj->info.position.mOrient.m.dir.u.v.coord.y , objPacked->orient.uvec.y);
	COPY_I(obj->info.position.mOrient.m.dir.u.v.coord.z , objPacked->orient.uvec.z);
	COPY_I(obj->info.xSize              , objPacked->xSize);
	COPY_I(obj->info.xShield            , objPacked->xShield);
	COPY_I(obj->info.vLastPos.v.coord.x , objPacked->vLastPos.x);
	COPY_I(obj->info.vLastPos.v.coord.y , objPacked->vLastPos.y);
	COPY_I(obj->info.vLastPos.v.coord.z , objPacked->vLastPos.z);
	COPY_B(obj->info.contains.nType     , objPacked->containsType);
	COPY_B(obj->info.contains.nId       , objPacked->containsId);
	COPY_B(obj->info.contains.nCount    , objPacked->containsCount);
	COPY_B(obj->info.nCreator           , objPacked->nCreator);
	COPY_I(obj->info.xLifeLeft          , objPacked->xLifeLeft);

	switch (obj->info.movementType)
	{
		case MT_NONE:
			break;

		case MT_PHYSICS:
			COPY_I(obj->mType.physInfo.velocity.v.coord.x  , objPacked->mType.physInfo.velocity.x);
			COPY_I(obj->mType.physInfo.velocity.v.coord.y  , objPacked->mType.physInfo.velocity.y);
			COPY_I(obj->mType.physInfo.velocity.v.coord.z  , objPacked->mType.physInfo.velocity.z);
			COPY_I(obj->mType.physInfo.thrust.v.coord.x    , objPacked->mType.physInfo.thrust.x);
			COPY_I(obj->mType.physInfo.thrust.v.coord.y    , objPacked->mType.physInfo.thrust.y);
			COPY_I(obj->mType.physInfo.thrust.v.coord.z    , objPacked->mType.physInfo.thrust.z);
			COPY_I(obj->mType.physInfo.mass        , objPacked->mType.physInfo.mass);
			COPY_I(obj->mType.physInfo.drag        , objPacked->mType.physInfo.drag);
			COPY_I(obj->mType.physInfo.brakes      , objPacked->mType.physInfo.brakes);
			COPY_I(obj->mType.physInfo.rotVel.v.coord.x    , objPacked->mType.physInfo.rotVel.x);
			COPY_I(obj->mType.physInfo.rotVel.v.coord.y    , objPacked->mType.physInfo.rotVel.y);
			COPY_I(obj->mType.physInfo.rotVel.v.coord.z    , objPacked->mType.physInfo.rotVel.z);
			COPY_I(obj->mType.physInfo.rotThrust.v.coord.x , objPacked->mType.physInfo.rotThrust.x);
			COPY_I(obj->mType.physInfo.rotThrust.v.coord.y , objPacked->mType.physInfo.rotThrust.y);
			COPY_I(obj->mType.physInfo.rotThrust.v.coord.z , objPacked->mType.physInfo.rotThrust.z);
			COPY_S(obj->mType.physInfo.turnRoll    , objPacked->mType.physInfo.turnRoll);
			COPY_S(obj->mType.physInfo.flags       , objPacked->mType.physInfo.flags);
			break;

		case MT_SPINNING:
			COPY_I(obj->mType.spinRate.v.coord.x , objPacked->mType.spinRate.x);
			COPY_I(obj->mType.spinRate.v.coord.y , objPacked->mType.spinRate.y);
			COPY_I(obj->mType.spinRate.v.coord.z , objPacked->mType.spinRate.z);
			break;

		default:
			PrintLog(0, "unknown movement type %d obj type %d\n", obj->info.movementType, obj->info.nType);
	}

	switch (obj->info.controlType)
	{
		case CT_NONE:
		case CT_FLYING:
		case CT_CNTRLCEN:
		case CT_REMOTE:
		case CT_SLEW:
		case CT_DEBRIS:
			break;

		case CT_WEAPON:
			COPY_S(obj->cType.laserInfo.parent.nType      , objPacked->cType.laserInfo.parentType);
			COPY_S(obj->cType.laserInfo.parent.nObject    , objPacked->cType.laserInfo.parentNum);
			COPY_I(obj->cType.laserInfo.parent.nSignature , objPacked->cType.laserInfo.parentSignature);
			COPY_I(obj->cType.laserInfo.xCreationTime     , objPacked->cType.laserInfo.xCreationTime);
			COPY_S(obj->cType.laserInfo.nLastHitObj       , objPacked->cType.laserInfo.nLastHitObj);
			COPY_S(obj->cType.laserInfo.nHomingTarget     , objPacked->cType.laserInfo.nHomingTarget);
			COPY_I(obj->cType.laserInfo.xScale            , objPacked->cType.laserInfo.xScale);
			break;

		case CT_EXPLOSION:
			COPY_I(obj->cType.explInfo.nSpawnTime       , objPacked->cType.explInfo.nSpawnTime);
			COPY_I(obj->cType.explInfo.nDeleteTime      , objPacked->cType.explInfo.nDeleteTime);
			COPY_S(obj->cType.explInfo.nDestroyedObj    , objPacked->cType.explInfo.nDestroyedObj);
			COPY_S(obj->cType.explInfo.attached.nParent , objPacked->cType.explInfo.attachParent);
			COPY_S(obj->cType.explInfo.attached.nPrev   , objPacked->cType.explInfo.attachPrev);
			COPY_S(obj->cType.explInfo.attached.nNext   , objPacked->cType.explInfo.attachNext);
			break;

		case CT_AI:
		case CT_MORPH:
		case CT_CAMERA:
		{
			int i;
			COPY_B(obj->cType.aiInfo.behavior           , objPacked->cType.aiInfo.behavior);
			for (i = 0; i < MAX_AI_FLAGS; i++)
				COPY_B(obj->cType.aiInfo.flags[i]       , objPacked->cType.aiInfo.flags[i]);
			COPY_S(obj->cType.aiInfo.nHideSegment       , objPacked->cType.aiInfo.nHideSegment);
			COPY_S(obj->cType.aiInfo.nHideIndex         , objPacked->cType.aiInfo.nHideIndex);
			COPY_S(obj->cType.aiInfo.nPathLength        , objPacked->cType.aiInfo.nPathLength);
			COPY_B(obj->cType.aiInfo.nCurPathIndex      , objPacked->cType.aiInfo.nCurPathIndex);
			COPY_B(obj->cType.aiInfo.bDyingSoundPlaying , objPacked->cType.aiInfo.bDyingSoundPlaying);
			COPY_S(obj->cType.aiInfo.nDangerLaser       , objPacked->cType.aiInfo.nDangerLaser);
			COPY_I(obj->cType.aiInfo.nDangerLaserSig    , objPacked->cType.aiInfo.nDangerLaserSig);
			COPY_I(obj->cType.aiInfo.xDyingStartTime    , objPacked->cType.aiInfo.xDyingStartTime);
			break;
		}

		case CT_POWERUP:
			COPY_I(obj->cType.powerupInfo.nCount        , objPacked->cType.powerupInfo.nCount);
			COPY_I(obj->cType.powerupInfo.xCreationTime , objPacked->cType.powerupInfo.xCreationTime);
			COPY_I(obj->cType.powerupInfo.nFlags        , objPacked->cType.powerupInfo.nFlags);
			break;

		case CT_LIGHT:
		    COPY_I(obj->cType.lightInfo.intensity, objPacked->cType.lightInfo.intensity);
			COPY_S(obj->cType.lightInfo.nSegment, objPacked->cType.lightInfo.nSegment);
			COPY_S(obj->cType.lightInfo.nObjects, objPacked->cType.lightInfo.nObjects);
			COPY_F(obj->cType.lightInfo.color, objPacked->cType.lightInfo.color);
			break;

		case CT_WAYPOINT:
			COPY_I(obj->cType.wayPointInfo.nId [0], objPacked->cType.wayPointInfo.nId [0]);
			COPY_I(obj->cType.wayPointInfo.nId [1], objPacked->cType.wayPointInfo.nId [1]);
			COPY_I(obj->cType.wayPointInfo.nSuccessor [0], objPacked->cType.wayPointInfo.nSuccessor [0]);
			COPY_I(obj->cType.wayPointInfo.nSuccessor [1], objPacked->cType.wayPointInfo.nSuccessor [1]);
			COPY_I(obj->cType.wayPointInfo.nSpeed, objPacked->cType.wayPointInfo.nSpeed);
			break;

		default:
			PrintLog(0, "unknown control type %d obj type %d\n", obj->info.controlType, obj->info.nType);
	}

	switch (obj->info.renderType)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->info.renderType == RT_NONE && obj->info.nType != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if renderType == RT_NONE at this time.
				break;
			COPY_I(obj->rType.polyObjInfo.nModel                , objPacked->rType.polyObjInfo.nModel);
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				COPY_S(obj->rType.polyObjInfo.animAngles[i].v.coord.p , objPacked->rType.polyObjInfo.animAngles[i].p);
				COPY_S(obj->rType.polyObjInfo.animAngles[i].v.coord.b , objPacked->rType.polyObjInfo.animAngles[i].b);
				COPY_S(obj->rType.polyObjInfo.animAngles[i].v.coord.h , objPacked->rType.polyObjInfo.animAngles[i].h);
			}
			COPY_I(obj->rType.polyObjInfo.nSubObjFlags             , objPacked->rType.polyObjInfo.nSubObjFlags);
			COPY_I(obj->rType.polyObjInfo.nTexOverride            , objPacked->rType.polyObjInfo.nTexOverride);
			COPY_I(obj->rType.polyObjInfo.nAltTextures             , objPacked->rType.polyObjInfo.nAltTextures);
			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
		case RT_THRUSTER:
		case RT_EXPLBLAST:
		case RT_SHRAPNELS:
		case RT_SHOCKWAVE:
			if (compat) {
				COPY_I(obj->rType.animationInfo.nClipIndex , objPacked->rType.animationInfoCompat.nClipIndex);
				COPY_I(obj->rType.animationInfo.xFrameTime , objPacked->rType.animationInfoCompat.xFrameTime);
				COPY_B(obj->rType.animationInfo.nCurFrame  , objPacked->rType.animationInfoCompat.nCurFrame);
				obj->rType.animationInfo.xTotalTime = 0; // only used for thruster clips
			} else {
				COPY_I(obj->rType.animationInfo.nClipIndex , objPacked->rType.animationInfo.nClipIndex);
				COPY_I(obj->rType.animationInfo.xTotalTime , objPacked->rType.animationInfo.xTotalTime);
				COPY_I(obj->rType.animationInfo.xFrameTime , objPacked->rType.animationInfo.xFrameTime);
				COPY_B(obj->rType.animationInfo.nCurFrame  , objPacked->rType.animationInfo.nCurFrame);
			}
			break;

		case RT_LASER:
			break;

		case RT_PARTICLES:
			COPY_I(obj->rType.particleInfo.nLife , objPacked->rType.particleInfo.nLife);
			COPY_I(obj->rType.particleInfo.nSize [0] , objPacked->rType.particleInfo.nSize [0]);
			COPY_I(obj->rType.particleInfo.nSize [1] , objPacked->rType.particleInfo.nSize [1]);
			COPY_I(obj->rType.particleInfo.nParts , objPacked->rType.particleInfo.nParts);
			COPY_I(obj->rType.particleInfo.nSpeed , objPacked->rType.particleInfo.nSpeed);
			COPY_I(obj->rType.particleInfo.nDrift , objPacked->rType.particleInfo.nDrift);
			COPY_I(obj->rType.particleInfo.nBrightness , objPacked->rType.particleInfo.nBrightness);
			COPY_I(obj->rType.particleInfo.color , objPacked->rType.particleInfo.color);
			COPY_B(obj->rType.particleInfo.nSide , objPacked->rType.particleInfo.nSide);
			COPY_B(obj->rType.particleInfo.nType , objPacked->rType.particleInfo.nType);
			COPY_B(obj->rType.particleInfo.bEnabled , objPacked->rType.particleInfo.bEnabled);

		case RT_LIGHTNING:
			COPY_I(obj->rType.lightningInfo.nLife , objPacked->rType.lightningInfo.nLife);
			COPY_I(obj->rType.lightningInfo.nDelay , objPacked->rType.lightningInfo.nDelay);
			COPY_I(obj->rType.lightningInfo.nLength , objPacked->rType.lightningInfo.nLength);
			COPY_I(obj->rType.lightningInfo.nAmplitude , objPacked->rType.lightningInfo.nAmplitude);
			COPY_I(obj->rType.lightningInfo.nOffset , objPacked->rType.lightningInfo.nOffset);
			COPY_S(obj->rType.lightningInfo.nBolts , objPacked->rType.lightningInfo.nBolts);
			COPY_S(obj->rType.lightningInfo.nId , objPacked->rType.lightningInfo.nId);
			COPY_S(obj->rType.lightningInfo.nTarget , objPacked->rType.lightningInfo.nTarget);
			COPY_S(obj->rType.lightningInfo.nNodes , objPacked->rType.lightningInfo.nNodes);
			COPY_S(obj->rType.lightningInfo.nChildren , objPacked->rType.lightningInfo.nChildren);
			COPY_S(obj->rType.lightningInfo.nFrames , objPacked->rType.lightningInfo.nFrames);
			COPY_B(obj->rType.lightningInfo.nWidth , objPacked->rType.lightningInfo.nWidth);
			COPY_B(obj->rType.lightningInfo.nAngle , objPacked->rType.lightningInfo.nAngle);
			COPY_B(obj->rType.lightningInfo.nStyle , objPacked->rType.lightningInfo.nStyle);
			COPY_B(obj->rType.lightningInfo.nSmoothe , objPacked->rType.lightningInfo.nSmoothe);
			COPY_B(obj->rType.lightningInfo.bClamp , objPacked->rType.lightningInfo.bClamp);
			COPY_B(obj->rType.lightningInfo.bGlow , objPacked->rType.lightningInfo.bGlow);
			COPY_B(obj->rType.lightningInfo.bSound , objPacked->rType.lightningInfo.bSound);
			COPY_B(obj->rType.lightningInfo.bRandom , objPacked->rType.lightningInfo.bRandom);
			COPY_B(obj->rType.lightningInfo.bInPlane , objPacked->rType.lightningInfo.bInPlane);
			COPY_I(obj->rType.lightningInfo.color , objPacked->rType.lightningInfo.color);
			COPY_B(obj->rType.lightningInfo.bEnabled , objPacked->rType.lightningInfo.bEnabled);
			break;

		case RT_SOUND:
			memcpy (obj->rType.soundInfo.szFilename, objPacked->rType.soundInfo.szFilename, sizeof (obj->rType.soundInfo.szFilename));
			obj->rType.soundInfo.szFilename [sizeof (obj->rType.soundInfo.szFilename) - 1] = '\0';
			COPY_I(obj->rType.soundInfo.nVolume , objPacked->rType.soundInfo.nVolume);
			COPY_B(obj->rType.soundInfo.bEnabled , objPacked->rType.soundInfo.bEnabled);
			break;

		default:
			PrintLog(0, "unknown render type %d obj type %d\n", obj->info.renderType, obj->info.nType);
	}
}
