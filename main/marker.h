/* $Id: automap.h,v 1.4 2003/11/15 00:36:54 btb Exp $ */
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

// -------------------------------------------------------------

static inline int MaxDrop (void)
{
return IsMultiGame ? IsCoopGame ? MAX_DROP_COOP : MAX_DROP_MULTI : MAX_DROP_SINGLE;
}

// -------------------------------------------------------------

void DropBuddyMarker (tObject *objP);
void DropSpawnMarker (void);
void DrawMarkers (void);
void DeleteMarker (int bForce);
void TeleportToMarker (void);
void ClearMarkers (void);
int LastMarker (void);
void InitMarkerInput (void);
void MarkerInputMessage (int key);
int SpawnMarkerIndex (int nPlayer);
tObject *SpawnMarkerObject (int nPlayer);
int IsSpawnMarkerObject (tObject *objP);
int MoveSpawnMarker (tPosition *posP, short nSegment);

// -------------------------------------------------------------

#endif //_MARKER_H
