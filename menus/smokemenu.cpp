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
#ifdef EDITOR
#	include "editor/editor.h"
#endif

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

static const char* pszSmokeAmount [5];
static const char* pszSmokeSize [4];
static const char* pszSmokeLife [3];
static const char* pszSmokeQual [3];
static const char* pszSmokeAlpha [5];

int SmokeOptionsCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			i, v;

m = menu + smokeOpts.nUse;
v = m->m_value;
if (v != extraGameInfo [0].bUseParticles) {
	extraGameInfo [0].bUseParticles = v;
	key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseParticles) {
	m = menu + smokeOpts.nQuality;
	v = m->m_value;
	if (v != gameOpts->render.particles.bSort) {
		gameOpts->render.particles.bSort = v;
		sprintf (m->m_text, TXT_SMOKE_QUALITY, pszSmokeQual [v]);
		m->m_bRebuild = 1;
		return nCurItem;
		}
	m = menu + smokeOpts.nSyncSizes;
	v = m->m_value;
	if (v != gameOpts->render.particles.bSyncSizes) {
		gameOpts->render.particles.bSyncSizes = v;
		key = -2;
		return nCurItem;
		}
	m = menu + smokeOpts.nPlayer;
	v = m->m_value;
	if (gameOpts->render.particles.bPlayers != v) {
		gameOpts->render.particles.bPlayers = v;
		key = -2;
		}
	m = menu + smokeOpts.nRobots;
	v = m->m_value;
	if (gameOpts->render.particles.bRobots != v) {
		gameOpts->render.particles.bRobots = v;
		key = -2;
		}
	m = menu + smokeOpts.nMissiles;
	v = m->m_value;
	if (gameOpts->render.particles.bMissiles != v) {
		gameOpts->render.particles.bMissiles = v;
		key = -2;
		}
	m = menu + smokeOpts.nDebris;
	v = m->m_value;
	if (gameOpts->render.particles.bDebris != v) {
		gameOpts->render.particles.bDebris = v;
		key = -2;
		}
	m = menu + smokeOpts.nBubbles;
	v = m->m_value;
	if (gameOpts->render.particles.bBubbles != v) {
		gameOpts->render.particles.bBubbles = v;
		key = -2;
		}
	if (gameOpts->render.particles.bSyncSizes) {
		m = menu + smokeOpts.nDensity [0];
		v = m->m_value;
		if (gameOpts->render.particles.nDens [0] != v) {
			gameOpts->render.particles.nDens [0] = v;
			sprintf (m->m_text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
			m->m_bRebuild = 1;
			}
		m = menu + smokeOpts.nSize [0];
		v = m->m_value;
		if (gameOpts->render.particles.nSize [0] != v) {
			gameOpts->render.particles.nSize [0] = v;
			sprintf (m->m_text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
			m->m_bRebuild = 1;
			}
		m = menu + smokeOpts.nAlpha [0];
		v = m->m_value;
		if (v != gameOpts->render.particles.nAlpha [0]) {
			gameOpts->render.particles.nAlpha [0] = v;
			sprintf (m->m_text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
			m->m_bRebuild = 1;
			}
		}
	else {
		for (i = 1; i < 5; i++) {
			if (smokeOpts.nDensity [i] >= 0) {
				m = menu + smokeOpts.nDensity [i];
				v = m->m_value;
				if (gameOpts->render.particles.nDens [i] != v) {
					gameOpts->render.particles.nDens [i] = v;
					sprintf (m->m_text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
					m->m_bRebuild = 1;
					}
				}
			if (smokeOpts.nSize [i] >= 0) {
				m = menu + smokeOpts.nSize [i];
				v = m->m_value;
				if (gameOpts->render.particles.nSize [i] != v) {
					gameOpts->render.particles.nSize [i] = v;
					sprintf (m->m_text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
					m->m_bRebuild = 1;
					}
				}
			if (smokeOpts.nLife [i] >= 0) {
				m = menu + smokeOpts.nLife [i];
				v = m->m_value;
				if (gameOpts->render.particles.nLife [i] != v) {
					gameOpts->render.particles.nLife [i] = v;
					sprintf (m->m_text, TXT_SMOKE_LIFE, pszSmokeLife [v]);
					m->m_bRebuild = 1;
					}
				}
			if (smokeOpts.nAlpha [i] >= 0) {
				m = menu + smokeOpts.nAlpha [i];
				v = m->m_value;
				if (v != gameOpts->render.particles.nAlpha [i]) {
					gameOpts->render.particles.nAlpha [i] = v;
					sprintf (m->m_text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
					m->m_bRebuild = 1;
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

void AddSmokeSliders (CMenu& m, int i)
{
sprintf (szSmokeDens [i] + 1, TXT_SMOKE_DENS, pszSmokeAmount [NMCLAMP (gameOpts->render.particles.nDens [i], 0, 4)]);
*szSmokeDens [i] = *(TXT_SMOKE_DENS - 1);
smokeOpts.nDensity [i] = m.AddSlider (szSmokeDens [i] + 1, gameOpts->render.particles.nDens [i], 0, 4, KEY_P, HTX_ADVRND_SMOKEDENS);
sprintf (szSmokeSize [i] + 1, TXT_SMOKE_SIZE, pszSmokeSize [NMCLAMP (gameOpts->render.particles.nSize [i], 0, 3)]);
*szSmokeSize [i] = *(TXT_SMOKE_SIZE - 1);
smokeOpts.nSize [i] = m.AddSlider (szSmokeSize [i] + 1, gameOpts->render.particles.nSize [i], 0, 3, KEY_Z, HTX_ADVRND_PARTSIZE);
if (i < 3)
	smokeOpts.nLife [i] = -1;
else {
	sprintf (szSmokeLife [i] + 1, TXT_SMOKE_LIFE, pszSmokeLife [NMCLAMP (gameOpts->render.particles.nLife [i], 0, 3)]);
	*szSmokeLife [i] = *(TXT_SMOKE_LIFE - 1);
	smokeOpts.nLife [i] = m.AddSlider (szSmokeLife [i] + 1, gameOpts->render.particles.nLife [i], 0, 2, KEY_L, HTX_SMOKE_LIFE);
	}
sprintf (szSmokeAlpha [i] + 1, TXT_SMOKE_ALPHA, pszSmokeAlpha [NMCLAMP (gameOpts->render.particles.nAlpha [i], 0, 4)]);
*szSmokeAlpha [i] = *(TXT_SMOKE_SIZE - 1);
smokeOpts.nAlpha [i] = m.AddSlider (szSmokeAlpha [i] + 1, gameOpts->render.particles.nAlpha [i], 0, 4, KEY_Z, HTX_ADVRND_SMOKEALPHA);
}

//------------------------------------------------------------------------------

void SmokeOptionsMenu (void)
{
	CMenu m;
	int	i, j, choice = 0;
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
	m.Destroy ();
	m.Create (50);
	memset (&smokeOpts, 0xff, sizeof (smokeOpts));
	nOptSmokeLag = optStaticParticles = optCollisions = optDisperse = -1;

	smokeOpts.nUse = m.AddCheck (TXT_USE_SMOKE, extraGameInfo [0].bUseParticles, KEY_U, HTX_ADVRND_USESMOKE);
	for (j = 1; j < 5; j++)
		smokeOpts.nSize [j] =
		smokeOpts.nDensity [j] = 
		smokeOpts.nAlpha [j] = -1;
	if (extraGameInfo [0].bUseParticles) {
		if (gameOpts->app.bExpertMode) {
			sprintf (szSmokeQual + 1, TXT_SMOKE_QUALITY, pszSmokeQual [NMCLAMP (gameOpts->render.particles.bSort, 0, 2)]);
			*szSmokeQual = *(TXT_SMOKE_QUALITY - 1);
			smokeOpts.nQuality = m.AddSlider (szSmokeQual + 1, gameOpts->render.particles.bSort, 0, 2, KEY_Q, HTX_ADVRND_SMOKEQUAL);
			smokeOpts.nSyncSizes = m.AddCheck (TXT_SYNC_SIZES, gameOpts->render.particles.bSyncSizes, KEY_M, HTX_ADVRND_SYNCSIZES);
			if (gameOpts->render.particles.bSyncSizes) {
				AddSmokeSliders (m, 0);
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
			if (!gameOpts->render.particles.bSyncSizes && gameOpts->render.particles.bPlayers)
				m.AddText ("", 0);
			smokeOpts.nPlayer = m.AddCheck (TXT_SMOKE_PLAYERS, gameOpts->render.particles.bPlayers, KEY_Y, HTX_ADVRND_PLRSMOKE);
			if (gameOpts->render.particles.bPlayers) {
				if (!gameOpts->render.particles.bSyncSizes)
					AddSmokeSliders (m, 1);
				nOptSmokeLag = m.AddCheck (TXT_SMOKE_DECREASE_LAG, gameOpts->render.particles.bDecreaseLag, KEY_R, HTX_ADVREND_DECSMOKELAG);
				m.AddText ("", 0);
				}
			else
				nOptSmokeLag = -1;
			smokeOpts.nRobots = m.AddCheck (TXT_SMOKE_ROBOTS, gameOpts->render.particles.bRobots, KEY_O, HTX_ADVRND_BOTSMOKE);
			if (gameOpts->render.particles.bRobots && !gameOpts->render.particles.bSyncSizes) {
				AddSmokeSliders (m, 2);
				m.AddText ("", 0);
				}
			smokeOpts.nMissiles = m.AddCheck (TXT_SMOKE_MISSILES, gameOpts->render.particles.bMissiles, KEY_M, HTX_ADVRND_MSLSMOKE);
			if (gameOpts->render.particles.bMissiles && !gameOpts->render.particles.bSyncSizes) {
				AddSmokeSliders (m, 3);
				m.AddText ("", 0);
				}
			smokeOpts.nDebris = m.AddCheck (TXT_SMOKE_DEBRIS, gameOpts->render.particles.bDebris, KEY_D, HTX_ADVRND_DEBRISSMOKE);
			if (gameOpts->render.particles.bDebris && !gameOpts->render.particles.bSyncSizes) {
				AddSmokeSliders (m, 4);
				m.AddText ("", 0);
				}
			optStaticParticles = m.AddCheck (TXT_SMOKE_STATIC, gameOpts->render.particles.bStatic, KEY_T, HTX_ADVRND_STATICSMOKE);
#if 0
			optCollisions = m.AddCheck (TXT_SMOKE_COLLISION, gameOpts->render.particles.bCollisions, KEY_I, HTX_ADVRND_SMOKECOLL);
#endif
			optDisperse = m.AddCheck (TXT_SMOKE_DISPERSE, gameOpts->render.particles.bDisperse, KEY_D, HTX_ADVRND_SMOKEDISP);
			optRotate = m.AddCheck (TXT_ROTATE_SMOKE, gameOpts->render.particles.bRotate, KEY_R, HTX_ROTATE_SMOKE);
			optAuxViews = m.AddCheck (TXT_SMOKE_AUXVIEWS, gameOpts->render.particles.bAuxViews, KEY_W, HTX_SMOKE_AUXVIEWS);
			optMonitors = m.AddCheck (TXT_SMOKE_MONITORS, gameOpts->render.particles.bMonitors, KEY_M, HTX_SMOKE_MONITORS);
			m.AddText ("", 0);
			smokeOpts.nBubbles = m.AddCheck (TXT_SMOKE_BUBBLES, gameOpts->render.particles.bBubbles, KEY_B, HTX_SMOKE_BUBBLES);
			if (gameOpts->render.particles.bBubbles) {
				optWiggle = m.AddCheck (TXT_WIGGLE_BUBBLES, gameOpts->render.particles.bWiggleBubbles, KEY_I, HTX_WIGGLE_BUBBLES);
				optWobble = m.AddCheck (TXT_WOBBLE_BUBBLES, gameOpts->render.particles.bWobbleBubbles, KEY_I, HTX_WOBBLE_BUBBLES);
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

	do {
		i = m.Menu (NULL, TXT_SMOKE_MENUTITLE, SmokeOptionsCallback, &choice);
		} while (i >= 0);
	if ((extraGameInfo [0].bUseParticles = m [smokeOpts.nUse].m_value)) {
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
//eof
