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

#ifndef _ANIM_H
#define _ANIM_H

#include "gr.h"
#include "object.h"
#include "piggy.h"
#include "cfile.h"

#define ANIM_PLAYER_HIT            1
#define ANIM_SMALL_EXPLOSION       2
#define ANIM_VOLATILE_WALL_HIT     5
#define ANIM_MORPHING_ROBOT        10
#define ANIM_PLAYER_APPEARANCE     61
#define ANIM_POWERUP_DISAPPEARANCE 62
#define ANIM_WATER_HIT             84
#define ANIM_AFTERBURNER_BLOB      95
#define ANIM_MONITOR_STATIC        99

#define MAX_VCLIPS                 110
#define D1_ANIM_MAXNUM             70
#define ANIM_MAX_FRAMES            30

// tAnimationInfo flags
#define VF_ROD      1       // draw as a rod, not a blob

typedef struct tAnimationInfo {
	fix             xTotalTime;          // total time (in seconds) of clip
	int32_t         nFrameCount;
	fix             xFrameTime;         // time (in seconds) of each frame
	int32_t         flags;
	int16_t         nSound;
	tBitmapIndex    frames [ANIM_MAX_FRAMES];
	fix             lightValue;
} __pack__ tAnimationInfo;

//extern int32_t Num_vclips [2];
//extern tAnimationInfo Vclip [2][MAX_VCLIPS];

// draw an CObject which renders as a tAnimationInfo.
void DrawVClipObject (CObject *objP, fix timeleft, int32_t lighted, int32_t vclip_num, CFloatVector *color);
void DrawWeaponVClip (CObject *objP);
void DrawExplBlast (CObject *objP);
void DrawShockwave (CObject *objP);
void ConvertWeaponToVClip (CObject *objP);
CRGBColor *AnimationColor (CObject *objP);

void ReadVideoClip (tAnimationInfo& vc, CFile& cf);
int32_t ReadVideoClips (CArray<tAnimationInfo>& vc, int32_t n, CFile& cf);

#endif /* _ANIM_H */
