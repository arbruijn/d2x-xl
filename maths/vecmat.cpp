/* $Id: vecmat.c, v 1.6 2004/05/12 07:31:37 btb Exp $ */
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
static char rcsid[] = "$Id: vecmat.c, v 1.6 2004/05/12 07:31:37 btb Exp $";
#endif

#include <stdlib.h>
#include <math.h>           // for sqrt

#include "maths.h"
#include "vecmat.h"
#include "inferno.h"
#include "error.h"

#define EXACT_VEC_MAG	1
#ifdef _DEBUG
#	define ENABLE_SSE		0
#elif defined (_MSC_VER)
#	define ENABLE_SSE		1
#else
#	define ENABLE_SSE		0
#endif

#ifndef ASM_VECMAT
vmsVector vmdZeroVector = {{0, 0, 0}};
vmsMatrix vmdIdentityMatrix = {{{f1_0, 0, 0}}, 
                               {{0, f1_0, 0}}, 
                               {{0, 0, f1_0}}};

// ------------------------------------------------------------------------
//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecInc() if so
#if !INLINE_VEC_ADD
vmsVector *VmVecAdd (vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
dest->p.x = src0->p.x + src1->p.x;
dest->p.y = src0->p.y + src1->p.y;
dest->p.z = src0->p.z + src1->p.z;
return dest;
}

// ------------------------------------------------------------------------
//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecDec() if so
vmsVector *VmVecSub(vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
dest->p.x = src0->p.x - src1->p.x;
dest->p.y = src0->p.y - src1->p.y;
dest->p.z = src0->p.z - src1->p.z;
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecSubf (fVector *dest, fVector *src0, fVector *src1)
{
dest->p.x = src0->p.x - src1->p.x;
dest->p.y = src0->p.y - src1->p.y;
dest->p.z = src0->p.z - src1->p.z;
return dest;
}

// ------------------------------------------------------------------------
//adds one vector to another. returns ptr to dest
//dest can equal source
vmsVector *VmVecInc (vmsVector *dest, vmsVector *src)
{
dest->p.x += src->p.x;
dest->p.y += src->p.y;
dest->p.z += src->p.z;
return dest;
}

// ------------------------------------------------------------------------
//subs one vector from another, returns ptr to dest
//dest can equal source
vmsVector *VmVecDec(vmsVector *dest, vmsVector *src)
{
dest->p.x -= src->p.x;
dest->p.y -= src->p.y;
dest->p.z -= src->p.z;
return dest;
}
#endif

// ------------------------------------------------------------------------
//averages two vectors. returns ptr to dest
//dest can equal either source
vmsVector *VmVecAvg(vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
dest->p.x = (src0->p.x + src1->p.x) / 2;
dest->p.y = (src0->p.y + src1->p.y) / 2;
dest->p.z = (src0->p.z + src1->p.z) / 2;
return dest;
}

// ------------------------------------------------------------------------
//averages four vectors. returns ptr to dest
//dest can equal any source
vmsVector *VmVecAvg4(vmsVector *dest, 
								vmsVector *src0, vmsVector *src1, 
								vmsVector *src2, vmsVector *src3)
{
dest->p.x = (src0->p.x + src1->p.x + src2->p.x + src3->p.x) / 4;
dest->p.y = (src0->p.y + src1->p.y + src2->p.y + src3->p.y) / 4;
dest->p.z = (src0->p.z + src1->p.z + src2->p.z + src3->p.z) / 4;
return dest;
}

// ------------------------------------------------------------------------
//averages two vectors. returns ptr to dest
//dest can equal either source
fVector *VmVecAvgf (fVector *dest, fVector *src0, fVector *src1)
{
dest->p.x = (src0->p.x + src1->p.x) / 2;
dest->p.y = (src0->p.y + src1->p.y) / 2;
dest->p.z = (src0->p.z + src1->p.z) / 2;
return dest;
}

// ------------------------------------------------------------------------
//scales a vector in place.  returns ptr to vector
vmsVector *VmVecScale (vmsVector *dest, fix s)
{
dest->p.x = FixMul (dest->p.x, s);
dest->p.y = FixMul (dest->p.y, s);
dest->p.z = FixMul (dest->p.z, s);
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecScalef (fVector *dest, fVector *src, float scale)
{
dest->p.x = src->p.x * scale;
dest->p.y = src->p.y * scale;
dest->p.z = src->p.z * scale;
return dest;
}

// ------------------------------------------------------------------------
//scales and copies a vector.  returns ptr to dest
vmsVector *VmVecCopyScale (vmsVector *dest, vmsVector *src, fix s)
{
dest->p.x = FixMul (src->p.x, s);
dest->p.y = FixMul (src->p.y, s);
dest->p.z = FixMul (src->p.z, s);
return dest;
}

// ------------------------------------------------------------------------
//scales a vector, adds it to another, and stores in a 3rd vector
//dest = src1 + k * src2
vmsVector *VmVecScaleAdd (vmsVector *dest, vmsVector *src1, vmsVector *src2, fix k)
{
dest->p.x = src1->p.x + FixMul (src2->p.x, k);
dest->p.y = src1->p.y + FixMul (src2->p.y, k);
dest->p.z = src1->p.z + FixMul (src2->p.z, k);
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecScaleAddf (fVector *dest, fVector *src1, fVector *src2, float scale)
{
dest->p.x = src1->p.x + src2->p.x * scale;
dest->p.y = src1->p.y + src2->p.y * scale;
dest->p.z = src1->p.z + src2->p.z * scale;
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecScaleAddf4 (fVector *dest, fVector *src1, fVector *src2, float scale)
{
dest->c.r = src1->c.r + src2->c.r * scale;
dest->c.g = src1->c.g + src2->c.g * scale;
dest->c.b = src1->c.b + src2->c.b * scale;
dest->c.a = src1->c.a + src2->c.a * scale;
return dest;
}

// ------------------------------------------------------------------------
//scales a vector and adds it to another
//dest += k * src
vmsVector *VmVecScaleInc (vmsVector *dest, vmsVector *src, fix k)
{
dest->p.x += FixMul (src->p.x, k);
dest->p.y += FixMul (src->p.y, k);
dest->p.z += FixMul (src->p.z, k);
return dest;
}

// ------------------------------------------------------------------------
//scales a vector and adds it to another
//dest += k * src
fVector *VmVecScaleIncf3 (fVector *dest, fVector *src, float scale)
{
dest->p.x += src->p.x * scale;
dest->p.y += src->p.y * scale;
dest->p.z += src->p.z * scale;
return dest;
}

// ------------------------------------------------------------------------
//scales a vector in place, taking n/d for scale.  returns ptr to vector
//dest *= n/d
vmsVector *VmVecScaleFrac (vmsVector *dest, fix n, fix d)
{
#if 1 // DPH: Kludge: this was overflowing a lot, so I made it use the FPU.
double nd = (double) n / (double) d;
dest->p.x = (fix) ((double) dest->p.x * nd);
dest->p.y = (fix) ((double) dest->p.y * nd);
dest->p.z = (fix) ((double) dest->p.z * nd);
#else
dest->p.x = FixMulDiv (src->p.x, n, d);
dest->p.y = FixMulDiv (src->p.y, n, d);
dest->p.z = FixMulDiv (src->p.z, n, d);
#endif
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecMulf (fVector *dest, fVector *src0, fVector *src1)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
#if defined (_WIN32)
	_asm {
		mov		esi,src0
		movups	xmm0,[esi]
		mov		esi,src1
		movups	xmm1,[esi]
		mulps		xmm0,xmm1
		mov		esi,dest
		movups	[esi],xmm0
		}
#elif defined (__unix__)
	asm (
		"mov		%1,%%rsi\n\t"
		"movups	(%%rsi),%%xmm0\n\t"
		"mov		%2,%%rsi\n\t"
		"movups	(%%rsi),%%xmm1\n\t"
		"mulps	%%xmm1,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (dest)
		: "m" (src0), "m" (src1)
		: "%rsi"
		);
#endif
	return dest;
	}
#endif
dest->p.x = src0->p.x * src1->p.x;
dest->p.y = src0->p.y * src1->p.y;
dest->p.z = src0->p.z * src1->p.z;
return dest;
}

// ------------------------------------------------------------------------

fix VmVecDotProd (vmsVector *v0, vmsVector *v1)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
		fVector	v0h, v1h;

	VmVecFixToFloat (&v0h, v0);
	VmVecFixToFloat (&v1h, v1);
#if defined (_WIN32)
	_asm {
		movups	xmm0,v0h
		movups	xmm1,v1h
		mulps		xmm0,xmm1
		movups	v0h,xmm0
		}
#elif defined (__unix__)
	asm (
		"movups	%0,%%xmm0\n\t"
		"movups	%1,%%xmm1\n\t"
		"mulps	%%xmm1,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (v0h)
		: "m" (v0h), "m" (v1h)
		);
#endif
	return (fix) ((v0h.p.x + v0h.p.y + v0h.p.z) * 65536);
	}
#endif
if (gameOpts->render.nMathFormat == 2)
	return (fix) (((double) v0->p.x * (double) v1->p.x + 
						(double) v0->p.y * (double) v1->p.y + 
						(double) v0->p.z * (double) v1->p.z) 
					  / 65536.0);
else {
	QLONG q = mul64 (v0->p.x, v1->p.x);
	q += mul64 (v0->p.y, v1->p.y);
	q += mul64 (v0->p.z, v1->p.z);
	return (fix) (q >> 16); 
	}
}

// ------------------------------------------------------------------------

float VmVecDotf (fVector *v0, fVector *v1)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
		fVector	v;

#if defined (_WIN32)
	_asm {
		mov		esi,v0
		movups	xmm0,[esi]
		mov		esi,v1
		movups	xmm1,[esi]
		mulps		xmm0,xmm1
		movups	v,xmm0
		}
#elif defined (__unix__)
	asm (
		"mov		%1,%%rsi\n\t"
		"movups	(%%rsi),%%xmm0\n\t"
		"mov		%2,%%rsi\n\t"
		"movups	(%%rsi),%%xmm1\n\t"
		"mulps	%%xmm1,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (v)
		: "m" (v0), "m" (v1)
		: "%rsi"
		);
#endif
	return v.p.x + v.p.y + v.p.z;
	}
#endif
return v0->p.x * v1->p.x + v0->p.y * v1->p.y + v0->p.z * v1->p.z;
}

// ------------------------------------------------------------------------

fix VmVecDot3(fix x, fix y, fix z, vmsVector *v)
{
if (gameOpts->render.nMathFormat == 2)
	return (fix) (((double) x * (double) v->p.x + (double) y * (double) v->p.y + (double) z * (double) v->p.z) / 65536.0);
else {
	QLONG q = mul64 (x, v->p.x);
	q += mul64 (y, v->p.y);
	q += mul64 (z, v->p.z);
	return (fix) (q >> 16);
	}
}

// ------------------------------------------------------------------------

#include <math.h>

inline double sqrd (double d)
{
return d * d;
}

inline double sqrf (float f)
{
return f * f;
}

// ------------------------------------------------------------------------

inline unsigned short sqrt32 (unsigned long a)
{
	unsigned long rem = 0;
	unsigned long root = 0;
	int i;

for (i = 0; i < 16; i++) {
	root <<= 1;
   rem <<= 2;
	rem += (a >> 30);
   a <<= 2;
   if (root < rem) {
		rem -= ++root;
		root++;
		}
  }
return (unsigned short) (root >> 1);
}

// ------------------------------------------------------------------------

inline unsigned int sqrt64 (unsigned QLONG a)
{
if (gameOpts->render.nMathFormat == 2)
	return (unsigned int) sqrt ((double) ((signed QLONG) a));
else if (gameOpts->render.nMathFormat == 1) {
	if (a <= 0xFFFFFFFF)
		return sqrt32 ((unsigned long) a);
	else {
		unsigned QLONG rem = 0;
		unsigned QLONG root = 0;
		int i;

		for (i = 0; i < 32; i++) {
			root <<= 1;
			rem <<= 2;
			rem += (a >> 62);
			a <<= 2;
			if (root < rem) {
				rem -= ++root;
				root++;
				}
		  }
		  return (unsigned int) (root >> 1);
		}
	}
else {
	return QuadSqrt ((u_int32_t) (a & 0xFFFFFFFF), (u_int32_t) (a >> 32));
	}
}

// ------------------------------------------------------------------------
//returns magnitude of a vector
fix VmVecMag (vmsVector *v)
{
#if 0
if (gameOpts->render.nMathFormat == 2) {
#if ENABLE_SSE
		fVector	h;

	VmVecFixToFloat (&h, v);
#if defined (_WIN32)
	__asm {
		movups	xmm0,h
		mulps		xmm0,xmm0
		movups	h,xmm0
		}
#elif defined (__unix__)
	asm (
		"mov		%0,%%rsi\n\t"
		"movups	(%%rsi),%%xmm0\n\t"
		"mulps	%%xmm0,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (h)
		: "m" (h)
		: "%rsi"
		);
#endif
	return (fix) (sqrt (h.p.x + h.p.y + h.p.z) * 65536);
#else
	return fl2f (sqrt (sqrd ((double) f2fl (v->p.x)) + sqrd ((double) f2fl (v->p.y)) + sqrd ((double) f2fl (v->p.z)))); 
#endif
	}
else 
#endif
{
#if 1//def _WIN32
	QLONG h = mul64 (v->p.x, v->p.x);
	h += mul64 (v->p.y, v->p.y);
	h += mul64 (v->p.z, v->p.z);
	return (fix) sqrt64 ((unsigned QLONG) h);
#else
	tQuadInt q;
	q.low = q.high = 0;
	FixMulAccum (&q, v->p.x, v->p.x);
	FixMulAccum (&q, v->p.y, v->p.y);
	FixMulAccum (&q, v->p.z, v->p.z);
	return (fix) QuadSqrt (q.low, q.high);
#endif
	}
}

// ------------------------------------------------------------------------

float VmVecMagf (fVector *v)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
		fVector	h;
#if defined (_WIN32)
	__asm {
		mov		esi,v
		movups	xmm0,[esi]
		mulps		xmm0,xmm0
		movups	h,xmm0
		}
#elif defined (__unix__)
	asm (
		"mov		%1,%%rsi\n\t"
		"movups	(%%rsi),%%xmm0\n\t"
		"mulps	%%xmm0,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (h)
		: "m" (v)
		: "%rsi"
		);
#endif
	return (float) sqrt (h.p.x + h.p.y + h.p.z);
	}
#endif
return (float) sqrt (sqrd ((double) v->p.x) + sqrd ((double) v->p.y) + sqrd ((double) v->p.z)); 
}

// ------------------------------------------------------------------------
//computes the distance between two points. (does sub and mag)
#if !INLINE_VEC_ADD

fix VmVecDist(vmsVector *v0, vmsVector *v1)
{
	vmsVector d;

VmVecSub (&d, v0, v1);
return VmVecMag (&d);
}

#endif

// ------------------------------------------------------------------------

float VmVecDistf (fVector *v0, fVector *v1)
{
	fVector	t;

#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
#if defined (_WIN32)
	__asm {
		mov		esi,v0
		movups	xmm0,[esi]
		mov		esi,v1
		movups	xmm1,[esi]
		subps		xmm0,xmm1
		mulps		xmm0,xmm0
		movups	t,xmm0
		}
#elif defined (__unix__)
	asm (
		"mov		%1,%%rsi\n\t"
		"movups	(%%rsi),%%xmm0\n\t"
		"mov		%2,%%rsi\n\t"
		"movups	(%%rsi),%%xmm1\n\t"
		"subps	%%xmm1,%%xmm0\n\t"
		"mulps	%%xmm0,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (t)
		: "m" (v0), "m" (v1)
		: "%rsi"
		);
#endif
	return (float) sqrt (t.p.x + t.p.y + t.p.z);
	}
#endif
return VmVecMagf (VmVecSubf (&t, v0, v1));
}

// ------------------------------------------------------------------------
//computes an approximation of the distance between two points.
//uses dist = largest + next_largest*3/8 + smallest*3/16
#if 0//QUICK_VEC_MATH
fix VmVecDistQuick (vmsVector *v0, vmsVector *v1)
{
return FixVecMagQuick (v0->p.x - v1->p.x, v0->p.y - v1->p.y, v0->p.z - v1->p.z);
}
#endif

// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec
fix VmVecCopyNormalize (vmsVector *dest, vmsVector *src)
{
fix m = VmVecMag (src);
if (m) {
	dest->p.x = FixDiv (src->p.x, m);
	dest->p.y = FixDiv (src->p.y, m);
	dest->p.z = FixDiv (src->p.z, m);
	}
else
	dest->p.x =
	dest->p.y =
	dest->p.z = 0;
return m;
}

// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec
float VmVecNormalizef (fVector *dest, fVector *src)
{
float m = VmVecMagf (src);
if (m) {
	dest->p.x = src->p.x / m;
	dest->p.y = src->p.y / m;
	dest->p.z = src->p.z / m;
	}
return m;
}

// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec. uses approx mag

#if EXACT_VEC_MAG
fix VmVecCopyNormalizeQuick(vmsVector *dest, vmsVector *src)
{
fix m = VmVecMag (src);
if (m) {
	dest->p.x = FixDiv(src->p.x, m);
	dest->p.y = FixDiv(src->p.y, m);
	dest->p.z = FixDiv(src->p.z, m);
	}
return m;
}

#else
//these routines use an approximation for 1/sqrt

// ------------------------------------------------------------------------
//returns approximation of 1/magnitude of a vector
fix VmVecInvMag(vmsVector *v)
{
#if FLOAT_COORD
return 1.0f / VmVecMag (v);
#else
#	if 1//def _WIN32
u_int32_t l, h;
QLONG q = mul64 (v->p.x, v->p.x);
q += mul64 (v->p.y, v->p.y);
q += mul64 (v->p.z, v->p.z);
h = q >> 32;
if (h >= 0x800000)
	return (FixISqrt (h) >> 8);
l = q & (QLONG) 0xFFFFFFFF;
if (!h)
	return FixISqrt (l);
return (FixISqrt ((h << 8) + (l >> 24)) >> 4);
#	else
tQuadInt q;
q.low = q.high = 0;
FixMulAccum(&q, v->p.x, v->p.x);
FixMulAccum(&q, v->p.y, v->p.y);
FixMulAccum(&q, v->p.z, v->p.z);
if (!q.high)
	return FixISqrt(FixQuadAdjust(&q));
if (q.high >= 0x800000)
	return (FixISqrt(q.high) >> 8);
return (FixISqrt((q.high<<8) + (q.low>>24)) >> 4);
#	endif
#endif
}

// ------------------------------------------------------------------------
//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
fix VmVecCopyNormalizeQuick (vmsVector *dest, vmsVector *src)
{
fix im = VmVecInvMag (src);
dest->p.x = FixMul (src->p.x, im);
dest->p.y = FixMul (src->p.y, im);
dest->p.z = FixMul (src->p.z, im);
return im;
}

#endif

// ------------------------------------------------------------------------
//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
fix VmVecNormalizeQuick (vmsVector *v)
{
return VmVecCopyNormalizeQuick(v, v);
}

// ------------------------------------------------------------------------
//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns 1/mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix VmVecNormalizedDirQuick(vmsVector *dest, vmsVector *end, vmsVector *start)
{
return VmVecNormalizeQuick (VmVecSub (dest, end, start));
}

// ------------------------------------------------------------------------
//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix VmVecNormalizedDir(vmsVector *dest, vmsVector *end, vmsVector *start)
{
return VmVecNormalize (VmVecSub (dest, end, start));
}

// ------------------------------------------------------------------------
//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
vmsVector *VmVecNormal (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
VmVecNormalize (VmVecPerp (dest, p0, p1, p2));
return dest;
}

vmsVector *VmVecNormalChecked (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
VmVecNormalize (VmVecPerpChecked (dest, p0, p1, p2));
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecCrossProdf (fVector *dest, fVector *src0, fVector *src1)
{
dest->p.x = src0->p.y * src1->p.z - src0->p.z * src1->p.y;
dest->p.y = src0->p.z * src1->p.x - src0->p.x * src1->p.z;
dest->p.z = src0->p.x * src1->p.y - src0->p.y * src1->p.x;
return dest;
}

// ------------------------------------------------------------------------
//computes non-normalized surface normal from three points. 
//returns ptr to dest
//dest CANNOT equal either source
fVector *VmVecPerpf (fVector *dest, fVector *p0, fVector *p1, fVector *p2)
{
	fVector t0, t1;

VmVecSubf (&t0, p1, p0);
VmVecSubf (&t1, p2, p1);
return VmVecCrossProdf (dest, &t0, &t1);
}

// ------------------------------------------------------------------------
//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
fVector *VmVecNormalf (fVector *dest, fVector *p0, fVector *p1, fVector *p2)
{
VmVecNormalizef (dest, VmVecPerpf (dest, p0, p1, p2));
return dest;
}

// ------------------------------------------------------------------------
//make sure a vector is reasonably sized to go into a cross product

void CheckVec (vmsVector *vp)
{
	fix check;
	int cnt = 0;
	vmsVector v = *vp;

if (!(check = labs (v.p.x) | labs (v.p.y) | labs (v.p.z)))
	return;
if (check & 0xfffc0000) {		//too big
	while (check & 0xfff00000) {
		cnt += 4;
		check >>= 4;
		}
	while (check & 0xfffc0000) {
		cnt += 2;
		check >>= 2;
		}
	}
else if (!(check & 0xffff8000)) {		//yep, too small
	while (!(check & 0xfffff000)) {
		cnt += 4;
		check <<= 4;
		}
	while (!(check & 0xffff8000)) {
		cnt += 2;
		check <<= 2;
		}
	}
else
	return;
v.p.x >>= cnt;
v.p.y >>= cnt;
v.p.z >>= cnt;
*vp = v;
}

// ------------------------------------------------------------------------
//computes cross product of two vectors. 
//Note: this magnitude of the resultant vector is the
//product of the magnitudes of the two source vectors.  This means it is
//quite easy for this routine to overflow and underflow.  Be careful that
//your inputs are ok.
//#ifndef __powerc

vmsVector *VmVecCrossProd (vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
#if 0
dest->p.x = (fix) (((double) src0->p.y * (double) src1->p.z - (double) src0->p.z * (double) src1->p.y) / 65536.0);
dest->p.y = (fix) (((double) src0->p.z * (double) src1->p.x - (double) src0->p.x * (double) src1->p.z) / 65536.0);
dest->p.z = (fix) (((double) src0->p.x * (double) src1->p.y - (double) src0->p.y * (double) src1->p.x) / 65536.0);
#else
QLONG q = mul64 (src0->p.y, src1->p.z);
Assert(dest!=src0 && dest!=src1);
q += mul64 (-src0->p.z, src1->p.y);
dest->p.x = (fix) (q >> 16);
q = mul64 (src0->p.z, src1->p.x);
q += mul64 (-src0->p.x, src1->p.z);
dest->p.y = (fix) (q >> 16);
q = mul64 (src0->p.x, src1->p.y);
q += mul64 (-src0->p.y, src1->p.x);
dest->p.z = (fix) (q >> 16);
#endif
return dest;
}

// ------------------------------------------------------------------------

vmsVector *VmVecPerpChecked (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
	vmsVector t0, t1;

VmVecSub (&t0, p1, p0);
VmVecSub (&t1, p2, p1);
CheckVec (&t0);
CheckVec (&t1);
return VmVecCrossProd (dest, &t0, &t1);
}

// ------------------------------------------------------------------------
//computes non-normalized surface normal from three points. 
//returns ptr to dest
//dest CANNOT equal either source
vmsVector *VmVecPerp (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
	vmsVector t0, t1;

VmVecNormalize (VmVecSub (&t0, p1, p0));
VmVecNormalize (VmVecSub (&t1, p2, p1));
return VmVecCrossProd (dest, &t0, &t1);
}

// ------------------------------------------------------------------------
//computes the delta angle between two vectors. 
//vectors need not be normalized. if they are, call VmVecDeltaAngNorm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
fixang VmVecDeltaAng (vmsVector *v0, vmsVector *v1, vmsVector *fVec)
{
vmsVector t0, t1;

VmVecCopyNormalize (&t0, v0);
VmVecCopyNormalize (&t1, v1);
return VmVecDeltaAngNorm (&t0, &t1, fVec);
}

// ------------------------------------------------------------------------
//computes the delta angle between two normalized vectors. 
fixang VmVecDeltaAngNorm (vmsVector *v0, vmsVector *v1, vmsVector *fVec)
{
fixang a = FixACos (VmVecDot (v0, v1));
if (fVec) {
	vmsVector t;
	if (VmVecDot (VmVecCrossProd (&t, v0, v1), fVec) < 0)
		a = -a;
	}
return a;
}

// ------------------------------------------------------------------------
//computes the delta angle between two vectors. 
//vectors need not be normalized. if they are, call VmVecDeltaAngNorm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
float VmVecDeltaAngf (fVector *v0, fVector *v1, fVector *fVec)
{
fVector t0, t1;

VmVecNormalizef (&t0, v0);
VmVecNormalizef (&t1, v1);
return VmVecDeltaAngNormf (&t0, &t1, fVec);
}

// ------------------------------------------------------------------------
//computes the delta angle between two normalized vectors. 
float VmVecDeltaAngNormf (fVector *v0, fVector *v1, fVector *fVec)
{
float a = (float) acos ((double) VmVecDotf (v0, v1));
if (fVec) {
	fVector t;
	if (VmVecDotf (VmVecCrossProdf (&t, v0, v1), fVec) < 0)
		a = -a;
	}
return a;
}

// ------------------------------------------------------------------------

vmsMatrix *VmSinCos2Matrix (vmsMatrix *m, fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh)
{
	fix sbsh, cbch, cbsh, sbch;

sbsh = FixMul (sinb, sinh);
cbch = FixMul (cosb, cosh);
cbsh = FixMul (cosb, sinh);
sbch = FixMul (sinb, cosh);
m->rVec.p.x = cbch + FixMul (sinp, sbsh);		//m1
m->uVec.p.z = sbsh + FixMul (sinp, cbch);		//m8
m->uVec.p.x = FixMul (sinp, cbsh) - sbch;		//m2
m->rVec.p.z = FixMul (sinp, sbch) - cbsh;		//m7
m->fVec.p.x = FixMul (sinh, cosp);				//m3
m->rVec.p.y = FixMul (sinb, cosp);				//m4
m->uVec.p.y = FixMul (cosb, cosp);				//m5
m->fVec.p.z = FixMul (cosh, cosp);				//m9
m->fVec.p.y = -sinp;								//m6
return m;
}

// ------------------------------------------------------------------------

fMatrix *VmSinCos2Matrixd (fMatrix *m, double sinp, double cosp, double sinb, double cosb, double sinh, double cosh)
{
#if 0
	double sbsh, cbch, cbsh, sbch;

sbsh = sinb * sinh;
cbch = cosb * cosh;
cbsh = cosb * sinh;
sbch = sinb * cosh;
m->rVec.p.x = (float) (cbch + sinp * sbsh);	
m->uVec.p.z = (float) (sbsh + sinp * cbch);	
m->uVec.p.x = (float) (sinp * cbsh - sbch);	
m->rVec.p.z = (float) (sinp * sbch - cbsh);	
m->fVec.p.x = (float) (sinh * cosp);			
m->rVec.p.y = (float) (sinb * cosp);			
m->uVec.p.y = (float) (cosb * cosp);			
m->fVec.p.z = (float) (cosh * cosp);			
m->fVec.p.y = (float) (-sinp);					
#else
#endif
return m;
}

// ------------------------------------------------------------------------

fMatrix *VmSinCos2Matrixf (fMatrix *m, float sinp, float cosp, float sinb, float cosb, float sinh, float cosh)
{
	float sbsh, cbch, cbsh, sbch;

sbsh = sinb * sinh;
cbch = cosb * cosh;
cbsh = cosb * sinh;
sbch = sinb * cosh;
m->rVec.p.x = cbch + sinp * sbsh;	//m1
m->uVec.p.z = sbsh + sinp * cbch;	//m8
m->uVec.p.x = sinp * cbsh - sbch;	//m2
m->rVec.p.z = sinp * sbch - cbsh;	//m7
m->fVec.p.x = sinh * cosp;				//m3
m->rVec.p.y = sinb * cosp;				//m4
m->uVec.p.y = cosb * cosp;				//m5
m->fVec.p.z = cosh * cosp;				//m9
m->fVec.p.y = -sinp;						//m6
return m;
}

// ------------------------------------------------------------------------
//computes a matrix from a set of three angles.  returns ptr to matrix
vmsMatrix *VmAngles2Matrix (vmsMatrix *m, vmsAngVec *a)
{
fix sinp, cosp, sinb, cosb, sinh, cosh;
FixSinCos (a->p, &sinp, &cosp);
FixSinCos (a->b, &sinb, &cosb);
FixSinCos (a->h, &sinh, &cosh);
return VmSinCos2Matrix (m, sinp, cosp, sinb, cosb, sinh, cosh);
}

// ------------------------------------------------------------------------
//computes a matrix from a forward vector and an angle
vmsMatrix *VmVecAng2Matrix (vmsMatrix *m, vmsVector *v, fixang a)
{
	fix sinb, cosb, sinp, cosp;

FixSinCos (a, &sinb, &cosb);
sinp = -v->p.y;
cosp = FixSqrt (f1_0 - FixMul (sinp, sinp));
return VmSinCos2Matrix (m, sinp, cosp, sinb, cosb, FixDiv(v->p.x, cosp), FixDiv(v->p.z, cosp));
}

// ------------------------------------------------------------------------

inline int VmVecSign (vmsVector *v)
{
return (v->p.x * v->p.y * v->p.z < 0) ? -1 : 1;
}

// ------------------------------------------------------------------------
//computes a matrix from one or more vectors. The forward vector is required, 
//with the other two being optional.  If both up & right vectors are passed, 
//the up vector is used.  If only the forward vector is passed, a bank of
//zero is assumed
//returns ptr to matrix
vmsMatrix *VmVector2Matrix (vmsMatrix *m, vmsVector *fVec, vmsVector *uVec, vmsVector *rVec)
{
	vmsVector	*xvec = &m->rVec, 
					*yvec = &m->uVec, 
					*zvec = &m->fVec;

Assert(fVec != NULL);
if (VmVecCopyNormalize (zvec, fVec) == 0) {
	Int3();		//forward vec should not be zero-length
	return m;
	}
if (uVec == NULL) {
	if (rVec == NULL) {		//just forward vec

bad_vector2:
;
		if ((zvec->p.x == 0) && (zvec->p.z == 0)) {		//forward vec is straight up or down
			m->rVec.p.x = F1_0;
			m->uVec.p.z = (zvec->p.y < 0) ? F1_0 : -F1_0;
			m->rVec.p.y = m->rVec.p.z = m->uVec.p.x = m->uVec.p.y = 0;
			}
		else { 		//not straight up or down
			xvec->p.x = zvec->p.z;
			xvec->p.y = 0;
			xvec->p.z = -zvec->p.x;
			VmVecNormalize (xvec);
			VmVecCrossProd (yvec, zvec, xvec);
			}
		}
	else {						//use right vec
		if (VmVecCopyNormalize (xvec, rVec) == 0)
			goto bad_vector2;
		VmVecCrossProd (yvec, zvec, xvec);
		//normalize new perpendicular vector
		if (VmVecNormalize (yvec) == 0)
			goto bad_vector2;
		//now recompute right vector, in case it wasn't entirely perpendiclar
		VmVecCrossProd (xvec, yvec, zvec);
		}
	}
else {		//use up vec
	if (VmVecCopyNormalize (yvec, uVec) == 0)
		goto bad_vector2;
	VmVecCrossProd (xvec, yvec, zvec);
	//normalize new perpendicular vector
	if (VmVecNormalize (xvec) == 0)
		goto bad_vector2;
	//now recompute up vector, in case it wasn't entirely perpendiclar
	VmVecCrossProd (yvec, zvec, xvec);
	}
return m;
}

// ------------------------------------------------------------------------
//quicker version of VmVector2Matrix() that takes normalized vectors
vmsMatrix *VmVector2MatrixNorm (vmsMatrix *m, vmsVector *fVec, vmsVector *uVec, vmsVector *rVec)
{
	vmsVector	*xvec = &m->rVec, 
					*yvec = &m->uVec, 
					*zvec = &m->fVec;

Assert(fVec != NULL);
if (!uVec) {
	if (!rVec) {		//just forward vec
bad_vector2:
;
		if (!(zvec->p.x || zvec->p.z)) {		//forward vec is straight up or down
			m->rVec.p.x = F1_0;
			m->uVec.p.z = (zvec->p.y < 0) ? F1_0 : -F1_0;
			m->rVec.p.y = m->rVec.p.z = m->uVec.p.x = m->uVec.p.y = 0;
			}
		else { 		//not straight up or down
			xvec->p.x = zvec->p.z;
			xvec->p.y = 0;
			xvec->p.z = -zvec->p.x;
			VmVecNormalize (xvec);
			VmVecCrossProd (yvec, zvec, xvec);
			}
		}
	else {						//use right vec
		VmVecCrossProd (yvec , zvec, xvec);
		//normalize new perpendicular vector
		if (VmVecNormalize (yvec) == 0)
			goto bad_vector2;
		//now recompute right vector, in case it wasn't entirely perpendiclar
		VmVecCrossProd (xvec, yvec, zvec);
		}
	}
else {		//use up vec
	VmVecCrossProd (xvec, yvec, zvec);
	//normalize new perpendicular vector
	if (VmVecNormalize (xvec) == 0)
		goto bad_vector2;
	//now recompute up vector, in case it wasn't entirely perpendiclar
	VmVecCrossProd (yvec, zvec, xvec);

}
return m;
}

// ------------------------------------------------------------------------
//rotates a vector through a matrix. returns ptr to dest vector
//dest CANNOT equal source

vmsVector *VmVecRotate (vmsVector *dest, vmsVector *src, vmsMatrix *m)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
		fVector	vf;
		fMatrix	mf, *mfP;

	VmVecFixToFloat (&vf, src);
	if (m == &viewInfo.view [0])
		mfP = &viewInfo.viewf [0];
	else {
		VmVecFixToFloat (&mf.fVec, &m->fVec);
		VmVecFixToFloat (&mf.rVec, &m->rVec);
		VmVecFixToFloat (&mf.uVec, &m->uVec);
		mfP = &mf;
		}
#if defined (_WIN32)
	__asm {
		movups	xmm0,vf
		// multiply source vector with matrix rows and store results in xmm1 .. xmm3
		mov		esi,mfP
		movups	xmm4,[esi]
		movups	xmm1,xmm0
		mulps		xmm1,xmm4
		movups	xmm4,[esi+0x10]
		movups	xmm2,xmm0
		mulps		xmm2,xmm4
		movups	xmm4,[esi+0x20]
		movups	xmm3,xmm0
		mulps		xmm3,xmm4
		// shuffle xmm1 .. xmm3 so that xmm1 = (x1,x2,x3), xmm2 = (y1,y2,y3), xmm3 = (z1,z2,z3)
		movups	xmm0,xmm1
		shufps	xmm0,xmm2,01000100b
		movups	xmm4,xmm0
		shufps	xmm0,xmm3,11001000b
		shufps	xmm4,xmm3,11011101b
		shufps	xmm1,xmm2,11101110b
		shufps	xmm1,xmm3,11101000b
		// cumulate xmm1 .. xmm3
		addps		xmm0,xmm1
		addps		xmm0,xmm4
		movups	vf,xmm0
		}
#elif defined (__unix__)
	asm (
		"movups	%0,%%xmm0\n\t"
		"mov		%1,%%rsi\n\t"
		"movups	(%%rsi),%%xmm4\n\t"
		"movups	%%xmm0,%%xmm1\n\t"
		"mulps	%%xmm4,%%xmm1\n\t"
		"movups	0x10(%%rsi),%%xmm4\n\t"
		"movups	%%xmm0,%%xmm2\n\t"
		"mulps	%%xmm4,%%xmm2\n\t"
		"movups	0x20(%%rsi),%%xmm4\n\t"
		"movups	%%xmm0,%%xmm3\n\t"
		"mulps	%%xmm4,%%xmm3\n\t"
		"shufps	$0x44,%%xmm2,%%xmm0\n\t"
		"movups	%%xmm0,%%xmm4\n\t"
		"shufps	$0xC8,%%xmm3,%%xmm0\n\t"
		"shufps	$0xDD,%%xmm3,%%xmm4\n\t"
		"shufps	$0xEE,%%xmm2,%%xmm1\n\t"
		"shufps	$0xE8,%%xmm3,%%xmm1\n\t"
		"addps	%%xmm1,%%xmm0\n\t"
		"addps	%%xmm4,%%xmm1\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (vf)
		: "m" (vf), "m" (mfP)
		: "%rsi"
		);
#endif
	dest->p.x = (fix) (vf.p.x * 65536);
	dest->p.y = (fix) (vf.p.y * 65536);
	dest->p.z = (fix) (vf.p.z * 65536);
	}
else 
#endif
	{
		vmsVector	h;

	if (!m)
		m = &viewInfo.view [0];
	if (src == dest) {
		h = *src;
		src = &h;
		}
	dest->p.x = VmVecDot (src, &m->rVec);
	dest->p.y = VmVecDot (src, &m->uVec);
	dest->p.z = VmVecDot (src, &m->fVec);
	}
return dest;
}

// ------------------------------------------------------------------------

fMatrix *VmsMatToFloat (fMatrix *dest, vmsMatrix *src)
{
VmVecFixToFloat (&dest->rVec, &src->rVec);
VmVecFixToFloat (&dest->uVec, &src->uVec);
VmVecFixToFloat (&dest->fVec, &src->fVec);
dest->wVec.p.x = 
dest->wVec.p.y = 
dest->wVec.p.z = 0;
dest->wVec.p.w = 1;
return dest;
}

// ------------------------------------------------------------------------
//rotates a vector through a matrix. returns ptr to dest vector

fVector *VmVecRotatef (fVector *dest, fVector *src, fMatrix *m)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
#if defined (_WIN32)
	__asm {
		mov		esi,src
		movups	xmm0,[esi]
		mov		esi,m
		// multiply source vector with matrix rows and store results in xmm1 .. xmm3
		movups	xmm4,[esi]
		movups	xmm1,xmm0
		mulps		xmm1,xmm4
		movups	xmm4,[esi+0x10]
		movups	xmm2,xmm0
		mulps		xmm2,xmm4
		movups	xmm4,[esi+0x20]
		movups	xmm3,xmm0
		mulps		xmm3,xmm4
		// shuffle xmm1 .. xmm3 so that xmm1 = (x1,x2,x3), xmm2 = (y1,y2,y3), xmm3 = (z1,z2,z3)
		movups	xmm0,xmm1
		shufps	xmm0,xmm2,01000100b
		movups	xmm4,xmm0
		shufps	xmm0,xmm3,11001000b
		shufps	xmm4,xmm3,11011101b
		shufps	xmm1,xmm2,11101110b
		shufps	xmm1,xmm3,11101000b
		// cumulate xmm1 .. xmm3
		addps		xmm0,xmm1
		addps		xmm0,xmm4
		mov		edi,dest
		movups	[edi],xmm0
		}
#elif defined (__unix__)
	asm (
		"movups	%1,%%xmm0\n\t"
		"mov		%2,%%rsi\n\t"
		"movups	(%%rsi),%%xmm4\n\t"
		"movups	%%xmm0,%%xmm1\n\t"
		"mulps	%%xmm4,%%xmm1\n\t"
		"movups	0x10(%%rsi),%%xmm4\n\t"
		"movups	%%xmm0,%%xmm2\n\t"
		"mulps	%%xmm4,%%xmm2\n\t"
		"movups	0x20(%%rsi),%%xmm4\n\t"
		"movups	%%xmm0,%%xmm3\n\t"
		"mulps	%%xmm4,%%xmm3\n\t"
		"shufps	$0x44,%%xmm2,%%xmm0\n\t"
		"movups	%%xmm0,%%xmm4\n\t"
		"shufps	$0xC8,%%xmm3,%%xmm0\n\t"
		"shufps	$0xDD,%%xmm3,%%xmm4\n\t"
		"shufps	$0xEE,%%xmm2,%%xmm1\n\t"
		"shufps	$0xE8,%%xmm3,%%xmm1\n\t"
		"addps	%%xmm1,%%xmm0\n\t"
		"addps	%%xmm4,%%xmm1\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (dest)
		: "m" (src), "m" (m)
		: "%rsi"
		);
#endif
	}
else 
#endif
	{
		fVector h;

	if (src == dest) {
		h = *src;
		src = &h;
		}
	dest->p.x = VmVecDotf (src, &m->rVec);
	dest->p.y = VmVecDotf (src, &m->uVec);
	dest->p.z = VmVecDotf (src, &m->fVec);
	}
return dest;
}

// ------------------------------------------------------------------------
//transpose a matrix in place. returns ptr to matrix
vmsMatrix *VmTransposeMatrix (vmsMatrix *m)
{
	fix t;

t = m->uVec.p.x;  m->uVec.p.x = m->rVec.p.y;  m->rVec.p.y = t;
t = m->fVec.p.x;  m->fVec.p.x = m->rVec.p.z;  m->rVec.p.z = t;
t = m->fVec.p.y;  m->fVec.p.y = m->uVec.p.z;  m->uVec.p.z = t;
return m;
}

// ------------------------------------------------------------------------
//transpose a matrix in place. returns ptr to matrix
fMatrix *VmTransposeMatrixf (fMatrix *m)
{
	float	t;

t = m->uVec.p.x;  m->uVec.p.x = m->rVec.p.y;  m->rVec.p.y = t;
t = m->fVec.p.x;  m->fVec.p.x = m->rVec.p.z;  m->rVec.p.z = t;
t = m->fVec.p.y;  m->fVec.p.y = m->uVec.p.z;  m->uVec.p.z = t;
return m;
}

// ------------------------------------------------------------------------

vmsMatrix *VmInvertMatrix (vmsMatrix *pDest, vmsMatrix *pSrc)
{
	fix	xDet = VmMatrixDetValue (pSrc);
	vmsMatrix	m = *pSrc;

pDest->rVec.p.x = FixDiv (FixMul (m.uVec.p.y, m.fVec.p.z) - FixMul (m.uVec.p.z, m.fVec.p.y), xDet);
pDest->rVec.p.y = FixDiv (FixMul (m.rVec.p.z, m.fVec.p.y) - FixMul (m.rVec.p.y, m.fVec.p.z), xDet);
pDest->rVec.p.z = FixDiv (FixMul (m.rVec.p.y, m.uVec.p.z) - FixMul (m.rVec.p.z, m.uVec.p.y), xDet);
pDest->uVec.p.x = FixDiv (FixMul (m.uVec.p.z, m.fVec.p.x) - FixMul (m.uVec.p.x, m.fVec.p.z), xDet);
pDest->uVec.p.y = FixDiv (FixMul (m.rVec.p.x, m.fVec.p.z) - FixMul (m.rVec.p.z, m.fVec.p.x), xDet);
pDest->uVec.p.z = FixDiv (FixMul (m.rVec.p.z, m.uVec.p.x) - FixMul (m.rVec.p.x, m.uVec.p.z), xDet);
pDest->fVec.p.x = FixDiv (FixMul (m.uVec.p.x, m.fVec.p.y) - FixMul (m.uVec.p.y, m.fVec.p.x), xDet);
pDest->fVec.p.y = FixDiv (FixMul (m.rVec.p.y, m.fVec.p.x) - FixMul (m.rVec.p.x, m.fVec.p.y), xDet);
pDest->fVec.p.z = FixDiv (FixMul (m.rVec.p.x, m.uVec.p.y) - FixMul (m.rVec.p.y, m.uVec.p.x), xDet);
return pDest;
}

// ------------------------------------------------------------------------

fMatrix *VmInvertMatrixf (fMatrix *pDest, fMatrix *pSrc)
{
	float		fDet = VmMatrixDetValuef (pSrc);
	fMatrix	m = *pSrc;

pDest->rVec.p.x = (m.uVec.p.y * m.fVec.p.z - m.uVec.p.z * m.fVec.p.y) / fDet;
pDest->rVec.p.y = (m.rVec.p.z * m.fVec.p.y - m.rVec.p.y * m.fVec.p.z) / fDet;
pDest->rVec.p.z = (m.rVec.p.y * m.uVec.p.z - m.rVec.p.z * m.uVec.p.y) / fDet;
pDest->uVec.p.x = (m.uVec.p.z * m.fVec.p.x - m.uVec.p.x * m.fVec.p.z) / fDet;
pDest->uVec.p.y = (m.rVec.p.x * m.fVec.p.z - m.rVec.p.z * m.fVec.p.x) / fDet;
pDest->uVec.p.z = (m.rVec.p.z * m.uVec.p.x - m.rVec.p.x * m.uVec.p.z) / fDet;
pDest->fVec.p.x = (m.uVec.p.x * m.fVec.p.y - m.uVec.p.y * m.fVec.p.x) / fDet;
pDest->fVec.p.y = (m.rVec.p.y * m.fVec.p.x - m.rVec.p.x * m.fVec.p.y) / fDet;
pDest->fVec.p.z = (m.rVec.p.x * m.uVec.p.y - m.rVec.p.y * m.uVec.p.x) / fDet;
return pDest;
}

// ------------------------------------------------------------------------
//copy and transpose a matrix. returns ptr to matrix
//dest CANNOT equal source. use VmTransposeMatrix() if this is the case
vmsMatrix *VmCopyTransposeMatrix (vmsMatrix *dest, vmsMatrix *src)
{
Assert(dest != src);
dest->rVec.p.x = src->rVec.p.x;
dest->rVec.p.y = src->uVec.p.x;
dest->rVec.p.z = src->fVec.p.x;
dest->uVec.p.x = src->rVec.p.y;
dest->uVec.p.y = src->uVec.p.y;
dest->uVec.p.z = src->fVec.p.y;
dest->fVec.p.x = src->rVec.p.z;
dest->fVec.p.y = src->uVec.p.z;
dest->fVec.p.z = src->fVec.p.z;
return dest;
}

// ------------------------------------------------------------------------

fMatrix *VmCopyTransposeMatrixf (fMatrix *dest, fMatrix *src)
{
Assert(dest != src);
dest->rVec.p.x = src->rVec.p.x;
dest->rVec.p.y = src->uVec.p.x;
dest->rVec.p.z = src->fVec.p.x;
dest->uVec.p.x = src->rVec.p.y;
dest->uVec.p.y = src->uVec.p.y;
dest->uVec.p.z = src->fVec.p.y;
dest->fVec.p.x = src->rVec.p.z;
dest->fVec.p.y = src->uVec.p.z;
dest->fVec.p.z = src->fVec.p.z;
return dest;
}

// ------------------------------------------------------------------------
//multiply 2 matrices, fill in dest.  returns ptr to dest
//dest CANNOT equal either source
vmsMatrix *VmMatMul (vmsMatrix *dest, vmsMatrix *src0, vmsMatrix *src1)
{
	vmsVector	v;

Assert(dest!=src0 && dest!=src1);
v.p.x = src0->rVec.p.x;
v.p.y = src0->uVec.p.x;
v.p.z = src0->fVec.p.x;
dest->rVec.p.x = VmVecDot (&v, &src1->rVec);
dest->uVec.p.x = VmVecDot (&v, &src1->uVec);
dest->fVec.p.x = VmVecDot (&v, &src1->fVec);
v.p.x = src0->rVec.p.y;
v.p.y = src0->uVec.p.y;
v.p.z = src0->fVec.p.y;
dest->rVec.p.y = VmVecDot (&v, &src1->rVec);
dest->uVec.p.y = VmVecDot (&v, &src1->uVec);
dest->fVec.p.y = VmVecDot (&v, &src1->fVec);
v.p.x = src0->rVec.p.z;
v.p.y = src0->uVec.p.z;
v.p.z = src0->fVec.p.z;
dest->rVec.p.z = VmVecDot (&v, &src1->rVec);
dest->uVec.p.z = VmVecDot (&v, &src1->uVec);
dest->fVec.p.z = VmVecDot (&v, &src1->fVec);
return dest;
}
#endif

// ------------------------------------------------------------------------
//extract angles from a matrix 
vmsAngVec *VmExtractAnglesMatrix (vmsAngVec *a, vmsMatrix *m)
{
	fix sinh, cosh, cosp;

if (m->fVec.p.x==0 && m->fVec.p.z==0)		//zero head
	a->h = 0;
else
	a->h = FixAtan2(m->fVec.p.z, m->fVec.p.x);
FixSinCos(a->h, &sinh, &cosh);
if (abs(sinh) > abs(cosh))				//sine is larger, so use it
	cosp = FixDiv(m->fVec.p.x, sinh);
else											//cosine is larger, so use it
	cosp = FixDiv(m->fVec.p.z, cosh);
if (cosp==0 && m->fVec.p.y==0)
	a->p = 0;
else
	a->p = FixAtan2(cosp, -m->fVec.p.y);
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a->b = 0;
else {
	fix sinb, cosb;

	sinb = FixDiv (m->rVec.p.y, cosp);
	cosb = FixDiv (m->uVec.p.y, cosp);
	if (sinb==0 && cosb==0)
		a->b = 0;
	else
		a->b = FixAtan2(cosb, sinb);
	}
return a;
}

//	-----------------------------------------------------------------------------
//returns the value of a determinant
fix VmMatrixDetValue (vmsMatrix *m)
{
#if 1
	fix	xDet;
//LogErr ("            CalcDetValue (R: %d, %d, %d; F: %d, %d, %d; U: %d, %d, %d)\n", m->rVec.p.x, m->rVec.p.y, m->rVec.p.z, m->fVec.p.x, m->fVec.p.y, m->fVec.p.z, m->uVec.p.x, m->uVec.p.y, m->uVec.p.z);
//LogErr ("               xDet = FixMul (m->rVec.p.x, FixMul (m->uVec.p.y, m->fVec.p.z))\n");
xDet = FixMul (m->rVec.p.x, FixMul (m->uVec.p.y, m->fVec.p.z));
//LogErr ("               xDet -= FixMul (m->rVec.p.x, FixMul (m->uVec.p.z, m->fVec.p.y))\n");
xDet -= FixMul (m->rVec.p.x, FixMul (m->uVec.p.z, m->fVec.p.y));
//LogErr ("               xDet -= FixMul (m->rVec.p.y, FixMul (m->uVec.p.x, m->fVec.p.z))\n");
xDet -= FixMul (m->rVec.p.y, FixMul (m->uVec.p.x, m->fVec.p.z));
//LogErr ("               xDet += FixMul (m->rVec.p.y, FixMul (m->uVec.p.z, m->fVec.p.x))\n");
xDet += FixMul (m->rVec.p.y, FixMul (m->uVec.p.z, m->fVec.p.x));
//LogErr ("               xDet += FixMul (m->rVec.p.z, FixMul (m->uVec.p.x, m->fVec.p.y))\n");
xDet += FixMul (m->rVec.p.z, FixMul (m->uVec.p.x, m->fVec.p.y));
//LogErr ("               xDet -= FixMul (m->rVec.p.z, FixMul (m->uVec.p.y, m->fVec.p.x))\n");
xDet -= FixMul (m->rVec.p.z, FixMul (m->uVec.p.y, m->fVec.p.x));
return xDet;
//LogErr ("             m = %d\n", xDet);
#else
return FixMul (m->rVec.p.x, FixMul (m->uVec.p.y, m->fVec.p.z)) -
		 FixMul (m->rVec.p.x, FixMul (m->uVec.p.z, m->fVec.p.y)) -
		 FixMul (m->rVec.p.y, FixMul (m->uVec.p.x, m->fVec.p.z)) +
		 FixMul (m->rVec.p.y, FixMul (m->uVec.p.z, m->fVec.p.x)) +
	 	 FixMul (m->rVec.p.z, FixMul (m->uVec.p.x, m->fVec.p.y)) -
		 FixMul (m->rVec.p.z, FixMul (m->uVec.p.y, m->fVec.p.x));
#endif
}

//	-----------------------------------------------------------------------------
//returns the value of a determinant
float VmMatrixDetValuef (fMatrix *m)
{
return m->rVec.p.x * m->uVec.p.y * m->fVec.p.z -
		 m->rVec.p.x * m->uVec.p.z * m->fVec.p.y -
		 m->rVec.p.y * m->uVec.p.x * m->fVec.p.z +
		 m->rVec.p.y * m->uVec.p.z * m->fVec.p.x +
	 	 m->rVec.p.z * m->uVec.p.x * m->fVec.p.y -
		 m->rVec.p.z * m->uVec.p.y * m->fVec.p.x;
}

// ------------------------------------------------------------------------
//extract heading and pitch from a vector, assuming bank==0
vmsAngVec *VmExtractAnglesVecNorm (vmsAngVec *a, vmsVector *v)
{
a->b = 0;		//always zero bank
a->p = FixASin (-v->p.y);
a->h = (v->p.x || v->p.z) ? FixAtan2 (v->p.z, v->p.x) : 0;
return a;
}

// ------------------------------------------------------------------------
//extract heading and pitch from a vector, assuming bank==0
vmsAngVec *VmExtractAnglesVector (vmsAngVec *a, vmsVector *v)
{
	vmsVector t;

if (VmVecCopyNormalize (&t, v))
	VmExtractAnglesVecNorm (a, &t);
return a;
}

// ------------------------------------------------------------------------
//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix VmDistToPlane (vmsVector *vCheck, vmsVector *vNorm, vmsVector *vPlane)
{
#if ENABLE_SSE
if (gameStates.render.bEnableSSE) {
		fVector	c, p, n, t;

	VmVecFixToFloat (&c, vCheck);
	VmVecFixToFloat (&p, vPlane);
	VmVecFixToFloat (&n, vNorm);
#if defined (_WIN32)
	_asm {
		movups	xmm0,c
		movups	xmm1,p
		subps		xmm0,xmm1
		movups	xmm1,n
		mulps		xmm0,xmm1
		movups	t,xmm0
		}
#elif defined (__unix__)
	asm (
		"movups	%1,%%xmm0\n\t"
		"movups	%2,%%xmm1\n\t"
		"subps	%%xmm1,%%xmm0\n\t"
		"movups	%3,%%xmm1\n\t"
		"mulps	%%xmm1,%%xmm0\n\t"
		"movups	%%xmm0,%0\n\t"
		: "=m" (t)
		: "m" (c), "m" (p), "m" (n)
		);
#endif
	return (fix) ((t.p.x + t.p.y + t.p.z) * 65536);
	}
else 
#endif
	{
	vmsVector t;

	return VmVecDot (VmVecSub (&t, vCheck, vPlane), vNorm);
	}
}

// ------------------------------------------------------------------------

vmsVector *VmVecMake (vmsVector *v, fix x, fix y, fix z)
{
v->p.x = x; 
v->p.y = y; 
v->p.z = z;
return v;
}

// ------------------------------------------------------------------------
// compute intersection of a line through a point a, with the line being orthogonal relative
// to the plane given by the normal n and a point p lieing in the plane, and store it in i.

vmsVector *VmPlaneProjection (vmsVector *i, vmsVector *n, vmsVector *p, vmsVector *a)
{
#if 1
#	ifdef _DEBUG
double d = (double) -VmVecDot (n, p);
double l = d / (double) VmVecDot (n, a);
#	else
double l = (double) -VmVecDot (n, p) / (double) VmVecDot (n, a);
#	endif
i->p.x = (fix) (l * (double) a->p.x);
i->p.y = (fix) (l * (double) a->p.y);
i->p.z = (fix) (l * (double) a->p.z);
return i;
#else
*i = *a;
return VmVecScale (i, -VmVecDot (n, p) / VmVecDot (n, a));
#endif
}

// ------------------------------------------------------------------------

inline int VmBehindPlane (vmsVector *n, vmsVector *p1, vmsVector *p2, vmsVector *i)
{
	vmsVector	t;
#ifdef _DEBUG
	fix			d;
#endif

VmVecSub (&t, p1, p2);
#ifdef _DEBUG
d = VmVecDot (p1, &t);
return VmVecDot (i, &t) < d;
#else
return VmVecDot (i, &t) - VmVecDot (p1, &t) < 0;
#endif
}

// ------------------------------------------------------------------------

int VmTriangleHitTest (vmsVector *n, vmsVector *p1, vmsVector *p2, vmsVector *p3, vmsVector *i)
{
return !(VmBehindPlane (n, p1, p2, i) || 
			VmBehindPlane (n, p2, p3, i) ||
			VmBehindPlane (n, p3, p1, i));
}

// ------------------------------------------------------------------------
// compute intersection of orthogonal line on plane n, p1 through point a
// and check whether the intersection lies inside the triangle p1, p2, p3
// in one step.

int VmTriangleHitTestQuick (vmsVector *n, vmsVector *p1, vmsVector *p2, vmsVector *p3, vmsVector *a)
{
	vmsVector	i;
	double l = (double) -VmVecDot (n, p1) / (double) VmVecDot (n, a);

i.p.x = (fix) (l * (double) a->p.x);
i.p.y = (fix) (l * (double) a->p.y);
i.p.z = (fix) (l * (double) a->p.z);
return !(VmBehindPlane (n, p1, p2, &i) || 
			VmBehindPlane (n, p2, p3, &i) ||
			VmBehindPlane (n, p3, p1, &i));
}

//	-----------------------------------------------------------------------------
// Find intersection of perpendicular on p1,p2 through p3 with p1,p2.
// If intersection is not between p1 and p2 and vPos is given, return
// further away point of p1 and p2 to vPos. Otherwise return intersection.
// returns 1 if intersection outside of p1,p2, otherwise 0.

int VmPointLineIntersection (vmsVector *hitP, vmsVector *p1, vmsVector *p2, vmsVector *p3, vmsVector *vPos, 
									  int bClampToFarthest)
{
	vmsVector	d31, d21;
	double		m, u;
	int			bClamped = 0;

VmVecSub (&d21, p2, p1);
m = fabs ((double) d21.p.x * (double) d21.p.x + (double) d21.p.y * (double) d21.p.y + (double) d21.p.z * (double) d21.p.z);
if (!m) {
	if (hitP)
		*hitP = *p1;
	return 0;
	}
VmVecSub (&d31, p3, p1);
u = (double) d31.p.x * (double) d21.p.x + (double) d31.p.y * (double) d21.p.y + (double) d31.p.z * (double) d21.p.z;
u /= m;
if (u < 0)
	bClamped = bClampToFarthest ? 2 : 1;
else if (u > 1)
	bClamped = bClampToFarthest ? 1 : 2;
else
	bClamped = 0;
if (bClamped == 2)
	*hitP = *p2;
else if (bClamped == 1)
	*hitP = *p1;
else {
	hitP->p.x = p1->p.x + (fix) (u * d21.p.x);
	hitP->p.y = p1->p.y + (fix) (u * d21.p.y);
	hitP->p.z = p1->p.z + (fix) (u * d21.p.z);
	}
return bClamped;
}

// ------------------------------------------------------------------------

int VmPointLineIntersectionf (fVector *hitP, fVector *p1, fVector *p2, fVector *p3, fVector *vPos, int bClamp)
{
	fVector	d31, d21;
	double	m, u;
	int		bClamped = 0;

VmVecSubf (&d21, p2, p1);
m = fabs (d21.p.x * d21.p.x + d21.p.y * d21.p.y + d21.p.z * d21.p.z);
if (!m) {
	if (hitP)
		*hitP = *p1;
	return 0;
	}
VmVecSubf (&d31, p3, p1);
u = (double) VmVecDotf (&d31, &d21);
u /= m;
if (u < 0)
	bClamped = 2;
else if (u > 1)
	bClamped = 1;
else
	bClamped = 0;
// limit the intersection to [p1,p2]
if (hitP) {
	if (bClamp && bClamped) {
		if (vPos)
			bClamped = (VmVecDistf (vPos, p1) < VmVecDistf (vPos, p2)) ? 2 : 1;
		*hitP = (bClamped == 1) ? *p1 : * p2;
		}
	else {
		hitP->p.x = p1->p.x + (fix) (u * d21.p.x);
		hitP->p.y = p1->p.y + (fix) (u * d21.p.y);
		hitP->p.z = p1->p.z + (fix) (u * d21.p.z);
		}
	}
return bClamped;
}

// ------------------------------------------------------------------------

fix VmLinePointDist (vmsVector *a, vmsVector *b, vmsVector *p)
{
#if 1
	vmsVector	h;

VmPointLineIntersection (&h, a, b, p, NULL, 0);
return VmVecDist (&h, p);
#else	//this is the original code, which doesn't always work?!
	vmsVector	ab, ap, abxap;
	double		magab;

VmVecSub (&ab, b, a);
VmVecSub (&ap, p, a);
if (!(magab = (double) VmVecMag (&ab)))
	return VmVecMag (&ap);
VmVecCrossProd (&abxap, &ab, &ap);
return (fix) ((double) VmVecMag (&abxap) / magab * F1_0);
#endif
}

// ------------------------------------------------------------------------

float VmLinePointDistf (fVector *a, fVector *b, fVector *p, int bClamp)
{
	fVector	h;

VmPointLineIntersectionf (&h, a, b, p, NULL, bClamp);
return VmVecDistf (&h, p);
}

//------------------------------------------------------------------------------
// Reflect vDir at surface with normal vNormal. Return result in vReflect
// 2 * n * (l dot n) - l

vmsVector *VmVecReflect (vmsVector *vReflect, vmsVector *vDir, vmsVector *vNormal)
{
	fix	dot = VmVecDot (vDir, vNormal);

VmVecCopyScale (vReflect, vNormal, 2 * dot);
VmVecNegate (VmVecDec (vReflect, vDir));
return vReflect;
}

//------------------------------------------------------------------------------
// Reflect vDir at surface with normal vNormal. Return result in vReflect
// 2 * n * (l dot n) - l

fVector *VmVecReflectf (fVector *vReflect, fVector *vDir, fVector *vNormal)
{
	float dot = VmVecDotf (vDir, vNormal);

VmVecScalef (vReflect, vNormal, 2 * dot);
VmVecDecf (vReflect, vDir);
return VmVecNegatef (vReflect);
}

// --------------------------------------------------------------------------------------------------------------------
//	Compute a somewhat random, normalized vector.
vmsVector *MakeRandomVector (vmsVector *v)
{
	int i = d_rand () % 3;

if (i == 2) {
	v->p.x = (d_rand () - 16384) | 1;	// make sure we don't create null vector
	v->p.y = d_rand () - 16384;
	v->p.z = d_rand () - 16384;
	}
else if (i == 1) {
	v->p.y = d_rand () - 16384;
	v->p.z = d_rand () - 16384;
	v->p.x = (d_rand () - 16384) | 1;	// make sure we don't create null vector
	}
else {
	v->p.z = d_rand () - 16384;
	v->p.x = (d_rand () - 16384) | 1;	// make sure we don't create null vector
	v->p.y = d_rand () - 16384;
	}
VmVecNormalizeQuick (v);
return v;
}

// ------------------------------------------------------------------------
// eof
