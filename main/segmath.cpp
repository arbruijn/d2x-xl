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

#define USE_DACS 0
#define BIDIRECTIONAL_SCAN 1
#if DBG
#	define DBG_SCAN 0
#else
#	define DBG_SCAN 0
#endif

#if USE_DACS
#	include "dialheap.h"
#endif

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------

typedef struct {
	short	seg0, seg1;
	int	pathLen;
	fix	dist;
} tFCDCacheData;

#define	MAX_FCD_CACHE	128

class CFCDCache {
	public:	
		int				m_nIndex;
		CStaticArray< tFCDCacheData, MAX_FCD_CACHE >	m_cache; // [MAX_FCD_CACHE];
		fix				m_xLastFlushTime;
		fix				m_nPathLength;

		void Flush (void);
		void Add (int seg0, int seg1, int nDepth, fix dist);
		fix Dist (short seg0, short seg1);
		inline void SetPathLength (fix nPathLength) { m_nPathLength = nPathLength; }
		inline fix GetPathLength (void) { return m_nPathLength; }
};

CFCDCache fcdCaches [2];

// -----------------------------------------------------------------------------------

#define	MIN_CACHE_FCD_DIST	 (I2X (80))	//	Must be this far apart for cache lookup to succeed.  Recognizes small changes in distance matter at small distances.

void CFCDCache::Flush (void)
{
m_nIndex = 0;
for (int i = 0; i < MAX_FCD_CACHE; i++)
	m_cache [i].seg0 = -1;
}

// -----------------------------------------------------------------------------------

void CFCDCache::Add (int seg0, int seg1, int nPathLen, fix dist)
{
if (dist > MIN_CACHE_FCD_DIST) {
	m_cache [m_nIndex].seg0 = seg0;
	m_cache [m_nIndex].seg1 = seg1;
	m_cache [m_nIndex].pathLen = nPathLen;
	m_cache [m_nIndex].dist = dist;
	if (++m_nIndex >= MAX_FCD_CACHE)
		m_nIndex = 0;
	SetPathLength (nPathLen);
	}
else {
	//	If it's in the cache, remove it.
	for (int i = 0; i < MAX_FCD_CACHE; i++) {
		if ((m_cache [i].seg0 == seg0) && (m_cache [i].seg1 == seg1)) {
			m_cache [m_nIndex].seg0 = -1;
			break;
			}
		}
	}
}

// -----------------------------------------------------------------------------------

fix CFCDCache::Dist (short seg0, short seg1)
{
	tFCDCacheData*	pc = m_cache.Buffer ();

for (int i = int (m_cache.Length ()); i; i--, pc++) {
	if ((pc->seg0 == seg0) && (pc->seg1 == seg1)) {
		SetPathLength (pc->pathLen);
		return pc->dist;
		}
	}
return -1;
}

// -----------------------------------------------------------------------------------

void FlushFCDCache (void)
{
fcdCaches [0].Flush ();
fcdCaches [1].Flush ();
}

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// Fill in array with four absolute point numbers for a given CSide
void GetCorners (int nSegment, int nSide, ushort* vertIndex)
{
	int*		sv = sideVertIndex [nSide];
	short*	vp = SEGMENTS [nSegment].m_verts;

vertIndex [0] = vp [sv [0]];
vertIndex [1] = vp [sv [1]];
vertIndex [2] = vp [sv [2]];
vertIndex [3] = vp [sv [3]];
}

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

ubyte CSegment::GetSideDistsf (const CFloatVector& refP, float* fSideDists, int bBehind)
{
	ubyte		mask = 0;

for (int nSide = 0; nSide < 6; nSide++)
	mask |= m_sides [nSide].Distf (refP, fSideDists [nSide], bBehind, 1 << nSide);
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
	for (nSide = 0, bit = 1; nSide < 6; nSide ++, bit <<= 1)
		if ((centerMask & bit) && (xTolerance || (segP->m_children [nSide] > -1)) && (xSideDists [nSide] < xMaxDist)) {
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
	for (nSide = 0, bit = 1; nSide < 6; nSide ++, bit <<= 1)
		if ((centerMask & bit) && (fSideDists [nSide] < fMaxDist)) {
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
return nMatchSeg;		//we haven't found a CSegment
}

int	nExhaustiveCount=0, nExhaustiveFailedCount=0;

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

static int FindSegByPosExhaustive (const CFixVector& vPos, int bSkyBox)
{
	int			i;
	short*		segListP;
	CSegment*	segP;

if (gameData.segs.HaveGrid (bSkyBox)) {
	for (i = gameData.segs.GetSegList (vPos, segListP, bSkyBox); i; i--, segListP++) {
		if (PointInSeg (SEGMENTS + *segListP, vPos))
			return *segListP;
		}
	}
else if (bSkyBox) {
	for (i = gameData.segs.skybox.GetSegList (segListP); i; i--, segListP++) {
		segP = SEGMENTS + *segListP;
		if ((segP->m_nType == SEGMENT_IS_SKYBOX) && PointInSeg (segP, vPos))
			return *segListP;
		}
	}
else {
	segP = SEGMENTS.Buffer ();
	for (i = 0; i <= gameData.segs.nLastSegment; i++) {
		segP = SEGMENTS + i;
		if ((segP->m_nType != SEGMENT_IS_SKYBOX) && PointInSeg (segP, vPos))
			return i;
		}
	}
return -1;
}

// -------------------------------------------------------------------------------
//Find segment containing point vPos.

int FindSegByPos (const CFixVector& vPos, int nSegment, int bExhaustive, int bSkyBox, fix xTolerance, int nThread)
{
	static ushort bVisited [MAX_THREADS][MAX_SEGMENTS_D2X]; 
	static ushort bFlags [MAX_THREADS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

//allow nSegment == -1, meaning we have no idea what CSegment point is in
Assert ((nSegment <= gameData.segs.nLastSegment) && (nSegment >= -1));
if (nSegment != -1) {
	if (!++bFlags [nThread]) {
		memset (bVisited [nThread], 0, sizeofa (bVisited [nThread]));
		bFlags [nThread] = 1;
		}
	if (0 <= (nSegment = TraceSegs (vPos, nSegment, 0, bVisited [nThread], bFlags [nThread], xTolerance))) 
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

//	----------------------------------------------------------------------------------------------------------

# if BIDIRECTIONAL_DACS

static int Expand (int nDir, short nDestSeg, int widFlag)
{
ushort nDist;
short nSegment = dialHeaps [nDir].Pop (nDist);
if ((nSegment < 0) || (nSegment == nDestSeg))
	return nSegment;
if (dialHeaps [!nDir].Popped (nSegment))
	return nSegment;
CSegment* segP = SEGMENTS + nSegment;
for (short nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	if ((segP->m_children [nSide] >= 0) && (segP->IsDoorWay (nSide, NULL) & widFlag))
		dialHeaps [nDir].Push (segP->m_children [nSide], nSegment, nDist + segP->m_childDists [nSide]);
	}
return nSegment;
}

#endif

//	-----------------------------------------------------------------------------

typedef struct tSegPathNode {
	uint		bVisited;
	short		nDepth;
	short		nPred;
} tSegPathNode;

typedef struct tSegScanInfo {
	uint	bFlag;
	int	nMaxDepth;
	int	widFlag;
	short bScanning;
	short	nLinkSeg;
} tSegScanInfo;

typedef struct tSegScanData {
	tSegPathNode segPath [MAX_SEGMENTS_D2X];
	short segQueue [MAX_SEGMENTS_D2X];
	uint	bFlag;
	short	nStartSeg;
	short nDestSeg;
	short nLinkSeg;
	short nHead;
	short nTail;
	int	nRouteDir;
} tSegScanData;

static tSegScanInfo scanInfo = {0xFFFFFFFF, 0, 0, 3, -1};
static tSegScanData scanData [2];
static SDL_mutex* semaphore = NULL;

//	-----------------------------------------------------------------------------

static short ExpandSegment (int nDir)
{
	tSegScanData& sd = scanData [nDir];

if (sd.nTail >= sd.nHead)
	return sd.nLinkSeg = -1;
short nPredSeg = sd.segQueue [sd.nTail++];
#if DBG_SCAN
if (nPredSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
short nDepth = sd.segPath [nPredSeg].nDepth + 1;
if (nDepth > scanInfo.nMaxDepth) {
	scanInfo.bScanning &= ~(1 << nDir);
	return sd.nLinkSeg = (scanInfo.bScanning ? 0 : -1);
	}
CSegment* segP = SEGMENTS + nPredSeg;
for (short nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	short nSuccSeg = segP->m_children [nSide];
	if (nSuccSeg < 0)
		continue;
	if (sd.nRouteDir) {
		CSegment* otherSegP = SEGMENTS + nSuccSeg;
		short nOtherSide = SEGMENTS [nPredSeg].ConnectedSide (otherSegP);
		if ((nOtherSide == -1) || !(otherSegP->IsDoorWay (nOtherSide, NULL) & scanInfo.widFlag))
			continue;
		}
	else {
		if (!(segP->IsDoorWay (nSide, NULL) & scanInfo.widFlag))
			continue;
		}
#if DBG_SCAN
	if (nSuccSeg == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	tSegPathNode* pathNodeP = &sd.segPath [nSuccSeg];
	if (pathNodeP->bVisited == scanInfo.bFlag)
		continue;
	pathNodeP->nPred = nPredSeg;
#if BIDIRECTIONAL_SCAN
	if (scanData [!nDir].segPath [nSuccSeg].bVisited == scanInfo.bFlag)
#else
	if (nSuccSeg == sd.nDestSeg)
#endif
		return sd.nLinkSeg = nSuccSeg + 1;
	pathNodeP->bVisited = scanInfo.bFlag;
	pathNodeP->nDepth = nDepth;
	sd.segQueue [sd.nHead++] = nSuccSeg;
	}
return 0;
}

//	-----------------------------------------------------------------------------

int _CDECL_ ExpandSegmentMT (void* nThreadP)
{
	int nDir = *((int *) nThreadP);

while (!(ExpandSegment (nDir) || scanData [!nDir].nLinkSeg))
	;
return 1;
}

//	-----------------------------------------------------------------------------

static fix BuildPathBiDir (CFixVector& p0, CFixVector& p1, int nCacheType)
{
	fix	xDist = 0;
	int	nLength = 0; 
	short	nPredSeg, nSuccSeg;

--scanInfo.nLinkSeg;
for (int nDir = 0; nDir < 2; nDir++) {
	tSegScanData& sd = scanData [nDir];
	nSuccSeg = scanInfo.nLinkSeg;
	for (;;) {
		nPredSeg = sd.segPath [nSuccSeg].nPred;
		if (nPredSeg == sd.nStartSeg) {
			xDist += CFixVector::Dist (nDir ? p1 : p0, SEGMENTS [nSuccSeg].Center ());
			break;
			}
		nLength++;
		xDist += CFixVector::Dist (SEGMENTS [nPredSeg].Center (), SEGMENTS [nSuccSeg].Center ());
		nSuccSeg = nPredSeg;
		}
	}
if (nCacheType >= 0) 
	fcdCaches [nCacheType].Add (scanData [0].nStartSeg, scanData [0].nDestSeg, nLength + 3, xDist);
return xDist;
}

//	-----------------------------------------------------------------------------

static fix BuildPathUniDir (CFixVector& p0, CFixVector& p1, int nCacheType)
{
	fix	xDist;
	int	nLength = 0; 
	short	nPredSeg, nSuccSeg;

nPredSeg = --scanInfo.nLinkSeg;
xDist = CFixVector::Dist (p1, SEGMENTS [nPredSeg].Center ());
for (;;) {
	nPredSeg = scanData [0].segPath [nSuccSeg].nPred;
	if (nPredSeg == scanData [0].nStartSeg)
		break;
	nLength++;
	xDist += CFixVector::Dist (SEGMENTS [nPredSeg].Center (), SEGMENTS [nSuccSeg].Center ());
	nSuccSeg = nPredSeg;
	}
xDist += CFixVector::Dist (p0, SEGMENTS [nSuccSeg].Center ());
if (nCacheType >= 0) 
	fcdCaches [nCacheType].Add (scanData [0].nStartSeg, scanData [0].nDestSeg, nLength + 3, xDist);
return xDist;
}

//	-----------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum nDepth of nMaxDepth.
//	Return the distance.

fix FindConnectedDistance (CFixVector& p0, short nStartSeg, CFixVector& p1, short nDestSeg, int nMaxDepth, int widFlag, int nCacheType)
{
#if 0 //DBG
gameData.fcd.nConnSegDist = 10000;
return -1;
#endif

// same segment?
if ((nCacheType >= 0) && (nStartSeg == nDestSeg)) {
	fcdCaches [nCacheType].SetPathLength (0);
	return CFixVector::Dist (p0, p1);
	}

// adjacent segments?
short nSide = SEGMENTS [nStartSeg].ConnectedSide (SEGMENTS + nDestSeg);
if ((nSide != -1) && (SEGMENTS [nDestSeg].IsDoorWay (nSide, NULL) & widFlag)) {
	fcdCaches [nCacheType].SetPathLength (1);
	return CFixVector::Dist (p0, p1);
	}

#if !DBG
if (nCacheType >= 0) {
	fix xDist = fcdCaches [nCacheType].Dist (nStartSeg, nDestSeg);
	if (xDist >= 0)
		return xDist;
	}
#endif

#if USE_DACS

	short	nSegment;
#if DBG
	short	nExpanded = 0;
#endif

# if BIDIRECTIONAL_DACS

	short	nSegments [2] = {nStartSeg, nDestSeg};
	static short route [2 * MAX_SEGMENTS_D2X];

dialHeaps [0].Setup (nStartSeg);
dialHeaps [1].Setup (nDestSeg);

for (;;) {
	if (nSegments [0] >= 0) {
		nExpanded++;
		nSegments [0] = Expand (0, nDestSeg, widFlag);
		}
	if (nSegments [1] >= 0) {
		nExpanded++;
		nSegments [1] = Expand (1, nStartSeg, widFlag);
		}
	if ((nSegments [0] < 0) && (nSegments [1] < 0)) {
		gameData.fcd.nConnSegDist = gameData.segs.nSegments + 1;
		return -1;
		}
	if (nSegments [0] == nDestSeg) {
		nSegment = nSegments [0];
		nSegments [1] = -1;
		}
	else if (nSegments [1] == nStartSeg) {
		nSegment = nSegments [1];
		nSegments [0] = -1;
		}
	else if ((nSegments [1] >= 0) && dialHeaps [0].Popped (nSegments [1]))
		nSegment = nSegments [1];
	else if ((nSegments [0] >= 0) && dialHeaps [1].Popped (nSegments [0]))
		nSegment = nSegments [0];
	else
		continue;
	gameData.fcd.nConnSegDist = 0;
	if (nSegments [0] >= 0)
		gameData.fcd.nConnSegDist += dialHeaps [0].BuildRoute (nSegment, 0, route) - 1;
	if (nSegments [1] >= 0)
		gameData.fcd.nConnSegDist += dialHeaps [1].BuildRoute (nSegment, 1, route + gameData.fcd.nConnSegDist);
	int j = gameData.fcd.nConnSegDist - 2;
	xDist = 0;
	for (int i = 1; i < j; i++)
		xDist += CFixVector::Dist (SEGMENTS [route [i]].Center (), SEGMENTS [route [i + 1]].Center ());
	xDist += CFixVector::Dist (p0, SEGMENTS [route [1]].Center ()) + CFixVector::Dist (p1, SEGMENTS [route [j]].Center ());
	return xDist;
	}

#	else

	ushort		nDist;
	CSegment*	segP;

dialHeap.Setup (nStartSeg);
nExpanded = 0;
for (;;) {
	nSegment = dialHeap.Pop (nDist);
	if (nSegment < 0) {
		gameData.fcd.nConnSegDist = gameData.segs.nSegments + 1;
		return -1;
		}
	if (nSegment == nDestSeg) {
		gameData.fcd.nConnSegDist = dialHeap.BuildRoute (nDestSeg);
		int j = gameData.fcd.nConnSegDist - 2;
		short* route = dialHeap.Route ();
		xDist = 0;
		for (int i = 1; i < j; i++)
			xDist += CFixVector::Dist (SEGMENTS [route [i]].Center (), SEGMENTS [route [i + 1]].Center ());
		xDist += CFixVector::Dist (p0, SEGMENTS [route [1]].Center ()) + CFixVector::Dist (p1, SEGMENTS [route [j]].Center ());
		return xDist;
		}
#if DBG
	nExpanded++;
#endif
	segP = SEGMENTS + nSegment;
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
		if ((segP->m_children [nSide] >= 0) && (segP->IsDoorWay (nSide, NULL) & widFlag))
			dialHeap.Push (segP->m_children [nSide], nSegment, nDist + segP->m_childDists [nSide]);
		}
	}

#	endif

#else

if (!++scanInfo.bFlag) {
	memset (scanData, 0, sizeof (scanData));
	scanInfo.bFlag = 1;
	}

if (nMaxDepth < 0)
	nMaxDepth = gameData.segs.nSegments;


scanData [0].segPath [nStartSeg].bVisited = scanInfo.bFlag;
scanData [0].segPath [nStartSeg].nDepth = 0;
scanData [0].segPath [nStartSeg].nPred = -1;
scanData [0].nStartSeg = 
scanData [0].segQueue [0] = nStartSeg;
scanData [0].nDestSeg = nDestSeg;
scanData [0].nTail = 0;
scanData [0].nHead = 1;
scanData [0].nRouteDir = (nCacheType == 0);
scanData [0].nLinkSeg = 0;

scanInfo.widFlag = widFlag;
scanInfo.nLinkSeg = -1;
scanInfo.bScanning = 3;

#if BIDIRECTIONAL_SCAN

scanInfo.nMaxDepth = nMaxDepth / 2 + 1;

// bi-directional scanner (expands 1/3 of the segments the uni-directional scanner does on average)

scanData [1].segPath [nDestSeg].bVisited = scanInfo.bFlag;
scanData [1].segPath [nDestSeg].nDepth = 0;
scanData [1].segPath [nDestSeg].nPred = -1;
scanData [1].nStartSeg =
scanData [1].segQueue [0] = nDestSeg;
scanData [1].nDestSeg = nStartSeg;
scanData [1].nTail = 0;
scanData [1].nHead = 1;
scanData [1].nRouteDir = (nCacheType != 0);
scanData [1].nLinkSeg = 0;

#if DBG
if ((nStartSeg == 4056) && (nDestSeg == 4060))
	nDbgSeg = nDbgSeg;
if ((nStartSeg == 702) && (nDestSeg == 71))
	nDbgSeg = nDbgSeg;
#endif

#if 0
if (gameStates.app.nThreads > 1) {
	SDL_Thread* threads [2];
	int nThreadIds [2] = {0, 1};
	threads [0] = SDL_CreateThread (ExpandSegmentMT, nThreadIds);
	threads [1] = SDL_CreateThread (ExpandSegmentMT, nThreadIds + 1);
	SDL_WaitThread (threads [0], NULL);
	SDL_WaitThread (threads [1], NULL);

	if ((0 < (scanInfo.nLinkSeg = scanData [0].nLinkSeg)) || (0 < (scanInfo.nLinkSeg = scanData [1].nLinkSeg)))
		return BuildPathBiDir (p0, p1, nCacheType);
	}
else 
#endif
	{
	for (;;) {
		for (int nDir = 0; nDir < 2; nDir++) {
			if (!(scanInfo.nLinkSeg = ExpandSegment (nDir)))
				continue;
			if (scanInfo.nLinkSeg < 0)
				goto errorExit;
			// destination segment reached
			return BuildPathBiDir (p0, p1, nCacheType);
			}
		}	
	}

errorExit:

#else

// uni-directional scanner

scanInfo.nMaxDepth = nMaxDepth;

for (;;) {
	if (0 > (scanInfo.nLinkSeg = ExpandSegment (nDir)))
		break;
	if (scanInfo.nLinkSeg < 0)
		// destination segment reached
		return BuildPathUniDir (p0, p1, nCacheType);
	}	

#endif

if (nCacheType >= 0) 
	fcdCaches [nCacheType].Add (nStartSeg, nDestSeg, 10000, I2X (10000));
return -1;

#endif

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

	objP->info.position.mOrient.RVec ()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec ()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec ()[X] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.RVec ()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec ()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec ()[Y] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.RVec ()[Z] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.UVec ()[Z] = *segP++ << MATRIX_PRECISION;
	objP->info.position.mOrient.FVec ()[Z] = *segP++ << MATRIX_PRECISION;

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

	vs.SetZero ();
	ve.SetZero ();

	for (i=0; i<4; i++) {
		vs += gameData.segs.vertices [segP->m_verts [sideVertIndex [start][i]]];
		ve += gameData.segs.vertices [segP->m_verts [sideVertIndex [end][i]]];
	}

	*vp = ve - vs;
	*vp *= (I2X (1)/4);

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

// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *bFlip set, then negate Normal after computation.
//	Note, pos [Y]u cannot just compute the Normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
short GetVertsForNormal (short v0, short v1, short v2, short v3, short* vSorted)
{
	int		i, j;
	ushort	index [4] = {0, 1, 2, 3};

//	index is a list that shows how things got scrambled so we know if our Normal is pointing backwards
vSorted [0] = v0;
vSorted [1] = v1;
vSorted [2] = v2;
vSorted [3] = v3;
// bubble sort vSorted in reverse order (largest first)
for (i = 1; i < 4; i++)
	for (j = 0; j < i; j++)
		if (vSorted [j] > vSorted [i]) {
			Swap (vSorted [i], vSorted [j]);
			Swap (index [i], index [j]);
			}

Assert ((vSorted [0] < vSorted [1]) && (vSorted [1] < vSorted [2]) && (vSorted [2] < vSorted [3]));
//	Now, if for any index [i] & index [i+1]: index [i+1] = (index [i]+3)%4, then must flip Normal
return (((index [0] + 3) % 4) == index [1]) || (((index [1] + 3) % 4) == index [2]);
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
	SEGMENTS [i].Setup ();
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
	short*	childP;

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
//	Do a bfs from nSegment, marking slots in markedSegs if the segment is reachable.

#define	AMBIENT_SEGMENT_DEPTH 5

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
	pp->p3_normal.vNormal.SetZero ();
	pp->p3_normal.nFaces = 0;
	}
}

//	-----------------------------------------------------------------------------
//eof
