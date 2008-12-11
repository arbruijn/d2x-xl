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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _BYTESWAP_H
#define _BYTESWAP_H

#include <string.h>
#include "pstypes.h"
#include "inferno.h"
#include "vecmat.h"

// ----------------------------------------------------------------------------

#define SWAP(_h,_a,_b)	(_h) = (_a), (_a) = (_b), (_b) = (_h)

// ----------------------------------------------------------------------------

typedef union dSwap {
	double	d;
	char		b [8];
} dSwap;

#define DSWAP(_d)	(reinterpret_cast<dSwap *> (&(_d)))

static inline double SWAPDOUBLE (double i)
{
char	h;
#if 0
dSwap	s;

s.i = i;
h = s[BA] [0];
s[BA] [0] = s[BA] [7];
s[BA] [7] = h;
h = s[BA] [1];
s[BA] [1] = s[BA] [6];
s[BA] [6] = h;
h = s[BA] [2];
s[BA] [2] = s[BA] [5];
s[BA] [5] = h;
h = s[BA] [3];
s[BA] [3] = s[BA] [4];
s[BA] [4] = h;
return s.d;
#else
SWAP (h, DSWAP (i)->b [0], DSWAP (i)->b [7]);
SWAP (h, DSWAP (i)->b [1], DSWAP (i)->b [6]);
SWAP (h, DSWAP (i)->b [2], DSWAP (i)->b [5]);
SWAP (h, DSWAP (i)->b [3], DSWAP (i)->b [4]);
return i;
#endif
}

static inline double SwapDouble (double i, int bEndian)
{
if (gameStates.app.bLittleEndian == bEndian)
	return i;
return SWAPDOUBLE (i);
}

// ----------------------------------------------------------------------------

typedef union fSwap {
	float	i;
	char	b [4];
} fSwap;

#define FSWAP(_f)	(reinterpret_cast<fSwap *> (&(_f)))

static inline float SWAPFLOAT (float i)
{
char	h;
#if 0
fSwap	s;

s.i = i;
h = s[BA] [0];
s[BA] [0] = s[BA] [3];
s[BA] [3] = h;
h = s[BA] [1];
s[BA] [1] = s[BA] [2];
s[BA] [2] = h;
return s.i;
#else
SWAP (h, FSWAP (i)->b [0], FSWAP (i)->b [3]);
SWAP (h, FSWAP (i)->b [1], FSWAP (i)->b [2]);
return i;
#endif
}

static inline float SwapFloat (float i, int bEndian)
{
if (gameStates.app.bLittleEndian == bEndian)
	return i;
return SWAPFLOAT (i);
}

// ----------------------------------------------------------------------------

typedef union iSwap {
	int	i;
	char	b [4];
} iSwap;

#define ISWAP(_i)	(reinterpret_cast<iSwap *> (&(_i)))

static inline int SWAPINT (int i)
{
char	h;
SWAP (h, ISWAP (i)->b [0], ISWAP (i)->b [3]);
SWAP (h, ISWAP (i)->b [1], ISWAP (i)->b [2]);
return i;
}

static inline int SwapInt (int i, int bEndian)
{
if (gameStates.app.bLittleEndian == bEndian)
	return i;
return SWAPINT (i);
}

// ----------------------------------------------------------------------------

typedef union sSwap {
	short	s;
	char	b [2];
} sSwap;

#define SSWAP(_s)	(reinterpret_cast<sSwap *> (&(_s)))

static inline short SWAPSHORT (short i)
{
char	h;
SWAP (h, SSWAP (i)->b [0], SSWAP (i)->b [1]);
return i;
}

static inline int SwapShort (short i, int bEndian)
{
if (gameStates.app.bLittleEndian == bEndian)
	return i;
return SWAPSHORT (i);
}

// ----------------------------------------------------------------------------

static inline CFixVector& SwapVector (CFixVector& v, int bEndian)
{
if (gameStates.app.bLittleEndian != bEndian) 
	{
	v[X] = (fix) SWAPINT ((int) v[X]);
	v[Y] = (fix) SWAPINT ((int) v[Y]);
	v[Z] = (fix) SWAPINT ((int) v[Z]);
	}
return v;
}

// ----------------------------------------------------------------------------

static inline vmsAngVec& SwapAngVec (vmsAngVec& v, int bEndian)
{
if (gameStates.app.bLittleEndian != bEndian) 
	{
	v[PA] = (fixang) SWAPSHORT ((short) v[PA]);
	v[BA] = (fixang) SWAPSHORT ((short) v[BA]);
	v[HA] = (fixang) SWAPSHORT ((short) v[HA]);
	}
return v;
}

// ----------------------------------------------------------------------------

static inline vmsMatrix& SwapMatrix (vmsMatrix& m, int bEndian)
{
if (gameStates.app.bLittleEndian != bEndian) {
	SwapVector (m[RVEC], bEndian);
	SwapVector (m[UVEC], bEndian);
	SwapVector (m[FVEC], bEndian);
	}
return m;
}

// ----------------------------------------------------------------------------

#define INTEL_INT(_x)		SwapInt(_x,1)
#define INTEL_SHORT(_x)		SwapShort(_x,1)
#define INTEL_FLOAT(_x)		SwapFloat(_x,1)
#define INTEL_DOUBLE(_x)	SwapDouble(_x,1)
#define INTEL_VECTOR(_v)	SwapVector(_v,1)
#define INTEL_ANGVEC(_v)	SwapAngVec(_v,1)
#define INTEL_MATRIX(_m)	SwapMatrix(_m,1)
#define BE_INT(_x)			SwapInt(_x,0)
#define BE_SHORT(_x)			SwapShort(_x,0)

#ifndef WORDS_NEED_ALIGNMENT
#define GET_INTEL_INT(s)        INTEL_INT(*reinterpret_cast<uint *>(s))
#define GET_INTEL_SHORT(s)      INTEL_SHORT(*reinterpret_cast<ushort *>(s))
#define PUT_INTEL_INT(d, s)     { *reinterpret_cast<uint *>(d) = INTEL_INT((uint)(s)); }
#define PUT_INTEL_SHORT(d, s)   { *reinterpret_cast<ushort *>(d) = INTEL_SHORT((ushort)(s)); }
#else // ! WORDS_NEED_ALIGNMENT
static inline uint GET_INTEL_INT(void *s)
{
	uint tmp;
	memcpy(reinterpret_cast<void *>&tmp, s, 4);
	return INTEL_INT(tmp);
}
static inline uint GET_INTEL_SHORT(void *s)
{
	ushort tmp;
	memcpy(reinterpret_cast<void *>&tmp, s, 2);
	return INTEL_SHORT(tmp);
}
#define PUT_INTEL_INT(d, s)     { uint tmp = INTEL_INT(s); \
                                  memcpy(reinterpret_cast<void *>(d), reinterpret_cast<void *>&tmp, 4); }
#define PUT_INTEL_SHORT(d, s)   { ushort tmp = INTEL_SHORT(s); \
                                  memcpy(reinterpret_cast<void *>(d), reinterpret_cast<void *>&tmp, 2); }
#endif // ! WORDS_NEED_ALIGNMENT

#endif // ! _BYTESWAP_H
