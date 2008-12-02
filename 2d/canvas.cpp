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
#include "inferno.h"
#include "grdef.h"
#include "canvas.h"

CCanvas*	CCanvas::m_current = NULL;
CCanvas*	CCanvas::m_save [10];
int CCanvas::m_tos = 0;
CScreen* CScreen::m_current = NULL;

CScreen screen;

//	-----------------------------------------------------------------------------

CCanvas *CCanvas::Create (int w, int h)
{
	CCanvas *canvP;

if ((canvP = new CCanvas)) //(CCanvas *)D2_ALLOC( sizeof(CCanvas) );
	canvP->Setup (w, h);
return canvP;
}

//	-----------------------------------------------------------------------------

void CCanvas::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
}

//	-----------------------------------------------------------------------------

void CCanvas::Setup (int w, int h)
{
Init ();
Bitmap ().Setup (BM_LINEAR, w, h, 1);
}

//	-----------------------------------------------------------------------------

CCanvas *CCanvas::CreatePane (int x, int y, int w, int h)
{
	CCanvas *paneP;

if (!(paneP = new CCanvas))	//(CCanvas *) D2_ALLOC (sizeof (CCanvas))))
	return NULL;
SetupPane (paneP, x, y, w, h);
return paneP;
}

//	-----------------------------------------------------------------------------

void CCanvas::Init (int nType, int w, int h, ubyte *data)
{
Init ();
m_info.bm.Init (nType, 0, 0, w, h, 1, data);
}

//	-----------------------------------------------------------------------------

void CCanvas::SetupPane (CCanvas *paneP, int x, int y, int w, int h)
{
paneP->SetColor (m_info.color);
paneP->SetDrawMode (m_info.nDrawMode);
paneP->SetFont (m_info.font);
paneP->SetFontColor (m_info.fontColors [0], 0);
paneP->SetFontColor (m_info.fontColors [1], 1);
paneP->Bitmap ().InitChild (&m_info.bm, x, y, w, h);
}

//	-----------------------------------------------------------------------------

void CCanvas::Destroy (void)
{
m_info.bm.Destroy ();
delete this;
}

//	-----------------------------------------------------------------------------

void CCanvas::SetCurrent (CCanvas *canvP)
{
m_current = canvP ? canvP : CScreen::Current ()->Canvas ();
}

//	-----------------------------------------------------------------------------

void CCanvas::Clear (unsigned int color)
{
if (MODE == BM_OGL)
	SetColorRGBi (color);
else 
	if (color)
		SetColor (CCanvas::Current ()->Bitmap ().Palette ()->ClosestColor (RGBA_RED (color), RGBA_GREEN (color), RGBA_BLUE (color)));
	else
		SetColor (TRANSPARENCY_COLOR);
GrRect (0, 0, GWIDTH-1, GHEIGHT-1);
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
m_info.color.color.red = red;
m_info.color.color.green = green;
m_info.color.color.blue = blue;
m_info.color.color.alpha = alpha;
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
	m_info.color.color.red = (ubyte) (m_info.color.color.red * dFade);
	m_info.color.color.green = (ubyte) (m_info.color.color.green * dFade);
	m_info.color.color.blue = (ubyte) (m_info.color.color.blue * dFade);
	//m_info.color.color.alpha = (ubyte) ((float) gameStates.render.grAlpha / (float) FADE_LEVELS * 255.0f);
	}
}

//	-----------------------------------------------------------------------------

//eof
