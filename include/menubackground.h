#ifndef _MENUBACKGROUND_H
#define _MENUBACKGROUND_H

#include "bitmap.h"
#include "canvas.h"

//------------------------------------------------------------------------------

#define BG_STANDARD			0
#define BG_STARS				1
#define BG_SCORES				2
#define BG_MAP					3
#define BG_LOADING			4
#define BG_MENU				BG_SCORES

#define BG_TOPMENU			0
#define BG_SUBMENU			1
#define BG_WALLPAPER			2

//------------------------------------------------------------------------------

const char* WallpaperName (int32_t nType, int32_t bHires = -1);

//------------------------------------------------------------------------------

class CBackground : public CCanvas {
	private:
		CBitmap		m_saved;				// copy of a screen area covered by a menu
		CBitmap*		m_bitmap;			// complete background
		int32_t		m_nType;
		int32_t		m_nWallpaper;
		CRGBColor	m_boxColor;

	public:
		CBackground () { 
			SetBoxColor ();
			Init (); 
			}
		~CBackground () { Destroy (); }
		void Init (void);
		void Destroy (void);
		bool Create (int32_t width, int32_t height, int32_t nType, int32_t nWallpaper);
		void Setup (int32_t width, int32_t height);
		void Draw (bool bUpdate = false);
		void DrawArea (void);
		void DrawBox (void);

		inline CBitmap* Bitmap (void) { return m_bitmap; }
		inline CBitmap* SetBitmap (CBitmap* pBm) { return m_bitmap = pBm; }
		inline CBitmap* Background (void) { return &m_saved; }
		inline CBitmap* Saved (void) { return &m_saved; }
		inline void GetExtent (int32_t& x, int32_t& y, int32_t& w, int32_t& h) { CCanvas::CViewport::GetExtent (x, y, w, h); }
		inline char* GetFilename (void) { return m_bitmap ? m_bitmap->Name () : NULL; }
		inline int32_t Wallpaper (void) { return m_nWallpaper; }

		inline void SetType (int32_t nType) { m_nType = nType; }
		inline int32_t GetType (void) { return m_nType; }

		inline void SetBoxColor (uint8_t red = PAL2RGBA (22), uint8_t green = PAL2RGBA (22), uint8_t blue = PAL2RGBA (38)) { m_boxColor.Set (red, green, blue); }

		void Activate (void);
		void Deactivate (void);
		
	private:
		CBitmap* Load (char* filename, int32_t width, int32_t height);
		void Save (int32_t i, int32_t width, int32_t height);
};

//------------------------------------------------------------------------------

#define WALLPAPER_COUNT 5

class CBackgroundManager : public CStack<CBackground> {
	private:
		CBackground		m_wallpapers [WALLPAPER_COUNT];
		const char*		m_filenames [WALLPAPER_COUNT];
		bool				m_bValid;
		bool				m_bShadow;

	public:
		CBackgroundManager () : m_bValid (false) { Init (); }
		~CBackgroundManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Create (void);
		void Rebuild (void);

		bool Setup (CBackground& bg, int32_t width, int32_t height, int32_t nType = BG_SUBMENU, int32_t nWallPaper = BG_STANDARD);
		void Activate (CBackground& bg);

		inline const char* Filename (uint32_t i = 0) { return m_filenames [i]; }
		inline CBitmap* Wallpaper (uint32_t i = 0) { return m_wallpapers [i].Bitmap (); }
		inline bool Shadow (void) { return m_bShadow; }
		inline void SetShadow (bool bShadow) { m_bShadow = bShadow; }

		void Draw (CBackground* bg = NULL, bool bUpdate = false);
		void Draw (int32_t nWallpaper);
		void DrawBox (int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t nLineWidth, float fAlpha, int32_t bForce);

		CBitmap* LoadCustomWallpaper (const char* filename = NULL);

		inline CBitmap* SetWallpaper (CBitmap* wallpaper, int32_t nWallpaper) {
			CBitmap* oldWallpaper = m_wallpapers [nWallpaper].Bitmap ();
			m_wallpapers [nWallpaper].SetBitmap (wallpaper);
			return oldWallpaper;
			}

	private:
		CBitmap* LoadWallpaper (const char* filename);
		CBitmap* LoadDesktopWallpaper (void);
		CBitmap* LoadMenuBackground (void);
	};

//------------------------------------------------------------------------------

extern  CBackgroundManager backgroundManager;

//------------------------------------------------------------------------------

#endif // _MENUBACKGROUND_H 

