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
#include "automap.h"
#include "renderframe.h"
#include "console.h"
#if defined (FORCE_FEEDBACK)
#	include "tactile.h"
#endif


//------------------------------------------------------------------------------ 

#define LHX(x) (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y) (gameStates.menus.bHires? (24 * (y)) / 10 : y)

//------------------------------------------------------------------------------ 

char bPauseableMenu = 0;
char bAlreadyShowingInfo = 0;

CMenu* CMenu::m_active = NULL;
int32_t CMenu::m_level = 0;


//------------------------------------------------------------------------------

bool MenuRenderTimeout (int32_t& t0, int32_t tFade)
{
int32_t i = SDL_GetTicks ();

if ((tFade < 0) || (i - tFade > int32_t (gameOpts->menus.nFade))) {
	if (i - t0 < 25) {
		G3_SLEEP (1);
		return false;
		}
	}
t0 = i;
return true;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
// Draw a left justfied string with black background.
void CMenu::DrawRightStringWXY (int32_t w1, int32_t x, int32_t y, const char* s)
{
	int32_t	w, h, aw;

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (RETRO_STYLE)
	backgroundManager.Current ()->BlitClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
GrString (x - w, y, s);
}

//------------------------------------------------------------------------------ 

int32_t CMenu::XOffset (void) { return (gameData.renderData.frame.Width () - m_props.width) / 2; }
int32_t CMenu::YOffset (void) { return (gameData.renderData.frame.Height () - m_props.height) / 2; }

//------------------------------------------------------------------------------ 

char* nmAllowedChars = NULL;

//returns true if char is bAllowed
int32_t CMenu::CharAllowed (char c)
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

int32_t CMenu::TinyMenu (const char* pszTitle, const char* pszSubTitle, pMenuCallback callback)
{
return Menu (pszTitle, pszSubTitle, callback, NULL, BG_SUBMENU, BG_STANDARD, -1, -1, 1);
}

//------------------------------------------------------------------------------ 

int32_t CMenu::FixedFontMenu (const char* pszTitle, const char* pszSubTitle, 
										pMenuCallback callback, int32_t* pnCurItem, int32_t nType, int32_t nWallpaper, 
										int32_t width, int32_t height)
{
SetScreenMode (SCREEN_MENU);
return Menu (pszTitle, pszSubTitle, callback, pnCurItem, nType, nWallpaper, width, height, 0);
}

//------------------------------------------------------------------------------ 

void CMenu::GetMousePos (void)
{
MouseGetPos (&m_xMouse, &m_yMouse);
CRectangle rc;
m_background.CCanvas::GetExtent (rc, true, true);
m_xMouse -= rc.Left ();
m_yMouse -= rc.Top ();
}

//------------------------------------------------------------------------------ 

//returns 1 if a control device button has been pressed
int32_t NMCheckButtonPress (void)
{
switch (gameConfig.nControlType) {
	case CONTROL_JOYSTICK:
	case CONTROL_FLIGHTSTICK_PRO:
	case CONTROL_THRUSTMASTER_FCS:
	case CONTROL_GRAVIS_GAMEPAD:
		for (int32_t i = 0; i < 4; i++)
	 		if (JoyGetButtonDownCnt (i) > 0) 
				return 1;
		break;
	case CONTROL_MOUSE:
	case CONTROL_CYBERMAN:
	case CONTROL_WINJOYSTICK:
		break;
	case	CONTROL_NONE:		//keyboard only
		#ifdef APPLE_DEMO
			if (KeyCheckChar ())	return 1;		
		#endif

		break;
	default:
		Error ("Bad control type (gameConfig.nControlType): %i", gameConfig.nControlType);
	}
return 0;
}

//------------------------------------------------------------------------------ 

extern int32_t bRestoringMenu;

# define JOYDEFS_CALIBRATING 0

void CMenu::DrawCloseBox (int32_t x, int32_t y)
{
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
OglDrawFilledRect (x + MENU_CLOSE_X, y + MENU_CLOSE_Y, x + MENU_CLOSE_X + Scaled (MENU_CLOSE_SIZE), y + MENU_CLOSE_Y + Scaled (MENU_CLOSE_SIZE));
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (21, 21, 21));
OglDrawFilledRect (x + MENU_CLOSE_X + LHX (1), y + MENU_CLOSE_Y + LHX (1), x + MENU_CLOSE_X + Scaled (MENU_CLOSE_SIZE - LHX (1)), y + MENU_CLOSE_Y + Scaled (MENU_CLOSE_SIZE - LHX (1)));
}

//------------------------------------------------------------------------------ 

int32_t CMenu::DrawTitle (const char* pszTitle, CFont* font, uint32_t color, int32_t ty)
{
if (pszTitle && *pszTitle) {
		int32_t x = 0x8000, w, h, aw;

	fontManager.SetCurrent (font);
	fontManager.SetColorRGBi (color, 1, 0, 0);
	fontManager.Current ()->StringSize (pszTitle, w, h, aw);
	GrPrintF (NULL, x, ty, pszTitle);
	ty += h;
	}
return ty;
}

//------------------------------------------------------------------------------ 

void CMenu::GetTitleSize (const char* pszTitle, CFont* font, int32_t& tw, int32_t& th)
{
if (pszTitle && *pszTitle) {
		int32_t nStringWidth, nStringHeight, nAverageWidth;

	fontManager.SetCurrent (font);
	fontManager.Current ()->StringSize (pszTitle, nStringWidth, nStringHeight, nAverageWidth);
	if (nStringWidth > tw)
		tw = nStringWidth;
	th += nStringHeight;
	tw = Scaled (tw);
	th = Scaled (th);
	}
}

//------------------------------------------------------------------------------ 

int32_t CMenu::GetSize (int32_t& w, int32_t& h, int32_t& aw, int32_t& nMenus, int32_t& nOthers)
{
	int32_t	nStringWidth = 0, nStringHeight = 0, nAverageWidth = 0;

#if 0
if ((gameOpts->menus.nHotKeys > 0) && !gameStates.app.bEnglish)
	gameOpts->menus.nHotKeys = -1;
#endif
for (uint32_t i = 0; i < ToS (); i++) {
	int32_t nLineCount = Item (i).GetSize (Scaled (h), aw, nStringWidth, nStringHeight, nAverageWidth, nMenus, nOthers, m_props.bTinyMode);
	if (w < nStringWidth)
		w = nStringWidth;		// Save maximum width
	if (aw < nAverageWidth)
		aw = nAverageWidth;
	h += nStringHeight + 1 + m_props.bTinyMode * nLineCount;		// Find the height of all strings
	}
w = Scaled (w);
h = Scaled (h);
aw = Scaled (aw);
return Scaled (nStringHeight);
}

//------------------------------------------------------------------------------ 

void CMenu::SetItemPos (int32_t titleWidth, int32_t xOffs, int32_t yOffs, int32_t m_rightOffset)
{
for (uint32_t i = 0; i < ToS (); i++) {
	if (((Item (i).m_x == int16_t (0x8000)) || Item (i).m_bCentered)) {
		Item (i).m_bCentered = 1;
		Item (i).m_x = -1;
		}
	else
		Item (i).m_x = xOffs + titleWidth + m_rightOffset;
	Item (i).m_y += yOffs;
	Item (i).m_xSave = Item (i).m_x;
	Item (i).m_ySave = Item (i).m_y;
	if (Item (i).m_nType == NM_TYPE_RADIO) {
		int32_t fm = -1;	// ffs first marked one
		for (uint32_t j = 0; j < ToS (); j++) {
			if ((Item (j).m_nType == NM_TYPE_RADIO) && (Item (j).m_group == Item (i).m_group)) {
				if ((fm == -1) && Item (j).Value ())
					fm = j;
				Item (j).Value () = 0;
				}
			}
		if (fm >= 0)
			Item (fm).Value () = 1;
		else
			Item (i).Value () = 1;
		}
	}
}

//------------------------------------------------------------------------------ 

class CFixFraction {
	fix	m_den, m_div;

	public:
		CFixFraction () : m_den(1), m_div(1) {}

		CFixFraction (fix den, fix div) {
			m_den = den;
			m_div = div;
			}

		inline fix operator* (fix v) { return m_den * v / m_div; }
	};

//------------------------------------------------------------------------------ 

int32_t CMenu::InitProps (const char* pszTitle, const char* pszSubTitle)
{
if ((m_props.screenWidth == gameData.renderData.frame.Width ()) && (m_props.screenHeight == gameData.renderData.frame.Height ()))
	return 0;

	int32_t	i, gap, haveTitle;

m_props.screenWidth = gameData.renderData.frame.Width ();
m_props.screenHeight = gameData.renderData.frame.Height ();
m_props.nDisplayMode = gameStates.video.nDisplayMode;
GetTitleSize (pszTitle, TITLE_FONT, m_props.titleWidth, m_props.titleHeight);
GetTitleSize (pszSubTitle, SUBTITLE_FONT, m_props.titleWidth, m_props.titleHeight);

haveTitle = ((pszTitle && *pszTitle) || (pszSubTitle && *pszSubTitle));
gap = haveTitle ? Scaled (LHY (8)) : 0;
m_props.titleHeight += gap;		//put some space between pszTitles & body
fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);

m_props.width = 0;
m_props.height = int32_t (ceil (double (m_props.titleHeight) / GetScale ())); // will be scaled in GetSize()
m_props.nMenus = m_props.nOthers = 0;
m_props.nStringHeight = GetSize (m_props.width, m_props.height, m_props.aw, m_props.nMenus, m_props.nOthers) + m_props.bTinyMode * 2;

CFixFraction heightScales [2] = { CFixFraction (4, 5), CFixFraction (3, 5) };

m_props.nMaxOnMenu = ((((m_props.screenHeight > 480)) ? heightScales [ogl.IsSideBySideDevice ()] * m_props.screenHeight : 480) - m_props.titleHeight - Scaled (LHY (8))) / m_props.nStringHeight - 2;
if (/*!m_props.bTinyMode && */ (m_props.height > (m_props.nMaxOnMenu * (m_props.nStringHeight + 1)) + gap)) {
 m_props.bIsScrollBox = 1;
 m_props.height = (m_props.nMaxOnMenu * (m_props.nStringHeight + haveTitle) + haveTitle * Scaled (LHY (m_props.bTinyMode ? 12 : 8)));
 }
else
	m_props.bIsScrollBox = 0;
m_props.nMaxDisplayable = (m_props.nMaxOnMenu < int32_t (ToS ())) ? m_props.nMaxOnMenu : int32_t (ToS ());
m_props.rightOffset = 0;
if (m_props.userWidth > -1)
	m_props.width = Scaled (m_props.userWidth);
if (m_props.userHeight > -1)
	m_props.height = Scaled (m_props.userHeight);

for (i = 0; i < int32_t (ToS ()); i++) {
	Item (i).m_w = Unscaled (m_props.width);
	if (Scaled (Item (i).m_rightOffset) > m_props.rightOffset)
		m_props.rightOffset = Scaled (Item (i).m_rightOffset);
	if (Item (i).m_bNoScroll && (i == m_props.nMaxNoScroll))
		m_props.nMaxNoScroll++;
	}
m_props.nScrollOffset = m_props.nMaxNoScroll;
if (m_props.rightOffset > 0)
	m_props.rightOffset += 3;

m_props.width += m_props.rightOffset;
m_props.titlePos = 0;
if (m_props.titleWidth > m_props.width) {
	m_props.titlePos = (m_props.titleWidth - m_props.width) / 2;
	m_props.width = m_props.titleWidth;
}

if (bRestoringMenu) { 
	m_props.rightOffset = 0; 
	m_props.titlePos = 0;
	}

if (m_props.width > m_props.screenWidth)
	m_props.width = m_props.screenWidth;
if (m_props.height > m_props.screenHeight)
	m_props.height = m_props.screenHeight;

m_props.xOffs = (gameStates.menus.bHires ? 30 : 15);
i = (m_props.screenWidth - m_props.width) / 2;
if (i < m_props.xOffs)
	m_props.xOffs = i / 2;
m_props.xOffs = Scaled (m_props.xOffs);
m_props.yOffs = Scaled (gameStates.menus.bHires ? 30 : 15);
if (m_props.screenHeight - m_props.height < 2 * m_props.yOffs)
	m_props.height = m_props.screenHeight - 2 * m_props.yOffs;
m_props.width += 2 * m_props.xOffs;
m_props.height += 2 * m_props.yOffs;
m_props.x = (m_props.screenWidth - m_props.width) / 2;
m_props.y = (m_props.screenHeight - m_props.height) / 2;
m_props.bValid = 1;
SetItemPos (m_props.titlePos, m_props.xOffs, m_props.yOffs, m_props.rightOffset);
return 1;
}

//------------------------------------------------------------------------------ 

void CMenu::SaveScreen (CCanvas **gameCanvasP)
{
gameData.renderData.frame.Activate ("CMenu::SaveScreen (frame)");
}

//------------------------------------------------------------------------------ 

void CMenu::RestoreScreen (void)
{
backgroundManager.Draw ();
//if (gameStates.app.bGameRunning && !gameOpts->menus.nStyle)
//	backgroundManager.Remove ();	// remove the stars background loaded for old style menus
gameData.renderData.frame.Deactivate ();
GrabMouse (1, 0);
}

//------------------------------------------------------------------------------ 

void CMenu::FreeTextBms (void)
{
for (int32_t i = 0; i < int32_t (ToS ()); i++)
	Item (i).FreeTextBms ();
}

//------------------------------------------------------------------------------ 

void CMenu::SwapText (int32_t i, int32_t j)
{
	char h [MENU_MAX_TEXTLEN + 1];

memcpy (h, Item (i).m_text, MENU_MAX_TEXTLEN + 1);
memcpy (Item (i).m_text, Item (j).m_text, MENU_MAX_TEXTLEN + 1);
memcpy (Item (j).m_text, h, MENU_MAX_TEXTLEN + 1);
}

//------------------------------------------------------------------------------ 

void RenderMenuGameFrame (void)
{
if (automap.Active ()) {
	automap.DoFrame (0, 0);
	CalcFrameTime ();
	}
else if (gameData.appData.bGamePaused /*|| timer_paused*/) {
	GameRenderFrame ();
	gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
	CalcFrameTime ();
	}
else {
	GameFrame (1, -1, 40);
	}
}

//------------------------------------------------------------------------------ 

bool BeginRenderMenu (void)
{
if (gameStates.app.bGameRunning && ogl.IsAnaglyphDevice () && (ogl.StereoSeparation () <= 0)) 
	return false;
if (gameStates.app.bGameRunning && (gameStates.render.bRenderIndirect > 0)) {
	ogl.FlushDrawBuffer ();
	if (gameStates.render.bRenderIndirect > 0)
		Draw2DFrameElements ();
	else
		ogl.SetDrawBuffer (GL_BACK, gameStates.render.bRenderIndirect);
	}
return true;
}

//------------------------------------------------------------------------------ 

void CMenu::Render (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
	static	int32_t t0 = 0;

m_props.pszTitle = pszTitle;
m_props.pszSubTitle = pszSubTitle;

if (!MenuRenderTimeout (t0, m_tEnter))
	return;

m_bRedraw = 0;

if (gameStates.app.bGameRunning) {
	if (!gameStates.app.bShowError) {
		gameData.renderData.frame.Activate ("CMenu::Render (frame, 1)");
		RenderMenuGameFrame ();
		gameData.renderData.frame.Deactivate ();
		}
	}
else {
	gameData.renderData.screen.Activate ("CMenu::Render", NULL, true);
	console.Draw ();
	CalcFrameTime ();
	if (!ogl.IsSideBySideDevice ()) {
		ogl.SetDrawBuffer (GL_BACK, 0);
		Render ();
		}
	else {
		for (int32_t i = 0; i < 2; i++) {
			gameData.SetStereoSeparation (i ? STEREO_RIGHT_FRAME : STEREO_LEFT_FRAME);
			SetupCanvasses (-1.0f);
			gameData.renderData.frame.Activate ("CMenu::Render (frame, 2)");
			ogl.ChooseDrawBuffer ();
			Render ();
			gameData.renderData.frame.Deactivate ();
			}
		}
	if (!gameStates.app.bGameRunning || m_bRedraw)
		ogl.Update (0);
	}
}

//------------------------------------------------------------------------------ 

float CMenu::GetScale (void)
{
#if 0 //DBG
return ogl.IsSideBySideDevice () ? 0.5f : 1.0f;
#else
return (ogl.IsOculusRift () && !gameData.renderData.rift.Resolution ()) ? 0.5f : 1.0f;
#endif
}

//------------------------------------------------------------------------------ 

void CMenu::Render (void)
{
if (m_bDone)
	return;

	int32_t y = 0;

ogl.SetDepthTest (false);
ogl.SetDepthMode (GL_ALWAYS);
FadeIn ();
ogl.ColorMask (1,1,1,1,0);
backgroundManager.Activate (m_background);
int32_t nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
fontManager.PushScale ();
fontManager.SetScale (fontManager.Scale (false) * GetScale ());
int32_t i = DrawTitle (m_props.pszTitle, TITLE_FONT, RGB_PAL (31, 31, 31), m_props.yOffs);
DrawTitle (m_props.pszSubTitle, SUBTITLE_FONT, RGB_PAL (21, 21, 21), i);
if (!m_bRedraw)
	m_props.ty = i;

if (m_callback && (gameStates.render.grAlpha == 1.0f))
	m_nChoice = (*m_callback) (*this, m_nKey, m_nChoice, 1);

fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);

for (i = 0; i < m_props.nMaxDisplayable + m_props.nScrollOffset - m_props.nMaxNoScroll; i++) {
	if ((i >= m_props.nMaxNoScroll) && (i < m_props.nScrollOffset))
		continue;
	CMenuItem& item = Item (i);
	if ((item.m_nType == NM_TYPE_TEXT) && !*item.m_text)
		continue;	// skip empty lines
	m_bRedraw = 1;
	if (item.m_bCentered)
		item.m_x = (item.m_nType == NM_TYPE_GAUGE) ? Max (m_props.width - item.m_w, 0) / 2 : fontManager.Current ()->GetCenteredX (item.m_text);
	if (i >= m_props.nScrollOffset) {
		y = item.m_y;
		item.m_y = Item (i - m_props.nScrollOffset + m_props.nMaxNoScroll).m_y;
		}
	item.Draw ((i == m_nChoice) && !m_bAllText, m_props.bTinyMode);
	item.m_bRedraw = 0;
	if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
		SDL_ShowCursor (1);
	if (i >= m_props.nScrollOffset)
		item.m_y = y;
	if ((i == m_nChoice) && 
		 ((item.m_nType == NM_TYPE_INPUT) || 
		 ((item.m_nType == NM_TYPE_INPUT_MENU) && item.m_group)))
		item.UpdateCursor ();
	}
#if 1
if (m_props.bIsScrollBox) {
//fontManager.SetCurrent (NORMAL_FONT);
	if (m_bRedraw || (m_nLastScrollCheck != m_props.nScrollOffset)) {
		m_nLastScrollCheck = m_props.nScrollOffset;
		fontManager.SetCurrent (SELECTED_FONT);
		int32_t sy = Item (m_props.nScrollOffset).m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		int32_t sx = Item (m_props.nScrollOffset).m_x - (gameStates.menus.bHires ? 24 : 12);
		if (m_props.nScrollOffset > m_props.nMaxNoScroll)
			DrawRightStringWXY (Scaled (gameStates.menus.bHires ? 20 : 10), sx, sy, UP_ARROW_MARKER);
		else
			DrawRightStringWXY (Scaled (gameStates.menus.bHires ? 20 : 10), sx, sy, " ");
		i = m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll -1;
		CMenuItem& item = Item (i);
		sy = item.m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		sx = item.m_x - (gameStates.menus.bHires ? 24 : 12);
		if (m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll < int32_t (ToS ()))
			DrawRightStringWXY (Scaled (gameStates.menus.bHires ? 20 : 10), sx, sy, DOWN_ARROW_MARKER);
		else
			DrawRightStringWXY (Scaled (gameStates.menus.bHires ? 20 : 10), sx, sy, " ");
		}
	}
if (m_bCloseBox) {
	DrawCloseBox (0, 0);
	m_bCloseBox = 1;
	}
#endif

m_background.Deactivate ();
fontManager.PopScale ();
m_bRedraw = 1;
m_bStart = 0;
#if 0
if (!m_bDontRestore && paletteManager.EffectDisabled ())
	paletteManager.EnableEffect ();
#endif
gameData.SetStereoOffsetType (nOffsetSave);
gameStates.render.grAlpha = 1.0f;
}

//------------------------------------------------------------------------------ 

bool CMenu::FadeIn (void)
{
if (gameOpts->menus.nFade && !gameStates.app.bNostalgia) {
	if (m_tEnter == uint32_t (-1))
		m_tEnter = SDL_GetTicks ();
	int32_t t = int32_t (SDL_GetTicks () - m_tEnter);
	if (t < 0)
		t = 0;
	gameStates.render.grAlpha = (t < int32_t (gameOpts->menus.nFade)) ? float (t) / float (gameOpts->menus.nFade) : 1.0f;
	return true;
	}
else {
	m_tEnter = 0;
	gameStates.render.grAlpha = 1.0f;
	return false;
	}
}

//------------------------------------------------------------------------------ 

void CMenu::FadeOut (const char* pszTitle, const char* pszSubTitle, CCanvas* gameCanvasP)
{
if (gameOpts->menus.nFade && !gameStates.app.bNostalgia) {
	int32_t t = int32_t (gameOpts->menus.nFade * gameStates.render.grAlpha);
	int32_t t0 = SDL_GetTicks ();
	int32_t t1, dt;
	do {
		t1 = SDL_GetTicks ();
		dt = t1 - t0;
		m_tEnter = t1 - gameOpts->menus.nFade + dt;
		Render (pszTitle, pszSubTitle, gameCanvasP);
		} while (dt <= t);
	}
}

//------------------------------------------------------------------------------ 

#define REDRAW_ALL	for (int32_t _i = 0; _i < int32_t (ToS ()); _i++) Item (_i).m_bRedraw = 1;

void CMenu::KeyScrollUp (void)
{
do {
	m_nChoice--;
 	if (m_props.bIsScrollBox) {
		m_nLastScrollCheck = -1;
		if (m_nChoice < m_nTopChoice) { 
			m_nChoice = int32_t (ToS ()) -1; 
			m_props.nScrollOffset = int32_t (ToS ()) - m_props.nMaxDisplayable + m_props.nMaxNoScroll;
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
		if (m_nChoice >= int32_t (ToS ())) 
			m_nChoice = 0;
		else if (m_nChoice < 0) 
			m_nChoice = int32_t (ToS ()) -1;
		}
	} while (Item (m_nChoice).m_nType == NM_TYPE_TEXT);
if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) && (m_nChoice != m_nOldChoice))
	Item (m_nChoice).Value () = -1;
if ((m_nOldChoice> -1) && (Item (m_nOldChoice).m_nType == NM_TYPE_INPUT_MENU) && (m_nOldChoice != m_nChoice)) {
	Item (m_nOldChoice).m_group = 0;
	strcpy (Item (m_nOldChoice).m_text, Item (m_nOldChoice).m_savedText);
	Item (m_nOldChoice).Value () = -1;
	}
if (m_nOldChoice> -1) 
	Item (m_nOldChoice).m_bRedraw = 1;
Item (m_nChoice).m_bRedraw = 1;
}

//------------------------------------------------------------------------------ 

void CMenu::KeyScrollDown (void)
{
do {
	m_nChoice++;
	if (m_props.bIsScrollBox) {
		m_nLastScrollCheck = -1;
			if (m_nChoice == int32_t (ToS ())) {
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
				m_nChoice = int32_t (ToS ()) -1;
			else if (m_nChoice >= int32_t (ToS ())) 
				m_nChoice = 0;
			}
		} while (Item (m_nChoice).m_nType == NM_TYPE_TEXT);
 
if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) && (m_nChoice != m_nOldChoice))
	Item (m_nChoice).Value () = -1;
if ((m_nOldChoice> -1) && (Item (m_nOldChoice).m_nType == NM_TYPE_INPUT_MENU) && (m_nOldChoice != m_nChoice)) {
	Item (m_nOldChoice).m_group = 0;
	strcpy (Item (m_nOldChoice).m_text, Item (m_nOldChoice).m_savedText);
	Item (m_nOldChoice).Value () = -1;
}
if (m_nOldChoice> -1)
	Item (m_nOldChoice).m_bRedraw = 1;
Item (m_nChoice).m_bRedraw = 1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::KeyScrollValue (void)
{
if ((Item (m_nChoice).m_nType != NM_TYPE_NUMBER) && (Item (m_nChoice).m_nType != NM_TYPE_SLIDER))
	return 0;

int32_t ov = Item (m_nChoice).Value ();
switch (m_nKey) {
	case KEY_PAD4:
	case KEY_LEFT:
	case KEY_MINUS:
	case KEY_MINUS + KEY_SHIFTED:
	case KEY_PADMINUS:
		Item (m_nChoice).Value ()--;
		break;

	case KEY_RIGHT:
	case KEY_PAD6:
	case KEY_EQUALS:
	case KEY_EQUALS + KEY_SHIFTED:
	case KEY_PADPLUS:
		Item (m_nChoice).Value ()++;
		break;

	case KEY_PAGEUP:
	case KEY_PAD9:
	case KEY_SPACEBAR:
		Item (m_nChoice).Value () += 10;
		break;

	case KEY_PAGEDOWN:
	case KEY_BACKSPACE:
	case KEY_PAD3:
		Item (m_nChoice).Value () -= 10;
		break;

	default:
		return 0;

	}
if ((Item (m_nChoice).m_nType == NM_TYPE_SLIDER) || (Item (m_nChoice).m_nType == NM_TYPE_NUMBER))
	Item (m_nChoice).Value () = NMCLAMP (Item (m_nChoice).Value (), Item (m_nChoice).MinValue (), Item (m_nChoice).MaxValue ());
if (ov != Item (m_nChoice).Value ())
	Item (m_nChoice).m_bRedraw = 1;
return 1;
}

//------------------------------------------------------------------------------ 

void CMenu::KeyCheckOption (void)
{
if (m_nChoice > -1) {
	CMenuItem& item = Item (m_nChoice);

	switch (item.m_nType) {
		case NM_TYPE_MENU:
		case NM_TYPE_INPUT:
		case NM_TYPE_INPUT_MENU:
			break;

		case NM_TYPE_CHECK:
			if (item.Value ())
				item.Value () = 0;
			else
				item.Value () = 1;
			if (m_props.bIsScrollBox) {
				if ((m_nChoice == m_props.nMaxOnMenu + m_props.nScrollOffset -1) || (m_nChoice == m_props.nScrollOffset))
					m_nLastScrollCheck = -1;				
					}
			
			item.m_bRedraw = 1;
			break;

		case NM_TYPE_RADIO:
			for (int32_t i = 0; i < int32_t (ToS ()); i++) {
				if ((i != m_nChoice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == item.m_group) && Item (i).Value ()) {
					Item (i).Value () = 0;
					Item (i).m_bRedraw = 1;
					}
				}
			item.Value () = 1;
			item.m_bRedraw = 1;
			break;
			}
	}
}

//------------------------------------------------------------------------------ 

void CMenu::LaunchOption (int32_t* pnCurItem)
{
if (m_nChoice > -1) {
	CMenuItem& item = Item (m_nChoice);

	if ((item.m_nType == NM_TYPE_INPUT_MENU) && (item.m_group == 0)) {
		item.m_group = 1;
		item.m_bRedraw = 1;
		if (!strnicmp (item.m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
			item.m_text [0] = '\0';
			item.Value () = -1;
			}
		else {
			item.TrimWhitespace ();
			}
		}
	else {
		if (pnCurItem)
			*pnCurItem = m_nChoice;
		m_bDone = 1;
		}
	}
}

//------------------------------------------------------------------------------ 

int32_t CMenu::EditValue (void)
{
if (m_nOldChoice != m_nChoice)
	return 0;

CMenuItem& item = Item (m_nChoice);

if ((item.m_nType != NM_TYPE_INPUT) && ((item.m_nType != NM_TYPE_INPUT_MENU) || (item.m_group != 1)))
	return 0;
if ((m_nKey == KEY_LEFT) || (m_nKey == KEY_BACKSPACE) || (m_nKey == KEY_PAD4)) {
	if (item.Value () == -1) 
		item.Value () = (int32_t) strlen (item.m_text);
	if (item.Value () > 0)
		item.Value ()--;
	item.m_text [item.Value ()] = '\0';
	item.m_bRedraw = 1;
	return 1;
	}
else {
	int32_t ascii = KeyToASCII (m_nKey);
	if ((ascii < 255) && (item.Value () < item.m_nTextLen)) {
		if (item.Value ()== -1)
			item.Value () = 0;
		int32_t bAllowed = CharAllowed ((char) ascii);
		if (!bAllowed && ascii == ' ' && CharAllowed ('_')) {
			ascii = '_';
			bAllowed = 1;
			}
		if (bAllowed) {
			item.m_text [item.Value ()++] = ascii;
			item.m_text [item.Value ()] = '\0';
			item.m_bRedraw = 1;
			}
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::QuickSelect (int32_t* pnCurItem)
{
if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) || (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU))
	return 0;

	int32_t ascii = KeyToASCII (m_nKey);

if (ascii < 255) {
	int32_t choice1 = m_nChoice;
	ascii = toupper (ascii);
	do {
		if (++choice1 >= int32_t (ToS ())) 
			choice1 = 0;
		int32_t t = Item (choice1).m_nType;
		int32_t ch;
		if (gameStates.app.bNostalgia)
			ch = Item (choice1).m_text [0];
		else if (Item (choice1).m_nKey > 0)
			ch = MENU_KEY (Item (choice1).m_nKey, 0);
		else if (Item (choice1).m_nKey < 0) //skip any leading blanks
			for (int32_t i = 0; (ch = Item (choice1).m_text [i]) && ch == ' '; i++)
				;
		else
			continue;
				;
		if (ascii != toupper (ch))
			continue;
		if ((t != NM_TYPE_MENU) && (t != NM_TYPE_CHECK) && (t != NM_TYPE_RADIO) && (t != NM_TYPE_NUMBER) && (t != NM_TYPE_SLIDER))
			continue;

		m_nKey = 0;
		m_nChoice = choice1;
		if (m_nOldChoice> -1)
			Item (m_nOldChoice).m_bRedraw = 1;
		Item (m_nChoice).m_bRedraw = 1;
		if (m_nChoice < m_props.nScrollOffset) {
			m_props.nScrollOffset = m_nChoice;
			REDRAW_ALL;
			}
		else if (m_nChoice > m_props.nScrollOffset + m_props.nMaxDisplayable -1) {
			m_props.nScrollOffset = m_nChoice;
			if (m_props.nScrollOffset + m_props.nMaxDisplayable >= int32_t (ToS ())) {
				m_props.nScrollOffset = int32_t (ToS ()) - m_props.nMaxDisplayable;
				if (m_props.nScrollOffset < m_props.nMaxNoScroll)
					m_props.nScrollOffset = m_props.nMaxNoScroll;
				}
			REDRAW_ALL;
			}
		if (t == NM_TYPE_CHECK)
			KeyCheckOption ();
		else if (t == NM_TYPE_RADIO)
			KeyCheckOption ();
		else if ((t != NM_TYPE_SLIDER) && (Item (choice1).m_nKey != 0))
			if (gameOpts->menus.nHotKeys > 0)
				LaunchOption (pnCurItem);
		return 1;

		} while (choice1 != m_nChoice);
	}
return 0;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::HandleKey (int32_t width, int32_t height, int32_t bTinyMode, int32_t* pnCurItem)
{
switch (m_nKey) {

	case KEY_SHIFTED + KEY_ESC:
		console.Show ();
		m_nKey = -1;
		break;

	case KEY_I:
		if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			ShowNetGameInfo (m_nChoice - 2 - tracker.m_bUse);
		if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			m_bDone = 1;
			m_nChoice = -1;
			}
		break;
		 
	case KEY_U:
		if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			NetworkRequestPlayerNames (m_nChoice - 2 - tracker.m_bUse);
		if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			m_bDone = 1;
			m_nChoice = -1;
			}
		break;

	case KEY_CTRLED + KEY_F1:
		SwitchDisplayMode (-1);
		break;

	case KEY_CTRLED + KEY_F2:
		SwitchDisplayMode (1);
		break;

	case KEY_COMMAND + KEY_T:
	case KEY_CTRLED + KEY_T:
		gameData.menuData.alpha = (gameData.menuData.alpha + 16) & 0xFF;
		break;

	case KEY_TAB + KEY_SHIFTED:
	case KEY_UP:
	case KEY_PAD8:
		if (!m_bAllText) 
			KeyScrollUp ();
		break;

		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
	// ((0, "Pressing down! m_props.bIsScrollBox = %d", m_props.bIsScrollBox);
		if (!m_bAllText) 
			KeyScrollDown ();
		break;

	case KEY_SPACEBAR:
		KeyCheckOption ();
		break;

	case KEY_SHIFTED + KEY_UP:
		if (gameStates.menus.bReordering && m_nChoice != m_nTopChoice) {
			SwapText (m_nChoice, m_nChoice -1);
			::Swap (Item (m_nChoice).Value (), Item (m_nChoice -1).Value ());
			Item (m_nChoice).m_bRebuild = 
			Item (m_nChoice -1).m_bRebuild = 1;
			m_nChoice--;
			}
			break;

	case KEY_SHIFTED + KEY_DOWN:
		if (gameStates.menus.bReordering && m_nChoice !=(int32_t (ToS ()) -1)) {
			SwapText (m_nChoice, m_nChoice + 1);
			::Swap (Item (m_nChoice).Value (), Item (m_nChoice + 1).Value ());
			Item (m_nChoice).m_bRebuild = 
			Item (m_nChoice + 1).m_bRebuild = 1;
			m_nChoice++;
			}
			break;

	case KEY_ALTED + KEY_ENTER: {
		//int32_t bLoadCustomBg = NMFreeCustomBg ();
		FreeTextBms ();
		//NMRestoreScreen (filename, & m_bDontRestore);
		GrToggleFullScreenGame ();
		GrabMouse (0, 0);
		SetScreenMode (SCREEN_MENU);
		memset (&m_props, 0, sizeof (m_props));
		m_props.userWidth = width;
		m_props.userHeight = height;
		m_props.bTinyMode = bTinyMode;
		return 1;
		}

	case KEY_ENTER:
	case KEY_PADENTER:
		LaunchOption (pnCurItem);
		break;

	case KEY_COMMAND + KEY_P:
	case KEY_CTRLED + KEY_P:
	case KEY_PAUSE:
		if (m_bPause) 
			m_nKey = KEY_ESC;	// quite pause menu
		else {
			if (bPauseableMenu) {
				bPauseableMenu = 0;
				m_bDone = 1;
				m_nChoice = -1;
				}
			else 
				gameData.appData.bGamePaused = !gameData.appData.bGamePaused;
			break;
			}

	case KEY_ESC:
		if ((m_nChoice > -1) && (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 1)) {
			Item (m_nChoice).m_group = 0;
			strcpy (Item (m_nChoice).m_text, Item (m_nChoice).m_savedText);
			Item (m_nChoice).m_bRedraw = 1;
			Item (m_nChoice).Value () = -1;
			}
		else {
			m_bDone = 1;
			if (pnCurItem)
				*pnCurItem = m_nChoice;
			m_nChoice = -1;
			}
		break;

	case KEY_COMMAND + KEY_SHIFTED + KEY_P:
	case KEY_PRINT_SCREEN:
		gameStates.app.bSaveScreenShot = 1;
		SaveScreenShot (NULL, 0);
		REDRAW_ALL
		break;

#if DBG
	case KEY_BACKSPACE:
		if ((m_nChoice > -1) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT_MENU))
			Int3 (); 
		break;
#endif

	case KEY_F1:
		Item (m_nChoice).ShowHelp ();
		break;
	}
return 0;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::FindClickedOption (int32_t iMin, int32_t iMax)
{
	int32_t i, x1, y1, x2, y2;

GetMousePos ();
for (i = iMin; i < iMax; i++) {
	if (!Item (i).Selectable ())
		continue;
	x1 = Item (i).m_x - Item (i).m_rightOffset - 6;
	x2 = x1 + Item (i).m_w;
	y1 = Item (i).m_y;
	y2 = y1 + Item (i).m_h;
	if ((m_xMouse > x1) && (m_xMouse < x2) && (m_yMouse > y1) && (m_yMouse < y2))
		return i;
	}
return -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::SelectClickedOption (void)
{
int32_t i = FindClickedOption (0, int32_t (ToS ()));
if (i < 0)
	return 0;
i += m_props.nScrollOffset - m_props.nMaxNoScroll;
if ((i < 0) || (i >= int32_t (ToS ())))
	return 0;
return m_nChoice = i;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::MouseCheckOption (void)
{
if (0 > SelectClickedOption ())
	return 0;

int32_t i;

switch (Item (m_nChoice).m_nType) {
	case NM_TYPE_CHECK:
		Item (m_nChoice).Value () = !Item (m_nChoice).Value ();
		Item (m_nChoice).m_bRedraw = 1;
		if (m_props.bIsScrollBox)
			m_nLastScrollCheck = -1;
		break;

	case NM_TYPE_RADIO:
		for (i = 0; i < int32_t (ToS ()); i++) {
			if ((i != m_nChoice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (m_nChoice).m_group) && (Item (i).Value ())) {
				Item (i).Value () = 0;
				Item (i).m_bRedraw = 1;
				}
			}
		Item (m_nChoice).Value () = 1;
		break;

	default:
		return 0;
	}
Item (m_nOldChoice).m_bRedraw = 1;
return 1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::MouseScrollUp (void)
{
	static CTimeout to (100);

if (m_props.bIsScrollBox) {
	if (m_props.nScrollOffset > m_props.nMaxNoScroll) {
		int32_t arrowWidth, arrowHeight, aw;
		fontManager.Current ()->StringSize (UP_ARROW_MARKER, arrowWidth, arrowHeight, aw);
		int32_t x2 = Item (m_props.nScrollOffset).m_x - (gameStates.menus.bHires ? 24 : 12);
		int32_t y1 = Item (m_props.nScrollOffset).m_y - ((m_props.nStringHeight + 1)*(m_props.nScrollOffset - m_props.nMaxNoScroll));
		int32_t x1 = x2 - arrowWidth;
		int32_t y2 = y1 + arrowHeight;
		if (((m_xMouse > x1) && (m_xMouse < x2)) && ((m_yMouse > y1) && (m_yMouse < y2))) {
			if (!to.Expired ())
				return 1;
			m_nChoice--;
			m_nLastScrollCheck = -1;
			if (m_nChoice < m_props.nScrollOffset) {
				REDRAW_ALL;
				m_props.nScrollOffset--;
				}
			return 1;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::MouseScrollDown (void)
{
	static CTimeout to (100);

if (m_props.bIsScrollBox) {
	int32_t i = m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll;
			
	if (i < int32_t (ToS ()) && Item (i).Selectable ()) {
		int32_t arrowWidth, arrowHeight, aw;
		fontManager.Current ()->StringSize (DOWN_ARROW_MARKER, arrowWidth, arrowHeight, aw);
		int32_t x2 = Item (i -1).m_x - (gameStates.menus.bHires ? 24 : 12);
		int32_t y1 = Item (i -1).m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		int32_t x1 = x2 - arrowWidth;
		int32_t y2 = y1 + arrowHeight;
		if ((m_xMouse > x1) && (m_xMouse < x2) && (m_yMouse > y1) && (m_yMouse < y2)) {
			if (!to.Expired ())
				return 1;
			m_nChoice++;
			m_nLastScrollCheck = -1;
			if (m_nChoice >= m_props.nMaxOnMenu + m_props.nScrollOffset) {
				REDRAW_ALL;
				m_props.nScrollOffset++;
				}
			return 1;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::MouseScrollValue (void)
{
if (0 > SelectClickedOption ())
	return 0;

CMenuItem& item = Item (m_nChoice);

if (item.m_nType == NM_TYPE_SLIDER) {
	int32_t lw, mw, rw, height, aw;
					
	fontManager.Current ()->StringSize (SLIDER_LEFT, lw, height, aw);
	fontManager.Current ()->StringSize (SLIDER_MIDDLE, mw, height, aw);
	fontManager.Current ()->StringSize (SLIDER_RIGHT, rw, height, aw);

	int32_t w = lw + mw * item.Range ();
	int32_t x = m_xMouse - item.m_xSlider;
	int32_t i;

	if ((x >= 0) && (x < lw))
		i = 0;
	else if ((x >= w) && (x < w + rw))
		i = item.Range () - 1;
	else if ((x >= lw) && (x < w)) 
		i = (x - lw) * item.Range () / (w - lw);
	else
		i = item.Value ();
	if (i != item.Value ()) {
		item.Value () = i;
		item.m_bRedraw = 2;
		}
	}

if (m_nChoice != m_nOldChoice) {
	if ((item.m_nType == NM_TYPE_INPUT) && (m_nChoice != m_nOldChoice))
		item.Value () = -1;
	if ((m_nOldChoice> -1) && (Item (m_nOldChoice).m_nType == NM_TYPE_INPUT_MENU) && (m_nOldChoice != m_nChoice)) {
		Item (m_nOldChoice).m_group = 0;
		strcpy (Item (m_nOldChoice).m_text, Item (m_nOldChoice).m_savedText);
		Item (m_nOldChoice).Value () = -1;
		}
	if (m_nOldChoice> -1) 
		Item (m_nOldChoice).m_bRedraw = 1;
	}
item.m_bRedraw = 1;
return 1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::HandleMouse (int32_t* pnCurItem)
{
if (m_bAllText) {
	if (!m_nMouseState) 
		return 0;
	if (pnCurItem)
		*pnCurItem = m_nChoice;
	m_bDone = 1;
	}

if (m_bDone)
	return 0;

GetMousePos ();

if (m_nMouseState) {
	if (!m_nOldMouseState)
		return MouseCheckOption ();
	if (MouseScrollUp ())
		return 1;
	if (MouseScrollDown ())
		return 1;
	if (MouseScrollValue ())
		return 1;
	return 0;
	}
	
if (!m_nOldMouseState)
	return 0;

if (m_bDone)
	return 0;

if (m_bCloseBox) {
	GetMousePos ();
	int32_t x1 = MENU_CLOSE_X;
	int32_t x2 = x1 + MENU_CLOSE_SIZE;
	int32_t y1 = MENU_CLOSE_Y;
	int32_t y2 = y1 + MENU_CLOSE_SIZE;
	if (((m_xMouse > x1) && (m_xMouse < x2)) && ((m_yMouse > y1) && (m_yMouse < y2))) {
		if (pnCurItem)
			*pnCurItem = m_nChoice;
		m_nChoice = -1;
		m_bDone = 1;
		}
	}

if (0 > SelectClickedOption ())
	return 0;

CMenuItem& item = Item (m_nChoice);

if (item.m_nType == NM_TYPE_INPUT_MENU) {
	if (pnCurItem)
		*pnCurItem = m_nChoice;
	m_bDone = 1;
	return 0;
	}

if (item.m_nType == NM_TYPE_MENU) {
	if (pnCurItem)
		*pnCurItem = m_nChoice;
	LaunchOption (pnCurItem);
	return 0;
	}

if (item.m_group == 0) {
	item.m_group = 1;
	item.m_bRedraw = 1;
	if	(strnicmp (item.m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) 
		item.TrimWhitespace ();
	else {
		item.m_text [0] = '\0';
		item.Value () = -1;
		} 
	}

return 0;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::Setup (int32_t nType, int32_t width, int32_t height, int32_t bTinyMode, pMenuCallback callback, int32_t& nItem)
{
if (gameStates.menus.nInMenu > 0)
	return 0;

m_bPause = gameData.appData.bGamePaused;
m_tEnter = -1;
m_bRedraw = 0;
m_nChoice = 0;
m_nLastScrollCheck = -1;
m_bStart = 1;
m_bCloseBox = 0;
m_bDontRestore = 0;
m_bDblClick = 0;
m_bAllText = 0;
m_callback = callback;

messageBox.Clear ();
memset (&m_props, 0, sizeof (m_props));
m_props.userWidth = Scaled (width);
m_props.userHeight = Scaled (height);
m_props.bTinyMode = bTinyMode;
controls.FlushInput ();

if (int32_t (ToS ()) < 1)
	return 0;
//if (gameStates.app.bGameRunning && !gameOpts->menus.nStyle)
//	backgroundManager.LoadStars (true);
SDL_ShowCursor (0);
SDL_EnableKeyRepeat (60, 30);
if (gameStates.menus.nInMenu >= 0)
	gameStates.menus.nInMenu++;
if (gameStates.app.bGameRunning && IsMultiGame)
	gameData.multigame.nTypingTimeout = 0;

SetPopupScreenMode ();
SetupCanvasses (-1.0f);

if (nItem == -1)
	m_nChoice = -1;
else {
	if (nItem < 0) 
		nItem = 0;
	else 
		nItem %= int32_t (ToS ());
	m_nChoice = nItem;
	m_bDblClick = 1;
	while (Item (m_nChoice).m_nType == NM_TYPE_TEXT) {
		m_nChoice++;
		if (m_nChoice >= int32_t (ToS ()))
			m_nChoice = 0; 
		if (m_nChoice == nItem) {
			m_nChoice = 0; 
			m_bAllText = 1;
			break; 
			}
		}
	} 


paletteManager.DisableEffect ();
m_bDone = 0;
m_nTopChoice = 0;

Register ();

while (Item (m_nTopChoice).m_nType == NM_TYPE_TEXT) {
	m_nTopChoice++;
	if (m_nTopChoice >= int32_t (ToS ()))
		m_nTopChoice = 0; 
	if (m_nTopChoice == nItem) {
		m_nTopChoice = 0; 
		break; 
		}
	}

//ogl.Update (0);
// Clear mouse, joystick to clear button presses.
GameFlushInputs ();
m_nMouseState = m_nOldMouseState = 0;
m_bCloseBox = (nType != BG_TOPMENU) && !gameStates.menus.bReordering;

if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING) {
	SDL_ShowCursor (1);
	}
GrabMouse (0, 0);
return 1;
}

//------------------------------------------------------------------------------ 

void CMenu::Update (const char* pszTitle, const char* pszSubTitle, int32_t nType, int32_t nWallpaper, int32_t width, int32_t height, int32_t bTinyMode, int32_t* pnCurItem)
{
if (m_bThrottle)
	m_to.Throttle ();	// give the CPU some time to breathe

if (pnCurItem)
	*pnCurItem = m_nChoice;
if (gameStates.app.bGameRunning && IsMultiGame) {
	gameStates.multi.bPlayerIsTyping [N_LOCALPLAYER] = 1;
	MultiSendTyping ();
	}
if (!JOYDEFS_CALIBRATING)
	SDL_ShowCursor (1); // possibly hidden

#if 1
m_nOldMouseState = m_nMouseState;
if (!gameStates.menus.bReordering) {
	int32_t b = gameOpts->legacy.bInput;
	gameOpts->legacy.bInput = 1;
	m_nMouseState = MouseButtonState (0);
	gameOpts->legacy.bInput = b;
	}
m_bWheelUp = MouseButtonState (3);
m_bWheelDown = MouseButtonState (4);
//see if redbook song needs to be restarted
redbook.CheckRepeat ();
if (m_bWheelUp)
	m_nKey = KEY_UP;
else if (m_bWheelDown)
	m_nKey = KEY_DOWN;
else
	m_nKey = KeyInKey ();
#endif

#if DBG
if (m_nKey)
	BRP;
#endif
if (mouseData.bDoubleClick)
	m_nKey = KEY_ENTER;
if ((m_props.screenWidth != gameData.renderData.frame.Width ()) || (m_props.screenHeight != gameData.renderData.frame.Height ())) {
	memset (&m_props, 0, sizeof (m_props));
	m_props.userWidth = width;
	m_props.userHeight = height;
	m_props.bTinyMode = bTinyMode;
	}

if (m_props.bValid && (m_props.nDisplayMode != gameStates.video.nDisplayMode)) {
	FreeTextBms ();
	SetScreenMode (SCREEN_MENU);
	memset (&m_props, 0, sizeof (m_props));
	m_props.userWidth = width;
	m_props.userHeight = height;
	m_props.bTinyMode = bTinyMode;
	}

if (InitProps (pszTitle, pszSubTitle)) {
	backgroundManager.Setup (m_background, m_props.width, m_props.height, nType, nWallpaper);
	m_bRedraw = 0;
	m_props.ty = m_props.yOffs;
	if (m_nChoice > m_props.nScrollOffset + m_props.nMaxOnMenu -1)
		m_props.nScrollOffset = m_nChoice - m_props.nMaxOnMenu + 1;
	}
}

//------------------------------------------------------------------------------ 

int32_t CMenu::Menu (const char* pszTitle, const char* pszSubTitle, pMenuCallback callback, int32_t* pnCurItem, 
							int32_t nType, int32_t nWallpaper, int32_t width, int32_t height, int32_t bTinyMode)
{
	int32_t			nItem = pnCurItem ? *pnCurItem : 0;
	int32_t			bSoundStopped = 0, bTimeStopped = 0;
	int32_t			exception = 0;
	int32_t			i;
	
if (!Setup (nType, width, height, bTinyMode, callback, nItem))
	return -1;

int32_t bKeyRepeat = gameStates.input.keys.bRepeat;
gameStates.input.keys.bRepeat = 1;

while (!m_bDone) {
	Update (pszTitle, pszSubTitle, nType, nWallpaper, width, height, bTinyMode, pnCurItem);
	// Redraw everything...
	Render (pszTitle, pszSubTitle, NULL);
	if (callback && (SDL_GetTicks () - m_tEnter > gameOpts->menus.nFade))
		try {
			m_nChoice = (*callback) (*this, m_nKey, m_nChoice, 0);
			}
		catch (int32_t e) {
			exception = e;
			m_bDone = 1;
			break;
			}

#if 1
	if (!bTimeStopped) {
		// Save current menu box
		if (MultiMenuPoll () == -1)
			m_nKey = -2;
		}

	if (m_nKey < -1) {
		m_bDontRestore = (m_nKey == -3);		// - 3 means don't restore
		if (m_nChoice < 0) {
			m_nChoice = -m_nChoice - 1;
			if (pnCurItem)
				*pnCurItem = m_nChoice;
			}
		m_nChoice = m_nKey;
		m_nKey = -1;
		m_bDone = 1;
		}

	m_nOldChoice = m_nChoice;
	if (m_nKey && (console.Events (m_nKey) || m_bWheelUp || m_bWheelDown))
		if (HandleKey (width, height, bTinyMode, pnCurItem))
			continue;

	if (HandleMouse (pnCurItem))
		continue;

	//	 HACK! Don't redraw loadgame preview
	if (bRestoringMenu) 
		Item (0).m_bRedraw = 0;

	if (m_nChoice > -1) {
		if (!QuickSelect (pnCurItem) && !EditValue ())
			KeyScrollValue ();
		}
#endif
	}
FadeOut (pszTitle, pszSubTitle, NULL);
SDL_ShowCursor (0);
// Restore everything...
RestoreScreen ();
FreeTextBms ();
gameStates.input.keys.bRepeat = bKeyRepeat;
GameFlushInputs ();
if (bTimeStopped) {
	StartTime (0);
#ifdef FORCE_FEEDBACK
		if (TactileStick)
			EnableForces ();
#endif
 }
if (bSoundStopped)
	audio.ResumeSounds ();
if (gameStates.menus.nInMenu > 0)
	gameStates.menus.nInMenu--;
paletteManager.EnableEffect ();
//paletteManager.StopEffect ();
SDL_EnableKeyRepeat (0, 0);
if (gameStates.app.bGameRunning && IsMultiGame)
	MultiSendMsgQuit();

for (i = 0; i < int32_t (ToS ()); i++)
	Item (i).GetInput ();

if (exception)
	throw (exception);
Unregister ();
return m_nChoice;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 

void CMenu::SetId (CMenuItem& item, const char* szId)
{
if (szId && *szId && IndexOf (szId, false) > 0)
	PrintLog (0, "duplicate menu id '%s'\n", szId);
item.SetId (szId);
}

//------------------------------------------------------------------------------ 

CMenuItem *CMenu::AddItem (void)
{
CMenuItem *pBuffer = Buffer (); 
CMenuItem *pItem = Grow (1) ? Top () : NULL;
if (pBuffer != Buffer ()) // reallocated?
	m_current = NULL;
return pItem;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddCheck (const char* szId, const char* szText, int32_t nValue, int32_t nKey, const char* szHelp)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_CHECK;
pItem->m_pszText = NULL;
pItem->SetText (szText);
pItem->Value () = NMBOOL (nValue);
pItem->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int32_t (*(szText -1));
pItem->m_szHelp = szHelp;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddRadio (const char* szId, const char* szText, int32_t nValue, int32_t nKey, const char* szHelp)
{
if (!ToS () || (Top ()->m_nType != NM_TYPE_RADIO))
	NewGroup ();
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_RADIO;
pItem->m_pszText = NULL;
pItem->SetText (szText);
pItem->Value () = nValue;
pItem->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int32_t (*(szText -1));
pItem->m_group = m_nGroup;
pItem->m_szHelp = szHelp;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddMenu (const char* szId, const char* szText, int32_t nKey, const char* szHelp)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_MENU;
pItem->m_pszText = NULL;
pItem->SetText (szText);
pItem->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int32_t (*(szText -1));
pItem->m_szHelp = szHelp;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddText (const char* szId, const char* szText, int32_t nKey)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_TEXT;
pItem->m_pszText = NULL;
pItem->SetText (szText);
pItem->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int32_t (*(szText -1));
SetId (*pItem, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddSlider (const char* szId, const char* szText, int32_t nValue, int32_t nMin, int32_t nMax, int32_t nKey, const char* szHelp)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_SLIDER;
pItem->m_pszText = NULL;
pItem->SetText (szText);
pItem->Value () = NMCLAMP (nValue, nMin, nMax);
pItem->MinValue () = nMin;
pItem->MaxValue () = nMax;
pItem->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int32_t (*(szText -1));
pItem->m_szHelp = szHelp;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddInput (const char* szId, const char* szText, int32_t nLen, const char* szHelp)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_INPUT;
pItem->m_pszText = const_cast<char*> (szText);
pItem->SetText (szText);
pItem->m_nTextLen = nLen;
pItem->m_szHelp = szHelp;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddInput (const char* szId, const char* szText, char* szValue, int32_t nLen, const char* szHelp)
{
if (AddText ("", szText, -1) < 0)
	return -1;
return AddInput (szId, szValue, nLen, szHelp);
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddInput (const char* szId, const char* szText, char* szValue, int32_t nValue, int32_t nLen, const char* szHelp)
{
sprintf (szValue, "%d", nValue);
return AddInput (szId, szText, szValue, nLen, szHelp);
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddInputBox (const char* szId, const char* szText, int32_t nLen, int32_t nKey, const char* szHelp)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_INPUT_MENU;
pItem->m_pszText = (char*) (szText);
pItem->SetText (szText);
pItem->m_nTextLen = nLen;
pItem->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int32_t (*(szText -1));
pItem->m_szHelp = szHelp;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddNumber (const char* szId, const char* szText, int32_t nValue, int32_t nMin, int32_t nMax)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_NUMBER;
pItem->m_pszText = (char*) (szText);
pItem->SetText (szText);
pItem->Value () = NMCLAMP (nValue, nMin, nMax);
pItem->MinValue () = nMin;
pItem->MaxValue () = nMax;
SetId (*pItem, szId);
return ToS () -1;
}

//------------------------------------------------------------------------------ 

int32_t CMenu::AddGauge (const char* szId, const char* szText, int32_t nValue, int32_t nMax)
{
CMenuItem* pItem = AddItem ();
if (!pItem)
	return -1;
pItem->m_nType = NM_TYPE_GAUGE;
pItem->m_pszText = NULL;
pItem->SetText (szText);
pItem->m_nTextLen = *szText ? (int32_t) strlen (szText) : 20;
pItem->Value () = NMCLAMP (nValue, 0, nMax);
pItem->MaxValue () = nMax;
SetId (*pItem, szId);
m_bThrottle = false;
return ToS () -1;
}

//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
/* Spiffy word wrap string formatting function */

void NMWrapText (char* dbuf, char* sbuf, int32_t line_length)
{
	int32_t col;
	char* wordptr;
	char* tbuf;

	tbuf = NEW char [strlen (sbuf) + 1];
	strcpy (tbuf, sbuf);

	wordptr = strtok (tbuf, " ");
	if (!wordptr) return;
	col = 0;
	dbuf [0] = 0;

while (wordptr) {
	col = col + (int32_t) strlen (wordptr) + 1;
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

void ProgressBar (const char* szCaption, int32_t nBars, int32_t nCurProgress, int32_t nMaxProgress, pMenuCallback doProgress)
{
	int32_t	i = 0, nInMenu;
	CMenu		menu;

nBars = Clamp (nBars, 1, 2);
menu.Create (3 + (nBars - 1) * 3 + (doProgress ? doProgress (menu, i, i, -1) : 0), "ProgressBar"); // doProgress can add menu items - ask it how many it will add

if (doProgress) { // let doProgress add menu items before the gauges
	i = 0;
	doProgress (menu, i, i, -2);
	}


for (i = 0; i < nBars; i++) {
	char szId [20];
	if (i) {
		menu.AddText ("", "");
		sprintf (szId, "subtitle %d", i);
		int32_t h = menu.AddText (szId, "");
		menu [h].m_bCentered = 1;
		}
	sprintf (szId, "progress bar %d", i + 1);
	int32_t h = menu.AddGauge (szId, "                    ", -1, nMaxProgress); //the blank string denotes the screen width of the gauge
	menu [h].m_bCentered = 1;
	menu [h].Value () = i ? 0 : nCurProgress;
	}

if (doProgress) { // let doProgress add menu items after the gauges
	i = 0;
	doProgress (menu, i, i, -3);
	}

nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameData.appData.bGamePaused = 1;
do {
	i = menu.Menu (NULL, szCaption, doProgress);
	} while (i >= 0);
gameData.appData.bGamePaused = 0;
gameStates.menus.nInMenu = nInMenu;
}

//------------------------------------------------------------------------------
//eof
