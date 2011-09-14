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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
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

#if 1 //DBG
static int fpsTable [] = {-1, 0, 10, 20, 30, 60, 120}; //40, 50, 60, 70, 80, 90, 100, 125, 150, 200, 250};
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
static const char *pszPrecision [3];

int RenderOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			h, v;

if (!gameStates.app.bNostalgia) {
	if ((m = menu ["brightness"])) {
		v = m->Value ();
		if (v != paletteManager.GetGamma ()) {
			paletteManager.SetGamma (v);
			sprintf (m->Text (), TXT_BRIGHTNESS, paletteManager.BrightnessLevel ());
			m->m_bRebuild = 1;
			}
		}
	}

#if !DBG
if (gameOpts->app.bNotebookFriendly)
#endif
{
	if ((m = menu ["frame cap"])) {
		v = fpsTable [m->Value ()];
		if (gameOpts->render.nMaxFPS != v) {
			if (v > 0)
				sprintf (m->Text (), TXT_FRAMECAP, v);
			else if (v < 0) {
				if (!gameStates.render.bVSyncOk) {
					m->Value () = 1;
					return nCurItem;
					}
				sprintf (m->Text (), TXT_VSYNC);
				}
			else
				sprintf (m->Text (), TXT_NO_FRAMECAP);
#if _WIN32
			if (gameStates.render.bVSyncOk)
				wglSwapIntervalEXT (v < 0);
#endif
			gameOpts->render.nMaxFPS = v;
			gameStates.render.bVSync = (v < 0);
			m->m_bRebuild = 1;
			}
		}
	}

if ((m = menu ["colorization"])) {
	v = m->Value ();
	if (gameOpts->render.color.nLevel != v) {
		gameOpts->render.color.nLevel = v;
		sprintf (m->Text (), TXT_LIGHTCOLOR, pszColorLevel [gameOpts->render.color.nLevel]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["image quality"])) {
	v = m->Value ();
	if (gameOpts->render.nImageQuality != v) {
		gameOpts->render.nImageQuality = v;
		sprintf (m->Text (), TXT_IMAGE_QUALITY, pszImgQual [gameOpts->render.nImageQuality]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["render quality"])) {
	v = m->Value ();
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->Text (), TXT_RENDER_QUALITY, pszRendQual [gameOpts->render.nQuality]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["mesh quality"])) {
	v = m->Value ();
	if (gameOpts->render.nMeshQuality != v) {
		gameOpts->render.nMeshQuality = v;
		sprintf (m->Text (), TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["3D glasses"])) {
	v = m->Value ();
	if ((h = gameOpts->render.stereo.nGlasses) != v) {
		transparencyRenderer.ResetBuffers ();
		gameOpts->render.stereo.nGlasses = v;
		sprintf (m->Text (), TXT_STEREO_VIEW, psz3DGlasses [v]);
		m->m_bRebuild = -1;
		key = -2;
		return nCurItem;
		}

	if ((m = menu ["stereo separation"])) {
		v = m->Value ();
		if (xStereoSeparation != v) {
			xStereoSeparation = v;
			gameOpts->render.stereo.xSeparation = (EXPERTMODE ? (xStereoSeparation + 1) : 3) * (STEREO_SEPARATION_STEP);
			sprintf (m->Text (), TXT_STEREO_SEPARATION, pszStereoSeparation [v]);
			m->m_bRebuild = -1;
			}
		}

	if ((m = menu ["3D method"])) {
		v = m->Value ();
		if (gameOpts->render.stereo.nMethod != v) {
			gameOpts->render.stereo.nMethod = v;
			sprintf (m->Text (), TXT_3D_METHOD, psz3DMethod [v]);
			m->m_bRebuild = -1;
			}
		}

	if ((m = menu ["screen distance"])) {
		v = m->Value ();
		if (gameOpts->render.stereo.nScreenDist != v) {
			gameOpts->render.stereo.nScreenDist = v;
			sprintf (m->Text (), TXT_3D_SCREEN_DIST, nScreenDists [v]);
			m->m_bRebuild = -1;
			}
		}

	if ((m = menu ["deghosting"])) {
		v = m->Value ();
		if (gameOpts->render.stereo.bDeghost != v) {
			if ((v == 4) || (gameOpts->render.stereo.bDeghost == 4))
				key = -2;
			gameOpts->render.stereo.bDeghost = v;
			sprintf (m->Text (), TXT_3D_DEGHOST, pszDeghost [v]);
			m->m_bRebuild = -1;
			if (key == -2)
				return nCurItem;
			}
		}

	if ((m = menu ["color gain"])) {
		v = m->Value ();
		if (gameOpts->render.stereo.bColorGain != v) {
			gameOpts->render.stereo.bColorGain = v;
			sprintf (m->Text (), TXT_COLORGAIN, pszEnhance3D [v]);
			m->m_bRebuild = -1;
			}
		}

	if ((m = menu ["enhance 3D"])) {
		v = m->Value ();
		if (gameOpts->render.stereo.bEnhance != v) {
			gameOpts->render.stereo.bEnhance = v;
			m->m_bRebuild = -1;
			key = -2;
			return nCurItem;
			}
		}

	gameOpts->render.stereo.bFlipFrames = menu.Value ("flip frames");
	gameOpts->render.stereo.bBrighten = menu.Value ("brighten scene");
	}

if ((m = menu ["cameras"])) {
	v = m->Value ();
	if (nCameras != v) {
		if ((nCameras = v)) {
			gameOpts->render.cameras.bHires = (nCameras == 2);
			cameraManager.ReAlign ();
			}
		sprintf (m->Text (), TXT_CAMERAS, pszNoneBasicFull [nCameras]);
		m->m_bRebuild = -1;
		}
	}

if ((m = menu ["powerup quality"])) {
	v = m->Value ();
	if (nPowerups != v) {
		nPowerups = v;
		sprintf (m->Text (), TXT_POWERUPS, pszNoneBasicFull [nPowerups]);
		m->m_bRebuild = -1;
		}
	}	

if (!gameStates.app.bGameRunning) {
	if ((m = menu ["lighting method"])) {
		v = m->Value ();
		if (nLighting != v) {
			nLighting = v;
			sprintf (m->Text (), TXT_LIGHTING, pszQuality [nLighting]);
			key = -2;
			return nCurItem;
			}
		}	

	if ((m = menu ["lightmap quality"])) {
		v = m->Value ();
		if (gameOpts->render.nLightmapQuality != v) {
			gameOpts->render.nLightmapQuality = v;
			sprintf (m->Text (), TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
			m->m_bRebuild = 1;
			}
		}

	if ((m = menu ["lightmap precision"])) {
		v = m->Value ();
		if (gameOpts->render.nLightmapPrecision != v) {
			gameOpts->render.nLightmapPrecision = v;
			sprintf (m->Text (), TXT_LMAP_PRECISION, pszPrecision [gameOpts->render.nLightmapPrecision]);
			m->m_bRebuild = 1;
			}
		}

	if ((m = menu ["light sources"])) {
		v = m->Value ();
		if (v != gameOpts->ogl.nMaxLightsPerPass - MIN_LIGHTS_PER_PASS) {
			gameOpts->ogl.nMaxLightsPerPass = v + MIN_LIGHTS_PER_PASS;
			sprintf (m->Text (), TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			key = -2;
			return nCurItem;
			}
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

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

pszPrecision [0] = TXT_BASIC;
pszPrecision [1] = TXT_STANDARD;
pszPrecision [2] = TXT_HIGH;

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
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenu	m;
	int	i;
	int	nRendQualSave = gameOpts->render.nImageQuality;

	static int choice = 0;

	char szSlider [50];

InitStrings ();

lightManager.SetMethod ();
nLighting = (gameOpts->render.nLightingMethod == 0)
				? 0
				: (gameOpts->render.nLightingMethod == 2)
					? 3
					: (gameStates.render.bLightmapsOk && gameOpts->render.bUseLightmaps) + 1;
nPowerups = gameOpts->Use3DPowerups () ? gameOpts->render.powerups.b3DShields ? 2 : 1 : 0;
nCameras = extraGameInfo [0].bUseCameras ? gameOpts->render.cameras.bHires ? 2 : 1 : 0;
xStereoSeparation = gameOpts->render.stereo.xSeparation / (STEREO_SEPARATION_STEP) - 1;
if (xStereoSeparation < 0)
	xStereoSeparation = 0;
else if (xStereoSeparation >= (int) sizeofa (pszStereoSeparation))
	xStereoSeparation = sizeofa (pszStereoSeparation) - 1;

do {
	m.Destroy ();
	m.Create (50);
#if !DBG
	if (!gameOpts->app.bNotebookFriendly)
		m.AddCheck ("frame cap", TXT_VSYNC, gameOpts->render.nMaxFPS == 1, KEY_V, HTX_RENDER_FRAMECAP);
#endif
	if (!gameStates.app.bNostalgia) {
		sprintf (szSlider + 1, TXT_BRIGHTNESS, paletteManager.BrightnessLevel ());
		*szSlider = *(TXT_BRIGHTNESS - 1);
		m.AddSlider ("brightness", szSlider + 1, paletteManager.GetGamma (), 0, 15, KEY_B, HTX_RENDER_BRIGHTNESS);
		}
	m.AddText ("", "");
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
		m.AddSlider ("frame cap", szSlider + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, sizeofa (fpsTable) - 1, KEY_F, HTX_RENDER_FRAMECAP);
		}

	if (!(gameStates.app.bGameRunning || gameStates.app.bNostalgia)) {
		sprintf (szSlider + 1, TXT_LIGHTING, pszQuality [nLighting]);
		*szSlider = *(TXT_LIGHTING - 1);
		m.AddSlider ("lighting method", szSlider + 1, nLighting, 0, (gameOpts->render.bUseShaders && ogl.m_features.bShaders) ? 3 : 1, KEY_L, HTX_LIGHTING);
		if (nLighting >= 2) {
			sprintf (szSlider + 1, TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
			*szSlider = *(TXT_LMAP_QUALITY - 1);
			m.AddSlider ("lightmap quality", szSlider + 1, gameOpts->render.nLightmapQuality, 0, 3, KEY_M, HTX_LMAP_QUALITY);

			if (gameOpts->app.bExpertMode) {
				sprintf (szSlider + 1, TXT_LMAP_PRECISION, pszPrecision [gameOpts->render.nLightmapPrecision]);
				*szSlider = *(TXT_LMAP_PRECISION - 1);
				m.AddSlider ("lightmap precision", szSlider + 1, gameOpts->render.nLightmapPrecision, 0, 2, KEY_P, HTX_LMAP_PRECISION);
				}

			if (nLighting == 3) {
				sprintf (szSlider + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
				*szSlider = *(TXT_MAX_LIGHTS_PER_PASS - 1);
				m.AddSlider ("light sources", szSlider + 1, gameOpts->ogl.nMaxLightsPerPass - 5, 0, 8 - MIN_LIGHTS_PER_PASS, KEY_P, HTX_MAX_LIGHTS_PER_PASS);
				}
			}
		sprintf (szSlider + 1, TXT_LIGHTCOLOR, pszColorLevel [gameOpts->render.color.nLevel]);
		*szSlider = *(TXT_LIGHTCOLOR - 1);
		m.AddSlider ("colorization", szSlider + 1, gameOpts->render.color.nLevel, 0, 2, KEY_C, HTX_RENDER_LIGHTCOLOR);
		m.AddText ("", "");
		}
	sprintf (szSlider + 1, TXT_IMAGE_QUALITY, pszImgQual [gameOpts->render.nImageQuality]);
	*szSlider = *(TXT_IMAGE_QUALITY - 1);
	m.AddSlider ("image quality", szSlider + 1, gameOpts->render.nImageQuality, 0, 4, KEY_I, HTX_ADVRND_RENDQUAL);
	sprintf (szSlider + 1, TXT_RENDER_QUALITY, pszRendQual [gameOpts->render.nQuality]);
	*szSlider = *(TXT_RENDER_QUALITY + 1);
	m.AddSlider ("render quality", szSlider + 1, gameOpts->render.nQuality, 0, 3, KEY_R, HTX_ADVRND_TEXQUAL);

	if (!gameStates.app.bGameRunning) {
		if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
			sprintf (szSlider + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			*szSlider = *(TXT_MESH_QUALITY - 1);
			m.AddSlider ("mesh quality", szSlider + 1, gameOpts->render.nMeshQuality, 0, 3, KEY_V, HTX_MESH_QUALITY);
			}
		}
	sprintf (szSlider + 1, TXT_CAMERAS, pszNoneBasicFull [nCameras]);
	*szSlider = *(TXT_CAMERAS - 1);
	m.AddSlider ("cameras", szSlider + 1, nCameras, 0, 2, KEY_A, HTX_CAMERAS);
	sprintf (szSlider + 1, TXT_POWERUPS, pszNoneBasicFull [nPowerups]);
	*szSlider = *(TXT_POWERUPS - 1);
	m.AddSlider ("powerup quality", szSlider + 1, nPowerups, 0, 2, KEY_O, HTX_POWERUPS);

	if (EXPERTMODE && gameOpts->render.stereo.nGlasses)
		m.AddText ("", "");
	sprintf (szSlider + 1, TXT_STEREO_VIEW, psz3DGlasses [gameOpts->render.stereo.nGlasses]);
	*szSlider = *(TXT_STEREO_VIEW - 1);
	m.AddSlider ("3D glasses", szSlider + 1, gameOpts->render.stereo.nGlasses, 0, sizeofa (psz3DGlasses) - 2 + ogl.m_features.bStereoBuffers, KEY_G, HTX_STEREO_VIEW);	//exclude shutter

	if (EXPERTMODE && gameOpts->render.stereo.nGlasses) {
		sprintf (szSlider + 1, TXT_3D_METHOD, psz3DMethod [gameOpts->render.stereo.nMethod]);
		*szSlider = *(TXT_3D_METHOD - 1);
		m.AddSlider ("3D method", szSlider + 1, gameOpts->render.stereo.nMethod, 0, sizeofa (psz3DMethod) - 1, KEY_J, HTX_3D_METHOD);

		sprintf (szSlider + 1, TXT_STEREO_SEPARATION, pszStereoSeparation [xStereoSeparation]);
		*szSlider = *(TXT_STEREO_SEPARATION - 1);
		m.AddSlider ("stereo separation", szSlider + 1, xStereoSeparation, 0, sizeofa (pszStereoSeparation) - 1, KEY_E, HTX_STEREO_SEPARATION);

		sprintf (szSlider + 1, TXT_3D_SCREEN_DIST, nScreenDists [gameOpts->render.stereo.nScreenDist]);
		*szSlider = *(TXT_3D_SCREEN_DIST - 1);
		m.AddSlider ("screen distance", szSlider + 1, gameOpts->render.stereo.nScreenDist, 0, sizeofa (nScreenDists) - 1, KEY_S, HTX_3D_SCREEN_DIST);

		if (ogl.Enhance3D () > 0) {
			sprintf (szSlider + 1, TXT_3D_DEGHOST, pszDeghost [gameOpts->render.stereo.bDeghost]);
			*szSlider = *(TXT_3D_DEGHOST - 1);
			m.AddSlider ("deghosting", szSlider + 1, gameOpts->render.stereo.bDeghost, 0, sizeofa (pszDeghost) - 2 + (ogl.Enhance3D (1) == 2), KEY_H, HTX_3D_DEGHOST);
			if (gameOpts->render.stereo.bDeghost < 4) {
				sprintf (szSlider + 1, TXT_COLORGAIN, pszEnhance3D [gameOpts->render.stereo.bColorGain]);
				*szSlider = *(TXT_COLORGAIN - 1);
				m.AddSlider ("color gain", szSlider + 1, gameOpts->render.stereo.bColorGain, 0, sizeofa (pszEnhance3D) - 1, KEY_G, HTX_COLORGAIN);
				}
			}
		m.AddText ("", "");
		m.AddCheck ("brighten scene", TXT_BUMP_BRIGHTNESS, gameOpts->render.stereo.bBrighten, KEY_T, HTX_BUMP_BRIGHTNESS);
		if (ogl.Enhance3D (1) > 0)
			m.AddCheck ("enhance 3D", TXT_ENHANCE_3D, gameOpts->render.stereo.bEnhance, KEY_D, HTX_ENHANCE_3D);
#if 0
		m.AddCheck ("flip frames", TXT_FLIPFRAMES, gameOpts->render.stereo.bFlipFrames, KEY_F, HTX_FLIPFRAMES);
#endif
		}
	m.AddText ("", "");
	m.AddCheck ("movie subtitles", TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_V, HTX_RENDER_SUBTTL);

#if DBG
	if (EXPERTMODE) {
		m.AddText ("", "");
		m.AddCheck ("draw wire frame", "Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
		m.AddCheck ("draw textures", "Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
		m.AddCheck ("draw walls", "Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
		m.AddCheck ("draw objects", "Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
		m.AddCheck ("dynamic light", "Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
		}
#endif

	do {
		i = m.Menu (NULL, TXT_RENDER_OPTS, RenderOptionsCallback, &choice);
		} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = (nCameras != 0)))
		gameOpts->render.cameras.bHires = (nCameras == 2);
	if ((gameOpts->render.powerups.b3D = (nPowerups != 0) || gameStates.app.bStandalone))
		gameOpts->render.powerups.b3DShields = (nPowerups == 2);
	if (m.Available ("movie subtitles"))
		gameOpts->movies.bSubTitles = (m.Value ("movie subtitles") != 0);

#if !DBG
	if (!gameOpts->app.bNotebookFriendly)
		gameOpts->render.nMaxFPS = m.Value ("frame cap") ? 1 : 120;
#endif
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m.Value ("brightness"));
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
		gameOpts->render.debug.bWireFrame = m.Value ("draw wire frame");
		gameOpts->render.debug.bTextures = m.Value ("draw textures");
		gameOpts->render.debug.bObjects = m.Value ("draw objects");
		gameOpts->render.debug.bWalls = m.Value ("draw walls");
		gameOpts->render.debug.bDynamicLight = m.Value ("dynamic light");
		}
#endif
	} while (i == -2);

lightManager.SetMethod ();
DefaultRenderSettings ();
}

//------------------------------------------------------------------------------
//eof
