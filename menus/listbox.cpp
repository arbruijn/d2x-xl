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

#define LB_ITEMS_ON_SCREEN 8

//------------------------------------------------------------------------------ 

void CListBox::Render (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
backgroundManager.Redraw ();
FadeIn ();
fontManager.SetCurrent (NORMAL_FONT);
GrString (0x8000, m_yOffset - m_nTitleHeight, pszTitle, NULL);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
for (int i = m_nFirstItem; i < m_nFirstItem + m_nVisibleItems; i++) {
	int w, h, aw, y;
	y = (i - m_nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + m_yOffset;
	if (i >= int (m_items->ToS ())) {
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		OglDrawFilledRect (m_xOffset, y - 1, m_xOffset + m_nWidth - 1, y + CCanvas::Current ()->Font ()->Height () + 1);
		}
	else {
		if (i == m_nChoice)
			fontManager.SetCurrent (SELECTED_FONT);
		else
			fontManager.SetCurrent (NORMAL_FONT);
		fontManager.Current ()->StringSize ((*m_items) [i], w, h, aw);
		OglDrawFilledRect (m_xOffset, y - 1, m_xOffset + m_nWidth - 1, y + h + 1);
		GrString (m_xOffset + 5, y, (*m_items) [i], NULL);
		}
	}	
gameStates.render.grAlpha = 1.0f;
SDL_ShowCursor (1);
GrUpdate (0);
}

//------------------------------------------------------------------------------ 

int CListBox::ListBox (const char* pszTitle, CStack<char*>& items, int nDefaultItem, int bAllowAbort, pListBoxCallback callback)
{
	int	i;
	int	done, m_nOldChoice, nPrevItem, key, m_bRedraw;
	int	bKeyRepeat = gameStates.input.keys.bRepeat;
	int	nOffsetSize;
	int	total_width, total_height;
	int	mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;	//, bDblClick;
	int	close_x, close_y, bWheelUp, bWheelDown;
	char	szPattern [40];
	int	nPatternLen = 0;
	int	w, h, aw;
	char*	pszFn;

m_items = &items;

gameStates.input.keys.bRepeat = 1;
SetPopupScreenMode ();
CCanvas::SetCurrent (NULL);
fontManager.SetCurrent (SUBTITLE_FONT);

m_nWidth = 0;
for (i = 0; i < int (items.ToS ()); i++) {
	int w, h, aw;
	fontManager.Current ()->StringSize (items [i], w, h, aw);	
	if (w > m_nWidth)
		m_nWidth = w;
	}
m_nVisibleItems = LB_ITEMS_ON_SCREEN * CCanvas::Current ()->Height () / 480;
m_nHeight = (CCanvas::Current ()->Font ()->Height () + 2) * m_nVisibleItems;

fontManager.Current ()->StringSize (pszTitle, w, h, aw);	
if (w > m_nWidth)
	m_nWidth = w;
m_nTitleHeight = h + 5;

nOffsetSize = CCanvas::Current ()->Font ()->Width ();

m_nWidth += (CCanvas::Current ()->Font ()->Width ());
if (m_nWidth > CCanvas::Current ()->Width () - (CCanvas::Current ()->Font ()->Width () * 3))
	m_nWidth = CCanvas::Current ()->Width () - (CCanvas::Current ()->Font ()->Width () * 3);

m_xOffset = (CCanvas::Current ()->Width () - m_nWidth) / 2;
m_yOffset = (CCanvas::Current ()->Height () - (m_nHeight + m_nTitleHeight))/2 + m_nTitleHeight;
if (m_yOffset < m_nTitleHeight)
	m_yOffset = m_nTitleHeight;

total_width = m_nWidth + 2*nOffsetSize;
total_height = m_nHeight + 2*nOffsetSize + m_nTitleHeight;

CCanvas::Push ();
backgroundManager.Setup (NULL, m_xOffset - nOffsetSize, m_yOffset - m_nTitleHeight - nOffsetSize, 
								 m_nWidth + nOffsetSize * 2, m_nHeight + m_nTitleHeight + nOffsetSize * 2);
CCanvas::Pop ();
done = 0;
m_nChoice = nDefaultItem;
if (m_nChoice < 0) 
	m_nChoice = 0;
if (m_nChoice >= int (items.ToS ())) 
	m_nChoice = 0;

m_nFirstItem = -1;

nMouseState = nOldMouseState = 0;	//bDblClick = 0;
close_x = m_xOffset - nOffsetSize;
close_y = m_yOffset - m_nTitleHeight - nOffsetSize;
CMenu::DrawCloseBox (close_x, close_y);
SDL_ShowCursor (1);

SDL_EnableKeyRepeat(60, 30);
while (!done) {
	m_nOldChoice = m_nChoice;
	nPrevItem = m_nFirstItem;
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
		m_bRedraw = (*callback) (&m_nChoice, items, &key);
	else
		m_bRedraw = 0;

	if (key< - 1) {
		m_nChoice = key;
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
		if (done) break;

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

			MouseGetPos (&mx, &my);
			for (i = m_nFirstItem; i < m_nFirstItem + m_nVisibleItems; i++) {
				if (i > int (items.ToS ()))
					break;
				fontManager.Current ()->StringSize (items [i], w, h, aw);
				x1 = m_xOffset;
				x2 = m_xOffset + m_nWidth;
				y1 = (i - m_nFirstItem)* (CCanvas::Current ()->Font ()->Height () + 2) + m_yOffset;
				y2 = y1 + h + 1;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					//if (i == m_nChoice) {
					//	break;
					//}
					//bDblClick = 0;
					m_nChoice = i;
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
				m_nChoice = -1;
				done = 1;
			}
		}
	Render (pszTitle);
	}
FadeOut ();
gameStates.input.keys.bRepeat = bKeyRepeat;
backgroundManager.Remove ();
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
i = lb.ListBox (pszTitle, filenames, 1, NULL);
if (i < 0) 
	return 0;
strcpy (filename, filenames [i]);
return 1;
}

//------------------------------------------------------------------------------
//eof
