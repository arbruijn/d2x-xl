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
if (!gameStates.app.bNostalgia && (nType == -BG_TOPMENU) && (nWallpaper == -BG_SCORES)) {
	m_nType = BG_SUBMENU;
	m_nWallpaper = BG_STANDARD;
	}
else {
	m_nType = abs (nType);
	m_nWallpaper = abs (nWallpaper);
	}
Setup (width, height);
return true;
}

//------------------------------------------------------------------------------

void CBackground::Activate (void)
{
int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
CViewport::SetLeft (CViewport::Left () - CScreen::Unscaled (gameData.StereoOffset2D ()));
CCanvas::Activate ("Background", &gameData.render.frame);
gameData.SetStereoOffsetType (nOffsetSave);
}

//------------------------------------------------------------------------------

void CBackground::Deactivate (void)
{
int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
CViewport::SetLeft (CViewport::Left () + CScreen::Unscaled (gameData.StereoOffset2D ()));
CCanvas::Deactivate ();
gameData.SetStereoOffsetType (nOffsetSave);
}

//------------------------------------------------------------------------------

void CBackground::Draw (bool bUpdate)
{
if (m_nType == BG_WALLPAPER) {
	if (!(gameStates.menus.bNoBackground || (gameStates.app.bGameRunning && !gameStates.app.bNostalgia))) {
		gameData.render.frame.Activate ("Frame");
		m_bitmap->RenderStretched ();
		PrintVersionInfo ();
		gameData.render.frame.Deactivate ();
		}
	}
else if (m_nType == BG_SUBMENU) {
	if (gameStates.app.bNostalgia)
		DrawArea ();
	else 
		DrawBox ();
	}
if (bUpdate && !gameStates.app.bGameRunning)
	ogl.Update (0);
}

//------------------------------------------------------------------------------

void CBackground::DrawBox (void)
{
Activate ();
::DrawBox (*this);
Deactivate ();
}

//------------------------------------------------------------------------------
// Redraw a part of the menu area's background

void CBackground::DrawArea (void)
{
ogl.SetBlending (false);
if (!backgroundManager.Shadow ()) {
	CCanvas canvas;
	canvas.Setup (this);
	int d = LHX (10);
	*((CViewport*) &canvas) += CViewport (-d, -d, 2 * d, 2 * d);
	canvas.Activate ("Background");
	m_bitmap->RenderFixed (NULL, 0, 0, Width (), Height ()); 
	canvas.Deactivate ();
	}
else {
	Activate ();
	m_bitmap->RenderFixed (NULL, 0, 0, Width (), Height ()); 

	gameStates.render.grAlpha = GrAlpha (2 * 7);
	ogl.SetBlending (true);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	CCanvas::Current ()->SetColorRGB (0, 0, 0, 200);

	int right = Width ();
	int bottom = Height ();

	OglDrawFilledRect (right - 5, 5, right - 6, bottom - 5);
	OglDrawFilledRect (right - 4, 4, right - 5, bottom - 5);
	OglDrawFilledRect (right - 3, 3, right - 4, bottom - 5);
	OglDrawFilledRect (right - 2, 2, right - 3, bottom - 5);
	OglDrawFilledRect (right - 1, 1, right - 2, bottom - 5);
	OglDrawFilledRect (right + 0, 0, right - 1, bottom - 5);
	OglDrawFilledRect (5, bottom - 5, right, bottom - 6);
	OglDrawFilledRect (4, bottom - 4, right, bottom - 5);
	OglDrawFilledRect (3, bottom - 3, right, bottom - 4);
	OglDrawFilledRect (2, bottom - 2, right, bottom - 3);
	OglDrawFilledRect (1, bottom - 1, right, bottom - 2);
	OglDrawFilledRect (0, bottom - 0, right, bottom - 1);

	CCanvas::Current ()->SetColorRGB (255, 255, 255, 50);
	OglDrawFilledRect (0, 0, right - 1, 1);
	OglDrawFilledRect (0, 1, right - 2, 2);
	OglDrawFilledRect (0, 2, right - 3, 3);
	OglDrawFilledRect (0, 3, right - 4, 4);
	OglDrawFilledRect (0, 4, right - 5, 5);
	OglDrawFilledRect (0, 5, right - 6, 6);
	OglDrawFilledRect (0, 6, 1, bottom - 1);
	OglDrawFilledRect (1, 6, 2, bottom - 2);
	OglDrawFilledRect (2, 6, 3, bottom - 3);
	OglDrawFilledRect (3, 6, 4, bottom - 4);
	OglDrawFilledRect (4, 6, 5, bottom - 5);
	OglDrawFilledRect (5, 6, 6, bottom - 6);
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

canvas.Setup (&gameData.render.frame, left - gameData.StereoOffset2D (), top, left + right - 1, top + bottom - 1, true);
canvas.Activate ("Background", &gameData.render.frame);
::DrawBox (canvas);
canvas.Deactivate ();
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
bg.Setup (bg.Width (), bg.Height ());
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
