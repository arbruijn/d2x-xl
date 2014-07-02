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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
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
	int32_t	nDlTimeout;
	int32_t	nAutoDl;
	int32_t	nExpertMode;
	int32_t	nScreenshots;
} miscOpts;

static const char* pszExpertMode [3];

//------------------------------------------------------------------------------

extern int32_t screenShotIntervals [];

int32_t MiscellaneousCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem * m;
	int32_t			v;

if ((m = menu ["expert mode"])) {
	v = m->Value ();
	if (gameOpts->app.bExpertMode != v) {
		gameOpts->app.bExpertMode = v;
		sprintf (m->m_text, TXT_EXPERT_MODE, pszExpertMode [v]);
		m->m_bRebuild = 1;
		key = -2;
		return nCurItem;
		}
	}

if ((m = menu ["screenshots"])) {
	v = m->Value ();
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

if ((m = menu ["auto download"])) {
	int32_t v = m->Value ();
	if (extraGameInfo [0].bAutoDownload != v) {
		extraGameInfo [0].bAutoDownload = v;
		key = -2;
		return nCurItem;
		}

	if (extraGameInfo [0].bAutoDownload && (m = menu ["download timeout"])) {
		v = m->Value ();
		if (downloadManager.GetTimeoutIndex () != v) {
			v = downloadManager.SetTimeoutIndex (v);
			sprintf (m->m_text, TXT_DL_TIMEOUT, downloadManager.GetTimeoutSecs ());
			m->m_bRebuild = 1;
			}
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

pszExpertMode [0] = TXT_OFF;
pszExpertMode [1] = TXT_BASIC;
pszExpertMode [2] = TXT_FULL;
}

//------------------------------------------------------------------------------

void DefaultMiscSettings (bool bSetup = false);

void MiscellaneousMenu (void)
{
	static int32_t choice = 0;

	CMenu m;
	int32_t	i;

InitStrings ();

if (gameOpts->app.bExpertMode == SUPERUSER)
	gameOpts->app.bExpertMode = 2;
else if (gameOpts->app.bExpertMode)
	gameOpts->app.bExpertMode = 1;

#if UDP_SAFEMODE
	int32_t	optSafeUDP;
#endif
	char  szSlider [50];

do {
	i = 0;
	m.Destroy ();
	m.Create (20);
	memset (&miscOpts, 0xff, sizeof (miscOpts));
	sprintf (szSlider + 1, TXT_EXPERT_MODE, pszExpertMode [gameOpts->app.bExpertMode]);
	*szSlider = *(TXT_EXPERT_MODE - 1);
	m.AddSlider ("expert mode", szSlider + 1, gameOpts->app.bExpertMode, 0, sizeofa (pszExpertMode) - 1, KEY_X, HTX_EXPERT_MODE);  
	m.AddText ("", "");
	if (gameStates.app.bNostalgia) {
		m.AddCheck ("auto leveling", TXT_AUTO_LEVEL, gameOpts->gameplay.nAutoLeveling, KEY_L, HTX_MISC_AUTOLEVEL);
		m.AddCheck ("show reticle", TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		m.AddCheck ("missile view", TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPIT_MSLVIEW);
		m.AddCheck ("guided in mainview", TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_G, HTX_CPIT_GUIDEDVIEW);
		m.AddCheck ("headlight on", TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_H, HTX_MISC_HEADLIGHT);
		}
	else {
		m.AddCheck ("epileptic friendly", TXT_EPILEPTIC_FRIENDLY, gameOpts->app.bEpilepticFriendly, KEY_E, HTX_EPILEPTIC_FRIENDLY);
		m.AddCheck ("colorblind friendly", TXT_COLORBLIND_FRIENDLY, gameOpts->app.bColorblindFriendly, KEY_C, HTX_COLORBLIND_FRIENDLY);
		m.AddCheck ("notebook friendly", TXT_NOTEBOOK_FRIENDLY, gameOpts->app.bNotebookFriendly, KEY_N, HTX_COLORBLIND_FRIENDLY);
		}

	if ((gameStates.app.bNostalgia < 2) && gameOpts->app.bExpertMode) {
		m.AddText ("", "");
		if (gameOpts->app.nScreenShotInterval)
			sprintf (szSlider + 1, TXT_SCREENSHOTS, screenShotIntervals [gameOpts->app.nScreenShotInterval]);
		else
			strcpy (szSlider + 1, TXT_NO_SCREENSHOTS);
		*szSlider = *(TXT_SCREENSHOTS - 1);
		m.AddSlider ("screenshots", szSlider + 1, gameOpts->app.nScreenShotInterval, 0, 7, KEY_S, HTX_MISC_SCREENSHOTS);  

		if (gameStates.app.bHaveSDLNet) {
			m.AddText ("", "");
			m.AddCheck ("auto download", TXT_DL_ENABLE, extraGameInfo [0].bAutoDownload, KEY_M, HTX_MISC_MISSIONDL);
			if (extraGameInfo [0].bAutoDownload) {
				sprintf (szSlider + 1, TXT_DL_TIMEOUT, downloadManager.GetTimeoutSecs ());
				*szSlider = *(TXT_DL_TIMEOUT - 1);
				m.AddSlider ("download timeout", szSlider + 1, downloadManager.GetTimeoutIndex (), 0, downloadManager.MaxTimeoutIndex (), KEY_T, HTX_MISC_DLTIMEOUT);
				}
			}
		}

	do {
		i = m.Menu (NULL, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISC_TITLE, MiscellaneousCallback, &choice);
	} while (i >= 0);

	if (gameStates.app.bNostalgia) {
		gameOpts->gameplay.nAutoLeveling = m.Value ("auto leveling");
		gameOpts->render.cockpit.bReticle = m.Value ("show reticle");
		gameOpts->render.cockpit.bMissileView = m.Value ("missile view");
		gameOpts->render.cockpit.bGuidedInMainView = m.Value ("guided in mainview");
		gameOpts->gameplay.bHeadlightOnWhenPickedUp = m.Value ("headlight on");
		}

	if (!gameStates.app.bNostalgia) {
		GET_VAL (gameOpts->app.bEpilepticFriendly, "epileptic friendly");
		GET_VAL (gameOpts->app.bColorblindFriendly, "colorblind friendly");
		GET_VAL (gameOpts->app.bNotebookFriendly, "notebook friendly");
		}
	} while (i == -2);

if (gameOpts->app.bExpertMode == 2)
	gameOpts->app.bExpertMode = SUPERUSER;
else if (gameOpts->app.bExpertMode)
	gameOpts->app.bExpertMode = 1;

DefaultMiscSettings ();
}

//------------------------------------------------------------------------------
//eof
