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

#ifndef _MARKER_H
#define _MARKER_H

#include "player.h"

#define	MAX_DROP_MULTI		2
#define	MAX_DROP_COOP		3
#define	MAX_DROP_SINGLE	9

//------------------------------------------------------------------------------

#define NUM_MARKERS        (MAX_PLAYERS * 3)
#define NUM_MARKERS_D2		16
#define MARKER_MESSAGE_LEN	40

class CMarkerData {
	public:
		CStaticArray< CFixVector, NUM_MARKERS >	position; // [NUM_MARKERS];	//three markers (two regular + one spawn) per player in multi
		char					szMessage [NUM_MARKERS][MARKER_MESSAGE_LEN];
		char					szOwner [NUM_MARKERS][CALLSIGN_LEN+1];
		CStaticArray< short, NUM_MARKERS >			objects; // [NUM_MARKERS];
		short					viewers [2];
		int					nHighlight;
		float					fScale;
		bool					bDefiningMsg;
		char					szInput [40];
		int					nIndex;
		int					nCurrent;
		int					nLast;
		bool					bRotate;

	public:
		CMarkerData ();
		void Init (void);
};

// -------------------------------------------------------------

class CMarkerManager {
	private:
		CMarkerData	m_data;

	public:
		CMarkerManager () {}
		~CMarkerManager () {}
		void Init (void) { m_data.Init (); }
		void Drop (char nPlayerMarker, int bSpawn);
		void DropForGuidebot (CObject *objP);
		void DropSpawnPoint (void);
		void Render (void);
		void Rotate (void);
		void Delete (int bForce);
		void Teleport (void);
		void Clear (void);
		int Last (void);
		void InitInput (bool bRotate = false);
		void InputMessage (int key);
		int SpawnIndex (int nPlayer);
		CObject *SpawnObject (int nPlayer);
		int IsSpawnObject (CObject *objP);
		int MoveSpawnPoint (tObjTransformation *posP, short nSegment);
		inline int MaxDrop (void) {
			return IsMultiGame ? IsCoopGame ? MAX_DROP_COOP : MAX_DROP_MULTI : MAX_DROP_SINGLE;
			}

		inline int Highlight (void) { return m_data.nHighlight; }
		inline int SetHighlight (int nHighlight) { 
			m_data.nHighlight = nHighlight; 
			return m_data.nHighlight;
			}
		inline char* Owner (int nMarker) { return m_data.szOwner [nMarker]; }
		inline void SetOwner (int nMarker, char* szOwner) { strcpy (m_data.szOwner [nMarker], szOwner); }
		inline char* Message (int nMarker = -1) { return m_data.szMessage [(nMarker < 0) ? m_data.nHighlight : nMarker]; }
		inline short Objects (int nMarker) { return m_data.objects [nMarker]; }
		inline void SetObject (int nMarker, int nObject) { m_data.objects [nMarker] = nObject; }
		inline bool DefiningMsg (void) { return m_data.bDefiningMsg; }
		inline char* Input (void) { return m_data.szInput; }
		inline short Viewer (int nWindow) { return m_data.viewers [nWindow]; }
		inline void SetViewer (int nWindow, short nViewer) { m_data.viewers [nWindow] = nViewer; }
		inline CFixVector Position (int nMarker) { return m_data.position [nMarker]; }
		inline void SetPosition (int nMarker, CFixVector vPos) { m_data.position [nMarker] = vPos; }

		void SaveState (CFile& cf);
		void LoadState (CFile& cf, bool bBinary = false);

	private:
		void DrawNumber (int nMarker);
		inline CObject *GetObject (int nPlayer, int nMarker);
};

extern CMarkerManager markerManager;

// -------------------------------------------------------------

#endif //_MARKER_H
