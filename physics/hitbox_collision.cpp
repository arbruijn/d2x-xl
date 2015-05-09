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

//	-----------------------------------------------------------------------------

uint8_t PointIsInTriangle (CFixVector* vRef, CFixVector* vNormal, uint16_t* triangleVerts)
{
CFloatVector v0, v1, v2;
v0 = FVERTICES [triangleVerts [2]] - FVERTICES [triangleVerts [0]];
v1 = FVERTICES [triangleVerts [1]] - FVERTICES [triangleVerts [0]];
v2.Assign (*vRef);
v2 -= FVERTICES [triangleVerts [0]];
float dot00 = CFloatVector::Dot (v0, v0);
float dot11 = CFloatVector::Dot (v1, v1);
float dot01 = CFloatVector::Dot (v0, v1);
float dot02 = CFloatVector::Dot (v0, v2);
float dot12 = CFloatVector::Dot (v1, v2);
float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
if (u < -0.001f)
	return 0;
float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
if (v < -0.001f)
	return 0;
return (u + v <= 1.001f);
}

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d

static bool PointIsInQuad (CFixVector point, CFixVector* pVertex, CFixVector vNormal)
{
#if 0
return PointIsInTriangle (&point, vNormal, pVertex) || PointIsInTriangle (&point, vNormal, pVertex + 1);
#else
	CFixVector		t;
	int32_t 			i, j, projPlane;

//now do 2d check to see if vRef is in side
//project polygon onto plane by finding largest component of Normal
t.Set (labs (vNormal.v.coord.x), labs (vNormal.v.coord.y), labs (vNormal.v.coord.z));
if (t.v.coord.x > t.v.coord.y)
	projPlane = (t.v.coord.x > t.v.coord.z) ? 0 : 2;
else
	projPlane = (t.v.coord.y > t.v.coord.z) ? 1 : 2;
if (vNormal.v.vec [projPlane] > 0) {
	i = ijTable [projPlane][0];
	j = ijTable [projPlane][1];
	}
else {
	i = ijTable [projPlane][1];
	j = ijTable [projPlane][0];
	}
//now do the 2d problem in the i, j plane
CFixVector2 vRef (point.v.vec [i], point.v.vec [j]);
CFixVector* v1 = pVertex;
for (int32_t iEdge = 1; iEdge <= 4; iEdge++) {
	CFixVector* v0 = v1;
	v1 = pVertex + iEdge % 4;
	CFixVector2 vEdge (v1->v.vec [i] - v0->v.vec [i], v1->v.vec [j] - v0->v.vec [j]);
	CFixVector2 vCheck (vRef.m_x - v0->v.vec [i], vRef.m_y - v0->v.vec [j]);
	if (FixMul (vCheck.m_x, vEdge.m_y) - FixMul (vCheck.m_y, vEdge.m_x) < 0)
		return false;
	}
return true;
#endif
}

//	-----------------------------------------------------------------------------

static fix DistToQuad (CFixVector vRef, CFixVector* pVertex, CFixVector vNormal)
{
// compute intersection of perpendicular through vRef with the plane spanned up by the face
if (PointIsInQuad (vRef, pVertex, vNormal))
	return 0;

	CFixVector	v;
	fix			dist, minDist = 0x7fffffff;

for (int32_t i = 0; i < 4; i++) {
	FindPointLineIntersection (v, pVertex [i], pVertex [(i + 1) % 4], vRef, 0);
	dist = CFixVector::Dist (vRef, v);
	if (minDist > dist)
		minDist = dist;
	}
return minDist;
}

//	-----------------------------------------------------------------------------
// Given: p3
// Find: intersection with p1,p2 of the line through p3 that is perpendicular on p1,p2

#if 0
static int32_t FindPointLineIntersectionf (CFixVector* pv1, CFixVector* pv2, CFixVector* pv3)
{
	CFloatVector	p1, p2, p3, d31, d21, h, v [2];
	float				m, u;

p1.Assign (*pv1);
p2.Assign (*pv2);
p3.Assign (*pv3);
d21 = p2 - p1;
if (!(m = d21.v.coord.x * d21.v.coord.x + d21.v.coord.y * d21.v.coord.y + d21.v.coord.z * d21.v.coord.z))
	return 0;
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
h = p1 + d21 * u;
// limit the intersection to [p1,p2]
v [0] = p1 - h;
v [1] = p2 - h;
m = CFloatVector::Dot (v [0], v [1]);
if (m >= 1)
	return 1;
return 0;
}
#endif

//	-----------------------------------------------------------------------------
// find the point on the specified plane where the line intersects
// returns true if intersection found, false if line parallel to plane
// intersection is the found intersection on the plane
// vPlanePoint & vPlaneNorm describe the plane
// p0 & p1 are the ends of the line

static int32_t FindLinePlaneIntersection (CFixVector& intersection, CFixVector* vPlane, CFixVector* vNormal, CFixVector* p0, CFixVector* p1, fix rad)
{
#if 1
	CFloatVector n, u, w;

u.Assign (*p1 - *p0);
n.Assign (*vNormal);
float den = -CFloatVector::Dot (n, u);
if ((den > -1e-6f) && (den < 1e-6f)) {// ~ parallel
	return 0;
	}
w.Assign (*p0 - *vPlane);
float num = CFloatVector::Dot (n, w);
float s = num / den;
if (s < 0.0f) {
	if (s < -0.001f) // compensate small numerical errors
		return 0;
	s = 0.0f;
	}
else if (s > 1.0f) {
	if (s > 1.001f) // compensate small numerical errors
		return 0;
	s = 1.0f;
	}
u *= s;
intersection.Assign (u);
intersection += *p0;
#else
CFixVector d = *p1 - *p0;
double den = double (CFixVector::Dot (*vNormal, d)) / 65536.0;
if (fabs (den) < 1e-10)
	return 0;
CFixVector w = *p0 - *vPlane;
double num = double (CFixVector::Dot (*vNormal, w) - rad) / 65536.0;
if (fabs (num) > fabs (den))
	return 0;
num /= den;
intersection.v.coord.x = fix (double (p0->v.coord.x) + double (d.v.coord.x) * num);
intersection.v.coord.y = fix (double (p0->v.coord.y) + double (d.v.coord.y) * num);
intersection.v.coord.z = fix (double (p0->v.coord.z) + double (d.v.coord.z) * num);
#endif
return 1;
}

//	-----------------------------------------------------------------------------
// if intersection is inside the rectangular (!) quad planeP, the perpendicular from intersection to each edge
// of the quad must hit each edge between the edge's end points (provided vHit
// is in the quad's plane).

#if 0
static int32_t CheckLineHitsQuad (CFixVector& intersection, CFixVector* planeP)
{
for (int32_t i = 0; i < 4; i++)
	if (FindPointLineIntersectionf (planeP + i, planeP + ((i + 1) % 4), &intersection))
		return 0;	//doesn't hit
return 1;	//hits
}
#endif

//	-----------------------------------------------------------------------------

static int32_t FindLineQuadIntersection (CFixVector& intersection, CFixVector* planeP, CFixVector* planeNormP, CFixVector* p0, CFixVector* p1, fix rad)
{
	CFixVector	vHit;
	fix			dist;

#if 0
if (CFixVector::Dot (*p1 - *p0, *planeNormP) > 0)
	return 0x7fffffff;	// hit back of face
#endif
if (!FindLinePlaneIntersection (vHit, planeP, planeNormP, p0, p1, 0))
	return 0x7fffffff;
if (!rad && (CFixVector::Dot (vHit - *p0, vHit - *p1) > 0))
	return 0x7fffffff;
dist = DistToQuad (vHit, planeP, *planeNormP);
if (rad < dist)
	return 0x7fffffff;
intersection = vHit;
return dist;
}

//	-----------------------------------------------------------------------------
// Simple intersection check by checking whether any of the edges of plane p1
// penetrate p2. Returns average of all penetration points.

static int32_t FindQuadQuadIntersectionSub (CFixVector& intersection, CFixVector* vQuad, CFixVector* vPlane, CFixVector* vNormal, CFixVector* vRef, fix& dMin)
{
	int32_t			i, nHits = 0;
	CFixVector	vHit;

intersection.SetZero ();
for (i = 0; i < 4; i++)
	if (FindLineQuadIntersection (vHit, vPlane, vNormal, vQuad + i, vQuad + ((i + 1) % 4), 0) < 0x7fffffff) {
#if DBG
		FindLineQuadIntersection (vHit, vPlane, vNormal, vQuad + i, vQuad + ((i + 1) % 4), 0);
#endif
		++nHits;
		intersection += vHit;
		}
if (nHits > 1)
	intersection /= I2X (nHits);
return nHits;
}

//	-----------------------------------------------------------------------------

static int32_t FindQuadQuadIntersection (CFixVector& intersection, CFixVector& normal, CFixVector* p0, CFixVector* vn1, CFixVector* p1, CFixVector* vn2, CFixVector* vRef, fix& dMin)
{
	CFixVector	vHit;
	int32_t			nHits = 0;

// test whether any edges of p0 penetrate p1
if (FindQuadQuadIntersectionSub (vHit, p0, p1, vn2, vRef, dMin))
	nHits += RegisterHit (&intersection, &normal, &vHit, vn2, vRef, dMin);
if (FindQuadQuadIntersectionSub (vHit, p1, p0, vn1, vRef, dMin))
	nHits += RegisterHit (&intersection, &normal, &vHit, vn2, vRef, dMin);
return nHits;
}

//	-----------------------------------------------------------------------------

static int32_t FindLineHitboxIntersection (CFixVector& intersection, CFixVector& normal, tBox *phb, CFixVector* p0, CFixVector* p1, CFixVector* vRef, fix rad, fix& dMin)
{
	int32_t			i, nHits = 0;
	fix			dist;
	tQuad			*pf;
	CFixVector	vHit;

// create all faces of hitbox 2 and their normals before testing because they will
// be used multiple times
for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
	dist = FindLineQuadIntersection (vHit, pf->v, pf->n + 1, p0, p1, rad);
	if (dist < 0x7fffffff)
		nHits += RegisterHit (&intersection, &normal, &vHit, pf->n + 1, vRef, dMin);
	}
return nHits;
}

//	-----------------------------------------------------------------------------

static int32_t FindHitboxIntersection (CFixVector& intersection, CFixVector& normal, tBox *phb1, tBox *phb2, CFixVector* vRef, fix& dMin)
{
	int32_t			i, j, nHits = 0;
	tQuad			*pf1, *pf2;

// create all faces of hitbox 2 and their normals before testing because they will
// be used multiple times
for (i = 0, pf1 = phb1->faces; i < 6; i++, pf1++) {
	for (j = 0, pf2 = phb2->faces; j < 6; j++, pf2++) {
#if 1
		if (CFixVector::Dot (pf1->n [1], pf2->n [1]) >= 0)
			continue;
#endif
		nHits += FindQuadQuadIntersection (intersection, normal, pf1->v, pf1->n + 1, pf2->v, pf2->n + 1, vRef, dMin);
#if DBG
		pf1->t = pf2->t = gameStates.app.nSDLTicks [0];
#endif
		}
	}
return nHits;
}

//	-----------------------------------------------------------------------------

//int32_t DropMarkerObject (CFixVector& vPos, int16_t nSegment, CFixMatrix& orient, uint8_t nMarker);

fix CheckHitboxCollision (CFixVector& intersection, CFixVector& normal, CObject *objP1, CObject *objP2, CFixVector* p0, CFixVector* p1, int16_t& nModel)
{
	CFixVector		vRef = OBJPOS (objP2)->vPos;
	int32_t				iModel1, nModels1, iModel2, nModels2, nHits = 0, nTotalHits = 0;
	CModelHitboxList	*pmhb1 = gameData.models.hitboxes + objP1->ModelId ();
	CModelHitboxList	*pmhb2 = gameData.models.hitboxes + objP2->ModelId ();
	fix				dMin = 0x7fffffff;

if (CollisionModel () == 1) {
	iModel1 =
	nModels1 = 0;
	iModel2 =
	nModels2 = 0;
	}
else {
	iModel1 =
	iModel2 = 1;
	nModels1 = pmhb1->nHitboxes;
	nModels2 = pmhb2->nHitboxes;
	}

tHitbox* hb1 = TransformHitboxes (objP1, p1);
tHitbox* hb2 = TransformHitboxes (objP2, &vRef);

int32_t i, j;
for (i = iModel1; i <= nModels1; i++) {
	for (j = iModel2; j <= nModels2; j++) {
		if ((nHits = FindHitboxIntersection (intersection, normal, &hb1 [i].box, &hb2 [j].box, p0, dMin))) {
			nTotalHits += nHits;
			nModel = iModel1;
			}
		}
	}
if (!nHits) {
	for (j = iModel2; j <= nModels2; j++) {
		if ((nHits = FindLineHitboxIntersection (intersection, normal, &hb2 [j].box, p0, p1, p0, 0, dMin))) {
			nTotalHits += nHits;
			nModel = iModel1;
			}
		}
	}
#if DBG
if (nTotalHits) {
	pmhb1->vHit = pmhb2->vHit = intersection;
	pmhb1->tHit = pmhb2->tHit = gameStates.app.nSDLTicks [0];
#	if 0
	if (gameStates.app.nSDLTicks [0] - tMarker > 3000) {
		if (nMarker >= 0)
			OBJECT (nMarker)->Die ();
		nMarker = DropMarkerObject (intersection, objP1->Segment (), objP1->Orientation (), 0);
		tMarker = gameStates.app.nSDLTicks [0];
		}
#	endif
	}
#endif
return (nTotalHits) ? dMin ? dMin : 1 : 0;
}

//	-----------------------------------------------------------------------------

fix CheckVectorHitboxCollision (CFixVector& intersection, CFixVector& normal, CFixVector* p0, CFixVector* p1, CFixVector* vRef, CObject *pObj, fix rad, int16_t& nModel)
{
	int32_t					iModel, nModels;
	fix					dMin = 0x7fffffff;
	CModelHitboxList*	pmhb = gameData.models.hitboxes + pObj->ModelId ();

if (CollisionModel () == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = pmhb->nHitboxes;
	}
if (!vRef)
	vRef = &OBJPOS (pObj)->vPos;
intersection.Create (0x7fffffff, 0x7fffffff, 0x7fffffff);
tHitbox* hb = TransformHitboxes (pObj, vRef);
for (; iModel <= nModels; iModel++) {
	if (FindLineHitboxIntersection (intersection, normal, &hb [iModel].box, p0, p1, p0, rad, dMin)) 
		nModel = iModel;
	}
return dMin;
}

//	-----------------------------------------------------------------------------

int32_t FindLineTriangleIntersection (CFixVector& intersection, uint16_t* triangleVerts, CFixVector* triangleNormal, CFixVector* p0, CFixVector* p1)
{
	CFixVector	vHit;

#if 0
if (CFixVector::Dot (*p1 - *p0, *planeNormP) > 0)
	return 0x7fffffff;	// hit back of face
#endif
if (!FindLinePlaneIntersection (vHit, VERTICES + *triangleVerts, triangleNormal, p0, p1, 0))
	return 0x7fffffff;
if (!PointIsInTriangle (&vHit, triangleNormal, triangleVerts))
	return 0x7fffffff;
intersection = vHit;
return 0;
}

//	-----------------------------------------------------------------------------
// Simple intersection check by checking whether any of the edges of plane p1
// penetrate p2. Returns average of all penetration points.

int32_t FindTriangleQuadIntersection (CFixVector& intersection, uint16_t* triangleVerts, CFixVector* triangleNormal, CFixVector* vQuad, CFixVector* vRef, fix& dMin)
{
	int32_t			i, nHits = 0;
	CFixVector	vHit;

intersection.SetZero ();
for (i = 0; i < 4; i++)
	if (FindLineTriangleIntersection (vHit, triangleVerts, triangleNormal, vQuad + i, vQuad + ((i + 1) % 4)) < 0x7fffffff) {
#if DBG
	FindLineTriangleIntersection (vHit, triangleVerts, triangleNormal, vQuad + i, vQuad + ((i + 1) % 4));
#endif
		++nHits;
		intersection += vHit;
		}
if (nHits > 1)
	intersection /= I2X (nHits);
return nHits;
}

//	-----------------------------------------------------------------------------

int32_t FindTriangleHitboxIntersection (CFixVector& intersection, CFixVector& normal, uint16_t* triangleVerts, CFixVector* triangleNormal, tBox *phb, CFixVector* vRef, fix& dMin)
{
	int32_t			i, h, nHits = 0;
	tQuad*		pf;
	CFixVector	vHit;

// create all faces of hitbox 2 and their normals before testing because they will
// be used multiple times
for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
	if (CFixVector::Dot (pf->n [1], *triangleNormal) >= 0)
		continue;
	if ((h = FindTriangleQuadIntersection (vHit, triangleVerts, triangleNormal, pf->v, vRef, dMin)) &&
		 RegisterHit (&intersection, &normal, &vHit, triangleNormal, vRef, dMin))
		nHits += h;
	}
return nHits;
}

//	-----------------------------------------------------------------------------

CSegMasks CheckFaceHitboxCollision (CFixVector& intersection, CFixVector& normal, int16_t nSegment, int16_t nSide, CFixVector* p0, CFixVector* p1, CObject *pObj)
{
	int32_t			iModel, nModels, nHits = 0;
	fix			dMin = 0x7fffffff;
	CSegMasks	masks;

#if 1 // always only use the primary hitbox (containing the entire object) here
iModel =
nModels = 1;
#else
CModelHitboxList*	pmhb = gameData.models.hitboxes + pObj->ModelId ();
if (CollisionModel () == 1) {
	iModel =
	nModels = 1;
	}
else {
	iModel = 1;
	nModels = pmhb->nHitboxes;
	}
#endif
if (!p1)
	p1 = &OBJPOS (pObj)->vPos;
intersection.Create (0x7fffffff, 0x7fffffff, 0x7fffffff);
tHitbox* hb = TransformHitboxes (pObj, p1);

CSide* pSide = SEGMENT (nSegment)->Side (nSide);

for (int32_t i = 0; i < 2; i++) {
	for (int32_t j = iModel; j <= nModels; j++) {
		int32_t h = FindTriangleHitboxIntersection (intersection, normal, pSide->m_vertices + 3 * i, pSide->m_normals + i, &hb [j].box, p0, dMin);
		if (h) {
			masks.m_side |= 1 << nSide;
			masks.m_face |= (i + 1) << (nSide * 2);
			nHits += h;
			}
		}
	}
return masks;
}

//	-----------------------------------------------------------------------------

int32_t UseHitbox (CObject *pObj)
{
return !gameStates.app.bNostalgia &&
		 (pObj->info.renderType == RT_POLYOBJ) && (pObj->rType.polyObjInfo.nModel >= 0) && 
		 ((pObj->info.nType != OBJ_WEAPON) || ((pObj->info.nId != GAUSS_ID) && (pObj->info.nId != VULCAN_ID)));
}

//	-----------------------------------------------------------------------------
//eof
