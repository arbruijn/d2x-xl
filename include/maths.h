/* Maths[HA] library header file */

#ifndef _MATHS_H
#define _MATHS_H

#define NO_FIX_INLINE 0
#define ALT_SQRT 2

#include <stdlib.h>
#include <cmath>
#include "pstypes.h"

//=============================== FIXED POINT ===============================

#ifndef Pi
#  define  Pi    3.141592653589793240
#endif


typedef int32_t fix;		//16 bits int, 16 bits frac
typedef int16_t fixang;		//angles

typedef struct tQuadInt {// integer 64 bit, previously called "quad"
	u_int32_t low;
   int32_t high;
} __pack__ tQuadInt;

//Convert an int to a fix
#define I2X(i) ((static_cast<fix> (i)) * 65536)

//Get the int part of a fix
#define X2I(_f) ((_f) / 65536)

//Get the int part of a fix, with rounding
#define X2IR(_f) (((_f) + (I2X (1) / 2)) / 65536)

//Convert fix to double and double to fix
#define X2F(_f) (float (_f) / 65536.0f)
#define X2D(_f) (double (_f) / 65536.0)
#define F2X(_f) (fix ((_f) * 65536))

//Some handy constants
//#define F0_1	0x199a

#ifdef _WIN32
#	define QLONG __int64
#else
#	define QLONG long long
#endif

#if 1

#	if 1
#	define Round(_v)					(_v)
#	else
#	define Round(_v)					(((_v) < 0.0) ? (_v) - 0.5 : (_v) + 0.5)
#endif

#define FixMul64(_a, _b)		((QLONG) Round (double (_b) / 65536.0 * double (_a)))
#define FixMul(_a, _b)			((fix) Round (double (_b) / 65536.0 * double (_a)))
#define FixDiv(_a, _b)			((fix) Round ((_b) ? (double (_a) / double (_b) * 65536.0) : 1))
#define FixMulDiv(_a, _b, _c) ((fix) Round ((_c) ? double (_a) / double (_c) * double (_b) : 1))

#else

#define FixMul64(_a, _b)		(static_cast<QLONG> (((static_cast<QLONG> (_a)) * (static_cast<QLONG> (_b))) / 65536))
#define FixMul(_a, _b)			(static_cast<fix> (FixMul64 (_a, _b)))
#define FixDiv(_a, _b)			(static_cast<fix> ((_b) ? (((static_cast<QLONG> (_a)) * 65536) / (static_cast<QLONG> (_b))) : 1))
#define FixMulDiv(_a, _b, _c) ((fix) ((_c) ? (((static_cast<QLONG> (_a)) * (static_cast<QLONG> (_b))) / (static_cast<QLONG> (_c))) : 1))

#endif

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
// return a random integer in the range [0 .. SHORT_RAND_MAX]

#define SHORT_RAND_MAX 0x7fff

static inline int RandShort (void)
{
return int (rand () & SHORT_RAND_MAX);
}

//-----------------------------------------------------------------------------
// return a random integer in the range [-SHORT_RAND_MAX / 2 .. +SHORT_RAND_MAX / 2]

static inline int SRandShort (void)
{
return SHORT_RAND_MAX / 2 - RandShort ();
}

//-----------------------------------------------------------------------------
// return a random float in the range [0.0f .. 1.0f]

static inline float RandFloat (float scale = 1.0f)
{
return float (rand ()) / (float (RAND_MAX) * scale);
}

//------------------------------------------------------------------------------
// return a random double in the range [0.0 .. 1.0]

inline double RandDouble (void)
{
return double (rand ()) / double (RAND_MAX);
}

//-----------------------------------------------------------------------------

#endif
