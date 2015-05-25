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

int16_t ConvertD1Texture (int16_t nD1Texture, int32_t bForce);

// ------------------------------------------------------------------------------------------

CFixVector& CSide::Normal (int32_t nFace)
{ 
return gameStates.render.bRendering ? m_rotNorms [nFace] : m_normals [nFace]; 
}

// ------------------------------------------------------------------------------------------

CFloatVector& CSide::Normalf (int32_t nFace)
{ 
return m_fNormals [nFace]; 
}

// ------------------------------------------------------------------------------------------
// Compute the center point of a CSide of a CSegment.
//	The center point is defined to be the average of the 4 points defining the CSide.
void CSide::ComputeCenter (void)
{
if (!FaceCount ())
	return;
CFloatVector vCenter;
vCenter.SetZero ();
for (int32_t i = 0; i < m_nCorners; i++)
	vCenter += FVERTICES [m_corners [i]];
vCenter /= float (m_nCorners);
m_vCenter.Assign (vCenter);
// make sure side center is inside segment
CFixVector v0 = m_vCenter + m_normals [2];
CFixVector v1 = m_vCenter - m_normals [2];
#if 1
FindPlaneLineIntersection (m_vCenter, &VERTICES [m_corners [0]], &m_normals [0], &v0, &v1, 0, false);
#else
CFixVector c0, c1;
FindPlaneLineIntersection (c0, &VERTICES [m_corners [0]], &m_normals [0], &v0, &v1, 0, false);
FindPlaneLineIntersection (c1, &VERTICES [m_corners [3]], &m_normals [1], &v0, &v1, 0, false);
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
for (int32_t i = 0; i < m_nCorners; i++) {
	CFixVector v = CFixVector::Avg (VERTICES [m_corners [i]], VERTICES [m_corners [(i + 1) % m_nCorners]]);
	fix d = CFixVector::Dist (v, m_vCenter);
	if (m_rads [0] > d)
		m_rads [0] = d;
	d = CFixVector::Dist (m_vCenter, VERTICES [m_corners [i]]);
	if (m_rads [1] < d)
		m_rads [1] = d;
	}
}

// -------------------------------------------------------------------------------

void CSide::SetupCorners (uint16_t* verts, uint16_t* index)
{
for (int32_t i = 0; i < 4; i++)
	m_corners [i] = (index [i] == 0xff) ? 0xFFFF : verts [index [i]];
}

// -------------------------------------------------------------------------------

void CSide::SetupVertexList (uint16_t* verts, uint16_t* index)
{
m_nFaces = -1;
if (m_nShape) {
	m_nFaces = (m_nShape == SIDE_SHAPE_TRIANGLE) ? 1 : 0;
	if (m_nFaces) {
		m_vertices [0] = m_corners [0];
		m_vertices [1] = m_corners [1];
		m_vertices [2] = m_corners [2];
		}
	}
else if (m_nType == SIDE_IS_QUAD) {
	m_vertices [0] = m_corners [0];
	m_vertices [1] = m_corners [1];
	m_vertices [2] = m_corners [2];
	m_vertices [3] = m_corners [3];
	m_nFaces = 1;
	}
else if (m_nType == SIDE_IS_TRI_02) {
	m_vertices [0] =
	m_vertices [5] = m_corners [0];
	m_vertices [1] = m_corners [1];
	m_vertices [2] =
	m_vertices [3] = m_corners [2];
	m_vertices [4] = m_corners [3];
	m_nMinVertex [1] = Min (m_vertices [1], m_vertices [4]);
	m_nFaces = 2;
	}
else if (m_nType == SIDE_IS_TRI_13) {
	m_vertices [0] =
	m_vertices [5] = m_corners [3];
	m_vertices [1] = m_corners [0];
	m_vertices [2] =
	m_vertices [3] = m_corners [1];
	m_vertices [4] = m_corners [2];
	m_nFaces = 2;
	}
else {
	return;
	}

m_nMinVertex [0] = Min (m_vertices [0], m_vertices [2]);
if (m_nFaces == 1) {
	if (m_nMinVertex [0] > m_vertices [1])
		m_nMinVertex [0] = m_vertices [1];
	}
if (m_nType == SIDE_IS_QUAD) {
	if (m_nMinVertex [0] > m_vertices [1])
		m_nMinVertex [0] = m_vertices [1];
	if (m_nMinVertex [0] > m_vertices [3])
		m_nMinVertex [0] = m_vertices [3];
	m_nMinVertex [1] = m_nMinVertex [0];
	}
else
	m_nMinVertex [1] = Min (m_vertices [1], m_vertices [4]);

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
	if (m_nFaces == 1) {
		m_faceVerts [0] = 0;
		m_faceVerts [5] = 1;
		m_faceVerts [1] = 2;
		}
	else {
		m_faceVerts [0] =
		m_faceVerts [5] = 0;
		m_faceVerts [1] = 1;
		m_faceVerts [2] =
		m_faceVerts [3] = 2;
		m_faceVerts [4] = 3;
		}
	}
else if (m_nType == SIDE_IS_TRI_13) {
	m_faceVerts [0] =
	m_faceVerts [5] = 3;
	m_faceVerts [1] = 0;
	m_faceVerts [2] =
	m_faceVerts [3] = 1;
	m_faceVerts [4] = 2;
	}
}

// -------------------------------------------------------------------------------

void CSide::SetupAsQuad (CFixVector& vNormal, CFloatVector& vNormalf, uint16_t* verts, uint16_t* index)
{
m_nType = SIDE_IS_QUAD;
m_normals [0] = 
m_normals [1] = vNormal;
m_fNormals [0] = 
m_fNormals [1] = vNormalf;
SetupVertexList (verts, index);
}

// -------------------------------------------------------------------------------

void CSide::FixNormals (void)
{
#if 1
#if DBG
if (((nDbgSeg > 0) && (nDbgSeg == m_nSegment)) && ((nDbgSide < 0) || (this - SEGMENT (m_nSegment)->m_sides == nDbgSide)))
	BRP;
bool bFlip = false;
#endif

{
	CFloatVector vRef;
	vRef.Assign (SEGMENT (m_nSegment)->Center () - m_vCenter);
	CFloatVector::Normalize (vRef);

	for (int32_t i = 0; i < 2; i++) {
		if (CFloatVector::Dot (m_fNormals [i], vRef) < 0) {
#if DBG
			if (!bFlip) {
				BRP;
				bFlip = true;
				}
#endif
			m_fNormals [i].Neg ();
			}
		}
	m_fNormals [2] = CFloatVector::Avg (m_fNormals [0], m_fNormals [1]);
	}

{
	CFixVector vRef;
	vRef = SEGMENT (m_nSegment)->Center () - m_vCenter;
	CFixVector::Normalize (vRef);

	for (int32_t i = 0; i < 2; i++) {
		if (CFixVector::Dot (m_normals [i], vRef) < 0) {
#if DBG
			if (!bFlip) {
				BRP;
				bFlip = true;
				}
#endif
			m_fNormals [i].Neg ();
			}
		}
	m_normals [2] = CFixVector::Avg (m_normals [0], m_normals [1]);
	}

#endif
}

// -------------------------------------------------------------------------------

void CSide::SetupAsTriangles (bool bSolid, uint16_t* verts, uint16_t* index)
{
	CFixVector		vNormal;
	fix				dot;
	CFixVector		vec_13;		//	vector from vertex 1 to vertex 3

	//	Choose how to triangulate.
	//	If a wall, then
	//		Always triangulate so segment is convex.
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
	uint16_t	vSorted [4];
	int32_t	bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], m_corners [3], vSorted);
	if ((vSorted [0] == m_corners [0]) || (vSorted [0] == m_corners [2])) {
		m_nType = SIDE_IS_TRI_02;
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], 0xFFFF, vSorted);
		m_normals [0] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [0] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip) {
			m_normals [0].Neg ();
			m_fNormals [0].Neg ();
			}
		bFlip = SortVertsForNormal (m_corners [0], m_corners [2], m_corners [3], 0xFFFF, vSorted);
		m_normals [1] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [1] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip) {
			m_normals [1].Neg ();
			m_fNormals [1].Neg ();
			}
		SortVertsForNormal (m_corners [0], m_corners [2], m_corners [3], 0xFFFF, vSorted);
		}
	else {
		m_nType = SIDE_IS_TRI_13;
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [3], 0xFFFF, vSorted);
		m_normals [0] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [0] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip) {
			m_normals [0].Neg ();
			m_fNormals [0].Neg ();
			}
		bFlip = SortVertsForNormal (m_corners [1], m_corners [2], m_corners [3], 0xFFFF, vSorted);
		m_normals [1] = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
		m_fNormals [1] = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
		if (bFlip) {
			m_normals [1].Neg ();
			m_fNormals [1].Neg ();
			}
		}
	}
SetupVertexList (verts, index);
}

// -------------------------------------------------------------------------------

int32_t sign (fix v)
{
if (v > PLANE_DIST_TOLERANCE)
	return 1;
if (v < - (PLANE_DIST_TOLERANCE + 1))		//neg & pos round differently
	return -1;
return 0;
}

// -------------------------------------------------------------------------------

void CSide::Setup (int16_t nSegment, uint16_t* verts, uint16_t* index, bool bSolid)
{
if (m_nShape > SIDE_SHAPE_TRIANGLE)
	return;

	uint16_t			vSorted [4], bFlip;
	CFixVector		vNormal;
	CFloatVector	vNormalf;

#if DBG
if (nSegment == nDbgSeg)
	BRP;
#endif
m_nSegment = nSegment;
if (gameData.segData.nLevelVersion > 24) 
	index = m_corners;
SetupCorners (verts, index);
#if 1
if (m_nShape) {
	m_nType = SIDE_IS_TRI_02;
	SetupVertexList (verts, index);
	bFlip = SortVertsForNormal (m_corners [0], m_corners [1], m_corners [2], m_nShape ? 0xFFFF : m_corners [3], vSorted);
	vNormal = CFixVector::Normal (VERTICES [vSorted [0]], VERTICES [vSorted [1]], VERTICES [vSorted [2]]);
	vNormalf = CFloatVector::Normal (FVERTICES [vSorted [0]], FVERTICES [vSorted [1]], FVERTICES [vSorted [2]]);
	if (bFlip) {
		vNormal.Neg ();
		vNormalf.Neg ();
		}

	m_normals [0] = m_normals [1] = m_normals [2] = vNormal;
	m_fNormals [0] = m_fNormals [1] = m_fNormals [2] = vNormalf;
	}
else {
	SetupAsTriangles (bSolid, verts, index);
	m_nFaces = 2;
	m_normals [2] = CFixVector::Avg (m_normals [0], m_normals [1]);
	m_fNormals [2] = CFloatVector::Avg (m_fNormals [0], m_fNormals [1]);
	}
#else
fix xDistToPlane = m_nShape ? 0 : abs (VERTICES [vSorted [3]].DistToPlane (vNormal, VERTICES [vSorted [0]]));
if (!m_nShape)
	SetupAsQuad (vNormal, vNormalf, verts, index);
if (PLANE_DIST_TOLERANCE < DEFAULT_PLANE_DIST_TOLERANCE) {
	SetupAsTriangles (bSolid, verts, index);
	}
else {
	if (xDistToPlane <= PLANE_DIST_TOLERANCE)
		SetupAsQuad (vNormal, vNormalf, verts, index);
	else {
		SetupAsTriangles (bSolid, verts, index);
		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.
		Assert (m_nFaces == 2);
		fix dist0 = VERTICES [m_vertices [1]].DistToPlane (m_normals [1], VERTICES [m_nMinVertex [0]]);
		fix dist1 = VERTICES [m_vertices [4]].DistToPlane (m_normals [0], VERTICES [m_nMinVertex [0]]);
		int32_t s0 = sign (dist0);
		int32_t s1 = sign (dist1);
		if (s0 == 0 || s1 == 0 || s0 != s1)
			SetupAsQuad (vNormal, vNormalf, verts, index);
		}
	}
#endif

ComputeCenter ();
#if 1 //DBG
FixNormals ();
#endif
m_bIsQuad = !m_nShape && (m_normals [0] == m_normals [1]);
for (int32_t i = 0; i < m_nCorners; i++)
	AddToVertexNormal (m_corners [i], m_normals [2]);
}

// -------------------------------------------------------------------------------

int32_t CSide::HasVertex (uint16_t nVertex) 
{ 
for (int32_t i = 0; i < m_nCorners; i++)
	if (m_corners [i] == nVertex)
		return 1;
return 0;
}

// -------------------------------------------------------------------------------

CFixVector& CSide::Vertex (int32_t nVertex)
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
CSegMasks CSide::Masks (const CFixVector& refPoint, fix xRad, int16_t sideBit, int16_t faceBit, bool bCheckPoke)
{
	CSegMasks	masks;
	fix			xDist;

masks.m_valid = 1;
if (m_nFaces == 2) {
	int32_t	nSideCount = 0, 
			nCenterCount = 0;

	CFixVector vMin = MinVertex ();
	for (int32_t nFace = 0; nFace < 2; nFace++, faceBit <<= 1) {
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
#if DBG
	fix vDist [3];
	for (int32_t i = 0;  i < m_nCorners; i++)
		vDist [i] = CFixVector::Dist (refPoint, VERTICES [m_corners [i]]);
#endif
	xDist = refPoint.DistToPlane (Normal (0), MinVertex ());
	if (xDist < -PLANE_DIST_TOLERANCE)
		masks.m_center |= sideBit;
	if ((xDist - xRad < -PLANE_DIST_TOLERANCE) && (!bCheckPoke || (xDist + xRad >= -PLANE_DIST_TOLERANCE))) {
		masks.m_face |= faceBit;
		masks.m_side |= sideBit;
		}
	}
masks.m_valid = 1;
return masks;
}

// -------------------------------------------------------------------------------

uint8_t CSide::Dist (const CFixVector& refPoint, fix& xSideDist, int32_t bBehind, int16_t sideBit)
{
	fix	xDist;
	uint8_t mask = 0;

xSideDist = 0;
if (m_nFaces == 2) {
	int32_t nCenterCount = 0;

	CFixVector vMin = MinVertex ();
	for (int32_t nFace = 0; nFace < 2; nFace++) {
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

uint8_t CSide::Distf (const CFloatVector& refPoint, float& fSideDist, int32_t bBehind, int16_t sideBit)
{
	float	fDist;
	uint8_t mask = 0;
	CFloatVector vMin, vNormal;

vMin.Assign (MinVertex ());
fSideDist = 0;
if (m_nFaces == 2) {
	int32_t nCenterCount = 0;

	for (int32_t nFace = 0; nFace < 2; nFace++) {
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
	int32_t			i, j;

// compute vIntersection of perpendicular through pRef with the plane spanned up by the face
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
	int32_t				i, j;

// compute vIntersection of perpendicular through pRef with the plane spanned up by the face
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
return (m_nFaces && IS_WALL (m_nWall)) ? WALL (m_nWall) : NULL; 
}

//	-----------------------------------------------------------------------------
// Check whether point vPoint in segment nDestSeg can be seen from this side.
// Level 0: Check from side center, 1: check from center and corners, 2: check from center, corners, and edge centers

int32_t CSide::IsConnected (int16_t nSegment, int16_t nSide)
{
	CSide				*pSide = SEGMENT (nSegment)->Side (nSide);
	CFloatVector	vDir;

for (int32_t i = 4 - pSide->m_nShape; i > 0; --i) {
	uint16_t h = pSide->m_corners [i];
	for (int32_t j = 4 - m_nShape; j; --j)
		if (m_corners [j] == h)
			return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
// Check whether side nSide in segment nSegment can be seen from this side.

int32_t CSide::SeesSide (int16_t nSegment, int16_t nSide)
{
	CSide				*pSide = SEGMENT (nSegment)->Side (nSide);
	CFloatVector	vDir;
	
vDir.Assign (pSide->Center () - Center ());
CFloatVector::Normalize (vDir);
return (CFloatVector::Dot (vDir, Normalf (2)) > 0.0f) && (CFloatVector::Dot (vDir, pSide->Normalf (2)) < 0.0f);
}

//	-----------------------------------------------------------------------------
// Check whether side nSide in segment nSegment can be seen from this side.

int32_t CSide::SeesConnectedSide (int16_t nSegment, int16_t nSide)
{
return !IsConnected (nSegment, nSide) || SeesSide (nSegment, nSide);
}

//	-----------------------------------------------------------------------------
// Check whether point vPoint in segment nDestSeg can be seen from this side.
// Level 0: Check from side center, 1: check from center and corners, 2: check from center, corners, and edge centers

int32_t CSide::SeesPoint (CFixVector& vPoint, int16_t nDestSeg, int32_t nLevel, int32_t nThread)
{
	static int32_t nLevels [3] = {4, 0, -4};

	CFloatVector	v0, v1;
	int32_t			i, j;

v1.Assign (vPoint);

if (nLevel >= 0) 
	i = 4;
else {
	i = 3;
	nLevel = -nLevel - 1;
	}
j = nLevels [nLevel];

for (; i >= j; i--) {
	if (i == 4)
		v0.Assign (Center ());
	else if (i >= 0)
		v0 = FVERTICES [m_corners [i]];
	else
		v0 = CFloatVector::Avg (FVERTICES [m_corners [4 + i]], FVERTICES [m_corners [(5 + i) % 4]]); // center of face's edges
	if (PointSeesPoint (&v0, &v1, m_nSegment, nDestSeg, 0, nThread))
		return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//see if a pRef is inside a face by projecting into 2d
uint32_t CSide::PointToFaceRelationf (CFloatVector& vRef, int16_t nFace, CFloatVector& vNormal)
{
//now do 2d check to see if pRef is in CSide
//project polygon onto plane by finding largest component of Normal
CFloatVector vt;
vt.Set (fabs (vNormal.v.coord.x), fabs (vNormal.v.coord.y), fabs (vNormal.v.coord.z));
int32_t nProjPlane = (vt.v.coord.x > vt.v.coord.y) ? (vt.v.coord.x > vt.v.coord.z) ? 0 : 2 : (vt.v.coord.y > vt.v.coord.z) ? 1 : 2;

int32_t x, y;
if (vNormal.v.vec [nProjPlane] > 0.0f) {
	x = ijTable [nProjPlane][0];
	y = ijTable [nProjPlane][1];
	}
else {
	x = ijTable [nProjPlane][1];
	y = ijTable [nProjPlane][0];
	}
//now do the 2d problem in the x, y plane
CFloatVector2 vRef2D (vRef.v.vec [x], vRef.v.vec [y]);

int32_t i = nFace * 3;
CFloatVector v0, v1;
if (gameStates.render.bRendering)
	v1.Assign (RENDERPOINTS [m_vertices [i]].ViewPos ());
else
	v1 = FVERTICES [m_vertices [i]];

uint32_t nEdgeMask = 0;
for (int32_t nEdge = 1; nEdge <= 3; nEdge++) {
	v0 = v1;
	if (gameStates.render.bRendering)
		v1.Assign (RENDERPOINTS [m_vertices [i + nEdge % 3]].ViewPos ());
	else
		v1 = FVERTICES [m_vertices [i + nEdge % 3]];
	CFloatVector2 vEdge (v1.v.vec [x] - v0.v.vec [x], v1.v.vec [y] - v0.v.vec [y]);
	CFloatVector2 vCheck (vRef2D.m_x - v0.v.vec [x], vRef2D.m_y - v0.v.vec [y]);
	if (vCheck.Cross (vEdge) < PLANE_DIST_TOLERANCE) //we are outside of triangle
		nEdgeMask |= (1 << (nEdge - 1));
	}
return nEdgeMask;
}

//	-----------------------------------------------------------------------------
//see if a pRef is inside a face by projecting into 2d
uint32_t CSide::PointToFaceRelation (CFixVector& vRef, int16_t nFace, CFixVector vNormal)
{
#if 0

	CFixVector		t;
	int32_t			biggest;
	int32_t 			h, i, j, nEdge, nVerts;
	uint32_t 		nEdgeMask;
	fix 				check_i, check_j;
	CFixVector		*v0, *v1;
	CFixVector2		vEdge, vCheck;
	int64_t			d;

//now do 2d check to see if pRef is in CSide
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
check_i = vRef.v.vec [i];
check_j = vRef.v.vec [j];
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
	int32_t 			h, i, j, nEdge, nProjPlane;
	uint32_t 		nEdgeMask;
	CFixVector*		v0, * v1;
	CFixVector2		vEdge, vCheck, vRef2D;

//now do 2d check to see if pRef is in CSide
//project polygon onto plane by finding largest component of Normal
t.Set (labs (vNormal.v.coord.x), labs (vNormal.v.coord.y), labs (vNormal.v.coord.z));
if (t.v.coord.x > t.v.coord.y)
   nProjPlane = (t.v.coord.x > t.v.coord.z) ? 0 : 2;
else 
   nProjPlane = (t.v.coord.y > t.v.coord.z) ? 1 : 2;
if (vNormal.v.vec [nProjPlane] > 0) {
	i = ijTable [nProjPlane][0];
	j = ijTable [nProjPlane][1];
	}
else {
	i = ijTable [nProjPlane][1];
	j = ijTable [nProjPlane][0];
	}
//now do the 2d problem in the i, j plane
vRef2D.m_x = vRef.v.vec [i];
vRef2D.m_y = vRef.v.vec [j];
h = nFace * 3;
v1 = gameStates.render.bRendering ? &RENDERPOINTS [m_vertices [h]].ViewPos () : VERTICES + m_vertices [h];
for (nEdge = 1, nEdgeMask = 0; nEdge <= 3; nEdge++) {
	v0 = v1;
	v1 = gameStates.render.bRendering 
		  ? &RENDERPOINTS [m_vertices [h + nEdge % 3]].ViewPos ()
		  : VERTICES + m_vertices [h + nEdge % 3];
	vEdge.m_x = v1->v.vec [i] - v0->v.vec [i];
	vEdge.m_y = v1->v.vec [j] - v0->v.vec [j];
	vCheck.m_x = vRef2D.m_x - v0->v.vec [i];
	vCheck.m_y = vRef2D.m_y - v0->v.vec [j];
	if (vCheck.Cross (vEdge) < PLANE_DIST_TOLERANCE) //we are outside of triangle
		nEdgeMask |= (1 << (nEdge - 1));
	}
return nEdgeMask;

#endif
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int32_t CSide::SphereToFaceRelationf (CFloatVector& vRef, float rad, int16_t iFace, CFloatVector& vNormal)
{
	CFloatVector	vEdge, vCheck;            //this time, real 3d vectors
	CFloatVector	vNearest;
	float				xEdgeLen, d, dist;
	CFloatVector	v0, v1;
	int32_t			iType;
	int32_t			nEdge;
	uint32_t			nEdgeMask;

//now do 2d check to see if pRef is in side
nEdgeMask = PointToFaceRelationf (vRef, iFace, vNormal);
//we've gone through all the sides, are we inside?
if (nEdgeMask == 0)
	return IT_FACE;
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
iFace *= 3;
if (gameStates.render.bRendering) {
	v0.Assign (RENDERPOINTS [m_vertices [iFace + nEdge]].ViewPos ());
	v1.Assign (RENDERPOINTS [m_vertices [iFace + ((nEdge + 1) % 3)]].ViewPos ());
	}
else {
	v0 = FVERTICES [m_vertices [iFace + nEdge]];
	v1 = FVERTICES [m_vertices [iFace + ((nEdge + 1) % 3)]];
	}
//check if we are touching an edge or pRef
vCheck = vRef - v0;
vEdge = v1 - v0;
xEdgeLen = CFloatVector::Normalize (vEdge);
//find pRef dist from planes of ends of edge
d = CFloatVector::Dot (vEdge, vCheck);
if (d + rad < 0)
	return IT_NONE;                  //too far behind start pRef
if (d - rad > xEdgeLen)
	return IT_NONE;    //too far part end pRef
//find closest pRef on edge to check pRef
iType = IT_POINT;
if (d < 0)
	vNearest = v0;
else if (d > xEdgeLen)
	vNearest = v1;
else {
	iType = IT_EDGE;
#if 0
	vNearest = *v0 + (vEdge * d);
#else
	vNearest = vEdge;
	vNearest *= d;
	vNearest += v0;
#endif
	}
dist = CFloatVector::Dist (vRef, vNearest);
if (dist <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//check if a sphere intersects a face
int32_t CSide::SphereToFaceRelation (CFixVector& vRef, fix rad, int16_t iFace, CFixVector vNormal)
{
	CFixVector	vEdge, vCheck;            //this time, real 3d vectors
	CFixVector	vNearest;
	fix			xEdgeLen, d, dist;
	CFixVector*	v0, * v1;
	int32_t		iType;
	int32_t		nEdge;
	uint32_t		nEdgeMask;

//now do 2d check to see if pRef is in side
nEdgeMask = PointToFaceRelation (vRef, iFace, vNormal);
//we've gone through all the sides, are we inside?
if (nEdgeMask == 0)
	return IT_FACE;
//get verts for edge we're behind
for (nEdge = 0; !(nEdgeMask & 1); (nEdgeMask >>= 1), nEdge++)
	;
iFace *= 3;
if (gameStates.render.bRendering) {
	v0 = &RENDERPOINTS [m_vertices [iFace + nEdge]].ViewPos ();
	v1 = &RENDERPOINTS [m_vertices [iFace + ((nEdge + 1) % 3)]].ViewPos ();
	}
else {
	v0 = VERTICES + m_vertices [iFace + nEdge];
	v1 = VERTICES + m_vertices [iFace + ((nEdge + 1) % 3)];
	}
//check if we are touching an edge or pRef
vCheck = vRef - *v0;
xEdgeLen = CFixVector::NormalizedDir (vEdge, *v1, *v0);
//find pRef dist from planes of ends of edge
d = CFixVector::Dot (vEdge, vCheck);
if (d + rad < 0)
	return IT_NONE;                  //too far behind start pRef
if (d - rad > xEdgeLen)
	return IT_NONE;    //too far part end pRef
//find closest pRef on edge to check pRef
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
dist = CFixVector::Dist (vRef, vNearest);
if (dist <= rad)
	return (iType == IT_POINT) ? IT_NONE : iType;
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in vIntersection with vIntersection
//pRef on plane, whether or not line intersects CSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a pRef field
int32_t CSide::CheckLineToFaceRegularf (CFloatVector& vIntersection, CFloatVector* p0, CFloatVector* p1, float rad, int16_t iFace, CFloatVector& vNormal)
{
	int32_t bCheckRad = 0;

//use lowest pRef number
#if DBG
if (m_nFaces <= iFace) {
	Error ("invalid face number in CSegment::CheckLineToFace()");
	return IT_ERROR;
	}
#endif

CFloatVector v;
if (p1 == p0) {
	if (rad == 0.0f)
		return IT_NONE;
	v = vNormal * (-rad);
	v += *p0;
	bCheckRad = rad != 0.0f;
	rad = 0;
	p1 = &v;
	}

int32_t nVertex = m_nMinVertex [0];
if (gameStates.render.bRendering)
	v.Assign (RENDERPOINTS [nVertex].ViewPos ());
else
	v = FVERTICES [nVertex];
int32_t pli = FindPlaneLineIntersectionf (vIntersection, v, vNormal, *p0, *p1, rad);
if (!pli)
	return IT_NONE;
CFloatVector vHit = vIntersection;
//if rad != 0, project the pRef down onto the plane of the polygon
if (rad)
	vHit += vNormal * (-rad);
if ((pli = SphereToFaceRelationf (vHit, rad, iFace, vNormal)))
	return pli;
if (bCheckRad) {
	CFloatVector*	a, * b = FVERTICES + m_corners [0];
	for (int32_t i = 1; i <= m_nCorners; i++) {
		a = b;
		b = FVERTICES + m_corners [i % m_nCorners];
		float d = VmLinePointDist (*a, *b, *p0, 0);
		if (d < bCheckRad)
			return IT_POINT;
		}
	}
return IT_NONE;
}

//	-----------------------------------------------------------------------------
//returns true if line intersects with face. fills in vIntersection with vIntersection
//pRef on plane, whether or not line intersects CSide
//iFace determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a pRef field
int32_t CSide::CheckLineToFaceRegular (CFixVector& vIntersection, CFixVector *p0, CFixVector *p1, fix rad, int16_t iFace, CFixVector vNormal)
{
#if 0 // FLOAT_COLLISION_MATH

	CFloatVector vi, v0, v1, vn;

v0.Assign (*p0);
v1.Assign (*p1);
vn.Assign (vNormal);
int32_t i = CheckLineToFaceRegularf (vi, &v0, &v1, X2F (rad), iFace, vn);
if (i != IT_NONE)
	vIntersection.Assign (vi);
return i;

#else

	CFixVector	v1;
	int32_t		bCheckRad = 0;

//use lowest pRef number
#if DBG
if (m_nFaces <= iFace) {
	Error ("invalid face number in CSegment::CheckLineToFace()");
	return IT_ERROR;
	}
#endif

if (*p1 == *p0) {
	if (!rad)
		return IT_NONE;
	v1 = vNormal * (-rad);
	v1 += *p0;
	bCheckRad = rad;
	rad = 0;
	p1 = &v1;
	}

int32_t nVertex = m_nMinVertex [0];
int32_t pli = FindPlaneLineIntersection (vIntersection, gameStates.render.bRendering ? &RENDERPOINTS [nVertex].ViewPos () : VERTICES + nVertex, &vNormal, p0, p1, rad);
if (!pli)
	return IT_NONE;
CFixVector vHit = vIntersection;
//if rad != 0, project the pRef down onto the plane of the polygon
if (rad)
	vHit += vNormal * (-rad);
if ((pli = SphereToFaceRelation (vHit, rad, iFace, vNormal)))
	return pli;
if (bCheckRad) {
	int32_t		i, d;
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

#endif
}

//	-----------------------------------------------------------------------------
//this version is for when the start and end positions both poke through
//the plane of a CSide.  In this case, we must do checks against the edge
//of faces
int32_t CSide::CheckLineToFaceEdgesf (CFloatVector& vIntersection, CFloatVector& p0, CFloatVector& p1, float rad, int16_t iFace, CFloatVector& vNormal)
{
CFloatVector vMove = p1 - p0;
//figure out which edge(side) to check against
uint32_t nEdgeMask = PointToFaceRelationf (p0, iFace, vNormal);
if (!nEdgeMask)
	return CheckLineToFaceRegularf (vIntersection, &p0, &p1, rad, iFace, vNormal);
int32_t nEdge;
for (nEdge = 0; !(nEdgeMask & 1); nEdgeMask >>= 1, nEdge++)
	;
CFloatVector* edge_v0 = FVERTICES + m_vertices [iFace * 3 + nEdge];
CFloatVector* edge_v1 = FVERTICES + m_vertices [iFace * 3 + ((nEdge + 1) % 3)];
CFloatVector vEdge = *edge_v1 - *edge_v0;
//is the start pRef already touching the edge?
//first, find pRef of closest approach of vec & edge
float edge_len = CFloatVector::Normalize (vEdge);
float move_len = CFloatVector::Normalize (vMove);
float edge_t, move_t;
CheckLineToLinef (edge_t, move_t, *edge_v0, vEdge, p0, vMove);
//make sure t values are in valid range
if ((move_t < 0) || (move_t > move_len + rad))
	return IT_NONE;
float move_t2 = (move_t > move_len) ? move_len : move_t;
float edge_t2 = (edge_t < 0) ? 0 : (edge_t > edge_len) ? edge_len : edge_t;
//now, edge_t & move_t determine closest points.  calculate the points.
CFloatVector vClosestEdgePoint = *edge_v0 + vEdge * edge_t2;
CFloatVector vClosestMovePoint = p0 + vMove * move_t2;
//find dist between closest points
float closestDist = CFloatVector::Dist (vClosestEdgePoint, vClosestMovePoint);
//could we hit with this dist?
//note massive tolerance here
if (closestDist < rad * 0.9f) {		//we hit.  figure out where
	//now figure out where we hit
	vIntersection = p0 + vMove * (move_t - rad);
	return IT_EDGE;
	}
return IT_NONE;			//no hit
}

//	-----------------------------------------------------------------------------
//this version is for when the start and end positions both poke through
//the plane of a CSide.  In this case, we must do checks against the edge
//of faces
int32_t CSide::CheckLineToFaceEdges (CFixVector& vIntersection, CFixVector *p0, CFixVector *p1, fix rad, int16_t iFace, CFixVector vNormal)
{
#if 0 // FLOAT_COLLISION_MATH

	CFloatVector vi, v0, v1, vn;

v0.Assign (*p0);
v1.Assign (*p1);
vn.Assign (vNormal);
int32_t i = CheckLineToFaceEdgesf (vi, v0, v1, X2F (rad), iFace, vn);
if (i != IT_NONE)
	vIntersection.Assign (vi);
return i;

#else

CFixVector vMove = *p1 - *p0;
//figure out which edge(side) to check against
uint32_t nEdgeMask = PointToFaceRelation (*p0, iFace, vNormal);
if (!nEdgeMask)
	return CheckLineToFaceRegular (vIntersection, p0, p1, rad, iFace, vNormal);
int32_t nEdge;
for (nEdge = 0; !(nEdgeMask & 1); nEdgeMask >>= 1, nEdge++)
	;
CFixVector* v0 = VERTICES + m_vertices [iFace * 3 + nEdge];
CFixVector* v1 = VERTICES + m_vertices [iFace * 3 + (nEdge + 1) % 3];
CFixVector vEdge = *v1 - *v0;
//is the start pRef already touching the edge?
//first, find pRef of closest approach of vec & edge
fix edgeLen = CFixVector::Normalize (vEdge);
fix moveLen = CFixVector::Normalize (vMove);
fix edge_t, move_t;
CheckLineToLine (&edge_t, &move_t, v0, &vEdge, p0, &vMove);
//make sure t values are in valid range
if ((move_t < 0) || (move_t > moveLen + rad))
#if DBG
	return CheckLineToFaceRegular (vIntersection, p0, p1, rad, iFace, vNormal);
#else
	return IT_NONE;
#endif
//now, edge_t & move_t determine closest points.  calculate the points.
CFixVector vClosestEdgePoint = *v0 + vEdge * Clamp (edge_t, 0, edgeLen);
CFixVector vClosestMovePoint = *p0 + vMove * Min (move_t, moveLen);
//find dist between closest points
fix closestDist = CFixVector::Dist (vClosestEdgePoint, vClosestMovePoint);
//could we hit with this dist?
//note massive tolerance here
if (closestDist < (rad * 9) / 10) {		//we hit.  figure out where
	//now figure out where we hit
	vIntersection = *p0 + vMove * (move_t - rad);
	return IT_EDGE;
	}
return IT_NONE;			//no hit

#endif
}

//	-----------------------------------------------------------------------------
//finds the uv coords of the given pRef on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
void CSide::HitPointUV (fix *u, fix *v, fix *l, CFixVector& vIntersection, int32_t iFace)
{
	CFixVector*		vPoints;
	CFixVector		vNormal;
	int32_t			nProjPlane, nProjX, nProjY;
	tUVL				uvls [3];
	int32_t			h;

if (iFace >= m_nFaces) {
	PrintLog (0, "invalid face number in CSide::HitPointUV\n");
	*u = *v = 0;
	return;
	}
//now the hard work.
//1. find what plane to project this CWall onto to make it a 2d case
vNormal = m_normals [iFace];
nProjPlane = 0;
if (abs (vNormal.v.coord.y) > abs (vNormal.v.vec [nProjPlane]))
	nProjPlane = 1;
if (abs (vNormal.v.coord.z) > abs (vNormal.v.vec [nProjPlane]))
	nProjPlane = 2;
nProjX = (nProjPlane == 0);
nProjY = (nProjPlane == 2) ? 1 : 2;
//2. compute u, v of vIntersection pRef
//vec from 1 -> 0
h = iFace * 3;
vPoints = VERTICES + m_vertices [h+1];
CFloatVector2 vRef (vPoints->v.vec [nProjX], vPoints->v.vec [nProjY]);
vPoints = VERTICES + m_vertices [h];
CFloatVector2 vec0 (vPoints->v.vec [nProjX], vPoints->v.vec [nProjY]);
vPoints = VERTICES + m_vertices [h+2];
CFloatVector2 vec1 (vPoints->v.vec [nProjX], vPoints->v.vec [nProjY]);
vec0 -= vRef;
vec1 -= vRef;
//vec from 1 -> checkPoint
CFloatVector2 vHit (vIntersection.v.vec [nProjX], vIntersection.v.vec [nProjY]);
float k1 = (vHit.Cross (vec0) + vec0.Cross (vRef)) / vec0.Cross (vec1);
float k0 = (fabs (vec0.m_x) > fabs (vec0.m_y))
			  ? (k1 * vec1.m_x + vHit.m_x - vRef.m_x) / vec0.m_x
			  : (k1 * vec1.m_y + vHit.m_y - vRef.m_y) / vec0.m_y;
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
return IS_WALL (m_nWall) ? WALL (m_nWall)->IsOpenableDoor () : false;
}

//------------------------------------------------------------------------------

CFixVector* CSide::GetCornerVertices (CFixVector* vertices) 
{ 
	int32_t i;

for (i = 0; i < m_nCorners; i++)
	vertices [i] = VERTICES [m_corners [i]];
for (; i < m_nCorners; i++)
	vertices [i].Set (0x7fffffff, 0x7fffffff, 0x7fffffff);
return vertices;
}

//------------------------------------------------------------------------------
// Move all vertex id indices > nDeletedIndex down one step

void CSide::RemapVertices (uint8_t* map)
{
for (int32_t i = 0; i < m_nCorners; i++)
	m_corners [i] = map [m_corners [i]];
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
return IsWall () && WALL (m_nWall)->IsVolatile ();
}


//------------------------------------------------------------------------------

bool CSide::IsSolid (void)
{
return IsWall () && Wall ()->IsSolid ();
}

//------------------------------------------------------------------------------

int32_t CSide::Physics (fix& damage, bool bSolid)
{
if (!bSolid) {
	CWall* pWall = Wall ();
	if (!pWall || (pWall->nType != WALL_ILLUSION) || (pWall->flags & WALL_ILLUSION_OFF))
		return 0;
	}
if (gameData.pig.tex.pTexMapInfo [m_nBaseTex].damage) {
	damage = gameData.pig.tex.pTexMapInfo [m_nBaseTex].damage;
	return 3;
	}
if (gameData.pig.tex.pTexMapInfo [m_nBaseTex].flags & TMI_WATER)
	return bSolid ? 2 : 4;
return 0;
}

//-----------------------------------------------------------------

int32_t CSide::CheckTransparency (void)
{
	CBitmap	*pBm;

if (m_nOvlTex) {
	pBm = gameData.pig.tex.pBitmap [gameData.pig.tex.pBmIndex [m_nOvlTex].index].Override (-1);
	if (pBm->Flags () & BM_FLAG_SUPER_TRANSPARENT)
		return 1;
	if (!(pBm->Flags () & BM_FLAG_TRANSPARENT))
		return 0;
	}
pBm = gameData.pig.tex.pBitmap [gameData.pig.tex.pBmIndex [m_nBaseTex].index].Override (-1);
if (pBm->Flags () & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 1;
if (gameStates.app.bD2XLevel && IS_WALL (m_nWall)) {
	int16_t c = WALL (m_nWall)->cloakValue;
	if (c && (c < FADE_LEVELS))
		return 1;
	}
return gameOpts->render.effects.bAutoTransparency && IsTransparentTexture (m_nBaseTex);
}

//------------------------------------------------------------------------------

void CSide::SetTextures (int32_t nBaseTex, int32_t nOvlTex)
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

int32_t CSide::Read (CFile& cf, uint16_t* segVerts, uint16_t* sideVerts, bool bSolid)
{
	int32_t nType = bSolid ? 1 : IS_WALL (m_nWall) ? 2 : 0;

m_nFrame = 0;
m_nShape = 0;
m_nCorners = 4;
if (gameData.segData.nLevelVersion > 24) {
	for (int32_t i = 0; i < 4; i++) 
		if (0xff == (m_corners [i] = cf.ReadUByte ()))
			m_nShape++;
	m_nCorners = 4 - m_nShape;
	for (int32_t i = 0; i < m_nCorners; i++)
		sideVerts [i] = segVerts [m_corners [i]];
	}
if (!nType)
	m_nBaseTex =
	m_nOvlTex = 0;
else {
	// Read int16_t pSide->m_nBaseTex;
	uint16_t nTexture;
	if (bNewFileFormat) {
		nTexture = cf.ReadUShort ();
		m_nBaseTex = nTexture & 0x7fff;
		}
	else {
		nTexture = 0;
		m_nBaseTex = cf.ReadShort ();
		}
	if (gameData.segData.nLevelVersion <= 1)
		m_nBaseTex = ConvertD1Texture (m_nBaseTex, 0);
	if (bNewFileFormat && !(nTexture & 0x8000))
		m_nOvlTex = 0;
	else {
		// Read int16_t m_nOvlTex;
		nTexture = cf.ReadShort ();
		m_nOvlTex = nTexture & 0x3fff;
		m_nOvlOrient = (nTexture >> 14) & 3;
		if ((gameData.segData.nLevelVersion <= 1) && m_nOvlTex)
			m_nOvlTex = ConvertD1Texture (m_nOvlTex, 0);
		}
	// guess what? One level from D2:CS contains a texture id "910". D'oh.
	m_nBaseTex %= MAX_WALL_TEXTURES;	
	m_nOvlTex %= MAX_WALL_TEXTURES;

	// Read tUVL m_uvls [4] (u, v>>5, write as int16_t, l>>1 write as int16_t)
	for (int32_t i = 0; i < 4; i++) {
		m_uvls [i].u = fix (cf.ReadShort ()) << 5;
		m_uvls [i].v = fix (cf.ReadShort ()) << 5;
		uint16_t l = cf.ReadUShort ();
		m_uvls [i].l = fix (l) << 1;
		float fBrightness = X2F (m_uvls [i].l);
		if ((i < m_nCorners) && (gameData.render.color.vertBright [sideVerts [i]] < fBrightness))
			gameData.render.color.vertBright [sideVerts [i]] = fBrightness;
		}
	}
		
return nType;
}

//------------------------------------------------------------------------------

void CSide::ReadWallNum (CFile& cf, bool bWall)
{
m_nWall = uint16_t (bWall ? (gameData.segData.nLevelVersion >= 13) ? cf.ReadUShort () : cf.ReadUByte () : -1);
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
	int16_t nTexture = cf.ReadShort ();
m_nOvlTex = nTexture & 0x3fff;
m_nOvlOrient = (nTexture >> 14) & 3;
}

//------------------------------------------------------------------------------

//eof
