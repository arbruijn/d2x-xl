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

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25))/2)
#define CENTERSCREEN (gameStates.menus.bHires?320:160)

char szConditionLetters [] = {' ','P','E','D','E','E','V','W'};

void ScoreTableReactor (const char *message);
void ScoreTablePhallic ();
void ScoreTableDrawCoop ();

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

static int xOffs = 0, yOffs = 0;

//-----------------------------------------------------------------------------

void ScoreTableDrawItem (int  i, int *sorted)
{
	int j, x, y;
	char temp [10];

	y = LHY (50+i*9) + yOffs;

	// Print CPlayerData name.

GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)) + xOffs, y, "%s", gameData.multiplayer.players [sorted [i]].callsign);
  if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
   GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)-15),y,"%c",szConditionLetters [gameData.multiplayer.players [sorted [i]].connected]);
   
for (j=0; j<gameData.multiplayer.nPlayers; j++) {
	x = LHX (70 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + j*25) + xOffs;
	if (sorted [i]==sorted [j]) {
		if (gameData.multigame.kills.matrix [sorted [i]][sorted [j]] == 0) {
			fontManager.SetColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.kills.matrix [sorted [i]][sorted [j]]);
			} 
		else {
			fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (NULL, x, y, "-%d", gameData.multigame.kills.matrix [sorted [i]][sorted [j]]);
			}
		} 
	else {
		if (gameData.multigame.kills.matrix [sorted [i]][sorted [j]] <= 0) {
			fontManager.SetColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.kills.matrix [sorted [i]][sorted [j]]);
			} 
		else {
			fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.kills.matrix [sorted [i]][sorted [j]]);
			}
		}
	}

if (gameData.multiplayer.players [sorted [i]].netKilledTotal + gameData.multiplayer.players [sorted [i]].netKillsTotal==0)
	sprintf (temp,"N/A");
else
   sprintf (temp,"%d%%", (int) ((double) ((double)gameData.multiplayer.players [sorted [i]].netKillsTotal/ ((double)gameData.multiplayer.players [sorted [i]].netKilledTotal+ (double)gameData.multiplayer.players [sorted [i]].netKillsTotal))*100.0));	
x = LHX (60 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + gameData.multiplayer.nPlayers*25) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (25,25,25),1, 0, 0);
GrPrintF (NULL, x ,y,"%4d/%s",gameData.multiplayer.players [sorted [i]].netKillsTotal,temp);
}

//-----------------------------------------------------------------------------

void ScoreTableDrawCoopItem (int  i, int *sorted)
{
	int  x, y = LHY (50+i*9) + yOffs;

// Print CPlayerData name.
GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)) + xOffs, y, "%s", gameData.multiplayer.players [sorted [i]].callsign);
GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)-15) + xOffs,y,"%c",szConditionLetters [gameData.multiplayer.players [sorted [i]].connected]);
x = CENTERSCREEN + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [sorted [i]].score);
x = CENTERSCREEN+LHX (50) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [sorted [i]].netKilledTotal);
}

//-----------------------------------------------------------------------------

void ScoreTableDrawNames (int *sorted)
{
	int j, x;
	int color;

if (gameData.score.bNoMovieMessage) {
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}

for (j = 0; j<gameData.multiplayer.nPlayers; j++) {
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (sorted [j]);
	else
		color = sorted [j];
	x = LHX (70 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + j*25) + xOffs;
	if (gameData.multiplayer.players [sorted [j]].connected == 0)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
   else
      fontManager.SetColorRGBi (RGBA_PAL2 (playerColors  [color].red, playerColors  [color].green, playerColors  [color].blue), 1, 0, 0);
	GrPrintF (NULL, x, LHY (40) + yOffs, "%c", gameData.multiplayer.players [sorted [j]].callsign [0]);
	}
x = LHX (72 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + gameData.multiplayer.nPlayers*25) + xOffs;
fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
GrPrintF (NULL, x, LHY (40) + yOffs, "K/E");
}

//-----------------------------------------------------------------------------

void ScoreTableDrawCoopNames (int *sorted)
{
	sorted=sorted;

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

void ScoreTableDrawDeaths (int *sorted)
{
	int	y,x;
	int	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
         
	   			
fontManager.SetColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
fontManager.Current ()->StringSize ("P-Playing E-Escaped D-Died", sw, sh, aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y+= (sh+5);
fontManager.Current ()->StringSize ("V-Viewing scores W-Waiting", sw, sh, aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected==7) {
   fontManager.Current ()->StringSize ("Waiting for other players...",sw, sh, aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"Waiting for other players...");
   }
else {
   fontManager.Current ()->StringSize (TXT_PRESS_ANY_KEY2, sw, sh, aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
   }
if (gameData.reactor.countdown.nSecsLeft <=0)
   ScoreTableReactor (TXT_REACTOR_EXPLODED);
else {
   sprintf (reinterpret_cast<char*> (&reactor_message), "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactor.countdown.nSecsLeft, TXT_SECONDS);
   ScoreTableReactor (reinterpret_cast<char*> (&reactor_message));
   }
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) 
	ScoreTablePhallic ();
}

//-----------------------------------------------------------------------------

void ScoreTableDrawCoopDeaths (int *sorted)
{
	int	j, x, y;
	int	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + gameData.multiplayer.nPlayers * 9) + yOffs;
//	fontManager.SetColor (gr_getcolor (playerColors [j].r,playerColors [j].g,playerColors [j].b),-1);
fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
x = CENTERSCREEN+LHX (50) + xOffs;
GrPrintF (NULL, x, y, TXT_DEATHS);
for (j=0; j<gameData.multiplayer.nPlayers; j++) {
	x = CENTERSCREEN+LHX (50) + xOffs;
	GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [sorted [j]].netKilledTotal);
	}
y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
fontManager.SetColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
fontManager.Current ()->StringSize ("P-Playing E-Escaped D-Died", sw, sh, aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y += (sh+5);
fontManager.Current ()->StringSize ("V-Viewing scores W-Waiting", sw, sh, aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected==7) {
	fontManager.Current ()->StringSize ("Waiting for other players...",sw, sh, aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"Waiting for other players...");
	}
else {
	fontManager.Current ()->StringSize (TXT_PRESS_ANY_KEY2, sw, sh, aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
	}
if (gameData.reactor.countdown.nSecsLeft <=0)
	ScoreTableReactor (TXT_REACTOR_EXPLODED);
else {
	sprintf (reinterpret_cast<char*> (&reactor_message), "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactor.countdown.nSecsLeft, TXT_SECONDS);
	ScoreTableReactor (reinterpret_cast<char*> (&reactor_message));
	}
}

//-----------------------------------------------------------------------------

void ScoreTableReactor (const char *message)
 {
  static char oldmessage [50]={0};
  int sw, sh, aw;

if ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL))
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

void ScoreTablePhallic (void)
 {
  int sw, sh, aw;
  char message [80];

if (! (gameData.app.nGameMode & GM_HOARD))
	return;
if ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL))
	return;
if (gameData.score.nPhallicMan==-1)
	strcpy (message,TXT_NO_RECORD);
else
	sprintf (message, TXT_BEST_RECORD, gameData.multiplayer.players [gameData.score.nPhallicMan].callsign,gameData.score.nPhallicLimit);
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
fontManager.Current ()->StringSize (message, sw, sh, aw);
GrPrintF (NULL, CENTERSCREEN- (sw/2), LHY (55+72+3), message);
}


//-----------------------------------------------------------------------------

void ScoreTableRedraw (void)
{
	int i, color;
	int sorted [MAX_NUM_NET_PLAYERS];

xOffs = (CCanvas::Current ()->Width () - 640) / 2;
yOffs = (CCanvas::Current ()->Height () - 480) / 2;
if (xOffs < 0)
	xOffs = 0;
if (yOffs < 0)
	yOffs = 0;
if (gameData.app.nGameMode & GM_MULTI_COOP) {
	ScoreTableDrawCoop ();
	return;
	}
MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), TXT_KILL_MATRIX_TITLE, NULL);
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (sorted);
ScoreTableDrawNames (sorted);
for (i=0; i<gameData.multiplayer.nPlayers; i++) {
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (sorted [i]);
	else
		color = sorted [i];
	if (!gameData.multiplayer.players [sorted [i]].connected)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors  [color].red, playerColors  [color].green, playerColors  [color].blue), 1, 0, 0);
	ScoreTableDrawItem (i, sorted);
	}
ScoreTableDrawDeaths (sorted);
GrUpdate (1);
paletteManager.LoadEffect ();
}

//-----------------------------------------------------------------------------

void ScoreTableDrawCoop (void)
{
	int i, color;
	int sorted [MAX_NUM_NET_PLAYERS];

MultiSortKillList ();
fontManager.SetCurrent (MEDIUM3_FONT);
GrString (0x8000, LHY (10), "COOPERATIVE SUMMARY", NULL);
fontManager.SetCurrent (SMALL_FONT);
MultiGetKillList (sorted);
ScoreTableDrawCoopNames (sorted);
for (i=0; i<gameData.multiplayer.nPlayers; i++) {
	color = sorted [i];
	if (gameData.multiplayer.players [sorted [i]].connected==0)
		fontManager.SetColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors  [color].red, playerColors  [color].green, playerColors  [color].blue), 1, 0, 0);
	ScoreTableDrawCoopItem (i, sorted);
	}
ScoreTableDrawDeaths (sorted);
CCanvas::SetCurrent (NULL);
paletteManager.LoadEffect ();
GrUpdate (1);
}

//-----------------------------------------------------------------------------

void ScoreTableQuit (int bQuit, int bNetwork)
{
if (bNetwork)
	NetworkSendEndLevelPacket ();
if (bQuit) {
	LOCALPLAYER.connected = 0;
	MultiLeaveGame ();
	}
gameData.score.bNoMovieMessage = 0;
backgroundManager.Remove ();
gameStates.menus.nInMenu--;
if ((gameData.missions.nCurrentLevel >= gameData.missions.nLastLevel) &&
	 !extraGameInfo [IsMultiGame].bRotateLevels)
	longjmp (gameExitPoint, 0);
}

//-----------------------------------------------------------------------------

#define MAX_VIEW_TIME   	15000
#define ENDLEVEL_IDLE_TIME	10000

#define LAST_OEM_LEVEL	IS_D2_OEM && (gameData.missions.nCurrentLevel == 8)

void ScoreTableView (int bNetwork)
{											 
   int	i, k, done,choice;
	uint	entryTime = SDL_GetTicks ();
	int	key;
   int	oldstates [MAX_PLAYERS];
   int	previousSeconds_left=-1;
   int	nReady,nEscaped;
	int	bRedraw = 0;

gameStates.menus.nInMenu++;
gameStates.app.bGameRunning = 0;

bNetwork = gameData.app.nGameMode & GM_NETWORK;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	audio.DestroyObjectSound (gameData.multiplayer.players [i].nObject);

SetScreenMode (SCREEN_MENU);
gameData.score.bWaitingForOthers = 0;
//@@GrPaletteFadeIn (grPalette,32, 0);
GameFlushInputs ();
done = 0;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	oldstates  [i] = gameData.multiplayer.players [i].connected;
if (bNetwork)
	NetworkEndLevel (&key);
backgroundManager.LoadStars (true);
while (!done) {
	if (!bRedraw || (ogl.m_states.nDrawBuffer == GL_BACK)) {
		backgroundManager.Redraw ();
		ScoreTableRedraw ();
		bRedraw = 1;
		}
	gameData.score.nKillsChanged = 0;
	for (i = 0; i < 4; i++)
		if (JoyGetButtonDownCnt (i)) {
			if (LAST_OEM_LEVEL) {
				ScoreTableQuit (1, bNetwork);
				return;
				}
			LOCALPLAYER.connected = 7;
			if (bNetwork)
				NetworkSendEndLevelPacket ();
			break;
			} 		
	for (i = 0; i < 3; i++)
		if (MouseButtonDownCount (i)) {
			if (LAST_OEM_LEVEL) {
				ScoreTableQuit (1, bNetwork);
				return;
				}
			LOCALPLAYER.connected=7;
			if (bNetwork)
				NetworkSendEndLevelPacket ();
			break;
			}		
	//see if redbook song needs to be restarted
	redbook.CheckRepeat ();
	k = KeyInKey ();
	switch (k) {
		case KEY_ENTER:
		case KEY_SPACEBAR:
			if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
				done=1;
				break;
				}
			if (LAST_OEM_LEVEL) {
				ScoreTableQuit (1, bNetwork);
				return;
				}
			gameData.multiplayer.players  [gameData.multiplayer.nLocalPlayer].connected = 7;
			if (bNetwork)
				NetworkSendEndLevelPacket ();
			break;

		case KEY_ESC:
			if (gameData.app.nGameMode & GM_NETWORK) {
				gameData.multiplayer.xStartAbortMenuTime = TimerGetApproxSeconds ();
				choice = MsgBox (NULL, NetworkEndLevelPoll3, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				}
			else
				choice=MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				if (choice == 0) {
					ScoreTableQuit (1, bNetwork);
					return;
					}
				gameData.score.nKillsChanged=1;
				break;

		case KEY_PRINT_SCREEN:
			SaveScreenShot (NULL, 0);
			break;

		case KEY_BACKSPACE:
			Int3 ();
			break;

		default:
			break;
		}
	if ((SDL_GetTicks () >= entryTime + MAX_VIEW_TIME) && 
		 (LOCALPLAYER.connected != 7)) {
		if (LAST_OEM_LEVEL) {
			ScoreTableQuit (1, bNetwork);
			return;
			}
		if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
			done = 1;
			break;
			}
		LOCALPLAYER.connected = 7;
		if (bNetwork)
			NetworkSendEndLevelPacket ();
		}
	if (bNetwork && (gameData.app.nGameMode & GM_NETWORK)) {
		CMenu m (1);
		m.AddGauge ("", -1, 1000); //dummy for NetworkEndLevelPoll2()
		NetworkEndLevelPoll2 (m, key, 0, 0);
		for (nEscaped = 0, nReady = 0, i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if (gameData.multiplayer.players [i].connected && i!=gameData.multiplayer.nLocalPlayer) {
			// Check timeout for idle players
			if (SDL_GetTicks () > (uint) networkData.nLastPacketTime [i] + ENDLEVEL_IDLE_TIME) {
	#if TRACE
				console.printf (CON_DBG, "idle timeout for CPlayerData %d.\n", i);
	#endif
				gameData.multiplayer.players [i].connected = 0;
				NetworkSendEndLevelSub (i);
				}
			}
		if (gameData.multiplayer.players [i].connected != oldstates [i]) {
			if (szConditionLetters [gameData.multiplayer.players [i].connected] != szConditionLetters [oldstates [i]])
				gameData.score.nKillsChanged = 1;
				oldstates [i] = gameData.multiplayer.players [i].connected;
				NetworkSendEndLevelPacket ();
				}
			if ((gameData.multiplayer.players [i].connected == 0) || (gameData.multiplayer.players [i].connected == 7))
				nReady++;
			if (gameData.multiplayer.players [i].connected != 1)
				nEscaped++;
			}
		if (nReady >= gameData.multiplayer.nPlayers)
			done = 1;
		if (nEscaped >= gameData.multiplayer.nPlayers)
			gameData.reactor.countdown.nSecsLeft = -1;
		if (previousSeconds_left != gameData.reactor.countdown.nSecsLeft) {
			previousSeconds_left = gameData.reactor.countdown.nSecsLeft;
			gameData.score.nKillsChanged=1;
			}
		if (gameData.score.nKillsChanged) {
			ScoreTableRedraw ();
			gameData.score.nKillsChanged = 0;
			}
		}
	}
LOCALPLAYER.connected = 7;
// Restore background and exit
paletteManager.DisableEffect ();
GameFlushInputs ();
ScoreTableQuit (0, bNetwork);
}

//-----------------------------------------------------------------------------
//eof
