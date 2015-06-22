/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Graphical routines for drawing fonts.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include "u_mem.h"

#include "descent.h"
#include "text.h"
#include "bitmap.h"
#include "font.h"
#include "grdef.h"
#include "error.h"
#include "cfile.h"
#include "mono.h"
#include "byteswap.h"
#include "gamefont.h"
#include "makesig.h"
#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "args.h"
#include "canvas.h"

//------------------------------------------------------------------------------

CFontManager fontManager;

#define LHX(x)	 (gameStates.menus.bHires ? 2 * (x) : x)

//------------------------------------------------------------------------------

void CFont::Init (void)
{
m_info.width = 0;      
m_info.height = 0;     
m_info.flags = 0;      
m_info.baseLine = 0;   
m_info.minChar = 0;    
m_info.maxChar = 0;    
m_info.byteWidth = 0;  
m_info.dataOffs = 0;
m_info.widthOffs = 0;
m_info.kernDataOffs = 0;
m_info.data = NULL;
m_info.widths = NULL; 
m_info.kernData = NULL;
m_info.chars.Clear ();
for (uint32_t i = 0; i < m_info.bitmaps.Length (); i++) {
	m_info.bitmaps [i].Init ();
	m_info.bitmaps [i].SetCartoonizable (0);
	}
m_info.parentBitmap.Init ();
}

//------------------------------------------------------------------------------

int16_t CFont::Width (void) 
{ 
return int16_t (m_info.width * fontManager.Scale ()); 
}

int16_t CFont::Height (void) 
{ 
return int16_t (m_info.height * fontManager.Scale ()); 
}

//------------------------------------------------------------------------------

uint8_t *CFont::FindKernEntry (uint8_t first, uint8_t second)
{
	uint8_t *p = m_info.kernData;

while (*p != 255)
	if ((*p == first) && (*(p + 1) == second))
		return p;
	else p += 3;
return NULL;
}

//------------------------------------------------------------------------------
//takes the character AFTER being offset into font
//takes the character BEFORE being offset into current font

void CFont::GetCharWidth (uint8_t c, uint8_t cNext, int32_t& width, int32_t& spacing)
{
	int32_t letter = c - m_info.minChar;

if (!InFont (letter)) {				//not in font, draw as space
	width = 0;
	if (m_info.flags & FT_PROPORTIONAL)
		spacing = m_info.width / 2;
	else
		spacing = m_info.width;
	}
else {
	width = (m_info.flags & FT_PROPORTIONAL) ? m_info.widths [letter] : m_info.width;
	spacing = width;
	if (m_info.flags & FT_KERNED) {
		if ((cNext != 0) && (cNext != '\n')) {
			int32_t letter2 = cNext - m_info.minChar;
			if (InFont (letter2)) {
				uint8_t *p = FindKernEntry ((uint8_t) letter, (uint8_t) letter2);
				if (p)
					spacing = p [2];
				}
			}
		}
	}
//if (!gameStates.app.bNostalgia)
#if DBG
if (fontManager.Scale () != 1.0f)
#endif
spacing = (int32_t) FRound (spacing * fontManager.Scale ());
}

//------------------------------------------------------------------------------

int32_t CFont::GetLineWidth (const char *s, float scale)
{
	int32_t w, w2, s2;

if (!s)
	return 0;
for (w = 0; *s && (*s != '\n'); s++) {
	if (*s <= 0x06) {
		if (*s <= 0x03)
			s++;
		continue;//skip color codes.
		}
	GetCharWidth (s[0], s[1], w2, s2);
	w += s2;
	}
return Scaled (w, scale);
}

//------------------------------------------------------------------------------

int32_t CFont::GetCenteredX (const char *s, float scale)
{
return ((CCanvas::Current ()->Width () - GetLineWidth (s, scale)) / 2);
}

//------------------------------------------------------------------------------

int32_t CFont::TotalWidth (float scale)
{
if (m_info.flags & FT_PROPORTIONAL) {
	int32_t i, w = 0, c = m_info.minChar;
	for (i = 0; c <= m_info.maxChar; i++, c++) {
		if (m_info.widths [i] < 0)
			Error (TXT_FONT_WIDTH);
		w += m_info.widths [i];
		}
	return w;
	}
return Scaled (m_info.width * Range (), scale);
}

//------------------------------------------------------------------------------

void CFont::ChooseSize (int32_t gap, int32_t& rw, int32_t& rh)
{
	int32_t nChars = Range ();
	int32_t r, x, y, nc = 0, smallest = 999999, smallr = -1, tries;
	int32_t smallprop = 10000;
	int32_t h, w;

for (h = 32; h <= 256; h *= 2) {
	if (m_info.height > h)
		continue;
	r= (h / (m_info.height + gap));
	w = Pow2ize ((TotalWidth () + (nChars - r) * gap) / r, 65536);
	tries = 0;
	do {
		if (tries)
			w = Pow2ize (w + 1, 65536);
		if (tries > 3) 
			break;
		nc = 0;
		y = 0;
		while (y + m_info.height <= h){
			x=0;
			while (x < w) {
				if (nc == nChars)
					break;
				if (m_info.flags & FT_PROPORTIONAL){
					if (x + m_info.widths [nc] + gap > w)
						break;
					x += m_info.widths [nc++]+gap;
					}
				else {
					if (x + m_info.width + gap > w)
						break;
					x += m_info.width + gap;
					nc++;
					}
				}
			if (nc == nChars)
				break;
			y += m_info.height + gap;
			}
		tries++;
		} while (nc != nChars);
	if (nc != nChars)
		continue;

	if (w * h == smallest) {//this gives squarer sizes priority (ie, 128x128 would be better than 512*32)
		if (w >= h){
			if (w / h < smallprop) {
				smallprop = w / h;
				smallest++;
				}
			}
		else {
			if (h / w < smallprop) {
				smallprop = h / w;
				smallest++;//hack
				}
			}
		}
	if (w * h < smallest) {
		smallr = 1;
		smallest = w * h;
		rw = w;
		rh = h;
		}
	}
if (smallr <= 0)
	Error ("couldn't fit font?\n");
}

//------------------------------------------------------------------------------

uint8_t* CFont::Remap (const char *fontname, uint8_t* fontData)
{
if (!m_info.parentBitmap.Buffer ())
	return Load (fontname, fontData);
m_info.parentBitmap.Texture ()->Destroy ();
m_info.parentBitmap.SetTranspType (2);
m_info.parentBitmap.PrepareTexture (0, 0, NULL);
for (uint32_t i = 0; i < m_info.bitmaps.Length (); i++)
	m_info.bitmaps [i].SetTexture (m_info.parentBitmap.Texture ());
return fontData;
}

//------------------------------------------------------------------------------

void CFont::Create (const char *fontname)
{
	uint8_t		*fp;
	CPalette *palette;
	int32_t		nChars = Range ();
	int32_t		i, j, x, y, w, h, tw = 0, th = 0, curx = 0, cury = 0;
	uint8_t		white;
	int32_t		gap = 0; //having a gap just wastes ram, since we don't filter text textures at all.

ChooseSize (gap, tw, th);
palette = m_info.parentBitmap.Palette ();
m_info.parentBitmap.Setup (BM_LINEAR, tw, th, 1, fontname, NULL);
m_info.parentBitmap.Clear (TRANSPARENCY_COLOR);
m_info.parentBitmap.AddFlags (BM_FLAG_TRANSPARENT);
m_info.parentBitmap.SetPalette (palette);
//if (!(m_info.flags & FT_COLOR))
m_info.parentBitmap.Texture ()->Register ();
m_info.bitmaps.Create (nChars); 
//m_info.bitmaps.Clear ();
h = m_info.height;
white = palette->ClosestColor (63, 63, 63);

for (i = 0; i < nChars; i++) {
	if (m_info.flags & FT_PROPORTIONAL)
		w = m_info.widths [i];
	else
		w = m_info.width;
	if (w < 1 || w > 256)
		continue;
	if (curx + w + gap > tw) {
		cury += h + gap;
		curx = 0;
		}
	if (cury + h > th)
		Error (TXT_FONT_SIZE, i, nChars);
	if (m_info.flags & FT_COLOR) {
		if (m_info.flags & FT_PROPORTIONAL)
			fp = m_info.chars [i];
		else
			fp = m_info.data + i * w * h;
		for (y = 0; y < h; y++)
#if 1
			memcpy (m_info.parentBitmap + curx + (cury + y) * tw, fp + y * w, w);
#else
			for (x = 0; x < w; x++)
				m_info.parentBitmap [curx + x + (cury + y) * tw] = fp [x + y * w];
#endif
		}
	else {
		int32_t mask, bits = 0;
		//			if (w*h>sizeof (data))
		//				Error ("OglInitFont: toobig\n");
		if (m_info.flags & FT_PROPORTIONAL)
			fp = m_info.chars [i];
		else
			fp = m_info.data + i * BITS_TO_BYTES (w) * h;
		for (y = 0; y < h; y++) {
			mask = 0;
			j = curx + (cury + y) * tw;
			for (x = 0; x < w; x++) {
				if (mask == 0) {
					bits = *fp++;
					mask = 0x80;
					}
				m_info.parentBitmap [j++] = (uint8_t) ((bits & mask) ? white : TRANSPARENCY_COLOR);
				mask >>= 1;
				}
			}
		}
	m_info.bitmaps [i].InitChild (&m_info.parentBitmap, curx, cury, w, h);
	m_info.bitmaps [i].SetTexture (m_info.parentBitmap.Texture ());
	m_info.bitmaps [i].SetTranspType (2);
	curx += w + gap;
	}
m_info.parentBitmap.SetTranspType (2);
m_info.parentBitmap.PrepareTexture (0, 0, NULL);
}

//------------------------------------------------------------------------------

//remap a font by re-reading its data & palette
void CFont::Setup (const char *fontname, uint8_t* fontData, CPalette& palette)
{
	int32_t	i;
	int32_t	nChars;
	uint8_t*	ptr;

Destroy ();

// make these offsets relative to font data
nChars = m_info.maxChar - m_info.minChar + 1;
if (m_info.flags & FT_PROPORTIONAL) {
	m_info.widths = reinterpret_cast<int16_t*> (fontData + (size_t) m_info.widthOffs - GRS_FONT_SIZE);
	m_info.data = reinterpret_cast<uint8_t*> (fontData + (size_t) m_info.dataOffs - GRS_FONT_SIZE);
	m_info.chars.Create (nChars);
	ptr = m_info.data;
	for (i = 0; i < nChars; i++) {
		m_info.widths [i] = INTEL_SHORT (m_info.widths [i]);
		m_info.chars [i] = ptr;
		if (m_info.flags & FT_COLOR)
			ptr += m_info.widths [i] * m_info.height;
		else
			ptr += BITS_TO_BYTES (m_info.widths [i]) * m_info.height;
		}
	}
else  {
	m_info.data = reinterpret_cast<uint8_t*> (fontData);
	m_info.widths = NULL;
	ptr = m_info.data + (nChars * m_info.width * m_info.height);
	}
if (m_info.flags & FT_KERNED)
	m_info.kernData = reinterpret_cast<uint8_t*> (fontData + (size_t) m_info.kernDataOffs - GRS_FONT_SIZE);

if (m_info.flags & FT_COLOR) {		//remap palette
#ifdef SWAP_TRANSPARENCY_COLOR			// swap the first and last palette entries (black and white)
	palette.SwapTransparency ();
	// we also need to swap the data entries. black is white and white is black
	for (i = 0; i < ptr - m_info.data; i++) {
		if (m_info.data [i] == 0)
			m_info.data [i] = 255;
		else if (m_info.data [i] == 255)
			m_info.data [i] = 0;
		}
#endif
	m_info.parentBitmap.SetCartoonizable (0);
	m_info.parentBitmap.SetPalette (&palette, TRANSPARENCY_COLOR, -1, m_info.data, (int32_t) (ptr - m_info.data));
	}
else
	m_info.parentBitmap.SetPalette (paletteManager.Default (), TRANSPARENCY_COLOR, -1);
Create (fontname);
}

//------------------------------------------------------------------------------

uint8_t* CFont::Load (const char *fontname, uint8_t* fontData)
{
	CFile 	cf;
	CPalette	palette;
	char 		fileId [4];
	int32_t 	dataSize;	//size up to (but not including) palette

if (!cf.Open (fontname, gameFolders.game.szData [0], "rb", 0)) {
#if TRACE
	console.printf (CON_VERBOSE, "Can't open font file %s\n", fontname);
#endif
	return NULL;
	}

cf.Read (fileId, 4, 1);
if (!strncmp (fileId, "NFSP", 4)) {
#if TRACE
	console.printf (CON_NORMAL, "File %s is not a font file\n", fontname);
#endif
	return NULL;
	}

dataSize = cf.ReadInt ();
dataSize -= GRS_FONT_SIZE; // subtract the size of the header.
Read (cf);
if (!(fontData || (fontData = new uint8_t [dataSize]))) {
	cf.Close ();
	return NULL;
	}
cf.Read (fontData, 1, dataSize);
if (m_info.flags & FT_COLOR)
	palette.Read (cf);
cf.Close ();

Setup (fontname, fontData, palette);
return fontData;
}

//------------------------------------------------------------------------------

void CFont::Destroy (void)
{
m_info.chars.Destroy ();
for (uint32_t i = 0; i < m_info.bitmaps.Length (); i++)
	m_info.bitmaps [i].SetTexture (NULL);
m_info.bitmaps.Destroy ();
m_info.parentBitmap.Destroy ();
}

//------------------------------------------------------------------------------

void CFont::Read (CFile& cf)
{
m_info.width = cf.ReadShort ();
m_info.height = cf.ReadShort ();
m_info.flags = cf.ReadShort ();
m_info.baseLine = cf.ReadShort ();
m_info.minChar = cf.ReadByte ();
m_info.maxChar = cf.ReadByte ();
m_info.byteWidth = cf.ReadShort ();
m_info.dataOffs = cf.ReadInt ();
cf.ReadInt (); // Skip
m_info.widthOffs = cf.ReadInt ();
m_info.kernDataOffs = cf.ReadInt ();
}

//------------------------------------------------------------------------------

int32_t CFont::StringSize (const char *s, int32_t& stringWidth, int32_t& stringHeight, int32_t& averageWidth)
{
	int32_t i = 0, longestWidth = 0;
	int32_t width, spacing;
	float fScale = fontManager.Scale ();

stringHeight = m_info.height;
stringWidth = 0;
averageWidth = int32_t (m_info.width * fScale);

if (s && *s) {
	fontManager.SetScale (1.0f / (gameStates.app.bDemoData + 1));
	while (*s) {
		while (*s == '\n') {
			s++;
			stringHeight += m_info.height + 2;
			stringWidth = 0;
			}

		if (*s == 0) 
			break;

		//	1 = next 3 bytes specify color, so skip the 1 and the color value
		if (*s == CC_COLOR)
			s += 4;
		else if (*s == CC_LSPACING) {
			stringHeight += *(s + 1) - '0';
			s += 2;
			}
		else {
			GetCharWidth (s[0], s[1], width, spacing);
			stringWidth += spacing;
			if (stringWidth > longestWidth)
				longestWidth = stringWidth;
			i++;
			s++;
			}
		}
	fontManager.SetScale (fScale / (gameStates.app.bDemoData + 1));
	}
int32_t lineCount = stringHeight / (m_info.height + 2);
stringHeight = int32_t (stringHeight * fScale);
stringWidth = int32_t (longestWidth * fScale);
return lineCount;
}

//------------------------------------------------------------------------------

int32_t CFont::StringSizeTabbed (const char *s, int32_t& stringWidth, int32_t& stringHeight, int32_t& averageWidth, int32_t *nTabs, int32_t nMaxWidth)
{
	float fScale = fontManager.Scale ();

stringWidth = 0;
stringHeight = int32_t (m_info.height * fScale);
averageWidth = int32_t (m_info.width * fScale);

if (!(s && *s))
	return 0;

stringHeight = 0;

	char		* pi, * pj;
	int32_t	w, h, sw = 0, nTab = 0;
	int32_t	nLineCount = 1;
	static char	hs [10000];

strncpy (hs, s, sizeof (hs));
for (pi = pj = hs; ; pi++) {
	switch (*pi) {
		case '\n':
			*pi++ = '\0';
			if (*pi)
				++nLineCount;

		case '\0':
			fontManager.Current ()->StringSize (pj, w, h, averageWidth);
			sw += w;
			if (stringWidth < sw)
				stringWidth = sw;
			stringHeight += h;
			if (*pi == '\0')
				return nLineCount;
			sw = 0;
			pj = pi;
			nTab = 0;
			break;

		case '\t':
			*pi++ = '\0';
			fontManager.Current ()->StringSize (pj, w, h, averageWidth);
			sw += w;
			if (stringHeight == 0)
				stringHeight = h;
			if (nTabs) {
				if (!gameStates.multi.bSurfingNet && nMaxWidth) {
					if (pj) {
						stringWidth = nMaxWidth;
						return nLineCount;
						}
					}
				else {
					int32_t xTab = LHX (int32_t (nTabs [nTab] * fScale));
					if (xTab > sw) {
						if (!nMaxWidth || (xTab < nMaxWidth))
							sw = xTab;
						else {
							stringWidth = nMaxWidth;
							return nLineCount;
							}
						}
					}
				}
			pj = pi;
			nTab++;
			break;

		default:
			break;
		}
	} 
return nLineCount;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CFontManager::Init (void)
{
memset (m_fonts, 0, sizeof (m_fonts));
m_save.Create (10);
SetScale (1.0f);
m_scaleStack.Create (10);
m_scaleStack.SetGrowth (10);
}

//------------------------------------------------------------------------------

const char* gameFontFilenames [] = { 
	"font1-1.fnt",      // Font 0
	"font1-1h.fnt",     // Font 0 High-res
	"font2-1.fnt",      // Font 1
	"font2-1h.fnt",     // Font 1 High-res
	"font2-2.fnt",      // Font 2
	"font2-2h.fnt",     // Font 2 High-res
	"font2-3.fnt",      // Font 3
	"font2-3h.fnt",     // Font 3 High-res
	"font3-1.fnt",      // Font 4
	"font3-1h.fnt",     // Font 4 High-res
	};

void CFontManager::Setup (void)
{
memset (m_fonts, 0, sizeof (m_fonts));

if (!gameStates.render.fonts.bInstalled) {
		int32_t i;

	gameStates.render.fonts.bInstalled = 1;
	gameStates.render.fonts.bHiresAvailable = !gameStates.app.bDemoData;
	// load lores fonts
	for (i = 0; i < MAX_FONTS; i += 2)
		m_gameFonts [i] = Load (gameFontFilenames [i]);
	// load hires fonts
	for (i = 1; i < MAX_FONTS; i += 2)
		if (!(m_gameFonts [i] = Load (gameFontFilenames [i])))
			gameStates.render.fonts.bHiresAvailable = 0;
	}
}

//------------------------------------------------------------------------------

void CFontManager::Destroy (void)
{
	int32_t i;

for (i = 0; (i < MAX_OPEN_FONTS); i++)
	if (m_fonts [i].data)
		Unload (&m_fonts [i].font);
}

//------------------------------------------------------------------------------

CFont *CFontManager::Load (const char* fontname)
{
	int32_t i;

for (i = 0; (i < MAX_OPEN_FONTS) && m_fonts [i].data; i++)
	;
if (i >= MAX_OPEN_FONTS)
	return NULL;
strncpy (m_fonts [i].filename, fontname, SHORT_FILENAME_LEN);
m_fonts [i].data = m_fonts [i].font.Load (fontname);
fontManager.SetCurrent (&m_fonts [i].font);
CCanvas::Current ()->FontColor (0).index = 0;
CCanvas::Current ()->FontColor (1).index = 0;
return &m_fonts [i].font;
}

//------------------------------------------------------------------------------

void CFontManager::Unload (CFont* font)
{
if (!font)
	return;

	int32_t	i;

		//find font in list
for (i = 0; (i < MAX_OPEN_FONTS) && (&m_fonts [i].font != font); i++)
		;
if (i >= MAX_OPEN_FONTS)
	return;
delete m_fonts [i].data;
m_fonts [i].data = NULL;
m_fonts [i].font.Destroy ();
}

//------------------------------------------------------------------------------

float CFontManager::Scale (bool bScaled) 
{ 
return bScaled ? m_scale : m_scale / float (gameStates.app.bDemoData + 1); 
}

//------------------------------------------------------------------------------

float CFontManager::SetScale (float fScale) 
{ 
float fOldScale = m_scale;
m_scale = Clamp (fScale * (gameStates.app.bDemoData + 1), 0.1f, 10.0f); 
#if DBG
if (m_scale < 1.0f)
	BRP;
#endif
return fOldScale;
}

//------------------------------------------------------------------------------

void CFontManager::SetColor (int32_t fg, int32_t bg)
{
if (fg >= 0)
	CCanvas::Current ()->FontColor (0).index = fg;
CCanvas::Current ()->FontColor (0).rgb = 0;
if (bg >= 0)
	CCanvas::Current ()->FontColor (1).index = bg;
CCanvas::Current ()->FontColor (1).rgb = 0;
}

//------------------------------------------------------------------------------

void CFontManager::SetColorRGB (CRGBAColor *fg, CRGBAColor *bg)
{
if (fg) {
	CCanvas::Current ()->FontColor (0).rgb = 1;
	CCanvas::Current ()->FontColor (0).Assign (*fg);
	}
if (bg) {
	CCanvas::Current ()->FontColor (1).rgb = 1;
	CCanvas::Current ()->FontColor (1).Assign (*bg);
	}
}

//------------------------------------------------------------------------------

void CFontManager::SetColorRGBi (uint32_t fg, int32_t bSetFG, uint32_t bg, int32_t bSetBG)
{
if (bSetFG) {
	CCanvas::Current ()->FontColor (0).rgb = 1;
	CCanvas::Current ()->FontColor (0).Red () = RGBA_RED (fg);
	CCanvas::Current ()->FontColor (0).Green () = RGBA_GREEN (fg);
	CCanvas::Current ()->FontColor (0).Blue () = RGBA_BLUE (fg);
	CCanvas::Current ()->FontColor (0).Alpha () = uint8_t (RGBA_ALPHA (fg) * gameStates.render.grAlpha);
	}
if (bSetBG) {
	CCanvas::Current ()->FontColor (1).rgb = 1;
	CCanvas::Current ()->FontColor (1).Red () = RGBA_RED (bg);
	CCanvas::Current ()->FontColor (1).Green () = RGBA_GREEN (bg);
	CCanvas::Current ()->FontColor (1).Blue () = RGBA_BLUE (bg);
	CCanvas::Current ()->FontColor (1).Alpha () = uint8_t (RGBA_ALPHA (bg) * gameStates.render.grAlpha);
	}
}

//------------------------------------------------------------------------------

void CFontManager::Remap (void)
{
for (int32_t i = 0; i < MAX_OPEN_FONTS; i++)
	if (m_fonts [i].data)
		m_fonts [i].font.Remap (m_fonts [i].filename, m_fonts [i].data);
}

 //------------------------------------------------------------------------------

void CFontManager::SetCurrent (CFont* pFont)
{
#if DBG
if (!pFont)
	pFont = GAME_FONT;
#endif
CCanvas::Current ()->SetFont (m_current = pFont); 
}
 
 //------------------------------------------------------------------------------
//eof
