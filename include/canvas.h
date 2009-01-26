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

typedef struct tCanvas {
	tCanvasColor	color;
	CFont				*font;				// the currently selected font
	tCanvasColor	fontColors [2];   // current font background color (-1==Invisible)
	short				nDrawMode;			// fill, XOR, etc.
} tCanvas;

class CCanvas : public CBitmap {
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

		inline tCanvasColor& Color (void) { return m_info.color; }
		inline tCanvasColor& FontColor (int i) { return m_info.fontColors [i]; }
		inline CFont* Font (void) { return m_info.font; }
		inline short DrawMode (void) { return m_info.nDrawMode; }

		inline void SetColor (tCanvasColor& color) { m_info.color = color; }
		inline void SetFontColor (tCanvasColor& color, int i) { m_info.fontColors [i] = color; }
		inline void SetFont (CFont *font) { m_info.font = font; }
		inline void SetDrawMode (short nDrawMode) { m_info.nDrawMode = nDrawMode; }

		static CCanvas* Current (void) { return m_current; }
		static void SetCurrent (CCanvas* canvP = NULL);
		static void Push (void) { 
			m_save.Push (m_current); 
			fontManager.Push ();
			}
		static void Pop (void) { 
			m_current = m_save.Pop (); 
			fontManager.Pop ();
			}

		void Clear (uint color);
		void SetColor (int color);
		void SetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha);
		void SetColorRGB15bpp (ushort c, ubyte alpha);
		inline void SetColorRGBi (int i) { SetColorRGB (RGBA_RED (i), RGBA_GREEN (i), RGBA_BLUE (i), 255); }

		inline bool Clip (int x, int y) { return this->CBitmap::Clip (x, y); }

		void FadeColorRGB (double dFade);
};

//===========================================================================

typedef struct tScreen {		// This is a video screen
	CCanvas  	canvas;			// Represents the entire screen
	u_int32_t	mode;				// Video mode number
	short   		width, height; // Actual Width and Height
	fix     		aspect;			//aspect ratio (w/h) for this screen
	float			scale [2];		//size ratio compared to 640x480
} tScreen;


class CScreen {
	private:
		tScreen	m_info;

		static CScreen* m_current;

	public:
		CScreen () { Init (); }
		~CScreen () { Destroy (); }

		void Init (void) { 
			m_info.mode = 0; 
			m_info.width = 0;
			m_info.height = 0;
			m_info.aspect = 0;
			if (!m_current)
				m_current = this;
			CCanvas::SetCurrent ();
			}
		void Destroy (void) { Canvas ()->CBitmap::Destroy (); };

		inline CCanvas* Canvas (void) { return &m_info.canvas; }
		inline u_int32_t Mode (void) { return m_info.mode; }
		inline short Width (void) { return m_info.width; }
		inline short Height (void) { return m_info.height; }
		inline fix Aspect (void) { return m_info.aspect; }

		inline void SetCanvas (CCanvas* canvas) { m_info.canvas = *canvas; }
		inline void SetMode (u_int32_t mode) { m_info.mode = mode; }
		inline void SetWidth (short width) { 
			m_info.width = width; 
			scale [0] = (width > 640) ? float (width) / 640.0f : 1.0;
			}
		inline void SetHeight (short height) { 
			m_info.height = height; 
			scale [1] = (height > 480) ? float (height) / 480.0f : 1.0f;
			}
		inline void SetAspect (fix aspect) { m_info.aspect = aspect; }
		inline float Scale (uint i = 0) { return m_info.scale [i]; }

		static CScreen* Current (void) { return m_current; }
};

extern CScreen screen;

//===========================================================================

#endif //_CANVAS_H
