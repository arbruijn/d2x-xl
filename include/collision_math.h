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

#ifndef _COLLISION_MATH_H
#define _COLLISION_MATH_H

#include "vecmat.h"
#include "segment.h"
#include "object.h"

//	-----------------------------------------------------------------------------

//return values for FindHitpoint() - what did we hit?
#define HIT_NONE					0		//we hit nothing
#define HIT_WALL					1		//we hit - guess - a CWall
#define HIT_OBJECT				2		//we hit an CObject - which one?  no way to tell...
#define HIT_BAD_P0				3		//start point is not in specified CSegment

//flags for fvi query
#define FQ_CHECK_OBJS			1		//check against objects?
#define FQ_TRANSWALL				2		//go through transparent walls
#define FQ_TRANSPOINT			4		//go through trans CWall if hit point is transparent
#define FQ_GET_SEGLIST			8		//build a list of segments
#define FQ_IGNORE_POWERUPS		16		//ignore powerups
#define FQ_VISIBLE_OBJS			32
#define FQ_ANY_OBJECT			64
#define FQ_CHECK_PLAYER			128
#define FQ_VISIBILITY			256

//intersection types
#define IT_ERROR					-1
#define IT_NONE					0      //doesn't touch face at all
#define IT_FACE					1      //touches face
#define IT_EDGE					2      //touches edge of face
#define IT_POINT					3      //touches vertex

#define MAX_FVI_SEGS 200

#define FLOAT_COLLISION_MATH	1
#define FLOAT_DIST_TOLERANCE	1e-6f


//	-----------------------------------------------------------------------------

class CFixVector2 {
	public:
		fix m_x, m_y;

	CFixVector2 () : m_x (0), m_y (0) {}
	CFixVector2 (fix x, fix y) : m_x (x), m_y (y) {}
	inline fix Cross (CFixVector2& other) { return fix (double (m_x) / 65536.0 * double (other.m_y) - double (m_y) / 65536.0 * double (other.m_x)); }
	};

//	-----------------------------------------------------------------------------

class CFloatVector2 {
	public:
		float m_x, m_y;

	CFloatVector2 () : m_x (0), m_y (0) {}
	CFloatVector2 (fix x, fix y) : m_x (X2F (x)), m_y (X2F (y)) {}
	CFloatVector2 (float x, float y) : m_x (x), m_y (y) {}
	inline float Cross (CFloatVector2& other) { return m_x * other.m_y - m_y * other.m_x; }
	inline CFloatVector2& operator-= (CFloatVector2& other) { 
		m_x -= other.m_x;
		m_y -= other.m_y;
		return *this;
		}
	};

//	-----------------------------------------------------------------------------

class CHitData {
	public:
		int32_t 		nType;						//what sort of intersection
		int16_t		nSegment;
		int16_t		nObject;
		CFixVector	vPoint;
		CFixVector	vNormal;

	public:
		CHitData () : nType (0), nSegment (-1), nObject (-1) { 
			vPoint.SetZero (); 
			vNormal.SetZero ();
			}

	inline CHitData& operator= (const CHitData& other) { 
		memcpy (this, &other, sizeof (CHitData)); 
		return *this;
		}
			
	};

//	-----------------------------------------------------------------------------

class CHitInfo : public CHitData {
	public:
		int16_t		nAltSegment;
		int16_t 		nSide;						//if hit wall, which side
		int16_t		nFace;
		int16_t 		nSideSegment;				//what segment the hit side is in
		int32_t		nNormals;
		int32_t		nNestCount;

	public:
		CHitInfo () : nAltSegment (-1), nSide (0), nFace (0), nSideSegment (-1), nNormals (0), nNestCount (0) {}
	};

//	-----------------------------------------------------------------------------
//this data structure gets filled in by FindHitpoint()

class CHitResult : public CHitInfo {
	public:
		int16_t 	nSegments;					//how many segs we went through
		int16_t 	segList [MAX_FVI_SEGS];	//list of segs vector went through

	public:
		CHitResult () : nSegments (0) {}
	};

//	-----------------------------------------------------------------------------

//this data contains the parms to fvi()
class CHitQuery {
	public:
		int32_t		flags;
		CFixVector*	p0, * p1;
		int16_t		nSegment;
		int16_t		nObject;
		fix			radP0, radP1;
		int32_t		bIgnoreObjFlag;

	public:
		CHitQuery () 
			: flags (0), p0 (NULL), p1 (NULL), nSegment (-1), nObject (-1), radP0 (0), radP1 (0), bIgnoreObjFlag (0)
			{}
		CHitQuery (int32_t flags, CFixVector* p0, CFixVector* p1, int16_t nSegment, int16_t nObject = -1, fix radP0 = 0, fix radP1 = 0, int32_t bIgnoreObjFlag = 0)
			: flags (flags), p0 (p0), p1 (p1), nSegment (nSegment), nObject (nObject), radP0 (radP0), radP1 (radP1), bIgnoreObjFlag (bIgnoreObjFlag)
			{}
		bool InFoV (CObject *pObj, float fov = -2.0f);
	};

//------------------------------------------------------------------------------

typedef struct tSpeedBoostData {
	int32_t			bBoosted;
	CFixVector		vVel;
	CFixVector		vMinVel;
	CFixVector		vMaxVel;
	CFixVector		vSrc;
	CFixVector		vDest;
} tSpeedBoostData;

//	-----------------------------------------------------------------------------

class CPhysSimData {
	public:
		int16_t				nObject;
		CObject*				pObj;
		int16_t				nStartSeg;
		CFixVector			vStartPos;
		int16_t				nOldSeg;
		CFixVector			velocity;
		CFixVector			vOldPos;
		CFixVector			vNewPos;
		CFixVector			vHitPos;
		CFixVector			vOffset;
		CFixVector			vMoved;
		fix					xSimTime;
		fix					xOldSimTime;
		fix					xTimeScale;
		fix					xMovedTime;
		fix					xMovedDist;
		fix					xAttemptedDist;
		tSpeedBoostData	speedBoost;
		CHitResult			hitResult;
		CHitQuery			hitQuery;
		int32_t				bInitialize;
		int32_t				bUpdateOffset;
		int32_t				bUseHitbox;
		int32_t				bGetPhysSegs;
		int32_t				bSpeedBoost;
		int32_t				bScaleSpeed;
		int32_t				bStopped;
		int32_t				bBounced;
		int32_t				bIgnoreObjFlag;
		int32_t				nTries;

		explicit CPhysSimData (int16_t nObject = -1) : nObject (nObject), bUpdateOffset (1), bStopped (0), bBounced (0), bIgnoreObjFlag (0), nTries (0) { Setup (); }
		void Setup (void);
		void GetPhysSegs (void);
	};

//	-----------------------------------------------------------------------------

//Find out if a vector intersects with anything.
//Fills in hitResult, a CHitResult structure (see above).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisobjnum 		used to prevent an CObject with colliding with itself
//  ingore_obj_list	NULL, or ptr to a list of objnums to ignore, terminated with -1
//  check_objFlag	determines whether collisions with objects are checked
//Returns the hitResult->hitType
int32_t FindHitpoint (CHitQuery& hitQuery, CHitResult& hitResult, int32_t nCollisionModel = -1, int32_t nThread = 0);

//finds the uv coords of the given point on the given seg & CSide
//fills in u & v. if l is non-NULL fills it in also
void FindHitPointUV (fix *u,fix *v,fix *l, CFixVector *pnt,CSegment *seg, int32_t nSide, int32_t facenum);

//Returns true if the CObject is through any walls
int32_t ObjectIntersectsWall (CObject *pObj);

int32_t CheckLineToSegFace (CFixVector *newP, CFixVector *p0, CFixVector *p1, int16_t nSegment, int16_t nSide, int16_t iFace, int32_t nv, fix rad);

int32_t FindPlaneLineIntersection (CFixVector& intersection, CFixVector *vPlanePoint, CFixVector *vPlaneNorm, CFixVector *p0, CFixVector *p1, fix rad);

int32_t FindPlaneLineIntersectionf (CFloatVector& intersection, CFloatVector& vPlanePoint, CFloatVector& vPlaneNorm, CFloatVector& p0, CFloatVector& p1, float rad);

int32_t CheckLineToLine (fix *t1, fix *t2, CFixVector *p1, CFixVector *v1, CFixVector *p2, CFixVector *v2);

int32_t CheckLineToLinef (float& t1, float& t2, CFloatVector& p1, CFloatVector& v1, CFloatVector& p2, CFloatVector& v2);

float DistToFace (CFloatVector3 vRef, int16_t nSegment, uint8_t nSide, CFloatVector3* vHit = NULL);

fix CheckVectorHitboxCollision (CFixVector& intersection, CFixVector& normal, CFixVector *p0, CFixVector *p1, CFixVector *vRef, CObject *pObj, fix rad, int16_t& nModel);

fix CheckHitboxCollision (CFixVector& intersection, CFixVector& normal, CObject *pObj1, CObject *pObj2, CFixVector *p0, CFixVector *p1, int16_t& nModel);

CSegMasks CheckFaceHitboxCollision (CFixVector& intersection, CFixVector& normal, int16_t nSegment, int16_t nSide, CFixVector* p0, CFixVector* p1, CObject *pObj);

uint8_t PointIsOutsideFace (CFixVector* pRef, CFixVector* vertices, int16_t nVerts);

uint8_t PointIsOutsideFace (CFixVector* pRef, uint16_t* nVertIndex, int16_t nVerts);

uint8_t PointIsOutsideFace (CFloatVector* pRef, uint16_t* nVertIndex, int16_t nVerts);

uint32_t PointToFaceRelation (CFixVector* pRef, CFixVector *vertList, int32_t nVerts, CFixVector* vNormal);

uint32_t PointToFaceRelationf (CFloatVector& vRef, CFloatVector* vertList, int32_t nVerts, CFloatVector& vNormal);

int32_t PointSeesPoint (CFloatVector* p0, CFloatVector* p1, int16_t nStartSeg, int16_t nDestSeg, int8_t nDestSide, int32_t nDepth, int32_t nThread);

//	-----------------------------------------------------------------------------

static inline fix RegisterHit (CFixVector *vBestHit, CFixVector *vCurHit, CFixVector *vRef, fix& dMin)
{
   fix d = CFixVector::Dist (*vRef, *vCurHit);

if (dMin > d) {
	dMin = d;
	*vBestHit = *vCurHit;
	}
return dMin;
}

//	-----------------------------------------------------------------------------

static inline fix RegisterHit (CFixVector *vBestHit, CFixVector *vBestNormal, CFixVector *vCurHit, CFixVector* vCurNormal, CFixVector *vRef, fix& dMin)
{
   fix d = CFixVector::Dist (*vRef, *vCurHit);

if (dMin < d)
	return 0;
dMin = d;
*vBestHit = *vCurHit;
*vBestNormal = *vCurNormal;
return 1;
}

//	-----------------------------------------------------------------------------

int32_t UseHitbox (CObject *pObj);
int32_t UseSphere (CObject *pObj);

//	-----------------------------------------------------------------------------

extern int32_t ijTable [3][2];

#endif

