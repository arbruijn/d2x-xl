/* $Id: rod.c, v 1.4 2002/07/17 21:55:19 bradleyb Exp $ */
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
 * Rod routines
 * 
 * Old Log:
 *
 * Revision 1.2  1995/09/13  11:31:46  allender
 * removed CheckMulDiv in PPC implemenation
 *
 * Revision 1.1  1995/05/05  08:52:45  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  06:42:08  matt
 * Initial revision
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: rod.c, v 1.4 2002/07/17 21:55:19 bradleyb Exp $";
#endif

#include "3d.h"
#include "globvars.h"
#include "fix.h"
#include "ogl_init.h"

grs_point blob_vertices[4];
g3s_point rodPoints[4];
g3s_point *rodPointList[] = {&rodPoints[0], &rodPoints[1], &rodPoints[2], &rodPoints[3]};

uvl uvl_list[4] = {
	{ 0x0200, 0x0200, 0 }, 
	{ 0xfe00, 0x0200, 0 }, 
	{ 0xfe00, 0xfe00, 0 }, 
	{ 0x0200, 0xfe00, 0 }};

//------------------------------------------------------------------------------
//compute the corners of a rod.  fills in vertbuf.
int CalcRodCorners (g3s_point *bot_point, fix bot_width, g3s_point *top_point, fix top_width)
{
	vmsVector	delta_vec, top, tempv, rod_norm;
	ubyte			codes_and;
	int			i;

//compute vector from one point to other, do cross product with vector
//from eye to get perpendiclar
VmVecSub (&delta_vec, &bot_point->p3_vec, &top_point->p3_vec);
//unscale for aspect
delta_vec.x = FixDiv (delta_vec.x, viewInfo.scale.x);
delta_vec.y = FixDiv (delta_vec.y, viewInfo.scale.y);
//calc perp vector
//do lots of normalizing to prevent overflowing.  When this code works, 
//it should be optimized
VmVecNormalize (&delta_vec);
VmVecCopyNormalize (&top, &top_point->p3_vec);
VmVecCross (&rod_norm, &delta_vec, &top);
VmVecNormalize (&rod_norm);
//scale for aspect
rod_norm.x = FixMul (rod_norm.x, viewInfo.scale.x);
rod_norm.y = FixMul (rod_norm.y, viewInfo.scale.y);
//now we have the usable edge.  generate four points
//top points
VmVecCopyScale (&tempv, &rod_norm, top_width);
tempv.z = 0;
VmVecAdd (&rodPoints [0].p3_vec, &top_point->p3_vec, &tempv);
VmVecSub (&rodPoints [1].p3_vec, &top_point->p3_vec, &tempv);
VmVecCopyScale (&tempv, &rod_norm, bot_width);
tempv.z = 0;
VmVecSub (&rodPoints [2].p3_vec, &bot_point->p3_vec, &tempv);
VmVecAdd (&rodPoints [3].p3_vec, &bot_point->p3_vec, &tempv);

//now code the four points
for (i = 0, codes_and = 0xff; i < 4; i++)
	codes_and &= G3EncodePoint (rodPoints + i);
if (codes_and)
	return 1;		//1 means off screen
//clear flags for new points (not projected)
for (i = 0; i < 4; i++) {
	rodPoints [i].p3Flags = 0;
	rodPoints [i].p3_index = -1;
	}
return 0;
}

//------------------------------------------------------------------------------
//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
bool G3DrawRodPoly (g3s_point *bot_point, fix bot_width, g3s_point *top_point, fix top_width)
{
if (CalcRodCorners (bot_point, bot_width, top_point, top_width))
	return 0;
return G3DrawPoly (4, rodPointList);
}

//------------------------------------------------------------------------------
//draw a bitmap tObject that is always facing you
//returns 1 if off screen, 0 if drew
bool G3DrawRodTexPoly (grs_bitmap *bitmap, g3s_point *bot_point, fix bot_width, g3s_point *top_point, fix top_width, fix light)
{
if (CalcRodCorners (bot_point, bot_width, top_point, top_width))
	return 0;
uvl_list [0].l = 
uvl_list [1].l = 
uvl_list [2].l = 
uvl_list [3].l = light;
return G3DrawTexPoly (4, rodPointList, uvl_list, bitmap, NULL, 1);
}

//------------------------------------------------------------------------------
#ifndef __powerc
int CheckMulDiv (fix *r, fix a, fix b, fix c);
#endif

#if (! (defined (D1XD3D) || defined (OGL)))
//draws a bitmap with the specified 3d width & height 
//returns 1 if off screen, 0 if drew
bool G3DrawBitMap (vmsVector *pos, fix width, fix height, grs_bitmap *bm, int orientation)
{
#ifndef __powerc
	g3s_point pnt;
	fix t, w, h;

	if (G3TransformAndEncodePoint (&pnt, pos) & CC_BEHIND)
		return 1;

	G3ProjectPoint (&pnt);

	if (pnt.p3Flags & PF_OVERFLOW)
		return 1;

	if (CheckMulDiv (&t, width, xCanvW2, pnt.p3_z))
		w = FixMul (t, viewInfo.scale.x);
	else
		return 1;

	if (CheckMulDiv (&t, height, xCanvH2, pnt.p3_z))
		h = FixMul (t, viewInfo.scale.y);
	else
		return 1;

	blob_vertices[0].x = pnt.p3_sx - w;
	blob_vertices[0].y = blob_vertices[1].y = pnt.p3_sy - h;
	blob_vertices[1].x = blob_vertices[2].x = pnt.p3_sx + w;
	blob_vertices[2].y = pnt.p3_sy + h;

	scale_bitmap (bm, blob_vertices, 0);

	return 0;
#else
	g3s_point pnt;
	fix w, h;
	double fz;

	if (G3TransformAndEncodePoint (&pnt, pos) & CC_BEHIND)
		return 1;

	G3ProjectPoint (&pnt);

	if (pnt.p3Flags & PF_OVERFLOW)
		return 1;

	if (pnt.p3_z == 0)
		return 1;
		
	fz = f2fl (pnt.p3_z);
	w = FixMul (fl2f (( (f2fl (width)*fxCanvW2) / fz)), viewInfo.scale.x);
	h = FixMul (fl2f (( (f2fl (height)*fxCanvH2) / fz)), viewInfo.scale.y);

	blob_vertices[0].x = pnt.p3_sx - w;
	blob_vertices[0].y = blob_vertices[1].y = pnt.p3_sy - h;
	blob_vertices[1].x = blob_vertices[2].x = pnt.p3_sx + w;
	blob_vertices[2].y = pnt.p3_sy + h;

	scale_bitmap (bm, blob_vertices);

	return 0;
#endif
}
#endif

//------------------------------------------------------------------------------
//eof
