/* $Id: pixel.c,v 1.5 2002/10/10 18:55:32 btb Exp $ */
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
/*
 *
 * Graphical routines for setting a pixel.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "vesa.h"
#include "modex.h"
#include "ogl_defs.h"
#include "gr.h"

#ifndef D1XD3D
void gr_upixel( int x, int y )
{
	switch (TYPE)
	{
	case BM_OGL:
		OglUPixelC(x,y, &COLOR);
		return;
	case BM_LINEAR:
		DATA [ROWSIZE*y+x] = (ubyte) COLOR.index;
		return;
#ifdef __DJGPP__
	case BM_MODEX:
		gr_modex_setplane( (x+XOFFSET) & 3 );
		gr_video_memory[(ROWSIZE * (y+YOFFSET)) + ((x+XOFFSET)>>2)] = COLOR.index;
		return;
	case BM_SVGA:
		gr_vesa_pixel( COLOR, (unsigned int)DATA + (unsigned int)ROWSIZE * y + x);
		return;
#endif
	}
}
#endif

void gr_pixel( int x, int y )
{
	if ((x<0) || (y<0) || (x>=GWIDTH) || (y>=GHEIGHT)) return;
	gr_upixel (x, y);
}

#ifndef D1XD3D
inline void gr_bm_upixel( grsBitmap * bm, int x, int y, unsigned char color )
{
	grsColor c;
	switch (bm->bmProps.nType)
	{
	case BM_OGL:
		c.index = color;
		c.rgb = 0;
		OglUPixelC(bm->bmProps.x + x, bm->bmProps.y + y, &c);
		return;
	case BM_LINEAR:
		bm->bmTexBuf[ bm->bmProps.rowSize*y+x ] = color;
		return;
#ifdef __DJGPP__
	case BM_MODEX:
		x += bm->bmProps.x;
		y += bm->bmProps.y;
		gr_modex_setplane( x & 3 );
		gr_video_memory[(bm->bmProps.rowSize * y) + (x/4)] = color;
		return;
	case BM_SVGA:
		gr_vesa_pixel(color,(unsigned int)bm->bmTexBuf + (unsigned int)bm->bmProps.rowSize * y + x);
		return;
#endif
	}
}
#endif

void gr_bm_pixel( grsBitmap * bm, int x, int y, unsigned char color )
{
	if ((x<0) || (y<0) || (x>=bm->bmProps.w) || (y>=bm->bmProps.h)) return;
	gr_bm_upixel (bm, x, y, color);
}


