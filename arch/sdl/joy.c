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

#include "joy.h"
#include "error.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "inferno.h"
#include "text.h"

extern int joybutton_text []; //from KConfig.c

char joy_present = 0;

int joy_deadzone [4] = {32767 / 10, 32767 / 10, 32767 / 10, 32767 / 10};	//10% of max. range

static struct joyinfo {
	int		n_axes;
	int		n_buttons;
	struct	joyaxis axes [MAX_JOYSTICKS * JOY_MAX_AXES];
	struct	joybutton buttons [MAX_JOYSTICKS * JOY_MAX_BUTTONS];
} Joystick;

struct sdl_joystick /*SDL_Joystick*/ SDL_Joysticks [MAX_JOYSTICKS];

//------------------------------------------------------------------------------

void joy_button_handler (SDL_JoyButtonEvent *jbe)
{
	int button;

	if ((jbe->which >= MAX_JOYSTICKS) || (jbe->button > MAX_BUTTONS_PER_JOYSTICK))
		return;
	button = SDL_Joysticks [jbe->which].button_map [jbe->button] + jbe->which * MAX_BUTTONS_PER_JOYSTICK;
	Joystick.buttons [button].state = jbe->state;
	switch (jbe->type) {
	case SDL_JOYBUTTONDOWN:
		Joystick.buttons [button].time_went_down
			= TimerGetFixedSeconds();
		Joystick.buttons [button].num_downs++;
		break;
	case SDL_JOYBUTTONUP:
		Joystick.buttons [button].num_ups++;
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
	hat = SDL_Joysticks [jhe->which].hat_map [jhe->hat] + jhe->which * MAX_BUTTONS_PER_JOYSTICK;
	//Save last state of the hat-button
	Joystick.buttons [hat  ].last_state = Joystick.buttons [hat  ].state;
	Joystick.buttons [hat+1].last_state = Joystick.buttons [hat+1].state;
	Joystick.buttons [hat+2].last_state = Joystick.buttons [hat+2].state;
	Joystick.buttons [hat+3].last_state = Joystick.buttons [hat+3].state;

	//get current state of the hat-button
	Joystick.buttons [hat  ].state = ((jhe->value & SDL_HAT_UP)>0);
	Joystick.buttons [hat+1].state = ((jhe->value & SDL_HAT_RIGHT)>0);
	Joystick.buttons [hat+2].state = ((jhe->value & SDL_HAT_DOWN)>0);
	Joystick.buttons [hat+3].state = ((jhe->value & SDL_HAT_LEFT)>0);

	//determine if a hat-button up or down event based on state and last_state
	for(hbi=0;hbi<4;hbi++)
	{
		if(	!Joystick.buttons [hat+hbi].last_state && Joystick.buttons [hat+hbi].state) //last_state up, current state down
		{
			Joystick.buttons [hat+hbi].time_went_down
				= TimerGetFixedSeconds();
			Joystick.buttons [hat+hbi].num_downs++;
		}
		else if(Joystick.buttons [hat+hbi].last_state && !Joystick.buttons [hat+hbi].state)  //last_state down, current state up
		{
			Joystick.buttons [hat+hbi].num_ups++;
		}
	}
}

//------------------------------------------------------------------------------

void joy_axis_handler(SDL_JoyAxisEvent *jae)
{
	int axis;

	if (jae->which >= MAX_JOYSTICKS)
		return;
	axis = SDL_Joysticks [jae->which].axis_map [jae->axis] + jae->which * MAX_AXES_PER_JOYSTICK;
	Joystick.axes [axis].value = jae->value;
	if (jae->which)
		axis = 0;
}

//------------------------------------------------------------------------------

int joy_init()
{
	int i,j,n;
	struct sdl_joystick	*jp = SDL_Joysticks;

	if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
#if TRACE	
		con_printf(CON_VERBOSE, "sdl-joystick: initialisation failed: %s.",SDL_GetError());
#endif
		return 0;
	}
	memset (&Joystick, 0, sizeof (Joystick));
	n = SDL_NumJoysticks();

#if TRACE	
	con_printf(CON_VERBOSE, "sdl-joystick: found %d joysticks\n", n);
#endif
	for (i = 0; (i < n) && (gameOpts->input.nJoysticks < MAX_JOYSTICKS); i++) {
#if TRACE	
		con_printf(CON_VERBOSE, "sdl-joystick %d: %s\n", i, SDL_JoystickName (i));
#endif
		jp->handle = SDL_JoystickOpen (i);
		if (jp->handle) {
			joy_present = 1;
			if((jp->n_axes = SDL_JoystickNumAxes (jp->handle)) > MAX_AXES_PER_JOYSTICK) {
				Warning (TXT_JOY_AXESNO, jp->n_axes, MAX_AXES_PER_JOYSTICK);
				jp->n_axes = MAX_AXES_PER_JOYSTICK;
				}

			if((jp->n_buttons = SDL_JoystickNumButtons (jp->handle)) > MAX_BUTTONS_PER_JOYSTICK) {
				Warning (TXT_JOY_BUTTONNO, jp->n_buttons, MAX_BUTTONS_PER_JOYSTICK);
				jp->n_buttons = MAX_BUTTONS_PER_JOYSTICK;
				}
			if((jp->n_hats = SDL_JoystickNumHats (jp->handle)) > MAX_HATS_PER_JOYSTICK) {
				Warning (TXT_JOY_HATNO, jp->n_hats, MAX_HATS_PER_JOYSTICK);
				jp->n_hats = MAX_HATS_PER_JOYSTICK;
				}
#if TRACE	
			con_printf(CON_VERBOSE, "sdl-joystick: %d axes\n", jp->n_axes);
			con_printf(CON_VERBOSE, "sdl-joystick: %d buttons\n", jp->n_buttons);
			con_printf(CON_VERBOSE, "sdl-joystick: %d hats\n", jp->n_hats);
#endif
			memset (&Joystick, 0, sizeof (Joystick));
			for (j = 0; j < jp->n_axes; j++)
				jp->axis_map [j] = Joystick.n_axes++;
			for (j = 0; j < jp->n_buttons; j++)
				jp->button_map [j] = Joystick.n_buttons++;
			for (j = 0; j < jp->n_hats; j++) {
				jp->hat_map [j] = Joystick.n_buttons;
				//a hat counts as four buttons
				joybutton_text [Joystick.n_buttons++] = i ? TNUM_HAT2_U : TNUM_HAT_U;
				joybutton_text [Joystick.n_buttons++] = i ? TNUM_HAT2_R : TNUM_HAT_R;
				joybutton_text [Joystick.n_buttons++] = i ? TNUM_HAT2_D : TNUM_HAT_D;
				joybutton_text [Joystick.n_buttons++] = i ? TNUM_HAT2_L : TNUM_HAT_L;
				}
			jp++;
			gameOpts->input.nJoysticks++;
			}
		else {
#if TRACE	
			con_printf(CON_VERBOSE, "sdl-joystick: initialization failed!\n");
#endif			
		}
#if TRACE	
		con_printf(CON_VERBOSE, "sdl-joystick: %d axes (total)\n", Joystick.n_axes);
		con_printf(CON_VERBOSE, "sdl-joystick: %d buttons (total)\n", Joystick.n_buttons);
#endif
	}
return joy_present;
}

//------------------------------------------------------------------------------

void joy_close()
{
	while (gameOpts->input.nJoysticks)
		SDL_JoystickClose(SDL_Joysticks [--gameOpts->input.nJoysticks].handle);
}

//------------------------------------------------------------------------------

void joy_get_pos(int *x, int *y)
{
	int axis [JOY_MAX_AXES];

	if (!gameOpts->input.nJoysticks) {
		*x=*y=0;
		return;
	}

	joystick_read_raw_axis (JOY_ALL_AXIS, axis);

	*x = joy_get_scaled_reading (axis [0], 0 );
	*y = joy_get_scaled_reading (axis [1], 1 );
}

//------------------------------------------------------------------------------

int joy_get_btns()
{
#if 0 // This is never used?
	int i, buttons = 0;
	for (i=0; i++; i<buttons) {
		switch (Joystick.buttons [i].state) {
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

int joy_get_button_down_cnt (int btn )
{
	int num_downs;

	if (!gameOpts->input.nJoysticks)
		return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	num_downs = Joystick.buttons [btn].num_downs;
	Joystick.buttons [btn].num_downs = 0;

	return num_downs;
}

//------------------------------------------------------------------------------

fix joy_get_button_downTime(int btn)
{
	fix time = F0_0;

	if (!gameOpts->input.nJoysticks)
		return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	switch (Joystick.buttons [btn].state) {
	case SDL_PRESSED:
		time = TimerGetFixedSeconds() - Joystick.buttons [btn].time_went_down;
		Joystick.buttons [btn].time_went_down = TimerGetFixedSeconds();
		break;
	case SDL_RELEASED:
		time = 0;
		break;
	}

	return time;
}

//------------------------------------------------------------------------------

unsigned long joystick_read_raw_axis (unsigned long mask, int * axis )
{
	int i;
	unsigned long channel_masks = 0;

if (!gameOpts->input.nJoysticks)
	return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
for (i = 0; i < JOY_MAX_AXES; i++)
	if (axis [i] = Joystick.axes [i].value) {
		channel_masks |= (1 << i);
		//Joystick.axes [i].value = 0;
		}
return channel_masks;
}

void joy_flush()
{
	int i;

	if (!gameOpts->input.nJoysticks)
		return;

	for (i = 0; i < MAX_JOYSTICKS * JOY_MAX_BUTTONS; i++) {
		Joystick.buttons [i].time_went_down = 0;
		Joystick.buttons [i].num_downs = 0;
	}

}

//------------------------------------------------------------------------------

int joy_get_button_state (int btn )
{
	if (!gameOpts->input.nJoysticks)
		return 0;

if(btn >= JOY_MAX_BUTTONS)
		return 0;
#ifndef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_JOYEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	return Joystick.buttons [btn].state;
}

//------------------------------------------------------------------------------

void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < JOY_MAX_AXES; i++) {
		axis_center [i] = Joystick.axes [i].center_val;
		axis_min [i] = Joystick.axes [i].min_val;
		axis_max [i] = Joystick.axes [i].max_val;
	}
}

//------------------------------------------------------------------------------

void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < JOY_MAX_AXES; i++) {
		Joystick.axes [i].center_val = axis_center [i];
		Joystick.axes [i].min_val = axis_min [i];
		Joystick.axes [i].max_val = axis_max [i];
	}
}

//------------------------------------------------------------------------------

int joy_get_scaled_reading (int raw, int axis_num )
{
#if 1
	return (raw + 128) / 256;
#else
	int d, x;

	raw -= Joystick.axes [axis_num].center_val;
	if (raw < 0)
		d = Joystick.axes [axis_num].center_val - Joystick.axes [axis_num].min_val;
	else if (raw > 0)
		d = Joystick.axes [axis_num].max_val - Joystick.axes [axis_num].center_val;
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

	d = (joy_deadzone [axis_num % 4]) * 6;
	if ((x > (-d)) && (x < d))
		x = 0;

	return x;
#endif
}

//------------------------------------------------------------------------------

void joy_set_slow_reading (int flag )
{
}

//------------------------------------------------------------------------------

int set_joy_deadzone (int nRelZone, int nAxis)
{
nAxis %= 4;
gameOpts->input.joyDeadZones [nAxis] = (nRelZone > 100) ? 100 : (nRelZone < 0) ? 0 : nRelZone;
return joy_deadzone [nAxis] = (32767 * gameOpts->input.joyDeadZones [nAxis] / 2) / 100;
}

//------------------------------------------------------------------------------
//eof
