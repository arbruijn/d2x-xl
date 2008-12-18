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
#include "inferno.h"
#include "error.h"
#include "mono.h"
#include "gameseg.h"
#include "byteswap.h"
#include "light.h"
#include "segment.h"

// How far a point can be from a plane, and still be "in" the plane

// ------------------------------------------------------------------------------------------
// Compute the center point of a CSide of a CSegment.
//	The center point is defined to be the average of the 4 points defining the CSide.
void CSide::ComputeCenter (void)
{
m_vCenter = gameData.segs.vertices [m_vertices [0]];
m_vCenter += gameData.segs.vertices [m_vertices [1]];
m_vCenter += gameData.segs.vertices [m_vertices [2]];
m_vCenter += gameData.segs.vertices [m_vertices [3]];
m_vCenter [X] /= 4;
m_vCenter [Y] /= 4;
m_vCenter [Z] /= 4;
}

// ------------------------------------------------------------------------------------------

void CSide::ComputeRads (void)
{
	fix 			d, rMin = 0x7fffffff, rMax = 0;
	CFixVector	v;

m_rads [0] = 0x7fffffff;
m_rads [1] = 0;
for (int i = 0; i < 4; i++) {
	v = CFixVector::Avg (gameData.segs.vertices [m_vertices [i]], gameData.segs.vertices [m_vertices [(i + 1) % 4]]);
	d = CFixVector::Dist (v, m_vCenter);
	if (m_rads [0] > d)
		m_rads [0] = d;
	d = CFixVector::Dist (m_vCenter, gameData.segs.vertices [m_vertices [i]]);
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

// -----------------------------------------------------------------------------------
// Fill in array with four absolute point numbers for a given CSide
CFixVector* CSide::GetVertices (CFixVector* vertices)
{
vertices [0] = gameData.segs.vertices [m_vertices [0]];
vertices [1] = gameData.segs.vertices [m_vertices [1]];
vertices [2] = gameData.segs.vertices [m_vertices [2]];
vertices [3] = gameData.segs.vertices [m_vertices [3]];
return vertices;
}

// -------------------------------------------------------------------------------

int CSide::CreateVertexList (short* verts, int* index)
{
m_nFaces = -1;
m_nMinVertex [0] = min (m_vertices [0], m_vertices [2]);
if (m_nType == SIDE_IS_QUAD) {
	m_vertices [0] = verts [index [0]];
	m_vertices [1] = verts [index [1]];
	m_vertices [2] = verts [index [2]];
	m_vertices [3] = verts [index [3]];
	m_nMinVertex = m_vertices [0];
	if (m_nMinVertex [0] > m_vertices [1])
		m_nMinVertex [0] = m_vertices [1];
	if (m_nMinVertex [0] > m_vertices [3])
		m_nMinVertex [0] = m_vertices [3];
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
	m_nMinVertex [1] = min (m_vertices [1], m_vertices [4]);
	m_nFaces = 2;
	}
else {
	Error ("Illegal CSide nType (3), nType = %i, CSegment # = %i, CSide # = %i\n", sideP->nType, nSegment, nSide);
	m_nFaces = -1;
	}
return CreateFaceVertIndex ();
}

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the CSide.
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0, 1, 2,
//	face #1 is stored in vertices 3, 4, 5.

void CSide::CreateFaceVertexIndex (void)
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
else {
	Error ("Illegal CSide nType (2), nType = %i, CSegment # = %i, CSide # = %i\n", m_nType, nSegment, nSide);
	return -1;
	}
return m_nFaces;
}

// -------------------------------------------------------------------------------

void CSide::AddAsQuad (CFixVector& vNormal)
{
m_nType = SIDE_IS_QUAD;
m_normals [0] = 
m_normals [1] = vNormal;
}

// -------------------------------------------------------------------------------

void CSide::AddAsTwoTriangles (bool bSolid)
{
	CFixVector	vNormal;
	short			v0 = m_vertices [0];
	short			v1 = m_vertices [1];
	short			v2 = m_vertices [2];
	short			v3 = m_vertices [3];
	fix			dot;
	CFixVector	vec_13;		//	vector from vertex 1 to vertex 3

	//	Choose how to triangulate.
	//	If a CWall, then
	//		Always triangulate so CSegment is convex.
	//		Use Matt's formula: Na . AD > 0, where ABCD are vertices on CSide, a is face formed by A, B, C, Na is Normal from face a.
	//	If not a CWall, then triangulate so whatever is on the other CSide is triangulated the same (ie, between the same absoluate vertices)
if (bSolid) {
	vNormal = CFixVector::Normal (gameData.segs.vertices [v0],
	                              gameData.segs.vertices [v1],
	                              gameData.segs.vertices [v2]);
	vec_13 = gameData.segs.vertices [v3] - gameData.segs.vertices [v1];
	dot = CFixVector::Dot (vNormal, vec_13);

	//	Now, signify whether to triangulate from 0:2 or 1:3
	m_nType = (dot >= 0) ? SIDE_IS_TRI_02 : SIDE_IS_TRI_13;
	//	Now, based on triangulation nType, set the normals.
	if (m_nType == SIDE_IS_TRI_02) {
		//VmVecNormalChecked (&vNormal, gameData.segs.vertices + v0, gameData.segs.vertices + v1, gameData.segs.vertices + v2);
		m_normals [0] = vNormal;
		m_normals [1] = CFixVector::Normal (gameData.segs.vertices [v0], gameData.segs.vertices [v2], gameData.segs.vertices [v3]);
		}
	else {
		m_normals [0] = CFixVector::Normal (gameData.segs.vertices [v0], gameData.segs.vertices [v1], gameData.segs.vertices [v3]);
		m_normals [1] = CFixVector::Normal (gameData.segs.vertices [v1], gameData.segs.vertices [v2], gameData.segs.vertices [v3]);
		}
	}
else {
	int	vSorted [4];
	int	bFlip;

	bFlip = GetVertsForNormal (v0, v1, v2, v3, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
	if ((vSorted [0] == v0) || (vSorted [0] == v2)) {
		m_nType = SIDE_IS_TRI_02;
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = GetVertsForNormal (v0, v1, v2, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		m_normals [0] = CFixVector::Normal (gameData.segs.vertices [vSorted [0]], 
														gameData.segs.vertices [vSorted [1]], 
														gameData.segs.vertices [vSorted [2]]);
		if (bFlip)
			m_normals [0].Neg ();
		bFlip = GetVertsForNormal (v0, v2, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		m_normals [1] = CFixVector::Normal (gameData.segs.vertices [vSorted [0]],
														gameData.segs.vertices [vSorted [1]],
														gameData.segs.vertices [vSorted [2]]);
		if (bFlip)
			m_normals [1].Neg ();
		GetVertsForNormal (v0, v2, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		}
	else {
		m_nType = SIDE_IS_TRI_13;
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = GetVertsForNormal (v0, v1, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		m_normals [0] = CFixVector::Normal (gameData.segs.vertices [vSorted [0]],
														gameData.segs.vertices [vSorted [1]],
														gameData.segs.vertices [vSorted [2]]);
		if (bFlip)
			m_normals [0].Neg ();
		bFlip = GetVertsForNormal (v1, v2, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		m_normals [1] = CFixVector::Normal (
						 gameData.segs.vertices [vSorted [0]],
						 gameData.segs.vertices [vSorted [1]],
						 gameData.segs.vertices [vSorted [2]]);
		if (bFlip)
			m_normals [1].Neg ();
		}
	}
}

// -------------------------------------------------------------------------------

void CSide::CreateWalls (bool bSolid)
{
	int			vm0, vm1, vm2, vm3, bFlip;
	int			v0, v1, v2, v3, i;
	int			vertexList [6];
	CFixVector	vNormal;
	fix			xDistToPlane;
	sbyte			*s2v = sideVertIndex [nSide];

	v0 = segP->verts [s2v [0]];
	v1 = segP->verts [s2v [1]];
	v2 = segP->verts [s2v [2]];
	v3 = segP->verts [s2v [3]];
	bFlip = GetVertsForNormal (m_vertices [0], m_vertices [1], m_vertices [2], m_vertices [3], &vm0, &vm1, &vm2, &vm3);
	vNormal = CFixVector::Normal (gameData.segs.vertices [vm0], gameData.segs.vertices [vm1], gameData.segs.vertices [vm2]);
	xDistToPlane = abs (gameData.segs.vertices [vm3].DistToPlane (vNormal, gameData.segs.vertices [vm0]));
	if (bFlip)
		vNormal.Neg ();
#if 1
	if (bRenderQuads || (xDistToPlane <= PLANE_DIST_TOLERANCE))
		AddAsQuad (&vNormal);
	else {
		AddAsTwoTriangles (bSolid);
		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.
		Assert (m_nFaces == 2);
		fix dist0 = gameData.segs.vertices [m_vertices [1]].DistToPlane (m_normals [1], gameData.segs.vertices [m_nMinVertex]);
		fix dist1 = gameData.segs.vertices [m_vertices [4]].DistToPlane (m_normals [0], gameData.segs.vertices [m_nMinVertex]);
		int s0 = sign (dist0);
		int s1 = sign (dist1);
		if (s0 == 0 || s1 == 0 || s0 != s1) {
			m_nType = SIDE_IS_QUAD; 	//detriangulate!
		}
	}
#endif
if (m_nType == SIDE_IS_QUAD) {
	AddToVertexNormal (m_vertices [0], &vNormal);
	AddToVertexNormal (m_vertices [1], &vNormal);
	AddToVertexNormal (m_vertices [2], &vNormal);
	AddToVertexNormal (m_vertices [3], &vNormal);
	}
else {
	for (i = 0; i < 3; i++)
		AddToVertexNormal (vertexList [i], m_normals);
	for (; i < 6; i++)
		AddToVertexNormal (vertexList [i], m_normals + 1);
	}
}

// -------------------------------------------------------------------------------
//	Make a just-modified CSegment CSide valid.
void CSide::Validate (bool bSolid)
{
CreateWalls (bSolid);
}

// -------------------------------------------------------------------------------

inline CFixVector& CSide::Vertex (int nVertex)
{
return gameStates.render.bRendering ? gameData.segs.points [nVertex].p3_vec : gameData.segs.vertices [nVertex];
}

// -------------------------------------------------------------------------------

inline CFixVector& CSide::MinVertex (void)
{
return Vertex (m_nMinVertex [0]);
}

// -------------------------------------------------------------------------------
// height of a two faced, non-planar side

inline fix CSide::Height (void)
{
return Vertex (m_nMinVertex [1]).DistToPlane (Normal (m_vertices [4] >= m_vertices [1]), Vertex (m_nMinVertex [0]));
}

// -------------------------------------------------------------------------------

inline bool CSide::IsPlanar (void)
{
return (m_nFaces < 2) || (Height () <= PLANE_DIST_TOLERANCE);
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See CSegMasks structure for info on fields
CSegMasks CSide::Masks (const CFixVector& refPoint, fix xRad, short sideBit, short& faceBit)
{
	CSegMasks	masks;
	fix			xDist;

masks.valid = 1;
if (m_nFaces == 2) {
	int	nSideCount = 0, 
			nCenterCount = 0;

	CFixVector minVertex = MinVertex ();
	for (int nFace = 0; nFace < 2; nFace++, faceBit <<= 1) {
		xDist = refPoint.DistToPlane (Normal (nFace), minVertex);
		if (xDist - xRad < -PLANE_DIST_TOLERANCE) {	// xRad must be >= 0!
			masks.faceMask |= faceBit;
			nSideCount++;
			if (xDist < -PLANE_DIST_TOLERANCE) //in front of face
				nCenterCount++;
			}
		}
	if (IsPlanar ()) {	//must be behind both faces
		if (nSideCount == 2)
			masks.sideMask |= sideBit;
		if (nCenterCount == 2)
			masks.centerMask |= sideBit;
		}
	else {	//must be behind at least one face
		if (nSideCount)
			masks.sideMask |= sideBit;
		if (nCenterCount)
			masks.centerMask |= sideBit;
		}
	}
else {	
	xDist = refPoint.DistToPlane (Normal (0), MinVertex ());
	if (xDist - xRad < -PLANE_DIST_TOLERANCE) {	// xRad must be >= 0!
		masks.faceMask |= faceBit;
		masks.sideMask |= sideBit;
		if (xDist < -PLANE_DIST_TOLERANCE)
			masks.centerMask |= sideBit;
		}
	faceBit <<= 2;
	}
masks.valid = 1;
return masks;
}

// -------------------------------------------------------------------------------

ubyte CSide::Dist (const CFixVector& refPoint, fix& xSideDist, int bBehind, short sideBit)
{
	fix	xDist;
	int	nVertex;
	ubyte mask = 0;

xSideDist = 0;
if (m_nFaces == 2) {
	int nCenterCount = 0;

	CFixVector minVertex = MinVertex ();
	for (nFace = 0; nFace < 2; nFace++, faceBit <<= 1) {
		xDist = refPoint.DistToPlane (Normal (nFace), minVertex);
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

//	-----------------------------------------------------------------------------

inline CWall* CSide::Wall (void) 
{ 
return IS_WALL (m_nWall) ? WALLS + m_nWall : NULL; 
}

//------------------------------------------------------------------------------

//eof
