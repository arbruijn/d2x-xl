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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "descent.h"
#include "error.h"
#include "key.h"
#include "u_mem.h"
#include "menu.h"
#include "netmenu.h"
#include "screens.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "highscores.h"
#include "cockpit.h"
#include "gamefont.h"
#include "network.h"
#include "network_lib.h"
#include "menubackground.h"
#include "songs.h"

#if 0 //DBG
#define MAX_VIEW_TIME   		150000
#	define ENDLEVEL_IDLE_TIME	100000
#	define LEVEL_LOAD_TIME		180000
#else
#	define MAX_VIEW_TIME			15000
#	define ENDLEVEL_IDLE_TIME	10000
#	define LEVEL_LOAD_TIME		180000
#endif

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25))/2)
#define CENTERSCREEN (gameStates.menus.bHires?320:160)

char szConditionLetters [] = {' ','P','E','D','E','E','V','W'};

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

static int32_t xOffs = 0, yOffs = 0;

CScoreTable scoreTable;

//-----------------------------------------------------------------------------

void CScoreTable::DrawItem (int32_t  i)
{
	int32_t j, x, y;
	char temp [10];

	y = LHY (50+i*9) + yOffs;

	// Print CPlayerData name.

GrPrintF (NULL, LHX (CENTERING_OFFSET (N_PLAYERS)) + xOffs, y, "%s", PLAYER (m_sorted [i]).callsign);
  if (!gameData.appData.GameMode (GM_MODEM | GM_SERIAL))
   GrPrintF (NULL, LHX (CENTERING_OFFSET (N_PLAYERS)-15),y,"%c",szConditionLetters [PLAYER (m_sorted [i]).connected]);
   
for (j = 0; j < N_PLAYERS; j++) {
	x = LHX (70 + CENTERING_OFFSET (N_PLAYERS) + j * 25) + xOffs;
	if (m_sorted [i] == m_sorted [j]) {
		if (gameData.multigame.score.matrix [m_sorted [i]][m_sorted [j]] == 0) {
			fontManager.SetColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.score.matrix [m_sorted [i]][m_sorted [j]]);
			} 
		else {
			fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (NULL, x, y, "-%d", gameData.multigame.score.matrix [m_sorted [i]][m_sorted [j]]);
			}
		} 
	else {
		if (gameData.multigame.score.matrix [m_sorted [i]][m_sorted [j]] <= 0) {
			fontManager.SetColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.score.matrix [m_sorted [i]][m_sorted [j]]);
			} 
		else {
			fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.score.matrix [m_sorted [i]][m_sorted [j]]);
			}
		}
	}

if (PLAYER (m_sorted [i]).netKilledTotal + PLAYER (m_sorted [i]).netKillsTotal == 0)
	sprintf (temp,"N/A");
else
   sprintf (temp,"%d%%", (int32_t) ((double) PLAYER (m_sorted [i]).netKillsTotal / (double) (PLAYER (m_sorted [i]).netKilledTotal + PLAYER (m_sorted [i]).netKillsTotal) * 100.0));	
x = LHX (60 + CENTERING_OFFSET (N_PLAYERS) + N_PLAYERS*25) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25),1, 0, 0);
GrPrintF (NULL, x ,y,"%4d/%s",PLAYER (m_sorted [i]).netKillsTotal,temp);
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawCoopItem (int32_t  i)
{
	int32_t  x, y = LHY (50+i*9) + yOffs;

// Print CPlayerData name.
GrPrintF (NULL, LHX (CENTERING_OFFSET (N_PLAYERS)) + xOffs, y, "%s", PLAYER (m_sorted [i]).callsign);
GrPrintF (NULL, LHX (CENTERING_OFFSET (N_PLAYERS)-15) + xOffs,y,"%c",szConditionLetters [PLAYER (m_sorted [i]).connected]);
x = CENTERSCREEN + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", PLAYER (m_sorted [i]).score);
x = CENTERSCREEN+LHX (50) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", PLAYER (m_sorted [i]).netKilledTotal);
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawNames (void)
{
	int32_t j, x;
	int32_t color;

if (gameData.scoreData.bNoMovieMessage) {
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}

for (j = 0; j < N_PLAYERS; j++) {
	if (IsTeamGame)
		color = GetTeam (m_sorted [j]);
	else
		color = m_sorted [j];
	x = LHX (70 + CENTERING_OFFSET (N_PLAYERS) + j*25) + xOffs;
	if (PLAYER (m_sorted [j]).connected == CONNECT_DISCONNECTED)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
   else
      fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
	GrPrintF (NULL, x, LHY (40) + yOffs, "%c", PLAYER (m_sorted [j]).callsign [0]);
	}
x = LHX (72 + CENTERING_OFFSET (N_PLAYERS) + N_PLAYERS*25) + xOffs;
fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
GrPrintF (NULL, x, LHY (40) + yOffs, "K/E");
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawCoopNames (void)
{
if (gameData.scoreData.bNoMovieMessage) {
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}
fontManager.SetColorRGBi (RGBA_PAL2 (63,31,31), 1, 0, 0);
GrPrintF (NULL, CENTERSCREEN, LHY (40), "SCORE");
fontManager.SetColorRGBi (RGBA_PAL2 (63,31,31), 1, 0, 0);
GrPrintF (NULL, CENTERSCREEN+LHX (50), LHY (40), "DEATHS");
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawDeaths (void)
{
	int32_t	y;
	int32_t	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + 72 + 35) + yOffs;
         
	   			
fontManager.SetColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
fontManager.Current ()->StringSize ("P-Playing E-Escaped D-Died", sw, sh, aw);
if (! (gameData.appData.GameMode (GM_MODEM | GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y+= (sh+5);
fontManager.Current ()->StringSize ("V-Viewing scores W-Waiting", sw, sh, aw);
if (! (gameData.appData.GameMode (GM_MODEM | GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected == CONNECT_ADVANCE_LEVEL) {
   fontManager.Current ()->StringSize (TXT_CLIENT_WAIT, sw, sh, aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_CLIENT_WAIT);
   }
else {
   fontManager.Current ()->StringSize (TXT_PRESS_ANY_KEY2, sw, sh, aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
   }
if (gameData.reactorData.countdown.nSecsLeft <=0)
   DrawReactor (TXT_REACTOR_EXPLODED);
else {
   sprintf (reinterpret_cast<char*> (&reactor_message), "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactorData.countdown.nSecsLeft, TXT_SECONDS);
   DrawReactor (reinterpret_cast<char*> (&reactor_message));
   }
if (gameData.appData.GameMode (GM_HOARD | GM_ENTROPY)) 
	DrawChampion ();
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawCoopDeaths (void)
{
	int32_t	j, x, y;
	int32_t	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + N_PLAYERS * 9) + yOffs;
//	fontManager.SetColor (gr_getcolor (playerColors [j].r,playerColors [j].g,playerColors [j].b),-1);
fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
x = CENTERSCREEN+LHX (50) + xOffs;
GrPrintF (NULL, x, y, TXT_DEATHS);
for (j = 0; j < N_PLAYERS; j++) {
	x = CENTERSCREEN + LHX (50) + xOffs;
	GrPrintF (NULL, x, y, "%d", PLAYER (m_sorted [j]).netKilledTotal);
	}
y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
fontManager.Current ()->StringSize ("P-Playing E-Escaped D-Died", sw, sh, aw);
if (! (gameData.appData.GameMode (GM_MODEM | GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y += (sh+5);
fontManager.Current ()->StringSize ("V-Viewing scores W-Waiting", sw, sh, aw);
if (! (gameData.appData.GameMode (GM_MODEM | GM_SERIAL)))
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected == CONNECT_ADVANCE_LEVEL) {
	fontManager.Current ()->StringSize (TXT_CLIENT_WAIT, sw, sh, aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_CLIENT_WAIT);
	}
else {
	fontManager.Current ()->StringSize (TXT_PRESS_ANY_KEY2, sw, sh, aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
	}
if (gameData.reactorData.countdown.nSecsLeft <=0)
	DrawReactor (TXT_REACTOR_EXPLODED);
else {
	sprintf (reinterpret_cast<char*> (&reactor_message), "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactorData.countdown.nSecsLeft, TXT_SECONDS);
	DrawReactor (reinterpret_cast<char*> (&reactor_message));
	}
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawReactor (const char *message)
 {
  static char oldmessage [50]={0};
  int32_t sw, sh, aw;

if (gameData.appData.GameMode (GM_MODEM | GM_SERIAL))
	return;
fontManager.SetCurrent (SMALL_FONT);
if (oldmessage [0]!=0) {
	fontManager.SetColorRGBi (RGBA_PAL2 (0, 1, 0), 1, 0, 0);
	fontManager.Current ()->StringSize (oldmessage, sw, sh, aw);
	}
fontManager.SetColorRGBi (RGBA_PAL2 (0, 32, 63), 1, 0, 0);
fontManager.Current ()->StringSize (message, sw, sh, aw);
GrPrintF (NULL, CENTERSCREEN- (sw/2), LHY (55+72+12), message);
strcpy (reinterpret_cast<char*> (&oldmessage), message);
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawChampion (void)
{
  int32_t sw, sh, aw;
  char message [80];

if (IsHoardGame != 0)
	return;
if ((gameData.appData.GameMode (GM_MODEM | GM_SERIAL)) != 0)
	return;
if (gameData.scoreData.nChampion == -1)
	strcpy (message,TXT_NO_RECORD);
else
	sprintf (message, TXT_BEST_RECORD, PLAYER (gameData.scoreData.nChampion).callsign, gameData.scoreData.nHighscore);
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
fontManager.Current ()->StringSize (message, sw, sh, aw);
GrPrintF (NULL, CENTERSCREEN - sw / 2, LHY (55+72+3), message);
}

//-----------------------------------------------------------------------------

bool CScoreTable::CapFramerate (void)
{
	static CTimeout to (30);

if (to.Expired ())
	return false;
G3_SLEEP (1);
return true;
}

//-----------------------------------------------------------------------------

void CScoreTable::Render (void)
{
if (CapFramerate ())
	return;

backgroundManager.Draw (&m_background);

if (IsCoopGame) {
	RenderCoop ();
	return;
	}
MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), TXT_KILL_MATRIX_TITLE);
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (m_sorted);

m_background.Activate ();
gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
DrawNames ();
for (int32_t i = 0; i < N_PLAYERS; i++) {
	if (!PLAYER (m_sorted [i]).connected)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else {
		int32_t color = IsTeamGame ? GetTeam (m_sorted [i]) : m_sorted [i];
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
		}
	DrawItem (i);
	}
DrawDeaths ();
m_background.Deactivate ();
ogl.Update (1);
//paletteManager.ResumeEffect ();
}

//-----------------------------------------------------------------------------

void CScoreTable::RenderCoop (void)
{
MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), "COOPERATIVE SUMMARY");
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (m_sorted);
m_background.Activate ();
gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
DrawCoopNames ();
for (int32_t i = 0; i < N_PLAYERS; i++) {
	if (PLAYER (m_sorted [i]).connected == CONNECT_DISCONNECTED)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else {
		int32_t color = m_sorted [i];
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
		}
	DrawCoopItem (i);
	}
DrawDeaths ();
m_background.Deactivate ();
//paletteManager.ResumeEffect ();
ogl.Update (1);
}

//-----------------------------------------------------------------------------

void CScoreTable::Cleanup (int32_t bQuit)
{
if (m_bNetwork) 
	NetworkSendEndLevelPacket ();
if (bQuit) {
	CONNECT (N_LOCALPLAYER, CONNECT_DISCONNECTED);
	MultiLeaveGame ();
	}
gameData.scoreData.bNoMovieMessage = 0;
backgroundManager.Draw ();
gameStates.menus.nInMenu--;
if (bQuit || ((missionManager.nCurrentLevel >= missionManager.nLastLevel) && !extraGameInfo [IsMultiGame].bRotateLevels))
	longjmp (gameExitPoint, 0);
}

//-----------------------------------------------------------------------------

#define LAST_OEM_LEVEL	IS_D2_OEM && (missionManager.nCurrentLevel == 8)

bool CScoreTable::Exit (void)
{
if (LAST_OEM_LEVEL) {
	Cleanup (1);
	return true;
	}
CONNECT (N_LOCALPLAYER, CONNECT_ADVANCE_LEVEL); // 7 means "I have arrived at the score screen and want to proceed"
if (m_bNetwork)
	NetworkSendEndLevelPacket ();
return false;
}

//-----------------------------------------------------------------------------

int32_t CScoreTable::Input (void)
{
	int32_t i, nChoice;

for (i = 0; i < 4; i++)
	if (JoyGetButtonDownCnt (i))
		return Exit () ? -1 : 1;

for (i = 0; i < 3; i++)
	if (MouseButtonDownCount (i))
		return Exit () ? -1 : 1;

//see if redbook song needs to be restarted
int32_t k = KeyInKey ();
switch (k) {
	case KEY_ENTER:
	case KEY_SPACEBAR:
		if ((gameData.appData.GameMode (GM_SERIAL | GM_MODEM)) != 0)
			return IAmGameHost ();
		if (Exit ())
			return -1;
		return 1;
		
	case KEY_ESC:
		if (IsNetworkGame && !networkThread.Available ()) {
			gameData.multiplayer.xStartAbortMenuTime = TimerGetApproxSeconds ();
			int32_t nInMenu = gameStates.menus.nInMenu;
			gameStates.menus.nInMenu = 0;
			nChoice = InfoBox (NULL, NetworkEndLevelPoll3, BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			gameStates.menus.nInMenu = nInMenu;
			}
		else {
			int32_t nInMenu = gameStates.menus.nInMenu;
			gameStates.menus.nInMenu = 0;
			nChoice = InfoBox (NULL, NULL, BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			gameStates.menus.nInMenu = nInMenu;
			}
		if (nChoice == 0) {
			Cleanup (1);
			return -1;
			}
		gameData.scoreData.nKillsChanged = 1;
		break;

	case KEY_PRINT_SCREEN:
		SaveScreenShot (NULL, 0);
		break;

	default:
		break;
	}
return 0;
}

//-----------------------------------------------------------------------------

int32_t CScoreTable::WaitForPlayers (void)
{
int32_t nEscaped = 0;
int32_t nReady = 0;
int32_t nConnected = 0;
int32_t bServer = gameStates.multi.bServer [0];

if (!networkThread.LockThread ())
	networkThread.CheckPlayerTimeouts (); // wait for eventual player timeout checking to complete
for (int32_t nPlayer = 0; nPlayer < N_PLAYERS; nPlayer++) {
	// check timeouts for idle players
#if 0 // handled by networkThread.CheckPlayerTimeouts () now
	if ((nPlayer != N_LOCALPLAYER) && PLAYER (nPlayer).connected) {
		if (SDL_GetTicks () > (uint32_t) networkData.nLastPacketTime [nPlayer] + ((PLAYER (nPlayer).connected == CONNECT_ADVANCE_LEVEL) ? LEVEL_LOAD_TIME : ENDLEVEL_IDLE_TIME)) {
			CONNECT (nPlayer, CONNECT_DISCONNECTED);
			if ((gameStates.multi.nGameType != UDP_GAME) || IAmGameHost ())
				NetworkSendEndLevelSub (nPlayer);
			}
		}
#endif

	if (PLAYER (nPlayer).connected != m_oldStates [nPlayer]) {
		if (szConditionLetters [PLAYER (nPlayer).connected] != szConditionLetters [m_oldStates [nPlayer]])
			gameData.scoreData.nKillsChanged = 1;
		m_oldStates [nPlayer] = PLAYER (nPlayer).connected;
		NetworkSendEndLevelPacket ();
		}

	if (PLAYER (nPlayer).connected != CONNECT_PLAYING) {
		nEscaped++;
		if ((PLAYER (nPlayer).connected == CONNECT_DISCONNECTED) || 
			 (PLAYER (nPlayer).connected == CONNECT_ADVANCE_LEVEL))
			nReady++;
		}

	if (PLAYER (nPlayer).connected != CONNECT_DISCONNECTED)
		nConnected++;

	if (nReady >= N_PLAYERS) {
		networkThread.UnlockThread ();
		return 1;
		}

	if (nEscaped >= N_PLAYERS)
		gameData.reactorData.countdown.nSecsLeft = -1;
	if (m_nPrevSecsLeft != gameData.reactorData.countdown.nSecsLeft) {
		m_nPrevSecsLeft = gameData.reactorData.countdown.nSecsLeft;
		gameData.scoreData.nKillsChanged = 1;
		}
	if (gameData.scoreData.nKillsChanged) {
		Render ();
		gameData.scoreData.nKillsChanged = 0;
		}
	}
networkThread.UnlockThread ();

if (!bServer && (nConnected < 2)) {
	int32_t nInMenu = gameStates.menus.nInMenu;
	gameStates.menus.nInMenu = 0;
	InfoBox (NULL, NULL, BG_STANDARD, 1, TXT_OK, TXT_CONNECT_LOST, gameData.multiplayer.players [0].callsign);
	gameStates.menus.nInMenu = nInMenu;
	Cleanup (1);
	return -1;
	}

return 0;
}

//-----------------------------------------------------------------------------

void CScoreTable::Display (void)
{											 
   int32_t	i;
	int32_t	key;
	int32_t	bRedraw = 0;
	uint32_t	tIdleTimeout = SDL_GetTicks () + MAX_VIEW_TIME;
	uint32_t tRenderTimeout = SDL_GetTicks () + 40;

m_bNetwork = IsNetworkGame != 0;
m_nPrevSecsLeft = -1;
gameStates.menus.nInMenu++;
gameStates.app.bGameRunning = 0;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	audio.DestroyObjectSound (PLAYER (i).nObject);

SetScreenMode (SCREEN_MENU);
int32_t nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
backgroundManager.Setup (m_background, 640, 480, BG_TOPMENU, BG_STARS);
gameData.scoreData.bWaitingForOthers = 0;
//@@GrPaletteFadeIn (grPalette,32, 0);
GameFlushInputs ();
for (i = 0; i < N_PLAYERS; i++)
	m_oldStates [i] = PLAYER (i).connected;
if (m_bNetwork)
	NetworkEndLevel (&key);

CMenu m (1);
m.AddGauge ("", "", -1, 1000); //dummy for NetworkEndLevelPoll2()

for (;;) {
	uint32_t t = SDL_GetTicks ();
	if (tRenderTimeout > t)
		G3_SLEEP (tRenderTimeout - t);
	tRenderTimeout = SDL_GetTicks () + 40;
	if ((!bRedraw || (ogl.m_states.nDrawBuffer == GL_BACK))) {
		Render ();
		bRedraw = 1;
		}
	gameData.scoreData.nKillsChanged = 0;

	if (m_bNetwork)
		NetworkEndLevelPoll2 (m, key, 0, 0); // check the states of the other players

	redbook.CheckRepeat ();
	i = Input ();
	if (i < 0)
		return;
	if ((i > 1) && !m_bNetwork)
		break;

	if ((SDL_GetTicks () >= tIdleTimeout) && (LOCALPLAYER.GetConnected () != CONNECT_ADVANCE_LEVEL)) {
		if (LAST_OEM_LEVEL) {
			Cleanup (1);
			return;
			}
		if ((gameData.appData.GameMode (GM_SERIAL | GM_MODEM)) != 0) 
			break;
		CONNECT (N_LOCALPLAYER, CONNECT_ADVANCE_LEVEL); // player is idling in score screen for MAX_VIEW_TIMES secs 
		if (m_bNetwork)
			NetworkSendEndLevelPacket ();
		}

	i = WaitForPlayers ();
	if (i < 0)
		return;
	if ((i > 0) && (LOCALPLAYER.GetConnected () == CONNECT_ADVANCE_LEVEL))
		break;
	}

CONNECT (N_LOCALPLAYER, CONNECT_ADVANCE_LEVEL); // player is idling in score screen for MAX_VIEW_TIMES secs 
for (i = 0; i < 3; i++) {
	NetworkSendEndLevelPacket ();
	G3_SLEEP (33);
	}
// Restore background and exit
paletteManager.DisableEffect ();
GameFlushInputs ();
gameData.SetStereoOffsetType (nOffsetSave);
Cleanup (0);
}

//-----------------------------------------------------------------------------
//eof
