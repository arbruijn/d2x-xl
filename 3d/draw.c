/* $Id: draw.c, v 1.4 2002/07/17 21:55:19 bradleyb Exp $ */
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
 * Drawing routines
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: draw.c, v 1.4 2002/07/17 21:55:19 bradleyb Exp $";
#endif

#include "error.h"

#include "3d.h"
#include "inferno.h"
#include "globvars.h"
#include "texmap.h"
#include "clipper.h"
#include "ogl_defs.h"
#include "ogl_render.h"

#if 1
tmap_drawer_fp tmap_drawer_ptr = draw_tmap;
flat_drawer_fp flat_drawer_ptr = gr_upoly_tmap;
line_drawer_fp line_drawer_ptr = GrLine;
#else
void (*tmap_drawer_ptr) (grsBitmap *bm, int nv, g3sPoint **vertlist) = draw_tmap;
void (*flat_drawer_ptr) (int nv, int *vertlist) = gr_upoly_tmap;
int (*line_drawer_ptr) (fix x0, fix y0, fix x1, fix y1) = GrLine;
#endif

//------------------------------------------------------------------------------
//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void G3SetSpecialRender (tmap_drawer_fp tmap_drawer, flat_drawer_fp flat_drawer, line_drawer_fp line_drawer)
{
tmap_drawer_ptr = (tmap_drawer)?tmap_drawer:draw_tmap;
flat_drawer_ptr = (flat_drawer)?flat_drawer:gr_upoly_tmap;
line_drawer_ptr = (line_drawer)?line_drawer:GrLine;
if (tmap_drawer == DrawTexPolyFlat)
	fpDrawTexPolyMulti = G3DrawTexPolyFlat;
else
	fpDrawTexPolyMulti = gameStates.render.color.bRenderLightMaps ? G3DrawTexPolyLightmap : G3DrawTexPolyMulti;
}

//------------------------------------------------------------------------------
//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool G3CheckNormalFacing (vmsVector *pv, vmsVector *pnorm)
{
vmsVector v;
return (VmVecDot (VmVecSub (&v, &viewInfo.pos, pv), pnorm) > 0);
}

//------------------------------------------------------------------------------

bool DoFacingCheck (vmsVector *norm, g3sPoint **vertlist, vmsVector *p)
{
if (norm) {		//have normal
	Assert (norm->p.x || norm->p.y || norm->p.z);
	return G3CheckNormalFacing (p, norm);
	}
else {	//normal not specified, so must compute
	vmsVector tempv;
	//get three points (rotated) and compute normal
	VmVecPerp (&tempv, &vertlist[0]->p3_vec, &vertlist[1]->p3_vec, &vertlist[2]->p3_vec);
	return (VmVecDot (&tempv, &vertlist[1]->p3_vec) < 0);
	}
}

//------------------------------------------------------------------------------
//like G3DrawPoly (), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like G3CheckNormalFacing () plus
//G3DrawPoly ().
//returns -1 if not facing, 1 if off screen, 0 if drew
bool G3CheckAndDrawPoly (int nv, g3sPoint **pointlist, vmsVector *norm, vmsVector *pnt)
{
	if (DoFacingCheck (norm, pointlist, pnt))
		return G3DrawPoly (nv, pointlist);
	else
		return 255;
}

//------------------------------------------------------------------------------

bool G3CheckAndDrawTMap (
	int nv, g3sPoint **pointlist, tUVL *uvl_list, grsBitmap *bm, vmsVector *norm, vmsVector *pnt)
{
if (DoFacingCheck (norm, pointlist, pnt))
	return !G3DrawTexPoly (nv, pointlist, uvl_list, bm, norm, 1);
else
	return 0;
}

//------------------------------------------------------------------------------
//deal with face that must be clipped
bool MustClipFlatFace (int nv, g3sCodes cc)
{
	int i;
        bool ret=0;
	g3sPoint **bufptr;

	bufptr = clip_polygon (Vbuf0, Vbuf1, &nv, &cc);

	if (nv>0 && ! (cc.or&CC_BEHIND) && !cc.and) {

		for (i=0;i<nv;i++) {
			g3sPoint *p = bufptr[i];
	
			if (! (p->p3_flags & PF_PROJECTED))
				G3ProjectPoint (p);
	
			if (p->p3_flags & PF_OVERFLOW) {
				ret = 1;
				goto free_points;
			}

			polyVertList[i*2]   = p->p3_screen.x;
			polyVertList[i*2+1] = p->p3_screen.y;
		}
	
		 (*flat_drawer_ptr) (nv, (int *)polyVertList);
	}
	else 
		ret=1;

	//D2_FREE temp points
free_points:
	;

	for (i=0;i<nv;i++)
		if (Vbuf1[i]->p3_flags & PF_TEMP_POINT)
			free_temp_point (Vbuf1[i]);

//	Assert (free_point_num==0);

	return ret;
}

//------------------------------------------------------------------------------
//eof
