/* $Id: songs.c,v 1.11 2004/04/14 07:35:55 btb Exp $ */
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
#include <unistd.h>
#endif

#include "inferno.h"
#include "error.h"
#include "pstypes.h"
#include "args.h"
#include "songs.h"
#include "mono.h"
#include "cfile.h"
#include "digi.h"
#include "rbaudio.h"
#include "kconfig.h"
#include "timer.h"
#include "mission.h"
#include "u_mem.h"

extern void DigiStopCurrentSong();


//0 if redbook is no playing, else the track number

extern int CD_blast_mixer();

#define REDBOOK_VOLUME_SCALE  (255/3)		//255 is MAX

//takes volume in range 0..8
void SetRedbookVolume(int volume)
{
	RBASetVolume(0);		// makes the macs sound really funny
	RBASetVolume(volume*REDBOOK_VOLUME_SCALE/8);
}

extern char CDROM_dir [];

//------------------------------------------------------------------------------

void SongsInit()
{
	int	i, bD1Songs;
	char	*p, inputline [81];
	CFILE	cf;

if (gameData.songs.bInitialized) 
	return;
CFUseD1HogFile("descent.hog");
for (i = 0, bD1Songs = 0; bD1Songs < 2; bD1Songs++) {
		if (!FindArg ("-nomixer"))
			CD_blast_mixer();   // Crank it!
	if (CFExist ("descent.sng", gameFolders.szDataDir, bD1Songs)) {   // mac (demo?) datafiles don't have the .sng file
		if (!CFOpen (&cf, "descent.sng", gameFolders.szDataDir, "rb", bD1Songs)) {
			if (bD1Songs)
				break;
			else
				Error ("Couldn't open descent.sng");
			}
		while (CFGetS (inputline, 80, &cf)) {
			if (p = strchr (inputline,'\n'))
				*p = '\0';
			if (*inputline) {
				Assert(i < MAX_NUM_SONGS);
				if (3 == sscanf (inputline, "%s %s %s",
									  gameData.songs.info [i].filename,
									  gameData.songs.info [i].melodicBankFile,
									  gameData.songs.info [i].drumBankFile)) {
					if (!gameData.songs.nFirstLevelSong [bD1Songs] && strstr (gameData.songs.info [i].filename, "game01.hmp"))
						 gameData.songs.nFirstLevelSong [bD1Songs] = i;
					if (bD1Songs && strstr (gameData.songs.info [i].filename, "endlevel.hmp"))
						gameData.songs.nD1EndLevelSong = i;
					i++;
					}
				}
			}
		gameData.songs.nSongs [bD1Songs] = i;
		gameData.songs.nLevelSongs [bD1Songs] = gameData.songs.nSongs [bD1Songs] - gameData.songs.nFirstLevelSong [bD1Songs];
		if (!gameData.songs.nFirstLevelSong [bD1Songs])
			Error("gameData.songs.info are missing.");
		CFClose(&cf);
		}
	gameData.songs.nTotalSongs = i;
	gameData.songs.bInitialized = 1;
	//	RBA Hook
		if (!gameOpts->sound.bUseRedbook)
			gameStates.sound.bRedbookEnabled = 0;
		else {	// use redbook
				RBAInit();
			if (RBAEnabled()) {
				SetRedbookVolume(gameConfig.nRedbookVolume);
				RBARegisterCD();
				}
			}
		atexit(RBAStop);    // stop song on exit
		}
}

#define FADE_TIME (f1_0/2)

//------------------------------------------------------------------------------
//stop the redbook, so we can read off the CD
void SongsStopRedbook(void)
{
	int oldVolume = gameConfig.nRedbookVolume*REDBOOK_VOLUME_SCALE/8;
	fix oldTime = TimerGetFixedSeconds();

	if (gameStates.sound.bRedbookPlaying) {		//fade out volume
		int newVolume;
		do {
			fix t = TimerGetFixedSeconds();

			newVolume = FixMulDiv(oldVolume,(FADE_TIME - (t-oldTime)),FADE_TIME);
			if (newVolume < 0)
				newVolume = 0;
			RBASetVolume(newVolume);
		} while (newVolume > 0);
	}
	RBAStop();              	// Stop CD, if playing
	RBASetVolume(oldVolume);	//restore volume
	gameStates.sound.bRedbookPlaying = 0;	
}

//------------------------------------------------------------------------------
//stop any songs - midi or redbook - that are currently playing
void SongsStopAll(void)
{
DigiStopCurrentSong();	// Stop midi song, if playing
SongsStopRedbook();			// Stop CD, if playing
}

//------------------------------------------------------------------------------

int force_rb_register=0;

void ReInitRedbook()
{
RBAInit();
if (RBAEnabled()) {
	SetRedbookVolume(gameConfig.nRedbookVolume);
	RBARegisterCD();
	force_rb_register=0;
	}
}

//------------------------------------------------------------------------------
//returns 1 if track started sucessfully
//start at tracknum.  if keep_playing set, play to end of disc.  else
//play only specified track
int PlayRedbookTrack(int tracknum,int keep_playing)
{
	gameStates.sound.bRedbookPlaying = 0;
#ifdef _DEBUG
	tracknum = 1;
#endif

	if (!RBAEnabled() && gameStates.sound.bRedbookEnabled && !gameOpts->sound.bUseRedbook)
		ReInitRedbook();

	if (force_rb_register) {
		RBARegisterCD();			//get new track list for new CD
		force_rb_register = 0;
	}

	if (gameStates.sound.bRedbookEnabled && RBAEnabled()) {
		int num_tracks = RBAGetNumberOfTracks();
		if (tracknum <= num_tracks)
			if (RBAPlayTracks(tracknum,keep_playing?num_tracks:tracknum))  {
				gameStates.sound.bRedbookPlaying = tracknum;
			}
	}

	return (gameStates.sound.bRedbookPlaying != 0);
}

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
#define REDBOOK_FIRST_LEVEL_TRACK   (SongsHaveD2CD()?4:1)

// SongsHaveD2CD returns 1 if the descent 2 CD is in the drive and
// 0 otherwise

//------------------------------------------------------------------------------

#if 1
int SongsHaveD2CD()
{
	int discid;

if (!gameStates.sound.bRedbookEnabled)
	return 0;
discid = RBAGetDiscID();
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

int SongsHaveD2CD()
{
	char temp [128],cwd [128];

getcwd(cwd, 128);
strcpy(temp,CDROM_dir);
if (temp [strlen(temp)-1] == '\\')
	temp [strlen(temp)-1] = 0;
if (!chdir(temp)) {
	chdir(cwd);
	return 1;
	}
return 0;
}
#endif

//------------------------------------------------------------------------------

void SongsPlaySong (int nSong, int repeat)
{
//Assert(nSong != SONG_ENDLEVEL && nSong != SONG_ENDGAME);	//not in full version
if (!gameData.songs.bInitialized)
	SongsInit();
//stop any music already playing
if (!(gameStates.sound.bRedbookEnabled ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume))
	return;
SongsStopAll();
//do we want any of these to be redbook songs?
if (force_rb_register) {
	RBARegisterCD ();			//get new track list for new CD
	force_rb_register = 0;
	}
gameStates.sound.nCurrentSong = nSong;
if (nSong == SONG_TITLE) {
	if (*gameData.songs.user.szIntroSong &&
		DigiPlayMidiSong (gameData.songs.user.szIntroSong, NULL, NULL, repeat, 0))
		return;
	PlayRedbookTrack (REDBOOK_TITLE_TRACK, 0);
	}
else if (nSong == SONG_CREDITS) {
	if (*gameData.songs.user.szCreditsSong &&
		 DigiPlayMidiSong (gameData.songs.user.szCreditsSong, NULL, NULL, repeat, 0))
		return;
	PlayRedbookTrack (REDBOOK_CREDITS_TRACK, 0);
	}
else if (nSong == SONG_BRIEFING) {
	if (*gameData.songs.user.szBriefingSong &&
		 DigiPlayMidiSong (gameData.songs.user.szBriefingSong, NULL, NULL, repeat, 0))
		return;
	}
if (!gameStates.sound.bRedbookPlaying) {		//not playing redbook, so play midi
	DigiPlayMidiSong (
		gameData.songs.info [nSong].filename, 
		gameData.songs.info [nSong].melodicBankFile, 
		gameData.songs.info [nSong].drumBankFile, 
		repeat, gameData.songs.nSongs [1] && (nSong >= gameData.songs.nSongs [0]));
	}
}

//------------------------------------------------------------------------------

void SongsPlayCurrentSong (int repeat)
{
SongsPlaySong (gameStates.sound.nCurrentSong, repeat);
}

//------------------------------------------------------------------------------

void ChangeFilenameExtension (char *dest, char *src, char *new_ext);

int nCurrentLevelSong;

void PlayLevelSong (int nLevel, int bFromHog)
{
	int	nSong;
	int	nTracks;
	int	bD1Song = (gameData.missions.list [gameData.missions.nCurrentMission].nDescentVersion == 1);
	char	szFilename [FILENAME_LEN];

Assert(nLevel != 0);
if (!gameData.songs.bInitialized)
	SongsInit();
SongsStopAll();
nCurrentLevelSong = nLevel;
nSong = (nLevel > 0) ? nLevel - 1 : -nLevel;
gameStates.sound.nCurrentSong = nSong;
if (!RBAEnabled() && gameStates.sound.bRedbookEnabled && gameOpts->sound.bUseRedbook)
	ReInitRedbook();
if (force_rb_register) {
	RBARegisterCD();			//get new track list for new CD
	force_rb_register = 0;
	}
if (bFromHog) {
	MakeLevelFilename (nLevel, szFilename, ".ogg");
	if (CFExtract (szFilename, gameFolders.szDataDir, 0, szFilename)) {
		char	szSong [FILENAME_LEN];
	
		sprintf (szSong, "%s%s%s", gameFolders.szTempDir, *gameFolders.szTempDir ? "/" : "", szFilename);
		if (DigiPlayMidiSong (szSong, NULL, NULL, 1, 0))
			return;
		}
	}
if ((nLevel > 0) && gameData.songs.user.nLevelSongs) {
	if (DigiPlayMidiSong (gameData.songs.user.pszLevelSongs [(nLevel - 1) % gameData.songs.user.nLevelSongs], NULL, NULL, 1, 0))
		return;
	}
if (gameStates.sound.bRedbookEnabled && RBAEnabled () && (nTracks = RBAGetNumberOfTracks()) > 1)	//try to play redbook
	PlayRedbookTrack (REDBOOK_FIRST_LEVEL_TRACK + (nSong % (nTracks-REDBOOK_FIRST_LEVEL_TRACK+1)),1);
if (!gameStates.sound.bRedbookPlaying) {			//not playing redbook, so play midi
	nSong = gameData.songs.nLevelSongs [bD1Song] ? gameData.songs.nFirstLevelSong [bD1Song] + (nSong % gameData.songs.nLevelSongs [bD1Song]) : 0;
	gameStates.sound.nCurrentSong = nSong;
		DigiPlayMidiSong (
			gameData.songs.info [nSong].filename, 
			gameData.songs.info [nSong].melodicBankFile, 
			gameData.songs.info [nSong].drumBankFile, 
			1, 
			bD1Song);
	}
}

//------------------------------------------------------------------------------
//this should be called regularly to check for redbook restart
void SongsCheckRedbookRepeat()
{
	static fix last_checkTime;
	fix currentTime;

	if (!gameStates.sound.bRedbookPlaying || gameConfig.nRedbookVolume==0) return;

	currentTime = TimerGetFixedSeconds();
	if (currentTime < last_checkTime || (currentTime - last_checkTime) >= F2_0) {
		if (!RBAPeekPlayStatus()) {
			StopTime();
			// if title ends, start credit music
			// if credits music ends, restart it
			if (gameStates.sound.bRedbookPlaying == REDBOOK_TITLE_TRACK || gameStates.sound.bRedbookPlaying == REDBOOK_CREDITS_TRACK)
				PlayRedbookTrack(REDBOOK_CREDITS_TRACK,0);
			else {
				//SongsGotoNextSong();

				//new code plays all tracks to end of disk, so if disk has
				//stopped we must be at end.  So start again with level 1 song.

				PlayLevelSong (1, 0);
			}
			StartTime();
		}
		last_checkTime = currentTime;
	}
}

//------------------------------------------------------------------------------
//goto the next level song
void SongsGotoNextSong()
{
if (gameStates.sound.bRedbookPlaying) 		//get correct track
	nCurrentLevelSong = RBAGetTrackNum() - REDBOOK_FIRST_LEVEL_TRACK + 1;
PlayLevelSong(nCurrentLevelSong + 1, 0);
}

//------------------------------------------------------------------------------
//goto the previous level song
void SongsGotoPrevSong()
{
if (gameStates.sound.bRedbookPlaying) 		//get correct track
	nCurrentLevelSong = RBAGetTrackNum() - REDBOOK_FIRST_LEVEL_TRACK + 1;
if (nCurrentLevelSong > 1)
	PlayLevelSong(nCurrentLevelSong - 1, 0);
}

//------------------------------------------------------------------------------

int LoadPlayList (char *pszPlayList)
{
	CFILE	cf;
	char	szSong [FILENAME_LEN], szListFolder [FILENAME_LEN], szSongFolder [FILENAME_LEN], *pszSong;
	int	l, bRead, nSongs, bMP3;

CFSplitPath (pszPlayList, szListFolder, NULL, NULL);
for (bRead = 0; bRead < 2; bRead++) {
	if (!CFOpen (&cf, pszPlayList, "", "rt", 0))
		return 0;
	nSongs = 0;
	while (CFGetS (szSong, sizeof (szSong), &cf)) {
		if ((bMP3 = (strstr (szSong, ".mp3") != NULL)) || strstr (szSong, ".ogg")) {
			if (bRead) {
				if (bMP3)
					gameData.songs.user.bMP3 = 1;
				if ((pszSong = strchr (szSong, '\r')))
					*pszSong = '\0';
				if ((pszSong = strchr (szSong, '\n')))
					*pszSong = '\0';
				l = (int) strlen (szSong) + 1;
				CFSplitPath (szSong, szSongFolder, NULL, NULL);
				if (!*szSongFolder)
					l += (int) strlen (szListFolder);
				if (!(pszSong = (char *) D2_ALLOC (l))) {
					CFClose (&cf);
					return nSongs = nSongs;
					}
				if (*szSongFolder)
					memcpy (pszSong, szSong, l);
				else
					sprintf (pszSong, "%s%s", szListFolder, szSong);
				gameData.songs.user.pszLevelSongs [nSongs] = pszSong;
				}
			nSongs++;
			}
		}
	CFClose (&cf);
	if (!bRead) {
		if (!(gameData.songs.user.pszLevelSongs = (char **) D2_ALLOC (nSongs * sizeof (char **))))
			return 0;
		}
	}
return gameData.songs.user.nLevelSongs = nSongs;
}

//------------------------------------------------------------------------------

void FreeUserSongs (void)
{
	int	i;

for (i = 0; i < gameData.songs.user.nLevelSongs; i++)
	D2_FREE (gameData.songs.user.pszLevelSongs [i]);
D2_FREE (gameData.songs.user.pszLevelSongs);
gameData.songs.user.nLevelSongs = 0;
}

//------------------------------------------------------------------------------
//eof
