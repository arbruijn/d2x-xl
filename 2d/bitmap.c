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

/*
 *
 * Graphical routines for manipulating grs_bitmaps.
 *
 * Old Log:
 * Revision 1.11  1995/08/23  18:46:06  allender
 * fixed compiler warning
 *
 * Revision 1.10  1995/08/14  14:25:45  allender
 * changed transparency color to 0
 *
 * Revision 1.9  1995/07/05  16:04:51  allender
 * transparency/game kitchen changes
 *
 * Revision 1.8  1995/06/15  09:50:48  allender
 * new d_malloc to align bitmap on 8 byte bountry
 *
 * Revision 1.7  1995/05/12  11:52:19  allender
 * changed memory stuff again
 *
 * Revision 1.6  1995/05/11  12:48:34  allender
 * nge transparency color
 *
 * Revision 1.5  1995/05/04  19:59:21  allender
 * use NewPtr instead of d_malloc
 *
 * Revision 1.4  1995/04/27  07:33:04  allender
 * rearrange functions
 *
 * Revision 1.3  1995/04/19  14:37:17  allender
 * removed dead asm code
 *
 * Revision 1.2  1995/04/18  12:04:51  allender
 * *** empty log message ***
 *
 * Revision 1.1  1995/03/09  08:48:06  allender
 * Initial revision
 *
 *
 * -------  PC version RCS information
 * Revision 1.17  1994/11/18  22:50:25  john
 * Changed shorts to ints in parameters.
 *
 * Revision 1.16  1994/11/10  15:59:46  john
 * Fixed bugs with canvas's being created with bogus bm_props.flags.
 *
 * Revision 1.15  1994/10/26  23:55:53  john
 * Took out roller; Took out inverse table.
 *
 * Revision 1.14  1994/09/19  14:40:21  john
 * Changed dpmi stuff.
 *
 * Revision 1.13  1994/09/19  11:44:04  john
 * Changed call to allocate selector to the dpmi module.
 *
 * Revision 1.12  1994/06/09  13:14:57  john
 * Made selectors zero our
 * out, I meant.
 *
 * Revision 1.11  1994/05/06  12:50:07  john
 * Added supertransparency; neatend things up; took out warnings.
 *
 * Revision 1.10  1994/04/08  16:59:39  john
 * Add fading poly's; Made palette fade 32 instead of 16.
 *
 * Revision 1.9  1994/03/16  17:21:09  john
 * Added slow palette searching options.
 *
 * Revision 1.8  1994/03/14  17:59:35  john
 * Added function to check bitmap's transparency.
 *
 * Revision 1.7  1994/03/14  17:16:21  john
 * fixed bug with counting freq of pixels.
 *
 * Revision 1.6  1994/03/14  16:55:47  john
 * Changed grs_bitmap structure to include bm_props.flags.
 *
 * Revision 1.5  1994/02/18  15:32:22  john
 * *** empty log message ***
 *
 * Revision 1.4  1993/10/15  16:22:49  john
 * *** empty log message ***
 *
 * Revision 1.3  1993/09/08  17:37:11  john
 * Checking for errors with Yuan...
 *
 * Revision 1.2  1993/09/08  14:46:27  john
 * looking for possible bugs...
 *
 * Revision 1.1  1993/09/08  11:43:05  john
 * Initial revision
 *
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

inline void GrSetBitmapData (grs_bitmap *bmP, unsigned char *data)
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

grs_bitmap *GrCreateBitmapSub (int w, int h, unsigned char *data, int bTGA)
{
    grs_bitmap *bmP;

bmP = (grs_bitmap *) d_malloc (sizeof (grs_bitmap));
GrInitBitmap (bmP, 0, 0, 0, w, h, w, data, bTGA);
return bmP;
}

//------------------------------------------------------------------------------

grs_bitmap *GrCreateBitmap (int w, int h, int bTGA)
{
return GrCreateBitmapSub (w, h, GrAllocBitmapData (w, h, bTGA), bTGA);
}

//------------------------------------------------------------------------------

#if defined(POLY_ACC)
//
//  Creates a bitmap of the requested size and type.
//    w, and h are in pixels.
//    type is a BM_... and is used to set the rowsize.
//    if data is NULL, memory is allocated, otherwise data is used for bm_texBuf.
//
//  This function is used only by the polygon accelerator code to handle the mixture of 15bit and
//  8bit bitmaps.
//
grs_bitmap *GrCreateBitmap2 (int w, int h, int type, void *data)
{
	grs_bitmap *newBM;

newBM = (grs_bitmap *) d_malloc(sizeof(grs_bitmap));
newBM->bm_props.x = 0;
newBM->bm_props.y = 0;
newBM->bm_props.w = w;
newBM->bm_props.h = h;
newBM->bm_props.flags = 0;
newBM->bm_props.type = type;
switch(type) {
   case BM_LINEAR:     
		newBM->bm_props.rowsize = w;            
		break;
	case BM_LINEAR15:   
		newBM->bm_props.rowsize = w*PA_BPP;     
		break;
      default: 
		Int3();    // unsupported type.
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
	grs_bitmap *bmP, int mode, int x, int y, int w, int h, int nBytesPerLine, 
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
bmP->bm_props.type = mode;
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

void GrInitBitmapAlloc (grs_bitmap *bmP, int mode, int x, int y, int w, int h, 
								int nBytesPerLine, int bTGA)
{
GrInitBitmap (bmP, mode, x, y, w, h, nBytesPerLine, GrAllocBitmapData (w, h, bTGA), bTGA);
}

//------------------------------------------------------------------------------

void GrInitBitmapData (grs_bitmap *bmP) // TODO: virtulize
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

grs_bitmap *GrCreateSubBitmap(grs_bitmap *bmP, int x, int y, int w, int h)
{
    grs_bitmap *newBM;

if  (!(newBM = (grs_bitmap *)d_malloc(sizeof(grs_bitmap))))
	return NULL;
memset (newBM, 0, sizeof  (*newBM));
GrInitSubBitmap (newBM, bmP, x, y, w, h);
return newBM;
}

//------------------------------------------------------------------------------

void GrFreeBitmap (grs_bitmap *bmP)
{
if (bmP) {
	GrFreeBitmapData (bmP);
	d_free (bmP);
	}
}

//------------------------------------------------------------------------------

void GrFreeSubBitmap(grs_bitmap *bmP)
{
	if (bmP!=NULL)
	{
#ifdef D1XD3D
		bmP->iMagic = 0;
#endif
		d_free(bmP);
	}
}

//------------------------------------------------------------------------------

void GrFreeBitmapData (grs_bitmap *bmP) // TODO: virtulize
{
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

//------------------------------------------------------------------------------

void GrInitSubBitmap (grs_bitmap *bmP, grs_bitmap *bmParent, int x, int y, int w, int h)	// TODO: virtualize
{
bmP->bm_props.x = x + bmParent->bm_props.x;
bmP->bm_props.y = y + bmParent->bm_props.y;
bmP->bm_props.w = w;
bmP->bm_props.h = h;
bmP->bm_props.flags = bmParent->bm_props.flags;
bmP->bm_props.type = bmParent->bm_props.type;
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
if (bmP->bm_props.type == BM_DIRECTX)
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

void gr_set_bitmap_flags (grs_bitmap *pbm, int flags)
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

void GrSetTransparent (grs_bitmap *pbm, int bTransparent)
{
if (bTransparent)
	gr_set_bitmap_flags (pbm, pbm->bm_props.flags | BM_FLAG_TRANSPARENT);
else
	gr_set_bitmap_flags (pbm, pbm->bm_props.flags & ~BM_FLAG_TRANSPARENT);
}

//------------------------------------------------------------------------------

void GrSetSuperTransparent (grs_bitmap *pbm, int bTransparent)
{
if (bTransparent)
	gr_set_bitmap_flags (pbm, pbm->bm_props.flags & ~BM_FLAG_SUPER_TRANSPARENT);
else
	gr_set_bitmap_flags (pbm, pbm->bm_props.flags | BM_FLAG_SUPER_TRANSPARENT);
}

//------------------------------------------------------------------------------

void GrSetPalette (grs_bitmap *bmP, ubyte *palette, int transparent_color, int super_transparent_color, int *freq)
{
if ((transparent_color >= 0) && (transparent_color <= 255)) {
	//palette [255] = transparent_color;
	if (freq [transparent_color])
		GrSetTransparent (bmP, 1);
	}
if ((super_transparent_color >= 0) && (super_transparent_color <= 255)) {
	//palette [254] = transparent_color;
	if (freq [super_transparent_color])
		GrSetSuperTransparent (bmP, 0);
	}
bmP->bm_palette = AddPalette (palette);
}

//------------------------------------------------------------------------------

void GrRemapBitmap (grs_bitmap * bmP, ubyte *palette, int transparent_color, int super_transparent_color)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = bmP->bm_palette;
if (!palette)
	return;
GrCountColors (bmP->bm_texBuf, bmP->bm_props.w * bmP->bm_props.h, freq);
GrSetPalette (bmP, palette, transparent_color, super_transparent_color, freq);
}

//------------------------------------------------------------------------------

void GrRemapBitmapGood (grs_bitmap * bmP, ubyte *palette, int transparent_color, int super_transparent_color)
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
GrSetPalette (bmP, palette, transparent_color, super_transparent_color, freq);
}

//------------------------------------------------------------------------------

#ifdef BITMAP_SELECTOR
int GrBitmapAssignSelector(grs_bitmap * bmP)
{
	if  (!dpmi_allocate_selector(bmP->bm_texBuf, bmP->bm_props.w*bmP->bm_props.h, &bmP->bm_selector)) {
		bmP->bm_selector = 0;
		return 1;
	}
	return 0;
}
#endif

//------------------------------------------------------------------------------

void GrBitmapCheckTransparency(grs_bitmap * bmP)
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

