/* $Id: bitmap.h,v 1.4 2002/09/05 07:55:20 btb Exp $ */
#ifndef _BITMAP_H
#define _BITMAP_H

void GrCountColors (ubyte *data, int i, int *freq);
void GrSetPalette (grs_bitmap *bmP, ubyte *palette, int transparent_color, int super_transparent_color, int *freq);
void decode_data_asm (ubyte *data, int num_pixels, ubyte * colormap, int * count);


static inline ubyte GrRemapColor (ubyte *destPal, ubyte *srcPal, ubyte color)
{
color *= 3;
return GrFindClosestColor (destPal, srcPal [color], srcPal [color + 1], srcPal [color + 2]);
}

#endif
