/* $Id: bitblt.c,v 1.13 2003/12/08 21:21:16 btb Exp $ */
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

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "mono.h"
#include "byteswap.h"       // because of rle code that has short for row offsets
#include "inferno.h"
#include "bitmap.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "error.h"
#include "ogl_defs.h"

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
ubyte *grBitBltFadeTable=NULL;

extern void gr_vesa_bitmap(grsBitmap * source, grsBitmap * dest, int x, int y);

void gr_linear_movsd(ubyte * source, ubyte * dest, unsigned int nbytes);
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd

//------------------------------------------------------------------------------

#define THRESHOLD   8

#ifndef _DEBUG
#define test_byteblit   0
#else
ubyte test_byteblit = 0;
#endif

void gr_linear_movsd(ubyte * src, ubyte * dest, unsigned int num_pixels)
{
	unsigned int i;
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
	s = (double *)src;
	d = (double *)dest;
	for (i = 0; i < n; i++)
		*d++ = *s++;
	s1 = (ubyte *)s;
	d1 = (ubyte *)d;
	for (i = 0; i < r; i++)
		*d1++ = *s1++;
}

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels);

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels)
{
	unsigned int i;
	for (i=0; i<num_pixels; i++) {
		if (*src != TRANSPARENCY_COLOR)
			*dest = *src;
		dest++;
		src++;
	}
}

//------------------------------------------------------------------------------

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, 
													ubyte fadeValue, ubyte *srcPalette, ubyte *destPalette);

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, 
													ubyte fadeValue, ubyte *srcPalette, ubyte *destPalette)
{
	int	i;
	short c;
	ubyte *fade_base;
	float fade = (float) fadeValue / 31.0f;

	if (!destPalette)
		destPalette = srcPalette;
	fade_base = grFadeTable + (fadeValue * 256);
	for (i=num_pixels; i != 0; i--) {
		c= (short) *src;
		if ((ubyte) c != (ubyte) TRANSPARENCY_COLOR) {
#if 1
			c *= 3;
			c = GrFindClosestColor (destPalette, 
											(ubyte) (srcPalette [c] * fade + 0.5), 
											(ubyte) (srcPalette [c + 1] * fade + 0.5), 
											(ubyte) (srcPalette [c + 2] * fade + 0.5));
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

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels);

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_pixels)
{
	double  *d = (double *)dest;
	uint    *s = (uint *)src;
	uint   doubletemp[2];
	uint    temp, work;
	unsigned int     i;

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

		*d = *((double *) doubletemp);
		d++;
	}
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void gr_ubitmap00(int x, int y, grsBitmap *bmP)
{
	register int y1;
	int dest_rowSize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowSize=grdCurCanv->cvBitmap.bmProps.rowSize << gr_bitblt_dest_step_shift;
	dest = &(grdCurCanv->cvBitmap.bmTexBuf[ dest_rowSize*y+x ]);

	src = bmP->bmTexBuf;

	for (y1=0; y1 < bmP->bmProps.h; y1++)    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x(src, dest, bmP->bmProps.w);
		else
			gr_linear_movsd(src, dest, bmP->bmProps.w);
		src += bmP->bmProps.rowSize;
		dest+= (int)(dest_rowSize);
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap00m(int x, int y, grsBitmap *bmP)
{
	register int y1;
	int dest_rowSize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowSize=grdCurCanv->cvBitmap.bmProps.rowSize << gr_bitblt_dest_step_shift;
	dest = &(grdCurCanv->cvBitmap.bmTexBuf[ dest_rowSize*y+x ]);

	src = bmP->bmTexBuf;

	if (grBitBltFadeTable==NULL) {
		for (y1=0; y1 < bmP->bmProps.h; y1++)    {
			gr_linear_rep_movsdm(src, dest, bmP->bmProps.w);
			src += bmP->bmProps.rowSize;
			dest+= (int)(dest_rowSize);
		}
	} else {
		for (y1=0; y1 < bmP->bmProps.h; y1++)    {
			gr_linear_rep_movsdm_faded(src, dest, bmP->bmProps.w, grBitBltFadeTable[y1+y], 
												bmP->bmPalette, grdCurCanv->cvBitmap.bmPalette);
			src += bmP->bmProps.rowSize;
			dest+= (int)(dest_rowSize);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012(int x, int y, grsBitmap *bm)
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bmTexBuf;

	for (y1=y; y1 < (y+bm->bmProps.h); y1++)    {
		for (x1=x; x1 < (x+bm->bmProps.w); x1++)    {
			GrSetColor(*src++);
			gr_upixel(x1, y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmap012m(int x, int y, grsBitmap *bm)
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bmTexBuf;

	for (y1=y; y1 < (y+bm->bmProps.h); y1++) {
		for (x1=x; x1 < (x+bm->bmProps.w); x1++) {
			if (*src != TRANSPARENCY_COLOR) {
				GrSetColor(*src);
				gr_upixel(x1, y1);
			}
			src++;
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERIC(int x, int y, grsBitmap * bm)
{
	register int x1, y1;

	for (y1=0; y1 < bm->bmProps.h; y1++)    {
		for (x1=0; x1 < bm->bmProps.w; x1++)    {
			GrSetColor(gr_gpixel(bm,x1,y1));
			gr_upixel(x+x1, y+y1);
		}
	}
}

//------------------------------------------------------------------------------

void gr_ubitmapGENERICm(int x, int y, grsBitmap * bm)
{
	register int x1, y1;
	ubyte c;

	for (y1=0; y1 < bm->bmProps.h; y1++) {
		for (x1=0; x1 < bm->bmProps.w; x1++) {
			c = gr_gpixel(bm,x1,y1);
			if (c != TRANSPARENCY_COLOR) {
				GrSetColor(c);
				gr_upixel(x+x1, y+y1);
			}
		}
	}
}

//------------------------------------------------------------------------------
//@extern int Interlacing_on;

// From Linear to Linear
void gr_bm_ubitblt00(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int src_bm_rowSize_2, dest_bm_rowSize_2;
	int dstep;

	int i;

	sbits =   src->bmTexBuf  + (src->bmProps.rowSize * sy) + sx;
	dbits =   dest->bmTexBuf + (dest->bmProps.rowSize * dy) + dx;

	dstep = dest->bmProps.rowSize << gr_bitblt_dest_step_shift;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x(sbits, dbits, w);
		else
			gr_linear_movsd(sbits, dbits, w);
		sbits += src->bmProps.rowSize;
		dbits += dstep;
	}
}

//------------------------------------------------------------------------------
// From Linear to Linear Masked
void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int src_bm_rowSize_2, dest_bm_rowSize_2;

	int i;

	sbits =   src->bmTexBuf  + (src->bmProps.rowSize * sy) + sx;
	dbits =   dest->bmTexBuf + (dest->bmProps.rowSize * dy) + dx;

	// No interlacing, copy the whole buffer.

	if (grBitBltFadeTable==NULL) {
		for (i=0; i < h; i++)    {
			gr_linear_rep_movsdm(sbits, dbits, w);
			sbits += src->bmProps.rowSize;
			dbits += dest->bmProps.rowSize;
		}
	} else {
		for (i=0; i < h; i++)    {
			gr_linear_rep_movsdm_faded(sbits, dbits, w, grBitBltFadeTable[dy+i], src->bmPalette, dest->bmPalette);
			sbits += src->bmProps.rowSize;
			dbits += dest->bmProps.rowSize;
		}
	}
}


//------------------------------------------------------------------------------

extern void gr_lbitblt(grsBitmap * source, grsBitmap * dest, int height, int width);

// Clipped bitmap ...

//------------------------------------------------------------------------------

void GrBitmap (int x, int y, grsBitmap *bm)
{
	grsBitmap * const scr = &grdCurCanv->cvBitmap;
	int dx1=x, dx2=x+bm->bmProps.w-1;
	int dy1=y, dy2=y+bm->bmProps.h-1;
	int sx=0, sy=0;

if ((dx1 >= scr->bmProps.w) || (dx2 < 0)) 
	return;
if ((dy1 >= scr->bmProps.h) || (dy2 < 0)) 
	return;
if (dx1 < 0) { 
	sx = -dx1; 
	dx1 = 0; 
	}
if (dy1 < 0) { 
	sy = -dy1; 
	dy1 = 0; 
	}
if (dx2 >= scr->bmProps.w)
	dx2 = scr->bmProps.w-1;
if (dy2 >= scr->bmProps.h)
	dy2 = scr->bmProps.h-1;

// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
GrBmUBitBlt (dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, &grdCurCanv->cvBitmap, 0);
}

//-NOT-used // From linear to SVGA
//-NOT-used void gr_bm_ubitblt02_2x(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * sbits;
//-NOT-used
//-NOT-used 	unsigned int offset, EndingOffset, VideoLocation;
//-NOT-used
//-NOT-used 	int sbpr, dbpr, y1, page, BytesToMove;
//-NOT-used
//-NOT-used 	sbpr = src->bmProps.rowSize;
//-NOT-used
//-NOT-used 	dbpr = dest->bmProps.rowSize << gr_bitblt_dest_step_shift;
//-NOT-used
//-NOT-used 	VideoLocation = (unsigned int)dest->bmTexBuf + (dest->bmProps.rowSize * dy) + dx;
//-NOT-used
//-NOT-used 	sbits = src->bmTexBuf + (sbpr*sy) + sx;
//-NOT-used
//-NOT-used 	for (y1=0; y1 < h; y1++)    {
//-NOT-used
//-NOT-used 		page    = VideoLocation >> 16;
//-NOT-used 		offset  = VideoLocation & 0xFFFF;
//-NOT-used
//-NOT-used 		gr_vesa_setpage(page);
//-NOT-used
//-NOT-used 		EndingOffset = offset+w-1;
//-NOT-used
//-NOT-used 		if (EndingOffset <= 0xFFFF)
//-NOT-used 		{
//-NOT-used 			gr_linear_rep_movsd_2x((void *)sbits, (void *)(offset+0xA0000), w);
//-NOT-used
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used 		else
//-NOT-used 		{
//-NOT-used 			BytesToMove = 0xFFFF-offset+1;
//-NOT-used
//-NOT-used 			gr_linear_rep_movsd_2x((void *)sbits, (void *)(offset+0xA0000), BytesToMove);
//-NOT-used
//-NOT-used 			page++;
//-NOT-used 			gr_vesa_setpage(page);
//-NOT-used
//-NOT-used 			gr_linear_rep_movsd_2x((void *)(sbits+BytesToMove/2), (void *)0xA0000, EndingOffset - 0xFFFF);
//-NOT-used
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used
//-NOT-used
//-NOT-used 	}
//-NOT-used }


//-NOT-used // From Linear to Linear
//-NOT-used void gr_bm_ubitblt00_2x(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * dbits;
//-NOT-used 	unsigned char * sbits;
//-NOT-used 	//int src_bm_rowSize_2, dest_bm_rowSize_2;
//-NOT-used
//-NOT-used 	int i;
//-NOT-used
//-NOT-used 	sbits =   src->bmTexBuf  + (src->bmProps.rowSize * sy) + sx;
//-NOT-used 	dbits =   dest->bmTexBuf + (dest->bmProps.rowSize * dy) + dx;
//-NOT-used
//-NOT-used 	// No interlacing, copy the whole buffer.
//-NOT-used 	for (i=0; i < h; i++)    {
//-NOT-used 		gr_linear_rep_movsd_2x(sbits, dbits, w);
//-NOT-used
//-NOT-used 		sbits += src->bmProps.rowSize;
//-NOT-used 		dbits += dest->bmProps.rowSize << gr_bitblt_dest_step_shift;
//-NOT-used 	}
//-NOT-used }

//------------------------------------------------------------------------------

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bmProps.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bmTexBuf[4 + (src->bmProps.h*data_offset)];

	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bmTexBuf[4+(i*data_offset)]));

	dbits = dest->bmTexBuf + (dest->bmProps.rowSize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline(dbits, sbits, sx, sx+w-1);
		if (src->bmProps.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bmTexBuf[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bmTexBuf[4+i+sy]);
		dbits += dest->bmProps.rowSize << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------

void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bmProps.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bmTexBuf[4 + (src->bmProps.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bmTexBuf[4+(i*data_offset)]));

	dbits = dest->bmTexBuf + (dest->bmProps.rowSize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++)    {
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx+w-1);
		if (src->bmProps.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bmTexBuf[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bmTexBuf[4+i+sy]);
		dbits += dest->bmProps.rowSize << gr_bitblt_dest_step_shift;
	}
}

//------------------------------------------------------------------------------
// in rle.c

extern void gr_rle_expand_scanline_generic(grsBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );
extern void gr_rle_expand_scanline_generic_masked(grsBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );
extern void gr_rle_expand_scanline_svga_masked(grsBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );

void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//con_printf(0, "SVGA RLE!\n");

	data_offset = 1;
	if (src->bmProps.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bmTexBuf[4 + (src->bmProps.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bmTexBuf[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_generic(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->bmProps.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bmTexBuf[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bmTexBuf[4+y1+sy];
	}

}

//------------------------------------------------------------------------------

void gr_bm_ubitblt0xm_rle(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//con_printf(0, "SVGA RLE!\n");

	data_offset = 1;
	if (src->bmProps.flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bmTexBuf[4 + (src->bmProps.h*data_offset)];
	for (i=0; i<sy; i++)
		sbits += (int)(INTEL_SHORT(src->bmTexBuf[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++)    {
		gr_rle_expand_scanline_generic_masked(dest, dx, dy+y1,  sbits, sx, sx+w-1 );
		if (src->bmProps.flags & BM_FLAG_RLE_BIG)
			sbits += (int)INTEL_SHORT(*((short *)&(src->bmTexBuf[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bmTexBuf[4+y1+sy];
	}

}

//------------------------------------------------------------------------------

void GrBmUBitBlt (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest, int bTransp)
{
	register int x1, y1;

if ((src->bmProps.nType == BM_LINEAR) && (dest->bmProps.nType == BM_LINEAR)) {
	if (src->bmProps.flags & BM_FLAG_RLE)
		gr_bm_ubitblt00_rle(w, h, dx, dy, sx, sy, src, dest);
	else
		gr_bm_ubitblt00(w, h, dx, dy, sx, sy, src, dest);
	return;
	}

if ((src->bmProps.nType == BM_LINEAR) && (dest->bmProps.nType == BM_OGL)) {
	OglUBitBlt (w, h, dx, dy, sx, sy, src, dest, bTransp);
	return;
	}
if ((src->bmProps.nType == BM_OGL) && (dest->bmProps.nType == BM_LINEAR)) {
	OglUBitBltToLinear (w, h, dx, dy, sx, sy, src, dest);
	return;
	}
if ((src->bmProps.nType == BM_OGL) && (dest->bmProps.nType == BM_OGL)) {
	OglUBitBltCopy (w, h, dx, dy, sx, sy, src, dest);
	return;
	}
if ((src->bmProps.flags & BM_FLAG_RLE) && (src->bmProps.nType == BM_LINEAR)) {
	gr_bm_ubitblt0x_rle (w, h, dx, dy, sx, sy, src, dest);
	return;
	}
for (y1=0; y1 < h; y1++)  
	for (x1=0; x1 < w; x1++)  
		gr_bm_pixel(dest, dx+x1, dy+y1, gr_gpixel(src,sx+x1,sy+y1));
}

//------------------------------------------------------------------------------

void GrBmBitBlt(int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest)
{
	int	dx1 = dx, 
			dx2 = dx + dest->bmProps.w - 1;
	int	dy1 = dy, 
			dy2 = dy + dest->bmProps.h - 1;
	int	sx1 = sx, 
			sx2 = sx + src->bmProps.w - 1;
	int	sy1 = sy, 
			sy2 = sy + src->bmProps.h - 1;

if ((dx1 >= dest->bmProps.w) || (dx2 < 0)) 
	return;
if ((dy1 >= dest->bmProps.h) || (dy2 < 0)) 
	return;
if (dx1 < 0) { 
	sx1 += -dx1; 
	dx1 = 0; 
	}
if (dy1 < 0) { 
	sy1 += -dy1; 
	dy1 = 0; 
	}
if (dx2 >= dest->bmProps.w) { 
	dx2 = dest->bmProps.w-1; 
	}
if (dy2 >= dest->bmProps.h) { 
	dy2 = dest->bmProps.h-1; 
	}

if ((sx1 >= src->bmProps.w) || (sx2 < 0)) 
	return;
if ((sy1 >= src->bmProps.h) || (sy2 < 0)) 
	return;
if (sx1 < 0) { 
	dx1 += -sx1; sx1 = 0; 
	}
if (sy1 < 0) { 
	dy1 += -sy1; sy1 = 0; 
	}
if (sx2 >= src->bmProps.w) { 
	sx2 = src->bmProps.w-1; 
	}
if (sy2 >= src->bmProps.h) { 
	sy2 = src->bmProps.h-1; 
	}

// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
if (dx2-dx1+1 < w)
	w = dx2-dx1+1;
if (dy2-dy1+1 < h)
	h = dy2-dy1+1;
if (sx2-sx1+1 < w)
	w = sx2-sx1+1;
if (sy2-sy1+1 < h)
	h = sy2-sy1+1;
GrBmUBitBlt (w, h, dx1, dy1, sx1, sy1, src, dest, 1);
}

//------------------------------------------------------------------------------

void gr_ubitmap(int x, int y, grsBitmap *bmP)
{
	int source, dest;

	source = bmP->bmProps.nType;
	dest = TYPE;

	if (source==BM_LINEAR) {
		switch(dest)
		{
		case BM_LINEAR:
			if (bmP->bmProps.flags & BM_FLAG_RLE)
				gr_bm_ubitblt00_rle(bmP->bmProps.w, bmP->bmProps.h, x, y, 0, 0, bmP, &grdCurCanv->cvBitmap);
			else
				gr_ubitmap00(x, y, bmP);
			return;
		case BM_OGL:
			OglUBitMapM(x,y,bmP);
			return;
		default:
			gr_ubitmap012(x, y, bmP);
			return;
		}
	} else  {
		gr_ubitmapGENERIC(x, y, bmP);
	}
}

//------------------------------------------------------------------------------

void GrUBitmapM(int x, int y, grsBitmap *bmP)
{
	int source, dest;

	source = bmP->bmProps.nType;
	dest = TYPE;

	Assert(x+bmP->bmProps.w <= grdCurCanv->cv_w);
	Assert(y+bmP->bmProps.h <= grdCurCanv->cv_h);

	if (source==BM_LINEAR) {
		switch(dest)
		{
		case BM_LINEAR:
			if (bmP->bmProps.flags & BM_FLAG_RLE)
				gr_bm_ubitblt00m_rle(bmP->bmProps.w, bmP->bmProps.h, x, y, 0, 0, bmP, &grdCurCanv->cvBitmap);
			else
				gr_ubitmap00m(x, y, bmP);
			return;
		case BM_OGL:
			OglUBitMapM(x,y,bmP);
			return;
		default:
			gr_ubitmap012m(x, y, bmP);
			return;
		}
	} else {
		gr_ubitmapGENERICm(x, y, bmP);
	}
}

//------------------------------------------------------------------------------

void GrBitmapM(int x, int y, grsBitmap *bmP, int bTransp)
{
	int dx1=x, dx2=x+bmP->bmProps.w-1;
	int dy1=y, dy2=y+bmP->bmProps.h-1;
	int sx=0, sy=0;

if ((dx1 >= grdCurCanv->cvBitmap.bmProps.w) || (dx2 < 0)) 
	return;
if ((dy1 >= grdCurCanv->cvBitmap.bmProps.h) || (dy2 < 0)) 
	return;
if (dx1 < 0) { 
	sx = -dx1; 
	dx1 = 0; 
	}
if (dy1 < 0) { 
	sy = -dy1; 
	dy1 = 0; 
	}
if (dx2 >= grdCurCanv->cvBitmap.bmProps.w)
	dx2 = grdCurCanv->cvBitmap.bmProps.w-1;
if (dy2 >= grdCurCanv->cvBitmap.bmProps.h)
	dy2 = grdCurCanv->cvBitmap.bmProps.h-1; 
// Draw bitmap bmP[x,y] into (dx1,dy1)-(dx2,dy2)

if ((bmP->bmProps.nType == BM_LINEAR) && (grdCurCanv->cvBitmap.bmProps.nType == BM_LINEAR)) {
	if (bmP->bmProps.flags & BM_FLAG_RLE)
		gr_bm_ubitblt00m_rle(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cvBitmap);
	else
		gr_bm_ubitblt00m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cvBitmap);
	return;
	}
GrBmUBitBltM (dx2-dx1 + 1,dy2-dy1 + 1, dx1, dy1, sx, sy, bmP, &grdCurCanv->cvBitmap, bTransp);
}

//------------------------------------------------------------------------------

void GrBmUBitBltM (int w, int h, int dx, int dy, int sx, int sy, grsBitmap * src, grsBitmap * dest, int bTransp)
{
	register int x1, y1;
	ubyte c;

if ((src->bmProps.nType == BM_LINEAR) && (dest->bmProps.nType == BM_OGL))
	OglUBitBlt (w, h, dx, dy, sx, sy, src, dest, bTransp);
else if ((src->bmProps.nType == BM_OGL) && (dest->bmProps.nType == BM_LINEAR))
	OglUBitBltToLinear (w, h, dx, dy, sx, sy, src, dest);
else if ((src->bmProps.nType == BM_OGL) && (dest->bmProps.nType == BM_OGL))
	OglUBitBltCopy (w, h, dx, dy, sx, sy, src, dest);
else
	for (y1=0; y1 < h; y1++) {
		for (x1=0; x1 < w; x1++) {
			if ((c=gr_gpixel(src,sx+x1,sy+y1))!=TRANSPARENCY_COLOR)
				gr_bm_pixel(dest, dx+x1, dy+y1,c);
		}
	}
}

//------------------------------------------------------------------------------
// rescalling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

inline void scale_line(sbyte *in, sbyte *out, int ilen, int olen)
{
	int a = olen/ilen, b = olen%ilen;
	int c = 0, i;
	sbyte *end = out + olen;
	while(out<end) {
		i = a;
		c += b;
		if(c >= ilen) {
			c -= ilen;
			goto inside;
		}
		while(--i>=0) {
		inside:
			*out++ = *in;
		}
		in++;
	}
}

//------------------------------------------------------------------------------

void GrBitmapScaleTo(grsBitmap *src, grsBitmap *dst)
{
	sbyte *s = (sbyte *) (src->bmTexBuf);
	sbyte *d = (sbyte *) (dst->bmTexBuf);
	int h = src->bmProps.h;
	int a = dst->bmProps.h/h, b = dst->bmProps.h%h;
	int c = 0, i, y;

	for(y=0; y<h; y++) {
		i = a;
		c += b;
		if(c >= h) {
			c -= h;
			goto inside;
		}
		while(--i>=0) {
		inside:
			scale_line(s, d, src->bmProps.w, dst->bmProps.w);
			d += dst->bmProps.rowSize;
		}
		s += src->bmProps.rowSize;
	}
}

//------------------------------------------------------------------------------

void ShowFullscreenImage (grsBitmap *src)
{
	grsBitmap * const dest = &grdCurCanv->cvBitmap;

if ((src->bmProps.nType == BM_LINEAR) && (dest->bmProps.nType == BM_OGL)) {
	if (!gameStates.render.bBlendBackground)
		glDisable (GL_BLEND);
	OglUBitBltI (dest->bmProps.w, dest->bmProps.h, 0, 0, src->bmProps.w, src->bmProps.h, 0, 0, src, dest, 0, 0, 1.0f);
	if (!gameStates.render.bBlendBackground)
		glEnable (GL_BLEND);
	return;
	}
if(dest->bmProps.nType != BM_LINEAR) {
	grsBitmap *tmp = GrCreateBitmap (dest->bmProps.w, dest->bmProps.h, 1);
	GrBitmapScaleTo(src, tmp);
	GrBitmap(0, 0, tmp);
	GrFreeBitmap (tmp);
	return;
	}
GrBitmapScaleTo (src, dest);
}

//------------------------------------------------------------------------------

