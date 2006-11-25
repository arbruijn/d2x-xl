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
void G3ProjectPoint(g3sPoint *p)
{
#ifndef __powerc
	fix tx,ty;

	if ((p->p3Flags & PF_PROJECTED) || (p->p3_codes & CC_BEHIND))
		return;

	if (CheckMulDiv (&tx, p->p3_x, xCanvW2, p->p3_z) && 
		 CheckMulDiv (&ty, p->p3_y, xCanvH2, p->p3_z)) {
		p->p3_sx = xCanvW2 + tx;
		p->p3_sy = xCanvH2 - ty;
		p->p3Flags |= PF_PROJECTED;
	}
	else
		p->p3Flags |= PF_OVERFLOW;
#else
	double fz;
	
	if ((p->p3Flags & PF_PROJECTED) || (p->p3_codes & CC_BEHIND))
		return;
	
	if ( p->p3_z <= 0 )	{
		p->p3Flags |= PF_OVERFLOW;
		return;
	}

	fz = f2fl(p->p3_z);
	p->p3_sx = fl2f(fxCanvW2 + (f2fl(p->p3_x)*fxCanvW2 / fz);
	p->p3_sy = fl2f(fxCanvH2 - (f2fl(p->p3_y)*fxCanvH2 / fz);

	p->p3Flags |= PF_PROJECTED;
#endif
}

// -----------------------------------------------------------------------------------
//from a 2d point, compute the vector through that point
void G3Point2Vec(vmsVector *v,short sx,short sy)
{
	vmsVector tempv;
	vmsMatrix tempm;

	tempv.x =  FixMulDiv(FixDiv((sx<<16) - xCanvW2,xCanvW2),viewInfo.scale.z,viewInfo.scale.x);
	tempv.y = -FixMulDiv(FixDiv((sy<<16) - xCanvH2,xCanvH2),viewInfo.scale.z,viewInfo.scale.y);
	tempv.z = f1_0;

	VmVecNormalize(&tempv);

	VmCopyTransposeMatrix(&tempm,&viewInfo.view [1]);

	VmVecRotate(v,&tempv,&tempm);

}

// -----------------------------------------------------------------------------------
//delta rotation functions
vmsVector *G3RotateDeltaX(vmsVector *dest,fix dx)
{
	dest->x = FixMul(viewInfo.view [0].rVec.x,dx);
	dest->y = FixMul(viewInfo.view [0].uVec.x,dx);
	dest->z = FixMul(viewInfo.view [0].fVec.x,dx);

	return dest;
}

// -----------------------------------------------------------------------------------

vmsVector *G3RotateDeltaY(vmsVector *dest,fix dy)
{
	dest->x = FixMul(viewInfo.view [0].rVec.y,dy);
	dest->y = FixMul(viewInfo.view [0].uVec.y,dy);
	dest->z = FixMul(viewInfo.view [0].fVec.y,dy);

	return dest;
}

// -----------------------------------------------------------------------------------

vmsVector *G3RotateDeltaZ(vmsVector *dest,fix dz)
{
	dest->x = FixMul(viewInfo.view [0].rVec.z,dz);
	dest->y = FixMul(viewInfo.view [0].uVec.z,dz);
	dest->z = FixMul(viewInfo.view [0].fVec.z,dz);

	return dest;
}

// -----------------------------------------------------------------------------------

vmsVector *G3RotateDeltaVec(vmsVector *dest,vmsVector *src)
{
	return VmVecRotate(dest,src,&viewInfo.view [0]);
}

// -----------------------------------------------------------------------------------

ubyte G3AddDeltaVec (g3sPoint *dest, g3sPoint *src, vmsVector *vDelta)
{
VmVecAdd (&dest->p3_vec, &src->p3_vec, vDelta);
dest->p3Flags = 0;		//not projected
return G3EncodePoint (dest);
}

// -----------------------------------------------------------------------------------
//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth(vmsVector *pnt)
{
#ifdef _WIN32
	QLONG q = mul64 (pnt->x - viewInfo.pos.x, viewInfo.view [0].fVec.x);
	q += mul64 (pnt->y - viewInfo.pos.y, viewInfo.view [0].fVec.y);
	q += mul64 (pnt->z - viewInfo.pos.z, viewInfo.view [0].fVec.z);
	return (fix) (q >> 16);
#else
	quadint q;

	q.low=q.high=0;
	fixmulaccum(&q,(pnt->x - viewInfo.pos.x),viewInfo.view [0].fVec.x);
	fixmulaccum(&q,(pnt->y - viewInfo.pos.y),viewInfo.view [0].fVec.y);
	fixmulaccum(&q,(pnt->z - viewInfo.pos.z),viewInfo.view [0].fVec.z);
	return fixquadadjust(&q);
#endif
}

// -----------------------------------------------------------------------------------
//eof

