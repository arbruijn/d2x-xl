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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "pstypes.h"
#include "console.h"
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "objrender.h"
#include "objsmoke.h"
#include "transprender.h"
#include "lightning.h"
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
#include "light.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "loadgame.h"
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

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "textdata.h"
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
#include "cheats.h"
#include "rle.h"
#include "digi.h"
#include "sphere.h"
#include "hiresmodels.h"
#include "dropobject.h"
#include "monsterball.h"
#include "dropobject.h"
#include "trackobject.h"
#include "slowmotion.h"

u_int32_t nCurrentVGAMode;

void GamePaletteStepUp (int r, int g, int b);


#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _DEBUG

void FreeHoardData (void);
int ReadControls (void);		// located in gamecntl.c
void DoFinalBossFrame (void);

#ifdef _DEBUG
int	MarkCount = 0;                 // number of debugging marks set
int	Speedtest_startTime;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	SpeedtestCount=0;				//	number of times to do the debug test.
#endif

fix ThisLevelTime=0;

#if defined (TIMER_TEST) && defined (_DEBUG)
fix TimerValue,actual_lastTimerValue,_last_frametime;
int gameData.time.xStops,gameData.time.xStarts;
int gameData.time.xStopped,gameData.time.xStarted;
#endif

#ifndef MACINTOSH
ubyte * bGameCockpitCopyCode = NULL;
#else
ubyte bGameCockpitCopyCode = 0;
ubyte bScanlineDouble = 1;
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

#ifdef _DEBUG                          //these only exist if debugging

int bGameDoubleBuffer = 1;     //double buffer by default
fix xFixedFrameTime = 0;          //if non-zero, set frametime to this

#endif

grsBitmap bmBackground;

#define BACKGROUND_NAME "statback.pcx"

//	Function prototypes for GAME.C exclusively.

int GameLoop (int RenderFlag, int bReadControls);
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

void FillBackground ();

#ifdef _DEBUG
void ShowFrameRate (void);
void ftoa (char *string, fix f);
#endif

//	==============================================================================================

void LoadBackgroundBitmap ()
{
	int pcx_error;

if (bmBackground.bmTexBuf)
	D2_FREE (bmBackground.bmTexBuf);
bmBackground.bmTexBuf=NULL;
pcx_error = PCXReadBitmap (gameStates.app.cheats.bJohnHeadOn ? "johnhead.pcx" : BACKGROUND_NAME,
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
/*---*/LogErr ("  exploding walls...\n");
InitExplodingWalls ();
/*---*/LogErr ("  particle systems...\n");
InitSmoke ();
/*---*/LogErr ("  loading background bitmap...\n");
LoadBackgroundBitmap ();
nClearWindow = 2;		//	do portal only window clear.
InitDetailLevels (gameStates.app.nDetailLevel);
gameStates.render.color.bRenderLightMaps =
	gameStates.render.color.bLightMapsOk && 
	gameOpts->render.color.bAmbientLight && 
	gameOpts->render.color.bUseLightMaps;
gameStates.ogl.bGlTexMerge = 
	gameOpts->ogl.bGlTexMerge && 
	gameStates.render.textures.bGlsTexMergeOk;
fpDrawTexPolyMulti = gameStates.render.color.bRenderLightMaps ? G3DrawTexPolyLightmap : G3DrawTexPolyMulti;
}

//------------------------------------------------------------------------------

void ResetPaletteAdd ()
{
gameStates.ogl.palAdd.red = 
gameStates.ogl.palAdd.green =
gameStates.ogl.palAdd.blue	= 0;
gameData.render.xFlashEffect = 0;
gameData.render.xTimeFlashLastPlayed = 0;
GrPaletteStepUp (0, 0, 0);
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
}

//------------------------------------------------------------------------------

void ShowInGameWarning (char *s)
{
if (grdCurScreen) {
	char	*hs, *ps = strstr (s, "Error");

	if (ps > s) {	//skip trailing non alphanum chars
		for (hs = ps - 1; (hs > s) && !isalnum (*hs); hs++)
			;
		if (hs > s)
			ps = NULL;
		}
	if (!(IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StopTime ();
	gameData.menu.warnColor = RED_RGBA;
	gameData.menu.colorOverride = gameData.menu.warnColor;
	if (!ps)
		ExecMessageBox (TXT_WARNING, NULL, -3, s, " ", TXT_OK);
	else {
		for (ps += 5; *ps && !isalnum (*ps); ps++)
			;
		ExecMessageBox (TXT_ERROR, NULL, -3, ps, " ", TXT_OK);
		}
	gameData.menu.colorOverride = 0;
	if (!((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StartTime ();
	}
}

//------------------------------------------------------------------------------
//these should be in gr.h
#define cv_w  cvBitmap.bmProps.w
#define cv_h  cvBitmap.bmProps.h

extern void NDRecordCockpitChange (int);

//------------------------------------------------------------------------------
//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void InitCockpit ()
{
	if (gameStates.video.nScreenMode != SCREEN_GAME)
		return;

//Initialize the on-screen canvases

if (gameData.demo.nState==ND_STATE_RECORDING)
	NDRecordCockpitChange (gameStates.render.cockpit.nMode);
if (gameStates.render.vr.nRenderMode != VR_NONE)
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
if (gameStates.video.nScreenMode == SCREEN_EDITOR)
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
	);
GrSetCurFont (GAME_FONT);

if (bGameCockpitCopyCode)
	D2_FREE (bGameCockpitCopyCode);
bGameCockpitCopyCode  = NULL;
switch (gameStates.render.cockpit.nMode) {
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:
     	gameData.render.window.hMax = (grdCurScreen->scHeight * 2) / 3;
		if (gameData.render.window.h > gameData.render.window.hMax)
			gameData.render.window.h = gameData.render.window.hMax;
		if (gameData.render.window.w > gameData.render.window.wMax)
			gameData.render.window.w = gameData.render.window.wMax;
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
		GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
		break;

	case CM_FULL_SCREEN:
		gameData.render.window.hMax = grdCurScreen->scHeight;
		gameData.render.window.h = gameData.render.window.hMax;
		gameData.render.window.w = gameData.render.window.wMax;
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
		GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
		break;

	case CM_STATUS_BAR:
		{
		int h = gameData.pig.tex.bitmaps [0][gameData.pig.tex.cockpitBmIndex [CM_STATUS_BAR + (gameStates.video.nDisplayMode ? (gameData.models.nCockpits / 2) : 0)].index].bmProps.h;
		if (gameStates.app.bDemoData)
			h *= 2;
		if (grdCurScreen->scHeight > 480)
			h = (int) ((double) h * (double) grdCurScreen->scHeight / 480.0);
     	gameData.render.window.hMax = grdCurScreen->scHeight - h;
		gameData.render.window.h = gameData.render.window.hMax;
		gameData.render.window.w = gameData.render.window.wMax;
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
		GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
		}
		break;

	case CM_LETTERBOX: {
		int x = 0; 
		int w = gameStates.render.vr.buffers.render[0].cvBitmap.bmProps.w;		//VR_render_width;
		int h = (int) ((gameStates.render.vr.buffers.render[0].cvBitmap.bmProps.h * 7) / 10 / ((double) grdCurScreen->scHeight / (double) grdCurScreen->scWidth / 0.75));
		int y = (gameStates.render.vr.buffers.render[0].cvBitmap.bmProps.h - h) / 2;
		GameInitRenderSubBuffers (x, y, w, h);
		break;
		}

	}

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
);
gameStates.render.cockpit.nShieldFlash = 0;
}

//------------------------------------------------------------------------------

//selects a given cockpit (or lack of one).  See types in game.h
void SelectCockpit (int nMode)
{
if (nMode != gameStates.render.cockpit.nMode) {		//new nMode
	gameStates.render.cockpit.nMode = nMode;
	InitCockpit ();
	}
}

//------------------------------------------------------------------------------

//force cockpit redraw next time. call this if you've trashed the screen
void ResetCockpit ()
{
gameStates.render.cockpit.bRedraw = 1;
gameStates.render.cockpit.nLastDrawn [0] =
gameStates.render.cockpit.nLastDrawn [1] = -1;
}

//------------------------------------------------------------------------------

//NEWVR
void VRResetParams ()
{
gameStates.render.vr.xEyeWidth = VR_SEPARATION;
gameStates.render.vr.nEyeOffset = VR_PIXEL_SHIFT;
gameStates.render.vr.bEyeOffsetChanged = 2;
}

//------------------------------------------------------------------------------

void GameInitRenderSubBuffers (int x, int y, int w, int h)
{
if (!bScanlineDouble) {
	GrInitSubCanvas (&gameStates.render.vr.buffers.subRender[0], &gameStates.render.vr.buffers.render[0], x, y, w, h);
	GrInitSubCanvas (&gameStates.render.vr.buffers.subRender[1], &gameStates.render.vr.buffers.render[1], x, y, w, h);
	}
}

//------------------------------------------------------------------------------

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void GameInitRenderBuffers (int screen_mode, int render_w, int render_h, int render_method, int flags)
{
//	if (vga_check_mode (screen_mode) != 0)
//		Error ("Cannot set requested video mode");

gameStates.render.vr.nScreenMode	=	screen_mode;
gameStates.render.vr.nScreenFlags	=  flags;
//NEWVR
VRResetParams ();
gameStates.render.vr.nRenderMode 	= render_method;
gameData.render.window.w 		= render_w;
gameData.render.window.h		= render_h;
if (gameStates.render.vr.buffers.offscreen) {
	GrFreeCanvas (gameStates.render.vr.buffers.offscreen);
	}

if ((gameStates.render.vr.nRenderMode==VR_AREA_DET) || (gameStates.render.vr.nRenderMode==VR_INTERLACED))	{
	if (render_h*2 < 200)	{
		gameStates.render.vr.buffers.offscreen = GrCreateCanvas (render_w, 200);
		}
	else {
		gameStates.render.vr.buffers.offscreen = GrCreateCanvas (render_w, render_h*2);
		}
	GrInitSubCanvas (&gameStates.render.vr.buffers.render[0], gameStates.render.vr.buffers.offscreen, 0, 0, render_w, render_h);
	GrInitSubCanvas (&gameStates.render.vr.buffers.render[1], gameStates.render.vr.buffers.offscreen, 0, render_h, render_w, render_h);
	}
else {
	if (render_h < 200) {
		gameStates.render.vr.buffers.offscreen = GrCreateCanvas (render_w, 200);
		}
	else {
		gameStates.render.vr.buffers.offscreen = GrCreateCanvas (render_w, render_h);
      }
	gameStates.render.vr.buffers.offscreen->cvBitmap.bmProps.nType = BM_OGL;
	GrInitSubCanvas (&gameStates.render.vr.buffers.render[0], gameStates.render.vr.buffers.offscreen, 0, 0, render_w, render_h);
	GrInitSubCanvas (&gameStates.render.vr.buffers.render[1], gameStates.render.vr.buffers.offscreen, 0, 0, render_w, render_h);
	}
GameInitRenderSubBuffers (0, 0, render_w, render_h);
}

//------------------------------------------------------------------------------

//called to get the screen in a mode compatible with popup menus.
//if we can't have popups over the game screen, switch to menu mode.

void SetPopupScreenMode (void)
{
	//WIN (LoadCursorWin (MOUSE_DEFAULT_CURSOR);

if (!gameOpts->menus.nStyle) {
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
if ((gameStates.video.nScreenMode == sm) && (nCurrentVGAMode == gameStates.render.vr.nScreenMode) && 
		(/* (sm != SCREEN_GAME) ||*/ (grdCurScreen->scMode == gameStates.render.vr.nScreenMode))) {
	GrSetCurrentCanvas (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
	OglSetScreenMode ();
	return 1;
	}
#ifdef EDITOR
	Canv_editor = NULL;
#endif

	gameStates.video.nScreenMode = sm;

	switch (gameStates.video.nScreenMode) {
		case SCREEN_MENU:
		{
			u_int32_t nMenuMode;

			gameStates.menus.bHires = gameStates.menus.bHiresAvailable;		//do highres if we can

            nMenuMode = 
					gameStates.gfx.bOverride ?
						gameStates.gfx.nStartScrMode
						: gameStates.menus.bHires ?
							 (gameStates.render.vr.nScreenMode >= SM (640,480)) ?
								gameStates.render.vr.nScreenMode
								: SM (640,480)
							: SM (320,200);
			gameStates.video.nLastScreenMode = -1;
			if (nCurrentVGAMode != nMenuMode) {
				if (GrSetMode (nMenuMode))
					Error ("Cannot set screen mode for menu");
				if (!gameStates.render.bPaletteFadedOut)
					GrPaletteStepLoad (NULL);
				gameStates.menus.bInitBG = 1;
				RebuildRenderContext (gameStates.app.bGameRunning);
			}

			GrInitSubCanvas (
				gameStates.render.vr.buffers.screenPages, &grdCurScreen->scCanvas, 0, 0, 
				grdCurScreen->scWidth, grdCurScreen->scHeight);
			GrInitSubCanvas (
				gameStates.render.vr.buffers.screenPages + 1, &grdCurScreen->scCanvas, 0, 0, 
				grdCurScreen->scWidth, grdCurScreen->scHeight);

			gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && gameStates.menus.bHires;

		}
		break;

	case SCREEN_GAME:
		if (nCurrentVGAMode != gameStates.render.vr.nScreenMode) {
			if (GrSetMode (gameStates.render.vr.nScreenMode))	{
				Error ("Cannot set desired screen mode for game!");
				//we probably should do something else here, like select a standard mode
			}
			#ifdef MACINTOSH
			if ((gameConfig.nControlType == 1) && (gameStates.app.nFunctionMode == FMODE_GAME))
				JoyDefsCalibrate ();
			#endif
			ResetCockpit ();
		}
			{
			gameData.render.window.wMax = grdCurScreen->scWidth;
			gameData.render.window.hMax = grdCurScreen->scHeight;
	      if (!gameData.render.window.h || (gameData.render.window.h > gameData.render.window.hMax) || 
				 !gameData.render.window.w || (gameData.render.window.w > gameData.render.window.wMax)) {
				gameData.render.window.w = gameData.render.window.wMax;
				gameData.render.window.h = gameData.render.window.hMax;
				}
			}
		InitCockpit ();

	//	Define screen pages for game mode
	// If we designate through screenFlags to use paging, then do so.
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[0], dd_grd_screencanv, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight),
			GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[0], &grdCurScreen->scCanvas, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight)
		);

		if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING) {
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_backcanv, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight),
			GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[1], &grdCurScreen->scCanvas, 0, grdCurScreen->scHeight, grdCurScreen->scWidth, grdCurScreen->scHeight)
		);
		}
		else {
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_screencanv, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight),
			GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[1], &grdCurScreen->scCanvas, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight)
		);
		}
		InitCockpit ();
		gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.menus.bHires = (gameStates.video.nDisplayMode > 1));
		if (gameStates.render.vr.nRenderMode != VR_NONE)	{
			// for 640x480 or higher, use hires font.
			if (gameStates.render.fonts.bHiresAvailable && (grdCurScreen->scHeight > 400))
				gameStates.render.fonts.bHires = 1;
			else
				gameStates.render.fonts.bHires = 0;
		}
		con_resize ();
		break;

	#ifdef EDITOR
	case SCREEN_EDITOR:
		if (grdCurScreen->scMode != SM (800,600))	{
			int gr_error;
			if ((gr_error=GrSetMode (SM (800,600)))!=0) { //force into game scrren
				Warning ("Cannot init editor screen (error=%d)",gr_error);
				return 0;
			}
		}
		GrPaletteStepLoad (NULL);

		GrInitSubCanvas (&gameStates.render.vr.buffers.editorCanvas, &grdCurScreen->scCanvas, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
		Canv_editor = &gameStates.render.vr.buffers.editorCanvas;
		GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[0], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[1], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		GrSetCurrentCanvas (Canv_editor);
		init_editor_screen ();   //setup other editor stuff
		break;
	#endif
	default:
		Error ("Invalid screen mode %d",sm);
	}

	gameStates.render.vr.nCurrentPage = 0;

	WINDOS (
		DDGrSetCurrentCanvas (&dd_VR_screen_pages[gameStates.render.vr.nCurrentPage]),
		GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages[gameStates.render.vr.nCurrentPage])
	);

	if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING)	{
	WINDOS (
		dd_gr_flip (),
		GrShowCanvas (&gameStates.render.vr.buffers.screenPages[gameStates.render.vr.nCurrentPage])
	);
	}
	OglSetScreenMode ();
	return 1;
}

//------------------------------------------------------------------------------

int GrToggleFullScreenGame (void)
{
int i=GrToggleFullScreen ();
FlushInput ();
if (gameStates.app.bGameRunning) {
	HUDMessage (MSGC_GAME_FEEDBACK, "toggling fullscreen mode %s",i?"on":"off");
	StopPlayerMovement ();
	}
return i;
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
	RebuildRenderContext (gameStates.app.bGameRunning);
	return i;
#else
	return -1;
#endif
}

//------------------------------------------------------------------------------

void StopTime ()
{
if (pfnTIRStop)
	pfnTIRStop ();
if (++gameData.time.nPaused == 1) {
	fix xTime = TimerGetFixedSeconds ();
	gameData.time.xSlack = xTime - gameData.time.xLast;
	if (gameData.time.xSlack < 0) {
#if defined (TIMER_TEST) && defined (_DEBUG)
		Int3 ();		//get Matt!!!!
#endif
		gameData.time.xLast = 0;
		}
#if defined (TIMER_TEST) && defined (_DEBUG)
	gameData.time.xStopped = xTime;
	#endif
}
#if defined (TIMER_TEST) && defined (_DEBUG)
gameData.time.xStops++;
#endif
}

//------------------------------------------------------------------------------

void StartTime ()
{
if (gameData.time.nPaused <= 0)
	return;
if (!--gameData.time.nPaused) {
	fix xTime = TimerGetFixedSeconds ();
#if defined (TIMER_TEST) && defined (_DEBUG)
	if (gameData.time.xLast < 0)
		Int3 ();		//get Matt!!!!
#endif
	gameData.time.xLast = xTime - gameData.time.xSlack;
#if defined (TIMER_TEST) && defined (_DEBUG)
	gameData.time.xStarted = time;
#endif
	}
#if defined (TIMER_TEST) && defined (_DEBUG)
gameData.time.xStarts++;
#endif
if (pfnTIRStart)
	pfnTIRStart ();
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
	JoyFlush ();
	MouseFlush ();
#endif
	#ifdef MACINTOSH
	if ((gameStates.app.nFunctionMode != FMODE_MENU) && !joydefs_calibrating)		// only reset mouse when not in menu or not calibrating
	#endif
		MouseGetDelta (&dx, &dy);	// Read mouse
	memset (&Controls,0,sizeof (tControlInfo));
}

//------------------------------------------------------------------------------

void ResetTime ()
{
gameData.time.xLast = TimerGetFixedSeconds ();
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
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
			minFrameTime = (MAXFPS ? f1_0 / MAXFPS : 1);

if (gameData.app.bGamePaused) {
	gameData.time.xLast = TimerGetFixedSeconds ();
	gameData.time.xFrame = 0;
	gameData.time.xRealFrame = 0;
	return;
	}
#if defined (TIMER_TEST) && defined (_DEBUG)
_last_frametime = last_frametime;
#endif
do {
	timerValue = TimerGetFixedSeconds ();
   gameData.time.xFrame = timerValue - gameData.time.xLast;
	timer_delay (1);
	if (MAXFPS < 2)
		break;
	} while (gameData.time.xFrame < minFrameTime);
#if defined (TIMER_TEST) && defined (_DEBUG)
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
#if defined (TIMER_TEST) && defined (_DEBUG)
actual_lastTimerValue = gameData.time.xLast;
#endif
if (gameStates.app.cheats.bTurboMode)
	gameData.time.xFrame *= 2;
// Limit frametime to be between 5 and 150 fps.
gameData.time.xRealFrame = gameData.time.xFrame;
gameData.time.xLast = timerValue;
if (gameData.time.xFrame < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.time.xFrame = last_frametime;		//d:\temp\dm_test.then use time from last frame
#ifdef _DEBUG
if (xFixedFrameTime) 
	gameData.time.xFrame = xFixedFrameTime;
#endif
#ifdef _DEBUG
// Pause here!!!
if (Debug_pause) {
	int c = 0;
	while (!c)
		c = KeyPeekKey ();
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
#if defined (TIMER_TEST) && defined (_DEBUG)
gameData.time.xStops = gameData.time.xStarts = 0;
#endif
//	Set value to determine whether homing missile can see target.
//	The lower frametime is, the more likely that it can see its target.
if (gameStates.limitFPS.bHomers)
	xMinTrackableDot = MIN_TRACKABLE_DOT;
else if (gameData.time.xFrame <= F1_0/64)
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
	static int nId = 0;

GrSetCurFont (GAME_FONT);    //GAME_FONT
GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
timevar=i2f (netGame.xPlayTimeAllowed*5*60);
i = f2i (timevar-ThisLevelTime) + 1;
sprintf (temp_string, TXT_TIME_LEFT, i);
if (i >= 0)
	nId = GrString (0, 32, temp_string, &nId);
}
#endif

//------------------------------------------------------------------------------

void do_photos ();
void level_with_floor ();

void modex_clear_box (int x,int y,int w,int h)
{
	gsrCanvas *temp_canv,*save_canv;

	save_canv = grdCurCanv;
	temp_canv = GrCreateCanvas (w,h);
	GrSetCurrentCanvas (temp_canv);
	GrClearCanvas (BLACK_RGBA);
	GrSetCurrentCanvas (save_canv);
	GrBitmapM (x,y,&temp_canv->cvBitmap, 0);
	GrFreeCanvas (temp_canv);

}

//------------------------------------------------------------------------------

extern void modex_printf (int x,int y,char *s,grsFont *font,int color);

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
	static int multiCount = 0;
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

	sprintf (filename, "screen%d", multiCount++);
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

void test_animStates ();

#include "fvi.h"

//put up the help message
void DoShowHelp ()
{
ShowHelp ();
}


extern int been_in_editor;

//	------------------------------------------------------------------------------------

void DoCloakStuff (void)
{
	int i;
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		if (gameData.multiplayer.players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (gameData.time.xGame - gameData.multiplayer.players[i].cloakTime > CLOAK_TIME_MAX) {
				gameData.multiplayer.players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == gameData.multiplayer.nLocalPlayer) {
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
if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (LOCALPLAYER.invulnerableTime != 0x7fffffff)) {
	if (gameData.time.xGame - LOCALPLAYER.invulnerableTime > INVULNERABLE_TIME_MAX) {
		LOCALPLAYER.flags ^= PLAYER_FLAGS_INVULNERABLE;
		if (!bFakingInvul) {
			DigiPlaySample (SOUND_INVULNERABILITY_OFF, F1_0);
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI) {
				MultiSendPlaySound (SOUND_INVULNERABILITY_OFF, F1_0);
				MaybeDropNetPowerup (-1, POW_INVUL, FORCE_DROP);
				}
#endif
#if TRACE
				//con_printf (CONDBG, " --- You have been DE-INVULNERABLEIZED! ---\n");
#endif
			}
		bFakingInvul=0;
		}
	}
}

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
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	gameData.physics.xAfterburnerCharge=0;
if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead) {
	if (DigiKillSoundLinkedToObject (LOCALPLAYER.nObject))
#ifdef NETWORK
		MultiSendSoundFunction (0,0)
#endif
	 	;
	}
else if ((gameStates.gameplay.xLastAfterburnerCharge && (Controls [0].afterburnerState != gameStates.gameplay.bLastAfterburnerState)) || 
	 		(gameStates.gameplay.bLastAfterburnerState && (gameStates.gameplay.xLastAfterburnerCharge && !gameData.physics.xAfterburnerCharge))) {
	if (gameData.physics.xAfterburnerCharge && Controls [0].afterburnerState && 
		 (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)) {
		DigiLinkSoundToObject3 ((short) SOUND_AFTERBURNER_IGNITE, (short) LOCALPLAYER.nObject, 
										1, F1_0, i2f (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END, NULL, 0, SOUNDCLASS_PLAYER);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_IGNITE);
#endif
		}
	else {
		DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
		DigiLinkSoundToObject2 ((short) SOUND_AFTERBURNER_PLAY, (short) LOCALPLAYER.nObject, 
										0, F1_0, i2f (256), SOUNDCLASS_PLAYER);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
		 	MultiSendSoundFunction (0,0);
#endif
		}
	}
gameStates.gameplay.bLastAfterburnerState = Controls [0].afterburnerState;
gameStates.gameplay.xLastAfterburnerCharge = gameData.physics.xAfterburnerCharge;
}

// -- //	------------------------------------------------------------------------------------
// -- //	if energy < F1_0/2, recharge up to F1_0/2
// -- void recharge_energy_frame (void)
// -- {
// -- 	if (LOCALPLAYER.energy < gameData.weapons.info[0].energy_usage) {
// -- 		LOCALPLAYER.energy += gameData.time.xFrame/4;
// --
// -- 		if (LOCALPLAYER.energy > gameData.weapons.info[0].energy_usage)
// -- 			LOCALPLAYER.energy = gameData.weapons.info[0].energy_usage;
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

if ((gameData.demo.nState==ND_STATE_RECORDING) && 
		(gameStates.ogl.palAdd.red || gameStates.ogl.palAdd.green || gameStates.ogl.palAdd.blue))
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
if (gameStates.render.vr.bUseRegCode)
	;//GrPaletteStepUpVR (r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX);
else
	GrPaletteStepUp (r, g, b);
}

//------------------------------------------------------------------------------

void PaletteRestore (void)
{
gameStates.ogl.palAdd = palAddSave; 
GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);
//	Forces flash effect to fixup palette next frame.
gameData.render.xTimeFlashLastPlayed = 0;
}

void DeadPlayerFrame (void);

//	-----------------------------------------------------------------------------

int AllowedToFireLaser (void)
{
	float	s;

if (gameStates.app.bPlayerIsDead) {
	gameData.missiles.nGlobalFiringCount = 0;
	return 0;
	}
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 0;
//	Make sure enough time has elapsed to fire laser, but if it looks like it will
//	be a long while before laser can be fired, then there must be some mistake!
if (!IsMultiGame && ((s = gameStates.gameplay.slowmo [0].fSpeed) > 1)) {
	fix t = gameData.laser.xLastFiredTime + (fix) ((gameData.laser.xNextFireTime - gameData.laser.xLastFiredTime) * s);
	if ((t > gameData.time.xGame) && (t < gameData.time.xGame + 2 * F1_0 * s))
		return 0;
	}
else {
	if ((gameData.laser.xNextFireTime > gameData.time.xGame) &&  (gameData.laser.xNextFireTime < gameData.time.xGame + 2 * F1_0))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

fix	xNextFlareFireTime = 0;
#define	FLARE_BIG_DELAY	 (F1_0*2)

int AllowedToFireFlare (void)
{
if ((xNextFlareFireTime > gameData.time.xGame) &&
	 (xNextFlareFireTime < gameData.time.xGame + FLARE_BIG_DELAY))	//	In case time is bogus, never wait > 1 second.
		return 0;
if (LOCALPLAYER.energy >= WI_energy_usage (FLARE_ID))
	xNextFlareFireTime = gameData.time.xGame + (fix) (gameStates.gameplay.slowmo [0].fSpeed * F1_0 / 4);
else
	xNextFlareFireTime = gameData.time.xGame + (fix) (gameStates.gameplay.slowmo [0].fSpeed * FLARE_BIG_DELAY);
return 1;
}

//------------------------------------------------------------------------------

int AllowedToFireMissile (void)
{
	float	s;

//	Make sure enough time has elapsed to fire missile, but if it looks like it will
//	be a long while before missile can be fired, then there must be some mistake!
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 0;
if (!IsMultiGame && ((s = gameStates.gameplay.slowmo [0].fSpeed) > 1)) {
	fix t = gameData.missiles.xLastFiredTime + (fix) ((gameData.missiles.xNextFireTime - gameData.missiles.xLastFiredTime) * s);
	if ((t > gameData.time.xGame) && (t < gameData.time.xGame + 5 * F1_0 * s))
		return 0;
	}
else {
	if ((gameData.missiles.xNextFireTime > gameData.time.xGame) && 
		 (gameData.missiles.xNextFireTime < gameData.time.xGame + 5 * F1_0))
		return 0;
	}
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
	ADD_HELP_OPT (TXT_HELP_PAUSE_D2X);
#else
	ADD_HELP_OPT (TXT_HLP_PAUSE);
#endif
	ADD_HELP_OPT (TXT_HELP_MINUSPLUS);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_PRTSCN_D2X);
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
	ADD_HELP_OPT (TXT_HLP_CHASECAM);
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
	if (Controls [0].rearViewDownCount) {		//key/button has gone down
#else
	if (Controls [0].rearViewDownCount && !gameStates.render.bExternalView) {		//key/button has gone down
#endif
		Controls [0].rearViewDownCount = 0;
		if (gameStates.render.bRearView) {
			gameStates.render.bRearView = 0;
			if (gameStates.render.cockpit.nMode == CM_REAR_VIEW) {
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
		if (Controls [0].rearViewDownState) {

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
gameStates.render.cockpit.nLastDrawn[0] = -1;				// Force cockpit to redraw next time a frame renders.
gameStates.render.cockpit.nLastDrawn[1] = -1;				// Force cockpit to redraw next time a frame renders.
gameStates.app.bEndLevelSequence = 0;
GrPaletteStepLoad (NULL);
SetScreenMode (SCREEN_GAME);
ResetPaletteAdd ();
SetWarnFunc (ShowInGameWarning);
#if TRACE
//con_printf (CONDBG, "   InitCockpit d:\temp\dm_test.\n");
#endif
InitCockpit ();
#if TRACE
//con_printf (CONDBG, "   InitGauges d:\temp\dm_test.\n");
#endif
InitGauges ();
//DigiInitSounds ();
//keyd_repeat = 0;                // Don't allow repeat in game
keyd_repeat = 1;                // Do allow repeat in game
#ifdef EDITOR
	if (gameData.segs.segments[gameData.objs.console->nSegment].nSegment == -1)      //tSegment no longer exists
		RelinkObject (OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM (Cursegp));

	if (!check_obj_seg (gameData.objs.console))
		MovePlayerToSegment (Cursegp,Curside);
#endif
gameData.objs.viewer = gameData.objs.console;
#if TRACE
//con_printf (CONDBG, "   FlyInit d:\temp\dm_test.\n");
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
//con_printf (CONDBG, "   FixObjectSegs d:\temp\dm_test.\n");
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
	int c;

GameSetup ();								// Replaces what was here earlier.
#ifdef MWPROFILE
ProfilerSetStatus (1);
#endif

if (pfnTIRStart)
	pfnTIRStart ();
if (!setjmp (gameExitPoint)) {
for (;;) {
	int player_shields;
		// GAME LOOP!
#ifdef _DEBUG
	if (gameStates.render.automap.bDisplay)
#endif
	gameStates.render.automap.bDisplay = 0;
	gameStates.app.bConfigMenu = 0;
	if (gameData.objs.console != &gameData.objs.objects[LOCALPLAYER.nObject]) {
#if TRACE
	    //con_printf (CONDBG,"gameData.multiplayer.nLocalPlayer=%d nObject=%d",gameData.multiplayer.nLocalPlayer,LOCALPLAYER.nObject);
#endif
	    //Assert (gameData.objs.console == &gameData.objs.objects[LOCALPLAYER.nObject]);
		}
	player_shields = LOCALPLAYER.shields;
	gameStates.app.nExtGameStatus = GAMESTAT_RUNNING;
	if (!GameLoop (1, 1))		// Do game loop with rendering and reading controls.
		continue;
	if (gameStates.app.bSingleStep) {
		while (!(c = KeyInKey ()))
			;
		if (c == KEY_ALTED + KEY_CTRLED + KEY_ESC)
			gameStates.app.bSingleStep = 0;
		}
	//if the tPlayer is taking damage, give up guided missile control
	if (LOCALPLAYER.shields != player_shields)
		ReleaseGuidedMissile (gameData.multiplayer.nLocalPlayer);
	//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();	// Handle RedBook Audio Repeating.
	if (gameStates.app.bConfigMenu) {
		int double_save = bScanlineDouble;
	//WIN (mouse_set_mode (0);
		if (!IsMultiGame) {
			PaletteSave (); 
			ResetPaletteAdd ();
			ApplyModifiedPalette (); 
			GrPaletteStepLoad (NULL); 
			}
		ConfigMenu ();
		if (bScanlineDouble != double_save)
			InitCockpit ();
		if (!IsMultiGame) 
			PaletteRestore ();
		//WIN (mouse_set_mode (1);
		}
	if (gameStates.render.automap.bDisplay) {
		int	save_w = gameData.render.window.w,
				save_h = gameData.render.window.h;
		DoAutomap (0, 0);
		gameStates.app.bEnterGame = 1;
		//	FlushInput ();
		//	StopPlayerMovement ();
		gameStates.video.nScreenMode = -1; 
		SetScreenMode (SCREEN_GAME);
		gameData.render.window.w = save_w; 
		gameData.render.window.h = save_h;
		InitCockpit ();
		gameStates.render.cockpit.nLastDrawn [0] =
		gameStates.render.cockpit.nLastDrawn [1] = -1;
		}
	if ((gameStates.app.nFunctionMode != FMODE_GAME) && 
		 gameData.demo.bAuto && !gameOpts->demo.bRevertFormat && 
		 (gameData.demo.nState != ND_STATE_NORMAL)) {
		int choice, fmode;
		fmode = gameStates.app.nFunctionMode;
		SetFunctionMode (FMODE_GAME);
		PaletteSave ();
		ApplyModifiedPalette ();
		ResetPaletteAdd ();
		GrPaletteStepLoad (NULL);
		choice = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
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
		 (gameStates.app.nFunctionMode != FMODE_EDITOR) &&
		 !gameStates.multi.bIWasKicked) {
		if (QuitSaveLoadMenu ())
			SetFunctionMode (FMODE_GAME);
		}
	gameStates.multi.bIWasKicked = 0;
	if (gameStates.app.nFunctionMode != FMODE_GAME)
		longjmp (gameExitPoint, 0);
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
if (gameStates.render.cockpit.nModeSave != -1) {
	gameStates.render.cockpit.nMode = gameStates.render.cockpit.nModeSave;
	gameStates.render.cockpit.nModeSave = -1;
	}
if (gameStates.app.nFunctionMode != FMODE_EDITOR)
	GrPaletteFadeOut (NULL,32,0);			// Fade out before going to menu
//@@	if ((!demo_playing) && (!multi_game) && (gameStates.app.nFunctionMode != FMODE_EDITOR))	{
//@@		MaybeAddPlayerScore (gameStates.app.bGameAborted);
//@@	}
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
GameDisableCheats ();
UnloadCamBot ();
#ifdef APPLE_DEMO
SetFunctionMode (FMODE_EXIT);		// get out of game in Apple OEM version
#endif
if (pfnTIRStop)
	pfnTIRStop ();
}

//-----------------------------------------------------------------------------
//called at the end of the program

void _CDECL_ sdl_close(void);

void _CDECL_ CloseGame (void)
{
	static	int bGameClosed = 0;
#if MULTI_THREADED
	int		i;
#endif
if (bGameClosed)
	return;
bGameClosed = 1;
if (gameStates.app.bMultiThreaded) {
	EndRenderThreads ();
	}
GrClose ();
DigiClose ();
FreeUserSongs ();
RLECacheClose ();
BMFreeExtraObjBitmaps ();
BMFreeExtraModels ();
LogErr ("unloading render buffers\n");
FreeRenderItems ();
LogErr ("unloading string pool\n");
FreeStringPool ();
LogErr ("unloading level messages\n");
FreeTextData (&gameData.messages);
FreeTextData (&gameData.sounds);
LogErr ("unloading hires animations\n");
PiggyFreeHiresAnimations ();
PiggyBitmapPageOutAll (0);
LogErr ("freeing sound buffers\n");
DigiFreeSoundBufs ();
FreeSoundReplacements ();
LogErr ("unloading hoard data\n");
FreeHoardData ();
LogErr ("unloading auxiliary poly model data\n");
G3FreeAllPolyModelItems ();
LogErr ("unloading hires models\n");
FreeHiresModels (0);
LogErr ("unloading tracker list\n");
DestroyTrackerList ();
LogErr ("unloading lightmap data\n");
DestroyLightMaps ();
LogErr ("unloading particle data\n");
DestroyAllSmoke ();
LogErr ("unloading shield sphere data\n");
DestroySphere (&gameData.render.shield);
DestroySphere (&gameData.render.monsterball);
LogErr ("unloading HUD icons\n");
FreeInventoryIcons ();
FreeObjTallyIcons ();
LogErr ("unloading extra texture data\n");
FreeExtraImages ();
LogErr ("unloading shield data\n");
FreeSphereCoord ();
LogErr ("unloading palettes\n");
FreePalettes ();
FreeSkyBoxSegList ();
CloseDynLighting ();
if (gameStates.render.vr.buffers.offscreen)	{
	GrFreeCanvas (gameStates.render.vr.buffers.offscreen);
	gameStates.render.vr.buffers.offscreen = NULL;
}
LogErr ("unloading gauge data\n");
CloseGaugeCanvases ();
LogErr ("restoring effect bitmaps\n");
RestoreEffectBitmapIcons ();
if (bGameCockpitCopyCode) {
	D2_FREE (bGameCockpitCopyCode);
	bGameCockpitCopyCode = NULL;
}
if (bmBackground.bmTexBuf) {
	LogErr ("unloading background bitmap\n");
	D2_FREE (bmBackground.bmTexBuf);
	}
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
LogErr ("unloading custom background data\n");
NMFreeAltBg (1);
SaveBanList ();
FreeBanList ();
sdl_close ();
#if 0
if (fErr) {
	fclose (fErr);
	fErr = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------

gsrCanvas *GetCurrentGameScreen ()
{
return gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage;
}

//-----------------------------------------------------------------------------
extern void kconfig_center_headset ();

#ifdef _DEBUG
void SpeedtestFrame (void);
Uint32 nDebugSlowdown = 0;
#endif

#ifdef EDITOR
extern void player_follow_path (tObject *objP);
extern void check_create_player_path (void);

#endif

//returns ptr to escort robot, or NULL
tObject *find_escort ()
{
	int 		i;
	tObject	*objP = gameData.objs.objects;

for (i = 0; i <= gameData.objs.nLastObject; i++, objP++)
	if ((objP->nType == OBJ_ROBOT) && ROBOTINFO (objP->id).companion)
		return objP;
return NULL;
}

//-----------------------------------------------------------------------------

extern void ProcessSmartMinesFrame (void);
extern void DoSeismicStuff (void);

#ifdef _DEBUG
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
	//con_printf (CONDBG,"Flushing movie bufferd:\temp\dm_test.");
#endif
	bmMovie.bmTexBuf = Movie_frame_buffer;

	for (f=0;f<nMovieFrames;f++) {
		sprintf (savename, "%sfrm%04d.pcx",movie_path,__Movie_frame_num);
		__Movie_frame_num++;
		pcx_write_bitmap (savename, &bmMovie);
		bmMovie.bmTexBuf += MOVIE_FRAME_SIZE;

		if (f % 5 == 0) {
#if TRACE
			//con_printf (CONDBG,"%3d/%3d\10\10\10\10\10\10\10",f,nMovieFrames);
#endif
		}		
	}

	nMovieFrames=0;
#if TRACE
	//con_printf (CONDBG,"done   \n");
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
			Movie_frame_buffer = D2_ALLOC (MAX_MOVIE_BUFFER_FRAMES * MOVIE_FRAME_SIZE);
			if (!Movie_frame_buffer) {
				Int3 ();
				Saving_movie_frames=0;
			}

			nMovieFrames=0;

			bmMovie.bmProps.x = bmMovie.bmProps.y = 0;
			bmMovie.bmProps.w = 320;
			bmMovie.bmProps.h = 200;
			bmMovie.bmProps.nType = BM_LINEAR;
			bmMovie.bmProps.flags = 0;
			bmMovie.bmProps.rowSize = 320;
			//bmMovie.bmHandle = 0;

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
	memcpy (Movie_frame_buffer+nMovieFrames*MOVIE_FRAME_SIZE,grdCurScreen->scCanvas.cvBitmap.bmTexBuf,MOVIE_FRAME_SIZE);

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

void GetSlowTicks (void)
{
gameStates.app.nSDLTicks = SDL_GetTicks ();
gameStates.app.tick40fps.nTime = gameStates.app.nSDLTicks - gameStates.app.tick40fps.nLastTick;
if ((gameStates.app.tick40fps.bTick = (gameStates.app.tick40fps.nTime >= 25)))
	gameStates.app.tick40fps.nLastTick = gameStates.app.nSDLTicks;
gameStates.app.tick60fps.nTime = gameStates.app.nSDLTicks - gameStates.app.tick60fps.nLastTick;
if ((gameStates.app.tick60fps.bTick = (gameStates.app.tick60fps.nTime >= (50 + ++gameStates.app.tick60fps.nSlack) / 3))) {
	gameStates.app.tick60fps.nLastTick = gameStates.app.nSDLTicks;
	gameStates.app.tick60fps.nSlack %= 3;
	}
}

//-----------------------------------------------------------------------------

extern int flFrameTime;

int FusionBump (void)
{
if (gameData.fusion.xAutoFireTime) {
	if (gameData.weapons.nPrimary != FUSION_INDEX)
		gameData.fusion.xAutoFireTime = 0;
	else if (gameData.time.xGame + flFrameTime/2 >= gameData.fusion.xAutoFireTime) {
		gameData.fusion.xAutoFireTime = 0;
		gameData.laser.nGlobalFiringCount = 1;
		}
	else {
		fix			xBump;
		vmsVector	vRand;
	
		static time_t t0 = 0;
		time_t t = gameStates.app.nSDLTicks;
		if (t - t0 < 30)
			return 0;
		t0 = t;
		gameData.laser.nGlobalFiringCount = 0;
		gameData.objs.console->mType.physInfo.rotVel.p.x += (d_rand () - 16384)/8;
		gameData.objs.console->mType.physInfo.rotVel.p.z += (d_rand () - 16384)/8;
		MakeRandomVector (&vRand);
		xBump = F1_0*4;
		if (gameData.fusion.xCharge > F1_0*2)
			xBump = gameData.fusion.xCharge*4;
		BumpOneObject (gameData.objs.console, &vRand, xBump);
		}
	}
return 1;
}

//-----------------------------------------------------------------------------

int screenShotIntervals [] = {0, 1, 3, 5, 10, 15, 30, 60};

void AutoScreenshot (void)
{
	int	h;

	static	time_t	t0 = 0;

if (gameData.app.bGamePaused || gameStates.menus.nInMenu)
	return;
if (!(h = screenShotIntervals [gameOpts->app.nScreenShotInterval]))
	return;
if (gameStates.app.nSDLTicks - t0 < h * 1000)
	return;
t0 = gameStates.app.nSDLTicks;
bSaveScreenShot = 1;
SaveScreenShot (0, 0);
}

//-----------------------------------------------------------------------------

int PlayerHasHeadLight (int nPlayer)
{
return EGI_FLAG (headlight.bAvailable, 0, 0, 0) &&
		 (EGI_FLAG (headlight.bBuiltIn, 0, 1, 0) || 
		  ((gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT) != 0));
}

//-----------------------------------------------------------------------------

int HeadLightIsOn (int nPlayer)
{
#ifdef _DEBUG
if (!PlayerHasHeadLight (nPlayer))
	return 0;
if (!(gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON))
	return 0;
return 1;
#else
return PlayerHasHeadLight (nPlayer) && ((gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) != 0);
#endif
}

//-----------------------------------------------------------------------------

void SetPlayerHeadLight (int nPlayer, int bOn)
{
if (bOn)
	gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags |= PLAYER_FLAGS_HEADLIGHT_ON;
else
	gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
}

//-----------------------------------------------------------------------------

void DrainHeadLightPower (void)
{
	static int bTurnedOff = 0;

if (!EGI_FLAG (headlight.bDrainPower, 0, 0, 1))
	return;
if (!HeadLightIsOn (-1))
	return;

LOCALPLAYER.energy -= (gameData.time.xFrame * 3 / 8);
if (LOCALPLAYER.energy < i2f (10)) {
	if (!bTurnedOff) {
		LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
		bTurnedOff = 1;
		if (IsMultiGame)
			MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);
		}
	}
else
	bTurnedOff = 0;
if (LOCALPLAYER.energy <= 0) {
	LOCALPLAYER.energy = 0;
	LOCALPLAYER.flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
	if (IsMultiGame)
		MultiSendFlags ((char) gameData.multiplayer.nLocalPlayer);
	}
}

//-----------------------------------------------------------------------------
// -- extern void lightning_frame (void);

void GameRenderFrame ();
void OmegaChargeFrame (void);
void FlickerLights ();

extern time_t t_currentTime, t_savedTime;

int GameLoop (int RenderFlag, int bReadControls)
{
gameStates.app.bGameRunning = 1;
gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
GetSlowTicks ();
#ifdef _DEBUG
//	Used to slow down frame rate for testing things.
//	RenderFlag = 1; // DEBUG
//	GrPaletteStepLoad (gamePalette);
if (nDebugSlowdown) {
	time_t	t = SDL_GetTicks ();
	while (SDL_GetTicks () - t < nDebugSlowdown)
		;
	}
#endif

#ifdef _DEBUG
	if (FindArg ("-invulnerability")) {
		LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
		SetSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
		}
#endif
	if (!MultiProtectGame ()) {
		SetFunctionMode (FMODE_MENU);
		return 1;
		}
	if (gameStates.gameplay.bMineMineCheat) {
		WowieCheat (0);
		GasolineCheat (0);
		}
	AutoBalanceTeams ();
	MultiSendTyping ();
	MultiSendWeapons (0);
	MultiSyncKills ();
	MultiRefillPowerups ();
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
DrainHeadLightPower ();
#ifdef EDITOR
	check_create_player_path ();
	player_follow_path (gameData.objs.console);
#endif

#ifdef NETWORK
	if (IsMultiGame) {
		AddServerToTracker ();
      MultiDoFrame ();
		CheckMonsterballScore ();
		if (netGame.xPlayTimeAllowed && ThisLevelTime>=i2f ((netGame.xPlayTimeAllowed*5*60)))
          MultiCheckForKillGoalWinner ();
		else 
			MultiCheckForEntropyWinner ();
     }
#endif

	if (RenderFlag) {
		if (gameStates.render.cockpit.bRedraw) {			//screen need redrawing?
			InitCockpit ();
			gameStates.render.cockpit.bRedraw = 0;
		}
//LogErr ("GameRenderFrame\n");
		GameRenderFrame ();
		gameStates.app.bUsingConverter = 0;
		//show_extraViews ();		//missile view, buddy bot, etc.

#ifdef _DEBUG
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
//LogErr ("DoFlashFrame \n");
	DoLightningFrame ();
//LogErr ("DoAmbientSounds\n");
	DoAmbientSounds ();
#ifdef _DEBUG
	if (gameData.speedtest.bOn)
		SpeedtestFrame ();
#endif
	if (bReadControls) {
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
#ifdef _DEBUG
	if (FindArg ("-checktime") != 0)
		if (gameData.time.xGame >= i2f (600))		//wrap after 10 minutes
			gameData.time.xGame = gameData.time.xFrame;
#endif
#ifdef NETWORK
   if ((gameData.app.nGameMode & GM_MULTI) && netGame.xPlayTimeAllowed)
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
	if (gameData.reactor.bDestroyed && (gameData.demo.nState == ND_STATE_RECORDING)) {
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
		LOCALPLAYER.homingObjectDist = -1;		//	Assume not being tracked.  LaserDoWeaponSequence modifies this.
//LogErr ("UpdateAllObjects\n");
		if (!UpdateAllObjects ())
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
		if (!FusionBump ())
			return 1;
		if (gameData.laser.nGlobalFiringCount) {
			//	Don't cap here, gets capped in CreateNewLaser and is based on whether in multiplayer mode, MK, 3/27/95
			// if (gameData.fusion.xCharge > F1_0*2)
			// 	gameData.fusion.xCharge = F1_0*2;
			gameData.laser.nGlobalFiringCount -= LocalPlayerFireLaser ();	//LaserFireObject (LOCALPLAYER.nObject, gameData.weapons.nPrimary);
			}
		if (gameData.laser.nGlobalFiringCount < 0)
			gameData.laser.nGlobalFiringCount = 0;
		}
	if (gameStates.render.bDoAppearanceEffect) {
		CreatePlayerAppearanceEffect (gameData.objs.console);
		gameStates.render.bDoAppearanceEffect = 0;
#ifdef NETWORK
	if (IsMultiGame && netGame.invul) {
		LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
		LOCALPLAYER.invulnerableTime = gameData.time.xGame-i2f (27);
		bFakingInvul = 1;
		SetSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
		}
#endif
	}
DoSlowMotionFrame ();
CheckInventory ();
//LogErr ("OmegaChargeFrame \n");
OmegaChargeFrame ();
//LogErr ("SlideTextures \n");
SlideTextures ();
//LogErr ("FlickerLights \n");
FlickerLights ();
//LogErr ("\n");
AutoScreenshot ();
return 1;
}


//	-----------------------------------------------------------------------------

void ComputeSlideSegs (void)
{
	int	nSegment, nSide, bIsSlideSeg, nTexture;

gameData.segs.nSlideSegs = 0;
for (nSegment = 0; nSegment <= gameData.segs.nLastSegment; nSegment++) {
	bIsSlideSeg = 0;
	for (nSide = 0; nSide < 6; nSide++) {
		nTexture = gameData.segs.segments [nSegment].sides [nSide].nBaseTex;
		if (gameData.pig.tex.pTMapInfo [nTexture].slide_u  || 
			 gameData.pig.tex.pTMapInfo [nTexture].slide_v) {
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
	tUVL	*uvlP;
	fix	slideU, slideV, xDelta;

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
#ifdef _DEBUG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nSegment = nSegment;
#endif
		i = (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_SKYBOX) ? 3 : 8;
		slideU = FixMul (gameData.time.xFrame, slideU << i);
		slideV = FixMul (gameData.time.xFrame, slideV << i);
		for (i = 0, uvlP = sideP->uvls; i < 4; i++) {
			uvlP [i].u += slideU;
			if (uvlP [i].u > f2_0) {
				xDelta = (uvlP [i].u / F1_0 - 1) * F1_0;
				for (j = 0; j < 4; j++)
					uvlP [j].u -= xDelta;
				}
			else if (uvlP [i].u < -f2_0) {
				xDelta = (-uvlP [i].u / F1_0 - 1) * F1_0;
				for (j = 0; j < 4; j++)
					uvlP [j].u += xDelta;
				}
			uvlP [i].v += slideV;
			if (uvlP [i].v > f2_0) {
				xDelta = (uvlP [i].v / F1_0 - 1) * F1_0;
				for (j = 0; j < 4; j++)
					uvlP [j].v -= xDelta;
				}
			else if (uvlP [i].v < -f2_0) {
				xDelta = (-uvlP [i].v / F1_0 - 1) * F1_0;
				for (j = 0; j < 4; j++)
					uvlP [j].v += xDelta;
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
	int h, i = primaryWeaponToWeaponInfo [gameData.weapons.nPrimary];

gameData.laser.nGlobalFiringCount += WI_fireCount (i) * (Controls [0].firePrimaryState || Controls [0].firePrimaryDownCount);
if ((gameData.weapons.nPrimary == FUSION_INDEX) && gameData.laser.nGlobalFiringCount) {
	if ((LOCALPLAYER.energy < F1_0 * 2) && 
		 (gameData.fusion.xAutoFireTime == 0)) {
		gameData.laser.nGlobalFiringCount = 0;
		} 
	else {
		flFrameTime += gameData.time.xFrame;
		if (gameData.fusion.xCharge == 0)
			LOCALPLAYER.energy -= F1_0 * 2;
		h = (flFrameTime <= LOCALPLAYER.energy) ? flFrameTime : LOCALPLAYER.energy;
		gameData.fusion.xCharge += h;
		LOCALPLAYER.energy -= h;
		if (LOCALPLAYER.energy <= 0) {
			LOCALPLAYER.energy = 0;
			gameData.fusion.xAutoFireTime = gameData.time.xGame - 1;	//	Fire now!
			} 
		else
			gameData.fusion.xAutoFireTime = gameData.time.xGame + flFrameTime/2 + 1;
		if (gameStates.limitFPS.bFusion && !gameStates.app.tick40fps.bTick)
			return;
		if (gameData.fusion.xCharge < F1_0*2)
			PALETTE_FLASH_ADD (gameData.fusion.xCharge >> 11, 0, gameData.fusion.xCharge >> 11);
		else
			PALETTE_FLASH_ADD (gameData.fusion.xCharge >> 11, gameData.fusion.xCharge >> 11, 0);
		if (gameData.time.xGame < gameData.fusion.xLastSoundTime)		//gametime has wrapped
			gameData.fusion.xNextSoundTime = gameData.fusion.xLastSoundTime = gameData.time.xGame;
		if (gameData.fusion.xNextSoundTime < gameData.time.xGame) {
			if (gameData.fusion.xCharge > F1_0*2) {
				DigiPlaySample (11, F1_0);
				ApplyDamageToPlayer (gameData.objs.console, gameData.objs.console, d_rand () * 4);
				} 
			else {
				CreateAwarenessEvent (gameData.objs.console, PA_WEAPON_ROBOT_COLLISION);
				DigiPlaySample (SOUND_FUSION_WARMUP, F1_0);
				if (gameData.app.nGameMode & GM_MULTI)
					MultiSendPlaySound (SOUND_FUSION_WARMUP, F1_0);
					}
			gameData.fusion.xLastSoundTime = gameData.time.xGame;
			gameData.fusion.xNextSoundTime = gameData.time.xGame + F1_0/8 + d_rand ()/4;
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
	fix	powerup_size = gameData.objs.objects [nObject].size;
	fix	player_size = player->size;
	fix	dist;

Assert (gameData.objs.objects[nObject].nType == OBJ_POWERUP);
dist = VmVecDistQuick (&gameData.objs.objects[nObject].position.vPos, &player->position.vPos);
if ((dist < 2 * (powerup_size + player_size)) && 
	 !(gameData.objs.objects [nObject].flags & OF_SHOULD_BE_DEAD)) {
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
if (gameStates.app.tick40fps.bTick) {
	tSegment	*segP;
	int		nObject;

	segP = gameData.segs.segments + gameData.objs.console->nSegment;
	nObject = segP->objects;
	while (nObject != -1) {
		if (gameData.objs.objects[nObject].nType == OBJ_POWERUP)
			PowerupGrabCheat (gameData.objs.console, nObject);
		nObject = gameData.objs.objects[nObject].next;
		}
	}
}

int	nLastLevelPathCreated = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for tPlayer from current tSegment to goal tSegment.
//	Return true if path created, else return false.
int MarkPlayerPathToSegment (int nSegment)
{
	int		i;
	tObject	*objP = gameData.objs.console;
	short		player_path_length=0;
	int		player_hide_index=-1;

if (nLastLevelPathCreated == gameData.missions.nCurrentLevel)
	return 0;
nLastLevelPathCreated = gameData.missions.nCurrentLevel;
if (CreatePathPoints (objP, objP->nSegment, nSegment, gameData.ai.freePointSegs, &player_path_length, 100, 0, 0, -1) == -1) {
#if TRACE
	//con_printf (CONDBG, "Unable to form path of length %i for myself\n", 100);
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
for (i = 1; i < player_path_length; i++) {
	short			nSegment, nObject;
	vmsVector	seg_center;
	tObject		*objP;

	nSegment = gameData.ai.pointSegs [player_hide_index + i].nSegment;
#if TRACE
	//con_printf (CONDBG, "%3i ", nSegment);
#endif
	seg_center = gameData.ai.pointSegs[player_hide_index+i].point;
	nObject = CreateObject (OBJ_POWERUP, POW_ENERGY, -1, nSegment, &seg_center, &vmdIdentityMatrix,
								   gameData.objs.pwrUp.info [POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP, 1);
	if (nObject == -1) {
		Int3 ();		//	Unable to drop energy powerup for path
		return 1;
		}
	objP = gameData.objs.objects + nObject;
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->lifeleft = F1_0*100 + d_rand () * 4;
	}
return 1;
}

//-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int MarkPathToExit (void)
{
	int	i,j;

	//	---------- Find exit doors ----------
for (i = 0; i <= gameData.segs.nLastSegment; i++)
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++)
		if (gameData.segs.segments [i].children [j] == -2)
			return MarkPlayerPathToSegment (i);
return 0;
}

#endif


//-----------------------------------------------------------------------------
#ifdef _DEBUG
int	Max_objCount_mike = 0;

//	Shows current number of used gameData.objs.objects.
void show_freeObjects (void)
{
	if (!(gameData.app.nFrameCount & 8)) {
		int	i;
		int	count=0;

#if TRACE
		//con_printf (CONDBG, "gameData.objs.nLastObject = %3i, MAX_OBJECTS = %3i, now used = ", gameData.objs.nLastObject, MAX_OBJECTS);
#endif
		for (i=0; i<=gameData.objs.nLastObject; i++)
			if (gameData.objs.objects[i].nType != OBJ_NONE)
				count++;
#if TRACE
		//con_printf (CONDBG, "%3i", count);
#endif
		if (count > Max_objCount_mike) {
			Max_objCount_mike = count;
#if TRACE
			//con_printf (CONDBG, " ***");
#endif
		}
#if TRACE
		//con_printf (CONDBG, "\n");
#endif
	}

}

#endif

//-----------------------------------------------------------------------------
/*
 * reads a tVariableLight structure from a CFILE
 */
void ReadVariableLight (tVariableLight *fl, CFILE *fp)
{
	fl->nSegment = CFReadShort (fp);
	fl->nSide = CFReadShort (fp);
	fl->mask = CFReadInt (fp);
	fl->timer = CFReadFix (fp);
	fl->delay = CFReadFix (fp);
}

//-----------------------------------------------------------------------------
//eof
