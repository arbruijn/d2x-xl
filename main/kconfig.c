/* $Id: KConfig.c,v 1.27 2003/12/18 11:24:04 btb Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING, AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE EVE.  ALL RIGHTS RESERVED.
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: KConfig.c,v 1.27 2003/12/18 11:24:04 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include "error.h"
#include "inferno.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "event.h"
#include "joy.h"
#include "mouse.h"
#include "kconfig.h"
#include "gauges.h"
#include "joydefs.h"
#include "songs.h"
#include "render.h"
#include "digi.h"
#include "newmenu.h"
#include "endlevel.h"
#include "multi.h"
#include "timer.h"
#include "text.h"
#include "player.h"
#include "menu.h"
#include "automap.h"
#include "args.h"
#include "lighting.h"
#include "ai.h"
#include "cntrlcen.h"
#include "network.h"
#include "hudmsg.h"
#include "ogl_init.h"
#include "object.h"
#include "inferno.h"
#include "input.h"
#include "playsave.h"
#if defined (TACTILE)
 #include "tactile.h"
#endif
#include "collide.h"

#ifdef USE_LINUX_JOY
#include "joystick.h"
#endif

#ifdef D2X_KEYS
//added/removed by Victor Rachels for adding rebindable keys for these
// KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0
ubyte system_keys [] = { (ubyte) KEY_ESC, (ubyte) KEY_F1, (ubyte) KEY_F2, (ubyte) KEY_F3, (ubyte) KEY_F4, (ubyte) KEY_F5, (ubyte) KEY_F6, (ubyte) KEY_F7, (ubyte) KEY_F8, (ubyte) KEY_F9, (ubyte) KEY_F10, (ubyte) KEY_F11, (ubyte) KEY_F12, (ubyte) KEY_MINUS, (ubyte) KEY_EQUAL, (ubyte) KEY_ALTED+KEY_F9 };
#else
ubyte system_keys [] = { (ubyte) KEY_ESC, (ubyte) KEY_F1, (ubyte) KEY_F2, (ubyte) KEY_F3, (ubyte) KEY_F4, (ubyte) KEY_F5, (ubyte) KEY_F6, (ubyte) KEY_F7, (ubyte) KEY_F8, (ubyte) KEY_F9, (ubyte) KEY_F10, (ubyte) KEY_F11, (ubyte) KEY_F12, (ubyte) KEY_0, (ubyte) KEY_1, (ubyte) KEY_2, (ubyte) KEY_3, (ubyte) KEY_4, (ubyte) KEY_5, (ubyte) KEY_6, (ubyte) KEY_7, (ubyte) KEY_8, (ubyte) KEY_9, (ubyte) KEY_0, (ubyte) KEY_MINUS, (ubyte) KEY_EQUAL, (ubyte) KEY_ALTED+KEY_F9 };
#endif

#define TABLE_CREATION 0

// Array used to 'blink' the cursor while waiting for a keypress.
sbyte fades [64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

//char * invert_text [2] = { "N", "Y" };
//char * joybutton_text [28] = { "TRIG", "BTN 1", "BTN 2", "BTN 3", "BTN 4", "", "LEFT", "HAT ", "RIGHT", "", "", "HAT ", "MID", "", "", "HAT ", "", "", "", "HAT ", "TRIG", "LEFT", "RIGHT", "", "UP","DOWN","LEFT", "RIGHT" };
//char * JOYAXIS_TEXT [4] = { "X1", "Y1", "X2", "Y2" };
//char * mouseaxis_text [2] = { "L/R", "F/B" };
//char * mousebutton_text [3] = { "Left", "Right", "Mid" };

int invert_text [2] = { TNUM_N, TNUM_Y };

#ifndef USE_LINUX_JOY
	int joybutton_text [28] = 
	{ TNUM_BTN_1, TNUM_BTN_2, TNUM_BTN_3, TNUM_BTN_4,
	  -1, TNUM_TRIG, TNUM_LEFT, TNUM_HAT_L,
	 TNUM_RIGHT, -1, TNUM_HAT2_D, TNUM_HAT_R,
	 TNUM_MID, -1, TNUM_HAT2_R, TNUM_HAT_U,
	 TNUM_HAT2_L, -1, TNUM_HAT2_U, TNUM_HAT_D,
	 TNUM_TRIG, TNUM_LEFT, TNUM_RIGHT, -1, 
	 TNUM_UP, TNUM_DOWN, TNUM_LEFT, TNUM_RIGHT };

	int joyaxis_text [7] = { TNUM_X1, TNUM_Y1, TNUM_Z1, TNUM_R1, TNUM_P1,TNUM_R1,TNUM_YA1 };
//	int JOYAXIS_TEXT [4] = { TNUM_X1, TNUM_Y1, TNUM_X2, TNUM_Y2 };
#endif

#define JOYAXIS_TEXT (v)		joyaxis_text [ (v) % MAX_AXES_PER_JOYSTICK]
#define JOYBUTTON_TEXT (v)	joybutton_text [ (v) % MAX_BUTTONS_PER_JOYSTICK]

int mouseaxis_text [3] = { TNUM_L_R, TNUM_F_B, TNUM_Z1 };
int mousebutton_text [3] = { TNUM_LEFT, TNUM_RIGHT, TNUM_MID };
char * mousebutton_textra [13] = { "MW UP", "MW DN", "M6", "M7", "M8", "M9", "M10","M11","M12","M13","M14","M15","M16" };//text for buttons above 3. -MPM

// macros for drawing lo/hi res KConfig screens (see scores.c as well)

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

char *btype_text [] = { "BT_KEY", "BT_MOUSE_BUTTON", "BT_MOUSE_AXIS", "BT_JOY_BUTTON", "BT_JOY_AXIS", "BT_INVERT" };

#define INFO_Y 28

int Num_items=28;
kcItem *All_items;

//----------- WARNING!!!!!!! -------------------------------------------
// THESE NEXT FOUR BLOCKS OF DATA ARE GENERATED BY PRESSING DEL+F12 WHEN
// IN THE KEYBOARD CONFIG SCREEN.  BASICALLY, THAT PROCEDURE MODIFIES THE
// U,D,L,R FIELDS OF THE ARRAYS AND DUMPS THE NEW ARRAYS INTO KCONFIG.COD
//-------------------------------------------------------------------------

tControlSettings controlSettings = {
	{
	{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x1f,0xff,0x33,0xff,0x34,0xff,0x23,0xff,0x14,0xff,0xff,0xff,0x0,0x0},
	{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0},
	{0x5,0xc,0xff,0xff,0xff,0xff,0x7,0xf,0x13,0xb,0xff,0x6,0x8,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0},
	{0x0,0x1,0xff,0xff,0x2,0xff,0x7,0xf,0x13,0xb,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
	{0x3,0x0,0x1,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x1,0xff,0xff,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0},
	},
	{
	{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x1f,0xff,0x33,0xff,0x34,0xff,0x23,0xff,0x14,0xff,0xff,0xff,0x0,0x0},
	{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0},
	{0x5,0xc,0xff,0xff,0xff,0xff,0x7,0xf,0x13,0xb,0xff,0x6,0x8,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0},
	{0x0,0x1,0xff,0xff,0x2,0xff,0x7,0xf,0x13,0xb,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
	{0x3,0x0,0x1,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
	{0x0,0x1,0xff,0xff,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0},
	},
	{
	0x2 ,0xff,0x3 ,0xff,0x4 ,0xff,0x5 ,0xff,0x6 ,0xff,0x7 ,0xff,0x8 ,0xff,0x9 ,
	0xff,0xa ,0xff,0xb ,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff 
	},
	{
	0x2 ,0xff,0x3 ,0xff,0x4 ,0xff,0x5 ,0xff,0x6 ,0xff,0x7 ,0xff,0x8 ,0xff,0x9 ,
	0xff,0xa ,0xff,0xb ,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff 
	}
};

kcItem kcKeyboard [] = {
	{  0, 15, 49, 71, 26, 62, 2, 63, 1,"Pitch forward", 270, BT_KEY, 255 },
	{  1, 15, 49,100, 26, 63, 3, 0, 24,"Pitch forward", 270, BT_KEY, 255 },
	{  2, 15, 57, 71, 26, 0, 4, 25, 3,"Pitch backward", 271, BT_KEY, 255 },
	{  3, 15, 57,100, 26, 1, 5, 2, 26,"Pitch backward", 271, BT_KEY, 255 },
	{  4, 15, 65, 71, 26, 2, 6, 27, 5,"Turn left", 272, BT_KEY, 255 },
	{  5, 15, 65,100, 26, 3, 7, 4, 28,"Turn left", 272, BT_KEY, 255 },
	{  6, 15, 73, 71, 26, 4, 8, 29, 7,"Turn right", 273, BT_KEY, 255 },
	{  7, 15, 73,100, 26, 5, 9, 6, 34,"Turn right", 273, BT_KEY, 255 },
	{  8, 15, 84, 71, 26, 6, 10, 35, 9,"Slide on", 274, BT_KEY, 255 },
	{  9, 15, 84,100, 26, 7, 11, 8, 36,"Slide on", 274, BT_KEY, 255 },
	{ 10, 15, 92, 71, 26, 8, 12, 37, 11,"Slide left", 275, BT_KEY, 255 },
	{ 11, 15, 92,100, 26, 9, 13, 10, 44,"Slide left", 275, BT_KEY, 255 },
	{ 12, 15,100, 71, 26, 10, 14, 45, 13,"Slide right", 276, BT_KEY, 255 },
	{ 13, 15,100,100, 26, 11, 15, 12, 30,"Slide right", 276, BT_KEY, 255 },
	{ 14, 15,108, 71, 26, 12, 16, 31, 15,"Slide up", 277, BT_KEY, 255 },
	{ 15, 15,108,100, 26, 13, 17, 14, 32,"Slide up", 277, BT_KEY, 255 },
	{ 16, 15,116, 71, 26, 14, 18, 33, 17,"Slide down", 278, BT_KEY, 255 },
	{ 17, 15,116,100, 26, 15, 19, 16, 46,"Slide down", 278, BT_KEY, 255 },
	{ 18, 15,127, 71, 26, 16, 20, 47, 19,"Bank on", 279, BT_KEY, 255 },
	{ 19, 15,127,100, 26, 17, 21, 18, 38,"Bank on", 279, BT_KEY, 255 },
	{ 20, 15,135, 71, 26, 18, 22, 39, 21,"Bank left", 280, BT_KEY, 255 },
	{ 21, 15,135,100, 26, 19, 23, 20, 40,"Bank left", 280, BT_KEY, 255 },
	{ 22, 15,143, 71, 26, 20, 48, 41, 23,"Bank right", 281, BT_KEY, 255 },
	{ 23, 15,143,100, 26, 21, 49, 22, 42,"Bank right", 281, BT_KEY, 255 },
	{ 24,158, 49, 83, 26, 59, 26, 1, 25,"Fire primary", 282, BT_KEY, 255 },
	{ 25,158, 49,112, 26, 57, 27, 24, 2,"Fire primary", 282, BT_KEY, 255 },
	{ 26,158, 57, 83, 26, 24, 28, 3, 27,"Fire secondary", 283,BT_KEY, 255 },
	{ 27,158, 57,112, 26, 25, 29, 26, 4,"Fire secondary", 283,BT_KEY, 255 },
	{ 28,158, 65, 83, 26, 26, 34, 5, 29,"Fire flare", 284,BT_KEY, 255 },
	{ 29,158, 65,112, 26, 27, 35, 28, 6,"Fire flare", 284,BT_KEY, 255 },
	{ 30,158,103, 83, 26, 44, 32, 13, 31,"Accelerate", 285,BT_KEY, 255 },
	{ 31,158,103,112, 26, 45, 33, 30, 14,"Accelerate", 285,BT_KEY, 255 },
	{ 32,158,111, 83, 26, 30, 46, 15, 33,"reverse", 286,BT_KEY, 255 },
	{ 33,158,111,112, 26, 31, 47, 32, 16,"reverse", 286,BT_KEY, 255 },
	{ 34,158, 73, 83, 26, 28, 36, 7, 35,"Drop Bomb", 287,BT_KEY, 255 },
	{ 35,158, 73,112, 26, 29, 37, 34, 8,"Drop Bomb", 287,BT_KEY, 255 },
	{ 36,158, 84, 83, 26, 34, 44, 9, 37,"Rear View", 288,BT_KEY, 255 },
	{ 37,158, 84, 112, 26, 35, 45, 36, 10,"Rear View", 288,BT_KEY, 255 },
	{ 38,158,130, 83, 26, 46, 40, 19, 39,"Cruise Faster", 289,BT_KEY, 255 },
	{ 39,158,130,112, 26, 47, 41, 38, 20,"Cruise Faster", 289,BT_KEY, 255 },
	{ 40,158,138, 83, 26, 38, 42, 21, 41,"Cruise Slower", 290,BT_KEY, 255 },
	{ 41,158,138,112, 26, 39, 43, 40, 22,"Cruise Slower", 290,BT_KEY, 255 },
	{ 42,158,146, 83, 26, 40, 54, 23, 43,"Cruise Off", 291,BT_KEY, 255 },
	{ 43,158,146,112, 26, 41, 55, 42, 48,"Cruise Off", 291,BT_KEY, 255 },
	{ 44,158, 92, 83, 26, 36, 30, 11, 45,"Automap", 292,BT_KEY, 255 },
	{ 45,158, 92,112, 26, 37, 31, 44, 12,"Automap", 292,BT_KEY, 255 },
	{ 46,158,119, 83, 26, 32, 38, 17, 47,"Afterburner", 293,BT_KEY, 255 },
	{ 47,158,119,112, 26, 33, 39, 46, 18,"Afterburner", 293,BT_KEY, 255 },
	{ 48, 15,154, 71, 26, 22, 50, 43, 49,"Cycle Primary", 294,BT_KEY, 255 },
	{ 49, 15,154,100, 26, 23, 51, 48, 54,"Cycle Primary", 294,BT_KEY, 255 },
	{ 50, 15,162, 71, 26, 48, 52, 55, 51,"Cycle Second", 295,BT_KEY, 255 },
	{ 51, 15,162,100, 26, 49, 53, 50, 56,"Cycle Second", 295,BT_KEY, 255 },
	{ 52, 15,170, 71, 26, 50, 60, 57, 53,"Zoom In", 296,BT_KEY, 255 },
	{ 53, 15,170,100, 26, 51, 61, 52, 58,"Zoom In", 296,BT_KEY, 255 },
	{ 54,158,157, 83, 26, 42, 56, 49, 55,"Headlight", 297,BT_KEY, 255 },
	{ 55,158,157,112, 26, 43, 57, 54, 50,"Headlight", 297,BT_KEY, 255 },
	{ 56,158,165, 83, 26, 54, 58, 51, 57,"Energy->Shield", 298,BT_KEY, 255 },
	{ 57,158,165,112, 26, 55, 25, 56, 52,"Energy->Shield", 298,BT_KEY, 255 },
   { 58,158,173, 83, 26, 56, 59, 53, 60,"Toggle Bomb", 299, BT_KEY,255},
   { 59,158,181, 83, 26, 58, 24, 61, 62,"Toggle Icons", 653, BT_KEY,255},
	{ 60, 15,181, 71, 26, 52, 62, 58, 61,"Use Cloak", 751,BT_KEY, 255 },
	{ 61, 15,181,100, 26, 53, 63, 60, 59,"Use Cloak", 751,BT_KEY, 255 },
	{ 62, 15,189, 71, 26, 60, 0, 59, 63,"Use Invul", 752,BT_KEY, 255 },
	{ 63, 15,189,100, 26, 61, 1, 62, 0,"Use Invul", 752,BT_KEY, 255 },
   { 64,158,189, 83, 26, 58, 24, 61, 62,"Slowmo/Speed", 910, BT_KEY,255 },
   { 65,158,189,112, 26, 58, 24, 61, 62,"Slowmo/Speed", 910, BT_KEY,255 }
};

ubyte kcKbdFlags [MAX_CONTROLS];

kcItem kcJoystick [] = {
	{  0, 15, 46, 71, 26, 15, 1, 24, 5,"Fire primary", 282, BT_JOY_BUTTON, 255 },
	{  1, 15, 54, 71, 26, 0, 4, 5, 6,"Fire secondary", 283, BT_JOY_BUTTON, 255 },
	{  2, 15, 94, 71, 26, 35, 3, 10, 11,"Accelerate", 285, BT_JOY_BUTTON, 255 },
	{  3, 15,102, 71, 26, 2, 25, 11, 12,"reverse", 286, BT_JOY_BUTTON, 255 },
	{  4, 15, 62, 71, 26, 1, 26, 6, 7,"Fire flare", 284, BT_JOY_BUTTON, 255 },
	{  5,158, 46, 71, 26, 23, 6, 0, 1,"Slide on", 274, BT_JOY_BUTTON, 255 },
	{  6,158, 54, 71, 26, 5, 7, 1, 4,"Slide left", 275, BT_JOY_BUTTON, 255 },
	{  7,158, 62, 71, 26, 6, 8, 4, 26,"Slide right", 276, BT_JOY_BUTTON, 255 },
	{  8,158, 70, 71, 26, 7, 9, 26, 34,"Slide up", 277, BT_JOY_BUTTON, 255 },
	{  9,158, 78, 71, 26, 8, 10, 34, 35,"Slide down", 278, BT_JOY_BUTTON, 255 },
	{ 10,158, 86, 71, 26, 9, 11, 35, 2,"Bank on", 279, BT_JOY_BUTTON, 255 },
	{ 11,158, 94, 71, 26, 10, 12, 2, 3,"Bank left", 280, BT_JOY_BUTTON, 255 },
	{ 12,158,102, 71, 26, 11, 28, 3, 25,"Bank right", 281, BT_JOY_BUTTON, 255 },
	{ 13, 15,162, 51, 26, 33, 15, 32, 14,"Pitch U/D", 300, BT_JOY_AXIS, 255 },
	{ 15, 15,170, 51, 26, 13, 0, 18, 16,"Turn L/R", 301, BT_JOY_AXIS, 255 },
	{ 17,158,162, 58, 26, 32, 19, 14, 18,"Slide L/R", 302, BT_JOY_AXIS, 255 },
	{ 19,158,170, 58, 26, 17, 21, 16, 20,"Slide U/D", 303, BT_JOY_AXIS, 255 },
	{ 21,158,178, 58, 26, 19, 23, 20, 22,"Bank L/R", 304, BT_JOY_AXIS, 255 },
	{ 23,158,186, 58, 26, 21, 5, 22, 24,"throttle", 305, BT_JOY_AXIS, 255 },
	{ 25, 15,110, 71, 26, 3, 27, 12, 28,"Rear View", 288, BT_JOY_BUTTON, 255 },
	{ 26, 15, 70, 71, 26, 4, 34, 7, 8,"Drop Bomb", 287, BT_JOY_BUTTON, 255 },
	{ 27, 15,118, 71, 26, 25, 30, 28, 29,"Afterburner", 293, BT_JOY_BUTTON, 255 },
	{ 28,158,110, 71, 26, 12, 29, 25, 27,"Cycle Primary", 294, BT_JOY_BUTTON, 255 },
	{ 29,158,118, 71, 26, 28, 31, 27, 30,"Cycle Secondary", 295, BT_JOY_BUTTON, 255 },
	{ 30, 15,126, 71, 26, 27, 33, 29, 31,"Headlight", 297, BT_JOY_BUTTON, 255 },
	{ 31,158,126, 71, 26, 29, 32, 30, 33,"Toggle Bomb", 299, BT_JOY_BUTTON, 255 },
	{ 32,158,134, 71, 26, 31, 18, 33, 13,"Toggle Icons", 653, BT_JOY_BUTTON, 255 },
	{ 33, 15,134, 71, 26, 30, 14, 31, 32,"Automap", 292, BT_JOY_BUTTON, 255 },
	{ 34, 15, 78, 71, 26, 26, 35, 8,  9,"Use Cloak", 751, BT_JOY_BUTTON, 255 },
	{ 35, 15, 86, 71, 26, 34, 2, 9, 10,"Use Invul", 752, BT_JOY_BUTTON, 255 },

	{ 36, 15, 46, 100, 26, 15, 1, 24, 5,"Fire primary", 282, BT_JOY_BUTTON, 255 },
	{ 37, 15, 54, 100, 26, 0, 4, 5, 6,"Fire secondary", 283, BT_JOY_BUTTON, 255 },
	{ 38, 15, 94, 100, 26, 35, 3, 10, 11,"Accelerate", 285, BT_JOY_BUTTON, 255 },
	{ 39, 15,102, 100, 26, 2, 25, 11, 12,"reverse", 286, BT_JOY_BUTTON, 255 },
	{ 40, 15, 62, 100, 26, 1, 26, 6, 7,"Fire flare", 284, BT_JOY_BUTTON, 255 },
	{ 41,158, 46, 100, 26, 23, 6, 0, 1,"Slide on", 274, BT_JOY_BUTTON, 255 },
	{ 42,158, 54, 100, 26, 5, 7, 1, 4,"Slide left", 275, BT_JOY_BUTTON, 255 },
	{ 43,158, 62, 100, 26, 6, 8, 4, 26,"Slide right", 276, BT_JOY_BUTTON, 255 },
	{ 44,158, 70, 100, 26, 7, 9, 26, 34,"Slide up", 277, BT_JOY_BUTTON, 255 },
	{ 45,158, 78, 100, 26, 8, 10, 34, 35,"Slide down", 278, BT_JOY_BUTTON, 255 },
	{ 46,158, 86, 100, 26, 9, 11, 35, 2,"Bank on", 279, BT_JOY_BUTTON, 255 },
	{ 47,158, 94, 100, 26, 10, 12, 2, 3,"Bank left", 280, BT_JOY_BUTTON, 255 },
	{ 48,158,102, 100, 26, 11, 28, 3, 25,"Bank right", 281, BT_JOY_BUTTON, 255 },
	{ 49, 15,162, 80, 26, 33, 15, 32, 14,"Pitch U/D", 300, BT_JOY_AXIS, 255 },
	{ 50, 15,170, 80, 26, 13, 0, 18, 16,"Turn L/R", 301, BT_JOY_AXIS, 255 },
	{ 51,158,162, 87, 26, 32, 19, 14, 18,"Slide L/R", 302, BT_JOY_AXIS, 255 },
	{ 52,158,170, 87, 26, 17, 21, 16, 20,"Slide U/D", 303, BT_JOY_AXIS, 255 },
	{ 53,158,178, 87, 26, 19, 23, 20, 22,"Bank L/R", 304, BT_JOY_AXIS, 255 },
	{ 54,158,186, 87, 26, 21, 5, 22, 24,"throttle", 305, BT_JOY_AXIS, 255 },
	{ 55, 15,110, 100, 26, 3, 27, 12, 28,"Rear View", 288, BT_JOY_BUTTON, 255 },
	{ 56, 15, 70, 100, 26, 4, 34, 7, 8,"Drop Bomb", 287, BT_JOY_BUTTON, 255 },
	{ 57, 15,118, 100, 26, 25, 30, 28, 29,"Afterburner", 293, BT_JOY_BUTTON, 255 },
	{ 58,158,110, 100, 26, 12, 29, 25, 27,"Cycle Primary", 294, BT_JOY_BUTTON, 255 },
	{ 59,158,118, 100, 26, 28, 31, 27, 30,"Cycle Secondary", 295, BT_JOY_BUTTON, 255 },
	{ 60, 15,126, 100, 26, 27, 33, 29, 31,"Headlight", 297, BT_JOY_BUTTON, 255 },
	{ 61,158,126, 100, 26, 29, 32, 30, 33,"Toggle Bomb", 299, BT_JOY_BUTTON, 255 },
	{ 62,158,134, 100, 26, 31, 18, 33, 13,"Toggle Icons", 653, BT_JOY_BUTTON, 255 },
	{ 63, 15,134, 100, 26, 30, 14, 31, 32,"Automap", 292, BT_JOY_BUTTON, 255 },
	{ 64, 15, 78, 100, 26, 26, 35, 8,  9,"Use Cloak", 751, BT_JOY_BUTTON, 255 },
	{ 65, 15, 86, 100, 26, 34, 2, 9, 10,"Use Invul", 752, BT_JOY_BUTTON, 255 },

	{ 14, 15,162,115, 8, 33, 16, 13, 17,"Pitch U/D", 300, BT_INVERT, 255 },
	{ 16, 15,170,115, 8, 14, 17, 15, 19,"Turn L/R", 301, BT_INVERT, 255 },
	{ 18,158,162,123, 8, 32, 20, 17, 15,"Slide L/R", 302, BT_INVERT, 255 },
	{ 20,158,170,123, 8, 18, 22, 19, 21,"Slide U/D", 303, BT_INVERT, 255 },
	{ 22,158,178,123, 8, 20, 24, 21, 23,"Bank L/R", 304, BT_INVERT, 255 },
	{ 24,158,186,123, 8, 22, 13, 23, 0,"throttle", 305, BT_INVERT, 255 }

};

kcItem kcMouse [] = {
	{  0, 25, 46, 85, 26, 23, 1, 24, 5,"Fire primary", 282, BT_MOUSE_BUTTON, 255 },
	{  1, 25, 54, 85, 26, 0, 4, 5, 6,"Fire secondary", 283, BT_MOUSE_BUTTON, 255 },
	{  2, 25, 78, 85, 26, 26, 3, 8, 9,"Accelerate", 285, BT_MOUSE_BUTTON, 255 },
	{  3, 25, 86, 85, 26, 2, 27, 9, 10,"reverse", 286, BT_MOUSE_BUTTON, 255 },
	{  4, 25, 62, 85, 26, 1, 26, 6, 7,"Fire flare", 284, BT_MOUSE_BUTTON, 255 },
	{  5,180, 46, 59, 26, 24, 6, 0, 1,"Slide on", 274, BT_MOUSE_BUTTON, 255 },
	{  6,180, 54, 59, 26, 5, 7, 1, 4,"Slide left", 275, BT_MOUSE_BUTTON, 255 },
	{  7,180, 62, 59, 26, 6, 8, 4, 26,"Slide right", 276, BT_MOUSE_BUTTON, 255 },
	{  8,180, 70, 59, 26, 7, 9, 26, 2,"Slide up", 277, BT_MOUSE_BUTTON, 255 },
	{  9,180, 78, 59, 26, 8, 10, 2, 3,"Slide down", 278, BT_MOUSE_BUTTON, 255 },
	{ 10,180, 86, 59, 26, 9, 11, 3, 27,"Bank on", 279, BT_MOUSE_BUTTON, 255 },
	{ 11,180, 94, 59, 26, 10, 12, 27, 25,"Bank left", 280, BT_MOUSE_BUTTON, 255 },
	{ 12,180,102, 59, 26, 11, 30, 25, 28,"Bank right", 281, BT_MOUSE_BUTTON, 255 },

	{ 13,103,146, 58, 26, 29, 15, 30, 14,"Pitch U/D", 300, BT_MOUSE_AXIS, 255 },
	{ 14,103,146,106, 8, 23, 16, 13, 15,"Pitch U/D", 300, BT_INVERT, 255 },
	{ 15,103,154, 58, 26, 13, 17, 14, 16,"Turn L/R", 301, BT_MOUSE_AXIS, 255 },
	{ 16,103,154,106, 8, 14, 18, 15, 17,"Turn L/R", 301, BT_INVERT, 255 },
	{ 17,103,162, 58, 26, 15, 19, 16, 18,"Slide L/R", 302, BT_MOUSE_AXIS, 255 },
	{ 18,103,162,106, 8, 16, 20, 17, 19,"Slide L/R", 302, BT_INVERT, 255 },
	{ 19,103,170, 58, 26, 17, 21, 18, 20,"Slide U/D", 303, BT_MOUSE_AXIS, 255 },
	{ 20,103,170,106, 8, 18, 22, 19, 21,"Slide U/D", 303, BT_INVERT, 255 },
	{ 21,103,178, 58, 26, 19, 23, 20, 22,"Bank L/R", 304, BT_MOUSE_AXIS, 255 },
	{ 22,103,178,106, 8, 20, 24, 21, 23,"Bank L/R", 304, BT_INVERT, 255 },
	{ 23,103,186, 58, 26, 21, 0, 22, 24,"Throttle", 305, BT_MOUSE_AXIS, 255 },
	{ 24,103,186,106, 8, 22, 5, 23, 0,"Throttle", 305, BT_INVERT, 255 },

	{ 25, 25,102, 85, 26, 27, 28, 11, 12,"Rear View", 288, BT_MOUSE_BUTTON, 255 },
	{ 26, 25, 70, 85, 26, 4, 2, 7, 8,"Drop Bomb", 287, BT_MOUSE_BUTTON, 255 },
	{ 27, 25, 94, 85, 26, 3, 25, 10, 11,"Afterburner", 293, BT_MOUSE_BUTTON, 255 },
	{ 28, 25,110, 85, 26, 25, 29, 12, 29,"Cycle Primary", 294, BT_MOUSE_BUTTON, 255 },
	{ 29, 25,118, 85, 26, 28, 13, 28, 30,"Cycle Second", 295, BT_MOUSE_BUTTON, 255 },
	{ 30,180,118, 59, 26, 12, 14, 29, 13,"Zoom in", 296, BT_MOUSE_BUTTON, 255 }
};

kcItem kcSuperJoy [] = {
	{  0, 25, 46, 85, 26, 15, 1, 24, 5,"Fire primary", 282, BT_JOY_BUTTON, 255 },
	{  1, 25, 54, 85, 26, 0, 4, 5, 6,"Fire secondary", 283, BT_JOY_BUTTON, 255 },
	{  2, 25, 85, 85, 26, 26, 3, 9, 10,"Accelerate", 285, BT_JOY_BUTTON, 255 },
	{  3, 25, 93, 85, 26, 2, 25, 10, 11,"reverse", 286, BT_JOY_BUTTON, 255 },
	{  4, 25, 62, 85, 26, 1, 26, 6, 7,"Fire flare", 284, BT_JOY_BUTTON, 255 },
	{  5,180, 46, 79, 26, 23, 6, 0, 1,"Slide on", 274, BT_JOY_BUTTON, 255 },
	{  6,180, 54, 79, 26, 5, 7, 1, 4,"Slide left", 275, BT_JOY_BUTTON, 255 },
	{  7,180, 62, 79, 26, 6, 8, 4, 26,"Slide right", 276, BT_JOY_BUTTON, 255 },
	{  8,180, 70, 79, 26, 7, 9, 26, 9,"Slide up", 277, BT_JOY_BUTTON, 255 },
	{  9,180, 78, 79, 26, 8, 10, 8, 2,"Slide down", 278, BT_JOY_BUTTON, 255 },
	{ 10,180, 86, 79, 26, 9, 11, 2, 3,"Bank on", 279, BT_JOY_BUTTON, 255 },
	{ 11,180, 94, 79, 26, 10, 12, 3, 12,"Bank left", 280, BT_JOY_BUTTON, 255 },
	{ 12,180,102, 79, 26, 11, 28, 11, 25,"Bank right", 281, BT_JOY_BUTTON, 255 },
	{ 13, 22,162, 51, 26, 33, 15, 32, 14,"Pitch U/D", 300, BT_JOY_AXIS, 255 },
	{ 14, 22,162, 99, 8, 33, 16, 13, 17,"Pitch U/D", 300, BT_INVERT, 255 },
	{ 15, 22,170, 51, 26, 13, 0, 18, 16,"Turn L/R", 301, BT_JOY_AXIS, 255 },
	{ 16, 22,170, 99, 8, 14, 17, 15, 19,"Turn L/R", 301, BT_INVERT, 255 },
	{ 17,164,162, 58, 26, 32, 19, 14, 18,"Slide L/R", 302, BT_JOY_AXIS, 255 },
	{ 18,164,162,106, 8, 32, 20, 17, 15,"Slide L/R", 302, BT_INVERT, 255 },
	{ 19,164,170, 58, 26, 17, 21, 16, 20,"Slide U/D", 303, BT_JOY_AXIS, 255 },
	{ 20,164,170,106, 8, 18, 22, 19, 21,"Slide U/D", 303, BT_INVERT, 255 },
	{ 21,164,178, 58, 26, 19, 23, 20, 22,"Bank L/R", 304, BT_JOY_AXIS, 255 },
	{ 22,164,178,106, 8, 20, 24, 21, 23,"Bank L/R", 304, BT_INVERT, 255 },
	{ 23,164,186, 58, 26, 21, 5, 22, 24,"throttle", 305, BT_JOY_AXIS, 255 },
	{ 24,164,186,106, 8, 22, 13, 23, 0,"throttle", 305, BT_INVERT, 255 },
	{ 25, 25,110, 85, 26, 3, 27, 12, 28,"Rear View", 288, BT_JOY_BUTTON, 255 },
	{ 26, 25, 70, 85, 26, 4, 2, 7, 8,"Drop Bomb", 287, BT_JOY_BUTTON, 255 },
	{ 27, 25,118, 85, 26, 25, 30, 28, 29,"Afterburner", 293, BT_JOY_BUTTON, 255 },
	{ 28,180,110, 79, 26, 12, 29, 25, 27,"Cycle Primary", 294, BT_JOY_BUTTON, 255 },
	{ 29,180,118, 79, 26, 28, 31, 27, 30,"Cycle Secondary", 295, BT_JOY_BUTTON, 255 },
	{ 30, 25,126, 85, 26, 27, 33, 29, 31,"Headlight", 297, BT_JOY_BUTTON, 255 },
	{ 31,180,126, 79, 26, 29, 32, 30, 33,"Toggle Bomb", 299, BT_JOY_BUTTON, 255 },
	{ 32,180,134, 79, 26, 31, 18, 33, 13,"Toggle Icons", 653, BT_JOY_BUTTON, 255 },
	{ 33, 25,134, 85, 26, 30, 14, 31, 32,"Automap", 292, BT_JOY_BUTTON, 255 },
};

#ifdef D2X_KEYS
//added on 2/4/99 by Victor Rachels to add d1x new keys
kcItem kcHotkeys [] = {
//        id,x,y,w1,w2,u,d,l,r,text_num1,nType,value
	{  0, 15, 49, 71, 26, 27, 2, 27, 1, "Weapon 1", 306, BT_KEY, 255 },
	{  1, 15, 49,100, 26, 26, 3, 0, 2, "Weapon 1", 306, BT_JOY_BUTTON, 255 },
	{  2, 15, 57, 71, 26, 0, 4, 1, 3, "Weapon 2", 307, BT_KEY, 255 },
	{  3, 15, 57,100, 26, 1, 5, 2, 4, "Weapon 2", 307, BT_JOY_BUTTON, 255 },
	{  4, 15, 65, 71, 26, 2, 6, 3, 5, "Weapon 3", 308, BT_KEY, 255 },
	{  5, 15, 65,100, 26, 3, 7, 4, 6, "Weapon 3", 308, BT_JOY_BUTTON, 255 },
	{  6, 15, 73, 71, 26, 4, 8, 5, 7, "Weapon 4", 309, BT_KEY, 255 },
	{  7, 15, 73,100, 26, 5, 9, 6, 8, "Weapon 4", 309, BT_JOY_BUTTON, 255 },
	{  8, 15, 81, 71, 26, 6, 10, 7, 9, "Weapon 5", 310, BT_KEY, 255 },
	{  9, 15, 81,100, 26, 7, 11, 8, 10, "Weapon 5", 310, BT_JOY_BUTTON, 255 },

	{ 10, 15, 89, 71, 26, 8, 12, 9, 11, "Weapon 6", 311, BT_KEY, 255 },
	{ 11, 15, 89,100, 26, 9, 13, 10, 12, "Weapon 6", 311, BT_JOY_BUTTON, 255 },
	{ 12, 15, 97, 71, 26, 10, 14, 11, 13, "Weapon 7", 312, BT_KEY, 255 },
	{ 13, 15, 97,100, 26, 11, 15, 12, 14, "Weapon 7", 312, BT_JOY_BUTTON, 255 },
	{ 14, 15,105, 71, 26, 12, 16, 13, 15, "Weapon 8", 313, BT_KEY, 255 },
	{ 15, 15,105,100, 26, 13, 17, 14, 16, "Weapon 8", 313, BT_JOY_BUTTON, 255 },
	{ 16, 15,113, 71, 26, 14, 18, 15, 17, "Weapon 9", 314, BT_KEY, 255 },
	{ 17, 15,113,100, 26, 15, 19, 16, 18, "Weapon 9", 314, BT_JOY_BUTTON, 255 },
	{ 18, 15,121, 71, 26, 16, 20, 17, 19, "Weapon 10", 315, BT_KEY, 255 },
	{ 19, 15,121,100, 26, 17, 21, 18, 20, "Weapon 10", 315, BT_JOY_BUTTON, 255 }

	//{ 20, 15,131, 71, 26, 18, 22, 19, 21, "CYC PRIMARY", BT_KEY, 255 },
	//{ 21, 15,131,100, 26, 19, 23, 20, 22, "CYC PRIMARY", BT_JOY_BUTTON, 255 },
	//{ 22, 15,139, 71, 26, 20, 24, 21, 23, "CYC SECONDARY", BT_KEY, 255 },
	//{ 23, 15,139,100, 26, 21, 25, 22, 24, "CYC SECONDARY", BT_JOY_BUTTON, 255 },
	//{ 24, 8,147, 78, 26, 22, 26, 23, 25, "TOGGLE_PRIM AUTO", BT_KEY, 255 },
	//{ 25, 8,147,107, 26, 23, 27, 24, 26, "TOGGLE_PRIM_AUTO", BT_JOY_BUTTON, 255 },
	//{ 26, 8,155, 78, 26, 24, 1, 25, 27, "TOGGLE SEC AUTO", BT_KEY, 255 },
	//{ 27, 8,155,107, 26, 25, 0, 26, 0, "TOGGLE SEC AUTO", BT_JOY_BUTTON, 255 },
};
//end this section addition - VR
#endif

static int xOffs = 0, yOffs = 0;

static int start_axis [JOY_MAX_AXES];

vmsVector ExtForceVec;
vmsMatrix ExtApplyForceMatrix;

int ExtJoltInfo [3]={0,0,0};
int ExtXVibrateInfo [2]={0,0};
int ExtYVibrateInfo [2]={0,0};
ubyte ExtXVibrateClear=0;
ubyte ExtYVibrateClear=0;

typedef struct tKCItemPos {
	int	i, l, r, y;
	} tKCItemPos;

//------------------------------------------------------------------------------

void KCDrawItemExt (kcItem *item, int is_current, int bRedraw);
int KCChangeInvert (kcItem * item);
void ControlsReadFCS (int raw_axis);
void KCSetFCSButton (int btn, int button);
void KCReadExternalControls (void);

//------------------------------------------------------------------------------

inline void KCDrawItem (kcItem *item, int is_current)
{
KCDrawItemExt (item, is_current, gameOpts->menus.nStyle);
}

//------------------------------------------------------------------------------

int KCIsAxisUsed (int axis)
{
	int i;

for (i = 0; i < NUM_JOY_CONTROLS; i++) {
	if ((kcJoystick [i].nType == BT_JOY_AXIS) && (kcJoystick [i].value == axis))
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

int FindItemAt (kcItem * items, int nItems, int x, int y)
{
	int i;
	
for (i = 0; i < nItems; i++)	{
	if (((items [i].x + items [i].w1) == x) && (items [i].y == y))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------

int FindNextItemRight (kcItem * items, int nItems, int cItem, tKCItemPos *pos, int *ref)
{
cItem = ref [cItem];
return pos [(cItem + 1) % nItems].i;
}

//------------------------------------------------------------------------------

int FindNextItemLeft (kcItem *items, int nItems, int cItem, tKCItemPos *pos, int *ref)
{
cItem = ref [cItem];
return pos [cItem ? cItem - 1 : nItems - 1].i;
}

//------------------------------------------------------------------------------

int FindNextItemUp (kcItem * items, int nItems, int cItem, tKCItemPos *pos, int *ref)
{
	int l, r, x, y, yStart, h, i, j, dx, dy, dMin;

i = j = ref [cItem];
l = pos [i].l;
r = pos [i].r;
x = (l + r) / 2;
y = yStart = pos [i].y;
#if 0
do {
	if (--i < 0)
		i = nItems - 1;
	if ((r >= pos [i].l) && (l <= pos [i].r))
		return pos [i].i;
	} while (i != j);
#endif
dMin = 0x7fffffff;
dy = 0;
for (;;) {
	if (--i < 0)
		i = nItems - 1;
	if (i == j)
		break;
	if (pos [i].y == yStart)
		continue;
	dx = abs (x - (pos [i].l + pos [i].r) / 2);
	if (y != pos [i].y) {
		y = pos [i].y;
		dy += 10;
		}
	dx += dy;
	if (dMin > dx) {
		dMin = dx;
		h = i;
		}
	}
return pos [h].i;
}

//------------------------------------------------------------------------------

int FindNextItemDown (kcItem * items, int nItems, int cItem, tKCItemPos *pos, int *ref)
{
	int l, r, x, y, yStart, h, i, j, dx, dy, dMin;

i = j = ref [cItem];
l = pos [i].l;
r = pos [i].r;
x = (l + r) / 2;
y = yStart = pos [i].y;
#if 0
do {
	i = (i + 1) % nItems;
	if ((r >= pos [i].l) && (l <= pos [i].r))
		return pos [i].i;
	} while (i != j);
#endif
dMin = 0x7fffffff;
dy = 0;
for (;;) {
	i = (i + 1) % nItems;
	if (i == j)
		break;
	if (pos [i].y == yStart)
		continue;
	dx = abs (x - (pos [i].l + pos [i].r) / 2);
	if (y != pos [i].y) {
		y = pos [i].y;
		dy += 10;
		}
	dx += dy;
	if (dMin > dx) {
		dMin = dx;
		h = i;
		}
	}
return pos [h].i;
}

//------------------------------------------------------------------------------

inline char *MouseTextString (i)
{
return (i < 3) ? baseGameTexts [mousebutton_text [i]] : mousebutton_textra [i - 3];
}

//------------------------------------------------------------------------------

int KCGetItemHeight (kcItem *item)
{
	int w, h, aw;
	char btext [10];

	if (item->value==255) {
		strcpy (btext, "");
	} else {
		switch (item->nType)	{
			case BT_KEY:
				strncpy (btext, key_text [item->value], 10); 
				break;
			case BT_MOUSE_BUTTON:
				strncpy (btext, MouseTextString (item->value), 10); 
				break;
			case BT_MOUSE_AXIS:
				strncpy (btext, baseGameTexts [mouseaxis_text [item->value]], 10); 
				break;
			case BT_JOY_BUTTON:
#if defined (USE_LINUX_JOY)
				sprintf (btext, "J%d B%d", j_button [item->value].joydev, 
						  j_Get_joydev_button_number (item->value);
#else 
				{
					int	nStick = item->value / MAX_BUTTONS_PER_JOYSTICK;
					int	nBtn = item->value % MAX_BUTTONS_PER_JOYSTICK;
					int	nHat = SDL_Joysticks [nStick].n_buttons;
					//static char szHatDirs [4] = {'U', 'L', 'D', 'R'};
					static char cHatDirs [4] = { (char) 130, (char) 127, (char) 128, (char) 129};

				if (nBtn < nHat)
					sprintf (btext, "J%d B%d", nStick + 1, nBtn + 1);
				else
					sprintf (btext, "HAT%d%c", nStick + 1, cHatDirs [nBtn - nHat]);
				}
#endif
				break;
			case BT_JOY_AXIS:
#if defined (USE_LINUX_JOY)
				sprintf (btext, "J%d A%d", j_axis [item->value].joydev, 
						  j_Get_joydev_axis_number (item->value);
#else
				{
					int	nStick = item->value / MAX_AXES_PER_JOYSTICK;
					int	nAxis = item->value % MAX_AXES_PER_JOYSTICK;
					static char	cAxis [4] = {'X', 'Y', 'Z', 'R'};

				if (nAxis < 4)
					sprintf (btext, "J%d %c", nStick + 1, cAxis [nAxis]);
				else
					sprintf (btext, "J%d A%d", nStick + 1, nAxis + 1);
				}
#endif
				break;
			case BT_INVERT:
				strncpy (btext, baseGameTexts [invert_text [item->value]], 10); 
				break;
		}
	}
	GrGetStringSize (btext, &w, &h, &aw);

	return h;
}

//------------------------------------------------------------------------------

#define kc_gr_scanline(_x1,_x2,_y)	GrScanLine ((_x1), (_x2), (_y))
#define kc_gr_pixel(_x,_y)	gr_pixel ((_x), (_y))
#define KC_LHX(_x) (LHX (_x)+xOffs)
#define KC_LHY(_y) (LHY (_y)+yOffs)

void KCDrawTitle (char *title)
{
char *p = strchr (title, '\n');

grdCurCanv->cv_font = MEDIUM3_FONT;
if (p) 
	*p = 32;
GrString (0x8000, KC_LHY (8), title);
if (p)
	*p = '\n';
}

//------------------------------------------------------------------------------

void KCDrawHeader (kcItem *items)
{
grdCurCanv->cv_font = GAME_FONT;
GrSetFontColorRGBi (RGBA_PAL2 (28, 28, 28), 1, 0, 0);

GrString (0x8000, KC_LHY (20), TXT_KCONFIG_STRING_1);
GrSetFontColorRGBi (RGBA_PAL2 (28, 28, 28), 1, 0, 0);
if (items == kcKeyboard)	{
	GrSetFontColorRGBi (RGBA_PAL2 (31, 27, 6), 1, 0, 0);
	GrSetColorRGBi (RGBA_PAL2 (31, 27, 6));
	kc_gr_scanline (KC_LHX (98), KC_LHX (106), KC_LHY (42));
	kc_gr_scanline (KC_LHX (120), KC_LHX (128), KC_LHY (42));
	kc_gr_pixel (KC_LHX (98), KC_LHY (43));						
	kc_gr_pixel (KC_LHX (98), KC_LHY (44));						
	kc_gr_pixel (KC_LHX (128), KC_LHY (43));						
	kc_gr_pixel (KC_LHX (128), KC_LHY (44));						
	
	GrString (KC_LHX (109), KC_LHY (40), "OR");

	kc_gr_scanline (KC_LHX (253), KC_LHX (261), KC_LHY (42));
	kc_gr_scanline (KC_LHX (274), KC_LHX (283), KC_LHY (42));
	kc_gr_pixel (KC_LHX (253), KC_LHY (43));						
	kc_gr_pixel (KC_LHX (253), KC_LHY (44));						
	kc_gr_pixel (KC_LHX (283), KC_LHY (43));						
	kc_gr_pixel (KC_LHX (283), KC_LHY (44));						

	GrString (KC_LHX (264), KC_LHY (40), "OR");

}
if (items == kcJoystick)	{
	GrSetFontColorRGBi (RGBA_PAL2 (31,27,6), 1, 0, 0);
	GrSetColorRGBi (RGBA_PAL2 (31, 27, 6));
	kc_gr_scanline (KC_LHX (18), KC_LHX (135), KC_LHY (37));
	kc_gr_scanline (KC_LHX (181), KC_LHX (294), KC_LHY (37));
	kc_gr_scanline (KC_LHX (18), KC_LHX (144), KC_LHY (119+18));
	kc_gr_scanline (KC_LHX (174), KC_LHX (294), KC_LHY (119+18));
	GrString (0x8000, KC_LHY (35), TXT_BUTTONS_HATS);
	GrString (0x8000,KC_LHY (125+18), TXT_AXES);
	GrSetFontColorRGBi (RGBA_PAL2 (28,28,28), 1, 0, 0);
	GrString (KC_LHX (85), KC_LHY (145+8), TXT_AXIS);
	GrString (KC_LHX (120), KC_LHY (145+8), TXT_INVERT);
	GrString (KC_LHX (235), KC_LHY (145+8), TXT_AXIS);
	GrString (KC_LHX (270), KC_LHY (145+8), TXT_INVERT);
} else if (items == kcMouse)	{
	GrSetFontColorRGBi (RGBA_PAL2 (31,27,6), 1, 0, 0);
	GrSetColorRGBi (RGBA_PAL2 (31,27,6));
	kc_gr_scanline (KC_LHX (18), KC_LHX (135), KC_LHY (37));
	kc_gr_scanline (KC_LHX (181), KC_LHX (294), KC_LHY (37));
	kc_gr_scanline (KC_LHX (18), KC_LHX (144), KC_LHY (119+5));
	kc_gr_scanline (KC_LHX (174), KC_LHX (294), KC_LHY (119+5));
	GrString (0x8000, KC_LHY (35), TXT_BUTTONS);
	GrString (0x8000,KC_LHY (125+5), TXT_AXES);
	GrSetFontColorRGBi (RGBA_PAL2 (28,28,28), 1, 0, 0);
	GrString (KC_LHX (169), KC_LHY (137), TXT_AXIS);
	GrString (KC_LHX (199), KC_LHY (137), TXT_INVERT);
}
#ifdef D2X_KEYS
else if (items == kcHotkeys)
{
	GrSetFontColorRGBi (RGBA_PAL2 (31,27,6), 1, 0, 0);
	GrSetColorRGBi (RGBA_PAL2 (31, 27, 6));

	GrString (KC_LHX (94), KC_LHY (40), "KB");
	GrString (KC_LHX (121), KC_LHY (40), "JOY");
}
#endif
}

//------------------------------------------------------------------------------

void KCDrawTable (kcItem *items, int nItems, int cItem)
{
	int	i;

for (i = 0; i < nItems; i++)
	KCDrawItemExt (items + i, 0, 0);
KCDrawItemExt (items + cItem, 1, 0);
}

//------------------------------------------------------------------------------

void KCQuitMenu (grs_canvas *save_canvas, grs_font *save_font, bkg *bg, int time_stopped)
{
grdCurCanv->cv_font	= save_font;
WIN (DEFINE_SCREEN (old_bg_pcx));
//WINDOS (DDGrFreeSubCanvas (bg->menu_canvas), GrFreeSubCanvas (bg->menu_canvas));
//bg->menu_canvas = NULL;
WINDOS (DDGrSetCurrentCanvas (save_canvas), GrSetCurrentCanvas (save_canvas));			
GameFlushInputs ();
NMRemoveBackground (bg);
SDL_ShowCursor (0);
if (time_stopped)
	StartTime ();
gameStates.menus.nInMenu--;
GrPaletteStepUp (0, 0, 0);
}

//------------------------------------------------------------------------------

inline int KCAssignControl (kcItem *item, int nType, ubyte code)
{
	int	i, n;

if (code == 255)
	return nType;

for (i = 0, n = (int) (item - All_items); i < Num_items; i++)	{
	if ((i != n) && (All_items [i].nType == nType) && (All_items [i].value == code)) {
		All_items [i].value = 255;
		if (curDrawBuffer == GL_FRONT)
			KCDrawItem (All_items + i, 0);
		}
	}
item->value = code;					 
if (curDrawBuffer == GL_FRONT) {
	KCDrawItem (item, 1);
	NMRestoreBackground (0, KC_LHY (INFO_Y), xOffs, yOffs, KC_LHX (310), grdCurCanv->cv_font->ft_h);
	}
GameFlushInputs ();
WIN (DDGRLOCK (dd_grd_curcanv));
GrSetFontColorRGBi (RGBA_PAL2 (28,28,28), 1, 0, 1);
WIN (DDGRUNLOCK (dd_grd_curcanv));
return BT_NONE;
}

//------------------------------------------------------------------------------

void KCDrawQuestion (kcItem *item)
{
	static int looper=0;

	int x, w, h, aw;

WIN (DDGRLOCK (dd_grd_curcanv));	
  // PA_DFX (pa_set_frontbuffer_current ();

	GrGetStringSize ("?", &w, &h, &aw);
	//@@GrSetColor (grFadeTable [fades [looper]*256+c]);
	GrSetColorRGBi (RGBA_PAL2 (21*fades [looper]/31, 0, 24*fades [looper]/31));
	if (++looper>63) 
		looper=0;
	GrURect (KC_LHX (item->w1 + item->x), KC_LHY (item->y - 1), 
				KC_LHX (item->w1 + item->x + item->w2), KC_LHY (item->y) + h);
	GrSetFontColorRGBi (RGBA_PAL2 (28,28,28), 1, 0, 0);
	x = LHX (item->w1+item->x)+ ((LHX (item->w2)-w)/2)+xOffs;
	GrString (x, KC_LHY (item->y), "?");
//	PA_DFX (pa_set_backbuffer_current ();
WIN (DDGRUNLOCK (dd_grd_curcanv));
if (curDrawBuffer != GL_BACK)
	GrUpdate (0);
}

//------------------------------------------------------------------------------

typedef ubyte kc_ctrlfuncType (void);
typedef kc_ctrlfuncType *kc_ctrlfunc_ptr;

//------------------------------------------------------------------------------

ubyte KCKeyCtrlFunc (void)
{
	int	i, n, f;

for (i = 0; i < 256; i++)	{
	if (keyd_pressed [i] && strlen (key_text [i]))	{
		f = 0;
		for (n = 0; n < sizeof (system_keys); n++)
			if (system_keys [n] == i)
				f = 1;
		if (!f)	
			return (ubyte) i;
		}
	}
return 255;
}

//------------------------------------------------------------------------------

ubyte KCJoyBtnCtrlFunc (void)
{
	int i;
	ubyte code = 255;

WIN (code = joydefsw_do_button ());
if (gameStates.input.nJoyType == CONTROL_THRUSTMASTER_FCS) {
	int axis [JOY_MAX_AXES];
	JoyReadRawAxis (JOY_ALL_AXIS, axis);
	ControlsReadFCS (axis [3]);
	if (JoyGetButtonState (19)) 
		code = 19;
	else if (JoyGetButtonState (15)) 
		code = 15;
	else if (JoyGetButtonState (11)) 
		code = 11;
	else if (JoyGetButtonState (7)) 
		code = 7;
	for (i = 0; i < 4; i++)
		if (JoyGetButtonState (i))
			return (ubyte) i;
	}
else if (gameStates.input.nJoyType == CONTROL_FLIGHTSTICK_PRO) {
	for (i = 4; i < 20; i++) {
		if (JoyGetButtonState (i))
			return (ubyte) i;
		}
	}
else {
	for (i = 0; i < JOY_MAX_BUTTONS; i++) {
		if (JoyGetButtonState (i))
			return (ubyte) i;
		}
	}
return code;
}

//------------------------------------------------------------------------------

ubyte KCMouseBtnCtrlFunc (void)
{
int i, b = MouseGetButtons ();
for (i = 0; i < 16; i++)
	if (b & (1 << i))	
		return (ubyte) i;
return 255;
}

//------------------------------------------------------------------------------

ubyte KCJoyAxisCtrlFunc (void)
{
	int cur_axis [JOY_MAX_AXES];
	int i, hd, dd;
	int bLinJoySensSave = gameOpts->input.joystick.bLinearSens;
	ubyte code = 255;

memset (cur_axis, 0, sizeof (cur_axis));
gameOpts->input.joystick.bLinearSens = 1;
gameStates.input.kcPollTime = 128;
ControlsReadJoystick (cur_axis);
gameOpts->input.joystick.bLinearSens = bLinJoySensSave;
for (i = dd = 0; i < JOY_MAX_AXES; i++) {
	hd = abs (cur_axis [i] - start_axis [i]);
  	if ((hd > (128 * 3 / 4)) && (hd > dd)) {
		dd = hd;
		code = i;
		start_axis [i] = cur_axis [i];
		}
	}
return code;
}

//------------------------------------------------------------------------------

ubyte KCMouseAxisCtrlFunc (void)
{
	int dx, dy;
#ifdef SDL_INPUT
	int dz;
#endif
	ubyte code = 255;

#ifdef SDL_INPUT
MouseGetDeltaZ (&dx, &dy, &dz);
#else
MouseGetDelta (&dx, &dy);
#endif
con_printf (CON_VERBOSE, "mouse: %3d %3d\n", dx, dy);
dx = abs (dx);
dy = abs (dy);
if (max (dx, dy) > 20) {
	code = dy > dx;
	}
#ifdef SDL_INPUT
dz = abs (dz);
if ((dz > 20) && (dz > code ? dy : dx))
	code = 2;
#endif
return code;
}

//------------------------------------------------------------------------------

int KCChangeControl (kcItem *item, int nType, kc_ctrlfunc_ptr ctrlfunc, char *pszMsg)
{
	int k = 255;

WIN (DDGRLOCK (dd_grd_curcanv));
	GrSetFontColorRGBi (RGBA_PAL2 (28,28,28), 1, 0, 0);
	GrString (0x8000, KC_LHY (INFO_Y), pszMsg);
WIN (DDGRUNLOCK (dd_grd_curcanv));	
{				
	if ((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME) && (!gameStates.app.bEndLevelSequence))
		MultiMenuPoll ();
//		if (gameData.app.nGameMode & GM_MULTI)
//			GameLoop (0, 0);				// Continue
	k = KeyInKey ();
	if (k == KEY_ESC)
		return KCAssignControl (item, BT_NONE, 255);
	if (k == KEY_ALTED+KEY_F9) {
		SaveScreenShot (NULL, 0);
		return KCAssignControl (item, BT_NONE, 255);
		}
	if (curDrawBuffer == GL_FRONT)
		timer_delay (f0_1 / 10);
	KCDrawQuestion (item);
	}
return KCAssignControl (item, nType, ctrlfunc ());
}

//------------------------------------------------------------------------------

inline int KCChangeKey (kcItem *item)
{
return KCChangeControl (item, BT_KEY, KCKeyCtrlFunc, TXT_PRESS_NEW_KEY);
}

//------------------------------------------------------------------------------

inline int KCChangeJoyButton (kcItem *item)
{
return KCChangeControl (item, BT_JOY_BUTTON, KCJoyBtnCtrlFunc, TXT_PRESS_NEW_JBUTTON);
}

//------------------------------------------------------------------------------

inline int KCChangeMouseButton (kcItem * item)
{
return KCChangeControl (item, BT_MOUSE_BUTTON, KCMouseBtnCtrlFunc, TXT_PRESS_NEW_MBUTTON);
}

//------------------------------------------------------------------------------

inline int KCChangeJoyAxis (kcItem *item)
{
return KCChangeControl (item, BT_JOY_AXIS, KCJoyAxisCtrlFunc, TXT_MOVE_NEW_JOY_AXIS);
}

//------------------------------------------------------------------------------

inline int KCChangeMouseAxis (kcItem * item)
{
return KCChangeControl (item, BT_MOUSE_AXIS, KCMouseAxisCtrlFunc, TXT_MOVE_NEW_MSE_AXIS);
}

//------------------------------------------------------------------------------

int KCChangeInvert (kcItem * item)
{
GameFlushInputs ();
item->value = !item->value;
if (curDrawBuffer == GL_FRONT) 
	KCDrawItem (item, 1);
return BT_NONE;
}

//------------------------------------------------------------------------------

void QSortItemPos (tKCItemPos *pos, int left, int right)
{
	int			l = left, 
					r = right;
	tKCItemPos	h, m = pos [(l + r) / 2];

while ((pos [l].y < m.y) || ((pos [l].y == m.y) && (pos [l].l < m.l)))
	l++;
while ((pos [r].y > m.y) || ((pos [r].y == m.y) && (pos [r].l > m.l)))
	r--;
if (l <= r) {
	if (l < r) {
		h = pos [l];
		pos [l] = pos [r];
		pos [r] = h;
		}
	l++;
	r--;
	}
if (l < right)
	QSortItemPos (pos, l, right);
if (left < r)
	QSortItemPos (pos, left, r);
}

//------------------------------------------------------------------------------

tKCItemPos *GetItemPos (kcItem *items, int nItems)
{
	tKCItemPos	*pos;
	int			i;

if (!(pos = (tKCItemPos *) D2_ALLOC (nItems * sizeof (tKCItemPos))))
	return NULL;
for (i = 0; i < nItems; i++) {
	pos [i].l = items [i].x + items [i].w1;
	pos [i].r = pos [i].l + items [i].w2;
	pos [i].y = items [i].y;
	pos [i].i = i;
	}
QSortItemPos (pos, 0, nItems - 1);
return pos;
}

//------------------------------------------------------------------------------

int *GetItemRef (kcItem *items, int nItems, tKCItemPos *pos)
{
	int	*ref;
	int	i;

if (!(ref = (int *) D2_ALLOC (nItems * sizeof (int))))
	return NULL;
for (i = 0; i < nItems; i++)
	ref [pos [i].i] = i;
return ref;
}

//------------------------------------------------------------------------------

void LinkKbdEntries (void)
{
	int			i, j, *ref;
	tKCItemPos	*pos = GetItemPos (kcKeyboard, NUM_KEY_CONTROLS);

if (pos) {
	if (ref = GetItemRef (kcKeyboard, NUM_KEY_CONTROLS, pos)) {
		for (i = 0, j = NUM_KEY_CONTROLS; i < j; i++) {
			kcKeyboard [i].u = FindNextItemUp (kcKeyboard, j, i, pos, ref);
			kcKeyboard [i].d = FindNextItemDown (kcKeyboard, j, i, pos, ref);
			kcKeyboard [i].l = FindNextItemLeft (kcKeyboard, j, i, pos, ref);
			kcKeyboard [i].r = FindNextItemRight (kcKeyboard, j, i, pos, ref);
			}
		D2_FREE (ref);
		}
	D2_FREE (pos);
	}
}

//------------------------------------------------------------------------------

void LinkJoyEntries (void)
{
	int			i, j, *ref;
	tKCItemPos	*pos = GetItemPos (kcJoystick, NUM_JOY_CONTROLS);

if (pos) {
	if (ref = GetItemRef (kcJoystick, NUM_JOY_CONTROLS, pos)) {
		for (i = 0, j = NUM_JOY_CONTROLS; i < j; i++) {
			kcJoystick [i].u = FindNextItemUp (kcJoystick, j, i, pos, ref);
			kcJoystick [i].d = FindNextItemDown (kcJoystick, j, i, pos, ref);
			kcJoystick [i].l = FindNextItemLeft (kcJoystick, j, i, pos, ref);
			kcJoystick [i].r = FindNextItemRight (kcJoystick, j, i, pos, ref);
			}
		D2_FREE (ref);
		}
	D2_FREE (pos);
	}
}

//------------------------------------------------------------------------------

void LinkMouseEntries (void)
{
	int			i, j, *ref;
	tKCItemPos	*pos = GetItemPos (kcMouse, NUM_MOUSE_CONTROLS);

if (pos) {
	if (ref = GetItemRef (kcMouse, NUM_MOUSE_CONTROLS, pos)) {
		for (i = 0, j = NUM_MOUSE_CONTROLS; i < j; i++)	{
			kcMouse [i].u = FindNextItemUp (kcMouse, j, i, pos, ref);
			kcMouse [i].d = FindNextItemDown (kcMouse, j, i, pos, ref);
			kcMouse [i].l = FindNextItemLeft (kcMouse, j, i, pos, ref);
			kcMouse [i].r = FindNextItemRight (kcMouse, j, i, pos, ref);
			}
		D2_FREE (ref);
		}
	D2_FREE (pos);
	}
}

//------------------------------------------------------------------------------

void LinkHotkeyEntries (void)
{
	int			i, j, *ref;
	tKCItemPos	*pos = GetItemPos (kcHotkeys, NUM_HOTKEY_CONTROLS);

if (pos) {
	if (ref = GetItemRef (kcHotkeys, NUM_HOTKEY_CONTROLS, pos)) {
		for (i = 0, j = NUM_HOTKEY_CONTROLS; i < j; i++)	{
			kcHotkeys [i].u = FindNextItemUp (kcHotkeys, j, i, pos, ref);
			kcHotkeys [i].d = FindNextItemDown (kcHotkeys, j, i, pos, ref);
			kcHotkeys [i].l = FindNextItemLeft (kcHotkeys, j, i, pos, ref);
			kcHotkeys [i].r = FindNextItemRight (kcHotkeys, j, i, pos, ref);
			}
		D2_FREE (ref);
		}
	D2_FREE (pos);
	}
}

//------------------------------------------------------------------------------

void LinkTableEntries (int tableFlags)
{
	static int nLinked = 0;

if ((tableFlags & 1) && !(nLinked & 1))
	LinkKbdEntries ();
if ((tableFlags & 2) && !(nLinked & 2))
	LinkJoyEntries ();
if ((tableFlags & 4) && !(nLinked & 4))
	LinkMouseEntries ();
if ((tableFlags & 8) && !(nLinked & 8))
	LinkHotkeyEntries ();
nLinked |= tableFlags;
}

//------------------------------------------------------------------------------

void KConfigSub (kcItem * items, int nItems, char * title)
{
WINDOS (
	dd_grs_canvas * save_canvas,
	grs_canvas * save_canvas
);
	grs_font * save_font;
	int	mouseState, omouseState, mx, my, x1, x2, y1, y2;
	int	close_x = 0, close_y = 0, close_size = 0;

	int	i,k,ocitem,cItem;
	int	time_stopped = 0;
	int	bRedraw = 0;
	int	nChangeMode = BT_NONE, nPrevMode = BT_NONE;
	bkg	bg;

All_items = items;
Num_items = nItems;
memset (&bg, 0, sizeof (bg));
bg.bIgnoreBg = 1;
GrPaletteStepUp (0, 0, 0);
gameStates.menus.nInMenu++;
memset (start_axis, 0, sizeof (start_axis));

if (!IsMultiGame || (gameStates.app.nFunctionMode != FMODE_GAME) || gameStates.app.bEndLevelSequence) {
	time_stopped = 1;
	StopTime ();
	}

save_canvas = grdCurCanv;
GrSetCurrentCanvas (NULL);		
save_font = grdCurCanv->cv_font;

FlushInput ();
NMDrawBackground (&bg, xOffs, yOffs, 
	xOffs + 639 /*grdCurCanv->cv_bitmap.bm_props.w - 1*/, 
	yOffs + 479 /*grdCurCanv->cv_bitmap.bm_props.h - 1*/, 0);
GrPaletteStepLoad (NULL);

cItem = 0;
SDL_ShowCursor (1);
mouseState = omouseState = 0;
if (items == kcKeyboard)
	i = 0;
else if (items == kcJoystick)
	i = 1;
else if (items == kcMouse)
	i = 2;
else if (items == kcHotkeys)
	i = 3;
else
	i = -1;
if (i >= 0)
	LinkTableEntries (1 << i);
for (;;) {
//	Windows addendum to allow for KConfig input.
	do {
		if (gameOpts->menus.nStyle || !bRedraw) {
			bRedraw = 1;
			WIN (DDGRLOCK (dd_grd_curcanv));	
			if (gameOpts->menus.nStyle && gameStates.app.bGameRunning)
				GameRenderFrame ();
			NMDrawBackground (&bg, xOffs, yOffs, xOffs + 639, yOffs + 479, 1);
			KCDrawTitle (title);
			close_x = close_y = gameStates.menus.bHires ? 15 : 7;
			close_x += xOffs;
			close_y += yOffs;
			close_size = gameStates.menus.bHires?10:5;
			GrSetColorRGB (0, 0, 0, 255);
			GrRect (close_x, close_y, close_x + close_size, close_y + close_size);
			GrSetColorRGBi (RGBA_PAL2 (21, 21, 21));
			GrRect (close_x + LHX (1), close_y + LHX (1), close_x + close_size - LHX (1), close_y + close_size - LHX (1));
			KCDrawHeader (items);
			WIN (DDGRUNLOCK (dd_grd_curcanv));	
			KCDrawTable (items, nItems, cItem);
			}
		SDL_ShowCursor (0);
		switch (nChangeMode) {
			case BT_KEY:
				nChangeMode = KCChangeKey (items + cItem);
				break;
			case BT_MOUSE_BUTTON:
				nChangeMode = KCChangeMouseButton (items + cItem);
				break;
			case BT_MOUSE_AXIS:
				nChangeMode = KCChangeMouseAxis (items + cItem);
				break;
			case BT_JOY_BUTTON:
				nChangeMode = KCChangeJoyButton (items + cItem);
				break;
			case BT_JOY_AXIS:
				if (nChangeMode != nPrevMode)
					ControlsReadJoystick (start_axis);
				nChangeMode = KCChangeJoyAxis (items + cItem);
				break;
			case BT_INVERT:
				nChangeMode = KCChangeInvert (items + cItem);
				break;
			default:
				nChangeMode = BT_NONE;
			}
		nPrevMode = nChangeMode;
		SDL_ShowCursor (1);
		GrUpdate (0);
		} while (nChangeMode != BT_NONE);
		//see if redbook song needs to be restarted
	SongsCheckRedbookRepeat ();

	k = KeyInKey ();
	MultiDoFrame();
	omouseState = mouseState;
	mouseState = MouseButtonState (0);

	if (!time_stopped) {
		if (MultiMenuPoll () == -1)
			k = -2;
		}
	ocitem = cItem;
	switch (k)	{
		case KEY_BACKSP:
			Int3 ();
			break;
		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_ALTED+KEY_F9:
			SaveScreenShot (NULL, 0);
			break;							
		case KEY_CTRLED+KEY_D:
			items [cItem].value = 255;
			KCDrawItem (items + cItem, 1);
			break;
		case KEY_CTRLED+KEY_R:	
			if (items==kcKeyboard)	{
				for (i=0; i<NUM_KEY_CONTROLS; i++) {
					items [i].value=controlSettings.defaults [0][i];
					KCDrawItem (items + i, 0);
					}
#ifdef D2X_KEYS
				}
			else if (items == kcHotkeys) {
				for (i = 0; i < NUM_HOTKEY_CONTROLS; i++) {
					items [i].value=controlSettings.d2xDefaults [i];
					KCDrawItem (items + i, 0);
					}
				}
#endif
			else if (items == kcMouse) {
				for (i = 0; i < NUM_MOUSE_CONTROLS; i++) {
					items [i].value = controlSettings.defaults [gameConfig.nControlType][i];
					KCDrawItem (items + i, 0);
					}
				}
			else {
				for (i = 0; i < NUM_JOY_CONTROLS; i++)	{
					items [i].value = controlSettings.defaults [gameConfig.nControlType][i];
					KCDrawItem (items + i, 0);
					}
				}
			KCDrawItem (items + cItem, 1);
			break;
		case KEY_DELETE:
			items [cItem].value = 255;
			KCDrawItem (items + cItem, 1);
			break;
		case KEY_UP: 		
		case KEY_PAD8:
#if TABLE_CREATION
			if (items [cItem].u == -1) 
				items [cItem].u=FindNextItemUp (items,nItems, cItem);
#endif
			cItem = items [cItem].u; 
			break;
		
		case KEY_DOWN: 	
		case KEY_PAD2:
#if TABLE_CREATION
			if (items [cItem].d == -1) 
				items [cItem].d=FindNextItemDown (items,nItems, cItem);
#endif
			cItem = items [cItem].d; 
			break;
		case KEY_LEFT: 	
		case KEY_PAD4:
#if TABLE_CREATION
			if (items [cItem].l == -1) 
				items [cItem].l=FindNextItemLeft (items,nItems, cItem);
#endif
			cItem = items [cItem].l; 
			break;
		case KEY_RIGHT: 	
		case KEY_PAD6:
#if TABLE_CREATION
			if (items [cItem].r == -1) 
				items [cItem].r=FindNextItemRight (items,nItems, cItem);
#endif
			cItem = items [cItem].r; 
			break;
		case KEY_ENTER:	
		case KEY_PADENTER:	
			nChangeMode = items [cItem].nType;
			GameFlushInputs ();
			break;
		case -2:	
		case KEY_ESC:
			KCQuitMenu (save_canvas, save_font, &bg, time_stopped);
			return;
#if TABLE_CREATION
		case KEYDBGGED+KEY_F12:	{
			FILE *fp;
#if TRACE		
			con_printf (CONDBG, "start table creation\n");
#endif
			LinkTableEntries (1 | 2 | 4 | 8);
			fp = fopen ("KConfig.cod", "wt");
			fprintf (fp, "ubyte controlSettings.defaults [CONTROL_MAX_TYPES][MAX_CONTROLS] = {\n");
			for (i = 0; i < CONTROL_MAX_TYPES; i++)	{
				int j;
				fprintf (fp, "{0x%x", controlSettings.custom [i][0]);
				for (j = 1; j < MAX_CONTROLS; j++)
					fprintf (fp, ",0x%x", controlSettings.custom [i][j]);
				fprintf (fp, "},\n");
				}
			fprintf (fp, "};\n");

			fprintf (fp, "\nkc_item kcKeyboard [NUM_KEY_CONTROLS] = {\n");
			for (i=0; i<NUM_KEY_CONTROLS; i++)	{
				fprintf (fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
					kcKeyboard [i].id, kcKeyboard [i].x, kcKeyboard [i].y, kcKeyboard [i].w1, kcKeyboard [i].w2,
					kcKeyboard [i].u, kcKeyboard [i].d, kcKeyboard [i].l, kcKeyboard [i].r,
													 34, kcKeyboard [i].text, 34, btype_text [kcKeyboard [i].nType]);
				}
			fprintf (fp, "};");

			fprintf (fp, "\nkc_item kcJoystick [NUM_JOY_CONTROLS] = {\n");
			for (i=0; i<NUM_JOY_CONTROLS; i++)	{
				if (kcJoystick [i].nType == BT_JOY_BUTTON)
					fprintf (fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
						kcJoystick [i].id, kcJoystick [i].x, kcJoystick [i].y, kcJoystick [i].w1, kcJoystick [i].w2,
						kcJoystick [i].u, kcJoystick [i].d, kcJoystick [i].l, kcJoystick [i].r,
														 34, kcJoystick [i].text, 34, btype_text [kcJoystick [i].nType]);
				else
					fprintf (fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
						kcJoystick [i].id, kcJoystick [i].x, kcJoystick [i].y, kcJoystick [i].w1, kcJoystick [i].w2,
						kcJoystick [i].u, kcJoystick [i].d, kcJoystick [i].l, kcJoystick [i].r,
														 34, kcJoystick [i].text, 34, btype_text [kcJoystick [i].nType]);
				}
			fprintf (fp, "};");

			fprintf (fp, "\nkc_item kcMouse [NUM_MOUSE_CONTROLS] = {\n");
			for (i=0; i<NUM_MOUSE_CONTROLS; i++)	{
				fprintf (fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
					kcMouse [i].id, kcMouse [i].x, kcMouse [i].y, kcMouse [i].w1, kcMouse [i].w2,
					kcMouse [i].u, kcMouse [i].d, kcMouse [i].l, kcMouse [i].r,
													 34, kcMouse [i].text, 34, btype_text [kcMouse [i].nType]);
				}
			fprintf (fp, "};");
			fclose (fp);
#if TRACE		
			con_printf (CONDBG, "end table creation\n");
#endif
			}
		break;
#endif
		}

		if ((mouseState && !omouseState) || (mouseState && omouseState)) {
			int item_height;
			mouse_get_pos (&mx, &my);
			mx -= xOffs;
			my -= yOffs;
//			my = (my * 12) / 10;	//y mouse pos is off here, no clue why
			for (i = 0; i < nItems; i++)	{
				item_height = KCGetItemHeight (items + i);
				x1 = grdCurCanv->cv_bitmap.bm_props.x + LHX (items [i].x) + LHX (items [i].w1);
				x2 = x1 + LHX (items [i].w2);
				y1 = grdCurCanv->cv_bitmap.bm_props.y + LHY (items [i].y);
				y2 = y1 + /*LHY*/ (item_height);
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					cItem = i;
					break;
				}
			}
		}
		else if (!mouseState && omouseState) {
			int item_height;
			
			mouse_get_pos (&mx, &my);
			mx -= xOffs;
			my -= yOffs;
			my = (my * 12) / 10;	//y mouse pos is off here, no clue why
			item_height = KCGetItemHeight (items + cItem);
			x1 = grdCurCanv->cv_bitmap.bm_props.x + LHX (items [cItem].x) + LHX (items [cItem].w1);
			x2 = x1 + LHX (items [cItem].w2);
			y1 = grdCurCanv->cv_bitmap.bm_props.y + LHY (items [cItem].y);
			y2 = y1 + /*LHY*/ (item_height);
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				nChangeMode = items [cItem].nType;
				GameFlushInputs ();
			} else {
				x1 = grdCurCanv->cv_bitmap.bm_props.x + close_x + LHX (1);
				x2 = x1 + close_size - LHX (1);
				y1 = grdCurCanv->cv_bitmap.bm_props.y + close_y + LHX (1);
				y2 = y1 + close_size - LHY (1);
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					KCQuitMenu (save_canvas, save_font, &bg, time_stopped);
					return;
				}
			}
		}
		if (ocitem!=cItem)	{
			SDL_ShowCursor (0);
			KCDrawItem (items + ocitem, 0);
			KCDrawItem (items + cItem, 1);
			SDL_ShowCursor (1);
		}
	}
KCQuitMenu (save_canvas, save_font, &bg, time_stopped);
}

//------------------------------------------------------------------------------

void KCDrawItemExt (kcItem *item, int is_current, int bRedraw)
{
	int x, w, h, aw;
	char btext [64];
//	PA_DFX (pa_set_frontbuffer_current ();

if (bRedraw && gameOpts->menus.nStyle)
	return;
WIN (DDGRLOCK (dd_grd_curcanv));

	if (is_current)
		GrSetFontColorRGBi (RGBA_PAL2 (20,20,29), 1, 0, 0);
	else
		GrSetFontColorRGBi (RGBA_PAL2 (15,15,24), 1, 0, 0);
   GrString (KC_LHX (item->x), KC_LHY (item->y), item->textId ? GT (item->textId) : item->text);
WIN (DDGRUNLOCK (dd_grd_curcanv));

	*btext = '\0';
	if (item->value != 255) {
		switch (item->nType)	{
			case BT_KEY:
				strncat (btext, key_text [item->value], 10); 
				break;

			case BT_MOUSE_BUTTON:
				//strncpy (btext, baseGameTexts [mousebutton_text [item->value]], 10); break;
				strncpy (btext, MouseTextString (item->value), 10); 
				break;

			case BT_MOUSE_AXIS:
				strncpy (btext, baseGameTexts [mouseaxis_text [item->value]], 10); 
				break;

			case BT_JOY_BUTTON:
#ifdef USE_LINUX_JOY
				sprintf (btext, "J%d B%d", 
						  j_button [item->value].joydev, j_Get_joydev_button_number (item->value);
#else
				{
					int	nStick = item->value / MAX_BUTTONS_PER_JOYSTICK;
					int	nBtn = item->value % MAX_BUTTONS_PER_JOYSTICK;
					int	nHat = SDL_Joysticks [nStick].n_buttons;
					//static char szHatDirs [4] = {'U', 'L', 'D', 'R'};
					static char cHatDirs [4] = { (char) 130, (char) 127, (char) 128, (char) 129};

				if (nBtn < nHat)
					sprintf (btext, "J%d B%d", nStick + 1, nBtn + 1);
				else
					sprintf (btext, "HAT%d%c", nStick + 1, cHatDirs [nBtn - nHat]);
				}
#endif
				break;

			case BT_JOY_AXIS:
#if defined (USE_LINUX_JOY)
				sprintf (btext, "J%d A%d", j_axis [item->value].joydev, j_Get_joydev_axis_number (item->value));
#elif 1//defined (_WIN32)
				{
					int	nStick = item->value / MAX_AXES_PER_JOYSTICK;
					int	nAxis = item->value % MAX_AXES_PER_JOYSTICK;
					static char	cAxis [4] = {'X', 'Y', 'Z', 'R'};

				if (nAxis < 4)
					sprintf (btext, "J%d %c", nStick + 1, cAxis [nAxis]);
				else
					sprintf (btext, "J%d A%d", nStick + 1, nAxis + 1);
				}
#else
				strncpy (btext, baseGameTexts [JOYAXIS_TEXT (item->value)], 10);
#endif
				break;

			case BT_INVERT:
				strncpy (btext, baseGameTexts [invert_text [item->value]], 10); 
				break;
		}
	}
	if (item->w1) {
	WIN (DDGRLOCK (dd_grd_curcanv));
		GrGetStringSize (btext, &w, &h, &aw);

		if (is_current)
			GrSetColorRGBi (RGBA_PAL2 (21, 0, 24));
		else
			GrSetColorRGBi (RGBA_PAL2 (16, 0, 19));
		GrURect (KC_LHX (item->x + item->w1), KC_LHY (item->y - 1), 
					KC_LHX (item->x + item->w1 + item->w2), KC_LHY (item->y) + h);
		GrSetFontColorRGBi (RGBA_PAL2 (28, 28, 28), 1, 0, 0);
		x = LHX (item->w1 + item->x) + ((LHX (item->w2) - w) / 2) + xOffs;
		GrString (x, KC_LHY (item->y), btext);
//		PA_DFX (pa_set_backbuffer_current ();

	WIN (DDGRUNLOCK (dd_grd_curcanv));
	}
}

//------------------------------------------------------------------------------

#include "screens.h"

void KConfig (int n, char * title)
{
	grsBitmap	*bmSave;
	int			i, j, b = gameOpts->legacy.bInput;

	xOffs = (grdCurCanv->cv_bitmap.bm_props.w - 640) / 2;
	yOffs = (grdCurCanv->cv_bitmap.bm_props.h - 480) / 2;
	if (xOffs < 0)
		xOffs = 0;
	if (yOffs < 0)
		yOffs = 0;

	gameOpts->legacy.bInput = 1;
	SetScreenMode (SCREEN_MENU);
	KCSetControls (0);
	//save screen
	if (gameOpts->menus.bFastMenus)
		bmSave = NULL;
	else {
		bmSave = GrCreateBitmap (grdCurCanv->cv_bitmap.bm_props.w, grdCurCanv->cv_bitmap.bm_props.h, 1);
		Assert (bmSave != NULL);
		bmSave->bm_palette = gameData.render.ogl.palette;
		GrBmBitBlt (grdCurCanv->cv_bitmap.bm_props.w, grdCurCanv->cv_bitmap.bm_props.w, 
						 0, 0, 0, 0, &grdCurCanv->cv_bitmap, bmSave);
		}
	if (n == 0)
		KConfigSub (kcKeyboard, NUM_KEY_CONTROLS, title);
	else if (n == 1)
		KConfigSub (kcJoystick, NUM_JOY_CONTROLS, title);
	else if (n == 2)
		KConfigSub (kcMouse, NUM_MOUSE_CONTROLS, title); 
#if 0
	else if (n == 3)
		KConfigSub (kcSuperJoy, NUM_JOY_CONTROLS, title); 
#endif
#ifdef D2X_KEYS
	else if (n == 4)
		KConfigSub (kcHotkeys, NUM_HOTKEY_CONTROLS, title); 
	//end this section addition - VR
#endif
 	else {
		Int3 ();
		gameOpts->legacy.bInput = b;
		return;
		}

	//restore screen
	if (bmSave) {
		GrBitmap (xOffs, yOffs, bmSave);
		GrFreeBitmap (bmSave);
		}
	ResetCockpit ();		//force cockpit redraw next time
	// Update save values...
	if (n == 0) {
		for (i = 0, j = NUM_KEY_CONTROLS; i < j; i++)	
			controlSettings.custom [0][i] = kcKeyboard [i].value;
		}
	else if (n == 1) {
		if (gameOpts->input.joystick.bUse)
			for (i = 0, j = NUM_JOY_CONTROLS; i < j; i++)	
				controlSettings.custom [gameStates.input.nJoyType][i] = kcJoystick [i].value;
		}
	else if (n == 2) {
		if (gameOpts->input.mouse.bUse)
			for (i = 0, j = NUM_MOUSE_CONTROLS; i < j; i++)	
				controlSettings.custom [gameStates.input.nMouseType][i] = kcMouse [i].value;
		}
	else if (n == 3) {
		if (gameConfig.nControlType == CONTROL_WINJOYSTICK)
			for (i = 0, j = NUM_JOY_CONTROLS; i < j; i++)	
				controlSettings.custom [gameConfig.nControlType][i] = kcSuperJoy [i].value;
		}
	else if (n == 4) {
		for (i=0, j = NUM_HOTKEY_CONTROLS; i < j; i++)
			controlSettings.d2xCustom [i] = kcHotkeys [i].value;
		}
gameOpts->legacy.bInput = b;
}

//------------------------------------------------------------------------------

fix Last_angles_p = 0;
fix Last_angles_b = 0;
fix Last_angles_h = 0;
ubyte Last_angles_read = 0;

int VR_sense_range [3] = { 25, 50, 75 };

#if 0
read_head_tracker ()
{
	fix yaw, pitch, roll;
	int buttons;

//------ read vfx1 helmet --------
	if (vfx1_installed) {
		vfx_get_data (&yaw,&pitch,&roll,&buttons);
	} else if (iglasses_headset_installed)	{
		iglasses_read_headset (&yaw, &pitch, &roll);
	} else if (Victor_headset_installed)   {
		victor_read_headset_filtered (&yaw, &pitch, &roll);
	} else {
		return;
	}

	viewInfo.bUsePlayerHeadAngles = 0;
	if (Last_angles_read)	{
		fix yaw1 = yaw;
		
		yaw1 = yaw;
		if ((Last_angles_h < (F1_0/4)) && (yaw > ((F1_0*3)/4)))	
			yaw1 -= F1_0;
		else if ((yaw < (F1_0/4)) && (Last_angles_h > ((F1_0*3)/4)))	
			yaw1 += F1_0;
	
		Controls [0].pitchTime	+= FixMul ((pitch- Last_angles_p)*VR_sense_range [gameStates.render.vr.nSensitivity],gameData.time.xFrame);
		Controls [0].headingTime+= FixMul ((yaw1 -  Last_angles_h)*VR_sense_range [gameStates.render.vr.nSensitivity],gameData.time.xFrame);
		Controls [0].bankTime	+= FixMul ((roll - Last_angles_b)*VR_sense_range [gameStates.render.vr.nSensitivity],gameData.time.xFrame);
	}
	Last_angles_read = 1;
	Last_angles_p = pitch;
	Last_angles_h = yaw;
	Last_angles_b = roll;
}
#endif

//------------------------------------------------------------------------------

ubyte 			kc_use_external_control = 0;
ubyte				kc_enable_external_control = 0;
ubyte 			kc_external_intno = 0;
ext_control_info	*kc_external_control = NULL;
ubyte				*kc_external_name = NULL;
ubyte				kc_external_version = 0;

void KCInitExternalControls (int intno, int address)
{
	int i;
	kc_external_intno = intno;
	kc_external_control	= (ext_control_info *) (size_t) address;
	kc_use_external_control = 1;
	kc_enable_external_control  = 1;

	i = FindArg ("-xname");
	if (i)	
		kc_external_name = (ubyte *) Args [i+1];
	else
		kc_external_name = (ubyte *) "External Controller";

   for (i = 0; i < (int) strlen ((char *) kc_external_name);i++)
    if (kc_external_name [i]=='_')
	  kc_external_name [i]=' '; 

	i = FindArg ("-xver");
	if (i)
		kc_external_version = atoi (Args [i+1]);
}

/*void KCReadExternalControls ()
{
	union REGS r;

	if (!kc_enable_external_control && !gameStates.input.bCybermouseActive) 
		return;

	if (kc_external_version == 0) 
		memset (kc_external_control, 0, sizeof (tControlInfo);
	else if (kc_external_version > 0) 	{
		memset (kc_external_control, 0, sizeof (tControlInfo)+sizeof (vmsAngVec) + 64);
		if (kc_external_version > 1) {
			// Write ship pos and angles to external controls...
			ubyte *temp_ptr = (ubyte *)kc_external_control;
			vmsVector *ship_pos;
			vmsMatrix *ship_orient;
			memset (kc_external_control, 0, sizeof (tControlInfo)+sizeof (vmsAngVec) + 64 + sizeof (vmsVector)+sizeof (vmsMatrix);
			temp_ptr += sizeof (tControlInfo)+sizeof (vmsAngVec) + 64;
			ship_pos = (vmsVector *)temp_ptr;
			temp_ptr += sizeof (vmsVector);
			ship_orient = (vmsMatrix *)temp_ptr;
			// Fill in ship postion...
			*ship_pos = gameData.objs.objects [LOCALPLAYER.nObject].position.vPos;
			// Fill in ship orientation...
			*ship_orient = gameData.objs.objects [LOCALPLAYER.nObject].position.mOrient;
		}
	}

        if (gameStates.render.automap.bDisplay)                    // (If in automap...)
		kc_external_control->automapState = 1;
	memset (&r,0,sizeof (r);

   if (!gameStates.input.bCybermouseActive)
   	int386 (kc_external_intno, &r, &r);		// Read external info...
//	else
  //		ReadOWL (kc_external_control);

	if (gameData.multiplayer.nLocalPlayer > -1)	{
		gameData.objs.objects [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_TURNROLL);	// Turn off roll when turning
		gameData.objs.objects [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_LEVELLING);	// Turn off leveling to nearest tSide.
		gameOpts->gameplay.nAutoLeveling = 0;

		if (kc_external_version > 0) {		
			vmsMatrix tempm, ViewMatrix;
			vmsAngVec * Kconfig_abs_movement;
			char * oem_message;
	
			Kconfig_abs_movement = (vmsAngVec *) ((uint)kc_external_control + sizeof (tControlInfo);
	
			if (Kconfig_abs_movement->p || Kconfig_abs_movement->b || Kconfig_abs_movement->h)	{
				VmAngles2Matrix (&tempm,Kconfig_abs_movement);
				VmMatMul (&ViewMatrix,&gameData.objs.objects [LOCALPLAYER.nObject].position.mOrient,&tempm);
				gameData.objs.objects [LOCALPLAYER.nObject].position.mOrient = ViewMatrix;		
			}
			oem_message = (char *) ((uint)Kconfig_abs_movement + sizeof (vmsAngVec);
			if (oem_message [0] != '\0')
				HUDInitMessage (oem_message);
		}
	}

	Controls [0].pitchTime += FixMul (kc_external_control->pitchTime,gameData.time.xFrame);						
	Controls [0].verticalThrustTime += FixMul (kc_external_control->verticalThrustTime,gameData.time.xFrame);
	Controls [0].headingTime += FixMul (kc_external_control->headingTime,gameData.time.xFrame);
	Controls [0].sidewaysThrustTime += FixMul (kc_external_control->sidewaysThrustTime ,gameData.time.xFrame);
	Controls [0].bankTime += FixMul (kc_external_control->bankTime ,gameData.time.xFrame);
	Controls [0].forwardThrustTime += FixMul (kc_external_control->forwardThrustTime ,gameData.time.xFrame);
	Controls [0].rearViewDownCount += kc_external_control->rearViewDownCount;	
	Controls [0].rearViewDownState |= kc_external_control->rearViewDownState;	
	Controls [0].firePrimaryDownCount += kc_external_control->firePrimaryDownCount;
	Controls [0].firePrimaryState |= kc_external_control->firePrimaryState;
	Controls [0].fireSecondaryState |= kc_external_control->fireSecondaryState;
	Controls [0].fireSecondaryDownCount += kc_external_control->fireSecondaryDownCount;
	Controls [0].fireFlareDownCount += kc_external_control->fireFlareDownCount;
	Controls [0].dropBombDownCount += kc_external_control->dropBombDownCount;	
	Controls [0].automapDownCount += kc_external_control->automapDownCount;
	Controls [0].automapState |= kc_external_control->automapState;
} */

//------------------------------------------------------------------------------

void KCReadExternalControls ()
{
	//union REGS r;
   int i;

	if (!kc_enable_external_control) return;

	if (kc_external_version == 0) 
		memset (kc_external_control, 0, sizeof (ext_control_info));
	else if (kc_external_version > 0) 	{
    	
		if (kc_external_version>=4)
			memset (kc_external_control, 0, sizeof (advanced_ext_control_info));
      else if (kc_external_version>0)     
			memset (kc_external_control, 0, sizeof (ext_control_info)+sizeof (vmsAngVec) + 64);
		else if (kc_external_version>2)
			memset (kc_external_control, 0, sizeof (ext_control_info)+sizeof (vmsAngVec) + 64 + sizeof (vmsVector) + sizeof (vmsMatrix) +4);

		if (kc_external_version > 1) {
			// Write ship pos and angles to external controls...
			ubyte *temp_ptr = (ubyte *)kc_external_control;
			vmsVector *ship_pos;
			vmsMatrix *ship_orient;
			memset (kc_external_control, 0, sizeof (ext_control_info)+sizeof (vmsAngVec) + 64 + sizeof (vmsVector)+sizeof (vmsMatrix));
			temp_ptr += sizeof (ext_control_info) + sizeof (vmsAngVec) + 64;
			ship_pos = (vmsVector *)temp_ptr;
			temp_ptr += sizeof (vmsVector);
			ship_orient = (vmsMatrix *)temp_ptr;
			// Fill in ship postion...
			*ship_pos = gameData.objs.objects [LOCALPLAYER.nObject].position.vPos;
			// Fill in ship orientation...
			*ship_orient = gameData.objs.objects [LOCALPLAYER.nObject].position.mOrient;
		}
    if (kc_external_version>=4)
	  {
	   advanced_ext_control_info *temp_ptr= (advanced_ext_control_info *)kc_external_control;
 
      temp_ptr->headlightState= (LOCALPLAYER.flags & PLAYER_FLAGS_HEADLIGHT_ON);
		temp_ptr->primaryWeaponFlags=LOCALPLAYER.primaryWeaponFlags;
		temp_ptr->secondaryWeaponFlags=LOCALPLAYER.secondaryWeaponFlags;
      temp_ptr->currentPrimary_weapon=gameData.weapons.nPrimary;
      temp_ptr->currentSecondary_weapon=gameData.weapons.nSecondary;

      temp_ptr->current_guidebot_command=gameData.escort.nGoalObject;

	   temp_ptr->force_vector=ExtForceVec;
		temp_ptr->force_matrix=ExtApplyForceMatrix;
	   for (i=0;i<3;i++)
       temp_ptr->joltinfo [i]=ExtJoltInfo [i];  
      for (i=0;i<2;i++)
		   temp_ptr->x_vibrate_info [i]=ExtXVibrateInfo [i];
		temp_ptr->x_vibrate_clear=ExtXVibrateClear;
 	   temp_ptr->gameStatus=gameStates.app.nExtGameStatus;
   
      memset ((void *)&ExtForceVec,0,sizeof (vmsVector));
      memset ((void *)&ExtApplyForceMatrix,0,sizeof (vmsMatrix));
      
      for (i=0;i<3;i++)
		 ExtJoltInfo [i]=0;
      for (i=0;i<2;i++)
		 ExtXVibrateInfo [i]=0;
      ExtXVibrateClear=0;
     }
	}

	if (gameStates.render.automap.bDisplay)			// (If in automap...)
		kc_external_control->automapState = 1;
	//memset (&r,0,sizeof (r);

  #if 0
 
	int386 (kc_external_intno, &r, &r);		// Read external info...

  #endif 

	if (gameData.multiplayer.nLocalPlayer > -1)	{
		gameData.objs.objects [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_TURNROLL);	// Turn off roll when turning
		gameData.objs.objects [LOCALPLAYER.nObject].mType.physInfo.flags &= (~PF_LEVELLING);	// Turn off leveling to nearest tSide.
		gameOpts->gameplay.nAutoLeveling = 0;

		if (kc_external_version > 0) {		
			vmsMatrix tempm, ViewMatrix;
			vmsAngVec * Kconfig_abs_movement;
			char * oem_message;
	
			Kconfig_abs_movement = (vmsAngVec *) (size_t) ((size_t) kc_external_control + sizeof (ext_control_info));
	
			if (Kconfig_abs_movement->p || Kconfig_abs_movement->b || Kconfig_abs_movement->h)	{
				VmAngles2Matrix (&tempm,Kconfig_abs_movement);
				VmMatMul (&ViewMatrix,&gameData.objs.objects [LOCALPLAYER.nObject].position.mOrient,&tempm);
				gameData.objs.objects [LOCALPLAYER.nObject].position.mOrient = ViewMatrix;		
			}
			oem_message = (char *) (size_t) ((size_t)Kconfig_abs_movement + sizeof (vmsAngVec));
			if (oem_message [0] != '\0')
				HUDInitMessage (oem_message);
		}
	}

	Controls [0].pitchTime += FixMul (kc_external_control->pitchTime,gameData.time.xFrame);						
	Controls [0].verticalThrustTime += FixMul (kc_external_control->verticalThrustTime,gameData.time.xFrame);
	Controls [0].headingTime += FixMul (kc_external_control->headingTime,gameData.time.xFrame);
	Controls [0].sidewaysThrustTime += FixMul (kc_external_control->sidewaysThrustTime ,gameData.time.xFrame);
	Controls [0].bankTime += FixMul (kc_external_control->bankTime ,gameData.time.xFrame);
	Controls [0].forwardThrustTime += FixMul (kc_external_control->forwardThrustTime ,gameData.time.xFrame);
	Controls [0].rearViewDownCount += kc_external_control->rearViewDownCount;	
	Controls [0].rearViewDownState |= kc_external_control->rearViewDownState;	
	Controls [0].firePrimaryDownCount += kc_external_control->firePrimaryDownCount;
	Controls [0].firePrimaryState |= kc_external_control->firePrimaryState;
	Controls [0].fireSecondaryState |= kc_external_control->fireSecondaryState;
	Controls [0].fireSecondaryDownCount += kc_external_control->fireSecondaryDownCount;
	Controls [0].fireFlareDownCount += kc_external_control->fireFlareDownCount;
	Controls [0].dropBombDownCount += kc_external_control->dropBombDownCount;	
	Controls [0].automapDownCount += kc_external_control->automapDownCount;
	Controls [0].automapState |= kc_external_control->automapState;
	
   if (kc_external_version>=3)
	 {
		ubyte *temp_ptr = (ubyte *)kc_external_control;
		temp_ptr += (sizeof (ext_control_info) + sizeof (vmsAngVec) + 64 + sizeof (vmsVector) + sizeof (vmsMatrix));
  
	   if (* (temp_ptr))
		 Controls [0].cyclePrimaryCount= (* (temp_ptr));
	   if (* (temp_ptr+1))
		 Controls [0].cycleSecondaryCount= (* (temp_ptr+1));

		if (* (temp_ptr+2))
		 Controls [0].afterburnerState= (* (temp_ptr+2));
		if (* (temp_ptr+3))
		 Controls [0].headlightCount= (* (temp_ptr+3));
  	 }
   if (kc_external_version>=4)
	 {
     int i;
	  advanced_ext_control_info *temp_ptr= (advanced_ext_control_info *)kc_external_control;
     
     for (i=0;i<128;i++)
	   if (temp_ptr->keyboard [i])
			key_putkey ((char) i);

     if (temp_ptr->Reactor_blown)
      {
       if (gameData.app.nGameMode & GM_MULTI)
		    NetDestroyReactor (ObjFindFirstOfType (OBJ_CNTRLCEN));
		 else
			 DoReactorDestroyedStuff (ObjFindFirstOfType (OBJ_CNTRLCEN));
	   }
    }
  
}

//------------------------------------------------------------------------------

void KCSetControls (int bGet)
{
	int i, j;

SetControlType ();
return;
for (i = 0, j = NUM_KEY_CONTROLS; i < j; i++) {
	if (bGet)
		controlSettings.custom [0][i] = kcKeyboard [i].value;
	else
		kcKeyboard [i].value = controlSettings.custom [0][i];
	}
//if ((gameConfig.nControlType > 0) && (gameConfig.nControlType < 5)) {
if (gameOpts->input.joystick.bUse) {
	for (i = 0, j = NUM_JOY_CONTROLS; i < j; i++) {
		if (bGet)
			controlSettings.custom [gameStates.input.nJoyType][i] = kcJoystick [i].value;
		else {
			kcJoystick [i].value = controlSettings.custom [gameStates.input.nJoyType][i];
			if (kcJoystick [i].nType == BT_INVERT)	{
				if (kcJoystick [i].value != 1)
					kcJoystick [i].value = 0;
				controlSettings.custom [gameStates.input.nJoyType][i] = kcJoystick [i].value;
				}
			}
		}
	}
//else if (gameConfig.nControlType > 4 && gameConfig.nControlType < CONTROL_WINJOYSTICK) {
if (gameOpts->input.mouse.bUse) {
	for (i = 0, j = NUM_MOUSE_CONTROLS; i < j; i++)	{
		if (bGet)
			controlSettings.custom [gameStates.input.nMouseType][i] = kcMouse [i].value;
		else {
			kcMouse [i].value = controlSettings.custom [gameStates.input.nMouseType][i];
			if (kcMouse [i].nType == BT_INVERT)	{
				if (kcMouse [i].value != 1)
					kcMouse [i].value = 0;
				controlSettings.custom [gameStates.input.nMouseType][i] = kcMouse [i].value;
				}
			}
		}
	}
//else 
if (gameConfig.nControlType == CONTROL_WINJOYSTICK) {
	for (i = 0, j = NUM_JOY_CONTROLS; i < j; i++) {
		if (bGet)	
			controlSettings.custom [gameConfig.nControlType][i] = kcSuperJoy [i].value;
		else {
			kcSuperJoy [i].value = controlSettings.custom [gameConfig.nControlType][i];
			if (kcSuperJoy [i].nType == BT_INVERT)	{
				if (kcSuperJoy [i].value!=1)
					kcSuperJoy [i].value	= 0;
				controlSettings.custom [gameConfig.nControlType][i] = kcSuperJoy [i].value;
				}
			}
		}
	}
for (i = 0, j = NUM_HOTKEY_CONTROLS; i < j; i++) {
	if (bGet)
		controlSettings.d2xCustom [i] = kcHotkeys [i].value;
	else
		kcHotkeys [i].value = controlSettings.d2xCustom [i];
	}
}

//------------------------------------------------------------------------------

int KcKeyboardSize (void) {return sizeofa (kcKeyboard);}
int KcMouseSize (void) {return sizeofa (kcMouse);}
int KcJoystickSize (void) {return sizeofa (kcJoystick);}
int KcSuperJoySize (void) {return sizeofa (kcSuperJoy);}
int KcHotkeySize (void) {return sizeofa (kcHotkeys);}

//------------------------------------------------------------------------------
//eof
