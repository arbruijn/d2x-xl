/*  CON_console.c
 *  Written By: Garrett Banuk <mongoose@mongeese.ccOrg>
 *  Code Cleanup and heavily extended by: Clemens Wacha <reflex-2000@gmx.net>
 *  Ported to use native Descent interfaces by: Bradley Bell <btb@icculus.ccOrg>
 *
 *  This is D2_FREE, just be sure to give us credit when using it
 *  in any of your programs.
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "CON_console.h"

#include "u_mem.h"
#include "gr.h"
#include "timer.h"
#include "inferno.h"
#include "gamefont.h"
#include "newmenu.h"

#define FG_COLOR    grdCurCanv->cvFontFgColor
#define get_msecs() approx_fsec_to_msec(TimerGetApproxSeconds())


/* This contains a pointer to the "topmost" console. The console that
 * is currently taking keyboard input. */
static ConsoleInformation *Topmost;

/*  Takes keys from the keyboard and inputs them to the console
    If the event was not handled (i.e. WM events or unknown ctrl-shift 
    sequences) the function returns the event for further processing. */
int CON_Events(int event)
{
	if(Topmost == NULL)
		return event;
	if(!CON_isVisible(Topmost))
		return event;

	if(event & KEY_CTRLED)
	{
		//CTRL pressed
		switch(event & ~KEY_CTRLED)
		{
		case KEY_A:
			Cursor_Home(Topmost);
			break;
		case KEY_E:
			Cursor_End(Topmost);
			break;
		case KEY_C:
			Clear_Command(Topmost);
			break;
		case KEY_L:
			Clear_History(Topmost);
			CON_UpdateConsole(Topmost);
			break;
		default:
			return event;
		}
	}
	else if(event & KEY_ALTED)
	{
		//the console does not handle ALT combinations!
		return event;
	}
	else
	{
		//first of all, check if the console hide key was pressed
		if(event == Topmost->HideKey)
		{
			CON_Hide(Topmost);
			return 0;
		}
		switch (event & 0xff)
		{
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			return event;
		case KEY_HOME:
			if(event & KEY_SHIFTED)
			{
				Topmost->ConsoleScrollBack = Topmost->LineBuffer-1;
				CON_UpdateConsole(Topmost);
			} else {
				Cursor_Home(Topmost);
			}
			break;
		case KEY_END:
			if(event & KEY_SHIFTED)
			{
				Topmost->ConsoleScrollBack = 0;
				CON_UpdateConsole(Topmost);
			} else {
				Cursor_End(Topmost);
			}
			break;
		case KEY_PAGEUP:
			Topmost->ConsoleScrollBack += CON_LINE_SCROLL;
			if(Topmost->ConsoleScrollBack > Topmost->LineBuffer-1)
				Topmost->ConsoleScrollBack = Topmost->LineBuffer-1;

			CON_UpdateConsole(Topmost);
			break;
		case KEY_PAGEDOWN:
			Topmost->ConsoleScrollBack -= CON_LINE_SCROLL;
			if(Topmost->ConsoleScrollBack < 0)
				Topmost->ConsoleScrollBack = 0;
			CON_UpdateConsole(Topmost);
			break;
		case KEY_UP:
			Command_Up(Topmost);
			break;
		case KEY_DOWN:
			Command_Down(Topmost);
			break;
		case KEY_LEFT:
			Cursor_Left(Topmost);
			break;
		case KEY_RIGHT:
			Cursor_Right(Topmost);
			break;
		case KEY_BACKSP:
			Cursor_BSpace(Topmost);
			break;
		case KEY_DELETE:
			Cursor_Del(Topmost);
			break;
		case KEY_INSERT:
			Topmost->InsMode = 1-Topmost->InsMode;
			break;
		case KEY_TAB:
			CON_TabCompletion(Topmost);
			break;
		case KEY_ENTER:
			if(strlen(Topmost->Command) > 0) {
				CON_NewLineCommand(Topmost);

				// copy the input into the past commands strings
				strcpy(Topmost->CommandLines[0], Topmost->Command);

				// display the command including the prompt
				CON_Out(Topmost, "%s%s", Topmost->Prompt, Topmost->Command);
				CON_UpdateConsole(Topmost);

				CON_Execute(Topmost, Topmost->Command);
				//printf("Command: %s\n", Topmost->Command);

				Clear_Command(Topmost);
				Topmost->CommandScrollBack = -1;
			}
			break;
		case KEY_ESC:
			//deactivate Console
			if (event & KEY_SHIFTED) {
				CON_Hide(Topmost);
				return 0;
				}
			break;
		default:
			if(Topmost->InsMode)
				Cursor_Add(Topmost, event);
			else {
				Cursor_Add(Topmost, event);
				Cursor_Del(Topmost);
			}
		}
	}
	return 0;
}

#if 0
/* CON_AlphaGL() -- sets the alpha channel of an SDL_Surface to the
 * specified value.  Preconditions: the surface in question is RGBA.
 * 0 <= a <= 255, where 0 is transparent and 255 is opaque. */
void CON_AlphaGL(SDL_Surface *s, int alpha) {
	Uint8 val;
	int x, y, w, h;
	Uint32 pixel;
	Uint8 r, g, b, a;
	SDL_PixelFormat *format;
	static char errorPrinted = 0;


	/* debugging assertions -- these slow you down, but hey, crashing sucks */
	if(!s) {
		PRINT_ERROR("NULL Surface passed to CON_AlphaGL\n");
		return;
	}

	/* clamp alpha value to 0...255 */
	if(alpha < SDL_ALPHA_TRANSPARENT)
		val = SDL_ALPHA_TRANSPARENT;
	else if(alpha > SDL_ALPHA_OPAQUE)
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
		if(!errorPrinted) {
			errorPrinted = 1;
			PRINT_ERROR("16-bit SDL surfaces do not support alpha-blending under OpenGL.\n");
		}
		break;
	case 4: {
			/* we can do this very quickly in 32-bit mode.  24-bit is more
			 * difficult.  And since 24-bit mode is reall the same as 32-bit,
			 * so it usually ends up taking this route too.  Win!  Unroll loop
			 * and use pointer arithmetic for extra speed. */
			int numpixels = h * (w << 2);
			Uint8 *pix = (Uint8 *) (s->pixels);
			Uint8 *last = pix + numpixels;
			Uint8 *pixel;
			if((numpixels & 0x7) == 0)
				for(pixel = pix + 3; pixel < last; pixel += 32)
					*pixel = *(pixel + 4) = *(pixel + 8) = *(pixel + 12) = *(pixel + 16) = *(pixel + 20) = *(pixel + 24) = *(pixel + 28) = val;
			else
				for(pixel = pix + 3; pixel < last; pixel += 4)
					*pixel = val;
			break;
		}
	default:
		/* we have no choice but to do this slowly.  <sigh> */
		for(y = 0; y < h; ++y)
			for(x = 0; x < w; ++x) {
				char print = 0;
				/* Lock the surface for direct access to the pixels */
				if(SDL_MUSTLOCK(s) && SDL_LockSurface(s) < 0) {
#ifndef WIN32
					PRINT_ERROR("Can't lock surface: ");
					fprintf(stderr, "%s\n", SDL_GetError();
#endif
					return;
				}
				pixel = DT_GetPixel(s, x, y);
				if(x == 0 && y == 0)
					print = 1;
				SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
				pixel = SDL_MapRGBA(format, r, g, b, val);
				SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
				DT_PutPixel(s, x, y, pixel);

				/* unlock surface again */
				if(SDL_MUSTLOCK(s))
					SDL_UnlockSurface(s);
			}
		break;
	}
}
#endif

//------------------------------------------------------------------------------

/* Updates the console buffer */
void CON_UpdateConsole(ConsoleInformation *console) {
	int loop;
	int loop2;
	int Screenlines;
	gsrCanvas *canv_save = NULL;
	grsColor	orig_color;
	grsFont		*orig_font;

	if(!console)
		return;

	/* Due to the Blits, the update is not very fast: So only update if it's worth it */
	if(!CON_isVisible(console))
		return;

	Screenlines = console->ConsoleSurface->cv_h / (CON_LINE_SPACE + console->ConsoleSurface->cvFont->ftHeight);

	if(!gameOpts->menus.nStyle) {
		canv_save = grdCurCanv;
		GrSetCurrentCanvas(console->ConsoleSurface);
#if 0
		SDL_FillRect(console->ConsoleSurface, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, console->ConsoleAlpha);
		if(console->OutputScreen->flags & SDL_OPENGLBLIT)
			SDL_SetAlpha(console->ConsoleSurface, 0, SDL_ALPHA_OPAQUE);
#endif
	/* draw the background image if there is one */
		if(console->BackgroundImage)
			GrBitmap(0, 0, console->BackgroundImage);
		}
	/* Draw the text from the back buffers, calculate in the scrollback from the user
	 * this is a normal SDL software-mode blit, so we need to temporarily set the ColorKey
	 * for the font, and then clear it when we're done.
	 */
#if 0
	if((console->OutputScreen->flags & SDL_OPENGLBLIT) && (console->OutputScreen->format->BytesPerPixel > 2)) {
		Uint32 *pix = (Uint32 *) (CurrentFont->FontSurface->pixels);
		SDL_SetColorKey(CurrentFont->FontSurface, SDL_SRCCOLORKEY, *pix);
	}
#endif

	orig_color = FG_COLOR;
	orig_font = grdCurCanv->cvFont;
	grdCurCanv->cvFont = SMALL_FONT;
	GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
	//now draw text from last but second line to top
	for(loop = 0; loop < Screenlines-1 && loop < console->LineBuffer - console->ConsoleScrollBack; loop++) {
		if(console->ConsoleScrollBack != 0 && loop == 0)
			for(loop2 = 0; loop2 < (console->VChars / 5) + 1; loop2++)
			{
				GrString (CON_CHAR_BORDER + (loop2*5*console->ConsoleSurface->cvFont->ftWidth), 
							 (Screenlines - loop - 2) * (CON_LINE_SPACE + console->ConsoleSurface->cvFont->ftHeight), 
							 CON_SCROLL_INDICATOR, NULL);
			}
		else
		{
			GrString(CON_CHAR_BORDER, 
						(Screenlines - loop - 2) * (CON_LINE_SPACE + console->ConsoleSurface->cvFont->ftHeight),
						 console->ConsoleLines[console->ConsoleScrollBack + loop], NULL);
		}
	}
	grdCurCanv->cvFont = orig_font;
	FG_COLOR = orig_color;
	if(!gameOpts->menus.nStyle)
		GrSetCurrentCanvas(canv_save);

#if 0
	if(console->OutputScreen->flags & SDL_OPENGLBLIT)
		SDL_SetColorKey(CurrentFont->FontSurface, 0, 0);
#endif
}

//------------------------------------------------------------------------------

void CON_UpdateOffset(ConsoleInformation* console) {
	if(!console)
		return;

	switch(console->Visible) {
	case CON_CLOSING:
		console->RaiseOffset -= CON_OPENCLOSE_SPEED;
		if(console->RaiseOffset <= 0) {
			console->RaiseOffset = 0;
			console->Visible = CON_CLOSED;
		}
		break;
	case CON_OPENING:
		console->RaiseOffset += CON_OPENCLOSE_SPEED;
		if(console->RaiseOffset >= console->ConsoleSurface->cv_h) {
			console->RaiseOffset = console->ConsoleSurface->cv_h;
			console->Visible = CON_OPEN;
		}
		break;
	case CON_OPEN:
	case CON_CLOSED:
		break;
	}
}

//------------------------------------------------------------------------------
/* Draws the console buffer to the screen if the console is "visible" */

void CON_DrawConsole(ConsoleInformation *console) {
	gsrCanvas *canv_save;
	grsBitmap *clip;

	if(!console)
		return;

	/* only draw if console is visible: here this means, that the console is not CON_CLOSED */
	if(console->Visible == CON_CLOSED)
		return;

	/* Update the scrolling offset */
	CON_UpdateOffset(console);

	/* Update the command line since it has a blinking cursor */
	DrawCommandLine();

#if 0
	/* before drawing, make sure the alpha channel of the console surface is set
	 * properly.  (sigh) I wish we didn't have to do this every frame... */
	if(console->OutputScreen->flags & SDL_OPENGLBLIT)
		CON_AlphaGL(console->ConsoleSurface, console->ConsoleAlpha);
#endif

	if (gameOpts->menus.nStyle)
		NMBlueBox (0, 0, console->ConsoleSurface->cv_w, console->RaiseOffset, 1, 1.0f, 0);
	else {
		canv_save = grdCurCanv;
		GrSetCurrentCanvas(&console->OutputScreen->scCanvas);
		clip = GrCreateSubBitmap(&console->ConsoleSurface->cvBitmap, 0, 
											 console->ConsoleSurface->cv_h - console->RaiseOffset, 
											 console->ConsoleSurface->cv_w, console->RaiseOffset);
		GrBitmap(console->DispX, console->DispY, clip);
		GrFreeSubBitmap(clip);
#if 0
		if(console->OutputScreen->flags & SDL_OPENGLBLIT)
			SDL_UpdateRects(console->OutputScreen, 1, &DestRect);
#endif
		GrSetCurrentCanvas(canv_save);
		}
}

//------------------------------------------------------------------------------

/* Initializes the console */
ConsoleInformation *CON_Init(grsFont *Font, grsScreen *DisplayScreen, int lines, int x, int y, int w, int h)
{
	int loop;
	ConsoleInformation *newinfo;


	/* Create a new console struct and init it. */
	if((newinfo = (ConsoleInformation *) D2_ALLOC(sizeof(ConsoleInformation))) == NULL) {
		//PRINT_ERROR("Could not allocate the space for a new console info struct.\n");
		return NULL;
	}
	newinfo->Visible = CON_CLOSED;
	newinfo->RaiseOffset = 0;
	newinfo->ConsoleLines = NULL;
	newinfo->CommandLines = NULL;
	newinfo->TotalConsoleLines = 0;
	newinfo->ConsoleScrollBack = 0;
	newinfo->TotalCommands = 0;
	newinfo->BackgroundImage = NULL;
#if 0
	newinfo->ConsoleAlpha = SDL_ALPHA_OPAQUE;
#endif
	newinfo->Offset = 0;
	newinfo->InsMode = 1;
	newinfo->CursorPos = 0;
	newinfo->CommandScrollBack = 0;
	newinfo->OutputScreen = DisplayScreen;
	newinfo->Prompt = CON_DEFAULT_PROMPT;
	newinfo->HideKey = CON_DEFAULT_HIDEKEY;

	CON_SetExecuteFunction(newinfo, Default_CmdFunction);
	CON_SetTabCompletion(newinfo, Default_TabFunction);

	/* make sure that the size of the console is valid */
	if(w > newinfo->OutputScreen->scWidth || w < Font->ftWidth * 32)
		w = newinfo->OutputScreen->scWidth;
	if(h > newinfo->OutputScreen->scHeight || h < Font->ftHeight)
		h = newinfo->OutputScreen->scHeight;
	if(x < 0 || x > newinfo->OutputScreen->scWidth - w)
		newinfo->DispX = 0;
	else
		newinfo->DispX = x;
	if(y < 0 || y > newinfo->OutputScreen->scHeight - h)
		newinfo->DispY = 0;
	else
		newinfo->DispY = y;

	/* load the console surface */
	newinfo->ConsoleSurface = GrCreateCanvas(w, h);

	/* Load the consoles font */
	{
		gsrCanvas *canv_save;

		canv_save = grdCurCanv;
		GrSetCurrentCanvas(newinfo->ConsoleSurface);
		GrSetCurFont(Font);
		GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
		GrSetCurrentCanvas(canv_save);
	}


	/* Load the dirty rectangle for user input */
	newinfo->InputBackground = GrCreateBitmap(w, newinfo->ConsoleSurface->cvFont->ftHeight, 1);
#if 0
	SDL_FillRect(newinfo->InputBackground, NULL, SDL_MapRGBA(newinfo->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
#endif

	/* calculate the number of visible characters in the command line */
	newinfo->VChars = (w - CON_CHAR_BORDER) / newinfo->ConsoleSurface->cvFont->ftWidth;
	if(newinfo->VChars > CON_CHARS_PER_LINE)
		newinfo->VChars = CON_CHARS_PER_LINE;

	/* We would like to have a minumum # of lines to guarentee we don't create a memory error */
	if(h / (CON_LINE_SPACE + newinfo->ConsoleSurface->cvFont->ftHeight) > lines)
		newinfo->LineBuffer = h / (CON_LINE_SPACE + newinfo->ConsoleSurface->cvFont->ftHeight);
	else
		newinfo->LineBuffer = lines;


	newinfo->ConsoleLines = (char **)D2_ALLOC(sizeof(char *) * newinfo->LineBuffer);
	newinfo->CommandLines = (char **)D2_ALLOC(sizeof(char *) * newinfo->LineBuffer);
	for(loop = 0; loop < newinfo->LineBuffer; loop++) {
		newinfo->ConsoleLines[loop] = (char *)D2_CALLOC(CON_CHARS_PER_LINE, sizeof(char));
		newinfo->CommandLines[loop] = (char *)D2_CALLOC(CON_CHARS_PER_LINE, sizeof(char));
	}
	memset(newinfo->Command, 0, CON_CHARS_PER_LINE);
	memset(newinfo->LCommand, 0, CON_CHARS_PER_LINE);
	memset(newinfo->RCommand, 0, CON_CHARS_PER_LINE);
	memset(newinfo->VCommand, 0, CON_CHARS_PER_LINE);


	CON_Out(newinfo, "Console initialised.");
	CON_NewLineConsole(newinfo);
	//CON_ListCommands(newinfo);

	return newinfo;
}

//------------------------------------------------------------------------------
/* Makes the console visible */
void CON_Show(ConsoleInformation *console) {
	if(console) {
		console->Visible = CON_OPENING;
		CON_UpdateConsole(console);
	}
}

//------------------------------------------------------------------------------
/* Hides the console (make it invisible) */
void CON_Hide(ConsoleInformation *console) {
	if(console)
		console->Visible = CON_CLOSING;
}

//------------------------------------------------------------------------------
/* tells wether the console is visible or not */
int CON_isVisible(ConsoleInformation *console) {
	if(!console)
		return CON_CLOSED;
	return((console->Visible == CON_OPEN) || (console->Visible == CON_OPENING));
}

//------------------------------------------------------------------------------
/* Frees all the memory loaded by the console */
void CON_Destroy(ConsoleInformation *console) {
	CON_Free(console);
}

/* Frees all the memory loaded by the console */
void CON_Free(ConsoleInformation *console) {
	int i;

	if(!console)
		return;

	//CON_DestroyCommands();
	for(i = 0; i <= console->LineBuffer - 1; i++) {
		D2_FREE(console->ConsoleLines[i]);
		D2_FREE(console->CommandLines[i]);
	}
	D2_FREE(console->ConsoleLines);
	D2_FREE(console->CommandLines);

	console->ConsoleLines = NULL;
	console->CommandLines = NULL;

	GrFreeCanvas(console->ConsoleSurface);
	console->ConsoleSurface = NULL;

	if (console->BackgroundImage)
		GrFreeBitmap(console->BackgroundImage);
	console->BackgroundImage = NULL;

	GrFreeBitmap(console->InputBackground);
	console->InputBackground = NULL;

	D2_FREE(console);
}

//------------------------------------------------------------------------------

/* Increments the console lines */
void CON_NewLineConsole(ConsoleInformation *console) {
	int loop;
	char* temp;

	if(!console)
		return;

	temp = console->ConsoleLines[console->LineBuffer - 1];

	for(loop = console->LineBuffer - 1; loop > 0; loop--)
		console->ConsoleLines[loop] = console->ConsoleLines[loop - 1];

	console->ConsoleLines[0] = temp;

	memset(console->ConsoleLines[0], 0, CON_CHARS_PER_LINE);
	if(console->TotalConsoleLines < console->LineBuffer - 1)
		console->TotalConsoleLines++;

	//Now adjust the ConsoleScrollBack
	//dont scroll if not at bottom
	if(console->ConsoleScrollBack != 0)
		console->ConsoleScrollBack++;
	//boundaries
	if(console->ConsoleScrollBack > console->LineBuffer-1)
		console->ConsoleScrollBack = console->LineBuffer-1;

}

//------------------------------------------------------------------------------

/* Increments the command lines */
void CON_NewLineCommand(ConsoleInformation *console) {
	int loop;
	char *temp;

	if(!console)
		return;

	temp  = console->CommandLines[console->LineBuffer - 1];


	for(loop = console->LineBuffer - 1; loop > 0; loop--)
		console->CommandLines[loop] = console->CommandLines[loop - 1];

	console->CommandLines[0] = temp;

	memset(console->CommandLines[0], 0, CON_CHARS_PER_LINE);
	if(console->TotalCommands < console->LineBuffer - 1)
		console->TotalCommands++;
}

//------------------------------------------------------------------------------
/* Draws the command line the user is typing in to the screen */
/* completely rewritten by C.Wacha */
void DrawCommandLine() {
	int x;
	int commandbuffer;
#if 0
	grsFont* CurrentFont;
#endif
	static int	LastBlinkTime = 0;	/* Last time the consoles cursor blinked */
	static int	LastCursorPos = 0;		// Last Cursor Position
	static int	bBlink = 0;			/* Is the cursor currently blinking */
	gsrCanvas	*canv_save = NULL;
	grsColor	orig_color;
	grsFont		*orig_font;

	if(!Topmost)
		return;

	commandbuffer = Topmost->VChars - (int) strlen(Topmost->Prompt) - 1; // -1 to make cursor visible

#if 0
	CurrentFont = Topmost->ConsoleSurface->cvFont;
#endif

	//Concatenate the left and right tSide to command
	strcpy(Topmost->Command, Topmost->LCommand);
	strncat(Topmost->Command, Topmost->RCommand, strlen(Topmost->RCommand));

	//calculate display offset from current cursor position
	if(Topmost->Offset < Topmost->CursorPos - commandbuffer)
		Topmost->Offset = Topmost->CursorPos - commandbuffer;
	if(Topmost->Offset > Topmost->CursorPos)
		Topmost->Offset = Topmost->CursorPos;

	//first add prompt to visible part
	strcpy(Topmost->VCommand, Topmost->Prompt);

	//then add the visible part of the command
	strncat(Topmost->VCommand, &Topmost->Command[Topmost->Offset], strlen(&Topmost->Command[Topmost->Offset]));

	//now display the result

#if 0
	//once again we're drawing text, so in OpenGL context we need to temporarily set up
	//software-mode transparency.
	if(Topmost->OutputScreen->flags & SDL_OPENGLBLIT) {
		Uint32 *pix = (Uint32 *) (CurrentFont->FontSurface->pixels);
		SDL_SetColorKey(CurrentFont->FontSurface, SDL_SRCCOLORKEY, *pix);
	}
#endif


	//first of all restore InputBackground
	if (!gameOpts->menus.nStyle) {
		canv_save = grdCurCanv;
		GrSetCurrentCanvas(Topmost->ConsoleSurface);
		GrBitmap(0, Topmost->ConsoleSurface->cv_h - Topmost->ConsoleSurface->cvFont->ftHeight, Topmost->InputBackground);
		}
	//now add the text
	orig_color = FG_COLOR;
	orig_font = grdCurCanv->cvFont;
	grdCurCanv->cvFont = SMALL_FONT;
	GrSetFontColorRGBi (WHITE_RGBA, 1, 0, 0);
	GrString(CON_CHAR_BORDER, Topmost->ConsoleSurface->cv_h - Topmost->ConsoleSurface->cvFont->ftHeight, Topmost->VCommand, NULL);

	//at last add the cursor
	//check if the blink period is over
	if(get_msecs() > LastBlinkTime) {
		LastBlinkTime = get_msecs() + CON_BLINK_RATE;
		if(bBlink)
			bBlink = 0;
		else
			bBlink = 1;
	}

	//check if cursor has moved - if yes display cursor anyway
	if(Topmost->CursorPos != LastCursorPos) {
		LastCursorPos = Topmost->CursorPos;
		LastBlinkTime = get_msecs() + CON_BLINK_RATE;
		bBlink = 1;
	}

	if(bBlink) {
#if 1
		int w, h, aw;
		GrGetStringSize (Topmost->VCommand, &w, &h, &aw);
		x = CON_CHAR_BORDER + w;
#else
		x = CON_CHAR_BORDER + Topmost->ConsoleSurface->cvFont->ftWidth * (Topmost->CursorPos - Topmost->Offset + (int) strlen(Topmost->Prompt));
#endif
		if(Topmost->InsMode)
			GrString(x, Topmost->ConsoleSurface->cv_h - Topmost->ConsoleSurface->cvFont->ftHeight, CON_INS_CURSOR, NULL);
		else
			GrString(x, Topmost->ConsoleSurface->cv_h - Topmost->ConsoleSurface->cvFont->ftHeight, CON_OVR_CURSOR, NULL);
	}
	grdCurCanv->cvFont = orig_font;
	FG_COLOR = orig_color;

	if (!gameOpts->menus.nStyle)
		GrSetCurrentCanvas(canv_save);


#if 0
	if(Topmost->OutputScreen->flags & SDL_OPENGLBLIT) {
		SDL_SetColorKey(CurrentFont->FontSurface, 0, 0);
	}
#endif
}

#ifdef _WIN32
# define vsnprintf _vsnprintf
#endif

//------------------------------------------------------------------------------
/* Outputs text to the console (in game), up to CON_CHARS_PER_LINE chars can be entered */
void _CDECL_ CON_Out(ConsoleInformation *console, const char *str, ...) {
	va_list marker;
	//keep some space D2_FREE for stuff like CON_Out(console, "blablabla %s", console->Command);
	char temp[CON_CHARS_PER_LINE + 128];
	char* ptemp;

	if(!console)
		return;

	va_start(marker, str);
	vsnprintf(temp, CON_CHARS_PER_LINE + 127, str, marker);
	va_end(marker);

	ptemp = temp;

	//temp now contains the complete string we want to output
	// the only problem is that temp is maybe longer than the console
	// width so we have to cut it into several pieces

	if(console->ConsoleLines) {
		while(strlen(ptemp) > (size_t) console->VChars) {
			CON_NewLineConsole(console);
			strncpy(console->ConsoleLines[0], ptemp, console->VChars);
			console->ConsoleLines[0][console->VChars] = '\0';
			ptemp = &ptemp[console->VChars];
		}
		CON_NewLineConsole(console);
		strncpy(console->ConsoleLines[0], ptemp, console->VChars);
		console->ConsoleLines[0][console->VChars] = '\0';
		CON_UpdateConsole(console);
	}

	/* And print to stdout */
	//printf("%s\n", temp);
}


//------------------------------------------------------------------------------
#if 0
/* Sets the alpha level of the console, 0 turns off alpha blending */
void CON_Alpha(ConsoleInformation *console, unsigned char alpha) {
	if(!console)
		return;

	/* store alpha as state! */
	console->ConsoleAlpha = alpha;

	if((console->OutputScreen->flags & SDL_OPENGLBLIT) == 0) {
		if(alpha == 0)
			SDL_SetAlpha(console->ConsoleSurface, 0, alpha);
		else
			SDL_SetAlpha(console->ConsoleSurface, SDL_SRCALPHA, alpha);
	}

	//	CON_UpdateConsole(console);
}
#endif


//------------------------------------------------------------------------------
/* Adds  background image to the console, scaled to size of console*/
int CON_Background(ConsoleInformation *console, grsBitmap *image)
{
	if(!console)
		return 1;

	/* Free the background from the console */
	if (image == NULL) {
		if (console->BackgroundImage)
			GrFreeBitmap(console->BackgroundImage);
		console->BackgroundImage = NULL;
#if 0
		SDL_FillRect(console->InputBackground, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
#endif
		return 0;
	}

	/* Load a new background */
	if (console->BackgroundImage)
		GrFreeBitmap(console->BackgroundImage);
	console->BackgroundImage = GrCreateBitmap(console->ConsoleSurface->cv_w, console->ConsoleSurface->cv_h, 1);
	GrBitmapScaleTo(image, console->BackgroundImage);

#if 0
	SDL_FillRect(console->InputBackground, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
#endif
	GrBmBitBlt(console->BackgroundImage->bmProps.w, console->InputBackground->bmProps.h, 0, 0, 0, console->ConsoleSurface->cv_h - console->ConsoleSurface->cvFont->ftHeight, console->BackgroundImage, console->InputBackground);

	return 0;
}

//------------------------------------------------------------------------------
/* Sets font info for the console */
void CON_Font(ConsoleInformation *console, grsFont *font, unsigned int fg, unsigned int bg)
{
	gsrCanvas *canv_save;

	canv_save = grdCurCanv;
	GrSetCurrentCanvas(console->ConsoleSurface);
	GrSetCurFont(font);
	GrSetFontColorRGBi (fg, 1, bg, bg != 0);
	GrSetCurrentCanvas(canv_save);
}

//------------------------------------------------------------------------------
/* takes a new x and y of the top left of the console window */
void CON_Position(ConsoleInformation *console, int x, int y) {
	if(!console)
		return;

	if(x < 0 || x > console->OutputScreen->scWidth - console->ConsoleSurface->cv_w)
		console->DispX = 0;
	else
		console->DispX = x;

	if(y < 0 || y > console->OutputScreen->scHeight - console->ConsoleSurface->cv_h)
		console->DispY = 0;
	else
		console->DispY = y;
}

//------------------------------------------------------------------------------

/* resizes the console, has to reset alot of stuff
 * returns 1 on error */
int CON_Resize(ConsoleInformation *console, int x, int y, int w, int h)
{
	if(!console)
		return 1;

	/* make sure that the size of the console is valid */
	if(w > console->OutputScreen->scWidth || w < console->ConsoleSurface->cvFont->ftWidth * 32)
		w = console->OutputScreen->scWidth;
	if(h > console->OutputScreen->scHeight || h < console->ConsoleSurface->cvFont->ftHeight)
		h = console->OutputScreen->scHeight;
	if(x < 0 || x > console->OutputScreen->scWidth - w)
		console->DispX = 0;
	else
		console->DispX = x;
	if(y < 0 || y > console->OutputScreen->scHeight - h)
		console->DispY = 0;
	else
		console->DispY = y;

	/* resize console surface */
	GrFreeBitmapData(&console->ConsoleSurface->cvBitmap);
	GrInitBitmapAlloc(&console->ConsoleSurface->cvBitmap, BM_LINEAR, 0, 0, w, h, w, 1);

	/* Load the dirty rectangle for user input */
	GrFreeBitmap(console->InputBackground);
	console->InputBackground = GrCreateBitmap(w, console->ConsoleSurface->cvFont->ftHeight, 1);

	/* Now reset some stuff dependent on the previous size */
	console->ConsoleScrollBack = 0;

	/* Reload the background image (for the input text area) in the console */
	if(console->BackgroundImage) {
#if 0
		SDL_FillRect(console->InputBackground, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
#endif
		GrBmBitBlt(console->BackgroundImage->bmProps.w, console->InputBackground->bmProps.h, 0, 0, 0, console->ConsoleSurface->cv_h - console->ConsoleSurface->cvFont->ftHeight, console->BackgroundImage, console->InputBackground);
	}

#if 0
	/* restore the alpha level */
	CON_Alpha(console, console->ConsoleAlpha);
#endif
	return 0;
}

//------------------------------------------------------------------------------
/* Transfers the console to another screen surface, and adjusts size */
int CON_Transfer(ConsoleInformation *console, grsScreen *new_outputscreen, int x, int y, int w, int h)
{
	if(!console)
		return 1;

	console->OutputScreen = new_outputscreen;

	return(CON_Resize(console, x, y, w, h));
}

/* Sets the topmost console for input */
void CON_Topmost(ConsoleInformation *console) {
	gsrCanvas *canv_save;
	short orig_color;

	if(!console)
		return;

	// Make sure the blinking cursor is gone
	if(Topmost) {
		canv_save = grdCurCanv;
		GrSetCurrentCanvas(Topmost->ConsoleSurface);

		GrBitmap(0, Topmost->ConsoleSurface->cv_h - Topmost->ConsoleSurface->cvFont->ftHeight, Topmost->InputBackground);
		orig_color = FG_COLOR.index;
		GrString(CON_CHAR_BORDER, Topmost->ConsoleSurface->cv_h - Topmost->ConsoleSurface->cvFont->ftHeight, Topmost->VCommand, NULL);
		FG_COLOR.index = orig_color;

		GrSetCurrentCanvas(canv_save);
	}
	Topmost = console;
}

//------------------------------------------------------------------------------
/* Sets the Prompt for console */
void CON_SetPrompt(ConsoleInformation *console, char* newprompt) {
	if(!console)
		return;

	//check length so we can still see at least 1 char :-)
	if(strlen(newprompt) < (size_t) console->VChars)
		console->Prompt = D2_STRDUP(newprompt);
	else
		CON_Out(console, "prompt too long. (max. %i chars)", console->VChars - 1);
}

//------------------------------------------------------------------------------
/* Sets the key that deactivates (hides) the console. */
void CON_SetHideKey(ConsoleInformation *console, int key) {
	if(console)
		console->HideKey = key;
}

//------------------------------------------------------------------------------
/* Executes the command entered */
void CON_Execute(ConsoleInformation *console, char* command) {
	if(console)
		console->CmdFunction(console, command);
}

//------------------------------------------------------------------------------

void CON_SetExecuteFunction(ConsoleInformation *console, void(*CmdFunction)(ConsoleInformation *console2, char* command)) {
	if(console)
		console->CmdFunction = CmdFunction;
}

//------------------------------------------------------------------------------

void Default_CmdFunction(ConsoleInformation *console, char* command) {
	CON_Out(console, "     No CommandFunction registered");
	CON_Out(console, "     use 'CON_SetExecuteFunction' to register one");
	CON_Out(console, " ");
	CON_Out(console, "Unknown Command \"%s\"", command);
}

//------------------------------------------------------------------------------

void CON_SetTabCompletion(ConsoleInformation *console, char*(*TabFunction)(char* command)) {
	if(console)
		console->TabFunction = TabFunction;
}

//------------------------------------------------------------------------------

void CON_TabCompletion(ConsoleInformation *console) {
	int i,j;
	char* command;

	if(!console)
		return;

	command = D2_STRDUP(console->LCommand);
	command = console->TabFunction(command);

	if(!command)
		return;	//no tab completion took place so return silently

	j = (int) strlen(command);
	if(j > CON_CHARS_PER_LINE - 2)
		j = CON_CHARS_PER_LINE-1;

	memset(console->LCommand, 0, CON_CHARS_PER_LINE);
	console->CursorPos = 0;

	for(i = 0; i < j; i++) {
		console->CursorPos++;
		console->LCommand[i] = command[i];
	}
	//add a trailing space
	console->CursorPos++;
	console->LCommand[j] = ' ';
	console->LCommand[j+1] = '\0';
}

//------------------------------------------------------------------------------

char* Default_TabFunction(char* command) {
	CON_Out(Topmost, "     No TabFunction registered");
	CON_Out(Topmost, "     use 'CON_SetTabCompletion' to register one");
	CON_Out(Topmost, " ");
	return NULL;
}

//------------------------------------------------------------------------------

void Cursor_Left(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	if(Topmost->CursorPos > 0) {
		Topmost->CursorPos--;
		strcpy(temp, Topmost->RCommand);
		strcpy(Topmost->RCommand, &Topmost->LCommand[strlen(Topmost->LCommand)-1]);
		strcat(Topmost->RCommand, temp);
		Topmost->LCommand[strlen(Topmost->LCommand)-1] = '\0';
		//CON_Out(Topmost, "L:%s, R:%s", Topmost->LCommand, Topmost->RCommand);
	}
}

//------------------------------------------------------------------------------

void Cursor_Right(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	if(Topmost->CursorPos < (int) strlen(Topmost->Command)) {
		Topmost->CursorPos++;
		strncat(Topmost->LCommand, Topmost->RCommand, 1);
		strcpy(temp, Topmost->RCommand);
		strcpy(Topmost->RCommand, &temp[1]);
		//CON_Out(Topmost, "L:%s, R:%s", Topmost->LCommand, Topmost->RCommand);
	}
}

//------------------------------------------------------------------------------

void Cursor_Home(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	Topmost->CursorPos = 0;
	strcpy(temp, Topmost->RCommand);
	strcpy(Topmost->RCommand, Topmost->LCommand);
	strncat(Topmost->RCommand, temp, strlen(temp));
	memset(Topmost->LCommand, 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void Cursor_End(ConsoleInformation *console) {
	Topmost->CursorPos = (int) strlen(Topmost->Command);
	strncat(Topmost->LCommand, Topmost->RCommand, strlen(Topmost->RCommand));
	memset(Topmost->RCommand, 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void Cursor_Del(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	if(strlen(Topmost->RCommand) > 0) {
		strcpy(temp, Topmost->RCommand);
		strcpy(Topmost->RCommand, &temp[1]);
	}
}

//------------------------------------------------------------------------------

void Cursor_BSpace(ConsoleInformation *console) {
	if(Topmost->CursorPos > 0) {
		Topmost->CursorPos--;
		Topmost->Offset--;
		if(Topmost->Offset < 0)
			Topmost->Offset = 0;
		Topmost->LCommand[strlen(Topmost->LCommand)-1] = '\0';
	}
}

//------------------------------------------------------------------------------

void Cursor_Add(ConsoleInformation *console, int event)
{
	if(strlen(Topmost->Command) < CON_CHARS_PER_LINE - 1)
	{
		Topmost->CursorPos++;
		Topmost->LCommand[strlen(Topmost->LCommand)] = KeyToASCII(event);
		Topmost->LCommand[strlen(Topmost->LCommand)] = '\0';
	}
}

//------------------------------------------------------------------------------

void Clear_Command(ConsoleInformation *console) {
	Topmost->CursorPos = 0;
	memset(Topmost->VCommand, 0, CON_CHARS_PER_LINE);
	memset(Topmost->Command, 0, CON_CHARS_PER_LINE);
	memset(Topmost->LCommand, 0, CON_CHARS_PER_LINE);
	memset(Topmost->RCommand, 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void Clear_History(ConsoleInformation *console) {
	int loop;

	for(loop = 0; loop <= console->LineBuffer - 1; loop++)
		memset(console->ConsoleLines[loop], 0, CON_CHARS_PER_LINE);
}

//------------------------------------------------------------------------------

void Command_Up(ConsoleInformation *console) {
	if(console->CommandScrollBack < console->TotalCommands - 1) {
		/* move back a line in the command strings and copy the command to the current input string */
		console->CommandScrollBack++;
		memset(console->RCommand, 0, CON_CHARS_PER_LINE);
		console->Offset = 0;
		strcpy(console->LCommand, console->CommandLines[console->CommandScrollBack]);
		console->CursorPos = (int) strlen(console->CommandLines[console->CommandScrollBack]);
		CON_UpdateConsole(console);
	}
}

//------------------------------------------------------------------------------

void Command_Down(ConsoleInformation *console) {
	if(console->CommandScrollBack > -1) {
		/* move forward a line in the command strings and copy the command to the current input string */
		console->CommandScrollBack--;
		memset(console->RCommand, 0, CON_CHARS_PER_LINE);
		memset(console->LCommand, 0, CON_CHARS_PER_LINE);
		console->Offset = 0;
		if(console->CommandScrollBack > -1)
			strcpy(console->LCommand, console->CommandLines[console->CommandScrollBack]);
		console->CursorPos = (int) strlen(console->LCommand);
		CON_UpdateConsole(console);
	}
}

//------------------------------------------------------------------------------
//eof
