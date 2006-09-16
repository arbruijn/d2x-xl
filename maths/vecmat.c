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

/*
 *
 * C version of vecmat library
 *
 * Old Log:
 * Revision 1.5  1995/10/30  11:08:16  allender
 * fix check_vec to return if vector is the NULL vector
 *
 * Revision 1.4  1995/09/23  09:38:14  allender
 * removed calls for PPC that are now handled in asm
 *
 * Revision 1.3  1995/08/31  15:50:24  allender
 * fixing up of functions for PPC
 *
 * Revision 1.2  1995/07/05  16:40:21  allender
 * some vecmat stuff might be using isqrt -- commented out
 * for now
 *
 * Revision 1.1  1995/04/17  16:18:02  allender
 * Initial revision
 *
 *
 * --- PC RCS Information ---
 * Revision 1.1  1995/03/08  15:56:50  matt
 * Initial revision
 *
 *
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

#define EXACT_VEC_MAG 1

#ifndef ASM_VECMAT
vms_vector vmd_zero_vector = {0, 0, 0};
vms_matrix vmd_identity_matrix = { { f1_0, 0, 0 }, 
                                   { 0, f1_0, 0 }, 
                                   { 0, 0, f1_0 } };

// ------------------------------------------------------------------------
//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecInc() if so
#if !INLINE_VEC_ADD
vms_vector *VmVecAdd(vms_vector *dest, vms_vector *src0, vms_vector *src1)
{
dest->x = src0->x + src1->x;
dest->y = src0->y + src1->y;
dest->z = src0->z + src1->z;
return dest;
}

// ------------------------------------------------------------------------
//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use VmVecDec() if so
vms_vector *VmVecSub(vms_vector *dest, vms_vector *src0, vms_vector *src1)
{
dest->x = src0->x - src1->x;
dest->y = src0->y - src1->y;
dest->z = src0->z - src1->z;
return dest;
}

// ------------------------------------------------------------------------
//adds one vector to another. returns ptr to dest
//dest can equal source
vms_vector *VmVecInc(vms_vector *dest, vms_vector *src)
{
dest->x += src->x;
dest->y += src->y;
dest->z += src->z;
return dest;
}

// ------------------------------------------------------------------------
//subs one vector from another, returns ptr to dest
//dest can equal source
vms_vector *VmVecDec(vms_vector *dest, vms_vector *src)
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
vms_vector *VmVecAvg(vms_vector *dest, vms_vector *src0, vms_vector *src1)
{
dest->x = (src0->x + src1->x)/2;
dest->y = (src0->y + src1->y)/2;
dest->z = (src0->z + src1->z)/2;
return dest;
}

// ------------------------------------------------------------------------
//averages four vectors. returns ptr to dest
//dest can equal any source
vms_vector *VmVecAvg4(vms_vector *dest, 
								vms_vector *src0, vms_vector *src1, 
								vms_vector *src2, vms_vector *src3)
{
dest->x = (src0->x + src1->x + src2->x + src3->x)/4;
dest->y = (src0->y + src1->y + src2->y + src3->y)/4;
dest->z = (src0->z + src1->z + src2->z + src3->z)/4;
return dest;
}

// ------------------------------------------------------------------------
//scales a vector in place.  returns ptr to vector
vms_vector *VmVecScale (vms_vector *dest, fix s)
{
dest->x = fixmul (dest->x, s);
dest->y = fixmul (dest->y, s);
dest->z = fixmul (dest->z, s);
return dest;
}

// ------------------------------------------------------------------------
//scales and copies a vector.  returns ptr to dest
vms_vector *VmVecCopyScale (vms_vector *dest, vms_vector *src, fix s)
{
dest->x = fixmul (src->x, s);
dest->y = fixmul (src->y, s);
dest->z = fixmul (src->z, s);
return dest;
}

// ------------------------------------------------------------------------
//scales a vector, adds it to another, and stores in a 3rd vector
//dest = src1 + k * src2
vms_vector *VmVecScaleAdd (vms_vector *dest, vms_vector *src1, vms_vector *src2, fix k)
{
dest->x = src1->x + fixmul (src2->x, k);
dest->y = src1->y + fixmul (src2->y, k);
dest->z = src1->z + fixmul (src2->z, k);
return dest;
}

// ------------------------------------------------------------------------
//scales a vector and adds it to another
//dest += k * src
vms_vector *VmVecScaleInc (vms_vector *dest, vms_vector *src, fix k)
{
dest->x += fixmul (src->x, k);
dest->y += fixmul (src->y, k);
dest->z += fixmul (src->z, k);
return dest;
}

// ------------------------------------------------------------------------
//scales a vector in place, taking n/d for scale.  returns ptr to vector
//dest *= n/d
vms_vector *VmVecScaleFrac (vms_vector *dest, fix n, fix d)
{
#if 1 // DPH: Kludge: this was overflowing a lot, so I made it use the FPU.
double nd = f2fl(n) / f2fl(d);
dest->x = fl2f(f2fl(dest->x) * nd);
dest->y = fl2f(f2fl(dest->y) * nd);
dest->z = fl2f(f2fl(dest->z) * nd);
#else
dest->x = fixmuldiv(dest->x, n, d);
dest->y = fixmuldiv(dest->y, n, d);
dest->z = fixmuldiv(dest->z, n, d);
#endif
return dest;
}

// ------------------------------------------------------------------------

fix VmVecDotProd (vms_vector *v0, vms_vector *v1)
{
#if 1//def _WIN32
if (gameOpts->render.nMathFormat == 2)
	return (fix) (((double) v0->x * (double) v1->x + (double) v0->y * (double) v1->y + (double) v0->z * (double) v1->z) / 65536.0);
else {
	QLONG q = mul64 (v0->x, v1->x);
	q += mul64 (v0->y, v1->y);
	q += mul64 (v0->z, v1->z);
	return (fix) (q / 65536); //>> 16);
	}
#	if 0//def _DEBUG
{
quadint i;
fix h;
i.low = i.high = 0;
fixmulaccum(&i, v0->x, v1->x);
fixmulaccum(&i, v0->y, v1->y);
fixmulaccum(&i, v0->z, v1->z);
h = fixquadadjust(&i);
CBRK (h != (fix) (q >> 16));
}
#	endif
#else
quadint i;
i.low = i.high = 0;
fixmulaccum(&i, v0->x, v1->x);
fixmulaccum(&i, v0->y, v1->y);
fixmulaccum(&i, v0->z, v1->z);
return fixquadadjust(&i);
#endif
}

// ------------------------------------------------------------------------

fix vm_vec_dot3(fix x, fix y, fix z, vms_vector *v)
{
#if 1//def _WIN32
if (gameOpts->render.nMathFormat == 2)
	return (fix) (((double) x * (double) v->x + (double) y * (double) v->y + (double) z * (double) v->z) / 65536.0);
else {
	QLONG q = mul64 (x, v->x);
	q += mul64 (y, v->y);
	q += mul64 (z, v->z);
	return (fix) (q >> 16);
	}
#else
quadint q;
q.low = q.high = 0;
fixmulaccum(&q, x, v->x);
fixmulaccum(&q, y, v->y);
fixmulaccum(&q, z, v->z);
return fixquadadjust(&q);
#endif
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
fix VmVecMag (vms_vector *v)
{
if (gameOpts->render.nMathFormat == 2)
	return (fix) sqrt (sqrd ((double) v->x) + sqrd ((double) v->y) + sqrd ((double) v->z)); 
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
//computes the distance between two points. (does sub and mag)
fix VmVecDist(vms_vector *v0, vms_vector *v1)
{
vms_vector t;
return VmVecMag (VmVecSub (&t, v0, v1));
}

// ------------------------------------------------------------------------
//computes an approximation of the magnitude of the vector
//uses dist = largest + next_largest*3/8 + smallest*3/16
fix FixVecMagQuick (fix a, fix b, fix c)
{
#if 0//def _WIN32
#	if 1
	vms_vector v = {a, b, c};

return VmVecMag (&v);
#	else
	_asm {
		mov	eax, a
		or		eax, eax
		jns	aPos
		neg	eax
	aPos:
		mov	ebx, b
		or		ebx, ebx
		jns	bPos
		neg	ebx
	bPos:
		mov	ecx, c
		or		ecx, ecx
		jns	cPos
		neg	ecx
	cPos:
		cmp	eax, ebx
		jge	agtb
		xchg	eax, ebx
	agtb:
		cmp	ebx, ecx
		jge	bgtc
		xchg	ebx, ecx
		cmp	eax, ebx
		jge	bgtc
		xchg	eax, ebx
	bgtc:
		shr	ebx, 2
		shr	ecx, 3
		add	ebx, ecx
		add	eax, ebx
		shr	ebx, 1
		add	eax, ebx
	}
#	endif
#else
#	if 1
	vms_vector v = {a, b, c};

return VmVecMag (&v);
#	else	// the following code works on Win32, but not on Linux and Mac OS X. Duh.
	fix bc, t;

a = labs (a);
b = labs (b);
c = labs (c);
if (a < b)
	t=a; a=b; b=t;
if (b < c) {
	t=b; b=c; c=t;
	if (a < b)
		t=a; a=b; b=t;
	}
bc = (b>>2) + (c>>3);
return a + bc + (bc >> 1);
#	endif
#endif
}

// ------------------------------------------------------------------------
//computes an approximation of the distance between two points.
//uses dist = largest + next_largest*3/8 + smallest*3/16
#if 0//QUICK_VEC_MATH
fix VmVecDistQuick (vms_vector *v0, vms_vector *v1)
{
return FixVecMagQuick (v0->x - v1->x, v0->y - v1->y, v0->z - v1->z);
}
#endif

// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec
fix VmVecCopyNormalize (vms_vector *dest, vms_vector *src)
{
fix m = VmVecMag (src);
if (m) {
	dest->x = fixdiv(src->x, m);
	dest->y = fixdiv(src->y, m);
	dest->z = fixdiv(src->z, m);
	}
return m;
}

// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec
#if 0
fix VmVecNormalize(vms_vector *v)
{
	return VmVecCopyNormalize(v, v);
}
#endif


// ------------------------------------------------------------------------
//normalize a vector. returns mag of source vec. uses approx mag

#if EXACT_VEC_MAG
fix VmVecCopyNormalizeQuick(vms_vector *dest, vms_vector *src)
{
fix m = VmVecMag (src);
if (m) {
	dest->x = fixdiv(src->x, m);
	dest->y = fixdiv(src->y, m);
	dest->z = fixdiv(src->z, m);
	}
return m;
}

#else
//these routines use an approximation for 1/sqrt

// ------------------------------------------------------------------------
//returns approximation of 1/magnitude of a vector
fix vm_vec_invmag(vms_vector *v)
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
fix VmVecCopyNormalizeQuick(vms_vector *dest, vms_vector *src)
{
fix im = vm_vec_invmag(src);
dest->x = fixmul(src->x, im);
dest->y = fixmul(src->y, im);
dest->z = fixmul(src->z, im);
return im;
}

#endif

// ------------------------------------------------------------------------
//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
fix VmVecNormalizeQuick(vms_vector *v)
{
return VmVecCopyNormalizeQuick(v, v);
}

// ------------------------------------------------------------------------
//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns 1/mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix VmVecNormalizedDirQuick(vms_vector *dest, vms_vector *end, vms_vector *start)
{
return VmVecNormalizeQuick (VmVecSub (dest, end, start));
}

// ------------------------------------------------------------------------
//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix VmVecNormalizedDir(vms_vector *dest, vms_vector *end, vms_vector *start)
{
return VmVecNormalize (VmVecSub (dest, end, start));
}

// ------------------------------------------------------------------------
//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
vms_vector *VmVecNormal(vms_vector *dest, vms_vector *p0, vms_vector *p1, vms_vector *p2)
{
VmVecNormalize (VmVecPerp (dest, p0, p1, p2));
return dest;
}

// ------------------------------------------------------------------------
//make sure a vector is reasonably sized to go into a cross product
void check_vec(vms_vector *vp)
{
	fix check;
	int cnt = 0;
	vms_vector v = *vp;

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
vms_vector *VmVecCrossProd(vms_vector *dest, vms_vector *src0, vms_vector *src1)
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

vms_vector *VmVecCrossProd(vms_vector *dest, vms_vector *src0, vms_vector *src1)
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
vms_vector *VmVecPerp(vms_vector *dest, vms_vector *p0, vms_vector *p1, vms_vector *p2)
{
	vms_vector t0, t1;

VmVecSub(&t0, p1, p0);
VmVecSub(&t1, p2, p1);
check_vec(&t0);
check_vec(&t1);
return VmVecCrossProd(dest, &t0, &t1);
}

// ------------------------------------------------------------------------
//computes the delta angle between two vectors. 
//vectors need not be normalized. if they are, call VmVecDeltaAngNorm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
fixang VmVecDeltaAng(vms_vector *v0, vms_vector *v1, vms_vector *fvec)
{
vms_vector t0, t1;

VmVecCopyNormalize (&t0, v0);
VmVecCopyNormalize (&t1, v1);
return VmVecDeltaAngNorm (&t0, &t1, fvec);
}

// ------------------------------------------------------------------------
//computes the delta angle between two normalized vectors. 
fixang VmVecDeltaAngNorm (vms_vector *v0, vms_vector *v1, vms_vector *fvec)
{
fixang a = fix_acos (VmVecDot (v0, v1));
if (fvec) {
	vms_vector t;
	if (VmVecDot (VmVecCrossProd (&t, v0, v1), fvec) < 0)
		a = -a;
	}
return a;
}

// ------------------------------------------------------------------------

vms_matrix *sincos_2_matrix(vms_matrix *m, fix sinp, fix cosp, fix sinb, fix cosb, fix sinh, fix cosh)
{
	fix sbsh, cbch, cbsh, sbch;

sbsh = fixmul(sinb, sinh);
cbch = fixmul(cosb, cosh);
cbsh = fixmul(cosb, sinh);
sbch = fixmul(sinb, cosh);
m->rvec.x = cbch + fixmul(sinp, sbsh);		//m1
m->uvec.z = sbsh + fixmul(sinp, cbch);		//m8
m->uvec.x = fixmul(sinp, cbsh) - sbch;		//m2
m->rvec.z = fixmul(sinp, sbch) - cbsh;		//m7
m->fvec.x = fixmul(sinh, cosp);				//m3
m->rvec.y = fixmul(sinb, cosp);				//m4
m->uvec.y = fixmul(cosb, cosp);				//m5
m->fvec.z = fixmul(cosh, cosp);				//m9
m->fvec.y = -sinp;								//m6
return m;
}

// ------------------------------------------------------------------------
//computes a matrix from a set of three angles.  returns ptr to matrix
vms_matrix *VmAngles2Matrix (vms_matrix *m, vms_angvec *a)
{
fix sinp, cosp, sinb, cosb, sinh, cosh;
fix_sincos(a->p, &sinp, &cosp);
fix_sincos(a->b, &sinb, &cosb);
fix_sincos(a->h, &sinh, &cosh);
return sincos_2_matrix(m, sinp, cosp, sinb, cosb, sinh, cosh);
}

// ------------------------------------------------------------------------
//computes a matrix from a forward vector and an angle
vms_matrix *VmVecAng2Matrix(vms_matrix *m, vms_vector *v, fixang a)
{
	fix sinb, cosb, sinp, cosp;

fix_sincos(a, &sinb, &cosb);
sinp = -v->y;
cosp = fix_sqrt(f1_0 - fixmul(sinp, sinp));
return sincos_2_matrix (m, sinp, cosp, sinb, cosb, fixdiv(v->x, cosp), fixdiv(v->z, cosp));
}

// ------------------------------------------------------------------------
//computes a matrix from one or more vectors. The forward vector is required, 
//with the other two being optional.  If both up & right vectors are passed, 
//the up vector is used.  If only the forward vector is passed, a bank of
//zero is assumed
//returns ptr to matrix
vms_matrix *VmVector2Matrix(vms_matrix *m, vms_vector *fvec, vms_vector *uvec, vms_vector *rvec)
{
	vms_vector	*xvec = &m->rvec, 
					*yvec = &m->uvec, 
					*zvec = &m->fvec;
	Assert(fvec != NULL);
if (VmVecCopyNormalize (zvec, fvec) == 0) {
	Int3();		//forward vec should not be zero-length
	return m;
	}
if (uvec == NULL) {
	if (rvec == NULL) {		//just forward vec

bad_vector2:
;
		if (zvec->x==0 && zvec->z==0) {		//forward vec is straight up or down
			m->rvec.x = F1_0;
			m->uvec.z = (zvec->y < 0) ? F1_0 : -F1_0;
			m->rvec.y = m->rvec.z = m->uvec.x = m->uvec.y = 0;
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
		if (VmVecCopyNormalize (xvec, rvec) == 0)
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
	if (VmVecCopyNormalize (yvec, uvec) == 0)
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
vms_matrix *VmVector2MatrixNorm(vms_matrix *m, vms_vector *fvec, vms_vector *uvec, vms_vector *rvec)
{
	vms_vector	*xvec = &m->rvec, 
					*yvec = &m->uvec, 
					*zvec = &m->fvec;

Assert(fvec != NULL);
if (!uvec) {
	if (!rvec) {		//just forward vec
bad_vector2:
;
		if (!(zvec->x || zvec->z)) {		//forward vec is straight up or down
			m->rvec.x = F1_0;
			m->uvec.z = (zvec->y < 0) ? F1_0 : -F1_0;
			m->rvec.y = m->rvec.z = m->uvec.x = m->uvec.y = 0;
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
vms_vector *VmVecRotate (vms_vector *dest, vms_vector *src, vms_matrix *m)
{
Assert (dest != src);
dest->x = VmVecDot (src, &m->rvec);
dest->y = VmVecDot (src, &m->uvec);
dest->z = VmVecDot (src, &m->fvec);
return dest;
}

// ------------------------------------------------------------------------
//transpose a matrix in place. returns ptr to matrix
vms_matrix *VmTransposeMatrix(vms_matrix *m)
{
	fix t;

	t = m->uvec.x;  m->uvec.x = m->rvec.y;  m->rvec.y = t;
	t = m->fvec.x;  m->fvec.x = m->rvec.z;  m->rvec.z = t;
	t = m->fvec.y;  m->fvec.y = m->uvec.z;  m->uvec.z = t;

	return m;
}

// ------------------------------------------------------------------------
//copy and transpose a matrix. returns ptr to matrix
//dest CANNOT equal source. use VmTransposeMatrix() if this is the case
vms_matrix *VmCopyTransposeMatrix(vms_matrix *dest, vms_matrix *src)
{
	Assert(dest != src);

	dest->rvec.x = src->rvec.x;
	dest->rvec.y = src->uvec.x;
	dest->rvec.z = src->fvec.x;

	dest->uvec.x = src->rvec.y;
	dest->uvec.y = src->uvec.y;
	dest->uvec.z = src->fvec.y;

	dest->fvec.x = src->rvec.z;
	dest->fvec.y = src->uvec.z;
	dest->fvec.z = src->fvec.z;

	return dest;
}

// ------------------------------------------------------------------------
//multiply 2 matrices, fill in dest.  returns ptr to dest
//dest CANNOT equal either source
vms_matrix *VmMatMul(vms_matrix *dest, vms_matrix *src0, vms_matrix *src1)
{
	Assert(dest!=src0 && dest!=src1);

	dest->rvec.x = vm_vec_dot3(src0->rvec.x, src0->uvec.x, src0->fvec.x, &src1->rvec);
	dest->uvec.x = vm_vec_dot3(src0->rvec.x, src0->uvec.x, src0->fvec.x, &src1->uvec);
	dest->fvec.x = vm_vec_dot3(src0->rvec.x, src0->uvec.x, src0->fvec.x, &src1->fvec);

	dest->rvec.y = vm_vec_dot3(src0->rvec.y, src0->uvec.y, src0->fvec.y, &src1->rvec);
	dest->uvec.y = vm_vec_dot3(src0->rvec.y, src0->uvec.y, src0->fvec.y, &src1->uvec);
	dest->fvec.y = vm_vec_dot3(src0->rvec.y, src0->uvec.y, src0->fvec.y, &src1->fvec);

	dest->rvec.z = vm_vec_dot3(src0->rvec.z, src0->uvec.z, src0->fvec.z, &src1->rvec);
	dest->uvec.z = vm_vec_dot3(src0->rvec.z, src0->uvec.z, src0->fvec.z, &src1->uvec);
	dest->fvec.z = vm_vec_dot3(src0->rvec.z, src0->uvec.z, src0->fvec.z, &src1->fvec);

	return dest;
}
#endif

// ------------------------------------------------------------------------
//extract angles from a matrix 
vms_angvec *VmExtractAnglesMatrix(vms_angvec *a, vms_matrix *m)
{
	fix sinh, cosh, cosp;

if (m->fvec.x==0 && m->fvec.z==0)		//zero head
	a->h = 0;
else
	a->h = fix_atan2(m->fvec.z, m->fvec.x);
fix_sincos(a->h, &sinh, &cosh);
if (abs(sinh) > abs(cosh))				//sine is larger, so use it
	cosp = fixdiv(m->fvec.x, sinh);
else											//cosine is larger, so use it
	cosp = fixdiv(m->fvec.z, cosh);
if (cosp==0 && m->fvec.y==0)
	a->p = 0;
else
	a->p = fix_atan2(cosp, -m->fvec.y);
if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
	a->b = 0;
else {
	fix sinb, cosb;

	sinb = fixdiv(m->rvec.y, cosp);
	cosb = fixdiv(m->uvec.y, cosp);
	if (sinb==0 && cosb==0)
		a->b = 0;
	else
		a->b = fix_atan2(cosb, sinb);
	}
return a;
}

// ------------------------------------------------------------------------
//extract heading and pitch from a vector, assuming bank==0
vms_angvec *vm_extract_angles_vector_normalized (vms_angvec *a, vms_vector *v)
{
a->b = 0;		//always zero bank
a->p = fix_asin (-v->y);
a->h = (v->x || v->z) ? fix_atan2(v->z, v->x) : 0;
return a;
}

// ------------------------------------------------------------------------
//extract heading and pitch from a vector, assuming bank==0
vms_angvec *VmExtractAnglesVector(vms_angvec *a, vms_vector *v)
{
	vms_vector t;

if (VmVecCopyNormalize(&t, v) != 0)
	vm_extract_angles_vector_normalized(a, &t);
return a;
}

// ------------------------------------------------------------------------
//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix VmDistToPlane (vms_vector *checkp, vms_vector *norm, vms_vector *planep)
{
vms_vector t;
return VmVecDot (VmVecSub (&t, checkp, planep), norm);
}

// ------------------------------------------------------------------------

vms_vector *VmVecMake(vms_vector *v, fix x, fix y, fix z)
{
v->x = x; 
v->y = y; 
v->z = z;
return v;
}

// ------------------------------------------------------------------------
// compute intersection of a line through a point a, with the line being orthogonal relative
// to the plane given by the normal n and a point p lieing in the plane, and store it in i.

vms_vector *VmPlaneProjection (vms_vector *i, vms_vector *n, vms_vector *p, vms_vector *a)
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

inline int vm_behind_plane (vms_vector *n, vms_vector *p1, vms_vector *p2, vms_vector *i)
{
	vms_vector	t;
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

int VmTriangleHitTest (vms_vector *n, vms_vector *p1, vms_vector *p2, vms_vector *p3, vms_vector *i)
{
return !(vm_behind_plane (n, p1, p2, i) || 
			vm_behind_plane (n, p2, p3, i) ||
			vm_behind_plane (n, p3, p1, i));
}

// ------------------------------------------------------------------------
// compute intersection of orthogonal line on plane n, p1 through point a
// and check whether the intersection lies inside the triangle p1, p2, p3
// in one step.

int VmTriangleHitTestQuick (vms_vector *n, vms_vector *p1, vms_vector *p2, vms_vector *p3, vms_vector *a)
{
	vms_vector	i;
	double l = (double) -VmVecDot (n, p1) / (double) VmVecDot (n, a);

i.x = (fix) (l * (double) a->x);
i.y = (fix) (l * (double) a->y);
i.z = (fix) (l * (double) a->z);
return !(vm_behind_plane (n, p1, p2, &i) || 
			vm_behind_plane (n, p2, p3, &i) ||
			vm_behind_plane (n, p3, p1, &i));
}

// ------------------------------------------------------------------------
// eof
