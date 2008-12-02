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

#ifndef _FONT_H
#define _FONT_H

#include "pstypes.h"
#include "fix.h"
#include "palette.h"
#include "bitmap.h"
#include "vecmat.h"

//-----------------------------------------------------------------------------

//font structure
typedef struct tFont {
	short       ftWidth;       // Width in pixels
	short       ftHeight;      // Height in pixels
	short       ftFlags;       // Proportional?
	short       ftBaseLine;    //
	ubyte       ftMinChar;     // First char defined by this font
	ubyte       ftMaxChar;     // Last char defined by this font
	short       ftByteWidth;   // Width in unsigned chars
	ubyte     	*ftData;       // Ptr to raw data.
	ubyte    	**ftChars;     // Ptrs to data for each char (required for prop font)
	short     	*ftWidths;     // Array of widths (required for prop font)
	ubyte     	*ftKernData;   // Array of kerning triplet data
	// These fields do not participate in disk i/o!
	CBitmap 		*ftBitmaps;
	CBitmap 		ftParentBitmap;
} __pack__ tFont;

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

#define FONT			CCanvas::Current ()->Font ()
#define FG_COLOR		CCanvas::Current ()->FontColor (0)
#define BG_COLOR		CCanvas::Current ()->FontColor (1)
#define FWIDTH       FONT->ftWidth
#define FHEIGHT      FONT->ftHeight
#define FBASELINE    FONT->ftBaseLine
#define FFLAGS       FONT->ftFlags
#define FMINCHAR     FONT->ftMinChar
#define FMAXCHAR     FONT->ftMaxChar
#define FDATA        FONT->ftData
#define FCHARS       FONT->ftChars
#define FWIDTHS      FONT->ftWidths

// Reads in a font file... current font set to this one.
tFont * GrInitFont(const char *fontfile);
void GrCloseFont(tFont * font);

//remap a font, re-reading its data & palette
void GrRemapFont(tFont *font, char * fontname, char *font_data);

//remap (by re-reading) all the color fonts
void GrRemapColorFonts();
void GrRemapMonoFonts();

// Writes a string using current font. Returns the next column after last char.
void SetFontColor(int fg, int bg);
void SetFontColorRGB (tRgbaColorb *fg, tRgbaColorb *bg);
void SetFontColorRGBi (unsigned int fg, int bSetFG, unsigned int bg, int bSetBG);
void GrSetCurFont (tFont * fontP);
int GrString (int x, int y, const char *s, int *idP);
int GrUString (int x, int y, const char *s);
int _CDECL_ GrPrintF (int *idP, int x, int y, const char * format, ...);
int _CDECL_ GrUPrintf (int x, int y, const char * format, ...);
void GrGetStringSize(const char *s, int *string_width, int *string_height, int *average_width);
void GrGetStringSizeTabbed (const char *s, int *string_width, int *string_height, int *average_width, int *nTabs, int nMaxWidth);
CBitmap *CreateStringBitmap (const char *s, int nKey, unsigned int nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce);
int GetCenteredX (const char *s);
//===========================================================================
// Global variables
//flags for fonts
#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4


void InitStringPool (void);
void FreeStringPool (void);


typedef struct grsString {
	char				*pszText;
	CBitmap			*bmP;
	int				*pId;
	short				nWidth;
	short				nLength;
	} grsString;

#endif //_FONT_H
