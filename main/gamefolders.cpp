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

#if defined(__macosx__)
#	define	DATADIR			"Data"
#	define	SHADERDIR		"Shaders"
#	define	MODELDIR			"Models"
#	define	SOUNDDIR			"Sounds"
#	define	SOUNDDIR1		"Sounds1"
#	define	SOUNDDIR2		"Sounds2"
#	define	SOUNDDIR1_D1	"Sounds1/D1"
#	define	SOUNDDIR2_D1	"Sounds2/D1"
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
strcpy (szDataRootDir, gameFolders.szGameDir);
strcpy (gameFolders.szHomeDir, *gameFolders.szGameDir ? gameFolders.szGameDir : szDataRootDir);
#else // Linux, OS X
#	if defined (__unix__) || defined (__macosx__)
if (getenv ("HOME"))
	strcpy (gameFolders.szHomeDir, getenv ("HOME"));
#		if 0
if (!*gameFolders.szGameDir && *gameFolders.szHomeDir && GetAppFolder (gameFolders.szHomeDir, gameFolders.szGameDir, "d2x-xl", "d2x-xl"))
		*gameFolders.szGameDir = '\0';
#		endif
#	endif //__unix__
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, STD_GAMEDIR, ""))
		*gameFolders.szGameDir = '\0';
if (!*gameFolders.szGameDir && GetAppFolder ("", gameFolders.szGameDir, SHAREPATH, ""))
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
	}
#endif
if (*gameFolders.szHomeDir) {
#ifdef __macosx__
	char *pszOSXCacheDir = GetMacOSXCacheFolder ();
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
	if (szDataRootDir [i = (int) strlen (szDataRootDir) - 1] == '\\')
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

#ifdef _WIN32

typedef struct tFileDesc {
	char*	pszFile;
	char*	pszFolder;
	bool	bOptional;
	bool	bAddon;
	bool	bFound;
} tFileDesc;

static tFileDesc gameFiles [] = {
	// basic game files
	{"\002descent.cfg", ".\\config", false, false, false},
	{"\002alien1.pig", ".\\data", false, false, false},
	{"\002alien2.pig", ".\\data", false, false, false},
	{"\002descent.pig", ".\\data", false, false, false},
	{"\002fire.pig", ".\\data", false, false, false},
	{"\002groupa.pig", ".\\data", false, false, false},
	{"\002ice.pig", ".\\data", false, false, false},
	{"\002water.pig", ".\\data", false, false, false},
	{"\002descent.hog", ".\\data", false, false, false},
	{"\002descent2.hog", ".\\data", false, false, false},
	{"\002descent2.ham", ".\\data", false, false, false},
	{"\002descent2.p11", ".\\data", false, false, false},
	{"\002descent2.p22", ".\\data", false, false, false},
	{"\002descent2.s11", ".\\data", false, false, false},
	{"\002descent2.s22", ".\\data", false, false, false},
	{"\002d2x-h.mvl", ".\\movies", false, false, false},
	{"\002d2x-l.mvl", ".\\movies", false, false, false},
	{"\002intro-h.mvl", ".\\movies", false, false, false},
	{"\002intro-l.mvl", ".\\movies", false, false, false},
	{"\002other-h.mvl", ".\\movies", false, false, false},
	{"\002other-l.mvl", ".\\movies", false, false, false},
	{"\002robots-h.mvl", ".\\movies", false, false, false},
	{"\002robots-l.mvl", ".\\movies", false, false, false}
};

static tFileDesc vertigoFiles [] = {
	// Vertigo expansion
	{"\002hoard.ham", ".\\data", true, false, false},
	{"\002d2x.hog", ".\\missions", true, false, false},
	{"\002d2x.mn2", ".\\missions", true, false, false}
};

static tFileDesc addonFiles [] = {
	// D2X-XL addon files
	{"\002d2x-default.ini", ".\\config", false, true, false},
	{"\002d2x.ini", ".\\config", true, true, false},

	{"\002d2x-xl.hog", ".\\data", false, true, false},

	{"*.plx", ".\\profiles", true, true, false},
	{"*.plr", ".\\profiles", true, true, false},

	{"\002bullet.ase", ".\\models", false, true, false},
	{"\002bullet.tga", ".\\models", false, true, false},

	{"*.sg?", ".\\savegames", true, true, false},

	{"\002afbr_1.wav", ".\\sounds2", false, true, false},
	{"\002airbubbles.wav", ".\\sounds2", false, true, false},
	{"\002gatling_slowdown.wav", ".\\sounds2", false, true, false},
	{"\002gatling-speedup.wav", ".\\sounds2", false, true, false},
	{"\002gauss-firing.wav", ".\\sounds2", false, true, false},
	{"\002headlight.wav", ".\\sounds2", false, true, false},
	{"\002highping.wav", ".\\sounds2", false, true, false},
	{"\002lightning.wav", ".\\sounds2", false, true, false},
	{"\002lowping.wav", ".\\sounds2", false, true, false},
	{"\002missileflight-big.wav", ".\\sounds2", false, true, false},
	{"\002mssileflight-small.wav", ".\\sounds2", false, true, false},
	{"\002slowdown.wav", ".\\sounds2", false, true, false},
	{"\002speedup.wav", ".\\sounds2", false, true, false},
	{"\002vulcan-firing.wav", ".\\sounds2", false, true, false},
	{"\002zoom1.wav", ".\\sounds2", false, true, false},
	{"\002zoom2.wav", ".\\sounds2", false, true, false},

	{"\002bullettime#0.tga", ".\\textures", false, true, false},   
	{"\002cockpit.tga", ".\\textures", false, true, false},       
	{"\002cockpitb.tga", ".\\textures", false, true, false},         
	{"\002monsterball.tga", ".\\textures", false, true, false},
	{"\002slowmotion#0.tga", ".\\textures", false, true, false},   
	{"\002status.tga", ".\\textures", false, true, false},        
	{"\002statusb.tga", ".\\textures", false, true, false},
    
	{"\002aimdmg.tga", ".\\textures\\d2x-xl", false, true, false},         
	{"\002blast.tga", ".\\textures\\d2x-xl", false, true, false},         
	{"\002blast-hard.tga", ".\\textures\\d2x-xl", false, true, false},       
	{"\002blast-medium.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002blast-soft.tga", ".\\textures\\d2x-xl", false, true, false},     
	{"\002bubble.tga", ".\\textures\\d2x-xl", false, true, false},        
	{"\002bullcase.tga", ".\\textures\\d2x-xl", false, true, false},         
	{"\002corona.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002deadzone.tga", ".\\textures\\d2x-xl", false, true, false},       
	{"\002drivedmg.tga", ".\\textures\\d2x-xl", false, true, false},      
	{"\002fire.tga", ".\\textures\\d2x-xl", false, true, false},             
	{"\002glare.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002gundmg.tga", ".\\textures\\d2x-xl", false, true, false},         
	{"\002halfhalo.tga", ".\\textures\\d2x-xl", false, true, false},      
	{"\002halo.tga", ".\\textures\\d2x-xl", false, true, false},             
	{"\002joymouse.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002pwupicon.tga", ".\\textures\\d2x-xl", false, true, false},       
	{"\002rboticon.tga", ".\\textures\\d2x-xl", false, true, false},      
	{"\002scope.tga", ".\\textures\\d2x-xl", false, true, false},            
	{"\002shield.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002smoke.tga", ".\\textures\\d2x-xl", false, true, false},          
	{"\002smoke-hard.tga", ".\\textures\\d2x-xl", false, true, false},    
	{"\002smoke-medium.tga", ".\\textures\\d2x-xl", false, true, false},     
	{"\002smoke-soft.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002sparks.tga", ".\\textures\\d2x-xl", false, true, false},         
	{"\002thrust2d.tga", ".\\textures\\d2x-xl", false, true, false},      
	{"\002thrust2d-blue.tga", ".\\textures\\d2x-xl", false, true, false},    
	{"\002thrust2d-red.tga", ".\\textures\\d2x-xl", false, true, false},
	{"\002thrust3d.tga", ".\\textures\\d2x-xl", false, true, false},       
	{"\002thrust3d-blue.tga", ".\\textures\\d2x-xl", false, true, false}, 
	{"\002thrust3d-red.tga", ".\\textures\\d2x-xl", false, true, false}
};

// ----------------------------------------------------------------------------

bool CheckAndCopyWildcards (const char *szFile, const char* szFolder)
{
	FFS	ffs;
	int	i;
	char	szFilter [FILENAME_LEN], szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	CFile	cf;


if (i = FFF (szFile, &ffs, 0)) {
	sprintf_s (szFilter, sizeof (szFilter), "%s\\%s", szFolder, szFile);
	return FFF (szFilter, &ffs, 0) == 0;
	}
do {
	sprintf_s (szDest, sizeof (szDest), "\002%s", ffs.name);
	if (!CFile::Exist (szDest, szFolder, 0)) {
		sprintf_s (szSrc, ".\\%s", ffs.name);
		sprintf_s (szDest, sizeof (szDest), "%s\\%s", szFolder, ffs.name);
		cf.Copy (szSrc, szDest);
		}
	} while (FFN (&ffs, 0));
return true;
}

// ----------------------------------------------------------------------------

int CheckAndCopyFiles (tFileDesc* fileList, int nFiles)
{
	char	szSrc [FILENAME_LEN], szDest [FILENAME_LEN];
	int	nErrors = 0;
	CFile	cf;

for (int i = 0; i < nFiles; i++) {
	if (strstr (fileList [i].pszFile, "*") || strstr (fileList [i].pszFile, "?")) {
		fileList [i].bFound = CheckAndCopyWildcards (fileList [i].pszFile, fileList [i].pszFolder);
		if (!(fileList [i].bFound || fileList [i].bOptional))
			nErrors++;		
		}
	else {
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, fileList [i].pszFolder, false) == 1;
		if (fileList [i].bFound)
			continue;
		fileList [i].bFound = CFile::Exist (fileList [i].pszFile, ".\\", false) == 1;
		if (fileList [i].bFound) {
			sprintf_s (szSrc, sizeof (szSrc), ".\\%s", fileList [i].pszFile + 1);
			sprintf_s (szDest, sizeof (szDest), "%s\\%s", fileList [i].pszFolder, fileList [i].pszFile + 1);
			cf.Copy (szSrc, szDest);
			}
		else if (!fileList [i].bOptional)
			nErrors++;
		}
	}	
return nErrors;
}

#endif

// ----------------------------------------------------------------------------

void CheckAndCreateGameFolders (void)
{
static char* gameFolders [] = {
	".\\cache",
	".\\config",
	".\\data",
	".\\models",
	".\\mods",
	".\\movies",
	".\\profiles",
	".\\savegames",
	".\\screenshots",
	".\\sounds2",
	".\\sounds2\\d2x-xl",
	".\\textures"
};

	FFS	ffs;

for (int i = 0; i < int (sizeofa (gameFolders)); i++) 
	  if (FFF (gameFolders [i], &ffs, 1))
	  		CFile::MkDir (gameFolders [i]);
}

// ----------------------------------------------------------------------------

int CheckAndFixSetup (void)
{
#if defined (_WIN32) && !defined(_M_IA64) && !defined(_M_AMD64)
	int	nResult = 0;

CheckAndCreateGameFolders ();
if (CheckAndCopyFiles (gameFiles, int (sizeofa (gameFiles))))
	nResult |= 1;
if (CheckAndCopyFiles (addonFiles, int (sizeofa (addonFiles))))
	nResult |= 2;
if (CheckAndCopyFiles (vertigoFiles, int (sizeofa (vertigoFiles))))
	nResult |= 4;
#endif
return nResult;
}

// ----------------------------------------------------------------------------
