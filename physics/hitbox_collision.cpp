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
#include "gameseg.h"
#include "interp.h"
#include "hitbox.h"
#include "network.h"
#include "renderlib.h"
#include "collision_math.h"

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d
bool PointIsInQuad (CFixVector vRef, CFixVector* vertP, CFixVector vNormal)
{
	CFixVector	t, v0, v1;
	int 			i, j, iEdge, biggest;
	fix			check_i, check_j;
	vec2d 		vEdge, vCheck;

//now do 2d check to see if vRef is in side
//project polygon onto plane by finding largest component of Normal
t [X] = labs (vNormal [0]);
t [Y] = labs (vNormal [1]);
t [Z] = labs (vNormal [2]);
if (t [X] > t [Y]) {
	if (t [X] > t [Z])
		biggest = 0;
	else
		biggest = 2;
	}
else {
	if (t [Y] > t [Z])
		biggest = 1;
	else
		biggest = 2;
	}
if (vNormal [biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = vRef [i];
check_j = vRef [j];
for (iEdge = 0; iEdge < 4; iEdge) {
	v0 = vertP [iEdge++];
	v1 = vertP [iEdge % 4];
	vEdge.i = v1 [i] - v0 [i];
	vEdge.j = v1 [j] - v0 [j];
	vCheck.i = check_i - v0 [i];
	vCheck.j = check_j - v0 [j];
	if (FixMul (vCheck.i, vEdge.j) - FixMul (vCheck.j, vEdge.i) < 0)
		return false;
	}
return true;
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
	VmPointLineIntersection (v, vertP [i], vertP [(i + 1) % 4], vRef, 0);
	dist = CFixVector::Dist (vRef, v);
	if (minDist > dist)
		minDist = dist;
	}
return minDist;
}

//	-----------------------------------------------------------------------------
// Given: p3
// Find: intersection with p1,p2 of the line through p3 that is perpendicular on p1,p2

int FindPointLineIntersectionf (CFixVector *pv1, CFixVector *pv2, CFixVector *pv3)
{
	CFloatVector	p1, p2, p3, d31, d21, h, v [2];
	float				m, u;

p1.Assign (*pv1);
p2.Assign (*pv2);
p3.Assign (*pv3);
d21 = p2 - p1;
if (!(m = d21 [X] * d21 [X] + d21 [Y] * d21 [Y] + d21 [Z] * d21 [Z]))
	return 0;
d31 = p3 - p1;
u = CFloatVector::Dot (d31, d21);
u /= m;
h = p1 + u * d21;
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

int FindLineQuadIntersectionSub (CFixVector& intersection, CFixVector *vPlanePoint, CFixVector *vPlaneNorm,
										   CFixVector *p0, CFixVector *p1, fix rad)
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
intersection [0] = fix (double ((*p0) [0]) + double (d [0]) * num);
intersection [1] = fix (double ((*p0) [1]) + double (d [1]) * num);
intersection [2] = fix (double ((*p0) [2]) + double (d [2]) * num);
#endif
return 1;
}

//	-----------------------------------------------------------------------------
// if intersection is inside the rectangular (!) quad planeP, the perpendicular from intersection to each edge
// of the quad must hit each edge between the edge's end points (provided vHit
// is in the quad's plane).

int CheckLineHitsQuad (CFixVector& intersection, CFixVector *planeP)
{
for (int i = 0; i < 4; i++)
	if (FindPointLineIntersectionf (planeP + i, planeP + ((i + 1) % 4), &intersection))
		return 0;	//doesn't hit
return 1;	//hits
}

//	-----------------------------------------------------------------------------

int FindLineQuadIntersection (CFixVector& intersection, CFixVector *planeP, CFixVector *planeNormP, CFixVector *p0, CFixVector *p1, fix rad)
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

int FindQuadQuadIntersectionSub (CFixVector& intersection, CFixVector *p1, CFixVector *vn1, CFixVector *p2, CFixVector *vn2, CFixVector *vRef)
{
	int			i, nHits = 0;
	fix			dMin = 0x7fffffff;
	CFixVector	vHit;

for (i = 0; i < 4; i++)
	if (FindLineQuadIntersection (vHit, p2, vn2, p1 + i, p1 + ((i + 1) % 4), 0) < 0x7fffffff) {
		dMin = RegisterHit (&intersection, &vHit, vRef, dMin);
		nHits++;
		}
return nHits;
}

//	-----------------------------------------------------------------------------

int FindQuadQuadIntersection (CFixVector& intersection, CFixVector *p0, CFixVector *vn1, CFixVector *p1, CFixVector *vn2, CFixVector *vRef)
{
	CFixVector	vHit;
	int			nHits = 0;
	fix			dMin = 0x7fffffff;

// test whether any edges of p0 penetrate p1
if (FindQuadQuadIntersectionSub (vHit, p0, vn1, p1, vn2, vRef)) {
	dMin = RegisterHit (&intersection, &vHit, vRef, dMin);
	nHits++;
	}
// test whether any edges of p1 penetrate p0
if (FindQuadQuadIntersectionSub (vHit, p1, vn2, p0, vn1, vRef)) {
	dMin = RegisterHit (&intersection, &vHit, vRef, dMin);
	nHits++;
	}
return nHits;
}

//	-----------------------------------------------------------------------------

int FindLineHitboxIntersection (CFixVector& intersection, tBox *phb, CFixVector *p0, CFixVector *p1, CFixVector *vRef, fix rad)
{
	int			i, nHits = 0;
	fix			dist, dMin = 0x7fffffff;
	CFixVector	vHit;
	tQuad			*pf;

// create all faces of hitbox 2 and their normals before testing because they will
// be used multiple times
for (i = 0, pf = phb->faces; i < 6; i++, pf++) {
	dist = FindLineQuadIntersection (vHit, pf->v, pf->n + 1, p0, p1, rad);
	if (dist < 0x7fffffff) {
		dMin = RegisterHit (&intersection, &vHit, vRef, dMin);
		nHits++;
		}
	}
return nHits;
}

//	-----------------------------------------------------------------------------

int FindHitboxIntersection (CFixVector& intersection, tBox *phb1, tBox *phb2, CFixVector *vRef)
{
	int			i, j, nHits = 0;
	fix			dMin = 0x7fffffff;
	CFixVector	vHit;
	tQuad			*pf1, *pf2;

// create all faces of hitbox 2 and their normals before testing because they will
// be used multiple times
for (i = 0, pf1 = phb1->faces; i < 6; i++, pf1++) {
	for (j = 0, pf2 = phb2->faces; j < 6; j++, pf2++) {
#if 1
		if (CFixVector::Dot (pf1->n [1], pf2->n [1]) >= 0)
			continue;
#endif
		if (FindQuadQuadIntersection (vHit, pf1->v, pf1->n + 1, pf2->v, pf2->n + 1, vRef)) {
			dMin = RegisterHit (&intersection, &vHit, vRef, dMin);
			nHits++;
#if DBG
			pf1->t = pf2->t = gameStates.app.nSDLTicks;
#endif
			}
		}
	}
return nHits;
}

//	-----------------------------------------------------------------------------

fix CheckHitboxToHitbox (CFixVector& intersection, CObject *objP1, CObject *objP2, CFixVector *p0, CFixVector *p1)
{
	CFixVector		vHit, vRef = objP2->info.position.vPos;
	int				iModel1, nModels1, iModel2, nModels2, nHits = 0;
	CModelHitboxes	*pmhb1 = gameData.models.hitboxes + objP1->rType.polyObjInfo.nModel;
	CModelHitboxes	*pmhb2 = gameData.models.hitboxes + objP2->rType.polyObjInfo.nModel;
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
for (; iModel1 <= nModels1; iModel1++) {
	for (; iModel2 <= nModels2; iModel2++) {
		if (FindHitboxIntersection (vHit, hb1 + iModel1, hb2 + iModel2, p0)) {
			dMin = RegisterHit (&intersection, &vHit, &vRef, dMin);
			nHits++;
			}
		}
	}
if (!nHits) {
	for (; iModel2 <= nModels2; iModel2++) {
		if (FindLineHitboxIntersection (vHit, hb2 + iModel2, p0, p1, p0, 0)) {
			dMin = RegisterHit (&intersection, &vHit, &vRef, dMin);
			nHits++;
			}
		}
	}
#if DBG
if (nHits) {
	pmhb1->vHit = pmhb2->vHit = intersection;
	pmhb1->tHit = pmhb2->tHit = gameStates.app.nSDLTicks;
	}
#endif
return nHits ? dMin ? dMin : 1 : 0;
}

//	-----------------------------------------------------------------------------

fix CheckVectorToHitbox (CFixVector& intersection, CFixVector *p0, CFixVector *p1, CFixVector *pn, CFixVector *vRef, CObject *objP, fix rad, short& nModel)
{
	int				iModel, nModels;
	fix				xDist = 0x7fffffff, dMin = 0x7fffffff;
	CModelHitboxes	*pmhb = gameData.models.hitboxes + objP->rType.polyObjInfo.nModel;
	tBox				hb [MAX_HITBOXES + 1];
	CFixVector		vHit;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = pmhb->nHitboxes;
	}
if (!vRef)
	vRef = &objP->info.position.vPos;
intersection.Create (0x7fffffff, 0x7fffffff, 0x7fffffff);
TransformHitboxes (objP, vRef, hb);
for (; iModel <= nModels; iModel++) {
#if 1	
	if (FindLineHitboxIntersection (vHit, hb + iModel, p0, p1, p0, rad)) {
		xDist = RegisterHit (&intersection, &vHit, vRef, dMin);
		if (dMin > xDist) {
			dMin = xDist;
			nModel = iModel;
			}
		}
#else
	tQuad				*pf;
	CFixVector		vHit, v;
	fix				h, d;

	for (int i = 0, pf = hb [iModel].faces; i < 6; i++, pf++) {
		h = CheckLineToFace (vHit, p0, p1, rad, pf->v, 4, pf->n + 1);
		if (h) {
			v = vHit - *p0;
			d = CFixVector::Normalize (v);
			if (xDist > d) {
				intersection = vHit;
				if (!(xDist = d))
					return 0;
				}
			}
		}
#endif
	}
return xDist;
}

//	-----------------------------------------------------------------------------
//eof
