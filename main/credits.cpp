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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "descent.h"
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
#include "ogl_render.h"
#include "renderlib.h"

#define LHX(x)      (gameStates.menus.bHires?2*(x):x)
#define LHY(y)      (gameStates.menus.bHires?(24*(y))/10:y)

#define ROW_SPACING (gameStates.menus.bHires?26:11)

uint8_t fadeValues[200] = { 1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,8,9,9,10,10,
11,11,12,12,12,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,19,19,20,20,
20,21,21,22,22,22,23,23,23,24,24,24,24,25,25,25,26,26,26,26,27,27,27,27,
28,28,28,28,28,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,30,30,
30,30,30,30,29,29,29,29,29,29,28,28,28,28,28,27,27,27,27,26,26,26,26,25,
25,25,24,24,24,24,23,23,23,22,22,22,21,21,20,20,20,19,19,19,18,18,17,17,
17,16,16,15,15,15,14,14,13,13,12,12,12,11,11,10,10,9,9,8,8,8,7,7,6,6,5,
5,4,4,3,3,2,2,1 };

uint8_t fadeValues_hires[480] = { 1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,
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

#define ALLOWED_CHAR 'R'

#if !DBG
#	define CREDITS_BACKGROUND_FILENAME (gameStates.menus.bHires ? "\x01starsb.pcx" : "\x01stars.pcx")	//only read from hog file
#else
#	define CREDITS_BACKGROUND_FILENAME (gameStates.menus.bHires ? "starsb.pcx" : "stars.pcx")
#endif

typedef struct box {
	int32_t left, top, width, height;
} box;

#define CREDITS_FILE    \
		  (CFile::Exist("mcredits.tex",gameFolders.game.szData [0],0)?"mcredits.tex":\
			CFile::Exist("ocredits.tex",gameFolders.game.szData [0],0)?"ocredits.tex":"credits.tex")

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
	"Yokelassence",
	"",
	"*Sounds",
	"Frustikus",
	"Red-5",
	"",
	"*Textures",
	"Aus-RED-5",
	"DarkFlameWolf",
	"DizzyRox",
	"MetalBeast",
	"Novacron",
	"Thiefbot",
	"",
	"*Tools",
	"Sirius",
	"",
	"*Testing",
	"Beltran",
	"Pokeman",
	"Pumo",
	"Pyro_2009",
	"Sykes",
	"Weyrman",
	"",
	"*Visit the D2X-XL project at",
	"http://www.descent2.de"
	};

#define NUM_XL_LINES	sizeofa(xlCredits)

#define FADE_DIST	240

CCreditsRenderer creditsRenderer;

//-----------------------------------------------------------------------------

uint32_t CCreditsRenderer::Read (void)
{
while (m_nExtraInc) {
	m_nLines [0] = (m_nLines [0] + 1) % NUM_LINES;
	for (;;) {
		if (m_cf.GetS (m_buffer [m_nLines [0]], 80)) {
			char *p = m_buffer [m_nLines [0]];
			if (m_bBinary)				// is this a binary tbl &m_cf
				for (int32_t i = (int32_t) strlen (m_buffer [m_nLines [0]]); i > 0; i--, p++)
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
	--m_nExtraInc;
	} 
m_nExtraInc = 0;
return m_bDone;
}

//-----------------------------------------------------------------------------

bool CCreditsRenderer::Open (char* creditsFilename)
{
	char filename [32];

m_bBinary = 0;
sprintf (filename, "%s", CREDITS_FILE);
if (creditsFilename) {
	strcpy (filename, creditsFilename);
	m_bBinary = 1;
	}
if (!m_cf.Open (filename, gameFolders.game.szData [0], "rb", 0)) {
	if (creditsFilename)
		return false;		//ok to not find special filename

	char nfile [32];
	char* pszTemp = strchr (filename, '.');
	if (pszTemp)
		*pszTemp = '\0';
	sprintf (nfile, "%s.txb", filename);
	if (!m_cf.Open (nfile, gameFolders.game.szData [0], "rb", 0)) {
		Error ("Missing CREDITS.TEX and CREDITS.TXB &m_cf\n");
		return false;
		}
	m_bBinary = 1;
	}
return true;
}

//-----------------------------------------------------------------------------

bool CCreditsRenderer::HandleInput (void)
{
int32_t k = KeyInKey ();

if ((k == KEY_PRINT_SCREEN) || (k == KEY_COMMAND + KEY_SHIFTED + KEY_P)) {
	gameStates.app.bSaveScreenShot = 1;
	SaveScreenShot (NULL, 0);
	}
else if ((k == KEY_ESC) || (m_bDone > uint32_t (NUM_LINES))) {
	Destroy ();
	paletteManager.DisableEffect ();
	paletteManager.Load (D2_DEFAULT_PALETTE, NULL);
	songManager.Play (SONG_TITLE, 1);
#if 0
	if (creditsOffscreenBuf != gameStates.render.vr.buffers.offscreen)
		creditsOffscreenBuf->Destroy ();
#endif
	ogl.SetBlending (false);
	gameStates.menus.nInMenu = 0;
	return false;
	}
return true;
}

//-----------------------------------------------------------------------------

void CCreditsRenderer::Render (void)
{
	CFloatVector		colors [4] = {{{{1,1,1,1}}},{{{1,1,1,1}}},{{{1,1,1,1}}},{{{1,1,1,1}}}};
	
for (int32_t i = 0; i < ROW_SPACING; i += gameStates.menus.bHires + 1) {
	backgroundManager.Draw (&m_background);
	m_background.Activate ();
	gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
	int32_t y = m_nFirstLineOffs - i;
	for (int32_t j = 0; j < NUM_LINES; j++) {
		int32_t l = (m_nLines [0] + j + 1) %  NUM_LINES;
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
			int32_t w, h, aw;

			fontManager.Current ()->StringSize (s, w, h, aw);
			if ((y >= 0) && (y + h <= 480)) {
				CBitmap* pBm = CreateStringBitmap (s, 0, 0, NULL, 1, w, 0, 1);
				if (pBm) {
					float dy = float ((y < FADE_DIST) ? y : (480 - y - h < FADE_DIST) ? 480 - y - h : FADE_DIST);
					colors [0].Alpha () = colors [1].Alpha () = dy / float (FADE_DIST);
					dy = float ((y + h < FADE_DIST) ? y + h : (480 - y - 2 * h < FADE_DIST) ? 480 - y - 2 * h : FADE_DIST);
					colors [2].Alpha () = colors [3].Alpha () = dy / float (FADE_DIST);
					pBm->Render (NULL, (CCanvas::Current ()->Width () - w) / 2, y, w, h, 0, 0, w, h, 1, 0, 0, 1, colors);
					delete pBm;
					}
				}
			}
		y += (m_buffer[l][0] == '!') ? ROW_SPACING / 2 : ROW_SPACING;
		}
	m_background.Deactivate ();
	}
}

//-----------------------------------------------------------------------------

void CCreditsRenderer::Init (void)
{
m_bDone = 0;
m_bBinary = 0;
m_nExtraInc = 0;
m_nLines [0] = 0;
m_nLines [1] = 0;
m_nFirstLineOffs = 0;
m_bmBackdrop.Init ();
}

//-----------------------------------------------------------------------------

void CCreditsRenderer::Destroy (void)
{
m_cf.Close ();
for (int32_t i = 0; i < 3; i++)
	fontManager.Unload (m_fonts [i]);
m_bmBackdrop.Destroy ();
}

//-----------------------------------------------------------------------------

//if filename passed is NULL, show Normal credits
void CCreditsRenderer::Show (char *creditsFilename)
{
	static char fontNames [3][2][13] = {
		{"font1-1.fnt", "font1-1h.fnt"},
		{"font2-3.fnt", "font2-3h.fnt"},
		{"font2-2.fnt", "font2-2h.fnt"}
	};
	
if (!Open (creditsFilename))
	return;

memset (m_buffer, 0, sizeof (m_buffer));

SetScreenMode (SCREEN_MENU);
backgroundManager.Setup (m_background, 640, 480, BG_TOPMENU, BG_STARS);
creditsPalette = paletteManager.Load ("credits.256", NULL);
//paletteManager.ResumeEffect ();
m_fonts [0] = fontManager.Load (fontNames [0][gameStates.menus.bHires]);
m_fonts [1] = fontManager.Load (fontNames [1][gameStates.menus.bHires]);
m_fonts [2] = fontManager.Load (fontNames [2][gameStates.menus.bHires]);
songManager.Play (SONG_CREDITS, 1);

KeyFlush ();
ogl.SetBlending (true);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
gameStates.menus.nInMenu = 1;
m_nExtraInc = 1;

uint32_t tRenderTimeout = SDL_GetTicks ();

for (;;) {
	Read ();

	CFrameController fc;
	for (fc.Begin (); fc.Continue (); fc.End ()) {
		gameData.render.scene.Activate ("CCreditsRenderer::Show (scene)");
		Render ();
		gameData.render.scene.Deactivate ();
		}
	ogl.Update (0);

	uint32_t t = SDL_GetTicks ();
	if (tRenderTimeout > t)
		G3_SLEEP (tRenderTimeout - t);
	tRenderTimeout = SDL_GetTicks () + 40; // throttle renderer at 25 fps

	redbook.CheckRepeat();
	if (!HandleInput ())
		break;

	if (m_buffer [(m_nLines [0] + 1) %  NUM_LINES][0] == '!')
		m_nFirstLineOffs -= ROW_SPACING / 2;
	else
		m_nFirstLineOffs -= 4;
	if (m_nFirstLineOffs <= -ROW_SPACING) {
		m_nFirstLineOffs += ROW_SPACING;
		m_nExtraInc++;
		}
	}
backgroundManager.Draw ();
}

//-----------------------------------------------------------------------------
