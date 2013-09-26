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

#ifndef _MENUBACKGROUND_H
#define _MENUBACKGROUND_H

#include "bitmap.h"
#include "canvas.h"

//------------------------------------------------------------------------------

#define BG_MENU		0
#define BG_STARS		1
#define BG_SCORES		2
#define BG_MAP			3

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
		bool Create (int x, int y, int width, int height, int nType, int nWallpaper);
		void Restore (void);
		void Restore (int dx, int dy, int w, int h, int sx, int sy);
		void Draw (bool bUpdate = false);
		void DrawArea (int left, int top, int right, int bottom);
		void DrawBox (void);

		inline CBitmap* Bitmap (void) { return m_bitmap; }
		inline CBitmap* SetBitmap (CBitmap* bmP) { return m_bitmap = bmP; }
		inline CBitmap* Background (void) { return &m_saved; }
		inline CBitmap* Saved (void) { return &m_saved; }
		inline void GetExtent (int& x, int& y, int& w, int& h) { CViewport::GetExtent (x, y, w, h); }
		inline char* GetFilename (void) { return m_bitmap ? m_bitmap->Name () : NULL; }
		inline int Wallpaper (void) { return m_nWallpaper; }

		void Activate (void);
		void Deactivate (void);
		
	private:
		CBitmap* Load (char* filename, int width, int height);
		void Setup (int x, int y, int width, int height);
		void Save (int i, int width, int height);
};

//------------------------------------------------------------------------------

class CBackgroundManager : public CStack<CBackground> {
	private:
		CStack< CBackground* >	m_backgrounds;
		CBackground					m_wallpapers [4];
		char*							m_filename [4];
		int							m_nWallpaper;
		int							m_nDepth;
		bool							m_bValid;
		bool							m_bShadow;

	public:
		CBackgroundManager ();
		void Init (void);
		void Destroy (void);
		void Create (void);
		CBackground* Setup (int x, int y, int width, int height, int nType, int nWallPaper);

		inline char* Filename (uint i = 0) { return m_filename [i]; }
		inline CBitmap* Wallpaper (uint i = 0) { return m_wallpapers [i].Bitmap (); }
		inline int Depth (void) { return m_nDepth; }
		inline bool Shadow (void) { return m_bShadow; }
		inline void SetShadow (bool bShadow) { m_bShadow = bShadow; }

		void Redraw (CBackground* bg, bool bUpdate = false);
#if 0
		inline void Restore (void) { 
			if (m_backgrounds.ToS ())
				(*m_backgrounds.Top ())->Restore (); 
			}
		inline void Restore (int dx, int dy, int w, int h, int sx, int sy) { 
			if (m_backgrounds.ToS ())
				(*m_backgrounds.Top ())->Restore (dx, dy, w, h, sy, sy); 
			}

		inline void DrawArea (int left, int top, int right, int bottom) { 
			if (m_backgrounds.ToS ())
				(*m_backgrounds.Top ())->DrawArea (left, top, right, bottom); 
			}

		inline void Draw (bool bUpdate = false) { 
			if (m_backgrounds.ToS ())
				(*m_backgrounds.Top ())->Draw (bUpdate); 
			}

		void DrawBox (int left, int top, int right, int bottom, int nLineWidth, float fAlpha, int bForce);

		inline void DrawBox (int nLineWidth, float fAlpha, int bForce) {
			DrawBox (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), 
						CCanvas::Current ()->Right (), CCanvas::Current ()->Bottom (), 
						nLineWidth, fAlpha, bForce);
			}
#endif

	private:
		CBitmap* LoadCustomWallpaper (char* filename = NULL);
		CBitmap* LoadWallpaper (char* filename);
	};

//------------------------------------------------------------------------------

extern  CBackgroundManager backgroundManager;

//------------------------------------------------------------------------------

#endif // _MENUBACKGROUND_H 
