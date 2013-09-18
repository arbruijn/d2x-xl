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
int CMenu::m_level = 0;


//------------------------------------------------------------------------------

bool MenuRenderTimeout (int& t0, int tFade)
{
int i = SDL_GetTicks ();

if ((tFade < 0) || (i - tFade > int (gameOpts->menus.nFade))) {
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
void CMenu::DrawRightStringWXY (int w1, int x, int y, const char* s)
{
	int	w, h, aw;

fontManager.Current ()->StringSize (s, w, h, aw);
x -= 3;
if (w1 == 0) 
	w1 = w;
if (RETRO_STYLE)
	backgroundManager.Current ()->BlitClipped (CCanvas::Current (), x - w1, y, w1, h, x - w1, y);
GrString (x - w, y, s);
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
switch (gameConfig.nControlType) {
	case CONTROL_JOYSTICK:
	case CONTROL_FLIGHTSTICK_PRO:
	case CONTROL_THRUSTMASTER_FCS:
	case CONTROL_GRAVIS_GAMEPAD:
		for (int i = 0; i < 4; i++)
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
x -= gameData.StereoOffset2D ();
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
OglDrawFilledRect (x + MENU_CLOSE_X, y + MENU_CLOSE_Y, x + MENU_CLOSE_X + Scale (MENU_CLOSE_SIZE), y + MENU_CLOSE_Y + Scale (MENU_CLOSE_SIZE));
CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (21, 21, 21));
OglDrawFilledRect (x + MENU_CLOSE_X + LHX (1), y + MENU_CLOSE_Y + LHX (1), x + MENU_CLOSE_X + Scale (MENU_CLOSE_SIZE - LHX (1)), y + MENU_CLOSE_Y + Scale (MENU_CLOSE_SIZE - LHX (1)));
}

//------------------------------------------------------------------------------ 

int CMenu::DrawTitle (const char* pszTitle, CFont* font, uint color, int ty)
{
if (pszTitle && *pszTitle) {
		int x = 0x8000, w, h, aw;

	fontManager.SetCurrent (font);
	fontManager.SetColorRGBi (color, 1, 0, 0);
	fontManager.Current ()->StringSize (pszTitle, w, h, aw);
	GrPrintF (NULL, x, ty, pszTitle);
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
	tw = Scale (tw);
	th = Scale (th);
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
	Item (i).GetSize (Scale (h), aw, nStringWidth, nStringHeight, nAverageWidth, nMenus, nOthers, m_props.bTinyMode);
	if (w < nStringWidth)
		w = nStringWidth;		// Save maximum width
	if (aw < nAverageWidth)
		aw = nAverageWidth;
	h += nStringHeight + 1 + m_props.bTinyMode;		// Find the height of all strings
	}
w = Scale (w);
h = Scale (h);
aw = Scale (aw);
return Scale (nStringHeight);
}

//------------------------------------------------------------------------------ 

void CMenu::SetItemPos (int twidth, int xOffs, int yOffs, int m_rightOffset)
{
for (uint i = 0; i < ToS (); i++) {
	if (((Item (i).m_x == short (0x8000)) || Item (i).m_bCentered)) {
		Item (i).m_bCentered = 1;
		Item (i).m_x = fontManager.Current ()->GetCenteredX (Item (i).m_text, GetScale ());
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

int CMenu::InitProps (const char* pszTitle, const char* pszSubTitle)
{
if ((m_props.screenWidth == gameData.render.frame.Width ()) && (m_props.screenHeight == gameData.render.frame.Height ()))
	return 0;

	int	i, gap, haveTitle;

m_props.screenWidth = gameData.render.frame.Width ();
m_props.screenHeight = gameData.render.frame.Height ();
m_props.nDisplayMode = gameStates.video.nDisplayMode;
GetTitleSize (pszTitle, TITLE_FONT, m_props.titleWidth, m_props.titleHeight);
GetTitleSize (pszSubTitle, SUBTITLE_FONT, m_props.titleWidth, m_props.titleHeight);

haveTitle = ((pszTitle && *pszTitle) || (pszSubTitle && *pszSubTitle));
gap = haveTitle ? Scale (LHY (8)) : 0;
m_props.titleHeight += gap;		//put some space between pszTitles & body
fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);

m_props.width = 0;
m_props.height = int (ceil (double (m_props.titleHeight) / GetScale ())); // will be scaled in GetSize()
m_props.nMenus = m_props.nOthers = 0;
m_props.nStringHeight = GetSize (m_props.width, m_props.height, m_props.aw, m_props.nMenus, m_props.nOthers) + m_props.bTinyMode * 2;

CFixFraction heightScales [2] = { CFixFraction (4, 5), CFixFraction (3, 5) };

m_props.nMaxOnMenu = ((((m_props.screenHeight > 480)) ? heightScales [ogl.IsSideBySideDevice ()] * m_props.screenHeight : 480) - m_props.titleHeight - Scale (LHY (8))) / m_props.nStringHeight - 2;
if (/*!m_props.bTinyMode && */ (m_props.height > (m_props.nMaxOnMenu * (m_props.nStringHeight + 1)) + gap)) {
 m_props.bIsScrollBox = 1;
 m_props.height = (m_props.nMaxOnMenu * (m_props.nStringHeight + haveTitle) + haveTitle * Scale (LHY (m_props.bTinyMode ? 12 : 8)));
 }
else
	m_props.bIsScrollBox = 0;
m_props.nMaxDisplayable = (m_props.nMaxOnMenu < int (ToS ())) ? m_props.nMaxOnMenu : int (ToS ());
m_props.rightOffset = 0;
if (m_props.userWidth > - 1)
	m_props.width = Scale (m_props.userWidth);
if (m_props.userHeight > - 1)
	m_props.height = Scale (m_props.userHeight);

for (i = 0; i < int (ToS ()); i++) {
	Item (i).m_w = Unscale (m_props.width);
	if (Scale (Item (i).m_rightOffset) > m_props.rightOffset)
		m_props.rightOffset = Scale (Item (i).m_rightOffset);
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
m_props.xOffs = Scale (m_props.xOffs);
m_props.yOffs = Scale (gameStates.menus.bHires ? 30 : 15);
if (m_props.screenHeight - m_props.height < 2 * m_props.yOffs)
	m_props.height = m_props.screenHeight - 2 * m_props.yOffs;
m_props.width += 2 * m_props.xOffs;
m_props.height += 2 * m_props.yOffs;
m_props.x = (m_props.screenWidth - m_props.width) / 2;
m_props.y = (m_props.screenHeight - m_props.height) / 2;
m_props.xOffs += m_props.x;
m_props.yOffs += m_props.y;
m_props.bValid = 1;
SetItemPos (m_props.titlePos, m_props.xOffs, m_props.yOffs, m_props.rightOffset);
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
	Item (i).FreeTextBms ();
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

void RenderMenuGameFrame (void)
{
if (automap.Display ()) {
	automap.DoFrame (0, 0);
	CalcFrameTime ();
	}
else if (gameData.app.bGamePaused /*|| timer_paused*/) {
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
	static	int t0 = 0;

if (!MenuRenderTimeout (t0, m_tEnter))
	return;

m_bRedraw = 0;

m_props.pszTitle = pszTitle;
m_props.pszSubTitle = pszSubTitle;
m_props.gameCanvasP = gameCanvasP;
CCanvas::Push ();
if (gameStates.app.bGameRunning && gameCanvasP) {
	CCanvas::SetCurrent (gameCanvasP);
	if (!gameStates.app.bShowError)
		RenderMenuGameFrame ();
	}
else {
	SetupCanvasses ();
	console.Draw ();
	CalcFrameTime ();
	if (!ogl.IsSideBySideDevice ()) {
		ogl.SetDrawBuffer (GL_BACK, 0);
		Render ();
		}
	else {
		for (int i = 0; i < 2; i++) {
			ogl.SetStereoSeparation (i ? gameOpts->render.stereo.xSeparation [ogl.IsOculusRift ()] : -gameOpts->render.stereo.xSeparation [ogl.IsOculusRift ()]);
			ogl.ChooseDrawBuffer ();
			Render ();
			}
		}
	if (!gameStates.app.bGameRunning || m_bRedraw)
		ogl.Update (0);
	}
CCanvas::Pop ();
}

//------------------------------------------------------------------------------ 

float CMenu::GetScale (void)
{
#if 0 //DBG
return ogl.IsSideBySideDevice () ? 0.5f : 1.0f;
#else
return (ogl.IsOculusRift () /*&& gameStates.app.bGameRunning*/) ? 0.5f : 1.0f;
#endif
}

//------------------------------------------------------------------------------ 

void CMenu::Render (void)
{
if (m_bDone)
	return;

	int x = 0, y = 0;

ogl.SetDepthTest (false);
FadeIn ();
ogl.ColorMask (1,1,1,1,0);
backgroundManager.Redraw ();
gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
fontManager.SetScale (fontManager.Scale () * GetScale ());
int i = DrawTitle (m_props.pszTitle, TITLE_FONT, RGB_PAL (31, 31, 31), m_props.yOffs);
DrawTitle (m_props.pszSubTitle, SUBTITLE_FONT, RGB_PAL (21, 21, 21), i);
if (!m_bRedraw)
	m_props.ty = i;

if (m_callback && (gameStates.render.grAlpha == 1.0f))
	m_nChoice = (*m_callback) (*this, m_nKey, m_nChoice, 1);

fontManager.SetCurrent (m_props.bTinyMode ? SMALL_FONT : NORMAL_FONT);
for (i = 0; i < m_props.nMaxDisplayable + m_props.nScrollOffset - m_props.nMaxNoScroll; i++) {
	if ((i >= m_props.nMaxNoScroll) && (i < m_props.nScrollOffset))
		continue;
	if ((Item (i).m_nType == NM_TYPE_TEXT) && !*Item (i).m_text)
		continue;	// skip empty lines
	m_bRedraw = 1;
	if (Item (i).m_bRebuild && Item (i).m_bCentered)
		Item (i).m_x = fontManager.Current ()->GetCenteredX (Item (i).m_text);
	if (i >= m_props.nScrollOffset) {
		y = Item (i).m_y;
		Item (i).m_y = Item (i - m_props.nScrollOffset + m_props.nMaxNoScroll).m_y;
		}
	x = Item (i).m_x;
	//Item (i).m_x -= gameData.StereoOffset2D ();
	Item (i).Draw ((i == m_nChoice) && !m_bAllText, m_props.bTinyMode);
	Item (i).m_bRedraw = 0;
	Item (i).m_x = x;
	if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
		SDL_ShowCursor (1);
	if (i >= m_props.nScrollOffset)
		Item (i).m_y = y;
	if ((i == m_nChoice) && 
		 ((Item (i).m_nType == NM_TYPE_INPUT) || 
		 ((Item (i).m_nType == NM_TYPE_INPUT_MENU) && Item (i).m_group)))
		Item (i).UpdateCursor ();
	}
#if 1
if (m_props.bIsScrollBox) {
//fontManager.SetCurrent (NORMAL_FONT);
	if (m_bRedraw || (m_nLastScrollCheck != m_props.nScrollOffset)) {
		m_nLastScrollCheck = m_props.nScrollOffset;
		fontManager.SetCurrent (SELECTED_FONT);
		int sy = Item (m_props.nScrollOffset).m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		int sx = Item (m_props.nScrollOffset).m_x - (gameStates.menus.bHires ? 24 : 12);
		if (m_props.nScrollOffset > m_props.nMaxNoScroll)
			DrawRightStringWXY (Scale (gameStates.menus.bHires ? 20 : 10), sx, sy, UP_ARROW_MARKER);
		else
			DrawRightStringWXY (Scale (gameStates.menus.bHires ? 20 : 10), sx, sy, " ");
		i = m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll - 1;
		sy = Item (i).m_y - ((m_props.nStringHeight + 1) * (m_props.nScrollOffset - m_props.nMaxNoScroll));
		sx = Item (i).m_x - (gameStates.menus.bHires ? 24 : 12);
		if (m_props.nScrollOffset + m_props.nMaxDisplayable - m_props.nMaxNoScroll < int (ToS ()))
			DrawRightStringWXY (Scale (gameStates.menus.bHires ? 20 : 10), sx, sy, DOWN_ARROW_MARKER);
		else
			DrawRightStringWXY (Scale (gameStates.menus.bHires ? 20 : 10), sx, sy, " ");
		}
	}
if (m_bCloseBox) {
	DrawCloseBox (m_props.x, m_props.y);
	m_bCloseBox = 1;
	}
#endif
fontManager.SetScale (fontManager.Scale () / GetScale ());
m_bRedraw = 1;
m_bStart = 0;
#if 0
if (!m_bDontRestore && paletteManager.EffectDisabled ())
	paletteManager.EnableEffect ();
#endif
gameStates.render.grAlpha = 1.0f;
}

//------------------------------------------------------------------------------ 

void CMenu::FadeIn (void)
{
if (gameOpts->menus.nFade && !gameStates.app.bNostalgia) {
	if (m_tEnter == uint (-1))
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
if (gameOpts->menus.nFade && !gameStates.app.bNostalgia) {
	int t = int (gameOpts->menus.nFade * gameStates.render.grAlpha);
	int t0 = SDL_GetTicks ();
	int t1, dt;
	do {
		t1 = SDL_GetTicks ();
		dt = t1 - t0;
		m_tEnter = t1 - gameOpts->menus.nFade + dt;
		Render (pszTitle, pszSubTitle, gameCanvasP);
		} while (dt <= t);
	}
}

//------------------------------------------------------------------------------ 

#define REDRAW_ALL	for (i = 0; i < int (ToS ()); i++) Item (i).m_bRedraw = 1;

int CMenu::Menu (const char* pszTitle, const char* pszSubTitle, pMenuCallback callback, 
					  int* nCurItemP, char* filename, int width, int height, int bTinyMode)
{
	int			bKeyRepeat, nItem = nCurItemP ? *nCurItemP : 0;
	int			oldChoice, i;
	int			bSoundStopped = 0, bTimeStopped = 0;
	int			topChoice;// Is this a scrolling box? Set to 0 at init
	CCanvas*		gameCanvasP;
	int			bWheelUp, bWheelDown, nMouseState, nOldMouseState, bDblClick = 0;
	int			mx = 0, my = 0, x1 = 0, x2, y1, y2;
	int			bLaunchOption = 0;
	int			exception = 0;

if (gameStates.menus.nInMenu > 0)
	return -1;

m_tEnter = -1;
m_bRedraw = 0;
m_nChoice = 0;
m_nLastScrollCheck = -1;
m_bStart = 1;
m_bCloseBox = 0;
m_bDontRestore = 0;
m_bAllText = 0;
m_callback = callback;

messageBox.Clear ();
memset (&m_props, 0, sizeof (m_props));
m_props.userWidth = Scale (width);
m_props.userHeight = Scale (height);
m_props.bTinyMode = bTinyMode;
controls.FlushInput ();

if (int (ToS ()) < 1)
	return - 1;
if (gameStates.app.bGameRunning && !gameOpts->menus.nStyle)
	backgroundManager.LoadStars (true);
SDL_ShowCursor (0);
SDL_EnableKeyRepeat(60, 30);
if (gameStates.menus.nInMenu >= 0)
	gameStates.menus.nInMenu++;
if (gameStates.app.bGameRunning && IsMultiGame)
	gameData.multigame.nTypingTimeout = 0;

SetPopupScreenMode ();
SaveScreen (&gameCanvasP);
SetupCanvasses ();

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

paletteManager.DisableEffect ();
m_bDone = 0;
topChoice = 0;

Register ();

while (Item (topChoice).m_nType == NM_TYPE_TEXT) {
	topChoice++;
	if (topChoice >= int (ToS ()))
		topChoice = 0; 
	if (topChoice == nItem) {
		topChoice = 0; 
		break; 
		}
	}

//ogl.Update (0);
// Clear mouse, joystick to clear button presses.
GameFlushInputs ();
nMouseState = nOldMouseState = 0;
m_bCloseBox = !(filename || gameStates.menus.bReordering);

if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING) {
	SDL_ShowCursor (1);
	}
GrabMouse (0, 0);
while (!m_bDone) {
	if (m_bThrottle)
		m_to.Throttle ();	// give the CPU some time to breathe

	if (nCurItemP)
		*nCurItemP = m_nChoice;
	if (gameStates.app.bGameRunning && IsMultiGame) {
		gameStates.multi.bPlayerIsTyping [N_LOCALPLAYER] = 1;
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
	if ((m_props.screenWidth != gameData.render.frame.Width ()) || (m_props.screenHeight != gameData.render.frame.Height ())) {
		memset (&m_props, 0, sizeof (m_props));
		m_props.userWidth = width;
		m_props.userHeight = height;
		m_props.bTinyMode = bTinyMode;
		}
#if 1
	if (m_props.bValid && (m_props.nDisplayMode != gameStates.video.nDisplayMode)) {
		FreeTextBms ();
		SetScreenMode (SCREEN_MENU);
		SaveScreen (&gameCanvasP);
		memset (&m_props, 0, sizeof (m_props));
		m_props.userWidth = width;
		m_props.userHeight = height;
		m_props.bTinyMode = bTinyMode;
		}
#endif
	if (InitProps (pszTitle, pszSubTitle)) {
		backgroundManager.Setup (filename, m_props.x, m_props.y, m_props.width, m_props.height, pszSubTitle == NULL);
		m_bRedraw = 0;
		m_props.ty = m_props.yOffs;
		if (m_nChoice > m_props.nScrollOffset + m_props.nMaxOnMenu - 1)
			m_props.nScrollOffset = m_nChoice - m_props.nMaxOnMenu + 1;
		}

	if (callback && (SDL_GetTicks () - m_tEnter > gameOpts->menus.nFade))
		try {
			m_nChoice = (*callback) (*this, m_nKey, m_nChoice, 0);
			}
		catch (int e) {
			exception = e;
			m_bDone = 1;
			break;
			}

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
		m_bDone = 1;
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
			 m_bDone = 1;
			 m_nChoice = -1;
			}
		 break;
		 
		case KEY_U:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 NetworkRequestPlayerNames (m_nChoice - 2 - tracker.m_bUse);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 m_bDone = 1;
			 m_nChoice = - 1;
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
			gameData.menu.alpha = (gameData.menu.alpha + 16) & 0xFF;
			break;

		case KEY_COMMAND + KEY_P:
		case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
			if (bPauseableMenu) {
				bPauseableMenu = 0;
				m_bDone = 1;
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
				Item (m_nChoice).Value () = -1;
			if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != m_nChoice)) {
				Item (oldChoice).m_group = 0;
				strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
				Item (oldChoice).Value () = -1;
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
				Item (m_nChoice).Value () = -1;
			if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != m_nChoice)) {
				Item (oldChoice).m_group = 0;
				strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
				Item (oldChoice).Value () = -1;
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
					if (Item (m_nChoice).Value ())
						Item (m_nChoice).Value () = 0;
					else
						Item (m_nChoice).Value () = 1;
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
						if ((i != m_nChoice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (m_nChoice).m_group) && (Item (i).Value ())) {
							Item (i).Value () = 0;
							Item (i).m_bRedraw = 1;
						}
					}
					Item (m_nChoice).Value () = 1;
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
				::Swap (Item (m_nChoice).Value (), Item (m_nChoice - 1).Value ());
				Item (m_nChoice).m_bRebuild = 
				Item (m_nChoice - 1).m_bRebuild = 1;
				m_nChoice--;
				}
			 break;

		case KEY_SHIFTED + KEY_DOWN:
			if (gameStates.menus.bReordering && m_nChoice !=(int (ToS ()) - 1)) {
				SwapText (m_nChoice, m_nChoice + 1);
				::Swap (Item (m_nChoice).Value (), Item (m_nChoice + 1).Value ());
				Item (m_nChoice).m_bRebuild = 
				Item (m_nChoice + 1).m_bRebuild = 1;
				m_nChoice++;
				}
			 break;

		case KEY_ALTED + KEY_ENTER: {
			//int bLoadCustomBg = NMFreeCustomBg ();
			FreeTextBms ();
			backgroundManager.Remove ();
			//NMRestoreScreen (filename, & m_bDontRestore);
			GrToggleFullScreenGame ();
			GrabMouse (0, 0);
			SetScreenMode (SCREEN_MENU);
			SaveScreen (&gameCanvasP);
			memset (&m_props, 0, sizeof (m_props));
			m_props.userWidth = width;
			m_props.userHeight = height;
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
					Item (m_nChoice).Value () = -1;
					}
				else {
					Item (m_nChoice).TrimWhitespace ();
					}
				}
			else {
				if (nCurItemP)
					*nCurItemP = m_nChoice;
				m_bDone = 1;
				}
			break;

		case KEY_ESC:
			if ((m_nChoice > - 1) && (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 1)) {
				Item (m_nChoice).m_group = 0;
				strcpy (Item (m_nChoice).m_text, Item (m_nChoice).m_savedText);
				Item (m_nChoice).m_bRedraw = 1;
				Item (m_nChoice).Value () = -1;
				}
			else {
				m_bDone = 1;
				if (nCurItemP)
					*nCurItemP = m_nChoice;
				m_nChoice = -1;
			}
			break;

		case KEY_COMMAND + KEY_SHIFTED + KEY_P:
		case KEY_PRINT_SCREEN:
			gameStates.app.bSaveScreenShot = 1;
			SaveScreenShot (NULL, 0);
			for (i = 0; i < int (ToS ()); i++)
				Item (i).m_bRedraw = 1;
		
			break;

		#if DBG
		case KEY_BACKSPACE:
			if ((m_nChoice > - 1) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT) && (Item (m_nChoice).m_nType != NM_TYPE_INPUT_MENU))
				Int3 (); 
			break;
		#endif

		case KEY_F1:
			Item (m_nChoice).ShowHelp ();
			break;
		}

		if (!m_bDone && nMouseState && !nOldMouseState && !m_bAllText) {
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
								Item (m_nChoice).Value () = !Item (m_nChoice).Value ();
								Item (m_nChoice).m_bRedraw = 1;
								if (m_props.bIsScrollBox)
									m_nLastScrollCheck = - 1;
								break;
							case NM_TYPE_RADIO:
								for (i = 0; i < int (ToS ()); i++) {
									if ((i!= m_nChoice) && (Item (i).m_nType == NM_TYPE_RADIO) && (Item (i).m_group == Item (m_nChoice).m_group) && (Item (i).Value ())) {
										Item (i).Value () = 0;
										Item (i).m_bRedraw = 1;
									}
								}
								Item (m_nChoice).Value () = 1;
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
			m_bDone = 1;
			}
	
		if (!m_bDone && nMouseState && !m_bAllText) {
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
							if ((mx > x1) && (mx < (x1 + sleft_width)) && (Item (m_nChoice).Value () != Item (m_nChoice).MinValue ())) {
								Item (m_nChoice).Value () = Item (m_nChoice).MinValue ();
								Item (m_nChoice).m_bRedraw = 2;
							} else if ((mx < x2) && (mx > (x2 - sright_width)) && (Item (m_nChoice).Value () != Item (m_nChoice).MaxValue ())) {
								Item (m_nChoice).Value () = Item (m_nChoice).MaxValue ();
								Item (m_nChoice).m_bRedraw = 2;
							} else if ((mx > (x1 + sleft_width)) && (mx < (x2 - sright_width))) {
								int numValues, value_width, newValue;
							
								numValues = Item (m_nChoice).MaxValue () - Item (m_nChoice).MinValue () + 1;
								value_width = (slider_width - sleft_width - sright_width) / numValues;
								newValue = (mx - x1 - sleft_width) / value_width;
								if (Item (m_nChoice).Value () != newValue) {
									Item (m_nChoice).Value () = newValue;
									Item (m_nChoice).m_bRedraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (m_nChoice == oldChoice)
						break;
					if ((Item (m_nChoice).m_nType == NM_TYPE_INPUT) && (m_nChoice != oldChoice))
						Item (m_nChoice).Value () = -1;
					if ((oldChoice> - 1) && (Item (oldChoice).m_nType == NM_TYPE_INPUT_MENU) && (oldChoice != m_nChoice)) {
						Item (oldChoice).m_group = 0;
						strcpy (Item (oldChoice).m_text, Item (oldChoice).m_savedText);
						Item (oldChoice).Value () = -1;
					}
					if (oldChoice> - 1) 
						Item (oldChoice).m_bRedraw = 1;
					Item (m_nChoice).m_bRedraw = 1;
					break;
				}
			}
		}
	
		if (!m_bDone && !nMouseState && nOldMouseState && !m_bAllText && (m_nChoice != -1) && (Item (m_nChoice).m_nType == NM_TYPE_MENU)) {
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
						m_bDone = 1;
						}
					else 
						bDblClick = 1;
				}
				else {
					m_bDone = 1;
					if (nCurItemP)
						*nCurItemP = m_nChoice;
				}
			}
		}
	
		if (!m_bDone && !nMouseState && nOldMouseState && (m_nChoice > - 1) && 
			 (Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 0)) {
			Item (m_nChoice).m_group = 1;
			Item (m_nChoice).m_bRedraw = 1;
			if (!strnicmp (Item (m_nChoice).m_savedText, TXT_EMPTY, strlen (TXT_EMPTY))) {
				Item (m_nChoice).m_text [0] = '\0';
				Item (m_nChoice).Value () = -1;
			} else {
				Item (m_nChoice).TrimWhitespace ();
			}
		}
	
		if (!m_bDone && !nMouseState && nOldMouseState && m_bCloseBox) {
			MouseGetPos (&mx, &my);
			x1 = m_props.x + MENU_CLOSE_X;
			x2 = x1 + MENU_CLOSE_SIZE;
			y1 = m_props.y + MENU_CLOSE_Y;
			y2 = y1 + MENU_CLOSE_SIZE;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (nCurItemP)
					*nCurItemP = m_nChoice;
				m_nChoice = -1;
				m_bDone = 1;
				}
			}

//	 HACK! Don't redraw loadgame preview
		if (bRestoringMenu) 
			Item (0).m_bRedraw = 0;

		if (m_nChoice > - 1) {
			int ascii;

			if (((Item (m_nChoice).m_nType == NM_TYPE_INPUT) || 
				 ((Item (m_nChoice).m_nType == NM_TYPE_INPUT_MENU) && (Item (m_nChoice).m_group == 1))) && (oldChoice == m_nChoice)) {
				if (m_nKey == KEY_LEFT || m_nKey == KEY_BACKSPACE || m_nKey == KEY_PAD4) {
					if (Item (m_nChoice).Value () == -1) 
						Item (m_nChoice).Value () = (int) strlen (Item (m_nChoice).m_text);
					if (Item (m_nChoice).Value () > 0)
						Item (m_nChoice).Value ()--;
					Item (m_nChoice).m_text [Item (m_nChoice).Value ()] = '\0';
					Item (m_nChoice).m_bRedraw = 1;
					}
				else {
					ascii = KeyToASCII (m_nKey);
					if ((ascii < 255) && (Item (m_nChoice).Value () < Item (m_nChoice).m_nTextLen)) {
						int bAllowed;

						if (Item (m_nChoice).Value ()== -1)
							Item (m_nChoice).Value () = 0;
						bAllowed = CharAllowed ((char) ascii);
						if (!bAllowed && ascii == ' ' && CharAllowed ('_')) {
							ascii = '_';
							bAllowed = 1;
							}
						if (bAllowed) {
							Item (m_nChoice).m_text [Item (m_nChoice).Value ()++] = ascii;
							Item (m_nChoice).m_text [Item (m_nChoice).Value ()] = '\0';
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
				int ov = Item (m_nChoice).Value ();
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
				}
			if ((Item (m_nChoice).m_nType == NM_TYPE_SLIDER) || (Item (m_nChoice).m_nType == NM_TYPE_NUMBER))
				Item (m_nChoice).Value () = NMCLAMP (Item (m_nChoice).Value (), Item (m_nChoice).MinValue (), Item (m_nChoice).MaxValue ());
			if (ov != Item (m_nChoice).Value ())
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
//paletteManager.SetEffect (0, 0, 0);
SDL_EnableKeyRepeat (0, 0);
if (gameStates.app.bGameRunning && IsMultiGame)
	MultiSendMsgQuit();

for (i = 0; i < int (ToS ()); i++)
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
if (IndexOf (szId, false) > 0)
	PrintLog (0, "duplicate menu id '%s'\n", szId);
item.SetId (szId);
}

//------------------------------------------------------------------------------ 

CMenuItem* CMenu::AddItem (void)
{
return Grow (1) ? Top () : NULL;
}

//------------------------------------------------------------------------------ 

int CMenu::AddCheck (const char* szId, const char* szText, int nValue, int nKey, const char* szHelp)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_CHECK;
item->m_pszText = NULL;
item->SetText (szText);
item->Value () = NMBOOL (nValue);
item->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int (*(szText - 1));
item->m_szHelp = szHelp;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddRadio (const char* szId, const char* szText, int nValue, int nKey, const char* szHelp)
{
if (!ToS () || (Top ()->m_nType != NM_TYPE_RADIO))
	NewGroup ();
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_RADIO;
item->m_pszText = NULL;
item->SetText (szText);
item->Value () = nValue;
item->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int (*(szText - 1));
item->m_group = m_nGroup;
item->m_szHelp = szHelp;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddMenu (const char* szId, const char* szText, int nKey, const char* szHelp)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_MENU;
item->m_pszText = NULL;
item->SetText (szText);
item->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int (*(szText - 1));
item->m_szHelp = szHelp;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddText (const char* szId, const char* szText, int nKey)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_TEXT;
item->m_pszText = NULL;
item->SetText (szText);
item->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int (*(szText - 1));
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddSlider (const char* szId, const char* szText, int nValue, int nMin, int nMax, int nKey, const char* szHelp)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_SLIDER;
item->m_pszText = NULL;
item->SetText (szText);
item->Value () = NMCLAMP (nValue, nMin, nMax);
item->MinValue () = nMin;
item->MaxValue () = nMax;
item->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int (*(szText - 1));
item->m_szHelp = szHelp;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szId, const char* szText, int nLen, const char* szHelp)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_INPUT;
item->m_pszText = const_cast<char*> (szText);
item->SetText (szText);
item->m_nTextLen = nLen;
item->m_szHelp = szHelp;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szId, const char* szText, char* szValue, int nLen, const char* szHelp)
{
if (AddText ("", szText, -1) < 0)
	return -1;
return AddInput (szId, szValue, nLen, szHelp);
}

//------------------------------------------------------------------------------ 

int CMenu::AddInput (const char* szId, const char* szText, char* szValue, int nValue, int nLen, const char* szHelp)
{
sprintf (szValue, "%d", nValue);
return AddInput (szId, szText, szValue, nLen, szHelp);
}

//------------------------------------------------------------------------------ 

int CMenu::AddInputBox (const char* szId, const char* szText, int nLen, int nKey, const char* szHelp)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_INPUT_MENU;
item->m_pszText = (char*) (szText);
item->SetText (szText);
item->m_nTextLen = nLen;
item->m_nKey = (gameStates.app.bEnglish || !szText) ? nKey : int (*(szText - 1));
item->m_szHelp = szHelp;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddNumber (const char* szId, const char* szText, int nValue, int nMin, int nMax)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_NUMBER;
item->m_pszText = (char*) (szText);
item->SetText (szText);
item->Value () = NMCLAMP (nValue, nMin, nMax);
item->MinValue () = nMin;
item->MaxValue () = nMax;
SetId (*item, szId);
return ToS () - 1;
}

//------------------------------------------------------------------------------ 

int CMenu::AddGauge (const char* szId, const char* szText, int nValue, int nMax)
{
CMenuItem* item = AddItem ();
if (!item)
	return -1;
item->m_nType = NM_TYPE_GAUGE;
item->m_pszText = NULL;
item->SetText (szText);
item->m_nTextLen = *szText ? (int) strlen (szText) : 20;
item->Value () = NMCLAMP (nValue, 0, nMax);
item->MaxValue () = nMax;
SetId (*item, szId);
m_bThrottle = false;
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
menu.AddGauge ("progress bar", "                    ", -1, nMaxProgress); //the blank string denotes the screen width of the gauge
menu.Item (0).m_bCentered = 1;
menu.Item (0).Value () = nCurProgress;
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
