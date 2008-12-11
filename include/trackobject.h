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

#ifndef _TRACKOBJECT_H
#define _TRACKOBJECT_H

// Constants governing homing missile behavior.
// MIN_TRACKABLE_DOT gets inversely scaled by FrameTime and stuffed in
// xMinTrackableDot
#define MIN_TRACKABLE_DOT          (7 * F1_0 / 8)
#define MAX_TRACKABLE_DIST         (F1_0 * 250)
#define HOMINGMSL_STRAIGHT_TIME    (F1_0 / 8)    //  Changed as per request of John, Adam, Yuan, but mostly John

extern fix xMinTrackableDot;   //  MIN_TRACKABLE_DOT inversely scaled by FrameTime

int CallFindHomingObjectComplete (CObject *tracker, CFixVector *curpos);
int FindHomingObject (CFixVector *curpos, CObject *trackerP);
int FindHomingObjectComplete (CFixVector *curpos, CObject *tracker, int track_objType1, int track_objType2);
int TrackHomingTarget (int nHomingTarget, CObject *tracker, fix *dot);

#endif //_TRACKOBJECT_H
