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
	int	nMethod;
	int	nHWObjLighting;
	int	nHWHeadlight;
	int	nMaxLightsPerFace;
	int	nMaxLightsPerPass;
	int	nMaxLightsPerObject;
	int	nLightmapQual;
	int	nGunColor;
	int	nObjectLight;
	int	nLightmaps;
} lightOpts;

static int nMaxLightsPerFaceTable [] = {3,4,5,6,7,8,12,16,20,24,32};

static const char *pszLMapQual [5];

//------------------------------------------------------------------------------

int LightOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (lightOpts.nMethod >= 0) {
	for (v = 0; v < 3; v++)
		if (menus [lightOpts.nMethod + v].value)
			break;
	if (v > 2)
		v = 0;
	if (v != gameOpts->render.nLightingMethod) {
		gameOpts->render.nLightingMethod = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nLightmapQual >= 0) {
	m = menus + lightOpts.nLightmapQual;
	v = m->value;
	if (gameOpts->render.nLightmapQuality != v) {
		gameOpts->render.nLightmapQuality = v;
		sprintf (m->text, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
		m->rebuild = 1;
		}
	}
if (lightOpts.nLightmaps >= 0) {
	m = menus + lightOpts.nLightmaps;
	v = m->value;
	if (v != gameOpts->render.bUseLightmaps) {
		gameOpts->render.bUseLightmaps = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nHWObjLighting >= 0) {
	m = menus + lightOpts.nHWObjLighting;
	v = m->value;
	if (v != gameOpts->ogl.bObjLighting) {
		gameOpts->ogl.bObjLighting = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nGunColor >= 0) {
	m = menus + lightOpts.nGunColor;
	v = m->value;
	if (v != gameOpts->render.color.bGunLight) {
		gameOpts->render.color.bGunLight = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nObjectLight >= 0) {
	m = menus + lightOpts.nObjectLight;
	v = m->value;
	if (v != gameOpts->ogl.bLightObjects) {
		gameOpts->ogl.bLightObjects = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerFace >= 0) {
	m = menus + lightOpts.nMaxLightsPerFace;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerFace) {
		gameOpts->ogl.nMaxLightsPerFace = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerObject >= 0) {
	m = menus + lightOpts.nMaxLightsPerObject;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerObject) {
		gameOpts->ogl.nMaxLightsPerObject = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerPass >= 0) {
	m = menus + lightOpts.nMaxLightsPerPass;
	v = m->value + 1;
	if (v != gameOpts->ogl.nMaxLightsPerPass) {
		gameOpts->ogl.nMaxLightsPerPass = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_PASS, v);
		m->rebuild = 1;
		return nCurItem;
		}
	}
return nCurItem;
}

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

void LightOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, choice = 0;
	int	nOptions;
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
	memset (m, 0, sizeof (m));
	nOptions = 0;
	optColorSat = 
	optColoredLight =
	optMixColors = 
	optPowerupLights = -1;
	memset (&lightOpts, 0xff, sizeof (lightOpts));
	if (!gameStates.app.bGameRunning) {
		if ((gameOpts->render.nLightingMethod == 2) && 
			 !(gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk))
			gameOpts->render.nLightingMethod = 1;
		m.AddRadio (nOptions, TXT_STD_LIGHTING, gameOpts->render.nLightingMethod == 0, KEY_S, 1, NULL);
		lightOpts.nMethod = nOptions++;
		m.AddRadio (nOptions, TXT_VERTEX_LIGHTING, gameOpts->render.nLightingMethod == 1, KEY_V, 1, HTX_VERTEX_LIGHTING);
		nOptions++;
		if (gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk) {
			m.AddRadio (nOptions, TXT_PER_PIXEL_LIGHTING, gameOpts->render.nLightingMethod == 2, KEY_P, 1, HTX_PER_PIXEL_LIGHTING);
			nOptions++;
			}	
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	gameOpts->ogl.nMaxLightsPerObject = LightTableIndex (gameOpts->ogl.nMaxLightsPerObject);
	gameOpts->ogl.nMaxLightsPerFace = LightTableIndex (gameOpts->ogl.nMaxLightsPerFace);
	if (gameOpts->render.nLightingMethod) {
#if 0
		m.AddText (nOptions, "", 0);
		nOptions++;
#endif
		if (gameOpts->render.nLightingMethod == 1) {
			if (gameStates.ogl.bHeadlight) {
				m.AddCheck (nOptions, TXT_HW_HEADLIGHT, gameOpts->ogl.bHeadlight, KEY_H, HTX_HW_HEADLIGHT);
				lightOpts.nHWHeadlight = nOptions++;
				}
			m.AddCheck (nOptions, TXT_OBJECT_HWLIGHTING, gameOpts->ogl.bObjLighting, KEY_A, HTX_OBJECT_HWLIGHTING);
			lightOpts.nHWObjLighting = nOptions++;
			if (!gameOpts->ogl.bObjLighting) {
				m.AddCheck (nOptions, TXT_OBJECT_LIGHTING, gameOpts->ogl.bLightObjects, KEY_B, HTX_OBJECT_LIGHTING);
				lightOpts.nObjectLight = nOptions++;
				if (gameOpts->ogl.bLightObjects) {
					m.AddCheck (nOptions, TXT_POWERUP_LIGHTING, gameOpts->ogl.bLightPowerups, KEY_W, HTX_POWERUP_LIGHTING);
					nPowerupLight = nOptions++;
					}
				else
					nPowerupLight = -1;
				}
			}
		sprintf (szMaxLightsPerObject + 1, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject]);
		*szMaxLightsPerObject = *(TXT_MAX_LIGHTS_PER_OBJECT - 1);
		m.AddSlider (nOptions, szMaxLightsPerObject + 1, gameOpts->ogl.nMaxLightsPerObject, 0, (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_O, HTX_MAX_LIGHTS_PER_OBJECT);
		lightOpts.nMaxLightsPerObject = nOptions++;

		if (gameOpts->render.nLightingMethod == 2) {
			sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace]);
			*szMaxLightsPerFace = *(TXT_MAX_LIGHTS_PER_FACE - 1);
			m.AddSlider (nOptions, szMaxLightsPerFace + 1, gameOpts->ogl.nMaxLightsPerFace, 0,  (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_A, HTX_MAX_LIGHTS_PER_FACE);
			lightOpts.nMaxLightsPerFace = nOptions++;
			sprintf (szMaxLightsPerPass + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			*szMaxLightsPerPass = *(TXT_MAX_LIGHTS_PER_PASS - 1);
			m.AddSlider (nOptions, szMaxLightsPerPass + 1, gameOpts->ogl.nMaxLightsPerPass - 1, 0, 7, KEY_S, HTX_MAX_LIGHTS_PER_PASS);
			lightOpts.nMaxLightsPerPass = nOptions++;
			}
		if (!gameStates.app.bGameRunning && 
			 ((gameOpts->render.nLightingMethod == 2) || ((gameOpts->render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps))) {
			sprintf (szLightmapQual + 1, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
			*szLightmapQual = *(TXT_LMAP_QUALITY + 1);
			m.AddSlider (nOptions, szLightmapQual + 1, gameOpts->render.nLightmapQuality, 0, 4, KEY_Q, HTX_LMAP_QUALITY);
			lightOpts.nLightmapQual = nOptions++;
			}

		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_FULL_COLORSAT, 0, KEY_F, 2, HTX_COLOR_SATURATION);
		optColorSat = nOptions++;
		m.AddRadio (nOptions, TXT_LIMIT_COLORSAT, 0, KEY_L, 2, HTX_COLOR_SATURATION);
		nOptions++;
		m.AddRadio (nOptions, TXT_NO_COLORSAT, 0, KEY_N, 2, HTX_COLOR_SATURATION);
		nOptions++;
		m [optColorSat + NMCLAMP (gameOpts->render.color.nSaturation, 0, 2)].value = 1;
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	if (gameOpts->render.nLightingMethod != 1)
		lightOpts.nLightmaps = -1;
	else {
		m.AddCheck (nOptions, TXT_USE_LIGHTMAPS, gameOpts->render.bUseLightmaps, KEY_G, HTX_USE_LIGHTMAPS);
		lightOpts.nLightmaps = nOptions++;
		}
	if (gameOpts->render.nLightingMethod < 2) {
		m.AddCheck (nOptions, TXT_USE_COLOR, gameOpts->render.color.bAmbientLight, KEY_C, HTX_RENDER_AMBICOLOR);
		optColoredLight = nOptions++;
		}
	if (!gameOpts->render.nLightingMethod) {
		m.AddCheck (nOptions, TXT_USE_WPNCOLOR, gameOpts->render.color.bGunLight, KEY_W, HTX_RENDER_WPNCOLOR);
		lightOpts.nGunColor = nOptions++;
		if (gameOpts->app.bExpertMode) {
			if (!gameOpts->render.nLightingMethod && gameOpts->render.color.bGunLight) {
				m.AddCheck (nOptions, TXT_MIX_COLOR, gameOpts->render.color.bMix, KEY_X, HTX_ADVRND_MIXCOLOR);
				optMixColors = nOptions++;
				}
			}
		}
	m.AddCheck (nOptions, TXT_POWERUPLIGHTS, !extraGameInfo [0].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
	optPowerupLights = nOptions++;
	m.AddCheck (nOptions, TXT_FLICKERLIGHTS, extraGameInfo [0].bFlickerLights, KEY_F, HTX_FLICKERLIGHTS);
	optFlickerLights = nOptions++;
	if (gameOpts->render.bHiresModels) {
		m.AddCheck (nOptions, TXT_BRIGHT_OBJECTS, extraGameInfo [0].bBrightObjects, KEY_B, HTX_BRIGHT_OBJECTS);
		optBrightObjects = nOptions++;
		}
	else
		optBrightObjects = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTING_MENUTITLE, nOptions, m, &LightOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.nLightingMethod == 1) {
		if (lightOpts.nObjectLight >= 0) {
			gameOpts->ogl.bLightObjects = m [lightOpts.nObjectLight].value;
			if (nPowerupLight >= 0)
				gameOpts->ogl.bLightPowerups = m [nPowerupLight].value;
			}
		}
	if (gameOpts->render.nLightingMethod < 2) {
		if (optColoredLight >= 0)
			gameOpts->render.color.bAmbientLight = m [optColoredLight].value;
		if (lightOpts.nGunColor >= 0)
			gameOpts->render.color.bGunLight = m [lightOpts.nGunColor].value;
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
		extraGameInfo [0].bPowerupLights = !m [optPowerupLights].value;
	extraGameInfo [0].bFlickerLights = m [optFlickerLights].value;
	GET_VAL (gameOpts->ogl.bHeadlight, lightOpts.nHWHeadlight);
	GET_VAL (extraGameInfo [0].bBrightObjects, optBrightObjects);
	gameOpts->ogl.nMaxLightsPerFace = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace];
	gameOpts->ogl.nMaxLightsPerObject = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject];
	} while (i == -2);
if (optColorSat >= 0) {
	for (i = 0; i < 3; i++)
		if (m [optColorSat + i].value) {
			gameOpts->render.color.nSaturation = i;
			break;
			}
	}
gameStates.render.nLightingMethod = gameOpts->render.nLightingMethod;
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

//------------------------------------------------------------------------------
//eof
