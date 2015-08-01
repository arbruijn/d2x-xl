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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "descent.h"
#include "carray.h"
#include "error.h"
#include "args.h"
#include "rbaudio.h"
#include "kconfig.h"
#include "timer.h"
#include "hogfile.h"
#include "midi.h"
#include "songs.h"
#include "soundthreads.h"

char CDROM_dir[40] = ".\\";

char CPlaylist::m_szDefaultPlaylist [FILENAME_LEN];

//------------------------------------------------------------------------------
/*
 * Some of these have different Track listings!
 * Which one is the "correct" order?
 */
#define D2_1_DISCID         0x7d0ff809 // Descent II
#define D2_2_DISCID         0xe010a30e // Descent II
#define D2_3_DISCID         0xd410070d // Descent II
#define D2_4_DISCID         0xc610080d // Descent II
#define D2_DEF_DISCID       0x87102209 // Definitive collection Disc 2
#define D2_OEM_DISCID       0xac0bc30d // Destination: Quartzon
#define D2_OEM2_DISCID      0xc40c0a0d // Destination: Quartzon
#define D2_VERTIGO_DISCID   0x53078208 // Vertigo
#define D2_VERTIGO2_DISCID  0x64071408 // Vertigo + DMB
#define D2_MAC_DISCID       0xb70ee40e // Macintosh
#define D2_IPLAY_DISCID     0x22115710 // iPlay for Macintosh

#define REDBOOK_TITLE_TRACK         2
#define REDBOOK_CREDITS_TRACK       3
#define REDBOOK_FIRST_LEVEL_TRACK   (redbook.HaveD2CD () ? 4 : 1)

#define REDBOOK_VOLUME_SCALE  (255/3)		//255 is MAX

#define FADE_TIME (I2X (1)/2)

//------------------------------------------------------------------------------

int32_t CD_blast_mixer (void);

CRedbook redbook;
CSongManager songManager;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CRedbook::Init (void)
{
m_bForceRegister = 0;
m_bEnabled = 1;
m_bPlaying = 0;
m_xLastCheck = 0;
}

//------------------------------------------------------------------------------

void CRedbook::Destroy (void)
{
rba.Stop ();    // stop song on exit
}

//------------------------------------------------------------------------------
//stop the redbook, so we can read off the CD

void CRedbook::Register (void)
{
if (m_bForceRegister) {
	rba.RegisterCD ();			//get new track list for new CD
	m_bForceRegister = 0;
	}
}

//------------------------------------------------------------------------------
//takes volume in range 0..8
void CRedbook::SetVolume (int32_t volume)
{
rba.SetVolume (0);		// makes the macs sound really funny
rba.SetVolume (volume * REDBOOK_VOLUME_SCALE / 8);
}

//------------------------------------------------------------------------------

void CRedbook::ReInit (void)
{
rba.Init ();
if (rba.Enabled ()) {
	SetVolume (gameConfig.nRedbookVolume);
	rba.RegisterCD ();
	m_bForceRegister = 0;
	}
}

//------------------------------------------------------------------------------
//returns 1 if track started sucessfully
//start at nTrack.  if bKeepPlaying set, play to end of disc.  else
//play only specified track

int32_t CRedbook::PlayTrack (int32_t nTrack, int32_t bKeepPlaying)
{
m_bPlaying = 0;
#if DBG
nTrack = 1;
#endif

if (!rba.Enabled () && redbook.Enabled () && !gameOpts->sound.bUseRedbook)
	redbook.ReInit ();
Register ();			//get new track list for new CD
if (redbook.Enabled () && rba.Enabled ()) {
	int32_t nTracks = rba.GetNumberOfTracks ();
	if (nTrack <= nTracks)
		if (rba.PlayTracks (nTrack, bKeepPlaying ? nTracks : nTrack))  {
			m_bPlaying = nTrack;
			}
	}
return (m_bPlaying != 0);
}

//------------------------------------------------------------------------------
//this should be called regularly to check for redbook restart
void CRedbook::CheckRepeat (void)
{
	fix currentTime;

if (!m_bPlaying || (gameConfig.nRedbookVolume == 0))
	return;

currentTime = TimerGetFixedSeconds ();
if ((currentTime < m_xLastCheck) || ((currentTime - m_xLastCheck) >= I2X (2))) {
	if (!rba.PeekPlayStatus ()) {
		StopTime ();
		// if title ends, start credit music
		// if credits music ends, restart it
		if ((m_bPlaying == REDBOOK_TITLE_TRACK) || (m_bPlaying == REDBOOK_CREDITS_TRACK))
			PlayTrack (REDBOOK_CREDITS_TRACK, 0);
		else {

			//new code plays all tracks to end of disk, so if disk has
			//stopped we must be at end.  So start again with level 1 song.

			songManager.PlayLevelSong (1, 0);
			}
		StartTime (0);
		}
	m_xLastCheck = currentTime;
	}
}

//------------------------------------------------------------------------------

void CRedbook::Stop (void)
{
	int32_t oldVolume = gameConfig.nRedbookVolume * REDBOOK_VOLUME_SCALE / 8;
	fix oldTime = TimerGetFixedSeconds ();

if (m_bPlaying) {		//fade out volume
	int32_t newVolume;
	do {
		fix t = TimerGetFixedSeconds ();

		newVolume = FixMulDiv (oldVolume, (FADE_TIME - (t-oldTime)), FADE_TIME);
		if (newVolume < 0)
			newVolume = 0;
		rba.SetVolume (newVolume);
	} while (newVolume > 0);
}
if (audio.Available ()) {
	rba.Stop ();              	// Stop CD, if playing
	rba.SetVolume (oldVolume);	//restore volume
	}
m_bPlaying = 0;
}

//------------------------------------------------------------------------------

#if 1
int32_t CRedbook::HaveD2CD (void)
{
	int32_t discid;

if (!redbook.Enabled ())
	return 0;
discid = rba.GetDiscID ();
switch (discid) {
	case D2_1_DISCID:
	case D2_2_DISCID:
	case D2_3_DISCID:
	case D2_4_DISCID:
	case D2_DEF_DISCID:
	case D2_OEM_DISCID:
	case D2_OEM2_DISCID:
	case D2_VERTIGO_DISCID:
	case D2_VERTIGO2_DISCID:
	case D2_MAC_DISCID:
	case D2_IPLAY_DISCID:
		//printf("Found D2 CD! discid: %x\n", discid);
		return 1;
	default:
		//printf("Unknown CD! discid: %x\n", discid);
		return 0;
	}
}

#else

int32_t SongsHaveD2CD ()
{
	char temp [128],cwd [128];

getcwd (cwd, 128);
strcpy (temp, CDROM_dir);
if (temp [strlen (temp) - 1] == '\\')
	temp [strlen (temp) - 1] = 0;
if (!chdir (temp)) {
	chdir (cwd);
	return 1;
	}
return 0;
}
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static void ShuffleIntegers (int32_t* v, int32_t l)
{
if ((l < 1) || !v)
	return;
int32_t* index = new int32_t [l];
if (!index)
	return;
for (int32_t i = 0; i < l; i++)
	index [i] = i;
while (l) {
	int32_t i = Rand (l);
	v [--l] = index [i];
	if (i < l)
		index [i] = index [l];
	}
}

//------------------------------------------------------------------------------

static void SortIntegers (int32_t* v, int32_t l)
{
if ((l < 1) || !v)
	return;
while (--l >= 0)
	v [l] = l;
}

//------------------------------------------------------------------------------

CPlaylist::CPlaylist ()
{
strcpy (m_szDefaultPlaylist, "playlist.txt");
}

//------------------------------------------------------------------------------

int32_t CPlaylist::Size (void)
{
return m_levelSongs.Length ();
}

//------------------------------------------------------------------------------

void CPlaylist::Shuffle (void)
{
ShuffleIntegers (m_songIndex.Buffer (), m_songIndex.Length ());
}

//------------------------------------------------------------------------------

void CPlaylist::Sort (void)
{
SortIntegers (m_songIndex.Buffer (), m_songIndex.Length ());
}

//------------------------------------------------------------------------------

void CPlaylist::Align (void)
{
if (gameOpts->sound.bShuffleMusic)
	Shuffle ();
else
	Sort ();
}

//------------------------------------------------------------------------------

static inline int32_t Wrap (int32_t i, int32_t l)
{
return (i < 0) ? i + l : i % l;
}

int32_t CPlaylist::SongIndex (int32_t nLevel)
{
int32_t l = m_songIndex.Length ();
if (l < 1)
	return -1;
if (nLevel >= 0)
	return m_nSongs [0] ? m_songIndex [Wrap (Max (nLevel - 1, 0), l)] : -1;
// nLevel < 0 denotes a secret level.
// Secret level song indices are stored in reverse order at the end of the song index.
return m_nSongs [1] ? m_songIndex [Wrap (l + nLevel, l)] : -1;
}

//------------------------------------------------------------------------------

const char* CPlaylist::LevelSong (int32_t nLevel)
{ 
int32_t i = SongIndex (nLevel);
return (i < 0) ? "" : m_levelSongs [i]; 
}

//------------------------------------------------------------------------------

int32_t CPlaylist::PlayLevelSong (int32_t nLevel, int32_t bD1)
{
return Size () && midi.PlaySong (LevelSong (nLevel), NULL, NULL, 1, bD1);
}

//------------------------------------------------------------------------------

int32_t CPlaylist::Load (char* pszFolder, char* pszPlaylist)
{
	CFile	cf;
	char	szPlaylist [FILENAME_LEN], szSong [FILENAME_LEN], szListFolder [FILENAME_LEN], szSongFolder [FILENAME_LEN];
	int32_t	bRead, nSongs [3], i;

if (pszFolder && *pszFolder) {
	strcpy (szListFolder, pszFolder);
	sprintf (szPlaylist, "%s%s", szListFolder, pszPlaylist);
	pszPlaylist = szPlaylist;
	}
else
	CFile::SplitPath (pszPlaylist, szListFolder, NULL, NULL);

for (i = (int32_t) strlen (pszPlaylist) - 1; (i >= 0) && isspace (pszPlaylist [i]); i--)
	;
pszPlaylist [++i] = '\0';

for (bRead = 0; bRead < 2; bRead++) {
	if (!cf.Open (pszPlaylist, "", "rb", 0))
		return 0;
	nSongs [0] =
	nSongs [1] =
	nSongs [2] = 0;
	while (cf.GetS (szSong, sizeof (szSong))) {
		if (strstr (szSong, ".ogg") || strstr (szSong, ".flac")) {
			int32_t bSecret = (*szSong == '-');
			if (bRead) {
				char* pszSong;
				if ((pszSong = strchr (szSong, '\r')))
					*pszSong = '\0';
				if ((pszSong = strchr (szSong, '\n')))
					*pszSong = '\0';
				i = (int32_t) strlen (szSong) + 1;
				CFile::SplitPath (szSong, szSongFolder, NULL, NULL);
				if (!*szSongFolder)
					i += (int32_t) strlen (szListFolder);
				pszSong = new char [i - bSecret];
				if (!pszSong) {
					cf.Close ();
					Destroy ();
					return 0;
					}
				if (*szSongFolder)
					memcpy (pszSong, szSong + bSecret, i - bSecret);
				else
					sprintf (pszSong, "%s%s", szListFolder, szSong + bSecret);
				i = bSecret ? m_levelSongs.Length () - nSongs [1] - 1 : nSongs [0];
				m_levelSongs [i] = pszSong;
				m_songIndex [i] = nSongs [2];
				pszSong = NULL;
				}
			nSongs [bSecret]++;
			nSongs [2]++;
			}
		}
	cf.Close ();
	if (!bRead && nSongs [2]) {
		m_levelSongs.Destroy ();
		if (!m_levelSongs.Create (nSongs [2])) {
			Destroy ();
			return 0;
			}
		m_songIndex.Create (nSongs [2]);
		m_levelSongs.Clear (0);
		}
	}

m_nSongs [0] = nSongs [0];
m_nSongs [1] = nSongs [1];
return nSongs [2];
}

//------------------------------------------------------------------------------

void CPlaylist::Destroy (int32_t* nSongs)
{
if (m_levelSongs.Buffer ()) {
	int32_t j = nSongs ? *nSongs : Size ();
	for (int32_t i = 0; i < j; i++) {
		if (m_levelSongs [i])
			delete[] m_levelSongs [i];
		}
	m_levelSongs.Destroy ();
	m_songIndex.Destroy ();
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CSongManager::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
m_info.nCurrent = 0;
*m_info.szIntroSong = '\0';
*m_info.szBriefingSong = '\0';
*m_info.szCreditsSong = '\0';
*m_info.szMenuSong = '\0';
}

//------------------------------------------------------------------------------

void CSongManager::Destroy (void)
{
songManager.DestroyPlaylists ();
}

//------------------------------------------------------------------------------

void CSongManager::Shuffle (void)
{
for (int32_t i = 0; i < 2; i++)
	ShuffleIntegers (m_info.songIndex [i], m_info.nLevelSongs [i]);
ShuffleIntegers (missionManager.songIndex, missionManager.nSongs);
}

//------------------------------------------------------------------------------

void CSongManager::Sort (void)
{
for (int32_t i = 0; i < 2; i++)
	SortIntegers (m_info.songIndex [i], m_info.nLevelSongs [i]);
SortIntegers (missionManager.songIndex, missionManager.nSongs);
}

//------------------------------------------------------------------------------

void CSongManager::Align (void)
{
if (gameOpts->sound.bShuffleMusic)
	Shuffle ();
else
	Sort ();
m_descent [0].Align ();
m_descent [1].Align ();
m_user.Align ();
m_mod.Align ();
}

//------------------------------------------------------------------------------

void CSongManager::Setup (void)
{
	int32_t	i, bD1Songs;
	char	*p, inputline [81];
	CFile	cf;

if (m_info.bInitialized)
	return;
hogFileManager.UseD1 ("descent.hog");
for (i = 0, bD1Songs = 0; bD1Songs < 2; bD1Songs++) {
		if (!FindArg ("-nomixer"))
			CD_blast_mixer ();   // Crank it!
	if (CFile::Exist ("descent.sng", gameFolders.game.szData [0], bD1Songs)) {   // mac (demo?) datafiles don't have the .sng file
		if (!cf.Open ("descent.sng", gameFolders.game.szData [0], "rb", bD1Songs)) {
			if (bD1Songs)
				break;
			else
				Warning ("Couldn't open descent.sng");
			}
		while (cf.GetS (inputline, 80)) {
			if ((p = strchr (inputline,'\n')))
				*p = '\0';
			if (*inputline) {
				Assert(i < MAX_NUM_SONGS);
				if (3 == sscanf (inputline, "%s %s %s", m_info.data [i].filename, m_info.data [i].melodicBankFile, m_info.data [i].drumBankFile)) {
					if (!m_info.nFirstLevelSong [bD1Songs] && strstr (m_info.data [i].filename, "game01.hmp"))
						 m_info.nFirstLevelSong [bD1Songs] = i;
					if (bD1Songs && strstr (m_info.data [i].filename, "endlevel.hmp"))
						m_info.nD1EndLevelSong = i;
					i++;
					}
				}
			}
		m_info.nSongs [bD1Songs] = i;
		m_info.nLevelSongs [bD1Songs] = m_info.nSongs [bD1Songs] - m_info.nFirstLevelSong [bD1Songs];
		if (!m_info.nFirstLevelSong [bD1Songs])
			Warning ("Descent 1 songs are missing.");
		cf.Close ();
		}
	m_info.nTotalSongs = i;
	m_info.bInitialized = 1;
	//	rba. Hook
	if (!gameOpts->sound.bUseRedbook)
		redbook.Enable (0);
	else {	// use redbook
		rba.Init ();
		if (rba.Enabled ()) {
			redbook.SetVolume (gameConfig.nRedbookVolume);
			redbook.ForceRegister ();
			redbook.Register ();
			}
		}
	}
LoadDescentPlaylists ();
Align ();
}

//------------------------------------------------------------------------------
//stop any songs - midi or redbook - that are currently playing

void CSongManager::StopAll (void)
{
audio.StopCurrentSong ();	// Stop midi song, if playing
redbook.Stop ();			// Stop CD, if playing
}

//------------------------------------------------------------------------------

int32_t CSongManager::PlayCustomSong (char* pszFolder, char* pszSong, int32_t bLoop)
{
	char	szFilename [FILENAME_LEN];

if (pszFolder && *pszFolder) {
	sprintf (szFilename, "%s%s.ogg", pszFolder, pszSong);
	pszSong = szFilename;
	}
else if (!*pszSong)
	return m_info.bPlaying = 0;
return m_info.bPlaying = midi.PlaySong (pszSong, NULL, NULL, bLoop, 0);
}

//------------------------------------------------------------------------------

void CSongManager::Play (int32_t nSong, int32_t bLoop)
{
//Assert(nSong != SONG_ENDLEVEL && nSong != SONG_ENDGAME);	//not in full version
if (!m_info.bInitialized)
	songManager.Setup ();
//stop any music already playing
if (!(redbook.Enabled () ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume))
	return;
StopAll ();
WaitForSoundThread ();
//do we want any of these to be redbook songs?
m_info.nCurrent = nSong;
if (nSong == SONG_TITLE) {
	if (PlayCustomSong (gameFolders.mods.szMusic, const_cast<char*>("title"), bLoop))
		return;
	if (PlayCustomSong (NULL, m_info.szIntroSong, bLoop))
		return;
	if (PlayCustomSong (gameFolders.game.szMusic [gameStates.app.bD1Mission], const_cast<char*>("title"), bLoop))
		return;
	m_info.bPlaying = redbook.PlayTrack (REDBOOK_TITLE_TRACK, 0);
	}
else if (nSong == SONG_CREDITS) {
	if (PlayCustomSong (gameFolders.mods.szMusic, const_cast<char*>("credits"), bLoop))
		return;
	if (PlayCustomSong (NULL, m_info.szCreditsSong, bLoop))
		return;
	if (PlayCustomSong (gameFolders.game.szMusic [gameStates.app.bD1Mission], const_cast<char*>("credits"), bLoop))
		return;
	m_info.bPlaying = redbook.PlayTrack (REDBOOK_CREDITS_TRACK, 0);
	}
else if (nSong == SONG_BRIEFING) {
	if (PlayCustomSong (gameFolders.mods.szMusic, const_cast<char*>("briefing"), bLoop))
		return;
	if (PlayCustomSong (NULL, m_info.szBriefingSong, bLoop))
		return;
	if (PlayCustomSong (gameFolders.game.szMusic [gameStates.app.bD1Mission], const_cast<char*>("briefing"), bLoop))
		return;
	}
if (!m_info.bPlaying) {		//not playing redbook, so play midi
	midi.PlaySong (m_info.data [nSong].filename,
						m_info.data [nSong].melodicBankFile,
						m_info.data [nSong].drumBankFile,
						bLoop, gameStates.app.bD1Mission || (m_info.nSongs [1] && (nSong >= m_info.nSongs [0])));
	}
}

//------------------------------------------------------------------------------

void CSongManager::PlayCurrent (int32_t bLoop)
{
songManager.Play (m_info.nCurrent, bLoop);
}

//------------------------------------------------------------------------------

int32_t CSongManager::PlayCustomLevelSong (char* pszFolder, int32_t nLevel)
{
if (*pszFolder) {
	char	szFilename [FILENAME_LEN];

	if (nLevel < 0)
		sprintf (szFilename, "%sslevel%02d.ogg", pszFolder, -nLevel);
	else
		sprintf (szFilename, "%slevel%02d.ogg", pszFolder, nLevel);
	if (midi.PlaySong (szFilename, NULL, NULL, 1, 0))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void CSongManager::PlayLevelSong (int32_t nLevel, int32_t bFromHog, bool bWaitForThread)
{
	int32_t	nSong;
	int32_t	nTracks;
	int32_t	bD1Song = (missionManager [missionManager.nCurrentMission].nDescentVersion == 1);
	char	szFilename [FILENAME_LEN];

if (!nLevel)
	return;
if (!m_info.bInitialized)
	Setup ();
StopAll ();
if (bWaitForThread)
	WaitForSoundThread ();
m_info.nLevel = nLevel;
nSong = (nLevel < 0) ? -nLevel - 1 : nLevel - 1;
m_info.nCurrent = nSong;
if (!rba.Enabled () && redbook.Enabled () && gameOpts->sound.bUseRedbook)
	redbook.ReInit ();
redbook.Register ();

// try the hog file first
if (bFromHog) {
	CFile	cf;
	strcpy (szFilename, LevelSongName (nSong));
	if (*szFilename && cf.Extract (szFilename, gameFolders.game.szData [0], 0, szFilename)) {
		char	szSong [FILENAME_LEN];

		sprintf (szSong, "%s%s", gameFolders.var.szCache, szFilename);
		if (midi.PlaySong (szSong, NULL, NULL, 1, 0))
			return;
		}
	}

if (m_mod.PlayLevelSong (nLevel))
	return;
if (PlayCustomLevelSong (gameFolders.mods.szMusic, nLevel))
	return;
if (m_user.PlayLevelSong (nLevel))
	return;
if (m_descent [bD1Song].PlayLevelSong (nLevel, bD1Song))
	return;
if (PlayCustomLevelSong (gameFolders.game.szMusic [bD1Song], nLevel))
	return;

if (redbook.Enabled () && rba.Enabled () && (nTracks = rba.GetNumberOfTracks ()) > 1)	//try to play redbook
	redbook.PlayTrack (REDBOOK_FIRST_LEVEL_TRACK + (nSong % (nTracks - REDBOOK_FIRST_LEVEL_TRACK + 1)) , 1);
if (!redbook.Playing ()) {			//not playing redbook, so play midi
	nSong = m_info.nLevelSongs [bD1Song] ? m_info.nFirstLevelSong [bD1Song] + m_info.SongIndex (nSong, bD1Song) : 0;
	m_info.nCurrent = nSong;
		midi.PlaySong (
			m_info.data [nSong].filename,
			m_info.data [nSong].melodicBankFile,
			m_info.data [nSong].drumBankFile,
			1,
			bD1Song);
	}
}

//------------------------------------------------------------------------------
//goto the next level song
void CSongManager::Next (void)
{
if (m_info.bPlaying) 		//get correct track
	m_info.nLevel = rba.GetTrackNum () - REDBOOK_FIRST_LEVEL_TRACK + 1;
PlayLevelSong (m_info.nLevel + 1, 0);
}

//------------------------------------------------------------------------------
//goto the previous level song
void CSongManager::Prev (void)
{
if (m_info.bPlaying) 		//get correct track
	m_info.nLevel = rba.GetTrackNum () - REDBOOK_FIRST_LEVEL_TRACK + 1;
if (m_info.nLevel > 1)
	songManager.PlayLevelSong (m_info.nLevel - 1, 0);
}

//------------------------------------------------------------------------------

int32_t CSongManager::LoadDescentPlaylists (void) { 
	return m_descent [0].Load (gameFolders.game.szMusic [0]) & (m_descent [1].Load (gameFolders.game.szMusic [1]) << 1); 
}

int32_t CSongManager::LoadUserPlaylist (char *pszPlaylist) { 
	return m_user.Load (NULL, pszPlaylist); 
}

int32_t CSongManager::LoadModPlaylist (void) { 
	return *gameFolders.mods.szMusic ? m_mod.Load (gameFolders.mods.szMusic) : 0; 
}

//------------------------------------------------------------------------------
//eof

