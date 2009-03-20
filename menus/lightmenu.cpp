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

static struct {
	int	nLighting;
	int	nHWObjLighting;
	int	nHWHeadlight;
	int	nLights;
	int	nPasses;
	int	nMaxLightsPerObject;
	int	nLightmapQual;
	int	nGunColor;
	int	nObjectLight;
	int	nLightmaps;
} lightOpts;

static int nMaxLightsPerFaceTable [] = {3,4,5,6,7,8,12,16,20,24,32};

//------------------------------------------------------------------------------

static int LightTableIndex (int nValue)
{
	int i, h = (int) sizeofa (nMaxLightsPerFaceTable);

for (i = 0; i < h; i++)
	if (nValue < nMaxLightsPerFaceTable [i])
		break;
return i ? i - 1 : 0;
}

//------------------------------------------------------------------------------

#if SIMPLE_MENUS

static int nLighting, nPasses;

static const char *pszQuality [4];

int LightOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

m = menu + lightOpts.nLighting;
v = m->m_value;
if (nLighting != v) {
	nLighting = v;
	sprintf (m->m_text, TXT_LIGHTING, pszQuality [nLighting]);
	key = -2;
	return nCurItem;
	}

if (lightOpts.nLightmaps >= 0) {
	m = menu + lightOpts.nLightmaps;
	v = m->m_value;
	if (gameOpts->render.nLightmapQuality != v) {
		gameOpts->render.nLightmapQuality = v;
		sprintf (m->m_text, TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
		m->m_bRebuild = 1;
		}
	}

if (lightOpts.nLights >= 0) {
	m = menu + lightOpts.nLights;
	v = m->m_value + 1;
	if (v != gameOpts->ogl.nMaxLightsPerPass) {
		gameOpts->ogl.nMaxLightsPerPass = v;
		sprintf (m->m_text, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	}

if (lightOpts.nPasses >= 0) {
	m = menu + lightOpts.nPasses;
	v = m->m_value + 1;
	if (v != nPasses) {
		nPasses = v;
		sprintf (m->m_text, TXT_MAX_PASSES_PER_FACE, v);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LightOptionsMenu (void)
{
if (gameStates.app.bGameRunning)
	return;

	CMenu m;
	int	i, choice = 0;
	int	optFlickerLights;
#if 0
	int checks;
#endif

	char szSlider [50];

	pszQuality [0] = TXT_BASIC;
	pszQuality [1] = TXT_STANDARD;
	pszQuality [2] = TXT_HIGH;
	pszQuality [3] = TXT_BEST;

nLighting = (gameOpts->render.nLightingMethod == 0) 
				? 0 
				: (gameOpts->render.nLightingMethod == 2) 
					? 3 
					: (gameStates.render.color.bLightmapsOk && gameOpts->render.color.bUseLightmaps) + 1;

nPasses = (gameOpts->ogl.nMaxLightsPerFace + gameOpts->ogl.nMaxLightsPerPass - 1) / gameOpts->ogl.nMaxLightsPerPass;

do {
	m.Destroy ();
	m.Create (10);
	memset (&lightOpts, 0xff, sizeof (lightOpts));

	sprintf (szSlider + 1, TXT_LIGHTING, pszQuality [nLighting]);
	*szSlider = *(TXT_LIGHTING + 1);
	lightOpts.nLighting = m.AddSlider (szSlider + 1, nLighting, 0, 3, KEY_L, HTX_LIGHTING);
	lightOpts.nLights = 
	lightOpts.nPasses = -1;
	if (nLighting >= 2) {
		sprintf (szSlider + 1, TXT_LMAP_QUALITY, pszQuality [gameOpts->render.nLightmapQuality]);
		*szSlider = *(TXT_LMAP_QUALITY + 1);
		lightOpts.nLightmaps = m.AddSlider (szSlider + 1, gameOpts->render.nLightmapQuality, 0, 4, KEY_Q, HTX_LMAP_QUALITY);

		if (nLighting == 3) {
			sprintf (szSlider + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			*szSlider = *(TXT_MAX_LIGHTS_PER_PASS - 1);
			lightOpts.nLights = m.AddSlider (szSlider + 1, gameOpts->ogl.nMaxLightsPerPass - 1, 0, 7, KEY_S, HTX_MAX_LIGHTS_PER_PASS);

			sprintf (szSlider + 1, TXT_MAX_PASSES_PER_FACE, nPasses);
			*szSlider = *(TXT_MAX_PASSES_PER_FACE - 1);
			lightOpts.nPasses = m.AddSlider (szSlider + 1, nPasses - 1, 0, min (15, 32 / gameOpts->ogl.nMaxLightsPerPass), KEY_P, HTX_MAX_PASSES_PER_FACE);
			}
		}
	optFlickerLights = m.AddCheck (TXT_FLICKERLIGHTS, extraGameInfo [0].bFlickerLights, KEY_F, HTX_FLICKERLIGHTS);

	for (;;) {
		i = m.Menu (NULL, TXT_LIGHTING_MENUTITLE, LightOptionsCallback, &choice);
		if (i < 0)
			break;
		} 

	if (nLighting == 0)
		gameOpts->render.nLightingMethod = 0;
	else {
		gameOpts->render.color.bUseLightmaps = (nLighting > 1);
		gameOpts->render.nLightingMethod = nLighting - gameOpts->render.color.bUseLightmaps;
		}
	extraGameInfo [0].bFlickerLights = m [optFlickerLights].m_value;
	gameOpts->ogl.nMaxLightsPerFace = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace];

	} while (i == -2);

gameOpts->ogl.nMaxLightsPerFace = nPasses * gameOpts->ogl.nMaxLightsPerPass;

gameStates.render.nLightingMethod = gameStates.app.bNostalgia ? 0 : gameOpts->render.nLightingMethod;
if (gameStates.render.nLightingMethod == 2)
	gameStates.render.bPerPixelLighting = 2;
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
if (gameStates.render.bPerPixelLighting == 2) {
	gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
	gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
	}
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = gameStates.render.bPerPixelLighting || gameOpts->render.color.bAmbientLight;
}

#else

//------------------------------------------------------------------------------

static const char *pszLMapQual [5];

int LightOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

if (lightOpts.nLighting >= 0) {
	for (v = 0; v < 3; v++)
		if (menu [lightOpts.nLighting + v].m_value)
			break;
	if (v > 2)
		v = 0;
	if (v != gameOpts->render.nLightingMethod) {
		gameOpts->render.nLightingMethod = v;
		key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nLightmapQual >= 0) {
	m = menu + lightOpts.nLightmapQual;
	v = m->m_value;
	if (gameOpts->render.nLightmapQuality != v) {
		gameOpts->render.nLightmapQuality = v;
		sprintf (m->m_text, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
		m->m_bRebuild = 1;
		}
	}
if (lightOpts.nLightmaps >= 0) {
	m = menu + lightOpts.nLightmaps;
	v = m->m_value;
	if (v != gameOpts->render.bUseLightmaps) {
		gameOpts->render.bUseLightmaps = v;
		key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nHWObjLighting >= 0) {
	m = menu + lightOpts.nHWObjLighting;
	v = m->m_value;
	if (v != gameOpts->ogl.bObjLighting) {
		gameOpts->ogl.bObjLighting = v;
		key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nGunColor >= 0) {
	m = menu + lightOpts.nGunColor;
	v = m->m_value;
	if (v != gameOpts->render.color.bGunLight) {
		gameOpts->render.color.bGunLight = v;
		key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nObjectLight >= 0) {
	m = menu + lightOpts.nObjectLight;
	v = m->m_value;
	if (v != gameOpts->ogl.bLightObjects) {
		gameOpts->ogl.bLightObjects = v;
		key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nLights >= 0) {
	m = menu + lightOpts.nLights;
	v = m->m_value;
	if (v != gameOpts->ogl.nMaxLightsPerFace) {
		gameOpts->ogl.nMaxLightsPerFace = v;
		sprintf (m->m_text, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [v]);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerObject >= 0) {
	m = menu + lightOpts.nMaxLightsPerObject;
	v = m->m_value;
	if (v != gameOpts->ogl.nMaxLightsPerObject) {
		gameOpts->ogl.nMaxLightsPerObject = v;
		sprintf (m->m_text, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [v]);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nPasses >= 0) {
	m = menu + lightOpts.nPasses;
	v = m->m_value + 1;
	if (v != gameOpts->ogl.nMaxLightsPerPass) {
		gameOpts->ogl.nMaxLightsPerPass = v;
		sprintf (m->m_text, TXT_MAX_LIGHTS_PER_PASS, v);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LightOptionsMenu (void)
{
	CMenu m;
	int	i, choice = 0;
	int	optColoredLight, optMixColors, optPowerupLights, optFlickerLights, optColorSat, optBrightObjects, nPowerupLight = -1;
#if 0
	int checks;
#endif

	char szMaxLightsPerFace [50];
	char szMaxLightsPerPass [50];
	char szMaxLightsPerObject [50];
	char szLightmapQual [50];

	pszLMapQual [0] = TXT_LOW;
	pszLMapQual [1] = TXT_MEDIUM;
	pszLMapQual [2] = TXT_HIGH;
	pszLMapQual [3] = TXT_VERY_HIGH;
	pszLMapQual [4] = TXT_EXTREME;

do {
	m.Destroy ();
	m.Create (30);
	optColorSat = 
	optColoredLight =
	optMixColors = 
	optPowerupLights = -1;
	memset (&lightOpts, 0xff, sizeof (lightOpts));
	if (!gameStates.app.bGameRunning) {
		if ((gameOpts->render.nLightingMethod == 2) && 
			 !(gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk))
			gameOpts->render.nLightingMethod = 1;
		lightOpts.nLighting = m.AddRadio (TXT_STD_LIGHTING, gameOpts->render.nLightingMethod == 0, KEY_S, NULL);
		m.AddRadio (TXT_VERTEX_LIGHTING, gameOpts->render.nLightingMethod == 1, KEY_V, HTX_VERTEX_LIGHTING);
		if (gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk)
			m.AddRadio (TXT_PER_PIXEL_LIGHTING, gameOpts->render.nLightingMethod == 2, KEY_P, HTX_PER_PIXEL_LIGHTING);
		m.AddText ("", 0);
		}
	gameOpts->ogl.nMaxLightsPerObject = LightTableIndex (gameOpts->ogl.nMaxLightsPerObject);
	gameOpts->ogl.nMaxLightsPerFace = LightTableIndex (gameOpts->ogl.nMaxLightsPerFace);
	if (gameOpts->render.nLightingMethod) {
		if (gameOpts->render.nLightingMethod == 1) {
			if (gameStates.ogl.bHeadlight)
				lightOpts.nHWHeadlight = m.AddCheck (TXT_HW_HEADLIGHT, gameOpts->ogl.bHeadlight, KEY_H, HTX_HW_HEADLIGHT);
			lightOpts.nHWObjLighting = m.AddCheck (TXT_OBJECT_HWLIGHTING, gameOpts->ogl.bObjLighting, KEY_A, HTX_OBJECT_HWLIGHTING);
			if (!gameOpts->ogl.bObjLighting) {
				lightOpts.nObjectLight = m.AddCheck (TXT_OBJECT_LIGHTING, gameOpts->ogl.bLightObjects, KEY_B, HTX_OBJECT_LIGHTING);
				if (gameOpts->ogl.bLightObjects) {
					nPowerupLight = m.AddCheck (TXT_POWERUP_LIGHTING, gameOpts->ogl.bLightPowerups, KEY_W, HTX_POWERUP_LIGHTING);
					}
				else
					nPowerupLight = -1;
				}
			}
		sprintf (szMaxLightsPerObject + 1, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject]);
		*szMaxLightsPerObject = *(TXT_MAX_LIGHTS_PER_OBJECT - 1);
		lightOpts.nMaxLightsPerObject = m.AddSlider (szMaxLightsPerObject + 1, gameOpts->ogl.nMaxLightsPerObject, 0, (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_O, HTX_MAX_LIGHTS_PER_OBJECT);

		if (gameOpts->render.nLightingMethod == 2) {
			sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace]);
			*szMaxLightsPerFace = *(TXT_MAX_LIGHTS_PER_FACE - 1);
			lightOpts.nLights = m.AddSlider (szMaxLightsPerFace + 1, gameOpts->ogl.nMaxLightsPerFace, 0,  (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_A, HTX_MAX_LIGHTS_PER_FACE);
			sprintf (szMaxLightsPerPass + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			*szMaxLightsPerPass = *(TXT_MAX_LIGHTS_PER_PASS - 1);
			lightOpts.nPasses = m.AddSlider (szMaxLightsPerPass + 1, gameOpts->ogl.nMaxLightsPerPass - 1, 0, 7, KEY_S, HTX_MAX_LIGHTS_PER_PASS);
			}
		if (!gameStates.app.bGameRunning && 
			 ((gameOpts->render.nLightingMethod == 2) || ((gameOpts->render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps))) {
			sprintf (szLightmapQual + 1, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
			*szLightmapQual = *(TXT_LMAP_QUALITY + 1);
			lightOpts.nLightmapQual = m.AddSlider (szLightmapQual + 1, gameOpts->render.nLightmapQuality, 0, 4, KEY_Q, HTX_LMAP_QUALITY);
			}

		m.AddText ("", 0);
		optColorSat = m.AddRadio (TXT_FULL_COLORSAT, 0, KEY_F, HTX_COLOR_SATURATION);
		m.AddRadio (TXT_LIMIT_COLORSAT, 0, KEY_L, HTX_COLOR_SATURATION);
		m.AddRadio (TXT_NO_COLORSAT, 0, KEY_N, HTX_COLOR_SATURATION);
		m [optColorSat + NMCLAMP (gameOpts->render.color.nSaturation, 0, 2)].m_value = 1;
		m.AddText ("", 0);
		}
	if (gameOpts->render.nLightingMethod != 1)
		lightOpts.nLightmaps = -1;
	else
		lightOpts.nLightmaps = m.AddCheck (TXT_USE_LIGHTMAPS, gameOpts->render.bUseLightmaps, KEY_G, HTX_USE_LIGHTMAPS);
	if (gameOpts->render.nLightingMethod < 2)
		optColoredLight = m.AddCheck (TXT_USE_COLOR, gameOpts->render.color.bAmbientLight, KEY_C, HTX_RENDER_AMBICOLOR);
	if (!gameOpts->render.nLightingMethod) {
		lightOpts.nGunColor = m.AddCheck (TXT_USE_WPNCOLOR, gameOpts->render.color.bGunLight, KEY_W, HTX_RENDER_WPNCOLOR);
		if (gameOpts->app.bExpertMode) {
			if (!gameOpts->render.nLightingMethod && gameOpts->render.color.bGunLight)
				optMixColors = m.AddCheck (TXT_MIX_COLOR, gameOpts->render.color.bMix, KEY_X, HTX_ADVRND_MIXCOLOR);
			}
		}
	optPowerupLights = m.AddCheck (TXT_POWERUPLIGHTS, !extraGameInfo [0].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
	optFlickerLights = m.AddCheck (TXT_FLICKERLIGHTS, extraGameInfo [0].bFlickerLights, KEY_F, HTX_FLICKERLIGHTS);
	if (gameOpts->render.bHiresModels [0])
		optBrightObjects = m.AddCheck (TXT_BRIGHT_OBJECTS, extraGameInfo [0].bBrightObjects, KEY_B, HTX_BRIGHT_OBJECTS);
	else
		optBrightObjects = -1;
	for (;;) {
		i = m.Menu (NULL, TXT_LIGHTING_MENUTITLE, LightOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.nLightingMethod == 1) {
		if (lightOpts.nObjectLight >= 0) {
			gameOpts->ogl.bLightObjects = m [lightOpts.nObjectLight].m_value;
			if (nPowerupLight >= 0)
				gameOpts->ogl.bLightPowerups = m [nPowerupLight].m_value;
			}
		}
	if (gameOpts->render.nLightingMethod < 2) {
		if (optColoredLight >= 0)
			gameOpts->render.color.bAmbientLight = m [optColoredLight].m_value;
		if (lightOpts.nGunColor >= 0)
			gameOpts->render.color.bGunLight = m [lightOpts.nGunColor].m_value;
		if (gameOpts->app.bExpertMode) {
			if (gameStates.render.color.bLightmapsOk && gameOpts->render.color.bUseLightmaps)
			gameStates.ogl.nContrast = 8;
			if (gameOpts->render.color.bGunLight)
				GET_VAL (gameOpts->render.color.bMix, optMixColors);
#if EXPMODE_DEFAULTS
				else
					gameOpts->render.color.bMix = 1;
#endif
			}
		}
	if (optPowerupLights >= 0)
		extraGameInfo [0].bPowerupLights = !m [optPowerupLights].m_value;
	extraGameInfo [0].bFlickerLights = m [optFlickerLights].m_value;
	GET_VAL (gameOpts->ogl.bHeadlight, lightOpts.nHWHeadlight);
	GET_VAL (extraGameInfo [0].bBrightObjects, optBrightObjects);
	gameOpts->ogl.nMaxLightsPerFace = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace];
	gameOpts->ogl.nMaxLightsPerObject = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject];
	} while (i == -2);
if (optColorSat >= 0) {
	for (i = 0; i < 3; i++)
		if (m [optColorSat + i].m_value) {
			gameOpts->render.color.nSaturation = i;
			break;
			}
	}
gameStates.render.nLightingMethod = gameStates.app.bNostalgia ? 0 : gameOpts->render.nLightingMethod;
if (gameStates.render.nLightingMethod == 2)
	gameStates.render.bPerPixelLighting = 2;
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
if (gameStates.render.bPerPixelLighting == 2) {
	gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
	gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
	}
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = gameStates.render.bPerPixelLighting || gameOpts->render.color.bAmbientLight;
}

#endif //SIMPLE_MENUS

//------------------------------------------------------------------------------
//eof
