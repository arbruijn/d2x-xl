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
#define MULTITHREADED_SCAN 1

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

#if MULTITHREADED_SCAN

int _CDECL_ ExpandSegmentMT (void* nThreadP)
{
	int nDir = *((int *) nThreadP);

while (!(ExpandSegment (nDir) || scanData [!nDir].nLinkSeg))
	;
return 1;
}

#endif

// -----------------------------------------------------------------------------

#if BIDIRECTIONAL_SCAN 

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
		if (nLength > 2 * scanInfo.nMaxDepth + 2)
			return -0x7FFFFFFF;
		xDist += CFixVector::Dist (SEGMENTS [nPredSeg].Center (), SEGMENTS [nSuccSeg].Center ());
		nSuccSeg = nPredSeg;
		}
	}
if (nCacheType >= 0) 
	fcdCaches [nCacheType].Add (scanData [0].nStartSeg, scanData [0].nDestSeg, nLength + 3, xDist);
return xDist;
}

#else //	-----------------------------------------------------------------------

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

#endif 

// -----------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum nDepth of nMaxDepth.
//	Return the distance.

fix PathLength (CFixVector& p0, short nStartSeg, CFixVector& p1, short nDestSeg, int nMaxDepth, int widFlag, int nCacheType)
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

#if USE_FCD_CACHE
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
scanInfo.nLinkSeg = 0;
scanInfo.bScanning = 3;

#if BIDIRECTIONAL_SCAN

scanInfo.nMaxDepth = nMaxDepth / 2 + 1;

// bi-directional scanner (expands about 1/3 of the segments the uni-directional scanner does on average)

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

#if MULTITHREADED_SCAN
if (gameStates.app.nThreads > 1) {
	SDL_Thread* threads [2];
	int nThreadIds [2] = {0, 1};
	threads [0] = SDL_CreateThread (ExpandSegmentMT, nThreadIds);
	threads [1] = SDL_CreateThread (ExpandSegmentMT, nThreadIds + 1);
	SDL_WaitThread (threads [0], NULL);
	SDL_WaitThread (threads [1], NULL);

	if (((0 < (scanInfo.nLinkSeg = scanData [0].nLinkSeg)) && (scanInfo.nLinkSeg != scanData [0].nDestSeg + 1)) || 
		 ((0 < (scanInfo.nLinkSeg = scanData [1].nLinkSeg)) && (scanInfo.nLinkSeg != scanData [1].nDestSeg + 1)))
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

//	-----------------------------------------------------------------------------
//eof
