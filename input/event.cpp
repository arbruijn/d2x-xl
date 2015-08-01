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

# define TO_EVENT_POLL	11 //ms
# if TO_EVENT_POLL
#  include <time.h>
# endif

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "descent.h"

extern void KeyHandler (SDL_KeyboardEvent *event);
extern void MouseButtonHandler (SDL_MouseButtonEvent *mbe);
extern void MouseMotionHandler (SDL_MouseMotionEvent *mme);
#ifndef USE_LINUX_JOY // stpohle - so we can choose at compile time..
extern void JoyButtonHandler (SDL_JoyButtonEvent *jbe);
extern void JoyHatHandler (SDL_JoyHatEvent *jhe);
extern void JoyAxisHandler (SDL_JoyAxisEvent *jae);
#endif

static int32_t initialised=0;

int32_t PollEvent (SDL_Event *event, uint32_t mask)
{
	SDL_PumpEvents();
	/* We can't return -1, just return 0 (no event) on error */
	return (SDL_PeepEvents(event, 1, SDL_GETEVENT, mask) > 0);
}


void event_poll(uint32_t mask)
{
	SDL_Event event;
#if TO_EVENT_POLL
	time_t t0 = SDL_GetTicks ();
	time_t _t = t0;
#endif

while (SDL_PollEvent (&event)) {
#if TO_EVENT_POLL
	if (!gameOpts->legacy.bInput)
		_t = SDL_GetTicks ();
#endif
	switch(event.type) {
		case SDL_KEYDOWN:
			KeyHandler (reinterpret_cast<SDL_KeyboardEvent*> (&event));
			break;
		case SDL_KEYUP:
			KeyHandler (reinterpret_cast<SDL_KeyboardEvent*> (&event));
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			MouseButtonHandler (reinterpret_cast<SDL_MouseButtonEvent*> (&event));
			break;
		case SDL_MOUSEMOTION:
			MouseMotionHandler (reinterpret_cast<SDL_MouseMotionEvent*> (&event));
			break;
#ifndef USE_LINUX_JOY       // stpohle - so we can choose at compile time..
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			JoyButtonHandler (reinterpret_cast<SDL_JoyButtonEvent*> (&event));
			break;
		case SDL_JOYAXISMOTION:
			JoyAxisHandler (reinterpret_cast<SDL_JoyAxisEvent*> (&event));
			break;
		case SDL_JOYHATMOTION:
			JoyHatHandler (reinterpret_cast<SDL_JoyHatEvent*> (&event));
			break;
		case SDL_JOYBALLMOTION:
			break;
#endif
		case SDL_QUIT:
			break;
		}
#if TO_EVENT_POLL
	if (!gameOpts->legacy.bInput && (_t - t0 >= TO_EVENT_POLL))
		break;
#endif
	}
}

int32_t event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}
