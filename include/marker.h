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

#define NUM_MARKERS         (MAX_PLAYERS * 3)
#define MARKER_MESSAGE_LEN  40

class CMarkerData {
	public:
		CStaticArray< CFixVector, NUM_MARKERS >	position; // [NUM_MARKERS];	//three markers (two regular + one spawn) per player in multi
		char					szMessage [NUM_MARKERS][MARKER_MESSAGE_LEN];
		char					nOwner [NUM_MARKERS][CALLSIGN_LEN+1];
		CStaticArray< short, NUM_MARKERS >			objects; // [NUM_MARKERS];
		short					viewers [2];
		int					nHighlight;
		float					fScale;
		ubyte					nDefiningMsg;
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

	private:
		void DrawNumber (int nMarker);
		inline CObject *GetObject (int nPlayer, int nMarker);
};

extern CMarkerManager markerManager;

// -------------------------------------------------------------

#endif //_MARKER_H
