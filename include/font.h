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
	int16_t					width;      // Width in pixels
	int16_t					height;     // Height in pixels
	int16_t					flags;      // Proportional?
	int16_t					baseLine;   //
	uint8_t					minChar;    // First char defined by this font
	uint8_t					maxChar;    // Last char defined by this font
	int16_t					byteWidth;  // Width in unsigned chars
	uint8_t*					data;       // Ptr to raw data.
	CArray<uint8_t*>		chars;		// Ptrs to data for each char (required for prop font)
	int16_t*					widths;     // Array of widths (required for prop font)
	uint8_t*					kernData;   // Array of kerning triplet data
	int32_t					dataOffs;
	int32_t					widthOffs;
	int32_t					kernDataOffs;
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
		uint8_t* Remap (const char *fontname, uint8_t* fontData);
		uint8_t* Load (const char *fontname, uint8_t* fontData = NULL);
		void Read (CFile& cf);
		int32_t StringSize (const char *s, int32_t& stringWidth, int32_t& stringHeight, int32_t& averageWidth);
		int32_t StringSizeTabbed (const char *s, int32_t& stringWidth, int32_t& stringHeight, int32_t& averageWidth, int32_t *nTabs, int32_t nMaxWidth = 0);

		int16_t Width (void);
		int16_t Height (void);
		inline int16_t Flags (void) { return m_info.flags; }
		inline int16_t BaseLine (void) { return m_info.baseLine; }
		inline uint8_t MinChar (void) { return m_info.minChar; }
		inline uint8_t MaxChar (void) { return m_info.maxChar; }
		inline int16_t ByteWidth (void) { return m_info.byteWidth; }
		inline uint8_t* Data (void) { return m_info.data; }
		inline uint8_t**	Chars (void) { return m_info.chars.Buffer (); }
		inline int16_t* Widths (void) { return m_info.widths; }
		inline uint8_t* KernData (void) { return m_info.kernData; }
		inline CBitmap* Bitmaps (void) { return m_info.bitmaps.Buffer (); }
		inline CBitmap& ParentBitmap (void) { return m_info.parentBitmap; }

		inline void GetWidth (int16_t width) { m_info.width = width; }
		inline void GetHeight (int16_t height) { m_info.height = height; }
		inline void GetFlags (int16_t flags) { m_info.flags = flags; }
		inline void GetBaseLine (int16_t baseLine) { m_info.baseLine = baseLine; }
		inline void GetMinChar (uint8_t minChar) { m_info.minChar = minChar; }
		inline void GetMaxChar (uint8_t maxChar) { m_info.maxChar = maxChar; }
		inline void GetByteWidth (int16_t byteWidth) { m_info.byteWidth = byteWidth; }
		inline void GetData (uint8_t* data) { m_info.data = data; }
		inline void GetChars (uint8_t** chars) { m_info.chars = chars; }
		inline void GetWidths (int16_t* widths) { m_info.widths = widths; }
		inline void KernData (uint8_t* kernData) { m_info.kernData = kernData; }
		inline void SetBitmaps (CBitmap* bitmaps) { m_info.bitmaps = bitmaps; }
		inline void SetParentBitmap (CBitmap& parent) { m_info.parentBitmap = parent; }
		inline void SetBuffer (uint8_t* buffer) { m_info.parentBitmap.SetBuffer (buffer); }

		inline void GetInfo (tFont& info) { memcpy (&info, &m_info, sizeof (info)); }

		inline bool InFont (char c) { return (c >= 0) && (c <= (char) (m_info.maxChar - m_info.minChar)); }
		inline int32_t Range (void) { return m_info.maxChar - m_info.minChar + 1; }

		void GetCharWidth (uint8_t c, uint8_t c2, int32_t& width, int32_t& spacing);
		inline int32_t GetCharWidth (char* psz) {
			int32_t w, s;
			GetCharWidth (*psz, *(psz + 1), w, s);
			return s;
			}
		int32_t GetLineWidth (const char *s, float scale = 1.0f);
		int32_t GetCenteredX (const char *s, float scale = 1.0f);
		int32_t TotalWidth (float scale = 1.0f);

		inline int32_t Scaled (int32_t v, float scale) { return int32_t (ceil (float (v) * scale)); }

		int32_t DrawString (int32_t x, int32_t y, const char *s);
		char* PadString (char* pszDest, const char* pszText, const char* pszFiller, int32_t nLength);

	private:
		uint8_t *FindKernEntry (uint8_t first, uint8_t second);
		void ChooseSize (int32_t gap, int32_t& rw, int32_t& rh);
		void Setup (const char *fontname, uint8_t* fontData, CPalette& palette);
		void Create (const char *fontname);
};

//-----------------------------------------------------------------------------

#define MAX_OPEN_FONTS	50

typedef struct tOpenFont {
	char		filename [SHORT_FILENAME_LEN];
	CFont		font;
	uint8_t*	data;
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
		inline CFont* GameFont (int32_t i) { return ((i >= 0) && (i < MAX_FONTS)) ? m_gameFonts [i] : NULL; }
		float SetScale (float fScale);
		float Scale (bool bScaled = true);
		inline void PushScale (void) { m_scaleStack.Push (m_scale); }
		inline void PopScale (void) { 
			if (m_scaleStack.ToS ()) 
				m_scale = m_scaleStack.Pop (); 
			}
		void SetCurrent (CFont* fontP);
		void SetColor (int32_t fgColor, int32_t bgColor);
		void SetColorRGB (CRGBAColor *fgColor, CRGBAColor *bgColor);
		void SetColorRGBi (uint32_t fgColor, int32_t bSetFG, uint32_t bgColor, int32_t bSetBG);
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
		inline int32_t ToS (void) { return m_save.ToS (); }
		inline void Reset (void) { 
			m_save.Truncate (1); 
			m_current = m_save.ToS () ? m_save.Top ()->m_font : NULL; 
			}
		void Remap ();

		inline int32_t Scaled (int32_t v) { return (int32_t) FRound ((float (v) * Scale ())); }
		inline int32_t Unscaled (int32_t v) { return (int32_t) FRound ((float (v) / Scale ())); }
	};

extern CFontManager fontManager;

//-----------------------------------------------------------------------------

int32_t GrString (int32_t x, int32_t y, const char *s, int32_t *idP = NULL);
int32_t GrUString (int32_t x, int32_t y, const char *s);
int32_t _CDECL_ GrPrintF (int32_t *idP, int32_t x, int32_t y, const char * format, ...);
int32_t _CDECL_ GrUPrintf (int32_t x, int32_t y, const char * format, ...);
CBitmap *CreateStringBitmap (const char *s, int32_t nKey, uint32_t nKeyColor, int32_t *nTabs, int32_t bCentered, int32_t nMaxWidth, int32_t nRowPad, int32_t bForce);
void DrawCenteredText (int32_t y, char * s);
int32_t StringWidth (char * s, int32_t n = 0);
int32_t CenteredStringPos (char* s);

//-----------------------------------------------------------------------------
// Global variables
//flags for fonts

typedef struct grsString {
	char				*pszText;
	CBitmap			*bmP;
	int32_t				*pId;
	int16_t				nWidth;
	int16_t				nLength;
	} grsString;

void InitStringPool (void);
void FreeStringPool (void);

//-----------------------------------------------------------------------------
#endif //_FONT_H
