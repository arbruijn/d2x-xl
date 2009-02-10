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

#define LHX(x) (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y) (gameStates.menus.bHires? (24 * (y)) / 10 : y)

//------------------------------------------------------------------------------ 

char bPauseableMenu = 0;
char bAlreadyShowingInfo = 0;

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
if (RETRO_STYLE)
	backgroundManager.Current ()->BlitClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
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

void CMenu::DrawCloseBox (int x, int y)
{
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
OglDrawFilledRect (x + MENU_CLOSE_X, y + MENU_CLOSE_Y, x + MENU_CLOSE_X + MENU_CLOSE_SIZE, y + MENU_CLOSE_Y + MENU_CLOSE_SIZE);
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (21, 21, 21));
OglDrawFilledRect (x + MENU_CLOSE_X + LHX (1), y + MENU_CLOSE_Y + LHX (1), x + MENU_CLOSE_X + MENU_CLOSE_SIZE - LHX (1), y + MENU_CLOSE_Y + MENU_CLOSE_SIZE - LHX (1));
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
for (uint i = 0; i < ToS (); i++) {
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
for (uint i = 0; i < ToS (); i++) {
	if (MODERN_STYLE && ((Item (i).m_x == short (0x8000)) || Item (i).m_bCentered)) {
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
		for (uint j = 0; j < ToS (); j++) {
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
m_props.nMaxOnMenu = (((MODERN_STYLE && (m_props.scHeight > 480)) ? m_props.scHeight * 4 / 5 : 480) - m_props.th - LHY (8)) / m_props.nStringHeight - 2;
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
if (MODERN_STYLE) {
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

void CMenu::RestoreScreen (char* filename, int m_bDontRestore)
{
//CCanvas::SetCurrent (bg->menu_canvas);
backgroundManager.Remove ();
if (gameStates.app.bGameRunning && !gameOpts->menus.nStyle)
	backgroundManager.Remove ();	// remove the stars background loaded for old style menus
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

void CMenu::SwapText (int i, int j)
{
	char h [MENU_MAX_TEXTLEN + 1];

memcpy (h, Item (i).m_text, MENU_MAX_TEXTLEN + 1);
memcpy (Item (i).m_text, Item (j).m_text, MENU_MAX_TEXTLEN + 1);
memcpy (Item (j).m_text, h, MENU_MAX_TEXTLEN + 1);
}

//------------------------------------------------------------------------------ 

void CMenu::Render (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
	static	int t0 = 0;

	int	i, sx, sy;

if (SDL_GetTicks () - m_tEnter > gameOpts->menus.nFade) {
	i = SDL_GetTicks ();
	if (i - t0 < 25)
		return;
	t0 = 0;
	}

m_bRedraw = 0;

if (gameStates.app.bGameRunning && gameCanvasP /*&& (gameData.demo.nState == ND_STATE_NORMAL)*/) {
	CCanvas::Push ();
	CCanvas::SetCurrent (gameCanvasP);
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
else
	console.Draw ();

FadeIn ();
backgroundManager.Redraw ();
i = DrawTitle (pszTitle, TITLE_FONT, RGB_PAL (31, 31, 31), m_props.yOffs);
DrawTitle (pszSubTitle, SUBTITLE_FONT, RGB_PAL (21, 21, 21), i);
if (!m_bRedraw)
	m_props.ty = i;

if (m_callback && (gameStates.render.grAlpha == 1.0f))
	m_nChoice = (*m_callback) (*this, m_nKey, m_nChoice, 1);

fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);
for (i = 0; i < m_props.nMaxDisplayable + m_props.nScrollOffset - m_props.nMaxNoScroll; i++) {
	if ((i >= m_props.nMaxNoScroll) && (i < m_props.nScrollOffset))
		continue;
	if (!(/*MODERN_STYLE ||*/ *Item (i).m_text))
		continue;
	if (m_bStart || MODERN_STYLE || Item (i).m_bRedraw || Item (i).m_bRebuild) {// warning! ugly hack below 
		m_bRedraw = 1;
		if (Item (i).m_bRebuild && Item (i).m_bCentered)
			Item (i).m_x = fontManager.Current ()->GetCenteredX (Item (i).m_text);
		if (i >= m_props.nScrollOffset)
			Item (i).m_y -= ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		if (!MODERN_STYLE) 
			SDL_ShowCursor (0);
		Item (i).Draw ((i == m_nChoice) && !m_bAllText, m_props.bTinyMode);
		Item (i).m_bRedraw = 0;
		if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
			SDL_ShowCursor (1);
		if (i >= m_props.nScrollOffset)
			Item (i).m_y += ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		}
	if ((i == m_nChoice) && 
		 ((Item (i).m_nType == NM_TYPE_INPUT) || 
		 ((Item (i).m_nType == NM_TYPE_INPUT_MENU) && Item (i).m_group)))
		Item (i).UpdateCursor ();
	}

if (m_props.bIsScrollBox) {
//fontManager.SetCurrent (NORMAL_FONT);
	if (m_bRedraw || (m_nLastScrollCheck != m_props.nScrollOffset)) {
		m_nLastScrollCheck = m_props.nScrollOffset;
		fontManager.SetCurrent (SELECTED_FONT);
		sy = Item (m_props.nScrollOffset).m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		sx = Item (m_props.nScrollOffset).m_x - (gameStates.menus.bHires ? 24 : 12);
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
	if (m_bCloseBox && (m_bStart || MODERN_STYLE)) {
		if (MODERN_STYLE)
			DrawCloseBox (m_props.x, m_props.y);
		else
			DrawCloseBox (m_props.x - CCanvas::Current ()->Left (), m_props.y - CCanvas::Current ()->Top ());
		m_bCloseBox = 1;
		}
	if (m_bRedraw || !MODERN_STYLE)
		GrUpdate (0);
	m_bRedraw = 1;
	m_bStart = 0;
	if (!m_bDontRestore && paletteManager.EffectDisabled ()) {
		paletteManager.EnableEffect ();
	}
gameStates.render.grAlpha = 1.0f;
}

//------------------------------------------------------------------------------ 

void CMenu::FadeIn (void)
{
if (gameOpts->menus.nFade) {
	if (m_tEnter < 0)
		m_tEnter = SDL_GetTicks ();
	int t = int (SDL_GetTicks () - m_tEnter);
	if (t < 0)
		t = 0;
	gameStates.render.grAlpha = (t < int (gameOpts->menus.nFade)) ? float (t) / float (gameOpts->menus.nFade) : 1.0f;
	}
else {
	m_tEnter = 0;
	gameStates.render.grAlpha = 1.0f;
	}
}

//------------------------------------------------------------------------------ 

void CMenu::FadeOut (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
if (gameOpts->menus.nFade) {
	int t = int (gameOpts->menus.nFade * gameStates.render.grAlpha);
	int t0 = SDL_GetTicks ();
	int t1, dt;
	for (;;) {
		t1 = SDL_GetTicks ();
		dt = t1 - t0;
		m_tEnter = t1 - gameOpts->menus.nFade + dt;
		Render (pszTitle, pszSubTitle, gameCanvasP);
		if (dt > t)
			break;
		}
	}
}

//------------------------------------------------------------------------------ 

#define REDRAW_ALL	for (i = 0; i < int (ToS ()); i++) Item (i).m_bRedraw = 1; bRedrawAll = 1

int CMenu::Menu (const char* pszTitle, const char* pszSubTitle, pMenuCallback callback, 
					  int* nCurItemP, char* filename, int width, int height, int bTinyMode)
{
	int			bKeyRepeat, done, nItem = nCurItemP ? *nCurItemP : 0;
	int			oldChoice, i;
	int			bSoundStopped = 0, bTimeStopped = 0;
	int			topChoice;// Is this a scrolling box? Set to 0 at init
	int			bRedrawAll = 0;
	CCanvas*		gameCanvasP;
	int			bWheelUp, bWheelDown, nMouseState, nOldMouseState, bDblClick = 0;
	int			mx = 0, my = 0, x1 = 0, x2, y1, y2;
	int			bLaunchOption = 0;

if (gameStates.menus.nInMenu)
	return - 1;

m_tEnter = -1;
m_bRedraw = 0;
m_nChoice = 0;
m_nLastScrollCheck = -1;
m_bStart = 1;
m_bCloseBox = 0;
m_bDontRestore = 0;
m_bAllText = 0;
m_callback = callback;

paletteManager.DisableEffect ();
messageBox.Clear ();
memset (&m_props, 0, sizeof (m_props));
m_props.width = width;
m_props.height = height;
m_props.bTinyMode = bTinyMode;
FlushInput ();

if (int (ToS ()) < 1)
	return - 1;
if (gameStates.app.bGameRunning && !gameOpts->menus.nStyle)
	backgroundManager.LoadStars (true);
SDL_ShowCursor (0);
SDL_EnableKeyRepeat(60, 30);
gameStates.menus.nInMenu++;
if (!MODERN_STYLE && (gameStates.app.nFunctionMode == FMODE_GAME) && !(gameData.app.nGameMode & GM_MULTI)) {
	audio.PauseSounds ();
	bSoundStopped = 1;
	}

if (!(MODERN_STYLE || (IsMultiGame && (gameStates.app.nFunctionMode == FMODE_GAME) && (!gameStates.app.bEndLevelSequence)))) {
	bTimeStopped = 1;
	StopTime ();
	#ifdef TACTILE 
	 if (TactileStick)
		 DisableForces ();
	#endif
	}

if (gameStates.app.bGameRunning && IsMultiGame)
	gameData.multigame.nTypingTimeout = 0;

SetPopupScreenMode ();
#if 0
if (!MODERN_STYLE) {
	OglSetDrawBuffer (GL_FRONT, 1);
	if (gameStates.menus.bNoBackground || gameStates.app.bGameRunning) {
		//NMLoadBackground (NULL, NULL, 0);
		GrUpdate (0);
		}
	}
#endif
SaveScreen (&gameCanvasP);
bKeyRepeat = gameStates.input.keys.bRepeat;
gameStates.input.keys.bRepeat = 1;
if (nItem == -1)
	m_nChoice = -1;
else {
	if (nItem < 0) 
		nItem = 0;
	else 
		nItem %= int (ToS ());
	m_nChoice = nItem;
	bDblClick = 1;
	while (Item (m_nChoice).m_nType == NM_TYPE_TEXT) {
		m_nChoice++;
		if (m_nChoice >= int (ToS ()))
			m_nChoice = 0; 
		if (m_nChoice == nItem) {
			m_nChoice = 0; 
			m_bAllText = 1;
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
m_bCloseBox = !(filename || gameStates.menus.bReordering);

if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING) {
	SDL_ShowCursor (1);
	}
GrabMouse (0, 0);
while (!done) {
	if (nCurItemP)
		*nCurItemP = m_nChoice;
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
		m_nKey = KEY_UP;
	else if (bWheelDown)
		m_nKey = KEY_DOWN;
	else
		m_nKey = KeyInKey ();
	if (mouseData.bDoubleClick)
		m_nKey = KEY_ENTER;
	if ((m_props.scWidth != screen.Width ()) || (m_props.scHeight != screen.Height ())) {
		memset (&m_props, 0, sizeof (m_props));
		m_props.width = width;
		m_props.height = height;
		m_props.bTinyMode = bTinyMode;
		}
#if 1
	if (m_props.bValid && (m_props.nDisplayMode != gameStates.video.nDisplayMode)) {
		FreeTextBms ();
		SetScreenMode (SCREEN_MENU);
		SaveScreen (&gameCanvasP);
		memset (&m_props, 0, sizeof (m_props));
		m_props.width = width;
		m_props.height = height;
		m_props.bTinyMode = bTinyMode;
		}
#endif
	if (InitProps (pszTitle, pszSubTitle)) {
		backgroundManager.Setup (filename, m_props.x, m_props.y, m_props.w, m_props.h);
		if (!MODERN_STYLE)
			CCanvas::SetCurrent (backgroundManager.Canvas ());
		m_bRedraw = 0;
		m_props.ty = m_props.yOffs;
		if (m_nChoice > m_props.nScrollOffset + m_props.nMaxOnMenu - 1)
			m_props.nScrollOffset = m_nChoice - m_props.nMaxOnMenu + 1;
		}

	if (callback && (SDL_GetTicks () - m_tEnter > gameOpts->menus.nFade))
		m_nChoice = (*callback) (*this, m_nKey, m_nChoice, 0);

	if (!bTimeStopped){
		// Save current menu box
		if (MultiMenuPoll () == -1)
			m_nKey = -2;
		}

	if (m_nKey < - 1) {
		m_bDontRestore = (m_nKey == -3);		// - 3 means don't restore
		if (m_nChoice >= 0)
			m_nChoice = m_nKey;
		else {
			m_nChoice = -m_nChoice - 1;
			*nCurItemP = m_nChoice;
			}
		m_nKey = -1;
		done = 1;
		}
	oldChoice = m_nChoice;
	if (m_nKey && (console.Events (m_nKey) || bWheelUp || bWheelDown))
		switch (m_nKey) {

		case KEY_SHIFTED + KEY_ESC:
			console.Show ();
			m_nKey = -1;
			break;

		case KEY_I:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 ShowNetGameInfo (m_nChoice - 2 - tracker.m_bUse);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done = 1;
			 m_nChoice = -1;
			}
		 break;
		 
		case KEY_U:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 NetworkRequestPlayerNames (m_nChoice - 2 - tracker.m_bUse);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done = 1;
			 m_nChoice = - 1;
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
				m_nChoice = - 1;
				}
			else
				gameData.app.bGamePaused = !gameData.app.bGamePaused;
			break;

		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (m_bAllText) 
				break;
			do {
				m_nChoice--;
 				if (m_props.bIsScrollBox) {
					m_nLastScrollCheck = - 1;
		 			if (m_nChoice < topChoice) { 
						m_nChoice = int (ToS ()) - 1; 
						m_props.nScrollOffset = int (ToS ()) - m_props.nMaxDisplayable + m_props.nMaxNoScroll;
						if (m_props.nScrollOffset < m_props.nMaxNoScroll)
							m_props.nScrollOffset = m_props.nMaxNoScroll;
						REDRAW_ALL;
						break; 
						}
					else if (m_nChoice < m_props.nScrollOffset) {
						REDRAW_ALL;
						m_props.nScrollOffset--;
						if (m_props.nScrollOffset < m_props.nMaxNoScroll)
							m_props.nScrollOffset = m_props.nMaxNoScroll;
						}
					}
				else {
					if (m_nChoice >= int (ToS ())) 
						m_nChoice = 0;
					else if (m_nChoice < 0) 
						m_nChoice = int (ToS ()) - 1;
					}
				} while (Item (m_nChoice).m_nType == NM_TYPE_TEXT);
			if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) && (m_nChoice != oldChoice))
				Item (m_nChoice).m_value = -1;
			if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != m_nChoice)) {
				Item (oldChoice).m_group = 0;
				strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
				Item (oldChoice).m_value = -1;
				}
			if (oldChoice> - 1) 
				Item (oldChoice).m_bRedraw = 1;
			Item (m_nChoice).m_bRedraw = 1;
			break;

			case KEY_TAB:
			case KEY_DOWN:
			case KEY_PAD2:
		// ((0, "Pressing down! m_props.bIsScrollBox = %d", m_props.bIsScrollBox);
			if (m_bAllText) 
				break;
			do {
				m_nChoice++;
				if (m_props.bIsScrollBox) {
					m_nLastScrollCheck = -1;
						if (m_nChoice == int (ToS ())) {
							m_nChoice = 0;
							if (m_props.nScrollOffset) {
								REDRAW_ALL;
								m_props.nScrollOffset = m_props.nMaxNoScroll;
								}
							}
						if (m_nChoice >= m_props.nMaxOnMenu + m_props.nScrollOffset - m_props.nMaxNoScroll) {
							REDRAW_ALL;
							m_props.nScrollOffset++;
							}
						}
					else {
						if (m_nChoice < 0) 
							m_nChoice = int (ToS ()) - 1;
						else if (m_nChoice >= int (ToS ())) 
							m_nChoice = 0;
						}
					} while (Item (m_nChoice).m_nType == NM_TYPE_TEXT);
 
			if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) && (m_nChoice != oldChoice))
				Item (m_nChoice).m_value = -1;
			if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != m_nChoice)) {
				Item (oldChoice).m_group = 0;
				strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
				Item (oldChoice).m_value = -1;
			}
			if (oldChoice> - 1)
				Item (oldChoice).m_bRedraw = 1;
			Item (m_nChoice).m_bRedraw = 1;
			break;

		case KEY_SPACEBAR:
checkOption:
radioOption:
			if (m_nChoice > - 1) {
				switch (Item (m_nChoice).m_nType) {
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if (Item (m_nChoice).m_value)
						Item (m_nChoice).m_value = 0;
					else
						Item (m_nChoice).m_value = 1;
					if (m_props.bIsScrollBox) {
						if (m_nChoice == (m_props.nMaxOnMenu + m_props.nScrollOffset - 1) || m_nChoice == m_props.nScrollOffset)
							m_nLastScrollCheck = - 1;				
						 }
			
					Item (m_nChoice).m_bRedraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				case NM_TYPE_RADIO:
					for (i = 0; i < int (ToS ()); i++) {
						if ((i != m_nChoice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (m_nChoice).m_group) && (Item (i).m_value)) {
							Item (i).m_value = 0;
							Item (i).m_bRedraw = 1;
						}
					}
					Item (m_nChoice).m_value = 1;
					Item (m_nChoice).m_bRedraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				}
			}
			break;

		case KEY_SHIFTED + KEY_UP:
			if (gameStates.menus.bReordering && m_nChoice != topChoice) {
				SwapText (m_nChoice, m_nChoice - 1);
				::Swap (Item (m_nChoice).m_value, Item (m_nChoice - 1).m_value);
				Item (m_nChoice).m_bRebuild = 
				Item (m_nChoice - 1).m_bRebuild = 1;
				m_nChoice--;
				}
			 break;

		case KEY_SHIFTED + KEY_DOWN:
			if (gameStates.menus.bReordering && m_nChoice !=(int (ToS ()) - 1)) {
				SwapText (m_nChoice, m_nChoice + 1);
				::Swap (Item (m_nChoice).m_value, Item (m_nChoice + 1).m_value);
				Item (m_nChoice).m_bRebuild = 
				Item (m_nChoice + 1).m_bRebuild = 1;
				m_nChoice++;
				}
			 break;

		case KEY_ALTED + KEY_ENTER: {
			//int bLoadCustomBg = NMFreeCustomBg ();
			FreeTextBms ();
			backgroundManager.Restore ();
			//NMRestoreScreen (filename, & m_bDontRestore);
			GrToggleFullScreenGame ();
			GrabMouse (0, 0);
			SetScreenMode (SCREEN_MENU);
			SaveScreen (&gameCanvasP);
			memset (&m_props, 0, sizeof (m_props));
			m_props.width = width;
			m_props.height = height;
			m_props.bTinyMode = bTinyMode;
			continue;
			}

		case KEY_ENTER:
		case KEY_PADENTER:
launchOption:
			if ((m_nChoice > - 1) && (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 0)) {
				Item (m_nChoice).m_group = 1;
				Item (m_nChoice).m_bRedraw = 1;
				if (!strnicmp (Item (m_nChoice).m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
					Item (m_nChoice).m_text [0] = '\0';
					Item (m_nChoice).m_value = -1;
					}
				else {
					Item (m_nChoice).TrimWhitespace ();
					}
				}
			else {
				if (nCurItemP)
					*nCurItemP = m_nChoice;
				done = 1;
				}
			break;

		case KEY_ESC:
			if ((m_nChoice > - 1) && (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 1)) {
				Item (m_nChoice).m_group = 0;
				strcpy (Item (m_nChoice).m_text, Item (m_nChoice).m_savedText);
				Item (m_nChoice).m_bRedraw = 1;
				Item (m_nChoice).m_value = -1;
				}
			else {
				done = 1;
				if (nCurItemP)
					*nCurItemP = m_nChoice;
				m_nChoice = -1;
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
			if ((m_nChoice > - 1) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT_MENU))
				Int3 (); 
			break;
		#endif

		case KEY_F1:
			Item (m_nChoice).ShowHelp ();
			break;
		}

		if (!done && nMouseState && !nOldMouseState && !m_bAllText) {
			MouseGetPos (&mx, &my);
			for (i = 0; i < int (ToS ()); i++) {
				if (!Item (i).Selectable ())
					continue;
				x1 = CCanvas::Current ()->Left () + Item (i).m_x - Item (i).m_rightOffset - 6;
				x2 = x1 + Item (i).m_w;
				y1 = CCanvas::Current ()->Top () + Item (i).m_y;
				y2 = y1 + Item (i).m_h;
				if ((mx > x1) && (mx < x2) && (my > y1) && (my < y2)) {
					if (i + m_props.nScrollOffset - m_props.nMaxNoScroll != m_nChoice) {
						if (bHackDblClickMenuMode) 
							bDblClick = 0; 
					}
				
					m_nChoice = i + m_props.nScrollOffset - m_props.nMaxNoScroll;
					if (m_nChoice >= 0 && m_nChoice < int (ToS ()))
						switch (Item (m_nChoice).m_nType) {
							case NM_TYPE_CHECK:
								Item (m_nChoice).m_value = !Item (m_nChoice).m_value;
								Item (m_nChoice).m_bRedraw = 1;
								if (m_props.bIsScrollBox)
									m_nLastScrollCheck = - 1;
								break;
							case NM_TYPE_RADIO:
								for (i = 0; i < int (ToS ()); i++) {
									if ((i!= m_nChoice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (m_nChoice).m_group) && (Item (i).m_value)) {
										Item (i).m_value = 0;
										Item (i).m_bRedraw = 1;
									}
								}
								Item (m_nChoice).m_value = 1;
								Item (m_nChoice).m_bRedraw = 1;
								break;
							}
					Item (oldChoice).m_bRedraw = 1;
					break;
				}
			}
		}

		if (nMouseState && m_bAllText) {
			if (nCurItemP)
				*nCurItemP = m_nChoice;
			done = 1;
			}
	
		if (!done && nMouseState && !m_bAllText) {
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
						m_nChoice--;
						m_nLastScrollCheck = - 1;
						if (m_nChoice < m_props.nScrollOffset) {
							REDRAW_ALL;
							m_props.nScrollOffset--;
							}
						}
					}
				if ((i = m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll) < int (ToS ()) && Item (i).Selectable ()) {
					fontManager.Current ()->StringSize (DOWN_ARROW_MARKER, arrowWidth, arrowHeight, aw);
					x2 = CCanvas::Current ()->Left () + Item (i - 1).m_x - (gameStates.menus.bHires?24:12);
					y1 = CCanvas::Current ()->Top () + Item (i - 1).m_y - ((m_props.nStringHeight + 1)*(m_props.nScrollOffset - m_props.nMaxNoScroll));
					x1 = x2 - arrowWidth;
					y2 = y1 + arrowHeight;
					if ((mx > x1) && (mx < x2) && (my > y1) && (my < y2)) {
						m_nChoice++;
						m_nLastScrollCheck = - 1;
						if (m_nChoice >= m_props.nMaxOnMenu + m_props.nScrollOffset) {
							REDRAW_ALL;
							m_props.nScrollOffset++;
							}
						}
					}
				}
		
			for (i = m_props.nScrollOffset; i < int (ToS ()); i++) {
				if (!Item (i).Selectable ())
					continue;
				x1 = CCanvas::Current ()->Left () + Item (i).m_x - Item (i).m_rightOffset - 6;
				x2 = x1 + Item (i).m_w;
				y1 = CCanvas::Current ()->Top () + Item (i).m_y;
				y1 -= ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
				y2 = y1 + Item (i).m_h;
				if ((mx > x1) && (mx < x2) && (my > y1) && (my < y2)) {
					if (i /* + m_props.nScrollOffset - m_props.nMaxNoScroll*/ != m_nChoice) {
						if (bHackDblClickMenuMode) 
							bDblClick = 0; 
					}

					m_nChoice = i/* + m_props.nScrollOffset - m_props.nMaxNoScroll*/;

					if (Item (m_nChoice).m_nType == NM_TYPE_SLIDER) {
						char slider_text [MENU_MAX_TEXTLEN + 1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
					
						strcpy (slider_text, Item (m_nChoice).m_savedText);
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

							x1 = CCanvas::Current ()->Left () + Item (m_nChoice).m_x + Item (m_nChoice).m_w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ((mx > x1) && (mx < (x1 + sleft_width)) && (Item (m_nChoice).m_value != Item (m_nChoice).m_minValue)) {
								Item (m_nChoice).m_value = Item (m_nChoice).m_minValue;
								Item (m_nChoice).m_bRedraw = 2;
							} else if ((mx < x2) && (mx > (x2 - sright_width)) && (Item (m_nChoice).m_value != Item (m_nChoice).m_maxValue)) {
								Item (m_nChoice).m_value = Item (m_nChoice).m_maxValue;
								Item (m_nChoice).m_bRedraw = 2;
							} else if ((mx > (x1 + sleft_width)) && (mx < (x2 - sright_width))) {
								int numValues, value_width, newValue;
							
								numValues = Item (m_nChoice).m_maxValue - Item (m_nChoice).m_minValue + 1;
								value_width = (slider_width - sleft_width - sright_width) / numValues;
								newValue = (mx - x1 - sleft_width) / value_width;
								if (Item (m_nChoice).m_value != newValue) {
									Item (m_nChoice).m_value = newValue;
									Item (m_nChoice).m_bRedraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (m_nChoice == oldChoice)
						break;
					if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) && (m_nChoice != oldChoice))
						Item (m_nChoice).m_value = -1;
					if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != m_nChoice)) {
						Item (oldChoice).m_group = 0;
						strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
						Item (oldChoice).m_value = -1;
					}
					if (oldChoice> - 1) 
						Item (oldChoice).m_bRedraw = 1;
					Item (m_nChoice).m_bRedraw = 1;
					break;
				}
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && !m_bAllText && (m_nChoice != -1) && (Item (m_nChoice).m_nType == NM_TYPE_MENU)) {
			MouseGetPos (&mx, &my);
			x1 = CCanvas::Current ()->Left () + Item (m_nChoice).m_x;
			x2 = x1 + Item (m_nChoice).m_w;
			y1 = CCanvas::Current ()->Top () + Item (m_nChoice).m_y;
			if (m_nChoice >= m_props.nScrollOffset)
				y1 -= ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
			y2 = y1 + Item (m_nChoice).m_h;
			if ((mx > x1) && (mx < x2) && (my > y1) && (my < y2)) {
				if (bHackDblClickMenuMode) {
					if (bDblClick) {
						if (nCurItemP)
							*nCurItemP = m_nChoice;
						done = 1;
						}
					else 
						bDblClick = 1;
				}
				else {
					done = 1;
					if (nCurItemP)
						*nCurItemP = m_nChoice;
				}
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && (m_nChoice > - 1) && 
			 (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 0)) {
			Item (m_nChoice).m_group = 1;
			Item (m_nChoice).m_bRedraw = 1;
			if (!strnicmp (Item (m_nChoice).m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
				Item (m_nChoice).m_text [0] = '\0';
				Item (m_nChoice).m_value = -1;
			} else {
				Item (m_nChoice).TrimWhitespace ();
			}
		}
	
		if (!done && !nMouseState && nOldMouseState && m_bCloseBox) {
			MouseGetPos (&mx, &my);
			x1 = (MODERN_STYLE ? m_props.x : CCanvas::Current ()->Left ()) + MENU_CLOSE_X;
			x2 = x1 + MENU_CLOSE_SIZE;
			y1 = (MODERN_STYLE ? m_props.y : CCanvas::Current ()->Top ()) + MENU_CLOSE_Y;
			y2 = y1 + MENU_CLOSE_SIZE;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (nCurItemP)
					*nCurItemP = m_nChoice;
				m_nChoice = -1;
				done = 1;
				}
			}

//	 HACK! Don't redraw loadgame preview
		if (bRestoringMenu) 
			Item (0).m_bRedraw = 0;

		if (m_nChoice > - 1) {
			int ascii;

			if (((Item (m_nChoice).m_nType == NM_TYPE_INPUT) || 
				 ((Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 1))) && (oldChoice == m_nChoice)) {
				if (m_nKey == KEY_LEFT || m_nKey == KEY_BACKSP || m_nKey == KEY_PAD4) {
					if (Item (m_nChoice).m_value == -1) 
						Item (m_nChoice).m_value = (int) strlen (Item (m_nChoice).m_text);
					if (Item (m_nChoice).m_value > 0)
						Item (m_nChoice).m_value--;
					Item (m_nChoice).m_text [Item (m_nChoice).m_value] = '\0';
					Item (m_nChoice).m_bRedraw = 1;
					}
				else {
					ascii = KeyToASCII (m_nKey);
					if ((ascii < 255) && (Item (m_nChoice).m_value < Item (m_nChoice).m_nTextLen)) {
						int bAllowed;

						if (Item (m_nChoice).m_value== -1)
							Item (m_nChoice).m_value = 0;
						bAllowed = CharAllowed ((char) ascii);
						if (!bAllowed && ascii == ' ' && CharAllowed ('_')) {
							ascii = '_';
							bAllowed = 1;
							}
						if (bAllowed) {
							Item (m_nChoice).m_text [Item (m_nChoice).m_value++] = ascii;
							Item (m_nChoice).m_text [Item (m_nChoice).m_value] = '\0';
							Item (m_nChoice).m_bRedraw = 1;
							}
						}
					}
				}
			else if ((Item (m_nChoice).m_nType != NM_TYPE_INPUT) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT_MENU)) {
				ascii = KeyToASCII (m_nKey);
				if (ascii < 255) {
					int choice1 = m_nChoice;
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
							m_nKey = 0;
							m_nChoice = choice1;
							if (oldChoice> - 1)
								Item (oldChoice).m_bRedraw = 1;
							Item (m_nChoice).m_bRedraw = 1;
							if (m_nChoice < m_props.nScrollOffset) {
								m_props.nScrollOffset = m_nChoice;
								REDRAW_ALL;
								}
							else if (m_nChoice > m_props.nScrollOffset + m_props.nMaxDisplayable - 1) {
								m_props.nScrollOffset = m_nChoice;
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
						} while (choice1 != m_nChoice);
					}
				}

			if ((Item (m_nChoice).m_nType == NM_TYPE_NUMBER) || (Item (m_nChoice).m_nType == NM_TYPE_SLIDER)) {
				int ov = Item (m_nChoice).m_value;
				switch (m_nKey) {
				case KEY_PAD4:
			 	case KEY_LEFT:
			 	case KEY_MINUS:
				case KEY_MINUS + KEY_SHIFTED:
				case KEY_PADMINUS:
					Item (m_nChoice).m_value -= 1;
					break;
			 	case KEY_RIGHT:
				case KEY_PAD6:
			 	case KEY_EQUAL:
				case KEY_EQUAL + KEY_SHIFTED:
				case KEY_PADPLUS:
					Item (m_nChoice).m_value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					Item (m_nChoice).m_value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					Item (m_nChoice).m_value -= 10;
					break;
				}
			if (ov!= Item (m_nChoice).m_value)
				Item (m_nChoice).m_bRedraw = 1;
			}
		}
	// Redraw everything...
	Render (pszTitle, pszSubTitle, gameCanvasP);
	}
FadeOut (pszTitle, pszSubTitle, gameCanvasP);
SDL_ShowCursor (0);
// Restore everything...
RestoreScreen (filename, m_bDontRestore);
FreeTextBms ();
gameStates.input.keys.bRepeat = bKeyRepeat;
GameFlushInputs ();
if (bTimeStopped) {
	StartTime (0);
#ifdef TACTILE
		if (TactileStick)
			EnableForces ();
#endif
 }
if (bSoundStopped)
	audio.ResumeSounds ();
gameStates.menus.nInMenu--;
paletteManager.EnableEffect ();
//paletteManager.SetEffect (0, 0, 0);
SDL_EnableKeyRepeat (0, 0);
if (gameStates.app.bGameRunning && IsMultiGame)
	MultiSendMsgQuit();

for (i = 0; i < int (ToS ()); i++)
	Item (i).GetInput ();

return m_nChoice;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 

int CMenu::AddCheck (const char* szText, int nValue, int m_nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_CHECK;
item.m_pszText = NULL;
item.SetText (szText);
item.m_value = NMBOOL (nValue);
item.m_nKey = m_nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddRadio (const char* szText, int nValue, int m_nKey, const char* szHelp)
{
CMenuItem item;
if (!ToS () || (Top ()->m_nType != NM_TYPE_RADIO))
	NewGroup ();
item.m_nType = NM_TYPE_RADIO;
item.m_pszText = NULL;
item.SetText (szText);
item.m_value = nValue;
item.m_nKey = m_nKey;
item.m_group = m_nGroup;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddMenu (const char* szText, int m_nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_MENU;
item.m_pszText = NULL;
item.SetText (szText);
item.m_nKey = m_nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddText (const char* szText, int m_nKey)
{
CMenuItem item;
item.m_nType = NM_TYPE_TEXT;
item.m_pszText = NULL;
item.SetText (szText);
item.m_nKey = m_nKey;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddSlider (const char* szText, int nValue, int nMin, int nMax, int m_nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_SLIDER;
item.m_pszText = NULL;
item.SetText (szText);
item.m_value = NMCLAMP (nValue, nMin, nMax);
item.m_minValue = nMin;
item.m_maxValue = nMax;
item.m_nKey = m_nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szText, int nLen, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_INPUT;
item.m_pszText = (char*) (szText);
item.SetText (szText);
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

int CMenu::AddInputBox (const char* szText, int nLen, int m_nKey, const char* szHelp)
{
CMenuItem item;
item.m_nType = NM_TYPE_INPUT_MENU;
item.m_pszText = (char*) (szText);
item.SetText (szText);
item.m_nTextLen = nLen;
item.m_nKey = m_nKey;
item.m_szHelp = szHelp;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddNumber (const char* szText, int nValue, int nMin, int nMax)
{
CMenuItem item;
item.m_nType = NM_TYPE_NUMBER;
item.m_pszText = (char*) (szText);
item.SetText (szText);
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
item.m_pszText = NULL;
item.SetText (szText);
item.m_nTextLen = *szText ? (int) strlen (szText) : 20;
item.m_value = NMCLAMP (nValue, 0, nMax);
item.m_maxValue = nMax;
Push (item);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
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
//eof
