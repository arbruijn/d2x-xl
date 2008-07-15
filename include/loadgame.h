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

#ifndef _LOADGAME_H
#define _LOADGAME_H

#include "player.h"
#include "mission.h"

#define SUPERMSL       0
#define SUPER_SEEKER        1
#define SUPER_SMARTBOMB     2
#define SUPER_SHOCKWAVE     3

extern int LastLevel, Last_secretLevel, Last_mission;   //set by mission code


// CurrentLevel_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 means not a real level loaded
#if 0
extern int CurrentLevel_num, NextLevel_num;
extern char CurrentLevel_name[LEVEL_NAME_LEN];
extern tObjPosition Player_init[MAX_PLAYERS];
extern int bPlayerIsTyping [MAX_PLAYERS];
extern int nTypingTimeout;
#endif
// This is the highest level the tPlayer has ever reached
extern int Player_highestLevel;

//
// New game sequencing functions
//

// starts a new game on the given level
int StartNewGame(int startLevel);

// starts the next level
int StartNewLevel(int level_num, int secretFlag);
void StartLevel(int randomFlag);

// Actually does the work to start new level
int StartNewLevelSub(int nLevel, int bPageInTextures, int bSecret, int bRestore);

void InitMultiPlayerObject (void);            //make sure tPlayer's tObject set up
void InitPlayerStatsGame (void);      //clear all stats

// starts a resumed game loaded from disk
void ResumeSavedGame(int startLevel);

// called when the tPlayer has finished a level
// if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel(int secretFlag);

// called when the tPlayer has died
void DoPlayerDead (void);

// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
int LoadLevel (int nLevel, int bPageInTextures, int bRestore);

void GameStartRemoveUnusedPlayers ();

extern void ShowHelp();
extern void UpdatePlayerStats();

// from scores.c

void show_high_scores(int place);
void draw_high_scores(int place);
int add_player_to_high_scores(tPlayer *playerP);
void input_name (int place);
int reset_high_scores();
void InitPlayerStatsLevel(int secretFlag);

void open_message_window(void);
void close_message_window(void);

// goto whatever secrect level is appropriate given the current level
void goto_secretLevel();

// reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_onLevel();

// Show endlevel bonus scores
void DoEndLevelScoreGlitz(int network);

// stuff for multiplayer
extern int MaxNumNetPlayers;
extern int NumNetPlayerPositions;

void BashToShield(int, const char *);
void BashToEnergy(int, const char *);

fix RobotDefaultShields (tObject *objP);

char *LevelName (int nLevel);
char *LevelSongName (int nLevel);
char *MakeLevelFilename (int nLevel, char *pszFilename, const char *pszFileExt);

int GetRandomPlayerPosition (void);

#endif /* _LOADGAME_H */
