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

#ifndef _SCORES_H
#define _SCORES_H

#define ROBOT_SCORE             1000
#define HOSTAGE_SCORE           1000
#define CONTROL_CEN_SCORE       5000
#define ENERGY_SCORE            0
#define SHIELD_SCORE            0
#define LASER_SCORE             0
#define DEBRIS_SCORE            0
#define CLUTTER_SCORE           0
#define MISSILE1_SCORE          0
#define MISSILE4_SCORE          0
#define KEY_SCORE               0
#define QUAD_FIRE_SCORE         0

#define VULCAN_AMMO_SCORE       0
#define CLOAK_SCORE             0
#define TURBO_SCORE             0
#define INVULNERABILITY_SCORE   0
#define HEADLIGHT_SCORE         0

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define COOL_MESSAGE_LEN 		50
#define MAX_HIGH_SCORES 		10

//------------------------------------------------------------------------------

typedef struct tStatsInfo {
  	char		name [CALLSIGN_LEN + 1];
	int32_t		score;
	int8_t		startingLevel;
	int8_t		endingLevel;
	int8_t		diffLevel;
	int16_t 	killRatio;		// 0-100
	int16_t		hostageRatio;  // 
	int32_t		seconds;		// How long it took in seconds...
} tStatsInfo;

//------------------------------------------------------------------------------

typedef struct tScoreInfo {
	char			nSignature [3];			// DHS
	int8_t       version;				// version
	char			szCoolSaying [COOL_MESSAGE_LEN];
	tStatsInfo	stats [MAX_HIGH_SCORES];
} tScoreInfo;

//------------------------------------------------------------------------------

class CScoreManager {
	private:
		tScoreInfo	m_scores;
		tStatsInfo	m_lastGame;
		char			m_filename [FILENAME_LEN];
		CBackground	m_background;
		int32_t		m_nFade;
		bool			m_bHilite;

	public:
		CScoreManager ();
		bool Load (void);
		bool Save (void);
		void Show (int32_t nCurItem);
		void Add (int32_t bAbort);

	private:
		char* GetFilename (void);
		void SetDefaultScores (void);
		void InitStats (tStatsInfo& stats);
		char* IntToString (int32_t number, char *dest);
		void _CDECL_ RPrintF (int32_t x, int32_t y, const char * format, ...);
		void RenderItem (int32_t i, tStatsInfo& stats);
		void Render (int32_t nCurItem);
		int32_t HandleInput (int32_t& nCurItem);
	};

extern CScoreManager scoreManager;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#endif /* _SCORES_H */
