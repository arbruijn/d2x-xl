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

#include "descent.h"
#include "console.h"
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
#include "network_lib.h"
#include "gamefont.h"
#include "gamepal.h"
#include "kconfig.h"
#include "mouse.h"
#include "playerprofile.h"
#include "piggy.h"
#include "ai.h"
#include "rbaudio.h"
#include "trigger.h"
#include "ogl_defs.h"
#include "object.h"
#include "rendermine.h"
#include "marker.h"
#include "systemkeys.h"
#include "songs.h"

#if defined (FORCE_FEEDBACK)
#	include "tactile.h"
#endif

char *pszPauseMsg = NULL;

//------------------------------------------------------------------------------
//#define TEST_TIMER    1		//if this is set, do checking on timer

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

//	Function prototypes --------------------------------------------------------

void SpeedtestInit(void);
void SpeedtestFrame(void);
void PlayTestSound(void);

// Functions ------------------------------------------------------------------

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shield

#define CONVERTER_SOUND_DELAY (I2X (1)/2)		//play every half second

void TransferEnergyToShield (fix time)
{
	static fix last_playTime = 0;

if (time <= 0)
	return;
fix e = Min (time * CONVERTER_RATE, LOCALPLAYER.Energy () - LOCALPLAYER.InitialEnergy ());
e = Min (e, (LOCALPLAYER.MaxShield () - LOCALPLAYER.Shield ()) * CONVERTER_SCALE);
if (e <= 0) {
	if (LOCALPLAYER.Energy () <= INITIAL_ENERGY)
		HUDInitMessage (TXT_TRANSFER_ENERGY, X2I (LOCALPLAYER.InitialEnergy ()));
	else
		HUDInitMessage (TXT_TRANSFER_SHIELDS);
	return;
}

LOCALPLAYER.UpdateEnergy (-e);
LOCALPLAYER.UpdateShield (e / CONVERTER_SCALE);
OBJECTS [N_LOCALPLAYER].ResetDamage ();
MultiSendShield ();
gameStates.app.bUsingConverter = 1;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;

if (gameData.time.xGame > last_playTime + CONVERTER_SOUND_DELAY) {
	audio.PlaySound (SOUND_CONVERT_ENERGY);
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
	audio.PauseAll ();
	rba.Pause ();
	StopTime ();
	paletteManager.DisableEffect ();
	GameFlushInputs ();
#if defined (FORCE_FEEDBACK)
	if (TactileStick)
		DisableForces();
#endif
	}
}

//------------------------------------------------------------------------------

void ResumeGame (void)
{
GameFlushInputs ();
paletteManager.EnableEffect ();
StartTime (0);
if (redbook.Playing ())
	rba.Resume ();
audio.ResumeAll ();
gameStates.render.cockpit.nShieldFlash = 0;
gameData.app.bGamePaused = 0;
}

//------------------------------------------------------------------------------

void DoShowNetGameHelp (void);

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int DoGamePause (void)
{
	int			key = 0;
	char			szPauseMsg [1000];
	char			szTotalTime [9], szLevelTime [9];

if (gameData.app.bGamePaused) {		//unpause!
	gameData.app.bGamePaused = 0;
	gameStates.app.bEnterGame = 1;
#if defined (FORCE_FEEDBACK)
	if (TactileStick)
		EnableForces();
#endif
	return KEY_PAUSE;
	}

if (IsNetworkGame) {
	 DoShowNetGameHelp ();
    return (KEY_PAUSE);
	}
else if (IsMultiGame) {
	HUDInitMessage (TXT_MODEM_PAUSE);
	return (KEY_PAUSE);
	}
PauseGame ();

formatTime (szTotalTime, X2I (LOCALPLAYER.timeTotal) + LOCALPLAYER.hoursTotal * 3600);
formatTime (szLevelTime, X2I (LOCALPLAYER.timeLevel) + LOCALPLAYER.hoursLevel * 3600);

if (gameData.demo.nState != ND_STATE_PLAYBACK)
	sprintf (szPauseMsg, TXT_PAUSE_MSG1, GAMETEXT (332 + gameStates.app.nDifficultyLevel), LOCALPLAYER.hostages.nOnBoard, szLevelTime, szTotalTime);
else
	sprintf (szPauseMsg, TXT_PAUSE_MSG2, GAMETEXT (332 + gameStates.app.nDifficultyLevel), LOCALPLAYER.hostages.nOnBoard);

CMenu	m (5);

char* pszToken = strtok (szPauseMsg + strlen ("PAUSE\n\n"), "\n");
while (pszToken) {
	m.AddText ("", pszToken);
	pszToken = strtok (NULL, "\n");
	}

#if 1

key = m.Menu (NULL, "PAUSE");

#else 

int bScreenChanged;

SetPopupScreenMode ();
if (!gameOpts->menus.nStyle) {
	gameStates.menus.nInMenu++;
	GameRenderFrame ();
	gameStates.menus.nInMenu--;
	}
messageBox.Show (pszPauseMsg = szPauseMsg, false);	
GrabMouse (0, 0);
while (gameData.app.bGamePaused) {
	if (!(gameOpts->menus.nStyle && gameStates.app.bGameRunning))
		key = KeyGetChar ();
	else {
		gameStates.menus.nInMenu++;
		while (!(key = KeyInKey ())) {
			GameRenderFrame ();
			messageBox.Render ();
			G3_SLEEP (1);
			}
		gameStates.menus.nInMenu--;
		}
		bScreenChanged = HandleSystemKey (key);
		if (bScreenChanged) {
			GameRenderFrame ();
			messageBox.Render ();
			}
	}
messageBox.Clear ();

#endif

GrabMouse (1, 0);
ResumeGame ();
return key;
}

//------------------------------------------------------------------------------
//switch a cockpit window to the next function
int SelectNextWindowFunction (int nWindow)
{
switch (gameStates.render.cockpit.n3DView [nWindow]) {
	case CV_NONE:
		gameStates.render.cockpit.n3DView [nWindow] = CV_REAR;
		break;

	case CV_REAR:
		if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
		    (!IsMultiGame || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_RADAR_TOPDOWN;
			break;
			}

	case CV_RADAR_TOPDOWN:
		if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
		    (!IsMultiGame || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_RADAR_HEADSUP;
			break;
			}

	case CV_RADAR_HEADSUP:
		if (FindEscort()) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_ESCORT;
			break;
			}

		//if no ecort, fall through
	case CV_ESCORT:
		gameStates.render.cockpit.nCoopPlayerView [nWindow] = -1;		//force first CPlayerData
		//fall through

	case CV_COOP:
		markerManager.SetViewer (nWindow, -1);
		if (IsCoopGame || IsTeamGame) {
			gameStates.render.cockpit.n3DView [nWindow] = CV_COOP;
			while (1) {
				gameStates.render.cockpit.nCoopPlayerView [nWindow]++;
				if (gameStates.render.cockpit.nCoopPlayerView [nWindow] == gameData.multiplayer.nPlayers) {
					gameStates.render.cockpit.n3DView [nWindow] = CV_MARKER;
					goto case_marker;
					}
				if (gameStates.render.cockpit.nCoopPlayerView [nWindow] == N_LOCALPLAYER)
					continue;

				if (IsCoopGame)
					break;
				else if (GetTeam (gameStates.render.cockpit.nCoopPlayerView [nWindow]) == GetTeam(N_LOCALPLAYER))
					break;
				}
			break;
			}
		//if not multi, fall through
	case CV_MARKER:
	case_marker:;
		if (!IsMultiGame || IsCoopGame || netGame.m_info.bAllowMarkerView) {	//anarchy only
			gameStates.render.cockpit.n3DView [nWindow] = CV_MARKER;
			if (markerManager.Viewer (nWindow) == -1)
				markerManager.SetViewer (nWindow, N_LOCALPLAYER * 3);
			else if (markerManager.Viewer (nWindow) < N_LOCALPLAYER * 3 + markerManager.MaxDrop ())
				markerManager.SetViewer (nWindow, markerManager.Viewer (nWindow) + 1);
			else
				gameStates.render.cockpit.n3DView [nWindow] = CV_NONE;
		}
		else
			gameStates.render.cockpit.n3DView [nWindow] = CV_NONE;
		break;
	}
SavePlayerProfile();
return 1;	 //bScreenChanged
}

//	Testing functions ----------------------------------------------------------

#if DBG
void SpeedtestInit(void)
{
	gameData.speedtest.nStartTime = TimerGetFixedSeconds();
	gameData.speedtest.bOn = 1;
	gameData.speedtest.nSegment = 0;
	gameData.speedtest.nSide = 0;
	gameData.speedtest.nFrameStart = gameData.app.nFrameCount;
#if TRACE
	console.printf (CON_DBG, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", gameData.segs.nLastSegment+1);
#endif
}

//------------------------------------------------------------------------------

void SpeedtestFrame(void)
{
	CFixVector	view_dir, center_point;

	gameData.speedtest.nSide=gameData.speedtest.nSegment % SEGMENT_SIDE_COUNT;

	gameData.objs.viewerP->info.position.vPos = SEGMENTS [gameData.speedtest.nSegment].Center ();
	gameData.objs.viewerP->info.position.vPos.v.coord.x += 0x10;	
	gameData.objs.viewerP->info.position.vPos.v.coord.y -= 0x10;	
	gameData.objs.viewerP->info.position.vPos.v.coord.z += 0x17;

	gameData.objs.viewerP->RelinkToSeg (gameData.speedtest.nSegment);
	center_point = SEGMENTS [gameData.speedtest.nSegment].SideCenter (gameData.speedtest.nSide);
	CFixVector::NormalizedDir(view_dir, center_point, gameData.objs.viewerP->info.position.vPos);
	//gameData.objs.viewerP->info.position.mOrient = CFixMatrix::Create(view_dir, NULL, NULL);
	gameData.objs.viewerP->info.position.mOrient = CFixMatrix::CreateF(view_dir);
	if (((gameData.app.nFrameCount - gameData.speedtest.nFrameStart) % 10) == 0) {
#if TRACE
		console.printf (CON_DBG, ".");
#endif
		}
	gameData.speedtest.nSegment++;

	if (gameData.speedtest.nSegment > gameData.segs.nLastSegment) {
		char    msg[128];

		sprintf(msg, TXT_SPEEDTEST, 
			gameData.app.nFrameCount-gameData.speedtest.nFrameStart, 
			X2F(TimerGetFixedSeconds() - gameData.speedtest.nStartTime), 
			(double) (gameData.app.nFrameCount-gameData.speedtest.nFrameStart) / X2F(TimerGetFixedSeconds() - gameData.speedtest.nStartTime));
#if TRACE
		console.printf (CON_DBG, "%s", msg);
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
