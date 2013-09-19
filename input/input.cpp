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
#include "descent.h"
#include "key.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "event.h"
#include "joy.h"
#include "mouse.h"
#include "kconfig.h"
#include "cockpit.h"
#include "rendermine.h"
#include "endlevel.h"
#include "timer.h"
#include "text.h"
#include "automap.h"
#include "input.h"
#include "gamecntl.h"
#include "transformation.h"

class CControlsManager controls;

#ifdef USE_LINUX_JOY
#include "joystick.h"
#endif
#ifdef _WIN32
tTransRotInfo	tirInfo;
#endif

//tControlInfo m_info [4];

// *must* be defined - set to 0 if no limit
#define MIN_TIME_360	3.0f	//time for a 360 degree turn in secs
//#define m_maxTurnRate		(m_pollTime / m_frameCount)

#define	KCCLAMP(_val,_max) \
			if ((_val) < -(_max)) \
				(_val) = (fix) -(_max); \
			else if ((_val) > (_max)) \
				(_val) = (fix) (_max)

//------------------------------------------------------------------------------

static inline int FastPitch (void)
{
if (!gameStates.app.bHaveExtraGameInfo [IsMultiGame])
	return 2;
if ((extraGameInfo [IsMultiGame].bFastPitch < 1) || ( extraGameInfo [IsMultiGame].bFastPitch > 2))
	return 2;
return extraGameInfo [IsMultiGame].bFastPitch;
}

//------------------------------------------------------------------------------

#define	PH_SCALE	1

#define	JOYSTICK_READ_TIME	 (I2X (1)/40)		//	Read joystick at 40 Hz.

fix	LastReadTime = 0;
fix	joyAxis [JOY_MAX_AXES];

//------------------------------------------------------------------------------

fix Next_toggleTime [3]={0,0,0};

int CControlsManager::AllowToToggle (int i)
{
//used for keeping tabs of when its ok to toggle headlight,primary,and secondary
if (Next_toggleTime [i] > gameData.time.xGame)
	if (Next_toggleTime [i] < gameData.time.xGame + (I2X (1)/8))	//	In case time is bogus, never wait > 1 second.
		return 0;
Next_toggleTime [i] = gameData.time.xGame + (I2X (1)/8);
return 1;
}


#ifdef D2X_KEYS
//added on 2/7/99 by Victor Rachels for jostick state setting
int d2xJoystick_ostate [20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//end this section adition - VR
#endif

//------------------------------------------------------------------------------

int CControlsManager::ReadKeyboard (void)
{
return 1;
}

//------------------------------------------------------------------------------

int CControlsManager::ReadMouse (int *mouseAxis, int *nMouseButtons)
{
	int	dx, dy;
	int	dz;

MouseGetDeltaZ (&dx, &dy, &dz);
mouseAxis [0] = (int) ((dx * m_pollTime) / 35);
mouseAxis [1] = (int) ((dy * m_pollTime) / 25);
mouseAxis [2] = (int) (dz * m_pollTime);
*nMouseButtons = MouseGetButtons ();
return 1;
}

//------------------------------------------------------------------------------

static int joy_sens_mod [4] = {16, 16, 16, 16};

static double dMaxAxis = 127.0;

int CControlsManager::AttenuateAxis (double a, int nAxis)
{
if (gameOpts->input.joystick.bLinearSens)
	return (int) a;
else if (!a)
	return 0;
else {
		double	d;
#if 0//DBG
		double	p, h, e;
#endif

	if (a > dMaxAxis)
		a = dMaxAxis;
	else if (a < -dMaxAxis)
		a = -dMaxAxis;
#if 1//!DBG
	d = dMaxAxis * pow (fabs ((double) a / dMaxAxis), (double) joy_sens_mod [nAxis % 4] / 16.0);
#else
	h = fabs ((double) vec / dMaxAxis);
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

int CControlsManager::ReadJoyAxis (int i, int rawJoyAxis [])
{
int dz = joyDeadzone [i % UNIQUE_JOY_AXES]; // / 128;
int h = rawJoyAxis [i]; // JoyGetScaledReading (rawJoyAxis [i], i);
if (gameOpts->input.joystick.bSyncAxis && (dz < 16384) && ((kcJoystick [18].value == i) || (kcJoystick [48].value == i)))		// If this is the throttle
	dz *= 2;				// Then use a larger dead-zone
if (h > dz)
	h = int ((h - dz) * 32767.0f / float (32767 - dz) + 0.5f);
else if (h < -dz)
	h = int ((h + dz) * 32767.0f / float (32767 - dz) + 0.5f);
else
	h = 0;
return (int) ((AttenuateAxis (h / 256, i) * m_pollTime) / 128);
}

//------------------------------------------------------------------------------

int CControlsManager::ReadJoystick (int * joyAxis)
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
		bUseJoystick = 1;
		}
	else if (gameOpts->input.joystick.bUse) {
		LastReadTime = ctime;
		if	((channelMasks = JoyReadRawAxis (JOY_ALL_AXIS, rawJoyAxis))) {
			for (i = 0; i < JOY_MAX_AXES; i++) {
				if ((i == 3) && (gameStates.input.nJoyType == CONTROL_THRUSTMASTER_FCS))
					ReadFCS (rawJoyAxis [i]);
				else
					joyAxis [i] = ReadJoyAxis (i, rawJoyAxis);
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
			for (i = 0; i < JOY_MAX_AXES; i++) {
				if (channelMasks & (1 << i))
					joyAxis [i] = ReadJoyAxis (i, rawJoyAxis);
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

void CControlsManager::SetFCSButton (int btn, int button)
{
JoyGetButtonState (btn);
}

//------------------------------------------------------------------------------

void CControlsManager::ReadFCS (int rawAxis)
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
SetFCSButton (19, button);
SetFCSButton (15, button);
SetFCSButton (11, button);
SetFCSButton (7, button);
}

//------------------------------------------------------------------------------

int CControlsManager::ReadCyberman (int *mouseAxis, int *nMouseButtons)
{
	int idx, idy;

MouseGetCybermanPos (&idx, &idy);
mouseAxis [0] = (int) ((idx * m_pollTime) / 128);
mouseAxis [1] = (int) ((idy * m_pollTime) / 128);
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
if ((v = KeyDownCount (v)))
	return v;
return 0;
#else
return KeyDownCount (v);
#endif
}

//------------------------------------------------------------------------------

void CControlsManager::DoD2XKeys (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank)
{
#ifdef D2X_KEYS
//added on 2/4/99 by Victor Rachels for d1x keys
//--------- Read primary weapon select -------------
//the following "if" added by WraithX to stop deadies from switchin weapons, 4/14/00
if (!(gameStates.app.bPlayerIsDead || automap.Display ())) { {
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

int CControlsManager::LimitTurnRate (int bUseMouse)
{
if (!(gameOpts->input.bLimitTurnRate || IsMultiGame))
	return 0;
if (automap.Display () ||
	 gameOpts->input.mouse.bJoystick ||
	 gameStates.app.bNostalgia ||
	 COMPETITION ||
	 !(bUseMouse && EGI_FLAG (bMouseLook, 0, 1, 0))) {
	KCCLAMP (m_info [0].pitchTime, m_maxTurnRate / FastPitch ());
	KCCLAMP (m_info [0].headingTime, m_maxTurnRate);
	}
KCCLAMP (m_info [0].bankTime, m_frameTime);
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

void CControlsManager::DoKeyboard (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank)
{
	int	i, v, pitchScale = (!(gameStates.app.bNostalgia || COMPETITION) &&
									 (FastPitch () == 1)) ? 2 * PH_SCALE : 1;
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
			m_info [0].firePrimaryState |= gameStates.input.keys.pressed [v];
			m_info [0].firePrimaryDownCount += KeyDownCount (v);
			}
		if ((v = HaveKey (kcKeyboard, 26 + i)) < 255) {
			m_info [0].fireSecondaryState |= gameStates.input.keys.pressed [v];
			m_info [0].fireSecondaryDownCount += KeyDownCount (v);
			}
		if ((v = HaveKey (kcKeyboard, 28 + i)) < 255)
			m_info [0].fireFlareDownCount += KeyDownCount (v);
		}

	for (i = 0; i < 2; i++) {
		m_info [0].sidewaysThrustTime -= DELTACTRL (10 + i, 2);
		m_info [0].sidewaysThrustTime += DELTACTRL (12 + i, 2);
		m_info [0].verticalThrustTime += DELTACTRL (14 + i, 2);
		m_info [0].verticalThrustTime -= DELTACTRL (16 + i, 2);
		m_info [1].bankTime += DELTACTRL (20 + i, 1);
		m_info [1].bankTime -= DELTACTRL (22 + i, 1);
		m_info [0].forwardThrustTime += DELTACTRL (30 + i, 0);
		m_info [0].forwardThrustTime -= DELTACTRL (32 + i, 0);
#if DBG
		if (m_info [0].forwardThrustTime)
			m_info [0].forwardThrustTime = m_info [0].forwardThrustTime;
#endif
		if ((v = HaveKey (kcKeyboard, 46 + i)) < 255)
			m_info [0].afterburnerState |= gameStates.input.keys.pressed [v];
		// count bomb drops
		if ((v = HaveKey (kcKeyboard, 34 + i)) < 255)
			m_info [0].dropBombDownCount += KeyDownCount (v);
		// charge chield
		if (LOCALPLAYER.flags & PLAYER_FLAGS_CONVERTER) {
			if (gameStates.input.keys.pressed [v = HaveKey (kcKeyboard, 56 + i)])
				TransferEnergyToShield (KeyDownTime (v));
			}
		// rear view
		if ((v = HaveKey (kcKeyboard, 36 + i)) < 255) {
			m_info [0].rearViewDownCount += KeyDownCount (v);
			m_info [0].rearViewDownState |= gameStates.input.keys.pressed [v];
			}
		// automap
		if ((v = HaveKey (kcKeyboard, 44 + i)) < 255) {
			m_info [0].automapDownCount += KeyDownCount (v);
			m_info [0].automapState |= gameStates.input.keys.pressed [v];
			}
	}
	// headlight and weapon cycling
	if ((v = HaveKey (kcKeyboard, 54)) < 255)
		m_info [0].headlightCount = KeyDownCount (v);
	m_info [0].headlightCount += HaveKeyCount (kcKeyboard, 55);
	m_info [0].cyclePrimaryCount = HaveKeyCount (kcKeyboard, 48);
	m_info [0].cyclePrimaryCount += HaveKeyCount (kcKeyboard, 49);
	m_info [0].cycleSecondaryCount += HaveKeyCount (kcKeyboard, 50);
	m_info [0].cycleSecondaryCount += HaveKeyCount (kcKeyboard, 51);
	m_info [0].zoomDownCount += HaveKeyCount (kcKeyboard, 52);
	m_info [0].zoomDownCount += HaveKeyCount (kcKeyboard, 53);
	m_info [0].toggleIconsCount += HaveKeyCount (kcKeyboard, 59);
	m_info [0].useCloakDownCount += HaveKeyCount (kcKeyboard, 60);
	m_info [0].useCloakDownCount += HaveKeyCount (kcKeyboard, 61);
	m_info [0].useInvulDownCount += HaveKeyCount (kcKeyboard, 62);
	m_info [0].useInvulDownCount += HaveKeyCount (kcKeyboard, 63);
	m_info [0].slowMotionCount += HaveKeyCount (kcKeyboard, 64);
	m_info [0].bulletTimeCount += HaveKeyCount (kcKeyboard, 65);

	// toggle bomb
	if (((v = HaveKey (kcKeyboard, 58)) < 255) && KeyDownCount (v))
		ToggleBomb ();

	// cruise speed
	for (i = 0; i < 4; i++)
		if ((v = HaveKey (kcKeyboard, 38 + i)) < 255)
			*nCruiseSpeed += key_signs [i] * FixDiv (speedFactor * KeyDownTime (v) * 5, m_pollTime);
	for (i = 0; i < 2; i++)
		if (((v = HaveKey (kcKeyboard,
 + i)) < 255) && KeyDownCount (v))
			*nCruiseSpeed = 0;
	}

// special slide & bank toggle handling
if (*bSlideOn) {
	if (bGetSlideBank == 2) {
		for (i = 0; i < 2; i++) {
			m_info [0].verticalThrustTime += DELTACTRL (i, 2);
			m_info [0].verticalThrustTime -= DELTACTRL (2 + i, 2);
			m_info [0].sidewaysThrustTime -= DELTACTRL (4 + i, 2);
			m_info [0].sidewaysThrustTime += DELTACTRL (6 + i, 2);
			}
		}
	}
else if (bGetSlideBank == 1) {
	for (i = 0; i < 4; i++)
		m_info [1].pitchTime += key_signs [i] * DELTACTRL (i, 1) / pitchScale;
	if (!*bBankOn)
		for (i = 4; i < 8; i++)
			m_info [1].headingTime += key_signs [i] * DELTACTRL (i, 1) / PH_SCALE;
	}
if (*bBankOn) {
	if (bGetSlideBank == 2) {
		for (i = 4; i < 6; i++)
			m_info [1].bankTime += DELTACTRL (i, 1);
		for (i = 6; i < 8; i++)
			m_info [1].bankTime -= DELTACTRL (i, 1);
		}
	}
if (bGetSlideBank == 2)
	LimitTurnRate (0);
//KCCLAMP (pitchTime, m_maxTurnRate / FastPitch ());
*pitchTimeP = m_info [1].pitchTime;
*headingTimeP = m_info [1].headingTime;
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
#if 0 //DBG
int vec = gameOpts->input.joystick.bLinearSens ? joyAxis [dir] * 16 / joy_sens_mod [dir % 4] : joyAxis [dir];
if (vec)
	HUDMessage (0, "%d", vec);
return vec;
#else
return gameOpts->input.joystick.bLinearSens ? joyAxis [v] * 16 / joy_sens_mod [v % 4] : joyAxis [v];
#endif
}

//------------------------------------------------------------------------------

void CControlsManager::DoJoystick (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank)
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
			m_info [0].firePrimaryState |= JoyGetButtonState (v);
			m_info [0].firePrimaryDownCount += JoyGetButtonDownCnt (v);
			}
		if ((v = kcJoystick [i + 1].value) < 255) {
			m_info [0].fireSecondaryState |= JoyGetButtonState (v);
			m_info [0].fireSecondaryDownCount += JoyGetButtonDownCnt (v);
			}
		if ((v = kcJoystick [i + 2].value) < 255)
			m_info [0].forwardThrustTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 3].value) < 255)
			m_info [0].forwardThrustTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 4].value) < 255)
			m_info [0].fireFlareDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 6].value) < 255)
			m_info [0].sidewaysThrustTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 7].value) < 255)
			m_info [0].sidewaysThrustTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 8].value) < 255)
			m_info [0].verticalThrustTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 9].value) < 255)
			m_info [0].verticalThrustTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 11].value) < 255)
			m_info [2].bankTime += JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 12].value) < 255)
			m_info [2].bankTime -= JoyGetButtonDownTime (v);
		if ((v = kcJoystick [i + 19].value) < 255) {
			m_info [0].rearViewDownCount += JoyGetButtonDownCnt (v);
			m_info [0].rearViewDownState |= JoyGetButtonState (v);
			}
		if ((v = kcJoystick [i + 20].value) < 255)
			m_info [0].dropBombDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 21].value) < 255)
			m_info [0].afterburnerState |= JoyGetButtonState (v);
		if ((v = kcJoystick [i + 22].value) < 255)
			m_info [0].cyclePrimaryCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 23].value) < 255)
			m_info [0].cycleSecondaryCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 24].value) < 255)
			m_info [0].headlightCount += JoyGetButtonDownCnt (v);
		if (((v = kcJoystick [i + 25].value) < 255) && JoyGetButtonDownCnt (v))
			ToggleBomb ();
		if ((v = kcJoystick [i + 26].value) < 255)
			m_info [0].toggleIconsCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 27].value) < 255)
			m_info [0].automapDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 28].value) < 255)
			m_info [0].useCloakDownCount += JoyGetButtonDownCnt (v);
		if ((v = kcJoystick [i + 29].value) < 255)
			m_info [0].useInvulDownCount += JoyGetButtonDownCnt (v);

		// Axis movements
		if ((v = kcJoystick [i + 15].value) < 255) {
			if (kcJoystick [62].value)		// If inverted...
				m_info [0].sidewaysThrustTime += joyAxis [v];
			else
				m_info [0].sidewaysThrustTime -= joyAxis [v];
			}
		if ((v = kcJoystick [i + 16].value) < 255) {
			if (kcJoystick [63].value)		// If inverted...
				m_info [0].verticalThrustTime -= joyAxis [v];
			else
				m_info [0].verticalThrustTime += joyAxis [v];
			}
		if ((v = kcJoystick [i + 17].value) < 255) {
			if (kcJoystick [64].value)		// If inverted...
				m_info [2].bankTime += joyAxis [v];
			else
				m_info [2].bankTime -= joyAxis [v];
			}
		if ((v = kcJoystick [i + 18].value) < 255) {
			if (kcJoystick [65].value)		// If inverted...
				m_info [0].forwardThrustTime += joyAxis [v];
			else
				m_info [0].forwardThrustTime -= joyAxis [v];
			}
		// special continuous slide & bank handling
		if (*bSlideOn) {
			if ((v = kcJoystick [i + 13].value) < 255) {
				if (kcJoystick [60].value)		// If inverted...
					m_info [0].verticalThrustTime -= joyAxis [v];
				else
					m_info [0].verticalThrustTime += joyAxis [v];
			}
			if ((v = kcJoystick [i + 14].value) < 255) {
				if (kcJoystick [61].value)		// If inverted...
					m_info [0].sidewaysThrustTime -= joyAxis [v];
				else
					m_info [0].sidewaysThrustTime += joyAxis [v];
				}
			}
		else {
			if ((v = kcJoystick [i + 13].value) < 255) {
				if (kcJoystick [60].value)		// If inverted...
					m_info [2].pitchTime += DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				else
					m_info [2].pitchTime -= DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				}
			if (!*bBankOn) {
				if ((v = kcJoystick [i + 14].value) < 255) {
					if (kcJoystick [61].value)		// If inverted...
						m_info [2].headingTime -= DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod); //m_frameCount;
					else
						m_info [2].headingTime += DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod); // m_frameCount;
					}
				}
			}
		if (*bBankOn) {
			if ((v = kcJoystick [i + 14].value) < 255) {
				if (kcJoystick [61].value)		// If inverted...
					m_info [2].bankTime += DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				else
					m_info [2].bankTime -= DeltaAxis (v); // (joyAxis [v] * 16 / joy_sens_mod);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

int CControlsManager::CalcDeadzone (int d, int nDeadzone)
{
double	r = 32 * nDeadzone;
return (int) (r ? (d ? sqrt (fabs (r * r - d * d)) : r) : 0);
}

//------------------------------------------------------------------------------

void CControlsManager::DoMouse (int *mouseAxis, int nMouseButtons,
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
		m_info [0].firePrimaryState |= MouseButtonState (v);
		m_info [0].firePrimaryDownCount += MouseButtonDownCount (v);
		}
	if ((v = kcMouse [1].value) < 255) {
		m_info [0].fireSecondaryState |= MouseButtonState (v);
		m_info [0].fireSecondaryDownCount += MouseButtonDownCount (v);
		}
	if ((v = kcMouse [2].value) < 255)
		m_info [0].forwardThrustTime += MouseButtonDownTime (v);
	if ((v = kcMouse [3].value) < 255)
		m_info [0].forwardThrustTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [4].value) < 255)
		m_info [0].fireFlareDownCount += MouseButtonDownCount (v);
	if ((v = kcMouse [6].value) < 255)
		m_info [0].sidewaysThrustTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [7].value) < 255)
		m_info [0].sidewaysThrustTime += MouseButtonDownTime (v);
	if ((v = kcMouse [8].value) < 255)
		m_info [0].verticalThrustTime += MouseButtonDownTime (v);
	if ((v = kcMouse [9].value) < 255)
		m_info [0].verticalThrustTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [11].value) < 255)
		m_info [3].bankTime += MouseButtonDownTime (v);
	if ((v = kcMouse [12].value) < 255)
		m_info [3].bankTime -= MouseButtonDownTime (v);
	if ((v = kcMouse [25].value) < 255) {
		m_info [0].rearViewDownCount += MouseButtonDownCount (v);
		m_info [0].rearViewDownState |= MouseButtonState (v);
		}
	if ((v = kcMouse [26].value) < 255)
		m_info [0].dropBombDownCount += MouseButtonDownCount (v);
	if ((v = kcMouse [27].value) < 255)
		m_info [0].afterburnerState |= MouseButtonState (v);
	if (((v = kcMouse [28].value) < 255) && MouseButtonState (v))
		m_info [0].cyclePrimaryCount += MouseButtonDownCount (v);
	if (((v = kcMouse [29].value) < 255) && MouseButtonState (v))
		m_info [0].cycleSecondaryCount += MouseButtonDownCount (v);
	if (((v = kcMouse [30].value) < 255) && MouseButtonState (v))
		m_info [0].zoomDownCount += MouseButtonDownCount (v);
	// Axis movements
	if ((v = kcMouse [17].value) < 255) {
		if (kcMouse [18].value)		// If inverted...
			m_info [0].sidewaysThrustTime -= mouseAxis [v];
		else
			m_info [0].sidewaysThrustTime += mouseAxis [v];
		}
	if ((v = kcMouse [19].value) < 255) {
		if (kcMouse [20].value)		// If inverted...
			m_info [0].verticalThrustTime -= mouseAxis [v];
		else
			m_info [0].verticalThrustTime += mouseAxis [v];
		}
	if (((v = kcMouse [21].value) < 255) && mouseAxis [v]) {
		if (kcMouse [22].value)		// If inverted...
			m_info [3].bankTime -= mouseAxis [v];
		else
			m_info [3].bankTime += mouseAxis [v];
		}
	if ((v = kcMouse [23].value) < 255) {
		if (kcMouse [24].value)		// If inverted...
			m_info [0].forwardThrustTime += mouseAxis [v];
		else
			m_info [0].forwardThrustTime -= mouseAxis [v];
		}
	// special continuous slide & bank handling
	if (*bSlideOn) {
		if ((v = kcMouse [13].value) < 255) {
			if (kcMouse [14].value)		// If inverted...
				m_info [0].verticalThrustTime += mouseAxis [v];
			else
				m_info [0].verticalThrustTime -= mouseAxis [v];
			}
		if ((v = kcMouse [15].value) < 255) {
			if (kcMouse [16].value)		// If inverted...
				m_info [0].sidewaysThrustTime -= mouseAxis [v];
			else
				m_info [0].sidewaysThrustTime += mouseAxis [v];
			}
		}
	else {
		SDL_GetMouseState (&mouseData.x, &mouseData.y);
		if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick) {
			int dx = mouseData.x - screen.Width () / 2;
			int dz = CalcDeadzone (0, gameOpts->input.mouse.nDeadzone);
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
			dx = 640 * dx / (screen.Width () / gameOpts->input.mouse.sensitivity [0]);
			m_info [3].headingTime += dx; // * gameOpts->input.mouse.sensitivity [0]); // nMouseSensMod;
			}
		else {
			if (((v = kcMouse [13].value) < 255) && mouseAxis [v]) {
				if (kcMouse [14].value)		// If inverted...
					m_info [3].pitchTime += (mouseAxis [v] * gameOpts->input.mouse.sensitivity [1]) / nMouseSensMod;
				else
					m_info [3].pitchTime -= (mouseAxis [v] * gameOpts->input.mouse.sensitivity [1]) / nMouseSensMod;
				}
			}
		if (*bBankOn) {
			if (((v = kcMouse [15].value) < 255) && mouseAxis [v]) {
				if (kcMouse [16].value)		// If inverted...
					m_info [3].bankTime -= (mouseAxis [v] * gameOpts->input.mouse.sensitivity [2]) / nMouseSensMod;
				else
					m_info [3].bankTime += (mouseAxis [v] * gameOpts->input.mouse.sensitivity [2]) / nMouseSensMod;
				}
			}
		else {
			if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick) {
				int	dy = mouseData.y - screen.Height () / 2;
				int	dz = CalcDeadzone (0, gameOpts->input.mouse.nDeadzone);
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
				dy = 480 * dy / (screen.Height () / gameOpts->input.mouse.sensitivity [1]);
				m_info [3].pitchTime += dy; // * gameOpts->input.mouse.sensitivity [1]); // nMouseSensMod;
				}
			else {
				if (((v = kcMouse [15].value) < 255) && mouseAxis [v]) {
					if (kcMouse [16].value)		// If inverted...
						m_info [3].headingTime -= (mouseAxis [v] * gameOpts->input.mouse.sensitivity [0]) / nMouseSensMod;
					else
						m_info [3].headingTime += (mouseAxis [v] * gameOpts->input.mouse.sensitivity [0]) / nMouseSensMod;
					}
				}
			}
		}
	}

if (gameStates.input.nMouseType == CONTROL_CYBERMAN) {
	if (bGetSlideBank == 2) {
		m_info [0].verticalThrustTime += MouseButtonDownTime (D2_MB_Z_UP) / 2;
		m_info [0].verticalThrustTime -= MouseButtonDownTime (D2_MB_Z_DOWN) / 2;
		m_info [3].bankTime += MouseButtonDownTime (D2_MB_BANK_LEFT);
		m_info [3].bankTime -= MouseButtonDownTime (D2_MB_BANK_RIGHT);
		}
	if (*bSlideOn) {
		if (bGetSlideBank == 2) {
			m_info [0].verticalThrustTime -= MouseButtonDownTime (D2_MB_PITCH_FORWARD);
			m_info [0].verticalThrustTime += MouseButtonDownTime (D2_MB_PITCH_BACKWARD);
			m_info [0].sidewaysThrustTime -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			m_info [0].sidewaysThrustTime += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
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
			m_info [3].bankTime -= MouseButtonDownTime (D2_MB_HEAD_LEFT);
			m_info [3].bankTime += MouseButtonDownTime (D2_MB_HEAD_RIGHT);
			}
		}
	}
}

//------------------------------------------------------------------------------

int CControlsManager::ReadOculusRift (void)
{
return transformation.m_info.bUsePlayerHeadAngles = gameData.render.rift.GetHeadAngles (transformation.m_info.playerHeadAngles);
}

//------------------------------------------------------------------------------

void CControlsManager::DoOculusRift (void)
{
}

//------------------------------------------------------------------------------

#ifdef _WIN32

int CControlsManager::ReadTrackIR (void)
{
transformation.m_info.bUsePlayerHeadAngles = 0;
if (!(gameStates.input.bHaveTrackIR && gameOpts->input.trackIR.bUse))
	return 0;
if (!pfnTIRQuery (&tirInfo)) {
	pfnTIRExit ();
	if ((gameStates.input.bHaveTrackIR = pfnTIRInit ((HWND) SDL_GetWindowHandle ())))
		pfnTIRStart ();
	return 0;
	}
#if 0//DBG
HUDMessage (0, "%1.0f %1.0f %1.0f", tirInfo.fvTrans.x, tirInfo.fvTrans.y, tirInfo.fvTrans.z);
#endif
return 1;
}

//------------------------------------------------------------------------------

void CControlsManager::DoTrackIR (void)
{
	int	dx = (int) ((float) tirInfo.fvRot.z * (float) screen.Width () / 16384.0f),
			dy = (int) ((float) tirInfo.fvRot.y * (float) screen.Height () / 16384.0f),
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
#if 0//DBG
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
		m_info [0].headingTime -= (fix) (dx * m_pollTime);
		m_info [0].pitchTime += (fix) (dy * m_pollTime);
		}
	if (gameOpts->input.trackIR.bMove [1])
		m_info [0].bankTime += (int) (tirInfo.fvRot.x * m_pollTime / 131072.0f * (gameOpts->input.trackIR.sensitivity [2] + 1));
	}
else if (gameOpts->input.trackIR.nMode == 1) {
	dx = (int) ((float) tirInfo.fvRot.z * (float) screen.Width () / 16384.0f);
	dy = (int) ((float) tirInfo.fvRot.y * (float) screen.Height () / 16384.0f);
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
#if 0//DBG
	HUDMessage (0, "%d %d", dx, dy);
#endif
	dx = 640 * dx / (screen.Width () / (gameOpts->input.trackIR.sensitivity [0] + 1));
	dy = 480 * dy / (screen.Height () / (gameOpts->input.trackIR.sensitivity [1] + 1));
	if (gameOpts->input.trackIR.bMove [0]) {
		m_info [0].headingTime -= dx;
		m_info [0].pitchTime += dy;
		}
	if (gameOpts->input.trackIR.bMove [1])
		m_info [0].bankTime += (int) (tirInfo.fvRot.x * m_pollTime / 131072.0f * (gameOpts->input.trackIR.sensitivity [2] + 1));
	}
else {
	transformation.m_info.bUsePlayerHeadAngles = 1;
	if (gameOpts->input.trackIR.bMove [0]) {
		transformation.m_info.playerHeadAngles.v.coord.h = fixang (-tirInfo.fvRot.z / 4 * (gameOpts->input.trackIR.sensitivity [0] + 1));
		transformation.m_info.playerHeadAngles.v.coord.p = fixang (tirInfo.fvRot.y / 4 * (gameOpts->input.trackIR.sensitivity [1] + 1));
		}
	else
		transformation.m_info.playerHeadAngles.v.coord.h =
		transformation.m_info.playerHeadAngles.v.coord.p = 0;
	if (gameOpts->input.trackIR.bMove [1])
		transformation.m_info.playerHeadAngles.v.coord.b = (fixang) tirInfo.fvRot.x / 4 * (gameOpts->input.trackIR.sensitivity [2] + 1);
	else
		transformation.m_info.playerHeadAngles.v.coord.b = 0;
	}
fDeadzone = 256.0f * gameOpts->input.trackIR.nDeadzone;
fScale = 16384.0f / (16384.0f - fDeadzone);
if (gameOpts->input.trackIR.bMove [2] && (float (fabs (tirInfo.fvTrans.x) > fDeadzone))) {
	if (tirInfo.fvTrans.x < 0)
		tirInfo.fvTrans.x += fDeadzone;
	else
		tirInfo.fvTrans.x -= fDeadzone;
	m_info [0].sidewaysThrustTime -= int ((tirInfo.fvTrans.x - fDeadzone) * m_pollTime / 65536.0f * (gameOpts->input.trackIR.sensitivity [0] + 1));
	}
if (gameOpts->input.trackIR.bMove [3] && (float (fabs (tirInfo.fvTrans.y) > fDeadzone))) {
	if (tirInfo.fvTrans.y < 0)
		tirInfo.fvTrans.y += fDeadzone;
	else
		tirInfo.fvTrans.y -= fDeadzone;
	m_info [0].verticalThrustTime += (int) ((tirInfo.fvTrans.y - fDeadzone) * m_pollTime / 65536.0f * (gameOpts->input.trackIR.sensitivity [1] + 1));
	}
if (gameOpts->input.trackIR.bMove [4] && ((float) fabs (tirInfo.fvTrans.z) > fDeadzone)) {
	if (tirInfo.fvTrans.z < 0)
		tirInfo.fvTrans.z += fDeadzone;
	else
		tirInfo.fvTrans.z -= fDeadzone;
	m_info [0].forwardThrustTime -= (int) ((tirInfo.fvTrans.z - fDeadzone) * m_pollTime / 8192.0f * (gameOpts->input.trackIR.sensitivity [2] + 1));
	}
}

#endif

//------------------------------------------------------------------------------

void CControlsManager::DoSlideBank (int bSlideOn, int bBankOn, fix pitchTime, fix headingTime)
{
if (bSlideOn)
	m_info [0].headingTime =
	m_info [0].pitchTime = 0;
else {
	if (pitchTime == 0)
		m_info [0].pitchTime = 0;
	else {
		if (pitchTime > 0) {
			if (m_info [0].pitchTime < 0)
				m_info [0].pitchTime = 0;
			}
		else {// pitchTime < 0
			if (m_info [0].pitchTime > 0)
				m_info [0].pitchTime = 0;
			}
		m_info [0].pitchTime += pitchTime;
		}
	if (!bBankOn) {
		if (headingTime == 0)
			m_info [0].headingTime = 0;
		else {
			if (headingTime > 0) {
				if (m_info [0].headingTime < 0)
					m_info [0].headingTime = 0;
				}
			else {
				if (m_info [0].headingTime > 0)
					m_info [0].headingTime = 0;
				}
			m_info [0].headingTime += headingTime;
			}
		}
	}
}

//------------------------------------------------------------------------------

int CControlsManager::CapSampleRate (void)
{
#if PHYSICS_FPS >= 0
	float t = float (SDL_GetTicks ());

if (t - m_lastTick < 1000.0f / 30.0f)
	return 1;
m_pollTime = time_t (gameData.physics.xTime);
m_frameTime = float (gameData.physics.xTime);
m_slackTurnRate += m_frameTime;
m_maxTurnRate = int (m_slackTurnRate);
m_slackTurnRate -= float (m_maxTurnRate);
m_lastTick = t;
return 0;
#else
if (gameData.app.bGamePaused)
	GetSlowTicks ();
m_frameCount++;
m_pollTime += gameData.time.xFrame;
if (!gameStates.app.tick40fps.bTick)
	return 1;
m_frameTime = float (m_pollTime) / m_frameCount;
m_maxTurnRate = int (m_frameTime);
#endif
return 0;

}

//------------------------------------------------------------------------------

void CControlsManager::Reset (void)
{
//fix ht = m_info [0].headingTime;
//fix pt = m_info [0].pitchTime;
memset (&m_info, 0, sizeof (m_info));
//m_info [0].headingTime = ht;
//m_info [0].pitchTime = pt;
LimitTurnRate (gameOpts->input.mouse.bUse && !gameStates.input.bCybermouseActive);
}

//------------------------------------------------------------------------------

void CControlsManager::SetType (void)
{
if (gameStates.input.nMouseType < 0)
	gameStates.input.nMouseType = (gameConfig.nControlType == CONTROL_CYBERMAN) ? CONTROL_CYBERMAN : CONTROL_MOUSE;
if (gameStates.input.nJoyType < 0)
	gameStates.input.nJoyType = ((gameConfig.nControlType >= CONTROL_FLIGHTSTICK_PRO) && (gameConfig.nControlType <= CONTROL_GRAVIS_GAMEPAD)) 
										 ? gameConfig.nControlType 
										 : CONTROL_JOYSTICK;
if (gameOpts->input.joystick.bUse)
	gameConfig.nControlType = gameStates.input.nJoyType;
else if (gameOpts->input.mouse.bUse)
	gameConfig.nControlType = gameStates.input.nMouseType;
else
	gameConfig.nControlType = CONTROL_NONE;
}

//------------------------------------------------------------------------------

void CControlsManager::ResetTriggers (void)
{
m_info [0].cyclePrimaryCount = 0;
m_info [0].cycleSecondaryCount = 0;
m_info [0].toggleIconsCount = 0;
m_info [0].zoomDownCount = 0;
m_info [0].headlightCount = 0;
m_info [0].fireFlareDownCount = 0;
m_info [0].dropBombDownCount = 0;
m_info [0].automapDownCount = 0;
m_info [0].rearViewDownCount = 0;
}

//------------------------------------------------------------------------------

int CControlsManager::Read (void)
{
	int	i;
	int	bSlideOn, bBankOn;
	fix	pitchTime, headingTime;
	fix	mouseAxis [3];
	int	nMouseButtons;
	int	bUseMouse, bUseJoystick;

ResetTriggers ();
gameStates.input.bControlsSkipFrame = 1;
if (CapSampleRate ())
	return 1;
gameStates.input.bControlsSkipFrame = 0;
Reset ();
gameStates.input.bKeepSlackTime = 1;
nMouseButtons = 0;
bUseMouse = 0;
bSlideOn = 0;
bBankOn = 0;

if (!gameOpts->legacy.bInput)
	event_poll (SDL_ALLEVENTS);	//why poll 2 dozen times in the following code when input polling calls all necessary input handlers anyway?

SetType ();
bUseJoystick = gameOpts->input.joystick.bUse && ReadJoystick (reinterpret_cast<int*> (&joyAxis [0]));
if (gameOpts->input.mouse.bUse)
	if (gameStates.input.bCybermouseActive) {
#if 0
		ReadOWL (externalControls.m_info);
		CybermouseAdjust ();
#endif
		}
	else if (gameStates.input.nMouseType == CONTROL_CYBERMAN)
		bUseMouse = ReadCyberman (reinterpret_cast<int*> (&mouseAxis [0]), &nMouseButtons);
	else
		bUseMouse = ReadMouse (reinterpret_cast<int*> (&mouseAxis [0]), &nMouseButtons);
else {
	mouseAxis [0] =
	mouseAxis [1] =
	mouseAxis [2] = 0;
	nMouseButtons = 0;
	bUseMouse = 0;
	}
pitchTime = headingTime = 0;
memset (m_info + 1, 0, sizeof (m_info) - sizeof (m_info [0]));
for (i = 0; i < 3; i++) {
	DoKeyboard (&bSlideOn, &bBankOn, &pitchTime, &headingTime, reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
	if (bUseJoystick)
		DoJoystick (&bSlideOn, &bBankOn, &pitchTime, &headingTime, reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
	if (bUseMouse)
		DoMouse (reinterpret_cast<int*> (&mouseAxis [0]), nMouseButtons, &bSlideOn, &bBankOn, &pitchTime, &headingTime,
							  reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
	m_info [0].pitchTime = m_info [1].pitchTime + m_info [2].pitchTime + m_info [3].pitchTime;
	m_info [0].headingTime = m_info [1].headingTime + m_info [2].headingTime + m_info [3].headingTime;
	m_info [0].bankTime = m_info [1].bankTime + m_info [2].bankTime + m_info [3].bankTime;
	if (i == 1) {
		DoSlideBank (bSlideOn, bBankOn, pitchTime, headingTime);
		}
	}
if (gameOpts->input.bUseHotKeys)
	DoD2XKeys (&bSlideOn, &bBankOn, &pitchTime, &headingTime, reinterpret_cast<int*> (&gameStates.input.nCruiseSpeed), i);
gameData.render.rift.AutoCalibrate ();
if (ReadOculusRift ())
	DoOculusRift ();
#ifdef _WIN32
else if (ReadTrackIR ())
	DoTrackIR ();
#endif
if (gameStates.input.nCruiseSpeed > I2X (100))
	gameStates.input.nCruiseSpeed = I2X (100);
else if (gameStates.input.nCruiseSpeed < 0)
	gameStates.input.nCruiseSpeed = 0;
if (!m_info [0].forwardThrustTime)
	m_info [0].forwardThrustTime = FixMul (gameStates.input.nCruiseSpeed, m_pollTime) / 100;

#if 0 //LIMIT_CONTROLS_FPS
if (nBankSensMod > 2) {
	m_info [0].bankTime *= 2;
	m_info [0].bankTime /= nBankSensMod;
	}
#endif

//----------- Clamp values between -m_pollTime and m_pollTime
if (m_pollTime > I2X (1)) {
#if TRACE
	console.printf (1, "Bogus frame time of %.2f seconds\n", X2F (m_pollTime));
#endif
	m_pollTime = I2X (1);
	}
KCCLAMP (m_info [0].verticalThrustTime, m_maxTurnRate);
KCCLAMP (m_info [0].sidewaysThrustTime, m_maxTurnRate);
KCCLAMP (m_info [0].forwardThrustTime, m_maxTurnRate);
if (LimitTurnRate (bUseMouse)) {
	if (m_info [1].headingTime || m_info [2].headingTime) {
		KCCLAMP (m_info [0].headingTime, m_maxTurnRate);
		}
	if (m_info [1].pitchTime || m_info [2].pitchTime) {
		KCCLAMP (m_info [0].pitchTime, m_maxTurnRate / FastPitch ());
		}
	if (m_info [1].bankTime || m_info [2].bankTime) {
		KCCLAMP (m_info [0].bankTime, m_maxTurnRate);
		}
	}
else {
	KCCLAMP (m_info [0].headingTime, m_maxTurnRate);
	KCCLAMP (m_info [0].pitchTime, m_maxTurnRate / FastPitch ());
	KCCLAMP (m_info [0].bankTime, m_maxTurnRate);
	}
if (gameStates.zoom.nFactor > I2X (1)) {
		int r = int (gameStates.zoom.nFactor * 100 / I2X (1));

	m_info [0].headingTime = (m_info [0].headingTime * 100) / r;
	m_info [0].pitchTime = (m_info [0].pitchTime * 100) / r;
	m_info [0].bankTime = (m_info [0].bankTime * 100) / r;
	}
gameStates.input.bKeepSlackTime = 0;
m_frameCount = 0;
m_pollTime = 0;
return 0;
}

//------------------------------------------------------------------------------

void CControlsManager::ResetCruise (void)
{
gameStates.input.nCruiseSpeed = 0;
}

//------------------------------------------------------------------------------

void CControlsManager::CybermouseAdjust (void)
{
#if 0
if (N_LOCALPLAYER > -1) {
	OBJECTS [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_TURNROLL);	// Turn off roll when turning
	OBJECTS [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_LEVELLING);	// Turn off leveling to nearest CSide.
	gameOpts->gameplay.nAutoLeveling = 0;

	if (kc_externalVersion > 0) {
		CFixMatrix tempm, ViewMatrix;
		CAngleVector * Kconfig_abs_movement;
		char * oem_message;

		Kconfig_abs_movement = reinterpret_cast<CAngleVector*> (((uint)externalControls.m_info + sizeof (tControlInfo));

		if (Kconfig_abs_movement->p || Kconfig_abs_movement->b || Kconfig_abs_movement->h) {
			VmAngles2Matrix (&tempm,Kconfig_abs_movement);
			VmMatMul (&ViewMatrix,&OBJECTS [LOCALPLAYER.nObject].info.position.mOrient,&tempm);
			OBJECTS [LOCALPLAYER.nObject].info.position.mOrient = ViewMatrix;
			}
		oem_message = reinterpret_cast<char*> (((uint)Kconfig_abs_movement + sizeof (CAngleVector));
		if (oem_message [0] != '\0')
			HUDInitMessage (oem_message);
		}
	}
#endif
	m_info [0].pitchTime += FixMul (externalControls.m_info->pitchTime,gameData.time.xFrame);
	m_info [0].verticalThrustTime += FixMul (externalControls.m_info->verticalThrustTime,gameData.time.xFrame);
	m_info [0].headingTime += FixMul (externalControls.m_info->headingTime,gameData.time.xFrame);
	m_info [0].sidewaysThrustTime += FixMul (externalControls.m_info->sidewaysThrustTime, gameData.time.xFrame);
	m_info [0].bankTime += FixMul (externalControls.m_info->bankTime, gameData.time.xFrame);
	m_info [0].forwardThrustTime += FixMul (externalControls.m_info->forwardThrustTime, gameData.time.xFrame);
//	m_info [0].rearViewDownCount += externalControls.m_info->rearViewDownCount;
//	m_info [0].rearViewDownState |= externalControls.m_info->rearViewDownState;
	m_info [0].firePrimaryDownCount += externalControls.m_info->firePrimaryDownCount;
	m_info [0].firePrimaryState |= externalControls.m_info->firePrimaryState;
	m_info [0].fireSecondaryState |= externalControls.m_info->fireSecondaryState;
	m_info [0].fireSecondaryDownCount += externalControls.m_info->fireSecondaryDownCount;
	m_info [0].fireFlareDownCount += externalControls.m_info->fireFlareDownCount;
	m_info [0].dropBombDownCount += externalControls.m_info->dropBombDownCount;
//	m_info [0].automapDownCount += externalControls.m_info->automapDownCount;
// 	m_info [0].automapState |= externalControls.m_info->automapState;
  }

//------------------------------------------------------------------------------

char CControlsManager::GetKeyValue (char key)
{
return (kcKeyboard [int (key)].value);
 }

//------------------------------------------------------------------------------

void CControlsManager::FlushInput (void)
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
