/* $Id: titles.c, v 1.29 2003/11/27 09:16:58 btb Exp $ */
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "timer.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "joy.h"
#include "mono.h"
#include "gamefont.h"
#include "cfile.h"
#include "error.h"
#include "polyobj.h"
#include "textures.h"
#include "screens.h"
#include "multi.h"
#include "player.h"
#include "digi.h"
#include "compbit.h"
#include "text.h"
#include "kmatrix.h"
#include "piggy.h"
#include "songs.h"
#include "newmenu.h"
#include "state.h"
#include "movie.h"
#include "menu.h"
#include "mouse.h"
#include "gamepal.h"
#include "cfile.h"
#include "globvars.h"
#include "strutil.h"
#include "vecmat.h"
#include "interp.h"

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texcache.h"
#ifdef __macosx__
# include <OpenGL/glu.h>
# undef GL_ARB_multitexture // hack!
#else
# include <GL/glu.h>
#endif

#define KEY_DELAY_DEFAULT       ((F1_0*20)/1000)

typedef struct {
	char    bs_name [14];                //  filename, eg merc01.  Assumes .lbm suffix.
	sbyte   nLevel;
	sbyte   nMessage;
	short   text_ulx, text_uly;         //  upper left x, y of text window
	short   text_width, text_height;    //  width and height of text window
} briefing_screen;


void DoBriefingColorStuff ();
int get_new_message_num (char **message);
int DefineBriefingBox (char **buf, briefing_screen *pBriefBuf);

extern unsigned RobSX, RobSY, RobDX, RobDY; // Robot movie coords

//static ubyte brief_palette [256*3];
//static ubyte robot_palette [256*3];
static int	brief_palette_254_bash;
static int  bInitRobot;

char curBriefScreenName [15] = "";
char	* szBriefingText = NULL;
int briefingTextLen;
char bRobotPlaying=0;

//Begin D1X modification
#define MAX_BRIEFING_COLORS     8
//End D1X modification

// Descent 1 briefings
#define	SHAREWARE_ENDING_FILENAME	"ending.tex"

//	Can be set by -noscreens command line option.  Causes bypassing of all briefing screens.
int	briefFgColorIndex [MAX_BRIEFING_COLORS], 
		briefBgColorIndex [MAX_BRIEFING_COLORS];
int	Current_color = 0;
unsigned int nEraseColor = BLACK_RGBA;

grsRgba briefFgColors [2][MAX_BRIEFING_COLORS] = {
	{{0, 160, 0, 255}, {160, 132, 140, 255}, {32, 124, 216, 255}, {0, 0, 216, 255}, {56, 56, 56, 255}, {216, 216, 0, 255}, {0, 216, 216, 255}, {255, 255, 255, 255}}, 
	{{0, 216, 0, 255}, {168, 152, 128, 255}, {252, 0, 0, 255}, {0, 0, 216, 255}, {56, 56, 56, 255}, {216, 216, 0, 255}, {0, 216, 216, 255}, {255, 255, 255, 255}}
	};

grsRgba briefBgColors [2][MAX_BRIEFING_COLORS] = {
	{{0, 24, 255}, {20, 20, 20, 255}, {4, 16, 28, 255}, {0, 0, 76, 255}, {0, 0, 0, 255}, {76, 76, 0, 255}, {0, 76, 76, 255}, {255, 255, 255, 255}}, 
	{{0, 76, 0, 255}, {56, 56, 56, 255}, {124, 0, 0, 255}, {0, 0, 76, 255}, {0, 0, 0, 255}, {76, 76, 0, 255}, {0, 76, 76, 255}, {255, 255, 255, 255}}
	};

grsRgba eraseColorRgb = {0, 0, 0, 255};

typedef struct tD1ExtraBotSound {
	char	*pszName;
	short nLevel;
	short	nBot;
} tD1ExtraBotSound;

tD1ExtraBotSound extraBotSounds [] = {
	{"enemy01.wav", 1, 0}, 
	{"enemy02.wav", 1, 1}, 
	{"enemy03.wav", 1, 2}, 
	{"enemy04.wav", 1, 3}, 
	{"enemy05.wav", 1, 4}, 
	{"enemy06.wav", 1, 5}, 
	{"enemy07.wav", 6, 0}, 
	{"enemy08.wav", 8, 0}, 
	{"enemy09.wav", 8, 1}, 
	{"enemy10.wav", 8, 2}, 
	{"enemy11.wav", 10, 0}, 
	{"enemy12.wav", 13, 0}, 
	{"enemy13.wav", 16, 0}, 
	{"enemy14.wav", 17, 0}
};

//-----------------------------------------------------------------------------

int StartBriefingSound (int c, short nSound, fix nVolume, char *pszWAV)
{
if (c < 0) {
	int b = gameOpts->sound.bD1Sound;
	gameOpts->sound.bD1Sound = 0;
	c = DigiStartSound (DigiXlatSound (nSound), nVolume, 0xFFFF / 2, 1, -1, -1, -1, F1_0, pszWAV, NULL);
	gameOpts->sound.bD1Sound = b;
	}
return c;
}

//-----------------------------------------------------------------------------

void StopBriefingSound (int *c)
{
if (*c > -1) {
	DigiStopSound (*c);
	*c = -1;
	}
}

//-----------------------------------------------------------------------------

int StartBriefingHum (int nChannel, int nLevel, int nScreen, int bExtraSounds)
{
if (bExtraSounds && (nScreen > 3)) {	//only play for the mission directives on a planet background
	char	szWAV [20];

	if (nLevel < 0)
		nLevel = 27 - nLevel;
 	sprintf (szWAV, "brf%02d.wav", nLevel);
	return StartBriefingSound (nChannel, SOUND_BRIEFING_HUM, F1_0 * 4, szWAV);
	}
return StartBriefingSound (nChannel, SOUND_BRIEFING_HUM, F1_0 / 2, NULL);
}

//-----------------------------------------------------------------------------

tD1ExtraBotSound *FindExtraBotSound (short nLevel, short nBot)
{
	tD1ExtraBotSound	*p = extraBotSounds;
	int					i = sizeof (extraBotSounds) / sizeof (tD1ExtraBotSound);

if (!gameStates.app.bD1Mission)
	return NULL;
for (; i; i--, p++)
	if ((p->nLevel == nLevel) && (p->nBot == nBot))
		return p;
return NULL;
}

//-----------------------------------------------------------------------------

int StartExtraBotSound (int nChannel, short nLevel, short nBot)
{
	tD1ExtraBotSound	*p;

StopBriefingSound (&nChannel);
if (!gameStates.app.bHaveExtraData)
	return -1;
if (!(p = FindExtraBotSound (nLevel, nBot)))
	return -1;
return StartBriefingSound (nChannel, -1, 8 * F1_0, p->pszName);
}

//-----------------------------------------------------------------------------

extern int NMCheckButtonPress ();

// added by Jan Bobrowski for variable-size menu screen
static int rescale_x (int x)
{
	return x * GWIDTH / 320;
}

//-----------------------------------------------------------------------------

static inline int RescaleY (int y)
{
return y * GHEIGHT / 200;
}

//-----------------------------------------------------------------------------

int local_key_inkey (void)
{
	int	rval = KeyInKey ();

if (rval == KEY_PRINT_SCREEN) {
	SaveScreenShot (NULL, 0);
	return 0;				//say no key pressed
	}
if (NMCheckButtonPress ())		//joystick or mouse button pressed?
	return KEY_SPACEBAR;
if (MouseButtonState (0))
	return KEY_SPACEBAR;
return rval;
}

//-----------------------------------------------------------------------------

int LoadBriefImg (char *pszImg, grsBitmap *bmP, int bFullScr)
{
	char	*ps, c = '0';
	char	szImg [FILENAME_LEN+1];
	int	pcxErr;

strcpy (szImg, pszImg);
strlwr (szImg);
if ((ps = strstr (szImg, "b.pcx")))
	 c = 'b';
else if ((ps = strstr (szImg, ".pcx"))) {
	c = *--ps;
	strcpy (--ps, "0b.pcx");
	*ps = c;
	}

if (strstr (szImg, ".tga")) {
	grsBitmap bm;
	if (!ReadTGA (szImg, gameFolders.szDataDir, &bm, -1, 1.0, 0, 0))
		return PCX_ERROR_OPENING;
	if (bFullScr) {
		ShowFullscreenImage (&bm);
		GrFreeBitmapData (&bm);
		}
	return PCX_ERROR_NONE;
	}
else {
	for (;;) {
		pcxErr = bFullScr ?
			PcxReadFullScrImage (szImg, gameStates.app.bD1Mission) :
			PCXReadBitmap (szImg, bmP, BM_LINEAR, gameStates.app.bD1Mission);
		if (pcxErr == PCX_ERROR_NONE)
			break;
		if (!ps)
			break;
		strcpy (ps, "00.pcx");
		*++ps = c;
		ps = NULL;
		} 
	}
return pcxErr;
}

//-----------------------------------------------------------------------------

int ShowBriefingImage (char * filename, int bAllowKeys, int from_hog_only)
{
	fix timer;
	int pcxResult;
	grsBitmap title_bm;
	char new_filename [FILENAME_LEN+1] = "";

#ifndef _DEBUG
if (from_hog_only)
	strcpy (new_filename, "\x01");	//only read from hog file
#endif

strcat (new_filename, filename);
filename = new_filename;

title_bm.bmTexBuf=NULL;
if ((pcxResult = LoadBriefImg (filename, &title_bm, 0)) != PCX_ERROR_NONE) {
#if TRACE
	con_printf (CONDBG, "File '%s', PCX load error: %s (%i)\n  (No big deal, just no title screen.)\n", filename, pcx_errormsg (pcxResult), pcxResult);
#endif
	Error ("Error loading briefing screen <%s>, PCX load error: %s (%i)\n", filename, pcx_errormsg (pcxResult), pcxResult);
}
//vfx_set_palette_sub (brief_palette);
GrPaletteStepLoad (NULL);
GrSetCurrentCanvas (NULL);
ShowFullscreenImage (&title_bm);
if (GrPaletteFadeIn (NULL, 32, bAllowKeys))
	return 1;

GrPaletteStepLoad (NULL);
timer	= TimerGetFixedSeconds () + i2f (3);
while (1) {
	if (local_key_inkey () && bAllowKeys) break;
	if (TimerGetFixedSeconds () > timer) break;
}
if (GrPaletteFadeOut (NULL, 32, bAllowKeys))
	return 1;
D2_FREE (title_bm.bmTexBuf);
return 0;
}

//-----------------------------------------------------------------------------

#define BRIEFING_SECRET_NUM 31          //  This must correspond to the first secret level which must come at the end of the list.
#define BRIEFING_OFFSET_NUM 4           // This must correspond to the first level screen (ie, past the bald guy briefing screens)

#define	SHAREWARE_ENDING_LEVEL_NUM  0x7f
#define	REGISTERED_ENDING_LEVEL_NUM 0x7e

#define ENDING_LEVEL_NUM 	REGISTERED_ENDING_LEVEL_NUM

#define MAX_BRIEFING_SCREENS 60

#if 0
briefing_screen briefingScreens [MAX_BRIEFING_SCREENS]=
 {{"brief03.pcx", 0, 3, 8, 8, 257, 177}}; // default=0!!!
#else
briefing_screen briefingScreens [] = {
	{ "brief01.pcx",   0,  1,  13, 140, 290,  59 }, 
	{ "brief02.pcx",   0,  2,  27,  34, 257, 177 }, 
	{ "brief03.pcx",   0,  3,  20,  22, 257, 177 }, 
	{ "brief02.pcx",   0,  4,  27,  34, 257, 177 }, 

	{ "moon01.pcx",    1,  5,  10,  10, 300, 170 }, // level 1
	{ "moon01.pcx",    2,  6,  10,  10, 300, 170 }, // level 2
	{ "moon01.pcx",    3,  7,  10,  10, 300, 170 }, // level 3

	{ "venus01.pcx",   4,  8,  15, 15, 300,  200 }, // level 4
	{ "venus01.pcx",   5,  9,  15, 15, 300,  200 }, // level 5

	{ "brief03.pcx",   6, 10,  20,  22, 257, 177 }, 
	{ "merc01.pcx",    6, 11,  10, 15, 300, 200 },  // level 6
	{ "merc01.pcx",    7, 12,  10, 15, 300, 200 },  // level 7

	{ "brief03.pcx",   8, 13,  20,  22, 257, 177 }, 
	{ "mars01.pcx",    8, 14,  10, 100, 300,  200 }, // level 8
	{ "mars01.pcx",    9, 15,  10, 100, 300,  200 }, // level 9
	{ "brief03.pcx",  10, 16,  20,  22, 257, 177 }, 
	{ "mars01.pcx",   10, 17,  10, 100, 300,  200 }, // level 10

	{ "jup01.pcx",    11, 18,  10, 40, 300,  200 }, // level 11
	{ "jup01.pcx",    12, 19,  10, 40, 300,  200 }, // level 12
	{ "brief03.pcx",  13, 20,  20,  22, 257, 177 }, 
	{ "jup01.pcx",    13, 21,  10, 40, 300,  200 }, // level 13
	{ "jup01.pcx",    14, 22,  10, 40, 300,  200 }, // level 14

	{ "saturn01.pcx", 15, 23,  10, 40, 300,  200 }, // level 15
	{ "brief03.pcx",  16, 24,  20,  22, 257, 177 }, 
	{ "saturn01.pcx", 16, 25,  10, 40, 300,  200 }, // level 16
	{ "brief03.pcx",  17, 26,  20,  22, 257, 177 }, 
	{ "saturn01.pcx", 17, 27,  10, 40, 300,  200 }, // level 17

	{ "uranus01.pcx", 18, 28,  100, 100, 300,  200 }, // level 18
	{ "uranus01.pcx", 19, 29,  100, 100, 300,  200 }, // level 19
	{ "uranus01.pcx", 20, 30,  100, 100, 300,  200 }, // level 20
	{ "uranus01.pcx", 21, 31,  100, 100, 300,  200 }, // level 21

	{ "neptun01.pcx", 22, 32,  10, 20, 300,  200 }, // level 22
	{ "neptun01.pcx", 23, 33,  10, 20, 300,  200 }, // level 23
	{ "neptun01.pcx", 24, 34,  10, 20, 300,  200 }, // level 24

	{ "pluto01.pcx",  25, 35,  10, 20, 300,  200 }, // level 25
	{ "pluto01.pcx",  26, 36,  10, 20, 300,  200 }, // level 26
	{ "pluto01.pcx",  27, 37,  10, 20, 300,  200 }, // level 27

	{ "aster01.pcx",  -1, 38,  10, 90, 300,  200 }, // secret level -1
	{ "aster01.pcx",  -2, 39,  10, 90, 300,  200 }, // secret level -2
	{ "aster01.pcx",  -3, 40,  10, 90, 300,  200 }, // secret level -3

	{ "end01.pcx",   SHAREWARE_ENDING_LEVEL_NUM,  1,  23, 40, 320, 200 },   // shareware end
	{ "end02.pcx",   REGISTERED_ENDING_LEVEL_NUM,  1,  5, 5, 300, 200 },    // registered end
	{ "end01.pcx",   REGISTERED_ENDING_LEVEL_NUM,  2,  23, 40, 320, 200 },  // registered end
	{ "end03.pcx",   REGISTERED_ENDING_LEVEL_NUM,  3,  5, 5, 300, 200 },    // registered end

};

#endif

int	Briefing_text_x, Briefing_text_y;

gsrCanvas	*robotCanv = NULL;
vmsAngVec	Robot_angles;

char    Bitmap_name [32] = "";
#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      //  Adam: This is the number of frames in your new animating thing.
#define DOOR_DIV_INIT   6
sbyte   Door_dir=1, Door_divCount=0, Animating_bitmapType=0;

//-----------------------------------------------------------------------------

void init_char_pos (briefing_screen *bsp, int bRescale)
{
if (bRescale) {
	bsp->text_ulx = rescale_x (bsp->text_ulx);
	bsp->text_uly = RescaleY (bsp->text_uly);
	bsp->text_width = rescale_x (bsp->text_width);
	bsp->text_height = RescaleY (bsp->text_height);
	}
Briefing_text_x = bsp->text_ulx;
Briefing_text_y = gameStates.app.bD1Mission ? bsp->text_uly : bsp->text_uly - (8 * (1 + gameStates.menus.bHires));
}

//-----------------------------------------------------------------------------

void ShowBitmapFrame (int bRedraw)
{
	gsrCanvas *curCanvSave, *bitmap_canv=0;
	vmsVector p = ZERO_VECTOR;

	grsBitmap *bmP;
	int x = rescale_x (138);
	int y = RescaleY (55);
	int w = rescale_x (166);
	int h = RescaleY (138);

	//	Only plot every nth frame.
	if (!bRedraw && Door_divCount) {
		Door_divCount--;
		if (!gameOpts->menus.nStyle)
			return;
	}

	if (Bitmap_name [0] != 0) {
		char		*pound_signp;
		int		num, dig1, dig2;
		//	Set supertransparency color to black
		switch (Animating_bitmapType) {
			case 0: 
				bitmap_canv = GrCreateSubCanvas (grdCurCanv, x, y, w, h);
				break;
			case 1:
				bitmap_canv = GrCreateSubCanvas (grdCurCanv, x, y, w, h);
				break;
			// Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
			default:
				Int3 (); // Impossible, illegal value for Animating_bitmapType
			}
		curCanvSave = grdCurCanv; 
		grdCurCanv = bitmap_canv;
		GrClearCanvas (0);
		if (!bRedraw) {	//extract current bitmap number from bitmap name (<name>#<number>)
			pound_signp = strchr (Bitmap_name, '#');
			Assert (pound_signp != NULL);

			dig1 = * (pound_signp+1);
			dig2 = * (pound_signp+2);
			if (dig2 == 0)
				num = dig1-'0';
			else
				num = (dig1-'0')*10 + (dig2-'0');

			switch (Animating_bitmapType) {
				case 0:
					if (!Door_divCount) {
						num += Door_dir;
						if (num > EXIT_DOOR_MAX) {
							num = EXIT_DOOR_MAX;
							Door_dir = -1;
						} else if (num < 0) {
							num = 0;
							Door_dir = 1;
						}
					}
					break;
				case 1:
					if (!Door_divCount)
						num++;
					if (num > OTHER_THING_MAX)
						num = 0;
					break;
				}

			Assert (num < 100);
			if (num >= 10) {
				* (pound_signp+1) = (num / 10) + '0';
				* (pound_signp+2) = (num % 10) + '0';
				* (pound_signp+3) = 0;
				} 
			else {
				* (pound_signp+1) = (num % 10) + '0';
				* (pound_signp+2) = 0;
				}
			}

		LoadPalette ("", "", 0, 0, 1);
		{
		tBitmapIndex bmi = PiggyFindBitmap (Bitmap_name, 0);
		bmP = gameData.pig.tex.bitmaps [0] + bmi.index;
		PIGGY_PAGE_IN (bmi, 0);
		}

		{
		GLint	depthFunc;
		G3StartFrame (1, 0);
		G3SetViewMatrix (&p, &mIdentity, gameStates.render.xZoom);
		p.p.z = 2 * w * F1_0;
		glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
		glDepthFunc (GL_ALWAYS);
		G3DrawBitmap (&p, w * F1_0, h * F1_0, bmP, NULL, 1.0, 3);
		glDepthFunc (depthFunc);
		G3EndFrame ();
		if (!gameOpts->menus.nStyle)
			GrUpdate (0);
		}
		GrPaletteStepLoad (NULL);
		grdCurCanv = curCanvSave;
		D2_FREE (bitmap_canv);
		if (!(bRedraw || Door_divCount)) {
#if 1
		Door_divCount = DOOR_DIV_INIT;
#else
		switch (Animating_bitmapType) {
			case 0:
				if (num == EXIT_DOOR_MAX) {
					Door_dir = -1;
					Door_divCount = DOOR_DIV_INIT;
					} 
				else if (num == 0) {
					Door_dir = 1;
					Door_divCount = DOOR_DIV_INIT;
				}
				break;
			case 1:
				break;
			}
#endif
		}
	}
}

//-----------------------------------------------------------------------------

void show_briefing_bitmap (grsBitmap *bmp)
{
	gsrCanvas	*curCanvSave, *bitmap_canv;

bitmap_canv = GrCreateSubCanvas (grdCurCanv, 220, 45, bmp->bmProps.w, bmp->bmProps.h);
curCanvSave = grdCurCanv;
GrSetCurrentCanvas (bitmap_canv);
GrBitmapM (0, 0, bmp, 0);
GrSetCurrentCanvas (curCanvSave);
D2_FREE (bitmap_canv);
}

//-----------------------------------------------------------------------------

void ShowSpinningRobotFrame (int nRobot)
{
	gsrCanvas	*curCanvSave;

if (nRobot != -1) {
	Robot_angles.h += 150;

	curCanvSave = grdCurCanv;
	grdCurCanv = robotCanv;
	Assert (ROBOTINFO (nRobot).nModel != -1);
	if (bInitRobot) {
		LoadPalette ("", "", 0, 0, 1);
		OglCachePolyModelTextures (ROBOTINFO (nRobot).nModel);
		GrPaletteStepLoad (NULL);
		bInitRobot = 0;
		}
	DrawModelPicture (ROBOTINFO (nRobot).nModel, &Robot_angles);
	grdCurCanv = curCanvSave;
	}
}

//-----------------------------------------------------------------------------

void RotateBriefingRobot (void)
{
gsrCanvas	*curCanvSave = grdCurCanv;
grdCurCanv = robotCanv;
RotateRobot ();
grdCurCanv = curCanvSave;
}

//-----------------------------------------------------------------------------

void InitSpinningRobot (void) // (int x, int y, int w, int h)
{
	//Robot_angles.p += 0;
	//Robot_angles.b += 0;
	//Robot_angles.h += 0;

	int x = rescale_x (138);
	int y = RescaleY (55);
	int w = rescale_x (163);
	int h = RescaleY (136);

	robotCanv = GrCreateSubCanvas (grdCurCanv, x, y, w, h);
	bInitRobot = 1;
	// 138, 55, 166, 138
}

//---------------------------------------------------------------------------
// Returns char width.
// If showRobotFlag set, then show a frame of the spinning robot.
int PrintCharDelayed (char the_char, int delay, int nRobot, int cursorFlag, int bRedraw)
{
	int w, h, aw;
	char message [2];
	static fix	t, t0, StartTime=0;

	message [0] = the_char;
	message [1] = 0;

	if (StartTime==0 && TimerGetFixedSeconds ()<0)
		StartTime=TimerGetFixedSeconds ();

GrGetStringSize (message, &w, &h, &aw);

Assert ((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));

//	Draw cursor if there is some delay and caller says to draw cursor
if (cursorFlag && !bRedraw) {
	WIN (DDGRLOCK (dd_grd_curcanv));
	GrSetFontColorRGB (briefFgColors [gameStates.app.bD1Mission] + Current_color, NULL);
	GrPrintF (NULL, Briefing_text_x+1, Briefing_text_y, "_");
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	if (!gameOpts->menus.nStyle)
		GrUpdate (0);
}

if (delay > 0)
	delay=FixDiv (F1_0, i2f (15));

if ((delay > 0) && !bRedraw) {
	if (Bitmap_name [0] != 0)
		ShowBitmapFrame (0);
	if (bRobotPlaying)
		RotateBriefingRobot ();
	t0 = StartTime;
	while ((t = TimerGetFixedSeconds ()) < (StartTime + delay)) {
		if (bRobotPlaying)
			RotateBriefingRobot ();
		if (t >= t0 + KEY_DELAY_DEFAULT/2) {
			if ((Bitmap_name [0] != 0) && (delay != 0))
				ShowBitmapFrame (0);
			if (nRobot != -1)
				ShowSpinningRobotFrame (nRobot);
				t0 = t;
			}
		}
	}

StartTime = TimerGetFixedSeconds ();

WIN (DDGRLOCK (dd_grd_curcanv));
//	Erase cursor
if (cursorFlag && (delay > 0) && !bRedraw) {
	GrSetFontColorRGBi (nEraseColor, 1, 0, 0);
	GrPrintF (NULL, Briefing_text_x+1, Briefing_text_y, "_");
	//	erase the character
	GrSetFontColorRGB (briefBgColors [gameStates.app.bD1Mission] + Current_color, NULL);
	GrPrintF (NULL, Briefing_text_x, Briefing_text_y, message);
}
//draw the character
GrSetFontColorRGB (briefFgColors [gameStates.app.bD1Mission] + Current_color, NULL);
GrPrintF (NULL, Briefing_text_x+1, Briefing_text_y, message);
WIN (DDGRUNLOCK (dd_grd_curcanv));

if (!(bRedraw || gameOpts->menus.nStyle)) 
	GrUpdate (0);

//	if (the_char != ' ')
//		if (!DigiIsSoundPlaying (SOUND_MARKER_HIT))
//			DigiPlaySample (SOUND_MARKER_HIT, F1_0);

return w;
}

//-----------------------------------------------------------------------------

int LoadBriefingScreen (int nScreen)
{
	int	pcxResult;
	char *szBriefScreen;

if (gameStates.app.bD1Mission)
	szBriefScreen = briefingScreens [nScreen].bs_name;
else
	szBriefScreen = curBriefScreenName;
if (*szBriefScreen) {
	glClear (GL_COLOR_BUFFER_BIT);
	WIN (DDGRLOCK (dd_grd_curcanv));
	if ((pcxResult = PcxReadFullScrImage (szBriefScreen, gameStates.app.bD1Mission)) != PCX_ERROR_NONE) {
		WIN (DDGRUNLOCK (dd_grd_curcanv));
#ifdef _DEBUG
		Error ("Error loading briefing screen <%s>, \nPCX load error: %s (%i)\n", szBriefScreen, pcx_errormsg (pcxResult), pcxResult);
#endif
		}
	WIN (DDGRUNLOCK (dd_grd_curcanv));
	WIN (DDGRRESTORE);
	}
return 0;
}

//-----------------------------------------------------------------------------

int LoadNewBriefingScreen (char *szBriefScreen, int bRedraw)
{
	int pcxResult;

#if TRACE
con_printf (CONDBG, "Loading new briefing <%s>\n", szBriefScreen);
#endif
strcpy (curBriefScreenName, szBriefScreen);
if (GrPaletteFadeOut (NULL, 32, 0))
	return 0;
WIN (DDGRLOCK (dd_grd_curcanv));
if ((pcxResult = LoadBriefImg (szBriefScreen, NULL, 1)) != PCX_ERROR_NONE) {
	WIN (DDGRUNLOCK (dd_grd_curcanv));
#ifdef _DEBUG
	Error ("Error loading briefing screen <%s>, \nPCX load error: %s (%i)\n", 
			szBriefScreen, pcx_errormsg (pcxResult), pcxResult);
#endif
	}
if (GrPaletteFadeIn (NULL, 32, 0))
	return 0;
DoBriefingColorStuff ();
return 1;
}

//-----------------------------------------------------------------------------

int GetMessageNum (char **message)
{
	int	num=0;
	char	*psz = *message;

while (*psz && (*psz == ' '))
	psz++;
while ((*psz >= '0') && (*psz <= '9')) {
	num = 10*num + *psz-'0';
	psz++;
	}
while (*psz && (*psz++ != 10))		//	Get and drop eoln
	;
*message = psz;
return num;
}

//-----------------------------------------------------------------------------

void get_message_name (char **message, char *result)
{
	char	*psz = *message;

while (*psz == ' ')
	 (*message)++;

while ((*psz != ' ') && (*psz != 10)) {
	if (*psz != '\n')
		*result++ = *psz;
	psz++;
	}
if (*psz != 10)
	while (*psz++ != 10)		//	Get and drop eoln
		;
*message = psz;
*result = 0;
}

//-----------------------------------------------------------------------------

void flash_cursor (int cursorFlag)
{
	if (cursorFlag == 0)
		return;

WIN (DDGRLOCK (dd_grd_curcanv));
	if ((TimerGetFixedSeconds () % (F1_0/2)) > (F1_0/4))
		GrSetFontColorRGB (briefFgColors [gameStates.app.bD1Mission] + Current_color, NULL);
	else
		GrSetFontColorRGB (&eraseColorRgb, NULL);

	GrPrintF (NULL, Briefing_text_x+1, Briefing_text_y, "_");
WIN (DDGRUNLOCK (dd_grd_curcanv));
if (curDrawBuffer == GL_FRONT)
	GrUpdate (0);
}

extern int InitMovieBriefing ();

//-----------------------------------------------------------------------------

char *NextPage (char *message)
{
	char	*pNextPage = strstr (message, "$P");
	char	*pNextBrief = strstr (message, "$S");

if (pNextPage && pNextBrief)
	return ((pNextPage < pNextBrief) ? pNextPage : pNextBrief);
return (pNextPage ? pNextPage : pNextBrief);
return NULL;
}

//-----------------------------------------------------------------------------

int PageHasRobot (char *message)
{
	char	*pEnd = NextPage (message);
	char	*pBot = strstr (message, "$R");

return pBot && (!pEnd || (pBot < pEnd));
}

//-----------------------------------------------------------------------------

char *SkipPage (char *message, briefing_screen *pBriefBuf, int *px, int *py, int *pnScreen)
{
	char	ch, *pEnd = NextPage (message);
	int	nScreen = *pnScreen, x = *px, y = *py;

while (*message && (!pEnd || (message < pEnd))) {
	ch = *message++;
	if (ch == '$') {
		ch = *message++;
		if (ch == 'D') {
			nScreen = DefineBriefingBox (&message, pBriefBuf);
			x = Briefing_text_x;
			y = Briefing_text_y;
			}
		else if (ch == 'U') {
			nScreen = GetMessageNum (&message);
			*pBriefBuf = briefingScreens [nScreen];
			init_char_pos (pBriefBuf, 1);
			x = Briefing_text_x;
			y = Briefing_text_y;
			}
		}
	}
*pnScreen = nScreen;
*px = x;
*py = y;
if (!message)
	return NULL;
ch = *message;
if (ch == '$') {
	ch = *++message;
	if (ch == 'S')
		return NULL;
	do {
		message++;
		} while (isdigit (*message) || isspace (*message));
	}
return message;
}

//-----------------------------------------------------------------------------
// Return true if message got aborted by user (pressed ESC), else return false.
int ShowBriefingMessage (int nScreen, char *message, int nLevel)
{
	static int tab_stop=0;

	briefing_screen	*bsp, briefBuf;
	int			prev_ch=-1;
	int			ch, done=0, i;
	int			delayCount = KEY_DELAY_DEFAULT;
	int			key_check;
	int			nRobot=-1;
	int			rval=0;
	int			flashing_cursor=0;
	int			new_page=0, GotZ=0;
	int			nHumChannel = -1, nPrintingChannel = -1, nBotChannel = -1;
	int			LineAdjustment=1;
	int			bHaveScreen = 0;
	char			*pi, *pj;
	int			x, y, bRedraw, bPageDone;
	char			spinRobotName []="rba.mve", kludge;  // matt don't change this!
	char			szBriefScreen [15], szBriefScreenB [15];
	char			bDumbAdjust=0;
	char			chattering=0;
	short 		nBot = 0;
	//char			*pEnd = strstr (message, "$S");
	int			bOnlyRobots, bExtraSounds;
	WIN (int wpage_done=0);

Bitmap_name [0] = 0;
Current_color = 0;
bRobotPlaying=0;

InitMovieBriefing ();

bExtraSounds = gameStates.app.bHaveExtraData && gameStates.app.bD1Mission && 
				  (gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission);
bOnlyRobots = gameStates.app.bHaveExtraMovies && bExtraSounds && (nLevel == 1) && (nScreen < 4);

nHumChannel = StartBriefingHum (nHumChannel, nLevel, nScreen, bExtraSounds);

GrSetCurFont (GAME_FONT);

briefBuf = briefingScreens [nScreen];
bsp = &briefBuf;
if (gameStates.app.bD1Mission)
	GotZ = 1;
init_char_pos (bsp, gameStates.app.bD1Mission);

x = Briefing_text_x;
y = Briefing_text_y;
*szBriefScreen = *szBriefScreenB = '\0';
if (bOnlyRobots) {
	while (message && !PageHasRobot (message))
		message = SkipPage (message, &briefBuf, &x, &y, &nScreen);
	if (!message)
		goto done;
	}
pi = pj = message;

redrawPage:

while (!done) {
	pj = message;
	if (curDrawBuffer == GL_FRONT)
		pi = message;
	else {
		GrUpdate (0);
		//StopBriefingSound (&nPrintingChannel);
		message = pi;
		Briefing_text_x = x;
		Briefing_text_y = y;
		//if (bHaveScreen)
		LoadBriefingScreen (nScreen);
		briefBuf = briefingScreens [nScreen];
		bsp = &briefBuf;
		init_char_pos (bsp, 1);
		}

	bRedraw = 1;
	bPageDone = 0;
	new_page = 0;
	while (bRedraw) {
		ch = *message++;
		if (ch == '$') {
			ch = *message++;
			if (ch == 'D') {
				nScreen = DefineBriefingBox (&message, &briefBuf);
		    	//LoadNewBriefingScreen (briefingScreens [nScreen].bs_name);

				bsp = &briefBuf;
				x = Briefing_text_x;
				y = Briefing_text_y;
				LineAdjustment = 0;
				prev_ch = 10;                                   // read to eoln
				}
			else if (ch == 'U') {
				nScreen = GetMessageNum (&message);
				briefBuf = briefingScreens [nScreen];
				bsp = &briefBuf;
				init_char_pos (bsp, 1);
				x = Briefing_text_x;
				y = Briefing_text_y;
				prev_ch = 10;                                   // read to eoln
				}
			else if (ch == 'C') {
				Current_color = GetMessageNum (&message) - 1;
				Assert ((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
				prev_ch = 10;
				}
			else if (ch == 'F') {     // toggle flashing cursor
				flashing_cursor = !flashing_cursor;
				prev_ch = 10;
				while (*message++ != 10)
					;
				}
			else if (ch == 'T') {
				tab_stop = GetMessageNum (&message) * (1 + gameStates.menus.bHires);
				prev_ch = 10;							//	read to eoln
				}
			else if (ch == 'R') {
				if (message > pj) {
					if (robotCanv != NULL) {
						D2_FREE (robotCanv);
						robotCanv = NULL;
						}
					if (bRobotPlaying) {
						DeInitRobotMovie ();
						bRobotPlaying = 0;
						}
					InitSpinningRobot ();
					nBotChannel = StartExtraBotSound (nBotChannel, (short) nLevel, nBot++);
					if (nBotChannel >= 0)
						StopBriefingSound (&nHumChannel);
					}
				if (gameStates.app.bD1Mission) {
					nRobot = GetMessageNum (&message);
					while (*message++ != 10)
						;
					}
				else {
					kludge = *message++;
					spinRobotName [2] = kludge; // ugly but proud
					if (message > pj) {
						gsrCanvas *curCanvSave = grdCurCanv;
						grdCurCanv = robotCanv;
						bRobotPlaying = InitRobotMovie (spinRobotName);
						grdCurCanv = curCanvSave;
						if (bRobotPlaying) {
							RotateBriefingRobot ();
							DoBriefingColorStuff ();
						}
					}
				}
				prev_ch = 10;                           // read to eoln
				}
			else if (ch == 'N') {
				if (message > pj) {
				//--grsBitmap *bmP;
					if (robotCanv != NULL)
						D2_FREE (robotCanv);
					StopBriefingSound (&nBotChannel);
					get_message_name (&message, Bitmap_name);
					strcat (Bitmap_name, "#0");
					Animating_bitmapType = 0;
					}
				prev_ch = 10;
				}
			else if (ch == 'O') {
				if (message > pj) {
					if (robotCanv != NULL)
						D2_FREE (robotCanv);
					get_message_name (&message, Bitmap_name);
					strcat (Bitmap_name, "#0");
					Animating_bitmapType = 1;
					}
				prev_ch = 10;
				}
			else if (ch == 'A') {
				LineAdjustment=1-LineAdjustment;
				}
			else if (ch == 'Z') {
				GotZ = 1;
				bDumbAdjust = 1;
				i = 0;
				while ((szBriefScreen [i] = *message) != '\n') {
					i++;
					message++;
					}
				szBriefScreen [i] = 0;
				if (*message != 10)
					while (*message++ != 10)    //  Get and drop eoln
						;
				for (i = 0; szBriefScreen [i] != '.'; i++)
					szBriefScreenB [i] = szBriefScreen [i];
				memcpy (szBriefScreenB + i, "b.pcx", sizeof ("b.pcx"));
				i += sizeof ("b.pcx");
				if ((gameStates.menus.bHires && CFExist (szBriefScreenB, gameFolders.szDataDir, 0)) || 
					 !CFExist (szBriefScreen, gameFolders.szDataDir, 0))
					LoadNewBriefingScreen (szBriefScreenB, message <= pj);
				else
					LoadNewBriefingScreen (szBriefScreen, message <= pj);
				bHaveScreen = 1;
				}
			else if (ch == 'B') {
				char        bitmap_name [32];
				grsBitmap  guy_bitmap;
				int         iff_error;

				if (message > pj) {
					if (robotCanv != NULL) {
						D2_FREE (robotCanv);
						robotCanv=NULL;
						}
					}
				get_message_name (&message, bitmap_name);
				strcat (bitmap_name, ".bbm");
				memset (&guy_bitmap, 0, sizeof (guy_bitmap));
				iff_error = iff_read_bitmap (bitmap_name, &guy_bitmap, BM_LINEAR);
				Assert (iff_error == IFF_NO_ERROR);

				show_briefing_bitmap (&guy_bitmap);
				D2_FREE (guy_bitmap.bmTexBuf);
				prev_ch = 10;
				}
			else if (ch == 'S') {
				int keypress = 0;
				fix StartTime;

				chattering=0;
				StopBriefingSound (&nPrintingChannel);

				StartTime = TimerGetFixedSeconds ();
				do {		//	Wait for a key
					while (TimerGetFixedSeconds () < StartTime + KEY_DELAY_DEFAULT / 2)
						;
					flash_cursor (flashing_cursor);
					if (bRobotPlaying)
						RotateBriefingRobot ();
					if (nRobot != -1)
						ShowSpinningRobotFrame (nRobot);
					if (Bitmap_name [0] != 0)
						ShowBitmapFrame (0);
					StartTime += KEY_DELAY_DEFAULT/2;
					//GrUpdate (0);
					keypress = local_key_inkey ();
					if (curDrawBuffer == GL_BACK)
						break;
					} while (!keypress);
				if (keypress) {
#ifdef _DEBUG
					if (keypress == KEY_BACKSP)
						Int3 ();
					else
#endif
						{
						if (keypress == KEY_ESC)
							rval = 1;
						flashing_cursor = 0;
						WIN (wpage_done = 0);
						goto done;
						}
					}
				else
					goto redrawPage;
				}
			else if (ch == 'P') {		//	New page.
				if (!GotZ) {
					Int3 (); // Hey ryan!!!!You gotta load a screen before you start
					        // printing to it!You know, $Z !!!
					//if (message > pj)
		  			LoadNewBriefingScreen (gameStates.menus.bHires?"end01b.pcx":"end01.pcx", message <= pj);
					bHaveScreen = 1;
					}
				new_page = 1;
				while (*message != 10)
					message++;	//	drop carriage return after special escape sequence
				message++;
				prev_ch = 10;
				//GrUpdate (0);
				}
			}
		else if (ch == '\t') {		//	Tab
			if (Briefing_text_x - bsp->text_ulx < tab_stop)
				Briefing_text_x = bsp->text_ulx + tab_stop;
			}
		else if ((ch == ';') && (prev_ch == 10)) {
			while (*message++ != 10)
				;
			prev_ch = 10;
			}
		else if (ch == '\\') {
			prev_ch = ch;
			}
		else if (ch == 10) {
			if (prev_ch != '\\') {
				prev_ch = ch;
				if (bDumbAdjust == 0)
					Briefing_text_y += (8* (gameStates.menus.bHires+1));
				else
					bDumbAdjust--;
				Briefing_text_x = bsp->text_ulx;
				if (Briefing_text_y > bsp->text_uly + bsp->text_height) {
					LoadBriefingScreen (nScreen);
					Briefing_text_x = bsp->text_ulx;
					Briefing_text_y = bsp->text_uly;
					}
				}
			else {
				if (*message == 13)		//Can this happen? Above says ch==10
					message++;
				prev_ch = ch;
				}
			}
		else {
			if (!GotZ) {
				Int3 (); // Hey ryan!!!!You gotta load a screen before you start
				        // printing to it!You know, $Z !!!
				//if (message > pj)
					LoadNewBriefingScreen (gameStates.menus.bHires?"end01b.pcx":"end01.pcx", message <= pj);
				}
			prev_ch = ch;
			bRedraw = !delayCount || (message <= pj);
			if (!bRedraw) {
		 		nPrintingChannel = StartBriefingSound (nPrintingChannel, SOUND_BRIEFING_PRINTING, F1_0, NULL);
				chattering = 1;
				}
			Briefing_text_x += PrintCharDelayed ((char) ch, delayCount, nRobot, flashing_cursor, bRedraw);
			}

		//	Check for Esc -> abort.
		if (!bRedraw) {
			key_check = delayCount ? local_key_inkey () : 0;
			if (key_check == KEY_ESC) {
				rval = 1;
				done = 1;
				goto done;
				}
			if ((key_check == KEY_SPACEBAR) || (key_check == KEY_ENTER)) {
				StopBriefingSound (&nBotChannel);
				delayCount = 0;
				bRedraw = 1;
				}
			if ((key_check == KEY_ALTED+KEY_ENTER) ||
				 (key_check == KEY_ALTED+KEY_PADENTER))
				GrToggleFullScreen ();
			}
		if (Briefing_text_x > bsp->text_ulx + bsp->text_width) {
			Briefing_text_x = bsp->text_ulx;
			Briefing_text_y += bsp->text_uly;
		}

		if (new_page || (Briefing_text_y > bsp->text_uly + bsp->text_height)) {
			fix	StartTime = 0;
			int	keypress = 0;

			new_page = 0;
			StopBriefingSound (&nPrintingChannel);
			chattering=0;
			{
				StartTime = TimerGetFixedSeconds ();
				do {		//	Wait for a key
					while (TimerGetFixedSeconds () < StartTime + KEY_DELAY_DEFAULT/2)
						;
					flash_cursor (flashing_cursor);
					if (bRobotPlaying)
						RotateBriefingRobot ();
					if (nRobot != -1)
						ShowSpinningRobotFrame (nRobot);
					if (Bitmap_name [0] != 0)
						ShowBitmapFrame (0);
					StartTime += KEY_DELAY_DEFAULT / 2;
					keypress = local_key_inkey ();
					if (curDrawBuffer == GL_BACK)
						break;
 					GrUpdate (0);
					} while (!keypress);
				if (keypress) {
					if (bRobotPlaying)
						DeInitRobotMovie ();
					bRobotPlaying=0;
					nRobot = -1;
#ifdef _DEBUG
					if (keypress == KEY_BACKSP)
						Int3 ();
#endif
					if (keypress == KEY_ESC) {
						rval = 1;
						goto done;
						}
					else {
						if (bOnlyRobots) {
							*szBriefScreen = *szBriefScreenB = '\0';
							while (message && !PageHasRobot (message))
								message = SkipPage (message, &briefBuf, &x, &y, &nScreen);
							if (!message)
								goto done;
							}
 						pi = message;
						if (curDrawBuffer == GL_FRONT) {
							LoadBriefingScreen (nScreen);
							GrUpdate (0);
							}
						Briefing_text_x = bsp->text_ulx;
						Briefing_text_y = bsp->text_uly;
						delayCount = KEY_DELAY_DEFAULT;
						WIN (wpage_done = 0);
						goto redrawPage;
						}
					}
				else
					goto redrawPage;
				}
		}
	}
}

done:

if (bRobotPlaying) {
	DeInitRobotMovie ();
	bRobotPlaying = 0;
	}
if (robotCanv)
	D2_FREE (robotCanv);
StopBriefingSound (&nHumChannel);
StopBriefingSound (&nPrintingChannel);
StopBriefingSound (&nBotChannel);
return rval;
}

//-----------------------------------------------------------------------------
// Return a pointer to the start of text for screen #nScreen.
char * GetBriefingMessage (int nScreen)
{
	char *tptr = szBriefingText;
	int	nCurScreen=0;
	int	ch, i;

Assert (nScreen >= 0);

for (i = 0; *tptr && (i < briefingTextLen); i++) {
	ch = *tptr++;
	if (ch != '$')
		continue;
	ch = *tptr++;
	if (ch != 'S')
		continue;
	nCurScreen = GetMessageNum (&tptr);
	if (nCurScreen == nScreen)
		return tptr;
	}
return (NULL);
}

//-----------------------------------------------------------------------------
//	Load Descent briefing text.
int LoadScreenText (char *filename, char **buf)
{
	CFILE *fp;
	int	len, i;
	int	bHaveBinary;
	char	*bufP;

if (!strstr (filename, ".t"))
	strcat (filename, ".tex");
bHaveBinary = (strstr (filename, ".txb") != NULL);
if (!(fp = CFOpen (filename, gameFolders.szDataDir, bHaveBinary ? "rb" : "rt", gameStates.app.bD1Mission))) {
	bHaveBinary = !bHaveBinary;
	strcpy (strstr (filename, ".t"), bHaveBinary ? ".txb" : ".tex");
	if (!(fp = CFOpen (filename, gameFolders.szDataDir, bHaveBinary ? "rb" : "rt", gameStates.app.bD1Mission))) {
		LogErr ("can't open briefing '%s'!\n", filename);
		return (0);
		}
	}
if (bHaveBinary) {
	len = CFLength (fp, 0);
	MALLOC (bufP, char, len);
	*buf = bufP;
	for (i = 0; i < len; i++, bufP++) {
		CFRead (bufP, 1, 1, fp);
			if (*bufP == 13)
				bufP--;
		}
	CFClose (fp);
	for (i = 0, bufP = *buf; i < len; i++, bufP++)
		if (*bufP != '\n')
			*bufP = EncodeRotateLeft ((char) (EncodeRotateLeft (*bufP) ^ BITMAP_TBL_XOR));
	}
else {
	len = CFLength (fp, 0);
	MALLOC (bufP, char, len+500);
	*buf = bufP;
	for (i = 0; i < len; i++, bufP++) {
		CFRead (bufP, 1, 1, fp);
			if (*bufP == 13)
				bufP--;
		}
	*bufP = 0;
	CFClose (fp);
	}
briefingTextLen = (int) (bufP - *buf);
return (1);
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int ShowBriefingText (int nScreen, short nLevel)
{
	char	*pszMsg;
	int i;

pszMsg = GetBriefingMessage (gameStates.app.bD1Mission ? briefingScreens [nScreen].nMessage : nScreen);
if (!pszMsg)
	return (0);
DoBriefingColorStuff ();
glEnable (GL_TEXTURE_2D);
i = ShowBriefingMessage (nScreen, pszMsg, nLevel);
glDisable (GL_TEXTURE_2D);
return i;
}

//-----------------------------------------------------------------------------

void DoBriefingColorStuff ()
{
GrPaletteStepLoad (NULL);

if (gameStates.app.bD1Mission) {
  //green
	briefFgColorIndex [0] = GrFindClosestColorCurrent (0, 54, 0);
	briefBgColorIndex [0] = GrFindClosestColorCurrent (0, 19, 0);
  //white
	briefFgColorIndex [1] = GrFindClosestColorCurrent (42, 38, 32);
	briefBgColorIndex [1] = GrFindClosestColorCurrent (14, 14, 14);
	//Begin D1X addition
	//red
	briefFgColorIndex [2] = GrFindClosestColorCurrent (63, 0, 0);
	briefBgColorIndex [2] = GrFindClosestColorCurrent (31, 0, 0);
	}
else {
	briefFgColorIndex [0] = GrFindClosestColorCurrent (0, 40, 0);
	briefBgColorIndex [0] = GrFindClosestColorCurrent (0, 6, 0);
	briefFgColorIndex [1] = GrFindClosestColorCurrent (40, 33, 35);
	briefBgColorIndex [1] = GrFindClosestColorCurrent (5, 5, 5);
	briefFgColorIndex [2] = GrFindClosestColorCurrent (8, 31, 54);
	briefBgColorIndex [2] = GrFindClosestColorCurrent (1, 4, 7);
	}
//blue
briefFgColorIndex [3] = GrFindClosestColorCurrent (0, 0, 54);
briefBgColorIndex [3] = GrFindClosestColorCurrent (0, 0, 19);
//gray
briefFgColorIndex [4] = GrFindClosestColorCurrent (14, 14, 14);
briefBgColorIndex [4] = GrFindClosestColorCurrent (0, 0, 0);
//yellow
briefFgColorIndex [5] = GrFindClosestColorCurrent (54, 54, 0);
briefBgColorIndex [5] = GrFindClosestColorCurrent (19, 19, 0);
//purple
briefFgColorIndex [6] = GrFindClosestColorCurrent (0, 54, 54);
briefBgColorIndex [6] = GrFindClosestColorCurrent (0, 19, 19);
//End D1X addition
}

//-----------------------------------------------------------------------------
// Return true if screen got aborted by user, else return false.
int ShowBriefingScreen (int nScreen, int bAllowKeys, short nLevel)
{
brief_palette_254_bash = 0;
if (gameOpts->gameplay.bSkipBriefingScreens) {
	con_printf (CONDBG, "Skipping briefing screen [%s]\n", &briefingScreens [nScreen].bs_name);
	return 0;
	}
if (gameStates.app.bD1Mission) {
	int pcxResult;
#if 1
	grsBitmap bmBriefing;

	GrInitBitmapData (&bmBriefing);
	if ((pcxResult = LoadBriefImg (briefingScreens [nScreen].bs_name, &bmBriefing, 0)) != PCX_ERROR_NONE) {
#else
	if ((pcxResult = PcxReadFullScrImage (briefingScreens [nScreen].bs_name, 1)) != PCX_ERROR_NONE) {
#endif
		con_printf (CONDBG, "File '%s', PCX load error: %s (%i)\n  (It's a briefing screen.  Does this cause you pain?)\n", briefingScreens [nScreen].bs_name, pcx_errormsg (pcxResult), pcxResult);
		Int3 ();
		return 0;
		}
	GrPaletteStepLoad (bmBriefing.bmPalette);
	GrSetCurrentCanvas (NULL);
	ShowFullscreenImage (&bmBriefing);
	GrUpdate (0);
	GrFreeBitmapData (&bmBriefing);
	if (GrPaletteFadeIn (NULL, 32, bAllowKeys))
		return 1;
	}
if (!ShowBriefingText (nScreen, nLevel))
	return 0;
if (GrPaletteFadeOut (NULL, 32, bAllowKeys))
	return 1;
return 1;
}

//-----------------------------------------------------------------------------

void DoBriefingScreens (char *filename, int nLevel)
{
	int	bAbortBriefing = 0;
	int	nCurBriefingScreen = 0;
	int	bEnding = strstr (filename, "endreg") || !stricmp (filename, gameData.missions.szEndingFilename);
	char	fnBriefing [FILENAME_LEN];

LogErr ("Creating cameras\n");
gameStates.render.bBriefing = 1;
RebuildRenderContext (1, 1);
if (gameOpts->gameplay.bSkipBriefingScreens) {
	con_printf (CONDBG, "Skipping all briefing screens.\n");
	gameStates.render.bBriefing = 0;
	return;
	}
strcpy (fnBriefing, *gameData.missions.szBriefingFilename ? gameData.missions.szBriefingFilename : filename);
con_printf (CONDBG, "Trying briefing screen <%s>\n", fnBriefing);
LogErr ("Looking for briefing screen '%s'\n", fnBriefing);
if (!fnBriefing) {
	gameStates.render.bBriefing = 0;
	return;
	}
if (gameStates.app.bD1Mission && (gameData.missions.nCurrentMission != gameData.missions.nD1BuiltinMission)) {
	FILE	*fp;
	int	i;
	char	*psz;

	for (i = 0; i < 2; i++) {
		if ((psz = strchr (fnBriefing, '.')))
			*psz = '\0';
		strcat (fnBriefing, i ? ".txb" : ".tex");
		if	 ((fp = CFFindHogFile (&gameHogFiles.AltHogFiles, "", fnBriefing, NULL))) {
			fclose (fp);
			break;
			}
		}
	if (i == 2) {
		gameStates.render.bBriefing = 0;
		return;
		}
	}
if (!LoadScreenText (fnBriefing, &szBriefingText)) {
	gameStates.render.bBriefing = 0;
	return;
	}
DigiStopAllChannels ();
SongsPlaySong (SONG_BRIEFING, 1);
SetScreenMode (SCREEN_MENU);
GrSetCurrentCanvas (NULL);
con_printf (CONDBG, "Playing briefing screen <%s>, level %d\n", fnBriefing, nLevel);
KeyFlush ();
if (gameStates.app.bD1Mission) {
	gamePalette = LoadPalette (NULL, NULL, 1, 1, 1);
	LoadD1BitmapReplacements ();
	if (nLevel == 1) {
		while (!bAbortBriefing && (briefingScreens [nCurBriefingScreen].nLevel == ((gameStates.app.bD1Mission && bEnding) ? nLevel : 0))) {
			bAbortBriefing = ShowBriefingScreen (nCurBriefingScreen, 0, (short) nLevel);
			nCurBriefingScreen++;
			if (gameStates.app.bD1Mission && bEnding)
				break;
			}
		}
	if (!bAbortBriefing) {
		for (nCurBriefingScreen = 0; nCurBriefingScreen < MAX_BRIEFING_SCREENS; nCurBriefingScreen++)
			if (briefingScreens [nCurBriefingScreen].nLevel == nLevel)
				if (ShowBriefingScreen (nCurBriefingScreen, 0, (short) nLevel))
					break;
		}
	}
else
	ShowBriefingScreen (nLevel, 0, (short) nLevel);
gameStates.render.bBriefing = 0;
D2_FREE (szBriefingText);
szBriefingText = NULL;
KeyFlush ();
return;
}

//-----------------------------------------------------------------------------

int DefineBriefingBox (char **buf, briefing_screen *pBriefBuf)
{
	int n, i=0;
	char name [20];

	n=get_new_message_num (buf);

	Assert (n < MAX_BRIEFING_SCREENS);

	while (**buf!=' ') {
		name [i++]=**buf;
		 (*buf)++;
		}
	name [i]='\0';   // slap a delimiter on this guy
	*pBriefBuf = briefingScreens [n];
	strcpy (pBriefBuf->bs_name, name);
	pBriefBuf->nLevel=get_new_message_num (buf);
	pBriefBuf->nMessage=get_new_message_num (buf);
	pBriefBuf->text_ulx=get_new_message_num (buf);
	pBriefBuf->text_uly=get_new_message_num (buf);
	pBriefBuf->text_width=get_new_message_num (buf);
	pBriefBuf->text_height=GetMessageNum (buf);  // NOTICE!!!
	init_char_pos (pBriefBuf, 1);
	return (n);
}

//-----------------------------------------------------------------------------

int get_new_message_num (char **message)
{
	int	num=0;

while (**message == ' ')
	 (*message)++;
while ((**message >= '0') && (**message <= '9')) {
	num = 10*num + **message-'0';
	 (*message)++;
	}
 (*message)++;
return num;
}

//-----------------------------------------------------------------------------

