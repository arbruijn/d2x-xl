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

#ifndef _KEY_H
#define _KEY_H 

#include "pstypes.h"
#include "fix.h"

#ifdef __macosx__
# include <SDL/SDL_keysym.h>
#else
# include <SDL_keysym.h>
#endif

//==========================================================================
// This installs the int9 vector and initializes the keyboard in buffered
// ASCII mode. KeyClose simply undoes that.
extern void KeyInit();
extern void _CDECL_ KeyClose(void);

//==========================================================================
// These are configuration parameters to setup how the buffer works.
// set keyd_bufferType to 0 for no key buffering.
// set it to 1 and it will buffer scancodes.
extern ubyte keyd_bufferType;

// keyd_editor_mode... 0=game mode, 1=editor mode.
// Editor mode makes KeyDownTime always return 0 if modifiers are down.
extern ubyte keyd_editor_mode;	

// Time in seconds when last key was pressed...
extern volatile int xLastKeyPressTime;

//==========================================================================
// These are the "buffered" keypress routines.  Use them by setting the
// "keyd_bufferType" variable.

extern void KeyPutKey (ushort); // simulates a keystroke
extern void KeyFlush();    // Clears the 256 char buffer
extern int KeyCheckChar();   // Returns 1 if a char is waiting
extern int KeyGetChar();     // Gets key if one waiting other waits for one.
extern int KeyInKey();     // Gets key if one, other returns 0.
extern int KeyInKeyTime(fix *time);     // Same as inkey, but returns the time the key was pressed down.
extern int KeyPeekKey();   // Same as inkey, but doesn't remove key from buffer.

extern ubyte KeyToASCII(int keycode );

extern void key_debug();    // Does an INT3

//==========================================================================
// These are the unbuffered routines. Index by the keyboard scancode.

// Set to 1 if the key is currently down, else 0
extern volatile ubyte keyd_pressed[];
extern volatile ubyte keydFlags[];
extern volatile ubyte keyd_last_pressed;
extern volatile ubyte keyd_last_released;

// Returns the seconds this key has been down since last call.
extern fix KeyDownTime(int scancode);

// Returns number of times key has went from up to down since last call.
extern uint KeyDownCount(int scancode);

extern ubyte KeyFlags(int scancode);
// Returns number of times key has went from down to up since last call.
extern uint KeyUpCount(int scancode);

// Clears the times & counts used by the above functions
// Took out... use KeyFlush();
//void key_clearTimes();
//void key_clearCounts();

extern const char * pszKeyText[256];

#define KEY_SHIFTED     KMOD_SHIFT //0x100
#define KEY_ALTED       KMOD_ALT //0x200
#define KEY_CTRLED      KMOD_CTRL //0x400
#define KEY_COMMAND     0x1000
#define KEYDBGGED			0x800

#define KEY_0           SDLK_0 //0x0B
#define KEY_1           SDLK_1 //0x02
#define KEY_2           SDLK_2 //0x03
#define KEY_3           SDLK_3 //0x04
#define KEY_4           SDLK_4 //0x05
#define KEY_5           SDLK_5 //0x06
#define KEY_6           SDLK_6 //0x07
#define KEY_7           SDLK_7 //0x08
#define KEY_8           SDLK_8 //0x09
#define KEY_9           SDLK_9 //0x0A

#define KEY_A           SDLK_a //0x1E
#define KEY_B           SDLK_b //0x30
#define KEY_C           SDLK_c //0x2E
#define KEY_D           SDLK_d //0x20
#define KEY_E           SDLK_e //0x12
#define KEY_F           SDLK_f //0x21
#define KEY_G           SDLK_g //0x22
#define KEY_H           SDLK_h //0x23
#define KEY_I           SDLK_i //0x17
#define KEY_J           SDLK_j //0x24
#define KEY_K           SDLK_k //0x25
#define KEY_L           SDLK_l //0x26
#define KEY_M           SDLK_m //0x32
#define KEY_N           SDLK_n //0x31
#define KEY_O           SDLK_o //0x18
#define KEY_P           SDLK_p //0x19
#define KEY_Q           SDLK_q //0x10
#define KEY_R           SDLK_r //0x13
#define KEY_S           SDLK_s //0x1F
#define KEY_T           SDLK_t //0x14
#define KEY_U           SDLK_u //0x16
#define KEY_V           SDLK_v //0x2F
#define KEY_W           SDLK_w //0x11
#define KEY_X           SDLK_x //0x2D
#define KEY_Y           SDLK_y //0x15
#define KEY_Z           SDLK_z //0x2C

#define KEY_MINUS       SDLK_MINUS //0x0C
#define KEY_EQUAL       SDLK_EQUALS //0x0D
//Note: this is what we normally think of as slash:
#define KEY_DIVIDE      SDLK_SLASH //0x35
//Note: this is BACKslash:
#define KEY_SLASH       SDLK_SLASH //0x2B
#define KEY_COMMA       SDLK_COMMA //0x33
#define KEY_PERIOD      SDLK_PERIOD //0x34
#define KEY_SEMICOL     SDLK_SEMICOLON //0x27

#define KEY_LBRACKET    SDLK_LEFTBRACKET //0x1A
#define KEY_RBRACKET    SDLK_RIGHTBRACKET //0x1B

#define KEY_RAPOSTRO    SDLK_QUOTE //0x28
#define KEY_LAPOSTRO    SDLK_BACKQUOTE //0x29

#define KEY_ESC         SDLK_ESCAPE //0x01
#define KEY_ENTER       SDLK_RETURN //0x1C
#define KEY_BACKSP      SDLK_BACKSPACE //0x0E
#define KEY_TAB         SDLK_TAB //0x0F
#define KEY_SPACEBAR    SDLK_SPACE //0x39

#define KEY_NUMLOCK     SDLK_NUMLOCK //0x45
#define KEY_CAPSLOCK    SDLK_CAPSLOCK //0x3A
#define KEY_SCROLLOCK   SDLK_SCROLLOCK //0x46

#define KEY_LSHIFT      SDLK_LSHIFT //0x2A
#define KEY_RSHIFT      SDLK_RSHIFT //0x36

#define KEY_LALT        SDLK_LALT //0x38
#define KEY_RALT        SDLK_RALT //0xB8

#define KEY_LCTRL       SDLK_LCTRL //0x1D
#define KEY_RCTRL       SDLK_RCTRL //0x9D

#define KEY_F1          SDLK_F1 //0x3B
#define KEY_F2          SDLK_F2 //0x3C
#define KEY_F3          SDLK_F3 //0x3D
#define KEY_F4          SDLK_F4 //0x3E
#define KEY_F5          SDLK_F5 //0x3F
#define KEY_F6          SDLK_F6 //0x40
#define KEY_F7          SDLK_F7 //0x41
#define KEY_F8          SDLK_F8 //0x42
#define KEY_F9          SDLK_F9 //0x43
#define KEY_F10         SDLK_F10 //0x44
#define KEY_F11         SDLK_F11 //0x57
#define KEY_F12         SDLK_F12 //0x58

#define KEY_PAD0        SDLK_KP0 //0x52
#define KEY_PAD1        SDLK_KP1 //0x4F
#define KEY_PAD2        SDLK_KP2 //0x50
#define KEY_PAD3        SDLK_KP3 //0x51
#define KEY_PAD4        SDLK_KP4 //0x4B
#define KEY_PAD5        SDLK_KP5 //0x4C
#define KEY_PAD6        SDLK_KP6 //0x4D
#define KEY_PAD7        SDLK_KP7 //0x47
#define KEY_PAD8        SDLK_KP8 //0x48
#define KEY_PAD9        SDLK_KP9 //0x49
#define KEY_PADMINUS    SDLK_KP_MINUS //0x4A
#define KEY_PADPLUS     SDLK_KP_PLUS //0x4E
#define KEY_PADPERIOD   SDLK_KP_PERIOD //0x53
#define KEY_PADDIVIDE   SDLK_KP_DIVIDE //0xB5
#define KEY_PADMULTIPLY SDLK_KP_MULTIPLY //0x37
#define KEY_PADENTER    SDLK_KP_ENTER //0x9C

#define KEY_INSERT      SDLK_INSERT //0xD2
#define KEY_HOME        SDLK_HOME //0xC7
#define KEY_PAGEUP      SDLK_PAGEUP //0xC9
#define KEY_DELETE      SDLK_DELETE //0xD3
#define KEY_END         SDLK_END //0xCF
#define KEY_PAGEDOWN    SDLK_PAGEDOWN //0xD1
#define KEY_UP          SDLK_UP //0xC8
#define KEY_DOWN        SDLK_DOWN //0xD0
#define KEY_LEFT        SDLK_LEFT //0xCB
#define KEY_RIGHT       SDLK_RIGHT //0xCD

#define KEY_LCMD        SDLK_ //0xFD
#define KEY_RCMD        SDLK_ //0xFE

#if 0//def _WIN32
#	define KEY_PRINT_SCREEN	(KEY_ALTED + KEY_F9)
#else
#	define KEY_PRINT_SCREEN	0xB7
#endif
#define KEY_PAUSE			SDLK_PRINT //0x61

#endif
