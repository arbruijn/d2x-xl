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
#define ENABLE_SSE		0

#ifndef ASM_VECMAT
vmsVector vmdZeroVector = {0, 0, 0};
vmsMatrix vmdIdentityMatrix = {{f1_0, 0, 0}, 
                                {0, f1_0, 0}, 
                                {0, 0, f1_0}};

// ------------------------------------------------------------------------
//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecInc() if so
#if !INLINE_VEC_ADD
vmsVector *VmVecAdd (vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
dest->x = src0->x + src1->x;
dest->y = src0->y + src1->y;
dest->z = src0->z + src1->z;
return dest;
}

// ------------------------------------------------------------------------
//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecDec() if so
vmsVector *VmVecSub(vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
dest->x = src0->x - src1->x;
dest->y = src0->y - src1->y;
dest->z = src0->z - src1->z;
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
dest->x += src->x;
dest->y += src->y;
dest->z += src->z;
return dest;
}

// ------------------------------------------------------------------------
//subs one vector from another, returns ptr to dest
//dest can equal source
vmsVector *VmVecDec(vmsVector *dest, vmsVector *src)
{
dest->x -= src->x;
dest->y -= src->y;
dest->z -= src->z;
return dest;
}
#endif

// ------------------------------------------------------------------------
//averages two vectors. returns ptr to dest
//dest can equal either source
vmsVector *VmVecAvg(vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
dest->x = (src0->x + src1->x)/2;
dest->y = (src0->y + src1->y)/2;
dest->z = (src0->z + src1->z)/2;
return dest;
}

// ------------------------------------------------------------------------
//averages four vectors. returns ptr to dest
//dest can equal any source
vmsVector *VmVecAvg4(vmsVector *dest, 
								vmsVector *src0, vmsVector *src1, 
								vmsVector *src2, vmsVector *src3)
{
dest->x = (src0->x + src1->x + src2->x + src3->x)/4;
dest->y = (src0->y + src1->y + src2->y + src3->y)/4;
dest->z = (src0->z + src1->z + src2->z + src3->z)/4;
return dest;
}

// ------------------------------------------------------------------------
//scales a vector in place.  returns ptr to vector
vmsVector *VmVecScale (vmsVector *dest, fix s)
{
dest->x = FixMul (dest->x, s);
dest->y = FixMul (dest->y, s);
dest->z = FixMul (dest->z, s);
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
dest->x = FixMul (src->x, s);
dest->y = FixMul (src->y, s);
dest->z = FixMul (src->z, s);
return dest;
}

// ------------------------------------------------------------------------
//scales a vector, adds it to another, and stores in a 3rd vector
//dest = src1 + k * src2
vmsVector *VmVecScaleAdd (vmsVector *dest, vmsVector *src1, vmsVector *src2, fix k)
{
dest->x = src1->x + FixMul (src2->x, k);
dest->y = src1->y + FixMul (src2->y, k);
dest->z = src1->z + FixMul (src2->z, k);
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
dest->x += FixMul (src->x, k);
dest->y += FixMul (src->y, k);
dest->z += FixMul (src->z, k);
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
double nd = f2fl(n) / f2fl(d);
dest->x = fl2f (f2fl (dest->x) * nd);
dest->y = fl2f (f2fl (dest->y) * nd);
dest->z = fl2f (f2fl (dest->z) * nd);
#else
dest->x = FixMulDiv (src->x, n, d);
dest->y = FixMulDiv (src->y, n, d);
dest->z = FixMulDiv (src->z, n, d);
#endif
return dest;
}

// ------------------------------------------------------------------------

fVector *VmVecMulf (fVector *dest, fVector *src0, fVector *src1)
{
#if ENABLE_SSE
if (gameOpts->render.bEnableSSE) {
#if defined (_WIN32)
	_asm {
		mov		esi,src0
		movups	xmm0,[esi]
		mov		esi,src1
		movups	xmm1,[esi]
		mulps		xmm0,xmm1
		movups	dest,xmm0
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
if (gameOpts->render.bEnableSSE) {
		fVector	v0h, v1h;
	
	VmsVecToFloat (&v0h, v0);
	VmsVecToFloat (&v1h, v1);
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
	return (fix) (((double) v0->x * (double) v1->x + 
						(double) v0->y * (double) v1->y + 
						(double) v0->z * (double) v1->z) 
					  / 65536.0);
else {
	QLONG q = mul64 (v0->x, v1->x);
	q += mul64 (v0->y, v1->y);
	q += mul64 (v0->z, v1->z);
	return (fix) (q / 65536); //>> 16);
	}
}

// ------------------------------------------------------------------------

float VmVecDotf (fVector *v0, fVector *v1)
{
#if ENABLE_SSE
if (gameOpts->render.bEnableSSE) {
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
	return (fix) (((double) x * (double) v->x + (double) y * (double) v->y + (double) z * (double) v->z) / 65536.0);
else {
	QLONG q = mul64 (x, v->x);
	q += mul64 (y, v->y);
	q += mul64 (z, v->z);
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
	if (a < 0x100000000)
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
	return quad_sqrt ((u_int32_t) (a & 0xFFFFFFFF), (u_int32_t) (a >> 32));
	}
}

// ------------------------------------------------------------------------
//returns magnitude of a vector
fix VmVecMag (vmsVector *v)
{
if (gameOpts->render.nMathFormat == 2) {
#ifdef _WIN32
		fVector	h;

	VmsVecToFloat (&h, v);
	__asm {
		movups	xmm0,h
		mulps		xmm0,xmm0
		movups	h,xmm0
		}
	return (fix) (sqrt (h.p.x + h.p.y + h.p.z) * 65536);
#else
	return (fix) sqrt (sqrd ((double) v->x) + sqrd ((double) v->y) + sqrd ((double) v->z)); 
#endif
	}
else {
#if 1//def _WIN32
	QLONG q = mul64 (v->x, v->x);
	q += mul64 (v->y, v->y);
	q += mul64 (v->z, v->z);
	return sqrt64 ((unsigned QLONG) q);
#else
	quadint q;
	q.low = q.high = 0;
	fixmulaccum(&q, v->x, v->x);
	fixmulaccum(&q, v->y, v->y);
	fixmulaccum(&q, v->z, v->z);
	return quad_sqrt(q.low, q.high);
#endif
	}
}

// ------------------------------------------------------------------------

float VmVecMagf (fVector *v)
{
#if ENABLE_SSE
if (gameOpts->render.bEnableSSE) {
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
fix VmVecDist(vmsVector *v0, vmsVector *v1)
{
	vmsVector t;

return VmVecMag (VmVecSub (&t, v0, v1));
}

// ------------------------------------------------------------------------

float VmVecDistf (fVector *v0, fVector *v1)
{
	fVector	t;

#if ENABLE_SSE
if (gameOpts->render.bEnableSSE) {
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
return FixVecMagQuick (v0->x - v1->x, v0->y - v1->y, v0->z - v1->z);
}
#endif

// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec
fix VmVecCopyNormalize (vmsVector *dest, vmsVector *src)
{
fix m = VmVecMag (src);
if (m) {
	dest->x = FixDiv (src->x, m);
	dest->y = FixDiv (src->y, m);
	dest->z = FixDiv (src->z, m);
	}
else
	dest->x =
	dest->y =
	dest->z = 0;
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
	dest->x = FixDiv(src->x, m);
	dest->y = FixDiv(src->y, m);
	dest->z = FixDiv(src->z, m);
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
QLONG q = mul64 (v->x, v->x);
q += mul64 (v->y, v->y);
q += mul64 (v->z, v->z);
h = q >> 32;
if (h >= 0x800000)
	return (fix_isqrt (h) >> 8);
l = q & (QLONG) 0xFFFFFFFF;
if (!h)
	return fix_isqrt (l);
return (fix_isqrt ((h << 8) + (l >> 24)) >> 4);
#	else
quadint q;
q.low = q.high = 0;
fixmulaccum(&q, v->x, v->x);
fixmulaccum(&q, v->y, v->y);
fixmulaccum(&q, v->z, v->z);
if (!q.high)
	return fix_isqrt(fixquadadjust(&q));
if (q.high >= 0x800000)
	return (fix_isqrt(q.high) >> 8);
return (fix_isqrt((q.high<<8) + (q.low>>24)) >> 4);
#	endif
#endif
}

// ------------------------------------------------------------------------
//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
fix VmVecCopyNormalizeQuick(vmsVector *dest, vmsVector *src)
{
fix im = VmVecInvMag(src);
dest->x = FixMul (src->x, im);
dest->y = FixMul (src->y, im);
dest->z = FixMul (src->z, im);
return im;
}

#endif

// ------------------------------------------------------------------------
//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
fix VmVecNormalizeQuick(vmsVector *v)
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
vmsVector *VmVecNormal(vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
VmVecNormalize (VmVecPerp (dest, p0, p1, p2));
return dest;
}

// ------------------------------------------------------------------------
//make sure a vector is reasonably sized to go into a cross product
void CheckVec(vmsVector *vp)
{
	fix check;
	int cnt = 0;
	vmsVector v = *vp;

if (!(check = labs (v.x) | labs (v.y) | labs (v.z)))
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
v.x >>= cnt;
v.y >>= cnt;
v.z >>= cnt;
*vp = v;
}

// ------------------------------------------------------------------------
//computes cross product of two vectors. 
//Note: this magnitude of the resultant vector is the
//product of the magnitudes of the two source vectors.  This means it is
//quite easy for this routine to overflow and underflow.  Be careful that
//your inputs are ok.
//#ifndef __powerc
#if 0
vmsVector *VmVecCrossProd (vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
	double d;
	Assert(dest!=src0 && dest!=src1);

	d = (double)(src0->y) * (double)(src1->z);
	d += (double)-(src0->z) * (double)(src1->y);
	d /= 65536.0;
	if (d < 0.0)
		d = d - 1.0;
	dest->x = (fix)d;

	d = (double)(src0->z) * (double)(src1->x);
	d += (double)-(src0->x) * (double)(src1->z);
	d /= 65536.0;
	if (d < 0.0)
		d = d - 1.0;
	dest->y = (fix)d;

	d = (double)(src0->x) * (double)(src1->y);
	d += (double)-(src0->y) * (double)(src1->x);
	d /= 65536.0;
	if (d < 0.0)
		d = d - 1.0;
	dest->z = (fix)d;

	return dest;
}
#else

// ------------------------------------------------------------------------

vmsVector *VmVecCrossProd (vmsVector *dest, vmsVector *src0, vmsVector *src1)
{
#if 1//def _WIN32
QLONG q = mul64 (src0->y, src1->z);
Assert(dest!=src0 && dest!=src1);
q += mul64 (-src0->z, src1->y);
dest->x = (fix) (q >> 16);
q = mul64 (src0->z, src1->x);
q += mul64 (-src0->x, src1->z);
dest->y = (fix) (q >> 16);
q = mul64 (src0->x, src1->y);
q += mul64 (-src0->y, src1->x);
dest->z = (fix) (q >> 16);
#else
quadint q;
Assert(dest!=src0 && dest!=src1);
q.low = q.high = 0;
fixmulaccum(&q, src0->y, src1->z);
fixmulaccum(&q, -src0->z, src1->y);
dest->x = fixquadadjust(&q);

q.low = q.high = 0;
fixmulaccum(&q, src0->z, src1->x);
fixmulaccum(&q, -src0->x, src1->z);
dest->y = fixquadadjust(&q);

q.low = q.high = 0;
fixmulaccum(&q, src0->x, src1->y);
fixmulaccum(&q, -src0->y, src1->x);
dest->z = fixquadadjust(&q);
#endif
return dest;
}

#endif

// ------------------------------------------------------------------------
//computes non-normalized surface normal from three points. 
//returns ptr to dest
//dest CANNOT equal either source
vmsVector *VmVecPerp (vmsVector *dest, vmsVector *p0, vmsVector *p1, vmsVector *p2)
{
	vmsVector t0, t1;

VmVecSub(&t0, p1, p0);
VmVecSub(&t1, p2, p1);
CheckVec(&t0);
CheckVec(&t1);
return VmVecCrossProd(dest, &t0, &t1);
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
fixang a = fix_acos (VmVecDot (v0, v1));
if (fVec) {
	vmsVector t;
	if (VmVecDot (VmVecCrossProd (&t, v0, v1), fVec) < 0)
		a = -a;
	}
return a;
}

// ------------------------------------------------------------------------

vmsMatrix *SinCos2Matrix (vmsMatrix *m, fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh)
{
	fix sbsh, cbch, cbsh, sbch;

sbsh = FixMul (sinb, sinh);
cbch = FixMul (cosb, cosh);
cbsh = FixMul (cosb, sinh);
sbch = FixMul (sinb, cosh);
m->rVec.x = cbch + FixMul (sinp, sbsh);		//m1
m->uVec.z = sbsh + FixMul (sinp, cbch);		//m8
m->uVec.x = FixMul (sinp, cbsh) - sbch;		//m2
m->rVec.z = FixMul (sinp, sbch) - cbsh;		//m7
m->fVec.x = FixMul (sinh, cosp);				//m3
m->rVec.y = FixMul (sinb, cosp);				//m4
m->uVec.y = FixMul (cosb, cosp);				//m5
m->fVec.z = FixMul (cosh, cosp);				//m9
m->fVec.y = -sinp;								//m6
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
return SinCos2Matrix (m, sinp, cosp, sinb, cosb, sinh, cosh);
}

// ------------------------------------------------------------------------
//computes a matrix from a forward vector and an angle
vmsMatrix *VmVecAng2Matrix (vmsMatrix *m, vmsVector *v, fixang a)
{
	fix sinb, cosb, sinp, cosp;

FixSinCos (a, &sinb, &cosb);
sinp = -v->y;
cosp = fix_sqrt (f1_0 - FixMul (sinp, sinp));
return SinCos2Matrix (m, sinp, cosp, sinb, cosb, FixDiv(v->x, cosp), FixDiv(v->z, cosp));
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
		if (zvec->x==0 && zvec->z==0) {		//forward vec is straight up or down
			m->rVec.x = F1_0;
			m->uVec.z = (zvec->y < 0) ? F1_0 : -F1_0;
			m->rVec.y = m->rVec.z = m->uVec.x = m->uVec.y = 0;
			}
		else { 		//not straight up or down
			xvec->x = zvec->z;
			xvec->y = 0;
			xvec->z = -zvec->x;
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
		if (!(zvec->x || zvec->z)) {		//forward vec is straight up or down
			m->rVec.x = F1_0;
			m->uVec.z = (zvec->y < 0) ? F1_0 : -F1_0;
			m->rVec.y = m->rVec.z = m->uVec.x = m->uVec.y = 0;
			}
		else { 		//not straight up or down
			xvec->x = zvec->z;
			xvec->y = 0;
			xvec->z = -zvec->x;
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
if (gameOpts->render.bEnableSSE) {
		fVector	vf;
		fMatrix	mf, *mfP;

	VmsVecToFloat (&vf, src);
	if (m == &viewInfo.view [0])
		mfP = &viewInfo.viewf [0];
	else {
		VmsVecToFloat (&mf.fVec, &m->fVec);
		VmsVecToFloat (&mf.rVec, &m->rVec);
		VmsVecToFloat (&mf.uVec, &m->uVec);
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
	dest->x = (fix) (vf.p.x * 65536);
	dest->y = (fix) (vf.p.y * 65536);
	dest->z = (fix) (vf.p.z * 65536);
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
	dest->x = VmVecDot (src, &m->rVec);
	dest->y = VmVecDot (src, &m->uVec);
	dest->z = VmVecDot (src, &m->fVec);
	}
return dest;
}

// ------------------------------------------------------------------------

fMatrix *VmsMatToFloat (fMatrix *dest, vmsMatrix *src)
{
VmsVecToFloat (&dest->rVec, &src->rVec);
VmsVecToFloat (&dest->uVec, &src->uVec);
VmsVecToFloat (&dest->fVec, &src->fVec);
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
if (gameOpts->render.bEnableSSE) {
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

t = m->uVec.x;  m->uVec.x = m->rVec.y;  m->rVec.y = t;
t = m->fVec.x;  m->fVec.x = m->rVec.z;  m->rVec.z = t;
t = m->fVec.y;  m->fVec.y = m->uVec.z;  m->uVec.z = t;
return m;
}

// ------------------------------------------------------------------------
//copy and transpose a matrix. returns ptr to matrix
//dest CANNOT equal source. use VmTransposeMatrix() if this is the case
vmsMatrix *VmCopyTransposeMatrix (vmsMatrix *dest, vmsMatrix *src)
{
Assert(dest != src);
dest->rVec.x = src->rVec.x;
dest->rVec.y = src->uVec.x;
dest->rVec.z = src->fVec.x;
dest->uVec.x = src->rVec.y;
dest->uVec.y = src->uVec.y;
dest->uVec.z = src->fVec.y;
dest->fVec.x = src->rVec.z;
dest->fVec.y = src->uVec.z;
dest->fVec.z = src->fVec.z;
return dest;
}

// ------------------------------------------------------------------------
//multiply 2 matrices, fill in dest.  returns ptr to dest
//dest CANNOT equal either source
vmsMatrix *VmMatMul (vmsMatrix *dest, vmsMatrix *src0, vmsMatrix *src1)
{
	vmsVector	v;

Assert(dest!=src0 && dest!=src1);
v.x = src0->rVec.x;
v.y = src0->uVec.x;
v.z = src0->fVec.x;
dest->rVec.x = VmVecDot (&v, &src1->rVec);
dest->uVec.x = VmVecDot (&v, &src1->uVec);
dest->fVec.x = VmVecDot (&v, &src1->fVec);
v.x = src0->rVec.y;
v.y = src0->uVec.y;
v.z = src0->fVec.y;
dest->rVec.y = VmVecDot (&v, &src1->rVec);
dest->uVec.y = VmVecDot (&v, &src1->uVec);
dest->fVec.y = VmVecDot (&v, &src1->fVec);
v.x = src0->rVec.z;
v.y = src0->uVec.z;
v.z = src0->fVec.z;
dest->rVec.z = VmVecDot (&v, &src1->rVec);
dest->uVec.z = VmVecDot (&v, &src1->uVec);
dest->fVec.z = VmVecDot (&v, &src1->fVec);
return dest;
}
#endif

// ------------------------------------------------------------------------
//extract angles from a matrix 
vmsAngVec *VmExtractAnglesMatrix (vmsAngVec *a, vmsMatrix *m)
{
	fix sinh, cosh, cosp;

if (m->fVec.x==0 && m->fVec.z==0)		//zero head
	a->h = 0;
else
	a->h = fix_atan2(m->fVec.z, m->fVec.x);
FixSinCos(a->h, &sinh, &cosh);
if (abs(sinh) > abs(cosh))				//sine is larger, so use it
	cosp = FixDiv(m->fVec.x, sinh);
else											//cosine is larger, so use it
	cosp = FixDiv(m->fVec.z, cosh);
if (cosp==0 && m->fVec.y==0)
	a->p = 0;
else
	a->p = fix_atan2(cosp, -m->fVec.y);
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a->b = 0;
else {
	fix sinb, cosb;

	sinb = FixDiv (m->rVec.y, cosp);
	cosb = FixDiv (m->uVec.y, cosp);
	if (sinb==0 && cosb==0)
		a->b = 0;
	else
		a->b = fix_atan2(cosb, sinb);
	}
return a;
}

// ------------------------------------------------------------------------
//extract heading and pitch from a vector, assuming bank==0
vmsAngVec *VmExtractAnglesVecNorm (vmsAngVec *a, vmsVector *v)
{
a->b = 0;		//always zero bank
a->p = fix_asin (-v->y);
a->h = (v->x || v->z) ? fix_atan2(v->z, v->x) : 0;
return a;
}

// ------------------------------------------------------------------------
//extract heading and pitch from a vector, assuming bank==0
vmsAngVec *VmExtractAnglesVector (vmsAngVec *a, vmsVector *v)
{
	vmsVector t;

if (VmVecCopyNormalize(&t, v) != 0)
	VmExtractAnglesVecNorm (a, &t);
return a;
}

// ------------------------------------------------------------------------
//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix VmDistToPlane (vmsVector *checkp, vmsVector *norm, vmsVector *planep)
{
#if ENABLE_SSE
if (gameOpts->render.bEnableSSE) {
		fVector	c, p, n, t;
	
	VmsVecToFloat (&c, checkp);
	VmsVecToFloat (&p, planep);
	VmsVecToFloat (&n, norm);
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

	return VmVecDot (VmVecSub (&t, checkp, planep), norm);
	}
}

// ------------------------------------------------------------------------

vmsVector *VmVecMake (vmsVector *v, fix x, fix y, fix z)
{
v->x = x; 
v->y = y; 
v->z = z;
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
i->x = (fix) (l * (double) a->x);
i->y = (fix) (l * (double) a->y);
i->z = (fix) (l * (double) a->z);
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

i.x = (fix) (l * (double) a->x);
i.y = (fix) (l * (double) a->y);
i.z = (fix) (l * (double) a->z);
return !(VmBehindPlane (n, p1, p2, &i) || 
			VmBehindPlane (n, p2, p3, &i) ||
			VmBehindPlane (n, p3, p1, &i));
}

// ------------------------------------------------------------------------
// eof
