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

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class CPathNode {
	public:
		uint		m_bVisited;
		short		m_nDepth;
		short		m_nPred;
	};

class CScanInfo {
	public:
		uint	m_bFlag;
		int	m_nMaxDepth;
		int	m_widFlag;
		short m_bScanning;
		short	m_nLinkSeg;
	};

class CScanData {
	public:
		CPathNode	m_path [MAX_SEGMENTS_D2X];
		short			m_queue [MAX_SEGMENTS_D2X];
		uint			m_bFlag;
		short			m_nStartSeg;
		short			m_nDestSeg;
		short			m_nLinkSeg;
		short			m_nHead;
		short			m_nTail;
		int			m_nDir;

		void Setup (short nStartSeg, short nDestSeg, uint flag, int dir);
		short Expand (CScanInfo& scanInfo);

	protected:
		virtual bool Match (short nSegment, short nDir) { return false; }
};

// -----------------------------------------------------------------------------

class CUniDirScanData : public CScanData {
	protected:
		virtual bool Match (short nSegment, short nDir);
};

// -----------------------------------------------------------------------------

class CBiDirScanData : public CScanData {
	protected:
		virtual bool Match (short nSegment, short nDir);
};

// -----------------------------------------------------------------------------

class CRouter {
	private:
		tSegScanInfo	m_scanInfo; // {0xFFFFFFFF, 0, 0, 3, -1}
		tSegScanData	m_scanData [2];
		CFCDCache		m_cache [2];
		CFixVector		m_p0, m_p1;
		short				m_nStartSeg, m_nDestSeg;
		int				m_maxDepth;
		int				m_widFlag;
		int				m_cacheType;

	public:
		fix Distance (CFixVector& p0, short nStartSeg, CFixVector& p1, short nDestSeg, int nMaxDepth, int widFlag, int nCacheType);

	protected:
		virtual fix Scan (void) { return -1; }
	};

// -----------------------------------------------------------------------------

class CSimpleRouter : public CRouter {
	protected:
		CScanInfo	m_scanInfo;

	protected:
		virtual fix Scan (void);
		virtual fix FindPath (void) { return -1; }
		short Expand (int nDir, CScanData& sd);
	};

// -----------------------------------------------------------------------------

class CSimpleUniDirRouter : public CSimpleRouter {
	private:
		CScanData	m_scanData;

	private:
		fix BuildPath (void);

	protected:
		virtual fix FindPath (void);
		virtual bool Match (short nSegment, short nDir);
};

// -----------------------------------------------------------------------------

class CSimpleBiDirRouter : public CSimpleRouter {
	private:
		CScanData	m_scanData [2];

	private:
		fix BuildPath (void);

	protected:
		virtual fix FindPath (void);
		virtual bool Match (short nSegment, short nDir);
};

// -----------------------------------------------------------------------------

class CDACSRouter : public CRouter {
	protected:
		virtual fix Scan (void);
		virtual fix FindPath (void) { return -1; }
	};

// -----------------------------------------------------------------------------

class CDACSUniDirRouter : public CDACSRouter {
	private:
		CDialHeap	m_heap [2];

	private:
		fix BuildPath (short nSegment);

	protected:
		virtual fix FindPath (void);
};

// -----------------------------------------------------------------------------

class CDACSBiDirRouter : public CDACSRouter {
	private:
		CDialHeap	m_heap [2];
		short			m_route [2 * MAX_SEGMENTS_D2X];
		short			m_nSegments [2];

	private:
		int Expand (int nDir);
		fix BuildPath (short nSegment);

	protected:
		virtual fix FindPath (void);
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------


#endif


