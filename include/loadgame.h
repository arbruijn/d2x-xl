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

// CurrentLevel_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 means not a real level loaded
#if 0
extern int32_t CurrentLevel_num, NextLevel_num;
extern char CurrentLevel_name [LEVEL_NAME_LEN];
extern tObjPosition Player_init[MAX_PLAYERS];
extern int32_t bPlayerIsTyping [MAX_PLAYERS];
extern int32_t nTypingTimeout;
#endif
// This is the highest level the player has ever reached
//
// New game sequencing functions
//

// starts a new game on the given level
int32_t StartNewGame (int32_t nStartLevel);

// starts the next level
int32_t StartNewLevel (int32_t nLevel, bool bNewGame);

// Actually does the work to start new level
void ResetPlayerData (bool bNewGame, bool bSecret, bool bRestore, int32_t nPlayer = -1);      //clear all stats

int32_t PrepareLevel (int32_t nLevel, bool bLoadTextures, bool bSecret, bool bRestore, bool bNewGame);
void StartLevel (int32_t nLevel, int32_t bRandom);
int32_t LoadLevel (int32_t nLevel, bool bLoadTextures, bool bRestore);

void GameStartInitNetworkPlayers (void);

// starts a resumed game loaded from disk
void ResumeSavedGame (int32_t startLevel);

// called when the player has finished a level
// if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel (int32_t secretFlag);

// called when the player has died
void DoPlayerDead (void);

void SetPosFromReturnSegment (int32_t bRelink);
// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
void UnloadLevelData (int32_t bRestore = 0, bool bQuit = true);
void AddPlayerLoadout (bool bRestore = false);
void ResetShipData (bool bRestore = false, int32_t nPlayer = -1);

void GameStartRemoveUnusedPlayers (void);

int32_t CountRobotsInLevel (void);

void ShowHelp (void);
void UpdatePlayerStats (void);

// from scores.c

void show_high_scores(int32_t place);
void draw_high_scores(int32_t place);
int32_t add_player_to_high_scores(CPlayerInfo *playerP);
void input_name (int32_t place);
int32_t reset_high_scores();

void open_message_window(void);
void close_message_window(void);

// goto whatever secrect level is appropriate given the current level
void goto_secretLevel();

// reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_onLevel();

// Show endlevel bonus scores
void DoEndLevelScoreGlitz(int32_t network);

// stuff for multiplayer
extern int32_t MaxNumNetPlayers;
extern int32_t NumNetPlayerPositions;

void BashToShield(int32_t, const char *);
void BashToEnergy(int32_t, const char *);

fix RobotDefaultShield (CObject *objP);

char *LevelName (int32_t nLevel);
char *LevelSongName (int32_t nLevel);
char *MakeLevelFilename (int32_t nLevel, char *pszFilename, const char *pszFileExt);

int32_t GetRandomPlayerPosition (int32_t nPlayer);

#endif /* _LOADGAME_H */
