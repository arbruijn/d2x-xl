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

#if 0

void MultiThreadingOptionsMenu (void)
{
	CMenu	m (10);
	int	h, i, bSound = gameData.app.bUseMultiThreading [rtSound], choice = 0;

	static int	menuToTask [rtTaskCount] = {0, 1, 1, 2, 2, 3, 4, 5};	//map menu entries to tasks
	static int	taskToMenu [6] = {0, 1, 3, 5, 6, 7};	//map tasks to menu entries

h = gameStates.app.bMultiThreaded ? 6 : 1;
for (i = 0; i < h; i++)
	m.AddCheck (GT (1060 + i), gameData.app.bUseMultiThreading [taskToMenu [i]], -1, HT (359 + i));
i = m.Menu (NULL, TXT_MT_MENU_TITLE, NULL, &choice);
h = gameStates.app.bMultiThreaded ? rtTaskCount : rtSound + 1;
for (i = rtSound; i < h; i++)
	gameData.app.bUseMultiThreading [i] = (m [menuToTask [i]].m_value != 0);
if (gameStates.app.bGameRunning) {
	ControlRenderThreads ();
	ControlSoundThread ();
	ControlEffectsThread ();
	}
}

#endif

//------------------------------------------------------------------------------

static int optBrightness = -1;

int ConfigMenuCallback (CMenu& menu, int& nLastKey, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (gameStates.app.bNostalgia) {
	CMenuItem* m = menu ["brightness"];
	int v = m->Value ();
	if ((nCurItem == menu.IndexOf ("brightness")) && (v != paletteManager.GetGamma ())) {
		paletteManager.SetGamma (v);
		sprintf (m->m_text, TXT_BRIGHTNESS, paletteManager.BrightnessLevel ());
		m->Rebuild ();
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ConfigMenu (void)
{
	static int choice = 0;

	CMenu	m;
	int	i;
	char	szSlider [50];


do {
	m.Destroy ();
	m.Create (20);
	m.AddMenu ("sound options", TXT_SOUND_MUSIC, KEY_M, HTX_OPTIONS_SOUND);
	m.AddText ("", 0);
	m.AddMenu ("config options", TXT_CONTROLS_, KEY_O, HTX_OPTIONS_CONFIG);
	m.AddText ("", 0);
	if (gameStates.app.bNostalgia) {
		sprintf (szSlider + 1, TXT_BRIGHTNESS, paletteManager.BrightnessLevel ());
		*szSlider = *(TXT_BRIGHTNESS - 1);
		m.AddSlider ("brightness", szSlider + 1, paletteManager.GetGamma (), 0, 15, KEY_B, HTX_RENDER_BRIGHTNESS);
		}

	if (gameStates.app.bNostalgia)
		m.AddMenu ("performance options", TXT_DETAIL_LEVELS, KEY_D, HTX_OPTIONS_DETAIL);
	m.AddMenu ("screen res options", TXT_SCREEN_RES, KEY_S, HTX_OPTIONS_SCRRES);
	m.AddText ("", 0);
	if (gameStates.app.bNostalgia) {
		m.AddMenu ("reorder primaries", TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
		m.AddMenu ("reorder secondaries", TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
		}
	m.AddMenu ("miscellaneous", gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISCELLANEOUS, gameStates.app.bNostalgia ? KEY_T : KEY_I, HTX_OPTIONS_MISC);
	if (!gameStates.app.bNostalgia) {
		m.AddMenu ("cockpit options", TXT_COCKPIT_OPTS2, KEY_C, HTX_OPTIONS_COCKPIT);
		m.AddMenu ("render options", TXT_RENDER_OPTS2, KEY_R, HTX_OPTIONS_RENDER);
		m.AddMenu ("effect options", TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
#ifndef DBG
		if (!(gameStates.app.bGameRunning && IsMultiGame && !IsCoopGame))
#endif
			{
			m.AddMenu ("gameplay options", TXT_GAMEPLAY_OPTS2, KEY_G, HTX_OPTIONS_GAMEPLAY);
			m.AddMenu ("physics options", TXT_PHYSICS_MENUCALL, KEY_P, HTX_OPTIONS_PHYSICS);
			}
#if 0
		m.AddMenu ("multithreading", TXT_MT_MENU_OPTION, KEY_U, HTX_MULTI_THREADING);
#endif
		}

	i = m.Menu (NULL, TXT_OPTIONS, ConfigMenuCallback, &choice);
	if (i >= 0) {
		if (i == m.IndexOf ("sound options"))
			SoundMenu ();
		else if (i == m.IndexOf ("config options"))
			InputDeviceConfig ();
		else if (i == m.IndexOf ("performance options")) {
			if (gameStates.app.bNostalgia)
				DetailLevelMenu ();
			}
		else if (i == m.IndexOf ("screen res options"))
			ScreenResMenu ();
		else if (i == m.IndexOf ("reorder primaries"))
			ReorderPrimary ();
		else if (i == m.IndexOf ("reorder secondaries"))
			ReorderSecondary ();
		else if (i == m.IndexOf ("miscellaneous"))
			MiscellaneousMenu ();
		else if (!gameStates.app.bNostalgia) {
			if (i == m.IndexOf ("cockpit options"))
				CockpitOptionsMenu ();
			else if (i == m.IndexOf ("render options"))
				RenderOptionsMenu ();
			else if (i == m.IndexOf ("effect options"))
				EffectOptionsMenu ();
			else if (i == m.IndexOf ("gameplay options"))
				GameplayOptionsMenu ();
			else if (i == m.IndexOf ("physics options"))
				PhysicsOptionsMenu ();
#if 0
			else if (i == m.IndexOf ("multiThreading"))
				MultiThreadingOptionsMenu ();
#endif
			}
		}
	} while (i > -1);
SavePlayerProfile ();
}

//------------------------------------------------------------------------------
//eof
