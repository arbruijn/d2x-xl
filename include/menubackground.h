#ifndef _MENUBACKGROUND_H
#define _MENUBACKGROUND_H

#include "bitmap.h"
#include "canvas.h"

//------------------------------------------------------------------------------

#define BG_STANDARD		0
#define BG_STARS			1
#define BG_SCORES			2
#define BG_MAP				3
#define BG_LOADING		4
#define BG_MENU			BG_SCORES

#define BG_TOPMENU		0
#define BG_SUBMENU		1
#define BG_WALLPAPER		2

//------------------------------------------------------------------------------

char* WallpaperName (int nType, int bHires = -1);

//------------------------------------------------------------------------------

class CBackground : public CCanvas {
	private:
		CBitmap	m_saved;				// copy of a screen area covered by a menu
		CBitmap*	m_bitmap;			// complete background
		int		m_nType;
		int		m_nWallpaper;

	public:
		CBackground () { Init (); }
		~CBackground () { Destroy (); }
		void Init (void);
		void Destroy (void);
		bool Create (int width, int height, int nType, int nWallpaper);
		void Setup (int width, int height);
		void Draw (bool bUpdate = false);
		void DrawArea (void);
		void DrawBox (void);

		inline CBitmap* Bitmap (void) { return m_bitmap; }
		inline CBitmap* SetBitmap (CBitmap* bmP) { return m_bitmap = bmP; }
		inline CBitmap* Background (void) { return &m_saved; }
		inline CBitmap* Saved (void) { return &m_saved; }
		inline void GetExtent (int& x, int& y, int& w, int& h) { CViewport::GetExtent (x, y, w, h); }
		inline char* GetFilename (void) { return m_bitmap ? m_bitmap->Name () : NULL; }
		inline int Wallpaper (void) { return m_nWallpaper; }

		inline void SetType (int nType) { m_nType = nType; }
		inline int GetType (void) { return m_nType; }

		void Activate (void);
		void Deactivate (void);
		
	private:
		CBitmap* Load (char* filename, int width, int height);
		void Save (int i, int width, int height);
};

//------------------------------------------------------------------------------

#define WALLPAPER_COUNT 5

class CBackgroundManager : public CStack<CBackground> {
	private:
		CBackground		m_wallpapers [WALLPAPER_COUNT];
		char*				m_filenames [WALLPAPER_COUNT];
		bool				m_bValid;
		bool				m_bShadow;

	public:
		CBackgroundManager () : m_bValid (false) { Init (); }
		~CBackgroundManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Create (void);
		void Rebuild (void);

		bool Setup (CBackground& bg, int width, int height, int nType = BG_SUBMENU, int nWallPaper = BG_STANDARD);
		void Activate (CBackground& bg);

		inline char* Filename (uint i = 0) { return m_filenames [i]; }
		inline CBitmap* Wallpaper (uint i = 0) { return m_wallpapers [i].Bitmap (); }
		inline bool Shadow (void) { return m_bShadow; }
		inline void SetShadow (bool bShadow) { m_bShadow = bShadow; }

		void Draw (CBackground* bg = NULL, bool bUpdate = false);
		void Draw (int nWallpaper);
		void DrawBox (int left, int top, int right, int bottom, int nLineWidth, float fAlpha, int bForce);

	private:
		CBitmap* LoadWallpaper (char* filename);
		CBitmap* LoadCustomWallpaper (void);
		CBitmap* LoadDesktopWallpaper (void);
		CBitmap* LoadMenuBackground (void);
	};

//------------------------------------------------------------------------------

extern  CBackgroundManager backgroundManager;

//------------------------------------------------------------------------------

#endif // _MENUBACKGROUND_H 
