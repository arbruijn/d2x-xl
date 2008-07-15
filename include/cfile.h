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

#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>
#include <sys/types.h>

#include "maths.h"
#include "vecmat.h"

// the maximum length of a filename
#define FILENAME_LEN 255
#define SHORT_FILENAME_LEN 13
#define MAX_HOGFILES 300

typedef struct CFILE {
	FILE		*file;
	char		*filename;
	int		size;
	int		lib_offset;
	int		raw_position;
} CFILE;

typedef struct tHogFile {
	char		name [13];
	int		offset;
	int		length;
} tHogFile;

typedef struct tHogFileList {
	tHogFile		files [MAX_HOGFILES];
	char			szName [FILENAME_LEN];
	int			nFiles;
	int			bInitialized;
} tHogFileList;

typedef struct tGameHogFiles {
	tHogFileList D1HogFiles;
	tHogFileList D2HogFiles;
	tHogFileList D2XHogFiles;
	tHogFileList XLHogFiles;
	tHogFileList ExtraHogFiles;
	tHogFileList AltHogFiles;
	char szAltHogFile [FILENAME_LEN];
	int bAltHogFileInited;
} tGameHogFiles;

typedef struct tGameFolders {
	char szHomeDir [FILENAME_LEN];
	char szGameDir [FILENAME_LEN];
	char szDataDir [FILENAME_LEN];
	char szShaderDir [FILENAME_LEN];
	char szModelDir [2][FILENAME_LEN];
	char szModelCacheDir [FILENAME_LEN];
	char szSoundDir [2][FILENAME_LEN];
	char szTextureDir [2][FILENAME_LEN];
	char szTextureCacheDir [2][FILENAME_LEN];
	char szMissionDir [FILENAME_LEN];
	char szConfigDir [FILENAME_LEN];
	char szSaveDir [FILENAME_LEN];
	char szProfDir [FILENAME_LEN];
	char szMovieDir [FILENAME_LEN];
	char szScrShotDir [FILENAME_LEN];
	char szDemoDir [FILENAME_LEN];
	char szAltHogDir [FILENAME_LEN];
	char szMissionDirs [2][FILENAME_LEN];
	char szMsnSubDir [FILENAME_LEN];
	char szTempDir [FILENAME_LEN];
	int bAltHogDirInited;
} tGameFolders;

int GetAppFolder (const char *szRootDir, char *szFolder, const char *szName, const char *szFilter);

//Specify the name of the tHogFile.  Returns 1 if tHogFile found & had files
int CFileInit (const char *hogname, const char *folder);

int CFSize (const char *hogname, const char *folder, int bUseD1Hog);

int CFOpen (CFILE *fp, const char *filename, const char *folder, const char *mode, int bUseD1Hog);
int CFLength (CFILE *fp, int bUseD1Hog);							// Returns actual size of file...
size_t CFRead (void *buf, size_t elsize, size_t nelem, CFILE *fp);
int CFClose (CFILE *fp);
int CFGetC (CFILE *fp);
int CFSeek (CFILE *fp, long int offset, int where);
int CFTell (CFILE *fp);
char *CFGetS (char *buf, size_t n, CFILE *fp);

int CFEoF (CFILE *fp);
int CFError (CFILE *fp);

int CFExist (const char *filename, const char *folder, int bUseD1Hog);	// Returns true if file exists on disk (1) or in hog (2).

// Deletes a file.
int CFDelete (const char *filename, const char *folder);

// Rename a file.
int CFRename (const char *oldname, const char *newname, const char *folder);

// Make a directory
int CFMkDir (const char *pathname);

// CFWrite () writes to the file
int CFWrite (const void *buf, int elsize, int nelem, CFILE *fp);

// CFPutC () writes a character to a file
int CFPutC (int c, CFILE *fp);

// CFPutS () writes a string to a file
int CFPutS (const char *str, CFILE *fp);

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
// Returns 1 if tHogFile found (& contains file), else 0.  
// If NULL passed, returns 1
int CFUseAltHogFile (const char *name);

// Allows files to be gotten from the Descent 1 hog file.
// Passing NULL disables this.
// Returns 1 if tHogFile found (& contains file), else 0.
// If NULL passed, returns 1
int CFUseD1HogFile (const char *name);

// All fp functions will check this directory if no file exists
// in the current directory.
void CFUseAltHogDir (char *path);

//tell fp about your critical error counter 
void CFSetCriticalErrorCounterPtr (int *ptr);

FILE *CFFindHogFile (tHogFileList *hog, const char *folder, const char *name, int *length);

int CFExtract (const char *filename, const char *folder, int bUseD1Hog, const char *szDest);

char *GameDataFilename (char *pszFilename, const char *pszExt, int nLevel, int nType);

// prototypes for reading basic types from fp
int CFReadInt (CFILE *cfP);
short CFReadShort (CFILE *cfP);
sbyte CFReadByte (CFILE *cfP);
fix CFReadFix (CFILE *cfP);
fixang CFReadFixAng (CFILE *cfP);
void CFReadVector (vmsVector *v, CFILE *cfP);
void CFReadAngVec (vmsAngVec *v, CFILE *cfP);
void CFReadMatrix (vmsMatrix *v, CFILE *cfP);
float CFReadFloat (CFILE *cfP);
double CFReadDouble (CFILE *cfP);
char *CFReadData (const char *filename, const char *folder, int bUseD1Hog);

// Reads variable length, null-termined string.   Will only read up
// to n characters.
void CFReadString (char *buf, int n, CFILE *cfP);

// functions for writing cfiles
int CFWriteFix (fix x, CFILE *cfP);
int CFWriteInt (int i, CFILE *cfP);
int CFWriteShort (short s, CFILE *cfP);
int CFWriteByte (sbyte u, CFILE *cfP);
int CFWriteFixAng (fixang a, CFILE *cfP);
void CFWriteAngVec (vmsAngVec *v, CFILE *cfP);
void CFWriteVector (vmsVector *v, CFILE *cfP);
void CFWriteMatrix (vmsMatrix *m, CFILE *cfP);
void CFSplitPath (const char *szFullPath, char *szFolder, char *szFile, char *szExt);
void ChangeFilenameExtension (char *dest, const char *src, const char *newExt);
time_t CFDate (const char *hogname, const char *folder, int bUseD1Hog);

// writes variable length, null-termined string.
int CFWriteString (const char *buf, CFILE *cfP);

#ifdef _WIN32
char *UnicodeToAsc (char *str, const wchar_t *w_str);
wchar_t *AscToUnicode (wchar_t *w_str, const char *str);
#endif

extern int nCFileError;
extern tGameHogFiles	gameHogFiles;
extern tGameFolders	gameFolders;

#endif
