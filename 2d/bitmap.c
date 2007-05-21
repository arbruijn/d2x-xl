/* $Id: bitmap.c,v 1.6 2004/01/08 20:31:35 schaffner Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION  ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "u_dpmi.h"
#include "error.h"
#include "inferno.h"

#include "ogl_init.h"

//------------------------------------------------------------------------------

inline void GrSetBitmapData (grsBitmap *bmP, unsigned char *data)
{
OglFreeBmTexture(bmP);
bmP->bm_texBuf = data;
}

//------------------------------------------------------------------------------

void *GrAllocBitmapData (int w, int h, int bpp)
{
return (w * h) ? D2_ALLOC ((bpp > 1) ? w * h * bpp : MAX_BMP_SIZE (w, h)) : NULL;
}

//------------------------------------------------------------------------------

grsBitmap *GrCreateBitmapSub (int w, int h, unsigned char *data, int bpp)
{
    grsBitmap *bmP;

bmP = (grsBitmap *) D2_ALLOC (sizeof (grsBitmap));
if (bmP)
	GrInitBitmap (bmP, 0, 0, 0, w, h, w, data, bpp);
return bmP;
}

//------------------------------------------------------------------------------

grsBitmap *GrCreateBitmap (int w, int h, int bpp)
{
return GrCreateBitmapSub (w, h, GrAllocBitmapData (w, h, bpp), bpp);
}

//------------------------------------------------------------------------------

void GrInitBitmap  (
	grsBitmap *bmP, int mode, int x, int y, int w, int h, int nBytesPerLine, 
	unsigned char *data, int bpp) // TODO: virtualize
{
memset (bmP, 0, sizeof  (*bmP));
bmP->bm_props.x = x;
bmP->bm_props.y = y;
bmP->bm_props.w = w;
bmP->bm_props.h = h;
bmP->bm_bpp = bpp;
bmP->bm_props.nType = mode;
bmP->bm_props.rowsize = nBytesPerLine * bpp;
if (bpp > 2)
	bmP->bm_props.flags = (char) BM_FLAG_TGA;
GrSetBitmapData (bmP, data);
}

//------------------------------------------------------------------------------

void GrInitBitmapAlloc (grsBitmap *bmP, int mode, int x, int y, int w, int h, 
								int nBytesPerLine, int bpp)
{
GrInitBitmap (bmP, mode, x, y, w, h, nBytesPerLine, GrAllocBitmapData (w, h, bpp), bpp);
}

//------------------------------------------------------------------------------

void GrInitBitmapData (grsBitmap *bmP) // TODO: virtulize
{
bmP->bm_texBuf = NULL;
//	OglFreeBmTexture(bmP);//not what we want here.
bmP->glTexture = NULL;
}

//------------------------------------------------------------------------------

grsBitmap *GrCreateSubBitmap(grsBitmap *bmP, int x, int y, int w, int h)
{
    grsBitmap *newBM;

if  (!(newBM = (grsBitmap *)D2_ALLOC(sizeof(grsBitmap))))
	return NULL;
memset (newBM, 0, sizeof  (*newBM));
GrInitSubBitmap (newBM, bmP, x, y, w, h);
return newBM;
}

//------------------------------------------------------------------------------

void GrFreeBitmap (grsBitmap *bmP)
{
if (bmP) {
	GrFreeBitmapData (bmP);
	D2_FREE (BM_FRAMES (bmP));
	D2_FREE (bmP);
	}
}

//------------------------------------------------------------------------------

void GrFreeSubBitmap(grsBitmap *bmP)
{
if (bmP) {
	D2_FREE(bmP);
	}
}

//------------------------------------------------------------------------------

void GrFreeBitmapData (grsBitmap *bmP) // TODO: virtulize
{
if (bmP) {
	OglFreeBmTexture (bmP);
	if (bmP->bm_texBuf) 
		D2_FREE (bmP->bm_texBuf);
	}
}

//------------------------------------------------------------------------------

void GrInitSubBitmap (grsBitmap *bmP, grsBitmap *bmParent, int x, int y, int w, int h)	// TODO: virtualize
{
bmP->bm_props.x = x + bmParent->bm_props.x;
bmP->bm_props.y = y + bmParent->bm_props.y;
bmP->bm_props.w = w;
bmP->bm_props.h = h;
bmP->bm_bpp = bmParent->bm_bpp;
bmP->bm_props.flags = bmParent->bm_props.flags;
bmP->bm_props.nType = bmParent->bm_props.nType;
bmP->bm_props.rowsize = bmParent->bm_props.rowsize;
bmP->glTexture = bmParent->glTexture;
bmP->bm_palette = bmParent->bm_palette;
BM_PARENT (bmP) = bmParent;
bmP->bm_texBuf = bmParent->bm_texBuf + (unsigned int) ((y * bmParent->bm_props.rowsize) + x);
}

//------------------------------------------------------------------------------

void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count);


//------------------------------------------------------------------------------

void decode_data_asm (ubyte *data, int num_pixels, ubyte *colormap, int *count)
{
	int i;
	ubyte mapped;

for (i = 0; i < num_pixels; i++) {
	mapped = *data;
	count [mapped]++;
	*data = colormap  [mapped];
	data++;
	}
}

//------------------------------------------------------------------------------

void GrCountColors (ubyte *data, int i, int *freq)
{
for (; i; i--)
	freq [*data++]++;
}

//------------------------------------------------------------------------------

void GrSetBitmapFlags (grsBitmap *pbm, int flags)
{
pbm->bm_props.flags = flags;
}

//------------------------------------------------------------------------------

void GrSetTransparent (grsBitmap *pbm, int bTransparent)
{
if (bTransparent)
	GrSetBitmapFlags (pbm, pbm->bm_props.flags | BM_FLAG_TRANSPARENT);
else
	GrSetBitmapFlags (pbm, pbm->bm_props.flags & ~BM_FLAG_TRANSPARENT);
}

//------------------------------------------------------------------------------

void GrSetSuperTransparent (grsBitmap *pbm, int bTransparent)
{
if (bTransparent)
	GrSetBitmapFlags (pbm, pbm->bm_props.flags & ~BM_FLAG_SUPER_TRANSPARENT);
else
	GrSetBitmapFlags (pbm, pbm->bm_props.flags | BM_FLAG_SUPER_TRANSPARENT);
}

//------------------------------------------------------------------------------

void GrSetPalette (grsBitmap *bmP, ubyte *palette, int transparentColor, int superTranspColor, int *freq)
{
if ((transparentColor >= 0) && (transparentColor <= 255)) {
	//palette [255] = transparentColor;
	if (freq [transparentColor])
		GrSetTransparent (bmP, 1);
	}
if ((superTranspColor >= 0) && (superTranspColor <= 255)) {
	//palette [254] = transparentColor;
	if (freq [superTranspColor])
		GrSetSuperTransparent (bmP, 0);
	}
bmP->bm_palette = AddPalette (palette);
}

//------------------------------------------------------------------------------

void GrRemapBitmap (grsBitmap * bmP, ubyte *palette, int transparentColor, int superTranspColor)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = bmP->bm_palette;
if (!palette)
	return;
GrCountColors (bmP->bm_texBuf, bmP->bm_props.w * bmP->bm_props.h, freq);
GrSetPalette (bmP, palette, transparentColor, superTranspColor, freq);
}

//------------------------------------------------------------------------------

void GrRemapBitmapGood (grsBitmap * bmP, ubyte *palette, int transparentColor, int superTranspColor)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = bmP->bm_palette;
if (!palette)
	return;
if (bmP->bm_props.w == bmP->bm_props.rowsize)
	GrCountColors (bmP->bm_texBuf, bmP->bm_props.w * bmP->bm_props.h, freq);
else {
	int y;
	ubyte *p = bmP->bm_texBuf;
	for (y = bmP->bm_props.h; y; y--, p += bmP->bm_props.rowsize)
		GrCountColors (p, bmP->bm_props.w, freq);
	}
GrSetPalette (bmP, palette, transparentColor, superTranspColor, freq);
}

//------------------------------------------------------------------------------

#ifdef BITMAP_SELECTOR
int GrBitmapAssignSelector(grsBitmap * bmP)
{
if  (!dpmi_allocate_selector(bmP->bm_texBuf, bmP->bm_props.w*bmP->bm_props.h, &bmP->bm_selector)) {
	bmP->bm_selector = 0;
	return 1;
	}
return 0;
}
#endif

//------------------------------------------------------------------------------

void GrBitmapCheckTransparency (grsBitmap * bmP)
{
	int x, y;
	ubyte * data;

	data = bmP->bm_texBuf;

for (y = 0; y < bmP->bm_props.h; y++)	{
	for (x = 0; x < bmP->bm_props.w; x++)	{
		if  (*data++ == TRANSPARENCY_COLOR)	{
			GrSetTransparent (bmP, 1);
			return;
			}
		}
	data += bmP->bm_props.rowsize - bmP->bm_props.w;
	}
bmP->bm_props.flags = 0;
}

//---------------------------------------------------------------

int GrBitmapHasTransparency (grsBitmap *bmP)
{
	int	i, nFrames;

if (bmP->bm_props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 1;
nFrames = bmP->bm_props.h / bmP->bm_props.w;
for (i = 1; i < nFrames; i++) {
	if (bmP->bm_transparentFrames [i / 32] & (1 << (i % 32)))
		return 1;
	if (bmP->bm_supertranspFrames [i / 32] & (1 << (i % 32)))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

