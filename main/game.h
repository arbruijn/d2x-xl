/* $Id: game.h,v 1.6 2003/12/08 22:32:56 btb Exp $ */
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

#ifndef _GAME_H
#define _GAME_H

#include <setjmp.h>

#include "vecmat.h"
#include "object.h"

//#include "segment.h"

// from mglobal.c
#define CV_NONE             0
#define CV_ESCORT           1
#define CV_REAR             2
#define CV_COOP             3
#define CV_MARKER   			 4
#define CV_RADAR_TOPDOWN    5
#define CV_RADAR_HEADSUP    6

// constants for ft_preference
#define FP_RIGHT        0
#define FP_UP           1
#define FP_FORWARD      2       // this is the default
#define FP_LEFT         3
#define FP_DOWN         4
#define FP_FIRST_TIME   5

extern int ft_preference;

// The following bits define the game modes.
#define GM_EDITOR       1       // You came into the game from the editor
#define GM_SERIAL       2       // You are in serial mode
#define GM_NETWORK      4       // You are in network mode
#define GM_MULTI_ROBOTS 8       // You are in a multiplayer mode with robots.
#define GM_MULTI_COOP   16      // You are in a multiplayer mode and can't hurt other players.
#define GM_MODEM        32      // You are in a modem (serial) game

#define GM_UNKNOWN      64      // You are not in any mode, kind of dangerous...
#define GM_GAME_OVER    128     // Game has been finished

#define GM_TEAM         256     // Team mode for network play
#define GM_CAPTURE      512     // Capture the flag mode for D2
#define GM_HOARD        1024    // New hoard mode for D2 Christmas
#define GM_ENTROPY      2048	  // Similar to Descent 3's entropy game mode
#define GM_MONSTERBALL	4096	  // Similar to Descent 3's monsterball game mode

#define GM_NORMAL       0       // You are in normal play mode, no multiplayer stuff
#define GM_MULTI        38      // You are in some nType of multiplayer game

#define IsMultiGame	((gameData.app.nGameMode & GM_MULTI) != 0)
#define IsTeamGame	((gameData.app.nGameMode & GM_TEAM) != 0)
#define IsCoopGame	((gameData.app.nGameMode & GM_MULTI_COOP) != 0)
#define IsRobotGame	(!IsMultiGame || (gameData.app.nGameMode & (GM_MULTI_ROBOTS | GM_MULTI_COOP)))
// Examples:
// Deathmatch mode on a network is GM_NETWORK
// Deathmatch mode via modem with robots is GM_MODEM | GM_MULTI_ROBOTS
// Cooperative mode via serial link is GM_SERIAL | GM_MULTI_COOP

#define NDL 5       // Number of difficulty levels.
#define NUM_DETAIL_LEVELS   6

extern int gauge_message_on;

#ifndef NDEBUG      // if debugging, these are variables

extern int Slew_on;                 // in slew or sim mode?
extern int bGameDoubleBuffer;      // double buffering?


#else               // if not debugging, these are constants

#define Slew_on             0       // no slewing in real game
#define Game_double_buffer  1       // always double buffer in real game

#endif

#define bScanlineDouble     0       // PC doesn't do scanline doubling

// Suspend flags

#define SUSP_NONE       0           // Everything moving normally
#define SUSP_ROBOTS     1           // Robot AI doesn't move
#define SUSP_WEAPONS    2           // Lasers, etc. don't move

extern int Game_suspended;          // if non-zero, nothing moves but tPlayer

// from game.c
void InitGame(void);
void game(void);
void _CDECL_ CloseGame(void);
void InitCockpit(void);
void CalcFrameTime(void);

int do_flythrough(tObject *obj,int firstTime);

extern jmp_buf gameExitPoint;       // Do a long jump to this when game is over.
extern int DifficultyLevel;    // Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
extern int nDifficultyLevel;
extern int DetailLevel;        // Detail level in 0..NUM_DETAIL_LEVELS-1, 0 = boringest, NUM_DETAIL_LEVELS = coolest
extern int Global_laser_firingCount;
extern int Global_missile_firingCount;
extern int Render_depth;
extern fix Auto_fire_fusion_cannonTime, Fusion_charge;

#define MAX_PALETTE_ADD 30

extern void PALETTE_FLASH_ADD(int dr, int dg, int db);

//sets the rgb values for palette flash
#define	PALETTE_FLASH_SET(_r,_g,_b) \
			gameStates.ogl.palAdd.red=(_r), \
			gameStates.ogl.palAdd.green=(_g), \
			gameStates.ogl.palAdd.blue=(_b)

extern int draw_gauges_on;

extern void init_game_screen(void);

extern void GameFlushInputs();    // clear all inputs

extern int Playing_game;    // True if playing game
extern int Auto_flythrough; // if set, start flythough automatically
extern int MarkCount;      // number of debugging marks set
extern char faded_in;

void StopTime(void);
void StartTime(void);
void ResetTime(void);       // called when starting level
int TimeStopped (void);

// If automapFlag == 1, then call automap routine to write message.

extern grs_canvas * GetCurrentGameScreen();

//valid modes for cockpit
#define CM_FULL_COCKPIT     0   // normal screen with cockput
#define CM_REAR_VIEW        1   // looking back with bitmap
#define CM_STATUS_BAR       2   // small status bar, w/ reticle
#define CM_FULL_SCREEN      3   // full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX        4   // half-height window (for cutscenes)

extern int Cockpit_mode;        // what sort of cockpit or window is up?
extern int Game_window_w,       // width and height of tPlayer's game window
           Game_window_h;

extern int RearView;           // if true, looking back.

// initalize flying
void FlyInit(tObject *obj);

// selects a given cockpit (or lack of one).
void SelectCockpit(int mode);

// force cockpit redraw next time. call this if you've trashed the screen
void ResetCockpit(void);       // called if you've trashed the screen

// functions to save, clear, and resture palette flash effects
void PaletteSave(void);
void ResetPaletteAdd(void);
void PaletteRestore(void);

// put up the help message
void DoShowHelp();

// show a message in a nice little box
void ShowBoxedMessage(char *msg);

// erases message drawn with ShowBoxedMessage()
void ClearBoxedMessage();

// turns off rear view & rear view cockpit
void ResetRearView(void);

extern int Game_turbo_mode;

// returns ptr to escort robot, or NULL
tObject *find_escort();

extern void ApplyModifiedPalette(void);

int GrToggleFullScreenGame(void);

void ShowInGameWarning (char *s);

void GetSlowTicks (void);
/*
 * reads a tFlickeringLight structure from a CFILE
 */
extern short maxfps;
extern int timer_paused;

#endif /* _GAME_H */
