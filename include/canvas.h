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
		int32_t m_t;

		CViewport (int32_t x = 0, int32_t y = 0, int32_t w = 0, int32_t h = 0, int32_t t = 0) : CRectangle (x, y, w, h), m_t (t) {}

		CViewport (const CViewport& other) : CRectangle (other), m_t (other.m_t) {}

		void Setup (int32_t x, int32_t y, int32_t w, int32_t h);

		void Apply (int32_t t = -1);

		inline CViewport& operator= (const CViewport other) {
			*((CRectangle*) this) = (CRectangle&) other;
			m_t = other.m_t;
			return *this;
			}

		inline CViewport& operator+= (const CViewport other) {
			*((CRectangle*) this) += CRectangle (other);
			m_t = other.m_t;
			return *this;
			}

		inline CViewport operator+ (const CViewport other) {
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
	const char*		pszId;
	CCanvasColor	color;
	CPalette*		palette;
	CFont*			font;				// the currently selected font
	CCanvasColor	fontColors [2];   // current font background color (-1==Invisible)
	int16_t			nDrawMode;			// fill, XOR, etc.
	bool				bRelative;
} tCanvas;

typedef struct CCanvasBackup {
	CCanvas*	m_canvas;
#if DBG
	char		m_szId [100];
#endif
} tCanvasBackup;

class CCanvas : public CViewport, public CBitmap {
	private:
		tCanvas	m_info;

		static CStack<tCanvasBackup>	m_save;

	public:
		using CViewport::operator+=;

		static fix					xCanvW2, xCanvH2;
		static float				fCanvW2, fCanvH2;

		CCanvas*						m_parent;
		int32_t						m_nActivations;

	public:
		CCanvas () { Init (); }
		~CCanvas () {}

		static CCanvas* Create (int32_t w, int32_t h);
		void Init (void);
		void Init (int32_t nType, int32_t w, int32_t h, uint8_t *data);
		//void Setup (int32_t w, int32_t h);
		void Setup (CCanvas* parentP);
		void Setup (int32_t w, int32_t h);
		void Setup (CCanvas* parentP, int32_t x, int32_t y, int32_t w, int32_t h, bool bUnscale = false);
		CCanvas* CreatePane (int32_t x, int32_t y, int32_t w, int32_t h);
		//void SetupPane (CCanvas* childP, int32_t x, int32_t y, int32_t w, int32_t h);
		void Destroy (void);
		void DestroyPane (void);
		void Clear (void);

		inline tCanvas& Info (void) { return m_info; }
		inline CCanvasColor& Color (void) { return m_info.color; }
		inline CCanvasColor& FontColor (int32_t i) { return m_info.fontColors [i]; }
		inline CFont* Font (void) { return m_info.font; }
		inline int16_t DrawMode (void) { return m_info.nDrawMode; }

		inline void SetColor (CCanvasColor& color) { m_info.color = color; }
		inline void SetFontColor (CCanvasColor& color, int32_t i) { m_info.fontColors [i] = color; }
		inline void SetFont (CFont *font) { m_info.font = font; }
		inline void SetDrawMode (int16_t nDrawMode) { m_info.nDrawMode = nDrawMode; }
		inline void SetId (const char* pszId) { m_info.pszId = pszId; }
		inline const char* Id (void) { return m_info.pszId; }

		static CCanvas* Current (void) { return m_save.ToS () ? m_save.Top ()->m_canvas : NULL; }

		static void Push (CCanvas* canvP) { 
			if (!m_save.Buffer ())
				m_save.Create (10);
			if (m_save.Grow ()) {
				m_save.Top ()->m_canvas = canvP;
#if DBG
				if (canvP->Id ())
					strncpy (m_save.Top ()->m_szId, canvP->Id (), sizeof (m_save.Top ()->m_szId));
				else
					*m_save.Top ()->m_szId = '\0';
#endif
				}
			fontManager.Push (canvP->Id ());
			}
		static void Pop (void) { 
			m_save.Pop (); 
			fontManager.Pop ();
			}

		void Clear (uint32_t color);
		void SetColor (int32_t color);
		void SetColorRGB (uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
		void SetColorRGB15bpp (uint16_t c, uint8_t alpha);
		inline void SetColorRGBi (int32_t i) { SetColorRGB (RGBA_RED (i), RGBA_GREEN (i), RGBA_BLUE (i), RGBA_ALPHA (i)); }

		inline void SetWidth (int16_t w = -1) { 
			if (w > 0)
				CViewport::SetWidth (w); 
			else
				w = Width ();
			if (this == Current ()) {
				fCanvW2 = (float) w * 0.5f;
				xCanvW2 = I2X (w) / 2;
				}
			}
		inline void SetHeight (int16_t h = -1) { 
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

		inline bool Clip (int32_t x, int32_t y) { return (x < 0) || (y < 0) || (x >= Width ()) || (y >= Width ()); }
		//inline bool Clip (int32_t x, int32_t y) { return this->CBitmap::Clip (x, y); }

		inline double AspectRatio (void) { return double (Width ()) / double (Height ()); }

		void FadeColorRGB (double dFade);

		inline float GetScale (void);
		inline int32_t Scaled (int32_t v);
		inline int32_t Unscaled (int32_t v);

		inline int32_t Width (bool bScale = true) { return bScale ? Scaled (CViewport::Width ()) : CViewport::Width (); }
		inline int32_t Height (bool bScale = true) { return bScale ? Scaled (CViewport::Height ()) : CViewport::Height (); }
		inline int32_t Left (bool bScale = true) { return bScale ? Scaled (CViewport::Left ()) : CViewport::Left (); }
		inline int32_t Top (bool bScale = true) { return bScale ? Scaled (CViewport::Top ()) : CViewport::Top (); }
		inline int32_t Right (void) { return Left () + Width (); }
		inline int32_t Bottom (void) { return Top () + Height (); }

		void GetExtent (CRectangle& rc, bool bScale = true, bool bDeep = false);

		inline CCanvas* Parent (void) { return m_parent; }

		void SetViewport (CCanvas* parent = NULL);

		void Activate (const char* szId, CCanvas* parent = NULL, bool bReset = false);

		inline void Reactivate (void) { SetViewport (m_parent); }

		void Deactivate (void);
	};

//------------------------------------------------------------------------------

class CGameScreenData : public CCanvas {
	public:
		void Set (int16_t l, int16_t t, int16_t w, int16_t h) { CViewport::SetLeft (l), CViewport::SetTop (t), CViewport::SetWidth (w), CViewport::SetHeight (h); }
		inline uint32_t Scalar (void) { return (((uint32_t) Width (false)) << 16) + (((uint32_t) Height (false)) & 0xFFFF); }
		inline uint32_t Area (void) { return (uint32_t) Width (false) * (uint32_t) Height (false); }
		void Set (uint32_t scalar) { Set (0, 0, (int16_t) (scalar >> 16), (int16_t) (scalar & 0xFFFF)); }

		static int32_t StereoOffset (void);
	};

//------------------------------------------------------------------------------

typedef struct tScreen {		// This is a video screen
	uint32_t	mode;				// Video mode number
	int16_t   		width, height; // Actual Width and Height
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

		inline uint32_t Mode (void) { return m_info.mode; }

		inline int16_t Width (bool bScale = true) { return bScale ? (int16_t) FRound (m_info.width * GetScale ()) : m_info.width; }
		inline int16_t Height (bool bScale = true) { return bScale ? (int16_t) FRound (m_info.height * GetScale ()) : m_info.height; }
		inline fix Aspect (void) { return m_info.aspect; }

		inline void SetMode (uint32_t mode) { m_info.mode = mode; }
		inline void SetWidth (int16_t width) { 
			m_info.width = width; 
			m_info.scale [0] = (width > 640) ? float (width) / 640.0f : 1.0f;
			}
		inline void SetHeight (int16_t height) { 
			m_info.height = height; 
			m_info.scale [1] = (height > 480) ? float (height) / 480.0f : 1.0f;
			}
		inline void SetAspect (fix aspect) { m_info.aspect = aspect; }
		inline float Scale (uint32_t i = 0) { return m_info.scale [i] ? m_info.scale [i] : 1.0f; }

		static float GetScale (void) { return m_fScale; }
		static void SetScale (float scale) { m_fScale = scale; }
		static int32_t Scaled (int32_t v) { return int32_t (ceil (v * GetScale ())); }
		static int32_t Unscaled (int32_t v) { return int32_t (ceil (v / GetScale ())); }
};

//extern CScreen screen;

inline float CCanvas::GetScale (void) { return CScreen::GetScale (); }

inline int32_t CCanvas::Scaled (int32_t v) { return CScreen::Scaled (v); }

inline int32_t CCanvas::Unscaled (int32_t v) { return CScreen::Unscaled (v); }

void SetupCanvasses (float scale = 0.0f);

//===========================================================================

#endif //_CANVAS_H
