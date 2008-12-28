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

int DifficultyMenu (void)
{
	int		i, choice = gameStates.app.nDifficultyLevel;
	CMenu		m (5);

for (i = 0; i < 5; i++)
	m.AddMenu ( MENU_DIFFICULTY_TEXT (i), 0, "");
i = m.Menu (NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, &choice);
if (i <= -1)
	return 0;
if (choice != gameStates.app.nDifficultyLevel) {       
	gameStates.app.nDifficultyLevel = choice;
	WritePlayerFile ();
	}
return 1;
}

//------------------------------------------------------------------------------

void MultiThreadingOptionsMenu (void)
{
	CMenuItem	m [10];
	int			h, i, bSound = gameData.app.bUseMultiThreading [rtSound], choice = 0;

	static int	menuToTask [rtTaskCount] = {0, 1, 1, 2, 2, 3, 4, 5};	//map menu entries to tasks
	static int	taskToMenu [6] = {0, 1, 3, 5, 6, 7};	//map tasks to menu entries

memset (m, 0, sizeof (m));
h = gameStates.app.bMultiThreaded ? 6 : 1;
for (i = 0; i < h; i++)
	m.AddCheck (i, GT (1060 + i), gameData.app.bUseMultiThreading [taskToMenu [i]], -1, HT (359 + i));
i = ExecMenu1 (NULL, TXT_MT_MENU_TITLE, h, m, NULL, &choice);
h = gameStates.app.bMultiThreaded ? rtTaskCount : rtSound + 1;
for (i = rtSound; i < h; i++)
	gameData.app.bUseMultiThreading [i] = (m [menuToTask [i]].value != 0);
if (bSound != gameData.app.bUseMultiThreading [rtSound]) {
	if (bSound)
		EndSoundThread ();
	else
		StartSoundThread ();
	}
}

//------------------------------------------------------------------------------

int ConfigMenuCallback (int nitems, CMenuItem * items, int *nLastKey, int nCurItem)
{
if (gameStates.app.bNostalgia) {
	if (nCurItem == optBrightness)
		paletteManager.SetGamma (items [optBrightness].value);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ConfigMenu (void)
{
	CMenuItem	m [20];
	int			i, nOptions, choice = 0;
	int			optSound, optConfig, optJoyCal, optPerformance, optScrRes, optReorderPrim, optReorderSec, 
					optMiscellaneous, optMultiThreading = -1, optRender, optGameplay, optCockpit, optPhysics = -1;

do {
	memset (m, 0, sizeof (m));
	optRender = optGameplay = optCockpit = optPerformance = -1;
	nOptions = 0;
	m.AddMenu (nOptions, TXT_SOUND_MUSIC, KEY_M, HTX_OPTIONS_SOUND);
	optSound = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddMenu (nOptions, TXT_CONTROLS_, KEY_O, HTX_OPTIONS_CONFIG);
	strupr (m [nOptions].text);
	optConfig = nOptions++;
#if defined (_WIN32) || defined (__linux__)
	optJoyCal = -1;
#else
	m.AddMenu (nOptions, TXT_CAL_JOYSTICK, KEY_J, HTX_OPTIONS_CALSTICK);
	optJoyCal = nOptions++;
#endif
	m.AddText (nOptions, "", 0);
	nOptions++;
	if (gameStates.app.bNostalgia) {
		m.AddSlider (nOptions, TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = nOptions++;
		}
	
	if (gameStates.app.bNostalgia)
		m.AddMenu (nOptions, TXT_DETAIL_LEVELS, KEY_D, HTX_OPTIONS_DETAIL);
	else if (gameStates.app.bGameRunning)
		optPerformance = -1;
	else {
		m.AddMenu (nOptions, TXT_SETPERF_OPTION, KEY_E, HTX_PERFORMANCE_SETTINGS);
		optPerformance = nOptions++;
		}
	m.AddMenu (nOptions, TXT_SCREEN_RES, KEY_S, HTX_OPTIONS_SCRRES);
	optScrRes = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddMenu (nOptions, TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
	optReorderPrim = nOptions++;
	m.AddMenu (nOptions, TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
	optReorderSec = nOptions++;
	m.AddMenu (nOptions, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISCELLANEOUS, gameStates.app.bNostalgia ? KEY_T : KEY_I, HTX_OPTIONS_MISC);
	optMiscellaneous = nOptions++;
	if (!gameStates.app.bNostalgia) {
		m.AddMenu (nOptions, TXT_COCKPIT_OPTS2, KEY_C, HTX_OPTIONS_COCKPIT);
		optCockpit = nOptions++;
		m.AddMenu (nOptions, TXT_RENDER_OPTS2, KEY_R, HTX_OPTIONS_RENDER);
		optRender = nOptions++;
		if (gameStates.app.bGameRunning && IsMultiGame && !IsCoopGame) 
			optPhysics =
			optGameplay = -1;
		else {
			m.AddMenu (nOptions, TXT_GAMEPLAY_OPTS2, KEY_G, HTX_OPTIONS_GAMEPLAY);
			optGameplay = nOptions++;
			m.AddMenu (nOptions, TXT_PHYSICS_MENUCALL, KEY_Y, HTX_OPTIONS_PHYSICS);
			optPhysics = nOptions++;
			}
		m.AddMenu (nOptions, TXT_MT_MENU_OPTION, KEY_U, HTX_MULTI_THREADING);
		optMultiThreading = nOptions++;
		}

	i = ExecMenu1 (NULL, TXT_OPTIONS, nOptions, m, ConfigMenuCallback, &choice);
	if (i >= 0) {
		if (i == optSound)
			SoundMenu (void);		
		else if (i == optConfig)
			InputDeviceConfig ();		
		else if (i == optJoyCal)
			JoyDefsCalibrate ();	
		else if (i == optPerformance) {
			if (gameStates.app.bNostalgia)
				DetailLevelMenu (void); 
			else
				PerformanceSettingsMenu (void);
			}
		else if (i == optScrRes)
			ScreenResMenu (void);	
		else if (i == optReorderPrim)
			ReorderPrimary ();		
		else if (i == optReorderSec)
			ReorderSecondary ();	
		else if (i == optMiscellaneous)
			MiscellaneousMenu (void);		
		else if (!gameStates.app.bNostalgia) {
			if (i == optCockpit)
				CockpitOptionsMenu (void);		
			else if (i == optRender)
				RenderOptionsMenu (void);		
			else if ((optGameplay >= 0) && (i == optGameplay))
				GameplayOptionsMenu (void);        
			else if ((optPhysics >= 0) && (i == optPhysics))
				PhysicsOptionsMenu (void);        
			else if ((optMultiThreading >= 0) && (i == optMultiThreading))
				MultiThreadingOptionsMenu (void);        
			}
		}
	} while (i > -1);
WritePlayerFile ();
}

//------------------------------------------------------------------------------
//eof
