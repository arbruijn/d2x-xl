/* $Id: fvi.c, v 1.3 2003/10/10 09:36:35 btb Exp $ */
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

#include "inferno.h"
#include "u_mem.h"
#include "error.h"
#include "mono.h"

#include "inferno.h"
#include "fvi.h"
#include "segment.h"
#include "object.h"
#include "wall.h"
#include "laser.h"
#include "rle.h"
#include "robot.h"
#include "piggy.h"
#include "player.h"
#include "gameseg.h"
#include "interp.h"
#include "hitbox.h"
#include "network.h"
#include "sphere.h"

#define faceType_num(nfaces, face_num, tri_edge) ((nfaces==1)?0:(tri_edge*2 + face_num))

#include "fvi_a.h"

int CheckSphereToFace (vmsVector *pnt, fix rad, vmsVector *vertList, vmsVector *vNormal, int nVerts);

//#define _DEBUG

//	-----------------------------------------------------------------------------

inline fix RegisterHit (vmsVector *vBestHit, vmsVector *vCurHit, vmsVector *vPos, fix dMax)
{
   fix d = VmVecDist (vPos, vCurHit);

if (dMax < d) {
	dMax = d;
	*vBestHit = *vCurHit;
	}
return dMax;
}

//	-----------------------------------------------------------------------------
// Find intersection of perpendicular on p1,p2 through p3 with p1,p2.

int FindPointLineIntersectionf (vmsVector *pv1, vmsVector *pv2, vmsVector *pv3)
{
	fVector	p1, p2, p3, d31, d21, h, v [2];
	float		m, u;

VmsVecToFloat (&p1, pv1);
VmsVecToFloat (&p2, pv2);
VmsVecToFloat (&p3, pv3);
VmVecSubf (&d21, &p2, &p1);
if (!(m = d21.p.x * d21.p.x + d21.p.y * d21.p.y + d21.p.z * d21.p.z))
	return 0;
VmVecSubf (&d31, &p3, &p1);
u = VmVecDotf (&d31, &d21);
u /= m;
h.p.x = p1.p.x + u * d21.p.x;
h.p.y = p1.p.y + u * d21.p.y;
h.p.z = p1.p.z + u * d21.p.z;
// limit the intersection to [p1,p2]
VmVecSubf (v, &p1, &h);
VmVecSubf (v + 1, &p2, &h);
m = VmVecDotf (v, v + 1);
if (m >= 1)
	return 1;
return 0;
}

//	-----------------------------------------------------------------------------
//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//vPlanePoint & vPlaneNorm describe the plane
//p0 & p1 are the ends of the line
int FindPlaneLineIntersection (vmsVector *hitP, vmsVector *vPlanePoint, vmsVector *vPlaneNorm, 
										 vmsVector *p0, vmsVector *p1, fix rad)
{
	vmsVector	d, w;
	fix			num, den;

VmVecSub (&w, p0, vPlanePoint);
VmVecSub (&d, p1, p0);
num = VmVecDot (vPlaneNorm, &w) - rad;
den = -VmVecDot (vPlaneNorm, &d);
if (!den) {
	fVector	nf, df;
	float denf;
	VmsVecToFloat (&nf, vPlaneNorm);
	VmsVecToFloat (&df, &d);
	denf = -VmVecDotf (&nf, &df);
	denf = -VmVecDotf (&nf, &df);
	return 0;
	}
if (den > 0) {
	if ((num > den) || ((-num >> 15) >= den)) //frac greater than one
		return 0;
	}
else {
	if (num < den)
		return 0;
	}
//do check for potential overflow
if (labs (num) / (f1_0 / 2) >= labs (den))
	return 0;
VmVecScaleFrac (&d, num, den);
VmVecAdd (hitP, p0, &d);
return 1;
}

//	-----------------------------------------------------------------------------
//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//vPlanePoint & vPlaneNorm describe the plane
//p0 & p1 are the ends of the line
int FindLineQuadIntersectionSub (vmsVector *hitP, vmsVector *vPlanePoint, vmsVector *vPlaneNorm, 
										   vmsVector *p0, vmsVector *p1, fix rad)
{
	vmsVector	d, w;
	fix			num, den;

VmVecSub (&w, vPlanePoint, p0);
VmVecSub (&d, p1, p0);
num = VmVecDot (vPlaneNorm, &w);
den = VmVecDot (vPlaneNorm, &d);
if (!den)
	return 0;
if (labs (num) > labs (den))
	return 0;
//do check for potential overflow
if (labs (num) / (f1_0 / 2) >= labs (den))
	return 0;
VmVecScaleFrac (&d, num, den);
VmVecAdd (hitP, p0, &d);
return 1;
}

//	-----------------------------------------------------------------------------
// if hitP is inside the quad planeP, the perpendicular from hitP to each edge
// of the quad must hit each edge between the edge's end points (provided hitP
// is in the quad's plane).

int CheckLineHitsQuad (vmsVector *hitP, vmsVector *planeP)
{
	int	i;

for (i = 0; i < 4; i++)
	if (FindPointLineIntersectionf (planeP + i, planeP + ((i + 1) % 4), hitP))
		return 0;	//doesn't hit
return 1;	//hits
}

//	-----------------------------------------------------------------------------

int FindLineQuadIntersection (vmsVector *hitP, vmsVector *planeP, vmsVector *planeNormP, vmsVector *p0, vmsVector *p1)
{
	vmsVector	vHit, d [2];

if (!FindLineQuadIntersectionSub (&vHit, planeP, planeNormP, p0, p1, 0))
	return 0;
VmVecSub (d, &vHit, p0);
VmVecSub (d + 1, &vHit, p1);
if (VmVecDot (d, d + 1) >= 0)
	return 0;
if (!CheckSphereToFace (&vHit, 0, planeP, planeNormP, 4))
	return 0;
*hitP = vHit;
return 1;
}

//	-----------------------------------------------------------------------------
// Simple intersection check by checking whether any of the edges of plane p1
// penetrate p2. Returns average of all penetration points.

int FindQuadQuadIntersectionSub (vmsVector *hitP, vmsVector *p1, vmsVector *vn1, vmsVector *p2, vmsVector *vn2, vmsVector *vPos)
{
	int			i, nHits = 0;
	fix			dMax = 0;
	vmsVector	vHit;

for (i = 0; i < 4; i++)
	if (FindLineQuadIntersection (&vHit, p2, vn2, p1 + i, p1 + ((i + 1) % 4))) {
		dMax = RegisterHit (hitP, &vHit, vPos, dMax);
		nHits++;
		}
return nHits;
}

//	-----------------------------------------------------------------------------

int FindQuadQuadIntersection (vmsVector *hitP, vmsVector *p1, vmsVector *vn1, vmsVector *p2, vmsVector *vn2, vmsVector *vPos)
{
	vmsVector	vHit;
	int			nHits = 0;
	fix			dMax = 0;

// test whether any edges of p1 penetrate p2
if (FindQuadQuadIntersectionSub (&vHit, p1, vn1, p2, vn2, vPos)) {
	dMax = RegisterHit (hitP, &vHit, vPos, dMax);
	nHits++;
	}
// test whether any edges of p2 penetrate p1
if (FindQuadQuadIntersectionSub (&vHit, p2, vn2, p1, vn1, vPos)) {
	dMax = RegisterHit (hitP, &vHit, vPos, dMax);
	nHits++;
	}
return nHits;
}

//	-----------------------------------------------------------------------------

int FindLineHitboxIntersection (vmsVector *hitP, tBox *phb, vmsVector *p0, vmsVector *p1, vmsVector *vPos)
{
	int			i, nHits = 0;
	fix			dMax = 0;
	vmsVector	vHit;
	tQuad			*pf;

// create all faces of hitbox 2 and their normals before testing because they will 
// be used multiple times
for (i = 0, pf = phb->faces; i < 6; i++, pf++)
	if (FindLineQuadIntersection (&vHit, pf->v, pf->n + 1, p0, p1)) {
		dMax = RegisterHit (hitP, &vHit, vPos, dMax);
		nHits++;
		}
return nHits;
}

//	-----------------------------------------------------------------------------

int FindHitboxIntersection (vmsVector *hitP, tBox *phb1, tBox *phb2, vmsVector *vPos)
{
	int			i, j, nHits = 0;
	fix			dMax = 0;
	vmsVector	vHit;
	tQuad			*pf1, *pf2;

// create all faces of hitbox 2 and their normals before testing because they will 
// be used multiple times
for (i = 0, pf1 = phb1->faces; i < 6; i++, pf1++) {
	for (j = 0, pf2 = phb2->faces; j < 6; j++, pf2++) {
#if 1
		if (VmVecDot (pf1->n + 1, pf2->n + 1) >= 0)
			continue;
#endif
		if (FindQuadQuadIntersection (&vHit, pf1->v, pf1->n + 1, pf2->v, pf2->n + 1, vPos)) {
			dMax = RegisterHit (hitP, &vHit, vPos, dMax);
			nHits++;
#ifdef _DEBUG
			pf1->t = pf2->t = gameStates.app.nSDLTicks;
#endif
			}
		}
	}
return nHits;
}

//	-----------------------------------------------------------------------------

typedef struct vec2d {
	fix i, j;
} vec2d;

//given largest component of normal, return i & j
//if largest component is negative, swap i & j
int ijTable [3][2] = {
	{2, 1},          //pos x biggest
	{0, 2},          //pos y biggest
	{1, 0},          //pos z biggest
	};

//intersection types
#define IT_NONE	0       //doesn't touch face at all
#define IT_FACE	1       //touches face
#define IT_EDGE	2       //touches edge of face
#define IT_POINT  3       //touches vertex

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d
uint CheckPointToFace (vmsVector *checkP, vmsVector *vertList, vmsVector *vNormal, int nVerts)
{
//	vmsVector	vNormal;
	vmsVector	t;
	int			biggest;
	int 			i, j, nEdge;
	uint 			nEdgeMask;
	fix 			check_i, check_j;
	vmsVector	*v0, *v1;
	vec2d 		vEdge, vCheck;
	fix 			d;

//VmVecNormal (&vNormal, vertList, vertList + 1, vertList + 2);
//now do 2d check to see if point is in tSide
//project polygon onto plane by finding largest component of normal
t.p.x = labs (vNormal->v [0]); 
t.p.y = labs (vNormal->v [1]); 
t.p.z = labs (vNormal->v [2]);
if (t.p.x > t.p.y) 
	if (t.p.x > t.p.z) 
		biggest = 0; 
	else 
		biggest = 2;
else if (t.p.y > t.p.z) 
	biggest = 1; 
else 
	biggest = 2;
if (vNormal->v [biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = checkP->v [i];
check_j = checkP->v [j];
for (nEdge = nEdgeMask = 0; nEdge < nVerts; nEdge++) {
	v0 = vertList + nEdge;
	v1 = vertList + ((nEdge + 1) % nVerts);
	vEdge.i = v1->v [i] - v0->v [i];
	vEdge.j = v1->v [j] - v0->v [j];
	vCheck.i = check_i - v0->v [i];
	vCheck.j = check_j - v0->v [j];
	d = FixMul (vCheck.i, vEdge.j) - FixMul (vCheck.j, vEdge.i);
	if (d < 0)              		//we are outside of triangle
		nEdgeMask |= (1 << nEdge);
	}
return nEdgeMask;
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int CheckSphereToFace (vmsVector *pnt, fix rad, vmsVector *vertList, vmsVector *vNormal, int nVerts)
{
	vmsVector	checkP = *pnt;
	vmsVector	vEdge, vCheck;            //this time, real 3d vectors
	vmsVector	vClosestPoint;
	fix			xEdgeLen, d, dist;
	vmsVector	*v0, *v1;
	int			iType;
	int			nEdge;
	uint			nEdgeMask;

//now do 2d check to see if point is inside
if (!(nEdgeMask = CheckPointToFace (pnt, vertList, vNormal, nVerts)))
	return IT_FACE;	//we've gone through all the sides, and are inside
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
v0 = vertList + nEdge;
v1 = vertList + ((nEdge + 1) % nVerts);
//check if we are touching an edge or point
VmVecSub (&vCheck, &checkP, v0);
xEdgeLen = VmVecNormalizedDir (&vEdge, v1, v0);
//find point dist from planes of ends of edge
d = VmVecDot (&vEdge, &vCheck);
if (d + rad < 0) 
	return IT_NONE;                  //too far behind start point
if (d - rad > xEdgeLen) 
	return IT_NONE;    //too far part end point
//find closest point on edge to check point
iType = IT_POINT;
if (d < 0) 
	vClosestPoint = *v0;
else if (d > xEdgeLen) 
	vClosestPoint = *v1;
else {
	iType = IT_EDGE;
	VmVecScaleAdd (&vClosestPoint, v0, &vEdge, d);
	}
dist = VmVecDist (&checkP, &vClosestPoint);
if (dist <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in newP with intersection
//point on plane, whether or not line intersects tSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
int CheckLineToFace (vmsVector *intP, vmsVector *p0, vmsVector *p1, 
							vmsVector *vertList, vmsVector *vNormal, int nVerts, fix rad)
{
	vmsVector	hitP, v1;
	int			pli, bCheckRad = 0;

//use lowest point number
if (p1 == p0) {
	if (!rad)
		return IT_NONE;
	VmVecCopyScale (&v1, vNormal, -rad);
	VmVecInc (&v1, p0);
	bCheckRad = rad;
	rad = 0;
	p1 = &v1;
	}
if (!(pli = FindPlaneLineIntersection (intP, vertList, vNormal, p0, p1, rad)))
	return IT_NONE;
hitP = *intP;
//if rad != 0, project the point down onto the plane of the polygon
if (rad)
	VmVecScaleInc (&hitP, vNormal, -rad);
if ((pli = CheckSphereToFace (&hitP, rad, vertList, vNormal, nVerts)))
	return pli;
#if 1
if (bCheckRad) {
	int			i, d;
	vmsVector	*a, *b;

	b = vertList;
	for (i = 1; i <= nVerts; i++) {
		a = b;
		b = vertList + (i % nVerts);
		d = VmLinePointDist (a, b, p0);
		if (d < bCheckRad)
			return IT_POINT;
		}
	}
#endif
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//see if a point is inside a face by projecting into 2d
uint CheckPointToSegFace (vmsVector *checkP, short nSegment, short nSide, short iFace, 
								  int nv, int *vertList)
{
	vmsVector vNormal;
	vmsVector t;
	int biggest;
///
	int 			i, j, nEdge;
	uint 			nEdgeMask;
	fix 			check_i, check_j;
	vmsVector	*v0, *v1;
	vec2d 		vEdge, vCheck;
	fix 			d;

if (gameStates.render.bRendering)
	vNormal = gameData.segs.segment2s [nSegment].sides [nSide].rotNorms [iFace];
else
	vNormal = gameData.segs.segments [nSegment].sides [nSide].normals [iFace];
//now do 2d check to see if point is in tSide
//project polygon onto plane by finding largest component of normal
t.p.x = labs (vNormal.v [0]); 
t.p.y = labs (vNormal.v [1]); 
t.p.z = labs (vNormal.v [2]);
if (t.p.x > t.p.y) 
	if (t.p.x > t.p.z) 
		biggest = 0; 
	else 
		biggest = 2;
else if (t.p.y > t.p.z) 
	biggest = 1; 
else 
	biggest = 2;
if (vNormal.v [biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = checkP->v [i];
check_j = checkP->v [j];
for (nEdge = nEdgeMask = 0; nEdge < nv; nEdge++) {
	if (gameStates.render.bRendering) {
		v0 = &gameData.segs.points [vertList [iFace * 3 + nEdge]].p3_vec;
		v1 = &gameData.segs.points [vertList [iFace * 3 + ((nEdge + 1) % nv)]].p3_vec;
		}
	else {
		v0 = gameData.segs.vertices + vertList [iFace * 3 + nEdge];
		v1 = gameData.segs.vertices + vertList [iFace * 3 + ((nEdge + 1) % nv)];
		}
	vEdge.i = v1->v [i] - v0->v [i];
	vEdge.j = v1->v [j] - v0->v [j];
	vCheck.i = check_i - v0->v [i];
	vCheck.j = check_j - v0->v [j];
	d = FixMul (vCheck.i, vEdge.j) - FixMul (vCheck.j, vEdge.i);
	if (d < 0)              		//we are outside of triangle
		nEdgeMask |= (1 << nEdge);
	}
return nEdgeMask;
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int CheckSphereToSegFace (vmsVector *pnt, short nSegment, short nSide, short iFace, int nv, 
								  fix rad, int *vertList)
{
	vmsVector	checkP = *pnt;
	vmsVector	vEdge, vCheck;            //this time, real 3d vectors
	vmsVector	vClosestPoint;
	fix			xEdgeLen, d, dist;
	vmsVector	*v0, *v1;
	int			iType;
	int			nEdge;
	uint			nEdgeMask;

//now do 2d check to see if point is in side
nEdgeMask = CheckPointToSegFace (pnt, nSegment, nSide, iFace, nv, vertList);
//we've gone through all the sides, are we inside?
if (nEdgeMask == 0)
	return IT_FACE;
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
if (gameStates.render.bRendering) {
	v0 = &gameData.segs.points [vertList [iFace * 3 + nEdge]].p3_vec;
	v1 = &gameData.segs.points [vertList [iFace * 3 + ((nEdge + 1) % nv)]].p3_vec;
	}
else {
	v0 = gameData.segs.vertices + vertList [iFace * 3 + nEdge];
	v1 = gameData.segs.vertices + vertList [iFace * 3 + ((nEdge + 1) % nv)];
	}
//check if we are touching an edge or point
VmVecSub (&vCheck, &checkP, v0);
xEdgeLen = VmVecNormalizedDir (&vEdge, v1, v0);
//find point dist from planes of ends of edge
d = VmVecDot (&vEdge, &vCheck);
if (d + rad < 0) 
	return IT_NONE;                  //too far behind start point
if (d - rad > xEdgeLen) 
	return IT_NONE;    //too far part end point
//find closest point on edge to check point
iType = IT_POINT;
if (d < 0) 
	vClosestPoint = *v0;
else if (d > xEdgeLen) 
	vClosestPoint = *v1;
else {
	iType = IT_EDGE;
	VmVecScaleAdd (&vClosestPoint, v0, &vEdge, d);
	}
dist = VmVecDist (&checkP, &vClosestPoint);
if (dist <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in newP with intersection
//point on plane, whether or not line intersects tSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
int CheckLineToSegFace (vmsVector *newP, vmsVector *p0, vmsVector *p1, 
								short nSegment, short nSide, short iFace, int nv, fix rad)
{
	vmsVector	checkP, vNormal, v1;
	int			vertexList [6];
	int			pli, nFaces, nVertex, bCheckRad = 0;

if (nSegment == -1)
	Error ("nSegment == -1 in CheckLineToSegFace()");
if (gameStates.render.bRendering)
	vNormal = gameData.segs.segment2s [nSegment].sides [nSide].rotNorms [iFace];
else
	vNormal = gameData.segs.segments [nSegment].sides [nSide].normals [iFace];
nFaces = CreateAbsVertexLists (vertexList, nSegment, nSide);
//use lowest point number
#if 1
if (p1 == p0) {
#if 0
	return CheckSphereToSegFace (p0, nSegment, nSide, iFace, nv, rad, vertexList);
#else
	if (!rad)
		return IT_NONE;
	VmVecCopyScale (&v1, &vNormal, -rad);
	VmVecInc (&v1, p0);
	bCheckRad = rad;
	rad = 0;
	p1 = &v1;
#endif
	}
#endif
nVertex = vertexList [0];
if (nVertex > vertexList [2])
	nVertex = vertexList [2];
if (nFaces == 1) {
	if (nVertex > vertexList [1])
		nVertex = vertexList [1];
	if (nVertex > vertexList [3])
		nVertex = vertexList [3];
	}
//LogErr ("         FindPlaneLineIntersection...");
pli = FindPlaneLineIntersection (newP, 
											gameStates.render.bRendering ? 
											&gameData.segs.points [nVertex].p3_vec : 
											gameData.segs.vertices + nVertex, 
											&vNormal, p0, p1, rad);
//LogErr ("done\n");
if (!pli) 
	return IT_NONE;
checkP = *newP;
//if rad != 0, project the point down onto the plane of the polygon
if (rad)
	VmVecScaleInc (&checkP, &vNormal, -rad);
if ((pli = CheckSphereToSegFace (&checkP, nSegment, nSide, iFace, nv, rad, vertexList)))
	return pli;
if (bCheckRad) {
	int			i, d;
	vmsVector	*a, *b;

	b = gameData.segs.vertices + vertexList [0];
	for (i = 1; i <= 4; i++) {
		a = b;
		b = gameData.segs.vertices + vertexList [i % 4];
		d = VmLinePointDist (a, b, p0);
		if (d < bCheckRad)
			return IT_POINT;
		}
	}
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//computes the parameters of closest approach of two lines
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
int CheckLineToLine (fix *t1, fix *t2, vmsVector *p1, vmsVector *v1, vmsVector *p2, vmsVector *v2)
{
	vmsMatrix det;
	fix d, cross_mag2;		//mag squared Cross product

//LogErr ("         VmVecSub\n");
VmVecSub (&det.rVec, p2, p1);
//LogErr ("         VmVecCross\n");
VmVecCross (&det.fVec, v1, v2);
//LogErr ("         VmVecDot\n");
cross_mag2 = VmVecDot (&det.fVec, &det.fVec);
if (!cross_mag2)
	return 0;			//lines are parallel
det.uVec = *v2;
d = VmMatrixDetValue (&det);
if (oflow_check (d, cross_mag2))
	return 0;
//LogErr ("         FixDiv (%d)\n", cross_mag2);
*t1 = FixDiv (d, cross_mag2);
det.uVec = *v1;
//LogErr ("         CalcDetValue\n");
d = VmMatrixDetValue (&det);
if (oflow_check (d, cross_mag2))
	return 0;
//LogErr ("         FixDiv (%d)\n", cross_mag2);
*t2 = FixDiv (d, cross_mag2);
return 1;		//found point
}

#ifdef NEW_FVI_STUFF
int bSimpleFVI = 0;
#else
#define bSimpleFVI 1
#endif

//	-----------------------------------------------------------------------------
//this version is for when the start and end positions both poke through
//the plane of a tSide.  In this case, we must do checks against the edge
//of faces
int SpecialCheckLineToSegFace (vmsVector *newP, vmsVector *p0, vmsVector *p1, short nSegment, 
									    short nSide, int iFace, int nv, fix rad)
{
	vmsVector	move_vec;
	fix			edge_t, move_t, edge_t2, move_t2, closestDist;
	fix			edge_len, move_len;
	int			vertList [6];
	int			h, nFaces, nEdge;
	uint			nEdgeMask;
	vmsVector	*edge_v0, *edge_v1, edge_vec;
	tSegment		*segP = gameData.segs.segments + nSegment;
	vmsVector	closest_point_edge, closest_point_move;

if (bSimpleFVI) {
	//LogErr ("      CheckLineToSegFace ...");
	h = CheckLineToSegFace (newP, p0, p1, nSegment, nSide, iFace, nv, rad);
	//LogErr ("done\n");
	return h;
	}
//calc some basic stuff
if ((SEG_IDX (segP)) == -1)
	Error ("nSegment == -1 in SpecialCheckLineToSegFace()");
//LogErr ("      CreateAbsVertexLists ...");
nFaces = CreateAbsVertexLists (vertList, nSegment, nSide);
//LogErr ("done\n");
VmVecSub (&move_vec, p1, p0);
//figure out which edge(side) to check against
//LogErr ("      CheckPointToSegFace ...\n");
if (!(nEdgeMask = CheckPointToSegFace (p0, nSegment, nSide, iFace, nv, vertList))) {
	//LogErr ("      CheckLineToSegFace ...");
	return CheckLineToSegFace (newP, p0, p1, nSegment, nSide, iFace, nv, rad);
	//LogErr ("done\n");
	}
for (nEdge = 0; !(nEdgeMask & 1); nEdgeMask >>= 1, nEdge++)
	;
//LogErr ("      setting edge vertices (%d, %d)...\n", vertList [iFace * 3 + nEdge], vertList [iFace * 3 + ((nEdge + 1) % nv)]);
edge_v0 = gameData.segs.vertices + vertList [iFace * 3 + nEdge];
edge_v1 = gameData.segs.vertices + vertList [iFace * 3 + ((nEdge + 1) % nv)];
//LogErr ("      setting edge vector...\n");
VmVecSub (&edge_vec, edge_v1, edge_v0);
//is the start point already touching the edge?
//first, find point of closest approach of vec & edge
//LogErr ("      getting edge length...\n");
edge_len = VmVecNormalize (&edge_vec);
//LogErr ("      getting move length...\n");
move_len = VmVecNormalize (&move_vec);
//LogErr ("      CheckLineToLine...");
CheckLineToLine (&edge_t, &move_t, edge_v0, &edge_vec, p0, &move_vec);
//LogErr ("done\n");
//make sure t values are in valid range
if ((move_t < 0) || (move_t > move_len + rad))
	return IT_NONE;
if (move_t > move_len)
	move_t2 = move_len;
else
	move_t2 = move_t;
if (edge_t < 0)		//clamp at points
	edge_t2 = 0;
else
	edge_t2 = edge_t;
if (edge_t2 > edge_len)		//clamp at points
	edge_t2 = edge_len;
//now, edge_t & move_t determine closest points.  calculate the points.
VmVecScaleAdd (&closest_point_edge, edge_v0, &edge_vec, edge_t2);
VmVecScaleAdd (&closest_point_move, p0, &move_vec, move_t2);
//find dist between closest points
//LogErr ("      computing closest dist.p...\n");
closestDist = VmVecDist (&closest_point_edge, &closest_point_move);
//could we hit with this dist?
//note massive tolerance here
if (closestDist < (rad * 9) / 10) {		//we hit.  figure out where
	//now figure out where we hit
	VmVecScaleAdd (newP, p0, &move_vec, move_t-rad);
	return IT_EDGE;
	}
return IT_NONE;			//no hit
}

//	-----------------------------------------------------------------------------
//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0, p1
//returns dist if intersects, and fills in intP
//else returns 0
int CheckVectorToSphere1 (vmsVector *intP, vmsVector *p0, vmsVector *p1, vmsVector *vSpherePos, 
								  fix xSphereRad)
{
	vmsVector d, dn, w, vClosestPoint;
	fix mag_d, dist, wDist, intDist;

//this routine could be optimized if it's taking too much time!

VmVecSub (&d, p1, p0);
VmVecSub (&w, vSpherePos, p0);
mag_d = VmVecCopyNormalize (&dn, &d);
if (mag_d == 0) {
	intDist = VmVecMag (&w);
	*intP = *p0;
	return ((xSphereRad < 0) || (intDist < xSphereRad)) ? intDist : 0;
	}
wDist = VmVecDot (&dn, &w);
if (wDist < 0)
	return 0;	//moving away from tObject
if (wDist > mag_d + xSphereRad)
	return 0;	//cannot hit
VmVecScaleAdd (&vClosestPoint, p0, &dn, wDist);
dist = VmVecDist (&vClosestPoint, vSpherePos);
if  (dist < xSphereRad) {
	fix	dist2, radius2, nShorten;

	dist2 = FixMul (dist, dist);
	radius2 = FixMul (xSphereRad, xSphereRad);
	nShorten = fix_sqrt (radius2 - dist2);
	intDist = wDist - nShorten;
	if ((intDist > mag_d) || (intDist < 0)) {
		//past one or the other end of vector, which means we're inside
		*intP = *p0;		//don't move at all
		return 1;
		}
	VmVecScaleAdd (intP, p0, &dn, intDist);         //calc intersection point
	return intDist;
	}
return 0;
}

//	-----------------------------------------------------------------------------

fix CheckHitboxToHitbox (vmsVector *intP, tObject *objP1, tObject *objP2, vmsVector *p0, vmsVector *p1)
{
	vmsVector		vHit, vPos = objP2->position.vPos;
	int				iModel1, nModels1, iModel2, nModels2, nHits = 0;
	tModelHitboxes	*pmhb1 = gameData.models.hitboxes + objP1->rType.polyObjInfo.nModel;
	tModelHitboxes	*pmhb2 = gameData.models.hitboxes + objP2->rType.polyObjInfo.nModel;
	tBox				hb1 [MAX_HITBOXES + 1];
	tBox				hb2 [MAX_HITBOXES + 1];
	fix				dMax = 0;

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel1 =
	nModels1 =
	iModel2 =
	nModels2 = 0;
	}
else {
	iModel1 =
	iModel2 = 1;
	nModels1 = pmhb1->nSubModels;
	nModels2 = pmhb2->nSubModels;
	}
#ifdef _DEBUG
memset (hb1, 0, sizeof (hb1));
memset (hb2, 0, sizeof (hb2));
#endif
TransformHitboxes (objP1, p1, hb1);
TransformHitboxes (objP2, &vPos, hb2);
for (; iModel1 <= nModels1; iModel1++) {
	for (; iModel2 <= nModels2; iModel2++) {
		if (FindHitboxIntersection (&vHit, hb1 + iModel1, hb2 + iModel2, &vPos)) {
			dMax = RegisterHit (intP, &vHit, &vPos, dMax);
			nHits++;
			}
		}
	}
if (!nHits) {
	for (; iModel2 <= nModels2; iModel2++) {
		if (FindLineHitboxIntersection (&vHit, hb2 + iModel2, p0, p1, &vPos)) {
			dMax = RegisterHit (intP, &vHit, &vPos, dMax);
			nHits++;
			}
		}
	}
#ifdef _DEBUG
if (nHits) {
	pmhb1->vHit = pmhb2->vHit = *intP;
	pmhb1->tHit = pmhb2->tHit = gameStates.app.nSDLTicks;
	}
#endif
return nHits ? dMax ? dMax : 1 : 0;
}

//	-----------------------------------------------------------------------------

fix CheckVectorToHitbox (vmsVector *intP, vmsVector *p0, vmsVector *p1, vmsVector *pn, vmsVector *vPos, tObject *objP, fix rad)
{
	tQuad				*pf;
	vmsVector		hitP, v;
	int				i, iModel, nModels;
	fix				h, d, xDist = 0x7fffffff;
	tModelHitboxes	*pmhb = gameData.models.hitboxes + objP->rType.polyObjInfo.nModel;
	tBox				hb [MAX_HITBOXES + 1];

if (extraGameInfo [IsMultiGame].nHitboxes == 1) {
	iModel =
	nModels = 0;
	}
else {
	iModel = 1;
	nModels = pmhb->nSubModels;
	}
TransformHitboxes (objP, vPos, hb);
for (; iModel <= nModels; iModel++) {
	for (i = 0, pf = hb [iModel].faces; i < 6; i++, pf++) {
#if 0
		dot = VmVecDot (pf->n + 1, pn);
		if (dot >= 0)
			continue;	//shield face facing away from vector
#endif
		h = CheckLineToFace (&hitP, p0, p1, pf->v, pf->n + 1, 4, rad);
		if (h) {
			d = VmVecNormalize (VmVecSub (&v, &hitP, p0));
#if 0
			dot = VmVecDot (pf->n + 1, pn);
			if (dot > 0)
				continue;	//behind shield face
			if (d > rad)
				continue;
#endif
			if (xDist > d) {
				*intP = hitP;
				if (!(xDist = d))
					return 0;
				}
			}
		}
	}
return xDist;
}

//	-----------------------------------------------------------------------------
//determine if a vector intersects with an tObject
//if no intersects, returns 0, else fills in intP and returns dist
fix CheckVectorToObject (vmsVector *intP, vmsVector *p0, vmsVector *p1, fix rad, 
								 tObject *thisObjP, tObject *otherObjP)
{
	fix			size, dist;
	vmsVector	hitP, v0, v1, vn;
	int			bThisPoly, bOtherPoly;

if (rad < 0)
	size = 0;
else {
	size = thisObjP->size;
	if ((thisObjP->nType == OBJ_ROBOT) && ROBOTINFO (thisObjP->id).attackType)
		size = 3 * size / 4;
	//if obj is tPlayer, and bumping into other tPlayer or a weapon of another coop tPlayer, reduce radius
	if ((thisObjP->nType == OBJ_PLAYER) &&
		 ((otherObjP->nType == OBJ_PLAYER) ||
 		 ((IsCoopGame) && (otherObjP->nType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parentType == OBJ_PLAYER))))
		size /= 2;
	}

	// check hit sphere collisions
if (!(dist = CheckVectorToSphere1 (&hitP, p0, p1, &thisObjP->position.vPos, size + rad)))
	return 0;

bThisPoly = (thisObjP->renderType == RT_POLYOBJ) && (thisObjP->rType.polyObjInfo.nModel >= 0); // && ((thisObjP->nType != OBJ_WEAPON) || gameData.objs.bIsMissile [thisObjP->id]);
bOtherPoly = (otherObjP->renderType == RT_POLYOBJ) && (otherObjP->rType.polyObjInfo.nModel >= 0); // && ((otherObjP->nType != OBJ_WEAPON) || gameData.objs.bIsMissile [otherObjP->id]);
if (EGI_FLAG (nHitboxes, 0, 0, 0) && (bThisPoly || bOtherPoly) && 
	 (thisObjP->nType != OBJ_MONSTERBALL) && (otherObjP->nType != OBJ_MONSTERBALL) && 
	 (thisObjP->nType != OBJ_HOSTAGE) && (otherObjP->nType != OBJ_HOSTAGE) && 
	 (thisObjP->nType != OBJ_POWERUP) && (otherObjP->nType != OBJ_POWERUP)) {
#if 0//def RELEASE
	dist = VmLinePointDist (p0, p1, &thisObjP->position.vPos);
	//HUDMessage (0, "%1.2f %1.2f", f2fl (dist), f2fl (thisObjP->size + otherObjP->size));
	if (dist > (size = thisObjP->size + otherObjP->size))
		return 0;
#	if 0
	if (((thisObjP->nType == OBJ_PLAYER) || (thisObjP->nType == OBJ_ROBOT)) && 
		 ((otherObjP->nType == OBJ_PLAYER) || (otherObjP->nType == OBJ_ROBOT)))
		size /= 2;
	else
		size /= 4;
	if (dist < size) {	// objects probably already stuck in each other
		VmVecScaleFrac (VmVecAdd (intP, &thisObjP->position.vPos, &otherObjP->position.vPos), 1, 2);
		return 1;
		}
#	endif
#endif
	// check hitbox collisions for all polygonal objects
	if (bThisPoly && bOtherPoly) {
		if (!(dist = CheckHitboxToHitbox (&hitP, otherObjP, thisObjP, p0, p1)))
			if (0x7fffffff == (dist = CheckVectorToHitbox (&hitP, p0, p1, &vn, NULL, thisObjP, 0)))
				return 0;
		VmPointLineIntersection (&hitP, p0, p1, &hitP, &thisObjP->position.vPos, 1);
		}
	else {
		if (bThisPoly) {
		// *thisObjP (stationary) has hitboxes, *otherObjP (moving) a hit sphere. To detect whether the sphere 
		// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			VmVecNormalize (VmVecSub (&vn, p1, p0));
			if (0x7fffffff == (dist = CheckVectorToHitbox (&hitP, p0, p1, &vn, NULL, thisObjP, otherObjP->size)))
				return 0;
			VmPointLineIntersection (&hitP, p0, p1, &hitP, &otherObjP->position.vPos, 1);
			}
		else {
		// *otherObjP (moving) has hitboxes, *thisObjP (stationary) a hit sphere. To detect whether the sphere 
		// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			v0 = thisObjP->position.vPos;
			VmVecNormalize (VmVecSub (&vn, &otherObjP->position.vPos, &v0));
			VmVecScaleAdd (&v1, &v0, &vn, thisObjP->size);
			if (0x7fffffff == (dist = CheckVectorToHitbox (&hitP, &v0, &v0, &vn, p1, otherObjP, thisObjP->size)))
				return 0;
			VmPointLineIntersection (&hitP, p0, p1, &hitP, &thisObjP->position.vPos, 1);
			}
		}
	}
*intP = hitP;
return dist;
}

//	-----------------------------------------------------------------------------

int FVICompute (vmsVector *intP, short *intS, vmsVector *p0, short nStartSeg, vmsVector *p1, 
					 fix radP0, fix radP1, short nThisObject, short *ignoreObjList, int flags, short *segList, 
					 short *nSegments, int nEntrySeg);

//Find out if a vector intersects with anything.
//Fills in hitData, an tFVIData structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisObjNum 		used to prevent an tObject with colliding with itself
//  ingore_obj			ignore collisions with this tObject
//  check_objFlag	determines whether collisions with gameData.objs.objects are checked
//Returns the hitData->nHitType
int FindVectorIntersection (tVFIQuery *fq, tFVIData *hitData)
{
	int			nHitType, nNewHitType;
	short			nHitSegment, nHitSegment2;
	vmsVector	vHitPoint;
	int			i;
	tSegMasks		masks;

Assert(fq->ignoreObjList != (short *)(-1));
VmVecZero (&gameData.collisions.hitData.vNormal);
gameData.collisions.hitData.nNormals = 0;
Assert((fq->startSeg <= gameData.segs.nLastSegment) && (fq->startSeg >= 0));

gameData.collisions.hitData.nSegment = -1;
gameData.collisions.hitData.nSide = -1;
gameData.collisions.hitData.nObject = -1;

//check to make sure start point is in seg its supposed to be in
//Assert(check_point_in_seg(p0, startseg, 0).centerMask==0);	//start point not in seg

// gameData.objs.viewer is not in tSegment as claimed, so say there is no hit.
masks = GetSegMasks (fq->p0, fq->startSeg, 0);
if (masks.centerMask) {
	hitData->hit.nType = HIT_BAD_P0;
	hitData->hit.vPoint = *fq->p0;
	hitData->hit.nSegment = fq->startSeg;
	hitData->hit.nSide = 0;
	hitData->hit.nObject = 0;
	hitData->hit.nSideSegment = -1;
	hitData->nSegments = 0;
	return hitData->hit.nType;
	}
gameData.collisions.segsVisited [0] = fq->startSeg;
gameData.collisions.nSegsVisited = 1;
gameData.collisions.hitData.nNestCount = 0;
nHitSegment2 = gameData.collisions.hitData.nSegment2 = -1;
nHitType = FVICompute (&vHitPoint, &nHitSegment2, fq->p0, (short) fq->startSeg, fq->p1, 
							  fq->radP0, fq->radP1, (short) fq->thisObjNum, fq->ignoreObjList, fq->flags, 
							  hitData->segList, &hitData->nSegments, -2);
//!!nHitSegment = FindSegByPoint(&vHitPoint, fq->startSeg, 1, 0);
if ((nHitSegment2 != -1) && !GetSegMasks (&vHitPoint, nHitSegment2, 0).centerMask)
	nHitSegment = nHitSegment2;
else {
	nHitSegment = FindSegByPoint (&vHitPoint, fq->startSeg, 1, 0);
	}
//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
if ((nHitType == HIT_WALL) && (nHitSegment == -1))
	if ((gameData.collisions.hitData.nSegment2 != -1) && !GetSegMasks (&vHitPoint, gameData.collisions.hitData.nSegment2, 0).centerMask)
		nHitSegment = gameData.collisions.hitData.nSegment2;

if (nHitSegment == -1) {
	//int nNewHitType;
	short nNewHitSeg2=-1;
	vmsVector vNewHitPoint;

	//because of code that deal with tObject with non-zero radius has
	//problems, try using zero radius and see if we hit a tWall
	nNewHitType = FVICompute (&vNewHitPoint, &nNewHitSeg2, fq->p0, (short) fq->startSeg, fq->p1, 0, 0,
								     (short) fq->thisObjNum, fq->ignoreObjList, fq->flags, hitData->segList, 
									  &hitData->nSegments, -2);
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

int ObjectInList (short nObject, short *objListP)
{
	short t;

while (((t = *objListP) != -1) && (t != nObject)) 
	objListP++;
return (t == nObject);

}

//	-----------------------------------------------------------------------------

#define FVI_NEWCODE 2

int CheckTransWall (vmsVector *vPoint, tSegment *segP, short nSide, short iFace);

int FVICompute (vmsVector *vIntP, short *intS, vmsVector *p0, short nStartSeg, vmsVector *p1, 
					 fix radP0, fix radP1, short nThisObject, short *ignoreObjList, int flags, short *segList, 
					 short *nSegments, int nEntrySeg)
{
	tSegment		*segP;				//the tSegment we're looking at
	int			startMask, endMask, centerMask;	//mask of faces
	short			nObject;
	tSegMasks	masks;
	vmsVector	vHitPoint, vClosestHitPoint; 	//where we hit
	fix			d, dMin = 0x7fffffff;					//distance to hit point
	int			nObjSegList [7], nObjSegs, iObjSeg, nSegment, i;
	int			nHitType = HIT_NONE;							//what sort of hit
	int			nHitSegment = -1;
	int			nHitNoneSegment = -1;
	int			nHitNoneSegs = 0;
	int			hitNoneSegList [MAX_FVI_SEGS];
	int			nCurNestLevel = gameData.collisions.hitData.nNestCount;
#if FVI_NEWCODE
	int			nFudgedRad;
	int			nFaces;
#if 0
	int			nFaceHitType;
#endif
	int			widResult;
	int			nThisType, nOtherType;
	tObject		*otherObjP,
					*thisObjP = (nThisObject < 0) ? NULL : gameData.objs.objects + nThisObject;
#endif
//LogErr ("Entry FVICompute\n");
if (flags & FQ_GET_SEGLIST)
	*segList = nStartSeg;
*nSegments = 1;
gameData.collisions.hitData.nNestCount++;
//first, see if vector hit any objects in this tSegment
nThisType = (nThisObject < 0) ? -1 : gameData.objs.objects [nThisObject].nType;
#if 1
if (flags & FQ_CHECK_OBJS) {
	//LogErr ("   checking objects...");
	nObjSegList [0] = nStartSeg;
	nObjSegs = 1;
#	ifdef _DEBUG
	if ((thisObjP->nType == OBJ_WEAPON) && (thisObjP->nSegment == gameData.objs.console->nSegment))
		flags = flags;
#	endif
#if 1
	segP = gameData.segs.segments + nStartSeg;
	for (iObjSeg = 0; iObjSeg < 6; iObjSeg++) {
		if (0 > (nSegment = segP->children [iObjSeg]))
			continue;
		for (i = 0; i < gameData.collisions.nSegsVisited && (nSegment != gameData.collisions.segsVisited [i]); i++)
			;
		if (i == gameData.collisions.nSegsVisited)
			nObjSegList [nObjSegs++] = nSegment;
		}
#endif
	for (iObjSeg = 0; iObjSeg < nObjSegs; iObjSeg++) {
		segP = gameData.segs.segments + nObjSegList [iObjSeg];
		for (nObject = segP->objects; nObject != -1; nObject = otherObjP->next) {
			otherObjP = gameData.objs.objects + nObject;
			nOtherType = otherObjP->nType;
			if (otherObjP->flags & OF_SHOULD_BE_DEAD)
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
			nFudgedRad = radP1;

			//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
			if ((nOtherType == OBJ_POWERUP) && (flags & FQ_IGNORE_POWERUPS))
				continue;
			//	If this is a robot:robot collision, only do it if both of them have attackType != 0 (eg, green guy)
			if (nThisType == OBJ_ROBOT) {
				if (flags & FQ_ANY_OBJECT) {
					if (nOtherType != OBJ_ROBOT)
						continue;
					}
				else {
					if (nOtherType == OBJ_ROBOT)
						continue;
					}
				if (nOtherType == OBJ_ROBOT)
					nOtherType = OBJ_ROBOT;
				if (ROBOTINFO (thisObjP->id).attackType)
					nFudgedRad = (radP1 * 3) / 4;
				}
			//if obj is tPlayer, and bumping into other tPlayer or a weapon of another coop tPlayer, reduce radius
			if ((nThisType == OBJ_PLAYER ) &&
				((nOtherType == OBJ_PLAYER) ||
				(IsCoopGame && (nOtherType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parentType == OBJ_PLAYER))))
				nFudgedRad = radP1 / 2;
			if (flags & FQ_ANY_OBJECT)
				d = CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);
			else
				d = CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);
			if (d && (d < dMin)) {
				gameData.collisions.hitData.nObject = nObject;
				Assert(gameData.collisions.hitData.nObject != -1);
				dMin = d;
				vClosestHitPoint = vHitPoint;
				nHitType = HIT_OBJECT;
#ifdef _DEBUG
				CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);
#endif
				if (flags & FQ_ANY_OBJECT)
					goto fviObjsDone;
				}
			}
		}
	}
#endif

fviObjsDone:

segP = gameData.segs.segments + nStartSeg;
if ((nThisObject > -1) && (gameData.objs.collisionResult [nThisType][OBJ_WALL] == RESULT_NOTHING))
	radP1 = 0;		//HACK - ignore when edges hit walls
//now, check segment walls
startMask = GetSegMasks (p0, nStartSeg, radP0).faceMask;
masks = GetSegMasks (p1, nStartSeg, radP1);    //on back of which faces?
if (!(centerMask = masks.centerMask))
	nHitNoneSegment = nStartSeg;
if ((endMask = masks.faceMask)) { //on the back of at least one face
	short nSide, iFace, bit;

	//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (endMask >= bit); nSide++) {
		if (!(nFaces = GetNumFaces (segP->sides + nSide)))
			nFaces = 1;
		for (iFace = 0; iFace < 2; iFace++, bit <<= 1) {
			if (segP->children [nSide] == nEntrySeg)	//must be executed here to have bit shifted
				continue;		//don't go back through entry nSide
			if (!(endMask & bit))	//on the back of this face?
				continue;
			//did we go through this tWall/door?
#if 0
			nFaceHitType = (startMask & bit)	?	//start was also though.  Do extra check
				SpecialCheckLineToSegFace (&vHitPoint, p0, p1, nStartSeg, nSide, iFace, 5 - nFaces, radP1) :
				CheckLineToSegFace (&vHitPoint, p0, p1, nStartSeg, nSide, iFace, 5 - nFaces, radP1);
			if (!nFaceHitType) 
				continue;
#endif
			widResult = WALL_IS_DOORWAY (segP, nSide, (nThisObject < 0) ? NULL : gameData.objs.objects + nThisObject);
			//LogErr ("done\n");
			//if what we have hit is a door, check the adjoining segP
			if ((nThisObject == LOCALPLAYER.nObject) && (gameStates.app.cheats.bPhysics == 0xBADA55)) {
				int childSide = segP->children [nSide];
				if (childSide >= 0) {
					int special = gameData.segs.segment2s [childSide].special;
					if (((special != SEGMENT_IS_BLOCKED) && (special != SEGMENT_IS_SKYBOX)) ||
							(gameData.objs.speedBoost [nThisObject].bBoosted &&
							((gameData.segs.segment2s [nStartSeg].special != SEGMENT_IS_SPEEDBOOST) ||
							(special == SEGMENT_IS_SPEEDBOOST))))
 						widResult |= WID_FLY_FLAG;
					}
				}
			if ((widResult & WID_FLY_FLAG) ||
				 (((widResult & (WID_RENDER_FLAG | WID_RENDPAST_FLAG)) == (WID_RENDER_FLAG | WID_RENDPAST_FLAG)) &&
				  ((flags & FQ_TRANSWALL) || ((flags & FQ_TRANSPOINT) && CheckTransWall (&vHitPoint, segP, nSide, iFace))))) {

				int			i, nNewSeg, subHitType;
				short			subHitSeg, nSaveHitObj = gameData.collisions.hitData.nObject;
				vmsVector	subHitPoint, vSaveWallNorm = gameData.collisions.hitData.vNormal;

				//do the check recursively on the next tSegment.p.
				nNewSeg = segP->children [nSide];
				//LogErr ("   check next seg (%d)\n", nNewSeg);
				for (i = 0; i < gameData.collisions.nSegsVisited && (nNewSeg != gameData.collisions.segsVisited [i]); i++)
					;
				if (i == gameData.collisions.nSegsVisited) {                //haven't visited here yet
					short tempSegList [MAX_FVI_SEGS], nTempSegs;
					if (gameData.collisions.nSegsVisited >= MAX_SEGS_VISITED)
						goto fviSegsDone;		//we've looked a long time, so give up
					gameData.collisions.segsVisited [gameData.collisions.nSegsVisited++] = nNewSeg;
					subHitType = FVICompute (&subHitPoint, &subHitSeg, p0, (short) nNewSeg, 
														p1, radP0, radP1, nThisObject, ignoreObjList, flags, 
														tempSegList, &nTempSegs, nStartSeg);
					if (subHitType != HIT_NONE) {
						d = VmVecDist (&subHitPoint, p0);
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
								//LogErr ("   segList <- tempSegList ...");
								memcpy (segList + *nSegments, tempSegList, i * sizeof (*segList));
								//LogErr ("done\n");
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
							//LogErr ("   hitNoneSegList <- tempSegList ...");
							memcpy (hitNoneSegList, tempSegList, i * sizeof (*hitNoneSegList));
							//LogErr ("done\n");
#endif
							}
						nHitNoneSegs = nTempSegs;
						}
					}
				}
			else {//a wall
#if 1
				if ((startMask & bit)	?	//start was also though.  Do extra check
					 SpecialCheckLineToSegFace (&vHitPoint, p0, p1, nStartSeg, nSide, iFace, 5 - nFaces, radP1) :
					 CheckLineToSegFace (&vHitPoint, p0, p1, nStartSeg, nSide, iFace, 5 - nFaces, radP1)) 
#endif
					{
					//is this the closest hit?
					d = VmVecDist (&vHitPoint, p0);
					if (d < dMin) {
						dMin = d;
						vClosestHitPoint = vHitPoint;
						nHitType = HIT_WALL;
						VmVecInc (&gameData.collisions.hitData.vNormal, segP->sides [nSide].normals + iFace);
						gameData.collisions.hitData.nNormals++;
						if (!GetSegMasks (&vHitPoint, nStartSeg, radP1).centerMask)
							nHitSegment = nStartSeg;             //hit in this tSegment
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

if (nHitType == HIT_NONE) {     //didn't hit anything, return end point
	int i;

	*vIntP = *p1;
	*intS = nHitNoneSegment;
	if (nHitNoneSegment != -1) {			//(centerMask == 0)
		if (flags & FQ_GET_SEGLIST) {
#if FVI_NEWCODE != 2
			for (i = 0; (i < nHitNoneSegs) && (*nSegments < MAX_FVI_SEGS - 1);)
				segList [(*nSegments)++] = hitNoneSegList [i++];
#else
			i = MAX_FVI_SEGS - 1 - *nSegments;
			if (i > nHitNoneSegs)
				i = nHitNoneSegs;
			//LogErr ("   segList <- hitNoneSegList ...");
			memcpy (segList + *nSegments, hitNoneSegList, i * sizeof (*segList));
			//LogErr ("done\n");
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
			*intS = gameData.collisions.hitData.nSegment2;
		else
			*intS = nHitNoneSegment;
	else
		*intS = nHitSegment;
	}
Assert(!(nHitType==HIT_OBJECT && gameData.collisions.hitData.nObject==-1));
//LogErr ("Exit FVICompute\n");
return nHitType;
}

#include "textures.h"
#include "texmerge.h"

#define Cross(v0, v1) (FixMul((v0)->i, (v1)->j) - FixMul((v0)->j, (v1)->i))

//	-----------------------------------------------------------------------------
//finds the uv coords of the given point on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
void FindHitPointUV (fix *u, fix *v, fix *l, vmsVector *pnt, tSegment *seg, int nSide, int iFace)
{
	vmsVector	*pnt_array;
	vmsVector	normal_array;
	int			nSegment = SEG_IDX (seg);
	int			nFaces;
	int			biggest, ii, jj;
	tSide			*sideP = &seg->sides [nSide];
	int			vertList [6], vertnum_list [6];
 	vec2d			p1, vec0, vec1, checkP;
	tUVL			uvls [3];
	fix			k0, k1;
	int			h, i;

//do lasers pass through illusory walls?
//when do I return 0 & 1 for non-transparent walls?
if ((nSegment < 0) || (nSegment > gameData.segs.nLastSegment)) {
#if TRACE
	con_printf (CONDBG, "Bad nSegment (%d) in FindHitPointUV()\n", nSegment);
#endif
	*u = *v = 0;
	return;
	}
if (nSegment == -1) {
	Error ("nSegment == -1 in FindHitPointUV()");
	return;
	}
nFaces = CreateAbsVertexLists (vertList, nSegment, nSide);
CreateAllVertNumLists (&nFaces, vertnum_list, nSegment, nSide);
//now the hard work.
//1. find what plane to project this tWall onto to make it a 2d case
memcpy (&normal_array, sideP->normals + iFace, sizeof (vmsVector));
biggest = 0;
if (abs (normal_array.v [1]) > abs (normal_array.v [biggest])) 
	biggest = 1;
if (abs (normal_array.v [2]) > abs (normal_array.v [biggest])) 
	biggest = 2;
ii = (biggest == 0);
jj = (biggest == 2) ? 1 : 2;
//2. compute u, v of intersection point
//vec from 1 -> 0
h = iFace * 3;
pnt_array = (vmsVector *) (gameData.segs.vertices + vertList [h+1]);
p1.i = pnt_array->v [ii];
p1.j = pnt_array->v [jj];

pnt_array = (vmsVector *) (gameData.segs.vertices + vertList [h]);
vec0.i = pnt_array->v [ii] - p1.i;
vec0.j = pnt_array->v [jj] - p1.j;

//vec from 1 -> 2
pnt_array = (vmsVector *) (gameData.segs.vertices + vertList [h+2]);
vec1.i = pnt_array->v [ii] - p1.i;
vec1.j = pnt_array->v [jj] - p1.j;

//vec from 1 -> checkPoint
pnt_array = (vmsVector *)pnt;
checkP.i = pnt_array->v [ii];
checkP.j = pnt_array->v [jj];

k1 = -FixDiv (Cross (&checkP, &vec0) + Cross (&vec0, &p1), Cross (&vec0, &vec1));
if (abs(vec0.i) > abs(vec0.j))
	k0 = FixDiv (FixMul (-k1, vec1.i) + checkP.i - p1.i, vec0.i);
else
	k0 = FixDiv (FixMul (-k1, vec1.j) + checkP.j - p1.j, vec0.j);
for (i = 0; i < 3; i++)
	uvls [i] = sideP->uvls [vertnum_list [h+i]];
*u = uvls [1].u + FixMul (k0, uvls [0].u - uvls [1].u) + FixMul (k1, uvls [2].u - uvls [1].u);
*v = uvls [1].v + FixMul (k0, uvls [0].v - uvls [1].v) + FixMul (k1, uvls [2].v - uvls [1].v);
if (l)
	*l = uvls [1].l + FixMul (k0, uvls [0].l - uvls [1].l) + FixMul (k1, uvls [2].l - uvls [1].l);
}

//	-----------------------------------------------------------------------------

grsBitmap *LoadFaceBitmap (short tMapNum, short nFrameNum);

int PixelTranspType (short nTexture, short nOrient, short nFrame, fix u, fix v)
{
	grsBitmap *bmP;
	int bmx, bmy, w, h, offs;
	unsigned char	c;
#if 0
	tBitmapIndex *bmiP;

//	Assert(WALL_IS_DOORWAY(seg, nSide) == WID_TRANSPARENT_WALL);

bmiP = gameData.pig.tex.pBmIndex + (nTexture);
PIGGY_PAGE_IN (*bmiP, gameStates.app.bD1Data);
bmP = BmOverride (gameData.pig.tex.pBitmaps + bmiP->index);
#else
bmP = LoadFaceBitmap (nTexture, nFrame);
#endif
if (bmP->bmProps.flags & BM_FLAG_RLE)
	bmP = rle_expand_texture (bmP);
w = bmP->bmProps.w;
h = ((bmP->bmType == BM_TYPE_ALT) && BM_FRAMES (bmP)) ? w : bmP->bmProps.h; 
if (nOrient == 0) {
	bmx = ((unsigned) f2i (u * w)) % w;
	bmy = ((unsigned) f2i (v * h)) % h;
	}
else if (nOrient == 1) {
	bmx = ((unsigned) f2i ((F1_0 - v) * w)) % w;
	bmy = ((unsigned) f2i (u * h)) % h;
	}
else if (nOrient == 2) {
	bmx = ((unsigned) f2i ((F1_0 - u) * w)) % w;
	bmy = ((unsigned) f2i ((F1_0 - v) * h)) % h;
	}
else {
	bmx = ((unsigned) f2i (v * w)) % w;
	bmy = ((unsigned) f2i ((F1_0 - u) * h)) % h;
	}
offs = bmy * w + bmx;
if (bmP->bmProps.flags & BM_FLAG_TGA) {
	ubyte *p;

	if (bmP->bmBPP == 3)	//no alpha -> no transparency
		return 0;
	p = bmP->bmTexBuf + offs * bmP->bmBPP;
	// check super transparency color
#if 1
	if ((p[0] == 120) && (p[1] == 88) && (p[2] == 128))
#else
	if ((gameOpts->ogl.bGlTexMerge && gameStates.render.textures.bGlsTexMergeOk) ?
	    (p [3] == 1) : ((p[0] == 120) && (p[1] == 88) && (p[2] == 128)))
#endif	    
		return -1;
	// check alpha
	if (!p[3])
		return 1;
	}
else {
	c = bmP->bmTexBuf [offs];
	if (c == SUPER_TRANSP_COLOR) 
		return -1;
	if (c == TRANSPARENCY_COLOR) 
		return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//check if a particular point on a tWall is a transparent pixel
//returns 1 if can pass though the tWall, else 0
int CheckTransWall (vmsVector *pnt, tSegment *segP, short nSide, short iFace)
{
	tSide *sideP = segP->sides + nSide;
	fix	u, v;
	int	nTranspType;

//	Assert(WALL_IS_DOORWAY(segP, nSide) == WID_TRANSPARENT_WALL);
//LogErr ("      FindHitPointUV (%d)...", iFace);
FindHitPointUV (&u, &v, NULL, pnt, segP, nSide, iFace);	//	Don't compute light value.
//LogErr ("done\n");
if (sideP->nOvlTex)	{
	//LogErr ("      PixelTranspType...");
	nTranspType = PixelTranspType (sideP->nOvlTex, sideP->nOvlOrient, sideP->nFrame, u, v);
	//LogErr ("done\n");
	if (nTranspType < 0)
		return 1;
	if (!nTranspType)
		return 0;
	}
//LogErr ("      PixelTranspType...");
nTranspType = PixelTranspType (sideP->nBaseTex, 0, sideP->nFrame, u, v) != 0;
//LogErr ("done\n");
return nTranspType;
}

//	-----------------------------------------------------------------------------
//new function for Mike
//note: gameData.collisions.nSegsVisited must be set to zero before this is called
int SphereIntersectsWall (vmsVector *vPoint, short nSegment, fix rad)
{
	int		faceMask;
	tSegment *segP;

if (nSegment == -1) {
	Error("nSegment == -1 in SphereIntersectsWall()");
	return 0;
	}
if ((gameData.collisions.nSegsVisited < 0) || (gameData.collisions.nSegsVisited > MAX_SEGS_VISITED))
	gameData.collisions.nSegsVisited = 0;
gameData.collisions.segsVisited [gameData.collisions.nSegsVisited++] = nSegment;
faceMask = GetSegMasks (vPoint, nSegment, rad).faceMask;
segP = gameData.segs.segments + nSegment;
if (faceMask != 0) {				//on the back of at least one face
	int nSide, bit, iFace, nChild, i;
	int nFaceHitType;      //in what way did we hit the face?
	int nFaces, vertList [6];

//for each face we are on the back of, check if intersected
	for (nSide = 0, bit = 1; (nSide < 6) && (faceMask >= bit); nSide++) {
		for (iFace = 0; iFace < 2; iFace++, bit <<= 1) {
			if (faceMask & bit) {            //on the back of this iFace
				//did we go through this tWall/door?
				nFaces = CreateAbsVertexLists (vertList, SEG_IDX (segP), nSide);
				nFaceHitType = CheckSphereToSegFace (vPoint, nSegment, nSide, iFace, 
															 (nFaces == 1) ? 4 : 3, rad, vertList);
				if (nFaceHitType) {            //through this tWall/door
					//if what we have hit is a door, check the adjoining segP
					nChild = segP->children [nSide];
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
//Returns true if the tObject is through any walls
int ObjectIntersectsWall (tObject *objP)
{
return SphereIntersectsWall (&objP->position.vPos, objP->nSegment, objP->size);
}

//------------------------------------------------------------------------------

int CanSeePoint (tObject *objP, vmsVector *vSource, vmsVector *vDest, short nSegment)
{
	tVFIQuery	fq;
	int			nHitType;
	tFVIData		hit_data;

	//see if we can see this tPlayer

fq.p0 = vSource;
fq.p1 = vDest;
fq.radP0 =
fq.radP1 = 0;
fq.thisObjNum = objP ? OBJ_IDX (objP) : -1;
fq.flags = FQ_TRANSWALL;
if (SPECTATOR (objP))
	fq.startSeg = FindSegByPoint (&objP->position.vPos, objP->nSegment, 1, 0);
else
	fq.startSeg = objP ? objP->nSegment : nSegment;
fq.ignoreObjList = NULL;
nHitType = FindVectorIntersection (&fq, &hit_data);
return nHitType != HIT_WALL;
}

//	-----------------------------------------------------------------------------
//eof
