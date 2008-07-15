/* $Id: gr.c,v 1.16 2003/11/27 04:59:49 btb Exp $ */
/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "inferno.h"

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
#include "gauges.h"
#include "render.h"
#include "particles.h"
#include "glare.h"
#include "lightmap.h"
#include "newmenu.h"

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
	if ((gameStates.ogl.nDrawBuffer == GL_FRONT) || (!gameOpts->menus.nStyle && gameStates.menus.nInMenu))
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
	unsigned int w, h;
	unsigned char *gr_bm_data;
	//int bForce = (nCurrentVGAMode < 0);

if (mode <= 0)
	return 0;
w = SM_W (mode);
h = SM_H (mode);
nCurrentVGAMode = mode;
gr_bm_data = grdCurScreen->scCanvas.cvBitmap.bmTexBuf;//since we use realloc, we want to keep this pointer around.
memset (grdCurScreen, 0, sizeof(grsScreen));
grdCurScreen->scMode = mode;
grdCurScreen->scWidth = w;
grdCurScreen->scHeight = h;
//grdCurScreen->scAspect = FixDiv(grdCurScreen->scWidth*3,grdCurScreen->scHeight*4);
grdCurScreen->scAspect = FixDiv (grdCurScreen->scWidth, (fix) (grdCurScreen->scHeight * ((double) w / (double) h)));
grdCurScreen->scCanvas.cvBitmap.bmProps.x = 0;
grdCurScreen->scCanvas.cvBitmap.bmProps.y = 0;
grdCurScreen->scCanvas.cvBitmap.bmProps.w = w;
grdCurScreen->scCanvas.cvBitmap.bmProps.h = h;
grdCurScreen->scCanvas.cvBitmap.bmBPP = 1;
grdCurScreen->scCanvas.cvBitmap.bmPalette = defaultPalette; //just need some valid palette here
//grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = screen->pitch;
grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = w;
grdCurScreen->scCanvas.cvBitmap.bmProps.nType = BM_OGL;
//grdCurScreen->scCanvas.cvBitmap.bmTexBuf = (unsigned char *)screen->pixels;
grdCurScreen->scCanvas.cvBitmap.bmTexBuf = (ubyte *) D2_REALLOC (gr_bm_data, w * h);
GrSetCurrentCanvas (NULL);
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

void GrRemapMonoFonts ();

void ResetTextures (int bReload, int bGame)
{
if (gameStates.app.bInitialized && gameStates.ogl.bInitialized) {
	OglSmashTextureListInternal (); 
	if (HaveLightmaps ())
		OglDestroyLightmaps ();
	DestroyGlareDepthTexture ();
	NMFreeAltBg (1);
	if (bReload)
		GrRemapMonoFonts ();
	if (bGame) {
		FreeInventoryIcons ();
		FreeObjTallyIcons ();
		ResetHoardData ();
		FreeParticleImages ();
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

int GrInit (void)
{
	int mode = SM (640, 480);
	int retcode, t;

// Only do this function once!
if (gameStates.gfx.bInstalled)
	return -1;

#ifdef OGL_RUNTIME_LOAD
OglInitLoadLibrary();
#endif
/***/PrintLog ("   initializing SDL\n");
#if !USE_IRRLICHT
if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
	PrintLog ("SDL library video initialisation failed: %s.\n", SDL_GetError());
	Error ("SDL library video initialisation failed: %s.", SDL_GetError());
}
#endif
if ((t = FindArg ("-gl_voodoo"))) {
	gameStates.ogl.bVoodooHack = 
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
if ((t = FindArg("-fullscreen"))) {
	/***/PrintLog ("   switching to fullscreen\n");
	gameStates.ogl.bFullScreen = NumArg (t, 1);
	//GrToggleFullScreen();
	}
SetRenderQuality ();
if ((t=FindArg ("-gl_vidmem"))){
	nOglMemTarget=atoi(pszArgList[t+1])*1024*1024;
}
if ((t=FindArg ("-gl_reticle"))){
	gameStates.ogl.nReticle=atoi(pszArgList[t+1]);
}
/***/PrintLog ("   initializing internal texture list\n");
OglInitTextureListInternal();
/***/PrintLog ("   allocating screen buffer\n");
MALLOC (grdCurScreen, grsScreen, 1);
memset (grdCurScreen, 0, sizeof (grsScreen));
grdCurScreen->scCanvas.cvBitmap.bmTexBuf = NULL;

// Set the mode.
for (t = 0; scrSizes [t].x && scrSizes [t].y; t++)
	if (FindArg (ScrSizeArg (scrSizes [t].x, scrSizes [t].y))) {
		gameStates.gfx.bOverride = 1;
		gameStates.gfx.nStartScrSize = t;
		gameStates.gfx.nStartScrMode =
		mode = SM (scrSizes [t].x, scrSizes [t].y);
		break;
		}
if ((retcode = GrSetMode (mode)))
	return retcode;

grdCurScreen->scCanvas.cvColor.index = 0;
grdCurScreen->scCanvas.cvColor.rgb = 0;
grdCurScreen->scCanvas.cvDrawMode = 0;
grdCurScreen->scCanvas.cvFont = NULL;
grdCurScreen->scCanvas.cvFontFgColor.index = 0;
grdCurScreen->scCanvas.cvFontBgColor.index = 0;
grdCurScreen->scCanvas.cvFontFgColor.rgb = 0;
grdCurScreen->scCanvas.cvFontBgColor.rgb = 0;
GrSetCurrentCanvas (&grdCurScreen->scCanvas);

gameStates.gfx.bInstalled = 1;
InitGammaRamp ();
//atexit(GrClose);
/***/PrintLog ("   initializing OpenGL extensions\n");
OglInitExtensions ();
OglDestroyDrawBuffer ();
OglCreateDrawBuffer ();
OglDrawBuffer (GL_BACK, 1);
return 0;
}

//------------------------------------------------------------------------------

void _CDECL_ GrClose (void)
{
PrintLog ("shutting down graphics subsystem\n");
OglClose();//platform specific code
if (grdCurScreen) {
	if (grdCurScreen->scCanvas.cvBitmap.bmTexBuf)
		D2_FREE (grdCurScreen->scCanvas.cvBitmap.bmTexBuf);
	D2_FREE (grdCurScreen);
	}
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
	return gameStates.gfx.nStartScrMode = GetDisplayMode (SM (x, y)); 
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
	tDisplayModeInfo *tDisplayModeInfo;

if ((gameStates.video.nDisplayMode == -1) || (gameStates.render.vr.nRenderMode != VR_NONE))	//special VR nMode
	return;								//...don't change
if (bOverride && gameStates.gfx.bOverride)
	nMode = gameStates.gfx.nStartScrSize;
else
	gameStates.gfx.bOverride = 0;
if (!gameStates.menus.bHiresAvailable && (nMode != 1))
	nMode = 0;
if (!GrVideoModeOK (displayModeInfo [nMode].VGA_mode))		//can't do nMode
	nMode = 0;
gameStates.video.nDisplayMode = nMode;
tDisplayModeInfo = displayModeInfo + nMode;
if (gameStates.video.nDisplayMode != -1) {
	GameInitRenderBuffers (tDisplayModeInfo->VGA_mode, tDisplayModeInfo->w, tDisplayModeInfo->h, tDisplayModeInfo->render_method, tDisplayModeInfo->flags);
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
return 1;
}

//------------------------------------------------------------------------------

int SetGameScreenMode (u_int32_t sm)
{
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
gameData.render.window.wMax = grdCurScreen->scWidth;
gameData.render.window.hMax = grdCurScreen->scHeight;
if (!gameData.render.window.h || (gameData.render.window.h > gameData.render.window.hMax) || 
	 !gameData.render.window.w || (gameData.render.window.w > gameData.render.window.wMax)) {
	gameData.render.window.w = gameData.render.window.wMax;
	gameData.render.window.h = gameData.render.window.hMax;
	}
InitCockpit ();
//	Define screen pages for game mode
// If we designate through screenFlags to use paging, then do so.
GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[0], &grdCurScreen->scCanvas, 
					  0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);

if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING) {
	GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[1], &grdCurScreen->scCanvas, 0, 
						  grdCurScreen->scHeight, grdCurScreen->scWidth, grdCurScreen->scHeight);
	}
else {
	GrInitSubCanvas (&gameStates.render.vr.buffers.screenPages[1], &grdCurScreen->scCanvas, 
						  0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight);
	}
InitCockpit ();
gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.menus.bHires = (gameStates.video.nDisplayMode > 1));
if (gameStates.render.vr.nRenderMode != VR_NONE) {
	// for 640x480 or higher, use hires font.
	if (gameStates.render.fonts.bHiresAvailable && (grdCurScreen->scHeight > 400))
		gameStates.render.fonts.bHires = 1;
	else
		gameStates.render.fonts.bHires = 0;
	}
con_resize ();
return 1;
}

//------------------------------------------------------------------------------

#ifdef EDITOR

int SetEditorScreenMode (u_int32_t sm)
{
if (grdCurScreen->scMode != SM (800,600))	{
	int gr_error = GrSetMode (SM (800,600);
	if (gr_error { //force into game scrren
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
return 1;
}

#endif

//------------------------------------------------------------------------------
//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int SetScreenMode (u_int32_t sm)
{
#if 0
	GLint nError = glGetError ();
#endif
#ifdef EDITOR
if ((sm == SCREEN_MENU) && (gameStates.video.nScreenMode == SCREEN_EDITOR)) {
	GrSetCurrentCanvas (Canv_editor);
	return 1;
	}
#endif
if ((gameStates.video.nScreenMode == sm) && (nCurrentVGAMode == gameStates.render.vr.nScreenMode) && 
		(grdCurScreen->scMode == gameStates.render.vr.nScreenMode)) {
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
			SetMenuScreenMode (sm);
			break;

	case SCREEN_GAME:
		SetGameScreenMode (sm);
		break;

#ifdef EDITOR
	case SCREEN_EDITOR:
		SetEditorScreenMode (sm);
		break;
#endif

	default:
		Error ("Invalid screen mode %d",sm);
	}
gameStates.render.vr.nCurrentPage = 0;
GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages[gameStates.render.vr.nCurrentPage]);
if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING)
	GrShowCanvas (&gameStates.render.vr.buffers.screenPages[gameStates.render.vr.nCurrentPage]);
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
int i = GrToggleFullScreen ();
FlushInput ();
if (gameStates.app.bGameRunning) {
	HUDMessage (MSGC_GAME_FEEDBACK, i ? (char *) "toggling fullscreen mode on" : (char *) "toggling fullscreen mode off");
	StopPlayerMovement ();
	}
return i;
}

//------------------------------------------------------------------------------

