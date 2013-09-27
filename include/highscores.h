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

#ifndef _HIGHSCORES_H
#define _HIGHSCORES_H

class CScoreTable {
	private:
		int	m_nReady;
		int	m_nEscaped;
		int	m_nPrevSecsLeft;
		int	m_oldStates [MAX_PLAYERS];
		int	m_sorted [MAX_PLAYERS];
		int	m_bNetwork;
		CBackground	m_background;
		
	public:
		void Display (void);

	private:
		void DrawItem (int  i);
		void DrawCoopItem (int  i);
		void DrawNames (void);
		void DrawCoopNames (void);
		void DrawCoop (void);
		void DrawDeaths (void);
		void DrawCoopDeaths (void);
		void DrawReactor (const char *message);
		void DrawChampion (void);
		void Render (void);
		void RenderCoop (void);
		void Cleanup (int bQuit);
		bool Exit (void);
		int Input (void);
		int WaitForPlayers (void);
	};

extern CScoreTable scoreTable;

#endif //_HIGHSCORES_H
