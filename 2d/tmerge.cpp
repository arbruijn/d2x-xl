/*
 *
 * tmerge.c - C Texture merge routines for use with D1X
 * Ripped from ldescent by <dph-man@iname.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"
#include "gr.h"

void GrMergeTextures( uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale )
{
	int32_t x,y,h;
	uint8_t c;
	for (y=0;y<height;y++) 
		for (x=0;x<width;x++) {
			h = (width*y+x) / scale;
			c=upper[h];
			if (c==TRANSPARENCY_COLOR)
				c=lower[h];
			*dest++=c;
		}
}

void GrMergeTextures1( uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale )
{
	int32_t x,y;
	uint8_t c;
	for (y=0; y<height; y++ )
		for (x=0; x<width; x++ ) {
			c = upper[ (width*x+(height-1-y)) / scale ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ (width*y+x) / scale ];
			*dest++ = c;
		}
}

void GrMergeTextures2( uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale )
{
 int32_t x,y;
 uint8_t c;
	for (y=0; y<height; y++ )
		for (x=0; x<width; x++ ) {
			c = upper[ (width*(height-1-y)+(width-1-x))/scale ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ (width*y+x)/scale ];
			*dest++ = c;
		}
}

void GrMergeTextures3( uint8_t * lower, uint8_t * upper, uint8_t * dest, uint16_t width, uint16_t height, int32_t scale )
{
 int32_t x,y;
 uint8_t c;
	for (y=0; y<height; y++ )
		for (x=0; x<width; x++ ) {
			c = upper[ width*(height-1-x)+y  ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ width*y+x ];
			*dest++ = c;
		}
}
