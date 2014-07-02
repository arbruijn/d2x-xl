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

typedef struct tEffectClip {
	tVideoClip	vClipInfo;        //embedded tVideoClip
	fix			xTimeLeft;			//for sequencing
	int32_t			nCurFrame;			//for sequencing
	int16_t			changingWallTexture;		//Which element of Textures array to replace.
	int16_t			changingObjectTexture;  //Which element of ObjBitmapPtrs array to replace.
	int32_t			flags;				//see above
	int32_t			nCritClip;			//use this clip instead of above one when mine critical
	int32_t			nDestBm;				//use this bitmap when monitor destroyed
	int32_t			nDestVClip;			//what tVideoClip to play when exploding
	int32_t			nDestroyedClip;			//what tEffectClip to play when exploding
	fix			xDestSize;			//3d size of explosion
	int32_t			nSound;				//what sound this makes
	int32_t			nSegment, nSide;	//what seg & CSide, for one-shot clips
} __pack__ tEffectClip;

typedef tEffectClip D1_eclip;

extern int32_t Num_effects [2];
extern tEffectClip Effects [2][MAX_EFFECTS];

// Set up special effects.
fix EffectFrameTime (tEffectClip* ecP);

// Set up special effects.
void InitSpecialEffects (void);

// Clear any active one-shots
void ResetSpecialEffects (void);

// Function called in game loop to do effects.
void DoSpecialEffects (bool bSetup = false);

// Restore bitmap
void RestoreEffectBitmapIcons (void);

//stop an effect from animating.  Show first frame.
void StopEffect(int32_t effect_num);

//restart a stopped effect
void RestartEffect (int32_t nEffect);

/*
 * reads n tEffectClip structs from a CFILE
 */
void ReadEffectClip (tEffectClip& ec, CFile& cf);
int32_t ReadEffectClips (CArray<tEffectClip>& ec, int32_t n, CFile& cf);

CBitmap *SetupHiresAnim (int16_t *frameP, int32_t nFrames, int32_t nBaseTex, int32_t bIndirect, int32_t bObj, int32_t *pnFrames, CBitmap* bmP = NULL);
void ResetPogEffects (void);
void CacheObjectEffects (void);

#endif /* _EFFECTS_H */
