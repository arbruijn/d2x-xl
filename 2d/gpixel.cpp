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

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#ifdef __DJGPP__
#include "modex.h"
#include "vesa.h"
#endif

ubyte gr_ugpixel( CBitmap * bitmap, int x, int y )
{
#ifdef __DJGPP__
	switch(bitmap->props.nMode)
 {
	case BM_LINEAR:
#endif
		return bitmap->Buffer () [bitmap->RowSize () * y + x];
#ifdef __DJGPP__
	case BM_MODEX:
		x += bitmap->props.x;
		y += bitmap->props.y;
		gr_modex_setplane( x & 3 );
		return gr_video_memory[(bitmap->RowSize () * y) + (x/4)];
	case BM_SVGA:
	 {
		uint offset;
		offset = (uint)bitmap->Buffer () + (uint)bitmap->RowSize () * y + x;
		gr_vesa_setpage( offset >> 16 );
		return gr_video_memory[offset & 0xFFFF];
		}
	}
	return 0;
#endif
}

ubyte gr_gpixel( CBitmap * bitmap, int x, int y )
{
	if ((x<0) || (y<0) || (x>=bitmap->Width ()) || (y>=bitmap->Height ())) return 0;
#ifdef __DJGPP__
	switch(bitmap->props.nMode)
 {
	case BM_LINEAR:
#endif
		return bitmap->Buffer ()[ bitmap->RowSize ()*y + x ];
#ifdef __DJGPP__
	case BM_MODEX:
		x += bitmap->props.x;
		y += bitmap->props.y;
		gr_modex_setplane( x & 3 );
		return gr_video_memory[(bitmap->RowSize () * y) + (x/4)];
	case BM_SVGA:
	 {
		uint offset;
		offset = (uint)bitmap->Buffer () + (uint)bitmap->RowSize () * y + x;
		gr_vesa_setpage( offset >> 16 );
		return gr_video_memory[offset & 0xFFFF];
		}
	}
	return 0;
#endif
}
