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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#if defined (_WIN32_WCE) || defined (_WIN32)
#	include <windows.h>
#	include <sys/stat.h>
#else
#	include <sys/stat.h>
#endif

#include "inferno.h"
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "d_io.h"
#include "error.h"
#include "cfile.h"
#include "byteswap.h"
#include "mission.h"
#include "console.h"
#include "findfile.h"

int nCFileError = 0;

tGameHogFiles gameHogFiles;
tGameFolders gameFolders;

// ----------------------------------------------------------------------------

int GetAppFolder (const char *szRootDir, char *szFolder, const char *szName, const char *szFilter)
{
	FFS	ffs;
	char	szDir [FILENAME_LEN];
	int	i, bAddSlash;

if (!(szName && *szName))
	return 1;
i = (int) strlen (szRootDir);
bAddSlash = i && (szRootDir [i-1] != '\\') && (szRootDir [i-1] != '/');
PrintLog ("GetAppFolder ('%s', '%s', '%s', '%s')\n", szRootDir, szFolder, szName, szFilter);
sprintf (szDir, "%s%s%s%s%s", szRootDir, bAddSlash ? "/" : "", szName, *szFilter ? "/" : "", szFilter);
if (!(i = FFF (szDir, &ffs, *szFilter == '\0'))) {
	if (szFolder != szName)
		sprintf (szFolder, "%s%s%s", szRootDir, bAddSlash ? "/" : "", szName);
	}
else if (*szRootDir)
	strcpy (szFolder, szRootDir);
PrintLog ("GetAppFolder (%s) = '%s' (%d)\n", szName, szFolder, i);
FFC (&ffs);
return i;
}

// ----------------------------------------------------------------------------

void CFUseAltHogDir (char * path) 
{
gameFolders.bAltHogDirInited = 
	(strcmp (path, gameFolders.szDataDir) != 0) && (GetAppFolder ("", gameFolders.szAltHogDir, path, "descent2.hog") == 0);
}

// ----------------------------------------------------------------------------
//in case no one installs one
int defaultErrorCounter = 0;

//ptr to counter of how many critical errors
int *criticalErrorCounterPtr = &defaultErrorCounter;

// ----------------------------------------------------------------------------
//tell cfP about your critical error counter
void CFSetCriticalErrorCounterPtr (int *ptr)
{
criticalErrorCounterPtr = ptr;
}

// ----------------------------------------------------------------------------

inline void CFCriticalError (int error)
{
if (*criticalErrorCounterPtr) {
	if (error)
		*criticalErrorCounterPtr += error;
	else
		*criticalErrorCounterPtr = 0;
	}
}

// ----------------------------------------------------------------------------

FILE *CFGetFileHandle (const char *filename, const char *folder, const char *mode) 
{
	FILE	*fp;
	char	fn [FILENAME_LEN];
	const char *pfn;

CFCriticalError (0);
if (!*filename) {
	CFCriticalError (1);
	return NULL;
	}
if ((*filename != '/') && (strstr (filename, "./") != filename) && *folder) {
	sprintf (fn, "%s/%s", folder, filename);
   pfn = fn;
	}
 else
 	pfn = filename;
 
fp = fopen (pfn, mode);
 if (!fp && gameFolders.bAltHogDirInited && strcmp (folder, gameFolders.szAltHogDir)) {
   sprintf (fn, "%s/%s", gameFolders.szAltHogDir, filename);
   pfn = fn;
   fp = fopen (pfn, mode);
	}
//if (!fp) PrintLog ("CFGetFileHandle(): error opening %s\n", pfn);
CFCriticalError (fp == NULL);
return fp;
}

// ----------------------------------------------------------------------------
//returns 1 if file loaded with no errors
int CFInitHogFile (const char *pszFile, const char *folder, tHogFile *hogFiles, int *nFiles) 
{
	char	id [4];
	FILE	*fp;
	int	i, len;
	char	fn [FILENAME_LEN];
	char  *psz;

CFCriticalError (0);
if (*folder) {
	sprintf (fn, "%s/%s", folder, pszFile);
	pszFile = fn;
	}
*nFiles = 0;
if (!(fp = CFGetFileHandle (pszFile, "", "rb")))
	return 0;
if ((psz = strstr (pszFile, ".rdl")) || (psz = strstr (pszFile, ".rl2"))) {
	while ((psz >= pszFile) && (*psz != '\\') && (*psz != '/') && (*psz != ':'))
		psz--;
	*nFiles = 1;
	strncpy (hogFiles [0].name, psz + 1, 13);
	hogFiles [0].offset = 0;
	hogFiles [0].length = -1;
	return 1;
	}

fread (id, 3, 1, fp);
if (strncmp (id, "DHF", 3)) {
	fclose (fp);
	return 0;
	}

for (;;) {
	if (*nFiles >= MAX_HOGFILES) {
		fclose (fp);
		Error ("HOGFILE is limited to %d files.\n",  MAX_HOGFILES);
		}
	i = (int) fread (hogFiles [*nFiles].name, 13, 1, fp);
	if (i != 1) 	{		//eof here is ok
		fclose (fp);
		return 1;
		}
	hogFiles [*nFiles].name [12] = '\0';
	i = (int) fread (&len, 4, 1, fp);
	if (i != 1)	{
		fclose (fp);
		return 0;
		}
	len = INTEL_INT (len);
	hogFiles [*nFiles].length = len;
	hogFiles [*nFiles].offset = ftell (fp);
	(*nFiles)++;
	// Skip over
	i = fseek (fp, len, SEEK_CUR);
	}
}

// ----------------------------------------------------------------------------

int CFUseHogFile (tHogFileList *hogP, const char *name, const char *folder)
{
if (hogP->bInitialized)
	return 1;
if (name) {
	strcpy (hogP->szName, name);
	hogP->bInitialized = 
		*name && 
		CFInitHogFile (hogP->szName, folder, hogP->files, &hogP->nFiles);
	if (*(hogP->szName))
		PrintLog ("   found hogP file '%s'\n", hogP->szName);
	return hogP->bInitialized && (hogP->nFiles > 0);
	} 
return 0;
}

// ----------------------------------------------------------------------------

int CFUseAltHogFile (const char * name) 
{
gameHogFiles.AltHogFiles.bInitialized = 0;
return CFUseHogFile (&gameHogFiles.AltHogFiles, name, "");
}

// ----------------------------------------------------------------------------

int CFUseD2XHogFile (const char * name) 
{
return CFUseHogFile (&gameHogFiles.D2XHogFiles, name, gameFolders.szMissionDir);
}

// ----------------------------------------------------------------------------

int CFUseXLHogFile (const char * name) 
{
return CFUseHogFile (&gameHogFiles.XLHogFiles, name, gameFolders.szDataDir);
}

// ----------------------------------------------------------------------------

int CFUseExtraHogFile (const char * name) 
{
return gameStates.app.bHaveExtraData = 
	!gameStates.app.bNostalgia &&
	CFUseHogFile (&gameHogFiles.ExtraHogFiles, name, gameFolders.szDataDir);
}

// ----------------------------------------------------------------------------

int CFUseD1HogFile (const char * name) 
{
return CFUseHogFile (&gameHogFiles.D1HogFiles, name, gameFolders.szDataDir);
}

// ----------------------------------------------------------------------------
//Specify the name of the tHogFile.  Returns 1 if tHogFile found & had files
int CFileInit (char *pszHogName, char *pszFolder)
{
if (!*pszHogName) {
	memset (&gameHogFiles, 0, sizeof (gameHogFiles));
	memset (&gameFolders, 0, sizeof (gameFolders));
	return 1;
	}
Assert (gameHogFiles.D2HogFiles.bInitialized == 0);
if (CFInitHogFile (pszHogName, pszFolder, gameHogFiles.D2HogFiles.files, &gameHogFiles.D2HogFiles.nFiles)) {
	strcpy (gameHogFiles.D2HogFiles.szName, pszHogName);
	gameHogFiles.D2HogFiles.bInitialized = 1;
	CFUseD2XHogFile ("d2x.hog");
	CFUseXLHogFile ("d2x-xl.hog");
	CFUseExtraHogFile ("extra.hog");
	CFUseD1HogFile ("descent.hog");
	return 1;
	}
return 0;	//not loaded!
}

// ----------------------------------------------------------------------------

int CFSize (const char *hogname, const char *folder, int bUseD1Hog)
{
	CFILE cf;
//	char fn [FILENAME_LEN];
#if !(defined (_WIN32_WCE) || defined (_WIN32))
	struct stat statbuf;

//	sprintf (fn, "%s/%s", folder, hogname);
if (!CFOpen (&cf, hogname, gameFolders.szDataDir, "rb", bUseD1Hog))
	return -1;
fstat (fileno (cf.file), &statbuf);
CFClose (&cf);
return statbuf.st_size;
#else
	DWORD size;

//sprintf (fn, "%s%s%s", folder, *folder ? "/" : "", hogname);
if (!CFOpen (&cf, hogname, gameFolders.szDataDir, "rb", bUseD1Hog))
	return -1;
size = cf.size;
CFClose (&cf);
return size;
#endif
}

// ----------------------------------------------------------------------------
/*
 * return handle for file called "name", embedded in one of the hogfiles
 */

FILE *CFFindHogFile (tHogFileList *hog, const char *folder, const char *name, int *length)
{
	FILE		*fp;
	int		i;
	tHogFile	*phf;
	char		*hogFilename = hog->szName;
  
if (!(hog->bInitialized && *hogFilename))
	return NULL;
if (*folder) {
	char fn [FILENAME_LEN];

	sprintf (fn, "%s/%s", folder, hog->szName);
	hogFilename = fn;
	}

for (i = hog->nFiles, phf = hog->files; i; i--, phf++) {
	if (stricmp (phf->name, name))
		continue;
	if (!(fp = CFGetFileHandle (hogFilename, "", "rb")))
		break;
	fseek (fp, phf->offset, SEEK_SET);
	if (length)
		*length = phf->length;
	return fp;
	}
//PrintLog ("CFFindHogFile(): '%s:%s' not found\n", hogFilename, name);
return NULL;
}

// ----------------------------------------------------------------------------

FILE* CFFindLibFile (const char *name, int *length, int bUseD1Hog)
{
	FILE* fp;
  
if ((fp = CFFindHogFile (&gameHogFiles.AltHogFiles, "", name, length)))
	return fp;
if ((fp = CFFindHogFile (&gameHogFiles.XLHogFiles, gameFolders.szDataDir, name, length)))
	return fp;
if ((fp = CFFindHogFile (&gameHogFiles.ExtraHogFiles, gameFolders.szDataDir, name, length)))
	return fp;
if (bUseD1Hog) {
	if ((fp = CFFindHogFile (&gameHogFiles.D1HogFiles, gameFolders.szDataDir, name, length)))
		return fp;
	}
else {
	if ((fp = CFFindHogFile (&gameHogFiles.D2XHogFiles, gameFolders.szMissionDir, name, length)))
		return fp;
	if ((fp = CFFindHogFile (&gameHogFiles.D2HogFiles, gameFolders.szDataDir, name, length)))
		return fp;
	}
//PrintLog ("File '%s' not found\n", name);
return NULL;
}

// ----------------------------------------------------------------------------
// CFEoF() Tests for end-of-file on a stream
//
// returns a nonzero value after the first read operation that attempts to read
// past the end of the file. It returns 0 if the current position is not end of file.
// There is no error return.

int CFEoF (CFILE *cfP)
{
#ifdef _DEBUG
if (!(cfP && cfP->file))
	return 1;
#endif
return (cfP->raw_position >= cfP->size);
}

// ----------------------------------------------------------------------------

int CFError (CFILE *cfP)
{
return ferror (cfP->file);
}

// ----------------------------------------------------------------------------

int CFExist (const char *filename, const char *folder, int bUseD1Hog) 
{
	int	length, bNoHOG = 0;
	FILE	*fp;

if (*filename != '\x01') {
	bNoHOG = (*filename == '\x02');
	fp = CFGetFileHandle (filename + bNoHOG, folder, "rb"); // Check for non-hog file first...
	}
else {
	fp = NULL;		//don't look in dir, only in tHogFile
	filename++;
	}
if (fp) {
	fclose (fp);
	return 1;
	}
if (bNoHOG)
	return 0;
fp = CFFindLibFile (filename, &length, bUseD1Hog);
if (fp) {
	fclose (fp);
	return 2;		// file found in hog
	}
return 0;		// Couldn't find it.
}

// ----------------------------------------------------------------------------
// Deletes a file.
int CFDelete (const char *filename, const char* folder)
{
	char	fn [FILENAME_LEN];

sprintf (fn, "%s%s%s", folder, *folder ? "/" : "", filename);
#ifndef _WIN32_WCE
	return remove(fn);
#else
	return !DeleteFile(fn);
#endif
}

// ----------------------------------------------------------------------------
// Rename a file.
int CFRename (const char *oldname, const char *newname, const char *folder)
{
	char	fno [FILENAME_LEN], fnn [FILENAME_LEN];

sprintf (fno, "%s%s%s", folder, *folder ? "/" : "", oldname);
sprintf (fnn, "%s%s%s", folder, *folder ? "/" : "", newname);
#ifndef _WIN32_WCE
	return rename(fno, fnn);
#else
	return !MoveFile(fno, fnn);
#endif
}

// ----------------------------------------------------------------------------
// Make a directory.
int CFMkDir (const char *pathname)
{
#if defined (_WIN32_WCE) || defined (_WIN32)
return !CreateDirectory (pathname, NULL);
#else
return mkdir(pathname, 0755);
#endif
}

// ----------------------------------------------------------------------------

int CFOpen (CFILE *cfP, const char *filename, const char *folder, const char *mode, int bUseD1Hog) 
{
	int	length = -1;
	FILE	*fp = NULL;
	char	*pszHogExt, *pszFileExt;

cfP->file = NULL;
if (!(filename && *filename))
	return 0;
if ((*filename != '\x01') /*&& !bUseD1Hog*/) {
	fp = CFGetFileHandle (filename, folder, mode);		// Check for non-hog file first...
	if (!fp && 
		 ((pszFileExt = strstr (filename, ".rdl")) || (pszFileExt = strstr (filename, ".rl2"))) &&
		 (pszHogExt = strchr (gameHogFiles.szAltHogFile, '.')) &&
		 !stricmp (pszFileExt, pszHogExt))
		fp = CFGetFileHandle (gameHogFiles.szAltHogFile, folder, mode);		// Check for non-hog file first...
	}
else {
	fp = NULL;		//don't look in dir, only in tHogFile
	filename++;
	}

if (!fp) {
	if ((fp = CFFindLibFile (filename, &length, bUseD1Hog)))
		if (stricmp (mode, "rb")) {
			Error ("Cannot read hog file\n(wrong file io mode).\n");
			return 0;
			}
	}
if (!fp) 
	return 0;
cfP->file = fp;
cfP->raw_position = 0;
cfP->size = (length < 0) ? ffilelength (fp) : length;
cfP->lib_offset = (length < 0) ? 0 : ftell (fp);
cfP->filename = (char *) filename;
return 1;
}

// ----------------------------------------------------------------------------

int CFLength (CFILE *cfP, int bUseD1Hog) 
{
return cfP ? cfP->size : 0;
}

// ----------------------------------------------------------------------------
// CFWrite () writes to the file
//
// returns:   number of full elements actually written
//
//
int CFWrite (const void *buf, int nElemSize, int nElemCount, CFILE *cfP)
{
	int nWritten;

if (!cfP) {
	CFCriticalError (1);
	return 0;
	}
Assert (cfP != NULL);
Assert (buf != NULL);
Assert (nElemSize > 0);
Assert (cfP->file != NULL);
Assert (cfP->lib_offset == 0);
nWritten = (int) fwrite (buf, nElemSize, nElemCount, cfP->file);
cfP->raw_position = ftell (cfP->file);
CFCriticalError (nWritten != nElemCount);
return nWritten;
}

// ----------------------------------------------------------------------------
// CFPutC() writes a character to a file
//
// returns:   success ==> returns character written
//            error   ==> EOF
//
int CFPutC (int c, CFILE *cfP)
{
	int char_written;

Assert (cfP != NULL);
Assert (cfP->file != NULL);
Assert (cfP->lib_offset == 0);
char_written = fputc (c, cfP->file);
cfP->raw_position = ftell (cfP->file);
return char_written;
}

// ----------------------------------------------------------------------------

int CFGetC (CFILE *cfP) 
{
	int c;

if (cfP->raw_position >= cfP->size) 
	return EOF;
c = getc (cfP->file);
if (c != EOF)
	cfP->raw_position = ftell (cfP->file) - cfP->lib_offset;
return c;
}

// ----------------------------------------------------------------------------
// CFPutS() writes a string to a file
//
// returns:   success ==> non-negative value
//            error   ==> EOF
//
int CFPutS (const char *str, CFILE *cfP)
{
	int ret;

Assert (cfP != NULL);
Assert (str != NULL);
Assert (cfP->file != NULL);
ret = fputs(str, cfP->file);
cfP->raw_position = ftell(cfP->file);
return ret;
}

// ----------------------------------------------------------------------------

char * CFGetS (char * buf, size_t n, CFILE *cfP) 
{
	char * t = buf;
	size_t i;
	int c;

for (i = 0; i < n - 1; i++) {
	do {
		if (cfP->raw_position >= cfP->size) {
			*buf = 0;
			return (buf == t) ? NULL : t;
			}
		c = CFGetC (cfP);
		if (c == 0 || c == 10)       // Unix line ending
			break;
		if (c == 13) {      // Mac or DOS line ending
			int c1;

			c1 = CFGetC (cfP);
			CFSeek (cfP, -1, SEEK_CUR);
			if (c1 == 10) // DOS line ending
				continue;
			else            // Mac line ending
				break;
			}
		} while (c == 13);
 	if (c == 0 || c == 10 || c == 13)  // because cr-lf is a bad thing on the mac
 		break;   // and anyway -- 0xod is CR on mac, not 0x0a
	*buf++ = c;
	if (c == '\n')
		break;
	}
*buf++ = 0;
return  t;
}

// ----------------------------------------------------------------------------

size_t CFRead (void *buf, size_t elsize, size_t nelem, CFILE *cfP) 
{
unsigned int i, size = (int) (elsize * nelem);

if (!cfP || (size < 1)) {
	CFCriticalError (1);
	return 0;
	}
i = (int) fread (buf, 1, size, cfP->file);
CFCriticalError (i != size);
cfP->raw_position += i;
return i / elsize;
}


// ----------------------------------------------------------------------------

int CFTell (CFILE *cfP) 
{
return cfP ? cfP->raw_position : -1;
}

// ----------------------------------------------------------------------------

int CFSeek (CFILE *cfP, long int offset, int where) 
{
if (!cfP)
	return -1;

	int c, goal_position;

switch (where) {
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = cfP->raw_position + offset;
		break;
	case SEEK_END:
		goal_position = cfP->size + offset;
		break;
	default:
		return 1;
	}
c = fseek (cfP->file, cfP->lib_offset + goal_position, SEEK_SET);
CFCriticalError (c);
cfP->raw_position = ftell (cfP->file) - cfP->lib_offset;
return c;
}

// ----------------------------------------------------------------------------

int CFClose (CFILE *cfP)
{
	int result;

if (!(cfP && cfP->file))
	return 0;
result = fclose (cfP->file);
cfP->file = NULL;
return result;
}

// ----------------------------------------------------------------------------
// routines to read basic data types from CFILE's.  Put here to
// simplify mac/pc reading from cfiles.

int CFReadInt (CFILE *file)
{
	int32_t i;

CFCriticalError (CFRead (&i, sizeof (i), 1, file) != 1);
//Error ("Error reading int in CFReadInt()");
return INTEL_INT (i);
}

// ----------------------------------------------------------------------------

short CFReadShort (CFILE *file)
{
	int16_t s;

CFCriticalError (CFRead (&s, sizeof (s), 1, file) != 1);
//Error ("Error reading short in CFReadShort()");
return INTEL_SHORT (s);
}

// ----------------------------------------------------------------------------

sbyte CFReadByte (CFILE *file)
{
	sbyte b;

if (CFRead (&b, sizeof (b), 1, file) != 1)
	return nCFileError;
//Error ("Error reading byte in CFReadByte()");
return b;
}

// ----------------------------------------------------------------------------

float CFReadFloat (CFILE *file)
{
	float f;

CFCriticalError (CFRead (&f, sizeof (f), 1, file) != 1);
//Error ("Error reading float in CFReadFloat()");
return INTEL_FLOAT (f);
}

// ----------------------------------------------------------------------------
//Read and return a double (64 bits)
//Throws an exception of nType (nCFileError *) if the OS returns an error on read
double CFReadDouble (CFILE *file)
{
	double d;

CFCriticalError (CFRead (&d, sizeof (d), 1, file) != 1);
return INTEL_DOUBLE (d);
}

// ----------------------------------------------------------------------------

fix CFReadFix (CFILE *file)
{
	fix f;

CFCriticalError (CFRead (&f, sizeof (f), 1, file) != 1);
//Error ("Error reading fix in CFReadFix()");
return (fix) INTEL_INT ((int) f);
return f;
}

// ----------------------------------------------------------------------------

fixang CFReadFixAng (CFILE *file)
{
	fixang f;

CFCriticalError (CFRead (&f, 2, 1, file) != 1);
//Error("Error reading fixang in CFReadFixAng()");
return (fixang) INTEL_SHORT ((int) f);
}

// ----------------------------------------------------------------------------

void CFReadVector (vmsVector *v, CFILE *file)
{
v->p.x = CFReadFix (file);
v->p.y = CFReadFix (file);
v->p.z = CFReadFix (file);
}

// ----------------------------------------------------------------------------

void CFReadAngVec(vmsAngVec *v, CFILE *file)
{
v->p = CFReadFixAng (file);
v->b = CFReadFixAng (file);
v->h = CFReadFixAng (file);
}

// ----------------------------------------------------------------------------

void CFReadMatrix(vmsMatrix *m,CFILE *file)
{
CFReadVector (&m->rVec,file);
CFReadVector (&m->uVec,file);
CFReadVector (&m->fVec,file);
}


// ----------------------------------------------------------------------------

void CFReadString (char *buf, int n, CFILE *file)
{
	char c;

do {
	c = (char) CFReadByte (file);
	if (n > 0) {
		*buf++ = c;
		n--;
		}
	} while (c != 0);
}

// ----------------------------------------------------------------------------
// equivalent write functions of above read functions follow

int CFWriteInt(int i, CFILE *file)
{
i = INTEL_INT(i);
return CFWrite (&i, sizeof (i), 1, file);
}

// ----------------------------------------------------------------------------

int CFWriteShort(short s, CFILE *file)
{
s = INTEL_SHORT(s);
return CFWrite (&s, sizeof (s), 1, file);
}

// ----------------------------------------------------------------------------

int CFWriteByte (sbyte b, CFILE *file)
{
return CFWrite (&b, sizeof (b), 1, file);
}

// ----------------------------------------------------------------------------

int CFWriteFloat (float f, CFILE *file)
{
f = INTEL_FLOAT (f);
return CFWrite (&f, sizeof (f), 1, file);
}

// ----------------------------------------------------------------------------
//Read and return a double (64 bits)
//Throws an exception of nType (nCFileError *) if the OS returns an error on read
int cfile_write_double (double d, CFILE *file)
{
d = INTEL_DOUBLE (d);
return CFWrite (&d, sizeof (d), 1, file);
}

// ----------------------------------------------------------------------------

int CFWriteFix (fix x, CFILE *file)
{
x = INTEL_INT (x);
return CFWrite (&x, sizeof (x), 1, file);
}

// ----------------------------------------------------------------------------

int CFWriteFixAng (fixang a, CFILE *file)
{
a = INTEL_SHORT (a);
return CFWrite (&a, sizeof (a), 1, file);
}

// ----------------------------------------------------------------------------

void CFWriteVector (vmsVector *v, CFILE *file)
{
CFWriteFix (v->p.x, file);
CFWriteFix (v->p.y, file);
CFWriteFix (v->p.z, file);
}

// ----------------------------------------------------------------------------

void CFWriteAngVec (vmsAngVec *v, CFILE *file)
{
CFWriteFixAng (v->p, file);
CFWriteFixAng (v->b, file);
CFWriteFixAng (v->h, file);
}

// ----------------------------------------------------------------------------

void CFWriteMatrix (vmsMatrix *m,CFILE *file)
{
CFWriteVector (&m->rVec, file);
CFWriteVector (&m->uVec, file);
CFWriteVector (&m->fVec, file);
}


// ----------------------------------------------------------------------------

int CFWriteString (const char *buf, CFILE *file)
{
if (buf && *buf && CFWrite (buf, (int) strlen (buf), 1, file))
	return (int) CFWriteByte (0, file);   // write out NULL termination
return 0;
}

// ----------------------------------------------------------------------------

int CFExtract (const char *filename, const char *folder, int bUseD1Hog, char *szDestName)
{
	CFILE		cf;
	FILE		*fp;
	char		szDest [FILENAME_LEN], fn [FILENAME_LEN];
	static	char buf [4096];
	int		h, l;

if (!CFOpen (&cf, filename, folder, "rb", bUseD1Hog))
	return 0;
strcpy (fn, filename);
if (*szDestName) {
	if (*szDestName == '.') {
		char *psz = strchr (fn, '.');
		if (psz)
			strcpy (psz, szDestName);
		else
			strcat (fn, szDestName);
		}
	else
		strcpy (fn, szDestName);
	}
sprintf (szDest, "%s%s%s", gameFolders.szTempDir, *gameFolders.szTempDir ? "/" : "", fn);
if (!(fp = fopen (szDest, "wb"))) {
	CFClose (&cf);
	return 0;
	}
for (h = sizeof (buf), l = cf.size; l; l -= h) {
	if (h > l)
		h = l;
	CFRead (buf, h, 1, &cf);
	fwrite (buf, h, 1, fp);
	}
CFClose (&cf);
fclose (fp);
return 1;
}

// ----------------------------------------------------------------------------

char *CFReadData (const char *filename, const char *folder, int bUseD1Hog)
{
	CFILE		cf;
	char		*pData = NULL;
	size_t	nSize;

if (!CFOpen (&cf, filename, folder, "rb", bUseD1Hog))
	return NULL;
nSize = CFLength (&cf, bUseD1Hog);
if (!(pData = (char *) D2_ALLOC ((unsigned int) nSize)))
	return NULL;
if (!CFRead (pData, nSize, 1, &cf)) {
	D2_FREE (pData);
	pData = NULL;
	}
CFClose (&cf);
return pData;
}

// ----------------------------------------------------------------------------

void CFSplitPath (char *szFullPath, char *szFolder, char *szFile, char *szExt)
{
	int	h = 0, i, j, l = (int) strlen (szFullPath) - 1;

i = l;
#ifdef _WIN32
while ((i >= 0) && (szFullPath [i] != '/') && (szFullPath [i] != '\\') && (szFullPath [i] != ':'))
#else
while ((i >= 0) && (szFullPath [i] != '/'))
#endif
	i--;
i++;
j = l - 1;
while ((j > i) && (szFullPath [j] != '.'))
	j--;
if (szFolder) {
	if (i > 0)
		strncpy (szFolder, szFullPath, i);
	szFolder [i] = '\0';
	}
if (szFile) {
	if (!j || (j > i))
		strncpy (szFile, szFullPath + i, h = (j ? j : l + 1) - i);
	szFile [h] = '\0';
	}
if (szExt) {
	if (j && (j < l))
		strncpy (szExt, szFullPath + j, l - j);
	szExt [l - j] = '\0';
	}
}

//------------------------------------------------------------------------------

void ChangeFilenameExtension (char *dest, char *src, char *new_ext)
{
	int i;

strcpy (dest, src);
if (new_ext[0] == '.')
	new_ext++;
for (i = 1; i < (int) strlen (dest); i++)
	if ((dest[i] == '.') || (dest[i] == ' ') || (dest[i] == 0))
		break;
if (i < 123) {
	dest [i] = '.';
	dest [i+1] = new_ext[0];
	dest [i+2] = new_ext[1];
	dest [i+3] = new_ext[2];
	dest [i+4] = 0;
	}
}

// ----------------------------------------------------------------------------

#ifdef _WIN32
#	define fileno	_fileno
#endif

time_t CFDate (char *filename, char *folder, int bUseD1Hog)
{
	CFILE cf;
	struct stat statbuf;

//	sprintf (fn, "%s/%s", folder, hogname);
if (!CFOpen (&cf, filename, folder, "rb", bUseD1Hog))
	return -1;
fstat (fileno (cf.file), &statbuf);
CFClose (&cf);
return statbuf.st_mtime;
}

//------------------------------------------------------------------------------

char *GameDataFilename (char *pszFilename, char *pszExt, int nLevel, int nType)
{
	char	szFilename [FILENAME_LEN];

CFSplitPath (*gameHogFiles.AltHogFiles.szName ? gameHogFiles.AltHogFiles.szName : 
				 gameStates.app.bD1Mission ? gameHogFiles.D1HogFiles.szName : gameHogFiles.D2HogFiles.szName, 
				 NULL, szFilename, NULL);
if (nType < 0) {
	if (nLevel < 0)
		sprintf (pszFilename, "%s-s%d.%s", szFilename, -nLevel, pszExt);
	else
		sprintf (pszFilename, "%s-%d.%s", szFilename, nLevel, pszExt);
	}
else {
	if (nLevel < 0)
		sprintf (pszFilename, "%s-s%d.%s%d", szFilename, -nLevel, pszExt, nType);
	else
		sprintf (pszFilename, "%s-%d.%s%d", szFilename, nLevel, pszExt, nType);
	}
return pszFilename;
}

// ----------------------------------------------------------------------------
