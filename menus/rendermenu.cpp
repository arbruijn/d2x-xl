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
#include "paging.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

void DefaultRenderSettings (bool bSetup = false);

#define EXPERTMODE	gameOpts->app.bExpertMode

#define MIN_LIGHTS_PER_PASS 5

#define STEREO_SEPARATION_STEP	(I2X (1) / 8)
#define VR_IPD_STEP				(I2X (1) / 64)
#define LIGHT_CONTRIB_SCALE		5

//------------------------------------------------------------------------------

#if 1 //DBG
static int32_t fpsTable [] = {-1, 0, 10, 20, 30, 60, 120}; //40, 50, 60, 70, 80, 90, 100, 125, 150, 200, 250};
#else
static int32_t fpsTable [] = {0, 60};
#endif

static const char *pszRendQual [4];
static const char *pszRendStyle [2];
static const char *pszMeshQual [5];
static const char *pszImgQual [6];
static const char *pszColorLevel [3];
static const char *pszStereoDevice [5];
static const char *pszAnaglyphColors [3];
static const char *pszEnhance3D [4];
static const char *pszDeghost [5];
static const char *psz3DMethod [2];
static const char *pszStereoSeparation [] = {"0.25", "0.5", "0.75", "1.0", "1.25", "1.5", "1.75", "2.0", "2.25", "2.5", "2.75", "3.0", "3.25", "3.5", "3.75", "4.0", "4.25", "4.5", "4.75", "5.0"};
static const char *pszFOV [5];

static int32_t stereoDeviceMap [5];
static int32_t xStereoSeparation = 0;
static int32_t nStereoDevice = 0;
static int32_t nStereoDeviceCount = 0;
static int32_t nAnaglyphColor = 0;
static int32_t nIPD = VR_DEFAULT_IPD - VR_MIN_IPD;

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

int32_t FindTableFps (int32_t nFps)
{
	int32_t	i, j = 0, d, dMin = 0x7fffffff;

for (i = 0; i < int32_t (sizeofa (fpsTable)); i++) {
	d = abs (nFps - fpsTable [i]);
	if (d < dMin) {
		j = i;
		dMin = d;
		}
	}
return j;
}

//------------------------------------------------------------------------------

static int32_t nPowerups, nCameras, nLighting;

static const char* pszNoneBasicAdv [3];
static const char* pszNoneBasicFull [3];
static const char* pszNoneBasicStdFull [4];
static const char *pszQuality [4];
static const char *pszPrecision [3];

//------------------------------------------------------------------------------

void UpdateLightmapOptions (CMenu& menu)
{
	CMenuItem*	m;

if ((m = menu ["direct light"])) {
	int32_t nDirectLight = Max (0, 100 - gameOpts->render.color.nAmbientLight - gameOpts->render.color.nSpecularLight);
	sprintf (m->Text (), TXT_DIRECT_LIGHT, nDirectLight);
	m->Value () = nDirectLight / LIGHT_CONTRIB_SCALE;
	m->m_bRebuild = 1;
	}

if ((m = menu ["diffuse light"])) {
	sprintf (m->Text (), TXT_DIFFUSE_LIGHT, gameOpts->render.color.nAmbientLight);
	m->Value () = gameOpts->render.color.nAmbientLight / LIGHT_CONTRIB_SCALE;
	m->m_bRebuild = 1;
	}

if ((m = menu ["specular light"])) {
	sprintf (m->Text (), TXT_SPECULAR_LIGHT, gameOpts->render.color.nSpecularLight);
	m->Value () = gameOpts->render.color.nSpecularLight / LIGHT_CONTRIB_SCALE;
	m->m_bRebuild = 1;
	}
}

//------------------------------------------------------------------------------

int32_t LightComponentOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (gameOpts->render.color.bConfigurable) {
		CMenuItem*	m;
		int32_t		v;

	if ((m = menu ["direct light"])) {
		int32_t nDirectLight = Max (0, 100 - gameOpts->render.color.nAmbientLight - gameOpts->render.color.nSpecularLight);
		v = m->Value () * LIGHT_CONTRIB_SCALE;
		if (nDirectLight != v) {
			nDirectLight = v;
	#	if USE_SPECULAR_LIGHT
			gameOpts->render.color.nAmbientLight = Max (0, 100 - nDirectLight - gameOpts->render.color.nSpecularLight);
			gameOpts->render.color.nSpecularLight =  (gameOpts->render.color.nSpecularLight, 100 - nDirectLight - gameOpts->render.color.nAmbientLight);
	#	else
			gameOpts->render.color.nAmbientLight = Clamp (100 - nDirectLight - gameOpts->render.color.nSpecularLight, 0, 100);
	#	endif
			UpdateLightmapOptions (menu);
			}
		}

	if ((m = menu ["diffuse light"])) {
		v = m->Value () * LIGHT_CONTRIB_SCALE;
		if (gameOpts->render.color.nAmbientLight != v) {
			gameOpts->render.color.nAmbientLight = v;
			gameOpts->render.color.nSpecularLight = Min (gameOpts->render.color.nSpecularLight, 100 - gameOpts->render.color.nAmbientLight);
			UpdateLightmapOptions (menu);
			}
		}

	#	if USE_SPECULAR_LIGHT
	if ((m = menu ["specular light"])) {
		v = m->Value () * LIGHT_CONTRIB_SCALE;
		if (gameOpts->render.color.nSpecularLight != v) {
			gameOpts->render.color.nSpecularLight = v;
			gameOpts->render.color.nAmbientLight = Min (gameOpts->render.color.nAmbientLight, 100 - gameOpts->render.color.nSpecularLight);
			UpdateLightmapOptions (menu);
			}
		}
	#	endif
	}

return nCurItem;
}

//------------------------------------------------------------------------------

int32_t LightmapOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
	CMenuItem*	m;
	int32_t		v;

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

LightComponentOptionsCallback (menu, key, nCurItem, nState);

return nCurItem;
}

//------------------------------------------------------------------------------

int32_t RenderOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int32_t		v;

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
if (gameOpts->app.bNotebookFriendly || gameOpts->app.bExpertMode)
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
			if (gameStates.render.bVSyncOk)
				ogl.SwapInterval (v < 0);
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
	if (nStereoDevice != v) {
		transparencyRenderer.ResetBuffers ();
		nStereoDevice = v;
		sprintf (m->Text (), TXT_STEREO_VIEW, pszStereoDevice [v]);
		m->m_bRebuild = -1;
		key = -2;
		return nCurItem;
		}

	if ((m = menu ["stereo separation"])) {
		v = m->Value ();
		if (xStereoSeparation != v) {
			xStereoSeparation = v;
			gameOpts->render.stereo.xSeparation [0] = (xStereoSeparation + 1) * STEREO_SEPARATION_STEP;
			sprintf (m->Text (), TXT_STEREO_SEPARATION, pszStereoSeparation [v]);
			m->m_bRebuild = -1;
			}
		}

	if ((m = menu ["IPD"])) {
		v = m->Value ();
		if (nIPD != v) {
			nIPD = v;
			gameOpts->render.stereo.xSeparation [1] = MM2X (nIPD + VR_MIN_IPD); ///*I2X (1) + */(nIPD + 1) * VR_IPD_STEP;
			sprintf (m->Text (), TXT_VR_IPD, nIPD + VR_MIN_IPD);
			m->m_bRebuild = -1;
			}
		}
#if 1 //DBG
	if ((m = menu ["FOV"])) {
		v = m->Value ();
		if (gameOpts->render.stereo.nVRFOV != v) {
			gameOpts->render.stereo.nVRFOV = EXPERTMODE ? v : VR_DEFAULT_FOV;
			sprintf (m->Text (), TXT_VR_FOV, pszFOV [gameOpts->render.stereo.nVRFOV]);
			m->m_bRebuild = -1;
			}
		}
#endif
	if ((m = menu ["anaglyph color"])) {
		v = m->Value ();
		if (nAnaglyphColor != v) {
			nAnaglyphColor = v;
			sprintf (m->Text (), TXT_ANAGLYPH_COLORS, pszAnaglyphColors [v]);
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
	if ((m = menu ["render style"])) {
		v = m->Value ();
		if (gameOpts->render.bCartoonize != v) {
			gameOpts->render.bCartoonize = v;
			sprintf (m->Text (), TXT_RENDER_STYLE, pszRendStyle [gameOpts->render.bCartoonize]);
			m->m_bRebuild = 1;
			}
		}

	if ((m = menu ["lighting method"])) {
		v = m->Value ();
		if (nLighting != v) {
			nLighting = v;
			sprintf (m->Text (), TXT_LIGHTING, pszQuality [nLighting]);
			key = -2;
			return nCurItem;
			}
		}	

	LightmapOptionsCallback (menu, key, nCurItem, nState);

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

void InitRenderMenuStrings (void)
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

pszRendStyle [0] = TXT_REALISTIC;
pszRendStyle [1] = TXT_CARTOON;

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

stereoDeviceMap [0] = 0;
pszStereoDevice [0] = TXT_NONE;
stereoDeviceMap [1] = 1;
pszStereoDevice [1] = TXT_ANAGLYPH_GLASSES;
pszStereoDevice [2] = TXT_SHUTTER_HDMI;
stereoDeviceMap [2] = GLASSES_SHUTTER_HDMI;
nStereoDeviceCount = 3;
#if !DBG
if (gameData.renderData.vr.Available ()) 
#endif
	{
	pszStereoDevice [nStereoDeviceCount] = TXT_VR;
	stereoDeviceMap [nStereoDeviceCount++] = GLASSES_VR;
	}
if (ogl.m_features.bStereoBuffers) {
	pszStereoDevice [nStereoDeviceCount] = TXT_SHUTTER_NVIDIA;
	stereoDeviceMap [nStereoDeviceCount++] = GLASSES_SHUTTER_NVIDIA;
	}

pszAnaglyphColors [0] = TXT_AMBER_BLUE;
pszAnaglyphColors [1] = TXT_RED_CYAN;
pszAnaglyphColors [2] = TXT_GREEN_MAGENTA;

pszFOV [0] = TXT_MINIMAL;
pszFOV [1] = TXT_SMALL;
pszFOV [2] = TXT_MEDIUM;
pszFOV [3] = TXT_LARGE;
pszFOV [4] = TXT_MAXIMAL;

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

const char *LightmapQualityText (void)
{
return pszQuality [gameOpts->render.nLightmapQuality];
}

const char *LightmapPrecisionText (void)
{
return pszPrecision [gameOpts->render.nLightmapPrecision];
}

//------------------------------------------------------------------------------

void AddLightComponentControls (CMenu& m)
{
if (gameOpts->render.color.bConfigurable) {
	if (gameStates.app.bPrecomputeLightmaps)
		m.AddText ("", "");

	char szSlider [50];

	int32_t nDirectLight = Max (0, 100 - gameOpts->render.color.nAmbientLight - gameOpts->render.color.nSpecularLight);
	sprintf (szSlider + 1, TXT_DIRECT_LIGHT, nDirectLight);
	*szSlider = *(TXT_DIFFUSE_LIGHT - 1);
	m.AddSlider ("direct light", szSlider + 1, nDirectLight / LIGHT_CONTRIB_SCALE, 0, 20, KEY_D, HTX_DIRECT_LIGHT);

	sprintf (szSlider + 1, TXT_DIFFUSE_LIGHT, gameOpts->render.color.nAmbientLight);
	*szSlider = *(TXT_DIFFUSE_LIGHT - 1);
	m.AddSlider ("diffuse light", szSlider + 1, gameOpts->render.color.nAmbientLight / LIGHT_CONTRIB_SCALE, 0, 20, KEY_A, HTX_DIFFUSE_LIGHT);
#	if USE_SPECULAR_LIGHT
	sprintf (szSlider + 1, TXT_SPECULAR_LIGHT, gameOpts->render.color.nSpecularLight);
	*szSlider = *(TXT_SPECULAR_LIGHT - 1);
	m.AddSlider ("specular light", szSlider + 1, gameOpts->render.color.nSpecularLight / LIGHT_CONTRIB_SCALE, 0, 10, KEY_A, HTX_SPECULAR_LIGHT);
#	endif
	}
}

//------------------------------------------------------------------------------

void AddLightmapControls (CMenu& m)
{
	char szSlider [50];

sprintf (szSlider + 1, TXT_LMAP_QUALITY, LightmapQualityText ());
*szSlider = *(TXT_LMAP_QUALITY - 1);
m.AddSlider ("lightmap quality", szSlider + 1, gameOpts->render.nLightmapQuality, 0, 3, KEY_M, HTX_LMAP_QUALITY);

if (gameOpts->app.bExpertMode || gameStates.app.bPrecomputeLightmaps) {
	sprintf (szSlider + 1, TXT_LMAP_PRECISION, LightmapPrecisionText ());
	*szSlider = *(TXT_LMAP_PRECISION - 1);
	m.AddSlider ("lightmap precision", szSlider + 1, gameOpts->render.nLightmapPrecision, 0, 2, KEY_P, HTX_LMAP_PRECISION);
	AddLightComponentControls (m);
	}
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	CMenu		m;
	int32_t	i;
	int32_t	nRendQualSave = gameOpts->render.nImageQuality;

	static int32_t choice = 0;

	char szSlider [50];

InitRenderMenuStrings ();

lightManager.SetMethod ();
nLighting = (gameOpts->render.nLightingMethod == 0)
				? 0
				: (gameOpts->render.nLightingMethod == 2)
					? 3
					: (gameStates.render.bLightmapsOk && gameOpts->render.bUseLightmaps) + 1;
nPowerups = gameOpts->Use3DPowerups () ? gameOpts->render.powerups.b3DShields ? 2 : 1 : 0;
nCameras = extraGameInfo [0].bUseCameras ? gameOpts->render.cameras.bHires ? 2 : 1 : 0;
if ((nStereoDevice = gameOpts->render.stereo.nGlasses)) {
	if (nStereoDevice >= DEVICE_STEREO_PHYSICAL)
		nStereoDevice -= 2;
	else {
		nAnaglyphColor = 3 - nStereoDevice;
		nStereoDevice = 1;
		}
	}
xStereoSeparation = gameOpts->render.stereo.xSeparation [0] / (STEREO_SEPARATION_STEP) - 1;
if (xStereoSeparation < 0)
	xStereoSeparation = 0;
else if (xStereoSeparation >= (int32_t) sizeofa (pszStereoSeparation))
	xStereoSeparation = sizeofa (pszStereoSeparation) - 1;
nIPD = X2MM (gameOpts->render.stereo.xSeparation [1]) - VR_MIN_IPD; //VR_IPD_STEP - 1;

do {
	m.Destroy ();
	m.Create (50, "RenderOptionsMenu");
	if (!gameStates.app.bPrecomputeLightmaps) {
#if !DBG
		if (!gameOpts->app.bNotebookFriendly && !gameOpts->app.bExpertMode)
			m.AddCheck ("frame cap", TXT_VSYNC, gameOpts->render.nMaxFPS == 1, KEY_V, HTX_RENDER_FRAMECAP);
#endif
		if (!gameStates.app.bNostalgia) {
			sprintf (szSlider + 1, TXT_BRIGHTNESS, paletteManager.BrightnessLevel ());
			*szSlider = *(TXT_BRIGHTNESS - 1);
			m.AddSlider ("brightness", szSlider + 1, paletteManager.GetGamma (), 0, 15, KEY_B, HTX_RENDER_BRIGHTNESS);
			}
		m.AddText ("", "");
#if !DBG
		if (gameOpts->app.bNotebookFriendly || gameOpts->app.bExpertMode)
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
		}

	if (!gameStates.app.bNostalgia) {
		if (!gameStates.app.bGameRunning) {
			sprintf (szSlider + 1, TXT_LIGHTING, pszQuality [nLighting]);
			*szSlider = *(TXT_LIGHTING - 1);
			m.AddSlider ("lighting method", szSlider + 1, nLighting, 0, (gameOpts->render.bUseShaders && ogl.m_features.bShaders) ? 3 : 1, KEY_L, HTX_LIGHTING);
			if (nLighting >= 2) {
				AddLightmapControls (m);
				if (!gameStates.app.bPrecomputeLightmaps && (nLighting == 3)) {
					sprintf (szSlider + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
					*szSlider = *(TXT_MAX_LIGHTS_PER_PASS - 1);
					m.AddSlider ("light sources", szSlider + 1, gameOpts->ogl.nMaxLightsPerPass - 5, 0, 8 - MIN_LIGHTS_PER_PASS, KEY_P, HTX_MAX_LIGHTS_PER_PASS);
					}
				}
			if (!gameStates.app.bPrecomputeLightmaps) {
				sprintf (szSlider + 1, TXT_LIGHTCOLOR, pszColorLevel [gameOpts->render.color.nLevel]);
				*szSlider = *(TXT_LIGHTCOLOR - 1);
				m.AddSlider ("colorization", szSlider + 1, gameOpts->render.color.nLevel, 0, 2, KEY_C, HTX_RENDER_LIGHTCOLOR);
				m.AddText ("", "");
				}
			}
		else if (nLighting == 1) {
			sprintf (szSlider + 1, TXT_LIGHTCOLOR, pszColorLevel [gameOpts->render.color.nLevel]);
			*szSlider = *(TXT_LIGHTCOLOR - 1);
			m.AddSlider ("colorization", szSlider + 1, gameOpts->render.color.nLevel, 0, 2, KEY_C, HTX_RENDER_LIGHTCOLOR);
			m.AddText ("", "");
			}
		}

	if (!gameStates.app.bPrecomputeLightmaps) {
		if (!(gameStates.app.bGameRunning || gameStates.app.bNostalgia)) {
			sprintf (szSlider + 1, TXT_RENDER_STYLE, pszRendStyle [gameOpts->render.bCartoonize]);
			*szSlider = *(TXT_RENDER_STYLE - 1);
			m.AddSlider ("render style", szSlider + 1, gameOpts->render.bCartoonize, 0, 1, KEY_S, HTX_RENDER_STYLE);
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

		if (EXPERTMODE && stereoDeviceMap [nStereoDevice])
			m.AddText ("", "");
		sprintf (szSlider + 1, TXT_STEREO_VIEW, pszStereoDevice [nStereoDevice]);
		*szSlider = *(TXT_STEREO_VIEW - 1);
		m.AddSlider ("3D glasses", szSlider + 1, nStereoDevice, 0, nStereoDeviceCount - 1, KEY_G, HTX_STEREO_VIEW);	//exclude shutter
		if (stereoDeviceMap [nStereoDevice] == GLASSES_VR) {
			sprintf (szSlider + 1, TXT_VR_IPD, nIPD + VR_MIN_IPD);
			*szSlider = *(TXT_VR_IPD - 1);
			m.AddSlider ("IPD", szSlider + 1, nIPD, 0, VR_MAX_IPD - VR_MIN_IPD, KEY_P, HTX_STEREO_SEPARATION);
			}

		int32_t nDevice = ogl.StereoDevice ();

		if (EXPERTMODE && stereoDeviceMap [nStereoDevice]) {
			if (stereoDeviceMap [nStereoDevice] == GLASSES_VR) {
#if 1 //DBG
				sprintf (szSlider + 1, TXT_VR_FOV, pszFOV [gameOpts->render.stereo.nVRFOV]);
				*szSlider = *(TXT_VR_FOV - 1);
				m.AddSlider ("FOV", szSlider + 1, gameOpts->render.stereo.nVRFOV, 0, sizeofa (pszFOV) - 1, KEY_F, HTX_STEREO_FOV);
#endif
				m.AddCheck ("chromAbCorr", TXT_CHROM_AB_CORR, gameOpts->render.stereo.bChromAbCorr, KEY_C, HTX_CHROM_AB_CORR);
				}
			else {
				if (stereoDeviceMap [nStereoDevice] == 1) {
					sprintf (szSlider + 1, TXT_ANAGLYPH_COLORS, pszAnaglyphColors [nAnaglyphColor]);
					*szSlider = *(TXT_ANAGLYPH_COLORS - 1);
					m.AddSlider ("anaglyph color", szSlider + 1, nAnaglyphColor, 0, sizeofa (pszAnaglyphColors) - 1, KEY_C, HTX_ANAGLYPH_COLORS);
					}

				sprintf (szSlider + 1, TXT_STEREO_SEPARATION, pszStereoSeparation [xStereoSeparation]);
				*szSlider = *(TXT_STEREO_SEPARATION - 1);
				m.AddSlider ("stereo separation", szSlider + 1, xStereoSeparation, 0, sizeofa (pszStereoSeparation) - 1, KEY_E, HTX_STEREO_SEPARATION);

				sprintf (szSlider + 1, TXT_3D_METHOD, psz3DMethod [gameOpts->render.stereo.nMethod]);
				*szSlider = *(TXT_3D_METHOD - 1);
				m.AddSlider ("3D method", szSlider + 1, gameOpts->render.stereo.nMethod, 0, sizeofa (psz3DMethod) - 1, KEY_J, HTX_3D_METHOD);

				sprintf (szSlider + 1, TXT_3D_SCREEN_DIST, nScreenDists [gameOpts->render.stereo.nScreenDist]);
				*szSlider = *(TXT_3D_SCREEN_DIST - 1);
				m.AddSlider ("screen distance", szSlider + 1, gameOpts->render.stereo.nScreenDist, 0, sizeofa (nScreenDists) - 1, KEY_S, HTX_3D_SCREEN_DIST);

				if (nDevice > 0) {
					sprintf (szSlider + 1, TXT_3D_DEGHOST, pszDeghost [gameOpts->render.stereo.bDeghost]);
					*szSlider = *(TXT_3D_DEGHOST - 1);
					m.AddSlider ("deghosting", szSlider + 1, gameOpts->render.stereo.bDeghost, 0, sizeofa (pszDeghost) - 2 + (gameOpts->render.stereo.bEnhance && (nDevice == 2)), KEY_H, HTX_3D_DEGHOST);
					if (gameOpts->render.stereo.bDeghost < 4) {
						sprintf (szSlider + 1, TXT_COLORGAIN, pszEnhance3D [gameOpts->render.stereo.bColorGain]);
						*szSlider = *(TXT_COLORGAIN - 1);
						m.AddSlider ("color gain", szSlider + 1, gameOpts->render.stereo.bColorGain, 0, sizeofa (pszEnhance3D) - 1, KEY_G, HTX_COLORGAIN);
						}
					}
				m.AddText ("", "");
				m.AddCheck ("brighten scene", TXT_BUMP_BRIGHTNESS, gameOpts->render.stereo.bBrighten, KEY_T, HTX_BUMP_BRIGHTNESS);
				if (nDevice > 0)
					m.AddCheck ("enhance 3D", TXT_ENHANCE_3D, gameOpts->render.stereo.bEnhance, KEY_D, HTX_ENHANCE_3D);
#if 0
				m.AddCheck ("flip frames", TXT_FLIPFRAMES, gameOpts->render.stereo.bFlipFrames, KEY_F, HTX_FLIPFRAMES);
#endif
				}
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
		}

	do {
		i = m.Menu (NULL, TXT_RENDER_OPTS, RenderOptionsCallback, &choice);
		} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = (nCameras != 0)))
		gameOpts->render.cameras.bHires = (nCameras == 2);
	if ((gameOpts->render.powerups.b3D = (nPowerups != 0) || gameStates.app.bStandalone))
		gameOpts->render.powerups.b3DShields = (nPowerups == 2);
	if (!gameOpts->render.powerups.b3D)
		LoadPowerupTextures ();

	if (!gameStates.app.bPrecomputeLightmaps) {
		if (m.Available ("movie subtitles"))
			gameOpts->movies.bSubTitles = (m.Value ("movie subtitles") != 0);

#if !DBG
		if (!gameOpts->app.bNotebookFriendly && !gameOpts->app.bExpertMode)
			gameOpts->render.nMaxFPS = m.Value ("frame cap") ? 1 : 120;
#endif
		if (!gameStates.app.bNostalgia)
			paletteManager.SetGamma (m.Value ("brightness"));
		if (nRendQualSave != gameOpts->render.nImageQuality)
			ogl.SetRenderQuality ();
		}

	if ((gameStates.app.bNostalgia > 1) || (nLighting == 0))
		gameOpts->render.nLightingMethod = 0;
	else {
		gameOpts->render.bUseLightmaps = (nLighting > 1);
		gameOpts->render.nLightingMethod = nLighting - gameOpts->render.bUseLightmaps;
		}

#if DBG
	if (!gameStates.app.bPrecomputeLightmaps) {
		if (EXPERTMODE) {
			gameOpts->render.debug.bWireFrame = m.Value ("draw wire frame");
			gameOpts->render.debug.bTextures = m.Value ("draw textures");
			gameOpts->render.debug.bObjects = m.Value ("draw objects");
			gameOpts->render.debug.bWalls = m.Value ("draw walls");
			gameOpts->render.debug.bDynamicLight = m.Value ("dynamic light");
			}
		}
#endif
	} while (i == -2);

gameOpts->render.stereo.nGlasses = stereoDeviceMap [nStereoDevice];
if (gameOpts->render.stereo.nGlasses == 1)
	gameOpts->render.stereo.nGlasses += nAnaglyphColor;
gameStates.render.SetCartoonStyle (gameOpts->render.bCartoonize);
#if 0
if (ogl.VRActive ())
	gameData.renderData.vr.m_magCalTO.Start (-1, true);
#endif
if (gameOpts->app.bExpertMode && gameOpts->render.color.bConfigurable) {
	gameData.SetAmbientLight (gameOpts->render.color.nAmbientLight);
	gameData.SetSpecularLight (gameOpts->render.color.nSpecularLight);
	}
lightManager.SetMethod ();
DefaultRenderSettings (false);
}

//------------------------------------------------------------------------------
//eof
