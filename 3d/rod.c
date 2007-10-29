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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: rod.c, v 1.4 2002/07/17 21:55:19 bradleyb Exp $";
#endif

#include "3d.h"
#include "globvars.h"
#include "fix.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_render.h"

#define RESCALE_ROD	0

grsPoint blobVertices [4];
g3sPoint rodPoints [4];
g3sPoint *rodPointList [] = {rodPoints, rodPoints + 1, rodPoints + 2, rodPoints + 3};

tUVL rodUvlList [4] = {
	{0x0200, 0x0200, 0}, 
	{0xfe00, 0x0200, 0}, 
	{0xfe00, 0xfe00, 0}, 
	{0x0200, 0xfe00, 0}};

//------------------------------------------------------------------------------
//compute the corners of a rod.  fills in vertbuf.
int CalcRodCorners (g3sPoint *btmPoint, fix xBtmWidth, g3sPoint *topPoint, fix xTopWidth)
{
	vmsVector	vDelta, vTop, vTemp, vRodNorm;
	ubyte			andCodes;
	int			i;

//compute vector from one point to other, do cross product with vector
//from eye to get perpendicular
VmVecSub (&vDelta, &btmPoint->p3_vec, &topPoint->p3_vec);
//unscale for aspect
#if RESCALE_ROD
vDelta.p.x = FixDiv (vDelta.p.x, viewInfo.scale.p.x);
vDelta.p.y = FixDiv (vDelta.p.y, viewInfo.scale.p.y);
#endif
//calc perp vector
//do lots of normalizing to prevent overflowing.  When this code works, 
//it should be optimized
VmVecNormalize (&vDelta);
VmVecCopyNormalize (&vTop, &topPoint->p3_vec);
VmVecCross (&vRodNorm, &vDelta, &vTop);
VmVecNormalize (&vRodNorm);
//scale for aspect
#if RESCALE_ROD
vRodNorm.p.x = FixMul (vRodNorm.p.x, viewInfo.scale.p.x);
vRodNorm.p.y = FixMul (vRodNorm.p.y, viewInfo.scale.p.y);
#endif
//now we have the usable edge.  generate four points
//vTop points
VmVecCopyScale (&vTemp, &vRodNorm, xTopWidth);
vTemp.p.z = 0;
VmVecAdd (&rodPoints [0].p3_vec, &topPoint->p3_vec, &vTemp);
VmVecSub (&rodPoints [1].p3_vec, &topPoint->p3_vec, &vTemp);
VmVecCopyScale (&vTemp, &vRodNorm, xBtmWidth);
vTemp.p.z = 0;
VmVecSub (&rodPoints [2].p3_vec, &btmPoint->p3_vec, &vTemp);
VmVecAdd (&rodPoints [3].p3_vec, &btmPoint->p3_vec, &vTemp);

//now code the four points
for (i = 0, andCodes = 0xff; i < 4; i++)
	andCodes &= G3EncodePoint (rodPoints + i);
if (andCodes)
	return 1;		//1 means off screen
//clear flags for new points (not projected)
for (i = 0; i < 4; i++) {
	rodPoints [i].p3_flags = 0;
	rodPoints [i].p3_index = -1;
	}
return 0;
}

//------------------------------------------------------------------------------
//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
bool G3DrawRodPoly (g3sPoint *btmPoint, fix xBtmWidth, g3sPoint *topPoint, fix xTopWidth)
{
if (CalcRodCorners (btmPoint, xBtmWidth, topPoint, xTopWidth))
	return 0;
return G3DrawPoly (4, rodPointList);
}

//------------------------------------------------------------------------------
//draw a bitmap tObject that is always facing you
//returns 1 if off screen, 0 if drew
bool G3DrawRodTexPoly (grsBitmap *bmP, g3sPoint *btmPoint, fix xBtmWidth, g3sPoint *topPoint, fix xTopWidth, fix light, tUVL *uvlList)
{
if (CalcRodCorners (btmPoint, xBtmWidth, topPoint, xTopWidth))
	return 0;
if (!uvlList)
	uvlList = rodUvlList;
uvlList [0].l = 
uvlList [1].l = 
uvlList [2].l = 
uvlList [3].l = light;
return G3DrawTexPoly (4, rodPointList, uvlList, bmP, NULL, 1);
}

//------------------------------------------------------------------------------
//eof
