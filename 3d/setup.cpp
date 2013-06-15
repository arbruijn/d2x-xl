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
#include "ogl_defs.h"
#include "ogl_lib.h"

//------------------------------------------------------------------------------
//start the frame
void G3StartFrame (CTransformation& transformation, int bFlat, int bResetColorBuf, fix xStereoSeparation)
{
CCanvas::Current ()->SetWidth ();
CCanvas::Current ()->SetHeight ();
transformation.ComputeAspect ();
ogl.StartFrame (bFlat, bResetColorBuf, xStereoSeparation);
gameStates.render.bHeadlightOn = 1;
gameStates.render.bDepthSort = 1;
}

//------------------------------------------------------------------------------
//this doesn't do anything, but is here for completeness
void G3EndFrame (CTransformation& transformation, int nWindow)
{
ogl.EndFrame (nWindow);
CCanvas::SetCurrent (&gameData.render.frame);
CCanvas::Current ()->SetWidth ();
CCanvas::Current ()->SetHeight ();
transformation.ComputeAspect ();
}

//------------------------------------------------------------------------------
//eof
