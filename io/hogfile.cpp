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

#include "descent.h"
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "d_io.h"
#include "error.h"
#include "cfile.h"
#include "hogfile.h"
#include "byteswap.h"
#include "mission.h"
#include "console.h"
#include "findfile.h"

CHogFile hogFileManager;

// ----------------------------------------------------------------------------

void CHogFile::QuickSort (tHogFile *hogFiles, int left, int right)
{
	int		l = left,
				r = right;
	tHogFile	m = hogFiles [(l + r) / 2];

do {
	while (stricmp (hogFiles [l].name, m.name) < 0)
		l++;
	while (stricmp (hogFiles [r].name, m.name) > 0)
		r--;
	if (l <= r) {
		if (l < r) {
			tHogFile	h = hogFiles [l];
			hogFiles [l] = hogFiles [r];
			hogFiles [r] = h;
			}
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	QuickSort (hogFiles, l, right);
if (r > left)
	QuickSort (hogFiles, left, r);
}

// ----------------------------------------------------------------------------

tHogFile *CHogFile::BinSearch (tHogFile *hogFiles, int nFiles, const char *pszFile)
{
	int	i, m,
			l = 0,
			r = nFiles - 1;

do {
	m = (l + r) / 2;
	i = stricmp (hogFiles [m].name, pszFile);
	if (i < 0)
		l = m + 1;
	else if (i > 0)
		r = m - 1;
	else
		return hogFiles + m;
	} while (l <= r);
return NULL;
}

// ----------------------------------------------------------------------------
//returns 1 if file loaded with no errors
int CHogFile::Setup (const char *pszFile, const char *folder, tHogFile *hogFiles, int *nFiles) 
{
	char	id [4];
	FILE	*fp;
	int	i, len;
	char	fn [FILENAME_LEN];
	const char  *psz;

if (*folder) {
	sprintf (fn, "%s/%s", folder, pszFile);
	pszFile = fn;
	}
*nFiles = 0;
if (! (fp = CFile::GetFileHandle (pszFile, "", "rb")))
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
	if (i != 1) {		//eof here is ok
		fclose (fp);
		return 1;
		}
	hogFiles [*nFiles].name [12] = '\0';
	i = (int) fread (&len, 4, 1, fp);
	if (i != 1) {
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

int CHogFile::Use (tHogFileList *hogP, const char *name, const char *folder)
{
if (hogP->bInitialized)
	return 1;
if (name) {
	strcpy (hogP->szName, name);
	hogP->bInitialized = *name && Setup (hogP->szName, folder, hogP->files, &hogP->nFiles);
	if (*(hogP->szName))
		PrintLog ("   found hogP file '%s'\n", hogP->szName);
	if (hogP->bInitialized && (hogP->nFiles > 0)) {
		QuickSort (hogP->files, 0, hogP->nFiles - 1);
		return 1;
		}
	} 
return 0;
}

// ----------------------------------------------------------------------------

void CHogFile::UseAltDir (const char * path) 
{
gameFolders.bAltHogDirInited = 
	 (strcmp (path, gameFolders.szDataDir) != 0) && (GetAppFolder ("", gameFolders.szAltHogDir, path, "descent2.hog") == 0);
}

// ----------------------------------------------------------------------------

int CHogFile::UseAlt (const char * name) 
{
m_files.AltHogFiles.bInitialized = 0;
return Use (&m_files.AltHogFiles, name, "");
}

// ----------------------------------------------------------------------------

int CHogFile::UseD2X (const char * name) 
{
return Use (&m_files.D2XHogFiles, name, gameFolders.szMissionDir);
}

// ----------------------------------------------------------------------------

int CHogFile::UseXL (const char * name) 
{
return Use (&m_files.XLHogFiles, name, gameFolders.szDataDir);
}

// ----------------------------------------------------------------------------

int CHogFile::UseExtra (const char * name) 
{
return gameStates.app.bHaveExtraData = 
	!gameStates.app.bNostalgia &&
	Use (&m_files.ExtraHogFiles, name, gameFolders.szDataDir);
}

// ----------------------------------------------------------------------------

int CHogFile::UseD1 (const char * name) 
{
return Use (&m_files.D1HogFiles, name, gameFolders.szDataDir);
}

// ----------------------------------------------------------------------------
// return handle for file called "name", embedded in one of the hogfiles

FILE *CHogFile::Find (tHogFileList *hogP, const char *folder, const char *name, int *length)
{
	FILE		*fp;
	tHogFile	*phf;
	char		*hogFilename = hogP->szName;
  
if (! (hogP->bInitialized && *hogFilename))
	return NULL;
if (*folder) {
	char fn [FILENAME_LEN];

	sprintf (fn, "%s/%s", folder, hogP->szName);
	hogFilename = fn;
	}
if ((phf = BinSearch (hogP->files, hogP->nFiles, name))) {
	if (!(fp = CFile::GetFileHandle (hogFilename, "", "rb")))
		return NULL;
	fseek (fp, phf->offset, SEEK_SET);
	if (length)
		*length = phf->length;
	return fp;
	}
return NULL;
}

// ----------------------------------------------------------------------------

FILE* CHogFile::Find (const char *name, int *length, int bUseD1Hog)
{
	FILE* fp;
  
if ((fp = Find (&m_files.AltHogFiles, "", name, length)))
	return fp;
if ((fp = Find (&m_files.XLHogFiles, gameFolders.szDataDir, name, length)))
	return fp;
if ((fp = Find (&m_files.ExtraHogFiles, gameFolders.szDataDir, name, length)))
	return fp;
if (bUseD1Hog) {
	if ((fp = Find (&m_files.D1HogFiles, gameFolders.szDataDir, name, length)))
		return fp;
	}
else {
	if ((fp = Find (&m_files.D2XHogFiles, gameFolders.szMissionDir, name, length)))
		return fp;
	if ((fp = Find (&m_files.D2HogFiles, gameFolders.szDataDir, name, length)))
		return fp;
	}
//PrintLog ("File '%s' not found\n", name);
return NULL;
}

// ----------------------------------------------------------------------------
//Specify the name of the tHogFile.  Returns 1 if tHogFile found & had files
int CHogFile::Init (const char *pszHogName, const char *pszFolder)
{
if (!*pszHogName) {
	memset (&m_files, 0, sizeof (m_files));
	memset (&gameFolders, 0, sizeof (gameFolders));
	return 1;
	}
Assert (m_files.D2HogFiles.bInitialized == 0);
if (Setup (pszHogName, pszFolder, m_files.D2HogFiles.files, &m_files.D2HogFiles.nFiles)) {
	strcpy (m_files.D2HogFiles.szName, pszHogName);
	m_files.D2HogFiles.bInitialized = 1;
	QuickSort (m_files.D2HogFiles.files, 0, m_files.D2HogFiles.nFiles - 1);
	UseD2X ("d2x.hog");
	UseXL ("d2x-xl.hog");
	UseExtra ("extra.hog");
	UseD1 ("descent.hog");
	return 1;
	}
return 0;	//not loaded!
}

// ----------------------------------------------------------------------------
