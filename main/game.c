/* $Id: game.c,v 1.25 2003/12/08 22:32:56 btb Exp $ */
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
#include <conf.h>
#endif

#ifdef RCS
char game_rcsid[] = "$Id: game.c,v 1.25 2003/12/08 22:32:56 btb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#ifdef MACINTOSH
#include <Files.h>
#include <StandardFile.h>
#include <Quickdraw.h>
#include <Script.h>
#include <Strings.h>
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "cameras.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "digi.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newmenu.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "gamefont.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "gamepal.h"
#include "particles.h"
#include "lightmap.h"
#include "oof.h"
#include "sphere.h"

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "fix.h"
#include "hudmsg.h"
#include "tracker.h"
#include "particles.h"
#include "banlist.h"
#include "input.h"
#include "interp.h"

u_int32_t nCurrentVGAMode;

void GamePaletteStepUp (int r, int g, int b);


#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

#define	SHOW_EXIT_PATH	1

#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>            //should come after inferno.h to get mark setting
#endif
#endif


void FreeHoardData (void);
int ReadControls (void);		// located in gamecntl.c
void DoFinalBossFrame (void);

int	Speedtest_on = 0;

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
int	Speedtest_startTime;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	Speedtest_count=0;				//	number of times to do the debug test.
#endif

fix ThisLevelTime=0;

#if defined (TIMER_TEST) && !defined (NDEBUG)
fix TimerValue,actual_lastTimerValue,_last_frametime;
int gameData.time.xStops,gameData.time.xStarts;
int gameData.time.xStopped,gameData.time.xStarted;
#endif

#ifndef MACINTOSH
ubyte * Game_cockpit_copy_code = NULL;
#else
ubyte Game_cockpit_copy_code = 0;
ubyte Scanline_double = 1;
#endif

u_int32_t	nVRScreenMode			= 0;
ubyte			VR_screenFlags	= 0;		//see values in screens.h
ubyte			nVRCurrentPage	= 0;
fix			VR_eye_width		= F1_0;
int			VR_render_mode		= VR_NONE;
int			VR_low_res 			= 3;				// Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity     = 1;		// 0 - 2

//NEWVR
int			VR_eye_offset		 = 0;
int			VR_eye_switch		 = 0;
int			VR_eye_offset_changed = 0;
int			VR_use_reg_code 	= 0;

grs_canvas  *VR_offscreen_buffer	= NULL;		// The offscreen data buffer
grs_canvas	VR_render_buffer[2];					//  Two offscreen buffers for left/right eyes.
grs_canvas	VR_render_sub_buffer[2];			//  Two sub buffers for left/right eyes.
grs_canvas	VR_screen_pages[2];					//  Two pages of VRAM if paging is available
grs_canvas	VR_editor_canvas;						//  The canvas that the editor writes to.

#ifdef WINDOWS
//@@ LPDIRECTDRAWSURFACE _lpDDSMask = NULL;
dd_grs_canvas *dd_VR_offscreen_buffer = NULL;
dd_grs_canvas dd_VR_screen_pages[2];
dd_grs_canvas dd_VR_render_buffer[2];
dd_grs_canvas dd_VR_render_sub_buffer[2];

void game_win_init_cockpit_mask (int sram);
#endif

//do menus work in 640x480 or 320x200?
//PC version sets this in main ().  Mac versios is always high-res, so set to 1 here
int Debug_pause=0;				//John's debugging pause system

//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.
int	Dummy_var;
int	*Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char faded_in;
#endif

#ifndef NDEBUG                          //these only exist if debugging

int Game_double_buffer = 1;     //double buffer by default
fix fixed_frametime=0;          //if non-zero, set frametime to this

#endif

grsBitmap bmBackground;

#define BACKGROUND_NAME "statback.pcx"

//	Function prototypes for GAME.C exclusively.

int GameLoop (int RenderFlag, int ReadControlsFlag);
void FireLaser (void);
void SlideTextures (void);
void PowerupGrabCheatAll (void);

//	Other functions
void MultiCheckForKillGoalWinner ();
void MultiCheckForEntropyWinner ();
void RestoreGameSurfaces ();

// window functions

void grow_window (void);
void shrink_window (void);

// text functions

void fill_background ();

#ifndef RELEASE
void show_framerate (void);
void ftoa (char *string, fix f);
#endif

//	==============================================================================================

void LoadBackgroundBitmap ()
{
	int pcx_error;

if (bmBackground.bm_texBuf)
	d_free (bmBackground.bm_texBuf);
bmBackground.bm_texBuf=NULL;
pcx_error = pcx_read_bitmap (gameStates.app.cheats.bJohnHeadOn ? "johnhead.pcx" : BACKGROUND_NAME,
									  &bmBackground, BM_LINEAR,0);
if (pcx_error != PCX_ERROR_NONE)
	Error ("File %s - PCX error: %s",BACKGROUND_NAME,pcx_errormsg (pcx_error));
GrRemapBitmapGood (&bmBackground, NULL, -1, -1);
}


//------------------------------------------------------------------------------
//this is called once per game
void InitGame ()
{
	atexit (CloseGame);             //for cleanup
/*---*/LogErr ("Initializing game data\n  Objects ...\n");
	InitObjects ();
/*---*/LogErr ("  Special effects...\n");
	InitSpecialEffects ();
/*---*/LogErr ("  AI system...\n");
	InitAISystem ();
//*---*/LogErr ("  gauge canvases...\n");
//	InitGaugeCanvases ();
/*---*/LogErr ("  exploding wall...\n");
	InitExplodingWalls ();
/*---*/LogErr ("  particle systems...\n");
	InitSmoke ();
/*---*/LogErr ("  loading background bitmap...\n");
	LoadBackgroundBitmap ();
	nClearWindow = 2;		//	do portal only window clear.
	InitDetailLevels (gameStates.app.nDetailLevel);
//	BuildMissionList (0);
	/* Register cvars */
}

//------------------------------------------------------------------------------

void ResetPaletteAdd ()
{
gameStates.ogl.palAdd.red = 
gameStates.ogl.palAdd.green =
gameStates.ogl.palAdd.blue	= 0;
GrPaletteStepUp (0, 0, 0);
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
}

//------------------------------------------------------------------------------

#ifdef WINDOWS
void win_get_span_list (grsBitmap *bm, int miny, int maxy)
{
	int x,y;
	int mode = 0;
	ubyte *data;
	int offset;
	int lspan=0, rspan=0, span=0;
//@@FILE *fp;

	data = bm->bm_texBuf;
//@@	fp = fopen ("cockspan.dat", "w");

	for (y = 0; y < miny; y++)
		win_cockpit_mask[y].num = 0;

	for (y = miny; y <= maxy; y++)
	{
		span = 0;
		//@@ fprintf (fp, "line %d: ", y);
		for (x = 0; x < bm->bm_props.w; x++)
		{
			offset = y*bm->bm_props.rowsize + x;

			if (data[offset] == 255) {
				switch (mode)
				{
					case 0:				// Start Mode
						lspan	= x;
						win_cockpit_mask[y].span[span].xmin = x;
						mode = 1;
					//@@	fprintf (fp, "<%d,", lspan);
						break;

					case 1:				// Transparency mode
						rspan = x;
						win_cockpit_mask[y].span[span].xmax = x;
						break;

					case 2:				// Switch from Draw mode to transparent
						lspan = x;
						win_cockpit_mask[y].span[span].xmin = x;
					//@@	fprintf (fp, "<%d,", lspan);
						mode = 1;
						break;
				}
			}
			else {
				switch (mode)
				{
					case 0:				// Start mode
						mode = 2;
						break;

					case 1:				// Switching from transparent to Draw
						rspan = x;
						mode = 2;
						win_cockpit_mask[y].span[span].xmax = x;
						span++;
					//@@	fprintf (fp, "%d> ", rspan);
						break;

					case 2:
						break;
				}
			}
		}
		if (mode == 1) {
		//@@	fprintf (fp, "%d> ", rspan);
			win_cockpit_mask[y].span[span].xmax = rspan;
			span++;
		}
		win_cockpit_mask[y].num = span;
	//@@	fprintf (fp, "\n");
		mode = 0;
	}
	win_cockpit_mask[y].num = 255;
}
#endif //WINDOWS

//------------------------------------------------------------------------------

void ShowInGameWarning (char *s)
{
if (grdCurScreen) {
	if (!((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StopTime ();
	gameData.menu.warnColor = RED_RGBA;
	gameData.menu.colorOverride = gameData.menu.warnColor;
	ExecMessageBox (TXT_WARNING, NULL, -3, s, " ", TXT_OK);
	gameData.menu.colorOverride = 0;
	if (!((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StartTime ();
	}
}

//------------------------------------------------------------------------------
//these should be in gr.h
#define cv_w  cv_bitmap.bm_props.w
#define cv_h  cv_bitmap.bm_props.h

int Game_window_x = 0;
int Game_window_y = 0;
int Game_window_w = 0;
int Game_window_h = 0;
int max_window_w = 0;
int max_window_h = 0;

extern void NDRecordCockpitChange (int);

//------------------------------------------------------------------------------
//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void InitCockpit ()
{
	if (gameStates.video.nScreenMode != SCREEN_GAME)
		return;

#if defined (POLY_ACC)
 pa_flush ();                 // get rid of undrawn polys.
 pa_clear_buffer (1, 0);
#endif

//Initialize the on-screen canvases

if (gameData.demo.nState==ND_STATE_RECORDING)
	NDRecordCockpitChange (gameStates.render.cockpit.nMode);
if (VR_render_mode != VR_NONE)
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
//if (!(VR_screenFlags & VRF_ALLOW_COCKPIT) && (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR || gameStates.render.cockpit.nMode==CM_REAR_VIEW))
//	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
if (gameStates.video.nScreenMode == SCREEN_EDITOR)
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
#if 0//def OGL
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT || gameStates.render.cockpit.nMode == CM_REAR_VIEW) {
	HUDMessage (MSGC_GAME_FEEDBACK, "Cockpit not available in GL mode");
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
}
#endif

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
	);
GrSetCurFont (GAME_FONT);

#if !defined (MACINTOSH) && !defined (WINDOWS)
if (Game_cockpit_copy_code)
	d_free (Game_cockpit_copy_code);
Game_cockpit_copy_code  = NULL;
#else
if (Game_cockpit_copy_code)
	Game_cockpit_copy_code = 0;
#endif

//@@	#ifdef WINDOWS
//@@		if (_lpDDSMask) { DDFreeSurface (_lpDDSMask); _lpDDSMask = NULL; }
//@@	#endif

#ifdef WINDOWS
	game_win_init_cockpit_mask (0);
#endif

switch (gameStates.render.cockpit.nMode) {
#ifndef OGL
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW: {
		grsBitmap *bm = &gameData.pig.tex.bitmaps[gameData.pig.tex.cockpitBmIndex[gameStates.render.cockpit.nMode+ (gameStates.video.nDisplayMode? (gameData.models.nCockpits/2):0)].index];
		PIGGY_PAGE_IN (gameData.pig.tex.cockpitBmIndex[gameStates.render.cockpit.nMode+ (gameStates.video.nDisplayMode? (gameData.models.nCockpits/2):0)]);

#ifdef WINDOWS
		DDGrSetCurrentCanvas (NULL);
		game_win_init_cockpit_mask (1);
		DDGrSetCurrentCanvas (dd_VR_offscreen_buffer);
#else
		GrSetCurrentCanvas (VR_offscreen_buffer);
#endif

		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBitmap (0, 0, bm);
		bm = &VR_offscreen_buffer->cv_bitmap;
		bm->bm_props.flags = BM_FLAG_TRANSPARENT;
		gr_ibitblt_find_hole_size (bm, &minx, &miny, &maxx, &maxy);
		WIN (	win_get_span_list (bm, miny, maxy);
				DDGRUNLOCK (dd_grd_curcanv)
			);

#ifndef WINDOWS
#ifndef __MSDOS__
		gr_ibitblt_create_mask (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
#else
		if (gameStates.video.nDisplayMode) {
#if defined (POLY_ACC)
			Game_cockpit_copy_code  = gr_ibitblt_create_mask_pa (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
			pa_clear_buffer (1, 0);      // clear offscreen to reduce white flash.
#else
			Game_cockpit_copy_code  = gr_ibitblt_create_mask_svga (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
#endif
		} else
			Game_cockpit_copy_code  = gr_ibitblt_create_mask (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
#endif
		bm->bm_props.flags = 0;		// Clear all flags for offscreen canvas
#else
		Game_cockpit_copy_code  = (ubyte *) (1);
		bm->bm_props.flags = 0;		// Clear all flags for offscreen canvas
#endif
		GameInitRenderSubBuffers (0, 0, maxx-minx+1, maxy-miny+1);
		break;
	}
#endif

#ifdef OGL
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:
     	max_window_h = (grdCurScreen->sc_h * 2) / 3;
		if (Game_window_h > max_window_h)
			Game_window_h = max_window_h;
		if (Game_window_w > max_window_w)
			Game_window_w = max_window_w;
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		GameInitRenderSubBuffers (Game_window_x, Game_window_y, Game_window_w, Game_window_h);
		break;
#endif
	case CM_FULL_SCREEN:
		max_window_h = grdCurScreen->sc_h;
//		if (Game_window_h > max_window_h || VR_screenFlags&VRF_ALLOW_COCKPIT)
			Game_window_h = max_window_h;
//		if (Game_window_w > max_window_w || VR_screenFlags&VRF_ALLOW_COCKPIT)
			Game_window_w = max_window_w;
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		GameInitRenderSubBuffers (Game_window_x, Game_window_y, Game_window_w, Game_window_h);
		break;

	case CM_STATUS_BAR:
		{
		int h = gameData.pig.tex.bitmaps [0][gameData.pig.tex.cockpitBmIndex[CM_STATUS_BAR+ (gameStates.video.nDisplayMode? (gameData.models.nCockpits/2):0)].index].bm_props.h;
		if (grdCurScreen->sc_h > 480)
			h = (int) ((double) h * (double) grdCurScreen->sc_h / 480.0);
     	max_window_h = grdCurScreen->sc_h - h;
//		if (Game_window_h > max_window_h || VR_screenFlags&VRF_ALLOW_COCKPIT)
			Game_window_h = max_window_h;
//		if (Game_window_w > max_window_w || VR_screenFlags&VRF_ALLOW_COCKPIT)
			Game_window_w = max_window_w;
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		GameInitRenderSubBuffers (Game_window_x, Game_window_y, Game_window_w, Game_window_h);
		}
		break;

	case CM_LETTERBOX:	{
		int x,y,w,h;

		x = 0; w = VR_render_buffer[0].cv_bitmap.bm_props.w;		//VR_render_width;
		h = (VR_render_buffer[0].cv_bitmap.bm_props.h * 7) / 10;
		y = (VR_render_buffer[0].cv_bitmap.bm_props.h-h)/2;
		GameInitRenderSubBuffers (x, y, w, h);
		break;
		}

	}

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
);
gameStates.gameplay.nShieldFlash = 0;
}

//------------------------------------------------------------------------------

//selects a given cockpit (or lack of one).  See types in game.h
void SelectCockpit (int mode)
{
	if (mode != gameStates.render.cockpit.nMode) {		//new mode
		gameStates.render.cockpit.nMode=mode;
		InitCockpit ();
	}
}

//------------------------------------------------------------------------------

extern int last_drawn_cockpit[2];

//force cockpit redraw next time. call this if you've trashed the screen
void ResetCockpit ()
{
	gameStates.render.cockpit.bRedraw=1;
	last_drawn_cockpit[0] = -1;
	last_drawn_cockpit[1] = -1;
}

//------------------------------------------------------------------------------

//NEWVR
void VRResetParams ()
{
	VR_eye_width = VR_SEPARATION;
	VR_eye_offset = VR_PIXEL_SHIFT;
	VR_eye_offset_changed = 2;
}

//------------------------------------------------------------------------------

void GameInitRenderSubBuffers (int x, int y, int w, int h)
{
#ifdef WINDOWS
	DDGrInitSubCanvas (&dd_VR_render_sub_buffer[0], &dd_VR_render_buffer[0], x, y, w, h);
	DDGrInitSubCanvas (&dd_VR_render_sub_buffer[1], &dd_VR_render_buffer[1], x, y, w, h);

	dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.x = 0;
	dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.y = 0;
	dd_VR_render_sub_buffer[0].xoff = x;
	dd_VR_render_sub_buffer[0].yoff = y;
	dd_VR_render_sub_buffer[1].canvas.cv_bitmap.bm_props.x = 0;
	dd_VR_render_sub_buffer[1].canvas.cv_bitmap.bm_props.y = 0;
	dd_VR_render_sub_buffer[1].xoff = x;
	dd_VR_render_sub_buffer[1].yoff = y;

#endif
	if (Scanline_double) {
		#ifdef MACINTOSH
		if (w & 0x3)
			w &= ~0x3;
		GrInitSubCanvas (&VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w/2, (h/2)+1);
		GrInitSubCanvas (&VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w/2, (h/2)+1);
		#endif
	} else {
		GrInitSubCanvas (&VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w, h);
		GrInitSubCanvas (&VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w, h);
	}

	#ifdef MACINTOSH
		#ifdef POLY_ACC
			if (PAEnabled)
			{
				TQARect	newBounds;

				newBounds.left = x;
				newBounds.right = x + w;
				newBounds.top = y;
				newBounds.bottom = y + h;

				pa_set_context (kGamePlayDrawContextID, &newBounds);		// must resize/create new context
			}
		#endif
	#endif

#ifdef WINDOWS
	VR_render_sub_buffer[0].cv_bitmap.bm_props.x = 0;
	VR_render_sub_buffer[0].cv_bitmap.bm_props.y = 0;
	VR_render_sub_buffer[1].cv_bitmap.bm_props.x = 0;
	VR_render_sub_buffer[1].cv_bitmap.bm_props.y = 0;
#endif
}

//------------------------------------------------------------------------------

#ifdef WINDOWS
// Sets up the canvases we will be rendering to (WIN95)
void GameInitRenderBuffers (int screen_mode, int render_w, int render_h, int render_method, int flags)
{
//	Hack for switching to higher that 640x480 modes (DDraw doesn't allow
//	 creating surfaces greater than the current resolution

	if (GRMODEINFO (rw) < render_w || GRMODEINFO (rh) < render_h) {
		render_w = GRMODEINFO (rw);
		render_h = GRMODEINFO (rh);
	}
	nVRScreenMode		= screen_mode;
	VR_screenFlags	=  flags;
	VRResetParams ();
	VR_render_mode 	= render_method;
	Game_window_w 		= render_w;
	Game_window_h		= render_h;
	if (dd_VR_offscreen_buffer && dd_VR_offscreen_buffer != dd_grd_backcanv) {
		dd_gr_free_canvas (dd_VR_offscreen_buffer);
	}

	if ((VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED))	{
		if (render_h*2 < 200)	{
		 	Int3 ();		// Not Supported yet!!!
//			VR_offscreen_buffer = GrCreateCanvas (render_w, 200);
		}
		else {
			Int3 ();		// Not Supported yet!!!
//			VR_offscreen_buffer = GrCreateCanvas (render_w, render_h*2);
		}

		Int3 ();			// Not Supported yet!!!
//		GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
//		GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h);
	}
	else if (GRMODEINFO (paged) && !GRMODEINFO (dbuf)) {
	//	Here we will make the VR_offscreen_buffer the 2nd page and hopefully
	//	we can just flip it, saving a blt.

		dd_VR_offscreen_buffer = dd_grd_backcanv;
		VR_offscreen_buffer = & dd_grd_backcanv->canvas;
	}
	else if (GRMODEINFO (dbuf)||GRMODEINFO (emul)) {
	//	The offscreen buffer will be created.  We will just blt this
	//	to the screen (which may be blted to the primary surface)
		if (render_h < 200) {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (render_w, 200);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
		}
		else {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (render_w, render_h);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
		}
	}

	DDGrInitSubCanvas (&dd_VR_render_buffer[0], dd_VR_offscreen_buffer, 0, 0, render_w, render_h);
	DDGrInitSubCanvas (&dd_VR_render_buffer[1], dd_VR_offscreen_buffer, 0, 0, render_w, render_h);
	GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
	GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h);

	GameInitRenderSubBuffers (0, 0, render_w, render_h);
}

#else

//------------------------------------------------------------------------------

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void GameInitRenderBuffers (int screen_mode, int render_w, int render_h, int render_method, int flags)
{
//	if (vga_check_mode (screen_mode) != 0)
//		Error ("Cannot set requested video mode");

	nVRScreenMode		=	screen_mode;
	VR_screenFlags	=  flags;
//NEWVR
	VRResetParams ();
	VR_render_mode 	= render_method;
	Game_window_w 		= render_w;
	Game_window_h		= render_h;
	if (VR_offscreen_buffer) {
		GrFreeCanvas (VR_offscreen_buffer);
	}

	if ((VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED))	{
		if (render_h*2 < 200)	{
			VR_offscreen_buffer = GrCreateCanvas (render_w, 200);
		}
		else {
			VR_offscreen_buffer = GrCreateCanvas (render_w, render_h*2);
		}

		GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
		GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h);
	}
	else {
		if (render_h < 200) {
			VR_offscreen_buffer = GrCreateCanvas (render_w, 200);
		}
		else {
#if defined (POLY_ACC)
			#ifndef MACINTOSH
            VR_offscreen_buffer = GrCreateCanvas (render_w, render_h);
            d_free (VR_offscreen_buffer->cv_bitmap.bm_texBuf);
            GrInitCanvas (VR_offscreen_buffer, pa_get_buffer_address (1), BM_LINEAR15, render_w, render_h);
			#else
				if (PAEnabled || gConfigInfo.mAcceleration) {
					gameStates.render.cockpit.nMode=CM_FULL_SCREEN;		// HACK HACK HACK HACK HACK!!!!
					VR_offscreen_buffer = GrCreateCanvas2 (render_w, render_h, BM_LINEAR15);
				} else
	            VR_offscreen_buffer = GrCreateCanvas (render_w, render_h);
			#endif
#else
            VR_offscreen_buffer = GrCreateCanvas (render_w, render_h);
#endif
        }

#ifdef OGL
		VR_offscreen_buffer->cv_bitmap.bm_props.nType = BM_OGL;
#endif

		GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
		GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h);
	}

	GameInitRenderSubBuffers (0, 0, render_w, render_h);
}
#endif

//------------------------------------------------------------------------------

//called to get the screen in a mode compatible with popup menus.
//if we can't have popups over the game screen, switch to menu mode.

void SetPopupScreenMode (void)
{
	//WIN (LoadCursorWin (MOUSE_DEFAULT_CURSOR);

#ifdef OGL // always have to switch to menu mode
	if (!gameOpts->menus.nStyle || (gameStates.video.nScreenMode < 0))
#else
	if (!(VR_screenFlags & VRF_COMPATIBLE_MENUS))
#endif
	{
//		gameStates.video.nLastScreenMode = -1;
//		gameStates.app.bGameRunning = (gameStates.app.nFunctionMode == FMODE_GAME);
		SetScreenMode (SCREEN_MENU);		//must switch to menu mode
	}
}

//------------------------------------------------------------------------------
//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int SetScreenMode (u_int32_t sm)
{
WIN (static int force_mode_change=0);
WIN (static int saved_window_w);
WIN (static int saved_window_h);

#ifdef EDITOR
	if ((sm==SCREEN_MENU) && (gameStates.video.nScreenMode==SCREEN_EDITOR))	{
		GrSetCurrentCanvas (Canv_editor);
		return 1;
	}
#endif

#ifdef WINDOWS
	if (gameStates.video.nScreenMode == sm && W95DisplayMode == nVRScreenMode) {
		DDGrSetCurrentCanvas (&dd_VR_screen_pages[nVRCurrentPage]);
		return 1;
	}
#else
#	ifdef OGL
	if ((gameStates.video.nScreenMode == sm) && (nCurrentVGAMode == nVRScreenMode) && 
		 (/* (sm != SCREEN_GAME) ||*/ (grdCurScreen->sc_mode == nVRScreenMode))) {
		GrSetCurrentCanvas (VR_screen_pages + nVRCurrentPage);
		OglSetScreenMode ();
		return 1;
	}
#	else
	if (gameStates.video.nScreenMode == sm && nCurrentVGAMode == nVRScreenMode) {
		GrSetCurrentCanvas (&VR_screen_pages[nVRCurrentPage]);
		return 1;
	}
#	endif
#endif


#ifdef EDITOR
	Canv_editor = NULL;
#endif

	gameStates.video.nScreenMode = sm;

	switch (gameStates.video.nScreenMode) {
		case SCREEN_MENU:
		#ifdef WINDOWS
			//mouse_set_mode (0);
			//ShowCursorW ();
			if (!(VR_screenFlags & VRF_COMPATIBLE_MENUS)) {
			// HACK!!!  Meant to save window size when switching from
			// non-compat menu mode to menu mode.
				saved_window_w = Game_window_w;
				saved_window_h = Game_window_h;
				force_mode_change = 1;
			}
			if (W95DisplayMode != SM95_640x480x8) {
//@@				PiggyBitmapPageOutAllW ();		// 2D GFX Flush cache.
				DDSETDISPLAYMODE (SM95_640x480x8);
				dd_gr_init_screen ();
				if (!gameStates.render.bPaletteFadedOut) 
					GrPaletteStepLoad (NULL);
			}

			DDGrInitSubCanvas (&dd_VR_screen_pages[0], dd_grd_screencanv,
									0,0,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.w,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.h);
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_screencanv,
									0,0,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.w,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.h);
			gameStates.menus.bHires = 1;
			gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable;

		#else
		{
			u_int32_t nMenuMode;

			gameStates.menus.bHires = gameStates.menus.bHiresAvailable;		//do highres if we can

#if defined (POLY_ACC)
				#ifndef MACINTOSH
	            nMenuMode = gameStates.menus.bHires ? SM (640,480) : SM (320,200);
				#else
					nMenuMode = PAEnabled ? SM_640x480x15xPA : SM_640x480V;
				#endif
#else
            nMenuMode = 
					gameStates.gfx.bOverride ?
						gameStates.gfx.nStartScrMode
						: gameStates.menus.bHires ?
							 (nVRScreenMode >= SM (640,480)) ?
								nVRScreenMode
								: SM (640,480)
							: SM (320,200);
#endif
			gameStates.video.nLastScreenMode = -1;
			if (nCurrentVGAMode != nMenuMode) {
				if (GrSetMode (nMenuMode))
					Error ("Cannot set screen mode for menu");
				if (!gameStates.render.bPaletteFadedOut)
					GrPaletteStepLoad (NULL);
				gameStates.menus.bInitBG = 1;
				RebuildGfxFx (gameStates.app.bGameRunning, 1);
			}

			GrInitSubCanvas (
				VR_screen_pages, &grdCurScreen->sc_canvas, 0, 0, 
				grdCurScreen->sc_w, grdCurScreen->sc_h);
			GrInitSubCanvas (
				VR_screen_pages + 1, &grdCurScreen->sc_canvas, 0, 0, 
				grdCurScreen->sc_w, grdCurScreen->sc_h);

			gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && gameStates.menus.bHires;

		}
		#endif
		break;

	case SCREEN_GAME:
#if 0 //def WINDOWS
		//mouse_set_mode (1);
		HideCursorW ();
		if (force_mode_change || (W95DisplayMode != nVRScreenMode)) {

			DDSETDISPLAYMODE (nVRScreenMode);
//@@			PiggyBitmapPageOutAllW ();		// 2D GFX Flush cache.
			dd_gr_init_screen ();
#if TRACE
			//con_printf (CON_DEBUG, "Reinitializing render buffers due to display mode change.\n");
#endif
			GameInitRenderBuffers (W95DisplayMode,
					GRMODEINFO (rw), GRMODEINFO (rh),
					VR_render_mode, VR_screenFlags);

			ResetCockpit ();
		}
#else
		if (nCurrentVGAMode != nVRScreenMode) {
			if (GrSetMode (nVRScreenMode))	{
				Error ("Cannot set desired screen mode for game!");
				//we probably should do something else here, like select a standard mode
			}
			#ifdef MACINTOSH
			if ((gameConfig.nControlType == 1) && (gameStates.app.nFunctionMode == FMODE_GAME))
				joydefs_calibrate ();
			#endif
			ResetCockpit ();
		}
#endif
#if 0
		if (VR_render_mode != VR_NONE)
			gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
		else 
#endif
			{
			max_window_w = grdCurScreen->sc_w;
			max_window_h = grdCurScreen->sc_h;
#if 0
			if (VR_screenFlags & VRF_ALLOW_COCKPIT) {
				if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		      	max_window_h = grdCurScreen->sc_h - gameData.pig.tex.bitmaps[gameData.pig.tex.cockpitBmIndex[CM_STATUS_BAR+ (gameStates.video.nDisplayMode? (gameData.models.nCockpits/2):0)].index].bm_props.h;
				}
			else if (gameStates.render.cockpit.nMode != CM_LETTERBOX)
				gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
#endif
	      if (!Game_window_h || (Game_window_h > max_window_h) || 
				 !Game_window_w || (Game_window_w > max_window_w)) {
				Game_window_w = max_window_w;
				Game_window_h = max_window_h;
				}
			}
		InitCockpit ();

		#ifdef WINDOWS
		//	Super hack.   If we are switching from a 320x200 game to 640x480.
		//						and we were in a menumode when switching, we don't
		//						restore Game_window vals
			if (force_mode_change && (W95DisplayMode == SM95_320x200x8X)) {
				Game_window_w = saved_window_w;
				Game_window_h = saved_window_h;
				force_mode_change = 0;
			}
		#endif


	//	Define screen pages for game mode
	// If we designate through screenFlags to use paging, then do so.
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[0], dd_grd_screencanv, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h),
			GrInitSubCanvas (&VR_screen_pages[0], &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h)
		);

		if (VR_screenFlags&VRF_USE_PAGING) {
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_backcanv, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h),
			GrInitSubCanvas (&VR_screen_pages[1], &grdCurScreen->sc_canvas, 0, grdCurScreen->sc_h, grdCurScreen->sc_w, grdCurScreen->sc_h)
		);
		}
		else {
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_screencanv, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h),
			GrInitSubCanvas (&VR_screen_pages[1], &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h)
		);
		}

		InitCockpit ();

	#ifdef WINDOWS
		gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.video.nDisplayMode != 0);
		gameStates.menus.bHires = 1;
	#else
		gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.menus.bHires = (gameStates.video.nDisplayMode > 1));
	#endif

		if (VR_render_mode != VR_NONE)	{
			// for 640x480 or higher, use hires font.
			if (gameStates.render.fonts.bHiresAvailable && (grdCurScreen->sc_h > 400))
				gameStates.render.fonts.bHires = 1;
			else
				gameStates.render.fonts.bHires = 0;
		}
		con_resize ();
		break;

	#ifdef EDITOR
	case SCREEN_EDITOR:
		if (grdCurScreen->sc_mode != SM (800,600))	{
			int gr_error;
			if ((gr_error=GrSetMode (SM (800,600)))!=0) { //force into game scrren
				Warning ("Cannot init editor screen (error=%d)",gr_error);
				return 0;
			}
		}
		GrPaletteStepLoad (NULL);

		GrInitSubCanvas (&VR_editor_canvas, &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
		Canv_editor = &VR_editor_canvas;
		GrInitSubCanvas (&VR_screen_pages[0], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		GrInitSubCanvas (&VR_screen_pages[1], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		GrSetCurrentCanvas (Canv_editor);
		init_editor_screen ();   //setup other editor stuff
		break;
	#endif
	default:
		Error ("Invalid screen mode %d",sm);
	}

	nVRCurrentPage = 0;

	WINDOS (
		DDGrSetCurrentCanvas (&dd_VR_screen_pages[nVRCurrentPage]),
		GrSetCurrentCanvas (&VR_screen_pages[nVRCurrentPage])
	);

	if (VR_screenFlags&VRF_USE_PAGING)	{
	WINDOS (
		dd_gr_flip (),
		GrShowCanvas (&VR_screen_pages[nVRCurrentPage])
	);
	}
#ifdef OGL
	OglSetScreenMode ();
#endif

	return 1;
}

//------------------------------------------------------------------------------

int GrToggleFullScreenGame (void)
{
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	int i=GrToggleFullScreen ();
	//added 2000/06/19 Matthew Mueller - hack to fix "infinite toggle" problem
	//it seems to be that the screen mode change takes long enough that the key has already sent repeat codes, or that its unpress event gets dropped, etc.  This is a somewhat ugly fix, but it works.
//	generic_key_handler (KEY_PADENTER,0);
	FlushInput ();
	if (gameStates.app.bGameRunning) {
		HUDMessage (MSGC_GAME_FEEDBACK, "toggling fullscreen mode %s",i?"on":"off");
		StopPlayerMovement ();
		//RebuildGfxFx (gameStates.app.bGameRunning);
		}
	//end addition -MM
	return i;
#else
	if (gameStates.app.bGameRunning)
		HUDMessage (MSGC_GAME_FEEDBACK, "fullscreen toggle not supported by this target");
	return -1;
#endif
}

//------------------------------------------------------------------------------

int arch_toggle_fullscreen_menu (void);

int GrToggleFullScreenMenu (void)
{
#ifdef GR_SUPPORTS_FULLSCREEN_MENU_TOGGLE
	int i;
	i=arch_toggle_fullscreen_menu ();

//	generic_key_handler (KEY_PADENTER,0);
	FlushInput ();
	StopPlayerMovement ();
	RebuildGfxFx (gameStates.app.bGameRunning, 1);
	return i;
#else
	return -1;
#endif
}

//------------------------------------------------------------------------------

void StopTime ()
{
if (++gameData.time.nPaused == 1) {
	fix xTime = TimerGetFixedSeconds ();
	gameData.time.xSlack = xTime - gameData.time.xLast;
	if (gameData.time.xSlack < 0) {
#if defined (TIMER_TEST) && !defined (NDEBUG)
		Int3 ();		//get Matt!!!!
#endif
		gameData.time.xLast = 0;
		}
#if defined (TIMER_TEST) && !defined (NDEBUG)
	gameData.time.xStopped = xTime;
	#endif
}
#if defined (TIMER_TEST) && !defined (NDEBUG)
gameData.time.xStops++;
#endif
}

//------------------------------------------------------------------------------

void StartTime ()
{
Assert (gameData.time.nPaused > 0);
if (!--gameData.time.nPaused) {
	fix xTime = TimerGetFixedSeconds ();
#if defined (TIMER_TEST) && !defined (NDEBUG)
	if (gameData.time.xLast < 0)
		Int3 ();		//get Matt!!!!
#endif
	gameData.time.xLast = xTime - gameData.time.xSlack;
#if defined (TIMER_TEST) && !defined (NDEBUG)
	gameData.time.xStarted = time;
#endif
	}
#if defined (TIMER_TEST) && !defined (NDEBUG)
gameData.time.xStarts++;
#endif
}

//------------------------------------------------------------------------------

int TimeStopped (void)
{
return gameData.time.nPaused > 0;
}

//------------------------------------------------------------------------------

extern ubyte joydefs_calibrating;

void GameFlushInputs ()
{
	int dx,dy;
#if 1
	FlushInput ();
#else	
	KeyFlush ();
	joy_flush ();
	mouse_flush ();
#endif	
	#ifdef MACINTOSH
	if ((gameStates.app.nFunctionMode != FMODE_MENU) && !joydefs_calibrating)		// only reset mouse when not in menu or not calibrating
	#endif
		MouseGetDelta (&dx, &dy);	// Read mouse
	memset (&Controls,0,sizeof (control_info));
}

//------------------------------------------------------------------------------

void ResetTime ()
{
gameData.time.xLast = TimerGetFixedSeconds ();
}

//------------------------------------------------------------------------------

#ifndef RELEASE
extern int Saving_movie_frames;
int Movie_fixed_frametime;
#else
#define Saving_movie_frames	0
#define Movie_fixed_frametime	0
#endif


void CalcFrameTime ()
{
	fix 	timerValue,
			last_frametime = gameData.time.xFrame,
			minFrameTime = (gameOpts->render.nMaxFPS ? f1_0 / gameOpts->render.nMaxFPS : 1);

if (gameData.app.bGamePaused) {
	gameData.time.xLast = TimerGetFixedSeconds ();
	gameData.time.xFrame = 0;
	gameData.time.xRealFrame = 0;
	return;
	}
#if defined (TIMER_TEST) && !defined (NDEBUG)
_last_frametime = last_frametime;
#endif
do {
	timerValue = TimerGetFixedSeconds ();
   gameData.time.xFrame = timerValue - gameData.time.xLast;
	timer_delay (1);
	if (gameOpts->render.nMaxFPS < 2)
		break;
	} while (gameData.time.xFrame < minFrameTime);
#if defined (TIMER_TEST) && !defined (NDEBUG)
TimerValue = timerValue;
#endif
#ifdef _DEBUG
if ((gameData.time.xFrame <= 0) || 
	 (gameData.time.xFrame > F1_0) || 
	 (gameStates.app.nFunctionMode == FMODE_EDITOR) || 
	 (gameData.demo.nState == ND_STATE_PLAYBACK)) {
#if TRACE
//con_printf (1,"Bad gameData.time.xFrame - value = %x\n",gameData.time.xFrame);
#endif
if (gameData.time.xFrame == 0)
	Int3 ();	//	Call Mike or Matt or John!  Your interrupts are probably trashed!
	}
#endif
#if defined (TIMER_TEST) && !defined (NDEBUG)
actual_lastTimerValue = gameData.time.xLast;
#endif
if (gameStates.app.cheats.bTurboMode)
	gameData.time.xFrame *= 2;
// Limit frametime to be between 5 and 150 fps.
gameData.time.xRealFrame = gameData.time.xFrame;
#if 0
if (gameData.time.xFrame < F1_0/150) 
	gameData.time.xFrame = F1_0/150;
if (gameData.time.xFrame > F1_0/5) 
	gameData.time.xFrame = F1_0/5;
#endif
gameData.time.xLast = timerValue;
if (gameData.time.xFrame < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.time.xFrame = last_frametime;		//d:\temp\dm_test.then use time from last frame
#ifndef NDEBUG
if (fixed_frametime) 
	gameData.time.xFrame = fixed_frametime;
#endif
#ifndef NDEBUG
// Pause here!!!
if (Debug_pause) {
	int c = 0;
	while (!c)
		c = key_peekkey ();
	if (c == KEY_P)       {
		Debug_pause = 0;
		c = KeyInKey ();
		}
	gameData.time.xLast = TimerGetFixedSeconds ();
	}
#endif
#if Arcade_mode
gameData.time.xFrame /= 2;
#endif
#if defined (TIMER_TEST) && !defined (NDEBUG)
gameData.time.xStops = gameData.time.xStarts = 0;
#endif
//	Set value to determine whether homing missile can see target.
//	The lower frametime is, the more likely that it can see its target.
if (gameData.time.xFrame <= F1_0/64)
	xMinTrackableDot = MIN_TRACKABLE_DOT;	// -- 3* (F1_0 - MIN_TRACKABLE_DOT)/4 + MIN_TRACKABLE_DOT;
else if (gameData.time.xFrame < F1_0/32)
	xMinTrackableDot = MIN_TRACKABLE_DOT + F1_0/64 - 2*gameData.time.xFrame;	// -- FixMul (F1_0 - MIN_TRACKABLE_DOT, F1_0-4*gameData.time.xFrame) + MIN_TRACKABLE_DOT;
else if (gameData.time.xFrame < F1_0/4)
	xMinTrackableDot = MIN_TRACKABLE_DOT + F1_0/64 - F1_0/16 - gameData.time.xFrame;	// -- FixMul (F1_0 - MIN_TRACKABLE_DOT, F1_0-4*gameData.time.xFrame) + MIN_TRACKABLE_DOT;
else
	xMinTrackableDot = MIN_TRACKABLE_DOT + F1_0/64 - F1_0/8;
}

//------------------------------------------------------------------------------

//--unused-- int Auto_flythrough=0;  //if set, start flythough automatically

void MovePlayerToSegment (tSegment *segP,int tSide)
{
	vmsVector vp;

COMPUTE_SEGMENT_CENTER (&gameData.objs.console->position.vPos,segP);
COMPUTE_SIDE_CENTER (&vp,segP,tSide);
VmVecDec (&vp,&gameData.objs.console->position.vPos);
VmVector2Matrix (&gameData.objs.console->position.mOrient, &vp, NULL, NULL);
RelinkObject (OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM (segP));
}

//------------------------------------------------------------------------------

#ifdef NETWORK
void GameDrawTimeLeft ()
{
        char temp_string[30];
        fix timevar;
        int i;

        GrSetCurFont (GAME_FONT);    //GAME_FONT
        GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);

        timevar=i2f (netGame.PlayTimeAllowed*5*60);
        i=f2i (timevar-ThisLevelTime);
        i++;

        sprintf (temp_string, TXT_TIME_LEFT, i);

        if (i>=0)
         GrString (0, 32, temp_string);
}
#endif

//------------------------------------------------------------------------------

void do_photos ();
void level_with_floor ();

void modex_clear_box (int x,int y,int w,int h)
{
	grs_canvas *temp_canv,*save_canv;

	save_canv = grdCurCanv;
	temp_canv = GrCreateCanvas (w,h);
	GrSetCurrentCanvas (temp_canv);
	GrClearCanvas (BLACK_RGBA);
	GrSetCurrentCanvas (save_canv);
	GrBitmapM (x,y,&temp_canv->cv_bitmap);
	GrFreeCanvas (temp_canv);

}

//------------------------------------------------------------------------------

extern void modex_printf (int x,int y,char *s,grs_font *font,int color);

// mac routine to drop contents of screen to a pict file using copybits
// save a PICT to a file
#ifdef MACINTOSH

void SavePictScreen (int multiplayer)
{
	OSErr err;
	int parid, i, count;
	char *pfilename, filename[50], buf[512], cwd[FILENAME_MAX];
	short fd;
	FSSpec spec;
	PicHandle pict_handle;
	static int multi_count = 0;
	StandardFileReply sf_reply;

// dump the contents of the GameWindow into a picture using copybits

	pict_handle = OpenPicture (&GameWindow->portRect);
	if (pict_handle == NULL)
		return;

	CopyBits (&GameWindow->portBits, &GameWindow->portBits, &GameWindow->portRect, &GameWindow->portRect, srcBic, NULL);
	ClosePicture ();

// get the cwd to restore with chdir when done -- this keeps the mac world sane
	if (!getcwd (cwd, FILENAME_MAX))
		Int3 ();
// create the fsspec

	sprintf (filename, "screen%d", multi_count++);
	pfilename = c2pstr (filename);
	if (!multiplayer) {
		show_cursor ();
		StandardPutFile ("\pSave PICT as:", pfilename, &sf_reply);
		if (!sf_reply.sfGood)
			goto end;
		memcpy (&spec, & (sf_reply.sfFile), sizeof (FSSpec));
		if (sf_reply.sfReplacing)
			FSpDelete (&spec);
		err = FSpCreate (&spec, 'ttxt', 'PICT', smSystemScript);
		if (err)
			goto end;
	} else {
//		parid = GetAppDirId ();
		err = FSMakeFSSpec (0, 0, pfilename, &spec);
		if (err == nsvErr)
			goto end;
		if (err != fnfErr)
			FSpDelete (&spec);
		err = FSpCreate (&spec, 'ttxt', 'PICT', smSystemScript);
		if (err != 0)
			goto end;
	}

// write the PICT file
	if (FSpOpenDF (&spec, fsRdWrPerm, &fd))
		goto end;
	memset (buf, 0, sizeof (buf);
	count = 512;
	if (FSWrite (fd, &count, buf))
		goto end;
	count = GetHandleSize ((Handle)pict_handle);
	HLock ((Handle)pict_handle);
	if (FSWrite (fd, &count, *pict_handle)) {
		FSClose (fd);
		FSpDelete (&spec);
	}

end:
	HUnlock ((Handle)pict_handle);
	DisposeHandle ((Handle)pict_handle);
	FSClose (fd);
	hide_cursor ();
	chdir (cwd);
}

#endif

//------------------------------------------------------------------------------

//automapFlag is now unused, since we just check if the screen we're
//writing to is modex
//if called from automap, current canvas is set to visible screen
#ifndef OGL
void SaveScreenShot (NULL, int automapFlag)
{
#if defined (WINDOWS)
#if TRACE
	//con_printf (CON_DEBUG, "Doing screen shot thing.\n");
#endif
	win95_save_pcx_shot ();

#elif !defined (MACINTOSH)
	fix t1;
	char message[100];
	grs_canvas *screen_canv=&grdCurScreen->sc_canvas;
	grs_font *save_font;
	static int savenum=0;
	static int stereo_savenum=0;
	grs_canvas *temp_canv,*temp_canv2,*save_canv;
        char savename[FILENAME_LEN],savename2[FILENAME_LEN];
	ubyte pal[768];
	int w,h,aw,x,y;
	int modexFlag;
	int stereo=0;

	temp_canv2=NULL;

//	// Can't do screen shots in VR modes.
//	if (VR_render_mode != VR_NONE)
//		return;

	StopTime ();

	save_canv = grdCurCanv;

	if (VR_render_mode != VR_NONE && !automapFlag && gameStates.app.nFunctionMode==FMODE_GAME && gameStates.video.nScreenMode==SCREEN_GAME)
		stereo = 1;

	if (stereo) {
		temp_canv = GrCreateCanvas (VR_render_buffer[0].cv_bitmap.bm_props.w,VR_render_buffer[0].cv_bitmap.bm_props.h);
		GrSetCurrentCanvas (temp_canv);
		gr_ubitmap (0,0,&VR_render_buffer[0].cv_bitmap);

		temp_canv2 = GrCreateCanvas (VR_render_buffer[1].cv_bitmap.bm_props.w,VR_render_buffer[1].cv_bitmap.bm_props.h);
		GrSetCurrentCanvas (temp_canv2);
		gr_ubitmap (0,0,&VR_render_buffer[1].cv_bitmap);
	}
	else {
		temp_canv = GrCreateCanvas (screen_canv->cv_bitmap.bm_props.w,screen_canv->cv_bitmap.bm_props.h);
		GrSetCurrentCanvas (temp_canv);
		gr_ubitmap (0,0,&screen_canv->cv_bitmap);
	}

	GrSetCurrentCanvas (save_canv);

	if (savenum > 99) savenum = 0;
	if (stereo_savenum > 99) stereo_savenum = 0;

	if (stereo) {
		sprintf (savename,"left%02d.pcx",stereo_savenum);
		sprintf (savename2,"right%02d.pcx",stereo_savenum);
		if (VR_eye_switch) {char t[FILENAME_LEN]; strcpy (t,savename); strcpy (savename,savename2); strcpy (savename2,t);}
		stereo_savenum++;
		sprintf (message, "%s '%s' & '%s'", TXT_DUMPING_SCREEN, savename, savename2);
	}
	else {
		sprintf (savename,"screen%02d.pcx",savenum++);
		sprintf (message, "%s '%s'", TXT_DUMPING_SCREEN, savename);
	}

	if (!automapFlag)		//if from automap, curcanv is already visible canv
		GrSetCurrentCanvas (NULL);
	modexFlag = (grdCurCanv->cv_bitmap.bm_props.nType==BM_MODEX);
	if (!automapFlag && modexFlag)
		GrSetCurrentCanvas (&VR_screen_pages[nVRCurrentPage]);

	save_font = grdCurCanv->cv_font;
	GrSetCurFont (GAME_FONT);
	GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	GrGetStringSize (message,&w,&h,&aw);

	if (modexFlag)
		h *= 2;

	//I changed how these coords were calculated for the high-res automap. -MT
	//x = (VR_screen_pages[nVRCurrentPage].cv_w-w)/2;
	//y = (VR_screen_pages[nVRCurrentPage].cv_h-h)/2;
	x = (grdCurCanv->cv_w-w)/2;
	y = (grdCurCanv->cv_h-h)/2;

	if (modexFlag) {
		modex_clear_box (x-2,y-2,w+4,h+4);
		modex_printf (x, y, message,GAME_FONT,GrFindClosestColorCurrent (0,31,0);
	} else {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (x-2,y-2,x+w+2,y+h+2);
		GrPrintF (x,y,message);
		GrSetCurFont (save_font);
	}
	t1 = TimerGetFixedSeconds () + F1_0;

	GrPaletteRead (pal);		//get actual palette from the hardware
	pcx_write_bitmap (savename,&temp_canv->cv_bitmap,pal);
	if (stereo)
		pcx_write_bitmap (savename2,&temp_canv2->cv_bitmap,pal);

	while (TimerGetFixedSeconds () < t1);		// Wait so that messag stays up at least 1 second.

	GrSetCurrentCanvas (screen_canv);

	if (grdCurCanv->cv_bitmap.bm_props.nType!=BM_MODEX && !stereo)
		gr_ubitmap (0,0,&temp_canv->cv_bitmap);

	GrFreeCanvas (temp_canv);
	if (stereo)
		GrFreeCanvas (temp_canv2);

	GrSetCurrentCanvas (save_canv);
	KeyFlush ();
	StartTime ();

#else

	grs_canvas *screen_canv = &grdCurScreen->sc_canvas;
	grs_canvas *temp_canv, *save_canv;

	// Can't do screen shots in VR modes.
	if (VR_render_mode != VR_NONE)
		return;

	StopTime ();

	save_canv = grdCurCanv;
	temp_canv = GrCreateCanvas (screen_canv->cv_bitmap.bm_props.w, screen_canv->cv_bitmap.bm_props.h);
	if (!temp_canv)
		goto shot_done;
	GrSetCurrentCanvas (temp_canv);
	gr_ubitmap (0, 0, &screen_canv->cv_bitmap);
	GrSetCurrentCanvas (&VR_screen_pages[nVRCurrentPage]);

	show_cursor ();
	key_close ();
	if (gameData.app.nGameMode & GM_MULTI)
		SavePictScreen (1);
	else
		SavePictScreen (0);
	key_init ();
	hide_cursor ();

	GrSetCurrentCanvas (screen_canv);

//	if (!automapFlag)
		gr_ubitmap (0, 0, &temp_canv->cv_bitmap);

	GrFreeCanvas (temp_canv);
shot_done:
	GrSetCurrentCanvas (save_canv);
	KeyFlush ();
	StartTime ();
	#endif
}

#endif

//------------------------------------------------------------------------------

//initialize flying
void FlyInit (tObject *objP)
{
	objP->controlType = CT_FLYING;
	objP->movementType = MT_PHYSICS;

	VmVecZero (&objP->mType.physInfo.velocity);
	VmVecZero (&objP->mType.physInfo.thrust);
	VmVecZero (&objP->mType.physInfo.rotVel);
	VmVecZero (&objP->mType.physInfo.rotThrust);
}

//void morph_test (), morph_step ();


//	------------------------------------------------------------------------------------

void test_anim_states ();

#include "fvi.h"

//put up the help message
void do_show_help ()
{
	ShowHelp ();
}


extern int been_in_editor;

//	------------------------------------------------------------------------------------

void DoCloakStuff (void)
{
	int i;
	for (i = 0; i < gameData.multi.nPlayers; i++)
		if (gameData.multi.players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (gameData.time.xGame - gameData.multi.players[i].cloakTime > CLOAK_TIME_MAX) {
				gameData.multi.players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == gameData.multi.nLocalPlayer) {
					DigiPlaySample (SOUND_CLOAK_OFF, F1_0);
					#ifdef NETWORK
					if (gameData.app.nGameMode & GM_MULTI)
						MultiSendPlaySound (SOUND_CLOAK_OFF, F1_0);
					MaybeDropNetPowerup (-1, POW_CLOAK, FORCE_DROP);
					MultiSendDeCloak (); // For demo recording
					#endif
				}
			}
		}
}

int bFakingInvul=0;

//	------------------------------------------------------------------------------------

void DoInvulnerableStuff (void)
{
if ((gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime != 0x7fffffff)) {
	if (gameData.time.xGame - gameData.multi.players [gameData.multi.nLocalPlayer].invulnerableTime > INVULNERABLE_TIME_MAX) {
		gameData.multi.players[gameData.multi.nLocalPlayer].flags ^= PLAYER_FLAGS_INVULNERABLE;
		if (!bFakingInvul) {
			DigiPlaySample (SOUND_INVULNERABILITY_OFF, F1_0);
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI) {
				MultiSendPlaySound (SOUND_INVULNERABILITY_OFF, F1_0);
				MaybeDropNetPowerup (-1, POW_INVULNERABILITY, FORCE_DROP);
				}
#endif
#if TRACE
				//con_printf (CON_DEBUG, " --- You have been DE-INVULNERABLEIZED! ---\n");
#endif
			}
		bFakingInvul=0;
		}
	}
}

ubyte	bLastAfterburnerState = 0;
fix xLastAfterburnerCharge = 0;

#define AFTERBURNER_LOOP_START	 ((gameOpts->sound.digiSampleRate==SAMPLE_RATE_22K)?32027: (32027/2))		//20098
#define AFTERBURNER_LOOP_END		 ((gameOpts->sound.digiSampleRate==SAMPLE_RATE_22K)?48452: (48452/2))		//25776

int	Ab_scale = 4;

//@@//	------------------------------------------------------------------------------------
//@@void afterburner_shake (void)
//@@{
//@@	int	rx, rz;
//@@
//@@	rx = (Ab_scale * FixMul (d_rand () - 16384, F1_0/8 + (((gameData.time.xGame + 0x4000)*4) & 0x3fff)))/16;
//@@	rz = (Ab_scale * FixMul (d_rand () - 16384, F1_0/2 + ((gameData.time.xGame*4) & 0xffff)))/16;
//@@
//@@	gameData.objs.console->mType.physInfo.rotVel.x += rx;
//@@	gameData.objs.console->mType.physInfo.rotVel.z += rz;
//@@
//@@}

//	------------------------------------------------------------------------------------
#ifdef NETWORK
extern void MultiSendSoundFunction (char,char);
#endif

void DoAfterburnerStuff (void)
{
if (!(gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER))
	xAfterburnerCharge=0;
if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead) {
	if (DigiKillSoundLinkedToObject (gameData.multi.players[gameData.multi.nLocalPlayer].nObject))
#ifdef NETWORK
		MultiSendSoundFunction (0,0)
#endif
	 	;
	}
else if ((xLastAfterburnerCharge && (Controls.afterburner_state != bLastAfterburnerState)) || 
	 		(bLastAfterburnerState && (xLastAfterburnerCharge && !xAfterburnerCharge))) {
	if (xAfterburnerCharge && Controls.afterburner_state && 
		 (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)) {
		DigiLinkSoundToObject3 ((short) SOUND_AFTERBURNER_IGNITE, (short) gameData.multi.players[gameData.multi.nLocalPlayer].nObject, 
										1, F1_0, i2f (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_IGNITE);
#endif
		}
	else {
		DigiKillSoundLinkedToObject (gameData.multi.players[gameData.multi.nLocalPlayer].nObject);
		DigiLinkSoundToObject2 ((short) SOUND_AFTERBURNER_PLAY, (short) gameData.multi.players[gameData.multi.nLocalPlayer].nObject, 
										0, F1_0, i2f (256));
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
		 	MultiSendSoundFunction (0,0);
#endif
		}
	}
bLastAfterburnerState = Controls.afterburner_state;
xLastAfterburnerCharge = xAfterburnerCharge;
}

// -- //	------------------------------------------------------------------------------------
// -- //	if energy < F1_0/2, recharge up to F1_0/2
// -- void recharge_energy_frame (void)
// -- {
// -- 	if (gameData.multi.players[gameData.multi.nLocalPlayer].energy < gameData.weapons.info[0].energy_usage) {
// -- 		gameData.multi.players[gameData.multi.nLocalPlayer].energy += gameData.time.xFrame/4;
// --
// -- 		if (gameData.multi.players[gameData.multi.nLocalPlayer].energy > gameData.weapons.info[0].energy_usage)
// -- 			gameData.multi.players[gameData.multi.nLocalPlayer].energy = gameData.weapons.info[0].energy_usage;
// -- 	}
// -- }

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE	16		//	gots to be a power of 2, else change the code in DiminishPaletteTowardsNormal

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD (int dr, int dg, int db)
{
	int	maxVal;

	maxVal = gameData.render.xFlashEffect ? 60 : MAX_PALETTE_ADD;
#if 0
	dMax = maxVal - gameStates.ogl.palAdd.red;
	h = maxVal - gameStates.ogl.palAdd.green;
	if (h < dMax)
		dMax = h;
	h = maxVal - gameStates.ogl.palAdd.blue;
	if (h < dMax)
		dMax = h;
	//the following code will keep the color of the flash effect
	//by limiting the intensity increase of the other color components
	//if one color component hits the ceiling.
	if ((dr > dMax) || (dg > dMax) || (db > dMax))
		dr = dg = db = dMax;
#endif
	gameStates.ogl.palAdd.red += dr;
	gameStates.ogl.palAdd.green += dg;
	gameStates.ogl.palAdd.blue += db;
	CLAMP (gameStates.ogl.palAdd.red, -maxVal, maxVal);
	CLAMP (gameStates.ogl.palAdd.green, -maxVal, maxVal);
	CLAMP (gameStates.ogl.palAdd.blue, -maxVal, maxVal);
}

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.

void DiminishPaletteTowardsNormal (void)
{
	int	dec_amount = 0;

	//	Diminish at DIMINISH_RATE units/second.
	//	For frame rates > DIMINISH_RATE Hz, use randomness to achieve this.
	if (gameData.time.xFrame < F1_0/DIMINISH_RATE) {
		if (d_rand () < gameData.time.xFrame*DIMINISH_RATE/2)	//	Note: d_rand () is in 0d:\temp\dm_test32767, and 8 Hz means decrement every frame
			dec_amount = 1;
		}
	else {
		dec_amount = f2i (gameData.time.xFrame*DIMINISH_RATE);		// one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++;						// make sure we decrement by something
	}

	if (gameData.render.xFlashEffect) {
		int	force_do = 0;

		//	Part of hack system to force update of palette after exiting a menu.
		if (gameData.render.xTimeFlashLastPlayed) {
			force_do = 1;
			gameStates.ogl.palAdd.red ^= 1;	//	Very Tricky!  In GrPaletteStepUp, if all stepups same as last time, won't do anything!
		}

		if ((gameData.render.xTimeFlashLastPlayed + F1_0/8 < gameData.time.xGame) || (gameData.render.xTimeFlashLastPlayed > gameData.time.xGame)) {
			DigiPlaySample (SOUND_CLOAK_OFF, gameData.render.xFlashEffect/4);
			gameData.render.xTimeFlashLastPlayed = gameData.time.xGame;
		}

		gameData.render.xFlashEffect -= gameData.time.xFrame;
		if (gameData.render.xFlashEffect < 0)
			gameData.render.xFlashEffect = 0;

		if (force_do || (d_rand () > 4096)) {
      	if ((gameData.demo.nState==ND_STATE_RECORDING) && (gameStates.ogl.palAdd.red || gameStates.ogl.palAdd.green || gameStates.ogl.palAdd.blue))
	      	NDRecordPaletteEffect (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

			GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

			return;
		}

	}

	if (gameStates.ogl.palAdd.red > 0) { 
		gameStates.ogl.palAdd.red -= dec_amount; 
		if (gameStates.ogl.palAdd.red < 0) 
			gameStates.ogl.palAdd.red = 0; 
		}
	else if (gameStates.ogl.palAdd.red < 0) { 
		gameStates.ogl.palAdd.red += dec_amount; 
		if (gameStates.ogl.palAdd.red > 0) 
			gameStates.ogl.palAdd.red = 0; 
		}
	if (gameStates.ogl.palAdd.green > 0) { 
		gameStates.ogl.palAdd.green -= dec_amount; 
		if (gameStates.ogl.palAdd.green < 0) 
			gameStates.ogl.palAdd.green = 0; 
		}
	else if (gameStates.ogl.palAdd.green < 0) { 
		gameStates.ogl.palAdd.green += dec_amount; 
		if (gameStates.ogl.palAdd.green > 0) 
			gameStates.ogl.palAdd.green = 0; 
		}
	if (gameStates.ogl.palAdd.blue > 0) { 
		gameStates.ogl.palAdd.blue -= dec_amount; 
		if (gameStates.ogl.palAdd.blue < 0) 
			gameStates.ogl.palAdd.blue = 0; 
		}
	else if (gameStates.ogl.palAdd.blue < 0) { 
		gameStates.ogl.palAdd.blue += dec_amount; 
		if (gameStates.ogl.palAdd.blue > 0) 
			gameStates.ogl.palAdd.blue = 0; 
		}

	if ((gameData.demo.nState==ND_STATE_RECORDING) && (gameStates.ogl.palAdd.red || gameStates.ogl.palAdd.green || gameStates.ogl.palAdd.blue))
		NDRecordPaletteEffect (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

	GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

}

//------------------------------------------------------------------------------

tRgbColors palAddSave;

void PaletteSave (void)
{
palAddSave = gameStates.ogl.palAdd;
}

//------------------------------------------------------------------------------

extern void GrPaletteStepUpVR (int r, int g, int b, int white, int black);

void GamePaletteStepUp (int r, int g, int b)
{
if (VR_use_reg_code)
#ifndef WINDOWS
	;//GrPaletteStepUpVR (r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX);
#endif
else
	GrPaletteStepUp (r, g, b);
}

void PaletteRestore (void)
{
gameStates.ogl.palAdd = palAddSave; 
GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);
//	Forces flash effect to fixup palette next frame.
gameData.render.xTimeFlashLastPlayed = 0;
}

void DeadPlayerFrame (void);
void player_smoke_frame (void);

//	-----------------------------------------------------------------------------

int AllowedToFireLaser (void)
{
	if (gameStates.app.bPlayerIsDead) {
		gameData.app.nGlobalMissileFiringCount = 0;
		return 0;
	}
	if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
		return 0;
	//	Make sure enough time has elapsed to fire laser, but if it looks like it will
	//	be a long while before laser can be fired, then there must be some mistake!
	if (xNextLaserFireTime > gameData.time.xGame)
		if (xNextLaserFireTime < gameData.time.xGame + 2*F1_0)
			return 0;

	return 1;
}

//------------------------------------------------------------------------------

fix	Next_flare_fireTime = 0;
#define	FLARE_BIG_DELAY	 (F1_0*2)

int AllowedToFireFlare (void)
{
	if (Next_flare_fireTime > gameData.time.xGame)
		if (Next_flare_fireTime < gameData.time.xGame + FLARE_BIG_DELAY)	//	In case time is bogus, never wait > 1 second.
			return 0;
	if (gameData.multi.players[gameData.multi.nLocalPlayer].energy >= WI_energy_usage (FLARE_ID))
		Next_flare_fireTime = gameData.time.xGame + F1_0/4;
	else
		Next_flare_fireTime = gameData.time.xGame + FLARE_BIG_DELAY;

	return 1;
}

//------------------------------------------------------------------------------

int AllowedToFireMissile (void)
{
//	Make sure enough time has elapsed to fire missile, but if it looks like it will
//	be a long while before missile can be fired, then there must be some mistake!
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 0;
if (xNextMissileFireTime > gameData.time.xGame) 
	if (xNextMissileFireTime < gameData.time.xGame + 5 * F1_0)
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void FullPaletteSave (void)
{
	PaletteSave ();
	ApplyModifiedPalette ();
	ResetPaletteAdd ();
	GrPaletteStepLoad (NULL);
}

//------------------------------------------------------------------------------

#define ADD_HELP_OPT(_text)	m [opt].nType = NM_TYPE_TEXT; m [opt++].text = (_text);

void ShowHelp ()
{
	int nitems, opt = 0;
	tMenuItem m[30];
	#ifdef MACINTOSH
	char command_help[64], pixel_double_help[64], save_help[64], restore_help[64];
	#endif

	memset (m, 0, sizeof (m));
	ADD_HELP_OPT (TXT_HELP_ESC);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_ALT_F2);
	ADD_HELP_OPT (TXT_HELP_ALT_F3);
#else
	sprintf (save_help, TXT_HLP_SAVE, 133);
	sprintf (restore_help, TXT_HLP_LOAD, 133);
	ADD_HELP_OPT (save_help);
	ADD_HELP_OPT (restore_help);
#endif
	ADD_HELP_OPT (TXT_HELP_F2);
	ADD_HELP_OPT (TXT_HELP_F3);
	ADD_HELP_OPT (TXT_HELP_F4);
	ADD_HELP_OPT (TXT_HELP_F5);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_PAUSE);
#else
	ADD_HELP_OPT (TXT_HLP_PAUSE);
#endif
	ADD_HELP_OPT (TXT_HELP_MINUSPLUS);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_PRTSCN);
#else
	ADD_HELP_OPT (TXT_HLP_SCREENIE);
#endif
	ADD_HELP_OPT (TXT_HELP_1TO5);
	ADD_HELP_OPT (TXT_HELP_6TO10);
	ADD_HELP_OPT (TXT_HLP_CYCLE_LEFT_WIN);
	ADD_HELP_OPT (TXT_HLP_CYCLE_RIGHT_WIN);
	ADD_HELP_OPT (TXT_HLP_RESIZE_WIN);
	ADD_HELP_OPT (TXT_HLP_REPOS_WIN);
	ADD_HELP_OPT (TXT_HLP_ZOOM_WIN);
	ADD_HELP_OPT (TXT_HLP_GUIDEBOT);
	ADD_HELP_OPT (TXT_HLP_RENAMEGB);
	ADD_HELP_OPT (TXT_HLP_DROP_PRIM);
	ADD_HELP_OPT (TXT_HLP_DROP_SEC);
	ADD_HELP_OPT (TXT_HLP_CALIBRATE);
	ADD_HELP_OPT (TXT_HLP_GBCMDS);
#ifdef MACINTOSH
	sprintf (pixel_double_help, "%c-D\t  Toggle Pixel Double Mode", 133);
	ADD_HELP_OPT (pixel_double_help);
	ADD_HELP_OPT ("");
	sprintf (command_help, " (Use %c-# for F#. i.e. %c-1 for F1)", 133, 133);
	ADD_HELP_OPT (command_help);
#endif
	nitems = opt;
	FullPaletteSave ();
	ExecMenutiny2 (NULL, TXT_KEYS, nitems, m, NULL);
	PaletteRestore ();
}

//------------------------------------------------------------------------------

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_onLevel ();

//deal with rear view - switch it on, or off, or whatever
void CheckRearView ()
{

	#define LEAVE_TIME 0x1000		//how long until we decide key is down	 (Used to be 0x4000)

	static int leave_mode;
	static fix entryTime;
#ifdef _DEBUG
	if (Controls.rear_viewDownCount) {		//key/button has gone down
#else
	if (Controls.rear_viewDownCount && !gameStates.render.bExternalView) {		//key/button has gone down
#endif
		if (gameStates.render.bRearView) {
			gameStates.render.bRearView = 0;
			if (gameStates.render.cockpit.nMode==CM_REAR_VIEW) {
				SelectCockpit (gameStates.render.cockpit.nModeSave);
				gameStates.render.cockpit.nModeSave = -1;
			}
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordRestoreRearView ();
		}
		else {
			gameStates.render.bRearView = 1;
			leave_mode = 0;		//means wait for another key
			entryTime = TimerGetFixedSeconds ();
			if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
				gameStates.render.cockpit.nModeSave = gameStates.render.cockpit.nMode;
				SelectCockpit (CM_REAR_VIEW);
			}
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordRearView ();
		}
	}
	else
		if (Controls.rear_view_down_state) {

			if (leave_mode==0 && (TimerGetFixedSeconds ()-entryTime)>LEAVE_TIME)
				leave_mode = 1;
		}
		else {

			//@@if (leave_mode==1 && gameStates.render.cockpit.nMode==CM_REAR_VIEW) {

			if (leave_mode==1 && gameStates.render.bRearView) {
				gameStates.render.bRearView = 0;
				if (gameStates.render.cockpit.nMode==CM_REAR_VIEW) {
					SelectCockpit (gameStates.render.cockpit.nModeSave);
					gameStates.render.cockpit.nModeSave = -1;
				}
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordRestoreRearView ();
			}
		}
}

//------------------------------------------------------------------------------

void ResetRearView (void)
{
	if (gameStates.render.bRearView) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordRestoreRearView ();
	}

	gameStates.render.bRearView = 0;

	if ((gameStates.render.cockpit.nMode < 0) || (gameStates.render.cockpit.nMode > 4)) {
		if (!(gameStates.render.cockpit.nModeSave == CM_FULL_COCKPIT || gameStates.render.cockpit.nModeSave == CM_STATUS_BAR || gameStates.render.cockpit.nModeSave == CM_FULL_SCREEN))
			gameStates.render.cockpit.nModeSave = CM_FULL_COCKPIT;
		SelectCockpit (gameStates.render.cockpit.nModeSave);
		gameStates.render.cockpit.nModeSave	= -1;
	}

}

//------------------------------------------------------------------------------

jmp_buf gameExitPoint;

void DoLunacyOn ();
void DoLunacyOff ();

extern char OldHomingState[20];
extern char old_IntMethod;

//turns off active cheats
void TurnCheatsOff ()
{
	int i;

	if (gameStates.app.cheats.bHomingWeapons)
		for (i=0;i<20;i++)
			WI_set_homingFlag (i, OldHomingState[i]);

	if (gameStates.app.cheats.bAcid)
	{
		gameStates.app.cheats.bAcid=0;
		gameStates.render.nInterpolationMethod=old_IntMethod;
	}

	gameStates.app.cheats.bMadBuddy = 0;
	gameStates.app.cheats.bBouncingWeapons=0;
   gameStates.app.cheats.bHomingWeapons=0;
	DoLunacyOff ();
	gameStates.app.cheats.bLaserRapidFire = 0;
	gameStates.app.cheats.bPhysics = 0;
	gameStates.app.cheats.bMonsterMode = 0;
	gameStates.app.cheats.bRobotsKillRobots=0;
	gameStates.app.cheats.bRobotsFiring = 1;
}

//turns off all cheats & resets cheater flag
void GameDisableCheats ()
{
	TurnCheatsOff ();
	gameStates.app.cheats.bEnabled=0;
}


//	GameSetup ()
// ----------------------------------------------------------------------------

void GameSetup (void)
{
DoLunacyOn ();		//	Copy values for insane into copy buffer in ai.c
DoLunacyOff ();		//	Restore true insane mode.
gameStates.app.bGameAborted = 0;
last_drawn_cockpit[0] = -1;				// Force cockpit to redraw next time a frame renders.
last_drawn_cockpit[1] = -1;				// Force cockpit to redraw next time a frame renders.
gameStates.app.bEndLevelSequence = 0;
GrPaletteStepLoad (NULL);
SetScreenMode (SCREEN_GAME);
ResetPaletteAdd ();
SetWarnFunc (ShowInGameWarning);
#if TRACE
//con_printf (CON_DEBUG, "   InitCockpit d:\temp\dm_test.\n");
#endif
InitCockpit ();
#if TRACE
//con_printf (CON_DEBUG, "   InitGauges d:\temp\dm_test.\n");
#endif
InitGauges ();
//DigiInitSounds ();
//keyd_repeat = 0;                // Don't allow repeat in game
keyd_repeat = 1;                // Do allow repeat in game
#if !defined (WINDOWS) && !defined (MACINTOSH)
//_MARK_ ("start of game");
#endif
#ifdef EDITOR
	if (gameData.segs.segments[gameData.objs.console->nSegment].nSegment == -1)      //tSegment no longer exists
		RelinkObject (OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM (Cursegp));

	if (!check_obj_seg (gameData.objs.console))
		MovePlayerToSegment (Cursegp,Curside);
#endif
gameData.objs.viewer = gameData.objs.console;
#if TRACE
//con_printf (CON_DEBUG, "   FlyInit d:\temp\dm_test.\n");
#endif
FlyInit (gameData.objs.console);
gameStates.app.bGameSuspended = 0;
ResetTime ();
gameData.time.xFrame = 0;			//make first frame zero
#ifdef EDITOR
if (gameData.missions.nCurrentLevel == 0) {			//not a real level
	InitPlayerStatsGame ();
	InitAIObjects ();
}
#endif
#if TRACE
//con_printf (CON_DEBUG, "   FixObjectSegs d:\temp\dm_test.\n");
#endif
GameFlushInputs ();
}


//------------------------------------------------------------------------------

void SetFunctionMode (int newFuncMode)
{
if (gameStates.app.nFunctionMode != newFuncMode) {
	gameStates.app.nLastFuncMode = gameStates.app.nFunctionMode;
	if ((gameStates.app.nFunctionMode = newFuncMode) == FMODE_GAME)
		gameStates.app.bEnterGame = 2;
#ifdef _DEBUG
	else if (newFuncMode == FMODE_MENU)
		gameStates.app.bEnterGame = 0;
#endif
	}
}


//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void game ()
{
GameSetup ();								// Replaces what was here earlier.
#ifdef MWPROFILE
ProfilerSetStatus (1);
#endif

if (setjmp (gameExitPoint)==0)	{
while (1) {
	int player_shields;
		// GAME LOOP!
	gameStates.app.bAutoMap = 0;
	gameStates.app.bConfigMenu = 0;
	if (gameData.objs.console != &gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject]) {
#if TRACE
	    //con_printf (CON_DEBUG,"gameData.multi.nLocalPlayer=%d nObject=%d",gameData.multi.nLocalPlayer,gameData.multi.players[gameData.multi.nLocalPlayer].nObject);
#endif
	    //Assert (gameData.objs.console == &gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].nObject]);
		}
	player_shields = gameData.multi.players[gameData.multi.nLocalPlayer].shields;
#ifdef WINDOWS
	{
	MSG msg;
	DoMessageStuff (&msg);		// Do Windows event handling.
	if (_RedrawScreen) {
		_RedrawScreen = FALSE;
		LoadPalette (szCurrentLevelPalette,1,1,0);
		GrPaletteStepLoad (NULL);
		}
	}
#endif
	gameStates.app.nExtGameStatus=GAMESTAT_RUNNING;
	if (!GameLoop (1, 1))		// Do game loop with rendering and reading controls.
		continue;
	//if the tPlayer is taking damage, give up guided missile control
	if (gameData.multi.players[gameData.multi.nLocalPlayer].shields != player_shields)
		ReleaseGuidedMissile (gameData.multi.nLocalPlayer);
	//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();	// Handle RedBook Audio Repeating.
	if (gameStates.app.bConfigMenu) {
		int double_save = Scanline_double;
	//WIN (mouse_set_mode (0);
		if (!(gameData.app.nGameMode&GM_MULTI)) {
			PaletteSave (); 
			ResetPaletteAdd ();	
			ApplyModifiedPalette (); 
			GrPaletteStepLoad (NULL); 
			}
		OptionsMenu ();
		if (Scanline_double != double_save)	
			InitCockpit ();
		if (!(gameData.app.nGameMode&GM_MULTI)) 
			PaletteRestore ();
		//WIN (mouse_set_mode (1);
		}
	if (gameStates.app.bAutoMap) {
		int save_w=Game_window_w,save_h=Game_window_h;
		DoAutomap (0, 0);
		gameStates.app.bEnterGame = 1;
		//	FlushInput ();
		//	StopPlayerMovement ();
		gameStates.video.nScreenMode=-1; 
		SetScreenMode (SCREEN_GAME);
		Game_window_w=save_w; 
		Game_window_h=save_h;
		InitCockpit ();
		last_drawn_cockpit[0] = -1;
		last_drawn_cockpit[1] = -1;
		}
	if ((gameStates.app.nFunctionMode != FMODE_GAME) && 
		 gameData.demo.bAuto && 
		 (gameData.demo.nState != ND_STATE_NORMAL))	{
		int choice, fmode;
		fmode = gameStates.app.nFunctionMode;
		SetFunctionMode (FMODE_GAME);
		PaletteSave ();
		ApplyModifiedPalette ();
		ResetPaletteAdd ();
		GrPaletteStepLoad (NULL);
		choice=ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
		PaletteRestore ();
		SetFunctionMode (fmode);
		if (choice)
			SetFunctionMode (FMODE_GAME);
		else {
			gameData.demo.bAuto = 0;
			NDStopPlayback ();
			SetFunctionMode (FMODE_MENU);
			} 
		}
	if ((gameStates.app.nFunctionMode != FMODE_GAME) &&  
		 (gameData.demo.nState != ND_STATE_PLAYBACK) &&  
		 (gameStates.app.nFunctionMode != FMODE_EDITOR)
	#ifdef NETWORK
			&& !gameStates.multi.bIWasKicked
	#endif
			) {
#if 1
		int choice = QuitSaveLoadMenu ();
#else
			int choice, fmode = gameStates.app.nFunctionMode;
			SetFunctionMode (FMODE_GAME);
			PaletteSave ();
			ApplyModifiedPalette ();
			ResetPaletteAdd ();
			GrPaletteStepLoad (NULL);
			gameStates.app.nExtGameStatus = GAMESTAT_ABORT_GAME;
			choice = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			PaletteRestore ();
			SetFunctionMode (fmode);
#endif
			if (choice)
				SetFunctionMode (FMODE_GAME);
			}
#ifdef NETWORK
	gameStates.multi.bIWasKicked = 0;
#endif
	if (gameStates.app.nFunctionMode != FMODE_GAME)
		longjmp (gameExitPoint,0);
#ifdef APPLE_DEMO
		if ((keydTime_when_last_pressed + (F1_0 * 60)) < TimerGetFixedSeconds ())		// idle in game for 1 minutes means exit
		longjmp (gameExitPoint,0);
#endif
		}
	}
#ifdef MWPROFILE
ProfilerSetStatus (0);
#endif
DigiStopAll ();
if (gameOpts->sound.bD1Sound) {
	gameOpts->sound.bD1Sound = 0;
	//DigiClose ();
	//DigiInit ();
	}
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
#ifdef NETWORK
MultiLeaveGame ();
#endif
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	NDStopPlayback ();
if (gameStates.render.cockpit.nModeSave!=-1) {
	gameStates.render.cockpit.nMode=gameStates.render.cockpit.nModeSave;
	gameStates.render.cockpit.nModeSave=-1;
	}
if (gameStates.app.nFunctionMode != FMODE_EDITOR)
	GrPaletteFadeOut (NULL,32,0);			// Fade out before going to menu
//@@	if ((!demo_playing) && (!multi_game) && (gameStates.app.nFunctionMode != FMODE_EDITOR))	{
//@@		scores_maybe_add_player (gameStates.app.bGameAborted);
//@@	}
#if !defined (WINDOWS) && !defined (MACINTOSH)
//_MARK_ ("end of game");
#endif
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
GameDisableCheats ();
UnloadCamBot ();
#ifdef APPLE_DEMO
SetFunctionMode (FMODE_EXIT);		// get out of game in Apple OEM version
#endif
}

// ----------------------------------------------------------------------------

void FreeHiresModels (void)
{
	int	i;

for (i = 0; i < gameData.models.nHiresModels; i++)
	OOF_FreeObject (gameData.models.hiresModels + i);
}

//-----------------------------------------------------------------------------
//called at the end of the program
void _CDECL_ CloseGame (void)
{
	static	int bGameClosed = 0;
	
if (bGameClosed)
	return;
bGameClosed = 1;
BMFreeExtraObjBitmaps ();
BMFreeExtraModels ();
LogErr ("unloading hires animations\n");
PiggyFreeHiresAnimations ();
PiggyBitmapPageOutAll (0);
LogErr ("freeing sound buffers\n");
DigiFreeSoundBufs ();
LogErr ("unloading hoard data\n");
FreeHoardData ();
LogErr ("unloading auxiliary poly model data\n");
G3FreeAllPolyModelItems ();
LogErr ("unloading hires models\n");
FreeHiresModels ();
LogErr ("unloading tracker list\n");
DestroyTrackerList ();
LogErr ("unloading lightmap data\n");
DestroyLightMaps ();
LogErr ("unloading particle data\n");
DestroyAllSmoke ();
LogErr ("unloading shield sphere data\n");
DestroySphere (&gameData.render.shield);
DestroySphere (&gameData.render.monsterball);
LogErr ("unloading inventory icons\n");
FreeInventoryIcons ();
LogErr ("unloading palettes\n");
FreePalettes ();
#ifdef WINDOWS
if (dd_VR_offscreen_buffer) {
	if (dd_grd_backcanv != dd_VR_offscreen_buffer) {
		dd_gr_free_canvas (dd_VR_offscreen_buffer);
	}
	dd_VR_offscreen_buffer = NULL;
	VR_offscreen_buffer = NULL;
}
#else
if (VR_offscreen_buffer)	{
	GrFreeCanvas (VR_offscreen_buffer);
	VR_offscreen_buffer = NULL;
}
#endif
LogErr ("unloading gauge data\n");
CloseGaugeCanvases ();
LogErr ("restoring effect bitmaps\n");
RestoreEffectBitmapIcons ();
#if !defined (MACINTOSH) && !defined (WINDOWS)
if (Game_cockpit_copy_code) {
	d_free (Game_cockpit_copy_code);
	Game_cockpit_copy_code = NULL;
}
#else
if (Game_cockpit_copy_code)
	Game_cockpit_copy_code = 0;
#endif
if (bmBackground.bm_texBuf) {
	LogErr ("unloading background bitmap\n");
	d_free (bmBackground.bm_texBuf);
	}
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
LogErr ("unloading custom background data\n");
NMFreeAltBg (1);
SaveBanList ();
FreeBanList ();
#if 0
if (fErr) {
	fclose (fErr);
	fErr = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef WINDOWS
dd_grs_canvas * GetCurrentGameScreen ()
{
	return &dd_VR_screen_pages[nVRCurrentPage];
}

#else

grs_canvas * GetCurrentGameScreen ()
{
	return &VR_screen_pages[nVRCurrentPage];
}
#endif

//-----------------------------------------------------------------------------

extern void kconfig_center_headset ();

#ifndef	NDEBUG
void speedtest_frame (void);
Uint32 Debug_slowdown=0;
#endif

#ifdef EDITOR
extern void player_follow_path (tObject *objP);
extern void check_create_player_path (void);

#endif

int Coop_view_player[2]={-1,-1};
int Cockpit_3d_view[2]={CV_NONE,CV_NONE};

//returns ptr to escort robot, or NULL
tObject *find_escort ()
{
	int i;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects[i].nType == OBJ_ROBOT)
			if (gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
				return &gameData.objs.objects[i];

	return NULL;
}

//-----------------------------------------------------------------------------

extern void ProcessSmartMinesFrame (void);
extern void DoSeismicStuff (void);

#ifndef RELEASE
int Saving_movie_frames=0;
int __Movie_frame_num=0;

#define MAX_MOVIE_BUFFER_FRAMES 250
#define MOVIE_FRAME_SIZE	 (320 * 200)

ubyte *Movie_frame_buffer;
int nMovieFrames;
ubyte Movie_pal[768];
char movie_path[50] = ".\\";

grsBitmap bmMovie;

void flush_movie_buffer ()
{
	char savename[128];
	int f;

	StopTime ();
#if TRACE
	//con_printf (CON_DEBUG,"Flushing movie bufferd:\temp\dm_test.");
#endif
	bmMovie.bm_texBuf = Movie_frame_buffer;

	for (f=0;f<nMovieFrames;f++) {
		sprintf (savename, "%sfrm%04d.pcx",movie_path,__Movie_frame_num);
		__Movie_frame_num++;
		pcx_write_bitmap (savename, &bmMovie);
		bmMovie.bm_texBuf += MOVIE_FRAME_SIZE;

		if (f % 5 == 0) {
#if TRACE
			//con_printf (CON_DEBUG,"%3d/%3d\10\10\10\10\10\10\10",f,nMovieFrames);
#endif
		}			
	}

	nMovieFrames=0;
#if TRACE
	//con_printf (CON_DEBUG,"done   \n");
#endif
	StartTime ();
}

//-----------------------------------------------------------------------------

void toggle_movie_saving ()
{
	int exit;

	Saving_movie_frames = !Saving_movie_frames;

	if (Saving_movie_frames) {
		tMenuItem m[1];

		memset (m, 0, sizeof (m));
		m[0].nType=NM_TYPE_INPUT; 
		m[0].text_len = 50; 
		m[0].text = movie_path;
		exit = ExecMenu (NULL, "Directory for movie frames?" , 1, & (m[0]), NULL, NULL);

		if (exit==-1) {
			Saving_movie_frames = 0;
			return;
		}

		while (isspace (movie_path[strlen (movie_path)-1]))
			movie_path[strlen (movie_path)-1] = 0;
		if (movie_path[strlen (movie_path)-1]!='\\' && movie_path[strlen (movie_path)-1]!=':')
			strcat (movie_path,"\\");


		if (!Movie_frame_buffer) {
			Movie_frame_buffer = d_malloc (MAX_MOVIE_BUFFER_FRAMES * MOVIE_FRAME_SIZE);
			if (!Movie_frame_buffer) {
				Int3 ();
				Saving_movie_frames=0;
			}

			nMovieFrames=0;

			bmMovie.bm_props.x = bmMovie.bm_props.y = 0;
			bmMovie.bm_props.w = 320;
			bmMovie.bm_props.h = 200;
			bmMovie.bm_props.nType = BM_LINEAR;
			bmMovie.bm_props.flags = 0;
			bmMovie.bm_props.rowsize = 320;
			//bmMovie.bm_handle = 0;

			GrPaletteRead (Movie_pal);		//get actual palette from the hardware

			if (gameData.demo.nState == ND_STATE_PLAYBACK)
				gameData.demo.bInterpolate = 0;
		}
	}
	else {
		flush_movie_buffer ();

		if (gameData.demo.nState == ND_STATE_PLAYBACK)
			gameData.demo.bInterpolate = 1;
	}

}

//-----------------------------------------------------------------------------

void save_movie_frame ()
{
	memcpy (Movie_frame_buffer+nMovieFrames*MOVIE_FRAME_SIZE,grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf,MOVIE_FRAME_SIZE);

	nMovieFrames++;

	if (nMovieFrames == MAX_MOVIE_BUFFER_FRAMES)
		flush_movie_buffer ();

}

#endif

//-----------------------------------------------------------------------------
//if water or fire level, make occasional sound
void DoAmbientSounds ()
{
	int has_water,has_lava;
	short sound;

	has_lava = (gameData.segs.segment2s[gameData.objs.console->nSegment].s2Flags & S2F_AMBIENT_LAVA);
	has_water = (gameData.segs.segment2s[gameData.objs.console->nSegment].s2Flags & S2F_AMBIENT_WATER);

	if (has_lava) {							//has lava
		sound = SOUND_AMBIENT_LAVA;
		if (has_water && (d_rand () & 1))	//both, pick one
			sound = SOUND_AMBIENT_WATER;
	}
	else if (has_water)						//just water
		sound = SOUND_AMBIENT_WATER;
	else
		return;

	if (((d_rand () << 3) < gameData.time.xFrame)) {						//play the sound
		fix volume = d_rand () + f1_0/2;
		DigiPlaySample (sound, volume);
	}
}

//-----------------------------------------------------------------------------

void GetSlowTick (void)
{
gameStates.app.nSDLTicks = SDL_GetTicks ();
gameStates.app.nDeltaTime = gameStates.app.nSDLTicks - gameStates.app.nLastTick;
if (gameStates.app.b40fpsTick = (gameStates.app.nDeltaTime >= 25))
	gameStates.app.nLastTick = gameStates.app.nSDLTicks;
}

//-----------------------------------------------------------------------------
// -- extern void lightning_frame (void);

void GameRenderFrame ();
extern void OmegaChargeFrame (void);

extern time_t t_currentTime, t_savedTime;
extern int flFrameTime;
int screenShotIntervals [] = {0, 1, 3, 5, 10, 15, 30, 60};

void FlickerLights ();

int GameLoop (int RenderFlag, int ReadControlsFlag)
{
	int	h;

gameStates.app.bGameRunning = 1;
gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
GetSlowTick ();
#ifndef	NDEBUG
//	Used to slow down frame rate for testing things.
//	RenderFlag = 1; // DEBUG
//	GrPaletteStepLoad (gamePalette);
if (Debug_slowdown) {
#if 1
	time_t	t = SDL_GetTicks ();
	while (SDL_GetTicks () - t < Debug_slowdown)
		;
#else
	int	h, i, j=0;

	for (h=0; h<Debug_slowdown; h++)
		for (i=0; i<1000; i++)
			j += i;
#endif
}
#endif

#ifdef WINDOWS
{
	static int desc_dead_countdown=100;   /*  used if tPlayer shouldn't be playing */

	if (desc_id_exit_num) {				 // are we supposed to be checking
		if (!(--desc_dead_countdown)) {// if so, at zero, then pull the plug
			char time_str[32], time_str2[32];

			_ctime (&t_savedTime, time_str);
			_ctime (&t_currentTime, time_str2);

			Error ("EXPIRES %s.  YOUR TIME %s.\n", time_str, time_str2);
			Error ("Loading overlay -- error number: %d\n", (int)desc_id_exit_num);
		}
	}
}
#endif

#ifndef RELEASE
	if (FindArg ("-invulnerability")) {
		gameData.multi.players[gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
		SetSpherePulse (gameData.multi.spherePulse + gameData.multi.nLocalPlayer, 0.02f, 0.5f);
		}
#endif
	if (!MultiProtectGame ()) {
		SetFunctionMode (FMODE_MENU);
		return 1;
		}
	AutoBalanceTeams ();
	MultiSendTyping ();
	MultiSendWeapons (0);
	//MultiSendMonsterball (0, 0);
	UpdatePlayerStats ();
	DiminishPaletteTowardsNormal ();		//	Should leave palette effect up for as long as possible by putting right before render.
//LogErr ("DoAfterburnerStuff\n");
	DoAfterburnerStuff ();
//LogErr ("DoCloakStuff\n");
	DoCloakStuff ();
//LogErr ("DoInvulnerableStuff\n");
	DoInvulnerableStuff ();
//LogErr ("DoInvulnerableStuff \n");
	RemoveObsoleteStuckObjects ();
//LogErr ("RemoveObsoleteStuckObjects \n");
	InitAIFrame ();
//LogErr ("InitAIFrame \n");
	DoFinalBossFrame ();
// -- lightning_frame ();
	// -- recharge_energy_frame ();

	if ((gameData.multi.players[gameData.multi.nLocalPlayer].flags & (PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON)) == (PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON)) {
		static int turned_off=0;
		gameData.multi.players[gameData.multi.nLocalPlayer].energy -= (gameData.time.xFrame*3/8);
		if (gameData.multi.players[gameData.multi.nLocalPlayer].energy < i2f (10)) {
			if (!turned_off) {
				gameData.multi.players[gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
#ifdef NETWORK
				if (gameData.app.nGameMode & GM_MULTI)
					MultiSendFlags ((char) gameData.multi.nLocalPlayer);
#endif
			}
		}
		else
			turned_off = 0;

		if (gameData.multi.players[gameData.multi.nLocalPlayer].energy <= 0) {
			gameData.multi.players[gameData.multi.nLocalPlayer].energy = 0;
			gameData.multi.players[gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI) {
//con_printf (CON_DEBUG, "      MultiSendFlags\n");
				MultiSendFlags ((char) gameData.multi.nLocalPlayer);
				}
#endif
		}
	}

#ifdef EDITOR
	check_create_player_path ();
	player_follow_path (gameData.objs.console);
#endif

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI) {
		AddServerToTracker ();
      MultiDoFrame ();
		CheckMonsterballScore ();
		if (netGame.PlayTimeAllowed && ThisLevelTime>=i2f ((netGame.PlayTimeAllowed*5*60)))
          MultiCheckForKillGoalWinner ();
		else 
			MultiCheckForEntropyWinner ();
     }
#endif

	if (RenderFlag) {
		if (gameStates.render.cockpit.bRedraw) {			//screen need redrawing?
			InitCockpit ();
			gameStates.render.cockpit.bRedraw=0;
		}
//LogErr ("GameRenderFrame\n");
		GameRenderFrame ();
		gameStates.app.bUsingConverter = 0;
		//show_extra_views ();		//missile view, buddy bot, etc.

#ifndef RELEASE
		if (Saving_movie_frames)
			save_movie_frame ();
#endif

	}
	CalcFrameTime ();
	DeadPlayerFrame ();
	if (gameData.demo.nState != ND_STATE_PLAYBACK) {
		DoReactorDeadFrame ();
		}	
//LogErr ("ProcessSmartMinesFrame \n");
	ProcessSmartMinesFrame ();
//LogErr ("DoSeismicStuff\n");
	DoSeismicStuff ();
//LogErr ("DoSmokeFrame \n");
	DoSmokeFrame ();
//LogErr ("DoAmbientSounds\n");
	DoAmbientSounds ();
#ifndef NDEBUG
	if (Speedtest_on)
		speedtest_frame ();
#endif
	if (ReadControlsFlag) {
//LogErr ("ReadControls\n");
		ReadControls ();
		}
	else
		memset (&Controls, 0, sizeof (Controls));
//LogErr ("DropPowerups\n");
	DropPowerups ();
	gameData.time.xGame += gameData.time.xFrame;
	if (f2i (gameData.time.xGame)/10 != f2i (gameData.time.xGame-gameData.time.xFrame)/10) {
		}
	if (gameData.time.xGame < 0 || gameData.time.xGame > i2f (0x7fff - 600)) {
		gameData.time.xGame = gameData.time.xFrame;	//wrap when goes negative, or gets within 10 minutes
		}
#ifndef NDEBUG
	if (FindArg ("-checktime") != 0)
		if (gameData.time.xGame >= i2f (600))		//wrap after 10 minutes
			gameData.time.xGame = gameData.time.xFrame;
#endif
#ifdef NETWORK
   if ((gameData.app.nGameMode & GM_MULTI) && netGame.PlayTimeAllowed)
       ThisLevelTime +=gameData.time.xFrame;
#endif
//LogErr ("DigiSyncSounds\n");
	DigiSyncSounds ();
	if (gameStates.app.bEndLevelSequence) {
		DoEndLevelFrame ();
		PowerupGrabCheatAll ();
		DoSpecialEffects ();
		return 1;					//skip everything else
		}
	if (gameData.demo.nState != ND_STATE_PLAYBACK) {
//LogErr ("DoExplodingWallFrame\n");
		DoExplodingWallFrame ();
		}
	if ((gameData.demo.nState != ND_STATE_PLAYBACK) || (gameData.demo.nVcrState != ND_STATE_PAUSED)) {
//LogErr ("DoSpecialEffects\n");
		DoSpecialEffects ();
//LogErr ("WallFrameProcess\n");
		WallFrameProcess ();
//LogErr ("TriggersFrameProcess\n");
		TriggersFrameProcess ();
		}
	if (gameData.reactor.bDestroyed && (gameData.demo.nState==ND_STATE_RECORDING)) {
			NDRecordControlCenterDestroyed ();
		}
//LogErr ("FlashFrame\n");
	UpdateFlagClips ();
	MultiSetFlagPos ();
	SetPlayerPaths ();
	FlashFrame ();
	if (gameData.demo.nState == ND_STATE_PLAYBACK) {
		NDPlayBackOneFrame ();
		if (gameData.demo.nState != ND_STATE_PLAYBACK) {
			longjmp (gameExitPoint, 0);		// Go back to menu
			}
		}
	else { // Note the link to above!
		gameData.multi.players[gameData.multi.nLocalPlayer].homingObjectDist = -1;		//	Assume not being tracked.  LaserDoWeaponSequence modifies this.
//LogErr ("MoveAllObjects\n");
		if (!MoveAllObjects ())
			return 0;
//LogErr ("PowerupGrabCheatAll \n");
		PowerupGrabCheatAll ();
		if (gameStates.app.bEndLevelSequence)	//might have been started during move
			return 1;
//LogErr ("FuelcenUpdateAll\n");
		FuelcenUpdateAll ();
//LogErr ("DoAIFrameAll\n");
		DoAIFrameAll ();
		if (AllowedToFireLaser ()) {
//LogErr ("FireLaser\n");
			FireLaser ();				// Fire Laser!
			}
		if (gameData.app.fusion.xAutoFireTime) {
			if (gameData.weapons.nPrimary != FUSION_INDEX)
				gameData.app.fusion.xAutoFireTime = 0;
			else if (gameData.time.xGame + flFrameTime/2 >= gameData.app.fusion.xAutoFireTime) {
				gameData.app.fusion.xAutoFireTime = 0;
				gameData.app.nGlobalLaserFiringCount = 1;
				}
			else {
				vmsVector	rand_vec;
				fix			bump_amount;
				static time_t t0 = 0;
				time_t t = SDL_GetTicks ();
				if (t - t0 < 30)
					return 1;
				t0 = t;
				gameData.app.nGlobalLaserFiringCount = 0;
				gameData.objs.console->mType.physInfo.rotVel.x += (d_rand () - 16384)/8;
				gameData.objs.console->mType.physInfo.rotVel.z += (d_rand () - 16384)/8;
				MakeRandomVector (&rand_vec);
				bump_amount = F1_0*4;
				if (gameData.app.fusion.xCharge > F1_0*2)
					bump_amount = gameData.app.fusion.xCharge*4;
				BumpOneObject (gameData.objs.console, &rand_vec, bump_amount);
				}
			}
		if (gameData.app.nGlobalLaserFiringCount) {
			//	Don't cap here, gets capped in CreateNewLaser and is based on whether in multiplayer mode, MK, 3/27/95
			// if (gameData.app.fusion.xCharge > F1_0*2)
			// 	gameData.app.fusion.xCharge = F1_0*2;
			gameData.app.nGlobalLaserFiringCount -= LaserFireLocalPlayer ();	//LaserFireObject (gameData.multi.players[gameData.multi.nLocalPlayer].nObject, gameData.weapons.nPrimary);
			}
		if (gameData.app.nGlobalLaserFiringCount < 0)
			gameData.app.nGlobalLaserFiringCount = 0;
		}
	if (gameStates.render.bDoAppearanceEffect) {
		CreatePlayerAppearanceEffect (gameData.objs.console);
		gameStates.render.bDoAppearanceEffect = 0;
#ifdef NETWORK
	if ((gameData.app.nGameMode & GM_MULTI) && netGame.invul) {
		gameData.multi.players[gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
		gameData.multi.players[gameData.multi.nLocalPlayer].invulnerableTime = gameData.time.xGame-i2f (27);
		bFakingInvul=1;
		SetSpherePulse (gameData.multi.spherePulse + gameData.multi.nLocalPlayer, 0.02f, 0.5f);
		}
#endif
	}
//LogErr ("OmegaChargeFrame \n");
OmegaChargeFrame ();
//LogErr ("SlideTextures \n");
SlideTextures ();
//LogErr ("FlickerLights \n");
FlickerLights ();
//LogErr ("\n");
if (!(gameData.app.bGamePaused || gameStates.menus.nInMenu))
	if (h = screenShotIntervals [gameOpts->app.nScreenShotInterval]) {
		static	time_t	t0 = 0;

		if (gameStates.app.nSDLTicks - t0 >= h * 1000) {
			t0 = gameStates.app.nSDLTicks;
			bSaveScreenShot = 1;
			SaveScreenShot (0, 0);
			}
		}
return 1;
}

//-----------------------------------------------------------------------------
//!!extern int Goal_blue_segnum,Goal_red_segnum;
//!!extern int Hoard_goal_eclip;
//!!
//!!//do cool pulsing lights in hoard goals
//!!hoard_light_pulse ()
//!!{
//!!	if (gameData.app.nGameMode & GM_HOARD) {
//!!		fix light;
//!!		int frame;
//!!
//!!		frame = gameData.eff.pEffects[Hoard_goal_eclip]..nCurFrame;
//!!
//!!		frame++;
//!!
//!!		if (frame >= gameData.eff.pEffects[Hoard_goal_eclip].vc.nFrameCount)
//!!			frame = 0;
//!!
//!!		light = abs (frame - 5) * f1_0 / 5;
//!!
//!!		gameData.segs.segment2s[Goal_red_segnum].static_light = gameData.segs.segment2s[Goal_blue_segnum].static_light = light;
//!!	}
//!!}

void ComputeSlideSegs (void)
{
	int	nSegment, nSide, bIsSlideSeg, tmn;

gameData.segs.nSlideSegs = 0;
for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
	bIsSlideSeg = 0;
	for (nSide = 0; nSide < 6; nSide++) {
		tmn = gameData.segs.segments [nSegment].sides [nSide].nBaseTex;
		if (gameData.pig.tex.pTMapInfo [tmn].slide_u  || 
			 gameData.pig.tex.pTMapInfo [tmn].slide_v) {
			if (!bIsSlideSeg) {
				bIsSlideSeg = 1;
				gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSegment = nSegment;
				gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSides = 0;
				}	
			gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSides |= (1 << nSide);
			}
		}
	if (bIsSlideSeg)
		gameData.segs.nSlideSegs++;
	}	
gameData.segs.bHaveSlideSegs = 1;
}

//	-----------------------------------------------------------------------------

void SlideTextures (void)
{
	int	nSegment, nSide, h, i, j, tmn;
	ubyte	sides;
	tSide	*sideP;
	uvl	*uvlP;
	fix	slideU, slideV;

if (!gameData.segs.bHaveSlideSegs)
	ComputeSlideSegs ();
for (h = 0; h < gameData.segs.nSlideSegs; h++) {
	nSegment = gameData.segs.slideSegs [h].nSegment;
	sides = gameData.segs.slideSegs [h].nSides;
	for (nSide = 0, sideP = gameData.segs.segments [nSegment].sides; nSide < 6; nSide++, sideP++) {
		if (!(sides & (1 << nSide)))
			continue;
		tmn = sideP->nBaseTex;
		slideU = (fix) gameData.pig.tex.pTMapInfo [tmn].slide_u;
		slideV = (fix) gameData.pig.tex.pTMapInfo [tmn].slide_v;
		if (!(slideU || slideV))
			continue;
		slideU = FixMul (gameData.time.xFrame, slideU << 8);
		slideV = FixMul (gameData.time.xFrame, slideV << 8);
		for (i = 0, uvlP = sideP->uvls; i < 4; i++) {
			uvlP [i].u += slideU;
			if (uvlP [i].u > f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].u -= f1_0;
				}
			else if (uvlP [i].u < -f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].u += f1_0;
				}
			uvlP [i].v += slideV;
			if (uvlP [i].v > f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].v -= f1_0;
				}
			else if (uvlP [i].v < -f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].v += f1_0;
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
int flFrameTime = 0;

void FireLaser ()
{
	int i = primaryWeaponToWeaponInfo[gameData.weapons.nPrimary];
	gameData.app.nGlobalLaserFiringCount += WI_fire_count (i) * (Controls.fire_primary_state || Controls.fire_primaryDownCount);
	if ((gameData.weapons.nPrimary == FUSION_INDEX) && (gameData.app.nGlobalLaserFiringCount)) {
		if ((gameData.multi.players[gameData.multi.nLocalPlayer].energy < F1_0*2) && (gameData.app.fusion.xAutoFireTime == 0)) {
			gameData.app.nGlobalLaserFiringCount = 0;
		} else {
			flFrameTime += gameData.time.xFrame;
			if (gameData.app.fusion.xCharge == 0)
				gameData.multi.players[gameData.multi.nLocalPlayer].energy -= F1_0*2;
			gameData.app.fusion.xCharge += flFrameTime;
			gameData.multi.players[gameData.multi.nLocalPlayer].energy -= flFrameTime;
			if (gameData.multi.players[gameData.multi.nLocalPlayer].energy <= 0) {
				gameData.multi.players[gameData.multi.nLocalPlayer].energy = 0;
				gameData.app.fusion.xAutoFireTime = gameData.time.xGame -1;	//	Fire now!
			} else
				gameData.app.fusion.xAutoFireTime = gameData.time.xGame + flFrameTime/2 + 1;
			if (gameStates.limitFPS.bFusion && !gameStates.app.b40fpsTick)
				return;
			if (gameData.app.fusion.xCharge < F1_0*2)
				PALETTE_FLASH_ADD (gameData.app.fusion.xCharge >> 11, 0, gameData.app.fusion.xCharge >> 11);
			else
				PALETTE_FLASH_ADD (gameData.app.fusion.xCharge >> 11, gameData.app.fusion.xCharge >> 11, 0);
			if (gameData.time.xGame < gameData.app.fusion.xLastSoundTime)		//gametime has wrapped
				gameData.app.fusion.xNextSoundTime = gameData.app.fusion.xLastSoundTime = gameData.time.xGame;
			if (gameData.app.fusion.xNextSoundTime < gameData.time.xGame) {
				if (gameData.app.fusion.xCharge > F1_0*2) {
					DigiPlaySample (11, F1_0);
					ApplyDamageToPlayer (gameData.objs.console, gameData.objs.console, d_rand () * 4);
				} else {
					CreateAwarenessEvent (gameData.objs.console, PA_WEAPON_ROBOT_COLLISION);
					DigiPlaySample (SOUND_FUSION_WARMUP, F1_0);
					#ifdef NETWORK
					if (gameData.app.nGameMode & GM_MULTI)
						MultiSendPlaySound (SOUND_FUSION_WARMUP, F1_0);
					#endif
				}
				gameData.app.fusion.xLastSoundTime = gameData.time.xGame;
				gameData.app.fusion.xNextSoundTime = gameData.time.xGame + F1_0/8 + d_rand ()/4;
			}
		flFrameTime = 0;
		}
	}
}


//	-------------------------------------------------------------------------------------------------------
//	If tPlayer is close enough to nObject, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void PowerupGrabCheat (tObject *player, int nObject)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert (gameData.objs.objects[nObject].nType == OBJ_POWERUP);

	powerup_size = gameData.objs.objects[nObject].size;
	player_size = player->size;

	dist = VmVecDistQuick (&gameData.objs.objects[nObject].position.vPos, &player->position.vPos);

	if ((dist < 2* (powerup_size + player_size)) && !(gameData.objs.objects[nObject].flags & OF_SHOULD_BE_DEAD)) {
		vmsVector	collision_point;

		VmVecAvg (&collision_point, &gameData.objs.objects[nObject].position.vPos, &player->position.vPos);
		CollidePlayerAndPowerup (player, gameData.objs.objects + nObject, &collision_point);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this tSegment, pick them up at up to twice pickuppable distance based on dot product
//	from tPlayer to powerup and tPlayer's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the tPlayer gets there.
void PowerupGrabCheatAll (void)
{
	tSegment	*segp;
	int		nObject;

	segp = gameData.segs.segments + gameData.objs.console->nSegment;
	nObject = segp->objects;

	while (nObject != -1) {
		if (gameData.objs.objects[nObject].nType == OBJ_POWERUP)
			PowerupGrabCheat (gameData.objs.console, nObject);
		nObject = gameData.objs.objects[nObject].next;
	}

}

int	nLastLevelPathCreated = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for tPlayer from current tSegment to goal tSegment.
//	Return true if path created, else return false.
int mark_player_path_to_segment (int nSegment)
{
	int		i;
	tObject	*objP = gameData.objs.console;
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (nLastLevelPathCreated == gameData.missions.nCurrentLevel) {
		return 0;
	}

	nLastLevelPathCreated = gameData.missions.nCurrentLevel;

	if (CreatePathPoints (objP, objP->nSegment, nSegment, gameData.ai.freePointSegs, &player_path_length, 100, 0, 0, -1) == -1) {
#if TRACE
		//con_printf (CON_DEBUG, "Unable to form path of length %i for myself\n", 100);
#endif
		return 0;
	}

	player_hide_index = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
	gameData.ai.freePointSegs += player_path_length;

	if ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs) + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
#if TRACE
		//con_printf (1, "Can't create path.  Not enough tPointSegs.\n");
#endif
		AIResetAllPaths ();
		return 0;
	}

	for (i=1; i<player_path_length; i++) {
		short			nSegment, nObject;
		vmsVector	seg_center;
		tObject		*obj;

		nSegment = gameData.ai.pointSegs[player_hide_index+i].nSegment;
#if TRACE
		//con_printf (CON_DEBUG, "%3i ", nSegment);
#endif
		seg_center = gameData.ai.pointSegs[player_hide_index+i].point;

		nObject = CreateObject (OBJ_POWERUP, POW_ENERGY, -1, nSegment, &seg_center, &vmdIdentityMatrix,
									  gameData.objs.pwrUp.info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP, 1);
		if (nObject == -1) {
			Int3 ();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj = &gameData.objs.objects[nObject];
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info[objP->id].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->lifeleft = F1_0*100 + d_rand () * 4;
	}
#if TRACE
	//con_printf (CON_DEBUG, "\n");
#endif
	return 1;
}

//-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int CreateSpecialPath (void)
{
	int	i,j;

	//	---------- Find exit doors ----------
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (gameData.segs.segments[i].children[j] == -2) {
#if TRACE
				//con_printf (CON_DEBUG, "Exit at tSegment %i\n", i);
#endif
				return mark_player_path_to_segment (i);
			}

	return 0;
}

#endif


//-----------------------------------------------------------------------------
#ifndef RELEASE
int	Max_obj_count_mike = 0;

//	Shows current number of used gameData.objs.objects.
void show_freeObjects (void)
{
	if (!(gameData.app.nFrameCount & 8)) {
		int	i;
		int	count=0;

#if TRACE
		//con_printf (CON_DEBUG, "gameData.objs.nLastObject = %3i, MAX_OBJECTS = %3i, now used = ", gameData.objs.nLastObject, MAX_OBJECTS);
#endif
		for (i=0; i<=gameData.objs.nLastObject; i++)
			if (gameData.objs.objects[i].nType != OBJ_NONE)
				count++;
#if TRACE
		//con_printf (CON_DEBUG, "%3i", count);
#endif
		if (count > Max_obj_count_mike) {
			Max_obj_count_mike = count;
#if TRACE
			//con_printf (CON_DEBUG, " ***");
#endif
		}
#if TRACE
		//con_printf (CON_DEBUG, "\n");
#endif
	}

}

#endif

//-----------------------------------------------------------------------------
#ifdef WINDOWS
void game_win_init_cockpit_mask (int sram)
{
	if (dd_VR_offscreen_buffer && dd_VR_offscreen_buffer != dd_grd_backcanv) {
		dd_gr_free_canvas (dd_VR_offscreen_buffer);
	}

	if (GRMODEINFO (paged) && !GRMODEINFO (dbuf)) {
	//	Here we will make the VR_offscreen_buffer the 2nd page and hopefully
	//	we can just flip it, saving a blt.
		Int3 ();
	}
	else if (GRMODEINFO (dbuf)||GRMODEINFO (emul)) {
	//	The offscreen buffer will be created.  We will just blt this
	//	to the screen (which may be blted to the primary surface)
		if (grdCurScreen->sc_h < 200) {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (grdCurScreen->sc_w, 200);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
			if (sram) {
				DDFreeSurface (dd_VR_offscreen_buffer->lpdds);
				dd_VR_offscreen_buffer->lpdds = DDCreateSysMemSurface (grdCurScreen->sc_w, 200);
				dd_VR_offscreen_buffer->sram = 1;
			}
		}
		else {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (grdCurScreen->sc_w, grdCurScreen->sc_h);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
			if (sram) {
				DDFreeSurface (dd_VR_offscreen_buffer->lpdds);
				dd_VR_offscreen_buffer->lpdds = DDCreateSysMemSurface (grdCurScreen->sc_w, grdCurScreen->sc_h);
				dd_VR_offscreen_buffer->sram = 1;
			}
		}
	}

	DDGrInitSubCanvas (&dd_VR_render_buffer[0], dd_VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	DDGrInitSubCanvas (&dd_VR_render_buffer[1], dd_VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);

	GameInitRenderSubBuffers (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
}

//@@void game_win_init_cockpit_mask ()
//@@{
//@@	char title_pal[768];
//@@	dd_grs_canvas ccanv;
//@@	int pcx_error;
//@@	LPDIRECTDRAWSURFACE dds;
//@@
//@@	dds = DDCreateSurface (GRMODEINFO (w), GRMODEINFO (h), 1);
//@@	Assert (dds != NULL);
//@@
//@@	_lpDDSMask = dds;
//@@	ccanv.lpdds = dds;
//@@	dd_gr_reinit_canvas (&ccanv);
//@@
//@@	DDGrSetCurrentCanvas (&ccanv);
//@@	DDGRLOCK (dd_grd_curcanv)
//@@	{
//@@		if (W95DisplayMode == SM95_640x480x8) {
//@@			pcx_error=pcx_read_bitmap ("MASKB.PCX", &grdCurCanv->cv_bitmap,
//@@				grdCurCanv->cv_bitmap.bm_props.nType,
//@@				title_pal);
//@@		}
//@@		else {
//@@			pcx_error=pcx_read_bitmap ("MASK.PCX", &grdCurCanv->cv_bitmap,
//@@				grdCurCanv->cv_bitmap.bm_props.nType,
//@@				title_pal);
//@@		}
//@@	}
//@@	DDGRUNLOCK (dd_grd_curcanv);
//@@
//@@	Assert (pcx_error == PCX_ERROR_NONE);
//@@	Game_cockpit_copy_code = (ubyte *) (0xABADC0DE);
//@@}

#endif

//-----------------------------------------------------------------------------
/*
 * reads a flickering_light structure from a CFILE
 */
void ReadFlickeringLight (flickering_light *fl, CFILE *fp)
{
	fl->nSegment = CFReadShort (fp);
	fl->nSide = CFReadShort (fp);
	fl->mask = CFReadInt (fp);
	fl->timer = CFReadFix (fp);
	fl->delay = CFReadFix (fp);
}

//-----------------------------------------------------------------------------
//eof
