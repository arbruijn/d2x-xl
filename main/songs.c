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

/*
 *
 * Routines to manage the songs in Descent.
 *
 * Old Log:
 * Revision 1.5  1995/11/03  12:52:59  allender
 * shareware changes
 *
 * Revision 1.4  1995/10/18  01:51:33  allender
 * fixed up stuff for redbook
 *
 * Revision 1.3  1995/10/17  13:13:44  allender
 * dont' add resource value to songs to play -- now done in digi
 * code
 *
 * Revision 1.2  1995/07/17  08:50:35  allender
 * make work with new music system
 *
 * Revision 1.1  1995/05/16  15:31:05  allender
 * Initial revision
 *
 * Revision 2.1  1995/05/02  16:15:21  john
 * Took out //printf.
 *
 * Revision 2.0  1995/02/27  11:27:13  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.2  1995/02/11  12:42:12  john
 * Added new song method, with FM bank switching..
 *
 * Revision 1.1  1995/02/11  10:20:33  john
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef _MSC_VER
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

extern void DigiStopCurrentSong();


//0 if redbook is no playing, else the track number

extern int CD_blast_mixer();

#define REDBOOK_VOLUME_SCALE  (255/3)		//255 is MAX

//takes volume in range 0..8
void set_redbook_volume(int volume)
{
	RBASetVolume(0);		// makes the macs sound really funny
	RBASetVolume(volume*REDBOOK_VOLUME_SCALE/8);
}

extern char CDROM_dir[];

void songs_init()
{
	int i, bD1Songs;
	char inputline[80+1];
	CFILE * fp;

if ( gameData.songs.bInitialized ) 
	return;
CFUseD1HogFile("descent.hog");
for (bD1Songs = 0; bD1Songs < 2; bD1Songs++) {
	#if !defined(WINDOWS)
		if (!FindArg("-nomixer"))
			CD_blast_mixer();   // Crank it!
	#endif
	if (CFExist("descent.sng", gameFolders.szDataDir, bD1Songs)) {   // mac (demo?) datafiles don't have the .sng file
		fp = CFOpen( "descent.sng", gameFolders.szDataDir, "rb", bD1Songs );
		if ( fp == NULL ) {
			if (bD1Songs)
				break;
			else
				Error( "Couldn't open descent.sng" );
			}
		i = gameData.songs.nSongs;
		while (CFGetS(inputline, 80, fp ))
		{
			char *p = strchr(inputline,'\n');
			if (p) *p = '\0';
			if ( strlen( inputline ) )
			{
				Assert( i < MAX_NUM_SONGS );
				sscanf( inputline, "%s %s %s",
						gameData.songs.info[i].filename,
						gameData.songs.info[i].melodic_bank_file,
						gameData.songs.info[i].drum_bank_file );
				if (!gameData.songs.nFirstLevelSong [bD1Songs] && strstr (gameData.songs.info [i].filename, "game01.hmp"))
					 gameData.songs.nFirstLevelSong [bD1Songs] = i;
				if (bD1Songs && strstr (gameData.songs.info [i].filename, "endlevel.hmp"))
					gameData.songs.nD1EndLevelSong = i;

				////printf( "%d. '%s' '%s' '%s'\n",i,gameData.songs.info[i].filename,gameData.songs.info[i].melodic_bank_file,gameData.songs.info[i].drum_bank_file );
				i++;
			}
		}
		if (bD1Songs) 
			gameData.songs.nD1Songs = i - gameData.songs.nSongs;
		else
			gameData.songs.nD2Songs = i - gameData.songs.nSongs;
		gameData.songs.nSongs = i;
		gameData.songs.nLevelSongs [bD1Songs] = gameData.songs.nSongs - gameData.songs.nFirstLevelSong [bD1Songs];
		if (!gameData.songs.nFirstLevelSong [bD1Songs])
			Error("gameData.songs.info are missing.");
		CFClose(fp);
	}
	gameData.songs.bInitialized = 1;
	//	RBA Hook
	#if !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )
		if (!gameOpts->sound.bUseRedbook)
			gameStates.sound.bRedbookEnabled = 0;
		else	// use redbook
		{
			#ifndef __MSDOS__
				RBAInit();
			#else
				RBAInit(toupper(CDROM_dir[0]) - 'A');
			#endif

				if (RBAEnabled())
			{
				set_redbook_volume(gameConfig.nRedbookVolume);
				RBARegisterCD();
			}
		}
		atexit(RBAStop);    // stop song on exit
	#endif	// endof ifndef SHAREWARE, ie ifdef SHAREWARE
	}
}

#define FADE_TIME (f1_0/2)

//stop the redbook, so we can read off the CD
void songs_stop_redbook(void)
{
	int old_volume = gameConfig.nRedbookVolume*REDBOOK_VOLUME_SCALE/8;
	fix old_time = TimerGetFixedSeconds();

	if (gameStates.sound.bRedbookPlaying) {		//fade out volume
		int new_volume;
		do {
			fix t = TimerGetFixedSeconds();

			new_volume = fixmuldiv(old_volume,(FADE_TIME - (t-old_time)),FADE_TIME);
			if (new_volume < 0)
				new_volume = 0;
			RBASetVolume(new_volume);
		} while (new_volume > 0);
	}
	RBAStop();              	// Stop CD, if playing
	RBASetVolume(old_volume);	//restore volume
	gameStates.sound.bRedbookPlaying = 0;		
}

//stop any songs - midi or redbook - that are currently playing
void SongsStopAll(void)
{
	DigiStopCurrentSong();	// Stop midi song, if playing
	songs_stop_redbook();			// Stop CD, if playing
}

int force_rb_register=0;

void reinit_redbook()
{
	#ifndef __MSDOS__
		RBAInit();
	#else
		RBAInit(toupper(CDROM_dir[0]) - 'A');
	#endif

	if (RBAEnabled())
	{
		set_redbook_volume(gameConfig.nRedbookVolume);
		RBARegisterCD();
		force_rb_register=0;
	}
}


//returns 1 if track started sucessfully
//start at tracknum.  if keep_playing set, play to end of disc.  else
//play only specified track
int play_redbook_track(int tracknum,int keep_playing)
{
	gameStates.sound.bRedbookPlaying = 0;
#ifdef _DEBUG
	tracknum = 1;
#endif

	if (!RBAEnabled() && gameStates.sound.bRedbookEnabled && !gameOpts->sound.bUseRedbook)
		reinit_redbook();

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
#define REDBOOK_FIRST_LEVEL_TRACK   (songs_haved2_cd()?4:1)

// songs_haved2_cd returns 1 if the descent 2 CD is in the drive and
// 0 otherwise

#if 1
int songs_haved2_cd()
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
int songs_haved2_cd()
{
	char temp[128],cwd[128];
	
	getcwd(cwd, 128);

	strcpy(temp,CDROM_dir);

	if (temp[strlen(temp)-1] == '\\')
		temp[strlen(temp)-1] = 0;

	if ( !chdir(temp) ) {
		chdir(cwd);
		return 1;
	}

	return 0;
}
#endif


void songs_play_song( int songnum, int repeat )
{
	#ifndef SHAREWARE
	//Assert(songnum != SONG_ENDLEVEL && songnum != SONG_ENDGAME);	//not in full version
	#endif

	if ( !gameData.songs.bInitialized )
		songs_init();

	//stop any music already playing
	if (!(gameStates.sound.bRedbookEnabled ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume))
		return;
	SongsStopAll();

	//do we want any of these to be redbook songs?

	if (force_rb_register) {
		RBARegisterCD();			//get new track list for new CD
		force_rb_register = 0;
	}

	gameStates.sound.nCurrentSong = songnum;

	if (songnum == SONG_TITLE)
		play_redbook_track(REDBOOK_TITLE_TRACK,0);
	else if (songnum == SONG_CREDITS)
		play_redbook_track(REDBOOK_CREDITS_TRACK,0);

	if (!gameStates.sound.bRedbookPlaying) {		//not playing redbook, so play midi
		DigiPlayMidiSong(
			gameData.songs.info[songnum].filename, gameData.songs.info[songnum].melodic_bank_file, 
			gameData.songs.info[songnum].drum_bank_file, repeat, gameData.songs.nD1Songs && (songnum >= gameData.songs.nD2Songs) );
	}
}

void songs_play_current_song( int repeat )
{
songs_play_song (gameStates.sound.nCurrentSong, repeat);
}

int nCurrentLevelSong;

void PlayLevelSong( int levelnum )
{
	int songnum;
	int n_tracks;
	int bD1Song = (gameData.missions.list[gameData.missions.nCurrentMission].descent_version == 1);

	Assert( levelnum != 0 );

	if ( !gameData.songs.bInitialized )
		songs_init();
	SongsStopAll();
	nCurrentLevelSong = levelnum;
	songnum = (levelnum > 0) ? levelnum - 1 : -levelnum;
	gameStates.sound.nCurrentSong = songnum;
	if (!RBAEnabled() && gameStates.sound.bRedbookEnabled && gameOpts->sound.bUseRedbook)
		reinit_redbook();
	if (force_rb_register) {
		RBARegisterCD();			//get new track list for new CD
		force_rb_register = 0;
	}
	if (gameStates.sound.bRedbookEnabled && RBAEnabled() && (n_tracks = RBAGetNumberOfTracks()) > 1) {
		//try to play redbook
		play_redbook_track(REDBOOK_FIRST_LEVEL_TRACK + (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK+1)),1);
	}
	if (! gameStates.sound.bRedbookPlaying) {			//not playing redbook, so play midi
		songnum = gameData.songs.nLevelSongs [bD1Song] ? gameData.songs.nFirstLevelSong [bD1Song] + (songnum % gameData.songs.nLevelSongs [bD1Song]) : 0;
		gameStates.sound.nCurrentSong = songnum;
			DigiPlayMidiSong( 
				gameData.songs.info [songnum].filename, 
				gameData.songs.info [songnum].melodic_bank_file, 
				gameData.songs.info [songnum].drum_bank_file, 
				1, 
				bD1Song);
	}
}

//this should be called regularly to check for redbook restart
void songs_check_redbook_repeat()
{
	static fix last_check_time;
	fix current_time;

	if (!gameStates.sound.bRedbookPlaying || gameConfig.nRedbookVolume==0) return;

	current_time = TimerGetFixedSeconds();
	if (current_time < last_check_time || (current_time - last_check_time) >= F2_0) {
		if (!RBAPeekPlayStatus()) {
			StopTime();
			// if title ends, start credit music
			// if credits music ends, restart it
			if (gameStates.sound.bRedbookPlaying == REDBOOK_TITLE_TRACK || gameStates.sound.bRedbookPlaying == REDBOOK_CREDITS_TRACK)
				play_redbook_track(REDBOOK_CREDITS_TRACK,0);
			else {
				//songs_goto_next_song();
	
				//new code plays all tracks to end of disk, so if disk has
				//stopped we must be at end.  So start again with level 1 song.
	
				PlayLevelSong(1);
			}
			StartTime();
		}
		last_check_time = current_time;
	}
}

//goto the next level song
void songs_goto_next_song()
{
	if (gameStates.sound.bRedbookPlaying) 		//get correct track
		nCurrentLevelSong = RBAGetTrackNum() - REDBOOK_FIRST_LEVEL_TRACK + 1;

	PlayLevelSong(nCurrentLevelSong+1);

}

//goto the previous level song
void songs_goto_prev_song()
{
	if (gameStates.sound.bRedbookPlaying) 		//get correct track
		nCurrentLevelSong = RBAGetTrackNum() - REDBOOK_FIRST_LEVEL_TRACK + 1;

	if (nCurrentLevelSong > 1)
		PlayLevelSong(nCurrentLevelSong-1);

}

