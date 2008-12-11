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
 * Files for debugging memory allocator
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __macosx__
# include <sys/malloc.h>
#else
# include <malloc.h>
#endif
#include "pstypes.h"
#include "mono.h"
#include "error.h"
#include "u_mem.h"
#include "text.h"

uint nCurAllocd = 0;
uint nMaxAllocd = 0;

#if DBG_MALLOC
#	define FULL_MEM_CHECKING 1
#	define LONG_MEM_ID 1
#	define MEMSTATS 0
#else
#	define FULL_MEM_CHECKING 0
#endif

#if FULL_MEM_CHECKING

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MEM_MAX_INDEX 1000000

static void *pMallocBase [MEM_MAX_INDEX];
static uint nMallocSize [MEM_MAX_INDEX];
static ubyte bPresent [MEM_MAX_INDEX];
static char * szFilename [MEM_MAX_INDEX];
static char * szVarname [MEM_MAX_INDEX];
static int nLineNum [MEM_MAX_INDEX];
static int nId [MEM_MAX_INDEX];
static int nBytesMalloced = 0;

static int nFreeList [MEM_MAX_INDEX];
static int iFreeList = 0;
static int bMemInitialized = 0;
static int nLargestIndex = 0;
static int nMemId = 0;
int bOutOfMemory = 0;

//------------------------------------------------------------------------------

typedef struct tMemBlock {
	void	*p;
	char	*pszFile;
	int	nLine;
	uint nId;
} tMemBlock;

#define MAX_MEM_BLOCKS	100000

tMemBlock	memBlocks [MAX_MEM_BLOCKS];
int nMemBlocks = 0;
uint nMemBlockId = 0;
uint nDbgMemBlockId = 0xffffffff;

//------------------------------------------------------------------------------

int FindMemBlock (void *p)
{
	int			i;
	tMemBlock	*pm;

for (i = nMemBlocks, pm = memBlocks; i; i--, pm++)
	if (pm->p == p)
		return (int) (pm - memBlocks);
return -1;
}

//------------------------------------------------------------------------------

void RegisterMemBlock (void *p, char *pszFile, int nLine)
{
if (nMemBlocks < MAX_MEM_BLOCKS) {
	tMemBlock *pm = memBlocks + nMemBlocks++;
	if (nMemBlockId == nDbgMemBlockId)
		pm = pm;
	pm->p = p;
	pm->pszFile = pszFile;
	pm->nLine = nLine;
	pm->nId = nMemBlockId++;
	}	
}

//------------------------------------------------------------------------------

int UnregisterMemBlock (void *p)
{
	int	i = FindMemBlock (p);

if ((i >= 0) && (i < --nMemBlocks))
	memBlocks [i] = memBlocks [nMemBlocks];
return i;
}

//------------------------------------------------------------------------------

#define MEM_INIT(_b)	memset (_b, 0, sizeof (_b))

void MemInit ()
{
if (!bMemInitialized) {
	atexit (MemDisplayBlocks);
	}
}

//------------------------------------------------------------------------------

void PrintInfo (int id)
{
PrintLog ("\tBlock '%s' created in %s, nLine %d.\n",
			szVarname [id], szFilename [id], nLineNum [id]);
}

//------------------------------------------------------------------------------

int MemCheckIntegrity (void *buffer)
{
	int	i, nSize, nErrors;
	ubyte	*pCheckData;

nSize = *reinterpret_cast<int*> (buffer);
pCheckData = reinterpret_cast<ubyte*> (reinterpret_cast<int*> (buffer) + 1) + nSize;
for (i = 0, nErrors = 0; i < CHECKSIZE; i++)
	if (pCheckData [i] != CHECKBYTE) {
		nErrors++;
		PrintLog ("OA: %p ", pCheckData + i);
		}

if (nErrors && !bOutOfMemory)	{
	PrintLog ("\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n");
#if LONG_MEM_ID
	PrintLog ("%s\n", reinterpret_cast<char*> (buffer) - 256);
#endif
	PrintLog ("\t%d/%d check bytes were overwritten.\n", nErrors, CHECKSIZE);
	Int3 ();
	}
return nErrors;
}

//------------------------------------------------------------------------------

void _CDECL_ MemDisplayBlocks (void)
{
#if DBG_MALLOC
	int i, numleft = 0;

if (!bMemInitialized) 
	return;

if (nMemBlocks) {
	PrintLog ("\nMEMORY LEAKS:\n");
	for (i = 0; i < nMemBlocks; i++)
		PrintLog ("%s:%d\n", memBlocks [i].pszFile, memBlocks [i].nLine);
	for (i = 0; i < nMemBlocks; i++)
		D2_FREE (memBlocks [i].p);
	}
#endif
}

#endif //FULL_MEM_CHECKING

//------------------------------------------------------------------------------

#if !DBG_MALLOC

void MemFree (void *buffer)
{
if (!buffer)
	return;
nCurAllocd -= *(reinterpret_cast<uint*> (buffer) - 1);
free (reinterpret_cast<uint*> (buffer) - 1);
}

#else

void MemFree (void *buffer)
{
if (!bMemInitialized)
	MemInit ();
if (!buffer)
	return;
if (UnregisterMemBlock (buffer) < 0)
	return;
buffer = reinterpret_cast<void*> (reinterpret_cast<int*> (buffer) - 1);
#ifndef __macosx__
nBytesMalloced -= *reinterpret_cast<int*> (buffer);
#endif
MemCheckIntegrity (buffer);
#if LONG_MEM_ID
buffer = reinterpret_cast<void*> (reinterpret_cast<char*> (buffer) - 256);
#endif
free (buffer);
}

#endif

//------------------------------------------------------------------------------

#if !DBG_MALLOC

void *MemAlloc (uint size)
{
if (!size)
	return NULL;
size += sizeof (uint);
uint *p = reinterpret_cast<uint*> (malloc (size));
if (!p)
	return NULL;
*p = size;
nCurAllocd += size;
if (nMaxAllocd < nCurAllocd)
	nMaxAllocd = nCurAllocd;
return reinterpret_cast<void*> (p + 1);
}

#else	//!RELEASE

void *MemAlloc (uint size, const char * var, const char * pszFile, int nLine, int bZeroFill)
{
	uint *ptr;

if (!bMemInitialized)
	MemInit ();
if (!size)
	return NULL;
if (nMemBlockId == nDbgMemBlockId)
	nDbgMemBlockId = nDbgMemBlockId;
#if LONG_MEM_ID
ptr = malloc (size + CHECKSIZE + sizeof (int) + 256);
#else
ptr = malloc (size + CHECKSIZE + sizeof (int));
#endif
if (!ptr) {
	PrintLog ("\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
	PrintLog ("\tVar %s, file %s, nLine %d.\n", var, pszFile, nLine);
	Int3 ();
	return NULL;
	}

#if LONG_MEM_ID
sprintf (reinterpret_cast<char*> (ptr, "%s:%d", pszFile, nLine);
ptr = reinterpret_cast<int*> (reinterpret_cast<char*> (ptr) + 256);
#endif
*ptr++ = size;
nBytesMalloced += size;
memset (reinterpret_cast<char*> (ptr + size), CHECKBYTE, CHECKSIZE);
if (bZeroFill)
	memset (ptr, 0, size);
RegisterMemBlock (ptr, pszFile, nLine);
return reinterpret_cast<void*> (ptr);
}

#endif

//------------------------------------------------------------------------------

#if !DBG_MALLOC

void *MemRealloc (void * buffer, uint size)
{
if (!buffer)
	return MemAlloc (size);
if (!size) {
	MemFree (buffer);
	return NULL;
	}
void *newBuffer = MemAlloc (size);
if (!newBuffer)
	return buffer;
uint oldSize = *(reinterpret_cast<uint*> (buffer) - 1);
memcpy (newBuffer, buffer, (size < oldSize) ? size : oldSize);
MemFree (buffer);
return newBuffer;
}

#else

void *MemRealloc (void * buffer, uint size, const char * var, const char * pszFile, int nLine)
{
	void *newbuffer = NULL;

#if !DBG_MALLOC
if (!size)
	MemFree (buffer);
else if (!buffer)
	newbuffer = MemAlloc (size, var, pszFile, nLine, 0);
else if (!(newbuffer = realloc (buffer, size))) {
#if TRACE
	con_printf (CON_MALLOC, "reallocating %d bytes in %s:%d failed.\n", size, pszFile, nLine);
#endif
	}
#else
if (!bMemInitialized)
	MemInit ();

if (!size)
	MemFree (buffer);
else {
	newbuffer = MemAlloc (size, var, pszFile, nLine, 0);
	if (FindMemBlock (buffer) >= 0) {
		memcpy (newbuffer, buffer, *(reinterpret_cast<int*> (buffer) - 1));
		MemFree (buffer);
		}
	}
#endif
return newbuffer;
}

#endif

// ----------------------------------------------------------------------------

void *GetMem (size_t size, char filler)
{
	void	*p = new ubyte [size];

if (p) {
	memset (p, filler, size);
	return reinterpret_cast<void*> p;
	}
Error (TXT_OUT_OF_MEMORY);
exit (1);
}

//------------------------------------------------------------------------------
//eof
