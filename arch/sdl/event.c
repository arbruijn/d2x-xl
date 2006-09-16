/* $Id: event.c,v 1.12 2003/06/06 19:04:27 btb Exp $ */
/*
 *
 * SDL Event related stuff
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef FAST_EVENTPOLL
# define TO_EVENT_POLL	11 //ms
# if TO_EVENT_POLL
#  include <time.h>
# endif
#endif

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "inferno.h"

extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
#ifndef USE_LINUX_JOY // stpohle - so we can choose at compile time..
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_hat_handler(SDL_JoyHatEvent *jhe);
extern void joy_axis_handler(SDL_JoyAxisEvent *jae);
#endif

static int initialised=0;

int PollEvent (SDL_Event *event, unsigned long mask)
{
	SDL_PumpEvents();
	/* We can't return -1, just return 0 (no event) on error */
	return (SDL_PeepEvents(event, 1, SDL_GETEVENT, mask) > 0);
}

void quit_request();

void event_poll(Uint32 mask)
{
	SDL_Event event;
#if TO_EVENT_POLL
	time_t _t, t0 = SDL_GetTicks ();
#endif

/*
	if (!mask)
		mask = SDL_ALLEVENTS;
	else
		mask |= SDL_ACTIVEEVENTMASK | SDL_QUITMASK | SDL_SYSWMEVENTMASK;
	while (PollEvent(&event, mask)) {
*/
	while (SDL_PollEvent (&event)) {
#if TO_EVENT_POLL
		if (!gameOpts->legacy.bInput)
			_t = SDL_GetTicks ();
#endif
		switch(event.type) {
			case SDL_KEYDOWN:
				key_handler((SDL_KeyboardEvent *)&event);
				break;
			case SDL_KEYUP:
				key_handler((SDL_KeyboardEvent *)&event);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouse_button_handler((SDL_MouseButtonEvent *)&event);
				break;
			case SDL_MOUSEMOTION:
				mouse_motion_handler((SDL_MouseMotionEvent *)&event);
				break;
#ifndef USE_LINUX_JOY       // stpohle - so we can choose at compile time..
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				joy_button_handler((SDL_JoyButtonEvent *)&event);
				break;
			case SDL_JOYAXISMOTION:
				joy_axis_handler((SDL_JoyAxisEvent *)&event);
				break;
			case SDL_JOYHATMOTION:
				joy_hat_handler((SDL_JoyHatEvent *)&event);
				break;
			case SDL_JOYBALLMOTION:
				break;
#endif
			case SDL_QUIT: {
				quit_request();
			} break;
		}
#if TO_EVENT_POLL
	if (!gameOpts->legacy.bInput && (_t - t0 >= TO_EVENT_POLL))
		break;
#endif
	}
}

int event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}
