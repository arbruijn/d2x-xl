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

#define AM_SHOW_PLAYERS			(!IsMultiGame || (gameData.app.nGameMode & (GM_TEAM | GM_MULTI_COOP)) || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))
#define AM_SHOW_PLAYER(_i)		(!IsMultiGame || \
										 (gameData.app.nGameMode & GM_MULTI_COOP) || \
										 (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP) || \
										 (GetTeam (gameData.multiplayer.nLocalPlayer) == GetTeam (_i)))
#define AM_SHOW_ROBOTS			EGI_FLAG (bRobotsOnRadar, 0, 1, 0)
#define AM_SHOW_POWERUPS(_i)	((EGI_FLAG (bPowerupsOnRadar, 0, 1, 0) >= (_i)) && (!IsMultiGame || IsCoopGame))

//------------------------------------------------------------------------------

#define MAX_EDGES 65536 // Determined by loading all the levels by John & Mike, Feb 9, 1995

typedef struct tEdgeInfo {
	short		verts [2];     // 4 bytes
	ubyte		sides [4];     // 4 bytes
	short		nSegment [4];  // 8 bytes  // This might not need to be stored... If you can access the normals of a CSide.
	ubyte		flags;			// 1 bytes  // See the EF_??? defines above.
	uint		color;			// 4 bytes
	ubyte		nFaces;			// 1 bytes  // 19 bytes...
} tEdgeInfo;

//------------------------------------------------------------------------------

typedef struct tAutomapWallColors {
	uint	nNormal;
	uint	nDoor;
	uint	nDoorBlue;
	uint	nDoorGold;
	uint	nDoorRed;
	uint	nRevealed;
} tAutomapWallColors;

typedef struct tAutomapColors {
	tAutomapWallColors	walls;
	uint						nHostage;
	uint						nMonsterball;
	uint						nWhite;
	uint						nMedGreen;
	uint						nLgtBlue;
	uint						nLgtRed;
	uint						nDkGray;
} tAutomapColors;

typedef struct tAutomapData {
	int			bCheat;
	int			bHires;
	fix			nViewDist;
	fix			nMaxDist;
	fix			nZoom;
	CFixVector	viewPos;
	CFixVector	viewTarget;
	CFixMatrix	viewMatrix;
} tAutomapData;

//------------------------------------------------------------------------------

class CAutomap {
	private:
		tAutomapData			m_data;
		tAutomapColors			m_colors;
		CArray<tEdgeInfo>		m_edges;
		CArray<tEdgeInfo*>	m_brightEdges;
		int						m_nEdges;
		int						m_nMaxEdges;
		int						m_nLastEdge;
		int						m_nWidth;
		int						m_nHeight;
		char						m_szLevelNum [200];
		char						m_szLevelName [200];
		CBitmap					m_background;

	public:
		CArray<ushort>			m_visited [2];
		CArray<ushort>			m_visible;
		int						m_bRadar;
		bool						m_bFull;
		bool						m_bDisplay;
		int						m_nSegmentLimit;
		int						m_nMaxSegsAway;

	public:
		CAutomap () { Init (); }
		~CAutomap () {}
		void Init (void);
		void InitColors (void);
		bool InitBackground (void);
		int Setup (int bPauseGame, fix& xEntryTime, CAngleVector& vTAngles);
		int Update (CAngleVector& vTAngles);
		void Draw (void);
		void DoFrame (int nKeyCode, int bRadar);
		void ClearVisited (void);
		int ReadControls (int nLeaveMode, int bDone, int& bPauseGame);

		inline int Radar (void) { return m_bRadar; }
		inline int SegmentLimit (void) { return m_nSegmentLimit; }
		inline int MaxSegsAway (void) { return m_nMaxSegsAway; }

	private:
		void AdjustSegmentLimit (int nSegmentLimit, CArray<ushort>& visited);
		void DrawEdges (void);
		void DrawPlayer (CObject* objP);
		void DrawObjects (void);
		void DrawLevelId (void);
		void CreateNameCanvas (void);
		int GameFrame (int bPauseGame, int bDone);
		int FindEdge (int v0, int v1, tEdgeInfo*& edgeP);
		void BuildEdgeList (void);
		void AddEdge (int va, int vb, uint color, ubyte CSide, short nSegment, int bHidden, int bGrate, int bNoFade);
		void AddUnknownEdge (int va, int vb);
		void AddSegmentEdges (CSegment *segP);
		void AddUnknownSegmentEdges (CSegment* segP);
};

//------------------------------------------------------------------------------

extern CAutomap automap;

//------------------------------------------------------------------------------

#endif //_AUTOMAP_H
