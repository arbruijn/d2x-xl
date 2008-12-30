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

#if DBG

const char *menuBgNames [4][2] = {
 {"menu.pcx", "menub.pcx"},
 {"menuo.pcx", "menuob.pcx"},
 {"menud.pcx", "menud.pcx"},
 {"menub.pcx", "menub.pcx"}
	};

#else

const char *menuBgNames [4][2] = {
 {"\x01menu.pcx", "\x01menub.pcx"},
 {"\x01menuo.pcx", "\x01menuob.pcx"},
 {"\x01menud.pcx", "\x01menud.pcx"},
 {"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

//------------------------------------------------------------------------------ 

#define LHX(x) (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y) (gameStates.menus.bHires? (24 * (y)) / 10 : y)

char NORMAL_RADIO_BOX [2] = {(char) 127, 0};
char CHECKED_RADIO_BOX [2] = {(char) 128, 0};
char NORMAL_CHECK_BOX [2] = {(char) 129, 0};
char CHECKED_CHECK_BOX [2] = {(char) 130, 0};
char CURSOR_STRING [2] = {'_', 0};
char SLIDER_LEFT [2] = {(char) 131, 0};
char SLIDER_RIGHT [2] = {(char) 132, 0};
char SLIDER_MIDDLE [2] = {(char) 133, 0};
char SLIDER_MARKER [2] = {(char) 134, 0};
char UP_ARROW_MARKER [2] = {(char) 135, 0};
char DOWN_ARROW_MARKER [2] = {(char) 136, 0};

//------------------------------------------------------------------------------ 

#define MESSAGEBOX_TEXT_SIZE 10000		// How many characters in messagebox
#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

char bPauseableMenu = 0;
char bAlreadyShowingInfo = 0;

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 

void CMenuItem::Destroy (void)
{
for (int i = 0; i < 2; i++)
	if (m_bmText [i]) {
		delete m_bmText [i];
		m_bmText [i] = NULL;
		}
}

//------------------------------------------------------------------------------ 

short CMenuItem::SetColor (int bIsCurrent, int bTiny) 
{
if (bTiny) {
	if (!gameData.menu.bValid) {
		gameData.menu.warnColor = RED_RGBA;
		gameData.menu.keyColor = RGBA_PAL (57, 49, 20);
		gameData.menu.tabbedColor = WHITE_RGBA;
		gameData.menu.tinyColors [0][0] = RGBA_PAL (29, 29, 47);
		gameData.menu.tinyColors [0][1] = RED_RGBA;
		gameData.menu.tinyColors [1][0] = RGBA_PAL (57, 49, 20);
		gameData.menu.tinyColors [1][1] = ORANGE_RGBA;
		gameData.menu.bValid = 1;
		}
	if (gameData.menu.colorOverride)
		fontManager.SetColorRGBi (gameData.menu.colorOverride, 1, 0, 0);
	else if (m_text [0] == '\t')
		fontManager.SetColorRGBi (gameData.menu.tabbedColor, 1, 0, 0);
	else 
		fontManager.SetColorRGBi (gameData.menu.tinyColors [bIsCurrent][m_bUnavailable], 1, 0, 0);
	}
else {
	if (bIsCurrent)
		fontManager.SetCurrent (SELECTED_FONT);
	else
		fontManager.SetCurrent (NORMAL_FONT);
	}
return CCanvas::Current ()->FontColor (0).index;
}

//------------------------------------------------------------------------------ 

void CMenuItem::TrimWhitespace (void)
{
int i, l = (int) strlen (m_text);

for (i = l - 1; i >= 0; i--) {
	if (::isspace (m_text [i]))
		m_text [i] = 0;
	else
		return;
	}
}

//------------------------------------------------------------------------------ 

int nTabIndex = -1;
int nTabs [] = {15, 87, 124, 162, 228, 253};

void CMenuItem::DrawHotKeyString (int bIsCurrent, int bTiny, int bCreateTextBms, int nDepth)
{
	CBitmap	*bmP = m_bmText [bIsCurrent];

if (!*m_text)
	return;
if (m_color)
	fontManager.SetColorRGBi (m_color, 1, 0, 0);
else
	SetColor (bIsCurrent, bTiny);
if (bCreateTextBms && gameOpts->menus.bFastMenus && 
	 (bmP || (bmP = CreateStringBitmap (m_text, MENU_KEY (m_nKey, - 1), gameData.menu.keyColor, nTabs, m_bCentered, m_w, 0)))) {
	bmP->Render (CCanvas::Current (), m_x, m_y, bmP->Width (), bmP->Height (), 0, 0, bmP->Width (), bmP->Height (), 1, 0);
	m_bmText [bIsCurrent] = bmP;
	}
else {

		int	w, h, aw, l, i, 
				x = m_x, 
				y = m_y;
		char	*t, *ps = m_text, s [256], ch = 0, ch2;

	if ((t = strchr (ps, '\n'))) {
		strncpy (s, ps, sizeof (s));
		m_text = s;
		fontManager.Current ()->StringSize (s, w, h, aw);
		do {
			if ((t = strchr (m_text, '\n')))
				*t = '\0';
			DrawHotKeyString (0, bTiny, 0, nDepth + 1);
			if (!t)
				break;
			m_text = t + 1;
			m_y += h / 2;
			m_x = m_xSave;
			nTabIndex = -1;
			} while (*(m_text));
		}
	else if ((t = strchr (ps, '\t'))) {
		strncpy (s, ps, sizeof (s));
		m_text = s;
		fontManager.Current ()->StringSize (s, w, h, aw);
		do {
			if ((t = strchr (m_text, '\t')))
				*t = '\0';
			DrawHotKeyString (0, bTiny, 0, nDepth + 1);
			if (!t)
				break;
			m_text = t + 1;
			nTabIndex++;
		} while (*(m_text));
		nTabIndex = -1;
		m_text = ps;
		m_y = y;
		}
	else {
		l = (int) strlen (m_text);
		if (bIsCurrent || !m_nKey)
			i = l;
		else {
			ch = MENU_KEY (m_nKey, *ps);
			for (i = 0; ps [i]; i++)
				if (ps [i] == ch)
					break;
			}
		strncpy (s, ps, sizeof (s));
		s [i] = '\0';
		fontManager.Current ()->StringSize (s, w, h, aw);
		if (nTabIndex >= 0) {
			x += LHX (nTabs [nTabIndex]);
			if (!gameStates.multi.bSurfingNet)
				x += m_w - w;
			}
		//m_x = x + w;
		if (i) {
			GrString (x, y, s, NULL);
	#if DBG
			//GrUpdate (0);
	#endif
			}
		if (i < l) {	// print the hotkey
			x += w;
			s [i] = ch;
			ch2 = s [++i];
			s [i] = '\0';
			SetColor (1, bTiny);
			GrString (x, y, s + i - 1, NULL);
	#if DBG
			//GrUpdate (0);
	#endif
			SetColor (0, bTiny);
			if (i < l) { // print text following the hotkey
				fontManager.Current ()->StringSize (s + i - 1, w, h, aw);
				x += w;
				s [i] = ch2;
				GrString (x, y, s + i, NULL);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------ 
// Draw a left justfied string
void CMenuItem::DrawString (int bIsCurrent, int bTiny)
{
	int w1 = m_w, x = m_x, y = m_y;
	int l, w, h, aw, tx = 0, t = 0, i;
	char* p, *s1, measure [2] , *s = m_text;
	int XTabs [6];
	static char s2 [1024];

p = s1 = NULL;
l = (int) strlen (s);
memcpy (s2, s, l + 1);

for (i = 0; i < 6; i++)
	XTabs [i] = (LHX (nTabs [i])) + x;

measure [1] = 0;

if (!gameStates.multi.bSurfingNet) {
	p = strchr (s2, '\t');
	if (p && (w1>0)) {
		*p = '\0';
		s1 = p + 1;
		}
	}
if (w1 > 0)
	w = w1;
fontManager.Current ()->StringSize (s2, w, h, aw);
// CHANGED
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), 5, y - 1, backgroundManager.Current ()->Width () - 15, h + 2, 5, y - 1);
if (0 && gameStates.multi.bSurfingNet) {
	for (i = 0;i < l;i++) {
		if (s2 [i] == '\t' && gameStates.multi.bSurfingNet) {
			x = XTabs [t];
			t++;
			continue;
			}
		measure [0] = s2 [i];
		fontManager.Current ()->StringSize (measure, tx, h, aw);
		GrString (x, y, measure, NULL);
		x += tx;
		}
	}
else {
	DrawHotKeyString (bIsCurrent, bTiny, 1, 0);
	return;
	} 

if (!gameStates.multi.bSurfingNet && p && (w1 > 0)) {
	fontManager.Current ()->StringSize (s1, w, h, aw);
	GrString (x + w1 - w, y, s1, NULL);
	*p = '\t';
	}
}

//------------------------------------------------------------------------------ 
// Draw a slider and it's string
void CMenuItem::DrawSlider (int bIsCurrent, int bTiny)
{
char* s1 = NULL;
char* p = strchr (m_savedText, '\t');
if (p) {
	*p = '\0';
	s1 = p + 1;
	}

int w, h, aw;
fontManager.Current ()->StringSize (m_savedText, w, h, aw);

int y = m_y;
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), 5, y, backgroundManager.Current ()->Width () - 15, h, 5, y);
char* t = m_text;
m_text = m_savedText;
DrawHotKeyString (bIsCurrent, bTiny, 1, 0);
m_text = t;
if (p) {
	fontManager.Current ()->StringSize (s1, w, h, aw);
	int x = m_x + m_w - w;
	if (gameStates.ogl.nDrawBuffer != GL_BACK) {
		backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x, y, w, 1, x, y);
		backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x, y + h - 1, w, 1, x, y);
		}
	GrString (x, y, s1, NULL);
	*p = '\t';
	}
}

//------------------------------------------------------------------------------ 
// Draw a right justfied string
void CMenuItem::DrawRightString (int bIsCurrent, int bTiny, char* s)
{
	int	w, h, aw;
	int	w1 = m_rightOffset, 
			x = m_x, 
			y = m_y;
	char	*hs;

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
hs = m_text;
m_text = s;
h = m_x;
m_x = x - w;
DrawHotKeyString (bIsCurrent, bTiny, 0, 0);
m_text = hs;
m_x = h;
}

//------------------------------------------------------------------------------ 

void CMenuItem::DrawBlackBox (int w1, int x, int y, const char* s)
{
	int w, h, aw;

fontManager.Current ()->StringSize (s, w, h, aw);
if (w1 == 0) 
	w1 = w;

CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (2, 2, 2));
GrRect (x - 1, y - 1, x - 1, y + h - 1);
GrRect (x - 1, y - 1, x + w1 - 1, y - 1);
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
GrRect (x, y + h, x + w1, y + h);
GrRect (x + w1, y - 1, x + w1, y + h);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
GrRect (x, y, x + w1 - 1, y + h - 1);
GrString (x + 1, y + 1, s, NULL);
}

//------------------------------------------------------------------------------ 

void CMenuItem::DrawInputBox (int w, int x, int y, char* text, int current)
{
	int w1, h1, aw;

while (*text) {
	fontManager.Current ()->StringSize (text, w1, h1, aw);
	if (w1 > w - 10)
		text++;
	else
		break;
	}
if (!*text)
	w1 = 0;
DrawBlackBox (w, x, y, text);
if (current)	
	GrString (x + w1 + 1, y, CURSOR_STRING, NULL);
}

//------------------------------------------------------------------------------ 

void CMenuItem::DrawGauge (int w, int x, int y, int val, int maxVal, int current)
{
	int w1, h, aw;

fontManager.Current ()->StringSize (" ", w1, h, aw);
if (!w) 
	w = w1 * 30;
w1 = w * val / maxVal;
if (w1 < w) {
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
	GrURect (x + w1 + 1, y, x + w, y + h - 2);
	}
CCanvas::Current ()->SetColorRGB (200, 0, 0, 255);
if (w1)
	GrURect (x + 1, y, x + w1, y + h - 2);
GrUBox (x, y, x + w - 1, y + h - 1);
}

//------------------------------------------------------------------------------ 

void CMenuItem::ShowHelp (void)
{
if (m_szHelp && *m_szHelp) {
	int nInMenu = gameStates.menus.nInMenu;
	int bFastMenus = gameOpts->menus.bFastMenus;
	gameStates.menus.nInMenu = 0;
	//gameOpts->menus.bFastMenus = 0;
	gameData.menu.helpColor = RGBA_PAL (47, 47, 47);
	gameData.menu.colorOverride = gameData.menu.helpColor;
	MsgBox ("D2X - XL online help", NULL, - 3, m_szHelp, " ", TXT_CLOSE);
	gameData.menu.colorOverride = 0;
	gameOpts->menus.bFastMenus = bFastMenus;
	gameStates.menus.nInMenu = nInMenu;
	}
}

//------------------------------------------------------------------------------ 

#include "timer.h"

//for text itemP, constantly redraw cursor (to achieve flash)
void CMenuItem::UpdateCursor (void)
{
	int w, h, aw;
	fix time = TimerGetApproxSeconds ();
	int x, y;
	char* text = m_text;

	Assert (m_nType == NM_TYPE_INPUT_MENU || m_nType == NM_TYPE_INPUT);

while (*text) {
	fontManager.Current ()->StringSize (text, w, h, aw);
	if (w > m_w - 10)
		text++;
	else
		break;
	}
if (!*text) 
	w = 0;
x = m_x + w; 
y = m_y;
if (time & 0x8000)
	GrString (x, y, CURSOR_STRING, NULL);
else {
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
	GrRect (x, y, x + CCanvas::Current ()->Font ()->Width () - 1, y + CCanvas::Current ()->Font ()->Height () - 1);
	}
}

//------------------------------------------------------------------------------ 

void CMenuItem::Draw (int bIsCurrent, int bTiny)
{
SetColor (bIsCurrent, bTiny);
if (m_bRebuild) {
	Destroy ();
	m_bRebuild = 0;
	}
switch (m_nType) {
	case NM_TYPE_TEXT:
 // CCanvas::Current ()->Font () = TEXT_FONT);
		// fall through on purpose

	case NM_TYPE_MENU:
		DrawString (bIsCurrent, bTiny);
		break;

	case NM_TYPE_SLIDER: {
		int h, l;
		char* psz = m_savedText;

		if (m_value < m_minValue) 
			m_value = m_minValue;
		else if (m_value > m_maxValue) 
			m_value = m_maxValue;
		sprintf (psz, "%s\t%s", m_text, SLIDER_LEFT);
#if 1
		l = (int) strlen (psz);
		h = m_maxValue - m_minValue + 1;
		memset (psz + l, SLIDER_MIDDLE [0], h);
		psz [l + h] = SLIDER_RIGHT [0];
		psz [l + h + 1] = '\0';
#else
		for (j = 0; j < (m_maxValue - m_minValue + 1); j++) {
			sprintf (psz, "%s%s", psz, SLIDER_MIDDLE);
			}
		sprintf (m_savedText, "%s%s", m_savedText, SLIDER_RIGHT);
#endif
		psz [m_value + 1 + strlen (m_text) + 1] = SLIDER_MARKER [0];
		DrawSlider (bIsCurrent, bTiny);
		}
		break;

	case NM_TYPE_INPUT_MENU:
		if (m_group)
			DrawInputBox (m_w, m_x, m_y, m_text, bIsCurrent);
		else
			DrawString (bIsCurrent, bTiny);
		break;

	case NM_TYPE_INPUT:
		DrawInputBox (m_w, m_x, m_y, m_text, bIsCurrent);
		break;

	case NM_TYPE_GAUGE:
		DrawGauge (m_w, m_x, m_y, m_value, m_maxValue, bIsCurrent);
		break;

	case NM_TYPE_CHECK:
		DrawString (bIsCurrent, bTiny);
		if (m_value)
			DrawRightString (bIsCurrent, bTiny, CHECKED_CHECK_BOX);
		else														 
			DrawRightString (bIsCurrent, bTiny, NORMAL_CHECK_BOX);
		break;

	case NM_TYPE_RADIO:
		DrawString (bIsCurrent, bTiny);
		if (m_value)
			DrawRightString (bIsCurrent, bTiny, CHECKED_RADIO_BOX);
		else
			DrawRightString (bIsCurrent, bTiny, NORMAL_RADIO_BOX);
		break;

	case NM_TYPE_NUMBER:
		char text [20];
		m_value = NMCLAMP (m_value, m_minValue, m_maxValue);
		DrawString (bIsCurrent, bTiny);
		sprintf (text, "%d", m_value);
		DrawRightString (bIsCurrent, bTiny, text);
		break;
	}
}

//------------------------------------------------------------------------------ 

int CMenuItem::GetSize (int h, int aw, int& nStringWidth, int& nStringHeight, int& nAverageWidth, int& nMenus, int& nOthers)
{
	int	j;

if (!gameStates.app.bEnglish)
	m_nKey = *(m_text - 1);
if (m_nKey) {
	if (gameOpts->menus.nHotKeys < 0) {
		if ((m_nKey < KEY_1) || (m_nKey > KEY_0))
			m_nKey = -1;
		}
	else if (gameOpts->menus.nHotKeys == 0)
		m_nKey = 0;
	}
m_bRedraw = 1;
m_y = h;
fontManager.Current ()->StringSize (m_text, nStringWidth, nStringHeight, nAverageWidth);
m_rightOffset = 0;

if (gameStates.multi.bSurfingNet)
	nStringHeight += LHY (3);

m_savedText [0] = '\0';
if (m_nType == NM_TYPE_SLIDER) {
	int w1, h1, aw1;
	nOthers++;
	sprintf (m_savedText, "%s", SLIDER_LEFT);
	for (j = 0; j< (m_maxValue - m_minValue + 1); j++) {
		sprintf (m_savedText, "%s%s", m_savedText, SLIDER_MIDDLE);
		}
	sprintf (m_savedText, "%s%s", m_savedText, SLIDER_RIGHT);
	fontManager.Current ()->StringSize (m_savedText, w1, h1, aw1);
	nStringWidth += w1 + aw;
	}
else if (m_nType == NM_TYPE_MENU) {
	nMenus++;
	}
else if (m_nType == NM_TYPE_CHECK) {
	int w1, h1, aw1;
	nOthers++;
	fontManager.Current ()->StringSize (NORMAL_CHECK_BOX, w1, h1, aw1);
	m_rightOffset = w1;
	fontManager.Current ()->StringSize (CHECKED_CHECK_BOX, w1, h1, aw1);
	if (w1 > m_rightOffset)
		m_rightOffset = w1;
	}
else if (m_nType == NM_TYPE_RADIO) {
	int w1, h1, aw1;
	nOthers++;
	fontManager.Current ()->StringSize (NORMAL_RADIO_BOX, w1, h1, aw1);
	m_rightOffset = w1;
	fontManager.Current ()->StringSize (CHECKED_RADIO_BOX, w1, h1, aw1);
	if (w1 > m_rightOffset)
		m_rightOffset = w1;
	}
else if (m_nType == NM_TYPE_NUMBER) {
	int w1, h1, aw1;
	char test_text [20];
	nOthers++;
	sprintf (test_text, "%d", m_maxValue);
	fontManager.Current ()->StringSize (test_text, w1, h1, aw1);
	m_rightOffset = w1;
	sprintf (test_text, "%d", m_minValue);
	fontManager.Current ()->StringSize (test_text, w1, h1, aw1);
	if (w1 > m_rightOffset)
		m_rightOffset = w1;
	}
else if (m_nType == NM_TYPE_INPUT) {
	Assert (strlen (m_text) < NM_MAX_TEXT_LEN);
	strncpy (m_savedText, m_text, NM_MAX_TEXT_LEN);
	nOthers++;
	nStringWidth = m_nTextLen*CCanvas::Current ()->Font ()->Width () + ((gameStates.menus.bHires?3:1)*m_nTextLen);
	if (nStringWidth > MAX_TEXT_WIDTH) 
		nStringWidth = MAX_TEXT_WIDTH;
	m_value = -1;
	}
else if (m_nType == NM_TYPE_INPUT_MENU) {
	Assert (strlen (m_text) < NM_MAX_TEXT_LEN);
	strncpy (m_savedText, m_text, NM_MAX_TEXT_LEN);
	nMenus++;
	nStringWidth = m_nTextLen*CCanvas::Current ()->Font ()->Width () + ((gameStates.menus.bHires?3:1)*m_nTextLen);
	m_value = -1;
	m_group = 0;
	}
m_w = nStringWidth;
m_h = nStringHeight;
return nStringHeight;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
// Draw a left justfied string with black background.
void CMenu::DrawRightStringWXY (int w1, int x, int y, const char* s)
{
	int	w, h, aw;

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (gameStates.ogl.nDrawBuffer != GL_BACK)
	backgroundManager.Current ()->RenderClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
GrString (x - w, y, s, NULL);
}

//------------------------------------------------------------------------------ 

char* nmAllowedChars = NULL;

//returns true if char is bAllowed
int CMenu::CharAllowed (char c)
{
	char* p = nmAllowedChars;

if (!p)
	return 1;

while (*p) {
	Assert (p [1]);
	if (c>= p [0] && c<= p [1])
		return 1;
	p += 2;
	}
return 0;
}

//------------------------------------------------------------------------------ 

int CMenu::TinyMenu (const char* pszTitle, const char* pszSubTitle, pMenuCallback callback)
{
return Menu (pszTitle, pszSubTitle, callback, NULL, NULL, - 1, - 1, 1);
}

//------------------------------------------------------------------------------ 

int CMenu::FixedFontMenu (const char* pszTitle, const char* pszSubTitle, 
											pMenuCallback callback, int* nCurItemP, char* filename, int width, int height)
{
SetScreenMode (SCREEN_MENU);
return Menu (pszTitle, pszSubTitle, callback, nCurItemP, filename, width, height, 0);
}

//------------------------------------------------------------------------------ 

//returns 1 if a control device button has been pressed
int NMCheckButtonPress ()
{
	int i;

	switch (gameConfig.nControlType) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
	case	CONTROL_GRAVIS_GAMEPAD:
		for (i = 0; i < 4; i++)
	 		if (JoyGetButtonDownCnt (i) > 0) 
				return 1;
		break;
	case	CONTROL_MOUSE:
	case	CONTROL_CYBERMAN:
	case	CONTROL_WINJOYSTICK:
		break;
	case	CONTROL_NONE:		//keyboard only
		#ifdef APPLE_DEMO
			if (KeyCheckChar ())	return 1;		
		#endif

		break;
	default:
		Error ("Bad control nType (gameConfig.nControlType):%i", gameConfig.nControlType);
	}

	return 0;
}

//------------------------------------------------------------------------------ 

extern int bRestoringMenu;

ubyte bHackDblClickMenuMode = 0;

# define JOYDEFS_CALIBRATING 0

#define CLOSE_X (gameStates.menus.bHires?15:7)
#define CLOSE_Y (gameStates.menus.bHires?15:7)
#define CLOSE_SIZE (gameStates.menus.bHires?10:5)

void CMenu::DrawCloseBox (int x, int y)
{
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
GrRect (x + CLOSE_X, y + CLOSE_Y, x + CLOSE_X + CLOSE_SIZE, y + CLOSE_Y + CLOSE_SIZE);
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (21, 21, 21));
GrRect (x + CLOSE_X + LHX (1), y + CLOSE_Y + LHX (1), x + CLOSE_X + CLOSE_SIZE - LHX (1), y + CLOSE_Y + CLOSE_SIZE - LHX (1));
}

//------------------------------------------------------------------------------ 

int CMenu::DrawTitle (const char* pszTitle, CFont* font, uint color, int ty)
{
if (pszTitle && *pszTitle) {
		int w, h, aw;

	fontManager.SetCurrent (font);
	fontManager.SetColorRGBi (color, 1, 0, 0);
	fontManager.Current ()->StringSize (pszTitle, w, h, aw);
	GrPrintF (NULL, 0x8000, ty, pszTitle);
	ty += h;
	}
return ty;
}

//------------------------------------------------------------------------------ 

void CMenu::GetTitleSize (const char* pszTitle, CFont* font, int& tw, int& th)
{
if (pszTitle && *pszTitle) {
		int nStringWidth, nStringHeight, nAverageWidth;

	fontManager.SetCurrent (font);
	fontManager.Current ()->StringSize (pszTitle, nStringWidth, nStringHeight, nAverageWidth);
	if (nStringWidth > tw)
		tw = nStringWidth;
	th += nStringHeight;
	}
}

//------------------------------------------------------------------------------ 

int CMenu::GetSize (int& w, int& h, int& aw, int& nMenus, int& nOthers)
{
	int	nStringWidth = 0, nStringHeight = 0, nAverageWidth = 0;

#if 0
if ((gameOpts->menus.nHotKeys > 0) && !gameStates.app.bEnglish)
	gameOpts->menus.nHotKeys = -1;
#endif
for (uint i = 0; i < int (ToS ()); i++) {
	Item (i).GetSize (h, aw, nStringWidth, nStringHeight, nAverageWidth, nMenus, nOthers);
	if (w < nStringWidth)
		w = nStringWidth;		// Save maximum width
	if (aw < nAverageWidth)
		aw = nAverageWidth;
	h += nStringHeight + 1;		// Find the height of all strings
	}
return nStringHeight;
}

//------------------------------------------------------------------------------ 

void CMenu::SetItemPos (int twidth, int xOffs, int yOffs, int m_rightOffset)
{
for (uint i = 0; i < int (ToS ()); i++) {
	if (gameOpts->menus.nStyle && ((Item (i).m_x == (short) 0x8000) || Item (i).m_bCentered)) {
		Item (i).m_bCentered = 1;
		Item (i).m_x = fontManager.Current ()->GetCenteredX (Item (i).m_text);
		}
	else
		Item (i).m_x = xOffs + twidth + m_rightOffset;
	Item (i).m_y += yOffs;
	Item (i).m_xSave = Item (i).m_x;
	Item (i).m_ySave = Item (i).m_y;
	if (Item (i).m_nType == NM_TYPE_RADIO) {
		int fm = -1;	// ffs first marked one
		for (uint j = 0; j < int (ToS ()); j++) {
			if ((Item (j).m_nType == NM_TYPE_RADIO) && (Item (j).m_group == Item (i).m_group)) {
				if ((fm == -1) && Item (j).m_value)
					fm = j;
				Item (j).m_value = 0;
				}
			}
		if ( fm >= 0)
			Item (fm).m_value = 1;
		else
			Item (i).m_value = 1;
		}
	}
}

//------------------------------------------------------------------------------ 

int CMenu::InitProps (const char* pszTitle, const char* pszSubTitle)
{
if ((m_props.scWidth == screen.Width ()) && (m_props.scHeight == screen.Height ()))
	return 0;

	int	i, gap, haveTitle;

m_props.scWidth = screen.Width ();
m_props.scHeight = screen.Height ();
m_props.nDisplayMode = gameStates.video.nDisplayMode;
GetTitleSize (pszTitle, TITLE_FONT, m_props.tw, m_props.th);
GetTitleSize (pszSubTitle, SUBTITLE_FONT, m_props.tw, m_props.th);

haveTitle = ((pszTitle && *pszTitle) || (pszSubTitle && *pszSubTitle));
gap = haveTitle ? (int) LHY (8) : 0;
m_props.th += gap;		//put some space between pszTitles & body
fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);

m_props.h = m_props.th;
m_props.nMenus = m_props.nOthers = 0;
m_props.nStringHeight = GetSize (m_props.w, m_props.h, m_props.aw, m_props.nMenus, m_props.nOthers);
m_props.nMaxOnMenu = (((gameOpts->menus.nStyle && (m_props.scHeight > 480)) ? m_props.scHeight * 4 / 5 : 480) - m_props.th - LHY (8)) / m_props.nStringHeight - 2;
if (/*!m_props.bTinyMode && */ (m_props.h > (m_props.nMaxOnMenu * (m_props.nStringHeight + 1)) + gap)) {
 m_props.bIsScrollBox = 1;
 m_props.h = (m_props.nMaxOnMenu * (m_props.nStringHeight + haveTitle) + haveTitle * LHY (m_props.bTinyMode ? 12 : 8));
 }
else
	m_props.bIsScrollBox = 0;
m_props.nMaxDisplayable = (m_props.nMaxOnMenu < int (ToS ())) ? m_props.nMaxOnMenu : int (ToS ());
m_props.rightOffset = 0;
if (m_props.width > - 1)
	m_props.w = m_props.width;
if (m_props.height > - 1)
	m_props.h = m_props.height;

for (i = 0; i < int (ToS ()); i++) {
	Item (i).m_w = m_props.w;
	if (Item (i).m_rightOffset > m_props.rightOffset)
		m_props.rightOffset = Item (i).m_rightOffset;
	if (Item (i).m_bNoScroll && (i == m_props.nMaxNoScroll))
		m_props.nMaxNoScroll++;
	}
m_props.nScrollOffset = m_props.nMaxNoScroll;
if (m_props.rightOffset > 0)
	m_props.rightOffset += 3;

m_props.w += m_props.rightOffset;
m_props.twidth = 0;
if (m_props.tw > m_props.w) {
	m_props.twidth = (m_props.tw - m_props.w) / 2;
	m_props.w = m_props.tw;
}

if (bRestoringMenu) { 
	m_props.rightOffset = 0; 
	m_props.twidth = 0;
	}

if (m_props.w > m_props.scWidth)
	m_props.w = m_props.scWidth;
if (m_props.h > m_props.scHeight)
	m_props.h = m_props.scHeight;

m_props.xOffs = (gameStates.menus.bHires ? 30 : 15);
i = (m_props.scWidth - m_props.w) / 2;
if (i < m_props.xOffs)
	m_props.xOffs = i / 2;
m_props.yOffs = (gameStates.menus.bHires ? 30 : 15);
if (m_props.scHeight - m_props.h < 2 * m_props.yOffs)
	m_props.h = m_props.scHeight - 2 * m_props.yOffs;
m_props.w += 2 * m_props.xOffs;
m_props.h += 2 * m_props.yOffs;
m_props.x = (m_props.scWidth - m_props.w) / 2;
m_props.y = (m_props.scHeight - m_props.h) / 2;
if (gameOpts->menus.nStyle) {
	m_props.xOffs += m_props.x;
	m_props.yOffs += m_props.y;
	}
m_props.bValid = 1;
SetItemPos (m_props.twidth, m_props.xOffs, m_props.yOffs, m_props.rightOffset);
return 1;
}

//------------------------------------------------------------------------------ 

void CMenu::SaveScreen (CCanvas **gameCanvasP)
{
*gameCanvasP = CCanvas::Current ();
CCanvas::Push ();
CCanvas::SetCurrent (NULL);
}

//------------------------------------------------------------------------------ 

void CMenu::RestoreScreen (char* filename, int bDontRestore)
{
//CCanvas::SetCurrent (bg->menu_canvas);
backgroundManager.Remove ();
if (gameStates.app.bGameRunning)
	backgroundManager.Remove ();
else
	GrUpdate (0);
CCanvas::SetCurrent (NULL);		
CCanvas::Pop ();
GrabMouse (1, 0);
}

//------------------------------------------------------------------------------ 

void CMenu::FreeTextBms (void)
{
for (int i = 0; i < int (ToS ()); i++)
	Item (i).Destroy ();
}

//------------------------------------------------------------------------------ 

#define REDRAW_ALL	for (i = 0; i < int (ToS ()); i++) Item (i).m_bRedraw = 1; bRedrawAll = 1

int CMenu::Menu (const char* pszTitle, const char* pszSubTitle, 
								pMenuCallback callback, int* nCurItemP, char* filename, int width, int height, int bTinyMode)
{
	int			bKeyRepeat, done, nItem = nCurItemP ? *nCurItemP : 0;
	int			choice, oldChoice, i;
	int			k, nLastScrollCheck = -1, sx, sy;
	int			bAllText = 0;		//set true if all text itemP
	int			sound_stopped = 0, time_stopped = 0;
	int			topChoice;// Is this a scrolling box? Set to 0 at init
	int			bDontRestore = 0;
	int			bRedraw = 0, bRedrawAll = 0, bStart = 1;
	CCanvas*		gameCanvasP;
	int			bWheelUp, bWheelDown, nMouseState, nOldMouseState, bDblClick = 0;
	int			mx = 0, my = 0, x1 = 0, x2, y1, y2;
	int			bLaunchOption = 0;
	int			bCloseBox = 0;

if (gameStates.menus.nInMenu)
	return - 1;
memset (&m_props, 0, sizeof (m_props));
m_props.width = width;
m_props.height = height;
m_props.bTinyMode = bTinyMode;
FlushInput ();

if (int (ToS ()) < 1)
	return - 1;
if (gameStates.app.bGameRunning)
	backgroundManager.LoadStars ();
SDL_ShowCursor (0);
SDL_EnableKeyRepeat(60, 30);
gameStates.menus.nInMenu++;
if (!gameOpts->menus.nStyle && (gameStates.app.nFunctionMode == FMODE_GAME) && !(gameData.app.nGameMode & GM_MULTI)) {
	audio.PauseSounds ();
	sound_stopped = 1;
	}

if (!(gameOpts->menus.nStyle || (IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME) && (!gameStates.app.bEndLevelSequence)))) {
	time_stopped = 1;
	StopTime ();
	#ifdef TACTILE 
	 if (TactileStick)
		 DisableForces ();
	#endif
	}

if (gameStates.app.bGameRunning && IsMultiGame)
	gameData.multigame.nTypingTimeout = 0;

SetPopupScreenMode ();
if (!gameOpts->menus.nStyle) {
	OglSetDrawBuffer (GL_FRONT, 1);
	if (gameStates.menus.bNoBackground || gameStates.app.bGameRunning) {
		//NMLoadBackground (NULL, NULL, 0);
		GrUpdate (0);
		}
	}
SaveScreen (&gameCanvasP);
bKeyRepeat = gameStates.input.keys.bRepeat;
gameStates.input.keys.bRepeat = 1;
if (nItem == -1)
	choice = -1;
else {
	if (nItem < 0) 
		nItem = 0;
	else 
		nItem %= int (ToS ());
	choice = nItem;
	bDblClick = 1;
	while (Item (choice).m_nType == NM_TYPE_TEXT) {
		choice++;
		if (choice >= int (ToS ()))
			choice = 0; 
		if (choice == nItem) {
			choice = 0; 
			bAllText = 1;
			break; 
			}
		}
	} 

done = 0;
topChoice = 0;
while (Item (topChoice).m_nType == NM_TYPE_TEXT) {
	topChoice++;
	if (topChoice >= int (ToS ()))
		topChoice = 0; 
	if (topChoice == nItem) {
		topChoice = 0; 
		break; 
		}
	}

//GrUpdate (0);
// Clear mouse, joystick to clear button presses.
GameFlushInputs ();
nMouseState = nOldMouseState = 0;
bCloseBox = !(filename || gameStates.menus.bReordering);

if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING) {
	SDL_ShowCursor (1);
	}
GrabMouse (0, 0);
while (!done) {
	if (nCurItemP)
		*nCurItemP = choice;
	if (gameStates.app.bGameRunning && IsMultiGame) {
		gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer] = 1;
		MultiSendTyping ();
		}
	if (!JOYDEFS_CALIBRATING)
		SDL_ShowCursor (1); // possibly hidden
	nOldMouseState = nMouseState;
	if (!gameStates.menus.bReordering) {
		int b = gameOpts->legacy.bInput;
		gameOpts->legacy.bInput = 1;
		nMouseState = MouseButtonState (0);
		gameOpts->legacy.bInput = b;
		}
	bWheelUp = MouseButtonState (3);
	bWheelDown = MouseButtonState (4);
	//see if redbook song needs to be restarted
	redbook.CheckRepeat ();
	//NetworkListen ();
	if (bWheelUp)
		k = KEY_UP;
	else if (bWheelDown)
		k = KEY_DOWN;
	else
		k = KeyInKey ();
	if (mouseData.bDoubleClick)
		k = KEY_ENTER;
	if ((m_props.scWidth != screen.Width ()) || (m_props.scHeight != screen.Height ())) {
		memset (&m_props, 0, sizeof (m_props));
		m_props.width = width;
		m_props.height = height;
		m_props.bTinyMode = bTinyMode;
		}
#if 1
	if (m_props.bValid && (m_props.nDisplayMode != gameStates.video.nDisplayMode)) {
		FreeTextBms ();
		backgroundManager.Remove (); //bDontRestore?
		SetScreenMode (SCREEN_MENU);
		SaveScreen (&gameCanvasP);
//		RemapFontsAndMenus (1);
		memset (&m_props, 0, sizeof (m_props));
		m_props.width = width;
		m_props.height = height;
		m_props.bTinyMode = bTinyMode;
		}
#endif
	if (InitProps (pszTitle, pszSubTitle)) {
		backgroundManager.Setup (filename, m_props.x, m_props.y, m_props.w, m_props.h);
		if (!gameOpts->menus.nStyle)
			CCanvas::SetCurrent (backgroundManager.Canvas ());
		bRedraw = 0;
		m_props.ty = m_props.yOffs;
		if (choice > m_props.nScrollOffset + m_props.nMaxOnMenu - 1)
			m_props.nScrollOffset = choice - m_props.nMaxOnMenu + 1;
		}
	if (!gameOpts->menus.nStyle) {
		if (callback)
			(*callback) (*this, k, choice);
		}
	else {
		if (gameStates.app.bGameRunning) {
			CCanvas::Push ();
			CCanvas::SetCurrent (gameCanvasP);
			//GrPaletteStepLoad (paletteManager.Game ());
			//GrCopyPalette (grPalette, paletteManager.Game (), sizeof (grPalette));
			if (!gameStates.app.bShowError) {
				if (gameData.app.bGamePaused /*|| timer_paused*/) {
					GameRenderFrame ();
					gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
					G3_SLEEP (0);
					}
				else {
					GameLoop (1, 0);
					}
				}
			CCanvas::Pop ();
			}
		}

	if (!bRedraw || gameOpts->menus.nStyle) {
		int t;
		backgroundManager.Draw ();
		if (!gameStates.app.bGameRunning)
			console.Draw ();
		if (callback)
 			choice = (*callback) (*this, k, choice);
		t = DrawTitle (pszTitle, TITLE_FONT, RGBA_PAL (31, 31, 31), m_props.yOffs);
		DrawTitle (pszSubTitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
		if (!bRedraw)
			m_props.ty = t;
		fontManager.SetCurrent (bTinyMode ? SMALL_FONT : NORMAL_FONT);
		}
	if (!time_stopped){
		// Save current menu box
		if (MultiMenuPoll () == -1)
			k = -2;
		}

	if (k < - 1) {
		bDontRestore = (k == -3);		// - 3 means don't restore
		if (choice >= 0)
			choice = k;
		else {
			choice = -choice - 1;
			*nCurItemP = choice;
			}
		k = -1;
		done = 1;
		}
	oldChoice = choice;
	if (k && (console.Events (k) || bWheelUp || bWheelDown))
		switch (k) {

		case KEY_SHIFTED + KEY_ESC:
			console.Show ();
			k = -1;
			break;

		case KEY_I:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 ShowNetGameInfo (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done = 1;
			 choice = -1;
			}
		 break;
		 
		case KEY_U:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 NetworkRequestPlayerNames (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done = 1;
			 choice = - 1;
			}
		 break;

		case KEY_CTRLED + KEY_F1:
			SwitchDisplayMode ( - 1);
			break;

		case KEY_CTRLED + KEY_F2:
			SwitchDisplayMode (1);
			break;

		case KEY_COMMAND + KEY_T:
		case KEY_CTRLED + KEY_T:
			gameData.menu.alpha = (gameData.menu.alpha + 16) & 0xFF;
			break;

		case KEY_COMMAND + KEY_P:
		case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
			if (bPauseableMenu) {
				bPauseableMenu = 0;
				done = 1;
				choice = - 1;
				}
			else
				gameData.app.bGamePaused = !gameData.app.bGamePaused;
			break;

		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (bAllText) 
				break;
			do {
				choice--;
 				if (m_props.bIsScrollBox) {
					nLastScrollCheck = - 1;
		 			if (choice < topChoice) { 
						choice = int (ToS ()) - 1; 
						m_props.nScrollOffset = int (ToS ()) - m_props.nMaxDisplayable + m_props.nMaxNoScroll;
						if (m_props.nScrollOffset < m_props.nMaxNoScroll)
							m_props.nScrollOffset = m_props.nMaxNoScroll;
						REDRAW_ALL;
						break; 
						}
					else if (choice < m_props.nScrollOffset) {
						REDRAW_ALL;
						m_props.nScrollOffset--;
						if (m_props.nScrollOffset < m_props.nMaxNoScroll)
							m_props.nScrollOffset = m_props.nMaxNoScroll;
						}
					}
				else {
					if (choice >= int (ToS ())) 
						choice = 0;
					else if (choice < 0) 
						choice = int (ToS ()) - 1;
					}
				} while (Item (choice).m_nType == NM_TYPE_TEXT);
			if ((Item (choice).m_nType == NM_TYPE_INPUT) && (choice != oldChoice))
				Item (choice).m_value = -1;
			if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != choice)) {
				Item (oldChoice).m_group = 0;
				strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
				Item (oldChoice).m_value = -1;
				}
			if (oldChoice> - 1) 
				Item (oldChoice).m_bRedraw = 1;
			Item (choice).m_bRedraw = 1;
			break;

			case KEY_TAB:
			case KEY_DOWN:
			case KEY_PAD2:
		// ((0, "Pressing down! m_props.bIsScrollBox = %d", m_props.bIsScrollBox);
			if (bAllText) 
				break;
			do {
				choice++;
				if (m_props.bIsScrollBox) {
					nLastScrollCheck = -1;
						if (choice == int (ToS ())) {
							choice = 0;
							if (m_props.nScrollOffset) {
								REDRAW_ALL;
								m_props.nScrollOffset = m_props.nMaxNoScroll;
								}
							}
						if (choice >= m_props.nMaxOnMenu + m_props.nScrollOffset - m_props.nMaxNoScroll) {
							REDRAW_ALL;
							m_props.nScrollOffset++;
							}
						}
					else {
						if (choice < 0) 
							choice = int (ToS ()) - 1;
						else if (choice >= int (ToS ())) 
							choice = 0;
						}
					} while (Item (choice).m_nType == NM_TYPE_TEXT);
 
			if ((Item (choice).m_nType == NM_TYPE_INPUT) && (choice != oldChoice))
				Item (choice).m_value = -1;
			if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != choice)) {
				Item (oldChoice).m_group = 0;
				strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
				Item (oldChoice).m_value = -1;
			}
			if (oldChoice> - 1)
				Item (oldChoice).m_bRedraw = 1;
			Item (choice).m_bRedraw = 1;
			break;

		case KEY_SPACEBAR:
checkOption:
radioOption:
			if (choice > - 1) {
				switch (Item (choice).m_nType) {
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if (Item (choice).m_value)
						Item (choice).m_value = 0;
					else
						Item (choice).m_value = 1;
					if (m_props.bIsScrollBox) {
						if (choice == (m_props.nMaxOnMenu + m_props.nScrollOffset - 1) || choice == m_props.nScrollOffset)
							nLastScrollCheck = - 1;				
						 }
			
					Item (choice).m_bRedraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				case NM_TYPE_RADIO:
					for (i = 0; i < int (ToS ()); i++) {
						if ((i != choice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (choice).m_group) && (Item (i).m_value)) {
							Item (i).m_value = 0;
							Item (i).m_bRedraw = 1;
						}
					}
					Item (choice).m_value = 1;
					Item (choice).m_bRedraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				}
			}
			break;

		case KEY_SHIFTED + KEY_UP:
			if (gameStates.menus.bReordering && choice != topChoice) {
				::Swap (Item (choice).m_text, Item (choice - 1).m_text);
				::Swap (Item (choice).m_value, Item (choice - 1).m_value);
				Item (choice).m_bRebuild = 
				Item (choice - 1).m_bRebuild = 1;
				choice--;
				}
			 break;

		case KEY_SHIFTED + KEY_DOWN:
			if (gameStates.menus.bReordering && choice !=(int (ToS ()) - 1)) {
				::Swap (Item (choice).m_text, Item (choice + 1).m_text);
				::Swap (Item (choice).m_value, Item (choice + 1).m_value);
				Item (choice).m_bRebuild = 
				Item (choice + 1).m_bRebuild = 1;
				choice++;
				}
			 break;

		case KEY_ALTED + KEY_ENTER: {
			//int bLoadCustomBg = NMFreeCustomBg ();
			FreeTextBms ();
			backgroundManager.Restore ();
			//NMRestoreScreen (filename, & bDontRestore);
			GrToggleFullScreenGame ();
			GrabMouse (0, 0);
			SetScreenMode (SCREEN_MENU);
			SaveScreen (&gameCanvasP);
			RemapFontsAndMenus (1);
			memset (&m_props, 0, sizeof (m_props));
			m_props.width = width;
			m_props.height = height;
			m_props.bTinyMode = bTinyMode;
			continue;
			}

		case KEY_ENTER:
		case KEY_PADENTER:
launchOption:
			if ((choice > - 1) && (Item (choice).m_nType == NM_TYPE_INPUT_MENU) && (Item (choice).m_group == 0)) {
				Item (choice).m_group = 1;
				Item (choice).m_bRedraw = 1;
				if (!strnicmp (Item (choice).m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
					Item (choice).m_text [0] = 0;
					Item (choice).m_value = -1;
					}
				else {
					Item (choice).TrimWhitespace ();
					}
				}
			else {
				if (nCurItemP)
					*nCurItemP = choice;
				done = 1;
				}
			break;

		case KEY_ESC:
			if ((choice > - 1) && (Item (choice).m_nType == NM_TYPE_INPUT_MENU) && (Item (choice).m_group == 1)) {
				Item (choice).m_group = 0;
				strcpy (Item (choice).m_text, Item (choice).m_savedText);
				Item (choice).m_bRedraw = 1;
				Item (choice).m_value = -1;
				}
			else {
				done = 1;
				if (nCurItemP)
					*nCurItemP = choice;
				choice = -1;
			}
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_ALTED + KEY_F9:
			gameStates.app.bSaveScreenshot = 1;
			SaveScreenShot (NULL, 0);
			for (i = 0;i < int (ToS ());i++)
				Item (i).m_bRedraw = 1;
		
			break;

		#if DBG
		case KEY_BACKSP:
			if ((choice > - 1) && (Item (choice).m_nType != NM_TYPE_INPUT) && (Item (choice).m_nType != NM_TYPE_INPUT_MENU))
				Int3 (); 
			break;
		#endif

		case KEY_F1:
			Item (choice).ShowHelp ();
			break;
		}

		if (!done && nMouseState && !nOldMouseState && !bAllText) {
			MouseGetPos (&mx, &my);
			for (i = 0; i < int (ToS ()); i++) {
				x1 = CCanvas::Current ()->Left () + Item (i).m_x - Item (i).m_rightOffset - 6;
				x2 = x1 + Item (i).m_w;
				y1 = CCanvas::Current ()->Top () + Item (i).m_y;
				y2 = y1 + Item (i).m_h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					if (i + m_props.nScrollOffset - m_props.nMaxNoScroll != choice) {
						if (bHackDblClickMenuMode) 
							bDblClick = 0; 
					}
				
					choice = i + m_props.nScrollOffset - m_props.nMaxNoScroll;
					if (choice >= 0 && choice < int (ToS ()))
						switch (Item (choice).m_nType) {
							case NM_TYPE_CHECK:
								Item (choice).m_value = !Item (choice).m_value;
								Item (choice).m_bRedraw = 1;
								if (m_props.bIsScrollBox)
									nLastScrollCheck = - 1;
								break;
							case NM_TYPE_RADIO:
								for (i = 0; i < int (ToS ()); i++) {
									if ((i!= choice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (choice).m_group) && (Item (i).m_value)) {
										Item (i).m_value = 0;
										Item (i).m_bRedraw = 1;
									}
								}
								Item (choice).m_value = 1;
								Item (choice).m_bRedraw = 1;
								break;
							}
					Item (oldChoice).m_bRedraw = 1;
					break;
				}
			}
		}

		if (nMouseState && bAllText) {
			if (nCurItemP)
				*nCurItemP = choice;
			done = 1;
			}
	
		if (!done && nMouseState && !bAllText) {
			MouseGetPos (&mx, &my);
		
			// check possible scrollbar stuff first
			if (m_props.bIsScrollBox) {
				int i, arrowWidth, arrowHeight, aw;
			
				if (m_props.nScrollOffset > m_props.nMaxNoScroll) {
					fontManager.Current ()->StringSize (UP_ARROW_MARKER, arrowWidth, arrowHeight, aw);
					x2 = CCanvas::Current ()->Left () + Item (m_props.nScrollOffset).m_x - (gameStates.menus.bHires ? 24 : 12);
					 y1 = CCanvas::Current ()->Top () + Item (m_props.nScrollOffset).m_y - ((m_props.nStringHeight + 1)*(m_props.nScrollOffset - m_props.nMaxNoScroll));
					x1 = x2 - arrowWidth;
					y2 = y1 + arrowHeight;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
						choice--;
						nLastScrollCheck = - 1;
						if (choice < m_props.nScrollOffset) {
							REDRAW_ALL;
							m_props.nScrollOffset--;
							}
						}
					}
				if ((i = m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll) < int (ToS ())) {
					fontManager.Current ()->StringSize (DOWN_ARROW_MARKER, arrowWidth, arrowHeight, aw);
					x2 = CCanvas::Current ()->Left () + Item (i - 1).m_x - (gameStates.menus.bHires?24:12);
					y1 = CCanvas::Current ()->Top () + Item (i - 1).m_y - ((m_props.nStringHeight + 1)*(m_props.nScrollOffset - m_props.nMaxNoScroll));
					x1 = x2 - arrowWidth;
					y2 = y1 + arrowHeight;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
						choice++;
						nLastScrollCheck = - 1;
						if (choice >= m_props.nMaxOnMenu + m_props.nScrollOffset) {
							REDRAW_ALL;
							m_props.nScrollOffset++;
							}
						}
					}
				}
		
			for (i = m_props.nScrollOffset; i < int (ToS ()); i++) {
				x1 = CCanvas::Current ()->Left () + Item (i).m_x - Item (i).m_rightOffset - 6;
				x2 = x1 + Item (i).m_w;
				y1 = CCanvas::Current ()->Top () + Item (i).m_y;
				y1 -= ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
				y2 = y1 + Item (i).m_h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (Item (i).m_nType != NM_TYPE_TEXT)) {
					if (i /* + m_props.nScrollOffset - m_props.nMaxNoScroll*/ != choice) {
						if (bHackDblClickMenuMode) 
							bDblClick = 0; 
					}

					choice = i/* + m_props.nScrollOffset - m_props.nMaxNoScroll*/;

					if (Item (choice).m_nType == NM_TYPE_SLIDER) {
						char slider_text [NM_MAX_TEXT_LEN + 1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
					
						strcpy (slider_text, Item (choice).m_savedText);
						p = strchr (slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p + 1;
						}
						if (p) {
							fontManager.Current ()->StringSize (s1, slider_width, height, aw);
							fontManager.Current ()->StringSize (SLIDER_LEFT, sleft_width, height, aw);
							fontManager.Current ()->StringSize (SLIDER_RIGHT, sright_width, height, aw);
							fontManager.Current ()->StringSize (SLIDER_MIDDLE, smiddle_width, height, aw);

							x1 = CCanvas::Current ()->Left () + Item (choice).m_x + Item (choice).m_w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ((mx > x1) && (mx < (x1 + sleft_width)) && (Item (choice).m_value != Item (choice).m_minValue)) {
								Item (choice).m_value = Item (choice).m_minValue;
								Item (choice).m_bRedraw = 2;
							} else if ((mx < x2) && (mx > (x2 - sright_width)) && (Item (choice).m_value != Item (choice).m_maxValue)) {
								Item (choice).m_value = Item (choice).m_maxValue;
								Item (choice).m_bRedraw = 2;
							} else if ((mx > (x1 + sleft_width)) && (mx < (x2 - sright_width))) {
								int numValues, value_width, newValue;
							
								numValues = Item (choice).m_maxValue - Item (choice).m_minValue + 1;
								value_width = (slider_width - sleft_width - sright_width) / numValues;
								newValue = (mx - x1 - sleft_width) / value_width;
								if (Item (choice).m_value != newValue) {
									Item (choice).m_value = newValue;
									Item (choice).m_bRedraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == oldChoice)
						break;
					if ((Item (choice).m_nType == NM_TYPE_INPUT) && (choice != oldChoice))
						Item (choice).m_value = -1;
					if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != choice)) {
						Item (oldChoice).m_group = 0;
						strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
						Item (oldChoice).m_value = -1;
					}
					if (oldChoice> - 1) 
						Item (oldChoice).m_bRedraw = 1;
					Item (choice).m_bRedraw = 1;
					break;
				}
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && !bAllText && (choice != -1) && (Item (choice).m_nType == NM_TYPE_MENU)) {
			MouseGetPos (&mx, &my);
			x1 = CCanvas::Current ()->Left () + Item (choice).m_x;
			x2 = x1 + Item (choice).m_w;
			y1 = CCanvas::Current ()->Top () + Item (choice).m_y;
			if (choice >= m_props.nScrollOffset)
				y1 -= ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
			y2 = y1 + Item (choice).m_h;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (bHackDblClickMenuMode) {
					if (bDblClick) {
						if (nCurItemP)
							*nCurItemP = choice;
						done = 1;
						}
					else 
						bDblClick = 1;
				}
				else {
					done = 1;
					if (nCurItemP)
						*nCurItemP = choice;
				}
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && (choice > - 1) && 
			 (Item (choice).m_nType == NM_TYPE_INPUT_MENU) && (Item (choice).m_group == 0)) {
			Item (choice).m_group = 1;
			Item (choice).m_bRedraw = 1;
			if (!strnicmp (Item (choice).m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
				Item (choice).m_text [0] = 0;
				Item (choice).m_value = -1;
			} else {
				Item (choice).TrimWhitespace ();
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && bCloseBox) {
			MouseGetPos (&mx, &my);
			x1 = (gameOpts->menus.nStyle ? m_props.x : CCanvas::Current ()->Left ()) + CLOSE_X;
			x2 = x1 + CLOSE_SIZE;
			y1 = (gameOpts->menus.nStyle ? m_props.y : CCanvas::Current ()->Top ()) + CLOSE_Y;
			y2 = y1 + CLOSE_SIZE;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (nCurItemP)
					*nCurItemP = choice;
				choice = -1;
				done = 1;
				}
			}

//	 HACK! Don't redraw loadgame preview
		if (bRestoringMenu) 
			Item (0).m_bRedraw = 0;

		if (choice > - 1) {
			int ascii;

			if (((Item (choice).m_nType == NM_TYPE_INPUT) || 
				 ((Item (choice).m_nType == NM_TYPE_INPUT_MENU) && (Item (choice).m_group == 1))) && (oldChoice == choice)) {
				if (k == KEY_LEFT || k == KEY_BACKSP || k == KEY_PAD4) {
					if (Item (choice).m_value == -1) 
						Item (choice).m_value = (int) strlen (Item (choice).m_text);
					if (Item (choice).m_value > 0)
						Item (choice).m_value--;
					Item (choice).m_text [Item (choice).m_value] = 0;
					Item (choice).m_bRedraw = 1;
					}
				else {
					ascii = KeyToASCII (k);
					if ((ascii < 255) && (Item (choice).m_value < Item (choice).m_nTextLen)) {
						int bAllowed;

						if (Item (choice).m_value== -1)
							Item (choice).m_value = 0;
						bAllowed = CharAllowed ((char) ascii);
						if (!bAllowed && ascii == ' ' && CharAllowed ('_')) {
							ascii = '_';
							bAllowed = 1;
							}
						if (bAllowed) {
							Item (choice).m_text [Item (choice).m_value++] = ascii;
							Item (choice).m_text [Item (choice).m_value] = 0;
							Item (choice).m_bRedraw = 1;
							}
						}
					}
				}
			else if ((Item (choice).m_nType != NM_TYPE_INPUT) && (Item (choice).m_nType != NM_TYPE_INPUT_MENU)) {
				ascii = KeyToASCII (k);
				if (ascii < 255) {
					int choice1 = choice;
					ascii = toupper (ascii);
					do {
						int i, ch = 0, t;

						if (++choice1 >= int (ToS ())) 
							choice1 = 0;
						t = Item (choice1).m_nType;
						if (gameStates.app.bNostalgia)
							ch = Item (choice1).m_text [0];
						else if (Item (choice1).m_nKey > 0)
							ch = MENU_KEY (Item (choice1).m_nKey, 0);
						else if (Item (choice1).m_nKey < 0) //skip any leading blanks
							for (i = 0; (ch = Item (choice1).m_text [i]) && ch == ' '; i++)
								;
						else
							continue;
								;
						if (((t == NM_TYPE_MENU) ||
							  (t == NM_TYPE_CHECK) ||
							  (t == NM_TYPE_RADIO) ||
							  (t == NM_TYPE_NUMBER) ||
							  (t == NM_TYPE_SLIDER))
								&& (ascii == toupper (ch))) {
							k = 0;
							choice = choice1;
							if (oldChoice> - 1)
								Item (oldChoice).m_bRedraw = 1;
							Item (choice).m_bRedraw = 1;
							if (choice < m_props.nScrollOffset) {
								m_props.nScrollOffset = choice;
								REDRAW_ALL;
								}
							else if (choice > m_props.nScrollOffset + m_props.nMaxDisplayable - 1) {
								m_props.nScrollOffset = choice;
								if (m_props.nScrollOffset + m_props.nMaxDisplayable >= int (ToS ())) {
									m_props.nScrollOffset = int (ToS ()) - m_props.nMaxDisplayable;
									if (m_props.nScrollOffset < m_props.nMaxNoScroll)
										m_props.nScrollOffset = m_props.nMaxNoScroll;
									}
								REDRAW_ALL;
								}
							if (t == NM_TYPE_CHECK)
								goto checkOption;
							else if (t == NM_TYPE_RADIO)
								goto radioOption;
							else if ((t != NM_TYPE_SLIDER) && (Item (choice1).m_nKey != 0))
								if (gameOpts->menus.nHotKeys > 0)
									goto launchOption;
						}
					} while (choice1 != choice);
				}
			}

			if ((Item (choice).m_nType == NM_TYPE_NUMBER) || (Item (choice).m_nType == NM_TYPE_SLIDER)) {
				int ov = Item (choice).m_value;
				switch (k) {
				case KEY_PAD4:
			 	case KEY_LEFT:
			 	case KEY_MINUS:
				case KEY_MINUS + KEY_SHIFTED:
				case KEY_PADMINUS:
					Item (choice).m_value -= 1;
					break;
			 	case KEY_RIGHT:
				case KEY_PAD6:
			 	case KEY_EQUAL:
				case KEY_EQUAL + KEY_SHIFTED:
				case KEY_PADPLUS:
					Item (choice).m_value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					Item (choice).m_value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					Item (choice).m_value -= 10;
					break;
				}
				if (ov!= Item (choice).m_value)
					Item (choice).m_bRedraw = 1;
			}

		}


	// Redraw everything...
#if 1
		bRedraw = 0;
		if (bRedrawAll && !gameOpts->menus.nStyle) {
			int t;
			backgroundManager.Draw ();
			t = DrawTitle (pszTitle, TITLE_FONT, RGBA_PAL (31, 31, 31), m_props.yOffs);
			DrawTitle (pszSubTitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
			bRedrawAll = 0;
			}
		if (!gameOpts->menus.nStyle)
			CCanvas::SetCurrent (backgroundManager.Canvas ());
		fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);
	 	for (i = 0; i < m_props.nMaxDisplayable + m_props.nScrollOffset - m_props.nMaxNoScroll; i++) {
			if ((i >= m_props.nMaxNoScroll) && (i < m_props.nScrollOffset))
				continue;
			if (!(gameOpts->menus.nStyle || (Item (i).m_text && *Item (i).m_text)))
				continue;
			if (bStart || (gameStates.ogl.nDrawBuffer == GL_BACK) || Item (i).m_bRedraw || Item (i).m_bRebuild) {// warning! ugly hack below 
				bRedraw = 1;
				if (Item (i).m_bRebuild && Item (i).m_bCentered)
					Item (i).m_x = fontManager.Current ()->GetCenteredX (Item (i).m_text);
				if (i >= m_props.nScrollOffset)
 				Item (i).m_y -= ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
				if (!gameOpts->menus.nStyle) 
					SDL_ShowCursor (0);
				Item (i).Draw ((i == choice) && !bAllText, bTinyMode);
				Item (i).m_bRedraw = 0;
				if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
					SDL_ShowCursor (1);
				if (i >= m_props.nScrollOffset)
					Item (i).m_y += ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
				}
			if ((i == choice) && 
				 ((Item (i).m_nType == NM_TYPE_INPUT) || 
				 ((Item (i).m_nType == NM_TYPE_INPUT_MENU) && Item (i).m_group)))
				Item (i).UpdateCursor ();
			}
#endif
 if (m_props.bIsScrollBox) {
 	//fontManager.SetCurrent (NORMAL_FONT);
	if (bRedraw || (nLastScrollCheck != m_props.nScrollOffset)) {
 	nLastScrollCheck = m_props.nScrollOffset;
 	fontManager.SetCurrent (SELECTED_FONT);
 	sy = Item (m_props.nScrollOffset).m_y - ((m_props.nStringHeight + 1)*(m_props.nScrollOffset - m_props.nMaxNoScroll));
 	sx = Item (m_props.nScrollOffset).m_x - (gameStates.menus.bHires?24:12);
 	if (m_props.nScrollOffset > m_props.nMaxNoScroll)
		DrawRightStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, UP_ARROW_MARKER);
 	else
		DrawRightStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, " ");
				i = m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll - 1;
 	sy = Item (i).m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
 	sx = Item (i).m_x - (gameStates.menus.bHires ? 24 : 12);
 	if (m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll < int (ToS ()))
		DrawRightStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, DOWN_ARROW_MARKER);
 	else
		DrawRightStringWXY ((gameStates.menus.bHires ? 20 : 10), sx, sy, " ");
			}
 		}
		if (bCloseBox && (bStart || gameOpts->menus.nStyle)) {
			if (gameOpts->menus.nStyle)
				DrawCloseBox (m_props.x, m_props.y);
			else
				DrawCloseBox (m_props.x - CCanvas::Current ()->Left (), m_props.y - CCanvas::Current ()->Top ());
			bCloseBox = 1;
			}
		if (bRedraw || !gameOpts->menus.nStyle)
			GrUpdate (0);
		bRedraw = 1;
		bStart = 0;
		if (!bDontRestore && paletteManager.EffectDisabled ()) {
			paletteManager.EnableEffect ();
		}
	}
SDL_ShowCursor (0);
// Restore everything...
RestoreScreen (filename, bDontRestore);
FreeTextBms ();
gameStates.input.keys.bRepeat = bKeyRepeat;
GameFlushInputs ();
if (time_stopped) {
	StartTime (0);
#ifdef TACTILE
		if (TactileStick)
			EnableForces ();
#endif
 }
if (sound_stopped)
	audio.ResumeSounds ();
gameStates.menus.nInMenu--;
paletteManager.SetEffect (0, 0, 0);
SDL_EnableKeyRepeat (0, 0);
if (gameStates.app.bGameRunning && IsMultiGame)
	MultiSendMsgQuit();
return choice;
}

//------------------------------------------------------------------------------ 

int _CDECL_ MsgBox (const char* pszTitle, pMenuCallback callback, char* filename, int nChoices, ...)
{
	int				i;
	char*				format, * s;
	va_list			args;
	char				szSubTitle [MESSAGEBOX_TEXT_SIZE];
	CMenu	mm;

if (!mm.Create (5))
	return - 1;

va_start (args, nChoices);
for (i = 0; i < nChoices; i++) {
	s = va_arg (args, char *);
	mm.AddMenu (s, - 1);
	}
format = va_arg (args, char*);
vsprintf (szSubTitle, format, args);
va_end (args);
Assert (strlen (szSubTitle) < MESSAGEBOX_TEXT_SIZE);
return mm.Menu (pszTitle, szSubTitle, callback, NULL, filename);
}

//------------------------------------------------------------------------------ 

int _CDECL_ MsgBox (const char* pszTitle, char* filename, int nChoices, ...)
{
	int				h, i, l, bTiny, nInMenu;
	char				*format, *s;
	va_list			args;
	char				nm_text [MESSAGEBOX_TEXT_SIZE];
	CMenu	mm;


if (!(nChoices && mm.Create (10)))
	return -1;
if ((bTiny = (nChoices < 0)))
	nChoices = -nChoices;
va_start (args, nChoices);
for (i = l = 0; i < nChoices; i++) {
	s = va_arg (args, char* );
	h = (int) strlen (s);
	if (l + h > MESSAGEBOX_TEXT_SIZE)
		break;
	l += h;
	if (!bTiny || i) 
		mm.AddMenu (s, - 1);
	else {
		mm.AddText (s);
		mm.Item (i).m_bUnavailable = 1;
		}
	if (bTiny)
		mm.Item (i).m_bCentered = (i != 0);
	}
if (!bTiny) {
	format = va_arg (args, char* );
	vsprintf (nm_text, format, args);
	va_end (args);
	}
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
i = bTiny 
	 ? mm.Menu (NULL, pszTitle, NULL, NULL, NULL, LHX (340), - 1, 1)
	 : mm.Menu (pszTitle, nm_text, NULL, NULL, filename);
gameStates.menus.nInMenu = nInMenu;
return i;
}

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

#define MAX_FILES 300

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
if (!filenames.Create (MAX_FILES))
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
		if (nFileCount < MAX_FILES) {
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
			if (nFileCount<MAX_FILES) {
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
		fontManager.Current ()->StringSize (filenames [nFileCount], w, h, aw);	
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
	filenames.SortAscending ();
else {
	filenames.SortAscending (1, filenames.Length () - 2); 
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
		x1 = w_x + CLOSE_X + 2;
		x2 = x1 + CLOSE_SIZE - 2;
		y1 = w_y + CLOSE_Y + 2;
		y2 = y1 + CLOSE_SIZE - 2;
		if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
			nItem = -1;
			done = 1;
			}
		}

	if ((nPrevItem != nFirstItem) || gameOpts->menus.nStyle) {
		if (!gameOpts->menus.nStyle) 
			SDL_ShowCursor (0);
		backgroundManager.Draw ();
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

		if (!gameOpts->menus.nStyle) 
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
	GrUpdate (0);
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
GrUpdate (0);
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
			x1 = close_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = close_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				nItem = -1;
				done = 1;
			}
		}

		if ((nPrevItem != nFirstItem) || redraw || gameOpts->menus.nStyle) {
			if (gameOpts->menus.nStyle) 
				backgroundManager.Draw ();
				//NMDrawBackground (wx - border_size, wy - nTitleHeight - border_size, wx + width + border_size - 1, wy + height + border_size - 1,1);
			else
				SDL_ShowCursor (0);
			if (gameOpts->menus.nStyle) {
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

			if (!gameOpts->menus.nStyle) 
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
GrUpdate (0);
SDL_EnableKeyRepeat(0, 0);
return nItem;
}

//------------------------------------------------------------------------------ 

int FileList (char* pszTitle, char* filespec, char* filename)
{
	static char filenameList [MAX_FILES][FILENAME_LEN + 1];

	int				i, nFiles;
	CStack<char*>	filenames;// [MAX_FILES];
	FFS				ffs;

if (!filenames.Create (MAX_FILES))
	return - 1;
nFiles = 0;
if (!FFF (filespec, &ffs, 0)) {
	do {
		if (filenames.ToS () >= MAX_FILES) 
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
	char				szSubTitle [MESSAGEBOX_TEXT_SIZE];
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
Assert (strlen (szSubTitle) < MESSAGEBOX_TEXT_SIZE);
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
	int				i, nInMenu;
	CMenu	mm;

mm.Create (3);
mm.AddGauge ("                    ", 0, nMaxProgress); //the blank string denotes the screen width of the gauge
mm.Item (0).m_bCentered = 1;
mm.Item (0).m_value = nCurProgress;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameData.app.bGamePaused = 1;
do {
	i = mm.Menu (NULL, szCaption, doProgress);
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
