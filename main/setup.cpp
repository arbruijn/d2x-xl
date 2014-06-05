#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(__unix__) || defined(__macosx__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef __macosx__
#	include "SDL/SDL_main.h"
#	include "SDL/SDL_keyboard.h"
#	include "FolderDetector.h"
#else
#	include "SDL_main.h"
#	include "SDL_keyboard.h"
#endif
#include "descent.h"
#include "findfile.h"
#include "args.h"
#include "config.h"
#include "text.h"

#ifdef __macosx__
#	include <SDL/SDL.h>
#	if USE_SDL_MIXER
#		include <SDL_mixer/SDL_mixer.h>
#	endif
#else
#	include <SDL.h>
#	if USE_SDL_MIXER
#		include <SDL_mixer.h>
#	endif
#endif
#include "vers_id.h"
#include "cfile.h"

// ----------------------------------------------------------------------------

#if defined (_WIN32) || defined(__unix__)

typedef struct tFileDesc {
	const char*	pszFile;
	const char*	pszFolder;
	bool	bOptional;
	bool	bUser;
	bool	bFound;
} tFileDesc;

static tFileDesc gameFilesD2 [] = {
	// basic game files
	{"\002descent.cfg", "config", true, true, false},
	{"\002alien1.pig", "data", false, false, false},
	{"\002alien2.pig", "data", false, false, false},
	{"\002fire.pig", "data", false, false, false},
	{"\002groupa.pig", "data", false, false, false},
	{"\002ice.pig", "data", false, false, false},
	{"\002water.pig", "data", false, false, false},
	{"\002descent2.hog", "data", false, false, false},
	{"\002descent2.ham", "data", false, false, false},
	{"\002descent2.s11", "data", false, false, false},
	{"\002descent2.s22", "data", false, false, false},
	{"\002intro-h.mvl", "movies", true, false, false},
	{"\002intro-l.mvl", "movies", true, false, false},
	{"\002other-h.mvl", "movies", true, false, false},
	{"\002other-l.mvl", "movies", true, false, false},
	{"\002robots-h.mvl", "movies", true, false, false},
	{"\002robots-l.mvl", "movies", true, false, false}
};

static tFileDesc demoFilesD2 [] = {
	// Descent 2 demo files
	{"d2demo.hog", "data", false, false, false},
	{"d2demo.ham", "data", false, false, false},
	{"d2demo.pig", "data", false, false, false}
};

static tFileDesc gameFilesD1 [] = {
	// Descent 1 game files
	{"\002descent.pig", "data", false, false, false},
	{"\002descent.hog", "data", false, false, false}
};

static tFileDesc vertigoFiles [] = {
	// Vertigo expansion
	{"\002hoard.ham", "data", false, false, false},
	{"\002d2x.hog", "missions", false, false, false},
	{"\002d2x.mn2", "missions", false, false, false},
	{"\002d2x-h.mvl", "movies", true, false, false},
	{"\002d2x-l.mvl", "movies", true, false, false},
};

static tFileDesc addonFiles [] = {
	// D2X-XL addon files
	{"\002d2x-default.ini", "config", false, true, false},
	{"\002d2x.ini", "config", true, true, false},

	{"\002d2x-xl.hog", "data", false, false, false},
	{"\002exit.ham", "data", true, false, false},

	{"*.plx", "profiles", true, true, false},
	{"*.plr", "profiles", true, true, false},

	{"\002bullet.ase", "models", false, false, false},
	{"\002bullet.tga", "models", false, false, false},

	{"*.sg?", "savegames", true, true, false}
	};

static tFileDesc addonTextureFiles [] = {
	{"\002bullettime#0.tga", "textures/d2", false, false, false},
	{"\002cockpit.tga", "textures/d2", false, false, false},
	{"\002cockpitb.tga", "textures/d2", false, false, false},
	{"\002monsterball.tga", "textures/d2", false, false, false},
	{"\002slowmotion#0.tga", "textures/d2", false, false, false},
	{"\002status.tga", "textures/d2", false, false, false},
	{"\002statusb.tga", "textures/d2", false, false, false},

	{"\002aimdmg.tga", "textures/d2x-xl", false, false, false},
	{"\002blast.tga", "textures/d2x-xl", false, false, false},
	{"\002blast-hard.tga", "textures/d2x-xl", true, false, false},
	{"\002blast-medium.tga", "textures/d2x-xl", true, false, false},
	{"\002blast-soft.tga", "textures/d2x-xl", true, false, false},
	{"\002bubble.tga", "textures/d2x-xl", false, false, false},
	{"\002bullcase.tga", "textures/d2x-xl", false, false, false},
	{"\002corona.tga", "textures/d2x-xl", false, false, false},
	{"\002deadzone.tga", "textures/d2x-xl", false, false, false},
	{"\002drivedmg.tga", "textures/d2x-xl", false, false, false},
	{"\002fire.tga", "textures/d2x-xl", false, false, false},
	{"\002glare.tga", "textures/d2x-xl", false, false, false},
	{"\002gundmg.tga", "textures/d2x-xl", false, false, false},
	{"\002halfhalo.tga", "textures/d2x-xl", false, false, false},
	{"\002halo.tga", "textures/d2x-xl", false, false, false},
	{"\002joymouse.tga", "textures/d2x-xl", false, false, false},
	{"\002pwupicon.tga", "textures/d2x-xl", false, false, false},
	{"\002rboticon.tga", "textures/d2x-xl", false, false, false},
	{"\002scope.tga", "textures/d2x-xl", false, false, false},
	{"\002shield.tga", "textures/d2x-xl", false, false, false},
	{"\002smoke.tga", "textures/d2x-xl", false, false, false},
	{"\002smoke-hard.tga", "textures/d2x-xl", true, false, false},
	{"\002smoke-medium.tga", "textures/d2x-xl", true, false, false},
	{"\002smoke-soft.tga", "textures/d2x-xl", true, false, false},
	{"\002sparks.tga", "textures/d2x-xl", false, false, false},
	{"\002thruster.tga", "textures/d2x-xl", false, false, false}
};

static tFileDesc addonSoundFiles [] = {
	{"\002afbr_1.wav", "sounds2", false, false, false},
	{"\002airbubbles.wav", "sounds2/d2x-xl", false, false, false},
	{"\002fire.wav", "sounds2/d2x-xl", false, false, false},
	{"\002gatling-slowdown.wav", "sounds2/d2x-xl", false, false, false},
	{"\002gatling-speedup.wav", "sounds2/d2x-xl", false, false, false},
	{"\002gauss-firing.wav", "sounds2/d2x-xl", false, false, false},
	{"\002headlight.wav", "sounds2/d2x-xl", false, false, false},
	{"\002highping.wav", "sounds2/d2x-xl", false, false, false},
	{"\002lightning.wav", "sounds2/d2x-xl", false, false, false},
	{"\002lowping.wav", "sounds2/d2x-xl", false, false, false},
	{"\002missileflight-big.wav", "sounds2/d2x-xl", false, false, false},
	{"\002missileflight-small.wav", "sounds2/d2x-xl", false, false, false},
	{"\002slowdown.wav", "sounds2/d2x-xl", false, false, false},
	{"\002speedup.wav", "sounds2/d2x-xl", false, false, false},
	{"\002vulcan-firing.wav", "sounds2/d2x-xl", false, false, false},
	{"\002zoom1.wav", "sounds2/d2x-xl", false, false, false},
	{"\002zoom2.wav", "sounds2/d2x-xl", false, false, false},
	{"\002boiling-lava.wav", "sounds2/d2x-xl", false, false, false},
	{"\002busy-machine.wav", "sounds2/d2x-xl", false, false, false},
	{"\002computer.wav", "sounds2/d2x-xl", false, false, false},
	{"\002deep-hum.wav", "sounds2/d2x-xl", false, false, false},
	{"\002dripping-water.wav", "sounds2/d2x-xl", false, false, false},
	{"\002dripping-water-2.wav", "sounds2/d2x-xl", false, false, false},
	{"\002dripping-water-3.wav", "sounds2/d2x-xl", false, false, false},
	{"\002earthquake.wav", "sounds2/d2x-xl", false, false, false},
	{"\002energy.wav", "sounds2/d2x-xl", false, false, false},
	{"\002falling-rocks.wav", "sounds2/d2x-xl", false, false, false},
	{"\002falling-rocks-2.wav", "sounds2/d2x-xl", false, false, false},
	{"\002fast-fan.wav", "sounds2/d2x-xl", false, false, false},
	{"\002flowing-lava.wav", "sounds2/d2x-xl", false, false, false},
	{"\002flowing-water.wav", "sounds2/d2x-xl", false, false, false},
	{"\002insects.wav", "sounds2/d2x-xl", false, false, false},
	{"\002machine-gear.wav", "sounds2/d2x-xl", false, false, false},
	{"\002mighty-machine.wav", "sounds2/d2x-xl", false, false, false},
	{"\002static-buzz.wav", "sounds2/d2x-xl", false, false, false},
	{"\002steam.wav", "sounds2/d2x-xl", false, false, false},
	{"\002teleporter.wav", "sounds2/d2x-xl", false, false, false},
	{"\002waterfall.wav", "sounds2/d2x-xl", false, false, false},
	{"\002jet-engine.wav", "sounds2/d2x-xl", false, false, false}

};

// ----------------------------------------------------------------------------

void MoveFiles (char* pszDestFolder, char* pszSourceFolder, bool bMoveSubFolders)
{
	FFS	ffs;
	char	szSource [FILENAME_LEN], szDest [FILENAME_LEN];

sprintf (szSource, "%s*.*", pszSourceFolder);
if (!FFF (szSource, &ffs, 0)) {
	do {
		sprintf (szSource, "%s%s", pszSourceFolder, ffs.name);
		sprintf (szDest, "%s%s", pszDestFolder, ffs.name);
#ifdef _WIN32
		MoveFile (szSource, szDest);
#else
		CFile::Rename (szDest, szSource, "");
#endif
		} while (!FFN (&ffs, 0));
	}

if (bMoveSubFolders) {
	if (!FFF (pszSourceFolder, &ffs, 1)) {
		do {
			sprintf (szSource, "%s%s", pszSourceFolder, ffs.name);
			sprintf (szDest, "%s%s", pszDestFolder, ffs.name);
			CFile::MkDir (szDest);
			MoveFiles (szDest, szSource, true);
			} while (!FFN (&ffs, 1));
		}
	}
}

// ----------------------------------------------------------------------------

void MoveD2Textures (void)
{
	static char* szSubFolders [] = { "", "64/", "128/", "256/" };

	char szSourceFolder [FILENAME_LEN], szDestFolder [FILENAME_LEN];

for (int i = 0; i < 4; i++) {
	sprintf (szSourceFolder, "%stextures/%s", STATIC_DATA_FOLDER, szSubFolders [i]);
	sprintf (szDestFolder, "%s%s", gameFolders.szTextureCacheFolder [0], szSubFolders [i]);
	MoveFiles (szDestFolder, szSourceFolder, false);
	}
}

// ----------------------------------------------------------------------------

static bool CheckAndCopyWildcards (tFileDesc* fileDesc)
{
	FFS	ffs;
	int	i;
	char	szFilter [FILENAME_LEN], szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	CFile	cf;

// quit if none of the specified files exist in the source folder
if ((i = FFF (fileDesc->pszFile, &ffs, 0))) {
	sprintf (szFilter, "%s%s\\%s", gameFolders.szDataRootFolder [(int) fileDesc->bUser], fileDesc->pszFolder, fileDesc->pszFile);
	return FFF (szFilter, &ffs, 0) == 0;
	}
do {
	sprintf (szDest, "\002%s", ffs.name);
	sprintf (szFilter, "%s%s", gameFolders.szDataRootFolder [(int) fileDesc->bUser], fileDesc->pszFolder);
	if (!CFile::Exist (szDest, szFilter, 0)) {	// if the file doesn't exist in the destination folder copy it
		sprintf (szSrc, "%s%s", gameFolders.szDataRootFolder [(int) fileDesc->bUser], ffs.name);
		sprintf (szDest, "%s%s\\%s", gameFolders.szDataRootFolder [(int) fileDesc->bUser], fileDesc->pszFolder, ffs.name);
		cf.Copy (szSrc, szDest);
		}
	} while (!FFN (&ffs, 0));
return true;
}

// ----------------------------------------------------------------------------

static int CheckAndCopyFiles (tFileDesc* fileList, int nFiles)
{
	char	szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	int	nErrors = 0;
	CFile	cf;

for (int i = 0; i < nFiles; i++) {
	if (strstr (fileList [i].pszFile, "*") || strstr (fileList [i].pszFile, "?")) {
		fileList [i].bFound = CheckAndCopyWildcards (fileList + i);
		if (!(fileList [i].bFound || fileList [i].bOptional))
			nErrors++;
		}
	else {
		sprintf (szDest, "%s%s", gameFolders.szDataRootFolder [(int) fileList [i].bUser], fileList [i].pszFolder);
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, szDest, false) == 1;
		if (fileList [i].bFound)
			continue;	// file exists in the destination folder
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, gameFolders.szDataRootFolder [(int) fileList [i].bUser], false) == 1;
		if (fileList [i].bFound) {	// file exists in the source folder
			sprintf (szSrc, "%s%s", STATIC_DATA_FOLDER, fileList [i].pszFile + 1);
			sprintf (szDest, "%s%s\\%s", gameFolders.szDataRootFolder [(int) fileList [i].bUser], fileList [i].pszFile + 1);
			cf.Copy (szSrc, szDest);
			}
		else if (!fileList [i].bOptional)
			nErrors++;
		}
	}
return nErrors;
}

// ----------------------------------------------------------------------------

#if defined(_WIN32)

static void CheckAndCreateGameFolders (void)
{
static const char* gameSubFolders [] = {
	"cache",
	"config",
	"data",
#if defined(_WIN32)
	"downloads",
#endif
	"models",
	"mods",
	"movies",
	"profiles",
	"savegames",
	"screenshots",
	"sounds2",
	"sounds2/d2x-xl",
	"textures"
};

	FFS	ffs;
	char	szFolder [FILENAME_LEN];

for (int i = 0; i < int (sizeofa (gameSubFolders)); i++) {
	sprintf (szFolder, "%s%s", STATIC_DATA_FOLDER, gameSubFolders [i]);
	if (FFF (szFolder, &ffs, 1))
  		CFile::MkDir (szFolder);
	}
}

#endif

// ----------------------------------------------------------------------------

static void CreateFileListMessage (char* szMsg, tFileDesc* fileList, int nFiles, bool bShowFolders = false)
{
	bool	bFirst = true;
	int	l = 0, nListed = 0;

for (int i = 0, j = -1; i < nFiles; i++) {
	if (/*DBG ||*/ !(fileList [i].bFound || fileList [i].bOptional)) {
		if (bShowFolders && ((j < 0) || strcmp (fileList [i].pszFolder, fileList [j].pszFolder))) {
			j = i;
			if (!bFirst) {
				l = 0;
				strcat (szMsg, "\n\n");
				bFirst = true;
				}
			if (strcmp (gameFolders.szDataRootFolder [(int) fileList [i].bUser], ".\\")) {
				strcat (szMsg, gameFolders.szDataRootFolder [(int) fileList [i].bUser]);
				l += int (strlen (gameFolders.szDataRootFolder [(int) fileList [i].bUser]));
				}
			strcat (szMsg, fileList [i].pszFolder);
			strcat (szMsg, ": ");
			l += int (strlen (fileList [i].pszFolder)) + 2;
			}
		if (bFirst)
			bFirst = false;
		else {
			strcat (szMsg, ", ");
			l += 2;
#if 0
			if (l >= 160) {
				l = 0;
				strcat (szMsg, "\n\n");
				}
#endif
			}
		strcat (szMsg, fileList [i].pszFile + (fileList [i].pszFile [0] == '\002'));
		l += int (strlen (fileList [i].pszFile) + (fileList [i].pszFile [0] == '\002'));
		nListed++;
		}
	}
}

// ----------------------------------------------------------------------------

int CheckAndFixSetup (void)
{
if (!gameStates.app.bCheckAndFixSetup)
	return 0;

	int	nResult = 0;
	bool	bDemoData = false;
	char	szMsg [10000];

#if defined(_WIN32)
//CheckAndCreateGameFolders ();
#endif
MoveD2Textures ();

if (CheckAndCopyFiles (gameFilesD2, int (sizeofa (gameFilesD2)))) {
	if (CheckAndCopyFiles (demoFilesD2, int (sizeofa (demoFilesD2))))
		nResult |= 1;
	else
		bDemoData = true;
	}
if (!bDemoData) {
	if (CheckAndCopyFiles (gameFilesD1, int (sizeofa (gameFilesD1))))
		nResult |= 2;
	if (CheckAndCopyFiles (vertigoFiles, int (sizeofa (vertigoFiles))))
		nResult |= 4;
	}
if (CheckAndCopyFiles (addonFiles, int (sizeofa (addonFiles))))
	nResult |= 8;
if (CheckAndCopyFiles (addonTextureFiles, int (sizeofa (addonTextureFiles))))
	nResult |= 16;
if (CheckAndCopyFiles (addonSoundFiles, int (sizeofa (addonSoundFiles))))
	nResult |= 32;

#if 0 //DBG
nResult = 255;
#endif
if (nResult) {
	*szMsg = '\0';
	if (nResult & 1) {
		strcat (szMsg, "\n\nCritical - D2X-XL couldn't find the following Descent 2 files:\n\n");
		CreateFileListMessage (szMsg, gameFilesD2, int (sizeofa (gameFilesD2)));
		}
	if (nResult & 2) {
		strcat (szMsg, "\n\nWarning - D2X-XL couldn't find the following Descent 1 files:\n\n");
		CreateFileListMessage (szMsg, gameFilesD1, int (sizeofa (gameFilesD1)));
		}
	if (nResult & 4) {
		strcat (szMsg, "\n\nWarning - D2X-XL couldn't find the following Vertigo files:\n\n");
		CreateFileListMessage (szMsg, vertigoFiles, int (sizeofa (vertigoFiles)));
		}
	if (nResult & 8) {
		strcat (szMsg, "\n\nCritical - D2X-XL couldn't find the following D2X-XL files:\n\n");
		CreateFileListMessage (szMsg, addonFiles, int (sizeofa (addonFiles)), true);
		}
	if (nResult & 16) {
		strcat (szMsg, "\n\nWarning - D2X-XL couldn't find the following D2X-XL texture files:\n\n");
		CreateFileListMessage (szMsg, addonTextureFiles, int (sizeofa (addonTextureFiles)), true);
		}
	if (nResult & 32) {
		strcat (szMsg, "\n\nWarning - D2X-XL couldn't find the following D2X-XL sound files:\n\n");
		CreateFileListMessage (szMsg, addonSoundFiles, int (sizeofa (addonSoundFiles)), true);
		}
	if (nResult & (1 | 8)) {
		strcat (szMsg, "\n\nD2X-XL cannot run because files are missing.\n");
			strcat (szMsg, "\nPlease download the required files. Download locations are\n");
		if (nResult & 8)
			strcat (szMsg, " - http://www.descent2.de/d2x.html\n - http://www.sourceforge.net/projects/d2x-xl\n");
		if (nResult & 1)
			strcat (szMsg, " - http://www.gog.com (buy the game here for little money)\n");
#if 0
		if (InitGraphics (false)) {
			gameData.menu.helpColor = RGB_PAL (47, 47, 47);
			gameData.menu.colorOverride = gameData.menu.helpColor;
			MsgBox (TXT_ERROR, NULL, - 3, szMsg, " ", TXT_CLOSE);
			gameData.menu.colorOverride = 0;
			}
		else
#endif
			Error (szMsg);
		}
	else if ((FindArg ("-setup") || (gameConfig.nVersion != D2X_IVER)) && (nResult & (2 | 4 | 16))) {	// only warn once each time a new game version is installed
		strcat (szMsg, "\n\n");
		if (nResult & 2)
			strcat (szMsg, "Descent 1 missions will be unavailable.\n");
		if (nResult & 4)
			strcat (szMsg, "Vertigo missions will be unavailable.\n");
		if (nResult & 16)
			strcat (szMsg, "Additional image effects will be unavailable.\n");
		if (nResult & 32)
			strcat (szMsg, "Additional sound effects will be unavailable.\n");
#if 0
		if (InitGraphics (false)) {
			gameData.menu.helpColor = RGB_PAL (47, 47, 47);
			gameData.menu.colorOverride = gameData.menu.helpColor;
			MsgBox (TXT_WARNING, NULL, - 3, szMsg, " ", TXT_CLOSE);
			gameData.menu.colorOverride = 0;
			}
		else
#endif
			Warning (szMsg);
		}
	CFile	cf;
	if (cf.Open ("d2x-xl-missing-files.txt", gameFolders.szUserFolder, "rb", 0)) {
		fprintf (cf.File (), szMsg);
		cf.Close ();
		}
	}
#if 0 //DBG
else
	Warning ("No errors were found in your D2X-XL installation.");
#endif
return nResult;
}

#endif //defined (_WIN32) && !defined(_M_IA64) && !defined(_M_AMD64)

// ----------------------------------------------------------------------------
