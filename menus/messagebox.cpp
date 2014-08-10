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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "descent.h"
#include "u_mem.h"
#include "error.h"
#include "segmath.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "ogl_color.h"
#include "ogl_render.h"
#include "ogl_hudstuff.h"
#include "renderlib.h"
#include "renderthreads.h"
#include "cameras.h"
#include "menubackground.h"

CMessageBox messageBox;

//------------------------------------------------------------------------------

#define BOX_BORDER (gameStates.menus.bHires ? 60 : 30)

//show a message in a nice little box
void CMessageBox::Show (const char *pszMsg, bool bFade)
{
	int32_t w, h, aw;
	
m_tEnter = -1;
m_nDrawBuffer = ogl.m_states.nDrawBuffer;
m_pszMsg = pszMsg;
m_callback = NULL;
Clear ();
fontManager.SetCurrent (MEDIUM1_FONT);
fontManager.PushScale ();
fontManager.SetScale (fontManager.Scale (false) * CMenu::GetScale ());
fontManager.Current ()->StringSize (m_pszMsg, w, h, aw);
m_x = (gameData.render.frame.Width (false) - w) / 2;
m_y = (gameData.render.frame.Height (false) - h) / 2;
backgroundManager.Setup (m_background, w + BOX_BORDER, h + BOX_BORDER, BG_SUBMENU);
gameStates.app.bClearMessage = 1;
if (bFade)
	do {
		CMenu::Render (NULL, NULL);
		} while (SDL_GetTicks () - m_tEnter < gameOpts->menus.nFade);
fontManager.PopScale ();
}

//------------------------------------------------------------------------------

void CMessageBox::Render (void)
{
#if 1

	static	int32_t t0 = 0;

if (!(BeginRenderMenu () && MenuRenderTimeout (t0, -1)))
	return;

FadeIn ();
backgroundManager.Activate (m_background);
fontManager.SetColorRGBi (DKGRAY_RGBA, 1, 0, 0);
fontManager.SetCurrent (MEDIUM1_FONT);
GrPrintF (NULL, m_x - CCanvas::Current ()->Left (), m_y - CCanvas::Current ()->Top (), m_pszMsg); 
#if 0
if (gameStates.app.bGameRunning)
	ogl.ChooseDrawBuffer ();
else
	ogl.SetDrawBuffer (m_nDrawBuffer, 0);
#endif
gameStates.render.grAlpha = 1.0f;
m_background.Deactivate ();

#else

	static	int32_t t0 = 0;

if (!(BeginRenderMenu () && MenuRenderTimeout (t0, -1)))
	return;

CCanvas::SetCurrent (backgroundManager.Canvas (1));
FadeIn ();
//backgroundManager.Redraw ();
backgroundManager.Activate (m_background);
fontManager.SetColorRGBi (DKGRAY_RGBA, 1, 0, 0);
fontManager.SetCurrent (MEDIUM1_FONT);
GrPrintF (NULL, 0x8000, BOX_BORDER / 2, m_pszMsg); 
GrUpdate (0);
if (gameStates.app.bGameRunning)
	ogl.ChooseDrawBuffer ();
else
	ogl.SetDrawBuffer (m_nDrawBuffer, 0);
gameStates.render.grAlpha = 1.0f;
m_background.Deactivate ();

#endif
}

//------------------------------------------------------------------------------

void CMessageBox::Clear (void)
{
if (gameStates.app.bClearMessage) {
	FadeOut ();
	backgroundManager.Draw ();
	gameStates.app.bClearMessage = 0;
	}
}

//------------------------------------------------------------------------------
// eof
