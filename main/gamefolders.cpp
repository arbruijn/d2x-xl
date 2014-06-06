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
	char szFolder [FILENAME_LEN];
	if (bFolder) {
		strcpy (szFolder, pszFolder);
		AppendSlash (pszFolder  = szFolder);
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
return GetAppFolder ("", gameFolders.szDataFolder [0], pszDataRootDir, "descent2.hog") &&
		 GetAppFolder ("", gameFolders.szDataFolder [0], pszDataRootDir, "d2demo.hog") &&
		 GetAppFolder (pszDataRootDir, gameFolders.szDataFolder [0], DATA_FOLDER, "descent2.hog") &&
		 GetAppFolder (pszDataRootDir, gameFolders.szDataFolder [0], DATA_FOLDER, "d2demo.hog");
}

// ----------------------------------------------------------------------------

int MakeFolder (char* pszAppFolder, char* pszFolder = "", char* pszSubFolder = "", char* format = "%s/%s")
{
if (pszSubFolder && *pszSubFolder) {
	if (!GetAppFolder (pszFolder, pszAppFolder, pszSubFolder, "")) {
		AppendSlash (pszAppFolder);
		return 1;
		}
	if (pszFolder && *pszFolder)
		sprintf (pszAppFolder, format, pszFolder, pszSubFolder);
	}
else {
	FFS	ffs;
	if (!FFF (pszAppFolder, &ffs, 1))
		return 1;
	}
if (CFile::MkDir (pszAppFolder))
	return 0;
AppendSlash (pszAppFolder);
return 1;
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

void GetAppFolders (bool bInit)
{
if (bInit)
	memset (&gameFolders, 0, sizeof (gameFolders));
*gameFolders.szUserFolder =
*gameFolders.szGameFolder =
*gameFolders.szDataFolder [0] =
*STATIC_DATA_FOLDER = 
*VAR_DATA_FOLDER = 
*PRIVATE_DATA_FOLDER = '\0';

#ifdef _WIN32
if (!*CheckFolder (gameFolders.szGameFolder, appConfig.Text ("-gamedir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, getenv ("DESCENT2"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, appConfig [1], D2X_APPNAME, false))
	CheckFolder (gameFolders.szGameFolder, DEFAULT_GAME_FOLDER, "");

CheckFolder (gameFolders.szUserFolder, appConfig.Text ("-userdir"), "");

CheckFolder (VAR_DATA_FOLDER, appConfig.Text ("-cachedir"), "");

#else // Linux, OS X

*gameFolders.szSharePath = '\0';
if (*SHAREPATH) {
	if (strstr (SHAREPATH, "games"))
		sprintf (gameFolders.szSharePath, "%s/d2x-xl", SHAREPATH);
	else
		sprintf (gameFolders.szSharePath, "%s/games/d2x-xl", SHAREPATH);
	}

if (!*CheckFolder (gameFolders.szGameFolder, appConfig.Text ("-gamedir"), D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, gameFolders.szSharePath, D2X_APPNAME) &&
	 !*CheckFolder (gameFolders.szGameFolder, getenv ("DESCENT2"), D2X_APPNAME))
	CheckFolder (gameFolders.szGameFolder, DEFAULT_GAME_FOLDER, "");

if (!*CheckFolder (gameFolders.szUserFolder, appConfig.Text ("-userdir"), "") &&
	!*CheckFolder (gameFolders.szUserFolder, getenv ("HOME"), ""))
	strcpy (gameFolders.szUserFolder, gameFolders.szGameFolder);

if (!*CheckFolder (VAR_DATA_FOLDER, appConfig.Text ("-cachedir"), "") &&
	!*CheckFolder (VAR_DATA_FOLDER, "/var/cache", ""))
	strcpy (VAR_DATA_FOLDER, gameFolders.szUserFolder);

if (!*CheckFolder (STATIC_DATA_FOLDER, appConfig.Text ("-datadir"), D2X_APPNAME))
	strcpy (STATIC_DATA_FOLDER, gameFolders.szGameFolder);

if (*gameFolders.szGameFolder)
	chdir (gameFolders.szGameFolder);

#endif

strcpy (STATIC_DATA_FOLDER, appConfig.Text ("-datadir"));
if (CheckDataFolder (STATIC_DATA_FOLDER)) {
#ifdef __macosx__
	GetOSXAppFolder (STATIC_DATA_FOLDER, gameFolders.szGameFolder);
#else
	strcpy (STATIC_DATA_FOLDER, gameFolders.szGameFolder);
#endif //__macosx__
	if (CheckDataFolder (STATIC_DATA_FOLDER))
		Error (TXT_NO_HOG2);
	}

/*---*/PrintLog (0, "expected game app folder = '%s'\n", gameFolders.szGameFolder);
/*---*/PrintLog (0, "expected game data folder = '%s'\n", gameFolders.szDataFolder [0]);

#ifdef _WIN32

GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szTextureFolder [0], TEXTURE_FOLDER, "");
if (GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szModelFolder [0], MODEL_FOLDER, "*.ase"))
	GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szModelFolder [0], MODEL_FOLDER, "*.oof");
GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szModRootFolder, MOD_FOLDER, "");

#else

GetAppFolder (VAR_DATA_FOLDER, gameFolders.szTextureFolder [0], TEXTURE_FOLDER, "");
if (GetAppFolder (VAR_DATA_FOLDER, gameFolders.szModelFolder [0], MODEL_FOLDER, "*.ase"))
	GetAppFolder (VAR_DATA_FOLDER, gameFolders.szModelFolder [0], MODEL_FOLDER, "*.oof");
GetAppFolder (VAR_DATA_FOLDER, gameFolders.szModRootFolder, MOD_FOLDER, "");

#endif

GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szSoundFolder [0], SOUND_FOLDER1, "*.wav");
GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szSoundFolder [1], SOUND_FOLDER2, "*.wav");
GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szSoundFolder [6], SOUND_FOLDER_D2X, "*.wav");
if (GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szSoundFolder [2], SOUND_FOLDER1_D1, "*.wav"))
	*gameFolders.szSoundFolder [2] = '\0';
if (GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szSoundFolder [3], SOUND_FOLDER2_D1, "*.wav"))
	*gameFolders.szSoundFolder [3] = '\0';
if (GetAppFolder (gameFolders.szTextureFolder [0], gameFolders.szTextureFolder [1], TEXTURE_FOLDER_D2, "*.tga") &&
	 GetAppFolder (gameFolders.szTextureFolder [0], gameFolders.szTextureFolder [1], TEXTURE_FOLDER_D2, ""))
	MakeFolder (gameFolders.szTextureFolder [0], gameFolders.szTextureFolder [0], TEXTURE_FOLDER_D2); // older D2X-XL installations may have a different D2 texture folder
GetAppFolder (gameFolders.szTextureFolder [0], gameFolders.szTextureFolder [2], TEXTURE_FOLDER_D1, "*.tga");
GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szMovieFolder, MOVIE_FOLDER, "*.mvl");
if (GetAppFolder (STATIC_DATA_FOLDER, gameFolders.szMissionFolder [0], BASE_MISSION_FOLDER, ""))
	GetAppFolder (gameFolders.szGameFolder, gameFolders.szMissionFolder [0], BASE_MISSION_FOLDER, "");

if (!*gameFolders.szUserFolder)
	strcpy (gameFolders.szUserFolder, STATIC_DATA_FOLDER);
strcpy (PRIVATE_DATA_FOLDER, gameFolders.szUserFolder);
if (!*VAR_DATA_FOLDER)
	strcpy (VAR_DATA_FOLDER, STATIC_DATA_FOLDER);

MakeFolder (PRIVATE_DATA_FOLDER);
MakeFolder (PRIVATE_CACHE_FOLDER, PRIVATE_DATA_FOLDER, CACHE_FOLDER);
MakeFolder (gameFolders.szMissionStateFolder, PRIVATE_CACHE_FOLDER, MISSIONSTATE_FOLDER);

#ifdef __macosx__

strcpy (VAR_DATA_FOLDER, GetMacOSXCacheFolder ());
MakeFolder (PUBLIC_CACHE_FOLDER, VAR_DATA_FOLDER, CACHE_FOLDER);
strcpy (PRIVATE_CACHE_FOLDER, PUBLIC_CACHE_FOLDER);

#else

MakeFolder (STATIC_DATA_FOLDER);
MakeFolder (gameFolders.szDataFolder [0], STATIC_DATA_FOLDER, DATA_FOLDER);
MakeFolder (gameFolders.szDataFolder [1], gameFolders.szDataFolder [0], "d2x-xl");
if (!MakeFolder (PUBLIC_CACHE_FOLDER, VAR_DATA_FOLDER, CACHE_FOLDER)) {
	strcpy (VAR_DATA_FOLDER, PRIVATE_DATA_FOLDER);	 // fall back
	strcpy (PUBLIC_CACHE_FOLDER, PRIVATE_CACHE_FOLDER);
	}

#endif // __macosx__

#ifdef _WIN32

MakeFolder (gameFolders.szTextureFolder [0], STATIC_DATA_FOLDER, TEXTURE_FOLDER);
MakeFolder (gameFolders.szModRootFolder, STATIC_DATA_FOLDER, MOD_FOLDER);
MakeFolder (gameFolders.szModCacheFolder, PUBLIC_CACHE_FOLDER, MOD_FOLDER);

MakeFolder (gameFolders.szProfileFolder, STATIC_DATA_FOLDER, PROF_FOLDER);
MakeFolder (gameFolders.szSavegameFolder, STATIC_DATA_FOLDER, SAVE_FOLDER);
MakeFolder (gameFolders.szScreenshotFolder, STATIC_DATA_FOLDER, SCRSHOT_FOLDER);
MakeFolder (gameFolders.szDemoFolder, STATIC_DATA_FOLDER, DEMO_FOLDER);
MakeFolder (gameFolders.szConfigFolder, STATIC_DATA_FOLDER, CONFIG_FOLDER);
MakeFolder (gameFolders.szDownloadFolder, STATIC_DATA_FOLDER, DOWNLOAD_FOLDER);

#else

MakeFolder (gameFolders.szTextureFolder [0], PUBLIC_CACHE_FOLDER, TEXTURE_FOLDER);
MakeFolder (gameFolders.szModelCacheFolder [0], PUBLIC_CACHE_FOLDER, MODEL_FOLDER);
MakeFolder (gameFolders.szModRootFolder, VAR_DATA_FOLDER, MOD_FOLDER);

MakeFolder (gameFolders.szProfileFolder, PRIVATE_CACHE_FOLDER, PROF_FOLDER);
MakeFolder (gameFolders.szSavegameFolder, PRIVATE_CACHE_FOLDER, SAVE_FOLDER);
MakeFolder (gameFolders.szScreenshotFolder, PRIVATE_CACHE_FOLDER, SCRSHOT_FOLDER);
MakeFolder (gameFolders.szDemoFolder, PRIVATE_CACHE_FOLDER, DEMO_FOLDER);
MakeFolder (gameFolders.szConfigFolder, PRIVATE_CACHE_FOLDER, CONFIG_FOLDER);
MakeFolder (gameFolders.szDownloadFolder, PRIVATE_CACHE_FOLDER, DOWNLOAD_FOLDER);

#endif

MakeFolder (gameFolders.szModelCacheFolder [0], PUBLIC_CACHE_FOLDER, MODEL_FOLDER);

MakeFolder (gameFolders.szTextureCacheFolder [0], PUBLIC_CACHE_FOLDER, TEXTURE_FOLDER);
MakeFolder (gameFolders.szTextureCacheFolder [1], gameFolders.szTextureCacheFolder [0], TEXTURE_FOLDER_D2);
MakeFolder (gameFolders.szTextureCacheFolder [2], gameFolders.szTextureCacheFolder [0], TEXTURE_FOLDER_D1);
MakeFolder (gameFolders.szWallpaperFolder [0], gameFolders.szTextureFolder [0], WALLPAPER_FOLDER);

MakeFolder (gameFolders.szDownloadFolder, PUBLIC_CACHE_FOLDER, DOWNLOAD_FOLDER);
MakeFolder (gameFolders.szLightmapFolder, PUBLIC_CACHE_FOLDER, LIGHTMAP_FOLDER);
MakeFolder (gameFolders.szLightDataFolder, PUBLIC_CACHE_FOLDER, LIGHTDATA_FOLDER);
MakeFolder (gameFolders.szMeshFolder, PUBLIC_CACHE_FOLDER, MESH_FOLDER);

MakeFolder (gameFolders.szMissionCacheFolder, PRIVATE_CACHE_FOLDER, MISSION_FOLDER);
MakeFolder (gameFolders.szMissionStateFolder, gameFolders.szMissionCacheFolder, MISSIONSTATE_FOLDER);
MakeFolder (gameFolders.szMissionDownloadFolder, gameFolders.szMissionCacheFolder, DOWNLOAD_FOLDER);

MakeTexSubFolders (gameFolders.szTextureCacheFolder [1]);
MakeTexSubFolders (gameFolders.szTextureCacheFolder [2]);
MakeTexSubFolders (gameFolders.szModelCacheFolder [0]);

//if (GetAppFolder (PRIVATE_DATA_FOLDER, gameFolders.szConfigFolder, CONFIG_FOLDER, "*.ini"))
//	strcpy (gameFolders.szConfigFolder, gameFolders.szGameFolder);

}

// ----------------------------------------------------------------------------

void ResetModFolders (void)
{
gameStates.app.bHaveMod = 0;
*gameFolders.szModName =
*gameFolders.szMusicFolder =
*gameFolders.szSoundFolder [4] =
*gameFolders.szSoundFolder [5] =
*gameFolders.szModFolder =
*gameFolders.szTextureFolder [3] =
*gameFolders.szTextureFolder [4] =
*gameFolders.szTextureCacheFolder [3] =
*gameFolders.szTextureCacheFolder [4] =
*gameFolders.szModelFolder [1] =
*gameFolders.szModelFolder [2] =
*gameFolders.szModelCacheFolder [1] =
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

if (GetAppFolder (gameFolders.szModRootFolder, gameFolders.szModFolder, gameFolders.szModName, "")) 
	*gameFolders.szModName = '\0';
else {
	sprintf (gameFolders.szSoundFolder [4], "%s/%s", gameFolders.szModFolder, SOUND_FOLDER);
	if (GetAppFolder (gameFolders.szModFolder, gameFolders.szTextureFolder [3], TEXTURE_FOLDER, "*.tga"))
		*gameFolders.szTextureFolder [3] = '\0';
	else {
		sprintf (gameFolders.szTextureCacheFolder [3], "%s/%s/%s", PUBLIC_CACHE_FOLDER, gameFolders.szModName, TEXTURE_FOLDER);
		//gameOpts->render.textures.bUseHires [0] = 1;
		}
	if (GetAppFolder (gameFolders.szModFolder, gameFolders.szModelFolder [1], MODEL_FOLDER, "*.ase") &&
		 GetAppFolder (gameFolders.szModFolder, gameFolders.szModelFolder [1], MODEL_FOLDER, "*.oof"))
		*gameFolders.szModelFolder [1] = '\0';
	else {
		sprintf (gameFolders.szModelFolder [1], "%s/%s", gameFolders.szModFolder, MODEL_FOLDER);
		sprintf (gameFolders.szModelCacheFolder [1], "%s/%s/%s", PUBLIC_CACHE_FOLDER, gameFolders.szModName, MODEL_FOLDER);
		}
	if (GetAppFolder (gameFolders.szModFolder, gameFolders.szWallpaperFolder [1], WALLPAPER_FOLDER, "*.tga")) {
		*gameFolders.szWallpaperFolder [1] = '\0';
		*gameOpts->menus.altBg.szName [1] = '\0';
		}
	else {
		sprintf (gameFolders.szWallpaperFolder [1], "%s/%s", gameFolders.szModFolder, WALLPAPER_FOLDER);
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
	if (GetAppFolder (gameFolders.szModFolder, gameFolders.szMusicFolder, MUSIC_FOLDER, "*.ogg"))
		*gameFolders.szMusicFolder = '\0';
	MakeTexSubFolders (gameFolders.szTextureCacheFolder [3]);
	MakeTexSubFolders (gameFolders.szModelCacheFolder [1]);
	gameStates.app.bHaveMod = 1;
	}
}

// ----------------------------------------------------------------------------
