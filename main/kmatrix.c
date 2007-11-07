/* $Id: kmatrix.c,v 1.6 2003/10/11 09:28:38 btb Exp $ */
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
#include <ctype.h>
#include <stdarg.h>

#include "error.h"
#include "inferno.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "u_mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "gamefont.h"
#include "cntrlcen.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "songs.h"
#include "multi.h"
#include "kmatrix.h"
#include "gauges.h"
#include "pcx.h"
#include "network.h"
#include "ogl_defs.h"

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25))/2)
#define CENTERSCREEN (gameStates.menus.bHires?320:160)

int nKMatrixKillsChanged = 0;
char ConditionLetters []={' ','P','E','D','E','E','V','W'};
char bWaitingForOthers=0;

int Kmatrix_nomovie_message=0;

extern int PhallicLimit,PhallicMan;

void kmatrix_reactor (char *message);
void kmatrix_phallic ();
void kmatrix_redraw_coop ();

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

static int xOffs = 0, yOffs = 0;

ubyte *starsPalette = NULL;

void LoadStars (bkg *bg, int bRedraw);

//-----------------------------------------------------------------------------

void kmatrix_draw_item (int  i, int *sorted)
{
	int j, x, y;
	char temp [10];

	y = LHY (50+i*9) + yOffs;

	// Print tPlayer name.

GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)) + xOffs, y, "%s", gameData.multiplayer.players [sorted [i]].callsign);
  if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
   GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)-15),y,"%c",ConditionLetters [gameData.multiplayer.players [sorted [i]].connected]);
   
for (j=0; j<gameData.multiplayer.nPlayers; j++) {
	x = LHX (70 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + j*25) + xOffs;
	if (sorted [i]==sorted [j]) {
		if (gameData.multigame.kills.matrix [sorted [i]] [sorted [j]] == 0) {
			GrSetFontColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.kills.matrix [sorted [i]] [sorted [j]]);
			} 
		else {
			GrSetFontColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (NULL, x, y, "-%d", gameData.multigame.kills.matrix [sorted [i]] [sorted [j]]);
			}
		} 
	else {
		if (gameData.multigame.kills.matrix [sorted [i]] [sorted [j]] <= 0) {
			GrSetFontColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.kills.matrix [sorted [i]] [sorted [j]]);
			} 
		else {
			GrSetFontColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (NULL, x, y, "%d", gameData.multigame.kills.matrix [sorted [i]] [sorted [j]]);
			}
		}
	}

if (gameData.multiplayer.players [sorted [i]].netKilledTotal + gameData.multiplayer.players [sorted [i]].netKillsTotal==0)
	sprintf (temp,"N/A");
else
   sprintf (temp,"%d%%", (int) ((double) ((double)gameData.multiplayer.players [sorted [i]].netKillsTotal/ ((double)gameData.multiplayer.players [sorted [i]].netKilledTotal+ (double)gameData.multiplayer.players [sorted [i]].netKillsTotal))*100.0));		
x = LHX (60 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + gameData.multiplayer.nPlayers*25) + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (25,25,25),1, 0, 0);
GrPrintF (NULL, x ,y,"%4d/%s",gameData.multiplayer.players [sorted [i]].netKillsTotal,temp);
}

//-----------------------------------------------------------------------------

void kmatrix_draw_coop_item (int  i, int *sorted)
{
	int  x, y = LHY (50+i*9) + yOffs;

// Print tPlayer name.
GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)) + xOffs, y, "%s", gameData.multiplayer.players [sorted [i]].callsign);
GrPrintF (NULL, LHX (CENTERING_OFFSET (gameData.multiplayer.nPlayers)-15) + xOffs,y,"%c",ConditionLetters [gameData.multiplayer.players [sorted [i]].connected]);
x = CENTERSCREEN + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [sorted [i]].score);
x = CENTERSCREEN+LHX (50) + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [sorted [i]].netKilledTotal);
}

//-----------------------------------------------------------------------------

void kmatrix_draw_names (int *sorted)
{
	int j, x;
	int color;

if (Kmatrix_nomovie_message) {
	GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}

for (j = 0; j<gameData.multiplayer.nPlayers; j++) {
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (sorted [j]);
	else
		color = sorted [j];
	x = LHX (70 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + j*25) + xOffs;
	if (gameData.multiplayer.players [sorted [j]].connected == 0)
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
   else
      GrSetFontColorRGBi (RGBA_PAL2 (playerColors  [color].r, playerColors  [color].g, playerColors  [color].b), 1, 0, 0);
	GrPrintF (NULL, x, LHY (40) + yOffs, "%c", gameData.multiplayer.players [sorted [j]].callsign [0]);
	}
x = LHX (72 + CENTERING_OFFSET (gameData.multiplayer.nPlayers) + gameData.multiplayer.nPlayers*25) + xOffs;
GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
GrPrintF (NULL, x, LHY (40) + yOffs, "K/E");
}

//-----------------------------------------------------------------------------

void kmatrix_draw_coop_names (int *sorted)
{
	sorted=sorted;

if (Kmatrix_nomovie_message) {
	GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}
GrSetFontColorRGBi (RGBA_PAL2 (63,31,31), 1, 0, 0);
GrPrintF (NULL, CENTERSCREEN, LHY (40), "SCORE");
GrSetFontColorRGBi (RGBA_PAL2 (63,31,31), 1, 0, 0);
GrPrintF (NULL, CENTERSCREEN+LHX (50), LHY (40), "DEATHS");
}

//-----------------------------------------------------------------------------

void kmatrix_draw_deaths (int *sorted)
{
	int	y,x;
	int	sw, sh, aw;
	char	reactor_message [50];

y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
         
	   				
GrSetFontColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
GrGetStringSize ("P-Playing E-Escaped D-Died", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y+= (sh+5);
GrGetStringSize ("V-Viewing scores W-Waiting", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected==7) {
   GrGetStringSize ("Waiting for other players...",&sw, &sh, &aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"Waiting for other players...");
   }
else {
   GrGetStringSize (TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
   }
if (gameData.reactor.countdown.nSecsLeft <=0)
   kmatrix_reactor (TXT_REACTOR_EXPLODED);
else {
   sprintf ((char *)&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactor.countdown.nSecsLeft, TXT_SECONDS);
   kmatrix_reactor ((char *)&reactor_message);
   }
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) 
	kmatrix_phallic ();
}

//-----------------------------------------------------------------------------

void kmatrix_draw_coop_deaths (int *sorted)
{
	int	j, x, y;
	int	sw, sh, aw;
	char	reactor_message [50];
	
y = LHY (55 + gameData.multiplayer.nPlayers * 9) + yOffs;
//	GrSetFontColor (gr_getcolor (playerColors [j].r,playerColors [j].g,playerColors [j].b),-1);
GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
x = CENTERSCREEN+LHX (50) + xOffs;
GrPrintF (NULL, x, y, TXT_DEATHS);
for (j=0; j<gameData.multiplayer.nPlayers; j++) {
	x = CENTERSCREEN+LHX (50) + xOffs;
	GrPrintF (NULL, x, y, "%d", gameData.multiplayer.players [sorted [j]].netKilledTotal);
	}
y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
GrGetStringSize ("P-Playing E-Escaped D-Died", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y += (sh+5);
GrGetStringSize ("V-Viewing scores W-Waiting", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
   GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
if (LOCALPLAYER.connected==7) {
	GrGetStringSize ("Waiting for other players...",&sw, &sh, &aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y,"Waiting for other players...");
	}
else {
	GrGetStringSize (TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);
	GrPrintF (NULL, CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
	}
if (gameData.reactor.countdown.nSecsLeft <=0)
	kmatrix_reactor (TXT_REACTOR_EXPLODED);
else {
	sprintf ((char *)&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, gameData.reactor.countdown.nSecsLeft, TXT_SECONDS);
	kmatrix_reactor ((char *)&reactor_message);
	}
}

//-----------------------------------------------------------------------------

void kmatrix_reactor (char *message)
 {
  static char oldmessage [50]={0};
  int sw, sh, aw;

if ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL))
	return;
grdCurCanv->cvFont = SMALL_FONT;
if (oldmessage [0]!=0) {
	GrSetFontColorRGBi (RGBA_PAL2 (0, 1, 0), 1, 0, 0);
	GrGetStringSize (oldmessage, &sw, &sh, &aw);
	}
GrSetFontColorRGBi (RGBA_PAL2 (0, 32, 63), 1, 0, 0);
GrGetStringSize (message, &sw, &sh, &aw);
GrPrintF (NULL, CENTERSCREEN- (sw/2), LHY (55+72+12), message);
strcpy ((char *)&oldmessage,message);
}

//-----------------------------------------------------------------------------

void kmatrix_phallic ()
 {
  int sw, sh, aw;
  char message [80];

if (! (gameData.app.nGameMode & GM_HOARD))
	return;
if ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL))
	return;
if (PhallicMan==-1)
	strcpy (message,TXT_NO_RECORD);
else
	sprintf (message, TXT_BEST_RECORD, gameData.multiplayer.players [PhallicMan].callsign,PhallicLimit);
grdCurCanv->cvFont = SMALL_FONT;
GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
GrGetStringSize (message, &sw, &sh, &aw);
GrPrintF (NULL, CENTERSCREEN- (sw/2), LHY (55+72+3), message);
}


//-----------------------------------------------------------------------------

void KMatrixRedraw ()
{
	int i, color;
	int sorted [MAX_NUM_NET_PLAYERS];

xOffs = (grdCurCanv->cvBitmap.bmProps.w - 640) / 2;
yOffs = (grdCurCanv->cvBitmap.bmProps.h - 480) / 2;
if (xOffs < 0)
	xOffs = 0;
if (yOffs < 0)
	yOffs = 0;
if (gameData.app.nGameMode & GM_MULTI_COOP) {
	kmatrix_redraw_coop ();
	return;
	}
MultiSortKillList ();
WIN (DDGRLOCK (dd_grd_curcanv));
grdCurCanv->cvFont = MEDIUM3_FONT;
GrString (0x8000, LHY (10), TXT_KILL_MATRIX_TITLE, NULL);
grdCurCanv->cvFont = SMALL_FONT;
MultiGetKillList (sorted);
kmatrix_draw_names (sorted);
for (i=0; i<gameData.multiplayer.nPlayers; i++) {
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (sorted [i]);
	else
		color = sorted [i];
	if (!gameData.multiplayer.players [sorted [i]].connected)
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		GrSetFontColorRGBi (RGBA_PAL2 (playerColors  [color].r, playerColors  [color].g, playerColors [color].b), 1, 0, 0);
	kmatrix_draw_item (i, sorted);
	}
kmatrix_draw_deaths (sorted);
PA_DFX (pa_set_frontbuffer_current ());
GrUpdate (0);
PA_DFX (pa_set_backbuffer_current ());
WIN (DDGRUNLOCK (dd_grd_curcanv));
GrPaletteStepLoad (NULL);
}

//-----------------------------------------------------------------------------

void kmatrix_redraw_coop ()
{
	int i, color;
	int sorted [MAX_NUM_NET_PLAYERS];

MultiSortKillList ();
WIN (DDGRLOCK (dd_grd_curcanv));
grdCurCanv->cvFont = MEDIUM3_FONT;
GrString (0x8000, LHY (10), "COOPERATIVE SUMMARY", NULL);
grdCurCanv->cvFont = SMALL_FONT;
MultiGetKillList (sorted);
kmatrix_draw_coop_names (sorted);
for (i=0; i<gameData.multiplayer.nPlayers; i++) {
	color = sorted [i];
	if (gameData.multiplayer.players [sorted [i]].connected==0)
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		GrSetFontColorRGBi (RGBA_PAL2 (playerColors  [color].r, playerColors  [color].g, playerColors [color].b), 1, 0, 0);
	kmatrix_draw_coop_item (i, sorted);
	}
kmatrix_draw_deaths (sorted);
WIN (DDGRUNLOCK (dd_grd_curcanv));
WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
	);
PA_DFX (pa_set_frontbuffer_current ());
PA_DFX (pa_set_backbuffer_current ());
GrPaletteStepLoad (NULL);
GrUpdate (0);
}

//-----------------------------------------------------------------------------

void KMatrixQuit (bkg *bg, int bQuit, int bNetwork)
{
if (bNetwork)
	NetworkSendEndLevelPacket ();
if (bQuit) {
	LOCALPLAYER.connected = 0;
	MultiLeaveGame ();
	}
Kmatrix_nomovie_message = 0;
NMRemoveBackground (bg);
gameStates.menus.nInMenu--;
if ((gameData.missions.nCurrentLevel >= gameData.missions.nLastLevel) &&
	 !extraGameInfo [IsMultiGame].bRotateLevels)
	longjmp (gameExitPoint, 0);
}

//-----------------------------------------------------------------------------

#define MAX_VIEW_TIME   F1_0*15
#define ENDLEVEL_IDLE_TIME	F1_0*10

#define LAST_OEM_LEVEL	IS_D2_OEM && (gameData.missions.nCurrentLevel == 8)

extern void NetworkEndLevelPoll3 (int nitems, struct tMenuItem * menus, int * key, int citem);

void KMatrixView (int bNetwork)
{											 
   int i, k, done,choice;
	fix entryTime = TimerGetApproxSeconds ();
	int key;
   int oldstates [MAX_PLAYERS];
   int previousSeconds_left=-1;
   int nReady,nEscaped;
	int bRedraw = 0;
	bkg bg;

gameStates.menus.nInMenu++;
gameStates.app.bGameRunning = 0;
memset (&bg, 0, sizeof (bg));

bNetwork=gameData.app.nGameMode & GM_NETWORK;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	DigiKillSoundLinkedToObject (gameData.multiplayer.players [i].nObject);

SetScreenMode (SCREEN_MENU);
bWaitingForOthers = 0;
//@@GrPaletteFadeIn (grPalette,32, 0);
GameFlushInputs ();
done = 0;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	oldstates  [i] = gameData.multiplayer.players [i].connected;
if (bNetwork)
	NetworkEndLevel (&key);
while (!done) {
	if (!bRedraw || (curDrawBuffer == GL_BACK)) {
		LoadStars (&bg, bRedraw);
		KMatrixRedraw ();
		bRedraw = 1;
		}
	nKMatrixKillsChanged = 0;
	for (i = 0; i < 4; i++)
		if (JoyGetButtonDownCnt (i)) {
			if (LAST_OEM_LEVEL) {
				KMatrixQuit (&bg, 1, bNetwork);
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
				KMatrixQuit (&bg, 1, bNetwork);
				return;	
				}
			LOCALPLAYER.connected=7;
			if (bNetwork)
				NetworkSendEndLevelPacket ();
			break;
			}			
	//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();
	k = KeyInKey ();
	switch (k) {
		case KEY_ENTER:
		case KEY_SPACEBAR:
			if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
				done=1;
				break;
				}
			if (LAST_OEM_LEVEL) {
				KMatrixQuit (&bg, 1, bNetwork);
				return;
				}
			gameData.multiplayer.players  [gameData.multiplayer.nLocalPlayer].connected = 7;
			if (bNetwork)	
				NetworkSendEndLevelPacket ();
			break;

		case KEY_ESC:
			if (gameData.app.nGameMode & GM_NETWORK) {
				gameData.multiplayer.xStartAbortMenuTime = TimerGetApproxSeconds ();
				choice=ExecMessageBox1 (NULL, NetworkEndLevelPoll3, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				}
			else
				choice=ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				if (choice == 0) {
					KMatrixQuit (&bg, 1, bNetwork);
					return;
					}
				nKMatrixKillsChanged=1;
				break;

		case KEY_PRINT_SCREEN:
			SaveScreenShot (NULL, 0);
			break;

		case KEY_BACKSP:
			Int3 ();
			break;

		default:
			break;
		}
	if ((TimerGetApproxSeconds () >= entryTime + MAX_VIEW_TIME) && 
		 (LOCALPLAYER.connected != 7)) {
		if (LAST_OEM_LEVEL) {
			KMatrixQuit (&bg, 1, bNetwork);
			return;
			}
		if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
			done=1;
			break;
			}
		LOCALPLAYER.connected = 7;
		if (bNetwork)
			NetworkSendEndLevelPacket ();
		}	
	if (bNetwork && (gameData.app.nGameMode & GM_NETWORK)) {
		NetworkEndLevelPoll2 (0, NULL, &key, 0);
		for (nEscaped = 0, nReady = 0, i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if (gameData.multiplayer.players [i].connected && i!=gameData.multiplayer.nLocalPlayer) {
			// Check timeout for idle players
			if (TimerGetApproxSeconds () > networkData.nLastPacketTime [i]+ENDLEVEL_IDLE_TIME) {
	#if TRACE
				con_printf (CONDBG, "idle timeout for tPlayer %d.\n", i);
	#endif
				gameData.multiplayer.players [i].connected = 0;
				NetworkSendEndLevelSub (i);
				}
			}
		if (gameData.multiplayer.players [i].connected!=oldstates [i]) {
			if (ConditionLetters [gameData.multiplayer.players [i].connected] != ConditionLetters [oldstates [i]])
				nKMatrixKillsChanged = 1;
				oldstates [i] = gameData.multiplayer.players [i].connected;
				NetworkSendEndLevelPacket ();
				}
			if (gameData.multiplayer.players [i].connected==0 || gameData.multiplayer.players [i].connected==7)
				nReady++;
			if (gameData.multiplayer.players [i].connected!=1)
				nEscaped++;
			}
		if (nReady >= gameData.multiplayer.nPlayers)
			done = 1;
		if (nEscaped >= gameData.multiplayer.nPlayers)
			gameData.reactor.countdown.nSecsLeft = -1;
		if (previousSeconds_left != gameData.reactor.countdown.nSecsLeft) {
			previousSeconds_left = gameData.reactor.countdown.nSecsLeft;
			nKMatrixKillsChanged=1;
			}
		if (nKMatrixKillsChanged) {
			KMatrixRedraw ();
			nKMatrixKillsChanged = 0;
			}
		}
	}
LOCALPLAYER.connected = 7;
// Restore background and exit
GrPaletteFadeOut (NULL, 32, 0);
GameFlushInputs ();
KMatrixQuit (&bg, 0, bNetwork);
}

//-----------------------------------------------------------------------------
//eof
