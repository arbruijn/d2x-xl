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
		CCanvas*	m_menu;			// canvas (screen area) of a menu
		CBitmap*	m_saved;			// copy of a screen area covered by a menu
		CBitmap*	m_background;	// complete background
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
		bool Create (void);
		void Save (bool bForce);
		void Restore (void);
		void Draw (void);
		void Box (void);
		bool Load (char* filename);

		static inline CCanvas* Canvas () { return m_canvas; }
};

class CBackgroundManager : private CStack<CBackground> {
	private:
		CStack<CBackground>	m_save;
		CBackground				m_bg;
		CBitmap*					m_background;
		int						m_nDepth;
		char*						m_filename;

	public:
		CBackgroundManager ();
		void Setup (char* filename);
		void Load (void);
		void Restore (void);
		void Remove (void);
		void LoadBackground (char* filename);

	private:
		bool IsDefault (char* filename);
		void LoadCustomBackground (void);
	};

extern  CBackgroundManager backgroundManager;

#endif // _MENUBACKGROUND_H 
