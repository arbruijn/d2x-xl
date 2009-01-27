/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_texcache.h"
#include "sdlgl.h"

#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"

#include "strutil.h"
#include "input.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "cockpit.h"
#include "render.h"
#include "particles.h"
#include "glare.h"
#include "lightmap.h"
#include "menu.h"
#include "menubackground.h"

#define DECLARE_VARS

void GrPaletteStepClear(void); // Function prototype for GrInit;
void ResetHoardData (void);

extern int screenShotIntervals [];

//------------------------------------------------------------------------------

tScrSize	scrSizes [] = {
 { 320, 200,1},
 { 640, 400,0},
 { 640, 480,0},
 { 800, 600,0},
 {1024, 768,0},
 {1152, 864,0},
 {1280, 960,0},
 {1280,1024,0},
 {1600,1200,0},
 {2048,1526,0},
 { 720, 480,0},
 {1280, 768,0},
 {1280, 800,0},
 {1280, 854,0},
 {1400,1050,0},
 {1440, 900,0},
 {1440, 960,0},
 {1680,1050,0},
   {1920,1200,0},
 {0,0,0}
};

//------------------------------------------------------------------------------

tDisplayModeInfo displayModeInfo [NUM_DISPLAY_MODES + 1] = {
 {SM ( 320,  200),  320,  200, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM ( 640,  400),  640,  400, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM ( 640,  480),  640,  480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM ( 800,  600),  800,  600, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM (1024,  768), 1024,  768, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM (1152,  864), 1152,  864, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM (1280,  960), 1280,  960, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM (1280, 1024), 1280, 1024, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM (1600, 1200), 1600, 1200, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
 {SM (2048, 1536), 2048, 1536, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
	//test>>>
 {SM (4096, 3072), 4096, 3072, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0}, 
	//<<<test
 {SM ( 720,  480),  720,  480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1280,  768), 1280,  768, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1280,  800), 1280,  800, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1280,  854), 1280,  854, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1360,  768), 1360,  768, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1400, 1050), 1400, 1050, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1440,  900), 1440,  900, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1440,  960), 1440,  960, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1680, 1050), 1680, 1050, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0}, 
 {SM (1920, 1200), 1920, 1200, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 1, 0},
	//placeholder for custom resolutions
 {              0,    0,    0, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT, 0, 0} 
	};

//------------------------------------------------------------------------------

void GrUpdate (int bClear)
{
if (gameStates.ogl.bInitialized) {
	if (gameStates.ogl.nDrawBuffer == GL_FRONT)
		glFlush ();
	else
		OglSwapBuffers (1, bClear);
	}
}

//------------------------------------------------------------------------------

int GrVideoModeOK (u_int32_t mode)
{
return OglVideoModeOK (SM_W (mode), SM_H (mode)); // platform specific code
}

//------------------------------------------------------------------------------

int GrSetMode (u_int32_t mode)
{
	uint w, h;
	//int bForce = (nCurrentVGAMode < 0);

if (mode <= 0)
	return 0;
w = SM_W (mode);
h = SM_H (mode);
nCurrentVGAMode = mode;
screen.Destroy ();
screen.Init ();
screen.SetMode (mode);
screen.SetWidth (w);
screen.SetHeight (h);
//screen.Aspect () = FixDiv(screen.Width ()*3,screen.Height ()*4);
screen.SetAspect (FixDiv (screen.Width (), (fix) (screen.Height () * ((double) w / (double) h))));
screen.Canvas ()->CBitmap::Init (BM_OGL, 0, 0, w, h, 1, NULL);
screen.Canvas ()->CreateBuffer ();
screen.Canvas ()->SetPalette (paletteManager.Default ()); //just need some valid palette here
//screen.Canvas ()->props.rowSize = screen->pitch;
//screen.Canvas ()->Buffer () = reinterpret_cast<ubyte*> (screen->pixels);
CCanvas::SetCurrent (NULL);
CCanvas::Current ()->SetFont (fontManager.Current ());
/***/PrintLog ("   initializing OpenGL window\n");
if (!OglInitWindow (w, h, 0))	//platform specific code
	return 0;
/***/PrintLog ("   initializing OpenGL view port\n");
OglViewport (0, 0, w, h);
/***/PrintLog ("   initializing OpenGL screen mode\n");
OglSetScreenMode ();
OglGetVerInfo ();
GrUpdate (0);
return 0;
}

//------------------------------------------------------------------------------

void ResetTextures (int bReload, int bGame)
{
if (gameStates.app.bInitialized && gameStates.ogl.bInitialized) {
	textureManager.Destroy (); 
	if (lightmapManager.HaveLightmaps ())
		lightmapManager.Release ();
	DestroyGlareDepthTexture ();
	if (bReload)
		fontManager.Remap ();
	if (bGame) {
		hudIcons.Destroy ();
		ResetHoardData ();
		particleImageManager.FreeAll ();
		FreeExtraImages ();
		LoadExtraImages ();
		FreeStringPool ();
		OOF_ReleaseTextures ();
		ASE_ReleaseTextures ();
		if (bReload) {
			OglCacheLevelTextures ();
			OOF_ReloadTextures ();
			ASE_ReloadTextures ();
			}
		}
	}
}

//------------------------------------------------------------------------------

int FindDisplayMode (int nScrSize)
{
	tDisplayModeInfo *dmiP = displayModeInfo;
	int w = nScrSize >> 16, h = nScrSize & 0xffff;

for (int j = sizeofa (displayModeInfo), i = 0; i < j; i++, dmiP++)
	if ((dmiP->w == w) && (dmiP->h == h))
		return i;
return -1;
}

//------------------------------------------------------------------------------

int GrInit (void)
{
	int mode = SM (640, 480);
	int retcode, t;

// Only do this function once!
if (gameStates.gfx.bInstalled)
	return -1;

#ifdef OGL_RUNTIME_LOAD
OglInitLoadLibrary ();
#endif
/***/PrintLog ("   initializing SDL\n");
#if !USE_IRRLICHT
if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
	PrintLog ("SDL library video initialisation failed: %s.\n", SDL_GetError());
	Error ("SDL library video initialisation failed: %s.", SDL_GetError());
}
#endif
if ((t = FindArg("-fullscreen"))) {
	/***/PrintLog ("   switching to fullscreen\n");
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
SetRenderQuality ();
if ((t=FindArg ("-gl_reticle")))
	gameStates.ogl.nReticle = atoi (pszArgList [t + 1]);
/***/PrintLog ("   initializing internal texture list\n");
textureManager.Init ();
/***/PrintLog ("   allocating screen buffer\n");
screen.Canvas ()->SetBuffer (NULL);

// Set the mode.
for (t = 0; scrSizes [t].x && scrSizes [t].y; t++)
	if (FindArg (ScrSizeArg (scrSizes [t].x, scrSizes [t].y))) {
		gameStates.gfx.bOverride = 1;
		gameStates.gfx.nStartScrSize =
		mode = SM (scrSizes [t].x, scrSizes [t].y);
		gameStates.gfx.nStartScrMode = FindDisplayMode (mode);
		break;
		}
if ((retcode = GrSetMode (mode)))
	return retcode;

gameStates.gfx.bInstalled = 1;
InitGammaRamp ();
//atexit(GrClose);
/***/PrintLog ("   initializing OpenGL extensions\n");
OglInitExtensions ();
OglDestroyDrawBuffer ();
OglCreateDrawBuffer ();
OglSetDrawBuffer (GL_BACK, 1);
return 0;
}

//------------------------------------------------------------------------------

void _CDECL_ GrClose (void)
{
PrintLog ("shutting down graphics subsystem\n");
OglClose();//platform specific code
screen.Destroy ();
#ifdef OGL_RUNTIME_LOAD
if (ogl_rt_loaded)
	OpenGL_LoadLibrary(false);
#endif
}

//------------------------------------------------------------------------------

char *ScrSizeArg (int x, int y)
{
	static	char	szScrMode [20];

sprintf (szScrMode, "-%dx%d", x, y);
return szScrMode;
}

//------------------------------------------------------------------------------

int SCREENMODE (int x, int y, int c)
{
	int i = FindArg (ScrSizeArg (x, y));

if (i && (i < nArgCount)) {
	gameStates.gfx.bOverride = 1; 
	gameData.render.window.w = x;
	gameData.render.window.h = y;
	return gameStates.gfx.nStartScrSize = GetDisplayMode (SM (x, y)); 
	}
return -1;
}

//------------------------------------------------------------------------------

int S_MODE (u_int32_t *VV, int *VG)
{
	int	h, i;

for (i = 0; scrSizes [i].x && scrSizes [i].y; i++)
	if ((h = FindArg (ScrSizeArg (scrSizes [i].x, scrSizes [i].y))) && (h < nArgCount)) {
		*VV = SM (scrSizes [i].x, scrSizes [i].y);
		*VG = 1; //always 1 in d2x-xl
		return h;
		}
return -1;
}

//------------------------------------------------------------------------------

int GrCheckFullScreen (void)
{
return gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

void GrDoFullScreen (int f)
{
if (gameStates.ogl.bVoodooHack)
	gameStates.ogl.bFullScreen = 1;//force fullscreen mode on voodoos.
else
	gameStates.ogl.bFullScreen = f;
if (gameStates.ogl.bInitialized)
	OglDoFullScreenInternal (0);
}

//------------------------------------------------------------------------------

int SetCustomDisplayMode (int w, int h)
{
displayModeInfo [NUM_DISPLAY_MODES].VGA_mode = SM (w, h);
displayModeInfo [NUM_DISPLAY_MODES].w = w;
displayModeInfo [NUM_DISPLAY_MODES].h = h;
if (!(displayModeInfo [NUM_DISPLAY_MODES].isAvailable = 
	   GrVideoModeOK (displayModeInfo [NUM_DISPLAY_MODES].VGA_mode)))
	return 0;
SetDisplayMode (NUM_DISPLAY_MODES, 0);
return 1;
}

//------------------------------------------------------------------------------

int GetDisplayMode (int mode)
{
	int h, i;

for (i = 0, h = NUM_DISPLAY_MODES; i < h; i++)
	if (mode == displayModeInfo [i].VGA_mode)
		return i;
return -1;
}

//------------------------------------------------------------------------------

#if VR_NONE
#   undef VR_NONE			//undef if != 0
#endif
#ifndef VR_NONE
#   define VR_NONE 0		//make sure VR_NONE is defined and 0 here
#endif


void SetDisplayMode (int nMode, int bOverride)
{
	tDisplayModeInfo *dmiP;

if ((gameStates.video.nDisplayMode == -1) || (gameStates.render.vr.nRenderMode != VR_NONE))	//special VR nMode
	return;	//...don't change
if (bOverride && gameStates.gfx.bOverride) {
	gameStates.gfx.nStartScrMode = nMode;
	}
else
	gameStates.gfx.bOverride = 0;
if (!gameStates.menus.bHiresAvailable && (nMode != 1))
	nMode = 0;
if (!GrVideoModeOK (displayModeInfo [nMode].VGA_mode))		//can't do nMode
	nMode = 0;
gameStates.video.nDisplayMode = nMode;
dmiP = displayModeInfo + nMode;
if (gameStates.video.nDisplayMode != -1) {
	GameInitRenderBuffers (dmiP->VGA_mode, dmiP->w, dmiP->h, dmiP->render_method, dmiP->flags);
	gameStates.video.nDefaultDisplayMode = gameStates.video.nDisplayMode;
	}
gameStates.video.nScreenMode = -1;		//force screen reset
}

//------------------------------------------------------------------------------

//called to get the screen in a mode compatible with popup menus.
//if we can't have popups over the game screen, switch to menu mode.

void SetPopupScreenMode (void)
{
if (!gameOpts->menus.nStyle)
	SetScreenMode (SCREEN_MENU);		//must switch to menu mode
}

//------------------------------------------------------------------------------

int SetMenuScreenMode (u_int32_t sm)
{
	u_int32_t nMenuMode;

gameStates.menus.bHires = gameStates.menus.bHiresAvailable;		//do highres if we can
nMenuMode = 
	gameStates.gfx.bOverride ?
		gameStates.gfx.nStartScrSize
		: gameStates.menus.bHires ?
			 (gameStates.render.vr.nScreenSize >= SM (640,480)) ?
				gameStates.render.vr.nScreenSize
				: SM (640,480)
			: SM (320,200);
gameStates.video.nLastScreenMode = -1;
if (nCurrentVGAMode != nMenuMode) {
	if (GrSetMode (nMenuMode))
		Error ("Cannot set screen mode for menu");
	if (!paletteManager.EffectDisabled ())
		paletteManager.LoadEffect ();
	gameStates.menus.bInitBG = 1;
	RebuildRenderContext (gameStates.app.bGameRunning);
	}

screen.Canvas ()->SetupPane (
	gameStates.render.vr.buffers.screenPages, 0, 0, 
	screen.Width (), screen.Height ());
screen.Canvas ()->SetupPane (
	gameStates.render.vr.buffers.screenPages + 1, 0, 0, 
	screen.Width (), screen.Height ());
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && gameStates.menus.bHires;
return 1;
}

//------------------------------------------------------------------------------

int SetGameScreenMode (u_int32_t sm)
{
if (nCurrentVGAMode != gameStates.render.vr.nScreenSize) {
	if (GrSetMode (gameStates.render.vr.nScreenSize)) {
		Error ("Cannot set desired screen mode for game!");
		//we probably should do something else here, like select a standard mode
		}
#ifdef MACINTOSH
	if ((gameConfig.nControlType == 1) && (gameStates.app.nFunctionMode == FMODE_GAME))
		JoyDefsCalibrate ();
#endif
	}
gameData.render.window.wMax = screen.Width ();
gameData.render.window.hMax = screen.Height ();
if (!gameData.render.window.h || (gameData.render.window.h > gameData.render.window.hMax) || 
	 !gameData.render.window.w || (gameData.render.window.w > gameData.render.window.wMax)) {
	gameData.render.window.w = gameData.render.window.wMax;
	gameData.render.window.h = gameData.render.window.hMax;
	}
cockpit->Init () ();
//	Define screen pages for game mode
// If we designate through screenFlags to use paging, then do so.
screen.Canvas ()->SetupPane (&gameStates.render.vr.buffers.screenPages[0], 
										0, 0, screen.Width (), screen.Height ());

if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING) {
	screen.Canvas ()->SetupPane (&gameStates.render.vr.buffers.screenPages[1], 0, 
											screen.Height (), screen.Width (), screen.Height ());
	}
else {
	screen.Canvas ()->SetupPane (&gameStates.render.vr.buffers.screenPages[1], 
											0, 0, screen.Width (), screen.Height ());
	}
cockpit->Init () ();
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.menus.bHires = (gameStates.video.nDisplayMode > 1));
if (gameStates.render.vr.nRenderMode != VR_NONE) {
	// for 640x480 or higher, use hires font.
	if (gameStates.render.fonts.bHiresAvailable && (screen.Height () > 400))
		gameStates.render.fonts.bHires = 1;
	else
		gameStates.render.fonts.bHires = 0;
	}
console.Resize (0, 0, screen.Width (), screen.Height () / 2);
return 1;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int SetScreenMode (u_int32_t sm)
{
#if 0
	GLint nError = glGetError ();
#endif
if ((gameStates.video.nScreenMode == sm) && (nCurrentVGAMode == gameStates.render.vr.nScreenSize) && 
		(screen.Mode () == gameStates.render.vr.nScreenSize)) {
	CCanvas::SetCurrent (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
	OglSetScreenMode ();
	return 1;
	}
	gameStates.video.nScreenMode = sm;
	switch (gameStates.video.nScreenMode) {
		case SCREEN_MENU:
			SetMenuScreenMode (sm);
			break;

	case SCREEN_GAME:
		SetGameScreenMode (sm);
		break;


	default:
		Error ("Invalid screen mode %d",sm);
	}
gameStates.render.vr.nCurrentPage = 0;
CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
OglSetScreenMode ();
return 1;
}

//------------------------------------------------------------------------------

int GrToggleFullScreen (void)
{
GrDoFullScreen (!gameStates.ogl.bFullScreen);
return gameStates.ogl.bFullScreen;
}

//------------------------------------------------------------------------------

int GrToggleFullScreenGame (void)
{
	static char szFullscreen [2][30] = {"toggling fullscreen mode off", "toggling fullscreen mode on"};

int i = GrToggleFullScreen ();
FlushInput ();
if (gameStates.app.bGameRunning) {
	HUDMessage (MSGC_GAME_FEEDBACK, szFullscreen [i]);
	StopPlayerMovement ();
	}
return i;
}

//------------------------------------------------------------------------------

