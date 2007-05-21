/* $Id: gr.c,v 1.15 2003/11/27 09:10:52 btb Exp $ */
/*
 *
 * SDL video functions.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#ifdef SDL_IMAGE
#include <SDL_image.h>
#endif

#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#include "menu.h"

//added 10/05/98 by Matt Mueller - make fullscreen mode optional
#include "args.h"

#ifdef _WIN32_WCE // should really be checking for "Pocket PC" somehow
# define LANDSCAPE
#endif

int sdl_videoFlags = SDL_SWSURFACE | SDL_HWPALETTE;
//end addition -MM

SDL_Surface *screen = NULL;
#ifdef LANDSCAPE
static SDL_Surface *real_screen = NULL, *screen2 = NULL;
#endif

int gameStates.gfx.bInstalled = 0;

//added 05/19/99 Matt Mueller - locking stuff
#ifdef GR_LOCK
#include "checker.h"
#ifdef TEST_GR_LOCK
int gr_testlocklevel=0;
#endif
inline void gr_dolock(const char *file,int line) {
	gr_dotestlock();
	if ( gr_testlocklevel==1 && SDL_MUSTLOCK(screen) ) {
#ifdef __CHECKER__
		chcksetwritable(screen.pixels,screen->w*screen->h*screen->format->BytesPerPixel);
#endif
		if ( SDL_LockSurface(screen) < 0 )Error("could not lock screen (%s:%i)\n",file,line);
	}
}
inline void gr_dounlock(void) {
	gr_dotestunlock();
	if (gr_testlocklevel==0 && SDL_MUSTLOCK(screen) ) {
		SDL_UnlockSurface(screen);
#ifdef __CHECKER__
		chcksetunwritable(screen.pixels,screen->w*screen->h*screen->format->BytesPerPixel);
#endif
	}
}
#endif
//end addition -MM

#ifdef LANDSCAPE
/* Create a new rotated surface for drawing */
SDL_Surface *CreateRotatedSurface(SDL_Surface *s)
{
#if 0
    return(SDL_CreateRGBSurface(s->flags, s->h, s->w,
    s->format->BitsPerPixel,
    s->format->Rmask,
    s->format->Gmask,
    s->format->Bmask,
    s->format->Amask);
#else
    return(SDL_CreateRGBSurface(s->flags, s->h, s->w, 8, 0, 0, 0, 0);
#endif
}

/* Used to copy the rotated scratch surface to the screen */
void BlitRotatedSurface(SDL_Surface *from, SDL_Surface *to)
{

    int bpp = from->format->BytesPerPixel;
    int w=from->w, h=from->h, pitch=to->pitch;
    int i,j;
    Uint8 *pfrom, *pto, *to0;

    SDL_LockSurface(from);
    SDL_LockSurface(to);
    pfrom=(Uint8 *)from->pixels;
    to0=(Uint8 *) to->pixels+pitch*(w-1);
    for (i=0; i<h; i++)
    {
        to0+=bpp;
        pto=to0;
        for (j=0; j<w; j++)
        {
            if (bpp==1) *pto=*pfrom;
            else if (bpp==2) *(Uint16 *)pto=*(Uint16 *)pfrom;
            else if (bpp==4) *(Uint32 *)pto=*(Uint32 *)pfrom;
            else if (bpp==3)
                {
                    pto[0]=pfrom[0];
                    pto[1]=pfrom[1];
                    pto[2]=pfrom[2];
                }
            pfrom+=bpp;
            pto-=pitch;
        }
    }
    SDL_UnlockSurface(from);
    SDL_UnlockSurface(to);
}
#endif

void GrPaletteStepClear(); // Function prototype for GrInit;


void GrUpdate (0)
{
	//added 05/19/99 Matt Mueller - locking stuff
//	gr_testunlock();
	//end addition -MM
#ifdef LANDSCAPE
	screen2 = SDL_DisplayFormat(screen);
	BlitRotatedSurface(screen2, real_screen);
	//SDL_SetColors(real_screen, screen->format->palette->colors, 0, 256);
	SDL_UpdateRect(real_screen, 0, 0, 0, 0);
	SDL_FreeSurface(screen2);
#else
	SDL_UpdateRect(screen, 0, 0, 0, 0);
#endif
}


int GrVideoModeOK(u_int32_t mode)
{
	int w, h;

	w = SM_W(mode);
	h = SM_H(mode);

	return SDL_VideoModeOK(w, h, 8, sdl_videoFlags) != 0;
}


extern int nCurrentVGAMode; // DPH: kludge - remove at all costs

int GrSetMode(u_int32_t mode)
{
	int w,h;

#ifdef NOGRAPH
	return 0;
#endif

if (mode<=0)
	return 0;
w=SM_W(mode);
h=SM_H(mode);
nCurrentVGAMode = mode;
if (screen) 
	GrPaletteStepClear();
SDL_WM_SetCaption(PACKAGE_STRING, "Descent II");
SDL_WM_SetIcon(IMG_ReadXPMFromArray(pixmap), NULL);
#ifdef LANDSCAPE
real_screen = SDL_SetVideoMode(h, w, 0, sdl_videoFlags);
screen = CreateRotatedSurface(real_screen);
#else
screen = SDL_SetVideoMode(w, h, 8, sdl_videoFlags);
#endif
if (!screen) {
	Error("Could not set %dx%dx8 video mode\n",w,h);
   exit(1);
   }
memset( grdCurScreen, 0, sizeof(grs_screen));
grdCurScreen->sc_mode = mode;
grdCurScreen->sc_w = w;
grdCurScreen->sc_h = h;
grdCurScreen->sc_aspect = FixDiv (grdCurScreen->sc_w * 3, grdCurScreen->sc_h * 4);
grdCurScreen->sc_canvas.cv_bitmap.bm_props.x = 0;
grdCurScreen->sc_canvas.cv_bitmap.bm_props.y = 0;
grdCurScreen->sc_canvas.cv_bitmap.bm_props.w = w;
grdCurScreen->sc_canvas.cv_bitmap.bm_props.h = h;
grdCurScreen->sc_canvas.cv_bitmap.bm_bpp = 1;
grdCurScreen->sc_canvas.cv_bitmap.bm_props.rowsize = screen->pitch;
grdCurScreen->sc_canvas.cv_bitmap.bm_props.nType = BM_LINEAR;
grdCurScreen->sc_canvas.cv_bitmap.bm_texBuf = (unsigned char *)screen->pixels;
GrSetCurrentCanvas (NULL);
SDL_ShowCursor (0);
return 0;
}

int GrCheckFullScreen(void)
{
	return (sdl_videoFlags & SDL_FULLSCREEN)?1:0;
}

int GrToggleFullScreen(void)
{
	sdl_videoFlags^=SDL_FULLSCREEN;
	SDL_WM_ToggleFullScreen(screen);
	return (sdl_videoFlags & SDL_FULLSCREEN)?1:0;
}

int GrInit(void)
{
 	// Only do this function once!
	if (gameStates.gfx.bInstalled==1)
		return -1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Error(TXT_SDL_INIT_VIDEO, SDL_GetError());
	}
	MALLOC( grdCurScreen,grs_screen,1 );
	memset( grdCurScreen, 0, sizeof(grs_screen));

//added 10/05/98 by Matt Mueller - make fullscreen mode optional
	if (FindArg("-fullscreen"))
	     sdl_videoFlags|=SDL_FULLSCREEN;
//end addition -MM
	//added 05/19/99 Matt Mueller - make HW surface optional
	if (FindArg("-hwsurface"))
	     sdl_videoFlags|=SDL_HWSURFACE;
	//end addition -MM

	grdCurScreen->sc_canvas.cv_color = 0;
	grdCurScreen->sc_canvas.cv_drawmode = 0;
	grdCurScreen->sc_canvas.cv_font = NULL;
	grdCurScreen->sc_canvas.cv_font_fg_color = 0;
	grdCurScreen->sc_canvas.cv_font_bg_color = 0;
	GrSetCurrentCanvas( &grdCurScreen->sc_canvas );

	gameStates.gfx.bInstalled = 1;
	// added on 980913 by adb to add cleanup
	//atexit(GrClose);
	// end changes by adb

	return 0;
}

void GrClose()
{
	if (gameStates.gfx.bInstalled==1)
	{
		gameStates.gfx.bInstalled = 0;
		D2_FREE(grdCurScreen);
	}
}

// Palette functions follow.

static int last_r=0, last_g=0, last_b=0;

void GrPaletteStepClear()
{
 SDL_Palette *palette;
 SDL_Color colors[256];
 int ncolors;

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 ncolors = palette->ncolors;
 memset(colors, 0, ncolors * sizeof(SDL_Color));

 SDL_SetColors(screen, colors, 0, 256);

 gameStates.render.bPaletteFadedOut = 1;
}


void GrPaletteStepUp( int r, int g, int b )
{
 int i;
 ubyte *p = grPalette;
 int temp;

 SDL_Palette *palette;
 SDL_Color colors[256];

 if (gameStates.render.bPaletteFadedOut) return;

 if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

 last_r = r;
 last_g = g;
 last_b = b;

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i=0; i<256; i++) {
   temp = (int)(*p++) + r + gameData.render.nPaletteGamma;
   if (temp<0) temp=0;
   else if (temp>63) temp=63;
   colors[i].r = temp * 4;
   temp = (int)(*p++) + g + gameData.render.nPaletteGamma;
   if (temp<0) temp=0;
   else if (temp>63) temp=63;
   colors[i].g = temp * 4;
   temp = (int)(*p++) + b + gameData.render.nPaletteGamma;
   if (temp<0) temp=0;
   else if (temp>63) temp=63;
   colors[i].b = temp * 4;
 }

 SDL_SetColors(screen, colors, 0, 256);
}

//added on 980913 by adb to fix palette problems
// need a min without tSide effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

void GrPaletteStepLoad( ubyte *pal )	
{
#if 0
 int i, j;
 SDL_Palette *palette;
 SDL_Color colors[256];

 for (i=0; i<768; i++ ) {
     gr_current_pal[i] = pal[i];
     if (gr_current_pal[i] > 63) 
     	gr_current_pal[i] = 63;
 }

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i = 0, j = 0; j < 256; j++) {
     //changed on 980913 by adb to fix palette problems
     colors[j].r = (min(gr_current_pal[i++] + gameData.render.nPaletteGamma, 63)) * 4;
     colors[j].g = (min(gr_current_pal[i++] + gameData.render.nPaletteGamma, 63)) * 4;
     colors[j].b = (min(gr_current_pal[i++] + gameData.render.nPaletteGamma, 63)) * 4;
     //end changes by adb
 }
 SDL_SetColors(screen, colors, 0, 256);
#endif
 gameStates.render.bPaletteFadedOut = 0;
}



int GrPaletteFadeOut(ubyte *pal, int nsteps, int allow_keys)
{
 int i, j, k;
 ubyte c;
 fix fade_palette[768];
 fix fade_palette_delta[768];

 SDL_Palette *palette;
 SDL_Color fade_colors[256];

 if (gameStates.render.bPaletteFadedOut) 
 	return 0;

#if 1 //ifndef NDEBUG
	if (gameStates.render.bDisableFades) {
		GrPaletteStepClear();
		return 0;
	}
#endif

 palette = screen->format->palette;
 if (palette == NULL) {
    return -1; // Display is not palettised
 }

 if (pal==NULL) pal=gr_current_pal;

 for (i=0; i<768; i++ )	{
     gr_current_pal[i] = pal[i];
     fade_palette[i] = i2f(pal[i]);
     fade_palette_delta[i] = fade_palette[i] / nsteps;
 }
 for (j=0; j<nsteps; j++ )	{
     for (i=0, k = 0; k<256; k++ )	{
         fade_palette[i] -= fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
            fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].r = c * 4;
         i++;

         fade_palette[i] -= fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
            fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].g = c * 4;
         i++;

         fade_palette[i] -= fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
            fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].b = c * 4;
         i++;
     }

  SDL_SetColors(screen, fade_colors, 0, 256);
 }

 gameStates.render.bPaletteFadedOut = 1;
 return 0;
}



int GrPaletteFadeIn(ubyte *pal, int nsteps, int allow_keys)
{
 int i, j, k, ncolors;
 ubyte c;
 fix fade_palette[768];
 fix fade_palette_delta[768];

 SDL_Palette *palette;
 SDL_Color fade_colors[256];

 if (!gameStates.render.bPaletteFadedOut) return 0;

#if 1 //ifndef NDEBUG
	if (gameStates.render.bDisableFades) {
		GrPaletteStepLoad(pal);
		return 0;
	}
#endif

 palette = screen->format->palette;

 if (palette == NULL) {
    return -1; // Display is not palettised
 }

 ncolors = palette->ncolors;

 for (i=0; i<768; i++ )	{
     gr_current_pal[i] = pal[i];
     fade_palette[i] = 0;
     fade_palette_delta[i] = i2f(pal[i]) / nsteps;
 }

 for (j=0; j<nsteps; j++ )	{
     for (i=0, k = 0; k<256; k++ )	{
         fade_palette[i] += fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
            fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].r = c * 4;
         i++;

         fade_palette[i] += fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
            fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].g = c * 4;
         i++;

         fade_palette[i] += fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
            fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].b = c * 4;
         i++;
     }

  SDL_SetColors(screen, fade_colors, 0, 256);
 }
 //added on 980913 by adb to fix palette problems
 GrPaletteStepLoad(pal);
 //end changes by adb

 gameStates.render.bPaletteFadedOut = 0;
 return 0;
}



void GrPaletteRead(ubyte * pal)
{
 SDL_Palette *palette;
 int i, j;

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i = 0, j=0; i < 256; i++) {
     pal[j++] = palette->colors[i].r / 4;
     pal[j++] = palette->colors[i].g / 4;
     pal[j++] = palette->colors[i].b / 4;
 }
}
