/* $Id: playsave.h,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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

#ifndef _PLAYSAVE_H
#define _PLAYSAVE_H

#define COMPATIBLE_PLAYER_FILE_VERSION    17
#define D2W95_PLAYER_FILE_VERSION			24
#define D2XW32_PLAYER_FILE_VERSION			45		// first flawless D2XW32 tPlayer file version
#define D2XXL_PLAYER_FILE_VERSION			160
#define PLAYER_FILE_VERSION					160	//increment this every time the tPlayer file changes

#define N_SAVE_SLOTS    10
#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

#ifndef EZERO
#	define EZERO 0
#endif

extern int DefaultLeveling_on;

// update the tPlayer's highest level.  returns errno (0 == no error)
int update_player_file ();

// Used to save KConfig values to disk.
int WritePlayerFile ();

int NewPlayerConfig ();

int ReadPlayerFile (int bOnlyWindowSizes);

// set a new highest level for tPlayer for this mission
void SetHighestLevel (ubyte nLevel);

// gets the tPlayer's highest level from the file for this mission
int GetHighestLevel(void);

void FreeParams (void);

#endif /* _PLAYSAVE_H */
