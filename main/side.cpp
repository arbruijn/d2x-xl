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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>	//	for memset ()

#include "u_mem.h"
#include "descent.h"
#include "error.h"
#include "mono.h"
#include "segmath.h"
#include "byteswap.h"
#include "light.h"
#include "segment.h"
#include "renderlib.h"
#include "collision_math.h"

// How far a point can be from a plane, and still be "in" the plane

extern bool bNewFileFormat;

// ------------------------------------------------------------------------------------------

CFixVector& CSide::Normal (int nFace)
{ 
return gameStates.render.bRendering ? m_rotNorms [nFace] : m_normals [nFace]; 
}

// ------------------------------------------------------------------------------------------
// Compute the center point of a CSide of a CSegment.
//	The center point is defined to be the average of the 4 points defining the CSide.
void CSide::ComputeCenter (void)
{
CFloatVector vCenter;
vCenter.SetZero ();
for (int i = 0; i < m_nCorners; i++)
	vCenter += FVERTICES [m_vertices [i]];
vCenter /= m_nCorners;
m_vCenter.Assign (vCenter);
// make sure side center is inside segment
CFixVector v0 = m_vCenter + m_normals [2];
CFixVector v1 = m_vCenter - m_normals [2];
#if 1
FindPlaneLineIntersection (m_vCenter, &VERTICES [m_vertices [0]], &m_normals [0], &v0, &v1, 0, false);
#else
CFixVector c0, c1;
FindPlaneLineIntersection (c0, &VERTICES [m_vertices [0]], &m_normals [0], &v0, &v1, 0, false);
FindPlaneLineIntersection (c1, &VERTICES [m_vertices [3]], &m_normals [1], &v0, &v1, 0, false);
if (c0 == c1) 
	m_vCenter = c0;
else {
	v0 = c0 - c1;
	CFixVector::Normalize (v0);
	if (CFixVector::Dot (v0, m_normals [2]) < 0)
		m_vCenter = c0;
	else
		m_vCenter = c1;
	}
#endif
}

// ------------------------------------------------------------------------------------------

void CSide::ComputeRads (void)
{
m_rads [0] = 0x7fffffff;
m_rads [1] = 0;
for (int i = 0; i < m_nCorners; i++) {
	CFixVector v = CFixVector::Avg (VERTICES [m_vertices [i]], VERTICES [m_vertices [(i + 1) % m_nCorners]]);
	fix d = CFixVector::Dist (v, m_vCenter);
	if (m_rads [0] > d)
		m_rads [0] = d;
	d = CFixVector::Dist (m_vCenter, VERTICES [m_vertices [i]]);
	if (m_rads [1] < d)
		m_rads [1] = d;
	}
}

// -----------------------------------------------------------------------------------
//	Given a CSide, return the number of faces
int CSide::FaceCount (void)
{
if (m_nType == SIDE_IS_QUAD)
	return 1;
if ((m_nType == SIDE_IS_TRI_02) || (m_nType == SIDE_IS_TRI_13))
	return 2;
Error ("Illegal side type = %i\n", m_nType);
return -1;
}

// -------------------------------------------------------------------------------

void CSide::SetupCorners (ushort* verts, ushort* index)
{
m_nCorners = 0;
for (int i = 0; i < 4; i++)
	if (verts [index [i]])
		m_corners [m_nCorners++] = verts [index [i]];
}

// -------------------------------------------------------------------------------

void CSide::SetupVertexList (ushort* verts, ushort* index)
{
m_nFaces = -1;
if ((m_nType == SIDE_IS_QUAD) || (m_nType == SIDE_IS_TRIANGLE)) {
	m_vertices [0] = verts [index [0]];
	m_vertices [1] = verts [index [1]];
	m_vertices [2] = verts [index [2]];
	m_vertices [3] = verts [index [3 % m_nCorners]];
	m_nFaces = 1;
	}
else if (m_nType == SIDE_IS_TRI_02) {
	m_vertices [0] =
	m_vertices [5] = verts [index [0]];
	m_vertices [1] = verts [index [1]];
	m_vertices [2] =
	m_vertices [3] = verts [index [2]];
	m_vertices [4] = verts [index [3]];
	m_nMinVertex [1] = min (m_vertices [1], m_vertices [4]);
	m_nFaces = 2;
	}
else if (m_nType == SIDE_IS_TRI_13) {
	m_vertices [0] =
	m_vertices [5] = verts [index [3]];
	m_vertices [1] = verts [index [0]];
	m_vertices [2] =
	m_vertices [3] = verts [index [1]];
	m_vertices [4] = verts [index [2]];
	m_nFaces = 2;
	}
else {
	return;
	}

m_nMinVertex [0] = min (m_vertices [0], m_vertices [2]);
if (m_nType == SIDE_IS_QUAD) {
	if (m_nMinVertex [0] > m_vertices [1])
		m_nMinVertex [0] = m_vertices [1];
	if (m_nMinVertex [0] > m_vertices [3])
		m_nMinVertex [0] = m_vertices [3];
	m_nMinVertex [1] = m_nMinVertex [0];
	}
else if (m_nType == SIDE_IS_TRIANGLE) {
	if (m_nMinVertex [0] > m_vertices [1])
		m_nMinVertex [0] = m_vertices [1];
	m_nMinVertex [1] = m_nMinVertex [0];
	}
else
	m_nMinVertex [1] = min (m_vertices [1], m_vertices [4]);

SetupFaceVertIndex ();
}

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the CSide.
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0, 1, 2,
//	face #1 is stored in vertices 3, 4, 5.

void CSide::SetupFaceVertIndex (void)
{
if (m_nType == SIDE_IS_QUAD) {
	m_faceVerts [0] = 0;
	m_faceVerts [1] = 1;
	m_faceVerts [2] = 2;
	m_faceVerts [3] = 3;
	}
else if (m_nType == SIDE_IS_TRI_02) {
	m_faceVerts [0] =
	m_faceVerts [5] = 0;
	m_faceVerts [1] = 1;
	m_faceVerts [2] =
	m_faceVerts [3] = 2;
	m_faceVerts [4] = 3;
	}
else if (m_nType == SIDE_IS_TRI_13) {
	m_faceVerts [0] =
	m_faceVerts [5] = 3;
	m_faceVerts [1] = 0;
	m_faceVerts [2] =
	m_faceVerts [3] = 1;
	m_faceVerts [4] = 2;
	}
else if (m_nType == SIDE_IS_TRIANGLE) {
	m_faceVerts [0] = 0;
	m_faceVerts [1] = 1;
	m_faceVerts [2] = 2;
	m_faceVerts [3] = 0;
	}
}

// -------------------------------------------------------------------------------

void CSide::SetupAsQuadOrTriangle (CFixVector& vNormal, CFloatVector& vNormalf, ushort* verts, ushort* index)
{
m_nType = (m_nCorners == 3) ? SIDE_IS_TRIANGLE : SIDE_IS_QUAD;
m_normals [0] = 
m_normals [1] = vNormal;
m_fNormals [0] = 
m_fNormals [1] = vNormalf;
SetupVertexList (verts, index);
}

// -------------------------------------------------------------------------------

void CSide::SetupAsTriangles (bool bSolid, ushort* verts, ushort* index)
{
	CFixVector		vNormal;
	fix				dot;
	CFixVector		vec_13;		//	vector from vertex 1 to vertex 3

	//	Choose how to triangulate.
	//	If a wall, then
	//		Always triangulate so CSegment is convex.
	//		Use Matt's formula: Na . AD > 0, where ABCD are vertices on side, a is face formed by A, B, C, Na is Normal from face a.
	//	If not a wall, then triangulate so whatever is on the other CSide is triangulated the same (ie, between the same absolute vertices)
if (bSolid) {
	vNormal = CFixVector::Normal (VERTICES [m_corners [0]], VERTICES [m_corners [1]], VERTICES [m_corners [2]]);
	vec_13 = VERTICES [m_corners [3]] - VERTICES [m_corners [1]];
	dot = CFixVector::Dot (vNormal, vec_13);

	//	Now, signify whether to triangulate from 0:2 or 1:3
	m_nType = (dot >= 0) ? SIDE_IS_TRI_02 : SIDE_IS_TRI_13;
	//	Now, based on triangulation nType, set the normals.
	if (m_nType == SIDE_IS_TRI_02) {
#if 0
		VmVecNormalChecked (&vNormal, 
								  VERTICES + m_corners [0], 
								  VERTICES + m_corners [1], 
								  VERTICES + m_corners [2]);
#endif
		m_normals [0] = vNormal;
		m_normals [1] = CFixVector::Normal (VERTICES [m_corners [0]], VERTICES [m_corners [2]], VERTICES [m_corners [3]]);
		m_fNormals [0] = CFloatVector::Normal (FVERTICES [m_corners [0]], FVERTICES [m_corners [1]], FVERTICES [m_corners [2]]);
		m_fNormals [1] = CFloatVector::Normal (FVERTICES [m_corners [0]], FVERTICES [m_corners [2]], FVERTICES [m_corners [3]]);
		}
	else {
		m_normals [0] = CFixVector::Normal (VERTICES [m_corners [0]], VERTICES [m_corners [1]], VERTICES [m_corners [3]]);
		m_normals [1] = CFixVector::Normal (VERTICES [m_corners [1]], VERTICES [m_corners [2]], VERTICES [m_corners [3]]);
		m_fNormals [0] = CFloatVector::Normal (FVERTICES [m_corners [0]], FVERTICES [m_corners [1]], FVERTICES [m_corners [3]]);
		m_fNormals [1] = CFloatVector::Normal (FVERTICES [m_corners [1]], FVERTICES [m_corners [2]], FVERTICES [m_corners [3]]);
		}
	}
else {
	ushort	vSorted [4];
	int		bFlip;

	bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], m_corners [3], vSorted);
	if ((vSorted [0] == m_corners [0]) || (vSorted [0] == m_corners [2])) {
		m_nType = SIDE_IS_TRI_02;
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], 0xFFFF, vSorted);
		m_normals [0] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [0] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip)
			m_normals [0].Neg ();
		bFlip = SortVertsForNormal (m_corners [0], m_corners [2], m_corners [3], 0xFFFF, vSorted);
		m_normals [1] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [1] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip)
			m_normals [1].Neg ();
		SortVertsForNormal (m_corners [0], m_corners [2], m_corners [3], 0xFFFF, vSorted);
		}
	else {
		m_nType = SIDE_IS_TRI_13;
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [3], 0xFFFF, vSorted);
		m_normals [0] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [0] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip)
			m_normals [0].Neg ();
		bFlip = SortVertsForNormal (m_corners [1], m_corners [2], m_corners [3], 0xFFFF, vSorted);
		m_normals [1] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [1] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip)
			m_normals [1].Neg ();
		}
	}
SetupVertexList (verts, index);
}

// -------------------------------------------------------------------------------

int sign (fix v)
{
if (v > PLANE_DIST_TOLERANCE)
	return 1;
if (v < - (PLANE_DIST_TOLERANCE + 1))		//neg & pos round differently
	return -1;
return 0;
}

// -------------------------------------------------------------------------------

void CSide::Setup (short nSegment, ushort* verts, ushort* index, bool bSolid)
{
	ushort			vSorted [4], bFlip;
	CFixVector		vNormal;
	CFloatVector	vNormalf;
	fix				xDistToPlane;

m_nSegment = nSegment;
SetupCorners (verts, index);
bFlip = (m_nCorners == 3)
		  ? SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], 0xFFFF, vSorted)
		  : bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], m_corners [3], vSorted);
vNormal = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
vNormalf = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
xDistToPlane = abs (VERTICES [vSorted [3]].DistToPlane (vNormal, VERTICES [vSorted [0]]));
if (bFlip)
	vNormal.Neg ();

m_bIsQuad = (m_normals [0] == m_normals [1]);
if (m_nCorners == 3)
	SetupAsQuadOrTriangle (vNormal, vNormalf, verts, index);
if (PLANE_DIST_TOLERANCE < DEFAULT_PLANE_DIST_TOLERANCE) {
	SetupAsTriangles (bSolid, verts, index);
	}
else {
	if (xDistToPlane <= PLANE_DIST_TOLERANCE)
		SetupAsQuadOrTriangle (vNormal, vNormalf, verts, index);
	else {
		SetupAsTriangles (bSolid, verts, index);
		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.
		Assert (m_nFaces == 2);
		fix dist0 = VERTICES [m_vertices [1]].DistToPlane (m_normals [1], VERTICES [m_nMinVertex [0]]);
		fix dist1 = VERTICES [m_vertices [4]].DistToPlane (m_normals [0], VERTICES [m_nMinVertex [0]]);
		int s0 = sign (dist0);
		int s1 = sign (dist1);
		if (s0 == 0 || s1 == 0 || s0 != s1)
			SetupAsQuadOrTriangle (vNormal, vNormalf, verts, index);
		}
	}

m_normals [2] = CFixVector::Avg (m_normals [0], m_normals [1]);
m_fNormals [2] = CFloatVector::Avg (m_fNormals [0], m_fNormals [1]);
if (m_nType == SIDE_IS_QUAD) {
	for (int i = 0; i < 4; i++)
		AddToVertexNormal (m_vertices [i], vNormal);
	}
else if (m_nType == SIDE_IS_TRIANGLE) {
	for (int i = 0; i < 3; i++)
		AddToVertexNormal (m_vertices [i], vNormal);
	}
else {
	for (int i = 0; i < 3; i++)
		AddToVertexNormal (m_vertices [i], m_normals [2]);
	}
}

// -------------------------------------------------------------------------------

CFixVector& CSide::Vertex (int nVertex)
{
return gameStates.render.bRendering ? RENDERPOINTS [nVertex].ViewPos () : VERTICES [nVertex];
}

// -------------------------------------------------------------------------------

CFixVector& CSide::MinVertex (void)
{
return Vertex (m_nMinVertex [0]);
}

// -------------------------------------------------------------------------------
// height of a two faced, non-planar side

fix CSide::Height (void)
{
return Vertex (m_nMinVertex [1]).DistToPlane (Normal (m_vertices [4] >= m_vertices [1]), Vertex (m_nMinVertex [0]));
}

// -------------------------------------------------------------------------------

bool CSide::IsPlanar (void)
{
return (m_nFaces < 2) || (Height () <= PLANE_DIST_TOLERANCE);
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSide::Masks (const CFixVector& refPoint, fix xRad, short sideBit, short& faceBit, bool bCheckPoke)
{
	CSegMasks	masks;
	fix			xDist;

masks.m_valid = 1;
if (m_nFaces == 2) {
	int	nSideCount = 0, 
			nCenterCount = 0;

	CFixVector vMin = MinVertex ();
	for (int nFace = 0; nFace < 2; nFace++, faceBit <<= 1) {
		xDist = refPoint.DistToPlane (Normal (nFace), vMin);
		if (xDist < -PLANE_DIST_TOLERANCE) //in front of face
			nCenterCount++;
		if ((xDist - xRad < -PLANE_DIST_TOLERANCE) && (!bCheckPoke || (xDist + xRad >= -PLANE_DIST_TOLERANCE))) {
			masks.m_face |= faceBit;
			nSideCount++;
			}
		}
	if (IsPlanar ()) {	//must be behind both faces
		if (nSideCount == 2)
			masks.m_side |= sideBit;
		if (nCenterCount == 2)
			masks.m_center |= sideBit;
		}
	else {	//must be behind at least one face
		if (nSideCount)
			masks.m_side |= sideBit;
		if (nCenterCount)
			masks.m_center |= sideBit;
		}
	}
else {	
	xDist = refPoint.DistToPlane (Normal (0), MinVertex ());
	if (xDist < -PLANE_DIST_TOLERANCE)
		masks.m_center |= sideBit;
	if ((xDist - xRad < -PLANE_DIST_TOLERANCE) && (!bCheckPoke || (xDist + xRad >= -PLANE_DIST_TOLERANCE))) {
		masks.m_face |= faceBit;
		masks.m_side |= sideBit;
		}
	faceBit <<= 2;
	}
masks.m_valid = 1;
return masks;
}

// -------------------------------------------------------------------------------

ubyte CSide::Dist (const CFixVector& refPoint, fix& xSideDist, int bBehind, short sideBit)
{
	fix	xDist;
	ubyte mask = 0;

xSideDist = 0;
if (m_nFaces == 2) {
	int nCenterCount = 0;

	CFixVector vMin = MinVertex ();
	for (int nFace = 0; nFace < 2; nFace++) {
		xDist = refPoint.DistToPlane (Normal (nFace), vMin);
		if ((xDist < -PLANE_DIST_TOLERANCE) == bBehind) {	//in front of face
			nCenterCount++;
			xSideDist += xDist;
			}
		}
	if (IsPlanar ()) {	//must be behind both faces
		if (nCenterCount == 2) {
			mask |= sideBit;
			xSideDist /= 2;	//get average
			}
		}
	else {	//must be behind at least one face
		if (nCenterCount) {
			mask |= sideBit;
			if (nCenterCount == 2)
				xSideDist /= 2;	//get average
			}
		}
	}
else {				//only one face on this CSide
	xDist = refPoint.DistToPlane (Normal (0), MinVertex ());
	if ((xDist < -PLANE_DIST_TOLERANCE) == bBehind) {
		mask |= sideBit;
		xSideDist = xDist;
		}
	}
return mask;
}

// -------------------------------------------------------------------------------

ubyte CSide::Distf (const CFloatVector& refPoint, float& fSideDist, int bBehind, short sideBit)
{
	float	fDist;
	ubyte mask = 0;
	CFloatVector vMin, vNormal;

vMin.Assign (MinVertex ());
fSideDist = 0;
if (m_nFaces == 2) {
	int nCenterCount = 0;

	for (int nFace = 0; nFace < 2; nFace++) {
		vNormal.Assign (Normal (nFace));
		fDist = refPoint.DistToPlane (vNormal, vMin);
		if ((fDist < -X2F (PLANE_DIST_TOLERANCE)) == bBehind) {	//in front of face
			nCenterCount++;
			fSideDist += fDist;
			}
		}
	if (IsPlanar ()) {	//must be behind both faces
		if (nCenterCount == 2) {
			mask |= sideBit;
			fSideDist /= 2;	//get average
			}
		}
	else {	//must be behind at least one face
		if (nCenterCount) {
			mask |= sideBit;
			if (nCenterCount == 2)
				fSideDist /= 2;	//get average
			}
		}
	}
else {				//only one face on this CSide
	vNormal.Assign (Normal (0));
#if DBG
	fDist = CFloatVector::Dist (refPoint, vMin);
#endif
	fDist = refPoint.DistToPlane (vNormal, vMin);
	if ((fDist < -X2F (PLANE_DIST_TOLERANCE)) == bBehind) {
		mask |= sideBit;
		fSideDist = fDist;
		}
	}
return mask;
}

//	-----------------------------------------------------------------------------

fix CSide::DistToPoint (CFixVector v)
{
	CFixVector		h, n;
	fix				dist, minDist = 0x7fffffff;
	int				i, j;

// compute intersection of perpendicular through refP with the plane spanned up by the face
for (i = j = 0; i < m_nFaces; i++, j += 3) {
	n = m_normals [i];
	h = v - VERTICES [m_vertices [j]];
	dist = CFixVector::Dot (h, n);
	if (minDist > abs (dist)) {
		h = v - n * dist;
		if (!PointToFaceRelation (h, i, n)) {
			minDist = abs (dist);
			}
		}
	}
if (minDist < 0x7fffffff)
	return 0;
for (i = 0; i < m_nCorners; i++) {
	FindPointLineIntersection (h, VERTICES [m_corners [i]], VERTICES [m_corners [(i + 1) % m_nCorners]], v, 1);
	dist = CFixVector::Dist (h, v);
	if (minDist > dist)
		minDist = dist;
	}
return minDist;
}

//	-----------------------------------------------------------------------------

float CSide::DistToPointf (CFloatVector v)
{
	CFloatVector	h, n;
	float				dist, minDist = 1e30f;
	int				i, j;

// compute intersection of perpendicular through refP with the plane spanned up by the face
for (i = j = 0; i < m_nFaces; i++, j += 3) {
	n.Assign (m_normals [i]);
	h = v - FVERTICES [m_vertices [j]];
	dist = CFloatVector::Dot (h, n);
	if (minDist > fabs (dist)) {
		h = v - n * dist;
		if (PointIsOutsideFace (&h, m_vertices + j, (m_nFaces == 1) ? 4 : 3)) {
			minDist = float (fabs (dist));
			}
		}
	}
if (minDist < 1e30f)
	return 0;
for (i = 0; i < m_nCorners; i++) {
	FindPointLineIntersection (h, FVERTICES [m_corners [i]], FVERTICES [m_corners [(i + 1) % m_nCorners]], v, 1);
	dist = CFloatVector::Dist (h, v);
	if (minDist > dist)
		minDist = dist;
	}
return minDist;
}

//	-----------------------------------------------------------------------------

CWall* CSide::Wall (void) 
{ 
return IS_WALL (m_nWall) ? WALLS + m_nWall : NULL; 
}

//	-----------------------------------------------------------------------------
// Check whether point vPoint in segment nDestSeg can be seen from this side.
// Level 0: Check from side center, 1: check from center and corners, 2: check from center, corners, and edge centers

int CSide::SeesPoint (CFixVector& vPoint, short nDestSeg, int nLevel, int nThread)
{
	static int nLevels [3] = {4, 0, -4};

	CFloatVector	v0, v1;
	int				i, j;

v1.Assign (vPoint);

if (nLevel >= 0) 
	i = 4;
else {
	i = 3;
	nLevel = -nLevel - 1;
	}
j = j = nLevels [nLevel];

for (; i >= j; i--) {
	if (i == m_nCorners)
		v0.Assign (Center ());
	else if (i >= 0)
		v0 = FVERTICES [m_corners [i]];
	else
		v0 = CFloatVector::Avg (FVERTICES [m_corners [m_nCorners + i]], FVERTICES [m_corners [(m_nCorners + 1 + i) % m_nCorners]]); // center of face's edges
	if (PointSeesPoint (&v0, &v1, m_nSegment, nDestSeg, 0, nThread))
		return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//see if a refP is inside a face by projecting into 2d
uint CSide::PointToFaceRelation (CFixVector& intersection, short nFace, CFixVector vNormal)
{
#if 0

	CFixVector		t;
	int				biggest;
	int 				h, i, j, nEdge, nVerts;
	uint 				nEdgeMask;
	fix 				check_i, check_j;
	CFixVector		*v0, *v1;
	CFixVector2		vEdge, vCheck;
	QLONG				d;

//now do 2d check to see if refP is in CSide
//project polygon onto plane by finding largest component of Normal
t.v.coord.x = labs (vNormal.v.coord.x);
t.v.coord.y = labs (vNormal.v.coord.y);
t.v.coord.z = labs (vNormal.v.coord.z);
if (t.v.coord.x > t.v.coord.y)
	if (t.v.coord.x > t.v.coord.z)
		biggest = 0;
	else
		biggest = 2;
else if (t.v.coord.y > t.v.coord.z)
	biggest = 1;
else
	biggest = 2;
if (vNormal.v.vec [biggest] > 0) {
	i = ijTable [biggest][0];
	j = ijTable [biggest][1];
	}
else {
	i = ijTable [biggest][1];
	j = ijTable [biggest][0];
	}
//now do the 2d problem in the i, j plane
check_i = intersection.v.vec [i];
check_j = intersection.v.vec [j];
nVerts = 5 - m_nFaces;
h = nFace * 3;
for (nEdge = nEdgeMask = 0; nEdge < nVerts; nEdge++) {
	if (gameStates.render.bRendering) {
		v0 = &RENDERPOINTS [m_vertices [h + nEdge]].ViewPos ();
		v1 = &RENDERPOINTS [m_vertices [h + ((nEdge + 1) % nVerts)]].ViewPos ();
		}
	else {
		v0 = VERTICES + m_vertices [h + nEdge];
		v1 = VERTICES + m_vertices [h + ((nEdge + 1) % nVerts)];
		}
	vEdge.i = v1->v.vec [i] - v0->v.vec [i];
	vEdge.j = v1->v.vec [j] - v0->v.vec [j];
	vCheck.i = check_i - v0->v.vec [i];
	vCheck.j = check_j - v0->v.vec [j];
	d = FixMul64 (vCheck.i, vEdge.j) - FixMul64 (vCheck.j, vEdge.i);
	if (d < 0)              		//we are outside of triangle
		nEdgeMask |= (1 << nEdge);
	}
return nEdgeMask;

#else

	CFixVector		t;
	int 				h, i, j, nEdge, nVerts, projPlane;
	uint 				nEdgeMask;
	CFixVector*		v0, * v1;
	CFixVector2		vEdge, vCheck, vRef;

//now do 2d check to see if refP is in CSide
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
vRef.x = intersection.v.vec [i];
vRef.y = intersection.v.vec [j];
nVerts = (m_nCorners == 3) ? 3 : 5 - m_nFaces;
h = nFace * 3;
v1 = gameStates.render.bRendering ? &RENDERPOINTS [m_vertices [h]].ViewPos () : VERTICES + m_vertices [h];
for (nEdge = 1, nEdgeMask = 0; nEdge <= nVerts; nEdge++) {
	v0 = v1;
	v1 = gameStates.render.bRendering 
		  ? &RENDERPOINTS [m_vertices [h + nEdge % nVerts]].ViewPos ()
		  : VERTICES + m_vertices [h + nEdge % nVerts];
	vEdge.x = v1->v.vec [i] - v0->v.vec [i];
	vEdge.y = v1->v.vec [j] - v0->v.vec [j];
	vCheck.x = vRef.x - v0->v.vec [i];
	vCheck.y = vRef.y - v0->v.vec [j];
	if (FixMul64 (vCheck.x, vEdge.y) - FixMul64 (vCheck.y, vEdge.x) < PLANE_DIST_TOLERANCE) //we are outside of triangle
		nEdgeMask |= (1 << (nEdge - 1));
	}
return nEdgeMask;

#endif
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int CSide::SphereToFaceRelation (CFixVector& intersection, fix rad, short iFace, CFixVector vNormal)
{
	CFixVector	vEdge, vCheck;            //this time, real 3d vectors
	CFixVector	vNearest;
	fix			xEdgeLen, d, dist;
	CFixVector	*v0, *v1;
	int			iType;
	int			nEdge, nVerts;
	uint			nEdgeMask;

//now do 2d check to see if refP is in side
nEdgeMask = PointToFaceRelation (intersection, iFace, vNormal);
//we've gone through all the sides, are we inside?
if (nEdgeMask == 0)
	return IT_FACE;
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
nVerts = (m_nCorners == 3) ? 3 : 5 - m_nFaces;
iFace *= 3;
if (gameStates.render.bRendering) {
	v0 = &RENDERPOINTS [m_vertices [iFace + nEdge]].ViewPos ();
	v1 = &RENDERPOINTS [m_vertices [iFace + ((nEdge + 1) % nVerts)]].ViewPos ();
	}
else {
	v0 = VERTICES + m_vertices [iFace + nEdge];
	v1 = VERTICES + m_vertices [iFace + ((nEdge + 1) % nVerts)];
	}
//check if we are touching an edge or refP
vCheck = intersection - *v0;
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
	vNearest = *v0;
else if (d > xEdgeLen)
	vNearest = *v1;
else {
	iType = IT_EDGE;
#if 0
	vNearest = *v0 + (vEdge * d);
#else
	vNearest = vEdge;
	vNearest *= d;
	vNearest += *v0;
#endif
	}
dist = CFixVector::Dist (intersection, vNearest);
if (dist <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in intersection with intersection
//refP on plane, whether or not line intersects CSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a refP field
int CSide::CheckLineToFaceRegular (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal)
{
	CFixVector	v1;
	int			pli, nVertex, bCheckRad = 0;

//use lowest refP number
#if DBG
if (m_nFaces <= iFace) {
	Error ("invalid face number in CSegment::CheckLineToFace()");
	return IT_ERROR;
	}
#endif
#if 1
if (*p1 == *p0) {
#if 0
	return SphereToFaceRelation (p0, rad, iFace, vNormal);
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
#if 1
nVertex = m_nMinVertex [0];
#else
nVertex = m_vertices [0];
if (nVertex > m_vertices [2])
	nVertex = m_vertices [2];
if (m_nFaces == 1) {
	if (nVertex > m_vertices [1])
		nVertex = m_vertices [1];
	if (m_nCorners == 4) {
		if (nVertex > m_vertices [3])
			nVertex = m_vertices [3];
		}
	}
#endif
if (!(pli = FindPlaneLineIntersection (intersection, gameStates.render.bRendering ? &RENDERPOINTS [nVertex].ViewPos () : VERTICES + nVertex, &vNormal, p0, p1, rad)))
	return IT_NONE;
CFixVector vHit = intersection;
//if rad != 0, project the refP down onto the plane of the polygon
if (rad)
	vHit += vNormal * (-rad);
if ((pli = SphereToFaceRelation (vHit, rad, iFace, vNormal)))
	return pli;
if (bCheckRad) {
	int			i, d;
	CFixVector	*a, *b;

	b = VERTICES + m_corners [0];
	for (i = 1; i <= m_nCorners; i++) {
		a = b;
		b = VERTICES + m_corners [i % m_nCorners];
		d = VmLinePointDist (*a, *b, *p0);
		if (d < bCheckRad)
			return IT_POINT;
		}
	}
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//this version is for when the start and end positions both poke through
//the plane of a CSide.  In this case, we must do checks against the edge
//of faces
int CSide::CheckLineToFaceSpecial (CFixVector& intersection, CFixVector *p0, CFixVector *p1, fix rad, short iFace, CFixVector vNormal)
{
	CFixVector	vMove;
	fix			edge_t, move_t, edge_t2, move_t2, closestDist;
	fix			edge_len, move_len;
	int			nEdge, nVerts;
	uint			nEdgeMask;
	CFixVector	*edge_v0, *edge_v1, vEdge;
	CFixVector	vClosestEdgePoint, vClosestMovePoint;

vMove = *p1 - *p0;
//figure out which edge(side) to check against
if (!(nEdgeMask = PointToFaceRelation (*p0, iFace, vNormal)))
	return CheckLineToFaceRegular (intersection, p0, p1, rad, iFace, vNormal);
for (nEdge = 0; !(nEdgeMask & 1); nEdgeMask >>= 1, nEdge++)
	;
nVerts = (m_nCorners == 3) ? 3 : 5 - m_nFaces;
edge_v0 = VERTICES + m_vertices [iFace * 3 + nEdge];
edge_v1 = VERTICES + m_vertices [iFace * 3 + ((nEdge + 1) % nVerts)];
vEdge = *edge_v1 - *edge_v0;
//is the start refP already touching the edge?
//first, find refP of closest approach of vec & edge
edge_len = CFixVector::Normalize (vEdge);
move_len = CFixVector::Normalize (vMove);
CheckLineToLine (&edge_t, &move_t, edge_v0, &vEdge, p0, &vMove);
//make sure t values are in valid range
if ((move_t < 0) || (move_t > move_len + rad))
	return IT_NONE;
move_t2 = (move_t > move_len) ? move_len : move_t;
edge_t2 = (edge_t < 0) ? 0 : (edge_t > edge_len) ? edge_len : edge_t;
//now, edge_t & move_t determine closest points.  calculate the points.
vClosestEdgePoint = *edge_v0 + vEdge * edge_t2;
vClosestMovePoint = *p0 + vMove * move_t2;
//find dist between closest points
closestDist = CFixVector::Dist (vClosestEdgePoint, vClosestMovePoint);
//could we hit with this dist?
//note massive tolerance here
if (closestDist < (rad * 9) / 10) {		//we hit.  figure out where
	//now figure out where we hit
	intersection = *p0 + vMove * (move_t-rad);
	return IT_EDGE;
	}
return IT_NONE;			//no hit
}

//	-----------------------------------------------------------------------------
//finds the uv coords of the given refP on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
void CSide::HitPointUV (fix *u, fix *v, fix *l, CFixVector& intersection, int iFace)
{
	CFixVector*		vPoints;
	CFixVector		vNormal;
	int				projPlane, ii, jj;
	tUVL				uvls [3];
	int				h;

if (iFace >= m_nFaces) {
	PrintLog (0, "invalid face number in CSide::HitPointUV\n");
	*u = *v = 0;
	return;
	}
//now the hard work.
//1. find what plane to project this CWall onto to make it a 2d case
vNormal = m_normals [iFace];
projPlane = 0;
if (abs (vNormal.v.coord.y) > abs (vNormal.v.vec [projPlane]))
	projPlane = 1;
if (abs (vNormal.v.coord.z) > abs (vNormal.v.vec [projPlane]))
	projPlane = 2;
ii = (projPlane == 0);
jj = (projPlane == 2) ? 1 : 2;
//2. compute u, v of intersection refP
//vec from 1 -> 0
h = iFace * 3;
vPoints = VERTICES + m_vertices [h+1];
CFloatVector2 vRef (vPoints->v.vec [ii], vPoints->v.vec [jj]);
vPoints = VERTICES + m_vertices [h];
CFloatVector2 vec0 (vPoints->v.vec [ii], vPoints->v.vec [jj]);
vPoints = VERTICES + m_vertices [h+2];
CFloatVector2 vec1 (vPoints->v.vec [ii], vPoints->v.vec [jj]);
vec0 -= vRef;
vec1 -= vRef;
//vec from 1 -> checkPoint
CFloatVector2 vHit (intersection.v.vec [ii], intersection.v.vec [jj]);
float k1 = (vHit.Cross (vec0) + vec0.Cross (vRef)) / vec0.Cross (vec1);
float k0 = (fabs (vec0.x) > fabs (vec0.y))
			  ? (k1 * vec1.x + vHit.x - vRef.x) / vec0.x
			  : (k1 * vec1.y + vHit.y - vRef.y) / vec0.y;
uvls [0] = m_uvls [m_faceVerts [h]];
uvls [1] = m_uvls [m_faceVerts [h+1]];
uvls [2] = m_uvls [m_faceVerts [h+2]];
*u = uvls [1].u + fix (k0 * (uvls [0].u - uvls [1].u) - k1 * (uvls [2].u - uvls [1].u));
*v = uvls [1].v + fix (k0 * (uvls [0].v - uvls [1].v) - k1 * (uvls [2].v - uvls [1].v));
if (0 > *u)
	*u = I2X (1) - (-*u) % I2X (1);
else
	*u %= I2X (1);
if (0 > *v)
	*v = I2X (1) - (-*v) % I2X (1);
else
	*v %= I2X (1);
if (l)
	*l = uvls [1].l + fix (k0 * (uvls [0].l - uvls [1].l) + k1 * (uvls [2].l - uvls [1].l));
}

//------------------------------------------------------------------------------

bool CSide::IsOpenableDoor (void)
{
return IS_WALL (m_nWall) ? WALLS [m_nWall].IsOpenableDoor () : false;
}

//------------------------------------------------------------------------------

CFixVector* CSide::GetCorners (CFixVector* vertices, ubyte* nCorners) 
{ 
for (int i = 0; i < m_nCorners; i++)
	vertices [i] = VERTICES [m_corners [i]];
if (nCorners)
	*nCorners = m_nCorners;
return vertices;
}

//------------------------------------------------------------------------------

bool CSide::IsWall (void)
{
return IS_WALL (m_nWall); 
}

//------------------------------------------------------------------------------

CTrigger* CSide::Trigger (void)
{
return IS_WALL (m_nWall) ? Wall ()->Trigger () : NULL;
}

//------------------------------------------------------------------------------

bool CSide::IsVolatile (void)
{
return IsWall () && WALLS [m_nWall].IsVolatile ();
}

//------------------------------------------------------------------------------

int CSide::Physics (fix& damage, bool bSolid)
{
if (!bSolid) {
	CWall* wallP = Wall ();
	if (!wallP || (wallP->nType != WALL_ILLUSION) || (wallP->flags & WALL_ILLUSION_OFF))
		return 0;
	}
if (gameData.pig.tex.tMapInfoP [m_nBaseTex].damage) {
	damage = gameData.pig.tex.tMapInfoP [m_nBaseTex].damage;
	return 3;
	}
if (gameData.pig.tex.tMapInfoP [m_nBaseTex].flags & TMI_WATER)
	return bSolid ? 2 : 4;
return 0;
}

//-----------------------------------------------------------------

int CSide::CheckTransparency (void)
{
	CBitmap	*bmP;

if (m_nOvlTex) {
	bmP = gameData.pig.tex.bitmapP [gameData.pig.tex.bmIndexP [m_nOvlTex].index].Override (-1);
	if (bmP->Flags () & BM_FLAG_SUPER_TRANSPARENT)
		return 1;
	if (!(bmP->Flags () & BM_FLAG_TRANSPARENT))
		return 0;
	}
bmP = gameData.pig.tex.bitmapP [gameData.pig.tex.bmIndexP [m_nBaseTex].index].Override (-1);
if (bmP->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 1;
if ((gameStates.app.bD2XLevel) && IS_WALL (m_nWall)) {
	short c = WALLS [m_nWall].cloakValue;
	if (c && (c < FADE_LEVELS))
		return 1;
	}
return gameOpts->render.effects.bAutoTransparency && IsTransparentTexture (m_nBaseTex);
}

//------------------------------------------------------------------------------

void CSide::SetTextures (int nBaseTex, int nOvlTex)
{
if (nBaseTex >= 0)
	m_nBaseTex = nBaseTex;
if (nOvlTex >= 0)
	m_nOvlTex = nOvlTex;
}

//------------------------------------------------------------------------------

bool CSide::IsTextured (void)
{
return IS_WALL (m_nWall);
}

//------------------------------------------------------------------------------

short ConvertD1Texture (short nD1Texture, int bForce);

int CSide::Read (CFile& cf, ushort* sideVerts, bool bSolid)
{
	int nType = bSolid ? 1 : IS_WALL (m_nWall) ? 2 : 0;

m_nFrame = 0;
if (!nType)
	m_nBaseTex =
	m_nOvlTex = 0;
else {
	// Read short sideP->m_nBaseTex;
	ushort nTexture;
	if (bNewFileFormat) {
		nTexture = cf.ReadUShort ();
		m_nBaseTex = nTexture & 0x7fff;
		}
	else {
		nTexture = 0;
		m_nBaseTex = cf.ReadShort ();
		}
	if (gameData.segs.nLevelVersion <= 1)
		m_nBaseTex = ConvertD1Texture (m_nBaseTex, 0);
	if (bNewFileFormat && !(nTexture & 0x8000))
		m_nOvlTex = 0;
	else {
		// Read short m_nOvlTex;
		nTexture = cf.ReadShort ();
		m_nOvlTex = nTexture & 0x3fff;
		m_nOvlOrient = (nTexture >> 14) & 3;
		if ((gameData.segs.nLevelVersion <= 1) && m_nOvlTex)
			m_nOvlTex = ConvertD1Texture (m_nOvlTex, 0);
		}
	// guess what? One level from D2:CS contains a texture id "910". D'oh.
	m_nBaseTex %= MAX_WALL_TEXTURES;	
	m_nOvlTex %= MAX_WALL_TEXTURES;

	// Read tUVL m_uvls [4] (u, v>>5, write as short, l>>1 write as short)
	for (int i = 0; i < 4; i++) {
		m_uvls [i].u = fix (cf.ReadShort ()) << 5;
		m_uvls [i].v = fix (cf.ReadShort ()) << 5;
		m_uvls [i].l = fix (cf.ReadUShort ()) << 1;
		float fBrightness = X2F (m_uvls [i].l);
		if (gameData.render.color.vertBright [sideVerts [i]] < fBrightness)
			gameData.render.color.vertBright [sideVerts [i]] = fBrightness;
		}
	}
m_nCorners = 4;
return nType;
}

//------------------------------------------------------------------------------

void CSide::ReadWallNum (CFile& cf, bool bWall)
{
m_nWall = ushort (bWall ? (gameData.segs.nLevelVersion >= 13) ? cf.ReadUShort () : cf.ReadUByte () : -1);
}

//------------------------------------------------------------------------------
// reads a CSegment structure from a CFile
 
void CSide::SaveState (CFile& cf)
{
cf.WriteShort (m_nWall);
cf.WriteShort (m_nBaseTex);
cf.WriteShort (m_nOvlTex | (m_nOvlOrient << 14));
}

//------------------------------------------------------------------------------
// reads a CSegment structure from a CFile
 
void CSide::LoadState (CFile& cf)
{
m_nWall = cf.ReadShort ();
m_nBaseTex = cf.ReadShort ();
	short nTexture = cf.ReadShort ();
m_nOvlTex = nTexture & 0x3fff;
m_nOvlOrient = (nTexture >> 14) & 3;
}

//------------------------------------------------------------------------------

//eof
