/* $Id: font.c,v 1.26 2003/11/26 12:26:23 btb Exp $ */
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

#ifndef _MSC_VER
#include <fcntl.h>
#include <unistd.h>
#endif

#include "pa_enabl.h"                   //$$POLY_ACC
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

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#include "makesig.h"

#define MAX_OPEN_FONTS	50
#define LHX(x)	(gameStates.menus.bHires ? 2 * (x) : x)

typedef struct openfont {
	char filename[SHORT_FILENAME_LEN];
	grs_font *ptr;
	char *dataptr;
} openfont;

//list of open fonts, for use (for now) for palette remapping
openfont open_font[MAX_OPEN_FONTS];

#define FONT			grdCurCanv->cv_font
#define FG_COLOR		grdCurCanv->cv_font_fg_color
#define BG_COLOR		grdCurCanv->cv_font_bg_color
#define FWIDTH       FONT->ft_w
#define FHEIGHT      FONT->ft_h
#define FBASELINE    FONT->ft_baseline
#define FFLAGS       FONT->ftFlags
#define FMINCHAR     FONT->ft_minchar
#define FMAXCHAR     FONT->ft_maxchar
#define FDATA        FONT->ft_data
#define FCHARS       FONT->ft_chars
#define FWIDTHS      FONT->ft_widths

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

int gr_internal_string_clipped(int x, int y, char *s);
int gr_internal_string_clipped_m(int x, int y, char *s);

//------------------------------------------------------------------------------

ubyte *find_kern_entry(grs_font *font,ubyte first,ubyte second)
{
	ubyte *p=font->ft_kerndata;

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
void get_char_width(ubyte c,ubyte c2,int *width,int *spacing)
{
	int letter;

	letter = c-FMINCHAR;

	if (!INFONT(letter)) {				//not in font, draw as space
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

			if (INFONT(letter2)) {

				p = find_kern_entry(FONT,(ubyte)letter,(ubyte)letter2);

				if (p)
					*spacing = p[2];
			}
		}
	}
}

//------------------------------------------------------------------------------

int get_line_width (char *s)
{
	int w,w2,s2;

if (!s)
	return 0;
for (w = 0; *s && (*s != '\n'); s++) {
	if (*s <= 0x06) {
		if (*s <= 0x03)
			s++;
		continue;//skip color codes.
		}
	get_char_width (s[0], s[1], &w2, &s2);
	w += s2;
	}
return w;
}

//------------------------------------------------------------------------------

int GetCenteredX(char *s)
{
return ((grdCurCanv->cv_bitmap.bm_props.w - get_line_width (s)) / 2);
}

//------------------------------------------------------------------------------
//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color nType thing would be better, but that would be way too much trouble for little gain
int gr_message_colorLevel=1;
#define CHECK_EMBEDDED_COLORS() \
	if ((c >= 0x01) && (c <= 0x03)) { \
		if (*++text_ptr) { \
			if (gr_message_colorLevel >= c) \
				FG_COLOR.index = *text_ptr; \
				FG_COLOR.rgb = 0; \
			text_ptr++; \
			} \
		} \
	else if ((c >= 0x04) && (c <= 0x06)) { \
		if (gr_message_colorLevel >= *text_ptr - 3) \
			FG_COLOR.index = orig_color; \
			FG_COLOR.rgb = 0; \
		text_ptr++; \
		}

//------------------------------------------------------------------------------

int GrInternalString0(int x, int y, char *s)
{
	unsigned char * fp;
	char *text_ptr, *next_row, *text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;
	unsigned int VideoOffset, VideoOffset1;
	ubyte *palette = grdCurCanv->cv_bitmap.bm_palette;

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
next_row = s;
while (next_row != NULL) {
	text_ptr1 = next_row;
	next_row = NULL;
	if (x==0x8000) {			//centered
		int xx = GetCenteredX(text_ptr1);
		VideoOffset1 = y * ROWSIZE + xx;
		}
	for (r=0; r<FHEIGHT; r++) {
		text_ptr = text_ptr1;
		VideoOffset = VideoOffset1;
		while (*text_ptr) {
			if (*text_ptr == '\n') {
				next_row = &text_ptr[1];
				break;
				}
			if (*text_ptr == CC_COLOR) {
				FG_COLOR.index = *(text_ptr+1);
				FG_COLOR.rgb = 0;
				text_ptr += 2;
				continue;
				}
			if (*text_ptr == CC_LSPACING) {
				skip_lines = *(text_ptr+1) - '0';
				text_ptr += 2;
				continue;
				}
			underline = 0;
			if (*text_ptr == CC_UNDERLINE) {
				if ((r==FBASELINE+2) || (r==FBASELINE+3))
					underline = 1;
				text_ptr++;
				}	
			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);
			letter = *text_ptr-FMINCHAR;
			if (!INFONT(letter)) {	//not in font, draw as space
				VideoOffset += spacing;
				text_ptr++;
				continue;
				}
			if (FFLAGS & FT_PROPORTIONAL)
				fp = FCHARS[letter];
			else
				fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;
			if (underline)
				for (i=0; i< width; i++)
					DATA[VideoOffset++] = (unsigned char) FG_COLOR.index;
				else {
					fp += BITS_TO_BYTES(width)*r;
					BitMask = 0;
					for (i=0; i< width; i++) {
						if (BitMask==0) {
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
			text_ptr++;
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

int GrInternalString0m(int x, int y, char *s)
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;
	char c;
	int orig_color;
	unsigned int VideoOffset, VideoOffset1;

if (FG_COLOR.rgb) {
	FG_COLOR.rgb = 0;
	FG_COLOR.index = GrFindClosestColor (FONT->ft_parent_bitmap.bm_palette, 
													 FG_COLOR.color.red, FG_COLOR.color.green, FG_COLOR.color.blue);
	}
if (BG_COLOR.rgb) {
	BG_COLOR.rgb = 0;
	BG_COLOR.index = GrFindClosestColor (FONT->ft_parent_bitmap.bm_palette, 
													 BG_COLOR.color.red, BG_COLOR.color.green, BG_COLOR.color.blue);
	}
orig_color=FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
bits=0;
VideoOffset1 = y * ROWSIZE + x;

	next_row = s;
	FG_COLOR.rgb = 0;
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = GetCenteredX(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
		}

		for (r=0; r<FHEIGHT; r++) {
			text_ptr = text_ptr1;
			VideoOffset = VideoOffset1;
			while (c = *text_ptr) {
				if (c == '\n') {
					next_row = &text_ptr[1];
					break;
					}
				if (c == CC_COLOR) {
					FG_COLOR.index = *(++text_ptr);
					text_ptr++;
					continue;
					}
				if (c == CC_LSPACING) {
					skip_lines = *(++text_ptr) - '0';
					text_ptr++;
					continue;
					}
				underline = 0;
				if (c == CC_UNDERLINE) {
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					c = *(++text_ptr);
					}
				get_char_width(c, text_ptr[1], &width, &spacing);
				letter = c - FMINCHAR;
				if (!INFONT(letter) || c <= 0x06) {	//not in font, draw as space
					CHECK_EMBEDDED_COLORS() 
					else{
						VideoOffset += spacing;
						text_ptr++;
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
					fp += BITS_TO_BYTES(width)*r;
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
				text_ptr++;
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

#ifdef __MSDOS__
int GrInternalString2(int x, int y, char *s)
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int page_switched, skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

	VideoOffset1 = (size_t)DATA + y * ROWSIZE + x;

	bits = 0;

	gr_vesa_setpage(VideoOffset1 >> 16);

	VideoOffset1 &= 0xFFFF;

	next_row = s;

	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = GetCenteredX(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
			gr_vesa_setpage(VideoOffset1 >> 16);
			VideoOffset1 &= 0xFFFF;
		}

		for (r=0; r<FHEIGHT; r++)
		{
			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			page_switched = 0;

			while (*text_ptr)
			{
				if (*text_ptr == '\n')
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					skip_lines = *(text_ptr+1) - '0';
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE)
				{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				Assert(width==spacing);		//no kerning support here

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					VideoOffset += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
				{
					if (VideoOffset+width > 0xFFFF)
					{
						for (i=0; i< width; i++)
						{
							gr_video_memory[VideoOffset++] = FG_COLOR;

							if (VideoOffset > 0xFFFF)
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}
						}
					}
					else
					{
						for (i=0; i< width; i++)
							gr_video_memory[VideoOffset++] = FG_COLOR;
					}
				}
				else
				{
					// fp -- dword
					// VideoOffset
					// width

					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					if (VideoOffset+width > 0xFFFF)
					{
						for (i=0; i< width; i++)
						{
							if (BitMask==0) {
								bits = *fp++;
								BitMask = 0x80;
							}

							if (bits & BitMask)
								gr_video_memory[VideoOffset++] = FG_COLOR;
							else
								gr_video_memory[VideoOffset++] = BG_COLOR;

							BitMask >>= 1;

							if (VideoOffset > 0xFFFF)
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}

						}
					} else {

						if (width == 8)
						{
							bits = *fp++;

							if (bits & 0x80) gr_video_memory[VideoOffset+0] = FG_COLOR;
							else gr_video_memory[VideoOffset+0] = BG_COLOR;

							if (bits & 0x40) gr_video_memory[VideoOffset+1] = FG_COLOR;
							else gr_video_memory[VideoOffset+1] = BG_COLOR;

							if (bits & 0x20) gr_video_memory[VideoOffset+2] = FG_COLOR;
							else gr_video_memory[VideoOffset+2] = BG_COLOR;

							if (bits & 0x10) gr_video_memory[VideoOffset+3] = FG_COLOR;
							else gr_video_memory[VideoOffset+3] = BG_COLOR;

							if (bits & 0x08) gr_video_memory[VideoOffset+4] = FG_COLOR;
							else gr_video_memory[VideoOffset+4] = BG_COLOR;

							if (bits & 0x04) gr_video_memory[VideoOffset+5] = FG_COLOR;
							else gr_video_memory[VideoOffset+5] = BG_COLOR;

							if (bits & 0x02) gr_video_memory[VideoOffset+6] = FG_COLOR;
							else gr_video_memory[VideoOffset+6] = BG_COLOR;

							if (bits & 0x01) gr_video_memory[VideoOffset+7] = FG_COLOR;
							else gr_video_memory[VideoOffset+7] = BG_COLOR;

							VideoOffset += 8;
						} else {
							for (i=0; i< width/2 ; i++)
							{
								if (BitMask==0) {
									bits = *fp++;
									BitMask = 0x80;
								}

								if (bits & BitMask)
									gr_video_memory[VideoOffset++] = FG_COLOR;
								else
									gr_video_memory[VideoOffset++] = BG_COLOR;
								BitMask >>= 1;


								// Unroll twice

								if (BitMask==0) {
									bits = *fp++;
									BitMask = 0x80;
								}

								if (bits & BitMask)
									gr_video_memory[VideoOffset++] = FG_COLOR;
								else
									gr_video_memory[VideoOffset++] = BG_COLOR;
								BitMask >>= 1;
							}
						}
					}
				}
				text_ptr++;
			}

			y ++;
			VideoOffset1 += ROWSIZE;

			if (VideoOffset1 > 0xFFFF) {
				VideoOffset1 -= 0xFFFF + 1;
				if (!page_switched)
					gr_vesa_incpage();
			}
		}

		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

//------------------------------------------------------------------------------

int GrInternalString2m(int x, int y, char *s)
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int page_switched, skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

	VideoOffset1 = (size_t)DATA + y * ROWSIZE + x;

	gr_vesa_setpage(VideoOffset1 >> 16);

	VideoOffset1 &= 0xFFFF;

	next_row = s;

	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = GetCenteredX(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
			gr_vesa_setpage(VideoOffset1 >> 16);
			VideoOffset1 &= 0xFFFF;
		}

		for (r=0; r<FHEIGHT; r++)
		{
			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			page_switched = 0;

			while (*text_ptr)
			{
				if (*text_ptr == '\n')
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					skip_lines = *(text_ptr+1) - '0';
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE)
				{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					VideoOffset += width;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
				{
					if (VideoOffset+width > 0xFFFF)
					{
						for (i=0; i< width; i++)
						{
							gr_video_memory[VideoOffset++] = FG_COLOR;

							if (VideoOffset > 0xFFFF)
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}
						}
					}
					else
					{
						for (i=0; i< width; i++)
							gr_video_memory[VideoOffset++] = FG_COLOR;
					}
				}
				else
				{
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					if (VideoOffset+width > 0xFFFF)
					{
						for (i=0; i< width; i++)
						{
							if (BitMask==0) {
								bits = *fp++;
								BitMask = 0x80;
							}

							if (bits & BitMask)
								gr_video_memory[VideoOffset++] = FG_COLOR;
							else
								VideoOffset++;

							BitMask >>= 1;

							if (VideoOffset > 0xFFFF)
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}

						}
					} else {
						for (i=0; i< width; i++)
						{
							if (BitMask==0) {
								bits = *fp++;
								BitMask = 0x80;
							}

							if (bits & BitMask)
								gr_video_memory[VideoOffset++] = FG_COLOR;
							else
								VideoOffset++;;
							BitMask >>= 1;
						}
					}
				}
				text_ptr++;

				VideoOffset += spacing-width;
			}

			y ++;
			VideoOffset1 += ROWSIZE;

			if (VideoOffset1 > 0xFFFF) {
				VideoOffset1 -= 0xFFFF + 1;
				if (!page_switched)
					gr_vesa_incpage();
			}
		}

		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

#endif // __MSDOS__

//------------------------------------------------------------------------------

#if defined(POLY_ACC)
int GrInternalString5(int x, int y, char *s)
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

  pa_flush();
  VideoOffset1 = y * ROWSIZE + x * PA_BPP;

	next_row = s;

	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = GetCenteredX(text_ptr1);
      VideoOffset1 = y * ROWSIZE + xx * PA_BPP;
		}

		for (r=0; r<FHEIGHT; r++)
		{

			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			while (*text_ptr)
			{
				if (*text_ptr == '\n')
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					skip_lines = *(text_ptr+1) - '0';
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE)
				{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
          VideoOffset += spacing * PA_BPP;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
					for (i=0; i< width; i++)
          {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
				else
				{
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++)
					{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}

						if (bits & BitMask)
                {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
                else
                {    *(short *)(DATA + VideoOffset) = pa_clut[BG_COLOR]; VideoOffset += PA_BPP; }
                BitMask >>= 1;
					}
				}

            VideoOffset += PA_BPP * (spacing-width);       //for kerning

				text_ptr++;
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

int GrInternalString5m(int x, int y, char *s)
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

  pa_flush();
  VideoOffset1 = y * ROWSIZE + x * PA_BPP;

	next_row = s;

	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = GetCenteredX(text_ptr1);
      VideoOffset1 = y * ROWSIZE + xx * PA_BPP;
		}

		for (r=0; r<FHEIGHT; r++)
		{

			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			while (*text_ptr)
			{
				if (*text_ptr == '\n')
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					skip_lines = *(text_ptr+1) - '0';
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE)
				{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
          VideoOffset += spacing * PA_BPP;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
					for (i=0; i< width; i++)
          {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
            else
				{
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++)
					{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}

						if (bits & BitMask)
                {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
                else
                  VideoOffset += PA_BPP;
						BitMask >>= 1;
					}
				}
				text_ptr++;

        VideoOffset += PA_BPP * (spacing-width);
			}

			VideoOffset1 += ROWSIZE; y++;
		}
		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}
#endif

//------------------------------------------------------------------------------

#ifndef OGL
//a bitmap for the character
grsBitmap char_bm = {
				0,0,0,0,						//x,y,w,h
				BM_LINEAR,					//nType
				BM_FLAG_TRANSPARENT,		//flags
				0,								//rowsize
				NULL,							//data
#ifdef BITMAP_SELECTOR
				0,				//selector
#endif
				0,     //bm_avgColor
				0      //unused
};

//------------------------------------------------------------------------------

int GrInternalColorString(int x, int y, char *s)
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter;
	int xx,yy;

	char_bm.bm_props.h = FHEIGHT;		//set height for chars of this font

	next_row = s;

	yy = y;


	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		xx = x;

		if (xx==0x8000)			//centered
			xx = GetCenteredX(text_ptr);

		while (*text_ptr)
		{
			if (*text_ptr == '\n')
			{
				next_row = &text_ptr[1];
				yy += FHEIGHT + 2;
				break;
			}

			letter = *text_ptr-FMINCHAR;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

			if (!INFONT(letter)) {	//not in font, draw as space
				xx += spacing;
				text_ptr++;
				continue;
			}

			if (FFLAGS & FT_PROPORTIONAL)
				fp = FCHARS[letter];
			else
				fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

			GrInitBitmap (&char_bm, BM_LINEAR, 0, 0, width, FHEIGHT, width, fp, 0);
			GrBitmapM(xx,yy,&char_bm);

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

//------------------------------------------------------------------------------

#else //OGL

#include "ogl_init.h"
#include "args.h"
//font handling routines for OpenGL - Added 9/25/99 Matthew Mueller - they are here instead of in arch/ogl because they use all these defines

int pow2ize(int x);//from ogl.c

//------------------------------------------------------------------------------

int get_fontTotal_width(grs_font * font){
	if (font->ftFlags & FT_PROPORTIONAL){
		int i,w=0,c=font->ft_minchar;
		for (i=0;c<=font->ft_maxchar;i++,c++){
			if (font->ft_widths[i]<0)
				Error(TXT_FONT_WIDTH);
			w+=font->ft_widths[i];
		}
		return w;
	}else{
		return font->ft_w*(font->ft_maxchar-font->ft_minchar+1);
	}
}

//------------------------------------------------------------------------------

void ogl_font_choose_size(grs_font * font,int gap,int *rw,int *rh){
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int r,x,y,nc=0,smallest=999999,smallr=-1,tries;
	int smallprop=10000;
	int h,w;
	for (h=32;h<=256;h*=2){
//		h=pow2ize(font->ft_h*rows+gap*(rows-1);
		if (font->ft_h>h)continue;
		r=(h/(font->ft_h+gap));
		w=pow2ize((get_fontTotal_width(font)+(nchars-r)*gap)/r);
		tries=0;
		do {
			if (tries)
				w=pow2ize(w+1);
			if(tries>3){
				break;
			}
			nc=0;
			y=0;
			while(y+font->ft_h<=h){
				x=0;
				while (x<w){
					if (nc==nchars)
						break;
					if (font->ftFlags & FT_PROPORTIONAL){
						if (x+font->ft_widths[nc]+gap>w)break;
						x+=font->ft_widths[nc++]+gap;
					}else{
						if (x+font->ft_w+gap>w)break;
						x+=font->ft_w+gap;
						nc++;
					}
				}
				if (nc==nchars)
					break;
				y+=font->ft_h+gap;
			}
			
			tries++;
		}while(nc!=nchars);
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
			*rw=w;
			*rh=h;
		}
	}
	if (smallr<=0)
		Error("couldn't fit font?\n");
}

//------------------------------------------------------------------------------

void ogl_init_font(grs_font * font)
{
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int i,w,h,tw,th,x,y,curx=0,cury=0;
	ubyte *fp, *data, *palette;
	//	char data[32*32*4];
	int gap=0;//having a gap just wastes ram, since we don't filter text textures at all.
	//	char s[2];
	ogl_font_choose_size(font,gap,&tw,&th);
	data=d_malloc(tw*th);
	palette = font->ft_parent_bitmap.bm_palette;
	GrInitBitmap(&font->ft_parent_bitmap,BM_LINEAR,0,0,tw,th,tw,data, 0);
	font->ft_parent_bitmap.bm_palette = palette;
	if (!(font->ftFlags & FT_COLOR))
		font->ft_parent_bitmap.glTexture=OglGetFreeTexture();
	font->ft_bitmaps=(grsBitmap*)d_malloc(nchars * sizeof(grsBitmap));
	memset (font->ft_bitmaps, 0, nchars * sizeof(grsBitmap));
#if TRACE	
//	con_printf (CON_DEBUG,"ogl_init_font %s, %s, nchars=%i, (%ix%i tex)\n",
//		(font->ftFlags & FT_PROPORTIONAL)?"proportional":"fixedwidth",(font->ftFlags & FT_COLOR)?"color":"mono",nchars,tw,th);
#endif		
	//	s[1]=0;
	h=font->ft_h;
	//	sleep(5);

	for(i=0;i<nchars;i++){
		//		s[0]=font->ft_minchar+i;
		//		GrGetStringSize(s,&w,&h,&aw);
		if (font->ftFlags & FT_PROPORTIONAL)
			w=font->ft_widths[i];
		else
			w=font->ft_w;
		if (w<1 || w>256){
			continue;
		}
		if (curx+w+gap>tw){
			cury+=h+gap;
			curx=0;
		}
		if (cury+h>th)
			Error(TXT_FONT_SIZE,i,nchars);
		if (font->ftFlags & FT_COLOR) {
			if (font->ftFlags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * w*h;
			for (y=0;y<h;y++)
				for (x=0;x<w;x++){
					font->ft_parent_bitmap.bm_texBuf[curx+x+(cury+y)*tw]=fp[x+y*w];
				}

			//			GrInitBitmap(&font->ft_bitmaps[i],BM_LINEAR,0,0,w,h,w,font->);
		}else{
			int BitMask,bits=0,white=GrFindClosestColor(palette,63,63,63);
			//			if (w*h>sizeof(data))
			//				Error("ogl_init_font: toobig\n");
			if (font->ftFlags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * BITS_TO_BYTES(w)*h;
			for (y=0;y<h;y++){
				BitMask=0;
				for (x=0; x< w; x++)
				{
					if (BitMask==0) {
						bits = *fp++;
						BitMask = 0x80;
					}

					if (bits & BitMask)
						font->ft_parent_bitmap.bm_texBuf[curx+x+(cury+y)*tw]=white;
					else
						font->ft_parent_bitmap.bm_texBuf[curx+x+(cury+y)*tw]=255;
					BitMask >>= 1;
				}
			}
		}
		GrInitSubBitmap(&font->ft_bitmaps[i],&font->ft_parent_bitmap,curx,cury,w,h);

		curx+=w+gap;
	}
	if (!(font->ftFlags & FT_COLOR)) {
		//use GL_INTENSITY instead of GL_RGB
		if (gameOpts->ogl.bIntensity4){
			font->ft_parent_bitmap.glTexture->internalformat=GL_INTENSITY4;
			font->ft_parent_bitmap.glTexture->format=GL_LUMINANCE;
			}
		else if (gameOpts->ogl.bLuminance4Alpha4){
			font->ft_parent_bitmap.glTexture->internalformat=GL_LUMINANCE4_ALPHA4;
			font->ft_parent_bitmap.glTexture->format=GL_LUMINANCE_ALPHA;
			}
		else if (gameOpts->ogl.bRgba2){
			font->ft_parent_bitmap.glTexture->internalformat=GL_RGBA2;
			font->ft_parent_bitmap.glTexture->format=GL_RGBA;
			}
		else {
			font->ft_parent_bitmap.glTexture->internalformat=gameOpts->ogl.bRgbaFormat;
			font->ft_parent_bitmap.glTexture->format=GL_RGBA;
			}
	OglLoadBmTextureM(&font->ft_parent_bitmap, 0, 0, 0, NULL);
	}
}

//------------------------------------------------------------------------------

int OglInternalString (int x, int y, char *s)
{
	char * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter;
	int xx, yy;
	int orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
	ubyte c;
	grsBitmap *bmf;

next_row = s;
yy = y;
if (grdCurScreen->sc_canvas.cv_bitmap.bm_props.nType != BM_OGL)
	Error("carp.\n");
while (next_row != NULL) {
	text_ptr1 = next_row;
	next_row = NULL;
	text_ptr = text_ptr1;
	xx = x;
	if (xx == 0x8000)			//centered
		xx = GetCenteredX(text_ptr);
	while (c = *text_ptr) {
		if (c == '\n') {
			next_row = text_ptr + 1;
			yy += FHEIGHT + 2;
			break;
			}
		letter = c - FMINCHAR;
		get_char_width (c, text_ptr [1], &width, &spacing);
		if (!INFONT(letter) || (c <= 0x06)) {	//not in font, draw as space
			CHECK_EMBEDDED_COLORS()
		else {
			xx += spacing;
			text_ptr++;
			}
		continue;
		}
		bmf = FONT->ft_bitmaps + letter;
		bmf->bm_props.flags |= BM_FLAG_TRANSPARENT;
		if (FFLAGS & FT_COLOR)
			GrBitmapM (xx, yy, bmf); // credits need clipping
		else {
			if (grdCurCanv->cv_bitmap.bm_props.nType == BM_OGL)
				OglUBitMapMC (xx, yy, 0, 0, bmf, &FG_COLOR, F1_0, 0);
			else
				Error("OglInternalString: non-color string to non-ogl dest\n");
	//					GrUBitmapM(xx,yy,&FONT->ft_bitmaps[letter]);//ignores color..
			}
		xx += spacing;
		text_ptr++;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

grsBitmap *CreateStringBitmap (
	char *s, int nKey, unsigned int nKeyColor, int nTabs [], int bCentered, int nMaxWidth)
{
	int			orig_color = FG_COLOR.index;//to allow easy reseting to default string color with colored strings -MPM
	int			i, x, y, hx, hy, w, h, aw, cw, spacing, nTab, nChars, bHotKey;
	grsBitmap	*bmP, *bmfP;
	grs_rgba		hc, kc, *pc;
	ubyte			*pf, *palP = NULL;
	ubyte			c;
	char			*text_ptr, *text_ptr1, *next_row;
	int			letter;

if (!(gameOpts->menus.nStyle && gameOpts->menus.bFastMenus))
	return NULL;
GrGetStringSizeTabbed (s, &w, &h, &aw, nTabs, nMaxWidth);
if (!(w && h && (bmP = GrCreateBitmap (w, h, 1))))
	return NULL;
if (!bmP->bm_texBuf) {
	GrFreeBitmap (bmP);
	return NULL;
	}
memset (bmP->bm_texBuf, 0, w * h * 4);
next_row = s;
y = 0;
nTab = 0;
nChars = 0;
while (next_row != NULL) {
	text_ptr1 = next_row;
	next_row = NULL;
	text_ptr = text_ptr1;
#ifdef _DEBUG
	if (bCentered)
		x = (w - get_line_width (text_ptr)) / 2;
	else
		x = 0;
#else
	x = bCentered ? (w - get_line_width (text_ptr)) / 2 : 0;
#endif
	while (c = *text_ptr) {
		if (c == '\n') {
			next_row = text_ptr + 1;
			y += FHEIGHT + 2;
			nTab = 0;
			break;
			}
		if (c == '\t') {
			text_ptr++;
			if (nTab < 6) {
				int	w, h, aw;

				GrGetStringSize (text_ptr, &w, &h, &aw);
				x = LHX (nTabs [nTab++]);
				if (!gameStates.multi.bSurfingNet)
					x += nMaxWidth - w;
				}
			continue;
			}
		letter = c - FMINCHAR;
		get_char_width (c, text_ptr [1], &cw, &spacing);
		if (!INFONT (letter) || (c <= 0x06)) {	//not in font, draw as space
			CHECK_EMBEDDED_COLORS()
		else {
			x += spacing;
			text_ptr++;
			}
		continue;
		}
		if (bHotKey = ((nKey < 0) && isalnum (c)) || (nKey && ((int) c == nKey)))
			nKey = 0;
		bmfP = (bHotKey && (FONT != SMALL_FONT)) ? SELECTED_FONT->ft_bitmaps + letter : FONT->ft_bitmaps + letter;
		palP = BM_PARENT (bmfP) ? BM_PARENT (bmfP)->bm_palette : bmfP->bm_palette;
		nChars++;
		i = nKeyColor * 3;
		kc.red = RGBA_RED (nKeyColor);
		kc.green = RGBA_GREEN (nKeyColor);
		kc.blue = RGBA_BLUE (nKeyColor);
		kc.alpha = 255;
		if (FFLAGS & FT_COLOR) {
			for (hy = 0; hy < bmfP->bm_props.h; hy++) {
				pc = ((grs_rgba *) bmP->bm_texBuf) + (y + hy) * w + x;
				pf = bmfP->bm_texBuf + hy * bmfP->bm_props.rowsize;
				for (hx = bmfP->bm_props.w; hx; hx--, pc++, pf++)
#if 0
					*pc = *pf;
#else
					if ((c = *pf) != TRANSPARENCY_COLOR) {
						i = c * 3;
						pc->red = palP [i] * 4;
						pc->green = palP [i + 1] * 4;
						pc->blue = palP [i + 2] * 4;
						pc->alpha = 255;
						}
#endif
				}
			}
		else {
			if (FG_COLOR.index < 0)
				memset (&hc, 0xff, sizeof (hc));
			else {
#if 1
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
#endif
				}
			for (hy = 0; hy < bmfP->bm_props.h; hy++) {
				pc = ((grs_rgba *) bmP->bm_texBuf) + (y + hy) * w + x;
				pf = bmfP->bm_texBuf + hy * bmfP->bm_props.rowsize;
				for (hx = bmfP->bm_props.w; hx; hx--, pc++, pf++)
					if (*pf != TRANSPARENCY_COLOR)
						*pc = bHotKey ? kc : hc;
				}
			}
		x += spacing;
		text_ptr++;
		}
	}
bmP->bm_palette = palP;
OglLoadBmTexture (bmP, 0, 0);
return bmP;
}

//------------------------------------------------------------------------------

int GrInternalColorString(int x, int y, char *s)
{
	return OglInternalString(x,y,s);
}
#endif //OGL

//------------------------------------------------------------------------------

int GrString(int x, int y, char *s)
{
	int w, h, aw;
	int clipped=0;

	Assert(FONT != NULL);

	if (x == 0x8000)	{
		if (y < 0)
			clipped |= 1;
		GrGetStringSize(s, &w, &h, &aw);
		// for x, since this will be centered, only look at
		// width.
		if (w > grdCurCanv->cv_bitmap.bm_props.w) 
			clipped |= 1;
		if (y > grdCurCanv->cv_bitmap.bm_props.h) 
			clipped |= 3;
		else if ((y+h) > grdCurCanv->cv_bitmap.bm_props.h)
			clipped |= 1;
		else if ((y+h) < 0) 
			clipped |= 2;
		}
	else {
		if ((x<0) || (y<0)) 
			clipped |= 1;
		GrGetStringSize(s, &w, &h, &aw);
		if (x > grdCurCanv->cv_bitmap.bm_props.w) 
			clipped |= 3;
		else if ((x+w) > grdCurCanv->cv_bitmap.bm_props.w) 
			clipped |= 1;
		else if ((x+w) < 0) 
			clipped |= 2;
		if (y > grdCurCanv->cv_bitmap.bm_props.h) 
			clipped |= 3;
		else if ((y+h) > grdCurCanv->cv_bitmap.bm_props.h) 
			clipped |= 1;
		else if ((y+h) < 0) 
			clipped |= 2;
	}

	if (!clipped)
		return GrUString(x, y, s);

	if (clipped & 2)	{
		// Completely clipped...
		return 0;
	}

	if (clipped & 1)	{
		// Partially clipped...
	}

	// Partially clipped...
#ifdef OGL
	if (TYPE==BM_OGL)
		return OglInternalString(x,y,s);
#endif

	if (FFLAGS & FT_COLOR)
		return GrInternalColorString(x, y, s);

	if (BG_COLOR.index == -1)
		return gr_internal_string_clipped_m(x, y, s);

	return gr_internal_string_clipped(x, y, s);
}

//------------------------------------------------------------------------------

int GrUString(int x, int y, char *s)
{
#ifdef OGL
	if (TYPE==BM_OGL)
		return OglInternalString(x,y,s);
#endif
	
	if (FFLAGS & FT_COLOR) {
		return GrInternalColorString(x,y,s);
	}
	else
		switch(TYPE)
		{
		case BM_LINEAR:
			if (BG_COLOR.index == -1)
				return GrInternalString0m(x,y,s);
			else
				return GrInternalString0(x,y,s);
#ifdef __MSDOS__
		case BM_SVGA:
			if (BG_COLOR.index == -1)
				return GrInternalString2m(x,y,s);
			else
				return GrInternalString2(x,y,s);
#endif // __MSDOS__
#if defined(POLY_ACC)
      case BM_LINEAR15:
			if (BG_COLOR.index == -1)
				return GrInternalString5m(x,y,s);
			else
				return GrInternalString5(x,y,s);
#endif
		}
	return 0;
}

//------------------------------------------------------------------------------

void GrGetStringSize(char *s, int *string_width, int *string_height, int *average_width)
{
	int i = 0, longest_width = 0, nTab = 0;
	int width,spacing;

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
				*string_height += *(s+1)-'0';
				s += 2;
				}
			else {
				get_char_width(s[0],s[1],&width,&spacing);

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

void GrGetStringSizeTabbed (char *s, int *string_width, int *string_height, int *average_width, 
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
	if (nTab) {
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

int _CDECL_ GrUPrintf(int x, int y, char * format, ...)
{
	char buffer[1000];
	va_list args;

	va_start(args, format);
	vsprintf(buffer,format,args);
	return GrUString(x, y, buffer);
}

//------------------------------------------------------------------------------

int _CDECL_ GrPrintF(int x, int y, char * format, ...)
{
	static char buffer[1000];
	va_list args;

	va_start(args, format);
	vsprintf(buffer,format,args);
	return GrString(x, y, buffer);
}

//------------------------------------------------------------------------------

void GrCloseFont(grs_font * font)
{
	if (font)
	{
		int fontnum;
		char * font_data;

		//find font in list
	for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=font;fontnum++)
		;
	Assert(fontnum<MAX_OPEN_FONTS);	//did we find slot?
	font_data = open_font[fontnum].dataptr;
	d_free(font_data);
	open_font[fontnum].ptr = NULL;
	open_font[fontnum].dataptr = NULL;
	if (font->ft_chars) {
		d_free(font->ft_chars);
		font->ft_chars = NULL;
		}
#ifdef OGL
	if (font->ft_bitmaps) {
		d_free(font->ft_bitmaps);
		font->ft_bitmaps = NULL;
		}
	GrFreeBitmapData(&font->ft_parent_bitmap);
//		OglFreeBmTexture(&font->ft_parent_bitmap);
#endif
	d_free(font);
	font = NULL;
	}
}

//------------------------------------------------------------------------------

void GrRemapMonoFonts()
{
	int fontnum;

	for (fontnum=0;fontnum<MAX_OPEN_FONTS;fontnum++) {
		grs_font *font;
		font = open_font[fontnum].ptr;
		if (font && !(font->ftFlags & FT_COLOR))
			GrRemapFont(font, open_font[fontnum].filename, open_font[fontnum].dataptr);
	}
}

//------------------------------------------------------------------------------
//remap (by re-reading) all the color fonts
void GrRemapColorFonts()
{
	int fontnum;

for (fontnum=0;fontnum<MAX_OPEN_FONTS;fontnum++) {
	grs_font *font;
	font = open_font[fontnum].ptr;
	if (font && (font->ftFlags & FT_COLOR))
		GrRemapFont(font, open_font[fontnum].filename, open_font[fontnum].dataptr);
	}
}

//------------------------------------------------------------------------------

#if 0//def FAST_FILE_IO
#define grs_font_read(gf, fp) CFRead(gf, GRS_FONT_SIZE, 1, fp)
#else
/*
 * reads a grs_font structure from a CFILE
 */
void grs_font_read(grs_font *gf, CFILE *fp)
{
	gf->ft_w = CFReadShort (fp);
	gf->ft_h = CFReadShort (fp);
	gf->ftFlags = CFReadShort (fp);
	gf->ft_baseline = CFReadShort (fp);
	gf->ft_minchar = CFReadByte (fp);
	gf->ft_maxchar = CFReadByte (fp);
	gf->ft_bytewidth = CFReadShort (fp);
	gf->ft_data = (ubyte *) ((long) CFReadInt (fp));
	gf->ft_chars = (ubyte **) ((long) CFReadInt (fp));
	gf->ft_widths = (short *) ((long) CFReadInt (fp));
	gf->ft_kerndata = (ubyte *) ((long) CFReadInt (fp));
}
#endif

//------------------------------------------------------------------------------

grs_font * GrInitFont(char * fontname)
{
	static int firstTime=1;
	grs_font *font;
	char *font_data;
	int i,fontnum;
	unsigned char * ptr;
	int nchars;
	CFILE *fontfile;
	char file_id[4];
	int datasize;	//size up to (but not including) palette
	int freq[256];

	if (firstTime) {
		int i;
		for (i=0;i<MAX_OPEN_FONTS;i++) {
			open_font[i].ptr = NULL;
			open_font[i].dataptr = NULL;
    }
		firstTime=0;
	}

	//find d_free font slot
	for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=NULL;fontnum++);
	Assert(fontnum<MAX_OPEN_FONTS);	//did we find one?

	strncpy(open_font[fontnum].filename,fontname,SHORT_FILENAME_LEN);

	fontfile = CFOpen(fontname, gameFolders.szDataDir, "rb", 0);

	if (!fontfile) {
#if TRACE
		con_printf(CON_VERBOSE, "Can't open font file %s\n", fontname);
#endif		
		return NULL;
	}

	CFRead(file_id, 4, 1, fontfile);
	if (!strncmp(file_id, "NFSP", 4)) {
#if TRACE		
		con_printf(CON_NORMAL, "File %s is not a font file\n", fontname);
#endif		
		return NULL;
	}

	datasize = CFReadInt(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	MALLOC(font, grs_font, sizeof(grs_font));
	grs_font_read(font, fontfile);

	MALLOC(font_data, char, datasize);
	CFRead(font_data, 1, datasize, fontfile);

	open_font[fontnum].ptr = font;
	open_font[fontnum].dataptr = font_data;

	// make these offsets relative to font_data
	font->ft_data = (ubyte *)((size_t)font->ft_data - GRS_FONT_SIZE);
	font->ft_widths = (short *)((size_t)font->ft_widths - GRS_FONT_SIZE);
	font->ft_kerndata = (ubyte *)((size_t)font->ft_kerndata - GRS_FONT_SIZE);

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ftFlags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = &font_data[(size_t)font->ft_data];
		font->ft_chars = (unsigned char **)d_malloc(nchars * sizeof(unsigned char *));

		ptr = font->ft_data;

		for (i=0; i< nchars; i++) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ftFlags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = font_data;
		font->ft_chars  = NULL;
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ftFlags & FT_KERNED)
		font->ft_kerndata = &font_data[(size_t)font->ft_kerndata];

	memset (&font->ft_parent_bitmap, 0, sizeof (font->ft_parent_bitmap));
	memset (freq, 0, sizeof (freq));
	if (font->ftFlags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		CFRead(palette,3,256,fontfile);		//read the palette

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

			for (i = 0; i < ptr-font->ft_data; i++) {
				if (font->ft_data[i] == 0)
					font->ft_data[i] = 255;
				else if (font->ft_data[i] == 255)
					font->ft_data[i] = 0;
			}

		}
#endif
		GrCountColors (font->ft_data, (int) (ptr - font->ft_data), freq);
		GrSetPalette (&font->ft_parent_bitmap, palette, TRANSPARENCY_COLOR, -1, freq);
		}
	else
		GrSetPalette (&font->ft_parent_bitmap, defaultPalette, TRANSPARENCY_COLOR, -1, freq);

	CFClose(fontfile);

	//set curcanv vars

	FONT = font;
	FG_COLOR.index = 0;
	BG_COLOR.index = 0;

	{
		int x,y,aw;
		char tests[]="abcdefghij1234.A";
		GrGetStringSize(tests,&x,&y,&aw);
//		newfont->ft_aw=x/(double)strlen(tests);
	}

#ifdef OGL
	ogl_init_font(font);
#endif

	return font;

}

//------------------------------------------------------------------------------

//remap a font by re-reading its data & palette
void GrRemapFont(grs_font *font, char * fontname, char *font_data)
{
	int i;
	int nchars;
	CFILE *fontfile;
	char file_id[4];
	int datasize;        //size up to (but not including) palette
	unsigned char *ptr;

//	if (! (font->ftFlags & FT_COLOR))
//		return;
	fontfile = CFOpen(fontname, gameFolders.szDataDir, "rb", 0);
//	CBRK (!strcmp (fontname, "font2-2h.fnt"));
	if (!fontfile)
		Error(TXT_FONT_FILE, fontname);
	CFRead(file_id, 4, 1, fontfile);
	if (!strncmp(file_id, "NFSP", 4))
		Error(TXT_FONT_FILETYPE, fontname);
	datasize = CFReadInt(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.
	d_free(font->ft_chars);
	grs_font_read(font, fontfile); // have to reread in case mission hogfile overrides font.
	CFRead(font_data, 1, datasize, fontfile);  //read raw data
	// make these offsets relative to font_data
	font->ft_data = (ubyte *)((size_t)font->ft_data - GRS_FONT_SIZE);
	font->ft_widths = (short *)((size_t)font->ft_widths - GRS_FONT_SIZE);
	font->ft_kerndata = (ubyte *)((size_t)font->ft_kerndata - GRS_FONT_SIZE);
	nchars = font->ft_maxchar - font->ft_minchar + 1;
	if (font->ftFlags & FT_PROPORTIONAL) {
		font->ft_widths = (short *) (font_data + (size_t)font->ft_widths);
		font->ft_data = font_data + (size_t)font->ft_data;
		font->ft_chars = (unsigned char **)d_malloc(nchars * sizeof(unsigned char *));
		ptr = font->ft_data;
		for (i=0; i< nchars; i++) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ftFlags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
			}
		}
	else  {
		font->ft_data   = font_data;
		font->ft_chars  = NULL;
		font->ft_widths = NULL;
		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
		}
	if (font->ftFlags & FT_KERNED)
		font->ft_kerndata = &font_data[(size_t)font->ft_kerndata];
	if (font->ftFlags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		int freq[256];
		CFRead(palette,3,256,fontfile);		//read the palette
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
			for (i = 0; i < ptr-font->ft_data; i++) {
				if (font->ft_data[i] == 0)
					font->ft_data[i] = 255;
				else if (font->ft_data[i] == 255)
					font->ft_data[i] = 0;
			}
		}
#endif
		memset (freq, 0, sizeof (freq));
		GrCountColors (font->ft_data, (int) (ptr - font->ft_data), freq);
		GrSetPalette (&font->ft_parent_bitmap, palette, TRANSPARENCY_COLOR, -1, freq);
	}
	CFClose(fontfile);
#ifdef OGL
	if (font->ft_bitmaps) {
		d_free(font->ft_bitmaps);
		font->ft_bitmaps = NULL;
		}
	GrFreeBitmapData(&font->ft_parent_bitmap);
//	OglFreeBmTexture(&font->ft_parent_bitmap);
	ogl_init_font(font);
#endif
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

void GrSetFontColorRGB (grs_rgba *fg, grs_rgba *bg)
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
	BG_COLOR.color.green = RGBA_GREEN (fg);
	BG_COLOR.color.blue = RGBA_BLUE (fg);
	BG_COLOR.color.alpha = RGBA_ALPHA (fg);
	}
}

//------------------------------------------------------------------------------

void GrSetCurFont(grs_font * newFont)
{
	FONT = newFont;
}


//------------------------------------------------------------------------------

int gr_internal_string_clipped(int x, int y, char *s)
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

  bits=0;

	next_row = s;
	FG_COLOR.rgb = 0;
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = GetCenteredX(text_ptr1);

		last_x = x;

		for (r=0; r<FHEIGHT; r++)	{
			text_ptr = text_ptr1;
			x = last_x;

			while (*text_ptr)	{
				if (*text_ptr == '\n')	{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR.index = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE)	{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)	{
					for (i=0; i< width; i++)	{
						GrSetColor(FG_COLOR.index);
						gr_pixel(x++, y);
					}
				} else {
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++)	{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)
							GrSetColor(FG_COLOR.index);
						else
							GrSetColor(BG_COLOR.index);
						gr_pixel(x++, y);
						BitMask >>= 1;
					}
				}

				x += spacing-width;		//for kerning

				text_ptr++;
			}
			y++;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------

int gr_internal_string_clipped_m(int x, int y, char *s)
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

  bits=0;

	next_row = s;
	FG_COLOR.rgb = 0;
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = GetCenteredX(text_ptr1);

		last_x = x;

		for (r=0; r<FHEIGHT; r++)	{
			x = last_x;

			text_ptr = text_ptr1;

			while (*text_ptr)	{
				if (*text_ptr == '\n')	{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR.index = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE)	{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)	{
					for (i=0; i< width; i++)	{
						GrSetColor(FG_COLOR.index);
						gr_pixel(x++, y);
					}
				} else {
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++)	{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)	{
							GrSetColor(FG_COLOR.index);
							gr_pixel(x++, y);
						} else {
							x++;
						}
						BitMask >>= 1;
					}
				}

				x += spacing-width;		//for kerning

				text_ptr++;
			}
			y++;
		}
	}
	return 0;
}
