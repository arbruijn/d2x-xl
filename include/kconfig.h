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
#include "text.h"
#include "menu.h"

#define	D2X_KEYS		1

typedef struct control_info {
	fix pitchTime;
	fix verticalThrustTime;
	fix headingTime;
	fix sidewaysThrustTime;
	fix bankTime;
	fix forwardThrustTime;

	uint8_t rearViewDownCount;
	uint8_t rearViewDownState;

	uint8_t firePrimaryDownCount;
	uint8_t firePrimaryState;
	uint8_t fireSecondaryState;
	uint8_t fireSecondaryDownCount;
	uint8_t fireFlareDownCount;

	uint8_t dropBombDownCount;

	uint8_t automapDownCount;
	uint8_t automapState;

	//CAngleVector heading;
	//char oem_message [64];

	uint8_t afterburnerState;
	uint8_t cyclePrimaryCount;
	uint8_t cycleSecondaryCount;
	uint8_t headlightCount;
	uint8_t toggleIconsCount;
	uint8_t zoomDownCount;
	uint8_t useCloakDownCount;
	uint8_t useInvulDownCount;
	uint8_t slowMotionCount;
	uint8_t bulletTimeCount;
} __pack__ tControlInfo;

typedef struct ext_control_info {
	fix pitchTime;
	fix verticalThrustTime;
	fix headingTime;
	fix sidewaysThrustTime;
	fix bankTime;
	fix forwardThrustTime;

	uint8_t rearViewDownCount;
	uint8_t rearViewDownState;

	uint8_t firePrimaryDownCount;
	uint8_t firePrimaryState;
	uint8_t fireSecondaryState;
	uint8_t fireSecondaryDownCount;
	uint8_t fireFlareDownCount;

	uint8_t dropBombDownCount;

	uint8_t automapDownCount;
	uint8_t automapState;

	//CAngleVector heading;   	    // for version >=1.0
	//char oem_message [64];     	// for version >=1.0

	//CFixVector ship_pos           // for version >=2.0
	//CFixMatrix ship_orient        // for version >=2.0

	//uint8_t cyclePrimaryCount     // for version >=3.0
	//uint8_t cycleSecondaryCount   // for version >=3.0
	//uint8_t afterburnerState       // for version >=3.0
	//uint8_t headlightCount         // for version >=3.0

	// everything below this line is for version >=4.0

	//uint8_t headlightState

	//int32_t primaryWeaponFlags
	//int32_t secondaryWeaponFlags
	//uint8_t Primary_weapon_selected
	//uint8_t Secondary_weapon_selected

	//CFixVector force_vector
	//CFixMatrix force_matrix
	//int32_t joltinfo[3]
	//int32_t x_vibrate_info[2]
	//int32_t y_vibrate_info[2]

	//int32_t x_vibrate_clear
	//int32_t y_vibrate_clear

	//uint8_t gameStatus;

	//uint8_t keyboard[128];          // scan code array, not ascii
	//uint8_t current_guidebot_command;

	//uint8_t Reactor_blown

} ext_control_info;

typedef struct advanced_ext_control_info {
	fix pitchTime;
	fix verticalThrustTime;
	fix headingTime;
	fix sidewaysThrustTime;
	fix bankTime;
	fix forwardThrustTime;

	uint8_t rearViewDownCount;
	uint8_t rearViewDownState;

	uint8_t firePrimaryDownCount;
	uint8_t firePrimaryState;
	uint8_t fireSecondaryState;
	uint8_t fireSecondaryDownCount;
	uint8_t fireFlareDownCount;

	uint8_t dropBombDownCount;

	uint8_t automapDownCount;
	uint8_t automapState;

	// everything below this line is for version >=1.0

	CAngleVector heading;
	char oem_message [64];

	// everything below this line is for version >=2.0

	CFixVector ship_pos;
	CFixMatrix ship_orient;

	// everything below this line is for version >=3.0

	uint8_t cyclePrimaryCount;
	uint8_t cycleSecondaryCount;
	uint8_t afterburnerState;
	uint8_t headlightCount;

	// everything below this line is for version >=4.0

	int32_t primaryWeaponFlags;
	int32_t secondaryWeaponFlags;
	uint8_t currentPrimary_weapon;
	uint8_t currentSecondary_weapon;

	CFixVector force_vector;
	CFixMatrix force_matrix;
	int32_t joltinfo[3];
	int32_t x_vibrate_info[2];
	int32_t y_vibrate_info[2];

	int32_t x_vibrate_clear;
	int32_t y_vibrate_clear;

	uint8_t gameStatus;

	uint8_t headlightState;
	uint8_t current_guidebot_command;

	uint8_t keyboard[128];    // scan code array, not ascii

	uint8_t Reactor_blown;

} advanced_ext_control_info;


class CExternalControls {
	public:
		uint8_t					m_bUse;
		uint8_t					m_bEnable;
		uint8_t					m_intno;
		ext_control_info*	m_info;
		char*					m_name;
		uint8_t					m_version;

	public:
		CExternalControls () { memset (this, 0, sizeof (*this)); }
		~CExternalControls () { Destroy (); }
		void Init (int32_t intno, int32_t address);
		void Destroy (void);
		void Read (void);
};

extern CExternalControls externalControls;


typedef struct kcItem {
	int16_t id;				// The id of this item
	int16_t x, y;
	int16_t w1;
	int16_t w2;
	int16_t u,d,l,r;
        //int16_t text_num1;
   const char 	*text;
	int32_t   textId;
	uint8_t nType;
	uint8_t value;		// what key,button,etc
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
	uint8_t		custom [CONTROL_MAX_TYPES][MAX_CONTROLS];
	uint8_t		defaults [CONTROL_MAX_TYPES][MAX_CONTROLS];
	uint8_t		d2xCustom [MAX_HOTKEY_CONTROLS];
	uint8_t		d2xDefaults [MAX_HOTKEY_CONTROLS];
} __pack__ tControlSettings;

extern tControlSettings controlSettings;

extern char *control_text[CONTROL_MAX_TYPES];

void KCSetControls (int32_t bGet);

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero

extern int32_t kconfig_is_axes_used(int32_t axis);

extern uint8_t nExtGameStatus;

extern uint8_t system_keys [];

extern tControlSettings controlSettings;
extern kcItem kcKeyboard []; //NUM_KEY_CONTROLS];
extern uint8_t kc_kbdFlags []; //NUM_KEY_CONTROLS];
extern kcItem kcJoystick []; //NUM_JOY_CONTROLS];
extern kcItem kcMouse [];	//NUM_MOUSE_CONTROLS];
extern kcItem kcHotkeys [];	//NUM_HOTKEY_CONTROLS];
extern kcItem kcSuperJoy []; //NUM_JOY_CONTROLS];

#define BT_NONE				-1
#define BT_KEY 				0
#define BT_MOUSE_BUTTON 	1
#define BT_MOUSE_AXIS		2
#define BT_JOY_BUTTON 		3
#define BT_JOY_AXIS			4
#define BT_INVERT				5

int32_t KcKeyboardSize (void);
int32_t KcMouseSize (void);
int32_t KcJoystickSize (void);
int32_t KcSuperJoySize (void);
int32_t KcHotkeySize (void);

//------------------------------------------------------------------------------

#include "joy.h"

class CControlConfig : public CMenu {
	public:
		void Run (int32_t nType, const char* pszTitle);

	private:
		typedef uint8_t kc_ctrlfuncType (int32_t);
		typedef kc_ctrlfuncType *kc_ctrlfunc_ptr;

		typedef struct tItemPos {
			int32_t	i, l, r, y;
			} tItemPos;

	private:
		kcItem*		m_items;
		int32_t		m_nItems;
		int32_t		m_nCurItem, m_nPrevItem;
		int32_t		m_nChangeMode, m_nPrevChangeMode;
		int32_t		m_nMouseState, m_nPrevMouseState;
		int32_t		m_bTimeStopped;
		int32_t		m_xOffs, m_yOffs;
		int32_t		m_closeX, m_closeY, m_closeSize;
		int32_t		m_nChangeState;
		const char*	m_pszTitle;

		static int32_t	m_startAxis [JOY_MAX_AXES];

	private:
		void Edit (kcItem* items, int32_t nItems);
		int32_t HandleControl (void);
		int32_t HandleInput (void);
		void Quit (void);


		const char* MouseButtonText (int32_t i);
		const char* MouseAxisText (int32_t i);
		const char* YesNoText (int32_t i);

		int32_t IsAxisUsed (int32_t axis);
		int32_t GetItemHeight (kcItem *item);
		void DrawTitle (void);
		void DrawHeader (void);
		void DrawTable (void);
		void DrawQuestion (kcItem *item);
		void DrawItem (kcItem* item, int32_t bIsCurrent, int32_t bRedraw);
		inline void DrawItem (kcItem *item, int32_t bIsCurrent) { DrawItem (item, bIsCurrent, 1); }

		void ReadFCS (int32_t raw_axis);

		int32_t AssignControl (kcItem *item, int32_t nType, uint8_t code);
		static uint8_t KeyCtrlFunc (int32_t nChangeState);
		static uint8_t JoyBtnCtrlFunc (int32_t nChangeState);
		static uint8_t MouseBtnCtrlFunc (int32_t nChangeState);
		static uint8_t JoyAxisCtrlFunc (int32_t nChangeState);
		static uint8_t MouseAxisCtrlFunc (int32_t nChangeState);
		int32_t ChangeControl (kcItem *item, int32_t nType, kc_ctrlfunc_ptr ctrlfunc, const char *pszMsg);

		inline int32_t ChangeKey (kcItem *item) { return ChangeControl (item, BT_KEY, &CControlConfig::KeyCtrlFunc, TXT_PRESS_NEW_KEY); }
		inline int32_t ChangeJoyButton (kcItem *item) { return ChangeControl (item, BT_JOY_BUTTON, &CControlConfig::JoyBtnCtrlFunc, TXT_PRESS_NEW_JBUTTON); }
		inline int32_t ChangeMouseButton (kcItem * item) { return ChangeControl (item, BT_MOUSE_BUTTON, &CControlConfig::MouseBtnCtrlFunc, TXT_PRESS_NEW_MBUTTON); }
		inline int32_t ChangeJoyAxis (kcItem *item) { return ChangeControl (item, BT_JOY_AXIS, &CControlConfig::JoyAxisCtrlFunc, TXT_MOVE_NEW_JOY_AXIS); }
		inline int32_t ChangeMouseAxis (kcItem * item) { return ChangeControl (item, BT_MOUSE_AXIS, &CControlConfig::MouseAxisCtrlFunc, TXT_MOVE_NEW_MSE_AXIS); }
		int32_t ChangeInvert (kcItem * item);

		void QSortItemPos (tItemPos* pos, int32_t left, int32_t right);
		tItemPos* GetItemPos (kcItem* items, int32_t nItems);
		int32_t* GetItemRef (int32_t nItems, tItemPos* pos);
		int32_t FindItemAt (int32_t x, int32_t y);
		int32_t FindNextItemLeft (int32_t nItems, int32_t nCurItem, tItemPos *pos, int32_t *ref);
		int32_t FindNextItemRight (int32_t nItems, int32_t nCurItem, tItemPos *pos, int32_t *ref);
		int32_t FindNextItemUp (int32_t nItems, int32_t nCurItem, tItemPos *pos, int32_t *ref);
		int32_t FindNextItemDown (int32_t nItems, int32_t nCurItem, tItemPos *pos, int32_t *ref);
		void LinkKbdEntries (void);
		void LinkJoyEntries (void);
		void LinkMouseEntries (void);
		void LinkHotkeyEntries (void);
		void LinkTableEntries (int32_t tableFlags);

	public:
		virtual void Render (void);
};

extern CControlConfig controlConfig;

//------------------------------------------------------------------------------


#endif /* _KCONFIG_H */
