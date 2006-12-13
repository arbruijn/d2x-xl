/* $Id: rle.c,v 1.17 2004/01/08 20:31:35 schaffner Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: rle.c,v 1.17 2004/01/08 20:31:35 schaffner Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "mono.h"


#include "gr.h"
#include "grdef.h"
#include "error.h"
//#include "key.h"
#include "rle.h"
#include "byteswap.h"
#include "inferno.h"

#define RLE_CODE        0xE0
#define NOT_RLE_CODE    31

#define IS_RLE_CODE(x) (( (x) & RLE_CODE) == RLE_CODE)

#define rle_stosb(_dest, _len, _color)	memset(_dest,_color,_len)

//------------------------------------------------------------------------------

void gr_rle_decode (ubyte *src, ubyte *dest)
{
	ubyte data, count = 0;

while (1) {
	data = *src++;
	if (!IS_RLE_CODE (data))
		*dest++ = data;
	else {
		if (!(count = (data & NOT_RLE_CODE)))
			return;
		data = *src++;
		memset (dest, data, count);
		}
	}
}

//------------------------------------------------------------------------------

// Given pointer to start of one scanline of rle data, uncompress it to
// dest, from source pixels x1 to x2.
void gr_rle_expand_scanline_masked (ubyte *dest, ubyte *src, int x1, int x2 )
{
	int i = 0;
	ubyte count;
	ubyte color=0;

	if (x2 < x1) return;

	count = 0;
	while (i < x1)	{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if (x1+count > x2)	{
		count = x2-x1+1;
		if (color != TRANSPARENCY_COLOR)	
			rle_stosb ((char* )dest, count, color);
		return;
	}

	if (color != TRANSPARENCY_COLOR)	
		rle_stosb ((char* )dest, count, color);
	dest += count;
	i += count;

	while (i <= x2)		{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i+count <= x2)	{
			if (color != TRANSPARENCY_COLOR)
				rle_stosb ((char* )dest, count, color);
			i += count;
			dest += count;
		} else {
			count = x2-i+1;
			if (color != TRANSPARENCY_COLOR)
				rle_stosb ((char* )dest, count, color);
			i += count;
			dest += count;
		}

	}	
}

//------------------------------------------------------------------------------

void gr_rle_expand_scanline (ubyte *dest, ubyte *src, int x1, int x2 )
{
	int i = 0;
	ubyte count;
	ubyte color=0;

	if (x2 < x1) return;

	count = 0;
	while (i < x1)	{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if (x1+count > x2)	{
		count = x2-x1+1;
		rle_stosb ((char* )dest, count, color);
		return;
	}

	rle_stosb ((char* )dest, count, color);
	dest += count;
	i += count;

	while (i <= x2)		{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i+count <= x2)	{
			rle_stosb ((char* )dest, count, color);
			i += count;
			dest += count;
		} else {
			count = x2-i+1;
			rle_stosb ((char* )dest, count, color);
			i += count;
			dest += count;
		}
	}
}

//------------------------------------------------------------------------------

int gr_rle_encode (int org_size, ubyte *src, ubyte *dest)
{
	int i;
	ubyte c, oc;
	ubyte count;
	ubyte *dest_start;

	dest_start = dest;
	oc = *src++;
	count = 1;

	for (i=1; i<org_size; i++)	{
		c = *src++;
		if (c!=oc)	{
			if (count)	{
				if ((count==1) && (! IS_RLE_CODE (oc)))	{
					*dest++ = oc;
					Assert (oc != RLE_CODE);
				} else {
					count |= RLE_CODE;
					*dest++ = count;
					*dest++ = oc;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if (count == NOT_RLE_CODE)	{
			count |= RLE_CODE;
			*dest++=count;
			*dest++=oc;
			count = 0;
		}
	}
	if (count)	{
		if ((count==1) && (! IS_RLE_CODE (oc)))	{
			*dest++ = oc;
			Assert (oc != RLE_CODE);
		} else {
			count |= RLE_CODE;
			*dest++ = count;
			*dest++ = oc;
		}
	}
	*dest++ = RLE_CODE;

	return (int) (dest - dest_start);
}

//------------------------------------------------------------------------------

int gr_rle_getsize (int org_size, ubyte *src)
{
	int i;
	ubyte c, oc;
	ubyte count;
	int dest_size=0;

	oc = *src++;
	count = 1;

	for (i=1; i<org_size; i++)	{
		c = *src++;
		if (c!=oc)	{
			if (count)	{
				if ((count==1) && (! IS_RLE_CODE (oc)))	{
					dest_size++;
				} else {
					dest_size++;
					dest_size++;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if (count == NOT_RLE_CODE)	{
			dest_size++;
			dest_size++;
			count = 0;
		}
	}
	if (count)	{
		if ((count==1) && (! IS_RLE_CODE (oc)))	{
			dest_size++;
		} else {
			dest_size++;
			dest_size++;
		}
	}
	dest_size++;

	return dest_size;
}

//------------------------------------------------------------------------------

int gr_bitmap_rle_compress (grsBitmap * bmP)
{
	int y, d1, d;
	int doffset;
	ubyte *rle_data;
	int large_rle = 0;

	// first must check to see if this is large bitmap.

	for (y=0; y<bmP->bm_props.h; y++)	{
		d1= gr_rle_getsize (bmP->bm_props.w, &bmP->bm_texBuf[bmP->bm_props.w*y]);
		if (d1 > 255) {
			large_rle = 1;
			break;
		}
	}

	rle_data=d_malloc (MAX_BMP_SIZE (bmP->bm_props.w, bmP->bm_props.h));
	if (rle_data==NULL) return 0;
	if (!large_rle)
		doffset = 4 + bmP->bm_props.h;
	else
		doffset = 4 + (2 * bmP->bm_props.h);		// each row of rle'd bitmap has short instead of byte offset now

	for (y=0; y<bmP->bm_props.h; y++)	{
		d1= gr_rle_getsize (bmP->bm_props.w, &bmP->bm_texBuf[bmP->bm_props.w*y]);
		if (( (doffset+d1) > bmP->bm_props.w*bmP->bm_props.h) || (d1 > (large_rle?32767:255))) {
			d_free (rle_data);
			return 0;
		}
		d = gr_rle_encode (bmP->bm_props.w, &bmP->bm_texBuf[bmP->bm_props.w*y], &rle_data[doffset]);
		Assert (d==d1);
		doffset	+= d;
		if (large_rle)
			* ((short *)& (rle_data[ (y*2)+4])) = (short)d;
		else
			rle_data[y+4] = d;
	}
	memcpy (	rle_data, &doffset, 4);
	memcpy (	bmP->bm_texBuf, rle_data, doffset);
	d_free (rle_data);
	bmP->bm_props.flags |= BM_FLAG_RLE;
	if (large_rle)
		bmP->bm_props.flags |= BM_FLAG_RLE_BIG;
	return 1;
}

//------------------------------------------------------------------------------

#define MAX_CACHE_BITMAPS 32

typedef struct rle_cache_element {
	grsBitmap * rle_bitmap;
	ubyte * rle_data;
	grsBitmap * expanded_bitmap;
	int last_used;
} rle_cache_element;

int rle_cache_initialized = 0;
int rle_counter = 0;
int rle_next = 0;
rle_cache_element rle_cache[MAX_CACHE_BITMAPS];

int rle_hits = 0;
int rle_misses = 0;

void _CDECL_ rle_cache_close (void)
{
if (rle_cache_initialized)	{
	int i;
	LogErr ("deleting RLE cache\n");
	rle_cache_initialized = 0;
	for (i=0; i<MAX_CACHE_BITMAPS; i++)
		GrFreeBitmap (rle_cache[i].expanded_bitmap);
	}
}

//------------------------------------------------------------------------------

void RLECacheInit ()
{
	int i;
	for (i=0; i<MAX_CACHE_BITMAPS; i++)	{
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].expanded_bitmap = GrCreateBitmap (64, 64, 0);
		rle_cache[i].last_used = 0;
		Assert (rle_cache[i].expanded_bitmap != NULL);
	}
	rle_cache_initialized = 1;
	atexit (rle_cache_close);
}

//------------------------------------------------------------------------------

void RLECacheFlush ()
{
	int i;
	for (i=0; i<MAX_CACHE_BITMAPS; i++)	{
		rle_cache [i].rle_bitmap = NULL;
		rle_cache [i].last_used = 0;
	}
}

//------------------------------------------------------------------------------

void rle_expand_texture_sub (grsBitmap * bmP, grsBitmap * rle_temp_bitmap_1)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i;
#ifdef RLE_DECODE_ASM
	unsigned char * dbits1;
#endif

sbits = bmP->bm_texBuf + 4 + bmP->bm_props.h;
dbits = rle_temp_bitmap_1->bm_texBuf;
rle_temp_bitmap_1->bm_props.flags = bmP->bm_props.flags & (~BM_FLAG_RLE);
for (i=0; i < bmP->bm_props.h; i++) {
	gr_rle_decode (sbits, dbits);
	sbits += (int) bmP->bm_texBuf [4+i];
	dbits += bmP->bm_props.w;
	}
}

//------------------------------------------------------------------------------

grsBitmap *rle_expand_texture (grsBitmap * bmP)
{
	int i;
	int lowest_count, lc;
	int least_recently_used;

if (!rle_cache_initialized) 
	RLECacheInit ();

Assert (!(bmP->bm_props.flags & BM_FLAG_PAGED_OUT));

lc = rle_counter;
rle_counter++;
if (rle_counter < lc) {
	for (i=0; i<MAX_CACHE_BITMAPS; i++)	{
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].last_used = 0;
		}
	}

lowest_count = rle_cache[rle_next].last_used;
least_recently_used = rle_next;
rle_next++;
if (rle_next >= MAX_CACHE_BITMAPS)
	rle_next = 0;
for (i = 0; i < MAX_CACHE_BITMAPS; i++) {
	if (rle_cache [i].rle_bitmap == bmP) {
		rle_hits++;
		rle_cache[i].last_used = rle_counter;
		return rle_cache [i].expanded_bitmap;
		}
	if (rle_cache [i].last_used < lowest_count) {
		lowest_count = rle_cache[i].last_used;
		least_recently_used = i;
		}
	}
Assert (bmP->bm_props.w<=64 && bmP->bm_props.h<=64); //dest buffer is 64x64
rle_misses++;
rle_expand_texture_sub (bmP, rle_cache[least_recently_used].expanded_bitmap);
rle_cache[least_recently_used].rle_bitmap = bmP;
rle_cache[least_recently_used].last_used = rle_counter;
return rle_cache[least_recently_used].expanded_bitmap;
}

//------------------------------------------------------------------------------

void gr_rle_expand_scanline_generic (grsBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2)
{
	int i = 0, j;
	int count;
	ubyte color=0;

	if (x2 < x1) return;

	count = 0;
	while (i < x1)	{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.

	if (x1+count > x2)	{
		count = x2-x1+1;
		for (j=0; j<count; j++)
			gr_bm_pixel (dest, dx++, dy, color);
		return;
	}

	for (j=0; j<count; j++)
		gr_bm_pixel (dest, dx++, dy, color);
	i += count;

	while (i <= x2)		{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i+count <= x2)	{
			for (j=0; j<count; j++)
				gr_bm_pixel (dest, dx++, dy, color);
			i += count;
		} else {
			count = x2-i+1;
			for (j=0; j<count; j++)
				gr_bm_pixel (dest, dx++, dy, color);
			i += count;
		}
	}
}

//------------------------------------------------------------------------------

void gr_rle_expand_scanline_generic_masked (grsBitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 )
{
	int i = 0, j;
	int count;
	ubyte color = 0;

	if (x2 < x1) return;

	count = 0;
	while (i < x1)	{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.

	if (x1+count > x2)	{
		count = x2-x1+1;
		if (color != TRANSPARENCY_COLOR) {
			for (j=0; j<count; j++)
				gr_bm_pixel (dest, dx++, dy, color);
		}
		return;
	}

	if (color != TRANSPARENCY_COLOR) {
		for (j=0; j<count; j++)
			gr_bm_pixel (dest, dx++, dy, color);
	} else
		dx += count;
	i += count;

	while (i <= x2)		{
		color = *src++;
		if (color == RLE_CODE) return;
		if (IS_RLE_CODE (color))	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if (i+count <= x2)	{
			if (color != TRANSPARENCY_COLOR) {
				for (j=0; j<count; j++)
					gr_bm_pixel (dest, dx++, dy, color);
			} else
				dx += count;
			i += count;
		} else {
			count = x2-i+1;
			if (color != TRANSPARENCY_COLOR) {
				for (j=0; j<count; j++)
					gr_bm_pixel (dest, dx++, dy, color);
			} else
				dx += count;
			i += count;
		}
	}
}

//------------------------------------------------------------------------------
// swaps entries 0 and 255 in an RLE bitmap without uncompressing it

void rle_swap_0_255 (grsBitmap *bmP)
{
	int h, i, j, len, rle_big;
	unsigned char *ptr, *ptr2, *temp, *start;
	unsigned short nLineSize;

rle_big = bmP->bm_props.flags & BM_FLAG_RLE_BIG;
temp = d_malloc (MAX_BMP_SIZE (bmP->bm_props.w, bmP->bm_props.h));
if (rle_big) {                  // set ptrs to first lines
	ptr = bmP->bm_texBuf + 4 + 2 * bmP->bm_props.h;
	ptr2 = temp + 4 + 2 * bmP->bm_props.h;
} else {
	ptr = bmP->bm_texBuf + 4 + bmP->bm_props.h;
	ptr2 = temp + 4 + bmP->bm_props.h;
}
for (i = 0; i < bmP->bm_props.h; i++) {
	start = ptr2;
	if (rle_big)
		nLineSize = INTEL_SHORT (* ((unsigned short *) &bmP->bm_texBuf [4 + 2 * i]));
	else
		nLineSize = bmP->bm_texBuf [4 + i];
	for (j = 0; j < nLineSize; j++) {
		h = ptr [j];
		if (!IS_RLE_CODE (h)) {
			if (h == 0) {
				*ptr2++ = RLE_CODE | 1;
				*ptr2++ = 255;
				}
			else
				*ptr2++ = ptr[j];
			}
		else {
			*ptr2++ = h;
			if ((h & NOT_RLE_CODE) == 0)
				break;
			h = ptr [++j];
			if (h == 0)
				*ptr2++ = 255;
			else if (h == 255)
				*ptr2++ = 0;
			else
				*ptr2++ = h;
			}
		}
	if (rle_big)                // set line size
		* ((unsigned short *)&temp[4 + 2 * i]) = INTEL_SHORT ((short) (ptr2 - start));
	else
		temp[4 + i] = (unsigned char) (ptr2 - start);
	ptr += nLineSize;           // go to next line
	}
len = (int) (ptr2 - temp);
* ((int *)temp) = len;           // set total size
memcpy (bmP->bm_texBuf, temp, len);
d_free (temp);
}

//------------------------------------------------------------------------------
// remaps all entries using colorMap in an RLE bitmap without uncompressing it

int rle_remap (grsBitmap *bmP, ubyte *colorMap, int maxLen)
{
	int				h, i, j, len, bBigRLE;
	unsigned char	*pSrc, *pDest, *remapBuf, *start;
	unsigned short nLineSize;

bBigRLE = bmP->bm_props.flags & BM_FLAG_RLE_BIG;
remapBuf = d_malloc (MAX_BMP_SIZE (bmP->bm_props.w, bmP->bm_props.h) + 30000);
if (bBigRLE) {                  // set ptrs to first lines
	pSrc = bmP->bm_texBuf + 4 + 2 * bmP->bm_props.h;
	pDest = remapBuf + 4 + 2 * bmP->bm_props.h;
	}
else {
	pSrc = bmP->bm_texBuf + 4 + bmP->bm_props.h;
	pDest = remapBuf + 4 + bmP->bm_props.h;
	}
for (i = 0; i < bmP->bm_props.h; i++) {
	start = pDest;
	if (bBigRLE)
		nLineSize = INTEL_SHORT (*((unsigned short *) (bmP->bm_texBuf + 4 + 2 * i)));
	else
		nLineSize = bmP->bm_texBuf [4 + i];
	for (j = 0; j < nLineSize; j++) {
		h = pSrc [j];
		if (!IS_RLE_CODE (h)) {
			if (IS_RLE_CODE (colorMap [h])) 
				*pDest++ = RLE_CODE | 1; // add "escape sequence"
			*pDest++ = colorMap [h]; // translate
			}
		else {
			*pDest++ = h; // just copy current rle code
			if ((h & NOT_RLE_CODE) == 0)
				break;
			*pDest++ = colorMap [pSrc [++j]]; // translate
			}
		}
	if (bBigRLE)                // set line size
		*((unsigned short *) (remapBuf + 4 + 2 * i)) = INTEL_SHORT ((short) (pDest - start));
	else
		remapBuf [4 + i] = (unsigned char) (pDest - start);
	pSrc += nLineSize;           // go to next line
	}
len = (int) (pDest - remapBuf);
Assert (len <= bmP->bm_props.h * bmP->bm_props.rowsize);
* ((int *)remapBuf) = len;           // set total size
memcpy (bmP->bm_texBuf, remapBuf, len);
d_free (remapBuf);
return len;
}

//------------------------------------------------------------------------------

int rle_expand (grsBitmap *bmP, ubyte *colorMap, int bSwap0255)
{
	ubyte				*expandBuf, *pSrc, *pDest;
	ubyte				c, h;
	int				i, j, l, bBigRLE;
	unsigned short nLineSize;

if (!(bmP->bm_props.flags & BM_FLAG_RLE))
	return bmP->bm_props.h * bmP->bm_props.rowsize;
if (!(expandBuf = d_malloc (bmP->bm_props.h * bmP->bm_props.rowsize)))
	return -1;

bBigRLE = bmP->bm_props.flags & BM_FLAG_RLE_BIG;
if (bBigRLE)
	pSrc = bmP->bm_texBuf + 4 + 2 * bmP->bm_props.h;
else
	pSrc = bmP->bm_texBuf + 4 + bmP->bm_props.h;
pDest = expandBuf;
for (i = 0; i < bmP->bm_props.h; i++, pSrc += nLineSize) {
	if (bBigRLE)
		nLineSize = INTEL_SHORT (*((unsigned short *) (bmP->bm_texBuf + 4 + 2 * i)));
	else
		nLineSize = bmP->bm_texBuf [4 + i];
	for (j = 0; j < nLineSize; j++) {
		h = pSrc [j];
		if (!IS_RLE_CODE (h)) {
			c = colorMap ? colorMap [h] : h; // translate
			if (bSwap0255)
				if (c == 0)
					c = 255;
				else if (c == 255)
					c = 0;
			if (pDest - expandBuf > bmP->bm_props.h * bmP->bm_props.rowsize) {
				d_free (expandBuf);
				return -1;
				}	
			*pDest++ = c;
			}
		else if (l = (h & NOT_RLE_CODE)) {
			c = pSrc [++j];
			if (colorMap)
				c = colorMap [c];
			if (bSwap0255)
				if (c == 0)
					c = 255;
				else if (c == 255)
					c = 0;
			if (pDest - expandBuf + l > bmP->bm_props.h * bmP->bm_props.rowsize) {
				d_free (expandBuf);
				return -1;
				}	
			memset (pDest, c, l);
			pDest += l;
			}
		}
	}
l = (int) (pDest - expandBuf);
Assert (l <= bmP->bm_props.h * bmP->bm_props.rowsize);
memcpy (bmP->bm_texBuf, expandBuf, l);
bmP->bm_props.flags &= ~(BM_FLAG_RLE | BM_FLAG_RLE_BIG);
d_free (expandBuf);
return l;
}

//------------------------------------------------------------------------------
// eof