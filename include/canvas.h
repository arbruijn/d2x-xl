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
#include "font.h"
#include "palette.h"
#include "rectangle.h"

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

class CViewport : public CRectangle {
	public:
		int m_t;

		CViewport (int x = 0, int y = 0, int w = 0, int h = 0, int t = 0) : CRectangle (x, y, w, h), m_t (t) {}

		void Setup (int x, int y, int w, int h);

		void Apply (int t = -1);

		inline CViewport& operator= (CViewport const other) {
			CRectangle (*this) = CRectangle (other);
			m_t = other.m_t;
			return *this;
			}

		inline CViewport& operator+= (CViewport const other) {
			CRectangle (*this) += CRectangle (other);
			m_t = other.m_t;
			return *this;
			}

		inline CViewport operator+ (CViewport const other) {
			CViewport vp = *this;
			vp += other;
			return vp;
			}

		inline const bool operator!= (CViewport const other) {
			return (m_x != other.m_x) || (m_y != other.m_y) || (m_w != other.m_w) || (m_h != other.m_h) || (m_t != other.m_t);
			}
	};

//-----------------------------------------------------------------------------

class CCanvas;
class CScreen;

typedef struct tCanvas {
	CCanvasColor	color;
	CPalette*		palette;
	CFont*			font;				// the currently selected font
	CCanvasColor	fontColors [2];   // current font background color (-1==Invisible)
	short				nDrawMode;			// fill, XOR, etc.
	bool				bRelative;
} tCanvas;

class CCanvas : public CViewport, public CBitmap {
	private:
		tCanvas	m_info;

		static CStack<CCanvas*>	m_save;

	public:
		static fix					xCanvW2, xCanvH2;
		static float				fCanvW2, fCanvH2;

		CCanvas*						m_parent;
		int							m_nActivations;

	public:
		CCanvas () { Init (); }
		~CCanvas () {}

		static CCanvas* Create (int w, int h);
		void Init (void);
		void Init (int nType, int w, int h, ubyte *data);
		//void Setup (int w, int h);
		void Setup (CCanvas* parentP);
		void Setup (int w, int h);
		void Setup (CCanvas* parentP, int x, int y, int w, int h);
		CCanvas* CreatePane (int x, int y, int w, int h);
		//void SetupPane (CCanvas* childP, int x, int y, int w, int h);
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

		static CCanvas* Current (void) { return m_save.ToS () ? *m_save.Top () : NULL; }

		static void Push (CCanvas* canvP) { 
			m_save.Push (canvP); 
			fontManager.Push ();
			}
		static void Pop (void) { 
			m_save.Pop (); 
			fontManager.Pop ();
			}

		void Clear (uint color);
		void SetColor (int color);
		void SetColorRGB (ubyte red, ubyte green, ubyte blue, ubyte alpha);
		void SetColorRGB15bpp (ushort c, ubyte alpha);
		inline void SetColorRGBi (int i) { SetColorRGB (RGBA_RED (i), RGBA_GREEN (i), RGBA_BLUE (i), RGBA_ALPHA (i)); }

		inline void SetWidth (short w = -1) { 
			if (w > 0)
				CViewport::SetWidth (w); 
			else
				w = Width ();
			if (this == Current ()) {
				fCanvW2 = (float) w * 0.5f;
				xCanvW2 = I2X (w) / 2;
				}
			}
		inline void SetHeight (short h = -1) { 
			if (h > 0)
				CViewport::SetHeight (h); 
			else
				h = Height ();
			if (this == Current ()) {
				fCanvH2 = (float) h * 0.5f;
				xCanvH2 = I2X (h) / 2;
				}
			}

		inline float XScale (void) { return (Width () > 640) ? float (Width ()) / 640.0f : 1.0f; }
		inline float YScale (void) { return (Height () > 480) ? float (Height ()) / 480.0f : 1.0f; }

		inline bool Clip (int x, int y) { return (x < 0) || (y < 0) || (x >= Width ()) || (y >= Width ()); }
		//inline bool Clip (int x, int y) { return this->CBitmap::Clip (x, y); }

		inline double AspectRatio (void) { return double (Width ()) / double (Height ()); }

		void FadeColorRGB (double dFade);

		inline float GetScale (void);
		inline int Scaled (int v);

		inline int Width (bool bScale = true) { return bScale ? Scaled (CViewport::Width ()) : CViewport::Width (); }
		inline int Height (bool bScale = true) { return bScale ? Scaled (CViewport::Height ()) : CViewport::Height (); }
		inline int Left (void) { return Scaled (CViewport::Left ()); }
		inline int Top (void) { return Scaled (CViewport::Top ()); }
		inline int Right (void) { return Left () + Width (); }
		inline int Bottom (void) { return Top () + Height (); }

		inline CCanvas* Parent (void) { return m_parent; }

		void SetViewport (CCanvas* parent = NULL);

		inline void Activate (CCanvas* parent = NULL, bool bReset = false) { 
			//if (CCanvas::Current () != this)
			if (bReset)
				m_save.Reset ();
				CCanvas::Push (this);
			SetViewport (parent);
			}

		inline void Reactivate (void) { SetViewport (m_parent); }

		void Deactivate (void);
	};

//------------------------------------------------------------------------------

class CGameScreenData : public CCanvas {
	public:
		void Set (short l, short t, short w, short h) { CViewport::SetLeft (l), CViewport::SetTop (t), CViewport::SetWidth (w), CViewport::SetHeight (h); }
		inline uint Scalar (void) { return (((uint) Width (false)) << 16) + (((uint) Height (false)) & 0xFFFF); }
		inline uint Area (void) { return (uint) Width (false) * (uint) Height (false); }
		void Set (uint scalar) { Set (0, 0, (short) (scalar >> 16), (short) (scalar & 0xFFFF)); }

		static int StereoOffset (void);
	};

//------------------------------------------------------------------------------

typedef struct tScreen {		// This is a video screen
	u_int32_t	mode;				// Video mode number
	short   		width, height; // Actual Width and Height
	fix     		aspect;			//aspect ratio (w/h) for this screen
	float			scale [2];		//size ratio compared to 640x480
} tScreen;


class CScreen : public CGameScreenData {
	private:
		tScreen	m_info;

		static float m_fScale;

	public:
		CScreen () { Init (); }
		~CScreen () { Destroy (); }

		void Init (void) { 
			m_info.mode = 0; 
			m_info.width = 0;
			m_info.height = 0;
			m_info.aspect = 0;
			}
		void Destroy (void) { /*Canvas ()->CBitmap::Destroy ();*/ };

		inline u_int32_t Mode (void) { return m_info.mode; }

		inline short Width (bool bScale = true) { return bScale ? (short) FRound (m_info.width * GetScale ()) : m_info.width; }
		inline short Height (bool bScale = true) { return bScale ? (short) FRound (m_info.height * GetScale ()) : m_info.height; }
		inline fix Aspect (void) { return m_info.aspect; }

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
		inline float Scale (uint i = 0) { return m_info.scale [i] ? m_info.scale [i] : 1.0f; }

		static float GetScale (void) { return m_fScale; }
		static void SetScale (float scale) { m_fScale = scale; }
		static int Scaled (int v) { return int (ceil (v * GetScale ())); }
};

extern CScreen screen;

inline float CCanvas::GetScale (void) { return CScreen::GetScale (); }

inline int CCanvas::Scaled (int v) { return CScreen::Scaled (v); }

void SetupCanvasses (float scale = 1.0f);

//===========================================================================

#endif //_CANVAS_H
