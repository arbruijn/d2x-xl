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


#ifndef _LOADOBJECTS_H
#define _LOADOBJECTS_H

typedef struct {
	uint16_t  fileinfo_signature;
	uint16_t  fileinfoVersion;
	int32_t     fileinfo_sizeof;
} game_top_fileinfo;    // Should be same as first two fields below...

void LoadGame(void);
void SaveGame(void);
void getLevel_name(void);

int32_t LoadLevelData(char *filename, int32_t nLevel);
int32_t SaveLevel(char *filename);

// called in place of load_game() to only load the .min data
extern void load_mine_only(char * filename);

extern char Gamesave_current_filename [];

// In dumpmine.c
extern void write_game_text_file(char *filename);

extern tGameFileInfo	gameFileInfo;
extern game_top_fileinfo gameTopFileInfo;

extern int32_t Errors_in_mine;

void BuildObjTriggerRef (void);

#endif /* _LOADOBJECTS_H */
