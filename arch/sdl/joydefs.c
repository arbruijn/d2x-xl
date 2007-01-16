/* $Id: joydefs.c,v 1.7 2003/10/08 19:18:46 btb Exp $ */
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
#include "inferno.h"
#include "menu.h"
#include "newmenu.h"
#include "config.h"
#include "text.h"
#include "key.h"
#include "kconfig.h"
#include "network.h"
#include "joy.h"
#include "joydefs.h"

//------------------------------------------------------------------------------


int	nDeadZoneOpt, nRampOpt, nRampKeyOpt, nJoySensOpt, nMouseSensOpt, aboveCustOpt, 
		kbdCustOpt, nLinSensOpt, nMouseLookOpt, nSyncJoyAxesOpt, nSyncMouseAxesOpt,
		nUseMouseOpt, nUseJoyOpt, nUseHotKeysOpt;

int nJoyDeadzones [] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 50};

char szMouseAxis [3] = {'X', 'Y', 'R'};

char mouseHotkeys [3] = {KEY_X, KEY_Y, KEY_R};

char szJoyAxis [UNIQUE_JOY_AXES] = {'X', 'Y', 'R', 'T', 'Z'};

char joyHotkeys [UNIQUE_JOY_AXES] = {KEY_X, KEY_Y, KEY_R, KEY_T, KEY_Z};

int joydefs_calibrateFlag = 0;

//------------------------------------------------------------------------------

void joydefs_calibrate()
{
	joydefs_calibrateFlag = 0;

	if (!gameOpts->input.nJoysticks) {
		ExecMessageBox( NULL, NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}
	//Actual calibration if necessary
}

//------------------------------------------------------------------------------

void joydef_menu_callback (int nitems, tMenuItem * items, int *last_key, int citem)
{
	int h, i, v;
	int ocType = gameConfig.nControlType;
	tMenuItem * m;
/*
	for (i=0; i<3; i++ )
		if (items[i].value) 
			gameConfig.nControlType = i;

	if (gameConfig.nControlType == 2) 
		gameConfig.nControlType = CONTROL_MOUSE;
*/
	SetControlType ();
	if ((ocType != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS)) {
		ExecMessageBox( TXT_IMPORTANT_NOTE, NULL, 1, TXT_OK, TXT_FCS );
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
		KCSetControls();
	}

if (items [nUseMouseOpt].value != gameOpts->input.bUseMouse)
	h = nUseMouseOpt;
else if ((nUseJoyOpt >= 0) && (items [nUseJoyOpt].value != gameOpts->input.bUseJoystick))
	h = nUseJoyOpt;
else if (!gameStates.app.bNostalgia && (items [nUseHotKeysOpt].value != gameOpts->input.bUseHotKeys))
	h = nUseHotKeysOpt;
else
	h = -1;
if (h >= 0) {
//	items [h].value = !items [h].value;
	if (gameStates.app.bNostalgia) {
		v = items [h].value;
		h = (h == nUseMouseOpt) ? nUseJoyOpt : nUseMouseOpt;
		items [h].value = !v;
		gameOpts->input.bUseMouse = items [nUseMouseOpt].value;
		gameOpts->input.bUseJoystick = gameOpts->input.nJoysticks && !gameOpts->input.bUseMouse;
		items [h].redraw = 1;
		}
	else {
		items [h].rebuild = 1;
		*last_key = -2;
		}
	return;
	}

if (gameStates.app.bNostalgia)
	return;

if (gameOpts->input.bUseMouse) {
	if (gameOpts->app.bExpertMode) {
		m = items + nSyncMouseAxesOpt;
		v = m->value;
		if (gameOpts->input.bSyncMouseAxes != v) {
			gameOpts->input.bSyncMouseAxes = v;
			if (gameOpts->input.bSyncMouseAxes)
				for (i = 1; i < 3; i++)
					gameOpts->input.mouseSensitivity [i] = gameOpts->input.mouseSensitivity [0];
			m->rebuild = 1;
			*last_key = -2;
			return;
			}
		}
	h = gameOpts->input.bSyncMouseAxes ? 1 : 3;
	for (i = 0; i < h; i++)
		gameOpts->input.mouseSensitivity [i] = items [nMouseSensOpt + i].value;
	for (i = h; i < 3; i++)
		gameOpts->input.mouseSensitivity [i] = gameOpts->input.mouseSensitivity [0];
	}

if (gameOpts->input.bUseJoystick) {
	if (gameOpts->app.bExpertMode) {
		m = items + nSyncJoyAxesOpt;
		v = m->value;
		if (gameOpts->input.bSyncJoyAxes != v) {
			gameOpts->input.bSyncJoyAxes = v;
			if (gameOpts->input.bSyncJoyAxes)
				for (i = 1; i < UNIQUE_JOY_AXES; i++) {
					gameOpts->input.joyDeadZones [i] = gameOpts->input.joyDeadZones [0];
					gameOpts->input.joySensitivity [i] = gameOpts->input.joySensitivity [0];
					}
			m->rebuild = 1;
			*last_key = -2;
			return;
			}
		}
	h = gameOpts->input.bSyncJoyAxes ? 1 : UNIQUE_JOY_AXES;
	for (i = 0; i < h; i++)
		gameOpts->input.joySensitivity [i] = items [nJoySensOpt + i].value;
	for (i = h; i < UNIQUE_JOY_AXES; i++)
		gameOpts->input.joySensitivity [i] = gameOpts->input.joySensitivity [0];

	for (i = 0; i < h; i++) {
		m = items + nDeadZoneOpt + i;
		v = nJoyDeadzones [m->value];
		if (gameOpts->input.joyDeadZones [i] != v) {
			gameOpts->input.joyDeadZones [i] = v;
			set_joy_deadzone (gameOpts->input.joyDeadZones [i], i);
			if (gameOpts->input.bSyncJoyAxes)
				sprintf (m->text, TXT_JOY_DEADZONE, gameOpts->input.joyDeadZones [i]);
			else
				sprintf (m->text, TXT_JOY_DEADZONE_N, szJoyAxis [i], gameOpts->input.joyDeadZones [i]);
			m->rebuild = 1;
			*last_key = -2;
			}
		}
	for (i = h; i < UNIQUE_JOY_AXES; i++)
		gameOpts->input.joyDeadZones [i] = gameOpts->input.joyDeadZones [0];
	}
if (gameOpts->app.bExpertMode) {
	m = items + nRampOpt;
	v = m->value * 10;
	if (gameOpts->input.keyRampScale != v) {
		if (!(gameOpts->input.keyRampScale && v))
			*last_key = -2;
		gameOpts->input.keyRampScale = v;
		sprintf(m->text, TXT_KBD_RAMP, gameOpts->input.keyRampScale);
		m->rebuild = 1;
		}

	for (i = 0; i < 3; i++) {
		m = items + nRampKeyOpt + i;
		v = (m->value != 0);
		if (gameOpts->input.bRampKeys [i] != v) {
			gameOpts->input.bRampKeys [i] = v;
			m->redraw = 1;
			}
		}
	}
}

//------------------------------------------------------------------------------

void joydefs_config()
{
	tMenuItem m [40];
	int	h, i, j, opt = 0, choice = 0;
	int	nCustKbdOpt, nCustMouseOpt, nCustJoyOpt, nCustHotKeysOpt, nMouseTypeOpt, nJoyTypeOpt, 
			nFastPitchOpt, nJoyMouseOpt;
	char	szKeyRampScale [40];
	char	szMouseSens [3][40];
	char	szJoySens [UNIQUE_JOY_AXES][40];
	char	szJoyDeadzone [UNIQUE_JOY_AXES][40];

rebuild_menu:

	memset (&m, 0, sizeof (m));
	opt = 0;

	nCustMouseOpt = nCustJoyOpt = nCustHotKeysOpt =	nMouseTypeOpt = nJoyTypeOpt = -1;
	SetControlType ();
	ADD_CHECK (opt, TXT_USE_MOUSE, gameOpts->input.bUseMouse, KEY_M, HTX_CONF_USEMOUSE);
	nUseMouseOpt = opt++;
	if (gameOpts->input.bUseMouse || gameStates.app.bNostalgia) {
		ADD_MENU (opt, TXT_CUST_MOUSE, KEY_O, HTX_CONF_CUSTMOUSE);
		nCustMouseOpt = opt++;
		if (gameStates.app.bNostalgia || !gameOpts->app.bExpertMode) {
			gameOpts->input.bSyncMouseAxes = 1;
			if (!gameStates.app.bNostalgia) {
				ADD_CHECK (opt, TXT_MOUSELOOK, extraGameInfo [0].bMouseLook, KEY_L, HTX_CONF_MOUSELOOK);
				nMouseLookOpt = opt++;
				ADD_CHECK (opt, TXT_JOYMOUSE, gameOpts->input.bJoyMouse, KEY_Y, HTX_CONF_JOYMOUSE);
				nJoyMouseOpt = opt++;
				}
			}
		else {
			ADD_CHECK (opt, TXT_MOUSELOOK, extraGameInfo [0].bMouseLook, KEY_L, HTX_CONF_MOUSELOOK);
			nMouseLookOpt = opt++;
			ADD_CHECK (opt, TXT_JOYMOUSE, gameOpts->input.bJoyMouse, KEY_Y, HTX_CONF_JOYMOUSE);
			nJoyMouseOpt = opt++;
			ADD_CHECK (opt, TXT_SYNC_MOUSE_AXES, gameOpts->input.bSyncMouseAxes, KEY_Y, HTX_CONF_SYNCMOUSE);
			nSyncMouseAxesOpt = opt++;
			}
		h = gameOpts->input.bSyncMouseAxes ? 1 : 3;
		for (i = 0; i < h; i++) {
			if (gameOpts->input.bSyncMouseAxes)
				strcpy (szMouseSens [i] + 1, TXT_MOUSE_SENS);
			else 
				sprintf (szMouseSens [i] + 1, TXT_MOUSE_SENS_N, szMouseAxis [i]);
			*szMouseSens [i] = *(TXT_MOUSE_SENS - 1);
			ADD_SLIDER (opt, szMouseSens [i] + 1, gameOpts->input.mouseSensitivity [i], 0, 16, 
						   gameOpts->input.bSyncMouseAxes ? KEY_O : mouseHotkeys [i], HTX_CONF_MOUSESENS);
			if (!i)
				nMouseSensOpt = opt;
			opt++;
			}
		if (!gameStates.app.bNostalgia) {
			nMouseTypeOpt = opt;
			ADD_RADIO (opt, TXT_STD_MOUSE, 0, 0, 1, HTX_CONF_STDMOUSE);
			opt++;
			ADD_RADIO (opt, TXT_CYBERMAN, 0, 0, 1, HTX_CONF_CYBERMAN);
			opt++;
			m [nMouseTypeOpt + NMCLAMP (gameStates.input.nMouseType - CONTROL_MOUSE, 0, 1)].value = 1;
			}
		}
#ifdef RELEASE
	if (gameOpts->input.bUseMouse || (gameOpts->input.nJoysticks && gameOpts->input.bUseJoystick)) {
#else
	if (gameOpts->input.bUseMouse || gameOpts->input.bUseJoystick) {
#endif
		ADD_TEXT (opt, "", 0);
		opt++;
		}

#ifdef RELEASE
	if (!gameOpts->input.nJoysticks) 
		nUseJoyOpt = -1;
	else 
#endif
		{
		ADD_CHECK (opt, TXT_USE_JOY, gameOpts->input.bUseJoystick, KEY_J, HTX_CONF_USEJOY);
		nUseJoyOpt = opt++;
		if (gameOpts->input.bUseJoystick || gameStates.app.bNostalgia) {
			ADD_MENU (opt, TXT_CUST_JOY, KEY_C, HTX_CONF_CUSTJOY);
			nCustJoyOpt = opt++;
			if (gameStates.app.bNostalgia || !gameOpts->app.bExpertMode)
				gameOpts->input.bSyncJoyAxes = 1;
			else {
				ADD_CHECK (opt, TXT_JOY_LINSCALE, gameOpts->input.bLinearJoySens, KEY_I, HTX_CONF_LINJOY);
				nLinSensOpt = opt++;
				ADD_CHECK (opt, TXT_SYNC_JOY_AXES, gameOpts->input.bSyncJoyAxes, KEY_Y, HTX_CONF_SYNCJOY);
				nSyncJoyAxesOpt = opt++;
				}
			h = gameOpts->input.bSyncJoyAxes ? 1 : UNIQUE_JOY_AXES;
			for (i = 0; i < h; i++) {
				if (gameOpts->input.bSyncJoyAxes)
					strcpy (szJoySens [i] + 1, TXT_JOY_SENS); 
				else
					sprintf (szJoySens [i] + 1, TXT_JOY_SENS_N, szJoyAxis [i]);
				*szJoySens [i] = *(TXT_JOY_SENS - 1);
				ADD_SLIDER (opt, szJoySens [i] + 1,  gameOpts->input.joySensitivity [i], 0, 16, gameOpts->input.bSyncJoyAxes ? KEY_S : joyHotkeys [i], HTX_CONF_JOYSENS);
				if (!i)
					nJoySensOpt = opt;
				opt++;
				}
			if (!gameStates.app.bNostalgia) {
				for (i = 0; i < h; i++) {
					if (gameOpts->input.bSyncJoyAxes)
						sprintf(szJoyDeadzone [i] + 1, TXT_JOY_DEADZONE, gameOpts->input.joyDeadZones [i]);
					else
						sprintf(szJoyDeadzone [i] + 1, TXT_JOY_DEADZONE_N, szJoyAxis [i], gameOpts->input.joyDeadZones [i]);
					*szJoyDeadzone [i] = *(TXT_JOY_DEADZONE - 1);
					for (j = 15; j >= 0; j--)
						if (gameOpts->input.joyDeadZones [i] >= nJoyDeadzones [j])
							break;
					ADD_SLIDER (opt, szJoyDeadzone [i] + 1, j, 0, 16, gameOpts->input.bSyncJoyAxes ? KEY_Z : joyHotkeys [i], HTX_CONF_JOYDEAD);
					if (!i)
						nDeadZoneOpt = opt;
					opt++;
					}
				nJoyTypeOpt = opt;
				ADD_RADIO (opt, TXT_STD_JOY, 0, 0, 2, HTX_CONF_STDJOY);
				opt++;
				ADD_RADIO (opt, TXT_FSPRO_JOY, 0, 0, 2, HTX_CONF_FSPRO);
				opt++;
				ADD_RADIO (opt, TXT_FCS_JOY, 0, 0, 2, HTX_CONF_FCS);
				opt++;
				ADD_RADIO (opt, TXT_GRAVIS_JOY, 0, 0, 2, HTX_CONF_GRAVIS);
				opt++;
				m [nJoyTypeOpt + NMCLAMP (gameStates.input.nJoyType - CONTROL_JOYSTICK, 0, 3)].value = 1;
				}
			}
		if (gameOpts->input.bUseJoystick) {
			ADD_TEXT (opt, "", 0);
			opt++;
			}
		}	

#ifdef D2X_KEYS
	if (gameStates.app.bNostalgia)
		gameOpts->input.bUseHotKeys = 0;
	else {
		ADD_CHECK (opt, TXT_USE_HOTKEYS, gameOpts->input.bUseHotKeys, KEY_H, HTX_CONF_HOTKEYS);
		nUseHotKeysOpt = opt++;
		if (gameOpts->input.bUseHotKeys) {
			ADD_MENU (opt, TXT_CUST_HOTKEYS, KEY_W, HTX_CONF_CUSTHOT);
			nCustHotKeysOpt = opt++;
			}
		ADD_TEXT (opt, "", 0);
		opt++;
		}
#endif

	ADD_MENU (opt, TXT_CUST_KBD, KEY_K, HTX_CONF_CUSTKBD);
	nCustKbdOpt = opt++;
	if (gameStates.app.bNostalgia)
		gameOpts->input.keyRampScale = 100;
	else if (gameOpts->app.bExpertMode) {
		sprintf(szKeyRampScale + 1, TXT_KBD_RAMP, gameOpts->input.keyRampScale, HTX_CONF_KBDRAMP);
		*szKeyRampScale = *(TXT_KBD_RAMP - 1);
		ADD_SLIDER (opt, szKeyRampScale + 1, gameOpts->input.keyRampScale / 10, 0, 10, KEY_R, HTX_CONF_RAMPSCALE);   
		nRampOpt = opt++;
		if (gameOpts->input.keyRampScale > 0) {
			nRampKeyOpt = opt;
			ADD_CHECK (opt, TXT_RAMP_ACCEL, gameOpts->input.bRampKeys [0], KEY_A, HTX_CONF_RAMPACC);
			opt++;
			ADD_CHECK (opt, TXT_RAMP_ROT, gameOpts->input.bRampKeys [1], KEY_O, HTX_CONF_RAMPROT);
			opt++;
			ADD_CHECK (opt, TXT_RAMP_SLIDE, gameOpts->input.bRampKeys [2], KEY_D, HTX_CONF_RAMPSLD);
			opt++;
			}
		}
	ADD_TEXT (opt, "", 0);
	opt++;
	if (gameOpts->app.bExpertMode && (gameStates.app.bNostalgia < 3)) {
		ADD_CHECK (opt, TXT_FASTPITCH, (extraGameInfo [0].bFastPitch == 1) ? 1 : 0, KEY_T, HTX_CONF_FASTPITCH);
		nFastPitchOpt = opt++;
		extraGameInfo [0].bFastPitch = m [nFastPitchOpt].value ? 1 : 2;
		}
	else
		nFastPitchOpt = -1;
	do {
		i = ExecMenu1 (NULL, TXT_CONTROLS, opt, m, joydef_menu_callback, &choice);
/*
		h = gameOpts->input.bSyncMouseAxes ? 1 : 3;
		for (j = 0; j < h; j++)
			gameOpts->input.mouseSensitivity [j] = m [nMouseSensOpt + j].value;
		for (j = h; j < 3; j++)
			gameOpts->input.mouseSensitivity [j] = gameOpts->input.mouseSensitivity [0];
		h = gameOpts->input.bSyncJoyAxes ? 1 : UNIQUE_JOY_AXES;
		for (j = 0; j < h; j++)
			gameOpts->input.joySensitivity [j] = m [nJoySensOpt + j].value;
		for (j = h; j < UNIQUE_JOY_AXES; j++)
			gameOpts->input.joySensitivity [j] = gameOpts->input.joySensitivity [0];
*/
		gameOpts->input.bUseMouse = m [nUseMouseOpt].value;
#ifdef RELEASE
		if (gameOpts->input.nJoysticks)
#endif
			gameOpts->input.bUseJoystick = m [nUseJoyOpt].value;
		gameOpts->input.bUseHotKeys = m [nUseHotKeysOpt].value;
		if (!gameStates.app.bNostalgia) {
			if (gameOpts->input.bUseMouse) {
				if (nMouseTypeOpt < 0) {
					gameStates.input.nMouseType = CONTROL_MOUSE;
					}
				else {
					gameOpts->input.bJoyMouse = m [nJoyMouseOpt].value;
					extraGameInfo [0].bMouseLook = m [nMouseLookOpt].value;
					for (j = 0; j < 2; j++)
						if (m [nMouseTypeOpt + j].value) {
							gameStates.input.nMouseType = CONTROL_MOUSE + j;
							break;
							}
						}
				}
#ifdef RELEASE
			if (gameOpts->input.nJoysticks && gameOpts->input.bUseJoystick) {
#else
			if (gameOpts->input.bUseJoystick) {
#endif
				if (nJoyTypeOpt < 0)
					gameStates.input.nJoyType = CONTROL_JOYSTICK;
				else {
					if (gameOpts->app.bExpertMode) {
						gameOpts->input.bLinearJoySens = m [nLinSensOpt].value;
					for (j = 0; j < UNIQUE_JOY_AXES; j++)
						if (m [nJoyTypeOpt + j].value) {
							gameStates.input.nJoyType = CONTROL_JOYSTICK + j;
							break;
							}
						}
					}
				extraGameInfo [0].bFastPitch = m [nFastPitchOpt].value ? 1 : 2;
				}
			else {
#if EXPMODE_DEFAULTS
				gameOpts->input.bLinearJoySens = 0;
				extraGameInfo [0].bFastPitch = 1;
#endif
				}
			}
/*
		for (j = 0; j < 3; j++)
			if (m [j].value) {
				gameConfig.nControlType = j;
				break;
				}
*/
		if (i == -2)
			goto rebuild_menu;
		gameConfig.nControlType = gameOpts->input.bUseMouse ? CONTROL_MOUSE : gameConfig.nControlType;
		if (i == -1)
			break;
		if (choice == nCustKbdOpt)
			KConfig (0, TXT_CFG_KBD);
		else if (choice == nCustJoyOpt)
			KConfig (1, TXT_CFG_JOY);
		else if (choice == nCustMouseOpt)
			KConfig (2, TXT_CFG_MOUSE);
#ifdef D2X_KEYS
		else if (choice == nCustHotKeysOpt)
			KConfig (4, TXT_CFG_HOTKEYS);
#endif
	} while (i >= 0);
}

//------------------------------------------------------------------------------
// eof
