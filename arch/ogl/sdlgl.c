/* $Id: sdlgl.c,v 1.9 2003/11/27 04:59:49 btb Exp $ */
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

#include "ogl_init.h"
#include "vers_id.h"
#include "error.h"
#include "u_mem.h"
#include "args.h"
#include "gamepal.h"
#include "oof.h"
#include "inferno.h"
#include "menu.h"

#define SDL_VIDEO_FLAGS	(SDL_OPENGL | SDL_DOUBLEBUF | SDL_HWSURFACE | (gameStates.ogl.bFullScreen ? SDL_FULLSCREEN : 0))

static int nCurWidth = -1, nCurHeight = -1, bCurFullScr = -1;
static Uint16 gammaRamp[512];

void GrRemapMonoFonts (void);
void FreeInventoryIcons (void);

//------------------------------------------------------------------------------

#include "screens.h"

void OglDoFullScreenInternal (int bForce)
{
OglInitWindow (nCurWidth, nCurHeight, bForce);
}

//------------------------------------------------------------------------------

void InitGammaRamp (void)
{
	int i, j;
	Uint16 *pg = gammaRamp;

for (i = 256, j = 0; i; i--, j += 256, pg++)
	*pg = j;
memset (pg, 0xff, 256 * sizeof (*pg));
}

//------------------------------------------------------------------------------

int OglSetBrightnessInternal (void)
{
return SDL_SetGammaRamp (gammaRamp + gameStates.ogl.bright.red * 4,
	                      gammaRamp + gameStates.ogl.bright.green * 4,
	                      gammaRamp + gameStates.ogl.bright.blue * 4);
}

//------------------------------------------------------------------------------

void OglSwapBuffersInternal (void)
{
SDL_GL_SwapBuffers ();
}

//------------------------------------------------------------------------------

int OglVideoModeOK (int w, int h)
{
gameStates.ogl.nColorBits = 
	SDL_VideoModeOK (w, h, FindArg ("-gl_16bpp") ? 16 : 32, SDL_VIDEO_FLAGS);
LogErr ("SDL suggests %d bits/pixel\n", gameStates.ogl.nColorBits);
return gameStates.ogl.nColorBits != 0;
}

//------------------------------------------------------------------------------

int OglInitWindow (int w, int h, int bForce)
{
	u_int32_t	sm = gameStates.video.nScreenMode;
	int			bRebuild = 0;
	GLint			i;

	static int	bAllowToggleFullScreen = 1;

if (gameStates.ogl.bInitialized) {
	if (!bForce && (w == nCurWidth) && (h == nCurHeight) && (bCurFullScr == gameStates.ogl.bFullScreen))
		return -1;
	if ((w != nCurWidth) || (h != nCurHeight) || (bCurFullScr != gameStates.ogl.bFullScreen)) {
		OglSmashTextureListInternal ();//if we are or were fullscreen, changing vid mode will invalidate current textures
		bRebuild = 1;
		}
	}
if (w < 0)
	w = nCurWidth;
if (h < 0)
	h = nCurHeight;
if ((w < 0) || (h < 0))
	return -1;
D2SetCaption ();
OglInitAttributes ();
/***/LogErr ("setting SDL video mode (%dx%dx%d, %s)\n",
				 w, h, gameStates.ogl.nColorBits, gameStates.ogl.bFullScreen ? "fullscreen" : "windowed");
if (!OglVideoModeOK (w, h) ||
	 !SDL_SetVideoMode (w, h, gameStates.ogl.nColorBits, SDL_VIDEO_FLAGS)) {
	Error ("Could not set %dx%dx%d opengl video mode\n", w, h, gameStates.ogl.nColorBits);
	return 0;
	}
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
nCurWidth = w;
nCurHeight = h;
bCurFullScr = gameStates.ogl.bFullScreen;
if (gameStates.ogl.bInitialized && bRebuild) {
	glViewport (0, 0, w, h);
	if (gameStates.app.bGameRunning) {
		GrPaletteStepLoad (NULL);
		RebuildGfxFx (1, 1);
		}
	else
		GrRemapMonoFonts ();
	//FreeInventoryIcons ();
	}
gameStates.ogl.bInitialized = 1;
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyWindow (void)
{
if (gameStates.ogl.bInitialized) {
	ResetTextures (0, 0);
	SDL_ShowCursor (1);
		//gameStates.ogl.bInitialized=0;
		//well..SDL doesn't really let you kill the window.. so we just need to wait for sdl_quit
	}
return;
}

//------------------------------------------------------------------------------

int OglSetAttribute (char *szSwitch, char *szAttr, SDL_GLattr attr, int value)
{
	int	i;
		
if (szSwitch && (i = FindArg (szSwitch)) && Args [i + 1])
	attr = atoi (Args [i + 1]);
i = SDL_GL_SetAttribute (attr, value);
/***/LogErr ("   setting %s to %d %s\n", szAttr, value, (i == -1) ? "failed" : "succeeded");
return i;
}

//------------------------------------------------------------------------------

void OglInitAttributes (void)
{
	int t;

/***/LogErr ("setting OpenGL attributes\n");
OglSetAttribute ("-gl_red", "SDL_GL_RED_SIZE", SDL_GL_RED_SIZE, 8);
OglSetAttribute ("-gl_green", "SDL_GL_GREEN_SIZE", SDL_GL_GREEN_SIZE, 8);
OglSetAttribute ("-gl_blue", "SDL_GL_BLUE_SIZE", SDL_GL_BLUE_SIZE, 8);
OglSetAttribute ("-gl_alpha", "SDL_GL_ALPHA_SIZE", SDL_GL_ALPHA_SIZE, 8);
OglSetAttribute ("-gl_buffer", "SDL_GL_BUFFER_SIZE", SDL_GL_BUFFER_SIZE, 32);
//	OglSetAttribute (NULL, "SDL_GL_RED_SIZE", SDL_GL_RED_SIZE, 5 );
//	OglSetAttribute (NULL, "SDL_GL_GREEN_SIZE", SDL_GL_GREEN_SIZE, 5 );
//	OglSetAttribute (NULL, "SDL_GL_BLUE_SIZE", SDL_GL_BLUE_SIZE, 5 );
//	OglSetAttribute (NULL, "SDL_GL_ALPHA_SIZE", SDL_GL_ALPHA_SIZE, 8 );
if (gameOpts->legacy.bZBuf)
	OglSetAttribute (NULL, "SDL_GL_DEPTH_SIZE", SDL_GL_DEPTH_SIZE, 16);
else {
	if ((t = FindArg ("-gl_depth")) && Args [t+1]) {
		gameStates.ogl.nDepthBits = atoi (Args [t + 1]);
		if (gameStates.ogl.nDepthBits <= 0)
			gameStates.ogl.nDepthBits = 24;
		else if (gameStates.ogl.nDepthBits > 24)
			gameStates.ogl.nDepthBits = 24;
		}
	OglSetAttribute (NULL, "SDL_GL_DEPTH_SIZE", SDL_GL_DEPTH_SIZE, gameStates.ogl.nDepthBits);
	OglSetAttribute (NULL, "SDL_GL_STENCIL_SIZE", SDL_GL_STENCIL_SIZE, 8);
	}
OglSetAttribute (NULL, "SDL_GL_ACCUM_RED_SIZE", SDL_GL_ACCUM_RED_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_ACCUM_GREEN_SIZE", SDL_GL_ACCUM_GREEN_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_ACCUM_BLUE_SIZE", SDL_GL_ACCUM_BLUE_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_ACCUM_ALPHA_SIZE", SDL_GL_ACCUM_ALPHA_SIZE, 5);
OglSetAttribute (NULL, "SDL_GL_DOUBLEBUFFER", SDL_GL_DOUBLEBUFFER, 1);
}

//------------------------------------------------------------------------------

void OglClose (void)
{
OglDestroyWindow ();
}

//------------------------------------------------------------------------------
