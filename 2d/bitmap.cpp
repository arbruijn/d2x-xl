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

#include "descent.h"
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
if ((m_info.nType != BM_TYPE_ALT) && m_info.parentP)
	SetBuffer (NULL, 0);
else {
	if (Buffer ())
		CArray<ubyte>::Destroy ();
#if TEXTURE_COMPRESSION
	m_info.compressed.buffer.Destroy ();
#endif
	}
//ReleaseTexture ();
}

//------------------------------------------------------------------------------

void CBitmap::Destroy (void)
{
SetPalette (NULL);
DestroyBuffer ();
DestroyFrames ();
DestroyMask ();
if (m_info.texP == &m_info.texture)
	ReleaseTexture ();
else
	m_info.texP = NULL;
m_info.texture.Init ();
m_info.texture.SetBitmap (NULL);
}

//------------------------------------------------------------------------------

void CBitmap::DestroyFrames (void)
{
if (m_info.frames.bmP) {
	delete[] m_info.frames.bmP;
	m_info.frames.bmP =
	m_info.frames.currentP = NULL;
	m_info.frames.nCount = 0;
	}
}

//------------------------------------------------------------------------------

void CBitmap::DestroyMask (void)
{
delete m_info.maskP;
m_info.maskP = NULL;
}

//------------------------------------------------------------------------------

void CBitmap::Init (void) 
{
	static int nSignature = 0;
	char szSignature [20];

memset (&m_info, 0, sizeof (m_info));
m_info.texP = &m_info.texture;
m_info.texture.SetBitmap (this);
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
m_info.texP = &m_info.texture;
m_info.texture.SetBitmap (this);
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

bool CBitmap::InitChild (CBitmap *parent, int x, int y, int w, int h)
{
*this = *parent;
m_info.parentP =
m_info.overrideP =
m_info.maskP = NULL;
memset (&m_info.texture, 0, sizeof (m_info.texture));
m_info.texP = &m_info.texture;
memset (&m_info.frames, 0, sizeof (m_info.frames));
m_info.bChild = 1;
m_info.props.x += x;
if (m_info.props.x > parent->Width () - 1)
	m_info.props.x = parent->Width () - 1;
m_info.props.y += y;
if (m_info.props.y > parent->Height () - 1)
	m_info.props.y = parent->Height () - 1;
if (w > parent->Width () - m_info.props.x)
	w = parent->Width () - m_info.props.x;
m_info.props.w = w;
if (h > parent->Height () - m_info.props.y)
	h = parent->Height () - m_info.props.y;
m_info.props.h = h;
SetParent (parent ? parent : this);
SetBuffer (NULL);	//force buffer to being a child even if identical with parent buffer
uint nOffset = uint ((m_info.props.y * m_info.props.rowSize) + m_info.props.x * m_info.nBPP);
uint nSize = uint (h * m_info.props.rowSize);
if (nOffset + nSize > parent->Size ())
	return false;
SetBuffer (parent->Buffer () + nOffset, 1, nSize);
return true;
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
	if  (*data++ == TRANSPARENCY_COLOR) {
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
if (m_info.props.w) {
	nFrames = m_info.props.h / m_info.props.w;
	for (i = 1; i < nFrames; i++) {
		if (m_info.transparentFrames [i / 32] & (1 << (i % 32)))
			return 1;
		if (m_info.supertranspFrames [i / 32] & (1 << (i % 32)))
			return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::ReleaseTexture (CBitmap *bmP)
{
while ((bmP->Type () != BM_TYPE_ALT) && bmP->Parent () && (bmP != bmP->Parent ()))
	bmP = bmP->Parent ();
bmP->ReleaseTexture ();
return bmP;
}

//------------------------------------------------------------------------------

void CBitmap::ReleaseTexture (void)
{
	CBitmap	*frames = Frames ();

if (frames) {
	int i, nFrames = m_info.props.h / m_info.props.w;

	for (i = 0; i < nFrames; i++) {
		frames [i].ReleaseTexture ();
		}
	}
else if (m_info.texP && (m_info.texP == &m_info.texture)) {
#if RENDER2TEXTURE == 2
	if (m_info.texP->IsRenderBuffer ())
		OGL_BINDTEX (0);
	else
#elif RENDER2TEXTURE == 1
#	ifdef _WIN32
	if (m_info.texP->bFrameBuf)
		m_info.texP->pbo.Destroy ();
	else
#	endif
#endif
		{
		m_info.texP->Destroy ();
		}
	}
else
	m_info.texP = &m_info.texture;
}

//------------------------------------------------------------------------------

int CBitmap::AvgColor (tRgbColorf* colorP, bool bForce)
{
	int			c, h, i, j = 0, r = 0, g = 0, b = 0;
	tRgbColorf	*color;
	tRgbColorb	*colorBuf;
	CPalette		*palette;
	ubyte			*bufP;

if (!bForce && (m_info.avgColor.red || m_info.avgColor.green || m_info.avgColor.blue))
	return 0;
if (m_info.nBPP > 1)
	return 0;
if (!(bufP = Buffer ())) {
	m_info.avgColor.red = 
	m_info.avgColor.green =
	m_info.avgColor.blue = 0;
	return -1;
	}
if (gameData.pig.tex.bitmapP.IsElement (this))
	h = int (this - gameData.pig.tex.bitmapP);
else if (gameData.pig.tex.altBitmapP.IsElement (this))
	h = int (this - gameData.pig.tex.altBitmapP);
else if (gameData.pig.tex.bitmaps [0].IsElement (this))
	h = int (this - gameData.pig.tex.bitmaps [0]);
else
	return -1;
color = gameData.pig.tex.bitmapColors + h;
if (!(h = (int) ((color->red + color->green + color->blue) * 255.0f))) {
	m_info.avgColor.red = 
	m_info.avgColor.green =
	m_info.avgColor.blue = 0;
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

if (!(p && m_info.palette))
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

tRgbaColorf *CBitmap::GetAvgColor (tRgbaColorf *colorP)
{ 
tRgbColorb* pc;

colorP->alpha = 1.0f;
#if 0
if (!m_data.buffer) {
	colorP->red = 
	colorP->green =
	colorP->blue = 0;
	return colorP;
	}
#endif
if (m_info.nBPP == 1) {
	if (!m_info.palette) {
		colorP->red = 
		colorP->green =
		colorP->blue = 0;
		return colorP;
		}	
	pc = m_info.palette->Color () + m_info.avgColorIndex;
	}
else
	pc = &m_info.avgColor;
colorP->red = float (pc->red) / 255.0f;
colorP->green = float (pc->green) / 255.0f;
colorP->blue = float (pc->blue) / 255.0f;
return colorP;
}

//------------------------------------------------------------------------------

void CBitmap::Swap_0_255 (void)
{
	ubyte*	p = Buffer ();

for (int i = m_info.props.h * m_info.props.w; i; i--, p++) {
	if (!*p)
		*p = 255;
	else if (*p == 255)
		*p = 0;
	}
}

//------------------------------------------------------------------------------

void CBitmap::RenderStretched (CBitmap* dest, int x, int y)
{ 
if (!dest)
	dest = CCanvas::Current ();
if (dest)
	Render (dest, x, y, dest->Width (), dest->Height (), 0, 0, Width (), Height ()); 
}

//------------------------------------------------------------------------------

void CBitmap::RenderFixed (CBitmap* dest, int x, int y, int w, int h)
{ 
if ((w <= 0) || (w > Width ()))
	w = Width ();
if ((h <= 0) || (h > Height ()))
	h = Height ();
Render (dest ? dest : CCanvas::Current (), x, y, w, h, 0, 0, w, h); 
}

//------------------------------------------------------------------------------

void CBitmap::NeedSetup (void)
{ 
m_info.bSetup = false; 
m_info.nMasks = 0; 
if (m_info.parentP && (m_info.parentP != this))
	m_info.parentP->NeedSetup ();
#if 0
if (m_info.overrideP)
	m_info.overrideP->NeedSetup ();
else {
	CBitmap* bmfP = m_info.frames.bmP;

	if (bmfP)
		for (int i = m_info.frames.nCount; i; i--, bmfP++)
			bmfP->NeedSetup ();
	}
#endif
}

//------------------------------------------------------------------------------

void CBitmap::FreeData (void)
{
if (Buffer ()) {
	UseBitmapCache (this, -int (Size ()));
	DestroyBuffer ();
	}
}

//------------------------------------------------------------------------------

void CBitmap::FreeMask (void)
{
	CBitmap	*mask;

if ((mask = Mask ())) {
	mask->FreeData ();
	DestroyMask ();
	}
}

//------------------------------------------------------------------------------

int CBitmap::FreeHiresFrame (int bD1)
{
if (m_info.nId < 0x8000)
	gameData.pig.tex.bitmaps [bD1][m_info.nId].SetOverride (NULL);
ReleaseTexture ();
FreeMask ();
SetType (0);
if (BPP () == 1)
	DelFlags (BM_FLAG_TGA);
SetBuffer (NULL);
return 1;
}

//------------------------------------------------------------------------------

int CBitmap::FreeHiresAnimation (int bD1)
{
	CBitmap*	altBmP, * bmfP;
	int		i;

if (!(altBmP = Override ()))
	return 0;
SetOverride (NULL);
if (BPP () == 1)
	DelFlags (BM_FLAG_TGA);
if ((altBmP->Type () == BM_TYPE_FRAME) && !(altBmP = altBmP->Parent ()))
	return 1;
if (altBmP->Type () != BM_TYPE_ALT)
	return 0;	//actually this would be an error
if ((bmfP = altBmP->Frames ()))
	for (i = altBmP->FrameCount (); i; i--, bmfP++)
		bmfP->FreeHiresFrame (bD1);
else
	altBmP->FreeMask ();
altBmP->ReleaseTexture ();
altBmP->DestroyFrames ();
altBmP->FreeData ();
altBmP->SetPalette (NULL);
altBmP->SetType (0);
altBmP->NeedSetup ();
return 1;
}

//------------------------------------------------------------------------------

void CBitmap::Unload (int i, int bD1)
{
m_info.bSetup = false;
m_info.nMasks = 0;
if (i < 0)
	i = int (this - gameData.pig.tex.bitmaps [bD1]);
FreeMask ();
if (!FreeHiresAnimation (0))
	ReleaseTexture ();
if (bitmapOffsets [bD1][i] > 0)
	AddFlags (BM_FLAG_PAGED_OUT);
if (BPP () == 1)
	DelFlags (BM_FLAG_TGA);
SetFromPog (0);
SetPalette (NULL);
FreeData ();
}

//------------------------------------------------------------------------------
//eof
