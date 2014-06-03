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
#include "hogfile.h"
#include "menubackground.h"
#include "vers_id.h"

// ----------------------------------------------------------------------------

void MakeTexSubFolders (char* pszParentFolder)
{
if (*pszParentFolder) {
		static char szTexSubFolders [4][4] = {"256", "128", "64", "dxt"};

		char	szFolder [FILENAME_LEN];

	for (int i = 0; i < 4; i++) {
		sprintf (szFolder, "%s/%s", pszParentFolder, szTexSubFolders [i]);
		CFile::MkDir (szFolder);
		}
	}
}

// ----------------------------------------------------------------------------

#ifdef _WIN32
#	define	STD_GAMEDIR		""
#	define	D2X_APPNAME		"d2x-xl.exe"
#elif defined(__macosx__)
#	define	STD_GAMEDIR		"/Applications/Games/D2X-XL"
#	define	D2X_APPNAME		"d2x-xl"
#else
#	define	STD_GAMEDIR		"/usr/local/games/d2x-xl"
#	define	D2X_APPNAME		"d2x-xl"
#endif

#ifndef SHAREPATH
#	define SHAREPATH			""
#endif

#if defined(__macosx__)
#	define	DATADIR			"Data"
#	define	SHADERDIR		"Shaders"
#	define	MODELDIR			"Models"
#	define	SOUNDDIR			"Sounds"
#	define	SOUNDDIR1		"Sounds1"
#	define	SOUNDDIR2		"Sounds2"
#	define	SOUNDDIR1_D1	"Sounds1/D1"
#	define	SOUNDDIR2_D1	"Sounds2/D1"
#	define	SOUNDDIR_D2X	"Sounds2/d2x-xl"
#	define	CONFIGDIR		"Config"
#	define	PROFDIR			"Profiles"
#	define	SCRSHOTDIR		"Screenshots"
#	define	MOVIEDIR			"Movies"
#	define	SAVEDIR			"Savegames"
#	define	DEMODIR			"Demos"
#	define	TEXTUREDIR		"Textures"
#	define	TEXTUREDIR_D2	"Textures"
#	define	TEXTUREDIR_D1	"Textures/D1"
#	define	CACHEDIR			"Cache"
#	define	LIGHTMAPDIR		"Cache/Lightmaps"
#	define	MODDIR			"Mods"
#	define	MUSICDIR			"Music"
#	define	DOWNLOADDIR		"Downloads"
#	define	WALLPAPERDIR	"Wallpapers"
#else
#	define	DATADIR			"data"
#	define	SHADERDIR		"shaders"
#	define	MODELDIR			"models"
#	define	SOUNDDIR			"sounds"
#	define	SOUNDDIR1		"sounds1"
#	define	SOUNDDIR2		"sounds2"
#	define	SOUNDDIR1_D1	"sounds1/D1"
#	define	SOUNDDIR2_D1	"sounds2/D1"
#	define	SOUNDDIR_D2X	"sounds2/d2x-xl"
#	define	CONFIGDIR		"config"
#	define	PROFDIR			"profiles"
#	define	SCRSHOTDIR		"screenshots"
#	define	MOVIEDIR			"movies"
#	define	SAVEDIR			"savegames"
#	define	DEMODIR			"demos"
#	define	TEXTUREDIR		"textures"
#	define	TEXTUREDIR_D2	"textures"
#	define	TEXTUREDIR_D1	"textures/d1"
#	if DBG
#		define	CACHEDIR			"cache/debug"
#		define	LIGHTMAPDIR		"cache/debug/lightmaps"
#	else
#		define	CACHEDIR			"cache"
#		define	LIGHTMAPDIR		"cache/lightmaps"
#	endif
#	define	MODDIR			"mods"
#	define	MUSICDIR			"music"
#	define	DOWNLOADDIR		"downloads"
#	define	WALLPAPERDIR	"wallpapers"
#endif

void GetAppFolders (void)
{
	int	i;
	char	szDataRootDir [FILENAME_LEN];
	char	*psz;
#ifdef _WIN32
	char	coord;
#endif

#if 0 //DBG && defined (WIN32)
strcpy (gameFolders.szHomeDir, "d:\\programs\\d2\\");
strcpy (gameFolders.szGameDir, "d:\\programs\\d2\\");
strcpy (gameFolders.szDataDir [0], "d:\\programs\\d2\\");
strcpy (szDataRootDir, "d:\\programs\\d2\\");
#else
*gameFolders.szHomeDir =
*gameFolders.szGameDir =
*gameFolders.szDataDir [0] =
*szDataRootDir = '\0';
#endif
if ((i = FindArg ("-userdir")) && appConfig [i + 1] && *appConfig [i + 1]) {
#	ifdef _WIN32
	sprintf (gameFolders.szGameDir, "%s\\%s\\", appConfig [i + 1], DATADIR);
#	else
	sprintf (gameFolders.szGameDir, "%s/%s/", appConfig [i + 1], DATADIR);
#	endif
	if (GetAppFolder ("", gameFolders.szGameDir, gameFolders.szGameDir, "*.hog"))
		*gameFolders.szGameDir = '\0';
	else {
		strcpy (gameFolders.szGameDir, appConfig [i + 1]);
		int j = (int) strlen (gameFolders.szGameDir);
		if (j && (gameFolders.szGameDir [j-1] != '\\') && (gameFolders.szGameDir [j-1] != '/'))
			strcat (gameFolders.szGameDir, "/");
		}
	}
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, getenv ("DESCENT2"), D2X_APPNAME))
	*gameFolders.szGameDir = '\0';
#ifdef _WIN32
if (!*gameFolders.szGameDir) {
	psz = appConfig [1];
	for (int j = (int) strlen (psz); j; ) {
		coord = psz [--j];
		if ((coord == '\\') || (coord == '/')) {
			memcpy (gameFolders.szGameDir, psz, ++j);
			gameFolders.szGameDir [j] = '\0';
			break;
			}
		}
	}
for (i = 0; gameFolders.szGameDir [i]; i++)
	if (gameFolders.szGameDir [i] == '\\')
		gameFolders.szGameDir [i] = '/';
strcpy (szDataRootDir, gameFolders.szGameDir);
strcpy (gameFolders.szHomeDir, *gameFolders.szGameDir ? gameFolders.szGameDir : szDataRootDir);
#else // Linux, OS X
#	if defined(__unix__) || defined(__macosx__) || defined(__FreeBSD__)
if (getenv ("HOME"))
	strcpy (gameFolders.szHomeDir, getenv ("HOME"));
#		if 0
if (!*gameFolders.szGameDir && *gameFolders.szHomeDir && GetAppFolder (gameFolders.szHomeDir, gameFolders.szGameDir, "d2x-xl", "d2x-xl"))
	*gameFolders.szGameDir = '\0';
#		endif
#	endif //__unix__
*gameFolders.szSharePath = '\0';
if (*gameFolders.szSharePath) {
	if (strstr (SHAREPATH, "games"))
		sprintf (gameFolders.szSharePath, "%s/d2x-xl", SHAREPATH);
	else
		sprintf (gameFolders.szSharePath, "%s/games/d2x-xl", SHAREPATH);
	if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, gameFolders.szSharePath, "")) {
		*gameFolders.szGameDir = 
		*gameFolders.szSharePath = '\0';
		}
	}
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, STD_GAMEDIR, ""))
	*gameFolders.szGameDir = '\0';
#	ifdef __macosx__
GetOSXAppFolder (szDataRootDir, gameFolders.szGameDir);
#	else
strcpy (szDataRootDir, gameFolders.szGameDir);
#	endif //__macosx__
int j = (int) strlen (gameFolders.szGameDir);
if (j && (gameFolders.szGameDir [j-1] != '\\') && (gameFolders.szGameDir [j-1] != '/'))
	strcat (gameFolders.szGameDir, "/");
if (*gameFolders.szGameDir)
	chdir (gameFolders.szGameDir);
#endif //Linux, OS X
if ((i = FindArg ("-hogdir")) && !GetAppFolder ("", gameFolders.szDataDir [0], appConfig [i + 1], "descent2.hog"))
	strcpy (szDataRootDir, appConfig [i + 1]);
else {
	sprintf (gameFolders.szDataDir [0], "%s%s", gameFolders.szGameDir, DATADIR);
	if (GetAppFolder ("", gameFolders.szDataDir [0], gameFolders.szDataDir [0], "descent2.hog") &&
		 GetAppFolder ("", gameFolders.szDataDir [0], gameFolders.szDataDir [0], "d2demo.hog") &&
		 GetAppFolder (szDataRootDir, gameFolders.szDataDir [0], DATADIR, "descent2.hog") &&
		 GetAppFolder (szDataRootDir, gameFolders.szDataDir [0], DATADIR, "d2demo.hog"))
	Error (TXT_NO_HOG2);
	}
psz = strstr (gameFolders.szDataDir [0], DATADIR);
if (psz && !*(psz + 4)) {
	if (psz == gameFolders.szDataDir [0])
		sprintf (gameFolders.szDataDir [0], "%s%s", gameFolders.szGameDir, DATADIR);
	else {
		*(psz - 1) = '\0';
		strcpy (szDataRootDir, gameFolders.szDataDir [0]);
		*(psz - 1) = '/';
		}
	}
else
	strcpy (szDataRootDir, gameFolders.szDataDir [0]);
/*---*/PrintLog (0, "expected game app folder = '%s'\n", gameFolders.szGameDir);
/*---*/PrintLog (0, "expected game data folder = '%s'\n", gameFolders.szDataDir [0]);
if (GetAppFolder (szDataRootDir, gameFolders.szModelDir [0], MODELDIR, "*.ase"))
	GetAppFolder (szDataRootDir, gameFolders.szModelDir [0], MODELDIR, "*.oof");
GetAppFolder (szDataRootDir, gameFolders.szSoundDir [0], SOUNDDIR1, "*.wav");
GetAppFolder (szDataRootDir, gameFolders.szSoundDir [1], SOUNDDIR2, "*.wav");
GetAppFolder (szDataRootDir, gameFolders.szSoundDir [6], SOUNDDIR_D2X, "*.wav");
if (GetAppFolder (szDataRootDir, gameFolders.szSoundDir [2], SOUNDDIR1_D1, "*.wav"))
	*gameFolders.szSoundDir [2] = '\0';
if (GetAppFolder (szDataRootDir, gameFolders.szSoundDir [3], SOUNDDIR2_D1, "*.wav"))
	*gameFolders.szSoundDir [3] = '\0';
GetAppFolder (szDataRootDir, gameFolders.szShaderDir, SHADERDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szTextureDir [0], TEXTUREDIR_D2, "*.tga");
GetAppFolder (szDataRootDir, gameFolders.szTextureDir [1], TEXTUREDIR_D1, "*.tga");
GetAppFolder (szDataRootDir, gameFolders.szModDir [0], MODDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szMovieDir, MOVIEDIR, "*.mvl");
#ifdef __unix__
if (*gameFolders.szHomeDir) {
		char	fn [FILENAME_LEN];

	sprintf (szDataRootDir, "%s/.d2x-xl", gameFolders.szHomeDir);
	CFile::MkDir (szDataRootDir);
	sprintf (fn, "%s/%s", szDataRootDir, PROFDIR);
	CFile::MkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, SAVEDIR);
	CFile::MkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, SCRSHOTDIR);
	CFile::MkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, DEMODIR);
	CFile::MkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, CONFIGDIR);
	CFile::MkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, CONFIGDIR);
	CFile::MkDir (fn);
	sprintf (fn, "%s/%s", szDataRootDir, DOWNLOADDIR);
	CFile::MkDir (fn);
	}
#endif
if (*gameFolders.szHomeDir) {
#ifdef __macosx__
	char *pszOSXCacheDir = GetMacOSXCacheFolder ();
	sprintf (gameFolders.szCacheDir, "%s/%s", pszOSXCacheDir, DOWNLOADDIR);
	CFile::MkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szTextureCacheDir [0], "%s/%s",pszOSXCacheDir, TEXTUREDIR_D2);
	CFile::MkDir (gameFolders.szTextureCacheDir [0]);
	sprintf (gameFolders.szTextureCacheDir [1], "%s/%s", pszOSXCacheDir, TEXTUREDIR_D1);
	CFile::MkDir (gameFolders.szTextureCacheDir [1]);
	sprintf (gameFolders.szModelCacheDir [0], "%s/%s", pszOSXCacheDir, MODELDIR);
	CFile::MkDir (gameFolders.szModelCacheDir [0]);
	sprintf (gameFolders.szCacheDir, "%s/%s", pszOSXCacheDir, CACHEDIR);
	CFile::MkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szLightmapDir, "%s/%s", pszOSXCacheDir, LIGHTMAPDIR);
	CFile::MkDir (gameFolders.szLightmapDir);
	sprintf (gameFolders.szCacheDir, "%s/%s/256", pszOSXCacheDir, CACHEDIR);
	CFile::MkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s/128", pszOSXCacheDir, CACHEDIR);
	CFile::MkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s/64", pszOSXCacheDir, CACHEDIR);
	CFile::MkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szCacheDir, "%s/%s", pszOSXCacheDir, CACHEDIR);
	CFile::MkDir (gameFolders.szCacheDir);
#else
#	ifdef __unix__
	sprintf (szDataRootDir, "%s/.d2x-xl", gameFolders.szHomeDir);
#	else
	strcpy (szDataRootDir, gameFolders.szHomeDir);
	i = int (strlen (szDataRootDir)) - 1;
	if ((szDataRootDir [i] == '\\') || (szDataRootDir [i] == '/'))
		szDataRootDir [i] = '\0';
#	endif // __unix__
	CFile::MkDir (szDataRootDir);
	sprintf (gameFolders.szTextureCacheDir [0], "%s/%s", szDataRootDir, TEXTUREDIR_D2);
	CFile::MkDir (gameFolders.szTextureCacheDir [0]);
	sprintf (gameFolders.szTextureCacheDir [1], "%s/%s", szDataRootDir, TEXTUREDIR_D1);
	CFile::MkDir (gameFolders.szTextureCacheDir [1]);
	sprintf (gameFolders.szModelCacheDir [0], "%s/%s", szDataRootDir, MODELDIR);
	CFile::MkDir (gameFolders.szModelCacheDir [0]);
	sprintf (gameFolders.szCacheDir, "%s/%s", szDataRootDir, CACHEDIR);
	CFile::MkDir (gameFolders.szCacheDir);
	sprintf (gameFolders.szLightmapDir, "%s/%s", szDataRootDir, LIGHTMAPDIR);
	CFile::MkDir (gameFolders.szLightmapDir);
	sprintf (gameFolders.szDownloadDir, "%s/%s", szDataRootDir, DOWNLOADDIR);
	CFile::MkDir (gameFolders.szDownloadDir);
#endif // __macosx__
	}
GetAppFolder (szDataRootDir, gameFolders.szProfDir, PROFDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szSaveDir, SAVEDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szScrShotDir, SCRSHOTDIR, "");
GetAppFolder (szDataRootDir, gameFolders.szDemoDir, DEMODIR, "");
if (GetAppFolder (szDataRootDir, gameFolders.szConfigDir, CONFIGDIR, "*.ini"))
	strcpy (gameFolders.szConfigDir, gameFolders.szGameDir);
#ifdef __unix__
GetAppFolder (szDataRootDir, gameFolders.szWallpaperDir [0], WALLPAPERDIR, "");
#else
GetAppFolder (gameFolders.szTextureDir [0], gameFolders.szWallpaperDir [0], WALLPAPERDIR, "");
#endif
#ifdef _WIN32
sprintf (gameFolders.szMissionDir, "%s%s", gameFolders.szGameDir, BASE_MISSION_DIR);
#else
sprintf (gameFolders.szMissionDir, "%s/%s", gameFolders.szGameDir, BASE_MISSION_DIR);
#endif
//if (i = FindArg ("-hogdir"))
//	CFUseAltHogDir (appArgs [i + 1]);
for (i = 0; i < 2; i++)
	MakeTexSubFolders (gameFolders.szTextureCacheDir [i]);
MakeTexSubFolders (gameFolders.szModelCacheDir [0]);
sprintf (gameFolders.szMissionDownloadDir, "%s/%s", gameFolders.szMissionDir, DOWNLOADDIR);
sprintf (gameFolders.szDataDir [1], "%s/d2x-xl", gameFolders.szDataDir [0]);
CFile::MkDir (gameFolders.szMissionDownloadDir);
}

// ----------------------------------------------------------------------------

void ResetModFolders (void)
{
gameStates.app.bHaveMod = 0;
*gameFolders.szModName =
*gameFolders.szMusicDir =
*gameFolders.szSoundDir [4] =
*gameFolders.szSoundDir [5] =
*gameFolders.szModDir [1] =
*gameFolders.szTextureDir [2] =
*gameFolders.szTextureCacheDir [2] =
*gameFolders.szTextureDir [3] =
*gameFolders.szTextureCacheDir [3] =
*gameFolders.szModelDir [1] =
*gameFolders.szModelCacheDir [1] =
*gameFolders.szModelDir [2] =
*gameFolders.szModelCacheDir [2] = 
*gameFolders.szWallpaperDir [1] = '\0';
sprintf (gameOpts->menus.altBg.szName [1], "default.tga");
}

// ----------------------------------------------------------------------------

void MakeModFolders (const char* pszMission, int nLevel)
{

	static int nLoadingScreen = -1;
	static int nShuffledLevels [24];

int bDefault, bBuiltIn;

ResetModFolders ();
if (gameStates.app.bDemoData)
	return;

if ((bDefault = (*pszMission == '\0')))
	pszMission = missionManager [missionManager.nCurrentMission].szMissionName;
else
	CFile::SplitPath (pszMission, NULL, gameFolders.szModName, NULL);

#if 1
if ((bBuiltIn = missionManager.IsBuiltIn (pszMission)))
	strcpy (gameFolders.szModName, (bBuiltIn == 1) ? "descent" : "descent2");
#else
if ((bBuiltIn = (strstr (pszMission, "Descent: First Strike") != NULL) ? 1 : 0))
	strcpy (gameFolders.szModName, "descent");
else if ((bBuiltIn = (strstr (pszMission, "Descent 2: Counterstrike!") != NULL) ? 2 : 0))
	strcpy (gameFolders.szModName, "descent2");
else if ((bBuiltIn = (strstr (pszMission, "d2x.hog") != NULL) ? 3 : 0))
	strcpy (gameFolders.szModName, "descent2");
#endif
else if (bDefault)
	return;

if (bBuiltIn && !gameOpts->app.bEnableMods)
	return;

if (GetAppFolder (gameFolders.szModDir [0], gameFolders.szModDir [1], gameFolders.szModName, "")) 
	*gameFolders.szModName = '\0';
else {
	sprintf (gameFolders.szSoundDir [4], "%s/%s", gameFolders.szModDir [1], SOUNDDIR);
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szTextureDir [2], TEXTUREDIR, "*.tga"))
		*gameFolders.szTextureDir [2] = '\0';
	else {
		sprintf (gameFolders.szTextureCacheDir [2], "%s/%s", gameFolders.szModDir [1], TEXTUREDIR);
		//gameOpts->render.textures.bUseHires [0] = 1;
		}
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szModelDir [1], MODELDIR, "*.ase") &&
		 GetAppFolder (gameFolders.szModDir [1], gameFolders.szModelDir [1], MODELDIR, "*.oof"))
		*gameFolders.szModelDir [1] = '\0';
	else {
		sprintf (gameFolders.szModelDir [1], "%s/%s", gameFolders.szModDir [1], MODELDIR);
		sprintf (gameFolders.szModelCacheDir [1], "%s/%s", gameFolders.szModDir [1], MODELDIR);
		}
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szWallpaperDir [1], WALLPAPERDIR, "*.tga")) {
		*gameFolders.szWallpaperDir [1] = '\0';
		*gameOpts->menus.altBg.szName [1] = '\0';
		}
	else {
		sprintf (gameFolders.szWallpaperDir [1], "%s/%s", gameFolders.szModDir [1], WALLPAPERDIR);
		if (nLevel < 0)
			sprintf (gameOpts->menus.altBg.szName [1], "slevel%02d.tga", -nLevel);
		else if (nLevel > 0) {
			// chose a random custom loading screen for missions other than D2:CS that do not have their own custom loading screens
			if ((bBuiltIn == 2) && (missionManager.IsBuiltIn (hogFileManager.MissionName ()) != 2)) {
				if (nLoadingScreen < 0) { // create a random offset the first time this function is called and use it later on
					int i;
					for (i = 0; i < 24; i++)
						nShuffledLevels [i] = i + 1;
					gameStates.app.SRand ();
					for (i = 0; i < 23; i++) {
						int h = 23 - i;
						int j = h ? rand () % h : 0;
						Swap (nShuffledLevels [i], nShuffledLevels [i + j]);
						}
					}
				nLoadingScreen = (nLoadingScreen + 1) % 24;
				nLevel = nShuffledLevels [nLoadingScreen];
				}
			sprintf (gameOpts->menus.altBg.szName [1], "level%02d.tga", nLevel);
			}
		else 
			sprintf (gameOpts->menus.altBg.szName [1], "default.tga");
		backgroundManager.Rebuild ();
		}
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szMusicDir, MUSICDIR, "*.ogg"))
		*gameFolders.szMusicDir = '\0';
	MakeTexSubFolders (gameFolders.szTextureCacheDir [2]);
	MakeTexSubFolders (gameFolders.szModelCacheDir [1]);
	gameStates.app.bHaveMod = 1;
	}
}

// ----------------------------------------------------------------------------
