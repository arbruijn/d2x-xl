/* $Id: mglobal.c,v 1.4 2003/10/10 09:36:35 btb Exp $ */
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
#	include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: mglobal.c,v 1.4 2003/10/10 09:36:35 btb Exp $";
#endif

#include "fix.h"
#include "vecmat.h"
#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "bm.h"
#include "3d.h"
#include "game.h"


// Global array of vertices, common to one mine.
//	This is the global mine which create_new_mine returns.
//lsegment	Lsegments[MAX_SEGMENTS];

// Number of vertices in current mine (ie, gameData.segs.vertices, pointed to by Vp)
//	Translate table to get opposite tSide of a face on a tSegment.
char	sideOpposite[MAX_SIDES_PER_SEGMENT] = {WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK};

#define TOLOWER(c) ((((c)>='A') && ((c)<='Z'))?((c)+('a'-'A')):(c))

#ifdef PASSWORD
#define encrypt(a,b,c,d)	a ^ TOLOWER((((int) PASSWORD)>>24)&255), \
									b ^ TOLOWER((((int) PASSWORD)>>16)&255), \
									c ^ TOLOWER((((int) PASSWORD)>>8)&255), \
									d ^ TOLOWER((((int) PASSWORD))&255)
#else
#define encrypt(a,b,c,d) a,b,c,d
#endif

sbyte sideToVerts[MAX_SIDES_PER_SEGMENT][4] = {
			{ encrypt(7,6,2,3) },			// left
			{ encrypt(0,4,7,3) },			// top
			{ encrypt(0,1,5,4) },			// right
			{ encrypt(2,6,5,1) },			// bottom
			{ encrypt(4,5,6,7) },			// back
			{ encrypt(3,2,1,0) },			// front
};	

//	Note, this MUST be the same as sideToVerts, it is an int for speed reasons.
int sideToVertsInt[MAX_SIDES_PER_SEGMENT][4] = {
			{ encrypt(7,6,2,3) },			// left
			{ encrypt(0,4,7,3) },			// top
			{ encrypt(0,1,5,4) },			// right
			{ encrypt(2,6,5,1) },			// bottom
			{ encrypt(4,5,6,7) },			// back
			{ encrypt(3,2,1,0) },			// front
};	

