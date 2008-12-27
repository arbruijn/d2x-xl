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
#include "bitmap.h"

#ifndef D1XD3D
void gr_upixel( int x, int y )
{
	switch (MODE)
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
		gr_vesa_pixel( COLOR, (uint)DATA + (uint)ROWSIZE * y + x);
		return;
#endif
	}
}
#endif

void gr_pixel( int x, int y )
{
	if ((x<0) || (y<0) || (x>=CCanvas::Current ()->Width ()) || (y>=CCanvas::Current ()->Height ())) return;
	gr_upixel (x, y);
}

#ifndef D1XD3D
inline void gr_bm_upixel( CBitmap * bmP, int x, int y, ubyte color )
{
	tCanvasColor c;
	switch (bmP->Mode ())
 {
	case BM_OGL:
		c.index = color;
		c.rgb = 0;
		OglUPixelC (bmP->Left () + x, bmP->Top () + y, &c);
		return;
	case BM_LINEAR:
		(*bmP) [bmP->RowSize () * y + x] = color;
		return;
#ifdef __DJGPP__
	case BM_MODEX:
		x += bmP->Left ();
		y += bmP->Top ();
		gr_modex_setplane( x & 3 );
		gr_video_memory[(bmP->RowSize () * y) + (x/4)] = color;
		return;
	case BM_SVGA:
		gr_vesa_pixel(color,(uint)bmP->Buffer () + (uint)bmP->RowSize () * y + x);
		return;
#endif
	}
}
#endif

void gr_bm_pixel( CBitmap * bmP, int x, int y, ubyte color )
{
	if ((x<0) || (y<0) || (x >= bmP->Width ()) || (y >= bmP->Height ())) return;
	gr_bm_upixel (bmP, x, y, color);
}


