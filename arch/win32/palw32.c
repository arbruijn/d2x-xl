#include "gr.h"
#include "grdef.h"
#include "palette.h"

#include <ddraw.h>


//added on 7/11/99 by adb to fix palette problem
#undef min
static __inline int min(int x, int y) { return x < y ? x : y; }
//end additions - adb

static int last_r=0, last_g=0, last_b=0;

extern void Win32_DoSetPalette(PALETTEENTRY *rgpe);
extern void Win32_DoGetPalette(PALETTEENTRY *rgpe);
extern void Win32_MakePalVisible(void);

void GrPaletteStepUp( int r, int g, int b )
{
	int i;
	ubyte *p;
	int temp;

	PALETTEENTRY pe[256];

	if (gameStates.render.bPaletteFadedOut) return;

	if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

	last_r = r;
	last_g = g;
	last_b = b;

	p=grPalette;
	for (i=0; i<256; i++)
	{
		temp = (int)(*p++) + r;
		if (temp<0) temp=0;
			else if (temp>63) temp=63;
		pe[i].peRed = min(temp + gameData.render.nPaletteGamma, 63) * 4;
		temp = (int)(*p++) + g;
		if (temp<0) temp=0;
			else if (temp>63) temp=63;
		pe[i].peGreen = min(temp + gameData.render.nPaletteGamma, 63) * 4 ;
		temp = (int)(*p++) + b;
		if (temp<0) temp=0;
			else if (temp>63) temp=63;
		pe[i].peBlue = min(temp + gameData.render.nPaletteGamma, 63) * 4;
		pe[i].peFlags = 0;
	}
	Win32_DoSetPalette (pe);
}

void GrPaletteStepClear()
{
	int i;

	PALETTEENTRY pe[256];
	for (i=0;i<256;i++)
	{
		pe[i].peRed=pe[i].peGreen=pe[i].peBlue=pe[i].peFlags=0;
	}

	Win32_DoSetPalette (pe);

	gameStates.render.bPaletteFadedOut = 1;
}

void GrPaletteStepLoad( ubyte *pal )	
{
	int i, j;
	PALETTEENTRY pe[256];

	for (i=0; i<768; i++ ) {
		//gr_current_pal[i] = pal[i] + gameData.render.nPaletteGamma;
		//if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
		gr_current_pal[i] = pal[i];
	}
	for (i=0,j=0;j<256;j++)
	{
		pe[j].peRed = min(gr_current_pal[i++] + gameData.render.nPaletteGamma, 63) * 4;
		pe[j].peGreen= min(gr_current_pal[i++] + gameData.render.nPaletteGamma, 63) * 4;
		pe[j].peBlue = min(gr_current_pal[i++] + gameData.render.nPaletteGamma, 63) * 4;
		pe[j].peFlags = 0;
	}
	Win32_DoSetPalette (pe);

	gameStates.render.bPaletteFadedOut = 0;
}

int GrPaletteFadeOut(ubyte *pal, int nsteps, int allow_keys )	
{
	int i,j,k;
	fix fade_palette[768];
	fix fade_palette_delta[768];
	PALETTEENTRY pe[256];

	allow_keys  = allow_keys;

	if (gameStates.render.bPaletteFadedOut) return 0;

	if (pal==NULL) pal=gr_current_pal;

	for (i=0; i<768; i++ )
	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = i2f(pal[i]);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}

	for (j=0; j<nsteps; j++ )
	{
		for (i=0, k = 0; k < 256; k++)
		{
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);

			pe[k].peRed = min(f2i(fade_palette[i]), 63) * 4;
 			i++;
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);

			pe[k].peGreen = min(f2i(fade_palette[i]), 63) * 4;
 			i++;
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);

			pe[k].peBlue = min(f2i(fade_palette[i]), 63) * 4;
			pe[k].peFlags = 0;
 			i++;
		}

		Win32_DoSetPalette (pe);
		Win32_MakePalVisible ();
	}

	gameStates.render.bPaletteFadedOut = 1;
	return 0;
}

int GrPaletteFadeIn(ubyte *pal, int nsteps, int allow_keys)	
{
	int i,j, k;
//	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	PALETTEENTRY pe[256];

	allow_keys  = allow_keys;

	if (!gameStates.render.bPaletteFadedOut) return 0;

	if (pal==NULL) pal=gr_current_pal;


	for (i=0; i<768; i++ )	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i]) / nsteps;
	}

	for (j=0; j<nsteps; j++ )	{
		for (i=0, k = 0; k<256; k++ )	{
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);

			pe[k].peRed = min(f2i(fade_palette[i]), 63) * 4;
 			i++;
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);

			pe[k].peGreen = min(f2i(fade_palette[i]), 63) * 4;
 			i++;
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			if (fade_palette[i] > i2f(pal[i] + gameData.render.nPaletteGamma) )
				fade_palette[i] = i2f(pal[i] + gameData.render.nPaletteGamma);

			pe[k].peBlue = min(f2i(fade_palette[i]), 63) * 4;
			pe[k].peFlags = 0;
 			i++;
		}

		Win32_DoSetPalette (pe);
		//added on 7/11/99 by adb to allow fade in on non-palette displays
		Win32_MakePalVisible ();
		//end additions - adb
	}

	gameStates.render.bPaletteFadedOut = 0;
	return 0;
}

void GrPaletteRead(ubyte * pal)
{
	int i;
//        char c;
	PALETTEENTRY rgpe [256];
	Win32_DoGetPalette (rgpe);
	for (i = 0; i < 256; i++)
	{
		*pal++ = (rgpe[i].peRed) / 4;
		*pal++ = (rgpe[i].peGreen) / 4;
		*pal++ = (rgpe[i].peBlue) / 4;
	}
}
