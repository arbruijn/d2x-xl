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

#ifndef _PLAYSAVE_H
#define _PLAYSAVE_H

#include "kconfig.h"

#define COMPATIBLE_PLAYER_FILE_VERSION    17
#define D2W95_PLAYER_FILE_VERSION			24
#define D2XW32_PLAYER_FILE_VERSION			45		// first flawless D2XW32 player file version
#define D2XXL_PLAYER_FILE_VERSION			161
#define PLAYER_FILE_VERSION					161	//increment this every time the player file changes

#define N_SAVE_SLOTS    10
#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

#ifndef EZERO
#	define EZERO 0
#endif

// update the player's highest level.  returns errno (0 == no error)
int32_t UpdatePlayerFile ();
// Used to save KConfig values to disk.
int32_t SavePlayerProfile ();
int32_t NewPlayerConfig ();

// called once at program startup to get the player's name
int32_t SelectPlayer (void);

int32_t LoadPlayerProfile (int32_t nStage = 2);

// set a new highest level for player for this mission
void SetHighestLevel (uint8_t nLevel);

// gets the player's highest level from the file for this mission
int32_t GetHighestLevel(void);

//------------------------------------------------------------------------------

class CParam {
	public:
		class CParam*	next;
		char*				valP;
		int16_t				nValues;
		uint8_t				nSize;
		char				szTag [1];

	public:
		int32_t Save (CFile& cf);
		int32_t Set (const char *pszIdent, const char *pszValue);
	};

//------------------------------------------------------------------------------

class CPlayerProfile {
	private:
		CParam*	paramList;
		CParam*	lastParam;
		bool		bRegistered;
		CFile		m_cf;
		
		char* MakeTag (char *pszTag, const char *pszIdent, int32_t i, int32_t j);
		void RegisterConfig (kcItem *cfgP, int32_t nItems, const char *pszId);
		int32_t FindInConfig (kcItem *cfgP, int32_t nItems, int32_t iItem, const char *pszText);
		int32_t Register (void *valP, const char *pszIdent, int32_t i, int32_t j, uint8_t nSize);
		void Create (void);
		CParam* Find (const char *pszTag);
		int32_t Set (const char *pszIdent, const char *pszValue);
		int32_t LoadParam (void);

	public:
		CPlayerProfile () { Init (); }
		~CPlayerProfile () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Setup (void);
		int32_t Load (bool bOnlyWindowSizes = false);
		int32_t Save (void);
		inline bool Busy (void) { return m_cf.File () != 0; }
	};

extern CPlayerProfile profile;

//------------------------------------------------------------------------------

#endif /* _PLAYSAVE_H */
