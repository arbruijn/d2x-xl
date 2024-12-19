/*
 *
 * Graphics functions for SDL-GL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef __macosx__
# include <SDL/SDL.h>
# ifdef SDL_IMAGE
#  include <SDL/SDL_image.h>
# endif
#else
# include <SDL.h>
# ifdef SDL_IMAGE
#  include <SDL_image.h>
# endif
#endif

#include "descent.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "vers_id.h"
#include "error.h"
#include "u_mem.h"
#include "args.h"
#include "gamepal.h"
#include "oof.h"
#include "descent.h"
#include "menu.h"
#include "screens.h"
#include "sdlgl.h"
#include "config.h"

//------------------------------------------------------------------------------

#define SDL_VIDEO_FLAGS	(SDL_OPENGL | SDL_DOUBLEBUF | SDL_HWSURFACE | \
	(gameConfig.bBorderless ? SDL_NOFRAME : ogl.m_states.bFullScreen ? SDL_FULLSCREEN : 0))

//#define SDL_VIDEO_FLAGS	(SDL_OPENGL)


//------------------------------------------------------------------------------

static uint16_t gammaRamp [512];

//------------------------------------------------------------------------------

void InitGammaRamp (void)
{
	int32_t i, j;
	uint16_t *pg = gammaRamp;

for (i = 256, j = 0; i; i--, j += 256, pg++)
	*pg = j;
memset (pg, 0xff, 256 * sizeof (*pg));
}

//------------------------------------------------------------------------------

int32_t SdlGlSetBrightnessInternal (void)
{
return SDL_SetGammaRamp ((Uint16*) (gammaRamp + paletteManager.RedEffect () * 4),
	                      (Uint16*) (gammaRamp + paletteManager.GreenEffect () * 4),
	                      (Uint16*) (gammaRamp + paletteManager.BlueEffect () * 4));
}

//------------------------------------------------------------------------------

int32_t SdlGlVideoModeOK (int32_t w, int32_t h)
{
/* Information about the current video settings. */
const SDL_VideoInfo* info = NULL;
PrintLog (1, "checking video mode (%d X %d)\n", w, h);
info = SDL_GetVideoInfo( );
int32_t nColorBits = SDL_VideoModeOK (w, h, FindArg ("-gl_16bpp") ? 16 : info->vfmt->BitsPerPixel, SDL_VIDEO_FLAGS);

PrintLog (0, "SDL suggests %d bits/pixel\n", nColorBits);
if (!nColorBits) {
	PrintLog (-1);
	return 0;
	}
ogl.m_states.nColorBits = nColorBits;
PrintLog (-1);
return 1;
}

//------------------------------------------------------------------------------

int32_t SdlGlSetAttribute (const char *szSwitch, const char *szAttr, SDL_GLattr attr, int32_t value)
{
	int32_t	i;

if (szSwitch && (i = FindArg (szSwitch)) && appConfig [i + 1])
	attr = (SDL_GLattr) atoi (appConfig [i + 1]);
i = SDL_GL_SetAttribute (attr, value);
/***/PrintLog (0, "setting %s to %d %s\n", szAttr, value, (i == -1) ? "failed" : "succeeded");
return i;
}

//------------------------------------------------------------------------------

void SdlGlInitAttributes (void)
{
	int32_t t;

/***/PrintLog (1, "setting OpenGL attributes\n");
SdlGlSetAttribute ("-gl_red", "SDL_GL_RED_SIZE", SDL_GL_RED_SIZE, 8);
SdlGlSetAttribute ("-gl_green", "SDL_GL_GREEN_SIZE", SDL_GL_GREEN_SIZE, 8);
SdlGlSetAttribute ("-gl_blue", "SDL_GL_BLUE_SIZE", SDL_GL_BLUE_SIZE, 8);
SdlGlSetAttribute ("-gl_alpha", "SDL_GL_ALPHA_SIZE", SDL_GL_ALPHA_SIZE, 8);
SdlGlSetAttribute ("-gl_buffer", "SDL_GL_BUFFER_SIZE", SDL_GL_BUFFER_SIZE, 32);
SdlGlSetAttribute ("-gl_stencil", "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
if (0 < (t = FindArg ("-gl_depth")) && appConfig [t+1]) {
	ogl.m_states.nDepthBits = atoi (appConfig [t + 1]);
	if (ogl.m_states.nDepthBits <= 0)
		ogl.m_states.nDepthBits = 24;
	else if (ogl.m_states.nDepthBits > 24)
		ogl.m_states.nDepthBits = 24;
	SdlGlSetAttribute (NULL, "SDL_GL_DEPTH_SIZE", SDL_GL_DEPTH_SIZE, ogl.m_states.nDepthBits);
	SdlGlSetAttribute (NULL, "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
	}

SdlGlSetAttribute (NULL, "SDL_GL_DOUBLEBUFFER", SDL_GL_DOUBLEBUFFER, 1);
if (ogl.m_features.bQuadBuffers/*.Apply ()*/)
	SdlGlSetAttribute (NULL, "SDL_GL_STEREO", SDL_GL_STEREO, 1);
if (ogl.m_states.bFSAA) {
	SdlGlSetAttribute (NULL, "SDL_GL_MULTISAMPLEBUFFERS", SDL_GL_MULTISAMPLEBUFFERS, 1);
	SdlGlSetAttribute (NULL, "SDL_GL_MULTISAMPLESAMPLES", SDL_GL_MULTISAMPLESAMPLES, 4);
}
PrintLog (-1);
}

//------------------------------------------------------------------------------

int32_t SdlGlInitWindow (int32_t w, int32_t h, int32_t bForce)
{
	int32_t			bRebuild = 0;
	GLint			i;

if (ogl.m_states.bInitialized) {
	if (!bForce && (w == ogl.m_states.nCurWidth) && (h == ogl.m_states.nCurHeight) &&
		(ogl.m_states.bCurFullScreen == ogl.m_states.bFullScreen) &&
		(ogl.m_states.bCurBorderless == gameConfig.bBorderless))
		return -1;
	ogl.Update (1); // blank screen/window
	ogl.Update (1);
	if ((w != ogl.m_states.nCurWidth) || (h != ogl.m_states.nCurHeight) ||
		 (ogl.m_states.bCurFullScreen != ogl.m_states.bFullScreen) ||
		 (ogl.m_states.bCurBorderless != gameConfig.bBorderless)) {
		textureManager.Destroy ();//if we are or were fullscreen, changing vid mode will invalidate current textures
		bRebuild = 1;
		}
	}
if (w < 0)
	w = ogl.m_states.nCurWidth;
if (h < 0)
	h = ogl.m_states.nCurHeight;
if ((w < 0) || (h < 0))
	return -1;
SdlGlInitAttributes ();
#if USE_IRRLICHT
if (!IrrInit (w, h, (bool) ogl.m_states.bFullScreen))
	return 0;
#else
SDL_putenv (const_cast<char*>("SDL_VIDEO_CENTERED=1"));
/***/PrintLog (1, "setting SDL video mode (%dx%dx%d, %s)\n", w, h, ogl.m_states.nColorBits, ogl.m_states.bFullScreen ? "fullscreen" : "windowed");
if (!SdlGlVideoModeOK (w, h) ||
	 !SDL_SetVideoMode (w, h, ogl.m_states.nColorBits, SDL_VIDEO_FLAGS)) {
	PrintLog(-1);
	PrintLog(1, "SDL error: %s\n", SDL_GetError());
	PrintLog (-1);
	Error ("Could not set %dx%dx%d opengl video mode\n", w, h, ogl.m_states.nColorBits);
	return 0;
	}
PrintLog (-1);
#endif
const SDL_VideoInfo* pVideoInfo = SDL_GetVideoInfo ();
if (pVideoInfo->video_mem) {
	if (pVideoInfo->video_mem < 256 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 1;
	else if (pVideoInfo->video_mem < 512 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 2;
	}
ogl.m_states.nColorBits = 0;
glGetIntegerv (GL_RED_BITS, &i);
ogl.m_states.nColorBits += i;
glGetIntegerv (GL_GREEN_BITS, &i);
ogl.m_states.nColorBits += i;
glGetIntegerv (GL_BLUE_BITS, &i);
ogl.m_states.nColorBits += i;
glGetIntegerv (GL_ALPHA_BITS, &i);
ogl.m_states.nColorBits += i;
glGetIntegerv (GL_DEPTH_BITS, &ogl.m_states.nDepthBits);
glGetIntegerv (GL_STENCIL_BITS, &ogl.m_states.nStencilBits);
ogl.m_features.bStencilBuffer = (ogl.m_states.nStencilBits > 0);
if (!ogl.m_features.bQuadBuffers/*.Apply ()*/)
	ogl.m_states.nStereo = 0;
else {
	glGetIntegerv (GL_STEREO, &ogl.m_states.nStereo);
	ogl.m_features.bStereoBuffers = (ogl.m_states.nStereo > 0);
	}
SDL_ShowCursor (0);
ogl.m_states.nCurWidth = w;
ogl.m_states.nCurHeight = h;
ogl.m_states.bCurFullScreen = ogl.m_states.bFullScreen;
ogl.m_states.bCurBorderless = gameConfig.bBorderless;
if (ogl.m_states.bInitialized && bRebuild) {
	ogl.SetViewport (0, 0, w, h);
	if (gameStates.app.bGameRunning) {
		//paletteManager.ResumeEffect ();
		ogl.RebuildContext (1);
		}
	else
		fontManager.Remap ();
	}
D2SetCaption ();
ogl.SelectDrawBuffer (0);
ogl.InitState ();
ogl.m_states.bInitialized = 1;
return 1;
}

//------------------------------------------------------------------------------

void SdlGlDestroyWindow (void)
{
if (ogl.m_states.bInitialized) {
	ResetTextures (0, 0);
#if !USE_IRRLICHT
	SDL_ShowCursor (1);
#endif
	}
}

//------------------------------------------------------------------------------

void SdlGlDoFullScreenInternal (int32_t bForce)
{
SdlGlInitWindow (ogl.m_states.nCurWidth, ogl.m_states.nCurHeight, bForce);
}

//------------------------------------------------------------------------------

void SdlGlSwapBuffersInternal (void)
{
#if !USE_IRRLICHT
SDL_GL_SwapBuffers ();
#endif
}

//------------------------------------------------------------------------------

void SdlGlClose (void)
{
SdlGlDestroyWindow ();
}

//------------------------------------------------------------------------------
