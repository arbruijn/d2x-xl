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

// -------------------------------------------------------------------------------
//this was converted from GetSegMasks ()...it fills in an array of 6
//elements for the distace behind each CSide, or zero if not behind
//only gets centerMask, and assumes zero rad

ubyte CSegment::GetSideDists (const CFixVector& refP, fix* xSideDists, int bBehind)
{
	ubyte		mask = 0;

for (int nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
	if (m_sides [nSide].FaceCount ())
		mask |= m_sides [nSide].Dist (refP, xSideDists [nSide], bBehind, 1 << nSide);
return mask;
}

// -------------------------------------------------------------------------------

ubyte CSegment::GetSideDistsf (const CFloatVector& refP, float* fSideDists, int bBehind)
{
	ubyte		mask = 0;

for (int nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
	if (m_sides [nSide].FaceCount ())
		mask |= m_sides [nSide].Distf (refP, fSideDists [nSide], bBehind, 1 << nSide);
return mask;
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
//figure out what seg the given point is in, tracing through segments
//returns CSegment number, or -1 if can't find segment

static int TraceSegs (const CFixVector& vPos, int nCurSeg, int nTraceDepth, ushort* bVisited, ushort bFlag, fix xTolerance = 0)
{
	CSegment*	segP;
	fix			xSideDists [6], xMaxDist;
	int			centerMask, nMaxSide, nSide, bit, nMatchSeg = -1;

if (nTraceDepth >= gameData.segs.nSegments) //gameData.segs.nSegments)
	return -1;
if (bVisited [nCurSeg] == bFlag)
	return -1;
bVisited [nCurSeg] = bFlag;
segP = SEGMENTS + nCurSeg;
if (!(centerMask = segP->GetSideDists (vPos, xSideDists, 1)))		//we're in the old segment
	return nCurSeg;		
for (;;) {
	nMaxSide = -1;
	xMaxDist = 0; // find only sides we're behind as seen from inside the current segment
	for (nSide = 0, bit = 1; nSide < SEGMENT_SIDE_COUNT; nSide++, bit <<= 1)
		if (segP->Side (nSide)->FaceCount () && (centerMask & bit) && (xTolerance || (segP->m_children [nSide] > -1)) && (xSideDists [nSide] < xMaxDist)) {
			if (xTolerance && (xTolerance >= -xSideDists [nSide]) && (xTolerance >= segP->Side (nSide)->DistToPoint (vPos))) 
				return nCurSeg;
			if (segP->m_children [nSide] >= 0) {
				xMaxDist = xSideDists [nSide];
				nMaxSide = nSide;
				}
			}
	if (nMaxSide == -1)
		break;
	xSideDists [nMaxSide] = 0;
	if (0 <= (nMatchSeg = TraceSegs (vPos, segP->m_children [nMaxSide], nTraceDepth + 1, bVisited, bFlag, xTolerance)))	//trace into adjacent CSegment
		break;
	}
return nMatchSeg;		//we haven't found a CSegment
}

// -------------------------------------------------------------------------------

static int TraceSegsf (const CFloatVector& vPos, int nCurSeg, int nTraceDepth, ushort* bVisited, ushort bFlag, float fTolerance)
{
	CSegment*		segP;
	float				fSideDists [6], fMaxDist;
	int				centerMask, nMaxSide, nSide, bit, nMatchSeg = -1;

if (nTraceDepth >= gameData.segs.nSegments)
	return -1;
if (bVisited [nCurSeg] == bFlag)
	return -1;
bVisited [nCurSeg] = bFlag;
segP = SEGMENTS + nCurSeg;
if (!(centerMask = segP->GetSideDistsf (vPos, fSideDists, 1)))		//we're in the old CSegment
	return nCurSeg;		
for (;;) {
	nMaxSide = -1;
	fMaxDist = 0; // find only sides we're behind as seen from inside the current segment
	for (nSide = 0, bit = 1; nSide < SEGMENT_SIDE_COUNT; nSide++, bit <<= 1)
		if (segP->Side (nSide)->FaceCount () && (centerMask & bit) && (fSideDists [nSide] < fMaxDist)) {
			if ((fTolerance >= -fSideDists [nSide])  && (fTolerance >= segP->Side (nSide)->DistToPointf (vPos))) {
#if DBG
				SEGMENTS [nCurSeg].GetSideDistsf (vPos, fSideDists, 1);
#endif
				return nCurSeg;
				}
			if (segP->m_children [nSide] >= 0) {
				fMaxDist = fSideDists [nSide];
				nMaxSide = nSide;
				}
			}
	if (nMaxSide == -1)
		break;
	fSideDists [nMaxSide] = 0;
	if (0 <= (nMatchSeg = TraceSegsf (vPos, segP->m_children [nMaxSide], nTraceDepth + 1, bVisited, bFlag, fTolerance)))	//trace into adjacent CSegment
		break;
	}
return nMatchSeg;		//we haven't found a segment
}

// -------------------------------------------------------------------------------

static inline int PointInSeg (CSegment* segP, CFixVector vPos)
{
fix d = CFixVector::Dist (vPos, segP->Center ());
if (d <= segP->m_rads [0])
	return 1;
if (d > segP->m_rads [1])
	return 0;
return (segP->Masks (vPos, 0).m_center == 0);
}

// -------------------------------------------------------------------------------

int FindSegByPosExhaustive (const CFixVector& vPos, int bSkyBox, int nStartSeg)
{
if ((nStartSeg >= 0) && (PointInSeg (SEGMENTS + nStartSeg, vPos)))
	return nStartSeg;

	int			i;
	short*		segListP = NULL;
	CSegment*	segP;

if (gameData.segs.HaveGrid (bSkyBox)) {
	for (i = gameData.segs.GetSegList (vPos, segListP, bSkyBox); i; i--, segListP++) {
#if DBG
		if ((*segListP < 0) || (*segListP >= gameData.segs.nSegments))
			continue;
#endif
		if (PointInSeg (SEGMENTS + *segListP, vPos))
			return *segListP;
		}
	}
else if (bSkyBox) {
	for (i = gameData.segs.skybox.GetSegList (segListP); i; i--, segListP++) {
		segP = SEGMENTS + *segListP;
		if ((segP->m_function == SEGMENT_FUNC_SKYBOX) && PointInSeg (segP, vPos))
			return *segListP;
		}
	}
else {
	segP = SEGMENTS.Buffer ();
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		segP = SEGMENTS + i;
		if ((segP->m_function != SEGMENT_FUNC_SKYBOX) && PointInSeg (segP, vPos))
			return i;
		}
	}
return -1;
}

// -------------------------------------------------------------------------------
//Find segment containing point vPos.

int FindSegByPos (const CFixVector& vPos, int nStartSeg, int bExhaustive, int bSkyBox, fix xTolerance, int nThread)
{
	static ushort bVisited [MAX_THREADS][MAX_SEGMENTS_D2X]; 
	static ushort bFlags [MAX_THREADS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
	int nSegment = -1;

if (nStartSeg >= 0) {
	if (PointInSeg (SEGMENTS + nStartSeg, vPos))
		return nStartSeg;
	if (!xTolerance && bExhaustive && gameData.segs.HaveGrid (bSkyBox)) 
		return FindSegByPosExhaustive (vPos, SEGMENTS [nStartSeg].m_function == SEGMENT_FUNC_SKYBOX, nStartSeg);
	if (SEGMENTS [nStartSeg].m_function == SEGMENT_FUNC_SKYBOX) {
		if (0 <= (nSegment = TraceSegs (vPos, nStartSeg, 1, bVisited [nThread], bFlags [nThread], xTolerance))) 
			return nSegment;
		}
	if (!++bFlags [nThread]) {
		memset (bVisited [nThread], 0, sizeofa (bVisited [nThread]));
		bFlags [nThread] = 1;
		}
	if (0 <= (nSegment = TraceSegs (vPos, nStartSeg, 0, bVisited [nThread], bFlags [nThread], xTolerance))) 
		return nSegment;
	}

//couldn't find via attached segs, so search all segs
if (!bExhaustive)
	return -1;

if (bSkyBox < 0) {
	if (0 <= (nSegment = FindSegByPosExhaustive (vPos, 0)))
		return nSegment;
	bSkyBox = 1;
	}
return FindSegByPosExhaustive (vPos, bSkyBox);
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

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	Do a bfs from nSegment, marking slots in markedSegs if the segment is reachable.

#define	AMBIENT_SEGMENT_DEPTH 5

void AmbientMarkBfs (short nSegment, sbyte* markedSegs, int nDepth)
{
	short	i, child;

if (nDepth < 0)
	return;
markedSegs [nSegment] = 1;
CSegment* segP = SEGMENTS + nSegment;
for (i = 0; i < SEGMENT_SIDE_COUNT; i++) {
	child = segP->m_children [i];
	if (IS_CHILD (child) && (segP->IsDoorWay (i, NULL) & WID_TRANSPARENT_FLAG) && !markedSegs [child])
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
	for (j = 0; j < SEGMENT_SIDE_COUNT; j++, sideP++) {
		if (!sideP->FaceCount ())
			continue;
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

//	-----------------------------------------------------------------------------
//eof
