/* $Id: canvas.c,v 1.5 2002/07/17 21:55:19 bradleyb Exp $ */
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

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"


#include "console.h"
#include "inferno.h"
#include "grdef.h"
#ifdef __DJGPP__
#include "modex.h"
#include "vesa.h"
#endif

grs_canvas * grdCurCanv = NULL;    //active canvas
grs_screen * grdCurScreen = NULL;  //active screen

//	-----------------------------------------------------------------------------

grs_canvas *GrCreateCanvas(int w, int h)
{
	grs_canvas *newCanv;
	
	newCanv = (grs_canvas *)d_malloc( sizeof(grs_canvas) );
	GrInitBitmapAlloc (&newCanv->cv_bitmap, BM_LINEAR, 0, 0, w, h, w, 0);

	newCanv->cv_color.index = 0;
	newCanv->cv_color.rgb = 0;
	newCanv->cv_drawmode = 0;
	newCanv->cv_font = NULL;
	newCanv->cv_font_fg_color.index = 0;
	newCanv->cv_font_fg_color.rgb = 0;
	newCanv->cv_font_bg_color.index = 0;
	newCanv->cv_font_bg_color.rgb = 0;
	return newCanv;
}

//	-----------------------------------------------------------------------------

grs_canvas *GrCreateSubCanvas(grs_canvas *canv, int x, int y, int w, int h)
{
	grs_canvas *newCanv;

newCanv = (grs_canvas *)d_malloc( sizeof(grs_canvas) );
GrInitSubBitmap (&newCanv->cv_bitmap, &canv->cv_bitmap, x, y, w, h);
newCanv->cv_color = canv->cv_color;
newCanv->cv_drawmode = canv->cv_drawmode;
newCanv->cv_font = canv->cv_font;
newCanv->cv_font_fg_color = canv->cv_font_fg_color;
newCanv->cv_font_bg_color = canv->cv_font_bg_color;
return newCanv;
}

//	-----------------------------------------------------------------------------

void GrInitCanvas(grs_canvas *canv, unsigned char * pixdata, int pixtype, int w, int h)
{
	int wreal;

canv->cv_color.index = 0;
canv->cv_color.rgb = 0;
canv->cv_drawmode = 0;
canv->cv_font = NULL;
canv->cv_font_fg_color.index = 0;
canv->cv_font_bg_color.index = 0;
#ifndef __DJGPP__
wreal = w;
#else
wreal = (pixtype == BM_MODEX) ? w / 4 : w;
#endif
GrInitBitmap (&canv->cv_bitmap, pixtype, 0, 0, w, h, wreal, pixdata, 0);
}

//	-----------------------------------------------------------------------------

void GrInitSubCanvas(grs_canvas *newCanv, grs_canvas *src, int x, int y, int w, int h)
{
	newCanv->cv_color = src->cv_color;
	newCanv->cv_drawmode = src->cv_drawmode;
	newCanv->cv_font = src->cv_font;
	newCanv->cv_font_fg_color = src->cv_font_fg_color;
	newCanv->cv_font_bg_color = src->cv_font_bg_color;

	GrInitSubBitmap (&newCanv->cv_bitmap, &src->cv_bitmap, x, y, w, h);
}

//	-----------------------------------------------------------------------------

void GrFreeCanvas(grs_canvas *canv)
{
	GrFreeBitmapData(&canv->cv_bitmap);
    d_free(canv);
}

//	-----------------------------------------------------------------------------

void GrFreeSubCanvas(grs_canvas *canv)
{
    d_free(canv);
}

//	-----------------------------------------------------------------------------

int gr_wait_for_retrace = 1;

void GrShowCanvas( grs_canvas *canv )
{
#ifdef __DJGPP__
	if (canv->cv_bitmap.bm_props.type == BM_MODEX )
		gr_modex_setstart( canv->cv_bitmap.bm_props.x, canv->cv_bitmap.bm_props.y, gr_wait_for_retrace );

	else if (canv->cv_bitmap.bm_props.type == BM_SVGA )
		gr_vesa_setstart( canv->cv_bitmap.bm_props.x, canv->cv_bitmap.bm_props.y );
#endif
		//	else if (canv->cv_bitmap.bm_props.type == BM_LINEAR )
		// Int3();		// Get JOHN!
		//gr_linear_movsd( canv->cv_bitmap.bm_texBuf, (void *)gr_video_memory, 320*200);
}

//	-----------------------------------------------------------------------------

void GrSetCurrentCanvas( grs_canvas *canv )
{
#if 0 //TRACE
con_printf (CON_DEBUG, 
	"grd_set_current_canvas (canv=%p, grdCurCanv=%p, grdCurScreen=%p)\n", 
	canv, grdCurCanv, grdCurScreen);
#endif
grdCurCanv = canv ? canv : &(grdCurScreen->sc_canvas);
#ifndef NO_ASM
gr_var_color = 
	(!grdCurCanv->cv_color_rgb && (grdCurCanv->cv_color >= 0) && (grdCurCanv->cv_color <= 255)) ? 
	grdCurCanv->cv_color : 0;
gr_var_bitmap = grdCurCanv->cv_bitmap.bm_texBuf;
gr_var_bwidth = grdCurCanv->cv_bitmap.bm_props.rowsize;
#endif
}

//	-----------------------------------------------------------------------------

void GrClearCanvas (unsigned int color)
{
if (TYPE == BM_OGL)
	GrSetColorRGBi (color);
else 
	if (color)
		GrSetColor (GrFindClosestColor (grdCurCanv->cv_bitmap.bm_palette, RGBA_RED (color), RGBA_GREEN (color), RGBA_BLUE (color)));
	else
		GrSetColor (TRANSPARENCY_COLOR);
GrRect (0, 0, GWIDTH-1, GHEIGHT-1);
}

//	-----------------------------------------------------------------------------

void GrSetColor(int color)
{
grdCurCanv->cv_color.index =color % 256;
grdCurCanv->cv_color.rgb = 0;
#ifndef NO_ASM
gr_var_color = color % 256;
#endif
}

//	-----------------------------------------------------------------------------

void GrSetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha)
{
grdCurCanv->cv_color.rgb = 1;
grdCurCanv->cv_color.color.red = red;
grdCurCanv->cv_color.color.green = green;
grdCurCanv->cv_color.color.blue = blue;
grdCurCanv->cv_color.color.alpha = alpha;
#ifndef NO_ASM
	gr_var_color = color % 256;
#endif
}

//	-----------------------------------------------------------------------------

void GrSetColorRGB15bpp (ushort c, ubyte alpha)
{
GrSetColorRGB (
	PAL2RGBA (((c >> 10) & 31) * 2), 
	PAL2RGBA (((c >> 5) & 31) * 2), 
	PAL2RGBA ((c & 31) * 2), 
	alpha);
}

//	-----------------------------------------------------------------------------

void GrFadeColorRGB (double dFade)
{
if (dFade && grdCurCanv->cv_color.rgb) {
	grdCurCanv->cv_color.color.red = (ubyte) (grdCurCanv->cv_color.color.red * dFade);
	grdCurCanv->cv_color.color.green = (ubyte) (grdCurCanv->cv_color.color.green * dFade);
	grdCurCanv->cv_color.color.blue = (ubyte) (grdCurCanv->cv_color.color.blue * dFade);
	grdCurCanv->cv_color.color.alpha = (ubyte) ((float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS * 255.0f);
	}
}

//	-----------------------------------------------------------------------------

//eof
