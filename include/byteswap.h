/* $Id: byteswap.h,v 1.11 2004/04/22 21:05:16 btb Exp $ */
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

/*
 *
 * code to swap bytes because of big/little endian problems.
 * contains the macros:
 * SWAP{INT,SHORT}(x): returns a swapped version of x
 * INTEL_{INT,SHORT}(x): returns x after conversion to/from little endian
 * GET_INTEL_{INT,SHORT}(src): gets value from little-endian buffer src
 * PUT_INTEL_{INT,SHORT}(dest, src): puts src into little-endian buffer dest
 *
 * the GET/PUT macros are safe to use on platforms which segfault on unaligned word access
 *
 * Old Log:
 * Revision 1.4  1995/08/23  21:28:15  allender
 * fix mcc compiler warning
 *
 * Revision 1.3  1995/08/18  15:51:42  allender
 * put back in old byteswapping code
 *
 * Revision 1.2  1995/05/04  20:10:18  allender
 * proper prototypes
 *
 * Revision 1.1  1995/03/30  15:02:11  allender
 * Initial revision
 *
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

#define DSWAP(_d)	((dSwap *) &(_d))

static inline double SWAPDOUBLE (double i) 
{
char	h;	
#if 0
dSwap	s;

s.i = i;
h = s.b [0];
s.b [0] = s.b [7];
s.b [7] = h;
h = s.b [1];
s.b [1] = s.b [6];
s.b [6] = h;
h = s.b [2];
s.b [2] = s.b [5];
s.b [5] = h;
h = s.b [3];
s.b [3] = s.b [4];
s.b [4] = h;
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

#define FSWAP(_f)	((fSwap *) &(_f))

static inline float SWAPFLOAT (float i) 
{
char	h;	
#if 0
fSwap	s;

s.i = i;
h = s.b [0];
s.b [0] = s.b [3];
s.b [3] = h;
h = s.b [1];
s.b [1] = s.b [2];
s.b [2] = h;
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

#define ISWAP(_i)	((iSwap *) &(_i))

static inline int SWAPINT (int i) 
{
char	h;	
#if 0
iSwap	s;

s.i = i;
h = s.b [0];
s.b [0] = s.b [3];
s.b [3] = h;
h = s.b [1];
s.b [1] = s.b [2];
s.b [2] = h;
return s.i;
#else
SWAP (h, ISWAP (i)->b [0], ISWAP (i)->b [3]);
SWAP (h, ISWAP (i)->b [1], ISWAP (i)->b [2]);
return i;
#endif
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

#define SSWAP(_s)	((sSwap *) &(_s))

static inline short SWAPSHORT (short i) 
{
char	h;	
#if 0
sSwap	s;
s.i = i;
h = s.b [0];
s.b [0] = s.b [1];
s.b [1] = h;
return s.i;
#else
SWAP (h, SSWAP (i)->b [0], SSWAP (i)->b [1]);
return i;
#endif
}

static inline int SwapShort (short i, int bEndian) 
{
if (gameStates.app.bLittleEndian == bEndian)
	return i;
return SWAPSHORT (i);
}

// ----------------------------------------------------------------------------

static inline vmsVector *SwapVector (vmsVector *v, int bEndian) 
{
if (gameStates.app.bLittleEndian != bEndian) {
	v->p.x = (fix) SWAPINT ((int) v->p.x);
	v->p.y = (fix) SWAPINT ((int) v->p.y);
	v->p.z = (fix) SWAPINT ((int) v->p.z);
	}
return v;
}

// ----------------------------------------------------------------------------

static inline vmsAngVec *SwapAngVec (vmsAngVec *v, int bEndian) 
{
if (gameStates.app.bLittleEndian != bEndian) {
	v->p = (fixang) SWAPSHORT ((short) v->p);
	v->b = (fixang) SWAPSHORT ((short) v->b);
	v->h = (fixang) SWAPSHORT ((short) v->h);
	}
return v;
}

// ----------------------------------------------------------------------------

static inline vmsMatrix *SwapMatrix (vmsMatrix *m, int bEndian) 
{
if (gameStates.app.bLittleEndian != bEndian) {
	SwapVector (&m->rVec, bEndian);
	SwapVector (&m->uVec, bEndian);
	SwapVector (&m->fVec, bEndian);
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
#define GET_INTEL_INT(s)        INTEL_INT(*(uint *)(s))
#define GET_INTEL_SHORT(s)      INTEL_SHORT(*(ushort *)(s))
#define PUT_INTEL_INT(d, s)     { *(uint *)(d) = INTEL_INT((uint)(s)); }
#define PUT_INTEL_SHORT(d, s)   { *(ushort *)(d) = INTEL_SHORT((ushort)(s)); }
#else // ! WORDS_NEED_ALIGNMENT
static inline uint GET_INTEL_INT(void *s)
{
	uint tmp;
	memcpy((void *)&tmp, s, 4);
	return INTEL_INT(tmp);
}
static inline uint GET_INTEL_SHORT(void *s)
{
	ushort tmp;
	memcpy((void *)&tmp, s, 2);
	return INTEL_SHORT(tmp);
}
#define PUT_INTEL_INT(d, s)     { uint tmp = INTEL_INT(s); \
                                  memcpy((void *)(d), (void *)&tmp, 4); }
#define PUT_INTEL_SHORT(d, s)   { ushort tmp = INTEL_SHORT(s); \
                                  memcpy((void *)(d), (void *)&tmp, 2); }
#endif // ! WORDS_NEED_ALIGNMENT

#endif // ! _BYTESWAP_H
