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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
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

#define LB_ITEMS_ON_SCREEN 8

//------------------------------------------------------------------------------ 

void CListBox::Render (void)
{
if (m_bDone)
	return;
gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
backgroundManager.Activate (m_background);
gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
FadeIn ();

fontManager.SetScale (fontManager.Scale () * GetScale ());
fontManager.SetCurrent (NORMAL_FONT);
GrString (0x8000, m_nTitleHeight, m_props.pszTitle);

CCanvas textArea;
textArea.Setup (&gameData.render.frame, m_nOffset, m_nOffset + m_nTitleHeight, m_nWidth, m_nHeight, true);
textArea.Activate ("CListbox::Render (textArea)", &m_background);

CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
for (int i = max (m_nFirstItem, 0); i < m_nFirstItem + m_nVisibleItems; i++) {
	int w, h, aw, x, y;

	x = 0;
	y = (i - m_nFirstItem) * (CCanvas::Current ()->Font ()->Height () + 2);

	if (i >= int (m_items->ToS ())) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (x, y - 1, x + m_nWidth - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
		}
	else {
		if (i == m_nChoice)
			fontManager.SetCurrent (SELECTED_FONT);
		else
			fontManager.SetCurrent (NORMAL_FONT);
		fontManager.Current ()->StringSize ((*m_items) [i], w, h, aw);
		OglDrawFilledRect (x, y - 1, x + m_nWidth - 1, y + h + 1);
		GrString (5, y, (*m_items) [i]);
		}
	}	
textArea.Deactivate ();
m_background.Deactivate ();
gameStates.render.grAlpha = 1.0f;
fontManager.SetScale (fontManager.Scale () / GetScale ());
SDL_ShowCursor (1);
}

//------------------------------------------------------------------------------ 

int CListBox::ListBox (const char* pszTitle, CStack<char*>& items, int nDefaultItem, int bAllowAbort, pListBoxCallback callback)
{
	int	i;
	int	bKeyRepeat = gameStates.input.keys.bRepeat;
	int	x1, x2, y1, y2, nMouseState, nOldMouseState;	//, bDblClick;
	int	xClose, yClose, bWheelUp, bWheelDown;
	char	szPattern [40];
	int	nPatternLen = 0;
	int	w, h, aw;
	char*	pszFn;

m_tEnter = -1;
m_items = &items;
m_callback = NULL;

gameStates.input.keys.bRepeat = 1;
SetPopupScreenMode ();
gameData.render.frame.Activate ("CListBox::ListBox (frame)");
fontManager.SetCurrent (SUBTITLE_FONT);

m_nWidth = 0;
for (i = 0; i < int (items.ToS ()); i++) {
//	int w, h, aw;
	fontManager.Current ()->StringSize (items [i], w, h, aw);	
	if (w > m_nWidth)
		m_nWidth = w;
	}
m_nVisibleItems = LB_ITEMS_ON_SCREEN * CCanvas::Current ()->Height () / 480;
m_nHeight = int (float ((CCanvas::Current ()->Font ()->Height () + 2) * m_nVisibleItems) * GetScale ());

fontManager.Current ()->StringSize (pszTitle, w, h, aw);	
if (w > m_nWidth)
	m_nWidth = w;
m_nTitleHeight = int ((h + 5) * GetScale ());

m_nOffset = int (CCanvas::Current ()->Font ()->Width () * GetScale ());
m_nWidth += (CCanvas::Current ()->Font ()->Width ());

int nMargin = CCanvas::Current ()->Font ()->Width () * 3;
if (ogl.IsSideBySideDevice ())
	nMargin += 4 * labs (gameData.StereoOffset2D ());
if (m_nWidth > CCanvas::Current ()->Width () - nMargin)
	m_nWidth = CCanvas::Current ()->Width () - nMargin;
m_nWidth = int (m_nWidth * GetScale ());

backgroundManager.Setup (m_background, m_nWidth + m_nOffset * 2, m_nHeight + m_nTitleHeight + m_nOffset * 2);
m_bDone = 0;
m_nChoice = nDefaultItem;
if (m_nChoice < 0) 
	m_nChoice = 0;
if (m_nChoice >= int (items.ToS ())) 
	m_nChoice = 0;

m_nFirstItem = -1;

nMouseState = nOldMouseState = 0;	//bDblClick = 0;
xClose = m_nOffset;
yClose = m_nTitleHeight - m_nOffset;
CMenu::DrawCloseBox (/*xClose, yClose*/0, 0);
SDL_ShowCursor (1);

SDL_EnableKeyRepeat(60, 30);
while (!m_bDone) {
	nOldMouseState = nMouseState;
	nMouseState = MouseButtonState (0);
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

	m_bRedraw = callback ? (*callback) (&m_nChoice, items, &m_nKey) : 0;

	if (m_nKey < -1) {
		m_nChoice = m_nKey;
		m_nKey = -1;
		m_bDone = 1;
		}

	switch (m_nKey) {
		case KEY_CTRLED + KEY_F1:
			SwitchDisplayMode (-1);
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
			m_nChoice = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			m_nChoice = int (items.ToS ()) - 1;
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
				m_bDone = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			m_bDone = 1;
			break;

		case KEY_BACKSPACE:
			if (!gameOpts->menus.bSmartFileSearch)
				break;
			if (nPatternLen > 0)
				szPattern [--nPatternLen] = '\0';
				
		default:
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < (int) sizeof (szPattern) - 1)) {
				int nStart,
					 ascii = KeyToASCII (m_nKey);
				if ((m_nKey == KEY_BACKSPACE) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						nStart = m_nChoice;
						cc = m_nChoice + 1;
						if (cc < 0) 
							cc = 0;
						else if (cc >= int (items.ToS ())) 
							cc = 0;
						}
					else {
						nStart = 0;
						cc = 0;
						if (m_nKey != KEY_BACKSPACE) {
							szPattern [nPatternLen++] = tolower (ascii);
							szPattern [nPatternLen] = '\0';
							}
						}
					do {
						pszFn = items [cc];
						const char* versionIds [] = {"(D1)", "(D2)", "(XL)"};
						int nSkip = 0;
						for (int i = 0; i < 3; i++)
							if (strstr (pszFn, versionIds [i]) == pszFn) {
								pszFn += 5;
								break;
							}
						if (pszFn [0] == '[')
							pszFn++;
						strlwr (pszFn);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == tolower (ascii)) {
							m_nChoice = cc;
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
		if (m_bDone) break;

		if (m_nChoice < 0)
			m_nChoice = int (items.ToS ()) - 1;
		else if (m_nChoice >= int (items.ToS ()))
			m_nChoice = 0;
		if (m_nChoice < m_nFirstItem)
			m_nFirstItem = m_nChoice;
		else if (m_nChoice >= (m_nFirstItem + m_nVisibleItems))
			m_nFirstItem = m_nChoice - m_nVisibleItems + 1;
		if (int (items.ToS ()) <= m_nVisibleItems)
			 m_nFirstItem = 0;
		if (m_nFirstItem > int (items.ToS ()) - m_nVisibleItems)
			m_nFirstItem = int (items.ToS ()) - m_nVisibleItems;
		if (m_nFirstItem < 0) 
			m_nFirstItem = 0;

		if (nMouseState) {
			int w, h, aw;

			GetMousePos ();
			m_yMouse -= m_nOffset + m_nTitleHeight;
			for (i = m_nFirstItem; i < m_nFirstItem + m_nVisibleItems; i++) {
				if (i >= int (items.ToS ()))
					break;
				fontManager.Current ()->StringSize (items [i], w, h, aw);
				x1 = 0;
				x2 = m_nWidth;
				y1 = (i - m_nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2);
				y2 = y1 + h + 1;
				if (((m_xMouse > x1) && (m_xMouse < x2)) && ((m_yMouse > y1) && (m_yMouse < y2))) {
					//if (i == m_nChoice) {
					//	break;
					//}
					//bDblClick = 0;
					m_nChoice = i;
					m_bDone = 1;
					break;
				}
			}
		}

		//check for close box clicked
		if (!nMouseState && nOldMouseState) {
			MouseGetPos (&m_xMouse, &m_yMouse);
			x1 = xClose + MENU_CLOSE_X + 2;
			x2 = x1 + MENU_CLOSE_SIZE - 2;
			y1 = yClose + MENU_CLOSE_Y + 2;
			y2 = y1 + MENU_CLOSE_SIZE - 2;
			if (((m_xMouse > x1) && (m_xMouse < x2)) && ((m_yMouse > y1) && (m_yMouse < y2))) {
				m_nChoice = -1;
				m_bDone = 1;
			}
		}
	CMenu::Render (pszTitle, NULL);
	}
FadeOut ();
gameStates.input.keys.bRepeat = bKeyRepeat;
backgroundManager.Draw ();
gameData.render.frame.Deactivate ();
SDL_EnableKeyRepeat(0, 0);
return m_nChoice;
}

//------------------------------------------------------------------------------ 

int FileList (char* pszTitle, char* filespec, char* filename)
{
	static char filenameList [MENU_MAX_FILES][FILENAME_LEN + 1];

	int				i, nFiles;
	CStack<char*>	filenames;// [MENU_MAX_FILES];
	FFS				ffs;
	CListBox			lb;

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
i = lb.ListBox (pszTitle, filenames, 0, 1, NULL);
if (i < 0) 
	return 0;
strcpy (filename, filenames [i]);
return 1;
}

//------------------------------------------------------------------------------
//eof
