/* $Id: timer.c,v 1.6 2003/02/21 04:08:48 btb Exp $ */
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

fix TimerGetApproxSeconds(void)
{
	return ApproxMSecToFSec(SDL_GetTicks());
}

#if 0//def _WIN32
QLONG TimerGetFixedSeconds(void)
{
	QLONG x, s, ms;
	QLONG tv_now = SDL_GetTicks();

	s = i2f (tv_now / 1000);
	ms = fixdiv (i2f (tv_now % 1000), i2f (1000));
	x = s | ms;
	if (x > (QLONG) 0xFFFFFFFF)
		_asm int 3;
	return x;
}
#else
fix TimerGetFixedSeconds(void)
{
	fix x;
	unsigned long tv_now = SDL_GetTicks();
	x=i2f(tv_now/1000) | fixdiv(i2f(tv_now % 1000),i2f(1000));
	return x;
}
#endif

void timer_delay(fix seconds)
{
	SDL_Delay(f2i(fixmul(seconds, i2f(1000))));
}
