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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _GR_H
#define _GR_H

#include "pstypes.h"
#include "fix.h"
//#include "palette.h"
#include "vecmat.h"

//-----------------------------------------------------------------------------

//these are control characters that have special meaning in the font code

#define CC_COLOR        1   //next char is new foreground color
#define CC_LSPACING     2   //next char specifies line spacing
#define CC_UNDERLINE    3   //next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S      "\x1"   //next char is new foreground color
#define CC_LSPACING_S   "\x2"   //next char specifies line spacing
#define CC_UNDERLINE_S  "\x3"   //next char is underlined

//-----------------------------------------------------------------------------

#define SM(w,h) ((((uint32_t)w)<<16)+(((uint32_t)h)&0xFFFF))
#define SM_W(m) (m>>16)
#define SM_H(m) (m&0xFFFF)

//=========================================================================
// System functions:
// setup and set mode. this creates a grsScreen structure and sets
// grdCurScreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

int32_t GrInit(void);

// This function sets up the main gameData.render.screen.  It should be called whenever
// the video mode changes.
int32_t GrInitScreen(int32_t mode, int32_t w, int32_t h, int32_t x, int32_t y, int32_t rowSize, uint8_t *data);

int32_t GrVideoModeOK(uint32_t mode);
int32_t GrSetMode(uint32_t mode);

//-----------------------------------------------------------------------------
// These 4 functions actuall change screen colors.

extern void gr_pal_fade_out(uint8_t * pal);
extern void gr_pal_fade_in(uint8_t * pal);
extern void gr_pal_clear(void);
extern void gr_pal_setblock(int32_t start, int32_t number, uint8_t * pal);
extern void gr_pal_getblock(int32_t start, int32_t number, uint8_t * pal);

extern uint8_t *gr_video_memory;

//shut down the 2d.  Restore the screen mode.
void _CDECL_ GrClose(void);

//=========================================================================

#include "bitmap.h"
//#include "canvas.h"

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until GrClose is called

void GrCopyPalette (uint8_t *gr_palette, uint8_t *pal, int32_t size);

//=========================================================================
// Drawing functions:

//	-----------------------------------------------------------------------------
// Draw a polygon into the current canvas in the current color and drawmode.
// verts points to an ordered list of x, y pairs.  the polygon should be
// convex; a concave polygon will be handled in some reasonable manner, 
// but not necessarily shaded as a concave polygon. It shouldn't hang.
// probably good solution is to shade from minx to maxx on each scan line.
// int32_t should really be fix
void DrawPixelClipped(int32_t x, int32_t y);
void DrawPixel(int32_t x, int32_t y);

#if 0
int32_t gr_poly(int32_t nverts, int32_t *verts);
int32_t gr_upoly(int32_t nverts, int32_t *verts);


// Draws a point into the current canvas in the current color and drawmode.
// Gets a pixel;
uint8_t gr_gpixel(CBitmap * bitmap, int32_t x, int32_t y);
uint8_t gr_ugpixel(CBitmap * bitmap, int32_t x, int32_t y);

// Draws a line into the current canvas in the current color and drawmode.
int32_t GrLine(fix x0, fix y0, fix x1, fix y1);
int32_t gr_uline(fix x0, fix y0, fix x1, fix y1);

// Draws an anti-aliased line into the current canvas in the current color and drawmode.
int32_t gr_aaline(fix x0, fix y0, fix x1, fix y1);
int32_t gr_uaaline(fix x0, fix y0, fix x1, fix y1);

void DrawScanLineClipped(int32_t x1, int32_t x2, int32_t y);
void DrawScanLine(int32_t x1, int32_t x2, int32_t y);

void RotateBitmap (CBitmap *bp, grsPoint *vertbuf, int32_t lightValue);
void ScaleBitmap (CBitmap *bp, grsPoint *vertbuf, int32_t orientation);

//===========================================================================
// Global variables
extern uint8_t Test_bitmap_data[64*64];

extern uint32_t FixDivide(uint32_t x, uint32_t y);

extern void gr_vesa_update(CBitmap * source1, CBitmap * dest, CBitmap * source2);

// Special effects
extern void gr_snow_out(int32_t num_dots);

extern void TestRotateBitmap(void);
extern void RotateBitmap(CBitmap *bp, grsPoint *vertbuf, int32_t lightValue);

extern uint8_t grInverseTable [32*32*32];

extern uint16_t grPaletteSelector;
extern uint16_t grInverseTableSelector;
extern uint16_t grFadeTableSelector;

// Remaps a bitmap into the current palette. If transparent_color is
// between 0 and 255 then all occurances of that color are mapped to
// whatever color the 2d uses for transparency. This is normally used
// right after a call to iff_read_bitmap like this:
//		iff_error = iff_read_bitmap(filename, new, BM_LINEAR, newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d", filename, iff_error);
//		if (iff_has_transparency)
//			GrRemapBitmap(new, newpal, iff_transparent_color);
//		else
//			GrRemapBitmap(new, newpal, -1);
// Allocates a selector that has a base address at 'address' and length 'size'.
// Returns 0 if successful... BE SURE TO CHECK the return value since there
// is a limited number of selectors available!!!
int32_t GetSelector(void * address, int32_t size, uint32_t * selector);

// Assigns a selector to a bitmap. Returns 0 if successful.  BE SURE TO CHECK
// this return value since there is a limited number of selectors!!!!!!!
int32_t GrBitmapAssignSelector(CBitmap * bmPp);

//#define GR_GETCOLOR(r, g, b) (gr_inverse_table [((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))])
//#define gr_getcolor(r, g, b) (gr_inverse_table [((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))])
//#define BM_XRGB(r, g, b) (gr_inverse_table [((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))])

#define BM_RGB(r, g, b) ((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))
//#define BM_XRGB(r, g, b) GrFindClosestColor((r)*2, (g)*2, (b)*2)
//#define GR_GETCOLOR(r, g, b) GrFindClosestColor((r)*2, (g)*2, (b)*2)
//#define gr_getcolor(r, g, b) GrFindClosestColor((r)*2, (g)*2, (b)*2)

// Given: r, g, b, each in range of 0-63, return the color index that
// best matches the input.
#endif

void GrMergeTextures(uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale);
void GrMergeTextures1(uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale);
void GrMergeTextures2(uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale);
void GrMergeTextures3(uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale);

void SaveScreenShot (uint8_t *buf, int32_t automapFlag);
void AutoScreenshot (void);

/*
 * currently SDL and OGL are the only things that supports toggling
 * fullgameData.render.screen.  otherwise add other checks to the #if -MPM
 */
#define GR_SUPPORTS_FULLSCREEN_TOGGLE

void ResetTextures (int32_t bReload, int32_t bGame);

typedef struct tScrSize {
	int32_t	x, y, c;
} __pack__ tScrSize;

extern int32_t curDrawBuffer;

char *ScrSizeArg (int32_t x, int32_t y);

class CDisplayModeInfo {
	public:
		int32_t	dim;
		int16_t	w, h;
		int16_t	renderMethod;
		int16_t	flags;
		char	bWideScreen;
		char	bFullScreen;
		char	bAvailable;

	inline bool operator< (CDisplayModeInfo& other) {
		if (bWideScreen < other.bWideScreen)
			return true;
		if (bWideScreen > other.bWideScreen)
			return false;
		if (w < other.w) 
			return true;
		if (w > other.w)
			return false;
		return h < other.h;
		}

	inline bool operator> (CDisplayModeInfo& other) {
		if (bWideScreen > other.bWideScreen)
			return true;
		if (bWideScreen < other.bWideScreen)
			return false;
		if (w > other.w) 
			return true;
		if (w < other.w)
			return false;
		return h > other.h;
		}

};

extern CArray<CDisplayModeInfo> displayModeInfo;

#define NUM_DISPLAY_MODES		int32_t (displayModeInfo.Length ())
#define CUSTOM_DISPLAY_MODE	(NUM_DISPLAY_MODES - 1)
#define MAX_DISPLAY_MODE		(NUM_DISPLAY_MODES - 2)

int32_t FindDisplayMode (int32_t nScrSize);
int32_t SetCustomDisplayMode (int32_t w, int32_t h, int32_t bFullScreen);

#endif /* def _GR_H */
