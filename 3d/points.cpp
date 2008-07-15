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
COPYRIGHT 1993-1998PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
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
	tQuadInt q,qt;

	q.low=q.high=0;
	FixMulAccum (&q,a,b);
	qt = q;
	if (qt.high < 0)
		FixQuadNegate (&qt);
	qt.high *= 2;
	if (qt.low > 0x7fff)
		qt.high++;
	if (qt.high >= c)
		return 0;
	else {
		*r = FixDivQuadLong (q.low,q.high,c);
		return 1;
	}
#endif
}

// -----------------------------------------------------------------------------------
//projects a point
void G3ProjectPoint (g3sPoint *p)
{
if ((p->p3_flags & PF_PROJECTED) || (p->p3_codes & CC_BEHIND))
	return;
fVector3	v;
v.p.x = f2fl (p->p3_vec.p.x) * viewInfo.scalef.p.x;
v.p.y = f2fl (p->p3_vec.p.y) * viewInfo.scalef.p.y;
v.p.z = f2fl (p->p3_vec.p.z) * viewInfo.scalef.p.z;
p->p3_screen.x = (fix) (fxCanvW2 + v.p.x * fxCanvW2 / v.p.z);
p->p3_screen.y = (fix) (fxCanvH2 - v.p.y * fxCanvH2 / v.p.z);
p->p3_flags |= PF_PROJECTED;
}

// -----------------------------------------------------------------------------------
//from a 2d point, compute the vector through that point
void G3Point2Vec (vmsVector *v,short sx,short sy)
{
	vmsVector tempv;
	vmsMatrix tempm;

tempv.p.x =  FixMulDiv (FixDiv ((sx<<16) - xCanvW2,xCanvW2),viewInfo.scale.p.z,viewInfo.scale.p.x);
tempv.p.y = -FixMulDiv (FixDiv ((sy<<16) - xCanvH2,xCanvH2),viewInfo.scale.p.z,viewInfo.scale.p.y);
tempv.p.z = f1_0;
VmVecNormalize (&tempv);
VmCopyTransposeMatrix (&tempm,&viewInfo.view [1]);
VmVecRotate (v,&tempv,&tempm);
}

// -----------------------------------------------------------------------------------
//delta rotation functions
vmsVector *G3RotateDeltaX (vmsVector *dest,fix dx)
{
	dest->p.x = FixMul (viewInfo.view [0].rVec.p.x,dx);
	dest->p.y = FixMul (viewInfo.view [0].uVec.p.x,dx);
	dest->p.z = FixMul (viewInfo.view [0].fVec.p.x,dx);

	return dest;
}

// -----------------------------------------------------------------------------------

vmsVector *G3RotateDeltaY (vmsVector *dest,fix dy)
{
	dest->p.x = FixMul (viewInfo.view [0].rVec.p.y,dy);
	dest->p.y = FixMul (viewInfo.view [0].uVec.p.y,dy);
	dest->p.z = FixMul (viewInfo.view [0].fVec.p.y,dy);

	return dest;
}

// -----------------------------------------------------------------------------------

vmsVector *G3RotateDeltaZ (vmsVector *dest,fix dz)
{
	dest->p.x = FixMul (viewInfo.view [0].rVec.p.z,dz);
	dest->p.y = FixMul (viewInfo.view [0].uVec.p.z,dz);
	dest->p.z = FixMul (viewInfo.view [0].fVec.p.z,dz);

	return dest;
}

// -----------------------------------------------------------------------------------

vmsVector *G3RotateDeltaVec (vmsVector *dest,vmsVector *src)
{
	return VmVecRotate (dest,src,&viewInfo.view [0]);
}

// -----------------------------------------------------------------------------------

ubyte G3AddDeltaVec (g3sPoint *dest, g3sPoint *src, vmsVector *vDelta)
{
VmVecAdd (&dest->p3_vec, &src->p3_vec, vDelta);
dest->p3_flags = 0;		//not projected
return G3EncodePoint (dest);
}

// -----------------------------------------------------------------------------------
//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth (vmsVector *pnt)
{
#ifdef _WIN32
	QLONG q = mul64 (pnt->p.x - viewInfo.pos.p.x, viewInfo.view [0].fVec.p.x);
	q += mul64 (pnt->p.y - viewInfo.pos.p.y, viewInfo.view [0].fVec.p.y);
	q += mul64 (pnt->p.z - viewInfo.pos.p.z, viewInfo.view [0].fVec.p.z);
	return (fix) (q >> 16);
#else
	tQuadInt q;

	q.low=q.high=0;
	FixMulAccum (&q, (pnt->p.x - viewInfo.pos.p.x),viewInfo.view [0].fVec.p.x);
	FixMulAccum (&q, (pnt->p.y - viewInfo.pos.p.y),viewInfo.view [0].fVec.p.y);
	FixMulAccum (&q, (pnt->p.z - viewInfo.pos.p.z),viewInfo.view [0].fVec.p.z);
	return FixQuadAdjust (&q);
#endif
}

// -----------------------------------------------------------------------------------
//eof

