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

#define MAX_FRAMERATE		120

#define PHYSICS_FPS			-1

// from mglobal.c
#define CV_NONE            0
#define CV_ESCORT          1
#define CV_REAR            2
#define CV_COOP            3
#define CV_MARKER   			4
#define CV_RADAR_TOPDOWN   5
#define CV_RADAR_HEADSUP   6
#define CV_FUNC_COUNT		7

// constants for ft_preference
#define FP_RIGHT        0
#define FP_UP           1
#define FP_FORWARD      2       // this is the default
#define FP_LEFT         3
#define FP_DOWN         4
#define FP_FIRST_TIME   5

extern int32_t ft_preference;

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
#define GM_MULTI        (GM_SERIAL | GM_NETWORK | GM_MODEM)      // You are in some nType of multiplayer game

#define IsNetworkGame	(gameData.app.GameMode (GM_NETWORK) != 0)
#define IsMultiGame		(gameData.app.GameMode (GM_MULTI) != 0)
#define IsTeamGame		(gameData.app.GameMode (GM_TEAM) != 0)
#define IsCoopGame		(gameData.app.GameMode (GM_MULTI_COOP) != 0)
#define IsRobotGame		(!IsMultiGame || (gameData.app.GameMode (GM_MULTI_ROBOTS | GM_MULTI_COOP) != 0))
#define IsHoardGame		(gameData.app.GameMode (GM_HOARD) != 0)
#define IsEntropyGame	(gameData.app.GameMode (GM_ENTROPY) != 0)
// Examples:
// Deathmatch mode on a network is GM_NETWORK
// Deathmatch mode via modem with robots is GM_MODEM | GM_MULTI_ROBOTS
// Cooperative mode via serial link is GM_SERIAL | GM_MULTI_COOP

#define NDL 5       // Number of difficulty levels.

//------------------------------------------------------------------------------

#define NUM_DETAIL_LEVELS   6

typedef struct tDetailData {
	uint8_t		renderDepths [NUM_DETAIL_LEVELS - 1];
	int8_t		maxPerspectiveDepths [NUM_DETAIL_LEVELS - 1];
	int8_t		maxLinearDepths [NUM_DETAIL_LEVELS - 1];
	int8_t		maxLinearDepthObjects [NUM_DETAIL_LEVELS - 1];
	int8_t		maxDebrisObjects [NUM_DETAIL_LEVELS - 1];
	int8_t		maxObjsOnScreenDetailed [NUM_DETAIL_LEVELS - 1];
	int8_t		simpleModelThresholdScales [NUM_DETAIL_LEVELS - 1];
	uint8_t		nSoundChannels [NUM_DETAIL_LEVELS - 1];
} __pack__ tDetailData;

extern tDetailData detailData;

//------------------------------------------------------------------------------

extern int32_t gauge_message_on;

#if DBG      // if debugging, these are variables

extern int32_t Slew_on;                 // in slew or sim mode?
extern int32_t bGameDoubleBuffer;      // double buffering?


#else               // if not debugging, these are constants

#define Slew_on             0       // no slewing in real game
#define Game_double_buffer  1       // always double buffer in real game

#endif

#define bScanlineDouble     0       // PC doesn't do scanline doubling

// Suspend flags

#define SUSP_NONE       0           // Everything moving normally
#define SUSP_ROBOTS     1           // Robot AI doesn't move
#define SUSP_WEAPONS    2           // Lasers, etc. don't move
#define SUSP_POWERUPS   4           // powerups aren't rendered and cannot be picked up
#define SUSP_TEMPORARY	128

extern int32_t Game_suspended;          // if non-zero, nothing moves but CPlayerData

// from game.c
bool InitGame(int32_t nSegments, int32_t nVertices);
void RunGame (void);
void CleanupAfterGame (bool bHaveLevel = true);
void _CDECL_ CloseGame (void);
void CalcFrameTime (int32_t fps = 0);

int32_t do_flythrough(CObject *obj,int32_t firstTime);

extern jmp_buf gameExitPoint;       // Do a long jump to this when game is over.
extern int32_t DifficultyLevel;    // Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
extern int32_t nDifficultyLevel;
extern int32_t DetailLevel;        // Detail level in 0..NUM_DETAIL_LEVELS-1, 0 = boringest, NUM_DETAIL_LEVELS = coolest
extern int32_t Global_laser_firingCount;
extern int32_t Global_missile_firingCount;
extern int32_t Render_depth;
extern fix Auto_fire_fusion_cannonTime, Fusion_charge;

extern int32_t draw_gauges_on;

extern void init_game_screen(void);

extern void GameFlushInputs();    // clear all inputs

extern int32_t Playing_game;    // True if playing game
extern int32_t Auto_flythrough; // if set, start flythough automatically
extern int32_t MarkCount;      // number of debugging marks set
extern char faded_in;

void StopTime (void);
void StartTime (int32_t bReset);
void ResetTime (void);       // called when starting level
int32_t TimeStopped (void);

// If automapFlag == 1, then call automap routine to write message.

CCanvas* CurrentGameScreen();

// initalize flying
void FlyInit(CObject *obj);

// put up the help message
void ShowHelp();

// show a message in a nice little box
// turns off rear view & rear view cockpit
void ResetRearView(void);

extern int32_t Game_turbo_mode;

// returns ptr to escort robot, or NULL
CObject *FindEscort();

extern void ApplyModifiedPalette(void);

int32_t GrToggleFullScreenGame(void);

void ShowInGameWarning (const char *s);

int32_t MarkPathToExit ();

void DoEffectsFrame (void);
void UpdateEffects (void);

void GameDisableCheats ();
void TurnCheatsOff ();

void GetSlowTicks (void);

void SetFunctionMode (int32_t newFuncMode);
int32_t DoGameFrame (int32_t bRenderFrame, int32_t bReadControls, int32_t bFrameTime = 1);
int32_t GameFrame (int32_t bRenderFrame, int32_t bReadControls, int32_t fps = 0);

void FullPaletteSave (void);

/*
 * reads a CVariableLight structure from a CFILE
 */
extern int16_t maxfps;
extern int32_t timer_paused;
extern uint32_t nCurrentVGAMode;


#endif /* _GAME_H */
