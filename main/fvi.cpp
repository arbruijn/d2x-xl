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
#include "rle.h"
#include "gameseg.h"
#include "interp.h"
#include "hitbox.h"
#include "network.h"
#include "fvi_a.h"

int CheckSphereToFace (vmsVector *pnt, fix rad, vmsVector *vertList, vmsVector *vNormal, int nVerts);

//#define _DEBUG

//	-----------------------------------------------------------------------------

inline fix RegisterHit (vmsVector *vBestHit, vmsVector *vCurHit, vmsVector *vPos, fix dMax)
{
   fix d = vmsVector::Dist(*vPos, *vCurHit);

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

p1 = pv1->ToFloat();
p2 = pv2->ToFloat();
p3 = pv3->ToFloat();
d21 = p2 - p1;
if (!(m = d21[X] * d21[X] + d21[Y] * d21[Y] + d21[Z] * d21[Z]))
	return 0;
d31 = p3 - p1;
u = fVector::Dot(d31, d21);
u /= m;
/*
h[X] = p1[X] + u * d21[X];
h[Y] = p1[Y] + u * d21[Y];
h[Z] = p1[Z] + u * d21[Z];
*/
h = p1 + u * d21;

// limit the intersection to [p1,p2]
v[0] = p1 - h;
v[1] = p2 - h;
m = fVector::Dot(v[0], v[1]);
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

w = *p0 - *vPlanePoint;
d = *p1 - *p0;
num = vmsVector::Dot(*vPlaneNorm, w) - rad;
den = -vmsVector::Dot(*vPlaneNorm, d);
if (!den) {
	fVector	nf, df;
	float denf;
	nf = vPlaneNorm->ToFloat();
	df = d.ToFloat();
	denf = -fVector::Dot(nf, df);
	denf = -fVector::Dot(nf, df);
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
d *= FixDiv(num, den);
(*hitP) = (*p0) + d;
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

w = *vPlanePoint - *p0;
d = *p1 - *p0;
num = vmsVector::Dot(*vPlaneNorm, w);
den = vmsVector::Dot(*vPlaneNorm, d);
if (!den)
	return 0;
if (labs (num) > labs (den))
	return 0;
//do check for potential overflow
if (labs (num) / (f1_0 / 2) >= labs (den))
	return 0;
d *= FixDiv(num, den);
(*hitP) = (*p0) + d;
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
d[0] = vHit - *p0;
d[1] = vHit - *p1;
if (vmsVector::Dot(d[0], d[1]) >= 0)
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
		if (vmsVector::Dot(pf1->n[1], pf2->n[1]) >= 0)
			continue;
#endif
		if (FindQuadQuadIntersection (&vHit, pf1->v, pf1->n + 1, pf2->v, pf2->n + 1, vPos)) {
			dMax = RegisterHit (hitP, &vHit, vPos, dMax);
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

typedef struct vec2d {
	fix i, j;
} vec2d;

//given largest component of Normal, return i & j
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
//project polygon onto plane by finding largest component of Normal
t[X] = labs ((*vNormal)[0]);
t[Y] = labs ((*vNormal)[1]);
t[Z] = labs ((*vNormal)[2]);
if (t[X] > t[Y])
	if (t[X] > t[Z])
		biggest = 0;
	else
		biggest = 2;
else if (t[Y] > t[Z])
	biggest = 1;
else
	biggest = 2;
if ((*vNormal)[biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = (*checkP)[i];
check_j = (*checkP)[j];
for (nEdge = nEdgeMask = 0; nEdge < nVerts; nEdge++) {
	v0 = vertList + nEdge;
	v1 = vertList + ((nEdge + 1) % nVerts);
	vEdge.i = (*v1)[i] - (*v0)[i];
	vEdge.j = (*v1)[j] - (*v0)[j];
	vCheck.i = check_i - (*v0)[i];
	vCheck.j = check_j - (*v0)[j];
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
vCheck = checkP - *v0;
xEdgeLen = vmsVector::NormalizedDir(vEdge, *v1, *v0);
//find point dist from planes of ends of edge
d = vmsVector::Dot(vEdge, vCheck);
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
	vClosestPoint = *v0 + vEdge * d;
	}
dist = vmsVector::Dist(checkP, vClosestPoint);
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
	v1 = *vNormal * (-rad);
	v1 += *p0;
	bCheckRad = rad;
	rad = 0;
	p1 = &v1;
	}
if (!(pli = FindPlaneLineIntersection (intP, vertList, vNormal, p0, p1, rad)))
	return IT_NONE;
hitP = *intP;
//if rad != 0, project the point down onto the plane of the polygon
if (rad)
	hitP += *vNormal * (-rad);
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
		d = VmLinePointDist(*a, *b, *p0);
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
//project polygon onto plane by finding largest component of Normal
t[X] = labs (vNormal[0]);
t[Y] = labs (vNormal[1]);
t[Z] = labs (vNormal[2]);
if (t[X] > t[Y])
	if (t[X] > t[Z])
		biggest = 0;
	else
		biggest = 2;
else if (t[Y] > t[Z])
	biggest = 1;
else
	biggest = 2;
if (vNormal[biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = (*checkP)[i];
check_j = (*checkP)[j];
for (nEdge = nEdgeMask = 0; nEdge < nv; nEdge++) {
	if (gameStates.render.bRendering) {
		v0 = &gameData.segs.points [vertList [iFace * 3 + nEdge]].p3_vec;
		v1 = &gameData.segs.points [vertList [iFace * 3 + ((nEdge + 1) % nv)]].p3_vec;
		}
	else {
		v0 = gameData.segs.vertices + vertList [iFace * 3 + nEdge];
		v1 = gameData.segs.vertices + vertList [iFace * 3 + ((nEdge + 1) % nv)];
		}
	vEdge.i = (*v1)[i] - (*v0)[i];
	vEdge.j = (*v1)[j] - (*v0)[j];
	vCheck.i = check_i - (*v0)[i];
	vCheck.j = check_j - (*v0)[j];
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
vCheck = checkP - *v0;
xEdgeLen = vmsVector::NormalizedDir(vEdge, *v1, *v0);
//find point dist from planes of ends of edge
d = vmsVector::Dot(vEdge, vCheck);
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
	vClosestPoint = *v0 + vEdge * d;
	}
dist = vmsVector::Dist(checkP, vClosestPoint);
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
#if 1 //def _DEBUG
if (nFaces <= iFace)
	nFaces = CreateAbsVertexLists (vertexList, nSegment, nSide);
#endif
#if 1
if (p1 == p0) {
#if 0
	return CheckSphereToSegFace (p0, nSegment, nSide, iFace, nv, rad, vertexList);
#else
	if (!rad)
		return IT_NONE;
	v1 = vNormal * (-rad);
	v1 += *p0;
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
//PrintLog ("         FindPlaneLineIntersection...");
pli = FindPlaneLineIntersection (newP,
											gameStates.render.bRendering ?
											&gameData.segs.points [nVertex].p3_vec :
											gameData.segs.vertices + nVertex,
											&vNormal, p0, p1, rad);
//PrintLog ("done\n");
if (!pli)
	return IT_NONE;
checkP = *newP;
//if rad != 0, project the point down onto the plane of the polygon
if (rad)
	checkP += vNormal * (-rad);
if ((pli = CheckSphereToSegFace (&checkP, nSegment, nSide, iFace, nv, rad, vertexList)))
	return pli;
if (bCheckRad) {
	int			i, d;
	vmsVector	*a, *b;

	b = gameData.segs.vertices + vertexList [0];
	for (i = 1; i <= 4; i++) {
		a = b;
		b = gameData.segs.vertices + vertexList [i % 4];
		d = VmLinePointDist(*a, *b, *p0);
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

//PrintLog ("         VmVecSub\n");
det[RVEC] = *p2 - *p1;
//PrintLog ("         VmVecCrossProd\n");
det[FVEC] = vmsVector::Cross(*v1, *v2);
//PrintLog ("         fVector::Dot\n");
cross_mag2 = vmsVector::Dot(det[FVEC], det[FVEC]);
if (!cross_mag2)
	return 0;			//lines are parallel
det[UVEC] = *v2;
d = det.Det();
if (oflow_check (d, cross_mag2))
	return 0;
//PrintLog ("         FixDiv (%d)\n", cross_mag2);
*t1 = FixDiv (d, cross_mag2);
det[UVEC] = *v1;
//PrintLog ("         CalcDetValue\n");
d = det.Det();
if (oflow_check (d, cross_mag2))
	return 0;
//PrintLog ("         FixDiv (%d)\n", cross_mag2);
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
	//PrintLog ("      CheckLineToSegFace ...");
	h = CheckLineToSegFace (newP, p0, p1, nSegment, nSide, iFace, nv, rad);
	//PrintLog ("done\n");
	return h;
	}
//calc some basic stuff
if ((SEG_IDX (segP)) == -1)
	Error ("nSegment == -1 in SpecialCheckLineToSegFace()");
//PrintLog ("      CreateAbsVertexLists ...");
nFaces = CreateAbsVertexLists (vertList, nSegment, nSide);
//PrintLog ("done\n");
move_vec = *p1 - *p0;
//figure out which edge(side) to check against
//PrintLog ("      CheckPointToSegFace ...\n");
if (!(nEdgeMask = CheckPointToSegFace (p0, nSegment, nSide, iFace, nv, vertList))) {
	//PrintLog ("      CheckLineToSegFace ...");
	return CheckLineToSegFace (newP, p0, p1, nSegment, nSide, iFace, nv, rad);
	//PrintLog ("done\n");
	}
for (nEdge = 0; !(nEdgeMask & 1); nEdgeMask >>= 1, nEdge++)
	;
//PrintLog ("      setting edge vertices (%d, %d)...\n", vertList [iFace * 3 + nEdge], vertList [iFace * 3 + ((nEdge + 1) % nv)]);
edge_v0 = gameData.segs.vertices + vertList [iFace * 3 + nEdge];
edge_v1 = gameData.segs.vertices + vertList [iFace * 3 + ((nEdge + 1) % nv)];
//PrintLog ("      setting edge vector...\n");
edge_vec = *edge_v1 - *edge_v0;
//is the start point already touching the edge?
//first, find point of closest approach of vec & edge
//PrintLog ("      getting edge length...\n");
edge_len = vmsVector::Normalize(edge_vec);
//PrintLog ("      getting move length...\n");
move_len = vmsVector::Normalize(move_vec);
//PrintLog ("      CheckLineToLine...");
CheckLineToLine (&edge_t, &move_t, edge_v0, &edge_vec, p0, &move_vec);
//PrintLog ("done\n");
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
closest_point_edge = *edge_v0 + edge_vec * edge_t2;
closest_point_move = *p0 + move_vec * move_t2;
//find dist between closest points
//PrintLog ("      computing closest dist.p...\n");
closestDist = vmsVector::Dist(closest_point_edge, closest_point_move);
//could we hit with this dist?
//note massive tolerance here
if (closestDist < (rad * 9) / 10) {		//we hit.  figure out where
	//now figure out where we hit
	*newP = *p0 + move_vec * (move_t-rad);
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
	vmsVector	d, dn, w, vClosestPoint;
	fix			mag_d, dist, wDist, intDist;

//this routine could be optimized if it's taking too much time!

d = *p1 - *p0;
w = *vSpherePos - *p0;
dn = d; mag_d = vmsVector::Normalize(dn);
if (mag_d == 0) {
	intDist = w.Mag();
	*intP = *p0;
	return ((xSphereRad < 0) || (intDist < xSphereRad)) ? intDist : 0;
	}
wDist = vmsVector::Dot(dn, w);
if (wDist < 0)
	return 0;	//moving away from tObject
if (wDist > mag_d + xSphereRad)
	return 0;	//cannot hit
vClosestPoint = *p0 + dn * wDist;
dist = vmsVector::Dist(vClosestPoint, *vSpherePos);
if  (dist < xSphereRad) {
	fix	dist2, radius2, nShorten;

	dist2 = FixMul (dist, dist);
	radius2 = FixMul (xSphereRad, xSphereRad);
	nShorten = FixSqrt (radius2 - dist2);
	intDist = wDist - nShorten;
	if ((intDist > mag_d) || (intDist < 0)) {
		//past one or the other end of vector, which means we're inside
		*intP = *p0;		//don't move at all
		return 1;
		}
	*intP = *p0 + dn * intDist;         //calc intersection point
	return intDist;
	}
return 0;
}

//	-----------------------------------------------------------------------------

fix CheckHitboxToHitbox (vmsVector *intP, tObject *objP1, tObject *objP2, vmsVector *p0, vmsVector *p1)
{
	vmsVector		vHit, vPos = objP2->info.position.vPos;
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
	nModels1 = pmhb1->nHitboxes;
	nModels2 = pmhb2->nHitboxes;
	}
#if DBG
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
#if DBG
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
	nModels = pmhb->nHitboxes;
	}
TransformHitboxes (objP, vPos, hb);
for (; iModel <= nModels; iModel++) {
	for (i = 0, pf = hb [iModel].faces; i < 6; i++, pf++) {
#if 0
		dot = vmsVector::Dot(pf->n + 1, pn);
		if (dot >= 0)
			continue;	//shield face facing away from vector
#endif
		h = CheckLineToFace (&hitP, p0, p1, pf->v, pf->n + 1, 4, rad);
		if (h) {
		h = CheckLineToFace (&hitP, p0, p1, pf->v, pf->n + 1, 4, rad);
			v = hitP - *p0;
			d = vmsVector::Normalize(v);
#if 0
			dot = vmsVector::Dot(pf->n + 1, pn);
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

static inline int UseHitbox (tObject *objP)
{
return (objP->info.renderType == RT_POLYOBJ) && (objP->rType.polyObjInfo.nModel >= 0); // && ((objP->info.nType != OBJ_WEAPON) || gameData.objs.bIsMissile [objP->info.nId]);
}

//	-----------------------------------------------------------------------------

static inline int UseSphere (tObject *objP)
{
	int nType = objP->info.nType;

return (nType == OBJ_MONSTERBALL) || (nType == OBJ_HOSTAGE) || (nType == OBJ_POWERUP);
}

//	-----------------------------------------------------------------------------
//determine if a vector intersects with an tObject
//if no intersects, returns 0, else fills in intP and returns dist
fix CheckVectorToObject (vmsVector *intP, vmsVector *p0, vmsVector *p1, fix rad,
								 tObject *thisObjP, tObject *otherObjP)
{
	fix			size, dist;
	vmsVector	hitP, v0, v1, vn, vPos;
	int			bThisPoly, bOtherPoly;

if (rad < 0)
	size = 0;
else {
	size = thisObjP->info.xSize;
	if ((thisObjP->info.nType == OBJ_ROBOT) && ROBOTINFO (thisObjP->info.nId).attackType)
		size = 3 * size / 4;
	//if obj is tPlayer, and bumping into other tPlayer or a weapon of another coop tPlayer, reduce radius
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
if (EGI_FLAG (nHitboxes, 0, 0, 0) &&
	 !(UseSphere (thisObjP) || UseSphere (otherObjP)) &&
	 (bThisPoly || bOtherPoly)) {
	VmPointLineIntersection(hitP, *p0, *p1, vPos, 0);
	dist = vmsVector::Dist(hitP, vPos);
	if (dist > 2 * (thisObjP->info.xSize + otherObjP->info.xSize))
		return 0;
	// check hitbox collisions for all polygonal objects
	if (bThisPoly && bOtherPoly) {
		if (!(dist = CheckHitboxToHitbox (&hitP, otherObjP, thisObjP, p0, p1))) {
			if (!vmsVector::Dist(*p0, *p1))
				return 0;
			dist = CheckVectorToHitbox (&hitP, p0, p1, &vn, NULL, thisObjP, 0);
			if ((dist == 0x7fffffff) || (dist > otherObjP->info.xSize))
				return 0;
			}
		CheckHitboxToHitbox(&hitP, otherObjP, thisObjP, p0, p1);
//		VmPointLineIntersection(hitP, *p0, *p1, hitP, thisObjP->info.position.vPos, 1);
		VmPointLineIntersection(hitP, *p0, *p1, hitP, 1);
		}
	else {
		if (bThisPoly) {
		// *thisObjP (stationary) has hitboxes, *otherObjP (moving) a hit sphere. To detect whether the sphere
		// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			vn = *p1-*p0;
			vmsVector::Normalize(vn);
			if (0x7fffffff == (dist = CheckVectorToHitbox (&hitP, p0, p1, &vn, NULL, thisObjP, otherObjP->info.xSize)))
				return 0;
//			VmPointLineIntersection(hitP, *p0, *p1, hitP, &otherObjP->info.position.vPos, 1);
			VmPointLineIntersection(hitP, *p0, *p1, hitP, 1);
			}
		else {
		// *otherObjP (moving) has hitboxes, *thisObjP (stationary) a hit sphere. To detect whether the sphere
		// intersects with the hitbox, check whether the radius line of *thisObjP intersects any of the hitboxes.
			v0 = thisObjP->info.position.vPos;
			vn = otherObjP->info.position.vPos - v0;
			vmsVector::Normalize(vn);
			v1 = v0 + vn * thisObjP->info.xSize;
			if (0x7fffffff == (dist = CheckVectorToHitbox (&hitP, &v0, &v0, &vn, p1, otherObjP, thisObjP->info.xSize)))
				return 0;
//			VmPointLineIntersection(hitP, *p0, *p1, hitP, &thisObjP->info.position.vPos, 1);
			VmPointLineIntersection(hitP, *p0, *p1, hitP, 1);
			}
		}
	}
else {
	if (!(dist = CheckVectorToSphere1 (&hitP, p0, p1, &vPos, size + rad)))
		return 0;
	}
*intP = hitP;
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

#define FVI_NEWCODE 2

int CheckTransWall (vmsVector *vPoint, tSegment *segP, short nSide, short iFace);

int FVICompute (vmsVector *vIntP, short *intS, vmsVector *p0, short nStartSeg, vmsVector *p1,
					 fix radP0, fix radP1, short nThisObject, short *ignoreObjList, int flags, short *segList,
					 short *nSegments, int nEntrySeg)
{
	tSegment		*segP;				//the tSegment we're looking at
	int			startMask, endMask, centerMask;	//mask of faces
	short			nObject, nSegment;
	tSegMasks	masks;
	vmsVector	vHitPoint, vClosestHitPoint; 	//where we hit
	fix			d, dMin = 0x7fffffff;					//distance to hit point
	int			nObjSegList [7], nObjSegs, iObjSeg, i;
	int			nHitType = HIT_NONE;							//what sort of hit
	int			nHitSegment = -1;
	int			nHitNoneSegment = -1;
	int			nHitNoneSegs = 0;
	int			hitNoneSegList [MAX_FVI_SEGS];
	int			nCurNestLevel = gameData.collisions.hitData.nNestCount;
#if FVI_NEWCODE
	int			nFudgedRad;
	int			nFaces;
#if 1
	int			nFaceHitType;
#endif
	int			widResult;
	int			nThisType, nOtherType;
	tObject		*otherObjP,
					*thisObjP = (nThisObject < 0) ? NULL : OBJECTS + nThisObject;
#endif

vClosestHitPoint.SetZero ();
//PrintLog ("Entry FVICompute\n");
if (flags & FQ_GET_SEGLIST)
	*segList = nStartSeg;
*nSegments = 1;
gameData.collisions.hitData.nNestCount++;
//first, see if vector hit any objects in this tSegment
nThisType = (nThisObject < 0) ? -1 : OBJECTS [nThisObject].info.nType;
#if 1
if (flags & FQ_CHECK_OBJS) {
	//PrintLog ("   checking objects...");
	nObjSegList [0] = nStartSeg;
	nObjSegs = 1;
#	if DBG
	if ((thisObjP->info.nType == OBJ_WEAPON) && (thisObjP->info.nSegment == gameData.objs.consoleP->info.nSegment))
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
		short nSegment = nObjSegList [iObjSeg];
		segP = gameData.segs.segments + nSegment;
		for (nObject = gameData.segs.objects [nSegment]; nObject != -1; nObject = otherObjP->info.nNextInSeg) {
			otherObjP = OBJECTS + nObject;
			nOtherType = otherObjP->info.nType;
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
				if (ROBOTINFO (thisObjP->info.nId).attackType)
					nFudgedRad = (radP1 * 3) / 4;
				}
			//if obj is tPlayer, and bumping into other tPlayer or a weapon of another coop tPlayer, reduce radius
			if ((nThisType == OBJ_PLAYER) &&
				 ((nOtherType == OBJ_PLAYER) ||
				 (IsCoopGame && (nOtherType == OBJ_WEAPON) && (otherObjP->cType.laserInfo.parent.nType == OBJ_PLAYER))))
				nFudgedRad = radP1 / 2;
			if (flags & FQ_ANY_OBJECT)
				d = CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);
			else
				d = CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);
			if (d && (d < dMin)) {
#if DBG
				CheckVectorToObject (&vHitPoint, p0, p1, nFudgedRad, otherObjP, thisObjP);
#endif
				gameData.collisions.hitData.nObject = nObject;
				Assert(gameData.collisions.hitData.nObject != -1);
				dMin = d;
				vClosestHitPoint = vHitPoint;
				nHitType = HIT_OBJECT;
#if DBG
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
startMask = GetSegMasks (*p0, nStartSeg, radP0).faceMask;
masks = GetSegMasks (*p1, nStartSeg, radP1);    //on back of which faces?
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
			if (iFace >= nFaces)
				continue;
			//did we go through this tWall/door?
			nFaceHitType = (startMask & bit)	?	//start was also though.  Do extra check
				SpecialCheckLineToSegFace (&vHitPoint, p0, p1, nStartSeg, nSide, iFace, 5 - nFaces, radP1) :
				CheckLineToSegFace (&vHitPoint, p0, p1, nStartSeg, nSide, iFace, 5 - nFaces, radP1);
#if 0
			if (!nFaceHitType)
				continue;
#endif
			widResult = WALL_IS_DOORWAY (segP, nSide, (nThisObject < 0) ? NULL : OBJECTS + nThisObject);
			//PrintLog ("done\n");
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
				//PrintLog ("   check next seg (%d)\n", nNewSeg);
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
						d = vmsVector::Dist(subHitPoint, *p0);
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
#if 1
				if (nFaceHitType)
#endif
					{
#if DBG
					int vertList [6];
					int nFaces = CreateAbsVertexLists (vertList, SEG_IDX (segP), nSide);
					if (iFace >= nFaces)
						d = dMin + 1;
					else
#endif
					//is this the closest hit?
					d = vmsVector::Dist(vHitPoint, *p0);
					if (d < dMin) {
						dMin = d;
						vClosestHitPoint = vHitPoint;
						nHitType = HIT_WALL;
#if 1
						gameData.collisions.hitData.vNormal = segP->sides [nSide].normals [iFace];
						gameData.collisions.hitData.nNormals = 1;
#else
						VmVecInc (&gameData.collisions.hitData.vNormal, segP->sides [nSide].normals + iFace);
						gameData.collisions.hitData.nNormals++;
#endif
						if (!GetSegMasks (vHitPoint, nStartSeg, radP1).centerMask)
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
			*intS = gameData.collisions.hitData.nSegment2;
		else
			*intS = nHitNoneSegment;
	else
		*intS = nHitSegment;
	}
Assert(!(nHitType==HIT_OBJECT && gameData.collisions.hitData.nObject==-1));
//PrintLog ("Exit FVICompute\n");
return nHitType;
}

#define Cross2D(v0, v1) (FixMul((v0).i, (v1).j) - FixMul((v0).j, (v1).i))

//	-----------------------------------------------------------------------------

//Find out if a vector intersects with anything.
//Fills in hitData, an tFVIData structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisObjNum 		used to prevent an tObject with colliding with itself
//  ingore_obj			ignore collisions with this tObject
//  check_objFlag	determines whether collisions with OBJECTS are checked
//Returns the hitData->nHitType
int FindVectorIntersection (tFVIQuery *fq, tFVIData *hitData)
{
	int			nHitType, nNewHitType;
	short			nHitSegment, nHitSegment2;
	vmsVector	vHitPoint;
	int			i;
	tSegMasks	masks;

Assert(fq->ignoreObjList != (short *)(-1));
gameData.collisions.hitData.vNormal.SetZero();
gameData.collisions.hitData.nNormals = 0;
Assert((fq->startSeg <= gameData.segs.nLastSegment) && (fq->startSeg >= 0));

gameData.collisions.hitData.nSegment = -1;
gameData.collisions.hitData.nSide = -1;
gameData.collisions.hitData.nObject = -1;

//check to make sure start point is in seg its supposed to be in
//Assert(check_point_in_seg(p0, startseg, 0).centerMask==0);	//start point not in seg

// gameData.objs.viewerP is not in tSegment as claimed, so say there is no hit.
masks = GetSegMasks (*fq->p0, fq->startSeg, 0);
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
//!!nHitSegment = FindSegByPos(&vHitPoint, fq->startSeg, 1, 0);
if ((nHitSegment2 != -1) && !GetSegMasks (vHitPoint, nHitSegment2, 0).centerMask)
	nHitSegment = nHitSegment2;
else {
	nHitSegment = FindSegByPos (vHitPoint, fq->startSeg, 1, 0);
	}
//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
if ((nHitType == HIT_WALL) && (nHitSegment == -1))
	if ((gameData.collisions.hitData.nSegment2 != -1) && !GetSegMasks (vHitPoint, gameData.collisions.hitData.nSegment2, 0).centerMask)
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
//finds the uv coords of the given point on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
void FindHitPointUV (fix *u, fix *v, fix *l, vmsVector *pnt, tSegment *segP, int nSide, int iFace)
{
	vmsVector	*vPoints;
	vmsVector	vNormals;
	int			nSegment = SEG_IDX (segP);
	int			nFaces;
	int			biggest, ii, jj;
	tSide			*sideP = segP->sides + nSide;
	int			vertList [6], vertNumList [6];
 	vec2d			p1, vec0, vec1, checkP;
	tUVL			uvls [3];
	fix			k0, k1;
	int			h;

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
if (iFace >= nFaces) {
	PrintLog ("invalid face number in FindHitPointUV\n");
	*u = *v = 0;
	return;
	}
CreateAllVertNumLists (&nFaces, vertNumList, nSegment, nSide);
//now the hard work.
//1. find what plane to project this tWall onto to make it a 2d case
memcpy (&vNormals, sideP->normals + iFace, sizeof (vmsVector));
biggest = 0;
if (abs (vNormals[1]) > abs (vNormals[biggest]))
	biggest = 1;
if (abs (vNormals[2]) > abs (vNormals[biggest]))
	biggest = 2;
ii = (biggest == 0);
jj = (biggest == 2) ? 1 : 2;
//2. compute u, v of intersection point
//vec from 1 -> 0
h = iFace * 3;
vPoints = (vmsVector *) (gameData.segs.vertices + vertList [h+1]);
p1.i = (*vPoints)[ii];
p1.j = (*vPoints)[jj];

vPoints = (vmsVector *) (gameData.segs.vertices + vertList [h]);
vec0.i = (*vPoints)[ii] - p1.i;
vec0.j = (*vPoints)[jj] - p1.j;

//vec from 1 -> 2
vPoints = (vmsVector *) (gameData.segs.vertices + vertList [h+2]);
vec1.i = (*vPoints)[ii] - p1.i;
vec1.j = (*vPoints)[jj] - p1.j;

//vec from 1 -> checkPoint
//vPoints = (vmsVector *)pnt;
checkP.i = (*pnt)[ii];
checkP.j = (*pnt)[jj];

#if 1 // the MSVC 9 optimizer doesn't like the code in the else branch ...
ii = Cross2D (checkP, vec0) + Cross2D (vec0, p1);
jj = Cross2D (vec0, vec1);
k1 = -FixDiv (ii, jj);
#else
k1 = -FixDiv (Cross2D (checkP, vec0) + Cross2D (vec0, p1), Cross2D (vec0, vec1));
#endif
if (abs (vec0.i) > abs (vec0.j))
	k0 = FixDiv (FixMul (-k1, vec1.i) + checkP.i - p1.i, vec0.i);
else
	k0 = FixDiv (FixMul (-k1, vec1.j) + checkP.j - p1.j, vec0.j);
uvls [0] = sideP->uvls [vertNumList [h]];
uvls [1] = sideP->uvls [vertNumList [h + 1]];
uvls [2] = sideP->uvls [vertNumList [h + 2]];
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
	bmx = ((unsigned) X2I (u * w)) % w;
	bmy = ((unsigned) X2I (v * h)) % h;
	}
else if (nOrient == 1) {
	bmx = ((unsigned) X2I ((F1_0 - v) * w)) % w;
	bmy = ((unsigned) X2I (u * h)) % h;
	}
else if (nOrient == 2) {
	bmx = ((unsigned) X2I ((F1_0 - u) * w)) % w;
	bmy = ((unsigned) X2I ((F1_0 - v) * h)) % h;
	}
else {
	bmx = ((unsigned) X2I (v * w)) % w;
	bmy = ((unsigned) X2I ((F1_0 - u) * h)) % h;
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
//PrintLog ("      FindHitPointUV (%d)...", iFace);
FindHitPointUV (&u, &v, NULL, pnt, segP, nSide, iFace);	//	Don't compute light value.
//PrintLog ("done\n");
if (sideP->nOvlTex)	{
	//PrintLog ("      PixelTranspType...");
	nTranspType = PixelTranspType (sideP->nOvlTex, sideP->nOvlOrient, sideP->nFrame, u, v);
	//PrintLog ("done\n");
	if (nTranspType < 0)
		return 1;
	if (!nTranspType)
		return 0;
	}
//PrintLog ("      PixelTranspType...");
nTranspType = PixelTranspType (sideP->nBaseTex, 0, sideP->nFrame, u, v) != 0;
//PrintLog ("done\n");
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
faceMask = GetSegMasks (*vPoint, nSegment, rad).faceMask;
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
return SphereIntersectsWall (&objP->info.position.vPos, objP->info.nSegment, objP->info.xSize);
}

//------------------------------------------------------------------------------

int CanSeePoint (tObject *objP, vmsVector *vSource, vmsVector *vDest, short nSegment)
{
	tFVIQuery	fq;
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
	fq.startSeg = FindSegByPos (objP->info.position.vPos, objP->info.nSegment, 1, 0);
else
	fq.startSeg = objP ? objP->info.nSegment : nSegment;
fq.ignoreObjList = NULL;
nHitType = FindVectorIntersection (&fq, &hit_data);
return nHitType != HIT_WALL;
}

//	-----------------------------------------------------------------------------------------------------------
//	Determine if two OBJECTS are on a line of sight.  If so, return true, else return false.
//	Calls fvi.
int ObjectToObjectVisibility (tObject *objP1, tObject *objP2, int transType)
{
	tFVIQuery	fq;
	tFVIData		hit_data;
	int			fate, nTries = 0, bSpectate = SPECTATOR (objP1);

do {
	if (nTries++)
		fq.startSeg		= bSpectate ? FindSegByPos (gameStates.app.playerPos.vPos, gameStates.app.nPlayerSegment, 1, 0) :
							  FindSegByPos (objP1->info.position.vPos, objP1->info.nSegment, 1, 0);
	else
		fq.startSeg		= bSpectate ? gameStates.app.nPlayerSegment : objP1->info.nSegment;
	fq.p0					= bSpectate ? &gameStates.app.playerPos.vPos : &objP1->info.position.vPos;
	fq.p1					= SPECTATOR (objP2) ? &gameStates.app.playerPos.vPos : &objP2->info.position.vPos;
	fq.radP0				=
	fq.radP1				= 0x10;
	fq.thisObjNum		= OBJ_IDX (objP1);
	fq.ignoreObjList	= NULL;
	fq.flags				= transType;
	fate = FindVectorIntersection (&fq, &hit_data);
	}
while ((fate == HIT_BAD_P0) && (nTries < 2));
return fate == HIT_NONE;
}

//	-----------------------------------------------------------------------------
//eof
