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

#ifndef _JOY_H
#define _JOY_H

#include "pstypes.h"
#include "fix.h"
#ifdef __macosx__
#	include "SDL/SDL_joystick.h"
#else
#	include "SDL_joystick.h"
#endif

#define JOY_1_BUTTON_A  1
#define JOY_1_BUTTON_B  2
#define JOY_2_BUTTON_A  4
#define JOY_2_BUTTON_B  8
#define JOY_ALL_BUTTONS (1+2+4+8+16)

#define JOY_1_X_AXIS        1
#define JOY_1_Y_AXIS        2
#define JOY_1_Z_AXIS        4
#define JOY_1_R_AXIS        16
#define JOY_1_U_AXIS        32
#define JOY_1_V_AXIS        64

#define MAX_JOYSTICKS	3	//16
#define JOY_MAP_SIZE		16	//16

#define MAX_AXES_PER_JOYSTICK 8
#define MAX_BUTTONS_PER_JOYSTICK 32
#define MAX_HATS_PER_JOYSTICK 4

#define JOY_MAX_AXES (MAX_AXES_PER_JOYSTICK * MAX_JOYSTICKS)
#define JOY_MAX_BUTTONS (MAX_BUTTONS_PER_JOYSTICK * MAX_JOYSTICKS)


#define JOY_2_X_AXIS        4
#define JOY_2_Y_AXIS        8
#define JOY_ALL_AXIS        (1+2+4+8)

#define JOY_SLOW_READINGS       1
#define JOY_POLLED_READINGS     2
#define JOY_BIOS_READINGS       4
/* not present in d2src, maybe we don't really need it? */
#define JOY_FRIENDLY_READINGS   8

#ifdef USE_LINUX_JOY
#define MAX_BUTTONS 64
#else
#define MAX_BUTTONS 20
#endif

typedef struct tJoyButton {
	int32_t state;
	int32_t lastState;
	fix xTimeWentDown;
	int32_t numDowns;
	int32_t numUps;
} __pack__ tJoyButton;

typedef struct tJoyAxisCal {
	int32_t		nMin;
	int32_t		nCenter;
	int32_t		nMax;
} __pack__ tJoyAxisCal;

typedef struct tJoyAxis {
	int32_t			nValue;
	tJoyAxisCal	cal;
} __pack__ tJoyAxis;

typedef struct tSdlJoystick {
	SDL_Joystick	*handle;
	int32_t				nAxes;
	int32_t				nButtons;
	int32_t				nHats;
	int32_t				hatMap [MAX_HATS_PER_JOYSTICK];  //Note: Descent expects hats to be buttons, so these are indices into Joystick.buttons
	int32_t				axisMap [MAX_AXES_PER_JOYSTICK];
	int32_t				buttonMap [MAX_BUTTONS_PER_JOYSTICK];
} __pack__ tSdlJoystick;

extern struct tSdlJoystick /*SDL_Joystick*/ sdlJoysticks [MAX_JOYSTICKS];

//==========================================================================
// This initializes the joy and does a "quick" calibration which
// assumes the stick is centered and sets the minimum value to 0 and
// the maximum value to 2 times the centered reading. Returns 0 if no
// joystick was detected, 1 if everything is ok.
// JoyInit (void) is called.

int32_t JoyInit (void);
void JoyClose (void);

extern char bJoyInstalled;
extern char bJoyPresent;

//==========================================================================
// The following 3 routines can be used to zero in on better joy
// calibration factors. To use them, ask the user to hold the stick
// in either the upper left, lower right, or center and then have them
// press a key or button and then call the appropriate one of these
// routines, and it will read the stick and update the calibration factors.
// Usually, assuming that the stick was centered when JoyInit was
// called, you really only need to call joy_set_lr, since the upper
// left position is usually always 0,0 on most joys.  But, the safest
// bet is to do all three, or let the user choose which ones to set.

void joy_set_ul (void);
void joy_set_lr (void);
void joy_set_cen (void);


//==========================================================================
// This reads the joystick. X and Y will be between -128 and 127.
// Takes about 1 millisecond in the worst case when the stick
// is in the lower right hand corner. Always returns 0,0 if no stick
// is present.

void JoyGetPos (int32_t *x, int32_t *y);

//==========================================================================
// This just reads the buttons and returns their status.  When bit 0
// is 1, button 1 is pressed, when bit 1 is 1, button 2 is pressed.
int32_t JoyGetBtns (void);

//==========================================================================
// This returns the number of times a button went either down or up since
// the last call to this function.
int32_t JoyGetButtonUpCnt (int32_t btn);
int32_t JoyGetButtonDownCnt (int32_t btn);

//==========================================================================
// This returns how long (in approximate milliseconds) that each of the
// buttons has been held down since the last call to this function.
// It is the total time... say you pressed it down for 3 ticks, released
// it, and held it down for 6 more ticks. The time returned would be 9.
fix JoyGetButtonDownTime(int32_t btn);

uint32_t JoyReadRawButtons (void);
uint32_t JoyReadRawAxis (uint32_t mask, int32_t *axis);
void JoyFlush (void);
uint8_t JoyGetPresentMask (void);
void JoySetTimerRate(int32_t maxValue);
int32_t JoyGetTimerRate (void);

int32_t JoyGetButtonState (int32_t btn);
void JoySetCenFake (int32_t channel);
uint8_t JoyReadStick (uint8_t masks, int32_t *axis);
void JoyGetCalVals (tJoyAxisCal *cal, int32_t nAxes);
void JoySetCalVals (tJoyAxisCal *cal, int32_t nAxes);
void JoySetBtnValues (int32_t btn, int32_t state, fix timedown, int32_t downCount, int32_t upCount);
int32_t JoyGetScaledReading (int32_t raw, int32_t axn);
void JoySetSlowReading (int32_t flag);
int32_t JoySetDeadzone (int32_t nRelZone, int32_t nAxis);

extern int32_t joyDeadzone [4];
extern int32_t joyDeadzoneRel [4];

#endif // _JOY_H
