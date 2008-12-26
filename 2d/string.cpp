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

#include "inferno.h"
#include "text.h"
#include "font.h"
#include "grdef.h"
#include "error.h"

#include "cfile.h"
#include "mono.h"
#include "byteswap.h"
#include "bitmap.h"
#include "gamefont.h"

#include "makesig.h"

#define LHX(x)	 (gameStates.menus.bHires ? 2 * (x) : x)

int GrInternalStringClipped (int x, int y, const char *s);
int GrInternalStringClippedM (int x, int y, const char *s);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define GRS_MAX_STRINGS	1000

grsString stringPool [GRS_MAX_STRINGS];
short nPoolStrings = 0;

//------------------------------------------------------------------------------

void InitStringPool (void)
{
nPoolStrings = 0;
memset (stringPool, 0, sizeof (stringPool));
}

//------------------------------------------------------------------------------

void FreeStringPool (void)
{
	int			i;
	grsString	*ps;

PrintLog ("unloading string pool\n");
for (i = nPoolStrings, ps = stringPool; i; i--, ps++) {
	delete [] ps->pszText;
	ps->pszText = NULL;
	if (ps->pId)
		*ps->pId = 0;
	ps->bmP->ReleaseTexture ();
	delete ps->bmP;
	ps->bmP = NULL;
	}
PrintLog ("initializing string pool\n");
InitStringPool ();
}

//------------------------------------------------------------------------------

grsString *CreatePoolString (const char *s, int *idP)
{
	grsString	*ps;
	int			l, w, h, aw;

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
if (!(ps->bmP = CreateStringBitmap (s, 0, 0, 0, 0, w, 1))) {
	*idP = 0;
	return NULL;
	}
l = (int) strlen (s) + 1;
if (ps->pszText && (ps->nLength < l))
	delete [] ps->pszText;
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

grsString *GetPoolString (const char *s, int *idP)
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
int grMsgColorLevel = 1;

//------------------------------------------------------------------------------

int GrInternalString0 (int x, int y, const char *s)
{
	ubyte*		fp;
	const char*	textP, *nextRowP, *text_ptr1;
	int			r, mask, i, bits, width, spacing, letter, underline;
	int			skip_lines = 0;
	uint			videoOffset, videoOffset1;
	CPalette*	palette = paletteManager.Game ();
	ubyte*		videoBuffer = CCanvas::Current ()->Buffer ();
	int			rowSize = CCanvas::Current ()->RowSize ();
	tFont			font;
	char			c;
	
if (CCanvas::Current ()->FontColor (0).rgb) {
	CCanvas::Current ()->FontColor (0).rgb = 0;
	CCanvas::Current ()->FontColor (0).index = palette->ClosestColor (CCanvas::Current ()->FontColor (0).color.red, 
																							CCanvas::Current ()->FontColor (0).color.green, 
																							CCanvas::Current ()->FontColor (0).color.blue);
	}
if (CCanvas::Current ()->FontColor (1).rgb) {
	CCanvas::Current ()->FontColor (1).rgb = 0;
	CCanvas::Current ()->FontColor (1).index = palette->ClosestColor (CCanvas::Current ()->FontColor (1).color.red, 
																							CCanvas::Current ()->FontColor (1).color.green, 
																							CCanvas::Current ()->FontColor (1).color.blue);
	}
bits=0;
videoOffset1 = y * rowSize + x;
nextRowP = s;
fontManager.Current ()->GetInfo (font);
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	if (x == 0x8000) {			//centered
		int xx = fontManager.Current ()->GetCenteredX (text_ptr1);
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
					videoBuffer [videoOffset++] = (ubyte) CCanvas::Current ()->FontColor (0).index;
				else {
					fp += BITS_TO_BYTES (width)*r;
					mask = 0;
					for (i = 0; i < width; i++) {
						if (mask == 0) {
							bits = *fp++;
							mask = 0x80;
							}
						if (bits & mask)
							videoBuffer [videoOffset++] = (ubyte) CCanvas::Current ()->FontColor (0).index;
						else
							videoBuffer [videoOffset++] = (ubyte) CCanvas::Current ()->FontColor (1).index;
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

//------------------------------------------------------------------------------

static inline const char* ScanEmbeddedColors (char c, const char* textP, int origColor, int nOffset, int nScale)
{
if ((c >= 1) && (c <= 3)) {
	if (textP [1]) {
		if (grMsgColorLevel >= c) {
			CCanvas::Current ()->FontColor (0).rgb = 1;
			CCanvas::Current ()->FontColor (0).color.red = (textP [0] - nOffset) * nScale;
			CCanvas::Current ()->FontColor (0).color.green = (textP [1] - nOffset) * nScale;
			CCanvas::Current ()->FontColor (0).color.blue = (textP [2] - nOffset) * nScale;
			CCanvas::Current ()->FontColor (0).color.alpha = 0;
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
return textP;
}

//------------------------------------------------------------------------------

int GrInternalString0m (int x, int y, const char *s)
{
	ubyte*			fp;
	const char*		textP, * nextRowP, * text_ptr1;
	int				r, mask, i, bits, width, spacing, letter, underline;
	int				skip_lines = 0;
	char				c;
	int				origColor;
	uint				videoOffset, videoOffset1;
	CPalette*		palette = paletteManager.Game ();
	ubyte*			videoBuffer = CCanvas::Current ()->Buffer ();
	int				rowSize = CCanvas::Current ()->RowSize ();
	tFont				font;

if (CCanvas::Current ()->FontColor (0).rgb) {
	CCanvas::Current ()->FontColor (0).rgb = 0;
	CCanvas::Current ()->FontColor (0).index = fontManager.Current ()->ParentBitmap ().Palette ()->ClosestColor (CCanvas::Current ()->FontColor (0).color.red, CCanvas::Current ()->FontColor (0).color.green, CCanvas::Current ()->FontColor (0).color.blue);
	}
if (CCanvas::Current ()->FontColor (1).rgb) {
	CCanvas::Current ()->FontColor (1).rgb = 0;
	CCanvas::Current ()->FontColor (1).index = fontManager.Current ()->ParentBitmap ().Palette ()->ClosestColor (CCanvas::Current ()->FontColor (1).color.red, CCanvas::Current ()->FontColor (1).color.green, CCanvas::Current ()->FontColor (1).color.blue);
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
		int xx = fontManager.Current ()->GetCenteredX (text_ptr1);
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
				fp = font.chars[letter];
			else
				fp = font.data + letter * BITS_TO_BYTES (width) * font.height;

			if (underline)
				for (i = 0; i < width; i++)
					videoBuffer [videoOffset++] = (uint) CCanvas::Current ()->FontColor (0).index;
			else {
				fp += BITS_TO_BYTES (width) * r;
				mask = 0;
				for (i = 0; i < width; i++) {
					if (mask == 0) {
						bits = *fp++;
						mask = 0x80;
						}
					if (bits & mask)
						videoBuffer [videoOffset++] = (uint) CCanvas::Current ()->FontColor (0).index;
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

//------------------------------------------------------------------------------

#include "ogl_defs.h"
#include "ogl_bitmap.h"
#include "args.h"
//font handling routines for OpenGL - Added 9/25/99 Matthew Mueller - they are here instead of in arch/ogl because they use all these defines

int Pow2ize (int x);//from ogl.c

//------------------------------------------------------------------------------

int CFont::DrawString (int x, int y, const char *s)
{
	const char*		textP, * nextRowP, * text_ptr1;
	int				width, spacing, letter;
	int				l, t;
	int				origColor = CCanvas::Current ()->FontColor (0).index;//to allow easy reseting to default string color with colored strings -MPM
	ubyte				c;
	CBitmap*			bmf;
	tCanvasColor*	colorP = (m_info.flags & FT_COLOR) ? NULL : &CCanvas::Current ()->FontColor (0);
	bool				bOGL = CCanvas::Current ()->Mode () == BM_OGL;

nextRowP = s;
t = y;
if (screen.Canvas ()->Mode () != BM_OGL)
	Error ("carp.\n");
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
	l = (x == 0x8000) ? fontManager.Current ()->GetCenteredX (textP) : x;
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			t += m_info.height + 2;
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
			if (bOGL)
				OglUBitMapMC (l, t, 0, 0, bmf, colorP, F1_0, 0);
			else
				GrBitmapM (l, t, bmf, 2); // credits need clipping
			}
		l += spacing;
		textP++;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

CBitmap *CreateStringBitmap (
	const char *s, int nKey, uint nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce)
{
	int			origColor = CCanvas::Current ()->FontColor (0).index;//to allow easy reseting to default string color with colored strings -MPM
	int			i, x, y, hx, hy, w, h, aw, cw, spacing, nTab, nChars, bHotKey;
	CBitmap		*bmP, *bmfP;
	tRgbaColorb	hc, kc, *pc;
	ubyte			*pf;
	CPalette		*palP = NULL;
	tRgbColorb	*colorP;
	ubyte			c;
	const char	*textP, *text_ptr1, *nextRowP;
	int			letter;
	CFont*		fontP = fontManager.Current ();

if (!(bForce || (gameOpts->menus.nStyle && gameOpts->menus.bFastMenus)))
	return NULL;
fontP->StringSizeTabbed (s, w, h, aw, nTabs, nMaxWidth);
if (!(w && h))
	return NULL;
if (bForce >= 0) {
	for (i = 1; i < w; i <<= 2)
		;
	w = i;
	for (i = 1; i < h; i <<= 2)
		;
	h = i;
	}
if (!(bmP = CBitmap::Create (0, w, h, 4))) 
	return NULL;
if (!bmP->Buffer ()) {
	delete bmP;
	return NULL;
	}
bmP->SetName ("String Bitmap");
bmP->Clear ();
bmP->AddFlags (BM_FLAG_TRANSPARENT);
nextRowP = s;
y = 0;
nTab = 0;
nChars = 0;
while (nextRowP) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
#if DBG
	if (bCentered)
		x = (w - fontManager.Current ()->GetLineWidth (textP)) / 2;
	else
		x = 0;
#else
	x = bCentered ? (w - GetLineWidth (textP)) / 2 : 0;
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
				int	w, h, aw;

				fontManager.Current ()->StringSize (textP, w, h, aw);
				x = LHX (nTabs [nTab++]);
				if (!gameStates.multi.bSurfingNet)
					x += nMaxWidth - w;
				for (i = 1; i < w; i <<= 2)
					;
				w = i;
				for (i = 1; i < h; i <<= 2)
					;
				h = i;
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
			}
		if ((bHotKey = ((nKey < 0) && isalnum (c)) || (nKey && ((int) c == nKey))))
			nKey = 0;
		bmfP = (bHotKey && (fontManager.Current () != SMALL_FONT)) ? SELECTED_FONT->Bitmaps () + letter : fontP->Bitmaps () + letter;
		palP = bmfP->Parent () ? bmfP->Parent ()->Palette () : bmfP->Palette ();
		nChars++;
		i = nKeyColor * 3;
		kc.red = RGBA_RED (nKeyColor);
		kc.green = RGBA_GREEN (nKeyColor);
		kc.blue = RGBA_BLUE (nKeyColor);
		kc.alpha = 255;
		if (fontP->Flags () & FT_COLOR) {
			for (hy = 0; hy < bmfP->Height (); hy++) {
				pc = reinterpret_cast<tRgbaColorb*> (bmP->Buffer ()) + (y + hy) * w + x;
				pf = bmfP->Buffer () + hy * bmfP->RowSize ();
				for (hx = bmfP->Width (); hx; hx--, pc++, pf++)
					if ((c = *pf) != TRANSPARENCY_COLOR) {
						colorP = palP->Color () + c;
						pc->red = colorP->red * 4;
						pc->green = colorP->green * 4;
						pc->blue = colorP->blue * 4;
						pc->alpha = 255;
						}
#if DBG
					else
						c = c;
#endif
				}
			}
		else {
			if (CCanvas::Current ()->FontColor (0).index < 0)
				memset (&hc, 0xff, sizeof (hc));
			else {
				if (CCanvas::Current ()->FontColor (0).rgb) {
					hc = CCanvas::Current ()->FontColor (0).color;
					}
				else {
					colorP = palP->Color () + CCanvas::Current ()->FontColor (0).index;
					hc.red = colorP->red * 4;
					hc.green = colorP->green * 4;
					hc.blue = colorP->blue * 4;
					hc.alpha = 255;
					}
				}
			for (hy = 0; hy < bmfP->Height (); hy++) {
				pc = reinterpret_cast<tRgbaColorb*> (bmP->Buffer ()) + (y + hy) * w + x;
				pf = bmfP->Buffer () + hy * bmfP->RowSize ();
				for (hx = bmfP->Width (); hx; hx--, pc++, pf++)
					if (*pf != TRANSPARENCY_COLOR)
						*pc = bHotKey ? kc : hc;
				}
			}
		x += spacing;
		textP++;
		}
	}
bmP->SetPalette (palP);
return bmP;
}

//------------------------------------------------------------------------------

int OglUBitMapMC2 (int x, int y, int dw, int dh, CBitmap *bmP, tCanvasColor *c, int scale, int orient);

int GrString (int x, int y, const char *s, int *idP)
{
if (gameOpts->render.coronas.nStyle < 2) {
		grsString	*ps;

	if ((MODE == BM_OGL) && (ps = GetPoolString (s, idP))) {
		OglUBitMapMC (x, y, 0, 0, ps->bmP, &CCanvas::Current ()->FontColor (0), F1_0, 0);
		return (int) (ps - stringPool) + 1;
		}
	}
#if 0

return fontManager.Current ()->DrawString (x, y, s);

#else

	int			w, h, aw, clipped = 0;

Assert (fontManager.Current () != NULL);
if (x == 0x8000)	{
	if (y < 0)
		clipped |= 1;
	fontManager.Current ()->StringSize (s, w, h, aw);
	// for x, since this will be centered, only look at
	// width.
	if (w > CCanvas::Current ()->Width ())
		clipped |= 1;
	if (y > CCanvas::Current ()->Height ())
		clipped |= 3;
	else if ((y + h) > CCanvas::Current ()->Height ())
		clipped |= 1;
	else if ((y + h) < 0)
		clipped |= 2;
	}
else {
	if ((x < 0) || (y < 0))
		clipped |= 1;
	fontManager.Current ()->StringSize (s, w, h, aw);
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
	}
if (!clipped)
	return fontManager.Current ()->DrawString (x, y, s);
if (clipped & 2) {
	// Completely clipped...
	return 0;
	}
if (clipped & 1) {
	// Partially clipped...
	}
// Partially clipped...
if (MODE == BM_OGL)
	return fontManager.Current ()->DrawString (x, y, s);
if (fontManager.Current ()->Flags () & FT_COLOR)
	return fontManager.Current ()->DrawString (x, y, s);
if (CCanvas::Current ()->FontColor (1).index == -1)
	return GrInternalStringClippedM (x, y, s);
return GrInternalStringClipped (x, y, s);

#endif

}

//------------------------------------------------------------------------------

int GrUString (int x, int y, const char *s)
{
if (MODE == BM_OGL)
	return fontManager.Current ()->DrawString (x, y, s);
if (fontManager.Current ()->Flags () & FT_COLOR)
	return fontManager.Current ()->DrawString (x, y, s);
else if (MODE != BM_LINEAR)
	return 0;
if (CCanvas::Current ()->FontColor (1).index == -1)
	return GrInternalString0m (x, y, s);
return GrInternalString0 (x, y, s);
}

//------------------------------------------------------------------------------

int _CDECL_ GrUPrintf (int x, int y, const char * format, ...)
{
	char buffer[1000];
	va_list args;

va_start (args, format);
vsprintf (buffer, format, args);
return GrUString (x, y, buffer);
}

//------------------------------------------------------------------------------

int _CDECL_ GrPrintF (int *idP, int x, int y, const char * format, ...)
{
	static char buffer [1000];
	va_list args;

va_start (args, format);
vsprintf (buffer, format, args);
return GrString (x, y, buffer, idP);
}

//------------------------------------------------------------------------------

int GrInternalStringClipped (int x, int y, const char *s)
{
	ubyte * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int r, mask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;
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

	for (r = 0; r < font.height; r++)	{
		textP = text_ptr1;
		x = last_x;

		while (*textP)	{
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
			if (*textP == CC_UNDERLINE)	{
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

			if (underline)	{
				for (i = 0; i < width; i++)	{
					CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
					gr_pixel (x++, y);
					}
				} 
			else {
				fp += BITS_TO_BYTES (width)*r;
				mask = 0;
				for (i = 0; i < width; i++)	{
					if (mask == 0) {
						bits = *fp++;
						mask = 0x80;
						}
					if (bits & mask)
						CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
					else
						CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (1).index);
					gr_pixel (x++, y);
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

//------------------------------------------------------------------------------

int GrInternalStringClippedM (int x, int y, const char *s)
{
	ubyte * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int r, mask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;
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
	for (r = 0; r < font.height; r++)	{
		x = last_x;
		textP = text_ptr1;
		while (*textP)	{
			if (*textP == '\n')	{
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

			if (underline)	{
				for (i = 0; i < width; i++) {
					CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
					gr_pixel (x++, y);
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
					if (bits & mask)	{
						CCanvas::Current ()->SetColor (CCanvas::Current ()->FontColor (0).index);
						gr_pixel (x++, y);
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

//------------------------------------------------------------------------------

