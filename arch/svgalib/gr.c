/*
 * $Source: /cvs/cvsroot/d2x/arch/svgalib/gr.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001/10/25 08:25:34 $
 *
 * SVGALib video functions
 *
 * $Log: gr.c,v $
 * Revision 1.1  2001/10/25 08:25:34  bradleyb
 * Finished moving stuff to arch/blah.  I know, it's ugly, but It'll be easier to sync with d1x.
 *
 * Revision 1.2  2001/01/29 13:47:52  bradleyb
 * Fixed build, some minor cleanups.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vga.h>
#include <vgagl.h>
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#ifdef SVGALIB_INPUT
#include <vgamouse.h>
#endif

#include "gamefont.h"

int gameStates.gfx.bInstalled = 0;
int usebuffer;

extern void mouse_handler (int button, int dx, int dy, int dz, int drx, int dry, int drz);

GraphicsContext *physicalscreen, *screenbuffer;

void GrUpdate (0)
{
	if (usebuffer)
		gl_copyscreen(physicalscreen);
}

int GrSetMode(u_int32_t mode)
{
	unsigned int w, h;
	char vgamode[16];
	vga_modeinfo *modeinfo;
	int modenum, rowsize;
	void *framebuffer;
	
#ifdef NOGRAPH
	return 0;
#endif
	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);

	GrPaletteStepClear();

	sprintf(vgamode, "G%dx%dx256", w, h);
	modenum = vga_getmodenumber(vgamode);	
	vga_setmode(modenum);
#ifdef SVGALIB_INPUT
	mouse_seteventhandler(mouse_handler);
#endif
	modeinfo = vga_getmodeinfo(modenum);

	if (modeinfo->flags & CAPABLE_LINEAR)
	{
		usebuffer = 0;

		vga_setlinearaddressing();

		// Set up physical screen only
		gl_setcontextvga(modenum);
		physicalscreen = gl_allocatecontext();
		gl_getcontext(physicalscreen);
		screenbuffer = physicalscreen;

		framebuffer = physicalscreen->vbuf;
		rowsize = physicalscreen->bytewidth;
	}
	else
	{
		usebuffer = 1;

		// Set up the physical screen
		gl_setcontextvga(modenum);
		physicalscreen = gl_allocatecontext();
		gl_getcontext(physicalscreen);

		// Set up the virtual screen
		gl_setcontextvgavirtual(modenum);
		screenbuffer = gl_allocatecontext();
		gl_getcontext(screenbuffer);

		framebuffer = screenbuffer->vbuf;
		rowsize = screenbuffer->bytewidth;
	}

	memset(grdCurScreen, 0, sizeof(grs_screen);
	grdCurScreen->sc_mode = mode;
	grdCurScreen->sc_w = w;
	grdCurScreen->sc_h = h;
	grdCurScreen->sc_aspect = FixDiv(grdCurScreen->sc_w*3,grdCurScreen->sc_h*4);
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.x = 0;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.y = 0;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.w = w;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.h = h;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize = rowsize;
	grdCurScreen->sc_canvas.cv_bitmap.bm_props.type = BM_LINEAR;
	grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf = framebuffer;
	GrSetCurrentCanvas(NULL);
	
	//gamefont_choose_game_font(w,h);
	
	return 0;
}

int GrInit(void)
{
	int retcode;
	int mode = SM(320,200);

 	// Only do this function once!
	if (gameStates.gfx.bInstalled==1)
		return -1;
	MALLOC(grdCurScreen,grs_screen, 1);
	memset(grdCurScreen, 0, sizeof(grs_screen);
	
	vga_init();

	if ((retcode=GrSetMode(mode)))
		return retcode;
	
	grdCurScreen->sc_canvas.cv_color = 0;
	grdCurScreen->sc_canvas.cv_drawmode = 0;
	grdCurScreen->sc_canvas.cv_font = NULL;
	grdCurScreen->sc_canvas.cv_font_fg_color = 0;
	grdCurScreen->sc_canvas.cv_font_bg_color = 0;
	GrSetCurrentCanvas( &grdCurScreen->sc_canvas );

	gameStates.gfx.bInstalled = 1;
	atexit(GrClose);
	return 0;
}

void GrClose ()
{
	if (gameStates.gfx.bInstalled==1)
	{
		gameStates.gfx.bInstalled = 0;
		d_free(grdCurScreen);
		gl_freecontext(screenbuffer);
		gl_freecontext(physicalscreen);
	}
}

// Palette functions follow.

static int last_r=0, last_g=0, last_b=0;

void GrPaletteStepClear ()
{
	int colors[768];

	memset (colors, 0, 768 * sizeof(int);
	vga_setpalvec (0, 256, colors);

	gameStates.render.bPaletteFadedOut = 1;
}


void GrPaletteStepUp (int r, int g, int b)
{
	int i = 0;
	ubyte *p = grPalette;
	int temp;

	int colors[768];

	if (gameStates.render.bPaletteFadedOut) return;

	if ((r==last_r) && (g==last_g) && (b==last_b)) return;

	last_r = r;
	last_g = g;
	last_b = b;

	while (i < 768)
	{
		temp = (int)(*p++) + r + gameData.render.nPaletteGamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i++] = temp;
		temp = (int)(*p++) + g + gameData.render.nPaletteGamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i++] = temp;
		temp = (int)(*p++) + b + gameData.render.nPaletteGamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i++] = temp;
	}
	vga_setpalvec (0, 256, colors);
}

//added on 980913 by adb to fix palette problems
// need a min without side effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

void GrPaletteStepLoad (ubyte *pal)	
{
	int i;
	int colors[768];

	for (i = 0; i < 768; i++)
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
		colors[i] = (min(gr_current_pal[i] + gameData.render.nPaletteGamma, 63);
	}

	vga_setpalvec (0, 256, colors);

	gameStates.render.bPaletteFadedOut = 0;
}



int GrPaletteFadeOut (ubyte *pal, int nsteps, int allow_keys)
{
	int i, j;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	int fade_colors[768];

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
			c = f2i(fade_palette[i]);
			if (c > 63) c = 63;
			fade_colors[i] = c;
		}
		vga_setpalvec (0, 256, fade_colors);
	}

	gameStates.render.bPaletteFadedOut = 1;
	return 0;
}



int GrPaletteFadeIn (ubyte *pal, int nsteps, int allow_keys)
{
	int i, j;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	int fade_colors[768];

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
			c = f2i(fade_palette[i]);
			if (c > 63) c = 63;
			fade_colors[i] = c;
		}
		vga_setpalvec (0, 256, fade_colors);
	}

	gameStates.render.bPaletteFadedOut = 0;
	return 0;
}



void GrPaletteRead (ubyte *pal)
{
	int colors[768];
	int i;

	vga_getpalvec (0, 256, colors);

	for (i = 0; i < 768; i++)
		pal[i] = colors[i];
}
