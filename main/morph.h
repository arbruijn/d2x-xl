/* $Id: morph.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

/*
 *
 * Header for morph.c
 *
 * Old Log:
 * Revision 1.2  1995/08/23  21:35:58  allender
 * fix mcc compiler warnings
 *
 * Revision 1.1  1995/05/16  15:59:37  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:19  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.9  1995/01/04  12:20:46  john
 * Declearations to work better with game state save.
 *
 *
 * Revision 1.8  1995/01/03  20:38:44  john
 * Externed MAX_OBJECTS
 *
 * Revision 1.7  1994/09/26  17:28:33  matt
 * Made new multiple-tObject morph code work with the demo system
 *
 * Revision 1.6  1994/09/26  15:40:17  matt
 * Allow multiple simultaneous morphing objects
 *
 * Revision 1.5  1994/06/28  11:55:19  john
 * Made newdemo system record/play directly to/from disk, so
 * we don't need the 4 MB buffer anymore.
 *
 * Revision 1.4  1994/06/16  13:57:40  matt
 * Added support for morphing objects in demos
 *
 * Revision 1.3  1994/06/08  18:22:03  matt
 * Made morphing objects light correctly
 *
 * Revision 1.2  1994/05/30  22:50:25  matt
 * Added morph effect for robots
 *
 * Revision 1.1  1994/05/30  12:04:19  matt
 * Initial revision
 *
 *
 */


#ifndef _H
#define _H

#include "object.h"

#define MAX_VECS 200

typedef struct tMorphInfo {
	tObject			*objP;                                // tObject which is morphing
	vmsVector		vecs [MAX_VECS];
	vmsVector		deltas [MAX_VECS];
	fix				times [MAX_VECS];
	int				submodelActive [MAX_SUBMODELS];         // which submodels are active
	int				nMorphingPoints [MAX_SUBMODELS];       // how many active points in each part
	int				submodelStartPoints [MAX_SUBMODELS];    // first point for each submodel
	int				nSubmodelsActive;
	ubyte				saveControlType;
	ubyte				saveMovementType;
	tPhysicsInfo	savePhysInfo;
	int				nSignature;
} tMorphInfo;

#define MAX_MORPH_OBJECTS 250

#define MORPH_RATE (f1_0*3)

void MorphStart(tObject *obj);
void MorphDrawObject(tObject *obj);

//process the morphing tObject for one frame
void DoMorphFrame(tObject *obj);

//called at the start of a level
void MorphInit();

extern tMorphInfo *MorphFindData (tObject *obj);

#endif /* _H */
