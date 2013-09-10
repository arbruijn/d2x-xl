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

//#error +++++++++++++++++++ CANVAS_H ++++++++++++++++++++++++

#include "pstypes.h"
#include "fix.h"
#include "cstack.h"
#include "bitmap.h"
#include "font.h"
#include "palette.h"

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
	CCanvasColor	color;
	CFont				*font;				// the currently selected font
	CCanvasColor	fontColors [2];   // current font background color (-1==Invisible)
	short				nDrawMode;			// fill, XOR, etc.
	 float			scale;
} tCanvas;

class CCanvas : public CBitmap {
	private:
		tCanvas	m_info;

		static CCanvas*			m_current;
		static CStack<CCanvas*> m_save;

	public:
		static fix					xCanvW2, xCanvH2;
		static float				fCanvW2, fCanvH2;

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

		inline tCanvas& Info (void) { return m_info; }
		inline CCanvasColor& Color (void) { return m_info.color; }
		inline CCanvasColor& FontColor (int i) { return m_info.fontColors [i]; }
		inline CFont* Font (void) { return m_info.font; }
		inline short DrawMode (void) { return m_info.nDrawMode; }

		inline void SetColor (CCanvasColor& color) { m_info.color = color; }
		inline void SetFontColor (CCanvasColor& color, int i) { m_info.fontColors [i] = color; }
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
		inline void SetColorRGBi (int i) { SetColorRGB (RGBA_RED (i), RGBA_GREEN (i), RGBA_BLUE (i), RGBA_ALPHA (i)); }

		inline void SetWidth (short w = -1) { 
			if (w > 0)
				CBitmap::SetWidth (w); 
			else
				w = Width ();
			if (this == m_current) {
				fCanvW2 = (float) w * 0.5f;
				xCanvW2 = I2X (w) / 2;
				}
			}
		inline void SetHeight (short h = -1) { 
			if (h > 0)
				CBitmap::SetHeight (h); 
			else
				h = Height ();
			if (this == m_current) {
				fCanvH2 = (float) h * 0.5f;
				xCanvH2 = I2X (h) / 2;
				}
			}

		inline float XScale (void) { return (Width () > 640) ? float (Width ()) / 640.0f : 1.0f; }
		inline float YScale (void) { return (Height () > 480) ? float (Height ()) / 480.0f : 1.0f; }

		inline void SetScale (float scale) { m_info.scale = scale; }
		inline float GetScale (void) { return m_info.scale; }

		inline bool Clip (int x, int y) { return this->CBitmap::Clip (x, y); }

		inline double AspectRatio (void) { return double (Width ()) / double (Height ()); }

		inline short Width (void) { return short (CBitmap::Width () * m_info.scale + 0.5f); }
		inline short Height (void) { return short (CBitmap::Height () * m_info.scale + 0.5f); }
		inline short Left (void) { return short (CBitmap::Left () * m_info.scale + 0.5f); }
		inline short Top (void) { return short (CBitmap::Top () * m_info.scale + 0.5f); }
		inline short Right (void) { return Left () + Width (); }
		inline short Bottom (void) { return Top () + Height (); }

		void FadeColorRGB (double dFade);
};

//===========================================================================

typedef struct tScreen {		// This is a video screen
	CCanvas  	canvas;			// Represents the entire screen
	u_int32_t	mode;				// Video mode number
	short   		width, height; // Actual Width and Height
	fix     		aspect;			//aspect ratio (w/h) for this screen
	float			scale [3];		//size ratio compared to 640x480
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
			m_info.scale [2] = 1.0f;
			if (!m_current)
				m_current = this;
			CCanvas::SetCurrent ();
			}
		void Destroy (void) { Canvas ()->CBitmap::Destroy (); };

		inline CCanvas* Canvas (void) { return &m_info.canvas; }
		inline u_int32_t Mode (void) { return m_info.mode; }
		inline short Width (void) { return short (m_info.width * m_info.scale [2] + 0.5f); }
		inline short Height (void) { return short (m_info.height * m_info.scale [2] + 0.5f); }
		inline fix Aspect (void) { return m_info.aspect; }

		inline void SetCanvas (CCanvas* canvas) { 
			if (canvas) {
				m_info.canvas = *canvas;
				m_info.canvas.SetScale (Scale (2));
				}
			}
		inline void SetMode (u_int32_t mode) { m_info.mode = mode; }
		inline void SetWidth (short width) { 
			m_info.width = width; 
			m_info.scale [0] = (width > 640) ? float (width) / 640.0f : 1.0f;
			}
		inline void SetHeight (short height) { 
			m_info.height = height; 
			m_info.scale [1] = (height > 480) ? float (height) / 480.0f : 1.0f;
			}
		inline void SetAspect (fix aspect) { m_info.aspect = aspect; }
		inline float SetScale (float scale) { m_info.canvas.SetScale (m_info.scale [2] = scale); }
		inline float Scale (uint i = 0) { return m_info.scale [i] ? m_info.scale [i] : 1.0f; }

		static CScreen* Current (void) { return m_current; }
};

extern CScreen screen;

//===========================================================================

#endif //_CANVAS_H
