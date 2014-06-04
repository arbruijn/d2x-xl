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

#if defined(__macosx__)

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
#	define	TEXTURE_FOLDER_D2			"Textures/D2"
#	define	TEXTURE_FOLDER_D1			"Textures/D1"
#	define	WALLPAPER_FOLDER			"Textures/Wallpapers"
#	define	TEXTURE_FOLDER_XL			"Textures/D2X-XL"
#	define	CACHE_FOLDER				"Cache"
#	define	LIGHTMAP_FOLDER			"Lightmaps"
#	define	LIGHTDATA_FOLDER			"Lights"
#	define	MESH_FOLDER					"Meshes"
#	define	MISSIONSTATE_FOLDER		"Missions"
#	define	MOD_FOLDER					"Mods"
#	define	MUSIC_FOLDER				"Music"
#	define	DOWNLOAD_FOLDER			"Downloads"

#else

#	define	DATA_FOLDER					"data"
#	define	SHADER_FOLDER				"shaders"
#	define	MODEL_FOLDER				"models"
#	define	SOUND_FOLDER				"sounds"
#	define	SOUND_FOLDER1				"sounds1"
#	define	SOUND_FOLDER2				"sounds2"
#	define	SOUND_FOLDER1_D1			"sounds1/d1"
#	define	SOUND_FOLDER2_D1			"sounds2/d1"
#	define	SOUND_FOLDER_D2X			"sounds2/d2x-xl"
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
#	define	MISSIONSTATE_FOLDER		"missions"

#	ifdef _WIN32
#		if DBG
#			define	CACHE_FOLDER		"cache/debug"
#		else
#			define	CACHE_FOLDER		"cache"
#		endif

#	else

#		define	CACHE_FOLDER			"d2x-xl"

#	endif

#endif

// ----------------------------------------------------------------------------

char* CheckFolder (char* pszAppFolder, char* pszFolder, char* pszFile, bool bFolder = true)
{
if (pszFolder && *pszFolder) {
	if (bFolder)
		AppendSlash (pszFolder);
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
return GetAppFolder ("", gameFolders.szDataFolder [0], pszDataRootDir, "descent2.hog") &&
		 GetAppFolder ("", gameFolders.szDataFolder [0], pszDataRootDir, "d2demo.hog") &&
		 GetAppFolder (pszDataRootDir, gameFolders.szDataFolder [0], DATA_FOLDER, "descent2.hog") &&
		 GetAppFolder (pszDataRootDir, gameFolders.szDataFolder [0], DATA_FOLDER, "d2demo.hog");
}

// ----------------------------------------------------------------------------

void MakeFolder (char* pszAppFolder, char* pszFolder = "", char* pszSubFolder = "", char* format = "%s/%s")
{
if (pszSubFolder && *pszSubFolder) {
	if (!GetAppFolder (pszFolder, pszAppFolder, pszSubFolder, "")) {
		AppendSlash (pszAppFolder);
		return;
		}
	if (pszFolder && *pszFolder)
		sprintf (pszAppFolder, format, pszFolder, pszSubFolder);
	}
else {
	FFS	ffs;
	if (!FFF (pszAppFolder, &ffs, 1))
		return;
	}
CFile::MkDir (pszAppFolder);
AppendSlash (pszAppFolder);
}

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

void GetAppFolders (void)
{
	int	i;

*gameFolders.szUserFolder =
*gameFolders.szGameFolder =
*gameFolders.szDataFolder [0] =
*gameFolders.szDataRootFolder [0] = 
*gameFolders.szDataRootFolder [1] = '\0';

#ifdef _WIN32
if (!*CheckFolder (gameFolders.szGameFolder, appConfig.Text ("-gamedir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, getenv ("DESCENT2"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, appConfig [1], D2X_APPNAME, false))
	CheckFolder (gameFolders.szGameFolder, DEFAULT_GAME_FOLDER, "");

CheckFolder (gameFolders.szUserFolder, appConfig.Text ("-userdir"), "");

CheckFolder (gameFolders.szDataRootFolder [2], appConfig.Text ("-cachedir"), "");

#else // Linux, OS X

*gameFolders.szSharePath = '\0';
if (*SHAREPATH) {
	if (strstr (SHAREPATH, "games"))
		sprintf (gameFolders.szSharePath, "%s/d2x-xl", SHAREPATH);
	else
		sprintf (gameFolders.szSharePath, "%s/games/d2x-xl", SHAREPATH);

if (!*CheckFolder (gameFolders.szGameFolder, appConfig.Text ("-gamedir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szameDir, gameFolders.szSharePath, D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, getenv ("DESCENT2"), D2X_APPNAME))
	CheckFolder (gameFolders.szGameFolder, DEFAULT_GAME_FOLDER, "");

if (!*CheckFolder (gameFolders.szUserFolder, appConfig.Text ("-userdir"), "") &&
	!*CheckFolder (gameFolders.szUserFolder, getenv ("HOME"), ""))
	strcpy (gameFolders.szUserFolder, gameFolders.szGameFolder);

if (!*CheckFolder (gameFolders.szDataFolder [2], appConfig.Text ("-cachedir"), "") &&
	!*CheckFolder (gameFolders.szDataFolder [2], "/var/cache", ""))
	strcpy (gameFolders.szDataFolder [2], gameFolders.szUserFolder);

if (!*CheckFolder (gameFolders.szDataFolder [0], appConfig.Text ("-datadir"), D2X_APPNAME))
	strcpy (gameFolders.szDataFolder [0], gameFolders.szGameFolder);

if (*gameFolders.szGameFolder)
	chdir (gameFolders.szGameFolder);

#endif

strcpy (gameFolders.szDataRootFolder [0], appConfig.Text ("-datadir"));
if (CheckDataFolder (gameFolders.szDataRootFolder [0])) {
#ifdef __macosx__
	GetOSXAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szGameFolder);
#else
	strcpy (gameFolders.szDataRootFolder [0], gameFolders.szGameFolder);
#endif //__macosx__
	if (CheckDataFolder (gameFolders.szDataRootFolder [0]))
		Error (TXT_NO_HOG2);
	}

/*---*/PrintLog (0, "expected game app folder = '%s'\n", gameFolders.szGameFolder);
/*---*/PrintLog (0, "expected game data folder = '%s'\n", gameFolders.szDataFolder [0]);
if (GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szModelFolder [0], MODEL_FOLDER, "*.ase"))
	GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szModelFolder [0], MODEL_FOLDER, "*.oof");
GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szSoundFolder [0], SOUND_FOLDER1, "*.wav");
GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szSoundFolder [1], SOUND_FOLDER2, "*.wav");
GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szSoundFolder [6], SOUND_FOLDER_D2X, "*.wav");
if (GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szSoundFolder [2], SOUND_FOLDER1_D1, "*.wav"))
	*gameFolders.szSoundFolder [2] = '\0';
if (GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szSoundFolder [3], SOUND_FOLDER2_D1, "*.wav"))
	*gameFolders.szSoundFolder [3] = '\0';
//GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szShaderFolder, SHADER_FOLDER, "");
GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szTextureRootFolder, TEXTURE_FOLDER, "");
if (GetAppFolder (gameFolders.szTextureRootFolder, gameFolders.szTextureFolder [0], TEXTURE_FOLDER_D2, "*.tga") &&
	 GetAppFolder (gameFolders.szTextureRootFolder, gameFolders.szTextureFolder [0], TEXTURE_FOLDER_D2, ""))
	MakeFolder (gameFolders.szTextureFolder [0], gameFolders.szTextureRootFolder, TEXTURE_FOLDER_D2); // older D2X-XL installations may have a different D2 texture folder
GetAppFolder (gameFolders.szTextureRootFolder, gameFolders.szTextureFolder [1], TEXTURE_FOLDER_D1, "*.tga");
GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szModFolder [0], MOD_FOLDER, "");
GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szMovieFolder, MOVIE_FOLDER, "*.mvl");

if (!*gameFolders.szUserFolder)
	strcpy (gameFolders.szUserFolder, gameFolders.szDataRootFolder [0]);
strcpy (gameFolders.szDataRootFolder [1], gameFolders.szUserFolder);
if (!*gameFolders.szDataRootFolder [2])
	strcpy (gameFolders.szDataRootFolder [2], gameFolders.szDataRootFolder [0]);

// create the user folders

MakeFolder (gameFolders.szDataRootFolder [1]);
MakeFolder (gameFolders.szCacheFolder [1], gameFolders.szDataRootFolder [1], CACHE_FOLDER);
MakeFolder (gameFolders.szProfileFolder, gameFolders.szDataRootFolder [1], PROF_FOLDER);
MakeFolder (gameFolders.szSavegameFolder, gameFolders.szDataRootFolder [1], SAVE_FOLDER);
MakeFolder (gameFolders.szScreenshotFolder, gameFolders.szDataRootFolder [1], SCRSHOT_FOLDER);
MakeFolder (gameFolders.szDemoFolder, gameFolders.szDataRootFolder [1], DEMO_FOLDER);
MakeFolder (gameFolders.szConfigFolder, gameFolders.szDataRootFolder [1], CONFIG_FOLDER);
MakeFolder (gameFolders.szDownloadFolder, gameFolders.szCacheFolder [1], DOWNLOAD_FOLDER);
MakeFolder (gameFolders.szMissionStateFolder, gameFolders.szCacheFolder [1], MISSIONSTATE_FOLDER);

// create the shared folders

#ifdef __macosx__

char *pszOSXCacheDir = GetMacOSXCacheFolder ();
MakeFolder (gameFolders.szCacheFolder [0], pszOSXCacheDir, DOWNLOAD_FOLDER);
strcpy (gameFolders.szCacheFolder [1], gameFolders.szCacheFolder [0]);
MakeFolder (gameFolders.szTextureCacheFolder [0], pszOSXCacheDir, TEXTURE_FOLDER_D2);
MakeFolder (gameFolders.szTextureCacheFolder [1], pszOSXCacheDir, TEXTURE_FOLDER_D1);
MakeFolder (gameFolders.szModelCacheFolder [0], pszOSXCacheDir, MODEL_FOLDER);
MakeFolder (gameFolders.szCacheFolder [0], pszOSXCacheDir, CACHE_FOLDER);
MakeFolder (gameFolders.szLightmapFolder, pszOSXCacheDir, LIGHTMAP_FOLDER);
MakeFolder (gameFolders.szLightDataFolder, pszOSXCacheDir, LIGHTDATA_FOLDER);
MakeFolder (gameFolders.szMeshFolder, pszOSXCacheDir, MESH_FOLDER);
MakeFolder (gameFolders.szMissionStateFolder, pszOSXCacheDir, MISSIONSTATE_FOLDER);
MakeFolder (gameFolders.szCacheFolder [0], pszOSXCacheDir, CACHE_FOLDER, "%s/%s/256");
MakeFolder (gameFolders.szCacheFolder [0], pszOSXCacheDir, CACHE_FOLDER, "%s/%s/128");
MakeFolder (gameFolders.szCacheFolder [0], pszOSXCacheDir, CACHE_FOLDER, "%s/%s/64");
MakeFolder (gameFolders.szCacheFolder [0], pszOSXCacheDir, CACHE_FOLDER);

#else

MakeFolder (gameFolders.szDataRootFolder [0]);
MakeFolder (gameFolders.szDataFolder [0], gameFolders.szDataRootFolder [0], DATA_FOLDER);
MakeFolder (gameFolders.szDataFolder [1], gameFolders.szDataFolder [0], "d2x-xl");
MakeFolder (gameFolders.szCacheFolder [0], gameFolders.szDataRootFolder [2], CACHE_FOLDER);
MakeFolder (gameFolders.szWallpaperFolder [0], gameFolders.szTextureRootFolder, WALLPAPER_FOLDER);
MakeFolder (gameFolders.szTextureCacheFolder [0], gameFolders.szTextureRootFolder, TEXTURE_FOLDER_D2);
MakeFolder (gameFolders.szTextureCacheFolder [1], gameFolders.szTextureRootFolder, TEXTURE_FOLDER_D1);
MakeFolder (gameFolders.szModelCacheFolder [0], gameFolders.szDataRootFolder [0], MODEL_FOLDER);
MakeFolder (gameFolders.szLightmapFolder, gameFolders.szCacheFolder [0], LIGHTMAP_FOLDER);
MakeFolder (gameFolders.szLightDataFolder, gameFolders.szCacheFolder [0], LIGHTDATA_FOLDER);
MakeFolder (gameFolders.szMeshFolder, gameFolders.szCacheFolder [0], MESH_FOLDER);

#endif // __macosx__

for (i = 0; i < 2; i++)
	MakeTexSubFolders (gameFolders.szTextureCacheFolder [i]);
MakeTexSubFolders (gameFolders.szModelCacheFolder [0]);

if (GetAppFolder (gameFolders.szDataRootFolder [1], gameFolders.szConfigFolder, CONFIG_FOLDER, "*.ini"))
	strcpy (gameFolders.szConfigFolder, gameFolders.szGameFolder);

if (GetAppFolder (gameFolders.szDataRootFolder [0], gameFolders.szMissionFolder, BASE_MISSION_FOLDER, ""))
	GetAppFolder (gameFolders.szGameFolder, gameFolders.szMissionFolder, BASE_MISSION_FOLDER, "");
MakeFolder (gameFolders.szMissionDownloadFolder, gameFolders.szMissionFolder, DOWNLOAD_FOLDER);
}

// ----------------------------------------------------------------------------

void ResetModFolders (void)
{
gameStates.app.bHaveMod = 0;
*gameFolders.szModName =
*gameFolders.szMusicFolder =
*gameFolders.szSoundFolder [4] =
*gameFolders.szSoundFolder [5] =
*gameFolders.szModFolder [1] =
*gameFolders.szTextureFolder [2] =
*gameFolders.szTextureCacheFolder [2] =
*gameFolders.szTextureFolder [3] =
*gameFolders.szTextureCacheFolder [3] =
*gameFolders.szModelFolder [1] =
*gameFolders.szModelCacheFolder [1] =
*gameFolders.szModelFolder [2] =
*gameFolders.szModelCacheFolder [2] = 
*gameFolders.szWallpaperFolder [1] = '\0';
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

if (GetAppFolder (gameFolders.szModFolder [0], gameFolders.szModFolder [1], gameFolders.szModName, "")) 
	*gameFolders.szModName = '\0';
else {
	sprintf (gameFolders.szSoundFolder [4], "%s/%s", gameFolders.szModFolder [1], SOUND_FOLDER);
	if (GetAppFolder (gameFolders.szModFolder [1], gameFolders.szTextureFolder [2], TEXTURE_FOLDER, "*.tga"))
		*gameFolders.szTextureFolder [2] = '\0';
	else {
		sprintf (gameFolders.szTextureCacheFolder [2], "%s/%s", gameFolders.szModFolder [1], TEXTURE_FOLDER);
		//gameOpts->render.textures.bUseHires [0] = 1;
		}
	if (GetAppFolder (gameFolders.szModFolder [1], gameFolders.szModelFolder [1], MODEL_FOLDER, "*.ase") &&
		 GetAppFolder (gameFolders.szModFolder [1], gameFolders.szModelFolder [1], MODEL_FOLDER, "*.oof"))
		*gameFolders.szModelFolder [1] = '\0';
	else {
		sprintf (gameFolders.szModelFolder [1], "%s/%s", gameFolders.szModFolder [1], MODEL_FOLDER);
		sprintf (gameFolders.szModelCacheFolder [1], "%s/%s", gameFolders.szModFolder [1], MODEL_FOLDER);
		}
	if (GetAppFolder (gameFolders.szModFolder [1], gameFolders.szWallpaperFolder [1], WALLPAPER_FOLDER, "*.tga")) {
		*gameFolders.szWallpaperFolder [1] = '\0';
		*gameOpts->menus.altBg.szName [1] = '\0';
		}
	else {
		sprintf (gameFolders.szWallpaperFolder [1], "%s/%s", gameFolders.szModFolder [1], WALLPAPER_FOLDER);
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
	if (GetAppFolder (gameFolders.szModFolder [1], gameFolders.szMusicFolder, MUSIC_FOLDER, "*.ogg"))
		*gameFolders.szMusicFolder = '\0';
	MakeTexSubFolders (gameFolders.szTextureCacheFolder [2]);
	MakeTexSubFolders (gameFolders.szModelCacheFolder [1]);
	gameStates.app.bHaveMod = 1;
	}
}

// ----------------------------------------------------------------------------
