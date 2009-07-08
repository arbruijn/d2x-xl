// Palette functions

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "hudmsgs.h"
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
#include "cockpit.h"
#include "rendermine.h"
#include "particles.h"
#include "glare.h"
#include "menu.h"

//------------------------------------------------------------------------------

void CPaletteManager::ApplyEffect (void)
{
	int	bDepthTest, bBlend;
	GLint	blendSrc, blendDest;

if (!m_data.bAllowEffect) {
#if 1
	return;
#else
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3f (0, 0, 0);
#endif
	}
else if (m_data.bDoEffect) {
	if ((bBlend = glIsEnabled (GL_BLEND))) {
		glGetIntegerv (GL_BLEND_SRC, &blendSrc);
		glGetIntegerv (GL_BLEND_DST, &blendDest);
		}
	else
		glEnable (GL_BLEND);
	glBlendFunc (GL_ONE,GL_ONE);
	glColor3fv (reinterpret_cast<GLfloat*> (&m_data.flash));
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

void CPaletteManager::SetEffect (float red, float green, float blue, bool bForce)
{
if (!m_data.bAllowEffect)
	return;
#if 0
if (!gameStates.render.nLightingMethod || gameStates.menus.nInMenu || !gameStates.app.bGameRunning) 
#endif
 {
	red += float (m_data.nGamma) / 64.0f;
	green += float (m_data.nGamma) / 64.0f;
	blue += float (m_data.nGamma) / 64.0f;
	}
CLAMP (red, 0, 1);
CLAMP (green, 0, 1);
CLAMP (blue, 0, 1);
if (!bForce && (m_data.flash.red == red) && (m_data.flash.green == green) && (m_data.flash.blue == blue))
	return;
m_data.flash.red = red;
m_data.flash.green = green;
m_data.flash.blue = blue;
if (gameOpts->ogl.bSetGammaRamp && ogl.m_states.bBrightness) {
	ogl.m_states.bBrightness = !SdlGlSetBrightnessInternal ();
	m_data.bDoEffect = 0;
	}
else {
	m_data.bDoEffect = ((red != 0) || (green != 0) || (blue != 0)); //if we arrive here, brightness needs adjustment
	}
}

//------------------------------------------------------------------------------

void CPaletteManager::SetEffect (int red, int green, int blue, bool bForce)
{
m_data.effect.red = float (red) / 64.0f;
m_data.effect.green = float (green) / 64.0f;
m_data.effect.blue = float (blue) / 64.0f;
CLAMP (m_data.effect.red, 0, 1);
CLAMP (m_data.effect.green, 0, 1);
CLAMP (m_data.effect.blue, 0, 1);
SetEffect (bForce);
}

//------------------------------------------------------------------------------

void CPaletteManager::ClearEffect (void)
{
SetEffect (0, 0, 0);
m_data.bAllowEffect = false;
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
m_data.bAllowEffect = true;
SetEffect (0, 0, 0);
m_data.current = Add (*palette);
return 1;
//ogl.m_states.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && ogl.m_states.bBrightness);
}

//------------------------------------------------------------------------------

int CPaletteManager::DisableEffect (void)
{
m_data.bAllowEffect = false;
return 0;
}

//------------------------------------------------------------------------------

int CPaletteManager::EnableEffect (void)
{
m_data.bAllowEffect = true;
return 0;
}

//------------------------------------------------------------------------------

