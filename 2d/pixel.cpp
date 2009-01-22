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

//------------------------------------------------------------------------------

void DrawPixel (int x, int y)
{
if (MODE == BM_OGL)
	OglDrawPixel (x, y);
else if (MODE == BM_LINEAR)
	DATA [ROWSIZE * y + x] = (ubyte) COLOR.index;
}

//------------------------------------------------------------------------------

void DrawPixelClipped (int x, int y)
{
if (!CCanvas::Current ()->Clip (x, y))
	DrawPixel (x, y);
}

//------------------------------------------------------------------------------

void CBitmap::DrawPixel (int x, int y, ubyte color)
{
if (!Buffer () || Clip (x, y))
	return;
if (Mode () == BM_OGL) {
	tCanvasColor c;
	c.index = color;
	c.rgb = 0;
	OglDrawPixel (Left () + x, Top () + y, &c);
	}
else if (Mode () == BM_LINEAR) 
	Buffer () [RowSize () * y + x] = color;
}

//------------------------------------------------------------------------------

ubyte CBitmap::GetPixel (int x, int y)
{
if (!Buffer () || Clip (x, y))
	return 0;
return Buffer () [RowSize () * y + x];
}

//------------------------------------------------------------------------------
//eof
