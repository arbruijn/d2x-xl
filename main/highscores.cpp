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

static int xOffs = 0, yOffs = 0;

CScoreTable scoreTable;

//-----------------------------------------------------------------------------

void CScoreTable::DrawItem (int  i)
{
	int j, x, y;
	char temp [10];

	y = LHY (50+i*9) + yOffs;

	// Print CPlayerData name.

GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)) + xOffs, y, "%s", gameData.multiplayer.players [m_sorted [i]].callsign);
  if (!gameData.app.GameMode (GM_MODEM | GM_SERIAL))
   GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)-15),y,"%c",szConditionLetters [gameData.multiplayer.players [m_sorted [i]].connected]);
   
for (j = 0; j < gameData.multiplayer.nPlayers; j++) {
	x = LHX (70 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + j * 25) + xOffs;
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

if (gameData.multiplayer.players [m_sorted [i]].netKilledTotal + gameData.multiplayer.players [m_sorted [i]].netKillsTotal == 0)
	sprintf (temp,"N/A");
else
   sprintf (temp,"%d%%", (int) ((double) gameData.multiplayer.players [m_sorted [i]].netKillsTotal / 
										  (double) (gameData.multiplayer.players [m_sorted [i]].netKilledTotal + gameData.multiplayer.players [m_sorted [i]].netKillsTotal) * 100.0));	
x = LHX (60 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + gameData.multiplayer.nPlayers*25) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25),1, 0, 0);
GrPrintF (NULL, x ,y,"%4d/%s",gameData.multiplayer.players [m_sorted [i]].netKillsTotal,temp);
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawCoopItem (int  i)
{
	int  x, y = LHY (50+i*9) + yOffs;

// Print CPlayerData name.
GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)) + xOffs, y, "%s", gameData.multiplayer.players [m_sorted [i]].callsign);
GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)-15) + xOffs,y,"%c",szConditionLetters [gameData.multiplayer.players [m_sorted [i]].connected]);
x = CENTERSCREEN + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [m_sorted [i]].score);
x = CENTERSCREEN+LHX (50) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [m_sorted [i]].netKilledTotal);
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawNames (void)
{
	int j, x;
	int color;

if (gameData.score.bNoMovieMessage) {
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}

for (j = 0; j < gameData.multiplayer.nPlayers; j++) {
	if (IsTeamGame)
		color = GetTeam (m_sorted [j]);
	else
		color = m_sorted [j];
	x = LHX (70 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + j*25) + xOffs;
	if (gameData.multiplayer.players [m_sorted [j]].connected == CONNECT_DISCONNECTED)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
   else
      fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
	GrPrintF (NULL, x, LHY (40) + yOffs, "%c", gameData.multiplayer.players [m_sorted [j]].callsign [0]);
	}
x = LHX (72 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + gameData.multiplayer.nPlayers*25) + xOffs;
fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
GrPrintF (NULL, x, LHY (40) + yOffs, "K/E");
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawCoopNames (void)
{
if (gameData.score.bNoMovieMessage) {
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
	int	y;
	int	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + 72 + 35) + yOffs;
         
	   			
fontManager.SetColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
fontManager.Current ()->StringSize ("P-Playing E-Escaped D-Died", sw, sh, aw);
if (! (gameData.app.GameMode (GM_MODEM | GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y+= (sh+5);
fontManager.Current ()->StringSize ("V-Viewing scores W-Waiting", sw, sh, aw);
if (! (gameData.app.GameMode (GM_MODEM | GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected == CONNECT_ADVANCE_LEVEL) {
   fontManager.Current ()->StringSize ("Waiting for other players...",sw, sh, aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"Waiting for other players...");
   }
else {
   fontManager.Current ()->StringSize (TXT_PRESS_ANY_KEY2, sw, sh, aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
   }
if (gameData.reactor.countdown.nSecsLeft <=0)
   DrawReactor (TXT_REACTOR_EXPLODED);
else {
   sprintf (reinterpret_cast<char*> (&reactor_message), "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactor.countdown.nSecsLeft, TXT_SECONDS);
   DrawReactor (reinterpret_cast<char*> (&reactor_message));
   }
if (gameData.app.GameMode (GM_HOARD | GM_ENTROPY)) 
	DrawChampion ();
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawCoopDeaths (void)
{
	int	j, x, y;
	int	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + gameData.multiplayer.nPlayers * 9) + yOffs;
//	fontManager.SetColor (gr_getcolor (playerColors [j].r,playerColors [j].g,playerColors [j].b),-1);
fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
x = CENTERSCREEN+LHX (50) + xOffs;
GrPrintF (NULL, x, y, TXT_DEATHS);
for (j = 0; j < gameData.multiplayer.nPlayers; j++) {
	x = CENTERSCREEN + LHX (50) + xOffs;
	GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [m_sorted [j]].netKilledTotal);
	}
y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
fontManager.Current ()->StringSize ("P-Playing E-Escaped D-Died", sw, sh, aw);
if (! (gameData.app.GameMode (GM_MODEM | GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y += (sh+5);
fontManager.Current ()->StringSize ("V-Viewing scores W-Waiting", sw, sh, aw);
if (! (gameData.app.GameMode (GM_MODEM | GM_SERIAL)))
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected == CONNECT_ADVANCE_LEVEL) {
	fontManager.Current ()->StringSize ("Waiting for other players...",sw, sh, aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"Waiting for other players...");
	}
else {
	fontManager.Current ()->StringSize (TXT_PRESS_ANY_KEY2, sw, sh, aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
	}
if (gameData.reactor.countdown.nSecsLeft <=0)
	DrawReactor (TXT_REACTOR_EXPLODED);
else {
	sprintf (reinterpret_cast<char*> (&reactor_message), "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactor.countdown.nSecsLeft, TXT_SECONDS);
	DrawReactor (reinterpret_cast<char*> (&reactor_message));
	}
}

//-----------------------------------------------------------------------------

void CScoreTable::DrawReactor (const char *message)
 {
  static char oldmessage [50]={0};
  int sw, sh, aw;

if (gameData.app.GameMode (GM_MODEM | GM_SERIAL))
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
  int sw, sh, aw;
  char message [80];

if (IsHoardGame != 0)
	return;
if ((gameData.app.GameMode (GM_MODEM | GM_SERIAL)) != 0)
	return;
if (gameData.score.nChampion == -1)
	strcpy (message,TXT_NO_RECORD);
else
	sprintf (message, TXT_BEST_RECORD, gameData.multiplayer.players [gameData.score.nChampion].callsign, gameData.score.nHighscore);
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
fontManager.Current ()->StringSize (message, sw, sh, aw);
GrPrintF (NULL, CENTERSCREEN - sw / 2, LHY (55+72+3), message);
}


//-----------------------------------------------------------------------------

void CScoreTable::Render (void)
{
	int			i, color;

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
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (IsTeamGame)
		color = GetTeam (m_sorted [i]);
	else
		color = m_sorted [i];
	if (!gameData.multiplayer.players [m_sorted [i]].connected)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
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
	int i, color;

MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), "COOPERATIVE SUMMARY");
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (m_sorted);
m_background.Activate ();
gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
DrawCoopNames ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	color = m_sorted [i];
	if (gameData.multiplayer.players [m_sorted [i]].connected == CONNECT_DISCONNECTED)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
	DrawCoopItem (i);
	}
DrawDeaths ();
m_background.Deactivate ();
//paletteManager.ResumeEffect ();
ogl.Update (1);
}

//-----------------------------------------------------------------------------

void CScoreTable::Cleanup (int bQuit)
{
if (m_bNetwork)
	NetworkSendEndLevelPacket ();
if (bQuit) {
	CONNECT (N_LOCALPLAYER, CONNECT_DISCONNECTED);
	MultiLeaveGame ();
	}
gameData.score.bNoMovieMessage = 0;
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

int CScoreTable::Input (void)
{
	int i, nChoice;

for (i = 0; i < 4; i++)
	if (JoyGetButtonDownCnt (i))
		return Exit () ? -1 : 1;

for (i = 0; i < 3; i++)
	if (MouseButtonDownCount (i))
		return Exit () ? -1 : 1;

//see if redbook song needs to be restarted
int k = KeyInKey ();
switch (k) {
	case KEY_ENTER:
	case KEY_SPACEBAR:
		if ((gameData.app.GameMode (GM_SERIAL | GM_MODEM)) != 0)
			return IAmGameHost ();
		if (Exit ())
			return -1;
		return 1;
		
	case KEY_ESC:
		if (IsNetworkGame) {
			gameData.multiplayer.xStartAbortMenuTime = TimerGetApproxSeconds ();
			int nInMenu = gameStates.menus.nInMenu;
			gameStates.menus.nInMenu = 0;
			nChoice = InfoBox (NULL, NetworkEndLevelPoll3, BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			gameStates.menus.nInMenu = nInMenu;
			}
		else {
			int nInMenu = gameStates.menus.nInMenu;
			gameStates.menus.nInMenu = 0;
			int choice = InfoBox (NULL, NULL, BG_STANDARD, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			gameStates.menus.nInMenu = nInMenu;
			}
		if (nChoice == 0) {
			Cleanup (1);
			return -1;
			}
		gameData.score.nKillsChanged = 1;
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

int CScoreTable::WaitForPlayers (void)
{
int nEscaped = 0;
int nReady = 0;
int nConnected = 0;
int bServer = gameStates.multi.bServer [0];

if (!networkThread.SemWait ())
	networkThread.CheckPlayerTimeouts (); // wait for eventual player timeout checking to complete
for (int nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {
	// check timeouts for idle players
#if 0 // handled by networkThread.CheckPlayerTimeouts () now
	if ((nPlayer != N_LOCALPLAYER) && gameData.multiplayer.players [nPlayer].connected) {
		if (SDL_GetTicks () > (uint) networkData.nLastPacketTime [nPlayer] + ((gameData.multiplayer.players [nPlayer].connected == CONNECT_ADVANCE_LEVEL) ? LEVEL_LOAD_TIME : ENDLEVEL_IDLE_TIME)) {
			CONNECT (nPlayer, CONNECT_DISCONNECTED);
			if ((gameStates.multi.nGameType != UDP_GAME) || IAmGameHost ())
				NetworkSendEndLevelSub (nPlayer);
			}
		}
#endif

	if (gameData.multiplayer.players [nPlayer].connected != m_oldStates [nPlayer]) {
		if (szConditionLetters [gameData.multiplayer.players [nPlayer].connected] != szConditionLetters [m_oldStates [nPlayer]])
			gameData.score.nKillsChanged = 1;
		m_oldStates [nPlayer] = gameData.multiplayer.players [nPlayer].connected;
		NetworkSendEndLevelPacket ();
		}

	if (gameData.multiplayer.players [nPlayer].connected != CONNECT_PLAYING) {
		nEscaped++;
		if ((gameData.multiplayer.players [nPlayer].connected == CONNECT_DISCONNECTED) || (gameData.multiplayer.players [nPlayer].connected == CONNECT_ADVANCE_LEVEL))
			nReady++;
		}

	if (gameData.multiplayer.players [nPlayer].connected != CONNECT_DISCONNECTED)
		nConnected++;

	if (nReady >= gameData.multiplayer.nPlayers) {
		networkThread.SemPost ();
		return 1;
		}

	if (nEscaped >= gameData.multiplayer.nPlayers)
		gameData.reactor.countdown.nSecsLeft = -1;
	if (m_nPrevSecsLeft != gameData.reactor.countdown.nSecsLeft) {
		m_nPrevSecsLeft = gameData.reactor.countdown.nSecsLeft;
		gameData.score.nKillsChanged = 1;
		}
	if (gameData.score.nKillsChanged) {
		Render ();
		gameData.score.nKillsChanged = 0;
		}
	}
networkThread.SemPost ();

if (!bServer && (nConnected < 2)) {
	int nInMenu = gameStates.menus.nInMenu;
	gameStates.menus.nInMenu = 0;
	int choice = InfoBox (NULL, NULL, BG_STANDARD, 1, TXT_YES, TXT_CONNECT_LOST);
	gameStates.menus.nInMenu = nInMenu;
	Cleanup (1);
	return -1;
	}

return 0;
}

//-----------------------------------------------------------------------------

void CScoreTable::Display (void)
{											 
   int	i;
	int	key;
	int	bRedraw = 0;
	uint	t0 = SDL_GetTicks ();

m_bNetwork = IsNetworkGame != 0;
m_nPrevSecsLeft = -1;
gameStates.menus.nInMenu++;
gameStates.app.bGameRunning = 0;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	audio.DestroyObjectSound (gameData.multiplayer.players [i].nObject);

SetScreenMode (SCREEN_MENU);
int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
backgroundManager.Setup (m_background, 640, 480, BG_TOPMENU, BG_STARS);
gameData.score.bWaitingForOthers = 0;
//@@GrPaletteFadeIn (grPalette,32, 0);
GameFlushInputs ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	m_oldStates [i] = gameData.multiplayer.players [i].connected;
if (m_bNetwork)
	NetworkEndLevel (&key);


CMenu m (1);
m.AddGauge ("", "", -1, 1000); //dummy for NetworkEndLevelPoll2()

for (;;) {
	if (!bRedraw || (ogl.m_states.nDrawBuffer == GL_BACK)) {
		Render ();
		bRedraw = 1;
		}
	gameData.score.nKillsChanged = 0;

	if (m_bNetwork)
		NetworkEndLevelPoll2 (m, key, 0, 0); // check the states of the other players

	redbook.CheckRepeat ();
	i = Input ();
	if (i < 0)
		return;
	if ((i > 1) && !m_bNetwork)
		break;

	uint t = SDL_GetTicks ();
	if ((t >= t0 + MAX_VIEW_TIME) && (LOCALPLAYER.GetConnected () != CONNECT_ADVANCE_LEVEL)) {
		if (LAST_OEM_LEVEL) {
			Cleanup (1);
			return;
			}
		if ((gameData.app.GameMode (GM_SERIAL | GM_MODEM)) != 0) 
			break;
		CONNECT (N_LOCALPLAYER, CONNECT_ADVANCE_LEVEL); // player is idling in score screen for MAX_VIEW_TIMES secs 
		}

	i = WaitForPlayers ();
	if (i < 0)
		return;
	if ((i > 0) && (LOCALPLAYER.GetConnected () == CONNECT_ADVANCE_LEVEL))
		break;
	}

NetworkSendEndLevelPacket ();
// Restore background and exit
paletteManager.DisableEffect ();
GameFlushInputs ();
gameData.SetStereoOffsetType (nOffsetSave);
Cleanup (0);
}

//-----------------------------------------------------------------------------
//eof
