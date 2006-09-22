/* $Id: points.c,v 1.5 2002/10/28 19:49:15 btb Exp $ */
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
 * Routines for point definition, rotation, etc.
 * 
 * Old Log:
 *
 * Revision 1.3  1995/09/21  17:29:40  allender
 * changed project_point to overflow if z <= 0
 *
 * Revision 1.2  1995/09/13  11:31:28  allender
 * removed CheckMulDiv from G3ProjectPoint
 *
 * Revision 1.1  1995/05/05  08:52:35  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  04:32:25  matt
 * Initial revision
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: points.c,v 1.5 2002/10/28 19:49:15 btb Exp $";
#endif

#include "3d.h"
#include "globvars.h"

// -----------------------------------------------------------------------------------
//checks for overflow & divides if ok, fillig in r
//returns true if div is ok, else false
#ifdef _WIN32
inline 
#endif
int CheckMulDiv (fix *r, fix a, fix b, fix c)
{
#ifdef _WIN32
	QLONG	q;
	if (!c)
		return 0;
	q = mul64 (a, b) / (QLONG) c;
	if ((q > 0x7fffffff) || (q < -0x7fffffff))
		return 0;
	*r = (fix) q;
	return 1;
#else
	quadint q,qt;

	q.low=q.high=0;
	fixmulaccum(&q,a,b);
	qt = q;
	if (qt.high < 0)
		fixquadnegate(&qt);
	qt.high *= 2;
	if (qt.low > 0x7fff)
		qt.high++;
	if (qt.high >= c)
		return 0;
	else {
		*r = fixdivquadlong(q.low,q.high,c);
		return 1;
	}
#endif
}

// -----------------------------------------------------------------------------------
//projects a point
void G3ProjectPoint(g3s_point *p)
{
#ifndef __powerc
	fix tx,ty;

	if ((p->p3_flags & PF_PROJECTED) || (p->p3_codes & CC_BEHIND))
		return;

	if (CheckMulDiv (&tx, p->p3_x, xCanvW2, p->p3_z) && 
		 CheckMulDiv (&ty, p->p3_y, xCanvH2, p->p3_z)) {
		p->p3_sx = xCanvW2 + tx;
		p->p3_sy = xCanvH2 - ty;
		p->p3_flags |= PF_PROJECTED;
	}
	else
		p->p3_flags |= PF_OVERFLOW;
#else
	double fz;
	
	if ((p->p3_flags & PF_PROJECTED) || (p->p3_codes & CC_BEHIND))
		return;
	
	if ( p->p3_z <= 0 )	{
		p->p3_flags |= PF_OVERFLOW;
		return;
	}

	fz = f2fl(p->p3_z);
	p->p3_sx = fl2f(fxCanvW2 + (f2fl(p->p3_x)*fxCanvW2 / fz);
	p->p3_sy = fl2f(fxCanvH2 - (f2fl(p->p3_y)*fxCanvH2 / fz);

	p->p3_flags |= PF_PROJECTED;
#endif
}

// -----------------------------------------------------------------------------------
//from a 2d point, compute the vector through that point
void G3Point2Vec(vms_vector *v,short sx,short sy)
{
	vms_vector tempv;
	vms_matrix tempm;

	tempv.x =  fixmuldiv(fixdiv((sx<<16) - xCanvW2,xCanvW2),viewInfo.scale.z,viewInfo.scale.x);
	tempv.y = -fixmuldiv(fixdiv((sy<<16) - xCanvH2,xCanvH2),viewInfo.scale.z,viewInfo.scale.y);
	tempv.z = f1_0;

	VmVecNormalize(&tempv);

	VmCopyTransposeMatrix(&tempm,&viewInfo.unscaledView);

	VmVecRotate(v,&tempv,&tempm);

}

// -----------------------------------------------------------------------------------
//delta rotation functions
vms_vector *G3RotateDeltaX(vms_vector *dest,fix dx)
{
	dest->x = fixmul(viewInfo.view.rvec.x,dx);
	dest->y = fixmul(viewInfo.view.uvec.x,dx);
	dest->z = fixmul(viewInfo.view.fvec.x,dx);

	return dest;
}

// -----------------------------------------------------------------------------------

vms_vector *G3RotateDeltaY(vms_vector *dest,fix dy)
{
	dest->x = fixmul(viewInfo.view.rvec.y,dy);
	dest->y = fixmul(viewInfo.view.uvec.y,dy);
	dest->z = fixmul(viewInfo.view.fvec.y,dy);

	return dest;
}

// -----------------------------------------------------------------------------------

vms_vector *G3RotateDeltaZ(vms_vector *dest,fix dz)
{
	dest->x = fixmul(viewInfo.view.rvec.z,dz);
	dest->y = fixmul(viewInfo.view.uvec.z,dz);
	dest->z = fixmul(viewInfo.view.fvec.z,dz);

	return dest;
}

// -----------------------------------------------------------------------------------

vms_vector *G3RotateDeltaVec(vms_vector *dest,vms_vector *src)
{
	return VmVecRotate(dest,src,&viewInfo.view);
}

// -----------------------------------------------------------------------------------

ubyte G3AddDeltaVec (g3s_point *dest, g3s_point *src, vms_vector *vDelta)
{
VmVecAdd (&dest->p3_vec, &src->p3_vec, vDelta);
dest->p3_flags = 0;		//not projected
return G3EncodePoint (dest);
}

// -----------------------------------------------------------------------------------
//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth(vms_vector *pnt)
{
#ifdef _WIN32
	QLONG q = mul64 (pnt->x - viewInfo.position.x, viewInfo.view.fvec.x);
	q += mul64 (pnt->y - viewInfo.position.y, viewInfo.view.fvec.y);
	q += mul64 (pnt->z - viewInfo.position.z, viewInfo.view.fvec.z);
	return (fix) (q >> 16);
#else
	quadint q;

	q.low=q.high=0;
	fixmulaccum(&q,(pnt->x - viewInfo.position.x),viewInfo.view.fvec.x);
	fixmulaccum(&q,(pnt->y - viewInfo.position.y),viewInfo.view.fvec.y);
	fixmulaccum(&q,(pnt->z - viewInfo.position.z),viewInfo.view.fvec.z);
	return fixquadadjust(&q);
#endif
}

// -----------------------------------------------------------------------------------
//eof

