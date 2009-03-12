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
#include "playsave.h"
#include "tracker.h"
#include "rendermine.h"
#include "sphere.h"
#include "endlevel.h"
#include "interp.h"
#include "autodl.h"
#include "hiresmodels.h"
#include "soundthreads.h"
#include "songs.h"
#include "lightcluster.h"

void SetMaxPitch (int nMinTurnRate);
void PrintVersion (void);

// ----------------------------------------------------------------------------

void EvalLegacyArgs (void)
{
#ifdef DEBUG	// it is strongly discouraged to use these switches!
	int bLegacy = FindArg ("-legacymode");

if (bLegacy || FindArg ("-legacyinput"))
	gameOptions [0].legacy.bInput = 1;
if (bLegacy || FindArg ("-legacyfuelcens"))
	gameOptions [0].legacy.bFuelCens = 1;
if (bLegacy || FindArg ("-legacymouse"))
	gameOptions [0].legacy.bMouse = 1;
if (bLegacy || FindArg ("-legacyhomers"))
	gameOptions [0].legacy.bHomers = 1;
if (bLegacy || FindArg ("-legacyrender"))
	gameOptions [0].legacy.bRender = 1;
if (bLegacy || FindArg ("-legacyswitches"))
	gameOptions [0].legacy.bSwitches = 1;
if (bLegacy || FindArg ("-legacywalls"))
	gameOptions [0].legacy.bWalls = 1;
#endif
}

// ----------------------------------------------------------------------------

void EvalAutoNetGameArgs (void)
{
	int	t, bHaveIp = 0;
	char	*p;

	static const char *pszTypes [] = {"anarchy", "coop", "ctf", "ctf+", "hoard", "entropy", NULL};
	static const char	*pszConnect [] = {"ipx", "udp", "", "multicast", NULL};

memset (&gameData.multiplayer.autoNG, 0, sizeof (gameData.multiplayer.autoNG));
if ((t = FindArg ("-ng_player")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szPlayer, pszArgList [t+1], 8);
	gameData.multiplayer.autoNG.szPlayer [8] = '\0';
	}
if ((t = FindArg ("-ng_file")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szFile, pszArgList [t+1], FILENAME_LEN - 1);
	gameData.multiplayer.autoNG.szFile [FILENAME_LEN - 1] = '\0';
	}
if ((t = FindArg ("-ng_mission")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szMission, pszArgList [t+1], 12);
	gameData.multiplayer.autoNG.szMission [12] = '\0';
	}
if ((t = FindArg ("-ngLevel")))
	gameData.multiplayer.autoNG.nLevel = NumArg (t, 1);
else
	gameData.multiplayer.autoNG.nLevel = 1;
if ((t = FindArg ("-ng_name")) && (p = pszArgList [t+1])) {
	strncpy (gameData.multiplayer.autoNG.szName, pszArgList [t+1], 80);
	gameData.multiplayer.autoNG.szName [80] = '\0';
	}
if ((t = FindArg ("-ng_ipaddr")) && (p = pszArgList [t+1]))
	bHaveIp = stoip (pszArgList [t+1], gameData.multiplayer.autoNG.ipAddr);
if ((t = FindArg ("-ng_connect")) && (p = pszArgList [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (*pszConnect [t] && !strcmp (p, pszConnect [t])) {
			gameData.multiplayer.autoNG.uConnect = t;
			break;
			}
	}
if ((t = FindArg ("-ng_join")) && (p = pszArgList [t+1])) {
	strlwr (p);
	gameData.multiplayer.autoNG.bHost = !strcmp (p, "host");
	}
if ((t = FindArg ("-ngType")) && (p = pszArgList [t+1])) {
	strlwr (p);
	for (t = 0; pszTypes [t]; t++)
		if (!strcmp (p, pszTypes [t])) {
			gameData.multiplayer.autoNG.uType = t;
			break;
			}
	}
if ((t = FindArg ("-ng_team")))
	gameData.multiplayer.autoNG.bTeam = NumArg (t, 1);
if (gameData.multiplayer.autoNG.bHost)
	gameData.multiplayer.autoNG.bValid = 
		*gameData.multiplayer.autoNG.szPlayer &&
		*gameData.multiplayer.autoNG.szName &&
		*gameData.multiplayer.autoNG.szFile &&
		*gameData.multiplayer.autoNG.szMission;
else
	gameData.multiplayer.autoNG.bValid = 
		*gameData.multiplayer.autoNG.szPlayer &&
		bHaveIp;
if (gameData.multiplayer.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
}

// ----------------------------------------------------------------------------

void EvalMultiplayerArgs (void)
{
	int t;

if ((t = FindArg ("-friendlyfire")))
	extraGameInfo [0].bFriendlyFire = NumArg (t, 1);
if ((t = FindArg ("-fixedrespawns")))
	extraGameInfo [0].bFixedRespawns = NumArg (t, 0);
if ((t = FindArg ("-immortalpowerups")))
	extraGameInfo [0].bImmortalPowerups = NumArg (t, 0);
if ((t = FindArg ("-spawndelay"))) {
	extraGameInfo [0].nSpawnDelay = NumArg (t, 0);
	if (extraGameInfo [0].nSpawnDelay < 0)
		extraGameInfo [0].nSpawnDelay = 0;
	else if (extraGameInfo [0].nSpawnDelay > 60)
		extraGameInfo [0].nSpawnDelay = 60;
	extraGameInfo [0].nSpawnDelay *= 1000;
	}
if ((t = FindArg ("-pps"))) {
	mpParams.nPPS = atoi(pszArgList [t+1]);
	if (mpParams.nPPS < 1)
		mpParams.nPPS = 1;
	else if (mpParams.nPPS > 20)
		mpParams.nPPS = 20;
	}
if (FindArg ("-shortpackets"))
	mpParams.bShortPackets = 1;
if (FindArg ("-norankings"))
	gameOptions [0].multi.bNoRankings = 1;
if ((t = FindArg ("-timeout")))
	gameOptions [0].multi.bTimeoutPlayers = NumArg (t, 1);
if ((t = FindArg ("-noredundancy")))
	gameOptions [0].multi.bNoRedundancy = NumArg (t, 1);
EvalAutoNetGameArgs ();
}

// ----------------------------------------------------------------------------

void EvalMovieArgs (void)
{
	int	t;

if ((t = FindArg ("-nomovies"))) {
	gameOptions [0].movies.nLevel = 2 - NumArg (t, 2);
	if (gameOptions [0].movies.nLevel < 0)
		gameOptions [0].movies.nLevel = 0;
	else if (gameOptions [0].movies.nLevel > 2)
		gameOptions [0].movies.nLevel = 2;
	}
if (FindArg ("-subtitles"))
	gameOptions [0].movies.bSubTitles = 1;
if ((t = FindArg ("-movie_quality")))
	gameOptions [0].movies.nQuality = NumArg (t, 0);
if (FindArg ("-lowresmovies"))
	gameOptions [0].movies.bHires = 0;
if (gameData.multiplayer.autoNG.bValid)
	gameOptions [0].movies.nLevel = 0;
if (FindArg ("-mac_movies"))
	gameStates.movies.bMac = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalSoundArgs (void)
{
	int	t;

if (FindArg ("-sound44k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_44K;
else if (FindArg ("-sound22k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_22K;
else if (FindArg ("-sound11k"))
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_11K;
else
	gameOptions [0].sound.digiSampleRate = SAMPLE_RATE_22K;
#if USE_SDL_MIXER
if ((t = FindArg ("-sdl_mixer")))
	gameOptions [0].sound.bUseSDLMixer = NumArg (t, 1);
#endif
#ifdef __macosx__
	void * volatile function_p = (void *)&(Mix_OpenAudio);
	if (function_p == NULL) {
	
		// the SDL_mixer framework is not present,
		// so regardless of what conf.h or d2x.ini says,
		// we don't want to use SDL_mixer
		
		gameOptions [0].sound.bUseSDLMixer = 0;
	}
#endif
if ((t = FindArg ("-use_d1sounds")))
	gameOptions [0].sound.bUseD1Sounds = NumArg (t, 1);
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

if ((t = FindArg ("-playlist")) && (p = pszArgList [t+1]))
	songManager.LoadPlayList (p);
if ((t = FindArg ("-introsong")) && (p = pszArgList [t+1]))
	strncpy (songManager.IntroSong (), p, FILENAME_LEN);
if ((t = FindArg ("-briefingsong")) && (p = pszArgList [t+1]))
	strncpy (songManager.BriefingSong (), p, FILENAME_LEN);
if ((t = FindArg ("-creditssong")) && (p = pszArgList [t+1]))
	strncpy (songManager.CreditsSong (), p, FILENAME_LEN);
if ((t = FindArg ("-menusong")) && (p = pszArgList [t+1]))
	strncpy (songManager.MenuSong (), p, FILENAME_LEN);
}

// ----------------------------------------------------------------------------

void EvalMenuArgs (void)
{
	int	t;

if (!gameStates.app.bNostalgia)
	if ((t = FindArg ("-menustyle")))
		gameOptions [0].menus.nStyle = NumArg (t, 1);
if ((t = FindArg ("-fademenus")))
	gameOptions [0].menus.nFade = NumArg (t, 0);
if ((t = FindArg ("-fastmenus")))
	gameOptions [0].menus.bFastMenus = 1; //NumArg (t, 1);
if ((t = FindArg ("-altbg_alpha")) && *pszArgList [t+1]) {
	gameOptions [0].menus.altBg.alpha = atof (pszArgList [t+1]);
	if (gameOptions [0].menus.altBg.alpha < 0)
		gameOptions [0].menus.altBg.alpha = -1.0;
	else if ((gameOptions [0].menus.altBg.alpha == 0) || (gameOptions [0].menus.altBg.alpha > 1.0))
		gameOptions [0].menus.altBg.alpha = 1.0;
	}
if ((t = FindArg ("-altbg_brightness")) && *pszArgList [t+1]) {
	gameOptions [0].menus.altBg.brightness = atof (pszArgList [t+1]);
	if ((gameOptions [0].menus.altBg.brightness <= 0) || (gameOptions [0].menus.altBg.brightness > 1.0))
		gameOptions [0].menus.altBg.brightness = 1.0;
	}
if ((t = FindArg ("-altbg_grayscale")))
	gameOptions [0].menus.altBg.grayscale = NumArg (t, 1);
if ((t = FindArg ("-altbg_name")) && *pszArgList [t+1])
	strncpy (gameOptions [0].menus.altBg.szName, pszArgList [t+1], sizeof (gameOptions [0].menus.altBg.szName));
if ((t = FindArg ("-menu_hotkeys")))
	gameOptions [0].menus.nHotKeys = NumArg (t, 1);
if ((t = FindArg ("-use_swapfile")))
	gameStates.app.bUseSwapFile = NumArg (t, 1);
else if (gameStates.app.bEnglish)
	gameOptions [0].menus.nHotKeys = 1;
}

// ----------------------------------------------------------------------------

void EvalGameplayArgs (void)
{
	int	t;

if ((t = FindArg ("-noscreens")))
	gameOpts->gameplay.bSkipBriefingScreens = NumArg (t, 1);
if ((t = FindArg ("-secretsave")))
	gameOptions [0].gameplay.bSecretSave = NumArg (t, 1);
if ((t = FindArg ("-nobotai")))
	gameStates.gameplay.bNoBotAI = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalInputArgs (void)
{
	int	t, i;

if ((t = FindArg ("-mouselook")))
	extraGameInfo [0].bMouseLook = NumArg (t, 1);
if ((t = FindArg ("-limitturnrate")))
	gameOptions [0].input.bLimitTurnRate = NumArg (t, 1);
#if DBG
if ((t = FindArg ("-minturnrate")))
	gameOptions [0].input.nMinTurnRate = NumArg (t, 20);
#endif
SetMaxPitch (gameOptions [0].input.nMinTurnRate);
if ((t = FindArg ("-joydeadzone")))
	for (i = 0; i < 4; i++)
		JoySetDeadzone (NumArg (t, 0), i);
if ((t = FindArg ("-grabmouse")))
	gameStates.input.bGrabMouse = NumArg (t, 1);
}

// ----------------------------------------------------------------------------

void EvalOglArgs (void)
{
	int	t;

if ((t = FindArg ("-gl_alttexmerge")))
	gameOpts->ogl.bGlTexMerge = NumArg (t, 1);
if ((t = FindArg ("-gl_16bittextures"))) {
	gameStates.ogl.bpp = 16;
	gameStates.ogl.nRGBAFormat = GL_RGBA4;
	gameStates.ogl.nRGBFormat = GL_RGB5;
	}
if ((t=FindArg ("-gl_intensity4_ok")))
	gameStates.ogl.bIntensity4 = NumArg (t, 1);
if ((t = FindArg ("-gl_luminance4_alpha4_ok")))
	gameStates.ogl.bLuminance4Alpha4 = NumArg (t, 1);
if ((t = FindArg ("-gl_readpixels_ok")))
	gameStates.ogl.bReadPixels = NumArg (t, 1);
if ((t = FindArg ("-gl_gettexlevelparam_ok")))
	gameStates.ogl.bGetTexLevelParam = NumArg (t, 1);
if ((t = FindArg ("-lowmem")))
	gameStates.ogl.bLowMemory = NumArg (t, 1);
if ((t = FindArg ("-preload_textures")))
	gameStates.ogl.nPreloadTextures = NumArg (t, 3);
else
	gameStates.ogl.nPreloadTextures = 3;
if ((t = FindArg ("-FSAA")))
	gameStates.ogl.bFSAA = NumArg (t, 1);

#ifdef GL_ARB_multitexture
if (t = FindArg ("-gl_arb_multitexture_ok")))
	gameStates.ogl.bArbMultiTexture = NumArg (t, 1);
#endif
#ifdef GL_SGIS_multitexture
if (t = FindArg ("-gl_sgis_multitexture_ok")))
	gameStates.ogl.bSgisMultiTexture = NumArg (t, 1);
#endif
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

if ((t = FindArg ("-render_quality")) && *pszArgList [t+1]) {
	gameOpts->render.nQuality = NumArg (t, 3);
	if (gameOpts->render.nQuality < 0)
		gameOpts->render.nQuality = 0;
	else if (gameOpts->render.nQuality > 3)
		gameOpts->render.nQuality = 3;
	}
#if FBO_DRAW_BUFFER
if ((t = FindArg ("-render_indirect")))
	gameStates.render.bRenderIndirect = NumArg (t, 1);
#endif
if ((t = FindArg ("-use_shaders")))
	gameOptions [0].render.bUseShaders = NumArg (t, 1);
if ((t = FindArg ("-shadows")))
	extraGameInfo [0].bShadows = NumArg (t, 0);
if ((t = FindArg ("-hires_textures")))
	gameOptions [0].render.textures.bUseHires [0] =
	gameOptions [0].render.textures.bUseHires [1] = NumArg (t, 1);
if ((t = FindArg ("-hires_models")))
	gameOptions [0].render.bHiresModels [0] = 
	gameOptions [0].render.bHiresModels [1] = NumArg (t, 1);
if ((t = FindArg ("-render_all_segs")))
	gameOptions [0].render.bAllSegs = NumArg (t, 1);
if ((t = FindArg ("-enable_lightmaps")))
	gameStates.render.color.bLightmapsOk = NumArg (t, 1);
if ((t = FindArg ("-blend_background")))
	gameStates.render.bBlendBackground = NumArg (t, 1);
if ((t = FindArg ("-maxfps"))) {
	t = NumArg (t, 250);
	if (t <= 0)
		t = 1;
	else if (t > 250)
		t = 250;
	gameOpts->render.nMaxFPS = t;
	}
if ((t = FindArg ("-usePerPixelLighting")))
	gameStates.render.bUsePerPixelLighting = NumArg (t, 1);
#if RENDER2TEXTURE
if ((t = FindArg ("-render2texture")))
	gameStates.ogl.bUseRender2Texture = NumArg (t, 1);
#endif
#if OGL_POINT_SPRITES
if ((t = FindArg ("-point_sprites")))
	gameStates.render.bPointSprites = NumArg (t, 1);
#endif
if ((t = FindArg ("-nofade")))
	gameStates.render.bDisableFades = NumArg (t, 1);
if ((t = FindArg ("-mathformat")))
	gameOpts->render.nDefMathFormat = NumArg (t, 2);
#if defined (_WIN32) || defined (__unix__)
if ((t = FindArg ("-enable_sse")))
	gameStates.render.bEnableSSE = NumArg (t, 1);
#endif
#if DBG
if ((t = FindArg ("-gl_transform")))
	gameStates.ogl.bUseTransform = NumArg (t, 1);
#endif
if ((t = FindArg ("-cache_textures")))
	gameStates.app.bCacheTextures = NumArg (t, 1);
if ((t = FindArg ("-cache_models")))
	gameStates.app.bCacheModelData = NumArg (t, 1);
if ((t = FindArg ("-model_quality")) && *pszArgList [t+1])
	gameStates.render.nModelQuality = NumArg (t, 3);
#if 0
if ((t = FindArg ("-gl_texcompress")))
	gameStates.ogl.bTextureCompression = NumArg (t, 1);
#endif
if ((t = FindArg ("-cache_meshes")))
	gameStates.app.bCacheMeshes = NumArg (t, 1);
if ((t = FindArg ("-cache_lightmaps")))
	gameStates.app.bCacheLightmaps = NumArg (t, 1);
#if 0
if ((t = FindArg ("-renderpath")))
	gameOptions [0].render.nPath = NumArg (t, 1);
#endif
if ((t = FindArg ("-split_polys")))
	gameStates.render.bSplitPolys = NumArg (t, 1);
if ((t = FindArg ("-cluster_lights")))
	lightClusterManager.SetUsage (NumArg (t, 1) != 0);
}

// ----------------------------------------------------------------------------

void EvalAppArgs (void)
{
	int	t;

if ((t = FindArg ("-max_segments")) && *pszArgList [t+1]) {
	t = NumArg (t, MAX_SEGMENTS_D2X);
	if (t < MAX_SEGMENTS_D2)
		t = MAX_SEGMENTS_D2;
	else if (t > MAX_SEGMENTS_D2X)
		t = MAX_SEGMENTS_D2X;
	gameData.segs.nMaxSegments = t;
	}
#if 0
if ((t = FindArg ("-gpgpu_lights")))
	gameStates.ogl.bVertexLighting = NumArg (t, 1);
#endif
if ((t = FindArg ("-cache_lights")))
	gameStates.app.bCacheLights = NumArg (t, 1);
if ((t = FindArg ("-enable_shadows")))
	gameStates.app.bEnableShadows = NumArg (t, 1);
if ((t = FindArg ("-enable_freecam")))
	gameStates.app.bEnableFreeCam = NumArg (t, 1);
if ((t = FindArg ("-pured2")))
	SetNostalgia (3);
else if ((t = FindArg ("-nostalgia")))
	SetNostalgia (pszArgList [t+1] ? NumArg (t, 0) : 1);
else
	SetNostalgia (0);

#if 1 //MULTI_THREADED
if ((t = FindArg ("-multithreaded")))
	gameStates.app.bMultiThreaded = NumArg (t, 1);
#endif
if ((t = FindArg ("-nosound")))
	gameStates.app.bUseSound = (NumArg (t, 1) == 0);
if ((t = FindArg ("-progress_bars")))
	gameStates.app.bProgressBars = NumArg (t, 1);
if ((t = FindArg ("-fix_models")))
	gameStates.app.bFixModels = NumArg (t, 1);
if ((t = FindArg ("-alt_models")))
	gameStates.app.bAltModels = NumArg (t, 1);
if ((t = FindArg ("-lores_shadows")))
	gameStates.render.bLoResShadows = NumArg (t, 1);
if ((t = FindArg ("-printVersion")))
	PrintVersion ();
if ((t = FindArg ("-altLanguage")))
	gameStates.app.bEnglish = (NumArg (t, 1) == 0);

if ((t = FindArg ("-auto_hogfile"))) {
	strcpy (szAutoHogFile, "missions/");
	strcat (szAutoHogFile, pszArgList [t+1]);
	if (*szAutoHogFile && !strchr (szAutoHogFile, '.'))
		strcat (szAutoHogFile, ".hog");
	}
if ((t = FindArg ("-auto_mission"))) {
	char	c = *pszArgList [++t];
	int		bDelim = ((c == '\'') || (c == '"'));

	strcpy (szAutoMission, &pszArgList [t][bDelim]);
	if (bDelim)
		szAutoMission [strlen (szAutoMission) - 1] = '\0';
	if (*szAutoMission && !strchr (szAutoMission, '.'))
		strcat (szAutoMission, ".rl2");
	}
if (FindArg ("-debug"))
	CCvar::Register ("con_threshold", "2.0");
else if (FindArg ("-verbose"))
	CCvar::Register ("con_threshold", "1.0");
else
	CCvar::Register ("con_threshold", "-1.0");
if ((t = FindArg ("-autodemo"))) {
	gameData.demo.bAuto = 1;
	strncpy (gameData.demo.fnAuto, *pszArgList [t+1] ? pszArgList [t+1] : "descent.dem", sizeof (gameData.demo.fnAuto));
	}
else
	gameData.demo.bAuto = 0;
gameStates.app.bMacData = FindArg ("-macdata");
if (gameStates.app.bNostalgia)
	gameData.segs.nMaxSegments = MAX_SEGMENTS_D2;
}

// ----------------------------------------------------------------------------

void EvalArgs (void)
{
EvalAppArgs ();
EvalGameplayArgs ();
EvalLegacyArgs ();
EvalInputArgs ();
EvalMultiplayerArgs ();
EvalMenuArgs ();
EvalMovieArgs ();
EvalOglArgs ();
EvalRenderArgs ();
EvalSoundArgs ();
EvalMusicArgs ();
EvalDemoArgs ();
}

// ----------------------------------------------------------------------------
//eof
