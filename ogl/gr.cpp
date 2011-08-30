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

#include "hudmsgs.h"
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
#include "hudicons.h"
#include "rendermine.h"
#include "particles.h"
#include "glare.h"
#include "lightmap.h"
#include "menu.h"
#include "menubackground.h"
#include "addon_bitmaps.h"

#define DECLARE_VARS

void GrPaletteStepClear(void); // Function prototype for GrInit;
void ResetHoardData (void);

extern int screenShotIntervals [];

//------------------------------------------------------------------------------

CArray<CDisplayModeInfo> displayModeInfo;

//------------------------------------------------------------------------------

void GrUpdate (int bClear)
{
if (ogl.m_states.bInitialized) {
	if (ogl.m_states.nDrawBuffer == GL_FRONT)
		glFlush ();
	else
		ogl.SwapBuffers (1, bClear);
	}
}

//------------------------------------------------------------------------------

int GrVideoModeOK (u_int32_t mode)
{
return SdlGlVideoModeOK (SM_W (mode), SM_H (mode)); // platform specific code
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
/***/Printlog (1, "initializing OpenGL window\n");
if (!SdlGlInitWindow (w, h, 0))	//platform specific code
	return 0;
/***/Printlog (1, "initializing OpenGL view port\n");
ogl.Viewport (0, 0, w, h);
/***/Printlog (1, "initializing OpenGL screen mode\n");
ogl.SetScreenMode ();
ogl.GetVerInfo ();
GrUpdate (0);
return 0;
}

//------------------------------------------------------------------------------

void ResetTextures (int bReload, int bGame)
{
if (gameStates.app.bInitialized && ogl.m_states.bInitialized) {
	textureManager.Destroy (); 
	if (lightmapManager.HaveLightmaps ())
		lightmapManager.Release ();
	ogl.DestroyDepthTexture (0);
	ogl.DestroyDepthTexture (1);
	if (bReload)
		fontManager.Remap ();
	if (bGame) {
		hudIcons.Destroy ();
		//ResetHoardData ();
		particleImageManager.FreeAll ();
		UnloadAddonImages ();
		LoadAddonImages ();
		FreeStringPool ();
		OOF_ReleaseTextures ();
		ASE_ReleaseTextures ();
		if (bReload) {
			for (;;) {
				try {
					OglCacheLevelTextures ();
				}
				catch (int e) {
					if (e == EX_OUT_OF_MEMORY) {
						if (!gameOpts->render.textures.nQuality) {
							throw (e);
							}
						textureManager.Destroy ();
						UnloadTextures ();
						--gameOpts->render.textures.nQuality;
						continue;
						}
					else {
						throw (e);
						}
					}
				break;
				}
			OOF_ReloadTextures ();
			ASE_ReloadTextures ();
			}
		}
	}
}

//------------------------------------------------------------------------------

int FindDisplayMode (short w, short h)
{
	CDisplayModeInfo	dmi;
	int					i;

dmi.w = w;
dmi.h = h;
dmi.bWideScreen = float (w) / float (h) > 1.5f;

for (i = 0; i < NUM_DISPLAY_MODES - 1; i++)
	if (displayModeInfo [i] > dmi) 
		break;
return i - 1;
}

//------------------------------------------------------------------------------

int FindDisplayMode (int nScrSize)
{
return FindDisplayMode (SM_W (nScrSize), SM_H (nScrSize));
}

//------------------------------------------------------------------------------


SDL_Rect defaultDisplayModes [] = {
	{ 320,  200},
	{ 640,  400},
	{ 640,  480},
	{ 720,  576},
	{ 800,  600},
	{1024,  768},
	{1152,  864},
	{1280,  720},
	{1280,  960},
	{1280, 1024},
	{1600, 1200},
	{2048, 1526},
	{ 720,  480},
	{1280,  768},
	{1280,  800},
	{1280,  854},
	{1360,  768},
	{1400, 1050},
	{1440,  900},
	{1440,  960},
	{1680, 1050},
	{1920, 1080},
	{1920, 1200},
	{2560, 1600}
	};



void CreateDisplayModeInfo (void)
{
	SDL_Rect**	displayModes, * displayModeP;
	int			h, i;

displayModes = SDL_ListModes (NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
if (displayModes == (SDL_Rect**) -1) {
	displayModeP = defaultDisplayModes;
	h = sizeofa (defaultDisplayModes);
	}
else {
	for (h = 0; displayModes [h]; h++)
		;
	displayModeP = new SDL_Rect [h];
	for (i = 0; i < h; i++)
		displayModeP [i] = *displayModes [i];
	}
displayModeInfo.Create (h + 1);
displayModeInfo.Clear ();
for (i = 0; i < h; i++) {
	displayModeInfo [i].w = displayModeP [i].w;
	displayModeInfo [i].h = displayModeP [i].h;
	displayModeInfo [i].dim = SM (displayModeInfo [i].w, displayModeInfo [i].h);
	displayModeInfo [i].renderMethod = VR_NONE;
	displayModeInfo [i].flags = VRF_COMPATIBLE_MENUS + VRF_ALLOW_COCKPIT;
	displayModeInfo [i].bWideScreen = float (displayModeInfo [i].w) / float (displayModeInfo [i].h) >= 1.5f;
	displayModeInfo [i].bFullScreen = 1;
	displayModeInfo [i].bAvailable = 1;
	}
if (displayModes != (SDL_Rect**) -1)
	delete displayModeP;
displayModeInfo.SortAscending (0, h - 1);
}

//------------------------------------------------------------------------------

int GrInit (void)
{
	int i;

// Only do this function once!
if (gameStates.gfx.bInstalled)
	return -1;

#ifdef OGL_RUNTIME_LOAD
OglInitLoadLibrary ();
#endif
/***/Printlog (1, "initializing SDL\n");
#if !USE_IRRLICHT
if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
	PrintLog (1, "SDL library video initialisation failed: %s.\n", SDL_GetError());
	Error ("SDL library video initialisation failed: %s.", SDL_GetError());
}
#endif
#if DBG
	ogl.m_states.bFullScreen = 0;
#else
if ((i = FindArg ("-fullscreen"))) {
	/***/Printlog (1, "switching to fullscreen\n");
	ogl.m_states.bFullScreen = NumArg (i, 1);
	//ogl.ToggleFullScreen();
	}
else
	ogl.m_states.bFullScreen = 1;
#endif
/***/Printlog (1, "initializing texture manager\n");
textureManager.Init ();
/***/Printlog (1, "allocating screen buffer\n");
screen.Canvas ()->SetBuffer (NULL);

CreateDisplayModeInfo ();
// Set the mode.
for (i = 0; i < NUM_DISPLAY_MODES - 1; i++)
	if (FindArg (ScrSizeArg (displayModeInfo [i].w, displayModeInfo [i].h))) {
		gameStates.gfx.bOverride = 1;
		gameStates.gfx.nStartScrSize = displayModeInfo [i].dim;
		gameStates.gfx.nStartScrMode = i;
		break;
		}
if (!gameStates.gfx.bOverride)
#if 0//DBG
	gameStates.gfx.nStartScrMode = FindDisplayMode (gameStates.gfx.nStartScrSize = SM (800, 600));
#else
	gameStates.gfx.nStartScrSize = displayModeInfo [gameStates.gfx.nStartScrMode = MAX_DISPLAY_MODE].dim;
#endif
if ((i = GrSetMode (gameStates.gfx.nStartScrSize)))
	return i;

gameStates.gfx.bInstalled = 1;
InitGammaRamp ();
//atexit(GrClose);
/***/Printlog (1, "initializing OpenGL extensions\n");
ogl.SetRenderQuality ();
ogl.SetupExtensions ();
ogl.DestroyDrawBuffers ();
ogl.SelectDrawBuffer (0);
ogl.SetDrawBuffer (GL_BACK, 1);
return 0;
}

//------------------------------------------------------------------------------

void _CDECL_ GrClose (void)
{
PrintLog (1, "shutting down graphics subsystem\n");
SdlGlClose ();//platform specific code
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

int SetCustomDisplayMode (int w, int h)
{
if (w && h) {
		int i = CUSTOM_DISPLAY_MODE;

	displayModeInfo [i].dim = SM (w, h);
	displayModeInfo [i].w = w;
	displayModeInfo [i].h = h;
	if (!(displayModeInfo [i].bAvailable = GrVideoModeOK (displayModeInfo [i].dim))) {
		displayModeInfo [i].dim = 0;
		displayModeInfo [i].w = 0;
		displayModeInfo [i].h = 0;
		return 0;
		}
	displayModeInfo [i].renderMethod = VR_NONE;
	displayModeInfo [i].flags = VRF_COMPATIBLE_MENUS + VRF_ALLOW_COCKPIT;
	displayModeInfo [i].bWideScreen = float (displayModeInfo [i].w) / float (displayModeInfo [i].h) >= 1.5f;
	displayModeInfo [i].bFullScreen = 1;
	displayModeInfo [i].bAvailable = 1;
	}
return 1;
}

//------------------------------------------------------------------------------

int GetDisplayMode (int mode)
{
	int h, i;

for (i = 0, h = NUM_DISPLAY_MODES; i < h; i++)
	if (mode == displayModeInfo [i].dim)
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
	CDisplayModeInfo *dmiP;

if ((gameStates.video.nDisplayMode == -1) || (gameStates.render.vr.nRenderMode != VR_NONE))	//special VR nMode
	return;	//...don't change
if (!bOverride)
	gameStates.gfx.bOverride = 0;
else if (gameStates.gfx.bOverride)
	nMode = gameStates.gfx.nStartScrMode;
else
	gameStates.gfx.nStartScrMode = nMode;
if (!gameStates.menus.bHiresAvailable && (nMode != 1))
	nMode = 0;
if (!GrVideoModeOK (displayModeInfo [nMode].dim))		//can't do nMode
	nMode = 0;
gameStates.video.nDisplayMode = nMode;
dmiP = displayModeInfo + nMode;
if (gameStates.video.nDisplayMode != -1) {
	GameInitRenderBuffers (dmiP->dim, dmiP->w, dmiP->h, dmiP->renderMethod, dmiP->flags);
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
nMenuMode = gameStates.gfx.bOverride 
		? gameStates.gfx.nStartScrSize
		: gameStates.menus.bHires 
			? (gameStates.render.vr.nScreenSize >= SM (640, 480)) 
				? gameStates.render.vr.nScreenSize
				: SM (800, 600)
			: SM (320, 200);
gameStates.video.nLastScreenMode = -1;
if (nCurrentVGAMode != nMenuMode) {
	if (GrSetMode (nMenuMode))
		Error ("Cannot set screen mode for menu");
	//paletteManager.ResumeEffect (!paletteManager.EffectDisabled ());
	gameStates.menus.bInitBG = 1;
	ogl.RebuildContext (gameStates.app.bGameRunning);
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
//	Define screen pages for game mode
// If we designate through screenFlags to use paging, then do so.
screen.Canvas ()->SetupPane (&gameStates.render.vr.buffers.screenPages[0], 
										0, 0, screen.Width (), screen.Height ());

if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING) {
	screen.Canvas ()->SetupPane (&gameStates.render.vr.buffers.screenPages[1], 0, screen.Height (), screen.Width (), screen.Height ());
	}
else {
	screen.Canvas ()->SetupPane (&gameStates.render.vr.buffers.screenPages[1], 0, 0, screen.Width (), screen.Height ());
	}
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
	ogl.SetScreenMode ();
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
ogl.SetScreenMode ();
return 1;
}

//------------------------------------------------------------------------------

int GrToggleFullScreenGame (void)
{
	static char szFullscreen [2][30] = {"toggling fullscreen mode off", "toggling fullscreen mode on"};

int i = ogl.ToggleFullScreen ();
controls.FlushInput ();
if (gameStates.app.bGameRunning) {
	HUDMessage (MSGC_GAME_FEEDBACK, szFullscreen [i]);
	StopPlayerMovement ();
	}
return i;
}

//------------------------------------------------------------------------------

