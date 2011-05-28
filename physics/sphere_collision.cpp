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

#define NEW_FVI_STUFF 1

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "rle.h"
#include "segmath.h"
#include "interp.h"
#include "hitbox.h"
#include "network.h"
#include "renderlib.h"
#include "collision_math.h"

int SphereToFaceRelation (CFixVector* refP, fix rad, CFixVector *vertList, int nVerts, CFixVector* vNormal);

//	-----------------------------------------------------------------------------

//given largest component of Normal, return i & j
//if largest component is negative, swap i & j
int ijTable [3][2] = {
	{2, 1},  //pos x biggest
	{0, 2},  //pos y biggest
	{1, 0}	//pos z biggest
	};

//	-----------------------------------------------------------------------------
//see if a point (refP) is inside a triangle using barycentric method

ubyte PointIsInFace (CFixVector* refP, CFixVector vNormal, CFixVector* vertices, short nVerts)
{
CFixVector v0 = vertices [2] - vertices [0];
CFixVector v1 = vertices [1] - vertices [0];
CFixVector v2 = *refP - vertices [0];
fix dot01 = CFixVector::Dot (v0, v1);
fix dot02 = CFixVector::Dot (v0, v2);
fix dot12 = CFixVector::Dot (v1, v2);
fix invDenom = -FixDiv (I2X (1), FixMul (dot01, dot01)); 
fix u = FixMul (dot02 - FixMul (dot01, dot12), invDenom);
fix v = FixMul (dot12 - FixMul (dot01, dot02), invDenom);
// Check if point is in triangle
return int (u < 0) + (int (v < 0) << 1) + (int (u + v > I2X (1)) << 2);
}

//	-----------------------------------------------------------------------------
//see if a point (refP) is inside a triangle using barycentric method

ubyte PointIsInFace (CFixVector* refP, CFixVector vNormal, short* nVertIndex, short nVerts)
{
CFixVector v0 = VERTICES [nVertIndex [2]] - VERTICES [nVertIndex [0]];
CFixVector v1 = VERTICES [nVertIndex [1]] - VERTICES [nVertIndex [0]];
CFixVector v2 = *refP - FVERTICES [nVertIndex [0]];
fix dot00 = CFixVector::Dot (v0, v0);
fix dot11 = CFixVector::Dot (v1, v1);
fix dot01 = CFixVector::Dot (v0, v1);
fix dot02 = CFixVector::Dot (v0, v2);
fix dot12 = CFixVector::Dot (v1, v2);
fix invDenom = FixDiv (I2X (1), FixMul (dot00, dot11) - FixMul (dot01, dot01)); 
fix u = FixMul (FixMul (dot11, dot02) - FixMul (dot01, dot12), invDenom);
fix v = FixMul (FixMul (dot00, dot12) - FixMul (dot01, dot02), invDenom);
// Check if point is in triangle
return int (u < 0) + (int (v < 0) << 1) + (int (u + v > I2X (1)) << 2);
}

//	-----------------------------------------------------------------------------
//see if a point (refP) is inside a triangle using barycentric method

ubyte PointIsInFace (CFloatVector* refP, CFloatVector vNormal, short* nVertIndex, short nVerts)
{
#if 1
CFloatVector v0 = FVERTICES [nVertIndex [2]] - FVERTICES [nVertIndex [0]];
CFloatVector v1 = FVERTICES [nVertIndex [1]] - FVERTICES [nVertIndex [0]];
CFloatVector v2 = *refP - FVERTICES [nVertIndex [0]];
float dot00 = CFloatVector::Dot (v0, v0);
float dot11 = CFloatVector::Dot (v1, v1);
float dot01 = CFloatVector::Dot (v0, v1);
float dot02 = CFloatVector::Dot (v0, v2);
float dot12 = CFloatVector::Dot (v1, v2);
float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
// Check if point is in triangle
CFloatVector p = FVERTICES [nVertIndex [0]];
p += v0 * u;
p += v1 * v;
p -= *refP;
float l = p.Mag ();
return (int (u < 0.0f)) + ((int (v < 0.0f)) << 1) + ((int (u + v > 1.0f)) << 2);
#else
	CFloatVector	t, *v0, *v1;
	int 				i, j, nEdge, biggest;
	float				check_i, check_j;
	fvec2d 			vEdge, vCheck;

//now do 2d check to see if refP is in side
//project polygon onto plane by finding largest component of Normal
t.v.coord.x = float (fabs (vNormal.v.coord.x));
t.v.coord.y = float (fabs (vNormal.v.coord.y));
t.v.coord.z = float (fabs (vNormal.v.coord.z));
if (t.v.coord.x > t.v.coord.y) {
	if (t.v.coord.x > t.v.coord.z)
		biggest = 0;
	else
		biggest = 2;
	}
else {
	if (t.v.coord.y > t.v.coord.z)
		biggest = 1;
	else
		biggest = 2;
	}
if (vNormal.v.vec [biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = refP->v.vec [i];
check_j = refP->v.vec [j];
v1 = FVERTICES + nVertIndex [0];
for (nEdge = 1; nEdge <= nVerts; nEdge++) {
	v0 = v1; //FVERTICES + nVertIndex [nEdge];
	v1 = FVERTICES + nVertIndex [nEdge % nVerts];
	vEdge.i = v1->v.vec [i] - v0->v.vec [i];
	vEdge.j = v1->v.vec [j] - v0->v.vec [j];
	vCheck.i = check_i - v0->v.vec [i];
	vCheck.j = check_j - v0->v.vec [j];
	if (vCheck.i * vEdge.j - vCheck.j * vEdge.i < -0.005f)
		return false;
	}
return true;
#endif
}

//	-----------------------------------------------------------------------------

float DistToFace (CFloatVector3& vHit, CFloatVector3 refP, short nSegment, ubyte nSide)
{
	CSide*			sideP = SEGMENTS [nSegment].Side (nSide);
	CFloatVector	h, n, v;
	short*			nVerts = sideP->m_vertices;
	float				dist, minDist = 1e30f;
	int				i, j;

v.Assign (refP);

// compute intersection of perpendicular through refP with the plane spanned up by the face
for (i = j = 0; i < sideP->m_nFaces; i++, j += 3) {
	n.Assign (sideP->m_normals [i]);
	h = v - FVERTICES [nVerts [j]];
	dist = CFloatVector::Dot (h, n);
	if (minDist > fabs (dist)) {
		h = v - n * dist;
		if (PointIsInFace (&h, n, nVerts + j, (sideP->m_nFaces == 1) ? 4 : 3)) {
			if (fabs (dist) < 0.01)
				vHit = refP;
			else
				vHit.Assign (h);
			minDist = float (fabs (dist));
			}
		}
	}
if (minDist < 1e30f)
	return 0;

nVerts = sideP->m_corners;
minDist = 1e30f;
for (i = 0; i < 4; i++) {
	VmPointLineIntersection (h, FVERTICES [nVerts [i]], FVERTICES [nVerts [(i + 1) % 4]], v, 1);
	dist = CFloatVector::Dist (h, v);
	if (minDist > dist) {
		minDist = dist;
		vHit.Assign (h);
		}
	}
return minDist;
}

//	-----------------------------------------------------------------------------
//find the intersection on the specified plane where the line intersects
//returns true if intersection found, false if line parallel to plane
//intersection is the found intersection on the plane
//vPlanePoint & vPlaneNorm describe the plane
//p0 & p1 are the ends of the line

static inline double DblRound (double v) { return (v < 0.0) ? v - 0.5 : v + 0.5; }

int FindPlaneLineIntersection (CFixVector& intersection, CFixVector *vPlanePoint, CFixVector *vPlaneNorm,
										 CFixVector *p0, CFixVector *p1, fix rad, bool bCheckOverflow)
{
CFixVector u = *p1 - *p0;
fix den = -CFixVector::Dot (*vPlaneNorm, u);
if (!den)
	return 0;
CFixVector w = *p0 - *vPlanePoint;
fix num = CFixVector::Dot (*vPlaneNorm, w) - rad;
//do check for potential overflow
if (bCheckOverflow) {
	if (den > 0) {
		if ((num > den) || ((-num >> 15) >= den)) //frac greater than one
			return 0;
		}
	else {
		if (num < den)
			return 0;
		}
	if (labs (num) / (I2X (1) / 2) >= labs (den))
		return 0;
	u *= FixDiv (num, den);
	}
else {
	double scale = double (num) / double (den);
	if ((scale < 1e-10f) || (scale > 1.0f))
		return 0;
	u.v.coord.x = fix (DblRound (double (u.v.coord.x) * scale));
	u.v.coord.y = fix (DblRound (double (u.v.coord.y) * scale));
	u.v.coord.z = fix (DblRound (double (u.v.coord.z) * scale));
	}
intersection = *p0 + u;
return 1;
}

//	-----------------------------------------------------------------------------

int FindPlaneLineIntersection (CFloatVector& intersection, CFloatVector* vPlane, CFloatVector* vNormal, CFloatVector* p0, CFloatVector* p1)
{
CFloatVector u = *p1;
u -= *p0;
float d = -CFloatVector::Dot (*vNormal, u);
if (d < 1e-10f) // ~ parallel
	return 0;
CFloatVector w = *p0;
w -= *vPlane;
float n = CFloatVector::Dot (*vNormal, w);
float s = n / d;
if ((s < 0.0f) || (s > 1.0f))
	return 0;
u *= s;
intersection = *p0;
intersection += u;
return 1;
}

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d
uint PointToFaceRelation (CFixVector* refP, CFixVector *vertList, int nVerts, CFixVector* vNormal)
{
#if 0
if (nVerts == 3)
	return uint (PointIsInFace (refP, vNormal, vertList));
#endif

//	CFixVector	vNormal;
	CFixVector	t;
	int			biggest;
	int 			i, j, nEdge;
	uint 			nEdgeMask;
	fix 			check_i, check_j;
	CFixVector	*v0, *v1;
	vec2d 		vEdge, vCheck;
	fix 			d;

//now do 2d check to see if refP is in side
//project polygon onto plane by finding largest component of Normal
t.v.coord.x = labs (vNormal->v.coord.z);
t.v.coord.y = labs (vNormal->v.coord.z);
t.v.coord.z = labs (vNormal->v.coord.z);
if (t.v.coord.x > t.v.coord.y)
	if (t.v.coord.x > t.v.coord.z)
		biggest = 0;
	else
		biggest = 2;
else if (t.v.coord.y > t.v.coord.z)
	biggest = 1;
else
	biggest = 2;
if (vNormal->v.vec [biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = refP->v.vec [i];
check_j = refP->v.vec [j];
for (nEdge = nEdgeMask = 0; nEdge < nVerts; nEdge++) {
	v0 = vertList + nEdge;
	v1 = vertList + ((nEdge + 1) % nVerts);
	vEdge.i = v1->v.vec [i] - v0->v.vec [i];
	vEdge.j = v1->v.vec [j] - v0->v.vec [j];
	vCheck.i = check_i - v0->v.vec [i];
	vCheck.j = check_j - v0->v.vec [j];
	d = FixMul (vCheck.i, vEdge.j) - FixMul (vCheck.j, vEdge.i);
	if (d < 0)              		//we are outside of triangle
		nEdgeMask |= (1 << nEdge);
	}
return nEdgeMask;
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int SphereToFaceRelation (CFixVector* refP, fix rad, CFixVector *vertList, int nVerts, CFixVector* vNormal)
{
	CFixVector	vEdge, vCheck;            //this time, real 3d vectors
	CFixVector	vClosestPoint;
	fix			xEdgeLen, d, dist;
	CFixVector	*v0, *v1;
	int			iType;
	int			nEdge;
	uint			nEdgeMask;

//now do 2d check to see if refP is inside
if (!(nEdgeMask = PointToFaceRelation (refP, vertList, nVerts, vNormal)))
	return IT_FACE;	//we've gone through all the sides, and are inside
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
v0 = vertList + nEdge;
v1 = vertList + ((nEdge + 1) % nVerts);
//check if we are touching an edge or refP
vCheck = *refP - *v0;
xEdgeLen = CFixVector::NormalizedDir (vEdge, *v1, *v0);
//find refP dist from planes of ends of edge
d = CFixVector::Dot (vEdge, vCheck);
if (d + rad < 0)
	return IT_NONE;                  //too far behind start refP
if (d - rad > xEdgeLen)
	return IT_NONE;    //too far part end refP
//find closest refP on edge to check refP
iType = IT_POINT;
if (d < 0)
	vClosestPoint = *v0;
else if (d > xEdgeLen)
	vClosestPoint = *v1;
else {
	iType = IT_EDGE;
	vClosestPoint = *v0 + vEdge * d;
	}
dist = CFixVector::Dist (*refP, vClosestPoint);
if (dist <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in intersection with intersection
//refP on plane, whether or not line intersects CSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a refP field
int CheckLineToFaceRegular (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad,
									 CFixVector *vertList, int nVerts, CFixVector *vNormal)
{
	CFixVector	v1, vHit;
	int			pli, bCheckRad = 0;

//use lowest refP number
if (p1 == p0) {
	if (!rad)
		return IT_NONE;
	v1 = *vNormal * (-rad);
	v1 += *p0;
	bCheckRad = rad;
	rad = 0;
	p1 = &v1;
	}
if (!(pli = FindPlaneLineIntersection (intersection, vertList, vNormal, p0, p1, rad)))
	return IT_NONE;
vHit = intersection;
//if rad != 0, project the refP down onto the plane of the polygon
if (rad)
	vHit += *vNormal * (-rad);
if ((pli = SphereToFaceRelation (&vHit, rad, vertList, nVerts, vNormal)))
	return pli;
if (bCheckRad) {
	int			i, d;
	CFixVector	*a, *b;

	b = vertList;
	for (i = 1; i <= nVerts; i++) {
		a = b;
		b = vertList + (i % nVerts);
		d = VmLinePointDist (*a, *b, *p0);
		if (d < bCheckRad)
			return IT_POINT;
		}
	}
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//computes the parameters of closest approach of two lines
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
int CheckLineToLine (fix *t1, fix *t2, CFixVector *p1, CFixVector *v1, CFixVector *p2, CFixVector *v2)
{
	CFixMatrix det;
	fix d, crossMag2;		//mag squared Cross product

det.m.dir.r = *p2 - *p1;
det.m.dir.f = CFixVector::Cross (*v1, *v2);
if (!(crossMag2 = CFixVector::Dot (det.m.dir.f, det.m.dir.f)))
	return 0;			//lines are parallel
det.m.dir.u = *v2;
d = det.Det();
*t1 = FixDiv (d, crossMag2);
det.m.dir.u = *v1;
d = det.Det();
*t2 = FixDiv (d, crossMag2);
return 1;		//found refP
}

#ifdef NEW_FVI_STUFF
int bSimpleFVI = 0;
#else
#define bSimpleFVI 1
#endif

//	-----------------------------------------------------------------------------
//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0, p1
//returns dist if intersects, and fills in intP
//else returns 0
int CheckVectorToSphere1 (CFixVector& intersection, CFixVector *p0, CFixVector *p1, CFixVector *vSpherePos, fix xSphereRad)
{
	CFixVector	d, dn, w, vClosestPoint;
	fix			mag_d, dist, wDist, intDist;

//this routine could be optimized if it's taking too much time!

d = *p1 - *p0;
w = *vSpherePos - *p0;
dn = d; 
mag_d = CFixVector::Normalize (dn);
if (mag_d == 0) {
	intDist = w.Mag ();
	intersection = *p0;
	return ((xSphereRad < 0) || (intDist < xSphereRad)) ? intDist : 0;
	}
wDist = CFixVector::Dot (dn, w);
if (wDist < 0)
	return 0;	//moving away from CObject
if (wDist > mag_d + xSphereRad)
	return 0;	//cannot hit
vClosestPoint = *p0 + dn * wDist;
dist = CFixVector::Dist (vClosestPoint, *vSpherePos);
if  (dist < xSphereRad) {
	fix dist2 = FixMul (dist, dist);
	fix radius2 = FixMul (xSphereRad, xSphereRad);
	fix nShorten = FixSqrt (radius2 - dist2);
	intDist = wDist - nShorten;
	if ((intDist > mag_d) || (intDist < 0)) {
		//paste one or the other end of vector, which means we're inside
		intersection = *p0;		//don't move at all
		return 1;
		}
	intersection = *p0 + dn * intDist;         //calc intersection refP
	return intDist;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//determine if a vector intersects with an CObject
//if no intersects, returns 0, else fills in intP and returns dist
fix CheckVectorToObject (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad,
								 CObject *thisObjP, CObject *otherObjP, bool bCheckVisibility)
{
	fix			size, dist;
	CFixVector	vHit, v0, v1, vn, vPos;
	int			bThisPoly, bOtherPoly;
	short			nModel = -1;

if (rad < 0)
	size = 0;
else {
	size = thisObjP->info.xSize;
	if ((thisObjP->info.nType == OBJ_ROBOT) && ROBOTINFO (thisObjP->info.nId).attackType)
		size = 3 * size / 4;
	//if obj is CPlayerData, and bumping into other CPlayerData or a weapon of another coop CPlayerData, reduce radius
	if ((thisObjP->info.nType == OBJ_PLAYER) &&
		 ((otherObjP->info.nType == OBJ_PLAYER) ||
 		 (IsCoopGame && (otherObjP->info.nType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parent.nType == OBJ_PLAYER))))
		size /= 2;
	}

	// check hit sphere collisions
bOtherPoly = UseHitbox (otherObjP);
#if 1
if ((bThisPoly = UseHitbox (thisObjP)))
	PolyObjPos (thisObjP, &vPos);
else
#endif
vPos = thisObjP->info.position.vPos;
if (CollisionModel () &&
	 !(UseSphere (thisObjP) || UseSphere (otherObjP)) &&
	 (bThisPoly || bOtherPoly)) {
	VmPointLineIntersection (vHit, *p0, *p1, vPos, 0);
#if 1 //!DBG
	dist = VmLinePointDist (*p0, *p1, OBJPOS (thisObjP)->vPos);
	if (dist > thisObjP->info.xSize + otherObjP->info.xSize + I2X (2))
		return 0;
#endif
	// check hitbox collisions for all polygonal objects
	if (bThisPoly && bOtherPoly) {
		if (!(dist = CheckHitboxToHitbox (vHit, otherObjP, thisObjP, p0, p1, nModel))) {
			if (!CFixVector::Dist (*p0, *p1))
				return 0;
			dist = CheckVectorToHitbox (vHit, p0, p1, NULL, thisObjP, 0, nModel);
			if ((dist == 0x7fffffff) || (dist > thisObjP->info.xSize))
				return 0;
			}
		CheckHitboxToHitbox (vHit, otherObjP, thisObjP, p0, p1, nModel);
		VmPointLineIntersection (vHit, *p0, *p1, vHit, 1);
		}
	else {
		if (bThisPoly) {
			// *thisObjP (stationary) has hitboxes, *otherObjP (moving) a hit sphere. To detect whether the sphere
			// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			if (0x7fffffff == (dist = CheckVectorToHitbox (vHit, p0, p1, NULL, thisObjP, otherObjP->info.xSize, nModel)))
				return 0;
			}
		else {
			// *otherObjP (moving) has hitboxes, *thisObjP (stationary) a hit sphere. To detect whether the sphere
			// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			v0 = thisObjP->info.position.vPos;
			vn = otherObjP->info.position.vPos - v0;
			CFixVector::Normalize (vn);
			v1 = v0 + vn * thisObjP->info.xSize;
			if (0x7fffffff == (dist = CheckVectorToHitbox (vHit, &v0, &v0, p1, otherObjP, thisObjP->info.xSize, nModel)))
				return 0;
			}
		}
	}
else {
	if (!(dist = CheckVectorToSphere1 (vHit, p0, p1, &vPos, size + rad)))
		return 0;
	nModel = 0;
	}
intersection = vHit;
#if 0 //DBG
CreatePowerup (POW_SHIELD_BOOST, thisObjP->Index (), otherObjP->info.nSegment, vHit, 1, 1);
#endif
if (!bCheckVisibility && (otherObjP->info.nType != OBJ_POWERUP) 
#if 0 // well, the Guidebot should actually cause critical damage when ramming the player, shouldn't it?
	 && ((otherObjP->info.nType != OBJ_ROBOT) || !ROBOTINFO (otherObjP->info.nId).companion)
#endif
	 ) {
	vHit = thisObjP->RegisterHit (vHit, nModel);
	//vHit = otherObjP->RegisterHit (vHit, nModel);
	}
return dist;
}

//	-----------------------------------------------------------------------------

int ObjectInList (short nObject, short *objListP)
{
	short t;

while (((t = *objListP) != -1) && (t != nObject))
	objListP++;
return (t == nObject);

}

//	-----------------------------------------------------------------------------

int ComputeObjectHitpoint (short nThisObject, short nStartSeg, CFixVector *p0, CFixVector *p1, fix radP0, fix radP1, int flags,
									short* ignoreObjList, CFixVector& vClosestHitPoint, int &nHitType)
{
	CObject		* thisObjP = (nThisObject < 0) ? NULL : OBJECTS + nThisObject,
			 		* otherObjP;
	int			nThisType = (nThisObject < 0) ? -1 : OBJECTS [nThisObject].info.nType;
	int			nObjSegList [7], nObjSegs = 1, i;
	fix			dMin = 0x7FFFFFFF;
	bool			bCheckVisibility = ((flags & FQ_VISIBILITY) != 0);
	CFixVector	vHitPoint;

nObjSegList [0] = nStartSeg;
#	if DBG
if ((thisObjP->info.nType == OBJ_WEAPON) && (thisObjP->info.nSegment == gameData.objs.consoleP->info.nSegment))
	flags = flags;
if (nStartSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#	endif
#if 1
CSegment* segP = SEGMENTS + nStartSeg;
for (int iObjSeg = 0; iObjSeg < 6; iObjSeg++) {
	short nSegment = segP->m_children [iObjSeg];
	if (0 > nSegment)
		continue;
	for (i = 0; i < gameData.collisions.nSegsVisited && (nSegment != gameData.collisions.segsVisited [i]); i++)
		;
	if (i == gameData.collisions.nSegsVisited)
		nObjSegList [nObjSegs++] = nSegment;
	}
#endif
for (int iObjSeg = 0; iObjSeg < nObjSegs; iObjSeg++) {
	short nSegment = nObjSegList [iObjSeg];
	segP = SEGMENTS + nSegment;
#if DBG
restart:
#endif
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
	short nSegObjs = gameData.objs.nObjects;
	short nFirstObj = SEGMENTS [nSegment].m_objects;
	for (short nObject = nFirstObj; nObject != -1; nObject = otherObjP->info.nNextInSeg, nSegObjs--) {
		otherObjP = OBJECTS + nObject;
#if DBG
		if (nObject == nDbgObj)
			nDbgObj = nDbgObj;
		if ((nSegObjs < 0) || !CheckSegObjList (otherObjP, nObject, nFirstObj)) {
			RelinkAllObjsToSegs ();
			goto restart;
			}
#endif
		int nOtherType = otherObjP->info.nType;
		if ((nOtherType == OBJ_POWERUP) && (gameStates.app.bGameSuspended & SUSP_POWERUPS))
			continue;
		if ((flags & FQ_CHECK_PLAYER) && (nOtherType != OBJ_PLAYER))
			continue;
		if (otherObjP->info.nFlags & OF_SHOULD_BE_DEAD)
			continue;
		if (nThisObject == nObject)
			continue;
		if (ignoreObjList && ObjectInList (nObject, ignoreObjList))
			continue;
		if (LasersAreRelated (nObject, nThisObject))
			continue;
		if (nThisObject > -1) {
			if ((gameData.objs.collisionResult [nThisType][nOtherType] == RESULT_NOTHING) &&
				 (gameData.objs.collisionResult [nOtherType][nThisType] == RESULT_NOTHING))
				continue;
			}
		int nFudgedRad = radP1;

		//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
		if ((nOtherType == OBJ_POWERUP) && (flags & FQ_IGNORE_POWERUPS))
			continue;
		//	If this is a robot:robot collision, only do it if both of them have attackType != 0 (eg, green guy)
		if (nThisType == OBJ_ROBOT) {
			if ((flags & FQ_ANY_OBJECT) ? (nOtherType != OBJ_ROBOT) : (nOtherType == OBJ_ROBOT))
				continue;
			if (ROBOTINFO (thisObjP->info.nId).attackType)
				nFudgedRad = (radP1 * 3) / 4;
			}
		//if obj is CPlayerData, and bumping into other CPlayerData or a weapon of another coop CPlayerData, reduce radius
		if (nThisType == OBJ_PLAYER) {
			if (nOtherType == OBJ_ROBOT) {
				if ((flags & FQ_VISIBLE_OBJS) && otherObjP->Cloaked ())
					continue;
				}
			else if (nOtherType == OBJ_PLAYER) {
				if (IsCoopGame && (nOtherType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parent.nType == OBJ_PLAYER))
					nFudgedRad = radP1 / 2;
				}
			}
#if DBG
		if (nObject == nDbgObj)
			nDbgObj = nDbgObj;
#endif
		fix d = CheckVectorToObject (vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP, bCheckVisibility);
		if (d && (d < dMin)) {
#if DBG
			CheckVectorToObject (vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP, bCheckVisibility);
#endif
			gameData.collisions.hitData.nObject = nObject;
			Assert(gameData.collisions.hitData.nObject != -1);
			dMin = d;
			vClosestHitPoint = vHitPoint;
			nHitType = HIT_OBJECT;
#if DBG
			CheckVectorToObject (vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP, bCheckVisibility);
#endif
			if (flags & FQ_ANY_OBJECT)
				return dMin;
			}
		}
	}
return dMin;
}

//	-----------------------------------------------------------------------------

#define FVI_NEWCODE 2

int ComputeHitpoint (CFixVector *vIntP, short *nHitSegP, CFixVector *p0, short nStartSeg, CFixVector *p1,
							fix radP0, fix radP1, short nThisObject, short *ignoreObjList, int flags, short *segList,
							short *nSegments, int nEntrySeg)
{
	CSegment		*segP;				//the CSegment we're looking at
	int			startMask, endMask, centerMask;	//mask of faces
	CSegMasks	masks;
	CFixVector	vHitPoint, vClosestHitPoint; 	//where we hit
	fix			d, dMin = 0x7fffffff;					//distance to hit refP
	int			nHitType = HIT_NONE;							//what sort of hit
	int			nHitSegment = -1;
	int			nHitNoneSegment = -1;
	int			nHitNoneSegs = 0;
	int			hitNoneSegList [MAX_FVI_SEGS];
	int			nCurNestLevel = gameData.collisions.hitData.nNestCount;
	int			nChildSide;
#if FVI_NEWCODE
	int			nFaces;
#if 1
	int			nFaceHitType;
#endif
	int			widResult;
#endif
	bool			bCheckVisibility = ((flags & FQ_VISIBILITY) != 0);

vClosestHitPoint.SetZero ();
//PrintLog ("Entry ComputeHitpoint\n");
if (flags & FQ_GET_SEGLIST)
	*segList = nStartSeg;
*nSegments = 1;
gameData.collisions.hitData.nNestCount++;
//first, see if vector hit any objects in this CSegment
#if 1
if (flags & FQ_CHECK_OBJS) {
	//PrintLog ("   checking objects...");
	dMin = ComputeObjectHitpoint (nThisObject, nStartSeg, p0, p1, radP0, radP1, flags, ignoreObjList, vClosestHitPoint, nHitType);
	}
#endif

segP = SEGMENTS + nStartSeg;
if ((nThisObject > -1) && (gameData.objs.collisionResult [OBJECTS [nThisObject].info.nType][OBJ_WALL] == RESULT_NOTHING))
	radP1 = 0;		//HACK - ignore when edges hit walls
//now, check segment walls
#if DBG
if (nStartSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
startMask = SEGMENTS [nStartSeg].Masks (*p0, radP0).m_face;
masks = SEGMENTS [nStartSeg].Masks (*p1, radP1);    //on back of which faces?
if (!(centerMask = masks.m_center))
	nHitNoneSegment = nStartSeg;
if ((endMask = masks.m_face)) { //on the back of at least one face
	short nSide, iFace, bit;

	//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (endMask >= bit); nSide++) {
		nChildSide = segP->m_children [nSide];
#if 0
		if (bCheckVisibility && (0 > nChildSide))	// poking through a wall into the void around the level?
			continue;
#endif
		nFaces = segP->Side (nSide)->m_nFaces;
		for (iFace = 0; iFace < 2; iFace++, bit <<= 1) {
			if (nChildSide == nEntrySeg)	//must be executed here to have bit shifted
				continue;		//don't go back through entry nSide
			if (!(endMask & bit))	//on the back of this face?
				continue;
			if (iFace >= nFaces)
				continue;
			//did we go through this wall/door?
			nFaceHitType = (startMask & bit)	//start was also though.  Do extra check
								? segP->CheckLineToFaceSpecial (vHitPoint, p0, p1, radP1, nSide, iFace)
								: segP->CheckLineToFaceRegular (vHitPoint, p0, p1, radP1, nSide, iFace);
#if 1
			if (bCheckVisibility && !nFaceHitType)
					continue;
#endif
#if DBG
			if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == nSide)))
				nDbgSeg = nDbgSeg;
#endif
			widResult = segP->IsDoorWay (nSide, (nThisObject < 0) ? NULL : OBJECTS + nThisObject);
			if (((widResult & WID_FLY_FLAG) ||
				 (((widResult & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)) == (WID_RENDER_FLAG | WID_RENDPAST_FLAG)) &&
				  ((flags & FQ_TRANSWALL) || ((flags & FQ_TRANSPOINT) && segP->CheckForTranspPixel (vHitPoint, nSide, iFace))))) ||
				 // check whether a cheat code allowing passing through walls is enabled. If so, allow player to pass through blocked segments
			    (!(widResult & WID_FLY_FLAG) && (nChildSide >= 0) && (gameStates.app.cheats.bPhysics == 0xBADA55) && (nThisObject == LOCALPLAYER.nObject) &&
				  (SEGMENTS [nChildSide].HasBlockedProp () ||
				   (gameData.objs.speedBoost [nThisObject].bBoosted &&
				   ((SEGMENTS [nStartSeg].m_function != SEGMENT_FUNC_SPEEDBOOST) || (SEGMENTS [nChildSide].m_function == SEGMENT_FUNC_SPEEDBOOST))))))
			{

				int			i, nNewSeg, subHitType;
				short			subHitSeg, nSaveHitObj = gameData.collisions.hitData.nObject;
				CFixVector	subHitPoint, vSaveWallNorm = gameData.collisions.hitData.vNormal;

				//do the check recursively on the next CSegment.p.
				nNewSeg = segP->m_children [nSide];
#if DBG
				if (nNewSeg == nDbgSeg)
					nDbgSeg = nDbgSeg;
#endif
				for (i = 0; i < gameData.collisions.nSegsVisited && (nNewSeg != gameData.collisions.segsVisited [i]); i++)
					;
				if (i == gameData.collisions.nSegsVisited) {                //haven't visited here yet
					short tempSegList [MAX_FVI_SEGS], nTempSegs;
					if (gameData.collisions.nSegsVisited >= MAX_SEGS_VISITED)
						goto fviSegsDone;		//we've looked a long time, so give up
					gameData.collisions.segsVisited [gameData.collisions.nSegsVisited++] = nNewSeg;
					subHitType = ComputeHitpoint (&subHitPoint, &subHitSeg, p0, (short) nNewSeg,
															p1, radP0, radP1, nThisObject, ignoreObjList, flags,
															tempSegList, &nTempSegs, nStartSeg);
					if (subHitType != HIT_NONE) {
						d = CFixVector::Dist (subHitPoint, *p0);
						if (d < dMin) {
							dMin = d;
							vClosestHitPoint = subHitPoint;
							nHitType = subHitType;
							if (subHitSeg != -1)
								nHitSegment = subHitSeg;
							//copy segList
							if (flags & FQ_GET_SEGLIST) {
#if FVI_NEWCODE != 2
								int i;
								for (i = 0; (i < nTempSegs) && (*nSegments < MAX_FVI_SEGS - 1); i++)
									segList [(*nSegments)++] = tempSegList [i];
#else
								int i = MAX_FVI_SEGS - 1 - *nSegments;
								if (i > nTempSegs)
									i = nTempSegs;
								//PrintLog ("   segList <- tempSegList ...");
								memcpy (segList + *nSegments, tempSegList, i * sizeof (*segList));
								//PrintLog ("done\n");
								*nSegments += i;
#endif
								}
							Assert (*nSegments < MAX_FVI_SEGS);
							}
						else {
							gameData.collisions.hitData.vNormal = vSaveWallNorm;     //global could be trashed
							gameData.collisions.hitData.nObject = nSaveHitObj;
 							}
						}
					else {
						gameData.collisions.hitData.vNormal = vSaveWallNorm;     //global could be trashed
						if (subHitSeg != -1)
							nHitNoneSegment = subHitSeg;
						//copy segList
						if (flags & FQ_GET_SEGLIST) {
#if FVI_NEWCODE != 2
							int i;
							for (i = 0; (i < nTempSegs) && (i < MAX_FVI_SEGS - 1); i++)
								hitNoneSegList [i] = tempSegList [i];
#else
							int i = MAX_FVI_SEGS - 1;
							if (i > nTempSegs)
								i = nTempSegs;
							//PrintLog ("   hitNoneSegList <- tempSegList ...");
							memcpy (hitNoneSegList, tempSegList, i * sizeof (*hitNoneSegList));
							//PrintLog ("done\n");
#endif
							}
						nHitNoneSegs = nTempSegs;
						}
					}
				}
			else {//a wall
				if (nFaceHitType) {
					//is this the closest hit?
					d = CFixVector::Dist (vHitPoint, *p0);
					if (d < dMin) {
						dMin = d;
						vClosestHitPoint = vHitPoint;
						nHitType = HIT_WALL;
						gameData.collisions.hitData.vNormal = segP->m_sides [nSide].m_normals [iFace];
						gameData.collisions.hitData.nNormals = 1;
						if (!SEGMENTS [nStartSeg].Masks (vHitPoint, radP1).m_center)
							nHitSegment = nStartSeg;             //hit in this CSegment
						else
							gameData.collisions.hitData.nSegment2 = nStartSeg;
						gameData.collisions.hitData.nSegment = nHitSegment;
						gameData.collisions.hitData.nSide = nSide;
						gameData.collisions.hitData.nFace = iFace;
						gameData.collisions.hitData.nSideSegment = nStartSeg;
						}
					}
				}
			}
		}
	}
fviSegsDone:
	;

if (nHitType == HIT_NONE) {     //didn't hit anything, return end refP
	int i;

	*vIntP = *p1;
	*nHitSegP = nHitNoneSegment;
	if (nHitNoneSegment != -1) {			//(centerMask == 0)
		if (flags & FQ_GET_SEGLIST) {
#if FVI_NEWCODE != 2
			for (i = 0; (i < nHitNoneSegs) && (*nSegments < MAX_FVI_SEGS - 1);)
				segList [(*nSegments)++] = hitNoneSegList [i++];
#else
			i = MAX_FVI_SEGS - 1 - *nSegments;
			if (i > nHitNoneSegs)
				i = nHitNoneSegs;
			//PrintLog ("   segList <- hitNoneSegList ...");
			memcpy (segList + *nSegments, hitNoneSegList, i * sizeof (*segList));
			//PrintLog ("done\n");
			*nSegments += i;
#endif
			}
		}
	else
		if (nCurNestLevel != 0)
			*nSegments = 0;
	}
else {
	*vIntP = vClosestHitPoint;
	if (nHitSegment == -1)
		if (gameData.collisions.hitData.nSegment2 != -1)
			*nHitSegP = gameData.collisions.hitData.nSegment2;
		else
			*nHitSegP = nHitNoneSegment;
	else
		*nHitSegP = nHitSegment;
	}
Assert(!(nHitType==HIT_OBJECT && gameData.collisions.hitData.nObject==-1));
//PrintLog ("Exit ComputeHitpoint\n");
return nHitType;
}

//	-----------------------------------------------------------------------------

//Find out if a vector intersects with anything.
//Fills in hitData, an CHitData structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisObjNum 		used to prevent an CObject with colliding with itself
//  ingore_obj			ignore collisions with this CObject
//  check_objFlag	determines whether collisions with OBJECTS are checked
//Returns the hitData->nHitType
int FindHitpoint (CHitQuery *fq, CHitData *hitData)
{
	int			nHitType, nNewHitType;
	short			nHitSegment, nHitSegment2;
	CFixVector	vHitPoint;
	int			i, nHitboxes = extraGameInfo [IsMultiGame].nHitboxes;
	CSegMasks	masks;

Assert(fq->ignoreObjList != reinterpret_cast<short*> (-1));
gameData.collisions.hitData.vNormal.SetZero ();
gameData.collisions.hitData.nNormals = 0;
Assert((fq->nSegment <= gameData.segs.nLastSegment) && (fq->nSegment >= 0));
if ((fq->nSegment > gameData.segs.nLastSegment) || (fq->nSegment < 0)) {
	fq->nSegment = FindSegByPos (*fq->p0, -1, 0, 0);
	if (fq->nSegment < 0) {
		hitData->hit.nType = HIT_BAD_P0;
		hitData->hit.vPoint = *fq->p0;
		hitData->hit.nSegment = fq->nSegment;
		hitData->hit.nSide = 0;
		hitData->hit.nObject = 0;
		hitData->hit.nSideSegment = -1;
		hitData->nSegments = 0;
		return hitData->hit.nType;
		}
	}

gameData.collisions.hitData.nSegment = -1;
gameData.collisions.hitData.nSide = -1;
gameData.collisions.hitData.nObject = -1;

//check to make sure start refP is in seg its supposed to be in
//Assert(check_point_in_seg(p0, startseg, 0).m_center==0);	//start refP not in seg

// gameData.objs.viewerP is not in CSegment as claimed, so say there is no hit.
masks = SEGMENTS [fq->nSegment].Masks (*fq->p0, 0);
if (masks.m_center) {
	hitData->hit.nType = HIT_BAD_P0;
	hitData->hit.vPoint = *fq->p0;
	hitData->hit.nSegment = fq->nSegment;
	hitData->hit.nSide = 0;
	hitData->hit.nObject = 0;
	hitData->hit.nSideSegment = -1;
	hitData->nSegments = 0;
	return hitData->hit.nType;
	}
gameData.collisions.segsVisited [0] = fq->nSegment;
gameData.collisions.nSegsVisited = 1;
gameData.collisions.hitData.nNestCount = 0;
nHitSegment2 = gameData.collisions.hitData.nSegment2 = -1;
if (fq->flags & FQ_VISIBILITY)
	extraGameInfo [IsMultiGame].nHitboxes = 0;
nHitType = ComputeHitpoint (&vHitPoint, &nHitSegment2, fq->p0, (short) fq->nSegment, fq->p1,
									 fq->radP0, fq->radP1, (short) fq->nObject, fq->ignoreObjList, fq->flags,
									 hitData->segList, &hitData->nSegments, -2);
extraGameInfo [IsMultiGame].nHitboxes = nHitboxes;

if ((nHitSegment2 != -1) && !SEGMENTS [nHitSegment2].Masks (vHitPoint, 0).m_center)
	nHitSegment = nHitSegment2;
else
	nHitSegment = FindSegByPos (vHitPoint, fq->nSegment, 1, 0);

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
if ((nHitType == HIT_WALL) && (nHitSegment == -1))
	if ((gameData.collisions.hitData.nSegment2 != -1) && !SEGMENTS [gameData.collisions.hitData.nSegment2].Masks (vHitPoint, 0).m_center)
		nHitSegment = gameData.collisions.hitData.nSegment2;

if (nHitSegment == -1) {
	//int nNewHitType;
	short nNewHitSeg2 = -1;
	CFixVector vNewHitPoint;

	//because the code that deals with objects with non-zero radius has
	//problems, try using zero radius and see if we hit a wall
	if (fq->flags & FQ_VISIBILITY)
		extraGameInfo [IsMultiGame].nHitboxes = 0;
	nNewHitType = ComputeHitpoint (&vNewHitPoint, &nNewHitSeg2, fq->p0, (short) fq->nSegment, fq->p1, 0, 0,
											 (short) fq->nObject, fq->ignoreObjList, fq->flags, hitData->segList,
											 &hitData->nSegments, -2);
	extraGameInfo [IsMultiGame].nHitboxes = nHitboxes;
	if (nNewHitSeg2 != -1) {
		nHitType = nNewHitType;
		nHitSegment = nNewHitSeg2;
		vHitPoint = vNewHitPoint;
		}
	}

if ((nHitSegment != -1) && (fq->flags & FQ_GET_SEGLIST))
	if ((nHitSegment != hitData->segList [hitData->nSegments - 1]) &&
		 (hitData->nSegments < MAX_FVI_SEGS - 1))
		hitData->segList [hitData->nSegments++] = nHitSegment;

if ((nHitSegment != -1) && (fq->flags & FQ_GET_SEGLIST))
	for (i = 0; i < (hitData->nSegments) && i < (MAX_FVI_SEGS - 1); i++)
		if (hitData->segList [i] == nHitSegment) {
			hitData->nSegments = i + 1;
			break;
		}
Assert ((nHitType != HIT_OBJECT) || (gameData.collisions.hitData.nObject != -1));
hitData->hit = gameData.collisions.hitData;
hitData->hit.nType = nHitType;
hitData->hit.vPoint = vHitPoint;
hitData->hit.nSegment = nHitSegment;
return nHitType;
}

//	-----------------------------------------------------------------------------
//new function for Mike
//note: gameData.collisions.nSegsVisited must be set to zero before this is called
int SphereIntersectsWall (CFixVector *vPoint, short nSegment, fix rad)
{
	int		faceMask;
	CSegment *segP;

#if DBG
if (nSegment == -1) {
	Error("nSegment == -1 in SphereIntersectsWall()");
	return 0;
	}
#endif
if ((gameData.collisions.nSegsVisited < 0) || (gameData.collisions.nSegsVisited > MAX_SEGS_VISITED))
	gameData.collisions.nSegsVisited = 0;
gameData.collisions.segsVisited [gameData.collisions.nSegsVisited++] = nSegment;
segP = SEGMENTS + nSegment;
faceMask = segP->Masks (*vPoint, rad).m_face;
if (faceMask != 0) {				//on the back of at least one face
	int		nSide, bit, iFace, nChild, i;
	int		nFaceHitType;      //in what way did we hit the face?

//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (faceMask >= bit); nSide++) {
		for (iFace = 0; iFace < 2; iFace++, bit <<= 1) {
			if (faceMask & bit) {            //on the back of this iFace
				//did we go through this CWall/door?
				nFaceHitType = segP->SphereToFaceRelation (*vPoint, rad, nSide, iFace);
				if (nFaceHitType) {            //through this CWall/door
					//if what we have hit is a door, check the adjoining segP
					nChild = segP->m_children [nSide];
					for (i = 0; (i < gameData.collisions.nSegsVisited) && (nChild != gameData.collisions.segsVisited [i]); i++)
						;
					if (i == gameData.collisions.nSegsVisited) {                //haven't visited here yet
						if (!IS_CHILD (nChild))
							return 1;
						if (SphereIntersectsWall (vPoint, nChild, rad))
							return 1;
						}
					}
				}
			}
		}
	}
return 0;
}

//	-----------------------------------------------------------------------------
//Returns true if the CObject is through any walls
int ObjectIntersectsWall (CObject *objP)
{
return SphereIntersectsWall (&objP->info.position.vPos, objP->info.nSegment, objP->info.xSize);
}

//------------------------------------------------------------------------------

int PointSeesPoint (CFloatVector* p0, CFloatVector* p1, short nStartSeg, short nDestSeg, int nDepth)
{
	CSegment*		segP;
	CSide*			sideP;
	CWall*			wallP;
	CFloatVector	intersection, v0, v1;
	float				l0, l1;
	short				nSide, nFace, nChildSeg, nPredSeg = 0x7FFF;

if (!nDepth)
	gameData.render.mine.bVisited.Clear (0, gameData.segs.nSegments);
			
for (;;) {
	gameData.render.mine.bVisited [nStartSeg] = 1;
	segP = &SEGMENTS [nStartSeg];
	sideP = segP->Side (0);
	for (nSide = 0; nSide < 6; nSide++, sideP++) {
		nChildSeg = segP->m_children [nSide];
		if ((nChildSeg >= 0) && gameData.render.mine.bVisited [nChildSeg])
			continue;
		for (nFace = 0; nFace < sideP->m_nFaces; nFace++) {
			CFloatVector* n = sideP->m_fNormals + nFace;
			if (!FindPlaneLineIntersection (intersection, &FVERTICES [sideP->m_vertices [nFace * 3]], n, p0, p1))
				continue;
			v0 = *p0 - intersection;
			v1 = *p1 - intersection;
			l0 = v0.Mag ();
			l1 = v1.Mag ();
			if ((l0 > X2F (PLANE_DIST_TOLERANCE)) && (l1 > X2F (PLANE_DIST_TOLERANCE))) {
				v0 /= l0;
				v1 /= l1;
				if (CFloatVector::Dot (v0, *n) == CFloatVector::Dot (v1, *n))
					continue;
				}
#if DBG
			if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			if (!PointIsInFace (&intersection, sideP->m_fNormals [nFace], sideP->m_vertices + nFace * 3, 5 - sideP->m_nFaces)) {
				if (l1 <= X2F (PLANE_DIST_TOLERANCE))
					return 1;
				break;
				}
			}
		if (nFace == sideP->m_nFaces)
			continue; // line doesn't intersect with this side
		if ((0 > nChildSeg) || ((wallP = sideP->Wall ()) && (wallP->IsDoorWay (NULL, false) & WID_WALL))) {
			if (nStartSeg == nDestSeg)
				return 1;
			continue;
			}
			//return (nStartSeg == nDestSeg); // line intersects a solid wall
		if (PointSeesPoint (p0, p1, nChildSeg, nDestSeg, nDepth + 1))
			return 1;
		}
	return (nStartSeg == nDestSeg); // line doesn't intersect any side of this segment -> p1 must be inside segment
	}
#if DBG
if (!nDepth)
	nDepth = nDepth;
#endif
}

//	-----------------------------------------------------------------------------
//eof
