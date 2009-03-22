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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "mono.h"
#include "byteswap.h"       // because of rle code that has short for row offsets
#include "bitmap.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_defs.h"

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
ubyte *grBitBltFadeTable=NULL;

extern void gr_vesa_bitmap(CBitmap * source, CBitmap * dest, int x, int y);

void gr_linear_movsd(ubyte * source, ubyte * dest, uint nbytes);
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd

//------------------------------------------------------------------------------

#define THRESHOLD   8

#if !DBG
#define test_byteblit   0
#else
ubyte test_byteblit = 0;
#endif

void gr_linear_movsd(ubyte * src, ubyte * dest, uint num_pixels)
{
	uint i;
	uint n, r;
	double *d, *s;
	ubyte *d1, *s1;

// check to see if we are starting on an even byte boundry
// if not, move appropriate number of bytes to even
// 8 byte boundry

	if ((num_pixels < THRESHOLD) || (((size_t)src & 0x7) != ((size_t)dest & 0x7)) || test_byteblit) {
		for (i = 0; i < num_pixels; i++)
			*dest++ = *src++;
		return;
	}

	i = 0;
	if ((r = (uint) ((size_t)src & 0x7))) {
		for (i = 0; i < 8 - r; i++)
			*dest++ = *src++;
	}
	num_pixels -= i;

	n = num_pixels / 8;
	r = num_pixels % 8;
	s = reinterpret_cast<double*> (src);
	d = reinterpret_cast<double*> (dest);
	for (i = 0; i < n; i++)
		*d++ = *s++;
	s1 = reinterpret_cast<ubyte*> (s);
	d1 = reinterpret_cast<ubyte*> (d);
	for (i = 0; i < r; i++)
		*d1++ = *s1++;
}

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, uint num_pixels);

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, uint num_pixels)
{
	uint i;
	for (i=0; i<num_pixels; i++) {
		if (*src != TRANSPARENCY_COLOR)
			*dest = *src;
		dest++;
		src++;
	}
}

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, uint num_pixels, 
													ubyte fadeValue, CPalette *srcPalette, CPalette *destPalette);

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, uint num_pixels, 
													ubyte fadeValue, CPalette *srcPalette, CPalette *destPalette)
{
	int	i;
	short c;
	ubyte *fade_base;
	float fade = (float) fadeValue / 31.0f;

	if (!destPalette)
		destPalette = srcPalette;
	fade_base = paletteManager.FadeTable () + (fadeValue * 256);
	for (i=num_pixels; i != 0; i--) {
		c= (short) *src;
		if ((ubyte) c != (ubyte) TRANSPARENCY_COLOR) {
#if 1
			c *= 3;
			c = destPalette->ClosestColor ((ubyte) (srcPalette->Raw () [c] * fade + 0.5), 
												    (ubyte) (srcPalette->Raw () [c + 1] * fade + 0.5), 
													 (ubyte) (srcPalette->Raw () [c + 2] * fade + 0.5));
			*dest = (ubyte) c;
#else
			*dest = fade_base [c];
#endif
			}
		dest++;
		src++;
	}
}

//------------------------------------------------------------------------------

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, uint num_dest_pixels);

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, uint num_pixels)
{
	double*	d = reinterpret_cast<double*> (dest);
	uint*		s = reinterpret_cast<uint*> (src);
	uint		doubletemp[2];
	uint		temp, work;
	uint     i;

if (num_pixels & 0x3) {
	// not a multiple of 4?  do single pixel at a time
	for (i=0; i<num_pixels; i++) {
		*dest++ = *src;
		*dest++ = *src++;
		}
	return;
	}

for (i = 0; i < num_pixels / 4; i++) {
	temp = work = *s++;

	temp = ((temp >> 8) & 0x00FFFF00) | (temp & 0xFF0000FF); // 0xABCDEFGH -> 0xABABCDEF
	temp = ((temp >> 8) & 0x000000FF) | (temp & 0xFFFFFF00); // 0xABABCDEF -> 0xABABCDCD
	doubletemp[0] = temp;

	work = ((work << 8) & 0x00FFFF00) | (work & 0xFF0000FF); // 0xABCDEFGH -> 0xABEFGHGH
	work = ((work << 8) & 0xFF000000) | (work & 0x00FFFFFF); // 0xABEFGHGH -> 0xEFEFGHGH
	doubletemp[1] = work;

	*d = *reinterpret_cast<double*> (doubletemp);
	d++;
	}
}


//------------------------------------------------------------------------------

void gr_ubitmap00(int x, int y, CBitmap *bmP)
{
int srcRowSize = bmP->RowSize ();
int destRowSize = CCanvas::Current ()->RowSize () << gr_bitblt_dest_step_shift;
ubyte* dest = &(CCanvas::Current ()->Buffer ()[ destRowSize*y+x ]);
ubyte* src = bmP->Buffer ();

for (int y1 = 0; y1 < bmP->Height (); y1++) {
	if (gr_bitblt_double)
		gr_linear_rep_movsd_2x(src, dest, bmP->Width ());
	else
		gr_linear_movsd(src, dest, bmP->Width ());
	src += srcRowSize;
	dest+= destRowSize;
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap00m(int x, int y, CBitmap *bmP)
{
	register int y1;
	int destRowSize;

	ubyte * dest;
	ubyte * src;

	destRowSize=CCanvas::Current ()->RowSize () << gr_bitblt_dest_step_shift;
	dest = &(CCanvas::Current ()->Buffer ()[ destRowSize*y+x ]);

	src = bmP->Buffer ();

	if (grBitBltFadeTable==NULL) {
		for (y1=0; y1 < bmP->Height (); y1++)    {
			gr_linear_rep_movsdm(src, dest, bmP->Width ());
			src += bmP->RowSize ();
			dest+= (int)(destRowSize);
		}
	} else {
		for (y1=0; y1 < bmP->Height (); y1++)    {
			gr_linear_rep_movsdm_faded (src, dest, bmP->Width (), grBitBltFadeTable [y1+y], 
												 bmP->Palette (), CCanvas::Current ()->Palette ());
			src += bmP->RowSize ();
			dest+= (int)(destRowSize);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012(int x, int y, CBitmap *bmP)
{
	register int x1, y1;
	ubyte * src;

	src = bmP->Buffer ();

	for (y1=y; y1 < (y+bmP->Height ()); y1++)    {
		for (x1=x; x1 < (x+bmP->Width ()); x1++)    {
			CCanvas::Current ()->SetColor(*src++);
			DrawPixel(x1, y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012m(int x, int y, CBitmap *bmP)
{
	register int x1, y1;
	ubyte * src;

	src = bmP->Buffer ();

	for (y1=y; y1 < (y+bmP->Height ()); y1++) {
		for (x1=x; x1 < (x+bmP->Width ()); x1++) {
			if (*src != TRANSPARENCY_COLOR) {
				CCanvas::Current ()->SetColor(*src);
				DrawPixel(x1, y1);
			}
			src++;
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERIC(int x, int y, CBitmap * bmP)
{
	register int x1, y1;

	for (y1=0; y1 < bmP->Height (); y1++)    {
		for (x1=0; x1 < bmP->Width (); x1++)    {
			CCanvas::Current ()->SetColor(bmP->GetPixel (x1,y1));
			DrawPixel(x+x1, y+y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERICm(int x, int y, CBitmap * bmP)
{
	register int x1, y1;
	ubyte c;

	for (y1=0; y1 < bmP->Height (); y1++) {
		for (x1=0; x1 < bmP->Width (); x1++) {
			c = bmP->GetPixel (x1,y1);
			if (c != TRANSPARENCY_COLOR) {
				CCanvas::Current ()->SetColor(c);
				DrawPixel(x+x1, y+y1);
			}
		}
	}
}

//------------------------------------------------------------------------------
//@extern int Interlacing_on;

// From Linear to Linear
void SWBlitToBitmap (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest)
{
ubyte* sbits = src->Buffer ()  + (src->RowSize () * sy) + sx;
ubyte* dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;
int dstep = dest->RowSize () << gr_bitblt_dest_step_shift;

// No interlacing, copy the whole buffer.
for (int i = 0; i < h; i++) {
	if (gr_bitblt_double)
		gr_linear_rep_movsd_2x (sbits, dbits, w);
	else
		gr_linear_movsd (sbits, dbits, w);
	sbits += src->RowSize ();
	dbits += dstep;
	}
}

//------------------------------------------------------------------------------
// From Linear to Linear Masked
void BlitToBitmapMasked (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest)
{
ubyte* sbits = src->Buffer ()  + (src->RowSize () * sy) + sx;
ubyte* dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;

	// No interlacing, copy the whole buffer.

if (grBitBltFadeTable == NULL) {
	for (int i = 0; i < h; i++) {
		gr_linear_rep_movsdm (sbits, dbits, w);
		sbits += src->RowSize ();
		dbits += dest->RowSize ();
		}
	} 
else {
	for (int i = 0; i < h; i++) {
		gr_linear_rep_movsdm_faded (sbits, dbits, w, grBitBltFadeTable [dy+i], src->Palette (), dest->Palette ());
		sbits += src->RowSize ();
		dbits += dest->RowSize ();
		}
	}
}

//------------------------------------------------------------------------------

void BlitToBitmapRLE (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->Flags () & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->Buffer ()[4 + (src->Height ()*data_offset)];

	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->Buffer ()[4+(i*data_offset)]));

	dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline(dbits, sbits, sx, sx+w-1);
		if (src->Flags () & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*(reinterpret_cast<short*> (src->Buffer (4 + (i + sy) * data_offset))));
		else
			sbits += (int)(src->Buffer ()[4+i+sy]);
		dbits += dest->RowSize () << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------

void BlitToBitmapMaskedRLE (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->Flags () & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->Buffer ()[4 + (src->Height ()*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->Buffer ()[4+(i*data_offset)]));

	dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx+w-1);
		if (src->Flags () & BM_FLAG_RLE_BIG)
			sbits += (int) INTEL_SHORT (*reinterpret_cast<short*> (src->Buffer () + 4 + (i + sy) * data_offset));
		else
			sbits += (int) (*src) [4 + i + sy];
		dbits += dest->RowSize () << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------
// in rle.c

extern void gr_rle_expand_scanline_generic(CBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );
extern void gr_rle_expand_scanline_generic_masked(CBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );
extern void gr_rle_expand_scanline_svga_masked(CBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );

void StretchToBitmapRLE (int w, int h, int dx, int dy, int sx, int sy, CBitmap * src, CBitmap * dest)
{
	int i, data_offset;
	register int y1;
	ubyte * sbits;

	data_offset = 1;
	if (src->Flags () & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->Buffer ()[4 + (src->Height ()*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->Buffer ()[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_generic(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->Flags () & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT (*reinterpret_cast<short*> (src->Buffer () + 4 + (y1 + sy) * data_offset));
		else
			sbits += (int) (*src) [4 + y1 + sy];
	}
}

//------------------------------------------------------------------------------
// rescaling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

inline void ScaleLine (ubyte *src, ubyte *dest, int ilen, int olen)
{
	int a = olen /ilen, b = olen % ilen;
	int c = 0, i;
	ubyte *end = dest + olen;

while (dest < end) {
	i = a;
	c += b;
	if(c >= ilen) {
		c -= ilen;
		goto inside;
		}
	while(--i >= 0) {
inside:
		*dest++ = *src;
		}
	src++;
	}
}

//------------------------------------------------------------------------------

void CBitmap::BlitScaled (CBitmap* destP)
{
	ubyte *s = Buffer ();
	ubyte *d = destP->Buffer ();
	int h = Height ();
	int a = destP->Height () / h, b = destP->Height () % h;
	int c = 0, i, y;

for (y = 0; y < h; y++) {
	i = a;
	c += b;
	if(c >= h) {
		c -= h;
		goto inside;
		}
	while(--i >= 0) {
inside:
		ScaleLine (s, d, Width (), destP->Width ());
		d += destP->RowSize ();
		}
	s += RowSize ();
	}
}

//------------------------------------------------------------------------------

void CBitmap::Blit (CBitmap* dest, int xDest, int yDest, int w, int h, int xSrc, int ySrc, int bTransp)
{
if (Mode () == BM_LINEAR) {
	if (dest->Mode () == BM_LINEAR) {
		if (Flags () & BM_FLAG_RLE)
			BlitToBitmapRLE (w, h, xDest, yDest, xSrc, ySrc, this, dest);
		else
			SWBlitToBitmap (w, h, xDest, yDest, xSrc, ySrc, this, dest);
		}
	else if (dest->Mode () == BM_OGL) {
		Render (dest, xDest, yDest, w, h, xSrc, ySrc, w, h, bTransp);
		}
	else if (Flags () & BM_FLAG_RLE) {
		StretchToBitmapRLE (w, h, xDest, yDest, xSrc, ySrc, this, dest);
		}
	else {
		for (int y1 = 0; y1 < h; y1++)  
			for (int x1 = 0; x1 < w; x1++)  
				dest->DrawPixel (xDest + x1, yDest + y1, GetPixel (xSrc + x1, ySrc + y1));
		}
	}
else if (Mode () == BM_OGL) {
	if (dest->Mode () == BM_LINEAR)
		ScreenCopy (dest, xDest, yDest, w, h, xSrc, ySrc);
	}
}

//------------------------------------------------------------------------------

void CBitmap::BlitClipped (int xSrc, int ySrc)
{
	CBitmap* const dest = CCanvas::Current ();

	int destLeft = xSrc, destRight = xSrc + Width () - 1;
	int destTop = ySrc, destBottom = ySrc + Height () - 1;

if ((destLeft >= dest->Width ()) || (destRight < 0)) 
	return;
if ((destTop >= dest->Height ()) || (destBottom < 0)) 
	return;
if (destLeft < 0) { 
	xSrc = -destLeft; 
	destLeft = 0; 
	}
else
	xSrc = 0;
if (destTop < 0) { 
	ySrc = -destTop; 
	destTop = 0; 
	}
else
	ySrc = 0;
if (destRight >= dest->Width ())
	destRight = dest->Width () - 1;
if (destBottom >= dest->Height ())
	destBottom = dest->Height () - 1;
Blit (dest, destLeft, destTop, destRight - destLeft + 1, destBottom - destTop + 1, xSrc, ySrc, 0);
}

//------------------------------------------------------------------------------
// GrBmBitBlt 
void CBitmap::BlitClipped (CBitmap* dest, int destLeft, int destTop, int w, int h, int srcLeft, int srcTop)
{
if (!dest)
	dest = CCanvas::Current ();

	int	destRight = destLeft + w - 1;
	int	destBottom = destTop + h - 1;
	int	srcRight = srcLeft + w - 1;
	int	srcBottom = srcTop + h - 1;

if ((destLeft >= dest->Width ()) || (destRight < 0)) 
	return;
if ((destTop >= dest->Height ()) || (destBottom < 0)) 
	return;

if (destLeft < 0) { 
	srcLeft -= destLeft; 
	destLeft = 0; 
	}
if (destTop < 0) { 
	srcTop -= destTop; 
	destTop = 0; 
	}

if (destRight >= dest->Width ())
	destRight = dest->Width () - 1; 
if (destBottom >= dest->Height ())
	destBottom = dest->Height () - 1; 

if ((srcLeft >= Width ()) || (srcRight < 0)) 
	return;
if ((srcTop >= Height ()) || (srcBottom < 0)) 
	return;
if (srcLeft < 0) { 
	destLeft += -srcLeft; 
	srcLeft = 0; 
	}
if (srcTop < 0) { 
	destTop += -srcTop; 
	srcTop = 0; 
	}
if (srcRight >= Width ()) { 
	srcRight = Width () - 1; 
	}
if (srcBottom >= Height ()) { 
	srcBottom = Height () - 1; 
	}

// Draw bitmap bmP[x,y] into (destLeft,destTop)-(destRight,destBottom)
if (w < 0)
	w = Width ();
if (h < 0)
	h = Height ();

if (destRight - destLeft + 1 < w)
	w = destRight - destLeft + 1;
if (destBottom - destTop + 1 < h)
	h = destBottom - destTop + 1;
if (srcRight - srcLeft + 1 < w)
	w = srcRight - srcLeft + 1;
if (srcBottom - srcTop + 1 < h)
	h = srcBottom - srcTop + 1;

Blit (dest, destLeft, destTop, w, h, srcLeft, srcTop, 1);
}

//------------------------------------------------------------------------------

void CBitmap::RenderFullScreen (void)
{
	CBitmap * const dest = CCanvas::Current ();

if ((Mode () == BM_LINEAR) && (dest->Mode () == BM_OGL)) {
	Render (dest, 0, 0, dest->Width (), dest->Height (), 0, 0, Width (), Height (), 0, 0);
	}
else if (dest->Mode () != BM_LINEAR) {
	CBitmap *tmp = CBitmap::Create (0, dest->Width (), dest->Height (), 1);
	BlitScaled (tmp);
	tmp->BlitClipped (0, 0);
	delete tmp;
	}
else
	BlitScaled (dest);
}

//------------------------------------------------------------------------------

