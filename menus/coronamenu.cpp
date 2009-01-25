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

int CoronaOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menu + effectOpts.nLightTrails;
v = m->m_value;
if (extraGameInfo [0].bLightTrails != v) {
	extraGameInfo [0].bLightTrails = v;
	key = -2;
	}
m = menu + effectOpts.nCoronas;
v = m->m_value;
if (gameOpts->render.coronas.bUse != v) {
	gameOpts->render.coronas.bUse = v;
	key = -2;
	}
m = menu + effectOpts.nShotCoronas;
v = m->m_value;
if (gameOpts->render.coronas.bShots != v) {
	gameOpts->render.coronas.bShots = v;
	key = -2;
	}
if (effectOpts.nCoronaStyle >= 0) {
	m = menu + effectOpts.nCoronaStyle;
	v = m->m_value;
	if (gameOpts->render.coronas.nStyle != v) {
		gameOpts->render.coronas.nStyle = v;
		sprintf (m->m_text, TXT_CORONA_QUALITY, pszCoronaQual [v]);
		m->m_bRebuild = -1;
		}
	}
if (effectOpts.nCoronaIntensity >= 0) {
	m = menu + effectOpts.nCoronaIntensity;
	v = m->m_value;
	if (gameOpts->render.coronas.nIntensity != v) {
		gameOpts->render.coronas.nIntensity = v;
		sprintf (m->m_text, TXT_CORONA_INTENSITY, pszCoronaInt [v]);
		m->m_bRebuild = -1;
		}
	}
if (effectOpts.nObjCoronaIntensity >= 0) {
	m = menu + effectOpts.nObjCoronaIntensity;
	v = m->m_value;
	if (gameOpts->render.coronas.nObjIntensity != v) {
		gameOpts->render.coronas.nObjIntensity = v;
		sprintf (m->m_text, TXT_OBJCORONA_INTENSITY, pszCoronaInt [v]);
		m->m_bRebuild = -1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CoronaOptionsMenu (void)
{
	CMenu m;
	int	i, choice = 0, optTrailType;
	char	szCoronaQual [50], szCoronaInt [50], szObjCoronaInt [50];

pszCoronaInt [0] = TXT_VERY_LOW;
pszCoronaInt [1] = TXT_LOW;
pszCoronaInt [2] = TXT_MEDIUM;
pszCoronaInt [3] = TXT_HIGH;

pszCoronaQual [0] = TXT_LOW;
pszCoronaQual [1] = TXT_MEDIUM;
pszCoronaQual [2] = TXT_HIGH;

do {
	m.Destroy ();
	m.Create (30);

	effectOpts.nCoronas = m.AddCheck (TXT_RENDER_CORONAS, gameOpts->render.coronas.bUse, KEY_C, HTX_ADVRND_CORONAS);
	effectOpts.nShotCoronas = m.AddCheck (TXT_SHOT_CORONAS, gameOpts->render.coronas.bShots, KEY_S, HTX_SHOT_CORONAS);
	effectOpts.nPowerupCoronas = m.AddCheck (TXT_POWERUP_CORONAS, gameOpts->render.coronas.bPowerups, KEY_P, HTX_POWERUP_CORONAS);
	effectOpts.nWeaponCoronas = m.AddCheck (TXT_WEAPON_CORONAS, gameOpts->render.coronas.bWeapons, KEY_W, HTX_WEAPON_CORONAS);
	m.AddText ("", 0);
	effectOpts.nAdditiveCoronas = m.AddCheck (TXT_ADDITIVE_CORONAS, gameOpts->render.coronas.bAdditive, KEY_A, HTX_ADDITIVE_CORONAS);
	effectOpts.nAdditiveObjCoronas = m.AddCheck (TXT_ADDITIVE_OBJCORONAS, gameOpts->render.coronas.bAdditiveObjs, KEY_O, HTX_ADDITIVE_OBJCORONAS);
	m.AddText ("", 0);

	sprintf (szCoronaQual + 1, TXT_CORONA_QUALITY, pszCoronaQual [gameOpts->render.coronas.nStyle]);
	*szCoronaQual = *(TXT_CORONA_QUALITY - 1);
	effectOpts.nCoronaStyle = m.AddSlider (szCoronaQual + 1, gameOpts->render.coronas.nStyle, 0, 1 + gameStates.ogl.bDepthBlending, KEY_Q, HTX_CORONA_QUALITY);
	m.AddText ("", 0);

	sprintf (szCoronaInt + 1, TXT_CORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nIntensity]);
	*szCoronaInt = *(TXT_CORONA_INTENSITY - 1);
	effectOpts.nCoronaIntensity = m.AddSlider (szCoronaInt + 1, gameOpts->render.coronas.nIntensity, 0, 3, KEY_I, HTX_CORONA_INTENSITY);
	sprintf (szObjCoronaInt + 1, TXT_OBJCORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nObjIntensity]);
	*szObjCoronaInt = *(TXT_OBJCORONA_INTENSITY - 1);
	effectOpts.nObjCoronaIntensity = m.AddSlider (szObjCoronaInt + 1, gameOpts->render.coronas.nObjIntensity, 0, 3, KEY_N, HTX_CORONA_INTENSITY);
	m.AddText ("", 0);

	effectOpts.nLightTrails = m.AddCheck (TXT_RENDER_LGTTRAILS, extraGameInfo [0].bLightTrails, KEY_T, HTX_RENDER_LGTTRAILS);
	if (extraGameInfo [0].bLightTrails) {
		optTrailType = m.AddRadio (TXT_SOLID_LIGHTTRAILS, 0, KEY_S, HTX_LIGHTTRAIL_TYPE);
		m.AddRadio (TXT_PLASMA_LIGHTTRAILS, 0, KEY_P, HTX_LIGHTTRAIL_TYPE);
		m [optTrailType + gameOpts->render.particles.bPlasmaTrails].m_value = 1;
		}
	else
		optTrailType = -1;

	for (;;) {
		i = m.Menu (NULL, TXT_CORONA_MENUTITLE, CoronaOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.coronas.bUse = m [effectOpts.nCoronas].m_value;
	gameOpts->render.coronas.bShots = m [effectOpts.nShotCoronas].m_value;
	gameOpts->render.coronas.bPowerups = m [effectOpts.nPowerupCoronas].m_value;
	gameOpts->render.coronas.bWeapons = m [effectOpts.nWeaponCoronas].m_value;
	gameOpts->render.coronas.bAdditive = m [effectOpts.nAdditiveCoronas].m_value;
	gameOpts->render.coronas.bAdditiveObjs = m [effectOpts.nAdditiveObjCoronas].m_value;
	if (optTrailType >= 0)
		gameOpts->render.particles.bPlasmaTrails = (m [optTrailType].m_value == 0);
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
