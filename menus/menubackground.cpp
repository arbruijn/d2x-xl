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

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
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
#include "descent.h"
#include "key.h"
#include "text.h"
#include "findfile.h"
#include "gamefont.h"
#include "pcx.h"
#include "u_mem.h"
#include "strutil.h"
#include "ogl_lib.h"
#include "ogl_bitmap.h"
#include "ogl_render.h"
#include "rendermine.h"
#include "menu.h"
#include "input.h"
#include "menubackground.h"

#define LHX(x)      (gameStates.menus.bHires ? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires ? (24 * (y)) / 10 : y)

CPalette* menuPalette;

CBackgroundManager backgroundManager;

#define MENU_PCX_FULL		menuBgNames [0][!gameStates.app.bDemoData && gameStates.menus.bHires]
#define MENU_PCX_OEM			menuBgNames [1][!gameStates.app.bDemoData && gameStates.menus.bHires]
#define MENU_PCX_SHAREWARE	menuBgNames [2][!gameStates.app.bDemoData && gameStates.menus.bHires]
#define MENU_PCX_MAC_SHARE	menuBgNames [3][!gameStates.app.bDemoData && gameStates.menus.bHires]

int bHiresBackground;

//------------------------------------------------------------------------------

static char* szDesktopFilenames [4][2] = {
	{"descentb.pcx", "descent.pcx"},
	{"descntob.pcx", "descento.pcx"},
	{"descentd.pcx", "descentd.pcx"},
	{"descentb.pcx", "descentb.pcx"}
};

#if DBG

static char* menuBgNames [4][2] = {
	{"menu.pcx", "menub.pcx"},
	{"menuo.pcx", "menuob.pcx"},
	{"menud.pcx", "menud.pcx"},
	{"menub.pcx", "menub.pcx"}
	};

#else

static char* menuBgNames [4][2] = {
	{"\x01menu.pcx", "\x01menub.pcx"},
	{"\x01menuo.pcx", "\x01menuob.pcx"},
	{"\x01menud.pcx", "\x01menud.pcx"},
	{"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

static char* szWallpapers [3][2] = {
	{"stars.pcx", "starsb.pcx"},
	{"scores.pcx", "scoresb.pcx"},
	{"map.pcx", "mapb.pcx"}
	};


//------------------------------------------------------------------------------

static inline int Hires (int bHires = -1)
{
return gameStates.app.bDemoData ? 0 : (bHires < 0) ? gameStates.menus.bHires : bHires;
}

//------------------------------------------------------------------------------

char* WallpaperName (int nType, int bHires)
{
return szWallpapers [nType - 1][Hires (bHires)];
}

//------------------------------------------------------------------------------

static inline void SetBoxBorderColor (void)
{
CCanvas::Current ()->SetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), 255);
}

//------------------------------------------------------------------------------

static inline void SetBoxFillColor (void)
{
CCanvas::Current ()->SetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), gameData.menu.alpha);
}

//------------------------------------------------------------------------------

static inline void DrawBox (CCanvas& canvas)
{
#if 1
gameStates.render.nFlashScale = 0;
canvas.Activate ();
CCanvasColor fontColors [2] = { canvas.FontColor (0), canvas.FontColor (1) };
SetBoxFillColor ();
ogl.SetTexturing (false);
OglDrawFilledRect (0, 0, canvas.Width (), canvas.Height ());
SetBoxBorderColor ();
float flw = GLfloat (gameData.menu.nLineWidth) * sqrt (GLfloat (gameData.render.frame.Width ()) / 640.0f);
glLineWidth (flw);
int lw = int (ceil (flw));
OglDrawEmptyRect (lw - 1, lw - 1, canvas.Width () - lw, canvas.Height () - lw);
glLineWidth (1.0f);
canvas.SetFontColor (fontColors [0], 0);
canvas.SetFontColor (fontColors [1], 1);
canvas.Deactivate ();
#endif
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBackground::Init (void)
{
m_bitmap = NULL;
m_nType = 0;
m_nWallpaper = 0;
gameStates.app.bClearMessage = 0;
}

//------------------------------------------------------------------------------

void CBackground::Destroy (void)
{
m_bitmap = NULL;
Init ();
}

//------------------------------------------------------------------------------

void CBackground::Setup (int width, int height)
{
SetupCanvasses ();
CCanvas::Setup (&gameData.render.screen, (gameData.render.frame.Width () - width) / 2, (gameData.render.frame.Height () - height) / 2, width, height, true);
}

//------------------------------------------------------------------------------

bool CBackground::Create (int width, int height, int nType, int nWallpaper)
{
Destroy ();
m_nType = nType;
m_nWallpaper = nWallpaper;
Setup (width, height);
return true;
}

//------------------------------------------------------------------------------

void CBackground::Activate (void)
{
CViewport::SetLeft (CViewport::Left () - CScreen::Unscaled (gameData.StereoOffset2D ()));
CCanvas::Activate (&gameData.render.frame);
}

//------------------------------------------------------------------------------

void CBackground::Deactivate (void)
{
CViewport::SetLeft (CViewport::Left () + CScreen::Unscaled (gameData.StereoOffset2D ()));
CCanvas::Deactivate ();
}

//------------------------------------------------------------------------------

void CBackground::Draw (bool bUpdate)
{
if (m_nType == BG_WALLPAPER) {
	if (!(gameStates.menus.bNoBackground || (gameStates.app.bGameRunning && !gameStates.app.bNostalgia))) {
		gameData.render.frame.Activate ();
		m_bitmap->RenderStretched ();
		PrintVersionInfo ();
		gameData.render.frame.Deactivate ();
		}
	}
else if (m_nType == BG_SUBMENU) {
	if (gameStates.app.bNostalgia)
		DrawArea (0, 0, Width (), Height ());
	else 
		DrawBox ();
	}
if (bUpdate && !gameStates.app.bGameRunning)
	ogl.Update (0);
}

//------------------------------------------------------------------------------

void CBackground::DrawBox (void)
{
::DrawBox (*this);
}

//------------------------------------------------------------------------------
// Redraw a part of the menu area's background

void CBackground::DrawArea (int left, int top, int right, int bottom)
{
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
ogl.SetBlending (false);
if (!backgroundManager.Shadow ()) {
	CCanvas canvas;
	canvas.Setup (this);
	*((CViewport*) &canvas) += CViewport (LHX (10), LHX (10), 0, 0);
	canvas.Activate ();
	m_bitmap->RenderFixed (NULL, left, top, width, height); //, LHX (10), LHY (10));
	canvas.Deactivate ();
	}
else {
	Activate ();
	m_bitmap->RenderFixed (NULL, left, top, width, height); //, 0, 0);

	gameStates.render.grAlpha = GrAlpha (2 * 7);
	ogl.SetBlending (true);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 200);
	OglDrawFilledRect (right - 5, top + 5, right - 6, bottom - 5);
	OglDrawFilledRect (right - 4, top + 4, right - 5, bottom - 5);
	OglDrawFilledRect (right - 3, top + 3, right - 4, bottom - 5);
	OglDrawFilledRect (right - 2, top + 2, right - 3, bottom - 5);
	OglDrawFilledRect (right - 1, top + 1, right - 2, bottom - 5);
	OglDrawFilledRect (right + 0, top + 0, right - 1, bottom - 5);
	OglDrawFilledRect (left + 5, bottom - 5, right, bottom - 6);
	OglDrawFilledRect (left + 4, bottom - 4, right, bottom - 5);
	OglDrawFilledRect (left + 3, bottom - 3, right, bottom - 4);
	OglDrawFilledRect (left + 2, bottom - 2, right, bottom - 3);
	OglDrawFilledRect (left + 1, bottom - 1, right, bottom - 2);
	OglDrawFilledRect (left + 0, bottom - 0, right, bottom - 1);

	CCanvas::Current ()->SetColorRGB (255, 255, 255, 50);
	OglDrawFilledRect (left, top + 0, right - 1, top + 1);
	OglDrawFilledRect (left, top + 1, right - 2, top + 2);
	OglDrawFilledRect (left, top + 2, right - 3, top + 3);
	OglDrawFilledRect (left, top + 3, right - 4, top + 4);
	OglDrawFilledRect (left, top + 4, right - 5, top + 5);
	OglDrawFilledRect (left, top + 5, right - 6, top + 6);
	OglDrawFilledRect (left + 0, top + 6, left + 1, bottom - 1);
	OglDrawFilledRect (left + 1, top + 6, left + 2, bottom - 2);
	OglDrawFilledRect (left + 2, top + 6, left + 3, bottom - 3);
	OglDrawFilledRect (left + 3, top + 6, left + 4, bottom - 4);
	OglDrawFilledRect (left + 4, top + 6, left + 5, bottom - 5);
	OglDrawFilledRect (left + 5, top + 6, left + 6, bottom - 6);
	ogl.SetBlending (false);
	Deactivate ();
	}
gameStates.render.grAlpha = 1.0f;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void CBackgroundManager::Create (void)
{
if (!m_bValid) {
	Init ();

	if (!m_wallpapers [0].SetBitmap (LoadCustomWallpaper ()))
		m_wallpapers [0].SetBitmap (LoadMenuBackground ());

	for (int i = 1; i < WALLPAPER_COUNT - 1; i++) {
		if (m_filenames [i] = WallpaperName (i))
			m_wallpapers [i].SetBitmap (LoadWallpaper (m_filenames [i]));
		}
	m_wallpapers [BG_LOADING].SetBitmap (LoadDesktopWallpaper ());
	// inner, black area of automap background is transparent
	m_wallpapers [BG_MAP].Bitmap ()->SetTranspType (2);
	m_wallpapers [BG_MAP].Bitmap ()->AddFlags (BM_FLAG_TRANSPARENT);
	m_bValid = true;
	}
}

//------------------------------------------------------------------------------

void CBackgroundManager::Init (void)
{
for (int i = 0; i < WALLPAPER_COUNT; i++) {
	m_wallpapers [i].SetBitmap (NULL);
	m_wallpapers [i].SetType (BG_WALLPAPER);
	}
m_bShadow = true;
m_bValid = false;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Destroy (void)
{
for (int i = 0; i < WALLPAPER_COUNT; i++)
	if (m_wallpapers [i].Bitmap ())
		delete m_wallpapers [i].Bitmap ();
Init ();
}

//------------------------------------------------------------------------------

void CBackgroundManager::Rebuild (void)
{
Destroy ();
Create ();
}

//------------------------------------------------------------------------------

void CBackgroundManager::Draw (CBackground* bg, bool bUpdate)
{
if (!bg)
	Draw (0);
else {
	m_wallpapers [bg->Wallpaper ()].Draw (false);
	bg->Draw (false);
	}
if (bUpdate)
	ogl.Update (1);
}

//------------------------------------------------------------------------------

void CBackgroundManager::Draw (int nWallpaper)
{
m_wallpapers [nWallpaper].Draw (false);
}

//------------------------------------------------------------------------------

void CBackgroundManager::DrawBox (int left, int top, int right, int bottom, int nLineWidth, float fAlpha, int bForce)
{
	CCanvas	canvas;

canvas.Setup (&gameData.render.frame, left, top, left + right - 1, top + bottom - 1, true);
::DrawBox (canvas);
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadCustomWallpaper (void)
{
if (gameStates.app.bNostalgia)
	return NULL;

gameOpts->menus.altBg.bHave = 0;

CBitmap* bmP = CBitmap::Create (0, 0, 0, 1);
if (!bmP)
	return NULL;

CTGA tga (bmP);

int bModBg = (*gameFolders.szWallpaperDir [1] != '\0');

if (bModBg) {
	if (CFile::Exist (gameOpts->menus.altBg.szName [1], gameFolders.szWallpaperDir [1], 0))
		bModBg = 1;
	else {
		strcpy (gameOpts->menus.altBg.szName [1], "default.tga");
		bModBg = CFile::Exist (gameOpts->menus.altBg.szName [1], gameFolders.szWallpaperDir [1], 0);
		}
	}

//if (filename && strcmp (filename, gameOpts->menus.altBg.szName [bModBg]))
//	return NULL;
if (!tga.Read (gameOpts->menus.altBg.szName [bModBg], gameFolders.szWallpaperDir [bModBg], 
				   (gameOpts->menus.altBg.alpha < 0) ? -1 : (int) (gameOpts->menus.altBg.alpha * 255),
					gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale)) {
	delete bmP;
	gameOpts->menus.altBg.bHave = -1;
	return NULL;
	}
bmP->SetName (m_filenames [0] = gameOpts->menus.altBg.szName [bModBg]);
gameOpts->menus.altBg.bHave = 1;
return bmP;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadMenuBackground (void)
{
for (int i = 0; i < 4; i++) {
	m_filenames [0] = menuBgNames [i][Hires ()];
	if (CFile::Exist (m_filenames [0], gameFolders.szDataDir [0], 0))
		return LoadWallpaper (m_filenames [0]);
	}
return NULL;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadDesktopWallpaper (void)
{
for (int i = 0; i < 4; i++) {
	m_filenames [0] = szDesktopFilenames [i][gameStates.menus.bHires];
	if (CFile::Exist (m_filenames [0], gameFolders.szDataDir [0], 0))
		return LoadWallpaper (m_filenames [0]);
	}
return NULL;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadWallpaper (char* filename)
{
	int width, height;

if (PCXGetDimensions (filename, &width, &height) != PCX_ERROR_NONE) {
	Error ("Could not open menu background file <%s>\n", filename);
	return NULL;
	}

CBitmap* bmP = CBitmap::Create (0, width, height, 1);

if (!bmP) {
	Error ("Not enough memory for menu background\n");
	return NULL;
	}
if (PCXReadBitmap (filename, bmP, bmP->Mode (), 0) != PCX_ERROR_NONE) {
	Error ("Could not read menu background file <%s>\n", filename);
	bmP->Destroy ();
	return NULL;
	}
bmP->SetName (filename);
bmP->SetTranspType (3);
bmP->Bind (0);
return bmP;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Activate (CBackground& bg)
{
bg.Setup (bg.Width (false), bg.Height (false));
Draw (&bg);
bg.Activate ();
}

//------------------------------------------------------------------------------

bool CBackgroundManager::Setup (CBackground& bg, int width, int height, int nType, int nWallPaper)
{
if (!bg.Create (width, height, nType, nWallPaper))
	return false;
bg.SetBitmap (m_wallpapers [BG_MENU].Bitmap ());
return true;
}


//------------------------------------------------------------------------------
