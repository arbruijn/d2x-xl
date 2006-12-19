/* $Id: gamerend.c,v 1.13 2003/10/12 09:38:48 btb Exp $ */
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
static char rcsid[] = "$Id: gamerend.c,v 1.13 2003/10/12 09:38:48 btb Exp $";
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
#include "cntrlcen.h"
#include "powerup.h"
#include "laser.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "gameseq.h"
#include "network.h"
#include "hudmsg.h"
#include "gamepal.h"

#include "ogl_init.h"

extern int LinearSVGABuffer;

#ifndef NDEBUG
extern int Debug_pause;				//John's debugging pause system
#endif

#ifndef RELEASE
extern int Saving_movie_frames;
#else
#define Saving_movie_frames 0
#endif

//------------------------------------------------------------------------------
int bSaveScreenShot = 0;

void update_cockpits(int force_redraw);

// Returns the length of the first 'n' characters of a string.
int string_width( char * s, int n )
{
	int w,h,aw;
	char p;
	p = s[n];
	s[n] = 0;
	GrGetStringSize( s, &w, &h, &aw );
	s[n] = p;
	return w;
}

//------------------------------------------------------------------------------
// Draw string 's' centered on a canvas... if wider than
// canvas, then wrap it.
void DrawCenteredText( int y, char * s )
{
	int i, l;
	char p;

	l = (int) strlen(s);

	if ( string_width( s, l ) < grdCurCanv->cv_bitmap.bm_props.w )	{
		GrString( 0x8000, y, s );
		return;
	}

	for (i=0; i<l; i++ )	{
		if ( string_width(s,i) > (grdCurCanv->cv_bitmap.bm_props.w - 16) )	{
			p = s[i];
			s[i] = 0;
			GrString( 0x8000, y, s );
			s[i] = p;
			GrString( 0x8000, y+grdCurCanv->cv_font->ft_h+1, &s[i] );
			return;
		}
	}
}

//------------------------------------------------------------------------------

#define MAX_MARKER_MESSAGE_LEN 120
void game_draw_marker_message()
{
	char temp_string[MAX_MARKER_MESSAGE_LEN+25];

if (gameData.marker.nDefiningMsg) {
	GrSetCurFont( GAME_FONT );    //GAME_FONT 
	GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
   sprintf( temp_string, TXT_DEF_MARKER, gameData.marker.szInput );
	DrawCenteredText(grdCurCanv->cv_bitmap.bm_props.h/2-16, temp_string );
   }
}

//------------------------------------------------------------------------------

void game_draw_multi_message()
{
	char temp_string[MAX_MULTI_MESSAGE_LEN+25];

	if ( (gameData.app.nGameMode&GM_MULTI) && (multiData.msg.bSending))	{
		GrSetCurFont( GAME_FONT );    //GAME_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		sprintf( temp_string, "%s: %s_", TXT_MESSAGE, multiData.msg.szMsg );
		DrawCenteredText(grdCurCanv->cv_bitmap.bm_props.h/2-16, temp_string );

	}

	if ( (gameData.app.nGameMode&GM_MULTI) && (multiData.msg.bDefining))	{
		GrSetCurFont( GAME_FONT );    //GAME_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		sprintf( temp_string, "%s #%d: %s_", TXT_MACRO, multiData.msg.bDefining, multiData.msg.szMsg );
		DrawCenteredText(grdCurCanv->cv_bitmap.bm_props.h/2-16, temp_string );
	}
}

//------------------------------------------------------------------------------
//these should be in gr.h 
#define cv_w  cv_bitmap.bm_props.w
#define cv_h  cv_bitmap.bm_props.h

fix frameTime_list[8] = {0,0,0,0,0,0,0,0};
fix frameTimeTotal=0;
int frameTime_cntr=0;

void ftoa(char *string, fix f)
{
	int decimal, fractional;
	
	decimal = f2i(f);
	fractional = ((f & 0xffff)*100)/65536;
	if (fractional < 0 )
		fractional *= -1;
	if (fractional > 99 ) fractional = 99;
	sprintf( string, "%d.%02d", decimal, fractional );
}

//------------------------------------------------------------------------------

void show_framerate()
{
	char temp[50];
	static time_t t, t0 = -1;
	static fix rate = 0;
	int x = 8, y = 5; // position measured from lower right corner
   //static int q;

	frameTimeTotal += gameData.time.xRealFrame - frameTime_list[frameTime_cntr];
	frameTime_list[frameTime_cntr] = gameData.time.xRealFrame;
	frameTime_cntr = (frameTime_cntr+1)%8;
	t = SDL_GetTicks ();
	if ((t0 < 0) || (t - t0 >= 500)) {
		t0 = t;
		rate = frameTimeTotal ? FixDiv(f1_0*8,frameTimeTotal) : 0;
		}
	GrSetCurFont( GAME_FONT );	
	GrSetFontColorRGBi (ORANGE_RGBA, 1, 0, 0);

	ftoa( temp, rate );	// Convert fixed to string
	if (gameData.app.nGameMode & GM_MULTI)
		y = 7;
	GrPrintF(grdCurCanv->cv_w-(x*GAME_FONT->ft_w),grdCurCanv->cv_h-y*(GAME_FONT->ft_h+GAME_FONT->ft_h/4),"FPS: %s ", temp );
}

//------------------------------------------------------------------------------
#ifndef NDEBUG

fix Show_view_textTimer = -1;

void draw_window_label()
{
	if ( Show_view_textTimer > 0 )
	{
		char *viewer_name,*control_name;
		char	*viewer_id;
		Show_view_textTimer -= gameData.time.xFrame;
		GrSetCurFont( GAME_FONT );

		viewer_id = "";
		switch( gameData.objs.viewer->nType )
		{
			case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
			case OBJ_ROBOT:		viewer_name = "Robot";
#ifdef EDITOR
										viewer_id = gameData.bots.names[gameData.objs.viewer->id];
#endif
				break;
			case OBJ_HOSTAGE:		viewer_name = "Hostage"; break;
			case OBJ_PLAYER:		viewer_name = "Player"; break;
			case OBJ_WEAPON:		viewer_name = "Weapon"; break;
			case OBJ_CAMERA:		viewer_name = "Camera"; break;
			case OBJ_POWERUP:		viewer_name = "Powerup";
#ifdef EDITOR
										viewer_id = Powerup_names[gameData.objs.viewer->id];
#endif
				break;
			case OBJ_DEBRIS:		viewer_name = "Debris"; break;
			case OBJ_CNTRLCEN:	viewer_name = "Reactor"; break;
			default:					viewer_name = "Unknown"; break;
		}

		switch ( gameData.objs.viewer->controlType) {
			case CT_NONE:			control_name = "Stopped"; break;
			case CT_AI:				control_name = "AI"; break;
			case CT_FLYING:		control_name = "Flying"; break;
			case CT_SLEW:			control_name = "Slew"; break;
			case CT_FLYTHROUGH:	control_name = "Flythrough"; break;
			case CT_MORPH:			control_name = "Morphing"; break;
			default:					control_name = "Unknown"; break;
		}

		GrSetFontColorRGBi (MEDRED_RGBA, 1, 0, 0);
		GrPrintF( 0x8000, 45, "%i: %s [%s] View - %s",OBJ_IDX (gameData.objs.viewer), viewer_name, viewer_id, control_name );

	}
}
#endif

//------------------------------------------------------------------------------

extern int Game_window_x;
extern int Game_window_y;
extern int Game_window_w;
extern int Game_window_h;
extern int max_window_w;
extern int max_window_h;

void render_countdown_gauge()
{
	if (!gameStates.app.bEndLevelSequence && gameData.reactor.bDestroyed  && (gameData.reactor.countdown.nSecsLeft>-1)) { // && (gameData.reactor.countdown.nSecsLeft<127))	{
		int	y;

		if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE)    // no countdown on registered only
		{
			//	On last level, we don't want a countdown.
			if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel))
			{
				if (!(gameData.app.nGameMode & GM_MULTI))
					return;
				if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
					return;
			}
		}

		GrSetCurFont( SMALL_FONT );
		GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
		y = SMALL_FONT->ft_h*4;
		if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
			y += SMALL_FONT->ft_h*2;

		if (gameStates.app.bPlayerIsDead)
			y += SMALL_FONT->ft_h*2;

		//if (!((gameStates.render.cockpit.nMode == CM_STATUS_BAR) && (Game_window_y >= 19)))
		//	y += 5;
		GrPrintF(0x8000, y, "T-%d s", gameData.reactor.countdown.nSecsLeft );
	}
}

//------------------------------------------------------------------------------

void game_draw_hud_stuff()
{
#ifndef NDEBUG
	if (Debug_pause) {
		GrSetCurFont( MEDIUM1_FONT );
		GrSetFontColorRGBi (GRAY_RGBA, 1, 0, 0);
		GrUString( 0x8000, 85/2, "Debug Pause - Press P to exit" );
	}
	draw_window_label();
#endif

	game_draw_multi_message();
   game_draw_marker_message();

//   if (gameData.app.nGameMode & GM_MULTI)
//    {
//     if (netGame.PlayTimeAllowed)
//       GameDrawTimeLeft ();
//  }

	if ((gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.demo.nState == ND_STATE_RECORDING)) {
		char message[128];
		int h,w,aw;

		if (gameData.demo.nState == ND_STATE_PLAYBACK) {
			if (gameData.demo.nVcrState != ND_STATE_PRINTSCREEN) {
			  	sprintf(message, "%s (%d%%%% %s)", TXT_DEMO_PLAYBACK, NDGetPercentDone(), TXT_DONE);
			} else {
				sprintf (message, " ");
			}
		} else 
			sprintf (message, TXT_DEMO_RECORDING);

		GrSetCurFont( GAME_FONT );    //GAME_FONT );
		GrSetFontColorRGBi (RGBA_PAL2 (27, 0, 0), 1, 0, 0);

		GrGetStringSize(message, &w, &h, &aw );
		if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
			if (grdCurCanv->cv_bitmap.bm_props.h > 240)
				h += 40;
			else
				h += 15;
		} else if ( gameStates.render.cockpit.nMode == CM_LETTERBOX )
			h += 7;
		if (gameStates.render.cockpit.nMode != CM_REAR_VIEW && !Saving_movie_frames)
			GrPrintF((grdCurCanv->cv_bitmap.bm_props.w-w)/2, grdCurCanv->cv_bitmap.bm_props.h - h - 2, message );
	}

	if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) {
		render_countdown_gauge();
		if ( gameData.multi.nLocalPlayer > -1 && gameData.objs.viewer->nType==OBJ_PLAYER && gameData.objs.viewer->id==gameData.multi.nLocalPlayer )	{
			int	x = 3;
			int	y = grdCurCanv->cv_bitmap.bm_props.h;

			GrSetCurFont( GAME_FONT );
			GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
			if (gameStates.input.nCruiseSpeed > 0) {
				int line_spacing = GAME_FONT->ft_h + GAME_FONT->ft_h/4;

				if (gameStates.render.cockpit.nMode==CM_FULL_SCREEN) {
					if (gameData.app.nGameMode & GM_MULTI)
						y -= line_spacing * 11;	//64
					else
						y -= line_spacing * 6;	//32
				} else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
					if (gameData.app.nGameMode & GM_MULTI)
						y -= line_spacing * 8;	//48
					else
						y -= line_spacing * 4;	//24
				} else {
					y = line_spacing * 2;	//12
					x = 20+2;
				}
				GrPrintF( x, y, "%s %2d%%", TXT_CRUISE, f2i(gameStates.input.nCruiseSpeed) );
			}
		}
	}

	if (gameStates.render.frameRate.value)
		show_framerate();

	if ( (gameData.demo.nState == ND_STATE_PLAYBACK) )
		gameData.app.nGameMode = gameData.demo.nGameMode;

	if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) 
		DrawHUD();

	if ( (gameData.demo.nState == ND_STATE_PLAYBACK) )
		gameData.app.nGameMode = GM_NORMAL;

	if ( gameStates.app.bPlayerIsDead )
		PlayerDeadMessage();
}

extern int gr_bitblt_dest_step_shift;
extern int gr_wait_for_retrace;
extern int gr_bitblt_double;

#if 0
void expand_row(ubyte * dest, ubyte * src, int num_src_pixels );
#pragma aux expand_row parm [edi] [esi] [ecx] modify exact [ecx esi edi eax ebx] = \
	"add	esi, ecx"			\
	"dec	esi"					\
	"add	edi, ecx"			\
	"add	edi, ecx"			\
	"dec	edi"					\
	"dec	edi"					\
"nextpixel:"					\
	"mov	al,[esi]"			\
	"mov	ah, al"				\
	"dec	esi"					\
	"mov	[edi], ax"			\
	"dec	edi"					\
	"dec	edi"					\
	"dec	ecx"					\
	"jnz	nextpixel"			\
"done:"
#else
void expand_row(ubyte * dest, ubyte * src, int num_src_pixels )
{
	int i;
	
	for (i = 0; i < num_src_pixels; i++) {
		*dest++ = *src;
		*dest++ = *src++;
	}
}
#endif

//------------------------------------------------------------------------------
// doubles the size in x or y of a bitmap in place.
void game_expand_bitmap( grsBitmap * bmp, uint flags )
{
	int i;
	ubyte * dptr, * sptr;

	switch(flags & 3)	{
	case 2:	// expand x
		Assert( bmp->bm_props.rowsize == bmp->bm_props.w*2 );
		dptr = &bmp->bm_texBuf[(bmp->bm_props.h-1)*bmp->bm_props.rowsize];
		for (i=bmp->bm_props.h-1; i>=0; i-- )	{
			expand_row( dptr, dptr, bmp->bm_props.w );	
			dptr -= bmp->bm_props.rowsize;
		}
		bmp->bm_props.w *= 2;
		break;
	case 1:	// expand y
		dptr = &bmp->bm_texBuf[(2*(bmp->bm_props.h-1)+1)*bmp->bm_props.rowsize];
		sptr = &bmp->bm_texBuf[(bmp->bm_props.h-1)*bmp->bm_props.rowsize];
		for (i=bmp->bm_props.h-1; i>=0; i-- )	{
			memcpy( dptr, sptr, bmp->bm_props.w );	
			dptr -= bmp->bm_props.rowsize;
			memcpy( dptr, sptr, bmp->bm_props.w );	
			dptr -= bmp->bm_props.rowsize;
			sptr -= bmp->bm_props.rowsize;
		}
		bmp->bm_props.h *= 2;
		break;
	case 3:	// expand x & y
		Assert( bmp->bm_props.rowsize == bmp->bm_props.w*2 );
		dptr = &bmp->bm_texBuf[(2*(bmp->bm_props.h-1)+1)*bmp->bm_props.rowsize];
		sptr = &bmp->bm_texBuf[(bmp->bm_props.h-1)*bmp->bm_props.rowsize];
		for (i=bmp->bm_props.h-1; i>=0; i-- )	{
			expand_row( dptr, sptr, bmp->bm_props.w );	
			dptr -= bmp->bm_props.rowsize;
			expand_row( dptr, sptr, bmp->bm_props.w );	
			dptr -= bmp->bm_props.rowsize;
			sptr -= bmp->bm_props.rowsize;
		}
		bmp->bm_props.w *= 2;
		bmp->bm_props.h *= 2;
		break;
	}
}

//------------------------------------------------------------------------------

extern int SW_drawn[2], SW_x[2], SW_y[2], SW_w[2], SW_h[2];

#if 0
//render a frame for the game in stereo
void game_render_frame_stereo()
{
	int dw,dh,sw,sh;
	fix save_aspect;
	fix actual_eye_width;
	int actual_eye_offset;
	grs_canvas RenderCanvas[2];
	int no_draw_hud=0;

	save_aspect = grdCurScreen->sc_aspect;
	grdCurScreen->sc_aspect *= 2;	//Muck with aspect ratio

	sw = dw = VR_render_buffer[0].cv_bitmap.bm_props.w;
	sh = dh = VR_render_buffer[0].cv_bitmap.bm_props.h;

	if (VR_low_res & 1)	{
		sh /= 2;				
		grdCurScreen->sc_aspect *= 2;  //Muck with aspect ratio	                        
	}
	if (VR_low_res & 2)	{
		sw /= 2;				
		grdCurScreen->sc_aspect /= 2;  //Muck with aspect ratio	                        
	}

	GrInitSubCanvas( RenderCanvas[0, VR_render_buffer, 0, 0, sw, sh );
	GrInitSubCanvas( RenderCanvas + 1, VR_render_buffer + 1, 0, 0, sw, sh );

	// Draw the left eye's view
	if (VR_eye_switch)	{
		actual_eye_width = -VR_eye_width;
		actual_eye_offset = -VR_eye_offset;
	} else {
		actual_eye_width = VR_eye_width;
		actual_eye_offset = VR_eye_offset;
	}

	if (gameData.objs.guidedMissile[gameData.multi.nLocalPlayer] && 
		 gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nType==OBJ_WEAPON && 
		 gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->id==GUIDEDMSL_ID && 
		 gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nSignature==gameData.objs.guidedMissileSig[gameData.multi.nLocalPlayer] && 
		 gameOpts->render.cockpit.bGuidedInMainView)
		actual_eye_offset = 0;

	GrSetCurrentCanvas(&RenderCanvas[0]);

	if (gameData.objs.guidedMissile[gameData.multi.nLocalPlayer] && gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nType==OBJ_WEAPON && gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->id==GUIDEDMSL_ID && gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nSignature==gameData.objs.guidedMissileSig[gameData.multi.nLocalPlayer] && gameOpts->render.cockpit.bGuidedInMainView) {
		char *msg = "Guided Missile View";
		tObject *viewer_save = gameData.objs.viewer;
		int w,h,aw;

		gameData.objs.viewer = gameData.objs.guidedMissile[gameData.multi.nLocalPlayer];

		WIN(DDGRLOCK(dd_grd_curcanv); 	// Must lock DD canvas!!!
  		{
			UpdateRenderedData(0, gameData.objs.viewer, 0, 0);
			RenderFrame(0, 0);
			WakeupRenderedObjects(gameData.objs.viewer, 0);
			gameData.objs.viewer = viewer_save;

			GrSetCurFont( GAME_FONT );    //GAME_FONT );
			GrSetFontColorRGBi (MEDRED_RGBA, 1, 0, 0);
			GrGetStringSize(msg, &w, &h, &aw );

			GrPrintF((grdCurCanv->cv_bitmap.bm_props.w-w)/2, 3, msg );

			glDisable(GL_DEPTH_TEST);
			DrawGuidedCrosshair();
			glEnable(GL_DEPTH_TEST);
		}
		WIN(DDGRUNLOCK(dd_grd_curcanv);

			HUDRenderMessageFrame();

		no_draw_hud=1;
	}
	else if (gameStates.render.bRearView)
		RenderFrame(actual_eye_width, 0);	// switch eye positions for rear view
	else
		RenderFrame(-actual_eye_width, 0);		// Left eye

	if ( VR_low_res )
		game_expand_bitmap( &RenderCanvas[0].cv_bitmap, VR_low_res );

	{	//render small window into left eye's canvas
		grs_canvas *save=grdCurCanv;
		fix save_aspect2 = grdCurScreen->sc_aspect;
		grdCurScreen->sc_aspect = save_aspect*2;
		SW_drawn[0] = SW_drawn[1] = 0;
		show_extra_views();
		GrSetCurrentCanvas(save);
		grdCurScreen->sc_aspect = save_aspect2;
	}

//NEWVR
	if (actual_eye_offset > 0 ) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect( grdCurCanv->cv_bitmap.bm_props.w-labs(actual_eye_offset)*2, 0, 
               grdCurCanv->cv_bitmap.bm_props.w-1, grdCurCanv->cv_bitmap.bm_props.h );
	} else if (actual_eye_offset < 0 ) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect( 0, 0, labs(actual_eye_offset)*2-1, grdCurCanv->cv_bitmap.bm_props.h );
	}

	if ( VR_show_hud && !no_draw_hud )	{
		grs_canvas tmp;
		if (actual_eye_offset < 0 ) {
			GrInitSubCanvas( &tmp, grdCurCanv, labs(actual_eye_offset*2), 0, grdCurCanv->cv_bitmap.bm_props.w-(labs(actual_eye_offset)*2), grdCurCanv->cv_bitmap.bm_props.h );
		} else {
			GrInitSubCanvas( &tmp, grdCurCanv, 0, 0, grdCurCanv->cv_bitmap.bm_props.w-(labs(actual_eye_offset)*2), grdCurCanv->cv_bitmap.bm_props.h );
		}
		GrSetCurrentCanvas( &tmp );
		game_draw_hud_stuff();
	}


	// Draw the right eye's view
	GrSetCurrentCanvas(&RenderCanvas[1]);

	if (gameData.objs.guidedMissile[gameData.multi.nLocalPlayer] && 
		 gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nType==OBJ_WEAPON && 
		 gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->id==GUIDEDMSL_ID && 
		 gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nSignature==gameData.objs.guidedMissileSig[gameData.multi.nLocalPlayer] && 
		 gameOpts->render.cockpit.bGuidedInMainView)
		GrBitmap(0,0,&RenderCanvas[0].cv_bitmap);
	else {
		if (gameStates.render.bRearView)
			RenderFrame(-actual_eye_width, 0);	// switch eye positions for rear view
		else
			RenderFrame(actual_eye_width, 0);		// Right eye

		if ( VR_low_res )
			game_expand_bitmap( &RenderCanvas[1].cv_bitmap, VR_low_res );
	}


	{	//copy small window from left eye
		grs_canvas temp;
		int w;
		for (w=0;w<2;w++) {
			if (SW_drawn[w]) {
				GrInitSubCanvas(&temp,&RenderCanvas[0],SW_x[w],SW_y[w],SW_w[w],SW_h[w]);
				GrBitmap(SW_x[w]+actual_eye_offset*2,SW_y[w],&temp.cv_bitmap);
			}
		}
	}

//NEWVR
	if (actual_eye_offset>0) {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect( 0, 0, labs(actual_eye_offset)*2-1, grdCurCanv->cv_bitmap.bm_props.h );
	} else if ( actual_eye_offset < 0 )	{
		GrSetColorRGB (0, 0, 0, 255);
		GrRect( grdCurCanv->cv_bitmap.bm_props.w-labs(actual_eye_offset)*2, 0, 
               grdCurCanv->cv_bitmap.bm_props.w-1, grdCurCanv->cv_bitmap.bm_props.h );
	}

//NEWVR (Add the next 2 lines)
	if ( VR_show_hud && !no_draw_hud )	{
		grs_canvas tmp;
		if (actual_eye_offset > 0 ) {
			GrInitSubCanvas( &tmp, grdCurCanv, labs(actual_eye_offset*2), 0, grdCurCanv->cv_bitmap.bm_props.w-(labs(actual_eye_offset)*2), grdCurCanv->cv_bitmap.bm_props.h );
		} else {
			GrInitSubCanvas( &tmp, grdCurCanv, 0, 0, grdCurCanv->cv_bitmap.bm_props.w-(labs(actual_eye_offset)*2), grdCurCanv->cv_bitmap.bm_props.h );
		}
		GrSetCurrentCanvas( &tmp );
		game_draw_hud_stuff();
	}


	// Draws white and black registration encoding lines
	// and Accounts for pixel-shift adjustment in upcoming bitblts
	if (VR_use_reg_code)	{
		int width, height, quarter;

		width = RenderCanvas[0].cv_bitmap.bm_props.w;
		height = RenderCanvas[0].cv_bitmap.bm_props.h;
		quarter = width / 4;

		// black out left-hand tSide of left page

		// draw registration code for left eye
		if ( VR_eye_switch )
			GrSetCurrentCanvas( &RenderCanvas[1] );
		else
			GrSetCurrentCanvas( &RenderCanvas[0] );
		GrSetColorRGB (255, 255, 255, 255);
		gr_scanline( 0, quarter, height-1 );
		GrSetColorRGB (0, 0, 0, 255);
		gr_scanline( quarter, width-1, height-1 );

		if ( VR_eye_switch )
			GrSetCurrentCanvas( &RenderCanvas[0] );
		else
			GrSetCurrentCanvas( &RenderCanvas[1] );
		GrSetColorRGB (255, 255, 255, 255);
		gr_scanline( 0, quarter*3, height-1 );
		GrSetColorRGB (0, 0, 0, 255);
		gr_scanline( quarter*3, width-1, height-1 );
   }

 		// Copy left eye, then right eye
	if ( VR_screenFlags&VRF_USE_PAGING )
		nVRCurrentPage = !nVRCurrentPage;
	else 
		nVRCurrentPage = 0;
	GrSetCurrentCanvas( &VR_screen_pages[nVRCurrentPage] );

//NEWVR

	if ( VR_eye_offset_changed > 0 )	{
		VR_eye_offset_changed--;
		GrClearCanvas(0);
	}

	sw = dw = VR_render_buffer[0].cv_bitmap.bm_props.w;
	sh = dh = VR_render_buffer[0].cv_bitmap.bm_props.h;

	// Copy left eye, then right eye
	gr_bitblt_dest_step_shift = 1;		// Skip every other scanline.

	if (VR_render_mode == VR_INTERLACED ) 	{
		if ( actual_eye_offset > 0 )	{
			int xoff = labs(actual_eye_offset);
			GrBmUBitBlt( dw-xoff, dh, xoff, 0, 0, 0, &RenderCanvas[0].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap);
			GrBmUBitBlt( dw-xoff, dh, 0, 1, xoff, 0, &RenderCanvas[1].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap);
		} else if ( actual_eye_offset < 0 )	{
			int xoff = labs(actual_eye_offset);
			GrBmUBitBlt( dw-xoff, dh, 0, 0, xoff, 0, &RenderCanvas[0].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap);
			GrBmUBitBlt( dw-xoff, dh, xoff, 1, 0, 0, &RenderCanvas[1].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap);
		} else {
			GrBmUBitBlt( dw, dh, 0, 0, 0, 0, &RenderCanvas[0].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap);
			GrBmUBitBlt( dw, dh, 0, 1, 0, 0, &RenderCanvas[1].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap);
		}
	} else if (VR_render_mode == VR_AREA_DET) {
		// VFX copy
		GrBmUBitBlt( dw, dh, 0,  nVRCurrentPage, 0, 0, &RenderCanvas[0].cv_bitmap, &VR_screen_pages[0].cv_bitmap);
		GrBmUBitBlt( dw, dh, dw, nVRCurrentPage, 0, 0, &RenderCanvas[1].cv_bitmap, &VR_screen_pages[0].cv_bitmap);
	} else {
		Int3();		// Huh?
	}

	gr_bitblt_dest_step_shift = 0;

	//if ( Game_vfxFlag )
	//	vfx_set_page(nVRCurrentPage);		// 0 or 1
	//else 
		if ( VR_screenFlags&VRF_USE_PAGING )	{
			gr_wait_for_retrace = 0;

//	Added by Samir from John's code
		if ( (VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.nType == BM_MODEX) && (Game_3dmaxFlag==3) )	{
			int old_x, old_y, new_x;
			old_x = VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.x;
			old_y = VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.y;
			new_x = old_y*VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.rowsize;
			new_x += old_x/4;
			VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.x = new_x;
			VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.y = 0;
			VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.nType = BM_SVGA;
			GrShowCanvas( &VR_screen_pages[nVRCurrentPage] );
			VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.nType = BM_MODEX;
			VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.x = old_x;
			VR_screen_pages[nVRCurrentPage].cv_bitmap.bm_props.y = old_y;
		} else {
			GrShowCanvas( &VR_screen_pages[nVRCurrentPage] );
		}
		gr_wait_for_retrace = 1;
	}
	grdCurScreen->sc_aspect=save_aspect;
}
#endif
ubyte DemoDoingRight=0,DemoDoingLeft=0;
extern ubyte DemoDoRight,DemoDoLeft;
extern tObject DemoRightExtra,DemoLeftExtra;

char DemoWBUType[]={0,WBUMSL,WBUMSL,WBU_REAR,WBU_ESCORT,WBU_MARKER,WBUMSL};
char DemoRearCheck[]={0,0,0,1,0,0,0};
char *DemoExtraMessage[]={"PLAYER","GUIDED","MISSILE","REAR","GUIDE-BOT","MARKER","SHIP"};

void show_extra_views()
{
	int did_missile_view=0;
	int save_newdemo_state = gameData.demo.nState;
	int w;

   if (gameData.demo.nState==ND_STATE_PLAYBACK)
    {
     if (DemoDoLeft)
      { 
		 DemoDoingLeft=DemoDoLeft;
		
       if (DemoDoLeft==3)
		 	DoCockpitWindowView(0,gameData.objs.console,1,WBU_REAR,"REAR");
       else
	      DoCockpitWindowView(0,&DemoLeftExtra,DemoRearCheck[DemoDoLeft],DemoWBUType[DemoDoLeft],DemoExtraMessage[DemoDoLeft]);
		}
     else
		DoCockpitWindowView(0,NULL,0,WBU_WEAPON,NULL);

	  if (DemoDoRight)
		{
		 DemoDoingRight=DemoDoRight;
		
       if (DemoDoRight==3)
		 	DoCockpitWindowView(1,gameData.objs.console,1,WBU_REAR,"REAR");
       else
   	   DoCockpitWindowView(1,&DemoRightExtra,DemoRearCheck[DemoDoRight],DemoWBUType[DemoDoRight],DemoExtraMessage[DemoDoRight]);
		} 
     else
    	DoCockpitWindowView(1,NULL,0,WBU_WEAPON,NULL);
	  
      DemoDoLeft=DemoDoRight=0;
		DemoDoingLeft=DemoDoingRight=0;
    
   	return;
    } 

	if (gameData.objs.guidedMissile[gameData.multi.nLocalPlayer] && gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nType==OBJ_WEAPON && gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->id==GUIDEDMSL_ID && gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]->nSignature==gameData.objs.guidedMissileSig[gameData.multi.nLocalPlayer]) {
		if (gameOpts->render.cockpit.bGuidedInMainView)
		 {
			gameStates.render.nRenderingType=6+(1<<4);
			DoCockpitWindowView(1,gameData.objs.viewer,0,WBUMSL,"SHIP");
		 }
		else
		 {
			gameStates.render.nRenderingType=1+(1<<4);
			DoCockpitWindowView(1,gameData.objs.guidedMissile[gameData.multi.nLocalPlayer],0,WBU_GUIDED,"GUIDED");
	    }
			
		did_missile_view=1;
	}
	else {

		if (gameData.objs.guidedMissile[gameData.multi.nLocalPlayer]) {		//used to be active
			if (!gameOpts->render.cockpit.bGuidedInMainView)
				DoCockpitWindowView(1,NULL,0,WBU_STATIC,NULL);
			gameData.objs.guidedMissile[gameData.multi.nLocalPlayer] = NULL;
		}

		if (gameData.objs.missileViewer && !gameStates.render.bExternalView) {		//do missile view
			static int mv_sig=-1;
			if (mv_sig == -1)
				mv_sig = gameData.objs.missileViewer->nSignature;
			if (/*!gameStates.app.bD1Mission && */ //allow in D1 levels
				 gameOpts->render.cockpit.bMissileView && 
				 (gameData.objs.missileViewer->nType!=OBJ_NONE) && 
				 (gameData.objs.missileViewer->nSignature == mv_sig)) {
  				gameStates.render.nRenderingType=2+(1<<4);
				DoCockpitWindowView(1,gameData.objs.missileViewer,0,WBUMSL,"MISSILE");
				did_missile_view=1;
			}
			else {
				gameData.objs.missileViewer = NULL;
				mv_sig = -1;
				gameStates.render.nRenderingType=255;
				DoCockpitWindowView(1,NULL,0,WBU_STATIC,NULL);
			}
		}
	}

	for (w=0;w<2;w++) {

		if (w==1 && did_missile_view)
			continue;		//if showing missile view in right window, can't show anything else

		//show special views if selected
		switch (Cockpit_3d_view[w]) {
			case CV_NONE:
				gameStates.render.nRenderingType=255;
				DoCockpitWindowView(w,NULL,0,WBU_WEAPON,NULL);
				break;
			case CV_REAR:
				if (gameStates.render.bRearView) {		//if big window is rear view, show front here
					gameStates.render.nRenderingType=3+(w<<4);				
					DoCockpitWindowView(w,gameData.objs.console,0,WBU_REAR,"FRONT");
				}
				else {					//show normal rear view
					gameStates.render.nRenderingType=3+(w<<4);				
					DoCockpitWindowView(w,gameData.objs.console,1,WBU_REAR,"REAR");
				}
			 	break;
			case CV_ESCORT: {
				tObject *buddy;
				buddy = find_escort();
				if (buddy == NULL) {
					DoCockpitWindowView(w,NULL,0,WBU_WEAPON,NULL);
					Cockpit_3d_view[w] = CV_NONE;
				}
				else {
					gameStates.render.nRenderingType=4+(w<<4);
					DoCockpitWindowView(w,buddy,0,WBU_ESCORT,gameData.escort.szName);
				}
				break;
			}
			case CV_COOP: {
				int tPlayer = Coop_view_player[w];

	         gameStates.render.nRenderingType=255; // don't handle coop stuff			
				
				if (tPlayer!=-1 && gameData.multi.players[tPlayer].connected && ((gameData.app.nGameMode & GM_MULTI_COOP) || ((gameData.app.nGameMode & GM_TEAM) && (GetTeam(tPlayer) == GetTeam(gameData.multi.nLocalPlayer)))))
					DoCockpitWindowView(w,&gameData.objs.objects[gameData.multi.players[Coop_view_player[w]].nObject],0,WBU_COOP,gameData.multi.players[Coop_view_player[w]].callsign);
				else {
					DoCockpitWindowView(w,NULL,0,WBU_WEAPON,NULL);
					Cockpit_3d_view[w] = CV_NONE;
				}
				break;
			}

			case CV_MARKER: {
				char label[10];
				short v = gameData.marker.viewers[w];
				gameStates.render.nRenderingType=5+(w<<4);
				if ((v == -1) || (gameData.marker.objects [v] == -1)) {
					Cockpit_3d_view[w] = CV_NONE;
					break;
				}
				sprintf(label,"Marker %d",gameData.marker.viewers[w]+1);
				DoCockpitWindowView(w,gameData.objs.objects+gameData.marker.objects[gameData.marker.viewers[w]],0,WBU_MARKER,label);
				break;
			}
			case CV_RADAR_TOPDOWN:
			case CV_RADAR_HEADSUP:
				if (EGI_FLAG (bRadarEnabled, 1, 0))
					DoCockpitWindowView(w,gameData.objs.console,0, 
					(Cockpit_3d_view[w] == CV_RADAR_TOPDOWN) ? WBU_RADAR_TOPDOWN : WBU_RADAR_HEADSUP, "RADAR");
				else
					Cockpit_3d_view[w] = CV_NONE;
				break;
			default:
				Int3();		//invalid window nType
		}
	}
	gameStates.render.nRenderingType=0;
	gameData.demo.nState = save_newdemo_state;
}

//------------------------------------------------------------------------------

extern ubyte * Game_cockpit_copy_code;

void DrawGuidedCrosshair(void);

void GameRenderFrameMono(void)
{
	tObject *gm;

	WINDOS (
		dd_grs_canvas Screen_3d_window,
		grs_canvas Screen_3d_window
	);
	int no_draw_hud=0;

	WINDOS (
		DDGrInitSubCanvas(&Screen_3d_window, &dd_VR_screen_pages[0],
			dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.x, 
			dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.y, 
			dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.w, 
			dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.h),
		GrInitSubCanvas( &Screen_3d_window, &VR_screen_pages[0], 
			VR_render_sub_buffer[0].cv_bitmap.bm_props.x, 
			VR_render_sub_buffer[0].cv_bitmap.bm_props.y, 
			VR_render_sub_buffer[0].cv_bitmap.bm_props.w, 
			VR_render_sub_buffer[0].cv_bitmap.bm_props.h)
	);

	if ( Game_double_buffer ) {	
		WINDOS(
			DDGrSetCurrentCanvas(&dd_VR_render_sub_buffer[0]),
			GrSetCurrentCanvas(&VR_render_sub_buffer[0])
		);
	}
	else	{
		WINDOS(
			DDGrSetCurrentCanvas(&Screen_3d_window),
			GrSetCurrentCanvas(&Screen_3d_window)
		);
	}
	
	gm = gameData.objs.guidedMissile [gameData.multi.nLocalPlayer];
	if (gm && 
		 gm->nType==OBJ_WEAPON && 
		 gm->id==GUIDEDMSL_ID && 
		 gm->nSignature==gameData.objs.guidedMissileSig[gameData.multi.nLocalPlayer] && 
		 gameOpts->render.cockpit.bGuidedInMainView) {
		char *msg = "Guided Missile View";
		tObject *viewer_save = gameData.objs.viewer;
		int w,h,aw;

      if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT)
			{
			 gameStates.render.cockpit.bBigWindowSwitch=1;
			 gameStates.render.cockpit.bRedraw=1;
			 gameStates.render.cockpit.nMode=CM_STATUS_BAR;
			 return;
		   }
  
		gameData.objs.viewer = gameData.objs.guidedMissile[gameData.multi.nLocalPlayer];

		WIN(DDGRLOCK(dd_grd_curcanv)); 	// Must lock DD canvas!!!
  		{
			UpdateRenderedData(0, gameData.objs.viewer, 0, 0);
			if (RenderCameras ()) {
				if ( Game_double_buffer ) {
					WINDOS(
						DDGrSetCurrentCanvas(&dd_VR_render_sub_buffer[0]),
						GrSetCurrentCanvas(&VR_render_sub_buffer[0])
						);
				}
				else	{
					WINDOS(
						DDGrSetCurrentCanvas(&Screen_3d_window),
						GrSetCurrentCanvas(&Screen_3d_window)
						);
					}
				}
			RenderFrame(0, 0);
  			WakeupRenderedObjects(gameData.objs.viewer, 0);
			gameData.objs.viewer = viewer_save;
			GrSetCurFont( GAME_FONT );    //GAME_FONT );
			GrSetFontColorRGBi (MEDRED_RGBA, 1, 0, 0);
			GrGetStringSize(msg, &w, &h, &aw );

			GrPrintF((grdCurCanv->cv_bitmap.bm_props.w-w)/2, 3, msg );

			DrawGuidedCrosshair();
		}
		WIN(DDGRUNLOCK(dd_grd_curcanv));

			HUDRenderMessageFrame();

		no_draw_hud=1;
	}
	else
	 {	
		if (gameStates.render.cockpit.bBigWindowSwitch)
		 {
		   gameStates.render.cockpit.bRedraw=1;
			gameStates.render.cockpit.nMode=CM_FULL_COCKPIT;
		   gameStates.render.cockpit.bBigWindowSwitch=0;
			return;
		 }
		WIN(DDGRLOCK(dd_grd_curcanv)); 	// Must lock DD canvas!!!
		UpdateRenderedData (0, gameData.objs.viewer, gameStates.render.bRearView, 0);
		if (RenderCameras ()) {
			if ( Game_double_buffer ) {
				WINDOS(
					DDGrSetCurrentCanvas(&dd_VR_render_sub_buffer[0]),
					GrSetCurrentCanvas(&VR_render_sub_buffer[0])
				);
			}
			else	{
				WINDOS(
					DDGrSetCurrentCanvas(&Screen_3d_window),
					GrSetCurrentCanvas(&Screen_3d_window)
					);
				}
			}
		RenderFrame(0, 0);
		WIN(DDGRUNLOCK(dd_grd_curcanv));
	 }

	if ( Game_double_buffer ) {	
		WINDOS(
			DDGrSetCurrentCanvas(&dd_VR_render_sub_buffer[0]),
			GrSetCurrentCanvas(&VR_render_sub_buffer[0])
		);
	}
	else	{
		WINDOS(
			DDGrSetCurrentCanvas(&Screen_3d_window),
			GrSetCurrentCanvas(&Screen_3d_window)
		);
	}

	if (!no_draw_hud)	{
		WIN(DDGRLOCK(dd_grd_curcanv));
		game_draw_hud_stuff();
		WIN(DDGRUNLOCK(dd_grd_curcanv));
	}

	if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT)
		show_extra_views();		//missile view, buddy bot, etc.
	if ( Game_double_buffer ) {		//copy to visible screen
		if ( !Game_cockpit_copy_code )	{
			if ( VR_screenFlags&VRF_USE_PAGING )	{	
				nVRCurrentPage = !nVRCurrentPage;
				GrSetCurrentCanvas( &VR_screen_pages[nVRCurrentPage] );
				GrBmUBitBlt( VR_render_sub_buffer[0].cv_w, VR_render_sub_buffer[0].cv_h, VR_render_sub_buffer[0].cv_bitmap.bm_props.x, VR_render_sub_buffer[0].cv_bitmap.bm_props.y, 0, 0, &VR_render_sub_buffer[0].cv_bitmap, &VR_screen_pages[nVRCurrentPage].cv_bitmap );
				gr_wait_for_retrace = 0;
				GrShowCanvas( &VR_screen_pages[nVRCurrentPage] );
				gr_wait_for_retrace = 1;
			}
		} else	
			{
			gr_ibitblt( &VR_render_sub_buffer[0].cv_bitmap, &VR_screen_pages[0].cv_bitmap, Scanline_double );
		}
	}

	if (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR) {

		if ( (gameData.demo.nState == ND_STATE_PLAYBACK) )
			gameData.app.nGameMode = gameData.demo.nGameMode;

		glDepthFunc(GL_ALWAYS);
		if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
			update_cockpits (1);
			show_extra_views();		//missile view, buddy bot, etc.
			}
		else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
			update_cockpits (1);
			show_extra_views();		//missile view, buddy bot, etc.
			}
		RenderGauges();
		HUDShowIcons ();
		if ( (gameData.demo.nState == ND_STATE_PLAYBACK) )
			gameData.app.nGameMode = GM_NORMAL;
	}
else if (gameStates.render.cockpit.nMode == CM_REAR_VIEW)
	update_cockpits (1);
else
	HUDShowIcons ();

con_update();
OglSwapBuffers(0, 0);
if (bSaveScreenShot)
	SaveScreenShot (NULL, 0);
}

//------------------------------------------------------------------------------

void ToggleCockpit()
{
	int new_mode;

	switch (gameStates.render.cockpit.nMode) {
		case CM_FULL_COCKPIT: {
			new_mode = CM_STATUS_BAR;
#if 0
			{
			int max_h = grdCurScreen->sc_h - gameData.pig.tex.bitmaps[gameData.pig.tex.cockpitBmIndex[CM_STATUS_BAR+(gameStates.video.nDisplayMode?(gameData.models.nCockpits/2):0)].index].bm_props.h;
			if (Game_window_h > max_h)		//too big for statusbar
				new_mode = CM_FULL_SCREEN;
			else
				new_mode = CM_STATUS_BAR;
			}
#endif
			break;
		}

		case CM_STATUS_BAR:
			if (gameStates.render.bRearView)
				return;
			new_mode = (gameStates.render.cockpit.nNextMode < 0) ? CM_FULL_SCREEN : CM_FULL_COCKPIT;
			break;
			
		case CM_FULL_SCREEN:
			if (gameStates.render.bRearView)
				return;
			new_mode = CM_LETTERBOX;
			break;

		case CM_LETTERBOX:
			new_mode = CM_FULL_COCKPIT;
			break;

		case CM_REAR_VIEW:
      default:
			return;			//do nothing
			break;
	}

	gameStates.render.cockpit.nNextMode = -1;
	SelectCockpit(new_mode);
	HUDClearMessages();
	WritePlayerFile();
}

//------------------------------------------------------------------------------

#define WINDOW_W_DELTA	((max_window_w / 16)&~1)	//24	//20
#define WINDOW_H_DELTA	((max_window_h / 16)&~1)	//12	//10

#define WINDOW_MIN_W		((max_window_w * 10) / 22)	//160
#define WINDOW_MIN_H		((max_window_h * 10) / 22)

void grow_window()
{
	StopTime ();
	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		Game_window_h = max_window_h;
		Game_window_w = max_window_w;
		ToggleCockpit();
		HUDInitMessage(TXT_COCKPIT_F3);
		StartTime ();
		return;
	}

	if (gameStates.render.cockpit.nMode != CM_STATUS_BAR && (VR_screenFlags & VRF_ALLOW_COCKPIT)) {
		StartTime ();
		return;
		}

	if (Game_window_h>=max_window_h || Game_window_w>=max_window_w) {
		//Game_window_w = max_window_w;
		//Game_window_h = max_window_h;
		SelectCockpit(CM_FULL_SCREEN);
	} else {
		//int x,y;

		Game_window_w += WINDOW_W_DELTA;
		Game_window_h += WINDOW_H_DELTA;

		if (Game_window_h > max_window_h)
			Game_window_h = max_window_h;

		if (Game_window_w > max_window_w)
			Game_window_w = max_window_w;

		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;

		GameInitRenderSubBuffers( Game_window_x, Game_window_y, Game_window_w, Game_window_h );
	}

	HUDClearMessages();	//	@mk, 11/11/94

	WritePlayerFile();
	StartTime ();
}

//------------------------------------------------------------------------------
// grsBitmap bmBackground;	already declared in line 434 (samir 4/10/94)

extern grsBitmap bmBackground;

void copy_background_rect(int left,int top,int right,int bot)
{
	grsBitmap *bm = &bmBackground;
	int x,y;
	int tile_left,tile_right,tile_top,tile_bot;
	int ofs_x,ofs_y;
	int dest_x,dest_y;

	if (right < left || bot < top)
		return;

	tile_left = left / bm->bm_props.w;
	tile_right = right / bm->bm_props.w;
	tile_top = top / bm->bm_props.h;
	tile_bot = bot / bm->bm_props.h;

	ofs_y = top % bm->bm_props.h;
	dest_y = top;

WIN(DDGRLOCK(dd_grd_curcanv))
{
	for (y=tile_top;y<=tile_bot;y++) {
		int w,h;

		ofs_x = left % bm->bm_props.w;
		dest_x = left;

		//h = (bot < dest_y+bm->bm_props.h)?(bot-dest_y+1):(bm->bm_props.h-ofs_y);
		h = min(bot-dest_y+1,bm->bm_props.h-ofs_y);

		for (x=tile_left;x<=tile_right;x++) {

			//w = (right < dest_x+bm->bm_props.w)?(right-dest_x+1):(bm->bm_props.w-ofs_x);
			w = min(right-dest_x+1,bm->bm_props.w-ofs_x);
		
			GrBmUBitBlt(w,h,dest_x,dest_y,ofs_x,ofs_y,
					&bmBackground,&grdCurCanv->cv_bitmap);

			ofs_x = 0;
			dest_x += w;
		}

		ofs_y = 0;
		dest_y += h;
	}
} 
WIN(DDGRUNLOCK(dd_grd_curcanv));

}

//------------------------------------------------------------------------------
//fills int the background surrounding the 3d window
void fill_background()
{
	int x,y,w,h,dx,dy;

	x = Game_window_x;
	y = Game_window_y;
	w = Game_window_w;
	h = Game_window_h;

	dx = x;
	dy = y;

	WINDOS(	DDGrSetCurrentCanvas(&dd_VR_screen_pages[nVRCurrentPage]),
				GrSetCurrentCanvas(&VR_screen_pages[nVRCurrentPage])
	);
	copy_background_rect(x-dx,y-dy,x-1,y+h+dy-1);
	copy_background_rect(x+w,y-dy,grdCurCanv->cv_w-1,y+h+dy-1);
	copy_background_rect(x,y-dy,x+w-1,y-1);
	copy_background_rect(x,y+h,x+w-1,y+h+dy-1);

	if (VR_screenFlags & VRF_USE_PAGING) {
		WINDOS(	DDGrSetCurrentCanvas(&dd_VR_screen_pages[!nVRCurrentPage]),
					GrSetCurrentCanvas(&VR_screen_pages[!nVRCurrentPage])
		);
		copy_background_rect(x-dx,y-dy,x-1,y+h+dy-1);
		copy_background_rect(x+w,y-dy,x+w+dx-1,y+h+dy-1);
		copy_background_rect(x,y-dy,x+w-1,y-1);
		copy_background_rect(x,y+h,x+w-1,y+h+dy-1);
	}

}

//------------------------------------------------------------------------------

void shrink_window()
{
	StopTime ();
	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT && (VR_screenFlags & VRF_ALLOW_COCKPIT)) {
		Game_window_h = max_window_h;
		Game_window_w = max_window_w;
		//!!ToggleCockpit();
		gameStates.render.cockpit.nNextMode = CM_FULL_COCKPIT;
		SelectCockpit(CM_STATUS_BAR);
//		shrink_window();
//		shrink_window();
		HUDInitMessage(TXT_COCKPIT_F3);
		WritePlayerFile();
		StartTime ();
		return;
	}

	if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN && (VR_screenFlags & VRF_ALLOW_COCKPIT))
	{
		//Game_window_w = max_window_w;
		//Game_window_h = max_window_h;
		SelectCockpit(CM_STATUS_BAR);
		WritePlayerFile();
		StartTime ();
		return;
	}

	if (gameStates.render.cockpit.nMode != CM_STATUS_BAR && (VR_screenFlags & VRF_ALLOW_COCKPIT)) {
		StartTime ();
		return;
		}

#if TRACE
   con_printf (CON_DEBUG,"Cockpit mode=%d\n",gameStates.render.cockpit.nMode);
#endif
	if (Game_window_w > WINDOW_MIN_W) {
		//int x,y;

      Game_window_w -= WINDOW_W_DELTA;
		Game_window_h -= WINDOW_H_DELTA;

#if TRACE
  con_printf (CON_DEBUG,"NewW=%d NewH=%d VW=%d maxH=%d\n",Game_window_w,Game_window_h,max_window_w,max_window_h);
#endif                  
		if ( Game_window_w < WINDOW_MIN_W )
			Game_window_w = WINDOW_MIN_W;

		if ( Game_window_h < WINDOW_MIN_H )
			Game_window_h = WINDOW_MIN_H;
			
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;

		fill_background();

		GameInitRenderSubBuffers( Game_window_x, Game_window_y, Game_window_w, Game_window_h );
		HUDClearMessages();
		WritePlayerFile();
	}
StartTime ();
}

//------------------------------------------------------------------------------

static 
#ifdef RELEASE
inline 
#endif
void DrawCockpit (int h, int y)
{
if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN))
{
	int i = gameData.pig.tex.cockpitBmIndex [h].index;
	grsBitmap *bm = gameData.pig.tex.bitmaps [0] + i; 
	grs_color c;

	PIGGY_PAGE_IN (gameData.pig.tex.cockpitBmIndex [h], 0);
	OglLoadBmTexture (bm, 1, 0);
	WINDOS (DDGrSetCurrentCanvas(dd_VR_screen_pages+ nVRCurrentPage),
			  GrSetCurrentCanvas(VR_screen_pages + nVRCurrentPage));
	WIN (DDGRLOCK (dd_grd_curcanv));
	#if 0
		GrUBitmapM(0, y, bm);
	#else
		c.index = 255;
		c.rgb = 0;
		OglUBitMapMC (0, y, -1, grdCurCanv->cv_bitmap.bm_props.h - y, bm, &c, F1_0, 0);
	#endif
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	}
}

//------------------------------------------------------------------------------

int last_drawn_cockpit[2] = { -1, -1 };

// This actually renders the new cockpit onto the screen.
void update_cockpits(int force_redraw)
{
	//int x, y, w, h;

	if (gameStates.render.cockpit.nMode != last_drawn_cockpit[nVRCurrentPage] || force_redraw )
		last_drawn_cockpit[nVRCurrentPage] = gameStates.render.cockpit.nMode;
	else
		return;

	//Redraw the on-screen cockpit bitmaps
	if (VR_render_mode != VR_NONE )	
		return;

	switch( gameStates.render.cockpit.nMode )	{
	case CM_FULL_COCKPIT:
		DrawCockpit (gameStates.video.nDisplayMode ? gameData.models.nCockpits / 2 : 0, 0);
		if (force_redraw)
			return;
		break;

	case CM_REAR_VIEW:
		WINDOS(
			DDGrSetCurrentCanvas(&dd_VR_screen_pages[nVRCurrentPage]),
			GrSetCurrentCanvas(&VR_screen_pages[nVRCurrentPage])
		);
		DrawCockpit (gameStates.render.cockpit.nMode + (gameStates.video.nDisplayMode ? gameData.models.nCockpits / 2 : 0), 0);
		if (force_redraw)
			return;
		break;

	case CM_FULL_SCREEN:
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		fill_background();
		break;

	case CM_STATUS_BAR:
		DrawCockpit (gameStates.render.cockpit.nMode + (gameStates.video.nDisplayMode ? gameData.models.nCockpits / 2 : 0), max_window_h);

		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		fill_background();
		break;

	case CM_LETTERBOX:
		WINDOS(	DDGrSetCurrentCanvas(dd_VR_screen_pages + nVRCurrentPage),
					GrSetCurrentCanvas(VR_screen_pages + nVRCurrentPage)
		);
		WINDOS ( dd_gr_clear_canvas (BLACK_RGBA),
					GrClearCanvas (BLACK_RGBA));

		// In a modex mode, clear the other buffer.
		if (grdCurCanv->cv_bitmap.bm_props.nType == BM_MODEX) {
			GrSetCurrentCanvas(&VR_screen_pages[nVRCurrentPage^1]);
			GrClearCanvas(BLACK_RGBA);
			GrSetCurrentCanvas(&VR_screen_pages[nVRCurrentPage]);
		}
		break;

	}

	WINDOS (	DDGrSetCurrentCanvas(dd_VR_screen_pages + nVRCurrentPage),
				GrSetCurrentCanvas(VR_screen_pages + nVRCurrentPage)
	);
	if ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nMode == CM_STATUS_BAR))
		InitGauges();
}

//------------------------------------------------------------------------------

void GameRenderFrame()
{
	SetScreenMode( SCREEN_GAME );
//	update_cockpits(0);
	PlayHomingWarning();
	if (GrPaletteStepLoad (gamePalette)) {
		//GrCopyPalette (grPalette, gamePalette, sizeof (grPalette));
		//GrRemapColorFonts ();
		}
	if (VR_render_mode == VR_NONE )
		GameRenderFrameMono();	 
//	update_cockpits(0);
/*	else
		game_render_frame_stereo();	 
		*/

	// Make sure palette is faded in
	StopTime();
	GrPaletteFadeIn( NULL, 32, 0 );
	StartTime();

	gameData.app.nFrameCount++;
}

//------------------------------------------------------------------------------

//draw a crosshair for the guided missile
void DrawGuidedCrosshair(void)
{
	int x,y,w,h;

	GrSetColorRGBi (RGBA_PAL (0, 31, 0));

	w = grdCurCanv->cv_w>>5;
	if (w < 5)
		w = 5;

	h = i2f(w) / grdCurScreen->sc_aspect;

	x = grdCurCanv->cv_w / 2;
	y = grdCurCanv->cv_h / 2;

//	gr_scanline(x-w/2,x+w/2,y);
#if 1
	x = i2f (x);
	y = i2f (y);
	w = i2f (w / 2);
	h = i2f (h / 2);
	gr_uline (x - w, y, x + w, y);
	gr_uline (x, y - h, x, y + h);
#else
	gr_uline(i2f(x-w/2),i2f(y),i2f(x+w/2),i2f(y));
	gr_uline(i2f(x),i2f(y-h/2),i2f(x),i2f(y+h/2));
#endif
}

//------------------------------------------------------------------------------

bkg bg = {0,0,0,0,NULL,NULL,NULL,NULL,1,1};

#define BOX_BORDER (gameStates.menus.bHires?60:30)

//show a message in a nice little box
void ShowBoxedMessage(char *msg)
{	
	int w,h,aw;
	int x,y;
	//ubyte save_pal [256*3];

	//memcpy (save_pal, grPalette, sizeof (save_pal));
	WINDOS(
		DDGrSetCurrentCanvas(&dd_VR_screen_pages[nVRCurrentPage]),
		GrSetCurrentCanvas(&VR_screen_pages[nVRCurrentPage])
	);
	GrSetCurFont( MEDIUM1_FONT );
	GrGetStringSize(msg,&w,&h,&aw);
	x = (grdCurScreen->sc_w-w)/2;
	y = (grdCurScreen->sc_h-h)/2;
	if (bg.bmp) {
		GrFreeBitmap(bg.bmp);
		bg.bmp = NULL;
	}
	// Save the background of the display
	bg.x=x; bg.y=y; bg.w=w; bg.h=h;
	if (!gameOpts->menus.nStyle) {
		bg.bmp = GrCreateBitmap( w+BOX_BORDER, h+BOX_BORDER, 0 );
		WIN( DDGRLOCK(dd_grd_curcanv));
		GrBmUBitBlt(w+BOX_BORDER, h+BOX_BORDER, 0, 0, x-BOX_BORDER/2, y-BOX_BORDER/2, &(grdCurCanv->cv_bitmap), bg.bmp );
		WIN( DDGRUNLOCK(dd_grd_curcanv));
		}
	NMDrawBackground (&bg, x-BOX_BORDER/2,y-BOX_BORDER/2,x+w+BOX_BORDER/2-1,y+h+BOX_BORDER/2-1,0);
	GrSetFontColorRGBi (DKGRAY_RGBA, 1, 0, 0);
	WIN( DDGRLOCK(dd_grd_curcanv));
	GrUString( 0x8000, y, msg );
	WIN( DDGRUNLOCK(dd_grd_curcanv));
	WIN( DDGRRESTORE);
	GrUpdate (0);
	NMRemoveBackground (&bg);
}

//------------------------------------------------------------------------------

void ClearBoxedMessage()
{
if (bg.bmp) {
	WIN(DDGRLOCK(dd_grd_curcanv));
	GrBitmap(bg.x-BOX_BORDER/2, bg.y-BOX_BORDER/2, bg.bmp);
	WIN(DDGRUNLOCK(dd_grd_curcanv));
	WIN(DDGRRESTORE);
	if (bg.bmp) {
		GrFreeBitmap(bg.bmp);
		bg.bmp = NULL;
		}
	}
}

//------------------------------------------------------------------------------
//eof

