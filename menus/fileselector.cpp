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

void DeleteSaveGames (char* name)
{
	int i;
	char filename [16];

for (i = 0; i < 10; i++) {
	sprintf (filename, "%s.sg%d", name, i);
	CFile::Delete (filename, gameFolders.szSaveDir);
	}
}

//------------------------------------------------------------------------------ 

int FileSelector (const char* pszTitle, const char* filespec, char* filename, int bAllowAbort)
{
	int					i;
	FFS					ffs;
	int					nFileCount = 0, key, done, nItem, ocitem;
	CArray<CFilename>	filenames;
	int					nFilesDisplayed = 8;
	int					nFirstItem = -1, nPrevItem;
	int					bKeyRepeat = gameStates.input.keys.bRepeat;
	int					bPlayerMode = 0;
	int					bDemoMode = 0;
	int					bDemosDeleted = 0;
	int					bInitialized = 0;
	int					exitValue = 0;
	int					w_x, w_y, w_w, w_h, nTitleHeight;
	int					box_x, box_y, box_w, box_h;
	int					mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;
	int					mouse2State, omouse2State, bWheelUp, bWheelDown;
	int					bDblClick = 0;
	char					szPattern [40];
	int					nPatternLen = 0;
	char*					pszFn;

w_x = w_y = w_w = w_h = nTitleHeight = 0;
box_x = box_y = box_w = box_h = 0;
if (!filenames.Create (MENU_MAX_FILES))
	return 0;
nItem = 0;
gameStates.input.keys.bRepeat = 1;

if (strstr (filespec, "*.plr"))
	bPlayerMode = 1;
else if (strstr (filespec, "*.dem"))
	bDemoMode = 1;

ReadFileNames:

done = 0;
nFileCount = 0;

#if !defined (APPLE_DEMO)		// no new pilots for special apple oem version
if (bPlayerMode) {
	filenames [nFileCount] = TXT_CREATE_NEW;
	nFileCount++;
	}
#endif

if (!FFF (filespec, &ffs, 0)) {
	do {
		if (nFileCount < MENU_MAX_FILES) {
			pszFn = (char*) (&filenames [nFileCount][0]);
			strncpy (pszFn, ffs.name, FILENAME_LEN);
			if (bPlayerMode) {
				char* p = strchr (pszFn, '.');
				if (p) 
					*p = '\0';
			}
			if (*pszFn) {
				strlwr (pszFn);
				nFileCount++;
				}
		} else {
			break;
		}
	} while (!FFN (&ffs, 0));
	FFC (&ffs);
	}
if (bDemoMode && gameFolders.bAltHogDirInited) {
	char filespec2 [PATH_MAX + FILENAME_LEN];
	sprintf (filespec2, "%s/%s", gameFolders.szAltHogDir, filespec);
	if (!FFF (filespec2, &ffs, 0)) {
		do {
			if (nFileCount<MENU_MAX_FILES) {
				filenames [nFileCount] = ffs.name;
				nFileCount++;
			} else {
				break;
			}
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	}

if ((nFileCount < 1) && bDemosDeleted) {
	exitValue = 0;
	goto exitFileMenu;
	}
if ((nFileCount < 1) && bDemoMode) {
	MsgBox (NULL, NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
	exitValue = 0;
	goto exitFileMenu;
	}

if (nFileCount < 1) {
#ifndef APPLE_DEMO
	MsgBox (NULL, NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
#endif
	exitValue = 0;
	goto exitFileMenu;
	}

if (!bInitialized) {
//		SetScreenMode (SCREEN_MENU);
	SetPopupScreenMode ();
	CCanvas::SetCurrent (NULL);
	fontManager.SetCurrent (SUBTITLE_FONT);
	w_w = 0;
	w_h = 0;

	for (i = 0; i < nFileCount; i++) {
		int w, h, aw;
		fontManager.Current ()->StringSize (filenames [i], w, h, aw);	
		if (w > w_w)
			w_w = w;
		}
	if (pszTitle) {
		int w, h, aw;
		fontManager.Current ()->StringSize (pszTitle, w, h, aw);	
		if (w > w_w)
			w_w = w;
		nTitleHeight = h + (CCanvas::Current ()->Font ()->Height ()*2);		// add a little space at the bottom of the pszTitle
		}

	box_w = w_w;
	box_h = ((CCanvas::Current ()->Font ()->Height () + 2) * nFilesDisplayed);

	w_w += (CCanvas::Current ()->Font ()->Width () * 4);
	w_h = nTitleHeight + box_h + (CCanvas::Current ()->Font ()->Height () * 2);		// more space at bottom

	if (w_w > CCanvas::Current ()->Width ()) 
		w_w = CCanvas::Current ()->Width ();
	if (w_h > CCanvas::Current ()->Height ()) 
		w_h = CCanvas::Current ()->Height ();
	if (w_w > 640)
		w_w = 640;
	if (w_h > 480)
		w_h = 480;

	w_x = (CCanvas::Current ()->Width () - w_w)/2;
	w_y = (CCanvas::Current ()->Height () - w_h)/2;

	if (w_x < 0) 
		w_x = 0;
	if (w_y < 0) 
		w_y = 0;

	box_x = w_x + (CCanvas::Current ()->Font ()->Width ()*2);			// must be in sync with w_w!!!
	box_y = w_y + nTitleHeight;

// save the screen behind the menu.

	CCanvas::Push ();
	backgroundManager.Setup (NULL, w_x, w_y, w_w, w_h);
	CCanvas::Pop ();
	GrString (0x8000, w_y + 10, pszTitle, NULL);
	bInitialized = 1;
	}

if (!bPlayerMode)
	filenames.SortAscending (0, nFileCount - 1);
else {
	filenames.SortAscending (1, nFileCount - 2); 
	for (i = 0; i < nFileCount; i++) {
		if (!stricmp (LOCALPLAYER.callsign, filenames [i])) {
			bDblClick = 1;
			nItem = i;
			}
	 	}
	}

nMouseState = nOldMouseState = 0;
mouse2State = omouse2State = 0;
CMenu::DrawCloseBox (w_x, w_y);
SDL_ShowCursor (1);

SDL_EnableKeyRepeat(60, 30);
while (!done) {
	ocitem = nItem;
	nPrevItem = nFirstItem;
	GrUpdate (0);
	nOldMouseState = nMouseState;
	omouse2State = mouse2State;
	nMouseState = MouseButtonState (0);
	mouse2State = MouseButtonState (1);
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
	switch (key) {
		case KEY_CTRLED + KEY_F1:
			SwitchDisplayMode ( - 1);
			break;
			
		case KEY_CTRLED + KEY_F2:
			SwitchDisplayMode (1);
			break;
			
		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_ALTED + KEY_F9:
			gameStates.app.bSaveScreenshot = 1;
			SaveScreenShot (NULL, 0);
			break;

		case KEY_CTRLED + KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_CTRLED + KEY_D:
			if (((bPlayerMode) && (nItem > 0)) || ((bDemoMode) && (nItem >= 0))) {
				int x = 1;
				char* pszFile = (char*) (&filenames [nItem][0]);
				if (*pszFile == '$')
					pszFile++;
				SDL_ShowCursor (0);
				if (bPlayerMode)
					x = MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, pszFile);
				else if (bDemoMode)
					x = MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, pszFile);
				SDL_ShowCursor (1);
				if (!x) {
					char* p;
					int ret;

					p = pszFile + strlen (pszFile);
					if (bPlayerMode)
						*p = '.';
					ret = CFile::Delete (pszFile, bPlayerMode ? gameFolders.szProfDir : gameFolders.szDemoDir);
					if (bPlayerMode) {
						if (!ret) {
							p [3] = 'x';	//turn ".plr" to ".plx"
							CFile::Delete (pszFile, gameFolders.szProfDir);
							*p = 0;
							DeleteSaveGames (pszFile);
							}
						*p = 0;
						}

					if (ret) {
						if (bPlayerMode)
							MsgBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, pszFile);
						else if (bDemoMode)
							MsgBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, pszFile);
						}
					else if (bDemoMode)
						bDemosDeleted = 1;
					nFirstItem = -1;
					goto ReadFileNames;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			nItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			nItem = nFileCount - 1;
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
			nItem += nFilesDisplayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			nItem -= nFilesDisplayed;
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
				int nStart, ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						cc = nItem + 1;
						nStart = nItem;
						if (cc < 0) 
							cc = 0;
						else if (cc >= nFileCount) 
							cc = 0;
						}
					else {
						cc = 0;
						nStart = 0;
						if (key != KEY_BACKSP) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = (char*) (&filenames [cc][0]);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == toupper (ascii)) {
							nItem = cc;
							bFound = 1;
							break;
						}
						cc++;
						cc %= nFileCount;
						} while (cc != nStart);
					if (gameOpts->menus.bSmartFileSearch && !bFound)
						szPattern [--nPatternLen] = '\0';
					}
				}
		}

	if (done) 
		break;

	if (nItem < 0)
		nItem = 0;
	if (nItem >= nFileCount)
		nItem = nFileCount - 1;

	if (nItem < nFirstItem)
		nFirstItem = nItem;

	if (nItem >= (nFirstItem + nFilesDisplayed))
		nFirstItem = nItem - nFilesDisplayed + 1;
	if (nFileCount <= nFilesDisplayed)
		 nFirstItem = 0;
	if (nFirstItem>nFileCount - nFilesDisplayed)
		nFirstItem = nFileCount - nFilesDisplayed;
	if (nFirstItem < 0) 
		nFirstItem = 0;

	if (nMouseState || mouse2State) {
		int w, h, aw;

		MouseGetPos (&mx, &my);
		for (i = nFirstItem; i < nFirstItem + nFilesDisplayed; i++) {
			fontManager.Current ()->StringSize (filenames [i], w, h, aw);
			x1 = box_x;
			x2 = box_x + box_w - 1;
			y1 = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
			y2 = y1 + h + 1;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (i == nItem && !mouse2State)
					break;
				nItem = i;
				bDblClick = 0;
				break;
				}
			}
		}

	if (!nMouseState && nOldMouseState) {
		int w, h, aw;

		fontManager.Current ()->StringSize (filenames [nItem], w, h, aw);
		MouseGetPos (&mx, &my);
		x1 = box_x;
		x2 = box_x + box_w - 1;
		y1 = (nItem - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
		y2 = y1 + h + 1;
		if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
			if (bDblClick) 
				done = 1;
			else 
				bDblClick = 1;
			}
		}

	if (!nMouseState && nOldMouseState) {
		MouseGetPos (&mx, &my);
		x1 = w_x + MENU_CLOSE_X + 2;
		x2 = x1 + MENU_CLOSE_SIZE - 2;
		y1 = w_y + MENU_CLOSE_Y + 2;
		y2 = y1 + MENU_CLOSE_SIZE - 2;
		if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
			nItem = -1;
			done = 1;
			}
		}

	if ((nPrevItem != nFirstItem) || MODERN_STYLE) {
		if (!MODERN_STYLE) 
			SDL_ShowCursor (0);
		backgroundManager.Redraw ();
		fontManager.SetCurrent (NORMAL_FONT);
		GrString (0x8000, w_y + 10, pszTitle, NULL);
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		for (i = nFirstItem; i < nFirstItem + nFilesDisplayed; i++) {
			int w, h, aw, y;
			y = (i - nFirstItem) * (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
	
			if (i >= nFileCount) {
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
				GrRect (box_x + box_w, y - 1, box_x + box_w, y + CCanvas::Current ()->Font ()->Height () + 1);
				//GrRect (box_x, y + CCanvas::Current ()->Font ()->Height () + 2, box_x + box_w, y + CCanvas::Current ()->Font ()->Height () + 2);
			
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
				GrRect (box_x - 1, y - 1, box_x - 1, y + CCanvas::Current ()->Font ()->Height () + 2);
			
				CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
				GrRect (box_x, y - 1, box_x + box_w - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
				} 
			else {
				fontManager.SetCurrent ((i == nItem) ? SELECTED_FONT : NORMAL_FONT);
				fontManager.Current ()->StringSize ((char*) (&filenames [i]), w, h, aw);
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
				GrRect (box_x + box_w, y - 1, box_x + box_w, y + h + 1);
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
				GrRect (box_x - 1, y - 1, box_x - 1, y + h + 1);
				CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
				GrRect (box_x, y - 1, box_x + box_w - 1, y + h + 1);
				GrString (box_x + 5, y, reinterpret_cast<char*> (&filenames [i]) + ((bPlayerMode && filenames [i][0] == '$') ? 1 : 0), NULL);
				}
			}	 
		SDL_ShowCursor (1);
		}
	else if (nItem != ocitem) {
		int w, h, aw, y;

		if (!MODERN_STYLE) 
			SDL_ShowCursor (0);
		i = ocitem;
		if ((i >= 0) && (i < nFileCount)) {
			y = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
			if (i == nItem)
				fontManager.SetCurrent (SELECTED_FONT);
			else
				fontManager.SetCurrent (NORMAL_FONT);
			fontManager.Current ()->StringSize (reinterpret_cast<char*> (&filenames [i]), w, h, aw);
			GrRect (box_x, y - 1, box_x + box_w - 1, y + h + 1);
			GrString (box_x + 5, y, reinterpret_cast<char*> (&filenames [i]) + (bPlayerMode && filenames [i][0] == '$'), NULL);
			}
		i = nItem;
		if ((i>= 0) && (i < nFileCount)) {
			y = (i - nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + box_y;
			if (i == nItem)
				fontManager.SetCurrent (SELECTED_FONT);
			else
				fontManager.SetCurrent (NORMAL_FONT);
			fontManager.Current ()->StringSize (reinterpret_cast<char*> (&filenames [i]), w, h, aw);
			GrRect (box_x, y - 1, box_x + box_w - 1, y + h + 1);
			GrString (box_x + 5, y, reinterpret_cast<char*> (&filenames [i]) + (bPlayerMode && filenames [i][0] == '$'), NULL);
			}
		GrUpdate (0);
		SDL_ShowCursor (1);
		}
	}

//exitFileMenuEarly:
if (nItem < 0)
	exitValue = 0;
else {
	strncpy (filename, reinterpret_cast<char*> (&filenames [nItem]) + (bPlayerMode && filenames [nItem][0] == '$'), FILENAME_LEN);
	exitValue = 1;
	}										 

exitFileMenu:
gameStates.input.keys.bRepeat = bKeyRepeat;

if (bInitialized) {
	backgroundManager.Remove ();
	}

SDL_EnableKeyRepeat(0, 0);
return exitValue;
}

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
					GrRect (wx, y - 1, wx + width - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
				} else {
					if (i == nItem)
						fontManager.SetCurrent (SELECTED_FONT);
					else
						fontManager.SetCurrent (NORMAL_FONT);
					fontManager.Current ()->StringSize (items [i], w, h, aw);
					GrRect (wx, y - 1, wx + width - 1, y + h + 1);
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
				GrRect (wx, y - 1, wx + width - 1, y + h + 1);
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
				GrRect (wx, y - 1, wx + width - 1, y + h);
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
//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int _CDECL_ FixedFontMsgBox (char* pszTitle, int nChoices, ...)
{
	int				i;
	char*				format;
	va_list			args;
	char*				s;
	char				szSubTitle [MSGBOX_TEXT_SIZE];
	CMenu	mm;

if (!mm.Create (5))
	return -1;

va_start (args, nChoices);
for (i = 0; i < nChoices; i++) {
	s = va_arg (args, char*);
	mm.AddMenu (s);
	}
format = va_arg (args, char* );
vsprintf (szSubTitle, format, args);
va_end (args);
Assert (strlen (szSubTitle) < MSGBOX_TEXT_SIZE);
return mm.FixedFontMenu (pszTitle, szSubTitle, NULL, 0, NULL, - 1, - 1);
}
//end this section addition - Victor Rachels

//------------------------------------------------------------------------------ 
/* Spiffy word wrap string formatting function */

void NMWrapText (char* dbuf, char* sbuf, int line_length)
{
	int col;
	char* wordptr;
	char* tbuf;

	tbuf = new char [strlen (sbuf) + 1];
	strcpy (tbuf, sbuf);

	wordptr = strtok (tbuf, " ");
	if (!wordptr) return;
	col = 0;
	dbuf [0] = 0;

while (wordptr) {
	col = col + (int) strlen (wordptr) + 1;
	if (col >= line_length) {
		col = 0;
		sprintf (dbuf, "%s\n%s ", dbuf, wordptr);
		}
	else {
		sprintf (dbuf, "%s%s ", dbuf, wordptr);
		}
	wordptr = strtok (NULL, " ");
	}
delete[] tbuf;
}

//------------------------------------------------------------------------------ 

void ProgressBar (const char* szCaption, int nCurProgress, int nMaxProgress, pMenuCallback doProgress)
{
	int	i, nInMenu;
	CMenu	menu;

menu.Create (3);
menu.AddGauge ("                    ", -1, nMaxProgress); //the blank string denotes the screen width of the gauge
menu.Item (0).m_bCentered = 1;
menu.Item (0).m_value = nCurProgress;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameData.app.bGamePaused = 1;
do {
	i = menu.Menu (NULL, szCaption, doProgress);
	} while (i >= 0);
gameData.app.bGamePaused = 0;
gameStates.menus.nInMenu = nInMenu;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 

int CMenu::AddCheck (const char* szText, int nValue, int nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_CHECK;
item.m_text = (char*) (szText);
item.m_value = NMBOOL (nValue);
item.m_nKey = nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddRadio (const char* szText, int nValue, int nKey, const char* szHelp)
{
CMenuItem item;
if (!ToS () || (Top ()->m_nType != NM_TYPE_RADIO))
	NewGroup ();
item.m_nType = NM_TYPE_RADIO;
item.m_text = (char*) (szText);
item.m_value = nValue;
item.m_nKey = nKey;
item.m_group = m_nGroup;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddMenu (const char* szText, int nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_MENU;
item.m_text = (char*) (szText);
item.m_nKey = nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddText (const char* szText, int nKey)
{
CMenuItem item;
item.m_nType = NM_TYPE_TEXT;
item.m_text = (char*) (szText);
item.m_nKey = nKey;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddSlider (const char* szText, int nValue, int nMin, int nMax, int nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_SLIDER;
item.m_text = (char*) (szText);
item.m_value = NMCLAMP (nValue, nMin, nMax);
item.m_minValue = nMin;
item.m_maxValue = nMax;
item.m_nKey = nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szText, int nLen, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_INPUT;
item.m_text = (char*) (szText);
item.m_nTextLen = nLen;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szText, char* szValue, int nLen, const char* szHelp)
{
AddText (szText);
return AddInput (szValue, nLen, szHelp);
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szText, char* szValue, int nValue, int nLen, const char* szHelp)
{
sprintf (szValue, "%d", nValue);
return AddInput (szText, szValue, nLen, szHelp);
}

//------------------------------------------------------------------------------ 

int CMenu::AddInputBox (const char* szText, int nLen, int nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_INPUT_MENU;
item.m_text = (char*) (szText);
item.m_nTextLen = nLen;
item.m_nKey = nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddNumber (const char* szText, int nValue, int nMin, int nMax)
{
CMenuItem item;
item.m_nType = NM_TYPE_NUMBER;
item.m_text = (char*) (szText);
item.m_value = NMCLAMP (nValue, nMin, nMax);
item.m_minValue = nMin;
item.m_maxValue = nMax;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddGauge (const char* szText, int nValue, int nMax)
{
CMenuItem item;
item.m_nType = NM_TYPE_GAUGE;
item.m_text = (char*) (szText);
item.m_nTextLen = *szText ? (int) strlen (szText) : 20;
item.m_value = NMCLAMP (nValue, 0, nMax);
item.m_maxValue = nMax;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------
//eof
