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
#include "savegame.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "playerprofile.h"
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

#if !SIMPLE_MENUS

static struct {
	int32_t	nPlayer;
	int32_t	nRobots;
	int32_t	nMissiles;
	int32_t	nDebris;
	int32_t	nDensity [5];
	int32_t	nLife [5];
	int32_t	nSize [5];
	int32_t	nAlpha [5];
	int32_t	nSyncSizes;
} smokeOpts;

//------------------------------------------------------------------------------

static int32_t HaveSmokeSliders (int32_t i)
{
switch (i) {
	case 1:
		return gameOpts->render.particles.bPlayers;
	case 2:
		return gameOpts->render.particles.bRobots;
	case 3:
		return gameOpts->render.particles.bMissiles;
	case 4:
		return gameOpts->render.particles.bDebris;
	default:
		return 0;
	}
}

//------------------------------------------------------------------------------

static const char* pszSmokeAmount [5];
static const char* pszSmokeSize [4];
static const char* pszSmokeLife [3];
static const char* pszSmokeQual [3];
static const char* pszSmokeAlpha [5];

int32_t SmokeOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int32_t			i, v;
	char			szId [100];

if ((m = menu ["sync sizes"])) {
	v = m->Value ();
	if (v != gameOpts->render.particles.bSyncSizes) {
		gameOpts->render.particles.bSyncSizes = v;
		key = -2;
		return nCurItem;
		}
	}
if ((m = menu ["player ships"])) {
	v = m->Value ();
	if (gameOpts->render.particles.bPlayers != v) {
		gameOpts->render.particles.bPlayers = v;
		key = -2;
		}
	}
if ((m = menu ["robots"])) {
	v = m->Value ();
	if (gameOpts->render.particles.bRobots != v) {
		gameOpts->render.particles.bRobots = v;
		key = -2;
		}
	}
if ((m = menu ["missiles"])) {
	v = m->Value ();
	if (gameOpts->render.particles.bMissiles != v) {
		gameOpts->render.particles.bMissiles = v;
		key = -2;
		}
	}
if ((m = menu ["debris"])) {
	v = m->Value ();
	if (gameOpts->render.particles.bDebris != v) {
		gameOpts->render.particles.bDebris = v;
		key = -2;
		}
	}
if (gameOpts->render.particles.bSyncSizes) {
	if ((m = menu ["density [0]"])) {
		v = m->Value ();
		if (gameOpts->render.particles.nDens [0] != v) {
			gameOpts->render.particles.nDens [0] = v;
			sprintf (m->m_text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
			m->m_bRebuild = 1;
			}
		}
	if ((m = menu ["size [0]"])) {
		v = m->Value ();
		if (gameOpts->render.particles.nSize [0] != v) {
			gameOpts->render.particles.nSize [0] = v;
			sprintf (m->m_text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
			m->m_bRebuild = 1;
			}
		}
	if ((m = menu ["alpha [0]"])) {
		v = m->Value ();
		if (v != gameOpts->render.particles.nAlpha [0]) {
			gameOpts->render.particles.nAlpha [0] = v;
			sprintf (m->m_text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
			m->m_bRebuild = 1;
			}
		}
	}
else {
	for (i = 1; i < 5; i++) {
		if (!HaveSmokeSliders (i))
			continue;

		sprintf (szId, "density [%d]", i);
		if ((m = menu [szId])) {
			v = m->Value ();
			if (gameOpts->render.particles.nDens [i] != v) {
				gameOpts->render.particles.nDens [i] = v;
				sprintf (m->m_text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
				m->m_bRebuild = 1;
				}
			}
		sprintf (szId, "size [%d]", i);
		if ((m = menu [szId])) {
			v = m->Value ();
			if (gameOpts->render.particles.nSize [i] != v + 1) {
				gameOpts->render.particles.nSize [i] = v + 1;
				sprintf (m->m_text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
				m->m_bRebuild = 1;
				}
			}
		if (i > 2) {
			sprintf (szId, "life [%d]", i);
			if ((m = menu [szId])) {
				v = m->Value ();
				if (gameOpts->render.particles.nLife [i] != v + 1) {
					gameOpts->render.particles.nLife [i] = v + 1;
					sprintf (m->m_text, TXT_SMOKE_LIFE, pszSmokeLife [v]);
					m->m_bRebuild = 1;
					}
				}
			}
		sprintf (szId, "alpha [%d]", i);
		if ((m = menu [szId])) {
			v = m->Value ();
			if (v != gameOpts->render.particles.nAlpha [i]) {
				gameOpts->render.particles.nAlpha [i] = v;
				sprintf (m->m_text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
				m->m_bRebuild = 1;
				}
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static char szSmokeDens [5][50];
static char szSmokeSize [5][50];
static char szSmokeLife [5][50];
static char szSmokeAlpha [5][50];

void AddSmokeSliders (CMenu& m, int32_t i)
{
if (gameOpts->render.particles.bSyncSizes && (i > 0))
	return;

	char szId [100];

sprintf (szSmokeDens [i] + 1, TXT_SMOKE_DENS, pszSmokeAmount [NMCLAMP (gameOpts->render.particles.nDens [i], 0, 4)]);
*szSmokeDens [i] = *(TXT_SMOKE_DENS - 1);
sprintf (szId, "density [%d]", i);
m.AddSlider (szId, szSmokeDens [i] + 1, gameOpts->render.particles.nDens [i], 0, 4, KEY_P, HTX_ADVRND_SMOKEDENS);

sprintf (szSmokeSize [i] + 1, TXT_SMOKE_SIZE, pszSmokeSize [NMCLAMP (gameOpts->render.particles.nSize [i] - 1, 0, 3)]);
*szSmokeSize [i] = *(TXT_SMOKE_SIZE - 1);
sprintf (szId, "size [%d]", i);
m.AddSlider (szId, szSmokeSize [i] + 1, gameOpts->render.particles.nSize [i] - 1, 0, 3, KEY_Z, HTX_ADVRND_PARTSIZE);

if (i > 2) {
	sprintf (szSmokeLife [i] + 1, TXT_SMOKE_LIFE, pszSmokeLife [NMCLAMP (gameOpts->render.particles.nLife [i] - 1, 0, 3)]);
	*szSmokeLife [i] = *(TXT_SMOKE_LIFE - 1);
	sprintf (szId, "life [%d]", i);
	m.AddSlider (szId, szSmokeLife [i] + 1, gameOpts->render.particles.nLife [i] - 1, 0, 2, KEY_L, HTX_SMOKE_LIFE);
	}

sprintf (szSmokeAlpha [i] + 1, TXT_SMOKE_ALPHA, pszSmokeAlpha [NMCLAMP (gameOpts->render.particles.nAlpha [i], 0, 4)]);
*szSmokeAlpha [i] = *(TXT_SMOKE_SIZE - 1);
sprintf (szId, "alpha [%d]", i);
m.AddSlider (szId, szSmokeAlpha [i] + 1, gameOpts->render.particles.nAlpha [i], 0, 4, KEY_Z, HTX_ADVRND_SMOKEALPHA);
}

//------------------------------------------------------------------------------

inline void AddSpace (CMenu& m, bool bNeedSpace)
{
if (!gameOpts->render.particles.bSyncSizes && bNeedSpace)
	m.AddText ("", "");
}

//------------------------------------------------------------------------------

void SmokeDetailsMenu (void)
{
	static int32_t choice = 0;

	CMenu m;
	int32_t	i, j;

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

	for (j = 1; j < 5; j++)
		smokeOpts.nSize [j] =
		smokeOpts.nDensity [j] = 
		smokeOpts.nAlpha [j] = -1;
	m.AddCheck ("sync sizes", TXT_SYNC_SIZES, gameOpts->render.particles.bSyncSizes, KEY_M, HTX_ADVRND_SYNCSIZES);
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

	m.AddText ("", "");
	m.AddCheck ("player ships", TXT_SMOKE_PLAYERS, gameOpts->render.particles.bPlayers, KEY_Y, HTX_ADVRND_PLRSMOKE);
	if (gameOpts->render.particles.bPlayers) {
		AddSmokeSliders (m, 1);
		m.AddCheck ("reduce lag", TXT_SMOKE_DECREASE_LAG, gameOpts->render.particles.bDecreaseLag, KEY_R, HTX_ADVREND_DECSMOKELAG);
		}

	AddSpace (m, gameOpts->render.particles.bPlayers || gameOpts->render.particles.bRobots);
	m.AddCheck ("robots", TXT_SMOKE_ROBOTS, gameOpts->render.particles.bRobots, KEY_O, HTX_ADVRND_BOTSMOKE);
	if (gameOpts->render.particles.bRobots)
		AddSmokeSliders (m, 2);

	AddSpace (m, gameOpts->render.particles.bRobots || gameOpts->render.particles.bMissiles);
	m.AddCheck ("missiles", TXT_SMOKE_MISSILES, gameOpts->render.particles.bMissiles, KEY_M, HTX_ADVRND_MSLSMOKE);
	if (gameOpts->render.particles.bMissiles)
		AddSmokeSliders (m, 3);

	AddSpace (m, gameOpts->render.particles.bMissiles || gameOpts->render.particles.bDebris);
	m.AddCheck ("debris", TXT_SMOKE_DEBRIS, gameOpts->render.particles.bDebris, KEY_D, HTX_ADVRND_DEBRISSMOKE);
	if (gameOpts->render.particles.bDebris)
		AddSmokeSliders (m, 4);

	m.AddText ("", "");
#if 1
	m.AddCheck ("check collisions", TXT_SMOKE_COLLISION, gameOpts->render.particles.bCollisions, KEY_I, HTX_ADVRND_SMOKECOLL);
#endif
	m.AddCheck ("disperse", TXT_SMOKE_DISPERSE, gameOpts->render.particles.bDisperse, KEY_D, HTX_ADVRND_SMOKEDISP);
	m.AddCheck ("rotate", TXT_ROTATE_SMOKE, gameOpts->render.particles.bRotate, KEY_R, HTX_ROTATE_SMOKE);
	m.AddCheck ("aux views", TXT_SMOKE_AUXVIEWS, gameOpts->render.particles.bAuxViews, KEY_W, HTX_SMOKE_AUXVIEWS);
	m.AddCheck ("monitors", TXT_SMOKE_MONITORS, gameOpts->render.particles.bMonitors, KEY_M, HTX_SMOKE_MONITORS);
	m.AddText ("", "");

	do {
		i = m.Menu (NULL, TXT_SMOKE_DETAILS_TITLE, SmokeOptionsCallback, &choice);
		} while (i >= 0);
	GET_VAL (gameOpts->render.particles.bPlayers, "players");
	GET_VAL (gameOpts->render.particles.bRobots, "robots");
	GET_VAL (gameOpts->render.particles.bMissiles, "missiles");
	GET_VAL (gameOpts->render.particles.bDebris, "debris");
#if 1
	GET_VAL (gameOpts->render.particles.bCollisions, "check collisions");
#else
	gameOpts->render.particles.bCollisions = 0;
#endif
	GET_VAL (gameOpts->render.particles.bDisperse, "disperse");
	GET_VAL (gameOpts->render.particles.bRotate, "rotate");
	GET_VAL (gameOpts->render.particles.bDecreaseLag, "reduce lag");
	GET_VAL (gameOpts->render.particles.bAuxViews, "aux views");
	GET_VAL (gameOpts->render.particles.bMonitors, "monitors");
	if (gameOpts->render.particles.bBubbles) {
		GET_VAL (gameOpts->render.particles.bWiggleBubbles, "wiggle");
		GET_VAL (gameOpts->render.particles.bWobbleBubbles, "wobble");
		}
	//GET_VAL (gameOpts->render.particles.bSyncSizes, "sync sizes");
	if (gameOpts->render.particles.bSyncSizes) {
		for (j = 1; j < 4; j++) {
			gameOpts->render.particles.nSize [j] = gameOpts->render.particles.nSize [0];
			gameOpts->render.particles.nDens [j] = gameOpts->render.particles.nDens [0];
			}
		}
	} while (i == -2);
}

#endif //SIMPLE_MENUS

//------------------------------------------------------------------------------
//eof
