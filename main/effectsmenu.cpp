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
	int	nUse;
	int	nQuality;
	int	nStyle;
	int	nOmega;
} lightningOpts;

static struct {
	int	nUse;
	int	nPlayer;
	int	nRobots;
	int	nMissiles;
	int	nDebris;
	int	nBubbles;
	int	nDensity [5];
	int	nLife [5];
	int	nSize [5];
	int	nAlpha [5];
	int	nSyncSizes;
	int	nQuality;
} smokeOpts;

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

static const char* pszExplShrapnels [5];

int EffectOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + effectOpts.nExplShrapnels;
v = m->value;
if (gameOpts->render.effects.nShrapnels != v) {
	gameOpts->render.effects.nShrapnels = v;
	sprintf (m->text, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [v]);
	m->rebuild = -1;
	}
m = menus + effectOpts.nSparks;
v = m->value;
if (gameOpts->render.effects.bEnergySparks != v) {
	gameOpts->render.effects.bEnergySparks = v;
	*key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void EffectOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, j, choice = 0;
	int	nOptions;
	int	optTranspExpl, optThrusterFlame, optDmgExpl, optAutoTransp, optPlayerShields, optSoftParticles [3],
			optRobotShields, optShieldHits, optTracers, optGatlingTrails, optExplBlast, optMovingSparks;
#if 0
	int	optShockwaves;
#endif
	int	bEnergySparks = gameOpts->render.effects.bEnergySparks;
	char	szExplShrapnels [50];

pszExplShrapnels [0] = TXT_NONE;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	sprintf (szExplShrapnels + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
	*szExplShrapnels = *(TXT_EXPLOSION_SHRAPNELS - 1);
	m.AddSlider (nOptions, szExplShrapnels + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_P, HTX_EXPLOSION_SHRAPNELS);
	effectOpts.nExplShrapnels = nOptions++;
	m.AddCheck (nOptions, TXT_EXPLOSION_BLAST, gameOpts->render.effects.bExplBlasts, KEY_B, HTX_EXPLOSION_BLAST);
	optExplBlast = nOptions++;
	m.AddCheck (nOptions, TXT_DMG_EXPL, extraGameInfo [0].bDamageExplosions, KEY_X, HTX_RENDER_DMGEXPL);
	optDmgExpl = nOptions++;
	m.AddRadio (nOptions, TXT_NO_THRUSTER_FLAME, 0, KEY_F, 1, HTX_RENDER_THRUSTER);
	optThrusterFlame = nOptions++;
	m.AddRadio (nOptions, TXT_2D_THRUSTER_FLAME, 0, KEY_2, 1, HTX_RENDER_THRUSTER);
	nOptions++;
	m.AddRadio (nOptions, TXT_3D_THRUSTER_FLAME, 0, KEY_3, 1, HTX_RENDER_THRUSTER);
	nOptions++;
	m [optThrusterFlame + extraGameInfo [0].bThrusterFlames].value = 1;
	m.AddCheck (nOptions, TXT_RENDER_SPARKS, gameOpts->render.effects.bEnergySparks, KEY_P, HTX_RENDER_SPARKS);
	effectOpts.nSparks = nOptions++;
	if (gameOpts->render.effects.bEnergySparks) {
		m.AddCheck (nOptions, TXT_MOVING_SPARKS, gameOpts->render.effects.bMovingSparks, KEY_V, HTX_MOVING_SPARKS);
		optMovingSparks = nOptions++;
		}
	else
		optMovingSparks = -1;
	if (gameOpts->render.textures.bUseHires)
		optTranspExpl = -1;
	else {
		m.AddCheck (nOptions, TXT_TRANSP_EFFECTS, gameOpts->render.effects.bTransparent, KEY_E, HTX_ADVRND_TRANSPFX);
		optTranspExpl = nOptions++;
		}
	m.AddCheck (nOptions, TXT_SOFT_SPRITES, (gameOpts->render.effects.bSoftParticles & 1) != 0, KEY_I, HTX_SOFT_SPRITES);
	optSoftParticles [0] = nOptions++;
	if (gameOpts->render.effects.bEnergySparks) {
		m.AddCheck (nOptions, TXT_SOFT_SPARKS, (gameOpts->render.effects.bSoftParticles & 2) != 0, KEY_A, HTX_SOFT_SPARKS);
		optSoftParticles [1] = nOptions++;
		}
	else
		optSoftParticles [1] = -1; 
	if (extraGameInfo [0].bUseParticles) {
		m.AddCheck (nOptions, TXT_SOFT_SMOKE, (gameOpts->render.effects.bSoftParticles & 4) != 0, KEY_O, HTX_SOFT_SMOKE);
		optSoftParticles [2] = nOptions++;
		}
	else
		optSoftParticles [2] = -1;
	m.AddCheck (nOptions, TXT_AUTO_TRANSPARENCY, gameOpts->render.effects.bAutoTransparency, KEY_A, HTX_RENDER_AUTOTRANSP);
	optAutoTransp = nOptions++;
	m.AddCheck (nOptions, TXT_RENDER_SHIELDS, extraGameInfo [0].bPlayerShield, KEY_P, HTX_RENDER_SHIELDS);
	optPlayerShields = nOptions++;
	m.AddCheck (nOptions, TXT_ROBOT_SHIELDS, gameOpts->render.effects.bRobotShields, KEY_O, HTX_ROBOT_SHIELDS);
	optRobotShields = nOptions++;
	m.AddCheck (nOptions, TXT_SHIELD_HITS, gameOpts->render.effects.bOnlyShieldHits, KEY_H, HTX_SHIELD_HITS);
	optShieldHits = nOptions++;
	m.AddCheck (nOptions, TXT_RENDER_TRACERS, extraGameInfo [0].bTracers, KEY_T, HTX_RENDER_TRACERS);
	optTracers = nOptions++;
	m.AddCheck (nOptions, TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
	optGatlingTrails = nOptions++;
#if 0
	m.AddCheck (nOptions, TXT_RENDER_SHKWAVES, extraGameInfo [0].bShockwaves, KEY_S, HTX_RENDER_SHKWAVES);
	optShockwaves = nOptions++;
#endif
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_EFFECT_MENUTITLE, nOptions, m, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.effects.bEnergySparks = m [effectOpts.nSparks].value;
	if ((gameOpts->render.effects.bEnergySparks != bEnergySparks) && gameStates.app.bGameRunning) {
		if (gameOpts->render.effects.bEnergySparks)
			sparkManager.Setup ();
		else
			sparkManager.Destroy ();
		}
	GET_VAL (gameOpts->render.effects.bTransparent, optTranspExpl);
	for (j = 0; j < 3; j++)
	if (optSoftParticles [j] >= 0) {
		if (m [optSoftParticles [j]].value)
			gameOpts->render.effects.bSoftParticles |= 1 << j;
		else
			gameOpts->render.effects.bSoftParticles &= ~(1 << j);
		}
	GET_VAL (gameOpts->render.effects.bMovingSparks, optMovingSparks);
	gameOpts->render.effects.bAutoTransparency = m [optAutoTransp].value;
	gameOpts->render.effects.bExplBlasts = m [optExplBlast].value;
	extraGameInfo [0].bTracers = m [optTracers].value;
	extraGameInfo [0].bGatlingTrails = m [optGatlingTrails].value;
	extraGameInfo [0].bShockwaves = 0; //m [optShockwaves].value;
	extraGameInfo [0].bDamageExplosions = m [optDmgExpl].value;
	for (j = 0; j < 3; j++)
		if (m [optThrusterFlame + j].value) {
			extraGameInfo [0].bThrusterFlames = j;
			break;
			}
	extraGameInfo [0].bPlayerShield = m [optPlayerShields].value;
	gameOpts->render.effects.bRobotShields = m [optRobotShields].value;
	gameOpts->render.effects.bOnlyShieldHits = m [optShieldHits].value;
#if EXPMODE_DEFAULTS
	if (!gameOpts->app.bExpertMode) {
		gameOpts->render.effects.bTransparent = 1;
	gameOpts->render.effects.bAutoTransparency = 1;
	gameOpts->render.coronas.bUse = 0;
	gameOpts->render.coronas.bShots = 0;
	extraGameInfo [0].bLightTrails = 1;
	extraGameInfo [0].bTracers = 1;
	extraGameInfo [0].bShockwaves = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bPlayerShield = 1;
		}
#endif
	} while (i == -2);
SetDebrisCollisions ();
}

//------------------------------------------------------------------------------

static const char* pszSmokeAmount [5];
static const char* pszSmokeSize [4];
static const char* pszSmokeLife [3];
static const char* pszSmokeQual [3];
static const char* pszSmokeAlpha [5];

int SmokeOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			i, v;

m = menus + smokeOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseParticles) {
	extraGameInfo [0].bUseParticles = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseParticles) {
	m = menus + smokeOpts.nQuality;
	v = m->value;
	if (v != gameOpts->render.particles.bSort) {
		gameOpts->render.particles.bSort = v;
		sprintf (m->text, TXT_SMOKE_QUALITY, pszSmokeQual [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	m = menus + smokeOpts.nSyncSizes;
	v = m->value;
	if (v != gameOpts->render.particles.bSyncSizes) {
		gameOpts->render.particles.bSyncSizes = v;
		*key = -2;
		return nCurItem;
		}
	m = menus + smokeOpts.nPlayer;
	v = m->value;
	if (gameOpts->render.particles.bPlayers != v) {
		gameOpts->render.particles.bPlayers = v;
		*key = -2;
		}
	m = menus + smokeOpts.nRobots;
	v = m->value;
	if (gameOpts->render.particles.bRobots != v) {
		gameOpts->render.particles.bRobots = v;
		*key = -2;
		}
	m = menus + smokeOpts.nMissiles;
	v = m->value;
	if (gameOpts->render.particles.bMissiles != v) {
		gameOpts->render.particles.bMissiles = v;
		*key = -2;
		}
	m = menus + smokeOpts.nDebris;
	v = m->value;
	if (gameOpts->render.particles.bDebris != v) {
		gameOpts->render.particles.bDebris = v;
		*key = -2;
		}
	m = menus + smokeOpts.nBubbles;
	v = m->value;
	if (gameOpts->render.particles.bBubbles != v) {
		gameOpts->render.particles.bBubbles = v;
		*key = -2;
		}
	if (gameOpts->render.particles.bSyncSizes) {
		m = menus + smokeOpts.nDensity [0];
		v = m->value;
		if (gameOpts->render.particles.nDens [0] != v) {
			gameOpts->render.particles.nDens [0] = v;
			sprintf (m->text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
			m->rebuild = 1;
			}
		m = menus + smokeOpts.nSize [0];
		v = m->value;
		if (gameOpts->render.particles.nSize [0] != v) {
			gameOpts->render.particles.nSize [0] = v;
			sprintf (m->text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
			m->rebuild = 1;
			}
		m = menus + smokeOpts.nAlpha [0];
		v = m->value;
		if (v != gameOpts->render.particles.nAlpha [0]) {
			gameOpts->render.particles.nAlpha [0] = v;
			sprintf (m->text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
			m->rebuild = 1;
			}
		}
	else {
		for (i = 1; i < 5; i++) {
			if (smokeOpts.nDensity [i] >= 0) {
				m = menus + smokeOpts.nDensity [i];
				v = m->value;
				if (gameOpts->render.particles.nDens [i] != v) {
					gameOpts->render.particles.nDens [i] = v;
					sprintf (m->text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nSize [i] >= 0) {
				m = menus + smokeOpts.nSize [i];
				v = m->value;
				if (gameOpts->render.particles.nSize [i] != v) {
					gameOpts->render.particles.nSize [i] = v;
					sprintf (m->text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nLife [i] >= 0) {
				m = menus + smokeOpts.nLife [i];
				v = m->value;
				if (gameOpts->render.particles.nLife [i] != v) {
					gameOpts->render.particles.nLife [i] = v;
					sprintf (m->text, TXT_SMOKE_LIFE, pszSmokeLife [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nAlpha [i] >= 0) {
				m = menus + smokeOpts.nAlpha [i];
				v = m->value;
				if (v != gameOpts->render.particles.nAlpha [i]) {
					gameOpts->render.particles.nAlpha [i] = v;
					sprintf (m->text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
					m->rebuild = 1;
					}
				}
			}
		}
	}
else
	particleManager.Shutdown ();
return nCurItem;
}

//------------------------------------------------------------------------------

static char szSmokeDens [5][50];
static char szSmokeSize [5][50];
static char szSmokeLife [5][50];
static char szSmokeAlpha [5][50];

int AddSmokeSliders (CMenuItem *m, int nOptions, int i)
{
sprintf (szSmokeDens [i] + 1, TXT_SMOKE_DENS, pszSmokeAmount [NMCLAMP (gameOpts->render.particles.nDens [i], 0, 4)]);
*szSmokeDens [i] = *(TXT_SMOKE_DENS - 1);
m.AddSlider (nOptions, szSmokeDens [i] + 1, gameOpts->render.particles.nDens [i], 0, 4, KEY_P, HTX_ADVRND_SMOKEDENS);
smokeOpts.nDensity [i] = nOptions++;
sprintf (szSmokeSize [i] + 1, TXT_SMOKE_SIZE, pszSmokeSize [NMCLAMP (gameOpts->render.particles.nSize [i], 0, 3)]);
*szSmokeSize [i] = *(TXT_SMOKE_SIZE - 1);
m.AddSlider (nOptions, szSmokeSize [i] + 1, gameOpts->render.particles.nSize [i], 0, 3, KEY_Z, HTX_ADVRND_PARTSIZE);
smokeOpts.nSize [i] = nOptions++;
if (i < 3)
	smokeOpts.nLife [i] = -1;
else {
	sprintf (szSmokeLife [i] + 1, TXT_SMOKE_LIFE, pszSmokeLife [NMCLAMP (gameOpts->render.particles.nLife [i], 0, 3)]);
	*szSmokeLife [i] = *(TXT_SMOKE_LIFE - 1);
	m.AddSlider (nOptions, szSmokeLife [i] + 1, gameOpts->render.particles.nLife [i], 0, 2, KEY_L, HTX_SMOKE_LIFE);
	smokeOpts.nLife [i] = nOptions++;
	}
sprintf (szSmokeAlpha [i] + 1, TXT_SMOKE_ALPHA, pszSmokeAlpha [NMCLAMP (gameOpts->render.particles.nAlpha [i], 0, 4)]);
*szSmokeAlpha [i] = *(TXT_SMOKE_SIZE - 1);
m.AddSlider (nOptions, szSmokeAlpha [i] + 1, gameOpts->render.particles.nAlpha [i], 0, 4, KEY_Z, HTX_ADVRND_SMOKEALPHA);
smokeOpts.nAlpha [i] = nOptions++;
return nOptions;
}

//------------------------------------------------------------------------------

void SmokeOptionsMenu ()
{
	CMenuItem m [40];
	int	i, j, choice = 0;
	int	nOptions;
	int	nOptSmokeLag, optStaticParticles, optCollisions, optDisperse, 
			optRotate = -1, optAuxViews = -1, optMonitors = -1, optWiggle = -1, optWobble = -1;
	char	szSmokeQual [50];

pszSmokeSize [0] = TXT_SMALL;
pszSmokeSize [1] = TXT_MEDIUM;
pszSmokeSize [2] = TXT_LARGE;
pszSmokeSize [3] = TXT_VERY_LARGE;

pszSmokeAmount [0] = TXT_QUALITY_LOW;
pszSmokeAmount [1] = TXT_QUALITY_MED;
pszSmokeAmount [2] = TXT_QUALITY_HIGH;
pszSmokeAmount [3] = TXT_VERY_HIGH;
pszSmokeAmount [4] = TXT_EXTREME;

pszSmokeLife [0] = TXT_SHORT;
pszSmokeLife [1] = TXT_MEDIUM;
pszSmokeLife [2] = TXT_LONG;

pszSmokeQual [0] = TXT_STANDARD;
pszSmokeQual [1] = TXT_GOOD;
pszSmokeQual [2] = TXT_HIGH;

pszSmokeAlpha [0] = TXT_LOW;
pszSmokeAlpha [1] = TXT_MEDIUM;
pszSmokeAlpha [2] = TXT_HIGH;
pszSmokeAlpha [3] = TXT_VERY_HIGH;
pszSmokeAlpha [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	memset (&smokeOpts, 0xff, sizeof (smokeOpts));
	nOptions = 0;
	nOptSmokeLag = optStaticParticles = optCollisions = optDisperse = -1;

	m.AddCheck (nOptions, TXT_USE_SMOKE, extraGameInfo [0].bUseParticles, KEY_U, HTX_ADVRND_USESMOKE);
	smokeOpts.nUse = nOptions++;
	for (j = 1; j < 5; j++)
		smokeOpts.nSize [j] =
		smokeOpts.nDensity [j] = 
		smokeOpts.nAlpha [j] = -1;
	if (extraGameInfo [0].bUseParticles) {
		if (gameOpts->app.bExpertMode) {
			sprintf (szSmokeQual + 1, TXT_SMOKE_QUALITY, pszSmokeQual [NMCLAMP (gameOpts->render.particles.bSort, 0, 2)]);
			*szSmokeQual = *(TXT_SMOKE_QUALITY - 1);
			m.AddSlider (nOptions, szSmokeQual + 1, gameOpts->render.particles.bSort, 0, 2, KEY_Q, HTX_ADVRND_SMOKEQUAL);
			smokeOpts.nQuality = nOptions++;
			m.AddCheck (nOptions, TXT_SYNC_SIZES, gameOpts->render.particles.bSyncSizes, KEY_M, HTX_ADVRND_SYNCSIZES);
			smokeOpts.nSyncSizes = nOptions++;
			if (gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 0);
				for (j = 1; j < 5; j++) {
					gameOpts->render.particles.nSize [j] = gameOpts->render.particles.nSize [0];
					gameOpts->render.particles.nDens [j] = gameOpts->render.particles.nDens [0];
					gameOpts->render.particles.nAlpha [j] = gameOpts->render.particles.nAlpha [0];
					}
				}
			else {
				smokeOpts.nDensity [0] =
				smokeOpts.nSize [0] = -1;
				}
			if (!gameOpts->render.particles.bSyncSizes && gameOpts->render.particles.bPlayers) {
				m.AddText (nOptions, "", 0);
				nOptions++;
				}
			m.AddCheck (nOptions, TXT_SMOKE_PLAYERS, gameOpts->render.particles.bPlayers, KEY_Y, HTX_ADVRND_PLRSMOKE);
			smokeOpts.nPlayer = nOptions++;
			if (gameOpts->render.particles.bPlayers) {
				if (!gameOpts->render.particles.bSyncSizes)
					nOptions = AddSmokeSliders (m, nOptions, 1);
				m.AddCheck (nOptions, TXT_SMOKE_DECREASE_LAG, gameOpts->render.particles.bDecreaseLag, KEY_R, HTX_ADVREND_DECSMOKELAG);
				nOptSmokeLag = nOptions++;
				m.AddText (nOptions, "", 0);
				nOptions++;
				}
			else
				nOptSmokeLag = -1;
			m.AddCheck (nOptions, TXT_SMOKE_ROBOTS, gameOpts->render.particles.bRobots, KEY_O, HTX_ADVRND_BOTSMOKE);
			smokeOpts.nRobots = nOptions++;
			if (gameOpts->render.particles.bRobots && !gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 2);
				m.AddText (nOptions, "", 0);
				nOptions++;
				}
			m.AddCheck (nOptions, TXT_SMOKE_MISSILES, gameOpts->render.particles.bMissiles, KEY_M, HTX_ADVRND_MSLSMOKE);
			smokeOpts.nMissiles = nOptions++;
			if (gameOpts->render.particles.bMissiles && !gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 3);
				m.AddText (nOptions, "", 0);
				nOptions++;
				}
			m.AddCheck (nOptions, TXT_SMOKE_DEBRIS, gameOpts->render.particles.bDebris, KEY_D, HTX_ADVRND_DEBRISSMOKE);
			smokeOpts.nDebris = nOptions++;
			if (gameOpts->render.particles.bDebris && !gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 4);
				m.AddText (nOptions, "", 0);
				nOptions++;
				}
			m.AddCheck (nOptions, TXT_SMOKE_STATIC, gameOpts->render.particles.bStatic, KEY_T, HTX_ADVRND_STATICSMOKE);
			optStaticParticles = nOptions++;
#if 0
			m.AddCheck (nOptions, TXT_SMOKE_COLLISION, gameOpts->render.particles.bCollisions, KEY_I, HTX_ADVRND_SMOKECOLL);
			optCollisions = nOptions++;
#endif
			m.AddCheck (nOptions, TXT_SMOKE_DISPERSE, gameOpts->render.particles.bDisperse, KEY_D, HTX_ADVRND_SMOKEDISP);
			optDisperse = nOptions++;
			m.AddCheck (nOptions, TXT_ROTATE_SMOKE, gameOpts->render.particles.bRotate, KEY_R, HTX_ROTATE_SMOKE);
			optRotate = nOptions++;
			m.AddCheck (nOptions, TXT_SMOKE_AUXVIEWS, gameOpts->render.particles.bAuxViews, KEY_W, HTX_SMOKE_AUXVIEWS);
			optAuxViews = nOptions++;
			m.AddCheck (nOptions, TXT_SMOKE_MONITORS, gameOpts->render.particles.bMonitors, KEY_M, HTX_SMOKE_MONITORS);
			optMonitors = nOptions++;
			m.AddText (nOptions, "", 0);
			nOptions++;
			m.AddCheck (nOptions, TXT_SMOKE_BUBBLES, gameOpts->render.particles.bBubbles, KEY_B, HTX_SMOKE_BUBBLES);
			smokeOpts.nBubbles = nOptions++;
			if (gameOpts->render.particles.bBubbles) {
				m.AddCheck (nOptions, TXT_WIGGLE_BUBBLES, gameOpts->render.particles.bWiggleBubbles, KEY_I, HTX_WIGGLE_BUBBLES);
				optWiggle = nOptions++;
				m.AddCheck (nOptions, TXT_WOBBLE_BUBBLES, gameOpts->render.particles.bWobbleBubbles, KEY_I, HTX_WOBBLE_BUBBLES);
				optWobble = nOptions++;
				}
			}
		}
	else
		nOptSmokeLag =
		smokeOpts.nPlayer =
		smokeOpts.nRobots =
		smokeOpts.nMissiles =
		smokeOpts.nDebris =
		smokeOpts.nBubbles =
		optStaticParticles =
		optCollisions =
		optDisperse = 
		optRotate = 
		optAuxViews = 
		optMonitors = -1;

	Assert (sizeofa (m) >= nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_SMOKE_MENUTITLE, nOptions, m, &SmokeOptionsCallback, &choice);
		} while (i >= 0);
	if ((extraGameInfo [0].bUseParticles = m [smokeOpts.nUse].value)) {
		GET_VAL (gameOpts->render.particles.bPlayers, smokeOpts.nPlayer);
		GET_VAL (gameOpts->render.particles.bRobots, smokeOpts.nRobots);
		GET_VAL (gameOpts->render.particles.bMissiles, smokeOpts.nMissiles);
		GET_VAL (gameOpts->render.particles.bDebris, smokeOpts.nDebris);
		GET_VAL (gameOpts->render.particles.bStatic, optStaticParticles);
#if 0
		GET_VAL (gameOpts->render.particles.bCollisions, optCollisions);
#else
		gameOpts->render.particles.bCollisions = 0;
#endif
		GET_VAL (gameOpts->render.particles.bDisperse, optDisperse);
		GET_VAL (gameOpts->render.particles.bRotate, optRotate);
		GET_VAL (gameOpts->render.particles.bDecreaseLag, nOptSmokeLag);
		GET_VAL (gameOpts->render.particles.bAuxViews, optAuxViews);
		GET_VAL (gameOpts->render.particles.bMonitors, optMonitors);
		if (gameOpts->render.particles.bBubbles) {
			GET_VAL (gameOpts->render.particles.bWiggleBubbles, optWiggle);
			GET_VAL (gameOpts->render.particles.bWobbleBubbles, optWobble);
			}
		//GET_VAL (gameOpts->render.particles.bSyncSizes, smokeOpts.nSyncSizes);
		if (gameOpts->render.particles.bSyncSizes) {
			for (j = 1; j < 4; j++) {
				gameOpts->render.particles.nSize [j] = gameOpts->render.particles.nSize [0];
				gameOpts->render.particles.nDens [j] = gameOpts->render.particles.nDens [0];
				}
			}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static const char* pszLightningQuality [2];
static const char* pszLightningStyle [3];

int LightningOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + lightningOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseLightnings) {
	if (!(extraGameInfo [0].bUseLightnings = v))
		lightningManager.Shutdown (0);
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseLightnings) {
	m = menus + lightningOpts.nQuality;
	v = m->value;
	if (gameOpts->render.lightnings.nQuality != v) {
		gameOpts->render.lightnings.nQuality = v;
		sprintf (m->text, TXT_LIGHTNING_QUALITY, pszLightningQuality [v]);
		m->rebuild = 1;
		lightningManager.Shutdown (0);
		}
	m = menus + lightningOpts.nStyle;
	v = m->value;
	if (gameOpts->render.lightnings.nStyle != v) {
		gameOpts->render.lightnings.nStyle = v;
		sprintf (m->text, TXT_LIGHTNING_STYLE, pszLightningStyle [v]);
		m->rebuild = 1;
		}
	m = menus + lightningOpts.nOmega;
	v = m->value;
	if (gameOpts->render.lightnings.bOmega != v) {
		gameOpts->render.lightnings.bOmega = v;
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LightningOptionsMenu (void)
{
	CMenuItem m [15];
	int	i, choice = 0;
	int	nOptions;
	int	optDamage, optExplosions, optPlayers, optRobots, optStatic, optRobotOmega, optPlasma, optAuxViews, optMonitors;
	char	szQuality [50], szStyle [100];

	pszLightningQuality [0] = TXT_LOW;
	pszLightningQuality [1] = TXT_HIGH;
	pszLightningStyle [0] = TXT_LIGHTNING_ERRATIC;
	pszLightningStyle [1] = TXT_LIGHTNING_JAGGY;
	pszLightningStyle [2] = TXT_LIGHTNING_SMOOTH;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	lightningOpts.nQuality = 
	optDamage = 
	optExplosions = 
	optPlayers = 
	optRobots = 
	optStatic = 
	optRobotOmega = 
	optPlasma = 
	optAuxViews = 
	optMonitors = -1;

	m.AddCheck (nOptions, TXT_LIGHTNING_ENABLE, extraGameInfo [0].bUseLightnings, KEY_U, HTX_LIGHTNING_ENABLE);
	lightningOpts.nUse = nOptions++;
	if (extraGameInfo [0].bUseLightnings) {
		sprintf (szQuality + 1, TXT_LIGHTNING_QUALITY, pszLightningQuality [gameOpts->render.lightnings.nQuality]);
		*szQuality = *(TXT_LIGHTNING_QUALITY - 1);
		m.AddSlider (nOptions, szQuality + 1, gameOpts->render.lightnings.nQuality, 0, 1, KEY_R, HTX_LIGHTNING_QUALITY);
		lightningOpts.nQuality = nOptions++;
		sprintf (szStyle + 1, TXT_LIGHTNING_STYLE, pszLightningStyle [gameOpts->render.lightnings.nStyle]);
		*szStyle = *(TXT_LIGHTNING_STYLE - 1);
		m.AddSlider (nOptions, szStyle + 1, gameOpts->render.lightnings.nStyle, 0, 2, KEY_S, HTX_LIGHTNING_STYLE);
		lightningOpts.nStyle = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_PLASMA, gameOpts->render.lightnings.bPlasma, KEY_L, HTX_LIGHTNING_PLASMA);
		optPlasma = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_DAMAGE, gameOpts->render.lightnings.bDamage, KEY_D, HTX_LIGHTNING_DAMAGE);
		optDamage = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_EXPLOSIONS, gameOpts->render.lightnings.bExplosions, KEY_E, HTX_LIGHTNING_EXPLOSIONS);
		optExplosions = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_PLAYERS, gameOpts->render.lightnings.bPlayers, KEY_P, HTX_LIGHTNING_PLAYERS);
		optPlayers = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_ROBOTS, gameOpts->render.lightnings.bRobots, KEY_R, HTX_LIGHTNING_ROBOTS);
		optRobots = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_STATIC, gameOpts->render.lightnings.bStatic, KEY_T, HTX_LIGHTNING_STATIC);
		optStatic = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_OMEGA, gameOpts->render.lightnings.bOmega, KEY_O, HTX_LIGHTNING_OMEGA);
		lightningOpts.nOmega = nOptions++;
		if (gameOpts->render.lightnings.bOmega) {
			m.AddCheck (nOptions, TXT_LIGHTNING_ROBOT_OMEGA, gameOpts->render.lightnings.bRobotOmega, KEY_B, HTX_LIGHTNING_ROBOT_OMEGA);
			optRobotOmega = nOptions++;
			}
		m.AddCheck (nOptions, TXT_LIGHTNING_AUXVIEWS, gameOpts->render.lightnings.bAuxViews, KEY_D, HTX_LIGHTNING_AUXVIEWS);
		optAuxViews = nOptions++;
		m.AddCheck (nOptions, TXT_LIGHTNING_MONITORS, gameOpts->render.lightnings.bMonitors, KEY_M, HTX_LIGHTNING_MONITORS);
		optMonitors = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTNING_MENUTITLE, nOptions, m, &LightningOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (extraGameInfo [0].bUseLightnings) {
		GET_VAL (gameOpts->render.lightnings.bPlasma, optPlasma);
		GET_VAL (gameOpts->render.lightnings.bDamage, optDamage);
		GET_VAL (gameOpts->render.lightnings.bExplosions, optExplosions);
		GET_VAL (gameOpts->render.lightnings.bPlayers, optPlayers);
		GET_VAL (gameOpts->render.lightnings.bRobots, optRobots);
		GET_VAL (gameOpts->render.lightnings.bStatic, optStatic);
		GET_VAL (gameOpts->render.lightnings.bRobotOmega, optRobotOmega);
		GET_VAL (gameOpts->render.lightnings.bAuxViews, optAuxViews);
		GET_VAL (gameOpts->render.lightnings.bMonitors, optMonitors);
		}
	} while (i == -2);
if (!gameOpts->render.lightnings.bPlayers)
	lightningManager.DestroyForPlayers ();
if (!gameOpts->render.lightnings.bRobots)
	lightningManager.DestroyForRobots ();
if (!gameOpts->render.lightnings.bStatic)
	lightningManager.DestroyStatic ();
}

//------------------------------------------------------------------------------
//eof
