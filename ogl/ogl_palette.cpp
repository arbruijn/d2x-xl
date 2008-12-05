// Palette functions

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

//------------------------------------------------------------------------------

void CPaletteManager::ApplyEffect (void)
{
	int	bDepthTest, bBlend;
	GLint	blendSrc, blendDest;

if (m_data.bFadedOut) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3f (0,0,0);
	}
else if (m_data.bDoEffect) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3fv (reinterpret_cast<GLfloat*> (&m_data.fflash));
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

void CPaletteManager::SetEffect (int r, int g, int b)
{
if (m_data.bFadedOut)
	return;
#if 0
if (!gameOpts->render.nLightingMethod || gameStates.menus.nInMenu || !gameStates.app.bGameRunning) 
#endif
	{
	r += m_data.nGamma;
	g += m_data.nGamma;
	b += m_data.nGamma;
	}
CLAMP (r, 0, 64);
CLAMP (g, 0, 64);
CLAMP (b, 0, 64);
if ((m_data.flash.red == r) && (m_data.flash.green == g) && (m_data.flash.blue == b))
	return;
m_data.flash.red = r;
m_data.flash.green = g;
m_data.flash.blue = b;
if (gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness) {
	gameStates.ogl.bBrightness = !OglSetBrightnessInternal ();
	m_data.bDoEffect = 0;
	}
else {
	m_data.fflash.red = m_data.flash.red / 64.0f;
	m_data.fflash.green = m_data.flash.green / 64.0f;
	m_data.fflash.blue = m_data.flash.blue / 64.0f;
	m_data.bDoEffect = (r || g || b); //if we arrive here, brightness needs adjustment
	}
}

//------------------------------------------------------------------------------

void CPaletteManager::ClearEffect (void)
{
SetEffect (0, 0, 0);
m_data.bFadedOut = true;
}

//------------------------------------------------------------------------------

#ifdef min
#	undef min
#endif
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

int CPaletteManager::ClearEffect (CPalette* palette)
{
if (m_data.nLastGamma == m_data.nGamma)
	return 1;
m_data.nLastGamma = m_data.nGamma;
m_data.bFadedOut = 0;
SetEffect (0, 0, 0);
m_data.current = Add (*palette);
return 1;
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
}

//------------------------------------------------------------------------------

int CPaletteManager:: FadeOut (void)
{
m_data.bFadedOut = 1;
return 0;
}

//------------------------------------------------------------------------------

int CPaletteManager::FadeIn (void)
{
m_data.bFadedOut = 0;
return 0;
}

//------------------------------------------------------------------------------

