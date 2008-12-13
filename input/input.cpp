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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE EVE.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include "error.h"
#include "inferno.h"
#include "key.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "event.h"
#include "joy.h"
#include "mouse.h"
#include "kconfig.h"
#include "gauges.h"
#include "render.h"
#include "endlevel.h"
#include "timer.h"
#include "text.h"
#include "automap.h"
#include "input.h"
#include "gamecntl.h"
#if defined (TACTILE)
#	include "tactile.h"
#endif

#ifdef USE_LINUX_JOY
#include "joystick.h"
#endif
#ifdef WIN32
tTransRotInfo	tirInfo;
#endif

tControlInfo Controls [4];

// *must* be defined - set to 0 if no limit
#define MIN_TIME_360	3.0f	//time for a 360 degree turn in secs
//#define nMaxTurnRate		(gameStates.input.kcPollTime / kcFrameCount)

#define	KCCLAMP(_val,_max) \
			if ((_val) < -(_max)) \
				(_val) = (fix) -(_max); \
			else if ((_val) > (_max)) \
				(_val) = (fix) (_max)

static int	kcFrameCount = 0;
static int	nMaxTurnRate;

#define FASTPITCH	(gameStates.app.bHaveExtraGameInfo [IsMultiGame] ? extraGameInfo [IsMultiGame].bFastPitch : 2)

//------------------------------------------------------------------------------

void SetMaxPitch (int nMinTurnRate)
{
gameOpts->input.nMaxPitch = (int) ((MIN_TIME_360 * 10) / (nMinTurnRate ? nMinTurnRate : 20));
}

//------------------------------------------------------------------------------

#define	PH_SCALE	1

#define	JOYSTICK_READ_TIME	 (F1_0/40)		//	Read joystick at 40 Hz.

fix	LastReadTime = 0;
fix	joyAxis [JOY_MAX_AXES];

//------------------------------------------------------------------------------

fix Next_toggleTime [3]={0,0,0};

int AllowToToggle (int i)
{
//used for keeping tabs of when its ok to toggle headlight,primary,and secondary
if (Next_toggleTime [i] > gameData.time.xGame)
	if (Next_toggleTime [i] < gameData.time.xGame + (F1_0/8))	//	In case time is bogus, never wait > 1 second.
		return 0;
Next_toggleTime [i] = gameData.time.xGame + (F1_0/8);
return 1;
}


#ifdef D2X_KEYS
//added on 2/7/99 by Victor Rachels for jostick state setting
int d2xJoystick_ostate [20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//end this section adition - VR
#endif

//------------------------------------------------------------------------------

int ControlsReadKeyboard (void)
{
return 1;
}

//------------------------------------------------------------------------------

int ControlsReadMouse (int *mouseAxis, int *nMouseButtons)
{
	int	dx, dy;
#ifdef SDL_INPUT
	int	dz;
#endif
#ifdef SDL_INPUT
MouseGetDeltaZ (&dx, &dy, &dz);
#else
MouseGetDelta (&dx, &dy);
#endif
mouseAxis [0] = (int) ((dx * gameStates.input.kcPollTime) / 35);
mouseAxis [1] = (int) ((dy * gameStates.input.kcPollTime) / 25);
#ifdef SDL_INPUT
mouseAxis [2] = (int) (dz * gameStates.input.kcPollTime);
#endif
*nMouseButtons = MouseGetButtons ();
return 1;
}

//------------------------------------------------------------------------------

static int joy_sens_mod [4] = {16, 16, 16, 16};

static double dMaxAxis = 127.0;

int AttenuateAxis (double a, int nAxis)
{
if (gameOpts->input.joystick.bLinearSens)
	return (int) a;
else if (!a)
	return 0;
else {
		double	d;
#if 0//def _DEBUG
		double	p, h, e;
#endif

	if (a > dMaxAxis)
		a = dMaxAxis;
	else if (a < -dMaxAxis)
		a = -dMaxAxis;
#if 1//ndef _DEBUG
	d = dMaxAxis * pow (fabs ((double) a / dMaxAxis), (double) joy_sens_mod [nAxis % 4] / 16.0);
#else
	h = fabs ((double) a / dMaxAxis);
	e = (double) joy_sens_mod [nAxis % 4] / 16.0;
	p = pow (h, e);
	d = dMaxAxis * p;
#endif
	if (d < 1.0)
		d = 1.0;
	if (a < 0)
		d = -d;
	return (int) d;
	}
}

//------------------------------------------------------------------------------

inline int ControlsReadJoyAxis (int i, int rawJoyAxis [])
{
int joyDeadzoneScaled = joyDeadzone [i % 4] / 128;
int h = JoyGetScaledReading (rawJoyAxis [i], i);
if (gameOpts->input.joystick.bSyncAxes && 
	 ((kcJoystick [18].value == i) || (kcJoystick [48].value == i)))		// If this is the throttle
	joyDeadzoneScaled *= 2;				// Then use a larger dead-zone
if (h > joyDeadzoneScaled) 
	h = ((h - joyDeadzoneScaled) * 128) / (128 - joyDeadzoneScaled);
else if (h < -joyDeadzoneScaled)
	h = ((h + joyDeadzoneScaled) * 128) / (128 - joyDeadzoneScaled);
else
	h = 0;
return (int) ((AttenuateAxis (h, i) * gameStates.input.kcPollTime) / 128);
}

//------------------------------------------------------------------------------

int ControlsReadJoystick (int *joyAxis)
{
	int	rawJoyAxis [JOY_MAX_AXES];
	ulong channelMasks;
	int	i, bUseJoystick = 0;
	fix	ctime = 0;

if (gameStates.limitFPS.bJoystick)
	ctime = TimerGetFixedSeconds ();

for (i = 0; i < 4; i++)
	joy_sens_mod [i] = 128 - 7 * gameOpts->input.joystick.sensitivity [i];
if (gameStates.limitFPS.bJoystick) {
	if ((LastReadTime + JOYSTICK_READ_TIME > ctime) && (gameStates.input.nJoyType != CONTROL_THRUSTMASTER_FCS)) {
		if ((ctime < 0) && (LastReadTime >= 0))
			LastReadTime = ctime;
		bUseJoystick=1;
		} 
	else if (gameOpts->input.joystick.bUse) {
		LastReadTime = ctime;
		if	((channelMasks = JoyReadRawAxis (JOY_ALL_AXIS, rawJoyAxis))) {
			for (i = 0; i < JOY_MAX_AXES; i++) {
#ifndef SDL_INPUT
				if (channelMasks & (1 << i))	{
#endif
					if ((i == 3) && (gameStates.input.nJoyType == CONTROL_THRUSTMASTER_FCS))
						ControlsReadFCS (rawJoyAxis [i]);
					else 
						joyAxis [i] = ControlsReadJoyAxis (i, rawJoyAxis);
#ifndef SDL_INPUT
					} 
					else
						joyAxis [i] = 0;
#endif
				}
			}
		bUseJoystick = 1;
		}
	else {
		for (i = 0; i < JOY_MAX_AXES; i++)
			joyAxis [i] = 0;
		}
	}
else {   // LIMIT_JOY_FPS
	memset (joyAxis, 0, sizeof (joyAxis));
	if (gameOpts->input.joystick.bUse) {
		if ((channelMasks = JoyReadRawAxis (JOY_ALL_AXIS, rawJoyAxis))) {
			for (i = 0; i < JOY_MAX_AXES; i++)	{
				if (channelMasks & (1 << i))
					joyAxis [i] = ControlsReadJoyAxis (i, rawJoyAxis);
				else
					joyAxis [i] = 0;
				}
			}
		bUseJoystick = 1;
		}
	}
return bUseJoystick;
}

//------------------------------------------------------------------------------

void ControlsSetFCSButton (int btn, int button)
{
	int state, timeDown, upcount, downcount;
	state = timeDown = upcount = downcount = 0;

if (JoyGetButtonState (btn)) {
	if (btn == button) {
		state = 1;
		timeDown = gameData.time.xFrame;
		} 
	else
		upcount=1;
	}
else {
	if (btn == button) {
		state = 1;
		timeDown = gameData.time.xFrame;
		downcount = 1;
		}
	else
		upcount=1;
	}			
}

//------------------------------------------------------------------------------

void ControlsReadFCS (int rawAxis)
{
	int raw_button, button;
	tJoyAxisCal	cal [4];

if (gameStates.input.nJoyType != CONTROL_THRUSTMASTER_FCS) 
	return;
JoyGetCalVals (cal, sizeofa (cal));
if (cal [3].nMax > 1)
	raw_button = (rawAxis * 100) / cal [3].nMax;
else
	raw_button = 0;
if (raw_button > 88)
	button = 0;
else if (raw_button > 63)
	button = 7;
else if (raw_button > 39)
	button = 11;
else if (raw_button > 15)
	button = 15;
else
	button = 19;
ControlsSetFCSButton (19, button);
ControlsSetFCSButton (15, button);
ControlsSetFCSButton (11, button);
ControlsSetFCSButton (7, button);
}
	
//------------------------------------------------------------------------------

int ControlsReadCyberman (int *mouseAxis, int *nMouseButtons)
{
	int idx, idy;

MouseGetCybermanPos (&idx, &idy);
mouseAxis [0] = (int) ((idx * gameStates.input.kcPollTime) / 128);
mouseAxis [1] = (int) ((idy * gameStates.input.kcPollTime) / 128);
*nMouseButtons = MouseGetButtons ();
return 1;
}

//------------------------------------------------------------------------------

inline int HaveKey (kcItem *k, int i)
{
return k [i].value;
}

//------------------------------------------------------------------------------

inline int HaveKeyCount (kcItem *k, int i)
{
	int	v = k [i].value;

if (v == 255)
	return 0;
#if DBG
v = KeyDownCount (v);
if (!v)
	return 0;
return v;
#else
return KeyDownCount (v);
#endif
}

//------------------------------------------------------------------------------

inline int HaveD2XKey (kcItem *k, int i)
{
	int	v = k [i].value;

if (v == 255)
	return 0;
if (KeyFlags (v))
	return 0;
#if DBG
if (v = KeyDownCount (v))
	return v;
return 0;
#else
return KeyDownCount (v);
#endif
}

//------------------------------------------------------------------------------

void ControlsDoD2XKeys (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank)
{
#ifdef D2X_KEYS
//added on 2/4/99 by Victor Rachels for d1x keys
//--------- Read primary weapon select -------------
//the following "if" added by WraithX to stop deadies from switchin weapons, 4/14/00
if (!(gameStates.app.bPlayerIsDead || gameStates.render.automap.bDisplay)) { {
		int i, d2xJoystickState [10];

	for (i = 0; i < 10; i++)
		d2xJoystickState [i] = JoyGetButtonState (kcHotkeys [i * 2+1].value);

	//----------------Weapon 1----------------
	if (HaveD2XKey (kcHotkeys, 0) ||
			(JoyGetButtonState (kcHotkeys [1].value) && (d2xJoystickState [0] != d2xJoystick_ostate [0])))
	{
		//int i, valu=0;
		DoSelectWeapon (0,0);
		/*
		for (i=MAX_PRIMARY_WEAPONS;i<MAX_PRIMARY_WEAPONS+NEWPRIMS;i++)
			if (primary_order [i]>primary_order [valu]&&PlayerHasWeapon (i,0,1))
				valu = i;
		LaserPowSelected = valu;
		*/
	}
	//----------------Weapon 2----------------
	if (HaveD2XKey (kcHotkeys, 2) ||
			(JoyGetButtonState (kcHotkeys [3].value) && (d2xJoystickState [1] != d2xJoystick_ostate [1])))
		DoSelectWeapon (1,0);
	//----------------Weapon 3----------------
	if (HaveD2XKey (kcHotkeys, 4) ||
			(JoyGetButtonState (kcHotkeys [5].value) && (d2xJoystickState [2] != d2xJoystick_ostate [2])))
		DoSelectWeapon (2,0);
	//----------------Weapon 4----------------
	if (HaveD2XKey (kcHotkeys, 6) ||
			(JoyGetButtonState (kcHotkeys [7].value) && (d2xJoystickState [3] != d2xJoystick_ostate [3])))
		DoSelectWeapon (3,0);
	//----------------Weapon 5----------------
	if (HaveD2XKey (kcHotkeys, 8) ||
			(JoyGetButtonState (kcHotkeys [9].value) && (d2xJoystickState [4] != d2xJoystick_ostate [4])))
		DoSelectWeapon (4,0);

	//--------- Read secondary weapon select ----------
	//----------------Weapon 6----------------
	if (HaveD2XKey (kcHotkeys, 10) ||
			(JoyGetButtonState (kcHotkeys [11].value) && (d2xJoystickState [5] != d2xJoystick_ostate [5])))
		DoSelectWeapon (0,1);
	//----------------Weapon 7----------------
	if (HaveD2XKey (kcHotkeys, 12) ||
			(JoyGetButtonState (kcHotkeys [13].value) && (d2xJoystickState [6] != d2xJoystick_ostate [6])))
		DoSelectWeapon (1,1);
	//----------------Weapon 8----------------
	if (HaveD2XKey (kcHotkeys, 14) ||
			(JoyGetButtonState (kcHotkeys [15].value) && (d2xJoystickState [7] != d2xJoystick_ostate [7])))
		DoSelectWeapon (2,1);
	//----------------Weapon 9----------------
	if (HaveD2XKey (kcHotkeys, 16) ||
			(JoyGetButtonState (kcHotkeys [17].value) && (d2xJoystickState [8] != d2xJoystick_ostate [8])))
		DoSelectWeapon (3,1);
	//----------------Weapon 0----------------
	if (HaveD2XKey (kcHotkeys, 18) ||
			(JoyGetButtonState (kcHotkeys [19].value) && (d2xJoystickState [9] != d2xJoystick_ostate [9])))
		DoSelectWeapon (4,1);
	memcpy (d2xJoystick_ostate, d2xJoystickState, 10 * sizeof (int));
	}
	//end this section addition - VR
}//end "if (!gameStates.app.bPlayerIsDead)" - WraithX
#endif
}

//------------------------------------------------------------------------------

int ControlsLimitTurnRate (int bUseMouse)
{
if (!(gameOpts->input.bLimitTurnRate || IsMultiGame))
	return 0;
if (gameStates.render.automap.bDisplay || 
	 gameOpts->input.mouse.bJoystick ||
	 gameStates.app.bNostalgia ||
	 COMPETITION ||
	 !(bUseMouse && EGI_FLAG (bMouseLook, 0, 1, 0))) {
	KCCLAMP (Controls [0].pitchTime, nMaxTurnRate / FASTPITCH);
	KCCLAMP (Controls [0].headingTime, nMaxTurnRate);
	}
KCCLAMP (Controls [0].bankTime, gameStates.input.kcFrameTime);
return 1;
}

//------------------------------------------------------------------------------

extern fix KeyRamp (int scancode);

inline int DeltaCtrl (ubyte v, int speedFactor, int keyRampScale, int i)
{
	int	h;

if (v == 255)
	return 0;
h = speedFactor * KeyDownTime (v);
if (!h)
	return 0;
if (gameOpts->input.keyboard.bRamp [i])
	return h / KeyRamp (v);
return h;
}


#define DELTACTRL(_i,_b)	DeltaCtrl (kcKeyboard [_i].value, speedFactor, gameOpts->input.keyboard.nRamp, _b)

void ControlsDoKeyboard (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank)
{
	int	i, v, pitchScale = (!(gameStates.app.bNostalgia || COMPETITION) && 
									 (FASTPITCH == 1)) ? 2 * PH_SCALE : 1;
	int	speedFactor = gameStates.app.cheats.bTurboMode ? 2 : 1;
	static int key_signs [8] = {1,1,-1,-1,-1,-1,1,1};

if (bGetSlideBank == 0) {
	for (i = 0; i < 2; i++) {
		if ((v = HaveKey (kcKeyboard, 8 + i)) < 255) {
			if (gameStates.input.keys.pressed [v])
				*bSlideOn |= gameStates.input.keys.pressed [v];
			}
		if ((v = HaveKey (kcKeyboard, 18 + i)) < 255) 
			if (gameStates.input.keys.pressed [v])
				*bBankOn |= gameStates.input.keys.pressed [v];
		}
	return;
	}

if (bGetSlideBank == 2) {
	for (i = 0; i < 2; i++) {
		if ((v = HaveKey (kcKeyboard, 24 + i)) < 255) {
			Controls [0].firePrimaryState |= gameStates.input.keys.pressed [v];
			Controls [0].firePrimaryDownCount += KeyDownCount (v);
			}
		if ((v = HaveKey (kcKeyboard, 26 + i)) < 255) {
			Controls [0].fireSecondaryState |= gameStates.input.keys.pressed [v];
			Controls [0].fireSecondaryDownCount += KeyDownCount (v);
			}
		if ((v = HaveKey (kcKeyboard, 28 + i)) < 255) 
			Controls [0].fireFlareDownCount += KeyDownCount (v);
		}

	for (i = 0; i < 2; i++) {
		Controls [0].sidewaysThrustTime -= DELTACTRL (10 + i, 2);
		Controls [0].sidewaysThrustTime += DELTACTRL (12 + i, 2);
		Controls [0].verticalThrustTime += DELTACTRL (14 + i, 2);
		Controls [0].verticalThrustTime -= DELTACTRL (16 + i, 2);
		Controls [1].bankTime += DELTACTRL (20 + i, 1);
		Controls [1].bankTime -= DELTACTRL (22 + i, 1);
		Controls [0].forwardThrustTime += DELTACTRL (30 + i, 0);
		Controls [0].forwardThrustTime -= DELTACTRL (32 + i, 0);
		if ((v = HaveKey (kcKeyboard, 46 + i)) < 255) 
			Controls [0].afterburnerState |= gameStates.input.keys.pressed [v];
		// count bomb drops
		if ((v = HaveKey (kcKeyboard, 34 + i)) < 255) 
			Controls [0].dropBombDownCount += KeyDownCount (v);
		// charge chield
		if (LOCALPLAYER.flags & PLAYER_FLAGS_CONVERTER) {
			if (gameStates.input.keys.pressed [v = HaveKey (kcKeyboard, 56 + i)])
				TransferEnergyToShield (KeyDownTime (v));
			}
		// rear view
		if ((v = HaveKey (kcKeyboard, 36 + i)) < 255) {
			Controls [0].rearViewDownCount += KeyDownCount (v);
			Controls [0].rearViewDownState |= gameStates.input.keys.pressed [v];
			}
		// automap
		if ((v = HaveKey (kcKeyboard, 44 + i)) < 255) {
			Controls [0].automapDownCount += KeyDownCount (v);
			Controls [0].automapState |= gameStates.input.keys.pressed [v];
			}
	}
	// headlight and weapon cycling
	if ((v = HaveKey (kcKeyboard, 54)) < 255)
		Controls [0].headlightCount = KeyDownCount (v);
	Controls [0].headlightCount += HaveKeyCount (kcKeyboard, 55);
	Controls [0].cyclePrimaryCount = HaveKeyCount (kcKeyboard, 48);
	Controls [0].cyclePrimaryCount += HaveKeyCount (kcKeyboard, 49);
	Controls [0].cycleSecondaryCount += HaveKeyCount (kcKeyboard, 50);
	Controls [0].cycleSecondaryCount += HaveKeyCount (kcKeyboard, 51);
	Controls [0].zoomDownCount += HaveKeyCount (kcKeyboard, 52);
	Controls [0].zoomDownCount += HaveKeyCount (kcKeyboard, 53);
	Controls [0].toggleIconsCount += HaveKeyCount (kcKeyboard, 59);
	Controls [0].useCloakDownCount += HaveKeyCount (kcKeyboard, 60);
	Controls [0].useCloakDownCount += HaveKeyCount (kcKeyboard, 61);
	Controls [0].useInvulDownCount += HaveKeyCount (kcKeyboard, 62);
	Controls [0].useInvulDownCount += HaveKeyCount (kcKeyboard, 63);
	Controls [0].slowMotionCount += HaveKeyCount (kcKeyboard, 64);
	Controls [0].bulletTimeCount += HaveKeyCount (kcKeyboard, 65);

	// toggle bomb
	if (((v = HaveKey (kcKeyboard, 58)) < 255) && KeyDownCount (v))
		ToggleBomb ();

	// cruise speed
	for (i = 0; i < 4; i++)
		if ((v = HaveKey (kcKeyboard, 38 + i)) < 255) 
			*nCruiseSpeed += key_signs [i] * FixDiv (speedFactor * KeyDownTime (v) * 5, gameStates.input.kcPollTime);
	for (i = 0; i < 2; i++)
		if (((v = HaveKey (kcKeyboard, 
 + i)) < 255) && KeyDownCount (v))
			*nCruiseSpeed = 0;
	}

// special slide & bank toggle handling
if (*bSlideOn) {
	if (bGetSlideBank == 2) {
		for (i = 0; i < 2; i++) {
			Controls [0].verticalThrustTime += DELTACTRL (i, 2);
			Controls [0].verticalThrustTime -= DELTACTRL (2 + i, 2);
			Controls [0].sidewaysThrustTime -= DELTACTRL (4 + i, 2);
			Controls [0].sidewaysThrustTime += DELTACTRL (6 + i, 2);
			}
		}
	}
else if (bGetSlideBank == 1) {
	for (i = 0; i < 4; i++)
		Controls [1].pitchTime += key_signs [i] * DELTACTRL (i, 1) / pitchScale;
	if (!*bBankOn)
		for (i = 4; i < 8; i++)
			Controls [1].headingTime += key_signs [i] * DELTACTRL (i, 1) / PH_SCALE;
	}
if (*bBankOn) {
	if (bGetSlideBank == 2) {
		for (i = 4; i < 6; i++)
			Controls [1].bankTime += DELTACTRL (i, 1);
		for (i = 6; i < 8; i++)
			Controls [1].bankTime -= DELTACTRL (i, 1);
		}
	}
if (bGetSlideBank == 2)
	ControlsLimitTurnRate (0);
//KCCLAMP (pitchTime, nMaxTurnRate / FASTPITCH);
*pitchTimeP = Controls [1].pitchTime;
*headingTimeP = Controls [1].headingTime;
}

//------------------------------------------------------------------------------

inline int ipower (int x, int y)
{
	int h = x;
	int r = y % 2;

while (y > 1) {
	h *= h;
	y /= 2;
	}
if (r)
	h *= x;
return h;
}

//------------------------------------------------------------------------------

inline int DeltaAxis (int v)
{
#if DBG
int a = gameOpts->input.joystick.bLinearSens ? joyAxis [v] * 16 / joy_sens_mod [v % 4] : joyAxis [v];
if (a)
	HUDMessage (0, "%d", a);
return a;
#else
return gameOpts->input.joystick.bLinearSens ? joyAxis [v] * 16 / joy_sens_mod [v % 4] : joyAxis [v];
#endif
}

//------------------------------------------------------------------------------

void ControlsDoJoystick (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank)
{
	int	i, v;

for (i = 0; i < 60; i += 30) {
	if (bGetSlideBank == 0) {
		if ((v = kcJoystick [i + 5].value) < 255) 
			*bSlideOn |= JoyGetButtonState (v);
		if ((v = kcJoystick [i + 10].value) < 255)
			*bBankOn |= JoyGetButtonState (v);
		return;
		}
	// Buttons
	if (bGetSlideBank == 2) {
		if ((v = kcJoystick [i + 0].value) < 255) {
			Controls [0].firePrimaryState |= JoyGetButtonState (v);
			Controls [0].firePrimaryDownCount += JoyGetButtonDownCnt (v);
			}
		if ((v = kcJoystick [i + 1].value) < 255) {
			Controls [0].fireSecondaryState |= JoyGetButtonState (v);
			Controls [0].fireSecondaryDownCount += JoyGetButtonDownCnt (v);
			}
		if ((v = kcJoystick [i + 2].value) < 255) 
			Controls [0].forwardThrustTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 3].value) < 255) 
			Controls [0].forwardThrustTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 4].value) < 255) 
			Controls [0].fireFlareDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 6].value) < 255) 
			Controls [0].sidewaysThrustTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 7].value) < 255) 
			Controls [0].sidewaysThrustTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 8].value) < 255) 
			Controls [0].verticalThrustTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 9].value) < 255) 
			Controls [0].verticalThrustTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 11].value) < 255) 
			Controls [2].bankTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 12].value) < 255) 
			Controls [2].bankTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 19].value) < 255) {
			Controls [0].rearViewDownCount += JoyGetButtonDownCnt (v);
			Controls [0].rearViewDownState |= JoyGetButtonState (v);
			}
		if ((v = kcJoystick [i + 20].value) < 255) 
			Controls [0].dropBombDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 21].value) < 255) 
			Controls [0].afterburnerState |= JoyGetButtonState (v);
		if ((v = kcJoystick [i + 22].value) < 255) 
			Controls [0].cyclePrimaryCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 23].value) < 255) 
			Controls [0].cycleSecondaryCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 24].value) < 255) 
			Controls [0].headlightCount += JoyGetButtonDownCnt (v);
		if (((v = kcJoystick [i + 25].value) < 255) && JoyGetButtonDownCnt (v))
			ToggleBomb ();
		if ((v = kcJoystick [i + 26].value) < 255) 
			Controls [0].toggleIconsCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 27].value) < 255) 
			Controls [0].automapDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 28].value) < 255) 
			Controls [0].useCloakDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 29].value) < 255) 
			Controls [0].useInvulDownCount += JoyGetButtonDownCnt (v);

		// Axis movements
		if ((v = kcJoystick [i + 15].value) < 255) {
			if (kcJoystick [62].value)		// If inverted...
				Controls [0].sidewaysThrustTime += joyAxis [v];
			else
				Controls [0].sidewaysThrustTime -= joyAxis [v];
			}
		if ((v = kcJoystick [i + 16].value) < 255) {
			if (kcJoystick [63].value)		// If inverted...
				Controls [0].verticalThrustTime -= joyAxis [v];
			else
				Controls [0].verticalThrustTime += joyAxis [v];
			}
		if ((v = kcJoystick [i + 17].value) < 255) {
			if (kcJoystick [64].value)		// If inverted...
				Controls [2].bankTime += joyAxis [v];
			else
				Controls [2].bankTime -= joyAxis [v];
			}
		if ((v = kcJoystick [i + 18].value) < 255) {
			if (kcJoystick [65].value)		// If inverted...
				Controls [0].forwardThrustTime += joyAxis [v];
			else
				Controls [0].forwardThrustTime -= joyAxis [v];
			}
		// special continuous slide & bank handling
		if (*bSlideOn) {
			if ((v = kcJoystick [i + 13].value) < 255) {
				if (kcJoystick [60].value)		// If inverted...
					Controls [0].verticalThrustTime -= joyAxis [v];
				else
					Controls [0].verticalThrustTime += joyAxis [v];
			}
			if ((v = kcJoystick [i + 14].value) < 255) {
				if (kcJoystick [61].value)		// If inverted...
					Controls [0].sidewaysThrustTime -= joyAxis [v];
				else
					Controls [0].sidewaysThrustTime += joyAxis [v];
				}
			}
		else {
			if ((v = kcJoystick [i + 13].value) < 255) {
				if (kcJoystick [60].value)		// If inverted...
					Controls [2].pitchTime += DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				else 
					Controls [2].pitchTime -= DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				}
			if (!*bBankOn) {
				if ((v = kcJoystick [i + 14].value) < 255) {
					if (kcJoystick [61].value)		// If inverted...
						Controls [2].headingTime -= DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod); //kcFrameCount;
					else
						Controls [2].headingTime += DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod); // kcFrameCount;
					}
				}
			}
		if (*bBankOn) {
			if ((v = kcJoystick [i + 14].value) < 255) {
				if (kcJoystick [61].value)		// If inverted...
					Controls [2].bankTime += DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				else
					Controls [2].bankTime -= DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int CalcDeadzone (int d, int nDeadzone)
{
double	r = 32 * nDeadzone;
return (int) (r ? (d ? sqrt (r * r - d * d) : r) : 0);
}

//------------------------------------------------------------------------------

void ControlsDoMouse (int *mouseAxis, int nMouseButtons, 
							 int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, 
							 int bGetSlideBank)
{
	int	v, nMouseSensMod = 8;

if (bGetSlideBank == 0) {
	if ((v = kcMouse [5].value) < 255) 
		*bSlideOn |= nMouseButtons & (1 << v);
	if ((v = kcMouse [10].value) < 255) 
		*bBankOn |= nMouseButtons & (1 << v);
	return;
	}
// Buttons
if (bGetSlideBank == 2) {
	if ((v = kcMouse [0].value) < 255) {
		Controls [0].firePrimaryState |= MouseButtonState (v);
		Controls [0].firePrimaryDownCount += MouseButtonDownCount (v);
		}
	if ((v = kcMouse [1].value) < 255) {
		Controls [0].fireSecondaryState |= MouseButtonState (v);
		Controls [0].fireSecondaryDownCount += MouseButtonDownCount (v);
		}
	if ((v = kcMouse [2].value) < 255) 
		Controls [0].forwardThrustTime += MouseButtonDownTime (v);
	if ((v = kcMouse [3].value) < 255) 
		Controls [0].forwardThrustTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [4].value) < 255) 
		Controls [0].fireFlareDownCount += MouseButtonDownCount (v);
	if ((v = kcMouse [6].value) < 255) 
		Controls [0].sidewaysThrustTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [7].value) < 255) 
		Controls [0].sidewaysThrustTime += MouseButtonDownTime (v);
	if ((v = kcMouse [8].value) < 255) 
		Controls [0].verticalThrustTime += MouseButtonDownTime (v);
	if ((v = kcMouse [9].value) < 255) 
		Controls [0].verticalThrustTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [11].value) < 255) 
		Controls [3].bankTime += MouseButtonDownTime (v);
	if ((v = kcMouse [12].value) < 255) 
		Controls [3].bankTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [25].value) < 255) {
		Controls [0].rearViewDownCount += MouseButtonDownCount (v);
		Controls [0].rearViewDownState |= MouseButtonState (v);
		}
	if ((v = kcMouse [26].value) < 255) 
		Controls [0].dropBombDownCount += MouseButtonDownCount (v);
	if ((v = kcMouse [27].value) < 255) 
		Controls [0].afterburnerState |= MouseButtonState (v);
	if (((v = kcMouse [28].value) < 255) && MouseButtonState (v))
		Controls [0].cyclePrimaryCount += MouseButtonDownCount (v);
	if (((v = kcMouse [29].value) < 255) && MouseButtonState (v))
		Controls [0].cycleSecondaryCount += MouseButtonDownCount (v);
	if (((v = kcMouse [30].value) < 255) && MouseButtonState (v))
		Controls [0].zoomDownCount += MouseButtonDownCount (v);
	// Axis movements
	if ((v = kcMouse [17].value) < 255) {
		if (kcMouse [18].value)		// If inverted...
			Controls [0].sidewaysThrustTime -= mouseAxis [v];
		else
			Controls [0].sidewaysThrustTime += mouseAxis [v];
		}
	if ((v = kcMouse [19].value) < 255) {
		if (kcMouse [20].value)		// If inverted...
			Controls [0].verticalThrustTime -= mouseAxis [v];
		else
			Controls [0].verticalThrustTime += mouseAxis [v];
		}
	if (((v = kcMouse [21].value) < 255) && mouseAxis [v]) {
		if (kcMouse [22].value)		// If inverted...
			Controls [3].bankTime -= mouseAxis [v];
		else
			Controls [3].bankTime += mouseAxis [v];
		}
	if ((v = kcMouse [23].value) < 255) {
		if (kcMouse [24].value)		// If inverted...
			Controls [0].forwardThrustTime += mouseAxis [v];
		else
			Controls [0].forwardThrustTime -= mouseAxis [v];
		}
	// special continuous slide & bank handling
	if (*bSlideOn) {
		if ((v = kcMouse [13].value) < 255) {
			if (kcMouse [14].value)		// If inverted...
				Controls [0].verticalThrustTime += mouseAxis [v];
			else
				Controls [0].verticalThrustTime -= mouseAxis [v];
			}
		if ((v = kcMouse [15].value) < 255)	{
			if (kcMouse [16].value)		// If inverted...
				Controls [0].sidewaysThrustTime -= mouseAxis [v];
			else
				Controls [0].sidewaysThrustTime += mouseAxis [v];
			}
		}
	else {
		SDL_GetMouseState (&mouseData.x, &mouseData.y);
		if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick) {
			int dx = mouseData.x - SWIDTH / 2;
			int dz = CalcDeadzone (mouseData.y - SHEIGHT / 2, gameOpts->input.mouse.nDeadzone);
			if (dx < 0) {
				if (dx > -dz)
					dx = 0;
				else
					dx += dz;
				}
			else {
				//dz = dz * screen.Width () / screen.Height ();
				if (dx < dz)
					dx = 0;
				else
					dx -= dz;
				}
			dx = 640 * dx / (SWIDTH / gameOpts->input.mouse.sensitivity [0]);
			Controls [3].headingTime += dx; // * gameOpts->input.mouse.sensitivity [0]); // nMouseSensMod;
			}
		else {
			if (((v = kcMouse [13].value) < 255) && mouseAxis [v]) {
				if (kcMouse [14].value)		// If inverted...
					Controls [3].pitchTime += (mouseAxis [v] * gameOpts->input.mouse.sensitivity [1]) / nMouseSensMod;
				else
					Controls [3].pitchTime -= (mouseAxis [v] * gameOpts->input.mouse.sensitivity [1]) / nMouseSensMod;
				}
			}
		if (*bBankOn) {
			if (((v = kcMouse [15].value) < 255) && mouseAxis [v]) {
				if (kcMouse [16].value)		// If inverted...
					Controls [3].bankTime -= (mouseAxis [v] * gameOpts->input.mouse.sensitivity [2]) / nMouseSensMod;
				else
					Controls [3].bankTime += (mouseAxis [v] * gameOpts->input.mouse.sensitivity [2]) / nMouseSensMod;
				}
			}
		else {
			if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick) {
				int	dy = mouseData.y - SHEIGHT / 2;
				int	dz = CalcDeadzone (mouseData.x - SWIDTH / 2, gameOpts->input.mouse.nDeadzone);
				if (kcMouse [14].value)
					dy = -dy;
				if (dy < 0) {
					if (dy > -dz)
						dy = 0;
					else
						dy += dz;
				}
				else {
					if (dy < dz)
						dy = 0;
					else
						dy -= dz;
					}
				dy = 480 * dy / (SHEIGHT / gameOpts->input.mouse.sensitivity [1]);
				Controls [3].pitchTime += dy; // * gameOpts->input.mouse.sensitivity [1]); // nMouseSensMod;
				}
			else {
				if (((v = kcMouse [15].value) < 255) && mouseAxis [v]) {
					if (kcMouse [16].value)		// If inverted...
						Controls [3].headingTime -= (mouseAxis [v] * gameOpts->input.mouse.sensitivity [0]) / nMouseSensMod;
					else
						Controls [3].headingTime += (mouseAxis [v] * gameOpts->input.mouse.sensitivity [0]) / nMouseSensMod;
					}
				}
			}
		}
	}

if (gameStates.input.nMouseType == CONTROL_CYBERMAN)	{
	if (bGetSlideBank == 2) {
		Controls [0].verticalThrustTime += MouseButtonDownTime (D2_MB_Z_UP) / 2;
		Controls [0].verticalThrustTime -= MouseButtonDownTime (D2_MB_Z_DOWN) / 2;
		Controls [3].bankTime += MouseButtonDownTime (D2_MB_BANK_LEFT);
		Controls [3].bankTime -= MouseButtonDownTime (D2_MB_BANK_RIGHT);
		}
	if (*bSlideOn) {
		if (bGetSlideBank == 2) {
			Controls [0].verticalThrustTime -= MouseButtonDownTime (D2_MB_PITCH_FORWARD);
			Controls [0].verticalThrustTime += MouseButtonDownTime (D2_MB_PITCH_BACKWARD);
			Controls [0].sidewaysThrustTime -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			Controls [0].sidewaysThrustTime += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
			}
		}
	else if (bGetSlideBank == 1) {
		*pitchTimeP += MouseButtonDownTime (D2_MB_PITCH_FORWARD) / (PH_SCALE * 2);
		*pitchTimeP -= MouseButtonDownTime (D2_MB_PITCH_BACKWARD) / (PH_SCALE * 2);
		if (!*bBankOn) {
			*headingTimeP -= MouseButtonDownTime (D2_MB_HEAD_LEFT) /PH_SCALE;
			*headingTimeP += MouseButtonDownTime (D2_MB_HEAD_RIGHT) /PH_SCALE;
			}
		}
	if (*bBankOn) {
		if (bGetSlideBank == 2) {
			Controls [3].bankTime -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			Controls [3].bankTime += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
			}
		}
	}
}

//------------------------------------------------------------------------------

#ifdef WIN32

int ControlsReadTrackIR (void)
{
viewInfo.bUsePlayerHeadAngles = 0;
if (!(gameStates.input.bHaveTrackIR && gameOpts->input.trackIR.bUse))
	return 0;
if (!pfnTIRQuery (&tirInfo)) {
	pfnTIRExit ();
	if ((gameStates.input.bHaveTrackIR = pfnTIRInit ((HWND) SDL_GetWindowHandle ())))
		pfnTIRStart ();
	return 0;
	}
#if 0//def _DEBUG
HUDMessage (0, "%1.0f %1.0f %1.0f", tirInfo.fvTrans.x, tirInfo.fvTrans.y, tirInfo.fvTrans.z);
#endif
return 1;
}

//------------------------------------------------------------------------------

void ControlsDoTrackIR (void)
{
	int	dx = (int) ((float) tirInfo.fvRot.z * (float) SWIDTH / 16384.0f),
			dy = (int) ((float) tirInfo.fvRot.y * (float) SHEIGHT / 16384.0f),
			dz;
	int	x, y;
	float	fDeadzone, fScale;

if (gameOpts->input.trackIR.nMode == 0) {
#if 1
#else
	dx = (int) tirInfo.fvRot.z * (gameOpts->input.trackIR.sensitivity [0] + 1);
	dy = (int) tirInfo.fvRot.y * (gameOpts->input.trackIR.sensitivity [1] + 1);
#endif
	x = gameData.trackIR.x;
	y = gameData.trackIR.y;
#if 0//def _DEBUG
		HUDMessage (0, "%d/%d %d/%d", x, dx, y, dy);
#endif
	if (abs (dx - x) > gameOpts->input.trackIR.nDeadzone * 4) {
		dx = dx * (gameOpts->input.trackIR.sensitivity [0] + 1) / 4;
		gameData.trackIR.x = dx;
		dx -= x;
		}
	else
		dx = 0;
	if (abs (dy - y) > gameOpts->input.trackIR.nDeadzone * 4) {
		dy = dy * (gameOpts->input.trackIR.sensitivity [1] + 1) / 4;
		gameData.trackIR.y = dy;
		dy -= y;
		}
	else
		dy = 0;
	if (gameOpts->input.trackIR.bMove [0]) {
		Controls [0].headingTime -= (fix) (dx * gameStates.input.kcPollTime);
		Controls [0].pitchTime += (fix) (dy * gameStates.input.kcPollTime);
		}
	if (gameOpts->input.trackIR.bMove [1])
		Controls [0].bankTime += (int) (tirInfo.fvRot.x * gameStates.input.kcPollTime / 131072.0f * (gameOpts->input.trackIR.sensitivity [2] + 1));
	}
else if (gameOpts->input.trackIR.nMode == 1) {
	dx = (int) ((float) tirInfo.fvRot.z * (float) SWIDTH / 16384.0f);
	dy = (int) ((float) tirInfo.fvRot.y * (float) SHEIGHT / 16384.0f);
	dz = 0; //CalcDeadzone (dy, gameOpts->input.trackIR.nDeadzone);
	if (dx < 0) {
		if (dx > -dz)
			dx = 0;
		else
			dx += dz;
	}
	else {
		if (dx < dz)
			dx = 0;
		else
			dx -= dz;
		}
	dz = 0; //CalcDeadzone (dx, gameOpts->input.trackIR.nDeadzone);
	if (dy < 0) {
		if (dy > -dz)
			dy = 0;
		else
			dy += dz;
	}
	else {
		if (dy < dz)
			dy = 0;
		else
			dy -= dz;
		}
#if 0//def _DEBUG
	HUDMessage (0, "%d %d", dx, dy);
#endif
	dx = 640 * dx / (SWIDTH / (gameOpts->input.trackIR.sensitivity [0] + 1));
	dy = 480 * dy / (SHEIGHT / (gameOpts->input.trackIR.sensitivity [1] + 1));
	if (gameOpts->input.trackIR.bMove [0]) {
		Controls [0].headingTime -= dx;
		Controls [0].pitchTime += dy;
		}
	if (gameOpts->input.trackIR.bMove [1])
		Controls [0].bankTime += (int) (tirInfo.fvRot.x * gameStates.input.kcPollTime / 131072.0f * (gameOpts->input.trackIR.sensitivity [2] + 1));
	}
else {
	viewInfo.bUsePlayerHeadAngles = 1;
	if (gameOpts->input.trackIR.bMove [0]) {
		viewInfo.playerHeadAngles[HA] = (fixang) -tirInfo.fvRot.z / 4 * (gameOpts->input.trackIR.sensitivity [0] + 1);
		viewInfo.playerHeadAngles[PA] = (fixang) tirInfo.fvRot.y / 4 * (gameOpts->input.trackIR.sensitivity [1] + 1);
		}
	else
		viewInfo.playerHeadAngles[HA] = 
		viewInfo.playerHeadAngles[PA] = 0;
	if (gameOpts->input.trackIR.bMove [1])
		viewInfo.playerHeadAngles[BA] = (fixang) tirInfo.fvRot.x / 4 * (gameOpts->input.trackIR.sensitivity [2] + 1);
	else
		viewInfo.playerHeadAngles[BA] = 0;
	}
fDeadzone = 256.0f * gameOpts->input.trackIR.nDeadzone;
fScale = 16384.0f / (16384.0f - fDeadzone);
if (gameOpts->input.trackIR.bMove [2] && ((float) fabs (tirInfo.fvTrans.x) > fDeadzone)) {
	if (tirInfo.fvTrans.x < 0)
		tirInfo.fvTrans.x += fDeadzone;
	else
		tirInfo.fvTrans.x -= fDeadzone;
	Controls [0].sidewaysThrustTime -= (int) ((tirInfo.fvTrans.x - fDeadzone) * gameStates.input.kcPollTime / 65536.0f * (gameOpts->input.trackIR.sensitivity [0] + 1));
	}
if (gameOpts->input.trackIR.bMove [3] && ((float) fabs (tirInfo.fvTrans.y) > fDeadzone)) {
	if (tirInfo.fvTrans.y < 0)
		tirInfo.fvTrans.y += fDeadzone;
	else
		tirInfo.fvTrans.y -= fDeadzone;
	Controls [0].verticalThrustTime += (int) ((tirInfo.fvTrans.y - fDeadzone) * gameStates.input.kcPollTime / 65536.0f * (gameOpts->input.trackIR.sensitivity [1] + 1));
	}
if (gameOpts->input.trackIR.bMove [4] && ((float) fabs (tirInfo.fvTrans.z) > fDeadzone)) {
	if (tirInfo.fvTrans.z < 0)
		tirInfo.fvTrans.z += fDeadzone;
	else
		tirInfo.fvTrans.z -= fDeadzone;
	Controls [0].forwardThrustTime -= (int) ((tirInfo.fvTrans.z - fDeadzone) * gameStates.input.kcPollTime / 8192.0f * (gameOpts->input.trackIR.sensitivity [2] + 1));
	}
}

#endif

//------------------------------------------------------------------------------

void ControlsDoSlideBank (int bSlideOn, int bBankOn, fix pitchTime, fix headingTime)
{
if (bSlideOn) 
	Controls [0].headingTime =
	Controls [0].pitchTime = 0;
else {
	if (pitchTime == 0)
		Controls [0].pitchTime = 0;
	else {
		if (pitchTime > 0) {
			if (Controls [0].pitchTime < 0)
				Controls [0].pitchTime = 0;
			}
		else {// pitchTime < 0
			if (Controls [0].pitchTime > 0)
				Controls [0].pitchTime = 0;
			}
		Controls [0].pitchTime += pitchTime;
		}
	if (!bBankOn) {
		if (headingTime == 0)
			Controls [0].headingTime = 0;
		else {
			if (headingTime > 0) {
				if (Controls [0].headingTime < 0)
					Controls [0].headingTime = 0;
				}
			else {
				if (Controls [0].headingTime > 0)
					Controls [0].headingTime = 0;
				}
			Controls [0].headingTime += headingTime;
			}
		}
	}
}

//------------------------------------------------------------------------------

int ControlsCapSampleRate (void)
{
//	DEFTIME

if (gameStates.limitFPS.bControls) {
	// check elapsed time since last call to ControlsReadAll
	// if less than 25 ms (i.e. 40 fps) return
	if (gameOpts->legacy.bMouse) 
		gameStates.input.kcPollTime = gameData.time.xFrame;
	else {
		if (gameData.app.bGamePaused)
			GetSlowTicks ();
		kcFrameCount++;
		gameStates.input.kcPollTime += gameData.time.xFrame;
		if (!gameStates.app.tick40fps.bTick)
			return 1;
		}
	}
else
	gameStates.input.kcPollTime = gameData.time.xFrame;
gameStates.input.kcFrameTime = (float) gameStates.input.kcPollTime / kcFrameCount;
#if 1
nMaxTurnRate = (int) gameStates.input.kcFrameTime;
#else
nMaxTurnRate = (int) (gameStates.input.kcFrameTime * (1.0f - X2F (gameData.pig.ship.player->maxRotThrust)));
#endif
if (CheckGameConfig ())
	SDL_Delay (gameData.app.nFrameCount % 100);
return 0;
}

//------------------------------------------------------------------------------

void ControlsResetControls (void)
{
//fix ht = Controls [0].headingTime;
//fix pt = Controls [0].pitchTime;
memset (&Controls, 0, sizeof (tControlInfo));
//Controls [0].headingTime = ht;
//Controls [0].pitchTime = pt;
ControlsLimitTurnRate (gameOpts->input.mouse.bUse && !gameStates.input.bCybermouseActive);
}

//------------------------------------------------------------------------------

void SetControlType (void)
{
if (gameStates.input.nMouseType < 0)
	gameStates.input.nMouseType = (gameConfig.nControlType == CONTROL_CYBERMAN) ? CONTROL_CYBERMAN : CONTROL_MOUSE;
if (gameStates.input.nJoyType < 0)
	gameStates.input.nJoyType = ((gameConfig.nControlType >= CONTROL_FLIGHTSTICK_PRO) && (gameConfig.nControlType <= CONTROL_GRAVIS_GAMEPAD)) ? gameConfig.nControlType : CONTROL_JOYSTICK;
if (gameOpts->input.joystick.bUse)
	gameConfig.nControlType = gameStates.input.nJoyType;
else if (gameOpts->input.mouse.bUse)
	gameConfig.nControlType = gameStates.input.nMouseType;
else
	gameConfig.nControlType = CONTROL_NONE;
}

//------------------------------------------------------------------------------

int ControlsReadAll (void)
{
	int	i;
	int	bSlideOn, bBankOn;
	int	nBankSensMod;
	fix	pitchTime, headingTime;
	fix	mouseAxis [3];
	int	nMouseButtons;
	int	bUseMouse, bUseJoystick;

Controls [0].cyclePrimaryCount = 0;
Controls [0].cycleSecondaryCount = 0;
Controls [0].toggleIconsCount = 0;
Controls [0].zoomDownCount = 0;
Controls [0].headlightCount = 0; 
Controls [0].fireFlareDownCount = 0;
Controls [0].dropBombDownCount = 0;
Controls [0].automapDownCount = 0;
Controls [0].rearViewDownCount = 0;
gameStates.input.bControlsSkipFrame = 1;
if (ControlsCapSampleRate ())
	return 1;
gameStates.input.bControlsSkipFrame = 0;
ControlsResetControls ();
gameStates.input.bKeepSlackTime = 1;
nBankSensMod = kcFrameCount;
nMouseButtons = 0;
bUseMouse = 0;
bSlideOn = 0;
bBankOn = 0;
#ifdef FAST_EVENTPOLL
if (!gameOpts->legacy.bInput)
	event_poll (SDL_ALLEVENTS);	//why poll 2 dozen times in the following code when input polling calls all necessary input handlers anyway?
#endif

SetControlType ();
bUseJoystick = gameOpts->input.joystick.bUse && ControlsReadJoystick (reinterpret_cast<int*> (&joyAxis [0]));
if (gameOpts->input.mouse.bUse)
	if (gameStates.input.bCybermouseActive) {
//		ReadOWL (kc_external_control);
//		CybermouseAdjust ();
		} 
	else if (gameStates.input.nMouseType == CONTROL_CYBERMAN)
		bUseMouse = ControlsReadCyberman (reinterpret_cast<int*> (&mouseAxis [0]), &nMouseButtons);
	else
		bUseMouse = ControlsReadMouse (reinterpret_cast<int*> (&mouseAxis [0]), &nMouseButtons);
else {
	mouseAxis [0] =
	mouseAxis [1] =
	mouseAxis [2] = 0;
	nMouseButtons = 0;
	bUseMouse = 0;
	}
pitchTime = headingTime = 0;
memset (Controls + 1, 0, 3 * sizeof (tControlInfo));
for (i = 0; i < 3; i++) {
	ControlsDoKeyboard (&bSlideOn, &bBankOn, &pitchTime, &headingTime, reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
	if (bUseJoystick)
		ControlsDoJoystick (&bSlideOn, &bBankOn, &pitchTime, &headingTime, reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
	if (bUseMouse)
		ControlsDoMouse (reinterpret_cast<int*> (&mouseAxis [0]), nMouseButtons, &bSlideOn, &bBankOn, &pitchTime, &headingTime, 
							  reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
	Controls [0].pitchTime = Controls [1].pitchTime + Controls [2].pitchTime + Controls [3].pitchTime;
	Controls [0].headingTime = Controls [1].headingTime + Controls [2].headingTime + Controls [3].headingTime;
	Controls [0].bankTime = Controls [1].bankTime + Controls [2].bankTime + Controls [3].bankTime;
	if (i == 1) {
		ControlsDoSlideBank (bSlideOn, bBankOn, pitchTime, headingTime);
		}
	}
if (gameOpts->input.bUseHotKeys)
	ControlsDoD2XKeys (&bSlideOn, &bBankOn, &pitchTime, &headingTime, reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
#ifdef WIN32
if (ControlsReadTrackIR ())
	ControlsDoTrackIR ();
#endif
if (gameStates.input.nCruiseSpeed > I2X (100)) 
	gameStates.input.nCruiseSpeed = I2X (100);
else if (gameStates.input.nCruiseSpeed < 0) 
	gameStates.input.nCruiseSpeed = 0;
if (!Controls [0].forwardThrustTime)
	Controls [0].forwardThrustTime = FixMul (gameStates.input.nCruiseSpeed,gameStates.input.kcPollTime) /100;

#if 0 //LIMIT_CONTROLS_FPS
if (nBankSensMod > 2) {
	Controls [0].bankTime *= 2;
	Controls [0].bankTime /= nBankSensMod;
	}
#endif

//----------- Clamp values between -gameStates.input.kcPollTime and gameStates.input.kcPollTime
if (gameStates.input.kcPollTime > F1_0) {
#if TRACE
	con_printf (1, "Bogus frame time of %.2f seconds\n", X2F (gameStates.input.kcPollTime));
#endif
	gameStates.input.kcPollTime = F1_0;
	}
#if 0
Controls [0].verticalThrustTime /= kcFrameCount;
Controls [0].sidewaysThrustTime /= kcFrameCount;
Controls [0].forwardThrustTime /= kcFrameCount;
Controls [0].headingTime /= kcFrameCount;
Controls [0].pitchTime /= kcFrameCount;
Controls [0].bankTime /= kcFrameCount;
#endif
KCCLAMP (Controls [0].verticalThrustTime, nMaxTurnRate);
KCCLAMP (Controls [0].sidewaysThrustTime, nMaxTurnRate);
KCCLAMP (Controls [0].forwardThrustTime, nMaxTurnRate);
if (ControlsLimitTurnRate (bUseMouse)) {
	if (Controls [1].headingTime || Controls [2].headingTime) {
		KCCLAMP (Controls [0].headingTime, nMaxTurnRate);
		}
	if (Controls [1].pitchTime || Controls [2].pitchTime) {
		KCCLAMP (Controls [0].pitchTime, nMaxTurnRate / FASTPITCH);
		}
	if (Controls [1].bankTime || Controls [2].bankTime) {
		KCCLAMP (Controls [0].bankTime, nMaxTurnRate);
		}
	}
else {
	KCCLAMP (Controls [0].headingTime, nMaxTurnRate);
	KCCLAMP (Controls [0].pitchTime, nMaxTurnRate / FASTPITCH);
	KCCLAMP (Controls [0].bankTime, nMaxTurnRate);
	}
if (gameStates.render.nZoomFactor > F1_0) {
		int r = (gameStates.render.nZoomFactor * 100) / F1_0;

	Controls [0].headingTime = (Controls [0].headingTime * 100) / r;
	Controls [0].pitchTime = (Controls [0].pitchTime * 100) / r;
	Controls [0].bankTime = (Controls [0].bankTime * 100) / r;
#if DBG
	r = (int) ((double) gameStates.render.nZoomFactor * 100.0 / (double) F1_0);
	HUDMessage (0, "x %d.%02d", r / 100, r % 100);
#endif
	}
//	KCCLAMP (Controls [0].afterburnerTime, gameStates.input.kcPollTime);
#if DBG
if (gameStates.input.keys.pressed [KEY_DELETE])
	memset (&Controls, 0, sizeof (tControlInfo));
#endif
gameStates.input.bKeepSlackTime = 0;
kcFrameCount = 0;
gameStates.input.kcPollTime = 0;
return 0;
}

//------------------------------------------------------------------------------

void ResetCruise (void)
{
gameStates.input.nCruiseSpeed=0;
}

//------------------------------------------------------------------------------

void CybermouseAdjust ()
 {
/*	if (gameData.multiplayer.nLocalPlayer > -1)	{
		OBJECTS [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_TURNROLL);	// Turn off roll when turning
		OBJECTS [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_LEVELLING);	// Turn off leveling to nearest tSide.
		gameOpts->gameplay.nAutoLeveling = 0;

		if (kc_externalVersion > 0) {	
			CFixMatrix tempm, ViewMatrix;
			CAngleVector * Kconfig_abs_movement;
			char * oem_message;

			Kconfig_abs_movement = reinterpret_cast<CAngleVector*> (((uint)kc_external_control + sizeof (tControlInfo));

			if (Kconfig_abs_movement->p || Kconfig_abs_movement->b || Kconfig_abs_movement->h)	{
				VmAngles2Matrix (&tempm,Kconfig_abs_movement);
				VmMatMul (&ViewMatrix,&OBJECTS [LOCALPLAYER.nObject].info.position.mOrient,&tempm);
				OBJECTS [LOCALPLAYER.nObject].info.position.mOrient = ViewMatrix;	
			}
			oem_message = reinterpret_cast<char*> (((uint)Kconfig_abs_movement + sizeof (CAngleVector));
			if (oem_message [0] != '\0')
				HUDInitMessage (oem_message);
		}
	}*/

	Controls [0].pitchTime += FixMul (kc_external_control->pitchTime,gameData.time.xFrame);					
	Controls [0].verticalThrustTime += FixMul (kc_external_control->verticalThrustTime,gameData.time.xFrame);
	Controls [0].headingTime += FixMul (kc_external_control->headingTime,gameData.time.xFrame);
	Controls [0].sidewaysThrustTime += FixMul (kc_external_control->sidewaysThrustTime ,gameData.time.xFrame);
	Controls [0].bankTime += FixMul (kc_external_control->bankTime ,gameData.time.xFrame);
	Controls [0].forwardThrustTime += FixMul (kc_external_control->forwardThrustTime ,gameData.time.xFrame);
//	Controls [0].rearViewDownCount += kc_external_control->rearViewDownCount;
//	Controls [0].rearViewDownState |= kc_external_control->rearViewDownState;
	Controls [0].firePrimaryDownCount += kc_external_control->firePrimaryDownCount;
	Controls [0].firePrimaryState |= kc_external_control->firePrimaryState;
	Controls [0].fireSecondaryState |= kc_external_control->fireSecondaryState;
	Controls [0].fireSecondaryDownCount += kc_external_control->fireSecondaryDownCount;
	Controls [0].fireFlareDownCount += kc_external_control->fireFlareDownCount;
	Controls [0].dropBombDownCount += kc_external_control->dropBombDownCount;
//	Controls [0].automapDownCount += kc_external_control->automapDownCount;
// 	Controls [0].automapState |= kc_external_control->automapState;
  } 

//------------------------------------------------------------------------------

char GetKeyValue (char key)
  {
#if TRACE
	con_printf (CONDBG,"Returning %c!\n",kcKeyboard [ (int)key].value);
#endif
	return (kcKeyboard [ (int)key].value);
  }

//------------------------------------------------------------------------------

void FlushInput (void)
{
	int	b = gameOpts->legacy.bInput;

gameOpts->legacy.bInput = 1;
event_poll (SDL_ALLEVENTS);
gameOpts->legacy.bInput = b;
KeyFlush ();
MouseFlush ();
JoyFlush ();
}

//------------------------------------------------------------------------------
//eof
