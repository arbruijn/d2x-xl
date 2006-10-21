/* $Id: matrix.c,v 1.4 2002/07/17 21:55:19 bradleyb Exp $ */
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
 * Matrix setup & manipulation routines
 * 
 * Old Log:
 *
 * Revision 1.1  1995/05/05  08:52:11  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  04:14:34  matt
 * Initial revision
 * 
 * 
 */
 
#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: matrix.c,v 1.4 2002/07/17 21:55:19 bradleyb Exp $";
#endif

#include "3d.h"
#include "globvars.h"
#include "ogl_init.h"
#include "inferno.h"
#include "oof.h"

void ScaleMatrix(void);

//------------------------------------------------------------------------------
//set view from x,y,z & p,b,h, zoom.  Must call one of g3_set_view_*() 
void G3SetViewAngles (vmsVector *view_pos, vmsAngVec *view_orient, fix zoom)
{
viewInfo.zoom = zoom;
viewInfo.position = *view_pos;
VmAngles2Matrix(&viewInfo.view,view_orient);
VmsVecToFloat (&viewInfo.posf, &viewInfo.position);
VmsMatToFloat (&viewInfo.viewf, &viewInfo.view);
#ifdef D1XD3D
Win32_set_view_matrix ();
#endif
ScaleMatrix();
}

//------------------------------------------------------------------------------
//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void G3SetViewMatrix (vmsVector *view_pos, vmsMatrix *view_matrix, fix zoom)
{
viewInfo.zoom = zoom;
viewInfo.glZoom = (float) zoom / 65536.0f;
if (view_pos) {
	viewInfo.position = *view_pos;
	VmsVecToFloat (&viewInfo.posf, &viewInfo.position);
	OOF_VecVms2Gl (viewInfo.glPosf, &viewInfo.position);
	}
if (view_matrix) {
	viewInfo.view = *view_matrix;
	VmsMatToFloat (&viewInfo.viewf, &viewInfo.view);
	OOF_MatVms2Gl (OOF_GlIdent (viewInfo.glViewf), &viewInfo.view);
	}
#ifdef D1XD3D
Win32_set_view_matrix ();
#endif
ScaleMatrix();
}

//------------------------------------------------------------------------------
//performs aspect scaling on global view matrix
void ScaleMatrix (void)
{
	viewInfo.unscaledView = viewInfo.view;		//so we can use unscaled if we want

viewInfo.scale = viewInfo.windowScale;
if (viewInfo.zoom <= f1_0) 		//zoom in by scaling z
	viewInfo.scale.z =  FixMul (viewInfo.scale.z, viewInfo.zoom);
else {			//zoom out by scaling x&y
	fix s = FixDiv (f1_0, viewInfo.zoom);

	viewInfo.scale.x = FixMul (viewInfo.scale.x,s);
	viewInfo.scale.y = FixMul (viewInfo.scale.y,s);
	}
//now scale matrix elements
VmVecScale (&viewInfo.view.rvec, viewInfo.scale.x);
VmVecScale (&viewInfo.view.uvec, viewInfo.scale.y);
VmVecScale (&viewInfo.view.fvec, viewInfo.scale.z);
VmsMatToFloat (&viewInfo.viewf, &viewInfo.view);
}

//------------------------------------------------------------------------------
//eof