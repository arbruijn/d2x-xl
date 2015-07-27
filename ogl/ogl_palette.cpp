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

void CPaletteManager::RenderEffect (void)
{
if (m_data.nSuspended)
	return;
if (!m_data.bDoEffect)
	return;
ogl.SetBlending (true);
ogl.SetBlendMode (OGL_BLEND_ADD);
glColor3fv (reinterpret_cast<GLfloat*> (&m_data.flash));
ogl.SetDepthTest (false);
ogl.SetTexturing (false);
ogl.RenderScreenQuad ();
ogl.SetDepthTest (true);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
}

//------------------------------------------------------------------------------

void CPaletteManager::SetEffect (float red, float green, float blue, bool bForce)
{
if (m_data.nSuspended)
	return;
#if 0
if (!gameStates.render.nLightingMethod) // || gameStates.menus.nInMenu || !gameStates.app.bGameRunning) 
	{
	red += float (m_data.nGamma) / 64.0f;
	green += float (m_data.nGamma) / 64.0f;
	blue += float (m_data.nGamma) / 64.0f;
	}
#endif
CLAMP (red, 0, 1);
CLAMP (green, 0, 1);
CLAMP (blue, 0, 1);
if (!bForce && (m_data.flash.Red () == red) && (m_data.flash.Green () == green) && (m_data.flash.Blue () == blue))
	return;
m_data.flash.Red () = red;
m_data.flash.Green () = green;
m_data.flash.Blue () = blue;
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
m_data.effect.Red () = float (red) / 64.0f;
m_data.effect.Green () = float (green) / 64.0f;
m_data.effect.Blue () = float (blue) / 64.0f;
CLAMP (m_data.effect.Red (), 0, 1);
CLAMP (m_data.effect.Green (), 0, 1);
CLAMP (m_data.effect.Blue (), 0, 1);
SetEffect (bForce);
}

//------------------------------------------------------------------------------

void CPaletteManager::ClearEffect (void)
{
SetEffect (0, 0, 0);
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
SetEffect (0, 0, 0);
m_data.current = Add (*palette);
return 1;
//ogl.m_states.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && ogl.m_states.bBrightness);
}

//------------------------------------------------------------------------------

int CPaletteManager::DisableEffect (void)
{
#if 0 //obsolete
m_data.nSuspended++; obsolete
#endif
return 0;
}

//------------------------------------------------------------------------------

int CPaletteManager::EnableEffect (bool bReset)
{
#if 0 //obsolete
if (bReset)
	m_data.nSuspended = 0;
else if (m_data.nSuspended > 0)
	m_data.nSuspended--;
#endif
return 0;
}

//------------------------------------------------------------------------------

