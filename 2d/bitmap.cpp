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
#include "ogl_lib.h"
#include "color.h"
#include "palette.h"
#include "pcx.h"

CStack< CBitmap* > bitmapList;
bool bRegisterBitmaps = false;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool CheckBitmaps (void)
{
#if DBG
uint32_t j = bitmapList.ToS ();

if (j--) {
	for (uint32_t i = 0; i < j; i++)
		if (bitmapList [i] && (!bitmapList [i]->m_info.szName [0] || (bitmapList [i]->m_info.szName [0] == -51)))
			return false;
	}
#endif
return true;
}

//------------------------------------------------------------------------------

#if DBG

uint32_t FindBitmap (CBitmap* pBm)
{
for (uint32_t i = 0, j = bitmapList.ToS (); i < j; i++)
	if (bitmapList [i] == pBm)
		return i + 1;
return 0;
}

#endif

//------------------------------------------------------------------------------

void RegisterBitmap (CBitmap* pBm)
{
#if DBG
if (bRegisterBitmaps) {
	if (!bitmapList.Buffer ()) {
		bitmapList.Create (1000);
		bitmapList.SetGrowth (1000);
		}
	if ((int32_t) bitmapList.ToS () == nDbgTexture)
		BRP;
	if (!FindBitmap (pBm))
		bitmapList.Push (pBm);
	}
#endif
}

//------------------------------------------------------------------------------

void UnregisterBitmap (CBitmap* pBm)
{
#if DBG
uint32_t i = FindBitmap (pBm);
if (i) {
	if (i < bitmapList.ToS ()) 
		bitmapList [i - 1] = *bitmapList.Top ();
	*bitmapList.Top () = NULL;
	bitmapList.Shrink ();
	}
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CBitmap::CBitmap ()
{
#if DBG
RegisterBitmap (this);
#endif
Reset (); 
};

//------------------------------------------------------------------------------

CBitmap::~CBitmap () 
{ 
Destroy (); 
#if DBG
UnregisterBitmap (this);
#endif
}

//------------------------------------------------------------------------------

uint8_t* CBitmap::CreateBuffer (void)
{
if (m_info.props.rowSize * m_info.props.h) {
	CArray<uint8_t>::Resize ((m_info.nBPP > 1) ? m_info.props.h * m_info.props.rowSize : MAX_BMP_SIZE (m_info.props.w, m_info.props.h));
#if DBG
	Clear ();
#endif
	}
return Buffer ();
}

//------------------------------------------------------------------------------

static int32_t _w, _h, _bpp;

CBitmap* CBitmap::Create (uint8_t mode, int32_t w, int32_t h, int32_t bpp, const char* pszName)
{
	CBitmap	*pBm = new CBitmap; 

_w = w;
_h = h;
_bpp = bpp;
if (pBm)
	pBm->Setup (mode, w, h, bpp, pszName);
return pBm;
}

//------------------------------------------------------------------------------

bool CBitmap::Setup (uint8_t mode, int32_t w, int32_t h, int32_t bpp, const char* pszName, uint8_t* buffer)
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
if ((m_info.nType != BM_TYPE_ALT) && m_info.pParent)
	SetBuffer (NULL, 0);
else {
	if (Buffer ())
		CArray<uint8_t>::Destroy ();
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
if (m_info.pTexture == &m_info.texture)
	ReleaseTexture ();
else
#if DBG
	if (m_info.pTexture)
#endif
	m_info.pTexture = NULL;

char szName [FILENAME_LEN];
memcpy (szName, m_info.szName, sizeof (szName));
Reset ();
memcpy (m_info.szName, szName, sizeof (szName));
}

//------------------------------------------------------------------------------

void CBitmap::DestroyFrames (void)
{
if (m_info.frames.pBm) {
	delete[] m_info.frames.pBm;
	m_info.frames.pBm =
	m_info.frames.pCurrent = NULL;
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

void CBitmap::Reset (void) 
{
memset (&m_info, 0, sizeof (m_info));
m_info.bCartoonizable = 1;
m_info.texture.Init ();
m_info.pTexture = &m_info.texture;
m_info.texture.SetBitmap (this);
}

//------------------------------------------------------------------------------

void CBitmap::Init (void) 
{
//Destroy ();
Reset ();
}

//------------------------------------------------------------------------------

void CBitmap::Init (int32_t mode, int32_t x, int32_t y, int32_t w, int32_t h, int32_t bpp, uint8_t *buffer, bool bReset) 
{
if (bReset)
	Init ();
m_info.props.x = x;
m_info.props.y = y;
m_info.props.w = w;
m_info.props.h = h;
m_info.props.nMode = mode;
m_info.nBPP = bpp ? bpp : 1;
m_info.props.rowSize = w * bpp;
m_info.pTexture = &m_info.texture;
m_info.texture.SetBitmap (this);
if (bpp > 2)
	m_info.props.flags = (uint16_t) BM_FLAG_TGA;
SetBuffer (buffer, 0, FrameSize ());
}

//------------------------------------------------------------------------------

CBitmap *CBitmap::CreateChild (int32_t x, int32_t y, int32_t w, int32_t h)
{
    CBitmap *child;

if  (!(child = new CBitmap))
	return NULL;
child->InitChild (this, x, y, w, h);
return child;
}

//------------------------------------------------------------------------------

bool CBitmap::InitChild (CBitmap *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
//*this = *parent;
memcpy (this, parent, sizeof (*this));
m_info.pParent =
m_info.pOverride =
m_info.maskP = NULL;
memset (&m_info.texture, 0, sizeof (m_info.texture));
m_info.pTexture = &m_info.texture;
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
uint32_t nOffset = uint32_t ((m_info.props.y * m_info.props.rowSize) + m_info.props.x * m_info.nBPP);
uint32_t nSize = uint32_t (h * m_info.props.rowSize);
if (nOffset + nSize > parent->Size ())
	return false;
SetBuffer (parent->Buffer () + nOffset, 1, nSize);
return true;
}

//------------------------------------------------------------------------------

void CBitmap::SetTransparent (int32_t bTransparent)
{
if (bTransparent)
	SetFlags (m_info.props.flags | BM_FLAG_TRANSPARENT);
else
	SetFlags (m_info.props.flags & ~BM_FLAG_TRANSPARENT);
}

//------------------------------------------------------------------------------

void CBitmap::SetSuperTransparent (int32_t bTransparent)
{
if (gameData.pig.tex.textureIndex [0][m_info.nId] >= 0) {
	if (bTransparent)
		SetFlags (m_info.props.flags | BM_FLAG_SUPER_TRANSPARENT);
	else
		SetFlags (m_info.props.flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
}

//------------------------------------------------------------------------------

void CBitmap::SetPalette (CPalette *palette, int32_t transparentColor, int32_t superTranspColor, uint8_t* pBuffer, int32_t bufLen)
{
if (!palette) {
	m_info.palette = NULL;
	return;
	}
m_info.palette = paletteManager.Add (*palette, transparentColor, superTranspColor);

	int32_t colorFrequencies [256];

memset (colorFrequencies, 0, 256 * sizeof (int32_t));
if (pBuffer)
	CountColors (pBuffer, bufLen, colorFrequencies);
else if (Buffer ()) {
	if (m_info.props.w == m_info.props.rowSize)
		CountColors (Buffer (), m_info.props.w * m_info.props.h, colorFrequencies);
	else {
		uint8_t *p = Buffer ();
		for (int32_t y = m_info.props.h; y; y--, p += m_info.props.rowSize)
			CountColors (p, m_info.props.w, colorFrequencies);
		}	
	}
if (transparentColor <= 255) {
	if (transparentColor < 0)
		transparentColor = TRANSPARENCY_COLOR;
	SetTransparent (colorFrequencies [transparentColor] != 0);
	}
if (superTranspColor <= 255) {
	if (superTranspColor < 0)
		superTranspColor = SUPER_TRANSP_COLOR;
	SetSuperTransparent (colorFrequencies [superTranspColor] != 0);
	}
SetTranspType ((Flags () | (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) ? 3 : 0);
}

//------------------------------------------------------------------------------

void CBitmap::CheckTransparency (void)
{
if (Buffer ()) {
	uint8_t *data = Buffer ();

for (int32_t i = m_info.props.w * m_info.props.h; i; i--, data++)
	if  (*data++ == TRANSPARENCY_COLOR) {
		SetTransparent (1);
		return;
		}
	}
m_info.props.flags = 0;
}

//---------------------------------------------------------------

int32_t CBitmap::HasTransparency (void)
{
	int32_t	i, nFrames;

if (m_info.nType && (m_info.nBPP == 4))
	return 1;
if (m_info.props.flags & BM_FLAG_OPAQUE)
	return 0;
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

CBitmap *CBitmap::ReleaseTexture (CBitmap *pBm)
{
while ((pBm->Type () != BM_TYPE_ALT) && pBm->Parent () && (pBm != pBm->Parent ()))
	pBm = pBm->Parent ();
pBm->ReleaseTexture ();
return pBm;
}

//------------------------------------------------------------------------------

void CBitmap::ReleaseTexture (void)
{
	CBitmap	*frames = Frames ();

if (frames) {
	int32_t i, nFrames = m_info.props.h / m_info.props.w;

	for (i = 0; i < nFrames; i++) {
		frames [i].ReleaseTexture ();
		}
	}
else if (m_info.pTexture && (m_info.pTexture == &m_info.texture)) 
	m_info.pTexture->Destroy ();
else
	m_info.pTexture = &m_info.texture;
}

//------------------------------------------------------------------------------

int32_t CBitmap::AvgColor (CFloatVector3* pColor, bool bForce)
{
	int32_t			c, h, i, j = 0, r = 0, g = 0, b = 0;
	CFloatVector3	*color;
	CRGBColor	*colorBuf;
	CPalette		*palette;
	uint8_t			*pBuffer;

if (!bForce && (m_info.avgColor.Red () || m_info.avgColor.Green () || m_info.avgColor.Blue ()))
	return 0;
if (m_info.nBPP > 1)
	return 0;
if (!(pBuffer = Buffer ())) {
	m_info.avgColor.Red () = 
	m_info.avgColor.Green () =
	m_info.avgColor.Blue () = 0;
	return -1;
	}
if (gameData.pig.tex.bitmapP.IsElement (this))
	h = int32_t (this - gameData.pig.tex.bitmapP);
else if (gameData.pig.tex.altBitmapP.IsElement (this))
	h = int32_t (this - gameData.pig.tex.altBitmapP);
else if (gameData.pig.tex.bitmaps [0].IsElement (this))
	h = int32_t (this - gameData.pig.tex.bitmaps [0]);
else
	return -1;
color = gameData.pig.tex.bitmapColors + h;
if (!(h = (int32_t) ((color->Red () + color->Green () + color->Blue ()) * 255.0f))) {
	m_info.avgColor.Red () = 
	m_info.avgColor.Green () =
	m_info.avgColor.Blue () = 0;
	if (!(palette = m_info.palette))
		palette = paletteManager.Default ();
	colorBuf = palette->Color ();
	for (h = i = m_info.props.w * m_info.props.h; i; i--, pBuffer++) {
		if ((c = *pBuffer) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
			r += colorBuf [c].Red ();
			g += colorBuf [c].Green ();
			b += colorBuf [c].Blue ();
			j++;
			}
		}
	if (j) {
		m_info.avgColor.Red () = 4 * (uint8_t) (r / j);
		m_info.avgColor.Green () = 4 * (uint8_t) (g / j);
		m_info.avgColor.Blue () = 4 * (uint8_t) (b / j);
		j *= 63;	//palette entries are all /4, so do not divide by 256
		color->Red () = (float) r / (float) j;
		color->Green () = (float) g / (float) j;
		color->Blue () = (float) b / (float) j;
		h = m_info.avgColor.Red () + m_info.avgColor.Green () + m_info.avgColor.Blue ();
		}
	else {
		m_info.avgColor.Red () =
		m_info.avgColor.Green () =
		m_info.avgColor.Blue () = 0;
		color->Red () =
		color->Green () =
		color->Blue () = 0.0f;
		h = 0;
		}
	}
if (pColor)
	*pColor = *color;
return h;
}

//------------------------------------------------------------------------------

inline int32_t sqr(int32_t i) { return i * i; }

int32_t CBitmap::AvgColorIndex (void)
{
	uint8_t *p = Buffer ();

if (!(p && m_info.palette))
	return 0;
if (m_info.avgColorIndex) 
	return m_info.avgColorIndex;

	int32_t			c, i, j = 0, r = 0, g = 0, b = 0;
	CRGBColor	*palette = m_info.palette->Color ();

for (i = m_info.props.w * m_info.props.h; i; i--, p++) {
	if ((c = *p) && (c != TRANSPARENCY_COLOR) && (c != SUPER_TRANSP_COLOR)) {
		r += palette [c].Red ();
		g += palette [c].Green ();
		b += palette [c].Blue ();
		j++;
		}
	}
return m_info.avgColorIndex = j ? m_info.palette->ClosestColor (r / j, g / j, b / j) : 0;
}

//------------------------------------------------------------------------------

CFloatVector *CBitmap::GetAvgColor (CFloatVector *pColor)
{ 
CRGBColor* pc;

pColor->Alpha () = 1.0f;
#if 0
if (!m_data.buffer) {
	pColor->Red () = 
	pColor->Green () =
	pColor->Blue () = 0;
	return pColor;
	}
#endif
if (m_info.nBPP == 1) {
	if (!m_info.palette) {
		pColor->Red () = 
		pColor->Green () =
		pColor->Blue () = 0;
		return pColor;
		}	
	pc = m_info.palette->Color () + AvgColorIndex ();
	}
else
	pc = &m_info.avgColor;
pColor->Red () = float (pc->Red ()) / 255.0f;
pColor->Green () = float (pc->Green ()) / 255.0f;
pColor->Blue () = float (pc->Blue ()) / 255.0f;
return pColor;
}

//------------------------------------------------------------------------------

void CBitmap::SwapTransparencyColor (void)
{
	uint8_t*	p = Buffer ();

for (int32_t i = m_info.props.h * m_info.props.w; i; i--, p++) {
	if (!*p)
		*p = 255;
	else if (*p == 255)
		*p = 0;
	}
}

//------------------------------------------------------------------------------

void CBitmap::RenderStretched (CRectangle* destP, int32_t x, int32_t y)
{ 
Render (destP, x, y, destP ? destP->Width () : -1, destP ? destP->Height () : -1, 0, 0, Width (), Height ()); 
}

//------------------------------------------------------------------------------

void CBitmap::RenderFixed (CRectangle* dest, int32_t x, int32_t y, int32_t w, int32_t h)
{ 
if ((w <= 0) || (w > Width ()))
	w = Width ();
if ((h <= 0) || (h > Height ()))
	h = Height ();
Render (dest, x, y, w, h, 0, 0, w, h); 
}

//------------------------------------------------------------------------------

void CBitmap::NeedSetup (void)
{ 
m_info.bSetup = false; 
m_info.nMasks = 0; 
if (m_info.pParent && (m_info.pParent != this))
	m_info.pParent->NeedSetup ();
#if 0
if (m_info.pOverride)
	m_info.pOverride->NeedSetup ();
else {
	CBitmap* pBmf = m_info.frames.pBm;

	if (pBmf)
		for (int32_t i = m_info.frames.nCount; i; i--, pBmf++)
			pBmf->NeedSetup ();
	}
#endif
}

//------------------------------------------------------------------------------

void CBitmap::FreeData (void)
{
if (Buffer ()) {
	UseBitmapCache (this, -int32_t (Size ()));
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

int32_t CBitmap::FreeHiresFrame (int32_t bD1)
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

int32_t CBitmap::FreeHiresAnimation (int32_t bD1)
{
	CBitmap*	pAltBm, * pBmf;
	int32_t		i;

if (!(pAltBm = Override ()))
	pAltBm = this; //return 0;
SetOverride (NULL);
if (BPP () == 1)
	DelFlags (BM_FLAG_TGA);
if ((pAltBm->Type () == BM_TYPE_FRAME) && !(pAltBm = pAltBm->Parent ()))
	return 1;
if (pAltBm->Type () != BM_TYPE_ALT)
	return 0;	//actually this would be an error
if ((pBmf = pAltBm->Frames ()))
	for (i = pAltBm->FrameCount (); i; i--, pBmf++)
		pBmf->FreeHiresFrame (bD1);
else
	pAltBm->FreeMask ();
pAltBm->ReleaseTexture ();
pAltBm->DestroyFrames ();
pAltBm->FreeData ();
pAltBm->SetPalette (NULL);
pAltBm->SetType (0);
pAltBm->NeedSetup ();
return 1;
}

//------------------------------------------------------------------------------

void CBitmap::Unload (int32_t i, int32_t bD1)
{
m_info.bSetup = false;
m_info.nMasks = 0;
m_info.nTranspType = 0;
if (i < 0)
	i = int32_t (this - gameData.pig.tex.bitmaps [bD1]);
FreeMask ();
if (!FreeHiresAnimation (bD1))
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

CBitmap* CBitmap::SetOverride (CBitmap *pOverride) 
{
if (pOverride == this)
	return m_info.pOverride;
CBitmap*	oldOverrideP = m_info.pOverride;
m_info.pOverride = pOverride;
return oldOverrideP;
}

//------------------------------------------------------------------------------

void CBitmap::SetName (const char* pszName) 
{ 
if (pszName) {
	strncpy (m_info.szName, pszName, sizeof (m_info.szName)); 
	m_info.szName [sizeof (m_info.szName) - 1] = '\0';
	}
}

//------------------------------------------------------------------------------
//eof
