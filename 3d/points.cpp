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

#define VIS_CULLING 1

ubyte ProjectPoint (CFloatVector3& v, tScreenPos& s, ubyte flags, ubyte codes)
{
if ((flags & PF_PROJECTED) || (codes & CC_BEHIND))
	return flags;
CFloatVector3 w = transformation.m_info.projection * v;
s.x = fix (fxCanvW2 + double (w [X]) * fxCanvW2 / w [Z]);
s.y = fix (fxCanvH2 - double (w [Y]) * fxCanvH2 / w [Z]);
return flags | PF_PROJECTED;
}

// -----------------------------------------------------------------------------------

ubyte ProjectPoint (CFixVector& v, tScreenPos& s, ubyte flags, ubyte codes)
{
CFloatVector3 h;
h.Assign (v);
return ProjectPoint (h, s, flags);
}

// -----------------------------------------------------------------------------------

void G3ProjectPoint (g3sPoint *p)
{
p->p3_flags = ProjectPoint (p->p3_vec, p->p3_screen, p->p3_flags, p->p3_codes);
}

// -----------------------------------------------------------------------------------
//from a 2d point, compute the vector through that point
void G3Point2Vec (CFixVector *v,short sx,short sy)
{
	CFixVector h;
	CFixMatrix m;

h [X] =  FixMulDiv (FixDiv ((sx<<16) - xCanvW2, xCanvW2), transformation.m_info.scale [Z], transformation.m_info.scale [X]);
h [Y] = -FixMulDiv (FixDiv ((sy<<16) - xCanvH2, xCanvH2), transformation.m_info.scale [Z], transformation.m_info.scale [Y]);
h [Z] = I2X (1);
CFixVector::Normalize (h);
m = transformation.m_info.view [1].Transpose();
*v = m * h;
}

// -----------------------------------------------------------------------------------

ubyte G3AddDeltaVec (g3sPoint *dest, g3sPoint *src, CFixVector *vDelta)
{
dest->p3_vec = src->p3_vec + *vDelta;
dest->p3_flags = 0;		//not projected
return G3EncodePoint (dest);
}

// -----------------------------------------------------------------------------------
//calculate the depth of a point - returns the z coord of the rotated point
fix G3CalcPointDepth (const CFixVector& pnt)
{
#ifdef _WIN32
	QLONG q = mul64 (pnt [X] - transformation.m_info.pos [X], transformation.m_info.view [0].FVec () [X]);
	q += mul64 (pnt [Y] - transformation.m_info.pos [Y], transformation.m_info.view [0].FVec () [Y]);
	q += mul64 (pnt [Z] - transformation.m_info.pos [Z], transformation.m_info.view [0].FVec () [Z]);
	return (fix) (q >> 16);
#else
	tQuadInt q;

	q.low=q.high=0;
	FixMulAccum (&q, (pnt [X] - transformation.m_info.pos [X]),transformation.m_info.view [0].FVec () [X]);
	FixMulAccum (&q, (pnt [Y] - transformation.m_info.pos [Y]),transformation.m_info.view [0].FVec () [Y]);
	FixMulAccum (&q, (pnt [Z] - transformation.m_info.pos [Z]),transformation.m_info.view [0].FVec () [Z]);
	return FixQuadAdjust (&q);
#endif
}

// -----------------------------------------------------------------------------------
//eof

