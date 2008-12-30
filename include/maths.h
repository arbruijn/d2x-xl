/* Maths[HA] library header file */

#ifndef _MATHS_H
#define _MATHS_H

#define NO_FIX_INLINE 0
#define ALT_SQRT 2

#include <stdlib.h>
#include <cmath>
#include "pstypes.h"

#define D_RAND_MAX 32767

//=============================== FIXED POINT ===============================

#ifndef Pi
#  define  Pi    3.141592653589793240
#endif


typedef int32_t fix;		//16 bits int, 16 bits frac
typedef int16_t fixang;		//angles

typedef struct tQuadInt {// integer 64 bit, previously called "quad"
	u_int32_t low;
   int32_t high;
} tQuadInt;

//Convert an int to a fix
#define I2X(i) ((static_cast<fix> (i)) * 65536)

//Get the int part of a fix
#define X2I(_f) ((_f) / 65536)

//Get the int part of a fix, with rounding
#define X2IR(_f) (((_f) + (I2X (1) / 2)) / 65536)

//Convert fix to double and double to fix
#define X2F(_f) ((static_cast<float> (_f)) / 65536.0f)
#define X2D(_f) ((static_cast<double> (_f)) / (double) 65536.0)
#define F2X(_f) (static_cast<fix> ((_f) * 65536))

//Some handy constants
//#define F0_1	0x199a

#ifdef _WIN32
#	define QLONG __int64
#else
#	define QLONG long long
#endif


#define FixMul(_a, _b)	(static_cast<fix> (((static_cast<QLONG> (_a)) * (static_cast<QLONG> (_b))) / 65536))
#define FixDiv(_a, _b)	(static_cast<fix> ((_b) ? (((static_cast<QLONG> (_a)) * 65536) / (static_cast<QLONG> (_b))) : 1))
#define FixMulDiv(_a, _b, _c) ((fix) ((_c) ? (((static_cast<QLONG> (_a)) * (static_cast<QLONG> (_b))) / (static_cast<QLONG> (_c))) : 1))

//divide a tQuadInt by a long
int32_t FixDivQuadLong (u_int32_t qlow, u_int32_t qhigh, u_int32_t d);

//computes the square root of a long, returning a short
ushort LongSqrt (int32_t a);

//computes the square root of a tQuadInt, returning a long
extern int nMathFormat;
extern int nDefMathFormat;

uint sqrt64 (unsigned QLONG a);

#define mul64(_a,_b)	(static_cast<QLONG> (_a) * static_cast<QLONG> (_b))

//multiply two fixes, and add 64-bit product to a tQuadInt
void FixMulAccum (tQuadInt * q, fix a, fix b);

u_int32_t QuadSqrt (u_int32_t low, int32_t high);
//unsigned long QuadSqrt (long low, long high);

//computes the square root of a fix, returning a fix
fix FixSqrt (fix a);

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
void FixSinCos (fix a, fix * s, fix * c);	//with interpolation

void FixFastSinCos (fix a, fix * s, fix * c);	//no interpolation

//compute inverse sine & cosine
fixang FixASin (fix v);

fixang FixACos (fix v);

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms
//need not be the actual cos & sin.
//NOTE: this is different from the standard C atan2, since it is left-handed.
fixang FixAtan2 (fix cos, fix sin);

//for passed value a, returns 1/sqrt(a)
fix FixISqrt (fix a);

//-----------------------------------------------------------------------------

static inline void d_srand (uint seed)
{
srand(seed);
}

//-----------------------------------------------------------------------------

static inline int d_rand (void)
{
return rand() & 0x7fff;
}

//-----------------------------------------------------------------------------

static inline float f_rand (void)
{
return static_cast<float> (d_rand()) / static_cast<float> (0x7fff);
}

//-----------------------------------------------------------------------------

#endif
