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

#include "descent.h"
#include "globvars.h"
#include "error.h"
#include "3d.h"
#include "globvars.h"
#include "clipper.h"
#include "ogl_defs.h"
#include "ogl_lib.h"

//------------------------------------------------------------------------------
//initialize the 3d system
void g3_init(void)
{
//	div0_init(DM_ERROR);
	atexit(g3_close);
}

//------------------------------------------------------------------------------
//close down the 3d system
void _CDECL_ g3_close(void) {}

//------------------------------------------------------------------------------
//start the frame
void G3StartFrame (int bFlat, int bResetColorBuf)
{
//set int w,h & fixed-point w,h/2
xCanvW2 = (nCanvasWidth = CCanvas::Current ()->Width ()) << 15;
xCanvH2 = (nCanvasHeight = CCanvas::Current ()->Height ()) << 15;
fxCanvW2 = X2F (xCanvW2);
fxCanvH2 = X2F (xCanvH2);
transformation.ComputeAspect ();
InitFreePoints ();
OglStartFrame (bFlat, bResetColorBuf);
gameStates.render.bHeadlightOn = 1;
if (RENDERPATH)
	gameOpts->render.bDepthSort = 1;
}

//------------------------------------------------------------------------------
//this doesn't do anything, but is here for completeness
void G3EndFrame(void)
{
OglEndFrame();
//	Assert(nFreePoints==0);
nFreePoints = 0;
}

//------------------------------------------------------------------------------
//eof
