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
#include "rendermine.h"
#include "ogl_render.h"
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

#define RETRO_STYLE	0 //gameStates.ogl.nDrawBuffer != GL_BACK
#define MODERN_STYLE	1 //gameOpts->menus.nStyle
#define FAST_MENUS	1 //gameOpts->menus.bFastMenus

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

#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

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
		gameData.menu.keyColor = RGB_PAL (57, 49, 20);
		gameData.menu.tabbedColor = WHITE_RGBA;
		gameData.menu.tinyColors [0][0] = RGB_PAL (29, 29, 47);
		gameData.menu.tinyColors [0][1] = RED_RGBA;
		gameData.menu.tinyColors [1][0] = RGB_PAL (57, 49, 20);
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
		m_text [i] = '\0';
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
if (bCreateTextBms && FAST_MENUS && 
	 (bmP || (bmP = CreateStringBitmap (m_text, MENU_KEY (m_nKey, - 1), gameData.menu.keyColor, nTabs, m_bCentered, m_w, 0)))) {
	bmP->Render (CCanvas::Current (), m_x, m_y, bmP->Width (), bmP->Height (), 0, 0, bmP->Width (), bmP->Height (), 1, 0);
	m_bmText [bIsCurrent] = bmP;
	}
else {

		int	w, h, aw, l, i, 
				x = m_x, 
				y = m_y;
		char	*t, *ps = m_text, s [MENU_MAX_TEXTLEN], ch = 0, ch2;

	if ((t = strchr (ps, '\n'))) {
		strncpy (s, ps, sizeof (s));
		SetText (s);
		fontManager.Current ()->StringSize (s, w, h, aw);
		do {
			if ((t = strchr (m_text, '\n')))
				*t = '\0';
			DrawHotKeyString (0, bTiny, 0, nDepth + 1);
			if (!t)
				break;
			SetText (t + 1);
			m_y += h / 2;
			m_x = m_xSave;
			nTabIndex = -1;
			} while (*(m_text));
		}
	else if ((t = strchr (ps, '\t'))) {
		strncpy (s, ps, sizeof (s));
		SetText (s);
		fontManager.Current ()->StringSize (s, w, h, aw);
		do {
			if ((t = strchr (m_text, '\t')))
				*t = '\0';
			DrawHotKeyString (0, bTiny, 0, nDepth + 1);
			if (!t)
				break;
			SetText (t + 1);
			nTabIndex++;
		} while (*(m_text));
		nTabIndex = -1;
		SetText (ps);
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
if (RETRO_STYLE)
	backgroundManager.Top ()->Saved (1)->BlitClipped (CCanvas::Current (), 5, y - 1, backgroundManager.Current ()->Width () - 15, h + 2, 5, y - 1);
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
char* p = strchr (m_text, '\t');
if (p) {
	*p = '\0';
	s1 = p + 1;
	}

int w, h, aw;
fontManager.Current ()->StringSize (m_text, w, h, aw);

int y = m_y;
if (RETRO_STYLE)
	backgroundManager.Current ()->BlitClipped (CCanvas::Current (), 5, y, backgroundManager.Current ()->Width () - 15, h, 5, y);
DrawHotKeyString (bIsCurrent, bTiny, 1, 0);
if (p) {
	fontManager.Current ()->StringSize (s1, w, h, aw);
	int x = m_x + m_w - w;
	if (RETRO_STYLE) {
		backgroundManager.Current ()->BlitClipped (CCanvas::Current (), x, y, w, 1, x, y);
		backgroundManager.Current ()->BlitClipped (CCanvas::Current (), x, y + h - 1, w, 1, x, y);
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

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (RETRO_STYLE)
	backgroundManager.Current ()->BlitClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
SaveText ();
SetText (s);
h = m_x;
m_x = x - w;
DrawHotKeyString (bIsCurrent, bTiny, 0, 0);
RestoreText ();
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
OglDrawFilledRect (x - 1, y - 1, x - 1, y + h - 1);
OglDrawFilledRect (x - 1, y - 1, x + w1 - 1, y - 1);
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (5, 5, 5));
OglDrawFilledRect (x, y + h, x + w1, y + h);
OglDrawFilledRect (x + w1, y - 1, x + w1, y + h);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
OglDrawFilledRect (x, y, x + w1 - 1, y + h - 1);
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
	OglDrawFilledRect (x + w1 + 1, y, x + w, y + h - 2);
	}
CCanvas::Current ()->SetColorRGB (200, 0, 0, 255);
if (w1)
	OglDrawFilledRect (x + 1, y, x + w1, y + h - 2);
OglDrawEmptyRect (x, y, x + w - 1, y + h - 1);
}

//------------------------------------------------------------------------------ 

void CMenuItem::ShowHelp (void)
{
if (m_szHelp && *m_szHelp) {
	int nInMenu = gameStates.menus.nInMenu;
	gameStates.menus.nInMenu = 0;
	gameData.menu.helpColor = RGB_PAL (47, 47, 47);
	gameData.menu.colorOverride = gameData.menu.helpColor;
	MsgBox ("D2X - XL online help", NULL, - 3, m_szHelp, " ", TXT_CLOSE);
	gameData.menu.colorOverride = 0;
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
	OglDrawFilledRect (x, y, x + CCanvas::Current ()->Font ()->Width () - 1, y + CCanvas::Current ()->Font ()->Height () - 1);
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
		SaveText ();
		if (m_value < m_minValue) 
			m_value = m_minValue;
		else if (m_value > m_maxValue) 
			m_value = m_maxValue;
		sprintf (m_text, "%s\t%s", m_savedText, SLIDER_LEFT);
		int l = int (strlen (m_text));
		int h = m_maxValue - m_minValue + 1;
		memset (m_text + l, SLIDER_MIDDLE [0], h);
		m_text [l + h] = SLIDER_RIGHT [0];
		m_text [l + h + 1] = '\0';
		m_text [m_value + 1 + strlen (m_savedText) + 1] = SLIDER_MARKER [0];
		DrawSlider (bIsCurrent, bTiny);
		RestoreText ();
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
	m_savedText [0] = '\t';
	m_savedText [1] = SLIDER_LEFT [0];
	memset (m_savedText + 2, SLIDER_MIDDLE [0], (m_maxValue - m_minValue + 1));
	m_savedText [m_maxValue - m_minValue + 2] = SLIDER_RIGHT [0];
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
	char szValue [20];
	nOthers++;
	sprintf (szValue, "%d", m_maxValue);
	fontManager.Current ()->StringSize (szValue, w1, h1, aw1);
	m_rightOffset = w1;
	sprintf (szValue, "%d", m_minValue);
	fontManager.Current ()->StringSize (szValue, w1, h1, aw1);
	if (w1 > m_rightOffset)
		m_rightOffset = w1;
	}
else if (m_nType == NM_TYPE_INPUT) {
	SetText (m_text, m_savedText);
	nOthers++;
	nStringWidth = m_nTextLen * CCanvas::Current ()->Font ()->Width () + ((gameStates.menus.bHires ? 3 : 1) * m_nTextLen);
	if (nStringWidth > MAX_TEXT_WIDTH) 
		nStringWidth = MAX_TEXT_WIDTH;
	m_value = -1;
	}
else if (m_nType == NM_TYPE_INPUT_MENU) {
	SetText (m_text, m_savedText);
	nMenus++;
	nStringWidth = m_nTextLen * CCanvas::Current ()->Font ()->Width () + ((gameStates.menus.bHires ? 3 : 1) * m_nTextLen);
	m_value = -1;
	m_group = 0;
	}
m_w = nStringWidth;
m_h = nStringHeight;
return nStringHeight;
}

//------------------------------------------------------------------------------

void CMenuItem::SetText (const char* pszSrc, char* pszDest)
{
if (!pszDest)
	pszDest = m_text;
strncpy (pszDest, pszSrc, MENU_MAX_TEXTLEN);
pszDest [MENU_MAX_TEXTLEN] = '\0';
}

//------------------------------------------------------------------------------

void CMenuItem::SaveText (void)
{
SetText (m_text, m_savedText);
}

//------------------------------------------------------------------------------

void CMenuItem::RestoreText (void)
{
SetText (m_savedText);
}

//------------------------------------------------------------------------------ 

char* CMenuItem::GetInput (void)
{
if (m_pszText)
	strcpy (m_pszText, m_text);
return m_pszText;
}

//------------------------------------------------------------------------------
//eof
