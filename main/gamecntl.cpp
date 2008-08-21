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
#include <stdarg.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "inferno.h"
#include "console.h"
#include "key.h"
#include "menu.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "timer.h"
#include "render.h"
#include "transprender.h"
#include "screens.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "gameseg.h"
#include "u_mem.h"
#include "light.h"
#include "newdemo.h"
#include "automap.h"
#include "text.h"
#include "network_lib.h"
#include "gamefont.h"
#include "gamepal.h"
#include "kconfig.h"
#include "mouse.h"
#include "playsave.h"
#include "piggy.h"
#include "ai.h"
#include "rbaudio.h"
#include "switch.h"
#include "ogl_defs.h"
#include "object.h"
#include "render.h"
#include "marker.h"
#include "systemkeys.h"

#if defined (TACTILE)
#	include "tactile.h"
#endif

char *pszPauseMsg = NULL;

//------------------------------------------------------------------------------
//#define TEST_TIMER    1		//if this is set, do checking on timer

#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>		//should come after inferno[HA] to get mark setting
#endif
#endif

#ifdef SDL_INPUT
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif
#endif

//	Function prototypes --------------------------------------------------------

void SpeedtestInit(void);
void SpeedtestFrame(void);
void PlayTestSound(void);

// Functions ------------------------------------------------------------------

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

void TransferEnergyToShield(fix time)
{
	fix e;		//how much energy gets transfered
	static fix last_playTime=0;

	if (time <= 0)
		return;
	e = std::min(std::min(time*CONVERTER_RATE, LOCALPLAYER.energy - INITIAL_ENERGY), 
		         (MAX_SHIELDS-LOCALPLAYER.shields)*CONVERTER_SCALE);
	if (e <= 0) {
		if (LOCALPLAYER.energy <= INITIAL_ENERGY)
			HUDInitMessage(TXT_TRANSFER_ENERGY, f2i(INITIAL_ENERGY));
		else
			HUDInitMessage(TXT_TRANSFER_SHIELDS);
		return;
	}

	LOCALPLAYER.energy  -= e;
	LOCALPLAYER.shields += e/CONVERTER_SCALE;
	MultiSendShields ();
	gameStates.app.bUsingConverter = 1;
	if (last_playTime > gameData.time.xGame)
		last_playTime = 0;

	if (gameData.time.xGame > last_playTime+CONVERTER_SOUND_DELAY) {
		DigiPlaySampleOnce(SOUND_CONVERT_ENERGY, F1_0);
		last_playTime = gameData.time.xGame;
	}
}

//------------------------------------------------------------------------------

void formatTime(char *str, int secs_int)
{
	int h, m, s;

h = secs_int/3600;
s = secs_int%3600;
m = s / 60;
s = s % 60;
sprintf(str, "%1d:%02d:%02d", h, m, s );
}

//------------------------------------------------------------------------------

void PauseGame (void)
{
if (!gameData.app.bGamePaused) {
	gameData.app.bGamePaused = 1;
	DigiPauseAll();
	RBAPause();
	StopTime();
	PaletteSave();
	ResetPaletteAdd();
	GameFlushInputs();
#if defined (TACTILE)
	if (TactileStick)
		DisableForces();
#endif
	}
}

//------------------------------------------------------------------------------

void ResumeGame (void)
{
GameFlushInputs ();
ResetCockpit ();
PaletteRestore ();
StartTime (0);
if (gameStates.sound.bRedbookPlaying)
	RBAResume ();
DigiResumeAll ();
gameData.app.bGamePaused = 0;
}

//------------------------------------------------------------------------------

void DoShowNetgameHelp();

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int DoGamePause()
{
	int key;
	int bScreenChanged;
	char msg[1000];
	char totalTime[9], xLevelTime[9];

	key=0;

if (gameData.app.bGamePaused) {		//unpause!
	gameData.app.bGamePaused = 0;
	gameStates.app.bEnterGame = 1;
#if defined (TACTILE)
	if (TactileStick)
		EnableForces();
#endif
	return KEY_PAUSE;
	}

if (gameData.app.nGameMode & GM_NETWORK) {
	 DoShowNetgameHelp();
    return (KEY_PAUSE);
	}
else if (gameData.app.nGameMode & GM_MULTI) {
	HUDInitMessage (TXT_MODEM_PAUSE);
	return (KEY_PAUSE);
	}
PauseGame ();
SetPopupScreenMode();
GrPaletteStepLoad (NULL);
formatTime(totalTime, f2i(LOCALPLAYER.timeTotal) + LOCALPLAYER.hoursTotal*3600);
formatTime(xLevelTime, f2i(LOCALPLAYER.timeLevel) + LOCALPLAYER.hoursLevel*3600);
  if (gameData.demo.nState!=ND_STATE_PLAYBACK)
	sprintf(msg, TXT_PAUSE_MSG1, GAMETEXT (332 + gameStates.app.nDifficultyLevel), 
			  LOCALPLAYER.hostages.nOnBoard, xLevelTime, totalTime);
   else
	  	sprintf(msg, TXT_PAUSE_MSG2, GAMETEXT (332 +  gameStates.app.nDifficultyLevel), 
				  LOCALPLAYER.hostages.nOnBoard);

if (!gameOpts->menus.nStyle) {
	gameStates.menus.nInMenu++;
	GameRenderFrame();
	gameStates.menus.nInMenu--;
	ShowBoxedMessage(pszPauseMsg=msg);		  //TXT_PAUSE);
	}
GrabMouse (0, 0);
while (gameData.app.bGamePaused) {
	if (!(gameOpts->menus.nStyle && gameStates.app.bGameRunning))
		key = KeyGetChar();
	else {
		gameStates.menus.nInMenu++;
		while (!(key = KeyInKey ())) {
			GameRenderFrame ();
			GrPaletteStepLoad(NULL);
			RemapFontsAndMenus (1);
			ShowBoxedMessage(msg);
			G3_SLEEP (0);
			}
		gameStates.menus.nInMenu--;
		}
#ifdef _DEBUG
		HandleTestKey(key);
#endif
		bScreenChanged = HandleSystemKey(key);
		HandleVRKey(key);
		if (bScreenChanged) {
			GameRenderFrame();
			ShowBoxedMessage(msg);
#if 0		
			show_extraViews();
			if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR)
				RenderGauges();
#endif			
			}
		}
	GrabMouse (1, 0);
	if (gameStates.render.vr.nScreenFlags & VRF_COMPATIBLE_MENUS) {
		ClearBoxedMessage();
	}
ResumeGame ();
return key;
}

//------------------------------------------------------------------------------
//switch a cockpit window to the next function
int SelectNextWindowFunction(int nWindow)
{
	Assert(nWindow==0 || nWindow==1);

	switch (gameStates.render.cockpit.n3DView [nWindow]) {
		case CV_NONE:
			gameStates.render.cockpit.n3DView [nWindow] = CV_REAR;
			break;
		case CV_REAR:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
			    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
				gameStates.render.cockpit.n3DView [nWindow] = CV_RADAR_TOPDOWN;
				break;
				}
		case CV_RADAR_TOPDOWN:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
			    (!(gameData.app.nGameMode & GM_MULTI) || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
				gameStates.render.cockpit.n3DView [nWindow] = CV_RADAR_HEADSUP;
				break;
				}
		case CV_RADAR_HEADSUP:
			if (find_escort()) {
				gameStates.render.cockpit.n3DView [nWindow] = CV_ESCORT;
				break;
			}
			//if no ecort, fall through
		case CV_ESCORT:
			gameStates.render.cockpit.nCoopPlayerView [nWindow] = -1;		//force first tPlayer
			//fall through
		case CV_COOP:
			gameData.marker.viewers [nWindow] = -1;
			if ((gameData.app.nGameMode & GM_MULTI_COOP) || (gameData.app.nGameMode & GM_TEAM)) {
				gameStates.render.cockpit.n3DView [nWindow] = CV_COOP;
				while (1) {
					gameStates.render.cockpit.nCoopPlayerView [nWindow]++;
					if (gameStates.render.cockpit.nCoopPlayerView [nWindow] == gameData.multiplayer.nPlayers) {
						gameStates.render.cockpit.n3DView [nWindow] = CV_MARKER;
						goto case_marker;
					}
					if (gameStates.render.cockpit.nCoopPlayerView [nWindow]==gameData.multiplayer.nLocalPlayer)
						continue;

					if (gameData.app.nGameMode & GM_MULTI_COOP)
						break;
					else if (GetTeam(gameStates.render.cockpit.nCoopPlayerView [nWindow]) == GetTeam(gameData.multiplayer.nLocalPlayer))
						break;
				}
				break;
			}
			//if not multi, fall through
		case CV_MARKER:
		case_marker:;
			if (!IsMultiGame || IsCoopGame || netGame.bAllowMarkerView) {	//anarchy only
				gameStates.render.cockpit.n3DView [nWindow] = CV_MARKER;
				if (gameData.marker.viewers [nWindow] == -1)
					gameData.marker.viewers [nWindow] = gameData.multiplayer.nLocalPlayer * 3;
				else if (gameData.marker.viewers [nWindow] < gameData.multiplayer.nLocalPlayer * 3 + MaxDrop ())
					gameData.marker.viewers [nWindow]++;
				else
					gameStates.render.cockpit.n3DView [nWindow] = CV_NONE;
			}
			else
				gameStates.render.cockpit.n3DView [nWindow] = CV_NONE;
			break;
	}
	WritePlayerFile();

	return 1;	 //bScreenChanged
}

//	Testing functions ----------------------------------------------------------

#ifdef _DEBUG
void SpeedtestInit(void)
{
	gameData.speedtest.nStartTime = TimerGetFixedSeconds();
	gameData.speedtest.bOn = 1;
	gameData.speedtest.nSegment = 0;
	gameData.speedtest.nSide = 0;
	gameData.speedtest.nFrameStart = gameData.app.nFrameCount;
#if TRACE
	con_printf (CONDBG, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", gameData.segs.nLastSegment+1);
#endif
}

//------------------------------------------------------------------------------

void SpeedtestFrame(void)
{
	vmsVector	view_dir, center_point;

	gameData.speedtest.nSide=gameData.speedtest.nSegment % MAX_SIDES_PER_SEGMENT;

	COMPUTE_SEGMENT_CENTER(&gameData.objs.viewer->position.vPos, &gameData.segs.segments[gameData.speedtest.nSegment]);
	gameData.objs.viewer->position.vPos[X] += 0x10;	
	gameData.objs.viewer->position.vPos[Y] -= 0x10;	
	gameData.objs.viewer->position.vPos[Z] += 0x17;

	RelinkObject(OBJ_IDX (gameData.objs.viewer), gameData.speedtest.nSegment);
	COMPUTE_SIDE_CENTER(&center_point, &gameData.segs.segments[gameData.speedtest.nSegment], gameData.speedtest.nSide);
	vmsVector::normalizedDir(view_dir, center_point, gameData.objs.viewer->position.vPos);
	/*
	gameData.objs.viewer->position.mOrient = vmsMatrix::Create(view_dir, NULL, NULL);
	*/
	// TODO: MatrixCreateFCheck
	gameData.objs.viewer->position.mOrient = vmsMatrix::CreateF(view_dir);
	if (((gameData.app.nFrameCount - gameData.speedtest.nFrameStart) % 10) == 0) {
#if TRACE
		con_printf (CONDBG, ".");
#endif
		}
	gameData.speedtest.nSegment++;

	if (gameData.speedtest.nSegment > gameData.segs.nLastSegment) {
		char    msg[128];

		sprintf(msg, TXT_SPEEDTEST, 
			gameData.app.nFrameCount-gameData.speedtest.nFrameStart, 
			f2fl(TimerGetFixedSeconds() - gameData.speedtest.nStartTime), 
			(double) (gameData.app.nFrameCount-gameData.speedtest.nFrameStart) / f2fl(TimerGetFixedSeconds() - gameData.speedtest.nStartTime));
#if TRACE
		con_printf (CONDBG, "%s", msg);
#endif
		HUDInitMessage(msg);

		gameData.speedtest.nCount--;
		if (gameData.speedtest.nCount == 0)
			gameData.speedtest.bOn = 0;
		else
			SpeedtestInit();
	}

}
#endif
//------------------------------------------------------------------------------
//eof
