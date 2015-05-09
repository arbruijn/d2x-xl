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

//#define DOORDBGGING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "key.h"
#include "menu.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "timer.h"
#include "rendermine.h"
#include "transprender.h"
#include "screens.h"
#include "slew.h"
#include "cockpit.h"
#include "texmap.h"
#include "segmath.h"
#include "u_mem.h"
#include "light.h"
#include "newdemo.h"
#include "automap.h"
#include "text.h"
#include "renderframe.h"
#include "network_lib.h"
#include "gamefont.h"
#include "gamepal.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "briefings.h"
#include "gamecntl.h"
#include "systemkeys.h"
#include "savegame.h"
#include "escort.h"
#include "cheats.h"
#include "input.h"
#include "marker.h"
#include "songs.h"
#include "hudicons.h"
#include "renderlib.h"
#include "network_lib.h"

// Global Variables -----------------------------------------------------------

int32_t	redbookVolume = 255;


//	External Variables ---------------------------------------------------------

extern fix	showViewTextTimer;

//	Function prototypes --------------------------------------------------------

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL)

//------------------------------------------------------------------------------
// Control Functions

fix newdemo_single_frameTime;

void UpdateVCRState(void)
{
if (gameOpts->demo.bRevertFormat && (gameData.demo.nVersion > DEMO_VERSION))
	return;
if ((gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT]) && gameStates.input.keys.pressed [KEY_RIGHT])
	gameData.demo.nVcrState = ND_STATE_FASTFORWARD;
else if ((gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT]) && gameStates.input.keys.pressed [KEY_LEFT])
	gameData.demo.nVcrState = ND_STATE_REWINDING;
else if (!(gameStates.input.keys.pressed [KEY_LCTRL] || gameStates.input.keys.pressed [KEY_RCTRL]) && gameStates.input.keys.pressed [KEY_RIGHT] && ((TimerGetFixedSeconds () - newdemo_single_frameTime) >= I2X (1)))
	gameData.demo.nVcrState = ND_STATE_ONEFRAMEFORWARD;
else if (!(gameStates.input.keys.pressed [KEY_LCTRL] || gameStates.input.keys.pressed [KEY_RCTRL]) && gameStates.input.keys.pressed [KEY_LEFT] && ((TimerGetFixedSeconds () - newdemo_single_frameTime) >= I2X (1)))
	gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
#if 0
else if ((gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || (gameData.demo.nVcrState == ND_STATE_REWINDING))
	gameData.demo.nVcrState = ND_STATE_PLAYBACK;
#endif
}

//------------------------------------------------------------------------------

char *Pause_msg;

//------------------------------------------------------------------------------

void HandleEndlevelKey (int32_t key)
{
if (key == (KEY_COMMAND + KEY_SHIFTED + KEY_P))
	SaveScreenShot (NULL, 0);

if (key==KEY_PRINT_SCREEN)
	SaveScreenShot (NULL, 0);

if (key == (KEY_COMMAND + KEY_P))
	key = DoGamePause ();

if (key == KEY_PAUSE)
	key = DoGamePause ();		//so esc from pause will end level

if (key == KEY_ESC) {
	StopEndLevelSequence ();
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	return;
	}
#if DBG
	if (key == KEY_BACKSPACE)
		Int3 ();
#endif
}

//------------------------------------------------------------------------------

void HandleDeathKey (int32_t key)
{
/*
	Commented out redundant calls because the key used here typically
	will be passed to HandleSystemKey later.  Note that I do this to pause
	which is supposed to pass the ESC key to leave the level.  This
	doesn't work in the DOS version anyway.   -Samir
*/

if (LOCALPLAYER.m_bExploded && !key_isfunc (key) && !key_ismod (key))
	gameStates.app.bDeathSequenceAborted = 1;		//Any key but func or modifier aborts

if (key == KEY_COMMAND + KEY_SHIFTED + KEY_P) {
	gameStates.app.bDeathSequenceAborted = 0;		// Clear because code above sets this for any key.
	}
else if (key == KEY_PRINT_SCREEN) {
	gameStates.app.bDeathSequenceAborted = 0;		// Clear because code above sets this for any key.
	}
else if (key == (KEY_COMMAND + KEY_P)) {
	gameStates.app.bDeathSequenceAborted = 0;		// Clear because code above sets this for any key.
	}
else if (key == KEY_PAUSE)   {
	gameStates.app.bDeathSequenceAborted = 0;		// Clear because code above sets this for any key.
	}
else if (key == KEY_ESC) {
	if (gameData.objData.pConsole->info.nFlags & OF_EXPLODING)
		gameStates.app.bDeathSequenceAborted = 1;
	}
else if (key == KEY_BACKSPACE)  {
	gameStates.app.bDeathSequenceAborted = 0;		// Clear because code above sets this for any key.
	Int3 ();
	}
//don't abort death sequence for netgame join/refuse keys
else if ((key == KEY_ALTED + KEY_1) || (key == KEY_ALTED + KEY_2))
	gameStates.app.bDeathSequenceAborted = 0;
if (gameStates.app.bDeathSequenceAborted)
	GameFlushInputs ();
}

//------------------------------------------------------------------------------

void HandleDemoKey (int32_t key)
{
if ((gameOpts->demo.bRevertFormat && (gameData.demo.nVersion > DEMO_VERSION)) || OBSERVING)
	return;
switch (key) {
	case KEY_F3:
		 if (!(GuidedInMainView () || gameStates.render.bChaseCam || (gameStates.render.bFreeCam > 0) || gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead))
			cockpit->Toggle ();
		 break;

	case KEY_SHIFTED + KEY_MINUS:
	case KEY_MINUS:
		if (gameData.render.rift.m_ipd > RIFT_MIN_IPD)
			gameData.render.rift.m_ipd--;
		break;

	case KEY_SHIFTED + KEY_EQUALS:
	case KEY_EQUALS:
		if (gameData.render.rift.m_ipd < RIFT_MAX_IPD)
			gameData.render.rift.m_ipd++;
		break;

	case KEY_F2:
		gameStates.app.bConfigMenu = 1;
		break;

	case KEY_F7:
		gameData.multigame.score.bShowList = (gameData.multigame.score.bShowList+1) % ((gameData.demo.nGameMode & GM_TEAM) ? 4 : 3);
		break;

	case KEY_CTRLED + KEY_F7:
		if ((gameStates.render.cockpit.bShowPingStats = !gameStates.render.cockpit.bShowPingStats))
			ResetPingStats ();
		break;

	case KEY_ESC:
		SetFunctionMode (FMODE_MENU);
		break;

	case KEY_UP:
		gameData.demo.nVcrState = ND_STATE_PLAYBACK;
		break;

	case KEY_DOWN:
		gameData.demo.nVcrState = ND_STATE_PAUSED;
		break;

	case KEY_LEFT:
		newdemo_single_frameTime = TimerGetFixedSeconds ();
		gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
		break;

	case KEY_RIGHT:
		newdemo_single_frameTime = TimerGetFixedSeconds ();
		gameData.demo.nVcrState = ND_STATE_ONEFRAMEFORWARD;
		break;

	case KEY_CTRLED + KEY_RIGHT:
		NDGotoEnd ();
		break;

	case KEY_CTRLED + KEY_LEFT:
		NDGotoBeginning ();
		break;

	case KEY_COMMAND + KEY_P:
	case KEY_CTRLED + KEY_P:
	case KEY_PAUSE:
		DoGamePause ();
		break;

	case KEY_COMMAND + KEY_SHIFTED + KEY_P:
	case KEY_PRINT_SCREEN: {
		int32_t oldState = gameData.demo.nVcrState;
		gameData.demo.nVcrState = ND_STATE_PRINTSCREEN;
		//RenderMonoFrame ();
		gameStates.app.bSaveScreenShot = 1;
		//SaveScreenShot (NULL, 0);
		gameData.demo.nVcrState = oldState;
		break;
		}
	}
}

//------------------------------------------------------------------------------

int32_t HandleDisplayKey (int32_t key)
{
switch (key) {
	case KEY_CTRLED + KEY_F1:
		SwitchDisplayMode (-1);
		return 1;
		break;
	case KEY_CTRLED + KEY_F2:
		SwitchDisplayMode (1);
		return 1;
		break;

	case KEY_ALTED + KEY_ENTER:
	case KEY_ALTED + KEY_PADENTER:
		GrToggleFullScreenGame ();
		return 1;
		break;
	}
return 0;
}

//------------------------------------------------------------------------------
//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
int32_t HandleSystemKey (int32_t key)
{
	int32_t bScreenChanged = 0;
	int32_t bStopPlayerMovement = 1;

	//if (gameStates.gameplay.bSpeedBoost)
	//	return 0;

if (!gameStates.app.bPlayerIsDead)
	switch (key) {
		case KEY_ESC:
			if (gameData.app.bGamePaused)
				gameData.app.bGamePaused = 0;
			else {
				gameStates.app.bGameAborted = 1;
				SetFunctionMode (FMODE_MENU);
			}
			break;

		case KEY_SPACEBAR:
			SwitchObservedPlayer ();
			break;

		case KEY_SHIFTED + KEY_F1:
			bScreenChanged = SelectNextWindowFunction (0);
			break;
		case KEY_SHIFTED + KEY_F2:
			bScreenChanged = SelectNextWindowFunction (1);
			break;
		case KEY_SHIFTED + KEY_F3:
			gameOpts->render.cockpit.nWindowSize = (gameOpts->render.cockpit.nWindowSize + 1) % 4;
			bScreenChanged = 1; //SelectNextWindowFunction(1);
			break;
		case KEY_CTRLED + KEY_F3:
			gameOpts->render.cockpit.nWindowPos = (gameOpts->render.cockpit.nWindowPos + 1) % 6;
			bScreenChanged = 1; //SelectNextWindowFunction(1);
			break;
		case KEY_SHIFTED + KEY_CTRLED + KEY_F3:
			gameOpts->render.cockpit.nWindowZoom = (gameOpts->render.cockpit.nWindowZoom + 1) % 4;
			bScreenChanged = 1; //SelectNextWindowFunction(1);
			break;
		}

if (!gameStates.app.bPlayerIsDead || (LOCALPLAYER.lives > 1)) {
	switch (key) {

		case KEY_SHIFTED + KEY_ESC:
			console.Show ();
			break;

		//case KEY_COMMAND + KEY_P:
		//case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
			DoGamePause ();
			break;

		case KEY_CTRLED + KEY_ALTED + KEY_S:
		case KEY_ALTED + KEY_F11:
			if (!ToggleFreeCam ())
				return 0;
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_PRINT_SCREEN:
			gameStates.app.bSaveScreenShot = 1;
			//SaveScreenShot (NULL, 0);
			break;

		case KEY_F1:
			ShowHelp ();
			break;

		case KEY_F2:					//gameStates.app.bConfigMenu = 1; break;
			//paletteManager.SuspendEffect (!IsMultiGame);
			ConfigMenu ();
			//paletteManager.ResumeEffect (!IsMultiGame);
			break;

		case KEY_F3:
			if (!(GuidedInMainView () || gameStates.render.bChaseCam || (gameStates.render.bFreeCam > 0) || gameStates.app.bEndLevelSequence)) {
				SetFreeCam (0);
				SetChaseCam (0);
				cockpit->Toggle ();
				bScreenChanged = 1;
				}
			break;

		case KEY_F7 + KEY_SHIFTED:
			//paletteManager.SaveEffect ();
			JoyDefsCalibrate ();
			//paletteManager.ResumeEffect ();
			break;

		case KEY_SHIFTED + KEY_MINUS:
		case KEY_MINUS:
			if (gameData.render.rift.m_ipd > RIFT_MIN_IPD)
				gameData.render.rift.m_ipd--;
			bScreenChanged = 1;
			break;

		case KEY_SHIFTED + KEY_EQUALS:
		case KEY_EQUALS:
			if (gameData.render.rift.m_ipd < RIFT_MAX_IPD)
				gameData.render.rift.m_ipd++;
			bScreenChanged = 1;
			break;

		case KEY_CTRLED + KEY_F5:
			saveGameManager.Save (0, 0, 1, 0);
			break;

		case KEY_CTRLED + KEY_F9:
			saveGameManager.Load (0, 0, 1, 0);
			break;

		case KEY_F5:
			if (gameData.demo.nState == ND_STATE_RECORDING) {
				NDStopRecording ();
				}
			else if (gameData.demo.nState == ND_STATE_NORMAL)
				if (!gameData.app.bGamePaused)		//can't start demo while paused
					NDStartRecording ();
			break;

		case KEY_CTRLED + KEY_F4:
			gameData.multigame.bShowReticleName = (gameData.multigame.bShowReticleName + 1) % 2;

		case KEY_F7:
			gameData.multigame.score.bShowList = (gameData.multigame.score.bShowList + 1) % (IsTeamGame ? 4 : 3);
			if (IsMultiGame)
				MultiSortKillList ();
			bStopPlayerMovement = 0;
			break;

		case KEY_CTRLED + KEY_F7:
			if ((gameStates.render.cockpit.bShowPingStats = !gameStates.render.cockpit.bShowPingStats))
				ResetPingStats ();
			break;

		case KEY_CTRLED + KEY_F8:
			gameData.stats.nDisplayMode = (gameData.stats.nDisplayMode + 1) % 5;
			gameOpts->render.cockpit.bPlayerStats = gameData.stats.nDisplayMode != 0;
			break;

		case KEY_F8:
			MultiSendMsgStart (-1);
			bStopPlayerMovement = 0;
			break;

		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			MultiSendMacro (key);
			bStopPlayerMovement = 0;
			break;		// send taunt macros

#if DBG
		case KEY_CTRLED + KEY_F10: // VS debugger intercepts F12
#else
		case KEY_CTRLED + KEY_F12:
#endif
			gameData.trackIR.x =
			gameData.trackIR.y = 0;
			gameData.render.rift.SetCenter ();
			break;

		case KEY_ALTED + KEY_F12:
			if (!ToggleChaseCam ())
				return 0;
			break;

		case KEY_SHIFTED + KEY_F9:
		case KEY_SHIFTED + KEY_F10:
		case KEY_SHIFTED + KEY_F11:
		case KEY_SHIFTED + KEY_F12:
			MultiDefineMacro (key);
			bStopPlayerMovement = 0;
			break;		// redefine taunt macros

		case KEY_ALTED + KEY_F2:
			if (!gameStates.app.bPlayerIsDead && !(IsMultiGame && !IsCoopGame)) {
				paletteManager.DisableEffect ();
				saveGameManager.Save (0, 0, 0, NULL);
				paletteManager.EnableEffect ();
			}
			break;  // 0 means not between levels.

		case KEY_ALTED + KEY_F3:
			if (!gameStates.app.bPlayerIsDead && (!IsMultiGame || IsCoopGame)) {
				paletteManager.SuspendEffect ();
				int32_t bLoaded = saveGameManager.Load (1, 0, 0, NULL);
				if (gameData.app.bGamePaused)
					DoGamePause ();
				paletteManager.ResumeEffect (!bLoaded);
			}
			break;


		case KEY_F4 + KEY_SHIFTED:
			if (!gameStates.app.bD1Mission)
				DoEscortMenu ();
			break;


		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			if (!gameStates.app.bD1Mission)
				ChangeGuidebotName ();
			break;

		case KEY_MINUS + KEY_ALTED:
			songManager.Prev ();
			break;

		case KEY_EQUALS + KEY_ALTED:
			songManager.Next ();
			break;

	//added 8/23/99 by Matt Mueller for hot key res/fullscreen changing, and menu access
		case KEY_CTRLED + KEY_SHIFTED + KEY_PADDIVIDE:
		case KEY_ALTED + KEY_CTRLED + KEY_PADDIVIDE:
		case KEY_ALTED + KEY_SHIFTED + KEY_PADDIVIDE:
			RenderOptionsMenu ();
			break;

		default:
			if (!HandleDisplayKey (key))
				return bScreenChanged;
		}	 //switch (key)
	}

if (bStopPlayerMovement) {
	StopPlayerMovement ();
	gameStates.app.bEnterGame = 2;
	}
return bScreenChanged;
}

//------------------------------------------------------------------------------

extern void DropFlag ();

extern int32_t gr_renderstats;

void HandleGameKey(int32_t key)
{
if (OBSERVING)
	return;

switch (key) {

#ifndef D2X_KEYS // weapon selection handled in controls.Read, d1x-style
// MWA changed the weapon select cases to have each case call
// DoSelectWeapon the macintosh keycodes aren't consecutive from 1
// -- 0 on the keyboard -- boy is that STUPID!!!!

	//	Select primary or secondary weapon.
	case KEY_1:
		DoSelectWeapon(0 , 0);
		break;
	case KEY_2:
		DoSelectWeapon(1 , 0);
		break;
	case KEY_3:
		DoSelectWeapon(2 , 0);
		break;
	case KEY_4:
		DoSelectWeapon(3 , 0);
		break;
	case KEY_5:
		DoSelectWeapon(4 , 0);
		break;
	case KEY_6:
		DoSelectWeapon(0 , 1);
		break;
	case KEY_7:
		DoSelectWeapon(1 , 1);
		break;
	case KEY_8:
		DoSelectWeapon(2 , 1);
		break;
	case KEY_9:
		DoSelectWeapon(3 , 1);
		break;
	case KEY_0:
		DoSelectWeapon(4 , 1);
		break;
#endif

	case KEY_1 + KEY_SHIFTED:
	case KEY_2 + KEY_SHIFTED:
	case KEY_3 + KEY_SHIFTED:
	case KEY_4 + KEY_SHIFTED:
	case KEY_5 + KEY_SHIFTED:
	case KEY_6 + KEY_SHIFTED:
	case KEY_7 + KEY_SHIFTED:
	case KEY_8 + KEY_SHIFTED:
	case KEY_9 + KEY_SHIFTED:
	case KEY_0 + KEY_SHIFTED:
	if (gameOpts->gameplay.bEscortHotKeys) {
		if (IsMultiGame)
			HUDInitMessage (TXT_GB_MULTIPLAYER);
		else
			EscortSetSpecialGoal (key);
		break;
		}

	case KEY_ALTED + KEY_P:
#if PROFILING
		gameData.profiler.bToggle = !gameData.profiler.bToggle;
#endif
		break;

	case KEY_ALTED + KEY_F:
		gameStates.render.bShowFrameRate = (gameStates.render.bShowFrameRate + 1) % (6 + (gameStates.render.bPerPixelLighting == 2));
		break;

	case KEY_ALTED + KEY_R:
		ToggleRadar ();
		break;

	case KEY_ALTED + KEY_T:
		gameStates.render.bShowTime = !gameStates.render.bShowTime;
		break;

	case KEY_CTRLED + KEY_ALTED + KEY_R:
		gr_renderstats = !gr_renderstats;
		break;

	case KEY_CTRLED + KEY_ALTED + KEY_ESC:
		gameStates.app.bSingleStep = 1;
		break;

	case KEY_F5 + KEY_SHIFTED:
		DropCurrentWeapon ();
		break;

	case KEY_F6 + KEY_SHIFTED:
		DropSecondaryWeapon (-1);
		break;

	case KEY_0 + KEY_ALTED:
		DropFlag ();
		break;

	case KEY_F4:
		if (!markerManager.DefiningMsg ())
			markerManager.InitInput ();
		break;

	case KEY_F4 + KEY_CTRLED:
		if (!markerManager.DefiningMsg ())
			markerManager.InitInput (true);
		break;

	case KEY_F4 + KEY_CTRLED + KEY_ALTED:
		if (!markerManager.DefiningMsg ())
			markerManager.DropSpawnPoint ();
		break;

	case KEY_ALTED + KEY_CTRLED + KEY_T:
		SwitchTeam (N_LOCALPLAYER, 0);
		break;
	case KEY_F6:
		if (netGameInfo.m_info.bRefusePlayers && networkData.refuse.bWaitForAnswer && !IsTeamGame) {
			networkData.refuse.bThisPlayer = 1;
			HUDInitMessage (TXT_ACCEPT_PLR);
			}
		break;
	case KEY_ALTED + KEY_1:
		if (netGameInfo.m_info.bRefusePlayers && networkData.refuse.bWaitForAnswer && IsTeamGame) {
			networkData.refuse.bThisPlayer = 1;
			HUDInitMessage (TXT_ACCEPT_PLR);
			networkData.refuse.bTeam = 1;
			}
		break;
	case KEY_ALTED + KEY_2:
		if (netGameInfo.m_info.bRefusePlayers && networkData.refuse.bWaitForAnswer && IsTeamGame) {
			networkData.refuse.bThisPlayer = 1;
			HUDInitMessage (TXT_ACCEPT_PLR);
			networkData.refuse.bTeam = 2;
			}
		break;

	default:
		break;
	}	 //switch (key)
}

//	--------------------------------------------------------------------------

#if DBG

void HandleTestKey(int32_t key)
{
	switch (key) {

		case KEYDBGGED + KEY_0:
			ShowWeaponStatus ();   break;

#ifdef SHOW_EXIT_PATH
		case KEYDBGGED + KEY_1:
			MarkPathToExit ();
			break;
#endif

		case KEYDBGGED + KEY_Y:
			DoReactorDestroyedStuff(NULL);
			break;

	case KEYDBGGED + KEY_ALTED + KEY_D:
			networkData.nNetLifeKills = 4000;
			networkData.nNetLifeKilled = 5;
			MultiAddLifetimeKills ();
			break;

		case KEY_BACKSPACE:
		case KEY_CTRLED + KEY_BACKSPACE:
		case KEY_ALTED + KEY_BACKSPACE:
		case KEY_ALTED + KEY_CTRLED + KEY_BACKSPACE:
		case KEY_SHIFTED + KEY_BACKSPACE:
		case KEY_SHIFTED + KEY_ALTED + KEY_BACKSPACE:
		case KEY_SHIFTED + KEY_CTRLED + KEY_BACKSPACE:
		case KEY_SHIFTED + KEY_CTRLED + KEY_ALTED + KEY_BACKSPACE:
			Int3 ();
			break;

		case KEY_CTRLED + KEY_ALTED + KEY_ENTER:
			exit (0);
			break;

		case KEYDBGGED + KEY_S:
			audio.Reset ();
			break;

		case KEYDBGGED + KEY_P:
			if (gameStates.app.bGameSuspended & SUSP_ROBOTS)
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;		//robots move
			else
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;		//robots don't move
			break;


		case KEYDBGGED + KEY_K:
			LOCALPLAYER.SetShield (1);
			break;
						//	a virtual kill
		case KEYDBGGED + KEY_SHIFTED + KEY_K:
			LOCALPLAYER.SetShield (-1);
			break;  //	an actual kill

		case KEYDBGGED + KEY_X:
			LOCALPLAYER.lives++;
			break; // Extra life cheat key.

		case KEYDBGGED + KEY_H:
//				if (!IsMultiGame)   {
				LOCALPLAYER.flags ^= PLAYER_FLAGS_CLOAKED;
				if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
					if (IsMultiGame)
						MultiSendCloak ();
					AIDoCloakStuff ();
					LOCALPLAYER.cloakTime = gameData.time.xGame;
#if TRACE
					console.printf (CON_DBG, "You are cloaked!\n");
#endif
				} else
#if TRACE
					console.printf (CON_DBG, "You are DE-cloaked!\n");
#endif
//				}
			break;


		case KEYDBGGED + KEY_R:
			gameStates.app.cheats.bRobotsFiring = !gameStates.app.cheats.bRobotsFiring;
			break;

		case KEYDBGGED + KEY_R + KEY_SHIFTED:
			KillAllRobots (1);
			break;

		//flythrough keys

#if DBG
		case KEYDBGGED + KEY_LAPOSTRO:
			showViewTextTimer = 0x30000;
			ObjectGotoNextViewer ();
			break;
		case KEYDBGGED + KEY_CTRLED + KEY_LAPOSTRO:
			showViewTextTimer = 0x30000;
			ObjectGotoPrevViewer ();
			break;
#endif
		case KEYDBGGED + KEY_SHIFTED + KEY_LAPOSTRO:
			gameData.objData.pViewer=gameData.objData.pConsole;
			break;

	#if DBG
		case KEYDBGGED + KEY_O:
			ToggleOutlineMode ();
			break;
	#endif
		case KEYDBGGED + KEY_L:
			if (++gameStates.render.nLighting >= 2)
				gameStates.render.nLighting = 0;
			break;
		case KEYDBGGED + KEY_SHIFTED + KEY_L:
			xBeamBrightness = 0x38000 - xBeamBrightness;
			break;
		case KEY_PAD5:
			slew_stop ();
			break;

		case KEYDBGGED  + KEY_F4: {
			//CHitResult hitResult;
			//CFixVector p0 = {-0x1d99a7, -0x1b20000, 0x186ab7f};
			//CFixVector p1 = {-0x217865, -0x1b20000, 0x187de3e};
			//FindHitpoint(&hitResult, &p0, 0x1b9, &p1, 0x40000, 0x0, NULL, -1);
			break;
		}

		case KEYDBGGED + KEY_M:
			gameStates.app.bDebugSpew = !gameStates.app.bDebugSpew;
			if (gameStates.app.bDebugSpew) {
				mopen(0, 8, 1, 78, 16, "Debug Spew");
				HUDInitMessage("Debug Spew: ON");
			} else {
				mclose(0);
				HUDInitMessage("Debug Spew: OFF");
			}
			break;

		case KEYDBGGED + KEY_C:
			//paletteManager.SuspendEffect ();
			DoCheatMenu ();
			//paletteManager.ResumeEffect ();
			break;

		case KEYDBGGED + KEY_SHIFTED + KEY_A:
			DoMegaWowPowerup(10);
			break;

		case KEYDBGGED + KEY_A: {
			DoMegaWowPowerup(200);
//								if (IsMultiGame)     {
//									InfoBox(NULL, 1, "Damn", "CHEATER!\nYou cannot use the\nmega-thing in network mode.");
//									gameData.multigame.msg.nReceiver = 100;		// Send to everyone...
//									sprintf(gameData.multigame.msg.szMsg, "%s cheated!", LOCALPLAYER.callsign);
//								} else {
//									DoMegaWowPowerup ();
//								}
						break;
		}

		case KEYDBGGED + KEY_F:
		gameStates.render.bShowFrameRate = !gameStates.render.bShowFrameRate;
		break;

		case KEYDBGGED + KEY_SPACEBAR:		//KEY_F7:				// Toggle physics flying
			slew_stop ();
			GameFlushInputs ();
			if (gameData.objData.pConsole->info.controlType != CT_FLYING) {
				FlyInit(gameData.objData.pConsole);
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(gameData.objData.pConsole);			//start player slewing
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEYDBGGED + KEY_COMMA:
			gameStates.render.xZoom = FixMul (gameStates.render.xZoom, 62259);
			break;
		case KEYDBGGED + KEY_PERIOD:
			gameStates.render.xZoom = FixMul (gameStates.render.xZoom, 68985);
			break;

		case KEYDBGGED + KEY_B: {
			CMenu	m (1);
			char text [FILENAME_LEN] = "";
			int32_t item;

			m.AddInput ("", text, FILENAME_LEN);
			item = m.Menu (NULL, "Briefing to play?");
			if (item != -1)
				briefing.Run (text, 1);
			break;
		}

		case KEYDBGGED + KEY_ALTED + KEY_F5:
			gameData.time.xGame = I2X(0x7fff - 840);		//will overflow in 14 minutes
#if TRACE
			console.printf (CON_DBG, "gameData.time.xGame bashed to %d secs\n", X2I(gameData.time.xGame));
#endif
			break;

		case KEYDBGGED + KEY_SHIFTED + KEY_B:
			KillEverything (1);
			break;
	}
}
#endif		//#if DBG

//	Cheat functions ------------------------------------------------------------

char old_IntMethod;
char OldHomingState [20];

//------------------------------------------------------------------------------

void ReadControls (void)
{
	int32_t	key;
	fix	keyTime;

	static uint8_t explodingFlag = 0;

gameStates.app.bPlayerFiredLaserThisFrame = -1;
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead) {
		if ((gameData.demo.nState == ND_STATE_PLAYBACK) || (markerManager.DefiningMsg ())
			|| gameData.multigame.msg.bSending || gameData.multigame.msg.bDefining
			)	 // WATCH OUT!!! WEIRD CODE ABOVE!!!
			controls.Reset ();
		else
			controls.Read ();		//NOTE LINK TO ABOVE!!!
	CheckRearView ();
	//	If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
	if (controls [0].automapDownCount &&
		 !LOCALOBJECT->Appearing (false) &&
		 !gameData.objData.speedBoost [OBJ_IDX (gameData.objData.pConsole)].bBoosted &&
		 !(IsMultiGame && gameData.reactor.bDestroyed && (gameData.reactor.countdown.nSecsLeft < 10)))
		automap.SetActive (-1);
	DoWeaponStuff ();
	hudIcons.ToggleWeaponIcons ();
	}
if (LOCALPLAYER.m_bExploded) { //gameStates.app.bPlayerIsDead && (gameData.objData.pConsole->flags & OF_EXPLODING)) {
	if (!explodingFlag)  {
		explodingFlag = 1;			// When player starts exploding, clear all input devices...
		GameFlushInputs ();
		}
	else {
		int32_t i;
		for (i = 0; i < 4; i++)
			if (JoyGetButtonDownCnt (i) > 0)
				gameStates.app.bDeathSequenceAborted = 1;
		for (i = 0; i < 3; i++)
			if (MouseButtonDownCount (i) > 0)
				gameStates.app.bDeathSequenceAborted = 1;
		if (gameStates.app.bDeathSequenceAborted)
			GameFlushInputs ();
		}
	}
else {
	explodingFlag = 0;
	}
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	UpdateVCRState ();

while ((key = KeyInKeyTime (&keyTime)) != 0) {
	if (markerManager.DefiningMsg ()) {
		markerManager.InputMessage (key);
		continue;
		}
	if (IsMultiGame && (gameData.multigame.msg.bSending || gameData.multigame.msg.bDefining)) {
		MultiMsgInputSub (key);
		continue;		//get next key
		}
#if DBG
	if ((key&KEYDBGGED) && IsMultiGame) {
		gameData.multigame.msg.nReceiver = 100;		// Send to everyone...
		sprintf(gameData.multigame.msg.szMsg, "%s %s", TXT_I_AM_A, TXT_CHEATER);
		}
#endif

	if (!console.Events (key))
		continue;

	if (gameStates.app.bPlayerIsDead)
		HandleDeathKey (key);
	if (gameStates.app.bEndLevelSequence)
		HandleEndlevelKey (key);
	else if (gameData.demo.nState == ND_STATE_PLAYBACK) {
		HandleDemoKey (key);
#if DBG
		HandleTestKey (key);
#endif
		}
	else {
		FinalCheats (key);
		HandleSystemKey (key);
		HandleGameKey (key);
#if DBG
		HandleTestKey (key);
#endif
		}
	}
}

//------------------------------------------------------------------------------
//eof
