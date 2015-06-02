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

int32_t SphereToFaceRelation (CFixVector* pRef, fix rad, CFixVector *vertList, int32_t nVerts, CFixVector* vNormal);

//	-----------------------------------------------------------------------------

//given largest component of Normal, return i & j
//if largest component is negative, swap i & j
int32_t ijTable [3][2] = {
	{2, 1},  //pos x biggest
	{0, 2},  //pos y biggest
	{1, 0}	//pos z biggest
	};

//	-----------------------------------------------------------------------------
// see if a point (pRef) is inside a triangle using barycentric method
// return 0 if point inside triangle, otherwise sides point is behind as bit code

uint8_t PointIsOutsideFace (CFixVector* vRef, CFixVector* vertices, int16_t nVerts)
{
#if 0

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
return int32_t (u < 0) + (int32_t (v < 0) << 1) + (int32_t (u + v > I2X (1)) << 2);

#endif
}

//	-----------------------------------------------------------------------------
//see if a point (pRef) is inside a triangle using barycentric method

uint8_t PointIsOutsideFace (CFixVector* vRef, uint16_t* nVertIndex, int16_t nVerts)
{
#if 0

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
return int32_t (u < -PLANE_DIST_TOLERANCE) + (int32_t (v < -PLANE_DIST_TOLERANCE) << 1) + (int32_t (u + v > I2X (1) + PLANE_DIST_TOLERANCE) << 2); // compensate for numerical errors

#endif
}

//	-----------------------------------------------------------------------------
//see if a point (pRef) is inside a triangle using barycentric method

uint8_t PointIsOutsideFace (CFloatVector* vRef, uint16_t* nVertIndex, int16_t nVerts)
{
#if 1
#	if 0


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
return (int32_t (u < -0.00001f)) + ((int32_t (v < -0.00001f)) << 1) + ((int32_t (u + v > 1.00001f)) << 2); // compensate for numerical errors


#	endif

#else

	CFloatVector	t, *v0, *v1;
	int32_t 			i, j, nEdge, biggest;
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

float DistToFace (CFloatVector3 vRef, int16_t nSegment, uint8_t nSide, CFloatVector3* vHit)
{
	CSide*			pSide = SEGMENT (nSegment)->Side (nSide);
	CFloatVector	h, r;
	uint16_t*		vertices = pSide->m_vertices;
	int32_t			i, j;
	float				d = 0.0f;

r.Assign (vRef);

// compute vIntersection of vector perpendicular to the plane through vRef with the face's plane(s)
for (i = j = 0; i < pSide->m_nFaces; i++, j += 3) {
	//if (!(i && pSide->IsQuad ())) 
		{
		CFloatVector& n = pSide->m_fNormals [i];
		if (n.Mag () < 1e-6f) // there is something wrong with this face
			continue;
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

vertices = pSide->m_corners;

	CFloatVector	* v0, * v1 = &FVERTICES [vertices [0]];
	float				minDist = 1e10f;
	int32_t			nVertices = pSide->m_nCorners;

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

int32_t FindPlaneLineIntersectionf (CFloatVector& vIntersection, CFloatVector& vPlane, CFloatVector& vNormal, CFloatVector& p0, CFloatVector& p1, float rad)
{
	CFloatVector vn, vu, vw;

vu = p1 - p0;
vn = vNormal;
float d = -CFloatVector::Dot (vn, vu);
if (fabs (d) < FLOAT_DIST_TOLERANCE) // ~ parallel
	return 0;
vw = p0 - vPlane;
float n = CFloatVector::Dot (vn, vw) - X2F (rad);
float s = n / d;
if (s < 0.0f) {
	if (s < -FLOAT_DIST_TOLERANCE) // compensate small numerical errors
		return 0;
	s = 0.0f;
	}
else if (s > 1.0f) {
	if (s > 1.0f + FLOAT_DIST_TOLERANCE) // compensate small numerical errors
		return 0;
	s = 1.0f;
	}
vu *= s;
vIntersection = vu;
vIntersection += p0;
return 1;
}

//	-----------------------------------------------------------------------------
//find the vIntersection on the specified plane where the line intersects
//returns true if vIntersection found, false if line parallel to plane
//vIntersection is the found vIntersection on the plane
//vPlanePoint & vPlaneNorm describe the plane
//p0 & p1 are the ends of the line

int32_t FindPlaneLineIntersection (CFixVector& vIntersection, CFixVector *vPlane, CFixVector *vNormal, CFixVector *p0, CFixVector *p1, fix rad)
{
#if 0 // FLOAT_COLLISION_MATH
// for some reason I haven't been able to figure, the following code is not equivalent to the fix point arithmetic code below it
	CFloatVector n, u, w;

n.Assign (*vNormal);
u.Assign (*p1 - *p0);
float den = -CFloatVector::Dot (n, u) - X2F (rad);
if (fabs (den) < FLOAT_DIST_TOLERANCE) // ~ parallel
	return 0;
w.Assign (*p0 - *vPlane);
float num = CFloatVector::Dot (n, w);
float s = num / den;
#if 1
if (s > 1.0f) // compensate small numerical errors
#else
if ((s < -FLOAT_DIST_TOLERANCE) || (s > 1.0f + FLOAT_DIST_TOLERANCE)) // compensate small numerical errors
#endif
	return 0;
u *= s;
vIntersection.Assign (u);
vIntersection += *p0;
return 1;

#else

CFixVector u = *p1 - *p0;
fix den = -CFixVector::Dot (*vNormal, u);
if (!den) // moving parallel to face's plane
	return 0;
CFixVector w = *p0 - *vPlane;
fix num = CFixVector::Dot (*vNormal, w) - rad; // distance of p0 to plane
//do check for potential overflow
#if 0
if (den > 0) {
	if (num > den) { //frac greater than one
		return 0;
		}
	}
else {
	if (num < den) {
		return 0;
		}
	}
#if 0
if (labs (num) >> 15 >= labs (den)) 
	return 0;
#endif
u *= FixDiv (num, den);
#else
fix s = FixDiv (num, den);
#if 1
if (s > I2X (1))
#else
if ((s < PLANE_DIST_TOLERANCE) || (s > I2X (1) + PLANE_DIST_TOLERANCE))
#endif
	return 0;
u *= s;
#endif
vIntersection = *p0 + u;
return 1;

#endif
}

//	-----------------------------------------------------------------------------

int32_t FindPlaneLineIntersection (CFloatVector& vIntersection, CFloatVector* vPlane, CFloatVector* vNormal, CFloatVector* p0, CFloatVector* p1)
{
CFloatVector u = *p1;
u -= *p0;
float d = -CFloatVector::Dot (*vNormal, u);
if (fabs (d) < FLOAT_DIST_TOLERANCE) {// ~ parallel
	return 0;
	}
CFloatVector w = *p0;
w -= *vPlane;
float n = CFloatVector::Dot (*vNormal, w);
float s = n / d;
if (s < 0.0f) {
	if (s < -FLOAT_DIST_TOLERANCE) // compensate small numerical errors
		return 0;
	s = 0.0f;
	}
else if (s > 1.0f) {
	if (s > 1.0f + FLOAT_DIST_TOLERANCE) // compensate small numerical errors
		return 0;
	s = 1.0f;
	}
u *= s;
vIntersection = *p0;
vIntersection += u;
return 1;
}

//	-----------------------------------------------------------------------------

uint32_t PointToFaceRelationf (CFloatVector& vRef, CFloatVector* vertList, int32_t nVerts, CFloatVector& vNormal)
{
//now do 2d check to see if pRef is in side
//project polygon onto plane by finding largest component of Normal
CFloatVector v;
v.v.coord.x = fabs (vNormal.v.coord.x);
v.v.coord.y = fabs (vNormal.v.coord.y);
v.v.coord.z = fabs (vNormal.v.coord.z);

int32_t nBiggest = (v.v.coord.x > v.v.coord.y) ? (v.v.coord.x > v.v.coord.z) ? 0 : 2 : (v.v.coord.y > v.v.coord.z) ? 1 : 2;

int32_t i, j;
if (vNormal.v.vec [nBiggest] > 0) {
	i = ijTable [nBiggest][0];
	j = ijTable [nBiggest][1];
	}
else {
	i = ijTable [nBiggest][1];
	j = ijTable [nBiggest][0];
	}

//now do the 2d problem in the i, j plane
float check_i = vRef.v.vec [i];
float check_j = vRef.v.vec [j];

uint32_t nEdgeMask = 0;
for (int32_t nEdge = 0; nEdge < nVerts; nEdge++) {
	CFloatVector& v0 = vertList [nEdge];
	CFloatVector& v1 = vertList [(nEdge + 1) % nVerts];
	CFloatVector2 vEdge (v1.v.vec [i] - v0.v.vec [i], v1.v.vec [j] - v0.v.vec [j]);
	CFloatVector2 vCheck (check_i - v0.v.vec [i], check_j - v0.v.vec [j]);
	float d = vCheck.Cross (vEdge);
	if (d < 0.0f)  //we are outside of triangle
		nEdgeMask |= (1 << nEdge);
	}
return nEdgeMask;
}

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d
uint32_t PointToFaceRelation (CFixVector* vRef, CFixVector *vertList, int32_t nVerts, CFixVector* vNormal)
{
#if FLOAT_COLLISION_MATH

	CFloatVector	vr, vn, vl [4];

vr.Assign (*vRef);
vn.Assign (*vNormal);
for (int32_t i = 0; i < nVerts; i++)
	vl [i].Assign (vertList [i]);
return PointToFaceRelationf (vr, vl, nVerts, vn);

#else

//now do 2d check to see if vRef is in side
//project polygon onto plane by finding largest component of Normal
CFixVector	t;
t.v.coord.x = labs (vNormal->v.coord.x);
t.v.coord.y = labs (vNormal->v.coord.y);
t.v.coord.z = labs (vNormal->v.coord.z);

int32_t nBiggest;
if (t.v.coord.x > t.v.coord.y)
	if (t.v.coord.x > t.v.coord.z)
		nBiggest = 0;
	else
		nBiggest = 2;
else 
	if (t.v.coord.y > t.v.coord.z)
		nBiggest = 1;
	else
		nBiggest = 2;

int32_t i, j;
if (vNormal->v.vec [nBiggest] > 0) {
	i = ijTable [nBiggest][0];
	j = ijTable [nBiggest][1];
	}
else {
	i = ijTable [nBiggest][1];
	j = ijTable [nBiggest][0];
	}
//now do the 2d problem in the i, j plane
fix check_i = vRef->v.vec [i];
fix check_j = vRef->v.vec [j];

uint32_t nEdgeMask = 0;
for (int32_t nEdge = 0; nEdge < nVerts; nEdge++) {
	CFixVector* v0 = vertList + nEdge;
	CFixVector* v1 = vertList + ((nEdge + 1) % nVerts);
	CFixVector2	vEdge (v1->v.vec [i] - v0->v.vec [i], v1->v.vec [j] - v0->v.vec [j]);
	CFixVector2 vCheck (check_i - v0->v.vec [i], check_j - v0->v.vec [j]);
	fix d = FixMul (vCheck.m_x, vEdge.m_y) - FixMul (vCheck.m_y, vEdge.m_x);
	if (d < 0)              		//we are outside of triangle
		nEdgeMask |= (1 << nEdge);
	}
return nEdgeMask;

#endif
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int32_t SphereToFaceRelationf (CFloatVector& vr, float r, CFloatVector *vl, int32_t nVerts, CFloatVector& vn)
{

//now do 2d check to see if vRef is inside
uint32_t nEdgeMask = PointToFaceRelationf (vr, vl, nVerts, vn);
if (!nEdgeMask)
	return IT_FACE;	//we've gone through all the sides, and are inside

//get verts for edge we're behind
int32_t nEdge;
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;

CFloatVector* v0 = vl + nEdge;
CFloatVector* v1 = vl + ((nEdge + 1) % nVerts);
//check if we are touching an edge or vRef
CFloatVector vCheck = vr - *v0;
CFloatVector vEdge = *v1 - *v0;
float nEdgeLen = CFloatVector::Normalize (vEdge);
//find vRef dist from planes of ends of edge
float d = CFloatVector::Dot (vEdge, vCheck);
if (d + r < 0)
	return IT_NONE;                  //too far behind start vRef
if (d - r > nEdgeLen)
	return IT_NONE;    //too far part end vRef
//find closest vRef on edge to check vRef
int32_t iType = IT_POINT;
CFloatVector vNearest;            //this time, real 3d vectors
if (d < 0)
	vNearest = *v0;
else if (d > nEdgeLen)
	vNearest = *v1;
else {
	iType = IT_EDGE;
	vNearest = *v0 + vEdge * d;
	}
d = CFloatVector::Dist (vr, vNearest);
if (d <= r)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int32_t SphereToFaceRelation (CFixVector* vRef, fix rad, CFixVector *vertList, int32_t nVerts, CFixVector* vNormal)
{
#if FLOAT_COLLISION_MATH

	CFloatVector	vr, vn, vl [4];

vr.Assign (*vRef);
vn.Assign (*vNormal);
for (int32_t i = 0; i < nVerts; i++)
	vl [i].Assign (vertList [i]);
return SphereToFaceRelationf (vr, X2F (rad), vl, nVerts, vn);

#else

	CFixVector	vEdge, vCheck;            //this time, real 3d vectors
	CFixVector	vNearest;
	fix			xEdgeLen, d;
	CFixVector* v0, * v1;
	int32_t		iType;
	int32_t		nEdge;
	uint32_t		nEdgeMask;

//now do 2d check to see if vRef is inside
if (!(nEdgeMask = PointToFaceRelation (vRef, vertList, nVerts, vNormal)))
	return IT_FACE;	//we've gone through all the sides, and are inside
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
v0 = vertList + nEdge;
v1 = vertList + ((nEdge + 1) % nVerts);
//check if we are touching an edge or vRef
vCheck = *vRef - *v0;
xEdgeLen = CFixVector::NormalizedDir (vEdge, *v1, *v0);
//find vRef dist from planes of ends of edge
d = CFixVector::Dot (vEdge, vCheck);
if (d + rad < 0)
	return IT_NONE;                  //too far behind start vRef
if (d - rad > xEdgeLen)
	return IT_NONE;    //too far part end vRef
//find closest vRef on edge to check vRef
iType = IT_POINT;
if (d < 0)
	vNearest = *v0;
else if (d > xEdgeLen)
	vNearest = *v1;
else {
	iType = IT_EDGE;
	vNearest = *v0 + vEdge * d;
	}
d = CFixVector::Dist (*vRef, vNearest);
if (d <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;

#endif
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in vIntersection with vIntersection
//pRef on plane, whether or not line intersects CSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a pRef field
int32_t CheckLineToFaceRegular (CFixVector& vIntersection, CFixVector *p0, CFixVector *p1, fix rad, CFixVector *vertList, int32_t nVerts, CFixVector *vNormal)
{
	CFixVector	v1, vHit;
	int32_t		pli, bCheckRad = 0;

//use lowest pRef number
if (p1 == p0) {
	if (!rad)
		return IT_NONE;
	v1 = *vNormal * (-rad);
	v1 += *p0;
	bCheckRad = rad;
	rad = 0;
	p1 = &v1;
	}
if (!(pli = FindPlaneLineIntersection (vIntersection, vertList, vNormal, p0, p1, rad)))
	return IT_NONE;
vHit = vIntersection;
//if rad != 0, project the pRef down onto the plane of the polygon
if (rad)
	vHit += *vNormal * (-rad);
if ((pli = SphereToFaceRelation (&vHit, rad, vertList, nVerts, vNormal)))
	return pli;
if (bCheckRad) {
	int32_t			i, d;
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
int32_t CheckLineToLinef (float& t1, float& t2, CFloatVector& p1, CFloatVector& v1, CFloatVector& p2, CFloatVector& v2)
{
CFloatMatrix det;
det.m.dir.r = p2 - p1;
det.m.dir.f = CFloatVector::Cross (v1, v2);
float crossMag2 = CFloatVector::Dot (det.m.dir.f, det.m.dir.f);
if (fabs (crossMag2) < FLOAT_DIST_TOLERANCE)
	return 0;			//lines are parallel
det.m.dir.u = v2;
float d = det.Det ();
t1 = d / crossMag2;
det.m.dir.u = v1;
d = det.Det ();
t2 = d / crossMag2;
return 1;		//found pRef
}

//	-----------------------------------------------------------------------------
//computes the parameters of closest approach of two lines
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
int32_t CheckLineToLine (fix *t1, fix *t2, CFixVector *p1, CFixVector *v1, CFixVector *p2, CFixVector *v2)
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
return 1;		//found pRef
}

//	-----------------------------------------------------------------------------
// determine if and where a vector intersects with a sphere
// vector defined by p0, p1
// returns distance of sphere vIntersection to sphere center if intersects, and fills in intP
// else returns -1

int32_t CheckVectorSphereCollisionf (CFloatVector& vi, CFloatVector& v0, CFloatVector& v1, CFloatVector& vs, float r)
{
CFloatVector vw = vs - v0;
CFloatVector vn = v1 - v0;
float l = CFloatVector::Normalize (vn);

if (l == 0.0f) {
	float di = vw.Mag ();
	vi = v0;
	return (di < r) ? F2X (di) : -1;
	}

float dw = CFloatVector::Dot (vn, vw);
if (dw < 0)
	return -1;	// moving away from object
if (dw > l + r)
	return -1;	// cannot hit
vi = v0 + vn * dw;
float d = CFloatVector::Dist (vi, vs);
if  (d < r) {
	float di = dw - sqrt (r * r - d * d);
	if (di > l)
		return -1; // ||p0,sphere center|| greater than ||p0,p1||
	if (di < 0) {
		vi = v0; // past one or the other end of vector, which means we're inside; so don't move at all
		return F2X (CFloatVector::Dist (v0, vs));
		}
	vn *= di;
	v0 += vn;
	vi = v0;         
	return F2X (di);
	}
return -1;
}

//	-----------------------------------------------------------------------------

int32_t CheckVectorSphereCollision (CFixVector& vIntersection, CFixVector *p0, CFixVector *p1, CFixVector *vSpherePos, fix xSphereRad)
{
#if FLOAT_COLLISION_MATH

	CFloatVector	v0, v1, vs, vi;

v0.Assign (*p0);
v1.Assign (*p1);
vs.Assign (*vSpherePos);
int32_t dist = CheckVectorSphereCollisionf (vi, v0, v1, vs, X2F (xSphereRad));
if (dist >= 0)
	vIntersection.Assign (vi);
return dist;

#else

	CFixVector	d, dn, w, vNearest;
	fix			vecLen, dist, wDist, intDist;

//this routine could be optimized if it's taking too much time!

d = *p1 - *p0;
w = *vSpherePos - *p0;
dn = d; 
vecLen = CFixVector::Normalize (dn);
if (vecLen == 0) {
	intDist = w.Mag ();
	vIntersection = *p0;
	return ((xSphereRad < 0) || (intDist < xSphereRad)) ? intDist : 0;
	}
wDist = CFixVector::Dot (dn, w);
if (wDist < 0)
	return -1;	//moving away from CObject
if (wDist > vecLen + xSphereRad)
	return -1;	//cannot hit
vNearest = *p0 + dn * wDist;
dist = CFixVector::Dist (vNearest, *vSpherePos);
if  (dist < xSphereRad) {
	fix dist2 = FixMul (dist, dist);
	fix radius2 = FixMul (xSphereRad, xSphereRad);
	fix nShorten = FixSqrt (radius2 - dist2);
	intDist = wDist - nShorten;
	if (intDist > vecLen) {
#if 1
		return -1;
#else
		vIntersection = *p0;		//don't move at all
		return 1;
#endif
		}
	if (intDist < 0) {
		//paste one or the other end of vector, which means we're inside
		vIntersection = *p0;		//don't move at all
		return 1;
		}
	dn *= intDist;
	vIntersection = *p0 + dn;         //calc vIntersection pRef
	return intDist;
	}
return -1;

#endif
}

//	-----------------------------------------------------------------------------
//determine if a vector intersects with an CObject
//if no intersects, returns 0, else fills in intP and returns dist
fix CheckVectorObjectCollision (CHitData& hitData, CFixVector *p0, CFixVector *p1, fix rad, CObject *pThisObj, CObject *pOtherObj, int32_t nCollisionModel, bool bCheckVisibility)
{
ENTER (1, 0);

	fix			size, dist;
	CFixVector	vHit, vNormal, v0, v1, vn, vPos;
	int32_t		bThisPoly, bOtherPoly;
	int16_t		nModel = -1;
	tRobotInfo	*pRobotInfo;

if (rad < 0)
	size = 0;
else {
	size = pThisObj->info.xSize;
	if (!pThisObj->IsStatic () && (pRobotInfo = ROBOTINFO (pThisObj)) && pRobotInfo->attackType)
		size = 3 * size / 4;
	//if obj is CPlayerData, and bumping into other CPlayerData or a weapon of another coop CPlayerData, reduce radius
	if ((pThisObj->info.nType == OBJ_PLAYER) &&
		 ((pOtherObj->info.nType == OBJ_PLAYER) ||
 		 (IsCoopGame && (pOtherObj->info.nType == OBJ_WEAPON) && (pOtherObj->cType.laserInfo.parent.nType == OBJ_PLAYER))))
		size /= 2;
	}

#if DBG
if (pThisObj->Index () == nDbgObj)
	BRP;
if ((pThisObj->info.nType == OBJ_WEAPON) || (pOtherObj->info.nType == OBJ_WEAPON))
	BRP;
#endif
// check hit sphere collisions
bOtherPoly = UseHitbox (pOtherObj);
if ((bThisPoly = UseHitbox (pThisObj)))
	PolyObjPos (pThisObj, &vPos);
else
	vPos = pThisObj->info.position.vPos;
if ((CollisionModel (nCollisionModel) || pThisObj->IsStatic () || pOtherObj->IsStatic ()) &&
	 !(UseSphere (pThisObj) || UseSphere (pOtherObj)) &&
	 (bThisPoly || bOtherPoly)) {
#if 1 //DBG
	FindPointLineIntersection (vHit, *p0, *p1, vPos, 0);
	dist = VmLinePointDist (*p0, *p1, OBJPOS (pThisObj)->vPos);
	if (dist > pThisObj->ModelRadius (0) + pOtherObj->ModelRadius (0))
		RETVAL (0)
#endif
	// check hitbox collisions for all polygonal objects
	if (bThisPoly && bOtherPoly) {
#if 1
		dist = CheckHitboxCollision (vHit, vNormal, pOtherObj, pThisObj, p0, p1, nModel);
		if ((dist == 0x7fffffff) /*|| (dist > pThisObj->info.xSize)*/)
			RETVAL (0)
#else
		// check whether one object is stuck inside the other
		if (!(dist = CheckHitboxCollision (vHit, vNormal, pOtherObj, pThisObj, p0, p1, nModel))) {
			if (!CFixVector::Dist (*p0, *p1))
				RETVAL (0)
			// check whether objects collide at all
			dist = CheckVectorHitboxCollision (vHit, vNormal, p0, p1, NULL, pThisObj, 0, nModel);
			if ((dist == 0x7fffffff) || (dist > pThisObj->info.xSize))
				RETVAL (0)
			}
		CheckHitboxCollision (vHit, vNormal, pOtherObj, pThisObj, p0, p1, nModel);
		FindPointLineIntersection (vHit, *p0, *p1, vHit, 1);
#endif
		}
	else {
		if (bThisPoly) {
			// *pThisObj (stationary) has hitboxes, *pOtherObj (moving) a hit sphere. To detect whether the sphere
			// intersects with the hitbox, check whether the radius line of *pThisObj intersects any of the hitboxes.
			if (0x7fffffff == (dist = CheckVectorHitboxCollision (vHit, vNormal, p0, p1, NULL, pThisObj, pOtherObj->info.xSize, nModel)))
				RETVAL (0)
			}
		else {
			// *pOtherObj (moving) has hitboxes, *pThisObj (stationary) a hit sphere. To detect whether the sphere
			// intersects with the hitbox, check whether the radius line of *pThisObj intersects any of the hitboxes.
			v0 = pThisObj->info.position.vPos;
			vn = pOtherObj->info.position.vPos - v0;
			CFixVector::Normalize (vn);
			v1 = v0 + vn * pThisObj->info.xSize;
			if (0x7fffffff == (dist = CheckVectorHitboxCollision (vHit, vNormal, &v0, &v0, p1, pOtherObj, pThisObj->info.xSize, nModel)))
				RETVAL (0)
			}
		}
	}
else {
	if (0 > (dist = CheckVectorSphereCollision (vHit, p0, p1, &vPos, size + rad)))
		RETVAL (0)
	nModel = 0;
	vNormal.SetZero ();
	}
hitData.vPoint = vHit;
hitData.vNormal = vNormal;
#if 0 //DBG
CreatePowerup (POW_SHIELD_BOOST, pThisObj->Index (), pOtherObj->info.nSegment, vHit, 1, 1);
#endif
if (!bCheckVisibility && (pOtherObj->info.nType != OBJ_POWERUP) 
#if 0 // well, the Guidebot should actually cause critical damage when ramming the player, shouldn't it?
	 && ((pOtherObj->info.nType != OBJ_ROBOT) || !ROBOTINFO (pOtherObj->info.nId)->companion)
#endif
	 ) {
	vHit = pThisObj->RegisterHit (vHit, nModel);
	//vHit = pOtherObj->RegisterHit (vHit, nModel);
	}
RETVAL (dist);
}

//	-----------------------------------------------------------------------------

int32_t ObjectInList (int16_t nObject, int16_t *objListP)
{
	int16_t t;

while (((t = *objListP) != -1) && (t != nObject))
	objListP++;
return (t == nObject);

}

//	-----------------------------------------------------------------------------

int32_t ComputeObjectHitpoint (CHitData& hitData, CHitQuery &hitQuery, int32_t nCollisionModel, int32_t nThread)
{
ENTER (1, nThread);

	CObject		* pThisObj = (hitQuery.nObject < 0) ? NULL : OBJECT (hitQuery.nObject),
			 		* pOtherObj;
	int32_t		nThisType = (hitQuery.nObject < 0) ? -1 : OBJECT (hitQuery.nObject)->info.nType;
	int32_t		nObjSegList [7], nObjSegs = 1, i;
	fix			dMin = 0x7FFFFFFF;
	bool			bCheckVisibility = ((hitQuery.flags & FQ_VISIBILITY) != 0);
	CHitData		curHit;

nObjSegList [0] = hitQuery.nSegment;
#	if DBG
if ((pThisObj->info.nType == OBJ_WEAPON) && (pThisObj->info.nSegment == gameData.objData.pConsole->info.nSegment))
	BRP;
if (hitQuery.nSegment == nDbgSeg)
	BRP;
#	endif
#if 1
CSegment* pSeg = SEGMENT (hitQuery.nSegment);
for (int32_t nSide = 0; nSide < 6; nSide++) {
	int16_t nSegment = pSeg->m_children [nSide];
	if (0 > nSegment)
		continue;
	CWall* pWall = pSeg->Wall (nSide);
	if (pWall && (pWall->IsPassable (NULL) & (WID_TRANSPARENT_WALL | WID_SOLID_WALL)))
		continue;
	for (i = 0; i < gameData.collisionData.nSegsVisited [nThread] && (nSegment != gameData.collisionData.segsVisited [nThread][i]); i++)
		;
	if (i == gameData.collisionData.nSegsVisited [nThread])
		nObjSegList [nObjSegs++] = nSegment;
	}
#endif
for (int32_t iObjSeg = 0; iObjSeg < nObjSegs; iObjSeg++) {
	int16_t nSegment = nObjSegList [iObjSeg];
	pSeg = SEGMENT (nSegment);
#if DBG
restart:
	if (nSegment == nDbgSeg)
		BRP;
#endif
	int16_t nSegObjs = gameData.objData.nObjects;
	int16_t nFirstObj = SEGMENT (nSegment)->m_objects;
	for (int16_t nObject = nFirstObj; nObject != -1; nObject = pOtherObj->info.nNextInSeg, nSegObjs--) {
		pOtherObj = OBJECT (nObject);
#if DBG
		if (nObject == nDbgObj)
			BRP;
		if ((nSegObjs < 0) || !CheckSegObjList (pOtherObj, nObject, nFirstObj)) {
			RelinkAllObjsToSegs ();
			goto restart;
			}
#endif
		int32_t nOtherType = pOtherObj->info.nType;
		if ((nOtherType == OBJ_POWERUP) && ((hitQuery.flags & FQ_IGNORE_POWERUPS) || (gameStates.app.bGameSuspended & SUSP_POWERUPS)))
			continue;
		if ((hitQuery.flags & FQ_CHECK_PLAYER) && (nOtherType != OBJ_PLAYER))
			continue;
		if (pOtherObj->info.nFlags & OF_SHOULD_BE_DEAD)
			continue;
		if (hitQuery.nObject == nObject)
			continue;
		if (pOtherObj->Ignored (gameData.physicsData.bIgnoreObjFlag))
			continue;
		if (LasersAreRelated (nObject, hitQuery.nObject))
			continue;
		if (hitQuery.nObject > -1) {
			if ((gameData.objData.collisionResult [nThisType][nOtherType] == RESULT_NOTHING) &&
				 (gameData.objData.collisionResult [nOtherType][nThisType] == RESULT_NOTHING))
				continue;
			}
		int32_t nFudgedRad = hitQuery.radP1;

		//	If this is a robot:robot collision, only do it if both of them have attackType != 0 (eg, green guy)
		if (nThisType == OBJ_ROBOT) {
			if ((hitQuery.flags & FQ_ANY_OBJECT) ? (nOtherType != OBJ_ROBOT) : (nOtherType == OBJ_ROBOT))
				continue;
			if (ROBOTINFO (pThisObj) && ROBOTINFO (pThisObj)->attackType)
				nFudgedRad = 3 * hitQuery.radP1 / 4;
			}
		//if obj is CPlayerData, and bumping into other CPlayerData or a weapon of another coop CPlayerData, reduce radius
		else if (nThisType == OBJ_PLAYER) {
			if (nOtherType == OBJ_ROBOT) {
				if ((hitQuery.flags & FQ_VISIBLE_OBJS) && pOtherObj->Cloaked ())
					continue;
				}
			else if (nOtherType == OBJ_PLAYER) {
				if (IsCoopGame && (nOtherType == OBJ_WEAPON) && (pOtherObj->cType.laserInfo.parent.nType == OBJ_PLAYER))
					nFudgedRad = hitQuery.radP1 / 2;
				}
			}
#if DBG
		if (nObject == nDbgObj)
			BRP;
#endif
		fix d = CheckVectorObjectCollision (curHit, hitQuery.p0, hitQuery.p1, nFudgedRad, pOtherObj, pThisObj, nCollisionModel, bCheckVisibility);
		if (d && (d < dMin)) {
#if DBG
			CheckVectorObjectCollision (curHit, hitQuery.p0, hitQuery.p1, nFudgedRad, pOtherObj, pThisObj, nCollisionModel, bCheckVisibility);
#endif
			dMin = d;
			hitData = curHit;
			hitData.nType = HIT_OBJECT;
			hitData.nObject = (gameData.collisionData.hitResult.nObject = nObject);
			Assert (nObject != -1);
			if (hitQuery.flags & FQ_ANY_OBJECT)
				RETVAL (dMin);
			}
		}
	}
RETVAL (dMin);
}

//	-----------------------------------------------------------------------------

static inline int32_t PassThrough (CHitQuery& hitQuery, int16_t nSide, int16_t nFace, CFixVector& vHitPoint)
{
CSegment* pSeg = SEGMENT (hitQuery.nSegment);
if (!pSeg)
	return 0;
if (pSeg->m_children [nSide] < 0)
	return 0;

int32_t widResult = pSeg->IsPassable (nSide, OBJECT (hitQuery.nObject));

if (widResult & WID_PASSABLE_FLAG) // check whether side can be passed through
	return 1; 

#if 1
if (!pSeg->Masks (*hitQuery.p1, hitQuery.radP1).m_center) {
	CFixVector vMoved = *hitQuery.p1 - *hitQuery.p0;
	CFixVector::Normalize (vMoved);
	if (CFixVector::Dot (pSeg->m_sides [nSide].m_normals [nFace], vMoved) > 0) // moving away from face
		return 1;
		}
#endif

if ((widResult & WID_TRANSPARENT_WALL) == WID_TRANSPARENT_WALL) { // check whether side can be seen through
    if (hitQuery.flags & FQ_TRANSWALL) 
		 return 1;
	 if (!(hitQuery.flags & FQ_TRANSPOINT))
		 return 0;
	if (pSeg->CheckForTranspPixel (vHitPoint, nSide, nFace))
		return 1;
	}

// check whether side can be passed through due to a cheat
if (gameStates.app.cheats.bPhysics != 0xBADA55)
	return 0;
if (hitQuery.nObject != LOCALPLAYER.nObject) 
	return 0;
int16_t nChildSeg = pSeg->m_children [nSide];
if (nChildSeg < 0)
	return 0;
CSegment* pChildSeg = SEGMENT (nChildSeg);
if (!pChildSeg)
	return 0;
if (pChildSeg->HasBlockedProp () ||
    (gameData.objData.speedBoost [hitQuery.nObject].bBoosted && ((pSeg->m_function != SEGMENT_FUNC_SPEEDBOOST) || (pChildSeg->m_function == SEGMENT_FUNC_SPEEDBOOST))))
	return 1;

return 0;
}

//	-----------------------------------------------------------------------------

static inline int32_t CopySegList (int16_t* dest, int16_t nDestLen, int16_t* src, int16_t nSrcLen, int32_t flags)
{
if (flags & FQ_GET_SEGLIST) {
	int32_t i = MAX_FVI_SEGS - 1 - nDestLen;
	if (i > nSrcLen)
		i = nSrcLen;
	memcpy (dest + nDestLen, src, i * sizeof (*dest));
	return i;
	}
return 0;
}

//	-----------------------------------------------------------------------------

#define FVI_NEWCODE 2

int32_t ComputeHitpoint (CHitData& hitData, CHitQuery& hitQuery, int16_t* segList, int16_t* nSegments, int32_t nEntrySeg, int32_t nCollisionModel, int32_t nThread)
{
ENTER (1, nThread);

	CHitData		bestHit, curHit;
	fix			d, dMin = 0x7fffffff;					//distance to hit pRef
	int32_t		nHitNoneSegment = -1;
	int16_t		hitNoneSegList [MAX_FVI_SEGS];
	int16_t		nHitNoneSegs = 0;
	int32_t		nCurNestLevel = gameData.collisionData.hitResult.nNestCount;
	bool			bCheckVisibility = ((hitQuery.flags & FQ_VISIBILITY) != 0);

bestHit.vPoint.SetZero ();
//PrintLog (1, "Entry ComputeHitpoint\n");
if (hitQuery.flags & FQ_GET_SEGLIST)
	*segList = hitQuery.nSegment;
*nSegments = 1;
gameData.collisionData.hitResult.nNestCount++;
//first, see if vector hit any objects in this CSegment
#if 1
if (hitQuery.flags & FQ_CHECK_OBJS) {
	//PrintLog (1, "checking objects...");
	dMin = ComputeObjectHitpoint (bestHit, hitQuery, nCollisionModel, nThread);
	}
#endif

CSegment* pSeg = SEGMENT (hitQuery.nSegment);
if ((hitQuery.nObject > -1) && (gameData.objData.collisionResult [OBJECT (hitQuery.nObject)->info.nType][OBJ_WALL] == RESULT_NOTHING))
	hitQuery.radP1 = 0;		//HACK - ignore when edges hit walls
//now, check segment walls
#if DBG
if (hitQuery.nSegment == nDbgSeg)
	BRP;
#endif
int32_t startMask = pSeg->Masks (*hitQuery.p0, hitQuery.radP0).m_face;
CSegMasks masks = pSeg->Masks (*hitQuery.p1, hitQuery.radP1);    //on back of which faces?
int32_t centerMask = masks.m_center;
int32_t endMask = masks.m_face;

if (!centerMask)
	nHitNoneSegment = hitQuery.nSegment;
if (endMask) { //on the back of at least one face
	int16_t nSide, iFace, bit;

	//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (endMask >= bit); nSide++) {
		int32_t nChildSeg = pSeg->m_children [nSide];
#if 0
		if (bCheckVisibility && (0 > nChildSeg))	// poking through a wall into the void around the level?
			continue;
#endif
		int32_t nFaces = pSeg->Side (nSide)->m_nFaces;
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
				BRP;
#endif
			int32_t nFaceHitType = (startMask & bit)	//start was also though.  Do extra check
										  ? pSeg->CheckLineToFaceEdges (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace)
										  : pSeg->CheckLineToFaceRegular (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace);
#if 1
			if (bCheckVisibility && !nFaceHitType)
					continue;
#endif
#if DBG
			if ((hitQuery.nSegment == nDbgSeg) && ((nDbgSide < 0) || (nDbgSide == nSide))) {
				BRP;
				if (nFaceHitType)
					nFaceHitType = (startMask & bit)	//start was also though.  Do extra check
										? pSeg->CheckLineToFaceEdges (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace)
										: pSeg->CheckLineToFaceRegular (curHit.vPoint, hitQuery.p0, hitQuery.p1, hitQuery.radP1, nSide, iFace);
				}
#endif
			if (PassThrough (hitQuery, nSide, iFace, curHit.vPoint)) {
				int32_t		i;
				int16_t		subSegList [MAX_FVI_SEGS];
				int16_t		nSubSegments;
				CHitData		subHit, saveHitData = gameData.collisionData.hitResult;
				CHitQuery	subHitQuery = hitQuery;

				subHitQuery.nSegment = pSeg->m_children [nSide];
#if DBG
				if (subHitQuery.nSegment == nDbgSeg)
					BRP;
#endif
				for (i = 0; i < gameData.collisionData.nSegsVisited [nThread] && (subHitQuery.nSegment != gameData.collisionData.segsVisited [nThread][i]); i++)
					;
				if (i == gameData.collisionData.nSegsVisited [nThread]) {                //haven't visited here yet
					if (gameData.collisionData.nSegsVisited [nThread] >= MAX_SEGS_VISITED)
						goto hitPointDone;	//we've looked a long time, so give up
					gameData.collisionData.segsVisited [nThread][gameData.collisionData.nSegsVisited [nThread]++] = subHitQuery.nSegment;
					subHit.nType = ComputeHitpoint (subHit, subHitQuery, subSegList, &nSubSegments, hitQuery.nSegment, nCollisionModel, nThread);
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
							*((CHitData *) &gameData.collisionData.hitResult) = saveHitData;    
 							}
						}
					else {
						*((CHitData *) &gameData.collisionData.hitResult) = saveHitData;    
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
						curHit.vNormal = pSeg->m_sides [nSide].m_normals [iFace];
						if (pSeg->Masks (curHit.vPoint, hitQuery.radP1).m_center) 
							gameData.collisionData.hitResult.nAltSegment = hitQuery.nSegment;
						else {
#if 0
							CFixVector vMoved = *hitQuery.p1 - *hitQuery.p0;
							CFixVector::Normalize (vMoved);
							if (CFixVector::Dot (curHit.vNormal, vMoved) > 0) { // both points on same side of face and moving away from face
								nFaceHitType = 0;
								continue; 
								}
#endif
							bestHit.nSegment = hitQuery.nSegment;             
							}
						dMin = d;
						bestHit = curHit;
						bestHit.nType = HIT_WALL;
						bestHit.vNormal = curHit.vNormal;
						gameData.collisionData.hitResult.nNormals = 1;
						gameData.collisionData.hitResult.nSegment = bestHit.nSegment;
						gameData.collisionData.hitResult.nSide = nSide;
						gameData.collisionData.hitResult.nFace = iFace;
						gameData.collisionData.hitResult.nSideSegment = hitQuery.nSegment;
						gameData.collisionData.hitResult.vNormal = bestHit.vNormal;
						}
					}
				}
			}
		}
	}

hitPointDone:

if (bestHit.nType == HIT_NONE) {     //didn't hit anything, return end pRef
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
		hitData.nSegment = 
			(gameData.collisionData.hitResult.nAltSegment != -1) 
			? gameData.collisionData.hitResult.nAltSegment
			: nHitNoneSegment;
	}
RETVAL (bestHit.nType);
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
int32_t FindHitpoint (CHitQuery& hitQuery, CHitResult& hitResult, int32_t nCollisionModel, int32_t nThread)
{
ENTER (1, nThread);

	CHitData		curHit, newHit;
	int32_t		i, nHitboxes = extraGameInfo [IsMultiGame].nHitboxes; // save value

gameData.collisionData.hitResult.nAltSegment = -1;
gameData.collisionData.hitResult.vNormal.SetZero ();
gameData.collisionData.hitResult.nNormals = 0;
if ((hitQuery.nSegment > gameData.segData.nLastSegment) || (hitQuery.nSegment < 0)) {
	hitQuery.nSegment = FindSegByPos (*hitQuery.p0, -1, 1, 0);
	if (hitQuery.nSegment < 0) {
		hitResult.nType = HIT_BAD_P0;
		hitResult.vPoint = *hitQuery.p0;
		hitResult.nSegment = hitQuery.nSegment;
		hitResult.nSide = 0;
		hitResult.nObject = 0;
		hitResult.nSideSegment = -1;
		hitResult.nSegments = 0;
		RETVAL (hitResult.nType);
		}
	}

gameData.collisionData.hitResult.nSegment = -1;
gameData.collisionData.hitResult.nSide = -1;
gameData.collisionData.hitResult.nObject = -1;

//check to make sure start pRef is in seg its supposed to be in
//Assert(check_point_in_seg(p0, startseg, 0).m_center==0);	//start pRef not in seg

// gameData.objData.pViewer is not in CSegment as claimed, so say there is no hit.
CSegMasks masks = SEGMENT (hitQuery.nSegment)->Masks (*hitQuery.p0, 0);
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
		RETVAL (hitResult.nType);
		}
	}
gameData.collisionData.segsVisited [nThread][0] = hitQuery.nSegment;
gameData.collisionData.nSegsVisited [nThread] = 1;
gameData.collisionData.hitResult.nNestCount = 0;
if (hitQuery.flags & FQ_VISIBILITY)
	extraGameInfo [IsMultiGame].nHitboxes = 0;
ComputeHitpoint (curHit, hitQuery, hitResult.segList, &hitResult.nSegments, -2, nCollisionModel, nThread);
#if 1 //DBG
if (curHit.nSegment >= gameData.segData.nSegments) {
	PrintLog (0, "Invalid hit segment in collision detection\n");
	ComputeHitpoint (curHit, hitQuery, hitResult.segList, &hitResult.nSegments, -2, nCollisionModel, nThread);
	if (curHit.nSegment > gameData.segData.nSegments) 
		curHit.nSegment = -1;
	}
#endif
extraGameInfo [IsMultiGame].nHitboxes = nHitboxes;

if ((curHit.nSegment < 0) || SEGMENT (curHit.nSegment)->Masks (curHit.vPoint, 0).m_center)
	curHit.nSegment = FindSegByPos (curHit.vPoint, hitQuery.nSegment, 1, 0);

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
if ((curHit.nType == HIT_WALL) && (curHit.nSegment == -1))
	if ((gameData.collisionData.hitResult.nAltSegment != -1) && !SEGMENT (gameData.collisionData.hitResult.nAltSegment)->Masks (curHit.vPoint, 0).m_center)
		curHit.nSegment = gameData.collisionData.hitResult.nAltSegment;

if (curHit.nSegment == -1) {
	CHitData altHit;
	CHitQuery altQuery = hitQuery;
	altQuery.radP0 = altQuery.radP1 = 0;

	//because the code that deals with objects with non-zero radius has
	//problems, try using zero radius and see if we hit a wall
	if (hitQuery.flags & FQ_VISIBILITY)
		extraGameInfo [IsMultiGame].nHitboxes = 0;
	ComputeHitpoint (altHit, altQuery, hitResult.segList, &hitResult.nSegments, -2, nCollisionModel, nThread);
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
*((CHitInfo *) &hitResult) = gameData.collisionData.hitResult;
*((CHitData *) &hitResult) = curHit;
if ((curHit.nType == HIT_OBJECT) && (gameData.collisionData.hitResult.nObject == -1))
	curHit.nType = HIT_NONE;
RETVAL (curHit.nType);
}

//	-----------------------------------------------------------------------------
//new function for Mike
//note: gameData.collisionData.nSegsVisited [nThread] must be set to zero before this is called
int32_t SphereIntersectsWall (CFixVector *vPoint, int16_t nSegment, fix rad)
{
ENTER (2, 0);
CSegment *pSeg = SEGMENT (nSegment);
if (!pSeg) {
	PrintLog (0, "Error: Invalid segment in SphereIntersectsWall ()\n");
	RETVAL (0)
	}

if ((gameData.collisionData.nSegsVisited [0] < 0) || (gameData.collisionData.nSegsVisited [0] > MAX_SEGS_VISITED))
	gameData.collisionData.nSegsVisited [0] = 0;
gameData.collisionData.segsVisited [0][gameData.collisionData.nSegsVisited [0]++] = nSegment;

int32_t faceMask = pSeg->Masks (*vPoint, rad).m_face;

if (faceMask != 0) {				//on the back of at least one face
	int32_t		nSide, bit, iFace, nChild, i;
	int32_t		nFaceHitType;      //in what way did we hit the face?

//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (faceMask >= bit); nSide++) {
		for (iFace = 0; iFace < 2; iFace++, bit <<= 1) {
			if (faceMask & bit) {            //on the back of this iFace
				//did we go through this CWall/door?
				nFaceHitType = pSeg->SphereToFaceRelation (*vPoint, rad, nSide, iFace);
				if (nFaceHitType) {            //through this CWall/door
					//if what we have hit is a door, check the adjoining pSeg
					nChild = pSeg->m_children [nSide];
					for (i = 0; (i < gameData.collisionData.nSegsVisited [0]) && (nChild != gameData.collisionData.segsVisited [0][i]); i++)
						;
					if (i == gameData.collisionData.nSegsVisited [0]) {                //haven't visited here yet
						if (!IS_CHILD (nChild))
							RETVAL (1)
						if (SphereIntersectsWall (vPoint, nChild, rad))
							RETVAL (1)
						}
					}
				}
			}
		}
	}
RETVAL (0)
}

//	-----------------------------------------------------------------------------
//Returns true if the CObject is through any walls
int32_t ObjectIntersectsWall (CObject *pObj)
{
#if DBG
if (pObj->Index () == nDbgObj)
	BRP;
#endif
ENTER (2, 0);
RETVAL (SphereIntersectsWall (&pObj->info.position.vPos, pObj->info.nSegment, pObj->info.xSize))
}

//------------------------------------------------------------------------------
// Check whether point p1 from segment nDestSeg can be seen from point p0 located in segment nStartSeg.

#define CHECK_FACE_ORIENT 1

int32_t PointSeesPoint (CFloatVector* p0, CFloatVector* p1, int16_t nStartSeg, int16_t nDestSeg, int8_t nDestSide, int32_t nDepth, int32_t nThread)
{
ENTER (1, nThread);

#if 0

return gameData.segData.faceGrid.PointSeesPoint (*p0, *p1, nDestSeg, nDestSide);

#else

	static			uint32_t segVisList [MAX_THREADS][MAX_SEGMENTS_D2X];
	static			uint32_t segVisFlags [MAX_THREADS] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

	CFloatVector	vIntersection, v0, v1;
	float				l0, l1 = 0.0f;
	int16_t			nSide, nFace, nFaceCount, nChildSeg;
	uint32_t			*bVisited = segVisList [nThread];

if (!nDepth) {
#if 0
	memset (bVisited, 0, gameData.segData.nSegments * sizeof (*bVisited));
#else
#if DBG
	if ((nDbgSeg >= 0) && (nStartSeg == nDbgSeg))
		BRP;
#endif
	if (!++segVisFlags [nThread]) {
		++segVisFlags [nThread];
		memset (bVisited, 0, gameData.segData.nSegments * sizeof (*bVisited));
		}
#endif
	}

	uint32_t			bFlag = segVisFlags [nThread];

#if CHECK_FACE_ORIENT
CFloatVector vRay = *p1 - *p0;
CFloatVector::Normalize (vRay);
#endif

for (;;) {
	CSegment *pSeg = SEGMENT (nStartSeg);
	if (!pSeg)
		RETVAL (0)
	bVisited [nStartSeg] = bFlag;
	CSide *pSide = pSeg->Side (0);
	CWall *pWall;
	// check all sides of current segment whether they are penetrated by the vector p0,p1.
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++, pSide++) {
		nChildSeg = pSeg->m_children [nSide];
		if (nChildSeg < 0) {
			if ((nDestSeg >= 0) && (nStartSeg != nDestSeg))
				continue;
			}
		else {
			if (bVisited [nChildSeg] == bFlag)
				continue;
			}
		uint16_t* vertices = pSide->m_vertices;
		nFaceCount = pSide->m_nFaces;
		for (nFace = 0; nFace < nFaceCount; nFace++, vertices += 3) {
			if (!(nFace && pSide->IsQuad ())) {
				CFloatVector* n = pSide->m_fNormals + nFace;
#if DBG
				if ((nDbgSeg >= 0) && (nStartSeg == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
					BRP;
#endif
#if CHECK_FACE_ORIENT
				if (CFloatVector::Dot (vRay, *n) >= 0.0f) {
#	if DBG
					CFloatVector vRef;
					vRef.Assign (pSeg->Center () - pSide->Center ());
					CFloatVector::Normalize (vRef);
					float dot = CFloatVector::Dot (vRef, *n);
					if (dot < 0.0f)
						BRP;
					else
#	endif
					continue;
					}
#endif
				if (!FindPlaneLineIntersection (vIntersection, &FVERTICES [*vertices], n, p0, p1))
					continue;
				v0 = *p0 - vIntersection;
				v1 = *p1 - vIntersection;
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
				BRP;
#endif
			if (PointIsOutsideFace (&vIntersection, vertices, 5 - nFaceCount))
				continue;
			if (l1 >= 0.001f) 
				break;
			if (l1 < 1.0e-10f)
				RETVAL (1)
			// end point lies in this face
			if (nDestSeg < 0)
				RETVAL (1) // any segment acceptable
			if (nStartSeg == nDestSeg)
				RETVAL (1) // point is in desired segment
			if ((nChildSeg == nDestSeg) && !((pWall = pSide->Wall ()) && !(pWall->IsVolatile () || (pWall->IsPassable (NULL, false) & WID_TRANSPARENT_FLAG))))
				RETVAL (1) // point at border to destination segment and the portal to that segment is passable
			nFace = nFaceCount; // no eligible child segment, so try next segment side
			break; 
			}
		if (nFace == nFaceCount)
			continue; // line doesn't intersect with this side
		if (0 > nChildSeg) // solid wall
			continue;
		if ((pWall = pSide->Wall ()) && !(pWall->IsVolatile () || (pWall->IsPassable (NULL, false) & WID_TRANSPARENT_FLAG))) // impassable
			continue;
		if (PointSeesPoint (p0, p1, nChildSeg, nDestSeg, nDestSide, nDepth + 1, nThread))
			RETVAL (1)
		}
#if DBG
	if (!nDepth)
		BRP;
#endif
	RETVAL ((nDestSeg < 0) || (nStartSeg == nDestSeg)) // line doesn't intersect any side of this segment -> p1 must be inside segment
	}

#endif

}

//	-----------------------------------------------------------------------------

int32_t UseSphere (CObject *pObj)
{
	int32_t nType = pObj->info.nType;

return gameStates.app.bNostalgia || (nType == OBJ_MONSTERBALL) || (nType == OBJ_HOSTAGE) || (nType == OBJ_POWERUP) || pObj->IsMine ();
}

//	-----------------------------------------------------------------------------

//eof
