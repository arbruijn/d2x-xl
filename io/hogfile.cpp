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
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#	include <sys/stat.h>
#else
#	include <sys/stat.h>
#endif

#include "descent.h"
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "error.h"
#include "cfile.h"
#include "hogfile.h"
#include "byteswap.h"
#include "mission.h"
#include "console.h"
#include "findfile.h"

CHogFile hogFileManager;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class CLevelHeader {
	public:
		int32_t	m_size;
		char	m_name [13];
		char	m_longName [256];
		int32_t	m_bExtended;

	public:
		explicit CLevelHeader (int32_t bExtended = 0) : m_size (0), m_bExtended (bExtended) { m_name [0] = '\0'; m_longName [0] = '\0'; }

		inline char* Name (void) { return m_bExtended ? m_longName : m_name; }
		inline int32_t NameSize (void) { return m_bExtended ? sizeof (m_longName) : sizeof (m_name); }
		inline int32_t Size (void) { return sizeof (m_size) + sizeof (m_name) + m_bExtended * sizeof (m_longName); }
		inline int32_t FileSize (void) { return m_bExtended ? -m_size : m_size; }
		inline void SetFileSize (int32_t size) { m_size = m_bExtended ? -size : size; }
		inline int32_t Extended (void) { return m_size < 0; }

		int32_t Read (FILE* fp);
	};

//------------------------------------------------------------------------------

int32_t CLevelHeader::Read (FILE* fp) 
{
if (fread (m_name, 1, sizeof (m_name), fp) != sizeof (m_name))
	return 0;
if (fread (&m_size, sizeof (m_size), 1, fp) != 1)
	return 1;
m_size = INTEL_INT (m_size);
if ((m_bExtended = m_size < 0)) {
	if (fread (m_longName, 1, sizeof (m_longName), fp) != sizeof (m_longName))
		return 0;
	}
return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CHogFile::QuickSort (tHogFile *hogFiles, int32_t left, int32_t right)
{
	int32_t		l = left,
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

//------------------------------------------------------------------------------

tHogFile *CHogFile::BinSearch (tHogFile *hogFiles, int32_t nFiles, const char *pszFile)
{
	int32_t	i, m,
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
// fills the data structure pointed to by hogfiles with the level file names
// found in the mission named *pszFile in folder *folder.
// returns 1 if file loaded with no errors
int32_t CHogFile::Setup (const char *pszFile, const char *folder, tHogFile *hogFiles, int32_t *nFiles) 
{
	FILE	*fp;
	char	fn [FILENAME_LEN];

if (*folder) {
	char filename [FILENAME_LEN], extension [FILENAME_LEN];
	CFile::SplitPath (pszFile, NULL, filename, extension);
	sprintf (fn, "%s%s%s", folder, filename, extension);
	pszFile = fn;
	}
*nFiles = 0;
if (! (fp = CFile::GetFileHandle (pszFile, "", "rb")))
	return 0;

const char* psz;
if ((psz = strstr (pszFile, ".rdl")) || (psz = strstr (pszFile, ".rl2"))) {
	while ((psz >= pszFile) && (*psz != '\\') && (*psz != '/') && (*psz != ':'))
		psz--;
	*nFiles = 1;
	strncpy (hogFiles [0].name, psz + 1, sizeof (hogFiles [0].name));
	hogFiles [0].offset = 0;
	hogFiles [0].length = -1;
	return 1;
	}

char sig [4];
if (fread (sig, 3, 1, fp) != 1) {
	fclose (fp);
	return 0;
	}
if (strncmp (sig, "DHF", 3) && strncmp (sig, "D2X", 3)) {
	fclose (fp);
	return 0;
	}

CLevelHeader lh;

for (;;) {
	if (*nFiles >= MAX_HOGFILES) {
		fclose (fp);
		Warning ("HOG file is limited to %d files.\n",  MAX_HOGFILES);
		}
	if (!lh.Read (fp)) {		//eof here is ok
		fclose (fp);
		return 1;
		}
	strcpy (hogFiles [*nFiles].name, lh.Name ());
	hogFiles [*nFiles].length = lh.FileSize ();
	hogFiles [*nFiles].offset = ftell (fp);
	(*nFiles)++;
	// Skip over
	fseek (fp, lh.FileSize (), SEEK_CUR);
	}
return 0;
}

// ----------------------------------------------------------------------------

int32_t CHogFile::Use (tHogFileList *pHogFiles, const char *name, const char *folder)
{
if (pHogFiles->bInitialized)
	return 1;
if (name) {
	strcpy (pHogFiles->szName, name);
	strcpy (pHogFiles->szFolder, folder ? folder : "");
	pHogFiles->bInitialized = *name && Setup (pHogFiles->szName, folder, pHogFiles->files, &pHogFiles->nFiles);
	if (*(pHogFiles->szName))
		PrintLog (0, "found hog file '%s'\n", pHogFiles->szName);
	if (pHogFiles->bInitialized && (pHogFiles->nFiles > 0)) {
		QuickSort (pHogFiles->files, 0, pHogFiles->nFiles - 1);
		return 1;
		}
	} 
return 0;
}

// ----------------------------------------------------------------------------

int32_t CHogFile::Reload (tHogFileList *pHogFiles)
{
if (!*pHogFiles->szName)
	return 0;
pHogFiles->bInitialized = Setup (pHogFiles->szName, pHogFiles->szFolder, pHogFiles->files, &pHogFiles->nFiles);
if (*(pHogFiles->szName))
	PrintLog (0, "found hog file '%s'\n", pHogFiles->szName);
if (pHogFiles->bInitialized && (pHogFiles->nFiles > 0)) {
	QuickSort (pHogFiles->files, 0, pHogFiles->nFiles - 1);
	return 1;
	} 
return 0;
}

// ----------------------------------------------------------------------------

void CHogFile::UseAltDir (const char * path) 
{
gameFolders.bAltHogDirInited = 
	 (strcmp (path, gameFolders.game.szData [0]) != 0) && (GetAppFolder ("", gameFolders.game.szAltHogs, path, "descent2.hog") == 0);
}

// ----------------------------------------------------------------------------

int32_t CHogFile::UseMission (const char * name) 
{
m_files.MsnHogFiles.bInitialized = 0;
if (!Use (&m_files.MsnHogFiles, name, ""))
	return 0;
return 1;
}

// ----------------------------------------------------------------------------

int32_t CHogFile::ReloadMission (const char * name) 
{
if (name && *name) {
	strncpy (m_files.MsnHogFiles.szFolder, name, sizeof (m_files.MsnHogFiles.szFolder));
	char filename [FILENAME_LEN], extension [FILENAME_LEN];
	CFile::SplitPath (m_files.MsnHogFiles.szName, NULL, filename, extension);
	sprintf (m_files.MsnHogFiles.szName, "%s%s", filename, extension);
	}
return Reload (&m_files.MsnHogFiles);
}

// ----------------------------------------------------------------------------

int32_t CHogFile::UseD2X (const char * name) 
{
return Use (&m_files.D2XHogFiles, name, gameFolders.missions.szRoot);
}

// ----------------------------------------------------------------------------

int32_t CHogFile::UseXL (const char * name) 
{
return Use (&m_files.XLHogFiles, name, gameFolders.game.szData [0]);
}

// ----------------------------------------------------------------------------

int32_t CHogFile::UseExtra (const char * name) 
{
return gameStates.app.bHaveExtraData = 
	!gameStates.app.bNostalgia &&
	Use (&m_files.ExtraHogFiles, name, gameFolders.game.szData [0]);
}

// ----------------------------------------------------------------------------

int32_t CHogFile::UseD1 (const char * name) 
{
return Use (&m_files.D1HogFiles, name, gameFolders.game.szData [0]);
}

// ----------------------------------------------------------------------------
// return handle for file called "name", embedded in one of the hogfiles

FILE *CHogFile::Find (tHogFileList *pHogFiles, const char *folder, const char *name, int32_t *length)
{
	FILE		*fp;
	tHogFile	*phf;
	char		*hogFilename = pHogFiles->szName;
	char		fn [FILENAME_LEN];
  
if (! (pHogFiles->bInitialized && *hogFilename))
	return NULL;
if (*folder) {
	sprintf (fn, "%s%s", folder, pHogFiles->szName);
	hogFilename = fn;
	}
if ((phf = BinSearch (pHogFiles->files, pHogFiles->nFiles, name))) {
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

FILE* CHogFile::Find (const char *name, int32_t *length, int32_t bUseD1Hog)
{
	FILE* fp;
  
if ((fp = Find (&m_files.MsnHogFiles, "", name, length)))
	return fp;
if ((fp = Find (&m_files.XLHogFiles, gameFolders.game.szData [0], name, length)))
	return fp;
if ((fp = Find (&m_files.ExtraHogFiles, gameFolders.game.szData [0], name, length)))
	return fp;
if (bUseD1Hog) {
	if ((fp = Find (&m_files.D1HogFiles, gameFolders.game.szData [0], name, length)))
		return fp;
	}
else {
	if ((fp = Find (&m_files.D2XHogFiles, gameFolders.missions.szRoot, name, length)))
		return fp;
	if ((fp = Find (&m_files.D2HogFiles, gameFolders.game.szData [0], name, length)))
		return fp;
	}
return NULL;
}

// ----------------------------------------------------------------------------
//Specify the name of the tHogFile.  Returns 1 if tHogFile found & had files
int32_t CHogFile::Init (const char *pszHogName, const char *pszFolder)
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
