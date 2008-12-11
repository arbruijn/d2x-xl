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

#ifndef _CANVAS_H
#define _CANVAS_H

#include "pstypes.h"
#include "fix.h"
#include "palette.h"
#include "bitmap.h"
#include "font.h"
#include "cstack.h"

//-----------------------------------------------------------------------------

#define GWIDTH  screen.Current ()->Bitmap ().Width ()
#define GHEIGHT screen.Current ()->Bitmap ().Height ()
#define SWIDTH  screen.Width ()
#define SHEIGHT screen.Height ()

//these are control characters that have special meaning in the font code

#define CC_COLOR        1   //next char is new foreground color
#define CC_LSPACING     2   //next char specifies line spacing
#define CC_UNDERLINE    3   //next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S      "\x1"   //next char is new foreground color
#define CC_LSPACING_S   "\x2"   //next char specifies line spacing
#define CC_UNDERLINE_S  "\x3"   //next char is underlined

//-----------------------------------------------------------------------------

class CCanvas;
class CScreen;

typedef struct tCanvasColor {
	short       index;       // current color
	ubyte			rgb;
	tRgbaColorb	color;
} tCanvasColor;

typedef struct tCanvas {
	CBitmap			bm;					// the bitmap for this canvas
	tCanvasColor	color;
	CFont				*font;				// the currently selected font
	tCanvasColor	fontColors [2];   // current font background color (-1==Invisible)
	short				nDrawMode;			// fill, XOR, etc.
} tCanvas;

class CCanvas {
	private:
		tCanvas	m_info;

		static CCanvas*			m_current;
		static CStack<CCanvas*> m_save;

	public:
		CCanvas () { Init (); }
		~CCanvas () {}

		static CCanvas* Create (int w, int h);
		void Init (void);
		void Init (int nType, int w, int h, ubyte *data);
		void Setup (int w, int h);
		CCanvas* CreatePane (int x, int y, int w, int h);
		void SetupPane (CCanvas *paneP, int x, int y, int w, int h);
		void Destroy (void);
		void DestroyPane (void);
		void Clear (void);

		inline CBitmap& Bitmap (void) { return m_info.bm; }
		inline tCanvasColor& Color (void) { return m_info.color; }
		inline tCanvasColor& FontColor (int i) { return m_info.fontColors [i]; }
		inline CFont* Font (void) { return m_info.font; }
		inline short DrawMode (void) { return m_info.nDrawMode; }

		inline void SetBitmap (CBitmap& bm) { m_info.bm = bm; }
		inline void SetColor (tCanvasColor& color) { m_info.color = color; }
		inline void SetFontColor (tCanvasColor& color, int i) { m_info.fontColors [i] = color; }
		inline void SetFont (CFont *font) { m_info.font = font; }
		inline void SetDrawMode (short nDrawMode) { m_info.nDrawMode = nDrawMode; }

		static CCanvas* Current (void) { return m_current; }
		static void SetCurrent (CCanvas* canvP = NULL);
		static void Push (void) { m_save.Push (m_current); }
		static void Pop (void) { 
			m_current = m_save.Pop (); 
			fontManager.Pop ();
			}

		void Clear (uint color);
		void SetColor (int color);
		void SetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha);
		void SetColorRGB15bpp (ushort c, ubyte alpha);
		inline void SetColorRGBi (int i) { SetColorRGB (RGBA_RED (i), RGBA_GREEN (i), RGBA_BLUE (i), 255); }

		void FadeColorRGB (double dFade);

		inline short Width (void) { return m_info.bm.Width (); }
		inline short Height (void) { return m_info.bm.Height (); }
		inline short Left (void) { return m_info.bm.Left (); }
		inline short Top (void) { return m_info.bm.Top (); }
		inline short RowSize (void) { return m_info.bm.RowSize (); }
		inline short Mode (void) { return m_info.bm.Mode (); }
		inline ubyte* Buffer (void) { return m_info.bm.Buffer (); }
};

//===========================================================================

typedef struct tScreen {		// This is a video screen
	CCanvas  	canvas;			// Represents the entire screen
	u_int32_t	mode;				// Video mode number
	short   		width, height; // Actual Width and Height
	fix     		aspect;			//aspect ratio (w/h) for this screen
} tScreen;


class CScreen {
	private:
		tScreen	m_info;

		static CScreen* m_current;

	public:
		CScreen () { Init (); }
		~CScreen () { Destroy (); }

		void Init (void) { memset (&m_info, 0, sizeof (m_info)); }
		void Destroy (void) { m_info.canvas.Bitmap ().Destroy (); };

		inline CCanvas* Canvas (void) { return &m_info.canvas; }
		inline u_int32_t Mode (void) { return m_info.mode; }
		inline short Width (void) { return m_info.width; }
		inline short Height (void) { return m_info.height; }
		inline fix Aspect (void) { return m_info.aspect; }

		inline void SetCanvas (CCanvas* canvas) { m_info.canvas = *canvas; }
		inline void SetMode (u_int32_t mode) { m_info.mode = mode; }
		inline void SetWidth (short width) { m_info.width = width; }
		inline void SetHeight (short height) { m_info.height = height; }
		inline void SetAspect (fix aspect) { m_info.aspect = aspect; }

		inline CBitmap& Bitmap (void) { return Canvas ()->Bitmap (); }
		
		static CScreen* Current (void) { return m_current; }
};

extern CScreen screen;

//===========================================================================

#endif //_CANVAS_H
