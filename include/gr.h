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

#define SM(w,h) ((((u_int32_t)w)<<16)+(((u_int32_t)h)&0xFFFF))
#define SM_W(m) (m>>16)
#define SM_H(m) (m&0xFFFF)

//=========================================================================
// System functions:
// setup and set mode. this creates a grsScreen structure and sets
// grdCurScreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

int GrInit(void);

// This function sets up the main gameData.render.screen.  It should be called whenever
// the video mode changes.
int GrInitScreen(int mode, int w, int h, int x, int y, int rowSize, ubyte *data);

int GrVideoModeOK(u_int32_t mode);
int GrSetMode(u_int32_t mode);

//-----------------------------------------------------------------------------
// These 4 functions actuall change screen colors.

extern void gr_pal_fade_out(ubyte * pal);
extern void gr_pal_fade_in(ubyte * pal);
extern void gr_pal_clear(void);
extern void gr_pal_setblock(int start, int number, ubyte * pal);
extern void gr_pal_getblock(int start, int number, ubyte * pal);

extern ubyte *gr_video_memory;

//shut down the 2d.  Restore the screen mode.
void _CDECL_ GrClose(void);

//=========================================================================

#include "bitmap.h"
//#include "canvas.h"

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until GrClose is called

void GrCopyPalette (ubyte *gr_palette, ubyte *pal, int size);

//=========================================================================
// Drawing functions:

//	-----------------------------------------------------------------------------
// Draw a polygon into the current canvas in the current color and drawmode.
// verts points to an ordered list of x, y pairs.  the polygon should be
// convex; a concave polygon will be handled in some reasonable manner, 
// but not necessarily shaded as a concave polygon. It shouldn't hang.
// probably good solution is to shade from minx to maxx on each scan line.
// int should really be fix
void DrawPixelClipped(int x, int y);
void DrawPixel(int x, int y);

#if 0
int gr_poly(int nverts, int *verts);
int gr_upoly(int nverts, int *verts);


// Draws a point into the current canvas in the current color and drawmode.
// Gets a pixel;
ubyte gr_gpixel(CBitmap * bitmap, int x, int y);
ubyte gr_ugpixel(CBitmap * bitmap, int x, int y);

// Draws a line into the current canvas in the current color and drawmode.
int GrLine(fix x0, fix y0, fix x1, fix y1);
int gr_uline(fix x0, fix y0, fix x1, fix y1);

// Draws an anti-aliased line into the current canvas in the current color and drawmode.
int gr_aaline(fix x0, fix y0, fix x1, fix y1);
int gr_uaaline(fix x0, fix y0, fix x1, fix y1);

void DrawScanLineClipped(int x1, int x2, int y);
void DrawScanLine(int x1, int x2, int y);

void RotateBitmap (CBitmap *bp, grsPoint *vertbuf, int lightValue);
void ScaleBitmap (CBitmap *bp, grsPoint *vertbuf, int orientation);

//===========================================================================
// Global variables
extern ubyte Test_bitmap_data[64*64];

extern uint FixDivide(uint x, uint y);

extern void gr_vesa_update(CBitmap * source1, CBitmap * dest, CBitmap * source2);

// Special effects
extern void gr_snow_out(int num_dots);

extern void TestRotateBitmap(void);
extern void RotateBitmap(CBitmap *bp, grsPoint *vertbuf, int lightValue);

extern ubyte grInverseTable [32*32*32];

extern ushort grPaletteSelector;
extern ushort grInverseTableSelector;
extern ushort grFadeTableSelector;

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
int GetSelector(void * address, int size, uint * selector);

// Assigns a selector to a bitmap. Returns 0 if successful.  BE SURE TO CHECK
// this return value since there is a limited number of selectors!!!!!!!
int GrBitmapAssignSelector(CBitmap * bmPp);

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

void GrMergeTextures(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures1(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures2(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures3(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);

void SaveScreenShot (ubyte *buf, int automapFlag);
void AutoScreenshot (void);

/*
 * currently SDL and OGL are the only things that supports toggling
 * fullgameData.render.screen.  otherwise add other checks to the #if -MPM
 */
#define GR_SUPPORTS_FULLSCREEN_TOGGLE

void ResetTextures (int bReload, int bGame);

typedef struct tScrSize {
	int	x, y, c;
} __pack__ tScrSize;

extern int curDrawBuffer;

char *ScrSizeArg (int x, int y);

class CDisplayModeInfo {
	public:
		int	dim;
		short	w, h;
		short	renderMethod;
		short	flags;
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

#define NUM_DISPLAY_MODES		int (displayModeInfo.Length ())
#define CUSTOM_DISPLAY_MODE	(NUM_DISPLAY_MODES - 1)
#define MAX_DISPLAY_MODE		(NUM_DISPLAY_MODES - 2)

int FindDisplayMode (int nScrSize);
int SetCustomDisplayMode (int w, int h, int bFullScreen);

#endif /* def _GR_H */
