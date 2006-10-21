/* $Id: mouse.c,v 1.8 2003/11/27 09:24:43 btb Exp $ */
/*
 *
 * SDL mouse driver.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "fix.h"
#include "timer.h"
#include "event.h"
#include "mouse.h"
#include "inferno.h"

#ifdef WIN32_WCE
# define LANDSCAPE
#endif

#define Z_SENSITIVITY 100

mouseinfo mouseData;

//------------------------------------------------------------------------------

void d_mouse_init(void)
{
	memset(&mouseData,0,sizeof(mouseData));
}

//------------------------------------------------------------------------------

void mouse_button_handler(SDL_MouseButtonEvent *mbe)
{
	// to bad, SDL buttons use a different mapping as descent expects,
	// this is at least true and tested for the first three buttons 
	int button_remap[11] = {
		D2_MB_LEFT,
		D2_MB_MIDDLE,
		D2_MB_RIGHT,
		D2_MB_Z_UP,
		D2_MB_Z_DOWN,
		D2_MB_PITCH_BACKWARD,
		D2_MB_PITCH_FORWARD,
		D2_MB_BANK_LEFT,
		D2_MB_BANK_RIGHT,
		D2_MB_HEAD_LEFT,
		D2_MB_HEAD_RIGHT
	};

	int button = button_remap[mbe->button - 1]; // -1 since SDL seems to start counting at 1
	struct mousebutton *mb = mouseData.buttons + button;

	if (mbe->state == SDL_PRESSED) {
		mb->pressed = 1;
		mb->time_went_down = TimerGetFixedSeconds();
		mb->num_downs++;

		if (button == D2_MB_Z_UP) {
			mouseData.delta_z += Z_SENSITIVITY;
			mouseData.z += Z_SENSITIVITY;
			mb->rotated = 1;
			}
		else if (button == D2_MB_Z_DOWN) {
			mouseData.delta_z -= Z_SENSITIVITY;
			mouseData.z -= Z_SENSITIVITY;
			mb->rotated = 1;
		}
	} else {
		mb->pressed = 0;
		mb->time_held_down += TimerGetFixedSeconds() - mb->time_went_down;
		mb->num_ups++;
	}
}

//------------------------------------------------------------------------------

void mouse_motion_handler(SDL_MouseMotionEvent *mme)
{
#ifdef LANDSCAPE
	mouseData.delta_y += mme->xrel;
	mouseData.delta_x += mme->yrel;
	mouseData.y += mme->xrel;
	mouseData.x += mme->yrel;
#else
	if (mme->xrel || mme->yrel) {
		mouseData.delta_x += mme->xrel;
		mouseData.delta_y += mme->yrel;
		mouseData.x += mme->xrel;
		mouseData.y += mme->yrel;
		}
#endif
}

//------------------------------------------------------------------------------

void mouse_flush()	// clears all mice events...
{
	int i;
	fix currentTime;

	event_poll(SDL_MOUSEEVENTMASK);

	currentTime = TimerGetFixedSeconds();
	for (i=0; i<MOUSE_MAX_BUTTONS; i++) {
		mouseData.buttons[i].pressed=0;
		mouseData.buttons[i].time_went_down=currentTime;
		mouseData.buttons[i].time_held_down=0;
		mouseData.buttons[i].num_ups=0;
		mouseData.buttons[i].num_downs=0;
		mouseData.buttons[i].rotated=0;
	}
	mouseData.delta_x = 0;
	mouseData.delta_y = 0;
	mouseData.delta_z = 0;
	mouseData.x = 0;
	mouseData.y = 0;
	mouseData.z = 0;
	SDL_GetMouseState(&mouseData.x, &mouseData.y); // necessary because polling only gives us the delta.
}

//========================================================================
void mouse_get_pos( int *x, int *y )
{
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
#ifdef WIN32_WCE // needed here only for touchpens?
# ifdef LANDSCAPE
	SDL_GetMouseState(&mouseData.y, &mouseData.x);
# else
//	SDL_GetMouseState(&mouseData.x, &mouseData.y);
# endif
#endif
	*x=mouseData.x;
	*y=mouseData.y;
}

//------------------------------------------------------------------------------

void MouseGetDelta( int *dx, int *dy )
{
#ifdef FAST_EVENTPOLL
if (gameOpts->legacy.bInput)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#else
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	*dx = mouseData.delta_x;
	*dy = mouseData.delta_y;
	mouseData.delta_x = 0;
	mouseData.delta_y = 0;
}

//------------------------------------------------------------------------------

void MouseGetPosZ( int *x, int *y, int *z )
{
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	*x=mouseData.x;
	*y=mouseData.y;
	*z=mouseData.z;
}

//------------------------------------------------------------------------------

void MouseGetDeltaZ( int *dx, int *dy, int *dz )
{
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	*dx = mouseData.delta_x;
	*dy = mouseData.delta_y;
	*dz = mouseData.delta_z;
	mouseData.delta_x = 0;
	mouseData.delta_y = 0;
	mouseData.delta_z = 0;
}

//------------------------------------------------------------------------------

int MouseGetButtons()
{
	int i;
	uint flag = 1;
	int status = 0;

#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll (SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	for (i = 0; i < MOUSE_MAX_BUTTONS; i++) {
		if (mouseData.buttons [i].pressed || mouseData.buttons [i].rotated)
			status |= flag;
		flag <<= 1;
	}

	return status;
}

//------------------------------------------------------------------------------

void mouse_get_cyberman_pos( int *x, int *y )
{
}

//------------------------------------------------------------------------------
// Returns how many times this button has went down since last call
int MouseButtonDownCount(int button)
{
	int count;

#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	count = mouseData.buttons[button].num_downs;
	mouseData.buttons[button].num_downs = 0;

	return count;
}

//------------------------------------------------------------------------------
// Returns how long this button has been down since last call.
fix MouseButtonDownTime(int button)
{
	fix time_down, time;

#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
	if (!mouseData.buttons[button].pressed) {
		time_down = mouseData.buttons[button].time_held_down;
		if (!time_down && mouseData.buttons [button].rotated)
			time_down = (fix) (MouseButtonDownCount (button) * gameStates.input.kcFrameTime);
		mouseData.buttons[button].time_held_down = 0;
	} else {
		time = TimerGetFixedSeconds();
		time_down = time - mouseData.buttons[button].time_held_down;
		mouseData.buttons[button].time_held_down = time;
	}
	return time_down;
}

//------------------------------------------------------------------------------
// Returns 1 if this button is currently down
int MouseButtonState(int button)
{
	int h;

#ifndef FAST_EVENTPOLL
if (!bFastPoll)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#else
if (gameOpts->legacy.bInput)
   event_poll(SDL_MOUSEEVENTMASK);	//polled in main/KConfig.c:read_bm_all()
#endif
h = mouseData.buttons[button].rotated;
mouseData.buttons[button].rotated = 0;
return mouseData.buttons[button].pressed || h;
}

//------------------------------------------------------------------------------
