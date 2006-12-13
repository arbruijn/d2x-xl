/* $Id: newmenu.h,v 1.7 2003/11/27 00:36:15 btb Exp $ */
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

/*
 *
 * Routines for menus.
 *
 * Old Log:
 * Revision 1.3  1995/10/17  13:15:19  allender
 * new call to nm_listbox for close_box
 *
 * Revision 1.2  1995/09/13  08:48:50  allender
 * new prototype for newmenu -- have close box
 *
 * Revision 1.1  1995/05/16  16:00:32  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:28  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.24  1995/02/11  16:20:05  john
 * Added code to make the default mission be the one last played.
 *
 * Revision 1.23  1995/01/31  10:21:41  john
 * Added code to specify width, height.
 *
 * Revision 1.22  1995/01/28  17:09:21  john
 * *** empty log message ***
 *
 * Revision 1.21  1995/01/23  18:38:43  john
 * Added listbox function.
 *
 * Revision 1.20  1994/11/26  15:29:55  matt
 * Allow escape out of change pilot menu
 *
 * Revision 1.19  1994/11/26  14:17:05  matt
 * Player can now only enter valid chars for his name
 *
 * Revision 1.18  1994/11/18  16:17:13  mike
 * prototype Max_linear_depthObjects.
 *
 * Revision 1.17  1994/11/08  14:51:17  john
 * Added ExecMessageBox1, (like the original, only you can pass a function).
 *
 * Revision 1.16  1994/11/05  14:31:45  john
 * Added a new menu function for the background.
 *
 * Revision 1.15  1994/11/05  14:05:46  john
 * Fixed fade transitions between all screens by making GrPaletteFadeIn and out keep
 * track of whether the palette is faded in or not.  Then, wherever the code needs to fade out,
 * it just calls GrPaletteFadeOut and it will fade out if it isn't already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out, game_fade_in palette, etc.
 *
 * Revision 1.14  1994/11/03  19:37:35  john
 * Added scrolling file list box
 *
 * Revision 1.13  1994/10/13  11:34:03  john
 * Made Thrustmaster FCS Hat work.  Put a background behind the
 * keyboard configure.  Took out turn_sensitivity.  Changed sound/config
 * menu to new menu. Made F6 be calibrate joystick.
 *
 * Revision 1.12  1994/10/11  17:08:32  john
 * Added sliders for volume controls.
 *
 * Revision 1.11  1994/10/04  10:26:23  matt
 * Changed fade in to happen every time a global var is set
 *
 * Revision 1.10  1994/10/03  14:43:56  john
 * Added ExecMenu1, which allows you to pass the starting
 * item to the menu system
 *
 * Revision 1.9  1994/09/30  11:51:33  john
 * Added Matt's NM_TYPE_INPUT_MENU
 *
 * Revision 1.8  1994/08/30  20:38:28  john
 * Passed citem in newmenu sub.
 *
 * Revision 1.7  1994/08/12  03:11:00  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.6  1994/08/11  13:47:05  john
 * Made newmenu have subtitles, passed key through to
 * the newmenu subfunctions.
 *
 * Revision 1.5  1994/07/27  16:12:24  john
 * Changed newmenu system to have a callback function.
 * /.
 *
 * Revision 1.4  1994/07/24  17:41:38  john
 * *** empty log message ***
 *
 * Revision 1.3  1994/07/24  17:33:01  john
 * Added percent item.  Also neatend up a bit.
 *
 * Revision 1.2  1994/07/22  17:48:12  john
 * Added new menuing system.
 *
 * Revision 1.1  1994/07/22  13:55:51  john
 * Initial revision
 *
 *
 */


#ifndef _NEWMENU_H
#define _NEWMENU_H

#include "gr.h"
#include "cfile.h"

#define NM_TYPE_MENU        0   // A menu item... when enter is hit on this, ExecMenu returns this item number
#define NM_TYPE_INPUT       1   // An input box... fills the text field in, and you need to fill in text_len field.
#define NM_TYPE_CHECK       2   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
#define NM_TYPE_RADIO       3   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
#define NM_TYPE_TEXT        4   // A line of text that does nothing.
#define NM_TYPE_NUMBER      5   // A numeric entry counter.  Changes value from minValue to maxValue;
#define NM_TYPE_INPUT_MENU  6   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
#define NM_TYPE_SLIDER      7   // A slider from minValue to maxValue. Draws with text_len chars.
#define NM_TYPE_GAUGE       8   // A slider from minValue to maxValue. Draws with text_len chars.

#define NM_MAX_TEXT_LEN     50

typedef struct tMenuItem {
	int			nType;           // What kind of item this is, see NM_TYPE_????? defines
	int			value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	int			minValue, maxValue;   // For sliders and number bars.
	int			group;          // What group this belongs to for radio buttons.
	int			text_len;       // The maximum length of characters that can be entered by this inputboxes
	char			*text;          // The text associated with this item.
	char			*textSave;
	unsigned int color;
	short			key;
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short			x, y, xSave, ySave;
	short			w, h;
	short			right_offset;
	ubyte			redraw;
	ubyte			rebuild;
	ubyte			noscroll;
	ubyte			unavailable;
	ubyte			centered;
	char			saved_text[NM_MAX_TEXT_LEN+1];
	grsBitmap	*text_bm [2];
	char			*szHelp;
} tMenuItem;

typedef struct bkg {
	short			x, y, w, h;			// The location of the menu.
	grs_canvas	*menu_canvas;
	grsBitmap	*saved;			// The background under the menu.
	grsBitmap	*background;
	grsBitmap	*bmp;
	char			bIgnoreBg;
	char			bIgnoreCanv;
	char			*pszPrevBg;
} bkg;

// Pass an array of newmenu_items and it processes the menu. It will
// return a -1 if Esc is pressed, otherwise, it returns the index of
// the item that was current when Enter was was selected.
// The subfunction function gets called constantly, so you can dynamically
// change the text of an item.  Just pass NULL if you don't want this.
// Title draws big, Subtitle draw medium sized.  You can pass NULL for
// either/both of these if you don't want them.
int ExecMenu(char * title, char * subtitle, int nitems, tMenuItem *item, 
					void (*subfunction)(int nitems, tMenuItem *items, int *last_key, int citem),
					char *filename);

// Same as above, only you can pass through what item is initially selected.
int ExecMenu1(char *title, char *subtitle, int nitems, tMenuItem *item, 
					 void (*subfunction)(int nitems, tMenuItem *items, int *last_key, int citem), 
					 int *pcitem);

// Same as above, only you can pass through what background bitmap to use.
int ExecMenu2(char *title, char *subtitle, int nitems, tMenuItem *item, 
					 void (*subfunction)(int nitems, tMenuItem *items, int *last_key, int citem), 
					 int *pcitem, char *filename);

// Same as above, only you can pass through the width & height
int ExecMenu3(char *title, char *subtitle, int nitems, tMenuItem *item, 
					 void (*subfunction)(int nitems, tMenuItem *items, int *last_key, int citem), 
					 int *pcitem, char *filename, int width, int height);

void NMLoadBackground (char * filename, bkg *bg, int bReload);

void NMDrawBackground (bkg *bg, int x1, int y1, int x2, int y2, int bReload);

void NMRemoveBackground (bkg *bg);


// This function pops up a messagebox and returns which choice was selected...
// Example:
// ExecMessageBox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nchoices-1.
int _CDECL_ ExecMessageBox(char *title, char *filename, int nchoices, ...);
// Same as above, but you can pass a function
int _CDECL_ ExecMessageBox1 (
					char *title,
					void (*subfunction)(int nitems, tMenuItem *items, int *last_key, int citem), 
					char *filename, int nchoices, ...);

void NMRestoreBackground( int sx, int sy, int dx, int dy, int w, int h);

// Returns 0 if no file selected, else filename is filled with selected file.
int ExecMenuFileSelector(char *title, char *filespec, char *filename, int allow_abortFlag);

// in menu.c
extern int Max_linear_depthObjects;

extern char *nmAllowedChars;

int ExecMenuListBox(char *title, int nitems, char *items[], int allow_abortFlag, 
						  int (*listbox_callback)(int *citem, int *nitems, char *items[], int *keypress));
int ExecMenuListBox1(char *title, int nitems, char *items[], int allow_abortFlag, int default_item, 
							int (*listbox_callback)(int *citem, int *nitems, char *items[], int *keypress));

int ExecMenuFileList(char *title, char *filespace, char *filename);

int ExecMenutiny (char * title, char * subtitle, int nItems, tMenuItem * item, 
						 void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int citem));

int ExecMenutiny2 (char * title, char * subtitle, int nitems, tMenuItem * item, 
							void (*subfunction) (int nitems,tMenuItem * items, int * last_key, int citem));

void NMProgressBar (char *szCaption, int nCurProgress, int nMaxProgress, 
						  void (*doProgress) (int nItems, tMenuItem *items, int *last_key, int cItem));

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int _CDECL_ NMMsgBoxFixedFont(char *title, int nchoices, ...);
//end this section addition

//should be called whenever the palette changes
void NMRemapBackground(void);
void NMLoadAltBg (void);
int NMFreeAltBg (int bForce);

void NMRestoreScreen (char *filename, bkg *bg, grs_canvas *save_canvas, grs_font *saveFont, int bDontRestore);

extern double altBgAlpha;
extern double altBgBrightness;
extern int altBgGrayScale;
extern char altBgName [FILENAME_LEN];

#define newmenu_show_cursor()
#define newmenu_hide_cursor()

#define STARS_BACKGROUND \
			((gameStates.menus.bHires && CFExist ("starsb.pcx", gameFolders.szDataDir, 0)) ? "starsb.pcx":\
			CFExist ("stars.pcx", gameFolders.szDataDir, 0) ? "stars.pcx" : "starsb.pcx")

#endif /* _NEWMENU_H */
