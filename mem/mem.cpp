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

size_t nCurAllocd = 0;
size_t nMaxAllocd = 0;

#if DBG_MALLOC
#	define FULL_MEM_CHECKING 1
#	define LONG_MEM_ID 0
#	define MEMSTATS 0
#else
#	define FULL_MEM_CHECKING 0
#endif

#if FULL_MEM_CHECKING

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MEM_MAX_INDEX 1000000

static void *	pMallocBase [MEM_MAX_INDEX];
static size_t	nMallocSize [MEM_MAX_INDEX];
static uint8_t bPresent [MEM_MAX_INDEX];
static char *	szFilename [MEM_MAX_INDEX];
static char *	szVarname [MEM_MAX_INDEX];
static int32_t nLineNum [MEM_MAX_INDEX];
static int32_t nId [MEM_MAX_INDEX];
static size_t	nBytesMalloced = 0;
static bool		bTrackMemory = true;

//static int32_t nFreeList [MEM_MAX_INDEX];
//static int32_t iFreeList = 0;
static int32_t bMemInitialized = 0;
static int32_t nLargestIndex = 0;
static int32_t nMemId = 0;
int32_t bOutOfMemory = 0;

//------------------------------------------------------------------------------

typedef struct tMemBlock {
	public:
		int32_t		nNext;
		void			*p;
		size_t		size;
		size_t		nId;
		int32_t		nLine;
		const char	*pszFile;

} tMemBlock;

#define MAX_MEM_BLOCKS	100000

tMemBlock	memBlocks [MAX_MEM_BLOCKS];
int32_t		nMemBlockIndex [MAX_MEM_BLOCKS];
int32_t		nFreeList = 0;
int32_t		nUsedList = -1;
int32_t		nMemBlocks = 0;
size_t		nMemBlockId = 0;
int32_t		nDbgMemBlock = -1;
size_t		nDbgMemBlockId = 0xffffffff;

//------------------------------------------------------------------------------

void CheckMemBlock (void)
{
if (memBlocks [6325].pszFile && (memBlocks [6325].pszFile [0] != '.'))
	BRP;
}

//------------------------------------------------------------------------------

bool CheckMemBlockName (const char* pszName)
{
if (((pszName [0] < 'A') || (pszName [0] > 'Z')) &&
	 ((pszName [0] < 'a') || (pszName [0] > 'z')) &&
	 ((pszName [0] < '0') || (pszName [0] > '9')) &&
	 (pszName [0] != '.') && (pszName [0] != '\\') && (pszName [0] != '\0'))
	return false;
return true;
}

//------------------------------------------------------------------------------

void CheckMemBlocks (void)
{
if (nUsedList >= 0) {
	for (int32_t i = nUsedList; i > -1; i = memBlocks [i].nNext) {
		if (i >= sizeofa (memBlocks))
			BRP;
		if (!CheckMemBlockName (memBlocks [i].pszFile))
			BRP;
		}
	}
}

//------------------------------------------------------------------------------

void LogMemBlocks (bool bCleanup)
{
#if DBG_MALLOC
if (!bMemInitialized) 
	return;

if (nUsedList >= 0) {
	int32_t j = 0;
	for (int32_t i = nUsedList; i > -1; i = memBlocks [i].nNext) {
		if (i >= sizeofa (memBlocks))
			BRP;
		if (!CheckMemBlockName (memBlocks [i].pszFile))
			BRP;
		else
			nMemBlockIndex [j++] = i;
		}

	PrintLog (0, "\nMEMORY BLOCKS ALLOCATED:\n");
	size_t nTotal = 0;
	while (j) {
		int32_t i = nMemBlockIndex [--j];
		if (memBlocks [i].nLine)
			PrintLog (0, "<%05d> [%9d] %s:%d\n", i, memBlocks [i].size, memBlocks [i].pszFile, memBlocks [i].nLine);
		else
			PrintLog (0, "<%05d> [%9d] %s\n", i, memBlocks [i].size, memBlocks [i].pszFile);
		nTotal += memBlocks [i].size;
		}
	PrintLog (0, "\nTOTAL: %d bytes\n\n", nTotal);
#if 1
	if (bCleanup) {
		bool b = TrackMemory (false);
		for (int32_t i = nUsedList; i > -1; i = memBlocks [i].nNext)
			MemFree (memBlocks [i].p);
		TrackMemory (b);
		bMemInitialized = 0;
		}
#endif
	}
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ CleanupMemBlocks (void)
{
#if DBG_MALLOC
LogMemBlocks (true);
#endif
}

//------------------------------------------------------------------------------

int32_t FindMemBlock (void *p, int32_t &j)
{
j = -1;
for (int32_t i = nUsedList; i >= 0; i = memBlocks [i].nNext) {
	if (memBlocks [i].p == p) {
		if (i == nDbgMemBlock)
			BRP;
		return i;
		}
	j = i;
	}
return -1;
}

//------------------------------------------------------------------------------

void RegisterMemBlock (void *p, size_t size, const char *pszFile, int32_t nLine)
{
if (!bTrackMemory)
	return;
#if USE_OPENMP
#pragma omp critical (RegisterMemBlock)
	{
#endif
if (nFreeList > -1) {
	if (!pszFile)
		BRP;
	if (!*pszFile)
		BRP;
	if (!CheckMemBlockName (pszFile))
		BRP;
	int32_t i = nFreeList;
	nFreeList = memBlocks [nFreeList].nNext;
	memBlocks [i].nNext = nUsedList;
	nUsedList = i;

	if (nMemBlockId == nDbgMemBlockId)
		BRP;
	if (i == nDbgMemBlock)
		BRP;
	memBlocks [i].p = p;
	memBlocks [i].size = size;
	memBlocks [i].pszFile = pszFile;
	memBlocks [i].nLine = nLine;
	memBlocks [i].nId = nMemBlockId++;
	++nMemBlocks;
	}	
#if USE_OPENMP
}
#endif
}

//------------------------------------------------------------------------------

int32_t UnregisterMemBlock (void *p)
{
	int32_t i, j;

#if USE_OPENMP
#pragma omp critical (UnregisterMemBlock)
	{
#endif
i = FindMemBlock (p, j);
if (i >= 0) {
	if (j < 0)
		nUsedList = memBlocks [i].nNext;
	else
		memBlocks [j].nNext = memBlocks [i].nNext;
	memBlocks [i].nNext = nFreeList;
	nFreeList = i;
	memBlocks [nFreeList].pszFile = "";
	memBlocks [nFreeList].nLine = -1;
	--nMemBlocks;
	if (nMemBlocks < 0)
		BRP;
	CheckMemBlocks ();
	}
#if USE_OPENMP
}
return i;
#endif
}

//------------------------------------------------------------------------------

#define MEM_INIT(_b)	memset (_b, 0, sizeof (_b))

void MemInit ()
{
if (!bMemInitialized) {
	atexit (CleanupMemBlocks);
	for (int32_t i = 0; i < MAX_MEM_BLOCKS; i++) {
		memBlocks [i].nId = -1;
		memBlocks [i].nLine = -1;
		memBlocks [i].pszFile = "";
		memBlocks [i].p = NULL;
		memBlocks [i].nNext = i + 1;
		}
	memBlocks [MAX_MEM_BLOCKS - 1].nNext = -1;
	bMemInitialized = 1;
	}
}

//------------------------------------------------------------------------------

void PrintInfo (int32_t id)
{
PrintLog (0, "\tBlock '%s' created in %s, nLine %d.\n",
			szVarname [id], szFilename [id], nLineNum [id]);
}

//------------------------------------------------------------------------------

int32_t MemCheckIntegrity (void *buffer)
{
	int32_t	i, nErrors;
	uint8_t	*pCheckData;

size_t nSize = *reinterpret_cast<size_t*> (buffer);
pCheckData = reinterpret_cast<uint8_t*> (reinterpret_cast<size_t*> (buffer) + 1) + nSize;
for (i = 0, nErrors = 0; i < CHECKSIZE; i++)
	if (pCheckData [i] != CHECKBYTE) {
		nErrors++;
		PrintLog (0, "OA: %p ", pCheckData + i);
		}

if (nErrors && !bOutOfMemory) {
	PrintLog (0, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n");
#if LONG_MEM_ID
	PrintLog (0, "%s\n", reinterpret_cast<char*> (buffer) - 256);
#endif
	PrintLog (0, "\t%d/%d check bytes were overwritten.\n", nErrors, CHECKSIZE);
	Int3 ();
	}
return nErrors;
}

#endif //FULL_MEM_CHECKING

//------------------------------------------------------------------------------

#if !DBG_MALLOC

void MemFree (void *buffer)
{
if (!buffer)
	return;
nCurAllocd -= *(reinterpret_cast<size_t*> (buffer) - 1);
free (reinterpret_cast<size_t*> (buffer) - 1);
}

#else

void MemFree (void *buffer)
{
if (!bMemInitialized)
	MemInit ();
if (!buffer)
	return;
if (bTrackMemory && (UnregisterMemBlock (buffer) < 0))
	return;
buffer = reinterpret_cast<void*> (reinterpret_cast<size_t*> (buffer) - 1);
#ifndef __macosx__
nBytesMalloced -= *reinterpret_cast<size_t*> (buffer);
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

void *MemAlloc (size_t size)
{
if (!size)
	return NULL;
size += sizeof (size_t);
size_t *p = reinterpret_cast<size_t*> (malloc (size));
if (!p)
	return NULL;
*p = size;
nCurAllocd += size;
if (nMaxAllocd < nCurAllocd)
	nMaxAllocd = nCurAllocd;
return reinterpret_cast<void*> (p + 1);
}

#else	//!RELEASE

void *MemAlloc (size_t size, const char * var, const char * pszFile, int32_t nLine, int32_t bZeroFill)
{
	size_t *ptr;

if (!bMemInitialized)
	MemInit ();
if (!size)
	return NULL;
if (nMemBlockId == nDbgMemBlockId)
	nDbgMemBlockId = nDbgMemBlockId;
#if LONG_MEM_ID
ptr = reinterpret_cast<size_t*> (malloc (size + CHECKSIZE + sizeof (size_t) + 256));
#else
ptr = reinterpret_cast<size_t*> (malloc (size + CHECKSIZE + sizeof (size_t)));
#endif
if (!ptr) {
	PrintLog (0, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
	PrintLog (0, "\tVar %s, file %s, nLine %d.\n", var, pszFile, nLine);
	Int3 ();
	return NULL;
	}

#if LONG_MEM_ID
sprintf (reinterpret_cast<char*> (ptr), "%s:%d", pszFile, nLine);
ptr = reinterpret_cast<size_t*> (reinterpret_cast<char*> (ptr) + 256);
#endif
*ptr++ = size;
nBytesMalloced += size;
memset (reinterpret_cast<char*> (ptr) + size, CHECKBYTE, CHECKSIZE);
if (bZeroFill)
	memset (ptr, 0, size);
RegisterMemBlock (ptr, size, pszFile, nLine);
return reinterpret_cast<void*> (ptr);
}

#endif

//------------------------------------------------------------------------------

#if !DBG_MALLOC

void *MemRealloc (void * buffer, size_t size)
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
size_t oldSize = *(reinterpret_cast<size_t*> (buffer) - 1);
memcpy (newBuffer, buffer, (size < oldSize) ? size : oldSize);
MemFree (buffer);
return newBuffer;
}

#else

void *MemRealloc (void * buffer, size_t size, const char * var, const char * pszFile, int32_t nLine)
{
	void *newbuffer = NULL;

#if !DBG_MALLOC
if (!size)
	MemFree (buffer);
else if (!buffer)
	newbuffer = MemAlloc (size, var, pszFile, nLine, 0);
else if (!(newbuffer = realloc (buffer, size))) {
#if TRACE
	console.printf (CON_MALLOC, "reallocating %d bytes in %s:%d failed.\n", size, pszFile, nLine);
#endif
	}
#else
if (!bMemInitialized)
	MemInit ();

if (!size)
	MemFree (buffer);
else {
	newbuffer = MemAlloc (size, var, pszFile, nLine, 0);
	int32_t h;
	if (FindMemBlock (buffer, h) >= 0) {
		memcpy (newbuffer, buffer, *(reinterpret_cast<size_t*> (buffer) - 1));
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
	void	*p = NEW uint8_t [size];

if (p) {
	memset (p, filler, size);
	return reinterpret_cast<void*> (p);
	}
Error (TXT_OUT_OF_MEMORY);
exit (1);
}

// ----------------------------------------------------------------------------

#if DBG_MALLOC

bool TrackMemory (bool bTrack)
{
bool b = bTrackMemory;
bTrackMemory = bTrack;
return b;
}

#endif

// ----------------------------------------------------------------------------

#if DBG_MALLOC

void* __CRTDECL operator new (size_t size, const char* pszFile, const int32_t nLine) 
	throw (bad_alloc)
	{
   return MemAlloc (size, "", pszFile, nLine, 0);
	}

void* __CRTDECL operator new[] (size_t size, const char* pszFile, const int32_t nLine) 
	throw (bad_alloc)
	{
   return MemAlloc (size, "", pszFile, nLine, 0);
	}

void __CRTDECL operator delete (void* p/*, const char* pszFile, const int32_t nLine*/) throw () {
	MemFree (p);
	}

void __CRTDECL operator delete[] (void* p/*, const char* pszFile, const int32_t nLine*/) throw () {
	MemFree (p);
	}

#endif

//------------------------------------------------------------------------------
//eof
