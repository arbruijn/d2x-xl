/* $Id: gamerend.c, v 1.13 2003/10/12 09:38:48 btb Exp $ */
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
 * Stuff for rendering the HUD
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: gamerend.c, v 1.13 2003/10/12 09:38:48 btb Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "error.h"
#include "mono.h"
#include "gr.h"
#include "palette.h"
#include "ibitblt.h"
#include "bm.h"
#include "player.h"
#include "cameras.h"
#include "render.h"
#include "menu.h"
#include "newmenu.h"
#include "screens.h"
#include "fix.h"
#include "robot.h"
#include "game.h"
#include "gauges.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "multi.h"
#include "endlevel.h"
#include "reactor.h"
#include "powerup.h"
#include "laser.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "loadgame.h"
#include "network.h"
#include "hudmsg.h"
#include "gamepal.h"
#include "lightning.h"

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_bitmap.h"

extern int LinearSVGABuffer;

#ifdef _DEBUG
extern int Debug_pause;				//John's debugging pause system
#endif

#ifdef _DEBUG
extern int bSavingMovieFrames;
#else
#define bSavingMovieFrames 0
#endif

//------------------------------------------------------------------------------

void UpdateCockpits (int bForceRedraw);

// Returns the length of the first 'n' characters of a string.
int string_width (char * s, int n)
{
	int w, h, aw;
	char p = s [n];

s [n] = 0;
GrGetStringSize (s, &w, &h, &aw);
s [n] = p;
return w;
}

//------------------------------------------------------------------------------
// Draw string 's' centered on a canvas... if wider than
// canvas, then wrap it.
void DrawCenteredText (int y, char * s)
{
	char p;
	int i, l = (int) strlen (s);

if (string_width (s, l) < grdCurCanv->cvBitmap.bmProps.w)	{
	GrString (0x8000, y, s, NULL);
	return;
	}

for (i=0; i<l; i++)	{
	if (string_width (s, i) > (grdCurCanv->cvBitmap.bmProps.w - 16))	{
		p = s [i];
		s [i] = 0;
		GrString (0x8000, y, s, NULL);
		s [i] = p;
		GrString (0x8000, y+grdCurCanv->cvFont->ftHeight+1, &s [i], NULL);
		return;
		}
	}
}

//------------------------------------------------------------------------------

#define MAX_MARKER_MESSAGE_LEN 120
void GameDrawMarkerMessage ()
{
	char temp_string [MAX_MARKER_MESSAGE_LEN+25];

if (gameData.marker.nDefiningMsg) {
	GrSetCurFont (GAME_FONT);    //GAME_FONT 
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
   sprintf (temp_string, TXT_DEF_MARKER, gameData.marker.szInput);
	DrawCenteredText (grdCurCanv->cvBitmap.bmProps.h/2-16, temp_string);
   }
}

//------------------------------------------------------------------------------

void GameDrawMultiMessage ()
{
	char temp_string [MAX_MULTI_MESSAGE_LEN+25];

if ((gameData.app.nGameMode&GM_MULTI) && (gameData.multigame.msg.bSending))	{
	GrSetCurFont (GAME_FONT);    //GAME_FONT);
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	sprintf (temp_string, "%s: %s_", TXT_MESSAGE, gameData.multigame.msg.szMsg);
	DrawCenteredText (grdCurCanv->cvBitmap.bmProps.h/2-16, temp_string);
	}
if ((gameData.app.nGameMode&GM_MULTI) && (gameData.multigame.msg.bDefining))	{
	GrSetCurFont (GAME_FONT);    //GAME_FONT);
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	sprintf (temp_string, "%s #%d: %s_", TXT_MACRO, gameData.multigame.msg.bDefining, gameData.multigame.msg.szMsg);
	DrawCenteredText (grdCurCanv->cvBitmap.bmProps.h/2-16, temp_string);
	}
}

//------------------------------------------------------------------------------
//these should be in gr.h 
#define cv_w  cvBitmap.bmProps.w
#define cv_h  cvBitmap.bmProps.h

//------------------------------------------------------------------------------
#ifdef _DEBUG

fix ShowView_textTimer = -1;

void DrawWindowLabel ()
{
if (ShowView_textTimer > 0) {
	char *viewer_name, *control_name;
	char	*viewer_id;
	ShowView_textTimer -= gameData.time.xFrame;
	GrSetCurFont (GAME_FONT);

	viewer_id = "";
	switch (gameData.objs.viewer->nType) {
		case OBJ_FIREBALL:
			viewer_name = "Fireball"; 
			break;
		case OBJ_ROBOT:	
			viewer_name = "Robot";
#ifdef EDITOR
			viewer_id = gameData.bots.names [gameData.objs.viewer->id];
#endif
			break;
		case OBJ_HOSTAGE:	
			viewer_name = "Hostage"; 
			break;
		case OBJ_PLAYER:	
			viewer_name = "Player"; 
			break;
		case OBJ_WEAPON:	
			viewer_name = "Weapon"; 
			break;
		case OBJ_CAMERA:	
			viewer_name = "Camera"; 
			break;
		case OBJ_POWERUP:	
			viewer_name = "Powerup";
#ifdef EDITOR
			viewer_id = Powerup_names [gameData.objs.viewer->id];
#endif
			break;
		case OBJ_DEBRIS:	
			viewer_name = "Debris"; 
			break;
		case OBJ_REACTOR:
			viewer_name = "Reactor"; 
			break;
		default:				
			viewer_name = "Unknown"; 
			break;
		}

	switch (gameData.objs.viewer->controlType) {
		case CT_NONE:		
			control_name = "Stopped"; 
			break;
		case CT_AI:			
			control_name = "AI"; 
			break;
		case CT_FLYING:	
			control_name = "Flying"; 
			break;
		case CT_SLEW:		
			control_name = "Slew"; 
			break;
		case CT_FLYTHROUGH:
			control_name = "Flythrough"; 
			break;
		case CT_MORPH:		
			control_name = "Morphing"; 
			break;
		default:				
			control_name = "Unknown"; 
			break;
		}
	GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, 45, "%i: %s [%s] View - %s", OBJ_IDX (gameData.objs.viewer), viewer_name, viewer_id, control_name);
	}
}
#endif

//------------------------------------------------------------------------------

void RenderCountdownGauge ()
{
if (!gameStates.app.bEndLevelSequence && gameData.reactor.bDestroyed  && (gameData.reactor.countdown.nSecsLeft>-1)) { // && (gameData.reactor.countdown.nSecsLeft<127))	{
	int	y;

	if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {    // no countdown on registered only
		//	On last level, we don't want a countdown.
		if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
			(gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {
			if (!(gameData.app.nGameMode & GM_MULTI))
				return;
			if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
				return;
			}
		}
	GrSetCurFont (SMALL_FONT);
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
	y = SMALL_FONT->ftHeight*4;
	if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
		y += SMALL_FONT->ftHeight*2;
	if (gameStates.app.bPlayerIsDead)
		y += SMALL_FONT->ftHeight*2;
	GrPrintF (NULL, 0x8000, y, "T-%d s", gameData.reactor.countdown.nSecsLeft);
	}
}

//------------------------------------------------------------------------------

void GameDrawHUDStuff ()
{
#ifdef _DEBUG
if (Debug_pause) {
	GrSetCurFont (MEDIUM1_FONT);
	GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
	GrUString (0x8000, 85/2, "Debug Pause - Press P to exit");
	}
DrawWindowLabel ();
#endif

GameDrawMultiMessage ();
GameDrawMarkerMessage ();
if ((gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.demo.nState == ND_STATE_RECORDING)) {
	char message [128];
	int h, w, aw;

	if (gameData.demo.nState == ND_STATE_PLAYBACK) {
		if (gameData.demo.nVcrState != ND_STATE_PRINTSCREEN) {
			sprintf (message, "%s (%d%%%% %s)", 
						gameOpts->demo.bRevertFormat ? TXT_DEMO_CONVERSION : TXT_DEMO_PLAYBACK, NDGetPercentDone (), TXT_DONE);
			}
		else {
			sprintf (message, " ");
			}
		}
	else 
		sprintf (message, TXT_DEMO_RECORDING);
	GrSetCurFont (GAME_FONT);    //GAME_FONT);
	GrSetFontColorRGBi (RGBA_PAL2 (27, 0, 0), 1, 0, 0);
	GrGetStringSize (message, &w, &h, &aw);
	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		if (grdCurCanv->cvBitmap.bmProps.h > 240)
			h += 40;
		else
			h += 15;
		}
	else if (gameStates.render.cockpit.nMode == CM_LETTERBOX)
		h += 7;
	if (gameStates.render.cockpit.nMode != CM_REAR_VIEW && !bSavingMovieFrames)
		GrPrintF (NULL, (grdCurCanv->cvBitmap.bmProps.w-w)/2, grdCurCanv->cvBitmap.bmProps.h - h - 2, message);
	}
if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) {
	RenderCountdownGauge ();
	if ((gameData.multiplayer.nLocalPlayer > -1) && 
		 (gameData.objs.viewer->nType == OBJ_PLAYER) && 
		 (gameData.objs.viewer->id == gameData.multiplayer.nLocalPlayer))	{
		int	x = 3;
		int	y = grdCurCanv->cvBitmap.bmProps.h;

		GrSetCurFont (GAME_FONT);
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		if (gameStates.input.nCruiseSpeed > 0) {
			int line_spacing = GAME_FONT->ftHeight + GAME_FONT->ftHeight/4;

			if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN) {
				if (gameData.app.nGameMode & GM_MULTI)
					y -= line_spacing * 11;	//64
				else
					y -= line_spacing * 6;	//32
				}
			else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
				if (gameData.app.nGameMode & GM_MULTI)
					y -= line_spacing * 8;	//48
				else
					y -= line_spacing * 4;	//24
				}
			else {
				y = line_spacing * 2;	//12
				x = 20+2;
				}
			GrPrintF (NULL, x, y, "%s %2d%%", TXT_CRUISE, f2i (gameStates.input.nCruiseSpeed));
			}
		}
	}
ShowFrameRate ();
if ((gameData.demo.nState == ND_STATE_PLAYBACK))
	gameData.app.nGameMode = gameData.demo.nGameMode;
if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) 
	DrawHUD ();
if ((gameData.demo.nState == ND_STATE_PLAYBACK))
	gameData.app.nGameMode = GM_NORMAL;
if (gameStates.app.bPlayerIsDead)
	PlayerDeadMessage ();
}

//------------------------------------------------------------------------------

extern int gr_bitblt_dest_step_shift;
extern int gr_wait_for_retrace;
extern int gr_bitblt_double;

void ExpandRow (ubyte * dest, ubyte * src, int num_src_pixels)
{
	int i;

for (i = 0; i < num_src_pixels; i++) {
	*dest++ = *src;
	*dest++ = *src++;
	}
}

//------------------------------------------------------------------------------
// doubles the size in x or y of a bitmap in place.
void GameExpandBitmap (grsBitmap * bmp, uint flags)
{
	int i;
	ubyte * dptr, * sptr;

switch (flags & 3) {
	case 2:	// expand x
		Assert (bmp->bmProps.rowSize == bmp->bmProps.w*2);
		dptr = &bmp->bmTexBuf [(bmp->bmProps.h-1)*bmp->bmProps.rowSize];
		for (i=bmp->bmProps.h-1; i>=0; i--)	{
			ExpandRow (dptr, dptr, bmp->bmProps.w);
			dptr -= bmp->bmProps.rowSize;
			}
		bmp->bmProps.w *= 2;
		break;

	case 1:	// expand y
		dptr = &bmp->bmTexBuf [(2* (bmp->bmProps.h-1)+1)*bmp->bmProps.rowSize];
		sptr = &bmp->bmTexBuf [(bmp->bmProps.h-1)*bmp->bmProps.rowSize];
		for (i=bmp->bmProps.h-1; i>=0; i--)	{
			memcpy (dptr, sptr, bmp->bmProps.w);
			dptr -= bmp->bmProps.rowSize;
			memcpy (dptr, sptr, bmp->bmProps.w);
			dptr -= bmp->bmProps.rowSize;
			sptr -= bmp->bmProps.rowSize;
			}
		bmp->bmProps.h *= 2;
		break;

	case 3:	// expand x & y
		Assert (bmp->bmProps.rowSize == bmp->bmProps.w*2);
		dptr = &bmp->bmTexBuf [(2* (bmp->bmProps.h-1)+1)*bmp->bmProps.rowSize];
		sptr = &bmp->bmTexBuf [(bmp->bmProps.h-1)*bmp->bmProps.rowSize];
		for (i=bmp->bmProps.h-1; i>=0; i--)	{
			ExpandRow (dptr, sptr, bmp->bmProps.w);
			dptr -= bmp->bmProps.rowSize;
			ExpandRow (dptr, sptr, bmp->bmProps.w);
			dptr -= bmp->bmProps.rowSize;
			sptr -= bmp->bmProps.rowSize;
			}
		bmp->bmProps.w *= 2;
		bmp->bmProps.h *= 2;
		break;
	}
}

//------------------------------------------------------------------------------

static inline bool GuidedMissileActive (void)
{
tObject *gmObjP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
return gmObjP && 
		 (gmObjP->nType == OBJ_WEAPON) && 
		 (gmObjP->id == GUIDEDMSL_ID) && 
		 (gmObjP->nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature);
}

//------------------------------------------------------------------------------

extern int SW_drawn [2], SW_x [2], SW_y [2], SW_w [2], SW_h [2];

#if 0
//render a frame for the game in stereo
void game_render_frame_stereo ()
{
	int dw, dh, sw, sh;
	fix save_aspect;
	fix actual_eye_width;
	int actual_eye_offset;
	gsrCanvas RenderCanvas [2];
	int bNoDrawHUD = 0, bGMView = 0;
	tObject *gmObjP;

	save_aspect = grdCurScreen->scAspect;
	grdCurScreen->scAspect *= 2;	//Muck with aspect ratio

	sw = dw = gameStates.render.vr.buffers.render [0].cvBitmap.bmProps.w;
	sh = dh = gameStates.render.vr.buffers.render [0].cvBitmap.bmProps.h;

	if (gameStates.render.vr.nLowRes & 1)	{
		sh /= 2;			
		grdCurScreen->scAspect *= 2;  //Muck with aspect ratio	                        
	}
	if (gameStates.render.vr.nLowRes & 2)	{
		sw /= 2;			
		grdCurScreen->scAspect /= 2;  //Muck with aspect ratio	                        
	}

	GrInitSubCanvas (RenderCanvas [0, gameStates.render.vr.buffers.render, 0, 0, sw, sh);
	GrInitSubCanvas (RenderCanvas + 1, gameStates.render.vr.buffers.render + 1, 0, 0, sw, sh);

	// Draw the left eye's view
	if (gameStates.render.vr.nEyeSwitch)	{
		actual_eye_width = -gameStates.render.vr.xEyeWidth;
		actual_eye_offset = -gameStates.render.vr.nEyeOffset;
	} else {
		actual_eye_width = gameStates.render.vr.xEyeWidth;
		actual_eye_offset = gameStates.render.vr.nEyeOffset;
	}

	if ((bGMView = GuidedInMainView ()))
		actual_eye_offset = 0;

	GrSetCurrentCanvas (&RenderCanvas [0]);

	if (bGMView) {
		char *msg = "Guided Missile View";
		tObject *viewerSave = gameData.objs.viewer;
		int w, h, aw;

		gameData.objs.viewer = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
		UpdateRenderedData (0, gameData.objs.viewer, 0, 0);
		RenderFrame (0, 0);
		WakeupRenderedObjects (gameData.objs.viewer, 0);
		gameData.objs.viewer = viewerSave;

		GrSetCurFont (GAME_FONT);    //GAME_FONT);
		GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
		GrGetStringSize (msg, &w, &h, &aw);

		GrPrintF (NULL, (grdCurCanv->cvBitmap.bmProps.w-w)/2, 3, msg);

		glDisable (GL_DEPTH_TEST);
		DrawGuidedCrosshair ();
		glEnable (GL_DEPTH_TEST);
		HUDRenderMessageFrame ();
		bNoDrawHUD = 1;
	}
	else if (gameStates.render.bRearView)
		RenderFrame (actual_eye_width, 0);	// switch eye positions for rear view
	else
		RenderFrame (-actual_eye_width, 0);		// Left eye

	if (gameStates.render.vr.nLowRes)
		GameExpandBitmap (&RenderCanvas [0].cvBitmap, gameStates.render.vr.nLowRes);

	{	//render small window into left eye's canvas
		gsrCanvas *save=grdCurCanv;
		fix save_aspect2 = grdCurScreen->scAspect;
		grdCurScreen->scAspect = save_aspect*2;
		SW_drawn [0] = SW_drawn [1] = 0;
		ShowExtraViews ();
		GrSetCurrentCanvas (save);
		grdCurScreen->scAspect = save_aspect2;
	}

//NEWVR
	if (actual_eye_offset > 0) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (grdCurCanv->cvBitmap.bmProps.w-labs (actual_eye_offset)*2, 0, 
               grdCurCanv->cvBitmap.bmProps.w-1, grdCurCanv->cvBitmap.bmProps.h);
	} else if (actual_eye_offset < 0) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (0, 0, labs (actual_eye_offset)*2-1, grdCurCanv->cvBitmap.bmProps.h);
	}

	if (gameStates.render.vr.bShowHUD && !bNoDrawHUD)	{
		gsrCanvas tmp;
		if (actual_eye_offset < 0) {
			GrInitSubCanvas (&tmp, grdCurCanv, labs (actual_eye_offset*2), 0, grdCurCanv->cvBitmap.bmProps.w- (labs (actual_eye_offset)*2), grdCurCanv->cvBitmap.bmProps.h);
		} else {
			GrInitSubCanvas (&tmp, grdCurCanv, 0, 0, grdCurCanv->cvBitmap.bmProps.w- (labs (actual_eye_offset)*2), grdCurCanv->cvBitmap.bmProps.h);
		}
		GrSetCurrentCanvas (&tmp);
		GameDrawHUDStuff ();
	}


	// Draw the right eye's view
	GrSetCurrentCanvas (&RenderCanvas [1]);

	if (gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ())
		GrBitmap (0, 0, &RenderCanvas [0].cvBitmap);
	else {
		if (gameStates.render.bRearView)
			RenderFrame (-actual_eye_width, 0);	// switch eye positions for rear view
		else
			RenderFrame (actual_eye_width, 0);		// Right eye

		if (gameStates.render.vr.nLowRes)
			GameExpandBitmap (&RenderCanvas [1].cvBitmap, gameStates.render.vr.nLowRes);
		}


	{	//copy small window from left eye
	gsrCanvas temp;
	int w;
	for (w=0;w<2;w++) {
		if (SW_drawn [w]) {
			GrInitSubCanvas (&temp, &RenderCanvas [0], SW_x [w], SW_y [w], SW_w [w], SW_h [w]);
			GrBitmap (SW_x [w]+actual_eye_offset*2, SW_y [w], &temp.cvBitmap);
			}
		}
	}

//NEWVR
	if (actual_eye_offset>0) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (0, 0, labs (actual_eye_offset)*2-1, grdCurCanv->cvBitmap.bmProps.h);
	} else if (actual_eye_offset < 0)	{
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (grdCurCanv->cvBitmap.bmProps.w-labs (actual_eye_offset)*2, 0, 
               grdCurCanv->cvBitmap.bmProps.w-1, grdCurCanv->cvBitmap.bmProps.h);
	}

//NEWVR (Add the next 2 lines)
	if (gameStates.render.vr.bShowHUD && !bNoDrawHUD)	{
		gsrCanvas tmp;
		if (actual_eye_offset > 0) {
			GrInitSubCanvas (&tmp, grdCurCanv, labs (actual_eye_offset*2), 0, grdCurCanv->cvBitmap.bmProps.w- (labs (actual_eye_offset)*2), grdCurCanv->cvBitmap.bmProps.h);
		} else {
			GrInitSubCanvas (&tmp, grdCurCanv, 0, 0, grdCurCanv->cvBitmap.bmProps.w- (labs (actual_eye_offset)*2), grdCurCanv->cvBitmap.bmProps.h);
		}
		GrSetCurrentCanvas (&tmp);
		GameDrawHUDStuff ();
	}


	// Draws white and black registration encoding lines
	// and Accounts for pixel-shift adjustment in upcoming bitblts
	if (gameStates.render.vr.bUseRegCode)	{
		int width, height, quarter;

		width = RenderCanvas [0].cvBitmap.bmProps.w;
		height = RenderCanvas [0].cvBitmap.bmProps.h;
		quarter = width / 4;

		// black out left-hand tSide of left page

		// draw registration code for left eye
		if (gameStates.render.vr.nEyeSwitch)
			GrSetCurrentCanvas (&RenderCanvas [1]);
		else
			GrSetCurrentCanvas (&RenderCanvas [0]);
		GrSetColorRGB (255, 255, 255, 255);
		GrScanLine (0, quarter, height-1);
		GrSetColorRGB (0, 0, 0, 255);
		GrScanLine (quarter, width-1, height-1);

		if (gameStates.render.vr.nEyeSwitch)
			GrSetCurrentCanvas (&RenderCanvas [0]);
		else
			GrSetCurrentCanvas (&RenderCanvas [1]);
		GrSetColorRGB (255, 255, 255, 255);
		GrScanLine (0, quarter*3, height-1);
		GrSetColorRGB (0, 0, 0, 255);
		GrScanLine (quarter*3, width-1, height-1);
   }

 		// Copy left eye, then right eye
	if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING)
		gameStates.render.vr.nCurrentPage = !gameStates.render.vr.nCurrentPage;
	else 
		gameStates.render.vr.nCurrentPage = 0;
	GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);

//NEWVR

	if (gameStates.render.vr.bEyeOffsetChanged > 0)	{
		gameStates.render.vr.bEyeOffsetChanged--;
		GrClearCanvas (0);
	}

	sw = dw = gameStates.render.vr.buffers.render [0].cvBitmap.bmProps.w;
	sh = dh = gameStates.render.vr.buffers.render [0].cvBitmap.bmProps.h;

	// Copy left eye, then right eye
	gr_bitblt_dest_step_shift = 1;		// Skip every other scanline.

	if (gameStates.render.vr.nRenderMode == VR_INTERLACED) 	{
		if (actual_eye_offset > 0)	{
			int xoff = labs (actual_eye_offset);
			GrBmUBitBlt (dw-xoff, dh, xoff, 0, 0, 0, &RenderCanvas [0].cvBitmap, &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap);
			GrBmUBitBlt (dw-xoff, dh, 0, 1, xoff, 0, &RenderCanvas [1].cvBitmap, &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap);
		} else if (actual_eye_offset < 0)	{
			int xoff = labs (actual_eye_offset);
			GrBmUBitBlt (dw-xoff, dh, 0, 0, xoff, 0, &RenderCanvas [0].cvBitmap, &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap);
			GrBmUBitBlt (dw-xoff, dh, xoff, 1, 0, 0, &RenderCanvas [1].cvBitmap, &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap);
		} else {
			GrBmUBitBlt (dw, dh, 0, 0, 0, 0, &RenderCanvas [0].cvBitmap, &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap);
			GrBmUBitBlt (dw, dh, 0, 1, 0, 0, &RenderCanvas [1].cvBitmap, &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap);
		}
	} else if (gameStates.render.vr.nRenderMode == VR_AREA_DET) {
		// VFX copy
		GrBmUBitBlt (dw, dh, 0,  gameStates.render.vr.nCurrentPage, 0, 0, &RenderCanvas [0].cvBitmap, &gameStates.render.vr.buffers.screenPages [0].cvBitmap);
		GrBmUBitBlt (dw, dh, dw, gameStates.render.vr.nCurrentPage, 0, 0, &RenderCanvas [1].cvBitmap, &gameStates.render.vr.buffers.screenPages [0].cvBitmap);
	} else {
		Int3 ();		// Huh?
	}

	gr_bitblt_dest_step_shift = 0;

	//if (Game_vfxFlag)
	//	vfx_set_page (gameStates.render.vr.nCurrentPage);		// 0 or 1
	//else 
		if (gameStates.render.vr.nScreenFlags&VRF_USE_PAGING)	{
			gr_wait_for_retrace = 0;

//	Added by Samir from John's code
		if ((gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.nType == BM_MODEX) && (Game_3dmaxFlag==3))	{
			int old_x, old_y, new_x;
			old_x = gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.x;
			old_y = gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.y;
			new_x = old_y*gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.rowSize;
			new_x += old_x/4;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.x = new_x;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.y = 0;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.nType = BM_SVGA;
			GrShowCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.nType = BM_MODEX;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.x = old_x;
			gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap.bmProps.y = old_y;
		} else {
			GrShowCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
		}
		gr_wait_for_retrace = 1;
	}
	grdCurScreen->scAspect=save_aspect;
}
#endif

//------------------------------------------------------------------------------

ubyte nDemoDoingRight = 0, nDemoDoingLeft = 0;
extern ubyte nDemoDoRight, nDemoDoLeft;
extern tObject demoRightExtra, demoLeftExtra;

char DemoWBUType []={0, WBUMSL, WBUMSL, WBU_REAR, WBU_ESCORT, WBU_MARKER, WBUMSL};
char DemoRearCheck []={0, 0, 0, 1, 0, 0, 0};
const char *DemoExtraMessage []={"PLAYER", "GUIDED", "MISSILE", "REAR", "GUIDE-BOT", "MARKER", "SHIP"};

void ShowExtraViews ()
{
	int		bDidMissileView = 0;
	int		saveNewDemoState = gameData.demo.nState;
	tObject	*objP;
	int		w;

if (gameData.demo.nState==ND_STATE_PLAYBACK) {
   if (nDemoDoLeft) { 
      if (nDemoDoLeft == 3)
			DoCockpitWindowView (0, gameData.objs.console, 1, WBU_REAR, "REAR");
      else
			DoCockpitWindowView (0, &demoLeftExtra, DemoRearCheck [nDemoDoLeft], DemoWBUType [nDemoDoLeft], DemoExtraMessage [nDemoDoLeft]);
		}
   else
		DoCockpitWindowView (0, NULL, 0, WBU_WEAPON, NULL);
	if (nDemoDoRight) {
      if (nDemoDoRight==3)
			DoCockpitWindowView (1, gameData.objs.console, 1, WBU_REAR, "REAR");
      else
			DoCockpitWindowView (1, &demoRightExtra, DemoRearCheck [nDemoDoRight], DemoWBUType [nDemoDoRight], DemoExtraMessage [nDemoDoRight]);
		} 
   else
		DoCockpitWindowView (1, NULL, 0, WBU_WEAPON, NULL);
   nDemoDoLeft = nDemoDoRight = 0;
	nDemoDoingLeft = nDemoDoingRight = 0;
   return;
   } 
if ((objP = GuidedMslView ())) {
	if (gameOpts->render.cockpit.bGuidedInMainView)	{
		gameStates.render.nRenderingType = 6+ (1<<4);
		DoCockpitWindowView (1, gameData.objs.viewer, 0, WBUMSL, "SHIP");
		}
	else {
		gameStates.render.nRenderingType = 1+ (1<<4);
		DoCockpitWindowView (1, objP, 0, WBU_GUIDED, "GUIDED");
	   }
	bDidMissileView = 1;
	}
else {
	if (objP) {		//used to be active
		if (!gameOpts->render.cockpit.bGuidedInMainView)
			DoCockpitWindowView (1, NULL, 0, WBU_STATIC, NULL);
		gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer],objP = NULL;
		}
	if (gameData.objs.missileViewer && !gameStates.render.bExternalView) {		//do missile view
		static int mv_sig=-1;
		if (mv_sig == -1)
			mv_sig = gameData.objs.missileViewer->nSignature;
		if (/*!gameStates.app.bD1Mission && */ //allow in D1 levels
				gameOpts->render.cockpit.bMissileView && 
				(gameData.objs.missileViewer->nType != OBJ_NONE) && 
				(gameData.objs.missileViewer->nSignature == mv_sig)) {
  			gameStates.render.nRenderingType = 2 + (1<<4);
			DoCockpitWindowView (1, gameData.objs.missileViewer, 0, WBUMSL, "MISSILE");
			bDidMissileView = 1;
			}
		else {
			gameData.objs.missileViewer = NULL;
			mv_sig = -1;
			gameStates.render.nRenderingType = 255;
			DoCockpitWindowView (1, NULL, 0, WBU_STATIC, NULL);
			}
		}
	}
for (w = 0; w < 2 - bDidMissileView; w++) {
	//show special views if selected
	switch (gameStates.render.cockpit.n3DView [w]) {
		case CV_NONE:
			gameStates.render.nRenderingType = 255;
			DoCockpitWindowView (w, NULL, 0, WBU_WEAPON, NULL);
			break;

		case CV_REAR:
			if (gameStates.render.bRearView) {		//if big window is rear view, show front here
				gameStates.render.nRenderingType = 3+ (w<<4);			
				DoCockpitWindowView (w, gameData.objs.console, 0, WBU_REAR, "FRONT");
				}
			else {					//show normal rear view
				gameStates.render.nRenderingType = 3+ (w<<4);			
				DoCockpitWindowView (w, gameData.objs.console, 1, WBU_REAR, "REAR");
				}
			break;

		case CV_ESCORT: {
			tObject *buddy = find_escort ();
			if (!buddy) {
				DoCockpitWindowView (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			else {
				gameStates.render.nRenderingType = 4+ (w<<4);
				DoCockpitWindowView (w, buddy, 0, WBU_ESCORT, gameData.escort.szName);
				}
			break;
			}

		case CV_COOP: {
			int nPlayer = gameStates.render.cockpit.nCoopPlayerView [w];
	      gameStates.render.nRenderingType = 255; // don't handle coop stuff		
			if ((nPlayer != -1) && 
				 gameData.multiplayer.players [nPlayer].connected && 
				 (IsCoopGame || (IsTeamGame && (GetTeam (nPlayer) == GetTeam (gameData.multiplayer.nLocalPlayer)))))
				DoCockpitWindowView (w, &OBJECTS [gameData.multiplayer.players [gameStates.render.cockpit.nCoopPlayerView [w]].nObject], 0, WBU_COOP, gameData.multiplayer.players [gameStates.render.cockpit.nCoopPlayerView [w]].callsign);
			else {
				DoCockpitWindowView (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			break;
			}

		case CV_MARKER: {
			char label [10];
			short v = gameData.marker.viewers [w];
			gameStates.render.nRenderingType = 5+ (w<<4);
			if ((v == -1) || (gameData.marker.objects [v] == -1)) {
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				break;
				}
			sprintf (label, "Marker %d", gameData.marker.viewers [w]+1);
			DoCockpitWindowView (w, OBJECTS + gameData.marker.objects [gameData.marker.viewers [w]], 0, WBU_MARKER, label);
			break;
			}

		case CV_RADAR_TOPDOWN:
		case CV_RADAR_HEADSUP:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0))
				DoCockpitWindowView (w, gameData.objs.console, 0, 
					(gameStates.render.cockpit.n3DView [w] == CV_RADAR_TOPDOWN) ? WBU_RADAR_TOPDOWN : WBU_RADAR_HEADSUP, "MINI MAP");
			else
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
			break;
		default:
			Int3 ();		//invalid window nType
		}
	}
gameStates.render.nRenderingType = 0;
gameData.demo.nState = saveNewDemoState;
}

//------------------------------------------------------------------------------

extern ubyte * bGameCockpitCopyCode;

void DrawGuidedCrosshair (void);

void GameRenderFrameMono (void)
{
	gsrCanvas	Screen_3d_window;
	int			bNoDrawHUD = 0;

GrInitSubCanvas (
	&Screen_3d_window, &gameStates.render.vr.buffers.screenPages [0], 
	gameStates.render.vr.buffers.subRender [0].cvBitmap.bmProps.x, 
	gameStates.render.vr.buffers.subRender [0].cvBitmap.bmProps.y, 
	gameStates.render.vr.buffers.subRender [0].cvBitmap.bmProps.w, 
	gameStates.render.vr.buffers.subRender [0].cvBitmap.bmProps.h);
GrSetCurrentCanvas (&gameStates.render.vr.buffers.subRender [0]);

SetLightningLights ();
if (gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()) {
	int w, h, aw;
	const char *msg = "Guided Missile View";
	tObject *viewerSave = gameData.objs.viewer;

   if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		gameStates.render.cockpit.bBigWindowSwitch = 1;
		gameStates.render.cockpit.bRedraw = 1;
		gameStates.render.cockpit.nMode = CM_STATUS_BAR;
		return;
		}
  	gameData.objs.viewer = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
	UpdateRenderedData (0, gameData.objs.viewer, 0, 0);
	if (RenderCameras ())
		GrSetCurrentCanvas (&gameStates.render.vr.buffers.subRender [0]);
	RenderFrame (0, 0);
  	WakeupRenderedObjects (gameData.objs.viewer, 0);
	gameData.objs.viewer = viewerSave;
	GrSetCurFont (GAME_FONT);    //GAME_FONT);
	GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);
	GrGetStringSize (msg, &w, &h, &aw);
	GrPrintF (NULL, (grdCurCanv->cvBitmap.bmProps.w-w) / 2, 3, msg);
	DrawGuidedCrosshair ();
	HUDRenderMessageFrame ();
	bNoDrawHUD = 1;
	}
else {
	if (gameStates.render.cockpit.bBigWindowSwitch) {
		gameStates.render.cockpit.bRedraw = 1;
		gameStates.render.cockpit.nMode = CM_FULL_COCKPIT;
		gameStates.render.cockpit.bBigWindowSwitch = 0;
		return;
		}
	UpdateRenderedData (0, gameData.objs.viewer, gameStates.render.bRearView, 0);
	if (RenderCameras ())
		GrSetCurrentCanvas (&gameStates.render.vr.buffers.subRender [0]);
	RenderFrame (0, 0);
	}
GrSetCurrentCanvas (&gameStates.render.vr.buffers.subRender [0]);
if (!bNoDrawHUD)
	GameDrawHUDStuff ();

if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT)
	ShowExtraViews ();		//missile view, buddy bot, etc.
if (!bGameCockpitCopyCode)	{
	if (gameStates.render.vr.nScreenFlags & VRF_USE_PAGING)	{
		gameStates.render.vr.nCurrentPage = !gameStates.render.vr.nCurrentPage;
		GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
		GrBmUBitBlt (gameStates.render.vr.buffers.subRender [0].cv_w, 
						 gameStates.render.vr.buffers.subRender [0].cv_h, 
						 gameStates.render.vr.buffers.subRender [0].cvBitmap.bmProps.x, 
						 gameStates.render.vr.buffers.subRender [0].cvBitmap.bmProps.y, 
						 0, 0, 
						 &gameStates.render.vr.buffers.subRender [0].cvBitmap, 
						 &gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage].cvBitmap, 1);
		gr_wait_for_retrace = 0;
		GrShowCanvas (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
		gr_wait_for_retrace = 1;
		}
	}

if (SHOW_COCKPIT) {
	if (gameData.demo.nState == ND_STATE_PLAYBACK)
		gameData.app.nGameMode = gameData.demo.nGameMode;
	glDepthFunc (GL_ALWAYS);
	if (SHOW_COCKPIT) {
		UpdateCockpits (1);
		ShowExtraViews ();		//missile view, buddy bot, etc.
		}
	RenderGauges ();
	HUDShowIcons ();
	if (gameData.demo.nState == ND_STATE_PLAYBACK)
		gameData.app.nGameMode = GM_NORMAL;
	}
else if (gameStates.render.cockpit.nMode == CM_REAR_VIEW)
	UpdateCockpits (1);
else
	HUDShowIcons ();
con_update ();
OglSwapBuffers (0, 0);
if (gameStates.app.bSaveScreenshot)
	SaveScreenShot (NULL, 0);
}

//------------------------------------------------------------------------------

#define WINDOW_W_DELTA	 ((gameData.render.window.wMax / 16)&~1)	//24	//20
#define WINDOW_H_DELTA	 ((gameData.render.window.hMax / 16)&~1)	//12	//10

#define WINDOW_MIN_W		 ((gameData.render.window.wMax * 10) / 22)	//160
#define WINDOW_MIN_H		 ((gameData.render.window.hMax * 10) / 22)

void GrowWindow ()
{
StopTime ();
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
	gameData.render.window.h = gameData.render.window.hMax;
	gameData.render.window.w = gameData.render.window.wMax;
	ToggleCockpit ();
	HUDInitMessage (TXT_COCKPIT_F3);
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nMode != CM_STATUS_BAR && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	StartTime (0);
	return;
	}

if (gameData.render.window.h>=gameData.render.window.hMax || gameData.render.window.w>=gameData.render.window.wMax) {
	//gameData.render.window.w = gameData.render.window.wMax;
	//gameData.render.window.h = gameData.render.window.hMax;
	SelectCockpit (CM_FULL_SCREEN);
	} 
else {
	//int x, y;
	gameData.render.window.w += WINDOW_W_DELTA;
	gameData.render.window.h += WINDOW_H_DELTA;
	if (gameData.render.window.h > gameData.render.window.hMax)
		gameData.render.window.h = gameData.render.window.hMax;
	if (gameData.render.window.w > gameData.render.window.wMax)
		gameData.render.window.w = gameData.render.window.wMax;
	gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
	gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
	GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
	}
HUDClearMessages ();	//	@mk, 11/11/94
WritePlayerFile ();
StartTime (0);
}

//------------------------------------------------------------------------------
// grsBitmap bmBackground;	already declared in line 434 (samir 4/10/94)

extern grsBitmap bmBackground;

void copy_background_rect (int left, int top, int right, int bot)
{
	grsBitmap *bm = &bmBackground;
	int x, y;
	int tile_left, tile_right, tile_top, tile_bot;
	int ofs_x, ofs_y;
	int dest_x, dest_y;

	if (right < left || bot < top)
		return;

	tile_left = left / bm->bmProps.w;
	tile_right = right / bm->bmProps.w;
	tile_top = top / bm->bmProps.h;
	tile_bot = bot / bm->bmProps.h;

	ofs_y = top % bm->bmProps.h;
	dest_y = top;

{
	for (y=tile_top;y<=tile_bot;y++) {
		int w, h;

		ofs_x = left % bm->bmProps.w;
		dest_x = left;

		//h = (bot < dest_y+bm->bmProps.h)? (bot-dest_y+1): (bm->bmProps.h-ofs_y);
		h = min (bot-dest_y+1, bm->bmProps.h-ofs_y);
		for (x=tile_left;x<=tile_right;x++) {
			//w = (right < dest_x+bm->bmProps.w)? (right-dest_x+1): (bm->bmProps.w-ofs_x);
			w = min (right-dest_x+1, bm->bmProps.w-ofs_x);
			GrBmUBitBlt (w, h, dest_x, dest_y, ofs_x, ofs_y, &bmBackground, &grdCurCanv->cvBitmap, 1);
			ofs_x = 0;
			dest_x += w;
			}
		ofs_y = 0;
		dest_y += h;
		}
	} 
}

//------------------------------------------------------------------------------
//fills int the background surrounding the 3d window
void FillBackground ()
{
	int x, y, w, h, dx, dy;

	x = gameData.render.window.x;
	y = gameData.render.window.y;
	w = gameData.render.window.w;
	h = gameData.render.window.h;

	dx = x;
	dy = y;

	GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
	copy_background_rect (x-dx, y-dy, x-1, y+h+dy-1);
	copy_background_rect (x+w, y-dy, grdCurCanv->cv_w-1, y+h+dy-1);
	copy_background_rect (x, y-dy, x+w-1, y-1);
	copy_background_rect (x, y+h, x+w-1, y+h+dy-1);

	if (gameStates.render.vr.nScreenFlags & VRF_USE_PAGING) {
		GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [!gameStates.render.vr.nCurrentPage]);
		copy_background_rect (x-dx, y-dy, x-1, y+h+dy-1);
		copy_background_rect (x+w, y-dy, x+w+dx-1, y+h+dy-1);
		copy_background_rect (x, y-dy, x+w-1, y-1);
		copy_background_rect (x, y+h, x+w-1, y+h+dy-1);
	}

}

//------------------------------------------------------------------------------

void ShrinkWindow ()
{
StopTime ();
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	gameData.render.window.h = gameData.render.window.hMax;
	gameData.render.window.w = gameData.render.window.wMax;
	//!!ToggleCockpit ();
	gameStates.render.cockpit.nNextMode = CM_FULL_COCKPIT;
	SelectCockpit (CM_STATUS_BAR);
//		ShrinkWindow ();
//		ShrinkWindow ();
	HUDInitMessage (TXT_COCKPIT_F3);
	WritePlayerFile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	//gameData.render.window.w = gameData.render.window.wMax;
	//gameData.render.window.h = gameData.render.window.hMax;
	SelectCockpit (CM_STATUS_BAR);
	WritePlayerFile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nMode != CM_STATUS_BAR && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	StartTime (0);
	return;
	}

#if TRACE
con_printf (CONDBG, "Cockpit mode=%d\n", gameStates.render.cockpit.nMode);
#endif
if (gameData.render.window.w > WINDOW_MIN_W) {
	//int x, y;

   gameData.render.window.w -= WINDOW_W_DELTA;
	gameData.render.window.h -= WINDOW_H_DELTA;

#if TRACE
  con_printf (CONDBG, "NewW=%d NewH=%d VW=%d maxH=%d\n", gameData.render.window.w, gameData.render.window.h, gameData.render.window.wMax, gameData.render.window.hMax);
#endif                  
	if (gameData.render.window.w < WINDOW_MIN_W)
		gameData.render.window.w = WINDOW_MIN_W;

	if (gameData.render.window.h < WINDOW_MIN_H)
		gameData.render.window.h = WINDOW_MIN_H;
	
	gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
	gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;

	FillBackground ();

	GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
	HUDClearMessages ();
	WritePlayerFile ();
	}
StartTime (0);
}

//------------------------------------------------------------------------------

static 
#ifndef _DEBUG
inline 
#endif
void DrawCockpit (int h, int y)
{
if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) {
	int i = gameData.pig.tex.cockpitBmIndex [h].index;
	grsBitmap *bmP = gameData.pig.tex.bitmaps [0] + i; 
	grsColor c;

	PIGGY_PAGE_IN (gameData.pig.tex.cockpitBmIndex [h].index, 0);
	OglLoadBmTexture (bmP, 0, 3, 1);
   GrSetCurrentCanvas (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
	c.index = 255;
	c.rgb = 0;
	OglUBitMapMC (0, y, -1, grdCurCanv->cvBitmap.bmProps.h - y, bmP, &c, F1_0, 0);
	}
}

//------------------------------------------------------------------------------

// This actually renders the new cockpit onto the screen.
void UpdateCockpits (int bForceRedraw)
{
	//int x, y, w, h;
	int nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.models.nCockpits / 2 : 0;

if ((gameStates.render.cockpit.nMode != gameStates.render.cockpit.nLastDrawn [gameStates.render.vr.nCurrentPage]) && !bForceRedraw)
	return;
gameStates.render.cockpit.nLastDrawn [gameStates.render.vr.nCurrentPage] = gameStates.render.cockpit.nMode;
//Redraw the on-screen cockpit bitmaps
if (gameStates.render.vr.nRenderMode != VR_NONE)
	return;
switch (gameStates.render.cockpit.nMode)	{
	case CM_FULL_COCKPIT:
		DrawCockpit (nCockpit, 0);
		if (bForceRedraw)
			return;
		break;

	case CM_REAR_VIEW:
		GrSetCurrentCanvas (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
		DrawCockpit (gameStates.render.cockpit.nMode + nCockpit, 0);
		if (bForceRedraw)
			return;
		break;

	case CM_FULL_SCREEN:
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
		FillBackground ();
		break;

	case CM_STATUS_BAR:
		DrawCockpit (gameStates.render.cockpit.nMode + nCockpit, gameData.render.window.hMax);
		gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
		gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
		FillBackground ();
		break;

	case CM_LETTERBOX:
		GrSetCurrentCanvas (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
		GrClearCanvas (BLACK_RGBA);
		// In a modex mode, clear the other buffer.
		if (grdCurCanv->cvBitmap.bmProps.nType == BM_MODEX) {
			GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage^1]);
			GrClearCanvas (BLACK_RGBA);
			GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
			}
		break;
	}
GrSetCurrentCanvas (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);
if (SHOW_COCKPIT)
	InitGauges ();
}

//------------------------------------------------------------------------------

void GameRenderFrame ()
{
PROF_START
SetScreenMode (SCREEN_GAME);
PlayHomingWarning ();
GrPaletteStepLoad (gamePalette);
if (gameStates.render.vr.nRenderMode == VR_NONE)
	GameRenderFrameMono ();	 
StopTime ();
GrPaletteFadeIn (NULL, 32, 0);
StartTime (0);
gameData.app.nFrameCount++;
PROF_END(ptRenderMine)
}

//------------------------------------------------------------------------------

//draw a crosshair for the guided missile
void DrawGuidedCrosshair (void)
{
	int x, y, w, h;

	GrSetColorRGBi (RGBA_PAL (0, 31, 0));

	w = grdCurCanv->cv_w>>5;
	if (w < 5)
		w = 5;

	h = i2f (w) / grdCurScreen->scAspect;

	x = grdCurCanv->cv_w / 2;
	y = grdCurCanv->cv_h / 2;

//	GrScanLine (x-w/2, x+w/2, y);
#if 1
	x = i2f (x);
	y = i2f (y);
	w = i2f (w / 2);
	h = i2f (h / 2);
	gr_uline (x - w, y, x + w, y);
	gr_uline (x, y - h, x, y + h);
#else
	gr_uline (i2f (x-w/2), i2f (y), i2f (x+w/2), i2f (y));
	gr_uline (i2f (x), i2f (y-h/2), i2f (x), i2f (y+h/2));
#endif
}

//------------------------------------------------------------------------------

bkg bg = {0, 0, 0, 0, NULL, NULL, NULL, NULL, 1, 1};

#define BOX_BORDER (gameStates.menus.bHires?60:30)

//show a message in a nice little box
void ShowBoxedMessage (const char *pszMsg)
{
	int w, h, aw;
	int x, y;
	//ubyte save_pal [256*3];

	//memcpy (save_pal, grPalette, sizeof (save_pal));
if (bg.bmp) {
	GrFreeBitmap (bg.bmp);
	bg.bmp = NULL;
	}
GrSetCurrentCanvas (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
GrSetCurFont (MEDIUM1_FONT);
GrGetStringSize (pszMsg, &w, &h, &aw);
x = (grdCurScreen->scWidth-w)/2;
y = (grdCurScreen->scHeight-h)/2;
// Save the background of the display
bg.x = x; 
bg.y = y; 
bg.w = w; 
bg.h = h;
if (!gameOpts->menus.nStyle) {
	bg.bmp = GrCreateBitmap (w+BOX_BORDER, h+BOX_BORDER, 1);
	GrBmUBitBlt (w+BOX_BORDER, h+BOX_BORDER, 0, 0, x-BOX_BORDER/2, y-BOX_BORDER/2, &(grdCurCanv->cvBitmap), bg.bmp, 1);
	}
NMDrawBackground (&bg, x-BOX_BORDER/2, y-BOX_BORDER/2, x+w+BOX_BORDER/2-1, y+h+BOX_BORDER/2-1, 0);
GrSetFontColorRGBi (DKGRAY_RGBA, 1, 0, 0);
GrSetCurFont (MEDIUM1_FONT);
GrPrintF (NULL, 0x8000, y, pszMsg);
GrUpdate (0);
NMRemoveBackground (&bg);
}

//------------------------------------------------------------------------------

void ClearBoxedMessage ()
{
if (bg.bmp) {
	GrBitmap (bg.x-BOX_BORDER/2, bg.y-BOX_BORDER/2, bg.bmp);
	if (bg.bmp) {
		GrFreeBitmap (bg.bmp);
		bg.bmp = NULL;
		}
	}
}

//------------------------------------------------------------------------------
//eof

