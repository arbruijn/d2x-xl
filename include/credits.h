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

#ifndef _CREDITS_H
#define _CREDITS_H

class CCreditsManager {

#define NUM_LINES_HIRES 21
#define NUM_LINES (gameStates.menus.bHires ? NUM_LINES_HIRES : 20)

	private:
		CFile				m_cf;
		char				m_buffer [NUM_LINES_HIRES][80];
		CFont*			m_fonts [3];
		CBitmap			m_bmBackdrop;
		uint				m_bDone;
		uint				m_nLines [2];
		fix				m_xDelay;
		int				m_nExtraInc;
		int				m_bBinary;
		int				m_xOffs;
		int				m_yOffs;
		int				m_nFirstLineOffs;

	public:
		CCreditsManager () { Init (); }
		~CCreditsManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void RenderBackdrop (void);
		bool HandleInput (void);
		uint GetLine (void);
		bool Open (char* creditsFilename);
		void Render (void);
		void Show (char* creditsFilename);
};

extern CCreditsManager creditsManager;

#endif /* _CREDITS_H */
