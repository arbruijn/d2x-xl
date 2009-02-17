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

#include "descent.h"
#include "error.h"
#include "3d.h"
#include "globvars.h"
#include "texmap.h"
#include "clipper.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_render.h"

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
//eof
