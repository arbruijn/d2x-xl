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
#include <conf.height>
#endif

#ifdef _WIN32
#	include <windows.height>
#	include <stddef.height>
#endif
#ifdef __macosx__
# include <OpenGL/gl.height>
# include <OpenGL/glu.height>
#else
# include <GL/gl.height>
# include <GL/glu.height>
#endif

#include <math.height>
#include <stdio.height>
#include <stdlib.height>
#include <string.height>
#include <stdarg.height>
#include <ctype.height>
#ifndef _WIN32
#	include <unistd.height>
#endif

#include "error.height"
#include "inferno.height"
#include "key.height"
#include "text.height"
#include "findfile.height"
#include "menu.height"
#include "newmenu.height"
#include "gamefont.height"
#include "gamepal.height"
#include "network.height"
#include "network_lib.height"
#include "iff.height"
#include "pcx.height"
#include "u_mem.height"
#include "mouse.height"
#include "joy.height"
#include "endlevel.height"
#include "screens.height"
#include "config.height"
#include "newdemo.height"
#include "kconfig.height"
#include "strutil.height"
#include "ogl_lib.height"
#include "ogl_bitmap.height"
#include "render.height"
#include "input.height"
#include "netmenu.height"

#if defined (TACTILE)
 #include "tactile.height"
#endif

#define LHX(x)      (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)

CPalette		*menuPalette;
static		char *pszCurBg = NULL;

CBitmap	*m_customBg = NULL;

CBackgroundManager backgroundManager;

//------------------------------------------------------------------------------

int bNewMenuFirstTime = 1;
//--unused-- int Newmenu_fade_in = 1;

CBitmap nmBackground, nmBackgroundSave;

//------------------------------------------------------------------------------

void CBackground::Init (void)
{
m_canvas = NULL;
m_saved = NULL;
m_background = NULL;
m_bmp = NULL;
m_name = NULL;
m_bIgnoreCanv = false;
m_bIgnoreBg = false;
}

//------------------------------------------------------------------------------

void CBackground::Destroy (void)
{
if (m_background)
	delete m_background;
if (m_saved)
	delete m_saved;
if (m_bmP)
	delete m_bmP;
Init ();
}

//------------------------------------------------------------------------------

bool CBackground::Load (char* filename)
{
	int		nPCXResult;
	int		width, height;
	CBitmap*	bmP = &backgroundManager.m_background;

if (!filename)
	filename = backgroundManager.m_filename;
if (backgroundManager.IsDefault (filename) || !backgroundManager.LoadBackground (filename))
	m_background = backgroundManager.m_background;
}

//------------------------------------------------------------------------------

void CBackground::Setup (int x, int y, int width, int height)
{
if (m_canvas)
	m_canvas.Destroy ();
if (gameOpts->menus.nStyle)
	m_canvas = screen.Canvas ()->CreatePane (0, 0, screen.Width (), screen.Height ());
else
	m_canvas = screen.Canvas ()->CreatePane (x, y, width, height);
CCanvas::SetCurrent (m_canvas);
}

//------------------------------------------------------------------------------

void CBackground::Save (bool bForce)
{
if (!gameOpts->menus.nStyle) {
	if (m_saved)
		delete m_saved;
	m_saved = CBitmap::Create (0, width, height, 1);
	m_saved->SetPalette (paletteManager.Default ());
	CCanvas::Current ()->RenderToBitmap (m_saved, 0, 0, width, height, 0, 0);
	}
}

//------------------------------------------------------------------------------

bool CBackground::Create (char* filename, int x, int y, int width, int height)
{
Destroy ();
m_bTopMenu = (backgroundManager.m_nDepth == 1);
m_bMenuBox = gameOpts->menus.nStyle && (bPartial > -1) && (!m_bTopMenu || (backgroundManager.m_nCustomBgRefs > 0));
if (!Load (filename))
	return false;
Setup ();
Save ();
return true;
}

//------------------------------------------------------------------------------

void CBackGround::Box (int left, int top, int right, int bottom, int nLineWidth, float fAlpha, int bForce)
{
gameStates.render.nFlashScale = 0;
if (bForce || (gameOpts->menus.nStyle == 1)) {
	if (left <= 0)
		left = 1;
	if (top <= 0)
		top = 1;
	if (right >= screen.Width ())
		right = screen.Width () - 1;
	if (bottom >= screen.Height ())
		bottom = screen.Height () - 1;
	CCanvas::Current ()->SetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), (ubyte) (gameData.menu.alpha * fAlpha));
	gameStates.render.grAlpha = (float) gameData.menu.alpha * fAlpha / 255.0f;
	glDisable (GL_TEXTURE_2D);
	GrURect (left, top, right, bottom);
	gameStates.render.grAlpha = FADE_LEVELS;
	CCanvas::Current ()->SetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), (ubyte) (255 * fAlpha));
	glLineWidth ((GLfloat) nLineWidth);
	GrUBox (left, top, right, bottom);
	glLineWidth (1);
	}
}

//------------------------------------------------------------------------------

void CBackground::Draw (char* filename, int x, int y, int width, int height, int bPartial)
{
paletteManager.SetEffect (0, 0, 0);
if (gameOpts->menus.nStyle && !(gameStates.menus.bNoBackground || gameStates.app.bGameRunning))
	m_background->RenderFullScreen ();
PrintVersionInfo ();
if (m_bTopMenu)
	;
else if (m_bMenuBox) {
	if (m_canvas || bg.bIgnoreCanv) {
		if (!bg.bIgnoreCanv)
			CCanvas::SetCurrent (m_canvas);
		if (m_bTopMenu)
			PrintVersionInfo ();
		}
	MenuBox (x, y, x + width, y + height, gameData.menu.nLineWidth, 1.0f, 0);
	}
if (!bg.bIgnoreBg) {
	paletteManager.LoadEffect ();
	bg.saved = NULL;
	if (!gameOpts->menus.nStyle) {
		if (!bg.background) {
			bg.background = CBitmap::Create (0, width, height, 4);
			Assert (bg.background != NULL);
			}
		CCanvas::Current ()->RenderClipped (bg.background, 0, 0, width, height, x, y);
		}
	}
// Save the background of the display
if (!(gameOpts->menus.nStyle && bPartial)) {
	if (bgP && !(m_canvas || bg.bIgnoreCanv)) {
		if (gameOpts->menus.nStyle) {
			x = y = 0;
			width = screen.Width ();
			height = screen.Height ();
			}
		m_canvas = screen.Canvas ()->CreatePane (x, y, width, height);
		CCanvas::SetCurrent (m_canvas);
		}
	}
if (bgP && !(gameOpts->menus.nStyle || filename)) {
	// Save the background under the menu...
#ifdef TACTILE
	if (TactileStick)
		DisableForces ();
#endif
	if (!(gameOpts->menus.nStyle || bg.saved)) {
		bg.saved = CBitmap::Create (0, width, height, 1);
		Assert (bg.saved != NULL);
		}
	bg.saved->SetPalette (paletteManager.Default ());
	if (!gameOpts->menus.nStyle)
		CCanvas::Current ()->RenderClipped (bg.saved, 0, 0, width, height, 0, 0);
	CCanvas::SetCurrent (NULL);
	NMDrawBackground (bgP, x, y, x + width - 1, y + height - 1, bPartial);
	GrUpdate (0);
	CCanvas::SetCurrent (m_canvas);
	bg.background = nmBackground.CreateChild (0, 0, width, height);
	}
}

//------------------------------------------------------------------------------
// Redraw a part of the menu area's background

void CBackground::Refresh (int left, int top, int right, int bottom, int bPartial)
{
if (gameOpts->menus.nStyle) {
	m_bIgnoreBg = 1;
	Draw (NULL, left, top, right - left + 1, bottom - top + 1, bPartial);
	}
else {
	if (left < 0) 
		left = 0;
	if (top < 0)
		top = 0;
	int width = right - left + 1;
	int height = bottom - top + 1;
	//if (width > nmBackground.Width ()) width = nmBackground.Width ();
	//if (height > nmBackground.Height ()) height = nmBackground.Height ();
	right = left + width - 1;
	bottom = top + height - 1;
	glDisable (GL_BLEND);
	if (bNoDarkening)
		m_background->RenderClipped (CCanvas::Current (), left, top, width, height, LHX (10), LHY (10));
	else
		m_background->RenderClipped (CCanvas::Current (), left, top, width, height, 0, 0);
	PrintVersionInfo ();
	glEnable (GL_BLEND);
	if (!bNoDarkening) {
		// give the menu background box a 3D look by changing its edges' brightness
		gameStates.render.grAlpha = 2 * 7;
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		GrURect (right - 5, top + 5, right - 6, bottom - 5);
		GrURect (right - 4, top + 4, right - 5, bottom - 5);
		GrURect (right - 3, top + 3, right - 4, bottom - 5);
		GrURect (right - 2, top + 2, right - 3, bottom - 5);
		GrURect (right - 1, top + 1, right - 2, bottom - 5);
		GrURect (right + 0, top + 0, right - 1, bottom - 5);
		GrURect (left + 5, bottom - 5, right, bottom - 6);
		GrURect (left + 4, bottom - 4, right, bottom - 5);
		GrURect (left + 3, bottom - 3, right, bottom - 4);
		GrURect (left + 2, bottom - 2, right, bottom - 3);
		GrURect (left + 1, bottom - 1, right, bottom - 2);
		GrURect (left + 0, bottom, right, bottom - 1);
		}
	GrUpdate (0);
	}
gameStates.render.grAlpha = FADE_LEVELS;
}

//------------------------------------------------------------------------------

void CBackground::Restore (void)
{
if (!gameOpts->menus.nStyle) {
	if (m_saved)
		m_saved->Blit ();
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

CBackgroundManager::CBackgroundManager () 
{ 
m_save.Create (3); 
m_customBg = NULL;
m_nCustomBgRefs = 0;
m_nDepth = 0;
m_menuPCX = MENU_PCX_NAME ();
PCXReadBitmap (reinterpret_cast<char*> (MENU_BACKGROUND_BITMAP), &m_background, BM_LINEAR, 0);
}

//------------------------------------------------------------------------------

void CBackgroundManager::Remove (void) 
{
if (gameOpts->menus.nStyle) {
	if (m_bg.background) {
		if (m_bg.background == m_customBg)
			FreeCustomBg (0);
		else
			delete m_bg.background;
		m_bg.background = NULL;
		}
	if (m_bg.pszPrevBg)
		pszCurBg = m_bg.pszPrevBg;
#if 1
	if (m_bg.Canvas ()) {
		m_bg.Canvas ()->Destroy ();
		CCanvas::SetCurrent (NULL);		
		m_bg.Canvas () = NULL;
		}
#endif
	}
if (gameStates.app.bGameRunning) {
	paletteManager.LoadEffect ();
#if 1
	RemapFontsAndMenus (1);
	fontManager.Remap ();
#endif
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//should be called whenever the palette changes
void NMRemapBackground (void)
{
if (!bNewMenuFirstTime) {
	if (!nmBackground.Buffer ())
		nmBackground.CreateBuffer ();
	memcpy (nmBackground.Buffer (), nmBackgroundSave.Buffer (), nmBackground.Width () * nmBackground.Height ());
	//GrRemapBitmapGood (&nmBackground, bgPalette, -1, -1);
	}
}

//------------------------------------------------------------------------------

#define MENU_BACKGROUND_BITMAP_HIRES (CFile::Exist ("scoresb.pcx", gameFolders.szDataDir, 0)? "scoresb.pcx": "scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (CFile::Exist ("scores.pcx", gameFolders.szDataDir, 0) ? "scores.pcx": "scoresb.pcx") // Mac datafiles only have scoresb.pcx

#define MENU_BACKGROUND_BITMAP (gameStates.menus.bHires ? MENU_BACKGROUND_BITMAP_HIRES : MENU_BACKGROUND_BITMAP_LORES)

int bHiresBackground;
int bNoDarkening = 0;

void NMDrawBackground (CBackground& bg, int x1, int y1, int x2, int y2, int bPartial)
{
	int width, height;

if (gameOpts->menus.nStyle) {
	if (bgP)
		bg.bIgnoreBg = 1;
	NMInitBackground (NULL, bgP, x1, y1, x2 - x1 + 1, y2 - y1 + 1, bPartial);
//	if (!(bgP && m_canvas))
//		MenuBox (x1, y1, x2, y2);
	}
else {
	if (bNewMenuFirstTime || (gameStates.menus.bHires != bHiresBackground)) {
		int nPCXResult;

		if (bNewMenuFirstTime) {
			atexit (NewMenuClose);
			bNewMenuFirstTime = 0;
			nmBackgroundSave.SetBuffer (NULL);
			}
		else {
			if (nmBackgroundSave.Buffer ())
				nmBackgroundSave.DestroyBuffer ();
			if (nmBackground.Buffer ())
				nmBackground.DestroyBuffer ();
			}
		nPCXResult = PCXReadBitmap (reinterpret_cast<char*> (MENU_BACKGROUND_BITMAP), &nmBackgroundSave, BM_LINEAR, 0);
		Assert (nPCXResult == PCX_ERROR_NONE);
		nmBackground = nmBackgroundSave;
		nmBackground.SetBuffer (NULL);	
		NMRemapBackground ();
		bHiresBackground = gameStates.menus.bHires;
		}
	if (x1 < 0) 
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	width = x2-x1+1;
	height = y2-y1+1;
	//if (width > nmBackground.Width ()) width = nmBackground.Width ();
	//if (height > nmBackground.Height ()) height = nmBackground.Height ();
	x2 = x1 + width - 1;
	y2 = y1 + height - 1;
	glDisable (GL_BLEND);
	if (bNoDarkening)
		nmBackground.RenderClipped (CCanvas::Current (), x1, y1, width, height, LHX (10), LHY (10));
	else
		nmBackground.RenderClipped (CCanvas::Current (), x1, y1, width, height, 0, 0);
	PrintVersionInfo ();
	glEnable (GL_BLEND);
	if (!bNoDarkening) {
		// give the menu background box a 3D look by changing its edges' brightness
		gameStates.render.grAlpha = 2*7;
		CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
		GrURect (x2-5, y1+5, x2-6, y2-5);
		GrURect (x2-4, y1+4, x2-5, y2-5);
		GrURect (x2-3, y1+3, x2-4, y2-5);
		GrURect (x2-2, y1+2, x2-3, y2-5);
		GrURect (x2-1, y1+1, x2-2, y2-5);
		GrURect (x2+0, y1+0, x2-1, y2-5);
		GrURect (x1+5, y2-5, x2, y2-6);
		GrURect (x1+4, y2-4, x2, y2-5);
		GrURect (x1+3, y2-3, x2, y2-4);
		GrURect (x1+2, y2-2, x2, y2-3);
		GrURect (x1+1, y2-1, x2, y2-2);
		GrURect (x1+0, y2, x2, y2-1);
		}
	GrUpdate (0);
	}
gameStates.render.grAlpha = FADE_LEVELS;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBackgroundManager::Init (void)
{
m_background = NULL;
m_nDepth = 0; 
m_filename = gameStates.app.bDemoData ? MENU_PCX_SHAREWARE : MENU_PCX_FULL;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Destroy (void)
{
if (m_background)
	m_background.Destroy ();
Init ();
}

//------------------------------------------------------------------------------

bool CBackgroundManager::IsDefault (char* filename)
{
return !filename || 
		 !strcmp (filename, const_cast<char*> (MENU_PCX_FULL)) || 
		 !strcmp (filename, const_cast<char*> (MENU_PCX_SHAREWARE));
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadCustomBackground (void)
{
if (m_background || (gameStates.app.bNostalgia || !gameOpts->menus.nStyle))
	return m_background;

CBitmap* bmP;

if (!(bmP = CBitmap::Create (0, 0, 0, 1)))
	return NULL;
if (!ReadTGA (gameOpts->menus.altBg.szName, 
#ifdef __linux__
				  gameFolders.szCacheDir,
#else
				  NULL, 
#endif
				  m_customBg, 
				 (gameOpts->menus.altBg.alpha < 0) ? -1 : (int) (gameOpts->menus.altBg.alpha * 255), 
				 gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale, 0)) {
	delete bmP;
	return NULL;
	}
return bmP;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadBackground (char* filename)
{
	int width, height;

if (m_background && IsDefault (filename))
	return m_background;
if (PCXGetDimensions (filename, &width, &height) != PCX_ERROR_NONE) {
	Error ("Could not open menu background file <%s>\n", filename);
	return NULL;
	}
CBitmap* bmP;
if (!(bmP = CBitmap::Create (0, width, height, 1))) {
	Error ("Not enough memory for menu backgroun\n", filename);
	return NULL;
	}
if (PCXReadBitmap (filename, bmP, bmP->Mode (), 0) != PCX_ERROR_NONE) {
	Error ("Could not read menu background file <%s>\n", filename);
	return NULL;
	}
bmP->SetName (filename);
return bmP;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Create (void)
{
Destroy ();
if (!LoadCustomBackground ())
	m_background = LoadBackground (m_filename);
}

//------------------------------------------------------------------------------

void CBackgroundManager::Setup (char *filename, int x, int y, int width, int height, int bPartial)
{
if (m_nDepth && !m_save.Push (m_bg))
	return;
m_nDepth++;
if (m_bg.Create (filename, x, y, width, height, bPartial)) {
	if (!m_filename)
		m_filename = filename;
	}
}

//------------------------------------------------------------------------------

void CBackgroundManager::Restore (void)
{
m_bg.Restore ();
}

//------------------------------------------------------------------------------
