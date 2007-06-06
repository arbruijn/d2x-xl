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
	int     nDestBm;    //use this bitmap when monitor destroyed
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

#if 0
#define EClipReadN(ec, n, fp) CFRead(ec, sizeof(eclip), n, fp)
#else
/*
 * reads n eclip structs from a CFILE
 */
extern int EClipReadN(eclip *ec, int n, CFILE *fp);
#endif

grsBitmap *SetupHiresAnim (short *frameP, int nFrames, int nBaseTex, int bIndirect, int bObj, int *pnFrames);
void ResetPogEffects (void);
void CacheObjectEffects (void);

#endif /* _EFFECTS_H */
