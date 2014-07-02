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

#ifndef _FINDPATH_H
#define _FINDPATH_H

#include "descent.h"
#include "dialheap.h"

// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------

typedef struct {
	int16_t	seg0, seg1;
	int32_t	pathLen;
	fix	dist;
} tFCDCacheData;

#define	MAX_FCD_CACHE	128

class CFCDCache {
	public:	
		int32_t				m_nIndex;
		CStaticArray< tFCDCacheData, MAX_FCD_CACHE >	m_cache; // [MAX_FCD_CACHE];
		fix				m_xLastFlushTime;
		fix				m_nPathLength;

		void Flush (void);
		void Add (int32_t seg0, int32_t seg1, int32_t nDepth, fix dist);
		fix Dist (int16_t seg0, int16_t seg1);
		inline void SetPathLength (fix nPathLength) { m_nPathLength = nPathLength; }
		inline fix GetPathLength (void) { return m_nPathLength; }
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class CPathNode {
	public:
		uint32_t		m_bVisited;
		int16_t		m_nDepth;
		int16_t		m_nPred;
		int16_t		m_nEdge;
	};

class CScanInfo;

class CSimpleHeap {
	public:
		CPathNode	m_path [MAX_SEGMENTS_D2X];
		int16_t			m_queue [MAX_SEGMENTS_D2X];
		uint32_t			m_bFlag;
		int16_t			m_nStartSeg;
		int16_t			m_nDestSeg;
		int16_t			m_nLinkSeg;
		int16_t			m_nHead;
		int16_t			m_nTail;
		int32_t			m_nDir;
		int32_t			m_nMaxDist;

		void Setup (int16_t nStartSeg, int16_t nDestSeg, uint32_t flag, int32_t dir);
		int16_t Expand (CScanInfo& scanInfo);

	protected:
		virtual bool Match (int16_t nSegment, CScanInfo& scanInfo) { return false; }
};

// -----------------------------------------------------------------------------

class CScanInfo {
	public:
		uint32_t				m_bFlag;
		int32_t				m_maxDist;
		int32_t				m_widFlag;
		int16_t 			m_bScanning;
		int16_t				m_nLinkSeg;
		CSimpleHeap*	m_heap;

		int32_t Setup (CSimpleHeap* heap, int32_t nWidFlag, int32_t nMaxDist);
		inline int32_t Scanning (int32_t nDir) {
			m_bScanning &= ~(1 << nDir);
			return m_bScanning;
			}
	};

// -----------------------------------------------------------------------------

class CSimpleUniDirHeap : public CSimpleHeap {
	protected:
		virtual bool Match (int16_t nSegment, CScanInfo& scanInfo);
};

// -----------------------------------------------------------------------------

class CSimpleBiDirHeap : public CSimpleHeap {
	protected:
		virtual bool Match (int16_t nSegment, CScanInfo& scanInfo);
};

// -----------------------------------------------------------------------------

class CRouter {
	protected:
		CFCDCache	m_cache [2];
		CFixVector	m_p0, m_p1;
		int16_t			m_nStartSeg;
		int16_t			m_nDestSeg;
		int32_t			m_maxDist;
		int32_t			m_widFlag;
		int32_t			m_cacheType;
		int32_t			m_nNodes;

	public:
		explicit CRouter () : m_nNodes (0) {}

		virtual bool Create (int32_t nNodes) { return true; }

		fix PathLength (const CFixVector& p0, const int16_t nStartSeg, const CFixVector& p1, 
							 const int16_t nDestSeg, const int32_t nMaxDist, const int32_t widFlag, const int32_t nCacheType);

		void Flush (void) {
			m_cache [0].Flush ();
			m_cache [1].Flush ();
			};

		virtual fix Distance (int16_t nSegment) { return -1; }

		inline int32_t StartSeg (void) { return m_nStartSeg; }
		inline int32_t DestSeg (void) { return m_nDestSeg; }
		inline void SetStartSeg (int16_t nSegment) { m_nStartSeg = nSegment; }
		inline void SetDestSeg (int16_t nSegment) { m_nDestSeg = nSegment; }
		inline void SetSize (int32_t nNodes) { m_nNodes = nNodes; }
		inline int32_t Size (void) { return m_nNodes; }

	protected:
		int32_t SetSegment (const int16_t nSegment, const CFixVector& p);
		virtual fix FindPath (void) { return -1; }
	};

// -----------------------------------------------------------------------------

class CSimpleRouter : public CRouter {
	protected:
		CScanInfo	m_scanInfo;

	protected:
		virtual fix FindPath (void) { return -1; }
	};

// -----------------------------------------------------------------------------

class CSimpleUniDirRouter : public CSimpleRouter {
	private:
		CSimpleUniDirHeap	m_heap;

	private:
		fix BuildPath (void);

	protected:
		virtual fix FindPath (void);
};

// -----------------------------------------------------------------------------

class CSimpleBiDirRouter : public CSimpleRouter {
	private:
		CSimpleBiDirHeap	m_heap [2];

	private:
		fix BuildPath (void);

	protected:
		virtual fix FindPath (void);
};

// -----------------------------------------------------------------------------

class CDACSRouter : public CRouter {
	protected:
		virtual fix FindPath (void) { return -1; }
	};

// -----------------------------------------------------------------------------

class CDACSUniDirRouter : public CDACSRouter {
	private:
		CDialHeap	m_heap;

	private:
		fix BuildPath (int16_t nSegment);

	protected:
		virtual fix FindPath (void);

	public:
		virtual bool Create (int32_t nNodes);

		virtual fix Distance (int16_t nSegment);

		inline int16_t RouteLength (int16_t nNode) { return m_heap.RouteLength (nNode); }
		inline CDialHeap::tPathNode* Route (uint32_t i = 0) { return m_heap.Route (i); }

};

// -----------------------------------------------------------------------------

class CDACSBiDirRouter : public CDACSRouter {
	private:
		CDialHeap				m_heap [2];
		CDialHeap::tPathNode	m_route [2 * MAX_SEGMENTS_D2X];
		int16_t						m_nSegments [2];

	private:
		int32_t Expand (int32_t nDir);
		fix BuildPath (int16_t nSegment);

	protected:
		virtual fix FindPath (void);

	public:
		virtual bool Create (int32_t nNodes);
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

extern CSimpleBiDirRouter simpleRouter [];
extern CDACSUniDirRouter uniDacsRouter [];
extern CDACSBiDirRouter biDacsRouter [];

// -----------------------------------------------------------------------------

#endif


