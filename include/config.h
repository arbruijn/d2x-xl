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

/*
 *
 * prototype definitions for descent.cfg reading/writing
 *
 *
 */

#ifndef _CONFIG_H
#define _CONFIG_H

#include "mission.h"
#include "player.h"

extern int32_t ReadConfigFile(void);
extern int32_t WriteConfigFile(bool bExitProgram = false);

typedef struct tGameConfig {
	char		szLastPlayer [CALLSIGN_LEN+1];
	char		szLastMission [MISSION_NAME_LEN+1];
	int32_t		nDigiType;
	int32_t		nDigiDMA;
	int32_t		nMidiType;
	uint8_t		nAudioVolume [2];
	uint8_t		nMidiVolume;
	uint8_t		nRedbookVolume;
	uint8_t		bReverseChannels;
	uint8_t		nControlType;
	int32_t		vrType;
	int32_t		vrResolution;
	int32_t		vrTracking;
	uint32_t		nVersion;
	int32_t		nTotalTime;
} __pack__ tGameConfig;

extern tGameConfig gameConfig;

//values for Config_controlType
#define CONTROL_NONE 0
#define CONTROL_JOYSTICK 1
#define CONTROL_FLIGHTSTICK_PRO 2
#define CONTROL_THRUSTMASTER_FCS 3
#define CONTROL_GRAVIS_GAMEPAD 4
#define CONTROL_MOUSE 5
#define CONTROL_CYBERMAN 6
#define CONTROL_WINJOYSTICK 7

#define CONTROL_MAX_TYPES 8

void InitGameConfig (void);
void SetNostalgia (int32_t nLevel);

#endif

//eof
