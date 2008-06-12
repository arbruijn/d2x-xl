/* $Id: u_mem.h,v 1.7 2003/11/27 00:21:04 btb Exp $ */
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

#ifdef _DEBUG
#	define DBG_MALLOC	0
#else
#	define DBG_MALLOC 0
#endif

#if DBG_MALLOC

void _CDECL_ MemDisplayBlocks (void);
void * MemAlloc (unsigned int size, char * var, char * file, int line, int fill_zero);
void * MemRealloc (void * buffer, unsigned int size, char * var, char * file, int line);
void MemFree (void * buffer);
char * MemStrDup (char * str, char * var, char * file, int line);
void MemInit ();

/* DPH: Changed malloc, etc. to d_malloc. Overloading system calls is very evil and error prone */
#define D2_ALLOC(size)			MemAlloc ((size), "Unknown", __FILE__, __LINE__, 0)
#define D2_CALLOC(n,size)		MemAlloc ((n*size), "Unknown", __FILE__, __LINE__, 1)
#define D2_REALLOC(ptr,size)	MemRealloc ((ptr), (size), "Unknown", __FILE__, __LINE__)
#define D2_FREE(ptr)				{MemFree(ptr); ptr = NULL;} 
#define D2_STRDUP(str)			MemStrDup ((str), "Unknown", __FILE__, __LINE__)

#define MALLOC(_var, _type, _count)   ((_var) = (_type *) MemAlloc ((_count) * sizeof (_type), #_var, __FILE__, __LINE__, 0))

// Checks to see if any blocks are overwritten
void MemValidateHeap ();

#else

void *MemAlloc (unsigned int size);
void *MemRealloc (void * buffer, unsigned int size);
void MemFree (void * buffer);
char *MemStrDup (char * str);

#define D2_ALLOC(size)			MemAlloc (size)
#define D2_CALLOC(n, size)		MemAlloc (n * size)
#define D2_REALLOC(ptr,size)	MemRealloc (ptr,size)
#define D2_FREE(ptr)				{MemFree (ptr); (ptr) = NULL;}
#define D2_STRDUP(str)			MemStrDup (str)

#define MALLOC(_var, _type, _count)   ((_var) = (_type *) D2_ALLOC ((_count) * sizeof (_type)))

#endif

extern unsigned int nCurAllocd, nMaxAllocd;

//eof
