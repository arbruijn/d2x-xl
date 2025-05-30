/*
 *
 * SDL tKeyboard input support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "descent.h"
#include "event.h"
#include "error.h"
#include "key.h"
#include "input.h"
#include "timer.h"
#include "hudmsgs.h"
#include "maths.h"

#define UNICODE_KEYS		0

#define KEY_BUFFER_SIZE 16

static uint8_t bInstalled = 0;

//-------- Variable accessed by outside functions ---------

typedef struct tKeyInfo {
	uint8_t		state;			// state of key 1 == down, 0 == up
	uint8_t		lastState;		// previous state of key
	int32_t		counter;		// incremented each time key is down in handler
	fix		timeWentDown;	// simple counter incremented each time in interrupt and key is down
	fix		timeHeldDown;	// counter to tell how long key is down -- gets reset to 0 by key routines
	uint8_t		downCount;		// number of key counts key was down
	uint8_t		upCount;		// number of times key was released
	uint8_t		flags;
} tKeyInfo;

typedef struct tKeyboard {
	uint16_t		keyBuffer [KEY_BUFFER_SIZE];
	tKeyInfo		keys [256];
	fix			xTimePressed [KEY_BUFFER_SIZE];
	uint32_t 			nKeyHead, nKeyTail;
} tKeyboard;

static tKeyboard keyData;

typedef struct tKeyProps {
	const char*	pszKeyText;
	wchar_t		asciiValue;
	wchar_t		shiftedAsciiValue;
	SDLKey		sym;
} tKeyProps;

tKeyProps keyProperties [] = {
{ "",       255,    255,    (SDLKey) -1        },
{ "ESC",    255,    255,    SDLK_ESCAPE        },
{ "1",      '1',    '!',    SDLK_1             },
{ "2",      '2',    '@',    SDLK_2             },
{ "3",      '3',    '#',    SDLK_3             },
{ "4",      '4',    '$',    SDLK_4             },
{ "5",      '5',    '%',    SDLK_5             },
{ "6",      '6',    '^',    SDLK_6             },
{ "7",      '7',    '&',    SDLK_7             },
{ "8",      '8',    '*',    SDLK_8             },
{ "9",      '9',    '(',    SDLK_9             },
{ "0",      '0',    ')',    SDLK_0             },
{ "-",      '-',    '_',    SDLK_MINUS         },
{ "=",      '=',    '+',    SDLK_EQUALS        },
{ "BSPC",   255,    255,    SDLK_BACKSPACE     },
{ "TAB",    255,    255,    SDLK_TAB           },
{ "Q",      'q',    'Q',    SDLK_q             },
{ "W",      'w',    'W',    SDLK_w             },
{ "E",      'e',    'E',    SDLK_e             },
{ "R",      'r',    'R',    SDLK_r             },
{ "T",      't',    'T',    SDLK_t             },
{ "Y",      'y',    'Y',    SDLK_y             },
{ "U",      'u',    'U',    SDLK_u             },
{ "I",      'i',    'I',    SDLK_i             },
{ "O",      'o',    'O',    SDLK_o             },
{ "P",      'p',    'P',    SDLK_p             },
{ "[",      '[',    '{',    SDLK_LEFTBRACKET   },
{ "]",      ']',    '}',    SDLK_RIGHTBRACKET  },
{ "RET",    255,    255,    SDLK_RETURN        },
{ "LCTRL",  255,    255,    SDLK_LCTRL         },
{ "A",      'a',    'A',    SDLK_a             },
{ "S",      's',    'S',    SDLK_s             },
{ "D",      'd',    'D',    SDLK_d             },
{ "F",      'f',    'F',    SDLK_f             },
{ "G",      'g',    'G',    SDLK_g             },
{ "H",      'h',    'H',    SDLK_h             },
{ "J",      'j',    'J',    SDLK_j             },
{ "K",      'k',    'K',    SDLK_k             },
{ "L",      'l',    'L',    SDLK_l             },
{ ";",      ';',    ':',    SDLK_SEMICOLON     },
{ "'",      '\'',   '"',    SDLK_QUOTE         },
{ "`",      '`',    '~',    SDLK_BACKQUOTE     },
{ "LSHFT",  255,    255,    SDLK_LSHIFT        },
{ "\\",     '\\',   '|',    SDLK_BACKSLASH     },
{ "Z",      'z',    'Z',    SDLK_z             },
{ "X",      'x',    'X',    SDLK_x             },
{ "C",      'c',    'C',    SDLK_c             },
{ "V",      'v',    'V',    SDLK_v             },
{ "B",      'b',    'B',    SDLK_b             },
{ "N",      'n',    'N',    SDLK_n             },
{ "M",      'm',    'M',    SDLK_m             },
{ ",",      ',',    '<',    SDLK_COMMA			  },
{ ".",      '.',    '>',    SDLK_PERIOD		  },
{ "/",      '/',    '?',    SDLK_SLASH			  },
{ "RSHFT",  255,    255,    SDLK_RSHIFT		  },
{ "PAD*",   '*',    255,    SDLK_KP_MULTIPLY   },
{ "LALT",   255,    255,    SDLK_LALT          },
{ "SPC",    ' ',    ' ',    SDLK_SPACE         },
{ "CPSLK",  255,    255,    SDLK_CAPSLOCK      },
{ "F1",     255,    255,    SDLK_F1            },
{ "F2",     255,    255,    SDLK_F2            },
{ "F3",     255,    255,    SDLK_F3            },
{ "F4",     255,    255,    SDLK_F4            },
{ "F5",     255,    255,    SDLK_F5            },
{ "F6",     255,    255,    SDLK_F6            },
{ "F7",     255,    255,    SDLK_F7            },
{ "F8",     255,    255,    SDLK_F8            },
{ "F9",     255,    255,    SDLK_F9            },
{ "F10",    255,    255,    SDLK_F10           },
{ "NMLCK",  255,    255,    SDLK_NUMLOCK       },
{ "SCLK",   255,    255,    SDLK_SCROLLOCK     },
{ "PAD7",   255,    255,    SDLK_KP7           },
{ "PAD8",   255,    255,    SDLK_KP8           },
{ "PAD9",   255,    255,    SDLK_KP9           },
{ "PAD-",   255,    255,    SDLK_KP_MINUS      },
{ "PAD4",   255,    255,    SDLK_KP4           },
{ "PAD5",   255,    255,    SDLK_KP5           },
{ "PAD6",   255,    255,    SDLK_KP6           },
{ "PAD+",   255,    255,    SDLK_KP_PLUS       },
{ "PAD1",   255,    255,    SDLK_KP1           },
{ "PAD2",   255,    255,    SDLK_KP2           },
{ "PAD3",   255,    255,    SDLK_KP3           },
{ "PAD0",   255,    255,    SDLK_KP0           },
{ "PAD.",   255,    255,    SDLK_KP_PERIOD     },
{ "ALTGR",  255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "F11",    255,    255,    SDLK_F11           },
{ "F12",    255,    255,    SDLK_F12           },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "PAUSE",  255,    255,    SDLK_PAUSE			  },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "PAD\x83",255,    255,    SDLK_KP_ENTER      },
{ "RCTRL",  255,    255,    SDLK_RCTRL         },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "PAD/",   255,    255,    SDLK_KP_DIVIDE     },
{ "",       255,    255,    (SDLKey) -1        },
{ "PRSCR",  255,    255,    SDLK_PRINT         },
{ "RALT",   255,    255,    SDLK_RALT          },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "HOME",   255,    255,    SDLK_HOME          },
{ "UP",		255,    255,    SDLK_UP            },
{ "PGUP",   255,    255,    SDLK_PAGEUP        },
{ "",       255,    255,    (SDLKey) -1        },
{ "LEFT",	255,    255,    SDLK_LEFT          },
{ "",       255,    255,    (SDLKey) -1        },
{ "RIGHT",	255,    255,    SDLK_RIGHT         },
{ "",       255,    255,    (SDLKey) -1        },
{ "END",    255,    255,    SDLK_END           },
{ "DOWN",	255,    255,    SDLK_DOWN          },
{ "PGDN",	255,    255,    SDLK_PAGEDOWN      },
{ "INS",		255,    255,    SDLK_INSERT        },
{ "DEL",		255,    255,    SDLK_DELETE        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "",       255,    255,    (SDLKey) -1        },
{ "RWIN",   255,    255,    SDLK_RSUPER        },
{ "LWIN",   255,    255,    SDLK_LSUPER        },
{ "RCMD",   255,    255,    SDLK_RMETA         },
{ "LCMD",   255,    255,    SDLK_LMETA         }
};

const char *pszKeyText [256];

//------------------------------------------------------------------------------

uint8_t KeyToASCII (int32_t keyCode)
{
	int32_t shifted;

shifted = keyCode & KEY_SHIFTED;
keyCode &= 0xFF;
return shifted ? static_cast<uint8_t> (keyProperties [keyCode].shiftedAsciiValue) : static_cast<uint8_t> (keyProperties [keyCode].asciiValue);
}

//------------------------------------------------------------------------------

#if UNICODE_KEYS == 0

static int32_t KeyGerman (int32_t keyCode)
{
if (keyCode == KEY_Z)
	return KEY_Y;
else if (keyCode == KEY_Y)
	return KEY_Z;
else if (keyCode == KEY_SEMICOLON)
	return -1;
else if (keyCode == KEY_EQUALS)
	return KEY_RAPOSTRO;
else if (keyCode == KEY_RAPOSTRO)
	return -1;
else if (keyCode == KEY_LBRACKET)
	return -1;
else if (keyCode == KEY_RBRACKET)
	return KEY_EQUALS;
else if (keyCode == KEY_SLASH)
	return KEY_MINUS;
else if (keyCode == KEY_BACKSLASH)
	return KEY_RAPOSTRO;
else if ((keyCode & KEY_SHIFTED) || ((gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT]))) {
	int32_t h = keyCode & 0xFF;
	if (h == KEY_7)
		return KEY_BACKSLASH;
	else if (h == KEY_0)
		return KEY_EQUALS;
	else if (h == KEY_COMMA)
		return KEY_SEMICOLON;
	else if (keyCode == KEY_PERIOD)
		return KEY_SEMICOLON;
	}
else if (gameStates.input.keys.pressed [KEY_LCTRL] && gameStates.input.keys.pressed [KEY_RALT]) {	//ALTGR
	if ((keyCode == KEY_7) || (keyCode == KEY_8))
		return KEY_LBRACKET;
	else if ((keyCode == KEY_9) || (keyCode == KEY_0))
		return KEY_RBRACKET;
	else if (keyCode == KEY_0)
		return KEY_EQUALS;
	else if (keyCode == KEY_MINUS)
		return KEY_BACKSLASH;
	}
return keyCode;
}

//------------------------------------------------------------------------------

static int32_t KeyFrench (int32_t keyCode)
{
if (keyCode == KEY_A)
	return KEY_Q;
else if (keyCode == KEY_Q)
	return KEY_A;
else if (keyCode == KEY_W)
	return KEY_Z;
else if (keyCode == KEY_Z)
	return KEY_W;
else if (keyCode == KEY_COMMA)
	return KEY_SEMICOLON;
else if (keyCode == KEY_PERIOD)
	return KEY_SEMICOLON;
else if (keyCode == KEY_SEMICOLON)
	return KEY_M;
else if (keyCode == KEY_M)
	return KEY_COMMA;
else if (keyCode == KEY_RBRACKET)
	return KEY_EQUALS;
else if (keyCode == KEY_BACKSLASH)
	return -1;
else if (keyCode == KEY_RAPOSTRO)
	return -1;
else if (keyCode == KEY_SLASH)
	return -1;
else if (keyCode == KEY_LBRACKET)
	return -1;
else if (keyCode == KEY_SLASH)
	return -1;
else if ((keyCode & KEY_SHIFTED) || ((gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT]))) {
	int32_t h = keyCode & 0xFF;
	if (h == KEY_COMMA)
		return KEY_PERIOD;
	else if (h == KEY_PERIOD)
		return KEY_SLASH;
	else if (h == KEY_COMMA)
		return KEY_SEMICOLON;
	}
else if (gameStates.input.keys.pressed [KEY_LCTRL] && gameStates.input.keys.pressed [KEY_RALT]) {	//ALTGR
	if ((keyCode == KEY_4) || (keyCode == KEY_5))
		return KEY_LBRACKET;
	else if ((keyCode == KEY_MINUS) || (keyCode == KEY_EQUALS))
		return KEY_RBRACKET;
	else if (keyCode == KEY_0)
		return KEY_EQUALS;
	else if (keyCode == KEY_8)
		return KEY_BACKSLASH;
	}
return keyCode;
}

//------------------------------------------------------------------------------

typedef struct tKeyMap {
	int32_t	inKey, outKey;
} tKeyMap;

static int32_t KeyDvorak (int32_t keyCode)
{
	static tKeyMap keyMap [] = {
		{KEY_A, KEY_A},
		{KEY_B, KEY_X},
		{KEY_C, KEY_J},
		{KEY_D, KEY_E},
		{KEY_E, KEY_PERIOD},
		{KEY_F, KEY_U},
		{KEY_G, KEY_I},
		{KEY_H, KEY_D},
		{KEY_I, KEY_C},
		{KEY_J, KEY_H},
		{KEY_K, KEY_T},
		{KEY_L, KEY_N},
		{KEY_M, KEY_M},
		{KEY_N, KEY_B},
		{KEY_O, KEY_R},
		{KEY_P, KEY_L},
		{KEY_Q, KEY_RAPOSTRO},
		{KEY_R, KEY_P},
		{KEY_S, KEY_O},
		{KEY_T, KEY_Y},
		{KEY_U, KEY_G},
		{KEY_V, KEY_K},
		{KEY_W, KEY_COMMA},
		{KEY_X, KEY_Q},
		{KEY_Y, KEY_F},
		{KEY_Z, KEY_SEMICOLON},
		{KEY_COMMA, KEY_W},
		{KEY_PERIOD, KEY_V},
		{KEY_SLASH, KEY_Z},
		{KEY_SEMICOLON, KEY_S},
		{KEY_LAPOSTRO, KEY_MINUS},
		{KEY_LBRACKET, KEY_SLASH},
		{KEY_RBRACKET, KEY_EQUALS},
		{KEY_MINUS, KEY_LBRACKET},
		{KEY_EQUALS, KEY_RBRACKET}
		};

	int32_t flags = keyCode & 0xff00;

keyCode &= 0xff;
for (int32_t i = 0, j = sizeofa (keyMap); i < j; i++)
	if (keyCode == keyMap [i].inKey)
		return keyMap [i].outKey;
return keyCode | flags;
}

#endif

//------------------------------------------------------------------------------

void KeyHandler (SDL_KeyboardEvent *event)
{
	uint8_t			state;
	int32_t			keyCode,
				keyState = (event->state == SDL_PRESSED),
				nKeyboard = gameOpts->input.keyboard.nType;
	SDLKey		keySym = event->keysym.sym;
#if UNICODE_KEYS
	wchar_t		unicode = event->keysym.unicode;
#endif
	tKeyInfo*	pKey;
	uint8_t			temp;

//=====================================================
//Here a translation from win keycodes to mac keycodes!
//=====================================================

for (int32_t i = 0, j = sizeofa (keyProperties); i < j; i++) {
#if UNICODE_KEYS == 0
	if (nKeyboard == 1) {
		if (-1 == (keyCode = KeyGerman (i)))
			continue;
		}
	else if (nKeyboard == 2) {
		if (-1 == (keyCode = KeyFrench (i)))
			continue;
		}
	else if (nKeyboard == 3) {
		if (-1 == (keyCode = KeyDvorak (i)))
			continue;
		}
	else
#endif
		keyCode = i;
	pKey = keyData.keys + keyCode;

#if UNICODE_KEYS
	if (unicode && (keyProperties [i].asciiValue != wchar_t (255))) {
		if ((keyProperties [i].asciiValue == unicode) || (keyProperties [i].shiftedAsciiValue == unicode))
#	if DBG
			if (pKey->lastState)
				state = keyState;
			else
#	endif
			state = keyState;
		else
			state = pKey->lastState;
		}
   else if (keyProperties [i].sym == keySym)
#else
   if (keyProperties [i].sym == keySym)
#endif
		state = keyState;
	else
		state = pKey->lastState;
	
	if (pKey->lastState == state) {
		if (state) {
			pKey->counter++;
			gameStates.input.keys.nLastPressed = keyCode;
			gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds ();
			pKey->flags = 0;
			if (gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT])
				pKey->flags |= uint8_t (KEY_SHIFTED / 256);
			if (gameStates.input.keys.pressed [KEY_LALT] || gameStates.input.keys.pressed [KEY_RALT])
				pKey->flags |= uint8_t (KEY_ALTED / 256);
			if ((/*(nKeyboard != 1) &&*/ gameStates.input.keys.pressed [KEY_LCTRL]) || gameStates.input.keys.pressed [KEY_RCTRL])
				pKey->flags |= uint8_t (KEY_CTRLED / 256);
			}
		}
	else {
		if (state) {
			gameStates.input.keys.nLastPressed = keyCode;
			pKey->timeWentDown = gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds();
			keyData.keys [keyCode].timeHeldDown = 0;
			gameStates.input.keys.pressed [keyCode] = 1;
			pKey->downCount += state;
			pKey->counter++;
			pKey->state = 1;
			pKey->flags = 0;
			if (gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT])
				pKey->flags |= uint8_t (KEY_SHIFTED / 256);
			if (gameStates.input.keys.pressed [KEY_LALT] || gameStates.input.keys.pressed [KEY_RALT])
				pKey->flags |= uint8_t (KEY_ALTED / 256);
			if ((/*(nKeyboard != 1) &&*/ gameStates.input.keys.pressed [KEY_LCTRL]) || gameStates.input.keys.pressed [KEY_RCTRL])
				pKey->flags |= uint8_t (KEY_CTRLED / 256);
//				pKey->timeWentDown = gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds();
			}
		else {
			gameStates.input.keys.pressed [keyCode] = 0;
			gameStates.input.keys.nLastReleased = keyCode;
			pKey->upCount += pKey->state;
			pKey->state = 0;
			pKey->counter = 0;
			pKey->timeHeldDown += TimerGetFixedSeconds () - pKey->timeWentDown;
			}
		}
	if (state && (!pKey->lastState || ((pKey->counter > 30) && (pKey->counter & 1)))) {
		if (gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT])
			keyCode |= KEY_SHIFTED;
		if (gameStates.input.keys.pressed [KEY_LALT] || gameStates.input.keys.pressed [KEY_RALT])
			keyCode |= KEY_ALTED;
		if ((/*(nKeyboard != 1) &&*/ gameStates.input.keys.pressed [KEY_LCTRL]) || gameStates.input.keys.pressed [KEY_RCTRL])
			keyCode |= KEY_CTRLED;
		if (gameStates.input.keys.pressed [KEY_LCMD] || gameStates.input.keys.pressed [KEY_RCMD])
			keyCode |= KEY_COMMAND;
#ifndef RELEASE
	   if (gameStates.input.keys.pressed [KEY_DELETE])
			keyCode |= KEYDBGGED;
#endif			
		temp = keyData.nKeyTail + 1;
		if (temp >= KEY_BUFFER_SIZE) 
			temp = 0;
		if (temp != keyData.nKeyHead) {
#if DBG
			if ((i == KEY_LSHIFT) || (i == KEY_RSHIFT) || 
				 (i == KEY_LALT) || (i == KEY_RALT) || 
				 (i == KEY_RCTRL) || (i == KEY_LCTRL))
				keyData.keyBuffer [keyData.nKeyTail] = keyCode;
			else
#endif
				keyData.keyBuffer [keyData.nKeyTail] = keyCode;
			keyData.xTimePressed [keyData.nKeyTail] = gameStates.input.keys.xLastPressTime;
			keyData.nKeyTail = temp;
			}
		}
	pKey->lastState = state;
	}
}

//------------------------------------------------------------------------------

void _CDECL_ KeyClose (void)
{
 bInstalled = 0;
}

//------------------------------------------------------------------------------

void KeyInit (void)
{
  int32_t i;
  
if (bInstalled) 
	return;
bInstalled = 1;
#if UNICODE_KEYS
SDL_EnableUNICODE (1);
#endif
gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds ();
gameStates.input.keys.nBufferType = 1;
gameStates.input.keys.bRepeat = 1;
for(i = 0; i < 255; i++)
	pszKeyText [i] = keyProperties [i].pszKeyText;
// Clear the tKeyboard array
KeyFlush ();
atexit (KeyClose);
}

//------------------------------------------------------------------------------

void KeyFlush (void)
{
 	int32_t i;

if (!bInstalled)
	KeyInit();
keyData.nKeyHead = keyData.nKeyTail = 0;
// clear the tKeyboard buffer
for (i = 0; i < KEY_BUFFER_SIZE; i++) {
	keyData.keyBuffer [i] = 0;
	keyData.xTimePressed [i] = 0;
	}
int32_t curtime = TimerGetFixedSeconds ();
for (i = 0; i < 256; i++) {
	gameStates.input.keys.pressed [i] = 0;
	keyData.keys [i].state = 1;
	keyData.keys [i].lastState = 0;
	keyData.keys [i].timeWentDown = curtime;
	keyData.keys [i].downCount = 0;
	keyData.keys [i].upCount = 0;
	keyData.keys [i].timeHeldDown = 0;
	keyData.keys [i].counter = 0;
	}
}

//------------------------------------------------------------------------------

int32_t KeyAddKey (int32_t n)
{
return (++n < KEY_BUFFER_SIZE) ? n : 0;
}

//------------------------------------------------------------------------------

int32_t KeyCheckChar (void)
{
event_poll (SDL_KEYDOWNMASK | SDL_KEYUPMASK);
return (keyData.nKeyTail != keyData.nKeyHead);
}

//------------------------------------------------------------------------------

int32_t KeyInKeyTime (fix * time)
{
	int32_t key = 0;
	int32_t bLegacy = gameOpts->legacy.bInput;

gameOpts->legacy.bInput = 1;

if (!bInstalled)
	KeyInit ();
event_poll (SDL_KEYDOWNMASK | SDL_KEYUPMASK);
if (keyData.nKeyTail != keyData.nKeyHead) {
	key = keyData.keyBuffer [keyData.nKeyHead];
	if ((key == KEY_CTRLED + KEY_ALTED + KEY_ENTER) || (key == KEY_ALTED + KEY_F4))
		exit (0);
	keyData.nKeyHead = KeyAddKey (keyData.nKeyHead);
	if (time)
		*time = keyData.xTimePressed [keyData.nKeyHead];
	}
else if (!time)
	G3_SLEEP (0);
gameOpts->legacy.bInput = bLegacy;
return key;
}

//------------------------------------------------------------------------------

int32_t KeyInKey (void)
{
return KeyInKeyTime (NULL);
}

//------------------------------------------------------------------------------

int32_t KeyPeekKey (void)
{
	int32_t key = 0;

event_poll (SDL_KEYDOWNMASK | SDL_KEYUPMASK);
if (keyData.nKeyTail != keyData.nKeyHead)
	key = keyData.keyBuffer [keyData.nKeyHead];
return key;
}

//------------------------------------------------------------------------------

int32_t KeyGetChar (void)
{
if (!bInstalled)
	return 0;
while (!KeyCheckChar ())
	;
return KeyInKey ();
}

//------------------------------------------------------------------------------

uint32_t KeyGetShiftStatus (void)
{
	uint32_t shift_status = 0;

if (gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT])
	shift_status |= KEY_SHIFTED;
if (gameStates.input.keys.pressed [KEY_LALT] || gameStates.input.keys.pressed [KEY_RALT])
	shift_status |= KEY_ALTED;
if (gameStates.input.keys.pressed [KEY_LCTRL] || gameStates.input.keys.pressed [KEY_RCTRL])
	shift_status |= KEY_CTRLED;
#if DBG
if (gameStates.input.keys.pressed [KEY_DELETE])
	shift_status |=KEYDBGGED;
#endif
return shift_status;
}

//------------------------------------------------------------------------------
// Returns the number of seconds this key has been down since last call.
fix KeyDownTime (int32_t scanCode)
{
	static fix lastTime = -1;
	fix timeDown, time, slack = 0;

if ((scanCode < 0) || (scanCode > 255))
	return 0;

if (!gameStates.input.keys.pressed [scanCode]) {
	timeDown = keyData.keys [scanCode].timeHeldDown;
	keyData.keys [scanCode].timeHeldDown = 0;
	} 
else {
	int64_t s, ms;
#if DBG
	if (scanCode == 72)
		scanCode = scanCode;
#endif
	time = TimerGetFixedSeconds();
	timeDown = time - keyData.keys [scanCode].timeWentDown;
	s = timeDown / 65536;
	ms = (timeDown & 0xFFFF);
	ms *= 1000;
	ms >>= 16;
	keyData.keys [scanCode].timeHeldDown += (int32_t) (s * 1000 + ms);
	// the following code takes care of clamping in KConfig.c::control_read_all()
	if (gameStates.input.bKeepSlackTime && (timeDown > controls.PollTime ())) {
		slack = (fix) (timeDown - controls.PollTime ());
		time -= slack + slack / 10;	// there is still some slack, so add an extra 10%
		if (time < lastTime)
			time = lastTime;
		timeDown = fix (controls.PollTime ());
		}
	keyData.keys [scanCode].timeWentDown = time;
	lastTime = time;
	if (timeDown && timeDown < controls.PollTime ())
		timeDown = fix (controls.PollTime ());
	}
return timeDown;
}

//------------------------------------------------------------------------------

uint32_t KeyDownCount (int32_t scanCode)
{
	int32_t n;

if ((scanCode<0)|| (scanCode>255)) return 0;

n = keyData.keys [scanCode].downCount;
keyData.keys [scanCode].downCount = 0;
keyData.keys [scanCode].flags = 0;
return n;
}

//------------------------------------------------------------------------------

uint8_t KeyFlags (int32_t scanCode)
{
if ((scanCode < 0)|| (scanCode > 255)) 
	return 0;
return keyData.keys [scanCode].flags;
}

//------------------------------------------------------------------------------

uint32_t KeyUpCount (int32_t scanCode)
{
	int32_t n;

if ((scanCode < 0)|| (scanCode > 255)) 
	return 0;
n = keyData.keys [scanCode].upCount;
keyData.keys [scanCode].upCount = 0;
return n;
}

//------------------------------------------------------------------------------

fix KeyRamp (int32_t scanCode)
{
if (!gameOpts->input.keyboard.nRamp)
	return 1;
else {
		int32_t maxRampTime = gameOpts->input.keyboard.nRamp * 20; // / gameOpts->input.keyboard.nRamp;
		fix t = keyData.keys [scanCode].timeHeldDown;

	if (!t)
		return maxRampTime;
	if (t >= maxRampTime)
		return 1;
	t = maxRampTime / t;
	return t ? t : 1;
	}
}

//------------------------------------------------------------------------------
//eof
