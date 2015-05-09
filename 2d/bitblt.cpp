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
#include "byteswap.h"       // because of rle code that has int16_t for row offsets
#include "bitmap.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "ogl_defs.h"

int32_t gr_bitblt_dest_step_shift = 0;
int32_t gr_bitblt_double = 0;
uint8_t *grBitBltFadeTable=NULL;

extern void gr_vesa_bitmap(CBitmap * source, CBitmap * dest, int32_t x, int32_t y);

void gr_linear_movsd(uint8_t * source, uint8_t * dest, uint32_t nbytes);
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd

//------------------------------------------------------------------------------

#define THRESHOLD   8

#if !DBG
#define test_byteblit   0
#else
uint8_t test_byteblit = 0;
#endif

void gr_linear_movsd(uint8_t * src, uint8_t * dest, uint32_t num_pixels)
{
	uint32_t i;
	uint32_t n, r;
	double *d, *s;
	uint8_t *d1, *s1;

// check to see if we are starting on an even byte boundry
// if not, move appropriate number of bytes to even
// 8 byte boundry

	if ((num_pixels < THRESHOLD) || (((size_t)src & 0x7) != ((size_t)dest & 0x7)) || test_byteblit) {
		for (i = 0; i < num_pixels; i++)
			*dest++ = *src++;
		return;
	}

	i = 0;
	if ((r = (uint32_t) ((size_t)src & 0x7))) {
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
	s1 = reinterpret_cast<uint8_t*> (s);
	d1 = reinterpret_cast<uint8_t*> (d);
	for (i = 0; i < r; i++)
		*d1++ = *s1++;
}

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm(uint8_t * src, uint8_t * dest, uint32_t num_pixels);

static void gr_linear_rep_movsdm(uint8_t * src, uint8_t * dest, uint32_t num_pixels)
{
	uint32_t i;
	for (i=0; i<num_pixels; i++) {
		if (*src != TRANSPARENCY_COLOR)
			*dest = *src;
		dest++;
		src++;
	}
}

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm_faded(uint8_t * src, uint8_t * dest, uint32_t num_pixels, 
													uint8_t fadeValue, CPalette *srcPalette, CPalette *destPalette);

static void gr_linear_rep_movsdm_faded(uint8_t * src, uint8_t * dest, uint32_t num_pixels, 
													uint8_t fadeValue, CPalette *srcPalette, CPalette *destPalette)
{
	int32_t	i;
	int16_t c;
	float fade = (float) fadeValue / 31.0f;

	if (!destPalette)
		destPalette = srcPalette;
	for (i=num_pixels; i != 0; i--) {
		c= (int16_t) *src;
		if ((uint8_t) c != (uint8_t) TRANSPARENCY_COLOR) {
			c *= 3;
			c = destPalette->ClosestColor ((uint8_t) FRound (srcPalette->Raw () [c] * fade), 
												    (uint8_t) FRound (srcPalette->Raw () [c + 1] * fade), 
													 (uint8_t) FRound (srcPalette->Raw () [c + 2] * fade));
			*dest = (uint8_t) c;
			}
		dest++;
		src++;
	}
}

//------------------------------------------------------------------------------

void gr_linear_rep_movsd_2x (uint8_t *src, uint8_t *dest, uint32_t num_dest_pixels);

void gr_linear_rep_movsd_2x (uint8_t *src, uint8_t *dest, uint32_t num_pixels)
{
	double*	d = reinterpret_cast<double*> (dest);
	uint32_t*		s = reinterpret_cast<uint32_t*> (src);

if (num_pixels & 0x3) {
	// not a multiple of 4?  do single pixel at a time
	for (uint32_t i = 0; i < num_pixels; i++) {
		*dest++ = *src;
		*dest++ = *src++;
		}
	return;
	}

union doubleCast {
	uint32_t		u [2];
	double	d;
} doubleCast;

for (uint32_t i = 0; i < num_pixels / 4; i++) {
	uint32_t	temp, work;
	temp = work = *s++;

	temp = ((temp >> 8) & 0x00FFFF00) | (temp & 0xFF0000FF); // 0xABCDEFGH -> 0xABABCDEF
	temp = ((temp >> 8) & 0x000000FF) | (temp & 0xFFFFFF00); // 0xABABCDEF -> 0xABABCDCD
	doubleCast.u [0] = temp;

	work = ((work << 8) & 0x00FFFF00) | (work & 0xFF0000FF); // 0xABCDEFGH -> 0xABEFGHGH
	work = ((work << 8) & 0xFF000000) | (work & 0x00FFFFFF); // 0xABEFGHGH -> 0xEFEFGHGH
	doubleCast.u [1] = work;

	*d = doubleCast.d;
	d++;
	}
}


//------------------------------------------------------------------------------

void gr_ubitmap00(int32_t x, int32_t y, CBitmap *pBm)
{
int32_t srcRowSize = pBm->RowSize ();
int32_t destRowSize = CCanvas::Current ()->RowSize () << gr_bitblt_dest_step_shift;
uint8_t* dest = &(CCanvas::Current ()->Buffer ()[ destRowSize*y+x ]);
uint8_t* src = pBm->Buffer ();

for (int32_t y1 = 0; y1 < pBm->Height (); y1++) {
	if (gr_bitblt_double)
		gr_linear_rep_movsd_2x(src, dest, pBm->Width ());
	else
		gr_linear_movsd(src, dest, pBm->Width ());
	src += srcRowSize;
	dest+= destRowSize;
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap00m(int32_t x, int32_t y, CBitmap *pBm)
{
	register int32_t y1;
	int32_t destRowSize;

	uint8_t * dest;
	uint8_t * src;

	destRowSize=CCanvas::Current ()->RowSize () << gr_bitblt_dest_step_shift;
	dest = &(CCanvas::Current ()->Buffer ()[ destRowSize*y+x ]);

	src = pBm->Buffer ();

	if (grBitBltFadeTable==NULL) {
		for (y1=0; y1 < pBm->Height (); y1++)    {
			gr_linear_rep_movsdm(src, dest, pBm->Width ());
			src += pBm->RowSize ();
			dest+= (int32_t)(destRowSize);
		}
	} else {
		for (y1=0; y1 < pBm->Height (); y1++)    {
			gr_linear_rep_movsdm_faded (src, dest, pBm->Width (), grBitBltFadeTable [y1+y], 
												 pBm->Palette (), CCanvas::Current ()->Palette ());
			src += pBm->RowSize ();
			dest+= (int32_t)(destRowSize);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012(int32_t x, int32_t y, CBitmap *pBm)
{
	register int32_t x1, y1;
	uint8_t * src;

	src = pBm->Buffer ();

	for (y1=y; y1 < (y+pBm->Height ()); y1++)    {
		for (x1=x; x1 < (x+pBm->Width ()); x1++)    {
			CCanvas::Current ()->SetColor(*src++);
			DrawPixel(x1, y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012m(int32_t x, int32_t y, CBitmap *pBm)
{
	register int32_t x1, y1;
	uint8_t * src;

	src = pBm->Buffer ();

	for (y1=y; y1 < (y+pBm->Height ()); y1++) {
		for (x1=x; x1 < (x+pBm->Width ()); x1++) {
			if (*src != TRANSPARENCY_COLOR) {
				CCanvas::Current ()->SetColor(*src);
				DrawPixel(x1, y1);
			}
			src++;
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERIC(int32_t x, int32_t y, CBitmap * pBm)
{
	register int32_t x1, y1;

	for (y1=0; y1 < pBm->Height (); y1++)    {
		for (x1=0; x1 < pBm->Width (); x1++)    {
			CCanvas::Current ()->SetColor(pBm->GetPixel (x1,y1));
			DrawPixel(x+x1, y+y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERICm(int32_t x, int32_t y, CBitmap * pBm)
{
	register int32_t x1, y1;
	uint8_t c;

	for (y1=0; y1 < pBm->Height (); y1++) {
		for (x1=0; x1 < pBm->Width (); x1++) {
			c = pBm->GetPixel (x1,y1);
			if (c != TRANSPARENCY_COLOR) {
				CCanvas::Current ()->SetColor(c);
				DrawPixel(x+x1, y+y1);
			}
		}
	}
}

//------------------------------------------------------------------------------
//@extern int32_t Interlacing_on;

// From Linear to Linear
void SWBlitToBitmap (int32_t w, int32_t h, int32_t dx, int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest)
{
uint8_t* sbits = src->Buffer ()  + (src->RowSize () * sy) + sx;
uint8_t* dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;
int32_t dstep = dest->RowSize () << gr_bitblt_dest_step_shift;

// No interlacing, copy the whole buffer.
for (int32_t i = 0; i < h; i++) {
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
void BlitToBitmapMasked (int32_t w, int32_t h, int32_t dx, int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest)
{
uint8_t* sbits = src->Buffer ()  + (src->RowSize () * sy) + sx;
uint8_t* dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;

	// No interlacing, copy the whole buffer.

if (grBitBltFadeTable == NULL) {
	for (int32_t i = 0; i < h; i++) {
		gr_linear_rep_movsdm (sbits, dbits, w);
		sbits += src->RowSize ();
		dbits += dest->RowSize ();
		}
	} 
else {
	for (int32_t i = 0; i < h; i++) {
		gr_linear_rep_movsdm_faded (sbits, dbits, w, grBitBltFadeTable [dy+i], src->Palette (), dest->Palette ());
		sbits += src->RowSize ();
		dbits += dest->RowSize ();
		}
	}
}

//------------------------------------------------------------------------------

void BlitToBitmapRLE (int32_t w, int32_t h, int32_t dx, int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest)
{
	uint8_t * dbits;
	uint8_t * sbits;
	int32_t i, data_offset;

	data_offset = 1;
	if (src->Flags () & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->Buffer ()[4 + (src->Height ()*data_offset)];

	for (i=0; i<sy; i++)
		sbits += (int32_t)(INTEL_SHORT(src->Buffer ()[4+(i*data_offset)]));

	dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline(dbits, sbits, sx, sx+w-1);
		if (src->Flags () & BM_FLAG_RLE_BIG)
			sbits += (int32_t)INTEL_SHORT(*(reinterpret_cast<int16_t*> (src->Buffer (4 + (i + sy) * data_offset))));
		else
			sbits += (int32_t)(src->Buffer ()[4+i+sy]);
		dbits += dest->RowSize () << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------

void BlitToBitmapMaskedRLE (int32_t w, int32_t h, int32_t dx, int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest)
{
	uint8_t * dbits;
	uint8_t * sbits;
	int32_t i, data_offset;

	data_offset = 1;
	if (src->Flags () & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->Buffer ()[4 + (src->Height ()*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int32_t)(INTEL_SHORT(src->Buffer ()[4+(i*data_offset)]));

	dbits = dest->Buffer () + (dest->RowSize () * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx+w-1);
		if (src->Flags () & BM_FLAG_RLE_BIG)
			sbits += (int32_t) INTEL_SHORT (*reinterpret_cast<int16_t*> (src->Buffer () + 4 + (i + sy) * data_offset));
		else
			sbits += (int32_t) (*src) [4 + i + sy];
		dbits += dest->RowSize () << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------
// in rle.c

extern void gr_rle_expand_scanline_generic(CBitmap * dest, int32_t dx, int32_t dy, uint8_t *src, int32_t x1, int32_t x2 );
extern void gr_rle_expand_scanline_generic_masked(CBitmap * dest, int32_t dx, int32_t dy, uint8_t *src, int32_t x1, int32_t x2 );
extern void gr_rle_expand_scanline_svga_masked(CBitmap * dest, int32_t dx, int32_t dy, uint8_t *src, int32_t x1, int32_t x2 );

void StretchToBitmapRLE (int32_t w, int32_t h, int32_t dx, int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest)
{
	int32_t i, data_offset;
	register int32_t y1;
	uint8_t * sbits;

	data_offset = 1;
	if (src->Flags () & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->Buffer ()[4 + (src->Height ()*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int32_t)(INTEL_SHORT(src->Buffer ()[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_generic(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->Flags () & BM_FLAG_RLE_BIG)
			sbits += (int32_t)INTEL_SHORT (*reinterpret_cast<int16_t*> (src->Buffer () + 4 + (y1 + sy) * data_offset));
		else
			sbits += (int32_t) (*src) [4 + y1 + sy];
	}
}

//------------------------------------------------------------------------------
// rescaling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

inline void ScaleLine (uint8_t *src, uint8_t *dest, int32_t ilen, int32_t olen)
{
	int32_t a = olen /ilen, b = olen % ilen;
	int32_t c = 0, i;
	uint8_t *end = dest + olen;

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
	uint8_t *s = Buffer ();
	uint8_t *d = destP->Buffer ();
	int32_t h = Height ();
	int32_t a = destP->Height () / h, b = destP->Height () % h;
	int32_t c = 0, i, y;

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

void CBitmap::Blit (CBitmap* destP, int32_t xDest, int32_t yDest, int32_t w, int32_t h, int32_t xSrc, int32_t ySrc, int32_t bTransp)
{
if (Mode () == BM_LINEAR) {
	if (destP->Mode () == BM_LINEAR) {
		if (Flags () & BM_FLAG_RLE)
			BlitToBitmapRLE (w, h, xDest, yDest, xSrc, ySrc, this, destP);
		else
			SWBlitToBitmap (w, h, xDest, yDest, xSrc, ySrc, this, destP);
		}
	else if (destP->Mode () == BM_OGL) {
		CRectangle rc;
		if (destP)
			rc = (CRectangle) (*destP);
		Render (destP ? &rc : NULL, xDest, yDest, w, h, xSrc, ySrc, w, h, bTransp);
		}
	else if (Flags () & BM_FLAG_RLE) {
		StretchToBitmapRLE (w, h, xDest, yDest, xSrc, ySrc, this, destP);
		}
	else {
		for (int32_t y1 = 0; y1 < h; y1++)  
			for (int32_t x1 = 0; x1 < w; x1++)  
				destP->DrawPixel (xDest + x1, yDest + y1, GetPixel (xSrc + x1, ySrc + y1));
		}
	}
else if (Mode () == BM_OGL) {
	if (destP->Mode () == BM_LINEAR)
		ScreenCopy (destP, xDest, yDest, w, h, xSrc, ySrc);
	}
}

//------------------------------------------------------------------------------

void CBitmap::BlitClipped (int32_t xSrc, int32_t ySrc)
{
	CBitmap* const dest = CCanvas::Current ();

	int32_t destLeft = xSrc, destRight = xSrc + Width () - 1;
	int32_t destTop = ySrc, destBottom = ySrc + Height () - 1;

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
void CBitmap::BlitClipped (CBitmap* dest, int32_t destLeft, int32_t destTop, int32_t w, int32_t h, int32_t srcLeft, int32_t srcTop)
{
if (!dest)
	dest = CCanvas::Current ();

	int32_t	destRight = destLeft + w - 1;
	int32_t	destBottom = destTop + h - 1;
	int32_t	srcRight = srcLeft + w - 1;
	int32_t	srcBottom = srcTop + h - 1;

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

// Draw bitmap pBm[x,y] into (destLeft,destTop)-(destRight,destBottom)
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
	CCanvas* const dest = CCanvas::Current ();

#if 1
Render (NULL, 0, 0, dest->Width (), dest->Height (), 0, 0, Width (), Height (), (m_info.props.flags & BM_FLAG_TRANSPARENT) != 0, 0);
#else
if ((Mode () == BM_LINEAR) && (dest->Mode () == BM_OGL)) {
	Render (dest, 0, 0, dest->Width (), dest->Height (), 0, 0, Width (), Height (), (m_info.props.flags & BM_FLAG_TRANSPARENT) != 0, 0);
	}
else if (dest->Mode () != BM_LINEAR) {
	CBitmap *tmp = CBitmap::Create (0, dest->Width (), dest->Height (), 1);
	BlitScaled (tmp);
	tmp->BlitClipped (0, 0);
	delete tmp;
	}
else
	BlitScaled (dest);
#endif
}

//------------------------------------------------------------------------------

