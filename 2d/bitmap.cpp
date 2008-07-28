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

#include "inferno.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "u_dpmi.h"
#include "error.h"
#include "ogl_defs.h"

//------------------------------------------------------------------------------

inline void GrSetBitmapData (grsBitmap *bmP, unsigned char *data)
{
OglFreeBmTexture(bmP);
bmP->bmTexBuf = data;
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
return GrCreateBitmapSub (w, h, (unsigned char *) GrAllocBitmapData (w, h, bpp), bpp);
}

//------------------------------------------------------------------------------

void GrInitBitmap  (
	grsBitmap *bmP, int mode, int x, int y, int w, int h, int nBytesPerLine, 
	unsigned char *data, int bpp) // TODO: virtualize
{
memset (bmP, 0, sizeof  (*bmP));
bmP->bmProps.x = x;
bmP->bmProps.y = y;
bmP->bmProps.w = w;
bmP->bmProps.h = h;
bmP->bmBPP = bpp;
bmP->bmProps.nType = mode;
bmP->bmProps.rowSize = nBytesPerLine * bpp;
if (bpp > 2)
	bmP->bmProps.flags = (char) BM_FLAG_TGA;
GrSetBitmapData (bmP, data);
}

//------------------------------------------------------------------------------

void GrInitBitmapAlloc (grsBitmap *bmP, int mode, int x, int y, int w, int h, 
								int nBytesPerLine, int bpp)
{
GrInitBitmap (bmP, mode, x, y, w, h, nBytesPerLine, (unsigned char *) GrAllocBitmapData (w, h, bpp), bpp);
}

//------------------------------------------------------------------------------

void GrInitBitmapData (grsBitmap *bmP) // TODO: virtulize
{
bmP->bmTexBuf = NULL;
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
	if (bmP->bmTexBuf) 
		D2_FREE (bmP->bmTexBuf);
	}
}

//------------------------------------------------------------------------------

void GrInitSubBitmap (grsBitmap *bmP, grsBitmap *bmParent, int x, int y, int w, int h)	// TODO: virtualize
{
bmP->bmProps.x = x + bmParent->bmProps.x;
bmP->bmProps.y = y + bmParent->bmProps.y;
bmP->bmProps.w = w;
bmP->bmProps.h = h;
bmP->bmBPP = bmParent->bmBPP;
bmP->bmProps.flags = bmParent->bmProps.flags;
bmP->bmProps.nType = bmParent->bmProps.nType;
bmP->bmProps.rowSize = bmParent->bmProps.rowSize;
bmP->glTexture = bmParent->glTexture;
bmP->bmPalette = bmParent->bmPalette;
bmP->bmAvgColor = bmParent->bmAvgColor;
bmP->bmAvgRGB = bmParent->bmAvgRGB;
BM_PARENT (bmP) = bmParent;
#ifdef _DEBUG
memcpy (bmP->szName, bmParent->szName, sizeof (bmP->szName));
#endif
bmP->bmTexBuf = bmParent->bmTexBuf + (unsigned int) ((y * bmParent->bmProps.rowSize) + x);
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

void GrSetBitmapFlags (grsBitmap *bmP, int flags)
{
bmP->bmProps.flags = flags;
}

//------------------------------------------------------------------------------

void GrSetTransparent (grsBitmap *bmP, int bTransparent)
{
if (bTransparent)
	GrSetBitmapFlags (bmP, bmP->bmProps.flags | BM_FLAG_TRANSPARENT);
else
	GrSetBitmapFlags (bmP, bmP->bmProps.flags & ~BM_FLAG_TRANSPARENT);
}

//------------------------------------------------------------------------------

void GrSetSuperTransparent (grsBitmap *bmP, int bTransparent)
{
if (gameData.pig.tex.textureIndex [0][bmP->bmHandle] >= 0) {
	if (bTransparent)
		GrSetBitmapFlags (bmP, bmP->bmProps.flags | BM_FLAG_SUPER_TRANSPARENT);
	else
		GrSetBitmapFlags (bmP, bmP->bmProps.flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
}

//------------------------------------------------------------------------------

void GrSetPalette (grsBitmap *bmP, ubyte *palette, int transparentColor, int superTranspColor, int *freq)
{
if ((transparentColor >= 0) && (transparentColor <= 255)) {
	//palette [255] = transparentColor;
	if (freq [transparentColor])
		GrSetTransparent (bmP, 1);
	else
		GrSetTransparent (bmP, 0);
	}
if ((superTranspColor >= 0) && (superTranspColor <= 255)) {
	//palette [254] = transparentColor;
	if (freq [superTranspColor])
		GrSetSuperTransparent (bmP, 1);
	else
		GrSetSuperTransparent (bmP, 0);
	}
bmP->bmPalette = AddPalette (palette);
}

//------------------------------------------------------------------------------

void GrRemapBitmap (grsBitmap * bmP, ubyte *palette, int transparentColor, int superTranspColor)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = bmP->bmPalette;
if (!palette)
	return;
GrCountColors (bmP->bmTexBuf, bmP->bmProps.w * bmP->bmProps.h, freq);
GrSetPalette (bmP, palette, transparentColor, superTranspColor, freq);
}

//------------------------------------------------------------------------------

void GrRemapBitmapGood (grsBitmap * bmP, ubyte *palette, int transparentColor, int superTranspColor)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = bmP->bmPalette;
if (!palette)
	return;
if (bmP->bmProps.w == bmP->bmProps.rowSize)
	GrCountColors (bmP->bmTexBuf, bmP->bmProps.w * bmP->bmProps.h, freq);
else {
	int y;
	ubyte *p = bmP->bmTexBuf;
	for (y = bmP->bmProps.h; y; y--, p += bmP->bmProps.rowSize)
		GrCountColors (p, bmP->bmProps.w, freq);
	}
GrSetPalette (bmP, palette, transparentColor, superTranspColor, freq);
}

//------------------------------------------------------------------------------

#ifdef BITMAP_SELECTOR
int GrBitmapAssignSelector(grsBitmap * bmP)
{
if  (!dpmi_allocate_selector(bmP->bmTexBuf, bmP->bmProps.w*bmP->bmProps.h, &bmP->bm_selector)) {
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

	data = bmP->bmTexBuf;

for (y = 0; y < bmP->bmProps.h; y++)	{
	for (x = 0; x < bmP->bmProps.w; x++)	{
		if  (*data++ == TRANSPARENCY_COLOR)	{
			GrSetTransparent (bmP, 1);
			return;
			}
		}
	data += bmP->bmProps.rowSize - bmP->bmProps.w;
	}
bmP->bmProps.flags = 0;
}

//---------------------------------------------------------------

int GrBitmapHasTransparency (grsBitmap *bmP)
{
	int	i, nFrames;

if (bmP->bmType && (bmP->bmBPP == 4))
	return 1;
if (bmP->bmProps.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 1;
nFrames = bmP->bmProps.h / bmP->bmProps.w;
for (i = 1; i < nFrames; i++) {
	if (bmP->bmTransparentFrames [i / 32] & (1 << (i % 32)))
		return 1;
	if (bmP->bmSupertranspFrames [i / 32] & (1 << (i % 32)))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

