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
 * Headers for gamesave.c
 *
 *
 */


#ifndef _GAMESAVE_H
#define _GAMESAVE_H

typedef struct {
	ushort  fileinfo_signature;
	ushort  fileinfoVersion;
	int     fileinfo_sizeof;
} game_top_fileinfo;    // Should be same as first two fields below...

typedef struct tGameItemInfo {
	int		offset;
	int		count;
	int		size;
} tGameItemInfo;

typedef struct {
	ushort  fileinfo_signature;
	ushort  fileinfoVersion;
	int     fileinfo_sizeof;
	char    mine_filename [15];
	int     level;
	tGameItemInfo	player;
	tGameItemInfo	objects;
	tGameItemInfo	walls;
	tGameItemInfo	doors;
	tGameItemInfo	triggers;
	tGameItemInfo	links;
	tGameItemInfo	control;
	tGameItemInfo	botGen;
	tGameItemInfo	lightDeltaIndices;
	tGameItemInfo	lightDeltas;
	tGameItemInfo	equipGen;
} tGameFileInfo;

void LoadGame(void);
void SaveGame(void);
void getLevel_name(void);

int LoadLevelSub(char *filename, int nLevel);
int SaveLevel(char *filename);

// called in place of load_game() to only load the .min data
extern void load_mine_only(char * filename);

extern char Gamesave_current_filename [];
extern int nGameSaveOrgRobots;

// In dumpmine.c
extern void write_game_text_file(char *filename);

extern tGameFileInfo	gameFileInfo;
extern game_top_fileinfo gameTopFileInfo;

extern int Errors_in_mine;

#endif /* _GAMESAVE_H */
