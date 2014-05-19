#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"
#include "console.h"
#include "descent.h"
#include "grdef.h"
#include "ogl_render.h"
#include "canvas.h"

fix CCanvas::xCanvW2;
fix CCanvas::xCanvH2;
float CCanvas::fCanvW2;
float CCanvas::fCanvH2;

CStack<tCanvasBackup> CCanvas::m_save;
float CScreen::m_fScale = 1.0f;

//CScreen screen;

//	-----------------------------------------------------------------------------

CCanvas *CCanvas::Create (int w, int h)
{
	CCanvas *canvP;

if ((canvP = new CCanvas)) 
	canvP->Setup (w, h);
return canvP;
}

//	-----------------------------------------------------------------------------

void CCanvas::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
if (!m_save.Buffer ())
	m_save.Create (10);
m_parent = NULL;
}

//	-----------------------------------------------------------------------------

void CCanvas::Setup (int w, int h)
{
Init ();
Setup (NULL, 0, 0, w, h);
//CBitmap::Setup (BM_LINEAR, w, h, 1, "Canvas");
}

//	-----------------------------------------------------------------------------

CCanvas *CCanvas::CreatePane (int x, int y, int w, int h)
{
	CCanvas *paneP;

if (!(paneP = new CCanvas))	
	return NULL;
//SetupPane (paneP, x, y, w, h);
paneP->Setup (this, x, y, w, h);
return paneP;
}

//	-----------------------------------------------------------------------------

void CCanvas::Init (int nType, int w, int h, ubyte *data)
{
Init ();
//CBitmap::Init (nType, 0, 0, w, h, 1, data);
}

//	-----------------------------------------------------------------------------

void CCanvas::Setup (CCanvas* parentP)
{
SetColor (parentP->m_info.color);
SetDrawMode (parentP->m_info.nDrawMode);
SetFont (parentP->m_info.font ? parentP->m_info.font : GAME_FONT);
SetFontColor (parentP->m_info.fontColors [0], 0);
SetFontColor (parentP->m_info.fontColors [1], 1);
*((CViewport*) this) = *((CViewport*) parentP);
SetWidth ();
SetHeight ();
}

//	-----------------------------------------------------------------------------

void CCanvas::Setup (CCanvas* parentP, int x, int y, int w, int h, bool bUnscale)
{
if (parentP)
	Setup (parentP);
*((CViewport*) this) = bUnscale ? CViewport (Unscaled (x), Unscaled (y), Unscaled (w), Unscaled (h)) : CViewport (x, y, w, h);
SetWidth ();
SetHeight ();
}

//	-----------------------------------------------------------------------------

void CCanvas::Destroy (void)
{
Deactivate ();
delete this;
}

//	-----------------------------------------------------------------------------

void CCanvas::Clear (uint color)
{
SetColorRGBi (color);
OglDrawFilledRect (0, 0, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height () - 1);
}

//	-----------------------------------------------------------------------------

void CCanvas::SetColor (int color)
{
m_info.color.index =color % 256;
m_info.color.rgb = 0;
}

//	-----------------------------------------------------------------------------

void CCanvas::SetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha)
{
m_info.color.rgb = 1;
m_info.color.Red () = red;
m_info.color.Green () = green;
m_info.color.Blue () = blue;
m_info.color.Alpha () = alpha;
}

//	-----------------------------------------------------------------------------

void CCanvas::SetColorRGB15bpp (ushort c, ubyte alpha)
{
CCanvas::SetColorRGB (
	PAL2RGBA (((c >> 10) & 31) * 2), 
	PAL2RGBA (((c >> 5) & 31) * 2), 
	PAL2RGBA ((c & 31) * 2), 
	alpha);
}

//	-----------------------------------------------------------------------------

void CCanvas::FadeColorRGB (double dFade)
{
if (dFade && m_info.color.rgb) {
	m_info.color.Red () = (ubyte) (m_info.color.Red () * dFade);
	m_info.color.Green () = (ubyte) (m_info.color.Green () * dFade);
	m_info.color.Blue () = (ubyte) (m_info.color.Blue () * dFade);
	//m_info.color.Alpha () = (ubyte) ((float) gameStates.render.grAlpha / (float) FADE_LEVELS * 255.0f);
	}
}

//	-----------------------------------------------------------------------------

void CCanvas::GetExtent (CRectangle& rc, bool bScale, bool bDeep)
{
	CCanvas* parent = m_parent;

if (!(bDeep && parent))
	rc = *((CRectangle*) this);
else {
	int l = Left (false);
	int t = Top (false);
	int w = Width (false);
	int h = Height (false);
	while (parent) {
		l += parent->Left ();
		t += parent->Top ();
		if (!w)
			w = parent->Width ();
		if (!h)
			h = parent->Height ();
		parent = parent->Parent ();
		}
	rc = CRectangle (l, t, w, h);
	}
if (bScale) {
	rc.SetLeft (Scaled (rc.Left ()));
	rc.SetTop (Scaled (rc.Top ()));
	rc.SetWidth (Scaled (rc.Width ()));
	rc.SetHeight (Scaled (rc.Width ()));
	}	
}

//	-----------------------------------------------------------------------------

void CCanvas::SetViewport (CCanvas* parent)
{
if (!parent)
	ogl.SetViewport (Left (), Top (), Width (), Height ());
else {
	m_parent = parent;
	int l = Left ();
	int t = Top ();
	int w = Width ();
	int h = Height ();
	while (parent) {
		l += parent->Left ();
		t += parent->Top ();
		if (!w)
			w = parent->Width ();
		if (!h)
			h = parent->Height ();
		parent = parent->Parent ();
		}
	ogl.SetViewport (l, t, w, h);
	}
}

//	-----------------------------------------------------------------------------

void CCanvas::Activate (char* szId, CCanvas* parent, bool bReset) 
{
if (bReset) {
	m_save.Reset ();
	fontManager.Reset ();
	}
//if (CCanvas::Current () != this) 
	{
#if DBG
	SetId (szId);
#endif
	CCanvas::Push (this);
	}
SetViewport (parent);
}

//	-----------------------------------------------------------------------------

void CCanvas::Deactivate (void) 
{ 
CCanvas* canvP;
do {
	if (!(canvP = Current ()))
		break;
	Pop ();
} while (canvP != this);
if (Current ())
	Current ()->Reactivate ();
else
	gameData.render.frame.Activate ("Deactivate (frame)");
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//------------------------------------------------------------------------------

void SetupCanvasses (float scale)
{
if (scale != 0.0f)
	gameData.render.screen.SetScale ((scale < 0.0f) ? ogl.IsOculusRift () ? 1.25f : 1.0f : scale);

if (!ogl.IsSideBySideDevice ())
	gameData.render.frame.Setup (&gameData.render.screen);
else {
	if (ogl.StereoSeparation () < 0)
		gameData.render.frame.Setup (&gameData.render.screen, 0, 0, gameData.render.screen.Width (false) / 2, gameData.render.screen.Height (false));
	else
		gameData.render.frame.Setup (&gameData.render.screen, gameData.render.screen.Width (false) / 2, 0, gameData.render.screen.Width (false) / 2, gameData.render.screen.Height (false));
	}
gameData.render.scene.Setup (&gameData.render.frame);
gameData.render.window.Setup (&gameData.render.scene);
//gameData.render.screen.SetScale (1.0f);
}

//	-----------------------------------------------------------------------------
//eof
