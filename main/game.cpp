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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "inferno.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "key.h"
#include "object.h"
#include "objeffects.h"
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
#include "render.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "menu.h"
#include "gameseg.h"
#include "u_mem.h"
#include "light.h"
#include "newdemo.h"
#include "collide.h"
#include "automap.h"
#include "text.h"
#include "fireball.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "gamepal.h"
#include "sphere.h"
#include "textdata.h"
#include "tracker.h"
#include "input.h"
#include "interp.h"
#include "cheats.h"
#include "rle.h"
#include "hiresmodels.h"
#include "monsterball.h"
#include "dropobject.h"
#include "trackobject.h"
#include "soundthreads.h"
#include "sparkeffect.h"
#include "createmesh.h"
#ifdef __macosx__
#include "SDL/SDL_syswm.h"
#else
#include "SDL_syswm.h"
#endif
#include "renderthreads.h"
#include "fvi.h"

u_int32_t nCurrentVGAMode;

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

#if DBG
int	MarkCount = 0;                 // number of debugging marks set
int	Speedtest_startTime;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	SpeedtestCount=0;				//	number of times to do the debug test.
#endif

#if defined (TIMER_TEST) && defined (_DEBUG)
fix TimerValue,actual_lastTimerValue,_xLastFrameTime;
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

#if DBG                          //these only exist if debugging

int bGameDoubleBuffer = 1;     //double buffer by default
fix xFixedFrameTime = 0;          //if non-zero, set frametime to this

#endif

CBitmap bmBackground;

#define BACKGROUND_NAME "statback.pcx"

//	Function prototypes for GAME.C exclusively.

void FireLaser (void);
void SlideTextures (void);
void PowerupGrabCheatAll (void);

//	Other functions
void MultiCheckForKillGoalWinner ();
void MultiCheckForEntropyWinner ();

// text functions

#if DBG
void ShowFrameRate (void);
#endif

//	==============================================================================================

//this is called once per game
void InitGame (void)
{
atexit (CloseGame);             //for cleanup
/*---*/PrintLog ("Initializing game data\n  Objects ...\n");
InitObjects ();
/*---*/PrintLog ("  Special effects...\n");
InitSpecialEffects ();
/*---*/PrintLog ("  AI system...\n");
InitAISystem ();
//*---*/PrintLog ("  gauge canvases...\n");
//	InitGaugeCanvases ();
/*---*/PrintLog ("  exploding walls...\n");
InitExplodingWalls ();
/*---*/PrintLog ("  particle systems...\n");
particleManager.Shutdown ();
particleManager.Init ();
/*---*/PrintLog ("  loading background bitmap...\n");
LoadBackgroundBitmap ();
InitAutomapData ();
InitDefaultPlayerShip ();
nClearWindow = 2;		//	do portal only window clear.
InitDetailLevels (gameStates.app.nDetailLevel);
gameStates.render.color.bRenderLightmaps =
	gameStates.render.color.bLightmapsOk &&
	gameStates.render.bAmbientColor &&
	gameOpts->render.color.bUseLightmaps;
gameStates.ogl.bGlTexMerge =
	gameOpts->ogl.bGlTexMerge &&
	gameStates.render.textures.bGlsTexMergeOk;
fpDrawTexPolyMulti = gameStates.render.color.bRenderLightmaps ? G3DrawTexPolyLightmap : G3DrawTexPolyMulti;
}

//------------------------------------------------------------------------------

void ShowInGameWarning (const char *s)
{
if (screen.Width () && screen.Height ()) {
	const char	*hs, *ps = strstr (s, "Error");

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
		StartTime (0);
	}
}

//------------------------------------------------------------------------------
//these should be in gr.h
#define cv_w  Bitmap ().Width ()
#define cv_h  Bitmap ().Height ()

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
CCanvas::SetCurrent (NULL);
fontManager.SetCurrent (GAME_FONT);

if (bGameCockpitCopyCode)
	D2_FREE (bGameCockpitCopyCode);
bGameCockpitCopyCode  = NULL;
switch (gameStates.render.cockpit.nMode) {
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:
     	gameData.render.window.hMax = (screen.Height () * 2) / 3;
		if (gameData.render.window.h > gameData.render.window.hMax)
			gameData.render.window.h = gameData.render.window.hMax;
		if (gameData.render.window.w > gameData.render.window.wMax)
			gameData.render.window.w = gameData.render.window.wMax;
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
		GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
		break;

	case CM_FULL_SCREEN:
		gameData.render.window.hMax = screen.Height ();
		gameData.render.window.h = gameData.render.window.hMax;
		gameData.render.window.w = gameData.render.window.wMax;
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
		GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
		break;

	case CM_STATUS_BAR:
		{
		int h = gameData.pig.tex.bitmaps [0][gameData.pig.tex.cockpitBmIndex [CM_STATUS_BAR + (gameStates.video.nDisplayMode ? (gameData.models.nCockpits / 2) : 0)].index].Height ();
		if (gameStates.app.bDemoData)
			h *= 2;
		if (screen.Height () > 480)
			h = (int) ((double) h * (double) screen.Height () / 480.0);
     	gameData.render.window.hMax = screen.Height () - h;
		gameData.render.window.h = gameData.render.window.hMax;
		gameData.render.window.w = gameData.render.window.wMax;
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
		GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
		}
		break;

	case CM_LETTERBOX: {
		int x = 0;
		int w = gameStates.render.vr.buffers.render[0].Width ();		//VR_render_width;
		int h = (int) ((gameStates.render.vr.buffers.render[0].Bitmap ().Height () * 7) / 10 / ((double) screen.Height () / (double) screen.Width () / 0.75));
		int y = (gameStates.render.vr.buffers.render[0].Bitmap ().Height () - h) / 2;
		GameInitRenderSubBuffers (x, y, w, h);
		break;
		}
	}
CCanvas::SetCurrent (NULL);
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
	gameStates.render.vr.buffers.render[0].SetupPane (&gameStates.render.vr.buffers.subRender[0], x, y, w, h);
	gameStates.render.vr.buffers.render[1].SetupPane (&gameStates.render.vr.buffers.subRender[1], x, y, w, h);
	}
}

//------------------------------------------------------------------------------

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void GameInitRenderBuffers (int nScreenSize, int render_w, int render_h, int render_method, int flags)
{
//	if (vga_check_mode (nScreenSize) != 0)
//		Error ("Cannot set requested video mode");

gameStates.render.vr.nScreenSize = nScreenSize;
gameStates.render.vr.nScreenFlags = flags;
//NEWVR
VRResetParams ();
gameStates.render.vr.nRenderMode = render_method;
gameData.render.window.w = render_w;
gameData.render.window.h = render_h;
if (gameStates.render.vr.buffers.offscreen) {
	gameStates.render.vr.buffers.offscreen->Destroy ();
	}

if ((gameStates.render.vr.nRenderMode == VR_AREA_DET) || (gameStates.render.vr.nRenderMode==VR_INTERLACED))	{
	if (render_h*2 < 200)	{
		gameStates.render.vr.buffers.offscreen = CCanvas::Create (render_w, 200);
		}
	else {
		gameStates.render.vr.buffers.offscreen = CCanvas::Create (render_w, render_h*2);
		}
	gameStates.render.vr.buffers.offscreen->SetupPane (&gameStates.render.vr.buffers.render[0], 0, 0, render_w, render_h);
	gameStates.render.vr.buffers.offscreen->SetupPane (&gameStates.render.vr.buffers.render[1], 0, render_h, render_w, render_h);
	}
else {
	if (render_h < 200) {
		gameStates.render.vr.buffers.offscreen = CCanvas::Create (render_w, 200);
		}
	else {
		gameStates.render.vr.buffers.offscreen = CCanvas::Create (render_w, render_h);
      }
	gameStates.render.vr.buffers.offscreen->Bitmap ().SetMode (BM_OGL);
	gameStates.render.vr.buffers.offscreen->SetupPane (&gameStates.render.vr.buffers.render[0], 0, 0, render_w, render_h);
	gameStates.render.vr.buffers.offscreen->SetupPane (&gameStates.render.vr.buffers.render[1], 0, 0, render_w, render_h);
	}
GameInitRenderSubBuffers (0, 0, render_w, render_h);
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

void StartTime (int bReset)
{
if (gameData.time.nPaused <= 0)
	return;
if (bReset)
	gameData.time.nPaused = 1;
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
gameData.time.xFrame = 0;
gameData.time.xLast = TimerGetFixedSeconds ();
}

//------------------------------------------------------------------------------

#if DBG
extern int bSavingMovieFrames;
int Movie_fixed_frametime;
#else
#define bSavingMovieFrames	0
#define Movie_fixed_frametime	0
#endif


void CalcFrameTime (void)
{

if (gameData.app.bGamePaused) {
	gameData.time.xLast = TimerGetFixedSeconds ();
	gameData.time.xFrame = 0;
	gameData.time.xRealFrame = 0;
	return;
	}

fix 	timerValue,
		xLastFrameTime = gameData.time.xFrame;
GetSlowTicks ();
#if 1
	fix xMinFrameTime = (MAXFPS ? f1_0 / MAXFPS : 1);
do {
	timerValue = TimerGetFixedSeconds ();
   gameData.time.xFrame = timerValue - gameData.time.xLast;
	if (MAXFPS < 2)
		break;
	G3_SLEEP (1);
	} while (gameData.time.xFrame < xMinFrameTime);
#else
int	tFrameTime, txMinFrameTime;

if (MAXFPS) {
	txMinFrameTime = 1000 / MAXFPS;
#if 0
	static float tSlack = 0;
	tSlack += 1000.0f / MAXFPS - txMinFrameTime;
	if (tSlack >= 1) {
		txMinFrameTime += (int) tSlack;
		tSlack -= (int) tSlack;
		}
#endif
	}

if (MAXFPS > 1) {
	tFrameTime = gameStates.app.nSDLTicks - gameData.time.tLast;
	if (tFrameTime < txMinFrameTime)
		G3_SLEEP (txMinFrameTime - tFrameTime);
	}
timerValue = SECS2X (gameStates.app.nSDLTicks);
#endif
gameData.time.xFrame = timerValue - gameData.time.xLast;
gameData.time.xRealFrame = gameData.time.xFrame;
if (gameStates.app.cheats.bTurboMode)
	gameData.time.xFrame *= 2;
gameData.time.xLast = timerValue;
gameData.time.tLast = SDL_GetTicks ();
if (gameData.time.xFrame < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.time.xFrame = xLastFrameTime;		//d:\temp\dm_test.then use time from last frame
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

COMPUTE_SEGMENT_CENTER (&gameData.objs.consoleP->info.position.vPos,segP);
COMPUTE_SIDE_CENTER (&vp,segP,tSide);
vp -= gameData.objs.consoleP->info.position.vPos;
/*
gameData.objs.consoleP->info.position.mOrient = vmsMatrix::Create(vp, NULL, NULL);
*/
// TODO: MatrixCreateFCheck
gameData.objs.consoleP->info.position.mOrient = vmsMatrix::CreateF(vp);
RelinkObjToSeg (OBJ_IDX (gameData.objs.consoleP), SEG_IDX (segP));
}

//------------------------------------------------------------------------------

#ifdef NETWORK
void GameDrawTimeLeft ()
{
	char temp_string[30];
	fix timevar;
	int i;
	static int nId = 0;

fontManager.SetCurrent (GAME_FONT);    //GAME_FONT
fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
timevar=I2X (netGame.xPlayTimeAllowed*5*60);
i = X2I (timevar-gameStates.app.xThisLevelTime) + 1;
sprintf (temp_string, TXT_TIME_LEFT, i);
if (i >= 0)
	nId = GrString (0, 32, temp_string, &nId);
}
#endif

//------------------------------------------------------------------------------

void modex_clear_box (int x,int y,int w,int h)
{
	CCanvas *canvP;

CCanvas::Push ();
canvP = CCanvas::Create (w,h);
CCanvas::SetCurrent (canvP);
CCanvas::Current ()->Clear (BLACK_RGBA);
CCanvas::Pop ();
GrBitmapM (x,y, &canvP->Bitmap (), 0);
canvP->Destroy ();

}

//------------------------------------------------------------------------------

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
	objP->info.controlType = CT_FLYING;
	objP->info.movementType = MT_PHYSICS;

	objP->mType.physInfo.velocity.SetZero();
	objP->mType.physInfo.thrust.SetZero();
	objP->mType.physInfo.rotVel.SetZero();
	objP->mType.physInfo.rotThrust.SetZero();
}

//void morph_test (), morph_step ();


//	------------------------------------------------------------------------------------

//put up the help message
void DoShowHelp ()
{
ShowHelp ();
}

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
//@@	gameData.objs.consoleP->mType.physInfo.rotVel.x += rx;
//@@	gameData.objs.consoleP->mType.physInfo.rotVel.z += rz;
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
										1, F1_0, I2X (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END, NULL, 0, SOUNDCLASS_PLAYER);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_IGNITE);
#endif
		}
	else {
		DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
		DigiLinkSoundToObject2 ((short) SOUND_AFTERBURNER_PLAY, (short) LOCALPLAYER.nObject, 
										0, F1_0, I2X (256), SOUNDCLASS_PLAYER);
#ifdef NETWORK
		if (gameData.app.nGameMode & GM_MULTI)
		 	MultiSendSoundFunction (0,0);
#endif
		}
	}
gameStates.gameplay.bLastAfterburnerState = Controls [0].afterburnerState;
gameStates.gameplay.xLastAfterburnerCharge = gameData.physics.xAfterburnerCharge;
}

//------------------------------------------------------------------------------

#define ADD_HELP_OPT(_text)	m [opt].nType = NM_TYPE_TEXT; m [opt++].text = reinterpret_cast<char*> (_text);

void ShowHelp ()
{
	int nitems, opt = 0;
	tMenuItem m [40];
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
	ADD_HELP_OPT (TXT_HELP_QUICKSAVE);
	ADD_HELP_OPT (TXT_HELP_QUICKLOAD);
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
	paletteManager.SaveEffectAndReset ();
	ExecMenutiny2 (NULL, TXT_KEYS, nitems, m, NULL);
	paletteManager.LoadEffect ();
}

//------------------------------------------------------------------------------

//temp function until Matt cleans up game sequencing
//deal with rear view - switch it on, or off, or whatever
void CheckRearView ()
{

	#define LEAVE_TIME 0x1000		//how long until we decide key is down	 (Used to be 0x4000)

	static int leave_mode;
	static fix entryTime;
#if DBG
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

// ----------------------------------------------------------------------------
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
paletteManager.LoadEffect  ();
SetScreenMode (SCREEN_GAME);
paletteManager.ResetEffect ();
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
//gameStates.input.keys.bRepeat = 0;                // Don't allow repeat in game
gameStates.input.keys.bRepeat = 1;                // Do allow repeat in game
#ifdef EDITOR
	if (gameData.segs.segments[gameData.objs.consoleP->info.nSegment].nSegment == -1)      //tSegment no longer exists
		RelinkObjToSeg (OBJ_IDX (gameData.objs.consoleP), SEG_IDX (Cursegp));

	if (!check_obj_seg (gameData.objs.consoleP))
		MovePlayerToSegment (Cursegp,Curside);
#endif
gameData.objs.viewerP = gameData.objs.consoleP;
#if TRACE
//con_printf (CONDBG, "   FlyInit d:\temp\dm_test.\n");
#endif
FlyInit (gameData.objs.consoleP);
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
#if DBG
	else if (newFuncMode == FMODE_MENU)
		gameStates.app.bEnterGame = 0;
#endif
	}
}


//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void RunGame (void)
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
	PROF_START
	int playerShields;
		// GAME LOOP!
#if DBG
	if (gameStates.render.automap.bDisplay)
#endif
	gameStates.render.automap.bDisplay = 0;
	gameStates.app.bConfigMenu = 0;
	if (gameData.objs.consoleP != OBJECTS + LOCALPLAYER.nObject) {
#if TRACE
	    //con_printf (CONDBG,"gameData.multiplayer.nLocalPlayer=%d nObject=%d",gameData.multiplayer.nLocalPlayer,LOCALPLAYER.nObject);
#endif
	    //Assert (gameData.objs.consoleP == &OBJECTS[LOCALPLAYER.nObject]);
		}
	playerShields = LOCALPLAYER.shields;
	gameStates.app.nExtGameStatus = GAMESTAT_RUNNING;
	if (!GameLoop (1, 1)) {		// Do game loop with rendering and reading controls.
		PROF_END(ptFrame);
		continue;
		}
	if (gameStates.app.bSingleStep) {
		while (!(c = KeyInKey ()))
			;
		if (c == KEY_ALTED + KEY_CTRLED + KEY_ESC)
			gameStates.app.bSingleStep = 0;
		}
	//if the tPlayer is taking damage, give up guided missile control
	if (LOCALPLAYER.shields != playerShields)
		ReleaseGuidedMissile (gameData.multiplayer.nLocalPlayer);
	//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();	// Handle RedBook Audio Repeating.
	if (gameStates.app.bConfigMenu) {
		int double_save = bScanlineDouble;
		if (!IsMultiGame) {
			paletteManager.SaveEffect ();
			paletteManager.ResetEffect ();
			paletteManager.LoadEffect  ();
			}
		ConfigMenu ();
		if (bScanlineDouble != double_save)
			InitCockpit ();
		if (!IsMultiGame)
			paletteManager.LoadEffect ();
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
		paletteManager.SaveEffect ();
		paletteManager.ResetEffect ();
		paletteManager.LoadEffect  ();
		choice = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
		paletteManager.LoadEffect ();
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
	PROF_END(ptFrame);
#if PROFILING
	if (gameData.app.nFrameCount % 10000 == 0)
		memset (&gameData.profiler, 0, sizeof (gameData.profiler));
#endif
	if (gameStates.app.nFunctionMode != FMODE_GAME)
		longjmp (gameExitPoint, 0);
		}
	}
#ifdef MWPROFILE
ProfilerSetStatus (0);
#endif
DigiStopAll ();
if (gameStates.sound.bD1Sound) {
	gameStates.sound.bD1Sound = 0;
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
	paletteManager.FadeOut ();			// Fade out before going to menu
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
meshBuilder.DestroyVBOs ();
}

//-----------------------------------------------------------------------------
//called at the end of the program

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
EndSoundThread ();
GrClose ();
PrintLog ("unloading addon sounds\n");
FreeAddonSounds ();
DigiClose ();
FreeUserSongs ();
RLECacheClose ();
BMFreeExtraObjBitmaps ();
BMFreeExtraModels ();
PrintLog ("unloading render buffers\n");
FreeTranspItems ();
PrintLog ("unloading string pool\n");
FreeStringPool ();
PrintLog ("unloading level messages\n");
FreeTextData (gameData.messages);
FreeTextData (&gameData.sounds);
PrintLog ("unloading hires animations\n");
PiggyFreeHiresAnimations ();
PiggyBitmapPageOutAll (0);
PrintLog ("freeing sound buffers\n");
DigiFreeSoundBufs ();
FreeSoundReplacements ();
PrintLog ("unloading hoard data\n");
FreeHoardData ();
PrintLog ("unloading auxiliary poly model data\n");
G3FreeAllPolyModelItems ();
PrintLog ("unloading hires models\n");
FreeHiresModels (0);
PrintLog ("unloading tracker list\n");
DestroyTrackerList ();
PrintLog ("unloading lightmap data\n");
DestroyLightmaps ();
PrintLog ("unloading particle data\n");
particleManager.Shutdown ();
PrintLog ("unloading shield sphere data\n");
DestroySphere (&gameData.render.shield);
DestroySphere (&gameData.render.monsterball);
PrintLog ("unloading HUD icons\n");
FreeInventoryIcons ();
FreeObjTallyIcons ();
PrintLog ("unloading extra texture data\n");
FreeExtraImages ();
PrintLog ("unloading shield data\n");
FreeSphereCoord ();
PrintLog ("unloading palettes\n");
FreeSkyBoxSegList ();
CloseDynLighting ();
if (gameStates.render.vr.buffers.offscreen) {
	gameStates.render.vr.buffers.offscreen->Destroy ();
	gameStates.render.vr.buffers.offscreen = NULL;
}
PrintLog ("unloading gauge data\n");
CloseGaugeCanvases ();
PrintLog ("restoring effect bitmaps\n");
RestoreEffectBitmapIcons ();
if (bGameCockpitCopyCode) {
	D2_FREE (bGameCockpitCopyCode);
	bGameCockpitCopyCode = NULL;
}
if (bmBackground.Buffer ()) {
	PrintLog ("unloading background bitmap\n");
	bmBackground.DestroyBuffer ();
	}
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
PrintLog ("unloading custom background data\n");
NMFreeAltBg (1);
SaveBanList ();
FreeBanList ();
PrintLog ("peak memory consumption: %ld bytes\n", nMaxAllocd);
SDL_Quit ();
#if 0
if (fErr) {
	fclose (fErr);
	fErr = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------

CCanvas *GetCurrentGameScreen (void)
{
return gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage;
}

//-----------------------------------------------------------------------------

#if DBG
void SpeedtestFrame (void);
uint nDebugSlowdown = 0;
#endif

#ifdef EDITOR
extern void player_follow_path (tObject *objP);
extern void check_create_player_path (void);

#endif

//returns ptr to escort robot, or NULL
tObject *find_escort ()
{
	int 		i;
	tObject	*objP = OBJECTS;

FORALL_ROBOT_OBJS (objP, i)
	if (IS_GUIDEBOT (objP))
		return objP;
return NULL;
}

//-----------------------------------------------------------------------------

extern void ProcessSmartMinesFrame (void);
extern void DoSeismicStuff (void);

#if DBG
int bSavingMovieFrames=0;
int __Movie_frame_num=0;

#define MAX_MOVIE_BUFFER_FRAMES 250
#define MOVIE_FRAME_SIZE	 (320 * 200)

ubyte *movieFrameBuffer;
int nMovieFrames;
char movie_path[50] = ".\\";

CBitmap bmMovie;

void flush_movie_buffer ()
{
	char savename[128];
	int f;

StopTime ();
#if TRACE
	//con_printf (CONDBG,"Flushing movie bufferd:\temp\dm_test.");
#endif
bmMovie.SetBuffer (movieFrameBuffer);
for (f = 0;f < nMovieFrames; f++) {
	sprintf (savename, "%sfrm%04d.pcx",movie_path,__Movie_frame_num);
	__Movie_frame_num++;
	pcx_write_bitmap (savename, &bmMovie);
	bmMovie.SetBuffer (bmMovie.Buffer () + MOVIE_FRAME_SIZE);
	}

nMovieFrames = 0;
bmMovie.SetBuffer (NULL);
StartTime (0);
}

//-----------------------------------------------------------------------------

void toggle_movie_saving ()
{
	int exit;

	bSavingMovieFrames = !bSavingMovieFrames;

	if (bSavingMovieFrames) {
		tMenuItem m[1];

		memset (m, 0, sizeof (m));
		m[0].nType=NM_TYPE_INPUT;
		m[0].text_len = 50;
		m[0].text = movie_path;
		exit = ExecMenu (NULL, "Directory for movie frames?" , 1, & (m[0]), NULL, NULL);

		if (exit==-1) {
			bSavingMovieFrames = 0;
			return;
		}

		while (::isspace (movie_path [strlen (movie_path)-1]))
			movie_path[strlen (movie_path)-1] = 0;
		if (movie_path[strlen (movie_path)-1]!='\\' && movie_path[strlen (movie_path)-1]!=':')
			strcat (movie_path,"\\");


		if (!movieFrameBuffer) {
			movieFrameBuffer = reinterpret_cast<ubyte*> (D2_ALLOC (MAX_MOVIE_BUFFER_FRAMES * MOVIE_FRAME_SIZE));
			if (!movieFrameBuffer) {
				Int3 ();
				bSavingMovieFrames=0;
			}

			nMovieFrames=0;

			bmMovie.Init (BM_LINEAR, 0, 0, 320, 200, 1, NULL);
			bmMovie.SetFlags (0);

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
memcpy (movieFrameBuffer + nMovieFrames * MOVIE_FRAME_SIZE, screen.Bitmap ().Buffer (), MOVIE_FRAME_SIZE);
nMovieFrames++;
if (nMovieFrames == MAX_MOVIE_BUFFER_FRAMES)
	flush_movie_buffer ();
}

#endif

//-----------------------------------------------------------------------------
//if water or fire level, make occasional sound
void DoAmbientSounds ()
{
if (gameStates.app.bPlayerIsDead)
	return;

	int	bLava, bWater;
	short nSound;

	bLava = (SEGMENT2S [gameData.objs.consoleP->info.nSegment].s2Flags & S2F_AMBIENT_LAVA);
	bWater = (SEGMENT2S [gameData.objs.consoleP->info.nSegment].s2Flags & S2F_AMBIENT_WATER);

if (bLava) {							//has lava
	nSound = SOUND_AMBIENT_LAVA;
	if (bWater && (d_rand () & 1))	//both, pick one
		nSound = SOUND_AMBIENT_WATER;
	}
else if (bWater)						//just water
	nSound = SOUND_AMBIENT_WATER;
else
	return;
if (((d_rand () << 3) < gameData.time.xFrame))	//play the nSound
	DigiPlaySample (nSound, (fix) (d_rand () + f1_0 / 2));
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
		gameData.objs.consoleP->mType.physInfo.rotVel[X] += (d_rand () - 16384)/8;
		gameData.objs.consoleP->mType.physInfo.rotVel[Z] += (d_rand () - 16384)/8;
		vRand = vmsVector::Random();
		xBump = F1_0*4;
		if (gameData.fusion.xCharge > F1_0*2)
			xBump = gameData.fusion.xCharge*4;
		BumpOneObject (gameData.objs.consoleP, &vRand, xBump);
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
gameStates.app.bSaveScreenshot = 1;
SaveScreenShot (0, 0);
}

//-----------------------------------------------------------------------------

int PlayerHasHeadlight (int nPlayer)
{
return EGI_FLAG (headlight.bAvailable, 0, 0, 0) &&
		 (EGI_FLAG (headlight.bBuiltIn, 0, 1, 0) ||
		  ((gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT) != 0));
}

//-----------------------------------------------------------------------------

int HeadlightIsOn (int nPlayer)
{
#if DBG
if (!PlayerHasHeadlight (nPlayer))
	return 0;
if (!(gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON))
	return 0;
return 1;
#else
return PlayerHasHeadlight (nPlayer) && ((gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags & PLAYER_FLAGS_HEADLIGHT_ON) != 0);
#endif
}

//-----------------------------------------------------------------------------

void SetPlayerHeadlight (int nPlayer, int bOn)
{
if (bOn)
	gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags |= PLAYER_FLAGS_HEADLIGHT_ON;
else
	gameData.multiplayer.players [(nPlayer < 0) ? gameData.multiplayer.nLocalPlayer : nPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
}

//-----------------------------------------------------------------------------

void DrainHeadlightPower (void)
{
	static int bTurnedOff = 0;

if (!EGI_FLAG (headlight.bDrainPower, 0, 0, 1))
	return;
if (!HeadlightIsOn (-1))
	return;

LOCALPLAYER.energy -= (gameData.time.xFrame * 3 / 8);
if (LOCALPLAYER.energy < I2X (10)) {
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

void DoEffectsFrame (void)
{
//PrintLog ("DoEffectsFrame \n");
if (gameStates.app.bMultiThreaded && gameData.app.bUseMultiThreading [rtEffects] && tiEffects.pThread) {
	while (tiEffects.bExec)
		G3_SLEEP (0);
	tiEffects.bExec = 1;
	}
else {
	lightningManager.DoFrame ();
	DoEnergySparkFrame ();
	DoParticleFrame ();
	}
}

//-----------------------------------------------------------------------------
// -- extern void lightning_frame (void);

void GameRenderFrame ();
void OmegaChargeFrame (void);
void FlickerLights ();

//int bLog = 0;

int GameLoop (int bRenderFrame, int bReadControls)
{
gameStates.app.bGameRunning = 1;
gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
gameData.objs.nLastObject [1] = gameData.objs.nLastObject [0];
if (!MultiProtectGame ()) {
	SetFunctionMode (FMODE_MENU);
	return 1;
	}
if (gameStates.gameplay.bMineMineCheat) {
	DoWowieCheat (0, 0);
	GasolineCheat (0);
	}
AutoBalanceTeams ();
MultiSendTyping ();
MultiSendWeapons (0);
MultiSyncKills ();
MultiRefillPowerups ();
UpdatePlayerStats ();
UpdatePlayerWeaponInfo ();
paletteManager.FadeEffect ();		//	Should leave palette effect up for as long as possible by putting right before render.
//PrintLog ("DoAfterburnerStuff\n");
DoAfterburnerStuff ();
//PrintLog ("DoCloakStuff\n");
DoCloakStuff ();
//PrintLog ("DoInvulnerableStuff\n");
DoInvulnerableStuff ();
//PrintLog ("RemoveObsoleteStuckObjects \n");
RemoveObsoleteStuckObjects ();
//PrintLog ("InitAIFrame \n");
InitAIFrame ();
//PrintLog ("DoFinalBossFrame\n");
DoFinalBossFrame ();
// -- lightning_frame ();
// -- recharge_energy_frame ();
//PrintLog ("DrainHeadlightPower\n");
DrainHeadlightPower ();
#ifdef EDITOR
check_create_player_path ();
player_follow_path (gameData.objs.consoleP);
#endif

if (IsMultiGame) {
	AddServerToTracker ();
   MultiDoFrame ();
	CheckMonsterballScore ();
	if (netGame.xPlayTimeAllowed && (gameStates.app.xThisLevelTime >= I2X ((netGame.xPlayTimeAllowed * 5 * 60))))
       MultiCheckForKillGoalWinner ();
	else
		MultiCheckForEntropyWinner ();
  }
if (bRenderFrame) {
	if (gameStates.render.cockpit.bRedraw) {			//screen need redrawing?
		//PrintLog ("InitCockpit\n");
		InitCockpit ();
		gameStates.render.cockpit.bRedraw = 0;
	}
	//PrintLog ("GameRenderFrame\n");
	GameRenderFrame ();
	gameStates.app.bUsingConverter = 0;
#if DBG
	if (bSavingMovieFrames)
		save_movie_frame ();
#endif
	}
//PrintLog ("CalcFrameTime\n");
CalcFrameTime ();
//PrintLog ("DeadPlayerFrame\n");
DeadPlayerFrame ();
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
	DoReactorDeadFrame ();
	}
//PrintLog ("ProcessSmartMinesFrame \n");
ProcessSmartMinesFrame ();
//PrintLog ("DoSeismicStuff\n");
DoSeismicStuff ();
//PrintLog ("DoEffectsFrame\n");
DoEffectsFrame ();
//PrintLog ("DoAmbientSounds\n");
DoAmbientSounds ();
#if DBG
if (gameData.speedtest.bOn)
	SpeedtestFrame ();
#endif
if (bReadControls)
	ReadControls ();
else
	memset (&Controls, 0, sizeof (Controls));
//PrintLog ("DropPowerups\n");
DropPowerups ();
gameData.time.xGame += gameData.time.xFrame;
if (gameData.time.xGame < 0 || gameData.time.xGame > I2X (0x7fff - 600)) {
	gameData.time.xGame = gameData.time.xFrame;	//wrap when goes negative, or gets within 10 minutes
	}
if (IsMultiGame && netGame.xPlayTimeAllowed)
   gameStates.app.xThisLevelTime +=gameData.time.xFrame;
//PrintLog ("DigiSyncSounds\n");
DigiSyncSounds ();
if (gameStates.app.bEndLevelSequence) {
	DoEndLevelFrame ();
	PowerupGrabCheatAll ();
	DoSpecialEffects ();
	return 1;					//skip everything else
	}
if (gameData.demo.nState != ND_STATE_PLAYBACK) {
//PrintLog ("DoExplodingWallFrame\n");
	DoExplodingWallFrame ();
	}
if ((gameData.demo.nState != ND_STATE_PLAYBACK) || (gameData.demo.nVcrState != ND_STATE_PAUSED)) {
//PrintLog ("DoSpecialEffects\n");
	DoSpecialEffects ();
//PrintLog ("WallFrameProcess\n");
	WallFrameProcess ();
//PrintLog ("TriggersFrameProcess\n");
	TriggersFrameProcess ();
	}
if (gameData.reactor.bDestroyed && (gameData.demo.nState == ND_STATE_RECORDING)) {
	NDRecordControlCenterDestroyed ();
	}
UpdateFlagClips ();
MultiSetFlagPos ();
SetPlayerPaths ();
//PrintLog ("FlashFrame\n");
FlashFrame ();
if (gameData.demo.nState == ND_STATE_PLAYBACK) {
	NDPlayBackOneFrame ();
	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		longjmp (gameExitPoint, 0);		// Go back to menu
	}
else { // Note the link to above!
	LOCALPLAYER.homingObjectDist = -1;		//	Assume not being tracked.  LaserDoWeaponSequence modifies this.
	//PrintLog ("UpdateAllObjects\n");
	if (!UpdateAllObjects ())
		return 0;
	//PrintLog ("PowerupGrabCheatAll \n");
	PowerupGrabCheatAll ();
	if (gameStates.app.bEndLevelSequence)	//might have been started during move
		return 1;
	//PrintLog ("FuelcenUpdateAll\n");
	FuelcenUpdateAll ();
	//PrintLog ("DoAIFrameAll\n");
	DoAIFrameAll ();
	if (AllowedToFireLaser ()) {
		//PrintLog ("FireLaser\n");
		FireLaser ();				// Fire Laser!
		}
	if (!FusionBump ())
		return 1;
	if (gameData.laser.nGlobalFiringCount) {
		//	Don't cap here, gets capped in CreateNewWeapon and is based on whether in multiplayer mode, MK, 3/27/95
		// if (gameData.fusion.xCharge > F1_0*2)
		// 	gameData.fusion.xCharge = F1_0*2;
		gameData.laser.nGlobalFiringCount -= LocalPlayerFireLaser ();	//LaserFireObject (LOCALPLAYER.nObject, gameData.weapons.nPrimary);
		}
	if (gameData.laser.nGlobalFiringCount < 0)
		gameData.laser.nGlobalFiringCount = 0;
	}
if (gameStates.render.bDoAppearanceEffect) {
	CreatePlayerAppearanceEffect (gameData.objs.consoleP);
	gameStates.render.bDoAppearanceEffect = 0;
	if (IsMultiGame && netGame.invul) {
		LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
		LOCALPLAYER.invulnerableTime = gameData.time.xGame-I2X (27);
		bFakingInvul = 1;
		SetSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
		}
	}
//PrintLog ("DoSlowMotionFrame\n");
DoSlowMotionFrame ();
//PrintLog ("CheckInventory\n");
CheckInventory ();
//PrintLog ("OmegaChargeFrame \n");
OmegaChargeFrame ();
//PrintLog ("SlideTextures \n");
SlideTextures ();
//PrintLog ("FlickerLights \n");
FlickerLights ();
//PrintLog ("\n");
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
#if DBG
			if ((nSegment == nDbgSeg) && ((nDbgSide < 0) || (nSide == nDbgSide)))
				nSegment = nSegment;
#endif
		i = (SEGMENT2S [nSegment].special == SEGMENT_IS_SKYBOX) ? 3 : 8;
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

if (gameData.weapons.firing [0].nDuration)
	gameData.laser.nGlobalFiringCount += WI_fireCount (i);
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
			paletteManager.BumpEffect (gameData.fusion.xCharge >> 11, 0, gameData.fusion.xCharge >> 11);
		else
			paletteManager.BumpEffect (gameData.fusion.xCharge >> 11, gameData.fusion.xCharge >> 11, 0);
		if (gameData.time.xGame < gameData.fusion.xLastSoundTime)		//gametime has wrapped
			gameData.fusion.xNextSoundTime = gameData.fusion.xLastSoundTime = gameData.time.xGame;
		if (gameData.fusion.xNextSoundTime < gameData.time.xGame) {
			if (gameData.fusion.xCharge > F1_0*2) {
				DigiPlaySample (11, F1_0);
				ApplyDamageToPlayer (gameData.objs.consoleP, gameData.objs.consoleP, d_rand () * 4);
				}
			else {
				CreateAwarenessEvent (gameData.objs.consoleP, WEAPON_ROBOT_COLLISION);
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
void PowerupGrabCheat (tObject *playerP, int nObject)
{
	tObject		*powerupP = OBJECTS + nObject;
	tTransformation	*posP = OBJPOS (playerP);
	vmsVector	vCollision;

Assert (powerupP->info.nType == OBJ_POWERUP);
if (powerupP->info.nFlags & OF_SHOULD_BE_DEAD)
	return;
if (vmsVector::Dist (powerupP->info.position.vPos, posP->vPos) >=
	 2 * (playerP->info.xSize + powerupP->info.xSize) / (gameStates.app.bHaveExtraGameInfo [IsMultiGame] + 1))
	return;
vCollision = vmsVector::Avg (powerupP->info.position.vPos, posP->vPos);
CollidePlayerAndPowerup (playerP, powerupP, &vCollision);
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
	short nObject = SEGMENTS [gameData.objs.consoleP->info.nSegment].objects;
	while (nObject != -1) {
		if (OBJECTS [nObject].info.nType == OBJ_POWERUP)
			PowerupGrabCheat (gameData.objs.consoleP, nObject);
		nObject = OBJECTS [nObject].info.nNextInSeg;
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
	tObject	*objP = gameData.objs.consoleP;
	short		player_path_length=0;
	int		player_hide_index=-1;

if (nLastLevelPathCreated == gameData.missions.nCurrentLevel)
	return 0;
nLastLevelPathCreated = gameData.missions.nCurrentLevel;
if (CreatePathPoints (objP, objP->info.nSegment, nSegment, gameData.ai.freePointSegs, &player_path_length, 100, 0, 0, -1) == -1) {
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
	vmsVector	vSegCenter;
	tObject		*objP;

	nSegment = gameData.ai.pointSegs [player_hide_index + i].nSegment;
#if TRACE
	//con_printf (CONDBG, "%3i ", nSegment);
#endif
	vSegCenter = gameData.ai.pointSegs[player_hide_index+i].point;
	nObject = CreatePowerup (POW_ENERGY, -1, nSegment, vSegCenter, 1);
	if (nObject == -1) {
		Int3 ();		//	Unable to drop energy powerup for path
		return 1;
		}
	objP = OBJECTS + nObject;
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->info.nId].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->info.xLifeLeft = F1_0*100 + d_rand () * 4;
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
#if DBG
int	Max_objCount_mike = 0;

//	Shows current number of used OBJECTS.
void show_freeObjects (void)
{
	if (!(gameData.app.nFrameCount & 8)) {
		int		i;
		tObject	*objP;
		int		count = 0;

#if TRACE
		//con_printf (CONDBG, "gameData.objs.nLastObject [0] = %3i, MAX_OBJECTS = %3i, now used = ", gameData.objs.nLastObject [0], MAX_OBJECTS);
#endif
		FORALL_OBJS (objP, i)
			if (objP->info.nType != OBJ_NONE)
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
 * reads a tVariableLight structure from a CFile
 */
void ReadVariableLight (tVariableLight *fl, CFile& cf)
{
fl->nSegment = cf.ReadShort ();
fl->nSide = cf.ReadShort ();
fl->mask = cf.ReadInt ();
fl->timer = cf.ReadFix ();
fl->delay = cf.ReadFix ();
}

//------------------------------------------------------------------------------

void LoadBackgroundBitmap (void)
{
	int pcx_error;

bmBackground.DestroyBuffer ();
pcx_error = PCXReadBitmap (gameStates.app.cheats.bJohnHeadOn ? reinterpret_cast<char*> ("johnhead.pcx") : reinterpret_cast<char*> (BACKGROUND_NAME),
									&bmBackground, BM_LINEAR,0);
if (pcx_error != PCX_ERROR_NONE)
	Error ("File %s - PCX error: %s",BACKGROUND_NAME,pcx_errormsg (pcx_error));
bmBackground.Remap (NULL, -1, -1);
}

//-----------------------------------------------------------------------------
//eof
