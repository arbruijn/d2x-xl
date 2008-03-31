/* $Id: joy.c,v 1.12 2003/04/12 00:11:46 btb Exp $ */
/*
 *
 * SDL joystick support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>   // for memset
#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "inferno.h"
#include "joy.h"
#include "error.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "text.h"

extern int joybutton_text []; //from KConfig.c

char bJoyPresent = 0;

int joyDeadzone [4] = {32767 / 10, 32767 / 10, 32767 / 10, 32767 / 10};	//10% of max. range

static struct tJoyInfo {
	int		n_axes;
	int		n_buttons;
	struct	joyaxis axes [MAX_JOYSTICKS * JOY_MAX_AXES];
	struct	joybutton buttons [MAX_JOYSTICKS * JOY_MAX_BUTTONS];
} joyInfo;

tSdlJoystick /*tSdlJoystick*/ sdlJoysticks [MAX_JOYSTICKS];

//------------------------------------------------------------------------------

void joy_button_handler (SDL_JoyButtonEvent *jbe)
{
	int button;

if ((jbe->which >= MAX_JOYSTICKS) || (jbe->button > MAX_BUTTONS_PER_JOYSTICK))
	return;
button = sdlJoysticks [jbe->which].button_map [jbe->button] + jbe->which * MAX_BUTTONS_PER_JOYSTICK;
joyInfo.buttons [button].state = jbe->state;
switch (jbe->type) {
	case SDL_JOYBUTTONDOWN:
		joyInfo.buttons [button].xTimeWentDown = TimerGetFixedSeconds();
		joyInfo.buttons [button].numDowns++;
		break;
	case SDL_JOYBUTTONUP:
		joyInfo.buttons [button].numUps++;
		break;
	}
}

//------------------------------------------------------------------------------

void joy_hat_handler (SDL_JoyHatEvent *jhe)
{
	int hat;
	int hbi;

	if (jhe->which >= MAX_JOYSTICKS)
		return;
	hat = sdlJoysticks [jhe->which].hat_map [jhe->hat] + jhe->which * MAX_BUTTONS_PER_JOYSTICK;
	//Save last state of the hat-button
	joyInfo.buttons [hat  ].lastState = joyInfo.buttons [hat  ].state;
	joyInfo.buttons [hat+1].lastState = joyInfo.buttons [hat+1].state;
	joyInfo.buttons [hat+2].lastState = joyInfo.buttons [hat+2].state;
	joyInfo.buttons [hat+3].lastState = joyInfo.buttons [hat+3].state;

	//get current state of the hat-button
	joyInfo.buttons [hat  ].state = ((jhe->value & SDL_HAT_UP)>0);
	joyInfo.buttons [hat+1].state = ((jhe->value & SDL_HAT_RIGHT)>0);
	joyInfo.buttons [hat+2].state = ((jhe->value & SDL_HAT_DOWN)>0);
	joyInfo.buttons [hat+3].state = ((jhe->value & SDL_HAT_LEFT)>0);

	//determine if a hat-button up or down event based on state and lastState
	for(hbi=0;hbi<4;hbi++)
	{
		if(	!joyInfo.buttons [hat+hbi].lastState && joyInfo.buttons [hat+hbi].state) //lastState up, current state down
		{
			joyInfo.buttons [hat+hbi].xTimeWentDown
				= TimerGetFixedSeconds();
			joyInfo.buttons [hat+hbi].numDowns++;
		}
		else if(joyInfo.buttons [hat+hbi].lastState && !joyInfo.buttons [hat+hbi].state)  //lastState down, current state up
		{
			joyInfo.buttons [hat+hbi].numUps++;
		}
	}
}

//------------------------------------------------------------------------------

void joy_axis_handler(SDL_JoyAxisEvent *jae)
{
	int axis;

	if (jae->which >= MAX_JOYSTICKS)
		return;
	axis = sdlJoysticks [jae->which].axis_map [jae->axis] + jae->which * MAX_AXES_PER_JOYSTICK;
	joyInfo.axes [axis].value = jae->value;
	if (jae->which)
		axis = 0;
}

//------------------------------------------------------------------------------

int JoyInit()
{
	int				i, j, n;
	tSdlJoystick	*joyP = sdlJoysticks;

	if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
#if TRACE
		con_printf(CON_VERBOSE, "sdl-joystick: initialisation failed: %s.",SDL_GetError());
#endif
		return 0;
	}
	memset (&joyInfo, 0, sizeof (joyInfo));
	n = SDL_NumJoysticks();

#if TRACE
	con_printf(CON_VERBOSE, "sdl-joystick: found %d joysticks\n", n);
#endif
	for (i = 0; (i < n) && (gameStates.input.nJoysticks < MAX_JOYSTICKS); i++) {
#if TRACE
		con_printf(CON_VERBOSE, "sdl-joystick %d: %s\n", i, SDL_JoystickName (i));
#endif
		joyP->handle = SDL_JoystickOpen (i);
		if (joyP->handle) {
			bJoyPresent = 1;
			if((joyP->n_axes = SDL_JoystickNumAxes (joyP->handle)) > MAX_AXES_PER_JOYSTICK) {
				Warning (TXT_JOY_AXESNO, joyP->n_axes, MAX_AXES_PER_JOYSTICK);
				joyP->n_axes = MAX_AXES_PER_JOYSTICK;
				}

			if((joyP->n_buttons = SDL_JoystickNumButtons (joyP->handle)) > MAX_BUTTONS_PER_JOYSTICK) {
				Warning (TXT_JOY_BUTTONNO, joyP->n_buttons, MAX_BUTTONS_PER_JOYSTICK);
				joyP->n_buttons = MAX_BUTTONS_PER_JOYSTICK;
				}
			if((joyP->n_hats = SDL_JoystickNumHats (joyP->handle)) > MAX_HATS_PER_JOYSTICK) {
				Warning (TXT_JOY_HATNO, joyP->n_hats, MAX_HATS_PER_JOYSTICK);
				joyP->n_hats = MAX_HATS_PER_JOYSTICK;
				}
#if TRACE
			con_printf(CON_VERBOSE, "sdl-joystick: %d axes\n", joyP->n_axes);
			con_printf(CON_VERBOSE, "sdl-joystick: %d buttons\n", joyP->n_buttons);
			con_printf(CON_VERBOSE, "sdl-joystick: %d hats\n", joyP->n_hats);
#endif
			memset (&joyInfo, 0, sizeof (joyInfo));
			for (j = 0; j < joyP->n_axes; j++)
				joyP->axis_map [j] = joyInfo.n_axes++;
			for (j = 0; j < joyP->n_buttons; j++)
				joyP->button_map [j] = joyInfo.n_buttons++;
			for (j = 0; j < joyP->n_hats; j++) {
				joyP->hat_map [j] = joyInfo.n_buttons;
				//a hat counts as four buttons
				joybutton_text [joyInfo.n_buttons++] = i ? TNUM_HAT2_U : TNUM_HAT_U;
				joybutton_text [joyInfo.n_buttons++] = i ? TNUM_HAT2_R : TNUM_HAT_R;
				joybutton_text [joyInfo.n_buttons++] = i ? TNUM_HAT2_D : TNUM_HAT_D;
				joybutton_text [joyInfo.n_buttons++] = i ? TNUM_HAT2_L : TNUM_HAT_L;
				}
			joyP++;
			gameStates.input.nJoysticks++;
			}
		else {
#if TRACE
			con_printf(CON_VERBOSE, "sdl-joystick: initialization failed!\n");
#endif		
		}
#if TRACE
		con_printf(CON_VERBOSE, "sdl-joystick: %d axes (total)\n", joyInfo.n_axes);
		con_printf(CON_VERBOSE, "sdl-joystick: %d buttons (total)\n", joyInfo.n_buttons);
#endif
	}
return bJoyPresent;
}

//------------------------------------------------------------------------------

void joy_close()
{
	while (gameStates.input.nJoysticks)
		SDL_JoystickClose(sdlJoysticks [--gameStates.input.nJoysticks].handle);
}

//------------------------------------------------------------------------------

void JoyGetPos(int *x, int *y)
{
	int axis [JOY_MAX_AXES];

	if (!gameStates.input.nJoysticks) {
		*x=*y=0;
		return;
	}

	JoyReadRawAxis (JOY_ALL_AXIS, axis);

	*x = JoyGetScaledReading (axis [0], 0 );
	*y = JoyGetScaledReading (axis [1], 1 );
}

//------------------------------------------------------------------------------

int JoyGetBtns()
{
#if 0 // This is never used?
	int i, buttons = 0;
	for (i=0; i++; i<buttons) {
		switch (joyInfo.buttons [i].state) {
		case SDL_PRESSED:
			buttons |= 1<<i;
			break;
		case SDL_RELEASED:
			break;
		}
	}
	return buttons;
#else
	return 0;
#endif
}

//------------------------------------------------------------------------------

int JoyGetButtonDownCnt (int nButton)
{
	int numDowns;

if (!gameStates.input.nJoysticks)
	return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll (SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
numDowns = joyInfo.buttons [nButton].numDowns;
joyInfo.buttons [nButton].numDowns = 0;
return numDowns;
}

//------------------------------------------------------------------------------

fix JoyGetButtonDownTime(int nButton)
{
	fix time = F0_0;

if (!gameStates.input.nJoysticks)
	return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
switch (joyInfo.buttons [nButton].state) {
	case SDL_PRESSED:
		time = TimerGetFixedSeconds() - joyInfo.buttons [nButton].xTimeWentDown;
		joyInfo.buttons [nButton].xTimeWentDown = TimerGetFixedSeconds();
		break;
	case SDL_RELEASED:
		time = 0;
		break;
	}
return time;
}

//------------------------------------------------------------------------------

unsigned int JoyReadRawAxis (unsigned int mask, int * axis )
{
	int i;
	unsigned int channel_masks = 0;

if (!gameStates.input.nJoysticks)
	return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
for (i = 0; i < JOY_MAX_AXES; i++)
	if ((axis [i] = joyInfo.axes [i].value)) {
		channel_masks |= (1 << i);
		//joyInfo.axes [i].value = 0;
		}
return channel_masks;
}

void JoyFlush()
{
	int i;

	if (!gameStates.input.nJoysticks)
		return;

	for (i = 0; i < MAX_JOYSTICKS * JOY_MAX_BUTTONS; i++) {
		joyInfo.buttons [i].xTimeWentDown = 0;
		joyInfo.buttons [i].numDowns = 0;
	}

}

//------------------------------------------------------------------------------

int JoyGetButtonState (int nButton )
{
	if (!gameStates.input.nJoysticks)
		return 0;

if(nButton >= JOY_MAX_BUTTONS)
		return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	return joyInfo.buttons [nButton].state;
}

//------------------------------------------------------------------------------

void JoyGetCalVals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < JOY_MAX_AXES; i++) {
		axis_center [i] = joyInfo.axes [i].center_val;
		axis_min [i] = joyInfo.axes [i].min_val;
		axis_max [i] = joyInfo.axes [i].max_val;
	}
}

//------------------------------------------------------------------------------

void JoySetCalVals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < JOY_MAX_AXES; i++) {
		joyInfo.axes [i].center_val = axis_center [i];
		joyInfo.axes [i].min_val = axis_min [i];
		joyInfo.axes [i].max_val = axis_max [i];
	}
}

//------------------------------------------------------------------------------

int JoyGetScaledReading (int raw, int axis_num )
{
#if 1
	return (raw + 128) / 256;
#else
	int d, x;

	raw -= joyInfo.axes [axis_num].center_val;
	if (raw < 0)
		d = joyInfo.axes [axis_num].center_val - joyInfo.axes [axis_num].min_val;
	else if (raw > 0)
		d = joyInfo.axes [axis_num].max_val - joyInfo.axes [axis_num].center_val;
	else
		d = 0;

	if (d)
		x = ((raw << 7) / d);
	else
		x = 0;

	if (x < -128)
		x = -128;
	if (x > 127)
		x = 127;

	d = (joyDeadzone [axis_num % 4]) * 6;
	if ((x > (-d)) && (x < d))
		x = 0;

	return x;
#endif
}

//------------------------------------------------------------------------------

void JoySetSlowReading (int flag )
{
}

//------------------------------------------------------------------------------

int JoySetDeadzone (int nRelZone, int nAxis)
{
nAxis %= 4;
gameOpts->input.joystick.deadzones [nAxis] = (nRelZone > 100) ? 100 : (nRelZone < 0) ? 0 : nRelZone;
return joyDeadzone [nAxis] = (32767 * gameOpts->input.joystick.deadzones [nAxis] / 2) / 100;
}

//------------------------------------------------------------------------------
//eof
