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

#include <stdlib.h>
#include <math.h>

#include "inferno.h"
#include "error.h"
#include "maths.h"

#if NO_FIX_INLINE
#ifdef _WIN32
#pragma message ("warning: FIX NOT INLINED")
#else
// #warning "FIX NOT INLINED"        fixc is now stable
#endif
#endif

extern ubyte guess_table [];
extern short sincos_table [];
extern ushort asin_table [];
extern ushort acos_table [];
extern fix isqrt_guess_table [];

// ------------------------------------------------------------------------
//multiply two ints & add 64-bit result to 64-bit sum
void FixMulAccum (tQuadInt *q, fix a, fix b)
{
	u_int32_t	aa, bb;
	u_int32_t	ah, al, bh, bl;
	u_int32_t	t, c = 0, old;
	int			neg = ((a ^ b) < 0);

aa = labs(a); 
bb = labs(b);
ah = aa >> 16;  
al = aa & 0xffff;
bh = bb >> 16;  
bl = bb & 0xffff;
t = ah * bl + bh * al;
if (neg)
	FixQuadNegate (q);
old = q->low;
q->low += al * bl;
if (q->low < old) 
	q->high++;
old = q->low;
q->low += (t << 16);
if (q->low < old)
	q->high++;
q->high += ah * bh + (t >> 16) + c;
if (neg)
	FixQuadNegate (q);
}

// ------------------------------------------------------------------------

#define EPSILON (F1_0/100)

// ------------------------------------------------------------------------
//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.

fixang FixAtan2 (fix cos, fix sin)
{
	double d;
	double dsin, dcos;
	fixang t;

dsin = (double) sin;
dcos = (double) cos;
d = sqrt ((dsin * dsin) + (dcos * dcos));
if (d == 0.0)
	return 0;

if (abs(sin) < abs(cos)) {				//sin is smaller, use arcsin
	t = FixASin ((fix) ((dsin / d) * 65536.0));
	if (cos < 0)
		t = 0x8000 - t;
	return t;
	}
else {
	t = FixACos ((fix) ((dcos / d) * 65536.0));
	return (sin < 0) ? -t : t;
	}
}

// ------------------------------------------------------------------------

//divide a tQuadInt by a fix, returning a fix
int32_t FixDivQuadLong (u_int32_t nl, u_int32_t nh, u_int32_t d)
{
	int i;
	u_int32_t tmp0;
	ubyte tmp1;
	u_int32_t r;
	ubyte T,Q,M;

	r = 0;

Q = ((nh & 0x80000000) != 0);
M = ((d & 0x80000000) != 0);
T = (M != Q);

if (M == 0) {
	for (i = 0; i < 32; i++) {
		r <<= 1;
		r |= T;
		T = ((nl & 0x80000000L) != 0);
		nl <<= 1;
		if (Q == 0) {
			Q = (ubyte)((0x80000000L & nh) != 0 );
			nh = (nh << 1) | (u_int32_t)T;
			tmp0 = nh;
			nh -= d;
			tmp1 = (nh>tmp0);
			if (Q == 0)
				Q = tmp1;
			else
				Q = (ubyte)(tmp1 == 0);
			}
		else if (Q == 1) {
			Q = (ubyte)((0x80000000L & nh) != 0 );
			nh = (nh << 1) | (u_int32_t)T;
			tmp0 = nh;
			nh += d;
			tmp1 = (nh < tmp0);
			if (Q == 0)
				Q = tmp1;
			else
				Q = (ubyte)(tmp1 == 0);
			}
		T = (Q == M);
		}
	}
else {
	for (i = 0; i < 32; i++ ) {
		r <<= 1;
		r |= T;
		T = ((nl & 0x80000000L) != 0);
		nl <<= 1;
		if (Q == 0) {
			Q = (ubyte)((0x80000000L & nh) != 0 );
			nh = (nh << 1) | (u_int32_t)T;
			tmp0 = nh;
			nh += d;
			tmp1 = (nh < tmp0);
			if (Q == 1)
				Q = tmp1;
			else
				Q = (ubyte)(tmp1 == 0);
			}
		else if (Q == 1) {
			Q = (ubyte) ((0x80000000L & nh) != 0);
			nh = (nh << 1) | (u_int32_t) T;
			tmp0 = nh;
			nh = nh - d;
			tmp1 = (nh > tmp0);
			if (Q == 1)
				Q = tmp1;
			else
				Q = (ubyte) (tmp1 == 0);
			}
		T = (Q == M);
		}
	}
return (r << 1) | T;
}

// ------------------------------------------------------------------------

uint FixDivQuadLongU (uint nl, uint nh, uint d)
{
return (uint) (((u_int64_t) nl | (((u_int64_t) nh) << 32)) / ((u_int64_t) d));
}

// ------------------------------------------------------------------------

u_int32_t QuadSqrt (u_int32_t low,int32_t high)
{
	int			i, cnt;
	u_int32_t	r, old_r, t;
	tQuadInt		tq;

if (high < 0)
	return 0;

if ((high == 0) && ((int32_t) low >= 0))
	return LongSqrt ((int32_t) low);

if (high & 0xff000000) {
	cnt = 12 + 16; 
	i = high >> 24;
	}
else if (high & 0xff0000) {
	cnt = 8 + 16; 
	i = high >> 16;
	}
else if (high & 0xff00) {
	cnt = 4 + 16; 
	i = high >> 8;
	}
else {
	cnt = 0 + 16; 
	i = high;
	}

r = guess_table [i] << cnt;
//quad loop usually executed 4 times
r = FixDivQuadLongU (low, high, r) / 2 + r / 2;
r = FixDivQuadLongU (low, high, r) / 2 + r / 2;
r = FixDivQuadLongU (low, high, r) / 2 + r / 2;
do {
	old_r = r;
	t = FixDivQuadLongU (low, high, r);
	if (t == r)	//got it!
		return r;
	r = t / 2 + r / 2;
	} while ((r != t) && (r != old_r));
t = FixDivQuadLongU (low, high, r);
//edited 05/17/99 Matt Mueller - tq.high is undefined here.. so set them to = 0
tq.low = tq.high = 0;
//end edit -MM
FixMulAccum (&tq, r, t);
if ((tq.low != low) || (tq.high != high))
	r++;
return r;
}

// ------------------------------------------------------------------------

//computes the square root of a long, returning a short
ushort LongSqrt (int32_t a)
{
int cnt, r, old_r, t;

if (a <= 0)
	return 0;
if (a & 0xff000000)
	cnt = 12;
else if (a & 0xff0000)
	cnt = 8;
else if (a & 0xff00)
	cnt = 4;
else
	cnt = 0;

r = guess_table [(a >> cnt) & 0xff] << cnt;

//the loop nearly always executes 3 times, so we'll unroll it 2 times and
//not do any checking until after the third time.  By my calcutations, the
//loop is executed 2 times in 99.97% of cases, 3 times in 93.65% of cases, 
//four times in 16.18% of cases, and five times in 0.44% of cases.  It never
//executes more than five times.  By timing, I determined that is is faster
//to always execute three times and not check for termination the first two
//times through.  This means that in 93.65% of cases, we save 6 cmp/jcc pairs,
//and in 6.35% of cases we do an extra divide.  In real life, these numbers
//might not be the same.

r = ((a / r) + r) / 2;
r = ((a / r) + r) / 2;
do {
	old_r = r;
	t = a / r;
	if (t == r)	//got it!
		return r;
 	r = (t + r) / 2;
	} while ((r != t) && (r != old_r));
if (a % r)
	r++;
return r;
}

// ------------------------------------------------------------------------

fix FixSqrt(fix a)
{
return ((fix) LongSqrt (a)) << 8;
}

// ------------------------------------------------------------------------
//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//with interpolation

void FixSinCos (fix a, fix *s, fix *c)
{
if (gameOpts->render.nMathFormat == 2) {
	double d = (double) (a * 2.0 * Pi) / F1_0;
	if (s)
		*s = (fix) (sin (d) * F1_0);
	if (c)
		*c = (fix) (cos (d) * F1_0);
	}
else  {
	int i = (a >> 8) & 0xff;
	int f = a & 0xff;

	if (s) {
		fix ss = sincos_table [i];
		*s = (ss + (((sincos_table [i+1] - ss) * f) >> 8)) << 2;
		}
	if (c) {
		fix cc = sincos_table [i+64];
		*c = (cc + (((sincos_table [i+64+1] - cc) * f) >> 8)) << 2;
		}
	}
}

// ------------------------------------------------------------------------
//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//no interpolation
void FixFastSinCos (fix a,fix *s,fix *c)
{
	int i = (a >> 8) & 0xff;

if (s) 
	*s = sincos_table [i] << 2;
if (c) 
	*c = sincos_table [i+64] << 2;
}

// ------------------------------------------------------------------------
//compute inverse sine
fixang FixASin (fix v)
{
	fix	vv = labs(v);
	int	i, f, aa;

if (vv >= f1_0)		//check for out of range
	return 0x4000;
i = (vv >> 8) & 0xff;
f = vv & 0xff;
aa = asin_table [i];
aa = aa + (((asin_table [i+1] - aa) * f) >> 8);
if (v < 0)
	aa = -aa;
return aa;
}

// ------------------------------------------------------------------------
//compute inverse cosine
fixang FixACos(fix v)
{
	fix	vv = labs (v);
	int	i, f, aa;

if (vv >= f1_0)		//check for out of range
	return 0;
i = (vv >> 8) & 0xff;
f = vv & 0xff;
aa = acos_table [i];
aa = aa + (((acos_table [i+1] - aa) * f)>>8);
if (v < 0)
	aa = 0x8000 - aa;
return aa;
}

#define TABLE_SIZE 1024

// ------------------------------------------------------------------------
//for passed value a, returns 1/sqrt(a) 
fix FixISqrt (fix a)
{
	int i, b = a;
	int cnt = 0;
	int r;

if (a == 0) 
	return 0;
while (b >= TABLE_SIZE) {
	b >>= 1;
	cnt++;
	}
r = isqrt_guess_table [b] >> ((cnt+1)/2);
for (i = 0; i < 3; i++ ) {
	int old_r = r;
	r = FixMul (((3*65536) - FixMul(FixMul (r, r), a)), r) / 2;
	if (old_r >= r) 
		return (r + old_r) / 2;
	}
return r;
}

// ------------------------------------------------------------------------
// eof
