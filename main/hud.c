/* $Id: hud.c,v 1.8 2003/10/10 09:36:35 btb Exp $ */
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
 * Routines for displaying HUD messages...
 *
 * Old Log:
 * Revision 1.4  1995/08/24  16:03:09  allender
 * fix up pszMsg placement
 *
 * Revision 1.3  1995/08/18  10:25:21  allender
 * added support for pixel doubling using PC game font
 *
 * Revision 1.2  1995/08/12  11:33:22  allender
 * removed #ifdef NEWDEMO -- always in
 *
 * Revision 1.1  1995/05/16  15:26:32  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/30  16:36:40  mike
 * text localization.
 *
 * Revision 2.1  1995/03/06  15:23:50  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:30:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.27  1995/01/23  16:51:30  mike
 * Show hud messages on 3d if window in three largest sizes.
 *
 * Revision 1.26  1995/01/17  17:42:45  rob
 * Made pszMsg timeout for HUD messages longer.
 *
 * Revision 1.25  1995/01/04  11:39:03  rob
 * Made HUD text get out of the way of large HUD messages.
 *
 * Revision 1.24  1995/01/01  14:20:32  rob
 * longer timer for hud messages.
 *
 *
 * Revision 1.23  1994/12/15  13:04:34  mike
 * Replace gameData.multi.players [gameData.multi.nLocalPlayer].time_total references with gameData.time.xGame.
 *
 * Revision 1.22  1994/12/13  12:55:12  mike
 * move press any key to continue pszMsg when you are dead to bottom of window.
 *
 * Revision 1.21  1994/12/07  17:08:01  rob
 * removed unnecessary debug info.
 *
 * Revision 1.20  1994/12/07  16:24:16  john
 * Took out code that kept track of messages differently for different
 * screen modes... I made it so they just draw differently depending on screen mode.
 *
 * Revision 1.19  1994/12/07  15:42:57  rob
 * Added a bunch of debug stuff to look for HUD pszMsg problems in net games...
 *
 * Revision 1.18  1994/12/06  16:30:35  yuan
 * Localization
 *
 * Revision 1.17  1994/12/05  00:32:36  mike
 * fix randomness of color on status bar hud messages.
 *
 * Revision 1.16  1994/11/19  17:05:53  rob
 * Moved dead_player_message down to avoid overwriting HUD messages.
 *
 * Revision 1.15  1994/11/18  23:37:56  john
 * Changed some shorts to ints.
 *
 * Revision 1.14  1994/11/12  16:38:25  mike
 * clear some annoying debug messages.
 *
 * Revision 1.13  1994/11/11  15:36:39  mike
 * write hud messages on background if 3d window small enough
 *
 * Revision 1.12  1994/10/20  09:49:31  mike
 * Reduce number of messages.
 *
 * Revision 1.11  1994/10/17  10:49:15  john
 * Took out some warnings.
 *
 * Revision 1.10  1994/10/17  10:45:13  john
 * Made the player able to abort death by pressing any button or key.
 *
 * Revision 1.9  1994/10/13  15:17:33  mike
 * Remove afterburner references.
 *
 * Revision 1.8  1994/10/11  12:06:32  mike
 * Only show pszMsg advertising death sequence abort after player exploded.
 *
 * Revision 1.7  1994/10/10  17:21:53  john
 * Made so instead of saying too many messages, it scrolls off the
 * oldest pszMsg.
 *
 * Revision 1.6  1994/10/07  23:05:39  john
 * Fixed bug with HUD not drawing stuff sometimes...
 * ( I had a circular buffer that I was stepping thru
 * to draw text that went: for (i=first;i<last;i++)...
 * duh!! last could be less than first.)
 * /
 *
 * Revision 1.5  1994/09/16  13:08:20  mike
 * Arcade stuff, afterburner stuff.
 *
 * Revision 1.4  1994/09/14  16:26:57  mike
 * PlayerDeadMessage.
 *
 * Revision 1.3  1994/08/18  16:35:45  john
 * Made gauges messages stay up a bit longer.
 *
 * Revision 1.2  1994/08/18  12:10:21  john
 * Made HUD messages scroll.
 *
 * Revision 1.1  1994/08/18  11:22:09  john
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hudmsg.h"

#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"

#include "menu.h"           // For the font.
#include "mono.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"

#include "wall.h"
#include "screens.h"
#include "text.h"
#include "laser.h"
#include "args.h"
#include "pa_enabl.h"

extern void SetFunctionMode (int);

extern void copy_background_rect (int left,int top,int right,int bot);
char szDisplayedBackgroundMsg [2][HUD_MESSAGE_LENGTH] = {"",""};

int nLastMsgYCrd = -1;
int nLastMsgHeight = 6;
int bMSGPlayerMsgs = 0;
int bNoMsgRedundancy = 0;
int nModexHUDMsgs;

#define LHX (x)      (gameStates.render.fonts.bHires?2* (x):x)
#define LHY (y)      (gameStates.render.fonts.bHires? (24* (y))/10:y)

#ifdef WINDOWS
int bExtraClear=0;
#endif

extern int max_window_h;
extern int max_window_w;

extern grs_canvas *PrintToCanvas (char *s,grs_font *font, unsigned int fc, unsigned int bc, int double_flag);

// ----------------------------------------------------------------------------

void ClearBackgroundMessages (void)
{
	#ifdef WINDOWS
if (bExtraClear == gameData.app.nFrameCount) 		//don't do extra clear on same frame
	return;
	#endif

#ifdef WINDOWS
if (( (gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)) && (nLastMsgYCrd != -1) && (dd_VR_render_sub_buffer [0].yoff >= 6)) {
	dd_grs_canvas *canv_save = dd_grd_curcanv;
#else
if (( (gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)) && (nLastMsgYCrd != -1) && (VR_render_sub_buffer [0].cv_bitmap.bm_props.y >= 6)) {
	grs_canvas	*canv_save = grdCurCanv;
#endif

WINDOS (
	DDGrSetCurrentCanvas (GetCurrentGameScreen ()),
	GrSetCurrentCanvas (GetCurrentGameScreen ())
	);
PA_DFX (pa_set_frontbuffer_current ());
PA_DFX (copy_background_rect (0, nLastMsgYCrd, grdCurCanv->cv_bitmap.bm_props.w, nLastMsgYCrd+nLastMsgHeight-1));
PA_DFX (pa_set_backbuffer_current ());
copy_background_rect (0, nLastMsgYCrd, grdCurCanv->cv_bitmap.bm_props.w, nLastMsgYCrd+nLastMsgHeight-1);
WINDOS (
	DDGrSetCurrentCanvas (canv_save),
	GrSetCurrentCanvas (canv_save)
	);
#ifdef WINDOWS
	if (bExtraClear || !GRMODEINFO (modex)) {
		bExtraClear = 0;
		nLastMsgYCrd = -1;
		}
	else
		bExtraClear = gameData.app.nFrameCount;
#else
	nLastMsgYCrd = -1;
#endif
	}
szDisplayedBackgroundMsg [VR_current_page][0] = 0;
}

//	-----------------------------------------------------------------------------

void HUDClearMessages ()
{
	int i, j;
	tHUDMessage	*pMsgs;

for (j = 2, pMsgs = gameData.hud.msgs; j; j--, pMsgs++) {
	pMsgs->nMessages = 0;
	pMsgs->nFirst = 
	pMsgs->nLast = 0;
	pMsgs->xTimer = 0;
	ClearBackgroundMessages ();
	for (i = 0; i < HUD_MAX_MSGS; i++)
		sprintf (pMsgs->szMsgs [i], TXT_SLAGEL);
	}
}


//	-----------------------------------------------------------------------------
//	print to buffer, double heights, and blit bitmap to screen
void HUDModexMessage (int x, int y, char *s, grs_font *font, unsigned int color)
{
grs_canvas *temp_canv = PrintToCanvas (s, font, color, 0, 1);
GrBitmapM (x, y, &temp_canv->cv_bitmap);
GrFreeCanvas (temp_canv);
}

// ----------------------------------------------------------------------------

void HUDRenderMessages (ubyte nType)
{
	int			h, i, n, w, y, aw, yStart, nMsg;
	char			*pszMsg;
	tHUDMessage *pMsgs = gameData.hud.msgs + nType;

if ((pMsgs->nMessages < 0) || (pMsgs->nMessages > HUD_MAX_MSGS))
	Int3 (); // Get Rob!
if ((pMsgs->nMessages < 1) && (nModexHUDMsgs == 0))
	return;
pMsgs->xTimer -= gameData.time.xFrame;
#ifdef WINDOWS
if (bExtraClear)
	ClearBackgroundMessages ();			//	If in status bar mode and no messages, then erase.
#endif
if ( pMsgs->xTimer < 0)	{
	// Timer expired... get rid of oldest pszMsg...
	if (pMsgs->nLast != pMsgs->nFirst)	{
		int	temp;

		//&pMsgs->szMsgs.szMsg [pMsgs->nFirst][0] is deing deleted...;
		pMsgs->nFirst = (pMsgs->nFirst + 1) % HUD_MAX_MSGS;
		pMsgs->xTimer = F1_0*2;
		if (!--pMsgs->nMessages)
			nModexHUDMsgs = 2;
		temp = nLastMsgYCrd;
		ClearBackgroundMessages ();			//	If in status bar mode and no messages, then erase.
		if (nModexHUDMsgs)
			nLastMsgYCrd = temp;
	}
}

if (pMsgs->nMessages > 0) {
	if (pMsgs->nColor == -1)
		pMsgs->nColor = RGBA_PAL2 (0,28,0);

#ifdef WINDOWS
	if ((VR_render_mode == VR_NONE) && ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || 
		 (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)) && (dd_VR_render_sub_buffer [0].yoff >= (max_window_h/8))) {
#else
	if ((VR_render_mode == VR_NONE) && ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || 
		 (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)) && (VR_render_sub_buffer [0].cv_bitmap.bm_props.y >= (max_window_h/8))) {
#endif
		// Only display the most recent pszMsg in this mode
		nMsg = (pMsgs->nFirst + pMsgs->nMessages-1) % HUD_MAX_MSGS;
		pszMsg = pMsgs->szMsgs [nMsg];

		if (strcmp (szDisplayedBackgroundMsg [VR_current_page], pszMsg)) {
			int	ycrd;
			WINDOS (
				dd_grs_canvas *canv_save = dd_grd_curcanv,
				grs_canvas	*canv_save = grdCurCanv
				);
			WINDOS (
				ycrd = dd_grd_curcanv->yoff - (SMALL_FONT->ft_h+2),
				ycrd = grdCurCanv->cv_bitmap.bm_props.y - (SMALL_FONT->ft_h+2)
				);
			if (ycrd < 0)
				ycrd = 0;
			WINDOS (
				DDGrSetCurrentCanvas (GetCurrentGameScreen ()),
				GrSetCurrentCanvas (GetCurrentGameScreen ())
				);
			GrSetCurFont (SMALL_FONT);
			GrGetStringSize (pszMsg, &w, &h, &aw);
			ClearBackgroundMessages ();
			if (grdCurCanv->cv_bitmap.bm_props.type == BM_MODEX) {
				WIN (Int3 ());    // No no no no ....
				ycrd -= h;
				h *= 2;
				HUDModexMessage ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, ycrd, pszMsg, SMALL_FONT, 
									  pMsgs->nColor);
				if (nModexHUDMsgs > 0) {
					nModexHUDMsgs--;
					szDisplayedBackgroundMsg [VR_current_page][0] = '!';
					}
				else
					strcpy (szDisplayedBackgroundMsg [VR_current_page], pszMsg);
				}
			else {
				WIN (DDGRLOCK (dd_grd_curcanv));
					if (pMsgs->nColor == -1)
						pMsgs->nColor = RGBA_PAL2 (0,28,0);
					GrSetFontColorRGBi (pMsgs->nColor, 1, 0, 0);
					PA_DFX (pa_set_frontbuffer_current ());
					PA_DFX (GrPrintF ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, ycrd, pszMsg));
					PA_DFX (pa_set_backbuffer_current ());
					GrPrintF ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, ycrd, pszMsg);
					strcpy (szDisplayedBackgroundMsg [VR_current_page], pszMsg);
				WIN (DDGRUNLOCK (dd_grd_curcanv));
				}
			WINDOS (
				DDGrSetCurrentCanvas (canv_save),
				GrSetCurrentCanvas (canv_save)
				);
			nLastMsgYCrd = ycrd;
			nLastMsgHeight = h;
			}
		} 
	else {
		GrSetCurFont ( SMALL_FONT);
		if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || 
			 (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
			if (Game_window_w == max_window_w)
				yStart = SMALL_FONT->ft_h / 2;
			else
				yStart= SMALL_FONT->ft_h * 2;
		} else
			yStart = SMALL_FONT->ft_h / 2;
		if (gameOpts->render.cockpit.bGuidedInMainView) {
			object *gmP = gameData.objs.guidedMissile [gameData.multi.nLocalPlayer];
			if (gmP && 
				 (gmP->type == OBJ_WEAPON) && 
				 (gmP->id == GUIDEDMISS_ID) &&
			    (gmP->signature == gameData.objs.guidedMissileSig [gameData.multi.nLocalPlayer]))
				yStart += SMALL_FONT->ft_h + 3;
			}

	WIN (DDGRLOCK (dd_grd_curcanv));
		for (i = 0, y = yStart; i < pMsgs->nMessages; i++)	{
			n = (pMsgs->nFirst + i) % HUD_MAX_MSGS;
			if ((n < 0) || (n >= HUD_MAX_MSGS))
				Int3 (); // Get Rob!!
			if (!strcmp (pMsgs->szMsgs [n], "This is a bug."))
				Int3 (); // Get Rob!!
			GrGetStringSize (pMsgs->szMsgs [n], &w, &h, &aw);
			GrSetFontColorRGBi (pMsgs->nColor, 1, 0, 0);
			PA_DFX (pa_set_frontbuffer_current ());
			y = yStart + i * (h + 1);
			if (nType)
				y += ((2 * HUD_MAX_MSGS - 1) * (h + 1)) / 2;
			PA_DFX (GrString ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, y [nType], pMsgs->szMsgs [n]));
			PA_DFX (pa_set_backbuffer_current ());
			GrString ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, y, pMsgs->szMsgs [n]);
			if (!gameOpts->render.cockpit.bSplitHUDMsgs) 
				y += h + 1;
			}
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		}
	}
#ifndef WINDOWS
else if (GetCurrentGameScreen ()->cv_bitmap.bm_props.type == BM_MODEX) {
	if (nModexHUDMsgs) {
		int temp = nLastMsgYCrd;
		nModexHUDMsgs--;
		ClearBackgroundMessages ();			//	If in status bar mode and no messages, then erase.
		nLastMsgYCrd = temp;
		}
	}
#endif
GrSetCurFont ( GAME_FONT);
}

// ----------------------------------------------------------------------------
//	Writes a pszMsg on the HUD and checks its timer.
void HUDRenderMessageFrame (void)
{
HUDRenderMessages (0);
if (gameOpts->render.cockpit.bSplitHUDMsgs)
	HUDRenderMessages (1);
}

//------------------------------------------------------------------------------
// Call to flash a pszMsg on the HUD.  Returns true if pszMsg drawn.
// (pszMsg might not be drawn if previous pszMsg was same)
int HUDInitMessageVA (ubyte nType, char * format, va_list args)
{
	tHUDMessage *pMsgs = gameData.hud.msgs + (gameOpts->render.cockpit.bSplitHUDMsgs ? nType : 0);
	int			temp;
	char			*pszMsg = NULL, 
					*pszLastMsg = NULL;

char con_message [HUD_MESSAGE_LENGTH + 3];
nModexHUDMsgs = 2;
if ((pMsgs->nLast < 0) || (pMsgs->nLast >= HUD_MAX_MSGS))
	Int3 (); // Get Rob!!
pszMsg = pMsgs->szMsgs [pMsgs->nLast];
vsprintf (pszMsg, format, args);
/* Produce a colorised version and send it to the console */
con_message [0] = CC_COLOR;
con_message [1] = (pMsgs->nColor == -1) ? nType ? GrFindClosestColor (gamePalette, 63,47,0) : GrFindClosestColor (gamePalette, 0,28,0) : pMsgs->nColor;
con_message [2] = '\0';
strcat (con_message, pszMsg);
#if TRACE		
con_printf (CON_NORMAL, "%s\n", con_message);
#endif
// Added by Leighton
if ((gameData.app.nGameMode & GM_MULTI) && gameOpts->multi.bNoRedundancy)
	if (!strnicmp ("You already", pszMsg, 11))
		return 0;
if ((gameData.app.nGameMode & GM_MULTI) && FindArg ("-PlayerMessages") && !gameData.hud.bPlayerMessage)
	return 0;
if (pMsgs->nMessages > 0)
	pszLastMsg = pMsgs->szMsgs [(pMsgs->nLast ? pMsgs->nLast : HUD_MAX_MSGS) - 1];
temp = (pMsgs->nLast + 1) % HUD_MAX_MSGS;
if (temp == pMsgs->nFirst)	{ // If too many messages, remove oldest pszMsg to make room
	pMsgs->nFirst = (pMsgs->nFirst + 1) % HUD_MAX_MSGS;
	pMsgs->nMessages--;
	}
if (pszLastMsg && (!strcmp (pszLastMsg, pszMsg))) {
	pMsgs->xTimer = F1_0*3;		// 1 second per 5 characters
	return 0;	// ignore since it is the same as the last one
	}
pMsgs->nLast = temp;
// Check if memory has been overwritten at this point.
if (strlen (pszMsg) >= HUD_MESSAGE_LENGTH)
	Error ( "Your message to HUD is too long.  Limit is %i characters.\n", HUD_MESSAGE_LENGTH);
#ifdef NEWDEMO
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordHUDMessage ( pszMsg);
#endif
pMsgs->xTimer = F1_0*3;		// 1 second per 5 characters
pMsgs->nMessages++;
return 1;
}

//------------------------------------------------------------------------------

int _CDECL_ HUDInitMessage (char *format, ...)
{
	int ret;
	va_list args;

va_start (args, format);
ret = HUDInitMessageVA (0, format, args);
va_end (args);
return ret;
}

//------------------------------------------------------------------------------

void PlayerDeadMessage (void)
{
if (gameStates.app.bPlayerExploded) {
	tHUDMessage	*pMsgs = gameData.hud.msgs;

   if ( gameData.multi.players [gameData.multi.nLocalPlayer].lives < 2) {
      int x, y, w, h, aw;
      GrSetCurFont ( HUGE_FONT);
      GrGetStringSize ( TXT_GAME_OVER, &w, &h, &aw);
      w += 20;
      h += 8;
      x = (grdCurCanv->cv_w - w) / 2;
      y = (grdCurCanv->cv_h - h) / 2;
      NO_DFX (gameStates.render.grAlpha = 2*7);
      NO_DFX (GrSetColorRGB (0, 0, 0, 255));
      NO_DFX (GrRect ( x, y, x+w, y+h));
      gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
      GrString (0x8000, (grdCurCanv->cv_h - grdCurCanv->cv_font->ft_h)/2 + h/8, TXT_GAME_OVER);
#if 0
      // Automatically exit death after 10 secs
      if ( gameData.time.xGame > gameStates.app.nPlayerTimeOfDeath + F1_0*10) {
               SetFunctionMode (FMODE_MENU);
               gameData.app.nGameMode = GM_GAME_OVER;
               longjmp ( gameExitPoint, 1);        // Exit out of game loop
	      }
#endif
	   }
   GrSetCurFont (GAME_FONT);
   if (pMsgs->nColor == -1)
      pMsgs->nColor = RGBA_PAL2 (0, 28, 0);
	GrSetFontColorRGBi (pMsgs->nColor, 1, 0, 0);
   GrString (0x8000, grdCurCanv->cv_h- (grdCurCanv->cv_font->ft_h+3), TXT_PRESS_ANY_KEY);
	}
}

//------------------------------------------------------------------------------
// void say_afterburner_status (void)
// {
// 	if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)
// 		InitHUDMessage ("Afterburner engaged.");
// 	else
// 		InitHUDMessage ("Afterburner disengaged.");
// }

void _CDECL_ HUDMessage (int nClass, char *format, ...)
{
if ((!bNoMsgRedundancy || (nClass & MSGC_NOREDUNDANCY)) &&
	   (!bMSGPlayerMsgs || ! (gameData.app.nGameMode & GM_MULTI) ||
		 (nClass & MSGC_PLAYERMESSAGES))) {
		va_list vp;

	va_start (vp, format);
	HUDInitMessageVA (0, format, vp);
	va_end (vp);
	}
}

//------------------------------------------------------------------------------

void _CDECL_ HUDPlayerMessage (char *format, ...)
{
	va_list vp;

va_start (vp, format);
HUDInitMessageVA (1, format, vp);
va_end (vp);
}

//------------------------------------------------------------------------------
//eof
