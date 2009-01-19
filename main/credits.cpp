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
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "inferno.h"
#include "error.h"
#include "key.h"
#include "gamepal.h"
#include "timer.h"
#include "menu.h"
#include "gamefont.h"
#include "network.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "screens.h"
#include "compbit.h"
#include "menubackground.h"
#include "credits.h"
#include "songs.h"

#define LHX(x)      (gameStates.menus.bHires?2*(x):x)
#define LHY(y)      (gameStates.menus.bHires?(24*(y))/10:y)

#define ROW_SPACING (gameStates.menus.bHires?26:11)

ubyte fadeValues[200] = { 1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,8,9,9,10,10,
11,11,12,12,12,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,19,19,20,20,
20,21,21,22,22,22,23,23,23,24,24,24,24,25,25,25,26,26,26,26,27,27,27,27,
28,28,28,28,28,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,30,30,
30,30,30,30,29,29,29,29,29,29,28,28,28,28,28,27,27,27,27,26,26,26,26,25,
25,25,24,24,24,24,23,23,23,22,22,22,21,21,20,20,20,19,19,19,18,18,17,17,
17,16,16,15,15,15,14,14,13,13,12,12,12,11,11,10,10,9,9,8,8,8,7,7,6,6,5,
5,4,4,3,3,2,2,1 };

ubyte fadeValues_hires[480] = { 1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,
5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9,9,9,10,10,10,10,10,10,11,11,11,11,11,12,12,12,12,12,12,
13,13,13,13,13,14,14,14,14,14,14,15,15,15,15,15,15,16,16,16,16,16,17,17,17,17,17,17,18,18,
18,18,18,18,18,19,19,19,19,19,19,20,20,20,20,20,20,20,21,21,21,21,21,21,22,22,22,22,22,22,
22,22,23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,25,26,26,26,26,
26,26,26,26,26,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,29,29,29,
29,29,29,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
30,30,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,
30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,29,29,29,29,29,29,29,29,29,29,29,29,
29,29,28,28,28,28,28,28,28,28,28,28,28,28,27,27,27,27,27,27,27,27,27,27,26,26,26,26,26,26,
26,26,26,25,25,25,25,25,25,25,25,25,24,24,24,24,24,24,24,24,23,23,23,23,23,23,23,22,22,22,
22,22,22,22,22,21,21,21,21,21,21,20,20,20,20,20,20,20,19,19,19,19,19,19,18,18,18,18,18,18,
18,17,17,17,17,17,17,16,16,16,16,16,15,15,15,15,15,15,14,14,14,14,14,14,13,13,13,13,13,12,
12,12,12,12,12,11,11,11,11,11,10,10,10,10,10,10,9,9,9,9,9,8,8,8,8,8,7,7,7,7,7,6,6,6,6,6,5,5,5,5,
5,5,4,4,4,4,4,3,3,3,3,3,2,2,2,2,2,1,1};

CPalette *creditsPalette = NULL;

extern ubyte *grBitBltFadeTable;

#define ALLOWED_CHAR 'R'

#if !DBG
#	define CREDITS_BACKGROUND_FILENAME (gameStates.menus.bHires ? "\x01starsb.pcx" : "\x01stars.pcx")	//only read from hog file
#else
#	define CREDITS_BACKGROUND_FILENAME (gameStates.menus.bHires ? "starsb.pcx" : "stars.pcx")
#endif

typedef struct box {
	int left, top, width, height;
} box;

#define CREDITS_FILE    \
		  (CFile::Exist("mcredits.tex",gameFolders.szDataDir,0)?"mcredits.tex":\
			CFile::Exist("ocredits.tex",gameFolders.szDataDir,0)?"ocredits.tex":"credits.tex")

#define cr_gr_printf(x,y,s)	//GrPrintF (NULL, (x) == 0x8000 ? (x) : (x), (y), s)

static const char *xlCredits [] = {
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"$D2X-XL",
	"",
	"",
	"*Coding",
	"Dietfrid Mali",
	"",
	"*Mac Coding",
	"simx and others",
	"",
	"*Models",
	"MetalBeast",
	"",
	"*Sounds",
	"Frustikus",
	"Red-5",
	"",
	"*Textures",
	"Aus-RED-5",
	"DizzyRox",
	"MetalBeast",
	"Novacron",
	"Thiefbot",
	"",
	"",
	"*Visit the D2X-XL project at",
	"www.descent2.de"
	};

#define NUM_XL_LINES	sizeofa(xlCredits)

#define FADE_DIST	240

CCreditsManager creditsManager;

//-----------------------------------------------------------------------------

void CCreditsManager::RenderBackdrop (void)
{
if (gameOpts->menus.nStyle)
	m_bmBackdrop.Stretch ();
else {
	m_bmBackdrop.Blit (CCanvas::Current (), m_xOffs, m_yOffs);
	if ((CCanvas::Current ()->Width () > 640) || (CCanvas::Current ()->Height () > 480)) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0,0,32));
		GrUBox (m_xOffs, m_yOffs, m_xOffs + m_bmBackdrop.Width () + 1, m_yOffs + m_bmBackdrop.Height () + 1);
		}
	}
}

//-----------------------------------------------------------------------------

uint CCreditsManager::Read (void)
{
do {
	m_nLines [0] = (m_nLines [0] + 1) % NUM_LINES;
	for (;;) {
		if (m_cf.GetS (m_buffer [m_nLines [0]], 80)) {
			char *p = m_buffer [m_nLines [0]];
			if (m_bBinary)				// is this a binary tbl &m_cf
				for (int i = (int) strlen (m_buffer [m_nLines [0]]); i > 0; i--, p++)
					*p = EncodeRotateLeft ((char) (EncodeRotateLeft (*p) ^ BITMAP_TBL_XOR));
			p = m_buffer [m_nLines [0]];
			if (*p == ';')
				continue;
			if (*p == '%') {
				if (p [1] == ALLOWED_CHAR)
					strcpy (p, p + 2);
				else
					continue;
				}
			if ((p = strchr (m_buffer [m_nLines [0]], '\n')))
				*p = '\0';
			} 
		else if (m_nLines [1] < NUM_XL_LINES) {
			strcpy (m_buffer [m_nLines [0]], xlCredits [m_nLines [1]++]);
			}
		else {
			m_buffer [m_nLines [0]][0] = 0;
			m_bDone++;
			}
		break;
		}
	} while (m_nExtraInc--);
m_nExtraInc = 0;
return m_bDone;
}

//-----------------------------------------------------------------------------

bool CCreditsManager::Open (char* creditsFilename)
{
	char filename [32];

m_bBinary = 0;
sprintf (filename, "%s", CREDITS_FILE);
if (creditsFilename) {
	strcpy (filename, creditsFilename);
	m_bBinary = 1;
	}
if (!m_cf.Open (filename, gameFolders.szDataDir, "rb", 0)) {
	if (creditsFilename)
		return false;		//ok to not find special filename

	char nfile [32];
	char* pszTemp = strchr (filename, '.');
	if (pszTemp)
		*pszTemp = '\0';
	sprintf (nfile, "%s.txb", filename);
	if (!m_cf.Open (nfile, gameFolders.szDataDir, "rb", 0)) {
		Error ("Missing CREDITS.TEX and CREDITS.TXB &m_cf\n");
		return false;
		}
	m_bBinary = 1;
	}
return true;
}

//-----------------------------------------------------------------------------

bool CCreditsManager::HandleInput (void)
{
int k = KeyInKey ();

if ((k == KEY_PRINT_SCREEN) || (k == KEY_ALTED+KEY_F9)) {
	gameStates.app.bSaveScreenshot = 1;
	SaveScreenShot (NULL, 0);
	}
else if (k == KEY_PADPLUS)
	m_xDelay /= 2;
else if (k == KEY_PADMINUS) {
	if (m_xDelay)
		m_xDelay *= 2;
	else
		m_xDelay = 1;
	}
else if ((k == KEY_ESC) || (m_bDone > uint (NUM_LINES))) {
	Destroy ();
	paletteManager.DisableEffect ();
	paletteManager.Load (D2_DEFAULT_PALETTE, NULL);
	CCanvas::Pop ();
	songManager.Play (SONG_TITLE, 1);
#if 0
	if (creditsOffscreenBuf != gameStates.render.vr.buffers.offscreen)
		creditsOffscreenBuf->Destroy ();
#endif
	glDisable (GL_BLEND);
	gameStates.menus.nInMenu = 0;
	return false;
	}
return true;
}

//-----------------------------------------------------------------------------

void CCreditsManager::Render (void)
{
	tRgbaColorf		colors [4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};

for (int i = 0; i < ROW_SPACING; i += gameStates.menus.bHires + 1) {
	RenderBackdrop ();
	int y = m_nFirstLineOffs - i;
	for (int j = 0; j < NUM_LINES; j++) {
		int l = (m_nLines [0] + j + 1) %  NUM_LINES;
		char* s = m_buffer [l];
		if (*s == '!') 
			s++;
		else if (*s == '$') {
			fontManager.SetCurrent (m_fonts [0]);
			s++;
			} 
		else if (*s == '*') {
			fontManager.SetCurrent (m_fonts [1]);
			s++;
			} 
		else
			fontManager.SetCurrent (m_fonts [2]);
		if (*s) {
			int w, h, aw;

			fontManager.Current ()->StringSize (s, w, h, aw);
			if ((y >= 0) && (y + h <= 480)) {
				CBitmap* bmP = CreateStringBitmap (s, 0, 0, NULL, 1, w, -1);
				if (bmP) {
					float dy = float ((y < FADE_DIST) ? y : (480 - y - h < FADE_DIST) ? 480 - y - h : FADE_DIST);
					colors [0].alpha = colors [1].alpha = dy / float (FADE_DIST);
					dy = float ((y + h < FADE_DIST) ? y + h : (480 - y - 2 * h < FADE_DIST) ? 480 - y - 2 * h : FADE_DIST);
					colors [2].alpha = colors [3].alpha = dy / float (FADE_DIST);
					bmP->Render (CCanvas::Current (), (screen.Width () - w) / 2, m_yOffs + y, w, h, 0, 0, w, h, 1, 0, 0, 1, colors);
					delete bmP;
					}
				}
			}
		if (m_buffer[l][0] == '!')
			y += ROW_SPACING / 2;
		else
			y += ROW_SPACING;
		}

		int t = m_xTimeout - SDL_GetTicks ();
		if (t > 0)
			G3_SLEEP (t);
		m_xTimeout = SDL_GetTicks () + m_xDelay;

		if (gameOpts->menus.nStyle) 
			CCanvas::SetCurrent (NULL);
	GrUpdate (0);
	}
}

//-----------------------------------------------------------------------------

void CCreditsManager::Init (void)
{
m_bDone = 0;
m_bBinary = 0;
m_nExtraInc = 0;
m_nLines [0] = 0;
m_nLines [1] = 0;
m_nFirstLineOffs = 0;
m_xDelay = X2I (2800 * 1000);
m_bmBackdrop.Init ();
}

//-----------------------------------------------------------------------------

void CCreditsManager::Destroy (void)
{
m_cf.Close ();
for (int i = 0; i < 3; i++)
	fontManager.Unload (m_fonts [i]);
m_bmBackdrop.DestroyBuffer ();
}

//-----------------------------------------------------------------------------

//if filename passed is NULL, show Normal credits
void CCreditsManager::Show (char *creditsFilename)
{
	static char fontNames [3][2][13] = {
		{"font1-1.fnt", "font1-1h.fnt"},
		{"font2-3.fnt", "font2-3h.fnt"},
		{"font2-2.fnt", "font2-2h.fnt"}
	};
	
	static char szStars [2][15] = {"\0x1stars.pcx", "\0x1starsb.pcx"};
	
if (!Open (creditsFilename))
	return;

CCanvas::Push ();
memset (m_buffer, 0, sizeof (m_buffer));

SetScreenMode (SCREEN_MENU);
m_xOffs = (CCanvas::Current ()->Width () - 640) / 2;
m_yOffs = (CCanvas::Current ()->Height () - 480) / 2;
if (m_xOffs < 0)
	m_xOffs = 0;
if (m_yOffs < 0)
	m_yOffs = 0;
creditsPalette = paletteManager.Load ("credits.256", NULL);
paletteManager.LoadEffect ();
m_fonts [0] = fontManager.Load (fontNames [0][gameStates.menus.bHires]);
m_fonts [1] = fontManager.Load (fontNames [1][gameStates.menus.bHires]);
m_fonts [2] = fontManager.Load (fontNames [2][gameStates.menus.bHires]);
m_bmBackdrop.SetBuffer (NULL);
m_bmBackdrop.SetPalette (NULL);

int nPcxError = PCXReadBitmap (BackgroundName (BG_STARS), &m_bmBackdrop, BM_LINEAR, 0);
if (nPcxError != PCX_ERROR_NONE) {
	m_cf.Close ();
	CCanvas::Pop ();
	return;
	}
songManager.Play (SONG_CREDITS, 1);
m_bmBackdrop.Remap (NULL, -1, -1);

paletteManager.EnableEffect ();
KeyFlush ();
m_xTimeout = SDL_GetTicks () + m_xDelay;
glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
gameStates.menus.nInMenu = 1;
for (;;) {
	Read ();
	Render ();
	redbook.CheckRepeat();
	if (!HandleInput ())
		break;

	if (m_buffer [(m_nLines [0] + 1) %  NUM_LINES][0] == '!') {
		m_nFirstLineOffs -= ROW_SPACING - ROW_SPACING / 2;
		if (m_nFirstLineOffs <= -ROW_SPACING) {
			m_nFirstLineOffs += ROW_SPACING;
			m_nExtraInc++;
			}
		}
	}
CCanvas::Pop ();
backgroundManager.Redraw (true);
}
