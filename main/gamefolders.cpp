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

#ifdef _WIN32
#	define	DEFAULT_GAME_FOLDER		""
#	define	D2X_APPNAME					"d2x-xl.exe"
#elif defined(__macosx__)
#	define	DEFAULT_GAME_FOLDER		"/Applications/Games/D2X-XL"
#	define	D2X_APPNAME					"d2x-xl"
#else
#	define	DEFAULT_GAME_FOLDER		"/usr/local/games/d2x-xl"
#	define	D2X_APPNAME					"d2x-xl"
#endif

#ifndef SHAREPATH
#	define SHAREPATH						""
#endif

#ifdef __macosx__

#	define	DATA_FOLDER					"Data"
#	define	SHADER_FOLDER				"Shaders"
#	define	MODEL_FOLDER				"Models"
#	define	SOUND_FOLDER				"Sounds"
#	define	SOUND_FOLDER1				"Sounds1"
#	define	SOUND_FOLDER2				"Sounds2"
#	define	SOUND_FOLDER1_D1			"Sounds1/D1"
#	define	SOUND_FOLDER2_D1			"Sounds2/D1"
#	define	SOUND_FOLDER_D2X			"Sounds2/d2x-xl"
#	define	CONFIG_FOLDER				"Config"
#	define	PROF_FOLDER					"Profiles"
#	define	SCRSHOT_FOLDER				"Screenshots"
#	define	MOVIE_FOLDER				"Movies"
#	define	SAVE_FOLDER					"Savegames"
#	define	DEMO_FOLDER					"Demos"
#	define	TEXTURE_FOLDER				"Textures"
#	define	TEXTURE_FOLDER_D2			"D2"
#	define	TEXTURE_FOLDER_D1			"D1"
#	define	WALLPAPER_FOLDER			"Wallpapers"
#	define	CACHE_FOLDER				"Cache"
#	define	SHARED_CACHE_FOLDER			"d2x-xl"
#	define	USER_CACHE_FOLDER			".d2x-xl"
#	define	LIGHTMAP_FOLDER			"Lightmaps"
#	define	LIGHTDATA_FOLDER			"Lights"
#	define	MESH_FOLDER					"Meshes"
#	define	MISSIONSTATE_FOLDER		"States"
#	define	MOD_FOLDER					"Mods"
#	define	MUSIC_FOLDER				"Music"
#	define	DOWNLOAD_FOLDER			"Downloads"

#else

#	define	DATA_FOLDER					"data"
#	define	SHADER_FOLDER				"shaders"
#	define	MODEL_FOLDER				"models"
#	define	SOUND_FOLDER				"sounds"
#	define	SOUND_FOLDER_D1			"d1"
#	define	SOUND_FOLDER_D2			"d2"
#	define	SOUND_FOLDER_22KHZ		"22khz"
#	define	SOUND_FOLDER_44KHZ		"44khz"
#	define	SOUND_FOLDER_D2X			"d2x-xl"
#	define	CONFIG_FOLDER				"config"
#	define	PROF_FOLDER					"profiles"
#	define	SCRSHOT_FOLDER				"screenshots"
#	define	MOVIE_FOLDER				"movies"
#	define	SAVE_FOLDER					"savegames"
#	define	DEMO_FOLDER					"demos"
#	define	TEXTURE_FOLDER				"textures"
#	define	TEXTURE_FOLDER_D2			"d2"
#	define	TEXTURE_FOLDER_D1			"d1"
#	define	WALLPAPER_FOLDER			"wallpapers"
#	define	MOD_FOLDER					"mods"
#	define	MUSIC_FOLDER				"music"
#	define	DOWNLOAD_FOLDER			"downloads"
#	define	LIGHTMAP_FOLDER			"lightmaps"
#	define	LIGHTDATA_FOLDER			"lights"
#	define	MESH_FOLDER					"meshes"
#	define	MISSIONSTATE_FOLDER		"states"

#	ifdef _WIN32

#		if DBG
#			define	SHARED_CACHE_FOLDER		"cache/debug"
#		else
#			define	SHARED_CACHE_FOLDER		"cache"
#		endif
#	define USER_CACHE_FOLDER	SHARED_CACHE_FOLDER

#	else

#		define	SHARED_CACHE_FOLDER			"d2x-xl"
#		define	USER_CACHE_FOLDER				".d2x-xl"

#	endif

#endif

// ----------------------------------------------------------------------------

char* CheckFolder (char* pszAppFolder, const char* pszFolder, const char* pszFile, bool bFolder = true)
{
if (pszFolder && *pszFolder) {
	char szFolder [FILENAME_LEN];
	if (bFolder) {
		strcpy (szFolder, pszFolder);
		AppendSlash (szFolder);
		pszFolder = const_cast<char*> (szFolder);
		}
	CFile::SplitPath (pszFolder, pszAppFolder, NULL, NULL);
	FlipBackslash (pszAppFolder);
	if (GetAppFolder ("", pszAppFolder, pszAppFolder, pszFile))
		*pszAppFolder = '\0';
	else
		AppendSlash (pszAppFolder);
	}
return pszAppFolder;
}

// ----------------------------------------------------------------------------

int CheckDataFolder (char* pszDataRootDir)
{
AppendSlash (FlipBackslash (pszDataRootDir));
return GetAppFolder ("", gameFolders.game.szData [0], pszDataRootDir, "descent2.hog") &&
		 GetAppFolder ("", gameFolders.game.szData [0], pszDataRootDir, "d2demo.hog") &&
		 GetAppFolder (pszDataRootDir, gameFolders.game.szData [0], DATA_FOLDER, "descent2.hog") &&
		 GetAppFolder (pszDataRootDir, gameFolders.game.szData [0], DATA_FOLDER, "d2demo.hog");
}

// ----------------------------------------------------------------------------

int MakeFolder (char* pszAppFolder, const char* pszFolder = "", const char* pszSubFolder = "")
{
if (pszSubFolder && *pszSubFolder) {
#if 0
	if (!GetAppFolder (pszFolder, pszAppFolder, pszSubFolder, "")) {
		AppendSlash (pszAppFolder);
		return 1;
		}
#endif
	if (pszFolder && *pszFolder) {
		char szFolder [FILENAME_LEN];
		strcpy (szFolder, pszFolder);
		AppendSlash (szFolder);
		sprintf (pszAppFolder, "%s%s", szFolder, pszSubFolder);
		}
	}
AppendSlash (pszAppFolder);

FFS	ffs;

PrintLog (0, "looking for folder '%s' ...", pszAppFolder);
if (!FFF (pszAppFolder, &ffs, 1)) {
	PrintLog (0, "found\n");
	return 1;
	}

PrintLog (0, "failed\n");
PrintLog (0, "trying to create folder '%s' ...", pszAppFolder);
pszAppFolder [strlen (pszAppFolder) - 1] = '\0'; // remove trailing slash
int nResult = CFile::MkDir (pszAppFolder) > -1;
if (nResult)
	PrintLog (0, "worked\n");
else
	PrintLog (0, "failed\n");
AppendSlash (pszAppFolder);
return nResult;
}

// ----------------------------------------------------------------------------

int MakeTexSubFolders (char* pszParentFolder)
{
if (!*pszParentFolder)
	return 0;

	static char szTexSubFolders [4][4] = {"256", "128", "64", "dxt"};

	char	szFolder [FILENAME_LEN];

for (int i = 0; i < 4; i++) {
	sprintf (szFolder, "%s%s", pszParentFolder, szTexSubFolders [i]);
	if (!MakeFolder (szFolder))
		return 0;
	}
return 1;
}

// ----------------------------------------------------------------------------

int MakeGameFolders (void)
{
if (GetAppFolder (gameFolders.game.szRoot, "", "", "")) {
	Error ("Game data could not be found");
	return 0;
	}
if (GetAppFolder (gameFolders.game.szRoot, gameFolders.missions.szRoot, BASE_MISSION_FOLDER, "")) {
	Error ("Missions could not be found");
	return 0;
	}

MakeFolder (gameFolders.game.szData [0], gameFolders.game.szRoot, DATA_FOLDER);
MakeFolder (gameFolders.game.szData [1], gameFolders.game.szData [0], "d2x-xl");

MakeFolder (gameFolders.mods.szRoot, gameFolders.game.szRoot, MOD_FOLDER);
MakeFolder (gameFolders.game.szShaders, gameFolders.game.szRoot, SHADER_FOLDER);

if (!GetAppFolder (gameFolders.game.szRoot, gameFolders.game.szTextures [0], TEXTURE_FOLDER, "")) {
	if (GetAppFolder (gameFolders.game.szTextures [0], gameFolders.game.szTextures [1], TEXTURE_FOLDER_D2, ""))
		MakeFolder (gameFolders.game.szTextures [0], gameFolders.game.szTextures [0], TEXTURE_FOLDER_D2); // older D2X-XL installations may have a different D2 texture folder
	GetAppFolder (gameFolders.game.szTextures [0], gameFolders.game.szTextures [2], TEXTURE_FOLDER_D1, "*.tga");
	}

GetAppFolder (gameFolders.game.szRoot, gameFolders.game.szMovies, MOVIE_FOLDER, "*.mvl");

if (GetAppFolder (gameFolders.game.szRoot, gameFolders.game.szModels, MODEL_FOLDER, "*.ase"))
	GetAppFolder (gameFolders.game.szRoot, gameFolders.game.szModels, MODEL_FOLDER, "*.oof");

MakeFolder (gameFolders.game.szSounds [0], gameFolders.game.szRoot, SOUND_FOLDER);
MakeFolder (gameFolders.game.szSounds [3], gameFolders.game.szSounds [0], SOUND_FOLDER_D2); // temp usage
MakeFolder (gameFolders.game.szSounds [1], gameFolders.game.szSounds [3], SOUND_FOLDER_22KHZ);
MakeFolder (gameFolders.game.szSounds [2], gameFolders.game.szSounds [3], SOUND_FOLDER_44KHZ);
MakeFolder (gameFolders.game.szSounds [3], gameFolders.game.szSounds [0], SOUND_FOLDER_D1);
MakeFolder (gameFolders.game.szSounds [4], gameFolders.game.szSounds [0], SOUND_FOLDER_D2X);
return 1;
}

// ----------------------------------------------------------------------------

int MakeSharedFolders (void)
{
return
	MakeFolder (gameFolders.var.szModels [0], gameFolders.var.szCache, MODEL_FOLDER) &&
	MakeFolder (gameFolders.var.szTextures [0], gameFolders.var.szCache, TEXTURE_FOLDER) &&
	MakeFolder (gameFolders.var.szTextures [1], gameFolders.var.szTextures [0], TEXTURE_FOLDER_D2) &&
	MakeFolder (gameFolders.var.szTextures [2], gameFolders.var.szTextures [0], TEXTURE_FOLDER_D1) &&
	MakeFolder (gameFolders.var.szDownloads, gameFolders.var.szCache, DOWNLOAD_FOLDER) &&
	MakeFolder (gameFolders.var.szLightmaps, gameFolders.var.szCache, LIGHTMAP_FOLDER) &&
	MakeFolder (gameFolders.var.szLightData, gameFolders.var.szCache, LIGHTDATA_FOLDER) &&
	MakeFolder (gameFolders.var.szMeshes, gameFolders.var.szCache, MESH_FOLDER) &&
	MakeFolder (gameFolders.var.szMods, gameFolders.var.szCache, MOD_FOLDER) &&
	MakeTexSubFolders (gameFolders.var.szTextures [1]) &&
	MakeTexSubFolders (gameFolders.var.szTextures [2]) &&
	MakeTexSubFolders (gameFolders.var.szModels [0]);
}

// ----------------------------------------------------------------------------

int MakeUserFolders (void)
{
#ifdef _WIN32
#	define PRIVATE_DATA_FOLDER	gameFolders.user.szRoot
#else
#	define PRIVATE_DATA_FOLDER	gameFolders.user.szCache
#endif

MakeFolder (gameFolders.user.szConfig, PRIVATE_DATA_FOLDER, CONFIG_FOLDER);
MakeFolder (gameFolders.user.szProfiles, PRIVATE_DATA_FOLDER, PROF_FOLDER);
MakeFolder (gameFolders.user.szSavegames, PRIVATE_DATA_FOLDER, SAVE_FOLDER);
MakeFolder (gameFolders.user.szScreenshots, PRIVATE_DATA_FOLDER, SCRSHOT_FOLDER);
MakeFolder (gameFolders.user.szDemos, PRIVATE_DATA_FOLDER, DEMO_FOLDER);

MakeFolder (gameFolders.missions.szCache, gameFolders.user.szCache, MISSION_FOLDER);
MakeFolder (gameFolders.missions.szStates, gameFolders.missions.szCache, MISSIONSTATE_FOLDER);
MakeFolder (gameFolders.missions.szDownloads, gameFolders.missions.szCache, DOWNLOAD_FOLDER);

#ifdef _WIN32
MakeFolder (gameFolders.user.szWallpapers, strcmp (gameFolders.game.szRoot, gameFolders.user.szRoot) ? gameFolders.user.szCache : gameFolders.game.szTextures [0], WALLPAPER_FOLDER);
#else
MakeFolder (gameFolders.user.szWallpapers, gameFolders.user.szCache, WALLPAPER_FOLDER);
#endif
return 1;
}

// ----------------------------------------------------------------------------

void GetAppFolders (bool bInit)
{
if (bInit)
	memset (&gameFolders, 0, sizeof (gameFolders));
*gameFolders.user.szRoot =
*gameFolders.game.szRoot =
*gameFolders.game.szData [0] =
*gameFolders.game.szData [1] =
*gameFolders.var.szCache =
*gameFolders.user.szCache = '\0';

#ifdef _WIN32

if (!*CheckFolder (gameFolders.game.szRoot, appConfig.Text ("-datadir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.game.szRoot, appConfig.Text ("-gamedir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.game.szRoot, getenv ("DESCENT2"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.game.szRoot, appConfig [1], D2X_APPNAME, false))
	CheckFolder (gameFolders.game.szRoot, DEFAULT_GAME_FOLDER, "");

int nUserFolderMode = !*CheckFolder (gameFolders.user.szRoot, appConfig.Text ("-userdir"), "");

if (nUserFolderMode)
	strcpy (gameFolders.user.szRoot, gameFolders.game.szRoot);

int nSharedFolderMode = !*CheckFolder (gameFolders.var.szRoot, appConfig.Text ("-cachedir"), "");

if (nSharedFolderMode)
	*gameFolders.var.szRoot = '\0';

#else // Linux, OS X

*gameFolders.szSharePath = '\0';
if (*SHAREPATH) {
	if (strstr (SHAREPATH, "games"))
		sprintf (gameFolders.szSharePath, "%s/d2x-xl", SHAREPATH);
	else
		sprintf (gameFolders.szSharePath, "%s/games/d2x-xl", SHAREPATH);
	}

if (!*CheckFolder (gameFolders.game.szRoot, appConfig.Text ("-datadir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.game.szRoot, appConfig.Text ("-gamedir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.game.szRoot, gameFolders.szSharePath, D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.game.szRoot, getenv ("DESCENT2"), D2X_APPNAME))
#ifdef __macosx__
	GetOSXAppFolder (gameFolders.game.szRoot, gameFolders.game.szRoot);
#else
	CheckFolder (gameFolders.game.szRoot, DEFAULT_GAME_FOLDER, "");
#endif

#ifdef __macosx__
GetOSXAppFolder (gameFolders.game.szRoot, gameFolders.game.szRoot);
#endif

int nUserFolderMode = !*CheckFolder (gameFolders.user.szRoot, appConfig.Text ("-userdir"), "");
if (nUserFolderMode && !*CheckFolder (gameFolders.user.szRoot, getenv ("HOME"), ""))
	*gameFolders.user.szRoot = '\0';

#ifdef __linux__
#	define SHARED_ROOT_FOLDER	"/var/cache"
#else
#	define SHARED_ROOT_FOLDER	"~/Library/Caches"
#endif

int nSharedFolderMode = !*CheckFolder (gameFolders.var.szRoot, appConfig.Text ("-cachedir"), "");
if (nSharedFolderMode && !*CheckFolder (gameFolders.var.szRoot, SHARED_ROOT_FOLDER, ""))
	*gameFolders.var.szRoot = '\0';

#endif

if (CheckDataFolder (gameFolders.game.szRoot))
	Error (TXT_NO_HOG2);

/*---*/PrintLog (0, "expected game app folder = '%s'\n", gameFolders.game.szRoot);
/*---*/PrintLog (0, "expected game data folder = '%s'\n", gameFolders.game.szData [0]);

#ifndef _WIN32
if (*gameFolders.game.szRoot)
	chdir (gameFolders.game.szRoot);
#endif

if (!*gameFolders.user.szRoot)
	strcpy (gameFolders.user.szRoot, *gameFolders.var.szRoot ? gameFolders.var.szRoot : gameFolders.game.szRoot);
if (nUserFolderMode)
	MakeFolder (gameFolders.user.szCache, gameFolders.user.szRoot, USER_CACHE_FOLDER);
else
	strcpy (gameFolders.user.szCache, gameFolders.user.szRoot);

if (!*gameFolders.var.szRoot) {
	strcpy (gameFolders.var.szRoot, *gameFolders.user.szRoot ? gameFolders.user.szRoot : gameFolders.game.szRoot);
	strcpy (gameFolders.var.szCache, *gameFolders.user.szCache ? gameFolders.user.szCache : gameFolders.game.szRoot);
	nSharedFolderMode = -1;
	}

#ifdef __macosx__

strcpy (gameFolders.var.szRoot, GetMacOSXCacheFolder ());
MakeFolder (gameFolders.var.szCache, gameFolders.var.szRoot, CACHE_FOLDER);
strcpy (gameFolders.user.szCache, gameFolders.var.szCache);

#else

if (!nSharedFolderMode)
	strcpy (gameFolders.var.szCache, gameFolders.var.szRoot);
else if ((nSharedFolderMode > 0) && !MakeFolder (gameFolders.var.szCache, gameFolders.var.szRoot, SHARED_CACHE_FOLDER)) {
	strcpy (gameFolders.var.szRoot, gameFolders.user.szRoot);	 // fall back
	strcpy (gameFolders.var.szCache, gameFolders.user.szCache);
	}

#endif // __macosx__

if (!MakeGameFolders ())
	return;

if (!MakeSharedFolders () && strcmp (gameFolders.var.szCache, gameFolders.user.szCache)) {
	strcpy (gameFolders.var.szCache, gameFolders.user.szCache);
	MakeSharedFolders ();
	}
MakeUserFolders ();
}

// ----------------------------------------------------------------------------

void ResetModFolders (void)
{
gameStates.app.bHaveMod = 0;
*gameFolders.mods.szName =
*gameFolders.mods.szCache =
*gameFolders.game.szMusic =
*gameFolders.mods.szSounds [0] =
*gameFolders.mods.szSounds [1] =
*gameFolders.mods.szCurrent =
*gameFolders.mods.szTextures [0] =
*gameFolders.mods.szTextures [1] =
*gameFolders.var.szTextures [3] =
*gameFolders.var.szTextures [4] =
*gameFolders.mods.szModels [0] =
*gameFolders.var.szModels [1] =
*gameFolders.var.szModels [2] = 
*gameFolders.mods.szWallpapers = '\0';
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
	CFile::SplitPath (pszMission, NULL, gameFolders.mods.szName, NULL);

#if 1
if ((bBuiltIn = missionManager.IsBuiltIn (pszMission)))
	strcpy (gameFolders.mods.szName, (bBuiltIn == 1) ? "descent" : "descent2");
#else
if ((bBuiltIn = (strstr (pszMission, "Descent: First Strike") != NULL) ? 1 : 0))
	strcpy (gameFolders.mods.szName, "descent");
else if ((bBuiltIn = (strstr (pszMission, "Descent 2: Counterstrike!") != NULL) ? 2 : 0))
	strcpy (gameFolders.mods.szName, "descent2");
else if ((bBuiltIn = (strstr (pszMission, "d2x.hog") != NULL) ? 3 : 0))
	strcpy (gameFolders.mods.szName, "descent2");
#endif
else if (bDefault)
	return;

if (bBuiltIn && !gameOpts->app.bEnableMods)
	return;

if (GetAppFolder (gameFolders.mods.szRoot, gameFolders.mods.szCurrent, gameFolders.mods.szName, "")) 
	*gameFolders.mods.szName = '\0';
else {
	sprintf (gameFolders.mods.szCache, "%s%s/", gameFolders.var.szMods, gameFolders.mods.szName);	// e.g. /var/cache/d2x-xl/mods/mymod/
	MakeFolder (gameFolders.mods.szCache);
	sprintf (gameFolders.mods.szSounds [0], "%s%s/", gameFolders.mods.szCurrent, SOUND_FOLDER);
	if (GetAppFolder (gameFolders.mods.szCurrent, gameFolders.mods.szTextures [0], TEXTURE_FOLDER, ""))
		*gameFolders.mods.szTextures [0] = '\0';
	else {
		sprintf (gameFolders.var.szTextures [3], "%s%s/", gameFolders.mods.szCache, TEXTURE_FOLDER);	// e.g. /var/cache/d2x-xl/mods/mymod/textures/
		MakeFolder (gameFolders.var.szTextures [3]);
		//gameOpts->render.textures.bUseHires [0] = 1;
		}
#if 0
	if (GetAppFolder (gameFolders.mods.szCurrent, gameFolders.mods.szModels [0], MODEL_FOLDER, "*.ase") &&
		 GetAppFolder (gameFolders.mods.szCurrent, gameFolders.mods.szModels [0], MODEL_FOLDER, "*.oof"))
#else
	if (GetAppFolder (gameFolders.mods.szCurrent, gameFolders.mods.szModels [0], MODEL_FOLDER, "*"))
#endif
		*gameFolders.mods.szModels [0] = '\0';
	else {
		sprintf (gameFolders.var.szModels [1], "%s%s/", gameFolders.mods.szCache, MODEL_FOLDER);
		MakeFolder (gameFolders.var.szModels [1]);
		}
	if (GetAppFolder (gameFolders.mods.szCurrent, gameFolders.mods.szWallpapers, WALLPAPER_FOLDER, "*.tga")) {
		*gameFolders.mods.szWallpapers = '\0';
		*gameOpts->menus.altBg.szName [1] = '\0';
		}
	else {
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
	if (GetAppFolder (gameFolders.mods.szCurrent, gameFolders.game.szMusic, MUSIC_FOLDER, "*.ogg"))
		*gameFolders.game.szMusic = '\0';
	MakeTexSubFolders (gameFolders.var.szTextures [3]);
	MakeTexSubFolders (gameFolders.var.szModels [1]);
	gameStates.app.bHaveMod = 1;
	}
}

// ----------------------------------------------------------------------------
