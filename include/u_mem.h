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

#include <stdlib.h>
#include "pstypes.h"

extern int bShowMemInfo;

#if DBG
#	define DBG_MALLOC	1
#else
#	define DBG_MALLOC 0
#endif

#if DBG_MALLOC

void _CDECL_ MemDisplayBlocks (void);
void * MemAlloc (uint size, const char * var, const char * file, int line, int fill_zero);
void * MemRealloc (void * buffer, uint size, const char * var, const char * file, int line);
void MemFree (void * buffer);
char * MemStrDup (const char * str, const char * var, const char * file, int line);
void MemInit ();

/* DPH: Changed malloc, etc. to d_malloc. Overloading system calls is very evil and error prone */
#define D2_ALLOC(size)			MemAlloc ((size), "Unknown", __FILE__, __LINE__, 0)
#define D2_CALLOC(n,size)		MemAlloc ((n*size), "Unknown", __FILE__, __LINE__, 1)
#define D2_REALLOC(ptr,size)	MemRealloc ((ptr), (size), "Unknown", __FILE__, __LINE__)
#define D2_FREE(ptr)				{MemFree (reinterpret_cast<void *> (ptr)); ptr = NULL;} 

#define MALLOC(_var, _type, _count)   ((_var) = reinterpret_cast<_type *> (MemAlloc ((_count) * sizeof (_type), #_var, __FILE__, __LINE__, 0)))

// Checks to see if any blocks are overwritten
void MemValidateHeap ();

#else

void *MemAlloc (uint size);
void *MemRealloc (void * buffer, uint size);
void MemFree (void * buffer);
char *MemStrDup (const char * str);

#define D2_ALLOC(_size)			MemAlloc (_size)
#define D2_CALLOC(n, _size)	MemAlloc (n * _size)
#define D2_REALLOC(_p,_size)	MemRealloc (_p,_size)
#define D2_FREE(_p)				{MemFree ((void *) (_p)); (_p) = NULL;}
#define D2_STRDUP(_s)			MemStrDup (_s)

#define MALLOC(_v,_t,_c)		(_v) = new _t [_c]
#define FREE(_v)					delete[] (_v)

#endif

extern uint nCurAllocd, nMaxAllocd;

#define CREATE(_p,_s,_f)	if ((_p).Create (_s)) (_p).Clear (_f)
#define DESTROY(_p)			(_p).Destroy ()

void *GetMem (size_t size, char filler);

//eof
