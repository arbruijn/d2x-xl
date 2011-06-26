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

ubyte PointIsInTriangle (CFixVector* vRef, CFixVector vNormal, CFixVector* vertices)
{
CFloatVector v0, v1, v2;
v0.Assign (vertices [2] - vertices [0]);
v1.Assign (vertices [1] - vertices [0]);
v2.Assign (*vRef - vertices [0]);
float dot00 = CFloatVector::Dot (v0, v0);
float dot11 = CFloatVector::Dot (v1, v1);
float dot01 = CFloatVector::Dot (v0, v1);
float dot02 = CFloatVector::Dot (v0, v2);
float dot12 = CFloatVector::Dot (v1, v2);
float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
return (u >= -0.001f) && (v >= -0.001f) && (u + v <= 1.001f);
}

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d
bool PointIsInQuad (CFixVector point, CFixVector* vertP, CFixVector vNormal)
{
#if 0
return PointIsInTriangle (&point, vNormal, vertP) || PointIsInTriangle (&point, vNormal, vertP + 1);
#else
	CFixVector		t, v0, v1;
	CFixVector2 	vEdge, vCheck, vRef;
	int 				i, j, iEdge, projPlane;

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
vRef.x = point.v.vec [i];
vRef.y = point.v.vec [j];
v1 = vertP [0];
for (iEdge = 1; iEdge <= 4; iEdge++) {
	v0 = v1;
	v1 = vertP [iEdge % 4];
	vEdge.x = v1.v.vec [i] - v0.v.vec [i];
	vEdge.y = v1.v.vec [j] - v0.v.vec [j];
	vCheck.x = vRef.x - v0.v.vec [i];
	vCheck.y = vRef.y - v0.v.vec [j];
	if (FixMul (vCheck.x, vEdge.y) - FixMul (vCheck.y, vEdge.x) < 0)
		return false;
	}
return true;
#endif
}

//	-----------------------------------------------------------------------------

fix DistToQuad (CFixVector vRef, CFixVector* vertP, CFixVector vNormal)
{
// compute intersection of perpendicular through vRef with the plane spanned up by the face
if (PointIsInQuad (vRef, vertP, vNormal))
	return 0;

	CFixVector	v;
	fix			dist, minDist = 0x7fffffff;

for (int i = 0; i < 4; i++) {
	FindPointLineIntersection (v, vertP [i], vertP [(i + 1) % 4], vRef, 0);
	dist = CFixVector::Dist (vRef, v);
	if (minDist > dist)
		minDist = dist;
	}
return minDist;
}

//	-----------------------------------------------------------------------------
// Given: p3
// Find: intersection with p1,p2 of the line through p3 that is perpendicular on p1,p2

int FindPointLineIntersectionf (CFixVector* pv1, CFixVector* pv2, CFixVector* pv3)
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

//	-----------------------------------------------------------------------------
// find the point on the specified plane where the line intersects
// returns true if intersection found, false if line parallel to plane
// intersection is the found intersection on the plane
// vPlanePoint & vPlaneNorm describe the plane
// p0 & p1 are the ends of the line

int FindLineQuadIntersectionSub (CFixVector& intersection, CFixVector* vPlanePoint, CFixVector* vPlaneNorm, CFixVector* p0, CFixVector* p1, fix rad)
{
	CFixVector	d, w;
	double		num, den;

w = *vPlanePoint - *p0;
d = *p1 - *p0;
num = double (CFixVector::Dot (*vPlaneNorm, w) - rad) / 65536.0;
den = double (CFixVector::Dot (*vPlaneNorm, d)) / 65536.0;
if (fabs (den) < 1e-10)
	return 0;
if (fabs (num) > fabs (den))
	return 0;
#if 0
//do check for potential overflow
if (labs (num) / (I2X (1) / 2) >= labs (den))
return 0;
#endif
#if 0
d *= FixDiv (num, den);
intersection = (*p0) + d;
#else
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

int CheckLineHitsQuad (CFixVector& intersection, CFixVector* planeP)
{
for (int i = 0; i < 4; i++)
	if (FindPointLineIntersectionf (planeP + i, planeP + ((i + 1) % 4), &intersection))
		return 0;	//doesn't hit
return 1;	//hits
}

//	-----------------------------------------------------------------------------

int FindLineQuadIntersection (CFixVector& intersection, CFixVector* planeP, CFixVector* planeNormP, CFixVector* p0, CFixVector* p1, fix rad)
{
	CFixVector	vHit;
	fix			dist;

#if 1
if (CFixVector::Dot (*p1 - *p0, *planeNormP) > 0)
	return 0x7fffffff;	// hit back of face
#endif
if (!FindLineQuadIntersectionSub (vHit, planeP, planeNormP, p0, p1, 0))
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

int FindQuadQuadIntersectionSub (CFixVector& intersection, CFixVector& normal, CFixVector* p1, CFixVector* vn1, CFixVector* p2, CFixVector* vn2, CFixVector* vRef, fix& dMin)
{
	int			i, nHits = 0;
	CFixVector	vHit;

for (i = 0; i < 4; i++)
	if (FindLineQuadIntersection (vHit, p2, vn2, p1 + i, p1 + ((i + 1) % 4), 0) < 0x7fffffff) 
		nHits += RegisterHit (&intersection, &normal, &vHit, vn2, vRef, dMin);
return nHits;
}

//	-----------------------------------------------------------------------------

int FindQuadQuadIntersection (CFixVector& intersection, CFixVector& normal, CFixVector* p0, CFixVector* vn1, CFixVector* p1, CFixVector* vn2, CFixVector* vRef, fix& dMin)
{
// test whether any edges of p0 penetrate p1
return FindQuadQuadIntersectionSub (intersection, normal, p0, vn1, p1, vn2, vRef, dMin) + 
		 FindQuadQuadIntersectionSub (intersection, normal, p1, vn2, p0, vn1, vRef, dMin);
}

//	-----------------------------------------------------------------------------

int FindLineHitboxIntersection (CFixVector& intersection, CFixVector& normal, tBox *phb, CFixVector* p0, CFixVector* p1, CFixVector* vRef, fix rad, fix& dMin)
{
	int			i, nHits = 0;
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

int FindHitboxIntersection (CFixVector& intersection, CFixVector& normal, tBox *phb1, tBox *phb2, CFixVector* vRef, fix& dMin)
{
	int			i, j, nHits = 0;
	tQuad			*pf1, *pf2;

// create all faces of hitbox 2 and their normals before testing because they will
// be used multiple times
for (i = 0, pf1 = phb1->faces; i < 6; i++, pf1++) {
	for (j = 0, pf2 = phb2->faces; j < 6; j++, pf2++) {
#if 0
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

fix CheckHitboxCollision (CFixVector& intersection, CFixVector& normal, CObject *objP1, CObject *objP2, CFixVector* p0, CFixVector* p1, short& nModel)
{
	CFixVector		vRef = OBJPOS (objP2)->vPos;
	int				iModel1, nModels1, iModel2, nModels2, nHits, nTotalHits = 0;
	CModelHitboxes	*pmhb1 = gameData.models.hitboxes + objP1->ModelId ();
	CModelHitboxes	*pmhb2 = gameData.models.hitboxes + objP2->ModelId ();
	tBox				hb1 [MAX_HITBOXES + 1];
	tBox				hb2 [MAX_HITBOXES + 1];
	fix				dMin = 0x7fffffff;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel1 =
	nModels1 =
	iModel2 =
	nModels2 = 0;
	}
else {
	iModel1 =
	iModel2 = 1;
	nModels1 = pmhb1->nHitboxes;
	nModels2 = pmhb2->nHitboxes;
	}
#if DBG
memset (hb1, 0, sizeof (hb1));
memset (hb2, 0, sizeof (hb2));
#endif
TransformHitboxes (objP1, p1, hb1);
TransformHitboxes (objP2, &vRef, hb2);

int i, j;
for (i = iModel1; i <= nModels1; i++) {
	for (j = iModel2; j <= nModels2; j++) {
		if ((nHits = FindHitboxIntersection (intersection, normal, hb1 + i, hb2 + j, p0, dMin))) {
			nTotalHits += nHits;
			nModel = iModel1;
			}
		}
	}
if (!nHits) {
	for (j = iModel2; j <= nModels2; j++) {
		if ((nHits = FindLineHitboxIntersection (intersection, normal, hb2 + j, p0, p1, p0, 0, dMin))) {
			nTotalHits += nHits;
			nModel = iModel1;
			}
		}
	}
#if DBG
if (nTotalHits) {
	pmhb1->vHit = pmhb2->vHit = intersection;
	pmhb1->tHit = pmhb2->tHit = gameStates.app.nSDLTicks [0];
	}
#endif
return (nTotalHits) ? dMin ? dMin : 1 : 0;
}

//	-----------------------------------------------------------------------------

fix CheckVectorHitboxCollision (CFixVector& intersection, CFixVector& normal, CFixVector* p0, CFixVector* p1, CFixVector* vRef, CObject *objP, fix rad, short& nModel)
{
	int				iModel, nModels;
	fix				dMin = 0x7fffffff;
	CModelHitboxes	*pmhb = gameData.models.hitboxes + objP->ModelId ();
	tBox				hb [MAX_HITBOXES + 1];

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = pmhb->nHitboxes;
	}
if (!vRef)
	vRef = &OBJPOS (objP)->vPos;
intersection.Create (0x7fffffff, 0x7fffffff, 0x7fffffff);
TransformHitboxes (objP, vRef, hb);
for (; iModel <= nModels; iModel++) {
	if (FindLineHitboxIntersection (intersection, normal, hb + iModel, p0, p1, p0, rad, dMin)) 
		nModel = iModel;
	}
return dMin;
}

//	-----------------------------------------------------------------------------
//eof
