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
#include "playerprofile.h"
#include "tracker.h"
#include "rendermine.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"
#include "songs.h"
#include "cvar.h"
#include "lightcluster.h"

void PrintVersion (void);

// ----------------------------------------------------------------------------

void EvalAutoNetGameArgs (void)
{
	int	t, bHaveIp = 0;
	char	*p;
#if 0
	static const char *pszTypes [] = {"anarchy", "coop", "ctf", "ctf+", "hoard", "entropy", NULL};
	static const char	*pszConnect [] = {"ipx", "udp", "", "multicast", NULL};
#endif
memset (&gameData.multiplayer.autoNG, 0, sizeof (gameData.multiplayer.autoNG));
if ((t = FindArg ("-ng_player")) && (p = appConfig [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szPlayer, appConfig [t+1], 8);
	gameData.multiplayer.autoNG.szPlayer [8] = '\0';
	}
if ((t = FindArg ("-ng_mission")) && (p = appConfig [t+1])) { // was -ng_file
	strncpy (gameData.multiplayer.autoNG.szFile, appConfig [t+1], FILENAME_LEN - 1);
	gameData.multiplayer.autoNG.szFile [FILENAME_LEN - 1] = '\0';
	}
#if 0
if ((t = FindArg ("-ng_mission")) && (p = appConfig [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szMission, appConfig [t+1], 12);
	gameData.multiplayer.autoNG.szMission [12] = '\0';
	}
#endif
if ((t = FindArg ("-ng_level")))
	gameData.multiplayer.autoNG.nLevel = NumArg (t, 1);
else
	gameData.multiplayer.autoNG.nLevel = 1;
if ((t = FindArg ("-ng_server")) && (p = appConfig [t+1]))
	bHaveIp = stoip (appConfig [t+1], gameData.multiplayer.autoNG.ipAddr, &gameData.multiplayer.autoNG.nPort);

if ((t = FindArg ("-ng_team")))
	gameData.multiplayer.autoNG.bTeam = NumArg (t, 1);

#if 0 // game host parameters
if ((t = FindArg ("-ng_name")) && (p = appConfig [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szName, appConfig [t+1], 80);
	gameData.multiplayer.autoNG.szName [80] = '\0';
	}
if ((t = FindArg ("-ng_host")))
	gameData.multiplayer.autoNG.bHost = !strcmp (p, "host");
if ((t = FindArg ("-ng_type")) && (p = appConfig [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (!strcmp (p, pszTypes [t])) {
			gameData.multiplayer.autoNG.uType = t;
			break;
			}
	}
if ((t = FindArg ("-ng_protocol")) && (p = appConfig [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (*pszConnect [t] && !strcmp (p, pszConnect [t])) {
			gameData.multiplayer.autoNG.uConnect = t;
			break;
			}
	}
else
#endif
	gameData.multiplayer.autoNG.uConnect = 1;

if (gameData.multiplayer.autoNG.bHost)
	gameData.multiplayer.autoNG.bValid =
		*gameData.multiplayer.autoNG.szPlayer &&
		*gameData.multiplayer.autoNG.szName &&
		*gameData.multiplayer.autoNG.szFile &&
		*gameData.multiplayer.autoNG.szMission;
else
	gameData.multiplayer.autoNG.bValid =
		//*gameData.multiplayer.autoNG.szPlayer &&
		//*gameData.multiplayer.autoNG.szFile &&
		//gameData.multiplayer.autoNG.nLevel &&
		bHaveIp;

if (gameData.multiplayer.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
}

// ----------------------------------------------------------------------------

void EvalMultiplayerArgs (void)
{
if (FindArg ("-norankings"))
	gameOptions [0].multi.bNoRankings = 1;
EvalAutoNetGameArgs ();
}

// ----------------------------------------------------------------------------

void EvalMovieArgs (void)
{
	int	t;

if ((t = FindArg ("-nomovies")))
	gameOptions [0].movies.nLevel = 2 - NumArg (t, 2);
if ((t = FindArg ("-movies")))
	gameOptions [0].movies.nLevel = NumArg (t, 2);
if (gameOptions [0].movies.nLevel < 0)
	gameOptions [0].movies.nLevel = 0;
else if (gameOptions [0].movies.nLevel > 2)
	gameOptions [0].movies.nLevel = 2;
if (FindArg ("-subtitles"))
	gameOptions [0].movies.bSubTitles = NumArg (t, 1);
if ((t = FindArg ("-movie_quality")))
	gameOptions [0].movies.nQuality = NumArg (t, 0);
if (gameData.multiplayer.autoNG.bValid > 0)
	gameOptions [0].movies.nLevel = 0;
}

// ----------------------------------------------------------------------------

void EvalSoundArgs (void)
{
	int	t;

#if USE_SDL_MIXER
#	ifdef __macosx__
void * volatile function_p = (void *)&(Mix_OpenAudio);
if (function_p == NULL) {

	// the SDL_mixer framework is not present,
	// so regardless of what conf.h or d2x.ini says,
	// we don't want to use SDL_mixer

	gameOptions [0].sound.bUseSDLMixer = 0;
	}
else
#	endif //__macosx__
if ((t = FindArg ("-sdl_mixer")))
	gameOptions [0].sound.bUseSDLMixer = NumArg (t, 1);
#endif //USE_SDL_MIXER
if ((t = FindArg ("-midifix")))
	gameStates.sound.bMidiFix = NumArg (t, 1);
if ((t = FindArg ("-noredbook")))
	gameOptions [0].sound.bUseRedbook = 0;
#if USE_SDL_MIXER
if (gameOptions [0].sound.bUseSDLMixer) {
	if ((t = FindArg ("-hires_sound")))
		gameOptions [0].sound.bHires [0] =
		gameOptions [0].sound.bHires [1] = NumArg (t, 2);
	}
#endif
}

// ----------------------------------------------------------------------------

void EvalMusicArgs (void)
{
	int	t;
	char	*p;

if ((t = FindArg ("-nomusic")))
	gameStates.sound.audio.bNoMusic = NumArg (t, 0) == 0;
if ((t = FindArg ("-playlist")) && (p = appConfig [t+1]))
	songManager.LoadPlayList (p);
if ((t = FindArg ("-introsong")) && (p = appConfig [t+1]))
	strncpy (songManager.IntroSong (), p, FILENAME_LEN);
if ((t = FindArg ("-briefingsong")) && (p = appConfig [t+1]))
	strncpy (songManager.BriefingSong (), p, FILENAME_LEN);
if ((t = FindArg ("-creditssong")) && (p = appConfig [t+1]))
	strncpy (songManager.CreditsSong (), p, FILENAME_LEN);
if ((t = FindArg ("-menusong")) && (p = appConfig [t+1]))
	strncpy (songManager.MenuSong (), p, FILENAME_LEN);
}

// ----------------------------------------------------------------------------

void EvalMenuArgs (void)
{
	int	t;

if (!gameStates.app.bNostalgia) {
	if ((t = FindArg ("-menustyle")))
		gameOptions [0].menus.nStyle = NumArg (t, 1);
	else
		gameOptions [0].menus.nStyle = 1;
	}
if ((t = FindArg ("-fademenus")))
	gameOptions [0].menus.nFade = NumArg (t, 0);
if ((t = FindArg ("-altbg_alpha")) && *appConfig [t+1]) {
	gameOptions [0].menus.altBg.alpha = atof (appConfig [t+1]);
	if (gameOptions [0].menus.altBg.alpha < 0)
		gameOptions [0].menus.altBg.alpha = -1.0;
	else if ((gameOptions [0].menus.altBg.alpha == 0) || (gameOptions [0].menus.altBg.alpha > 1.0))
		gameOptions [0].menus.altBg.alpha = 1.0;
	}
if ((t = FindArg ("-altbg_brightness")) && *appConfig [t+1]) {
	gameOptions [0].menus.altBg.brightness = atof (appConfig [t+1]);
	if ((gameOptions [0].menus.altBg.brightness <= 0) || (gameOptions [0].menus.altBg.brightness > 1.0))
		gameOptions [0].menus.altBg.brightness = 1.0;
	}
if ((t = FindArg ("-altbg_grayscale")))
	gameOptions [0].menus.altBg.grayscale = NumArg (t, 1);
if ((t = FindArg ("-altbg_name")) && *appConfig [t+1])
	strncpy (gameOptions [0].menus.altBg.szName [0], appConfig [t+1], sizeof (gameOptions [0].menus.altBg.szName [0]));
if ((t = FindArg ("-use_swapfile")))
	gameStates.app.bUseSwapFile = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalGameplayArgs (void)
{
	int	t;

if ((t = FindArg ("-briefings")))
	gameOpts->gameplay.bSkipBriefingScreens = !NumArg (t, 1);
if ((t = FindArg ("-noscreens")))
	gameOpts->gameplay.bSkipBriefingScreens = NumArg (t, 1);
if ((t = FindArg ("-secretsave")))
	gameOptions [0].gameplay.bSecretSave = NumArg (t, 1);
if ((t = FindArg ("-disable_robots")) && NumArg (t, 1))
	gameStates.app.bGameSuspended |= SUSP_ROBOTS;
if ((t = FindArg ("-disable_powerups")) && NumArg (t, 1))
	gameStates.app.bGameSuspended |= SUSP_POWERUPS;
}

// ----------------------------------------------------------------------------

void EvalInputArgs (void)
{
	int	t;

if ((t = FindArg ("-grabmouse")))
	gameStates.input.bGrabMouse = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalOglArgs (void)
{
	int	t;

#if DBG
if ((t = FindArg ("-gl_alttexmerge")))
	gameOpts->ogl.bGlTexMerge = NumArg (t, 1);
#endif
if ((t = FindArg ("-lowmem")))
	ogl.m_states.bLowMemory = NumArg (t, 1);
if ((t = FindArg ("-preload_textures")))
	ogl.m_states.nPreloadTextures = NumArg (t, 3);
else
	ogl.m_states.nPreloadTextures = 5;
if ((t = FindArg ("-FSAA")))
	ogl.m_states.bFSAA = NumArg (t, 1);
if ((t = FindArg ("-quad_buffering")))
	ogl.m_features.bQuadBuffers = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalDemoArgs (void)
{
	int	t;

if ((t = FindArg ("-revert_demos")))
	gameOpts->demo.bRevertFormat = NumArg (t, 1);
if ((t = FindArg ("-auto_demos")))
	gameStates.app.bAutoDemos = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalRenderArgs (void)
{
	int	t;

if ((t = FindArg ("-hires_textures")))
	gameOptions [0].render.textures.bUseHires [0] =
	gameOptions [0].render.textures.bUseHires [1] = NumArg (t, 1);
if ((t = FindArg ("-hires_models")))
	gameOptions [0].render.bHiresModels [0] =
	gameOptions [0].render.bHiresModels [1] = NumArg (t, 1);
if ((t = FindArg ("-model_quality")) && *appConfig [t+1])
	gameStates.render.nModelQuality = NumArg (t, 3);
#if 0
if ((t = FindArg ("-gl_texcompress")))
	ogl.m_features.bTextureCompression.Apply (NumArg (t, 1));
#endif
gameOptions [0].render.bUseShaders = 1;
gameStates.app.bReadOnly = 0;
gameStates.app.bCacheTextures = 1;
gameStates.app.bCacheModelData = 1;
gameStates.app.bCacheMeshes = 1;
gameStates.app.bCacheLightmaps = 1;
gameStates.app.bCacheLights = 1;
#if 1 //DBG
if ((t = FindArg ("-readonly"))) {
	gameStates.app.bReadOnly = 1;
	gameStates.app.bCacheTextures = 0;
	gameStates.app.bCacheModelData = 0;
	gameStates.app.bCacheMeshes = 0;
	gameStates.app.bCacheLightmaps = 0;
	gameStates.app.bCacheLights = 0;
	}
#else
	gameStates.app.bCacheTextures = NumArg (t, 1);
if ((t = FindArg ("-cache_textures")))
	gameStates.app.bCacheTextures = NumArg (t, 1);
if ((t = FindArg ("-cache_models")))
	gameStates.app.bCacheModelData = NumArg (t, 1);
if ((t = FindArg ("-cache_meshes")))
	gameStates.app.bCacheMeshes = NumArg (t, 1);
if ((t = FindArg ("-cache_lightmaps")))
	gameStates.app.bCacheLightmaps = NumArg (t, 1);
if ((t = FindArg ("-cache_lights")))
	gameStates.app.bCacheLights = NumArg (t, 1);
#endif
if ((t = FindArg ("-use_shaders")))
	gameOptions [0].render.bUseShaders = NumArg (t, 1);
if ((t = FindArg ("-enable_freecam")))
	gameStates.render.bEnableFreeCam = NumArg (t, 1);
if ((t = FindArg ("-oculus_rift")))
	gameOptions [0].render.bUseRift = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalShipArgs (void)
{
	const char*	szShipArgs [] = {"-medium_ship", "-light_ship", "-heavy_ship"};
	int	t;
	char	*p;

for (int i = 0; i < MAX_SHIP_TYPES; i++) {
	if ((t = FindArg (szShipArgs [i])) && (p = appConfig [t+1]) && *p) {
		strncpy (gameData.models.szShipModels [i], appConfig [t+1], FILENAME_LEN);
		strlwr (gameData.models.szShipModels [i]);
		replacementModels [i].pszHires = gameData.models.szShipModels [i];
		}
	}
}

// ----------------------------------------------------------------------------

void EvalAppArgs (void)
{
	int	t;

#if 0
if ((t = FindArg ("-gpgpu_lights")))
	ogl.m_states.bVertexLighting = NumArg (t, 1);
#endif
#ifdef __unix__
if ((t = FindArg ("-linux_msgbox")))
	gameStates.app.bLinuxMsgBox = NumArg (t, 1);
#endif
if ((t = FindArg ("-show_version_info")))
	gameStates.app.bShowVersionInfo = NumArg (t, 1);
if ((t = FindArg ("-check_setup")))
	gameStates.app.bCheckAndFixSetup = NumArg (t, 1);
#if 0
if ((t = FindArg ("-expertmode")))
	gameOpts->app.bExpertMode = NumArg (t, 1);
#endif
if ((t = FindArg ("-pured2")))
	SetNostalgia (3);
else if ((t = FindArg ("-nostalgia")))
	SetNostalgia (appConfig [t+1] ? NumArg (t, 0) : 1);
else
	SetNostalgia (0);

if (!gameStates.app.bNostalgia && (t = FindArg ("-standalone")))
	gameStates.app.bStandalone = NumArg (t, 1);
else
	gameStates.app.bStandalone = 0;

#ifdef _OPENMP //MULTI_THREADED
if ((t = FindArg ("-multithreaded"))) {
	gameStates.app.nThreads = NumArg (t, 1);
	gameStates.app.bMultiThreaded = (gameStates.app.nThreads > 0);
	if (gameStates.app.nThreads == 0)
		gameStates.app.nThreads = 1;
	else if (gameStates.app.nThreads == 1)
		gameStates.app.nThreads = MAX_THREADS;
	else if (gameStates.app.nThreads > MAX_THREADS)
		gameStates.app.nThreads = MAX_THREADS;
	}
else 
#else
	{
	gameStates.app.nThreads = 1;
	gameStates.app.bMultiThreaded = 0;
	}
#endif
if ((t = FindArg ("-nosound")))
	gameStates.app.bUseSound = (NumArg (t, 1) == 0);
if ((t = FindArg ("-progress_bars")))
	gameStates.app.bProgressBars = NumArg (t, 1);
if ((t = FindArg ("-altLanguage")))
	gameStates.app.bEnglish = (NumArg (t, 1) == 0);

if ((t = FindArg ("-auto_hogfile"))) {
	strcpy (szAutoHogFile, "missions/");
	strcat (szAutoHogFile, appConfig [t+1]);
	if (*szAutoHogFile && !strchr (szAutoHogFile, '.'))
		strcat (szAutoHogFile, ".hog");
	}
if ((t = FindArg ("-auto_mission"))) {
	char	c = *appConfig [++t];
	int		bDelim = ((c == '\'') || (c == '"'));

	strcpy (szAutoMission, &appConfig [t][bDelim]);
	if (bDelim)
		szAutoMission [strlen (szAutoMission) - 1] = '\0';
	if (*szAutoMission && !strchr (szAutoMission, '.'))
		strcat (szAutoMission, ".rl2");
	}
if (FindArg ("-debug"))
	CCvar::Register (const_cast<char*>("con_threshold"), 2.0);
else if (FindArg ("-verbose"))
	CCvar::Register (const_cast<char*>("con_threshold"), 1.0);
else
	CCvar::Register (const_cast<char*>("con_threshold"), -1.0);
if ((t = FindArg ("-autodemo"))) {
	gameData.demo.bAuto = 1;
	strncpy (gameData.demo.fnAuto, *appConfig [t+1] ? appConfig [t+1] : "descent.dem", sizeof (gameData.demo.fnAuto));
	}
else
	gameData.demo.bAuto = 0;
gameStates.app.bMacData = FindArg ("-macdata");
if ((t = FindArg ("-compress_data")))
	gameStates.app.bCompressData = (NumArg (t, 1) == 1);
else
	gameStates.app.bCompressData = 0;
if (gameStates.app.bNostalgia)
	gameData.segs.nMaxSegments = MAX_SEGMENTS_D2;
}

// ----------------------------------------------------------------------------

void EvalArgs (void)
{
EvalAppArgs ();
EvalGameplayArgs ();
EvalInputArgs ();
EvalMultiplayerArgs ();
EvalMenuArgs ();
EvalMovieArgs ();
EvalOglArgs ();
EvalRenderArgs ();
EvalSoundArgs ();
EvalMusicArgs ();
EvalDemoArgs ();
EvalShipArgs ();
}

// ----------------------------------------------------------------------------
//eof
