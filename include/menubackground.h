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

class CBackground {
	private:
		CCanvas*	m_canvas;			// canvas (screen area) of a menu
		CBitmap*	m_saved;				// copy of a screen area covered by a menu
		CBitmap*	m_background;		// complete background
		char*		m_name;
		bool		m_bIgnoreCanv;
		bool		m_bIgnoreBg;
		bool		m_bTopMenu;
		bool		m_bMenuBox;

	public:
		CBackground () { Init (); }
		~CBackground () { Destroy (); }
		void Init (void);
		void Destroy (void);
		bool Create (char* filename, int x, int y, int width, int height);
		void Restore (void);
		void Draw (void);
		void DrawArea (int left, int top, int right, int bottom);
		void DrawBox (void);
		bool Load (char* filename);

		inline CCanvas* Canvas () { return m_canvas; }

	private:
		bool Load (char* filename, int width, int height);
		void Setup (int x, int y, int width, int height);
		void Save (int width, int height);
};

class CBackgroundManager : private CStack<CBackground> {
	private:
		CStack<CBackground>	m_save;
		CBackground				m_bg;
		CBitmap*					m_background [2];
		char*						m_filename [2];
		int						m_nDepth;
		bool						m_b3DEffect;

	public:
		CBackgroundManager ();
		void Init (void);
		void Destroy (void);
		void Setup (char* filename);
		void Load (void);
		void Restore (void);
		void Remove (void);
		CBitmap* LoadBackground (char* filename);
		bool IsDefault (char* filename);

		inline char* Filename (uint i = 0) { return m_filename [i]; }
		inline CBitmap* Background (uint i = 0) { return m_background [i]; }
		inline int Depth (void) { return m_nDepth; }

		void DrawBox (int nLineWidth, float fAlpha, int bForce);

	private:
		CBitmap* LoadCustomBackground (void);
	};

extern  CBackgroundManager backgroundManager;

#endif // _MENUBACKGROUND_H 
