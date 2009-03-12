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
#include "descent.h"
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
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "ogl_render.h"
#include "rendermine.h"
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

void CFileSelector::Render (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
backgroundManager.Redraw ();
fontManager.SetCurrent (NORMAL_FONT);
FadeIn ();
GrString (0x8000, m_yOffset + 10, pszTitle, NULL);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
for (int i = m_nFirstItem; i < m_nFirstItem + m_nVisibleItems; i++) {
	int w, h, aw, y;
	y = (i - m_nFirstItem) * (CCanvas::Current ()->Font ()->Height () + 2) + m_nTop;

	if (i >= m_nFileCount) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
		OglDrawFilledRect (m_nLeft + m_nWidth, y - 1, m_nLeft + m_nWidth, y + CCanvas::Current ()->Font ()->Height () + 1);
		//OglDrawFilledRect (m_nLeft, y + CCanvas::Current ()->Font ()->Height () + 2, m_nLeft + m_nWidth, y + CCanvas::Current ()->Font ()->Height () + 2);
	
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
		OglDrawFilledRect (m_nLeft - 1, y - 1, m_nLeft - 1, y + CCanvas::Current ()->Font ()->Height () + 2);
	
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (m_nLeft, y - 1, m_nLeft + m_nWidth - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
		} 
	else {
		fontManager.SetCurrent ((i == m_nChoice) ? SELECTED_FONT : NORMAL_FONT);
		fontManager.Current ()->StringSize ((char*) (&m_filenames [i]), w, h, aw);
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
		OglDrawFilledRect (m_nLeft + m_nWidth, y - 1, m_nLeft + m_nWidth, y + h + 1);
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
		OglDrawFilledRect (m_nLeft - 1, y - 1, m_nLeft - 1, y + h + 1);
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (m_nLeft, y - 1, m_nLeft + m_nWidth - 1, y + h + 1);
		GrString (m_nLeft + 5, y, reinterpret_cast<char*> (&m_filenames [i]) + ((m_bPlayerMode && m_filenames [i][0] == '$') ? 1 : 0), NULL);
		}
	}	 
gameStates.render.grAlpha = 1.0f;
SDL_ShowCursor (1);
GrUpdate (0);
}

//------------------------------------------------------------------------------ 

int CFileSelector::FileSelector (const char* pszTitle, const char* filespec, char* filename, int bAllowAbort)
{
	int					i;
	FFS					ffs;
	int					done, m_nOldChoice;
	int					nPrevItem;
	int					bKeyRepeat = gameStates.input.keys.bRepeat;
	int					m_bPlayerMode = 0;
	int					bDemoMode = 0;
	int					bDemosDeleted = 0;
	int					bInitialized = 0;
	int					exitValue = 0;
	int					w_w, w_h, nTitleHeight;
	int					mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;
	int					mouse2State, omouse2State, bWheelUp, bWheelDown;
	int					bDblClick = 0;
	char					szPattern [40];
	int					nPatternLen = 0;
	char*					pszFn;

m_tEnter = -1;
m_nFirstItem = -1;
m_nVisibleItems = 8;
m_bPlayerMode = 0;
m_nFileCount = 0;
m_callback = NULL;

m_xOffset = m_yOffset = w_w = w_h = nTitleHeight = 0;
m_nLeft = m_nTop = m_nWidth = m_nHeight = 0;
if (!m_filenames.Create (MENU_MAX_FILES))
	return 0;
m_nChoice = 0;
gameStates.input.keys.bRepeat = 1;

if (strstr (filespec, "*.plr"))
	m_bPlayerMode = 1;
else if (strstr (filespec, "*.dem"))
	bDemoMode = 1;

ReadFileNames:

done = 0;
m_nFileCount = 0;

#if !defined (APPLE_DEMO)		// no new pilots for special apple oem version
if (m_bPlayerMode) {
	m_filenames [m_nFileCount] = TXT_CREATE_NEW;
	m_nFileCount++;
	}
#endif

if (!FFF (filespec, &ffs, 0)) {
	do {
		if (m_nFileCount < MENU_MAX_FILES) {
			pszFn = (char*) (&m_filenames [m_nFileCount][0]);
			strncpy (pszFn, ffs.name, FILENAME_LEN);
			if (m_bPlayerMode) {
				char* p = strchr (pszFn, '.');
				if (p) 
					*p = '\0';
			}
			if (*pszFn) {
				strlwr (pszFn);
				m_nFileCount++;
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
			if (m_nFileCount < MENU_MAX_FILES) {
				m_filenames [m_nFileCount] = ffs.name;
				m_nFileCount++;
			} else {
				break;
			}
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
		}
	}

if ((m_nFileCount < 1) && bDemosDeleted) {
	exitValue = 0;
	goto exitFileMenu;
	}
if ((m_nFileCount < 1) && bDemoMode) {
	MsgBox (NULL, NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
	exitValue = 0;
	goto exitFileMenu;
	}

if (m_nFileCount < 1) {
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

	for (i = 0; i < m_nFileCount; i++) {
		int w, h, aw;
		fontManager.Current ()->StringSize (m_filenames [i], w, h, aw);	
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

	m_nWidth = w_w;
	m_nHeight = ((CCanvas::Current ()->Font ()->Height () + 2) * m_nVisibleItems);

	w_w += (CCanvas::Current ()->Font ()->Width () * 4);
	w_h = nTitleHeight + m_nHeight + (CCanvas::Current ()->Font ()->Height () * 2);		// more space at bottom

	if (w_w > CCanvas::Current ()->Width ()) 
		w_w = CCanvas::Current ()->Width ();
	if (w_h > CCanvas::Current ()->Height ()) 
		w_h = CCanvas::Current ()->Height ();
	if (w_w > 640)
		w_w = 640;
	if (w_h > 480)
		w_h = 480;

	m_xOffset = (CCanvas::Current ()->Width () - w_w)/2;
	m_yOffset = (CCanvas::Current ()->Height () - w_h)/2;

	if (m_xOffset < 0) 
		m_xOffset = 0;
	if (m_yOffset < 0) 
		m_yOffset = 0;

	m_nLeft = m_xOffset + (CCanvas::Current ()->Font ()->Width ()*2);			// must be in sync with w_w!!!
	m_nTop = m_yOffset + nTitleHeight;

// save the screen behind the menu.

	CCanvas::Push ();
	backgroundManager.Setup (NULL, m_xOffset, m_yOffset, w_w, w_h);
	CCanvas::Pop ();
	GrString (0x8000, m_yOffset + 10, pszTitle, NULL);
	bInitialized = 1;
	}

if (!m_bPlayerMode)
	m_filenames.SortAscending (0, m_nFileCount - 1);
else {
	m_filenames.SortAscending (1, m_nFileCount - 2); 
	for (i = 0; i < m_nFileCount; i++) {
		if (!stricmp (LOCALPLAYER.callsign, m_filenames [i])) {
			bDblClick = 1;
			m_nChoice = i;
			}
	 	}
	}

nMouseState = nOldMouseState = 0;
mouse2State = omouse2State = 0;
CMenu::DrawCloseBox (m_xOffset, m_yOffset);
SDL_ShowCursor (1);

SDL_EnableKeyRepeat(60, 30);
while (!done) {
	m_nOldChoice = m_nChoice;
	nPrevItem = m_nFirstItem;
	nOldMouseState = nMouseState;
	omouse2State = mouse2State;
	nMouseState = MouseButtonState (0);
	mouse2State = MouseButtonState (1);
	bWheelUp = MouseButtonState (3);
	bWheelDown = MouseButtonState (4);
	//see if redbook song needs to be restarted
	redbook.CheckRepeat ();
	if (bWheelUp)
		m_nKey = KEY_UP;
	else if (bWheelDown)
		m_nKey = KEY_DOWN;
	else
		m_nKey = KeyInKey ();
	switch (m_nKey) {
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
			if (((m_bPlayerMode) && (m_nChoice > 0)) || ((bDemoMode) && (m_nChoice >= 0))) {
				int x = 1;
				char* pszFile = (char*) (&m_filenames [m_nChoice][0]);
				if (*pszFile == '$')
					pszFile++;
				SDL_ShowCursor (0);
				if (m_bPlayerMode)
					x = MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, pszFile);
				else if (bDemoMode)
					x = MsgBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, pszFile);
				SDL_ShowCursor (1);
				if (!x) {
					char* p;
					int ret;

					p = pszFile + strlen (pszFile);
					if (m_bPlayerMode)
						*p = '.';
					ret = CFile::Delete (pszFile, m_bPlayerMode ? gameFolders.szProfDir : gameFolders.szDemoDir);
					if (m_bPlayerMode) {
						if (!ret) {
							p [3] = 'x';	//turn ".plr" to ".plx"
							CFile::Delete (pszFile, gameFolders.szProfDir);
							*p = 0;
							DeleteSaveGames (pszFile);
							}
						*p = 0;
						}

					if (ret) {
						if (m_bPlayerMode)
							MsgBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, pszFile);
						else if (bDemoMode)
							MsgBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, pszFile);
						}
					else if (bDemoMode)
						bDemosDeleted = 1;
					m_nFirstItem = -1;
					goto ReadFileNames;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			m_nChoice = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			m_nChoice = m_nFileCount - 1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			m_nChoice--;		
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			m_nChoice++;		
			break;
		case KEY_PAGEDOWN:
		case KEY_PAD3:
			m_nChoice += m_nVisibleItems;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			m_nChoice -= m_nVisibleItems;
			break;
		case KEY_ESC:
			if (bAllowAbort) {
				m_nChoice = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		case KEY_BACKSPACE:
			if (!gameOpts->menus.bSmartFileSearch)
				break;
			if (nPatternLen > 0)
				szPattern [--nPatternLen] = '\0';
				
		default:
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < (int) sizeof (szPattern) - 1)) {
				int nStart, ascii = KeyToASCII (m_nKey);
				if ((m_nKey == KEY_BACKSPACE) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						cc = m_nChoice + 1;
						nStart = m_nChoice;
						if (cc < 0) 
							cc = 0;
						else if (cc >= m_nFileCount) 
							cc = 0;
						}
					else {
						cc = 0;
						nStart = 0;
						if (m_nKey != KEY_BACKSPACE) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = (char*) (&m_filenames [cc][0]);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == toupper (ascii)) {
							m_nChoice = cc;
							bFound = 1;
							break;
						}
						cc++;
						cc %= m_nFileCount;
						} while (cc != nStart);
					if (gameOpts->menus.bSmartFileSearch && !bFound)
						szPattern [--nPatternLen] = '\0';
					}
				}
		}

	if (done) 
		break;

	if (m_nChoice < 0)
		m_nChoice = 0;
	if (m_nChoice >= m_nFileCount)
		m_nChoice = m_nFileCount - 1;

	if (m_nChoice < m_nFirstItem)
		m_nFirstItem = m_nChoice;

	if (m_nChoice >= (m_nFirstItem + m_nVisibleItems))
		m_nFirstItem = m_nChoice - m_nVisibleItems + 1;
	if (m_nFileCount <= m_nVisibleItems)
		 m_nFirstItem = 0;
	if (m_nFirstItem>m_nFileCount - m_nVisibleItems)
		m_nFirstItem = m_nFileCount - m_nVisibleItems;
	if (m_nFirstItem < 0) 
		m_nFirstItem = 0;

	if (nMouseState || mouse2State) {
		int w, h, aw;

		MouseGetPos (&mx, &my);
		for (i = m_nFirstItem; i < m_nFirstItem + m_nVisibleItems; i++) {
			fontManager.Current ()->StringSize (m_filenames [i], w, h, aw);
			x1 = m_nLeft;
			x2 = m_nLeft + m_nWidth - 1;
			y1 = (i - m_nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + m_nTop;
			y2 = y1 + h + 1;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (i == m_nChoice && !mouse2State)
					break;
				m_nChoice = i;
				bDblClick = 0;
				break;
				}
			}
		}

	if (!nMouseState && nOldMouseState) {
		int w, h, aw;

		fontManager.Current ()->StringSize (m_filenames [m_nChoice], w, h, aw);
		MouseGetPos (&mx, &my);
		x1 = m_nLeft;
		x2 = m_nLeft + m_nWidth - 1;
		y1 = (m_nChoice - m_nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + m_nTop;
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
		x1 = m_xOffset + MENU_CLOSE_X + 2;
		x2 = x1 + MENU_CLOSE_SIZE - 2;
		y1 = m_yOffset + MENU_CLOSE_Y + 2;
		y2 = y1 + MENU_CLOSE_SIZE - 2;
		if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
			m_nChoice = -1;
			done = 1;
			}
		}
	Render (pszTitle);
	}
FadeOut ();
//exitFileMenuEarly:
if (m_nChoice < 0)
	exitValue = 0;
else {
	strncpy (filename, reinterpret_cast<char*> (&m_filenames [m_nChoice]) + (m_bPlayerMode && m_filenames [m_nChoice][0] == '$'), FILENAME_LEN);
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
