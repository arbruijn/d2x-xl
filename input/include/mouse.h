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

#ifndef MOUSE_H
#define MOUSE_H

#include "pstypes.h"
#include "fix.h"

#define D2_MB_LEFT				0
#define D2_MB_RIGHT				1
#define D2_MB_MIDDLE				2
#define D2_MB_Z_UP				3
#define D2_MB_Z_DOWN				4
#define D2_MB_PITCH_BACKWARD	5
#define D2_MB_PITCH_FORWARD	6
#define D2_MB_BANK_LEFT			7
#define D2_MB_BANK_RIGHT		8
#define D2_MB_HEAD_LEFT			9
#define D2_MB_HEAD_RIGHT		10

#define MOUSE_LBTN 1
#define MOUSE_RBTN 2
#define MOUSE_MBTN 4

//========================================================================
// Check for mouse driver, reset driver if installed. returns number of
// buttons if driver is present.

void MouseInit (void);
void MouseFlush (void);	// clears all mice events...

//========================================================================
// Shutdowns mouse system.
void MouseGetPos (int *x, int *y);
void MouseGetDelta (int *dx, int *dy);
#ifdef SDL_INPUT
void MouseGetPosZ (int *x, int *y, int *z);
void MouseGetDeltaZ (int *dx, int *dy, int *dz);
#endif
int MouseGetButtons (void);
void MouseGetCybermanPos (int *x, int *y);

// Returns how long this button has been down since last call.
extern fix MouseButtonDownTime(int button);

// Returns how many times this button has went down since last call.
extern int MouseButtonDownCount(int button);

// Returns 1 if this button is currently down
extern int MouseButtonState(int button);

#define MOUSE_MAX_BUTTONS       8

typedef struct tMouseButton {
	ubyte pressed;
	ubyte rotated;
	fix	xPrevTimeWentDown;
	fix	xTimeWentDown;
	fix	xTimeHeldDown;
	uint	numDowns;
	uint	numUps;
} tMouseButton;

typedef struct tMouseInfo {
	tMouseButton	buttons [MOUSE_MAX_BUTTONS];
	int				dx, dy, dz;
	int				x, y, z;
	int				bDoubleClick;
} tMouseInfo;

extern tMouseInfo mouseData;

#endif
	