/* $Id: newmenu.c, v 1.25 2003/11/27 04:52:18 btb Exp $ */
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
#include <windows.h>
#include <stddef.h>
#endif
#ifdef __macosx__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <limits.h>

#include "error.h"
#include "inferno.h"
#include "gr.h"
#include "mono.h"
#include "songs.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "text.h"
#include "findfile.h"

#include "menu.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gamepal.h"
#include "network.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "digi.h"

#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "config.h"
#include "player.h"
#include "newdemo.h"
#include "kconfig.h"
#include "strutil.h"
#include "ogl_init.h"
#include "render.h"
#include "input.h"
#include "gameseq.h"
#include "netmenu.h"

#if defined (TACTILE)
 #include "tactile.h"
#endif

#define MAXDISPLAYABLEITEMS 15

#define LHX(x)      (gameStates.menus.bHires? 2 * (x) : x)
#define LHY(y)      (gameStates.menus.bHires? (24 * (y)) / 10 : y)

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

ubyte			menuPalette [256*3];
static		char *pszCurBg = NULL;

grsBitmap	*pAltBg = NULL;

typedef struct nm_control {
	int	scWidth,
			scHeight,
			x, 
			y, 
			xOffs, 
			yOffs,
			width,
			height,
			w, 
			h, 
			aw, 
			tw, 
			th, 
			ty,
			twidth, 
			right_offset, 
			nStringHeight,
			bTinyMode,
			nMenus, 
			nOthers, 
			nMaxNoScroll, 
			nMaxOnMenu,
			nMaxDisplayable,
			nScrollOffset,
			bIsScrollBox,
			nDisplayMode,
			bValid;
} nm_control;

//------------------------------------------------------------------------------

extern int bSaveScreenShot;

void GameLoop (int, int);
void GameRenderFrame ();

int ExecMenu4 (char * title, char * subtitle, int nItems, tMenuItem * item, 
					  void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem), 
					  int *cItemP, char * filename, int width, int height, int bTinyMode);
void ShowNetGameInfo (int choice);


int bNewMenuFirstTime = 1;
//--unused-- int Newmenu_fade_in = 1;

grsBitmap nm_background, nm_background_save;

#define MESSAGEBOX_TEXT_SIZE 10000		// How many characters in messagebox
#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

char Pauseable_menu=0;
char bAlreadyShowingInfo=0;

void NMInitBackground (char *filename, bkg *bg, int x, int y, int w, int h, int bRedraw);

//------------------------------------------------------------------------------

inline void NMFreeTextBm (tMenuItem *item)
{
	int	i;

for (i = 0; i < 2; i++) {
	GrFreeBitmap (item->text_bm [i]);
	item->text_bm [i] = NULL;
	}
}

//------------------------------------------------------------------------------

void NMFreeAllTextBms (tMenuItem *items, int nItems)
{
for (; nItems; nItems--, items++)
	NMFreeTextBm (items);
}

//------------------------------------------------------------------------------

void GrRemapMonoFonts ();

void NMRemoveBackground (bkg *bg) 
{
if (gameOpts->menus.nStyle) {
	if (bg) {
		if (bg->background) {
			if (bg->background == pAltBg)
				NMFreeAltBg (0);
			else
				GrFreeBitmap (bg->background);
			bg->background = NULL;
			}
		if (bg->pszPrevBg)
			pszCurBg = bg->pszPrevBg;
#if 1
		if (bg->menu_canvas) {
			WINDOS (DDGrFreeSubCanvas (bg->menu_canvas), 
					  GrFreeSubCanvas (bg->menu_canvas));
			WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));			
			bg->menu_canvas = NULL;
			}
#endif
		}
	}
if (gameStates.app.bGameRunning) {
	GrPaletteStepLoad (NULL);
#if 1
	RemapFontsAndMenus (1);
	GrRemapMonoFonts ();
#endif
	}
}

//------------------------------------------------------------------------------

void _CDECL_ NewMenuClose (void)	
{
LogErr ("unloading menu data\n");
if (nm_background.bmTexBuf)
	D2_FREE (nm_background.bmTexBuf);
if (nm_background_save.bmTexBuf)
	D2_FREE (nm_background_save.bmTexBuf);
bNewMenuFirstTime = 1;
}

//------------------------------------------------------------------------------

ubyte bgPalette [256*3];
//should be called whenever the palette changes
void NMRemapBackground ()
{
if (!bNewMenuFirstTime) {
	if (!nm_background.bmTexBuf)
		nm_background.bmTexBuf = D2_ALLOC (nm_background.bmProps.w * nm_background.bmProps.h);
	memcpy (nm_background.bmTexBuf, nm_background_save.bmTexBuf, nm_background.bmProps.w * nm_background.bmProps.h);
	//GrRemapBitmapGood (&nm_background, bgPalette, -1, -1);
	}
}

//------------------------------------------------------------------------------

void NMBlueBox (int x1, int y1, int x2, int y2)
{
if ((gameOpts->menus.nStyle == 1)) {// && ((gameOpts->menus.altBg.bHave < 1) || gameStates.app.bGameRunning)) {
	if (x1 <= 0)
		x1 = 1;
	if (y1 <= 0)
		y1 = 1;
	if (x2 >= grdCurScreen->scWidth)
		x2 = grdCurScreen->scWidth - 1;
	if (y2 >= grdCurScreen->scHeight)
		y2 = grdCurScreen->scHeight - 1;
#if 0
	if ((gameOpts->menus.altBg.bHave > 0) && !gameStates.app.bGameRunning)
		GrSetColorRGB (PAL2RGBA (15), PAL2RGBA (15), PAL2RGBA (27), 80);
	else
#endif
		GrSetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), gameData.menu.alpha);
	gameStates.render.grAlpha = (float) gameData.menu.alpha / 255.0f;
	GrURect (x1, y1, x2, y2);
	gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
	GrSetColorRGB (PAL2RGBA (22), PAL2RGBA (22), PAL2RGBA (38), 255);
	//GrSetColorRGBi (D2BLUE_RGBA);
	GrUBox (x1, y1, x2, y2);
	}
}

//------------------------------------------------------------------------------

void NMLoadAltBg (void)
{
if (gameStates.app.bNostalgia || !gameOpts->menus.nStyle)
	gameOpts->menus.altBg.bHave = -1;
else if (gameOpts->menus.altBg.bHave > 0)
	gameOpts->menus.altBg.bHave++;
else if (!gameOpts->menus.altBg.bHave) {
	if ((pAltBg = GrCreateBitmap (0, 0, 1))) {
		memset (pAltBg, 0, sizeof (*pAltBg));
		gameOpts->menus.altBg.bHave = 
			ReadTGA (gameOpts->menus.altBg.szName, 
#ifdef __linux__
						gameFolders.szTempDir,
#else
						NULL, 
#endif
						pAltBg, 
						(gameOpts->menus.altBg.alpha < 0) ? -1 : (int) (gameOpts->menus.altBg.alpha * 255), 
						gameOpts->menus.altBg.brightness, gameOpts->menus.altBg.grayscale, 0) ? 1 : -1;
		}
	else
		gameOpts->menus.altBg.bHave = -1;
	}
}

//------------------------------------------------------------------------------

int NMFreeAltBg (int bForce)
{
if (!pAltBg)
	return 0;
if (!bForce && (gameOpts->menus.altBg.bHave == 1))
	return 0;
if (bForce || !--gameOpts->menus.altBg.bHave) {
	gameOpts->menus.altBg.bHave = 0;
	GrFreeBitmap (pAltBg);
	pAltBg = NULL;
	}	
return 1;
}

//------------------------------------------------------------------------------

#include <math.h>

extern char szLastPaletteLoaded [];

void NMLoadBackground (char * filename, bkg *bg, int bRedraw)
{
	int			pcx_error;
	int			width, height;
	grsBitmap	*bmP = bg ? bg->background : NULL;

	//@@//I think this only gets called to fill the whole screen
	//@@Assert (grdCurCanv->cvBitmap.bmProps.w == 320);
	//@@Assert (grdCurCanv->cvBitmap.bmProps.h == 200);
#if 0
	if (bRedraw)
		bRedraw = gameOpts->menus.nStyle && bmP && pszCurBg && filename && !strcmp (pszCurBg, filename);
#endif
if (!(bRedraw && gameOpts->menus.nStyle)) {
	if (bmP && (bmP != pAltBg)) {
		GrFreeBitmap (bmP);
		bmP = NULL;
		}
	if (!filename)
		filename = pszCurBg;
	else if (!pszCurBg)
		pszCurBg = filename;
	if (!filename)
		filename = gameStates.app.bDemoData ? MENU_PCX_SHAREWARE : MENU_PCX_FULL;
	if (!(filename && strcmp (filename, MENU_PCX_FULL) && strcmp (filename, MENU_PCX_SHAREWARE))) {
		NMLoadAltBg ();
		if (gameOpts->menus.altBg.bHave > 0)
			bmP = pAltBg;
		}
	if (gameOpts->menus.altBg.bHave < 1) {
		if (!filename)
			return;
		pcx_error = pcx_get_dimensions (filename, &width, &height);
		if (pcx_error != PCX_ERROR_NONE)
			Error ("Could not open pcx file <%s>\n", filename);
		bmP = GrCreateBitmap (width, height, 1);
		Assert (bmP != NULL);
		}
	pcx_error = PCXReadBitmap (filename, (gameOpts->menus.altBg.bHave > 0) ? NULL : bmP, bmP ? bmP->bmProps.nType : 0, 0);
	Assert (pcx_error == PCX_ERROR_NONE);
// Remap stuff. this code is kind of a hack. Before we bring up the menu, we need to
// do some stuff to make sure the palette is ok. First,we need to get our current palette 
// into the 2d's array, so the remapping will work.  Second, we need to remap the fonts.  
// Third, we need to fill in part of the fade tables so the darkening of the menu edges works.
	GrPaletteStepLoad (gameData.render.pal.pCurPal);
	//GrCopyPalette (grPalette, menuPalette, sizeof (grPalette));
	//RemapFontsAndMenus (1);
	//GrRemapMonoFonts ();
	strcpy (szLastPaletteLoaded, "");		//force palette load next time
	if (bg) {
		if (gameOpts->menus.nStyle && gameOpts->menus.bFastMenus)
			OglLoadBmTexture (bmP, 0, 0, 1);
		bg->pszPrevBg = pszCurBg;
		pszCurBg = filename;
		}
	}
if (!bmP)
	return;
WIN (DDGRLOCK (dd_grd_curcanv));
if (!(gameStates.app.bGameRunning && gameOpts->menus.nStyle))
	ShowFullscreenImage (bmP);
WIN (DDGRUNLOCK (dd_grd_curcanv));
if (bg)
	bg->background = bmP;
else
	GrFreeBitmap (bmP);
}

//------------------------------------------------------------------------------

#define MENU_BACKGROUND_BITMAP_HIRES (CFExist ("scoresb.pcx", gameFolders.szDataDir, 0)?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (CFExist ("scores.pcx", gameFolders.szDataDir, 0)?"scores.pcx":"scoresb.pcx") // Mac datafiles only have scoresb.pcx

#define MENU_BACKGROUND_BITMAP (gameStates.menus.bHires?MENU_BACKGROUND_BITMAP_HIRES:MENU_BACKGROUND_BITMAP_LORES)

int bHiresBackground;
int bNoDarkening = 0;

void NMDrawBackground (bkg *bg, int x1, int y1, int x2, int y2, int bRedraw)
{
	int w, h;

if (gameOpts->menus.nStyle) {
	if (bg)
		bg->bIgnoreBg = 1;
	NMInitBackground (NULL, bg, x1, y1, x2 - x1 + 1, y2 - y1 + 1, bRedraw);
//	if (!(bg && bg->menu_canvas))
//		NMBlueBox (x1, y1, x2, y2);
	}
else {
	if (bNewMenuFirstTime || gameStates.menus.bHires!=bHiresBackground)	{
		int pcx_error;

		if (bNewMenuFirstTime) {
			atexit (NewMenuClose);
			bNewMenuFirstTime = 0;
			nm_background_save.bmTexBuf = NULL;
			}
		else {
			if (nm_background_save.bmTexBuf)
				D2_FREE (nm_background_save.bmTexBuf);
			if (nm_background.bmTexBuf)
				D2_FREE (nm_background.bmTexBuf);
			}
		pcx_error = PCXReadBitmap (MENU_BACKGROUND_BITMAP, &nm_background_save, BM_LINEAR, 0);
		Assert (pcx_error == PCX_ERROR_NONE);
		nm_background = nm_background_save;
		nm_background.bmTexBuf = NULL;		
		NMRemapBackground ();
		bHiresBackground = gameStates.menus.bHires;
		}
	if (x1 < 0) 
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	w = x2-x1+1;
	h = y2-y1+1;
	//if (w > nm_background.bmProps.w) w = nm_background.bmProps.w;
	//if (h > nm_background.bmProps.h) h = nm_background.bmProps.h;
	x2 = x1 + w - 1;
	y2 = y1 + h - 1;
#if 0
	{
		grsBitmap *tmp = GrCreateBitmap (w, h, 1);
		GrBitmapScaleTo (&nm_background, tmp);
		WIN (DDGRLOCK (dd_grd_curcanv);
		glDisable (GL_BLEND);
		if (bNoDarkening)
			GrBmBitBlt (w, h, x1, y1, LHX (10), LHY (10), tmp, &(grdCurCanv->cvBitmap));
		else
			GrBmBitBlt (w, h, x1, y1, 0, 0, tmp, &(grdCurCanv->cvBitmap));
		glEnable (GL_BLEND);
		GrFreeBitmap (tmp);
	}
#else
	WIN (DDGRLOCK (dd_grd_curcanv));
	glDisable (GL_BLEND);
	if (bNoDarkening)
		GrBmBitBlt (w, h, x1, y1, LHX (10), LHY (10), &nm_background, &(grdCurCanv->cvBitmap));
	else
		GrBmBitBlt (w, h, x1, y1, 0, 0, &nm_background, &(grdCurCanv->cvBitmap));
	glEnable (GL_BLEND);
#endif
	if (!bNoDarkening) {
		// give the menu background box a 3D look by changing its edges' brightness
		gameStates.render.grAlpha = 2*7;
		GrSetColorRGB (0, 0, 0, 255);
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
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	}
gameStates.render.grAlpha = GR_ACTUAL_FADE_LEVELS;
}

//------------------------------------------------------------------------------

void NMInitBackground (char *filename, bkg *bg, int x, int y, int w, int h, int bRedraw)
{
	static char *pszMenuPcx = NULL;
	int bVerInfo, bBlueBox;

GrPaletteStepUp (0, 0, 0);
if (!pszMenuPcx)
	pszMenuPcx = MENU_PCX_NAME;
bVerInfo = filename && !strcmp (filename, pszMenuPcx);
bBlueBox = gameOpts->menus.nStyle && (!bVerInfo || (gameOpts->menus.altBg.bHave > 0));
if (filename || gameOpts->menus.nStyle) {	// background image file present
	NMLoadBackground (filename, bg, bRedraw);
	if (bVerInfo)
		PrintVersionInfo ();
	else if (bBlueBox) {
		if (bg && (bg->menu_canvas || bg->bIgnoreCanv)) {
			if (bg->menu_canvas)
				WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), 
						  GrSetCurrentCanvas (bg->menu_canvas));
//			NMLoadBackground (filename, bg, bRedraw);
			if (bVerInfo)
				PrintVersionInfo ();
			}
		NMBlueBox (x, y, x + w, y + h);
		}
	if (bg && !bg->bIgnoreBg) {
		GrPaletteStepLoad (NULL);
		bg->saved = NULL;
		if (!gameOpts->menus.nStyle) {
			if (!bg->background) {
				bg->background = GrCreateBitmap (w, h, 4);
				Assert (bg->background != NULL);
				}
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBmBitBlt (w, h, 0, 0, x, y, &grdCurCanv->cvBitmap, bg->background);
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			}
		}
	} 
// Save the background of the display
//	Win95 must refer to the screen as a ddgrs_canvas, so...
if (!(gameOpts->menus.nStyle && bRedraw)) {
	if (bg && !(bg->menu_canvas || bg->bIgnoreCanv)) {
		if (gameOpts->menus.nStyle) {
			x = y = 0;
			w = grdCurScreen->scWidth;
			h = grdCurScreen->scHeight;
			}
		WINDOS (bg->menu_canvas = DDGrCreateSubCanvas (dd_grd_screencanv, x, y, w, h), 
				  bg->menu_canvas = GrCreateSubCanvas (&grdCurScreen->scCanvas, x, y, w, h)
			);
		WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), 
				  GrSetCurrentCanvas (bg->menu_canvas));
			}
	}
if (bg && !(gameOpts->menus.nStyle || filename)) {
	// Save the background under the menu...
#ifdef TACTILE
	if (TactileStick)
		DisableForces ();
#endif
	if (!(gameOpts->menus.nStyle || bg->saved)) {
		bg->saved = GrCreateBitmap (w, h, 1);
		Assert (bg->saved != NULL);
		}
	bg->saved->bmPalette = defaultPalette;
	if (!gameOpts->menus.nStyle) {
		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBmBitBlt (w, h, 0, 0, 0, 0, &grdCurCanv->cvBitmap, bg->saved);
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		}
	WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));
	NMDrawBackground (bg, x, y, x + w - 1, y + h - 1, bRedraw);
	GrUpdate (0);
	WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), 
			  GrSetCurrentCanvas (bg->menu_canvas));
	bg->background = GrCreateSubBitmap (&nm_background, 0, 0, w, h);
	GrUpdate (0);
	}
}

//------------------------------------------------------------------------------

void NMRestoreBackground (int sx, int sy, int dx, int dy, int w, int h)
{
	int x1, x2, y1, y2;

	x1 = sx; 
	x2 = sx+w-1;
	y1 = sy; 
	y2 = sy+h-1;

	if (x1 < 0) 
		x1 = 0;
	if (y1 < 0) 
		y1 = 0;

	if (x2 >= nm_background.bmProps.w) 
		x2=nm_background.bmProps.w-1;
	if (y2 >= nm_background.bmProps.h) 
		y2=nm_background.bmProps.h-1;

	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	WIN (DDGRLOCK (dd_grd_curcanv));
	GrBmBitBlt (w, h, dx, dy, x1, y1, &nm_background, &(grdCurCanv->cvBitmap));
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

short NMSetItemColor (tMenuItem *item, int bIsCurrent, int bTiny) 
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
		GrSetFontColorRGBi (gameData.menu.colorOverride, 1, 0, 0);
	else if (item->text [0] == '\t')
		GrSetFontColorRGBi (gameData.menu.tabbedColor, 1, 0, 0);
	else 
		GrSetFontColorRGBi (gameData.menu.tinyColors [bIsCurrent][item->unavailable], 1, 0, 0);
	}
else {
	if (bIsCurrent)
		grdCurCanv->cvFont = SELECTED_FONT;
	else
		grdCurCanv->cvFont = NORMAL_FONT;
	}
return grdCurCanv->cvFontFgColor.index;
}

//------------------------------------------------------------------------------

int nTabIndex = -1;
int nTabs [] = {15, 87, 124, 162, 228, 253};
int qqqi = 0;

void NMHotKeyString (tMenuItem *item, int bIsCurrent, int bTiny, int bCreateTextBms, int nDepth)
{
#if 1
	grsBitmap	*bm = item->text_bm [bIsCurrent];

if (!*item->text)
	return;
if (item->color)
	GrSetFontColorRGBi (item->color, 1, 0, 0);
else
	NMSetItemColor (item, bIsCurrent, bTiny);
if (/*!strchr (item->text, '\t') &&*/ bCreateTextBms && gameOpts->menus.bFastMenus &&
	 (bm || (bm = CreateStringBitmap (item->text, MENU_KEY (item->key, -1), 
												 gameData.menu.keyColor,
												 nTabs, item->centered, item->w, 0)))) {
	OglUBitBltI (bm->bmProps.w, bm->bmProps.h, item->x, item->y, bm->bmProps.w, bm->bmProps.h, 0, 0, 
					 bm, &grdCurCanv->cvBitmap, 0, 1);
	item->text_bm [bIsCurrent] = bm;
	}
else 
#endif
	{
	int	w, h, aw, l, i, 
			x = item->x, 
			y = item->y;
	char	*t, *ps = item->text, s [256], ch = 0, ch2;

if (!nDepth)
	item->textSave = item->text;
if ((t = strchr (ps, '\n'))) {
	strncpy (s, ps, sizeof (s));
	item->text = s;
	GrGetStringSize (s, &w, &h, &aw);
	do {
		if ((t = strchr (item->text, '\n')))
			*t = '\0';
		NMHotKeyString (item, 0, bTiny, 0, nDepth + 1);
		if (!t)
			break;
		item->text = t + 1;
		item->y += h / 2;
		item->x = item->xSave;
		nTabIndex = -1;
		} while (*(item->text));
	}
else if ((t = strchr (ps, '\t'))) {
	strncpy (s, ps, sizeof (s));
	item->text = s;
	GrGetStringSize (s, &w, &h, &aw);
	do {
		if ((t = strchr (item->text, '\t')))
			*t = '\0';
		NMHotKeyString (item, 0, bTiny, 0, nDepth + 1);
		if (!t)
			break;
		item->text = t + 1;
		nTabIndex++;
	} while (*(item->text));
	nTabIndex = -1;
	item->text = ps;
	item->y = y;
	}
else {
	l = (int) strlen (item->text);
	if (bIsCurrent || !item->key)
		i = l;
	else {
		ch = MENU_KEY (item->key, *ps);
		for (i = 0; ps [i]; i++)
			if (ps [i] == ch)
				break;
		}
	strncpy (s, ps, sizeof (s));
	s [i] = '\0';
	GrGetStringSize (s, &w, &h, &aw);
	if (nTabIndex >= 0) {
		x += LHX (nTabs [nTabIndex]);
		if (!gameStates.multi.bSurfingNet)
			x += item->w - w;
		}
	//item->x = x + w;
	if (i) {
		GrString (x, y, s, NULL);
#ifdef _DEBUG
		//GrUpdate (0);
#endif
		}
	if (i < l) {	// print the hotkey
		x += w;
		s [i] = ch;
		ch2 = s [++i];
		s [i] = '\0';
		NMSetItemColor (item, 1, bTiny);
		GrString (x, y, s + i - 1, NULL);
#ifdef _DEBUG
		//GrUpdate (0);
#endif
		NMSetItemColor (item, 0, bTiny);
		if (i < l) { // print text following the hotkey
			GrGetStringSize (s + i - 1, &w, &h, &aw);
			x += w;
			s [i] = ch2;
			GrString (x, y, s + i, NULL);
			}
		}
	}
#if 0
if (!nDepth) {
	item->text = item->textSave;
	item->x = item->xSave;
	item->y = item->ySave;
	}
#endif
}
}

//------------------------------------------------------------------------------
// Draw a left justfied string
void NMString (tMenuItem *item, bkg * b, int bIsCurrent, int bTiny)
{
	int w1 = item->w, x = item->x, y = item->y;
	int l, w, h, aw, tx=0, t=0, i;
	char *p, *s1, measure [2] , *s=item->text;
	int XTabs [6];
	static char s2 [1024];

	p=s1=NULL;
	l = (int) strlen (s);
	memcpy (s2, s, l + 1);

	for (i=0;i<6;i++)
		XTabs [i]= (LHX (nTabs [i])) + x;
 
	measure [1]=0;

	if (!gameStates.multi.bSurfingNet) {
		p = strchr (s2, '\t');
		if (p &&(w1>0)) {
			*p = '\0';
			s1 = p+1;
		}
	}
	if (w1 > 0)
		w = w1;
	GrGetStringSize (s2, &w, &h, &aw);
	// CHANGED
	if (curDrawBuffer != GL_BACK)
		GrBmBitBlt (b->background->bmProps.w-15, h+2, 5, y-1, 5, y-1, b->background, &(grdCurCanv->cvBitmap));
	//GrBmBitBlt (w, h, x, y, x, y, b->background, &(grdCurCanv->cvBitmap));

	if (0 && gameStates.multi.bSurfingNet) {
		for (i=0;i<l;i++) {
			if (s2 [i]=='\t' && gameStates.multi.bSurfingNet) {
				x=XTabs [t];
				t++;
				continue;
			}
			measure [0]=s2 [i];
			GrGetStringSize (measure, &tx, &h, &aw);
			GrString (x, y, measure, NULL);
			x+=tx;
		}
	}
	else {
		NMHotKeyString (item, bIsCurrent, bTiny, 1, 0);
		return;
		}         

	if (!gameStates.multi.bSurfingNet && p &&(w1>0)) {
		GrGetStringSize (s1, &w, &h, &aw);
		GrString (x+w1-w, y, s1, NULL);
		*p = '\t';
	}
}

//------------------------------------------------------------------------------
// Draw a slider and it's string
void NMStringSlider (tMenuItem *item, bkg * b, int bIsCurrent, int bTiny)
{
	int	w, h, aw;
	int	w1 = item->w, 
			x = item->x, 
			y = item->y;
	char	*t = item->text, 
			*s = item->saved_text;
	char	*p, *s1;

	s1=NULL;

	p = strchr (s, '\t');
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	GrGetStringSize (s, &w, &h, &aw);
	// CHANGED

		if (curDrawBuffer != GL_BACK)
			GrBmBitBlt (b->background->bmProps.w-15, h, 5, y, 5, y, b->background, &(grdCurCanv->cvBitmap));
		//GrBmBitBlt (w, h, x, y, x, y, b->background, &(grdCurCanv->cvBitmap));

		item->text = s;
		NMHotKeyString (item, bIsCurrent, bTiny, 1, 0);
		item->text = t;
		//GrString (x, y, s, NULL);

		if (p)	{
			GrGetStringSize (s1, &w, &h, &aw);

			// CHANGED
			if (curDrawBuffer != GL_BACK) {
				GrBmBitBlt (w, 1, x+w1-w, y, x+w1-w, y, b->background, &(grdCurCanv->cvBitmap));
				GrBmBitBlt (w, 1, x+w1-w, y+h-1, x+w1-w, y, b->background, &(grdCurCanv->cvBitmap));
				}
			GrString (x+w1-w, y, s1, NULL);

			*p = '\t';
		}
}


//------------------------------------------------------------------------------
// Draw a left justfied string with black background.
void NMStringBlack (bkg * b, int w1, int x, int y, char * s)
{
	int w, h, aw;
	GrGetStringSize (s, &w, &h, &aw);

	if (w1 == 0) 
		w1 = w;

	WIN (DDGRLOCK (dd_grd_curcanv));
		GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
  		GrRect (x-1, y-1, x-1, y+h-1);
		GrRect (x-1, y-1, x+w1-1, y-1);

	 
		GrSetColorRGBi (RGBA_PAL2 (5, 5, 5));
		GrRect (x, y+h, x+w1, y+h);
		GrRect (x+w1, y-1, x+w1, y+h);
     
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (x, y, x+w1-1, y+h-1);
	
		GrString (x+1, y+1, s, NULL);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------
// Draw a right justfied string
void NMRString (tMenuItem *item, bkg * b, int bIsCurrent, int bTiny, char *s)
{
	int	w, h, aw;
	int	w1 = item->right_offset, 
			x = item->x, 
			y = item->y;
	char	*hs;

	GrGetStringSize (s, &w, &h, &aw);
	x -= 3;

	if (w1 == 0) 
		w1 = w;

	// CHANGED
	WIN (DDGRLOCK (dd_grd_curcanv));
		if (curDrawBuffer != GL_BACK)
			GrBmBitBlt (w1, h, x-w1, y, x-w1, y, b->background, &(grdCurCanv->cvBitmap));
		hs = item->text;
		item->text = s;
		h = item->x;
		item->x = x - w;
		NMHotKeyString (item, bIsCurrent, bTiny, 0, 0);
		item->text = hs;
		item->x = h;
//		GrString (x-w, y, s, NULL);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

void NMRStringWXY (bkg * b, int w1, int x, int y, char *s)
{
	int	w, h, aw;

	GrGetStringSize (s, &w, &h, &aw);
	x -= 3;

	if (w1 == 0) 
		w1 = w;

	// CHANGED
	WIN (DDGRLOCK (dd_grd_curcanv));
		if (curDrawBuffer != GL_BACK)
			GrBmBitBlt (w1, h, x-w1, y, x-w1, y, b->background, &(grdCurCanv->cvBitmap));
		GrString (x-w, y, s, NULL);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

#include "timer.h"

//for text items, constantly redraw cursor (to achieve flash)
void NMUpdateCursor (tMenuItem *item)
{
	int w, h, aw;
	fix time = TimerGetApproxSeconds ();
	int x, y;
	char * text = item->text;

	Assert (item->nType==NM_TYPE_INPUT_MENU || item->nType==NM_TYPE_INPUT);

	while (*text)	{
		GrGetStringSize (text, &w, &h, &aw);
		if (w > item->w-10)
			text++;
		else
			break;
	}
	if (*text==0) 
		w = 0;
	x = item->x+w; y = item->y;

WIN (DDGRLOCK (dd_grd_curcanv));
	if (time & 0x8000)
		GrString (x, y, CURSOR_STRING, NULL);
	else {
		GrSetColorRGB (0, 0, 0, 255);
		GrRect (x, y, x+grdCurCanv->cvFont->ftWidth-1, y+grdCurCanv->cvFont->ftHeight-1);
	}
WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

void NMStringInputBox (bkg *b, int w, int x, int y, char * text, int current)
{
	int w1, h1, aw;

	while (*text)	{
		GrGetStringSize (text, &w1, &h1, &aw);
		if (w1 > w-10)
			text++;
		else
			break;
	}
	if (*text == 0)
		w1 = 0;

   NMStringBlack (b, w, x, y, text);
		
	if (current)	{
		GrString (x+w1+1, y, CURSOR_STRING, NULL);
	}
}

//------------------------------------------------------------------------------

void NMGauge (bkg *b, int w, int x, int y, int val, int maxVal, int current)
{
	int w1, h, aw;

GrGetStringSize (" ", &w1, &h, &aw);
if (!w) {
	w = w1 * 30;
	}
w1 = w * val / maxVal;
if (w1 < w) {
	GrSetColorRGB (0, 0, 0, 255);
	GrURect (x + w1 + 1, y, x + w, y + h - 2);
	}
GrSetColorRGB (200, 0, 0, 255);
if (w1) {
	GrURect (x + 1, y, x + w1, y + h - 2);
	}
GrUBox (x, y, x + w - 1, y + h - 1);
}

//------------------------------------------------------------------------------

void NMDrawItem (bkg * b, tMenuItem *item, int bIsCurrent, int bTiny)
{
NMSetItemColor (item, bIsCurrent, bTiny);
if (item->rebuild) {
	NMFreeTextBm (item);
	item->rebuild = 0;
	}
WIN (DDGRLOCK (dd_grd_curcanv));	
	switch (item->nType)	{
	case NM_TYPE_TEXT:
      // grdCurCanv->cvFont=TEXT_FONT;
		// fall through on purpose

	case NM_TYPE_MENU:
		NMString (item, b, bIsCurrent, bTiny);
		break;

	case NM_TYPE_SLIDER:	{
		int h, l;
		char *psz = item->saved_text;

		if (item->value < item->minValue) 
			item->value = item->minValue;
		else if (item->value > item->maxValue) 
			item->value = item->maxValue;
		sprintf (psz, "%s\t%s", item->text, SLIDER_LEFT);
#if 1
		l = (int) strlen (psz);
		h = item->maxValue - item->minValue + 1;
		memset (psz + l, SLIDER_MIDDLE [0], h);
		psz [l + h] = SLIDER_RIGHT [0];
		psz [l + h + 1] = '\0';
#else
		for (j = 0; j < (item->maxValue - item->minValue + 1); j++) {
			sprintf (psz, "%s%s", psz, SLIDER_MIDDLE);
			}
		sprintf (item->saved_text, "%s%s", item->saved_text, SLIDER_RIGHT);
#endif
		psz [item->value + 1 + strlen (item->text) + 1] = SLIDER_MARKER [0];
		NMStringSlider (item, b, bIsCurrent, bTiny);
		}
		break;

	case NM_TYPE_INPUT_MENU:
		if (item->group)	
			NMStringInputBox (b, item->w, item->x, item->y, item->text, bIsCurrent);
		else
			NMString (item, b, bIsCurrent, bTiny);
		break;

	case NM_TYPE_INPUT:
		NMStringInputBox (b, item->w, item->x, item->y, item->text, bIsCurrent);
		break;

	case NM_TYPE_GAUGE:
		NMGauge (b, item->w, item->x, item->y, item->value, item->maxValue, bIsCurrent);
		break;

	case NM_TYPE_CHECK:
		NMString (item, b, bIsCurrent, bTiny);
		if (item->value)
			NMRString (item, b, bIsCurrent, bTiny, CHECKED_CHECK_BOX);
		else														  
			NMRString (item, b, bIsCurrent, bTiny, NORMAL_CHECK_BOX);
		break;

	case NM_TYPE_RADIO:
		NMString (item, b, bIsCurrent, bTiny);
		if (item->value)
			NMRString (item, b, bIsCurrent, bTiny, CHECKED_RADIO_BOX);
		else
			NMRString (item, b, bIsCurrent, bTiny, NORMAL_RADIO_BOX);
		break;

	case NM_TYPE_NUMBER:	
		{
		char text [10];
		if (item->value < item->minValue) 
			item->value=item->minValue;
		if (item->value > item->maxValue) 
			item->value=item->maxValue;
		NMString (item, b, bIsCurrent, bTiny);
		sprintf (text, "%d", item->value);
		NMRString (item, b, bIsCurrent, bTiny, text);
		}
		break;
	}
WIN (DDGRUNLOCK (dd_grd_curcanv));

}

//------------------------------------------------------------------------------

char *nmAllowedChars=NULL;

//returns true if char is bAllowed
int NMCharAllowed (char c)
{
	char *p = nmAllowedChars;

	if (!p)
		return 1;

	while (*p) {
		Assert (p [1]);

		if (c>=p [0] && c<=p [1])
			return 1;

		p += 2;
	}

	return 0;
}

//------------------------------------------------------------------------------

void NMTrimWhitespace (char * text)
{
	int i, l = (int) strlen (text);
	for (i=l-1; i>=0; i--)	{
		if (isspace (text [i]))
			text [i] = 0;
		else
			return;
	}
}

//------------------------------------------------------------------------------

int ExecMenu (char * title, char * subtitle, int nItems, tMenuItem * item, 
					void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem),
					char *filename)
{
	return ExecMenu3 (title, subtitle, nItems, item, subfunction, NULL, filename, -1, -1);
}
int ExecMenutiny (char * title, char * subtitle, int nItems, tMenuItem * item, 
						 void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem))
{
        return ExecMenu4 (title, subtitle, nItems, item, subfunction, NULL, NULL, LHX (310), -1, 1);
}

//------------------------------------------------------------------------------

int ExecMenutiny2 (char * title, char * subtitle, int nItems, tMenuItem * item, 
						  void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem))
{
        return ExecMenu4 (title, subtitle, nItems, item, subfunction, 0, NULL, -1, -1, 1);
}

//------------------------------------------------------------------------------

int ExecMenu1 (char * title, char * subtitle, int nItems, tMenuItem * item, 
					 void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem), 
					 int *cItemP)
{
	return ExecMenu3 (title, subtitle, nItems, item, subfunction, cItemP, NULL, -1, -1);
}

//------------------------------------------------------------------------------

int ExecMenu2 (char * title, char * subtitle, int nItems, tMenuItem * item, 
					 void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem), 
					 int *cItemP, char * filename)
{
	return ExecMenu3 (title, subtitle, nItems, item, subfunction, cItemP, filename, -1, -1);
}

//------------------------------------------------------------------------------

int ExecMenu3 (char * title, char * subtitle, int nItems, tMenuItem * item, 
					 void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem), 
					 int *cItemP, char * filename, int width, int height)
 {
  return ExecMenu4 (title, subtitle, nItems, item, subfunction, cItemP, filename, width, height, 0);
 }

//------------------------------------------------------------------------------

int ExecMenuFixedFont (char * title, char * subtitle, int nItems, tMenuItem * item, 
							  void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem), 
							  int *cItemP, char * filename, int width, int height){
	SetScreenMode (SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
//	return ExecMenu3_real (title, subtitle, nItems, item, subfunction, cItem, filename, width, height, GAME_FONT, GAME_FONT, GAME_FONT, GAME_FONT);
	return ExecMenu4 (title, subtitle, nItems, item, subfunction, cItemP, filename, width, height, 0);
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
		for (i=0; i<4; i++)	
	 		if (JoyGetButtonDownCnt (i)>0) return 1;
		break;
	case	CONTROL_MOUSE:
	case	CONTROL_CYBERMAN:
	case	CONTROL_WINJOYSTICK:
		break;
	case	CONTROL_NONE:		//keyboard only
		#ifdef APPLE_DEMO
			if (key_checkch ())	return 1;			
		#endif

		break;
	default:
		Error ("Bad control nType (gameConfig.nControlType):%i", gameConfig.nControlType);
	}

	return 0;
}

//------------------------------------------------------------------------------

extern int NetworkRequestPlayerNames (int);
extern int RestoringMenu;

ubyte Hack_DblClick_MenuMode=0;

# define JOYDEFS_CALIBRATING 0

#define CLOSE_X     (gameStates.menus.bHires?15:7)
#define CLOSE_Y     (gameStates.menus.bHires?15:7)
#define CLOSE_SIZE  (gameStates.menus.bHires?10:5)

void NMDrawCloseBox (int x, int y)
{
	WIN (DDGRLOCK (dd_grd_curcanv));
	GrSetColorRGB (0, 0, 0, 255);
	GrRect (x + CLOSE_X, y + CLOSE_Y, x + CLOSE_X + CLOSE_SIZE, y + CLOSE_Y + CLOSE_SIZE);
	GrSetColorRGBi (RGBA_PAL2 (21, 21, 21));
	GrRect (x + CLOSE_X + LHX (1), y + CLOSE_Y + LHX (1), x + CLOSE_X + CLOSE_SIZE - LHX (1), y + CLOSE_Y + CLOSE_SIZE - LHX (1));
	WIN (DDGRUNLOCK (dd_grd_curcanv));
}

//------------------------------------------------------------------------------

int NMDrawTitle (char *title, grsFont *font, unsigned int color, int ty)
{
if (title && *title)	{
		int string_width, nStringHeight, average_width, tw, th;

	grdCurCanv->cvFont = font;
	GrSetFontColorRGBi (color, 1, 0, 0);
	GrGetStringSize (title, &string_width, &nStringHeight, &average_width);
	tw = string_width;
	th = nStringHeight;
	WIN (DDGRLOCK (dd_grd_curcanv));
	GrPrintF (NULL, 0x8000, ty, title);
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	ty += nStringHeight;
	}
return ty;
}

//------------------------------------------------------------------------------

void NMGetTitleSize (char *title, grsFont *font, int *tw, int *th)
{
if (title && *title)	{
		int string_width, nStringHeight, average_width;

	grdCurCanv->cvFont = font;
	GrGetStringSize (title, &string_width, &nStringHeight, &average_width);
	if (string_width > *tw)
		*tw = string_width;
	*th += nStringHeight;
	}
}

//------------------------------------------------------------------------------

int NMGetMenuSize (tMenuItem *item, int nItems, int *w, int *h, int *aw, int *nMenus, int *nOthers)
{
	int	string_width, nStringHeight, average_width;
	int	i, j;

#if 0
if ((gameOpts->menus.nHotKeys > 0) && !gameStates.app.bEnglish)
	gameOpts->menus.nHotKeys = -1;
#endif
for (i = 0; i < nItems; i++) {
	if (!gameStates.app.bEnglish)
		item [i].key = *(item [i].text - 1);
	if (item [i].key) {
		if (gameOpts->menus.nHotKeys < 0) {
			if ((item [i].key < KEY_1) || (item [i].key > KEY_0))
				item [i].key = -1;
			}
		else if (gameOpts->menus.nHotKeys == 0)
			item [i].key = 0;
		}
	item [i].redraw = 1;
	item [i].y = *h;
	GrGetStringSize (item [i].text, &string_width, &nStringHeight, &average_width);
	item [i].right_offset = 0;
	
	if (gameStates.multi.bSurfingNet)
		nStringHeight+=LHY (3);

	item [i].saved_text [0] = '\0';
	if (item [i].nType == NM_TYPE_SLIDER) {
		int w1, h1, aw1;
		(*nOthers)++;
		sprintf (item [i].saved_text, "%s", SLIDER_LEFT);
		for (j=0; j< (item [i].maxValue-item [i].minValue+1); j++)	{
			sprintf (item [i].saved_text, "%s%s", item [i].saved_text, SLIDER_MIDDLE);
			}
		sprintf (item [i].saved_text, "%s%s", item [i].saved_text, SLIDER_RIGHT);
		GrGetStringSize (item [i].saved_text, &w1, &h1, &aw1);
		string_width += w1 + *aw;
		}
	else if (item [i].nType == NM_TYPE_MENU)	{
		(*nMenus)++;
		}
	else if (item [i].nType == NM_TYPE_CHECK)	{
		int w1, h1, aw1;
		(*nOthers)++;
		GrGetStringSize (NORMAL_CHECK_BOX, &w1, &h1, &aw1);
		item [i].right_offset = w1;
		GrGetStringSize (CHECKED_CHECK_BOX, &w1, &h1, &aw1);
		if (w1 > item [i].right_offset)
			item [i].right_offset = w1;
		}
	else if (item [i].nType == NM_TYPE_RADIO) {
		int w1, h1, aw1;
		(*nOthers)++;
		GrGetStringSize (NORMAL_RADIO_BOX, &w1, &h1, &aw1);
		item [i].right_offset = w1;
		GrGetStringSize (CHECKED_RADIO_BOX, &w1, &h1, &aw1);
		if (w1 > item [i].right_offset)
			item [i].right_offset = w1;
		}
	else if  (item [i].nType==NM_TYPE_NUMBER)	{
		int w1, h1, aw1;
		char test_text [20];
		(*nOthers)++;
		sprintf (test_text, "%d", item [i].maxValue);
		GrGetStringSize (test_text, &w1, &h1, &aw1);
		item [i].right_offset = w1;
		sprintf (test_text, "%d", item [i].minValue);
		GrGetStringSize (test_text, &w1, &h1, &aw1);
		if (w1 > item [i].right_offset)
			item [i].right_offset = w1;
		}
	else if (item [i].nType == NM_TYPE_INPUT)	{
		Assert (strlen (item [i].text) < NM_MAX_TEXT_LEN);
		strncpy (item [i].saved_text, item [i].text, NM_MAX_TEXT_LEN);
		(*nOthers)++;
		string_width = item [i].text_len*grdCurCanv->cvFont->ftWidth+ ((gameStates.menus.bHires?3:1)*item [i].text_len);
		if (string_width > MAX_TEXT_WIDTH) 
			string_width = MAX_TEXT_WIDTH;
		item [i].value = -1;
		}
	else if (item [i].nType == NM_TYPE_INPUT_MENU)	{
		Assert (strlen (item [i].text) < NM_MAX_TEXT_LEN);
		strncpy (item [i].saved_text, item [i].text, NM_MAX_TEXT_LEN);
		(*nMenus)++;
		string_width = item [i].text_len*grdCurCanv->cvFont->ftWidth+ ((gameStates.menus.bHires?3:1)*item [i].text_len);
		item [i].value = -1;
		item [i].group = 0;
		}
	item [i].w = string_width;
	item [i].h = nStringHeight;

	if (string_width > *w)
		*w = string_width;		// Save maximum width
	if (average_width > *aw)
		*aw = average_width;
	*h += nStringHeight + 1;		// Find the height of all strings
	}
return nStringHeight;
}

//------------------------------------------------------------------------------

void NMSetItemPos (tMenuItem *item, int nItems, int twidth, int xOffs, int yOffs, int right_offset)
{
	int	i, j;

for (i = 0; i < nItems; i++)	{
	if ((item [i].x == (short) 0x8000) || item [i].centered) {
		item [i].centered = 1;
		item [i].x = GetCenteredX (item [i].text);
		}
	else
		item [i].x = xOffs + twidth + right_offset;
	item [i].y += yOffs;
	item [i].xSave = item [i].x;
	item [i].ySave = item [i].y;
	if (item [i].nType == NM_TYPE_RADIO)	{
		int fm = -1;	// ffs first marked one
		for (j = 0; j < nItems; j++)	{
			if ((item [j].nType == NM_TYPE_RADIO) && (item [j].group == item [i].group)) {
				if ((fm == -1) && item [j].value)
					fm = j;
				item [j].value = 0;
				}
			}
		if ( fm >= 0)	
			item [fm].value = 1;
		else
			item [i].value = 1;
		}
	}
}

//------------------------------------------------------------------------------

int NMInitCtrl (nm_control *ctrlP, char *title, char *subtitle, int nItems, tMenuItem *item)
{
if ((ctrlP->scWidth != grdCurScreen->scWidth) || (ctrlP->scHeight != grdCurScreen->scHeight)) {
		nm_control	ctrl = *ctrlP;
		int			i, gap, haveTitle;

	ctrl.scWidth = grdCurScreen->scWidth;
	ctrl.scHeight = grdCurScreen->scHeight;
	ctrl.nDisplayMode = gameStates.video.nDisplayMode;
	NMGetTitleSize (title, TITLE_FONT, &ctrl.tw, &ctrl.th);
	NMGetTitleSize (subtitle, SUBTITLE_FONT, &ctrl.tw, &ctrl.th);

	haveTitle = ((title && *title) || (subtitle && *subtitle));
	gap = haveTitle ? (int) LHY (8) : 0;
	ctrl.th += gap;		//put some space between titles & body
	grdCurCanv->cvFont = ctrl.bTinyMode ? SMALL_FONT : NORMAL_FONT;

	ctrl.h = ctrl.th;
	ctrl.nMenus = ctrl.nOthers = 0;
	ctrl.nStringHeight = NMGetMenuSize (item, nItems, &ctrl.w, &ctrl.h, &ctrl.aw, &ctrl.nMenus, &ctrl.nOthers);
	ctrl.nMaxOnMenu = (((gameOpts->menus.nStyle && (ctrl.scHeight > 480)) ? ctrl.scHeight * 4 / 5 : 480) - ctrl.th - LHY (8)) / ctrl.nStringHeight - 2;
	if (/*!ctrl.bTinyMode &&*/ (ctrl.h > (ctrl.nMaxOnMenu * (ctrl.nStringHeight+1)) + gap)) {
	  ctrl.bIsScrollBox = 1;
	  ctrl.h = (ctrl.nMaxOnMenu * (ctrl.nStringHeight + haveTitle) + haveTitle * LHY (ctrl.bTinyMode ? 12 : 8));
	 }
	else
		ctrl.bIsScrollBox = 0;
	ctrl.nMaxDisplayable = (ctrl.nMaxOnMenu < nItems) ? ctrl.nMaxOnMenu : nItems;
	ctrl.right_offset = 0;
	if (ctrl.width > -1)
		ctrl.w = ctrl.width;
	if (ctrl.height > -1)
		ctrl.h = ctrl.height;

	for (i = 0; i < nItems; i++) {
		item [i].w = ctrl.w;
		if (item [i].right_offset > ctrl.right_offset)
			ctrl.right_offset = item [i].right_offset;
		if (item [i].noscroll && (i == ctrl.nMaxNoScroll))
			ctrl.nMaxNoScroll++;
		}
	ctrl.nScrollOffset = ctrl.nMaxNoScroll;
	if (ctrl.right_offset > 0)
		ctrl.right_offset += 3;

	ctrl.w += ctrl.right_offset;
	ctrl.twidth = 0;
	if (ctrl.tw > ctrl.w)	{
		ctrl.twidth = (ctrl.tw - ctrl.w) / 2;
		ctrl.w = ctrl.tw;
	}

	if (RestoringMenu) { 
		ctrl.right_offset = 0; 
		ctrl.twidth = 0;
		}

	if (ctrl.w > ctrl.scWidth)
		ctrl.w = ctrl.scWidth;
	if (ctrl.h > ctrl.scHeight)
		ctrl.h = ctrl.scHeight;

	ctrl.xOffs = (gameStates.menus.bHires ? 30 : 15);
	i = (ctrl.scWidth - ctrl.w) / 2;
	if (i < ctrl.xOffs)
		ctrl.xOffs = i / 2;
	ctrl.yOffs = (gameStates.menus.bHires ? 30 : 15);
	if (ctrl.scHeight - ctrl.h < 2 * ctrl.yOffs)
		ctrl.h = ctrl.scHeight - 2 * ctrl.yOffs;
	ctrl.w += 2 * ctrl.xOffs;
	ctrl.h += 2 * ctrl.yOffs;
	ctrl.x = (ctrl.scWidth - ctrl.w) / 2;
	ctrl.y = (ctrl.scHeight - ctrl.h) / 2;
	if (gameOpts->menus.nStyle) {
		ctrl.xOffs += ctrl.x;
		ctrl.yOffs += ctrl.y;
		}
	ctrl.bValid = 1;
	*ctrlP = ctrl;
	NMSetItemPos (item, nItems, ctrl.twidth, ctrl.xOffs, ctrl.yOffs, ctrl.right_offset);
	return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void NMSaveScreen (gsrCanvas **save_canvas, gsrCanvas **game_canvas, grsFont **saveFont)
{
WINDOS (*game_canvas = dd_grd_curcanv, *game_canvas = grdCurCanv);
WINDOS (*save_canvas = dd_grd_curcanv, *save_canvas = grdCurCanv);
*saveFont = grdCurCanv->cvFont;
WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));
}

//------------------------------------------------------------------------------

void NMRestoreScreen (char *filename, bkg *bg, gsrCanvas *save_canvas, grsFont *saveFont, int bDontRestore)
{
WINDOS (DDGrSetCurrentCanvas (bg->menu_canvas), GrSetCurrentCanvas (bg->menu_canvas));
if (gameOpts->menus.nStyle)
	NMRemoveBackground (bg);
else {
	if (!filename) {
		// Save the background under the menu...
		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBitmap (0, 0, bg->saved); 	
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		GrFreeBitmap (bg->saved);
		D2_FREE (bg->background);
		} 
	else {
		if (!bDontRestore) {	//info passed back from subfunction
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBitmap (0, 0, bg->background);
			WIN (DDGRUNLOCK (dd_grd_curcanv)); 	
			}
		GrFreeBitmap (bg->background);
		}
	GrUpdate (0);
	}
WINDOS (DDGrFreeSubCanvas (bg->menu_canvas), 
		  GrFreeSubCanvas (bg->menu_canvas));
WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));			
grdCurCanv->cvFont	= saveFont;
WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (save_canvas));
memset (bg, 0, sizeof (*bg));
GrabMouse (1, 0);
}

//------------------------------------------------------------------------------

void ShowMenuHelp (char *szHelp)
{
if (szHelp && *szHelp) {
	int nInMenu = gameStates.menus.nInMenu;
	int bFastMenus = gameOpts->menus.bFastMenus;
	gameStates.menus.nInMenu = 0;
	//gameOpts->menus.bFastMenus = 0;
	gameData.menu.helpColor = RGBA_PAL (47, 47, 47);
	gameData.menu.colorOverride = gameData.menu.helpColor;
	ExecMessageBox ("D2X-XL online help", NULL, -3, szHelp, " ", TXT_CLOSE);
	gameData.menu.colorOverride = 0;
	gameOpts->menus.bFastMenus = bFastMenus;
	gameStates.menus.nInMenu = nInMenu;
	}
}

//------------------------------------------------------------------------------

extern void OglDoFullScreenInternal (int bForce);

#define REDRAW_ALL	for (i = 0; i < nItems; i++) item [i].redraw = 1; bRedrawAll = 1

int ExecMenu4 (char *title, char *subtitle, int nItems, tMenuItem *item, 
					 void (*subfunction) (int nItems, tMenuItem *items, int *last_key, int cItem), 
					 int *cItemP, char *filename, int width, int height, int bTinyMode)
{
	int			old_keyd_repeat, done, cItem = cItemP ? *cItemP : 0;
	int			choice, old_choice, i;
	nm_control	ctrl;
	int			k, nLastScrollCheck=-1, sx, sy;
	grsFont		*saveFont;
	bkg			bg;
	int			bAllText=0;		//set true if all text items
	int			sound_stopped=0, time_stopped=0;
   int			topChoice;   // Is this a scrolling box? Set to 0 at init
   char			*Temp, TempVal;
	int			bDontRestore = 0;
	int			bRedraw = 0, bRedrawAll = 0, bStart = 1;
	WINDOS (ddgrs_canvas *save_canvas, gsrCanvas *save_canvas);	
	WINDOS (ddgrs_canvas *game_canvas, gsrCanvas *game_canvas);	
	int			bWheelUp, bWheelDown, nMouseState, nOldMouseState, bDblClick=0;
	int			mx=0, my=0, x1 = 0, x2, y1, y2;
	int			bLaunchOption = 0;
	int			bCloseBox = 0;

if (!grdCurScreen)
	return -1;
if (gameStates.menus.nInMenu)
	return -1;
memset (&bg, 0, sizeof (bg));
memset (&ctrl, 0, sizeof (ctrl));
ctrl.width = width;
ctrl.height = height;
ctrl.bTinyMode = bTinyMode;
FlushInput ();
PA_DFX (pa_set_frontbuffer_current ());
PA_DFX (pa_set_front_to_read ());

WIN (if (!_AppActive) {
	return -1}
	);		// Don't draw message if minimized!
SDL_ShowCursor (0);
if (nItems < 1)
	return -1;
SDL_EnableKeyRepeat(60, 30);
gameStates.menus.nInMenu++;
WIN (mouse_set_mode (0));		//disable centering mode
if (!gameOpts->menus.nStyle && (gameStates.app.nFunctionMode == FMODE_GAME) && !(gameData.app.nGameMode & GM_MULTI)) {
	DigiPauseDigiSounds ();
	sound_stopped = 1;
	}

if (!(gameOpts->menus.nStyle || ((gameData.app.nGameMode & GM_MULTI) && (gameStates.app.nFunctionMode == FMODE_GAME) &&(!gameStates.app.bEndLevelSequence)))) {
	time_stopped = 1;
	StopTime ();
	#ifdef TACTILE 
	  if (TactileStick)	
		  DisableForces ();	
	#endif
	}

if (gameStates.app.bGameRunning && (gameData.app.nGameMode && GM_MULTI))
	gameData.multigame.nTypingTimeout = 0;

SetPopupScreenMode ();
NMSaveScreen (&save_canvas, &game_canvas, &saveFont);
old_keyd_repeat = keyd_repeat;
keyd_repeat = 1;
if (cItem == -1)
	choice = -1;
else {
	if (cItem < 0) 
		cItem = 0;
	else 
		cItem %= nItems;
	choice = cItem;
	bDblClick = 1;
	while (item [choice].nType == NM_TYPE_TEXT) {
		choice++;
		if (choice >= nItems)
			choice = 0; 
		if (choice == cItem) {
			choice = 0; 
			bAllText = 1;
			break; 
			}
		}
	} 

done = 0;
topChoice = 0;
while (item [topChoice].nType == NM_TYPE_TEXT) {
	topChoice++;
	if (topChoice >= nItems)
		topChoice = 0; 
	if (topChoice == cItem) {
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
	if (cItemP)
		*cItemP = choice;
	if (gameStates.app.bGameRunning && (gameData.app.nGameMode && GM_MULTI)) {
		gameStates.multi.bPlayerIsTyping [gameData.multiplayer.nLocalPlayer] = 1;
		MultiSendTyping ();
		}
	if (!JOYDEFS_CALIBRATING)
		SDL_ShowCursor (1);      // possibly hidden
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
	SongsCheckRedbookRepeat ();
	//NetworkListen ();
	if (bWheelUp)
		k = KEY_UP;
	else if (bWheelDown)
		k = KEY_DOWN;
	else
		k = KeyInKey ();
	if ((ctrl.scWidth != grdCurScreen->scWidth) || (ctrl.scHeight != grdCurScreen->scHeight)) {
		memset (&ctrl, 0, sizeof (ctrl));
		ctrl.width = width;
		ctrl.height = height;
		ctrl.bTinyMode = bTinyMode;
		}
#if 1
	if (ctrl.bValid && (ctrl.nDisplayMode != gameStates.video.nDisplayMode)) {
		NMFreeAllTextBms (item, nItems);
		NMRestoreScreen (filename, &bg, save_canvas, saveFont, bDontRestore);
		SetScreenMode (SCREEN_MENU);
		NMSaveScreen (&save_canvas, &game_canvas, &saveFont);
//		RemapFontsAndMenus (1);
		memset (&ctrl, 0, sizeof (ctrl));
		ctrl.width = width;
		ctrl.height = height;
		ctrl.bTinyMode = bTinyMode;
		}
#endif
	if (NMInitCtrl (&ctrl, title, subtitle, nItems, item)) {
		bRedraw = 0;
		ctrl.ty = ctrl.yOffs;
		if (choice > ctrl.nScrollOffset + ctrl.nMaxOnMenu - 1)
			ctrl.nScrollOffset = choice - ctrl.nMaxOnMenu + 1;
		}
	if (!gameOpts->menus.nStyle) {
		if (subfunction)
			(*subfunction) (nItems, item, &k, choice);
		}
	else {
		if (gameStates.app.bGameRunning) {
			WINDOS (ddgrs_canvas *save_canvas, gsrCanvas *save_canvas);	
			WINDOS (save_canvas = dd_grd_curcanv, save_canvas = grdCurCanv);
			WINDOS (DDGrSetCurrentCanvas (game_canvas), GrSetCurrentCanvas (game_canvas));
			//GrPaletteStepLoad (gamePalette);
			//GrCopyPalette (grPalette, gamePalette, sizeof (grPalette));
			if (gameData.app.bGamePaused /*|| timer_paused*/) {
				GameRenderFrame ();
				gameStates.render.nFrameFlipFlop = !gameStates.render.nFrameFlipFlop;
				Sleep (0);
				}
			else {
				GameLoop (1, 0);
				}
			WINDOS (DDGrSetCurrentCanvas (save_canvas), GrSetCurrentCanvas (save_canvas));
			GrPaletteStepLoad (menuPalette);
			//RemapFontsAndMenus (1);
			}
		}

	if (!bRedraw || gameOpts->menus.nStyle) {
		int t;
		NMInitBackground (filename, &bg, ctrl.x, ctrl.y, ctrl.w, ctrl.h, bRedraw);
		if (!gameStates.app.bGameRunning)
			con_update();
		if (subfunction)
     		(*subfunction) (nItems, item, &k, choice);
		t = NMDrawTitle (title, TITLE_FONT, RGBA_PAL (31, 31, 31), ctrl.yOffs);
		NMDrawTitle (subtitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
		if (!bRedraw)
			ctrl.ty = t;
		grdCurCanv->cvFont = bTinyMode ? SMALL_FONT : NORMAL_FONT;
		}
	if (!time_stopped){
		// Save current menu box
		if (MultiMenuPoll () == -1)
			k = -2;
		}

	if (k < -1) {
		bDontRestore = (k == -3);		//-3 means don't restore
		choice = k;
		k = -1;
		done = 1;
		}
old_choice = choice;
if (k && (con_events (k) || bWheelUp || bWheelDown))
	switch (k)	{

		case KEY_SHIFTED + KEY_ESC:
			con_show();
			k = -1;
			break;

		case KEY_I:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 ShowNetGameInfo (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo)
			{
			 done = 1;
			 choice = -1;
			}
		 break;
		 
		case KEY_U:
		 if (gameStates.multi.bSurfingNet && !bAlreadyShowingInfo)
			 NetworkRequestPlayerNames (choice - 2 - gameStates.multi.bUseTracker);
		 if (gameStates.multi.bSurfingNet && bAlreadyShowingInfo) {
			 done=1;
			 choice=-1;
			}
		 break;

		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;

		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;

		case KEY_COMMAND + KEY_T:
		case KEY_CTRLED + KEY_T:
			gameData.menu.alpha = (gameData.menu.alpha + 16) & 0xFF;
			break;

		case KEY_COMMAND + KEY_P:
		case KEY_CTRLED + KEY_P:
		case KEY_PAUSE:
		 if (Pauseable_menu) {	
			 Pauseable_menu=0;
			 done=1;
		 	 choice=-1;
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
      		if (ctrl.bIsScrollBox) {
           		nLastScrollCheck=-1;
              	if (choice < topChoice) { 
						choice = nItems - 1; 
						ctrl.nScrollOffset = nItems - ctrl.nMaxDisplayable + ctrl.nMaxNoScroll;
						if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
							ctrl.nScrollOffset = ctrl.nMaxNoScroll;
						REDRAW_ALL;
						break; 
						}
             	else if (choice < ctrl.nScrollOffset) {
						REDRAW_ALL;
                  ctrl.nScrollOffset--;
						if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
							ctrl.nScrollOffset = ctrl.nMaxNoScroll;
					   }
	           	}
           	else {
           		if (choice >= nItems) 
						choice=0;
            	else if (choice < 0) 
						choice = nItems - 1;
           		}
				} while (item [choice].nType == NM_TYPE_TEXT);
			if ((item [choice].nType==NM_TYPE_INPUT) &&(choice!=old_choice))	
				item [choice].value = -1;
			if ((old_choice>-1) &&(item [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice!=choice))	{
				item [old_choice].group=0;
				strcpy (item [old_choice].text, item [old_choice].saved_text);
				item [old_choice].value = -1;
			}
			if (old_choice>-1) 
				item [old_choice].redraw = 1;
			item [choice].redraw=1;
			break;

		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
      	// ((0, "Pressing down! ctrl.bIsScrollBox=%d", ctrl.bIsScrollBox);
     		if (bAllText) 
				break;
			do {
				choice++;
        		if (ctrl.bIsScrollBox) {
            	nLastScrollCheck = -1;
					if (choice == nItems) {
						choice = 0;
						if (ctrl.nScrollOffset) {
							REDRAW_ALL;
			            ctrl.nScrollOffset = ctrl.nMaxNoScroll;
							}
						}
              	if (choice >= ctrl.nMaxOnMenu + ctrl.nScrollOffset - ctrl.nMaxNoScroll) {
						REDRAW_ALL;
                  ctrl.nScrollOffset++;
						}
           		}
            else {
         		if (choice < 0) 
						choice=nItems-1;
           		else if (choice >= nItems) 
						choice=0;
           		}
				} while (item [choice].nType==NM_TYPE_TEXT);
                                                      
			if ((item [choice].nType == NM_TYPE_INPUT) && (choice != old_choice))	
				item [choice].value = -1;
			if ((old_choice>-1) &&(item [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice!=choice))	{
				item [old_choice].group=0;
				strcpy (item [old_choice].text, item [old_choice].saved_text);	
				item [old_choice].value = -1;
			}
			if (old_choice>-1)
				item [old_choice].redraw=1;
			item [choice].redraw=1;
			break;
		case KEY_SPACEBAR:
checkOption:
radioOption:
			if (choice > -1)	{
				switch (item [choice].nType)	{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if (item [choice].value)
						item [choice].value = 0;
					else
						item [choice].value = 1;
					if (ctrl.bIsScrollBox) {
						if (choice== (ctrl.nMaxOnMenu+ctrl.nScrollOffset-1) || choice==ctrl.nScrollOffset)
							nLastScrollCheck=-1;					
						 }
				
					item [choice].redraw=1;
					if (bLaunchOption)
						goto launchOption;
					break;
				case NM_TYPE_RADIO:
					for (i=0; i<nItems; i++)	{
						if ((i!=choice) &&(item [i].nType==NM_TYPE_RADIO) &&(item [i].group==item [choice].group) &&(item [i].value))	{
							item [i].value = 0;
							item [i].redraw = 1;
						}
					}
					item [choice].value = 1;
					item [choice].redraw = 1;
					if (bLaunchOption)
						goto launchOption;
					break;
				}	
			}
			break;

      case KEY_SHIFTED+KEY_UP:
         if (gameStates.menus.bReordering && choice!=topChoice)
         {
            Temp=item [choice].text;
            TempVal=item [choice].value;
            item [choice].text=item [choice-1].text;
            item [choice].value=item [choice-1].value;
            item [choice-1].text=Temp;
            item [choice-1].value=TempVal;
            item [choice].rebuild=1;
            item [choice-1].rebuild=1;
            choice--;
         }
         break;
      case KEY_SHIFTED+KEY_DOWN:
         if (gameStates.menus.bReordering && choice!= (nItems-1))
         {
            Temp=item [choice].text;
            TempVal=item [choice].value;
            item [choice].text=item [choice+1].text;
            item [choice].value=item [choice+1].value;
            item [choice+1].text=Temp;
            item [choice+1].value=TempVal;
            item [choice].rebuild=1;
            item [choice+1].rebuild=1;
            choice++;
         }
         break;
                
		case KEY_ALTED + KEY_ENTER:
			{
			//int bLoadAltBg = NMFreeAltBg ();
			NMFreeAllTextBms (item, nItems);
			NMRestoreScreen (filename, &bg, save_canvas, saveFont, bDontRestore);
			GrToggleFullScreenGame ();
			GrabMouse (0, 0);
			SetScreenMode (SCREEN_MENU);
			NMSaveScreen (&save_canvas, &game_canvas, &saveFont);
			RemapFontsAndMenus (1);
			memset (&ctrl, 0, sizeof (ctrl));
			ctrl.width = width;
			ctrl.height = height;
			ctrl.bTinyMode = bTinyMode;
			continue;
			}

		case KEY_ENTER:
		case KEY_PADENTER:
launchOption:
			if ((choice>-1) &&(item [choice].nType==NM_TYPE_INPUT_MENU) &&(item [choice].group==0))	{
				item [choice].group = 1;
				item [choice].redraw = 1;
				if (!strnicmp (item [choice].saved_text, TXT_EMPTY, strlen (TXT_EMPTY)))	{
					item [choice].text [0] = 0;
					item [choice].value = -1;
					}
				else {	
					NMTrimWhitespace (item [choice].text);
					}
				}
			else {
				if (cItemP)
					*cItemP = choice;
				done = 1;
				}
			break;

		case KEY_ESC:
			if ((choice>-1) &&(item [choice].nType==NM_TYPE_INPUT_MENU) &&(item [choice].group==1))	{
				item [choice].group=0;
				strcpy (item [choice].text, item [choice].saved_text);	
				item [choice].redraw=1;
				item [choice].value = -1;
				}
			else {
				done = 1;
				if (cItemP)
					*cItemP = choice;
				choice = -1;
			}
			break;

		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_ALTED + KEY_F9:
			bSaveScreenShot = 1;
			SaveScreenShot (NULL, 0);
			PA_DFX (pa_set_frontbuffer_current ());
			PA_DFX (pa_set_front_to_read ());
			for (i=0;i<nItems;i++)
				item [i].redraw=1;
			
			break;

		#ifdef _DEBUG
		case KEY_BACKSP:	
			if ((choice>-1) &&(item [choice].nType!=NM_TYPE_INPUT)&&(item [choice].nType!=NM_TYPE_INPUT_MENU))
				Int3 (); 
			break;
		#endif

		case KEY_F1:
			ShowMenuHelp (item [choice].szHelp);
			break;
		}

		WIN (Sleep (100));
		if (!done && nMouseState && !nOldMouseState && !bAllText) {
			mouse_get_pos (&mx, &my);
			for (i=0; i<nItems; i++)	{
				x1 = grdCurCanv->cvBitmap.bmProps.x + item [i].x - item [i].right_offset - 6;
				x2 = x1 + item [i].w;
				y1 = grdCurCanv->cvBitmap.bmProps.y + item [i].y;
				y2 = y1 + item [i].h;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					if (i + ctrl.nScrollOffset - ctrl.nMaxNoScroll != choice) {
						if (Hack_DblClick_MenuMode) bDblClick = 0; 
					}
					
					choice = i + ctrl.nScrollOffset - ctrl.nMaxNoScroll;
					if (choice >= 0 && choice < nItems)
						switch (item [choice].nType)	{
							case NM_TYPE_CHECK:
								item [choice].value = !item [choice].value;
								item [choice].redraw=1;
								if (ctrl.bIsScrollBox)
									nLastScrollCheck=-1;
								break;
							case NM_TYPE_RADIO:
								for (i=0; i<nItems; i++)	{
									if ((i!=choice) &&(item [i].nType==NM_TYPE_RADIO) &&(item [i].group==item [choice].group) &&(item [i].value))	{
										item [i].value = 0;
										item [i].redraw = 1;
									}
								}
								item [choice].value = 1;
								item [choice].redraw = 1;
								break;
							}
					item [old_choice].redraw=1;
					break;
				}
			}
		}

		if (nMouseState && bAllText) {
			if (cItemP)
				*cItemP = choice;
			done = 1;
			}
		
		if (!done && nMouseState && !bAllText) {
			mouse_get_pos (&mx, &my);
			
			// check possible scrollbar stuff first
			if (ctrl.bIsScrollBox) {
				int i, arrow_width, arrow_height, aw;
				
				if (ctrl.nScrollOffset > ctrl.nMaxNoScroll) {
					GrGetStringSize (UP_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grdCurCanv->cvBitmap.bmProps.x + item [ctrl.nScrollOffset].x- (gameStates.menus.bHires?24:12);
		         y1 = grdCurCanv->cvBitmap.bmProps.y + item [ctrl.nScrollOffset].y- ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
					x1 = x2 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
						choice--;
		           	nLastScrollCheck=-1;
		            if (choice < ctrl.nScrollOffset) {
							REDRAW_ALL;
		               ctrl.nScrollOffset--;
			            }
						}
					}
				if ((i = ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll) < nItems) {
					GrGetStringSize (DOWN_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grdCurCanv->cvBitmap.bmProps.x + item [i-1].x - (gameStates.menus.bHires?24:12);
					y1 = grdCurCanv->cvBitmap.bmProps.y + item [i-1].y - ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
					x1 = x2 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
						choice++;
		            nLastScrollCheck=-1;
		            if (choice >= ctrl.nMaxOnMenu + ctrl.nScrollOffset) {
							REDRAW_ALL;
		               ctrl.nScrollOffset++;
							}
						}
					}
				}
			
			for (i = ctrl.nScrollOffset; i < nItems; i++)	{
				x1 = grdCurCanv->cvBitmap.bmProps.x + item [i].x - item [i].right_offset - 6;
				x2 = x1 + item [i].w;
				y1 = grdCurCanv->cvBitmap.bmProps.y + item [i].y;
				y1 -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
				y2 = y1 + item [i].h;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2)) &&(item [i].nType != NM_TYPE_TEXT)) {
					if (i /*+ ctrl.nScrollOffset - ctrl.nMaxNoScroll*/ != choice) {
						if (Hack_DblClick_MenuMode) 
							bDblClick = 0; 
					}

					choice = i/* + ctrl.nScrollOffset - ctrl.nMaxNoScroll*/;

					if (item [choice].nType == NM_TYPE_SLIDER) {
						char slider_text [NM_MAX_TEXT_LEN+1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
						
						strcpy (slider_text, item [choice].saved_text);
						p = strchr (slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p+1;
						}
						if (p) {
							GrGetStringSize (s1, &slider_width, &height, &aw);
							GrGetStringSize (SLIDER_LEFT, &sleft_width, &height, &aw);
							GrGetStringSize (SLIDER_RIGHT, &sright_width, &height, &aw);
							GrGetStringSize (SLIDER_MIDDLE, &smiddle_width, &height, &aw);

							x1 = grdCurCanv->cvBitmap.bmProps.x + item [choice].x + item [choice].w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ((mx > x1) &&(mx < (x1 + sleft_width)) &&(item [choice].value != item [choice].minValue)) {
								item [choice].value = item [choice].minValue;
								item [choice].redraw = 2;
							} else if ((mx < x2) &&(mx > (x2 - sright_width)) &&(item [choice].value != item [choice].maxValue)) {
								item [choice].value = item [choice].maxValue;
								item [choice].redraw = 2;
							} else if ((mx > (x1 + sleft_width)) &&(mx < (x2 - sright_width))) {
								int numValues, value_width, newValue;
								
								numValues = item [choice].maxValue - item [choice].minValue + 1;
								value_width = (slider_width - sleft_width - sright_width) / numValues;
								newValue = (mx - x1 - sleft_width) / value_width;
								if (item [choice].value != newValue) {
									item [choice].value = newValue;
									item [choice].redraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == old_choice)
						break;
					if ((item [choice].nType==NM_TYPE_INPUT) &&(choice!=old_choice))	
						item [choice].value = -1;
					if ((old_choice>-1) &&(item [old_choice].nType==NM_TYPE_INPUT_MENU) &&(old_choice!=choice))	{
						item [old_choice].group=0;
						strcpy (item [old_choice].text, item [old_choice].saved_text);
						item [old_choice].value = -1;
					}
					if (old_choice>-1) 
						item [old_choice].redraw = 1;
					item [choice].redraw=1;
					break;
				}
			}
		}
		
		if (!done && !nMouseState && nOldMouseState && !bAllText &&(choice != -1) &&(item [choice].nType == NM_TYPE_MENU)) {
			mouse_get_pos (&mx, &my);
			x1 = grdCurCanv->cvBitmap.bmProps.x + item [choice].x;
			x2 = x1 + item [choice].w;
			y1 = grdCurCanv->cvBitmap.bmProps.y + item [choice].y;
			if (choice >= ctrl.nScrollOffset)
				y1 -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
			y2 = y1 + item [choice].h;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (Hack_DblClick_MenuMode) {
					if (bDblClick) {
						if (cItemP)
							*cItemP = choice;
						done = 1;
						}
					else 
						bDblClick = 1;
				}
				else {
					done = 1;
					if (cItemP)
						*cItemP = choice;
				}
			}
		}
		
		if (!done && !nMouseState && nOldMouseState &&(choice>-1) &&(item [choice].nType==NM_TYPE_INPUT_MENU) &&(item [choice].group==0))	{
			item [choice].group = 1;
			item [choice].redraw = 1;
			if (!strnicmp (item [choice].saved_text, TXT_EMPTY, strlen (TXT_EMPTY)))	{
				item [choice].text [0] = 0;
				item [choice].value = -1;
			} else {	
				NMTrimWhitespace (item [choice].text);
			}
		}
		
		if (!done && !nMouseState && nOldMouseState && bCloseBox) {
			mouse_get_pos (&mx, &my);
			x1 = (gameOpts->menus.nStyle ? ctrl.x : grdCurCanv->cvBitmap.bmProps.x) + CLOSE_X;
			x2 = x1 + CLOSE_SIZE;
			y1 = (gameOpts->menus.nStyle ? ctrl.y : grdCurCanv->cvBitmap.bmProps.y) + CLOSE_Y;
			y2 = y1 + CLOSE_SIZE;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (cItemP)
					*cItemP = choice;
				choice = -1;
				done = 1;
				}
			}

//	 HACK! Don't redraw loadgame preview
		if (RestoringMenu) 
			item [0].redraw = 0;

		if (choice > -1)	{
			int ascii;

			if (((item [choice].nType==NM_TYPE_INPUT)|| ((item [choice].nType==NM_TYPE_INPUT_MENU)&&(item [choice].group==1)))&&(old_choice==choice))	{
				if (k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4)	{
					if (item [choice].value==-1) 
						item [choice].value = (int) strlen (item [choice].text);
					if (item [choice].value > 0)
						item [choice].value--;
					item [choice].text [item [choice].value] = 0;
					item [choice].redraw = 1;	
					}
				else {
					ascii = KeyToASCII (k);
					if ((ascii < 255) && (item [choice].value < item [choice].text_len)) {
						int bAllowed;

						if (item [choice].value==-1)
							item [choice].value = 0;
						bAllowed = NMCharAllowed ((char) ascii);
						if (!bAllowed && ascii==' ' && NMCharAllowed ('_')) {
							ascii = '_';
							bAllowed=1;
							}
						if (bAllowed) {
							item [choice].text [item [choice].value++] = ascii;
							item [choice].text [item [choice].value] = 0;
							item [choice].redraw=1;	
							}
						}
					}
				}
			else if ((item [choice].nType!=NM_TYPE_INPUT) && (item [choice].nType!=NM_TYPE_INPUT_MENU)) {
				ascii = KeyToASCII (k);
				if (ascii < 255) {
					int choice1 = choice;
					ascii = toupper (ascii);
					do {
						int i, ch = 0, t;

						if (++choice1 >= nItems) 
							choice1=0;
						t = item [choice1].nType;
						if (gameStates.app.bNostalgia)
							ch = item [choice1].text [0];
						else if (item [choice1].key > 0)
							ch = MENU_KEY (item [choice1].key, 0);
						else if (item [choice1].key < 0) //skip any leading blanks
							for (i=0; (ch=item [choice1].text [i]) && ch==' ';i++)
								;
						else
							continue;
								;
						if (((t == NM_TYPE_MENU) ||
							  (t == NM_TYPE_CHECK) ||
							  (t == NM_TYPE_RADIO) ||
							  (t == NM_TYPE_NUMBER) ||
							  (t == NM_TYPE_SLIDER))
								&& (ascii == toupper (ch)))	{
							k = 0;
							choice = choice1;
							if (old_choice>-1)
								item [old_choice].redraw = 1;
							item [choice].redraw = 1;
							if (choice < ctrl.nScrollOffset) {
								ctrl.nScrollOffset = choice;
								REDRAW_ALL;
								}
							else if (choice > ctrl.nScrollOffset + ctrl.nMaxDisplayable - 1) {
								ctrl.nScrollOffset = choice;
								if (ctrl.nScrollOffset + ctrl.nMaxDisplayable >= nItems) {
									ctrl.nScrollOffset = nItems - ctrl.nMaxDisplayable;
									if (ctrl.nScrollOffset < ctrl.nMaxNoScroll)
										ctrl.nScrollOffset = ctrl.nMaxNoScroll;
									}
								REDRAW_ALL;
								}
							if (t == NM_TYPE_CHECK)
								goto checkOption;
							else if (t == NM_TYPE_RADIO)
								goto radioOption;
							else if ((t != NM_TYPE_SLIDER) && (item [choice1].key != 0))
								if (gameOpts->menus.nHotKeys > 0)
									goto launchOption;
						}
					} while (choice1 != choice);
				}	
			}

			if ((item [choice].nType==NM_TYPE_NUMBER) || (item [choice].nType==NM_TYPE_SLIDER)) 	{
				int ov=item [choice].value;
				switch (k) {
				case KEY_PAD4:
			  	case KEY_LEFT:
			  	case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					item [choice].value -= 1;
					break;
			  	case KEY_RIGHT:
				case KEY_PAD6:
			  	case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					item [choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					item [choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					item [choice].value -= 10;
					break;
				}
				if (ov!=item [choice].value)
					item [choice].redraw=1;
			}
	
		}


    	// Redraw everything...
#if 1
		bRedraw = 0;
		if (bRedrawAll && !gameOpts->menus.nStyle) {
			int t;
			NMInitBackground (filename, &bg, ctrl.x, ctrl.y, ctrl.w, ctrl.h, bRedraw);
			t = NMDrawTitle (title, TITLE_FONT, RGBA_PAL (31, 31, 31), ctrl.yOffs);
			NMDrawTitle (subtitle, SUBTITLE_FONT, RGBA_PAL (21, 21, 21), t);
			bRedrawAll = 0;
			}
		WINDOS (	DDGrSetCurrentCanvas (bg.menu_canvas), 
				GrSetCurrentCanvas (bg.menu_canvas));
		grdCurCanv->cvFont = ctrl.bTinyMode ? SMALL_FONT : NORMAL_FONT;
     	for (i = 0; i < ctrl.nMaxDisplayable + ctrl.nScrollOffset - ctrl.nMaxNoScroll; i++) {
			if ((i >= ctrl.nMaxNoScroll) && (i < ctrl.nScrollOffset))
				continue;
			if (!(gameOpts->menus.nStyle || (item [i].text && *item [i].text)))
				continue;
			if (bStart || (curDrawBuffer == GL_BACK) || item [i].redraw || item [i].rebuild) {// warning! ugly hack below                  
				bRedraw = 1;
				if (item [i].rebuild && item [i].centered)
					item [i].x = GetCenteredX (item [i].text);
				if (i >= ctrl.nScrollOffset)
         		item [i].y -= ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
				if (!gameOpts->menus.nStyle) 
					SDL_ShowCursor (0);
           	NMDrawItem (&bg, item + i, (i == choice) && !bAllText, bTinyMode);
				item [i].redraw = 0;
				if (!gameStates.menus.bReordering && !JOYDEFS_CALIBRATING)
					SDL_ShowCursor (1);
				if (i >= ctrl.nScrollOffset)
	            item [i].y += ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
	        	}   
         if ((i == choice) && 
				 ((item [i].nType == NM_TYPE_INPUT) || 
				 ((item [i].nType == NM_TYPE_INPUT_MENU) && item [i].group)))
				NMUpdateCursor (&item [i]);
			}
#endif
      if (ctrl.bIsScrollBox) {
      	//grdCurCanv->cvFont = NORMAL_FONT;
        	if (bRedraw || (nLastScrollCheck != ctrl.nScrollOffset)) {
          	nLastScrollCheck=ctrl.nScrollOffset;
          	grdCurCanv->cvFont = SELECTED_FONT;
          	sy=item [ctrl.nScrollOffset].y - ((ctrl.nStringHeight+1)*(ctrl.nScrollOffset - ctrl.nMaxNoScroll));
          	sx=item [ctrl.nScrollOffset].x - (gameStates.menus.bHires?24:12);
          	if (ctrl.nScrollOffset > ctrl.nMaxNoScroll)
           		NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, UP_ARROW_MARKER);
          	else
           		NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, "  ");
				i = ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll - 1;
          	sy=item [i].y - ((ctrl.nStringHeight + 1) * (ctrl.nScrollOffset - ctrl.nMaxNoScroll));
          	sx=item [i].x - (gameStates.menus.bHires ? 24 : 12);
          	if (ctrl.nScrollOffset + ctrl.nMaxDisplayable - ctrl.nMaxNoScroll<nItems)
           		NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, DOWN_ARROW_MARKER);
          	else
	           	NMRStringWXY (&bg, (gameStates.menus.bHires ? 20 : 10), sx, sy, "  ");
		    	}
     		}   
		if (bCloseBox && (bStart || gameOpts->menus.nStyle)) {
			if (gameOpts->menus.nStyle)
				NMDrawCloseBox (ctrl.x, ctrl.y);
			else
				NMDrawCloseBox (ctrl.x - grdCurCanv->cvBitmap.bmProps.x, ctrl.y - grdCurCanv->cvBitmap.bmProps.y);
			bCloseBox = 1;
			}
		if (bRedraw || !gameOpts->menus.nStyle)
			GrUpdate (0);
		bRedraw = 1;
		bStart = 0;
		if (!bDontRestore && gameStates.render.bPaletteFadedOut) {
			GrPaletteFadeIn (NULL, 32, 0);
		}
	}
SDL_ShowCursor (0);
// Restore everything...
NMRestoreScreen (filename, &bg, save_canvas, saveFont, bDontRestore);
NMFreeAllTextBms (item, nItems);
keyd_repeat = old_keyd_repeat;
GameFlushInputs ();
if (time_stopped) {
	StartTime ();
#ifdef TACTILE
		if (TactileStick)
			EnableForces ();
#endif
  }
if (sound_stopped)
	DigiResumeDigiSounds ();
WIN (mouse_set_mode (1));				//re-enable centering mode
gameStates.menus.nInMenu--;
GrPaletteStepUp (0, 0, 0);
SDL_EnableKeyRepeat(0, 0);
if (gameStates.app.bGameRunning && (gameData.app.nGameMode && GM_MULTI))
	MultiSendMsgQuit();
return choice;
}

//------------------------------------------------------------------------------

int _CDECL_ ExecMessageBox1 (
					char *title, 
					void (*subfunction) (int nItems, tMenuItem * items, int * last_key, int cItem), 
					char *filename, int nChoices, ...)
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text [MESSAGEBOX_TEXT_SIZE];
	tMenuItem nmMsgItems [5];
	va_start (args, nChoices);
	Assert (nChoices <= 5);
	memset (nmMsgItems, 0, sizeof (nmMsgItems));
	for (i=0; i<nChoices; i++)	{
      s = va_arg (args, char *);
      nmMsgItems [i].nType = NM_TYPE_MENU; 
		nmMsgItems [i].text = s;
		nmMsgItems [i].key = -1;
	}
	format = va_arg (args, char *);
	strcpy (nm_text, "");
	vsprintf (nm_text, format, args);
	va_end (args);

	Assert (strlen (nm_text) < MESSAGEBOX_TEXT_SIZE);

	return ExecMenu (title, nm_text, nChoices, nmMsgItems, subfunction, filename);
}

//------------------------------------------------------------------------------

int _CDECL_ ExecMessageBox (char *title, char *filename, int nChoices, ...)
{
	int				h, i, l, bTiny;
	char				*format, *s;
	va_list			args;
	char				nm_text [MESSAGEBOX_TEXT_SIZE];
	tMenuItem		nmMsgItems [10];


if (!nChoices)
	return -1;
if ((bTiny = (nChoices < 0)))
	nChoices = -nChoices;
va_start (args, nChoices);
memset (nmMsgItems, 0, h = nChoices * sizeof (tMenuItem));
for (i = l = 0; i < nChoices; i++) {
	s = va_arg (args, char *);
	h = (int) strlen (s);
	if (l + h > MESSAGEBOX_TEXT_SIZE)
		break;
	l += h;
	if (!bTiny || i) 
		nmMsgItems [i].nType = NM_TYPE_MENU; 
	else {
		nmMsgItems [i].nType = NM_TYPE_TEXT; 
		nmMsgItems [i].unavailable = 1; 
		}
	nmMsgItems [i].text = s;
	nmMsgItems [i].key = (bTiny && !i) ? 0 : -1;
	if (bTiny)
		nmMsgItems [i].centered = (i != 0);
	}
if (!bTiny) {
	format = va_arg (args, char *);
	vsprintf (nm_text, format, args);
	va_end (args);
	}
i = bTiny ? 
	 ExecMenutiny (NULL, title, nChoices, nmMsgItems, NULL) :
	 ExecMenu (title, nm_text, nChoices, nmMsgItems, NULL, filename);
return i;
}

//------------------------------------------------------------------------------

void NMFileSort (int n, char *list)
{
	int i, j, incr;
	char t [FILENAME_LEN];

	incr = n / 2;
	while (incr > 0)		{
		for (i=incr; i<n; i++)		{
			j = i - incr;
			while (j>=0)			{
				if (strncmp (&list [j* (FILENAME_LEN + 1)], &list [ (j+incr)* (FILENAME_LEN + 1)], FILENAME_LEN - 1) > 0) {
               memcpy (t, &list [j* (FILENAME_LEN + 1)], FILENAME_LEN);
               memcpy (&list [j* (FILENAME_LEN + 1)], &list [ (j+incr)* (FILENAME_LEN + 1)], FILENAME_LEN);
               memcpy (&list [ (j+incr)* (FILENAME_LEN + 1)], t, FILENAME_LEN);
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}

//------------------------------------------------------------------------------

void DeletePlayerSavedGames (char * name)
{
	int i;
	char filename [16];
	
for (i = 0; i < 10; i++) {
	sprintf (filename, "%s.sg%d", name, i);
	CFDelete (filename, gameFolders.szSaveDir);
	}
}

//------------------------------------------------------------------------------

#define MAX_FILES 300

int MakeNewPlayerFile (int allow_abort);

int ExecMenuFileSelector (char * title, char * filespec, char * filename, int allow_abortFlag)
{
	int i;
	FFS ffs;
	int NumFiles=0, key, done, cItem, ocitem;
	char *filenames = NULL;
	int NumFiles_displayed = 8;
	int first_item = -1, ofirst_item;
	int old_keyd_repeat = keyd_repeat;
	int player_mode=0;
	int demo_mode=0;
	int demos_deleted=0;
	int initialized = 0;
	int exitValue = 0;
	int w_x, w_y, w_w, w_h, title_height;
	int box_x, box_y, box_w, box_h;
	bkg bg;		// background under listbox
	int mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;
	int mouse2State, omouse2State, bWheelUp, bWheelDown;
	int bDblClick=0;
	char szPattern [40];
	int nPatternLen = 0;
	char *pszFn;
WIN (int win_redraw=0);

	w_x = w_y = w_w = w_h = title_height = 0;
	box_x = box_y = box_w = box_h = 0;

	if (! (filenames = D2_ALLOC (MAX_FILES * (FILENAME_LEN + 1))))
		return 0;

	memset (&bg, 0, sizeof (bg));
	bg.bIgnoreBg = 1;
	cItem = 0;
	keyd_repeat = 1;

	WIN (mouse_set_mode (0));				//disable centering mode

	if (strstr (filespec, "*.plr"))
		player_mode = 1;
	else if (strstr (filespec, "*.dem"))
		demo_mode = 1;

ReadFileNames:
	done = 0;
	NumFiles=0;
	
#if !defined (APPLE_DEMO)		// no new pilots for special apple oem version
	if (player_mode)	{
		strncpy (filenames + NumFiles * (FILENAME_LEN + 1), TXT_CREATE_NEW, FILENAME_LEN);
		NumFiles++;
	}
#endif

	if (!FFF (filespec, &ffs, 0))	{
		pszFn = filenames + NumFiles * (FILENAME_LEN + 1);
		do	{
			if (NumFiles < MAX_FILES)	{
            strncpy (pszFn, ffs.name, FILENAME_LEN);
				if (player_mode)	{
					char * p = strchr (pszFn, '.');
					if (p) 
						*p = '\0';
				}
				if (*pszFn) {
					strlwr (pszFn);
					NumFiles++;
					}
			} else {
				break;
			}
		pszFn += (FILENAME_LEN + 1);
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
	}
	if (demo_mode && gameFolders.bAltHogDirInited) {
		char filespec2 [PATH_MAX + FILENAME_LEN];
		sprintf (filespec2, "%s/%s", gameFolders.szAltHogDir, filespec);
		if (!FFF (filespec2, &ffs, 0)) {
			do {
				if (NumFiles<MAX_FILES)	{
					strncpy (filenames+NumFiles* (FILENAME_LEN + 1), ffs.name, FILENAME_LEN);
					NumFiles++;
				} else {
					break;
				}
			} while (!FFN (&ffs, 0));
			FFC (&ffs);
		}
	}

	if ((NumFiles < 1) && demos_deleted)	{
		exitValue = 0;
		goto ExitFileMenu;
	}
	if ((NumFiles < 1) && demo_mode) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		exitValue = 0;
		goto ExitFileMenu;
	}

#if 0//ndef APPLE_DEMO
	if ((NumFiles < 2) && player_mode) {
		cItem = 0;
		goto ExitFileMenuEarly;
	}
#endif


	if (NumFiles<1)	{
		#ifndef APPLE_DEMO
			ExecMessageBox (NULL, NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
		#endif
		exitValue = 0;
		goto ExitFileMenu;
	}

	if (!initialized) {	
//		SetScreenMode (SCREEN_MENU);
		SetPopupScreenMode ();
		GrSetCurrentCanvas (NULL);
		WIN (DDGRLOCK (dd_grd_curcanv))					//mwa put these here -- are these needed Samir???
		{
			grdCurCanv->cvFont = SUBTITLE_FONT;
		}
		WIN (DDGRUNLOCK (dd_grd_curcanv));

		w_w = 0;
		w_h = 0;

		for (i=0; i<NumFiles; i++) {
			int w, h, aw;
			GrGetStringSize (filenames+i* (FILENAME_LEN+1), &w, &h, &aw);		
			if (w > w_w)
				w_w = w;
		}
		if (title) {
			int w, h, aw;
			GrGetStringSize (title, &w, &h, &aw);		
			if (w > w_w)
				w_w = w;
			title_height = h + (grd_curfont->ftHeight*2);		// add a little space at the bottom of the title
		}

		box_w = w_w;
		box_h = ((grd_curfont->ftHeight + 2) * NumFiles_displayed);

		w_w += (grd_curfont->ftWidth * 4);
		w_h = title_height + box_h + (grd_curfont->ftHeight * 2);		// more space at bottom

		if (w_w > grdCurCanv->cv_w) 
			w_w = grdCurCanv->cv_w;
		if (w_h > grdCurCanv->cv_h) 
			w_h = grdCurCanv->cv_h;
		if (w_w > 640)
			w_w = 640;
		if (w_h > 480)
			w_h = 480;
	
		w_x = (grdCurCanv->cv_w-w_w)/2;
		w_y = (grdCurCanv->cv_h-w_h)/2;
	
		if (w_x < 0) 
			w_x = 0;
		if (w_y < 0) 
			w_y = 0;

		box_x = w_x + (grd_curfont->ftWidth*2);			// must be in sync with w_w!!!
		box_y = w_y + title_height;

// save the screen behind the menu.

		bg.saved = NULL;
		if (!gameOpts->menus.nStyle) {
			if ((gameStates.render.vr.buffers.offscreen->cv_w >= w_w) &&(gameStates.render.vr.buffers.offscreen->cv_h >= w_h)) 
				bg.background = &gameStates.render.vr.buffers.offscreen->cvBitmap;
			else
				bg.background = GrCreateBitmap (w_w, w_h, 1);
			Assert (bg.background != NULL);
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBmBitBlt (w_w, w_h, 0, 0, w_x, w_y, &grdCurCanv->cvBitmap, bg.background);
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			}
		NMDrawBackground (&bg, w_x, w_y, w_x+w_w-1, w_y+w_h-1, 0);
		WIN (DDGRLOCK (dd_grd_curcanv))
		{	
			GrString (0x8000, w_y+10, title, NULL);
		}
		WIN (DDGRUNLOCK (dd_grd_curcanv));

		WIN (DDGRRESTORE);
		initialized = 1;
	}

	if (!player_mode)	{
		NMFileSort (NumFiles, filenames);
	} else {
		NMFileSort (NumFiles-1, filenames+ (FILENAME_LEN+1));		// Don't sort first one!
		for (i=0; i<NumFiles; i++)	{
			if (!stricmp (LOCALPLAYER.callsign, filenames+i* (FILENAME_LEN+1)))	{
				bDblClick = 1;
				cItem = i;
			}
	 	}
	}

	nMouseState = nOldMouseState = 0;
	mouse2State = omouse2State = 0;
	NMDrawCloseBox (w_x, w_y);
	SDL_ShowCursor (1);

	SDL_EnableKeyRepeat(60, 30);
	while (!done)	{
		ocitem = cItem;
		ofirst_item = first_item;
		GrUpdate (0);
		nOldMouseState = nMouseState;
		omouse2State = mouse2State;
		nMouseState = MouseButtonState (0);
		mouse2State = MouseButtonState (1);
		bWheelUp = MouseButtonState (3);
		bWheelDown = MouseButtonState (4);
		//see if redbook song needs to be restarted
		SongsCheckRedbookRepeat ();
		if (bWheelUp)
			key = KEY_UP;
		else if (bWheelDown)
			key = KEY_DOWN;
		else
			key = KeyInKey ();
		switch (key) {
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_ALTED + KEY_F9:
			bSaveScreenShot = 1;
			SaveScreenShot (NULL, 0);
			PA_DFX (pa_set_frontbuffer_current ());
			PA_DFX (pa_set_front_to_read ());
			
			break;

		case KEY_CTRLED+KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_CTRLED+KEY_D:

			if (((player_mode) && (cItem > 0)) || ((demo_mode) && (cItem >= 0))) {
				int x = 1;
				char *pszFile = filenames + cItem * (FILENAME_LEN+1);
				if (*pszFile == '$')
					pszFile++;
				SDL_ShowCursor (0);
				if (player_mode)
					x = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, pszFile);
				else if (demo_mode)
					x = ExecMessageBox (NULL, NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, pszFile);
				SDL_ShowCursor (1);
 				if (!x)	{
					char *p;
					int ret;

					p = pszFile + strlen (pszFile);
					if (player_mode)
						*p = '.';
#if 0
					_splitpath (filespec, NULL, name, dir, NULL);
					strcat (name, dir);
					//strcat (name, &filenames [cItem* (FILENAME_LEN+1)]);
#endif					
					ret = CFDelete (pszFile, player_mode ? gameFolders.szProfDir : gameFolders.szDemoDir);
					if (player_mode)
						*p = 0;

					if ((!ret) && player_mode)	{
						DeletePlayerSavedGames (pszFile);
					}

					if (ret) {
						if (player_mode)
							ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, pszFile);
						else if (demo_mode)
							ExecMessageBox (NULL, NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, pszFile);
						}
					else if (demo_mode)
						demos_deleted = 1;
					first_item = -1;
					goto ReadFileNames;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			cItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			cItem = NumFiles-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			cItem--;			
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			cItem++;			
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			cItem += NumFiles_displayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			cItem -= NumFiles_displayed;
			break;
		case KEY_ESC:
			if (allow_abortFlag) {
				cItem = -1;
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
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < sizeof (szPattern) - 1)) {
				int nStart, ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						cc = cItem + 1;
						nStart = cItem;
						if (cc < 0)  
							cc = 0;
						else if (cc >= NumFiles)  
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
						pszFn = filenames + cc * (FILENAME_LEN + 1);
						if (gameOpts->menus.bSmartFileSearch ? strstr (pszFn, szPattern) == pszFn : *pszFn == toupper (ascii)) {
							cItem = cc;
							bFound = 1;
							break;
						}
						cc++;
						cc %= NumFiles;
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound)
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
		if (done) break;


		if (cItem<0)
			cItem=0;

		if (cItem>=NumFiles)
			cItem = NumFiles-1;

		if (cItem< first_item)
			first_item = cItem;

		if (cItem>= (first_item+NumFiles_displayed))
		{
			first_item = cItem-NumFiles_displayed+1;
		}

		if (NumFiles <= NumFiles_displayed)
			 first_item = 0;

		if (first_item>NumFiles-NumFiles_displayed)
		{
			first_item = NumFiles-NumFiles_displayed;
		}

		if (first_item < 0) first_item = 0;

		WIN (Sleep (100));
		if (nMouseState || mouse2State) {
			int w, h, aw;

			mouse_get_pos (&mx, &my);
			for (i=first_item; i<first_item+NumFiles_displayed; i++)	{
				GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);
				x1 = box_x;
				x2 = box_x + box_w - 1;
				y1 = (i-first_item)* (grd_curfont->ftHeight + 2) + box_y;
				y2 = y1+h+1;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					if (i == cItem && !mouse2State) {
						break;
					}
					cItem = i;
					bDblClick = 0;
					break;
				}
			}
		}
		
		if (!nMouseState && nOldMouseState) {
			int w, h, aw;

			GrGetStringSize (&filenames [cItem* (FILENAME_LEN+1)], &w, &h, &aw);
			mouse_get_pos (&mx, &my);
			x1 = box_x;
			x2 = box_x + box_w - 1;
			y1 = (cItem-first_item)* (grd_curfont->ftHeight + 2) + box_y;
			y2 = y1+h+1;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				if (bDblClick) 
					done = 1;
				else 
					bDblClick = 1;
			}
		}

		if (!nMouseState && nOldMouseState) {
			mouse_get_pos (&mx, &my);
			x1 = w_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = w_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				cItem = -1;
				done = 1;
			}
		}
  
	WIN (DDGRLOCK (dd_grd_curcanv));
		if ((ofirst_item != first_item) || gameOpts->menus.nStyle) {
			if (!gameOpts->menus.nStyle) 
				SDL_ShowCursor (0);
			NMDrawBackground (&bg, w_x, w_y, w_x+w_w-1, w_y+w_h-1,1);
			WIN (DDGRLOCK (dd_grd_curcanv))
			{	
				grdCurCanv->cvFont = NORMAL_FONT;
				GrString (0x8000, w_y+10, title, NULL);
			}
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			GrSetColorRGB (0, 0, 0, 255);
			for (i=first_item; i<first_item+NumFiles_displayed; i++)	{
				int w, h, aw, y;
				y = (i-first_item)* (grd_curfont->ftHeight + 2) + box_y;
			
				if (i >= NumFiles)	{

					GrSetColorRGBi (RGBA_PAL2 (5, 5, 5));
					GrRect (box_x + box_w, y-1, box_x + box_w, y + grd_curfont->ftHeight + 1);
					//GrRect (box_x, y + grd_curfont->ftHeight + 2, box_x + box_w, y + grd_curfont->ftHeight + 2);
					
					GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
					GrRect (box_x - 1, y - 1, box_x - 1, y + grd_curfont->ftHeight + 2);
					
					GrSetColorRGB (0, 0, 0, 255);
					GrRect (box_x, y - 1, box_x + box_w - 1, y + grd_curfont->ftHeight + 1);
					
				} else {
					if (i == cItem)	
						grdCurCanv->cvFont = SELECTED_FONT;
					else	
						grdCurCanv->cvFont = NORMAL_FONT;
					GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);

					GrSetColorRGBi (RGBA_PAL2 (5, 5, 5));
				  //	GrRect (box_x, y + h + 2, box_x + box_w, y + h + 2);
					GrRect (box_x + box_w, y - 1, box_x + box_w, y + h + 1);
					
					GrSetColorRGBi (RGBA_PAL2 (2, 2, 2));
					GrRect (box_x - 1, y - 1, box_x - 1, y + h + 1);
					GrSetColorRGB (0, 0, 0, 255);
							
					GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
					GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((player_mode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0), NULL);
				}
			}	 
			SDL_ShowCursor (1);
		} else if (cItem != ocitem)	{
			int w, h, aw, y;

			if (!gameOpts->menus.nStyle) 
				SDL_ShowCursor (0);
			i = ocitem;
			if ((i>=0) &&(i<NumFiles))	{
				y = (i-first_item)* (grd_curfont->ftHeight+2)+box_y;
				if (i == cItem)	
					grdCurCanv->cvFont = SELECTED_FONT;
				else	
					grdCurCanv->cvFont = NORMAL_FONT;
				GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);
				GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
				GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((player_mode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0), NULL);
			}
			i = cItem;
			if ((i>=0) &&(i<NumFiles))	{
				y = (i-first_item)* (grd_curfont->ftHeight+2)+box_y;
				if (i == cItem)	
					grdCurCanv->cvFont = SELECTED_FONT;
				else	
					grdCurCanv->cvFont = NORMAL_FONT;
				GrGetStringSize (&filenames [i* (FILENAME_LEN+1)], &w, &h, &aw);
				GrRect (box_x, y-1, box_x + box_w - 1, y + h + 1);
				GrString (box_x + 5, y, (&filenames [i* (FILENAME_LEN+1)])+ ((player_mode && filenames [i* (FILENAME_LEN+1)]=='$')?1:0), NULL);
			}
			GrUpdate (0);
			SDL_ShowCursor (1);
		}
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	}

//ExitFileMenuEarly:
	if (cItem > -1)	{
		strncpy (filename, (&filenames [cItem* (FILENAME_LEN+1)])+ ((player_mode && filenames [cItem* (FILENAME_LEN+1)]=='$')?1:0), FILENAME_LEN);
		exitValue = 1;
	} else {
		exitValue = 0;
	}											 

ExitFileMenu:
	keyd_repeat = old_keyd_repeat;

	if (initialized) {
		if (gameOpts->menus.nStyle) {
			NMRemoveBackground (&bg);
			}
		else {
			if (gameData.demo.nState != ND_STATE_PLAYBACK)	//horrible hack to prevent restore when screen has been cleared
			{
			WIN (DDGRLOCK (dd_grd_curcanv));
			GrBmBitBlt (w_w, w_h, w_x, w_y, 0, 0, bg.background, &grdCurCanv->cvBitmap);
			WIN (DDGRUNLOCK (dd_grd_curcanv)); 	
			}
			if (bg.background != &gameStates.render.vr.buffers.offscreen->cvBitmap)
				GrFreeBitmap (bg.background);
#if 0
		WINDOS (
			dd_gr_blt_notrans (dd_VR_offscreen_buffer, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_grd_curcanv, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
			GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(gameStates.render.vr.buffers.offscreen->cvBitmap), &(grdCurCanv->cvBitmap))
		);
#endif
		GrUpdate (0);
		WIN (DDGRRESTORE);
		}
	}

	if (filenames)
		D2_FREE (filenames);

	WIN (mouse_set_mode (1));				//re-enable centering mode
	WIN (SDL_ShowCursor (0));

	SDL_EnableKeyRepeat(0, 0);
	return exitValue;

}

//------------------------------------------------------------------------------
// Example listbox callback function...
// int lb_callback (int * cItem, int *nItems, char * items [], int *keypress)
// {
// 	int i;
// 
// 	if (*keypress = KEY_CTRLED+KEY_D)	{
// 		if (*nItems > 1)	{
// 			CFDelete (items [*cItem]);    // Delete the file
// 			for (i=*cItem; i<*nItems-1; i++)	{
// 				items [i] = items [i+1];
// 			}
// 			*nItems = *nItems - 1;
// 			D2_FREE (items [*nItems]);
// 			items [*nItems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

//------------------------------------------------------------------------------

#define LB_ITEMS_ON_SCREEN 8

int ExecMenuListBox (char * title, int nItems, char * items [], int allow_abortFlag, int (*listbox_callback) (int * cItem, int *nItems, char * items [], int *keypress))
{
return ExecMenuListBox1 (title, nItems, items, allow_abortFlag, 0, listbox_callback);
}

//------------------------------------------------------------------------------

int ExecMenuListBox1 (char * title, int nItems, char * items [], int allow_abortFlag, int default_item, int (*listbox_callback) (int * cItem, int *nItems, char * items [], int *keypress))
{
	int i;
	int done, ocitem, cItem, ofirst_item, first_item, key, redraw;
	int old_keyd_repeat = keyd_repeat;
	int width, height, wx, wy, title_height, border_size;
	int total_width, total_height;
	bkg bg;
	int mx, my, x1, x2, y1, y2, nMouseState, nOldMouseState;	//, bDblClick;
	int close_x, close_y, bWheelUp, bWheelDown;
	int nItemsOnScreen;
	char szPattern [40];
	int nPatternLen = 0;
	char *pszFn;
	
WIN (int win_redraw=0);

	keyd_repeat = 1;

   PA_DFX (pa_set_frontbuffer_current ());
	PA_DFX (pa_set_front_to_read ());
	WIN (mouse_set_mode (0));				//disable centering mode

//	SetScreenMode (SCREEN_MENU);
	SetPopupScreenMode ();
	memset (&bg, 0, sizeof (bg));
	bg.bIgnoreBg = 1;
	GrSetCurrentCanvas (NULL);
	grdCurCanv->cvFont = SUBTITLE_FONT;

	width = 0;
	for (i=0; i<nItems; i++)	{
		int w, h, aw;
		GrGetStringSize (items [i], &w, &h, &aw);		
		if (w > width)
			width = w;
	}
	nItemsOnScreen = LB_ITEMS_ON_SCREEN * grdCurCanv->cvBitmap.bmProps.h / 480;
	height = (grd_curfont->ftHeight + 2) * nItemsOnScreen;

	{
		int w, h, aw;
		GrGetStringSize (title, &w, &h, &aw);		
		if (w > width)
			width = w;
		title_height = h + 5;
	}

	border_size = grd_curfont->ftWidth;
   WIN (border_size=grd_curfont->ftWidth*2);
		
	width += (grd_curfont->ftWidth);
	if (width > grdCurCanv->cv_w - (grd_curfont->ftWidth * 3))
		width = grdCurCanv->cv_w - (grd_curfont->ftWidth * 3);

	wx = (grdCurCanv->cvBitmap.bmProps.w-width)/2;
	wy = (grdCurCanv->cvBitmap.bmProps.h- (height+title_height))/2 + title_height;
	if (wy < title_height)
		wy = title_height;

	total_width = width+2*border_size;
	total_height = height+2*border_size+title_height;

	bg.saved = NULL;

	if (!gameOpts->menus.nStyle) {
		if ((gameStates.render.vr.buffers.offscreen->cv_w >= total_width) &&(gameStates.render.vr.buffers.offscreen->cv_h >= total_height))
			bg.background = &gameStates.render.vr.buffers.offscreen->cvBitmap;
		else
			//bg.background = GrCreateBitmap (width, (height + title_height));
			bg.background = GrCreateBitmap (total_width, total_height, 1);
		Assert (bg.background != NULL);
		WIN (DDGRLOCK (dd_grd_curcanv));
			//GrBmBitBlt (wx+width+border_size, wy+height+border_size, 0, 0, wx-border_size, wy-title_height-border_size, &grdCurCanv->cvBitmap, bg.background);
			GrBmBitBlt (total_width, total_height, 0, 0, wx-border_size, wy-title_height-border_size, &grdCurCanv->cvBitmap, bg.background);
		WIN (DDGRUNLOCK (dd_grd_curcanv));
		}

#if 0
	WINDOS (
 		dd_gr_blt_notrans (dd_grd_curcanv, 0, 0, 
				_DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_VR_offscreen_buffer, 0, 0, 
				_DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
		GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(grdCurCanv->cvBitmap), &(gameStates.render.vr.buffers.offscreen->cvBitmap))
	);
#endif

	NMDrawBackground (&bg, wx-border_size, wy-title_height-border_size, wx+width+border_size-1, wy+height+border_size-1,0);
	GrUpdate (0);
	WIN (DDGRLOCK (dd_grd_curcanv));
		GrString (0x8000, wy - title_height, title, NULL);
	WIN (DDGRUNLOCK (dd_grd_curcanv));	

	WIN (DDGRRESTORE);

	done = 0;
	cItem = default_item;
	if (cItem < 0) 
		cItem = 0;
	if (cItem >= nItems) 
		cItem = 0;

	first_item = -1;

	nMouseState = nOldMouseState = 0;	//bDblClick = 0;
	close_x = wx-border_size;
	close_y = wy-title_height-border_size;
	NMDrawCloseBox (close_x, close_y);
	SDL_ShowCursor (1);

	SDL_EnableKeyRepeat(60, 30);
	while (!done)	{
		ocitem = cItem;
		ofirst_item = first_item;
		nOldMouseState = nMouseState;
		nMouseState = MouseButtonState (0);
		bWheelUp = MouseButtonState (3);
		bWheelDown = MouseButtonState (4);
		//see if redbook song needs to be restarted
		SongsCheckRedbookRepeat ();

		if (bWheelUp)
			key = KEY_UP;
		else if (bWheelDown)
			key = KEY_DOWN;
		else
			key = KeyInKey ();

		if (listbox_callback)
			redraw = (*listbox_callback) (&cItem, &nItems, items, &key);
		else
			redraw = 0;

		if (key<-1) {
			cItem = key;
			key = -1;
			done = 1;
		}


		switch (key)	{
		case KEY_CTRLED+KEY_F1:
			SwitchDisplayMode (-1);
			break;
		case KEY_CTRLED+KEY_F2:
			SwitchDisplayMode (1);
			break;
		case KEY_CTRLED+KEY_S:
			if (gameStates.app.bNostalgia)
				gameOpts->menus.bSmartFileSearch = 0;
			else
				gameOpts->menus.bSmartFileSearch = !gameOpts->menus.bSmartFileSearch;
			break;

		case KEY_COMMAND+KEY_SHIFTED+KEY_P:
		case KEY_PRINT_SCREEN: 		
			SaveScreenShot (NULL, 0); 
			PA_DFX (pa_set_frontbuffer_current ());
			PA_DFX (pa_set_front_to_read ());			
			break;
		case KEY_HOME:
		case KEY_PAD7:
			cItem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			cItem = nItems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			cItem--;			
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			cItem++;			
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			cItem += nItemsOnScreen;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			cItem -= nItemsOnScreen;
			break;
		case KEY_ESC:
			if (allow_abortFlag) {
				cItem = -1;
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
			if (!gameOpts->menus.bSmartFileSearch || (nPatternLen < sizeof (szPattern) - 1)) {
				int nStart,
					 ascii = KeyToASCII (key);
				if ((key == KEY_BACKSP) || (ascii < 255)) {
					int cc, bFound = 0;
					if (!gameOpts->menus.bSmartFileSearch) {
						nStart = cItem;
						cc = cItem + 1;
						if (cc < 0)  
							cc = 0;
						else if (cc >= nItems)  
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
							cItem = cc;
							bFound = 1;
							break;
							}
						cc++;
						cc %= nItems;
					} while (cc != nStart);
				if (gameOpts->menus.bSmartFileSearch && !bFound && (nPatternLen > 0))
					szPattern [--nPatternLen] = '\0';
				}
			}
		}
		if (done) break;

		if (cItem<0)
			cItem=nItems-1;
		else if (cItem>=nItems)
			cItem = 0;
		if (cItem< first_item)
			first_item = cItem;
		else if (cItem>= (first_item+nItemsOnScreen))
			first_item = cItem-nItemsOnScreen+1;
		if (nItems <= nItemsOnScreen)
			 first_item = 0;
		if (first_item>nItems-nItemsOnScreen)
			first_item = nItems-nItemsOnScreen;
		if (first_item < 0) 
			first_item = 0;

		WIN (Sleep (100));
		if (nMouseState) {
			int w, h, aw;

			mouse_get_pos (&mx, &my);
			for (i=first_item; i<first_item+nItemsOnScreen; i++)	{
				if (i > nItems)
					break;
				GrGetStringSize (items [i], &w, &h, &aw);
				x1 = wx;
				x2 = wx + width;
				y1 = (i-first_item)* (grd_curfont->ftHeight+2)+wy;
				y2 = y1+h+1;
				if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
					//if (i == cItem) {
					//	break;
					//}
					//bDblClick= 0;
					cItem = i;
					done = 1;
					break;
				}
			}
		}

		//check for close box clicked
		if (!nMouseState && nOldMouseState) {
			mouse_get_pos (&mx, &my);
			x1 = close_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = close_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if (((mx > x1) &&(mx < x2)) &&((my > y1) &&(my < y2))) {
				cItem = -1;
				done = 1;
			}
		}

		if ((ofirst_item != first_item) || redraw || gameOpts->menus.nStyle) {
			if (gameOpts->menus.nStyle) 
				NMDrawBackground (&bg, wx-border_size, wy-title_height-border_size, wx+width+border_size-1, wy+height+border_size-1,1);
			else
				SDL_ShowCursor (0);
			WIN (DDGRLOCK (dd_grd_curcanv));
			if (gameOpts->menus.nStyle) {
				grdCurCanv->cvFont = NORMAL_FONT;
				GrString (0x8000, wy - title_height, title, NULL);
				}

			GrSetColorRGB (0, 0, 0, 255);
			for (i=first_item; i<first_item+nItemsOnScreen; i++)	{
				int w, h, aw, y;
				y = (i-first_item)* (grd_curfont->ftHeight+2)+wy;
				if (i >= nItems)	{
					GrSetColorRGB (0, 0, 0, 255);
					GrRect (wx, y-1, wx+width-1, y+grd_curfont->ftHeight + 1);
				} else {
					if (i == cItem)	
						grdCurCanv->cvFont = SELECTED_FONT;
					else	
						grdCurCanv->cvFont = NORMAL_FONT;
					GrGetStringSize (items [i], &w, &h, &aw);
					GrRect (wx, y-1, wx+width-1, y+h+1);
					GrString (wx+5, y, items [i], NULL);
				}
			}		

				
			// If Win95 port, draw up/down arrows on left tSide of menu
			WIN (DDGRUNLOCK (dd_grd_curcanv));
			SDL_ShowCursor (1);
			GrUpdate (0);
		} else if (cItem != ocitem)	{
			int w, h, aw, y;

			if (!gameOpts->menus.nStyle) 
				SDL_ShowCursor (0);

			WIN (DDGRLOCK (dd_grd_curcanv));

			i = ocitem;
			if ((i>=0) &&(i<nItems))	{
				y = (i-first_item)* (grd_curfont->ftHeight+2)+wy;
				if (i == cItem)	
					grdCurCanv->cvFont = SELECTED_FONT;
				else	
					grdCurCanv->cvFont = NORMAL_FONT;
				GrGetStringSize (items [i], &w, &h, &aw);
				GrRect (wx, y-1, wx+width-1, y+h+1);
				GrString (wx+5, y, items [i], NULL);

			}
			i = cItem;
			if ((i>=0) &&(i<nItems))	{
				y = (i-first_item)* (grd_curfont->ftHeight+2)+wy;
				if (i == cItem)	
					grdCurCanv->cvFont = SELECTED_FONT;
				else	
					grdCurCanv->cvFont = NORMAL_FONT;
				GrGetStringSize (items [i], &w, &h, &aw);
				GrRect (wx, y-1, wx+width-1, y+h);
				GrString (wx+5, y, items [i], NULL);
			}
			WIN (DDGRUNLOCK (dd_grd_curcanv));

			SDL_ShowCursor (1);
			GrUpdate (0);
		}
	}

	keyd_repeat = old_keyd_repeat;

	if (gameOpts->menus.nStyle) {
		NMRemoveBackground (&bg);
#if 1
		if (bg.menu_canvas) {
			WINDOS (DDGrFreeSubCanvas (bg.menu_canvas), 
					  GrFreeSubCanvas (bg.menu_canvas));
			WINDOS (DDGrSetCurrentCanvas (NULL), GrSetCurrentCanvas (NULL));			
			bg.menu_canvas = NULL;
			}
#endif
		}
	else {
		SDL_ShowCursor (0);
		WIN (DDGRLOCK (dd_grd_curcanv));
		GrBmBitBlt (total_width, total_height, wx-border_size, wy-title_height-border_size, 0, 0, bg.background, &grdCurCanv->cvBitmap);
		WIN (DDGRUNLOCK (dd_grd_curcanv)); 	
		if (bg.background != &gameStates.render.vr.buffers.offscreen->cvBitmap)
			GrFreeBitmap (bg.background);
		GrUpdate (0);
		}


#if 0
	WINDOS (
		dd_gr_blt_notrans (dd_VR_offscreen_buffer, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh, 
				dd_grd_curcanv, 
				0, 0, _DDModeList [W95DisplayMode].rw, _DDModeList [W95DisplayMode].rh), 
		GrBmBitBlt (grdCurCanv->cv_w, grdCurCanv->cv_h, 0, 0, 0, 0, &(gameStates.render.vr.buffers.offscreen->cvBitmap), &(grdCurCanv->cvBitmap))
	);
#endif

	WIN (DDGRRESTORE);

	WIN (mouse_set_mode (1));				//re-enable centering mode

	SDL_EnableKeyRepeat(0, 0);
	return cItem;
}

//------------------------------------------------------------------------------

static char FilenameText [MAX_FILES] [ (FILENAME_LEN+1)];

int ExecMenuFileList (char * title, char * filespec, char * filename)
{
	int i, NumFiles;
	char * Filenames [MAX_FILES];
	FFS  ffs;

	NumFiles = 0;
	if (!FFF (filespec, &ffs, 0))	{
		do	{
			if (NumFiles<MAX_FILES)	{
            strncpy (FilenameText [NumFiles], ffs.name, FILENAME_LEN);
				Filenames [NumFiles] = FilenameText [NumFiles];
				NumFiles++;
			} else {
				break;
			}
		} while (!FFN (&ffs, 0));
		FFC (&ffs);
	}

	i = ExecMenuListBox (title, NumFiles, Filenames, 1, NULL);
	if (i > -1)	{
		strcpy (filename, Filenames [i]);
		return 1;
	} 
	return 0;
}

//------------------------------------------------------------------------------
//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int _CDECL_ NMMsgBoxFixedFont (char *title, int nChoices, ...)
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text [MESSAGEBOX_TEXT_SIZE];
	tMenuItem nmMsgItems [5];

	va_start (args, nChoices);

	Assert (nChoices <= 5);

	memset (nmMsgItems, 0, sizeof (nmMsgItems));
	for (i=0; i<nChoices; i++)	{
		s = va_arg (args, char *);
		nmMsgItems [i].nType = NM_TYPE_MENU; nmMsgItems [i].text = s;
	}
	format = va_arg (args, char *);
	//sprintf (	  nm_text, ""); // adb: ?
	vsprintf (nm_text, format, args);
	va_end (args);

	Assert (strlen (nm_text) < MESSAGEBOX_TEXT_SIZE);

   return ExecMenuFixedFont (title, nm_text, nChoices, nmMsgItems, NULL, 0, NULL, -1, -1);
}
//end this section addition - Victor Rachels

//------------------------------------------------------------------------------
/* Spiffy word wrap string formatting function */

void NMWrapText (char *dbuf, char *sbuf, int line_length)
{
	int col;
	char *wordptr;
	char *tbuf;

	tbuf = (char *)D2_ALLOC ((int) strlen (sbuf)+1);
	strcpy (tbuf, sbuf);

	wordptr = strtok (tbuf, " ");
	if (!wordptr) return;
	col = 0;
	dbuf [0] = 0;

	while (wordptr)
	{
		col = col + (int) strlen (wordptr)+1;
		if (col >=line_length) {
			col = 0;
			sprintf (dbuf, "%s\n%s ", dbuf, wordptr);
		}
		else {
			sprintf (dbuf, "%s%s ", dbuf, wordptr);
		}
		wordptr = strtok (NULL, " ");
	}

	D2_FREE (tbuf);
}

//------------------------------------------------------------------------------

void NMProgressBar (char *szCaption, int nCurProgress, int nMaxProgress, 
						  void (*doProgress) (int nItems, tMenuItem *items, int *last_key, int cItem))
{
	tMenuItem	m [3];
	int			i, nInMenu;

memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, nMaxProgress); 
m [0].centered = 1;
m [0].value = nCurProgress;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
gameData.app.bGamePaused = 1;
do {
	i = ExecMenu2 (NULL, szCaption, 1, m, doProgress, 0, NULL);
	} while (i >= 0);
gameData.app.bGamePaused = 0;
gameStates.menus.nInMenu = nInMenu;
}

//------------------------------------------------------------------------------
