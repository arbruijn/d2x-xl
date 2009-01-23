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

#include "inferno.h"
#include "error.h"
#include "3d.h"
#include "globvars.h"
#include "texmap.h"
#include "clipper.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_render.h"

#if 1
tmap_drawer_fp tmap_drawer_ptr = draw_tmap;
flat_drawer_fp flat_drawer_ptr = gr_upoly_tmap;
#else
void (*tmap_drawer_ptr) (CBitmap *bm, int nv, g3sPoint **vertlist) = draw_tmap;
void (*flat_drawer_ptr) (int nv, int *vertlist) = gr_upoly_tmap;
#endif

//------------------------------------------------------------------------------
//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void G3SetSpecialRender (tmap_drawer_fp tmap_drawer, flat_drawer_fp flat_drawer, line_drawer_fp line_drawer)
{
tmap_drawer_ptr = (tmap_drawer)?tmap_drawer:draw_tmap;
flat_drawer_ptr = (flat_drawer)?flat_drawer:gr_upoly_tmap;
if (tmap_drawer == DrawTexPolyFlat)
	fpDrawTexPolyMulti = G3DrawTexPolyFlat;
else
	fpDrawTexPolyMulti = gameStates.render.color.bRenderLightmaps ? G3DrawTexPolyLightmap : G3DrawTexPolyMulti;
}

//------------------------------------------------------------------------------
//returns true if a plane is facing the viewer. takes the unrotated surface
//Normal of the plane, and a point on it.  The Normal need not be normalized
int G3CheckNormalFacing(const CFixVector& pv, const CFixVector& pnorm)
{
CFixVector v = transformation.m_info.pos - pv;
return (CFixVector::Dot (v, pnorm) > 0);
}

//------------------------------------------------------------------------------

int DoFacingCheck (CFixVector *norm, g3sPoint **vertlist, CFixVector *p)
{
if (norm) {		//have Normal
	return G3CheckNormalFacing (*p, *norm);
	}
else {	//Normal not specified, so must compute
	CFixVector vTemp;
	//get three points (rotated) and compute Normal
	vTemp = CFixVector::Perp(vertlist [0]->p3_vec, vertlist [1]->p3_vec, vertlist [2]->p3_vec);
	return (CFixVector::Dot (vTemp, vertlist [1]->p3_vec) < 0);
	}
}

//------------------------------------------------------------------------------
//like G3DrawPoly (), but checks to see if facing.  If surface Normal is
//NULL, this routine must compute it, which will be slow.  It is better to
//pre-compute the Normal, and pass it to this function.  When the Normal
//is passed, this function works like G3CheckNormalFacing () plus
//G3DrawPoly ().
//returns -1 if not facing, 1 if off screen, 0 if drew
int G3CheckAndDrawPoly (int nv, g3sPoint **pointlist, CFixVector *norm, CFixVector *pnt)
{
	if (DoFacingCheck (norm, pointlist, pnt))
		return G3DrawPoly (nv, pointlist);
	else
		return 255;
}

//------------------------------------------------------------------------------

int G3CheckAndDrawTMap (
	int nv, g3sPoint **pointlist, tUVL *uvl_list, CBitmap *bm, CFixVector *norm, CFixVector *pnt)
{
if (DoFacingCheck (norm, pointlist, pnt))
	return !G3DrawTexPoly (nv, pointlist, uvl_list, bm, norm, 1, -1);
else
	return 0;
}

//------------------------------------------------------------------------------
//deal with face that must be clipped
int MustClipFlatFace (int nv, g3sCodes cc)
{
	int i;
        int ret=0;
	g3sPoint **bufptr;

	bufptr = clip_polygon (Vbuf0, Vbuf1, &nv, &cc);

	if (nv>0 && ! (cc.ccOr&CC_BEHIND) && !cc.ccAnd) {

		for (i=0;i<nv;i++) {
			g3sPoint *p = bufptr[i];

			if (! (p->p3_flags & PF_PROJECTED))
				G3ProjectPoint (p);

			if (p->p3_flags & PF_OVERFLOW) {
				ret = 1;
				goto free_points;
			}

			polyVertList[i*2]   = I2X (p->p3_screen.x);
			polyVertList[i*2+1] = I2X (p->p3_screen.y);
		}

		 (*flat_drawer_ptr) (nv, reinterpret_cast<int*> (polyVertList));
	}
	else
		ret=1;

	//D2_FREE temp points
free_points:
	;

	for (i=0;i<nv;i++)
		if (Vbuf1[i]->p3_flags & PF_TEMP_POINT)
			free_temp_point (Vbuf1[i]);

//	Assert (nFreePoints==0);

	return ret;
}

//------------------------------------------------------------------------------
//eof
