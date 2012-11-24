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
// see if a point (refP) is inside a triangle using barycentric method
// return 0 if point inside triangle, otherwise sides point is behind as bit code

ubyte PointIsOutsideFace (CFixVector* vRef, CFixVector* vertices, short nVerts)
{
#if 1
CFixVector n1 = CFixVector::Normal (vertices [0], vertices [1], *vRef);
CFixVector n2 = CFixVector::Normal (vertices [1], vertices [2], *vRef);
if (CFixVector::Dot (n1, n2) < PLANE_DIST_TOLERANCE)
	return true;
CFixVector n3 = CFixVector::Normal (vertices [2], vertices [0], *vRef);
if (CFixVector::Dot (n2, n3) < PLANE_DIST_TOLERANCE)
	return true;
return false;
#else
CFixVector v0 = vertices [2] - vertices [0];
CFixVector v1 = vertices [1] - vertices [0];
CFixVector v2 = *vRef - vertices [0];
fix dot01 = CFixVector::Dot (v0, v1);
fix dot02 = CFixVector::Dot (v0, v2);
fix dot12 = CFixVector::Dot (v1, v2);
fix invDenom = -FixDiv (I2X (1), FixMul (dot01, dot01)); 
fix u = FixMul (dot02 - FixMul (dot01, dot12), invDenom);
fix v = FixMul (dot12 - FixMul (dot01, dot02), invDenom);
// Check if point is in triangle
return int (u < 0) + (int (v < 0) << 1) + (int (u + v > I2X (1)) << 2);
#endif
}

//	-----------------------------------------------------------------------------
//see if a point (refP) is inside a triangle using barycentric method

ubyte PointIsOutsideFace (CFixVector* vRef, ushort* nVertIndex, short nVerts)
{
#if 1
CFixVector n1 = CFixVector::Normal (VERTICES [nVertIndex [0]], VERTICES [nVertIndex [1]], *vRef);
CFixVector n2 = CFixVector::Normal (VERTICES [nVertIndex [1]], VERTICES [nVertIndex [2]], *vRef);
if (CFixVector::Dot (n1, n2) < PLANE_DIST_TOLERANCE)
	return true;
CFixVector n3 = CFixVector::Normal (VERTICES [nVertIndex [2]], VERTICES [nVertIndex [0]], *vRef);
if (CFixVector::Dot (n2, n3) < PLANE_DIST_TOLERANCE)
	return true;
return false;
#else
CFixVector v0 = VERTICES [nVertIndex [2]] - VERTICES [nVertIndex [0]];
CFixVector v1 = VERTICES [nVertIndex [1]] - VERTICES [nVertIndex [0]];
CFixVector v2 = *vRef - FVERTICES [nVertIndex [0]];
fix dot00 = CFixVector::Dot (v0, v0);
fix dot11 = CFixVector::Dot (v1, v1);
fix dot01 = CFixVector::Dot (v0, v1);
fix dot02 = CFixVector::Dot (v0, v2);
fix dot12 = CFixVector::Dot (v1, v2);
fix invDenom = FixDiv (I2X (1), FixMul (dot00, dot11) - FixMul (dot01, dot01)); 
fix u = FixMul (FixMul (dot11, dot02) - FixMul (dot01, dot12), invDenom);
fix v = FixMul (FixMul (dot00, dot12) - FixMul (dot01, dot02), invDenom);
// Check if point is in triangle
return int (u < -PLANE_DIST_TOLERANCE) + (int (v < -PLANE_DIST_TOLERANCE) << 1) + (int (u + v > I2X (1) + PLANE_DIST_TOLERANCE) << 2); // compensate for numerical errors
#endif
}

//	-----------------------------------------------------------------------------
//see if a point (refP) is inside a triangle using barycentric method

ubyte PointIsOutsideFace (CFloatVector* vRef, ushort* nVertIndex, short nVerts)
{
#if 1
#	if 1
CFloatVector n1 = CFloatVector::Normal (FVERTICES [nVertIndex [0]], FVERTICES [nVertIndex [1]], *vRef);
CFloatVector n2 = CFloatVector::Normal (FVERTICES [nVertIndex [1]], FVERTICES [nVertIndex [2]], *vRef);
if (CFloatVector::Dot (n1, n2) < 0.999999)
	return true;
CFloatVector n3 = CFloatVector::Normal (FVERTICES [nVertIndex [2]], FVERTICES [nVertIndex [0]], *vRef);
if (CFloatVector::Dot (n2, n3) < 0.999999)
	return true;
return false;
#	else
CFloatVector v0 = FVERTICES [nVertIndex [2]] - FVERTICES [nVertIndex [0]];
CFloatVector v1 = FVERTICES [nVertIndex [1]] - FVERTICES [nVertIndex [0]];
CFloatVector v2 = *vRef - FVERTICES [nVertIndex [0]];
float dot00 = CFloatVector::Dot (v0, v0);
float dot01 = CFloatVector::Dot (v0, v1);
float dot02 = CFloatVector::Dot (v0, v2);
float dot11 = CFloatVector::Dot (v1, v1);
float dot12 = CFloatVector::Dot (v1, v2);
float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
// Check if point is in triangle
#if 0 //DBG
CFloatVector p = FVERTICES [nVertIndex [0]];
p += v0 * u;
p += v1 * v;
p -= *vRef;
float l = p.Mag ();
#endif
return (int (u < -0.001f)) + ((int (v < -0.001f)) << 1) + ((int (u + v > 1.001f)) << 2); // compensate for numerical errors
#	endif
#else
	CFloatVector	t, *v0, *v1;
	int 				i, j, nEdge, biggest;
	float				check_i, check_j;
	CFloatVector2	vEdge, vCheck;

//now do 2d check to see if vRef is in side
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
check_i = vRef->v.vec [i];
check_j = vRef->v.vec [j];
v1 = FVERTICES + nVertIndex [0];
for (nEdge = 1; nEdge <= nVerts; nEdge++) {
	v0 = v1; //FVERTICES + nVertIndex [nEdge];
	v1 = FVERTICES + nVertIndex [nEdge % nVerts];
	vEdge.x = v1->v.vec [i] - v0->v.vec [i];
	vEdge.y = v1->v.vec [j] - v0->v.vec [j];
	vCheck.x = check_i - v0->v.vec [i];
	vCheck.y = check_j - v0->v.vec [j];
	if (vCheck.x * vEdge.y - vCheck.y * vEdge.x < -X2F (PLANE_DIST_TOLERANCE))
		return false;
	}
return true;
#endif
}

//	-----------------------------------------------------------------------------

float DistToFace (CFloatVector3 vRef, short nSegment, ubyte nSide, CFloatVector3* vHit)
{
	CSide*			sideP = SEGMENTS [nSegment].Side (nSide);
	CFloatVector	h, r;
	ushort*			vertices = sideP->m_vertices;
	int				i, j;
	float				d = 0.0f;

r.Assign (vRef);

// compute intersection of vector perpendicular to the plane through vRef with the face's plane(s)
for (i = j = 0; i < sideP->m_nFaces; i++, j += 3) {
	if (!(i && sideP->IsQuad ())) {
		CFloatVector& n = sideP->m_fNormals [i];
		h = r - FVERTICES [vertices [j]];
		d = CFloatVector::Dot (h, n);
		h = n;
		h *= -d;
		h += r;
		}
	if (!PointIsOutsideFace (&h, vertices + j, 3)) {
		d = float (fabs (d));
		if (!vHit)
			return d;
		if (d < 0.001f)
			*vHit = vRef;
		else
			vHit->Assign (h);
		return 0.0f;
		}
	}

vertices = sideP->m_corners;

	CFloatVector	* v0, * v1 = &FVERTICES [vertices [0]];
	float				minDist = 1e10f;
	int				nVertices = sideP->m_nCorners;

for (i = 1; i <= nVertices; i++) {
	v0 = v1;
	v1 = &FVERTICES [vertices [i % nVertices]];
	FindPointLineIntersection (h, *v0, *v1, r, 1);
	d = CFloatVector::Dist (h, r);
	if (minDist > d) {
		minDist = d;
		if (vHit)
			vHit->Assign (h);
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

int FindPlaneLineIntersection (CFixVector& intersection, CFixVector *vPlane, CFixVector *vNormal,
										 CFixVector *p0, CFixVector *p1, fix rad, bool bCheckOverflow)
{
#if 0
	CFloatVector n, u, w;

u.Assign (*p1 - *p0);
n.Assign (*vNormal);
float den = -CFloatVector::Dot (n, u);
if ((den > -1e-6f) && (den < 1e-6f)) {// ~ parallel
	return 0;
	}
w.Assign (*p0 - *vPlane);
float num = CFloatVector::Dot (n, w) - X2F (rad);
float s = num / den;
if (s < 0.0f) {
	if (s < -0.000001f) // compensate small numerical errors
		return 0;
	s = 0.0f;
	}
else if (s > 1.0f) {
	if (s > 1.000001f) // compensate small numerical errors
		return 0;
	s = 1.0f;
	}
u *= s;
intersection.Assign (u);
intersection += *p0;
#else
CFixVector u = *p1 - *p0;
fix den = -CFixVector::Dot (*vNormal, u);
if (!den)
	return 0;
CFixVector w = *p0 - *vPlane;
fix num = CFixVector::Dot (*vNormal, w) - rad;
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
	if ((scale < -0.000001f) || (scale > 1.000001f))
		return 0;
	u.v.coord.x = fix (DblRound (double (u.v.coord.x) * scale));
	u.v.coord.y = fix (DblRound (double (u.v.coord.y) * scale));
	u.v.coord.z = fix (DblRound (double (u.v.coord.z) * scale));
	}
intersection = *p0 + u;
#endif
return 1;
}

//	-----------------------------------------------------------------------------

int FindPlaneLineIntersection (CFloatVector& intersection, CFloatVector* vPlane, CFloatVector* vNormal, CFloatVector* p0, CFloatVector* p1)
{
CFloatVector u = *p1;
u -= *p0;
float d = -CFloatVector::Dot (*vNormal, u);
if ((d > -1e-10f) && (d < 1e-10f)) {// ~ parallel
	return 0;
	}
CFloatVector w = *p0;
w -= *vPlane;
float n = CFloatVector::Dot (*vNormal, w);
float s = n / d;
if (s < 0.0f) {
	if (s < -0.000001f) // compensate small numerical errors
		return 0;
	s = 0.0f;
	}
else if (s > 1.0f) {
	if (s > 1.000001f) // compensate small numerical errors
		return 0;
	s = 1.0f;
	}
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
	return uint (PointIsOutsideFace (refP, vNormal, vertList));
#endif

//	CFixVector	vNormal;
	CFixVector	t;
	int			biggest;
	int 			i, j, nEdge;
	uint 			nEdgeMask;
	fix 			check_i, check_j;
	CFixVector	*v0, *v1;
	CFixVector2	vEdge, vCheck;
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
	vEdge.x = v1->v.vec [i] - v0->v.vec [i];
	vEdge.y = v1->v.vec [j] - v0->v.vec [j];
	vCheck.x = check_i - v0->v.vec [i];
	vCheck.y = check_j - v0->v.vec [j];
	d = FixMul (vCheck.x, vEdge.y) - FixMul (vCheck.y, vEdge.x);
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

//	-----------------------------------------------------------------------------
//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0, p1
//returns dist if intersects, and fills in intP
//else returns 0
int CheckVectorSphereCollision (CFixVector& intersection, CFixVector *p0, CFixVector *p1, CFixVector *vSpherePos, fix xSphereRad)
{
#if 0
FindPointLineIntersection (intersection, *p0, *p1, *vSpherePos, 0);
fix dist = CFixVector::Dist (intersection, *vSpherePos);
if (xSphereRad < 0)
	return dist;
if (dist > xSphereRad)
	return 0;
if (dist < xSphereRad) {
	CFixVector v = *p0;
	v -= intersection;
	fix l1 = CFixVector::Normalize (v);
	float d = X2F (dist);
	float r = X2F (xSphereRad);
	fix l2 = F2X (sqrt (r * r - d * d));
	if (l2 >= l1) {
		intersection = *p0;
		return 1;
		}
	v *= l2;
	intersection += v;
	}
dist = CFixVector::Dist (*p0, intersection);
return dist ? dist : 1;

#else
	CFixVector	d, dn, w, vClosestPoint;
	fix			vecLen, dist, wDist, intDist;

//this routine could be optimized if it's taking too much time!

d = *p1 - *p0;
w = *vSpherePos - *p0;
dn = d; 
vecLen = CFixVector::Normalize (dn);
if (vecLen == 0) {
	intDist = w.Mag ();
	intersection = *p0;
	return ((xSphereRad < 0) || (intDist < xSphereRad)) ? intDist : 0;
	}
wDist = CFixVector::Dot (dn, w);
if (wDist < 0)
	return 0;	//moving away from CObject
if (wDist > vecLen + xSphereRad)
	return 0;	//cannot hit
vClosestPoint = *p0 + dn * wDist;
dist = CFixVector::Dist (vClosestPoint, *vSpherePos);
if  (dist < xSphereRad) {
	fix dist2 = FixMul (dist, dist);
	fix radius2 = FixMul (xSphereRad, xSphereRad);
	fix nShorten = FixSqrt (radius2 - dist2);
	intDist = wDist - nShorten;
	if ((intDist > vecLen) || (intDist < 0)) {
		//paste one or the other end of vector, which means we're inside
		intersection = *p0;		//don't move at all
		return 1;
		}
	dn *= intDist;
	intersection = *p0 + dn;         //calc intersection refP
	return intDist;
	}
#endif
return 0;
}

//	-----------------------------------------------------------------------------
//determine if a vector intersects with an CObject
//if no intersects, returns 0, else fills in intP and returns dist
fix CheckVectorObjectCollision (CHitData& hitData, CFixVector *p0, CFixVector *p1, fix rad, CObject *thisObjP, CObject *otherObjP, bool bCheckVisibility)
{
	fix			size, dist;
	CFixVector	vHit, vNormal, v0, v1, vn, vPos;
	int			bThisPoly, bOtherPoly;
	short			nModel = -1;

if (rad < 0)
	size = 0;
else {
	size = thisObjP->info.xSize;
	if (!thisObjP->IsStatic () && (thisObjP->info.nType == OBJ_ROBOT) && ROBOTINFO (thisObjP->info.nId).attackType)
		size = 3 * size / 4;
	//if obj is CPlayerData, and bumping into other CPlayerData or a weapon of another coop CPlayerData, reduce radius
	if ((thisObjP->info.nType == OBJ_PLAYER) &&
		 ((otherObjP->info.nType == OBJ_PLAYER) ||
 		 (IsCoopGame && (otherObjP->info.nType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parent.nType == OBJ_PLAYER))))
		size /= 2;
	}

#if DBG
if (thisObjP->Index () == nDbgObj)
	nDbgObj = nDbgObj;
if ((thisObjP->info.nType == OBJ_WEAPON) || (otherObjP->info.nType == OBJ_WEAPON))
	nDbgObj = nDbgObj;
#endif
// check hit sphere collisions
bOtherPoly = UseHitbox (otherObjP);
if ((bThisPoly = UseHitbox (thisObjP)))
	PolyObjPos (thisObjP, &vPos);
else
	vPos = thisObjP->info.position.vPos;
if ((CollisionModel () || thisObjP->IsStatic () || otherObjP->IsStatic ()) &&
	 !(UseSphere (thisObjP) || UseSphere (otherObjP)) &&
	 (bThisPoly || bOtherPoly)) {
#if 1 //DBG
	FindPointLineIntersection (vHit, *p0, *p1, vPos, 0);
	dist = VmLinePointDist (*p0, *p1, OBJPOS (thisObjP)->vPos);
	if (dist > thisObjP->ModelRadius (0) + otherObjP->ModelRadius (0))
		return 0;
#endif
	// check hitbox collisions for all polygonal objects
	if (bThisPoly && bOtherPoly) {
#if 1
		dist = CheckHitboxCollision (vHit, vNormal, otherObjP, thisObjP, p0, p1, nModel);
		if ((dist == 0x7fffffff) /*|| (dist > thisObjP->info.xSize)*/)
			return 0;
#else
		// check whether one object is stuck inside the other
		if (!(dist = CheckHitboxCollision (vHit, vNormal, otherObjP, thisObjP, p0, p1, nModel))) {
			if (!CFixVector::Dist (*p0, *p1))
				return 0;
			// check whether objects collide at all
			dist = CheckVectorHitboxCollision (vHit, vNormal, p0, p1, NULL, thisObjP, 0, nModel);
			if ((dist == 0x7fffffff) || (dist > thisObjP->info.xSize))
				return 0;
			}
		CheckHitboxCollision (vHit, vNormal, otherObjP, thisObjP, p0, p1, nModel);
		FindPointLineIntersection (vHit, *p0, *p1, vHit, 1);
#endif
		}
	else {
		if (bThisPoly) {
			// *thisObjP (stationary) has hitboxes, *otherObjP (moving) a hit sphere. To detect whether the sphere
			// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			if (0x7fffffff == (dist = CheckVectorHitboxCollision (vHit, vNormal, p0, p1, NULL, thisObjP, otherObjP->info.xSize, nModel)))
				return 0;
			}
		else {
			// *otherObjP (moving) has hitboxes, *thisObjP (stationary) a hit sphere. To detect whether the sphere
			// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			v0 = thisObjP->info.position.vPos;
			vn = otherObjP->info.position.vPos - v0;
			CFixVector::Normalize (vn);
			v1 = v0 + vn * thisObjP->info.xSize;
			if (0x7fffffff == (dist = CheckVectorHitboxCollision (vHit, vNormal, &v0, &v0, p1, otherObjP, thisObjP->info.xSize, nModel)))
				return 0;
			}
		}
	}
else {
	if (!(dist = CheckVectorSphereCollision (vHit, p0, p1, &vPos, size + rad)))
		return 0;
	nModel = 0;
	vNormal.SetZero ();
	}
hitData.vPoint = vHit;
hitData.vNormal = vNormal;
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

int ComputeObjectHitpoint (CHitData& hitData, CHitQuery &hitQuery)
{
	CObject		* thisObjP = (hitQuery.nObject < 0) ? NULL : OBJECTS + hitQuery.nObject,
			 		* otherObjP;
	int			nThisType = (hitQuery.nObject < 0) ? -1 : OBJECTS [hitQuery.nObject].info.nType;
	int			nObjSegList [7], nObjSegs = 1, i;
	fix			dMin = 0x7FFFFFFF;
	bool			bCheckVisibility = ((hitQuery.flags & FQ_VISIBILITY) != 0);
	CHitData		curHit;

nObjSegList [0] = hitQuery.nSegment;
#	if DBG
if ((thisObjP->info.nType == OBJ_WEAPON) && (thisObjP->info.nSegment == gameData.objs.consoleP->info.nSegment))
	hitQuery.flags = hitQuery.flags;
if (hitQuery.nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#	endif
#if 1
CSegment* segP = SEGMENTS + hitQuery.nSegment;
for (int nSide = 0; nSide < 6; nSide++) {
	short nSegment = segP->m_children [nSide];
	if (0 > nSegment)
		continue;
	CWall* wallP = segP->Wall (nSide);
	if (wallP && (wallP->IsDoorWay (NULL) & (WID_TRANSPARENT_WALL | WID_SOLID_WALL)))
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
		if ((nOtherType == OBJ_POWERUP) && ((hitQuery.flags & FQ_IGNORE_POWERUPS) || (gameStates.app.bGameSuspended & SUSP_POWERUPS)))
			continue;
		if ((hitQuery.flags & FQ_CHECK_PLAYER) && (nOtherType != OBJ_PLAYER))
			continue;
		if (otherObjP->info.nFlags & OF_SHOULD_BE_DEAD)
			continue;
		if (hitQuery.nObject == nObject)
			continue;
		if (otherObjP->Ignored (gameData.physics.bIgnoreObjFlag))
			continue;
		if (LasersAreRelated (nObject, hitQuery.nObject))
			continue;
		if (hitQuery.nObject > -1) {
			if ((gameData.objs.collisionResult [nThisType][nOtherType] == RESULT_NOTHING) &&
				 (gameData.objs.collisionResult [nOtherType][nThisType] == RESULT_NOTHING))
				continue;
			}
		int nFudgedRad = hitQuery.radP1;

		//	If this is a robot:robot collision, only do it if both of them have attackType != 0 (eg, green guy)
		if (nThisType == OBJ_ROBOT) {
			if ((hitQuery.flags & FQ_ANY_OBJECT) ? (nOtherType != OBJ_ROBOT) : (nOtherType == OBJ_ROBOT))
				continue;
			if (ROBOTINFO (thisObjP->info.nId).attackType)
				nFudgedRad = 3 * hitQuery.radP1 / 4;
			}
		//if obj is CPlayerData, and bumping into other CPlayerData or a weapon of another coop CPlayerData, reduce radius
		else if (nThisType == OBJ_PLAYER) {
			if (nOtherType == OBJ_ROBOT) {
				if ((hitQuery.flags & FQ_VISIBLE_OBJS) && otherObjP->Cloaked ())
					continue;
				}
			else if (nOtherType == OBJ_PLAYER) {
				if (IsCoopGame && (nOtherType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parent.nType == OBJ_PLAYER))
					nFudgedRad = hitQuery.radP1 / 2;
				}
			}
#if DBG
		if (nObject == nDbgObj)
			nDbgObj = nDbgObj;
#endif
		fix d = CheckVectorObjectCollision (curHit, hitQuery.p0, hitQuery.p1, nFudgedRad, otherObjP, thisObjP, bCheckVisibility);
		if (d && (d < dMin)) {
#if DBG
			CheckVectorObjectCollision (curHit, hitQuery.p0, hitQuery.p1, nFudgedRad, otherObjP, thisObjP, bCheckVisibility);
#endif
			dMin = d;
			hitData = curHit;
			hitData.nType = HIT_OBJECT;
			hitData.nObject = gameData.collisions.hitResult.nObject = nObject;
			Assert (gameData.collisions.hitResult.nObject != -1);
			if (hitQuery.flags & FQ_ANY_OBJECT)
				return dMin;
			}
		}
	}
return dMin;
}

//	-----------------------------------------------------------------------------

static inline int PassThrough (short nObject, short nSegment, short nSide, short nFace, int flags, CFixVector& vHitPoint)
{
CSegment* segP = SEGMENTS + nSegment;
int widResult = segP->IsDoorWay (nSide, (nObject < 0) ? NULL : OBJECTS + nObject);

if (widResult & WID_PASSABLE_FLAG) // check whether side can be passed through
	return 1; 

if ((widResult & WID_TRANSPARENT_WALL) == WID_TRANSPARENT_WALL) { // check whether side can be seen through
    if (flags & FQ_TRANSWALL) 
		 return 1;
	 if (!(flags & FQ_TRANSPOINT))
		 return 0;
	if (segP->CheckForTranspPixel (vHitPoint, nSide, nFace))
		return 1;
	}

// check whether side can be passed through due to a cheat
if (gameStates.app.cheats.bPhysics != 0xBADA55)
	return 0;
if (nObject != LOCALPLAYER.nObject) 
	return 0;
short nChildSeg = segP->m_children [nSide];
if (nChildSeg < 0)
	return 0;
CSegment* childSegP = SEGMENTS + nChildSeg;
if (childSegP->HasBlockedProp () ||
    (gameData.objs.speedBoost [nObject].bBoosted && ((segP->m_function != SEGMENT_FUNC_SPEEDBOOST) || (childSegP->m_function == SEGMENT_FUNC_SPEEDBOOST))))
	return 1;

return 0;
}

//	-----------------------------------------------------------------------------

static inline int CopySegList (short* dest, short nDestLen, short* src, short nSrcLen, int flags)
{
if (flags & FQ_GET_SEGLIST) {
	int i = MAX_FVI_SEGS - 1 - nDestLen;
	if (i > nSrcLen)
		i = nSrcLen;
	memcpy (dest + nDestLen, src, i * sizeof (*dest));
	return i;
	}
return 0;
}

//	-----------------------------------------------------------------------------

#define FVI_NEWCODE 2

int ComputeHitpoint (CHitData& hitData, CHitQuery& hitQuery, short* segList, short* nSegments, int nEntrySeg)
{
	CHitData		bestHit, curHit;
	fix			d, dMin = 0x7fffffff;					//distance to hit refP
	int			nHitNoneSegment = -1;
	short			hitNoneSegList [MAX_FVI_SEGS];
	short			nHitNoneSegs = 0;
	int			nCurNestLevel = gameData.collisions.hitResult.nNestCount;
	bool			bCheckVisibility = ((hitQuery.flags & FQ_VISIBILITY) != 0);

bestHit.vPoint.SetZero ();
//PrintLog (1, "Entry ComputeHitpoint\n");
if (hitQuery.flags & FQ_GET_SEGLIST)
	*segList = hitQuery.nSegment;
*nSegments = 1;
gameData.collisions.hitResult.nNestCount++;
//first, see if vector hit any objects in this CSegment
#if 1
if (hitQuery.flags & FQ_CHECK_OBJS) {
	//PrintLog (1, "checking objects...");
	dMin = ComputeObjectHitpoint (bestHit, hitQuery);
	}
#endif

CSegment* segP = SEGMENTS + hitQuery.nSegment;
if ((hitQuery.nObject > -1) && (gameData.objs.collisionResult [OBJECTS [hitQuery.nObject].info.nType][OBJ_WALL] == RESULT_NOTHING))
	hitQuery.radP1 = 0;		//HACK - ignore when edges hit walls
//now, check segment walls
#if DBG
if (hitQuery.nSegment == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
int startMask = segP->Masks (*hitQuery.p0, hitQuery.radP0).m_face;
CSegMasks masks = segP->Masks (*hitQuery.p1, hitQuery.radP1);    //on back of which faces?
int centerMask = masks.m_center;
int endMask = masks.m_face;

if (!centerMask)
	nHitNoneSegment = hitQuery.nSegment;
if (endMask) { //on the back of at least one face
	short nSide, iFace, bit;

	//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (endMask >= bit); nSide++) {
		int nChildSeg = segP->m_children [nSide];
#if 0
		if (bCheckVisibility && (0 > nChildSeg))	// poking through a wall into the void around the level?
			continue;
#endif
		int nFaces = segP->Side (nSide)->m_nFaces;
		for (iFace = 0; iFace < 2; iFace++, bit <<= 1) {
			if (nChildSeg == nEntrySeg)	//must be executed here to have bit shifted
				continue;		//don't go back through entry nSide
			if (!(endMask & bit))	//on the back of this face?
				continue;
			if (iFace >= nFaces)
				continue;
			//did we go through this wall/door?
#if DBG
			if ((hitQuery.nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			int nFaceHitType;
				nFaceHitType = (startMask & bit)	//start was also though.  Do extra check
									? segP->CheckLineToFaceSpecial (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace)
									: segP->CheckLineToFaceRegular (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace);
#if 1
			if (bCheckVisibility && !nFaceHitType)
					continue;
#endif
#if DBG
			if ((hitQuery.nSegment == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == nSide))) {
				nDbgSeg = nDbgSeg;
				if (nFaceHitType)
					nFaceHitType = (startMask & bit)	//start was also though.  Do extra check
										? segP->CheckLineToFaceSpecial (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace)
										: segP->CheckLineToFaceRegular (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace);
				}
#endif
			if (PassThrough (hitQuery.nObject, hitQuery.nSegment, nSide, iFace, hitQuery.flags, curHit.vPoint)) {
				int			i;
				short			subSegList [MAX_FVI_SEGS];
				short			nSubSegments;
				CHitData		subHit, saveHitData = gameData.collisions.hitResult;
				CHitQuery	subHitQuery = hitQuery;

				subHitQuery.nSegment = segP->m_children [nSide];
#if DBG
				if (subHitQuery.nSegment == nDbgSeg)
					nDbgSeg = nDbgSeg;
#endif
				for (i = 0; i < gameData.collisions.nSegsVisited && (subHitQuery.nSegment != gameData.collisions.segsVisited [i]); i++)
					;
				if (i == gameData.collisions.nSegsVisited) {                //haven't visited here yet
					if (gameData.collisions.nSegsVisited >= MAX_SEGS_VISITED)
						goto hitPointDone;	//we've looked a long time, so give up
					gameData.collisions.segsVisited [gameData.collisions.nSegsVisited++] = subHitQuery.nSegment;
					subHit.nType = ComputeHitpoint (subHit, subHitQuery, subSegList, &nSubSegments, hitQuery.nSegment);
					if (subHit.nType != HIT_NONE) {
						d = CFixVector::Dist (subHit.vPoint, *hitQuery.p0);
						if (d < dMin) {
							dMin = d;
							if (subHit.nSegment == -1)
								subHit.nSegment = curHit.nSegment;
							bestHit = subHit;
							*nSegments += CopySegList (segList, *nSegments, subSegList, nSubSegments, hitQuery.flags);
							}
						else {
							*((CHitData *) &gameData.collisions.hitResult) = saveHitData;    
 							}
						}
					else {
						*((CHitData *) &gameData.collisions.hitResult) = saveHitData;    
						if (subHit.nSegment != -1)
							nHitNoneSegment = subHit.nSegment;
						nHitNoneSegs = CopySegList (hitNoneSegList, 0, subSegList, nSubSegments, hitQuery.flags);
						}
					}
				}
			else {//a wall
				if (nFaceHitType) {
					//is this the closest hit?
					d = CFixVector::Dist (curHit.vPoint, *hitQuery.p0);
					if (d < dMin) {
						dMin = d;
						bestHit = curHit;
						bestHit.nType = HIT_WALL;
						bestHit.vNormal = gameData.collisions.hitResult.vNormal = segP->m_sides [nSide].m_normals [iFace];
						gameData.collisions.hitResult.nNormals = 1;
						if (!segP->Masks (curHit.vPoint, hitQuery.radP1).m_center)
							bestHit.nSegment = hitQuery.nSegment;             
						else
							gameData.collisions.hitResult.nAltSegment = hitQuery.nSegment;
						gameData.collisions.hitResult.nSegment = bestHit.nSegment;
						gameData.collisions.hitResult.nSide = nSide;
						gameData.collisions.hitResult.nFace = iFace;
						gameData.collisions.hitResult.nSideSegment = hitQuery.nSegment;
						}
					}
				}
			}
		}
	}

hitPointDone:

if (bestHit.nType == HIT_NONE) {     //didn't hit anything, return end refP
	hitData.vPoint = *hitQuery.p1;
	hitData.nSegment = nHitNoneSegment;
	if (nHitNoneSegment != -1) {			//(centerMask == 0)
		nSegments += CopySegList (segList, *nSegments, hitNoneSegList, nHitNoneSegs, hitQuery.flags);
		}
	else if (nCurNestLevel != 0)
		*nSegments = 0;
	}
else {
	hitData = bestHit;
	if (hitData.nSegment == -1)
		hitData.nSegment = (gameData.collisions.hitResult.nAltSegment != -1) 
			? gameData.collisions.hitResult.nAltSegment
			: hitData.nSegment = nHitNoneSegment;
	}
return bestHit.nType;
}

//	-----------------------------------------------------------------------------

//Find out if a vector intersects with anything.
//Fills in hitResult, an CHitResult structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisObjNum 		used to prevent an CObject with colliding with itself
//  ingore_obj			ignore collisions with this CObject
//  check_objFlag	determines whether collisions with OBJECTS are checked
//Returns the hitResult->nHitType
int FindHitpoint (CHitQuery& hitQuery, CHitResult& hitResult)
{
	CHitData		curHit, newHit;
	int			i, nHitboxes = extraGameInfo [IsMultiGame].nHitboxes; // save value

gameData.collisions.hitResult.vNormal.SetZero ();
gameData.collisions.hitResult.nNormals = 0;
if ((hitQuery.nSegment > gameData.segs.nLastSegment) || (hitQuery.nSegment < 0)) {
	hitQuery.nSegment = FindSegByPos (*hitQuery.p0, -1, 1, 0);
	if (hitQuery.nSegment < 0) {
		hitResult.nType = HIT_BAD_P0;
		hitResult.vPoint = *hitQuery.p0;
		hitResult.nSegment = hitQuery.nSegment;
		hitResult.nSide = 0;
		hitResult.nObject = 0;
		hitResult.nSideSegment = -1;
		hitResult.nSegments = 0;
		return hitResult.nType;
		}
	}

gameData.collisions.hitResult.nSegment = -1;
gameData.collisions.hitResult.nSide = -1;
gameData.collisions.hitResult.nObject = -1;

//check to make sure start refP is in seg its supposed to be in
//Assert(check_point_in_seg(p0, startseg, 0).m_center==0);	//start refP not in seg

// gameData.objs.viewerP is not in CSegment as claimed, so say there is no hit.
CSegMasks masks = SEGMENTS [hitQuery.nSegment].Masks (*hitQuery.p0, 0);
if (masks.m_center) {
	hitQuery.nSegment = FindSegByPos (*hitQuery.p0, -1, 1, 0);
	if (hitQuery.nSegment < 0) {
		hitResult.nType = HIT_BAD_P0;
		hitResult.vPoint = *hitQuery.p0;
		hitResult.nSegment = hitQuery.nSegment;
		hitResult.nSide = 0;
		hitResult.nObject = 0;
		hitResult.nSideSegment = -1;
		hitResult.nSegments = 0;
		return hitResult.nType;
		}
	}
gameData.collisions.segsVisited [0] = hitQuery.nSegment;
gameData.collisions.nSegsVisited = 1;
gameData.collisions.hitResult.nNestCount = 0;
if (hitQuery.flags & FQ_VISIBILITY)
	extraGameInfo [IsMultiGame].nHitboxes = 0;
ComputeHitpoint (curHit, hitQuery, hitResult.segList, &hitResult.nSegments, -2);
extraGameInfo [IsMultiGame].nHitboxes = nHitboxes;

if ((curHit.nSegment == -1) || SEGMENTS [curHit.nSegment].Masks (curHit.vPoint, 0).m_center)
	curHit.nSegment = FindSegByPos (curHit.vPoint, hitQuery.nSegment, 1, 0);

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
if ((curHit.nType == HIT_WALL) && (curHit.nSegment == -1))
	if ((gameData.collisions.hitResult.nAltSegment != -1) && !SEGMENTS [gameData.collisions.hitResult.nAltSegment].Masks (curHit.vPoint, 0).m_center)
		curHit.nSegment = gameData.collisions.hitResult.nAltSegment;

if (curHit.nSegment == -1) {
	CHitData altHit;
	CHitQuery altQuery = hitQuery;
	altQuery.radP0 = altQuery.radP1 = 0;

	//because the code that deals with objects with non-zero radius has
	//problems, try using zero radius and see if we hit a wall
	if (hitQuery.flags & FQ_VISIBILITY)
		extraGameInfo [IsMultiGame].nHitboxes = 0;
	ComputeHitpoint (altHit, altQuery, hitResult.segList, &hitResult.nSegments, -2);
	extraGameInfo [IsMultiGame].nHitboxes = nHitboxes;
	if (altHit.nSegment != -1)
		curHit = altHit;
	}

if ((curHit.nSegment != -1) && (hitQuery.flags & FQ_GET_SEGLIST))
	if ((curHit.nSegment != hitResult.segList [hitResult.nSegments - 1]) &&
		 (hitResult.nSegments < MAX_FVI_SEGS - 1))
		hitResult.segList [hitResult.nSegments++] = curHit.nSegment;

if ((curHit.nSegment != -1) && (hitQuery.flags & FQ_GET_SEGLIST))
	for (i = 0; i < (hitResult.nSegments) && i < (MAX_FVI_SEGS - 1); i++)
		if (hitResult.segList [i] == curHit.nSegment) {
			hitResult.nSegments = i + 1;
			break;
		}
*((CHitInfo *) &hitResult) = gameData.collisions.hitResult;
*((CHitData *) &hitResult) = curHit;
Assert ((curHit.nType != HIT_OBJECT) || (gameData.collisions.hitResult.nObject != -1));
return curHit.nType;
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
// Check whether point p1 from segment nDestSeg can be seen from point p0 located in segment nStartSeg.

int PointSeesPoint (CFloatVector* p0, CFloatVector* p1, short nStartSeg, short nDestSeg, int nDepth, int nThread)
{
	static			uint segVisList [MAX_THREADS][MAX_SEGMENTS_D2X];
	static			uint segVisFlags [MAX_THREADS] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

	CSegment*		segP;
	CSide*			sideP;
	CWall*			wallP;
	CFloatVector	intersection, v0, v1;
	float				l0, l1 = 0.0f;
	short				nSide, nFace, nFaceCount, nChildSeg;
	uint*				bVisited = segVisList [nThread];

if (!nDepth) {
#if 0
	memset (bVisited, 0, gameData.segs.nSegments * sizeof (*bVisited));
#else
	if (!++segVisFlags [nThread]) {
		++segVisFlags [nThread];
		memset (bVisited, 0, gameData.segs.nSegments * sizeof (*bVisited));
		}
#endif
	}

	uint			bFlag = segVisFlags [nThread];

for (;;) {
	bVisited [nStartSeg] = bFlag;
	segP = &SEGMENTS [nStartSeg];
	sideP = segP->Side (0);
	// check all sides of current segment whether they are penetrated by the vector p0,p1.
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++, sideP++) {
		nChildSeg = segP->m_children [nSide];
		if (nChildSeg < 0) {
			if ((nDestSeg >= 0) && (nStartSeg != nDestSeg))
				continue;
			}
		else {
			if (bVisited [nChildSeg] == bFlag)
				continue;
			}
		ushort* vertices = sideP->m_vertices;
		nFaceCount = sideP->m_nFaces;
		for (nFace = 0; nFace < nFaceCount; nFace++, vertices += 3) {
			if (!(nFace && sideP->IsQuad ())) {
				CFloatVector* n = sideP->m_fNormals + nFace;
				if (!FindPlaneLineIntersection (intersection, &FVERTICES [*vertices], n, p0, p1))
					continue;
				v0 = *p0 - intersection;
				v1 = *p1 - intersection;
				l0 = v0.Mag ();
				l1 = v1.Mag ();
				if ((l0 >= 0.001f) && (l1 >= 0.001f)) {
					v0 /= l0;
					v1 /= l1;
					if (CFloatVector::Dot (v0, *n) == CFloatVector::Dot (v1, *n))
						continue;
					}
				}
#if DBG
			if ((nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nDbgSeg = nDbgSeg;
#endif
			if (PointIsOutsideFace (&intersection, vertices, 5 - nFaceCount))
				continue;
			if (l1 >= 0.001f) 
				break;
			if (l1 < 1.0e-10f)
				return 1;
			// end point lies in this face
			if (nDestSeg < 0)
				return 1; // any segment acceptable
			if (nStartSeg == nDestSeg)
				return 1; // point is in desired segment
			if ((nChildSeg == nDestSeg) && !((wallP = sideP->Wall ()) && !(wallP->IsVolatile () || wallP->IsDoorWay (NULL, false) & WID_TRANSPARENT_FLAG)))
				return 1; // point at border to destination segment and the portal to that segment is passable
			nFace = nFaceCount; // no eligible child segment, so try next segment side
			break; 
			}
		if (nFace == nFaceCount)
			continue; // line doesn't intersect with this side
		if (0 > nChildSeg) // solid wall
			continue;
		if ((wallP = sideP->Wall ()) && !(wallP->IsVolatile () || wallP->IsDoorWay (NULL, false) & WID_TRANSPARENT_FLAG)) // impassable
			continue;
		if (PointSeesPoint (p0, p1, nChildSeg, nDestSeg, nDepth + 1, nThread))
			return 1;
		}
#if DBG
	if (!nDepth)
		nDepth = nDepth;
#endif
	return (nDestSeg < 0) || (nStartSeg == nDestSeg); // line doesn't intersect any side of this segment -> p1 must be inside segment
	}
}

//	-----------------------------------------------------------------------------

int UseSphere (CObject *objP)
{
	int nType = objP->info.nType;

return gameStates.app.bNostalgia || (nType == OBJ_MONSTERBALL) || (nType == OBJ_HOSTAGE) || (nType == OBJ_POWERUP) || objP->IsMine ();
}

//	-----------------------------------------------------------------------------

//eof
