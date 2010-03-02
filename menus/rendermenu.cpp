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

#define EXPERTMODE	gameOpts->app.bExpertMode

#define MIN_LIGHTS_PER_PASS 5

#define STEREO_SEPARATION_STEP (I2X (1) / 8)

//------------------------------------------------------------------------------

static struct {
	int	nFrameCap;
	int	nImageQual;
	int	nRenderQual;
	int	nMeshQual;
	int	nPowerups;
	int	nLighting;
	int	nLightmaps;
	int	nColorLevel;
	int	n3DGlasses;
	int   nScreenDist;
	int   n3DMethod;
	int	nStereoSeparation;
	int	nEnhance3D;
	int	nColorGain;
	int	nDeghost;
	int	nFlipFrames;
	int	nFastScreen;
	int	nCameras;
	int	nLights;
	int	nPasses;
	int	nWallTransp;
	int	nContrast;
	int	nBrightness;
} renderOpts;

#if 1 //DBG
static int fpsTable [] = {-1, 0, 10, 20, 30, 60}; //40, 50, 60, 70, 80, 90, 100, 125, 150, 200, 250};
#else
static int fpsTable [] = {0, 60};
#endif

static const char *pszRendQual [4];
static const char *pszMeshQual [5];
static const char *pszImgQual [5];
static const char *pszColorLevel [3];
static const char *psz3DGlasses [5];
static const char *pszEnhance3D [4];
static const char *pszDeghost [5];
static const char *psz3DMethod [2];
static const char *pszStereoSeparation [] = {"0.25", "0.5", "0.75", "1.0", "1.25", "1.5", "1.75", "2.0", "2.25", "2.5", "2.75", "3.0"};

static int xStereoSeparation = 0;

//------------------------------------------------------------------------------

static inline const char *ContrastText (void)
{
return (ogl.m_states.nContrast == 8) 
		 ? TXT_STANDARD 
		 : (ogl.m_states.nContrast < 8) 
			? TXT_LOW 
			: TXT_HIGH;
}

//------------------------------------------------------------------------------

int FindTableFps (int nFps)
{
	int	i, j = 0, d, dMin = 0x7fffffff;

for (i = 0; i < int (sizeofa (fpsTable)); i++) {
	d = abs (nFps - fpsTable [i]);
	if (d < dMin) {
		j = i;
		dMin = d;
		}
	}
return j;
}

//------------------------------------------------------------------------------

static int nPowerups, nCameras, nLighting;

static const char* pszNoneBasicAdv [3];
static const char* pszNoneBasicFull [3];
static const char* pszNoneBasicStdFull [4];
static const char *pszQuality [4];

int RenderOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			h, v;

if (!gameStates.app.bNostalgia) {
	m = menu + renderOpts.nBrightness;
	v = m->m_value;
	if (v != paletteManager.GetGamma ())
		paletteManager.SetGamma (v);
	}

#if !DBG
if (gameOpts->app.bNotebookFriendly)
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
#if _WIN32
		if (gameStates.render.bVSyncOk)
			wglSwapIntervalEXT (v < 0);
#endif
		gameOpts->render.nMaxFPS = v;
		gameStates.render.bVSync = (v < 0);
		m->m_bRebuild = 1;
		}
	}

if (renderOpts.nColorLevel >= 0) {
	m = menu + renderOpts.nColorLevel;
	v = m->m_value;
	if (gameOpts->render.color.nLevel != v) {
		gameOpts->render.color.nLevel = v;
		sprintf (m->m_text, TXT_LIGHTCOLOR, pszColorLevel [gameOpts->render.color.nLevel]);
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

if (renderOpts.n3DGlasses >= 0) {
	m = menu + renderOpts.n3DGlasses;
	v = m->m_value;
	if ((h = gameOpts->render.n3DGlasses) != v) {
		transparencyRenderer.ResetBuffers ();
		gameOpts->render.n3DGlasses = v;
		sprintf (m->m_text, TXT_STEREO_VIEW, psz3DGlasses [v]);
		m->m_bRebuild = -1;
		key = -2;
		return nCurItem;
		}

	if (renderOpts.nStereoSeparation >= 0) {
		m = menu + renderOpts.nStereoSeparation;
		v = m->m_value;
		if (xStereoSeparation != v) {
			xStereoSeparation = v;
			gameOpts->render.xStereoSeparation = (EXPERTMODE ? (xStereoSeparation + 1) : 3) * (STEREO_SEPARATION_STEP);
			sprintf (m->m_text, TXT_STEREO_SEPARATION, pszStereoSeparation [v]);
			m->m_bRebuild = -1;
			}
		}

	if (renderOpts.n3DMethod >= 0) {
		m = menu + renderOpts.n3DMethod;
		v = m->m_value;
		if (gameOpts->render.n3DMethod != v) {
			gameOpts->render.n3DMethod = v;
			sprintf (m->m_text, TXT_3D_METHOD, psz3DMethod [v]);
			m->m_bRebuild = -1;
			}
		}

	if (renderOpts.nScreenDist >= 0) {
		m = menu + renderOpts.nScreenDist;
		v = m->m_value;
		if (gameOpts->render.nScreenDist != v) {
			gameOpts->render.nScreenDist = v;
			sprintf (m->m_text, TXT_3D_SCREEN_DIST, nScreenDists [v]);
			m->m_bRebuild = -1;
			}
		}

	if (renderOpts.nDeghost >= 0) {
		m = menu + renderOpts.nDeghost;
		v = m->m_value;
		if (gameOpts->render.bDeghost != v) {
			if ((v == 4) || (gameOpts->render.bDeghost == 4))
				key = -2;
			gameOpts->render.bDeghost = v;
			sprintf (m->m_text, TXT_3D_DEGHOST, pszDeghost [v]);
			m->m_bRebuild = -1;
			if (key == -2)
				return nCurItem;
			}
		}

	if (renderOpts.nColorGain >= 0) {
		m = menu + renderOpts.nColorGain;
		v = m->m_value;
		if (gameOpts->render.bColorGain != v) {
			gameOpts->render.bColorGain = v;
			sprintf (m->m_text, TXT_COLORGAIN, pszEnhance3D [v]);
			m->m_bRebuild = -1;
			}
		}

	if (renderOpts.nEnhance3D >= 0) {
		m = menu + renderOpts.nEnhance3D;
		v = m->m_value;
		if (gameOpts->render.bEnhance3D != v) {
			gameOpts->render.bEnhance3D = v;
			m->m_bRebuild = -1;
			key = -2;
			return nCurItem;
			}
		}

	if (renderOpts.nFlipFrames >= 0) 
		gameOpts->render.bFlipFrames = menu [renderOpts.nFlipFrames].m_value;
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

pszColorLevel [0] = TXT_OFF;
pszColorLevel [1] = TXT_WEAPONS;
pszColorLevel [2] = TXT_FULL;

psz3DGlasses [0] = TXT_NONE;
psz3DGlasses [1] = TXT_AMBER_BLUE;
psz3DGlasses [2] = TXT_RED_CYAN;
psz3DGlasses [3] = TXT_GREEN_MAGENTA;
psz3DGlasses [4] = TXT_SHUTTER;

pszDeghost [0] = TXT_OFF;
pszDeghost [1] = TXT_LOW;
pszDeghost [2] = TXT_MEDIUM;
pszDeghost [3] = TXT_HIGH;
pszDeghost [4] = TXT_3D_DUBOIS;

pszEnhance3D [0] = TXT_OFF;
pszEnhance3D [1] = TXT_LOW;
pszEnhance3D [2] = TXT_MEDIUM;
pszEnhance3D [3] = TXT_HIGH;

psz3DMethod [0] = TXT_3D_PARALLEL;
psz3DMethod [1] = TXT_3D_TOE_IN;

lightManager.SetMethod ();
nLighting = (gameOpts->render.nLightingMethod == 0)
				? 0
				: (gameOpts->render.nLightingMethod == 2)
					? 3
					: (gameStates.render.bLightmapsOk && gameOpts->render.bUseLightmaps) + 1;
nPowerups = gameOpts->render.powerups.b3D ? gameOpts->render.powerups.b3DShields ? 2 : 1 : 0;
nCameras = extraGameInfo [0].bUseCameras ? gameOpts->render.cameras.bHires ? 2 : 1 : 0;
xStereoSeparation = gameOpts->render.xStereoSeparation / (STEREO_SEPARATION_STEP) - 1;
if (xStereoSeparation < 0)
	xStereoSeparation = 0;
else if (xStereoSeparation >= sizeofa (pszStereoSeparation))
	xStereoSeparation = sizeofa (pszStereoSeparation) - 1;

do {
	m.Destroy ();
	m.Create (50);
#if !DBG
	if (!gameOpts->app.bNotebookFriendly)
		renderOpts.nFrameCap = m.AddCheck (TXT_VSYNC, gameOpts->render.nMaxFPS == 1, KEY_V, HTX_RENDER_FRAMECAP);
#endif
	if (!gameStates.app.bNostalgia)
		renderOpts.nBrightness = m.AddSlider (TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
	m.AddText ("");
#if !DBG
	if (gameOpts->app.bNotebookFriendly)
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
		renderOpts.nLighting = 
		renderOpts.nColorLevel =
		renderOpts.nLightmaps = -1;
	else {
		sprintf (szSlider + 1, TXT_LIGHTING, pszQuality [nLighting]);
		*szSlider = *(TXT_LIGHTING + 1);
		renderOpts.nLighting = m.AddSlider (szSlider + 1, nLighting, 0, (gameOpts->render.bUseShaders && ogl.m_states.bShadersOk) ? 3 : 1, KEY_L, HTX_LIGHTING);
		if (nLighting >= 2) {
			sprintf (szSlider + 1, TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
			*szSlider = *(TXT_LMAP_QUALITY + 1);
			renderOpts.nLightmaps = m.AddSlider (szSlider + 1, gameOpts->render.nLightmapQuality, 0, 3, KEY_M, HTX_LMAP_QUALITY);

			if (nLighting == 3) {
				sprintf (szSlider + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
				*szSlider = *(TXT_MAX_LIGHTS_PER_PASS - 1);
				renderOpts.nLights = m.AddSlider (szSlider + 1, gameOpts->ogl.nMaxLightsPerPass - 5, 0, 8 - MIN_LIGHTS_PER_PASS, KEY_P, HTX_MAX_LIGHTS_PER_PASS);
				}
			}
		sprintf (szSlider + 1, TXT_LIGHTCOLOR, pszColorLevel [gameOpts->render.color.nLevel]);
		*szSlider = *(TXT_LIGHTCOLOR + 1);
		renderOpts.nColorLevel = m.AddSlider (szSlider + 1, gameOpts->render.color.nLevel, 0, 2, KEY_C, HTX_RENDER_LIGHTCOLOR);
		m.AddText ("", 0);
		}
	sprintf (szSlider + 1, TXT_IMAGE_QUALITY, pszImgQual [gameOpts->render.nImageQuality]);
	*szSlider = *(TXT_IMAGE_QUALITY - 1);
	renderOpts.nImageQual = m.AddSlider (szSlider + 1, gameOpts->render.nImageQuality, 0, 4, KEY_I, HTX_ADVRND_RENDQUAL);
	sprintf (szSlider + 1, TXT_RENDER_QUALITY, pszRendQual [gameOpts->render.nQuality]);
	*szSlider = *(TXT_RENDER_QUALITY + 1);
	renderOpts.nRenderQual = m.AddSlider (szSlider + 1, gameOpts->render.nQuality, 0, 3, KEY_R, HTX_ADVRND_TEXQUAL);

	if (gameStates.app.bGameRunning)
		renderOpts.nMeshQual = -1;
	else {
		if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
			sprintf (szSlider + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			*szSlider = *(TXT_MESH_QUALITY + 1);
			renderOpts.nMeshQual = m.AddSlider (szSlider + 1, gameOpts->render.nMeshQuality, 0, 3, KEY_V, HTX_MESH_QUALITY);
			}
		else
			renderOpts.nMeshQual = -1;
		}
	sprintf (szSlider + 1, TXT_CAMERAS, pszNoneBasicFull [nCameras]);
	*szSlider = *(TXT_CAMERAS - 1);
	renderOpts.nCameras = m.AddSlider (szSlider + 1, nCameras, 0, 2, KEY_A, HTX_CAMERAS);
	sprintf (szSlider + 1, TXT_POWERUPS, pszNoneBasicFull [nPowerups]);
	*szSlider = *(TXT_POWERUPS - 1);
	renderOpts.nPowerups = m.AddSlider (szSlider + 1, nPowerups, 0, 2, KEY_O, HTX_POWERUPS);

	if (EXPERTMODE && gameOpts->render.n3DGlasses)
		m.AddText ("");
	sprintf (szSlider + 1, TXT_STEREO_VIEW, psz3DGlasses [gameOpts->render.n3DGlasses]);
	*szSlider = *(TXT_STEREO_VIEW - 1);
	renderOpts.n3DGlasses = m.AddSlider (szSlider + 1, gameOpts->render.n3DGlasses, 0, sizeofa (psz3DGlasses) - 2 + gameStates.render.bHaveStereoBuffers, KEY_G, HTX_STEREO_VIEW);	//exclude shutter
	renderOpts.n3DMethod = 
	renderOpts.nScreenDist =
	renderOpts.nColorGain =
	renderOpts.nEnhance3D = 
	renderOpts.nDeghost =
	renderOpts.nFlipFrames = 
	renderOpts.nStereoSeparation = -1;

	if (EXPERTMODE && gameOpts->render.n3DGlasses) {
		sprintf (szSlider + 1, TXT_3D_METHOD, psz3DMethod [gameOpts->render.n3DMethod]);
		*szSlider = *(TXT_3D_METHOD - 1);
		renderOpts.n3DMethod = m.AddSlider (szSlider + 1, gameOpts->render.n3DMethod, 0, sizeofa (psz3DMethod) - 1, KEY_J, HTX_3D_METHOD);

		sprintf (szSlider + 1, TXT_STEREO_SEPARATION, pszStereoSeparation [xStereoSeparation]);
		*szSlider = *(TXT_STEREO_SEPARATION - 1);
		renderOpts.nStereoSeparation = m.AddSlider (szSlider + 1, xStereoSeparation, 0, sizeofa (pszStereoSeparation) - 1, KEY_E, HTX_STEREO_SEPARATION);

		sprintf (szSlider + 1, TXT_3D_SCREEN_DIST, nScreenDists [gameOpts->render.nScreenDist]);
		*szSlider = *(TXT_3D_SCREEN_DIST - 1);
		renderOpts.nScreenDist = m.AddSlider (szSlider + 1, gameOpts->render.nScreenDist, 0, sizeofa (nScreenDists) - 1, KEY_S, HTX_3D_SCREEN_DIST);

		if (ogl.Enhance3D () > 0) {
			sprintf (szSlider + 1, TXT_3D_DEGHOST, pszDeghost [gameOpts->render.bDeghost]);
			*szSlider = *(TXT_3D_DEGHOST - 1);
			renderOpts.nDeghost = m.AddSlider (szSlider + 1, gameOpts->render.bDeghost, 0, sizeofa (pszDeghost) - 2 + (ogl.Enhance3D (1) == 2), KEY_H, HTX_3D_DEGHOST);
			if (gameOpts->render.bDeghost < 4) {
				sprintf (szSlider + 1, TXT_COLORGAIN, pszEnhance3D [gameOpts->render.bColorGain]);
				*szSlider = *(TXT_COLORGAIN - 1);
				renderOpts.nColorGain = m.AddSlider (szSlider + 1, gameOpts->render.bColorGain, 0, sizeofa (pszEnhance3D) - 1, KEY_G, HTX_COLORGAIN);
				}
			}
		m.AddText ("");
		if (ogl.Enhance3D (1) > 0)
			renderOpts.nEnhance3D = m.AddCheck (TXT_ENHANCE_3D, gameOpts->render.bEnhance3D, KEY_D, HTX_ENHANCE_3D);
#if 0
		renderOpts.nFlipFrames = m.AddCheck (TXT_FLIPFRAMES, gameOpts->render.bFlipFrames, KEY_F, HTX_FLIPFRAMES);
#endif
		}
	else
		m.AddText ("");
	optSubTitles = m.AddCheck (TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_V, HTX_RENDER_SUBTTL);

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

	if ((extraGameInfo [0].bUseCameras = (nCameras != 0)))
		gameOpts->render.cameras.bHires = (nCameras == 2);
	if ((gameOpts->render.powerups.b3D = (nPowerups != 0)))
		gameOpts->render.powerups.b3DShields = (nPowerups == 2);
	gameOpts->movies.bSubTitles = (m [optSubTitles].m_value != 0);

#if !DBG
	if (!gameOpts->app.bNotebookFriendly)
		gameOpts->render.nMaxFPS = m [renderOpts.nFrameCap].m_value ? 1 : 60;
#endif
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [renderOpts.nBrightness].m_value);
	if (nRendQualSave != gameOpts->render.nImageQuality)
		ogl.SetRenderQuality ();

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
