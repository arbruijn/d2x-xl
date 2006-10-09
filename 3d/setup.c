/* $Id: setup.c,v 1.5 2003/03/19 19:21:34 btb Exp $ */
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
/*
 * 
 * Setup for 3d library
 * 
 * Old Log:
 *
 * Revision 1.4  1995/10/11  00:27:04  allender
 * bash free_num_points to 0
 *
 * Revision 1.3  1995/09/13  11:31:58  allender
 * calc for fxCanvW2 and fxCanvH2
 *
 * Revision 1.2  1995/06/25  21:57:57  allender
 * *** empty log message ***
 *
 * Revision 1.1  1995/05/05  08:52:54  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  03:59:01  matt
 * Initial revision
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: setup.c,v 1.5 2003/03/19 19:21:34 btb Exp $";
#endif

#include <stdlib.h>

#include "error.h"

#include "3d.h"
#include "globvars.h"
#include "inferno.h"
#include "clipper.h"
//#include "div0.h"

#ifdef OGL
#include "ogl_init.h"
#else
#include "texmap.h"  // for init_interface_vars_to_assembler()
#endif

//initialize the 3d system
void g3_init(void)
{
//	div0_init(DM_ERROR);
	atexit(g3_close);
}

//close down the 3d system
void _CDECL_ g3_close(void) {}

//start the frame
void G3StartFrame(int bFlat, int bResetColorBuf)
{
	fix s;

	//set int w,h & fixed-point w,h/2
xCanvW2 = (nCanvasWidth  = grdCurCanv->cv_bitmap.bm_props.w)<<15;
xCanvH2 = (nCanvasHeight = grdCurCanv->cv_bitmap.bm_props.h)<<15;
#ifdef __powerc
fxCanvW2 = f2fl((nCanvasWidth  = grdCurCanv->cv_bitmap.bm_props.w)<<15);
fxCanvH2 = f2fl((nCanvasHeight = grdCurCanv->cv_bitmap.bm_props.h)<<15);
#endif

//compute aspect ratio for this canvas
s = FixMulDiv(grdCurScreen->sc_aspect,nCanvasHeight,nCanvasWidth);
if (s <= f1_0) {	   //scale x
	viewInfo.windowScale.x = s;
	viewInfo.windowScale.y = f1_0;
}
else {
	viewInfo.windowScale.y = FixDiv(f1_0,s);
	viewInfo.windowScale.x = f1_0;
}
viewInfo.windowScale.z = f1_0;		//always 1
init_free_points();
#ifdef D1XD3D
Win32_start_frame ();
#else
#ifdef OGL
OglStartFrame (bFlat, bResetColorBuf);
#else
init_interface_vars_to_assembler ();		//for the texture-mapper
#endif
#endif
gameStates.render.bHeadlightOn = 1;
}

//this doesn't do anything, but is here for completeness
void G3EndFrame(void)
{
#ifdef D1XD3D
	Win32_end_frame ();
#endif
#ifdef OGL
	OglEndFrame();
#endif

//	Assert(free_point_num==0);
	free_point_num = 0;

}


