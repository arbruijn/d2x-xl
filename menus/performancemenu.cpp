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
	int	nUseCompSpeed;
	int	nCompSpeed;
} performanceOpts;

//------------------------------------------------------------------------------

void UseDefaultPerformanceSettings (void)
{
if (gameStates.app.bNostalgia || !gameStates.app.bUseDefaults)
	return;
	
SetMaxCustomDetails ();
gameOpts->render.nMaxFPS = 60;
gameOpts->render.effects.bTransparent = 1;
gameOpts->render.color.nLightmapRange = 5;
gameOpts->render.color.bMix = 1;
gameOpts->render.color.bWalls = 1;
gameOpts->render.cameras.bFitToWall = 0;
gameOpts->render.cameras.nSpeed = 5000;
gameOpts->ogl.bSetGammaRamp = 0;
gameStates.ogl.nContrast = 8;
if (gameStates.app.nCompSpeed == 0) {
	gameOpts->render.bUseLightmaps = 0;
	gameOpts->render.nQuality = 1;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 0;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	gameOpts->render.effects.nShrapnels = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->ogl.nMaxLightsPerFace = 4;
	gameOpts->ogl.nMaxLightsPerPass = 4;
	gameOpts->ogl.nMaxLightsPerObject = 4;
	extraGameInfo [0].bShadows = 0;
	extraGameInfo [0].bUseParticles = 0;
	extraGameInfo [0].bUseLightnings = 0;
	extraGameInfo [0].bUseCameras = 0;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 1) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 0;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 1;
	gameOpts->render.particles.bStatic = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 0;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 8;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 5;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 2) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 0;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 2;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 1;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 1;
	gameOpts->render.lightnings.bPlasma = 0;
	gameOpts->render.lightnings.bPlayers = 0;
	gameOpts->render.lightnings.bRobots = 0;
	gameOpts->render.lightnings.bDamage = 0;
	gameOpts->render.lightnings.bExplosions = 0;
	gameOpts->render.lightnings.bStatic = 0;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 12;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 3) {
	gameOpts->render.nQuality = 3;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 1;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 3;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 2;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 1;
	gameOpts->render.lightnings.bPlasma = 0;
	gameOpts->render.lightnings.bPlayers = 1;
	gameOpts->render.lightnings.bRobots = 1;
	gameOpts->render.lightnings.bDamage = 1;
	gameOpts->render.lightnings.bExplosions = 1;
	gameOpts->render.lightnings.bStatic = 1;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 16;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 1;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 4) {
	gameOpts->render.nQuality = 4;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 1;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 1;
	gameOpts->render.effects.nShrapnels = 4;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 3;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 1;
	gameOpts->render.particles.bMonitors = 1;
	gameOpts->render.lightnings.bMonitors = 1;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 2;
	gameOpts->render.lightnings.bPlasma = 1;
	gameOpts->render.lightnings.bPlayers = 1;
	gameOpts->render.lightnings.bRobots = 1;
	gameOpts->render.lightnings.bDamage = 1;
	gameOpts->render.lightnings.bExplosions = 1;
	gameOpts->render.lightnings.bStatic = 1;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 1;
	gameOpts->ogl.bLightObjects = 1;
	gameOpts->ogl.nMaxLightsPerFace = 20;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 16;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 1;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
}

// -----------------------------------------------------------------------------

static const char *pszCompSpeeds [5];

int PerformanceSettingsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem * m;
	int			v;

m = menu + performanceOpts.nUseCompSpeed;
v = m->m_value;
if (gameStates.app.bUseDefaults != v) {
	gameStates.app.bUseDefaults = v;
	key = -2;
	return nCurItem;
	}
if (gameStates.app.bUseDefaults) {
	m = menu + performanceOpts.nCompSpeed;
	v = m->m_value;
	if (gameStates.app.nCompSpeed != v) {
		gameStates.app.nCompSpeed = v;
		sprintf (m->m_text, TXT_COMP_SPEED, pszCompSpeeds [v]);
		}
	m->m_bRebuild = 1;
	}
return nCurItem;
}

// -----------------------------------------------------------------------------

void PerformanceSettingsMenu (void)
{
	int	i, choice = gameStates.app.nDetailLevel;
	char	szCompSpeed [50];
	CMenu m;

pszCompSpeeds [0] = TXT_VERY_SLOW;
pszCompSpeeds [1] = TXT_SLOW;
pszCompSpeeds [2] = TXT_MEDIUM;
pszCompSpeeds [3] = TXT_FAST;
pszCompSpeeds [4] = TXT_VERY_FAST;

do {
	i = 0;
	m.Destroy ();
	m.Create (10);
	m.AddText ("", 0);
	performanceOpts.nUseCompSpeed = m.AddCheck (TXT_USE_DEFAULTS, gameStates.app.bUseDefaults, KEY_U, HTX_MISC_DEFAULTS);
	if (gameStates.app.bUseDefaults) {
		sprintf (szCompSpeed + 1, TXT_COMP_SPEED, pszCompSpeeds [gameStates.app.nCompSpeed]);
		*szCompSpeed = *(TXT_COMP_SPEED - 1);
		performanceOpts.nCompSpeed = m.AddSlider (szCompSpeed + 1, gameStates.app.nCompSpeed, 0, 4, KEY_C, HTX_MISC_COMPSPEED);
		}
	do {
		i = m.Menu (NULL, TXT_SETPERF_MENUTITLE, PerformanceSettingsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);
UseDefaultPerformanceSettings ();
}

//------------------------------------------------------------------------------
//eof
