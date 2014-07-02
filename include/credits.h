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

class CCreditsRenderer {

#define NUM_LINES_HIRES 21
#define NUM_LINES (gameStates.menus.bHires ? NUM_LINES_HIRES : 20)

	private:
		CFile				m_cf;
		char				m_buffer [NUM_LINES_HIRES][80];
		CFont*			m_fonts [3];
		CBitmap			m_bmBackdrop;
		uint32_t				m_bDone;
		uint32_t				m_nLines [2];
		int32_t				m_xDelay;
		int32_t				m_xTimeout;
		int32_t				m_nExtraInc;
		int32_t				m_bBinary;
		int32_t				m_nFirstLineOffs;
		CBackground		m_background;

	public:
		CCreditsRenderer () { Init (); }
		~CCreditsRenderer () { Destroy (); }
		void Init (void);
		void Destroy (void);
		bool HandleInput (void);
		uint32_t Read (void);
		bool Open (char* creditsFilename);
		void Render (void);
		void Show (char* creditsFilename);
};

extern CCreditsRenderer creditsRenderer;

#endif /* _CREDITS_H */
