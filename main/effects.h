/* $Id: effects.h,v 1.4 2003/10/10 09:36:35 btb Exp $ */
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
 * Headerfile for effects.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:56:08  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:34  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.15  1994/11/08  21:12:07  matt
 * Added functions to stop & start effects
 *
 * Revision 1.14  1994/10/13  17:14:11  adam
 * MAX_EFFECTS to 60 (ugh)
 *
 * Revision 1.13  1994/10/05  10:14:34  adam
 * MAX_EFFECTS to 50
 *
 * Revision 1.12  1994/10/04  18:59:09  matt
 * Exploding eclips now play eclip while exploding, then switch to static bm
 *
 * Revision 1.11  1994/10/04  15:17:52  matt
 * Took out references to unused constant
 *
 * Revision 1.10  1994/09/29  14:15:00  matt
 * Added sounds for eclips (wall effects)
 *
 * Revision 1.9  1994/09/25  00:40:24  matt
 * Added the ability to make eclips (monitors, fans) which can be blown up
 *
 * Revision 1.8  1994/08/05  15:55:25  matt
 * Cleaned up effects system, and added alternate effects for after mine
 * destruction.
 *
 * Revision 1.7  1994/08/01  23:17:20  matt
 * Add support for animating textures on robots
 *
 * Revision 1.6  1994/05/19  18:13:18  yuan
 * MAX_EFFECTS increased to 30
 *
 * Revision 1.5  1994/03/15  16:32:37  yuan
 * Cleaned up bm-loading code.
 * (Fixed structures too)
 *
 * Revision 1.4  1994/03/04  17:09:07  yuan
 * New door stuff.
 *
 * Revision 1.3  1994/01/19  18:22:45  yuan
 * Changed number of effects from 10-20
 *
 * Revision 1.2  1994/01/11  10:39:07  yuan
 * Special effects new implementation
 *
 * Revision 1.1  1994/01/10  10:36:14  yuan
 * Initial revision
 *
 *
 */


#ifndef _EFFECTS_H
#define _EFFECTS_H

#include "vclip.h"

#define MAX_EFFECTS 110
#define D1_MAX_EFFECTS 60

//flags for eclips.  If no flags are set, always plays

#define EF_CRITICAL		1   //this doesn't get played directly (only when mine critical)
#define EF_ONE_SHOT		2   //this is a special that gets played once
#define EF_STOPPED		4   //this has been stopped
#define EF_ALTFMT			8
#define EF_FROMPOG		16
#define EF_INITIALIZED	32

typedef struct eclip {
	tVideoClip   vc;             //embedded tVideoClip
	fix     time_left;      //for sequencing
	int     nCurFrame;      //for sequencing
	short   changingWallTexture;      //Which element of Textures array to replace.
	short   changingObjectTexture;    //Which element of ObjBitmapPtrs array to replace.
	int     flags;          //see above
	int     crit_clip;      //use this clip instead of above one when mine critical
	int     dest_bm_num;    //use this bitmap when monitor destroyed
	int     dest_vclip;     //what tVideoClip to play when exploding
	int     dest_eclip;     //what eclip to play when exploding
	fix     dest_size;      //3d size of explosion
	int     nSound;      //what sound this makes
	int     nSegment,nSide; //what seg & tSide, for one-shot clips
} __pack__ eclip;

typedef eclip D1_eclip;

extern int Num_effects [2];
extern eclip Effects [2][MAX_EFFECTS];

// Set up special effects.
extern fix EffectFrameTime (eclip *ec);

// Set up special effects.
extern void InitSpecialEffects();

// Clear any active one-shots
void ResetSpecialEffects();

// Function called in game loop to do effects.
extern void DoSpecialEffects();

// Restore bitmap
extern void RestoreEffectBitmapIcons();

//stop an effect from animating.  Show first frame.
void StopEffect(int effect_num);

//restart a stopped effect
void RestartEffect(int effect_num);

#ifdef FAST_FILE_IO
#define EClipReadN(ec, n, fp) CFRead(ec, sizeof(eclip), n, fp)
#else
/*
 * reads n eclip structs from a CFILE
 */
extern int EClipReadN(eclip *ec, int n, CFILE *fp);
#endif

grsBitmap *SetupHiresAnim (short *frameP, int nFrames, int nBaseTex, int bIndirect, int bObj, int *pnFrames);
void ResetPogEffects (void);

#endif /* _EFFECTS_H */
