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
	int	nDlTimeout;
	int	nAutoDl;
	int	nExpertMode;
	int	nScreenshots;
} miscOpts;

//------------------------------------------------------------------------------

extern int screenShotIntervals [];

int MiscellaneousCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem * m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menu + miscOpts.nScreenshots;
	v = m->m_value;
	if (gameOpts->app.nScreenShotInterval != v) {
		gameOpts->app.nScreenShotInterval = v;
		if (v)
			sprintf (m->m_text, TXT_SCREENSHOTS, screenShotIntervals [v]);
		else
			strcpy (m->m_text, TXT_NO_SCREENSHOTS);
		m->m_bRebuild = 1;
		key = -2;
		return nCurItem;
		}
	m = menu + miscOpts.nExpertMode;
	v = m->m_value;
	if (gameOpts->app.bExpertMode != v) {
		gameOpts->app.bExpertMode = v;
		key = -2;
		return nCurItem;
		}
	if (gameOpts->app.bExpertMode) {
		m = menu + miscOpts.nAutoDl;
		v = m->m_value;
		if (extraGameInfo [0].bAutoDownload != v) {
			extraGameInfo [0].bAutoDownload = v;
			key = -2;
			return nCurItem;
			}
		if (extraGameInfo [0].bAutoDownload) {
			m = menu + miscOpts.nDlTimeout;
			v = m->m_value;
			if (downloadManager.GetTimeoutIndex () != v) {
				v = downloadManager.SetTimeoutIndex (v);
				sprintf (m->m_text, TXT_AUTODL_TO, downloadManager.GetTimeoutSecs ());
				m->m_bRebuild = 1;
				}
			}
		}
	else
		downloadManager.SetTimeoutIndex (15);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void MiscellaneousMenu (void)
{
	CMenu m;
	int	i, choice,
#if 0
			optFastResp, 
#endif
			optHeadlight, optEscort, optUseMacros,	optAutoLevel, optEnableMods,
			optReticle, optMissileView, optGuided, optSmartSearch, optLevelVer, optDemoFmt;
#if UDP_SAFEMODE
	int	optSafeUDP;
#endif
	char  szDlTimeout [50];
	char	szScreenShots [50];

do {
	i = 0;
	m.Destroy ();
	m.Create (20);
	memset (&miscOpts, 0xff, sizeof (miscOpts));
	optReticle = optMissileView = optGuided = optSmartSearch = optLevelVer = optDemoFmt = -1;
	if (gameStates.app.bNostalgia) {
		optAutoLevel = m.AddCheck (TXT_AUTO_LEVEL, gameOpts->gameplay.nAutoLeveling, KEY_L, HTX_MISC_AUTOLEVEL);
		optReticle = m.AddCheck (TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optMissileView = m.AddCheck (TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
		optGuided = m.AddCheck (TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_G, HTX_CPIT_GUIDEDVIEW);
		optHeadlight = m.AddCheck (TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_H, HTX_MISC_HEADLIGHT);
		}
	else
		optHeadlight = 
		optAutoLevel = -1;
	optEscort = m.AddCheck (TXT_ESCORT_KEYS, gameOpts->gameplay.bEscortHotKeys, KEY_K, HTX_MISC_ESCORTKEYS);
#if 0
	optFastResp = m.AddCheck (TXT_FAST_RESPAWN, gameOpts->gameplay.bFastRespawn, KEY_F, HTX_MISC_FASTRESPAWN);
#endif
	optUseMacros = m.AddCheck (TXT_USE_MACROS, gameOpts->multi.bUseMacros, KEY_M, HTX_MISC_USEMACROS);
	if (!(gameStates.app.bNostalgia || gameStates.app.bGameRunning)) {
#if UDP_SAFEMODE
		optSafeUDP = m.AddCheck (TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MISC_UDPQUAL);
#endif
		}
	if (!gameStates.app.bNostalgia) {
		if (gameOpts->app.bExpertMode) {
			optSmartSearch = m.AddCheck (TXT_SMART_SEARCH, gameOpts->menus.bSmartFileSearch, KEY_S, HTX_MISC_SMARTSEARCH);
			optLevelVer = m.AddCheck (TXT_SHOW_LVL_VERSION, gameOpts->menus.bShowLevelVersion, KEY_V, HTX_MISC_SHOWLVLVER);
			optEnableMods = m.AddCheck (TXT_ENABLE_MODS, gameOpts->app.bEnableMods, KEY_O, HTX_ENABLE_MODS);
			}
		else
			optEnableMods = 
			optSmartSearch =
			optLevelVer = -1;
		miscOpts.nExpertMode = m.AddCheck (TXT_EXPERT_MODE, gameOpts->app.bExpertMode, KEY_X, HTX_MISC_EXPMODE);
		optDemoFmt = m.AddCheck (TXT_OLD_DEMO_FORMAT, gameOpts->demo.bOldFormat, KEY_C, HTX_OLD_DEMO_FORMAT);
		}
	if (gameStates.app.bNostalgia < 2) {
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode)
			m.AddText ("", 0);
		miscOpts.nAutoDl = m.AddCheck (TXT_AUTODL_ENABLE, extraGameInfo [0].bAutoDownload, KEY_A, HTX_MISC_AUTODL);
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			sprintf (szDlTimeout + 1, TXT_AUTODL_TO, downloadManager.GetTimeoutSecs ());
			*szDlTimeout = *(TXT_AUTODL_TO - 1);
			miscOpts.nDlTimeout = m.AddSlider (szDlTimeout + 1, downloadManager.GetTimeoutIndex (), 0, downloadManager.MaxTimeoutIndex (), KEY_T, HTX_MISC_AUTODLTO);  
			}
		m.AddText ("", 0);
		if (gameOpts->app.nScreenShotInterval)
			sprintf (szScreenShots + 1, TXT_SCREENSHOTS, screenShotIntervals [gameOpts->app.nScreenShotInterval]);
		else
			strcpy (szScreenShots + 1, TXT_NO_SCREENSHOTS);
		*szScreenShots = *(TXT_SCREENSHOTS - 1);
		miscOpts.nScreenshots = m.AddSlider (szScreenShots + 1, gameOpts->app.nScreenShotInterval, 0, 7, KEY_S, HTX_MISC_SCREENSHOTS);  
		}
	do {
		i = m.Menu (NULL, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISC_TITLE, MiscellaneousCallback, &choice);
	} while (i >= 0);
	if (gameStates.app.bNostalgia) {
		gameOpts->gameplay.nAutoLeveling = m [optAutoLevel].m_value;
		gameOpts->render.cockpit.bReticle = m [optReticle].m_value;
		gameOpts->render.cockpit.bMissileView = m [optMissileView].m_value;
		gameOpts->render.cockpit.bGuidedInMainView = m [optGuided].m_value;
		gameOpts->gameplay.bHeadlightOnWhenPickedUp = m [optHeadlight].m_value;
		}
	gameOpts->gameplay.bEscortHotKeys = m [optEscort].m_value;
	gameOpts->multi.bUseMacros = m [optUseMacros].m_value;
	if (!gameStates.app.bNostalgia) {
		gameOpts->app.bExpertMode = m [miscOpts.nExpertMode].m_value;
		gameOpts->demo.bOldFormat = m [optDemoFmt].m_value;
		if (gameOpts->app.bExpertMode) {
#if UDP_SAFEMODE
			if (!gameStates.app.bGameRunning)
				GET_VAL (extraGameInfo [0].bSafeUDP, optSafeUDP);
#endif
#if 0
			GET_VAL (gameOpts->gameplay.bFastRespawn, optFastResp);
#endif
			GET_VAL (gameOpts->app.bEnableMods, optEnableMods);
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
		extraGameInfo [0].bAutoDownload = m [miscOpts.nAutoDl].m_value;
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
