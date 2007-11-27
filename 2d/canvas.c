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

gsrCanvas * grdCurCanv = NULL;    //active canvas
grsScreen * grdCurScreen = NULL;  //active screen

//	-----------------------------------------------------------------------------

gsrCanvas *GrCreateCanvas(int w, int h)
{
	gsrCanvas *newCanv;

	newCanv = (gsrCanvas *)D2_ALLOC( sizeof(gsrCanvas) );
	GrInitBitmapAlloc (&newCanv->cvBitmap, BM_LINEAR, 0, 0, w, h, w, 1);

	newCanv->cvColor.index = 0;
	newCanv->cvColor.rgb = 0;
	newCanv->cvDrawMode = 0;
	newCanv->cvFont = NULL;
	newCanv->cvFontFgColor.index = 0;
	newCanv->cvFontFgColor.rgb = 0;
	newCanv->cvFontBgColor.index = 0;
	newCanv->cvFontBgColor.rgb = 0;
	return newCanv;
}

//	-----------------------------------------------------------------------------

gsrCanvas *GrCreateSubCanvas(gsrCanvas *canv, int x, int y, int w, int h)
{
	gsrCanvas *newCanv;

newCanv = (gsrCanvas *)D2_ALLOC( sizeof(gsrCanvas) );
GrInitSubBitmap (&newCanv->cvBitmap, &canv->cvBitmap, x, y, w, h);
newCanv->cvColor = canv->cvColor;
newCanv->cvDrawMode = canv->cvDrawMode;
newCanv->cvFont = canv->cvFont;
newCanv->cvFontFgColor = canv->cvFontFgColor;
newCanv->cvFontBgColor = canv->cvFontBgColor;
return newCanv;
}

//	-----------------------------------------------------------------------------

void GrInitCanvas (gsrCanvas *canv, unsigned char * pixdata, int pixtype, int w, int h)
{
	int wreal;

canv->cvColor.index = 0;
canv->cvColor.rgb = 0;
canv->cvDrawMode = 0;
canv->cvFont = NULL;
canv->cvFontFgColor.index = 0;
canv->cvFontBgColor.index = 0;
#ifndef __DJGPP__
wreal = w;
#else
wreal = (pixtype == BM_MODEX) ? w / 4 : w;
#endif
GrInitBitmap (&canv->cvBitmap, pixtype, 0, 0, w, h, wreal, pixdata, 1);
}

//	-----------------------------------------------------------------------------

void GrInitSubCanvas(gsrCanvas *newCanv, gsrCanvas *src, int x, int y, int w, int h)
{
	newCanv->cvColor = src->cvColor;
	newCanv->cvDrawMode = src->cvDrawMode;
	newCanv->cvFont = src->cvFont;
	newCanv->cvFontFgColor = src->cvFontFgColor;
	newCanv->cvFontBgColor = src->cvFontBgColor;

	GrInitSubBitmap (&newCanv->cvBitmap, &src->cvBitmap, x, y, w, h);
}

//	-----------------------------------------------------------------------------

void GrFreeCanvas(gsrCanvas *canv)
{
if (canv) {
	GrFreeBitmapData (&canv->cvBitmap);
	D2_FREE(canv);
	}
}

//	-----------------------------------------------------------------------------

void GrFreeSubCanvas(gsrCanvas *canv)
{
if (canv)
    D2_FREE(canv);
}

//	-----------------------------------------------------------------------------

int gr_wait_for_retrace = 1;

void GrShowCanvas( gsrCanvas *canv )
{
#ifdef __DJGPP__
	if (canv->cvBitmap.bmProps.nType == BM_MODEX )
		gr_modex_setstart( canv->cvBitmap.bmProps.x, canv->cvBitmap.bmProps.y, gr_wait_for_retrace );

	else if (canv->cvBitmap.bmProps.nType == BM_SVGA )
		gr_vesa_setstart( canv->cvBitmap.bmProps.x, canv->cvBitmap.bmProps.y );
#endif
		//	else if (canv->cvBitmap.bmProps.nType == BM_LINEAR )
		// Int3();		// Get JOHN!
		//gr_linear_movsd( canv->cvBitmap.bmTexBuf, (void *)gr_video_memory, 320*200);
}

//	-----------------------------------------------------------------------------

void GrSetCurrentCanvas( gsrCanvas *canv )
{
#if 0 //TRACE
con_printf (CONDBG, 
	"grd_set_current_canvas (canv=%p, grdCurCanv=%p, grdCurScreen=%p)\n", 
	canv, grdCurCanv, grdCurScreen);
#endif
grdCurCanv = canv ? canv : &(grdCurScreen->scCanvas);
#ifndef NO_ASM
gr_var_color = 
	(!grdCurCanv->cv_color_rgb && (grdCurCanv->cvColor >= 0) && (grdCurCanv->cvColor <= 255)) ? 
	grdCurCanv->cvColor : 0;
gr_var_bitmap = grdCurCanv->cvBitmap.bmTexBuf;
gr_var_bwidth = grdCurCanv->cvBitmap.bmProps.rowSize;
#endif
}

//	-----------------------------------------------------------------------------

void GrClearCanvas (unsigned int color)
{
if (TYPE == BM_OGL)
	GrSetColorRGBi (color);
else 
	if (color)
		GrSetColor (GrFindClosestColor (grdCurCanv->cvBitmap.bmPalette, RGBA_RED (color), RGBA_GREEN (color), RGBA_BLUE (color)));
	else
		GrSetColor (TRANSPARENCY_COLOR);
GrRect (0, 0, GWIDTH-1, GHEIGHT-1);
}

//	-----------------------------------------------------------------------------

void GrSetColor(int color)
{
grdCurCanv->cvColor.index =color % 256;
grdCurCanv->cvColor.rgb = 0;
#ifndef NO_ASM
gr_var_color = color % 256;
#endif
}

//	-----------------------------------------------------------------------------

void GrSetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha)
{
grdCurCanv->cvColor.rgb = 1;
grdCurCanv->cvColor.color.red = red;
grdCurCanv->cvColor.color.green = green;
grdCurCanv->cvColor.color.blue = blue;
grdCurCanv->cvColor.color.alpha = alpha;
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
if (dFade && grdCurCanv->cvColor.rgb) {
	grdCurCanv->cvColor.color.red = (ubyte) (grdCurCanv->cvColor.color.red * dFade);
	grdCurCanv->cvColor.color.green = (ubyte) (grdCurCanv->cvColor.color.green * dFade);
	grdCurCanv->cvColor.color.blue = (ubyte) (grdCurCanv->cvColor.color.blue * dFade);
	//grdCurCanv->cvColor.color.alpha = (ubyte) ((float) gameStates.render.grAlpha / (float) GR_ACTUAL_FADE_LEVELS * 255.0f);
	}
}

//	-----------------------------------------------------------------------------

//eof
