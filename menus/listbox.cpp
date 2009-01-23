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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "inferno.h"
#include "ipx.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"
#include "joy.h"
#include "slew.h"
#include "findfile.h"
#include "args.h"
#include "hogfile.h"
#include "newdemo.h"
#include "timer.h"
#include "text.h"
#include "gamefont.h"
#include "menu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "strutil.h"
#include "reorder.h"
#include "ogl_render.h"
#include "render.h"
#include "light.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "mouse.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#include "soundthreads.h"
#include "menubackground.h"
#include "songs.h"
#ifdef EDITOR
#	include "editor/editor.h"
#endif

//------------------------------------------------------------------------------ 

#define LHX(x) (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y) (gameStates.menus.bHires? (24 * (y)) / 10 : y)

//------------------------------------------------------------------------------ 

#define LB_ITEMS_ON_SCREEN 8

int ListBox (const char* pszTitle, CStack<char*>& items, int nDefaultItem, int bAllowAbort, pListBoxCallback callback)
{
	int	i;
	int	done, ocitem, nItem, nPrevItem, nFirstItem, key, redraw;
	int	bKeyRepeat = gameStates.input.keys.bRepeat;
	int	width, height, wx, wy, nTitleHeight, border_size;
	int	total_width, total_height;
	int	mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;	//, bDblClick;
	int	close_x, close_y, bWheelUp, bWheelDown;
	int	nVisibleItems;
	char	szPattern [40];
	int	nPatternLen = 0;
	int	w, h, aw;
	char*	pszFn;

	gameStates.input.keys.bRepeat = 1;

//	SetScreenMode (SCREEN_MENU);
	SetPopupScreenMode ();
	CCanvas::SetCurrent (NULL);
	fontManager.SetCurrent (SUBTITLE_FONT);

width = 0;
for (i = 0; i < int (items.ToS ()); i++) {
	int w, h, aw;
	fontManager.Current ()->StringSize (items [i], w, h, aw);	
	if (w > width)
		width = w;
	}
nVisibleItems = LB_ITEMS_ON_SCREEN * CCanvas::Current ()->Height () / 480;
height = (CCanvas::Current ()->Font ()->Height () + 2) * nVisibleItems;

fontManager.Current ()->StringSize (pszTitle, w, h, aw);	
if (w > width)
	width = w;
nTitleHeight = h + 5;

border_size = CCanvas::Current ()->Font ()->Width ();

width += (CCanvas::Current ()->Font ()->Width ());
if (width > CCanvas::Current ()->Width () - (CCanvas::Current ()->Font ()->Width () * 3))
	width = CCanvas::Current ()->Width () - (CCanvas::Current ()->Font ()->Width () * 3);

wx = (CCanvas::Current ()->Width () - width)/2;
wy = (CCanvas::Current ()->Height () - (height + nTitleHeight))/2 + nTitleHeight;
if (wy < nTitleHeight)
	wy = nTitleHeight;

total_width = width + 2*border_size;
total_height = height + 2*border_size + nTitleHeight;

CCanvas::Push ();
backgroundManager.Setup (NULL, wx - border_size, wy - nTitleHeight - border_size, 
								 width + border_size * 2, height + nTitleHeight + border_size * 2);
//GrUpdate (0);
CCanvas::Pop ();
GrString (0x8000, wy - nTitleHeight, pszTitle, NULL);
done = 0;
nItem = nDefaultItem;
if (nItem < 0) 
	nItem = 0;
if (nItem >= int (items.ToS ())) 
	nItem = 0;

nFirstItem = -1;

nMouseState = nOldMouseState = 0;	//bDblClick = 0;
close_x = wx - border_size;
close_y = wy - nTitleHeight - border_size;
CMenu::DrawCloseBox (close_x, close_y);
SDL_ShowCursor (1);

SDL_EnableKeyRepeat(60, 30);
while (!done) {
	ocitem = nItem;
	nPrevItem = nFirstItem;
	nOldMouseState = nMouseState;
	nMouseState = MouseButtonState (0);
	bWheelUp = MouseButtonState (3);
	bWheelDown = MouseButtonState (4);
	//see if redbook song needs to be restarted
	redbook.CheckRepeat ();

	if (bWheelUp)
		key = KEY_UP;
	else if (bWheelDown)
		key = KEY_DOWN;
	else
		key = KeyInKey ();

	if (callback)
		redraw = (*callback) (&nItem, items, &key);
	else
		redraw = 0;

	if (key< - 1) {
		nItem = key;
		key = -1;
		done = 1;
		}

	switch (key) {
		case KEY_CTRLED + KEY_F1:
			SwitchDisplayMode ( - 1);
			break;
		case KEY_CTRLED + KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_CTRLED + KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_PRINT_SCREEN: 	
			SaveScreenShot (NULL, 0); 
			break;
		case KEY_HOME:
		case KEY_PAD7:
			nItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			nItem = int (items.ToS ()) - 1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			nItem--;		
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			nItem++;		
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			nItem += nVisibleItems;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			nItem -= nVisibleItems;
			break;
		case KEY_ESC:
			if (bAllowAbort) {
				nItem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		case KEY_BACKSP:
			if (!gameOpts->menus.bSmartFileSearch)
				break;
			if (nPatternLen > 0)
				szPattern [--nPatternLen] = '\0';
				
		default:
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < (int) sizeof (szPattern) - 1)) {
				int nStart,
					 ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						nStart = nItem;
						cc = nItem + 1;
						if (cc < 0) 
							cc = 0;
						else if (cc >= int (items.ToS ())) 
							cc = 0;
						}
					else {
						nStart = 0;
						cc = 0;
						if (key != KEY_BACKSP) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = items [cc];
						if (items [cc][0] == '[') {
							if (((items [cc][1] == '1') || (items [cc][1] == '2')) && (items [cc][2] == ']'))
								pszFn += 4;
							else
								pszFn++;
							}
						strlwr (pszFn);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == tolower (ascii)) {
							nItem = cc;
							bFound = 1;
							break;
							}
						cc++;
						cc %= int (items.ToS ());
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound && (nPatternLen > 0))
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
		if (done) break;

		if (nItem < 0)
			nItem = int (items.ToS ()) - 1;
		else if (nItem >= int (items.ToS ()))
			nItem = 0;
		if (nItem < nFirstItem)
			nFirstItem = nItem;
		else if (nItem >= (nFirstItem + nVisibleItems))
			nFirstItem = nItem - nVisibleItems + 1;
		if (int (items.ToS ()) <= nVisibleItems)
			 nFirstItem = 0;
		if (nFirstItem > int (items.ToS ()) - nVisibleItems)
			nFirstItem = int (items.ToS ()) - nVisibleItems;
		if (nFirstItem < 0) 
			nFirstItem = 0;

		if (nMouseState) {
			int w, h, aw;

			MouseGetPos (&mx, &my);
			for (i = nFirstItem; i < nFirstItem + nVisibleItems; i++) {
				if (i > int (items.ToS ()))
					break;
				fontManager.Current ()->StringSize (items [i], w, h, aw);
				x1 = wx;
				x2 = wx + width;
				y1 = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + wy;
				y2 = y1 + h + 1;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					//if (i == nItem) {
					//	break;
					//}
					//bDblClick = 0;
					nItem = i;
					done = 1;
					break;
				}
			}
		}

		//check for close box clicked
		if (!nMouseState && nOldMouseState) {
			MouseGetPos (&mx, &my);
			x1 = close_x + MENU_CLOSE_X + 2;
			x2 = x1 + MENU_CLOSE_SIZE - 2;
			y1 = close_y + MENU_CLOSE_Y + 2;
			y2 = y1 + MENU_CLOSE_SIZE - 2;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				nItem = -1;
				done = 1;
			}
		}

		if ((nPrevItem != nFirstItem) || redraw || MODERN_STYLE) {
			if (MODERN_STYLE) 
				backgroundManager.Redraw ();
				//NMDrawBackground (wx - border_size, wy - nTitleHeight - border_size, wx + width + border_size - 1, wy + height + border_size - 1,1);
			else
				SDL_ShowCursor (0);
			if (MODERN_STYLE) {
				fontManager.SetCurrent (NORMAL_FONT);
				GrString (0x8000, wy - nTitleHeight, pszTitle, NULL);
				}

			CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
			for (i = nFirstItem; i < nFirstItem + nVisibleItems; i++) {
				int w, h, aw, y;
				y = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + wy;
				if (i >= int (items.ToS ())) {
					CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
					OglDrawFilledRect (wx, y - 1, wx + width - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
				} else {
					if (i == nItem)
						fontManager.SetCurrent (SELECTED_FONT);
					else
						fontManager.SetCurrent (NORMAL_FONT);
					fontManager.Current ()->StringSize (items [i], w, h, aw);
					OglDrawFilledRect (wx, y - 1, wx + width - 1, y + h + 1);
					GrString (wx + 5, y, items [i], NULL);
				}
			}	

			
			// If Win95 port, draw up/down arrows on left CSide of menu
			SDL_ShowCursor (1);
			GrUpdate (0);
		} else if (nItem != ocitem) {
			int w, h, aw, y;

			if (!MODERN_STYLE) 
				SDL_ShowCursor (0);

			i = ocitem;
			if ((i>= 0) && (i < int (items.ToS ()))) {
				y = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + wy;
				if (i == nItem)
					fontManager.SetCurrent (SELECTED_FONT);
				else
					fontManager.SetCurrent (NORMAL_FONT);
				fontManager.Current ()->StringSize (items [i], w, h, aw);
				OglDrawFilledRect (wx, y - 1, wx + width - 1, y + h + 1);
				GrString (wx + 5, y, items [i], NULL);

			}
			i = nItem;
			if ((i >= 0) && (i < int (items.ToS ()))) {
				y = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + wy;
				if (i == nItem)
					fontManager.SetCurrent (SELECTED_FONT);
				else
					fontManager.SetCurrent (NORMAL_FONT);
				fontManager.Current ()->StringSize (items [i], w, h, aw);
				OglDrawFilledRect (wx, y - 1, wx + width - 1, y + h);
				GrString (wx + 5, y, items [i], NULL);
			}
			SDL_ShowCursor (1);
			GrUpdate (0);
		}
	}

gameStates.input.keys.bRepeat = bKeyRepeat;
backgroundManager.Remove ();
SDL_EnableKeyRepeat(0, 0);
return nItem;
}

//------------------------------------------------------------------------------ 

int FileList (char* pszTitle, char* filespec, char* filename)
{
	static char filenameList [MENU_MAX_FILES][FILENAME_LEN + 1];

	int				i, nFiles;
	CStack<char*>	filenames;// [MENU_MAX_FILES];
	FFS				ffs;

if (!filenames.Create (MENU_MAX_FILES))
	return - 1;
nFiles = 0;
if (!FFF (filespec, &ffs, 0)) {
	do {
		if (filenames.ToS () >= MENU_MAX_FILES) 
			break;
		strncpy (filenameList [nFiles], ffs.name, FILENAME_LEN);
		filenames.Push (reinterpret_cast<char*> (&filenameList [nFiles]));
		} while (!FFN (&ffs, 0));
	FFC (&ffs);
	}
i = ListBox (pszTitle, filenames, 1, NULL);
if (i < 0) 
	return 0;
strcpy (filename, filenames [i]);
return 1;
}

//------------------------------------------------------------------------------
//eof
