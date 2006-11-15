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

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

//------------------------------------------------------------------------------

inline void GrSetBitmapData (grsBitmap *bmP, unsigned char *data)
{
#ifdef OGL
OglFreeBmTexture(bmP);
#endif
bmP->bm_texBuf = data;
#ifdef D1XD3D
Assert (bmP->iMagic == BM_MAGIC_NUMBER);
Win32_SetTextureBits (bmP, data, bmP->bm_props.flags & BM_FLAG_RLE);
#endif
}

//------------------------------------------------------------------------------

void *GrAllocBitmapData (int w, int h, int bTGA)
{
return (w * h) ? d_malloc (bTGA ? w * h * 4 : MAX_BMP_SIZE (w, h)) : NULL;
}

//------------------------------------------------------------------------------

grsBitmap *GrCreateBitmapSub (int w, int h, unsigned char *data, int bTGA)
{
    grsBitmap *bmP;

bmP = (grsBitmap *) d_malloc (sizeof (grsBitmap));
if (bmP)
	GrInitBitmap (bmP, 0, 0, 0, w, h, w, data, bTGA);
return bmP;
}

//------------------------------------------------------------------------------

grsBitmap *GrCreateBitmap (int w, int h, int bTGA)
{
return GrCreateBitmapSub (w, h, GrAllocBitmapData (w, h, bTGA), bTGA);
}

//------------------------------------------------------------------------------

#if defined(POLY_ACC)
//
//  Creates a bitmap of the requested size and nType.
//    w, and h are in pixels.
//    nType is a BM_... and is used to set the rowsize.
//    if data is NULL, memory is allocated, otherwise data is used for bm_texBuf.
//
//  This function is used only by the polygon accelerator code to handle the mixture of 15bit and
//  8bit bitmaps.
//
grsBitmap *GrCreateBitmap2 (int w, int h, int nType, void *data)
{
	grsBitmap *newBM;

newBM = (grsBitmap *) d_malloc(sizeof(grsBitmap));
newBM->bm_props.x = 0;
newBM->bm_props.y = 0;
newBM->bm_props.w = w;
newBM->bm_props.h = h;
newBM->bm_props.flags = 0;
newBM->bm_props.nType = nType;
switch(nType) {
   case BM_LINEAR:     
		newBM->bm_props.rowsize = w;            
		break;
	case BM_LINEAR15:   
		newBM->bm_props.rowsize = w*PA_BPP;     
		break;
      default: 
		Int3();    // unsupported nType.
	   }
if(data)
   newBM->bm_texBuf = data;
else
   newBM->bm_texBuf = d_malloc(newBM->bm_props.rowsize * newBM->bm_props.h);
newBM->bm_handle = 0;
return newBM;
}
#endif

//------------------------------------------------------------------------------

void GrInitBitmap  (
	grsBitmap *bmP, int mode, int x, int y, int w, int h, int nBytesPerLine, 
	unsigned char *data, int bTGA) // TODO: virtualize
{
#ifdef D1XD3D
Assert (bmP->iMagic != BM_MAGIC_NUMBER || bmP->pvSurface == NULL);
#endif
memset (bmP, 0, sizeof  (*bmP));
bmP->bm_props.x = x;
bmP->bm_props.y = y;
bmP->bm_props.w = w;
bmP->bm_props.h = h;
bmP->bm_props.nType = mode;
bmP->bm_props.rowsize = bTGA ? nBytesPerLine * 4 : nBytesPerLine;
#ifdef D1XD3D
bmP->iMagic = BM_MAGIC_NUMBER;
#endif
#ifdef D1XD3D
Win32_CreateTexture (bmP);
#endif
if (bTGA)
	bmP->bm_props.flags = (char) BM_FLAG_TGA;
GrSetBitmapData (bmP, data);
}

//------------------------------------------------------------------------------

void GrInitBitmapAlloc (grsBitmap *bmP, int mode, int x, int y, int w, int h, 
								int nBytesPerLine, int bTGA)
{
GrInitBitmap (bmP, mode, x, y, w, h, nBytesPerLine, GrAllocBitmapData (w, h, bTGA), bTGA);
}

//------------------------------------------------------------------------------

void GrInitBitmapData (grsBitmap *bmP) // TODO: virtulize
{
bmP->bm_texBuf = NULL;
#ifdef D1XD3D
	Assert (bmP->iMagic != BM_MAGIC_NUMBER);
	bmP->iMagic = BM_MAGIC_NUMBER;
	bmP->pvSurface = NULL;
#endif
#ifdef OGL
//	OglFreeBmTexture(bmP);//not what we want here.
bmP->glTexture = NULL;
#endif
}

//------------------------------------------------------------------------------

grsBitmap *GrCreateSubBitmap(grsBitmap *bmP, int x, int y, int w, int h)
{
    grsBitmap *newBM;

if  (!(newBM = (grsBitmap *)d_malloc(sizeof(grsBitmap))))
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
	d_free (BM_FRAMES (bmP));
	d_free (bmP);
	}
}

//------------------------------------------------------------------------------

void GrFreeSubBitmap(grsBitmap *bmP)
{
if (bmP) {
#ifdef D1XD3D
	bmP->iMagic = 0;
#endif
	d_free(bmP);
	}
}

//------------------------------------------------------------------------------

void GrFreeBitmapData (grsBitmap *bmP) // TODO: virtulize
{
if (bmP) {
#ifdef D1XD3D
	Assert (bmP->iMagic == BM_MAGIC_NUMBER);
	Win32_FreeTexture (bmP);
	bmP->iMagic = 0;
	if (bmP->bm_texBuf == BM_D3D_RENDER)
		bmP->bm_texBuf = NULL;
#endif
#ifdef OGL
	OglFreeBmTexture (bmP);
#endif
	if (bmP->bm_texBuf) 
		d_free (bmP->bm_texBuf);
	}
}

//------------------------------------------------------------------------------

void GrInitSubBitmap (grsBitmap *bmP, grsBitmap *bmParent, int x, int y, int w, int h)	// TODO: virtualize
{
bmP->bm_props.x = x + bmParent->bm_props.x;
bmP->bm_props.y = y + bmParent->bm_props.y;
bmP->bm_props.w = w;
bmP->bm_props.h = h;
bmP->bm_props.flags = bmParent->bm_props.flags;
bmP->bm_props.nType = bmParent->bm_props.nType;
bmP->bm_props.rowsize = bmParent->bm_props.rowsize;
#ifdef OGL
bmP->glTexture = bmParent->glTexture;
bmP->bm_palette = bmParent->bm_palette;
BM_PARENT (bmP) = bmParent;
#endif
#ifdef D1XD3D
Assert (bmParent->iMagic == BM_MAGIC_NUMBER);
bmP->iMagic = BM_MAGIC_NUMBER;
bmP->pvSurface = bmParent->pvSurface;
if (bmP->bm_props.nType == BM_DIRECTX)
	bmP->bm_texBuf = bmParent->bm_texBuf;
else
#endif
	bmP->bm_texBuf = bmParent->bm_texBuf+(unsigned int) ((y*bmParent->bm_props.rowsize)+x);
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
		count  [mapped]++;
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
#ifdef D1XD3D
	Assert (pbm->iMagic == BM_MAGIC_NUMBER);

	if (pbm->pvSurface)
	{
		if  ((flags & BM_FLAG_TRANSPARENT) != (pbm->bm_props.flags & BM_FLAG_TRANSPARENT))
		{
			Win32_SetTransparent (pbm->pvSurface, flags & BM_FLAG_TRANSPARENT);
		}
	}
#endif
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

void GrBitmapCheckTransparency(grsBitmap * bmP)
{
	int x, y;
	ubyte * data;

	data = bmP->bm_texBuf;

	for (y=0; y<bmP->bm_props.h; y++)	{
		for (x=0; x<bmP->bm_props.w; x++)	{
			if  (*data++ == TRANSPARENCY_COLOR)	{
				GrSetTransparent (bmP, 1);
				return;
			}
		}
		data += bmP->bm_props.rowsize - bmP->bm_props.w;
	}

	bmP->bm_props.flags = 0;

}

//------------------------------------------------------------------------------

