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

/*
 *
 * Header for songs.c
 *
 * Old Log:
 * Revision 2.0  1995/02/27  11:30:52  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.6  1995/02/11  22:21:44  adam
 * *** empty log message ***
 *
 * Revision 1.5  1995/02/11  19:10:49  adam
 * *** empty log message ***
 *
 * Revision 1.4  1995/02/11  18:34:40  adam
 * *** empty log message ***
 *
 * Revision 1.3  1995/02/11  18:04:51  adam
 * upped songs
 *
 * Revision 1.2  1995/02/11  12:42:12  john
 * Added new song method, with FM bank switching..
 *
 * Revision 1.1  1995/02/11  10:20:18  john
 * Initial revision
 *
 *
 */

#ifndef _SONGS_H
#define _SONGS_H

#define MAX_NUM_SONGS           100

typedef struct song_info {
	char    filename[16];
	char    melodic_bank_file[16];
	char    drum_bank_file[16];
} song_info;

extern song_info Songs[MAX_NUM_SONGS];

#define SONG_D2_TITLE            0
#define SONG_D2_BRIEFING         1
#define SONG_D2_ENDLEVEL         2
#define SONG_D2_ENDGAME          3
#define SONG_D2_CREDITS          4
#define SONG_D2_FIRST_LEVEL_SONG 5
#define SONG_D2_INTER				16

#define SONG_D1_TITLE            9
#define SONG_D1_BRIEFING			10
#define SONG_D1_ENDLEVEL         11
#define SONG_D1_ENDGAME          12
#define SONG_D1_CREDITS          13
#define SONG_D1_FIRST_LEVEL_SONG 14
#define SONG_D1_INTER				15

#define SONG_TITLE            (gameStates.app.bD1Mission ? SONG_D1_TITLE : SONG_D2_TITLE)
#define SONG_BRIEFING         (gameStates.app.bD1Mission ? SONG_D1_BRIEFING : SONG_D2_BRIEFING)
#define SONG_ENDLEVEL         (gameStates.app.bD1Mission ? SONG_D1_ENDLEVEL : SONG_D2_ENDLEVEL)
#define SONG_ENDGAME          (gameStates.app.bD1Mission ? SONG_D1_ENDGAME : SONG_D2_ENDGAME)
#define SONG_CREDITS          (gameStates.app.bD1Mission ? SONG_D1_CREDITS : SONG_D2_CREDITS)
#define SONG_FIRST_LEVEL_SONG (gameStates.app.bD1Mission ? SONG_D1_FIRST_LEVEL_SONG : SONG_D2_FIRST_LEVEL_SONG)
#define SONG_INTER				(gameStates.app.bD1Mission ? SONG_D1_INTER : SONG_D2_INTER)

extern int Num_songs, nD1SongNum, nD2SongNum;   //how many MIDI songs

//whether or not redbook audio should be played
extern int Redbook_enabled;
extern int Redbook_playing;     // track that is currently playing

void songs_play_song( int songnum, int repeat );
void PlayLevelSong( int levelnum );

// stop the redbook, so we can read off the CD
void songs_stop_redbook(void);

// stop any songs - midi or redbook - that are currently playing
void SongsStopAll(void);

// this should be called regularly to check for redbook restart
void songs_check_redbook_repeat(void);

void songs_play_current_song(int repeat);

#endif /* _SONGS_H */
