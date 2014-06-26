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

//#error ++++++++++++++++++++ FONT_H ++++++++++++++++++++++

#include "pstypes.h"
#include "cstack.h"
#include "bitmap.h"
#include "palette.h"
#include "gamefont.h"

//-----------------------------------------------------------------------------

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

#define BITS_TO_BYTES(x) (((x)+7)>>3)

//-----------------------------------------------------------------------------

//font structure
typedef struct tFont {
	short					width;      // Width in pixels
	short					height;     // Height in pixels
	short					flags;      // Proportional?
	short					baseLine;   //
	ubyte					minChar;    // First char defined by this font
	ubyte					maxChar;    // Last char defined by this font
	short					byteWidth;  // Width in unsigned chars
	ubyte*				data;       // Ptr to raw data.
	CArray<ubyte*>		chars;		// Ptrs to data for each char (required for prop font)
	short*				widths;     // Array of widths (required for prop font)
	ubyte*				kernData;   // Array of kerning triplet data
	int					dataOffs;
	int					widthOffs;
	int					kernDataOffs;
	// These fields do not participate in disk i/o!
	CArray<CBitmap> 	bitmaps;
	CBitmap 				parentBitmap;
} tFont;

class CFont {
	private:
		tFont		m_info;

	public:
		CFont () { Init (); }
		~CFont () { Destroy (); }
		void Init (void);
		void Destroy (void);
		ubyte* Remap (const char *fontname, ubyte* fontData);
		ubyte* Load (const char *fontname, ubyte* fontData = NULL);
		void Read (CFile& cf);
		void StringSize (const char *s, int& stringWidth, int& stringHeight, int& averageWidth);
		void StringSizeTabbed (const char *s, int& stringWidth, int& stringHeight, int& averageWidth, int *nTabs, int nMaxWidth = 0);

		short Width (void);
		short Height (void);
		inline short Flags (void) { return m_info.flags; }
		inline short BaseLine (void) { return m_info.baseLine; }
		inline ubyte MinChar (void) { return m_info.minChar; }
		inline ubyte MaxChar (void) { return m_info.maxChar; }
		inline short ByteWidth (void) { return m_info.byteWidth; }
		inline ubyte* Data (void) { return m_info.data; }
		inline ubyte**	Chars (void) { return m_info.chars.Buffer (); }
		inline short* Widths (void) { return m_info.widths; }
		inline ubyte* KernData (void) { return m_info.kernData; }
		inline CBitmap* Bitmaps (void) { return m_info.bitmaps.Buffer (); }
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
		inline void SetBuffer (ubyte* buffer) { m_info.parentBitmap.SetBuffer (buffer); }

		inline void GetInfo (tFont& info) { memcpy (&info, &m_info, sizeof (info)); }

		inline bool InFont (char c) { return (c >= 0) && (c <= (char) (m_info.maxChar - m_info.minChar)); }
		inline int Range (void) { return m_info.maxChar - m_info.minChar + 1; }

		void GetCharWidth (ubyte c, ubyte c2, int& width, int& spacing);
		int GetLineWidth (const char *s, float scale = 1.0f);
		int GetCenteredX (const char *s, float scale = 1.0f);
		int TotalWidth (float scale = 1.0f);

		inline int Scaled (int v, float scale) { return int (ceil (float (v) * scale)); }

		int DrawString (int x, int y, const char *s);
		char* PadString (char* pszDest, const char* pszText, const char* pszFiller, int nLength);

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

typedef struct tFontBackup {
	CFont*	m_font;
	float		m_scale;
#if DBG
	char		m_szId [100];
#endif
} tFontBackup;

class CFontManager {
	private:
		tOpenFont				m_fonts [MAX_OPEN_FONTS];
		CFont*					m_gameFonts [MAX_FONTS];
		CFont*					m_current;
		CStack<tFontBackup>	m_save;
		float						m_scale;
		CStack<float>			m_scaleStack;

	public:
		CFontManager () { Init (); }
		~CFontManager () { Destroy (); }
		void Init (void);
		void Setup (void);
		void Destroy (void);
		CFont* Load (const char* fontname);
		void Unload (CFont* font);
		inline CFont* Current (void) { return m_current; }
		inline CFont* GameFont (int i) { return ((i >= 0) && (i < MAX_FONTS)) ? m_gameFonts [i] : NULL; }
		float SetScale (float fScale);
		float Scale (void);
		inline void PushScale (void) { m_scaleStack.Push (m_scale); }
		inline void PopScale (void) { 
			if (m_scaleStack.ToS ()) 
				m_scale = m_scaleStack.Pop (); 
			}
		void SetCurrent (CFont* fontP);
		void SetColor (int fgColor, int bgColor);
		void SetColorRGB (CRGBAColor *fgColor, CRGBAColor *bgColor);
		void SetColorRGBi (uint fgColor, int bSetFG, uint bgColor, int bSetBG);
		void Push (const char* pszId) { 
			if (m_current && m_save.Grow ()) {
				m_save.Top ()->m_font = m_current;
				m_save.Top ()->m_scale = m_scale;
#if DBG
				if (pszId)
					strncpy (m_save.Top ()->m_szId, pszId, sizeof (m_save.Top ()->m_szId));
				else
					*m_save.Top ()->m_szId = '\0';
#endif
				}
			}
		void Pop (void) { 
			if (ToS ()) {
				m_current = m_save.Top ()->m_font; 
				m_scale = m_save.Pop ().m_scale; 
				}
			}
		inline int ToS (void) { return m_save.ToS (); }
		inline void Reset (void) { 
			m_save.Truncate (1); 
			m_current = m_save.ToS () ? m_save.Top ()->m_font : NULL; 
			}
		void Remap ();

		inline int Scaled (int v) { return (int) FRound ((float (v) * Scale ())); }
		inline int Unscaled (int v) { return (int) FRound ((float (v) / Scale ())); }
	};

extern CFontManager fontManager;

//-----------------------------------------------------------------------------

int GrString (int x, int y, const char *s, int *idP = NULL);
int GrUString (int x, int y, const char *s);
int _CDECL_ GrPrintF (int *idP, int x, int y, const char * format, ...);
int _CDECL_ GrUPrintf (int x, int y, const char * format, ...);
CBitmap *CreateStringBitmap (const char *s, int nKey, uint nKeyColor, int *nTabs, int bCentered, int nMaxWidth, int bForce);
void DrawCenteredText (int y, char * s);
int StringWidth (char * s, int n = 0);
int CenteredStringPos (char* s);

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
