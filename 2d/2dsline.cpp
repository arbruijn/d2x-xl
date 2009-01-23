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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>

#include "inferno.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "error.h"
#include "ogl_defs.h"
#include "ogl_render.h"

//------------------------------------------------------------------------------

void gr_linear_darken (ubyte * dest, int darkeningLevel, int count, ubyte * fade_table) 
{
for (int i = 0; i <count; i++) {
	*dest = fade_table [*dest + darkeningLevel * 256];
	dest++;
	}
}

//------------------------------------------------------------------------------

static inline void gr_linear_stosd (ubyte * dest, tCanvasColor *color, uint nbytes) 
{
memset (dest, color->index, nbytes);
}

//------------------------------------------------------------------------------

void DrawScanLine (int x1, int x2, int y)
{
if ((MODE != BM_LINEAR) && (MODE != BM_OGL))
	return;
if (gameStates.render.grAlpha >= FADE_LEVELS) 
//	gr_linear_stosd (DATA + ROWSIZE * y + x1, &COLOR, x2 - x1 + 1);
	OglDrawLine (x1, y, x2, y, &COLOR);
else 
	gr_linear_darken (DATA + ROWSIZE * y + x1, (int) gameStates.render.grAlpha, x2 - x1 + 1, paletteManager.FadeTable ());
}

//------------------------------------------------------------------------------

void DrawScanLineClipped (int x1, int x2, int y)
{
if ((MODE != BM_LINEAR) && (MODE != BM_OGL))
	return;

if ((y < 0) || (y > MAXY)) 
	return;

if (x2 < x1) 
	x2 ^= x1 ^= x2;

if ((x1 > MAXX) || (x2 < MINX))
	return;

if (x1 < MINX) 
	x1 = MINX;
if (x2 > MAXX) 
	x2 = MAXX;

#if 1
DrawScanLine (x1, x2, y);
#else
if (gameStates.render.grAlpha >= FADE_LEVELS)
	gr_linear_stosd (DATA + ROWSIZE * y + x1, &COLOR, x2 - x1 + 1);
else
	gr_linear_darken (DATA + ROWSIZE * y + x1, int (gameStates.render.grAlpha), x2 - x1 + 1, paletteManager.FadeTable ());
#endif
}

//------------------------------------------------------------------------------

