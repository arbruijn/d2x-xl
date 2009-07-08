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
#include "gameseg.h"
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
	int w, h, aw;
	int x, y;
	
m_tEnter = -1;
m_nDrawBuffer = ogl.m_states.nDrawBuffer;
m_pszMsg = pszMsg;
m_callback = NULL;
Clear ();
fontManager.SetCurrent (MEDIUM1_FONT);
fontManager.Current ()->StringSize (m_pszMsg, w, h, aw);
x = (screen.Width () - w) / 2;
y = (screen.Height () - h) / 2;
backgroundManager.Setup (NULL, x - BOX_BORDER / 2, y - BOX_BORDER / 2, w + BOX_BORDER, h + BOX_BORDER);
gameStates.app.bClearMessage = 1;
if (bFade)
	do {
		Render ();
//		G3_SLEEP (33);
	} while (SDL_GetTicks () - m_tEnter < gameOpts->menus.nFade);
}

//------------------------------------------------------------------------------

void CMessageBox::Render (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
#if 0
if (!gameStates.app.bGameRunning)
	OglSetDrawBuffer (GL_FRONT, 0);
#endif
CCanvas::SetCurrent (backgroundManager.Canvas (1));
FadeIn ();
backgroundManager.Redraw ();
fontManager.SetColorRGBi (DKGRAY_RGBA, 1, 0, 0);
fontManager.SetCurrent (MEDIUM1_FONT);
GrPrintF (NULL, 0x8000, BOX_BORDER / 2, m_pszMsg); //(h / 2 + BOX_BORDER) / 2
GrUpdate (0);
if (!gameStates.app.bGameRunning)
	ogl.SetDrawBuffer (m_nDrawBuffer, 0);
gameStates.render.grAlpha = 1.0f;
}

//------------------------------------------------------------------------------

void CMessageBox::Clear (void)
{
if (gameStates.app.bClearMessage) {
	FadeOut ();
	backgroundManager.Remove ();
	gameStates.app.bClearMessage = 0;
	}
}

//------------------------------------------------------------------------------
// eof
