/* $Id: gr.h, v 1.23 2004/01/08 20:31:35 schaffner Exp $ */
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
#include "palette.h"

#ifdef MACDATA
#	define SWAP_0_255              // swap black and white
#	define DEFAULT_TRANSPARENCY_COLOR  0 // palette entry of transparency color -- 255 on the PC
#	define TRANSPARENCY_COLOR_STR  "0"
#else
/* #undef  SWAP_0_255 */        // no swapping for PC people
#	define DEFAULT_TRANSPARENCY_COLOR  255 // palette entry of transparency color -- 255 on the PC
#	define TRANSPARENCY_COLOR_STR  "255"
#endif /* MACDATA */

#define TRANSPARENCY_COLOR  gameData.render.transpColor // palette entry of transparency color -- 255 on the PC

#define SUPER_TRANSP_COLOR  254   // palette entry of super transparency color

#define GR_FADE_LEVELS 34
#define GR_ACTUAL_FADE_LEVELS 31

#define GWIDTH  grdCurCanv->cv_bitmap.bm_props.w
#define GHEIGHT grdCurCanv->cv_bitmap.bm_props.h
#define SWIDTH  (grdCurScreen->sc_w)
#define SHEIGHT (grdCurScreen->sc_h)

#define MAX_BMP_SIZE(width, height) (4 + ((width) + 2) * (height))

typedef struct _grs_point {
	fix x, y;
} grs_point;

typedef struct tRgbColord {
	double red;
	double green;
	double blue;
} tRgbColord;

typedef struct tRgbColorf {
	float red;
	float green;
	float blue;
} tRgbColorf;

typedef struct tRgbaColorf {
	float red;
	float green;
	float blue;
	float	alpha;
} tRgbaColorf;


typedef struct tRgbaColorb {
	ubyte	red, green, blue, alpha;
} tRgbaColorb;

typedef struct tRgbColorb {
	ubyte	red, green, blue;
} tRgbColorb;

typedef struct tRgbColors {
	short red, green, blue;
} tRgbColors;

//these are control characters that have special meaning in the font code

#define CC_COLOR        1   //next char is new foreground color
#define CC_LSPACING     2   //next char specifies line spacing
#define CC_UNDERLINE    3   //next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S      "\x1"   //next char is new foreground color
#define CC_LSPACING_S   "\x2"   //next char specifies line spacing
#define CC_UNDERLINE_S  "\x3"   //next char is underlined

#define BM_LINEAR   0
#define BM_MODEX    1
#define BM_SVGA     2
#define BM_RGB15    3   //5 bits each r, g, b stored at 16 bits
#define BM_SVGA15   4
#define BM_OGL      5

//@@// Define these modes for Gameplay too, since the game was developed under
//@@// DOS, we will adapt these modes to other systems thru rendering.
//@@#define SM_ORIGINAL		-1
//@@#define SM_320x200C     0
//@@#define SM_320x200U     1
//@@#define SM_320x240U     2
//@@#define SM_360x200U     3
//@@#define SM_360x240U     4
//@@#define SM_376x282U     5
//@@#define SM_320x400U     6
//@@#define SM_320x480U     7
//@@#define SM_360x400U     8
//@@#define SM_360x480U     9
//@@#define SM_360x360U     10
//@@#define SM_376x308U     11
//@@#define SM_376x564U     12
//@@#define SM_640x400V     13
//@@#define SM_640x480V     14
//@@#define SM_800x600V     15
//@@#define SM_1024x768V    16
//@@#define SM_640x480V15   17
//@@#define SM_800x600V15   18

#define SM(w, h) ((((u_int32_t)w)<<16)+(((u_int32_t)h)&0xFFFF))
#define SM_W(m) (m>>16)
#define SM_H(m) (m&0xFFFF)


#define BM_FLAG_TRANSPARENT         1
#define BM_FLAG_SUPER_TRANSPARENT   2
#define BM_FLAG_NO_LIGHTING         4
#define BM_FLAG_RLE                 8   // A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT           16  // This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG             32  // for bitmaps that RLE to > 255 per row (i.e. cockpits)
#define BM_FLAG_TGA						128

typedef struct _grs_bmProps {
	short   x, y;		// Offset from parent's origin
	short   w, h;		// width, height
	short   rowsize;	// unsigned char offset to next row
	sbyte	  nType;		// 0=Linear, 1=ModeX, 2=SVGA
	sbyte	  flags;			
} grs_bmProps;

typedef struct _grs_stdBmData {
	struct _grsBitmap	*bm_alt;
	struct _grsBitmap	*bm_mask;	//intended for supertransparency masks 
	struct _grsBitmap	*bm_parent;
} grs_stdBmData;

typedef struct _grs_altBmData {
	ubyte						bm_frameCount;
	struct _grsBitmap	*bm_frames;
	struct _grsBitmap	*bm_curFrame;
	int						bm_transparentFrames [4];
	int						bm_supertranspFrames [4];
} grs_altBmData;

#define BM_TYPE_STD		0
#define BM_TYPE_ALT		1
#define BM_TYPE_FRAME	2
#define BM_TYPE_MASK		4

typedef struct _grsBitmap {
	grs_bmProps		bm_props;
	ubyte				*bm_palette;
	unsigned char	*bm_texBuf;		// ptr to texture data...
											//   Linear = *parent+(rowsize*y+x)
											//   ModeX = *parent+(rowsize*y+x/4)
											//   SVGA = *parent+(rowsize*y+x)
	unsigned short	bm_handle;		//for application.  initialized to 0
	ubyte				bm_avgColor;	//  Average color of all pixels in texture map.
	tRgbColorb		bm_avgRGB;
	ubyte				bm_wallAnim :1;
	ubyte				bm_fromPog :1;
	ubyte				bmType :3;

	struct _ogl_texture	*glTexture;
	union {
		grs_stdBmData		std;
		grs_altBmData		alt;
		} bm_data;
} grsBitmap;

#define BM_FRAMECOUNT(_bmP)	((_bmP)->bm_data.alt.bm_frameCount)
#define BM_FRAMES(_bmP)			((_bmP)->bm_data.alt.bm_frames)
#define BM_CURFRAME(_bmP)		((_bmP)->bm_data.alt.bm_curFrame)
#define BM_OVERRIDE(_bmP)		((_bmP)->bm_data.std.bm_alt)
#define BM_MASK(_bmP)			((_bmP)->bm_data.std.bm_mask)
#define BM_PARENT(_bmP)			((_bmP)->bm_data.std.bm_parent)

static inline grsBitmap *BmCurFrame (grsBitmap *bmP)
{
return ((bmP->bmType == BM_TYPE_ALT) && BM_CURFRAME (bmP)) ? BM_CURFRAME (bmP) : bmP;
}

static inline grsBitmap *BmOverride (grsBitmap *bmP)
{
if (!bmP)
	return bmP;
if (bmP->bmType == BM_TYPE_STD) {
	if (!BM_OVERRIDE (bmP))
		return bmP;
	bmP = BM_OVERRIDE (bmP);
	}
return BmCurFrame (bmP);
}

//font structure
typedef struct _grs_font {
	short       ft_w;           // Width in pixels
	short       ft_h;           // Height in pixels
	short       ftFlags;       // Proportional?
	short       ft_baseline;    //
	ubyte       ft_minchar;     // First char defined by this font
	ubyte       ft_maxchar;     // Last char defined by this font
	short       ft_bytewidth;   // Width in unsigned chars
	ubyte     * ft_data;        // Ptr to raw data.
	ubyte    ** ft_chars;       // Ptrs to data for each char (required for prop font)
	short     * ft_widths;      // Array of widths (required for prop font)
	ubyte     * ft_kerndata;    // Array of kerning triplet data
	// These fields do not participate in disk i/o!
	grsBitmap *ft_bitmaps;
	grsBitmap ft_parent_bitmap;
} __pack__ grs_font;

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

typedef struct _grs_rgba {
	ubyte			red;
	ubyte			green;
	ubyte			blue;
	ubyte			alpha;
} grs_rgba;

typedef struct _grs_color {
	short       index;       // current color
	ubyte			rgb;
	grs_rgba		color;
} grs_color;

typedef struct _grs_canvas {
	grsBitmap  cv_bitmap;      // the bitmap for this canvas
	grs_color	cv_color;
	short       cv_drawmode;    // fill, XOR, etc.
	grs_font		*cv_font;        // the currently selected font
	grs_color	cv_font_fg_color;   // current font foreground color (-1==Invisible)
	grs_color	cv_font_bg_color;   // current font background color (-1==Invisible)
} grs_canvas;

//shortcuts
#define cv_w cv_bitmap.bm_props.w
#define cv_h cv_bitmap.bm_props.h

typedef struct _grs_screen {    // This is a video screen
	grs_canvas  sc_canvas;  // Represents the entire screen
	u_int32_t	sc_mode;        // Video mode number
	short   		sc_w, sc_h;     // Actual Width and Height
	fix     		sc_aspect;      //aspect ratio (w/h) for this screen
} grs_screen;


//=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grdCurScreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

int GrInit(void);

// This function sets up the main screen.  It should be called whenever
// the video mode changes.
int GrInitScreen(int mode, int w, int h, int x, int y, int rowsize, ubyte *data);

int GrVideoModeOK(u_int32_t mode);
int GrSetMode(u_int32_t mode);


// These 4 functions actuall change screen colors.

extern void gr_pal_fade_out(unsigned char * pal);
extern void gr_pal_fade_in(unsigned char * pal);
extern void gr_pal_clear(void);
extern void gr_pal_setblock( int start, int number, unsigned char * pal );
extern void gr_pal_getblock( int start, int number, unsigned char * pal );


extern unsigned char *gr_video_memory;
	                                            // All graphic modules will define this value.

//shut down the 2d.  Restore the screen mode.
void _CDECL_ GrClose(void);

//=========================================================================
// Canvas functions:

// Makes a new canvas. allocates memory for the canvas and its bitmap, 
// including the raw pixel buffer.

grs_canvas *GrCreateCanvas(int w, int h);
// Creates a canvas that is part of another canvas.  this can be used to make
// a window on the screen.  the canvas structure is malloc'd; the address of
// the raw pixel data is inherited from the parent canvas.

grs_canvas *GrCreateSubCanvas(grs_canvas *canv, int x, int y, int w, int h);

// Initialize the specified canvas. the raw pixel data buffer is passed as
// a parameter. no memory allocation is performed.

void GrInitCanvas(grs_canvas *canv, unsigned char *pixdata, int pixtype, int w, int h);

// Initialize the specified sub canvas. no memory allocation is performed.

void GrInitSubCanvas(grs_canvas *new, grs_canvas *src, int x, int y, int w, int h);

// Free up the canvas and its pixel data.

void GrFreeCanvas(grs_canvas *canv);

// Free up the canvas. do not free the pixel data, which belongs to the
// parent canvas.

void GrFreeSubCanvas(grs_canvas *canv);

// Clear the current canvas to the specified color
void GrClearCanvas(unsigned int color);

//=========================================================================
// Bitmap functions:

// Allocate a bitmap and its pixel data buffer.
grsBitmap *GrCreateBitmap(int w, int h, int bTGA);

// Allocated a bitmap and makes its data be raw_data that is already somewhere.
grsBitmap *GrCreateBitmapSub (int w, int h, unsigned char * raw_data, int bTGA );

// Creates a bitmap which is part of another bitmap
grsBitmap *GrCreateSubBitmap(grsBitmap *bm, int x, int y, int w, int h);

void *GrAllocBitmapData (int w, int h, int bTGA);

void GrInitBitmapAlloc (grsBitmap *bmP, int mode, int x, int y, int w, int h, 
								int nBytesPerLine, int bTGA);

void GrInitSubBitmap (grsBitmap *bm, grsBitmap *bmParent, int x, int y, int w, int h );

void GrInitBitmap (grsBitmap *bm, int mode, int x, int y, int w, int h, int bytesPerLine, 
						 unsigned char * data, int bTGA);
// Free the bitmap and its pixel data
void GrFreeBitmap(grsBitmap *bm);

// Free the bitmap's data
void GrFreeBitmapData (grsBitmap *bm);
void GrInitBitmapData (grsBitmap *bm);

// Free the bitmap, but not the pixel data buffer
void GrFreeSubBitmap(grsBitmap *bm);

void gr_bm_pixel( grsBitmap * bm, int x, int y, unsigned char color );
void gr_bm_upixel( grsBitmap * bm, int x, int y, unsigned char color );
void GrBmBitBlt(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest);
void GrBmUBitBlt( int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest);
void GrBmUBitBltM(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest);

void gr_update_buffer( void * sbuf1, void * sbuf2, void * dbuf, int size );

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until GrClose is called

ubyte *GrUsePaletteTable(char * filename, char *level_name );
void GrCopyPalette(ubyte *gr_palette, ubyte *pal, int size);

//=========================================================================
// Drawing functions:

// For solid, XOR, or other fill modes.
int gr_set_drawmode(int mode);

// Sets the color in the current canvas.  should be a macro
// Use: GrSetColor(int color);
void GrSetColor(int color);
void GrSetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha);
void GrSetColorRGB15bpp (ushort c, ubyte alpha);
void GrFadeColorRGB (double dFade);
	
#define GrSetColorRGBi(_c)	GrSetColorRGB (RGBA_RED (_c), RGBA_GREEN (_c), RGBA_BLUE (_c), RGBA_ALPHA (_c))

//	-----------------------------------------------------------------------------

// Draw a polygon into the current canvas in the current color and drawmode.
// verts points to an ordered list of x, y pairs.  the polygon should be
// convex; a concave polygon will be handled in some reasonable manner, 
// but not necessarily shaded as a concave polygon. It shouldn't hang.
// probably good solution is to shade from minx to maxx on each scan line.
// int should really be fix
int gr_poly(int nverts, int *verts);
int gr_upoly(int nverts, int *verts);


// Draws a point into the current canvas in the current color and drawmode.
void gr_pixel(int x, int y);
void gr_upixel(int x, int y);

// Gets a pixel;
unsigned char gr_gpixel( grsBitmap * bitmap, int x, int y );
unsigned char gr_ugpixel( grsBitmap * bitmap, int x, int y );

// Draws a line into the current canvas in the current color and drawmode.
int GrLine(fix x0, fix y0, fix x1, fix y1);
int gr_uline(fix x0, fix y0, fix x1, fix y1);

// Draws an anti-aliased line into the current canvas in the current color and drawmode.
int gr_aaline(fix x0, fix y0, fix x1, fix y1);
int gr_uaaline(fix x0, fix y0, fix x1, fix y1);

// Draw the bitmap into the current canvas at the specified location.
void GrBitmap(int x, int y, grsBitmap *bm);
void gr_ubitmap(int x, int y, grsBitmap *bm);
void GrBitmapScaleTo(grsBitmap *src, grsBitmap *dst);
void show_fullscr(grsBitmap *bm);

// bitmap function with transparency
void GrBitmapM( int x, int y, grsBitmap *bm );
void GrUBitmapM( int x, int y, grsBitmap *bm );

// Draw a rectangle into the current canvas.
void GrRect(int left, int top, int right, int bot);
void GrURect(int left, int top, int right, int bot);

// Draw a filled circle
int gr_disk(fix x, fix y, fix r);
int gr_udisk(fix x, fix y, fix r);

// Draw an outline circle
int gr_circle(fix x, fix y, fix r);
int GrUCircle(fix x, fix y, fix r);

// Draw an unfilled rectangle into the current canvas
void gr_box(int left, int top, int right, int bot);
void GrUBox(int left, int top, int right, int bot);

void gr_scanline( int x1, int x2, int y );
void gr_uscanline( int x1, int x2, int y );


// Reads in a font file... current font set to this one.
grs_font * GrInitFont( char * fontfile );
void GrCloseFont( grs_font * font );

//remap a font, re-reading its data & palette
void GrRemapFont( grs_font *font, char * fontname, char *font_data );

//remap (by re-reading) all the color fonts
void GrRemapColorFonts();
void GrRemapMonoFonts();

#define RGBA(_r,_g,_b,_a)			(((unsigned int) (_r) << 24) | ((unsigned int) (_g) << 16) | ((unsigned int) (_b) << 8) | ((unsigned int) (_a)))
#define RGBA_RED(_i)					(((unsigned int) (_i) >> 24) & 0xff)
#define RGBA_GREEN(_i)				(((unsigned int) (_i) >> 16) & 0xff)
#define RGBA_BLUE(_i)				(((unsigned int) (_i) >> 8) & 0xff)
#define RGBA_ALPHA(_i)				(((unsigned int) (_i)) & 0xff)
#define PAL2RGBA(_c)					((unsigned char) (((unsigned int) (_c) * 255) / 63))
#define RGBA_PAL(_r,_g,_b)			RGBA (PAL2RGBA (_r), PAL2RGBA (_g), PAL2RGBA (_b), 255)
#define RGBA_PALX(_r,_g,_b,_x)	RGBA_PAL ((_r) * (_x), (_g) * (_x), (_b) * (_x))
#define RGBA_PAL3(_r,_g,_b)		RGBA_PALX (_r, _g, _b, 3)
#define RGBA_PAL2(_r,_g,_b)		RGBA_PALX (_r, _g, _b, 2)
#define RGBA_FADE(_c,_f)			RGBA (RGBA_RED (_c) / (_f), RGBA_GREEN (_c) / (_f), RGBA_BLUE (_c) / (_f), RGBA_ALPHA (_c))

#define WHITE_RGBA					RGBA (255,255,255,255)
#define GRAY_RGBA						RGBA (128,128,128,255)
#define DKGRAY_RGBA					RGBA (80,80,80,255)
#define BLACK_RGBA					RGBA (0,0,0,255)
#define RED_RGBA						RGBA (255,0,0,255)
#define MEDRED_RGBA					RGBA (128,0,0,255)
#define DKRED_RGBA					RGBA (80,0,0,255)
#define GREEN_RGBA					RGBA (0,255,0,255)
#define MEDGREEN_RGBA				RGBA (0,128,0,255)
#define DKGREEN_RGBA					RGBA (0,80,0,255)
#define BLUE_RGBA						RGBA (0,0,255,255)
#define MEDBLUE_RGBA					RGBA (0,0,128,255)
#define DKBLUE_RGBA					RGBA (0,0,80,255)
#define ORANGE_RGBA					RGBA (255,128,0,255)
#define GOLD_RGBA						RGBA (255,224,0,255)

#define D2BLUE_RGBA			RGBA_PAL (35,35,55)

// Writes a string using current font. Returns the next column after last char.
void GrSetFontColor( int fg, int bg );
void GrSetFontColorRGB (grs_rgba *fg, grs_rgba *bg);
void GrSetFontColorRGBi (unsigned int fg, int bSetFG, unsigned int bg, int bSetBG);
void GrSetCurFont( grs_font * new );
int GrString(int x, int y, char *s );
int GrUString(int x, int y, char *s );
int _CDECL_ GrPrintF( int x, int y, char * format, ... );
int _CDECL_ GrUPrintf( int x, int y, char * format, ... );
void GrGetStringSize(char *s, int *string_width, int *string_height, int *average_width);
void GrGetStringSizeTabbed (char *s, int *string_width, int *string_height, int *average_width, int *nTabs, int nMaxWidth);
grsBitmap *CreateStringBitmap (char *s, int nKey, unsigned int nKeyColor, int nTabs [], int bCentered, int nMaxWidth);
int GetCenteredX (char *s);

//  From roller.c
void RotateBitmap(grsBitmap *bp, grs_point *vertbuf, int lightValue);

// From scale.c
void scale_bitmap(grsBitmap *bp, grs_point *vertbuf, int orientation );

//===========================================================================
// Global variables
extern grs_canvas *grdCurCanv;             //active canvas
extern grs_screen *grdCurScreen;           //active screen
extern unsigned char Test_bitmap_data[64*64];

//shortcut to look at current font
#define grd_curfont grdCurCanv->cv_font

extern unsigned int FixDivide( unsigned int x, unsigned int y );

extern void GrShowCanvas( grs_canvas *canv );
extern void GrSetCurrentCanvas( grs_canvas *canv );

//flags for fonts
#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

extern void gr_vesa_update( grsBitmap * source1, grsBitmap * dest, grsBitmap * source2 );

// Special effects
extern void gr_snow_out(int num_dots);

extern void TestRotateBitmap(void);
extern void RotateBitmap(grsBitmap *bp, grs_point *vertbuf, int lightValue);

extern ubyte grFadeTable[256*GR_FADE_LEVELS];
extern ubyte grInverseTable[32*32*32];

extern ushort grPaletteSelector;
extern ushort grInverseTableSelector;
extern ushort grFadeTableSelector;

// Remaps a bitmap into the current palette. If transparent_color is
// between 0 and 255 then all occurances of that color are mapped to
// whatever color the 2d uses for transparency. This is normally used
// right after a call to iff_read_bitmap like this:
//		iff_error = iff_read_bitmap(filename, new, BM_LINEAR, newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d", filename, iff_error);
//		if ( iff_has_transparency )
//			GrRemapBitmap( new, newpal, iff_transparent_color );
//		else
//			GrRemapBitmap( new, newpal, -1 );
void GrRemapBitmap( grsBitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color );

// Same as above, but searches using GrFindClosestColor which uses
// 18-bit accurracy instead of 15bit when translating colors.
void GrRemapBitmapGood( grsBitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color );

void GrPaletteStepUp( int r, int g, int b );

void GrBitmapCheckTransparency( grsBitmap * bmp );

// Allocates a selector that has a base address at 'address' and length 'size'.
// Returns 0 if successful... BE SURE TO CHECK the return value since there
// is a limited number of selectors available!!!
int GetSelector( void * address, int size, unsigned int * selector );

// Assigns a selector to a bitmap. Returns 0 if successful.  BE SURE TO CHECK
// this return value since there is a limited number of selectors!!!!!!!
int GrBitmapAssignSelector( grsBitmap * bmp );

//#define GR_GETCOLOR(r, g, b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])
//#define gr_getcolor(r, g, b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])
//#define BM_XRGB(r, g, b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])

#define BM_RGB(r, g, b) ( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )
//#define BM_XRGB(r, g, b) GrFindClosestColor( (r)*2, (g)*2, (b)*2 )
//#define GR_GETCOLOR(r, g, b) GrFindClosestColor( (r)*2, (g)*2, (b)*2 )
//#define gr_getcolor(r, g, b) GrFindClosestColor( (r)*2, (g)*2, (b)*2 )

// Given: r, g, b, each in range of 0-63, return the color index that
// best matches the input.
int GrFindClosestColor(ubyte *palette, int r, int g, int b);
int GrFindClosestColor15bpp( int rgb );
int GrAvgColor (grsBitmap *bm);

void GrMergeTextures( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale );
void GrMergeTextures1( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale );
void GrMergeTextures2( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale );
void GrMergeTextures3( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale );

void GrUpdate (int bClear);
void SaveScreenShot (unsigned char *buf, int automapFlag);

extern int bSaveScreenShot;
/*
 * currently SDL and OGL are the only things that supports toggling
 * fullscreen.  otherwise add other checks to the #if -MPM
 */
#define GR_SUPPORTS_FULLSCREEN_TOGGLE

/*
 * must return 0 if windowed, 1 if fullscreen
 */
int GrCheckFullScreen(void);

void ResetTextures (int bReload, int bGame);

/*
 * returns state after toggling (ie, same as if you had called
 * check_fullscreen immediatly after)
 */
int GrToggleFullScreen(void);

int GrToggleFullScreenMenu(void);//returns state after toggling (ie, same as if you had called check_fullscreen immediatly after)

typedef struct tScrSize {
	int	x, y, c;
} tScrSize;

extern tScrSize scrSizes [];

char *ScrSizeArg (int x, int y);
int SCREENMODE (int x, int y, int c);
int S_MODE (u_int32_t *VV, int *VG);


#endif /* def _GR_H */
