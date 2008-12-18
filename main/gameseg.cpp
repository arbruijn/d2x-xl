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

// ------------------------------------------------------------------------------------------
// Compute the center point of a CSide of a CSegment.
//	The center point is defined to be the average of the 4 points defining the CSide.
void CSegment::ComputeSideRads (void)
{
for (int i = 0; i < 6; i++)
	m_sides [i].ComputeRads ();
}

// ------------------------------------------------------------------------------------------
// Compute CSegment center.
//	The center point is defined to be the average of the 8 points defining the CSegment.
void CSegment::ComputeCenter (void)
{
m_vCenter = gameData.segs.vertices [m_verts [0]];
m_vCenter += gameData.segs.vertices [m_verts [1]];
m_vCenter += gameData.segs.vertices [m_verts [2]];
m_vCenter += gameData.segs.vertices [m_verts [3]];
m_vCenter += gameData.segs.vertices [m_verts [4]];
m_vCenter += gameData.segs.vertices [m_verts [5]];
m_vCenter += gameData.segs.vertices [m_verts [6]];
m_vCenter += gameData.segs.vertices [m_verts [7]];
m_vCenter [X] /= 8;
m_vCenter [Y] /= 8;
m_vCenter [Z] /= 8;
}

// -----------------------------------------------------------------------------

void CSegment::ComputeRads (fix xMinDist)
{
	CFixVector	vMin, vMax, v;
	fix			xDist;

m_rads [0] = xMinDist;
m_rads [1] = 0;
vMin [X] = vMin [Y] = vMin [Z] = 0x7FFFFFFF;
vMax [X] = vMax [Y] = vMax [Z] = -0x7FFFFFFF;
for (int i = 0; i < 8; i++) {
	v = m_vCenter - gameData.segs.vertices [m_verts [i]];
	if (vMin [X] > v [X])
		vMin [X] = v [X];
	if (vMin [Y] > v [Y])
		vMin [Y] = v [Y];
	if (vMin [Z] > v [Z])
		vMin [Z] = v [Z];
	if (vMax [X] < v [X])
		vMax [X] = v [X];
	if (vMax [Y] < v [Y])
		vMax [Y] = v [Y];
	if (vMax [Z] < v [Z])
		vMax [Z] = v [Z];
	xDist = v.Mag ();
	if (m_rads [1] < xDist)
		m_rads [1] = xDist;
	}
m_extents [0] = vMin;
m_extents [1] = vMax;
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment

int CSegment::FindConnectedSide (CSegment* other)
{
	short		nSegment = SEG_IDX (this);
	ushort*	childP = other->m_children;

if (childP [0] == nSegment)
		return 0;
if (childP [1] == nSegment)
		return 1;
if (childP [2] == nSegment)
		return 2;
if (childP [3] == nSegment)
		return 3;
if (childP [4] == nSegment)
		return 4;
if (childP [5] == nSegment)
		return 5;
return -1;
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
void GetSideVertIndex (ushort* vertIndex, int nSegment, int nSide)
{
	sbyte *sv = sideVertIndex [nSide];
	short	*vp = SEGMENTS [nSegment].m_verts;

vertIndex [0] = vp [sv [0]];
vertIndex [1] = vp [sv [1]];
vertIndex [2] = vp [sv [2]];
vertIndex [3] = vp [sv [3]];
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

#ifdef EDITOR
// -----------------------------------------------------------------------------------
//	Create all vertex lists (1 or 2) for faces on a CSide.
//	Sets:
//		nFaces		number of lists
//		vertices			vertices in all (1 or 2) faces
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0, 1, 2,
//	face #1 is stored in vertices 3, 4, 5.
// Note: these are not absolute vertex numbers, but are relative to the CSegment
// Note:  for triagulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
void CreateAllVertexLists (int *nFaces, int *vertices, int nSegment, int nSide)
{
	CSide	*sideP = &SEGMENTS [nSegment].m_sides [nSide];
	int  *sv = sideToVertsInt [nSide];

Assert ((nSegment <= gameData.segs.nLastSegment) && (nSegment >= 0);
Assert ((nSide >= 0) && (nSide < 6);

switch (sideP->nType) {
	case SIDE_IS_QUAD:

		vertices [0] = sv [0];
		vertices [1] = sv [1];
		vertices [2] = sv [2];
		vertices [3] = sv [3];

		*nFaces = 1;
		break;
	case SIDE_IS_TRI_02:
		*nFaces = 2;
		vertices [0] =
		vertices [5] = sv [0];
		vertices [1] = sv [1];
		vertices [2] =
		vertices [3] = sv [2];
		vertices [4] = sv [3];

		//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS ()
		//CREATE_ABS_VERTEX_LISTS (), CREATE_ALL_VERTEX_LISTS (), CREATE_ALL_VERTNUM_LISTS ()
		break;
	case SIDE_IS_TRI_13:
		*nFaces = 2;
		vertices [0] =
		vertices [5] = sv [3];
		vertices [1] = sv [0];
		vertices [2] =
		vertices [3] = sv [1];
		vertices [4] = sv [2];
		//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS ()
		//CREATE_ABS_VERTEX_LISTS (), CREATE_ALL_VERTEX_LISTS (), CREATE_ALL_VERTNUM_LISTS ()
		break;
	default:
		Error ("Illegal CSide nType (1), nType = %i, CSegment # = %i, CSide # = %i\n", sideP->nType, nSegment, nSide);
		break;
	}
}
#endif

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the CSide.
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0, 1, 2,
//	face #1 is stored in vertices 3, 4, 5.

void CreateAllVertNumLists (int *nFaces, int *vertnums, int nSegment, int nSide)
{
	CSide	*sideP = &SEGMENTS [nSegment].m_sides [nSide];

Assert ((nSegment <= gameData.segs.nLastSegment) && (nSegment >= 0));
switch (sideP->nType) {
	case SIDE_IS_QUAD:
		vertnums [0] = 0;
		vertnums [1] = 1;
		vertnums [2] = 2;
		vertnums [3] = 3;
		*nFaces = 1;
		break;

	case SIDE_IS_TRI_02:
		*nFaces = 2;
		vertnums [0] =
		vertnums [5] = 0;
		vertnums [1] = 1;
		vertnums [2] =
		vertnums [3] = 2;
		vertnums [4] = 3;
		//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS ()
		//CREATE_ABS_VERTEX_LISTS (), CREATE_ALL_VERTEX_LISTS (), CREATE_ALL_VERTNUM_LISTS ()
		break;

	case SIDE_IS_TRI_13:
		*nFaces = 2;
		vertnums [0] =
		vertnums [5] = 3;
		vertnums [1] = 0;
		vertnums [2] =
		vertnums [3] = 1;
		vertnums [4] = 2;
		//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS ()
		//CREATE_ABS_VERTEX_LISTS (), CREATE_ALL_VERTEX_LISTS (), CREATE_ALL_VERTNUM_LISTS ()
		break;

	default:
		Error ("Illegal CSide nType (2), nType = %i, CSegment # = %i, CSide # = %i\n", sideP->nType, nSegment, nSide);
		break;
	}
}

// -------------------------------------------------------------------------------

int CSide::CreateVertexList (short* verts, int* index)
{
m_nFaces = -1;
m_nMinVertex = -1;
if (m_nType == SIDE_IS_QUAD) {
	m_vertices [0] = verts [index [0]];
	m_vertices [1] = verts [index [1]];
	m_vertices [2] = verts [index [2]];
	m_vertices [3] = verts [index [3]];
	m_nMinVertex = m_vertices [0];
	if (m_nMinVertex > m_vertices [1])
		m_nMinVertex = m_vertices [1];
	if (m_nMinVertex > m_vertices [2])
		m_nMinVertex = m_vertices [2];
	if (m_nMinVertex > m_vertices [3])
		m_nMinVertex = m_vertices [3];
	m_nFaces = 1;
	}
else if (m_nType == SIDE_IS_TRI_02) {
	m_vertices [0] =
	m_vertices [5] = verts [index [0]];
	m_vertices [1] = verts [index [1]];
	m_vertices [2] =
	m_vertices [3] = verts [index [2]];
	m_vertices [4] = verts [index [3]];
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
	Error ("Illegal CSide nType (3), nType = %i, CSegment # = %i, CSide # = %i\n", sideP->nType, nSegment, nSide);
	m_nFaces = -1;
	}
return m_nFaces;
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See tSegMasks structure for info on fields
tSegMasks CSide::Masks (const CFixVector& refP, fix xRad, short sideBit, short& faceBit)
{
	int			nVertex, nSideCount, nCenterCount, bSidePokesOut;
	fix			xDist;
	tSegMasks	masks;

masks.centerMask = 0;
masks.faceMask = 0;
masks.sideMask = 0;
masks.valid = 1;
if (m_nFaces == 2) {
	nVertex = min (m_vertices [0], m_vertices [2]);
	if (m_vertices [4] < m_vertices [1])
		if (gameStates.render.bRendering)
			xDist = gameData.segs.points [m_vertices[4]].p3_vec.DistToPlane (m_rotNorms [0], gameData.segs.points [nVertex].p3_vec);
		else
			xDist = gameData.segs.vertices [m_vertices[4]].DistToPlane (m_m_normals [0], gameData.segs.vertices [nVertex]);
	else
		if (gameStates.render.bRendering)
			xDist = gameData.segs.points [m_vertices [1]].p3_vec.DistToPlane (m_rotNorms [1], gameData.segs.points [nVertex].p3_vec);
		else
			xDist = gameData.segs.vertices [m_vertices[1]].DistToPlane (m_m_normals [1], gameData.segs.vertices [nVertex]);
	bSidePokesOut = (xDist > PLANE_DIST_TOLERANCE);
	nSideCount = nCenterCount = 0;

	for (int nFace = 0; nFace < 2; nFace++, faceBit <<= 1) {
		if (gameStates.render.bRendering)
			xDist = refP.DistToPlane (m_rotNorms [nFace], gameData.segs.points [nVertex].p3_vec);
		else
			xDist = refP.DistToPlane (m_m_normals [nFace], gameData.segs.vertices [nVertex]);
		if (xDist < -PLANE_DIST_TOLERANCE) //in front of face
			nCenterCount++;
		if (xDist - xRad < -PLANE_DIST_TOLERANCE) {
			masks.faceMask |= faceBit;
			nSideCount++;
			}
		}
	if (bSidePokesOut) {		//must be behind at least one face
		if (nSideCount)
			masks.sideMask |= sideBit;
		if (nCenterCount)
			masks.centerMask |= sideBit;
		}
	else {							//must be behind both faces
		if (nSideCount == 2)
			masks.sideMask |= sideBit;
		if (nCenterCount == 2)
			masks.centerMask |= sideBit;
		}
	}
else {	
	xDist = gameStates.render.bRendering
			  ? refP.DistToPlane (m_rotNorms [0], gameData.segs.points [m_nMinVertex].p3_vec)
			  : refP.DistToPlane (m_m_normals [0], gameData.segs.vertices [m_nMinVertex]);
	if (xDist < -PLANE_DIST_TOLERANCE)
		masks.centerMask |= sideBit;
	if (xDist - xRad < -PLANE_DIST_TOLERANCE) {
		masks.faceMask |= faceBit;
		masks.sideMask |= sideBit;
		}
	faceBit <<= 2;
	}
masks.valid = 1;
return masks;
}

// -------------------------------------------------------------------------------
//returns 3 different bitmasks with info telling if this sphere is in
//this CSegment.  See tSegMasks structure for info on fields
tSegMasks CSegment::GetSideMasks (const CFixVector& refP, fix xRad)
{
	short			nSide, faceBit;

masks.centerMask = 0;
masks.faceMask = 0;
masks.sideMask = 0;
//check point against each CSide of CSegment. return bitmask
for (nSide = 0, faceBit = 1; nSide < 6; nSide++)
	sideMasks = m_sides [nSide].Masks (refP, rad, 1 << nSide, faceBit);
	masks.centerMask |= sideMasks.centerMask;
	masks.faceMask |= sideMasks.faceMask;
	masks.sideMask |= sideMasks.sideMask;
	}
masks.valid = 1;
return masks;
}

// -------------------------------------------------------------------------------

ubyte CSide::Dist (const CFixVector& refP, fix& xSideDist, int bBehind, short sideBit)
{
	fix	xDist;
	int	nVertex;
	ubyte mask = 0;

xSideDist = 0;
if (m_nFaces == 2) {
	nVertex = min (m_vertices [0], m_vertices [2]);
	if (m_vertices [4] < m_vertices [1])
		if (gameStates.render.bRendering)
			xDist = gameData.segs.points [m_vertices [4]].p3_vec.DistToPlane (m_rotNorms [0], gameData.segs.points [nVertex].p3_vec);
		else
			xDist = gameData.segs.vertices [m_vertices [4]].DistToPlane (m_normals [0], gameData.segs.vertices [nVertex]);
	else
		if (gameStates.render.bRendering)
			xDist = gameData.segs.points [m_vertices [1]].p3_vec.DistToPlane (m_rotNorms [1], gameData.segs.points [nVertex].p3_vec);
		else
			xDist = gameData.segs.vertices [m_vertices [1]].DistToPlane (m_normals [1], gameData.segs.vertices [nVertex]);

	bool bSidePokesOut = (xDist > PLANE_DIST_TOLERANCE);
	int nCenterCount = 0;

	for (nFace = 0; nFace < 2; nFace++, faceBit <<= 1) {
		if (gameStates.render.bRendering)
			xDist = refP.DistToPlane (m_rotNorms [nFace], gameData.segs.points [nVertex].p3_vec);
		else
			xDist = refP.DistToPlane (m_normals [nFace], gameData.segs.vertices [nVertex]);
		if ((xDist < -PLANE_DIST_TOLERANCE) == bBehind) {	//in front of face
			nCenterCount++;
			xSideDist += xDist;
			}
		}
	if (!bSidePokesOut) {		//must be behind both faces
		if (nCenterCount == 2) {
			mask |= sideBit;
			xSideDist /= 2;		//get average
			}
		}
	else {							//must be behind at least one face
		if (nCenterCount) {
			mask |= sideBit;
			if (nCenterCount == 2)
				xSideDist /= 2;		//get average
			}
		}
	}
else {				//only one face on this CSide
	//use lowest point number
	)
		xDist = gameStates.render.bRendering 
				  ? refP.DistToPlane (m_rotNorms [0], gameData.segs.points [m_nMinVertex].p3_vec)
				  : refP.DistToPlane (m_normals [0], gameData.segs.vertices [m_nMinVertex]);
	if ((xDist < -PLANE_DIST_TOLERANCE) == bBehind) {
		mask |= sideBit;
		xSideDist = xDist;
		}
	}
return mask;
}

// -------------------------------------------------------------------------------
//this was converted from GetSegMasks ()...it fills in an array of 6
//elements for the distace behind each CSide, or zero if not behind
//only gets centerMask, and assumes zero rad
ubyte CSegment::GetSideDists (const CFixVector& refP, fix* xSideDists, int bBehind)
{
	ubyte		mask = 0;

for (nSide = 0; nSide < 6; nSide++)
	mask |= segP->m_sides [nSide].GetDist (refP, xSideDists [nSide], bBehind, 1 << nSide);
return mask;
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
//	Used to become a constant based on editor, but I wanted to be able to set
//	this for omega blob FindSegByPos calls.  Would be better to pass a paremeter
//	to the routine...--MK, 01/17/96
int	bDoingLightingHack=0;

//figure out what seg the given point is in, tracing through segments
//returns CSegment number, or -1 if can't find CSegment
int TraceSegs (const CFixVector& p0, int nOldSeg, int nTraceDepth, char* bVisited)
{
	CSegment			*segP;
	fix				xSideDists, xMaxDist;
	int				centerMask, nMaxSide, nSide, bit, nMatchSeg = -1;

if (nTraceDepth >= gameData.segs.nSegments)
	return -1;
if (bVisited [nOldSeg])
	return -1;
bVisited [nOldSeg] = 1;
if (!(centerMask = SEGMENTS [nOldSeg].GetSideDists (p0, xSideDists, 1)))		//we're in the old CSegment
	return nOldSeg;		
segP = SEGMENTS + nOldSeg;
for (;;) {
	nMaxSide = -1;
	xMaxDist = 0;
	for (nSide = 0, bit = 1; nSide < 6; nSide ++, bit <<= 1)
		if ((centerMask & bit) && (segP->m_children [nSide] > -1) && (xSideDists [nSide] < xMaxDist)) {
			xMaxDist = xSideDists [nSide];
			nMaxSide = nSide;
			}
	if (nMaxSide == -1)
		break;
	xSideDists [nMaxSide] = 0;
	if (0 <= (nMatchSeg = TraceSegs (p0, segP->m_children [nMaxSide], nTraceDepth + 1, bVisited)))	//trace into adjacent CSegment
		break;
	}
return nMatchSeg;		//we haven't found a CSegment
}



int	nExhaustiveCount=0, nExhaustiveFailedCount=0;

// -------------------------------------------------------------------------------
//Tries to find a CSegment for a point, in the following way:
// 1. Check the given CSegment
// 2. Recursivel [Y] trace through attached segments
// 3. Check all the segmentns
//Returns nSegment if found, or -1
int FindSegByPos (const CFixVector& p, int nSegment, int bExhaustive, int bSkyBox, int nThread)
{
	static char		bVisited [2][MAX_SEGMENTS_D2X]; 

	int		nNewSeg, i;
	short		*segP;
#if 0
	static	int nSemaphore = 0;

while (nSemaphore > 0)
	G3_SLEEP (0);
if (nSemaphore < 0)
	nSemaphore = 0;
nSemaphore++;
#endif
//allow nSegment == -1, meaning we have no idea what CSegment point is in
Assert ((nSegment <= gameData.segs.nLastSegment) && (nSegment >= -1));
if (nSegment != -1) {
	memset (bVisited [nThread], 0, gameData.segs.nSegments);
	nNewSeg = TraceSegs (p, nSegment, 0, bVisited [nThread]);
	if (nNewSeg != -1)//we found a CSegment!
		goto funcExit;
	}
//couldn't find via attached segs, so search all segs
if (bDoingLightingHack || !bExhaustive) {
	nNewSeg = -1;
	goto funcExit;
	}
++nExhaustiveCount;
#if 0 //TRACE
console.printf (1, "Warning: doing exhaustive search to find point CSegment (%i times)\n", nExhaustiveCount);
#endif
if (bSkyBox) {
	for (i = gameData.segs.skybox.ToS (), segP = gameData.segs.skybox.Buffer (); i; i--, segP++)
		if (!GetSegMasks (p, *segP, 0).centerMask)
			goto funcExit;
	}
else {
	for (nNewSeg = 0; nNewSeg <= gameData.segs.nLastSegment; nNewSeg++)
		if ((SEGMENTS [nNewSeg].m_nType != SEGMENT_IS_SKYBOX) && !GetSegMasks (p, nNewSeg, 0).centerMask)
			goto funcExit;
	}
nNewSeg = -1;
++nExhaustiveFailedCount;
#if TRACE
console.printf (1, "Warning: could not find point CSegment (%i times)\n", nExhaustiveFailedCount);
#endif

funcExit:
#if 0
if (nSemaphore > 0)
	nSemaphore--;
else
	nSemaphore = 0;
#endif
return nNewSeg;		//no CSegment found
}

// -------------------------------------------------------------------------------

short FindClosestSeg (CFixVector& vPos)
{
	short		nSegment, nClosestSeg = -1;
	fix		nDist, nMinDist = 0x7fffffff;

for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++) {
	nDist = CFixVector::Dist (vPos, *SEGMENT_CENTER_I (nSegment)) - MinSegRad (nSegment);
	if (nDist < nMinDist) {
		nMinDist = nDist;
		nClosestSeg = nSegment;
		}
	}
return nClosestSeg;
}

//--repair-- //	------------------------------------------------------------------------------
//--repair-- void clsd_repair_center (int nSegment)
//--repair-- {
//--repair-- 	int	nSide;
//--repair--
//--repair-- 	//	--- Set repair center bit for all repair center segments.
//--repair-- 	if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_REPAIRCEN) {
//--repair-- 		Lsegments [nSegment].specialType |= SS_REPAIR_CENTER;
//--repair-- 		Lsegments [nSegment].special_segment = nSegment;
//--repair-- 	}
//--repair--
//--repair-- 	//	--- Set repair center bit for all segments adjacent to a repair center.
//--repair-- 	for (nSide=0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
//--repair-- 		int	s = SEGMENTS [nSegment].children [nSide];
//--repair--
//--repair-- 		if ((s != -1) && (SEGMENTS [s].m_nType == SEGMENT_IS_REPAIRCEN)) {
//--repair-- 			Lsegments [nSegment].specialType |= SS_REPAIR_CENTER;
//--repair-- 			Lsegments [nSegment].special_segment = s;
//--repair-- 		}
//--repair-- 	}
//--repair-- }

//--repair-- //	------------------------------------------------------------------------------
//--repair-- //	--- Set destination points for all Materialization centers.
//--repair-- void clsd_materialization_center (int nSegment)
//--repair-- {
//--repair-- 	if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_ROBOTMAKER) {
//--repair--
//--repair-- 	}
//--repair-- }
//--repair--
//--repair-- int	Lsegment_highest_segment_index, Lsegment_highest_vertex_index;
//--repair--
//--repair-- //	------------------------------------------------------------------------------
//--repair-- //	Create data specific to mine which doesn't get written to disk.
//--repair-- //	gameData.segs.nLastSegment and gameData.objs.nLastObject [0] must be valid.
//--repair-- //	07/21:	set repair center bit
//--repair-- void create_local_segment_data (void)
//--repair-- {
//--repair-- 	int	nSegment;
//--repair--
//--repair-- 	//	--- Initialize all Lsegments.
//--repair-- 	for (nSegment=0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
//--repair-- 		Lsegments [nSegment].specialType = 0;
//--repair-- 		Lsegments [nSegment].special_segment = -1;
//--repair-- 	}
//--repair--
//--repair-- 	for (nSegment=0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
//--repair--
//--repair-- 		clsd_repair_center (nSegment);
//--repair-- 		clsd_materialization_center (nSegment);
//--repair--
//--repair-- 	}
//--repair--
//--repair-- 	//	Set check variables.
//--repair-- 	//	In main game loop, make sure these are valid, else Lsegments is not valid.
//--repair-- 	Lsegment_highest_segment_index = gameData.segs.nLastSegment;
//--repair-- 	Lsegment_highest_vertex_index = gameData.segs.nLastVertex;
//--repair-- }
//--repair--
//--repair-- //	------------------------------------------------------------------------------------------
//--repair-- //	Sort of makes sure create_local_segment_data has been called for the currently executing mine.
//--repair-- //	It is not failsafe, as pos [Y]u will see if pos [Y]u look at the code.
//--repair-- //	Returns 1 if Lsegments appears valid, 0 if not.
//--repair-- int check_lsegments_validity (void)
//--repair-- {
//--repair-- 	return ((Lsegment_highest_segment_index == gameData.segs.nLastSegment) && (Lsegment_highest_vertex_index == gameData.segs.nLastVertex);
//--repair-- }

#define	MAX_LOC_POINT_SEGS	64

#define	MIN_CACHE_FCD_DIST	 (F1_0*80)	//	Must be this far apart for cache lookup to succeed.  Recognizes small changes in distance matter at small distances.
//	----------------------------------------------------------------------------------------------------------

void FlushFCDCache (void)
{
	int	i;

gameData.fcd.nIndex = 0;
for (i = 0; i < MAX_FCD_CACHE; i++)
	gameData.fcd.cache [i].seg0 = -1;
}

//	----------------------------------------------------------------------------------------------------------

void AddToFCDCache (int seg0, int seg1, int nDepth, fix dist)
{
	if (dist > MIN_CACHE_FCD_DIST) {
		gameData.fcd.cache [gameData.fcd.nIndex].seg0 = seg0;
		gameData.fcd.cache [gameData.fcd.nIndex].seg1 = seg1;
		gameData.fcd.cache [gameData.fcd.nIndex].csd = nDepth;
		gameData.fcd.cache [gameData.fcd.nIndex].dist = dist;

		gameData.fcd.nIndex++;

		if (gameData.fcd.nIndex >= MAX_FCD_CACHE)
			gameData.fcd.nIndex = 0;

	} else {
		//	If it's in the cache, remove it.
		int	i;

		for (i=0; i<MAX_FCD_CACHE; i++)
			if (gameData.fcd.cache [i].seg0 == seg0)
				if (gameData.fcd.cache [i].seg1 == seg1) {
					gameData.fcd.cache [gameData.fcd.nIndex].seg0 = -1;
					break;
				}
	}

}

//	----------------------------------------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum nDepth of nMaxDepth.
//	Return the distance.
fix FindConnectedDistance (CFixVector *p0, short seg0, CFixVector *p1, short seg1, int nMaxDepth, int widFlag, int bUseCache)
{
	short				nConnSide;
	short				nCurSeg, nParentSeg, nThisSeg;
	short				nSide;
	int				qTail = 0, qHead = 0;
	int				i, nCurDepth, nPoints;
	sbyte				visited [MAX_SEGMENTS_D2X];
	segQueueEntry	segmentQ [MAX_SEGMENTS_D2X];
	short				nDepth [MAX_SEGMENTS_D2X];
	tPointSeg		pointSegs [MAX_LOC_POINT_SEGS];
	fix				dist;
	CSegment			*segP;
	tFCDCacheData	*pc;

	//	If > this, will overrun pointSegs buffer
if (nMaxDepth > MAX_LOC_POINT_SEGS-2) {
#if TRACE
	console.printf (1, "Warning: In FindConnectedDistance, nMaxDepth = %i, limited to %i\n", nMaxDepth, MAX_LOC_POINT_SEGS-2);
#endif
	nMaxDepth = MAX_LOC_POINT_SEGS - 2;
	}
if (seg0 == seg1) {
	gameData.fcd.nConnSegDist = 0;
	return CFixVector::Dist(*p0, *p1);
	}
nConnSide = FindConnectedSide (SEGMENTS + seg0, SEGMENTS + seg1);
if ((nConnSide != -1) &&
	 (SEGMENTS [seg1].IsDoorWay (nConnSide, NULL) & widFlag)) {
	gameData.fcd.nConnSegDist = 1;
	return CFixVector::Dist(*p0, *p1);
	}
//	Periodically flush cache.
if ((gameData.time.xGame - gameData.fcd.xLastFlushTime > F1_0*2) ||
	 (gameData.time.xGame < gameData.fcd.xLastFlushTime)) {
	FlushFCDCache ();
	gameData.fcd.xLastFlushTime = gameData.time.xGame;
	}

//	Can't quickly get distance, so see if in gameData.fcd.cache.
if (bUseCache) {
	for (i = MAX_FCD_CACHE, pc = gameData.fcd.cache; i; i--, pc++)
		if ((pc->seg0 == seg0) && (pc->seg1 == seg1)) {
			gameData.fcd.nConnSegDist = pc->csd;
			return pc->dist;
			}
	}
memset (visited, 0, gameData.segs.nLastSegment + 1);
memset (nDepth, 0, sizeof (nDepth [0]) * (gameData.segs.nLastSegment + 1));

nPoints = 0;
nCurSeg = seg0;
visited [nCurSeg] = 1;
nCurDepth = 0;

while (nCurSeg != seg1) {
	segP = SEGMENTS + nCurSeg;

	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
		if (segP->IsDoorWay (nSide, NULL) & widFlag) {
			nThisSeg = segP->m_children [nSide];
			Assert ((nThisSeg >= 0) && (nThisSeg < MAX_SEGMENTS));
			Assert ((qTail >= 0) && (qTail < MAX_SEGMENTS - 1));
			if (!visited [nThisSeg]) {
				segmentQ [qTail].start = nCurSeg;
				segmentQ [qTail].end = nThisSeg;
				visited [nThisSeg] = 1;
				nDepth [qTail++] = nCurDepth+1;
				if (nMaxDepth != -1) {
					if (nDepth [qTail - 1] == nMaxDepth) {
						gameData.fcd.nConnSegDist = 1000;
						AddToFCDCache (seg0, seg1, gameData.fcd.nConnSegDist, F1_0*1000);
						return -1;
						}
					}
				else if (nThisSeg == seg1) {
					goto fcd_done1;
				}
			}
		}
	}	//	for (nSide...

	if (qHead >= qTail) {
		gameData.fcd.nConnSegDist = 1000;
		AddToFCDCache (seg0, seg1, gameData.fcd.nConnSegDist, F1_0*1000);
		return -1;
		}
	Assert ((qHead >= 0) && (qHead < MAX_SEGMENTS));
	nCurSeg = segmentQ [qHead].end;
	nCurDepth = nDepth [qHead];
	qHead++;

fcd_done1: ;
	}	//	while (nCurSeg ...

//	Set qTail to the CSegment which ends at the goal.
while (segmentQ [--qTail].end != seg1)
	if (qTail < 0) {
		gameData.fcd.nConnSegDist = 1000;
		AddToFCDCache (seg0, seg1, gameData.fcd.nConnSegDist, F1_0*1000);
		return -1;
		}

while (qTail >= 0) {
	nThisSeg = segmentQ [qTail].end;
	nParentSeg = segmentQ [qTail].start;
	pointSegs [nPoints].nSegment = nThisSeg;
	COMPUTE_SEGMENT_CENTER_I (&pointSegs [nPoints].point, nThisSeg);
	nPoints++;
	if (nParentSeg == seg0)
		break;
	while (segmentQ [--qTail].end != nParentSeg)
		Assert (qTail >= 0);
	}
pointSegs [nPoints].nSegment = seg0;
COMPUTE_SEGMENT_CENTER_I (&pointSegs [nPoints].point, seg0);
nPoints++;
if (nPoints == 1) {
	gameData.fcd.nConnSegDist = nPoints;
	return CFixVector::Dist(*p0, *p1);
	}
else {
	fix	ndist;
	dist = CFixVector::Dist(*p1, pointSegs [1].point);
	dist += CFixVector::Dist(*p0, pointSegs [nPoints-2].point);
	for (i = 1; i < nPoints - 2; i++) {
		ndist = CFixVector::Dist(pointSegs [i].point, pointSegs [i+1].point);
		dist += ndist;
		}
	}
gameData.fcd.nConnSegDist = nPoints;
AddToFCDCache (seg0, seg1, nPoints, dist);
return dist;
}

// -------------------------------------------------------------------------------

sbyte convert_to_byte (fix f)
{
	if (f >= 0x00010000)
		return MATRIX_MAX;
	else if (f <= -0x00010000)
		return -MATRIX_MAX;
	else
		return (sbyte) (f >> MATRIX_PRECISION);
}

#define VEL_PRECISION 12

// -------------------------------------------------------------------------------
//	Create a tShortPos struct from an CObject.
//	Extract the matrix into byte values.
//	Create a position relative to vertex 0 with 1/256 Normal "fix" precision.
//	Stuff CSegment in a short.
void CreateShortPos (tShortPos *spp, CObject *objP, int swap_bytes)
{
	// int	nSegment;
	CFixMatrix orient = objP->info.position.mOrient;
	sbyte   *segP = spp->orient;
	CFixVector *pv;

	*segP++ = convert_to_byte(orient.RVec ()[X]);
	*segP++ = convert_to_byte(orient.UVec ()[X]);
	*segP++ = convert_to_byte(orient.FVec ()[X]);
	*segP++ = convert_to_byte(orient.RVec ()[Y]);
	*segP++ = convert_to_byte(orient.UVec ()[Y]);
	*segP++ = convert_to_byte(orient.FVec ()[Y]);
	*segP++ = convert_to_byte(orient.RVec ()[Z]);
	*segP++ = convert_to_byte(orient.UVec ()[Z]);
	*segP++ = convert_to_byte(orient.FVec ()[Z]);

	pv = gameData.segs.vertices + SEGMENTS [objP->info.nSegment].verts [0];
	spp->pos [X] = (short) ((objP->info.position.vPos [X] - (*pv)[X]) >> RELPOS_PRECISION);
	spp->pos [Y] = (short) ((objP->info.position.vPos [Y] - (*pv)[Y]) >> RELPOS_PRECISION);
	spp->pos [Z] = (short) ((objP->info.position.vPos [Z] - (*pv)[Z]) >> RELPOS_PRECISION);

	spp->nSegment = objP->info.nSegment;

 	spp->vel [X] = (short) ((objP->mType.physInfo.velocity[X]) >> VEL_PRECISION);
	spp->vel [Y] = (short) ((objP->mType.physInfo.velocity[Y]) >> VEL_PRECISION);
	spp->vel [Z] = (short) ((objP->mType.physInfo.velocity[Z]) >> VEL_PRECISION);

// swap the short values for the big-endian machines.

	if (swap_bytes) {
		spp->pos [X] = INTEL_SHORT (spp->pos [X]);
		spp->pos [Y] = INTEL_SHORT (spp->pos [Y]);
		spp->pos [Z] = INTEL_SHORT (spp->pos [Z]);
		spp->nSegment = INTEL_SHORT (spp->nSegment);
		spp->vel [X] = INTEL_SHORT (spp->vel [X]);
		spp->vel [Y] = INTEL_SHORT (spp->vel [Y]);
		spp->vel [Z] = INTEL_SHORT (spp->vel [Z]);
	}
}

// -------------------------------------------------------------------------------

void ExtractShortPos (CObject *objP, tShortPos *spp, int swap_bytes)
{
	int	nSegment;
	sbyte   *segP;
	CFixVector *pv;

	segP = spp->orient;

	objP->info.position.mOrient.RVec()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.RVec()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.RVec()[Z] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec()[Z] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec()[Z] = *segP++ << MATRIX_PRECISION;

	if (swap_bytes) {
		spp->pos [X] = INTEL_SHORT (spp->pos [X]);
		spp->pos [Y] = INTEL_SHORT (spp->pos [Y]);
		spp->pos [Z] = INTEL_SHORT (spp->pos [Z]);
		spp->nSegment = INTEL_SHORT (spp->nSegment);
		spp->vel [X] = INTEL_SHORT (spp->vel [X]);
		spp->vel [Y] = INTEL_SHORT (spp->vel [Y]);
		spp->vel [Z] = INTEL_SHORT (spp->vel [Z]);
	}

	nSegment = spp->nSegment;

	Assert ((nSegment >= 0) && (nSegment <= gameData.segs.nLastSegment));

	pv = gameData.segs.vertices + SEGMENTS [nSegment].verts [0];
	objP->info.position.vPos [X] = (spp->pos [X] << RELPOS_PRECISION) + (*pv)[X];
	objP->info.position.vPos [Y] = (spp->pos [Y] << RELPOS_PRECISION) + (*pv)[Y];
	objP->info.position.vPos [Z] = (spp->pos [Z] << RELPOS_PRECISION) + (*pv)[Z];

	objP->mType.physInfo.velocity[X] = (spp->vel [X] << VEL_PRECISION);
	objP->mType.physInfo.velocity[Y] = (spp->vel [Y] << VEL_PRECISION);
	objP->mType.physInfo.velocity[Z] = (spp->vel [Z] << VEL_PRECISION);

	objP->RelinkToSeg (nSegment);

}

//--unused-- void test_shortpos (void)
//--unused-- {
//--unused-- 	tShortPos	spp;
//--unused--
//--unused-- 	CreateShortPos (&spp, &OBJECTS [0]);
//--unused-- 	ExtractShortPos (&OBJECTS [0], &spp);
//--unused--
//--unused-- }

//	-----------------------------------------------------------------------------
//	Segment validation functions.
//	Moved from editor to game so we can compute surface normals at load time.
// -------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------
//	Extract a vector from a CSegment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
void extract_vector_from_segment (CSegment *segP, CFixVector *vp, int start, int end)
{
	int			i;
	CFixVector	vs, ve;

	vs.SetZero();
	ve.SetZero();

	for (i=0; i<4; i++) {
		vs += gameData.segs.vertices [segP->verts [sideVertIndex [start][i]]];
		ve += gameData.segs.vertices [segP->verts [sideVertIndex [end][i]]];
	}

	*vp = ve - vs;
	*vp *= (F1_0/4);

}

// -------------------------------------------------------------------------------
//create a matrix that describes the orientation of the given CSegment
void ExtractOrientFromSegment (CFixMatrix *m, CSegment *seg)
{
	CFixVector fVec, uVec;

	extract_vector_from_segment (seg, &fVec, WFRONT, WBACK);
	extract_vector_from_segment (seg, &uVec, WBOTTOM, WTOP);

	//vector to matrix does normalizations and orthogonalizations
	*m = CFixMatrix::CreateFU(fVec, uVec);
//	*m = CFixMatrix::CreateFU(fVec, &uVec, NULL);
}

#ifdef EDITOR
// ------------------------------------------------------------------------------------------
//	Extract the forward vector from CSegment *segP, return in *vp.
//	The forward vector is defined to be the vector from the the center of the front face of the CSegment
// to the center of the back face of the CSegment.
void extract_forward_vector_from_segment (CSegment *segP, CFixVector *vp)
{
	extract_vector_from_segment (segP, vp, WFRONT, WBACK);
}

// ------------------------------------------------------------------------------------------
//	Extract the right vector from CSegment *segP, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the CSegment
// to the center of the right face of the CSegment.
void extract_right_vector_from_segment (CSegment *segP, CFixVector *vp)
{
	extract_vector_from_segment (segP, vp, WLEFT, WRIGHT);
}

// ------------------------------------------------------------------------------------------
//	Extract the up vector from CSegment *segP, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the CSegment
// to the center of the top face of the CSegment.
void extract_up_vector_from_segment (CSegment *segP, CFixVector *vp)
{
	extract_vector_from_segment (segP, vp, WBOTTOM, WTOP);
}
#endif

// -------------------------------------------------------------------------------

void AddSideAsQuad (CSegment *segP, int nSide, CFixVector *Normal)
{
	CSide	*sideP = segP->m_sides + nSide;

	sideP->nType = SIDE_IS_QUAD;
	#ifdef COMPACT_SEGS
		Normal = Normal;		//avoid compiler warning
	#else
	sideP->m_normals [0] = *Normal;
	sideP->m_normals [1] = *Normal;
	#endif

	//	If there is a connection here, we only formed the faces for the purpose of determining CSegment boundaries,
	//	so don't generate polys, else they will get rendered.
//	if (segP->m_children [nSide] != -1)
//		sideP->renderFlag = 0;
//	else
//		sideP->renderFlag = 1;

}


// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *bFlip set, then negate Normal after computation.
//	Note, pos [Y]u cannot just compute the Normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
int GetVertsForNormal (int v0, int v1, int v2, int v3, int *pv0, int *pv1, int *pv2, int *pv3)
{
	int	i, j, t;
	int	v [4], w [4] = {0, 1, 2, 3};

//	w is a list that shows how things got scrambled so we know if our Normal is pointing backwards

v [0] = v0;
v [1] = v1;
v [2] = v2;
v [3] = v3;
// bubble sort v in reverse order (largest first)
for (i = 1; i < 4; i++)
	for (j = 0; j < i; j++)
		if (v [j] > v [i]) {
			t = v [j]; v [j] = v [i]; v [i] = t;
			t = w [j]; w [j] = w [i]; w [i] = t;
			}

Assert ((v [0] < v [1]) && (v [1] < v [2]) && (v [2] < v [3]));
*pv0 = v [0];
*pv1 = v [1];
*pv2 = v [2];
*pv3 = v [3];
//	Now, if for any w [i] & w [i+1]: w [i+1] = (w [i]+3)%4, then must flip Normal
return ((((w [0] + 3) % 4) == w [1]) || (((w [1] + 3) % 4) == w [2]));
}

// -------------------------------------------------------------------------------

void AddSideAsTwoTriangles (CSegment *segP, int nSide)
{
	CFixVector	vNormal;
	sbyte       *vs = sideVertIndex [nSide];
	short			v0 = segP->verts [vs [0]];
	short			v1 = segP->verts [vs [1]];
	short			v2 = segP->verts [vs [2]];
	short			v3 = segP->verts [vs [3]];
	fix			dot;
	CFixVector	vec_13;		//	vector from vertex 1 to vertex 3

	CSide	*sideP = segP->m_sides + nSide;

	//	Choose how to triangulate.
	//	If a CWall, then
	//		Always triangulate so CSegment is convex.
	//		Use Matt's formula: Na . AD > 0, where ABCD are vertices on CSide, a is face formed by A, B, C, Na is Normal from face a.
	//	If not a CWall, then triangulate so whatever is on the other CSide is triangulated the same (ie, between the same absoluate vertices)
#if DBG
if ((SEG_IDX (segP) == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
	segP = segP;
#endif
if (!IS_CHILD (segP->m_children [nSide])) {
	vNormal = CFixVector::Normal(gameData.segs.vertices [v0],
	                            gameData.segs.vertices [v1],
	                            gameData.segs.vertices [v2]);
	vec_13 = gameData.segs.vertices [v3] - gameData.segs.vertices [v1];
	dot = CFixVector::Dot(vNormal, vec_13);

	//	Now, signify whether to triangulate from 0:2 or 1:3
	if (dot >= 0)
		sideP->nType = SIDE_IS_TRI_02;
	else
		sideP->nType = SIDE_IS_TRI_13;

#	ifndef COMPACT_SEGS
	//	Now, based on triangulation nType, set the normals.
	if (sideP->nType == SIDE_IS_TRI_02) {
		//VmVecNormalChecked (&vNormal, gameData.segs.vertices + v0, gameData.segs.vertices + v1, gameData.segs.vertices + v2);
		sideP->m_normals [0] = vNormal;
		vNormal = CFixVector::Normal(gameData.segs.vertices [v0], gameData.segs.vertices [v2], gameData.segs.vertices [v3]);
		sideP->m_normals [1] = vNormal;
		}
	else {
		vNormal = CFixVector::Normal(gameData.segs.vertices [v0], gameData.segs.vertices [v1], gameData.segs.vertices [v3]);
		sideP->m_normals [0] = vNormal;
		vNormal = CFixVector::Normal(gameData.segs.vertices [v1], gameData.segs.vertices [v2], gameData.segs.vertices [v3]);
		sideP->m_normals [1] = vNormal;
		}
#	endif
	}
else {
	int	vSorted [4];
	int	bFlip;

	bFlip = GetVertsForNormal (v0, v1, v2, v3, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
	if ((vSorted [0] == v0) || (vSorted [0] == v2)) {
		sideP->nType = SIDE_IS_TRI_02;
#	ifndef COMPACT_SEGS
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = GetVertsForNormal (v0, v1, v2, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		sideP->m_normals [0] = CFixVector::Normal(
						 gameData.segs.vertices [vSorted[0]],
						 gameData.segs.vertices [vSorted[1]],
						 gameData.segs.vertices [vSorted[2]]);
		if (bFlip)
			sideP->m_normals [0].Neg();
		bFlip = GetVertsForNormal (v0, v2, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		sideP->m_normals [1] = CFixVector::Normal(
						 gameData.segs.vertices [vSorted[0]],
						 gameData.segs.vertices [vSorted[1]],
						 gameData.segs.vertices [vSorted[2]]);
		if (bFlip)
			sideP->m_normals [1].Neg();
		GetVertsForNormal (v0, v2, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
#	endif
		}
	else {
		sideP->nType = SIDE_IS_TRI_13;
#	ifndef COMPACT_SEGS
		//	Now, get vertices for Normal for each triangle based on triangulation nType.
		bFlip = GetVertsForNormal (v0, v1, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		sideP->m_normals [0] = CFixVector::Normal(
						 gameData.segs.vertices [vSorted[0]],
						 gameData.segs.vertices [vSorted[1]],
						 gameData.segs.vertices [vSorted[2]]);
		if (bFlip)
			sideP->m_normals [0].Neg();
		bFlip = GetVertsForNormal (v1, v2, v3, 32767, vSorted, vSorted + 1, vSorted + 2, vSorted + 3);
		sideP->m_normals [1] = CFixVector::Normal(
						 gameData.segs.vertices [vSorted[0]],
						 gameData.segs.vertices [vSorted[1]],
						 gameData.segs.vertices [vSorted[2]]);
		if (bFlip)
			sideP->m_normals [1].Neg();
#	endif
		}
	}
}

// -------------------------------------------------------------------------------

int sign (fix v)
{
if (v > PLANE_DIST_TOLERANCE)
	return 1;
if (v < - (PLANE_DIST_TOLERANCE+1))		//neg & pos round differently
	return -1;
return 0;
}

// -------------------------------------------------------------------------------

void AddToVertexNormal (int nVertex, CFixVector *pvNormal)
{
	g3sNormal	*pn = &gameData.segs.points [nVertex].p3_normal;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
pn->nFaces++;
pn->vNormal[X] += X2F ((*pvNormal)[X]);
pn->vNormal[Y] += X2F ((*pvNormal)[Y]);
pn->vNormal[Z] += X2F ((*pvNormal)[Z]);
}

// -------------------------------------------------------------------------------

int bRenderQuads = 0;

void CreateWallsOnSide (CSegment *segP, int nSide)
{
	int			vm0, vm1, vm2, vm3, bFlip;
	int			v0, v1, v2, v3, i;
	int			vertexList [6];
	CFixVector	vn;
	fix			xDistToPlane;
	sbyte			*s2v = sideVertIndex [nSide];

	v0 = segP->verts [s2v [0]];
	v1 = segP->verts [s2v [1]];
	v2 = segP->verts [s2v [2]];
	v3 = segP->verts [s2v [3]];
	bFlip = GetVertsForNormal (v0, v1, v2, v3, &vm0, &vm1, &vm2, &vm3);
	vn = CFixVector::Normal(gameData.segs.vertices [vm0], gameData.segs.vertices [vm1], gameData.segs.vertices [vm2]);
	xDistToPlane = abs(gameData.segs.vertices [vm3].DistToPlane (vn, gameData.segs.vertices [vm0]));
	if (bFlip)
		vn.Neg();
#if 1
	if (bRenderQuads || (xDistToPlane <= PLANE_DIST_TOLERANCE))
		AddSideAsQuad (segP, nSide, &vn);
	else {
		AddSideAsTwoTriangles (segP, nSide);
		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.
		{
			int			nFaces;
			fix			dist0, dist1;
			int			s0, s1;
			int			nVertex;
			CSide			*s;

			nFaces = CreateAbsVertexLists (vertexList, SEG_IDX (segP), nSide);
#if DBG
			if (nFaces != 2)
				nFaces = CreateAbsVertexLists (vertexList, SEG_IDX (segP), nSide);
#endif
			Assert (nFaces == 2);
			s = segP->m_sides + nSide;
			nVertex = min(vertexList [0], vertexList [2]);
			{
			dist0 = gameData.segs.vertices [vertexList[1]].DistToPlane (s->m_normals [1], gameData.segs.vertices [nVertex]);
			dist1 = gameData.segs.vertices [vertexList[4]].DistToPlane (s->m_normals [0], gameData.segs.vertices [nVertex]);
			}
			s0 = sign (dist0);
			s1 = sign (dist1);
			if (s0 == 0 || s1 == 0 || s0 != s1) {
				segP->m_sides [nSide].nType = SIDE_IS_QUAD; 	//detriangulate!
			}
		}
	}
#endif
if (segP->m_sides [nSide].nType == SIDE_IS_QUAD) {
	AddToVertexNormal (v0, &vn);
	AddToVertexNormal (v1, &vn);
	AddToVertexNormal (v2, &vn);
	AddToVertexNormal (v3, &vn);
	}
else {
	vn = segP->m_sides [nSide].m_normals [0];
	for (i = 0; i < 3; i++)
		AddToVertexNormal (vertexList [i], &vn);
	vn = segP->m_sides [nSide].m_normals [1];
	for (; i < 6; i++)
		AddToVertexNormal (vertexList [i], &vn);
	}
}

// -------------------------------------------------------------------------------

void ValidateRemovableWall (CSegment *segP, int nSide, int nTexture)
{
CreateWallsOnSide (segP, nSide);
segP->m_sides [nSide].m_nBaseTex = nTexture;
//	assign_default_uvs_to_side (segP, nSide);
//	assign_light_to_side (segP, nSide);
}

// -------------------------------------------------------------------------------
//	Make a just-modified CSegment CSide valid.
void ValidateSegmentSide (CSegment *segP, short nSide)
{
if (IS_WALL (WallNumP (segP, nSide)))
	ValidateRemovableWall (segP, nSide, segP->m_sides [nSide].m_nBaseTex);
else
	CreateWallsOnSide (segP, nSide);
}

// -------------------------------------------------------------------------------

void ComputeVertexNormals (void)
{
	int		h, i;
	g3sPoint	*pp;

for (i = gameData.segs.nVertices, pp = gameData.segs.points.Buffer (); i; i--, pp++) {
	if (1 < (h = pp->p3_normal.nFaces)) {
		pp->p3_normal.vNormal /= (float) h;
		/*
		pp->p3_normal.vNormal[Y] /= h;
		pp->p3_normal.vNormal[Z] /= h;
		*/
		CFloatVector::Normalize (pp->p3_normal.vNormal);
		}
	pp->p3_normal.nFaces = 1;
	}
}

// -------------------------------------------------------------------------------

float FaceSize (short nSegment, ubyte nSide)
{
	CSegment		*segP = SEGMENTS + nSegment;
	sbyte			*s2v = sideVertIndex [nSide];

	short			v0 = segP->verts [s2v [0]];
	short			v1 = segP->verts [s2v [1]];
	short			v2 = segP->verts [s2v [2]];
	short			v3 = segP->verts [s2v [3]];

return TriangleSize (gameData.segs.vertices [v0], gameData.segs.vertices [v1], gameData.segs.vertices [v2]) +
		 TriangleSize (gameData.segs.vertices [v0], gameData.segs.vertices [v2], gameData.segs.vertices [v3]);
}

// -------------------------------------------------------------------------------

void ResetVertexNormals (void)
{
	int		i;
	g3sPoint	*pp;

for (i = gameData.segs.nVertices, pp = gameData.segs.points.Buffer (); i; i--, pp++) {
	pp->p3_normal.vNormal.SetZero();
	pp->p3_normal.nFaces = 0;
	}
}

// -------------------------------------------------------------------------------
//	Make a just-modified CSegment valid.
//		check all sides to see how many faces they each should have (0, 1, 2)
//		create new vector normals
void ValidateSegment (CSegment *segP)
{
	short	CSide;

#ifdef EDITOR
check_for_degenerate_segment (segP);
#endif
for (CSide = 0; CSide < MAX_SIDES_PER_SEGMENT; CSide++)
	ValidateSegmentSide (segP, CSide);
}

// -------------------------------------------------------------------------------
//	Validate all segments.
//	gameData.segs.nLastSegment must be set.
//	For all used segments (number <= gameData.segs.nLastSegment), nSegment field must be != -1.

void ValidateSegmentAll (void)
{
	int	s;

gameOpts->render.nMathFormat = 0;
gameData.segs.points.Clear ();
for (s = 0; s <= gameData.segs.nLastSegment; s++)
#ifdef EDITOR
	if (SEGMENTS [s].nSegment != -1)
#endif
		ValidateSegment (SEGMENTS + s);
#ifdef EDITOR
	{
	int said = 0;
	for (s = gameData.segs.nLastSegment + 1; s < MAX_SEGMENTS; s++)
		if (SEGMENTS [s].nSegment != -1) {
			if (!said) {
#if TRACE
				console.printf (CON_DBG, "Segment %i has invalid nSegment.  Bashing to -1.  Silently bashing all others...", s);
#endif
				}
			said++;
			SEGMENTS [s].nSegment = -1;
			}
	if (said) {
#if TRACE
		console.printf (CON_DBG, "%i fixed.\n", said);
#endif
		}
	}
#endif

#if DBG
#	ifndef COMPACT_SEGS
if (CheckSegmentConnections ())
	Int3 ();		//Get Matt, si vous plait.
#	endif
#endif
ComputeVertexNormals ();
gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
}


//	------------------------------------------------------------------------------------------------------
//	Picks a Random point in a CSegment like so:
//		From center, go up to 50% of way towards any of the 8 vertices.
void PickRandomPointInSeg (CFixVector *new_pos, int nSegment)
{
	int			vnum;
	CFixVector	vec2;

	COMPUTE_SEGMENT_CENTER_I (new_pos, nSegment);
	vnum = (d_rand () * MAX_VERTICES_PER_SEGMENT) >> 15;
	vec2 = gameData.segs.vertices [SEGMENTS[nSegment].verts [vnum]] - *new_pos;
	vec2 *= (d_rand ());          // d_rand () always in 0..1/2
	*new_pos += vec2;
}


//	----------------------------------------------------------------------------------------------------------
//	Set the segment depth of all segments from nStartSeg in *segbuf.
//	Returns maximum nDepth value.
int SetSegmentDepths (int nStartSeg, ushort *pDepthBuf)
{
	ubyte		bVisited [MAX_SEGMENTS_D2X];
	short		queue [MAX_SEGMENTS_D2X];
	int		head = 0;
	int		tail = 0;
	int		nDepth = 1;
	int		nSegment, nSide, nChild;
	ushort	nParentDepth = 0;
	short		*childP;

	head = 0;
	tail = 0;

if ((nStartSeg < 0) || (nStartSeg >= gameData.segs.nSegments))
	return 1;
if (pDepthBuf [nStartSeg] == 0)
	return 1;
queue [tail++] = nStartSeg;
memset (bVisited, 0, sizeof (*bVisited) * gameData.segs.nSegments);
bVisited [nStartSeg] = 1;
pDepthBuf [nStartSeg] = nDepth++;
if (nDepth == 0)
	nDepth = 0x7fff;
while (head < tail) {
	nSegment = queue [head++];
#if DBG
	if (nSegment == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	nParentDepth = pDepthBuf [nSegment];
	childP = SEGMENTS [nSegment].children;
	for (nSide = MAX_SIDES_PER_SEGMENT; nSide; nSide--, childP++) {
		if (0 > (nChild = *childP))
			continue;
#if DBG
		if (nChild >= gameData.segs.nSegments) {
			Error ("Invalid segment in SetSegmentDepths()\nsegment=%d, side=%d, child=%d",
					 nSegment, nSide, nChild);
			return 1;
			}
#endif
#if DBG
		if (nChild == nDbgSeg)
			nDbgSeg = nDbgSeg;
#endif
		if (!pDepthBuf [nChild])
			continue;
		if (bVisited [nChild])
			continue;
		bVisited [nChild] = 1;
		pDepthBuf [nChild] = nParentDepth + 1;
		queue [tail++] = nChild;
		}
	}
return (nParentDepth + 1) * gameStates.render.bViewDist;
}


//	-----------------------------------------------------------------------------

fix FindConnectedDistanceSegments (short seg0, short seg1, int nDepth, int widFlag)
{
	CFixVector	p0, p1;

COMPUTE_SEGMENT_CENTER_I (&p0, seg0);
COMPUTE_SEGMENT_CENTER_I (&p1, seg1);
return FindConnectedDistance (&p0, seg0, &p1, seg1, nDepth, widFlag, 0);
}

#define	AMBIENT_SEGMENT_DEPTH		5

//	-----------------------------------------------------------------------------
//	Do a bfs from nSegment, marking slots in marked_segs if the CSegment is reachable.
void AmbientMarkBfs (short nSegment, sbyte* markedSegs, int nDepth)
{
	short	i, child;

if (nDepth < 0)
	return;
marked_segs [nSegment] = 1;
for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
	child = SEGMENTS [nSegment].children [i];
	if (IS_CHILD (child) && (SEGMENTS [nSegment].IsDoorWay (i, NULL) & WID_RENDPAST_FLAG) && !markedSegs [child])
		AmbientMarkBfs (child, markedSegs, nDepth - 1);
	}
}

//	-----------------------------------------------------------------------------
//	Indicate all segments which are within audible range of falling water or lava,
//	and so should hear ambient gurgles.
void SetAmbientSoundFlagsCommon (int tmi_bit, int s2f_bit)
{
	short		i, j;
	tSegment2	*seg2p;
	static sbyte   marked_segs [MAX_SEGMENTS_D2X];

	//	Now, all segments containing ambient lava or water sound makers are flagged.
	//	Additionally flag all segments which are within range of them.
for (i=0; i<=gameData.segs.nLastSegment; i++) {
	marked_segs [i] = 0;
	gameData.segs.segment2s [i].s2Flags &= ~s2f_bit;
	}

//	Mark all segments which are sources of the sound.
for (i=0; i<=gameData.segs.nLastSegment; i++) {
	CSegment	*segp = &SEGMENTS [i];
	tSegment2	*seg2p = &gameData.segs.segment2s [i];

	for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
		CSide	*sideP = &segp->m_sides [j];

		if ((gameData.pig.tex.tMapInfoP [sideP->nBaseTex].flags & tmi_bit) ||
			   (gameData.pig.tex.tMapInfoP [sideP->nOvlTex].flags & tmi_bit)) {
			if (!IS_CHILD (segp->children [j]) || IS_WALL (sideP->nWall)) {
				seg2p->s2Flags |= s2f_bit;
				marked_segs [i] = 1;		//	Say it's itself that it is close enough to to hear something.
				}
			}
		}
	}
//	Next mark all segments within N segments of a source.
for (i=0; i<=gameData.segs.nLastSegment; i++) {
	seg2p = &gameData.segs.segment2s [i];
	if (seg2p->s2Flags & s2f_bit)
		AmbientMarkBfs (i, marked_segs, AMBIENT_SEGMENT_DEPTH);
	}
//	Now, flip bits in all segments which can hear the ambient sound.
for (i=0; i<=gameData.segs.nLastSegment; i++)
	if (marked_segs [i])
		gameData.segs.segment2s [i].s2Flags |= s2f_bit;
}

//	-----------------------------------------------------------------------------
//	Indicate all segments which are within audible range of falling water or lava,
//	and so should hear ambient gurgles.
//	Bashes values in gameData.segs.segment2s array.
void SetAmbientSoundFlags (void)
{
SetAmbientSoundFlagsCommon (TMI_VOLATILE, S2F_AMBIENT_LAVA);
SetAmbientSoundFlagsCommon (TMI_WATER, S2F_AMBIENT_WATER);
}

//	-----------------------------------------------------------------------------
//eof
