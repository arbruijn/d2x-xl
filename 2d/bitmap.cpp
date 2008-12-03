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
#include "color.h"
#include "palette.h"
#include "pcx.h"

//------------------------------------------------------------------------------

ubyte* CBitmap::CreateTexBuf (void)
{
if (m_bm.props.rowSize * m_bm.props.h) {
	m_bm.texBuf = new ubyte [(m_bm.nBPP > 1) ? m_bm.props.h * m_bm.props.rowSize : MAX_BMP_SIZE (m_bm.props.w, m_bm.props.h)];
#if DBG
	if (m_bm.texBuf) 
		memset (m_bm.texBuf, 0, (m_bm.nBPP > 1) ? m_bm.props.h * m_bm.props.rowSize : MAX_BMP_SIZE (m_bm.props.w, m_bm.props.h));
#endif
	}
return m_bm.texBuf;
}

//------------------------------------------------------------------------------

CBitmap* CBitmap::Create (ubyte mode, int w, int h, int bpp)
{
	CBitmap	*bmP = new CBitmap; //(CBitmap *) D2_ALLOC (sizeof (CBitmap));

if (bmP)
	bmP->Setup (mode, w, h, bpp);
return bmP;
}

//------------------------------------------------------------------------------

bool CBitmap::Setup (ubyte mode, int w, int h, int bpp, ubyte* texBuf)
{
Init (mode, 0, 0, w, h, bpp);
return SetTexBuf (texBuf ? texBuf : CreateTexBuf ()) != NULL;
}

//------------------------------------------------------------------------------

void CBitmap::DestroyTexBuf (void)
{
if (m_bm.texBuf) {
	FreeTexture (this);
	delete[] m_bm.texBuf;
	m_bm.texBuf = NULL;
	}
}

//------------------------------------------------------------------------------

void CBitmap::Destroy (void)
{
SetPalette (NULL);
if ((Type () == BM_TYPE_ALT) || !Parent ())
	DestroyTexBuf ();
DestroyFrames ();
DestroyMask ();
}

//------------------------------------------------------------------------------

void CBitmap::DestroyFrames (void)
{
D2_FREE (m_bm.info.alt.frames);
m_bm.info.alt.curFrame = NULL;
m_bm.info.alt.nFrameCount = 0;
}

//------------------------------------------------------------------------------

void CBitmap::DestroyMask (void)
{
D2_FREE (m_bm.info.std.mask);
}

//------------------------------------------------------------------------------

void CBitmap::Init (void) 
{
memset (&m_bm, 0, sizeof (m_bm));
}

//------------------------------------------------------------------------------

void CBitmap::Init (int mode, int x, int y, int w, int h, int bpp, ubyte *texBuf) 
{
Init ();
m_bm.props.x = x;
m_bm.props.y = y;
m_bm.props.w = w;
m_bm.props.h = h;
m_bm.props.nMode = mode;
m_bm.nBPP = bpp ? bpp : 1;
m_bm.props.rowSize = w * bpp;
if (bpp > 2)
	m_bm.props.flags = (char) BM_FLAG_TGA;
SetTexBuf (texBuf);
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::CreateChild (int x, int y, int w, int h)
{
    CBitmap *child;

if  (!(child = new CBitmap))
	return NULL;
child->InitChild (this, x, y, w, h);
return child;
}

//------------------------------------------------------------------------------

void CBitmap::InitChild (CBitmap *parent, int x, int y, int w, int h)
{
*this = *parent;
m_bm.props.x += x;
m_bm.props.y += y;
m_bm.props.w = w;
m_bm.props.h = h;
SetParent (parent ? parent : this);
m_bm.texBuf += (unsigned int) ((m_bm.props.y * m_bm.props.rowSize) + m_bm.props.x * m_bm.nBPP);
}

//------------------------------------------------------------------------------

void CBitmap::SetTransparent (int bTransparent)
{
if (bTransparent)
	SetFlags (m_bm.props.flags | BM_FLAG_TRANSPARENT);
else
	SetFlags (m_bm.props.flags & ~BM_FLAG_TRANSPARENT);
}

//------------------------------------------------------------------------------

void CBitmap::SetSuperTransparent (int bTransparent)
{
if (gameData.pig.tex.textureIndex [0][m_bm.nId] >= 0) {
	if (bTransparent)
		SetFlags (m_bm.props.flags | BM_FLAG_SUPER_TRANSPARENT);
	else
		SetFlags (m_bm.props.flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
}

//------------------------------------------------------------------------------

void CBitmap::SetPalette (CPalette *palette, int transparentColor, int supertranspColor, int *freq)
{
if (!palette)
	m_bm.palette = NULL;
else {
	if (freq) {
		if ((transparentColor >= 0) && (transparentColor <= 255)) {
			if (freq [transparentColor])
				SetTransparent (1);
			else
				SetTransparent (0);
			}
		if ((supertranspColor >= 0) && (supertranspColor <= 255)) {
			if (freq [supertranspColor])
				SetSuperTransparent (1);
			else
				SetSuperTransparent (0);
			}
		}
	m_bm.palette = paletteManager.Add (*palette);
	}
}

//------------------------------------------------------------------------------

void CBitmap::Remap (CPalette *palette, int transparentColor, int supertranspColor)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = m_bm.palette;
if (!palette)
	return;
if (m_bm.props.w == m_bm.props.rowSize)
	CountColors (m_bm.texBuf, m_bm.props.w * m_bm.props.h, freq);
else {
	int y;
	ubyte *p = m_bm.texBuf;
	for (y = m_bm.props.h; y; y--, p += m_bm.props.rowSize)
		CountColors (p, m_bm.props.w, freq);
	}
SetPalette (palette, transparentColor, supertranspColor, freq);
}

//------------------------------------------------------------------------------

void CBitmap::CheckTransparency (void)
{
	ubyte *data = m_bm.texBuf;

for (int i = m_bm.props.w * m_bm.props.h; i; i--, data++)
	if  (*data++ == TRANSPARENCY_COLOR)	{
		SetTransparent (1);
		return;
		}
m_bm.props.flags = 0;
}

//---------------------------------------------------------------

int CBitmap::HasTransparency (void)
{
	int	i, nFrames;

if (m_bm.nType && (m_bm.nBPP == 4))
	return 1;
if (m_bm.props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 1;
nFrames = m_bm.props.h / m_bm.props.w;
for (i = 1; i < nFrames; i++) {
	if (m_bm.transparentFrames [i / 32] & (1 << (i % 32)))
		return 1;
	if (m_bm.supertranspFrames [i / 32] & (1 << (i % 32)))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::FreeTexture (CBitmap *bmP)
{
while ((bmP->Type () != BM_TYPE_ALT) && BM_PARENT (bmP) && (bmP != BM_PARENT (bmP)))
	bmP = BM_PARENT (bmP);
bmP->FreeTexture ();
return bmP;
}

//------------------------------------------------------------------------------

void CBitmap::FreeTexture (void)
{
	CBitmap	*frames = Frames ();

if (frames) {
	int i, nFrames = m_bm.props.h / m_bm.props.w;

	for (i = 0; i < nFrames; i++) {
		frames [i].FreeTexture ();
		frames [i].SetTexture (NULL);
		}
	}
else if (m_bm.texture) {
#if RENDER2TEXTURE == 2
	if (m_bm.texture->IsRenderBuffer ())
		OGL_BINDTEX (0);
	else
#elif RENDER2TEXTURE == 1
#	ifdef _WIN32
	if (m_bm.texture->bFrameBuf)
		m_bm.texture->pbo.Release ();
	else
#	endif
#endif
		{
		m_bm.texture->Destroy ();
		m_bm.texture = NULL;
		}
	}
}

//------------------------------------------------------------------------------

int CBitmap::AvgColor (tRgbColorf* colorP)
{
	int			c, h, i, j = 0, r = 0, g = 0, b = 0;
	tRgbColorf	*color;
	tRgbColorb	*colorBuf;
	CPalette		*palette;
	ubyte			*bufP;
	
if (!(bufP = m_bm.texBuf))
	return -1;
h = (int) (this - gameData.pig.tex.pBitmaps);
if ((h < 0) || (h >= MAX_BITMAP_FILES)) {
	h = (int) (this - gameData.pig.tex.pAltBitmaps);
	if ((h < 0) || (h >= MAX_BITMAP_FILES)) {
		h = (int) (this - gameData.pig.tex.bitmaps [0]);
		if ((h < 0) || (h >= MAX_BITMAP_FILES))
			return NULL;
		}
	}
color = gameData.pig.tex.bitmapColors + h;
if (!(h = (int) ((color->red + color->green + color->blue) * 255.0f))) {
	if (!(palette = m_bm.palette))
		palette = paletteManager.Default ();
	colorBuf = palette->Color ();
	for (h = i = m_bm.props.w * m_bm.props.h; i; i--, bufP++) {
		if ((c = *bufP) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
			r += colorBuf [c].red;
			g += colorBuf [c].green;
			b += colorBuf [c].blue;
			j++;
			}
		}
	if (j) {
		m_bm.avgColor.red = 4 * (ubyte) (r / j);
		m_bm.avgColor.green = 4 * (ubyte) (g / j);
		m_bm.avgColor.blue = 4 * (ubyte) (b / j);
		j *= 63;	//palette entries are all /4, so do not divide by 256
		color->red = (float) r / (float) j;
		color->green = (float) g / (float) j;
		color->blue = (float) b / (float) j;
		h = m_bm.avgColor.red + m_bm.avgColor.green + m_bm.avgColor.blue;
		}
	else {
		m_bm.avgColor.red =
		m_bm.avgColor.green =
		m_bm.avgColor.blue = 0;
		color->red =
		color->green =
		color->blue = 0.0f;
		h = 0;
		}
	}
if (colorP)
	*colorP = *color;
return h;
}

//------------------------------------------------------------------------------

int CBitmap::AvgColorIndex (void)
{
if (!m_bm.texBuf)
	return 0;
if (m_bm.avgColorIndex) 
	return m_bm.avgColorIndex;

	int			c, h, i, j = 0, r = 0, g = 0, b = 0;
	ubyte			*p = m_bm.texBuf;
	tRgbColorb	*palette = m_bm.palette->Color ();

for (h = i = m_bm.props.w * m_bm.props.h; i; i--, p++) {
	if ((c = *p) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
		r += palette->red;
		g += palette->green;
		b += palette->blue;
		j++;
		}
	}
return j ? m_bm.palette->ClosestColor (r / j, g / j, b / j) : 0;
}

//------------------------------------------------------------------------------

void CBitmap::Swap_0_255 (void)
{
	int	i;
	ubyte	*p;

for (i = m_bm.props.h * m_bm.props.w, p = m_bm.texBuf; i; i--, p++) {
	if (!*p)
		*p = 255;
	else if (*p == 255)
		*p = 0;
	}
}

//------------------------------------------------------------------------------
//eof
