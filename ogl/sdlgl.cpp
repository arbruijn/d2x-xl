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

//------------------------------------------------------------------------------

#define SDL_VIDEO_FLAGS	(SDL_OPENGL | SDL_DOUBLEBUF | SDL_HWSURFACE | (ogl.m_states.bFullScreen ? SDL_FULLSCREEN : 0))

//------------------------------------------------------------------------------

static ushort gammaRamp [512];

//------------------------------------------------------------------------------

void InitGammaRamp (void)
{
	int i, j;
	ushort *pg = gammaRamp;

for (i = 256, j = 0; i; i--, j += 256, pg++)
	*pg = j;
memset (pg, 0xff, 256 * sizeof (*pg));
}

//------------------------------------------------------------------------------

int SdlGlSetBrightnessInternal (void)
{
return SDL_SetGammaRamp ((Uint16*) (gammaRamp + paletteManager.RedEffect () * 4),
	                      (Uint16*) (gammaRamp + paletteManager.GreenEffect () * 4),
	                      (Uint16*) (gammaRamp + paletteManager.BlueEffect () * 4));
}

//------------------------------------------------------------------------------

int SdlGlVideoModeOK (int w, int h)
{
int nColorBits = SDL_VideoModeOK (w, h, FindArg ("-gl_16bpp") ? 16 : 32, SDL_VIDEO_FLAGS);
PrintLog ("SDL suggests %d bits/pixel\n", nColorBits);
if (!nColorBits)
	return 0;
ogl.m_states.nColorBits = nColorBits;
return 1;
}

//------------------------------------------------------------------------------

int SdlGlSetAttribute (const char *szSwitch, const char *szAttr, SDL_GLattr attr, int value)
{
	int	i;

if (szSwitch && (i = FindArg (szSwitch)) && pszArgList [i + 1])
	attr = (SDL_GLattr) atoi (pszArgList [i + 1]);
i = SDL_GL_SetAttribute (attr, value);
/***/PrintLog ("   setting %s to %d %s\n", szAttr, value, (i == -1) ? "failed" : "succeeded");
return i;
}

//------------------------------------------------------------------------------

void SdlGlInitAttributes (void)
{
	int t;

/***/PrintLog ("setting OpenGL attributes\n");
SdlGlSetAttribute ("-gl_red", "SDL_GL_RED_SIZE", SDL_GL_RED_SIZE, 8);
SdlGlSetAttribute ("-gl_green", "SDL_GL_GREEN_SIZE", SDL_GL_GREEN_SIZE, 8);
SdlGlSetAttribute ("-gl_blue", "SDL_GL_BLUE_SIZE", SDL_GL_BLUE_SIZE, 8);
SdlGlSetAttribute ("-gl_alpha", "SDL_GL_ALPHA_SIZE", SDL_GL_ALPHA_SIZE, 8);
SdlGlSetAttribute ("-gl_buffer", "SDL_GL_BUFFER_SIZE", SDL_GL_BUFFER_SIZE, 32);
SdlGlSetAttribute ("-gl_stencil", "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
if ((t = FindArg ("-gl_depth")) && pszArgList [t+1]) {
	ogl.m_states.nDepthBits = atoi (pszArgList [t + 1]);
	if (ogl.m_states.nDepthBits <= 0)
		ogl.m_states.nDepthBits = 24;
	else if (ogl.m_states.nDepthBits > 24)
		ogl.m_states.nDepthBits = 24;
	SdlGlSetAttribute (NULL, "SDL_GL_DEPTH_SIZE", SDL_GL_DEPTH_SIZE, ogl.m_states.nDepthBits);
	SdlGlSetAttribute (NULL, "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
	}
SdlGlSetAttribute (NULL, "SDL_GL_ACCUM_RED_SIZE", SDL_GL_ACCUM_RED_SIZE, 5);
SdlGlSetAttribute (NULL, "SDL_GL_ACCUM_GREEN_SIZE", SDL_GL_ACCUM_GREEN_SIZE, 5);
SdlGlSetAttribute (NULL, "SDL_GL_ACCUM_BLUE_SIZE", SDL_GL_ACCUM_BLUE_SIZE, 5);
SdlGlSetAttribute (NULL, "SDL_GL_ACCUM_ALPHA_SIZE", SDL_GL_ACCUM_ALPHA_SIZE, 5);
SdlGlSetAttribute (NULL, "SDL_GL_DOUBLEBUFFER", SDL_GL_DOUBLEBUFFER, 1);
if (ogl.m_states.bFSAA) {
	SdlGlSetAttribute (NULL, "SDL_GL_MULTISAMPLEBUFFERS", SDL_GL_MULTISAMPLEBUFFERS, 1);
	SdlGlSetAttribute (NULL, "SDL_GL_MULTISAMPLESAMPLES", SDL_GL_MULTISAMPLESAMPLES, 4);
	}
}

//------------------------------------------------------------------------------

int SdlGlInitWindow (int w, int h, int bForce)
{
	int			bRebuild = 0;
	GLint			i;

if (ogl.m_states.bInitialized) {
	if (!bForce && (w == ogl.m_states.nCurWidth) && (h == ogl.m_states.nCurHeight) && (ogl.m_states.bCurFullScreen == ogl.m_states.bFullScreen))
		return -1;
	GrUpdate (1); // blank screen/window
	GrUpdate (1);
	if ((w != ogl.m_states.nCurWidth) || (h != ogl.m_states.nCurHeight) ||
		 (ogl.m_states.bCurFullScreen != ogl.m_states.bFullScreen)) {
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
SDL_putenv ("SDL_VIDEO_CENTERED=1");
/***/PrintLog ("setting SDL video mode (%dx%dx%d, %s)\n",
				 w, h, ogl.m_states.nColorBits, ogl.m_states.bFullScreen ? "fullscreen" : "windowed");
if (!SdlGlVideoModeOK (w, h) ||
	 !SDL_SetVideoMode (w, h, ogl.m_states.nColorBits, SDL_VIDEO_FLAGS)) {
	Error ("Could not set %dx%dx%d opengl video mode\n", w, h, ogl.m_states.nColorBits);
	return 0;
	}
#endif
const SDL_VideoInfo* viP = SDL_GetVideoInfo ();
if (viP->video_mem) {
	if (viP->video_mem < 256 * 1024 * 1024)
		gameStates.render.nMaxTextureQuality = 1;
	else if (viP->video_mem < 512 * 1024 * 1024)
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
gameStates.render.bHaveStencilBuffer = (ogl.m_states.nStencilBits > 0);
SDL_ShowCursor (0);
ogl.m_states.nCurWidth = w;
ogl.m_states.nCurHeight = h;
ogl.m_states.bCurFullScreen = ogl.m_states.bFullScreen;
if (ogl.m_states.bInitialized && bRebuild) {
	ogl.Viewport (0, 0, w, h);
	if (gameStates.app.bGameRunning) {
		paletteManager.LoadEffect ();
		ogl.RebuildContext (1);
		}
	else
		fontManager.Remap ();
	}
D2SetCaption ();
ogl.CreateDrawBuffer ();
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

void SdlGlDoFullScreenInternal (int bForce)
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
