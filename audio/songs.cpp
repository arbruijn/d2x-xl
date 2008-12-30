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

#include "inferno.h"
#include "carray.h"
#include "error.h"
#include "args.h"
#include "rbaudio.h"
#include "kconfig.h"
#include "timer.h"
#include "hogfile.h"
#include "midi.h"
#include "songs.h"

char CDROM_dir[40] = ".";


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

int CD_blast_mixer (void);

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
RBAStop ();    // stop song on exit
}

//------------------------------------------------------------------------------
//stop the redbook, so we can read off the CD

void CRedbook::Register (void)
{
if (m_bForceRegister) {
	RBARegisterCD ();			//get new track list for new CD
	m_bForceRegister = 0;
	}
}

//------------------------------------------------------------------------------
//takes volume in range 0..8
void CRedbook::SetVolume (int volume)
{
RBASetVolume (0);		// makes the macs sound really funny
RBASetVolume (volume * REDBOOK_VOLUME_SCALE / 8);
}

//------------------------------------------------------------------------------

void CRedbook::ReInit (void)
{
RBAInit ();
if (RBAEnabled ()) {
	SetVolume (gameConfig.nRedbookVolume);
	RBARegisterCD ();
	m_bForceRegister = 0;
	}
}

//------------------------------------------------------------------------------
//returns 1 if track started sucessfully
//start at nTrack.  if bKeepPlaying set, play to end of disc.  else
//play only specified track

int CRedbook::PlayTrack (int nTrack, int bKeepPlaying)
{
m_bPlaying = 0;
#if DBG
nTrack = 1;
#endif

if (!RBAEnabled () && redbook.Enabled () && !gameOpts->sound.bUseRedbook)
	redbook.ReInit ();
Register ();			//get new track list for new CD
if (redbook.Enabled () && RBAEnabled ()) {
	int nTracks = RBAGetNumberOfTracks ();
	if (nTrack <= nTracks)
		if (RBAPlayTracks(nTrack, bKeepPlaying ? nTracks : nTrack))  {
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
if (currentTime < m_xLastCheck || (currentTime - m_xLastCheck) >= I2X (2)) {
	if (!RBAPeekPlayStatus ()) {
		StopTime ();
		// if title ends, start credit music
		// if credits music ends, restart it
		if (m_bPlaying == REDBOOK_TITLE_TRACK || m_bPlaying == REDBOOK_CREDITS_TRACK)
			PlayTrack (REDBOOK_CREDITS_TRACK, 0);
		else {

			//new code plays all tracks to end of disk, so if disk has
			//stopped we must be at end.  So start again with level 1 song.

			songManager.PlayLevel (1, 0);
			}
		StartTime (0);
		}
	m_xLastCheck = currentTime;
	}
}

//------------------------------------------------------------------------------

void CRedbook::Stop (void)
{
	int oldVolume = gameConfig.nRedbookVolume * REDBOOK_VOLUME_SCALE / 8;
	fix oldTime = TimerGetFixedSeconds ();

if (m_bPlaying) {		//fade out volume
	int newVolume;
	do {
		fix t = TimerGetFixedSeconds ();

		newVolume = FixMulDiv (oldVolume, (FADE_TIME - (t-oldTime)), FADE_TIME);
		if (newVolume < 0)
			newVolume = 0;
		RBASetVolume (newVolume);
	} while (newVolume > 0);
}
RBAStop ();              	// Stop CD, if playing
RBASetVolume (oldVolume);	//restore volume
m_bPlaying = 0;
}

//------------------------------------------------------------------------------

#if 1
int CRedbook::HaveD2CD (void)
{
	int discid;

if (!redbook.Enabled ())
	return 0;
discid = RBAGetDiscID ();
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

int SongsHaveD2CD ()
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

void CSongManager::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
m_user.nLevelSongs = 0;
m_user.nCurrentSong = 0;
m_user.bMP3 = 0;
*m_user.szIntroSong = '\0';
*m_user.szBriefingSong = '\0';
*m_user.szCreditsSong = '\0';
*m_user.szMenuSong = '\0';
}

//------------------------------------------------------------------------------

void CSongManager::Destroy (void)
{
songManager.FreeUserSongs ();
}

//------------------------------------------------------------------------------

void CSongManager::Setup (void)
{
	int	i, bD1Songs;
	char	*p, inputline [81];
	CFile	cf;

if (m_info.bInitialized)
	return;
hogFileManager.UseD1 ("descent.hog");
for (i = 0, bD1Songs = 0; bD1Songs < 2; bD1Songs++) {
		if (!FindArg ("-nomixer"))
			CD_blast_mixer ();   // Crank it!
	if (CFile::Exist ("descent.sng", gameFolders.szDataDir, bD1Songs)) {   // mac (demo?) datafiles don't have the .sng file
		if (!cf.Open ("descent.sng", gameFolders.szDataDir, "rb", bD1Songs)) {
			if (bD1Songs)
				break;
			else
				Error ("Couldn't open descent.sng");
			}
		while (cf.GetS (inputline, 80)) {
			if ((p = strchr (inputline,'\n')))
				*p = '\0';
			if (*inputline) {
				Assert(i < MAX_NUM_SONGS);
				if (3 == sscanf (inputline, "%s %s %s",
									 m_info.data [i].filename,
									 m_info.data [i].melodicBankFile,
									 m_info.data [i].drumBankFile)) {
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
			Error ("Descent 1 songs are missing.");
		cf.Close ();
		}
	m_info.nTotalSongs = i;
	m_info.bInitialized = 1;
	//	RBA Hook
	if (!gameOpts->sound.bUseRedbook)
		redbook.Enable (0);
	else {	// use redbook
			RBAInit ();
		if (RBAEnabled ()) {
			redbook.SetVolume (gameConfig.nRedbookVolume);
			redbook.ForceRegister ();
			redbook.Register ();
			}
		}
	}
}

//------------------------------------------------------------------------------
//stop any songs - midi or redbook - that are currently playing

void CSongManager::StopAll (void)
{
audio.StopCurrentSong ();	// Stop midi song, if playing
redbook.Stop ();			// Stop CD, if playing
}

//------------------------------------------------------------------------------

void CSongManager::Play (int nSong, int repeat)
{
//Assert(nSong != SONG_ENDLEVEL && nSong != SONG_ENDGAME);	//not in full version
if (!m_info.bInitialized)
	songManager.Setup ();
//stop any music already playing
if (!(redbook.Enabled () ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume))
	return;
StopAll ();
//do we want any of these to be redbook songs?
m_info.nCurrent = nSong;
if (nSong == SONG_TITLE) {
	if (*m_user.szIntroSong && midi.PlaySong (m_user.szIntroSong, NULL, NULL, repeat, 0))
		return;
	redbook.PlayTrack (REDBOOK_TITLE_TRACK, 0);
	}
else if (nSong == SONG_CREDITS) {
	if (*m_user.szCreditsSong && midi.PlaySong (m_user.szCreditsSong, NULL, NULL, repeat, 0))
		return;
	redbook.PlayTrack (REDBOOK_CREDITS_TRACK, 0);
	}
else if (nSong == SONG_BRIEFING) {
	if (*m_user.szBriefingSong && midi.PlaySong (m_user.szBriefingSong, NULL, NULL, repeat, 0))
		return;
	}
if (!m_info.bPlaying) {		//not playing redbook, so play midi
	midi.PlaySong (
		m_info.data [nSong].filename,
		m_info.data [nSong].melodicBankFile,
		m_info.data [nSong].drumBankFile,
		repeat, m_info.nSongs [1] && (nSong >= m_info.nSongs [0]));
	}
}

//------------------------------------------------------------------------------

void CSongManager::PlayCurrent (int repeat)
{
songManager.Play (m_info.nCurrent, repeat);
}

//------------------------------------------------------------------------------

void CSongManager::PlayLevel (int nLevel, int bFromHog)
{
	int	nSong;
	int	nTracks;
	int	bD1Song = (gameData.missions.list [gameData.missions.nCurrentMission].nDescentVersion == 1);
	char	szFilename [FILENAME_LEN];

if (!nLevel)
	return;
if (!m_info.bInitialized)
	Setup ();
StopAll ();
m_info.nLevel = nLevel;
nSong = (nLevel > 0) ? nLevel - 1 : -nLevel;
m_info.nCurrent = nSong;
if (!RBAEnabled () && redbook.Enabled () && gameOpts->sound.bUseRedbook)
	redbook.ReInit ();
redbook.Register ();
if (bFromHog) {
	CFile	cf;
	strcpy (szFilename, LevelSongName (nLevel));
	if (*szFilename && cf.Extract (szFilename, gameFolders.szDataDir, 0, szFilename)) {
		char	szSong [FILENAME_LEN];

		sprintf (szSong, "%s%s%s", gameFolders.szCacheDir, *gameFolders.szCacheDir ? "/" : "", szFilename);
		if (midi.PlaySong (szSong, NULL, NULL, 1, 0))
			return;
		}
	}
if ((nLevel > 0) && m_user.nLevelSongs) {
	if (midi.PlaySong (m_user.levelSongs [(nLevel - 1) % m_user.nLevelSongs], NULL, NULL, 1, 0))
		return;
	}
if (redbook.Enabled () && RBAEnabled () && (nTracks = RBAGetNumberOfTracks ()) > 1)	//try to play redbook
	redbook.PlayTrack (REDBOOK_FIRST_LEVEL_TRACK + (nSong % (nTracks - REDBOOK_FIRST_LEVEL_TRACK + 1)) , 1);
if (!redbook.Playing ()) {			//not playing redbook, so play midi
	nSong = m_info.nLevelSongs [bD1Song] ? m_info.nFirstLevelSong [bD1Song] + (nSong % m_info.nLevelSongs [bD1Song]) : 0;
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
	m_info.nLevel = RBAGetTrackNum () - REDBOOK_FIRST_LEVEL_TRACK + 1;
PlayLevel (m_info.nLevel + 1, 0);
}

//------------------------------------------------------------------------------
//goto the previous level song
void CSongManager::Prev (void)
{
if (m_info.bPlaying) 		//get correct track
	m_info.nLevel = RBAGetTrackNum () - REDBOOK_FIRST_LEVEL_TRACK + 1;
if (m_info.nLevel > 1)
	songManager.PlayLevel(m_info.nLevel - 1, 0);
}

//------------------------------------------------------------------------------

int CSongManager::LoadPlayList (char *pszPlayList)
{
	CFile	cf;
	char	szSong [FILENAME_LEN], szListFolder [FILENAME_LEN], szSongFolder [FILENAME_LEN], *pszSong;
	int	l, bRead, nSongs, bMP3;

CFile::SplitPath (pszPlayList, szListFolder, NULL, NULL);
for (l = (int) strlen (pszPlayList) - 1; (l >= 0) && isspace (pszPlayList [l]); l--)
	;
pszPlayList [++l] = '\0';
for (bRead = 0; bRead < 2; bRead++) {
	if (!cf.Open (pszPlayList, "", "rt", 0))
		return 0;
	nSongs = 0;
	while (cf.GetS (szSong, sizeof (szSong))) {
		if ((bMP3 = (strstr (szSong, ".mp3") != NULL)) || strstr (szSong, ".ogg")) {
			if (bRead) {
				if (bMP3)
					m_user.bMP3 = 1;
				if ((pszSong = strchr (szSong, '\r')))
					*pszSong = '\0';
				if ((pszSong = strchr (szSong, '\n')))
					*pszSong = '\0';
				l = (int) strlen (szSong) + 1;
				CFile::SplitPath (szSong, szSongFolder, NULL, NULL);
				if (!*szSongFolder)
					l += (int) strlen (szListFolder);
				if (!(pszSong = new char [l])) {
					cf.Close ();
					return nSongs = nSongs;
					}
				if (*szSongFolder)
					memcpy (pszSong, szSong, l);
				else
					sprintf (pszSong, "%s%s", szListFolder, szSong);
				m_user.levelSongs [nSongs] = pszSong;
				}
			nSongs++;
			}
		}
	cf.Close ();
	if (!bRead) {
		if (!m_user.levelSongs.Create (nSongs))
			return 0;
		}
	}
return m_user.nLevelSongs = nSongs;
}

//------------------------------------------------------------------------------

void CSongManager::FreeUserSongs (void)
{
	int	i;

for (i = 0; i < m_user.nLevelSongs; i++)
	delete[] m_user.levelSongs [i];
m_user.levelSongs.Destroy ();
m_user.nLevelSongs = 0;
}

//------------------------------------------------------------------------------
//eof
