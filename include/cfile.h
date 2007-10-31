/*$Id: cfile.h,v 1.10 2003/10/04 02:58:23 btb Exp $ */
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

#include "maths.h"
#include "vecmat.h"

// the maximum length of a filename
#define FILENAME_LEN 255
#define SHORT_FILENAME_LEN 13
#define MAX_HOGFILES 300

typedef struct CFILE {
	FILE    *file;
	int     size;
	int     lib_offset;
	int     raw_position;
} CFILE;

typedef struct hogfile {
	char    name[13];
	int     offset;
	int     length;
} hogfile;

typedef struct tHogFileList {
	hogfile		files [MAX_HOGFILES];
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
	char szModelDir [FILENAME_LEN];
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

int GetAppFolder (char *szRootDir, char *szFolder, char *szName, char *szFilter);

//Specify the name of the hogfile.  Returns 1 if hogfile found & had files
int CFileInit (char *hogname, char *folder);

int CFSize (char *hogname, char *folder, int bUseD1Hog);

CFILE *CFOpen (char *filename, char *folder, char *mode, int bUseD1Hog);
int CFLength (CFILE *fp, int bUseD1Hog);							// Returns actual size of file...
size_t CFRead (void *buf, size_t elsize, size_t nelem, CFILE *fp);
int CFClose (CFILE *cfile);
int CFGetC (CFILE *fp);
int CFSeek (CFILE *fp, long int offset, int where);
int CFTell (CFILE *fp);
char *CFGetS (char *buf, size_t n, CFILE *fp);

int CFEoF (CFILE *cfile);
int CFError (CFILE *cfile);

int CFExist (char *filename, char *folder, int bUseD1Hog);	// Returns true if file exists on disk (1) or in hog (2).

// Deletes a file.
int CFDelete (char *filename, char* folder);

// Rename a file.
int CFRename (char *oldname, char *newname, char *folder);

// Make a directory
int CFMkDir (char *pathname);

// CFWrite () writes to the file
int CFWrite (void *buf, int elsize, int nelem, CFILE *cfile);

// CFPutC () writes a character to a file
int CFPutC (int c, CFILE *cfile);

// CFPutS () writes a string to a file
int CFPutS (char *str, CFILE *cfile);

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
// Returns 1 if hogfile found (& contains file), else 0.  
// If NULL passed, returns 1
int CFUseAltHogFile (char *name);

// Allows files to be gotten from the Descent 1 hog file.
// Passing NULL disables this.
// Returns 1 if hogfile found (& contains file), else 0.
// If NULL passed, returns 1
int CFUseD1HogFile (char *name);

// All cfile functions will check this directory if no file exists
// in the current directory.
void CFUseAltHogDir (char *path);

//tell cfile about your critical error counter 
void CFSetCriticalErrorCounterPtr (int *ptr);

FILE *CFFindHogFile (tHogFileList *hog, char *folder, char *name, int *length);

int CFExtract (char *filename, char *folder, int bUseD1Hog, char *szDest);

// prototypes for reading basic types from cfile
int CFReadInt (CFILE *file);
short CFReadShort (CFILE *file);
sbyte CFReadByte (CFILE *file);
fix CFReadFix (CFILE *file);
fixang CFReadFixAng (CFILE *file);
void CFReadVector (vmsVector *v, CFILE *file);
void CFReadAngVec (vmsAngVec *v, CFILE *file);
void CFReadMatrix (vmsMatrix *v, CFILE *file);
float CFReadFloat (CFILE *file);
double CFReadDouble (CFILE *file);
char *CFReadData (char *filename, char *folder, int bUseD1Hog);

// Reads variable length, null-termined string.   Will only read up
// to n characters.
void CFReadString (char *buf, int n, CFILE *file);

// functions for writing cfiles
int CFWriteFix (fix x, CFILE *file);
int CFWriteInt (int i, CFILE *file);
int CFWriteShort (short s, CFILE *file);
int CFWriteByte (sbyte u, CFILE *file);
int CFWriteFixAng (fixang a, CFILE *file);
void CFWriteAngVec (vmsAngVec *v, CFILE *file);
void CFWriteVector (vmsVector *v, CFILE *file);
void CFWriteMatrix (vmsMatrix *m,CFILE *file);
void CFSplitPath (char *szFullPath, char *szFolder, char *szFile, char *szExt);
void ChangeFilenameExtension (char *dest, char *src, char *new_ext);

// writes variable length, null-termined string.
int CFWriteString (char *buf, CFILE *file);

extern int nCFileError;

extern tGameHogFiles	gameHogFiles;
extern tGameFolders	gameFolders;

#endif
