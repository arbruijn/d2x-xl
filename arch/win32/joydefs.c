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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include "render.h"
#include "palette.h"
#include "newmenu.h"
#include "args.h"
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"

int joydefs_calibrateFlag=0;

int Joy_is_Sidewinder=0; //not needed, but lots of main/* stuff use it

void joy_delay()
{

}

int joycal_message( char * title, char * text )
{
	int i;
	newmenu_item	m[2];
	m[0].nType = NM_TYPE_TEXT; m[0].text = text;
	m[1].nType = NM_TYPE_MENU; m[1].text = TXT_OK;
	i = ExecMenu( title, NULL, 2, m, NULL );
	if ( i < 0 ) 
		return 1;
	return 0;
}

void joydefs_calibrate()
{
  joycal_message("No Calibration", "calibration should be performed\nthrough windows");
  return;
}

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int ocType = gameConfig.nControlType;

	nitems = nitems;
	last_key = last_key;
	citem = citem;		

	for (i=0; i<3; i++ )
		if (items[i].value) gameConfig.nControlType = i;

        if (gameConfig.nControlType == 2) gameConfig.nControlType = CONTROL_MOUSE;

	if ( (ocType != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS ) )	{
		ExecMessageBox( TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS );
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

}

extern ubyte kc_use_external_control;
extern ubyte kc_enable_external_control;
extern ubyte *kc_external_name;

void joydefs_config()
{
        newmenu_item m[13];
        int i, j, i1=0, nitems=7;

            m[0].nType = NM_TYPE_RADIO;
            m[0].text = "KEYBOARD"; m[0].value = 0; m[0].group = 0; m [0].key = KEY_K;
            m[1].nType = NM_TYPE_RADIO; m[1].text = "JOYSTICK"; m[1].value = 0; m[1].group = 0; m [1].key = KEY_J;
            m[2].nType = NM_TYPE_RADIO; m[2].text = "MOUSE"; m[2].value = 0; m[2].group = 0; m [2].key = KEY_M;
            m[3].nType = NM_TYPE_TEXT; m[3].text="";
            m[4].nType = NM_TYPE_MENU; m[4].text="CUSTOMIZE ABOVE";  m [4].key = KEY_A;
            m[5].nType = NM_TYPE_MENU; m[5].text="CUSTOMIZE KEYBOARD"; m [5].key = KEY_C;
            m[6].nType = NM_TYPE_MENU; m[6].text="CUSTOMIZE D1X KEYS"; m [6].key = KEY_X;

            do {
                i = gameConfig.nControlType;
                 if(i==CONTROL_MOUSE) i = 2;
                  m[i].value=1;
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
                                KConfig (i, m[i].text);
                                break;
			case 5: 
				KConfig(0, "KEYBOARD"); 
				break;
                        case 6:
                                KConfig(3, "D1X KEYS");
                                break;
		} 
	} while(i1>-1);
}
