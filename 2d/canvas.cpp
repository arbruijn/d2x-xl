/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

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

CCanvas*	CCanvas::m_current = NULL;
fix CCanvas::xCanvW2;
fix CCanvas::xCanvH2;
float CCanvas::fCanvW2;
float CCanvas::fCanvH2;

CStack<CCanvas*> CCanvas::m_save;
CScreen* CScreen::m_current = NULL;


CScreen screen;

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
m_info.scale = 1.0;
if (!m_save.Buffer ())
	m_save.Create (10);
}

//	-----------------------------------------------------------------------------

void CCanvas::Setup (int w, int h)
{
Init ();
CBitmap::Setup (BM_LINEAR, w, h, 1, "Canvas");
}

//	-----------------------------------------------------------------------------

CCanvas *CCanvas::CreatePane (int x, int y, int w, int h)
{
	CCanvas *paneP;

if (!(paneP = new CCanvas))	
	return NULL;
SetupPane (paneP, x, y, w, h);
return paneP;
}

//	-----------------------------------------------------------------------------

void CCanvas::Init (int nType, int w, int h, ubyte *data)
{
Init ();
CBitmap::Init (nType, 0, 0, w, h, 1, data);
}

//	-----------------------------------------------------------------------------

void CCanvas::SetupPane (CCanvas *paneP, int x, int y, int w, int h)
{
paneP->SetColor (m_info.color);
paneP->SetDrawMode (m_info.nDrawMode);
paneP->SetScale (m_info.scale);
paneP->SetFont (m_info.font ? m_info.font : GAME_FONT);
paneP->SetFontColor (m_info.fontColors [0], 0);
paneP->SetFontColor (m_info.fontColors [1], 1);
paneP->CBitmap::InitChild (this, x, y, w, h);
}

//	-----------------------------------------------------------------------------

void CCanvas::Destroy (void)
{
if (CCanvas::Current () == this)
	CCanvas::SetCurrent (NULL);
delete this;
}

//	-----------------------------------------------------------------------------

void CCanvas::SetCurrent (CCanvas *canvP)
{
m_current = canvP ? canvP : &gameData.render.frame;
fontManager.SetCurrent (m_current->Font ());
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

//eof
