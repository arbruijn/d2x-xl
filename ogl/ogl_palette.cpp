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
#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_color.h"
#include "ogl_texcache.h"
#include "sdlgl.h"

#include "screens.h"

#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "gauges.h"
#include "render.h"
#include "particles.h"
#include "glare.h"
#include "newmenu.h"

void GrPaletteStepClear(void); // Function prototype for GrInit;

//------------------------------------------------------------------------------

void OglDoPalFx (void)
{
	int	bDepthTest, bBlend;
	GLint	blendSrc, blendDest;

if (gameStates.render.bPaletteFadedOut) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3f (0,0,0);
	}
else if (gameStates.ogl.bDoPalStep) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3fv ((GLfloat *) &gameStates.ogl.fBright);
	}
else
	return;
if ((bDepthTest = glIsEnabled (GL_DEPTH_TEST)))
	glDisable (GL_DEPTH_TEST);
glDisable (GL_TEXTURE_2D);
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (0,1);
glVertex2f (1,1);
glVertex2f (1,0);
glEnd ();
if (bDepthTest)
	glEnable (GL_DEPTH_TEST);
if (bBlend)
	glBlendFunc (blendSrc, blendDest);
else
	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------

void GrPaletteStepUp (int r, int g, int b)
{
if (gameStates.render.bPaletteFadedOut)
	return;
#if 0
if (!gameOpts->render.bDynLighting || gameStates.menus.nInMenu || !gameStates.app.bGameRunning) 
#endif
	{
	r += gameData.render.nPaletteGamma;
	g += gameData.render.nPaletteGamma;
	b += gameData.render.nPaletteGamma;
	}
CLAMP (r, 0, 64);
CLAMP (g, 0, 64);
CLAMP (b, 0, 64);
if ((gameStates.ogl.bright.red == r) && 
	 (gameStates.ogl.bright.green == g) && 
	 (gameStates.ogl.bright.blue == b))
	return;
gameStates.ogl.bright.red = r;
gameStates.ogl.bright.green = g;
gameStates.ogl.bright.blue = b;
if (gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness) {
	gameStates.ogl.bBrightness = !OglSetBrightnessInternal ();
	gameStates.ogl.bDoPalStep = 0;
	}
else {
	gameStates.ogl.fBright.red = gameStates.ogl.bright.red / 64.0f;
	gameStates.ogl.fBright.green = gameStates.ogl.bright.green / 64.0f;
	gameStates.ogl.fBright.blue = gameStates.ogl.bright.blue / 64.0f;
	gameStates.ogl.bDoPalStep = (r || g || b); //if we arrive here, brightness needs adjustment
	}
}

//------------------------------------------------------------------------------

void GrPaletteStepClear (void)
{
GrPaletteStepUp (0, 0, 0);
gameStates.render.bPaletteFadedOut = 1;
}

//------------------------------------------------------------------------------

#ifdef min
#	undef min
#endif
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

ubyte *pCurPal = NULL;

int GrPaletteStepLoad (ubyte *pal)
{
	static int nCurPalGamma = -1;

if (nCurPalGamma == gameData.render.nPaletteGamma)
	return 1;
nCurPalGamma = gameData.render.nPaletteGamma;
gameStates.render.bPaletteFadedOut = 0;
GrPaletteStepUp (0, 0, 0);
pCurPal = pal;
return 1;
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
}

//------------------------------------------------------------------------------

int GrPaletteFadeOut(ubyte *pal, int nsteps, int allow_keys)
{
gameStates.render.bPaletteFadedOut=1;
return 0;
}

//------------------------------------------------------------------------------

int GrPaletteFadeIn(ubyte *pal, int nsteps, int allow_keys)
{
gameStates.render.bPaletteFadedOut=0;
return 0;
}

//------------------------------------------------------------------------------

void GrPaletteRead (ubyte * pal)
{
Assert (1);
}

//------------------------------------------------------------------------------

