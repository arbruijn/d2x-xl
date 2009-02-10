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

//------------------------------------------------------------------------------

#define BOX_BORDER (gameStates.menus.bHires?60:30)

//show a message in a nice little box
void ShowBoxedMessage (const char *pszMsg)
{
	int w, h, aw;
	int x, y;
	int nDrawBuffer = gameStates.ogl.nDrawBuffer;

ClearBoxedMessage ();
CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
fontManager.SetCurrent (MEDIUM1_FONT);
fontManager.Current ()->StringSize (pszMsg, w, h, aw);
x = (screen.Width () - w) / 2;
y = (screen.Height () - h) / 2;
if (!gameStates.app.bGameRunning)
	OglSetDrawBuffer (GL_FRONT, 0);
backgroundManager.Setup (NULL, x - BOX_BORDER / 2, y - BOX_BORDER / 2, w + BOX_BORDER, h + BOX_BORDER);
CCanvas::SetCurrent (backgroundManager.Canvas (1));
fontManager.SetColorRGBi (DKGRAY_RGBA, 1, 0, 0);
fontManager.SetCurrent (MEDIUM1_FONT);
GrPrintF (NULL, 0x8000, BOX_BORDER / 2, pszMsg); //(h / 2 + BOX_BORDER) / 2
gameStates.app.bClearMessage = 1;
GrUpdate (0);
if (!gameStates.app.bGameRunning)
	OglSetDrawBuffer (nDrawBuffer, 0);
}

//------------------------------------------------------------------------------

void ClearBoxedMessage (void)
{
if (gameStates.app.bClearMessage) {
	backgroundManager.Remove ();
	gameStates.app.bClearMessage = 0;
	}
#if 0
	CBitmap* bmP = backgroundManager.Current ();

if (bmP) {
	bg.bmP.BlitClipped (bg.x - BOX_BORDER / 2, bg.y - BOX_BORDER / 2);
	if (bg.bmP) {
		delete bg.bmP;
		bg.bmP = NULL;
		}
	}
#endif
}

//------------------------------------------------------------------------------
// eof
