/* $Id: songs.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#define MAX_NUM_SONGS           100

typedef struct tSongInfo {
	char    filename[16];
	char    melodicBankFile[16];
	char    drumBankFile[16];
} tSongInfo;

extern tSongInfo Songs[MAX_NUM_SONGS];

#define SONG_TITLE            0
#define SONG_BRIEFING         1
#define SONG_ENDLEVEL         2
#define SONG_ENDGAME          3
#define SONG_CREDITS          4
#define SONG_FIRST_LEVEL_SONG 5
#define SONG_INTER				2

extern int Num_songs, nD1SongNum, nD2SongNum;   //how many MIDI songs

//whether or not redbook audio should be played
void SongsPlaySong (int songnum, int repeat);
void PlayLevelSong (int nLevel, int bFromHog);
// stop the redbook, so we can read off the CD
void SongsStopRedbook(void);
// stop any songs - midi or redbook - that are currently playing
void SongsStopAll(void);
// this should be called regularly to check for redbook restart
void SongsCheckRedbookRepeat(void);
void SongsPlayCurrentSong(int repeat);
int LoadPlayList (char *pszPlayList);
void FreeUserSongs (void);

#endif /* _SONGS_H */
