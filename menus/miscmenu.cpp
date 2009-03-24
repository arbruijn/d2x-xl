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

if (miscOpts.nScreenshots >= 0) {
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
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void DefaultMiscSettings (void);

void MiscellaneousMenu (void)
{
	static int choice = 0;

	CMenu m;
	int	i;
	int	optHeadlight, optAutoLevel, optEnableMods, optEpileptic, optColorblind, optNotebook,
			optReticle, optMissileView, optGuided, optSmartSearch, optLevelVer, optDemoFmt;
#if UDP_SAFEMODE
	int	optSafeUDP;
#endif
	char  szSlider [50];

do {
	i = 0;
	m.Destroy ();
	m.Create (20);
	memset (&miscOpts, 0xff, sizeof (miscOpts));
	optReticle = optMissileView = optGuided = optSmartSearch = optLevelVer = optDemoFmt = optEpileptic = optColorblind = -1;
	if (gameStates.app.bNostalgia) {
		optAutoLevel = m.AddCheck (TXT_AUTO_LEVEL, gameOpts->gameplay.nAutoLeveling, KEY_L, HTX_MISC_AUTOLEVEL);
		optReticle = m.AddCheck (TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optMissileView = m.AddCheck (TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
		optGuided = m.AddCheck (TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_G, HTX_CPIT_GUIDEDVIEW);
		optHeadlight = m.AddCheck (TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_H, HTX_MISC_HEADLIGHT);
		}
	else {
		optEpileptic = m.AddCheck (TXT_EPILEPTIC_FRIENDLY, gameOpts->app.bEpilepticFriendly, KEY_E, HTX_EPILEPTIC_FRIENDLY);
		optColorblind = m.AddCheck (TXT_COLORBLIND_FRIENDLY, gameOpts->app.bColorblindFriendly, KEY_C, HTX_COLORBLIND_FRIENDLY);
		optNotebook = m.AddCheck (TXT_NOTEBOOK_FRIENDLY, gameOpts->app.bNotebookFriendly, KEY_N, HTX_COLORBLIND_FRIENDLY);
		optEnableMods = m.AddCheck (TXT_ENABLE_MODS, gameOpts->app.bEnableMods, KEY_O, HTX_ENABLE_MODS);
		optHeadlight = 
		optAutoLevel = -1;
		}
	if (gameOpts->app.bExpertMode && (gameStates.app.bNostalgia < 2)) {
		m.AddText ("", 0);
		if (gameOpts->app.nScreenShotInterval)
			sprintf (szSlider + 1, TXT_SCREENSHOTS, screenShotIntervals [gameOpts->app.nScreenShotInterval]);
		else
			strcpy (szSlider + 1, TXT_NO_SCREENSHOTS);
		*szSlider = *(TXT_SCREENSHOTS - 1);
		miscOpts.nScreenshots = m.AddSlider (szSlider + 1, gameOpts->app.nScreenShotInterval, 0, 7, KEY_S, HTX_MISC_SCREENSHOTS);  
		}
	else
		miscOpts.nScreenshots = -1;
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

	if (!gameStates.app.bNostalgia) {
		GET_VAL (gameOpts->app.bEpilepticFriendly, optEpileptic);
		GET_VAL (gameOpts->app.bColorblindFriendly, optColorblind);
		GET_VAL (gameOpts->app.bNotebookFriendly, optNotebook);
		GET_VAL (gameOpts->app.bEnableMods, optEnableMods);
		}
	} while (i == -2);

DefaultMiscSettings ();
}

//------------------------------------------------------------------------------
//eof
