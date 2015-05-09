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

int32_t bHiresBackground;

//------------------------------------------------------------------------------

static const char* szDesktopFilenames [4][2] = {
	{"descentb.pcx", "descent.pcx"},
	{"descntob.pcx", "descento.pcx"},
	{"descentd.pcx", "descentd.pcx"},
	{"descentb.pcx", "descentb.pcx"}
};

#if DBG

static const char* menuBgNames [4][2] = {
	{"menu.pcx", "menub.pcx"},
	{"menuo.pcx", "menuob.pcx"},
	{"menud.pcx", "menud.pcx"},
	{"menub.pcx", "menub.pcx"}
	};

#else

static const char* menuBgNames [4][2] = {
	{"\x01menu.pcx", "\x01menub.pcx"},
	{"\x01menuo.pcx", "\x01menuob.pcx"},
	{"\x01menud.pcx", "\x01menud.pcx"},
	{"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

static const char* szWallpapers [3][2] = {
	{"stars.pcx", "starsb.pcx"},
	{"scores.pcx", "scoresb.pcx"},
	{"map.pcx", "mapb.pcx"}
	};


//------------------------------------------------------------------------------

static inline int32_t Hires (int32_t bHires = -1)
{
return gameStates.app.bDemoData ? 0 : (bHires < 0) ? gameStates.menus.bHires : bHires;
}

//------------------------------------------------------------------------------

const char* WallpaperName (int32_t nType, int32_t bHires)
{
return szWallpapers [nType - 1][Hires (bHires)];
}

//------------------------------------------------------------------------------

static inline void SetBoxBorderColor (CRGBColor& color)
{
CCanvas::Current ()->SetColorRGB (color.Red (), color.Green (), color.Blue (), 255); //PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), 255);
}

//------------------------------------------------------------------------------

static inline void SetBoxFillColor (CRGBColor& color)
{
CCanvas::Current ()->SetColorRGB (color.Red (), color.Green (), color.Blue (), gameData.menu.alpha);
}

//------------------------------------------------------------------------------

static inline void DrawBox (CCanvas& canvas, CRGBColor& boxColor)
{
#if 1
gameStates.render.nFlashScale = 0;
CCanvasColor fontColors [2] = { canvas.FontColor (0), canvas.FontColor (1) };
SetBoxFillColor (boxColor);
ogl.SetTexturing (false);
OglDrawFilledRect (0, 0, canvas.Width (), canvas.Height ());
SetBoxBorderColor (boxColor);
float flw = GLfloat (gameData.menu.nLineWidth) * sqrt (GLfloat (gameData.render.frame.Width ()) / 640.0f);
glLineWidth (flw);
int32_t lw = int32_t (ceil (flw));
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
m_saved.SetName ("menu background backup");
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

void CBackground::Setup (int32_t width, int32_t height)
{
SetupCanvasses ();
CCanvas::Setup (&gameData.render.screen, (gameData.render.frame.Width () - width) / 2, (gameData.render.frame.Height () - height) / 2, width, height, true);
}

//------------------------------------------------------------------------------

bool CBackground::Create (int32_t width, int32_t height, int32_t nType, int32_t nWallpaper)
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
int32_t nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
CViewport::SetLeft (CViewport::Left () - CScreen::Unscaled (gameData.StereoOffset2D ()));
CCanvas::Activate ("CBackground::Activate", &gameData.render.frame);
gameData.SetStereoOffsetType (nOffsetSave);
}

//------------------------------------------------------------------------------

void CBackground::Deactivate (void)
{
int32_t nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
CViewport::SetLeft (CViewport::Left () + CScreen::Unscaled (gameData.StereoOffset2D ()));
CCanvas::Deactivate ();
gameData.SetStereoOffsetType (nOffsetSave);
}

//------------------------------------------------------------------------------

void CBackground::Draw (bool bUpdate)
{
if (m_nType == BG_WALLPAPER) {
	if (!(gameStates.menus.bNoBackground || (gameStates.app.bGameRunning && !gameStates.app.bNostalgia))) {
		gameData.render.frame.Activate ("CBackGround::Draw (frame)");
		int32_t bCartoonize;
		if (gameOptions [0].menus.altBg.bCartoonize)
			bCartoonize = gameStates.render.SetCartoonStyle (-1);
		gameStates.render.bClampBlur = 1;
		m_bitmap->RenderStretched ();
		gameStates.render.bClampBlur = 0;
		if (gameOptions [0].menus.altBg.bCartoonize)
			gameStates.render.SetCartoonStyle (bCartoonize);
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
::DrawBox (*this, m_boxColor);
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
	int32_t d = LHX (10);
	*((CViewport*) &canvas) += CViewport (-d, -d, 2 * d, 2 * d);
	canvas.Activate ("CBackground::DrawArea");
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

	int32_t right = Width ();
	int32_t bottom = Height ();

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

	for (int32_t i = 1; i < WALLPAPER_COUNT - 1; i++) {
		if ((m_filenames [i] = WallpaperName (i)))
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
for (int32_t i = 0; i < WALLPAPER_COUNT; i++) {
	m_wallpapers [i].SetBitmap (NULL);
	m_wallpapers [i].SetType (BG_WALLPAPER);
	}
m_bShadow = true;
m_bValid = false;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Destroy (void)
{
for (int32_t i = 0; i < WALLPAPER_COUNT; i++)
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

void CBackgroundManager::Draw (int32_t nWallpaper)
{
m_wallpapers [nWallpaper].Draw (false);
}

//------------------------------------------------------------------------------

void CBackgroundManager::DrawBox (int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t nLineWidth, float fAlpha, int32_t bForce)
{
	CCanvas		canvas;
	CRGBColor	boxColor;
	
canvas.Setup (&gameData.render.frame, left - gameData.StereoOffset2D (), top, right - left + 1, bottom - top + 1, true);
canvas.Activate ("CBackgroundManager::DrawBox", &gameData.render.frame);
boxColor.Set (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38));
::DrawBox (canvas, boxColor);
canvas.Deactivate ();
}

//------------------------------------------------------------------------------

extern bool bRegisterBitmaps;

CBitmap* CBackgroundManager::LoadCustomWallpaper (const char* filename)
{
if (gameStates.app.bNostalgia)
	return NULL;

gameOpts->menus.altBg.bHave = 0;

bool b = bRegisterBitmaps;
bRegisterBitmaps = false;
CBitmap* pBm = CBitmap::Create (0, 0, 0, 1);
bRegisterBitmaps = b;
if (!pBm)
	return NULL;

char	szFile [FILENAME_LEN], szFolder [FILENAME_LEN], szExt [FILENAME_LEN];
char	*pszFile, *pszFolder;

CTGA tga (pBm);

int32_t bModBg = 0;

if (filename) {
	CFile::SplitPath (filename, szFolder, szFile, szExt);
	strcat (szFile, szExt);
	if (CFile::Exist (szFile, szFolder, 0)) {
		pszFile = szFile;
		pszFolder = szFolder;
		bModBg = -1;
		}
	}
if (bModBg == 0) {
	bModBg = (*gameFolders.mods.szWallpapers != '\0');
	if (bModBg) {
		if (CFile::Exist (gameOpts->menus.altBg.szName [1], gameFolders.mods.szWallpapers, 0))
			bModBg = 1;
		else {
			strcpy (gameOpts->menus.altBg.szName [1], "default.tga");
			bModBg = CFile::Exist (gameOpts->menus.altBg.szName [1], gameFolders.mods.szWallpapers, 0);
		}
	}
	pszFile = gameOpts->menus.altBg.szName [bModBg];
	pszFolder = bModBg ? gameFolders.mods.szWallpapers : gameFolders.user.szWallpapers;
	}

//if (filename && strcmp (filename, gameOpts->menus.altBg.szName [bModBg]))
//	return NULL;
if (!tga.Read (pszFile, pszFolder, 
				   (gameOpts->menus.altBg.alpha < 0) ? -1 : (int32_t) (gameOpts->menus.altBg.alpha * 255),
					gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale)) {
	delete pBm;
	gameOpts->menus.altBg.bHave = -1;
	return NULL;
	}
pBm->SetName (m_filenames [0] = (bModBg < 0) ? filename : gameOpts->menus.altBg.szName [bModBg]);
gameOpts->menus.altBg.bHave = 1;
return pBm;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadMenuBackground (void)
{
for (int32_t i = 0; i < 4; i++) {
	m_filenames [0] = menuBgNames [i][Hires ()];
	if (CFile::Exist (m_filenames [0], gameFolders.game.szData [0], 0))
		return LoadWallpaper (m_filenames [0]);
	}
return NULL;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadDesktopWallpaper (void)
{
for (int32_t i = 0; i < 4; i++) {
	m_filenames [0] = szDesktopFilenames [i][gameStates.menus.bHires];
	if (CFile::Exist (m_filenames [0], gameFolders.game.szData [0], 0))
		return LoadWallpaper (m_filenames [0]);
	}
return NULL;
}

//------------------------------------------------------------------------------

CBitmap* CBackgroundManager::LoadWallpaper (const char* filename)
{
	int32_t width, height;

if (PCXGetDimensions (filename, &width, &height) != PCX_ERROR_NONE) {
	Error ("Could not open menu background file <%s>\n", filename);
	return NULL;
	}

bool b = bRegisterBitmaps;
bRegisterBitmaps = false;
CBitmap* pBm = CBitmap::Create (0, width, height, 1);
bRegisterBitmaps = b;

if (!pBm) {
	Error ("Not enough memory for menu background\n");
	return NULL;
	}
if (PCXReadBitmap (filename, pBm, pBm->Mode (), 0) != PCX_ERROR_NONE) {
	Error ("Could not read menu background file <%s>\n", filename);
	pBm->Destroy ();
	return NULL;
	}
pBm->SetName (filename);
pBm->SetTranspType (3);

int32_t bCartoonize;
if (gameOptions [0].menus.altBg.bCartoonize)
	bCartoonize = gameStates.render.SetCartoonStyle (-1);
gameStates.render.bClampBlur = 1;
pBm->Bind (0);
gameStates.render.bClampBlur = 0;
if (gameOptions [0].menus.altBg.bCartoonize)
	gameStates.render.SetCartoonStyle (bCartoonize);
return pBm;
}

//------------------------------------------------------------------------------

void CBackgroundManager::Activate (CBackground& bg)
{
bg.Setup (bg.Width (), bg.Height ());
Draw (&bg);
bg.Activate ();
}

//------------------------------------------------------------------------------

bool CBackgroundManager::Setup (CBackground& bg, int32_t width, int32_t height, int32_t nType, int32_t nWallPaper)
{
if (!bg.Create (width, height, nType, nWallPaper))
	return false;
bg.SetBitmap (m_wallpapers [BG_MENU].Bitmap ());
return true;
}


//------------------------------------------------------------------------------
