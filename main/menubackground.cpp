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
#	include <windows.h>
#	include <stddef.h>
#endif
#ifdef __macosx__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef _WIN32
#	include <unistd.h>
#endif

#include "error.h"
#include "inferno.h"
#include "key.h"
#include "text.h"
#include "findfile.h"
#include "gamefont.h"
#include "pcx.h"
#include "u_mem.h"
#include "strutil.h"
#include "ogl_lib.h"
#include "ogl_bitmap.h"
#include "render.h"
#include "menu.h"
#include "input.h"
#include "menubackground.h"

#if defined (TACTILE)
 #include "tactile.height"
#endif

#define LHX(x)      (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)

CPalette* menuPalette;

CBackgroundManager backgroundManager;

#define MENU_BACKGROUND_BITMAP_HIRES (CFile::Exist ("scoresb.pcx", gameFolders.szDataDir, 0)? "scoresb.pcx": "scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (CFile::Exist ("scores.pcx", gameFolders.szDataDir, 0) ? "scores.pcx": "scoresb.pcx") // Mac datafiles only have scoresb.pcx

#define MENU_BACKGROUND_BITMAP (gameStates.menus.bHires ? MENU_BACKGROUND_BITMAP_HIRES : MENU_BACKGROUND_BITMAP_LORES)

int bHiresBackground;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBackground::Init (void)
{
m_canvas = NULL;
m_saved = NULL;
m_background = NULL;
m_name = NULL;
m_bIgnoreCanv = false;
m_bIgnoreBg = false;
}

//------------------------------------------------------------------------------

void CBackground::Destroy (void)
{
if (m_background && (m_background != backgroundManager.Background ()))
	delete m_background;
if (m_saved)
	delete m_saved;
if (m_canvas)
	m_canvas->Destroy ();
Init ();
}

//------------------------------------------------------------------------------

CBitmap* CBackground::Load (char* filename, int width, int height)
{
if (!(m_name = filename))
	return backgroundManager.Background (1)->CreateChild (0, 0, width, height);
else if (backgroundManager.IsDefault (filename) || !(m_background = backgroundManager.LoadBackground (filename)))
	return backgroundManager.Background (0);
return m_background;
}

//------------------------------------------------------------------------------

void CBackground::Setup (int x, int y, int width, int height)
{
if (m_canvas)
	m_canvas->Destroy ();
if (gameOpts->menus.nStyle)
	m_canvas = screen.Canvas ()->CreatePane (0, 0, screen.Width (), screen.Height ());
else
	m_canvas = screen.Canvas ()->CreatePane (x, y, width, height);
CCanvas::SetCurrent (m_canvas);
	
}

//------------------------------------------------------------------------------

void CBackground::Save (int width, int height)
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
m_bTopMenu = (backgroundManager.Depth () == 0);
m_bMenuBox = gameOpts->menus.nStyle && (gameOpts->menus.altBg.bHave > 0);
if (!(m_background = Load (filename, width, height)))
	return false;
Setup (x, y, width, height);
Save (width, height);
Draw ();
return true;
}

//------------------------------------------------------------------------------

void CBackground::Draw (void)
{
paletteManager.SetEffect (0, 0, 0);
if (!(gameStates.menus.bNoBackground || gameStates.app.bGameRunning)) {
	if (gameOpts->menus.nStyle || m_bTopMenu) 
		m_background->Stretch ();
	if (m_bTopMenu)
		PrintVersionInfo ();
	}
if (!m_name) {
	if (m_bMenuBox) {
		if (m_canvas || m_bIgnoreCanv) {
			if (!m_bIgnoreCanv)
				CCanvas::SetCurrent (m_canvas);
			}
		backgroundManager.DrawBox (gameData.menu.nLineWidth, 1.0f, 0);
		}
	else
		DrawArea (0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
		//CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), 
		//			 CCanvas::Current ()->Right (), CCanvas::Current ()->Bottom ());
	}
paletteManager.LoadEffect ();
GrUpdate (0);
}

//------------------------------------------------------------------------------
// Redraw a part of the menu area's background

void CBackground::DrawArea (int left, int top, int right, int bottom)
{
if (gameOpts->menus.nStyle) 
	Draw ();
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
	if (!backgroundManager.Shadow ())
		m_background->RenderClipped (CCanvas::Current (), left, top, width, height, LHX (10), LHY (10));
	else {
		m_background->RenderClipped (CCanvas::Current (), left, top, width, height, 0, 0);
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
	glEnable (GL_BLEND);
	GrUpdate (0);
	}
gameStates.render.grAlpha = FADE_LEVELS;
}

//------------------------------------------------------------------------------

void CBackground::Restore (void)
{
if (!gameOpts->menus.nStyle) {
	CCanvas::SetCurrent (m_canvas);
	if (m_saved) {
		m_saved->Blit ();
		GrUpdate (0);
		}
	}
}

//------------------------------------------------------------------------------

void CBackground::Restore (int dx, int dy, int w, int h, int sx, int sy)
{
int x1 = sx; 
int x2 = sx+w-1;
int y1 = sy; 
int y2 = sy+h-1;

if (x1 < 0) 
	x1 = 0;
if (y1 < 0) 
	y1 = 0;

if (x2 >= m_background->Width ()) 
	x2 = m_background->Width () - 1;
if (y2 >= m_background->Height ()) 
	y2 = m_background->Height () - 1;

w = x2 - x1 + 1;
h = y2 - y1 + 1;
m_background->Render (CCanvas::Current (), dx, dy, w, h, x1, y1, w, h);
}

//------------------------------------------------------------------------------

CBackgroundManager::CBackgroundManager () 
{ 
m_background [0] = NULL;
m_background [1] = NULL;
m_nDepth = -1;
m_filename [0] = MENU_PCX_NAME ();
m_filename [1] = MENU_BACKGROUND_BITMAP;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Remove (void) 
{
if (m_nDepth) {
	m_bg [m_nDepth].Restore ();
	m_bg [m_nDepth--].Destroy ();
	if (gameStates.app.bGameRunning) 
		paletteManager.LoadEffect ();
	}
}

//------------------------------------------------------------------------------

void CBackgroundManager::Init (void)
{
m_background [0] = NULL;
m_background [1] = NULL;
m_filename [0] = MENU_PCX_NAME ();
m_filename [1] = MENU_BACKGROUND_BITMAP;
m_nDepth = -1; 
m_bShadow = true;
m_bValid = false;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Destroy (void)
{
if (m_background [0])
	m_background [0]->Destroy ();
if (m_background [1])
	m_background [1]->Destroy ();
while (m_nDepth >= 0)
	m_bg [m_nDepth--].Destroy ();
Init ();
}

//------------------------------------------------------------------------------

void CBackgroundManager::DrawBox (int left, int top, int right, int bottom, int nLineWidth, float fAlpha, int bForce)
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

bool CBackgroundManager::IsDefault (char* filename)
{
return !strcmp (filename, m_filename [0]);
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadCustomBackground (void)
{
if (m_background [0]|| (gameStates.app.bNostalgia || !gameOpts->menus.nStyle))
	return m_background [0];

CBitmap* bmP;

gameOpts->menus.altBg.bHave = 0;
if (!(bmP = CBitmap::Create (0, 0, 0, 1)))
	return NULL;
if (!ReadTGA (gameOpts->menus.altBg.szName, 
#ifdef __linux__
				  gameFolders.szCacheDir,
#else
				  NULL, 
#endif
				  bmP, 
				 (gameOpts->menus.altBg.alpha < 0) ? -1 : (int) (gameOpts->menus.altBg.alpha * 255), 
				 gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale, 0)) {
	delete bmP;
	gameOpts->menus.altBg.bHave = -1;
	return NULL;
	}
gameOpts->menus.altBg.bHave = 1;
return bmP;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadBackground (char* filename)
{
	int width, height;

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
if (!m_bValid) {
	if (!LoadCustomBackground ())
	m_background [0] = LoadBackground (m_filename [0]);
	m_background [1] = LoadBackground (m_filename [1]);
	m_bValid = true;
	}
}

//------------------------------------------------------------------------------

bool CBackgroundManager::Setup (char *filename, int x, int y, int width, int height)
{
Create ();
if (m_nDepth >= 2)
	return false;
return m_bg [++m_nDepth].Create (filename, x, y, width, height);
}

//------------------------------------------------------------------------------
