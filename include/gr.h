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
#include "bitmap.h"
#include "vecmat.h"

//-----------------------------------------------------------------------------

#define GWIDTH  grdCurCanv->cvBitmap.props.w
#define GHEIGHT grdCurCanv->cvBitmap.props.h
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

//-----------------------------------------------------------------------------

#define SM(w,h) ((((u_int32_t)w)<<16)+(((u_int32_t)h)&0xFFFF))
#define SM_W(m) (m>>16)
#define SM_H(m) (m&0xFFFF)

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
	CBitmap 		*ftBitmaps;
	CBitmap 		ftParentBitmap;
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
	CBitmap		cvBitmap;      // the bitmap for this canvas
	grsColor		cvColor;
	short       cvDrawMode;    // fill, XOR, etc.
	grsFont		*cvFont;        // the currently selected font
	grsColor		cvFontFgColor;   // current font foreground color (-1==Invisible)
	grsColor		cvFontBgColor;   // current font background color (-1==Invisible)
} gsrCanvas;

//shortcuts
#define cv_w cvBitmap.props.w
#define cv_h cvBitmap.props.h

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

void ShowFullscreenImage (CBitmap *src);

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
void gr_bm_pixel (CBitmap * bmP, int x, int y, unsigned char color);
void gr_bm_upixel (CBitmap * bmP, int x, int y, unsigned char color);
void GrBmBitBlt (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest);
void GrBmUBitBlt (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest, int bTransp);
void GrBmUBitBltM (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest, int bTransp);
void GrUBitmapM(int x, int y, CBitmap *bmP);

void GrBitmap (int x, int y, CBitmap *bmP);

void GrBitmapScaleTo(CBitmap *src, CBitmap *dst);

void gr_update_buffer (void * sbuf1, void * sbuf2, void * dbuf, int size);

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until GrClose is called

void GrCopyPalette (ubyte *gr_palette, ubyte *pal, int size);

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

#define GrSetColorRGBi(_c)	GrSetColorRGB (RGBA_RED (_c), RGBA_GREEN (_c), RGBA_BLUE (_c), 255) //RGBA_ALPHA (_c))

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
unsigned char gr_gpixel(CBitmap * bitmap, int x, int y);
unsigned char gr_ugpixel(CBitmap * bitmap, int x, int y);

// Draws a line into the current canvas in the current color and drawmode.
int GrLine(fix x0, fix y0, fix x1, fix y1);
int gr_uline(fix x0, fix y0, fix x1, fix y1);

// Draws an anti-aliased line into the current canvas in the current color and drawmode.
int gr_aaline(fix x0, fix y0, fix x1, fix y1);
int gr_uaaline(fix x0, fix y0, fix x1, fix y1);

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
grsFont * GrInitFont(const char *fontfile);
void GrCloseFont(grsFont * font);

//remap a font, re-reading its data & palette
void GrRemapFont(grsFont *font, char * fontname, char *font_data);

//remap (by re-reading) all the color fonts
void GrRemapColorFonts();
void GrRemapMonoFonts();

// Writes a string using current font. Returns the next column after last char.
void GrSetFontColor(int fg, int bg);
void GrSetFontColorRGB (grsRgba *fg, grsRgba *bg);
void GrSetFontColorRGBi (unsigned int fg, int bSetFG, unsigned int bg, int bSetBG);
void GrSetCurFont (grsFont * fontP);
int GrString (int x, int y, const char *s, int *idP);
int GrUString (int x, int y, const char *s);
int _CDECL_ GrPrintF (int *idP, int x, int y, const char * format, ...);
int _CDECL_ GrUPrintf (int x, int y, const char * format, ...);
void GrGetStringSize(const char *s, int *string_width, int *string_height, int *average_width);
void GrGetStringSizeTabbed (const char *s, int *string_width, int *string_height, int *average_width, int *nTabs, int nMaxWidth);
CBitmap *CreateStringBitmap (const char *s, int nKey, unsigned int nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce);
int GetCenteredX (const char *s);

//  From roller.c
void RotateBitmap(CBitmap *bp, grsPoint *vertbuf, int lightValue);

// From scale.c
void ScaleBitmap(CBitmap *bp, grsPoint *vertbuf, int orientation);

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

extern void gr_vesa_update(CBitmap * source1, CBitmap * dest, CBitmap * source2);

// Special effects
extern void gr_snow_out(int num_dots);

extern void TestRotateBitmap(void);
extern void RotateBitmap(CBitmap *bp, grsPoint *vertbuf, int lightValue);

extern ubyte grFadeTable[256*FADE_LEVELS];
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
void GrRemapBitmap(CBitmap * bmPp, ubyte * palette, int transparent_color, int super_transparent_color);

// Same as above, but searches using GrFindClosestColor which uses
// 18-bit accurracy instead of 15bit when translating colors.
void GrRemapBitmapGood(CBitmap * bmPp, ubyte * palette, int transparent_color, int super_transparent_color);

void GrPaletteStepUp(int r, int g, int b);

void GrBitmapCheckTransparency(CBitmap * bmPp);

// Allocates a selector that has a base address at 'address' and length 'size'.
// Returns 0 if successful... BE SURE TO CHECK the return value since there
// is a limited number of selectors available!!!
int GetSelector(void * address, int size, unsigned int * selector);

// Assigns a selector to a bitmap. Returns 0 if successful.  BE SURE TO CHECK
// this return value since there is a limited number of selectors!!!!!!!
int GrBitmapAssignSelector(CBitmap * bmPp);

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
int GrAvgColor (CBitmap *bm);

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
int GrBitmapHasTransparency (CBitmap *bmP);

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
	int					nFrame;
	CBitmap				*bmBot;
	CBitmap				*bmTop;
	tTexCoord2f			*pTexCoord;	//needed to override default tex coords, e.g. for camera outputs
	tRgbaColorf			color;
	float					fRads [2];
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
	ubyte					bHasColor :1;
	ubyte					widFlags;
	char					nCamera;
	char					nType;
	char					nSegColor;
	char					nShader;
	struct grsFace		*nextSlidingFace;
	} grsFace;

typedef struct grsString {
	char					*pszText;
	CBitmap			*bmP;
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
