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

void DefaultEffectSettings (void);

//------------------------------------------------------------------------------

static struct {
	int	nExplShrapnels;
	int	nLightTrails;
	int	nSparks;
	int	nThrusters;
} effectOpts;

//------------------------------------------------------------------------------

static int nLightTrails;

static const char* pszExplShrapnels [5];
static const char* pszLightTrails [3];
static const char* pszThrusters [3];

int EffectOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

if (effectOpts.nExplShrapnels >= 0) {
	m = menu + effectOpts.nExplShrapnels;
	v = m->m_value;
	if (gameOpts->render.effects.nShrapnels != v) {
		gameOpts->render.effects.nShrapnels = v;
		sprintf (m->m_text, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [v]);
		m->m_bRebuild = -1;
		}
	}

#if SIMPLE_MENUS

m = menu + effectOpts.nThrusters;
v = m->m_value;
if (extraGameInfo [0].bThrusterFlames != v) {
	extraGameInfo [0].bThrusterFlames = v;
	sprintf (m->m_text, TXT_THRUSTER_FLAMES, pszThrusters [v]);
	m->m_bRebuild = 1;
	}

m = menu + effectOpts.nLightTrails;
v = m->m_value;
if (nLightTrails != v) {
	nLightTrails = v;
	key = -2;
	}

#endif

return nCurItem;
}

//------------------------------------------------------------------------------

#if SIMPLE_MENUS

void EffectOptionsMenu (void)
{
	CMenu	m;
	int	i, j, choice = 0;
	int	optThrusterFlame, optGatlingTrails, optSoftParticles [3];
#if 0
	int	optShockwaves;
#endif
	int	bEnergySparks = gameOpts->render.effects.bEnergySparks;
	char	szSlider [50];

pszExplShrapnels [0] = TXT_NONE;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

pszLightTrails [0] = TXT_NONE;
pszLightTrails [1] = TXT_LOW;
pszLightTrails [2] = TXT_HIGH;

nLightTrails = extraGameInfo [0].bLightTrails ? (gameOpts->render.particles.bPlasmaTrails << 1) : 0;
do {
	m.Destroy ();
	m.Create (30);

	if (extraGameInfo [0].bUseParticles) {
		sprintf (szSlider + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
		*szSlider = *(TXT_EXPLOSION_SHRAPNELS - 1);
		effectOpts.nExplShrapnels = m.AddSlider (szSlider + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_P, HTX_EXPLOSION_SHRAPNELS);
		}
	else
		effectOpts.nExplShrapnels = -1;

	sprintf (szSlider + 1, TXT_LIGHTTRAIL_QUAL, pszLightTrails [nLightTrails]);
	*szSlider = *(TXT_LIGHTTRAIL_QUAL - 1);
	effectOpts.nLightTrails = m.AddSlider (szSlider + 1, nLightTrails, 0, 1 + extraGameInfo [0].bUseParticles, KEY_P, HTX_LIGHTTRAIL_QUAL);

	sprintf (szSlider + 1, TXT_THRUSTER_FLAMES, pszThrusters [extraGameInfo [0].bThrusterFlames]);
	*szSlider = *(TXT_THRUSTER_FLAMES - 1);
	effectOpts.nThrusters = m.AddSlider (szSlider + 1, extraGameInfo [0].bThrusterFlames, 0, 2, KEY_T, HTX_THRUSTER_FLAMES);

	m.AddText ("");
	if (extraGameInfo [0].bUseParticles)
		optGatlingTrails = m.AddCheck (TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
	else
		optGatlingTrails = -1;
	optSoftParticles [0] = m.AddCheck (TXT_SOFT_SPRITES, (gameOpts->render.effects.bSoftParticles & 1) != 0, KEY_I, HTX_SOFT_SPRITES);
	optSoftParticles [1] = m.AddCheck (TXT_SOFT_SPARKS, (gameOpts->render.effects.bSoftParticles & 2) != 0, KEY_A, HTX_SOFT_SPARKS);
	if (extraGameInfo [0].bUseParticles)
		optSoftParticles [2] = m.AddCheck (TXT_SOFT_SMOKE, (gameOpts->render.effects.bSoftParticles & 4) != 0, KEY_O, HTX_SOFT_SMOKE);
	else
		optSoftParticles [2] = -1;
	m [optThrusterFlame + extraGameInfo [0].bThrusterFlames].m_value = 1;
	for (;;) {
		i = m.Menu (NULL, TXT_EFFECT_MENUTITLE, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	for (j = 0; j < 3; j++) {
		if (optSoftParticles [j] >= 0) {
			if (m [optSoftParticles [j]].m_value)
				gameOpts->render.effects.bSoftParticles |= 1 << j;
			else
				gameOpts->render.effects.bSoftParticles &= ~(1 << j);
			}
		if (m [optThrusterFlame + j].m_value) {
			extraGameInfo [0].bThrusterFlames = j;
			break;
			}
		}
	if ((extraGameInfo [0].bLightTrails = (nLightTrails != 0)))
		gameOpts->render.particles.bPlasmaTrails = (nLightTrails == 2);
	} while (i == -2);

SetDebrisCollisions ();
DefaultEffectSettings ();
if (gameOpts->render.effects.bEnergySparks && gameStates.app.bGameRunning && !sparkManager.HaveSparks ())
	sparkManager.Setup ();
}

#else

void EffectOptionsMenu (void)
{
	CMenu	m;
	int	i, j, choice = 0;
	int	optTranspExpl, optThrusterFlame, optDmgExpl, optAutoTransp, optPlayerShields, optSoftParticles [3],
			optRobotShields, optShieldHits, optTracers, optGatlingTrails, optExplBlast, optMovingSparks;
#if 0
	int	optShockwaves;
#endif
	int	bEnergySparks = gameOpts->render.effects.bEnergySparks;
	char	szSlider [50];

pszExplShrapnels [0] = TXT_NONE;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

do {
	m.Destroy ();
	m.Create (30);

	sprintf (szSlider + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
	*szSlider = *(TXT_EXPLOSION_SHRAPNELS - 1);
	effectOpts.nExplShrapnels = m.AddSlider (szSlider + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_P, HTX_EXPLOSION_SHRAPNELS);
	optExplBlast = m.AddCheck (TXT_EXPLOSION_BLAST, gameOpts->render.effects.bExplBlasts, KEY_B, HTX_EXPLOSION_BLAST);
	optDmgExpl = m.AddCheck (TXT_DMG_EXPL, extraGameInfo [0].bDamageExplosions, KEY_X, HTX_RENDER_DMGEXPL);
	optThrusterFlame = m.AddRadio (TXT_NO_THRUSTER_FLAME, 0, KEY_F, HTX_RENDER_THRUSTER);
	m.AddRadio (TXT_2D_THRUSTER_FLAME, 0, KEY_2, HTX_RENDER_THRUSTER);
	m.AddRadio (TXT_3D_THRUSTER_FLAME, 0, KEY_3, HTX_RENDER_THRUSTER);
	m [optThrusterFlame + extraGameInfo [0].bThrusterFlames].m_value = 1;
	effectOpts.nSparks = m.AddCheck (TXT_RENDER_SPARKS, gameOpts->render.effects.bEnergySparks, KEY_P, HTX_RENDER_SPARKS);
	if (gameOpts->render.effects.bEnergySparks)
		optMovingSparks = m.AddCheck (TXT_MOVING_SPARKS, gameOpts->render.effects.bMovingSparks, KEY_V, HTX_MOVING_SPARKS);
	else
		optMovingSparks = -1;
	if (gameOpts->render.textures.bUseHires [0])
		optTranspExpl = -1;
	else
		optTranspExpl = m.AddCheck (TXT_TRANSP_EFFECTS, gameOpts->render.effects.bTransparent, KEY_E, HTX_ADVRND_TRANSPFX);
	optSoftParticles [0] = m.AddCheck (TXT_SOFT_SPRITES, (gameOpts->render.effects.bSoftParticles & 1) != 0, KEY_I, HTX_SOFT_SPRITES);
	if (gameOpts->render.effects.bEnergySparks)
		optSoftParticles [1] = m.AddCheck (TXT_SOFT_SPARKS, (gameOpts->render.effects.bSoftParticles & 2) != 0, KEY_A, HTX_SOFT_SPARKS);
	else
		optSoftParticles [1] = -1; 
	if (extraGameInfo [0].bUseParticles)
		optSoftParticles [2] = m.AddCheck (TXT_SOFT_SMOKE, (gameOpts->render.effects.bSoftParticles & 4) != 0, KEY_O, HTX_SOFT_SMOKE);
	else
		optSoftParticles [2] = -1;
	optAutoTransp = m.AddCheck (TXT_AUTO_TRANSPARENCY, gameOpts->render.effects.bAutoTransparency, KEY_A, HTX_RENDER_AUTOTRANSP);
	optPlayerShields = m.AddCheck (TXT_RENDER_SHIELDS, extraGameInfo [0].bPlayerShield, KEY_P, HTX_RENDER_SHIELDS);
	optRobotShields = m.AddCheck (TXT_ROBOT_SHIELDS, gameOpts->render.effects.bRobotShields, KEY_O, HTX_ROBOT_SHIELDS);
	optShieldHits = m.AddCheck (TXT_SHIELD_HITS, gameOpts->render.effects.bOnlyShieldHits, KEY_H, HTX_SHIELD_HITS);
	optTracers = m.AddCheck (TXT_RENDER_TRACERS, extraGameInfo [0].bTracers, KEY_T, HTX_RENDER_TRACERS);
	optGatlingTrails = m.AddCheck (TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
#if 0
	optShockwaves = m.AddCheck (TXT_RENDER_SHKWAVES, extraGameInfo [0].bShockwaves, KEY_S, HTX_RENDER_SHKWAVES);
#endif
	for (;;) {
		i = m.Menu (NULL, TXT_EFFECT_MENUTITLE, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.effects.bEnergySparks = m [effectOpts.nSparks].m_value;
	if ((gameOpts->render.effects.bEnergySparks != bEnergySparks) && gameStates.app.bGameRunning) {
		if (gameOpts->render.effects.bEnergySparks)
			sparkManager.Setup ();
		else
			sparkManager.Destroy ();
		}
	GET_VAL (gameOpts->render.effects.bTransparent, optTranspExpl);
	for (j = 0; j < 3; j++)
	if (optSoftParticles [j] >= 0) {
		if (m [optSoftParticles [j]].m_value)
			gameOpts->render.effects.bSoftParticles |= 1 << j;
		else
			gameOpts->render.effects.bSoftParticles &= ~(1 << j);
		}
	GET_VAL (gameOpts->render.effects.bMovingSparks, optMovingSparks);
	gameOpts->render.effects.bAutoTransparency = m [optAutoTransp].m_value;
	gameOpts->render.effects.bExplBlasts = m [optExplBlast].m_value;
	extraGameInfo [0].bTracers = m [optTracers].m_value;
	extraGameInfo [0].bGatlingTrails = m [optGatlingTrails].m_value;
	extraGameInfo [0].bShockwaves = 0; //m [optShockwaves].m_value;
	extraGameInfo [0].bDamageExplosions = m [optDmgExpl].m_value;
	for (j = 0; j < 3; j++)
		if (m [optThrusterFlame + j].m_value) {
			extraGameInfo [0].bThrusterFlames = j;
			break;
			}
	extraGameInfo [0].bPlayerShield = m [optPlayerShields].m_value;
	gameOpts->render.effects.bRobotShields = m [optRobotShields].m_value;
	gameOpts->render.effects.bOnlyShieldHits = m [optShieldHits].m_value;
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

#endif

//------------------------------------------------------------------------------
//eof
