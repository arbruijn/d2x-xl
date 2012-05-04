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

#define MAX_VIEW_TIME   	15000
#define ENDLEVEL_IDLE_TIME	10000

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
	if (gameData.multiplayer.players [m_sorted [j]].connected == 0)
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
	int	y,x;
	int	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
         
	   			
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
if (LOCALPLAYER.connected == 7) {
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
if (LOCALPLAYER.connected == 7) {
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
	int i, color;

backgroundManager.Redraw ();
xOffs = (CCanvas::Current ()->Width () - 640) / 2;
yOffs = (CCanvas::Current ()->Height () - 480) / 2;
if (xOffs < 0)
	xOffs = 0;
if (yOffs < 0)
	yOffs = 0;
if (IsCoopGame) {
	RenderCoop ();
	return;
	}
MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), TXT_KILL_MATRIX_TITLE, NULL);
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (m_sorted);
DrawNames ();
for (i= 0; i<gameData.multiplayer.nPlayers; i++) {
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
GrUpdate (1);
//paletteManager.ResumeEffect ();
}

//-----------------------------------------------------------------------------

void CScoreTable::RenderCoop (void)
{
	int i, color;

MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), "COOPERATIVE SUMMARY", NULL);
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (m_sorted);
DrawCoopNames ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	color = m_sorted [i];
	if (gameData.multiplayer.players [m_sorted [i]].connected == 0)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()), 1, 0, 0);
	DrawCoopItem (i);
	}
DrawDeaths ();
CCanvas::SetCurrent (NULL);
//paletteManager.ResumeEffect ();
GrUpdate (1);
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
backgroundManager.Remove ();
gameStates.menus.nInMenu--;
if ((missionManager.nCurrentLevel >= missionManager.nLastLevel) &&
	 !extraGameInfo [IsMultiGame].bRotateLevels)
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
	int i;

for (i = 0; i < 4; i++)
	if (JoyGetButtonDownCnt (i) && Exit ())
		return -1;

for (i = 0; i < 3; i++)
	if (MouseButtonDownCount (i) && Exit ())
		return -1;

//see if redbook song needs to be restarted
int k = KeyInKey ();
switch (k) {
	case KEY_ENTER:
	case KEY_SPACEBAR:
		if ((gameData.app.GameMode (GM_SERIAL | GM_MODEM)) != 0)
			return 1;
		if (Exit ())
			return -1;
		break;

	case KEY_ESC:
		if (IsNetworkGame) {
			gameData.multiplayer.xStartAbortMenuTime = TimerGetApproxSeconds ();
			int nInMenu = gameStates.menus.nInMenu;
			gameStates.menus.nInMenu = 0;
			MsgBox (NULL, NetworkEndLevelPoll3, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			gameStates.menus.nInMenu = nInMenu;
			}
		else {
			int nInMenu = gameStates.menus.nInMenu;
			gameStates.menus.nInMenu = 0;
			int choice = MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			gameStates.menus.nInMenu = nInMenu;
			if (choice == 0) {
				Cleanup (1);
				return -1;
				}
			gameData.score.nKillsChanged = 1;
			}
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
m_nEscaped = m_nReady = 0;
for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (gameData.multiplayer.players [i].connected && (i != N_LOCALPLAYER)) {
	// Check timeout for idle players
		if (SDL_GetTicks () > (uint) networkData.nLastPacketTime [i] + ENDLEVEL_IDLE_TIME) {
			CONNECT (i, CONNECT_DISCONNECTED);
			if ((gameStates.multi.nGameType != UDP_GAME) || IAmGameHost ())
				NetworkSendEndLevelSub (i);
			}
		}

	if (gameData.multiplayer.players [i].connected != m_oldStates [i]) {
		if (szConditionLetters [gameData.multiplayer.players [i].connected] != szConditionLetters [m_oldStates [i]])
			gameData.score.nKillsChanged = 1;
		m_oldStates [i] = gameData.multiplayer.players [i].connected;
		NetworkSendEndLevelPacket ();
		}

	if (gameData.multiplayer.players [i].connected != 1) {
		m_nEscaped++;
		if ((gameData.multiplayer.players [i].connected == 0) || (gameData.multiplayer.players [i].connected == 7))
			m_nReady++;
		}

	if (m_nReady >= gameData.multiplayer.nPlayers)
		return 0;
	if (m_nEscaped >= gameData.multiplayer.nPlayers)
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
return 1;
}

//-----------------------------------------------------------------------------

void CScoreTable::Display (void)
{											 
   int	i;
	uint	t0 = SDL_GetTicks ();
	int	key;
	int	bRedraw = 0;

m_bNetwork = IsNetworkGame != 0;
m_nPrevSecsLeft = -1;
gameStates.menus.nInMenu++;
gameStates.app.bGameRunning = 0;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	audio.DestroyObjectSound (gameData.multiplayer.players [i].nObject);

SetScreenMode (SCREEN_MENU);
gameData.score.bWaitingForOthers = 0;
//@@GrPaletteFadeIn (grPalette,32, 0);
GameFlushInputs ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	m_oldStates [i] = gameData.multiplayer.players [i].connected;
if (m_bNetwork)
	NetworkEndLevel (&key);
backgroundManager.LoadStars (true);
while (true) {
	if (!bRedraw || (ogl.m_states.nDrawBuffer == GL_BACK)) {
		Render ();
		bRedraw = 1;
		}
	gameData.score.nKillsChanged = 0;
	redbook.CheckRepeat ();
	i = Input ();
	if (i < 0)
		return;
	if (i > 0)
		break;

	uint t = SDL_GetTicks ();
	if (t >= t0 + MAX_VIEW_TIME) {
		if (LAST_OEM_LEVEL) {
			Cleanup (1);
			return;
			}
		if ((gameData.app.GameMode (GM_SERIAL | GM_MODEM)) != 0) 
			break;
		CONNECT (N_LOCALPLAYER, CONNECT_ADVANCE_LEVEL); // player is idling in score screen for MAX_VIEW_TIMES secs 
#if 1
		if (t >= t0 + 2 * MAX_VIEW_TIME) // player wants to proceed and has waited for MAX_VIEW_TIME secs, so proceed
			break;
#endif
		}
	if (m_bNetwork) {
		CMenu m (1);
		m.AddGauge ("", "", -1, 1000); //dummy for NetworkEndLevelPoll2()
		NetworkEndLevelPoll2 (m, key, 0, 0); // check the states of the other players
		if (!WaitForPlayers ())
			break;
		}
	}
CONNECT (N_LOCALPLAYER, CONNECT_ADVANCE_LEVEL);
// Restore background and exit
paletteManager.DisableEffect ();
GameFlushInputs ();
Cleanup (0);
}

//-----------------------------------------------------------------------------
//eof
