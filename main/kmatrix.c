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

/*
 *
 * Kill matrix displayed at end of level.
 *
 * Old Log:
 * Revision 1.6  1995/09/24  10:57:48  allender
 * made any key move off of kill matrix screen as text indicates it should
 *
 * Revision 1.5  1995/08/18  08:33:05  allender
 * fixed text problem with top level player names
 *
 * Revision 1.4  1995/07/26  17:03:05  allender
 * sort of fixed spacing for mac
 *
 * Revision 1.3  1995/06/06  15:36:14  allender
 * be sure to bitblt to screen inside of kmatrix loop
 *
 * Revision 1.2  1995/06/02  07:47:15  allender
 * removed bogus include files
 *
 * Revision 1.1  1995/05/16  15:27:07  allender
 * Initial revision
 *
 * Revision 2.3  1995/05/02  17:01:22  john
 * Fixed bug with kill list not showing up in VFX mode.
 *
 * Revision 2.2  1995/03/21  14:38:20  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.1  1995/03/06  15:22:54  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:25:56  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.19  1995/02/15  14:47:23  john
 * Added code to keep track of kills during endlevel.
 *
 * Revision 1.18  1995/02/08  11:00:06  rob
 * Moved string to localized file
 *
 * Revision 1.17  1995/02/01  23:45:55  rob
 * Fixed string.
 *
 * Revision 1.16  1995/01/30  21:47:11  rob
 * Added a line of instructions.
 *
 * Revision 1.15  1995/01/20  16:58:43  rob
 * careless careless careless...
 *
 *
 * Revision 1.14  1995/01/20  13:43:48  rob
 * Longer time to view.
 *
 * Revision 1.13  1995/01/20  13:42:34  rob
 * Fixed sorting bug.
 *
 * Revision 1.12  1995/01/19  17:35:21  rob
 * Fixed coloration of player names in team mode.
 *
 * Revision 1.11  1995/01/16  21:26:15  rob
 * Fixed it!!
 *
 * Revision 1.10  1995/01/16  18:55:41  rob
 * Added include of network.h
 *
 * Revision 1.9  1995/01/16  18:22:35  rob
 * Fixed problem with signs.
 *
 * Revision 1.8  1995/01/12  16:07:51  rob
 * ADded sorting before display.
 *
 * Revision 1.7  1995/01/04  08:46:53  rob
 * JOHN CHECKED IN FOR ROB !!!
 *
 * Revision 1.6  1994/12/09  20:17:20  yuan
 * Touched up
 *
 * Revision 1.5  1994/12/09  19:46:35  yuan
 * Localized the sucker.
 *
 * Revision 1.4  1994/12/09  19:24:58  rob
 * Yuan's fix to the centering.
 *
 * Revision 1.3  1994/12/09  19:02:37  yuan
 * Cleaned up a bit.
 *
 * Revision 1.2  1994/12/09  16:19:46  yuan
 * kill matrix stuff.
 *
 * Revision 1.1  1994/12/09  15:08:58  yuan
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "pa_enabl.h"       //$$POLY_ACC
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
#include "ogl_init.h"

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25))/2)
#define CENTERSCREEN (gameStates.menus.bHires?320:160)

int kmatrixKills_changed = 0;
char ConditionLetters []={' ','P','E','D','E','E','V','W'};
char WaitingForOthers=0;

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

	// Print player name.

GrPrintF (LHX (CENTERING_OFFSET (gameData.multi.nPlayers)) + xOffs, y, "%s", gameData.multi.players [sorted [i]].callsign);
  if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
   GrPrintF (LHX (CENTERING_OFFSET (gameData.multi.nPlayers)-15),y,"%c",ConditionLetters [gameData.multi.players [sorted [i]].connected]);
   
for (j=0; j<gameData.multi.nPlayers; j++) {
	x = LHX (70 + CENTERING_OFFSET (gameData.multi.nPlayers) + j*25) + xOffs;
	if (sorted [i]==sorted [j]) {
		if (multiData.kills.matrix [sorted [i]] [sorted [j]] == 0) {
			GrSetFontColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (x, y, "%d", multiData.kills.matrix [sorted [i]] [sorted [j]]);
			} 
		else {
			GrSetFontColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (x, y, "-%d", multiData.kills.matrix [sorted [i]] [sorted [j]]);
			}
		} 
	else {
		if (multiData.kills.matrix [sorted [i]] [sorted [j]] <= 0) {
			GrSetFontColorRGBi (RGBA_PAL2 (10,10,10), 1, 0, 0);
			GrPrintF (x, y, "%d", multiData.kills.matrix [sorted [i]] [sorted [j]]);
			} 
		else {
			GrSetFontColorRGBi (RGBA_PAL2 (25,25,25), 1, 0, 0);
			GrPrintF (x, y, "%d", multiData.kills.matrix [sorted [i]] [sorted [j]]);
			}
		}
	}

if (gameData.multi.players [sorted [i]].netKilledTotal + gameData.multi.players [sorted [i]].netKillsTotal==0)
	sprintf (temp,"N/A");
else
   sprintf (temp,"%d%%", (int) ((double) ((double)gameData.multi.players [sorted [i]].netKillsTotal/ ((double)gameData.multi.players [sorted [i]].netKilledTotal+ (double)gameData.multi.players [sorted [i]].netKillsTotal))*100.0));		
x = LHX (60 + CENTERING_OFFSET (gameData.multi.nPlayers) + gameData.multi.nPlayers*25) + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (25,25,25),1, 0, 0);
GrPrintF (x ,y,"%4d/%s",gameData.multi.players [sorted [i]].netKillsTotal,temp);
}

//-----------------------------------------------------------------------------

void kmatrix_draw_coop_item (int  i, int *sorted)
{
	int  x, y = LHY (50+i*9) + yOffs;

// Print player name.
GrPrintF (LHX (CENTERING_OFFSET (gameData.multi.nPlayers)) + xOffs, y, "%s", gameData.multi.players [sorted [i]].callsign);
GrPrintF (LHX (CENTERING_OFFSET (gameData.multi.nPlayers)-15) + xOffs,y,"%c",ConditionLetters [gameData.multi.players [sorted [i]].connected]);
x = CENTERSCREEN + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (x, y, "%d", gameData.multi.players [sorted [i]].score);
x = CENTERSCREEN+LHX (50) + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (60,40,10),1, 0, 0);
GrPrintF (x, y, "%d", gameData.multi.players [sorted [i]].netKilledTotal);
}

//-----------------------------------------------------------------------------

void kmatrix_draw_names (int *sorted)
{
	int j, x;
	int color;

if (Kmatrix_nomovie_message) {
	GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}

for (j=0; j<gameData.multi.nPlayers; j++) {
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (sorted [j]);
	else
		color = sorted [j];
	x = LHX (70 + CENTERING_OFFSET (gameData.multi.nPlayers) + j*25) + xOffs;
	if (gameData.multi.players [sorted [j]].connected==0)
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
   else
      GrSetFontColorRGBi (RGBA_PAL2 (player_rgb  [color].r, player_rgb  [color].g, player_rgb  [color].b), 1, 0, 0);
	GrPrintF (x, LHY (40) + yOffs, "%c", gameData.multi.players [sorted [j]].callsign [0]);
	}
x = LHX (72 + CENTERING_OFFSET (gameData.multi.nPlayers) + gameData.multi.nPlayers*25) + xOffs;
GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
GrPrintF (x, LHY (40) + yOffs, "K/E");
}

//-----------------------------------------------------------------------------

void kmatrix_draw_coop_names (int *sorted)
{
	sorted=sorted;

if (Kmatrix_nomovie_message) {
	GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (CENTERSCREEN-LHX (40), LHY (20), " (Movie not played)");
	}
GrSetFontColorRGBi (RGBA_PAL2 (63,31,31), 1, 0, 0);
GrPrintF (CENTERSCREEN, LHY (40), "SCORE");
GrSetFontColorRGBi (RGBA_PAL2 (63,31,31), 1, 0, 0);
GrPrintF (CENTERSCREEN+LHX (50), LHY (40), "DEATHS");
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
	GrPrintF (CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y+= (sh+5);
GrGetStringSize ("V-Viewing scores W-Waiting", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
if (gameData.multi.players [gameData.multi.nLocalPlayer].connected==7) {
   GrGetStringSize ("Waiting for other players...",&sw, &sh, &aw);
   GrPrintF (CENTERSCREEN- (sw/2), y,"Waiting for other players...");
   }
else {
   GrGetStringSize (TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);
   GrPrintF (CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
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
	
y = LHY (55 + gameData.multi.nPlayers * 9) + yOffs;
//	GrSetFontColor (gr_getcolor (player_rgb [j].r,player_rgb [j].g,player_rgb [j].b),-1);
GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
x = CENTERSCREEN+LHX (50) + xOffs;
GrPrintF (x, y, TXT_DEATHS);
for (j=0; j<gameData.multi.nPlayers; j++) {
	x = CENTERSCREEN+LHX (50) + xOffs;
	GrPrintF (x, y, "%d", gameData.multi.players [sorted [j]].netKilledTotal);
	}
y = LHY (55 + 72 + 35) + yOffs;
x = LHX (35) + xOffs;
GrSetFontColorRGBi (RGBA_PAL2 (63,20,0), 1, 0, 0);
GrGetStringSize ("P-Playing E-Escaped D-Died", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
	GrPrintF (CENTERSCREEN- (sw/2), y,"P-Playing E-Escaped D-Died");
y += (sh+5);
GrGetStringSize ("V-Viewing scores W-Waiting", &sw, &sh, &aw);
if (! ((gameData.app.nGameMode & GM_MODEM) || (gameData.app.nGameMode & GM_SERIAL)))
   GrPrintF (CENTERSCREEN- (sw/2), y,"V-Viewing scores W-Waiting");
y+=LHY (20);
GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
if (gameData.multi.players [gameData.multi.nLocalPlayer].connected==7) {
	GrGetStringSize ("Waiting for other players...",&sw, &sh, &aw);
	GrPrintF (CENTERSCREEN- (sw/2), y,"Waiting for other players...");
	}
else {
	GrGetStringSize (TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);
	GrPrintF (CENTERSCREEN- (sw/2), y, TXT_PRESS_ANY_KEY2);
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
grdCurCanv->cv_font = SMALL_FONT;
if (oldmessage [0]!=0) {
	GrSetFontColorRGBi (RGBA_PAL2 (0, 1, 0), 1, 0, 0);
	GrGetStringSize (oldmessage, &sw, &sh, &aw);
	}
GrSetFontColorRGBi (RGBA_PAL2 (0, 32, 63), 1, 0, 0);
GrGetStringSize (message, &sw, &sh, &aw);
GrPrintF (CENTERSCREEN- (sw/2), LHY (55+72+12), message);
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
	sprintf (message, TXT_BEST_RECORD, gameData.multi.players [PhallicMan].callsign,PhallicLimit);
grdCurCanv->cv_font = SMALL_FONT;
GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
GrGetStringSize (message, &sw, &sh, &aw);
GrPrintF (CENTERSCREEN- (sw/2), LHY (55+72+3), message);
}


//-----------------------------------------------------------------------------

void kmatrix_redraw ()
{
	int i, color;
	int sorted [MAX_NUM_NET_PLAYERS];

xOffs = (grdCurCanv->cv_bitmap.bm_props.w - 640) / 2;
yOffs = (grdCurCanv->cv_bitmap.bm_props.h - 480) / 2;
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
grdCurCanv->cv_font = MEDIUM3_FONT;
GrString (0x8000, LHY (10), TXT_KILL_MATRIX_TITLE	);
grdCurCanv->cv_font = SMALL_FONT;
MultiGetKillList (sorted);
kmatrix_draw_names (sorted);
for (i=0; i<gameData.multi.nPlayers; i++) {
	if (gameData.app.nGameMode & GM_TEAM)
		color = GetTeam (sorted [i]);
	else
		color = sorted [i];
	if (gameData.multi.players [sorted [i]].connected==0)
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		GrSetFontColorRGBi (RGBA_PAL2 (player_rgb  [color].r, player_rgb  [color].g, player_rgb [color].b), 1, 0, 0);
	kmatrix_draw_item (i, sorted);
	}
kmatrix_draw_deaths (sorted);
#if defined (POLY_ACC)
pa_save_clut ();
pa_update_clut (grPalette, 0, 256, 0);
#endif
PA_DFX (pa_set_frontbuffer_current ());
GrUpdate (0);
PA_DFX (pa_set_backbuffer_current ());
#if defined (POLY_ACC)
pa_restore_clut ();
#endif
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
grdCurCanv->cv_font = MEDIUM3_FONT;
GrString (0x8000, LHY (10), "COOPERATIVE SUMMARY"	);
grdCurCanv->cv_font = SMALL_FONT;
MultiGetKillList (sorted);
kmatrix_draw_coop_names (sorted);
for (i=0; i<gameData.multi.nPlayers; i++) {
	color = sorted [i];
	if (gameData.multi.players [sorted [i]].connected==0)
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
	else
		GrSetFontColorRGBi (RGBA_PAL2 (player_rgb  [color].r, player_rgb  [color].g, player_rgb [color].b), 1, 0, 0);
	kmatrix_draw_coop_item (i, sorted);
	}
kmatrix_draw_deaths (sorted);
WIN (DDGRUNLOCK (dd_grd_curcanv));
WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
	);
#if defined (POLY_ACC)
pa_save_clut ();
pa_update_clut (grPalette, 0, 256, 0);
#endif
PA_DFX (pa_set_frontbuffer_current ());
PA_DFX (pa_set_backbuffer_current ());
#if defined (POLY_ACC)
pa_restore_clut ();
#endif
GrPaletteStepLoad (NULL);
GrUpdate (0);
}

//-----------------------------------------------------------------------------

void KMatrixQuit (bkg *bg, int bQuit, int network)
{
if (network)
	NetworkSendEndLevelPacket ();
if (bQuit) {
	gameData.multi.players [gameData.multi.nLocalPlayer].connected = 0;
	MultiLeaveGame ();
	}
Kmatrix_nomovie_message = 0;
NMRemoveBackground (bg);
gameStates.menus.nInMenu--;
longjmp (gameExitPoint, 0);
}

//-----------------------------------------------------------------------------

#define MAX_VIEW_TIME   F1_0*15
#define ENDLEVEL_IDLE_TIME	F1_0*10

#define LAST_OEM_LEVEL	IS_D2_OEM && (gameData.missions.nCurrentLevel == 8)

extern void NetworkEndLevelPoll3 (int nitems, struct tMenuItem * menus, int * key, int citem);

void kmatrix_view (int network)
{											 
   int i, k, done,choice;
	fix entryTime = TimerGetApproxSeconds ();
	int key;
   int oldstates [MAX_PLAYERS];
   int previous_seconds_left=-1;
   int nReady,nEscaped;
	int bRedraw = 0;
	bkg bg;

gameStates.menus.nInMenu++;
gameStates.app.bGameRunning = 0;
memset (&bg, 0, sizeof (bg));

network=gameData.app.nGameMode & GM_NETWORK;

for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
DigiKillSoundLinkedToObject (gameData.multi.players [i].nObject);

SetScreenMode (SCREEN_MENU);
WaitingForOthers=0;
//@@GrPaletteFadeIn (grPalette,32, 0);
GameFlushInputs ();
done = 0;
for (i = 0; i < gameData.multi.nPlayers; i++)
	oldstates  [i] = gameData.multi.players [i].connected;
if (network)
	NetworkEndLevel (&key);
while (!done) {
	if (!bRedraw || (curDrawBuffer == GL_BACK)) {
		LoadStars (&bg, bRedraw);
		kmatrix_redraw ();
		bRedraw = 1;
		}
	kmatrixKills_changed = 0;
	for (i = 0; i < 4; i++)
		if (joy_get_button_down_cnt (i)) {
			if (LAST_OEM_LEVEL) {
				KMatrixQuit (&bg, 1, network);
				return;
				}
			gameData.multi.players [gameData.multi.nLocalPlayer].connected = 7;
			if (network)
				NetworkSendEndLevelPacket ();
			break;
			} 			
	for (i = 0; i < 3; i++)	
		if (MouseButtonDownCount (i)) {
			if (LAST_OEM_LEVEL) {
				KMatrixQuit (&bg, 1, network);
				return;	
				}
			gameData.multi.players [gameData.multi.nLocalPlayer].connected=7;
			if (network)
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
				KMatrixQuit (&bg, 1, network);
				return;
				}
			gameData.multi.players  [gameData.multi.nLocalPlayer].connected = 7;
			if (network)	
				NetworkSendEndLevelPacket ();
			break;

		case KEY_ESC:
			if (gameData.app.nGameMode & GM_NETWORK) {
				gameData.multi.xStartAbortMenuTime = TimerGetApproxSeconds ();
				choice=ExecMessageBox1 (NULL, NetworkEndLevelPoll3, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				}
			else
				choice=ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
				if (choice == 0) {
					KMatrixQuit (&bg, 1, network);
					return;
					}
				kmatrixKills_changed=1;
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
		 (gameData.multi.players [gameData.multi.nLocalPlayer].connected != 7)) {
		if (LAST_OEM_LEVEL) {
			KMatrixQuit (&bg, 1, network);
			return;
			}
		if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
			done=1;
			break;
			}
		gameData.multi.players [gameData.multi.nLocalPlayer].connected = 7;
		if (network)
			NetworkSendEndLevelPacket ();
		}	
	if (network && (gameData.app.nGameMode & GM_NETWORK)) {
		NetworkEndLevelPoll2 (0, NULL, &key, 0);
		for (nEscaped = 0, nReady = 0, i = 0; i < gameData.multi.nPlayers; i++) {
			if (gameData.multi.players [i].connected && i!=gameData.multi.nLocalPlayer) {
			// Check timeout for idle players
			if (TimerGetApproxSeconds () > networkData.nLastPacketTime [i]+ENDLEVEL_IDLE_TIME) {
	#if TRACE
				con_printf (CON_DEBUG, "idle timeout for player %d.\n", i);
	#endif
				gameData.multi.players [i].connected = 0;
				NetworkSendEndLevelSub (i);
				}
			}
		if (gameData.multi.players [i].connected!=oldstates [i]) {
			if (ConditionLetters [gameData.multi.players [i].connected] != ConditionLetters [oldstates [i]])
				kmatrixKills_changed = 1;
				oldstates [i] = gameData.multi.players [i].connected;
				NetworkSendEndLevelPacket ();
				}
			if (gameData.multi.players [i].connected==0 || gameData.multi.players [i].connected==7)
				nReady++;
			if (gameData.multi.players [i].connected!=1)
				nEscaped++;
			}
		if (nReady >= gameData.multi.nPlayers)
			done = 1;
		if (nEscaped >= gameData.multi.nPlayers)
			gameData.reactor.countdown.nSecsLeft = -1;
		if (previous_seconds_left != gameData.reactor.countdown.nSecsLeft) {
			previous_seconds_left = gameData.reactor.countdown.nSecsLeft;
			kmatrixKills_changed=1;
			}
		if (kmatrixKills_changed) {
			kmatrix_redraw ();
			kmatrixKills_changed = 0;
			}
		}
	}
gameData.multi.players [gameData.multi.nLocalPlayer].connected = 7;
// Restore background and exit
GrPaletteFadeOut (NULL, 32, 0);
GameFlushInputs ();
KMatrixQuit (&bg, 0, network);
}

//-----------------------------------------------------------------------------
//eof
