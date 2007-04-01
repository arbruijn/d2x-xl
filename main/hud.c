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

extern int max_window_h;
extern int max_window_w;

extern grs_canvas *PrintToCanvas (char *s,grs_font *font, unsigned int fc, unsigned int bc, int doubleFlag);

// ----------------------------------------------------------------------------

void ClearBackgroundMessages (void)
{
if (( (gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)) && (nLastMsgYCrd != -1) && (VR_render_sub_buffer [0].cv_bitmap.bm_props.y >= 6)) {
	grs_canvas	*canv_save = grdCurCanv;

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
	nLastMsgYCrd = -1;
	}
szDisplayedBackgroundMsg [nVRCurrentPage][0] = 0;
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
		pMsgs->nColor = GREEN_RGBA;

	if ((VR_render_mode == VR_NONE) && ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || 
		 (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)) && (VR_render_sub_buffer [0].cv_bitmap.bm_props.y >= (max_window_h/8))) {
		// Only display the most recent pszMsg in this mode
		nMsg = (pMsgs->nFirst + pMsgs->nMessages-1) % HUD_MAX_MSGS;
		pszMsg = pMsgs->szMsgs [nMsg];

		if (strcmp (szDisplayedBackgroundMsg [nVRCurrentPage], pszMsg)) {
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
			if (grdCurCanv->cv_bitmap.bm_props.nType == BM_MODEX) {
				WIN (Int3 ());    // No no no no ....
				ycrd -= h;
				h *= 2;
				HUDModexMessage ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, ycrd, pszMsg, SMALL_FONT, 
									  pMsgs->nColor);
				if (nModexHUDMsgs > 0) {
					nModexHUDMsgs--;
					szDisplayedBackgroundMsg [nVRCurrentPage][0] = '!';
					}
				else
					strcpy (szDisplayedBackgroundMsg [nVRCurrentPage], pszMsg);
				}
			else {
				WIN (DDGRLOCK (dd_grd_curcanv));
					if (pMsgs->nColor == -1)
						pMsgs->nColor = GREEN_RGBA;
					GrSetFontColorRGBi (pMsgs->nColor, 1, 0, 0);
					PA_DFX (pa_set_frontbuffer_current ());
					PA_DFX (GrPrintF ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, ycrd, pszMsg));
					PA_DFX (pa_set_backbuffer_current ());
					GrPrintF ((grdCurCanv->cv_bitmap.bm_props.w-w)/2, ycrd, pszMsg);
					strcpy (szDisplayedBackgroundMsg [nVRCurrentPage], pszMsg);
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
			tObject *gmP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer];
			if (gmP && 
				 (gmP->nType == OBJ_WEAPON) && 
				 (gmP->id == GUIDEDMSL_ID) &&
			    (gmP->nSignature == gameData.objs.guidedMissileSig [gameData.multiplayer.nLocalPlayer]))
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
else if (GetCurrentGameScreen ()->cv_bitmap.bm_props.nType == BM_MODEX) {
	if (nModexHUDMsgs) {
		int temp = nLastMsgYCrd;
		nModexHUDMsgs--;
		ClearBackgroundMessages ();			//	If in status bar mode and no messages, then erase.
		nLastMsgYCrd = temp;
		}
	}
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
// Call to flash a message on the HUD.  Returns true if message drawn.
// (pszMsg might not be drawn if previous pszMsg was same)
int HUDInitMessageVA (ubyte nType, char * format, va_list args)
{
	tHUDMessage *pMsgs = gameData.hud.msgs + (gameOpts->render.cockpit.bSplitHUDMsgs ? nType : 0);
	int			temp;
	char			*pszMsg = NULL, 
					*pszLastMsg = NULL;
	char			con_message [HUD_MESSAGE_LENGTH + 3];

if (!gameOpts->render.cockpit.bHUDMsgs)
	return 0;
nModexHUDMsgs = 2;
if ((pMsgs->nLast < 0) || (pMsgs->nLast >= HUD_MAX_MSGS))
	Int3 (); // Get Rob!!
pszMsg = pMsgs->szMsgs [pMsgs->nLast];
vsprintf (pszMsg, format, args);
/* Produce a colorised version and send it to the console */
con_message [0] = CC_COLOR;
if (pMsgs->nColor != -1) {
	con_message [1] = (char) RGBA_RED (pMsgs->nColor) / 2 + 128;
	con_message [2] = (char) RGBA_GREEN (pMsgs->nColor) / 2 + 128;
	con_message [3] = (char) RGBA_BLUE (pMsgs->nColor) / 2 + 128;
	}
else if (nType) {
	con_message [1] = (char) (127 + 128);
	con_message [2] = (char) (95 + 128);
	con_message [3] = (char) (0 + 128);
	}
else {
	con_message [1] = (char) (0 + 128);
	con_message [3] = (char) (0 + 128);
	con_message [2] = (char) (63 + 128);
	}
con_message [4] = '\0';
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
	int ret = 0;
	
if (gameOpts->render.cockpit.bHUDMsgs) {
	va_list args;

	va_start (args, format);
	ret = HUDInitMessageVA (0, format, args);
	va_end (args);
	}
return ret;
}

//------------------------------------------------------------------------------

void PlayerDeadMessage (void)
{
if (gameOpts->render.cockpit.bHUDMsgs && gameStates.app.bPlayerExploded) {
	tHUDMessage	*pMsgs = gameData.hud.msgs;

   if ( gameData.multiplayer.players [gameData.multiplayer.nLocalPlayer].lives < 2) {
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
// 	if (gameData.multiplayer.players [gameData.multiplayer.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)
// 		InitHUDMessage ("Afterburner engaged.");
// 	else
// 		InitHUDMessage ("Afterburner disengaged.");
// }

void _CDECL_ HUDMessage (int nClass, char *format, ...)
{
if (gameOpts->render.cockpit.bHUDMsgs &&
	 (!bNoMsgRedundancy || (nClass & MSGC_NOREDUNDANCY)) &&
	 (!bMSGPlayerMsgs || !(gameData.app.nGameMode & GM_MULTI) || (nClass & MSGC_PLAYERMESSAGES))) {
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

if (gameOpts->render.cockpit.bHUDMsgs) {
	va_start (vp, format);
	HUDInitMessageVA (1, format, vp);
	va_end (vp);
	}
}

//------------------------------------------------------------------------------
//eof
