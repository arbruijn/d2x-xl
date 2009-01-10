/*  console.c
 *  Written By: Garrett Banuk <mongoose@mongeese.ccOrg>
 *  Code Cleanup and heavily extended by: Clemens Wacha <reflex-2000@gmx.net>
 *  Ported to use native Descent interfaces by: Bradley Bell <btb@icculus.ccOrg>
 *
 *  This is free, just be sure to give us credit when using it
 *  in any of your programs.
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "inferno.h"
#include "strutil.h"
#include "u_mem.h"
#include "gr.h"
#include "timer.h"
#include "gamefont.h"
#include "menu.h"
#include "pcx.h"
#include "cmd.h"
#include "console.h"
#include "menubackground.h"

#define get_msecs() approx_fsec_to_msec(TimerGetApproxSeconds ())

/* Private console stuff */
#if 0
#define CON_LINE_LEN 40
static char con_display[40][40];
static int  con_line; /* Current display line */
#endif

CConsole console;

/* This contains a pointer to the "topmost" console. The console that
 * is currently taking keyboard input. */
bool CConsole::m_bInitialized = false;
CCvar* CConsole::m_threshold = NULL;

/*  Takes keys from the keyboard and inputs them to the console
    If the event was not handled (i.e. WM events or unknown ctrl-shift 
    sequences) the function returns the event for further processing. */
int CConsole::Events (int event)
{
if (!IsVisible ())
	return event;

if (event & KEY_CTRLED) {
		//CTRL pressed
	switch (event & ~KEY_CTRLED) {
		case KEY_A:
			CursorHome ();
			break;
		case KEY_E:
			CursorEnd ();
			break;
		case KEY_C:
			ClearCommand ();
			break;
		case KEY_L:
			ClearHistory ();
			Update ();
			break;
		default:
			return event;
		}
	}
else if (event & KEY_ALTED) {
		//the console does not handle ALT combinations!
		return event;
	}
else {
		//first of all, check if the console hide key was pressed
	if (event == m_HideKey) {
		Hide ();
		return 0;
		}
	switch (event & 0xff) {
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			return event;

		case KEY_HOME:
			if (event & KEY_SHIFTED) {
				m_ConsoleScrollBack = m_LineBuffer-1;
				Update ();
				}
			else {
				CursorHome ();
				}
			break;

		case KEY_END:
			if (event & KEY_SHIFTED) {
				m_ConsoleScrollBack = 0;
				Update ();
				}
			else {
				CursorEnd ();
				}
			break;

		case KEY_PAGEUP:
			m_ConsoleScrollBack += CON_LINE_SCROLL;
			if (m_ConsoleScrollBack > m_LineBuffer-1)
				m_ConsoleScrollBack = m_LineBuffer-1;

			Update ();
			break;

		case KEY_PAGEDOWN:
			m_ConsoleScrollBack -= CON_LINE_SCROLL;
			if (m_ConsoleScrollBack < 0)
				m_ConsoleScrollBack = 0;
			Update ();
			break;

		case KEY_UP:
			CommandUp ();
			break;

		case KEY_DOWN:
			CommandDown ();
			break;

		case KEY_LEFT:
			CursorLeft ();
			break;

		case KEY_RIGHT:
			CursorRight ();
			break;

		case KEY_BACKSP:
			CursorBSpace ();
			break;

		case KEY_DELETE:
			CursorDel ();
			break;

		case KEY_INSERT:
			m_InsMode = !m_InsMode;
			break;

		case KEY_TAB:
			TabCompletion ();
			break;

		case KEY_ENTER:
			if (strlen (m_Command) > 0) {
				NewLineCommand ();
				// copy the input into the past commands strings
				strcpy (m_CommandLines [0].Buffer (), m_Command);
				// display the command including the prompt
				Out ("%s%s", m_Prompt, m_Command);
				Update ();
				Execute (m_Command);
				//printf ("Command: %s\n", Command);
				ClearCommand ();
				m_CommandScrollBack = -1;
			}
			break;
		case KEY_ESC:
			//deactivate Console
			if (event & KEY_SHIFTED) {
				Hide ();
				return 0;
				}
			break;
		default:
			if (m_InsMode)
				CursorAdd (event);
			else {
				CursorAdd (event);
				CursorDel ();
				}
			}
	}
return 0;
}

#if 0
/* CConsole::AlphaGL () -- sets the alpha channel of an SDL_Surface to the
 * specified value.  Preconditions: the surface in question is RGBA.
 * 0 <= a <= 255, where 0 is transparent and 255 is opaque. */
void CConsole::AlphaGL (SDL_Surface *s, int alpha) {
	ubyte val;
	int x, y, w, h;
	uint pixel;
	ubyte r, g, b, a;
	SDL_PixelFormat *format;
	static char errorPrinted = 0;


	/* debugging assertions -- these slow you down, but hey, crashing sucks */
	if (!s) {
		PRINT_ERROR ("NULL Surface passed to CConsole::AlphaGL\n");
		return;
	}

	/* clamp alpha value to 0...255 */
	if (alpha < SDL_ALPHA_TRANSPARENT)
		val = SDL_ALPHA_TRANSPARENT;
	else if (alpha > SDL_ALPHA_OPAQUE)
		val = SDL_ALPHA_OPAQUE;
	else
		val = alpha;

	/* loop over alpha channels of each pixel, setting them appropriately. */
	w = s->w;
	h = s->h;
	format = s->format;
	switch (format->BytesPerPixel) {
	case 2:
		/* 16-bit surfaces don't seem to support alpha channels. */
		if (!errorPrinted) {
			errorPrinted = 1;
			PRINT_ERROR ("16-bit SDL surfaces do not support alpha-blending under OpenGL.\n");
		}
		break;
	case 4: {
			/* we can do this very quickly in 32-bit mode.  24-bit is more
			 * difficult.  And since 24-bit mode is reall the same as 32-bit,
			 * so it usually ends up taking this route too.  Win!  Unroll loop
			 * and use pointer arithmetic for extra speed. */
			int numpixels = h * (w << 2);
			ubyte *pix = reinterpret_cast<ubyte*> (s->pixels);
			ubyte *last = pix + numpixels;
			ubyte *pixel;
			if ((numpixels & 0x7) == 0)
				for (pixel = pix + 3; pixel < last; pixel += 32)
					*pixel = * (pixel + 4) = * (pixel + 8) = * (pixel + 12) = * (pixel + 16) = * (pixel + 20) = * (pixel + 24) = * (pixel + 28) = val;
			else
				for (pixel = pix + 3; pixel < last; pixel += 4)
					*pixel = val;
			break;
		}
	default:
		/* we have no choice but to do this slowly.  <sigh> */
		for (y = 0; y < h; ++y)
			for (x = 0; x < w; ++x) {
				char print = 0;
				/* Lock the surface for direct access to the pixels */
				if (SDL_MUSTLOCK (s) && SDL_LockSurface (s) < 0) {
#ifndef WIN32
					PRINT_ERROR ("Can't lock surface: ");
					fprintf (stderr, "%s\n", SDL_GetError ();
#endif
					return;
				}
				pixel = DT_GetPixel (s, x, y);
				if (x == 0 && y == 0)
					print = 1;
				SDL_GetRGBA (pixel, format, &r, &g, &b, &a);
				pixel = SDL_MapRGBA (format, r, g, b, val);
				SDL_GetRGBA (pixel, format, &r, &g, &b, &a);
				DT_PutPixel (s, x, y, pixel);

				/* unlock surface again */
				if (SDL_MUSTLOCK (s))
					SDL_UnlockSurface (s);
			}
		break;
	}
}
#endif

//------------------------------------------------------------------------------

/* Updates the console buffer */
void CConsole::Update (void) 
{
	int				loop;
	int				loop2;
	int				Screenlines;
	tCanvasColor	orig_color;

	/* Due to the Blits, the update is not very fast: So only update if it's worth it */
if (!IsVisible ())
	return;
Screenlines = m_surface->Height () / (CON_LINE_SPACE + m_surface->Font ()->Height ());
if (!gameOpts->menus.nStyle) {
	CCanvas::Push ();
	CCanvas::SetCurrent (m_surface);
#if 0
	SDL_FillRect (m_surface, NULL, SDL_MapRGBA (m_surface->format, 0, 0, 0, m_ConsoleAlpha);
	if (m_output->flags & SDL_OPENGLBLIT)
		SDL_SetAlpha (m_surface, 0, SDL_ALPHA_OPAQUE);
#endif
/* draw the background image if there is one */
	if (m_background)
		m_background->Stretch ();
	}
/* Draw the text from the back buffers, calculate in the scrollback from the user
 * this is a Normal SDL software-mode blit, so we need to temporarily set the ColorKey
 * for the font, and then clear it when we're done.
 */
#if 0
if ((m_output->flags & SDL_OPENGLBLIT) && (m_output->format->BytesPerPixel > 2)) {
	uint *pix = reinterpret_cast<uint*> (CurrentFont->FontSurface->pixels);
	SDL_SetColorKey (CurrentFont->FontSurface, SDL_SRCCOLORKEY, *pix);
}
#endif

orig_color = CCanvas::Current ()->FontColor (0);
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
//now draw text from last but second line to top
for (loop = 0; loop < Screenlines-1 && loop < m_LineBuffer - m_ConsoleScrollBack; loop++) {
	if (m_ConsoleScrollBack != 0 && loop == 0)
		for (loop2 = 0; loop2 < (m_VChars / 5) + 1; loop2++) {
			GrString (CON_CHAR_BORDER + (loop2*5*m_surface->Font ()->Width ()), 
						 (Screenlines - loop - 2) * (CON_LINE_SPACE + m_surface->Font ()->Height ()), 
						 CON_SCROLL_INDICATOR, NULL);
			}
	else {
		GrString (CON_CHAR_BORDER, 
					 (Screenlines - loop - 2) * (CON_LINE_SPACE + m_surface->Font ()->Height ()),
					 m_ConsoleLines [m_ConsoleScrollBack + loop].Buffer (), NULL);
		}
	}
CCanvas::Current ()->FontColor (0) = orig_color;
if (!gameOpts->menus.nStyle)
	CCanvas::Pop ();

#if 0
if (m_output->flags & SDL_OPENGLBLIT)
	SDL_SetColorKey (CurrentFont->FontSurface, 0, 0);
#endif
}

//------------------------------------------------------------------------------

void CConsole::UpdateOffset (void) 
{
switch (m_Visible) {
	case CON_CLOSING:
		m_RaiseOffset -= CON_OPENCLOSE_SPEED;
		if (m_RaiseOffset <= 0) {
			m_RaiseOffset = 0;
			m_Visible = CON_CLOSED;
		}
		break;

	case CON_OPENING:
		m_RaiseOffset += CON_OPENCLOSE_SPEED;
		if (m_RaiseOffset >= m_surface->Height ()) {
			m_RaiseOffset = m_surface->Height ();
			m_Visible = CON_OPEN;
		}
		break;

	case CON_OPEN:
	case CON_CLOSED:
		break;
	}
}

//------------------------------------------------------------------------------
/* Draws the console buffer to the screen if the console is "visible" */

void CConsole::Draw (void) 
{
	CBitmap *clip;

	/* only draw if console is visible: here this means, that the console is not CON_CLOSED */
if (m_Visible == CON_CLOSED)
	return;

/* Update the scrolling offset */
UpdateOffset ();
/* Update the command line since it has a blinking cursor */
DrawCommandLine ();

#if 0
	/* before drawing, make sure the alpha channel of the console surface is set
	 * properly.  (sigh) I wish we didn't have to do this every frame... */
	if (m_output->flags & SDL_OPENGLBLIT)
		CConsole::AlphaGL (m_surface, m_ConsoleAlpha);
#endif

CCanvas::Push ();
CCanvas::SetCurrent (m_output->Canvas ());
if (gameOpts->menus.nStyle)
	backgroundManager.DrawBox (0, 0, m_surface->Width (), m_RaiseOffset, 1, 1.0f, 0);
else {
	clip = m_surface->CreateChild (
		0, m_surface->Height () - m_RaiseOffset, 
		m_surface->Width (), m_RaiseOffset);
	GrBitmap (m_DispX, m_DispY, clip);
	clip->Destroy ();
#if 0
	if (m_output->flags & SDL_OPENGLBLIT)
		SDL_UpdateRects (m_output, 1, &DestRect);
#endif
	}
CCanvas::Pop ();
Update ();
}

//------------------------------------------------------------------------------

CConsole* CConsole::Create (void)
{
return new CConsole;
}

//------------------------------------------------------------------------------

void CConsole::Init (void)
{
m_Visible = 0;			
m_RaiseOffset = 0;		
m_HideKey = 0;			
m_TotalConsoleLines = 0;
m_ConsoleScrollBack = 0;
m_TotalCommands = 0;	
m_LineBuffer = 0;		
m_VChars = 0;			
m_CursorPos = 0;
m_Offset = 0;	
m_InsMode = 0;	
m_DispX = 0;
m_DispY = 0;	
m_CommandScrollBack;		
m_Prompt= NULL;			
m_surface = NULL;	
m_output= NULL;	
m_background = NULL;	
m_input= NULL;	
m_CmdFunction = NULL;	
m_TabFunction = NULL;	
memset (m_Command, 0, sizeof (m_Command));	
memset (m_RCommand, 0, sizeof (m_RCommand));	
memset (m_LCommand, 0, sizeof (m_LCommand));	
memset (m_VCommand, 0, sizeof (m_VCommand));	
m_threshold = CCvar::Register ("con_threshold", "0");
}

//------------------------------------------------------------------------------

/* Initializes the console */
void CConsole::Setup (CFont *font, CScreen *output, int lines, int x, int y, int w, int h)
{
	int loop;

/* Create a new console struct and init it. */
m_Visible = CON_CLOSED;
m_RaiseOffset = 0;
m_ConsoleLines = NULL;
m_CommandLines = NULL;
m_TotalConsoleLines = 0;
m_ConsoleScrollBack = 0;
m_TotalCommands = 0;
m_background = NULL;
#if 0
m_ConsoleAlpha = SDL_ALPHA_OPAQUE;
#endif
m_Offset = 0;
m_InsMode = 1;
m_CursorPos = 0;
m_CommandScrollBack = 0;
m_output = output;
m_Prompt = CON_DEFAULT_PROMPT;
m_HideKey = CON_DEFAULT_HIDEKEY;

SetExecuteFunction (&CConsole::DefaultCmdFunction);
SetTabCompletion (&CConsole::DefaultTabFunction);

/* make sure that the size of the console is valid */
if (w > m_output->Width () || w < font->Width () * 32)
	w = m_output->Width ();
if (h > m_output->Height () || h < font->Height ())
	h = m_output->Height ();
if (x < 0 || x > m_output->Width () - w)
	m_DispX = 0;
else
	m_DispX = x;
if (y < 0 || y > m_output->Height () - h)
	m_DispY = 0;
else
	m_DispY = y;

/* load the console surface */
m_surface = CCanvas::Create (w, h);
/* Load the consoles font */
CCanvas::Push ();
CCanvas::SetCurrent (m_surface);
fontManager.SetCurrent (font);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
CCanvas::Pop ();

/* Load the dirty rectangle for user input */
m_input = CBitmap::Create (0, w, m_surface->Font ()->Height (), 1);
#if 0
SDL_FillRect (m_input, NULL, SDL_MapRGBA (m_surface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
#endif

/* calculate the number of visible characters in the command line */
m_VChars = (w - CON_CHAR_BORDER) / m_surface->Font ()->Width ();
if (m_VChars > CON_CHARS_PER_LINE)
	m_VChars = CON_CHARS_PER_LINE;

/* We would like to have a minumum # of lines to guarentee we don't create a memory error */
if (h / (CON_LINE_SPACE + m_surface->Font ()->Height ()) > lines)
	m_LineBuffer = h / (CON_LINE_SPACE + m_surface->Font ()->Height ());
else
	m_LineBuffer = lines;

m_ConsoleLines.Create (m_LineBuffer);
m_CommandLines.Create (m_LineBuffer);
for (loop = 0; loop < m_LineBuffer; loop++) {
	m_ConsoleLines [loop].Create (CON_CHARS_PER_LINE);
	m_CommandLines [loop].Create (CON_CHARS_PER_LINE);
	}
memset (m_Command, 0, CON_CHARS_PER_LINE);
memset (m_LCommand, 0, CON_CHARS_PER_LINE);
memset (m_RCommand, 0, CON_CHARS_PER_LINE);
memset (m_VCommand, 0, CON_CHARS_PER_LINE);
LoadBackground (CON_BG);

Out ("Console initialised.");
NewLineConsole ();
//CConsole::ListCommands (newinfo);
}

//------------------------------------------------------------------------------
/* Makes the console visible */
void CConsole::Show (void) 
{
m_Visible = CON_OPENING;
Update ();
}

//------------------------------------------------------------------------------
/* Hides the console (make it invisible) */
void CConsole::Hide (void) 
{
m_Visible = CON_CLOSING;
}

//------------------------------------------------------------------------------
/* tells wether the console is visible or not */
int CConsole::IsVisible (void) 
{
return (m_Visible == CON_OPEN) || (m_Visible == CON_OPENING);
}

//------------------------------------------------------------------------------
/* Frees all the memory loaded by the console */
/* Frees all the memory loaded by the console */
void CConsole::Destroy (void) 
{
	int i;

	//CConsole::DestroyCommands ();
for (i = 0; i <= m_LineBuffer - 1; i++) {
	m_ConsoleLines [i].Destroy ();
	m_CommandLines [i].Destroy ();
	}
m_ConsoleLines.Destroy ();
m_CommandLines.Destroy ();
m_surface->Destroy ();
m_surface = NULL;
if (m_background)
	delete m_background;
m_background = NULL;
if (m_input)
	delete m_input;
m_input = NULL;
}

//------------------------------------------------------------------------------

/* Increments the console lines */
void CConsole::NewLineConsole (void) 
{
	int	loop;
	char* temp;

temp = m_ConsoleLines [m_LineBuffer - 1].Buffer ();
for (loop = m_LineBuffer - 1; loop > 0; loop--)
	m_ConsoleLines [loop].SetBuffer (m_ConsoleLines [loop - 1].Buffer (), false, CON_CHARS_PER_LINE);
m_ConsoleLines [0].SetBuffer (temp, false, CON_CHARS_PER_LINE);
m_ConsoleLines [0].Clear ();
if (m_TotalConsoleLines < m_LineBuffer - 1)
	m_TotalConsoleLines++;
//Now adjust the ConsoleScrollBack
//dont scroll if not at bottom
if (m_ConsoleScrollBack != 0)
	m_ConsoleScrollBack++;
//boundaries
if (m_ConsoleScrollBack > m_LineBuffer-1)
	m_ConsoleScrollBack = m_LineBuffer-1;
}

//------------------------------------------------------------------------------

/* Increments the command lines */
void CConsole::NewLineCommand (void) 
{
	int	loop;
	char*	temp;

temp = m_CommandLines [m_LineBuffer - 1].Buffer ();
for (loop = m_LineBuffer - 1; loop > 0; loop--)
	m_CommandLines [loop].SetBuffer (m_CommandLines [loop - 1].Buffer (), false, CON_CHARS_PER_LINE);
m_CommandLines [0].SetBuffer (temp, false, CON_CHARS_PER_LINE);
m_CommandLines [0].Clear ();
if (m_TotalCommands < m_LineBuffer - 1)
	m_TotalCommands++;
}

//------------------------------------------------------------------------------
/* Draws the command line the user is typing in to the screen */
/* completely rewritten by C.Wacha */
void CConsole::DrawCommandLine (void) 
{
	int x;
	int commandbuffer;
#if 0
	CFont* CurrentFont;
#endif
	static int		LastBlinkTime = 0;	/* Last time the consoles cursor blinked */
	static int		LastCursorPos = 0;		// Last Cursor Position
	static int		bBlink = 0;			/* Is the cursor currently blinking */
	tCanvasColor	orig_color;

commandbuffer = m_VChars - (int) strlen (m_Prompt) - 1; // -1 to make cursor visible

#if 0
CurrentFont = m_surface->Font ();
#endif

//Concatenate the left and right CSide to command
strcpy (m_Command, m_LCommand);
strncat (m_Command, m_RCommand, strlen (m_RCommand));

//calculate display offset from current cursor position
if (m_Offset < m_CursorPos - commandbuffer)
	m_Offset = m_CursorPos - commandbuffer;
if (m_Offset > m_CursorPos)
	m_Offset = m_CursorPos;

//first add prompt to visible part
strcpy (m_VCommand, m_Prompt);
//then add the visible part of the command
strncat (m_VCommand, &m_Command [m_Offset], strlen (&m_Command [m_Offset]));
//now display the result

#if 0
//once again we're drawing text, so in OpenGL context we need to temporarily set up
//software-mode transparency.
if (m_output->flags & SDL_OPENGLBLIT) {
	uint *pix = reinterpret_cast<uint*> (CurrentFont->FontSurface->pixels);
	SDL_SetColorKey (CurrentFont->FontSurface, SDL_SRCCOLORKEY, *pix);
}
#endif

//first of all restore input
if (!gameOpts->menus.nStyle) {
	CCanvas::Push ();
	CCanvas::SetCurrent (m_surface);
	if (m_background)
		m_background->Render (CCanvas::Current (), 
									 m_surface->Width (), m_surface->Font ()->Height (),
									 0, m_surface->Height () - m_surface->Font ()->Height (),
									 m_surface->Width (), m_surface->Font ()->Height (),
									 0, m_surface->Height () - m_surface->Font ()->Height ());
	else
		GrBitmap (0, m_surface->Height () - m_surface->Font ()->Height (), m_input);
	}
//now add the text
orig_color = CCanvas::Current ()->FontColor (0);
fontManager.SetCurrent (SMALL_FONT);
fontManager.SetColorRGBi (WHITE_RGBA, 1, 0, 0);
GrString (CON_CHAR_BORDER, m_surface->Height () - m_surface->Font ()->Height (), m_VCommand, NULL);

//at last add the cursor
//check if the blink period is over
if (get_msecs () > LastBlinkTime) {
	LastBlinkTime = get_msecs () + CON_BLINK_RATE;
	if (bBlink)
		bBlink = 0;
	else
		bBlink = 1;
	}

//check if cursor has moved - if yes display cursor anyway
if (m_CursorPos != LastCursorPos) {
	LastCursorPos = m_CursorPos;
	LastBlinkTime = get_msecs () + CON_BLINK_RATE;
	bBlink = 1;
	}

if (bBlink) {
#if 1
	int w, h, aw;
	fontManager.Current ()->StringSize (m_VCommand, w, h, aw);
	x = CON_CHAR_BORDER + w;
#else
	x = CON_CHAR_BORDER + m_surface->Font ()->Width () * (m_CursorPos - m_Offset + (int) strlen (m_Prompt));
#endif
	if (m_InsMode)
		GrString (x, m_surface->Height () - m_surface->Font ()->Height (), CON_INS_CURSOR, NULL);
	else
		GrString (x, m_surface->Height () - m_surface->Font ()->Height (), CON_OVR_CURSOR, NULL);
	}
CCanvas::Current ()->FontColor (0) = orig_color;
if (!gameOpts->menus.nStyle)
	CCanvas::Pop ();

#if 0
if (m_output->flags & SDL_OPENGLBLIT) {
	SDL_SetColorKey (CurrentFont->FontSurface, 0, 0);
}
#endif
}

#ifdef _WIN32
# define vsnprintf _vsnprintf
#endif

//------------------------------------------------------------------------------
/* Outputs text to the console (in game), up to CON_CHARS_PER_LINE chars can be entered */
void _CDECL_ CConsole::Out (const char *str, ...) 
{
	va_list marker;
	//keep some space free for stuff like CConsole::Out ("blablabla %s", m_Command);
	char temp [CON_CHARS_PER_LINE + 128];
	char* ptemp;

va_start (marker, str);
vsnprintf (temp, CON_CHARS_PER_LINE + 127, str, marker);
va_end (marker);

ptemp = temp;
//temp now contains the complete string we want to output
// the only problem is that temp is maybe longer than the console
// width so we have to cut it into several pieces

if (m_ConsoleLines.Buffer ()) {
	while (strlen (ptemp) > (size_t) m_VChars) {
		NewLineConsole ();
		strncpy (m_ConsoleLines [0].Buffer (), ptemp, m_VChars);
		m_ConsoleLines [0][m_VChars] = '\0';
		ptemp = &ptemp [m_VChars];
		}
	NewLineConsole ();
	strncpy (m_ConsoleLines [0].Buffer (), ptemp, m_VChars);
	m_ConsoleLines [0][m_VChars] = '\0';
	Update ();
	}
/* And print to stdout */
//printf ("%s\n", temp);
}


//------------------------------------------------------------------------------
#if 0
/* Sets the alpha level of the 0 turns off alpha blending */
void CConsole::Alpha (ubyte alpha) {
	if (!console)
		return;

	/* store alpha as state! */
	m_ConsoleAlpha = alpha;

	if ((m_output->flags & SDL_OPENGLBLIT) == 0) {
		if (alpha == 0)
			SDL_SetAlpha (m_surface, 0, alpha);
		else
			SDL_SetAlpha (m_surface, SDL_SRCALPHA, alpha);
	}

	//	CConsole::Update ();
}
#endif

//------------------------------------------------------------------------------

void CConsole::LoadBackground (const char *filename)
{
	CBitmap bm;

int pcxError = PCXReadBitmap (filename, &bm, BM_LINEAR, 0);
Assert(pcxError == PCX_ERROR_NONE);
bm.Remap (NULL, -1, -1);
SetBackground (&bm);
}

//------------------------------------------------------------------------------
/* Adds  background image to the scaled to size of console*/
int CConsole::SetBackground (CBitmap *image)
{
/* Free the background from the console */
if (image == NULL) {
	if (m_background) {
		delete m_background;
		m_background = NULL;
		}
#if 0
SDL_FillRect (m_input, NULL, SDL_MapRGBA (m_surface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
#endif
	return 0;
	}

/* Load a new background */
if (m_background)
	delete m_background;
m_background = CBitmap::Create (0, m_surface->Width (), m_surface->Height (), 1);
GrBitmapScaleTo (image, m_background);
return 0;
}

//------------------------------------------------------------------------------
/* Sets font info for the console */
void CConsole::SetFont (CFont *font, uint fg, uint bg)
{
CCanvas::Push ();
CCanvas::SetCurrent (m_surface);
fontManager.SetCurrent (font);
fontManager.SetColorRGBi (fg, 1, bg, bg != 0);
CCanvas::Pop ();
}

//------------------------------------------------------------------------------
/* takes a new x and y of the top left of the console window */
void CConsole::Position (int x, int y) 
{
if (x < 0 || x > m_output->Width () - m_surface->Width ())
	m_DispX = 0;
else
	m_DispX = x;
if (y < 0 || y > m_output->Height () - m_surface->Height ())
	m_DispY = 0;
else
	m_DispY = y;
}

//------------------------------------------------------------------------------

/* resizes the console, has to reset a lot of stuff
 * returns 1 on error */
int CConsole::Resize (int x, int y, int w, int h)
{
/* make sure that the size of the console is valid */
if (w > m_output->Width () || w < m_surface->Font ()->Width () * 32)
	w = m_output->Width ();
if (h > m_output->Height () || h < m_surface->Font ()->Height ())
	h = m_output->Height ();
if (x < 0 || x > m_output->Width () - w)
	m_DispX = 0;
else
	m_DispX = x;
if (y < 0 || y > m_output->Height () - h)
	m_DispY = 0;
else
	m_DispY = y;

/* resize console surface */
CFont* font = m_surface->Font ();
m_surface->Destroy ();
m_surface = CCanvas::Create (w, h);
m_surface->SetFont (font);
delete m_input;
m_input = CBitmap::Create (0, w, m_surface->Font ()->Height (), 1);
m_ConsoleScrollBack = 0;
return 0;
}

//------------------------------------------------------------------------------
/* Transfers the console to another screen surface, and adjusts size */
int CConsole::Transfer (CScreen *newOutput, int x, int y, int w, int h)
{
m_output = newOutput;
return Resize (x, y, w, h);
}

//------------------------------------------------------------------------------
/* Sets the Prompt for console */
void CConsole::SetPrompt (char* newprompt) 
{
//check length so we can still see at least 1 char :-)
if (strlen (newprompt) < (size_t) m_VChars)
	m_Prompt = StrDup (newprompt);
else
	Out ("prompt too long. (max. %i chars)", m_VChars - 1);
}

//------------------------------------------------------------------------------
/* Sets the key that deactivates (hides) the console. */
void CConsole::SetHideKey (int key) 
{
m_HideKey = key;
}

//------------------------------------------------------------------------------
/* Executes the command entered */
void CConsole::Execute (char* command) 
{
m_CmdFunction (command);
}

//------------------------------------------------------------------------------

void CConsole::SetExecuteFunction (void ( _CDECL_ *CmdFunction) (char* command)) 
{
m_CmdFunction = CmdFunction;
}

//------------------------------------------------------------------------------

void CConsole::DefaultCmdFunction (char* command) 
{
cmd_parse (command);
}

//------------------------------------------------------------------------------

void CConsole::SetTabCompletion (char* (*TabFunction) (char* command)) 
{
m_TabFunction = TabFunction;
}

//------------------------------------------------------------------------------

void CConsole::TabCompletion (void) {
	int i,j;
	char* command;

command = StrDup (m_LCommand);
command = m_TabFunction (command);

if (!command)
	return;	//no tab completion took place so return silently

j = (int) strlen (command);
if (j > CON_CHARS_PER_LINE - 2)
	j = CON_CHARS_PER_LINE-1;
memset (m_LCommand, 0, CON_CHARS_PER_LINE);
m_CursorPos = 0;
for (i = 0; i < j; i++) {
	m_CursorPos++;
	m_LCommand [i] = command [i];
	}
//add a trailing space
m_CursorPos++;
m_LCommand [j] = ' ';
m_LCommand [j+1] = '\0';
}

//------------------------------------------------------------------------------

char* CConsole::DefaultTabFunction (char* command) 
{
return NULL;
}

//------------------------------------------------------------------------------

void CConsole::CursorLeft (void) 
{
	char temp [CON_CHARS_PER_LINE];

if (m_CursorPos > 0) {
	m_CursorPos--;
	strcpy (temp, m_RCommand);
	strcpy (m_RCommand, &m_LCommand [strlen (m_LCommand)-1]);
	strcat (m_RCommand, temp);
	m_LCommand [strlen (m_LCommand)-1] = '\0';
	}
}

//------------------------------------------------------------------------------

void CConsole::CursorRight (void) 
{
	char temp [CON_CHARS_PER_LINE];

if (m_CursorPos < (int) strlen (m_Command)) {
	m_CursorPos++;
	strncat (m_LCommand, m_RCommand, 1);
	strcpy (temp, m_RCommand);
	strcpy (m_RCommand, &temp [1]);
	}
}

//------------------------------------------------------------------------------

void CConsole::CursorHome (void) 
{
	char temp [CON_CHARS_PER_LINE];

m_CursorPos = 0;
strcpy (temp, m_RCommand);
strcpy (m_RCommand, m_LCommand);
strncat (m_RCommand, temp, strlen (temp));
memset (m_LCommand, 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void CConsole::CursorEnd (void) 
{
m_CursorPos = (int) strlen (m_Command);
strncat (m_LCommand, m_RCommand, strlen (m_RCommand));
memset (m_RCommand, 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void CConsole::CursorDel (void) 
{
	char temp [CON_CHARS_PER_LINE];

if (strlen (m_RCommand) > 0) {
	strcpy (temp, m_RCommand);
	strcpy (m_RCommand, &temp [1]);
	}
}

//------------------------------------------------------------------------------

void CConsole::CursorBSpace (void) 
{
if (m_CursorPos > 0) {
	m_CursorPos--;
	m_Offset--;
	if (m_Offset < 0)
		m_Offset = 0;
	m_LCommand [strlen (m_LCommand)-1] = '\0';
	}
}

//------------------------------------------------------------------------------

void CConsole::CursorAdd (int event)
{
if (strlen (m_Command) < CON_CHARS_PER_LINE - 1) {
	m_CursorPos++;
	m_LCommand [strlen (m_LCommand)] = KeyToASCII (event);
	m_LCommand [strlen (m_LCommand)] = '\0';
	}
}

//------------------------------------------------------------------------------

void CConsole::ClearCommand (void) 
{
m_CursorPos = 0;
memset (m_VCommand, 0, CON_CHARS_PER_LINE);
memset (m_Command, 0, CON_CHARS_PER_LINE);
memset (m_LCommand, 0, CON_CHARS_PER_LINE);
memset (m_RCommand, 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void CConsole::ClearHistory (void) 
{
for (int loop = 0; loop <= m_LineBuffer - 1; loop++)
	m_ConsoleLines [loop].Clear ();
}

//------------------------------------------------------------------------------

void CConsole::CommandUp (void) 
{
if (m_CommandScrollBack < m_TotalCommands - 1) {
	/* move back a line in the command strings and copy the command to the current input string */
	m_CommandScrollBack++;
	memset (m_RCommand, 0, CON_CHARS_PER_LINE);
	m_Offset = 0;
	strcpy (m_LCommand, m_CommandLines [m_CommandScrollBack].Buffer ());
	m_CursorPos = (int) strlen (m_CommandLines [m_CommandScrollBack].Buffer ());
	Update ();
	}
}

//------------------------------------------------------------------------------

void CConsole::CommandDown (void) 
{
if (m_CommandScrollBack > -1) {
	/* move forward a line in the command strings and copy the command to the current input string */
	m_CommandScrollBack--;
	memset (m_RCommand, 0, CON_CHARS_PER_LINE);
	memset (m_LCommand, 0, CON_CHARS_PER_LINE);
	m_Offset = 0;
	if (m_CommandScrollBack > -1)
		strcpy (m_LCommand, m_CommandLines [m_CommandScrollBack].Buffer ());
	m_CursorPos = (int) strlen (m_LCommand);
	Update ();
	}
}

//------------------------------------------------------------------------------

void _CDECL_ CConsole::printf (int priority, const char *fmt, ...)
{
	va_list arglist;
  
	static char buffer[65536];

if (!(fmt && *fmt)) 
	return;
if (priority <= ((int) m_threshold->Value ())) {
	va_start (arglist, fmt);
	vsprintf (buffer,  fmt, arglist);
	va_end (arglist);
	if (fErr) {
	   fprintf(fErr, buffer);
	   fflush(fErr);
		}
#ifdef CONSOLE
	Out (buffer);
#endif
	/* Produce a sanitised version and send it to the console */
		char *p1, *p2;
	
	p1 = p2 = buffer;
	do {
		switch (*p1) {
			case CC_COLOR:
			case CC_LSPACING:
				p1++;
			case CC_UNDERLINE:
				p1++;
			break;
			default:
				*p2++ = *p1++;
			} 
		} while (*p1);
	*p2 = 0;
	if (priority == CON_NORMAL) 
		::printf (buffer);
	}
}

//------------------------------------------------------------------------------
//eof
