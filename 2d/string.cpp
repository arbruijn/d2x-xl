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

#define LHX(x)	 (gameStates.menus.bHires ? 2 * (x) : x)

//int32_t GrInternalStringClipped (int32_t x, int32_t y, const char *s);
//int32_t GrInternalStringClippedM (int32_t x, int32_t y, const char *s);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define STRINGPOOL		1
#define GRS_MAX_STRINGS	1000

grsString stringPool [GRS_MAX_STRINGS];
int16_t nPoolStrings = 0;

//------------------------------------------------------------------------------

void InitStringPool (void)
{
nPoolStrings = 0;
memset (stringPool, 0, sizeof (stringPool));
}

//------------------------------------------------------------------------------

void FreeStringPool (void)
{
	int32_t			i;
	grsString	*ps;

PrintLog (1, "unloading string pool\n");
for (i = nPoolStrings, ps = stringPool; i; i--, ps++) {
	if (ps->pszText) {
		delete[] ps->pszText;
		ps->pszText = NULL;
		}
	if (ps->pId)
		*ps->pId = 0;
	if (ps->bmP) {
		ps->bmP->ReleaseTexture ();
		delete ps->bmP;
		ps->bmP = NULL;
		}
	}
PrintLog (-1);
PrintLog (1, "initializing string pool\n");
InitStringPool ();
PrintLog (-1);
}

//------------------------------------------------------------------------------

grsString *CreatePoolString (const char *s, int32_t *idP)
{
if (!fontManager.Current ())
	return NULL;
if (fontManager.Scale () != 1.0f)
	return NULL;

	grsString*	ps;
	int32_t			l, w, h, aw;

if (*idP) {
	ps = stringPool + *idP - 1;
	ps->bmP->ReleaseTexture ();
	delete ps->bmP;
	ps->bmP = NULL;
	}
else {
	if (nPoolStrings >= GRS_MAX_STRINGS)
		return NULL;
	ps = stringPool + nPoolStrings;
	}
fontManager.Current ()->StringSize (s, w, h, aw);
if (!(ps->bmP = CreateStringBitmap (s, 0, 0, 0, 0, w, 0, 1))) {
	*idP = 0;
	return NULL;
	}
l = (int32_t) strlen (s) + 1;
if (ps->pszText && (ps->nLength < l)) {
	delete[] ps->pszText;
	ps->pszText = NULL;
	}
if (!ps->pszText) {
	ps->nLength = 3 * l / 2;
	if (!(ps->pszText = new char [ps->nLength])) {
		delete ps->bmP;
		ps->bmP = NULL;
		*idP = 0;
		return NULL;
		}
	}
memcpy (ps->pszText, s, l);
ps->nWidth = w;
if (!*idP)
	*idP = ++nPoolStrings;
ps->pId = idP;
return ps;
}

//------------------------------------------------------------------------------

grsString *GetPoolString (const char *s, int32_t *idP)
{
	grsString	*ps;

if (!idP)
	return NULL;
if (*idP > nPoolStrings)
	*idP = 0;
else if (*idP) {
	ps = stringPool + *idP - 1;
	if (ps->bmP && ps->pszText && !strcmp (ps->pszText, s))
		return ps;
	}
return CreatePoolString (s, idP);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have origColor var set (or they could be passed as args...)
//perhaps some sort of recursive origColor nType thing would be better, but that would be way too much trouble for little gain
int32_t grMsgColorLevel = 1;

//------------------------------------------------------------------------------

#if 0

int32_t GrInternalString0 (int32_t x, int32_t y, const char *s)
{
	uint8_t*		fp;
	const char*	textP, *nextRowP, *text_ptr1;
	int32_t			r, mask, i, bits, width, spacing, letter, underline;
	int32_t			skip_lines = 0;
	uint32_t			videoOffset, videoOffset1;
	CPalette*	palette = paletteManager.Game ();
	uint8_t*		videoBuffer = CCanvas::Current ()->Buffer ();
	int32_t			rowSize = CCanvas::Current ()->RowSize ();
	tFont			font;
	char			c;
	
if (CCanvas::Current ()->FontColor (0).rgb) {
	CCanvas::Current ()->FontColor (0).rgb = 0;
	CCanvas::Current ()->FontColor (0).index = palette->ClosestColor (CCanvas::Current ()->FontColor (0).Red (), 
																							CCanvas::Current ()->FontColor (0).Green (), 
																							CCanvas::Current ()->FontColor (0).Blue ());
	}
if (CCanvas::Current ()->FontColor (1).rgb) {
	CCanvas::Current ()->FontColor (1).rgb = 0;
	CCanvas::Current ()->FontColor (1).index = palette->ClosestColor (CCanvas::Current ()->FontColor (1).Red (), 
																							CCanvas::Current ()->FontColor (1).Green (), 
																							CCanvas::Current ()->FontColor (1).Blue ());
	}
bits = 0;
videoOffset1 = y * rowSize + x;
nextRowP = s;
fontManager.Current ()->GetInfo (font);
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	if (x == 0x8000) {			//centered
		int32_t xx = fontManager.Current ()->GetCenteredX (text_ptr1);
		videoOffset1 = y * rowSize + xx;
		}
	for (r = 0; r < font.height; r++) {
		textP = text_ptr1;
		videoOffset = videoOffset1;
		while ((c = *textP)) {
			if (c == '\n') {
				nextRowP = textP + 1;
				break;
				}
			if (c == CC_COLOR) {
				CCanvas::Current ()->FontColor (0).index = *(++textP);
				CCanvas::Current ()->FontColor (0).rgb = 0;
				textP++;
				continue;
				}
			if (c == CC_LSPACING) {
				skip_lines = *(++textP) - '0';
				textP++;
				continue;
				}
			underline = 0;
			if (c == CC_UNDERLINE) {
				if ((r == font.baseLine + 2) || (r == font.baseLine + 3))
					underline = 1;
				textP++;
				}
			fontManager.Current ()->GetCharWidth (textP[0], textP[1], width, spacing);
			letter = c - font.minChar;
			if (!fontManager.Current ()->InFont (letter)) {	//not in font, draw as space
				videoOffset += spacing;
				textP++;
				continue;
				}
			if (font.flags & FT_PROPORTIONAL)
				fp = font.chars [letter];
			else
				fp = font.data + letter * BITS_TO_BYTES (width) * font.height;
			if (underline)
				for (i = 0; i < width; i++)
					videoBuffer [videoOffset++] = (uint8_t) CCanvas::Current ()->FontColor (0).index;
				else {
					fp += BITS_TO_BYTES (width)*r;
					mask = 0;
					for (i = 0; i < width; i++) {
						if (mask == 0) {
							bits = *fp++;
							mask = 0x80;
							}
						if (bits & mask)
							videoBuffer [videoOffset++] = (uint8_t) CCanvas::Current ()->FontColor (0).index;
						else
							videoBuffer [videoOffset++] = (uint8_t) CCanvas::Current ()->FontColor (1).index;
						mask >>= 1;
						}
					}
			videoOffset += spacing-width;		//for kerning
			textP++;
			}
		videoOffset1 += rowSize; y++;
		}
	y += skip_lines;
	videoOffset1 += rowSize * skip_lines;
	skip_lines = 0;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

static inline const char* ScanEmbeddedColors (char c, const char* textP, int32_t origColor, int32_t nOffset, int32_t nScale)
{
if ((c >= 1) && (c <= 3)) {
	if (textP [1]) {
		if (grMsgColorLevel >= c) {
			CCanvas::Current ()->FontColor (0).rgb = 1;
			CCanvas::Current ()->FontColor (0).Set ((textP [1] - nOffset) * nScale, (textP [2] - nOffset) * nScale, (textP [3] - nOffset) * nScale, 255);
			}
		return textP + 4;
		}
	}
else if ((c >= 4) && (c <= 6)) {
	if (grMsgColorLevel >= *textP - 3) {
		CCanvas::Current ()->FontColor (0).index = origColor;
		CCanvas::Current ()->FontColor (0).rgb = 0;
		}
	return textP + 1;
	}
return textP + 1;
}

//------------------------------------------------------------------------------

#if 0

int32_t GrInternalString0m (int32_t x, int32_t y, const char *s)
{
	uint8_t*			fp;
	const char*		textP, * nextRowP, * text_ptr1;
	int32_t				r, mask, i, bits, width, spacing, letter, underline;
	int32_t				skip_lines = 0;
	char				c;
	int32_t				origColor;
	uint32_t				videoOffset, videoOffset1;
	uint8_t*			videoBuffer = CCanvas::Current ()->Buffer ();
	int32_t				rowSize = CCanvas::Current ()->RowSize ();
	tFont				font;

if (CCanvas::Current ()->FontColor (0).rgb) {
	CCanvas::Current ()->FontColor (0).rgb = 0;
	CCanvas::Current ()->FontColor (0).index = fontManager.Current ()->ParentBitmap ().Palette ()->ClosestColor (CCanvas::Current ()->FontColor (0).Red (), CCanvas::Current ()->FontColor (0).Green (), CCanvas::Current ()->FontColor (0).Blue ());
	}
if (CCanvas::Current ()->FontColor (1).rgb) {
	CCanvas::Current ()->FontColor (1).rgb = 0;
	CCanvas::Current ()->FontColor (1).index = fontManager.Current ()->ParentBitmap ().Palette ()->ClosestColor (CCanvas::Current ()->FontColor (1).Red (), CCanvas::Current ()->FontColor (1).Green (), CCanvas::Current ()->FontColor (1).Blue ());
	}
origColor = CCanvas::Current ()->FontColor (0).index;//to allow easy reseting to default string color with colored strings -MPM
bits=0;
videoOffset1 = y * rowSize + x;
fontManager.Current ()->GetInfo (font);
nextRowP = s;
CCanvas::Current ()->FontColor (0).rgb = 0;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;

	if (x==0x8000) {			//centered
		int32_t xx = fontManager.Current ()->GetCenteredX (text_ptr1);
		videoOffset1 = y * rowSize + xx;
		}

	for (r = 0; r < font.height; r++) {
		textP = text_ptr1;
		videoOffset = videoOffset1;
		while ((c = *textP)) {
			if (c == '\n') {
				nextRowP = textP + 1;
				break;
				}
			if (c == CC_COLOR) {
				CCanvas::Current ()->FontColor (0).index = * (++textP);
				textP++;
				continue;
				}
			if (c == CC_LSPACING) {
				skip_lines = * (++textP) - '0';
				textP++;
				continue;
				}
			underline = 0;
			if (c == CC_UNDERLINE) {
				if ((r==font.baseLine+2) || (r==font.baseLine+3))
					underline = 1;
				c = * (++textP);
				}
			fontManager.Current ()->GetCharWidth (c, textP[1], width, spacing);
			letter = c - font.minChar;
			if (c <= 0x06) {	//not in font, draw as space
				textP = ScanEmbeddedColors (c, textP, origColor, 0, 1);
				continue;
				}

			if (!fontManager.Current ()->InFont (letter)) {
				videoOffset += spacing;
				textP++;
				}

			if (font.flags & FT_PROPORTIONAL)
				fp = font.chars [letter];
			else
				fp = font.data + letter * BITS_TO_BYTES (width) * font.height;

			if (underline)
				for (i = 0; i < width; i++)
					videoBuffer [videoOffset++] = (uint32_t) CCanvas::Current ()->FontColor (0).index;
			else {
				fp += BITS_TO_BYTES (width) * r;
				mask = 0;
				for (i = 0; i < width; i++) {
					if (mask == 0) {
						bits = *fp++;
						mask = 0x80;
						}
					if (bits & mask)
						videoBuffer [videoOffset++] = (uint32_t) CCanvas::Current ()->FontColor (0).index;
					else
						videoOffset++;
					mask >>= 1;
					}
				}
			textP++;
			videoOffset += spacing-width;
			}
		videoOffset1 += rowSize;
		y++;
		}
	y += skip_lines;
	videoOffset1 += rowSize * skip_lines;
	skip_lines = 0;
	}
return 0;
}

#endif

//------------------------------------------------------------------------------

#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "args.h"
//font handling routines for OpenGL - Added 9/25/99 Matthew Mueller - they are here instead of in arch/ogl because they use all these defines

//------------------------------------------------------------------------------

char* CFont::PadString (char* pszDest, const char* pszText, const char* pszFiller, int32_t nLength)
{
int32_t sw, swf, sh, aw;
fontManager.Current ()->StringSize (pszText, sw, sh, aw);
fontManager.Current ()->StringSize (pszFiller, swf, sh, aw);
*pszDest = '\0';
if (sw + swf <= nLength) {
	int32_t i = (nLength - sw) / swf;
	if (i > 0) {
		int32_t l = sw + i * swf;
		if (l < nLength)
			PadString (pszDest, pszText, " ", nLength - l);
		if (!*pszDest);
			strcpy (pszDest, pszText);
		for (; i; i--)
			strcat (pszDest, pszFiller);
		}
	}
return pszDest;
}

//------------------------------------------------------------------------------

int32_t CFont::DrawString (int32_t left, int32_t top, const char *s)
{
	const char*		textP, * nextRowP, * text_ptr1;
	int32_t				width, spacing, letter;
	int32_t				x, y;
	int32_t				origColor = CCanvas::Current ()->FontColor (0).index; //to allow easy reseting to default string color with colored strings -MPM
	float				fScale = fontManager.Scale ();
	uint8_t				c;
	CBitmap*			bmf;
	CCanvasColor*	colorP = (m_info.flags & FT_COLOR) ? NULL : &CCanvas::Current ()->FontColor (0);
	
nextRowP = s;
y = top;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
	x = (left == 0x8000) ? fontManager.Current ()->GetCenteredX (textP) : left;
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			y += fontManager.Scaled (m_info.height + 2);
			break;
			}
		letter = c - m_info.minChar;
		fontManager.Current ()->GetCharWidth (c, textP [1], width, spacing);
		if (c <= 0x06) {	//not in font, draw as space
			textP = ScanEmbeddedColors (c, textP, origColor, 128, 2);
			continue;
			}
		if (fontManager.Current ()->InFont (letter)) {
			bmf = m_info.bitmaps + letter;
			bmf->AddFlags (BM_FLAG_TRANSPARENT);
			bmf->RenderScaled (x, y, int32_t (bmf->Width () * fScale), int32_t (bmf->Height () * fScale), I2X (1), 0, colorP, !gameStates.app.bDemoData);
			}
		x += spacing;
		textP++;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

static bool FillStringBitmap (CBitmap* bmP, const char *s, int32_t nKey, uint32_t nKeyColor, int32_t *nTabs, int32_t bCentered, int32_t nMaxWidth, int32_t bForce, int32_t& w, int32_t& h)
{

	int32_t		origColor = CCanvas::Current ()->FontColor (0).index;//to allow easy reseting to default string color with colored strings -MPM
	int32_t		x, y, cw, spacing, nTab, nChars, bHotKey;
	CBitmap*		bmfP;
	CRGBAColor	hc, kc;
	CPalette*	palP = NULL;
	CRGBColor*	colorP;
	uint8_t		c;
	const char*	textP, *text_ptr1, *nextRowP;
	int32_t		letter;
	CFont*		fontP = fontManager.Current ();

nextRowP = s;
y = 0;
nTab = 0;
nChars = 0;
while (nextRowP) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
#if 0
#	if DBG
	if (bCentered)
		x = (w - fontManager.Current ()->GetLineWidth (textP)) / 2;
	else
		x = 0;
#	else
	x = bCentered ? (w - fontManager.Current ()->GetLineWidth (textP)) / 2 : 0;
#	endif
#else
x = 0;
#endif
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			y += fontP->Height () + 2;
			nTab = 0;
			break;
			}
		if (c == '\t') {
			textP++;
			if (nTabs && (nTab < 6)) {
				int32_t	sw, sh, aw;

				fontManager.Current ()->StringSize (textP, sw, sh, aw);
				int32_t tx = LHX (nTabs [nTab++]);
				if (!gameStates.multi.bSurfingNet)
					x = nMaxWidth - sw;
				else if (x < tx) 
					x = tx;
				}
			continue;
			}
		letter = c - fontP->MinChar ();
		fontP->GetCharWidth (c, textP [1], cw, spacing);
		if (c <= 0x06) {	//not in font, draw as space
			textP = ScanEmbeddedColors (c, textP, origColor, 128, 2);
			continue;
			}
		if (!fontManager.Current ()->InFont (letter)) {
			x += spacing;
			textP++;
			continue;
			}
		if ((bHotKey = ((nKey < 0) && isalnum (c)) || (nKey && ((int32_t) c == nKey))))
			nKey = 0;
		bmfP = (bHotKey && (fontManager.Current () != SMALL_FONT)) ? SELECTED_FONT->Bitmaps () + letter : fontP->Bitmaps () + letter;
		palP = bmfP->Parent () ? bmfP->Parent ()->Palette () : bmfP->Palette ();
		int32_t transparencyColor = paletteManager.Texture ()->TransparentColor ();
		nChars++;
		kc.Red () = RGBA_RED (nKeyColor);
		kc.Green () = RGBA_GREEN (nKeyColor);
		kc.Blue () = RGBA_BLUE (nKeyColor);
		kc.Alpha () = 255;
		if (fontP->Flags () & FT_COLOR) {
			for (int32_t hy = 0; hy < bmfP->Height (); hy++) {
				CRGBAColor* pc = reinterpret_cast<CRGBAColor*> (bmP->Buffer ()) + (y + hy) * w + x;
				uint8_t* pf = bmfP->Buffer () + hy * bmfP->RowSize ();
				for (int32_t hx = bmfP->Width (); hx; hx--, pc++, pf++) {
#if 1 || DBG
					if ((pc - reinterpret_cast<CRGBAColor*> (bmP->Buffer ())) >= bmP->Width () * bmP->Height ())
						continue;
#endif
					if ((c = *pf) != transparencyColor) {
						colorP = palP->Color () + c;
						pc->Red () = colorP->Red () * 4;
						pc->Green () = colorP->Green () * 4;
						pc->Blue () = colorP->Blue () * 4;
						pc->Alpha () = 255;
						}
					}
				}
			}
		else {
			if (CCanvas::Current ()->FontColor (0).index < 0)
				memset (&hc, 0xff, sizeof (hc));
			else {
				if (CCanvas::Current ()->FontColor (0).rgb)
					hc = CCanvas::Current ()->FontColor (0);
				else {
					colorP = palP->Color () + CCanvas::Current ()->FontColor (0).index;
					hc.Red () = colorP->Red () * 4;
					hc.Green () = colorP->Green () * 4;
					hc.Blue () = colorP->Blue () * 4;
					}
				hc.Alpha () = 255;
				}
			for (int32_t hy = 0; hy < bmfP->Height (); hy++) {
				CRGBAColor* pc = reinterpret_cast<CRGBAColor*> (bmP->Buffer ()) + (y + hy) * w + x;
				uint8_t* pf = bmfP->Buffer () + hy * bmfP->RowSize ();
				for (int32_t hx = bmfP->Width (); hx; hx--, pc++, pf++) {
#if 1 || DBG
					if ((pc - reinterpret_cast<CRGBAColor*> (bmP->Buffer ())) >= bmP->Width () * bmP->Height ())
						continue;
#endif
					if (*pf != transparencyColor)
						*pc = bHotKey ? kc : hc;
					}
				}
			}
		x += spacing;
		textP++;
		}
	}
bmP->SetPalette (palP);
return true;
}

//------------------------------------------------------------------------------

extern bool bRegisterBitmaps;

CBitmap *CreateStringBitmap (const char *s, int32_t nKey, uint32_t nKeyColor, int32_t *nTabs, int32_t bCentered, int32_t nMaxWidth, int32_t nRowPad, int32_t bForce)
{
	CFont*		fontP = fontManager.Current ();
	float			fScale = fontManager.Scale ();
	CBitmap*		bmP;
	int32_t		w, h, aw;

#if 0
if (!(bForce || (gameOpts->menus.nStyle && gameOpts->menus.bFastMenus)))
	return NULL;
#endif
fontManager.SetScale (1.0f / (gameStates.app.bDemoData + 1));
int32_t nLineCount = fontP->StringSizeTabbed (s, w, h, aw, nTabs, nMaxWidth);
if (!(w && h)) {
	fontManager.SetScale (fScale);
	return NULL;
	}
h += nRowPad * nLineCount;

for (;;) {
	if (bForce) {
		w = Pow2ize (w, 65536);
		h = Pow2ize (h, 65536);	
		}
	bool b = bRegisterBitmaps;
	bRegisterBitmaps = false;
	bmP = CBitmap::Create (0, w, h, 4);
	bRegisterBitmaps = b;
	if (!bmP) {
		fontManager.SetScale (fScale);
		return NULL;
		}
	if (!bmP->Buffer ()) {
		fontManager.SetScale (fScale);
		delete bmP;
		return NULL;
		}
	bmP->SetName ("String Bitmap");
	bmP->Clear ();
	bmP->AddFlags (BM_FLAG_TRANSPARENT);
	if (FillStringBitmap (bmP, s, nKey, nKeyColor, nTabs, bCentered, nMaxWidth, bForce, w, h))
		break;
	bmP->Destroy ();
	}
bmP->SetTranspType (-1);
fontManager.SetScale (fScale);
return bmP;
}

//------------------------------------------------------------------------------

int32_t GrString (int32_t x, int32_t y, const char *s, int32_t *idP)
{
#if STRINGPOOL
	grsString	*ps;

if (/*(CCanvas::Current ()->Mode () == BM_OGL) &&*/ (ps = GetPoolString (s, idP))) {
	CBitmap* bmP = ps->bmP;
	float		fScale = fontManager.Scale ();
	int32_t		w = int32_t (bmP->Width () * fScale);
	int32_t		xs = gameData.X (x);

	ps->bmP->RenderScaled (xs, y, gameData.X (x + w) - xs, int32_t (bmP->Height () * fScale), I2X (1), 0, &CCanvas::Current ()->FontColor (0), !gameStates.app.bDemoData);
	return (int32_t) (ps - stringPool) + 1;
	}
#endif
	int32_t		w, h, aw, clipped = 0;

Assert (fontManager.Current () != NULL);
fontManager.Current ()->StringSize (s, w, h, aw);
bool bCentered = (x == 0x8000);
if (bCentered)
	x = (CCanvas::Current ()->Width () - w) / 2;
x = gameData.X (x);
if ((x < 0) || (y < 0))
	clipped |= 1;
if (x > CCanvas::Current ()->Width ())
	clipped |= 3;
else if ((x + w) > CCanvas::Current ()->Width ())
	clipped |= 1;
else if ((x + w) < 0)
	clipped |= 2;
if (y > CCanvas::Current ()->Height ())
	clipped |= 3;
else if ((y + h) > CCanvas::Current ()->Height ())
	clipped |= 1;
else if ((y + h) < 0)
	clipped |= 2;
if (!clipped)
	return fontManager.Current ()->DrawString (bCentered ? 0x8000 : x, y, s);
if (clipped & 2) {
	// Completely clipped...
	return 0;
	}
if (clipped & 1) {
	// Partially clipped...
	}
// Partially clipped...
#if 1
return fontManager.Current ()->DrawString (bCentered ? 0x8000 : x, y, s);
#else
if (MODE == BM_OGL)
	return fontManager.Current ()->DrawString (x, y, s);
if (fontManager.Current ()->Flags () & FT_COLOR)
	return fontManager.Current ()->DrawString (x, y, s);
if (CCanvas::Current ()->FontColor (1).index == -1)
	return GrInternalStringClippedM (gameData.X (x), y, s);
return GrInternalStringClipped (gameData.X (x), y, s);
#endif
}

//------------------------------------------------------------------------------

int32_t GrUString (int32_t x, int32_t y, const char *s)
{
#if 1
return fontManager.Current ()->DrawString (x, y, s);
#else
if (MODE == BM_OGL)
	return fontManager.Current ()->DrawString (x, y, s);
if (fontManager.Current ()->Flags () & FT_COLOR)
	return fontManager.Current ()->DrawString (x, y, s);
else if (MODE != BM_LINEAR)
	return 0;
if (CCanvas::Current ()->FontColor (1).index == -1)
	return GrInternalString0m (x, y, s);
return GrInternalString0 (x, y, s);
#endif
}

//------------------------------------------------------------------------------

int32_t _CDECL_ GrUPrintf (int32_t x, int32_t y, const char * format, ...)
{
	char buffer[1000];
	va_list args;

va_start (args, format);
vsprintf (buffer, format, args);
return GrUString (x, y, buffer);
}

//------------------------------------------------------------------------------

int32_t _CDECL_ GrPrintF (int32_t *idP, int32_t x, int32_t y, const char * format, ...)
{
	static char buffer [1000];
	va_list args;

va_start (args, format);
vsprintf (buffer, format, args);
return GrString (x, y, buffer, idP);
}

//------------------------------------------------------------------------------

#if 0

int32_t GrInternalStringClipped (int32_t x, int32_t y, const char *s)
{
	uint8_t * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int32_t r, mask, i, bits, width, spacing, letter, underline;
	int32_t x1 = x, last_x;
	tFont font;

fontManager.Current ()->GetInfo (font);
bits = 0;
nextRowP = s;
CCanvas::Current ()->FontColor (0).rgb = 0;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	x = x1;
	if (x == 0x8000)			//centered
		x = fontManager.Current ()->GetCenteredX (text_ptr1);
	last_x = x;

	for (r = 0; r < font.height; r++) {
		textP = text_ptr1;
		x = last_x;

		while (*textP) {
			if (*textP == '\n') {
				nextRowP = &textP[1];
				break;
				}

			if (*textP == CC_COLOR) {
				CCanvas::Current ()->FontColor (0).index = * (textP+1);
				textP += 2;
				continue;
				}

			if (*textP == CC_LSPACING) {
				Int3 ();	//	Warning: skip lines not supported for clipped strings.
				textP += 2;
				continue;
				}

			underline = 0;
			if (*textP == CC_UNDERLINE) {
				if ((r==font.baseLine+2) || (r==font.baseLine+3))
					underline = 1;
				textP++;
				}

			fontManager.Current ()->GetCharWidth (textP [0], textP [1], width, spacing);
			letter = *textP - font.minChar;
			if (!fontManager.Current ()->InFont (letter)) {	//not in font, draw as space
				x += spacing;
				textP++;
				continue;
				}

			if (font.flags & FT_PROPORTIONAL)
				fp = font.chars [letter];
			else
				fp = font.data + letter * BITS_TO_BYTES (width)*font.height;

			if (underline) {
				for (i = 0; i < width; i++) {
					CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
					DrawPixelClipped (x++, y);
					}
				} 
			else {
				fp += BITS_TO_BYTES (width)*r;
				mask = 0;
				for (i = 0; i < width; i++) {
					if (mask == 0) {
						bits = *fp++;
						mask = 0x80;
						}
					if (bits & mask)
						CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
					else
						CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (1).index);
					DrawPixelClipped (x++, y);
					mask >>= 1;
					}
				}
			x += spacing-width;		//for kerning
			textP++;
			}
		y++;
		}
	}
font.parentBitmap.SetBuffer (NULL);	//beware of the destructor!
return 0;
}

#endif

//------------------------------------------------------------------------------

#if 0

int32_t GrInternalStringClippedM (int32_t x, int32_t y, const char *s)
{
	uint8_t * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int32_t r, mask, i, bits, width, spacing, letter, underline;
	int32_t x1 = x, last_x;
	tFont font;

fontManager.Current ()->GetInfo (font);
bits = 0;
nextRowP = s;
CCanvas::Current ()->FontColor (0).rgb = 0;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	x = x1;
	if (x==0x8000)			//centered
		x = fontManager.Current ()->GetCenteredX (text_ptr1);
	last_x = x;
	for (r = 0; r < font.height; r++) {
		x = last_x;
		textP = text_ptr1;
		while (*textP) {
			if (*textP == '\n') {
				nextRowP = &textP[1];
				break;
				}
	
			if (*textP == CC_COLOR) {
				CCanvas::Current ()->FontColor (0).index = * (textP+1);
				textP += 2;
				continue;
				}

			if (*textP == CC_LSPACING) {
				Int3 ();	//	Warning: skip lines not supported for clipped strings.
				textP += 2;
				continue;
				}

			underline = 0;
			if (*textP == CC_UNDERLINE) {
				if ((r == font.baseLine + 2) || (r == font.baseLine + 3))
					underline = 1;
				textP++;
				}
			fontManager.Current ()->GetCharWidth (textP[0], textP[1], width, spacing);
			letter = *textP-font.minChar;
			if (!fontManager.Current ()->InFont (letter)) {	//not in font, draw as space
				x += spacing;
				textP++;
				continue;
				}

			if (font.flags & FT_PROPORTIONAL)
				fp = font.chars[letter];
			else
				fp = font.data + letter * BITS_TO_BYTES (width)*font.height;

			if (underline) {
				for (i = 0; i < width; i++) {
					CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
					DrawPixelClipped (x++, y);
					}
				}
			else {
				fp += BITS_TO_BYTES (width)*r;
				mask = 0;
				for (i = 0; i< width; i++) {
					if (mask==0) {
						bits = *fp++;
						mask = 0x80;
						}
					if (bits & mask) {
						CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
						DrawPixelClipped (x++, y);
						} 
					else {
						x++;
						}
					mask >>= 1;
					}
				}
			x += spacing-width;		//for kerning
			textP++;
			}
		y++;
		}
	}
font.parentBitmap.SetBuffer (NULL);	//beware of the destructor!
return 0;
}

#endif

//------------------------------------------------------------------------------
// Returns the length of the first 'n' characters of a string.
int32_t StringWidth (char * s, int32_t n)
{
	int32_t w, h, aw;
	char p = s [n];

if (n)
	s [n] = 0;
fontManager.Current ()->StringSize (s, w, h, aw);
if (n)
	s [n] = p;
return w;
}

//------------------------------------------------------------------------------

int32_t CenteredStringPos (char* s)
{
return (CCanvas::Current ()->Width () - StringWidth (s)) / 2;
}

//------------------------------------------------------------------------------
// Draw string 's' centered on a canvas... if wider than
// canvas, then wrap it.
void DrawCenteredText (int32_t y, char * s)
{
	char	p;
	int32_t	i, l = (int32_t) strlen (s);

if (StringWidth (s, l) < CCanvas::Current ()->Width ()) {
	GrString (0x8000, y, s);
	return;
	}
int32_t w = CCanvas::Current ()->Width () - 16;
int32_t h = CCanvas::Current ()->Font ()->Height () + 1;
for (i = 0; i < l; i++) {
	if (StringWidth (s, i) > w) {
		p = s [i];
		s [i] = 0;
		GrString (0x8000, y, s);
		s [i] = p;
		GrString (0x8000, y + h, &s [i]);
		return;
		}
	}
}

//------------------------------------------------------------------------------

