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
#include "palette.h"
#include "bitmap.h"
#include "gamefont.h"

//-----------------------------------------------------------------------------

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

#define FONT			CCanvas::Current ()->Font ()
#define FG_COLOR		CCanvas::Current ()->FontColor (0)
#define BG_COLOR		CCanvas::Current ()->FontColor (1)
#define FWIDTH       FONT->Width ()
#define FHEIGHT      FONT->Height ()
#define FBASELINE    FONT->baseLine
#define FFLAGS       FONT->flags
#define FMINCHAR     FONT->minChar
#define FMAXCHAR     FONT->maxChar
#define FDATA        FONT->data
#define FCHARS       FONT->chars
#define FWIDTHS      FONT->Width ()s

#define BITS_TO_BYTES(x) (((x)+7)>>3)

//-----------------------------------------------------------------------------

//font structure
typedef struct tFont {
	short       width;       // Width in pixels
	short       height;      // Height in pixels
	short       flags;       // Proportional?
	short       baseLine;    //
	ubyte       minChar;     // First char defined by this font
	ubyte       maxChar;     // Last char defined by this font
	short       byteWidth;   // Width in unsigned chars
	ubyte     	*data;       // Ptr to raw data.
	ubyte    	**chars;     // Ptrs to data for each char (required for prop font)
	short     	*widths;     // Array of widths (required for prop font)
	ubyte     	*kernData;   // Array of kerning triplet data
	// These fields do not participate in disk i/o!
	CBitmap 		*bitmaps;
	CBitmap 		parentBitmap;
} tFont;

class CFont {
	private:
		tFont		m_info;

	public:
		CFont () { Init (); };
		~CFont () { Destroy (); }
		void Init (void) { memset (&m_info, 0, sizeof (m_info)); }
		void Destroy (void);
		ubyte* Load (const char *fontname, ubyte* fontData = NULL);
		void Read (CFile& cf);
		void StringSize (const char *s, int& stringWidth, int& stringHeight, int& averageWidth);
		void StringSizeTabbed (const char *s, int& stringWidth, int& stringHeight, int& averageWidth, int *nTabs, int nMaxWidth);

		inline short Width (void) { return m_info.width; }
		inline short Height (void) { return m_info.height; }
		inline short Flags (void) { return m_info.flags; }
		inline short BaseLine (void) { return m_info.baseLine; }
		inline ubyte MinChar (void) { return m_info.minChar; } 
		inline ubyte MaxChar (void) { return m_info.maxChar; } 
		inline short ByteWidth (void) { return m_info.byteWidth; }
		inline ubyte* Data (void) { return m_info.data; } 
		inline ubyte**	Chars (void) { return m_info.chars; }
		inline short* Widths (void) { return m_info.widths; }
		inline ubyte* KernData (void) { return m_info.kernData; } 
		inline CBitmap* Bitmaps (void) { return m_info.bitmaps; }
		inline CBitmap& ParentBitmap (void) { return m_info.parentBitmap; }

		inline void GetWidth (short width) { m_info.width = width; }
		inline void GetHeight (short height) { m_info.height = height; }
		inline void GetFlags (short flags) { m_info.flags = flags; }
		inline void GetBaseLine (short baseLine) { m_info.baseLine = baseLine; }
		inline void GetMinChar (ubyte minChar) { m_info.minChar = minChar; } 
		inline void GetMaxChar (ubyte maxChar) { m_info.maxChar = maxChar; } 
		inline void GetByteWidth (short byteWidth) { m_info.byteWidth = byteWidth; }
		inline void GetData (ubyte* data) { m_info.data = data; } 
		inline void GetChars (ubyte** chars) { m_info.chars = chars; }
		inline void GetWidths (short* widths) { m_info.widths = widths; }
		inline void KernData (ubyte* kernData) { m_info.kernData = kernData; } 
		inline void SetBitmaps (CBitmap* bitmaps) { m_info.bitmaps = bitmaps; }
		inline void SetParentBitmap (CBitmap& parent) { m_info.parentBitmap = parent; }
		inline void GetInfo (tFont& info) { info = m_info; }

		inline bool InFont (char c) { return (c >= 0) && (c <= (char) (m_info.maxChar - m_info.minChar)); }
		inline int Range (void) { return m_info.maxChar - m_info.minChar + 1; }

		void GetCharWidth (ubyte c, ubyte c2, int& width, int& spacing);
		int GetLineWidth (const char *s);
		int GetCenteredX (const char *s);
		int TotalWidth (void);

	private:
		ubyte *FindKernEntry (ubyte first, ubyte second);
		void ChooseSize (int gap, int& rw, int& rh);
		void Setup (const char *fontname, ubyte* fontData, CPalette& palette);
		void Create (const char *fontname);
};

//-----------------------------------------------------------------------------

#define MAX_OPEN_FONTS	50

typedef struct tOpenFont {
	char		filename [SHORT_FILENAME_LEN];
	CFont		font;
	ubyte*	data;
} tOpenFont;

class CFontManager {
	private:
		tOpenFont	m_fonts [MAX_OPEN_FONTS];
		CFont			*m_gameFonts [MAX_FONTS];
		CFont			*m_current;
		CFont			*m_save [10];
		int			m_tos;

	public:
		CFontManager () { Init (); }
		~CFontManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		CFont* Load (const char* fontname);
		void Close (CFont* font);
		inline CFont* Current (void) { return m_current; }
		inline CFont* GameFont (int i) { return ((i >= 0) && (i < MAX_FONTS)) ? m_gameFonts [i] : NULL; }
		inline void SetCurrent (CFont* fontP) { m_current = fontP; }
		void SetColor (int fgColor, int bgColor);
		void SetColorRGB (tRgbaColorb *fgColor, tRgbaColorb *bgColor);
		void SetColorRGBi (unsigned int fgColor, int bSetFG, unsigned int bgColor, int bSetBG);
		void Push (void) { if (m_tos < 10) m_save [m_tos++] = m_current; }
		void Pop (void) { if (m_tos > 0) m_current = m_save [--m_tos]; }
		void RemapColor ();
		void RemapMono ();
	};

extern CFontManager fontManager;

//-----------------------------------------------------------------------------

int GrString (int x, int y, const char *s, int *idP);
int GrUString (int x, int y, const char *s);
int _CDECL_ GrPrintF (int *idP, int x, int y, const char * format, ...);
int _CDECL_ GrUPrintf (int x, int y, const char * format, ...);
CBitmap *CreateStringBitmap (const char *s, int nKey, unsigned int nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce);

//-----------------------------------------------------------------------------
// Global variables
//flags for fonts

typedef struct grsString {
	char				*pszText;
	CBitmap			*bmP;
	int				*pId;
	short				nWidth;
	short				nLength;
	} grsString;

void InitStringPool (void);
void FreeStringPool (void);

//-----------------------------------------------------------------------------
#endif //_FONT_H
