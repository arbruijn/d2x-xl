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
				OglDrawFilledRect (box_x + box_w, y - 1, box_x + box_w, y + CCanvas::Current ()->Font ()->Height () + 1);
				//OglDrawFilledRect (box_x, y + CCanvas::Current ()->Font ()->Height () + 2, box_x + box_w, y + CCanvas::Current ()->Font ()->Height () + 2);
			
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
				OglDrawFilledRect (box_x - 1, y - 1, box_x - 1, y + CCanvas::Current ()->Font ()->Height () + 2);
			
				CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
				OglDrawFilledRect (box_x, y - 1, box_x + box_w - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
				} 
			else {
				fontManager.SetCurrent ((i == nItem) ? SELECTED_FONT : NORMAL_FONT);
				fontManager.Current ()->StringSize ((char*) (&filenames [i]), w, h, aw);
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
				OglDrawFilledRect (box_x + box_w, y - 1, box_x + box_w, y + h + 1);
				CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
				OglDrawFilledRect (box_x - 1, y - 1, box_x - 1, y + h + 1);
				CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
				OglDrawFilledRect (box_x, y - 1, box_x + box_w - 1, y + h + 1);
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
			OglDrawFilledRect (box_x, y - 1, box_x + box_w - 1, y + h + 1);
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
			OglDrawFilledRect (box_x, y - 1, box_x + box_w - 1, y + h + 1);
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
//eof
