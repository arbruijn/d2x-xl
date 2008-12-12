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

ubyte* CBitmap::CreateBuffer (void)
{
if (m_info.props.rowSize * m_info.props.h) {
	CArray<ubyte>::Resize ((m_info.nBPP > 1) ? m_info.props.h * m_info.props.rowSize : MAX_BMP_SIZE (m_info.props.w, m_info.props.h));
#if DBG
	Clear ();
#endif
	}
return Buffer ();
}

//------------------------------------------------------------------------------

CBitmap* CBitmap::Create (ubyte mode, int w, int h, int bpp, const char* pszName)
{
	CBitmap	*bmP = new CBitmap; 

if (bmP)
	bmP->Setup (mode, w, h, bpp, pszName);
return bmP;
}

//------------------------------------------------------------------------------

bool CBitmap::Setup (ubyte mode, int w, int h, int bpp, const char* pszName, ubyte* buffer)
{
Init (mode, 0, 0, w, h, bpp);
SetName (pszName);
if (!buffer)
	CreateBuffer ();
return Buffer () != NULL;
}

//------------------------------------------------------------------------------

void CBitmap::DestroyBuffer (void)
{
if ((m_info.nType != BM_TYPE_ALT) && m_info.info.std.parent)
	SetBuffer (NULL, 0);
else if (Buffer ())
	CArray<ubyte>::Destroy ();
FreeTexture ();
}

//------------------------------------------------------------------------------

void CBitmap::Destroy (void)
{
SetPalette (NULL);
DestroyBuffer ();
DestroyFrames ();
DestroyMask ();
}

//------------------------------------------------------------------------------

void CBitmap::DestroyFrames (void)
{
if (m_info.info.alt.frames) {
	delete[] m_info.info.alt.frames;
	m_info.info.alt.curFrame = NULL;
	m_info.info.alt.nFrameCount = 0;
	}
}

//------------------------------------------------------------------------------

void CBitmap::DestroyMask (void)
{
delete m_info.info.std.mask;
m_info.info.std.mask = NULL;
}

//------------------------------------------------------------------------------

void CBitmap::Init (void) 
{
	static int nSignature = 0;
	char szSignature [20];

memset (&m_info, 0, sizeof (m_info));
sprintf (szSignature, "Bitmap %d", nSignature++);
SetName (szSignature);
}

//------------------------------------------------------------------------------

void CBitmap::Init (int mode, int x, int y, int w, int h, int bpp, ubyte *buffer) 
{
Init ();
m_info.props.x = x;
m_info.props.y = y;
m_info.props.w = w;
m_info.props.h = h;
m_info.props.nMode = mode;
m_info.nBPP = bpp ? bpp : 1;
m_info.props.rowSize = w * bpp;
if (bpp > 2)
	m_info.props.flags = (char) BM_FLAG_TGA;
SetBuffer (buffer, false, FrameSize ());
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
memset (&m_info.info, 0, sizeof (m_info.info));
m_info.bChild = 1;
m_info.props.x += x;
m_info.props.y += y;
m_info.props.w = w;
m_info.props.h = h;
SetParent (parent ? parent : this);
SetBuffer (parent->Buffer () + (uint) ((m_info.props.y * m_info.props.rowSize) + m_info.props.x * m_info.nBPP), true);
}

//------------------------------------------------------------------------------

void CBitmap::SetTransparent (int bTransparent)
{
if (bTransparent)
	SetFlags (m_info.props.flags | BM_FLAG_TRANSPARENT);
else
	SetFlags (m_info.props.flags & ~BM_FLAG_TRANSPARENT);
}

//------------------------------------------------------------------------------

void CBitmap::SetSuperTransparent (int bTransparent)
{
if (gameData.pig.tex.textureIndex [0][m_info.nId] >= 0) {
	if (bTransparent)
		SetFlags (m_info.props.flags | BM_FLAG_SUPER_TRANSPARENT);
	else
		SetFlags (m_info.props.flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
}

//------------------------------------------------------------------------------

void CBitmap::SetPalette (CPalette *palette, int transparentColor, int supertranspColor, int *freq)
{
if (!palette)
	m_info.palette = NULL;
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
	m_info.palette = paletteManager.Add (*palette);
	}
}

//------------------------------------------------------------------------------

void CBitmap::Remap (CPalette *palette, int transparentColor, int supertranspColor)
{
	int freq [256];

memset (freq, 0, 256 * sizeof (int));
if (!palette)
	palette = m_info.palette;
if (!palette)
	return;
if (m_info.props.w == m_info.props.rowSize)
	CountColors (Buffer (), m_info.props.w * m_info.props.h, freq);
else {
	int y;
	ubyte *p = Buffer ();
	for (y = m_info.props.h; y; y--, p += m_info.props.rowSize)
		CountColors (p, m_info.props.w, freq);
	}
SetPalette (palette, transparentColor, supertranspColor, freq);
}

//------------------------------------------------------------------------------

void CBitmap::CheckTransparency (void)
{
	ubyte *data = Buffer ();

for (int i = m_info.props.w * m_info.props.h; i; i--, data++)
	if  (*data++ == TRANSPARENCY_COLOR)	{
		SetTransparent (1);
		return;
		}
m_info.props.flags = 0;
}

//---------------------------------------------------------------

int CBitmap::HasTransparency (void)
{
	int	i, nFrames;

if (m_info.nType && (m_info.nBPP == 4))
	return 1;
if (m_info.props.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
	return 1;
nFrames = m_info.props.h / m_info.props.w;
for (i = 1; i < nFrames; i++) {
	if (m_info.transparentFrames [i / 32] & (1 << (i % 32)))
		return 1;
	if (m_info.supertranspFrames [i / 32] & (1 << (i % 32)))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::FreeTexture (CBitmap *bmP)
{
while ((bmP->Type () != BM_TYPE_ALT) && bmP->Parent () && (bmP != bmP->Parent ()))
	bmP = bmP->Parent ();
bmP->FreeTexture ();
return bmP;
}

//------------------------------------------------------------------------------

void CBitmap::FreeTexture (void)
{
	CBitmap	*frames = Frames ();

if (frames) {
	int i, nFrames = m_info.props.h / m_info.props.w;

	for (i = 0; i < nFrames; i++) {
		frames [i].FreeTexture ();
		frames [i].SetTexture (NULL);
		}
	}
else if (m_info.texture) {
#if RENDER2TEXTURE == 2
	if (m_info.texture->IsRenderBuffer ())
		OGL_BINDTEX (0);
	else
#elif RENDER2TEXTURE == 1
#	ifdef _WIN32
	if (m_info.texture->bFrameBuf)
		m_info.texture->pbo.Release ();
	else
#	endif
#endif
		{
		m_info.texture->Destroy ();
		m_info.texture = NULL;
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
	
if (!(bufP = Buffer ()))
	return -1;
h = (int) (this - gameData.pig.tex.bitmapP);
if ((h < 0) || (h >= MAX_BITMAP_FILES)) {
	h = (int) (this - gameData.pig.tex.altBitmapP);
	if ((h < 0) || (h >= MAX_BITMAP_FILES)) {
		h = (int) (this - gameData.pig.tex.bitmaps [0]);
		if ((h < 0) || (h >= MAX_BITMAP_FILES))
			return -1;
		}
	}
color = gameData.pig.tex.bitmapColors + h;
if (!(h = (int) ((color->red + color->green + color->blue) * 255.0f))) {
	if (!(palette = m_info.palette))
		palette = paletteManager.Default ();
	colorBuf = palette->Color ();
	for (h = i = m_info.props.w * m_info.props.h; i; i--, bufP++) {
		if ((c = *bufP) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
			r += colorBuf [c].red;
			g += colorBuf [c].green;
			b += colorBuf [c].blue;
			j++;
			}
		}
	if (j) {
		m_info.avgColor.red = 4 * (ubyte) (r / j);
		m_info.avgColor.green = 4 * (ubyte) (g / j);
		m_info.avgColor.blue = 4 * (ubyte) (b / j);
		j *= 63;	//palette entries are all /4, so do not divide by 256
		color->red = (float) r / (float) j;
		color->green = (float) g / (float) j;
		color->blue = (float) b / (float) j;
		h = m_info.avgColor.red + m_info.avgColor.green + m_info.avgColor.blue;
		}
	else {
		m_info.avgColor.red =
		m_info.avgColor.green =
		m_info.avgColor.blue = 0;
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
	ubyte *p = Buffer ();

if (!p)
	return 0;
if (m_info.avgColorIndex) 
	return m_info.avgColorIndex;

	int			c, h, i, j = 0, r = 0, g = 0, b = 0;
	tRgbColorb	*palette = m_info.palette->Color ();

for (h = i = m_info.props.w * m_info.props.h; i; i--, p++) {
	if ((c = *p) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
		r += palette->red;
		g += palette->green;
		b += palette->blue;
		j++;
		}
	}
return j ? m_info.palette->ClosestColor (r / j, g / j, b / j) : 0;
}

//------------------------------------------------------------------------------

void CBitmap::Swap_0_255 (void)
{
	int	i;
	ubyte	*p;

for (i = m_info.props.h * m_info.props.w, p = Buffer (); i; i--, p++) {
	if (!*p)
		*p = 255;
	else if (*p == 255)
		*p = 0;
	}
}

//------------------------------------------------------------------------------
//eof
