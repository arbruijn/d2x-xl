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

#include "inferno.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "vers_id.h"
#include "error.h"
#include "u_mem.h"
#include "args.h"
#include "gamepal.h"
#include "oof.h"
#include "inferno.h"
#include "menu.h"
#include "screens.h"
#include "sdlgl.h"

//------------------------------------------------------------------------------

#define SDL_VIDEO_FLAGS	(SDL_OPENGL | SDL_DOUBLEBUF | SDL_HWSURFACE | (gameStates.ogl.bFullScreen ? SDL_FULLSCREEN : 0))

//------------------------------------------------------------------------------

void FreeInventoryIcons (void);

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

int OglSetBrightnessInternal (void)
{
return SDL_SetGammaRamp (gammaRamp + paletteManager.RedEffect () * 4,
	                      gammaRamp + paletteManager.GreenEffect () * 4,
	                      gammaRamp + paletteManager.BlueEffect () * 4);
}

//------------------------------------------------------------------------------

int OglVideoModeOK (int w, int h)
{
int nColorBits = SDL_VideoModeOK (w, h, FindArg ("-gl_16bpp") ? 16 : 32, SDL_VIDEO_FLAGS);
PrintLog ("SDL suggests %d bits/pixel\n", nColorBits);
if (!nColorBits)
	return 0;
gameStates.ogl.nColorBits = nColorBits;
return 1;
}

//------------------------------------------------------------------------------

int OglSetAttribute (const char *szSwitch, const char *szAttr, SDL_GLattr attr, int value)
{
	int	i;
	
if (szSwitch && (i = FindArg (szSwitch)) && pszArgList [i + 1])
	attr = (SDL_GLattr) atoi (pszArgList [i + 1]);
i = SDL_GL_SetAttribute (attr, value);
/***/PrintLog ("   setting %s to %d %s\n", szAttr, value, (i == -1) ? "failed" : "succeeded");
return i;
}

//------------------------------------------------------------------------------

void OglInitAttributes (void)
{
#if !USE_IRRLICHT
	int t;

/***/PrintLog ("setting OpenGL attributes\n");
OglSetAttribute ("-gl_red", "SDL_GL_RED_SIZE", SDL_GL_RED_SIZE, 8);
OglSetAttribute ("-gl_green", "SDL_GL_GREEN_SIZE", SDL_GL_GREEN_SIZE, 8);
OglSetAttribute ("-gl_blue", "SDL_GL_BLUE_SIZE", SDL_GL_BLUE_SIZE, 8);
OglSetAttribute ("-gl_alpha", "SDL_GL_ALPHA_SIZE", SDL_GL_ALPHA_SIZE, 8);
OglSetAttribute ("-gl_buffer", "SDL_GL_BUFFER_SIZE", SDL_GL_BUFFER_SIZE, 32);
OglSetAttribute ("-gl_stencil", "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
if ((t = FindArg ("-gl_depth")) && pszArgList [t+1]) {
	gameStates.ogl.nDepthBits = atoi (pszArgList [t + 1]);
	if (gameStates.ogl.nDepthBits <= 0)
		gameStates.ogl.nDepthBits = 24;
	else if (gameStates.ogl.nDepthBits > 24)
		gameStates.ogl.nDepthBits = 24;
	OglSetAttribute (NULL, "SDL_GL_DEPTH_SIZE", SDL_GL_DEPTH_SIZE, gameStates.ogl.nDepthBits);
	OglSetAttribute (NULL, "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
	}
OglSetAttribute (NULL, "SDL_GL_ACCUM_RED_SIZE", SDL_GL_ACCUM_RED_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_ACCUM_GREEN_SIZE", SDL_GL_ACCUM_GREEN_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_ACCUM_BLUE_SIZE", SDL_GL_ACCUM_BLUE_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_ACCUM_ALPHA_SIZE", SDL_GL_ACCUM_ALPHA_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_DOUBLEBUFFER", SDL_GL_DOUBLEBUFFER, 1);
#endif
}

//------------------------------------------------------------------------------

void OglInitState (void)
{
// select clearing (background) color 
glClearColor (0, 0, 0, 0);
glShadeModel (GL_SMOOTH);
// initialize viewing values
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();
glOrtho (0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
glMatrixMode (GL_MODELVIEW);
glLoadIdentity ();//clear matrix
glScalef (1.0f, -1.0f, 1.0f);
glTranslatef (0.0f, -1.0f, 0.0f);
glDisable (GL_CULL_FACE);
OglSetDrawBuffer (GL_BACK, 1);
glDisable (GL_SCISSOR_TEST);
glDisable (GL_ALPHA_TEST);
glDisable (GL_DEPTH_TEST);
glDisable (GL_CULL_FACE);
glDisable (GL_STENCIL_TEST);
glDisable (GL_LIGHTING);
glDisable (GL_COLOR_MATERIAL);
glDepthMask (1);
glColorMask (1,1,1,1);
if (gameStates.ogl.bAntiAliasingOk && gameStates.ogl.bAntiAliasing)
	glDisable (GL_MULTISAMPLE_ARB);
}

//------------------------------------------------------------------------------

int OglInitWindow (int w, int h, int bForce)
{
	int			bRebuild = 0;
	GLint			i;

if (gameStates.ogl.bInitialized) {
	if (!bForce && (w == gameStates.ogl.nCurWidth) && (h == gameStates.ogl.nCurHeight) && (gameStates.ogl.bCurFullScreen == gameStates.ogl.bFullScreen))
		return -1;
	if ((w != gameStates.ogl.nCurWidth) || (h != gameStates.ogl.nCurHeight) || 
		 (gameStates.ogl.bCurFullScreen != gameStates.ogl.bFullScreen)) {
		textureManager.Destroy ();//if we are or were fullscreen, changing vid mode will invalidate current textures
		bRebuild = 1;
		}
	}
if (w < 0)
	w = gameStates.ogl.nCurWidth;
if (h < 0)
	h = gameStates.ogl.nCurHeight;
if ((w < 0) || (h < 0))
	return -1;
OglInitAttributes ();
#if USE_IRRLICHT
if (!IrrInit (w, h, (bool) gameStates.ogl.bFullScreen))
	return 0;
#else
SDL_putenv (reinterpret_cast<char*> ("SDL_VIDEO_CENTERED=1"));
/***/PrintLog ("setting SDL video mode (%dx%dx%d, %s)\n",
				 w, h, gameStates.ogl.nColorBits, gameStates.ogl.bFullScreen ? "fullscreen" : "windowed");
if (!OglVideoModeOK (w, h) ||
	 !SDL_SetVideoMode (w, h, gameStates.ogl.nColorBits, SDL_VIDEO_FLAGS)) {
	Error ("Could not set %dx%dx%d opengl video mode\n", w, h, gameStates.ogl.nColorBits);
	return 0;
	}
#endif
gameStates.ogl.nColorBits = 0;
glGetIntegerv (GL_RED_BITS, &i);
gameStates.ogl.nColorBits += i;
glGetIntegerv (GL_GREEN_BITS, &i);
gameStates.ogl.nColorBits += i;
glGetIntegerv (GL_BLUE_BITS, &i);
gameStates.ogl.nColorBits += i;
glGetIntegerv (GL_ALPHA_BITS, &i);
gameStates.ogl.nColorBits += i;
glGetIntegerv (GL_DEPTH_BITS, &gameStates.ogl.nDepthBits);
glGetIntegerv (GL_STENCIL_BITS, &gameStates.ogl.nStencilBits);
gameStates.render.bHaveStencilBuffer = (gameStates.ogl.nStencilBits > 0);
SDL_ShowCursor (0);
gameStates.ogl.nCurWidth = w;
gameStates.ogl.nCurHeight = h;
gameStates.ogl.bCurFullScreen = gameStates.ogl.bFullScreen;
if (gameStates.ogl.bInitialized && bRebuild) {
	OglViewport (0, 0, w, h);
	if (gameStates.app.bGameRunning) {
		paletteManager.LoadEffect  ();
		RebuildRenderContext (1);
		}
	else
		fontManager.Remap ();
	}
D2SetCaption ();
OglCreateDrawBuffer ();
OglInitState ();
gameStates.ogl.bInitialized = 1;
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyWindow (void)
{
if (gameStates.ogl.bInitialized) {
	ResetTextures (0, 0);
#if !USE_IRRLICHT
	SDL_ShowCursor (1);
#endif
	}
}

//------------------------------------------------------------------------------

void OglDoFullScreenInternal (int bForce)
{
OglInitWindow (gameStates.ogl.nCurWidth, gameStates.ogl.nCurHeight, bForce);
}

//------------------------------------------------------------------------------

void OglSwapBuffersInternal (void)
{
#if !USE_IRRLICHT
SDL_GL_SwapBuffers ();
#endif
}

//------------------------------------------------------------------------------

void OglClose (void)
{
OglDestroyWindow ();
}

//------------------------------------------------------------------------------
