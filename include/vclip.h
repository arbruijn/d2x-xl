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

#ifndef _VCLIP_H
#define _VCLIP_H

#include "gr.h"
#include "object.h"
#include "piggy.h"
#include "cfile.h"

#define VCLIP_PLAYER_HIT            1
#define VCLIP_SMALL_EXPLOSION       2
#define VCLIP_VOLATILE_WALL_HIT     5
#define VCLIP_MORPHING_ROBOT        10
#define VCLIP_PLAYER_APPEARANCE     61
#define VCLIP_POWERUP_DISAPPEARANCE 62
#define VCLIP_WATER_HIT             84
#define VCLIP_AFTERBURNER_BLOB      95
#define VCLIP_MONITOR_STATIC        99

#define VCLIP_MAXNUM                110
#define D1_VCLIP_MAXNUM             70
#define VCLIP_MAX_FRAMES            30

// tVideoClip flags
#define VF_ROD      1       // draw as a rod, not a blob

typedef struct {
	fix             xTotalTime;          // total time (in seconds) of clip
	int             nFrameCount;
	fix             xFrameTime;         // time (in seconds) of each frame
	int             flags;
	short           nSound;
	tBitmapIndex    frames[VCLIP_MAX_FRAMES];
	fix             lightValue;
} __pack__ tVideoClip;

extern int Num_vclips [2];
extern tVideoClip Vclip [2][VCLIP_MAXNUM];

// draw an CObject which renders as a tVideoClip.
void DrawVClipObject (CObject *objP, fix timeleft, int lighted, int vclip_num, tRgbaColorf *color);
void DrawWeaponVClip (CObject *objP);
void DrawExplBlast (CObject *objP);
void ConvertWeaponToVClip (CObject *objP);
int SetupHiresVClip (tVideoClip *vcP, tVClipInfo *vciP);
tRgbColorb *VClipColor (CObject *objP);

#if 0
#define VClipReadN(vc, n, fp) CFRead(vc, sizeof(tVideoClip), n, fp)
#else
/*
 * reads n tVideoClip structs from a CFILE
 */
extern int VClipReadN(tVideoClip *vc, int n, CFile& cf);
#endif

#endif /* _VCLIP_H */
