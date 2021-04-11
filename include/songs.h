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

#ifndef _SONGS_H
#define _SONGS_H

#include "maths.h"
#include "carray.h"
#include "cfile.h"

#define MAX_NUM_SONGS         100

#define SONG_TITLE            0
#define SONG_BRIEFING         1
#define SONG_ENDLEVEL         2
#define SONG_ENDGAME          3
#define SONG_CREDITS          4
#define SONG_FIRST_LEVEL_SONG 5
#define SONG_INTER				2

extern int32_t Num_songs, nD1SongNum, nD2SongNum;   //how many MIDI songs

//------------------------------------------------------------------------------

class CRedbook {
	private:
		int32_t	m_bForceRegister;
		int32_t	m_bEnabled;
		int32_t	m_bPlaying;
		fix	m_xLastCheck;

	public:
		CRedbook () { Init (); }
		~CRedbook () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Register (void);
		void SetVolume (int32_t volume);
		int32_t PlayTrack (int32_t nTrack, int32_t bKeepPlaying);
		void CheckRepeat (void);
		void ReInit (void);
		void Stop (void);
		int32_t HaveD2CD (void);
		inline void ForceRegister (void) { m_bForceRegister = 1; }
		inline int32_t Enabled (void) { return m_bEnabled; }
		inline void Enable (int32_t bEnabled) { m_bEnabled = bEnabled; }
		inline int32_t Playing (void) { return m_bPlaying; }
		inline void SetPlaying (int32_t bPlaying) { m_bPlaying = bPlaying; }
};

extern CRedbook redbook;

//------------------------------------------------------------------------------

class CPlaylist {
	public:
		CArray<char*>		m_levelSongs;
		CArray<int32_t>			m_songIndex;
		int16_t					m_nSongs [2];

		static char m_szDefaultPlaylist [FILENAME_LEN];

	public:
		CPlaylist ();
		int32_t Size (void);
		void Shuffle (void);
		void Sort (void);
		void Align (void);
		int32_t SongIndex (int32_t nLevel);
		int32_t Load (char* pszFolder, char *pszPlaylist = m_szDefaultPlaylist);
		void Destroy (int32_t* nSongs = NULL);
		const char* LevelSong (int32_t nLevel);
		int32_t PlayLevelSong (int32_t nSong, int32_t bD1 = 0);
	};

class CSongData {
	public:
		char					filename [16];
		char					melodicBankFile [16];
		char					drumBankFile [16];
	};

class CSongInfo {
	public:
		CSongData			data [MAX_NUM_SONGS];
		int32_t				songIndex [2][MAX_NUM_SONGS];
		int32_t				bInitialized;
		int32_t				bPlaying;
		int32_t				nTotalSongs;
		int32_t				nSongs [2];
		int32_t				nFirstLevelSong [2];
		int32_t				nLevelSongs [2];
		int32_t				nCurrent;
		int32_t				nLevel;
		int32_t				nD1EndLevelSong;
		time_t				tStart;
		time_t				tSlowDown;
		time_t				tPos;
		char					szIntroSong [FILENAME_LEN];
		char					szBriefingSong [FILENAME_LEN];
		char					szCreditsSong [FILENAME_LEN];
		char					szMenuSong [FILENAME_LEN];

	public:
		inline int32_t SongIndex (int32_t nSong, int32_t bD1) { return songIndex [bD1][nSong % nLevelSongs [bD1]]; }
	};

class CSongManager {
	private:
		CSongInfo	m_info;
		CPlaylist	m_descent [2];
		CPlaylist	m_user;
		CPlaylist	m_mod;

	public:
		CSongManager () { Init (); }
		~CSongManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		void Setup (void);
		void CheckRepeat (void);
		void StopAll (void);
		int32_t PlayCustomSong (char* pszFolder, char* pszSong, int32_t bLoop);
		void Play (int32_t nSong, int32_t repeat);
		int32_t PlayCustomLevelSong (char* pszFolder, int32_t nLevel);
		void PlayLevelSong (int32_t nLevel, int32_t bFromHog, bool bWaitForThread = true);
		void PlayCurrent (int32_t repeat = 0);
		void Prev (void);
		void Next (void);
		void Shuffle (void);
		void Sort (void);
		void Align (void);
		int32_t LoadDescentSongs (char *pszFile, int32_t bD1Songs);
		int32_t LoadDescentPlaylists (void);
		int32_t LoadUserPlaylist (char *pszPlaylist);
		int32_t LoadModPlaylist (void);
		inline void DestroyPlaylist (int32_t* nSongs = NULL) {
			m_mod.Destroy (nSongs);
			}
		inline void DestroyPlaylists (void) {
			m_mod.Destroy ();
			}
		inline int32_t Current (void) { return m_info.nCurrent; }
		inline int32_t Playing (void) { return m_info.bPlaying; }
		inline void SetPlaying (int32_t bPlaying) { m_info.bPlaying = bPlaying; }
		inline time_t Pos (void) { return m_info.tPos; }
		inline time_t Start (void) { return m_info.tStart; }
		inline time_t SlowDown (void) { return m_info.tSlowDown; }
		inline void SetPos (time_t t) { m_info.tPos = t; }
		inline void SetStart (time_t t) { m_info.tStart = t; }
		inline void SetSlowDown (time_t t) { m_info.tSlowDown = t; }
		inline char* IntroSong (void) { return m_info.szIntroSong; }
		inline char* BriefingSong (void) { return m_info.szBriefingSong; }
		inline char* CreditsSong (void) { return m_info.szCreditsSong; }
		inline char* MenuSong (void) { return m_info.szMenuSong; }
		inline int32_t TotalCount (void) { return m_info.nTotalSongs; }
		inline int32_t Count (uint32_t i) { return m_info.nSongs [i]; }
		inline CSongData& SongData (uint32_t i = 0) { return m_info.data [i]; }

	};

extern CSongManager songManager;

#endif /* _SONGS_H */
