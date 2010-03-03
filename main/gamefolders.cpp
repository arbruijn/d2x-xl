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
#include "u_mem.h"
#include "strutil.h"
#include "key.h"
#include "timer.h"
#include "error.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "iff.h"
#include "pcx.h"
#include "args.h"
#include "hogfile.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "sdlgl.h"
#include "text.h"
#include "newdemo.h"
#include "objrender.h"
#include "renderthreads.h"
#include "network.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "desc_id.h"
#include "joydefs.h"
#include "gamepal.h"
#include "movie.h"
#include "compbit.h"
#include "playsave.h"
#include "tracker.h"
#include "rendermine.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"
#include "gameargs.h"

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
#	define	CACHEDIR			"cache"
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
	char	c;
#endif

*gameFolders.szHomeDir =
*gameFolders.szGameDir =
*gameFolders.szDataDir =
*szDataRootDir = '\0';
if ((i = FindArg ("-userdir")) && pszArgList [i + 1] && *pszArgList [i + 1]) {
	sprintf (gameFolders.szGameDir, "%s\\%s\\", pszArgList [i + 1], DATADIR);
	if (GetAppFolder ("", gameFolders.szGameDir, gameFolders.szGameDir, "*.hog"))
		*gameFolders.szGameDir = '\0';
	else {
		strcpy (gameFolders.szGameDir, pszArgList [i + 1]);
		int j = (int) strlen (gameFolders.szGameDir);
		if (j && (gameFolders.szGameDir [j-1] != '\\') && (gameFolders.szGameDir [j-1] != '/'))
			strcat (gameFolders.szGameDir, "/");
		}
	}
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, getenv ("DESCENT2"), D2X_APPNAME))
	*gameFolders.szGameDir = '\0';
#ifdef _WIN32
if (!*gameFolders.szGameDir) {
	psz = pszArgList [0];
	for (int j = (int) strlen (psz); j; ) {
		c = psz [--j];
		if ((c == '\\') || (c == '/')) {
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
if (*gameFolders.szGameDir)
	chdir (gameFolders.szGameDir);
#endif //Linux, OS X
if ((i = FindArg ("-hogdir")) && !GetAppFolder ("", gameFolders.szDataDir, pszArgList [i + 1], "descent2.hog"))
	strcpy (szDataRootDir, pszArgList [i + 1]);
else {
	sprintf (gameFolders.szDataDir, "%s%s", gameFolders.szGameDir, DATADIR);
	if (GetAppFolder ("", gameFolders.szDataDir, gameFolders.szDataDir, "descent2.hog") &&
		 GetAppFolder ("", gameFolders.szDataDir, gameFolders.szDataDir, "d2demo.hog") &&
		 GetAppFolder (szDataRootDir, gameFolders.szDataDir, DATADIR, "descent2.hog") &&
		 GetAppFolder (szDataRootDir, gameFolders.szDataDir, DATADIR, "d2demo.hog"))
	Error (TXT_NO_HOG2);
	}
psz = strstr (gameFolders.szDataDir, DATADIR);
if (psz && !*(psz + 4)) {
	if (psz == gameFolders.szDataDir)
		sprintf (gameFolders.szDataDir, "%s%s", gameFolders.szGameDir, DATADIR);
	else {
		*(psz - 1) = '\0';
		strcpy (szDataRootDir, gameFolders.szDataDir);
		*(psz - 1) = '/';
		}
	}
else
	strcpy (szDataRootDir, gameFolders.szDataDir);
/*---*/PrintLog ("expected game app folder = '%s'\n", gameFolders.szGameDir);
/*---*/PrintLog ("expected game data folder = '%s'\n", gameFolders.szDataDir);
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
GetAppFolder (szDataRootDir, gameFolders.szWallpaperDir, WALLPAPERDIR, "");
#else
GetAppFolder (gameFolders.szTextureDir [0], gameFolders.szWallpaperDir, WALLPAPERDIR, "");
#endif
#ifdef _WIN32
sprintf (gameFolders.szMissionDir, "%s%s", gameFolders.szGameDir, BASE_MISSION_DIR);
#else
sprintf (gameFolders.szMissionDir, "%s/%s", gameFolders.szGameDir, BASE_MISSION_DIR);
#endif
//if (i = FindArg ("-hogdir"))
//	CFUseAltHogDir (pszArgList [i + 1]);
for (i = 0; i < 2; i++)
	MakeTexSubFolders (gameFolders.szTextureCacheDir [i]);
MakeTexSubFolders (gameFolders.szModelCacheDir [0]);
sprintf (gameFolders.szMissionDownloadDir, "%s/%s", gameFolders.szMissionDir, DOWNLOADDIR);
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
*gameFolders.szModelCacheDir [2] = '\0';
}

// ----------------------------------------------------------------------------

void MakeModFolders (const char* pszMission)
{
int bDefault, bBuiltIn;

ResetModFolders ();
if (gameStates.app.bDemoData)
	return;

if ((bDefault = (*pszMission == '\0')))
	pszMission = gameData.missions.list [gameData.missions.nCurrentMission].szMissionName;
else
	CFile::SplitPath (pszMission, NULL, gameFolders.szModName, NULL);

if ((bBuiltIn = (strstr (pszMission, "Descent: First Strike") != NULL)))
	strcpy (gameFolders.szModName, "descent");
else if ((bBuiltIn = (strstr (pszMission, "Descent 2: Counterstrike!") != NULL)))
	strcpy (gameFolders.szModName, "descent2");
else if ((bBuiltIn = (strstr (pszMission, "Descent 2: Vertigo") != NULL)))
	strcpy (gameFolders.szModName, "d2x");
else if (bDefault)
	return;

if (bBuiltIn && !gameOpts->app.bEnableMods)
	return;

if (!GetAppFolder (gameFolders.szModDir [0], gameFolders.szModDir [1], gameFolders.szModName, "")) {
	sprintf (gameFolders.szSoundDir [4], "%s/%s", gameFolders.szModDir [1], SOUNDDIR);
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szTextureDir [2], TEXTUREDIR, "*.tga"))
		*gameFolders.szTextureDir [2] = '\0';
	else {
		sprintf (gameFolders.szTextureCacheDir [2], "%s/%s", gameFolders.szModDir [1], TEXTUREDIR);
		gameOpts->render.textures.bUseHires [0] = 1;
		}
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szModelDir [1], MODELDIR, "*.ase") &&
		 GetAppFolder (gameFolders.szModDir [1], gameFolders.szModelDir [1], MODELDIR, "*.oof"))
		*gameFolders.szModelDir [1] = '\0';
	else {
		sprintf (gameFolders.szModelDir [1], "%s/%s", gameFolders.szModDir [1], MODELDIR);
		sprintf (gameFolders.szModelCacheDir [1], "%s/%s", gameFolders.szModDir [1], MODELDIR);
		}
	if (GetAppFolder (gameFolders.szModDir [1], gameFolders.szMusicDir, MUSICDIR, "*.ogg"))
		*gameFolders.szMusicDir = '\0';
	MakeTexSubFolders (gameFolders.szTextureCacheDir [2]);
	MakeTexSubFolders (gameFolders.szModelCacheDir [1]);
	gameStates.app.bHaveMod = 1;
	}
else
	*gameFolders.szModName = '\0';
}

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
	{"\002d2x-h.mvl", "movies", false, false, false},
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
	{"\002bullettime#0.tga", "textures", false, false, false},
	{"\002cockpit.tga", "textures", false, false, false},
	{"\002cockpitb.tga", "textures", false, false, false},
	{"\002monsterball.tga", "textures", false, false, false},
	{"\002slowmotion#0.tga", "textures", false, false, false},
	{"\002status.tga", "textures", false, false, false},
	{"\002statusb.tga", "textures", false, false, false},

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
	{"\002thrust2d.tga", "textures/d2x-xl", false, false, false},
	{"\002thrust2d-blue.tga", "textures/d2x-xl", false, false, false},
	{"\002thrust2d-red.tga", "textures/d2x-xl", false, false, false},
	{"\002thrust3d.tga", "textures/d2x-xl", true, false, false},
	{"\002thrust3d-blue.tga", "textures/d2x-xl", true, false, false},
	{"\002thrust3d-red.tga", "textures/d2x-xl", true, false, false}
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

static char szRootFolder [FILENAME_LEN];
static char szHomeFolder [FILENAME_LEN];
static char szUserFolder [FILENAME_LEN];

// ----------------------------------------------------------------------------

static bool CheckAndCopyWildcards (tFileDesc* fileDesc)
{
	FFS	ffs;
	int	i;
	char	szFilter [FILENAME_LEN], szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	CFile	cf;

// quit if none of the specified files exist in the source folder
if ((i = FFF (fileDesc->pszFile, &ffs, 0))) {
	sprintf (szFilter, "%s%s\\%s", fileDesc->bUser ? szHomeFolder : szRootFolder, fileDesc->pszFolder, fileDesc->pszFile);
	return FFF (szFilter, &ffs, 0) == 0;
	}
do {
	sprintf (szDest, "\002%s", ffs.name);
	sprintf (szFilter, "%s%s", fileDesc->bUser ? szUserFolder : szRootFolder, fileDesc->pszFolder);
	if (!CFile::Exist (szDest, szFilter, 0)) {	// if the file doesn't exist in the destination folder copy it
		sprintf (szSrc, "%s%s", fileDesc->bUser ? szHomeFolder : szRootFolder, ffs.name);
		sprintf (szDest, "%s%s\\%s", fileDesc->bUser ? szUserFolder : szRootFolder, fileDesc->pszFolder, ffs.name);
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
		sprintf (szDest, "%s%s", fileList [i].bUser ? szUserFolder : szRootFolder, fileList [i].pszFolder);
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, szDest, false) == 1;
		if (fileList [i].bFound)
			continue;	// file exists in the destination folder
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, fileList [i].bUser ? szHomeFolder : szRootFolder, false) == 1;
		if (fileList [i].bFound) {	// file exists in the source folder
			sprintf (szSrc, "%s%s", szRootFolder, fileList [i].pszFile + 1);
			sprintf (szDest, "%s%s\\%s", fileList [i].bUser ? szUserFolder : szRootFolder, fileList [i].pszFolder, fileList [i].pszFile + 1);
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
static const char* gameFolders [] = {
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

for (int i = 0; i < int (sizeofa (gameFolders)); i++) {
	sprintf (szFolder, "%s%s", szRootFolder, gameFolders [i]);
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
			if (strcmp (fileList [i].bUser ? szUserFolder : szRootFolder, ".\\")) {
				strcat (szMsg, fileList [i].bUser ? szUserFolder : szRootFolder);
				l += int (strlen (fileList [i].bUser ? szUserFolder : szRootFolder));
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

	int	i, nResult = 0;
	bool	bDemoData = false;
	char	szMsg [10000];

if ((i = FindArg ("-userdir")) && pszArgList [i + 1] && *pszArgList [i + 1]) {
	strcpy (szRootFolder, pszArgList [i + 1]);
	i = int (strlen (szRootFolder));
#if defined(__unix__)
	if (szRootFolder [i - 1] != '/')
		strcat (szRootFolder, "/");
#else
	if ((szRootFolder [i - 1] != '\\') && (szRootFolder [i - 1] != '/') && (szRootFolder [i - 1] != ':'))
		strcat (szRootFolder, "/");
#endif
	}
else
#if defined(__unix__)
	strcpy (szRootFolder, *gameFolders.szSharePath ? gameFolders.szSharePath : "/usr/local/games/d2x-xl/");
	if (szRootFolder [strlen (szRootFolder) - 1] != '/')
		strcat (szRootFolder, "/");
#else
	strcpy (szRootFolder, "./");
#endif

#if defined(__unix__)
if (getenv ("HOME")) {
	strcpy (szHomeFolder, getenv ("HOME"));
	i = int (strlen (szHomeFolder));
	if (szHomeFolder [i - 1] != '/')
		strcat (szHomeFolder, "/");
	}
else
	strcpy (szHomeFolder, "./");
sprintf (szUserFolder, "%s/.d2x-xl/", szHomeFolder);
#else
	strcpy (szHomeFolder, szRootFolder);
	strcpy (szUserFolder, szRootFolder);
#endif

#if defined(_WIN32)
CheckAndCreateGameFolders ();
#endif
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
	if (cf.Open ("d2x-xl-missing-files.txt", gameFolders.szHomeDir, "rt", 0)) {
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

#if defined(__unix__)

#	define FILEEXT		"rar"
#	define FILETYPE		"src"

#include <curl/curl.h>
#include <cstdio>

// link with libcurl (-lcurl)

int DownloadFile (const char* pszSrc, const char* pszDest)
{
CURL* hCurl;
if (!(hCurl = curl_easy_init ()))
	return 1;
if (curl_easy_setopt (hCurl, CURLOPT_URL, pszSrc)) {
	curl_easy_cleanup (hCurl);
	return 1;
	}
std::FILE* file;
if (!(file = std::fopen (pszDest, "w"))) {
	curl_easy_cleanup (hCurl);
	return 1;
	}
#if DBG
curl_easy_setopt (hCurl, CURLOPT_VERBOSE, 1);
#endif
if (curl_easy_setopt (hCurl, CURLOPT_WRITEDATA, file)) {
	curl_easy_cleanup (hCurl);
	return 1;
	}
if (curl_easy_perform (hCurl)) {
	curl_easy_cleanup (hCurl);
	std::fclose (file);
	unlink (pszDest);
	return 1;
	}
curl_easy_cleanup (hCurl);
std::fclose (file);
return 0;
}

// ----------------------------------------------------------------------------

#elif defined(_WIN32)

#	include "urlmon.h"
#	include <process.h>
#	include "errno.h"

#	define DownloadFile(_src,_dest)	URLDownloadToFile (NULL, _src, _dest, NULL, NULL)

#	define FILEEXT		"exe"
#	define FILETYPE		"win"

#endif

#if defined(_WIN32) || defined(__unix__)

int CheckForUpdate (void)
{
	char		szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	CFile		cf;
	int		nVersion [3], nLocation;
	char		szMsg [1000];

	static char* pszSource [2] = {
		"http://www.descent2.de/downloads", 
		"http://sourceforge.net/projects/d2x-xl/files"
	};

sprintf (szDest, "%s/d2x-xl-version.txt", gameFolders.szDownloadDir);
if (!DownloadFile ("http://www.descent2.de/downloads/d2x-xl-version.txt", szDest))
	nLocation = 0;
else if (!DownloadFile ("http://sourceforge.net/projects/d2x-xl/files/d2x-xl-version.txt/download", szDest))
	nLocation = 1;
else {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
if (!cf.Open ("d2x-xl-version.txt", gameFolders.szDownloadDir, "rt", -1)) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
if (3 != fscanf (cf.File (), "%d.%d.%d", &nVersion [0], &nVersion [1], &nVersion [2])) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}

#if !DBG
if (D2X_IVER >= nVersion [0] * 100000 + nVersion [1] * 1000 + nVersion [2]) {
	MsgBox (NULL, NULL, 1, TXT_CLOSE, "No updates were found.");
	return 0;
	}
#endif

if (MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, "An update has been found. Download it?"))
	return 0;
sprintf (szDest, "%s/d2x-xl-%s-%d.%d.%d.%s", gameFolders.szDownloadDir,
			FILETYPE, nVersion [0], nVersion [1], nVersion [2], FILEEXT);
#if 1
messageBox.Show ("Downloading...");
sprintf (szSrc, "%s/d2x-xl-%s-%d.%d.%d.%s", pszSource [nLocation], FILETYPE, nVersion [0], nVersion [1], nVersion [2], FILEEXT);
if (DownloadFile (szSrc, szDest)) {
	messageBox.Clear ();
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
messageBox.Clear ();
if (!cf.Exist (szDest, "", 0)) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, "Download failed.");
	return -1;
	}
#endif
#if defined(__unix__)
sprintf (szMsg, "\nThe file\n\n%s\n\nwas sucessfully downloaded.", szDest);
MsgBox (NULL, NULL, 1, TXT_CLOSE, szMsg);
#else
#	if 1
#	include "shellapi.h"
#	include "objbase.h"
#if !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_OPENMP) && !defined(__INTEL_COMPILER)
CoInitializeEx (NULL, COINIT_MULTITHREADED);
#	else
CoInitialize (NULL);
#	endif
if (HINSTANCE (32) < ShellExecute (NULL, NULL, szDest, NULL, NULL, SW_SHOW))
	exit (1);
#else
char*	args [2] = {szDest, NULL};
if (0 <= _execv (szDest, args))
	exit (1);
#endif
sprintf (szMsg, "\nThe file\n\n%s\n\nwas sucessfully downloaded, but couldn't be excuted.\nPlease leave D2X-XL and start the installer manually.", szDest);
//Warning (szMsg);
MsgBox (TXT_ERROR, NULL, 1, TXT_CLOSE, szMsg);
#endif
return -1;
}

#endif

// ----------------------------------------------------------------------------
