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
#define HIT_NONE		0		//we hit nothing
#define HIT_WALL		1		//we hit - guess - a CWall
#define HIT_OBJECT	2		//we hit an CObject - which one?  no way to tell...
#define HIT_BAD_P0	3		//start point is not in specified CSegment

//flags for fvi query
#define FQ_CHECK_OBJS		1		//check against objects?
#define FQ_TRANSWALL			2		//go through transparent walls
#define FQ_TRANSPOINT		4		//go through trans CWall if hit point is transparent
#define FQ_GET_SEGLIST		8		//build a list of segments
#define FQ_IGNORE_POWERUPS	16		//ignore powerups
#define FQ_VISIBLE_OBJS		32
#define FQ_ANY_OBJECT		64
#define FQ_CHECK_PLAYER		128
#define FQ_VISIBILITY		256

//intersection types
#define IT_ERROR	-1
#define IT_NONE	0       //doesn't touch face at all
#define IT_FACE	1       //touches face
#define IT_EDGE	2       //touches edge of face
#define IT_POINT  3       //touches vertex

#define MAX_FVI_SEGS 200

//	-----------------------------------------------------------------------------

class CFixVector2 {
	public:
		fix x, y;

	inline fix Cross (CFixVector2& other) { return fix (double (x) / 65536.0 * double (other.y) - double (y) / 65536.0 * double (other.x)); }
	};

//	-----------------------------------------------------------------------------

class CFloatVector2 {
	public:
		float x, y;

	CFloatVector2 (fix x = 0, fix y = 0) : x (X2F (x)), y (X2F (y)) {}
	inline float Cross (CFloatVector2& other) { return x * other.y - y * other.x; }
	inline CFloatVector2& operator-= (CFloatVector2& other) { 
		x -= other.x;
		y -= other.y;
		return *this;
		}
	};

//	-----------------------------------------------------------------------------

class CHitData {
	public:
		int 			nType;						//what sort of intersection
		short			nSegment;
		short			nObject;
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
		short			nAltSegment;
		short 		nSide;						//if hit wall, which side
		short			nFace;
		short 		nSideSegment;				//what segment the hit side is in
		int			nNormals;
		int			nNestCount;

	public:
		CHitInfo () : nAltSegment (-1), nSide (0), nFace (0), nSideSegment (-1), nNormals (0), nNestCount (0) {}
	};

//	-----------------------------------------------------------------------------
//this data structure gets filled in by FindHitpoint()

class CHitResult : public CHitInfo {
	public:
		short 	nSegments;					//how many segs we went through
		short 	segList [MAX_FVI_SEGS];	//list of segs vector went through

	public:
		CHitResult () : nSegments (0) {}
	};

//	-----------------------------------------------------------------------------

//this data contains the parms to fvi()
class CHitQuery {
	public:
		int			flags;
		CFixVector* p0, * p1;
		short			nSegment;
		short			nObject;
		fix			radP0, radP1;
		int			bIgnoreObjFlag;

	public:
		CHitQuery () 
			: flags (0), p0 (NULL), p1 (NULL), nSegment (-1), nObject (-1), radP0 (0), radP1 (0), bIgnoreObjFlag (0)
			{}
		CHitQuery (int flags, CFixVector* p0, CFixVector* p1, short nSegment, short nObject = -1, fix radP0 = 0, fix radP1 = 0, int bIgnoreObjFlag = 0)
			: flags (flags), p0 (p0), p1 (p1), nSegment (nSegment), nObject (nObject), radP0 (radP0), radP1 (radP1), bIgnoreObjFlag (bIgnoreObjFlag)
			{}
	};

//------------------------------------------------------------------------------

typedef struct tSpeedBoostData {
	int						bBoosted;
	CFixVector				vVel;
	CFixVector				vMinVel;
	CFixVector				vMaxVel;
	CFixVector				vSrc;
	CFixVector				vDest;
} tSpeedBoostData;

//	-----------------------------------------------------------------------------

class CPhysSimData {
	public:
		short					nObject;
		CObject*				objP;
		short					nStartSeg;
		CFixVector			vStartPos;
		short					nOldSeg;
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
		int					bInitialize;
		int					bUpdateOffset;
		int					bUseHitbox;
		int					bGetPhysSegs;
		int					bSpeedBoost;
		int					bScaleSpeed;
		int					bStopped;
		int					bBounced;
		int					bIgnoreObjFlag;
		int					nTries;

		explicit CPhysSimData (short nObject = -1) : nObject (nObject), bUpdateOffset (1), bStopped (0), bBounced (0), bIgnoreObjFlag (0), nTries (0) { Setup (); }
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
int FindHitpoint (CHitQuery& hitQuery, CHitResult& hitResult);

//finds the uv coords of the given point on the given seg & CSide
//fills in u & v. if l is non-NULL fills it in also
void FindHitPointUV (fix *u,fix *v,fix *l, CFixVector *pnt,CSegment *seg,int nSide,int facenum);

//Returns true if the CObject is through any walls
int ObjectIntersectsWall (CObject *objP);

int CheckLineToSegFace (CFixVector *newP, CFixVector *p0, CFixVector *p1, 
								short nSegment, short nSide, short iFace, int nv, fix rad);

int FindPlaneLineIntersection (CFixVector& intersection, CFixVector *vPlanePoint, CFixVector *vPlaneNorm,
										 CFixVector *p0, CFixVector *p1, fix rad, bool bCheckOverflow = true);

int CheckLineToLine (fix *t1, fix *t2, CFixVector *p1, CFixVector *v1, CFixVector *p2, CFixVector *v2);

float DistToFace (CFloatVector3 vRef, short nSegment, ubyte nSide, CFloatVector3* vHit = NULL);

fix CheckVectorHitboxCollision (CFixVector& intersection, CFixVector& normal, CFixVector *p0, CFixVector *p1, CFixVector *vRef, CObject *objP, fix rad, short& nModel);

fix CheckHitboxCollision (CFixVector& intersection, CFixVector& normal, CObject *objP1, CObject *objP2, CFixVector *p0, CFixVector *p1, short& nModel);

CSegMasks CheckFaceHitboxCollision (CFixVector& intersection, CFixVector& normal, short nSegment, short nSide, CFixVector* p0, CFixVector* p1, CObject *objP);

ubyte PointIsOutsideFace (CFixVector* refP, CFixVector* vertices, short nVerts);

ubyte PointIsOutsideFace (CFixVector* refP, ushort* nVertIndex, short nVerts);

ubyte PointIsOutsideFace (CFloatVector* refP, ushort* nVertIndex, short nVerts);

uint PointToFaceRelation (CFixVector* refP, CFixVector *vertList, int nVerts, CFixVector* vNormal);

int PointSeesPoint (CFloatVector* p0, CFloatVector* p1, short nStartSeg, short nDestSeg, int nDepth, int nThread);

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

int UseHitbox (CObject *objP);
int UseSphere (CObject *objP);

//	-----------------------------------------------------------------------------

extern int ijTable [3][2];

#endif

