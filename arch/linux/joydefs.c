/* $Id: joydefs.c,v 1.2 2003/03/13 00:20:21 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Linux joystick support
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
//#include "victor.h"
#include "render.h"
#include "palette.h"
#include "newmenu.h"
#include "args.h"
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"

#include "joystick.h"

int joydefs_calibrate_flag = 0;

//added 9/6/98 Matt Mueller - not needed at all in linux code but bunches 
int Joy_is_Sidewinder=0;//    of main/* stuff uses it
//end addition

void joy_delay()
{
	//int t1 = TICKER + 19/4;			// Wait 1/4 second...
	//StopTime();
	//while( TICKER < t1 );
	//joy_flush();
	//StartTime();
}


int joycal_message( char * title, char * text )
{
	int i;
	newmenu_item	m[2];
	memset (m, 0, sizeof (m));
	m[0].type = NM_TYPE_TEXT; 
	m[0].text = text;
	m[1].type = NM_TYPE_MENU; 
	m[1].text = TXT_OK;
	i = ExecMenu( title, NULL, 2, m, NULL );
	if ( i < 0 ) 
		return 1;
	return 0;
}

extern int WriteConfigFile();	

extern joystick_device j_joystick[4];
extern joystick_axis j_axis[MAX_AXES];
extern joystick_button j_button[MAX_BUTTONS];

void joydefs_calibrate()
{

	int i;
	int temp_values[MAX_AXES];
	char title[50];
	char text[256];
//added/killed on 10/17/98 by Hans de Goede for joystick/mouse # fix
//-killed-        int nsticks = 0;
//end this section kill - Hans

	joydefs_calibrate_flag = 0;

	if (!joy_present)	{
		ExecMessageBox( NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}

	if (j_joystick[0].version) {
		joycal_message ("No Calibration", "calibration not required for\njoystick v1.x");
		return;
	}
	
	for (i = 0; i < j_num_axes; i += 2) {
		sprintf (title, "js%d Calibration", j_axis[i].joydev);

		sprintf (text, "center joystick %d", j_axis[i].joydev);
		joycal_message (title, text);
		joystick_read_raw_axis (JOY_ALL_AXIS, temp_values);
		j_axis[i].center_val = temp_values[i];
		j_axis[i + 1].center_val = temp_values[i + 1];

		sprintf (text, "move joystick %d to the upper left", j_axis[i].joydev);
		joycal_message (title, text);
		joystick_read_raw_axis (JOY_ALL_AXIS, temp_values);
		j_axis[i].min_val = temp_values[i];
		j_axis[i + 1].min_val = temp_values[i + 1];

		sprintf (text, "move joystick %d to the lower right", j_axis[i].joydev);
		joycal_message (title, text);
		joystick_read_raw_axis (JOY_ALL_AXIS, temp_values);
		j_axis[i].max_val = temp_values[i];
		j_axis[i + 1].max_val = temp_values[i + 1];

	}
	
	WriteConfigFile ();
}


//char *control_text[CONTROL_MAX_TYPES] = { "Keyboard only", "Joystick", "Flightstick Pro", "Thrustmaster FCS", "Gravis Gamepad", "Mouse", "Cyberman" };

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int oc_type = gameConfig.nControlType;

	nitems = nitems;
	last_key = last_key;
	citem = citem;		

	for (i=0; i<3; i++ )
		if (items[i].value) gameConfig.nControlType = i;

//added on 10/17/98 by Hans de Goede for joystick/mouse # fix
       // remap mouse, since "Flightstick Pro", "Thrustmaster FCS"
       //   and "Gravis Gamepad" where removed from the options
        if (gameConfig.nControlType == 2) gameConfig.nControlType = CONTROL_MOUSE;
//end this section addition - Hans

	if ( (oc_type != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS ) )	{
		ExecMessageBox( TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS );
	}

	if (oc_type != gameConfig.nControlType) {
		switch (gameConfig.nControlType) {
	//		case	CONTROL_NONE:
			case	CONTROL_JOYSTICK:
			case	CONTROL_FLIGHTSTICK_PRO:
			case	CONTROL_THRUSTMASTER_FCS:
			case	CONTROL_GRAVIS_GAMEPAD:
	//		case	CONTROL_MOUSE:
	//		case	CONTROL_CYBERMAN:
				joydefs_calibrate_flag = 1;
		}
		KCSetControls();
	}

}

extern ubyte kc_use_external_control;
extern ubyte kc_enable_external_control;
extern ubyte *kc_external_name;

void joydefs_config()
{
//added/changed/killed on 10/17/98 by Hans de Goede for joystick/mouse # fix
//-killed-        char xtext[128];
//-killed-        int i, old_masks, masks;
        newmenu_item m[13];
//-killed-        int i1=5;
//-killed-        int nitems;
//-killed-
//-killed-        do {
//-killed-                nitems = 6;
        int i, i1=5, j, nitems=7;
//end this section kill/change - Hans

            m[0].type = NM_TYPE_RADIO; m[0].text = "KEYBOARD"; m[0].value = 0; m[0].group = 0; m [0].key = KEY_K;
            m[1].type = NM_TYPE_RADIO; m[1].text = "JOYSTICK"; m[1].value = 0; m[1].group = 0; m [1].key = KEY_J;
            m[2].type = NM_TYPE_RADIO; m[2].text = "MOUSE"; m[2].value = 0; m[2].group = 0; m [2].key = KEY_M;
            m[3].type = NM_TYPE_TEXT; m[3].text=""; 
            m[4].type = NM_TYPE_MENU; m[4].text="CUSTOMIZE ABOVE"; m [4].key = KEY_A;
            m[5].type = NM_TYPE_MENU; m[5].text="CUSTOMIZE KEYBOARD"; m [5].key = KEY_C;
//added on 2/5/99 by Victor Rachels for D1X keys menu
            m[6].type = NM_TYPE_MENU; m[6].text="CUSTOMIZE D1X KEYS"; m [6].key = KEY_X;
//end this section addition - VR

//added/changed/killed on 10/17/98 by Hans de Goede for joystick/mouse # fix
//-killed-                m[gameConfig.nControlType].value = 1;

            do {


              i = gameConfig.nControlType;
              if(i==CONTROL_MOUSE) i = 2;
              m[i].value=1;
//end section - OE
//end this section change/addition - Hans

		i1 = ExecMenu1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );

//added 6-15-99 Owen Evans
		for (j = 0; j <= 2; j++)
			if (m[j].value)
				gameConfig.nControlType = j;
		i = gameConfig.nControlType;
		if (gameConfig.nControlType == 2)
			gameConfig.nControlType = CONTROL_MOUSE;
//end added - OE

		switch(i1)	{
			case 4: 
//added/changed on 10/17/98 by Hans de Goede for joystick/mouse # fix
//-killed-                                KConfig(gameConfig.nControlType, m[gameConfig.nControlType].text);
                                KConfig (i, m[i].text);
//end this section change - Hans
                                break;
			case 5: 
				KConfig(0, "KEYBOARD"); 
				break;
//added on 2/5/99 by Victor Rachels for D1X keys menu
                        case 6:
                                KConfig(3, "D1X KEYS");
                                break;
//end this section addition - VR
		} 

	} while(i1>-1);

}
