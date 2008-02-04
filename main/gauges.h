/* $Id: gauges.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _GAUGES_H
#define _GAUGES_H

#include "fix.h"
#include "gr.h"
#include "piggy.h"
#include "object.h"
#include "hudmsg.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
//from gauges.c

// Flags for gauges/hud stuff
extern ubyte Reticle_on;

extern void InitGaugeCanvases();
extern void CloseGaugeCanvases();

extern void show_score();
extern void show_score_added();
extern void AddPointsToScore();
extern void AddBonusPointsToScore();

void RenderGauges(void);
void InitGauges(void);
extern void check_erase_message(void);

extern void HUDRenderMessageFrame();
extern void HUDClearMessages();

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
#define gauge_message HUDInitMessage

extern void DrawHUD();     // draw all the HUD stuff

extern void PlayerDeadMessage(void);
//extern void say_afterburner_status(void);

// fills in the coords of the hostage video window
void get_hostage_window_coords(int *x, int *y, int *w, int *h);

// from testgaug.c

void gauge_frame(void);
extern void UpdateLaserWeaponInfo(void);
extern void PlayHomingWarning(void);

typedef struct {
	ubyte r,g,b;
} rgb;

extern rgb playerColors[];

#define WBU_WEAPON			0       // the weapons display
#define WBUMSL					1       // the missile view
#define WBU_ESCORT			2       // the "buddy bot"
#define WBU_REAR				3       // the rear view
#define WBU_COOP				4       // coop or team member view
#define WBU_GUIDED			5       // the guided missile
#define WBU_MARKER			6       // a dropped marker
#define WBU_STATIC			7       // playing static after missile hits
#define WBU_RADAR_TOPDOWN	8
#define WBU_RADAR_HEADSUP	9

// draws a 3d view into one of the cockpit windows.  win is 0 for
// left, 1 for right.  viewer is tObject.  NULL tObject means give up
// window user is one of the WBU_ constants.  If rearViewFlag is
// set, show a rear view.  If label is non-NULL, print the label at
// the top of the window.
void DoCockpitWindowView(int win, tObject *viewer, int rearViewFlag, int user, char *label);
void FreeInventoryIcons (void);
void FreeObjTallyIcons (void);
void HUDShowIcons (void);
int CanSeeObject(int nObject, int bCheckObjs);
void ShowFrameRate (void);
void ToggleCockpit ();

#define SHOW_COCKPIT	((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nMode == CM_STATUS_BAR))
#define SHOW_HUD		(!gameStates.app.bEndLevelSequence && (gameOpts->render.cockpit.bHUD || !SHOW_COCKPIT))
#define HIDE_HUD		(gameStates.app.bEndLevelSequence || (!gameOpts->render.cockpit.bHUD && (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)))

extern double cmScaleX, cmScaleY;
extern int nHUDLineSpacing;

//	-----------------------------------------------------------------------------

#define HUD_SCALE(v, s)	((int) ((double) (v) * (s) + 0.5))
#define HUD_SCALE_X(v)	HUD_SCALE (v, cmScaleX)
#define HUD_SCALE_Y(v)	HUD_SCALE (v, cmScaleY)
#define HUD_LHX(x)      (gameStates.menus.bHires ? 2 * (x) : x)
#define HUD_LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)
#define HUD_ASPECT		((double) grdCurScreen->scHeight / (double) grdCurScreen->scWidth / 0.75)

//	-----------------------------------------------------------------------------

static inline void HUDBitBlt (int x, int y, grsBitmap *bmP, int scale, int orient)
{
OglUBitMapMC (
	 (x < 0) ? -x : HUD_SCALE_X (x), 
	 (y < 0) ? -y : HUD_SCALE_Y (y), 
	HUD_SCALE_X (bmP->bmProps.w) * (gameStates.app.bDemoData + 1), 
	HUD_SCALE_Y (bmP->bmProps.h) * (gameStates.app.bDemoData + 1), 
	bmP, 
	NULL, 
	scale, 
	orient
	);
}

//	-----------------------------------------------------------------------------

#endif /* _GAUGES_H */
