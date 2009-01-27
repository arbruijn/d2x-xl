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

#define COMPATIBLE_PLAYER_FILE_VERSION    17
#define D2W95_PLAYER_FILE_VERSION			24
#define D2XW32_PLAYER_FILE_VERSION			45		// first flawless D2XW32 CPlayerData file version
#define D2XXL_PLAYER_FILE_VERSION			160
#define PLAYER_FILE_VERSION					160	//increment this every time the CPlayerData file changes

#define N_SAVE_SLOTS    10
#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

#ifndef EZERO
#	define EZERO 0
#endif

// update the CPlayerData's highest level.  returns errno (0 == no error)
int UpdatePlayerFile ();
// Used to save KConfig values to disk.
int SavePlayerProfile ();
int NewPlayerConfig ();

// called once at program startup to get the CPlayerData's name
int SelectPlayer (void);

int LoadPlayerProfile (int bOnlyWindowSizes);

// set a new highest level for CPlayerData for this mission
void SetHighestLevel (ubyte nLevel);

// gets the CPlayerData's highest level from the file for this mission
int GetHighestLevel(void);

void FreeParams (void);

//------------------------------------------------------------------------------

class CParam {
	public:
		class CParam*	next;
		char*				valP;
		short				nValues;
		ubyte				nSize;
		char				szTag [1];

	public:
		int Save (CFile& cf);
		int Set (const char *pszIdent, const char *pszValue);
	};

//------------------------------------------------------------------------------

class CPlayerProfile {
	private:
		CParam*	paramList;
		CParam*	lastParam;
		bool		bRegistered;
		CFile		m_cf;
		
		char* MakeTag (char *pszTag, const char *pszIdent, int i, int j);
		void RegisterConfig (kcItem *cfgP, int nItems, const char *pszId);
		int FindInConfig (kcItem *cfgP, int nItems, int iItem, const char *pszText);
		int Register (void *valP, const char *pszIdent, int i, int j, ubyte nSize);
		void Create (void);
		CParam* Find (const char *pszTag);
		int Set (const char *pszIdent, const char *pszValue);
		int LoadParam (void);

	public:
		CPlayerProfile () { Init (); }
		~CPlayerProfile () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Setup (void);
		int Load (void);
		int Save (void);
	};

extern CPlayerProfile profile;

//------------------------------------------------------------------------------

#endif /* _PLAYSAVE_H */
