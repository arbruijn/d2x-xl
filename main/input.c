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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: KConfig.c,v 1.27 2003/12/18 11:24:04 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

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

#define	JOYSTICK_READ_TIME	 (F1_0/40)		//	Read joystick at 40 Hz.

fix	LastReadTime = 0;
fix	joy_axis [JOY_MAX_AXES];

//------------------------------------------------------------------------------

fix Next_toggleTime [3]={0,0,0};

int AllowToToggle (int i)
{
  //used for keeping tabs of when its ok to toggle headlight,primary,and secondary
 
	if (Next_toggleTime [i] > gameData.time.xGame)
		if (Next_toggleTime [i] < gameData.time.xGame + (F1_0/8))	//	In case time is bogus, never wait > 1 second.
			return 0;

	Next_toggleTime [i] = gameData.time.xGame + (F1_0/8);

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
#ifndef RELEASE
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
		if ((ctime < 0) && (LastReadTime >= 0))
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
		time_down = gameData.time.xFrame;
		} 
	else
		upcount=1;
	}
else {
	if (btn==button) {
		state = 1;
		time_down = gameData.time.xFrame;
		downcount=1;
		}
	else
		upcount=1;
	}				
joy_set_btnValues (btn, state, time_down, downcount, upcount);
					
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
if (keyFlags (v))
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
	 gameStates.app.bNostalgia ||
	 COMPETITION ||
	 !(bUseMouse && EGI_FLAG (bMouseLook, 0, 1, 0))) {
	KCCLAMP (Controls.pitchTime, MAX_PITCH);
	KCCLAMP (Controls.headingTime, MAX_PITCH);
	}
KCCLAMP (Controls.bankTime, MAX_PITCH);
return 1;
}

//------------------------------------------------------------------------------

void KCToggleBomb (void)
{
int bomb = bLastSecondaryWasSuper [PROXIMITY_INDEX] ? PROXIMITY_INDEX : SMART_MINE_INDEX;
if ((gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) ||
	 (!gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX] &&
	  !gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [SMART_MINE_INDEX])) {
	DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
	HUDInitMessage (TXT_NOBOMBS);
	}
else if (!gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [bomb]) {
	DigiPlaySampleOnce (SOUND_BAD_SELECTION, F1_0);
	HUDInitMessage (TXT_NOBOMB_ANY, (bomb == SMART_MINE_INDEX)? TXT_SMART_MINES : TXT_PROX_BOMBS);
	}
else {
	bLastSecondaryWasSuper [PROXIMITY_INDEX] = !bLastSecondaryWasSuper [PROXIMITY_INDEX];
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
	int	i, v, pitchScale = (!(gameStates.app.bNostalgia || COMPETITION) && 
									 extraGameInfo [IsMultiGame].bFastPitch) ? 2 * PH_SCALE : 1;
	fix	kp = 0;
	int	speedFactor = gameStates.app.cheats.bTurboMode ? 2 : 1;
	static int key_signs [8] = {1,1,-1,-1,-1,-1,1,1};

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
		Controls.sideways_thrustTime -= DELTACTRL (10 + i, 2);
		Controls.sideways_thrustTime += DELTACTRL (12 + i, 2);
		Controls.vertical_thrustTime += DELTACTRL (14 + i, 2);
		Controls.vertical_thrustTime -= DELTACTRL (16 + i, 2);
		Controls.bankTime += DELTACTRL (20 + i, 1);
		Controls.bankTime -= DELTACTRL (22 + i, 1);
		Controls.forward_thrustTime += DELTACTRL (30 + i, 0);
		Controls.forward_thrustTime -= DELTACTRL (32 + i, 0);
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
	for (i = 0; i < 4; i++)
		if ((v = HaveKey (kc_keyboard, 38 + i)) < 255) 
			*nCruiseSpeed += key_signs [i] * FixDiv (speedFactor * KeyDownTime (v) * 5, gameStates.input.kcFrameTime);
	for (i = 0; i < 2; i++)
		if (((v = HaveKey (kc_keyboard, 42 + i)) < 255) && keyDownCount (v))
			*nCruiseSpeed = 0;
	}

// special slide & bank toggle handling
if (*slide_on) {
	if (bGetSlideBank == 2) {
		for (i = 0; i < 2; i++) {
			Controls.vertical_thrustTime += DELTACTRL (i, 2);
			Controls.vertical_thrustTime -= DELTACTRL (2 + i, 2);
			Controls.sideways_thrustTime -= DELTACTRL (4 + i, 2);
			Controls.sideways_thrustTime += DELTACTRL (6 + i, 2);
			}
		}
	}
else if (bGetSlideBank == 1) {
	for (i = 0; i < 4; i++)
		kp += key_signs [i] * DELTACTRL (i, 1) / pitchScale;
	if (!*bank_on)
		for (i = 4; i < 8; i++)
			*pkh += key_signs [i] * DELTACTRL (i, 1) / PH_SCALE;
	}
if (*bank_on) {
	if (bGetSlideBank == 2) {
		for (i = 4; i < 6; i++)
			Controls.bankTime += DELTACTRL (i, 1);
		for (i = 6; i < 8; i++)
			Controls.bankTime -= DELTACTRL (i, 1);
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
		Controls.forward_thrustTime += joy_get_button_downTime (v);
	if ((v = kc_joystick [3].value) < 255) 
		Controls.forward_thrustTime -= joy_get_button_downTime (v);
	if ((v = kc_joystick [4].value) < 255) 
		Controls.fire_flareDownCount += joy_get_button_down_cnt (v);
	if ((v = kc_joystick [6].value) < 255) 
		Controls.sideways_thrustTime -= joy_get_button_downTime (v);
	if ((v = kc_joystick [7].value) < 255) 
		Controls.sideways_thrustTime += joy_get_button_downTime (v);
	if ((v = kc_joystick [8].value) < 255) 
		Controls.vertical_thrustTime += joy_get_button_downTime (v);
	if ((v = kc_joystick [9].value) < 255) 
		Controls.vertical_thrustTime -= joy_get_button_downTime (v);
	if ((v = kc_joystick [11].value) < 255) 
		Controls.bankTime += joy_get_button_downTime (v);
	if ((v = kc_joystick [12].value) < 255) 
		Controls.bankTime -= joy_get_button_downTime (v);
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
			Controls.sideways_thrustTime += joy_axis [v];
		else
			Controls.sideways_thrustTime -= joy_axis [v];
	if ((v = kc_joystick [19].value) < 255)
		if (kc_joystick [20].value)		// If inverted...
			Controls.vertical_thrustTime -= joy_axis [v];
		else
			Controls.vertical_thrustTime += joy_axis [v];
	if ((v = kc_joystick [21].value) < 255)
		if (kc_joystick [22].value)		// If inverted...
			Controls.bankTime += joy_axis [v];
		else
			Controls.bankTime -= joy_axis [v];
	if ((v = kc_joystick [23].value) < 255)
		if (kc_joystick [24].value)		// If inverted...
			Controls.forward_thrustTime += joy_axis [v];
		else
			Controls.forward_thrustTime -= joy_axis [v];

	// special continuous slide & bank handling
	if (*slide_on) {
		if ((v = kc_joystick [13].value) < 255)
			if (kc_joystick [14].value)		// If inverted...
				Controls.vertical_thrustTime -= joy_axis [v];
			else
				Controls.vertical_thrustTime += joy_axis [v];
		if ((v = kc_joystick [15].value) < 255)
			if (kc_joystick [16].value)		// If inverted...
				Controls.sideways_thrustTime -= joy_axis [v];
			else
				Controls.sideways_thrustTime += joy_axis [v];
		}
	else {
		if ((v = kc_joystick [13].value) < 255)
			if (kc_joystick [14].value)		// If inverted...
				Controls.pitchTime += DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
			else 
				Controls.pitchTime -= DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
		if (!*bank_on) {
			if ((v = kc_joystick [15].value) < 255)
				if (kc_joystick [16].value)		// If inverted...
					Controls.headingTime -= DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod); //kcFrameCount;
				else
					Controls.headingTime += DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod); // kcFrameCount;
			}
		}
	if (*bank_on) {
		if ((v = kc_joystick [15].value) < 255)
			if (kc_joystick [16].value)		// If inverted...
				Controls.bankTime += DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
			else
				Controls.bankTime -= DeltaAxis (v); // (joy_axis [v] * 16 / joy_sens_mod);
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
		Controls.forward_thrustTime += MouseButtonDownTime (v);
	if ((v = kc_mouse [3].value) < 255) 
		Controls.forward_thrustTime -= MouseButtonDownTime (v);
	if ((v = kc_mouse [4].value) < 255) 
		Controls.fire_flareDownCount += MouseButtonDownCount (v);
	if ((v = kc_mouse [6].value) < 255) 
		Controls.sideways_thrustTime -= MouseButtonDownTime (v);
	if ((v = kc_mouse [7].value) < 255) 
		Controls.sideways_thrustTime += MouseButtonDownTime (v);
	if ((v = kc_mouse [8].value) < 255) 
		Controls.vertical_thrustTime += MouseButtonDownTime (v);
	if ((v = kc_mouse [9].value) < 255) 
		Controls.vertical_thrustTime -= MouseButtonDownTime (v);
	if ((v = kc_mouse [11].value) < 255) 
		Controls.bankTime += MouseButtonDownTime (v);
	if ((v = kc_mouse [12].value) < 255) 
		Controls.bankTime -= MouseButtonDownTime (v);
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
			Controls.sideways_thrustTime -= mouse_axis [v];
		else
			Controls.sideways_thrustTime += mouse_axis [v];
	if ((v = kc_mouse [19].value) < 255)
		if (kc_mouse [20].value)		// If inverted...
			Controls.vertical_thrustTime -= mouse_axis [v];
		else
			Controls.vertical_thrustTime += mouse_axis [v];
	if ((v = kc_mouse [21].value) < 255)
		if (kc_mouse [22].value)		// If inverted...
			Controls.bankTime -= mouse_axis [v];
		else
			Controls.bankTime += mouse_axis [v];
	if ((v = kc_mouse [23].value) < 255)
		if (kc_mouse [24].value)		// If inverted...
			Controls.forward_thrustTime += mouse_axis [v];
		else
			Controls.forward_thrustTime -= mouse_axis [v];
	// special continuous slide & bank handling
	if (*slide_on) {
		if ((v = kc_mouse [13].value) < 255)
			if (kc_mouse [14].value)		// If inverted...
				Controls.vertical_thrustTime += mouse_axis [v];
			else
				Controls.vertical_thrustTime -= mouse_axis [v];
		if ((v = kc_mouse [15].value) < 255)	
			if (kc_mouse [16].value)		// If inverted...
				Controls.sideways_thrustTime -= mouse_axis [v];
			else
				Controls.sideways_thrustTime += mouse_axis [v];
		}
	else {
		SDL_GetMouseState(&mouseData.x, &mouseData.y);
		if (!gameStates.app.bNostalgia && gameOpts->input.bJoyMouse) {
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
			Controls.headingTime += (dx * gameOpts->input.mouseSensitivity [0]); // mouse_sens_mod;
			}
		else {
			if ((v = kc_mouse [13].value) < 255)
				if (kc_mouse [14].value)		// If inverted...
					Controls.pitchTime += (mouse_axis [v]*gameOpts->input.mouseSensitivity [1])/mouse_sens_mod;
				else
					Controls.pitchTime -= (mouse_axis [v]*gameOpts->input.mouseSensitivity [1])/mouse_sens_mod;
			}
		if (*bank_on) {
			if ((v = kc_mouse [15].value) < 255)
				if (kc_mouse [16].value)		// If inverted...
					Controls.bankTime -= (mouse_axis [v]*gameOpts->input.mouseSensitivity [2])/mouse_sens_mod;
				else
					Controls.bankTime += (mouse_axis [v]*gameOpts->input.mouseSensitivity [2])/mouse_sens_mod;
			}
		else {
			if (!gameStates.app.bNostalgia && gameOpts->input.bJoyMouse) {
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
				Controls.pitchTime += (dy * gameOpts->input.mouseSensitivity [1]); // mouse_sens_mod;
				}
			else {
				if ((v = kc_mouse [15].value) < 255 && mouse_axis [v])
					if (kc_mouse [16].value)		// If inverted...
						Controls.headingTime -= (mouse_axis [v]*gameOpts->input.mouseSensitivity [0])/mouse_sens_mod;
					else
						Controls.headingTime += (mouse_axis [v]*gameOpts->input.mouseSensitivity [0])/mouse_sens_mod;
				}
			}
		}
	}

if (gameStates.input.nMouseType == CONTROL_CYBERMAN)	{
	if (bGetSlideBank == 2) {
		Controls.vertical_thrustTime += MouseButtonDownTime (D2_MB_Z_UP)/2;
		Controls.vertical_thrustTime -= MouseButtonDownTime (D2_MB_Z_DOWN)/2;
		Controls.bankTime += MouseButtonDownTime (D2_MB_BANK_LEFT);
		Controls.bankTime -= MouseButtonDownTime (D2_MB_BANK_RIGHT);
		}
	if (*slide_on) {
		if (bGetSlideBank == 2) {
			Controls.vertical_thrustTime -= MouseButtonDownTime (D2_MB_PITCH_FORWARD);
			Controls.vertical_thrustTime += MouseButtonDownTime (D2_MB_PITCH_BACKWARD);
			Controls.sideways_thrustTime -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			Controls.sideways_thrustTime += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
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
			Controls.bankTime -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			Controls.bankTime += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
			}
		}
	}
}

//------------------------------------------------------------------------------

void ControlsDoSlideBank (int slide_on, int bank_on, fix kp, fix kh)
{
if (slide_on) 
	Controls.headingTime =
	Controls.pitchTime = 0;
else {
	if (kp == 0)
		Controls.pitchTime = 0;
	else {
		if (kp > 0) {
			if (Controls.pitchTime < 0)
				Controls.pitchTime = 0;
			}
		else {// kp < 0
			if (Controls.pitchTime > 0)
				Controls.pitchTime = 0;
			}
		Controls.pitchTime += kp;
		}
	if (!bank_on) {
		if (kh == 0)
			Controls.headingTime = 0;
		else {
			if (kh > 0) {
				if (Controls.headingTime < 0)
					Controls.headingTime = 0;
				}
			else {
				if (Controls.headingTime > 0)
					Controls.headingTime = 0;
				}
			Controls.headingTime += kh;
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
		gameStates.input.kcFrameTime = gameData.time.xFrame;
	else {
		if (gameData.app.bGamePaused)
			GetSlowTick ();
		kcFrameCount++;
		if (!gameStates.app.b40fpsTick)
			return 1;
		fps = (int) ((1000 + gameStates.app.nDeltaTime / 2) / gameStates.app.nDeltaTime);
		gameStates.input.kcFrameTime = fps ? (f1_0 + fps / 2) / fps : f1_0;
		}
	}
else
	gameStates.input.kcFrameTime = gameData.time.xFrame;
return 0;
}

//------------------------------------------------------------------------------

void ControlsResetControls (void)
{
fix ht = Controls.headingTime;
fix pt = Controls.pitchTime;
memset (&Controls, 0, sizeof (control_info));
//Controls.headingTime = ht;
//Controls.pitchTime = pt;
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

Controls.vertical_thrustTime = 0;
Controls.sideways_thrustTime = 0;
Controls.forward_thrustTime = 0;
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
if (!Controls.forward_thrustTime)
	Controls.forward_thrustTime = FixMul (gameStates.input.nCruiseSpeed,gameStates.input.kcFrameTime)/100;

#if 0 //LIMIT_CONTROLS_FPS
if (bank_sens_mod > 2) {
	Controls.bankTime *= 2;
	Controls.bankTime /= bank_sens_mod;
	}
#endif

//----------- Clamp values between -gameStates.input.kcFrameTime and gameStates.input.kcFrameTime
if (gameStates.input.kcFrameTime > F1_0) {
#if TRACE
	con_printf (1, "Bogus frame time of %.2f seconds\n", f2fl (gameStates.input.kcFrameTime));
#endif	
	gameStates.input.kcFrameTime = F1_0;
	}

KCCLAMP (Controls.vertical_thrustTime, gameStates.input.kcFrameTime);
KCCLAMP (Controls.sideways_thrustTime, gameStates.input.kcFrameTime);
KCCLAMP (Controls.forward_thrustTime, gameStates.input.kcFrameTime);
if (!ControlsLimitTurnRate (bUseMouse)) {
	KCCLAMP (Controls.headingTime, gameStates.input.kcFrameTime);
	KCCLAMP (Controls.pitchTime, gameStates.input.kcFrameTime);
	KCCLAMP (Controls.bankTime, gameStates.input.kcFrameTime);
	}
if (gameStates.render.nZoomFactor > F1_0) {
		int r = (gameStates.render.nZoomFactor * 100) / F1_0;

	Controls.headingTime = (Controls.headingTime * 100) / r;
	Controls.pitchTime = (Controls.pitchTime * 100) / r;
	Controls.bankTime = (Controls.bankTime * 100) / r;
#ifdef _DEBUG
	r = (int) ((double) gameStates.render.nZoomFactor * 100.0 / (double) F1_0);
	HUDMessage (0, "x %d.%02d", r / 100, r % 100);
#endif
	}
//	KCCLAMP (Controls.afterburnerTime, gameStates.input.kcFrameTime);
#ifdef _DEBUG
if (keyd_pressed [KEY_DELETE])
	memset (&Controls, 0, sizeof (control_info));
#endif
gameStates.input.bKeepSlackTime = 0;
kcFrameCount = 0;
if (Controls.forward_thrustTime)
	Controls.forward_thrustTime = Controls.forward_thrustTime;
return 0;
}

//------------------------------------------------------------------------------

void ResetCruise (void)
{
gameStates.input.nCruiseSpeed=0;
}

//------------------------------------------------------------------------------

void CybermouseAdjust ()
 {
/*	if (gameData.multi.nLocalPlayer > -1)	{
		gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].mType.physInfo.flags &= (~PF_TURNROLL);	// Turn off roll when turning
		gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].mType.physInfo.flags &= (~PF_LEVELLING);	// Turn off leveling to nearest tSide.
		gameOpts->gameplay.bAutoLeveling = 0;

		if (kc_external_version > 0) {		
			vmsMatrix tempm, ViewMatrix;
			vmsAngVec * Kconfig_abs_movement;
			char * oem_message;
	
			Kconfig_abs_movement = (vmsAngVec *) ((uint)kc_external_control + sizeof (control_info);
	
			if (Kconfig_abs_movement->p || Kconfig_abs_movement->b || Kconfig_abs_movement->h)	{
				VmAngles2Matrix (&tempm,Kconfig_abs_movement);
				VmMatMul (&ViewMatrix,&gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].position.mOrient,&tempm);
				gameData.objs.objects [gameData.multi.players [gameData.multi.nLocalPlayer].nObject].position.mOrient = ViewMatrix;		
			}
			oem_message = (char *) ((uint)Kconfig_abs_movement + sizeof (vmsAngVec);
			if (oem_message [0] != '\0')
				HUDInitMessage (oem_message);
		}
	}*/

	Controls.pitchTime += FixMul (kc_external_control->pitchTime,gameData.time.xFrame);						
	Controls.vertical_thrustTime += FixMul (kc_external_control->vertical_thrustTime,gameData.time.xFrame);
	Controls.headingTime += FixMul (kc_external_control->headingTime,gameData.time.xFrame);
	Controls.sideways_thrustTime += FixMul (kc_external_control->sideways_thrustTime ,gameData.time.xFrame);
	Controls.bankTime += FixMul (kc_external_control->bankTime ,gameData.time.xFrame);
	Controls.forward_thrustTime += FixMul (kc_external_control->forward_thrustTime ,gameData.time.xFrame);
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
