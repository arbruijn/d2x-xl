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
	ControlTranspRenderThread ();
	ControlEffectsThread ();
	}
}

#endif

//------------------------------------------------------------------------------

static int optBrightness = -1;

int ConfigMenuCallback (CMenu& m, int& nLastKey, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (gameStates.app.bNostalgia) {
	if (nCurItem == optBrightness)
		paletteManager.SetGamma (m [optBrightness].m_value);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ConfigMenu (void)
{
	static int choice = 0;

	CMenu	menu;
	int	i;
	int	optSound, optConfig, optPerformance, optScrRes, optReorderPrim, optReorderSec, optEffects,
			optMiscellaneous, optMultiThreading = -1, optRender, optGameplay, optCockpit, optPhysics = -1;

do {
	menu.Destroy ();
	menu.Create (20);
	optRender = optGameplay = optCockpit = -1;
	optSound = menu.AddMenu (TXT_SOUND_MUSIC, KEY_M, HTX_OPTIONS_SOUND);
	menu.AddText ("", 0);
	optConfig = menu.AddMenu (TXT_CONTROLS_, KEY_O, HTX_OPTIONS_CONFIG);
	menu.AddText ("", 0);
	if (gameStates.app.bNostalgia)
		optBrightness = menu.AddSlider (TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
	
	if (gameStates.app.bNostalgia)
		optPerformance = menu.AddMenu (TXT_DETAIL_LEVELS, KEY_D, HTX_OPTIONS_DETAIL);
	else 
		optPerformance = -1;
	optScrRes = menu.AddMenu (TXT_SCREEN_RES, KEY_S, HTX_OPTIONS_SCRRES);
	menu.AddText ("", 0);
	if (gameStates.app.bNostalgia) {
		optReorderPrim = menu.AddMenu (TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
		optReorderSec = menu.AddMenu (TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
		}
	else
		optReorderPrim = 
		optReorderSec = -1;
	optMiscellaneous = menu.AddMenu (gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISCELLANEOUS, gameStates.app.bNostalgia ? KEY_T : KEY_I, HTX_OPTIONS_MISC);
	if (!gameStates.app.bNostalgia) {
		optCockpit = menu.AddMenu (TXT_COCKPIT_OPTS2, KEY_C, HTX_OPTIONS_COCKPIT);
		optRender = menu.AddMenu (TXT_RENDER_OPTS2, KEY_R, HTX_OPTIONS_RENDER);
		optEffects = menu.AddMenu (TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
		if (gameStates.app.bGameRunning && IsMultiGame && !IsCoopGame) 
			optPhysics =
			optGameplay = -1;
		else {
			optGameplay = menu.AddMenu (TXT_GAMEPLAY_OPTS2, KEY_G, HTX_OPTIONS_GAMEPLAY);
			optPhysics = menu.AddMenu (TXT_PHYSICS_MENUCALL, KEY_P, HTX_OPTIONS_PHYSICS);
			}
#if 0
		optMultiThreading = menu.AddMenu (TXT_MT_MENU_OPTION, KEY_U, HTX_MULTI_THREADING);
#endif
		}

	i = menu.Menu (NULL, TXT_OPTIONS, ConfigMenuCallback, &choice);
	if (i >= 0) {
		if (i == optSound)
			SoundMenu ();		
		else if (i == optConfig)
			InputDeviceConfig ();		
		else if (i == optPerformance) {
			if (gameStates.app.bNostalgia)
				DetailLevelMenu (); 
			}
		else if (i == optScrRes)
			ScreenResMenu ();	
		else if ((optReorderPrim >= 0) && (i == optReorderPrim))
			ReorderPrimary ();		
		else if ((optReorderSec >= 0) && (i == optReorderSec))
			ReorderSecondary ();	
		else if (i == optMiscellaneous)
			MiscellaneousMenu ();		
		else if (!gameStates.app.bNostalgia) {
			if (i == optCockpit)
				CockpitOptionsMenu ();		
			else if (i == optRender)
				RenderOptionsMenu ();		
			else if (i == optEffects)
				EffectOptionsMenu ();
			else if ((optGameplay >= 0) && (i == optGameplay))
				GameplayOptionsMenu ();        
			else if ((optPhysics >= 0) && (i == optPhysics))
				PhysicsOptionsMenu ();        
#if 0
			else if ((optMultiThreading >= 0) && (i == optMultiThreading))
				MultiThreadingOptionsMenu ();        
#endif
			}
		}
	} while (i > -1);
SavePlayerProfile ();
}

//------------------------------------------------------------------------------
//eof
