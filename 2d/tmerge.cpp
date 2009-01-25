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

void GrMergeTextures( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale )
{
	int x,y,h;
	ubyte c;
	for (y=0;y<height;y++) 
		for (x=0;x<width;x++) {
			h = (width*y+x) / scale;
			c=upper[h];
			if (c==TRANSPARENCY_COLOR)
				c=lower[h];
			*dest++=c;
		}
}

void GrMergeTextures1( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale )
{
	int x,y;
	ubyte c;
	for (y=0; y<height; y++ )
		for (x=0; x<width; x++ ) {
			c = upper[ (width*x+(height-1-y)) / scale ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ (width*y+x) / scale ];
			*dest++ = c;
		}
}

void GrMergeTextures2( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale )
{
 int x,y;
 ubyte c;
	for (y=0; y<height; y++ )
		for (x=0; x<width; x++ ) {
			c = upper[ (width*(height-1-y)+(width-1-x))/scale ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ (width*y+x)/scale ];
			*dest++ = c;
		}
}

void GrMergeTextures3( ubyte * lower, ubyte * upper, ubyte * dest, ushort width, ushort height, int scale )
{
 int x,y;
 ubyte c;
	for (y=0; y<height; y++ )
		for (x=0; x<width; x++ ) {
			c = upper[ width*(height-1-x)+y  ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ width*y+x ];
			*dest++ = c;
		}
}
