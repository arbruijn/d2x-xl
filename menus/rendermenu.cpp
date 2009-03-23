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

#define EXPERTMODE	0 //gameOpts->app.bExpertMode

#define MIN_LIGHTS_PER_PASS 5

//------------------------------------------------------------------------------

static struct {
	int	nFrameCap;
	int	nImageQual;
	int	nRenderQual;
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

#if DBG
static int fpsTable [] = {-1, 0, 10, 20, 30, 60}; //40, 50, 60, 70, 80, 90, 100, 125, 150, 200, 250};
#else
static int fpsTable [] = {0, 60};
#endif

static const char *pszRendQual [4];
static const char *pszMeshQual [5];
static const char *pszImgQual [5];

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
if (EXPERTMODE) {
	m = menu + renderOpts.nImageQual;
	v = m->m_value;
	if (gameOpts->render.nImageQuality != v) {
		gameOpts->render.nImageQuality = v;
		sprintf (m->m_text, TXT_IMAGE_QUALITY, pszImgQual [gameOpts->render.nImageQuality]);
		m->m_bRebuild = 1;
		}
	if (renderOpts.nRenderQual > 0) {
		m = menu + renderOpts.nRenderQual;
		v = m->m_value;
		if (gameOpts->render.nQuality != v) {
			gameOpts->render.nQuality = v;
			sprintf (m->m_text, TXT_RENDER_QUALITY, pszRendQual [gameOpts->render.nQuality]);
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

static int nCoronas, nShadows, nPowerups, nCameras, nLighting;

static const char* pszNoneBasicAdv [3];
static const char* pszNoneBasicFull [3];
static const char* pszNoneBasicStdFull [4];
static const char *pszQuality [4];

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

#if !DBG
if (EXPERTMODE) 
#endif
{
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

m = menu + renderOpts.nImageQual;
v = m->m_value;
if (gameOpts->render.nImageQuality != v) {
	gameOpts->render.nImageQuality = v;
	sprintf (m->m_text, TXT_IMAGE_QUALITY, pszImgQual [gameOpts->render.nImageQuality]);
	m->m_bRebuild = 1;
	}

if (renderOpts.nRenderQual > 0) {
	m = menu + renderOpts.nRenderQual;
	v = m->m_value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->m_text, TXT_RENDER_QUALITY, pszRendQual [gameOpts->render.nQuality]);
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
if (gameOpts->render.particles.nQuality != v) {
	gameOpts->render.particles.nQuality = v;
	sprintf (m->m_text, TXT_SMOKE, pszNoneBasicFull [gameOpts->render.particles.nQuality]);
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

if (!gameStates.app.bGameRunning) {
	m = menu + renderOpts.nLighting;
	v = m->m_value;
	if (nLighting != v) {
		nLighting = v;
		sprintf (m->m_text, TXT_LIGHTING, pszQuality [nLighting]);
		key = -2;
		return nCurItem;
		}

	if (renderOpts.nLightmaps >= 0) {
		m = menu + renderOpts.nLightmaps;
		v = m->m_value;
		if (gameOpts->render.nLightmapQuality != v) {
			gameOpts->render.nLightmapQuality = v;
			sprintf (m->m_text, TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
			m->m_bRebuild = 1;
			}
		}

	if (renderOpts.nLights >= 0) {
		m = menu + renderOpts.nLights;
		v = m->m_value;
		if (v != gameOpts->ogl.nMaxLightsPerPass - MIN_LIGHTS_PER_PASS) {
			gameOpts->ogl.nMaxLightsPerPass = v + MIN_LIGHTS_PER_PASS;
			sprintf (m->m_text, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			key = -2;
			return nCurItem;
			}
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenu	m;
	int	i;
#if DBG
	int	optWireFrame, optTextures, optObjects, optWalls, optDynLight;
#endif
	int nRendQualSave = gameOpts->render.nImageQuality, optSubTitles;

	static int choice = 0;

	char szSlider [50];

pszNoneBasicAdv [0] = TXT_NONE;
pszNoneBasicAdv [1] = TXT_BASIC;
pszNoneBasicAdv [2] = TXT_ADVANCED;

pszNoneBasicFull [0] = TXT_NONE;
pszNoneBasicFull [1] = TXT_BASIC;
pszNoneBasicFull [2] = TXT_FULL;

pszNoneBasicStdFull [0] = TXT_NONE;
pszNoneBasicStdFull [1] = TXT_BASIC;
pszNoneBasicStdFull [2] = TXT_STANDARD;
pszNoneBasicStdFull [3] = TXT_FULL;

pszImgQual [0] = TXT_QUALITY_LOW;
pszImgQual [1] = TXT_QUALITY_MED;
pszImgQual [2] = TXT_QUALITY_HIGH;
pszImgQual [3] = TXT_VERY_HIGH;
pszImgQual [4] = TXT_QUALITY_MAX;

pszRendQual [0] = TXT_QUALITY_LOW;
pszRendQual [1] = TXT_QUALITY_MED;
pszRendQual [2] = TXT_QUALITY_HIGH;
pszRendQual [3] = TXT_QUALITY_MAX;

pszMeshQual [0] = TXT_NONE;
pszMeshQual [1] = TXT_SMALL;
pszMeshQual [2] = TXT_MEDIUM;
pszMeshQual [3] = TXT_HIGH;
pszMeshQual [4] = TXT_EXTREME;

pszQuality [0] = TXT_BASIC;
pszQuality [1] = TXT_STANDARD;
pszQuality [2] = TXT_HIGH;
pszQuality [3] = TXT_BEST;

if (!gameOpts->ogl.bGlTexMerge) {
	if (gameOpts->render.nLightingMethod > 1)
		gameOpts->render.nLightingMethod = 1;
	gameOpts->render.bUseLightmaps = 0;
	}
nLighting = (gameOpts->render.nLightingMethod == 0) 
				? 0 
				: (gameOpts->render.nLightingMethod == 2) 
					? 3 
					: (gameStates.render.bLightmapsOk && gameOpts->render.bUseLightmaps) + 1;
nCoronas = gameOpts->render.coronas.bUse ? gameOpts->render.coronas.nStyle == 2 ? 2 : 1 : 0;
nShadows = extraGameInfo [0].bShadows ? ((gameOpts->render.shadows.nReach == 2) && (gameOpts->render.shadows.nClip == 2)) ? 2 : 1 : 0;
nPowerups = gameOpts->render.powerups.b3D ? gameOpts->render.powerups.b3DShields ? 2 : 1 : 0;
nCameras = extraGameInfo [0].bUseCameras ? gameOpts->render.cameras.bHires ? 2 : 1 : 0;

do {
	m.Destroy ();
	m.Create (50);
#if !DBG
	if (!EXPERTMODE)
		renderOpts.nFrameCap = m.AddCheck (TXT_VSYNC, gameOpts->render.nMaxFPS == 1, KEY_V, HTX_RENDER_FRAMECAP);
#endif
	if (!gameStates.app.bNostalgia)
		renderOpts.nBrightness = m.AddSlider (TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
	m.AddText ("");
#if !DBG
	if (EXPERTMODE) 
#endif
		{
		if (gameOpts->render.nMaxFPS > 1)
			sprintf (szSlider + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
		else if (gameOpts->render.nMaxFPS < 0)
			sprintf (szSlider + 1, TXT_VSYNC, gameOpts->render.nMaxFPS);
		else
			sprintf (szSlider + 1, TXT_NO_FRAMECAP);
		*szSlider = *(TXT_FRAMECAP - 1);
		renderOpts.nFrameCap = m.AddSlider (szSlider + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, sizeofa (fpsTable) - 1, KEY_F, HTX_RENDER_FRAMECAP);
		}

	renderOpts.nLightmaps = 
	renderOpts.nLights = 
	renderOpts.nPasses = -1;
	if (gameStates.app.bGameRunning || gameStates.app.bNostalgia)
		renderOpts.nLighting = -1;
	else {
		sprintf (szSlider + 1, TXT_LIGHTING, pszQuality [nLighting]);
		*szSlider = *(TXT_LIGHTING + 1);
		renderOpts.nLighting = m.AddSlider (szSlider + 1, nLighting, 0, gameOpts->ogl.bGlTexMerge ? 3 : 1, KEY_L, HTX_LIGHTING);
		if (nLighting >= 2) {
			sprintf (szSlider + 1, TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
			*szSlider = *(TXT_LMAP_QUALITY + 1);
			renderOpts.nLightmaps = m.AddSlider (szSlider + 1, gameOpts->render.nLightmapQuality, 0, 3, KEY_Q, HTX_LMAP_QUALITY);

			if (nLighting == 3) {
				sprintf (szSlider + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
				*szSlider = *(TXT_MAX_LIGHTS_PER_PASS - 1);
				renderOpts.nLights = m.AddSlider (szSlider + 1, gameOpts->ogl.nMaxLightsPerPass - 5, 0, 8 - MIN_LIGHTS_PER_PASS, KEY_P, HTX_MAX_LIGHTS_PER_PASS);
				}
			}
		m.AddText ("", 0);
		}

	sprintf (szSlider + 1, TXT_IMAGE_QUALITY, pszImgQual [gameOpts->render.nImageQuality]);
	*szSlider = *(TXT_IMAGE_QUALITY - 1);
	renderOpts.nImageQual = m.AddSlider (szSlider + 1, gameOpts->render.nImageQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);
	if (gameStates.app.bGameRunning)
		renderOpts.nRenderQual =
		renderOpts.nMeshQual = -1;
	else {
		sprintf (szSlider + 1, TXT_RENDER_QUALITY, pszRendQual [gameOpts->render.nQuality]);
		*szSlider = *(TXT_RENDER_QUALITY + 1);
		renderOpts.nRenderQual = m.AddSlider (szSlider + 1, gameOpts->render.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);

		if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
			sprintf (szSlider + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			*szSlider = *(TXT_MESH_QUALITY + 1);
			renderOpts.nMeshQual = m.AddSlider (szSlider + 1, gameOpts->render.nMeshQuality, 0, 3, KEY_V, HTX_MESH_QUALITY);
			}
		else
			renderOpts.nMeshQual = -1;
		}
	m.AddText ("");
	sprintf (szSlider + 1, TXT_SMOKE, pszNoneBasicFull [gameOpts->render.particles.nQuality]);
	*szSlider = *(TXT_SMOKE - 1);
	renderOpts.nSmoke = m.AddSlider (szSlider + 1, gameOpts->render.particles.nQuality, 0, 2, KEY_S, HTX_SMOKE);
	if (!gameStates.render.bHaveStencilBuffer)
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

	m.AddText ("");
	optSubTitles = m.AddCheck (TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_O, HTX_RENDER_SUBTTL);

#if DBG
	if (EXPERTMODE) {
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
		} while (i >= 0);

	extraGameInfo [0].bUseParticles = (gameOpts->render.particles.nQuality != 0);
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
	gameOpts->movies.bSubTitles = (m [optSubTitles].m_value != 0);

#if !DBG
	if (!EXPERTMODE)
		gameOpts->render.nMaxFPS = m [renderOpts.nFrameCap].m_value ? 1 : 60;
#endif
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [renderOpts.nBrightness].m_value);
	if (nRendQualSave != gameOpts->render.nImageQuality)
		SetRenderQuality ();

	if ((gameStates.app.bNostalgia > 1) || (nLighting == 0))
		gameOpts->render.nLightingMethod = 0;
	else {
		gameOpts->render.bUseLightmaps = (nLighting > 1);
		gameOpts->render.nLightingMethod = nLighting - gameOpts->render.bUseLightmaps;
		}

#if DBG
	if (EXPERTMODE) {
		gameOpts->render.debug.bWireFrame = m [optWireFrame].m_value;
		gameOpts->render.debug.bTextures = m [optTextures].m_value;
		gameOpts->render.debug.bObjects = m [optObjects].m_value;
		gameOpts->render.debug.bWalls = m [optWalls].m_value;
		gameOpts->render.debug.bDynamicLight = m [optDynLight].m_value;
		}
#endif
	} while (i == -2);

lightManager.SetMethod ();
DefaultRenderSettings ();
}

//------------------------------------------------------------------------------
//eof
