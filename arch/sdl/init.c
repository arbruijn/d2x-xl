/* $Id: init.c,v 1.11 2003/01/15 02:42:41 btb Exp $ */
/*
 *
 * SDL architecture support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "text.h"
#include "event.h"
#include "error.h"
#include "args.h"
#include "digi.h"
#include "inferno.h"
#include "text.h"

extern void d_mouse_init();

void _CDECL_ sdl_close(void)
{
LogErr ("shutting down SDL\n");
#if 1//!defined (__unix__) || defined (_DEBUG)
SDL_Quit();
#endif
}

void arch_sdl_init()
{
#if defined(SDL_VIDEO) || defined(SDL_GL_VIDEO)
#	if 0 //done in gr.c::GrInit()
if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	Error(TXT_SDL_INIT_VIDEO,SDL_GetError());
	}
#	endif
#endif
#ifdef SDL_INPUT
if (!FindArg("-nomouse"))
	d_mouse_init();
#endif
//	if (!FindArg("-nosound"))
//		DigiInit();
atexit(sdl_close);
}
