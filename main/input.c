/* $Id: KConfig.c,v 1.27 2003/12/18 11:24:04 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE EVE.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines to configure keyboard, joystick, etc..
 *
 * Old Log:
 * Revision 1.18  1995/10/29  20:14:10  allender
 * don't read mouse 30x/sec.  Still causes problems -- left with
 * exposure at > 60 frame/s
 *
 * Revision 1.17  1995/10/27  14:16:35  allender
 * don't set lastreadtime when doing mouse stuff if we didn't
 * read mouse this frame
 *
 * Revision 1.16  1995/10/24  18:10:22  allender
 * get mouse stuff working right this time?
 *
 * Revision 1.15  1995/10/23  14:50:50  allender
 * corrected values for control type in KCSetControls
 *
 * Revision 1.14  1995/10/21  16:36:54  allender
 * fix up mouse read time
 *
 * Revision 1.13  1995/10/20  00:46:53  allender
 * fix up mouse reading problem
 *
 * Revision 1.12  1995/10/19  13:36:38  allender
 * mouse support in KConfig screens
 *
 * Revision 1.11  1995/10/18  21:06:06  allender
 * removed Int3 in cruise stuff -- was in there for debugging and
 * now not needed
 *
 * Revision 1.10  1995/10/17  13:12:47  allender
 * fixed config menus so buttons don't get configured
 *
 * Revision 1.9  1995/10/15  23:07:55  allender
 * added return key as second button for primary fire
 *
 * Revision 1.8  1995/09/05  08:49:47  allender
 * change 'PADRTN' label to 'ENTER'
 *
 * Revision 1.7  1995/09/01  15:38:22  allender
 * took out cap of reading controls max 25 times/sec
 *
 * Revision 1.6  1995/09/01  13:33:59  allender
 * erase all old text
 *
 * Revision 1.5  1995/08/18  10:20:55  allender
 * keep controls reading to 25 times/s max so fast
 * frame rates don't mess up control reading
 *
 * Revision 1.4  1995/07/28  15:43:13  allender
 * make mousebutton control primary fire
 *
 * Revision 1.3  1995/07/26  17:04:32  allender
 * new defaults and make joystick main button work correctly
 *
 * Revision 1.2  1995/07/17  08:51:03  allender
 * fixed up configuration menus to look right
 *
 * Revision 1.1  1995/05/16  15:26:56  allender
 * Initial revision
 *
 * Revision 2.11  1995/08/23  16:08:04  john
 * Added version 2 of external controls that passes the ship
 * position and orientation the drivers.
 *
 * Revision 2.10  1995/07/07  16:48:01  john
 * Fixed bug with new interface.
 *
 * Revision 2.9  1995/07/03  15:02:32  john
 * Added new version of external controls for Cybermouse absolute position.
 *
 * Revision 2.8  1995/06/30  12:30:28  john
 * Added -Xname command line.
 *
 * Revision 2.7  1995/03/30  16:36:56  mike
 * text localization.
 *
 * Revision 2.6  1995/03/21  14:39:31  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.5  1995/03/16  10:53:07  john
 * Move VFX center to Shift+Z instead of Enter because
 * it conflicted with toggling HUD on/off.
 *
 * Revision 2.4  1995/03/10  13:47:24  john
 * Added head tracking sensitivity.
 *
 * Revision 2.3  1995/03/09  18:07:06  john
 * Fixed bug with iglasses tracking not "centering" right.
 * Made VFX have bright headlight lighting.
 *
 * Revision 2.2  1995/03/08  15:32:39  john
 * Made VictorMaxx head tracking use Greenleaf code.
 *
 * Revision 2.1  1995/03/06  15:23:31  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:29:26  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.105  1995/02/22  14:11:58  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.104  1995/02/13  12:01:56  john
 * Fixed bug with buggin not mmaking player faster.
 *
 * Revision 1.103  1995/02/09  22:00:46  john
 * Added i-glasses tracking.
 *
 * Revision 1.102  1995/01/24  21:25:47  john
 * Fixed bug with slide/bank on not working with
 * Cyberman heading.,
 *
 * Revision 1.101  1995/01/24  16:09:56  john
 * Fixed bug with Wingman extreme customize text overwriting title.
 *
 * Revision 1.100  1995/01/24  12:37:46  john
 * Made Esc exit key define menu.
 *
 * Revision 1.99  1995/01/23  23:54:43  matt
 * Made keypad enter work
 *
 * Revision 1.98  1995/01/23  16:42:00  john
 * Made the external controls always turn banking off, leveling off
 * and passed automap state thru to the tsr.
 *
 * Revision 1.97  1995/01/12  11:41:33  john
 * Added external control reading.
 *
 * Revision 1.96  1995/01/05  10:43:58  mike
 * Handle case when TimerGetFixedSeconds () goes negative.  Happens at 9.1
 * hours.  Previously, joystick would stop functioning.  Now will work.
 *
 * Revision 1.95  1994/12/29  11:17:38  john
 * Took out some warnings and con_printf.
 *
 * Revision 1.94  1994/12/29  11:07:41  john
 * Fixed Thrustmaster and Logitech Wingman extreme
 * Hat by reading the y2 axis during the center stage
 * of the calibration, and using 75, 50, 27, and 3 %
 * as values for the 4 positions.
 *
 * Revision 1.93  1994/12/27  12:16:20  john
 * Fixed bug with slide on not working with joystick or mouse buttons.
 *
 * Revision 1.92  1994/12/20  10:34:15  john
 * Made sensitivity work for mouse & joystick and made
 * it only affect, pitch, heading, and roll.
 *
 * Revision 1.91  1994/12/16  00:11:23  matt
 * Made delete key act normally when debug out
 *
 * Revision 1.90  1994/12/14  17:41:15  john
 * Added more buttons so that  Yoke would work.
 *
 * Revision 1.89  1994/12/13  17:25:35  allender
 * Added Assert for bogus time for joystick reading.
 *
 * Revision 1.88  1994/12/13  14:48:01  john
 * Took out some debugging con_printf's
 *
 *
 * Revision 1.87  1994/12/13  14:43:02  john
 * Took out the code in KConfig to build direction array.
 * Called KCSetControls after selecting a new control type.
 *
 * Revision 1.86  1994/12/13  01:11:32  john
 * Fixed bug with message clearing overwriting
 * right border.
 *
 * Revision 1.85  1994/12/12  00:35:58  john
 * Added or thing for keys.
 *
 * Revision 1.84  1994/12/09  17:08:06  john
 * Made mouse a bit less sensitive.
 *
 * Revision 1.83  1994/12/09  16:04:00  john
 * Increased mouse sensitivity.
 *
 * Revision 1.82  1994/12/09  00:41:26  mike
 * fix hang in automap print screen
 *
 * Revision 1.81  1994/12/08  11:50:37  john
 * Made strcpy only copy corect number of chars,.
 *
 * Revision 1.80  1994/12/07  16:16:06  john
 * Added command to check to see if a joystick axes has been used.
 *
 * Revision 1.79  1994/12/07  14:52:28  yuan
 * Localization 492
 *
 * Revision 1.78  1994/12/07  13:37:40  john
 * Made the joystick thrust work in reverse.
 *
 * Revision 1.77  1994/12/07  11:28:24  matt
 * Did a little localization support
 *
 * Revision 1.76  1994/12/04  12:30:03  john
 * Made the Thrustmaster stick read every frame, not every 10 frames,
 * because it uses analog axis as buttons.
 *
 * Revision 1.75  1994/12/03  22:35:25  yuan
 * Localization 412
 *
 * Revision 1.74  1994/12/03  15:39:24  john
 * Made numeric keypad move in conifg.
 *
 * Revision 1.73  1994/12/01  16:23:39  john
 * Fixed include mistake.
 *
 * Revision 1.72  1994/12/01  16:07:57  john
 * Fixed bug that disabled joystick in automap because it used gametime, which is
 * paused during automap. Fixed be used timer_Get_fixed_seconds instead of gameData.app.xGameTime.
 *
 * Revision 1.71  1994/12/01  12:30:49  john
 * Made Ctrl+D delete, not Ctrl+E
 *
 * Revision 1.70  1994/12/01  11:52:52  john
 * Added default values for GamePad.
 *
 * Revision 1.69  1994/11/30  00:59:12  mike
 * optimizations.
 *
 * Revision 1.68  1994/11/29  03:45:50  john
 * Added joystick sensitivity; Added sound channels to detail menu.  Removed -maxchannels
 * command line arg.
 *
 * Revision 1.67  1994/11/27  23:13:44  matt
 * Made changes for new con_printf calling convention
 *
 * Revision 1.66  1994/11/27  19:52:12  matt
 * Made screen shots work in a few more places
 *
 * Revision 1.65  1994/11/22  16:54:50  mike
 * autorepeat on missiles.
 *
 * Revision 1.64  1994/11/21  11:16:17  rob
 * Changed calls to GameLoop to calls to MultiMenuPoll and changed
 * conditions under which they are called.
 *
 * Revision 1.63  1994/11/19  15:14:48  mike
 * remove unused code and data
 *
 * Revision 1.62  1994/11/18  23:37:56  john
 * Changed some shorts to ints.
 *
 * Revision 1.61  1994/11/17  13:36:35  rob
 * Added better network hook in KConfig menu.
 *
 * Revision 1.60  1994/11/14  20:09:13  john
 * Made Tab be default for automap.
 *
 * Revision 1.59  1994/11/13  16:34:07  matt
 * Fixed victormaxx angle conversions
 *
 * Revision 1.58  1994/11/12  14:47:05  john
 * Added support for victor head tracking.
 *
 * Revision 1.57  1994/11/08  15:14:55  john
 * Added more calls so net doesn't die in net game.
 *
 * Revision 1.56  1994/11/07  14:01:07  john
 * Changed the gamma correction sequencing.
 *
 * Revision 1.55  1994/11/01  16:40:08  john
 * Added Gamma correction.
 *
 * Revision 1.54  1994/10/25  23:09:26  john
 * Made the automap key configurable.
 *
 * Revision 1.53  1994/10/25  13:11:59  john
 * Made keys the way Adam speced 'em for final game.
 *
 * Revision 1.52  1994/10/24  17:44:22  john
 * Added stereo channel reversing.
 *
 * Revision 1.51  1994/10/22  13:23:18  john
 * Made default Rear View key be R.
 *
 * Revision 1.50  1994/10/22  13:20:09  john
 * Took out toggle primary/secondary weapons.  Fixed black
 * background for 'axes' and 'buttons' text.
 *
 * Revision 1.49  1994/10/21  15:20:15  john
 * Made PrtScr do screen dump, not F2.
 *
 * Revision 1.48  1994/10/21  13:41:36  john
 * Allowed F2 to screen dump.
 *
 * Revision 1.47  1994/10/17  13:07:05  john
 * Moved the descent.cfg info into the player config file.
 *
 * Revision 1.46  1994/10/14  15:30:22  john
 * Added Cyberman default positions.
 *
 * Revision 1.45  1994/10/14  15:24:54  john
 * Made Cyberman work with config.
 *
 * Revision 1.44  1994/10/14  12:46:04  john
 * Added the ability to reset all to default.
 *
 * Revision 1.43  1994/10/14  12:18:31  john
 * Made mouse invert axis always be 0 or 1.
 *
 * Revision 1.42  1994/10/14  12:16:03  john
 * Changed code so that by doing DEL+F12 saves the current KConfig
 * values as default. Added support for drop_bomb key.  Took out
 * unused slots for keyboard.  Made keyboard use control_type of 0
 * save slots.
 *
 * Revision 1.41  1994/10/13  21:27:02  john
 * Made axis invert value always be 0 or 1.
 *
 * Revision 1.40  1994/10/13  20:18:15  john
 * Added some more system keys, such as F? and CAPSLOCK.
 *
 * Revision 1.39  1994/10/13  19:22:29  john
 * Added separate config saves for different devices.
 * Made all the devices work together better, such as mice won't
 * get read when you're playing with the joystick.
 *
 * Revision 1.38  1994/10/13  15:41:57  mike
 * Remove afterburner.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: KConfig.c,v 1.27 2003/12/18 11:24:04 btb Exp $";
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
#include <math.h>

#include "pa_enabl.h"                   //$$POLY_ACC
#include "error.h"
#include "inferno.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "event.h"
#include "joy.h"
#include "mouse.h"
#include "kconfig.h"
#include "gauges.h"
#include "joydefs.h"
#include "songs.h"
#include "render.h"
#include "digi.h"
#include "newmenu.h"
#include "endlevel.h"
#include "multi.h"
#include "timer.h"
#include "text.h"
#include "player.h"
#include "menu.h"
#include "automap.h"
#include "args.h"
#include "lighting.h"
#include "ai.h"
#include "cntrlcen.h"
#include "network.h"
#include "hudmsg.h"
#include "ogl_init.h"
#include "object.h"
#include "inferno.h"
#include "kconfig.h"
#include "input.h"
#include "gamecntl.h"
#if defined (TACTILE)
 #include "tactile.h"
#endif

#if defined (POLY_ACC)
#include "poly_acc.h"
#endif

#include "collide.h"

#ifdef USE_LINUX_JOY
#include "joystick.h"
#endif

control_info Controls;

// *must* be defined - set to 0 if no limit
#define BASE_PITCH	2200
#define MAX_PITCH		 (gameStates.input.kcFrameTime / kcFrameCount)

#define KCCLAMP(_val,_max)	if ((_val) < - (_max)) (_val) = (fix) -(_max); else if ((_val) > (_max)) (_val) = (fix) (_max)

static long kcFrameCount = 0;

//------------------------------------------------------------------------------

void SetMaxPitch (int nMinTurnRate)
{
gameOpts->input.nMaxPitch = (BASE_PITCH * 10) / (nMinTurnRate ? nMinTurnRate : 20);
}

//------------------------------------------------------------------------------

#define	PH_SCALE	1

#ifndef __MSDOS__ // WINDOWS
#define	JOYSTICK_READ_TIME	 (F1_0/40)		//	Read joystick at 40 Hz.
#else
#define	JOYSTICK_READ_TIME	 (F1_0/10)		//	Read joystick at 10 Hz.
#endif

fix	LastReadTime = 0;

fix	joy_axis [JOY_MAX_AXES];

//------------------------------------------------------------------------------

#ifndef WINDOWS

fix Next_toggle_time [3]={0,0,0};

int AllowToToggle (int i)
{
  //used for keeping tabs of when its ok to toggle headlight,primary,and secondary
 
	if (Next_toggle_time [i] > gameData.app.xGameTime)
		if (Next_toggle_time [i] < gameData.app.xGameTime + (F1_0/8))	//	In case time is bogus, never wait > 1 second.
			return 0;

	Next_toggle_time [i] = gameData.app.xGameTime + (F1_0/8);

	return 1;
}


#ifdef D2X_KEYS
//added on 2/7/99 by Victor Rachels for jostick state setting
int d2x_joystick_ostate [20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//end this section adition - VR
#endif

#ifdef _DEBUG
# define TT_THRESHOLD 0xFFFF
# define DEFTIME time_t _t0 = SDL_GetTicks (); time_t _t = _t0; time_t _dt;
# define TAKETIME (_c,_i) _dt = SDL_GetTicks (); _t = _dt;
# define TAKETIME0 (_c) _dt = SDL_GetTicks ();
#else
# define TAKETIME (_c,_i)
# define TAKETIME0 (_c)
# define DEFTIME
#endif

//------------------------------------------------------------------------------

int ControlsReadKeyboard (void)
{
return 1;
}

//------------------------------------------------------------------------------

int ControlsReadMouse (int *mouse_axis, int *mouse_buttons)
{
	int	dx, dy;
#ifdef SDL_INPUT
	int	dz;
#endif
#ifdef SDL_INPUT
MouseGetDeltaZ (&dx, &dy, &dz);
#else
MouseGetDelta (&dx, &dy);
#endif
mouse_axis [0] = (int) ((dx * gameStates.input.kcFrameTime) / 35);
mouse_axis [1] = (int) ((dy * gameStates.input.kcFrameTime) / 25);
#ifdef SDL_INPUT
mouse_axis [2] = (int) (dz * gameStates.input.kcFrameTime);
#endif
*mouse_buttons = MouseGetButtons ();
return 1;
}

//------------------------------------------------------------------------------

static int joy_sens_mod [4] = {16, 16, 16, 16};

static double dMaxAxis = 127.0;

int AttenuateAxis (double a, int nAxis)
{
if (gameOpts->input.bLinearJoySens)
	return (int) a;
else if (!a)
	return 0;
else {
		double	d;
#ifdef _DEBUG
		double	p, h, e;
#endif

	if (a > dMaxAxis)
		a = dMaxAxis;
	else if (a < -dMaxAxis)
		a = -dMaxAxis;
#ifdef RELEASE
	d = dMaxAxis * pow (fabs ((double) a / dMaxAxis), (double) joy_sens_mod [nAxis % 4] / 16.0);
#else
	h = fabs ((double) a / dMaxAxis);
	e = (double) joy_sens_mod [nAxis % 4] / 16.0;
	p = pow (h, e);
	d = dMaxAxis * p;
#endif
	if (d < 1.0)
		d = 1.0;
	if (a < 0)
		d = -d;
	return (int) d;
	}
}

//------------------------------------------------------------------------------

inline int ControlsReadJoyAxis (int i, int raw_joy_axis [])
{
int joy_deadzone_scaled = joy_deadzone [i % 4] / 128;
int h = joy_get_scaled_reading (raw_joy_axis [i], i);
if (gameOpts->input.bSyncJoyAxes && (kc_joystick [23].value == i))		// If this is the throttle
	joy_deadzone_scaled *= 2;				// Then use a larger dead-zone
if (h > joy_deadzone_scaled) 
	h = ((h - joy_deadzone_scaled) * 128) / (128 - joy_deadzone_scaled);
else if (h < -joy_deadzone_scaled)
	h = ((h + joy_deadzone_scaled) * 128) / (128 - joy_deadzone_scaled);
else
	h = 0;
return (int) ((AttenuateAxis (h, i) * gameStates.input.kcFrameTime) / 128);	
}

//------------------------------------------------------------------------------

int ControlsReadJoystick (int *joy_axis)
{
	int	raw_joy_axis [JOY_MAX_AXES];
	unsigned long channel_masks;
	int	i, use_joystick = 0;
	fix	ctime;

if (gameStates.limitFPS.bJoystick)
	ctime = TimerGetFixedSeconds ();

for (i = 0; i < 4; i++)
	joy_sens_mod [i] = 128 - 7 * gameOpts->input.joySensitivity [i];
if (gameStates.limitFPS.bJoystick) {
	if ((LastReadTime + JOYSTICK_READ_TIME > ctime) && (gameStates.input.nJoyType != CONTROL_THRUSTMASTER_FCS)) {
# ifndef __MSDOS__
		if ((ctime < 0) && (LastReadTime >= 0))
# else
		if ((ctime < 0) && (LastReadTime > 0))
# endif
			LastReadTime = ctime;
			use_joystick=1;
			} 
	else if (gameOpts->input.bUseJoystick) {
		LastReadTime = ctime;
		if	(channel_masks = joystick_read_raw_axis (JOY_ALL_AXIS, raw_joy_axis)) {
			for (i = 0; i < JOY_MAX_AXES; i++) {
#ifndef SDL_INPUT
				if (channel_masks & (1 << i))	{
#endif
					if ((i == 3) && (gameStates.input.nJoyType == CONTROL_THRUSTMASTER_FCS))
						ControlsReadFCS (raw_joy_axis [i]);
					else 
						joy_axis [i] = ControlsReadJoyAxis (i, raw_joy_axis);
#ifndef SDL_INPUT
					} 
					else
						joy_axis [i] = 0;
#endif
				}	
			}
		use_joystick=1;
		}
	else {
		for (i = 0; i < JOY_MAX_AXES; i++)
			joy_axis [i] = 0;
		}
	}
else {   // LIMIT_JOY_FPS
	memset (joy_axis, 0, sizeof (joy_axis));
	if (gameOpts->input.bUseJoystick) {
		if (channel_masks = joystick_read_raw_axis (JOY_ALL_AXIS, raw_joy_axis)) {
			for (i = 0; i < JOY_MAX_AXES; i++)	{
				if (channel_masks & (1 << i))
					joy_axis [i] = ControlsReadJoyAxis (i, raw_joy_axis);
				else
					joy_axis [i] = 0;
				}	
			}
		use_joystick = 1;
		}
	}
return use_joystick;
}

//------------------------------------------------------------------------------

void ControlsSetFCSButton (int btn, int button)
{
	int state,time_down,upcount,downcount;
	state = time_down = upcount = downcount = 0;

if (joy_get_button_state (btn)) {
	if (btn==button) {
		state = 1;
		time_down = gameData.app.xFrameTime;
		} 
	else
		upcount=1;
	}
else {
	if (btn==button) {
		state = 1;
		time_down = gameData.app.xFrameTime;
		downcount=1;
		}
	else
		upcount=1;
	}				
joy_set_btn_values (btn, state, time_down, downcount, upcount);
					
}

//------------------------------------------------------------------------------

void ControlsReadFCS (int raw_axis)
{
	int raw_button, button, axis_min [4], axis_center [4], axis_max [4];

	if (gameStates.input.nJoyType != CONTROL_THRUSTMASTER_FCS) return;

	joy_get_cal_vals (axis_min, axis_center, axis_max);

	if (axis_max [3] > 1)
		raw_button = (raw_axis*100)/axis_max [3];
	else
		raw_button = 0;

	if (raw_button > 88)
		button = 0;
	else if (raw_button > 63)
		button = 7;
	else if (raw_button > 39)
		button = 11;
	else if (raw_button > 15)
		button = 15;
	else	
		button = 19;

	ControlsSetFCSButton (19, button);
	ControlsSetFCSButton (15, button);
	ControlsSetFCSButton (11, button);
	ControlsSetFCSButton (7, button);
}
		
//------------------------------------------------------------------------------

int ControlsReadCyberman (int *mouse_axis, int *mouse_buttons)
{
	int idx, idy;

mouse_get_cyberman_pos (&idx,&idy);
mouse_axis [0] = (int) ((idx*gameStates.input.kcFrameTime)/128);
mouse_axis [1] = (int) ((idy*gameStates.input.kcFrameTime)/128);
*mouse_buttons = MouseGetButtons ();
return 1;
}

//------------------------------------------------------------------------------

inline int HaveKey (kc_item *k, int i)
{
return k [i].value;
}

//------------------------------------------------------------------------------

inline int HaveKeyCount (kc_item *k, int i)
{
	int	v = k [i].value;

if (v == 255)
	return 0;
#ifdef _DEBUG
v = keyDownCount (v);
if (!v)
	return 0;
return v;
#else
return keyDownCount (v);
#endif
}

//------------------------------------------------------------------------------

inline int HaveD2XKey (kc_item *k, int i)
{
	int	v = k [i].value;

if (v == 255)
	return 0;
if (key_flags (v))
	return 0;
#ifdef _DEBUG
if (v = keyDownCount (v))
	return v;
return 0;
#else
return keyDownCount (v);
#endif
}

//------------------------------------------------------------------------------

void ControlsDoD2XKeys (int *slide_on, int *bank_on, fix *pkp, fix *pkh, int *nCruiseSpeed, int bGetSlideBank)
{
#ifdef D2X_KEYS
//added on 2/4/99 by Victor Rachels for d1x keys
//--------- Read primary weapon select -------------
//the following "if" added by WraithX to stop deadies from switchin weapons, 4/14/00
if (!(gameStates.app.bPlayerIsDead || gameStates.app.bAutoMap)) {
	{
		int i, d2x_joystick_state [10];

		for (i=0;i<10;i++)
			d2x_joystick_state [i] = joy_get_button_state (kc_d2x [i*2+1].value);


		//----------------Weapon 1----------------
		if ((HaveD2XKey (kc_d2x, 0)) ||
			 (joy_get_button_state (kc_d2x [1].value) &&
			 (d2x_joystick_state [0]!=d2x_joystick_ostate [0])))
		{
			//int i, valu=0;
			DoSelectWeapon (0,0);
			/*
			for (i=MAX_PRIMARY_WEAPONS;i<MAX_PRIMARY_WEAPONS+NEWPRIMS;i++)
				if (primary_order [i]>primary_order [valu]&&PlayerHasWeapon (i,0))
					valu = i;
			LaserPowSelected = valu;
			*/
		}
		//----------------Weapon 2----------------
		if ((HaveD2XKey (kc_d2x, 2)) ||
			 (joy_get_button_state (kc_d2x [3].value) &&
			 (d2x_joystick_state [1]!=d2x_joystick_ostate [1])))
			DoSelectWeapon (1,0);
		//----------------Weapon 3----------------
		if ((HaveD2XKey (kc_d2x, 4)) ||
			 (joy_get_button_state (kc_d2x [5].value) &&
			 (d2x_joystick_state [2]!=d2x_joystick_ostate [2])))
			DoSelectWeapon (2,0);
		//----------------Weapon 4----------------
		if ((HaveD2XKey (kc_d2x, 6)) ||
			 (joy_get_button_state (kc_d2x [7].value) &&
			 (d2x_joystick_state [3]!=d2x_joystick_ostate [3])))
			DoSelectWeapon (3,0);
		//----------------Weapon 5----------------
		if ((HaveD2XKey (kc_d2x, 8)) ||
			 (joy_get_button_state (kc_d2x [9].value) &&
			 (d2x_joystick_state [4]!=d2x_joystick_ostate [4])))
			DoSelectWeapon (4,0);

		//--------- Read secondary weapon select ----------
		//----------------Weapon 6----------------
		if ((HaveD2XKey (kc_d2x, 10)) ||
			 (joy_get_button_state (kc_d2x [11].value) &&
			 (d2x_joystick_state [5]!=d2x_joystick_ostate [5])))
			DoSelectWeapon (0,1);
		//----------------Weapon 7----------------
		if ((HaveD2XKey (kc_d2x, 12)) ||
			 (joy_get_button_state (kc_d2x [13].value) &&
			 (d2x_joystick_state [6]!=d2x_joystick_ostate [6])))
			DoSelectWeapon (1,1);
		//----------------Weapon 8----------------
		if ((HaveD2XKey (kc_d2x, 14)) ||
			 (joy_get_button_state (kc_d2x [15].value) &&
			 (d2x_joystick_state [7]!=d2x_joystick_ostate [7])))
			DoSelectWeapon (2,1);
		//----------------Weapon 9----------------
		if ((HaveD2XKey (kc_d2x, 16)) ||
			 (joy_get_button_state (kc_d2x [17].value) &&
			 (d2x_joystick_state [8]!=d2x_joystick_ostate [8])))
			DoSelectWeapon (3,1);
		//----------------Weapon 0----------------
		if ((HaveD2XKey (kc_d2x, 18)) ||
			 (joy_get_button_state (kc_d2x [19].value) &&
			 (d2x_joystick_state [9]!=d2x_joystick_ostate [9])))
			DoSelectWeapon (4,1);
		memcpy (d2x_joystick_ostate,d2x_joystick_state,10*sizeof (int));
	}
	//end this section addition - VR
}//end "if (!gameStates.app.bPlayerIsDead)" - WraithX
#endif
}

//------------------------------------------------------------------------------

int ControlsLimitTurnRate (int bUseMouse)
{
if (! (gameOpts->input.bLimitTurnRate || (gameData.app.nGameMode & GM_MULTI)) /*|| (kcFrameCount == 1)*/)
	return 0;
if (gameStates.app.bAutoMap || 
	 gameOpts->input.bJoyMouse ||
	 !(bUseMouse && EGI_FLAG (bMouseLook, 0, 0))) {
	KCCLAMP (Controls.pitch_time, MAX_PITCH);
	KCCLAMP (Controls.heading_time, MAX_PITCH);
	}
KCCLAMP (Controls.bank_time, MAX_PITCH);
return 1;
}

//------------------------------------------------------------------------------

void KCToggleBomb (void)
{
int bomb = bLastSecondaryWasSuper [PROXIMITY_INDEX] ? PROXIMITY_INDEX : SMART_MINE_INDEX;
if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) ||
	 (!gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [PROXIMITY_INDEX] &&
	  !gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [SMART_MINE_INDEX])) {
	DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
	HUDInitMessage (TXT_NOBOMBS);
	}
else if (!gameData.multi.players [gameData.multi.nLocalPlayer].secondary_ammo [bomb]) {
	DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
	HUDInitMessage (TXT_NOBOMB_ANY, (bomb==SMART_MINE_INDEX)? TXT_SMART_MINES : TXT_PROX_BOMBS);
	}
else {
	bLastSecondaryWasSuper [PROXIMITY_INDEX]=!bLastSecondaryWasSuper [PROXIMITY_INDEX];
	DigiPlaySampleOnce (SOUND_GOOD_SELECTION_SECONDARY, F1_0);
	}
}

//------------------------------------------------------------------------------

extern fix key_ramp (int scancode);

inline int DeltaCtrl (v, speedFactor, keyRampScale, i)
{
	int	h;

if (v == 255)
	return 0;
h = speedFactor * KeyDownTime (v);
if (!h)
	return 0;
if (gameOpts->input.bRampKeys [i])
	return h / key_ramp (v);
return h;
}


#define DELTACTRL(_i,_b)	DeltaCtrl (kc_keyboard [_i].value, speedFactor, gameOpts->input.keyRampScale, _b)

void ControlsDoKeyboard (int *slide_on, int *bank_on, fix *pkp, fix *pkh, int *nCruiseSpeed, int bGetSlideBank)
{
	int	i, v, pitchScale = extraGameInfo [IsMultiGame].bFastPitch ? 2 * PH_SCALE : 1;
	fix	kp = 0;
	int	speedFactor = gameStates.app.cheats.bTurboMode ? 2 : 1;
	static int key_signs [8] = {1,1,-1,-1,-1,-1,1,1};

#if 0//def DELTACTRL
if (gameOpts->input.keyRampScale < 100)
	speedFactor *= gameOpts->input.keyRampScale;
#endif

if (bGetSlideBank == 0) {
	for (i = 0; i < 2; i++) {
		if ((v = HaveKey (kc_keyboard, 8 + i)) < 255) {
			if (keyd_pressed [v])
				*slide_on |= keyd_pressed [v];
			}
		if ((v = HaveKey (kc_keyboard, 18 + i)) < 255) 
			if (keyd_pressed [v])
				*bank_on |= keyd_pressed [v];
		}
	return;
	}

if (bGetSlideBank == 2) {
	for (i = 0; i < 2; i++) {
		if ((v = HaveKey (kc_keyboard, 24 + i)) < 255) {
			Controls.fire_primary_state |= keyd_pressed [v];
			Controls.fire_primaryDownCount += keyDownCount (v);
			}
		if ((v = HaveKey (kc_keyboard, 26 + i)) < 255) {
			Controls.fire_secondary_state |= keyd_pressed [v];
			Controls.fire_secondaryDownCount += keyDownCount (v);
			}
		if ((v = HaveKey (kc_keyboard, 28 + i)) < 255) 
			Controls.fire_flareDownCount += keyDownCount (v);
		}

	for (i = 0; i < 2; i++) {
#ifdef DELTACTRL
		Controls.sideways_thrust_time -= DELTACTRL (10 + i, 2);
		Controls.sideways_thrust_time += DELTACTRL (12 + i, 2);
		Controls.vertical_thrust_time += DELTACTRL (14 + i, 2);
		Controls.vertical_thrust_time -= DELTACTRL (16 + i, 2);
		Controls.bank_time += DELTACTRL (20 + i, 1);
		Controls.bank_time -= DELTACTRL (22 + i, 1);
		Controls.forward_thrust_time += DELTACTRL (30 + i, 0);
		Controls.forward_thrust_time -= DELTACTRL (32 + i, 0);
#else
		if ((v = HaveKey (kc_keyboard, 14 + i)) < 255) {
			h = speedFactor * KeyDownTime (v);
			if (gameOpts->input.bRampKeys [1])
				h /= key_ramp (v);
			Controls.vertical_thrust_time += h;
			}
		if ((v = HaveKey (kc_keyboard, 16 + i)) < 255) 
			Controls.vertical_thrust_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 10 + i)) < 255) 
			Controls.sideways_thrust_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 12 + i)) < 255) 
			Controls.sideways_thrust_time += speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 20 + i)) < 255) 
			Controls.bank_time += speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 22 + i)) < 255) 
			Controls.bank_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 30 + i)) < 255) 
			Controls.forward_thrust_time += speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 32 + i)) < 255) 
			Controls.forward_thrust_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
#endif
		if ((v = HaveKey (kc_keyboard, 46 + i)) < 255) 
			Controls.afterburner_state |= keyd_pressed [v];
		// count bomb drops
		if ((v = HaveKey (kc_keyboard, 34 + i)) < 255) 
			Controls.drop_bombDownCount += keyDownCount (v);
		// charge chield
		if (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_CONVERTER) {
			if (keyd_pressed [v = HaveKey (kc_keyboard, 56 + i)])
				TransferEnergyToShield (KeyDownTime (v));
			}
		// rear view
		if ((v = HaveKey (kc_keyboard, 36 + i)) < 255) {
			Controls.rear_viewDownCount += keyDownCount (v);
			Controls.rear_view_down_state |= keyd_pressed [v];
			}
		// automap
		if ((v = HaveKey (kc_keyboard, 44 + i)) < 255) {
			Controls.automapDownCount += keyDownCount (v);
			Controls.automap_state |= keyd_pressed [v];
			}
	}
	// headlight and weapon cycling
	if ((v = HaveKey (kc_keyboard, 54)) < 255)
		Controls.headlight_count = keyDownCount (v);
	Controls.headlight_count += HaveKeyCount (kc_keyboard, 55);
	Controls.cycle_primary_count = HaveKeyCount (kc_keyboard, 48);
	Controls.cycle_primary_count += HaveKeyCount (kc_keyboard, 49);
	Controls.cycle_secondary_count += HaveKeyCount (kc_keyboard, 50);
	Controls.cycle_secondary_count += HaveKeyCount (kc_keyboard, 51);
	Controls.zoomDownCount += HaveKeyCount (kc_keyboard, 52);
	Controls.zoomDownCount += HaveKeyCount (kc_keyboard, 53);
	Controls.toggle_icons_count += HaveKeyCount (kc_keyboard, 59);
	Controls.useCloakDownCount += HaveKeyCount (kc_keyboard, 60);
	Controls.useCloakDownCount += HaveKeyCount (kc_keyboard, 61);
	Controls.useInvulDownCount += HaveKeyCount (kc_keyboard, 62);
	Controls.useInvulDownCount += HaveKeyCount (kc_keyboard, 63);

	// toggle bomb
	if (((v = HaveKey (kc_keyboard, 58)) < 255) && keyDownCount (v))
		KCToggleBomb ();

	// cruise speed
#ifndef DELTACTRL
//	if (gameOpts->input.keyRampScale)
//		speedFactor /= gameOpts->input.keyRampScale;
#endif
	for (i = 0; i < 4; i++)
		if ((v = HaveKey (kc_keyboard, 38 + i)) < 255) 
			*nCruiseSpeed += key_signs [i] * fixdiv (speedFactor * KeyDownTime (v) * 5, gameStates.input.kcFrameTime);
	for (i = 0; i < 2; i++)
		if (((v = HaveKey (kc_keyboard, 42 + i)) < 255) && keyDownCount (v))
			*nCruiseSpeed = 0;
#ifndef DELTACTRL
//	if (gameOpts->input.keyRampScale)
//		speedFactor *= gameOpts->input.keyRampScale;
#endif
	}

// special slide & bank toggle handling
if (*slide_on) {
	if (bGetSlideBank == 2) {
		for (i = 0; i < 2; i++) {
#ifdef DELTACTRL
			Controls.vertical_thrust_time += DELTACTRL (i, 2);
			Controls.vertical_thrust_time -= DELTACTRL (2 + i, 2);
			Controls.sideways_thrust_time -= DELTACTRL (4 + i, 2);
			Controls.sideways_thrust_time += DELTACTRL (6 + i, 2);
#else
			if ((v = HaveKey (kc_keyboard, 0 + i)) < 255) 
				Controls.vertical_thrust_time += speedFactor * KeyDownTime (v) / key_ramp (v);
			if ((v = HaveKey (kc_keyboard, 2 + i)) < 255) 
				Controls.vertical_thrust_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
			if ((v = HaveKey (kc_keyboard, 4 + i)) < 255) 
				Controls.sideways_thrust_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
			if ((v = HaveKey (kc_keyboard, 6 + i)) < 255) 
				Controls.sideways_thrust_time += speedFactor * KeyDownTime (v) / key_ramp (v);
#endif
			}
		}
	}
else if (bGetSlideBank == 1) {
	for (i = 0; i < 4; i++)
#ifdef DELTACTRL
		kp += key_signs [i] * DELTACTRL (i, 1) / pitchScale;
#else
		if ((v = HaveKey (kc_keyboard, i)) < 255)
			kp += key_signs [i] * speedFactor * KeyDownTime (v) / pitchScale / key_ramp (v);
#endif
	if (!*bank_on)
		for (i = 4; i < 8; i++)
#ifdef DELTACTRL
			*pkh += key_signs [i] * DELTACTRL (i, 1) / PH_SCALE;
#else
			if ((v = HaveKey (kc_keyboard, i)) < 255)
				*pkh += key_signs [i] * speedFactor * KeyDownTime (v) / PH_SCALE / key_ramp (v);
#endif
	}
if (*bank_on) {
	if (bGetSlideBank == 2) {
#ifdef DELTACTRL
		for (i = 4; i < 6; i++)
			Controls.bank_time += DELTACTRL (i, 1);
		for (i = 6; i < 8; i++)
			Controls.bank_time -= DELTACTRL (i, 1);
#else
		if ((v = HaveKey (kc_keyboard, 4)) < 255) 
			Controls.bank_time += speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 5)) < 255) 
			Controls.bank_time += speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 6)) < 255) 
			Controls.bank_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
		if ((v = HaveKey (kc_keyboard, 7)) < 255) 
			Controls.bank_time -= speedFactor * KeyDownTime (v) / key_ramp (v);
#endif
		}
	}
if (bGetSlideBank == 2)
	ControlsLimitTurnRate (0);
//KCCLAMP (kp, MAX_PITCH / extraGameInfo [IsMultiGame].bFastPitch);
*pkp = kp;
}

//------------------------------------------------------------------------------

inline int ipower (int x, int y)
{
	int h = x;
	int r = y % 2;

while (y > 1) {
	h *= h;
	y /= 2;
	}
if (r)
	h *= x;
return h;
}

//------------------------------------------------------------------------------

inline int DeltaAxis (int v)
{
#if 0//def _DEBUG
int a = gameOpts->input.bLinearJoySens ? joy_axis [v] * 16 / joy_sens_mod [v % 4] : joy_axis [v];
if (a)
	HUDMessage (0, "%d", a);
return a;
#else
return gameOpts->input.bLinearJoySens ? joy_axis [v] * 16 / joy_sens_mod [v % 4] : joy_axis [v];
#endif
}

//------------------------------------------------------------------------------

void ControlsDoJoystick (int *slide_on, int *bank_on, fix *pkp, fix *pkh, int *nCruiseSpeed, int bGetSlideBank)
{
	int	v;

if (bGetSlideBank == 0) {
	if ((v = kc_joystick [5].value) < 255) 
		*slide_on |= joy_get_button_state (v);
	if ((v = kc_joystick [10].value) < 255)
		*bank_on |= joy_get_button_state (v);
	return;
	}
// Buttons
if (bGetSlideBank == 2) {
	if ((v = kc_joystick [0].value) < 255) {
		Controls.fire_primary_state |= joy_get_button_state (v);
		Controls.fire_primaryDownCount += joy_get_button_down_cnt (v);
		}
	if ((v = kc_joystick [1].value) < 255) {
		Controls.fire_secondary_state |= joy_get_button_state (v);
		Controls.fire_secondaryDownCount += joy_get_button_down_cnt (v);
		}
	if ((v = kc_joystick [2].value) < 255) 
		Controls.forward_thrust_time += joy_get_button_down_time (v);
	if ((v = kc_joystick [3].value) < 255) 
		Controls.forward_thrust_time -= joy_get_button_down_time (v);
	if ((v = kc_joystick [4].value) < 255) 
		Controls.fire_flareDownCount += joy_get_button_down_cnt (v);
	if ((v = kc_joystick [6].value) < 255) 
		Controls.sideways_thrust_time -= joy_get_button_down_time (v);
	if ((v = kc_joystick [7].value) < 255) 
		Controls.sideways_thrust_time += joy_get_button_down_time (v);
	if ((v = kc_joystick [8].value) < 255) 
		Controls.vertical_thrust_time += joy_get_button_down_time (v);
	if ((v = kc_joystick [9].value) < 255) 
		Controls.vertical_thrust_time -= joy_get_button_down_time (v);
	if ((v = kc_joystick [11].value) < 255) 
		Controls.bank_time += joy_get_button_down_time (v);
	if ((v = kc_joystick [12].value) < 255) 
		Controls.bank_time -= joy_get_button_down_time (v);
	if ((v = kc_joystick [25].value) < 255) {
		Controls.rear_viewDownCount += joy_get_button_down_cnt (v);
		Controls.rear_view_down_state |= joy_get_button_state (v);
		}
	if ((v = kc_joystick [26].value) < 255) 
		Controls.drop_bombDownCount += joy_get_button_down_cnt (v);
	if ((v = kc_joystick [27].value) < 255) 
		Controls.afterburner_state |= joy_get_button_state (v);
	if ((v = kc_joystick [28].value) < 255) 
		Controls.cycle_primary_count += joy_get_button_down_cnt (v);
	if ((v = kc_joystick [29].value) < 255) 
		Controls.cycle_secondary_count += joy_get_button_down_cnt (v);
	if ((v = kc_joystick [30].value) < 255) 
		Controls.headlight_count += joy_get_button_down_cnt (v);
	if (((v = kc_joystick [31].value) < 255) && joy_get_button_down_cnt (v))
		KCToggleBomb ();
	if ((v = kc_joystick [32].value) < 255) 
		Controls.toggle_icons_count += joy_get_button_down_cnt (v);
	if ((v = kc_joystick [33].value) < 255) 
		Controls.automapDownCount += joy_get_button_down_cnt (v);

	// Axis movements
	if ((v = kc_joystick [17].value) < 255)
		if (kc_joystick [18].value)		// If inverted...
			Controls.sideways_thrust_time += joy_axis [v];
		else
			Controls.sideways_thrust_time -= joy_axis [v];
	if ((v = kc_joystick [19].value) < 255)
		if (kc_joystick [20].value)		// If inverted...
			Controls.vertical_thrust_time -= joy_axis [v];
		else
			Controls.vertical_thrust_time += joy_axis [v];
	if ((v = kc_joystick [21].value) < 255)
		if (kc_joystick [22].value)		// If inverted...
			Controls.bank_time += joy_axis [v];
		else
			Controls.bank_time -= joy_axis [v];
	if ((v = kc_joystick [23].value) < 255)
		if (kc_joystick [24].value)		// If inverted...
			Controls.forward_thrust_time += joy_axis [v];
		else
			Controls.forward_thrust_time -= joy_axis [v];

	// special continuous slide & bank handling
	if (*slide_on) {
		if ((v = kc_joystick [13].value) < 255)
			if (kc_joystick [14].value)		// If inverted...
				Controls.vertical_thrust_time -= joy_axis [v];
			else
				Controls.vertical_thrust_time += joy_axis [v];
		if ((v = kc_joystick [15].value) < 255)
			if (kc_joystick [16].value)		// If inverted...
				Controls.sideways_thrust_time -= joy_axis [v];
			else
				Controls.sideways_thrust_time += joy_axis [v];
		}
	else {
		if ((v = kc_joystick [13].value) < 255)
			if (kc_joystick [14].value)		// If inverted...
				Controls.pitch_time += DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
			else 
				Controls.pitch_time -= DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
		if (!*bank_on) {
			if ((v = kc_joystick [15].value) < 255)
				if (kc_joystick [16].value)		// If inverted...
					Controls.heading_time -= DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod); //kcFrameCount;
				else
					Controls.heading_time += DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod); // kcFrameCount;
			}
		}
	if (*bank_on) {
		if ((v = kc_joystick [15].value) < 255)
			if (kc_joystick [16].value)		// If inverted...
				Controls.bank_time += DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
			else
				Controls.bank_time -= DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
		}
	}
}

//------------------------------------------------------------------------------

void ControlsDoMouse (int *mouse_axis, int mouse_buttons, 
							 int *slide_on, int *bank_on, fix *pkp, fix *pkh, int *nCruiseSpeed, 
							 int bGetSlideBank)
{
	int	v, mouse_sens_mod = 8;

if (bGetSlideBank == 0) {
	if ((v = kc_mouse [5].value) < 255) 
		*slide_on |= mouse_buttons & (1 << v);
	if ((v = kc_mouse [10].value) < 255) 
		*bank_on |= mouse_buttons & (1 << v);
	return;
	}
// Buttons
if (bGetSlideBank == 2) {
	if ((v = kc_mouse [0].value) < 255) {
		Controls.fire_primary_state |= MouseButtonState (v);
		Controls.fire_primaryDownCount += MouseButtonDownCount (v);
		}
	if ((v = kc_mouse [1].value) < 255) {
		Controls.fire_secondary_state |= MouseButtonState (v);
		Controls.fire_secondaryDownCount += MouseButtonDownCount (v);
		}
	if ((v = kc_mouse [2].value) < 255) 
		Controls.forward_thrust_time += MouseButtonDownTime (v);
	if ((v = kc_mouse [3].value) < 255) 
		Controls.forward_thrust_time -= MouseButtonDownTime (v);
	if ((v = kc_mouse [4].value) < 255) 
		Controls.fire_flareDownCount += MouseButtonDownCount (v);
	if ((v = kc_mouse [6].value) < 255) 
		Controls.sideways_thrust_time -= MouseButtonDownTime (v);
	if ((v = kc_mouse [7].value) < 255) 
		Controls.sideways_thrust_time += MouseButtonDownTime (v);
	if ((v = kc_mouse [8].value) < 255) 
		Controls.vertical_thrust_time += MouseButtonDownTime (v);
	if ((v = kc_mouse [9].value) < 255) 
		Controls.vertical_thrust_time -= MouseButtonDownTime (v);
	if ((v = kc_mouse [11].value) < 255) 
		Controls.bank_time += MouseButtonDownTime (v);
	if ((v = kc_mouse [12].value) < 255) 
		Controls.bank_time -= MouseButtonDownTime (v);
	if ((v = kc_mouse [25].value) < 255) {
		Controls.rear_viewDownCount += MouseButtonDownCount (v);
		Controls.rear_view_down_state |= MouseButtonState (v);
		}
	if ((v = kc_mouse [26].value) < 255) 
		Controls.drop_bombDownCount += MouseButtonDownCount (v);
	if ((v = kc_mouse [27].value) < 255) 
		Controls.afterburner_state |= MouseButtonState (v);
	if (((v = kc_mouse [28].value) < 255) && MouseButtonState (v))
		Controls.cycle_primary_count += MouseButtonDownCount (v);
	if (((v = kc_mouse [29].value) < 255) && MouseButtonState (v))
		Controls.cycle_secondary_count += MouseButtonDownCount (v);
	if (((v = kc_mouse [30].value) < 255) && MouseButtonState (v))
		Controls.zoomDownCount += MouseButtonDownCount (v);
	// Axis movements
	if ((v = kc_mouse [17].value) < 255)
		if (kc_mouse [18].value)		// If inverted...
			Controls.sideways_thrust_time -= mouse_axis [v];
		else
			Controls.sideways_thrust_time += mouse_axis [v];
	if ((v = kc_mouse [19].value) < 255)
		if (kc_mouse [20].value)		// If inverted...
			Controls.vertical_thrust_time -= mouse_axis [v];
		else
			Controls.vertical_thrust_time += mouse_axis [v];
	if ((v = kc_mouse [21].value) < 255)
		if (kc_mouse [22].value)		// If inverted...
			Controls.bank_time -= mouse_axis [v];
		else
			Controls.bank_time += mouse_axis [v];
	if ((v = kc_mouse [23].value) < 255)
		if (kc_mouse [24].value)		// If inverted...
			Controls.forward_thrust_time += mouse_axis [v];
		else
			Controls.forward_thrust_time -= mouse_axis [v];
	// special continuous slide & bank handling
	if (*slide_on) {
		if ((v = kc_mouse [13].value) < 255)
			if (kc_mouse [14].value)		// If inverted...
				Controls.vertical_thrust_time += mouse_axis [v];
			else
				Controls.vertical_thrust_time -= mouse_axis [v];
		if ((v = kc_mouse [15].value) < 255)	
			if (kc_mouse [16].value)		// If inverted...
				Controls.sideways_thrust_time -= mouse_axis [v];
			else
				Controls.sideways_thrust_time += mouse_axis [v];
		}
	else {
		SDL_GetMouseState(&mouseData.x, &mouseData.y);
		if (gameOpts->input.bJoyMouse) {
			int	dx = mouseData.x - SWIDTH / 2;
			if (dx < 0)
				if (dx > -16)
					dx = 0;
				else
					dx += 16;
			else
				if (dx < 16)
					dx = 0;
				else
					dx -= 16;
			Controls.heading_time += (dx * gameOpts->input.mouseSensitivity [0]); // mouse_sens_mod;
			}
		else {
			if ((v = kc_mouse [13].value) < 255)
				if (kc_mouse [14].value)		// If inverted...
					Controls.pitch_time += (mouse_axis [v]*gameOpts->input.mouseSensitivity [1])/mouse_sens_mod;
				else
					Controls.pitch_time -= (mouse_axis [v]*gameOpts->input.mouseSensitivity [1])/mouse_sens_mod;
			}
		if (*bank_on) {
			if ((v = kc_mouse [15].value) < 255)
				if (kc_mouse [16].value)		// If inverted...
					Controls.bank_time -= (mouse_axis [v]*gameOpts->input.mouseSensitivity [2])/mouse_sens_mod;
				else
					Controls.bank_time += (mouse_axis [v]*gameOpts->input.mouseSensitivity [2])/mouse_sens_mod;
			}
		else {
			if (gameOpts->input.bJoyMouse) {
				int	dy = mouseData.y - SHEIGHT / 2;
				if (dy < 0)
					if (dy > -16)
						dy = 0;
					else
						dy += 16;
				else
					if (dy < 16)
						dy = 0;
					else
						dy -= 16;
				Controls.pitch_time += (dy * gameOpts->input.mouseSensitivity [1]); // mouse_sens_mod;
				}
			else {
				if ((v = kc_mouse [15].value) < 255 && mouse_axis [v])
					if (kc_mouse [16].value)		// If inverted...
						Controls.heading_time -= (mouse_axis [v]*gameOpts->input.mouseSensitivity [0])/mouse_sens_mod;
					else
						Controls.heading_time += (mouse_axis [v]*gameOpts->input.mouseSensitivity [0])/mouse_sens_mod;
				}
			}
		}
	}

if (gameStates.input.nMouseType == CONTROL_CYBERMAN)	{
	if (bGetSlideBank == 2) {
		Controls.vertical_thrust_time += MouseButtonDownTime (D2_MB_Z_UP)/2;
		Controls.vertical_thrust_time -= MouseButtonDownTime (D2_MB_Z_DOWN)/2;
		Controls.bank_time += MouseButtonDownTime (D2_MB_BANK_LEFT);
		Controls.bank_time -= MouseButtonDownTime (D2_MB_BANK_RIGHT);
		}
	if (*slide_on) {
		if (bGetSlideBank == 2) {
			Controls.vertical_thrust_time -= MouseButtonDownTime (D2_MB_PITCH_FORWARD);
			Controls.vertical_thrust_time += MouseButtonDownTime (D2_MB_PITCH_BACKWARD);
			Controls.sideways_thrust_time -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			Controls.sideways_thrust_time += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
			}
		}
	else if (bGetSlideBank == 1) {
		*pkp += MouseButtonDownTime (D2_MB_PITCH_FORWARD)/ (PH_SCALE*2);
		*pkp -= MouseButtonDownTime (D2_MB_PITCH_BACKWARD)/ (PH_SCALE*2);
		if (!*bank_on) {
			*pkh -= MouseButtonDownTime (D2_MB_HEAD_LEFT)/PH_SCALE;
			*pkh += MouseButtonDownTime (D2_MB_HEAD_RIGHT)/PH_SCALE;
			}
		}
	if (*bank_on) {
		if (bGetSlideBank == 2) {
			Controls.bank_time -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			Controls.bank_time += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
			}
		}
	}
}

//------------------------------------------------------------------------------

void ControlsDoSlideBank (int slide_on, int bank_on, fix kp, fix kh)
{
if (slide_on) 
	Controls.heading_time =
	Controls.pitch_time = 0;
else {
	if (kp == 0)
		Controls.pitch_time = 0;
	else {
		if (kp > 0) {
			if (Controls.pitch_time < 0)
				Controls.pitch_time = 0;
			}
		else {// kp < 0
			if (Controls.pitch_time > 0)
				Controls.pitch_time = 0;
			}
		Controls.pitch_time += kp;
		}
	if (!bank_on) {
		if (kh == 0)
			Controls.heading_time = 0;
		else {
			if (kh > 0) {
				if (Controls.heading_time < 0)
					Controls.heading_time = 0;
				}
			else {
				if (Controls.heading_time > 0)
					Controls.heading_time = 0;
				}
			Controls.heading_time += kh;
			}
		}
	}
}	

//------------------------------------------------------------------------------

int ControlsCapSampleRate (void)
{
//	DEFTIME
if (gameStates.limitFPS.bControls) {
		int		fps;
	// check elapsed time since last call to ControlsReadAll
	// if less than 25 ms (i.e. 40 fps) return
	if (gameOpts->legacy.bMouse) 
		gameStates.input.kcFrameTime = gameData.app.xFrameTime;
	else {
		kcFrameCount++;
		if (!gameStates.app.b40fpsTick)
			return 1;
		fps = (int) ((1000 + gameStates.app.nDeltaTime / 2) / gameStates.app.nDeltaTime);
		gameStates.input.kcFrameTime = fps ? (f1_0 + fps / 2) / fps : f1_0;
		}
	}
else
	gameStates.input.kcFrameTime = gameData.app.xFrameTime;
return 0;
}

//------------------------------------------------------------------------------

void ControlsResetControls (void)
{
fix ht = Controls.heading_time;
fix pt = Controls.pitch_time;
memset (&Controls, 0, sizeof (control_info));
//Controls.heading_time = ht;
//Controls.pitch_time = pt;
ControlsLimitTurnRate (gameOpts->input.bUseMouse && !gameStates.input.bCybermouseActive);
}

//------------------------------------------------------------------------------

void SetControlType (void)
{
if (gameStates.input.nMouseType < 0)
	gameStates.input.nMouseType = (gameConfig.nControlType == CONTROL_CYBERMAN) ? CONTROL_CYBERMAN : CONTROL_MOUSE;
if (gameStates.input.nJoyType < 0)
	gameStates.input.nJoyType = ((gameConfig.nControlType >= CONTROL_FLIGHTSTICK_PRO) && (gameConfig.nControlType <= CONTROL_GRAVIS_GAMEPAD)) ? gameConfig.nControlType : CONTROL_JOYSTICK;
if (gameOpts->input.bUseJoystick)
	gameConfig.nControlType = gameStates.input.nJoyType;
else if (gameOpts->input.bUseMouse)
	gameConfig.nControlType = gameStates.input.nMouseType;
else
	gameConfig.nControlType = CONTROL_NONE;
}

//------------------------------------------------------------------------------

int ControlsReadAll (void)
{
	int	i;
	int	slide_on, bank_on;
	int	bank_sens_mod;
	fix	kp, kh;
	fix	mouse_axis [3];
	int	mouse_buttons;
	int	bUseMouse, use_joystick;

Controls.vertical_thrust_time = 0;
Controls.sideways_thrust_time = 0;
Controls.forward_thrust_time = 0;
Controls.cycle_primary_count = 0;
Controls.cycle_secondary_count = 0;
Controls.toggle_icons_count = 0;
Controls.zoomDownCount = 0;
Controls.headlight_count = 0; 
Controls.fire_flareDownCount = 0;
Controls.drop_bombDownCount = 0;	
Controls.automapDownCount = 0;
Controls.rear_viewDownCount = 0;
gameStates.input.bControlsSkipFrame = 1;
if (ControlsCapSampleRate ())
	return 1;
gameStates.input.bControlsSkipFrame = 0;
ControlsResetControls ();
gameStates.input.bKeepSlackTime = 1;
bank_sens_mod = kcFrameCount;
mouse_buttons = 0;
bUseMouse = 0;
slide_on = 0;
bank_on = 0;
#ifdef FAST_EVENTPOLL
if (!gameOpts->legacy.bInput)
	event_poll (SDL_ALLEVENTS);	//why poll 2 dozen times in the following code when input polling calls all necessary input handlers anyway?
#endif

SetControlType ();
use_joystick = gameOpts->input.bUseJoystick && ControlsReadJoystick (joy_axis);
if (gameOpts->input.bUseMouse)
	if (gameStates.input.bCybermouseActive) {
//		ReadOWL (kc_external_control);
//		CybermouseAdjust ();
		} 
	else if (gameStates.input.nMouseType == CONTROL_CYBERMAN)
		bUseMouse = ControlsReadCyberman (mouse_axis, &mouse_buttons);
	else
		bUseMouse = ControlsReadMouse (mouse_axis, &mouse_buttons);
else {
	mouse_axis [0] =
	mouse_axis [1] =
	mouse_axis [2] = 0;
	mouse_buttons = 0;
	bUseMouse = 0;
	}

kp = kh = 0;
for (i = 0; i < 3; i++) {
	ControlsDoKeyboard (&slide_on, &bank_on, &kp, &kh, &gameStates.input.nCruiseSpeed, i);
	if (use_joystick)
		ControlsDoJoystick (&slide_on, &bank_on, &kp, &kh, &gameStates.input.nCruiseSpeed, i);
	if (bUseMouse)
		ControlsDoMouse (mouse_axis, mouse_buttons, &slide_on, &bank_on, &kp, &kh, &gameStates.input.nCruiseSpeed, i);
	if (i == 1)
		ControlsDoSlideBank (slide_on, bank_on, kp, kh);
	}
if (gameOpts->input.bUseHotKeys)
	ControlsDoD2XKeys (&slide_on, &bank_on, &kp, &kh, &gameStates.input.nCruiseSpeed, i);

if (gameStates.input.nCruiseSpeed > i2f (100)) 
	gameStates.input.nCruiseSpeed = i2f (100);
else if (gameStates.input.nCruiseSpeed < 0) 
	gameStates.input.nCruiseSpeed = 0;
if (!Controls.forward_thrust_time)
	Controls.forward_thrust_time = FixMul (gameStates.input.nCruiseSpeed,gameStates.input.kcFrameTime)/100;

#if 0 //LIMIT_CONTROLS_FPS
if (bank_sens_mod > 2) {
	Controls.bank_time *= 2;
	Controls.bank_time /= bank_sens_mod;
	}
#endif

//----------- Clamp values between -gameStates.input.kcFrameTime and gameStates.input.kcFrameTime
if (gameStates.input.kcFrameTime > F1_0) {
#if TRACE
	con_printf (1, "Bogus frame time of %.2f seconds\n", f2fl (gameStates.input.kcFrameTime));
#endif	
	gameStates.input.kcFrameTime = F1_0;
	}

KCCLAMP (Controls.vertical_thrust_time, gameStates.input.kcFrameTime);
KCCLAMP (Controls.sideways_thrust_time, gameStates.input.kcFrameTime);
KCCLAMP (Controls.forward_thrust_time, gameStates.input.kcFrameTime);
if (!ControlsLimitTurnRate (bUseMouse)) {
	KCCLAMP (Controls.heading_time, gameStates.input.kcFrameTime);
	KCCLAMP (Controls.pitch_time, gameStates.input.kcFrameTime);
	KCCLAMP (Controls.bank_time, gameStates.input.kcFrameTime);
	}
if (gameStates.render.nZoomFactor > F1_0) {
		int r = (gameStates.render.nZoomFactor * 100) / F1_0;

	Controls.heading_time = (Controls.heading_time * 100) / r;
	Controls.pitch_time = (Controls.pitch_time * 100) / r;
	Controls.bank_time = (Controls.bank_time * 100) / r;
#ifdef _DEBUG
	r = (int) ((double) gameStates.render.nZoomFactor * 100.0 / (double) F1_0);
	HUDMessage (0, "x %d.%02d", r / 100, r % 100);
#endif
	}
//	KCCLAMP (Controls.afterburner_time, gameStates.input.kcFrameTime);
#ifdef _DEBUG
if (keyd_pressed [KEY_DELETE])
	memset (&Controls, 0, sizeof (control_info));
#endif
gameStates.input.bKeepSlackTime = 0;
kcFrameCount = 0;
if (Controls.forward_thrust_time)
	Controls.forward_thrust_time = Controls.forward_thrust_time;
return 0;
}
#endif

//------------------------------------------------------------------------------

void ResetCruise (void)
{
gameStates.input.nCruiseSpeed=0;
}

//------------------------------------------------------------------------------

#if 0
void kconfig_center_headset ()
{
#ifndef WINDOWS
	if (vfx1_installed)
		vfx_center_headset ();
#endif
//	} else if (iglasses_headset_installed)	{
//	} else if (Victor_headset_installed)   {
//	} else {
//	}
}
#endif

//------------------------------------------------------------------------------

void CybermouseAdjust ()
 {
/*	if (gameData.multi.nLocalPlayer > -1)	{
		gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum].mtype.phys_info.flags &= (~PF_TURNROLL);	// Turn off roll when turning
		gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum].mtype.phys_info.flags &= (~PF_LEVELLING);	// Turn off leveling to nearest side.
		gameOpts->gameplay.bAutoLeveling = 0;

		if (kc_external_version > 0) {		
			vms_matrix tempm, ViewMatrix;
			vms_angvec * Kconfig_abs_movement;
			char * oem_message;
	
			Kconfig_abs_movement = (vms_angvec *) ((uint)kc_external_control + sizeof (control_info);
	
			if (Kconfig_abs_movement->p || Kconfig_abs_movement->b || Kconfig_abs_movement->h)	{
				VmAngles2Matrix (&tempm,Kconfig_abs_movement);
				VmMatMul (&ViewMatrix,&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum].orient,&tempm);
				gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].objnum].orient = ViewMatrix;		
			}
			oem_message = (char *) ((uint)Kconfig_abs_movement + sizeof (vms_angvec);
			if (oem_message [0] != '\0')
				HUDInitMessage (oem_message);
		}
	}*/

	Controls.pitch_time += FixMul (kc_external_control->pitch_time,gameData.app.xFrameTime);						
	Controls.vertical_thrust_time += FixMul (kc_external_control->vertical_thrust_time,gameData.app.xFrameTime);
	Controls.heading_time += FixMul (kc_external_control->heading_time,gameData.app.xFrameTime);
	Controls.sideways_thrust_time += FixMul (kc_external_control->sideways_thrust_time ,gameData.app.xFrameTime);
	Controls.bank_time += FixMul (kc_external_control->bank_time ,gameData.app.xFrameTime);
	Controls.forward_thrust_time += FixMul (kc_external_control->forward_thrust_time ,gameData.app.xFrameTime);
//	Controls.rear_viewDownCount += kc_external_control->rear_viewDownCount;	
//	Controls.rear_view_down_state |= kc_external_control->rear_view_down_state;	
	Controls.fire_primaryDownCount += kc_external_control->fire_primaryDownCount;
	Controls.fire_primary_state |= kc_external_control->fire_primary_state;
	Controls.fire_secondary_state |= kc_external_control->fire_secondary_state;
	Controls.fire_secondaryDownCount += kc_external_control->fire_secondaryDownCount;
	Controls.fire_flareDownCount += kc_external_control->fire_flareDownCount;
	Controls.drop_bombDownCount += kc_external_control->drop_bombDownCount;	
//	Controls.automapDownCount += kc_external_control->automapDownCount;
// 	Controls.automap_state |= kc_external_control->automap_state;
  } 

//------------------------------------------------------------------------------

char GetKeyValue (char key)
  {
#if TRACE
	con_printf (CON_DEBUG,"Returning %c!\n",kc_keyboard [ (int)key].value);
#endif
	return (kc_keyboard [ (int)key].value);
  }

//------------------------------------------------------------------------------

void FlushInput (void)
{
	int	b = gameOpts->legacy.bInput;

gameOpts->legacy.bInput = 1;
event_poll (SDL_ALLEVENTS);
gameOpts->legacy.bInput = b;
KeyFlush ();
mouse_flush ();
joy_flush ();
}

//------------------------------------------------------------------------------
//eof
