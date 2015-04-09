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
for (int32_t i = 0; i < MAX_FCD_CACHE; i++)
	m_cache [i].seg0 = -1;
}

// -----------------------------------------------------------------------------------

void CFCDCache::Add (int32_t seg0, int32_t seg1, int32_t nPathLen, fix dist)
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
	for (int32_t i = 0; i < MAX_FCD_CACHE; i++) {
		if ((m_cache [i].seg0 == seg0) && (m_cache [i].seg1 == seg1)) {
			m_cache [m_nIndex].seg0 = -1;
			break;
			}
		}
	}
}

// -----------------------------------------------------------------------------

fix CFCDCache::Dist (int16_t seg0, int16_t seg1)
{
	tFCDCacheData*	pc = m_cache.Buffer ();

for (int32_t i = int32_t (m_cache.Length ()); i; i--, pc++) {
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

int32_t CScanInfo::Setup (CSimpleHeap* heap, int32_t nWidFlag, int32_t nMaxDist)
{
m_nLinkSeg = 0;
m_bScanning = 3;
m_heap = heap;
m_widFlag = nWidFlag;
m_maxDist = (nMaxDist < 0) ? gameData.segData.nSegments : nMaxDist;
if (!++m_bFlag)
	m_bFlag = 1;
return m_bFlag;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CSimpleHeap::Setup (int16_t nStartSeg, int16_t nDestSeg, uint32_t flag, int32_t dir)
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

int16_t CSimpleHeap::Expand (CScanInfo& scanInfo)
{
if (m_nTail >= m_nHead)
	return m_nLinkSeg = -1;
int16_t nPredSeg = m_queue [m_nTail++];

#if DBG_SCAN
if (nPredSeg == nDbgSeg)
	BRP;
#endif

int16_t m_nDepth = m_path [nPredSeg].m_nDepth + 1;
if (m_nDepth > scanInfo.m_maxDist)
	return m_nLinkSeg = scanInfo.Scanning (m_nDir) ? 0 : -1;

CSegment* segP = SEGMENT (nPredSeg);
for (int16_t nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	int16_t nSuccSeg = segP->m_children [nSide];
	if (nSuccSeg < 0)
		continue;
	if (m_nDir) {
		CSegment* otherSegP = SEGMENT (nSuccSeg);
		int16_t nOtherSide = SEGMENT (nPredSeg)->ConnectedSide (otherSegP);
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
	if (nSuccSeg >= gameData.segData.nSegments) {
		PrintLog (0, "internal error in simple router!\n");
		return -1;
		}
	if ((nSuccSeg < 0) || (nSuccSeg >= gameData.segData.nSegments)) {
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
	if (m_nHead >= gameData.segData.nSegments) {
		PrintLog (0, "internal error in simple router!\n");
		return -1;
		}
#endif
	}
return 0;
}

// -----------------------------------------------------------------------------

bool CSimpleUniDirHeap::Match (int16_t nSegment, CScanInfo& scanInfo)
{
return (nSegment == m_nDestSeg);
}

// -----------------------------------------------------------------------------

bool CSimpleBiDirHeap::Match (int16_t nSegment, CScanInfo& scanInfo)
{
return (scanInfo.m_heap [!m_nDir].m_path [nSegment].m_bVisited == m_bFlag);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

//static SDL_mutex* semaphore = NULL;

#if MULTITHREADED_SCAN

int32_t _CDECL_ ExpandSegmentMT (void* nThreadP)
{
	int32_t nDir = *((int32_t *) nThreadP);

while (!(ExpandSegment (nDir) || m_heap [!nDir].nLinkSeg))
	;
return 1;
}

#endif

// -----------------------------------------------------------------------------

int32_t CRouter::SetSegment (const int16_t nSegment, const CFixVector& p)
{
return (nSegment < 0) ? FindSegByPos (p, 0, 1, 0) : nSegment;
}

// -----------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum m_nDepth of m_maxDist.
//	Return the distance.

fix CRouter::PathLength (const CFixVector& p0, const int16_t nStartSeg, const CFixVector& p1, 
								 const int16_t nDestSeg, const int32_t nMaxDist, const int32_t nWidFlag, const int32_t nCacheType)
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
		int16_t nSide = SEGMENT (m_nStartSeg)->ConnectedSide (SEGMENT (m_nDestSeg));
		if ((nSide != -1) && (SEGMENT (m_nDestSeg)->IsPassable (nSide, NULL) & m_widFlag)) {
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
	int32_t	nLength = 0; 
	int16_t	nPredSeg, nSuccSeg = m_nDestSeg;

nPredSeg = --m_scanInfo.m_nLinkSeg;
xDist = CFixVector::Dist (m_p1, SEGMENT (nPredSeg)->Center ());
for (;;) {
	nPredSeg = m_heap.m_path [nSuccSeg].m_nPred;
	if (nPredSeg == m_heap.m_nStartSeg)
		break;
	nLength++;
	xDist += SEGMENT (nPredSeg)->m_childDists [0][m_heap.m_path [nSuccSeg].m_nEdge];
	nSuccSeg = nPredSeg;
	}
xDist += CFixVector::Dist (m_p0, SEGMENT (nSuccSeg)->Center ());
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
	int32_t	nLength = 0; 
	int16_t	nPredSeg, nSuccSeg;

--m_scanInfo.m_nLinkSeg;
for (int32_t nDir = 0; nDir < 2; nDir++) {
	CSimpleBiDirHeap& heap = m_heap [nDir];
	nSuccSeg = m_scanInfo.m_nLinkSeg;
	for (;;) {
		nPredSeg = heap.m_path [nSuccSeg].m_nPred;
		if (nPredSeg < 0)
			break;
		if (nPredSeg == heap.m_nStartSeg) {
			xDist += CFixVector::Dist (nDir ? m_p1 : m_p0, SEGMENT (nSuccSeg)->Center ());
			break;
			}
		++nLength;
		if ((nLength > 2 * m_scanInfo.m_maxDist + 2) || (nLength > gameData.segData.nSegments))
			return -0x7FFFFFFF;
		xDist += SEGMENT (nPredSeg)->m_childDists [0][heap.m_path [nSuccSeg].m_nEdge];
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
	int32_t nThreadIds [2] = {0, 1};
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
		for (int32_t nDir = 0; nDir < 2; nDir++) {
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

bool CDACSUniDirRouter::Create (int32_t nNodes) 
{ 
if ((m_nNodes != nNodes) && !m_heap.Create (nNodes)) {
	SetSize (0);
	return false;
	}
SetSize (nNodes);
return true;
}

// -----------------------------------------------------------------------------

fix CDACSUniDirRouter::BuildPath (int16_t nSegment)
{
if (m_heap.Cost (nSegment) == 0xFFFFFFFF)
	return -1;

	int32_t h = m_heap.BuildRoute (nSegment);
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
	int16_t			nStartSeg, nDestSeg;

for (int32_t i = 0, j; i < h; i = j) {
	// beginning at segment route [i].node, traverse the route until the center of route segment route [j].nNode cannot be seen from 
	// the center of segment route [i].nNode. That way, the distance calculation is corrected by using direct lines of sight between
	// segments of the route that can "see" each other even if they aren't directly connected.
	nStartSeg = route [i].nNode;
	if ((nStartSeg < 0) || (nStartSeg >= gameData.segData.nSegments))
		return -2;
	/*fq.*/p0 = p1 = &SEGMENT (nStartSeg)->Center ();
	for (j = i + 1; j < h; j++) { 
		nDestSeg = route [j].nNode;
#if 1
		if (!gameData.segData.SegVis (nStartSeg, nDestSeg))
			break;
		p1 = &SEGMENT (nDestSeg)->Center ();
#else
		fq.p1 = &SEGMENT (nDestSeg)->Center ();
		int32_t nHitType = FindHitpoint (&fq, &hitResult);
		if (nHitType && ((nHitType != HIT_WALL) || (hitResult.nSegment != nDestSeg)))
			break;
		p1 = fq.p1;
#endif
		}	
	if (j < i + 2) // can only see next segment after route [i].nNode
		xDist += SEGMENT (nStartSeg)->m_childDists [0][route [i].nEdge];
	else {// skipped some segment(s)
		xDist += CFixVector::Dist (*p0, *p1);
		--j;
		}
	}
if	(m_nDestSeg >= 0) {
	xDist += CFixVector::Dist (m_p0, SEGMENT (route [1].nNode)->Center ()) + CFixVector::Dist (m_p1, SEGMENT (route [h].nNode)->Center ());
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
	uint32_t		nDist;
	int16_t		nSegment, nSide;
	CSegment*	segP;

m_heap.Setup (m_nStartSeg);

	int32_t nExpanded = 1;

for (;;) {
	nSegment = m_heap.Pop (nDist);
	if (nSegment < 0)
		return (m_nDestSeg < 0) ? nExpanded : -1;
	if (nSegment == m_nDestSeg)
		return BuildPath (nSegment);
	segP = SEGMENT (nSegment);
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
			if (m_heap.Push (segP->m_children [nSide], nSegment, nSide, nDist + uint16_t (segP->m_childDists [1][nSide])))
				++nExpanded;
			}
		}
	}
}

// -----------------------------------------------------------------------------

fix CDACSUniDirRouter::Distance (int16_t nSegment)
{
return BuildPath (nSegment);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

bool CDACSBiDirRouter::Create (int32_t nNodes) 
{ 
if ((m_nNodes != nNodes) && !(m_heap [0].Create (nNodes) && m_heap [1].Create (nNodes)))
	return false;
m_nNodes = nNodes;
return true;
}

// -----------------------------------------------------------------------------

int32_t CDACSBiDirRouter::Expand (int32_t nDir)
{
	uint32_t nDist;

int16_t nSegment = m_heap [nDir].Pop (nDist);
if ((nSegment < 0) || (nSegment == m_nDestSeg))
	return nSegment;
if (m_heap [!nDir].Popped (nSegment))
	return nSegment;
CSegment* segP = SEGMENT (nSegment);
for (int16_t nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++) {
	if ((segP->m_children [nSide] >= 0) && (segP->IsPassable (nSide, NULL) & m_widFlag)) {
		uint32_t nNewDist = nDist + uint16_t (segP->m_childDists [1][nSide]);
		if (nNewDist < uint32_t (m_maxDist))
			m_heap [nDir].Push (segP->m_children [nSide], nSegment, nSide, nNewDist);
		}
	}
return nSegment;
}

// -----------------------------------------------------------------------------

fix CDACSBiDirRouter::BuildPath (int16_t nSegment)
{
	int32_t j = -2;

if (m_nSegments [0] >= 0)
	j += m_heap [0].BuildRoute (nSegment, 0, m_route) - 1;
if (m_nSegments [1] >= 0)
	j += m_heap [1].BuildRoute (nSegment, 1, m_route + j);
fix xDist = 0;
for (int32_t i = 1; i < j; i++)
	xDist += SEGMENT (m_route [i - 1].nNode)->m_childDists [0][m_route [i].nEdge];
xDist += CFixVector::Dist (m_p0, SEGMENT (m_route [1].nNode)->Center ()) + CFixVector::Dist (m_p1, SEGMENT (m_route [j].nNode)->Center ());
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

	int16_t	nSegment;

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
