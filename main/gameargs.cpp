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

void PrintVersion (void);

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
if (FindArg ("-norankings"))
	gameOptions [0].multi.bNoRankings = 1;
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
	gameOptions [0].movies.bSubTitles = NumArg (t, 1);
if ((t = FindArg ("-movie_quality")))
	gameOptions [0].movies.nQuality = NumArg (t, 0);
if (gameData.multiplayer.autoNG.bValid)
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

if (!gameStates.app.bNostalgia) {
	if ((t = FindArg ("-menustyle")))
		gameOptions [0].menus.nStyle = NumArg (t, 1);
	else
		gameOptions [0].menus.nStyle = 1;
	}
if ((t = FindArg ("-fademenus")))
	gameOptions [0].menus.nFade = NumArg (t, 0);
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
if ((t = FindArg ("-use_swapfile")))
	gameStates.app.bUseSwapFile = NumArg (t, 1);
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

#ifdef GL_ARB_multitexture
if (t = FindArg ("-gl_arb_multitexture_ok")))
	ogl.m_states.bArbMultiTexture = NumArg (t, 1);
#endif
#ifdef GL_SGIS_multitexture
if (t = FindArg ("-gl_sgis_multitexture_ok")))
	ogl.m_states.bSgisMultiTexture = NumArg (t, 1);
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

#if FBO_DRAW_BUFFER
if ((t = FindArg ("-render_indirect")))
	gameStates.render.bRenderIndirect = NumArg (t, 1);
#endif
if ((t = FindArg ("-hires_textures")))
	gameOptions [0].render.textures.bUseHires [0] =
	gameOptions [0].render.textures.bUseHires [1] = NumArg (t, 1);
if ((t = FindArg ("-hires_models")))
	gameOptions [0].render.bHiresModels [0] = 
	gameOptions [0].render.bHiresModels [1] = NumArg (t, 1);
if ((t = FindArg ("-model_quality")) && *pszArgList [t+1])
	gameStates.render.nModelQuality = NumArg (t, 3);
#if 0
if ((t = FindArg ("-gl_texcompress")))
	ogl.m_states.bTextureCompression = NumArg (t, 1);
#endif
#if 0 //DBG
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
#else
#	if 1 //DBG
if ((t = FindArg ("-use_shaders")))
	gameOptions [0].render.bUseShaders = NumArg (t, 1);
else
#	endif
gameOptions [0].render.bUseShaders = 1;
gameStates.app.bCacheTextures = 1;
gameStates.app.bCacheModelData = 1;
gameStates.app.bCacheMeshes = 1;
gameStates.app.bCacheLightmaps = 1; 
gameStates.app.bCacheLights = 1;
#endif
}

// ----------------------------------------------------------------------------

void EvalAppArgs (void)
{
	int	t;

#if 0
if ((t = FindArg ("-gpgpu_lights")))
	ogl.m_states.bVertexLighting = NumArg (t, 1);
#endif
if ((t = FindArg ("-expertmode")))
	gameOpts->app.bExpertMode = NumArg (t, 1);
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
