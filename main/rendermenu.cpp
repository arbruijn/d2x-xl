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
#include "inferno.h"
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
#include "newmenu.h"
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
#include "gauges.h"
#include "strutil.h"
#include "reorder.h"
#include "render.h"
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
#ifdef EDITOR
#	include "editor/editor.h"
#endif

//------------------------------------------------------------------------------

static struct {
	int	nMaxFPS;
	int	nRenderQual;
	int	nTexQual;
	int	nMeshQual;
	int	nWallTransp;
} renderOpts;

static int fpsTable [16] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 250};

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

for (i = 0; i < 16; i++) {
	d = abs (nFps - fpsTable [i]);
	if (d < dMin) {
		j = i;
		dMin = d;
		}
	}
return j;
}

//------------------------------------------------------------------------------

int AdvancedRenderOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (optContrast >= 0) {
	m = menus + optContrast;
	v = m->value;
	if (v != gameStates.ogl.nContrast) {
		gameStates.ogl.nContrast = v;
		sprintf (m->text, TXT_CONTRAST, ContrastText ());
		m->rebuild = 1;
		}
	}
if (gameOpts->app.bExpertMode) {
	m = menus + renderOpts.nRenderQual;
	v = m->value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->rebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menus + renderOpts.nTexQual;
		v = m->value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->rebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menus + renderOpts.nMeshQual;
		v = m->value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int RenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + optBrightness;
	v = m->value;
	if (v != paletteManager.GetGamma ())
		paletteManager.SetGamma (v);
	}
m = menus + renderOpts.nMaxFPS;
v = fpsTable [m->value];
if (gameOpts->render.nMaxFPS != (v ? v : 1)) {
	gameOpts->render.nMaxFPS = v ? v : 1;
	if (v)
		sprintf (m->text, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (m->text, TXT_NO_FRAMECAP);
	m->rebuild = 1;
	}
if (gameOpts->app.bExpertMode) {
	if (optContrast >= 0) {
		m = menus + optContrast;
		v = m->value;
		if (v != gameStates.ogl.nContrast) {
			gameStates.ogl.nContrast = v;
			sprintf (m->text, TXT_CONTRAST, ContrastText ());
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nRenderQual;
	v = m->value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->rebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menus + renderOpts.nTexQual;
		v = m->value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->rebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menus + renderOpts.nMeshQual;
		v = m->value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenuItem m [40];
	int	h, i, choice = 0;
	int	nOptions;
	int	optSmokeOpts, optShadowOpts, optCameraOpts, optLightOpts, optMovieOpts,
			optAdvOpts, optEffectOpts, optPowerupOpts, optAutomapOpts, optLightningOpts,
			optUseGamma, optColoredWalls, optDepthSort, optCoronaOpts, optShipRenderOpts;
#if DBG
	int	optWireFrame, optTextures, optObjects, optWalls, optDynLight;
#endif

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
	memset (m, 0, sizeof (m));
	nOptions = 0;
	optPowerupOpts = optAutomapOpts = -1;
	if (!gameStates.app.bNostalgia) {
		m.AddSlider (nOptions, TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = nOptions++;
		}
	if (gameOpts->render.nMaxFPS > 1)
		sprintf (szMaxFps + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (szMaxFps + 1, TXT_NO_FRAMECAP);
	*szMaxFps = *(TXT_FRAMECAP - 1);
	m.AddSlider (nOptions, szMaxFps + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, 15, KEY_F, HTX_RENDER_FRAMECAP);
	renderOpts.nMaxFPS = nOptions++;

	if (gameOpts->app.bExpertMode) {
		if ((gameOpts->render.nLightingMethod < 2) && !gameOpts->render.bUseLightmaps) {
			sprintf (szContrast, TXT_CONTRAST, ContrastText ());
			m.AddSlider (nOptions, szContrast, gameStates.ogl.nContrast, 0, 16, KEY_C, HTX_ADVRND_CONTRAST);
			optContrast = nOptions++;
			}
		sprintf (szRendQual + 1, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		*szRendQual = *(TXT_RENDQUAL - 1);
		m.AddSlider (nOptions, szRendQual + 1, gameOpts->render.nQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);
		renderOpts.nRenderQual = nOptions++;
		if (gameStates.app.bGameRunning)
			renderOpts.nTexQual =
			renderOpts.nMeshQual = 
			lightOpts.nLightmapQual = -1;
		else {
			sprintf (szTexQual + 1, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			*szTexQual = *(TXT_TEXQUAL + 1);
			m.AddSlider (nOptions, szTexQual + 1, gameOpts->render.textures.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);
			renderOpts.nTexQual = nOptions++;
			if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
				sprintf (szMeshQual + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
				*szMeshQual = *(TXT_MESH_QUALITY + 1);
				m.AddSlider (nOptions, szMeshQual + 1, gameOpts->render.nMeshQuality, 0, 4, KEY_O, HTX_MESH_QUALITY);
				renderOpts.nMeshQual = nOptions++;
				}
			else
				renderOpts.nMeshQual = -1;
			}
		m.AddText (nOptions, "", 0);
		nOptions++;
		h = extraGameInfo [0].grWallTransparency * 10 / FADE_LEVELS;
		sprintf (szWallTransp + 1, TXT_WALL_TRANSP, h * 10, '%');
		*szWallTransp = *(TXT_WALL_TRANSP - 1);
		m.AddSlider (nOptions, szWallTransp + 1, h, 0, 10, KEY_T, HTX_ADVRND_WALLTRANSP);
		renderOpts.nWallTransp = nOptions++;
		m.AddCheck (nOptions, TXT_COLOR_WALLS, gameOpts->render.color.bWalls, KEY_W, HTX_ADVRND_COLORWALLS);
		optColoredWalls = nOptions++;
		if (RENDERPATH)
			optDepthSort = -1;
		else {
			m.AddCheck (nOptions, TXT_TRANSP_DEPTH_SORT, gameOpts->render.bDepthSort, KEY_D, HTX_TRANSP_DEPTH_SORT);
			optDepthSort = nOptions++;
			}
#if 0
		m.AddCheck (nOptions, TXT_GAMMA_BRIGHT, gameOpts->ogl.bSetGammaRamp, KEY_V, HTX_ADVRND_GAMMA);
		optUseGamma = nOptions++;
#else
		optUseGamma = -1;
#endif
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddMenu (nOptions, TXT_LIGHTING_OPTIONS, KEY_L, HTX_RENDER_LIGHTINGOPTS);
		optLightOpts = nOptions++;
		m.AddMenu (nOptions, TXT_SMOKE_OPTIONS, KEY_S, HTX_RENDER_SMOKEOPTS);
		optSmokeOpts = nOptions++;
		m.AddMenu (nOptions, TXT_LIGHTNING_OPTIONS, KEY_I, HTX_LIGHTNING_OPTIONS);
		optLightningOpts = nOptions++;
		if (!(gameStates.app.bEnableShadows && gameStates.render.bHaveStencilBuffer))
			optShadowOpts = -1;
		else {
			m.AddMenu (nOptions, TXT_SHADOW_OPTIONS, KEY_A, HTX_RENDER_SHADOWOPTS);
			optShadowOpts = nOptions++;
			}
		m.AddMenu (nOptions, TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
		optEffectOpts = nOptions++;
		m.AddMenu (nOptions, TXT_CORONA_OPTIONS, KEY_O, HTX_RENDER_CORONAOPTS);
		optCoronaOpts = nOptions++;
		m.AddMenu (nOptions, TXT_CAMERA_OPTIONS, KEY_C, HTX_RENDER_CAMERAOPTS);
		optCameraOpts = nOptions++;
		m.AddMenu (nOptions, TXT_POWERUP_OPTIONS, KEY_P, HTX_RENDER_PRUPOPTS);
		optPowerupOpts = nOptions++;
		m.AddMenu (nOptions, TXT_AUTOMAP_OPTIONS, KEY_M, HTX_RENDER_AUTOMAPOPTS);
		optAutomapOpts = nOptions++;
		m.AddMenu (nOptions, TXT_SHIP_RENDEROPTIONS, KEY_H, HTX_RENDER_SHIPOPTS);
		optShipRenderOpts = nOptions++;
		m.AddMenu (nOptions, TXT_MOVIE_OPTIONS, KEY_M, HTX_RENDER_MOVIEOPTS);
		optMovieOpts = nOptions++;
		}
	else
		renderOpts.nRenderQual =
		renderOpts.nTexQual =
		renderOpts.nMeshQual =
		renderOpts.nWallTransp = 
		optUseGamma = 
		optColoredWalls =
		optDepthSort =
		optContrast =
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
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddCheck (nOptions, "Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
	optWireFrame = nOptions++;
	m.AddCheck (nOptions, "Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
	optTextures = nOptions++;
	m.AddCheck (nOptions, "Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
	optWalls = nOptions++;
	m.AddCheck (nOptions, "Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
	optObjects = nOptions++;
	m.AddCheck (nOptions, "Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
	optDynLight = nOptions++;
#endif

	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_RENDER_OPTS, nOptions, m, &RenderOptionsCallback, &choice);
		if (i < 0)
			break;
		if (gameOpts->app.bExpertMode) {
			if ((optLightOpts >= 0) && (i == optLightOpts))
				i = -2, LightOptionsMenu (void);
			else if ((optSmokeOpts >= 0) && (i == optSmokeOpts))
				i = -2, SmokeOptionsMenu (void);
			else if ((optLightningOpts >= 0) && (i == optLightningOpts))
				i = -2, LightningOptionsMenu (void);
			else if ((optShadowOpts >= 0) && (i == optShadowOpts))
				i = -2, ShadowOptionsMenu (void);
			else if ((optEffectOpts >= 0) && (i == optEffectOpts))
				i = -2, EffectOptionsMenu (void);
			else if ((optCoronaOpts >= 0) && (i == optCoronaOpts))
				i = -2, CoronaOptionsMenu (void);
			else if ((optCameraOpts >= 0) && (i == optCameraOpts))
				i = -2, CameraOptionsMenu (void);
			else if ((optPowerupOpts >= 0) && (i == optPowerupOpts))
				i = -2, PowerupOptionsMenu (void);
			else if ((optAutomapOpts >= 0) && (i == optAutomapOpts))
				i = -2, AutomapOptionsMenu (void);
			else if ((optMovieOpts >= 0) && (i == optMovieOpts))
				i = -2, MovieOptionsMenu (void);
			else if ((optShipRenderOpts >= 0) && (i == optShipRenderOpts))
				i = -2, ShipRenderOptionsMenu (void);
			}
		} while (i >= 0);
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [optBrightness].value);
	if (gameOpts->app.bExpertMode) {
		gameOpts->render.color.bWalls = m [optColoredWalls].value;
		GET_VAL (gameOpts->render.bDepthSort, optDepthSort);
		GET_VAL (gameOpts->ogl.bSetGammaRamp, optUseGamma);
		if (optContrast >= 0)
			gameStates.ogl.nContrast = m [optContrast].value;
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
	gameOpts->render.debug.bWireFrame = m [optWireFrame].value;
	gameOpts->render.debug.bTextures = m [optTextures].value;
	gameOpts->render.debug.bObjects = m [optObjects].value;
	gameOpts->render.debug.bWalls = m [optWalls].value;
	gameOpts->render.debug.bDynamicLight = m [optDynLight].value;
#endif
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
