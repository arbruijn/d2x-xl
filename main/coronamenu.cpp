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
	int	nCoronas;
	int	nCoronaStyle;
	int	nShotCoronas;
	int	nWeaponCoronas;
	int	nPowerupCoronas;
	int	nAdditiveCoronas;
	int	nAdditiveObjCoronas;
	int	nCoronaIntensity;
	int	nObjCoronaIntensity;
	int	nLightTrails;
	int	nExplShrapnels;
	int	nSparks;
} effectOpts;

//------------------------------------------------------------------------------

static const char* pszCoronaInt [4];
static const char* pszCoronaQual [3];

int CoronaOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + effectOpts.nLightTrails;
v = m->value;
if (extraGameInfo [0].bLightTrails != v) {
	extraGameInfo [0].bLightTrails = v;
	*key = -2;
	}
m = menus + effectOpts.nCoronas;
v = m->value;
if (gameOpts->render.coronas.bUse != v) {
	gameOpts->render.coronas.bUse = v;
	*key = -2;
	}
m = menus + effectOpts.nShotCoronas;
v = m->value;
if (gameOpts->render.coronas.bShots != v) {
	gameOpts->render.coronas.bShots = v;
	*key = -2;
	}
if (effectOpts.nCoronaStyle >= 0) {
	m = menus + effectOpts.nCoronaStyle;
	v = m->value;
	if (gameOpts->render.coronas.nStyle != v) {
		gameOpts->render.coronas.nStyle = v;
		sprintf (m->text, TXT_CORONA_QUALITY, pszCoronaQual [v]);
		m->rebuild = -1;
		}
	}
if (effectOpts.nCoronaIntensity >= 0) {
	m = menus + effectOpts.nCoronaIntensity;
	v = m->value;
	if (gameOpts->render.coronas.nIntensity != v) {
		gameOpts->render.coronas.nIntensity = v;
		sprintf (m->text, TXT_CORONA_INTENSITY, pszCoronaInt [v]);
		m->rebuild = -1;
		}
	}
if (effectOpts.nObjCoronaIntensity >= 0) {
	m = menus + effectOpts.nObjCoronaIntensity;
	v = m->value;
	if (gameOpts->render.coronas.nObjIntensity != v) {
		gameOpts->render.coronas.nObjIntensity = v;
		sprintf (m->text, TXT_OBJCORONA_INTENSITY, pszCoronaInt [v]);
		m->rebuild = -1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CoronaOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, choice = 0, optTrailType;
	int	nOptions;
	char	szCoronaQual [50], szCoronaInt [50], szObjCoronaInt [50];

pszCoronaInt [0] = TXT_VERY_LOW;
pszCoronaInt [1] = TXT_LOW;
pszCoronaInt [2] = TXT_MEDIUM;
pszCoronaInt [3] = TXT_HIGH;

pszCoronaQual [0] = TXT_LOW;
pszCoronaQual [1] = TXT_MEDIUM;
pszCoronaQual [2] = TXT_HIGH;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	m.AddCheck (nOptions, TXT_RENDER_CORONAS, gameOpts->render.coronas.bUse, KEY_C, HTX_ADVRND_CORONAS);
	effectOpts.nCoronas = nOptions++;
	m.AddCheck (nOptions, TXT_SHOT_CORONAS, gameOpts->render.coronas.bShots, KEY_S, HTX_SHOT_CORONAS);
	effectOpts.nShotCoronas = nOptions++;
	m.AddCheck (nOptions, TXT_POWERUP_CORONAS, gameOpts->render.coronas.bPowerups, KEY_P, HTX_POWERUP_CORONAS);
	effectOpts.nPowerupCoronas = nOptions++;
	m.AddCheck (nOptions, TXT_WEAPON_CORONAS, gameOpts->render.coronas.bWeapons, KEY_W, HTX_WEAPON_CORONAS);
	effectOpts.nWeaponCoronas = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddCheck (nOptions, TXT_ADDITIVE_CORONAS, gameOpts->render.coronas.bAdditive, KEY_A, HTX_ADDITIVE_CORONAS);
	effectOpts.nAdditiveCoronas = nOptions++;
	m.AddCheck (nOptions, TXT_ADDITIVE_OBJCORONAS, gameOpts->render.coronas.bAdditiveObjs, KEY_O, HTX_ADDITIVE_OBJCORONAS);
	effectOpts.nAdditiveObjCoronas = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;

	sprintf (szCoronaQual + 1, TXT_CORONA_QUALITY, pszCoronaQual [gameOpts->render.coronas.nStyle]);
	*szCoronaQual = *(TXT_CORONA_QUALITY - 1);
	m.AddSlider (nOptions, szCoronaQual + 1, gameOpts->render.coronas.nStyle, 0, 1 + gameStates.ogl.bDepthBlending, KEY_Q, HTX_CORONA_QUALITY);
	effectOpts.nCoronaStyle = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;

	sprintf (szCoronaInt + 1, TXT_CORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nIntensity]);
	*szCoronaInt = *(TXT_CORONA_INTENSITY - 1);
	m.AddSlider (nOptions, szCoronaInt + 1, gameOpts->render.coronas.nIntensity, 0, 3, KEY_I, HTX_CORONA_INTENSITY);
	effectOpts.nCoronaIntensity = nOptions++;
	sprintf (szObjCoronaInt + 1, TXT_OBJCORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nObjIntensity]);
	*szObjCoronaInt = *(TXT_OBJCORONA_INTENSITY - 1);
	m.AddSlider (nOptions, szObjCoronaInt + 1, gameOpts->render.coronas.nObjIntensity, 0, 3, KEY_N, HTX_CORONA_INTENSITY);
	effectOpts.nObjCoronaIntensity = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;

	m.AddCheck (nOptions, TXT_RENDER_LGTTRAILS, extraGameInfo [0].bLightTrails, KEY_T, HTX_RENDER_LGTTRAILS);
	effectOpts.nLightTrails = nOptions++;
	if (extraGameInfo [0].bLightTrails) {
		m.AddRadio (nOptions, TXT_SOLID_LIGHTTRAILS, 0, KEY_S, 2, HTX_LIGHTTRAIL_TYPE);
		optTrailType = nOptions++;
		m.AddRadio (nOptions, TXT_PLASMA_LIGHTTRAILS, 0, KEY_P, 2, HTX_LIGHTTRAIL_TYPE);
		nOptions++;
		m [optTrailType + gameOpts->render.particles.bPlasmaTrails].value = 1;
		}
	else
		optTrailType = -1;

	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_CORONA_MENUTITLE, nOptions, m, CoronaOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.coronas.bUse = m [effectOpts.nCoronas].value;
	gameOpts->render.coronas.bShots = m [effectOpts.nShotCoronas].value;
	gameOpts->render.coronas.bPowerups = m [effectOpts.nPowerupCoronas].value;
	gameOpts->render.coronas.bWeapons = m [effectOpts.nWeaponCoronas].value;
	gameOpts->render.coronas.bAdditive = m [effectOpts.nAdditiveCoronas].value;
	gameOpts->render.coronas.bAdditiveObjs = m [effectOpts.nAdditiveObjCoronas].value;
	if (optTrailType >= 0)
		gameOpts->render.particles.bPlasmaTrails = (m [optTrailType].value == 0);
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
