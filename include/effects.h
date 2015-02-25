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

typedef struct tChangeEffect {
	int16_t					nWallTexture;		//Which element of Textures array to replace.
	int16_t					nObjectTexture;  //Which element of ObjBitmapPtrs array to replace.
} tChangeEffect;

typedef struct tDestructionEffect {
	int32_t					nTexture;
	int32_t					nAnimation;
	int32_t					nEffect;
	fix						xSize;
} tDestroyedEffect;

typedef struct tEffectInfo {
	tAnimationInfo	animationInfo;						//embedded tAnimationInfo
	fix						xTimeLeft;					//for sequencing
	int32_t					nCurFrame;					//for sequencing
	tChangeEffect			changing;
	int32_t					flags;						//see above
	int32_t					nCriticalAnimation;		//use this clip instead of above one when mine critical
	tDestructionEffect	destroyed;
	int32_t					nSound;						//what sound this makes
	int32_t					nSegment, nSide;			//what seg & CSide, for one-shot clips
} __pack__ tEffectInfo;

typedef tEffectInfo D1_eclip;

// Set up special effects.
fix EffectFrameTime (tEffectInfo* effectInfoP);

// Set up special effects.
void InitSpecialEffects (void);

// Clear any active one-shots
void ResetSpecialEffects (void);

// Function called in game loop to do effects.
void DoSpecialEffects (bool bSetup = false);

// Restore bitmap
void RestoreEffectBitmapIcons (void);

//stop an effect from animating.  Show first frame.
void StopEffect (int32_t effect_num);

//restart a stopped effect
void RestartEffect (int32_t nEffect);

/*
 * reads n tEffectInfo structs from a CFILE
 */
void ReadEffectClip (tEffectInfo& effectInfo, CFile& cf);
int32_t ReadEffectInfo (CArray<tEffectInfo>& effectInfo, int32_t n, CFile& cf);

CBitmap *SetupHiresAnim (int16_t *frameP, int32_t nFrames, int32_t nBaseTex, int32_t bIndirect, int32_t bObj, int32_t *pnFrames, CBitmap* bmP = NULL);
void ResetPogEffects (void);
void CacheObjectEffects (void);

#endif /* _EFFECTS_H */
