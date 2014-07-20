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

#ifndef _AUTOMAP_H
#define _AUTOMAP_H

#include "carray.h"
#include "player.h"

#define AM_SHOW_PLAYERS				(!IsMultiGame || IsTeamGame || IsCoopGame)
#define AM_SHOW_PLAYER(_i)			(!IsMultiGame || \
											 IsCoopGame || \
											 (netGameInfo.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP) || \
											 (GetTeam (N_LOCALPLAYER) == GetTeam (_i)))
#define AM_SHOW_ROBOTS				EGI_FLAG (bRobotsOnRadar, 0, 1, 0)
#if DBG == 1
#	define AM_SHOW_POWERUPS(_i)	(EGI_FLAG (bPowerupsOnRadar, 0, 1, 0) >= (_i))
#else
#	define AM_SHOW_POWERUPS(_i)	((EGI_FLAG (bPowerupsOnRadar, 0, 1, 0) >= (_i)) && (!IsMultiGame || IsCoopGame))
#endif

//------------------------------------------------------------------------------

#define MAX_EDGES 65536 // Determined by loading all the levels by John & Mike, Feb 9, 1995

typedef struct tEdgeInfo {
	uint16_t	verts [2];     // 4 bytes
	uint8_t		sides [4];     // 4 bytes
	int16_t		nSegment [4];  // 8 bytes  // This might not need to be stored... If you can access the normals of a CSide.
	uint32_t		color;			// 4 bytes
	uint8_t		nFaces;			// 1 bytes  // 19 bytes...
	uint8_t		flags;			// 1 bytes  // See the EF_??? defines above.
} __pack__ tEdgeInfo;

//------------------------------------------------------------------------------

typedef struct tAutomapWallColors {
	uint32_t	nNormal;
	uint32_t	nDoor;
	uint32_t	nDoorBlue;
	uint32_t	nDoorGold;
	uint32_t	nDoorRed;
	uint32_t	nRevealed;
} tAutomapWallColors;

typedef struct tAutomapColors {
	tAutomapWallColors	walls;
	uint32_t					nHostage;
	uint32_t					nMonsterball;
	uint32_t					nWhite;
	uint32_t					nMedGreen;
	uint32_t					nLgtBlue;
	uint32_t					nLgtRed;
	uint32_t					nDkGray;
} __pack__ tAutomapColors;

typedef struct tAutomapData {
	int32_t					bCheat;
	int32_t					bHires;
	fix						nViewDist;
	fix						nMaxDist;
	fix						nZoom;
	tObjTransformation	viewer;
	CFixVector				viewTarget;
} __pack__ tAutomapData;

//------------------------------------------------------------------------------

class CAutomap {
	private:
		tAutomapData			m_data;
		tAutomapColors			m_colors;
		CArray<tEdgeInfo>		m_edges;
		CArray<tEdgeInfo*>	m_brightEdges;
		int32_t					m_nEdges;
		int32_t					m_nMaxEdges;
		int32_t					m_nLastEdge;
		int32_t					m_nWidth;
		int32_t					m_nHeight;
		int32_t					m_bFade;
		int32_t					m_nColor;
		float						m_fScale;
		CFloatVector			m_color;
		int32_t					m_bChaseCam;
		int32_t					m_bFreeCam;
		char						m_szLevelNum [200];
		char						m_szLevelName [200];
		CAngleVector			m_vTAngles;
		bool						m_bDrawBuffers;
		int32_t					m_nVerts;

	public:
		CArray<uint16_t>		m_visited;
		CArray<uint16_t>		m_visible;
		int32_t					m_bRadar;
		bool						m_bFull;
		int32_t					m_bActive;
		int32_t					m_nSegmentLimit;
		int32_t					m_nMaxSegsAway;

	public:
		CAutomap () { Init (); }
		~CAutomap () {}
		void Init (void);
		void InitColors (void);
		int32_t Setup (int32_t bPauseGame, fix& xEntryTime);
		int32_t Update (void);
		void Render (fix xStereoSeparation = 0);
		void RenderInfo (void);
		void DoFrame (int32_t nKeyCode, int32_t bRadar);
		void ClearVisited (void);
		int32_t ReadControls (int32_t nLeaveMode, int32_t bDone, int32_t& bPauseGame);

		inline int32_t Radar (void) { return m_bRadar; }
		inline int32_t SegmentLimit (void) { return m_nSegmentLimit; }
		inline int32_t MaxSegsAway (void) { return m_nMaxSegsAway; }
		inline int32_t Visible (int32_t nSegment) { return m_bFull || m_visited [nSegment] || (OBSERVING && IsMultiGame && !IsCoopGame); }
		int32_t Active (void);
		inline void SetActive (int32_t bActive) { m_bActive = bActive; }

	private:
		int32_t SetSegmentDepths (int32_t nStartSeg, uint16_t *depthBufP);
		void InitView (void);
		void AdjustSegmentLimit (void);
		void DrawEdges (void);
		void DrawPlayer (CObject* objP);
		void DrawObjects (void);
		void DrawLevelId (void);
		void CreateNameCanvas (void);
		int32_t GameFrame (int32_t bPauseGame, int32_t bDone);
		int32_t FindEdge (int32_t v0, int32_t v1, tEdgeInfo*& edgeP);
		void BuildEdgeList (void);
		void AddEdge (int32_t va, int32_t vb, uint32_t color, uint8_t CSide, int16_t nSegment, int32_t bHidden, int32_t bGrate, int32_t bNoFade);
		void AddUnknownEdge (int32_t va, int32_t vb);
		void AddSegmentEdges (CSegment *segP);
		void AddUnknownSegmentEdges (CSegment* segP);
		void SetEdgeColor (int32_t nColor, int32_t bFade, float fScale = 1.e10f);
		void DrawLine (int16_t v0, int16_t v1);
		int32_t Texturing (void);

};

//------------------------------------------------------------------------------

extern CAutomap automap;

//------------------------------------------------------------------------------

#endif //_AUTOMAP_H
