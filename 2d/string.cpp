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
	delete[] ps->pszText;
	ps->pszText = NULL;
	if (ps->pId)
		*ps->pId = 0;
	ps->bmP->FreeTexture ();
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
	ps->bmP->FreeTexture ();
	delete ps->bmP;
	ps->bmP = NULL;
	}
else {
	if (nPoolStrings >= GRS_MAX_STRINGS)
		return NULL;
	ps = stringPool + nPoolStrings;
	}
FONT->StringSize (s, w, h, aw);
if (!(ps->bmP = CreateStringBitmap (s, 0, 0, 0, 0, w, 1))) {
	*idP = 0;
	return NULL;
	}
l = (int) strlen (s) + 1;
if (ps->pszText && (ps->nLength < l))
	delete[] ps->pszText;
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
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color nType thing would be better, but that would be way too much trouble for little gain
int grMsgColorLevel = 1;

inline char *CheckEmbeddedColors (char *textP, char c, int orig_color)
{
if ((c >= 1) && (c <= 3)) {
	if (*++textP) {
		if (grMsgColorLevel >= c) {
			FG_COLOR.rgb = 1;
			FG_COLOR.color.red = textP [0];
			FG_COLOR.color.green = textP [1];
			FG_COLOR.color.blue = textP [2];
			FG_COLOR.color.alpha = 0;
			}
		textP += 3;
		}
	}
else if ((c >= 4) && (c <= 6)) {
	if (grMsgColorLevel >= *textP - 3) {
		FG_COLOR.index = orig_color;
		FG_COLOR.rgb = 0;
		}
	textP++;
	}
return textP;
}

#define CHECK_EMBEDDED_COLORS () \
	if ((c >= 1) && (c <= 3)) { \
		if (*++textP) { \
			if (grMsgColorLevel >= c) { \
				FG_COLOR.rgb = 1; \
				FG_COLOR.color.red = textP [0] - 128; \
				FG_COLOR.color.green = textP [1] - 128; \
				FG_COLOR.color.blue = textP [2] - 128; \
				FG_COLOR.color.alpha = 0; \
				} \
			textP += 3; \
			} \
		} \
	else if ((c >= 4) && (c <= 6)) { \
		if (grMsgColorLevel >= *textP - 3) { \
			FG_COLOR.index = orig_color; \
			FG_COLOR.rgb = 0; \
			} \
		textP++; \
		}

//------------------------------------------------------------------------------

int GrInternalString0 (int x, int y, const char *s)
{
	ubyte * fp;
	const char *textP, *nextRowP, *text_ptr1;
	int r, mask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;
	uint videoOffset, videoOffset1;
	CPalette *palette = paletteManager.Game ();
	ubyte* videoBuffer = CCanvas::Current ()->Buffer ();
	int rowSize = CCanvas::Current ()->RowSize ();
	tFont	font;
	
if (FG_COLOR.rgb) {
	FG_COLOR.rgb = 0;
	FG_COLOR.index = palette->ClosestColor (
		FG_COLOR.color.red, FG_COLOR.color.green, FG_COLOR.color.blue);
	}
if (BG_COLOR.rgb) {
	BG_COLOR.rgb = 0;
	BG_COLOR.index = palette->ClosestColor (
			BG_COLOR.color.red, BG_COLOR.color.green, BG_COLOR.color.blue);
	}
bits=0;
videoOffset1 = y * rowSize + x;
nextRowP = s;
FONT->GetInfo (font);
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	if (x == 0x8000) {			//centered
		int xx = FONT->GetCenteredX (text_ptr1);
		videoOffset1 = y * rowSize + xx;
		}
	for (r = 0; r < font.height; r++) {
		textP = text_ptr1;
		videoOffset = videoOffset1;
		while (*textP) {
			if (*textP == '\n') {
				nextRowP = &textP[1];
				break;
				}
			if (*textP == CC_COLOR) {
				FG_COLOR.index = *(textP+1);
				FG_COLOR.rgb = 0;
				textP += 2;
				continue;
				}
			if (*textP == CC_LSPACING) {
				skip_lines = * (textP+1) - '0';
				textP += 2;
				continue;
				}
			underline = 0;
			if (*textP == CC_UNDERLINE) {
				if ((r == font.baseLine + 2) || (r == font.baseLine + 3))
					underline = 1;
				textP++;
				}
			FONT->GetCharWidth (textP[0], textP[1], width, spacing);
			letter = *textP - font.minChar;
			if (!FONT->InFont (letter)) {	//not in font, draw as space
				videoOffset += spacing;
				textP++;
				continue;
				}
			if (font.flags & FT_PROPORTIONAL)
				fp = font.chars [letter];
			else
				fp = font.data + letter * BITS_TO_BYTES (width)*font.height;
			if (underline)
				for (i = 0; i < width; i++)
					videoBuffer[videoOffset++] = (ubyte) FG_COLOR.index;
				else {
					fp += BITS_TO_BYTES (width)*r;
					mask = 0;
					for (i = 0; i < width; i++) {
						if (mask == 0) {
							bits = *fp++;
							mask = 0x80;
							}
						if (bits & mask)
							videoBuffer[videoOffset++] = (ubyte) FG_COLOR.index;
						else
							videoBuffer[videoOffset++] = (ubyte) BG_COLOR.index;
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

int GrInternalString0m (int x, int y, const char *s)
{
	ubyte	* fp;
	const char		* textP, * nextRowP, * text_ptr1;
	int				r, mask, i, bits, width, spacing, letter, underline;
	int				skip_lines = 0;
	char				c;
	int				orig_color;
	uint	videoOffset, videoOffset1;
	CPalette			*palette = paletteManager.Game ();
	ubyte*			videoBuffer = CCanvas::Current ()->Buffer ();
	int				rowSize = CCanvas::Current ()->RowSize ();
	tFont				font;

if (FG_COLOR.rgb) {
	FG_COLOR.rgb = 0;
	FG_COLOR.index = FONT->ParentBitmap ().Palette ()->ClosestColor (FG_COLOR.color.red, FG_COLOR.color.green, FG_COLOR.color.blue);
	}
if (BG_COLOR.rgb) {
	BG_COLOR.rgb = 0;
	BG_COLOR.index = FONT->ParentBitmap ().Palette ()->ClosestColor (BG_COLOR.color.red, BG_COLOR.color.green, BG_COLOR.color.blue);
	}
orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
bits=0;
videoOffset1 = y * rowSize + x;
FONT->GetInfo (font);
	nextRowP = s;
	FG_COLOR.rgb = 0;
	while (nextRowP != NULL)
	{
		text_ptr1 = nextRowP;
		nextRowP = NULL;

		if (x==0x8000) {			//centered
			int xx = FONT->GetCenteredX (text_ptr1);
			videoOffset1 = y * rowSize + xx;
		}

		for (r=0; r<font.height; r++) {
			textP = text_ptr1;
			videoOffset = videoOffset1;
			while ((c = *textP)) {
				if (c == '\n') {
					nextRowP = &textP[1];
					break;
					}
				if (c == CC_COLOR) {
					FG_COLOR.index = * (++textP);
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
				FONT->GetCharWidth (c, textP[1], width, spacing);
				letter = c - font.minChar;
				if (!FONT->InFont (letter) || c <= 0x06) {	//not in font, draw as space
#if 0
					CHECK_EMBEDDED_COLORS ()
#else
					if ((c >= 1) && (c <= 3)) {
						if (*++textP) {
							if (grMsgColorLevel >= c) {
								FG_COLOR.rgb = 1;
								FG_COLOR.color.red = textP [0] - 128;
								FG_COLOR.color.green = textP [1] - 128;
								FG_COLOR.color.blue = textP [2] - 128;
								FG_COLOR.color.alpha = 0;
								}
							textP += 3;
							}
						}
					else if ((c >= 4) && (c <= 6)) {
						if (grMsgColorLevel >= *textP - 3) {
							FG_COLOR.index = orig_color;
							FG_COLOR.rgb = 0;
							}
						textP++;
						}
#endif
					else {
						videoOffset += spacing;
						textP++;
						}
					continue;
					}
				if (font.flags & FT_PROPORTIONAL)
					fp = font.chars[letter];
				else
					fp = font.data + letter * BITS_TO_BYTES (width) * font.height;

				if (underline)
					for (i=0; i< width; i++)
						videoBuffer[videoOffset++] = (uint) FG_COLOR.index;
				else {
					fp += BITS_TO_BYTES (width)*r;
					mask = 0;
					for (i=0; i< width; i++) {
						if (mask==0) {
							bits = *fp++;
							mask = 0x80;
						}
						if (bits & mask)
							videoBuffer[videoOffset++] = (uint) FG_COLOR.index;
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

int OglInternalString (int x, int y, const char *s)
{
	const char* textP, * nextRowP, * text_ptr1;
	int			width, spacing, letter;
	int			xx, yy;
	int			orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
	ubyte			c;
	CBitmap		*bmf;
	tFont			font;

FONT->GetInfo (font);
nextRowP = s;
yy = y;
if (screen.Canvas ()->Mode () != BM_OGL)
	Error ("carp.\n");
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
	xx = x;
	if (xx == 0x8000)			//centered
		xx = FONT->GetCenteredX (textP);
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			yy += font.height + 2;
			break;
			}
		letter = c - font.minChar;
		FONT->GetCharWidth (c, textP [1], width, spacing);
		if (!FONT->InFont (letter) || (c <= 0x06)) {	//not in font, draw as space
#if 0
			CHECK_EMBEDDED_COLORS ()
#else
			if ((c >= 1) && (c <= 3)) {
				if (*++textP) {
					if (grMsgColorLevel >= c) {
						FG_COLOR.rgb = 1;
						FG_COLOR.color.red = 2 * (textP [0] - 128);
						FG_COLOR.color.green = 2 * (textP [1] - 128);
						FG_COLOR.color.blue = 2 * (textP [2] - 128);
						FG_COLOR.color.alpha = 255;
						}
					textP += 3;
					}
				}
			else if ((c >= 4) && (c <= 6)) {
				if (grMsgColorLevel >= *textP - 3) {
					FG_COLOR.index = orig_color;
					FG_COLOR.rgb = 0;
					}
				textP++;
				}
#endif
			else {
				xx += spacing;
				textP++;
				}
			continue;
			}
		bmf = font.bitmaps + letter;
		bmf->AddFlags (BM_FLAG_TRANSPARENT);
		if (font.flags & FT_COLOR)
			GrBitmapM (xx, yy, bmf, 2); // credits need clipping
		else {
			OglUBitMapMC (xx, yy, 0, 0, bmf, &FG_COLOR, F1_0, 0);
			}
		xx += spacing;
		textP++;
		}
	}
font.parentBitmap.SetBuffer (NULL);	//beware of the destructor!
memset (&font, 0, sizeof (font));
return 0;
}

//------------------------------------------------------------------------------

CBitmap *CreateStringBitmap (
	const char *s, int nKey, uint nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce)
{
	int			orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
	int			i, x, y, hx, hy, w, h, aw, cw, spacing, nTab, nChars, bHotKey;
	CBitmap		*bmP, *bmfP;
	tRgbaColorb	hc, kc, *pc;
	ubyte			*pf;
	CPalette		*palP = NULL;
	tRgbColorb	*colorP;
	ubyte			c;
	const char	*textP, *text_ptr1, *nextRowP;
	int			letter;
	tFont			font;

if (!(bForce || (gameOpts->menus.nStyle && gameOpts->menus.bFastMenus)))
	return NULL;
FONT->GetInfo (font);
FONT->StringSizeTabbed (s, w, h, aw, nTabs, nMaxWidth);
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
		x = (w - FONT->GetLineWidth (textP)) / 2;
	else
		x = 0;
#else
	x = bCentered ? (w - GetLineWidth (textP)) / 2 : 0;
#endif
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			y += font.height + 2;
			nTab = 0;
			break;
			}
		if (c == '\t') {
			textP++;
			if (nTabs && (nTab < 6)) {
				int	w, h, aw;

				FONT->StringSize (textP, w, h, aw);
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
		letter = c - font.minChar;
		FONT->GetCharWidth (c, textP [1], cw, spacing);
		if (!FONT->InFont (letter) || (c <= 0x06)) {	//not in font, draw as space
#if 0
			CHECK_EMBEDDED_COLORS ()
#else
			if ((c >= 1) && (c <= 3)) {
				if (*++textP) {
					if (grMsgColorLevel >= c) {
						FG_COLOR.rgb = 1;
						FG_COLOR.color.red = (textP [0] - 128) * 2;
						FG_COLOR.color.green = (textP [1] - 128) * 2;
						FG_COLOR.color.blue = (textP [2] - 128) * 2;
						FG_COLOR.color.alpha = 255;
						}
					textP += 3;
					}
				}
			else if ((c >= 4) && (c <= 6)) {
				if (grMsgColorLevel >= *textP - 3) {
					FG_COLOR.index = orig_color;
					FG_COLOR.rgb = 0;
					}
				textP++;
				}
#endif
			else {
				x += spacing;
				textP++;
				}
			continue;
			}
		if ((bHotKey = ((nKey < 0) && isalnum (c)) || (nKey && ((int) c == nKey))))
			nKey = 0;
		bmfP = (bHotKey && (FONT != SMALL_FONT)) ? SELECTED_FONT->Bitmaps () + letter : font.bitmaps + letter;
		palP = bmfP->Parent () ? bmfP->Parent ()->Palette () : bmfP->Palette ();
		nChars++;
		i = nKeyColor * 3;
		kc.red = RGBA_RED (nKeyColor);
		kc.green = RGBA_GREEN (nKeyColor);
		kc.blue = RGBA_BLUE (nKeyColor);
		kc.alpha = 255;
		if (font.flags & FT_COLOR) {
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
			if (FG_COLOR.index < 0)
				memset (&hc, 0xff, sizeof (hc));
			else {
				if (FG_COLOR.rgb) {
					hc = FG_COLOR.color;
					}
				else {
					colorP = palP->Color () + FG_COLOR.index;
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
//bmP->SetupTexture (0, 2, 1);
font.parentBitmap.SetBuffer (NULL);	//beware of the destructor!
return bmP;
}

//------------------------------------------------------------------------------

int GrInternalColorString (int x, int y, const char *s)
{
return OglInternalString (x, y, s);
}

//------------------------------------------------------------------------------

int OglUBitMapMC2 (int x, int y, int dw, int dh, CBitmap *bmP, tCanvasColor *c, int scale, int orient);

int GrString (int x, int y, const char *s, int *idP)
{
	int			w, h, aw, clipped = 0;

if (gameOpts->render.coronas.nStyle < 2) {
		grsString	*ps;

	if ((MODE == BM_OGL) && (ps = GetPoolString (s, idP))) {
		OglUBitMapMC (x, y, 0, 0, ps->bmP, &FG_COLOR, F1_0, 0);
		return (int) (ps - stringPool) + 1;
		}
	}
Assert (FONT != NULL);
if (x == 0x8000)	{
	if (y < 0)
		clipped |= 1;
	FONT->StringSize (s, w, h, aw);
	// for x, since this will be centered, only look at
	// width.
	if (w > CCanvas::Current ()->Width ())
		clipped |= 1;
	if (y > CCanvas::Current ()->Height ())
		clipped |= 3;
	else if ((y+h) > CCanvas::Current ()->Height ())
		clipped |= 1;
	else if ((y+h) < 0)
		clipped |= 2;
	}
else {
	if ((x < 0) || (y < 0))
		clipped |= 1;
	FONT->StringSize (s, w, h, aw);
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
	return GrUString (x, y, s);
if (clipped & 2) {
	// Completely clipped...
	return 0;
	}
if (clipped & 1) {
	// Partially clipped...
	}
// Partially clipped...
if (MODE == BM_OGL)
	return OglInternalString (x, y, s);
if (FONT->Flags () & FT_COLOR)
	return GrInternalColorString (x, y, s);
if (BG_COLOR.index == -1)
	return GrInternalStringClippedM (x, y, s);
return GrInternalStringClipped (x, y, s);
}

//------------------------------------------------------------------------------

int GrUString (int x, int y, const char *s)
{
if (MODE == BM_OGL)
	return OglInternalString (x, y, s);
if (FONT->Flags () & FT_COLOR)
	return GrInternalColorString (x, y, s);
else if (MODE != BM_LINEAR)
	return 0;
if (BG_COLOR.index == -1)
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

FONT->GetInfo (font);
bits = 0;
nextRowP = s;
FG_COLOR.rgb = 0;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	x = x1;
	if (x == 0x8000)			//centered
		x = FONT->GetCenteredX (text_ptr1);
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
				FG_COLOR.index = * (textP+1);
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

			FONT->GetCharWidth (textP [0], textP [1], width, spacing);
			letter = *textP - font.minChar;
			if (!FONT->InFont (letter)) {	//not in font, draw as space
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
					CCanvas::Current ()->SetColor (FG_COLOR.index);
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
						CCanvas::Current ()->SetColor (FG_COLOR.index);
					else
						CCanvas::Current ()->SetColor (BG_COLOR.index);
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

FONT->GetInfo (font);
bits = 0;
nextRowP = s;
FG_COLOR.rgb = 0;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	x = x1;
	if (x==0x8000)			//centered
		x = FONT->GetCenteredX (text_ptr1);
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
				FG_COLOR.index = * (textP+1);
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
			FONT->GetCharWidth (textP[0], textP[1], width, spacing);
			letter = *textP-font.minChar;
			if (!FONT->InFont (letter)) {	//not in font, draw as space
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
					CCanvas::Current ()->SetColor (FG_COLOR.index);
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
						CCanvas::Current ()->SetColor (FG_COLOR.index);
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

