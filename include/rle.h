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
/*
 *
 * Protypes for rle functions.
 *
 * Old Log:
 * Revision 1.5  1995/01/14  11:32:20  john
 * Added RLECacheFlush function.
 *
 * Revision 1.4  1994/11/10  13:16:01  matt
 * Added includes
 *
 * Revision 1.3  1994/11/09  19:53:51  john
 * Added texture rle caching.
 *
 * Revision 1.2  1994/11/09  16:35:18  john
 * First version with working RLE bitmaps.
 *
 * Revision 1.1  1994/11/09  12:40:17  john
 * Initial revision
 *
 *
 */

#ifndef _RLE_H
#define _RLE_H

#include "pstypes.h"
#include "gr.h"

void gr_rle_decode( uint8_t * src, uint8_t * dest );
int32_t gr_rle_encode( int32_t org_size, uint8_t *src, uint8_t *dest );
int32_t gr_rle_getsize( int32_t org_size, uint8_t *src );
uint8_t * r_rle_find_xth_pixel( uint8_t *src, int32_t x,int32_t * count, uint8_t color );
int32_t gr_bitmap_rle_compress( CBitmap * bmp );
void gr_rle_expand_scanline_masked( uint8_t *dest, uint8_t *src, int32_t x1, int32_t x2  );
void gr_rle_expand_scanline( uint8_t *dest, uint8_t *src, int32_t x1, int32_t x2  );

CBitmap * rle_expand_texture( CBitmap * pBm );

void RLECacheFlush();

void rle_swap_0_255(CBitmap *pBm);

int32_t rle_remap (CBitmap *pBm, uint8_t *colorMap, int32_t maxLen);

int32_t rle_expand (CBitmap *pBm, uint8_t *colorMap, int32_t bSwapTranspColor);

void _CDECL_ RLECacheClose (void);

#endif
