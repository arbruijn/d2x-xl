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
		CStaticArray< int16_t, NUM_MARKERS >			objects; // [NUM_MARKERS];
		int16_t					viewers [2];
		int32_t					nHighlight;
		float					fScale;
		bool					bDefiningMsg;
		char					szInput [40];
		int32_t					nIndex;
		int32_t					nCurrent;
		int32_t					nLast;
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
		void Drop (char nPlayerMarker, int32_t bSpawn);
		void DropForGuidebot (CObject *objP);
		void DropSpawnPoint (void);
		void Render (void);
		void Rotate (void);
		void Delete (int32_t bForce);
		void Teleport (void);
		void Clear (void);
		int32_t Last (void);
		void InitInput (bool bRotate = false);
		void InputMessage (int32_t key);
		int32_t SpawnIndex (int32_t nPlayer);
		CObject *SpawnObject (int32_t nPlayer);
		int32_t IsSpawnObject (CObject *objP);
		int32_t MoveSpawnPoint (tObjTransformation *posP, int16_t nSegment);
		inline int32_t MaxDrop (void) {
			return IsMultiGame ? IsCoopGame ? MAX_DROP_COOP : MAX_DROP_MULTI : MAX_DROP_SINGLE;
			}

		inline int32_t Highlight (void) { return m_data.nHighlight; }
		inline int32_t SetHighlight (int32_t nHighlight) { 
			m_data.nHighlight = nHighlight; 
			return m_data.nHighlight;
			}
		inline char* Owner (int32_t nMarker) { return m_data.szOwner [nMarker]; }
		inline void SetOwner (int32_t nMarker, char* szOwner) { strcpy (m_data.szOwner [nMarker], szOwner); }
		inline char* Message (int32_t nMarker = -1) { return m_data.szMessage [(nMarker < 0) ? m_data.nHighlight : nMarker]; }
		inline int16_t Objects (int32_t nMarker) { return m_data.objects [nMarker]; }
		inline void SetObject (int32_t nMarker, int32_t nObject) { m_data.objects [nMarker] = nObject; }
		inline bool DefiningMsg (void) { return m_data.bDefiningMsg; }
		inline char* Input (void) { return m_data.szInput; }
		inline int16_t Viewer (int32_t nWindow) { return m_data.viewers [nWindow]; }
		inline void SetViewer (int32_t nWindow, int16_t nViewer) { m_data.viewers [nWindow] = nViewer; }
		inline CFixVector Position (int32_t nMarker) { return m_data.position [nMarker]; }
		inline void SetPosition (int32_t nMarker, CFixVector vPos) { m_data.position [nMarker] = vPos; }

		void SaveState (CFile& cf);
		void LoadState (CFile& cf, bool bBinary = false);

	private:
		void DrawNumber (int32_t nMarker);
		inline CObject *GetObject (int32_t nPlayer, int32_t nMarker);
};

extern CMarkerManager markerManager;

// -------------------------------------------------------------

#endif //_MARKER_H
