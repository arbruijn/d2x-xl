#ifndef CON_console_H
#define CON_console_H

/*! \mainpage
 
\section intro Introduction
SDL_Console is a console that can be added to any SDL application. It is similar to Quake and other games consoles.
A console is meant to be a very simple way of interacting with a program and executing commands. You can also have 
more than one console at a time. 
 
\section docs Documentation
For a detailed description of all functions see \ref CON_console.h. Remark that functions that have the mark "Internal" 
are only used internally. There's not much use of calling these functions.
 
Have Fun!
 
\author Garett Banuk <mongoose@mongeese.org> (Original Version)
\author Clemens Wacha <reflex-2000@gmx.net> (Version 2.x, Documentation)
\author Boris Lesner <talanthyr@tuxfamily.org> (Package Maintainer)
\author Bradley Bell <btb@icculus.org> (Descent Version)
*/

#include "gr.h"
#include "key.h"
#include "cvar.h"

//! Cut the buffer line if it becomes longer than this
#define CON_CHARS_PER_LINE   128
//! Cursor blink frequency in ms
#define CON_BLINK_RATE       500
//! Border in pixels from the most left to the first letter
#define CON_CHAR_BORDER      4
//! Spacing in pixels between lines
#define CON_LINE_SPACE       1
//! Default prompt used at the commandline
#define CON_DEFAULT_PROMPT	"]"
//! Scroll this many lines at a time (when pressing PGUP or PGDOWN)
#define CON_LINE_SCROLL	2
//! Indicator showing that you scrolled up the history
#define CON_SCROLL_INDICATOR "^"
//! Cursor shown if we are in insert mode
#define CON_INS_CURSOR "_"
//! Cursor shown if we are in overwrite mode
#define CON_OVR_CURSOR "|"
//! Defines the default hide key (Hide () the console if pressed)
#define CON_DEFAULT_HIDEKEY	KEY_SHIFTED+KEY_ESC
//! Defines the opening/closing speed
#define CON_OPENCLOSE_SPEED 25

#define CON_NUM_LINES 400

enum {
    CON_CLOSED,	//! The console is closed (and not shown)
    CON_CLOSING,	//! The console is still open and visible but closing
    CON_OPENING,	//! The console is visible and opening but not yet fully open
    CON_OPEN	//! The console is open and visible
};

/* Priority levels */
#define CON_CRITICAL -2
#define CON_URGENT   -1
#define CON_NORMAL    0
#define CON_VERBOSE   1
#define CON_DBG		 2

class CConsole {
	public:
		static bool				m_bInitialized; 
		static CCvar*			m_threshold;

		int						m_Visible;			//! enum that tells which visible state we are in CON_HIDE, CON_SHOW, CON_RAISE, CON_LOWER
		int						m_RaiseOffset;			//! Offset used when scrolling in the console
		int						m_HideKey;			//! the key that can hide the console
		CArray<CCharArray>	m_ConsoleLines;		//! List of all the past lines
		CArray<CCharArray>	m_CommandLines;		//! List of all the past commands
		int						m_TotalConsoleLines;		//! Total number of lines in the console
		int						m_ConsoleScrollBack;		//! How much the user scrolled back in the console
		int						m_TotalCommands;		//! Number of commands in the Back Commands
		int						m_LineBuffer;			//! The number of visible lines in the console (autocalculated)
		int						m_VChars;			//! The number of visible characters in one console line (autocalculated)
		char*						m_Prompt;			//! Prompt displayed in command line
		char						m_Command [CON_CHARS_PER_LINE];	//! current command in command line = lcommand + rcommand
		char						m_RCommand [CON_CHARS_PER_LINE];	//! left hand CSide of cursor
		char						m_LCommand [CON_CHARS_PER_LINE];	//! right hand CSide of cursor
		char						m_VCommand [CON_CHARS_PER_LINE];	//! current visible command line
		int						m_CursorPos;			//! Current cursor position in CurrentCommand
		int						m_Offset;			//! CommandOffset (first visible char of command) - if command is too long to fit into console
		int						m_InsMode;			//! Insert or Overwrite characters?
		CCanvas*					m_surface;	//! Canvas of the console
		CScreen*					m_output;	//! This is the screen to draw the console to
		CBitmap*					m_background;	//! Background image for the console
		CBitmap*					m_input;	//! Dirty rectangle to draw over behind the users background
		int						m_DispX;
		int						m_DispY;		//! The top left x and y coords of the console on the display screen
#if 0
		ubyte ConsoleAlpha;	//! The consoles alpha level
#endif
		int m_CommandScrollBack;		//! How much the users scrolled back in the command lines
		void ( _CDECL_ *m_CmdFunction) (char* command);	//! The Function that is executed if you press <Return> in the console
		char* ( _CDECL_ *m_TabFunction) (char* command);	//! The Function that is executed if you press <Tab> in the console

	public:
		CConsole () { Init (); }
		~CConsole () { Destroy (); }
		void Init (void);
		CConsole* Create (void);
		void Destroy (void);
		/*! Takes keys from the keyboard and inputs them to the console if the console isVisible ().
			If the event was not handled (i.e. WM events or unknown ctrl- or alt-sequences) 
			the function returns the event for further processing. */
		int Events (int event);
		/*! Makes the console visible */
		void Show (void);
		/*! Hides the console */
		void Hide (void);
		/*! Returns 1 if the console is visible, 0 else */
		int IsVisible (void);
		/*! Internal: Updates visible state. Used in DrawConsole () */
		void UpdateOffset (void);
		/*! Draws the console to the screen if it isVisible ()*/
		void Draw (void);
		/*! Initializes a new console */
		void Setup (CFont *Font, CScreen *DisplayScreen, int lines, int x, int y, int w, int h);
		/*! printf for the console */
		void _CDECL_ Out (const char *str, ...);
	#if 0
		/*! Sets the alpha channel of an SDL_Surface to the specified value (0 - transparend,
			255 - opaque). Use this function also for OpenGL. */
		void Alpha (ubyte alpha);
		/*! Internal: Sets the alpha channel of an SDL_Surface to the specified value.
			Preconditions: the surface in question is RGBA. 0 <= a <= 255, where 0 is transparent and 255 opaque */
		void AlphaGL (SDL_Surface *s, int alpha);
		/*! Sets a background image for the console */
	#endif
		void LoadBackground (const char *filename);
		int SetBackground (CBitmap *image);
		/*! Sets font info for the console */
		void SetFont (CFont *font, uint fg, uint bg);
		/*! Changes current position of the console */
		void Position (int x, int y);
		/*! Changes the size of the console */
		int Resize (int x, int y, int w, int h);
		/*! Beams a console to another screen surface. Needed if you want to make a Video restart in your program. This
			function first changes the output Pointer then calls Resize to adjust the new size. */
		int Transfer (CScreen* newOutput, int x, int y, int w, int h);
		/*! Modify the prompt of the console */
		void SetPrompt (char* newprompt);
		/*! Set the key, that invokes a Hide () after press. default is ESCAPE and you can always hide using
			ESCAPE and the HideKey. compared against event->key.keysym.sym !! */
		void SetHideKey (int key);
		/*! Internal: executes the command typed in at the console (called if you press ENTER)*/
		void Execute (char* command);
		/*! Sets the callback function that is called if a command was typed in. The function could look like this:
			void my_command_handler (char* command). @param console: the console the command
			came from. @param command: the command string that was typed in. */
		void SetExecuteFunction (void ( _CDECL_ * CmdFunction) (char* command));
		/*! Sets the callback tabulator completion function. char* my_tabcompletion (char* command). If Tab is
			pressed, the function gets called with the already typed in command. my_tabcompletion then checks if if can
			complete the command or if it should display a list of all matching commands (with Out ()). Returns the 
			completed command or NULL if no completion was made. */
		void SetTabCompletion (char* ( _CDECL_ * TabFunction) (char* command));
		/*! Internal: Gets called when TAB was pressed */
		void TabCompletion (void);
		/*! Internal: makes newline (same as printf ("\n") or Out (console, "\n") ) */
		void NewLineConsole (void);
		/*! Internal: shift command history (the one you can switch with the up/down keys) */
		void NewLineCommand (void);
		/*! Internal: updates console after resize etc. */
		void Update (void);


		/*! Internal: Default Execute callback */
		static void _CDECL_ DefaultCmdFunction (char* command);
		/*! Internal: Default TabCompletion callback */
		static char* _CDECL_ DefaultTabFunction (char* command);

		/*! Internal: draws the commandline the user is typing in to the screen. called by update? */
		void DrawCommandLine ();

		/*! Internal: Gets called if you press the LEFT key (move cursor left) */
		void CursorLeft (void);
		/*! Internal: Gets called if you press the RIGHT key (move cursor right) */
		void CursorRight (void);
		/*! Internal: Gets called if you press the HOME key (move cursor to the beginning
		of the line */
		void CursorHome (void);
		/*! Internal: Gets called if you press the END key (move cursor to the end of the line*/
		void CursorEnd (void);
		/*! Internal: Called if you press DELETE (deletes character under the cursor) */
		void CursorDel (void);
		/*! Internal: Called if you press BACKSPACE (deletes character left of cursor) */
		void CursorBSpace (void);
		/*! Internal: Called if you nType in a character (add the char to the command) */
		void CursorAdd (int event);

		/*! Internal: Called if you press Ctrl-C (deletes the commandline) */
		void ClearCommand (void);
		/*! Internal: Called if you press Ctrl-L (deletes the History) */
		void ClearHistory (void);

		/*! Internal: Called if you press UP key (switches through recent typed in commands */
		void CommandUp (void);
		/*! Internal: Called if you press DOWN key (switches through recent typed in commands */
		void CommandDown (void);

		void _CDECL_ printf (int priority, const char *fmt, ...);
	};

#define CON_BG BackgroundName (BG_SCORES, screen.Width () >= 640)

extern CConsole console;

#endif //_CONSOLE_H
