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

#ifndef _MISSION_H
#define _MISSION_H

#include "pstypes.h"
#include "cfile.h"

#define MAX_MISSIONS                    5000
#define MAX_LEVELS_PER_MISSION          100
#define MAX_SECRET_LEVELS_PER_MISSION   (MAX_LEVELS_PER_MISSION / 5)
#define MISSION_NAME_LEN                25
#define MISSION_PREFIX_LEN					 5

#define D1_MISSION_FILENAME             "descent"
#define D1_MISSION_NAME                 "Descent: First Strike"
#define D1_MISSION_HOGSIZE              6856701 // v1.4 - 1.5
#define D1_10_MISSION_HOGSIZE           7261423 // v1.0
#define D1_15_MISSION_HOGSIZE           6856183 // v1.0 -> 1.5
#define D1_3DFX_MISSION_HOGSIZE         9934912
#define D1_MAC_MISSION_HOGSIZE          7456179
#define D1_OEM_MISSION_NAME             "Destination Saturn"
#define D1_OEM_MISSION_HOGSIZE          4492107 // v1.4a
#define D1_OEM_10_MISSION_HOGSIZE       4494862 // v1.0
#define D1_SHAREWARE_MISSION_NAME       "Descent Demo"
#define D1_SHAREWARE_MISSION_HOGSIZE    2339773 // v1.4
#define D1_SHAREWARE_10_MISSION_HOGSIZE 2365676 // v1.0 - 1.2
#define D1_MAC_SHARE_MISSION_HOGSIZE    3370339

#define SHAREWARE_MISSION_FILENAME		"d2demo"
#define SHAREWARE_MISSION_NAME			"Descent 2 Demo"
#define SHAREWARE_MISSION_HOGSIZE		2292566 // v1.0 (d2demo.hog)
#define MAC_SHARE_MISSION_HOGSIZE		4292746

#define OEM_MISSION_FILENAME				"d2"
#define OEM_MISSION_NAME					"D2 Destination:Quartzon"
#define OEM_MISSION_HOGSIZE				6132957 // v1.1

#define FULL_MISSION_FILENAME				"d2"
#define D2_MISSION_NAME                "Descent 2: Counterstrike"
#define FULL_MISSION_HOGSIZE				7595079 // v1.1 - 1.2
#define FULL_10_MISSION_HOGSIZE			7107354 // v1.0
#define MAC_FULL_MISSION_HOGSIZE			7110007 // v1.1 - 1.2

//values that describe where a mission is located
#define ML_MISSIONDIR   0
#define ML_ALTHOGDIR    1
#define ML_CURDIR       2
#define ML_CDROM        3
#define ML_DATADIR		4
#define ML_MSNROOTDIR	5

//where the missions go
#define BASE_MISSION_FOLDER	"missions"
#define MISSION_FOLDER			(gameOpts->app.bSinglePlayer ? BASE_MISSION_FOLDER "/single/" : BASE_MISSION_FOLDER)

//------------------------------------------------------------------------------

#define LEVEL_NAME_LEN 36       //make sure this is a multiple of 4!

//mission list entry
typedef struct tMsnListEntry {
	char    filename [FILENAME_LEN];                // path and filename without extension
	char    szMissionName [MISSION_NAME_LEN + MISSION_PREFIX_LEN + 1];
	ubyte   bAnarchyOnly;          // if true, mission is anarchy only
	ubyte   location;                   // see defines below
	ubyte   nDescentVersion;            // descent 1 or descent 2?
} tMsnListEntry;


class CMissionData {
	public:
		int					nCurrentMission;
		int					nLastMission;
		int					nEnhancedMission;
		int					nEntryLevel;
		int					nCurrentLevel;
		int					nLastLevel;
		int					nSecretLevels;
		int					nLastSecretLevel;
		int					nNextLevel [2];
		int					nBuiltInMission [2];
		int					nBuiltInHogSize [2];
		char					szCurrentLevel [LEVEL_NAME_LEN];
		char					szBuiltinMissionFilename [2][9];
		char					szBriefingFilename [2][13];
		char					szLevelNames [MAX_LEVELS_PER_MISSION][FILENAME_LEN];
		char					szSecretLevelNames [MAX_SECRET_LEVELS_PER_MISSION][FILENAME_LEN];
		int					secretLevelTable [MAX_SECRET_LEVELS_PER_MISSION];
		char					szSongNames [MAX_LEVELS_PER_MISSION][FILENAME_LEN];
		char					nLevelState [MAX_LEVELS_PER_MISSION];

	protected:
		tMsnListEntry		m_list [MAX_MISSIONS + 1];
		int					m_nCount;

	public:
		CMissionData ();
};


class CMissionManager : public CMissionData {
	public:
		//fills in the global list of missions.  Returns the number of missions
		//in the list.  If anarchy_mode set, don't include non-anarchy levels.
		//if there is only one mission, this function will call missionManager.Load on it.
		int BuildList (int anarchy_mode, int nSubFolder);

		//loads the specfied mission from the mission list.  BuildMissionList()
		//must have been called.  If BuildMissionList() returns 0, this function
		//does not need to be called.  Returns true if mission loaded ok, else false.
		int Load (int nMission);

		//loads the named mission if exists.
		//Returns true if mission loaded ok, else false.
		int LoadByName (char *szMissionName, int nSubFolder, const char* szSubFolder = NULL);
		int FindByName (char *szMissionName, int nSubFolder);

		int IsBuiltIn (const char* pszMission);
		int LoadLevelStates (void);
		int SaveLevelStates (void);

		inline int NextLevel (int i = 0) { return nNextLevel [i]; }
		inline void SetNextLevel (int nLevel, int i = 0) { nNextLevel [i] = nLevel; }
		void AdvanceLevel (int nNextLevel = 0);

		inline int LastLevel (void) { return nLastLevel; }
		inline int LastSecretLevel (void) { return nLastSecretLevel; }

		void DeleteLevelStates (void);

		inline int GetLevelState (int nLevel) {
			return ((nLevel < 1) || (nLevel > nLastLevel)) ? 0 : nLevelState [nLevel]; // 0: not yet been there, -1: destroyed, 1: been there already
			}

		inline void SetLevelState (int nLevel, char nState) {
			if ((nLevel >= 1) && (nLevel <= nLastLevel))
				nLevelState [nLevel] = nState;
			}

		char* LevelStateName (char* szFile, int nLevel = 0); 

		inline tMsnListEntry& operator[] (const int i) { return m_list [i]; }

	private:
		int LoadD1 (int nMission);
		int LoadShareware (int nMission);
		int LoadOEM (int nMission);
		int ReadFile (const char *filename, int count, int location);
		void AddBuiltinD1Mission (void);
		void AddBuiltinD2XMission (void);
		void AddBuiltinMission (void);
		void Add (int anarchy_mode, int bD1Mission, int bSubFolder, int bHaveSubFolders, int nLocation);
		void Promote (const char* szMissionName, int& nTopPlace);
		void MoveFolderUp (void);
		void MoveFolderDown (int nSubFolder);
		int Parse (CFile& cf);
		void LoadSongList (char *szFile);
};

extern CMissionManager missionManager;

//------------------------------------------------------------------------------

#define IS_SHAREWARE (missionManager.nBuiltInHogSize [0] == SHAREWARE_MISSION_HOGSIZE)
#define IS_MAC_SHARE (missionManager.nBuiltInHogSize [0] == MAC_SHARE_MISSION_HOGSIZE)
#define IS_D2_OEM		(missionManager.nBuiltInHogSize [0] == OEM_MISSION_HOGSIZE)


//------------------------------------------------------------------------------

static inline bool MsnHasGameVer (const char *pszMission)
{
return (pszMission [0] == '[') && ::isdigit ((ubyte) pszMission [1]) && (pszMission [2] == ']');
}

//------------------------------------------------------------------------------

#endif
