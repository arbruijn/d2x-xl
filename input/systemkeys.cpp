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
#include "gameseg.h"
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
#include "state.h"
#include "escort.h"
#include "cheats.h"
#include "input.h"
#include "marker.h"
#include "songs.h"

#if defined (TACTILE)
#	include "tactile.h"
#endif

// Global Variables -----------------------------------------------------------

int	redbookVolume = 255;


//	External Variables ---------------------------------------------------------

extern int	*Toggle_var;
extern int	Debug_pause;

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
if ((gameStates.input.keys.pressed[KEY_LSHIFT] || gameStates.input.keys.pressed[KEY_RSHIFT]) && gameStates.input.keys.pressed[KEY_RIGHT])
	gameData.demo.nVcrState = ND_STATE_FASTFORWARD;
else if ((gameStates.input.keys.pressed[KEY_LSHIFT] || gameStates.input.keys.pressed[KEY_RSHIFT]) && gameStates.input.keys.pressed[KEY_LEFT])
	gameData.demo.nVcrState = ND_STATE_REWINDING;
else if (!(gameStates.input.keys.pressed[KEY_LCTRL] || gameStates.input.keys.pressed[KEY_RCTRL]) && gameStates.input.keys.pressed[KEY_RIGHT] && ((TimerGetFixedSeconds () - newdemo_single_frameTime) >= I2X (1)))
	gameData.demo.nVcrState = ND_STATE_ONEFRAMEFORWARD;
else if (!(gameStates.input.keys.pressed[KEY_LCTRL] || gameStates.input.keys.pressed[KEY_RCTRL]) && gameStates.input.keys.pressed[KEY_LEFT] && ((TimerGetFixedSeconds () - newdemo_single_frameTime) >= I2X (1)))
	gameData.demo.nVcrState = ND_STATE_ONEFRAMEBACKWARD;
#if 0
else if ((gameData.demo.nVcrState == ND_STATE_FASTFORWARD) || (gameData.demo.nVcrState == ND_STATE_REWINDING))
	gameData.demo.nVcrState = ND_STATE_PLAYBACK;
#endif
}

//------------------------------------------------------------------------------

char *Pause_msg;

//------------------------------------------------------------------------------

int SetRearView (int bOn)
{
if (gameStates.render.bRearView == bOn)
	return 0;

if ((gameStates.render.bRearView = bOn)) {
	SetFreeCam (0);
	SetChaseCam (0);
	if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
		CGenericCockpit::Save ();
		cockpit->Activate (CM_REAR_VIEW);
		}
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordRearView ();

	}
else {
	if (gameStates.render.cockpit.nType == CM_REAR_VIEW)
		CGenericCockpit::Restore ();
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordRestoreRearView ();
	}
return 1;
}

//------------------------------------------------------------------------------

void CheckRearView (void)
{
	#define LEAVE_TIME 0x1000		//how long until we decide key is down	 (Used to be 0x4000)

	static int nLeaveMode;
	static fix entryTime;

if (Controls [0].rearViewDownCount) {		//key/button has gone down
	Controls [0].rearViewDownCount = 0;
	if (ToggleRearView () && gameStates.render.bRearView) {
		nLeaveMode = 0;		//means wait for another key
		entryTime = TimerGetFixedSeconds ();
		}
	}
else if (Controls [0].rearViewDownState) {
	if (!nLeaveMode && (TimerGetFixedSeconds () - entryTime) > LEAVE_TIME)
		nLeaveMode = 1;
	}
else if (nLeaveMode)
	SetRearView (0);
}

//------------------------------------------------------------------------------

int ToggleRearView (void)
{
return SetRearView (!gameStates.render.bRearView);
}

//------------------------------------------------------------------------------

void ResetRearView (void)
{
if (gameStates.render.bRearView) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordRestoreRearView ();
	}
gameStates.render.bRearView = 0;
if ((gameStates.render.cockpit.nType < 0) || (gameStates.render.cockpit.nType > 4) || (gameStates.render.cockpit.nType == CM_REAR_VIEW)) {
	if (!CGenericCockpit::Restore ())
		cockpit->Activate (CM_FULL_COCKPIT);
	}
}

//------------------------------------------------------------------------------

int SetChaseCam (int bOn)
{
if (gameStates.render.bChaseCam == bOn)
	return 0;
if ((gameStates.render.bChaseCam = bOn)) {
	SetFreeCam (0);
	SetRearView (0);
	CGenericCockpit::Save ();
	if (gameStates.render.cockpit.nType < CM_FULL_SCREEN)
		cockpit->Activate (CM_FULL_SCREEN);
	else
		gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
	}
else
	CGenericCockpit::Restore ();
externalView.Reset (-1, -1);
return 1;
}

//------------------------------------------------------------------------------

int ToggleChaseCam (void)
{
#if !DBG
if (IsMultiGame && !(IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0))) {
	HUDMessage (0, "Chase camera is not available");
	return 0;
	}
#endif
return SetChaseCam (!gameStates.render.bChaseCam);
}

//------------------------------------------------------------------------------

int SetFreeCam (int bOn)
{
if (gameStates.render.bFreeCam == bOn)
	return 0;
if ((gameStates.render.bFreeCam = bOn)) {
	SetChaseCam (0);
	SetRearView (0);
	gameStates.app.playerPos = gameData.objs.viewerP->info.position;
	gameStates.app.nPlayerSegment = gameData.objs.viewerP->info.nSegment;
	CGenericCockpit::Save ();
	if (gameStates.render.cockpit.nType < CM_FULL_SCREEN)
		cockpit->Activate (CM_FULL_SCREEN);
	else
		gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
	}
else {
	gameData.objs.viewerP->info.position = gameStates.app.playerPos;
	gameData.objs.viewerP->RelinkToSeg (gameStates.app.nPlayerSegment);
	CGenericCockpit::Restore ();
	}
return 1;
}

//------------------------------------------------------------------------------

int ToggleFreeCam (void)
{
#if !DBG
if ((IsMultiGame && !(IsCoopGame || EGI_FLAG (bEnableCheats, 0, 0, 0)))) {
	HUDMessage (0, "Free camera is not available");
	return 0;
	}
#endif
return SetFreeCam (!gameStates.render.bFreeCam);
}

//------------------------------------------------------------------------------

void ToggleRadar (void)
{
if (extraGameInfo [0].nRadar && (gameOpts->render.automap.nRange < 2))
	gameOpts->render.automap.nRange++;
else {
	extraGameInfo [0].nRadar = (extraGameInfo [0].nRadar + 1) % 3;
	gameOpts->render.automap.nRange = 0;
	}
}

//------------------------------------------------------------------------------

void HandleEndlevelKey (int key)
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

void HandleDeathKey(int key)
{
/*
	Commented out redundant calls because the key used here typically
	will be passed to HandleSystemKey later.  Note that I do this to pause
	which is supposed to pass the ESC key to leave the level.  This
	doesn't work in the DOS version anyway.   -Samir
*/

if (gameStates.app.bPlayerExploded && !key_isfunc (key) && !key_ismod (key))
	gameStates.app.bDeathSequenceAborted  = 1;		//Any key but func or modifier aborts

if (key == KEY_COMMAND + KEY_SHIFTED + KEY_P) {
	gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}
else if (key == KEY_PRINT_SCREEN) {
	gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}
else if (key == (KEY_COMMAND + KEY_P)) {
	gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}
else if (key == KEY_PAUSE)   {
	gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	}
else if (key == KEY_ESC) {
	if (gameData.objs.consoleP->info.nFlags & OF_EXPLODING)
		gameStates.app.bDeathSequenceAborted = 1;
	}
else if (key == KEY_BACKSPACE)  {
	gameStates.app.bDeathSequenceAborted  = 0;		// Clear because code above sets this for any key.
	Int3 ();
	}
//don't abort death sequence for netgame join/refuse keys
else if ((key == KEY_ALTED + KEY_1) || (key == KEY_ALTED + KEY_2))
	gameStates.app.bDeathSequenceAborted  = 0;
if (gameStates.app.bDeathSequenceAborted)
	GameFlushInputs ();
}

//------------------------------------------------------------------------------

void HandleDemoKey (int key)
{
if (gameOpts->demo.bRevertFormat && (gameData.demo.nVersion > DEMO_VERSION))
	return;
switch (key) {
	case KEY_F3:
		 if (!(GuidedInMainView () || gameStates.render.bChaseCam || gameStates.render.bFreeCam))
			cockpit->Toggle ();
		 break;

	case KEY_SHIFTED + KEY_MINUS:
	case KEY_MINUS:
		ShrinkWindow ();
		break;

	case KEY_SHIFTED + KEY_EQUALS:
	case KEY_EQUALS:
		GrowWindow ();
		break;

	case KEY_F2:
		gameStates.app.bConfigMenu = 1;
		break;

	case KEY_F7:
		gameData.multigame.kills.bShowList = (gameData.multigame.kills.bShowList+1) % ((gameData.demo.nGameMode & GM_TEAM) ? 4 : 3);
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
		int oldState = gameData.demo.nVcrState;
		gameData.demo.nVcrState = ND_STATE_PRINTSCREEN;
		//RenderMonoFrame ();
		gameStates.app.bSaveScreenshot = 1;
		SaveScreenShot (NULL, 0);
		gameData.demo.nVcrState = oldState;
		break;
		}
	}
}

//------------------------------------------------------------------------------
//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
int HandleSystemKey (int key)
{
	int bScreenChanged = 0;
	int bStopPlayerMovement = 1;

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
/*
		case KEY_SHIFTED + KEY_ESC:
			console.Show ();
			break;
*/
		case KEY_COMMAND + KEY_P:
		case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
			DoGamePause ();
			break;

		case KEY_CTRLED + KEY_ALTED + KEY_S:
			if (!ToggleFreeCam ())
				return 0;
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_PRINT_SCREEN:
			gameStates.app.bSaveScreenshot = 1;
			SaveScreenShot (NULL, 0);
			break;

		case KEY_F1:
			ShowHelp ();
			break;

		case KEY_F2:					//gameStates.app.bConfigMenu = 1; break;
			if (!IsMultiGame) {
				paletteManager.SaveEffect ();
				paletteManager.ResetEffect ();
				paletteManager.LoadEffect ();
				}
			ConfigMenu ();
			if (!IsMultiGame)
				paletteManager.LoadEffect ();
			break;

		case KEY_F3:
			if (!GuidedInMainView ()) {
				SetFreeCam (0);
				SetChaseCam (0);
				cockpit->Toggle ();
				bScreenChanged = 1;
				}
			break;

		case KEY_F7 + KEY_SHIFTED:
			paletteManager.SaveEffect ();
			JoyDefsCalibrate ();
			paletteManager.LoadEffect ();
			break;

		case KEY_SHIFTED + KEY_MINUS:
		case KEY_MINUS:
			ShrinkWindow ();
			bScreenChanged = 1;
			break;

		case KEY_SHIFTED + KEY_EQUALS:
		case KEY_EQUALS:
			GrowWindow ();
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

		case KEY_ALTED + KEY_F4:
			gameData.multigame.bShowReticleName = (gameData.multigame.bShowReticleName + 1) % 2;

		case KEY_F7:
			gameData.multigame.kills.bShowList = (gameData.multigame.kills.bShowList + 1) % (IsTeamGame ? 4 : 3);
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

		case KEY_CTRLED + KEY_F12:
			gameData.trackIR.x =
			gameData.trackIR.y = 0;
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
				paletteManager.SaveEffectAndReset ();
				paletteManager.SetEffect (); // get only the effect color back
				saveGameManager.Save (0, 0, 0, NULL);
				paletteManager.LoadEffect ();
			}
			break;  // 0 means not between levels.

		case KEY_ALTED + KEY_F3:
			if (!gameStates.app.bPlayerIsDead && (!IsMultiGame || IsCoopGame)) {
				paletteManager.SaveEffectAndReset ();
				saveGameManager.Load (1, 0, 0, NULL);
				if (gameData.app.bGamePaused)
					DoGamePause ();
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

		case KEY_CTRLED + KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED + KEY_F2:
			SwitchDisplayMode (1);
			break;

		case KEY_ALTED + KEY_ENTER:
		case KEY_ALTED + KEY_PADENTER:
			GrToggleFullScreenGame ();
			break;

		default:
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

void HandleVRKey(int key)
{
	switch (key)   {

		case KEY_ALTED + KEY_F5:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				VRResetParams ();
				HUDInitMessage(TXT_VR_RESET);
				HUDInitMessage(TXT_VR_SEPARATION, X2F(gameStates.render.vr.xEyeWidth));
				HUDInitMessage(TXT_VR_BALANCE, (double)gameStates.render.vr.nEyeOffset/30.0);
			}
			break;

		case KEY_ALTED + KEY_F6:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				gameStates.render.vr.nLowRes++;
				if (gameStates.render.vr.nLowRes > 3) gameStates.render.vr.nLowRes = 0;
				switch(gameStates.render.vr.nLowRes)    {
					case 0: HUDInitMessage(TXT_VR_NORMRES); break;
					case 1: HUDInitMessage(TXT_VR_LOWVRES); break;
					case 2: HUDInitMessage(TXT_VR_LOWHRES); break;
					case 3: HUDInitMessage(TXT_VR_LOWRES); break;
				}
			}
			break;

		case KEY_ALTED + KEY_F7:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				gameStates.render.vr.nEyeSwitch = !gameStates.render.vr.nEyeSwitch;
				HUDInitMessage(TXT_VR_TOGGLE);
				if (gameStates.render.vr.nEyeSwitch)
					HUDInitMessage(TXT_VR_RLEYE);
				else
					HUDInitMessage(TXT_VR_LREYE);
			}
			break;

		case KEY_ALTED + KEY_F8:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
			gameStates.render.vr.nSensitivity++;
			if (gameStates.render.vr.nSensitivity > 2)
				gameStates.render.vr.nSensitivity = 0;
			HUDInitMessage(TXT_VR_SENSITIVITY, gameStates.render.vr.nSensitivity);
		 }
			break;
		case KEY_ALTED + KEY_F9:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				gameStates.render.vr.xEyeWidth -= I2X (1)/10;
				if (gameStates.render.vr.xEyeWidth < 0) gameStates.render.vr.xEyeWidth = 0;
				HUDInitMessage(TXT_VR_SEPARATION, X2F(gameStates.render.vr.xEyeWidth));
				HUDInitMessage(TXT_VR_DEFAULT, X2F(VR_SEPARATION));
			}
			break;
		case KEY_ALTED + KEY_F10:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				gameStates.render.vr.xEyeWidth += I2X (1)/10;
				if (gameStates.render.vr.xEyeWidth > I2X (4))    gameStates.render.vr.xEyeWidth = I2X (4);
				HUDInitMessage(TXT_VR_SEPARATION, X2F(gameStates.render.vr.xEyeWidth));
				HUDInitMessage(TXT_VR_DEFAULT, X2F(VR_SEPARATION));
			}
			break;

		case KEY_ALTED + KEY_F11:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				gameStates.render.vr.nEyeOffset--;
				if (gameStates.render.vr.nEyeOffset < -30)	gameStates.render.vr.nEyeOffset = -30;
				HUDInitMessage(TXT_VR_BALANCE, (double)gameStates.render.vr.nEyeOffset/30.0);
				HUDInitMessage(TXT_VR_DEFAULT, (double)VR_PIXEL_SHIFT/30.0);
				gameStates.render.vr.bEyeOffsetChanged = 2;
			}
			break;
		case KEY_ALTED + KEY_F12:
			if (gameStates.render.vr.nRenderMode != VR_NONE) {
				gameStates.render.vr.nEyeOffset++;
				if (gameStates.render.vr.nEyeOffset > 30)	 gameStates.render.vr.nEyeOffset = 30;
				HUDInitMessage(TXT_VR_BALANCE, (double)gameStates.render.vr.nEyeOffset/30.0);
				HUDInitMessage(TXT_VR_DEFAULT, (double)VR_PIXEL_SHIFT/30.0);
				gameStates.render.vr.bEyeOffsetChanged = 2;
			}
			break;
	}
}

//------------------------------------------------------------------------------

extern void DropFlag ();

extern int gr_renderstats;

void HandleGameKey(int key)
{
	switch (key) {

#ifndef D2X_KEYS // weapon selection handled in ControlsReadAll, d1x-style
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

		case KEY_ALTED + KEY_M:
			gameStates.app.bMultiThreaded = !gameStates.app.bMultiThreaded;
			HUDMessage (0, "multi-threading %s", gameStates.app.bMultiThreaded ? "ON" : "OFF");
			break;

		case KEY_ALTED + KEY_P:
#if PROFILING
			if ((gameStates.render.bShowProfiler = !gameStates.render.bShowProfiler))
				memset (&gameData.profiler, 0, sizeof (gameData.profiler));
#endif
			break;

		case KEY_ALTED + KEY_F:
			gameStates.render.bShowFrameRate = ++gameStates.render.bShowFrameRate % (6 + (gameStates.render.bPerPixelLighting == 2));
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
	 DropSecondaryWeapon(-1);
	 break;

		case KEY_0 + KEY_ALTED:
			DropFlag ();
			break;

		case KEY_F4:
			if (!gameData.marker.nDefiningMsg)
				InitMarkerInput ();
			break;

		case KEY_F4 + KEY_CTRLED:
			if (!gameData.marker.nDefiningMsg)
				DropSpawnMarker ();
			break;

		case KEY_ALTED + KEY_CTRLED + KEY_T:
			SwitchTeam (gameData.multiplayer.nLocalPlayer, 0);
			break;
		case KEY_F6:
			if (netGame.bRefusePlayers && networkData.refuse.bWaitForAnswer && !(gameData.app.nGameMode & GM_TEAM))
			 {
					networkData.refuse.bThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
				}
			break;
		case KEY_ALTED + KEY_1:
			if (netGame.bRefusePlayers && networkData.refuse.bWaitForAnswer && (gameData.app.nGameMode & GM_TEAM))
			 {
					networkData.refuse.bThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
					networkData.refuse.bTeam=1;
				}
			break;
		case KEY_ALTED + KEY_2:
			if (netGame.bRefusePlayers && networkData.refuse.bWaitForAnswer && (gameData.app.nGameMode & GM_TEAM))
			 {
					networkData.refuse.bThisPlayer=1;
					HUDInitMessage (TXT_ACCEPT_PLR);
					networkData.refuse.bTeam=2;
				}
			break;

		default:
			break;

	}	 //switch (key)
}

//	--------------------------------------------------------------------------

void ToggleMovieSaving(void);

#if DBG

void HandleTestKey(int key)
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
			networkData.nNetLifeKills=4000;
			networkData.nNetLifeKilled=5;
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
			LOCALPLAYER.shields = 1;
			MultiSendShields ();
			break;
						//	a virtual kill
		case KEYDBGGED + KEY_SHIFTED + KEY_K:
			LOCALPLAYER.shields = -1;
			MultiSendShields ();
			break;  //	an actual kill

		case KEYDBGGED + KEY_X:
			LOCALPLAYER.lives++;
			break; // Extra life cheat key.

		case KEYDBGGED + KEY_H:
//				if (!(gameData.app.nGameMode & GM_MULTI))   {
				LOCALPLAYER.flags ^= PLAYER_FLAGS_CLOAKED;
				if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
					if (gameData.app.nGameMode & GM_MULTI)
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
			gameData.objs.viewerP=gameData.objs.consoleP;
			break;

	#if DBG
		case KEYDBGGED + KEY_O:
			ToggleOutlineMode ();
			break;
	#endif
		case KEYDBGGED + KEY_T:
			*Toggle_var = !*Toggle_var;
#if TRACE
			console.printf (CON_DBG, "Variable at %08x set to %i\n", Toggle_var, *Toggle_var);
#endif
			break;
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
			//tFVIData hit_data;
			//CFixVector p0 = {-0x1d99a7, -0x1b20000, 0x186ab7f};
			//CFixVector p1 = {-0x217865, -0x1b20000, 0x187de3e};
			//FindVectorIntersection(&hit_data, &p0, 0x1b9, &p1, 0x40000, 0x0, NULL, -1);
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
			paletteManager.SaveEffectAndReset ();
			DoCheatMenu ();
			paletteManager.LoadEffect ();
			break;

		case KEYDBGGED + KEY_SHIFTED + KEY_A:
			DoMegaWowPowerup(10);
			break;

		case KEYDBGGED + KEY_A: {
			DoMegaWowPowerup(200);
//								if (gameData.app.nGameMode & GM_MULTI)     {
//									MsgBox(NULL, 1, "Damn", "CHEATER!\nYou cannot use the\nmega-thing in network mode.");
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
			if (gameData.objs.consoleP->info.controlType != CT_FLYING) {
				FlyInit(gameData.objs.consoleP);
				gameStates.app.bGameSuspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(gameData.objs.consoleP);			//start CPlayerData slewing
				gameStates.app.bGameSuspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEYDBGGED + KEY_COMMA:
			gameStates.render.xZoom = FixMul(gameStates.render.xZoom, 62259);
			break;
		case KEYDBGGED + KEY_PERIOD:
			gameStates.render.xZoom = FixMul(gameStates.render.xZoom, 68985);
			break;

		case KEYDBGGED + KEY_P + KEY_SHIFTED:
			Debug_pause = 1;
			break;

		case KEYDBGGED + KEY_B: {
			CMenu	m (1);
			char text [FILENAME_LEN] = "";
			int item;

			m.AddInput (text, FILENAME_LEN);
			item = m.Menu (NULL, "Briefing to play?");
			if (item != -1)
				briefing.Run (text, 1);
			break;
		}

		case KEYDBGGED + KEY_F5:
			ToggleMovieSaving ();
			break;

		case KEYDBGGED + KEY_SHIFTED + KEY_F5: {
			extern int Movie_fixed_frametime;
			Movie_fixed_frametime = !Movie_fixed_frametime;
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

int ReadControls (void)
{
	int key, skipControls = 0;
	fix keyTime;
	static ubyte explodingFlag=0;

gameStates.app.bPlayerFiredLaserThisFrame=-1;
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead) {
		if ((gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.marker.nDefiningMsg)
			|| gameData.multigame.msg.bSending || gameData.multigame.msg.bDefining
			)	 // WATCH OUT!!! WEIRD CODE ABOVE!!!
			memset(&Controls, 0, sizeof(tControlInfo));
		else
			skipControls = ControlsReadAll ();		//NOTE LINK TO ABOVE!!!
	CheckRearView ();
	//	If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
	if (Controls [0].automapDownCount &&
		 !gameData.objs.speedBoost [OBJ_IDX (gameData.objs.consoleP)].bBoosted &&
		 !(IsMultiGame && gameData.reactor.bDestroyed && (gameData.reactor.countdown.nSecsLeft < 10)))
		automap.m_bDisplay = -1;
	DoWeaponStuff ();
	}
if (gameStates.app.bPlayerExploded) { //gameStates.app.bPlayerIsDead && (gameData.objs.consoleP->flags & OF_EXPLODING)) {
	if (!explodingFlag)  {
		explodingFlag = 1;			// When CPlayerData starts exploding, clear all input devices...
		GameFlushInputs ();
		}
	else {
		int i;
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
	if (gameData.marker.nDefiningMsg) {
		MarkerInputMessage (key);
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
#ifdef CONSOLE
if(!console.Events (key))
	continue;
#endif
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
		HandleVRKey (key);
		HandleGameKey (key);
#if DBG
		HandleTestKey (key);
#endif
		}
	}
return skipControls;
}

//------------------------------------------------------------------------------

extern kcItem kcMouse [];

inline int ZoomKeyPressed (void)
{
#if 1
	int	v;

return gameStates.input.keys.pressed [kcKeyboard [52].value] || gameStates.input.keys.pressed [kcKeyboard [53].value] ||
		 (((v = kcMouse [30].value) < 255) && MouseButtonState (v));
#else
return (Controls [0].zoomDownCount > 0);
#endif
}

//------------------------------------------------------------------------------

void HandleZoom (void)
{
if (extraGameInfo [IsMultiGame].nZoomMode == 0)
	return;

	bool bAllow = (gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX);

if (extraGameInfo [IsMultiGame].nZoomMode == 1) {
	if (!gameStates.zoom.nState) {
		if (!bAllow || ZoomKeyPressed ()) {
			if (!bAllow) {
				if (gameStates.zoom.nFactor <= gameStates.zoom.nMinFactor)
					return;
				gameStates.zoom.nDestFactor = gameStates.zoom.nMinFactor;
				}
			else if (gameStates.zoom.nFactor >= gameStates.zoom.nMaxFactor)
				gameStates.zoom.nDestFactor = gameStates.zoom.nMinFactor;
			else
				gameStates.zoom.nDestFactor = fix (gameStates.zoom.nFactor * pow (float (gameStates.zoom.nMaxFactor) / float (gameStates.zoom.nMinFactor), 0.25f) + 0.5f);
			gameStates.zoom.nStep = float (gameStates.zoom.nDestFactor - gameStates.zoom.nFactor) / 6.25f;
			gameStates.zoom.nTime = gameStates.app.nSDLTicks - 40;
			gameStates.zoom.nState = (gameStates.zoom.nStep > 0) ? 1 : -1;
			if (audio.ChannelIsPlaying (gameStates.zoom.nChannel))
				audio.StopSound (gameStates.zoom.nChannel);
			gameStates.zoom.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xFFFF / 2, 0, 0, 0, -1, I2X (1), AddonSoundName (SND_ADDON_ZOOM1));
			}
		}
	}
else if (extraGameInfo [IsMultiGame].nZoomMode == 2) {
	if (bAllow && ZoomKeyPressed ()) {
		if ((gameStates.zoom.nState <= 0) && (gameStates.zoom.nFactor < gameStates.zoom.nMaxFactor)) {
			if (audio.ChannelIsPlaying (gameStates.zoom.nChannel))
				audio.StopSound (gameStates.zoom.nChannel);
			gameStates.zoom.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xFFFF / 2, 0, 0, 0, -1, I2X (1), AddonSoundName (SND_ADDON_ZOOM2));
			gameStates.zoom.nDestFactor = gameStates.zoom.nMaxFactor;
			gameStates.zoom.nStep = float (gameStates.zoom.nDestFactor - gameStates.zoom.nFactor) / 25.0f;
			gameStates.zoom.nTime = gameStates.app.nSDLTicks - 40;
			gameStates.zoom.nState = 1;
			}
		}
	else if ((gameStates.zoom.nState >= 0) && (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor)) {
		if (audio.ChannelIsPlaying (gameStates.zoom.nChannel))
			audio.StopSound (gameStates.zoom.nChannel);
		gameStates.zoom.nChannel = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (1), 0xFFFF / 2, 0, 0, 0, -1, I2X (1), AddonSoundName (SND_ADDON_ZOOM2));
		gameStates.zoom.nDestFactor = gameStates.zoom.nMinFactor;
		gameStates.zoom.nStep = float (gameStates.zoom.nDestFactor - gameStates.zoom.nFactor) / 25.0f;
		gameStates.zoom.nTime = gameStates.app.nSDLTicks - 40;
		gameStates.zoom.nState = -1;
		}
	}
if (!gameStates.zoom.nState)
	gameStates.zoom.nChannel = -1;
else if (gameStates.app.nSDLTicks - gameStates.zoom.nTime >= 40) {
	gameStates.zoom.nTime += 40;
	gameStates.zoom.nFactor += gameStates.zoom.nStep;
	if (((gameStates.zoom.nState > 0) && (gameStates.zoom.nFactor > gameStates.zoom.nDestFactor)) || 
		 ((gameStates.zoom.nState < 0) && (gameStates.zoom.nFactor < gameStates.zoom.nDestFactor))) {
		gameStates.zoom.nFactor = float (gameStates.zoom.nDestFactor);
		gameStates.zoom.nState = 0;
		gameStates.zoom.nChannel = -1;
		}
	}
}

//------------------------------------------------------------------------------
//eof
