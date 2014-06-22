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

#include "descent.h"
#include "error.h"
#include "segment.h"
#include "segmath.h"
#include "findpath.h"
#include "byteswap.h"

#define USE_DACS 0
#define USE_FCD_CACHE 1
#define BIDIRECTIONAL_SCAN 1
#define MULTITHREADED_SCAN 0

#if DBG
#	define DBG_SCAN 0
#else
#	define DBG_SCAN 0
#endif

#if USE_DACS
#	include "dialheap.h"
#endif

CSimpleBiDirRouter simpleRouter [MAX_THREADS];
CDACSUniDirRouter uniDacsRouter [MAX_THREADS];
CDACSBiDirRouter biDacsRouter [MAX_THREADS];

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int CScanInfo::Setup (CSimpleHeap* heap, int nWidFlag, int nMaxDist)
{
m_nLinkSeg = 0;
m_bScanning = 3;
m_heap = heap;
m_widFlag = nWidFlag;
m_maxDist = (nMaxDist < 0) ? gameData.segs.nSegments : nMaxDist;
if (!++m_bFlag)
	m_bFlag = 1;
return m_bFlag;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CSimpleHeap::Setup (short nStartSeg, short nDestSeg, uint flag, int dir)
{
m_path [nStartSeg].m_bVisited = flag;
m_path [nStartSeg].m_nDepth = 0;
m_path [nStartSeg].m_nPred = -1;
m_nStartSeg =
m_queue [0] = nStartSeg;
m_nDestSeg = nDestSeg;
m_nTail = 0;
m_nHead = 1;
m_bFlag = flag;
m_nDir = dir;
m_nLinkSeg = 0;
}

//	-----------------------------------------------------------------------------

short CSimpleHeap::Expand (CScanInfo& scanInfo)
{
if (m_nTail >= m_nHead)
	return m_nLinkSeg = -1;
short nPredSeg = m_queue [m_nTail++];

#if DBG_SCAN
if (nPredSeg == nDbgSeg)
	BRP;
#endif

short m_nDepth = m_path [nPredSeg].m_nDepth + 1;
if (m_nDepth > scanInfo.m_maxDist)
	return m_nLinkSeg = scanInfo.Scanning (m_nDir) ? 0 : -1;

CSegment* segP = SEGMENTS + nPredSeg;
for (short nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	short nSuccSeg = segP->m_children [nSide];
	if (nSuccSeg < 0)
		continue;
	if (m_nDir) {
		CSegment* otherSegP = SEGMENTS + nSuccSeg;
		short nOtherSide = SEGMENTS [nPredSeg].ConnectedSide (otherSegP);
		if ((nOtherSide == -1) || !(otherSegP->IsPassable (nOtherSide, NULL) & scanInfo.m_widFlag))
			continue;
		}
	else {
		if (!(segP->IsPassable (nSide, NULL) & scanInfo.m_widFlag))
			continue;
		}

#if DBG_SCAN
	if (nSuccSeg == nDbgSeg)
		BRP;
	if (nSuccSeg >= gameData.segs.nSegments) {
		PrintLog (0, "internal error in simple router!\n");
		return -1;
		}
	if ((nSuccSeg < 0) || (nSuccSeg >= gameData.segs.nSegments)) {
		PrintLog (0, "invalid successor in CSimpleHeap::Expand\n");
		return -1;
		}
#endif
	CPathNode& pathNode = m_path [nSuccSeg];
	if (pathNode.m_bVisited == scanInfo.m_bFlag)
		continue;
	pathNode.m_nPred = nPredSeg;
	pathNode.m_nEdge = nSide;
	if (Match (nSuccSeg, scanInfo))
		return m_nLinkSeg = nSuccSeg + 1;
	pathNode.m_bVisited = scanInfo.m_bFlag;
	pathNode.m_nDepth = m_nDepth;
	m_queue [m_nHead++] = nSuccSeg;
#if DBG_SCAN
	if (m_nHead >= gameData.segs.nSegments) {
		PrintLog (0, "internal error in simple router!\n");
		return -1;
		}
#endif
	}
return 0;
}

// -----------------------------------------------------------------------------

bool CSimpleUniDirHeap::Match (short nSegment, CScanInfo& scanInfo)
{
return (nSegment == m_nDestSeg);
}

// -----------------------------------------------------------------------------

bool CSimpleBiDirHeap::Match (short nSegment, CScanInfo& scanInfo)
{
return (scanInfo.m_heap [!m_nDir].m_path [nSegment].m_bVisited == m_bFlag);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

//static SDL_mutex* semaphore = NULL;

#if MULTITHREADED_SCAN

int _CDECL_ ExpandSegmentMT (void* nThreadP)
{
	int nDir = *((int *) nThreadP);

while (!(ExpandSegment (nDir) || m_heap [!nDir].nLinkSeg))
	;
return 1;
}

#endif

// -----------------------------------------------------------------------------

int CRouter::SetSegment (const short nSegment, const CFixVector& p)
{
return (nSegment < 0) ? FindSegByPos (p, 0, 1, 0) : nSegment;
}

// -----------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum m_nDepth of m_maxDist.
//	Return the distance.

fix CRouter::PathLength (const CFixVector& p0, const short nStartSeg, const CFixVector& p1, 
								 const short nDestSeg, const int nMaxDist, const int nWidFlag, const int nCacheType)
{
#if 0 //DBG
//if (!m_cacheType) 
	{
	m_cache [m_cacheType].SetPathLength (10000);
	return -1;
	}
#endif

if (0 > (m_nStartSeg = SetSegment (nStartSeg, p0)))
	return -1;
m_nDestSeg = nDestSeg;
m_p0 = p0;
m_p1 = p1;

if (m_nDestSeg >= 0) {
	// same segment?
#if !DBG
	m_cacheType = nCacheType;
	if ((m_cacheType >= 0) && (m_nStartSeg == m_nDestSeg)) {
		m_cache [m_cacheType].SetPathLength (0);
		return CFixVector::Dist (m_p0, m_p1);
		}
#endif
	// adjacent segments?
	if (m_cacheType >= 0) {
		short nSide = SEGMENTS [m_nStartSeg].ConnectedSide (SEGMENTS + m_nDestSeg);
		if ((nSide != -1) && (SEGMENTS [m_nDestSeg].IsPassable (nSide, NULL) & m_widFlag)) {
			m_cache [m_cacheType].SetPathLength (1);
			return CFixVector::Dist (m_p0, m_p1);
			}
		}

#if !DBG
#	if USE_FCD_CACHE
	if (m_cacheType >= 0) {
		fix xDist = m_cache [m_cacheType].Dist (m_nStartSeg, m_nDestSeg);
		if (xDist >= 0)
			return xDist;
		}
#	endif
#endif
	}

m_maxDist = nMaxDist;
m_widFlag = nWidFlag;

fix distance = FindPath ();

if ((distance < 0) && (m_cacheType >= 0))
	m_cache [m_cacheType].Add (m_nStartSeg, m_nDestSeg, 1024, I2X (1024));

return distance;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// uni-directional scanner

fix CSimpleUniDirRouter::BuildPath (void)
{
	fix	xDist;
	int	nLength = 0; 
	short	nPredSeg, nSuccSeg = m_nDestSeg;

nPredSeg = --m_scanInfo.m_nLinkSeg;
xDist = CFixVector::Dist (m_p1, SEGMENTS [nPredSeg].Center ());
for (;;) {
	nPredSeg = m_heap.m_path [nSuccSeg].m_nPred;
	if (nPredSeg == m_heap.m_nStartSeg)
		break;
	nLength++;
	xDist += SEGMENTS [nPredSeg].m_childDists [0][m_heap.m_path [nSuccSeg].m_nEdge];
	nSuccSeg = nPredSeg;
	}
xDist += CFixVector::Dist (m_p0, SEGMENTS [nSuccSeg].Center ());
if (m_cacheType >= 0) 
	m_cache [m_cacheType].Add (m_heap.m_nStartSeg, m_heap.m_nDestSeg, nLength + 3, xDist);
return xDist;
}

// -----------------------------------------------------------------------------

fix CSimpleUniDirRouter::FindPath (void)
{
if (0 > (m_nDestSeg = SetSegment (m_nDestSeg, m_p1)))
	return -1;
m_scanInfo.Setup (&m_heap, m_widFlag, m_maxDist);
m_heap.Setup (m_nStartSeg, m_nDestSeg, m_scanInfo.m_bFlag, 0);

for (;;) {
	if (0 > (m_scanInfo.m_nLinkSeg = m_heap.Expand (m_scanInfo)))
		return -1;
	if (m_scanInfo.m_nLinkSeg > 0) // destination segment reached
		return BuildPath ();
	}	
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

fix CSimpleBiDirRouter::BuildPath (void)
{
	fix	xDist = 0;
	int	nLength = 0; 
	short	nPredSeg, nSuccSeg;

--m_scanInfo.m_nLinkSeg;
for (int nDir = 0; nDir < 2; nDir++) {
	CSimpleBiDirHeap& heap = m_heap [nDir];
	nSuccSeg = m_scanInfo.m_nLinkSeg;
	for (;;) {
		nPredSeg = heap.m_path [nSuccSeg].m_nPred;
		if (nPredSeg < 0)
			break;
		if (nPredSeg == heap.m_nStartSeg) {
			xDist += CFixVector::Dist (nDir ? m_p1 : m_p0, SEGMENTS [nSuccSeg].Center ());
			break;
			}
		++nLength;
		if ((nLength > 2 * m_scanInfo.m_maxDist + 2) || (nLength > gameData.segs.nSegments))
			return -0x7FFFFFFF;
		xDist += SEGMENTS [nPredSeg].m_childDists [0][heap.m_path [nSuccSeg].m_nEdge];
		nSuccSeg = nPredSeg;
		}
	}
if (xDist == 0)
	return -0x7FFFFFFF;
if (m_cacheType >= 0) 
	m_cache [m_cacheType].Add (m_heap [0].m_nStartSeg, m_heap [0].m_nDestSeg, nLength + 3, xDist);
return xDist;
}

// -----------------------------------------------------------------------------

fix CSimpleBiDirRouter::FindPath (void)
{
if (0 > (m_nDestSeg = SetSegment (m_nDestSeg, m_p1)))
	return -1;

m_scanInfo.Setup (m_heap, m_widFlag, m_maxDist);
m_heap [0].Setup (m_nStartSeg, m_nDestSeg, m_scanInfo.m_bFlag, 0);
m_heap [1].Setup (m_nDestSeg, m_nStartSeg, m_scanInfo.m_bFlag, 1);

#if MULTITHREADED_SCAN
if (gameStates.app.nThreads > 1) {
	SDL_Thread* threads [2];
	int nThreadIds [2] = {0, 1};
	threads [0] = SDL_CreateThread (ExpandSegmentMT, nThreadIds);
	threads [1] = SDL_CreateThread (ExpandSegmentMT, nThreadIds + 1);
	SDL_WaitThread (threads [0], NULL);
	SDL_WaitThread (threads [1], NULL);

	if (((0 < (m_scanInfo.m_nLinkSeg = m_heap [0].nLinkSeg)) && (m_scanInfo.m_nLinkSeg != m_heap [0].m_nDestSeg + 1)) || 
		 ((0 < (m_scanInfo.m_nLinkSeg = m_heap [1].nLinkSeg)) && (m_scanInfo.m_nLinkSeg != m_heap [1].m_nDestSeg + 1)))
		return BuildPathBiDir (m_p0, m_p1, m_cacheType);
	}
else 
#endif
	{
	for (;;) {
		for (int nDir = 0; nDir < 2; nDir++) {
			if (!(m_scanInfo.m_nLinkSeg = m_heap [nDir].Expand (m_scanInfo))) // nLinkSeg == 0 -> keep expanding
				continue;
			if (m_scanInfo.m_nLinkSeg < 0)
				return -1;
			// destination segment reached
			return BuildPath ();
			}
		}	
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

bool CDACSUniDirRouter::Create (int nNodes) 
{ 
if ((m_nNodes != nNodes) && !m_heap.Create (nNodes)) {
	SetSize (0);
	return false;
	}
SetSize (nNodes);
return true;
}

// -----------------------------------------------------------------------------

fix CDACSUniDirRouter::BuildPath (short nSegment)
{
if (m_heap.Cost (nSegment) == 0xFFFFFFFF)
	return -1;

	int h = m_heap.BuildRoute (nSegment);
	CDialHeap::tPathNode* route = m_heap.Route ();

if (route [0].nNode == nSegment)
	return 0;
if (m_nDestSeg >= 0)
	h -= 2;

	fix xDist = 0;

#if 0
	CHitQuery	fq (FQ_TRANSWALL | FQ_TRANSPOINT | FQ_VISIBILITY, &VERTICES [0], &VERTICES [0], route [0].nNode, -1, 1, 0);
	CHitResult		hitResult;
#endif
	CFixVector* p0, *p1;
	short			nStartSeg, nDestSeg;

for (int i = 0, j; i < h; i = j) {
	// beginning at segment route [i].node, traverse the route until the center of route segment route [j].nNode cannot be seen from 
	// the center of segment route [i].nNode. That way, the distance calculation is corrected by using direct lines of sight between
	// segments of the route that can "see" each other even if they aren't directly connected.
	nStartSeg = route [i].nNode;
	if ((nStartSeg < 0) || (nStartSeg >= gameData.segs.nSegments))
		return -2;
	/*fq.*/p0 = p1 = &SEGMENTS [nStartSeg].Center ();
	for (j = i + 1; j < h; j++) { 
		nDestSeg = route [j].nNode;
#if 1
		if (!gameData.segs.SegVis (nStartSeg, nDestSeg))
			break;
		p1 = &SEGMENTS [nDestSeg].Center ();
#else
		fq.p1 = &SEGMENTS [nDestSeg].Center ();
		int nHitType = FindHitpoint (&fq, &hitResult);
		if (nHitType && ((nHitType != HIT_WALL) || (hitResult.nSegment != nDestSeg)))
			break;
		p1 = fq.p1;
#endif
		}	
	if (j < i + 2) // can only see next segment after route [i].nNode
		xDist += SEGMENTS [nStartSeg].m_childDists [0][route [i].nEdge];
	else {// skipped some segment(s)
		xDist += CFixVector::Dist (*p0, *p1);
		--j;
		}
	}
if	(m_nDestSeg >= 0) {
	xDist += CFixVector::Dist (m_p0, SEGMENTS [route [1].nNode].Center ()) + CFixVector::Dist (m_p1, SEGMENTS [route [h].nNode].Center ());
	if (m_cacheType >= 0) 
		m_cache [m_cacheType].Add (m_nStartSeg, m_nDestSeg, h + 2, xDist);
	}
return xDist;
}

// -----------------------------------------------------------------------------
// Specialty: If m_nDestSeg < 0, then the paths to all segments reachable 
// from m_nStartSeg is computed

fix CDACSUniDirRouter::FindPath (void)
{
	uint			nDist;
	short			nSegment, nSide;
	CSegment*	segP;

m_heap.Setup (m_nStartSeg);

	int nExpanded = 1;

for (;;) {
	nSegment = m_heap.Pop (nDist);
	if (nSegment < 0)
		return (m_nDestSeg < 0) ? nExpanded : -1;
	if (nSegment == m_nDestSeg)
		return BuildPath (nSegment);
	segP = SEGMENTS + nSegment;
#if DBG
	if (nSegment == nDbgSeg)
		BRP;
#endif
	for (nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
		if ((segP->m_children [nSide] >= 0) && (segP->IsPassable (nSide, NULL) & m_widFlag)) {
#if DBG
			if (segP->m_children [nSide] == nDbgSeg)
				BRP;
#endif
			if (m_heap.Push (segP->m_children [nSide], nSegment, nSide, nDist + ushort (segP->m_childDists [1][nSide])))
				++nExpanded;
			}
		}
	}
}

// -----------------------------------------------------------------------------

fix CDACSUniDirRouter::Distance (short nSegment)
{
return BuildPath (nSegment);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

bool CDACSBiDirRouter::Create (int nNodes) 
{ 
if ((m_nNodes != nNodes) && !(m_heap [0].Create (nNodes) && m_heap [1].Create (nNodes)))
	return false;
m_nNodes = nNodes;
return true;
}

// -----------------------------------------------------------------------------

int CDACSBiDirRouter::Expand (int nDir)
{
	uint nDist;

short nSegment = m_heap [nDir].Pop (nDist);
if ((nSegment < 0) || (nSegment == m_nDestSeg))
	return nSegment;
if (m_heap [!nDir].Popped (nSegment))
	return nSegment;
CSegment* segP = SEGMENTS + nSegment;
for (short nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	if ((segP->m_children [nSide] >= 0) && (segP->IsPassable (nSide, NULL) & m_widFlag)) {
		uint nNewDist = nDist + ushort (segP->m_childDists [1][nSide]);
		if (nNewDist < uint (m_maxDist))
			m_heap [nDir].Push (segP->m_children [nSide], nSegment, nSide, nNewDist);
		}
	}
return nSegment;
}

// -----------------------------------------------------------------------------

fix CDACSBiDirRouter::BuildPath (short nSegment)
{
	int j = -2;

if (m_nSegments [0] >= 0)
	j += m_heap [0].BuildRoute (nSegment, 0, m_route) - 1;
if (m_nSegments [1] >= 0)
	j += m_heap [1].BuildRoute (nSegment, 1, m_route + j);
fix xDist = 0;
for (int i = 1; i < j; i++)
	xDist += SEGMENTS [m_route [i - 1].nNode].m_childDists [0][m_route [i].nEdge];
xDist += CFixVector::Dist (m_p0, SEGMENTS [m_route [1].nNode].Center ()) + CFixVector::Dist (m_p1, SEGMENTS [m_route [j].nNode].Center ());
if (m_cacheType >= 0) 
	m_cache [m_cacheType].Add (m_nStartSeg, m_nDestSeg, j + 2, xDist);
return xDist;
}

// -----------------------------------------------------------------------------

fix CDACSBiDirRouter::FindPath (void)
{
if (0 > (m_nDestSeg = SetSegment (m_nDestSeg, m_p1)))
	return -1;
m_heap [0].Setup (m_nSegments [0] = m_nStartSeg);
m_heap [1].Setup (m_nSegments [1] = m_nDestSeg);

	short	nSegment;

for (;;) {
	if (m_nSegments [0] >= 0) {
		m_nSegments [0] = Expand (0);
		}
	if (m_nSegments [1] >= 0) {
		m_nSegments [1] = Expand (1);
		}
	if ((m_nSegments [0] < 0) && (m_nSegments [1] < 0))
		return -1;
	if (m_nSegments [0] == m_nDestSeg) {
		nSegment = m_nSegments [0];
		m_nSegments [1] = -1;
		}
	else if (m_nSegments [1] == m_nStartSeg) {
		nSegment = m_nSegments [1];
		m_nSegments [0] = -1;
		}
	else if ((m_nSegments [1] >= 0) && m_heap [0].Popped (m_nSegments [1]))
		nSegment = m_nSegments [1];
	else if ((m_nSegments [0] >= 0) && m_heap [1].Popped (m_nSegments [0]))
		nSegment = m_nSegments [0];
	else
		continue;
	return BuildPath (nSegment);
	}
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//eof
