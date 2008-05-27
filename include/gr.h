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
#include "vecmat.h"

//-----------------------------------------------------------------------------

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

#define GWIDTH  grdCurCanv->cvBitmap.bmProps.w
#define GHEIGHT grdCurCanv->cvBitmap.bmProps.h
#define SWIDTH  (grdCurScreen->scWidth)
#define SHEIGHT (grdCurScreen->scHeight)

#define MAX_BMP_SIZE(width, height) (4 + ((width) + 2) * (height))

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

//-----------------------------------------------------------------------------

typedef struct grsPoint {
	fix x, y;
} grsPoint;

typedef struct tRgbColord {
	double red;
	double green;
	double blue;
} tRgbColord;

typedef struct tRgbaColord {
	double red;
	double green;
	double blue;
	double alpha;
} tRgbaColord;

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

typedef struct tFaceColor {
	tRgbaColorf	color;
	char			index;
} tFaceColor;

//-----------------------------------------------------------------------------

#define SM(w, h) ((((u_int32_t)w)<<16)+(((u_int32_t)h)&0xFFFF))
#define SM_W(m) (m>>16)
#define SM_H(m) (m&0xFFFF)

//-----------------------------------------------------------------------------

#define BM_FLAG_TRANSPARENT         1
#define BM_FLAG_SUPER_TRANSPARENT   2
#define BM_FLAG_NO_LIGHTING         4
#define BM_FLAG_RLE                 8   // A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT           16  // This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG             32  // for bitmaps that RLE to > 255 per row (i.e. cockpits)
#define BM_FLAG_SEE_THRU				64  // door or other texture containing see-through areas
#define BM_FLAG_TGA						128

typedef struct grsBmProps {
	short   x, y;		// Offset from parent's origin
	short   w, h;		// width, height
	short   rowSize;	// unsigned char offset to next row
	sbyte	  nType;		// 0=Linear, 1=ModeX, 2=SVGA
	ubyte	  flags;		
} grsBmProps;

typedef struct grsStdBmData {
	struct grsBitmap	*bmAlt;
	struct grsBitmap	*bmMask;	//intended for supertransparency masks 
	struct grsBitmap	*bmParent;
} grsStdBmData;

typedef struct grsAltBmData {
	ubyte						bmFrameCount;
	struct grsBitmap		*bmFrames;
	struct grsBitmap		*bmCurFrame;
} grsAltBmData;

#define BM_TYPE_STD		0
#define BM_TYPE_ALT		1
#define BM_TYPE_FRAME	2
#define BM_TYPE_MASK		4

typedef struct grsBitmap {
#if 1//def _DEBUG
	char				szName [20];
#endif
	grsBmProps		bmProps;
	ubyte				*bmPalette;
	ubyte				*bmTexBuf;		// ptr to texture data...
											//   Linear = *parent+(rowSize*y+x)
											//   ModeX = *parent+(rowSize*y+x/4)
											//   SVGA = *parent+(rowSize*y+x)
	ushort			bmHandle;		//for application.  initialized to 0
	ubyte				bmAvgColor;		//  Average color of all pixels in texture map.
	tRgbColorb		bmAvgRGB;
	ubyte				bmBPP :3;
	ubyte				bmType :3;
	ubyte				bmWallAnim :1;
	ubyte				bmFromPog :1;
	ubyte				bmFlat;			//no texture, just a colored area
	ubyte				bmTeam;
#if TEXTURE_COMPRESSION
	ubyte				bmCompressed;
	int				bmFormat;
	int				bmBufSize;
#endif
	int				bmTransparentFrames [4];
	int				bmSupertranspFrames [4];

	struct tOglTexture	*glTexture;
	struct {
		grsStdBmData		std;
		grsAltBmData		alt;
		} bmData;
} grsBitmap;

#define BM_FRAMECOUNT(_bmP)	((_bmP)->bmData.alt.bmFrameCount)
#define BM_FRAMES(_bmP)			((_bmP)->bmData.alt.bmFrames)
#define BM_CURFRAME(_bmP)		((_bmP)->bmData.alt.bmCurFrame)
#define BM_OVERRIDE(_bmP)		((_bmP)->bmData.std.bmAlt)
#define BM_MASK(_bmP)			((_bmP)->bmData.std.bmMask)
#define BM_PARENT(_bmP)			((_bmP)->bmData.std.bmParent)

//-----------------------------------------------------------------------------

static inline grsBitmap *BmCurFrame (grsBitmap *bmP, int iFrame)
{
if (bmP->bmType != BM_TYPE_ALT)
	return bmP;
if (iFrame < 0)
	return BM_CURFRAME (bmP) ? BM_CURFRAME (bmP) : bmP;
return BM_CURFRAME (bmP) = ((BM_FRAMES (bmP) ? BM_FRAMES (bmP) + iFrame % BM_FRAMECOUNT (bmP) : bmP));
}

//-----------------------------------------------------------------------------

static inline grsBitmap *BmOverride (grsBitmap *bmP, int iFrame)
{
if (!bmP)
	return bmP;
if (bmP->bmType == BM_TYPE_STD) {
	if (!BM_OVERRIDE (bmP))
		return bmP;
	bmP = BM_OVERRIDE (bmP);
	}
return BmCurFrame (bmP, iFrame);
}

//-----------------------------------------------------------------------------

//font structure
typedef struct grsFont {
	short       ftWidth;       // Width in pixels
	short       ftHeight;      // Height in pixels
	short       ftFlags;       // Proportional?
	short       ftBaseLine;    //
	ubyte       ftMinChar;     // First char defined by this font
	ubyte       ftMaxChar;     // Last char defined by this font
	short       ftByteWidth;   // Width in unsigned chars
	ubyte     	*ftData;        // Ptr to raw data.
	ubyte    	**ftChars;       // Ptrs to data for each char (required for prop font)
	short     	*ftWidths;      // Array of widths (required for prop font)
	ubyte     	*ftKernData;    // Array of kerning triplet data
	// These fields do not participate in disk i/o!
	grsBitmap 	*ftBitmaps;
	grsBitmap 	ftParentBitmap;
} __pack__ grsFont;

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

#define FONT			grdCurCanv->cvFont
#define FG_COLOR		grdCurCanv->cvFontFgColor
#define BG_COLOR		grdCurCanv->cvFontBgColor
#define FWIDTH       FONT->ftWidth
#define FHEIGHT      FONT->ftHeight
#define FBASELINE    FONT->ftBaseLine
#define FFLAGS       FONT->ftFlags
#define FMINCHAR     FONT->ftMinChar
#define FMAXCHAR     FONT->ftMaxChar
#define FDATA        FONT->ftData
#define FCHARS       FONT->ftChars
#define FWIDTHS      FONT->ftWidths

//-----------------------------------------------------------------------------

typedef struct grsRgba {
	ubyte			red;
	ubyte			green;
	ubyte			blue;
	ubyte			alpha;
} grsRgba;

typedef struct grsColor {
	short       index;       // current color
	ubyte			rgb;
	grsRgba		color;
} grsColor;

typedef struct grsCanvas {
	grsBitmap   cvBitmap;      // the bitmap for this canvas
	grsColor		cvColor;
	short       cvDrawMode;    // fill, XOR, etc.
	grsFont		*cvFont;        // the currently selected font
	grsColor		cvFontFgColor;   // current font foreground color (-1==Invisible)
	grsColor		cvFontBgColor;   // current font background color (-1==Invisible)
} gsrCanvas;

//shortcuts
#define cv_w cvBitmap.bmProps.w
#define cv_h cvBitmap.bmProps.h

typedef struct grsScreen {    // This is a video screen
	gsrCanvas  	scCanvas;  // Represents the entire screen
	u_int32_t	scMode;        // Video mode number
	short   		scWidth, scHeight;     // Actual Width and Height
	fix     		scAspect;      //aspect ratio (w/h) for this screen
} grsScreen;


//=========================================================================
// System functions:
// setup and set mode. this creates a grsScreen structure and sets
// grdCurScreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

int GrInit(void);

// This function sets up the main screen.  It should be called whenever
// the video mode changes.
int GrInitScreen(int mode, int w, int h, int x, int y, int rowSize, ubyte *data);

int GrVideoModeOK(u_int32_t mode);
int GrSetMode(u_int32_t mode);

//-----------------------------------------------------------------------------
// These 4 functions actuall change screen colors.

extern void gr_pal_fade_out(unsigned char * pal);
extern void gr_pal_fade_in(unsigned char * pal);
extern void gr_pal_clear(void);
extern void gr_pal_setblock(int start, int number, unsigned char * pal);
extern void gr_pal_getblock(int start, int number, unsigned char * pal);

extern unsigned char *gr_video_memory;

//shut down the 2d.  Restore the screen mode.
void _CDECL_ GrClose(void);

//=========================================================================
// Canvas functions:

// Makes a new canvas. allocates memory for the canvas and its bitmap, 
// including the raw pixel buffer.

gsrCanvas *GrCreateCanvas(int w, int h);
// Creates a canvas that is part of another canvas.  this can be used to make
// a window on the screen.  the canvas structure is malloc'd; the address of
// the raw pixel data is inherited from the parent canvas.

gsrCanvas *GrCreateSubCanvas(gsrCanvas *canvP, int x, int y, int w, int h);

// Initialize the specified canvas. the raw pixel data buffer is passed as
// a parameter. no memory allocation is performed.

void GrInitCanvas(gsrCanvas *canvP, unsigned char *pixdata, int pixtype, int w, int h);

// Initialize the specified sub canvas. no memory allocation is performed.

void GrInitSubCanvas(gsrCanvas *canvP, gsrCanvas *src, int x, int y, int w, int h);

// Free up the canvas and its pixel data.

void GrFreeCanvas(gsrCanvas *canvP);

// Free up the canvas. do not free the pixel data, which belongs to the
// parent canvas.

void GrFreeSubCanvas(gsrCanvas *canvP);

// Clear the current canvas to the specified color
void GrClearCanvas(unsigned int color);

//=========================================================================
// Bitmap functions:

// Allocate a bitmap and its pixel data buffer.
grsBitmap *GrCreateBitmap(int w, int h, int bpp);

// Allocated a bitmap and makes its data be raw_data that is already somewhere.
grsBitmap *GrCreateBitmapSub (int w, int h, unsigned char * raw_data, int bpp);

// Creates a bitmap which is part of another bitmap
grsBitmap *GrCreateSubBitmap(grsBitmap *bm, int x, int y, int w, int h);

void *GrAllocBitmapData (int w, int h, int bpp);

void GrInitBitmapAlloc (grsBitmap *bmP, int mode, int x, int y, int w, int h, 
								int nBytesPerLine, int bpp);

void GrInitSubBitmap (grsBitmap *bm, grsBitmap *bmParent, int x, int y, int w, int h);

void GrInitBitmap (grsBitmap *bm, int mode, int x, int y, int w, int h, int bytesPerLine, 
						 unsigned char * data, int bpp);
// Free the bitmap and its pixel data
void GrFreeBitmap(grsBitmap *bm);

// Free the bitmap's data
void GrFreeBitmapData (grsBitmap *bm);
void GrInitBitmapData (grsBitmap *bm);

// Free the bitmap, but not the pixel data buffer
void GrFreeSubBitmap(grsBitmap *bm);

void gr_bm_pixel (grsBitmap * bm, int x, int y, unsigned char color);
void gr_bm_upixel (grsBitmap * bm, int x, int y, unsigned char color);
void GrBmBitBlt (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest);
void GrBmUBitBlt (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest, int bTransp);
void GrBmUBitBltM (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest, int bTransp);

void gr_update_buffer (void * sbuf1, void * sbuf2, void * dbuf, int size);

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until GrClose is called

ubyte *GrUsePaletteTable(char * filename, char *level_name);
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
unsigned char gr_gpixel(grsBitmap * bitmap, int x, int y);
unsigned char gr_ugpixel(grsBitmap * bitmap, int x, int y);

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
void ShowFullscreenImage(grsBitmap *bm);

// bitmap function with transparency
void GrBitmapM (int x, int y, grsBitmap *bmP, int bTransp);
void GrUBitmapM (int x, int y, grsBitmap *bmP);

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

void GrScanLine(int x1, int x2, int y);
void gr_uscanline(int x1, int x2, int y);


// Reads in a font file... current font set to this one.
grsFont * GrInitFont(char * fontfile);
void GrCloseFont(grsFont * font);

//remap a font, re-reading its data & palette
void GrRemapFont(grsFont *font, char * fontname, char *font_data);

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
void GrSetFontColor(int fg, int bg);
void GrSetFontColorRGB (grsRgba *fg, grsRgba *bg);
void GrSetFontColorRGBi (unsigned int fg, int bSetFG, unsigned int bg, int bSetBG);
void GrSetCurFont (grsFont * fontP);
int GrString (int x, int y, char *s, int *idP);
int GrUString (int x, int y, char *s);
int _CDECL_ GrPrintF (int *idP, int x, int y, char * format, ...);
int _CDECL_ GrUPrintf (int x, int y, char * format, ...);
void GrGetStringSize(char *s, int *string_width, int *string_height, int *average_width);
void GrGetStringSizeTabbed (char *s, int *string_width, int *string_height, int *average_width, int *nTabs, int nMaxWidth);
grsBitmap *CreateStringBitmap (char *s, int nKey, unsigned int nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce);
int GetCenteredX (char *s);

//  From roller.c
void RotateBitmap(grsBitmap *bp, grsPoint *vertbuf, int lightValue);

// From scale.c
void ScaleBitmap(grsBitmap *bp, grsPoint *vertbuf, int orientation);

void OglURect(int left,int top,int right,int bot);
void OglUPixelC (int x, int y, grsColor *c);
void OglULineC (int left,int top,int right,int bot, grsColor *c);
void OglUPolyC (int left, int top, int right, int bot, grsColor *c);

//===========================================================================
// Global variables
extern gsrCanvas *grdCurCanv;             //active canvas
extern grsScreen *grdCurScreen;           //active screen
extern unsigned char Test_bitmap_data[64*64];

//shortcut to look at current font
#define grd_curfont grdCurCanv->cvFont

extern unsigned int FixDivide(unsigned int x, unsigned int y);

extern void GrShowCanvas(gsrCanvas *canvP);
extern void GrSetCurrentCanvas(gsrCanvas *canvP);

//flags for fonts
#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

extern void gr_vesa_update(grsBitmap * source1, grsBitmap * dest, grsBitmap * source2);

// Special effects
extern void gr_snow_out(int num_dots);

extern void TestRotateBitmap(void);
extern void RotateBitmap(grsBitmap *bp, grsPoint *vertbuf, int lightValue);

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
//		if (iff_has_transparency)
//			GrRemapBitmap(new, newpal, iff_transparent_color);
//		else
//			GrRemapBitmap(new, newpal, -1);
void GrRemapBitmap(grsBitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color);

// Same as above, but searches using GrFindClosestColor which uses
// 18-bit accurracy instead of 15bit when translating colors.
void GrRemapBitmapGood(grsBitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color);

void GrPaletteStepUp(int r, int g, int b);

void GrBitmapCheckTransparency(grsBitmap * bmp);

// Allocates a selector that has a base address at 'address' and length 'size'.
// Returns 0 if successful... BE SURE TO CHECK the return value since there
// is a limited number of selectors available!!!
int GetSelector(void * address, int size, unsigned int * selector);

// Assigns a selector to a bitmap. Returns 0 if successful.  BE SURE TO CHECK
// this return value since there is a limited number of selectors!!!!!!!
int GrBitmapAssignSelector(grsBitmap * bmp);

//#define GR_GETCOLOR(r, g, b) (gr_inverse_table[((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))])
//#define gr_getcolor(r, g, b) (gr_inverse_table[((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))])
//#define BM_XRGB(r, g, b) (gr_inverse_table[((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))])

#define BM_RGB(r, g, b) ((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))
//#define BM_XRGB(r, g, b) GrFindClosestColor((r)*2, (g)*2, (b)*2)
//#define GR_GETCOLOR(r, g, b) GrFindClosestColor((r)*2, (g)*2, (b)*2)
//#define gr_getcolor(r, g, b) GrFindClosestColor((r)*2, (g)*2, (b)*2)

// Given: r, g, b, each in range of 0-63, return the color index that
// best matches the input.
int GrFindClosestColor(ubyte *palette, int r, int g, int b);
int GrFindClosestColor15bpp(int rgb);
int GrAvgColor (grsBitmap *bm);

void GrMergeTextures(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures1(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures2(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures3(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);

void GrUpdate (int bClear);
void SaveScreenShot (unsigned char *buf, int automapFlag);

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

void OglDoPalFx (void);

void InitStringPool (void);
void FreeStringPool (void);

typedef struct tScrSize {
	int	x, y, c;
} tScrSize;

extern tScrSize scrSizes [];

extern int curDrawBuffer;

char *ScrSizeArg (int x, int y);
int SCREENMODE (int x, int y, int c);
int S_MODE (u_int32_t *VV, int *VG);
int GrBitmapHasTransparency (grsBitmap *bmP);

typedef union tTexCoord2f {
	float a [2];
	struct {
		float	u, v;
		} v;
	} tTexCoord2f;

typedef union tTexCoord3f {
	float a [3];
	struct {
		float	u, v, l;
		} v;
	} tTexCoord3f;

typedef struct grsTriangle {
	ushort				nFace;
	ushort				index [3];
	int					nIndex;
	} grsTriangle;

typedef struct grsFace {
	ushort				index [4];
	ushort				*triIndex;
	int					nIndex;
	int					nTriIndex;
#if USE_RANGE_ELEMENTS
	uint					*vertIndex;
#endif
	int					nVerts;
	int					nTris;
	grsBitmap			*bmBot;
	grsBitmap			*bmTop;
	tTexCoord2f			*pTexCoord;	//needed to override default tex coords, e.g. for camera outputs
	tRgbaColorf			color;
	float					fRad;
	short					nWall;
	short					nBaseTex;
	short					nOvlTex;
	short					nCorona;
	short					nSegment;
	ushort				nLightmap;
	ubyte					nSide;
	ubyte					nOvlOrient :2;
	ubyte					bVisible :1;
	ubyte					bTextured :1;
	ubyte					bOverlay :1;
	ubyte					bSplit :1;
	ubyte					bTransparent :1;
	ubyte					bIsLight :1;
	ubyte					bHaveCameraBg :1;
	ubyte					bAnimation :1;
	ubyte					bTeleport :1;
	ubyte					bSlide :1;
	ubyte					bSolid :1;
	ubyte					bAdditive :2;
	ubyte					bSparks :1;
	ubyte					nRenderType : 2;
	ubyte					bColored :1;
	ubyte					widFlags;
	char					nCamera;
	char					nType;
	char					nSegColor;
	char					nShader;
	struct grsFace		*nextSlidingFace;
	} grsFace;

typedef struct grsString {
	char					*pszText;
	grsBitmap			*bmP;
	int					*pId;
	short					nWidth;
	short					nLength;
	} grsString;

typedef struct tDisplayModeInfo {
	int	VGA_mode;
	short	w,h;
	short	render_method;
	short	flags;
	char	isWideScreen;
	char	isAvailable;
} tDisplayModeInfo;

#define NUM_DISPLAY_MODES	21

extern tDisplayModeInfo displayModeInfo [NUM_DISPLAY_MODES + 1];

#endif /* def _GR_H */
