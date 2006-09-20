/* $Id: game.c,v 1.25 2003/12/08 22:32:56 btb Exp $ */
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
 * Game loop for Inferno
 *
 * Old Log:
 * Revision 1.1  1995/12/05  16:01:09  allender
 * Initial revision
 *
 * Revision 1.38  1995/11/13  13:02:35  allender
 * put up HUD message when player starts tournament
 *
 * Revision 1.37  1995/11/13  09:21:05  allender
 * ved and shorted tournament mode messages
 *
 * Revision 1.36  1995/11/09  17:27:00  allender
 * took out cheats during demo playback
 *
 * Revision 1.35  1995/11/07  17:05:41  allender
 * move registered cheats
 *
 * Revision 1.34  1995/11/03  12:55:45  allender
 * shareware changes
 *
 * Revision 1.33  1995/10/29  20:15:00  allender
 * took out frame rate cheat.  Pause for at least a second because
 * of cmd-P problem
 *
 * Revision 1.32  1995/10/26  14:11:26  allender
 * fix up message box stuff to align correctly
 *
 * Revision 1.31  1995/10/24  18:09:41  allender
 * ixed cockpit weirdness -- don't update cockpit when do_appl_quit
 * is called -- screen saved in mevent.c
 *
 * Revision 1.30  1995/10/21  23:39:10  allender
 * ruise marking indicator
 *
 * Revision 1.29  1995/10/21  22:52:27  allender
 * bald guy cheat -- print screen stuff
 *
 * Revision 1.28  1995/10/20  00:54:28  allender
 * new help menus and redbook checking in outer game loop
 *
 * Revision 1.27  1995/10/17  15:34:19  allender
 * pixel double is now default mode
 *
 * Revision 1.26  1995/10/12  17:34:44  allender
 * bigger message box -- command key equivs for function keys
 *
 * Revision 1.25  1995/10/11  12:17:14  allender
 * removed event loop processing
 *
 * Revision 1.24  1995/10/11  00:58:47  allender
 * removed debugging code
 *
 * Revision 1.23  1995/10/10  11:50:32  allender
 * fixed boxed message to align on 8 byte boundry,
 * and some debug code
 *
 * Revision 1.22  1995/09/24  10:51:26  allender
 * cannot go to finder in networkd:\temp\dm_testadded cmd-q for quit
 *
 * Revision 1.21  1995/09/22  15:05:18  allender
 * *more* hud and font type stuff (messages)
 *
 * Revision 1.20  1995/09/22  14:39:57  allender
 * ved framerate counter up
 *
 * Revision 1.19  1995/09/18  17:01:28  allender
 * start of compatibility stuff
 *
 * Revision 1.18  1995/09/15  15:53:13  allender
 * better handling of PICT screen shots
 *
 * Revision 1.17  1995/09/14  15:27:41  allender
 * fixed function type on message_box routiens
 *
 * Revision 1.16  1995/09/13  11:37:47  allender
 * put in call to dump PICT file instead of PCX
 *
 * Revision 1.15  1995/09/08  17:13:28  allender
 * put back in ibitblt.h and start of PICT picture dump
 *
 * Revision 1.14  1995/09/07  10:20:58  allender
 * make cockpit mode default
 *
 * Revision 1.13  1995/09/07  10:17:34  allender
 * added command key equivalents for function keys
 *
 * Revision 1.12  1995/09/04  11:36:47  allender
 * fixed pixel double mode to have correct number of rendered
 * lines
 *
 * Revision 1.11  1995/09/01  15:47:07  allender
 * cap frame rate at 60 fps
 *
 * Revision 1.10  1995/08/26  16:25:59  allender
 * whole buncha' stuff!!!!
 *
 * Revision 1.9  1995/08/01  16:04:47  allender
 * put in ctrl_esc sequence to go to menubar
 *
 * Revision 1.8  1995/07/28  14:15:11  allender
 * added FRAME cheat to display frame rate
 *
 * Revision 1.7  1995/07/17  08:54:19  allender
 * *** empty log message ***
 *
 * Revision 1.6  1995/07/12  12:54:06  allender
 * removed some debug keys
 *
 * Revision 1.5  1995/07/05  16:44:35  allender
 * changed some debug keys
 *
 * Revision 1.4  1995/06/23  10:24:57  allender
 * added scanline doubling routine
 *
 * Revision 1.3  1995/06/13  13:08:26  allender
 * added special debug key to move window into upper left corner.
 * also added debug key to put game in 640x480 mode
 *
 * Revision 1.2  1995/06/12  11:10:31  allender
 * added DEL_SHIFT_M to move window to corner of screen
 *
 * Revision 1.1  1995/05/16  15:25:08  allender
 * Initial revision
 *
 * Revision 2.36  1996/01/05  16:52:05  john
 * Improved 3d stuff.
 *
 * Revision 2.35  1995/10/09  22:17:10  john
 * Took out the page flipping in SetScreenMode, which shouldn't
 * be there.  This was hosing the modex stuff.
 *
 * Revision 2.34  1995/10/09  19:46:34  john
 * Fixed bug with modex paging with lcdbios.
 *
 * Revision 2.33  1995/10/08  11:46:09  john
 * Fixed bug with 2d offset in interlaced mode in low res.
 * Made LCDBIOS with pageflipping using VESA set start
 * Address function.  X=CRTC offset, Y=0.
 *
 * Revision 2.32  1995/10/07  13:20:51  john
 * Added new modes for LCDBIOS, also added support for -JoyNice,
 * and added Shift-F1-F4 to controls various stereoscopic params.
 *
 * Revision 2.31  1995/05/31  14:34:43  unknown
 * fixed warnings.
 *
 * Revision 2.30  1995/05/08  11:23:45  john
 * Made 3dmax work like Kasan wants it to.
 *
 * Revision 2.29  1995/04/06  13:47:39  yuan
 * Restored rear view to original.
 *
 * Revision 2.28  1995/04/06  12:13:07  john
 * Fixed some bugs with 3dmax.
 *
 * Revision 2.27  1995/04/05  13:18:18  mike
 * decrease energy usage on fusion cannon
 *
 * Revision 2.26  1995/03/30  16:36:32  mike
 * text localization.
 *
 * Revision 2.25  1995/03/27  16:45:26  john
 * Fixed some cheat bugs.  Added astral cheat.
 *
 * Revision 2.24  1995/03/27  15:37:11  mike
 * boost fusion cannon for non-multiplayer modes.
 *
 * Revision 2.23  1995/03/24  17:48:04  john
 * Fixed bug with menus and 320x100.
 *
 * Revision 2.22  1995/03/24  15:34:02  mike
 * cheats.
 *
 * Revision 2.21  1995/03/24  13:11:39  john
 * Added save game during briefing screens.
 *
 * Revision 2.20  1995/03/21  14:40:50  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.19  1995/03/16  22:07:16  john
 * Made so only for screen can be used for anything other
 * than mode 13.
 *
 * Revision 2.18  1995/03/16  21:45:35  john
 * Made all paged modes have incompatible menus!
 *
 * Revision 2.17  1995/03/16  18:30:35  john
 * Made wider than 320 screens not have
 * a status bar mode.
 *
 * Revision 2.16  1995/03/16  10:53:34  john
 * Move VFX center to Shift-Z instead of Enter because
 * it conflicted with toggling HUD on/off.
 *
 * Revision 2.15  1995/03/16  10:18:33  john
 * Fixed bug with VFX mode not working. also made warning
 * when it can't set VESA mode.
 *
 * Revision 2.14  1995/03/14  16:22:39  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.13  1995/03/14  12:14:17  john
 * Made VR helmets have 4 resolutions to choose from.
 *
 * Revision 2.12  1995/03/10  13:47:33  john
 * Added head tracking sensitivity.
 *
 * Revision 2.11  1995/03/10  13:13:47  john
 * Added code to show T-xx on iglasses.
 *
 * Revision 2.10  1995/03/09  18:07:29  john
 * Fixed bug with iglasses tracking not "centering" right.
 * Made VFX have bright headlight lighting.
 *
 * Revision 2.9  1995/03/09  11:48:02  john
 * Added HUD for VR helmets.
 *
 * Revision 2.8  1995/03/07  15:12:53  john
 * Fixed VFX,3dmax support.
 *
 * Revision 2.7  1995/03/07  11:35:03  john
 * Fixed bug with cockpit in rear view.
 *
 * Revision 2.6  1995/03/06  18:40:17  john
 * Added some ifdef EDITOR stuff.
 *
 * Revision 2.5  1995/03/06  18:31:21  john
 * Fixed bug with nmenu popping up on editor screen.
 *
 * Revision 2.4  1995/03/06  17:28:33  john
 * Fixed but with cockpit toggling wrong.
 *
 * Revision 2.3  1995/03/06  16:08:10  mike
 * Fix compile errors if building without editor.
 *
 * Revision 2.2  1995/03/06  15:24:10  john
 * New screen techniques.
 *
 * Revision 2.1  1995/02/27  13:41:03  john
 * Removed floating point from frame rate calculations.
 *
 * Revision 2.0  1995/02/27  11:31:54  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.770  1995/02/22  12:45:15  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.769  1995/02/15  10:06:25  allender
 * make pause pause game during demo playback
 *
 * Revision 1.768  1995/02/13  20:35:11  john
 * Lintized
 *
 * Revision 1.767  1995/02/13  19:40:29  allender
 * added place to demo record restoration from rear view in place that
 * I forgot before
 *
 * Revision 1.766  1995/02/13  10:29:27  john
 * Fixed bug with cheats not restoreing across save games.
 *
 * Revision 1.765  1995/02/11  22:54:33  john
 * Made loading for pig not show up for demos.
 *
 * Revision 1.764  1995/02/11  17:30:08  allender
 * ifndef NDEBUG around strip frame stuff
 *
 * Revision 1.763  1995/02/11  17:13:01  rob
 * Took out modem.c code fille stuff.
 *
 * Revision 1.762  1995/02/11  16:36:47  allender
 * debug key to strip frames from end of demo
 *
 * Revision 1.761  1995/02/11  14:29:16  john
 * Turned off cheats when going into game.
 *
 * Revision 1.760  1995/02/11  13:46:54  mike
 * fix cheats.
 *
 * Revision 1.759  1995/02/11  12:36:09  matt
 * Cleaned up cheats
 *
 * Revision 1.758  1995/02/11  12:27:04  mike
 * fix path-to-exit cheat.
 *
 * Revision 1.757  1995/02/11  01:56:24  mike
 * robots don't fire cheat.
 *
 * Revision 1.756  1995/02/10  16:38:40  mike
 * illuminate path to exit cheat.
 *
 * Revision 1.755  1995/02/10  16:19:40  mike
 * new show-path-to-exit system, still buggy, compiled out.
 *
 * Revision 1.754  1995/02/10  15:54:46  matt
 * Added new cheats
 *
 * Revision 1.753  1995/02/09  12:25:42  matt
 * Made mem_fill () test routines not be called if RELEASE
 *
 * Revision 1.752  1995/02/09  08:49:32  mike
 * change fill opcode value to 0xcc, int 3 value.
 *
 *
 * Revision 1.751  1995/02/09  02:59:26  mike
 * check code for 00066xxx bugs.
 *
 * Revision 1.750  1995/02/08  17:10:02  mike
 * add, but don't call, debug code.
 *
 * Revision 1.749  1995/02/07  11:07:27  john
 * Added hooks for confirm on game state restore.
 *
 * Revision 1.748  1995/02/06  15:52:45  mike
 * add mini megawow powerup for giving reasonable weapons.
 *
 * Revision 1.747  1995/02/06  12:53:35  allender
 * force endlevel_sequence to 0 to fix weird bug
 *
 * Revision 1.746  1995/02/04  10:03:30  mike
 * Fly to exit cheat.
 *
 * Revision 1.745  1995/02/02  15:57:52  john
 * Added turbo mode cheat.
 *
 * Revision 1.744  1995/02/02  14:43:39  john
 * Uppped frametime limit to 150 Hz.
 *
 * Revision 1.743  1995/02/02  13:37:16  mike
 * move T-?? message down in certain modes.
 *
 * Revision 1.742  1995/02/02  01:26:59  john
 * Took out no key repeating.
 *
 * Revision 1.741  1995/01/29  21:36:44  mike
 * make fusion cannon not make pitching slow.
 *
 * Revision 1.740  1995/01/28  15:57:57  john
 * Made joystick calibration be only when wrong detected in
 * menu or joystick axis changed.
 *
 * Revision 1.739  1995/01/28  15:21:03  yuan
 * Added X-tra life cheat.
 *
 * Revision 1.738  1995/01/27  14:08:31  rob
 * Fixed a bug.
 *
 * Revision 1.737  1995/01/27  14:04:59  rob
 * Its not my fault, Mark told me to do it!
 *
 * Revision 1.736  1995/01/27  13:12:18  rob
 * Added charging noises to play across net.
 *
 * Revision 1.735  1995/01/27  11:48:28  allender
 * check for newdemo_state to be paused and stop recording.  We might be
 * in between levels
 *
 * Revision 1.734  1995/01/26  22:11:41  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 *
 * Revision 1.733  1995/01/26  17:03:04  mike
 * make fusion cannon have more chrome, make fusion, mega rock you!
 *
 * Revision 1.732  1995/01/25  14:37:25  john
 * Made joystick only prompt for calibration onced:\temp\dm_test.
 *
 * Revision 1.731  1995/01/24  15:49:14  john
 * Made typeing in long net messages wrap on
 * small screen sizes.
 *
 * Revision 1.730  1995/01/24  15:23:42  mike
 * network message tweaking.
 *
 * Revision 1.729  1995/01/24  12:00:47  john
 * Fixed bug with defing macro passing keys to controls.
 *
 * Revision 1.728  1995/01/24  11:53:35  john
 * Added better macro defining code.
 *
 * Revision 1.727  1995/01/23  22:17:15  john
 * Fixed bug with not clearing key buffer when leaving f8.
 *
 * Revision 1.726  1995/01/23  22:07:09  john
 * Added flush to game inputs during F8.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
char game_rcsid[] = "$Id: game.c,v 1.25 2003/12/08 22:32:56 btb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#ifdef MACINTOSH
#include <Files.h>
#include <StandardFile.h>
#include <Quickdraw.h>
#include <Script.h>
#include <Strings.h>
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "cameras.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "digi.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newmenu.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "gamefont.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "gamepal.h"
#include "particles.h"
#include "lightmap.h"
#include "oof.h"
#include "sphere.h"

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "fix.h"
#include "hudmsg.h"
#include "tracker.h"
#include "particles.h"
#include "banlist.h"
#include "input.h"

u_int32_t VGA_current_mode;

void GamePaletteStepUp (int r, int g, int b);


#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

#define	SHOW_EXIT_PATH	1

#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>            //should come after inferno.h to get mark setting
#endif
#endif


void FreeHoardData (void);
int ReadControls (void);		// located in gamecntl.c
void DoFinalBossFrame (void);

int	Speedtest_on = 0;

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
int	Speedtest_start_time;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	Speedtest_count=0;				//	number of times to do the debug test.
#endif

static fix last_timer_value=0;
fix ThisLevelTime=0;

#if defined (TIMER_TEST) && !defined (NDEBUG)
fix _timer_value,actual_last_timer_value,_last_frametime;
int stop_count,start_count;
int time_stopped,time_started;
#endif

#ifndef MACINTOSH
ubyte * Game_cockpit_copy_code = NULL;
#else
ubyte Game_cockpit_copy_code = 0;
ubyte Scanline_double = 1;
#endif

u_int32_t	VR_screen_mode			= 0;
u_int32_t   bScreenModeOverride = 0;
ubyte			VR_screen_flags	= 0;		//see values in screens.h
ubyte			VR_current_page	= 0;
fix			VR_eye_width		= F1_0;
int			VR_render_mode		= VR_NONE;
int			VR_low_res 			= 3;				// Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity     = 1;		// 0 - 2

//NEWVR
int			VR_eye_offset		 = 0;
int			VR_eye_switch		 = 0;
int			VR_eye_offset_changed = 0;
int			VR_use_reg_code 	= 0;

grs_canvas  *VR_offscreen_buffer	= NULL;		// The offscreen data buffer
grs_canvas	VR_render_buffer[2];					//  Two offscreen buffers for left/right eyes.
grs_canvas	VR_render_sub_buffer[2];			//  Two sub buffers for left/right eyes.
grs_canvas	VR_screen_pages[2];					//  Two pages of VRAM if paging is available
grs_canvas	VR_editor_canvas;						//  The canvas that the editor writes to.

#ifdef WINDOWS
//@@ LPDIRECTDRAWSURFACE _lpDDSMask = NULL;
dd_grs_canvas *dd_VR_offscreen_buffer = NULL;
dd_grs_canvas dd_VR_screen_pages[2];
dd_grs_canvas dd_VR_render_buffer[2];
dd_grs_canvas dd_VR_render_sub_buffer[2];

void game_win_init_cockpit_mask (int sram);
#endif

//do menus work in 640x480 or 320x200?
//PC version sets this in main ().  Mac versios is always high-res, so set to 1 here
int Debug_pause=0;				//John's debugging pause system

//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.
int	Dummy_var;
int	*Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char faded_in;
#endif

#ifndef NDEBUG                          //these only exist if debugging

int Game_double_buffer = 1;     //double buffer by default
fix fixed_frametime=0;          //if non-zero, set frametime to this

#endif

grs_bitmap bmBackground;

#define BACKGROUND_NAME "statback.pcx"

//	Function prototypes for GAME.C exclusively.

int GameLoop (int RenderFlag, int ReadControlsFlag);
void FireLaser (void);
void SlideTextures (void);
void PowerupGrabCheatAll (void);

//	Other functions
void MultiCheckForKillGoalWinner ();
void MultiCheckForEntropyWinner ();
void RestoreGameSurfaces ();

// window functions

void grow_window (void);
void shrink_window (void);

// text functions

void fill_background ();

#ifndef RELEASE
void show_framerate (void);
void ftoa (char *string, fix f);
#endif

//	==============================================================================================

void LoadBackgroundBitmap ()
{
	int pcx_error;

if (bmBackground.bm_texBuf)
	d_free (bmBackground.bm_texBuf);
bmBackground.bm_texBuf=NULL;
pcx_error = pcx_read_bitmap (gameStates.app.cheats.bJohnHeadOn ? "johnhead.pcx" : BACKGROUND_NAME,
									  &bmBackground, BM_LINEAR,0);
if (pcx_error != PCX_ERROR_NONE)
	Error ("File %s - PCX error: %s",BACKGROUND_NAME,pcx_errormsg (pcx_error));
GrRemapBitmapGood (&bmBackground, NULL, -1, -1);
}


//------------------------------------------------------------------------------
//this is called once per game
void InitGame ()
{
	atexit (CloseGame);             //for cleanup
/*---*/LogErr ("Initializing game data\n  Objects ...\n");
	InitObjects ();
/*---*/LogErr ("  Special effects...\n");
	InitSpecialEffects ();
/*---*/LogErr ("  AI system...\n");
	InitAISystem ();
//*---*/LogErr ("  gauge canvases...\n");
//	InitGaugeCanvases ();
/*---*/LogErr ("  exploding wall...\n");
	InitExplodingWalls ();
/*---*/LogErr ("  particle systems...\n");
	InitSmoke ();
/*---*/LogErr ("  loading background bitmap...\n");
	LoadBackgroundBitmap ();
	nClearWindow = 2;		//	do portal only window clear.
	InitDetailLevels (gameStates.app.nDetailLevel);
//	BuildMissionList (0);
	/* Register cvars */
}

//------------------------------------------------------------------------------

void ResetPaletteAdd ()
{
gameStates.ogl.palAdd.red = 
gameStates.ogl.palAdd.green =
gameStates.ogl.palAdd.blue	= 0;
GrPaletteStepUp (0, 0, 0);
//gameStates.ogl.bDoPalStep = !(gameOpts->ogl.bSetGammaRamp && gameStates.ogl.bBrightness);
}

//------------------------------------------------------------------------------

#ifdef WINDOWS
void win_get_span_list (grs_bitmap *bm, int miny, int maxy)
{
	int x,y;
	int mode = 0;
	ubyte *data;
	int offset;
	int lspan=0, rspan=0, span=0;
//@@FILE *fp;

	data = bm->bm_texBuf;
//@@	fp = fopen ("cockspan.dat", "w");

	for (y = 0; y < miny; y++)
		win_cockpit_mask[y].num = 0;

	for (y = miny; y <= maxy; y++)
	{
		span = 0;
		//@@ fprintf (fp, "line %d: ", y);
		for (x = 0; x < bm->bm_props.w; x++)
		{
			offset = y*bm->bm_props.rowsize + x;

			if (data[offset] == 255) {
				switch (mode)
				{
					case 0:				// Start Mode
						lspan	= x;
						win_cockpit_mask[y].span[span].xmin = x;
						mode = 1;
					//@@	fprintf (fp, "<%d,", lspan);
						break;

					case 1:				// Transparency mode
						rspan = x;
						win_cockpit_mask[y].span[span].xmax = x;
						break;

					case 2:				// Switch from Draw mode to transparent
						lspan = x;
						win_cockpit_mask[y].span[span].xmin = x;
					//@@	fprintf (fp, "<%d,", lspan);
						mode = 1;
						break;
				}
			}
			else {
				switch (mode)
				{
					case 0:				// Start mode
						mode = 2;
						break;

					case 1:				// Switching from transparent to Draw
						rspan = x;
						mode = 2;
						win_cockpit_mask[y].span[span].xmax = x;
						span++;
					//@@	fprintf (fp, "%d> ", rspan);
						break;

					case 2:
						break;
				}
			}
		}
		if (mode == 1) {
		//@@	fprintf (fp, "%d> ", rspan);
			win_cockpit_mask[y].span[span].xmax = rspan;
			span++;
		}
		win_cockpit_mask[y].num = span;
	//@@	fprintf (fp, "\n");
		mode = 0;
	}
	win_cockpit_mask[y].num = 255;
}
#endif //WINDOWS

//------------------------------------------------------------------------------

void ShowInGameWarning (char *s)
{
if (grdCurScreen) {
	if (!((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StopTime ();
	gameData.menu.warnColor = MEDRED_RGBA;
	gameData.menu.colorOverride = gameData.menu.warnColor;
	ExecMessageBox (TXT_WARNING, NULL, -3, s, " ", TXT_OK);
	gameData.menu.colorOverride = 0;
	if (!((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME)))
		StartTime ();
	}
}

//------------------------------------------------------------------------------
//these should be in gr.h
#define cv_w  cv_bitmap.bm_props.w
#define cv_h  cv_bitmap.bm_props.h

int Game_window_x = 0;
int Game_window_y = 0;
int Game_window_w = 0;
int Game_window_h = 0;
int max_window_w = 0;
int max_window_h = 0;

extern void NDRecordCockpitChange (int);

//------------------------------------------------------------------------------
//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void InitCockpit ()
{
	if (gameStates.video.nScreenMode != SCREEN_GAME)
		return;

#if defined (POLY_ACC)
 pa_flush ();                 // get rid of undrawn polys.
 pa_clear_buffer (1, 0);
#endif

//Initialize the on-screen canvases

if (gameData.demo.nState==ND_STATE_RECORDING)
	NDRecordCockpitChange (gameStates.render.cockpit.nMode);
if (VR_render_mode != VR_NONE)
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
//if (!(VR_screen_flags & VRF_ALLOW_COCKPIT) && (gameStates.render.cockpit.nMode==CM_FULL_COCKPIT || gameStates.render.cockpit.nMode==CM_STATUS_BAR || gameStates.render.cockpit.nMode==CM_REAR_VIEW))
//	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
if (gameStates.video.nScreenMode == SCREEN_EDITOR)
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
#if 0//def OGL
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT || gameStates.render.cockpit.nMode == CM_REAR_VIEW) {
	HUDMessage (MSGC_GAME_FEEDBACK, "Cockpit not available in GL mode");
	gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
}
#endif

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
	);
GrSetCurFont (GAME_FONT);

#if !defined (MACINTOSH) && !defined (WINDOWS)
if (Game_cockpit_copy_code)
	d_free (Game_cockpit_copy_code);
Game_cockpit_copy_code  = NULL;
#else
if (Game_cockpit_copy_code)
	Game_cockpit_copy_code = 0;
#endif

//@@	#ifdef WINDOWS
//@@		if (_lpDDSMask) { DDFreeSurface (_lpDDSMask); _lpDDSMask = NULL; }
//@@	#endif

#ifdef WINDOWS
	game_win_init_cockpit_mask (0);
#endif

switch (gameStates.render.cockpit.nMode) {
#ifndef OGL
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW: {
		grs_bitmap *bm = &gameData.pig.tex.bitmaps[cockpit_bitmap[gameStates.render.cockpit.nMode+ (gameStates.video.nDisplayMode? (Num_cockpits/2):0)].index];
		PIGGY_PAGE_IN (cockpit_bitmap[gameStates.render.cockpit.nMode+ (gameStates.video.nDisplayMode? (Num_cockpits/2):0)]);

#ifdef WINDOWS
		DDGrSetCurrentCanvas (NULL);
		game_win_init_cockpit_mask (1);
		DDGrSetCurrentCanvas (dd_VR_offscreen_buffer);
#else
		GrSetCurrentCanvas (VR_offscreen_buffer);
#endif

		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBitmap (0, 0, bm);
		bm = &VR_offscreen_buffer->cv_bitmap;
		bm->bm_props.flags = BM_FLAG_TRANSPARENT;
		gr_ibitblt_find_hole_size (bm, &minx, &miny, &maxx, &maxy);
		WIN (	win_get_span_list (bm, miny, maxy);
				DDGRUNLOCK (dd_grd_curcanv)
			);

#ifndef WINDOWS
#ifndef __MSDOS__
		gr_ibitblt_create_mask (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
#else
		if (gameStates.video.nDisplayMode) {
#if defined (POLY_ACC)
			Game_cockpit_copy_code  = gr_ibitblt_create_mask_pa (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
			pa_clear_buffer (1, 0);      // clear offscreen to reduce white flash.
#else
			Game_cockpit_copy_code  = gr_ibitblt_create_mask_svga (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
#endif
		} else
			Game_cockpit_copy_code  = gr_ibitblt_create_mask (bm, minx, miny, maxx-minx+1, maxy-miny+1, VR_offscreen_buffer->cv_bitmap.bm_props.rowsize);
#endif
		bm->bm_props.flags = 0;		// Clear all flags for offscreen canvas
#else
		Game_cockpit_copy_code  = (ubyte *) (1);
		bm->bm_props.flags = 0;		// Clear all flags for offscreen canvas
#endif
		GameInitRenderSubBuffers (0, 0, maxx-minx+1, maxy-miny+1);
		break;
	}
#endif

#ifdef OGL
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:
     	max_window_h = (grdCurScreen->sc_h * 2) / 3;
		if (Game_window_h > max_window_h)
			Game_window_h = max_window_h;
		if (Game_window_w > max_window_w)
			Game_window_w = max_window_w;
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		GameInitRenderSubBuffers (Game_window_x, Game_window_y, Game_window_w, Game_window_h);
		break;
#endif
	case CM_FULL_SCREEN:
		max_window_h = grdCurScreen->sc_h;
//		if (Game_window_h > max_window_h || VR_screen_flags&VRF_ALLOW_COCKPIT)
			Game_window_h = max_window_h;
//		if (Game_window_w > max_window_w || VR_screen_flags&VRF_ALLOW_COCKPIT)
			Game_window_w = max_window_w;
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		GameInitRenderSubBuffers (Game_window_x, Game_window_y, Game_window_w, Game_window_h);
		break;

	case CM_STATUS_BAR:
		{
		int h = gameData.pig.tex.bitmaps [0][cockpit_bitmap[CM_STATUS_BAR+ (gameStates.video.nDisplayMode? (Num_cockpits/2):0)].index].bm_props.h;
		if (grdCurScreen->sc_h > 480)
			h = (int) ((double) h * (double) grdCurScreen->sc_h / 480.0);
     	max_window_h = grdCurScreen->sc_h - h;
//		if (Game_window_h > max_window_h || VR_screen_flags&VRF_ALLOW_COCKPIT)
			Game_window_h = max_window_h;
//		if (Game_window_w > max_window_w || VR_screen_flags&VRF_ALLOW_COCKPIT)
			Game_window_w = max_window_w;
		Game_window_x = (max_window_w - Game_window_w)/2;
		Game_window_y = (max_window_h - Game_window_h)/2;
		GameInitRenderSubBuffers (Game_window_x, Game_window_y, Game_window_w, Game_window_h);
		}
		break;

	case CM_LETTERBOX:	{
		int x,y,w,h;

		x = 0; w = VR_render_buffer[0].cv_bitmap.bm_props.w;		//VR_render_width;
		h = (VR_render_buffer[0].cv_bitmap.bm_props.h * 7) / 10;
		y = (VR_render_buffer[0].cv_bitmap.bm_props.h-h)/2;
		GameInitRenderSubBuffers (x, y, w, h);
		break;
		}

	}

WINDOS (
	DDGrSetCurrentCanvas (NULL),
	GrSetCurrentCanvas (NULL)
);
gameStates.gameplay.nShieldFlash = 0;
}

//------------------------------------------------------------------------------

//selects a given cockpit (or lack of one).  See types in game.h
void SelectCockpit (int mode)
{
	if (mode != gameStates.render.cockpit.nMode) {		//new mode
		gameStates.render.cockpit.nMode=mode;
		InitCockpit ();
	}
}

//------------------------------------------------------------------------------

extern int last_drawn_cockpit[2];

//force cockpit redraw next time. call this if you've trashed the screen
void ResetCockpit ()
{
	gameStates.render.cockpit.bRedraw=1;
	last_drawn_cockpit[0] = -1;
	last_drawn_cockpit[1] = -1;
}

//------------------------------------------------------------------------------

//NEWVR
void VRResetParams ()
{
	VR_eye_width = VR_SEPARATION;
	VR_eye_offset = VR_PIXEL_SHIFT;
	VR_eye_offset_changed = 2;
}

//------------------------------------------------------------------------------

void GameInitRenderSubBuffers (int x, int y, int w, int h)
{
#ifdef WINDOWS
	DDGrInitSubCanvas (&dd_VR_render_sub_buffer[0], &dd_VR_render_buffer[0], x, y, w, h);
	DDGrInitSubCanvas (&dd_VR_render_sub_buffer[1], &dd_VR_render_buffer[1], x, y, w, h);

	dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.x = 0;
	dd_VR_render_sub_buffer[0].canvas.cv_bitmap.bm_props.y = 0;
	dd_VR_render_sub_buffer[0].xoff = x;
	dd_VR_render_sub_buffer[0].yoff = y;
	dd_VR_render_sub_buffer[1].canvas.cv_bitmap.bm_props.x = 0;
	dd_VR_render_sub_buffer[1].canvas.cv_bitmap.bm_props.y = 0;
	dd_VR_render_sub_buffer[1].xoff = x;
	dd_VR_render_sub_buffer[1].yoff = y;

#endif
	if (Scanline_double) {
		#ifdef MACINTOSH
		if (w & 0x3)
			w &= ~0x3;
		GrInitSubCanvas (&VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w/2, (h/2)+1);
		GrInitSubCanvas (&VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w/2, (h/2)+1);
		#endif
	} else {
		GrInitSubCanvas (&VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w, h);
		GrInitSubCanvas (&VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w, h);
	}

	#ifdef MACINTOSH
		#ifdef POLY_ACC
			if (PAEnabled)
			{
				TQARect	newBounds;

				newBounds.left = x;
				newBounds.right = x + w;
				newBounds.top = y;
				newBounds.bottom = y + h;

				pa_set_context (kGamePlayDrawContextID, &newBounds);		// must resize/create new context
			}
		#endif
	#endif

#ifdef WINDOWS
	VR_render_sub_buffer[0].cv_bitmap.bm_props.x = 0;
	VR_render_sub_buffer[0].cv_bitmap.bm_props.y = 0;
	VR_render_sub_buffer[1].cv_bitmap.bm_props.x = 0;
	VR_render_sub_buffer[1].cv_bitmap.bm_props.y = 0;
#endif
}

//------------------------------------------------------------------------------

#ifdef WINDOWS
// Sets up the canvases we will be rendering to (WIN95)
void GameInitRenderBuffers (int screen_mode, int render_w, int render_h, int render_method, int flags)
{
//	Hack for switching to higher that 640x480 modes (DDraw doesn't allow
//	 creating surfaces greater than the current resolution

	if (GRMODEINFO (rw) < render_w || GRMODEINFO (rh) < render_h) {
		render_w = GRMODEINFO (rw);
		render_h = GRMODEINFO (rh);
	}
	VR_screen_mode		= screen_mode;
	VR_screen_flags	=  flags;
	VRResetParams ();
	VR_render_mode 	= render_method;
	Game_window_w 		= render_w;
	Game_window_h		= render_h;
	if (dd_VR_offscreen_buffer && dd_VR_offscreen_buffer != dd_grd_backcanv) {
		dd_gr_free_canvas (dd_VR_offscreen_buffer);
	}

	if ((VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED))	{
		if (render_h*2 < 200)	{
		 	Int3 ();		// Not Supported yet!!!
//			VR_offscreen_buffer = GrCreateCanvas (render_w, 200);
		}
		else {
			Int3 ();		// Not Supported yet!!!
//			VR_offscreen_buffer = GrCreateCanvas (render_w, render_h*2);
		}

		Int3 ();			// Not Supported yet!!!
//		GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
//		GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h);
	}
	else if (GRMODEINFO (paged) && !GRMODEINFO (dbuf)) {
	//	Here we will make the VR_offscreen_buffer the 2nd page and hopefully
	//	we can just flip it, saving a blt.

		dd_VR_offscreen_buffer = dd_grd_backcanv;
		VR_offscreen_buffer = & dd_grd_backcanv->canvas;
	}
	else if (GRMODEINFO (dbuf)||GRMODEINFO (emul)) {
	//	The offscreen buffer will be created.  We will just blt this
	//	to the screen (which may be blted to the primary surface)
		if (render_h < 200) {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (render_w, 200);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
		}
		else {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (render_w, render_h);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
		}
	}

	DDGrInitSubCanvas (&dd_VR_render_buffer[0], dd_VR_offscreen_buffer, 0, 0, render_w, render_h);
	DDGrInitSubCanvas (&dd_VR_render_buffer[1], dd_VR_offscreen_buffer, 0, 0, render_w, render_h);
	GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
	GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h);

	GameInitRenderSubBuffers (0, 0, render_w, render_h);
}

#else

//------------------------------------------------------------------------------

// Sets up the canvases we will be rendering to (NORMAL VERSION)
void GameInitRenderBuffers (int screen_mode, int render_w, int render_h, int render_method, int flags)
{
//	if (vga_check_mode (screen_mode) != 0)
//		Error ("Cannot set requested video mode");

	VR_screen_mode		=	screen_mode;
	VR_screen_flags	=  flags;
//NEWVR
	VRResetParams ();
	VR_render_mode 	= render_method;
	Game_window_w 		= render_w;
	Game_window_h		= render_h;
	if (VR_offscreen_buffer) {
		GrFreeCanvas (VR_offscreen_buffer);
	}

	if ((VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED))	{
		if (render_h*2 < 200)	{
			VR_offscreen_buffer = GrCreateCanvas (render_w, 200);
		}
		else {
			VR_offscreen_buffer = GrCreateCanvas (render_w, render_h*2);
		}

		GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
		GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h);
	}
	else {
		if (render_h < 200) {
			VR_offscreen_buffer = GrCreateCanvas (render_w, 200);
		}
		else {
#if defined (POLY_ACC)
			#ifndef MACINTOSH
            VR_offscreen_buffer = GrCreateCanvas (render_w, render_h);
            d_free (VR_offscreen_buffer->cv_bitmap.bm_texBuf);
            GrInitCanvas (VR_offscreen_buffer, pa_get_buffer_address (1), BM_LINEAR15, render_w, render_h);
			#else
				if (PAEnabled || gConfigInfo.mAcceleration) {
					gameStates.render.cockpit.nMode=CM_FULL_SCREEN;		// HACK HACK HACK HACK HACK!!!!
					VR_offscreen_buffer = GrCreateCanvas2 (render_w, render_h, BM_LINEAR15);
				} else
	            VR_offscreen_buffer = GrCreateCanvas (render_w, render_h);
			#endif
#else
            VR_offscreen_buffer = GrCreateCanvas (render_w, render_h);
#endif
        }

#ifdef OGL
		VR_offscreen_buffer->cv_bitmap.bm_props.type = BM_OGL;
#endif

		GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h);
		GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h);
	}

	GameInitRenderSubBuffers (0, 0, render_w, render_h);
}
#endif

//------------------------------------------------------------------------------

//called to get the screen in a mode compatible with popup menus.
//if we can't have popups over the game screen, switch to menu mode.

void SetPopupScreenMode (void)
{
	//WIN (LoadCursorWin (MOUSE_DEFAULT_CURSOR);

#ifdef OGL // always have to switch to menu mode
	if (!gameOpts->menus.nStyle || (gameStates.video.nScreenMode < 0))
#else
	if (!(VR_screen_flags & VRF_COMPATIBLE_MENUS))
#endif
	{
//		gameStates.video.nLastScreenMode = -1;
//		gameStates.app.bGameRunning = (gameStates.app.nFunctionMode == FMODE_GAME);
		SetScreenMode (SCREEN_MENU);		//must switch to menu mode
	}
}

//------------------------------------------------------------------------------
//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int SetScreenMode (u_int32_t sm)
{
WIN (static int force_mode_change=0);
WIN (static int saved_window_w);
WIN (static int saved_window_h);

#ifdef EDITOR
	if ((sm==SCREEN_MENU) && (gameStates.video.nScreenMode==SCREEN_EDITOR))	{
		GrSetCurrentCanvas (Canv_editor);
		return 1;
	}
#endif

#ifdef WINDOWS
	if (gameStates.video.nScreenMode == sm && W95DisplayMode == VR_screen_mode) {
		DDGrSetCurrentCanvas (&dd_VR_screen_pages[VR_current_page]);
		return 1;
	}
#else
#	ifdef OGL
	if ((gameStates.video.nScreenMode == sm) && (VGA_current_mode == VR_screen_mode) && 
		 (/* (sm != SCREEN_GAME) ||*/ (grdCurScreen->sc_mode == VR_screen_mode))) {
		GrSetCurrentCanvas (VR_screen_pages + VR_current_page);
		OglSetScreenMode ();
		return 1;
	}
#	else
	if (gameStates.video.nScreenMode == sm && VGA_current_mode == VR_screen_mode) {
		GrSetCurrentCanvas (&VR_screen_pages[VR_current_page]);
		return 1;
	}
#	endif
#endif


#ifdef EDITOR
	Canv_editor = NULL;
#endif

	gameStates.video.nScreenMode = sm;

	switch (gameStates.video.nScreenMode)
	{
		case SCREEN_MENU:
		#ifdef WINDOWS
			//mouse_set_mode (0);
			//ShowCursorW ();
			if (!(VR_screen_flags & VRF_COMPATIBLE_MENUS)) {
			// HACK!!!  Meant to save window size when switching from
			// non-compat menu mode to menu mode.
				saved_window_w = Game_window_w;
				saved_window_h = Game_window_h;
				force_mode_change = 1;
			}
			if (W95DisplayMode != SM95_640x480x8) {
//@@				PiggyBitmapPageOutAllW ();		// 2D GFX Flush cache.
				DDSETDISPLAYMODE (SM95_640x480x8);
				dd_gr_init_screen ();
				if (!gameStates.render.bPaletteFadedOut) 
					GrPaletteStepLoad (NULL);
			}

			DDGrInitSubCanvas (&dd_VR_screen_pages[0], dd_grd_screencanv,
									0,0,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.w,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.h);
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_screencanv,
									0,0,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.w,
									dd_grd_screencanv->canvas.cv_bitmap.bm_props.h);
			gameStates.menus.bHires = 1;
			gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable;

		#else
		{
			u_int32_t menu_mode;

			gameStates.menus.bHires = gameStates.menus.bHiresAvailable;		//do highres if we can

#if defined (POLY_ACC)
				#ifndef MACINTOSH
	            menu_mode = gameStates.menus.bHires?SM (640,480):SM (320,200);
				#else
					menu_mode = PAEnabled?SM_640x480x15xPA:SM_640x480V;
				#endif
#else
            menu_mode = 
					gameStates.menus.bHires?
						 (VR_screen_mode >= SM (640,480))?
							VR_screen_mode
							:SM (640,480)
						:SM (320,200);
#endif
			gameStates.video.nLastScreenMode = -1;
#if 0
			if (gameStates.menus.bFullScreen) {
				gameStates.menus.bFullScreen = 0;
				SDL_SetVideoMode (gameStates.video.nWidth, gameStates.video.nHeight, gameStates.ogl.nColorBits, 
										 SDL_OPENGL | (gameStates.ogl.bFullScreen ? SDL_FULLSCREEN : 0));
				VGA_current_mode = -1;
				}
#endif
			if (VGA_current_mode != menu_mode) {
				if (GrSetMode (menu_mode))
					Error ("Cannot set screen mode for menu");
				if (!gameStates.render.bPaletteFadedOut)
					GrPaletteStepLoad (NULL);
				gameStates.menus.bInitBG = 1;
				RebuildGfxFx (gameStates.app.bGameRunning, 1);
			}

			GrInitSubCanvas (
				VR_screen_pages, &grdCurScreen->sc_canvas, 0, 0, 
				grdCurScreen->sc_w, grdCurScreen->sc_h);
			GrInitSubCanvas (
				VR_screen_pages + 1, &grdCurScreen->sc_canvas, 0, 0, 
				grdCurScreen->sc_w, grdCurScreen->sc_h);

			gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && gameStates.menus.bHires;

		}
		#endif
		break;

	case SCREEN_GAME:
#if 0 //def WINDOWS
		//mouse_set_mode (1);
		HideCursorW ();
		if (force_mode_change || (W95DisplayMode != VR_screen_mode)) {

			DDSETDISPLAYMODE (VR_screen_mode);
//@@			PiggyBitmapPageOutAllW ();		// 2D GFX Flush cache.
			dd_gr_init_screen ();
#if TRACE
			//con_printf (CON_DEBUG, "Reinitializing render buffers due to display mode change.\n");
#endif
			GameInitRenderBuffers (W95DisplayMode,
					GRMODEINFO (rw), GRMODEINFO (rh),
					VR_render_mode, VR_screen_flags);

			ResetCockpit ();
		}
#else
//		VR_screen_mode = SM (Game_window_w,Game_window_h);
		if (VGA_current_mode != VR_screen_mode) {
			if (GrSetMode (VR_screen_mode))	{
				Error ("Cannot set desired screen mode for game!");
				//we probably should do something else here, like select a standard mode
			}
			#ifdef MACINTOSH
			if ((gameConfig.nControlType == 1) && (gameStates.app.nFunctionMode == FMODE_GAME))
				joydefs_calibrate ();
			#endif
			ResetCockpit ();
		}
#endif
#if 0
		if (VR_render_mode != VR_NONE)
			gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
		else 
#endif
			{
			max_window_w = grdCurScreen->sc_w;
			max_window_h = grdCurScreen->sc_h;
#if 0
			if (VR_screen_flags & VRF_ALLOW_COCKPIT) {
				if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		      	max_window_h = grdCurScreen->sc_h - gameData.pig.tex.bitmaps[cockpit_bitmap[CM_STATUS_BAR+ (gameStates.video.nDisplayMode? (Num_cockpits/2):0)].index].bm_props.h;
				}
			else if (gameStates.render.cockpit.nMode != CM_LETTERBOX)
				gameStates.render.cockpit.nMode = CM_FULL_SCREEN;
#endif
	      if (!Game_window_h || (Game_window_h > max_window_h) || 
				 !Game_window_w || (Game_window_w > max_window_w)) {
				Game_window_w = max_window_w;
				Game_window_h = max_window_h;
				}
			}
		InitCockpit ();

		#ifdef WINDOWS
		//	Super hack.   If we are switching from a 320x200 game to 640x480.
		//						and we were in a menumode when switching, we don't
		//						restore Game_window vals
			if (force_mode_change && (W95DisplayMode == SM95_320x200x8X)) {
				Game_window_w = saved_window_w;
				Game_window_h = saved_window_h;
				force_mode_change = 0;
			}
		#endif


	//	Define screen pages for game mode
	// If we designate through screen_flags to use paging, then do so.
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[0], dd_grd_screencanv, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h),
			GrInitSubCanvas (&VR_screen_pages[0], &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h)
		);

		if (VR_screen_flags&VRF_USE_PAGING) {
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_backcanv, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h),
			GrInitSubCanvas (&VR_screen_pages[1], &grdCurScreen->sc_canvas, 0, grdCurScreen->sc_h, grdCurScreen->sc_w, grdCurScreen->sc_h)
		);
		}
		else {
		WINDOS (
			DDGrInitSubCanvas (&dd_VR_screen_pages[1], dd_grd_screencanv, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h),
			GrInitSubCanvas (&VR_screen_pages[1], &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h)
		);
		}

		InitCockpit ();

	#ifdef WINDOWS
		gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.video.nDisplayMode != 0);
		gameStates.menus.bHires = 1;
	#else
		gameStates.render.fonts.bHires = gameStates.render.fonts.bHiresAvailable && (gameStates.menus.bHires = (gameStates.video.nDisplayMode > 1));
	#endif

		if (VR_render_mode != VR_NONE)	{
			// for 640x480 or higher, use hires font.
			if (gameStates.render.fonts.bHiresAvailable && (grdCurScreen->sc_h > 400))
				gameStates.render.fonts.bHires = 1;
			else
				gameStates.render.fonts.bHires = 0;
		}
		con_resize ();
		break;

	#ifdef EDITOR
	case SCREEN_EDITOR:
		if (grdCurScreen->sc_mode != SM (800,600))	{
			int gr_error;
			if ((gr_error=GrSetMode (SM (800,600)))!=0) { //force into game scrren
				Warning ("Cannot init editor screen (error=%d)",gr_error);
				return 0;
			}
		}
		GrPaletteStepLoad (NULL);

		GrInitSubCanvas (&VR_editor_canvas, &grdCurScreen->sc_canvas, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
		Canv_editor = &VR_editor_canvas;
		GrInitSubCanvas (&VR_screen_pages[0], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		GrInitSubCanvas (&VR_screen_pages[1], Canv_editor, 0, 0, Canv_editor->cv_w, Canv_editor->cv_h);
		GrSetCurrentCanvas (Canv_editor);
		init_editor_screen ();   //setup other editor stuff
		break;
	#endif
	default:
		Error ("Invalid screen mode %d",sm);
	}

	VR_current_page = 0;

	WINDOS (
		DDGrSetCurrentCanvas (&dd_VR_screen_pages[VR_current_page]),
		GrSetCurrentCanvas (&VR_screen_pages[VR_current_page])
	);

	if (VR_screen_flags&VRF_USE_PAGING)	{
	WINDOS (
		dd_gr_flip (),
		GrShowCanvas (&VR_screen_pages[VR_current_page])
	);
	}
#ifdef OGL
	OglSetScreenMode ();
#endif

	return 1;
}

//------------------------------------------------------------------------------

int GrToggleFullScreenGame (void)
{
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	int i=GrToggleFullScreen ();
	//added 2000/06/19 Matthew Mueller - hack to fix "infinite toggle" problem
	//it seems to be that the screen mode change takes long enough that the key has already sent repeat codes, or that its unpress event gets dropped, etc.  This is a somewhat ugly fix, but it works.
//	generic_key_handler (KEY_PADENTER,0);
	FlushInput ();
	if (gameStates.app.bGameRunning) {
		HUDMessage (MSGC_GAME_FEEDBACK, "toggling fullscreen mode %s",i?"on":"off");
		StopPlayerMovement ();
		//RebuildGfxFx (gameStates.app.bGameRunning);
		}
	//end addition -MM
	return i;
#else
	if (gameStates.app.bGameRunning)
		HUDMessage (MSGC_GAME_FEEDBACK, "fullscreen toggle not supported by this target");
	return -1;
#endif
}

//------------------------------------------------------------------------------

int arch_toggle_fullscreen_menu (void);

int GrToggleFullScreenMenu (void)
{
#ifdef GR_SUPPORTS_FULLSCREEN_MENU_TOGGLE
	int i;
	i=arch_toggle_fullscreen_menu ();

//	generic_key_handler (KEY_PADENTER,0);
	FlushInput ();
	StopPlayerMovement ();
	RebuildGfxFx (gameStates.app.bGameRunning, 1);
	return i;
#else
	return -1;
#endif
}

//------------------------------------------------------------------------------

int nTimerPaused=0;

void StopTime ()
{
if (++nTimerPaused == 1) {
	fix time = TimerGetFixedSeconds ();
	last_timer_value = time - last_timer_value;
	if (last_timer_value < 0) {
#if defined (TIMER_TEST) && !defined (NDEBUG)
		Int3 ();		//get Matt!!!!
#endif
		last_timer_value = 0;
		}
#if defined (TIMER_TEST) && !defined (NDEBUG)
	time_stopped = time;
	#endif
}
#if defined (TIMER_TEST) && !defined (NDEBUG)
stop_count++;
#endif
}

//------------------------------------------------------------------------------

void StartTime ()
{
Assert (nTimerPaused > 0);
if (!--nTimerPaused) {
	fix time = TimerGetFixedSeconds ();
#if defined (TIMER_TEST) && !defined (NDEBUG)
	if (last_timer_value < 0)
		Int3 ();		//get Matt!!!!
#endif
	last_timer_value = time - last_timer_value;
#if defined (TIMER_TEST) && !defined (NDEBUG)
	time_started = time;
#endif
	}
#if defined (TIMER_TEST) && !defined (NDEBUG)
start_count++;
#endif
}

//------------------------------------------------------------------------------

int TimeStopped (void)
{
return nTimerPaused > 0;
}

//------------------------------------------------------------------------------

extern ubyte joydefs_calibrating;

void GameFlushInputs ()
{
	int dx,dy;
#if 1
	FlushInput ();
#else	
	KeyFlush ();
	joy_flush ();
	mouse_flush ();
#endif	
	#ifdef MACINTOSH
	if ((gameStates.app.nFunctionMode != FMODE_MENU) && !joydefs_calibrating)		// only reset mouse when not in menu or not calibrating
	#endif
		MouseGetDelta (&dx, &dy);	// Read mouse
	memset (&Controls,0,sizeof (control_info));
}

//------------------------------------------------------------------------------

void ResetTime ()
{
	last_timer_value = TimerGetFixedSeconds ();

}

//------------------------------------------------------------------------------

#ifndef RELEASE
extern int Saving_movie_frames;
int Movie_fixed_frametime;
#else
#define Saving_movie_frames	0
#define Movie_fixed_frametime	0
#endif


void CalcFrameTime ()
{
	fix 	timer_value,
			last_frametime = gameData.app.xFrameTime,
			minFrameTime = (gameOpts->render.nMaxFPS ? f1_0 / gameOpts->render.nMaxFPS : 1);

if (gameData.app.bGamePaused) {
	last_timer_value = TimerGetFixedSeconds ();
	gameData.app.xFrameTime = 0;
	gameData.app.xRealFrameTime = 0;
	return;
	}
#if defined (TIMER_TEST) && !defined (NDEBUG)
_last_frametime = last_frametime;
#endif
do {
	timer_value = TimerGetFixedSeconds ();
   gameData.app.xFrameTime = timer_value - last_timer_value;
	timer_delay (1);
	if (gameOpts->render.nMaxFPS < 2)
		break;
	} while (gameData.app.xFrameTime < minFrameTime);
#if defined (TIMER_TEST) && !defined (NDEBUG)
_timer_value = timer_value;
#endif
#ifdef _DEBUG
if ((gameData.app.xFrameTime <= 0) || 
	 (gameData.app.xFrameTime > F1_0) || 
	 (gameStates.app.nFunctionMode == FMODE_EDITOR) || 
	 (gameData.demo.nState == ND_STATE_PLAYBACK)) {
#if TRACE
//con_printf (1,"Bad gameData.app.xFrameTime - value = %x\n",gameData.app.xFrameTime);
#endif
if (gameData.app.xFrameTime == 0)
	Int3 ();	//	Call Mike or Matt or John!  Your interrupts are probably trashed!
	}
#endif
#if defined (TIMER_TEST) && !defined (NDEBUG)
actual_last_timer_value = last_timer_value;
#endif
if (gameStates.app.cheats.bTurboMode)
	gameData.app.xFrameTime *= 2;
// Limit frametime to be between 5 and 150 fps.
gameData.app.xRealFrameTime = gameData.app.xFrameTime;
#if 0
if (gameData.app.xFrameTime < F1_0/150) 
	gameData.app.xFrameTime = F1_0/150;
if (gameData.app.xFrameTime > F1_0/5) 
	gameData.app.xFrameTime = F1_0/5;
#endif
last_timer_value = timer_value;
if (gameData.app.xFrameTime < 0)						//if bogus frametimed:\temp\dm_test.
	gameData.app.xFrameTime = last_frametime;		//d:\temp\dm_test.then use time from last frame
#ifndef NDEBUG
if (fixed_frametime) 
	gameData.app.xFrameTime = fixed_frametime;
#endif
#ifndef NDEBUG
// Pause here!!!
if (Debug_pause) {
	int c = 0;
	while (!c)
		c = key_peekkey ();
	if (c == KEY_P)       {
		Debug_pause = 0;
		c = KeyInKey ();
		}
	last_timer_value = TimerGetFixedSeconds ();
	}
#endif
#if Arcade_mode
gameData.app.xFrameTime /= 2;
#endif
#if defined (TIMER_TEST) && !defined (NDEBUG)
stop_count = start_count = 0;
#endif
//	Set value to determine whether homing missile can see target.
//	The lower frametime is, the more likely that it can see its target.
if (gameData.app.xFrameTime <= F1_0/64)
	xMinTrackableDot = MIN_TRACKABLE_DOT;	// -- 3* (F1_0 - MIN_TRACKABLE_DOT)/4 + MIN_TRACKABLE_DOT;
else if (gameData.app.xFrameTime < F1_0/32)
	xMinTrackableDot = MIN_TRACKABLE_DOT + F1_0/64 - 2*gameData.app.xFrameTime;	// -- fixmul (F1_0 - MIN_TRACKABLE_DOT, F1_0-4*gameData.app.xFrameTime) + MIN_TRACKABLE_DOT;
else if (gameData.app.xFrameTime < F1_0/4)
	xMinTrackableDot = MIN_TRACKABLE_DOT + F1_0/64 - F1_0/16 - gameData.app.xFrameTime;	// -- fixmul (F1_0 - MIN_TRACKABLE_DOT, F1_0-4*gameData.app.xFrameTime) + MIN_TRACKABLE_DOT;
else
	xMinTrackableDot = MIN_TRACKABLE_DOT + F1_0/64 - F1_0/8;
}

//------------------------------------------------------------------------------

//--unused-- int Auto_flythrough=0;  //if set, start flythough automatically

void MovePlayerToSegment (segment *segP,int side)
{
	vms_vector vp;

COMPUTE_SEGMENT_CENTER (&gameData.objs.console->pos,segP);
COMPUTE_SIDE_CENTER (&vp,segP,side);
VmVecDec (&vp,&gameData.objs.console->pos);
VmVector2Matrix (&gameData.objs.console->orient, &vp, NULL, NULL);
RelinkObject (OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM (segP));
}

//------------------------------------------------------------------------------

#ifdef NETWORK
void GameDrawTimeLeft ()
{
        char temp_string[30];
        fix timevar;
        int i;

        GrSetCurFont (GAME_FONT);    //GAME_FONT
        GrSetFontColorRGBi (RED_RGBA, 1, 0, 0);

        timevar=i2f (netGame.PlayTimeAllowed*5*60);
        i=f2i (timevar-ThisLevelTime);
        i++;

        sprintf (temp_string, TXT_TIME_LEFT, i);

        if (i>=0)
         GrString (0, 32, temp_string);
}
#endif

//------------------------------------------------------------------------------

void do_photos ();
void level_with_floor ();

void modex_clear_box (int x,int y,int w,int h)
{
	grs_canvas *temp_canv,*save_canv;

	save_canv = grdCurCanv;
	temp_canv = GrCreateCanvas (w,h);
	GrSetCurrentCanvas (temp_canv);
	GrClearCanvas (BLACK_RGBA);
	GrSetCurrentCanvas (save_canv);
	GrBitmapM (x,y,&temp_canv->cv_bitmap);
	GrFreeCanvas (temp_canv);

}

//------------------------------------------------------------------------------

extern void modex_printf (int x,int y,char *s,grs_font *font,int color);

// mac routine to drop contents of screen to a pict file using copybits
// save a PICT to a file
#ifdef MACINTOSH

void SavePictScreen (int multiplayer)
{
	OSErr err;
	int parid, i, count;
	char *pfilename, filename[50], buf[512], cwd[FILENAME_MAX];
	short fd;
	FSSpec spec;
	PicHandle pict_handle;
	static int multi_count = 0;
	StandardFileReply sf_reply;

// dump the contents of the GameWindow into a picture using copybits

	pict_handle = OpenPicture (&GameWindow->portRect);
	if (pict_handle == NULL)
		return;

	CopyBits (&GameWindow->portBits, &GameWindow->portBits, &GameWindow->portRect, &GameWindow->portRect, srcBic, NULL);
	ClosePicture ();

// get the cwd to restore with chdir when done -- this keeps the mac world sane
	if (!getcwd (cwd, FILENAME_MAX))
		Int3 ();
// create the fsspec

	sprintf (filename, "screen%d", multi_count++);
	pfilename = c2pstr (filename);
	if (!multiplayer) {
		show_cursor ();
		StandardPutFile ("\pSave PICT as:", pfilename, &sf_reply);
		if (!sf_reply.sfGood)
			goto end;
		memcpy (&spec, & (sf_reply.sfFile), sizeof (FSSpec));
		if (sf_reply.sfReplacing)
			FSpDelete (&spec);
		err = FSpCreate (&spec, 'ttxt', 'PICT', smSystemScript);
		if (err)
			goto end;
	} else {
//		parid = GetAppDirId ();
		err = FSMakeFSSpec (0, 0, pfilename, &spec);
		if (err == nsvErr)
			goto end;
		if (err != fnfErr)
			FSpDelete (&spec);
		err = FSpCreate (&spec, 'ttxt', 'PICT', smSystemScript);
		if (err != 0)
			goto end;
	}

// write the PICT file
	if (FSpOpenDF (&spec, fsRdWrPerm, &fd))
		goto end;
	memset (buf, 0, sizeof (buf);
	count = 512;
	if (FSWrite (fd, &count, buf))
		goto end;
	count = GetHandleSize ((Handle)pict_handle);
	HLock ((Handle)pict_handle);
	if (FSWrite (fd, &count, *pict_handle)) {
		FSClose (fd);
		FSpDelete (&spec);
	}

end:
	HUnlock ((Handle)pict_handle);
	DisposeHandle ((Handle)pict_handle);
	FSClose (fd);
	hide_cursor ();
	chdir (cwd);
}

#endif

//------------------------------------------------------------------------------

//automap_flag is now unused, since we just check if the screen we're
//writing to is modex
//if called from automap, current canvas is set to visible screen
#ifndef OGL
void SaveScreenShot (NULL, int automap_flag)
{
#if defined (WINDOWS)
#if TRACE
	//con_printf (CON_DEBUG, "Doing screen shot thing.\n");
#endif
	win95_save_pcx_shot ();

#elif !defined (MACINTOSH)
	fix t1;
	char message[100];
	grs_canvas *screen_canv=&grdCurScreen->sc_canvas;
	grs_font *save_font;
	static int savenum=0;
	static int stereo_savenum=0;
	grs_canvas *temp_canv,*temp_canv2,*save_canv;
        char savename[FILENAME_LEN],savename2[FILENAME_LEN];
	ubyte pal[768];
	int w,h,aw,x,y;
	int modex_flag;
	int stereo=0;

	temp_canv2=NULL;

//	// Can't do screen shots in VR modes.
//	if (VR_render_mode != VR_NONE)
//		return;

	StopTime ();

	save_canv = grdCurCanv;

	if (VR_render_mode != VR_NONE && !automap_flag && gameStates.app.nFunctionMode==FMODE_GAME && gameStates.video.nScreenMode==SCREEN_GAME)
		stereo = 1;

	if (stereo) {
		temp_canv = GrCreateCanvas (VR_render_buffer[0].cv_bitmap.bm_props.w,VR_render_buffer[0].cv_bitmap.bm_props.h);
		GrSetCurrentCanvas (temp_canv);
		gr_ubitmap (0,0,&VR_render_buffer[0].cv_bitmap);

		temp_canv2 = GrCreateCanvas (VR_render_buffer[1].cv_bitmap.bm_props.w,VR_render_buffer[1].cv_bitmap.bm_props.h);
		GrSetCurrentCanvas (temp_canv2);
		gr_ubitmap (0,0,&VR_render_buffer[1].cv_bitmap);
	}
	else {
		temp_canv = GrCreateCanvas (screen_canv->cv_bitmap.bm_props.w,screen_canv->cv_bitmap.bm_props.h);
		GrSetCurrentCanvas (temp_canv);
		gr_ubitmap (0,0,&screen_canv->cv_bitmap);
	}

	GrSetCurrentCanvas (save_canv);

	if (savenum > 99) savenum = 0;
	if (stereo_savenum > 99) stereo_savenum = 0;

	if (stereo) {
		sprintf (savename,"left%02d.pcx",stereo_savenum);
		sprintf (savename2,"right%02d.pcx",stereo_savenum);
		if (VR_eye_switch) {char t[FILENAME_LEN]; strcpy (t,savename); strcpy (savename,savename2); strcpy (savename2,t);}
		stereo_savenum++;
		sprintf (message, "%s '%s' & '%s'", TXT_DUMPING_SCREEN, savename, savename2);
	}
	else {
		sprintf (savename,"screen%02d.pcx",savenum++);
		sprintf (message, "%s '%s'", TXT_DUMPING_SCREEN, savename);
	}

	if (!automap_flag)		//if from automap, curcanv is already visible canv
		GrSetCurrentCanvas (NULL);
	modex_flag = (grdCurCanv->cv_bitmap.bm_props.type==BM_MODEX);
	if (!automap_flag && modex_flag)
		GrSetCurrentCanvas (&VR_screen_pages[VR_current_page]);

	save_font = grdCurCanv->cv_font;
	GrSetCurFont (GAME_FONT);
	GrSetFontColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	GrGetStringSize (message,&w,&h,&aw);

	if (modex_flag)
		h *= 2;

	//I changed how these coords were calculated for the high-res automap. -MT
	//x = (VR_screen_pages[VR_current_page].cv_w-w)/2;
	//y = (VR_screen_pages[VR_current_page].cv_h-h)/2;
	x = (grdCurCanv->cv_w-w)/2;
	y = (grdCurCanv->cv_h-h)/2;

	if (modex_flag) {
		modex_clear_box (x-2,y-2,w+4,h+4);
		modex_printf (x, y, message,GAME_FONT,GrFindClosestColorCurrent (0,31,0);
	} else {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (x-2,y-2,x+w+2,y+h+2);
		GrPrintF (x,y,message);
		GrSetCurFont (save_font);
	}
	t1 = TimerGetFixedSeconds () + F1_0;

	GrPaletteRead (pal);		//get actual palette from the hardware
	pcx_write_bitmap (savename,&temp_canv->cv_bitmap,pal);
	if (stereo)
		pcx_write_bitmap (savename2,&temp_canv2->cv_bitmap,pal);

	while (TimerGetFixedSeconds () < t1);		// Wait so that messag stays up at least 1 second.

	GrSetCurrentCanvas (screen_canv);

	if (grdCurCanv->cv_bitmap.bm_props.type!=BM_MODEX && !stereo)
		gr_ubitmap (0,0,&temp_canv->cv_bitmap);

	GrFreeCanvas (temp_canv);
	if (stereo)
		GrFreeCanvas (temp_canv2);

	GrSetCurrentCanvas (save_canv);
	KeyFlush ();
	StartTime ();

#else

	grs_canvas *screen_canv = &grdCurScreen->sc_canvas;
	grs_canvas *temp_canv, *save_canv;

	// Can't do screen shots in VR modes.
	if (VR_render_mode != VR_NONE)
		return;

	StopTime ();

	save_canv = grdCurCanv;
	temp_canv = GrCreateCanvas (screen_canv->cv_bitmap.bm_props.w, screen_canv->cv_bitmap.bm_props.h);
	if (!temp_canv)
		goto shot_done;
	GrSetCurrentCanvas (temp_canv);
	gr_ubitmap (0, 0, &screen_canv->cv_bitmap);
	GrSetCurrentCanvas (&VR_screen_pages[VR_current_page]);

	show_cursor ();
	key_close ();
	if (gameData.app.nGameMode & GM_MULTI)
		SavePictScreen (1);
	else
		SavePictScreen (0);
	key_init ();
	hide_cursor ();

	GrSetCurrentCanvas (screen_canv);

//	if (!automap_flag)
		gr_ubitmap (0, 0, &temp_canv->cv_bitmap);

	GrFreeCanvas (temp_canv);
shot_done:
	GrSetCurrentCanvas (save_canv);
	KeyFlush ();
	StartTime ();
	#endif
}

#endif

//------------------------------------------------------------------------------

//initialize flying
void FlyInit (object *objP)
{
	objP->control_type = CT_FLYING;
	objP->movement_type = MT_PHYSICS;

	VmVecZero (&objP->mtype.phys_info.velocity);
	VmVecZero (&objP->mtype.phys_info.thrust);
	VmVecZero (&objP->mtype.phys_info.rotvel);
	VmVecZero (&objP->mtype.phys_info.rotthrust);
}

//void morph_test (), morph_step ();


//	------------------------------------------------------------------------------------

void test_anim_states ();

#include "fvi.h"

//put up the help message
void do_show_help ()
{
	ShowHelp ();
}


extern int been_in_editor;

//	------------------------------------------------------------------------------------

void DoCloakStuff (void)
{
	int i;
	for (i = 0; i < gameData.multi.nPlayers; i++)
		if (gameData.multi.players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (gameData.app.xGameTime - gameData.multi.players[i].cloak_time > CLOAK_TIME_MAX) {
				gameData.multi.players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == gameData.multi.nLocalPlayer) {
					DigiPlaySample (SOUND_CLOAK_OFF, F1_0);
					#ifdef NETWORK
					if (gameData.app.nGameMode & GM_MULTI)
						MultiSendPlaySound (SOUND_CLOAK_OFF, F1_0);
					MaybeDropNetPowerup (-1, POW_CLOAK, FORCE_DROP);
					MultiSendDeCloak (); // For demo recording
					#endif
				}
			}
		}
}

int FakingInvul=0;

//	------------------------------------------------------------------------------------

void DoInvulnerableStuff (void)
{
	if (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (gameData.app.xGameTime - gameData.multi.players[gameData.multi.nLocalPlayer].invulnerable_time > INVULNERABLE_TIME_MAX) {
			gameData.multi.players[gameData.multi.nLocalPlayer].flags ^= PLAYER_FLAGS_INVULNERABLE;
			if (FakingInvul==0)
			{
				DigiPlaySample (SOUND_INVULNERABILITY_OFF, F1_0);
				#ifdef NETWORK
				if (gameData.app.nGameMode & GM_MULTI)
				{
					MultiSendPlaySound (SOUND_INVULNERABILITY_OFF, F1_0);
					MaybeDropNetPowerup (-1, POW_INVULNERABILITY, FORCE_DROP);
				}
				#endif
#if TRACE
				//con_printf (CON_DEBUG, " --- You have been DE-INVULNERABLEIZED! ---\n");
#endif
			}
			FakingInvul=0;
		}
	}
}

ubyte	Last_afterburner_state = 0;
fix Last_afterburner_charge = 0;

#define AFTERBURNER_LOOP_START	 ((gameOpts->sound.digiSampleRate==SAMPLE_RATE_22K)?32027: (32027/2))		//20098
#define AFTERBURNER_LOOP_END		 ((gameOpts->sound.digiSampleRate==SAMPLE_RATE_22K)?48452: (48452/2))		//25776

int	Ab_scale = 4;

//@@//	------------------------------------------------------------------------------------
//@@void afterburner_shake (void)
//@@{
//@@	int	rx, rz;
//@@
//@@	rx = (Ab_scale * fixmul (d_rand () - 16384, F1_0/8 + (((gameData.app.xGameTime + 0x4000)*4) & 0x3fff)))/16;
//@@	rz = (Ab_scale * fixmul (d_rand () - 16384, F1_0/2 + ((gameData.app.xGameTime*4) & 0xffff)))/16;
//@@
//@@	gameData.objs.console->mtype.phys_info.rotvel.x += rx;
//@@	gameData.objs.console->mtype.phys_info.rotvel.z += rz;
//@@
//@@}

//	------------------------------------------------------------------------------------
#ifdef NETWORK
extern void MultiSendSoundFunction (char,char);
#endif

void DoAfterburnerStuff (void)
{
   if (!(gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER))
		xAfterburnerCharge=0;

	if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead)
		{
		 if (DigiKillSoundLinkedToObject (gameData.multi.players[gameData.multi.nLocalPlayer].objnum))
#ifdef NETWORK
			MultiSendSoundFunction (0,0)
#endif
		 ;
		}

	else if ((Last_afterburner_charge && (Controls.afterburner_state != Last_afterburner_state)) || 
		 (Last_afterburner_state && (Last_afterburner_charge && !xAfterburnerCharge))) {
		if (xAfterburnerCharge && Controls.afterburner_state && 
			 (gameData.multi.players[gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_AFTERBURNER)) {
			DigiLinkSoundToObject3 ((short) SOUND_AFTERBURNER_IGNITE, (short) gameData.multi.players[gameData.multi.nLocalPlayer].objnum, 1, F1_0, 
												 i2f (256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendSoundFunction (3, (char) SOUND_AFTERBURNER_IGNITE);
#endif
			}
		else {
			DigiKillSoundLinkedToObject (gameData.multi.players[gameData.multi.nLocalPlayer].objnum);
			DigiLinkSoundToObject2 ((short) SOUND_AFTERBURNER_PLAY, (short) gameData.multi.players[gameData.multi.nLocalPlayer].objnum, 0, F1_0, 
												 i2f (256));
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI)
			 	MultiSendSoundFunction (0,0);
#endif
#if TRACE
			//con_printf (CON_DEBUG,"Killing afterburner sound\n");
#endif
			}
		}

	//@@if (Controls.afterburner_state && xAfterburnerCharge)
	//@@	afterburner_shake ();

	Last_afterburner_state = Controls.afterburner_state;
	Last_afterburner_charge = xAfterburnerCharge;
}

// -- //	------------------------------------------------------------------------------------
// -- //	if energy < F1_0/2, recharge up to F1_0/2
// -- void recharge_energy_frame (void)
// -- {
// -- 	if (gameData.multi.players[gameData.multi.nLocalPlayer].energy < gameData.weapons.info[0].energy_usage) {
// -- 		gameData.multi.players[gameData.multi.nLocalPlayer].energy += gameData.app.xFrameTime/4;
// --
// -- 		if (gameData.multi.players[gameData.multi.nLocalPlayer].energy > gameData.weapons.info[0].energy_usage)
// -- 			gameData.multi.players[gameData.multi.nLocalPlayer].energy = gameData.weapons.info[0].energy_usage;
// -- 	}
// -- }

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE	16		//	gots to be a power of 2, else change the code in DiminishPaletteTowardsNormal

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD (int dr, int dg, int db)
{
	int	maxVal;

	maxVal = gameData.render.xFlashEffect ? 60 : MAX_PALETTE_ADD;
#if 0
	dMax = maxVal - gameStates.ogl.palAdd.red;
	h = maxVal - gameStates.ogl.palAdd.green;
	if (h < dMax)
		dMax = h;
	h = maxVal - gameStates.ogl.palAdd.blue;
	if (h < dMax)
		dMax = h;
	//the following code will keep the color of the flash effect
	//by limiting the intensity increase of the other color components
	//if one color component hits the ceiling.
	if ((dr > dMax) || (dg > dMax) || (db > dMax))
		dr = dg = db = dMax;
#endif
	gameStates.ogl.palAdd.red += dr;
	gameStates.ogl.palAdd.green += dg;
	gameStates.ogl.palAdd.blue += db;
	CLAMP (gameStates.ogl.palAdd.red, -maxVal, maxVal);
	CLAMP (gameStates.ogl.palAdd.green, -maxVal, maxVal);
	CLAMP (gameStates.ogl.palAdd.blue, -maxVal, maxVal);
}

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.

void DiminishPaletteTowardsNormal (void)
{
	int	dec_amount = 0;

	//	Diminish at DIMINISH_RATE units/second.
	//	For frame rates > DIMINISH_RATE Hz, use randomness to achieve this.
	if (gameData.app.xFrameTime < F1_0/DIMINISH_RATE) {
		if (d_rand () < gameData.app.xFrameTime*DIMINISH_RATE/2)	//	Note: d_rand () is in 0d:\temp\dm_test32767, and 8 Hz means decrement every frame
			dec_amount = 1;
		}
	else {
		dec_amount = f2i (gameData.app.xFrameTime*DIMINISH_RATE);		// one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++;						// make sure we decrement by something
	}

	if (gameData.render.xFlashEffect) {
		int	force_do = 0;

		//	Part of hack system to force update of palette after exiting a menu.
		if (gameData.render.xTimeFlashLastPlayed) {
			force_do = 1;
			gameStates.ogl.palAdd.red ^= 1;	//	Very Tricky!  In GrPaletteStepUp, if all stepups same as last time, won't do anything!
		}

		if ((gameData.render.xTimeFlashLastPlayed + F1_0/8 < gameData.app.xGameTime) || (gameData.render.xTimeFlashLastPlayed > gameData.app.xGameTime)) {
			DigiPlaySample (SOUND_CLOAK_OFF, gameData.render.xFlashEffect/4);
			gameData.render.xTimeFlashLastPlayed = gameData.app.xGameTime;
		}

		gameData.render.xFlashEffect -= gameData.app.xFrameTime;
		if (gameData.render.xFlashEffect < 0)
			gameData.render.xFlashEffect = 0;

		if (force_do || (d_rand () > 4096)) {
      	if ((gameData.demo.nState==ND_STATE_RECORDING) && (gameStates.ogl.palAdd.red || gameStates.ogl.palAdd.green || gameStates.ogl.palAdd.blue))
	      	NDRecordPaletteEffect (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

			GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

			return;
		}

	}

	if (gameStates.ogl.palAdd.red > 0) { 
		gameStates.ogl.palAdd.red -= dec_amount; 
		if (gameStates.ogl.palAdd.red < 0) 
			gameStates.ogl.palAdd.red = 0; 
		}
	else if (gameStates.ogl.palAdd.red < 0) { 
		gameStates.ogl.palAdd.red += dec_amount; 
		if (gameStates.ogl.palAdd.red > 0) 
			gameStates.ogl.palAdd.red = 0; 
		}
	if (gameStates.ogl.palAdd.green > 0) { 
		gameStates.ogl.palAdd.green -= dec_amount; 
		if (gameStates.ogl.palAdd.green < 0) 
			gameStates.ogl.palAdd.green = 0; 
		}
	else if (gameStates.ogl.palAdd.green < 0) { 
		gameStates.ogl.palAdd.green += dec_amount; 
		if (gameStates.ogl.palAdd.green > 0) 
			gameStates.ogl.palAdd.green = 0; 
		}
	if (gameStates.ogl.palAdd.blue > 0) { 
		gameStates.ogl.palAdd.blue -= dec_amount; 
		if (gameStates.ogl.palAdd.blue < 0) 
			gameStates.ogl.palAdd.blue = 0; 
		}
	else if (gameStates.ogl.palAdd.blue < 0) { 
		gameStates.ogl.palAdd.blue += dec_amount; 
		if (gameStates.ogl.palAdd.blue > 0) 
			gameStates.ogl.palAdd.blue = 0; 
		}

	if ((gameData.demo.nState==ND_STATE_RECORDING) && (gameStates.ogl.palAdd.red || gameStates.ogl.palAdd.green || gameStates.ogl.palAdd.blue))
		NDRecordPaletteEffect (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

	GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);

}

//------------------------------------------------------------------------------

tRgbColors palAddSave;

void PaletteSave (void)
{
palAddSave = gameStates.ogl.palAdd;
}

//------------------------------------------------------------------------------

extern void GrPaletteStepUpVR (int r, int g, int b, int white, int black);

void GamePaletteStepUp (int r, int g, int b)
{
if (VR_use_reg_code)
#ifndef WINDOWS
	;//GrPaletteStepUpVR (r, g, b, VR_WHITE_INDEX, VR_BLACK_INDEX);
#endif
else
	GrPaletteStepUp (r, g, b);
}

void PaletteRestore (void)
{
gameStates.ogl.palAdd = palAddSave; 
GamePaletteStepUp (gameStates.ogl.palAdd.red, gameStates.ogl.palAdd.green, gameStates.ogl.palAdd.blue);
//	Forces flash effect to fixup palette next frame.
gameData.render.xTimeFlashLastPlayed = 0;
}

void DeadPlayerFrame (void);
void player_smoke_frame (void);

//	-----------------------------------------------------------------------------

int AllowedToFireLaser (void)
{
	if (gameStates.app.bPlayerIsDead) {
		gameData.app.nGlobalMissileFiringCount = 0;
		return 0;
	}
	if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->segnum].special == SEGMENT_IS_NODAMAGE))
		return 0;
	//	Make sure enough time has elapsed to fire laser, but if it looks like it will
	//	be a long while before laser can be fired, then there must be some mistake!
	if (xNextLaserFireTime > gameData.app.xGameTime)
		if (xNextLaserFireTime < gameData.app.xGameTime + 2*F1_0)
			return 0;

	return 1;
}

//------------------------------------------------------------------------------

fix	Next_flare_fire_time = 0;
#define	FLARE_BIG_DELAY	 (F1_0*2)

int AllowedToFireFlare (void)
{
	if (Next_flare_fire_time > gameData.app.xGameTime)
		if (Next_flare_fire_time < gameData.app.xGameTime + FLARE_BIG_DELAY)	//	In case time is bogus, never wait > 1 second.
			return 0;
	if (gameData.multi.players[gameData.multi.nLocalPlayer].energy >= WI_energy_usage (FLARE_ID))
		Next_flare_fire_time = gameData.app.xGameTime + F1_0/4;
	else
		Next_flare_fire_time = gameData.app.xGameTime + FLARE_BIG_DELAY;

	return 1;
}

//------------------------------------------------------------------------------

int AllowedToFireMissile (void)
{
//	Make sure enough time has elapsed to fire missile, but if it looks like it will
//	be a long while before missile can be fired, then there must be some mistake!
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [gameData.objs.console->segnum].special == SEGMENT_IS_NODAMAGE))
	return 0;
if (xNextMissileFireTime > gameData.app.xGameTime) 
	if (xNextMissileFireTime < gameData.app.xGameTime + 5 * F1_0)
		return 0;
return 1;
}

//------------------------------------------------------------------------------

void FullPaletteSave (void)
{
	PaletteSave ();
	apply_modified_palette ();
	ResetPaletteAdd ();
	GrPaletteStepLoad (NULL);
}

//------------------------------------------------------------------------------

#define ADD_HELP_OPT(_text)	m [opt].type = NM_TYPE_TEXT; m [opt++].text = (_text);

void ShowHelp ()
{
	int nitems, opt = 0;
	newmenu_item m[30];
	#ifdef MACINTOSH
	char command_help[64], pixel_double_help[64], save_help[64], restore_help[64];
	#endif

	memset (m, 0, sizeof (m));
	ADD_HELP_OPT (TXT_HELP_ESC);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_ALT_F2);
	ADD_HELP_OPT (TXT_HELP_ALT_F3);
#else
	sprintf (save_help, TXT_HLP_SAVE, 133);
	sprintf (restore_help, TXT_HLP_LOAD, 133);
	ADD_HELP_OPT (save_help);
	ADD_HELP_OPT (restore_help);
#endif
	ADD_HELP_OPT (TXT_HELP_F2);
	ADD_HELP_OPT (TXT_HELP_F3);
	ADD_HELP_OPT (TXT_HELP_F4);
	ADD_HELP_OPT (TXT_HELP_F5);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_PAUSE);
#else
	ADD_HELP_OPT (TXT_HLP_PAUSE);
#endif
	ADD_HELP_OPT (TXT_HELP_MINUSPLUS);
#ifndef MACINTOSH
	ADD_HELP_OPT (TXT_HELP_PRTSCN);
#else
	ADD_HELP_OPT (TXT_HLP_SCREENIE);
#endif
	ADD_HELP_OPT (TXT_HELP_1TO5);
	ADD_HELP_OPT (TXT_HELP_6TO10);
	ADD_HELP_OPT (TXT_HLP_CYCLE_LEFT_WIN);
	ADD_HELP_OPT (TXT_HLP_CYCLE_RIGHT_WIN);
	ADD_HELP_OPT (TXT_HLP_RESIZE_WIN);
	ADD_HELP_OPT (TXT_HLP_REPOS_WIN);
	ADD_HELP_OPT (TXT_HLP_ZOOM_WIN);
	ADD_HELP_OPT (TXT_HLP_GUIDEBOT);
	ADD_HELP_OPT (TXT_HLP_RENAMEGB);
	ADD_HELP_OPT (TXT_HLP_DROP_PRIM);
	ADD_HELP_OPT (TXT_HLP_DROP_SEC);
	ADD_HELP_OPT (TXT_HLP_CALIBRATE);
	ADD_HELP_OPT (TXT_HLP_GBCMDS);
#ifdef MACINTOSH
	sprintf (pixel_double_help, "%c-D\t  Toggle Pixel Double Mode", 133);
	ADD_HELP_OPT (pixel_double_help);
	ADD_HELP_OPT ("");
	sprintf (command_help, " (Use %c-# for F#. i.e. %c-1 for F1)", 133, 133);
	ADD_HELP_OPT (command_help);
#endif
	nitems = opt;
	FullPaletteSave ();
	ExecMenutiny2 (NULL, TXT_KEYS, nitems, m, NULL);
	PaletteRestore ();
}

//------------------------------------------------------------------------------

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level ();

//deal with rear view - switch it on, or off, or whatever
void CheckRearView ()
{

	#define LEAVE_TIME 0x1000		//how long until we decide key is down	 (Used to be 0x4000)

	static int leave_mode;
	static fix entry_time;
#ifdef _DEBUG
	if (Controls.rear_viewDownCount) {		//key/button has gone down
#else
	if (Controls.rear_viewDownCount && !gameStates.render.bExternalView) {		//key/button has gone down
#endif
		if (gameStates.render.bRearView) {
			gameStates.render.bRearView = 0;
			if (gameStates.render.cockpit.nMode==CM_REAR_VIEW) {
				SelectCockpit (gameStates.render.cockpit.nModeSave);
				gameStates.render.cockpit.nModeSave = -1;
			}
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordRestoreRearView ();
		}
		else {
			gameStates.render.bRearView = 1;
			leave_mode = 0;		//means wait for another key
			entry_time = TimerGetFixedSeconds ();
			if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
				gameStates.render.cockpit.nModeSave = gameStates.render.cockpit.nMode;
				SelectCockpit (CM_REAR_VIEW);
			}
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordRearView ();
		}
	}
	else
		if (Controls.rear_view_down_state) {

			if (leave_mode==0 && (TimerGetFixedSeconds ()-entry_time)>LEAVE_TIME)
				leave_mode = 1;
		}
		else {

			//@@if (leave_mode==1 && gameStates.render.cockpit.nMode==CM_REAR_VIEW) {

			if (leave_mode==1 && gameStates.render.bRearView) {
				gameStates.render.bRearView = 0;
				if (gameStates.render.cockpit.nMode==CM_REAR_VIEW) {
					SelectCockpit (gameStates.render.cockpit.nModeSave);
					gameStates.render.cockpit.nModeSave = -1;
				}
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordRestoreRearView ();
			}
		}
}

//------------------------------------------------------------------------------

void ResetRearView (void)
{
	if (gameStates.render.bRearView) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordRestoreRearView ();
	}

	gameStates.render.bRearView = 0;

	if ((gameStates.render.cockpit.nMode < 0) || (gameStates.render.cockpit.nMode > 4)) {
		if (!(gameStates.render.cockpit.nModeSave == CM_FULL_COCKPIT || gameStates.render.cockpit.nModeSave == CM_STATUS_BAR || gameStates.render.cockpit.nModeSave == CM_FULL_SCREEN))
			gameStates.render.cockpit.nModeSave = CM_FULL_COCKPIT;
		SelectCockpit (gameStates.render.cockpit.nModeSave);
		gameStates.render.cockpit.nModeSave	= -1;
	}

}

//------------------------------------------------------------------------------

jmp_buf gameExitPoint;

void DoLunacyOn ();
void DoLunacyOff ();

extern char OldHomingState[20];
extern char old_IntMethod;

//turns off active cheats
void TurnCheatsOff ()
{
	int i;

	if (gameStates.app.cheats.bHomingWeapons)
		for (i=0;i<20;i++)
			WI_set_homing_flag (i, OldHomingState[i]);

	if (gameStates.app.cheats.bAcid)
	{
		gameStates.app.cheats.bAcid=0;
		gameStates.render.nInterpolationMethod=old_IntMethod;
	}

	gameStates.app.cheats.bMadBuddy = 0;
	gameStates.app.cheats.bBouncingWeapons=0;
   gameStates.app.cheats.bHomingWeapons=0;
	DoLunacyOff ();
	gameStates.app.cheats.bLaserRapidFire = 0;
	gameStates.app.cheats.bPhysics = 0;
	gameStates.app.cheats.bMonsterMode = 0;
	gameStates.app.cheats.bRobotsKillRobots=0;
	gameStates.app.cheats.bRobotsFiring = 1;
}

//turns off all cheats & resets cheater flag
void GameDisableCheats ()
{
	TurnCheatsOff ();
	gameStates.app.cheats.bEnabled=0;
}


//	GameSetup ()
// ----------------------------------------------------------------------------

void GameSetup (void)
{
	//@@int demo_playing=0;
	//@@int multi_game=0;
	DoLunacyOn ();		//	Copy values for insane into copy buffer in ai.c
	DoLunacyOff ();		//	Restore true insane mode.
	gameStates.app.bGameAborted = 0;
	last_drawn_cockpit[0] = -1;				// Force cockpit to redraw next time a frame renders.
	last_drawn_cockpit[1] = -1;				// Force cockpit to redraw next time a frame renders.
	gameStates.app.bEndLevelSequence = 0;

	//@@if (gameData.demo.nState == ND_STATE_PLAYBACK)
	//@@	demo_playing = 1;
	//@@if (gameData.app.nGameMode & GM_MULTI)
	//@@	multi_game = 1;
	GrPaletteStepLoad (NULL);
	SetScreenMode (SCREEN_GAME);
	ResetPaletteAdd ();
	SetWarnFunc (ShowInGameWarning);
#if TRACE
	//con_printf (CON_DEBUG, "   InitCockpit d:\temp\dm_test.\n");
#endif
	InitCockpit ();
#if TRACE
	//con_printf (CON_DEBUG, "   InitGauges d:\temp\dm_test.\n");
#endif
	InitGauges ();
	//DigiInitSounds ();
	//keyd_repeat = 0;                // Don't allow repeat in game
	keyd_repeat = 1;                // Do allow repeat in game
#if !defined (WINDOWS) && !defined (MACINTOSH)
	//_MARK_ ("start of game");
#endif
	#ifdef EDITOR
		if (gameData.segs.segments[gameData.objs.console->segnum].segnum == -1)      //segment no longer exists
			RelinkObject (OBJ_IDX (gameData.objs.console), SEG_PTR_2_NUM (Cursegp));

		if (!check_obj_seg (gameData.objs.console))
			MovePlayerToSegment (Cursegp,Curside);
	#endif
	gameData.objs.viewer = gameData.objs.console;
#if TRACE
	//con_printf (CON_DEBUG, "   FlyInit d:\temp\dm_test.\n");
#endif
	FlyInit (gameData.objs.console);
	gameStates.app.bGameSuspended = 0;
	ResetTime ();
	gameData.app.xFrameTime = 0;			//make first frame zero
	#ifdef EDITOR
	if (gameData.missions.nCurrentLevel == 0) {			//not a real level
		InitPlayerStatsGame ();
		InitAIObjects ();
	}
	#endif
#if TRACE
	//con_printf (CON_DEBUG, "   FixObjectSegs d:\temp\dm_test.\n");
#endif
	FixObjectSegs ();
	GameFlushInputs ();
}


//------------------------------------------------------------------------------

void SetFunctionMode (int newFuncMode)
{
if (gameStates.app.nFunctionMode != newFuncMode) {
	gameStates.app.nLastFuncMode = gameStates.app.nFunctionMode;
	if ((gameStates.app.nFunctionMode = newFuncMode) == FMODE_GAME)
		gameStates.app.bEnterGame = 2;
#ifdef _DEBUG
	else if (newFuncMode == FMODE_MENU)
		gameStates.app.bEnterGame = 0;
#endif
	}
}


//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void game ()
{
GameSetup ();								// Replaces what was here earlier.
#ifdef MWPROFILE
ProfilerSetStatus (1);
#endif

if (setjmp (gameExitPoint)==0)	{
while (1) {
	int player_shields;
		// GAME LOOP!
	gameStates.app.bAutoMap = 0;
	gameStates.app.bConfigMenu = 0;
	if (gameData.objs.console != &gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].objnum]) {
#if TRACE
	    //con_printf (CON_DEBUG,"gameData.multi.nLocalPlayer=%d objnum=%d",gameData.multi.nLocalPlayer,gameData.multi.players[gameData.multi.nLocalPlayer].objnum);
#endif
	    //Assert (gameData.objs.console == &gameData.objs.objects[gameData.multi.players[gameData.multi.nLocalPlayer].objnum]);
		}
	player_shields = gameData.multi.players[gameData.multi.nLocalPlayer].shields;
#ifdef WINDOWS
	{
	MSG msg;
	DoMessageStuff (&msg);		// Do Windows event handling.
	if (_RedrawScreen) {
		_RedrawScreen = FALSE;
		LoadPalette (szCurrentLevelPalette,1,1,0);
		GrPaletteStepLoad (NULL);
		}
	}
#endif
	gameStates.app.nExtGameStatus=GAMESTAT_RUNNING;
	if (!GameLoop (1, 1))		// Do game loop with rendering and reading controls.
		continue;
	//if the player is taking damage, give up guided missile control
	if (gameData.multi.players[gameData.multi.nLocalPlayer].shields != player_shields)
		ReleaseGuidedMissile (gameData.multi.nLocalPlayer);
	//see if redbook song needs to be restarted
	songs_check_redbook_repeat ();	// Handle RedBook Audio Repeating.
	if (gameStates.app.bConfigMenu) {
		int double_save = Scanline_double;
	//WIN (mouse_set_mode (0);
		if (!(gameData.app.nGameMode&GM_MULTI)) {
			PaletteSave (); 
			ResetPaletteAdd ();	
			apply_modified_palette (); 
			GrPaletteStepLoad (NULL); 
			}
		OptionsMenu ();
		if (Scanline_double != double_save)	
			InitCockpit ();
		if (!(gameData.app.nGameMode&GM_MULTI)) 
			PaletteRestore ();
		//WIN (mouse_set_mode (1);
		}
	if (gameStates.app.bAutoMap) {
		int save_w=Game_window_w,save_h=Game_window_h;
		DoAutomap (0, 0);
		gameStates.app.bEnterGame = 1;
		//	FlushInput ();
		//	StopPlayerMovement ();
		gameStates.video.nScreenMode=-1; 
		SetScreenMode (SCREEN_GAME);
		Game_window_w=save_w; 
		Game_window_h=save_h;
		InitCockpit ();
		last_drawn_cockpit[0] = -1;
		last_drawn_cockpit[1] = -1;
		}
	if ((gameStates.app.nFunctionMode != FMODE_GAME) && gameData.demo.bAuto && (gameData.demo.nState != ND_STATE_NORMAL))	{
		int choice, fmode;
		fmode = gameStates.app.nFunctionMode;
		SetFunctionMode (FMODE_GAME);
		PaletteSave ();
		apply_modified_palette ();
		ResetPaletteAdd ();
		GrPaletteStepLoad (NULL);
		choice=ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO);
		PaletteRestore ();
		SetFunctionMode (fmode);
		if (choice)
			SetFunctionMode (FMODE_GAME);
		else {
			gameData.demo.bAuto = 0;
			NDStopPlayback ();
			SetFunctionMode (FMODE_MENU);
			} 
		}
	if ((gameStates.app.nFunctionMode != FMODE_GAME) &&  
		 (gameData.demo.nState != ND_STATE_PLAYBACK) &&  
		 (gameStates.app.nFunctionMode!=FMODE_EDITOR)
	#ifdef NETWORK
			&& !gameStates.multi.bIWasKicked
	#endif
			) {
			int choice, fmode = gameStates.app.nFunctionMode;
			SetFunctionMode (FMODE_GAME);
			PaletteSave ();
			apply_modified_palette ();
			ResetPaletteAdd ();
			GrPaletteStepLoad (NULL);
			gameStates.app.nExtGameStatus=GAMESTAT_ABORT_GAME;
			choice=ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME);
			PaletteRestore ();
			SetFunctionMode (fmode);
			if (choice)
				SetFunctionMode (FMODE_GAME);
			}
#ifdef NETWORK
	gameStates.multi.bIWasKicked = 0;
#endif
	if (gameStates.app.nFunctionMode != FMODE_GAME)
		longjmp (gameExitPoint,0);
#ifdef APPLE_DEMO
		if ((keyd_time_when_last_pressed + (F1_0 * 60)) < TimerGetFixedSeconds ())		// idle in game for 1 minutes means exit
		longjmp (gameExitPoint,0);
#endif
		}
	}
#ifdef MWPROFILE
ProfilerSetStatus (0);
#endif
DigiStopAll ();
if (gameOpts->sound.bD1Sound) {
	gameOpts->sound.bD1Sound = 0;
	//DigiClose ();
	//DigiInit ();
	}
if ((gameData.demo.nState == ND_STATE_RECORDING) || (gameData.demo.nState == ND_STATE_PAUSED))
	NDStopRecording ();
#ifdef NETWORK
MultiLeaveGame ();
#endif
if (gameData.demo.nState == ND_STATE_PLAYBACK)
	NDStopPlayback ();
if (gameStates.render.cockpit.nModeSave!=-1) {
	gameStates.render.cockpit.nMode=gameStates.render.cockpit.nModeSave;
	gameStates.render.cockpit.nModeSave=-1;
	}
if (gameStates.app.nFunctionMode != FMODE_EDITOR)
	GrPaletteFadeOut (NULL,32,0);			// Fade out before going to menu
//@@	if ((!demo_playing) && (!multi_game) && (gameStates.app.nFunctionMode != FMODE_EDITOR))	{
//@@		scores_maybe_add_player (gameStates.app.bGameAborted);
//@@	}
#if !defined (WINDOWS) && !defined (MACINTOSH)
//_MARK_ ("end of game");
#endif
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
GameDisableCheats ();
UnloadCamBot ();
#ifdef APPLE_DEMO
SetFunctionMode (FMODE_EXIT);		// get out of game in Apple OEM version
#endif
}

// ----------------------------------------------------------------------------

void FreeHiresModels (void)
{
	int	i;

for (i = 0; i < gameData.models.nHiresModels; i++)
	OOF_FreeObject (gameData.models.hiresModels + i);
}

//-----------------------------------------------------------------------------
//called at the end of the program
void _CDECL_ CloseGame (void)
{
	static	int bGameClosed = 0;
	
if (bGameClosed)
	return;
bGameClosed = 1;
BMFreeExtraObjBitmaps ();
BMFreeExtraModels ();
LogErr ("unloading hires animations\n");
PiggyFreeHiresAnimations ();
PiggyBitmapPageOutAll (0);
LogErr ("unloading hoard data\n");
FreeHoardData ();
LogErr ("unloading hires models\n");
FreeHiresModels ();
LogErr ("unloading tracker list\n");
DestroyTrackerList ();
LogErr ("unloading lightmap data\n");
DestroyLightMaps ();
LogErr ("unloading particle data\n");
DestroyAllSmoke ();
LogErr ("unloading shield sphere data\n");
DestroySphere ();
LogErr ("unloading inventory icons\n");
FreeInventoryIcons ();
LogErr ("unloading palettes\n");
FreePalettes ();
#ifdef WINDOWS
if (dd_VR_offscreen_buffer) {
	if (dd_grd_backcanv != dd_VR_offscreen_buffer) {
		dd_gr_free_canvas (dd_VR_offscreen_buffer);
	}
	dd_VR_offscreen_buffer = NULL;
	VR_offscreen_buffer = NULL;
}
#else
if (VR_offscreen_buffer)	{
	GrFreeCanvas (VR_offscreen_buffer);
	VR_offscreen_buffer = NULL;
}
#endif
LogErr ("unloading gauge data\n");
CloseGaugeCanvases ();
LogErr ("restoring effect bitmaps\n");
RestoreEffectBitmapIcons ();
#if !defined (MACINTOSH) && !defined (WINDOWS)
if (Game_cockpit_copy_code) {
	d_free (Game_cockpit_copy_code);
	Game_cockpit_copy_code = NULL;
}
#else
if (Game_cockpit_copy_code)
	Game_cockpit_copy_code = 0;
#endif
if (bmBackground.bm_texBuf) {
	LogErr ("unloading background bitmap\n");
	d_free (bmBackground.bm_texBuf);
	}
ClearWarnFunc (ShowInGameWarning);     //don't use this func anymore
LogErr ("unloading custom background data\n");
NMFreeAltBg (1);
SaveBanList ();
FreeBanList ();
#if 0
if (fErr) {
	fclose (fErr);
	fErr = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef WINDOWS
dd_grs_canvas * GetCurrentGameScreen ()
{
	return &dd_VR_screen_pages[VR_current_page];
}

#else

grs_canvas * GetCurrentGameScreen ()
{
	return &VR_screen_pages[VR_current_page];
}
#endif

//-----------------------------------------------------------------------------

extern void kconfig_center_headset ();

#ifndef	NDEBUG
void speedtest_frame (void);
Uint32 Debug_slowdown=0;
#endif

#ifdef EDITOR
extern void player_follow_path (object *objP);
extern void check_create_player_path (void);

#endif

int Marker_viewer_num[2]={-1,-1};
int Coop_view_player[2]={-1,-1};
int Cockpit_3d_view[2]={CV_NONE,CV_NONE};

//returns ptr to escort robot, or NULL
object *find_escort ()
{
	int i;

	for (i=0; i<=gameData.objs.nLastObject; i++)
		if (gameData.objs.objects[i].type == OBJ_ROBOT)
			if (gameData.bots.pInfo[gameData.objs.objects[i].id].companion)
				return &gameData.objs.objects[i];

	return NULL;
}

//-----------------------------------------------------------------------------

extern void ProcessSmartMinesFrame (void);
extern void DoSeismicStuff (void);

#ifndef RELEASE
int Saving_movie_frames=0;
int __Movie_frame_num=0;

#define MAX_MOVIE_BUFFER_FRAMES 250
#define MOVIE_FRAME_SIZE	 (320 * 200)

ubyte *Movie_frame_buffer;
int nMovieFrames;
ubyte Movie_pal[768];
char movie_path[50] = ".\\";

grs_bitmap bmMovie;

void flush_movie_buffer ()
{
	char savename[128];
	int f;

	StopTime ();
#if TRACE
	//con_printf (CON_DEBUG,"Flushing movie bufferd:\temp\dm_test.");
#endif
	bmMovie.bm_texBuf = Movie_frame_buffer;

	for (f=0;f<nMovieFrames;f++) {
		sprintf (savename, "%sfrm%04d.pcx",movie_path,__Movie_frame_num);
		__Movie_frame_num++;
		pcx_write_bitmap (savename, &bmMovie);
		bmMovie.bm_texBuf += MOVIE_FRAME_SIZE;

		if (f % 5 == 0) {
#if TRACE
			//con_printf (CON_DEBUG,"%3d/%3d\10\10\10\10\10\10\10",f,nMovieFrames);
#endif
		}			
	}

	nMovieFrames=0;
#if TRACE
	//con_printf (CON_DEBUG,"done   \n");
#endif
	StartTime ();
}

//-----------------------------------------------------------------------------

void toggle_movie_saving ()
{
	int exit;

	Saving_movie_frames = !Saving_movie_frames;

	if (Saving_movie_frames) {
		newmenu_item m[1];

		memset (m, 0, sizeof (m));
		m[0].type=NM_TYPE_INPUT; 
		m[0].text_len = 50; 
		m[0].text = movie_path;
		exit = ExecMenu (NULL, "Directory for movie frames?" , 1, & (m[0]), NULL, NULL);

		if (exit==-1) {
			Saving_movie_frames = 0;
			return;
		}

		while (isspace (movie_path[strlen (movie_path)-1]))
			movie_path[strlen (movie_path)-1] = 0;
		if (movie_path[strlen (movie_path)-1]!='\\' && movie_path[strlen (movie_path)-1]!=':')
			strcat (movie_path,"\\");


		if (!Movie_frame_buffer) {
			Movie_frame_buffer = d_malloc (MAX_MOVIE_BUFFER_FRAMES * MOVIE_FRAME_SIZE);
			if (!Movie_frame_buffer) {
				Int3 ();
				Saving_movie_frames=0;
			}

			nMovieFrames=0;

			bmMovie.bm_props.x = bmMovie.bm_props.y = 0;
			bmMovie.bm_props.w = 320;
			bmMovie.bm_props.h = 200;
			bmMovie.bm_props.type = BM_LINEAR;
			bmMovie.bm_props.flags = 0;
			bmMovie.bm_props.rowsize = 320;
			//bmMovie.bm_handle = 0;

			GrPaletteRead (Movie_pal);		//get actual palette from the hardware

			if (gameData.demo.nState == ND_STATE_PLAYBACK)
				gameData.demo.bInterpolate = 0;
		}
	}
	else {
		flush_movie_buffer ();

		if (gameData.demo.nState == ND_STATE_PLAYBACK)
			gameData.demo.bInterpolate = 1;
	}

}

//-----------------------------------------------------------------------------

void save_movie_frame ()
{
	memcpy (Movie_frame_buffer+nMovieFrames*MOVIE_FRAME_SIZE,grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf,MOVIE_FRAME_SIZE);

	nMovieFrames++;

	if (nMovieFrames == MAX_MOVIE_BUFFER_FRAMES)
		flush_movie_buffer ();

}

#endif

//-----------------------------------------------------------------------------
//if water or fire level, make occasional sound
void DoAmbientSounds ()
{
	int has_water,has_lava;
	short sound;

	has_lava = (gameData.segs.segment2s[gameData.objs.console->segnum].s2_flags & S2F_AMBIENT_LAVA);
	has_water = (gameData.segs.segment2s[gameData.objs.console->segnum].s2_flags & S2F_AMBIENT_WATER);

	if (has_lava) {							//has lava
		sound = SOUND_AMBIENT_LAVA;
		if (has_water && (d_rand () & 1))	//both, pick one
			sound = SOUND_AMBIENT_WATER;
	}
	else if (has_water)						//just water
		sound = SOUND_AMBIENT_WATER;
	else
		return;

	if (((d_rand () << 3) < gameData.app.xFrameTime)) {						//play the sound
		fix volume = d_rand () + f1_0/2;
		DigiPlaySample (sound, volume);
	}
}

//-----------------------------------------------------------------------------

void GetSlowTick (void)
{
gameStates.app.nSDLTicks = SDL_GetTicks ();
gameStates.app.nDeltaTime = gameStates.app.nSDLTicks - gameStates.app.nLastTick;
if (gameStates.app.b40fpsTick = (gameStates.app.nDeltaTime >= 25))
	gameStates.app.nLastTick = gameStates.app.nSDLTicks;
}

//-----------------------------------------------------------------------------
// -- extern void lightning_frame (void);

void GameRenderFrame ();
extern void OmegaChargeFrame (void);

extern time_t t_current_time, t_saved_time;
extern int flFrameTime;

void FlickerLights ();

int GameLoop (int RenderFlag, int ReadControlsFlag)
{
gameStates.app.bGameRunning = 1;
GetSlowTick ();
#ifndef	NDEBUG
//	Used to slow down frame rate for testing things.
//	RenderFlag = 1; // DEBUG
//	GrPaletteStepLoad (gamePalette);
if (Debug_slowdown) {
#if 1
	time_t	t = SDL_GetTicks ();
	while (SDL_GetTicks () - t < Debug_slowdown)
		;
#else
	int	h, i, j=0;

	for (h=0; h<Debug_slowdown; h++)
		for (i=0; i<1000; i++)
			j += i;
#endif
}
#endif

#ifdef WINDOWS
{
	static int desc_dead_countdown=100;   /*  used if player shouldn't be playing */

	if (desc_id_exit_num) {				 // are we supposed to be checking
		if (!(--desc_dead_countdown)) {// if so, at zero, then pull the plug
			char time_str[32], time_str2[32];

			_ctime (&t_saved_time, time_str);
			_ctime (&t_current_time, time_str2);

			Error ("EXPIRES %s.  YOUR TIME %s.\n", time_str, time_str2);
			Error ("Loading overlay -- error number: %d\n", (int)desc_id_exit_num);
		}
	}
}
#endif

#ifndef RELEASE
	if (FindArg ("-invulnerability")) {
		gameData.multi.players[gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
		SetSpherePulse (gameData.multi.spherePulse + gameData.multi.nLocalPlayer, 0.02f, 0.5f);
		}
#endif
	if (!MultiProtectGame ()) {
		SetFunctionMode (FMODE_MENU);
		return 1;
		}
	AutoBalanceTeams ();
	MultiSendTyping ();
	MultiSendWeapons (0);
	//MultiSendMonsterball (0, 0);
	UpdatePlayerStats ();
	DiminishPaletteTowardsNormal ();		//	Should leave palette effect up for as long as possible by putting right before render.
//LogErr ("DoAfterburnerStuff\n");
	DoAfterburnerStuff ();
//LogErr ("DoCloakStuff\n");
	DoCloakStuff ();
//LogErr ("DoInvulnerableStuff\n");
	DoInvulnerableStuff ();
//LogErr ("DoInvulnerableStuff \n");
	RemoveObsoleteStuckObjects ();
//LogErr ("RemoveObsoleteStuckObjects \n");
	InitAIFrame ();
//LogErr ("InitAIFrame \n");
	DoFinalBossFrame ();
// -- lightning_frame ();
	// -- recharge_energy_frame ();

	if ((gameData.multi.players[gameData.multi.nLocalPlayer].flags & (PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON)) == (PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON)) {
		static int turned_off=0;
		gameData.multi.players[gameData.multi.nLocalPlayer].energy -= (gameData.app.xFrameTime*3/8);
		if (gameData.multi.players[gameData.multi.nLocalPlayer].energy < i2f (10)) {
			if (!turned_off) {
				gameData.multi.players[gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
#ifdef NETWORK
				if (gameData.app.nGameMode & GM_MULTI)
					MultiSendFlags ((char) gameData.multi.nLocalPlayer);
#endif
			}
		}
		else
			turned_off = 0;

		if (gameData.multi.players[gameData.multi.nLocalPlayer].energy <= 0) {
			gameData.multi.players[gameData.multi.nLocalPlayer].energy = 0;
			gameData.multi.players[gameData.multi.nLocalPlayer].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#ifdef NETWORK
			if (gameData.app.nGameMode & GM_MULTI) {
//con_printf (CON_DEBUG, "      MultiSendFlags\n");
				MultiSendFlags ((char) gameData.multi.nLocalPlayer);
				}
#endif
		}
	}

#ifdef EDITOR
	check_create_player_path ();
	player_follow_path (gameData.objs.console);
#endif

#ifdef NETWORK
	if (gameData.app.nGameMode & GM_MULTI) {
		AddServerToTracker ();
      MultiDoFrame ();
		CheckMonsterballScore ();
		if (netGame.PlayTimeAllowed && ThisLevelTime>=i2f ((netGame.PlayTimeAllowed*5*60)))
          MultiCheckForKillGoalWinner ();
		else 
			MultiCheckForEntropyWinner ();
     }
#endif

	if (RenderFlag) {
		if (gameStates.render.cockpit.bRedraw) {			//screen need redrawing?
			InitCockpit ();
			gameStates.render.cockpit.bRedraw=0;
		}
//LogErr ("GameRenderFrame\n");
		GameRenderFrame ();
		//show_extra_views ();		//missile view, buddy bot, etc.

#ifndef RELEASE
		if (Saving_movie_frames)
			save_movie_frame ();
#endif

	}
	CalcFrameTime ();
	DeadPlayerFrame ();
	if (gameData.demo.nState != ND_STATE_PLAYBACK) {
		DoReactorDeadFrame ();
		}	
//LogErr ("ProcessSmartMinesFrame \n");
	ProcessSmartMinesFrame ();
//LogErr ("DoSeismicStuff\n");
	DoSeismicStuff ();
//LogErr ("DoSmokeFrame \n");
	DoSmokeFrame ();
//LogErr ("DoAmbientSounds\n");
	DoAmbientSounds ();
#ifndef NDEBUG
	if (Speedtest_on)
		speedtest_frame ();
#endif
	if (ReadControlsFlag) {
//LogErr ("ReadControls\n");
		ReadControls ();
		}
	else
		memset (&Controls, 0, sizeof (Controls));
//LogErr ("DropPowerups\n");
	DropPowerups ();
	gameData.app.xGameTime += gameData.app.xFrameTime;
	if (f2i (gameData.app.xGameTime)/10 != f2i (gameData.app.xGameTime-gameData.app.xFrameTime)/10) {
		}
	if (gameData.app.xGameTime < 0 || gameData.app.xGameTime > i2f (0x7fff - 600)) {
		gameData.app.xGameTime = gameData.app.xFrameTime;	//wrap when goes negative, or gets within 10 minutes
	}

#ifndef NDEBUG
	if (FindArg ("-checktime") != 0)
		if (gameData.app.xGameTime >= i2f (600))		//wrap after 10 minutes
			gameData.app.xGameTime = gameData.app.xFrameTime;
#endif

#ifdef NETWORK
   if ((gameData.app.nGameMode & GM_MULTI) && netGame.PlayTimeAllowed)
       ThisLevelTime +=gameData.app.xFrameTime;
#endif
//LogErr ("DigiSyncSounds\n");
	DigiSyncSounds ();
	if (gameStates.app.bEndLevelSequence) {
		DoEndLevelFrame ();
		PowerupGrabCheatAll ();
		DoSpecialEffects ();
		return 1;					//skip everything else
	}

	if (gameData.demo.nState != ND_STATE_PLAYBACK) {
//LogErr ("DoExplodingWallFrame\n");
		DoExplodingWallFrame ();
		}
	if ((gameData.demo.nState != ND_STATE_PLAYBACK) || (gameData.demo.nVcrState != ND_STATE_PAUSED)) {
//LogErr ("DoSpecialEffects\n");
		DoSpecialEffects ();
//LogErr ("WallFrameProcess\n");
		WallFrameProcess ();
//LogErr ("TriggersFrameProcess\n");
		TriggersFrameProcess ();
	}
	if (gameData.reactor.bDestroyed && (gameData.demo.nState==ND_STATE_RECORDING)) {
			NDRecordControlCenterDestroyed ();
	}
//LogErr ("FlashFrame\n");
	FlashFrame ();
	if (gameData.demo.nState == ND_STATE_PLAYBACK)	{
		NDPlayBackOneFrame ();
		if (gameData.demo.nState != ND_STATE_PLAYBACK)		{
			longjmp (gameExitPoint, 0);		// Go back to menu
		}
	} else { // Note the link to above!
		gameData.multi.players[gameData.multi.nLocalPlayer].homing_object_dist = -1;		//	Assume not being tracked.  LaserDoWeaponSequence modifies this.
//LogErr ("MoveAllObjects\n");
		if (!MoveAllObjects ())
			return 0;
//LogErr ("PowerupGrabCheatAll \n");
		PowerupGrabCheatAll ();
		if (gameStates.app.bEndLevelSequence)	//might have been started during move
			return 1;
//LogErr ("FuelcenUpdateAll\n");
		FuelcenUpdateAll ();
//LogErr ("DoAIFrameAll\n");
		DoAIFrameAll ();
		if (AllowedToFireLaser ()) {
//LogErr ("FireLaser\n");
			FireLaser ();				// Fire Laser!
			}
		if (gameData.app.fusion.xAutoFireTime) {
			if (gameData.weapons.nPrimary != FUSION_INDEX)
				gameData.app.fusion.xAutoFireTime = 0;
			else if (gameData.app.xGameTime + flFrameTime/2 >= gameData.app.fusion.xAutoFireTime) {
				gameData.app.fusion.xAutoFireTime = 0;
				gameData.app.nGlobalLaserFiringCount = 1;
			} else {
				vms_vector	rand_vec;
				fix			bump_amount;
				static time_t t0 = 0;
				time_t t = SDL_GetTicks ();
				if (t - t0 < 30)
					return 1;
				t0 = t;
				gameData.app.nGlobalLaserFiringCount = 0;
				gameData.objs.console->mtype.phys_info.rotvel.x += (d_rand () - 16384)/8;
				gameData.objs.console->mtype.phys_info.rotvel.z += (d_rand () - 16384)/8;
				make_random_vector (&rand_vec);
				bump_amount = F1_0*4;
				if (gameData.app.fusion.xCharge > F1_0*2)
					bump_amount = gameData.app.fusion.xCharge*4;
				BumpOneObject (gameData.objs.console, &rand_vec, bump_amount);
			}
		}
		if (gameData.app.nGlobalLaserFiringCount) {
			//	Don't cap here, gets capped in CreateNewLaser and is based on whether in multiplayer mode, MK, 3/27/95
			// if (gameData.app.fusion.xCharge > F1_0*2)
			// 	gameData.app.fusion.xCharge = F1_0*2;
			gameData.app.nGlobalLaserFiringCount -= LaserFireLocalPlayer ();	//LaserFireObject (gameData.multi.players[gameData.multi.nLocalPlayer].objnum, gameData.weapons.nPrimary);
		}
		if (gameData.app.nGlobalLaserFiringCount < 0)
			gameData.app.nGlobalLaserFiringCount = 0;
	}
if (gameStates.render.bDoAppearanceEffect) {
	CreatePlayerAppearanceEffect (gameData.objs.console);
	gameStates.render.bDoAppearanceEffect = 0;
#ifdef NETWORK
	if ((gameData.app.nGameMode & GM_MULTI) && netGame.invul) {
		gameData.multi.players[gameData.multi.nLocalPlayer].flags |= PLAYER_FLAGS_INVULNERABLE;
		gameData.multi.players[gameData.multi.nLocalPlayer].invulnerable_time = gameData.app.xGameTime-i2f (27);
		FakingInvul=1;
		SetSpherePulse (gameData.multi.spherePulse + gameData.multi.nLocalPlayer, 0.02f, 0.5f);
	}
#endif

}
//LogErr ("OmegaChargeFrame \n");
OmegaChargeFrame ();
//LogErr ("SlideTextures \n");
SlideTextures ();
//LogErr ("FlickerLights \n");
FlickerLights ();
//LogErr ("\n");
return 1;
}

//-----------------------------------------------------------------------------
//!!extern int Goal_blue_segnum,Goal_red_segnum;
//!!extern int Hoard_goal_eclip;
//!!
//!!//do cool pulsing lights in hoard goals
//!!hoard_light_pulse ()
//!!{
//!!	if (gameData.app.nGameMode & GM_HOARD) {
//!!		fix light;
//!!		int frame;
//!!
//!!		frame = gameData.eff.pEffects[Hoard_goal_eclip]..nCurFrame;
//!!
//!!		frame++;
//!!
//!!		if (frame >= gameData.eff.pEffects[Hoard_goal_eclip].vc.nFrameCount)
//!!			frame = 0;
//!!
//!!		light = abs (frame - 5) * f1_0 / 5;
//!!
//!!		gameData.segs.segment2s[Goal_red_segnum].static_light = gameData.segs.segment2s[Goal_blue_segnum].static_light = light;
//!!	}
//!!}

void ComputeSlideSegs (void)
{
	int	segnum, sidenum, bIsSlideSeg, tmn;

gameData.segs.nSlideSegs = 0;
for (segnum = 0; segnum <= gameData.segs.nLastSegment; segnum++) {
	bIsSlideSeg = 0;
	for (sidenum = 0; sidenum < 6; sidenum++) {
		tmn = gameData.segs.segments [segnum].sides [sidenum].tmap_num;
		if (gameData.pig.tex.pTMapInfo [tmn].slide_u  || 
			 gameData.pig.tex.pTMapInfo [tmn].slide_v) {
			if (!bIsSlideSeg) {
				bIsSlideSeg = 1;
				gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSegment = segnum;
				gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSides = 0;
				}	
			gameData.segs.slideSegs [gameData.segs.nSlideSegs].nSides |= (1 << sidenum);
			}
		}
	if (bIsSlideSeg)
		gameData.segs.nSlideSegs++;
	}	
gameData.segs.bHaveSlideSegs = 1;
}

//	-----------------------------------------------------------------------------

void SlideTextures (void)
{
	int	segnum, sidenum, h, i, j, tmn;
	ubyte	sides;
	side	*sideP;
	uvl	*uvlP;
	fix	slideU, slideV;

if (!gameData.segs.bHaveSlideSegs)
	ComputeSlideSegs ();
for (h = 0; h < gameData.segs.nSlideSegs; h++) {
	segnum = gameData.segs.slideSegs [h].nSegment;
	sides = gameData.segs.slideSegs [h].nSides;
	for (sidenum = 0, sideP = gameData.segs.segments [segnum].sides; sidenum < 6; sidenum++, sideP++) {
		if (!(sides & (1 << sidenum)))
			continue;
		tmn = sideP->tmap_num;
		slideU = (fix) gameData.pig.tex.pTMapInfo [tmn].slide_u;
		slideV = (fix) gameData.pig.tex.pTMapInfo [tmn].slide_v;
		if (!(slideU || slideV))
			continue;
		slideU = fixmul (gameData.app.xFrameTime, slideU << 8);
		slideV = fixmul (gameData.app.xFrameTime, slideV << 8);
		for (i = 0, uvlP = sideP->uvls; i < 4; i++) {
			uvlP [i].u += slideU;
			if (uvlP [i].u > f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].u -= f1_0;
				}
			else if (uvlP [i].u < -f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].u += f1_0;
				}
			uvlP [i].v += slideV;
			if (uvlP [i].v > f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].v -= f1_0;
				}
			else if (uvlP [i].v < -f2_0) {
				for (j = 0; j < 4; j++)
					uvlP [j].v += f1_0;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------

flickering_light Flickering_lights[MAX_FLICKERING_LIGHTS];

int Num_flickering_lights=0;

void FlickerLights ()
{
	int l;
	flickering_light *f;

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++) {
		segment *segp = &gameData.segs.segments[f->segnum];

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY (segp, f->sidenum, NULL) & WID_RENDER_FLAG))
			continue;
		if (! (gameData.pig.tex.pTMapInfo[segp->sides[f->sidenum].tmap_num].lighting | gameData.pig.tex.pTMapInfo[segp->sides[f->sidenum].tmap_num2 & 0x3fff].lighting))
			continue;

		if (f->timer == 0x80000000)		//disabled
			continue;

		if ((f->timer -= gameData.app.xFrameTime) < 0) {

			while (f->timer < 0)
				f->timer += f->delay;

			f->mask = ((f->mask&0x80000000)?1:0) + (f->mask<<1);

			if (f->mask & 1)
				add_light (f->segnum,f->sidenum);
			else
				subtract_light (f->segnum,f->sidenum);
		}
	}
}

//-----------------------------------------------------------------------------
//returns ptr to flickering light structure, or NULL if can't find
flickering_light *find_flicker (int segnum,int sidenum)
{
	int l;
	flickering_light *f;

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			return f;

	return NULL;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned off)
void disable_flicker (int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker (segnum,sidenum)) != NULL)
		f->timer = 0x80000000;
}

//-----------------------------------------------------------------------------
//turn flickering off (because light has been turned on)
void enable_flicker (int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker (segnum,sidenum)) != NULL)
		f->timer = 0;
}


#ifdef EDITOR

//returns 1 if ok, 0 if error
int add_flicker (int segnum, int sidenum, fix delay, unsigned long mask)
{
	int l;
	flickering_light *f;

#if TRACE
	//con_printf (CON_DEBUG,"add_flicker: %d:%d %x %x\n",segnum,sidenum,delay,mask);
#endif
	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			break;

	if (mask==0) {		//clearing entry
		if (l == Num_flickering_lights)
			return 0;
		else {
			int i;
			for (i=l;i<Num_flickering_lights-1;i++)
				Flickering_lights[i] = Flickering_lights[i+1];
			Num_flickering_lights--;
			return 1;
		}
	}

	if (l == Num_flickering_lights) {
		if (Num_flickering_lights == MAX_FLICKERING_LIGHTS)
			return 0;
		else
			Num_flickering_lights++;
	}

	f->segnum = segnum;
	f->sidenum = sidenum;
	f->delay = f->timer = delay;
	f->mask = mask;

	return 1;
}

#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
int flFrameTime = 0;

void FireLaser ()
{
	int i = primaryWeaponToWeaponInfo[gameData.weapons.nPrimary];
	gameData.app.nGlobalLaserFiringCount += WI_fire_count (i) * (Controls.fire_primary_state || Controls.fire_primaryDownCount);
	if ((gameData.weapons.nPrimary == FUSION_INDEX) && (gameData.app.nGlobalLaserFiringCount)) {
		if ((gameData.multi.players[gameData.multi.nLocalPlayer].energy < F1_0*2) && (gameData.app.fusion.xAutoFireTime == 0)) {
			gameData.app.nGlobalLaserFiringCount = 0;
		} else {
			flFrameTime += gameData.app.xFrameTime;
			if (gameData.app.fusion.xCharge == 0)
				gameData.multi.players[gameData.multi.nLocalPlayer].energy -= F1_0*2;
			gameData.app.fusion.xCharge += flFrameTime;
			gameData.multi.players[gameData.multi.nLocalPlayer].energy -= flFrameTime;
			if (gameData.multi.players[gameData.multi.nLocalPlayer].energy <= 0) {
				gameData.multi.players[gameData.multi.nLocalPlayer].energy = 0;
				gameData.app.fusion.xAutoFireTime = gameData.app.xGameTime -1;	//	Fire now!
			} else
				gameData.app.fusion.xAutoFireTime = gameData.app.xGameTime + flFrameTime/2 + 1;
			if (gameStates.limitFPS.bFusion && !gameStates.app.b40fpsTick)
				return;
			if (gameData.app.fusion.xCharge < F1_0*2)
				PALETTE_FLASH_ADD (gameData.app.fusion.xCharge >> 11, 0, gameData.app.fusion.xCharge >> 11);
			else
				PALETTE_FLASH_ADD (gameData.app.fusion.xCharge >> 11, gameData.app.fusion.xCharge >> 11, 0);
			if (gameData.app.xGameTime < gameData.app.fusion.xLastSoundTime)		//gametime has wrapped
				gameData.app.fusion.xNextSoundTime = gameData.app.fusion.xLastSoundTime = gameData.app.xGameTime;
			if (gameData.app.fusion.xNextSoundTime < gameData.app.xGameTime) {
				if (gameData.app.fusion.xCharge > F1_0*2) {
					DigiPlaySample (11, F1_0);
					ApplyDamageToPlayer (gameData.objs.console, gameData.objs.console, d_rand () * 4);
				} else {
					create_awareness_event (gameData.objs.console, PA_WEAPON_ROBOT_COLLISION);
					DigiPlaySample (SOUND_FUSION_WARMUP, F1_0);
					#ifdef NETWORK
					if (gameData.app.nGameMode & GM_MULTI)
						MultiSendPlaySound (SOUND_FUSION_WARMUP, F1_0);
					#endif
				}
				gameData.app.fusion.xLastSoundTime = gameData.app.xGameTime;
				gameData.app.fusion.xNextSoundTime = gameData.app.xGameTime + F1_0/8 + d_rand ()/4;
			}
		flFrameTime = 0;
		}
	}
}


//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void powerup_grab_cheat (object *player, int objnum)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert (gameData.objs.objects[objnum].type == OBJ_POWERUP);

	powerup_size = gameData.objs.objects[objnum].size;
	player_size = player->size;

	dist = VmVecDistQuick (&gameData.objs.objects[objnum].pos, &player->pos);

	if ((dist < 2* (powerup_size + player_size)) && !(gameData.objs.objects[objnum].flags & OF_SHOULD_BE_DEAD)) {
		vms_vector	collision_point;

		VmVecAvg (&collision_point, &gameData.objs.objects[objnum].pos, &player->pos);
		CollidePlayerAndPowerup (player, &gameData.objs.objects[objnum], &collision_point);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this segment, pick them up at up to twice pickuppable distance based on dot product
//	from player to powerup and player's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void PowerupGrabCheatAll (void)
{
	segment	*segp;
	int		objnum;

	segp = gameData.segs.segments + gameData.objs.console->segnum;
	objnum = segp->objects;

	while (objnum != -1) {
		if (gameData.objs.objects[objnum].type == OBJ_POWERUP)
			powerup_grab_cheat (gameData.objs.console, objnum);
		objnum = gameData.objs.objects[objnum].next;
	}

}

int	nLastLevelPathCreated = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
int mark_player_path_to_segment (int segnum)
{
	int		i;
	object	*objP = gameData.objs.console;
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (nLastLevelPathCreated == gameData.missions.nCurrentLevel) {
		return 0;
	}

	nLastLevelPathCreated = gameData.missions.nCurrentLevel;

	if (CreatePathPoints (objP, objP->segnum, segnum, gameData.ai.freePointSegs, &player_path_length, 100, 0, 0, -1) == -1) {
#if TRACE
		//con_printf (CON_DEBUG, "Unable to form path of length %i for myself\n", 100);
#endif
		return 0;
	}

	player_hide_index = (int) (gameData.ai.freePointSegs - gameData.ai.pointSegs);
	gameData.ai.freePointSegs += player_path_length;

	if ((int) (gameData.ai.freePointSegs - gameData.ai.pointSegs) + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
#if TRACE
		//con_printf (1, "Can't create path.  Not enough point_segs.\n");
#endif
		ai_reset_all_paths ();
		return 0;
	}

	for (i=1; i<player_path_length; i++) {
		short			segnum, objnum;
		vms_vector	seg_center;
		object		*obj;

		segnum = gameData.ai.pointSegs[player_hide_index+i].segnum;
#if TRACE
		//con_printf (CON_DEBUG, "%3i ", segnum);
#endif
		seg_center = gameData.ai.pointSegs[player_hide_index+i].point;

		objnum = CreateObject (OBJ_POWERUP, POW_ENERGY, -1, segnum, &seg_center, &vmdIdentityMatrix,
									  gameData.objs.pwrUp.info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP, 1);
		if (objnum == -1) {
			Int3 ();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj = &gameData.objs.objects[objnum];
		objP->rtype.vclip_info.nClipIndex = gameData.objs.pwrUp.info[objP->id].nClipIndex;
		objP->rtype.vclip_info.xFrameTime = gameData.eff.vClips [0][objP->rtype.vclip_info.nClipIndex].xFrameTime;
		objP->rtype.vclip_info.nCurFrame = 0;
		objP->lifeleft = F1_0*100 + d_rand () * 4;
	}
#if TRACE
	//con_printf (CON_DEBUG, "\n");
#endif
	return 1;
}

//-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
int create_special_path (void)
{
	int	i,j;

	//	---------- Find exit doors ----------
	for (i=0; i<=gameData.segs.nLastSegment; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (gameData.segs.segments[i].children[j] == -2) {
#if TRACE
				//con_printf (CON_DEBUG, "Exit at segment %i\n", i);
#endif
				return mark_player_path_to_segment (i);
			}

	return 0;
}

#endif


//-----------------------------------------------------------------------------
#ifndef RELEASE
int	Max_obj_count_mike = 0;

//	Shows current number of used gameData.objs.objects.
void show_free_objects (void)
{
	if (!(gameData.app.nFrameCount & 8)) {
		int	i;
		int	count=0;

#if TRACE
		//con_printf (CON_DEBUG, "gameData.objs.nLastObject = %3i, MAX_OBJECTS = %3i, now used = ", gameData.objs.nLastObject, MAX_OBJECTS);
#endif
		for (i=0; i<=gameData.objs.nLastObject; i++)
			if (gameData.objs.objects[i].type != OBJ_NONE)
				count++;
#if TRACE
		//con_printf (CON_DEBUG, "%3i", count);
#endif
		if (count > Max_obj_count_mike) {
			Max_obj_count_mike = count;
#if TRACE
			//con_printf (CON_DEBUG, " ***");
#endif
		}
#if TRACE
		//con_printf (CON_DEBUG, "\n");
#endif
	}

}

#endif

//-----------------------------------------------------------------------------
#ifdef WINDOWS
void game_win_init_cockpit_mask (int sram)
{
	if (dd_VR_offscreen_buffer && dd_VR_offscreen_buffer != dd_grd_backcanv) {
		dd_gr_free_canvas (dd_VR_offscreen_buffer);
	}

	if (GRMODEINFO (paged) && !GRMODEINFO (dbuf)) {
	//	Here we will make the VR_offscreen_buffer the 2nd page and hopefully
	//	we can just flip it, saving a blt.
		Int3 ();
	}
	else if (GRMODEINFO (dbuf)||GRMODEINFO (emul)) {
	//	The offscreen buffer will be created.  We will just blt this
	//	to the screen (which may be blted to the primary surface)
		if (grdCurScreen->sc_h < 200) {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (grdCurScreen->sc_w, 200);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
			if (sram) {
				DDFreeSurface (dd_VR_offscreen_buffer->lpdds);
				dd_VR_offscreen_buffer->lpdds = DDCreateSysMemSurface (grdCurScreen->sc_w, 200);
				dd_VR_offscreen_buffer->sram = 1;
			}
		}
		else {
			dd_VR_offscreen_buffer = dd_gr_create_canvas (grdCurScreen->sc_w, grdCurScreen->sc_h);
			VR_offscreen_buffer = &dd_VR_offscreen_buffer->canvas;
			if (sram) {
				DDFreeSurface (dd_VR_offscreen_buffer->lpdds);
				dd_VR_offscreen_buffer->lpdds = DDCreateSysMemSurface (grdCurScreen->sc_w, grdCurScreen->sc_h);
				dd_VR_offscreen_buffer->sram = 1;
			}
		}
	}

	DDGrInitSubCanvas (&dd_VR_render_buffer[0], dd_VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	DDGrInitSubCanvas (&dd_VR_render_buffer[1], dd_VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	GrInitSubCanvas (&VR_render_buffer[0], VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
	GrInitSubCanvas (&VR_render_buffer[1], VR_offscreen_buffer, 0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);

	GameInitRenderSubBuffers (0, 0, grdCurScreen->sc_w, grdCurScreen->sc_h);
}

//@@void game_win_init_cockpit_mask ()
//@@{
//@@	char title_pal[768];
//@@	dd_grs_canvas ccanv;
//@@	int pcx_error;
//@@	LPDIRECTDRAWSURFACE dds;
//@@
//@@	dds = DDCreateSurface (GRMODEINFO (w), GRMODEINFO (h), 1);
//@@	Assert (dds != NULL);
//@@
//@@	_lpDDSMask = dds;
//@@	ccanv.lpdds = dds;
//@@	dd_gr_reinit_canvas (&ccanv);
//@@
//@@	DDGrSetCurrentCanvas (&ccanv);
//@@	DDGRLOCK (dd_grd_curcanv)
//@@	{
//@@		if (W95DisplayMode == SM95_640x480x8) {
//@@			pcx_error=pcx_read_bitmap ("MASKB.PCX", &grdCurCanv->cv_bitmap,
//@@				grdCurCanv->cv_bitmap.bm_props.type,
//@@				title_pal);
//@@		}
//@@		else {
//@@			pcx_error=pcx_read_bitmap ("MASK.PCX", &grdCurCanv->cv_bitmap,
//@@				grdCurCanv->cv_bitmap.bm_props.type,
//@@				title_pal);
//@@		}
//@@	}
//@@	DDGRUNLOCK (dd_grd_curcanv);
//@@
//@@	Assert (pcx_error == PCX_ERROR_NONE);
//@@	Game_cockpit_copy_code = (ubyte *) (0xABADC0DE);
//@@}

#endif

//-----------------------------------------------------------------------------
/*
 * reads a flickering_light structure from a CFILE
 */
void flickering_light_read (flickering_light *fl, CFILE *fp)
{
	fl->segnum = CFReadShort (fp);
	fl->sidenum = CFReadShort (fp);
	fl->mask = CFReadInt (fp);
	fl->timer = CFReadFix (fp);
	fl->delay = CFReadFix (fp);
}

//-----------------------------------------------------------------------------
//eof
