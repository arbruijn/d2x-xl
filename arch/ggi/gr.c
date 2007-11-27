/*
 * $Source: /cvs/cvsroot/d2x/arch/ggi/gr.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001/10/25 08:25:34 $
 *
 * Graphics functions for GGI.
 *
 * $Log: gr.c,v $
 * Revision 1.1  2001/10/25 08:25:34  bradleyb
 * Finished moving stuff to arch/blah.  I know, it's ugly, but It'll be easier to sync with d1x.
 *
 * Revision 1.3  2001/01/29 13:47:51  bradleyb
 * Fixed build, some minor cleanups.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <ggi/ggi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#include "console.h"

#include "gamefont.h"

int gameStates.gfx.bInstalled = 0;
void *screenbuffer;

ggi_visual_t *screenvis;
const ggi_directbuffer *dbuffer;
ggi_mode init_mode;

ubyte use_directbuffer;

void GrUpdate (0)
{
	ggiFlush(screenvis);
	if (!use_directbuffer)
		ggiPutBox(screenvis, 0, 0, grdCurScreen->scWidth, grdCurScreen->scHeight, screenbuffer);
}

int GrSetMode(u_int32_t mode)
{
	unsigned int w, h;
	ggi_mode other_mode;

#ifdef NOGRAPH
	return 0;
#endif
	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);


	GrPaletteStepClear();

	if(ggiCheckGraphMode(screenvis, w, h, GGI_AUTO, GGI_AUTO, GT_8BIT, &other_mode))
		ggiSetMode(screenvis, &other_mode);
	else
		ggiSetGraphMode(screenvis, w, h, GGI_AUTO, GGI_AUTO, GT_8BIT);

	ggiSetFlags(screenvis, GGIFLAG_ASYNC);
	
	if (!ggiDBGetNumBuffers(screenvis))
		use_directbuffer = 0;
	else
	{
		dbuffer = ggiDBGetBuffer(screenvis, 0);
		if (!(dbuffer->nType & GGI_DB_SIMPLE_PLB))
			use_directbuffer = 0;
		else
			use_directbuffer = 1;
	}

        memset(grdCurScreen, 0, sizeof(grsScreen);

	grdCurScreen->scMode = mode;
	grdCurScreen->scWidth = w;
	grdCurScreen->scHeight = h;
	grdCurScreen->scAspect = FixDiv(grdCurScreen->scWidth*3,grdCurScreen->scHeight*4);
	grdCurScreen->scCanvas.cvBitmap.bmProps.x = 0;
	grdCurScreen->scCanvas.cvBitmap.bmProps.y = 0;
	grdCurScreen->scCanvas.cvBitmap.bmProps.w = w;
	grdCurScreen->scCanvas.cvBitmap.bmProps.h = h;
	grdCurScreen->scCanvas.cvBitmap.bmProps.nType = BM_LINEAR;
	grdCurScreen->scCanvas.cvBitmap.bmBPP = 1;

	if (use_directbuffer)
	{
	        grdCurScreen->scCanvas.cvBitmap.bmTexBuf = dbuffer->write;
        	grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = dbuffer->buffer.plb.stride;
	}
	else
	{
		D2_FREE(screenbuffer);
		screenbuffer = D2_ALLOC (w * h);
		grdCurScreen->scCanvas.cvBitmap.bmTexBuf = screenbuffer;
		grdCurScreen->scCanvas.cvBitmap.bmProps.rowSize = w;
	}

	GrSetCurrentCanvas(NULL);

	//gamefont_choose_game_font(w,h);

	return 0;
}

int GrInit(void)
{
	int retcode;
	int mode = SM(648,480);
 	// Only do this function once!
	if (gameStates.gfx.bInstalled==1)
		return -1;
	MALLOC(grdCurScreen,grsScreen, 1);
	memset(grdCurScreen, 0, sizeof(grsScreen);

	ggiInit();
	screenvis = ggiOpen(NULL);
	ggiGetMode(screenvis, &init_mode);

	if ((retcode=GrSetMode(mode)))
		return retcode;

	grdCurScreen->scCanvas.cvColor = 0;
	grdCurScreen->scCanvas.cvDrawMode = 0;
	grdCurScreen->scCanvas.cvFont = NULL;
	grdCurScreen->scCanvas.cvFontFgColor = 0;
	grdCurScreen->scCanvas.cvFontBgColor = 0;
	GrSetCurrentCanvas( &grdCurScreen->scCanvas );

	gameStates.gfx.bInstalled = 1;
	atexit(GrClose);
	return 0;
}

void GrClose ()
{
	if (gameStates.gfx.bInstalled==1)
	{
	        ggiSetMode(screenvis, &init_mode);
		ggiClose(screenvis);
		ggiExit();
		gameStates.gfx.bInstalled = 0;
		D2_FREE(grdCurScreen);
	}
}

// Palette functions follow.

static int last_r=0, last_g=0, last_b=0;

void GrPaletteStepClear()
{
	ggi_color *colors = calloc (256, sizeof(ggi_color);

	ggiSetPalette (screenvis, 0, 256, colors);

	gameStates.render.bPaletteFadedOut = 1;
}


void GrPaletteStepUp (int r, int g, int b)
{
	int i;
	ubyte *p = grPalette;
	int temp;

	ggi_color colors[256];

	if (gameStates.render.bPaletteFadedOut) return;

	if ((r==last_r) && (g==last_g) && (b==last_b)) return;

	last_r = r;
	last_g = g;
	last_b = b;

	for (i = 0; i < 256; i++)
	{
		temp = (int)(*p++) + r + gameData.render.nPaletteGamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i].r = temp * 0x3ff;
		temp = (int)(*p++) + g + gameData.render.nPaletteGamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i].g = temp * 0x3ff;
		temp = (int)(*p++) + b + gameData.render.nPaletteGamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i].b = temp * 0x3ff;
	}
	ggiSetPalette (screenvis, 0, 256, colors);
}

//added on 980913 by adb to fix palette problems
// need a min without tSide effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

void GrPaletteStepLoad (ubyte *pal)
{
	int i, j;
	ggi_color colors[256];

	for (i = 0, j = 0; j < 256; j++)
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
		colors[j].r = (min(gr_current_pal[i] + gameData.render.nPaletteGamma, 63)) * 0x3ff;
		i++;
                gr_current_pal[i] = pal[i];
                if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
                colors[j].g = (min(gr_current_pal[i] + gameData.render.nPaletteGamma, 63)) * 0x3ff;
		i++;
                gr_current_pal[i] = pal[i];
                if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
                colors[j].b = (min(gr_current_pal[i] + gameData.render.nPaletteGamma, 63)) * 0x3ff;
		i++;
	}

	ggiSetPalette(screenvis, 0, 256, colors);

	gameStates.render.bPaletteFadedOut = 0;
}



int GrPaletteFadeOut (ubyte *pal, int nsteps, int allow_keys)
{
	int i, j, k;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	ggi_color fade_colors[256];

	if (gameStates.render.bPaletteFadedOut) return 0;

	if (pal==NULL) pal=gr_current_pal;

	for (i=0; i<768; i++)
	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = i2f(pal[i]);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}

	for (j=0; j<nsteps; j++)
	{
		for (i = 0; i < 768; i++)
		{
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma))
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
		}
		for (i = 0, k = 0; k < 256; k++)
		{
			c = f2i(fade_palette[i++]);
			if (c > 63) c = 63;
			fade_colors[k].r = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].g = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].b = c * 0x3ff;
		}
		ggiSetPalette (screenvis, 0, 256, fade_colors);
		GrUpdate (0);
	}

	gameStates.render.bPaletteFadedOut = 1;
	return 0;
}



int GrPaletteFadeIn (ubyte *pal, int nsteps, int allow_keys)
{
	int i, j, k;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	ggi_color fade_colors[256];

	if (!gameStates.render.bPaletteFadedOut) return 0;

	if (nsteps <= 0)
		nsteps = 1;

	for (i=0; i<768; i++)
	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i] + gameData.render.nPaletteGamma) / nsteps;
	}

 	for (j=0; j<nsteps; j++ )
	{
		for (i = 0; i < 768; i++ )
		{
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma))
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
		}
         	for (i = 0, k = 0; k < 256; k++)
		{
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].r = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].g = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].b = c * 0x3ff;
                }
                ggiSetPalette (screenvis, 0, 256, fade_colors);
		GrUpdate (0);
	}

	gameStates.render.bPaletteFadedOut = 0;
	return 0;
}



void GrPaletteRead (ubyte *pal)
{
	ggi_color colors[256];
	int i, j;

	ggiGetPalette (screenvis, 0, 256, colors);

	for (i = 0, j = 0; i < 256; i++)
	{
                pal[j++] = colors[i].r / 0x3ff;
                pal[j++] = colors[i].g / 0x3ff;
                pal[j++] = colors[i].b / 0x3ff;
	}
}
