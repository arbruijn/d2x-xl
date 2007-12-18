/* $Id: mem.c,v 1.12 2003/11/26 12:39:00 btb Exp $ */
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

#ifdef malloc
#	undef malloc
#endif

int bShowMemInfo = 0;

#if DBG_MALLOC
#	define CHECKID1 1977
#	define CHECKID2 1979
#endif

#ifdef D2X_MEM_HANDLER

#define LONG_MEM_ID 1
#define MEMSTATS 0

#if DBG_MALLOC
#	define FULL_MEM_CHECKING 1
#else
#	define FULL_MEM_CHECKING 0
#endif

#if FULL_MEM_CHECKING

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MEM_MAX_INDEX 1000000

static void *pMallocBase [MEM_MAX_INDEX];
static unsigned int nMallocSize [MEM_MAX_INDEX];
static unsigned char bPresent [MEM_MAX_INDEX];
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

#if DBG_MALLOC

typedef struct tMemBlock {
	void	*p;
	char	*pszFile;
	int	nLine;
	unsigned int nId;
} tMemBlock;

#define MAX_MEM_BLOCKS	100000

tMemBlock	memBlocks [MAX_MEM_BLOCKS];
int nMemBlocks = 0;
unsigned int nMemBlockId = 0;
unsigned int nDbgMemBlockId = 0xffffffff;

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

#endif

//------------------------------------------------------------------------------

#define MEM_INIT(_b)	memset (_b, 0, sizeof (_b))

void MemInit ()
{
if (!bMemInitialized) {
	int i;

	bMemInitialized = 1;
	MEM_INIT (pMallocBase);
	MEM_INIT (nMallocSize);
	MEM_INIT (bPresent);
	MEM_INIT (szFilename);
	MEM_INIT (szVarname);
	MEM_INIT (nLineNum);
	for (i = 0; i < MEM_MAX_INDEX - 1; i++)
		nFreeList [i] = i + 1;
	nFreeList [i] = -1;
	nLargestIndex = 0;
	nMemId = 0;
	atexit (MemDisplayBlocks);
	}
}

//------------------------------------------------------------------------------

void PrintInfo (int id)
{
LogErr ("\tBlock '%s' created in %s, nLine %d.\n",
			szVarname [id], szFilename [id], nLineNum [id]);
}

//------------------------------------------------------------------------------

int MemFindId (void *buffer)
{
	int id;

buffer = (void *) (((int *) buffer) - 2);
id = *((int *) buffer);
if ((id < 0) || (id > nLargestIndex))
	return -1;
else if (!bPresent [id])
	return -1;
else if (pMallocBase [id] != buffer)
	return -1;
else
	return id;
}

//------------------------------------------------------------------------------

int MemCheckIntegrity (int block_number)
{
	int	i, nErrorCount;
	ubyte	*pCheckData;

pCheckData = (ubyte *) ((char *) pMallocBase [block_number] + nMallocSize [block_number] + 2 * sizeof (int));
nErrorCount = 0;
for (i = 0; i < CHECKSIZE; i++)
	if (pCheckData [i] != CHECKBYTE) {
		nErrorCount++;
		LogErr ("OA: %p ", pCheckData + i);
		}

if (nErrorCount && !bOutOfMemory)	{
	LogErr ("\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n");
	PrintInfo (block_number);
	LogErr ("\t%d/%d check bytes were overwritten.\n", nErrorCount, CHECKSIZE);
	Int3 ();
	}
return nErrorCount;
}

//------------------------------------------------------------------------------

void _CDECL_ MemDisplayBlocks (void)
{
#if DBG_MALLOC
	int i, numleft;

if (!bMemInitialized) 
	return;

#if MEMSTATS
if (bMemStatsFileInitialized) {
	unsigned long	nFreeMem = FreeMem ();
	fprintf (sMemStatsFile,
			"\n%9u bytes free before closing MEMSTATS file.", nFreeMem);
	fprintf (sMemStatsFile, "\nMemory Stats File Closed.");
	fclose (sMemStatsFile);
	}
#endif	// end of ifdef memstats
#if 1
if (nMemBlocks) {
	LogErr ("\nMEMORY LEAKS:\n");
	for (i = 0; i < nMemBlocks; i++)
		LogErr ("%s:%d\n", memBlocks [i].pszFile, memBlocks [i].nLine);
	for (i = 0; i < nMemBlocks; i++)
		D2_FREE (memBlocks [i].p);
	}
#else
numleft = 0;
for (i = 0; i <= nLargestIndex; i++) {
	if (bPresent [i] && !bOutOfMemory) {
		numleft++;
		if (bShowMemInfo)	{
			LogErr ("\nMEM_LEAKAGE: Memory block has not been freed.\n");
			PrintInfo (i);
			}
		MemFree ((void *) pMallocBase [i]);
		}
	}
#endif
if (numleft && !bOutOfMemory)
	Warning ("MEM: %d blocks were left allocated!\n", numleft);
#endif
}

//------------------------------------------------------------------------------

void MemValidateHeap ()
{
	int i;

for (i = 0; i < nLargestIndex; i++)
	if (bPresent [i])
		MemCheckIntegrity (i);
}

//------------------------------------------------------------------------------

void MemPrintAll ()
{
	FILE * ef;
	int i, size = 0;

ef = fopen ("DESCENT.MEM", "wt");
for (i = 0; i < nLargestIndex; i++)
	if (bPresent [i])	{
		size += nMallocSize [i];
		//fprintf (ef, "Var:%s\t File:%s\t Line:%d\t Size:%d Base:%x\n", szVarname [i], szFilename [i], Line [i], nMallocSize [i], pMallocBase [i]);
		fprintf (ef, "%12d bytes in %s declared in %s, nLine %d\n", nMallocSize [i], szVarname [i], szFilename [i], nLineNum [i] );
		}
fprintf (ef, "%d bytes (%d Kbytes) allocated.\n", size, size/1024); 
fclose (ef);
}

#else

//------------------------------------------------------------------------------

static int bMemInitialized = 0;
static unsigned int nSmallestAddress = 0xFFFFFFF;
static unsigned int nLargestAddress = 0x0;
static unsigned int nBytesMalloced = 0;

void _CDECL_ MemDisplayBlocks ();

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

int bShowMemInfo = 0;

void MemInit ()
{
bMemInitialized = 1;
nSmallestAddress = 0xFFFFFFF;
nLargestAddress = 0x0;
atexit (MemDisplayBlocks);
}

//------------------------------------------------------------------------------

void _CDECL_ MemDisplayBlocks (void)
{
if (!bMemInitialized) 
	return;

#if MEMSTATS
if (bMemStatsFileInitialized) {
	unsigned long	nFreeMem = FreeMem ();
	fprintf (sMemStatsFile,
			"\n%9u bytes free before closing MEMSTATS file.", nFreeMem);
	fprintf (sMemStatsFile, "\nMemory Stats File Closed.");
	fclose (sMemStatsFile);
	}
#endif	// end of ifdef memstats
if (nBytesMalloced) {
	LogErr ("\nMEM_LEAKAGE: %d bytes of memory have not been freed.\n", nBytesMalloced);
	}
if (bShowMemInfo)	{
	LogErr ("\n\nMEMORY USAGE:\n");
	LogErr ("  %u Kbytes dynamic data\n", (nLargestAddress-nSmallestAddress+512)/1024);
	LogErr ("  %u Kbytes code/static data.\n", (nSmallestAddress- (4*1024*1024)+512)/1024);
	LogErr ("  ---------------------------\n");
	LogErr ("  %u Kbytes required.\n", 	 (nLargestAddress- (4*1024*1024)+512)/1024);
	}
}

//------------------------------------------------------------------------------

void MemValidateHeap ()
{
}

//------------------------------------------------------------------------------

void MemPrintAll ()
{
}

#endif


//------------------------------------------------------------------------------

void MemFree (void *buffer)
{
#if !DBG_MALLOC
if (!buffer)
	return;
if (buffer)
	free (buffer);
#else
#if FULL_MEM_CHECKING
	int id;
#endif
if (!buffer)
	return;
#if DBG_MALLOC
if (UnregisterMemBlock (buffer) < 0)
	return;
#endif
if (!bMemInitialized)
	MemInit ();

#if MEMSTATS
{
unsigned long	nFreeMem = 0;
if (bMemStatsFileInitialized) {
	nFreeMem = FreeMem ();
	fprintf (sMemStatsFile, "\n%9u bytes free before attempting: FREE", nFreeMem);
	}
}
#endif	// end of ifdef memstats

#ifdef FULL_MEM_CHECKING
if (!(buffer || bOutOfMemory))
#else
if (!buffer)
#endif
	{
	//LogErr ("\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n");
#if DBG_MALLOC
	//Warning ("MEM: Freeing the NULL pointer!");
#endif
	Int3 ();
	return;
	}
#if FULL_MEM_CHECKING
#	if LONG_MEM_ID
buffer = (void *) (((char *) buffer) - 256);
#	endif
id = MemFindId (buffer);
if ((id == -1) && !bOutOfMemory) {
	LogErr ("\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included.\n");
#if DBG_MALLOC
	//Warning ("Freeing a non-allocated pointer!");
#endif
	Int3 ();
	return;
	}
MemCheckIntegrity (id);
#endif
#ifndef __macosx__
nBytesMalloced -= *(((int *) buffer) - 1);
#endif
free (((int *) buffer) - 2);
#if FULL_MEM_CHECKING
bPresent [id] = 0;
#if DBG_MALLOC
if (id == CHECKID1)
	id = CHECKID1;
if (id == CHECKID2)
	id = CHECKID2;
#endif
pMallocBase [id] = 0;
nMallocSize [id] = 0;
nFreeList [id] = iFreeList;
iFreeList = id;
#	endif
#endif
}

//------------------------------------------------------------------------------

#ifdef malloc
#	undef malloc
#endif

void *MemAlloc (unsigned int size, char * var, char * pszFile, int nLine, int bZeroFill)
{
	int *ptr;

#ifndef _DEBUG

if (!(ptr = (int *) malloc (size))) {
#if 1//TRACE
	LogErr ("allocating %d bytes in %s:%d failed.\n", size, pszFile, nLine);
#endif
	}
else if (bZeroFill)
	memset (ptr, 0, size);

#else	//!RELEASE

#if FULL_MEM_CHECKING
	int id;
#endif
if (!bMemInitialized)
	MemInit ();

#if MEMSTATS
{
unsigned long	nFreeMem = 0;
if (bMemStatsFileInitialized) {
	nFreeMem = FreeMem ();
	fprintf (sMemStatsFile, "\n%9u bytes free before attempting: MALLOC %9u bytes.", nFreeMem, size);
	}
}
#endif	// end of ifdef memstats

#if FULL_MEM_CHECKING
if (iFreeList < 0) {
	LogErr ("\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n");	
	LogErr ("\tBlock '%s' created in %s, nLine %d.\n", var, pszFile, nLine);
	Error ("Out of free memory slots");
	}

id = iFreeList;
iFreeList = nFreeList [iFreeList];
if (id > nLargestIndex) 
	nLargestIndex = id;
if (id == -1) {
	LogErr ("\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n");	
	LogErr ("\tBlock '%s' created in %s, nLine %d.\n", szVarname [id], szFilename [id], nLineNum [id]);
	Error ("Out of free memory slots");
	}
#endif

if (!size)	{
	LogErr ("\nMEM_MALLOC_ZERO: Attempting to malloc 0 bytes.\n");
	LogErr ("\tVar %s, file %s, nLine %d.\n", var, pszFile, nLine);
	Error ("Tried to allocate block of 0 bytes");
	Int3 ();
	}

#if LONG_MEM_ID
ptr = malloc (size + CHECKSIZE + 2 * sizeof (int) + 256);
#else
ptr = malloc (size + CHECKSIZE + 2 * sizeof (int));
#endif
if (!ptr) {
	LogErr ("\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
	LogErr ("\tVar %s, file %s, nLine %d.\n", var, pszFile, nLine);
	//Error ("MEM_OUT_OF_MEMORY");
	Int3 ();
	return NULL;
	}

#if FULL_MEM_CHECKING
pMallocBase [id] = ptr;
nMallocSize [id] = size;
szVarname [id] = var;
szFilename [id] = pszFile;
nLineNum [id] = nLine;
nId [id] = nMemId++;
#if DBG_MALLOC
if (id == CHECKID1)
	id = CHECKID1;
if (id == CHECKID2)
	id = CHECKID2;
#endif
bPresent [id] = 1;
#else
{
unsigned int base = (unsigned int) ptr;
if (base < nSmallestAddress) 
	nSmallestAddress = base;
if ((base + size) > nLargestAddress) 
	nLargestAddress = base+size;
}
#endif

*ptr++ = id;
*ptr++ = size;
#if LONG_MEM_ID
sprintf ((char *) ptr, "%s:%d", pszFile, nLine);
ptr = (int *) (((char *) ptr) + 256);
#endif
nBytesMalloced += size;
#if FULL_MEM_CHECKING
memset ((char *) ptr + size, CHECKBYTE, CHECKSIZE);
#endif
if (bZeroFill)
	memset (ptr, 0, size);
#endif //!RELEASE
#if DBG_MALLOC
RegisterMemBlock (ptr, pszFile, nLine);
#endif
return (void *) ptr;
}

//------------------------------------------------------------------------------

void *MemRealloc (void * buffer, unsigned int size, char * var, char * pszFile, int nLine)
{
	void *newbuffer = NULL;

#if !DBG_MALLOC
if (!size)
	MemFree (buffer);
else if (!buffer)
	newbuffer = MemAlloc (size, var, pszFile, nLine, 0);
else if (! (newbuffer = realloc (buffer, size))) {
#if TRACE
	con_printf (CON_MALLOC, "reallocating %d bytes in %s:%d failed.\n", size, pszFile, nLine);
#endif
	}
#else
	int id;

if (!bMemInitialized)
	MemInit ();

if (!size)
	MemFree (buffer);
else {
	newbuffer = MemAlloc (size, var, pszFile, nLine, 0);
	if (buffer) {
		id = MemFindId (buffer);
		if (nMallocSize [id] < size)
			size = nMallocSize [id];
		memcpy (newbuffer, buffer, size);
		MemFree (buffer);
		}
	}
#endif
return newbuffer;
}

//------------------------------------------------------------------------------

char *MemStrDup (char *str, char *var, char *pszFile, int nLine)
{
	char *newstr;
	int l = (int) strlen (str) + 1;

if ((newstr = MemAlloc (l, var, pszFile, nLine, 0)))
	memcpy (newstr, str, l);
return newstr;
}

#endif

//------------------------------------------------------------------------------
//eof
