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

#include "inferno.h"
#include "event.h"
#include "error.h"
#include "key.h"
#include "timer.h"
#include "hudmsg.h"
#include "maths.h"

#define KEY_BUFFER_SIZE 16

static unsigned char bInstalled = 0;

//-------- Variable accessed by outside functions ---------

typedef struct tKeyInfo {
	ubyte		state;			// state of key 1 == down, 0 == up
	ubyte		lastState;		// previous state of key
	int		counter;		// incremented each time key is down in handler
	fix		timewentdown;	// simple counter incremented each time in interrupt and key is down
	fix		timehelddown;	// counter to tell how long key is down -- gets reset to 0 by key routines
	ubyte		downcount;		// number of key counts key was down
	ubyte		upcount;		// number of times key was released
	ubyte		flags;
} tKeyInfo;

typedef struct tKeyboard	{
	unsigned short		keybuffer [KEY_BUFFER_SIZE];
	tKeyInfo				keys [256];
	fix					xTimePressed [KEY_BUFFER_SIZE];
	unsigned int 		nKeyHead, nKeyTail;
} tKeyboard;

static tKeyboard keyData;

typedef struct tKeyProps {
	const char *pszKeyText;
	unsigned char asciiValue;
	unsigned char shiftedAsciiValue;
	SDLKey sym;
} tKeyProps;

tKeyProps keyProperties [256] = {
{ "",       255,    255,    (SDLKey) -1                 },
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
//edited 06/08/99 Matt Mueller - set to correct text
{ "�",      255,    255,    SDLK_RETURN        },
//end edit -MM
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
//edited 06/08/99 Matt Mueller - set to correct sym
{ ";",      ';',    ':',    SDLK_SEMICOLON     },
//end edit -MM
{ "'",      '\'',   '"',    SDLK_QUOTE         },
//edited 06/08/99 Matt Mueller - set to correct sym
{ "`",      '`',    '~',    SDLK_BACKQUOTE     },
//end edit -MM
{ "LSHFT",  255,    255,    SDLK_LSHIFT        },
{ "\\",     '\\',   '|',    SDLK_BACKSLASH     },
{ "Z",      'z',    'Z',    SDLK_z             },
{ "X",      'x',    'X',    SDLK_x             },
{ "C",      'c',    'C',    SDLK_c             },
{ "V",      'v',    'V',    SDLK_v             },
{ "B",      'b',    'B',    SDLK_b             },
{ "N",      'n',    'N',    SDLK_n             },
{ "M",      'm',    'M',    SDLK_m             },
//edited 06/08/99 Matt Mueller - set to correct syms
{ ",",      ',',    '<',    SDLK_COMMA			  },
{ ".",      '.',    '>',    SDLK_PERIOD		  },
{ "/",      '/',    '?',    SDLK_SLASH			  },
//end edit -MM
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
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "F11",    255,    255,    SDLK_F11           },
{ "F12",    255,    255,    SDLK_F12           },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
//edited 06/08/99 Matt Mueller - add pause ability
{ "PAUSE",       255,    255,    SDLK_PAUSE    },
//end edit -MM
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
//edited 06/08/99 Matt Mueller - set to correct pszKeyText
{ "PAD�",   255,    255,    SDLK_KP_ENTER      },
//end edit -MM
//edited 06/08/99 Matt Mueller - set to correct sym
{ "RCTRL",  255,    255,    SDLK_RCTRL         },
//end edit -MM
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "PAD/",   255,    255,    SDLK_KP_DIVIDE     },
{ "",       255,    255,    (SDLKey) -1                 },
//edited 06/08/99 Matt Mueller - add printscreen ability
{ "PRSCR",  255,    255,    SDLK_PRINT         },
//end edit -MM
{ "RALT",   255,    255,    SDLK_RALT          },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "HOME",   255,    255,    SDLK_HOME          },
//edited 06/08/99 Matt Mueller - set to correct pszKeyText
{ "UP",		255,    255,    SDLK_UP            },
//end edit -MM
{ "PGUP",   255,    255,    SDLK_PAGEUP        },
{ "",       255,    255,    (SDLKey) -1                 },
//edited 06/08/99 Matt Mueller - set to correct pszKeyText
{ "LEFT",	255,    255,    SDLK_LEFT          },
//end edit -MM
{ "",       255,    255,    (SDLKey) -1                 },
//edited 06/08/99 Matt Mueller - set to correct pszKeyText
{ "RIGHT",	255,    255,    SDLK_RIGHT         },
//end edit -MM
{ "",       255,    255,    (SDLKey) -1                 },
//edited 06/08/99 Matt Mueller - set to correct pszKeyText
{ "END",    255,    255,    SDLK_END           },
//end edit -MM
{ "DOWN",	255,    255,    SDLK_DOWN          },
{ "PGDN",	255,    255,    SDLK_PAGEDOWN      },
{ "INS",	255,    255,    SDLK_INSERT        },
{ "DEL",	255,    255,    SDLK_DELETE        },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "",       255,    255,    (SDLKey) -1                 },
{ "RWIN",   255,    255,    SDLK_RSUPER        },
{ "LWIN",   255,    255,    SDLK_LSUPER        },
{ "RCMD",   255,    255,    SDLK_RMETA         },
{ "LCMD",   255,    255,    SDLK_LMETA         }
};

const char *pszKeyText [256];

//------------------------------------------------------------------------------

unsigned char KeyToASCII(int keycode )
{
	int shifted;

shifted = keycode & KEY_SHIFTED;
keycode &= 0xFF;
return shifted ? keyProperties [keycode].shiftedAsciiValue : keyProperties [keycode].asciiValue;
}

//------------------------------------------------------------------------------

void KeyHandler(SDL_KeyboardEvent *event)
{
	ubyte				state;
	int				i, keycode, event_key, keyState;
	tKeyInfo			*key;
	unsigned char	temp;

   event_key = event->keysym.sym;
	keyState = (event->state == SDL_PRESSED); //  !(wInfo & KF_UP);
	//=====================================================
	//Here a translation from win keycodes to mac keycodes!
	//=====================================================
	for (i = 255; i >= 0; i--) {
		keycode = i;
		key = keyData.keys + keycode;
      if (keyProperties [i].sym == event_key)
			state = keyState;
		else
			state = key->lastState;
		
		if ( key->lastState == state )	{
			if (state) {
				key->counter++;
				gameStates.input.keys.nLastPressed = keycode;
				gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds();
				key->flags = 0;
				if (gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT])
					key->flags |= (ubyte) (KEY_SHIFTED / 256);
				if (gameStates.input.keys.pressed [KEY_LALT] || gameStates.input.keys.pressed [KEY_RALT])
					key->flags |= (ubyte) (KEY_ALTED / 256);
				if (gameStates.input.keys.pressed [KEY_LCTRL] || gameStates.input.keys.pressed [KEY_RCTRL])
					key->flags |= (ubyte) (KEY_CTRLED / 256);
				}
			}
		else {
			if (state) {
				gameStates.input.keys.nLastPressed = keycode;
				key->timewentdown = gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds();
				keyData.keys [keycode].timehelddown = 0;
				gameStates.input.keys.pressed [keycode] = 1;
				key->downcount += state;
				key->counter++;
				key->state = 1;
				key->flags = 0;
				if (gameStates.input.keys.pressed [KEY_LSHIFT] || gameStates.input.keys.pressed [KEY_RSHIFT])
					key->flags |= (ubyte) (KEY_SHIFTED / 256);
				if (gameStates.input.keys.pressed [KEY_LALT] || gameStates.input.keys.pressed [KEY_RALT])
					key->flags |= (ubyte) (KEY_ALTED / 256);
				if (gameStates.input.keys.pressed [KEY_LCTRL] || gameStates.input.keys.pressed [KEY_RCTRL])
					key->flags |= (ubyte) (KEY_CTRLED / 256);
//				key->timewentdown = gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds();
			} else {
				gameStates.input.keys.pressed [keycode] = 0;
				gameStates.input.keys.nLastReleased = keycode;
				key->upcount += key->state;
				key->state = 0;
				key->counter = 0;
				key->timehelddown += TimerGetFixedSeconds() - key->timewentdown;
			}
		}
		if ( (state && !key->lastState) || (state && key->lastState && (key->counter > 30) && (key->counter & 0x01)) ) {
			if ( gameStates.input.keys.pressed[KEY_LSHIFT] || gameStates.input.keys.pressed[KEY_RSHIFT])
				keycode |= KEY_SHIFTED;
			if ( gameStates.input.keys.pressed[KEY_LALT] || gameStates.input.keys.pressed[KEY_RALT])
				keycode |= KEY_ALTED;
			if ( gameStates.input.keys.pressed[KEY_LCTRL] || gameStates.input.keys.pressed[KEY_RCTRL])
				keycode |= KEY_CTRLED;
			if ( gameStates.input.keys.pressed[KEY_LCMD] || gameStates.input.keys.pressed[KEY_RCMD])
				keycode |= KEY_COMMAND;
#if DBG
      if ( gameStates.input.keys.pressed[KEY_DELETE] )
				keycode |= KEYDBGGED;
#endif			
			temp = keyData.nKeyTail+1;
			if ( temp >= KEY_BUFFER_SIZE ) temp=0;
			if (temp!=keyData.nKeyHead)	{
				keyData.keybuffer[keyData.nKeyTail] = keycode;
				keyData.xTimePressed[keyData.nKeyTail] = gameStates.input.keys.xLastPressTime;
				keyData.nKeyTail = temp;
			}
		}
		key->lastState = state;
	}
}

//------------------------------------------------------------------------------

void _CDECL_ KeyClose(void)
{
 bInstalled = 0;
}

//------------------------------------------------------------------------------

void KeyInit()
{
  int i;
  
if (bInstalled) 
	return;
bInstalled = 1;
gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds();
gameStates.input.keys.nBufferType = 1;
gameStates.input.keys.bRepeat = 1;
for(i = 0; i < 256; i++)
	pszKeyText [i] = keyProperties [i].pszKeyText;
// Clear the tKeyboard array
KeyFlush();
atexit(KeyClose);
}

//------------------------------------------------------------------------------

void KeyFlush()
{
 	int i;
	fix curtime;

	if (!bInstalled)
		KeyInit();

	keyData.nKeyHead = keyData.nKeyTail = 0;

	//Clear the tKeyboard buffer
	for (i=0; i<KEY_BUFFER_SIZE; i++ )	{
		keyData.keybuffer[i] = 0;
		keyData.xTimePressed[i] = 0;
	}

//use gettimeofday here:
	curtime = TimerGetFixedSeconds();

	for (i=0; i<256; i++ )	{
		gameStates.input.keys.pressed[i] = 0;
		keyData.keys[i].state = 1;
		keyData.keys[i].lastState = 0;
		keyData.keys[i].timewentdown = curtime;
		keyData.keys[i].downcount=0;
		keyData.keys[i].upcount=0;
		keyData.keys[i].timehelddown = 0;
		keyData.keys[i].counter = 0;
	}
}

//------------------------------------------------------------------------------

int KeyAddKey(int n)
{
 n++;
 if ( n >= KEY_BUFFER_SIZE ) n=0;
 return n;
}

//------------------------------------------------------------------------------

int KeyCheckChar()
{
	int is_one_waiting = 0;
	event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
	if (keyData.nKeyTail!=keyData.nKeyHead)
		is_one_waiting = 1;
	return is_one_waiting;
}

//------------------------------------------------------------------------------

int KeyInKey()
{
	int key = 0;
	int b = gameOpts->legacy.bInput;
	gameOpts->legacy.bInput = 1;
	if (!bInstalled)
		KeyInit();
   event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
	if (keyData.nKeyTail!=keyData.nKeyHead) {
		key = keyData.keybuffer[keyData.nKeyHead];
		keyData.nKeyHead = KeyAddKey(keyData.nKeyHead);
		if (key == KEY_CTRLED+KEY_ALTED+KEY_ENTER)
			exit (0);
	}
//added 9/3/98 by Matt Mueller to D2_FREE cpu time instead of hogging during menus and such
	else TimerDelay(1);
//end addition - Matt Mueller
	gameOpts->legacy.bInput = b;
	return key;
}

//------------------------------------------------------------------------------

int KeyInKeyTime(fix * time)
{
	int key = 0;

	if (!bInstalled)
		KeyInit();
        event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
	if (keyData.nKeyTail!=keyData.nKeyHead)	{
		key = keyData.keybuffer[keyData.nKeyHead];
		*time = keyData.xTimePressed[keyData.nKeyHead];
		keyData.nKeyHead = KeyAddKey(keyData.nKeyHead);
	}
	if (key == KEY_CTRLED+KEY_ALTED+KEY_ENTER)
		exit (0);
	return key;
}

//------------------------------------------------------------------------------

int KeyPeekKey()
{
	int key = 0;
        event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
	if (keyData.nKeyTail!=keyData.nKeyHead)
		key = keyData.keybuffer[keyData.nKeyHead];

	return key;
}

//------------------------------------------------------------------------------

int KeyGetChar()
{
	int dummy=0;

	if (!bInstalled)
		return 0;
//		return getch();

	while (!KeyCheckChar())
		dummy++;
	return KeyInKey();
}

//------------------------------------------------------------------------------

unsigned int KeyGetShiftStatus()
{
	unsigned int shift_status = 0;

	if ( gameStates.input.keys.pressed[KEY_LSHIFT] || gameStates.input.keys.pressed[KEY_RSHIFT] )
		shift_status |= KEY_SHIFTED;

	if ( gameStates.input.keys.pressed[KEY_LALT] || gameStates.input.keys.pressed[KEY_RALT] )
		shift_status |= KEY_ALTED;

	if ( gameStates.input.keys.pressed[KEY_LCTRL] || gameStates.input.keys.pressed[KEY_RCTRL] )
		shift_status |= KEY_CTRLED;

#if DBG
	if (gameStates.input.keys.pressed[KEY_DELETE])
		shift_status |=KEYDBGGED;
#endif

	return shift_status;
}

//------------------------------------------------------------------------------
// Returns the number of seconds this key has been down since last call.
fix KeyDownTime(int scancode)
{
	static fix lastTime = -1;
	fix timeDown, time, slack = 0;
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
	event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
#endif
   if ((scancode<0)|| (scancode>255)) return 0;

	if (!gameStates.input.keys.pressed[scancode]) {
		timeDown = keyData.keys[scancode].timehelddown;
		keyData.keys[scancode].timehelddown = 0;
	} else {
		QLONG s, ms;

		time = TimerGetFixedSeconds();
		timeDown = time - keyData.keys[scancode].timewentdown;
		s = timeDown / 65536;
		ms = (timeDown & 0xFFFF);
		ms *= 1000;
		ms >>= 16;
		keyData.keys[scancode].timehelddown += (int) (s * 1000 + ms);
		// the following code takes care of clamping in KConfig.c::control_read_all()
		if (gameStates.input.bKeepSlackTime && (timeDown > gameStates.input.kcPollTime)) {
			slack = (fix) (timeDown - gameStates.input.kcPollTime);
			time -= slack + slack / 10;	// there is still some slack, so add an extra 10%
			if (time < lastTime)
				time = lastTime;
			timeDown = (fix) gameStates.input.kcPollTime;
			}
		keyData.keys[scancode].timewentdown = time;
		lastTime = time;
if (timeDown && timeDown < gameStates.input.kcPollTime)
	timeDown = (fix) gameStates.input.kcPollTime;
	}
	return timeDown;
}

//------------------------------------------------------------------------------

unsigned int KeyDownCount(int scancode)
{
	int n;
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
	event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
#endif
   if ((scancode<0)|| (scancode>255)) return 0;

	n = keyData.keys[scancode].downcount;
	keyData.keys[scancode].downcount = 0;
	keyData.keys[scancode].flags = 0;
	return n;
}

//------------------------------------------------------------------------------

ubyte KeyFlags (int scancode)
{
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
	event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
#endif
   if ((scancode<0)|| (scancode>255)) return 0;
	return keyData.keys[scancode].flags;
}

//------------------------------------------------------------------------------

unsigned int KeyUpCount(int scancode)
{
	int n;
#ifndef FAST_EVENTPOLL
if (!bFastPoll)
	event_poll(SDL_KEYDOWNMASK | SDL_KEYUPMASK);
#endif
        if ((scancode<0)|| (scancode>255)) return 0;

	n = keyData.keys[scancode].upcount;
	keyData.keys[scancode].upcount = 0;

	return n;
}

//------------------------------------------------------------------------------

fix KeyRamp (int scancode)
{
if (!gameOpts->input.keyboard.nRamp)
	return 1;
else {
		int maxRampTime = gameOpts->input.keyboard.nRamp * 20; // / gameOpts->input.keyboard.nRamp;
		fix t = keyData.keys [scancode].timehelddown;

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
