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
#include "gr.h"
#include "grdef.h"
#include "error.h"

#include "cfile.h"
#include "mono.h"
#include "byteswap.h"
#include "bitmap.h"
#include "gamefont.h"

#include "makesig.h"

#define MAX_OPEN_FONTS	50
#define LHX(x)	 (gameStates.menus.bHires ? 2 * (x) : x)

typedef struct tOpenFont {
	char filename[SHORT_FILENAME_LEN];
	grsFont *ptr;
	char *pData;
} tOpenFont;

//list of open fonts, for use (for now) for palette remapping
tOpenFont openFont [MAX_OPEN_FONTS];

#define BITS_TO_BYTES(x)    (( (x)+7)>>3)

int GrInternalStringClipped (int x, int y, const char *s);
int GrInternalStringClippedM (int x, int y, const char *s);

//------------------------------------------------------------------------------

ubyte *find_kern_entry (grsFont *font, ubyte first, ubyte second)
{
	ubyte *p=font->ftKernData;

	while (*p!=255)
		if (p[0]==first && p[1]==second)
			return p;
		else p+=3;

	return NULL;

}

//------------------------------------------------------------------------------
//takes the character AFTER being offset into font
#define INFONT(_c) ((_c >= 0) && (_c <= FMAXCHAR-FMINCHAR))

//takes the character BEFORE being offset into current font
void GetCharWidth (ubyte c, ubyte c2, int *width, int *spacing)
{
	int letter;

	letter = c-FMINCHAR;

	if (!INFONT (letter)) {				//not in font, draw as space
		*width=0;
		if (FFLAGS & FT_PROPORTIONAL)
			*spacing = FWIDTH/2;
		else
			*spacing = FWIDTH;
		return;
	}

	if (FFLAGS & FT_PROPORTIONAL)
		*width = FWIDTHS[letter];
	else
		*width = FWIDTH;

	*spacing = *width;

	if (FFLAGS & FT_KERNED)  {
		ubyte *p;

		if (!(c2==0 || c2=='\n')) {
			int letter2;

			letter2 = c2-FMINCHAR;

			if (INFONT (letter2)) {

				p = find_kern_entry (FONT, (ubyte)letter, (ubyte)letter2);

				if (p)
					*spacing = p[2];
			}
		}
	}
}

//------------------------------------------------------------------------------

int GetLineWidth (const char *s)
{
	int w, w2, s2;

if (!s)
	return 0;
for (w = 0; *s && (*s != '\n'); s++) {
	if (*s <= 0x06) {
		if (*s <= 0x03)
			s++;
		continue;//skip color codes.
		}
	GetCharWidth (s[0], s[1], &w2, &s2);
	w += s2;
	}
return w;
}

//------------------------------------------------------------------------------

int GetCenteredX (const char *s)
{
return ((grdCurCanv->cvBitmap.bmProps.w - GetLineWidth (s)) / 2);
}

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
	unsigned char * fp;
	const char *textP, *nextRowP, *text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;
	unsigned int VideoOffset, VideoOffset1;
	ubyte *palette = grdCurCanv->cvBitmap.bmPalette;

if (!palette)
	palette = gamePalette;

if (FG_COLOR.rgb) {
	FG_COLOR.rgb = 0;
	FG_COLOR.index = GrFindClosestColor (gamePalette,
													 FG_COLOR.color.red, FG_COLOR.color.green, FG_COLOR.color.blue);
	}
if (BG_COLOR.rgb) {
	BG_COLOR.rgb = 0;
	BG_COLOR.index = GrFindClosestColor (gamePalette,
													 BG_COLOR.color.red, BG_COLOR.color.green, BG_COLOR.color.blue);
	}
bits=0;
VideoOffset1 = y * ROWSIZE + x;
nextRowP = s;
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	if (x == 0x8000) {			//centered
		int xx = GetCenteredX (text_ptr1);
		VideoOffset1 = y * ROWSIZE + xx;
		}
	for (r = 0; r < FHEIGHT; r++) {
		textP = text_ptr1;
		VideoOffset = VideoOffset1;
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
				if ((r == FBASELINE + 2) || (r == FBASELINE + 3))
					underline = 1;
				textP++;
				}
			GetCharWidth (textP[0], textP[1], &width, &spacing);
			letter = *textP - FMINCHAR;
			if (!INFONT (letter)) {	//not in font, draw as space
				VideoOffset += spacing;
				textP++;
				continue;
				}
			if (FFLAGS & FT_PROPORTIONAL)
				fp = FCHARS [letter];
			else
				fp = FDATA + letter * BITS_TO_BYTES (width)*FHEIGHT;
			if (underline)
				for (i = 0; i < width; i++)
					DATA[VideoOffset++] = (unsigned char) FG_COLOR.index;
				else {
					fp += BITS_TO_BYTES (width)*r;
					BitMask = 0;
					for (i = 0; i < width; i++) {
						if (BitMask == 0) {
							bits = *fp++;
							BitMask = 0x80;
							}
						if (bits & BitMask)
							DATA[VideoOffset++] = (unsigned char) FG_COLOR.index;
						else
							DATA[VideoOffset++] = (unsigned char) BG_COLOR.index;
						BitMask >>= 1;
						}
					}
			VideoOffset += spacing-width;		//for kerning
			textP++;
			}
		VideoOffset1 += ROWSIZE; y++;
		}
	y += skip_lines;
	VideoOffset1 += ROWSIZE * skip_lines;
	skip_lines = 0;
	}
return 0;
}

//------------------------------------------------------------------------------

int GrInternalString0m (int x, int y, const char *s)
{
	unsigned char * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;
	char c;
	int orig_color;
	unsigned int VideoOffset, VideoOffset1;

if (FG_COLOR.rgb) {
	FG_COLOR.rgb = 0;
	FG_COLOR.index = GrFindClosestColor (FONT->ftParentBitmap.bmPalette,
													 FG_COLOR.color.red, FG_COLOR.color.green, FG_COLOR.color.blue);
	}
if (BG_COLOR.rgb) {
	BG_COLOR.rgb = 0;
	BG_COLOR.index = GrFindClosestColor (FONT->ftParentBitmap.bmPalette,
													 BG_COLOR.color.red, BG_COLOR.color.green, BG_COLOR.color.blue);
	}
orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
bits=0;
VideoOffset1 = y * ROWSIZE + x;

	nextRowP = s;
	FG_COLOR.rgb = 0;
	while (nextRowP != NULL)
	{
		text_ptr1 = nextRowP;
		nextRowP = NULL;

		if (x==0x8000) {			//centered
			int xx = GetCenteredX (text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
		}

		for (r=0; r<FHEIGHT; r++) {
			textP = text_ptr1;
			VideoOffset = VideoOffset1;
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					c = * (++textP);
					}
				GetCharWidth (c, textP[1], &width, &spacing);
				letter = c - FMINCHAR;
				if (!INFONT (letter) || c <= 0x06) {	//not in font, draw as space
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
						VideoOffset += spacing;
						textP++;
						}
					continue;
					}
				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES (width) * FHEIGHT;

				if (underline)
					for (i=0; i< width; i++)
						DATA[VideoOffset++] = (unsigned int) FG_COLOR.index;
				else {
					fp += BITS_TO_BYTES (width)*r;
					BitMask = 0;
					for (i=0; i< width; i++) {
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)
							DATA[VideoOffset++] = (unsigned int) FG_COLOR.index;
						else
							VideoOffset++;
						BitMask >>= 1;
					}
				}
				textP++;
				VideoOffset += spacing-width;
			}
			VideoOffset1 += ROWSIZE;
			y++;
		}
		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
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

int get_fontTotal_width (grsFont * font){
	if (font->ftFlags & FT_PROPORTIONAL){
		int i, w = 0, c=font->ftMinChar;
		for (i=0;c<=font->ftMaxChar;i++, c++){
			if (font->ftWidths[i]<0)
				Error (TXT_FONT_WIDTH);
			w+=font->ftWidths[i];
		}
		return w;
	}else{
		return font->ftWidth* (font->ftMaxChar-font->ftMinChar+1);
	}
}

//------------------------------------------------------------------------------

void OglFontChooseSize (grsFont * font, int gap, int *rw, int *rh)
{
	int	nchars = font->ftMaxChar-font->ftMinChar+1;
	int r, x, y, nc=0, smallest=999999, smallr=-1, tries;
	int smallprop=10000;
	int h, w;
	for (h=32;h<=256;h*=2){
//		h=Pow2ize (font->ftHeight*rows+gap* (rows-1);
		if (font->ftHeight>h)continue;
		r= (h/ (font->ftHeight+gap));
		w = Pow2ize ((get_fontTotal_width (font)+ (nchars-r)*gap)/r);
		tries=0;
		do {
			if (tries)
				w = Pow2ize (w+1);
			if (tries>3){
				break;
			}
			nc=0;
			y=0;
			while (y+font->ftHeight<=h){
				x=0;
				while (x<w){
					if (nc==nchars)
						break;
					if (font->ftFlags & FT_PROPORTIONAL){
						if (x+font->ftWidths[nc]+gap>w)break;
						x+=font->ftWidths[nc++]+gap;
					}else{
						if (x+font->ftWidth+gap>w)break;
						x+=font->ftWidth+gap;
						nc++;
					}
				}
				if (nc==nchars)
					break;
				y+=font->ftHeight+gap;
			}

			tries++;
		}while (nc!=nchars);
		if (nc!=nchars)
			continue;

		if (w*h==smallest){//this gives squarer sizes priority (ie, 128x128 would be better than 512*32)
			if (w>=h){
				if (w/h<smallprop){
					smallprop=w/h;
					smallest++;//hack
				}
			}else{
				if (h/w<smallprop){
					smallprop=h/w;
					smallest++;//hack
				}
			}
		}
		if (w*h<smallest){
			smallr=1;
			smallest=w*h;
			*rw = w;
			*rh=h;
		}
	}
	if (smallr<=0)
		Error ("couldn't fit font?\n");
}

//------------------------------------------------------------------------------

void OglInitFont (grsFont * font)
{
	int	nchars = font->ftMaxChar-font->ftMinChar+1;
	int i, w, h, tw, th, x, y, curx=0, cury=0;
	ubyte *fp, *data, *palette;
	//	char data[32*32*4];
	int gap=0;//having a gap just wastes ram, since we don't filter text textures at all.
	//	char s[2];
	OglFontChooseSize (font, gap, &tw, &th);
	data = (ubyte *) D2_ALLOC (tw*th);
	palette = font->ftParentBitmap.bmPalette;
	GrInitBitmap (&font->ftParentBitmap, BM_LINEAR, 0, 0, tw, th, tw, data, 1);
	font->ftParentBitmap.bmPalette = palette;
	if (!(font->ftFlags & FT_COLOR))
		font->ftParentBitmap.glTexture = OglGetFreeTexture ();
	font->ftBitmaps = (grsBitmap*) D2_ALLOC (nchars * sizeof (grsBitmap));
	memset (font->ftBitmaps, 0, nchars * sizeof (grsBitmap));
#if TRACE
//	con_printf (CONDBG, "OglInitFont %s, %s, nchars=%i, (%ix%i tex)\n",
//		 (font->ftFlags & FT_PROPORTIONAL)?"proportional":"fixedwidth", (font->ftFlags & FT_COLOR)?"color":"mono", nchars, tw, th);
#endif
	//	s[1]=0;
	h = font->ftHeight;
	//	sleep (5);

	for (i = 0; i < nchars; i++) {
		//		s[0]=font->ftMinChar+i;
		//		GrGetStringSize (s, &w, &h, &aw);
		if (font->ftFlags & FT_PROPORTIONAL)
			w = font->ftWidths [i];
		else
			w = font->ftWidth;
		if (w < 1 || w > 256)
			continue;
		if (curx + w + gap > tw) {
			cury += h + gap;
			curx = 0;
			}
		if (cury + h > th)
			Error (TXT_FONT_SIZE, i, nchars);
		if (font->ftFlags & FT_COLOR) {
			if (font->ftFlags & FT_PROPORTIONAL)
				fp = font->ftChars[i];
			else
				fp = font->ftData + i * w*h;
			for (y = 0; y < h; y++)
				for (x = 0; x < w; x++){
					font->ftParentBitmap.bmTexBuf [curx + x + (cury + y) * tw] = fp [x + y * w];
				}

			//			GrInitBitmap (&font->ftBitmaps[i], BM_LINEAR, 0, 0, w, h, w, font->);
			}
		else {
			int BitMask, bits = 0, white = GrFindClosestColor (palette, 63, 63, 63);
			//			if (w*h>sizeof (data))
			//				Error ("OglInitFont: toobig\n");
			if (font->ftFlags & FT_PROPORTIONAL)
				fp = font->ftChars [i];
			else
				fp = font->ftData + i * BITS_TO_BYTES (w) * h;
			for (y = 0; y < h; y++) {
				BitMask = 0;
				for (x = 0; x < w; x++) {
					if (BitMask == 0) {
						bits = *fp++;
						BitMask = 0x80;
						}
					font->ftParentBitmap.bmTexBuf [curx + x + (cury + y) * tw] = (bits & BitMask) ? white : 255;
					BitMask >>= 1;
					}
				}
			}
		GrInitSubBitmap (&font->ftBitmaps[i], &font->ftParentBitmap, curx, cury, w, h);
		curx += w + gap;
		}
	if (!(font->ftFlags & FT_COLOR)) {
		//use GL_INTENSITY instead of GL_RGB
		if (gameStates.ogl.bIntensity4) {
			font->ftParentBitmap.glTexture->internalformat = 1;
			font->ftParentBitmap.glTexture->format = GL_LUMINANCE;
			}
		else if (gameStates.ogl.bLuminance4Alpha4){
			font->ftParentBitmap.glTexture->internalformat = 1;
			font->ftParentBitmap.glTexture->format = GL_LUMINANCE_ALPHA;
			}
		else {
			font->ftParentBitmap.glTexture->internalformat = gameStates.ogl.bpp / 8;
			font->ftParentBitmap.glTexture->format = gameStates.ogl.nRGBAFormat;
			}
	OglLoadBmTextureM (&font->ftParentBitmap, 0, 2, 0, NULL);
	}
}

//------------------------------------------------------------------------------

int OglInternalString (int x, int y, const char *s)
{
	const char * textP, * nextRowP, * text_ptr1;
	int width, spacing, letter;
	int xx, yy;
	int orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
	ubyte c;
	grsBitmap *bmf;

nextRowP = s;
yy = y;
if (grdCurScreen->scCanvas.cvBitmap.bmProps.nType != BM_OGL)
	Error ("carp.\n");
while (nextRowP != NULL) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
	xx = x;
	if (xx == 0x8000)			//centered
		xx = GetCenteredX (textP);
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			yy += FHEIGHT + 2;
			break;
			}
		letter = c - FMINCHAR;
		GetCharWidth (c, textP [1], &width, &spacing);
		if (!INFONT (letter) || (c <= 0x06)) {	//not in font, draw as space
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
		bmf = FONT->ftBitmaps + letter;
		bmf->bmProps.flags |= BM_FLAG_TRANSPARENT;
		if (FFLAGS & FT_COLOR)
			GrBitmapM (xx, yy, bmf, 2); // credits need clipping
		else {
			if (grdCurCanv->cvBitmap.bmProps.nType == BM_OGL)
				OglUBitMapMC (xx, yy, 0, 0, bmf, &FG_COLOR, F1_0, 0);
			else
				Error ("OglInternalString: non-color string to non-ogl dest\n");
	//					GrUBitmapM (xx, yy, &FONT->ftBitmaps[letter]);//ignores color..
			}
		xx += spacing;
		textP++;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

grsBitmap *CreateStringBitmap (
	const char *s, int nKey, unsigned int nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce)
{
	int			orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
	int			i, x, y, hx, hy, w, h, aw, cw, spacing, nTab, nChars, bHotKey;
	grsBitmap	*bmP, *bmfP;
	grsRgba		hc, kc, *pc;
	ubyte			*pf, *palP = NULL;
	ubyte			c;
	const char	*textP, *text_ptr1, *nextRowP;
	int			letter;

if (!(bForce || (gameOpts->menus.nStyle && gameOpts->menus.bFastMenus)))
	return NULL;
GrGetStringSizeTabbed (s, &w, &h, &aw, nTabs, nMaxWidth);
if (!(w && h))
	return NULL;
for (i = 1; i < w; i <<= 2)
	;
w = i;
for (i = 1; i < h; i <<= 2)
	;
h = i;
if (!(bmP = GrCreateBitmap (w, h, 4))) {
	return NULL;
	}
if (!bmP->bmTexBuf) {
	GrFreeBitmap (bmP);
	return NULL;
	}
memset (bmP->bmTexBuf, 0, w * h * bmP->bmBPP);
bmP->bmProps.flags |= BM_FLAG_TRANSPARENT;
nextRowP = s;
y = 0;
nTab = 0;
nChars = 0;
while (nextRowP) {
	text_ptr1 = nextRowP;
	nextRowP = NULL;
	textP = text_ptr1;
#ifdef _DEBUG
	if (bCentered)
		x = (w - GetLineWidth (textP)) / 2;
	else
		x = 0;
#else
	x = bCentered ? (w - GetLineWidth (textP)) / 2 : 0;
#endif
	while ((c = *textP)) {
		if (c == '\n') {
			nextRowP = textP + 1;
			y += FHEIGHT + 2;
			nTab = 0;
			break;
			}
		if (c == '\t') {
			textP++;
			if (nTabs && (nTab < 6)) {
				int	w, h, aw;

				GrGetStringSize (textP, &w, &h, &aw);
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
		letter = c - FMINCHAR;
		GetCharWidth (c, textP [1], &cw, &spacing);
		if (!INFONT (letter) || (c <= 0x06)) {	//not in font, draw as space
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
		bmfP = (bHotKey && (FONT != SMALL_FONT)) ? SELECTED_FONT->ftBitmaps + letter : FONT->ftBitmaps + letter;
		palP = BM_PARENT (bmfP) ? BM_PARENT (bmfP)->bmPalette : bmfP->bmPalette;
		nChars++;
		i = nKeyColor * 3;
		kc.red = RGBA_RED (nKeyColor);
		kc.green = RGBA_GREEN (nKeyColor);
		kc.blue = RGBA_BLUE (nKeyColor);
		kc.alpha = 255;
		if (FFLAGS & FT_COLOR) {
			for (hy = 0; hy < bmfP->bmProps.h; hy++) {
				pc = ((grsRgba *) bmP->bmTexBuf) + (y + hy) * w + x;
				pf = bmfP->bmTexBuf + hy * bmfP->bmProps.rowSize;
				for (hx = bmfP->bmProps.w; hx; hx--, pc++, pf++)
					if ((c = *pf) != TRANSPARENCY_COLOR) {
						i = c * 3;
						pc->red = palP [i] * 4;
						pc->green = palP [i + 1] * 4;
						pc->blue = palP [i + 2] * 4;
						pc->alpha = 255;
						}
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
					i = FG_COLOR.index * 3;
					hc.red = palP [i] * 4;
					hc.green = palP [i + 1] * 4;
					hc.blue = palP [i + 2] * 4;
					hc.alpha = 255;
					}
				}
			for (hy = 0; hy < bmfP->bmProps.h; hy++) {
				pc = ((grsRgba *) bmP->bmTexBuf) + (y + hy) * w + x;
				pf = bmfP->bmTexBuf + hy * bmfP->bmProps.rowSize;
				for (hx = bmfP->bmProps.w; hx; hx--, pc++, pf++)
					if (*pf != TRANSPARENCY_COLOR)
						*pc = bHotKey ? kc : hc;
				}
			}
		x += spacing;
		textP++;
		}
	}
bmP->bmPalette = palP;
//OglLoadBmTexture (bmP, 0, 2, 1);
return bmP;
}

//------------------------------------------------------------------------------

int GrInternalColorString (int x, int y, const char *s)
{
	return OglInternalString (x, y, s);
}

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
	D2_FREE (ps->pszText);
	if (ps->pId)
		*ps->pId = 0;
	OglFreeBmTexture (ps->bmP);
	GrFreeBitmap (ps->bmP);
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
	OglFreeBmTexture (ps->bmP);
	GrFreeBitmap (ps->bmP);
	}
else {
	if (nPoolStrings >= GRS_MAX_STRINGS)
		return NULL;
	ps = stringPool + nPoolStrings;
	}
GrGetStringSize (s, &w, &h, &aw);
if (!(ps->bmP = CreateStringBitmap (s, 0, 0, 0, 0, w, 1))) {
	*idP = 0;
	return NULL;
	}
l = (int) strlen (s) + 1;
if (ps->pszText && (ps->nLength < l))
	D2_FREE (ps->pszText);
if (!ps->pszText) {
	ps->nLength = 3 * l / 2;
	if (!(ps->pszText = (char *) D2_ALLOC (ps->nLength))) {
		GrFreeBitmap (ps->bmP);
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
int OglUBitMapMC2 (int x, int y, int dw, int dh, grsBitmap *bmP, grsColor *c, int scale, int orient);
int GrString (int x, int y, const char *s, int *idP)
{
	int			w, h, aw, clipped = 0;

if (gameOpts->render.coronas.nStyle < 2) {
		grsString	*ps;

	if ((TYPE == BM_OGL) && (ps = GetPoolString (s, idP))) {
		OglUBitMapMC (x, y, 0, 0, ps->bmP, &FG_COLOR, F1_0, 0);
		return (int) (ps - stringPool) + 1;
		}
	}
Assert (FONT != NULL);
if (x == 0x8000)	{
	if (y < 0)
		clipped |= 1;
	GrGetStringSize (s, &w, &h, &aw);
	// for x, since this will be centered, only look at
	// width.
	if (w > grdCurCanv->cvBitmap.bmProps.w)
		clipped |= 1;
	if (y > grdCurCanv->cvBitmap.bmProps.h)
		clipped |= 3;
	else if ((y+h) > grdCurCanv->cvBitmap.bmProps.h)
		clipped |= 1;
	else if ((y+h) < 0)
		clipped |= 2;
	}
else {
	if ((x < 0) || (y < 0))
		clipped |= 1;
	GrGetStringSize (s, &w, &h, &aw);
	if (x > grdCurCanv->cvBitmap.bmProps.w)
		clipped |= 3;
	else if ((x + w) > grdCurCanv->cvBitmap.bmProps.w)
		clipped |= 1;
	else if ((x + w) < 0)
		clipped |= 2;
	if (y > grdCurCanv->cvBitmap.bmProps.h)
		clipped |= 3;
	else if ((y + h) > grdCurCanv->cvBitmap.bmProps.h)
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
if (TYPE == BM_OGL)
	return OglInternalString (x, y, s);
if (FFLAGS & FT_COLOR)
	return GrInternalColorString (x, y, s);
if (BG_COLOR.index == -1)
	return GrInternalStringClippedM (x, y, s);
return GrInternalStringClipped (x, y, s);
}

//------------------------------------------------------------------------------

int GrUString (int x, int y, const char *s)
{
if (TYPE == BM_OGL)
	return OglInternalString (x, y, s);
if (FFLAGS & FT_COLOR)
	return GrInternalColorString (x, y, s);
else if (TYPE != BM_LINEAR)
	return 0;
if (BG_COLOR.index == -1)
	return GrInternalString0m (x, y, s);
return GrInternalString0 (x, y, s);
}

//------------------------------------------------------------------------------

void GrGetStringSize (const char *s, int *string_width, int *string_height, int *average_width)
{
	int i = 0, longest_width = 0, nTab = 0;
	int width, spacing;

	*string_height = FHEIGHT;
	*string_width = 0;
	*average_width = FWIDTH;

	if (s != NULL)
	{
		*string_width = 0;
		while (*s)
		{
//			if (*s == CC_UNDERLINE)
//				s++;
			while (*s == '\n')
			{
				s++;
				*string_height += FHEIGHT + 2;
				*string_width = 0;
				nTab = 0;
			}

			if (*s == 0) break;

			//	1 = next byte specifies color, so skip the 1 and the color value
			if (*s == CC_COLOR)
				s += 2;
			else if (*s == CC_LSPACING) {
				*string_height += * (s+1)-'0';
				s += 2;
				}
			else {
				GetCharWidth (s[0], s[1], &width, &spacing);

				*string_width += spacing;

				if (*string_width > longest_width)
					longest_width = *string_width;

				i++;
				s++;
			}
		}
	}
	*string_width = longest_width;
}

//------------------------------------------------------------------------------

void GrGetStringSizeTabbed (const char *s, int *string_width, int *string_height, int *average_width,
									 int *nTabs, int nMaxWidth)
{
	char	*pi, *pj;
	int	w = 0, nTab = 0;
	static char	hs [10000];

strcpy (hs, s);
*string_width =
*string_height = 0;
pi = hs;
do {
	pj = strchr (pi, '\t');
	if (pj)
		*pj = '\0';
	GrGetStringSize (pi, &w, string_height, average_width);
	if (nTab && nTabs) {
		*string_width = LHX (nTabs [nTab - 1]);
		if (gameStates.multi.bSurfingNet)
			*string_width += w;
		else
			*string_width += nMaxWidth;
		}
	else
		*string_width += w;
	if (pj) {
		nTab++;
		pi = pj + 1;
		}
	} while (pj);
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
	static char buffer[1000];
	va_list args;

va_start (args, format);
vsprintf (buffer, format, args);
return GrString (x, y, buffer, idP);
}

//------------------------------------------------------------------------------

void GrCloseFont (grsFont * font)
{
	if (font)
	{
		int nFont;
		char * fontDataP;

		//find font in list
	for (nFont=0;nFont<MAX_OPEN_FONTS && openFont[nFont].ptr!=font;nFont++)
		;
	Assert (nFont<MAX_OPEN_FONTS);	//did we find slot?
	fontDataP = openFont[nFont].pData;
	D2_FREE (fontDataP);
	openFont[nFont].ptr = NULL;
	openFont[nFont].pData = NULL;
	if (font->ftChars) {
		D2_FREE (font->ftChars);
		font->ftChars = NULL;
		}
	if (font->ftBitmaps) {
		D2_FREE (font->ftBitmaps);
		font->ftBitmaps = NULL;
		}
	GrFreeBitmapData (&font->ftParentBitmap);
	D2_FREE (font);
	font = NULL;
	}
}

//------------------------------------------------------------------------------

void GrRemapMonoFonts ()
{
	int nFont;

	for (nFont=0;nFont<MAX_OPEN_FONTS;nFont++) {
		grsFont *font;
		font = openFont[nFont].ptr;
		if (font && !(font->ftFlags & FT_COLOR))
			GrRemapFont (font, openFont[nFont].filename, openFont[nFont].pData);
	}
}

//------------------------------------------------------------------------------
//remap (by re-reading) all the color fonts
void GrRemapColorFonts ()
{
	int nFont;

for (nFont=0;nFont<MAX_OPEN_FONTS;nFont++) {
	grsFont *font;
	font = openFont[nFont].ptr;
	if (font && (font->ftFlags & FT_COLOR))
		GrRemapFont (font, openFont[nFont].filename, openFont[nFont].pData);
	}
}

//------------------------------------------------------------------------------

#if 0//def FAST_FILE_IO
#define grs_font_read (gf, fp) CFRead (gf, GRS_FONT_SIZE, 1, fp)
#else
/*
 * reads a grsFont structure from a CFILE
 */
void grs_font_read (grsFont *gf, CFILE *fp)
{
	gf->ftWidth = CFReadShort (fp);
	gf->ftHeight = CFReadShort (fp);
	gf->ftFlags = CFReadShort (fp);
	gf->ftBaseLine = CFReadShort (fp);
	gf->ftMinChar = CFReadByte (fp);
	gf->ftMaxChar = CFReadByte (fp);
	gf->ftByteWidth = CFReadShort (fp);
	gf->ftData = (ubyte *) (size_t) CFReadInt (fp);
	gf->ftChars = (ubyte **) (size_t) CFReadInt (fp);
	gf->ftWidths = (short *) (size_t) CFReadInt (fp);
	gf->ftKernData = (ubyte *) (size_t) CFReadInt (fp);
}
#endif

//------------------------------------------------------------------------------

grsFont * GrInitFont (const char *fontname)
{
	static int bFirstTime = 1;
	grsFont *font;
	char *fontDataP;
	int i, nFont;
	unsigned char *ptr;
	int nchars;
	CFILE fontfile;
	char file_id[4];
	int datasize;	//size up to (but not including) palette
	int freq[256];

if (bFirstTime) {
	int i;
	for (i = 0; i < MAX_OPEN_FONTS; i++) {
		openFont [i].ptr = NULL;
		openFont [i].pData = NULL;
		}
	bFirstTime = 0;
	}

	//find D2_FREE font slot
for (nFont = 0; (nFont < MAX_OPEN_FONTS) && openFont [nFont].ptr; nFont++)
	;
Assert (nFont<MAX_OPEN_FONTS);	//did we find one?
strncpy (openFont[nFont].filename, fontname, SHORT_FILENAME_LEN);
if (!CFOpen (&fontfile, fontname, gameFolders.szDataDir, "rb", 0)) {
#if TRACE
	con_printf (CON_VERBOSE, "Can't open font file %s\n", fontname);
#endif
	return NULL;
	}

CFRead (file_id, 4, 1, &fontfile);
if (!strncmp (file_id, "NFSP", 4)) {
#if TRACE
	con_printf (CON_NORMAL, "File %s is not a font file\n", fontname);
#endif
	return NULL;
}

datasize = CFReadInt (&fontfile);
datasize -= GRS_FONT_SIZE; // subtract the size of the header.

MALLOC (font, grsFont, sizeof (grsFont));
grs_font_read (font, &fontfile);

MALLOC (fontDataP, char, datasize);
CFRead (fontDataP, 1, datasize, &fontfile);

openFont[nFont].ptr = font;
openFont[nFont].pData = fontDataP;

// make these offsets relative to fontDataP
font->ftData = (ubyte *) ((size_t)font->ftData - GRS_FONT_SIZE);
font->ftWidths = (short *) ((size_t)font->ftWidths - GRS_FONT_SIZE);
font->ftKernData = (ubyte *) ((size_t)font->ftKernData - GRS_FONT_SIZE);

nchars = font->ftMaxChar - font->ftMinChar + 1;

if (font->ftFlags & FT_PROPORTIONAL) {
	font->ftWidths = (short *) &fontDataP[ (size_t)font->ftWidths];
	font->ftData = (ubyte *) (fontDataP + (size_t)font->ftData);
	font->ftChars = (unsigned char **)D2_ALLOC (nchars * sizeof (unsigned char *));
	ptr = font->ftData;
	for (i = 0; i < nchars; i++) {
		font->ftWidths[i] = INTEL_SHORT (font->ftWidths[i]);
		font->ftChars[i] = ptr;
		if (font->ftFlags & FT_COLOR)
			ptr += font->ftWidths[i] * font->ftHeight;
		else
			ptr += BITS_TO_BYTES (font->ftWidths[i]) * font->ftHeight;
		}
	}
else  {
	font->ftData   = (ubyte *) fontDataP;
	font->ftChars  = NULL;
	font->ftWidths = NULL;
	ptr = font->ftData + (nchars * font->ftWidth * font->ftHeight);
	}
if (font->ftFlags & FT_KERNED)
	font->ftKernData = (ubyte *) (fontDataP + (size_t)font->ftKernData);
memset (&font->ftParentBitmap, 0, sizeof (font->ftParentBitmap));
memset (freq, 0, sizeof (freq));
if (font->ftFlags & FT_COLOR) {		//remap palette
	ubyte palette[256*3];
	CFRead (palette, 3, 256, &fontfile);		//read the palette
#ifdef SWAP_0_255			// swap the first and last palette entries (black and white)
	{
		int i;
		ubyte c;

	for (i = 0; i < 3; i++) {
		c = palette[i];
		palette[i] = palette[765+i];
		palette[765+i] = c;
		}

//  we also need to swap the data entries as well.  black is white and white is black

	for (i = 0; i < ptr-font->ftData; i++) {
		if (font->ftData[i] == 0)
			font->ftData[i] = 255;
		else if (font->ftData[i] == 255)
			font->ftData[i] = 0;
		}
	}
#endif
	GrCountColors (font->ftData, (int) (ptr - font->ftData), freq);
	GrSetPalette (&font->ftParentBitmap, palette, TRANSPARENCY_COLOR, -1, freq);
	}
else
	GrSetPalette (&font->ftParentBitmap, defaultPalette, TRANSPARENCY_COLOR, -1, freq);
CFClose (&fontfile);

FONT = font;
FG_COLOR.index = 0;
BG_COLOR.index = 0;

int x, y, aw;
char tests[]="abcdefghij1234.A";
GrGetStringSize (tests, &x, &y, &aw);

OglInitFont (font);
return font;
}

//------------------------------------------------------------------------------

//remap a font by re-reading its data & palette
void GrRemapFont (grsFont *font, char * fontname, char *fontDataP)
{
	int i;
	int nchars;
	CFILE fontfile;
	char file_id[4];
	int datasize;        //size up to (but not including) palette
	unsigned char *ptr;

//	if (!(font->ftFlags & FT_COLOR))
//		return;
	if (!CFOpen (&fontfile, fontname, gameFolders.szDataDir, "rb", 0))
		Error (TXT_FONT_FILE, fontname);
	CFRead (file_id, 4, 1, &fontfile);
	if (!strncmp (file_id, "NFSP", 4))
		Error (TXT_FONT_FILETYPE, fontname);
	datasize = CFReadInt (&fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.
	D2_FREE (font->ftChars);
	grs_font_read (font, &fontfile); // have to reread in case mission tHogFile overrides font.
	CFRead (fontDataP, 1, datasize, &fontfile);  //read raw data
	// make these offsets relative to fontDataP
	font->ftData = (ubyte *) ((size_t)font->ftData - GRS_FONT_SIZE);
	font->ftWidths = (short *) ((size_t)font->ftWidths - GRS_FONT_SIZE);
	font->ftKernData = (ubyte *) ((size_t)font->ftKernData - GRS_FONT_SIZE);
	nchars = font->ftMaxChar - font->ftMinChar + 1;
	if (font->ftFlags & FT_PROPORTIONAL) {
		font->ftWidths = (short *) (fontDataP + (size_t)font->ftWidths);
		font->ftData = (ubyte *) (fontDataP + (size_t) font->ftData);
		font->ftChars = (unsigned char **)D2_ALLOC (nchars * sizeof (unsigned char *));
		ptr = font->ftData;
		for (i=0; i< nchars; i++) {
			font->ftWidths[i] = INTEL_SHORT (font->ftWidths[i]);
			font->ftChars[i] = ptr;
			if (font->ftFlags & FT_COLOR)
				ptr += font->ftWidths[i] * font->ftHeight;
			else
				ptr += BITS_TO_BYTES (font->ftWidths[i]) * font->ftHeight;
			}
		}
	else  {
		font->ftData   = (ubyte *) fontDataP;
		font->ftChars  = NULL;
		font->ftWidths = NULL;
		ptr = font->ftData + (nchars * font->ftWidth * font->ftHeight);
		}
	if (font->ftFlags & FT_KERNED)
		font->ftKernData = (ubyte *) (fontDataP + (size_t)font->ftKernData);
	if (font->ftFlags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		int freq[256];
		CFRead (palette, 3, 256, &fontfile);		//read the palette
#ifdef SWAP_0_255			// swap the first and last palette entries (black and white)
		{
			int i;
			ubyte c;

			for (i = 0; i < 3; i++) {
				c = palette[i];
				palette[i] = palette[765+i];
				palette[765+i] = c;
			}
//  we also need to swap the data entries as well.  black is white and white is black
			for (i = 0; i < ptr-font->ftData; i++) {
				if (font->ftData[i] == 0)
					font->ftData[i] = 255;
				else if (font->ftData[i] == 255)
					font->ftData[i] = 0;
			}
		}
#endif
		memset (freq, 0, sizeof (freq));
		GrCountColors (font->ftData, (int) (ptr - font->ftData), freq);
		GrSetPalette (&font->ftParentBitmap, palette, TRANSPARENCY_COLOR, -1, freq);
	}
CFClose (&fontfile);
if (font->ftBitmaps) {
	D2_FREE (font->ftBitmaps);
	}
GrFreeBitmapData (&font->ftParentBitmap);
OglInitFont (font);
}

//------------------------------------------------------------------------------

void GrSetFontColor (int fg, int bg)
{
if (fg >= 0)
	FG_COLOR.index = fg;
FG_COLOR.rgb = 0;
if (bg >= 0)
	BG_COLOR.index = bg;
BG_COLOR.rgb = 0;
}

//------------------------------------------------------------------------------

void GrSetFontColorRGB (grsRgba *fg, grsRgba *bg)
{
if (fg) {
	FG_COLOR.rgb = 1;
	FG_COLOR.color = *fg;
	}
if (bg) {
	BG_COLOR.rgb = 1;
	BG_COLOR.color = *fg;
	}
}

//------------------------------------------------------------------------------

void GrSetFontColorRGBi (unsigned int fg, int bSetFG, unsigned int bg, int bSetBG)
{
if (bSetFG) {
	FG_COLOR.rgb = 1;
	FG_COLOR.color.red = RGBA_RED (fg);
	FG_COLOR.color.green = RGBA_GREEN (fg);
	FG_COLOR.color.blue = RGBA_BLUE (fg);
	FG_COLOR.color.alpha = RGBA_ALPHA (fg);
	}
if (bSetBG) {
	BG_COLOR.rgb = 1;
	BG_COLOR.color.red = RGBA_RED (bg);
	BG_COLOR.color.green = RGBA_GREEN (bg);
	BG_COLOR.color.blue = RGBA_BLUE (bg);
	BG_COLOR.color.alpha = RGBA_ALPHA (bg);
	}
}

//------------------------------------------------------------------------------

void GrSetCurFont (grsFont * newFont)
{
	FONT = newFont;
}


//------------------------------------------------------------------------------

int GrInternalStringClipped (int x, int y, const char *s)
{
	unsigned char * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

  bits=0;

	nextRowP = s;
	FG_COLOR.rgb = 0;
	while (nextRowP != NULL)
	{
		text_ptr1 = nextRowP;
		nextRowP = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = GetCenteredX (text_ptr1);

		last_x = x;

		for (r=0; r<FHEIGHT; r++)	{
			textP = text_ptr1;
			x = last_x;

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
				if (*textP == CC_UNDERLINE)	{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					textP++;
				}

				GetCharWidth (textP[0], textP[1], &width, &spacing);

				letter = *textP-FMINCHAR;

				if (!INFONT (letter)) {	//not in font, draw as space
					x += spacing;
					textP++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES (width)*FHEIGHT;

				if (underline)	{
					for (i=0; i< width; i++)	{
						GrSetColor (FG_COLOR.index);
						gr_pixel (x++, y);
					}
				} else {
					fp += BITS_TO_BYTES (width)*r;

					BitMask = 0;

					for (i=0; i< width; i++)	{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)
							GrSetColor (FG_COLOR.index);
						else
							GrSetColor (BG_COLOR.index);
						gr_pixel (x++, y);
						BitMask >>= 1;
					}
				}

				x += spacing-width;		//for kerning

				textP++;
			}
			y++;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------

int GrInternalStringClippedM (int x, int y, const char *s)
{
	unsigned char * fp;
	const char * textP, * nextRowP, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

  bits=0;

	nextRowP = s;
	FG_COLOR.rgb = 0;
	while (nextRowP != NULL)
	{
		text_ptr1 = nextRowP;
		nextRowP = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = GetCenteredX (text_ptr1);

		last_x = x;

		for (r=0; r<FHEIGHT; r++)	{
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
				if (*textP == CC_UNDERLINE)	{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					textP++;
				}

				GetCharWidth (textP[0], textP[1], &width, &spacing);

				letter = *textP-FMINCHAR;

				if (!INFONT (letter)) {	//not in font, draw as space
					x += spacing;
					textP++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES (width)*FHEIGHT;

				if (underline)	{
					for (i=0; i< width; i++)	{
						GrSetColor (FG_COLOR.index);
						gr_pixel (x++, y);
					}
				} else {
					fp += BITS_TO_BYTES (width)*r;

					BitMask = 0;

					for (i=0; i< width; i++)	{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)	{
							GrSetColor (FG_COLOR.index);
							gr_pixel (x++, y);
						} else {
							x++;
						}
						BitMask >>= 1;
					}
				}

				x += spacing-width;		//for kerning

				textP++;
			}
			y++;
		}
	}
	return 0;
}
