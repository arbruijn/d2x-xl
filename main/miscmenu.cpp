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
	int	nDlTimeout;
	int	nAutoDl;
	int	nExpertMode;
	int	nScreenshots;
} miscOpts;

//------------------------------------------------------------------------------

extern int screenShotIntervals [];

int MiscellaneousCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem * m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + performanceOpts.nUseCompSpeed;
	v = m->value;
	if (gameStates.app.bUseDefaults != v) {
		gameStates.app.bUseDefaults = v;
		*key = -2;
		return nCurItem;
		}
	m = menus + miscOpts.nScreenshots;
	v = m->value;
	if (gameOpts->app.nScreenShotInterval != v) {
		gameOpts->app.nScreenShotInterval = v;
		if (v)
			sprintf (m->text, TXT_SCREENSHOTS, screenShotIntervals [v]);
		else
			strcpy (m->text, TXT_NO_SCREENSHOTS);
		m->rebuild = 1;
		*key = -2;
		return nCurItem;
		}
	m = menus + miscOpts.nExpertMode;
	v = m->value;
	if (gameOpts->app.bExpertMode != v) {
		gameOpts->app.bExpertMode = v;
		*key = -2;
		return nCurItem;
		}
	if (gameOpts->app.bExpertMode) {
		m = menus + miscOpts.nAutoDl;
		v = m->value;
		if (extraGameInfo [0].bAutoDownload != v) {
			extraGameInfo [0].bAutoDownload = v;
			*key = -2;
			return nCurItem;
			}
		if (extraGameInfo [0].bAutoDownload) {
			m = menus + miscOpts.nDlTimeout;
			v = m->value;
			if (GetDlTimeout () != v) {
				v = SetDlTimeout (v);
				sprintf (m->text, TXT_AUTODL_TO, GetDlTimeoutSecs ());
				m->rebuild = 1;
				}
			}
		}
	else
		SetDlTimeout (15);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void MiscellaneousMenu (void)
{
	CMenuItem m [20];
	int	i, nOptions, choice,
#if 0
			optFastResp, 
#endif
			optHeadlight, optEscort, optUseMacros,	optAutoLevel,
			optReticle, optMissileView, optGuided, optSmartSearch, optLevelVer, optDemoFmt;
#if UDP_SAFEMODE
	int	optSafeUDP;
#endif
	char  szDlTimeout [50];
	char	szScreenShots [50];

do {
	i = nOptions = 0;
	memset (m, 0, sizeof (m));
	memset (&miscOpts, 0xff, sizeof (miscOpts));
	optReticle = optMissileView = optGuided = optSmartSearch = optLevelVer = optDemoFmt = -1;
	if (gameStates.app.bNostalgia) {
		m.AddCheck (0, TXT_AUTO_LEVEL, gameOpts->gameplay.nAutoLeveling, KEY_L, HTX_MISC_AUTOLEVEL);
		optAutoLevel = nOptions++;
		m.AddCheck (nOptions, TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optReticle = nOptions++;
		m.AddCheck (nOptions, TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
		optMissileView = nOptions++;
		m.AddCheck (nOptions, TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_G, HTX_CPIT_GUIDEDVIEW);
		optGuided = nOptions++;
		m.AddCheck (nOptions, TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_H, HTX_MISC_HEADLIGHT);
		optHeadlight = nOptions++;
		}
	else
		optHeadlight = 
		optAutoLevel = -1;
	m.AddCheck (nOptions, TXT_ESCORT_KEYS, gameOpts->gameplay.bEscortHotKeys, KEY_K, HTX_MISC_ESCORTKEYS);
	optEscort = nOptions++;
#if 0
	m.AddCheck (nOptions, TXT_FAST_RESPAWN, gameOpts->gameplay.bFastRespawn, KEY_F, HTX_MISC_FASTRESPAWN);
	optFastResp = nOptions++;
#endif
	m.AddCheck (nOptions, TXT_USE_MACROS, gameOpts->multi.bUseMacros, KEY_M, HTX_MISC_USEMACROS);
	optUseMacros = nOptions++;
	if (!(gameStates.app.bNostalgia || gameStates.app.bGameRunning)) {
#if UDP_SAFEMODE
		m.AddCheck (nOptions, TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MISC_UDPQUAL);
		optSafeUDP=nOptions++;
#endif
		}
	if (!gameStates.app.bNostalgia) {
		if (gameOpts->app.bExpertMode) {
			m.AddCheck (nOptions, TXT_SMART_SEARCH, gameOpts->menus.bSmartFileSearch, KEY_S, HTX_MISC_SMARTSEARCH);
			optSmartSearch = nOptions++;
			m.AddCheck (nOptions, TXT_SHOW_LVL_VERSION, gameOpts->menus.bShowLevelVersion, KEY_V, HTX_MISC_SHOWLVLVER);
			optLevelVer = nOptions++;
			}
		else
			optSmartSearch =
			optLevelVer = -1;
		m.AddCheck (nOptions, TXT_EXPERT_MODE, gameOpts->app.bExpertMode, KEY_X, HTX_MISC_EXPMODE);
		miscOpts.nExpertMode = nOptions++;
		m.AddCheck (nOptions, TXT_OLD_DEMO_FORMAT, gameOpts->demo.bOldFormat, KEY_C, HTX_OLD_DEMO_FORMAT);
		optDemoFmt = nOptions++;
		}
	if (gameStates.app.bNostalgia < 2) {
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			m.AddText (nOptions, "", 0);
			nOptions++;
			}
		m.AddCheck (nOptions, TXT_AUTODL_ENABLE, extraGameInfo [0].bAutoDownload, KEY_A, HTX_MISC_AUTODL);
		miscOpts.nAutoDl = nOptions++;
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			sprintf (szDlTimeout + 1, TXT_AUTODL_TO, GetDlTimeoutSecs ());
			*szDlTimeout = *(TXT_AUTODL_TO - 1);
			m.AddSlider (nOptions, szDlTimeout + 1, GetDlTimeout (), 0, MaxDlTimeout (), KEY_T, HTX_MISC_AUTODLTO);  
			miscOpts.nDlTimeout = nOptions++;
			}
		m.AddText (nOptions, "", 0);
		nOptions++;
		if (gameOpts->app.nScreenShotInterval)
			sprintf (szScreenShots + 1, TXT_SCREENSHOTS, screenShotIntervals [gameOpts->app.nScreenShotInterval]);
		else
			strcpy (szScreenShots + 1, TXT_NO_SCREENSHOTS);
		*szScreenShots = *(TXT_SCREENSHOTS - 1);
		m.AddSlider (nOptions, szScreenShots + 1, gameOpts->app.nScreenShotInterval, 0, 7, KEY_S, HTX_MISC_SCREENSHOTS);  
		miscOpts.nScreenshots = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISC_TITLE, nOptions, m, MiscellaneousCallback, &choice);
	} while (i >= 0);
	if (gameStates.app.bNostalgia) {
		gameOpts->gameplay.nAutoLeveling = m [optAutoLevel].value;
		gameOpts->render.cockpit.bReticle = m [optReticle].value;
		gameOpts->render.cockpit.bMissileView = m [optMissileView].value;
		gameOpts->render.cockpit.bGuidedInMainView = m [optGuided].value;
		gameOpts->gameplay.bHeadlightOnWhenPickedUp = m [optHeadlight].value;
		}
	gameOpts->gameplay.bEscortHotKeys = m [optEscort].value;
	gameOpts->multi.bUseMacros = m [optUseMacros].value;
	if (!gameStates.app.bNostalgia) {
		gameOpts->app.bExpertMode = m [miscOpts.nExpertMode].value;
		gameOpts->demo.bOldFormat = m [optDemoFmt].value;
		if (gameOpts->app.bExpertMode) {
#if UDP_SAFEMODE
			if (!gameStates.app.bGameRunning)
				GET_VAL (extraGameInfo [0].bSafeUDP, optSafeUDP);
#endif
#if 0
			GET_VAL (gameOpts->gameplay.bFastRespawn, optFastResp);
#endif
			GET_VAL (gameOpts->menus.bSmartFileSearch, optSmartSearch);
			GET_VAL (gameOpts->menus.bShowLevelVersion, optLevelVer);
			}
		else {
#if EXPMODE_DEFAULTS
			extraGameInfo [0].bWiggle = 1;
#if 0
			gameOpts->gameplay.bFastRespawn = 0;
#endif
			gameOpts->menus.bSmartFileSearch = 1;
			gameOpts->menus.bShowLevelVersion = 1;
#endif
			}
		}
	if (gameStates.app.bNostalgia > 1)
		extraGameInfo [0].bAutoDownload = 0;
	else
		extraGameInfo [0].bAutoDownload = m [miscOpts.nAutoDl].value;
	} while (i == -2);
}

//------------------------------------------------------------------------------

int QuitSaveLoadMenu (void)
{
	CMenuItem m [5];
	int	i, choice = 0, nOptions, optQuit, optOptions, optLoad, optSave;

memset (m, 0, sizeof (m));
nOptions = 0;
m.AddMenu (nOptions, TXT_QUIT_GAME, KEY_Q, HTX_QUIT_GAME);
optQuit = nOptions++;
m.AddMenu (nOptions, TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
optOptions = nOptions++;
m.AddMenu (nOptions, TXT_LOAD_GAME2, KEY_L, HTX_LOAD_GAME);
optLoad = nOptions++;
m.AddMenu (nOptions, TXT_SAVE_GAME2, KEY_S, HTX_SAVE_GAME);
optSave = nOptions++;
i = ExecMenu1 (NULL, TXT_ABORT_GAME, nOptions, m, NULL, &choice);
if (!i)
	return 0;
if (i == optOptions)
	ConfigMenu (void);
else if (i == optLoad)
	saveGameHandler.Load (1, 0, 0, NULL);
else if (i == optSave)
	saveGameHandler.Save (0, 0, 0, NULL);
return 1;
}

//------------------------------------------------------------------------------
//eof
