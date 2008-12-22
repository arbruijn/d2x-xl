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
#include "canvas.h"

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

// This function sets up the main screen.  It should be called whenever
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
void gr_bm_pixel (CBitmap * bmP, int x, int y, ubyte color);
void gr_bm_upixel (CBitmap * bmP, int x, int y, ubyte color);
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
ubyte gr_gpixel(CBitmap * bitmap, int x, int y);
ubyte gr_ugpixel(CBitmap * bitmap, int x, int y);

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


void RotateBitmap(CBitmap *bp, grsPoint *vertbuf, int lightValue);

void ScaleBitmap(CBitmap *bp, grsPoint *vertbuf, int orientation);

void OglURect(int left,int top,int right,int bot);
void OglUPixelC (int x, int y, tCanvasColor *c);
void OglULineC (int left,int top,int right,int bot, tCanvasColor *c);
void OglUPolyC (int left, int top, int right, int bot, tCanvasColor *c);

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
void GrMergeTextures(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures1(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures2(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);
void GrMergeTextures3(ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale);

void GrUpdate (int bClear);
void SaveScreenShot (ubyte *buf, int automapFlag);

/*
 * currently SDL and OGL are the only things that supports toggling
 * fullscreen.  otherwise add other checks to the #if -MPM
 */
#define GR_SUPPORTS_FULLSCREEN_TOGGLE

void ResetTextures (int bReload, int bGame);

int GrCheckFullScreen(void);
int GrToggleFullScreen(void);
int GrToggleFullScreenMenu(void);//returns state after toggling (ie, same as if you had called check_fullscreen immediatly after)

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

typedef struct tFace {
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
	struct tFace		*nextSlidingFace;
	} tFace;

inline int operator- (tFace* f, CArray<tFace>& a) { return a.Index (f); }

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
