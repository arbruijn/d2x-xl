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

// -----------------------------------------------------------------------------------
// Fill in array with four absolute point numbers for a given CSide
void GetCorners (int nSegment, int nSide, ushort* vertIndex)
{
	int*		sv = sideVertIndex [nSide];
	ushort*	vp = SEGMENTS [nSegment].m_verts;

vertIndex [0] = vp [sv [0]];
vertIndex [1] = vp [sv [1]];
vertIndex [2] = vp [sv [2]];
vertIndex [3] = vp [sv [3]];
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

switch (sideP->m_nType) {
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
		Error ("Illegal CSide nType (1), nType = %i, CSegment # = %i, CSide # = %i\n", sideP->m_nType, nSegment, nSide);
		break;
	}
}
#endif

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
//this was converted from GetSegMasks ()...it fills in an array of 6
//elements for the distace behind each CSide, or zero if not behind
//only gets centerMask, and assumes zero rad
ubyte CSegment::GetSideDists (const CFixVector& refP, fix* xSideDists, int bBehind)
{
	ubyte		mask = 0;

for (int nSide = 0; nSide < 6; nSide++)
	mask |= m_sides [nSide].Dist (refP, xSideDists [nSide], bBehind, 1 << nSide);
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
	fix				xSideDists [6], xMaxDist;
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
	short		*segNumP;
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
	for (i = gameData.segs.skybox.ToS (), segNumP = gameData.segs.skybox.Buffer (); i; i--, segNumP++)
		if (!SEGMENTS [*segNumP].Masks (p, 0).m_center)
			goto funcExit;
	}
else {
	for (nNewSeg = 0; nNewSeg <= gameData.segs.nLastSegment; nNewSeg++)
		if ((SEGMENTS [nNewSeg].m_nType != SEGMENT_IS_SKYBOX) && !SEGMENTS [nNewSeg].Masks (p, 0).m_center)
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
	CSegment*	segP = SEGMENTS + 0;
	short			nSegment, nClosestSeg = -1;
	fix			nDist, nMinDist = 0x7fffffff;

for (nSegment = 0; nSegment < gameData.segs.nSegments; nSegment++, segP++) {
	nDist = CFixVector::Dist (vPos, segP->Center ()) - segP->MinRad ();
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
//--repair-- 		int	s = SEGMENTS [nSegment].m_children [nSide];
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
fix FindConnectedDistance (CFixVector& p0, short seg0, CFixVector& p1, short seg1, int nMaxDepth, int widFlag, int bUseCache)
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
	return CFixVector::Dist (p0, p1);
	}
nConnSide = SEGMENTS [seg0].ConnectedSide (SEGMENTS + seg1);
if ((nConnSide != -1) &&
	 (SEGMENTS [seg1].IsDoorWay (nConnSide, NULL) & widFlag)) {
	gameData.fcd.nConnSegDist = 1;
	return CFixVector::Dist (p0, p1);
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
	pointSegs [nPoints].point = SEGMENTS [nThisSeg].Center ();
	nPoints++;
	if (nParentSeg == seg0)
		break;
	while (segmentQ [--qTail].end != nParentSeg)
		Assert (qTail >= 0);
	}
pointSegs [nPoints].nSegment = seg0;
pointSegs [nPoints].point = SEGMENTS [seg0].Center ();
nPoints++;
if (nPoints == 1) {
	gameData.fcd.nConnSegDist = nPoints;
	return CFixVector::Dist (p0, p1);
	}
else {
	fix	ndist;
	dist = CFixVector::Dist (p1, pointSegs [1].point);
	dist += CFixVector::Dist (p0, pointSegs [nPoints-2].point);
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

	pv = gameData.segs.vertices + SEGMENTS [objP->info.nSegment].m_verts [0];
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

	pv = gameData.segs.vertices + SEGMENTS [nSegment].m_verts [0];
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
		vs += gameData.segs.vertices [segP->m_verts [sideVertIndex [start][i]]];
		ve += gameData.segs.vertices [segP->m_verts [sideVertIndex [end][i]]];
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
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *bFlip set, then negate Normal after computation.
//	Note, pos [Y]u cannot just compute the Normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
int GetVertsForNormal (int v0, int v1, int v2, int v3, int *pv0, int *pv1, int *pv2, int *pv3)
{
	int		i, j, t;
	ushort	v [4], w [4] = {0, 1, 2, 3};

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

void AddToVertexNormal (int nVertex, CFixVector& vNormal)
{
	g3sNormal	*pn = &gameData.segs.points [nVertex].p3_normal;

#if DBG
if (nVertex == nDbgVertex)
	nDbgVertex = nDbgVertex;
#endif
pn->nFaces++;
pn->vNormal += vNormal;
}

// -------------------------------------------------------------------------------
//	Set up all segments.
//	gameData.segs.nLastSegment must be set.
//	For all used segments (number <= gameData.segs.nLastSegment), nSegment field must be != -1.

void SetupSegments (void)
{
gameOpts->render.nMathFormat = 0;
gameData.segs.points.Clear ();
for (int i = 0; i <= gameData.segs.nLastSegment; i++)
#ifdef EDITOR
	if (SEGMENTS [s].nSegment != -1)
#endif
	SEGMENTS [i].Setup ();
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

ComputeVertexNormals ();
gameOpts->render.nMathFormat = gameOpts->render.nDefMathFormat;
}



//	-----------------------------------------------------------------------------
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
	ushort*	childP;

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
	childP = SEGMENTS [nSegment].m_children;
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

p0 = SEGMENTS [seg0].Center ();
p1 = SEGMENTS [seg1].Center ();
return FindConnectedDistance (p0, seg0, p1, seg1, nDepth, widFlag, 0);
}

#define	AMBIENT_SEGMENT_DEPTH		5

//	-----------------------------------------------------------------------------
//	Do a bfs from nSegment, marking slots in markedSegs if the segment is reachable.
void AmbientMarkBfs (short nSegment, sbyte* markedSegs, int nDepth)
{
	short	i, child;

if (nDepth < 0)
	return;
markedSegs [nSegment] = 1;
for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	child = SEGMENTS [nSegment].m_children [i];
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

	static sbyte	markedSegs [MAX_SEGMENTS_D2X];

	//	Now, all segments containing ambient lava or water sound makers are flagged.
	//	Additionally flag all segments which are within range of them.
memset (markedSegs, 0, sizeof (markedSegs));
for (i = 0; i <= gameData.segs.nLastSegment; i++) {
	SEGMENTS [i].m_flags &= ~s2f_bit;
	}

//	Mark all segments which are sources of the sound.
CSegment	*segP = SEGMENTS.Buffer ();
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	CSide	*sideP = segP->m_sides;
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++, sideP++) {
		if ((gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & tmi_bit) ||
			 (gameData.pig.tex.tMapInfoP [sideP->m_nOvlTex].flags & tmi_bit)) {
			if (!IS_CHILD (segP->m_children [j]) || IS_WALL (sideP->m_nWall)) {
				segP->m_flags |= s2f_bit;
				markedSegs [i] = 1;		//	Say it's itself that it is close enough to to hear something.
				}
			}
		}
	}
//	Next mark all segments within N segments of a source.
segP = SEGMENTS.Buffer ();
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	if (segP->m_flags & s2f_bit)
		AmbientMarkBfs (i, markedSegs, AMBIENT_SEGMENT_DEPTH);
	}
//	Now, flip bits in all segments which can hear the ambient sound.
for (i = 0; i <= gameData.segs.nLastSegment; i++)
	if (markedSegs [i])
		SEGMENTS [i].m_flags |= s2f_bit;
}

//	-----------------------------------------------------------------------------
//	Indicate all segments which are within audible range of falling water or lava,
//	and so should hear ambient gurgles.

void SetAmbientSoundFlags (void)
{
SetAmbientSoundFlagsCommon (TMI_VOLATILE, S2F_AMBIENT_LAVA);
SetAmbientSoundFlagsCommon (TMI_WATER, S2F_AMBIENT_WATER);
}

// -------------------------------------------------------------------------------

float FaceSize (short nSegment, ubyte nSide)
{
	CSegment*	segP = SEGMENTS + nSegment;
	int*			s2v = sideVertIndex [nSide];

	short			v0 = segP->m_verts [s2v [0]];
	short			v1 = segP->m_verts [s2v [1]];
	short			v2 = segP->m_verts [s2v [2]];
	short			v3 = segP->m_verts [s2v [3]];

return TriangleSize (gameData.segs.vertices [v0], gameData.segs.vertices [v1], gameData.segs.vertices [v2]) +
		 TriangleSize (gameData.segs.vertices [v0], gameData.segs.vertices [v2], gameData.segs.vertices [v3]);
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

void ResetVertexNormals (void)
{
	int		i;
	g3sPoint	*pp;

for (i = gameData.segs.nVertices, pp = gameData.segs.points.Buffer (); i; i--, pp++) {
	pp->p3_normal.vNormal.SetZero();
	pp->p3_normal.nFaces = 0;
	}
}

//	-----------------------------------------------------------------------------
//eof
