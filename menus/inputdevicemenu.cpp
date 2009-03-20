/*
 *
 * SDL joystick support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif
#include <string.h>

#include "pstypes.h"
#include "descent.h"
#include "menu.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "key.h"
#include "kconfig.h"
#include "network.h"
#include "joy.h"
#include "joydefs.h"

//------------------------------------------------------------------------------

static struct {
	int	nUse;
	int	nMode;
	int	nDeadzone;
	int	nSensitivity;
	int	nMove;
	int	nSyncAxes;
} tirOpts;

static struct {
	int	nUse;
	int	nMouselook;
	int	nSyncAxes;
	int	nJoystick;
	int	nDeadzone;
	int	nSensitivity;
} mouseOpts;

static struct {
	int	nUse;
	int	nSyncAxes;
	int	nDeadzone;
	int	nSensitivity;
	int	nLinearSens;
} joyOpts;

static struct {
	int	nCustomize;
	int	nRamp;
	int	nRampValues;
	int	nUseHotkeys;
} kbdOpts;

int nCustomizeAboveOpt;

int nJoyDeadzones [] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 50};

char szAxis3D [3] = {'X', 'Y', 'R'};

char axis3DHotkeys [3] = {KEY_X, KEY_Y, KEY_R};

char tirMoveHotkeys [5] = {KEY_U, KEY_R, KEY_H, KEY_V, KEY_T};

char szJoyAxis [UNIQUE_JOY_AXES] = {'X', 'Y', 'R', 'T', 'Z'};

char joyHotkeys [UNIQUE_JOY_AXES] = {KEY_X, KEY_Y, KEY_R, KEY_T, KEY_Z};

static const char *szDZoneSizes [5];

int joydefs_calibrateFlag = 0;

//------------------------------------------------------------------------------

void JoyDefsCalibrate (void)
{
	joydefs_calibrateFlag = 0;

	if (!gameStates.input.nJoysticks) {
		MsgBox( NULL, NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}
	//Actual calibration if necessary
}

//------------------------------------------------------------------------------

int AddDeadzoneControl (CMenu& m, char* szText, const char *szFmt, const char *szHelp, const char **szSizes, ubyte nValue, char nKey)
{
if (szSizes)
	sprintf (szText + 1, szFmt, szSizes [nValue]);
else
	sprintf (szText + 1, szFmt);
*szText = *(szFmt - 1);
return m.AddSlider (szText + 1, nValue, 0, szSizes ? 4 : 15, nKey, szHelp);
}

//------------------------------------------------------------------------------

int AddAxisControls (CMenu& menu, char *szText, const char *szFmtSyncd, const char *szFmt, const char *szLabel, const char *szHelp, 
							int nControls, int *pnValues, int nValues, int *pnIntervals, 
							char nKeySyncd, char *pnKeys, int bSyncControls)
{
	int	h = 0, i, j, v, opt; 

j = bSyncControls ? 1 : nControls;
for (i = 0; i < j; i++) {
	if (pnIntervals) {
		for (v = nValues - 1; v >= 0; v--)
			if (pnValues [i] >= pnIntervals [v])
				break;
		}
	else
		v = pnValues [i];
	if (bSyncControls) {
		if (pnIntervals)
			sprintf (szText + i * 50 + 1, szFmtSyncd, pnValues [i]);
		else
			strcpy (szText + i * 50 + 1, szFmtSyncd);
		}
	else {
		if (pnIntervals)
			sprintf (szText + i * 50 + 1, szFmt, szLabel [i], pnValues [i]);
		else
			sprintf (szText + i * 50 + 1, szFmt, szLabel [i]);
		}
	szText [i * 50] = *(szFmtSyncd - 1);
	opt = menu.AddSlider (szText + i * 50 + 1, v, 0, nValues - 1, bSyncControls ? nKeySyncd : pnKeys [i], szHelp);
	if (!i)
		h = opt;
	}
return h;
}

//------------------------------------------------------------------------------

int MouseConfigCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int h, i, v;
	int ocType = gameConfig.nControlType;
	CMenuItem * m;

SetControlType ();
if ((ocType != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS)) {
	MsgBox (TXT_IMPORTANT_NOTE, NULL, 1, TXT_OK, TXT_FCS);
	}
if (ocType != gameConfig.nControlType) {
	switch (gameConfig.nControlType) {
//		case	CONTROL_NONE:
		case	CONTROL_JOYSTICK:
		case	CONTROL_FLIGHTSTICK_PRO:
		case	CONTROL_THRUSTMASTER_FCS:
		case	CONTROL_GRAVIS_GAMEPAD:
//		case	CONTROL_MOUSE:
//		case	CONTROL_CYBERMAN:
			joydefs_calibrateFlag = 1;
		}
	KCSetControls(0);
	}

m = menu + mouseOpts.nUse;
v = m->m_value;
if (gameOpts->input.mouse.bUse != v) {
	if (gameStates.app.bNostalgia) {
		gameOpts->input.mouse.bUse = v;
		gameOpts->input.joystick.bUse = gameStates.input.nJoysticks && !v;
		m->m_bRedraw = 1;
		}
	else {
		m->m_bRebuild = 1;
		key = -2;
		}
	return nCurItem;
	}

if (gameStates.app.bNostalgia)
	return nCurItem;

if (gameOpts->input.mouse.bUse) {
	if (gameOpts->app.bExpertMode) {
		m = menu + mouseOpts.nSyncAxes;
		v = m->m_value;
		if (gameOpts->input.mouse.bSyncAxes != v) {
			gameOpts->input.mouse.bSyncAxes = v;
			if (gameOpts->input.mouse.bSyncAxes)
				for (i = 1; i < 3; i++)
					gameOpts->input.mouse.sensitivity [i] = gameOpts->input.mouse.sensitivity [0];
			m->m_bRebuild = 1;
			key = -2;
			return nCurItem;
			}
		m = menu + mouseOpts.nJoystick;
		v = m->m_value;
		if (gameOpts->input.mouse.bJoystick != v) {
			gameOpts->input.mouse.bJoystick = v;
			key = -2;
			}
		}
	h = gameOpts->input.mouse.bSyncAxes ? 1 : 3;
	for (i = 0; i < h; i++)
		gameOpts->input.mouse.sensitivity [i] = menu [mouseOpts.nSensitivity + i].m_value;
	for (i = h; i < 3; i++)
		gameOpts->input.mouse.sensitivity [i] = gameOpts->input.mouse.sensitivity [0];
	if (gameOpts->input.mouse.bJoystick && (mouseOpts.nDeadzone >= 0)) {
		m = menu + mouseOpts.nDeadzone;
		v = m->m_value;
		if (gameOpts->input.mouse.nDeadzone != v) {
			gameOpts->input.mouse.nDeadzone = v;
			sprintf (m->m_text, TXT_MOUSE_DEADZONE, szDZoneSizes [v]);
			m->m_bRebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void MouseConfigMenu (void)
{
	CMenu	m;
	int	i, j, choice = 0;
	int	nCustMouseOpt, nMouseTypeOpt;
	char	szMouseSens [3][50];
	char	szMouseDeadzone [50];

szDZoneSizes [0] = TXT_NONE;
szDZoneSizes [1] = TXT_SMALL;
szDZoneSizes [2] = TXT_MEDIUM;
szDZoneSizes [3] = TXT_LARGE;
szDZoneSizes [4] = TXT_VERY_LARGE;

do {
	do {
		m.Destroy ();
		m.Create (20);

		nCustMouseOpt = nMouseTypeOpt = -1;
		SetControlType ();
		mouseOpts.nUse = m.AddCheck (TXT_USE_MOUSE, gameOpts->input.mouse.bUse, KEY_M, HTX_CONF_USEMOUSE);
		mouseOpts.nDeadzone = -1;
		if (gameOpts->input.mouse.bUse || gameStates.app.bNostalgia) {
			if (gameOpts->input.mouse.bUse)
				nCustMouseOpt = m.AddMenu (TXT_CUST_MOUSE, KEY_O, HTX_CONF_CUSTMOUSE);
			m.AddText ("", 0);
			if (gameStates.app.bNostalgia) 
				gameOpts->input.mouse.bSyncAxes = 1;
			else
				mouseOpts.nSyncAxes = m.AddCheck (TXT_SYNC_MOUSE_AXES, gameOpts->input.mouse.bSyncAxes, KEY_A, HTX_CONF_SYNCMOUSE);
			mouseOpts.nSensitivity = AddAxisControls (m, &szMouseSens [0][0], TXT_MOUSE_SENS, TXT_MOUSE_SENS_N, szAxis3D, HTX_CONF_MOUSESENS, 
																	3, gameOpts->input.mouse.sensitivity, 16, NULL, KEY_O, axis3DHotkeys, gameOpts->input.mouse.bSyncAxes);
			if (gameOpts->input.mouse.bUse && !gameStates.app.bNostalgia) {
				m.AddText ("", 0);
				mouseOpts.nMouselook = m.AddCheck (TXT_MOUSELOOK, extraGameInfo [0].bMouseLook, KEY_L, HTX_CONF_MOUSELOOK);
				mouseOpts.nJoystick = m.AddCheck (TXT_JOYMOUSE, gameOpts->input.mouse.bJoystick, KEY_J, HTX_CONF_JOYMOUSE);
				if (gameOpts->input.mouse.bJoystick && gameOpts->app.bExpertMode)
					mouseOpts.nDeadzone = AddDeadzoneControl (m, szMouseDeadzone, TXT_MOUSE_DEADZONE, HTX_MOUSE_DEADZONE, szDZoneSizes, gameOpts->input.mouse.nDeadzone, KEY_U);
				m.AddText ("", 0);
				nMouseTypeOpt = m.AddRadio (TXT_STD_MOUSE, 0, 0, HTX_CONF_STDMOUSE);
				m.AddRadio (TXT_CYBERMAN, 0, 0, HTX_CONF_CYBERMAN);
				m [nMouseTypeOpt + NMCLAMP (gameStates.input.nMouseType - CONTROL_MOUSE, 0, 1)].m_value = 1;
				}
			}
		i = m.Menu (NULL, TXT_MOUSE_CONFIG, MouseConfigCallback, &choice);
		gameOpts->input.mouse.bUse = m [mouseOpts.nUse].m_value;
		if (gameOpts->input.mouse.bUse && !gameStates.app.bNostalgia) {
			if (nMouseTypeOpt < 0) {
				gameStates.input.nMouseType = CONTROL_MOUSE;
				}
			else {
				extraGameInfo [0].bMouseLook = m [mouseOpts.nMouselook].m_value;
				for (j = 0; j < 2; j++) {
					if (m [nMouseTypeOpt + j].m_value) {
						gameStates.input.nMouseType = CONTROL_MOUSE + j;
						break;
						}
					}
				}
			}
		} while (i == -2);
		if (i == -1)
			return;
		gameConfig.nControlType = gameOpts->input.mouse.bUse ? CONTROL_MOUSE : gameConfig.nControlType;
		if (choice == nCustMouseOpt)
			KConfig (2, TXT_CFG_MOUSE);
	} while (i >= 0);
}

//------------------------------------------------------------------------------

int JoystickConfigCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;
	int h, i, v;
	int ocType = gameConfig.nControlType;
	CMenuItem * m;

SetControlType ();
if ((ocType != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS)) {
	MsgBox (TXT_IMPORTANT_NOTE, NULL, 1, TXT_OK, TXT_FCS);
	}

if (ocType != gameConfig.nControlType) {
	switch (gameConfig.nControlType) {
//		case	CONTROL_NONE:
		case	CONTROL_JOYSTICK:
		case	CONTROL_FLIGHTSTICK_PRO:
		case	CONTROL_THRUSTMASTER_FCS:
		case	CONTROL_GRAVIS_GAMEPAD:
//		case	CONTROL_MOUSE:
//		case	CONTROL_CYBERMAN:
			joydefs_calibrateFlag = 1;
		}
	KCSetControls(0);
	}
m = menu + joyOpts.nUse;
v = m->m_value;
if (gameOpts->input.joystick.bUse != v) {
	if (gameStates.app.bNostalgia) {
		gameOpts->input.joystick.bUse = v;
		gameOpts->input.mouse.bUse = !v;
		m->m_bRedraw = 1;
		}
	else {
		m->m_bRebuild = 1;
		key = -2;
		}
	return nCurItem;
	}

if (gameStates.app.bNostalgia)
	return nCurItem;


if (gameStates.input.nJoysticks && gameOpts->input.joystick.bUse) {
	if (gameOpts->app.bExpertMode) {
		m = menu + joyOpts.nSyncAxes;
		v = m->m_value;
		if (gameOpts->input.joystick.bSyncAxes != v) {
			gameOpts->input.joystick.bSyncAxes = v;
			if (gameOpts->input.joystick.bSyncAxes)
				for (i = 1; i < UNIQUE_JOY_AXES; i++) {
					gameOpts->input.joystick.deadzones [i] = gameOpts->input.joystick.deadzones [0];
					gameOpts->input.joystick.sensitivity [i] = gameOpts->input.joystick.sensitivity [0];
					}
			m->m_bRebuild = 1;
			key = -2;
			return nCurItem;
			}
		}
	h = gameOpts->input.joystick.bSyncAxes ? 1 : UNIQUE_JOY_AXES;
	for (i = 0; i < h; i++)
		gameOpts->input.joystick.sensitivity [i] = menu [joyOpts.nSensitivity + i].m_value;
	for (i = h; i < UNIQUE_JOY_AXES; i++)
		gameOpts->input.joystick.sensitivity [i] = gameOpts->input.joystick.sensitivity [0];

	for (i = 0; i < h; i++) {
		m = menu + joyOpts.nDeadzone + i;
		v = nJoyDeadzones [m->m_value];
		if (gameOpts->input.joystick.deadzones [i] != v) {
			gameOpts->input.joystick.deadzones [i] = v;
			JoySetDeadzone (gameOpts->input.joystick.deadzones [i], i);
			if (gameOpts->input.joystick.bSyncAxes)
				sprintf (m->m_text, TXT_JOY_DEADZONE, gameOpts->input.joystick.deadzones [i]);
			else
				sprintf (m->m_text, TXT_JOY_DEADZONE_N, szJoyAxis [i], gameOpts->input.joystick.deadzones [i]);
			m->m_bRebuild = 1;
			}
		}
	for (i = h; i < UNIQUE_JOY_AXES; i++)
		gameOpts->input.joystick.deadzones [i] = gameOpts->input.joystick.deadzones [0];
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void JoystickConfigMenu (void)
{
	CMenu m;
	int	h, i, j, choice = 0;
	int	nCustJoyOpt, nJoyTypeOpt;
	char	szJoySens [UNIQUE_JOY_AXES][50];
	char	szJoyDeadzone [UNIQUE_JOY_AXES][50];

do {
	do {
		m.Destroy ();
		m.Create (20);

		nCustJoyOpt = nJoyTypeOpt = -1;
		SetControlType ();
		joyOpts.nUse = m.AddCheck (TXT_USE_JOY, gameOpts->input.joystick.bUse, KEY_J, HTX_CONF_USEJOY);
		if (gameOpts->input.joystick.bUse || gameStates.app.bNostalgia) {
			nCustJoyOpt = m.AddMenu (TXT_CUST_JOY, KEY_C, HTX_CONF_CUSTJOY);
			if (gameStates.app.bNostalgia || !gameOpts->app.bExpertMode)
				gameOpts->input.joystick.bSyncAxes = 1;
			else {
				joyOpts.nLinearSens = m.AddCheck (TXT_JOY_LINSCALE, gameOpts->input.joystick.bLinearSens, KEY_I, HTX_CONF_LINJOY);
				joyOpts.nSyncAxes = m.AddCheck (TXT_SYNC_JOY_AXES, gameOpts->input.joystick.bSyncAxes, KEY_Y, HTX_CONF_SYNCJOY);
				}
			h = gameOpts->input.joystick.bSyncAxes ? 1 : UNIQUE_JOY_AXES;
			joyOpts.nSensitivity = AddAxisControls (m, &szJoySens [0][0], TXT_JOY_SENS, TXT_JOY_SENS_N, szJoyAxis, HTX_CONF_JOYSENS, 
																 UNIQUE_JOY_AXES, gameOpts->input.joystick.sensitivity, 16, NULL, KEY_S, joyHotkeys, gameOpts->input.joystick.bSyncAxes);
			if (!gameStates.app.bNostalgia) {
				joyOpts.nDeadzone = AddAxisControls (m, &szJoyDeadzone [0][0], TXT_JOY_DEADZONE, TXT_JOY_DEADZONE_N, szJoyAxis, HTX_CONF_JOYDZONE, 
																 UNIQUE_JOY_AXES, gameOpts->input.joystick.deadzones, 16, nJoyDeadzones, KEY_S, joyHotkeys, gameOpts->input.joystick.bSyncAxes);
				m.AddText ("", 0);
				nJoyTypeOpt = m.AddRadio (TXT_STD_JOY, 0, 0, HTX_CONF_STDJOY);
				m.AddRadio (TXT_FSPRO_JOY, 0, 0, HTX_CONF_FSPRO);
				m.AddRadio (TXT_FCS_JOY, 0, 0, HTX_CONF_FCS);
				m.AddRadio (TXT_GRAVIS_JOY, 0, 0, HTX_CONF_GRAVIS);
				m [nJoyTypeOpt + NMCLAMP (gameStates.input.nJoyType - CONTROL_JOYSTICK, 0, 3)].m_value = 1;
				}
			}
		i = m.Menu (NULL, TXT_JOYSTICK_CONFIG, JoystickConfigCallback, &choice);
		gameOpts->input.joystick.bUse = m [joyOpts.nUse].m_value;
		if (gameOpts->input.joystick.bUse && !gameStates.app.bNostalgia) {
			if (nJoyTypeOpt < 0)
				gameStates.input.nJoyType = CONTROL_JOYSTICK;
			else {
				if (gameOpts->app.bExpertMode) {
					gameOpts->input.joystick.bLinearSens = m [joyOpts.nLinearSens].m_value;
				for (j = 0; j < UNIQUE_JOY_AXES; j++)
					if (m [nJoyTypeOpt + j].m_value) {
						gameStates.input.nJoyType = CONTROL_JOYSTICK + j;
						break;
						}
					}
				}
			}
		} while (i == -2);
	if (i == -1)
		return;
	if (choice == nCustJoyOpt)
		KConfig (1, TXT_CFG_JOY);
	} while (i >= 0);
}

//------------------------------------------------------------------------------

int TrackIRConfigCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int			h, i, v;
	CMenuItem*	m;

m = menu + tirOpts.nUse;
v = m->m_value;
if (gameOpts->input.trackIR.bUse != v) {
	gameOpts->input.trackIR.bUse = v;
	key = -2;
	return nCurItem;
	}
if (gameOpts->input.trackIR.bUse) {
	for (i = 0; i < 3; i++) {
		if (m [tirOpts.nMode + i].m_value && (gameOpts->input.trackIR.nMode != i)) {
			gameOpts->input.trackIR.nMode = i;
			if (i == 0) {
				gameData.trackIR.x = 
				gameData.trackIR.y = 0;
				}
			key = -2;
			return nCurItem;
			}
		}
	for (i = 0; i < 3; i++) {
		if (gameOpts->app.bExpertMode) {
			m = menu + tirOpts.nSyncAxes;
			v = m->m_value;
			if (gameOpts->input.trackIR.bSyncAxes != v) {
				gameOpts->input.trackIR.bSyncAxes = v;
				if (gameOpts->input.trackIR.bSyncAxes)
					for (i = 1; i < 3; i++)
						gameOpts->input.trackIR.sensitivity [i] = gameOpts->input.trackIR.sensitivity [0];
				m->m_bRebuild = 1;
				key = -2;
				return nCurItem;
				}
			}
		}
	for (i = 0; i < 5; i++) {
		if (tirOpts.nMove >= 0) {
			m = menu + tirOpts.nMove + i;
			v = m->m_value;
			if (gameOpts->input.trackIR.bMove [i] != v) {
				gameOpts->input.trackIR.bMove [i] = v;
				for (h = 0, i = 2; i < 5; i++)
					h += gameOpts->input.trackIR.bMove [i];
				if (h < 2) {
					key = -2;
					return nCurItem;
					}
				}
			}
		}
	if (tirOpts.nSyncAxes < 0)
		return nCurItem;
	h = gameOpts->input.trackIR.bSyncAxes ? 1 : 3;
	for (i = 0; i < h; i++)
		gameOpts->input.trackIR.sensitivity [i] = menu [tirOpts.nSensitivity + i].m_value;
	for (i = h; i < 3; i++)
		gameOpts->input.trackIR.sensitivity [i] = gameOpts->input.trackIR.sensitivity [0];
	if (tirOpts.nDeadzone < 0)
		return nCurItem;
	m = menu + tirOpts.nDeadzone;
	v = m->m_value;
	if (gameOpts->input.trackIR.nDeadzone != v) {
		gameOpts->input.trackIR.nDeadzone = v;
		sprintf (m->m_text, TXT_TRACKIR_DEADZONE, szDZoneSizes [v]);
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void TrackIRConfigMenu (void)
{
	CMenu	m;
	int	i, choice = 0;
	char	szTrackIRSens [3][50];
	char	szTrackIRDeadzone [50];

szDZoneSizes [0] = TXT_NONE;
szDZoneSizes [1] = TXT_SMALL;
szDZoneSizes [2] = TXT_MEDIUM;
szDZoneSizes [3] = TXT_LARGE;
szDZoneSizes [4] = TXT_VERY_LARGE;

do {
	m.Destroy ();
	m.Create (20);

	tirOpts.nUse = m.AddCheck (TXT_USE_TRACKIR, gameOpts->input.trackIR.bUse, KEY_M, HTX_USE_TRACKIR);
	if (gameOpts->input.trackIR.bUse) {
		tirOpts.nMode = m.AddRadio (TXT_TRACKIR_AIM, 0, KEY_A, HTX_TRACKIR_AIM);
		m.AddRadio (TXT_TRACKIR_STEER, 0, KEY_S, HTX_TRACKIR_STEER);
		m.AddRadio (TXT_TRACKIR_LOOK, 0, KEY_L, HTX_TRACKIR_LOOK);
		m [tirOpts.nMode + gameOpts->input.trackIR.nMode].m_value = 1;
		m.AddText ("", 0);
		tirOpts.nMove = m.ToS ();
		for (i = 0; i < 5; i++)
			m.AddCheck (GT (924 + i), gameOpts->input.trackIR.bMove [i], tirMoveHotkeys [i], HT (281 + i));
		if ((gameOpts->input.trackIR.nMode != 1) ||
			 gameOpts->input.trackIR.bMove [0] || 
			 gameOpts->input.trackIR.bMove [1] || 
			 gameOpts->input.trackIR.bMove [2] || 
			 gameOpts->input.trackIR.bMove [3] || 
			 gameOpts->input.trackIR.bMove [4]) {
			m.AddText ("", 0);
			tirOpts.nSyncAxes = m.AddCheck (TXT_SYNC_TRACKIR_AXES, gameOpts->input.trackIR.bSyncAxes, KEY_K, HTX_SYNC_TRACKIR_AXES);
			tirOpts.nSensitivity = AddAxisControls (m, &szTrackIRSens [0][0], TXT_TRACKIR_SENS, TXT_TRACKIR_SENS_N, szAxis3D, HTX_TRACKIR_SENS, 
																 3, gameOpts->input.trackIR.sensitivity, 16, NULL, KEY_S, axis3DHotkeys, gameOpts->input.trackIR.bSyncAxes);
			tirOpts.nDeadzone = AddDeadzoneControl (m, szTrackIRDeadzone, TXT_TRACKIR_DEADZONE, HTX_TRACKIR_DEADZONE, NULL, gameOpts->input.trackIR.nDeadzone, KEY_U);
			}
		else
			tirOpts.nSyncAxes =
			tirOpts.nDeadzone = -1;
		}
	i = m.Menu (NULL, TXT_TRACKIR_CONFIG, TrackIRConfigCallback, &choice);
	if (gameStates.input.bHaveTrackIR)
		gameOpts->input.trackIR.bUse = m [tirOpts.nUse].m_value;
	} while ((i >= 0) || (i == -2));
}

//------------------------------------------------------------------------------

int KeyboardConfigCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int			i, v;
	CMenuItem	*m;

if (!gameStates.app.bNostalgia) { 
	m = menu + kbdOpts.nUseHotkeys;
	v = m->m_value;
	if (gameOpts->input.bUseHotKeys != v) {
		gameOpts->input.bUseHotKeys = v;
		key = -2;
		return nCurItem;
		}
	}

if (gameOpts->app.bExpertMode) {
	m = menu + kbdOpts.nRamp;
	v = m->m_value * 10;
	if (gameOpts->input.keyboard.nRamp != v) {
		if (!(gameOpts->input.keyboard.nRamp && v))
			key = -2;
		gameOpts->input.keyboard.nRamp = v;
		sprintf(m->m_text, TXT_KBD_RAMP, gameOpts->input.keyboard.nRamp);
		m->m_bRebuild = 1;
		}

	for (i = 0; i < 3; i++) {
		m = menu + kbdOpts.nRampValues + i;
		v = (m->m_value != 0);
		if (gameOpts->input.keyboard.bRamp [i] != v) {
			gameOpts->input.keyboard.bRamp [i] = v;
			m->m_bRedraw = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void KeyboardConfigMenu (void)
{
	CMenu	m;
	int	i, choice = 0;
	int	nCustKbdOpt, nCustHotKeysOpt, optKeyboard;
	char	szKeyRampScale [50];

do {
	do {
		m.Destroy ();
		m.Create (20);

		nCustHotKeysOpt = -1;
		SetControlType ();
		nCustKbdOpt = m.AddMenu (TXT_CUST_JOY, KEY_K, HTX_CONF_CUSTKBD);
		if (gameStates.app.bNostalgia) {
			gameOpts->input.keyboard.nRamp = 100;
			gameOpts->input.bUseHotKeys = 0;
			}
		else {
			if (gameOpts->app.bExpertMode) {
				m.AddText ("", 0);
				sprintf (szKeyRampScale + 1, TXT_KBD_RAMP, gameOpts->input.keyboard.nRamp, HTX_CONF_KBDRAMP);
				*szKeyRampScale = *(TXT_KBD_RAMP - 1);
				kbdOpts.nRamp = m.AddSlider (szKeyRampScale + 1, gameOpts->input.keyboard.nRamp / 10, 0, 10, KEY_R, HTX_CONF_RAMPSCALE);   
				if (gameOpts->input.keyboard.nRamp > 0) {
					kbdOpts.nRampValues = m.AddCheck (TXT_RAMP_ACCEL, gameOpts->input.keyboard.bRamp [0], KEY_A, HTX_CONF_RAMPACC);
					m.AddCheck (TXT_RAMP_ROT, gameOpts->input.keyboard.bRamp [1], KEY_O, HTX_CONF_RAMPROT);
					m.AddCheck (TXT_RAMP_SLIDE, gameOpts->input.keyboard.bRamp [2], KEY_D, HTX_CONF_RAMPSLD);
					}
				}
			m.AddText ("", 0);
			kbdOpts.nUseHotkeys = m.AddCheck (TXT_USE_HOTKEYS, gameOpts->input.bUseHotKeys, KEY_H, HTX_CONF_HOTKEYS);
			if (gameOpts->input.bUseHotKeys)
				nCustHotKeysOpt = m.AddMenu (TXT_CUST_HOTKEYS, KEY_W, HTX_CONF_CUSTHOT);
			}
		m.AddText ("", 0);
		m.AddText (TXT_KEYBOARD_LAYOUT, 0);
		optKeyboard = m.AddRadio (TXT_QWERTY, gameOpts->input.keyboard.nType == 0, KEY_E, HTX_KEYBOARD_LAYOUT);
		m.AddRadio (TXT_QWERTZ, gameOpts->input.keyboard.nType == 1, KEY_G, HTX_KEYBOARD_LAYOUT);
		m.AddRadio (TXT_AZERTY, gameOpts->input.keyboard.nType == 2, KEY_F, HTX_KEYBOARD_LAYOUT);
		m.AddRadio (TXT_DVORAK, gameOpts->input.keyboard.nType == 3, KEY_D, HTX_KEYBOARD_LAYOUT);
		i = m.Menu (NULL, TXT_KEYBOARD_CONFIG, KeyboardConfigCallback, &choice);
		if (!gameStates.app.bNostalgia)
			gameOpts->input.bUseHotKeys = m [kbdOpts.nUseHotkeys].m_value;
		} while (i == -2);
	if (i == -1)
		return;
	for (int j = 0; j < 4; j++)
		if (m [optKeyboard + j].m_value != 0) {
			gameOpts->input.keyboard.nType = j;
			break;
			}
	if (choice == nCustKbdOpt)
		KConfig (0, TXT_CFG_KBD);
	else if (choice == nCustHotKeysOpt)
		KConfig (4, TXT_CFG_HOTKEYS);
	} while (i >= 0);
}

//------------------------------------------------------------------------------

void InputDeviceConfig (void)
{
	CMenu	m;
	int	i, choice = 0;
	int	nMouseOpt, nJoystickOpt, nTrackIROpt, nKeyboardOpt, nFastPitchOpt;

do {
	if (TIRLoad ())
		pfnTIRStart ();
	m.Destroy ();
	m.Create (10);

	SetControlType ();
	nKeyboardOpt = m.AddMenu (TXT_KBDCFG_MENUCALL, KEY_K, HTX_KEYBOARD_CONFIG);
	nMouseOpt = m.AddMenu (TXT_MOUSECFG_MENUCALL, KEY_M, HTX_MOUSE_CONFIG);
	if (gameStates.input.nJoysticks)
		nJoystickOpt = m.AddMenu (TXT_JOYCFG_MENUCALL, KEY_J, HTX_JOYSTICK_CONFIG);
	else
		nJoystickOpt = -1;
	if (!gameStates.app.bNostalgia && gameStates.input.bHaveTrackIR) 
		nTrackIROpt = m.AddMenu (TXT_TRACKIRCFG_MENUCALL, KEY_I, HTX_TRACKIR_CONFIG);
	else
		nTrackIROpt = -1;
	m.AddText ("", 0);
	if (gameOpts->app.bExpertMode && (gameStates.app.bNostalgia < 3)) {
		nFastPitchOpt = m.AddCheck (TXT_FASTPITCH, (extraGameInfo [0].bFastPitch == 1) ? 1 : 0, KEY_T, HTX_CONF_FASTPITCH);
		extraGameInfo [0].bFastPitch = m [nFastPitchOpt].m_value ? 1 : 2;
		}
	else
		nFastPitchOpt = -1;
	i = m.Menu (NULL, TXT_CONTROLS, NULL, &choice);
	if (i == -1)
		return;
	if (nFastPitchOpt >= 0)
		extraGameInfo [0].bFastPitch = m [nFastPitchOpt].m_value ? 1 : 2;
	if (choice == nKeyboardOpt)
		KeyboardConfigMenu ();
	else if (choice == nMouseOpt)
		MouseConfigMenu ();
	else if ((nJoystickOpt >= 0) && (choice == nJoystickOpt))
		JoystickConfigMenu ();
	else if ((nTrackIROpt >= 0) && (choice == nTrackIROpt))
		TrackIRConfigMenu ();
	} while (i >= 0);
}

//------------------------------------------------------------------------------
// eof
