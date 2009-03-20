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

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "descent.h"
#include "ipx.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"
#include "joy.h"
#include "slew.h"
#include "args.h"
#include "hogfile.h"
#include "newdemo.h"
#include "timer.h"
#include "text.h"
#include "gamefont.h"
#include "menu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
#include "movie.h"
#include "gamepal.h"
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "rendermine.h"
#include "light.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#include "soundthreads.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

void DefaultRenderSettings (void);

//------------------------------------------------------------------------------

static struct {
	int	nFrameCap;
	int	nRenderQual;
	int	nTexQual;
	int	nMeshQual;
	int	nCoronas;
	int	nLightning;
	int	nSmoke;
	int	nShadows;
	int	nPowerups;
	int	nLighting;
	int	nLightmaps;
	int	nCameras;
	int	nLights;
	int	nPasses;
	int	nWallTransp;
	int	nContrast;
	int	nBrightness;
} renderOpts;

#if DBG || !SIMPLE_MENUS
static int fpsTable [16] = {-1, 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 200, 250};
#else
static int fpsTable [2] = {0, 60};
#endif

static const char *pszTexQual [4];
static const char *pszMeshQual [5];
static const char *pszRendQual [5];

//------------------------------------------------------------------------------

static inline const char *ContrastText (void)
{
return (gameStates.ogl.nContrast == 8) ? TXT_STANDARD : 
		 (gameStates.ogl.nContrast < 8) ? TXT_LOW : 
		 TXT_HIGH;
}

//------------------------------------------------------------------------------

int FindTableFps (int nFps)
{
	int	i, j = 0, d, dMin = 0x7fffffff;

for (i = 0; i < sizeofa (fpsTable); i++) {
	d = abs (nFps - fpsTable [i]);
	if (d < dMin) {
		j = i;
		dMin = d;
		}
	}
return j;
}

//------------------------------------------------------------------------------

int AdvancedRenderOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

if (renderOpts.nContrast >= 0) {
	m = menu + renderOpts.nContrast;
	v = m->m_value;
	if (v != gameStates.ogl.nContrast) {
		gameStates.ogl.nContrast = v;
		sprintf (m->m_text, TXT_CONTRAST, ContrastText ());
		m->m_bRebuild = 1;
		}
	}
if (gameOpts->app.bExpertMode) {
	m = menu + renderOpts.nRenderQual;
	v = m->m_value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->m_text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->m_bRebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menu + renderOpts.nTexQual;
		v = m->m_value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->m_text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->m_bRebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menu + renderOpts.nMeshQual;
		v = m->m_value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->m_text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->m_bRebuild = 1;
			}
		}
	m = menu + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->m_value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->m_text, TXT_WALL_TRANSP, m->m_value * 10, '%');
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

#if SIMPLE_MENUS

static int nCoronas, nSmoke, nShadows, nPowerups, nCameras;

static const char* pszNoneBasicAdv [3];
static const char* pszNoneBasicFull [3];

int RenderOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menu + renderOpts.nBrightness;
	v = m->m_value;
	if (v != paletteManager.GetGamma ())
		paletteManager.SetGamma (v);
	}

if (0 && gameOpts->app.bExpertMode) {
	m = menu + renderOpts.nFrameCap;
	v = fpsTable [m->m_value];
	if (gameOpts->render.nMaxFPS != v) {
		if (v > 0)
			sprintf (m->m_text, TXT_FRAMECAP, v);
		else if (v < 0) {
			if (!gameStates.render.bVSyncOk) {
				m->m_value = 1;
				return nCurItem;
				}
			sprintf (m->m_text, TXT_VSYNC);
			}
		else
			sprintf (m->m_text, TXT_NO_FRAMECAP);
#if WIN32
		if (gameStates.render.bVSyncOk)
			wglSwapIntervalEXT (v < 0);
#endif
		gameOpts->render.nMaxFPS = v;
		gameStates.render.bVSync = (v < 0);
		m->m_bRebuild = 1;
		}
	}

m = menu + renderOpts.nRenderQual;
v = m->m_value;
if (gameOpts->render.nQuality != v) {
	gameOpts->render.nQuality = v;
	sprintf (m->m_text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
	m->m_bRebuild = 1;
	}

if (renderOpts.nTexQual > 0) {
	m = menu + renderOpts.nTexQual;
	v = m->m_value;
	if (gameOpts->render.textures.nQuality != v) {
		gameOpts->render.textures.nQuality = v;
		sprintf (m->m_text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
		m->m_bRebuild = 1;
		}
	}

if (renderOpts.nMeshQual > 0) {
	m = menu + renderOpts.nMeshQual;
	v = m->m_value;
	if (gameOpts->render.nMeshQuality != v) {
		gameOpts->render.nMeshQuality = v;
		sprintf (m->m_text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
		m->m_bRebuild = 1;
		}
	}

m = menu + renderOpts.nSmoke;
v = m->m_value;
if (nSmoke != v) {
	nSmoke = v;
	sprintf (m->m_text, TXT_SMOKE, pszNoneBasicFull [nSmoke]);
	m->m_bRebuild = -1;
	}

if (renderOpts.nShadows >= 0) {
	m = menu + renderOpts.nShadows;
	v = m->m_value;
	if (nShadows != v) {
		nShadows = v;
		sprintf (m->m_text, TXT_SHADOWS, pszNoneBasicFull [nShadows]);
		m->m_bRebuild = -1;
		}
	}

m = menu + renderOpts.nLightning;
v = m->m_value;
if (extraGameInfo [0].bUseLightning != v) {
	extraGameInfo [0].bUseLightning = v;
	sprintf (m->m_text, TXT_LIGHTNING, pszNoneBasicFull [extraGameInfo [0].bUseLightning]);
	m->m_bRebuild = -1;
	}

m = menu + renderOpts.nCoronas;
v = m->m_value;
if (nCoronas != v) {
	nCoronas = v;
	sprintf (m->m_text, TXT_CORONAS, pszNoneBasicAdv [nCoronas]);
	m->m_bRebuild = -1;
	}

m = menu + renderOpts.nCameras;
v = m->m_value;
if (nCameras != v) {
	nCameras = v;
	sprintf (m->m_text, TXT_CAMERAS, pszNoneBasicFull [nCameras]);
	m->m_bRebuild = -1;
	}

m = menu + renderOpts.nPowerups;
v = m->m_value;
if (nPowerups != v) {
	nPowerups = v;
	sprintf (m->m_text, TXT_POWERUPS, pszNoneBasicFull [nPowerups]);
	m->m_bRebuild = -1;
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenu	m;
	int	i;
	int	optLightOpts, optEffectOpts, optAutomapOpts;
#if DBG
	int	optWireFrame, optTextures, optObjects, optWalls, optDynLight;
#endif
	int nRendQualSave = gameOpts->render.nQuality;

	static int choice = 0;

	char szMaxFps [50];
	char szRendQual [50];
	char szTexQual [50];
	char szMeshQual [50];
	char szSlider [50];

pszNoneBasicAdv [0] = TXT_NONE;
pszNoneBasicAdv [1] = TXT_BASIC;
pszNoneBasicAdv [2] = TXT_ADVANCED;

pszNoneBasicFull [0] = TXT_NONE;
pszNoneBasicFull [1] = TXT_BASIC;
pszNoneBasicFull [2] = TXT_FULL;

pszRendQual [0] = TXT_QUALITY_LOW;
pszRendQual [1] = TXT_QUALITY_MED;
pszRendQual [2] = TXT_QUALITY_HIGH;
pszRendQual [3] = TXT_VERY_HIGH;
pszRendQual [4] = TXT_QUALITY_MAX;

pszTexQual [0] = TXT_QUALITY_LOW;
pszTexQual [1] = TXT_QUALITY_MED;
pszTexQual [2] = TXT_QUALITY_HIGH;
pszTexQual [3] = TXT_QUALITY_MAX;

pszMeshQual [0] = TXT_NONE;
pszMeshQual [1] = TXT_SMALL;
pszMeshQual [2] = TXT_MEDIUM;
pszMeshQual [3] = TXT_HIGH;
pszMeshQual [4] = TXT_EXTREME;

nCoronas = gameOpts->render.coronas.bUse ? gameOpts->render.coronas.nStyle == 2 ? 2 : 1 : 0;
nSmoke = extraGameInfo [0].bUseParticles ? gameOpts->render.particles.bStatic ? 2 : 1 : 0;
nShadows = extraGameInfo [0].bShadows ? ((gameOpts->render.shadows.nReach == 2) && (gameOpts->render.shadows.nClip == 2)) ? 2 : 1 : 0;
nPowerups = gameOpts->render.powerups.b3D ? gameOpts->render.powerups.b3DShields ? 2 : 1 : 0;
nCameras = extraGameInfo [0].bUseCameras ? gameOpts->render.cameras.bHires ? 2 : 1 : 0;

do {
	m.Destroy ();
	m.Create (50);
	optAutomapOpts = -1;

	if (1 || !gameOpts->app.bExpertMode) {
		renderOpts.nFrameCap = m.AddCheck (TXT_VSYNC, gameOpts->render.nMaxFPS == 1, KEY_V, HTX_RENDER_FRAMECAP);
		m.AddText ("", 0);
		}

	if (!gameStates.app.bNostalgia)
		renderOpts.nBrightness = m.AddSlider (TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);

	if (0 && gameOpts->app.bExpertMode) {
		if (gameOpts->render.nMaxFPS > 1)
			sprintf (szMaxFps + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
		else if (gameOpts->render.nMaxFPS < 0)
			sprintf (szMaxFps + 1, TXT_VSYNC, gameOpts->render.nMaxFPS);
		else
			sprintf (szMaxFps + 1, TXT_NO_FRAMECAP);
		*szMaxFps = *(TXT_FRAMECAP - 1);
		renderOpts.nFrameCap = m.AddSlider (szMaxFps + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, 15, KEY_F, HTX_RENDER_FRAMECAP);
		}

	sprintf (szRendQual + 1, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
	*szRendQual = *(TXT_RENDQUAL - 1);
	renderOpts.nRenderQual = m.AddSlider (szRendQual + 1, gameOpts->render.nQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);

	if (gameStates.app.bGameRunning)
		renderOpts.nTexQual =
		renderOpts.nMeshQual = -1;
	else {
		sprintf (szTexQual + 1, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
		*szTexQual = *(TXT_TEXQUAL + 1);
		renderOpts.nTexQual = m.AddSlider (szTexQual + 1, gameOpts->render.textures.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);

		if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
			sprintf (szMeshQual + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			*szMeshQual = *(TXT_MESH_QUALITY + 1);
			renderOpts.nMeshQual = m.AddSlider (szMeshQual + 1, gameOpts->render.nMeshQuality, 0, 3, KEY_V, HTX_MESH_QUALITY);
			}
		else
			renderOpts.nMeshQual = -1;
		}

	m.AddText ("");
	sprintf (szSlider + 1, TXT_SMOKE, pszNoneBasicFull [nSmoke]);
	*szSlider = *(TXT_SMOKE - 1);
	renderOpts.nSmoke = m.AddSlider (szSlider + 1, nSmoke, 0, 2, KEY_S, HTX_SMOKE);

	if (!(gameStates.app.bEnableShadows && gameStates.render.bHaveStencilBuffer))
		renderOpts.nShadows = -1;
	else {
		sprintf (szSlider + 1, TXT_SHADOWS, pszNoneBasicFull [nShadows]);
		*szSlider = *(TXT_SHADOWS - 1);
		renderOpts.nShadows = m.AddSlider (szSlider + 1, nShadows, 0, 2, KEY_A, HTX_SHADOWS);
		}

	sprintf (szSlider + 1, TXT_CORONAS, pszNoneBasicAdv [nCoronas]);
	*szSlider = *(TXT_CORONAS - 1);
	renderOpts.nCoronas = m.AddSlider (szSlider + 1, nCoronas, 0, 1 + gameStates.ogl.bDepthBlending, KEY_O, HTX_CORONAS);

	sprintf (szSlider + 1, TXT_LIGHTNING, pszNoneBasicFull [extraGameInfo [0].bUseLightning]);
	*szSlider = *(TXT_LIGHTNING - 1);
	renderOpts.nLightning = m.AddSlider (szSlider + 1, extraGameInfo [0].bUseLightning, 0, 2, KEY_I, HTX_LIGHTNING);

	sprintf (szSlider + 1, TXT_CAMERAS, pszNoneBasicFull [nCameras]);
	*szSlider = *(TXT_CAMERAS - 1);
	renderOpts.nCameras = m.AddSlider (szSlider + 1, nCameras, 0, 2, KEY_C, HTX_CAMERAS);

	sprintf (szSlider + 1, TXT_POWERUPS, pszNoneBasicFull [nPowerups]);
	*szSlider = *(TXT_POWERUPS - 1);
	renderOpts.nPowerups = m.AddSlider (szSlider + 1, nPowerups, 0, 2, KEY_P, HTX_POWERUPS);

	m.AddText ("", 0);
	optLightOpts = m.AddMenu (TXT_LIGHTING_OPTIONS, KEY_L, HTX_RENDER_LIGHTINGOPTS);
	optEffectOpts = m.AddMenu (TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
	optAutomapOpts = m.AddMenu (TXT_AUTOMAP_OPTIONS, KEY_M, HTX_RENDER_AUTOMAPOPTS);

#if DBG
	if (0 && gameOpts->app.bExpertMode) {
		m.AddText ("", 0);
		optWireFrame = m.AddCheck ("Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
		optTextures = m.AddCheck ("Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
		optWalls = m.AddCheck ("Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
		optObjects = m.AddCheck ("Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
		optDynLight = m.AddCheck ("Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
		}
#endif

	do {
		i = m.Menu (NULL, TXT_RENDER_OPTS, RenderOptionsCallback, &choice);
		if (i < 0)
			break;
		if (gameOpts->app.bExpertMode) {
			if ((optLightOpts >= 0) && (i == optLightOpts))
				i = -2, LightOptionsMenu ();
			else if ((optEffectOpts >= 0) && (i == optEffectOpts))
				i = -2, EffectOptionsMenu ();
			else if ((optAutomapOpts >= 0) && (i == optAutomapOpts))
				i = -2, AutomapOptionsMenu ();
			}
		} while (i >= 0);

	if ((extraGameInfo [0].bUseParticles == nSmoke != 0))
		gameOpts->render.particles.bStatic = 
		gameOpts->render.particles.bBubbles = nSmoke == 2;

	if (renderOpts.nShadows >= 0) {
		if ((extraGameInfo [0].bShadows = (nShadows != 0)))
			gameOpts->render.shadows.nReach =
			gameOpts->render.shadows.nClip = nShadows;
		}	

	if ((gameOpts->render.coronas.bUse = (nCoronas != 0)))
		gameOpts->render.coronas.nStyle = nCoronas;

	if ((extraGameInfo [0].bUseCameras = (nCameras != 0))) 
		gameOpts->render.cameras.bHires = (nCameras == 2);

	if ((gameOpts->render.powerups.b3D = (nPowerups != 0)))
		gameOpts->render.powerups.b3DShields = (nPowerups == 2);

	gameOpts->render.nMaxFPS = m [renderOpts.nFrameCap].m_value ? 60 : 1;
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [renderOpts.nBrightness].m_value);
	if (nRendQualSave != gameOpts->render.nQuality)
		SetRenderQuality ();
#if EXPMODE_DEFAULTS
	else {
		gameOpts->render.nMaxFPS = 250;
		gameOpts->render.color.nLightmapRange = 5;
		gameOpts->render.color.bMix = 1;
		gameOpts->render.nQuality = 3;
		gameOpts->render.color.bWalls = 1;
		gameOpts->render.effects.bTransparent = 1;
		gameOpts->render.particles.bPlayers = 0;
		gameOpts->render.particles.bRobots =
		gameOpts->render.particles.bMissiles = 1;
		gameOpts->render.particles.bCollisions = 0;
		gameOpts->render.particles.bDisperse = 0;
		gameOpts->render.particles.nDens = 2;
		gameOpts->render.particles.nSize = 3;
		gameOpts->render.cameras.bFitToWall = 0;
		gameOpts->render.cameras.nSpeed = 5000;
		gameOpts->render.cameras.nFPS = 0;
		gameOpts->movies.nQuality = 0;
		gameOpts->movies.bResize = 1;
		gameStates.ogl.nContrast = 8;
		gameOpts->ogl.bSetGammaRamp = 0;
		}
#endif
#if DBG
	if (0 && gameOpts->app.bExpertMode) {
		gameOpts->render.debug.bWireFrame = m [optWireFrame].m_value;
		gameOpts->render.debug.bTextures = m [optTextures].m_value;
		gameOpts->render.debug.bObjects = m [optObjects].m_value;
		gameOpts->render.debug.bWalls = m [optWalls].m_value;
		gameOpts->render.debug.bDynamicLight = m [optDynLight].m_value;
		}
#endif
	} while (i == -2);

DefaultRenderSettings ();
}

#else //SIMPLE_MENUS

int RenderOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menu + renderOpts.nBrightness;
	v = m->m_value;
	if (v != paletteManager.GetGamma ())
		paletteManager.SetGamma (v);
	}
m = menu + renderOpts.nFrameCap;
v = fpsTable [m->m_value];
if (gameOpts->render.nMaxFPS != v) {
	if (v > 0)
		sprintf (m->m_text, TXT_FRAMECAP, v);
	else if (v < 0) {
		if (!gameStates.render.bVSyncOk) {
			m->m_value = 1;
			return nCurItem;
			}
		sprintf (m->m_text, TXT_VSYNC);
		}
	else
		sprintf (m->m_text, TXT_NO_FRAMECAP);
#if WIN32
	if (gameStates.render.bVSyncOk)
		wglSwapIntervalEXT (v < 0);
#endif
	gameOpts->render.nMaxFPS = v;
	gameStates.render.bVSync = (v < 0);
	m->m_bRebuild = 1;
	}
if (gameOpts->app.bExpertMode) {
	if (renderOpts.nContrast >= 0) {
		m = menu + renderOpts.nContrast;
		v = m->m_value;
		if (v != gameStates.ogl.nContrast) {
			gameStates.ogl.nContrast = v;
			sprintf (m->m_text, TXT_CONTRAST, ContrastText ());
			m->m_bRebuild = 1;
			}
		}
	m = menu + renderOpts.nRenderQual;
	v = m->m_value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->m_text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->m_bRebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menu + renderOpts.nTexQual;
		v = m->m_value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->m_text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->m_bRebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menu + renderOpts.nMeshQual;
		v = m->m_value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->m_text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->m_bRebuild = 1;
			}
		}
	m = menu + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->m_value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->m_text, TXT_WALL_TRANSP, m->m_value * 10, '%');
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenu	m;
	int	h, i, choice = 0;
	int	optSmokeOpts, optShadowOpts, optCameraOpts, optLightOpts, optMovieOpts,
			optAdvOpts, optEffectOpts, optPowerupOpts, optAutomapOpts, optLightningOpts,
			optUseGamma, optColoredWalls, optDepthSort, optCoronaOpts, optShipRenderOpts;
#if DBG
	int	optWireFrame, optTextures, optObjects, optWalls, optDynLight;
#endif

	char szWallTransp [50];
	char szContrast [50];

	char szMaxFps [50];
	char szWallTransp [50];
	char szRendQual [50];
	char szTexQual [50];
	char szMeshQual [50];
	char szContrast [50];

	int nRendQualSave = gameOpts->render.nQuality;

	pszRendQual [0] = TXT_QUALITY_LOW;
	pszRendQual [1] = TXT_QUALITY_MED;
	pszRendQual [2] = TXT_QUALITY_HIGH;
	pszRendQual [3] = TXT_VERY_HIGH;
	pszRendQual [4] = TXT_QUALITY_MAX;

	pszTexQual [0] = TXT_QUALITY_LOW;
	pszTexQual [1] = TXT_QUALITY_MED;
	pszTexQual [2] = TXT_QUALITY_HIGH;
	pszTexQual [3] = TXT_QUALITY_MAX;

	pszMeshQual [0] = TXT_NONE;
	pszMeshQual [1] = TXT_SMALL;
	pszMeshQual [2] = TXT_MEDIUM;
	pszMeshQual [3] = TXT_HIGH;
	pszMeshQual [4] = TXT_EXTREME;

do {
	m.Destroy ();
	m.Create (50);
	optPowerupOpts = optAutomapOpts = -1;
	if (!gameStates.app.bNostalgia) {
		renderOpts.nBrightness = m.AddSlider (TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		}
	if (gameOpts->render.nMaxFPS > 1)
		sprintf (szMaxFps + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else if (gameOpts->render.nMaxFPS < 0)
		sprintf (szMaxFps + 1, TXT_VSYNC, gameOpts->render.nMaxFPS);
	else
		sprintf (szMaxFps + 1, TXT_NO_FRAMECAP);
	*szMaxFps = *(TXT_FRAMECAP - 1);
	renderOpts.nFrameCap = m.AddSlider (szMaxFps + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, 15, KEY_F, HTX_RENDER_FRAMECAP);

	renderOpts.nContrast = -1;
	if (gameOpts->app.bExpertMode) {
		if ((gameOpts->render.nLightingMethod < 2) && !gameOpts->render.bUseLightmaps) {
			sprintf (szContrast, TXT_CONTRAST, ContrastText ());
			renderOpts.nContrast = m.AddSlider (szContrast, gameStates.ogl.nContrast, 0, 16, KEY_C, HTX_ADVRND_CONTRAST);
			}
		sprintf (szRendQual + 1, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		*szRendQual = *(TXT_RENDQUAL - 1);
		renderOpts.nRenderQual = m.AddSlider (szRendQual + 1, gameOpts->render.nQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);
		if (gameStates.app.bGameRunning)
			renderOpts.nTexQual =
			renderOpts.nMeshQual = -1;
		else {
			sprintf (szTexQual + 1, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			*szTexQual = *(TXT_TEXQUAL + 1);
			renderOpts.nTexQual = m.AddSlider (szTexQual + 1, gameOpts->render.textures.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);
			if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
				sprintf (szMeshQual + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
				*szMeshQual = *(TXT_MESH_QUALITY + 1);
				renderOpts.nMeshQual = m.AddSlider (szMeshQual + 1, gameOpts->render.nMeshQuality, 0, 4, KEY_V, HTX_MESH_QUALITY);
				}
			else
				renderOpts.nMeshQual = -1;
			}
		m.AddText ("", 0);
		h = extraGameInfo [0].grWallTransparency * 10 / FADE_LEVELS;
		sprintf (szWallTransp + 1, TXT_WALL_TRANSP, h * 10, '%');
		*szWallTransp = *(TXT_WALL_TRANSP - 1);
		renderOpts.nWallTransp = m.AddSlider (szWallTransp + 1, h, 0, 10, KEY_T, HTX_ADVRND_WALLTRANSP);
		optColoredWalls = m.AddCheck (TXT_COLOR_WALLS, gameOpts->render.color.bWalls, KEY_W, HTX_ADVRND_COLORWALLS);
		if (RENDERPATH)
			optDepthSort = -1;
		else
			optDepthSort = m.AddCheck (TXT_TRANSP_DEPTH_SORT, gameOpts->render.bDepthSort, KEY_D, HTX_TRANSP_DEPTH_SORT);
#if 0
		optUseGamma = m.AddCheck (TXT_GAMMA_BRIGHT, gameOpts->ogl.bSetGammaRamp, KEY_V, HTX_ADVRND_GAMMA);
#else
		optUseGamma = -1;
#endif
		m.AddText ("", 0);
		optLightOpts = m.AddMenu (TXT_LIGHTING_OPTIONS, KEY_L, HTX_RENDER_LIGHTINGOPTS);
		optSmokeOpts = m.AddMenu (TXT_SMOKE_OPTIONS, KEY_S, HTX_RENDER_SMOKEOPTS);
		optLightningOpts = m.AddMenu (TXT_LIGHTNING_OPTIONS, KEY_I, HTX_LIGHTNING_OPTIONS);
		if (!(gameStates.app.bEnableShadows && gameStates.render.bHaveStencilBuffer))
			optShadowOpts = -1;
		else
			optShadowOpts = m.AddMenu (TXT_SHADOW_OPTIONS, KEY_A, HTX_RENDER_SHADOWOPTS);
		optEffectOpts = m.AddMenu (TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
		optCoronaOpts = m.AddMenu (TXT_CORONA_OPTIONS, KEY_O, HTX_RENDER_CORONAOPTS);
		optCameraOpts = m.AddMenu (TXT_CAMERA_OPTIONS, KEY_C, HTX_RENDER_CAMERAOPTS);
		optPowerupOpts = m.AddMenu (TXT_POWERUP_OPTIONS, KEY_P, HTX_RENDER_PRUPOPTS);
		optAutomapOpts = m.AddMenu (TXT_AUTOMAP_OPTIONS, KEY_M, HTX_RENDER_AUTOMAPOPTS);
		optShipRenderOpts = m.AddMenu (TXT_SHIP_RENDEROPTIONS, KEY_H, HTX_RENDER_SHIPOPTS);
		optMovieOpts = m.AddMenu (TXT_MOVIE_OPTIONS, KEY_M, HTX_RENDER_MOVIEOPTS);
		}
	else
		renderOpts.nRenderQual =
		renderOpts.nTexQual =
		renderOpts.nMeshQual =
		renderOpts.nWallTransp = 
		optUseGamma = 
		optColoredWalls =
		optDepthSort =
		renderOpts.nContrast =
		optLightOpts =
		optLightningOpts =
		optSmokeOpts =
		optShadowOpts =
		optEffectOpts =
		optCoronaOpts =
		optCameraOpts = 
		optMovieOpts = 
		optShipRenderOpts =
		optAdvOpts = -1;

#if DBG
	m.AddText ("", 0);
	optWireFrame = m.AddCheck ("Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
	optTextures = m.AddCheck ("Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
	optWalls = m.AddCheck ("Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
	optObjects = m.AddCheck ("Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
	optDynLight = m.AddCheck ("Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
#endif

	do {
		i = m.Menu (NULL, TXT_RENDER_OPTS, RenderOptionsCallback, &choice);
		if (i < 0)
			break;
		if (gameOpts->app.bExpertMode) {
			if ((optLightOpts >= 0) && (i == optLightOpts))
				i = -2, LightOptionsMenu ();
			else if ((optSmokeOpts >= 0) && (i == optSmokeOpts))
				i = -2, SmokeOptionsMenu ();
			else if ((optLightningOpts >= 0) && (i == optLightningOpts))
				i = -2, LightningOptionsMenu ();
			else if ((optShadowOpts >= 0) && (i == optShadowOpts))
				i = -2, ShadowOptionsMenu ();
			else if ((optEffectOpts >= 0) && (i == optEffectOpts))
				i = -2, EffectOptionsMenu ();
			else if ((optCoronaOpts >= 0) && (i == optCoronaOpts))
				i = -2, CoronaOptionsMenu ();
			else if ((optCameraOpts >= 0) && (i == optCameraOpts))
				i = -2, CameraOptionsMenu ();
			else if ((optPowerupOpts >= 0) && (i == optPowerupOpts))
				i = -2, PowerupOptionsMenu ();
			else if ((optAutomapOpts >= 0) && (i == optAutomapOpts))
				i = -2, AutomapOptionsMenu ();
			else if ((optMovieOpts >= 0) && (i == optMovieOpts))
				i = -2, MovieOptionsMenu ();
			else if ((optShipRenderOpts >= 0) && (i == optShipRenderOpts))
				i = -2, ShipRenderOptionsMenu ();
			}
		} while (i >= 0);
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [renderOpts.nBrightness].m_value);
	if (gameOpts->app.bExpertMode) {
		gameOpts->render.color.bWalls = m [optColoredWalls].m_value;
		GET_VAL (gameOpts->render.bDepthSort, optDepthSort);
		GET_VAL (gameOpts->ogl.bSetGammaRamp, optUseGamma);
		if (renderOpts.nContrast >= 0)
			gameStates.ogl.nContrast = m [renderOpts.nContrast].m_value;
		if (nRendQualSave != gameOpts->render.nQuality)
			SetRenderQuality ();
		}
#if EXPMODE_DEFAULTS
	else {
		gameOpts->render.nMaxFPS = 250;
		gameOpts->render.color.nLightmapRange = 5;
		gameOpts->render.color.bMix = 1;
		gameOpts->render.nQuality = 3;
		gameOpts->render.color.bWalls = 1;
		gameOpts->render.effects.bTransparent = 1;
		gameOpts->render.particles.bPlayers = 0;
		gameOpts->render.particles.bRobots =
		gameOpts->render.particles.bMissiles = 1;
		gameOpts->render.particles.bCollisions = 0;
		gameOpts->render.particles.bDisperse = 0;
		gameOpts->render.particles.nDens = 2;
		gameOpts->render.particles.nSize = 3;
		gameOpts->render.cameras.bFitToWall = 0;
		gameOpts->render.cameras.nSpeed = 5000;
		gameOpts->render.cameras.nFPS = 0;
		gameOpts->movies.nQuality = 0;
		gameOpts->movies.bResize = 1;
		gameStates.ogl.nContrast = 8;
		gameOpts->ogl.bSetGammaRamp = 0;
		}
#endif
#if DBG
	gameOpts->render.debug.bWireFrame = m [optWireFrame].m_value;
	gameOpts->render.debug.bTextures = m [optTextures].m_value;
	gameOpts->render.debug.bObjects = m [optObjects].m_value;
	gameOpts->render.debug.bWalls = m [optWalls].m_value;
	gameOpts->render.debug.bDynamicLight = m [optDynLight].m_value;
#endif
	} while (i == -2);

}

#endif

//------------------------------------------------------------------------------
//eof
