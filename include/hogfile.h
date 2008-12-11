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

#ifndef _HOGFILE_H
#define _HOGFILE_H

#include <stdio.h>
#include <sys/types.h>

#include "maths.h"
#include "vecmat.h"

// the maximum length of a filename
#define FILENAME_LEN			255
#define SHORT_FILENAME_LEN 13
#define MAX_HOGFILES			300

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

class CHogFile {
	public:
		tGameHogFiles	m_files;
	public:
		CHogFile () {};
		~CHogFile () {};
		int Init (const char *pszHogName, const char *pszFolder);

		int UseXL (const char *name);
		int UseD2X (const char *name);
		int UseExtra (const char *name);
		int UseAlt (const char *name);
		int UseD1 (const char *name);
		void UseAltDir (const char *path);
		FILE* Find (const char *name, int *length, int bUseD1Hog);
		FILE *Find (tHogFileList *hogP, const char *folder, const char *name, int *length);

		inline tHogFileList& D1Files (void) { return m_files.D1HogFiles; }
		inline tHogFileList& D2Files (void) { return m_files.D2HogFiles; }
		inline tHogFileList& D2XFiles (void) { return m_files.D2XHogFiles; }
		inline tHogFileList& XLFiles (void) { return m_files.XLHogFiles; }
		inline tHogFileList& ExtraFiles (void) { return m_files.ExtraHogFiles; }
		inline tHogFileList& AltFiles (void) { return m_files.AltHogFiles; }
		char *AltHogFile (void) { return m_files.szAltHogFile; }

	private:
		void QuickSort (tHogFile *hogFiles, int left, int right);
		tHogFile *BinSearch (tHogFile *hogFiles, int nFiles, const char *pszFile);
		int Use (tHogFileList *hogP, const char *name, const char *folder);
		int Setup (const char *pszFile, const char *folder, tHogFile *hogFiles, int *nFiles);
};

extern CHogFile hogFileManager;

#endif
