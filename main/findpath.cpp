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

// -----------------------------------------------------------------------------

void FlushFCDCache (void)
{
m_cache [0].Flush ();
m_cache [1].Flush ();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void CScanData::Setup (short nStartSeg, short nDestSeg, uint flag, int dir)
{
m_path [nStartSeg].bVisited = flag;
m_path [nStartSeg].nDepth = 0;
m_path [nStartSeg].nPred = -1;
m_nStartSeg =
m_segQueue [0] = nStartSeg;
m_nDestSeg = nDestSeg;
m_nTail = 0;
m_nHead = 1;
m_nRouteDir = dir;
m_nLinkSeg = 0;
}

//	-----------------------------------------------------------------------------

short CScanData::Expand (CScanInfo& scanInfo)
{
if (m_nTail >= m_nHead)
	return m_nLinkSeg = -1;
short nPredSeg = m_queue [m_nTail++];
#if DBG_SCAN
if (nPredSeg == nDbgSeg)
	nDbgSeg = nDbgSeg;
#endif
short nDepth = m_path [nPredSeg].nDepth + 1;
if (nDepth > scanInfo.m_maxDepth) {
	scanInfo.bScanning &= ~(1 << nDir);
	return m_nLinkSeg = (scanInfo.bScanning ? 0 : -1);
	}
CSegment* segP = SEGMENTS + nPredSeg;
for (short nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	short nSuccSeg = segP->m_children [nSide];
	if (nSuccSeg < 0)
		continue;
	if (m_nRouteDir) {
		CSegment* otherSegP = SEGMENTS + nSuccSeg;
		short nOtherSide = SEGMENTS [nPredSeg].ConnectedSide (otherSegP);
		if ((nOtherSide == -1) || !(otherSegP->IsDoorWay (nOtherSide, NULL) & scanInfo.m_widFlag))
			continue;
		}
	else {
		if (!(segP->IsDoorWay (nSide, NULL) & scanInfo.m_widFlag))
			continue;
		}
#if DBG_SCAN
	if (nSuccSeg == nDbgSeg)
		nDbgSeg = nDbgSeg;
#endif
	tSegPathNode* pathNodeP = &m_m_path [nSuccSeg];
	if (pathNodeP->bVisited == scanInfo.bFlag)
		continue;
	pathNodeP->nPred = nPredSeg;
	if (Match (nSuccSeg, nDir))
		return m_nLinkSeg = nSuccSeg + 1;
	pathNodeP->bVisited = scanInfo.bFlag;
	pathNodeP->nDepth = nDepth;
	m_segQueue [m_nHead++] = nSuccSeg;
	}
return 0;
}

// -----------------------------------------------------------------------------

bool CBiDirScanData::Match (short nSegment, int nDir)
{
return (m_scanData [!nDir].segPath [nSegment].bVisited == m_scanInfo.bFlag);
#else
	if (nSuccSeg == sd.m_nDestSeg)
#endif
}

// -----------------------------------------------------------------------------

bool CUniDirScanData::Match (short nSegment, int nDir)
{
return (nSegment == m_scanData.m_nDestSeg)
}

//	-----------------------------------------------------------------------------

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

//static SDL_mutex* semaphore = NULL;

//	-----------------------------------------------------------------------------

#if MULTITHREADED_SCAN

int _CDECL_ ExpandSegmentMT (void* nThreadP)
{
	int nDir = *((int *) nThreadP);

while (!(ExpandSegment (nDir) || m_scanData [!nDir].nLinkSeg))
	;
return 1;
}

#endif

// -----------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum nDepth of m_maxDepth.
//	Return the distance.

fix CRouter::Distance (CFixVector& m_p0, short m_nStartSeg, CFixVector& m_p1, short m_nDestSeg, int m_maxDepth, int m_widFlag, int nCacheType)
{
#if 0 //DBG
//if (!m_cacheType) 
	{
	m_cache [m_cacheType].SetPathLength (10000);
	return -1;
	}
#endif

if (m_nStartSeg < 0) {
	m_nStartSeg = FindSegByPos (m_p0, 0, 1, 0);
	if (m_nStartSeg < 0)
		return -1;
	}
if (m_nDestSeg < 0) {
	m_nDestSeg = FindSegByPos (m_p0, 0, 1, 0);
	if (m_nDestSeg < 0)
		return -1;
	}

// same segment?
if ((m_cacheType >= 0) && (m_nStartSeg == m_nDestSeg)) {
	m_cache [m_cacheType].SetPathLength (0);
	return CFixVector::Dist (m_p0, m_p1);
	}

// adjacent segments?
short nSide = SEGMENTS [m_nStartSeg].ConnectedSide (SEGMENTS + m_nDestSeg);
if ((nSide != -1) && (SEGMENTS [m_nDestSeg].IsDoorWay (nSide, NULL) & m_widFlag)) {
	m_cache [m_cacheType].SetPathLength (1);
	return CFixVector::Dist (m_p0, m_p1);
	}

#if USE_FCD_CACHE
if (m_cacheType >= 0) {
	fix xDist = m_cache [m_cacheType].Dist (m_nStartSeg, m_nDestSeg);
	if (xDist >= 0)
		return xDist;
	}
#endif

fix distance = Scan ();


if (m_cacheType >= 0) 
	m_cache [m_cacheType].Add (m_nStartSeg, m_nDestSeg, 10000, I2X (10000));


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

int CDACSBiDirRouter::Expand (int nDir)
{
	ushort nDist;

short nSegment = m_heap [nDir].Pop (nDist);
if ((nSegment < 0) || (nSegment == m_nDestSeg))
	return nSegment;
if (m_heap [!nDir].Popped (nSegment))
	return nSegment;
CSegment* segP = SEGMENTS + nSegment;
for (short nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
	if ((segP->m_children [nSide] >= 0) && (segP->IsDoorWay (nSide, NULL) & m_widFlag))
		m_heap [nDir].Push (segP->m_children [nSide], nSegment, nDist + segP->m_childDists [nSide]);
	}
return nSegment;
}

// -----------------------------------------------------------------------------

fix CDACSBiDirRouter::BuildRoute (void)

// -----------------------------------------------------------------------------

fix CDACSBiDirRouter::FindPath (void)

	short	nSegment;
#if DBG
	short	nExpanded = 0;
#endif

	short	nSegments [2] = {m_nStartSeg, m_nDestSeg};

m_heap [0].Setup (m_segments [0], m_nStartSeg);
m_heap [1].Setup (m_nDestSeg);

for (;;) {
	if (nSegments [0] >= 0) {
#if DBG
		nExpanded++;
#endif
		nSegments [0] = Expand (0);
		}
	if (nSegments [1] >= 0) {
#if DBG
		nExpanded++;
#endif
		nSegments [1] = Expand (1);
		}
	if ((nSegments [0] < 0) && (nSegments [1] < 0))
	if (nSegments [0] == m_nDestSeg) {
		nSegment = nSegments [0];
		nSegments [1] = -1;
		}
	else if (nSegments [1] == m_nStartSeg) {
		nSegment = nSegments [1];
		nSegments [0] = -1;
		}
	else if ((nSegments [1] >= 0) && m_heap [0].Popped (nSegments [1]))
		nSegment = nSegments [1];
	else if ((nSegments [0] >= 0) && m_heap [1].Popped (nSegments [0]))
		nSegment = nSegments [0];
	else
		continue;
	int j = -2;
	if (nSegments [0] >= 0)
		j += m_heap [0].BuildRoute (nSegment, 0, route) - 1;
	if (nSegments [1] >= 0)
		j += m_heap [1].BuildRoute (nSegment, 1, route + gameData.fcd.nConnSegDist);
	xDist = 0;
	for (int i = 1; i < j; i++)
		xDist += CFixVector::Dist (SEGMENTS [route [i]].Center (), SEGMENTS [route [i + 1]].Center ());
	xDist += CFixVector::Dist (m_p0, SEGMENTS [route [1]].Center ()) + CFixVector::Dist (m_p1, SEGMENTS [route [j]].Center ());
	return xDist;
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

fix CDACSUniDirRouter::FindPath (void)

	ushort		nDist;
	CSegment*	segP;

m_heap.Setup (m_nStartSeg);
#if DBG
int nExpanded = 0;
#endif
for (;;) {
	nSegment = m_heap.Pop (nDist);
	if (nSegment < 0)
		return -1;
	if (nSegment == m_nDestSeg) {
		int j = m_heap.BuildRoute (m_nDestSeg) - 2;
		short* route = m_heap.Route ();
		xDist = 0;
		for (int i = 1; i < j; i++)
			xDist += CFixVector::Dist (SEGMENTS [route [i]].Center (), SEGMENTS [route [i + 1]].Center ());
		xDist += CFixVector::Dist (m_p0, SEGMENTS [route [1]].Center ()) + CFixVector::Dist (m_p1, SEGMENTS [route [j]].Center ());
		return xDist;
		}
#if DBG
	nExpanded++;
#endif
	segP = SEGMENTS + nSegment;
	for (nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++) {
		if ((segP->m_children [nSide] >= 0) && (segP->IsDoorWay (nSide, NULL) & m_widFlag))
			m_heap.Push (segP->m_children [nSide], nSegment, nDist + segP->m_childDists [nSide]);
		}
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

fix CSimpleRouter::Scan (void)
{
if (!++m_scanInfo.bFlag) {
	memset (m_scanData, 0, sizeof (m_scanData));
	m_scanInfo.bFlag = 1;
	}

if (m_maxDepth < 0)
	m_maxDepth = gameData.segs.nSegments;

m_scanData [0].segPath [m_nStartSeg].bVisited = m_scanInfo.bFlag;
m_scanData [0].segPath [m_nStartSeg].nDepth = 0;
m_scanData [0].segPath [m_nStartSeg].nPred = -1;
m_scanData [0].segQueue [0] = m_nStartSeg;
m_scanData [0].nTail = 0;
m_scanData [0].nHead = 1;
m_scanData [0].nRouteDir = (m_cacheType == 0);
m_scanData [0].nLinkSeg = 0;

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



m_scanInfo.m_widFlag = m_widFlag;
m_scanInfo.nLinkSeg = 0;
m_scanInfo.bScanning = 3;

FindPath ();

return -1;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

fix CSimpleBiDirRouter::BuildPath (void)
{
	fix	xDist = 0;
	int	nLength = 0; 
	short	nPredSeg, nSuccSeg;

--m_scanInfo.nLinkSeg;
for (int nDir = 0; nDir < 2; nDir++) {
	tSegScanData& sd = m_scanData [nDir];
	nSuccSeg = m_scanInfo.nLinkSeg;
	for (;;) {
		nPredSeg = sd.segPath [nSuccSeg].nPred;
		if (nPredSeg == sd.m_nStartSeg) {
			xDist += CFixVector::Dist (nDir ? m_p1 : m_p0, SEGMENTS [nSuccSeg].Center ());
			break;
			}
		nLength++;
		if (nLength > 2 * m_scanInfo.m_maxDepth + 2)
			return -0x7FFFFFFF;
		xDist += CFixVector::Dist (SEGMENTS [nPredSeg].Center (), SEGMENTS [nSuccSeg].Center ());
		nSuccSeg = nPredSeg;
		}
	}
if (m_cacheType >= 0) 
	m_cache [m_cacheType].Add (m_scanData [0].m_nStartSeg, m_scanData [0].m_nDestSeg, nLength + 3, xDist);
return xDist;
}

// -----------------------------------------------------------------------------

fix CSimpleBiDirRouter::FindPath (void)
{
m_scanData [0].Setup (nStartSeg, nDestSeg, 0, m_scanInfo.bFlag);
m_scanData [1].Setup (nDestSeg, nStartSeg, 1, m_scanInfo.bFlag);

#if MULTITHREADED_SCAN
if (gameStates.app.nThreads > 1) {
	SDL_Thread* threads [2];
	int nThreadIds [2] = {0, 1};
	threads [0] = SDL_CreateThread (ExpandSegmentMT, nThreadIds);
	threads [1] = SDL_CreateThread (ExpandSegmentMT, nThreadIds + 1);
	SDL_WaitThread (threads [0], NULL);
	SDL_WaitThread (threads [1], NULL);

	if (((0 < (m_scanInfo.nLinkSeg = m_scanData [0].nLinkSeg)) && (m_scanInfo.nLinkSeg != m_scanData [0].m_nDestSeg + 1)) || 
		 ((0 < (m_scanInfo.nLinkSeg = m_scanData [1].nLinkSeg)) && (m_scanInfo.nLinkSeg != m_scanData [1].m_nDestSeg + 1)))
		return BuildPathBiDir (m_p0, m_p1, m_cacheType);
	}
else 
#endif
	{
	for (;;) {
		for (int nDir = 0; nDir < 2; nDir++) {
			if (!(m_scanInfo.nLinkSeg = m_scanData.Expand ()))
				continue;
			if (m_scanInfo.nLinkSeg < 0)
				return -1;
			// destination segment reached
			return BuildPath (m_p0, m_p1, m_cacheType);
			}
		}	
	}
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// uni-directional scanner

fix CSimpleUniDirRouter::BuildPathUniDir (CFixVector& m_p0, CFixVector& m_p1, int nCacheType)
{
	fix	xDist;
	int	nLength = 0; 
	short	nPredSeg, nSuccSeg;

nPredSeg = --m_scanInfo.nLinkSeg;
xDist = CFixVector::Dist (m_p1, SEGMENTS [nPredSeg].Center ());
for (;;) {
	nPredSeg = m_scanData [0].segPath [nSuccSeg].nPred;
	if (nPredSeg == m_scanData [0].m_nStartSeg)
		break;
	nLength++;
	xDist += CFixVector::Dist (SEGMENTS [nPredSeg].Center (), SEGMENTS [nSuccSeg].Center ());
	nSuccSeg = nPredSeg;
	}
xDist += CFixVector::Dist (m_p0, SEGMENTS [nSuccSeg].Center ());
if (m_cacheType >= 0) 
	m_cache [m_cacheType].Add (m_scanData [0].m_nStartSeg, m_scanData [0].m_nDestSeg, nLength + 3, xDist);
return xDist;
}

// -----------------------------------------------------------------------------

fix CSimpleUniDirRouter::Distance (CFixVector& m_p0, short m_nStartSeg, CFixVector& m_p1, short m_nDestSeg, int m_maxDepth, int m_widFlag, int nCacheType)
{
m_scanInfo.m_maxDepth = m_maxDepth;

for (;;) {
	if (0 > (m_scanInfo.nLinkSeg = ExpandSegment (nDir)))
		return -1;
	if (m_scanInfo.nLinkSeg > 0)
		// destination segment reached
		return BuildPathUniDir (m_p0, m_p1, m_cacheType);
	}	
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//eof
