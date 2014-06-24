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
	const char*	pszFolders [2];
	int bOptional;
	int bUser;
	int bFound;
} tFileDesc;

static tFileDesc gameFilesD2 [] = {
	// basic game files
	{"\002descent.cfg", {"config/", ""}, 1, -1, 0},
	{"\002alien1.pig", {"data/", ""}, 0, 0, 0},
	{"\002alien2.pig", {"data/", ""}, 0, 0, 0},
	{"\002fire.pig", {"data/", ""}, 0, 0, 0},
	{"\002groupa.pig", {"data/", ""}, 0, 0, 0},
	{"\002ice.pig", {"data/", ""}, 0, 0, 0},
	{"\002water.pig", {"data/", ""}, 0, 0, 0},
	{"\002descent2.hog", {"data/", ""}, 0, 0, 0},
	{"\002descent2.ham", {"data/", ""}, 0, 0, 0},
	{"\002descent2.s11", {"data/", ""}, 0, 0, 0},
	{"\002descent2.s22", {"data/", ""}, 0, 0, 0},
	{"\002intro-h.mvl", {"movies/", ""}, 1, 0, 0},
	{"\002intro-l.mvl", {"movies/", ""}, 1, 0, 0},
	{"\002other-h.mvl", {"movies/", ""}, 1, 0, 0},
	{"\002other-l.mvl", {"movies/", ""}, 1, 0, 0},
	{"\002robots-h.mvl", {"movies/", ""}, 1, 0, 0},
	{"\002robots-l.mvl", {"movies/", ""}, 1, 0, 0}
};

static tFileDesc demoFilesD2 [] = {
	// Descent 2 demo files
	{"d2demo.hog", {"data/", ""}, 0, 0, 0},
	{"d2demo.ham", {"data/", ""}, 0, 0, 0},
	{"d2demo.pig", {"data/", ""}, 0, 0, 0}
};

static tFileDesc gameFilesD1 [] = {
	// Descent 1 game files
	{"\002descent.pig", {"data/", ""}, 0, 0, 0},
	{"\002descent.hog", {"data/", ""}, 0, 0, 0}
};

static tFileDesc vertigoFiles [] = {
	// Vertigo expansion
	{"\002hoard.ham", {"data/", ""}, 0, 0, 0},
	{"\002d2x.hog", {"missions/", ""}, 0, 0, 0},
	{"\002d2x.mn2", {"missions/", ""}, 0, 0, 0},
	{"\002d2x-h.mvl", {"movies/", ""}, 1, 0, 0},
	{"\002d2x-l.mvl", {"movies/", ""}, 1, 0, 0},
};

static tFileDesc addonFiles [] = {
	// D2X-XL addon files
	{"\002d2x-default.ini", {"config/", ""}, 0, -1, 0},
	{"\002d2x.ini", {"config/", ""}, 1, -1, 0},

	{"\002d2x-xl.hog", {"data/", ""}, 0, 0, 0},
	{"\002exit.ham", {"data/", ""}, 1, 0, 0},

	{"*.plx", {"profiles/", ""}, 1, -1, 0},
	{"*.plr", {"profiles/", ""}, 1, -1, 0},

	{"\002bullet.ase", {"models/", ""}, 0, 0, 0},
	{"\002bullet.tga", {"models/", ""}, 0, 0, 0},

	{"*.sg?", {"savegames/", ""}, 1, 1, 0}
	};

static tFileDesc addonTextureFiles [] = {
	{"\002bullettime#0.tga", {"textures/d2/", ""}, 0, 0, 0},
	{"\002cockpit.tga", {"textures/d2/", ""}, 0, 0, 0},
	{"\002cockpitb.tga", {"textures/d2/", ""}, 0, 0, 0},
	{"\002monsterball.tga", {"textures/d2/", ""}, 0, 0, 0},
	{"\002slowmotion#0.tga", {"textures/d2/", ""}, 0, 0, 0},
	{"\002status.tga", {"textures/d2/", ""}, 0, 0, 0},
	{"\002statusb.tga", {"textures/d2/", ""}, 0, 0, 0},

	{"\002aimdmg.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002blast.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002blast-hard.tga", {"textures/d2x-xl/", ""}, 1, 0, 0},
	{"\002blast-medium.tga", {"textures/d2x-xl/", ""}, 1, 0, 0},
	{"\002blast-soft.tga", {"textures/d2x-xl/", ""}, 1, 0, 0},
	{"\002bubble.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002bullcase.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002corona.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002deadzone.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002drivedmg.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002fire.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002glare.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002gundmg.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002halfhalo.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002halo.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002joymouse.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002pwupicon.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002rboticon.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002scope.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002shield.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002smoke.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002smoke-hard.tga", {"textures/d2x-xl/", ""}, 1, 0, 0},
	{"\002smoke-medium.tga", {"textures/d2x-xl/", ""}, 1, 0, 0},
	{"\002smoke-soft.tga", {"textures/d2x-xl/", ""}, 1, 0, 0},
	{"\002sparks.tga", {"textures/d2x-xl/", ""}, 0, 0, 0},
	{"\002thruster.tga", {"textures/d2x-xl/", ""}, 0, 0, 0}
};

static tFileDesc addonSoundFiles [] = {
	{"\002afbr_1.wav", {"sounds/d2/44khz/", "sounds/d2/22khz/"}, 0, 0, 0},
	{"\002airbubbles.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002fire.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002gatling-slowdown.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002gatling-speedup.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002gauss-firing.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002headlight.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002highping.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002lightning.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002lowping.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002missileflight-big.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002missileflight-small.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002slowdown.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002speedup.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002vulcan-firing.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002zoom1.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002zoom2.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002boiling-lava.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002busy-machine.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002computer.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002deep-hum.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002dripping-water.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002dripping-water-2.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002dripping-water-3.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002earthquake.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002energy.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002falling-rocks.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002falling-rocks-2.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002fast-fan.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002flowing-lava.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002flowing-water.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002insects.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002machine-gear.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002mighty-machine.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002static-buzz.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002steam.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002teleporter.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002waterfall.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0},
	{"\002jet-engine.wav", {"sounds/d2x-xl/", ""}, 0, 0, 0}

};

static inline int User (tFileDesc& fd)
{
#ifdef _WIN32
return 0;
#else
return fd.bUser; 
#endif
}

// ----------------------------------------------------------------------------

void MoveFiles (const char* pszDestFolder, const char* pszSourceFolder, bool bMoveSubFolders)
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
	static const char* szSubFolders [] = { "64/", "128/", "256/" };

	char szSourceFolder [FILENAME_LEN], szDestFolder [FILENAME_LEN];

sprintf (szSourceFolder, "%stextures/", gameFolders.game.szRoot);
MoveFiles (gameFolders.game.szTextures [1], szSourceFolder, false);
for (int i = 0; i < 3; i++) {
	sprintf (szSourceFolder, "%stextures/%s", gameFolders.game.szRoot, szSubFolders [i]);
	sprintf (szDestFolder, "%s%s", gameFolders.var.szTextures [1], szSubFolders [i]);
	MoveFiles (szDestFolder, szSourceFolder, false);
	}
for (int i = 0; i < 3; i++) {
	sprintf (szSourceFolder, "%stextures/d2/%s", gameFolders.game.szRoot, szSubFolders [i]);
	sprintf (szDestFolder, "%s%s", gameFolders.var.szTextures [1], szSubFolders [i]);
	MoveFiles (szDestFolder, szSourceFolder, false);
	}
}

// ----------------------------------------------------------------------------

void MoveD2Sounds (void)
{
	static const char* szOldSoundFolders [] = { "sounds1/d1/", "sounds2/d1/", "sounds1/", "sounds2/", "sounds1/d2x-xl/", "sounds2/d2x-xl/"};
	static const char* pszNewSoundFolders [] = { gameFolders.game.szSounds [3], gameFolders.game.szSounds [3], gameFolders.game.szSounds [1], 
																gameFolders.game.szSounds [2], gameFolders.game.szSounds [4], gameFolders.game.szSounds [4] };

	char szSourceFolder [FILENAME_LEN];

for (int i = 0; i < 6; i++) {
	sprintf (szSourceFolder, "%s%s", gameFolders.game.szRoot, szOldSoundFolders [i]);
	MoveFiles (pszNewSoundFolders [i], szSourceFolder, false);
	}
}

// ----------------------------------------------------------------------------

static inline const char* GetFolder (tFileDesc* fileDesc)
{
int bUser = User (*fileDesc);
return (bUser < 0) ? gameFolders.user.szRoot : (bUser > 0) ? gameFolders.user.szCache : gameFolders.game.szRoot;
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
	for (int j = 0; j < 2; j++)
		if (*fileDesc->pszFolders [j]) {
			sprintf (szFilter, "%s%s%s", GetFolder (fileDesc), fileDesc->pszFolders [j], fileDesc->pszFile);
			if (!FFF (szFilter, &ffs, 0))
				return true;
			}
	return false;
	}
do {
	for (int j = 0; j < 2; j++)
		if (*fileDesc->pszFolders [j]) {
			sprintf (szDest, "\002%s", ffs.name);
			sprintf (szFilter, "%s%s", GetFolder (fileDesc), fileDesc->pszFolders [j]);
			if (!CFile::Exist (szDest, szFilter, 0)) {	// if the file doesn't exist in the destination folder copy it
				sprintf (szSrc, "%s%s", GetFolder (fileDesc), ffs.name);
				sprintf (szDest, "%s%s%s", GetFolder (fileDesc), fileDesc->pszFolders [j], ffs.name);
				cf.Copy (szSrc, szDest);
				}
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
		for (int j = 0; j < 2; j++)
			if (*fileList [i].pszFolders [j]) {
				sprintf (szDest, "%s%s", GetFolder (fileList + i), fileList [i].pszFolders [j]);
				fileList [i].bFound = CFile::Exist (fileList [i].pszFile, szDest, false) == 1;
				if (fileList [i].bFound)
					break;
				}
		if (fileList [i].bFound)
			continue;	// file exists in the destination folder
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, GetFolder (fileList + i), false) == 1;
		if (fileList [i].bFound) {	// file exists in the source folder
			sprintf (szSrc, "%s%s", gameFolders.game.szRoot, fileList [i].pszFile + 1);
			sprintf (szDest, "%s%s", GetFolder (fileList + i), fileList [i].pszFile + 1);
			cf.Copy (szSrc, szDest);
			}
		else if (!fileList [i].bOptional) {
			for (int j = 0; j < 2; j++)
				if (*fileList [i].pszFolders [j]) {
				sprintf (szDest, "%s%s", GetFolder (fileList + i), fileList [i].pszFolders [j]);
				fileList [i].bFound = CFile::Exist (fileList [i].pszFile, szDest, false) == 1;
				}
			nErrors++;
			}
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
	"sounds/d2",
	"sounds/d2x-xl",
	"textures"
};

	FFS	ffs;
	char	szFolder [FILENAME_LEN];

for (int i = 0; i < int (sizeofa (gameSubFolders)); i++) {
	sprintf (szFolder, "%s%s", gameFolders.game.szRoot, gameSubFolders [i]);
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
		for (int h = 0; h < 2; h++) {
			if (!*fileList [i].pszFolders [h])
				continue;
			if (bShowFolders && ((j < 0) || strcmp (fileList [i].pszFolders [h], fileList [j].pszFolders [h]))) {
				j = i;
				if (!bFirst) {
					l = 0;
					strcat (szMsg, "\n\n");
					bFirst = true;
					}
				if (strcmp (GetFolder (fileList + i), ".\\")) {
					strcat (szMsg, GetFolder (fileList + i));
					l += int (strlen (GetFolder (fileList + i)));
					}
				strcat (szMsg, fileList [i].pszFolders [h]);
				strcat (szMsg, ": ");
				l += int (strlen (fileList [i].pszFolders [h])) + 2;
				}
			if (bFirst)
				bFirst = false;
			else {
				strcat (szMsg, ", ");
				l += 2;
				}
			strcat (szMsg, fileList [i].pszFile + (fileList [i].pszFile [0] == '\002'));
			l += int (strlen (fileList [i].pszFile) + (fileList [i].pszFile [0] == '\002'));
			nListed++;
			}
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
MoveD2Sounds ();

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
		strcat (szMsg, "\n\nD2X-XL may not be able to run because files are missing.\n");
			strcat (szMsg, "\nPlease download the required files. Download locations are\n");
		if (nResult & 8)
			strcat (szMsg, " - http://www.descent2.de/d2x.html\n - http://www.sourceforge.net/projects/d2x-xl\n");
		if (nResult & 1)
			strcat (szMsg, " - http://www.gog.com (buy the game here for little money)\n");
#if 0
		if (InitGraphics (false)) {
			gameData.menu.helpColor = RGB_PAL (47, 47, 47);
			gameData.menu.colorOverride = gameData.menu.helpColor;
			InfoBox (TXT_ERROR, (pMenuCallback) NULL, BG_STANDARD, -3, szMsg, " ", TXT_CLOSE);
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
			InfoBox (TXT_WARNING, (pMenuCallback) NULL, BG_STANDARD, -3, szMsg, " ", TXT_CLOSE);
			gameData.menu.colorOverride = 0;
			}
		else
#endif
			Warning (szMsg);
		}
	CFile	cf;
	if (cf.Open ("d2x-xl-missing-files.txt", gameFolders.user.szCache, "rb", 0)) {
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
