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

#ifndef _KCONFIG_H
#define _KCONFIG_H

#include "config.h"
#include "gamestat.h"

#define	D2X_KEYS		1

typedef struct control_info {
	fix pitchTime;
	fix verticalThrustTime;
	fix headingTime;
	fix sidewaysThrustTime;
	fix bankTime;
	fix forwardThrustTime;

	ubyte rearViewDownCount;
	ubyte rearViewDownState;

	ubyte firePrimaryDownCount;
	ubyte firePrimaryState;
	ubyte fireSecondaryState;
	ubyte fireSecondaryDownCount;
	ubyte fireFlareDownCount;

	ubyte dropBombDownCount;

	ubyte automapDownCount;
	ubyte automapState;

	//vmsAngVec heading;
	//char oem_message[64];

	ubyte afterburnerState;
	ubyte cyclePrimaryCount;
	ubyte cycleSecondaryCount;
	ubyte headlightCount;
	ubyte toggleIconsCount;
	ubyte zoomDownCount;
	ubyte useCloakDownCount;
	ubyte useInvulDownCount;
	ubyte slowMotionCount;
	ubyte bulletTimeCount;
} tControlInfo;

typedef struct ext_control_info {
	fix pitchTime;
	fix verticalThrustTime;
	fix headingTime;
	fix sidewaysThrustTime;
	fix bankTime;
	fix forwardThrustTime;

	ubyte rearViewDownCount;
	ubyte rearViewDownState;

	ubyte firePrimaryDownCount;
	ubyte firePrimaryState;
	ubyte fireSecondaryState;
	ubyte fireSecondaryDownCount;
	ubyte fireFlareDownCount;

	ubyte dropBombDownCount;

	ubyte automapDownCount;
	ubyte automapState;

	//vmsAngVec heading;   	    // for version >=1.0
	//char oem_message[64];     	// for version >=1.0

	//vmsVector ship_pos           // for version >=2.0
	//vmsMatrix ship_orient        // for version >=2.0

	//ubyte cyclePrimaryCount     // for version >=3.0
	//ubyte cycleSecondaryCount   // for version >=3.0
	//ubyte afterburnerState       // for version >=3.0
	//ubyte headlightCount         // for version >=3.0

	// everything below this line is for version >=4.0

	//ubyte headlightState

	//int primaryWeaponFlags
	//int secondaryWeaponFlags
	//ubyte Primary_weapon_selected
	//ubyte Secondary_weapon_selected

	//vmsVector force_vector
	//vmsMatrix force_matrix
	//int joltinfo[3]
	//int x_vibrate_info[2]
	//int y_vibrate_info[2]

	//int x_vibrate_clear
	//int y_vibrate_clear

	//ubyte gameStatus;

	//ubyte keyboard[128];          // scan code array, not ascii
	//ubyte current_guidebot_command;

	//ubyte Reactor_blown

} ext_control_info;

typedef struct advanced_ext_control_info {
	fix pitchTime;
	fix verticalThrustTime;
	fix headingTime;
	fix sidewaysThrustTime;
	fix bankTime;
	fix forwardThrustTime;

	ubyte rearViewDownCount;
	ubyte rearViewDownState;

	ubyte firePrimaryDownCount;
	ubyte firePrimaryState;
	ubyte fireSecondaryState;
	ubyte fireSecondaryDownCount;
	ubyte fireFlareDownCount;

	ubyte dropBombDownCount;

	ubyte automapDownCount;
	ubyte automapState;

	// everything below this line is for version >=1.0

	vmsAngVec heading;
	char oem_message[64];

	// everything below this line is for version >=2.0

	vmsVector ship_pos;
	vmsMatrix ship_orient;

	// everything below this line is for version >=3.0

	ubyte cyclePrimaryCount;
	ubyte cycleSecondaryCount;
	ubyte afterburnerState;
	ubyte headlightCount;

	// everything below this line is for version >=4.0

	int primaryWeaponFlags;
	int secondaryWeaponFlags;
	ubyte currentPrimary_weapon;
	ubyte currentSecondary_weapon;

	vmsVector force_vector;
	vmsMatrix force_matrix;
	int joltinfo[3];
	int x_vibrate_info[2];
	int y_vibrate_info[2];

	int x_vibrate_clear;
	int y_vibrate_clear;

	ubyte gameStatus;

	ubyte headlightState;
	ubyte current_guidebot_command;

	ubyte keyboard[128];    // scan code array, not ascii

	ubyte Reactor_blown;

} advanced_ext_control_info;


typedef struct kcItem {
	short id;				// The id of this item
	short x, y;			
	short w1;
	short w2;
	short u,d,l,r;
        //short text_num1;
   const char 	*text;
	int   textId;
	ubyte nType;
	ubyte value;		// what key,button,etc
} kcItem;


// added on 2/4/99 by Victor Rachels to add new keys menu
#define NUM_HOTKEY_CONTROLS	KcHotkeySize () //20
#define MAX_HOTKEY_CONTROLS	40

#define NUM_KEY_CONTROLS		KcKeyboardSize () //64
#define NUM_JOY_CONTROLS		KcJoystickSize () //66
#define NUM_SUPERJOY_CONTROLS	KcSuperJoySize () //66
#define NUM_MOUSE_CONTROLS		KcMouseSize () //31
#define MAX_CONTROLS				64		// there are actually 48, so this leaves room for more

typedef struct tControlSettings {
	ubyte		custom [CONTROL_MAX_TYPES][MAX_CONTROLS];
	ubyte		defaults [CONTROL_MAX_TYPES][MAX_CONTROLS];
	ubyte		d2xCustom [MAX_HOTKEY_CONTROLS];
	ubyte		d2xDefaults [MAX_HOTKEY_CONTROLS];
} tControlSettings;

extern tControlSettings controlSettings;

extern char *control_text[CONTROL_MAX_TYPES];

void KCSetControls (int bGet);

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero

extern int kconfig_is_axes_used(int axis);

extern void KCInitExternalControls(int intno, int address);

extern ubyte nExtGameStatus;
void KConfig(int n, const char *pszTitle);
void SetControlType (void);

extern ubyte system_keys [];

extern tControlSettings controlSettings;
extern kcItem kcKeyboard []; //NUM_KEY_CONTROLS];
extern ubyte kc_kbdFlags []; //NUM_KEY_CONTROLS];
extern kcItem kcJoystick []; //NUM_JOY_CONTROLS];
extern kcItem kcMouse [];	//NUM_MOUSE_CONTROLS];
extern kcItem kcHotkeys [];	//NUM_HOTKEY_CONTROLS];
extern kcItem kcSuperJoy []; //NUM_JOY_CONTROLS];
extern ext_control_info	*kc_external_control;

#define BT_NONE				-1
#define BT_KEY 				0
#define BT_MOUSE_BUTTON 	1
#define BT_MOUSE_AXIS		2
#define BT_JOY_BUTTON 		3
#define BT_JOY_AXIS			4
#define BT_INVERT				5

int KcKeyboardSize (void);
int KcMouseSize (void);
int KcJoystickSize (void);
int KcSuperJoySize (void);
int KcHotkeySize (void);

#endif /* _KCONFIG_H */
