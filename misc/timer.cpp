/*
 *
 * SDL library timer functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "maths.h"
#include "timer.h"

fix TimerGetApproxSeconds	(void)
{
return ApproxMSecToFSec (SDL_GetTicks ());
}

fix TimerGetFixedSeconds (void)
{
unsigned int ms = SDL_GetTicks ();
return secs2f (ms);
}

void TimerDelay (fix seconds)
{
SDL_Delay (X2I (FixMul (seconds, I2X (1000))));
}
