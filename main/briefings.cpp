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
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "joy.h"
#include "hogfile.h"
#include "mono.h"
#include "gamefont.h"
#include "error.h"
#include "textures.h"
#include "screens.h"
#include "compbit.h"
#include "text.h"
#include "newmenu.h"
#include "movie.h"
#include "mouse.h"
#include "gamepal.h"
#include "strutil.h"
#include "interp.h"
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
} tBriefingScreen;


void DoBriefingColorStuff ();
int GetNewMessageNum (char **message);
int DefineBriefingBox (char **buf, tBriefingScreen *pBriefBuf);

extern unsigned RobSX, RobSY, RobDX, RobDY; // Robot movie coords

//static ubyte brief_palette [256*3];
//static ubyte robot_palette [256*3];
static int	brief_palette_254_bash;
static int  bInitRobot;

char	curBriefScreenName [15] = "";
char	*szBriefingText = NULL;
int	nBriefingTextLen;
int	nTabStop = 0;
char	bRobotPlaying = 0;

//Begin D1X modification
#define MAX_BRIEFING_COLORS     8
//End D1X modification

// Descent 1 briefings
#define	SHAREWARE_ENDING_FILENAME	"ending.tex"

//	Can be set by -noscreens command line option.  Causes bypassing of all briefing screens.
int	briefFgColorIndex [MAX_BRIEFING_COLORS], 
		briefBgColorIndex [MAX_BRIEFING_COLORS];
int	nCurrentColor = 0;
uint nEraseColor = BLACK_RGBA;

tRgbaColorb briefFgColors [2][MAX_BRIEFING_COLORS] = {
	{{0, 160, 0, 255}, {160, 132, 140, 255}, {32, 124, 216, 255}, {0, 0, 216, 255}, {56, 56, 56, 255}, {216, 216, 0, 255}, {0, 216, 216, 255}, {255, 255, 255, 255}}, 
	{{0, 216, 0, 255}, {168, 152, 128, 255}, {252, 0, 0, 255}, {0, 0, 216, 255}, {56, 56, 56, 255}, {216, 216, 0, 255}, {0, 216, 216, 255}, {255, 255, 255, 255}}
	};

tRgbaColorb briefBgColors [2][MAX_BRIEFING_COLORS] = {
	{{0, 24, 255}, {20, 20, 20, 255}, {4, 16, 28, 255}, {0, 0, 76, 255}, {0, 0, 0, 255}, {76, 76, 0, 255}, {0, 76, 76, 255}, {255, 255, 255, 255}}, 
	{{0, 76, 0, 255}, {56, 56, 56, 255}, {124, 0, 0, 255}, {0, 0, 76, 255}, {0, 0, 0, 255}, {76, 76, 0, 255}, {0, 76, 76, 255}, {255, 255, 255, 255}}
	};

tRgbaColorb eraseColorRgb = {0, 0, 0, 255};

typedef struct tD1ExtraBotSound {
	const char	*pszName;
	short nLevel;
	short	nBotSig;
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

#define BRIEFING_SECRET_NUM 31          //  This must correspond to the first secret level which must come at the end of the list.
#define BRIEFING_OFFSET_NUM 4           // This must correspond to the first level screen (ie, past the bald guy briefing screens)

#define	SHAREWARE_ENDING_LEVEL_NUM  0x7f
#define	REGISTERED_ENDING_LEVEL_NUM 0x7e

#define ENDING_LEVEL_NUM 	REGISTERED_ENDING_LEVEL_NUM

tBriefingScreen briefingScreens [] = {
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
	{ "end03.pcx",   REGISTERED_ENDING_LEVEL_NUM,  3,  5, 5, 300, 200 }    // registered end

};

#define MAX_BRIEFING_SCREENS (sizeofa (briefingScreens))

int	briefingTextX, briefingTextY;

CCanvas*		robotCanvP = NULL;
CAngleVector	vRobotAngles;

char    szBitmapName [32] = "";
#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      //  Adam: This is the nFrameber of frames in your new animating thing.
#define DOOR_DIV_INIT   6
sbyte   nDoorDir = 1, nDoorDivCount = 0, nAnimatingBitmapType = 0;

//-----------------------------------------------------------------------------

typedef struct tBriefingInfo {
	char					*message;
	int					nScreen;
	int					nLevel;
	tBriefingScreen	briefBuf;
	tBriefingScreen	*bsP;
	char					*pi;
	char					*pj;
	int					ch;
	int					prevCh;
	int					bPageDone;
	int					bRedraw;
	int					bKeyCheck;
	int					bFlashingCursor;
	int					bNewPage;
	int					bHaveScreen;
	int					bDumbAdjust;
	int					bChattering;
	int					bGotZ;
	int					bOnlyRobots;
	int					bExtraSounds;
	int					nHumChannel;
	int					nPrintingChannel;
	int					nBotChannel;
	int					nLineAdjustment;
	int					nDelayCount;
	int					nRobot;
	int					nBotSig;
	int					x;
	int					y;
	char					szSpinningRobot [8];
	char					szBriefScreen [15];
	char					szBriefScreenB [15];
	time_t				t0;
	int					nFuncRes;
} tBriefingInfo;

//-----------------------------------------------------------------------------

int StartBriefingSound (int c, short nSound, fix nVolume, const char *pszWAV)
{
if (c < 0) {
	int b = gameStates.sound.bD1Sound;
	gameStates.sound.bD1Sound = 0;
	c = DigiStartSound (DigiXlatSound (nSound), nVolume, 0xFFFF / 2, 1, -1, -1, -1, F1_0, pszWAV, NULL, 0);
	gameStates.sound.bD1Sound = b;
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

tD1ExtraBotSound *FindExtraBotSound (short nLevel, short nBotSig)
{
	tD1ExtraBotSound	*p = extraBotSounds;
	int					i = sizeof (extraBotSounds) / sizeof (tD1ExtraBotSound);

if (!gameStates.app.bD1Mission)
	return NULL;
for (; i; i--, p++)
	if ((p->nLevel == nLevel) && (p->nBotSig == nBotSig))
		return p;
return NULL;
}

//-----------------------------------------------------------------------------

int StartExtraBotSound (int nChannel, short nLevel, short nBotSig)
{
	tD1ExtraBotSound	*p;

StopBriefingSound (&nChannel);
if (!gameStates.app.bHaveExtraData)
	return -1;
if (!(p = FindExtraBotSound (nLevel, nBotSig)))
	return -1;
return StartBriefingSound (nChannel, -1, 8 * F1_0, p->pszName);
}

//-----------------------------------------------------------------------------

extern int NMCheckButtonPress ();

// added by Jan Bobrowski for variable-size menu screen
static int RescaleX (int x)
{
	return x * CCanvas::Current ()->Width () / 320;
}

//-----------------------------------------------------------------------------

static inline int RescaleY (int y)
{
return y * CCanvas::Current ()->Height () / 200;
}

//-----------------------------------------------------------------------------

int BriefingInKey (void)
{
	int	funcRes = KeyInKey ();

if (funcRes == KEY_PRINT_SCREEN) {
	SaveScreenShot (NULL, 0);
	return 0;				//say no key pressed
	}
if (NMCheckButtonPress ())		//joystick or mouse button pressed?
	return KEY_SPACEBAR;
if (MouseButtonState (0))
	return KEY_SPACEBAR;
return funcRes;
}

//-----------------------------------------------------------------------------

int LoadBriefImg (char *pszImg, CBitmap *bmP, int bFullScr)
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
	CBitmap bm;
	if (!ReadTGA (szImg, gameFolders.szDataDir, &bm, -1, 1.0, 0, 0))
		return PCX_ERROR_OPENING;
	if (bFullScr) {
		bm.RenderFullScreen ();
		bm.Destroy ();
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
	CBitmap title_bm;
	char new_filename [FILENAME_LEN+1] = "";

#if !DBG
if (from_hog_only)
	strcpy (new_filename, "\x01");	//only read from hog file
#endif

strcat (new_filename, filename);
filename = new_filename;

title_bm.SetBuffer (NULL);
if ((pcxResult = LoadBriefImg (filename, &title_bm, 0)) != PCX_ERROR_NONE) {
#if TRACE
	console.printf (CON_DBG, "File '%s', PCX load error: %s (%i)\n  (No big deal, just no title screen.)\n", filename, pcx_errormsg (pcxResult), pcxResult);
#endif
	Error ("Error loading briefing screen <%s>, PCX load error: %s (%i)\n", filename, pcx_errormsg (pcxResult), pcxResult);
}
//vfx_set_palette_sub (brief_palette);
paletteManager.LoadEffect  ();
CCanvas::SetCurrent (NULL);
title_bm.RenderFullScreen ();
if (paletteManager.EnableEffect ())
	return 1;

paletteManager.LoadEffect  ();
timer	= TimerGetFixedSeconds () + I2X (3);
while (1) {
	if (BriefingInKey () && bAllowKeys) break;
	if (TimerGetFixedSeconds () > timer) break;
}
if (paletteManager.DisableEffect ())
	return 1;
title_bm.DestroyBuffer ();
return 0;
}

//-----------------------------------------------------------------------------

void InitCharPos (tBriefingScreen *bsP, int bRescale)
{
if (bRescale) {
	bsP->text_ulx = RescaleX (bsP->text_ulx);
	bsP->text_uly = RescaleY (bsP->text_uly);
	bsP->text_width = RescaleX (bsP->text_width);
	bsP->text_height = RescaleY (bsP->text_height);
	}
briefingTextX = bsP->text_ulx;
briefingTextY = gameStates.app.bD1Mission ? bsP->text_uly : bsP->text_uly - (8 *(1 + gameStates.menus.bHires));
}

//-----------------------------------------------------------------------------

void ShowBitmapFrame (int bRedraw)
{
	CCanvas *curCanvSave, *bitmapCanv=0;
	CFixVector p = CFixVector::ZERO;

	CBitmap *bmP;
	int x = RescaleX (138);
	int y = RescaleY (55);
	int w = RescaleX (166);
	int h = RescaleY (138);

	//	Only plot every nth frame.
if (!bRedraw && nDoorDivCount) {
	nDoorDivCount--;
	if (!gameOpts->menus.nStyle)
		return;
	}

if (*szBitmapName) {
	char		*poundSignP;
	int		nFrame, dig1, dig2;
	//	Set supertransparency color to black
	switch (nAnimatingBitmapType) {
		case 0: 
			bitmapCanv = CCanvas::Current ()->CreatePane (x, y, w, h);
			break;
		case 1:
			bitmapCanv = CCanvas::Current ()->CreatePane (x, y, w, h);
			break;
		// Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
		default:
			Int3 (); // Impossible, illegal value for nAnimatingBitmapType
		}
	curCanvSave = CCanvas::Current (); 
	CCanvas::SetCurrent (bitmapCanv);
	CCanvas::Current ()->Clear (0);
	if (!bRedraw) {	//extract current bitmap nFrameber from bitmap name (<name>#<nFrameber>)
		poundSignP = strchr (szBitmapName, '#');
		Assert (poundSignP != NULL);

		dig1 = poundSignP [1];
		dig2 = poundSignP [2];
		if (dig2 == 0)
			nFrame = dig1-'0';
		else
			nFrame = (dig1-'0')*10 + (dig2-'0');

		switch (nAnimatingBitmapType) {
			case 0:
				if (!nDoorDivCount) {
					nFrame += nDoorDir;
					if (nFrame > EXIT_DOOR_MAX) {
						nFrame = EXIT_DOOR_MAX;
						nDoorDir = -1;
						}
					else if (nFrame < 0) {
						nFrame = 0;
						nDoorDir = 1;
					}
				}
				break;
			case 1:
				if (!nDoorDivCount)
					nFrame++;
				if (nFrame > OTHER_THING_MAX)
					nFrame = 0;
				break;
			}

		Assert (nFrame < 100);
		if (nFrame >= 10) {
			poundSignP [1] = (nFrame / 10) + '0';
			poundSignP [2] = (nFrame % 10) + '0';
			poundSignP [3] = 0;
			} 
		else {
			poundSignP [1] = (nFrame % 10) + '0';
			poundSignP [2] = 0;
			}
		}

	paletteManager.Load ("", "", 0, 0, 1);
	tBitmapIndex bmi = PiggyFindBitmap (szBitmapName, gameStates.app.bD1Mission);
	if (bmi.index < 0)
		return;
	bmP = gameData.pig.tex.bitmaps [gameStates.app.bD1Mission] + bmi.index;
	PIGGY_PAGE_IN (bmi.index, gameStates.app.bD1Mission);

	GLint	depthFunc;
	G3StartFrame (1, 0);
	G3SetViewMatrix (p, CFixMatrix::IDENTITY, gameStates.render.xZoom, 1);
	p[Z] = 2 * w * F1_0;
	glGetIntegerv (GL_DEPTH_FUNC, &depthFunc);
	glDepthFunc (GL_ALWAYS);
	G3DrawBitmap (p, w * F1_0, h * F1_0, bmP, NULL, 1.0, 3);
	glDepthFunc (depthFunc);
	G3EndFrame ();
	if (!gameOpts->menus.nStyle)
		GrUpdate (0);
	paletteManager.LoadEffect  ();
	CCanvas::SetCurrent (curCanvSave);
	delete bitmapCanv;
	bitmapCanv = NULL;
	if (!(bRedraw || nDoorDivCount)) {
#if 1
	nDoorDivCount = DOOR_DIV_INIT;
#else
	switch (nAnimatingBitmapType) {
		case 0:
			if (nFrame == EXIT_DOOR_MAX) {
				nDoorDir = -1;
				nDoorDivCount = DOOR_DIV_INIT;
				} 
			else if (nFrame == 0) {
				nDoorDir = 1;
				nDoorDivCount = DOOR_DIV_INIT;
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

void ShowBriefingBitmap (CBitmap *bmp)
{
	CCanvas	*bitmapCanv;

bitmapCanv = CCanvas::Current ()->CreatePane (220, 45, bmp->Width (), bmp->Height ());
CCanvas::Push ();
CCanvas::SetCurrent (bitmapCanv);
GrBitmapM (0, 0, bmp, 0);
CCanvas::Pop ();
bitmapCanv->Destroy ();
}

//-----------------------------------------------------------------------------

void ShowSpinningRobotFrame (int nRobot)
{
if (nRobot != -1) {
	vRobotAngles [HA] += 150;

	CCanvas::Push ();
	CCanvas::SetCurrent (robotCanvP);
	Assert (ROBOTINFO (nRobot).nModel != -1);
	if (bInitRobot) {
		paletteManager.Load ("", "", 0, 0, 1);
		OglCachePolyModelTextures (ROBOTINFO (nRobot).nModel);
		paletteManager.LoadEffect ();
		bInitRobot = 0;
		}
	gameStates.render.bFullBright	= 1;

	CCanvas::Current ()->Clear (0);
	gameStates.ogl.bEnableScissor = 1;
	DrawModelPicture (ROBOTINFO (nRobot).nModel, &vRobotAngles);
	gameStates.ogl.bEnableScissor = 0;
	gameStates.render.bFullBright = 0;
	CCanvas::Pop ();
	}
}

//-----------------------------------------------------------------------------

void RotateBriefingRobot (void)
{
CCanvas::Push ();
CCanvas::SetCurrent (robotCanvP);
RotateRobot ();
CCanvas::Pop ();
}

//-----------------------------------------------------------------------------

void InitSpinningRobot (void) // (int x, int y, int w, int h)
{
	//vRobotAngles[PA] += 0;
	//vRobotAngles[BA] += 0;
	//vRobotAngles[HA] += 0;

int x = RescaleX (138);
int y = RescaleY (55);
int w = RescaleX (163);
int h = RescaleY (136);
if (robotCanvP)
	delete robotCanvP;
robotCanvP = CCanvas::Current ()->CreatePane (x, y, w, h);
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
	static fix	t, tText = 0, tImage = 0;

	message [0] = the_char;
	message [1] = 0;

if (!tText)
	tText = SDL_GetTicks ();
if ((nRobot < 0) && !*szBitmapName)
	tImage = 0;
else if (!tImage)
	tImage = SDL_GetTicks ();

fontManager.Current ()->StringSize (message, w, h, aw);
Assert ((nCurrentColor >= 0) && (nCurrentColor < MAX_BRIEFING_COLORS));

//	Draw cursor if there is some delay and caller says to draw cursor
if (cursorFlag && !bRedraw) {
	fontManager.SetColorRGB (briefFgColors [gameStates.app.bD1Mission] + nCurrentColor, NULL);
	GrPrintF (NULL, briefingTextX+1, briefingTextY, "_");
	if (!gameOpts->menus.nStyle)
		GrUpdate (0);
}

if (delay > 0)
	delay = 1000 / 15;

if ((delay > 0) && !bRedraw) {
	if (*szBitmapName)
		ShowBitmapFrame (0);
	do {
		if (bRobotPlaying)
			RotateBriefingRobot ();
		else {
			if (tImage && (t - tImage >= 10)) {
				if (nRobot != -1)
					ShowSpinningRobotFrame (nRobot);
				else if (*szBitmapName && (delay != 0))
					ShowBitmapFrame (0);
				tImage = t;
				}
			}
		} while ((t = SDL_GetTicks ()) < (tText + delay));
	}

tText = SDL_GetTicks ();

//	Erase cursor
if (cursorFlag && (delay > 0) && !bRedraw) {
	fontManager.SetColorRGBi (nEraseColor, 1, 0, 0);
	GrPrintF (NULL, briefingTextX+1, briefingTextY, "_");
	//	erase the character
	fontManager.SetColorRGB (briefBgColors [gameStates.app.bD1Mission] + nCurrentColor, NULL);
	GrPrintF (NULL, briefingTextX, briefingTextY, message);
}
//draw the character
fontManager.SetColorRGB (briefFgColors [gameStates.app.bD1Mission] + nCurrentColor, NULL);
GrPrintF (NULL, briefingTextX+1, briefingTextY, message);

if (!(bRedraw || gameOpts->menus.nStyle)) 
	GrUpdate (0);
return w;
}

//-----------------------------------------------------------------------------

int LoadBriefingScreen (int nScreen)
{
	int	pcxResult;
	char *szBriefScreen;

if (gameStates.app.bD1Mission)
	szBriefScreen = briefingScreens [nScreen % MAX_BRIEFING_SCREENS].bs_name;
else
	szBriefScreen = curBriefScreenName;
if (*szBriefScreen) {
	glClear (GL_COLOR_BUFFER_BIT);
	if ((pcxResult = PcxReadFullScrImage (szBriefScreen, gameStates.app.bD1Mission)) != PCX_ERROR_NONE) {
#if DBG
		Error ("Error loading briefing screen <%s>, \nPCX load error: %s (%i)\n", szBriefScreen, pcx_errormsg (pcxResult), pcxResult);
#endif
		}
	}
return 0;
}

//-----------------------------------------------------------------------------

int LoadNewBriefingScreen (char *szBriefScreen, int bRedraw)
{
	int pcxResult;

#if TRACE
console.printf (CON_DBG, "Loading new briefing <%s>\n", szBriefScreen);
#endif
strcpy (curBriefScreenName, szBriefScreen);
if (paletteManager.DisableEffect ())
	return 0;
if ((pcxResult = LoadBriefImg (szBriefScreen, NULL, 1)) != PCX_ERROR_NONE) {
#if DBG
	static char szErrScreen [15] = "";

	if (strncmp (szErrScreen, szBriefScreen, sizeof (szErrScreen))) {
		Error ("Error loading briefing screen <%s>, \nPCX load error: %s (%i)\n", 
				 szBriefScreen, pcx_errormsg (pcxResult), pcxResult);
		strncpy (szErrScreen, szBriefScreen, sizeof (szErrScreen));
		}
#endif
	}
if (paletteManager.EnableEffect ())
	return 0;
DoBriefingColorStuff ();
return 1;
}

//-----------------------------------------------------------------------------

int GetMessageNum (char **message)
{
	int	n = 0;
	char	*psz = *message;

while (*psz && (*psz == ' '))
	psz++;
while ((*psz >= '0') && (*psz <= '9')) {
	n = 10 * n + (*psz - '0');
	psz++;
	}
while (*psz && (*psz++ != 10))		//	Get and drop eoln
	;
*message = psz;
return n;
}

//-----------------------------------------------------------------------------

void GetMessageName (char **message, char *result)
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

void FlashCursor (int cursorFlag)
{
if (cursorFlag == 0)
	return;
if ((TimerGetFixedSeconds () % (F1_0/2)) > (F1_0/4))
	fontManager.SetColorRGB (briefFgColors [gameStates.app.bD1Mission] + nCurrentColor, NULL);
else
	fontManager.SetColorRGB (&eraseColorRgb, NULL);
GrPrintF (NULL, briefingTextX+1, briefingTextY, "_");
if (gameStates.ogl.nDrawBuffer == GL_FRONT)
	GrUpdate (0);
}

extern int InitMovieBriefing ();

//-----------------------------------------------------------------------------

const char *NextPage (const char *message)
{
	const char	*pNextPage = strstr (message, "$P");
	const char	*pNextBrief = strstr (message, "$S");

if (pNextPage && pNextBrief)
	return ((pNextPage < pNextBrief) ? pNextPage : pNextBrief);
return (pNextPage ? pNextPage : pNextBrief);
return NULL;
}

//-----------------------------------------------------------------------------

int PageHasRobot (const char *message)
{
	const char	*pEnd = NextPage (message);
	const char	*pBot = strstr (message, "$R");

return pBot && (!pEnd || (pBot < pEnd));
}

//-----------------------------------------------------------------------------

int WaitForKeyPress (tBriefingInfo& bi)
{
	time_t t;
	int keypress = 0;

bi.bChattering = 0;
StopBriefingSound (&bi.nPrintingChannel);
if (!bi.t0)
	bi.t0 = SDL_GetTicks ();
do {		//	Wait for a key
	while ((t = SDL_GetTicks ()) - bi.t0 < 10)
		;
	FlashCursor (bi.bFlashingCursor);
	if (bRobotPlaying)
		RotateBriefingRobot ();
	else if (bi.nRobot != -1)
		ShowSpinningRobotFrame (bi.nRobot);
	else if (*szBitmapName)
		ShowBitmapFrame (0);
	bi.t0 = t;
	keypress = BriefingInKey ();
	if (gameStates.ogl.nDrawBuffer == GL_BACK)
		break;
	GrUpdate (0);
	} while (!keypress);
return keypress;
}

//-----------------------------------------------------------------------------

int _A (tBriefingInfo& bi)
{
bi.nLineAdjustment = 1 - bi.nLineAdjustment;
return 1;
}

//-----------------------------------------------------------------------------

int _B (tBriefingInfo& bi)
{
	char        szBitmap [32];
	CBitmap   guy_bitmap;
	CIFF			iff;
	int         iff_error;

if (bi.message > bi.pj) {
	if (robotCanvP != NULL) {
		delete robotCanvP;
		robotCanvP = NULL;
		}
	}
GetMessageName (&bi.message, szBitmap);
strcat (szBitmap, ".bbm");
memset (&guy_bitmap, 0, sizeof (guy_bitmap));
iff_error = iff.ReadBitmap (szBitmap, &guy_bitmap, BM_LINEAR);
if (iff_error != IFF_NO_ERROR)
	return 0;
ShowBriefingBitmap (&guy_bitmap);
guy_bitmap.DestroyBuffer ();
bi.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int _C (tBriefingInfo& bi)
{
nCurrentColor = GetMessageNum (&bi.message) - 1;
Assert ((nCurrentColor >= 0) && (nCurrentColor < MAX_BRIEFING_COLORS));
bi.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int _D (tBriefingInfo& bi)
{
bi.nScreen = DefineBriefingBox (&bi.message, &bi.briefBuf);
bi.bsP = &bi.briefBuf;
bi.x = briefingTextX;
bi.y = briefingTextY;
bi.nLineAdjustment = 0;
bi.prevCh = 10;                                   // read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int _F (tBriefingInfo& bi)
{
bi.bFlashingCursor = !bi.bFlashingCursor;
bi.prevCh = 10;
while (*bi.message != 10)
	bi.message++;
return 1;
}

//-----------------------------------------------------------------------------

int _N (tBriefingInfo& bi)
{
if (bi.message > bi.pj) {
	if (robotCanvP != NULL) {
		delete robotCanvP;
		robotCanvP = NULL;
		}
	StopBriefingSound (&bi.nBotChannel);
	GetMessageName (&bi.message, szBitmapName);
	strcat (szBitmapName, "#0");
	nAnimatingBitmapType = 0;
	}
bi.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int _O (tBriefingInfo& bi)
{
if (bi.message > bi.pj) {
	if (robotCanvP != NULL) {
		delete robotCanvP;
		robotCanvP = NULL;
		}
	GetMessageName (&bi.message, szBitmapName);
	strcat (szBitmapName, "#0");
	nAnimatingBitmapType = 1;
	}
bi.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int _P (tBriefingInfo& bi)
{
if (!bi.bGotZ) {
	Int3 (); // Hey ryan!!!!You gotta load a screen before you start printing to it!You know, $Z !!!
	LoadNewBriefingScreen (gameStates.menus.bHires ? reinterpret_cast<char*> ("end01b.pcx") : reinterpret_cast<char*> ("end01.pcx"), bi.message <= bi.pj);
	bi.bHaveScreen = 1;
	}
bi.bNewPage = 1;
while (*bi.message != 10)
	bi.message++;	//	drop carriage return after special escape sequence
bi.message++;
bi.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int _R (tBriefingInfo& bi)
{
if (bi.message > bi.pj) {
	if (robotCanvP != NULL) {
		delete robotCanvP;
		robotCanvP = NULL;
		}
	if (bRobotPlaying) {
		DeInitRobotMovie ();
		bRobotPlaying = 0;
		}
	InitSpinningRobot ();
	if (!gameData.songs.bPlaying) {
		bi.nBotChannel = StartExtraBotSound (bi.nBotChannel, (short) bi.nLevel, bi.nBotSig++);
		if (bi.nBotChannel >= 0)
			StopBriefingSound (&bi.nHumChannel);
		}
	}
if (gameStates.app.bD1Mission)
	bi.nRobot = GetMessageNum (&bi.message);
else {
	bi.szSpinningRobot [2] = *bi.message++; // ugly but proud
	if (bi.message > bi.pj) {
		CCanvas::Push ();
		CCanvas::SetCurrent (robotCanvP);
		bRobotPlaying = InitRobotMovie (bi.szSpinningRobot);
		CCanvas::Pop ();
		if (bRobotPlaying) {
			RotateBriefingRobot ();
			DoBriefingColorStuff ();
			}
		}
	}
bi.prevCh = 10;                           // read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int _S (tBriefingInfo& bi)
{
int keypress = WaitForKeyPress (bi);

if (!keypress)
	return -1;
if (keypress == KEY_ESC)
	bi.nFuncRes = 1;
bi.bFlashingCursor = 0;
return 0;
}

//-----------------------------------------------------------------------------

int _T (tBriefingInfo& bi)
{
nTabStop = GetMessageNum (&bi.message) * (1 + gameStates.menus.bHires);
bi.prevCh = 10;							//	read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int _U (tBriefingInfo& bi)
{
bi.nScreen = GetMessageNum (&bi.message);
bi.briefBuf = briefingScreens [bi.nScreen % MAX_BRIEFING_SCREENS];
bi.bsP = &bi.briefBuf;
InitCharPos (bi.bsP, 1);
bi.x = briefingTextX;
bi.y = briefingTextY;
bi.prevCh = 10;                                   // read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int _Z (tBriefingInfo& bi)
{
bi.bGotZ = 1;
bi.bDumbAdjust = 1;
int i = 0;
while ((bi.szBriefScreen [i] = *bi.message) != '\n') {
	i++;
	bi.message++;
	}
bi.szBriefScreen [i] = 0;
while (*bi.message != 10)    //  Get and drop eoln
	bi.message++;
for (i = 0; bi.szBriefScreen [i] != '.'; i++)
	bi.szBriefScreenB [i] = bi.szBriefScreen [i];
memcpy (bi.szBriefScreenB + i, "b.pcx", sizeof ("b.pcx"));
i += sizeof ("b.pcx");
if ((gameStates.menus.bHires && CFile::Exist (bi.szBriefScreenB, gameFolders.szDataDir, 0)) || 
	 !CFile::Exist (bi.szBriefScreen, gameFolders.szDataDir, 0))
	LoadNewBriefingScreen (bi.szBriefScreenB, bi.message <= bi.pj);
else
	LoadNewBriefingScreen (bi.szBriefScreen, bi.message <= bi.pj);
bi.bHaveScreen = 1;
return 1;
}

//-----------------------------------------------------------------------------

int _TAB (tBriefingInfo& bi)
{
if (briefingTextX - bi.bsP->text_ulx < nTabStop)
	briefingTextX = bi.bsP->text_ulx + nTabStop;
return 1;
}

//-----------------------------------------------------------------------------

int _BS (tBriefingInfo& bi)
{
bi.prevCh = bi.ch;
return 1;
}

//-----------------------------------------------------------------------------

int _NEWL (tBriefingInfo& bi)
{
if (bi.prevCh != '\\') {
	bi.prevCh = bi.ch;
	if (bi.bDumbAdjust == 0)
		briefingTextY += (8*(gameStates.menus.bHires+1));
	else
		bi.bDumbAdjust--;
	briefingTextX = bi.bsP->text_ulx;
	if (briefingTextY > bi.bsP->text_uly + bi.bsP->text_height) {
		LoadBriefingScreen (bi.nScreen);
		briefingTextX = bi.bsP->text_ulx;
		briefingTextY = bi.bsP->text_uly;
		}
	}
else {
	if (*bi.message == 13)		//Can this happen? Above says bi.ch==10
		bi.message++;
	bi.prevCh = bi.ch;
	}
return 1;
}

//-----------------------------------------------------------------------------

int _SEMI (tBriefingInfo& bi)
{
while (*bi.message != 10)
	bi.message++;
bi.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int _ANY (tBriefingInfo& bi)
{
if (!bi.bGotZ) 
	LoadNewBriefingScreen (gameStates.menus.bHires ? reinterpret_cast<char*> ("end01b.pcx") : reinterpret_cast<char*> ("end01.pcx"), bi.message <= bi.pj);
bi.prevCh = bi.ch;
bi.bRedraw = !bi.nDelayCount || (bi.message <= bi.pj);
if (!bi.bRedraw) {
	bi.nPrintingChannel = StartBriefingSound (bi.nPrintingChannel, SOUND_BRIEFING_PRINTING, F1_0, NULL);
	bi.bChattering = 1;
	}
briefingTextX += PrintCharDelayed ((char) bi.ch, bi.nDelayCount, bi.nRobot, bi.bFlashingCursor, bi.bRedraw);
return 1;
}

//-----------------------------------------------------------------------------

char *SkipPage (tBriefingInfo &bi) //char *message, tBriefingScreen *pBriefBuf, int *px, int *py, int *pnScreen)
{
	char	ch;
	const char *pEnd = NextPage (bi.message);

while (*bi.message && (!pEnd || (bi.message < pEnd))) {
	ch = *bi.message++;
	if (ch == '$') {
		ch = *bi.message++;
		if (ch == 'D') {
			bi.nScreen = DefineBriefingBox (&bi.message, &bi.briefBuf);
			bi.x = briefingTextX;
			bi.y = briefingTextY;
			}
		else if (ch == 'U') {
			bi.nScreen = GetMessageNum (&bi.message);
			bi.briefBuf = briefingScreens [bi.nScreen % MAX_BRIEFING_SCREENS];
			InitCharPos (&bi.briefBuf, 1);
			bi.x = briefingTextX;
			bi.y = briefingTextY;
			}
		}
	}
if (!bi.message)
	return NULL;
ch = *bi.message;
if (ch != '$')
	return bi.message;
ch = *(++bi.message);
if (ch == 'S')
	return NULL;
do {
	bi.message++;
	} while (::isdigit (*bi.message) || ::isspace (*bi.message));
return bi.message;
}

//-----------------------------------------------------------------------------

int HandleInput (tBriefingInfo& bi)
{
if (!bi.bRedraw && bi.nDelayCount) {
	int keypress = BriefingInKey ();
	if (!keypress)
		return 1;
	if (keypress == KEY_ESC)
		return 0;
	if ((keypress == KEY_SPACEBAR) || (keypress == KEY_ENTER)) {
		StopBriefingSound (&bi.nBotChannel);
		bi.nDelayCount = 0;
		bi.bRedraw = 1;
		}
	if ((keypress == KEY_ALTED + KEY_ENTER) ||
		 (keypress == KEY_ALTED + KEY_PADENTER))
		GrToggleFullScreen ();
	}
return 1;
}

//-----------------------------------------------------------------------------

int HandleNewPage (tBriefingInfo& bi)
{
if (!bi.bNewPage && (briefingTextY <= bi.bsP->text_uly + bi.bsP->text_height))
	return 1;
bi.bNewPage = 0;
int keypress = WaitForKeyPress (bi);
if (!keypress)
	return -1;
if (bRobotPlaying)
	DeInitRobotMovie ();
bRobotPlaying = 0;
bi.nRobot = -1;
if (keypress == KEY_ESC)
	return 0;
if (bi.bOnlyRobots) {
	*bi.szBriefScreen = *bi.szBriefScreenB = '\0';
	while (bi.message && !PageHasRobot (bi.message))
		SkipPage (bi); //.message, &bi.briefBuf, &bi.x, &bi.y, &bi.nScreen);
	if (!bi.message)
		return 0;
	}
bi.pi = bi.message;
if (gameStates.ogl.nDrawBuffer == GL_FRONT) {
	LoadBriefingScreen (bi.nScreen);
	GrUpdate (0);
	}
briefingTextX = bi.bsP->text_ulx;
briefingTextY = bi.bsP->text_uly;
bi.nDelayCount = KEY_DELAY_DEFAULT;
return -1;
}

//-----------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int (__fastcall * pBriefingHandler) (tBriefingInfo& bi);
#else
typedef int (* pBriefingHandler) (tBriefingInfo& bi);
#endif

typedef struct tBriefingHandlerInfo {
	int					ch;
	int					prevCh;
	pBriefingHandler	handlerP;
	} tBriefingHandlerInfo;

static tBriefingHandlerInfo briefingHandlers1 [] = {
	{'A', 0, _A},
	{'B', 0, _B},
	{'C', 0, _C},
	{'D', 0, _D},
	{'F', 0, _F},
	{'N', 0, _N},
	{'O', 0, _O},
	{'P', 0, _P},
	{'R', 0, _R},
	{'S', 0, _S},
	{'T', 0, _T},
	{'U', 0, _U},
	{'Z', 0, _Z}
	};

static tBriefingHandlerInfo briefingHandlers2 [] = {
	{'\t', 0, _TAB},
	{'\\', 0, _BS},
	{10, 0, _NEWL},
	{';', 10, _SEMI},
	{0, 0, _ANY}
	};

//-----------------------------------------------------------------------------
// Return true if message got aborted by user (pressed ESC), else return false.

int ShowBriefingMessage (int nScreen, char *message, int nLevel)
{
	tBriefingInfo			bi = {message, nScreen, nLevel, {"", 0, 0, 0, 0, 0, 0}, NULL, NULL, NULL, 
										-1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 1, 
										KEY_DELAY_DEFAULT, -1, 0, 0, 0, "rba.mve", "", "", 0, 0};
	tBriefingHandlerInfo	*bhP;
	int						h, i;

*szBitmapName = '\0';
nCurrentColor = 0;
bRobotPlaying = 0;

if (gameStates.app.bNostalgia)
	OglSetDrawBuffer (GL_FRONT, 0);
InitMovieBriefing ();

bi.bExtraSounds = gameStates.app.bHaveExtraData && gameStates.app.bD1Mission && 
				  (gameData.missions.nCurrentMission == gameData.missions.nD1BuiltinMission);
bi.bOnlyRobots = gameStates.app.bHaveExtraMovies && bi.bExtraSounds && (bi.nLevel == 1) && (bi.nScreen < 4);
if (!gameData.songs.bPlaying)
	bi.nHumChannel = StartBriefingHum (bi.nHumChannel, bi.nLevel, bi.nScreen, bi.bExtraSounds);

fontManager.SetCurrent (GAME_FONT);

bi.briefBuf = briefingScreens [bi.nScreen % MAX_BRIEFING_SCREENS];
bi.bsP = &bi.briefBuf;
if (gameStates.app.bD1Mission)
	bi.bGotZ = 1;
InitCharPos (bi.bsP, gameStates.app.bD1Mission);

bi.x = briefingTextX;
bi.y = briefingTextY;
*bi.szBriefScreen = *bi.szBriefScreenB = '\0';
if (bi.bOnlyRobots) {
	while (bi.message && !PageHasRobot (bi.message))
		SkipPage (bi); //.message, &bi.briefBuf, &bi.x, &bi.y, &bi.nScreen);
	if (!bi.message)
		goto done;
	}
bi.pi = bi.pj = bi.message;

redrawPage:

for (;;) {
	bi.pj = bi.message;
	GrUpdate (0);
	if (gameStates.ogl.nDrawBuffer == GL_FRONT)
		bi.pi = bi.message;
	else {
		bi.message = bi.pi;
		briefingTextX = bi.x;
		briefingTextY = bi.y;
		LoadBriefingScreen (bi.nScreen);
		bi.briefBuf = briefingScreens [bi.nScreen % MAX_BRIEFING_SCREENS];
		bi.bsP = &bi.briefBuf;
		InitCharPos (bi.bsP, 1);
		}

	bi.bRedraw = 1;
	bi.bNewPage = 0;
	while (bi.bRedraw) {
		bi.ch = *bi.message++;
		if (bi.ch == '$') {
			bi.ch = *bi.message++;
			for (i = sizeofa (briefingHandlers1), bhP = briefingHandlers1; i; i--, bhP++)
				if ((bi.ch == bhP->ch) && (!bhP->prevCh || (bi.prevCh == bhP->prevCh))) {
					h = bhP->handlerP (bi);
					if (h < 0)
						goto redrawPage;
					if (!h)
						goto done;
					break;
					}
			}
		else {
			for (i = sizeofa (briefingHandlers2), bhP = briefingHandlers2; i; i--, bhP++)
				if (!bhP->ch || ((bi.ch == bhP->ch) && (!bhP->prevCh || (bi.prevCh == bhP->prevCh)))) {
					bhP->handlerP (bi);
					break;
					}
			}
		if (!HandleInput (bi)) {
			bi.nFuncRes = 1;
			goto done;
			}

		if (briefingTextX > bi.bsP->text_ulx + bi.bsP->text_width) {
			briefingTextX = bi.bsP->text_ulx;
			briefingTextY += bi.bsP->text_uly;
			}

		h = HandleNewPage (bi);
		if (h < 0)
			goto redrawPage;
		if (!h) {
			bi.nFuncRes = 1;
			goto done;
			}
		}
	}

done:

if (bRobotPlaying) {
	DeInitRobotMovie ();
	bRobotPlaying = 0;
	}
if (robotCanvP != NULL) {
	delete robotCanvP;
	robotCanvP = NULL;
	}
if (!gameData.songs.bPlaying)
	StopBriefingSound (&bi.nHumChannel);
StopBriefingSound (&bi.nPrintingChannel);
StopBriefingSound (&bi.nBotChannel);
return bi.nFuncRes;
}

//-----------------------------------------------------------------------------
// Return a pointer to the start of text for screen #nScreen.
char * GetBriefingMessage (int nScreen)
{
	char *tptr = szBriefingText;
	int	nCurScreen = 0;
	int	ch, i;

Assert (nScreen >= 0);

for (i = 0; *tptr && (i < nBriefingTextLen); i++) {
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
	CFile cf;
	int	len, i;
	int	bHaveBinary;
	char	*bufP;

if (!strstr (filename, ".t"))
	strcat (filename, ".tex");
bHaveBinary = (strstr (filename, ".txb") != NULL);
if (!cf.Open (filename, gameFolders.szDataDir, bHaveBinary ? reinterpret_cast<char*> ("rb") : reinterpret_cast<char*> ("rt"), gameStates.app.bD1Mission)) {
	bHaveBinary = !bHaveBinary;
	strcpy (strstr (filename, ".t"), bHaveBinary ? ".txb" : ".tex");
	if (!cf.Open (filename, gameFolders.szDataDir, bHaveBinary ? reinterpret_cast<char*> ("rb") : reinterpret_cast<char*> ("rt"), gameStates.app.bD1Mission)) {
		PrintLog ("can't open briefing '%s'!\n", filename);
		return (0);
		}
	}
if (bHaveBinary) {
	len = cf.Length ();
	bufP = new char [len];
	*buf = bufP;
	for (i = 0; i < len; i++, bufP++) {
		cf.Read (bufP, 1, 1);
			if (*bufP == 13)
				bufP--;
		}
	cf.Close ();
	for (i = 0, bufP = *buf; i < len; i++, bufP++)
		if (*bufP != '\n')
			*bufP = EncodeRotateLeft ((char) (EncodeRotateLeft (*bufP) ^ BITMAP_TBL_XOR));
	}
else {
	len = cf.Length ();
	bufP = new char [len + 500];
	*buf = bufP;
	for (i = 0; i < len; i++, bufP++) {
		cf.Read (bufP, 1, 1);
			if (*bufP == 13)
				bufP--;
		}
	*bufP = 0;
	cf.Close ();
	}
nBriefingTextLen = (int) (bufP - *buf);
return (1);
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int ShowBriefingText (int nScreen, short nLevel)
{
	char	*pszMsg;
	int i;

pszMsg = GetBriefingMessage (gameStates.app.bD1Mission ? briefingScreens [nScreen % MAX_BRIEFING_SCREENS].nMessage : nScreen);
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
paletteManager.LoadEffect  ();

if (gameStates.app.bD1Mission) {
  //green
	briefFgColorIndex [0] = paletteManager.ClosestColor (0, 54, 0);
	briefBgColorIndex [0] = paletteManager.ClosestColor (0, 19, 0);
  //white
	briefFgColorIndex [1] = paletteManager.ClosestColor (42, 38, 32);
	briefBgColorIndex [1] = paletteManager.ClosestColor (14, 14, 14);
	//Begin D1X addition
	//red
	briefFgColorIndex [2] = paletteManager.ClosestColor (63, 0, 0);
	briefBgColorIndex [2] = paletteManager.ClosestColor (31, 0, 0);
	}
else {
	briefFgColorIndex [0] = paletteManager.ClosestColor (0, 40, 0);
	briefBgColorIndex [0] = paletteManager.ClosestColor (0, 6, 0);
	briefFgColorIndex [1] = paletteManager.ClosestColor (40, 33, 35);
	briefBgColorIndex [1] = paletteManager.ClosestColor (5, 5, 5);
	briefFgColorIndex [2] = paletteManager.ClosestColor (8, 31, 54);
	briefBgColorIndex [2] = paletteManager.ClosestColor (1, 4, 7);
	}
//blue
briefFgColorIndex [3] = paletteManager.ClosestColor (0, 0, 54);
briefBgColorIndex [3] = paletteManager.ClosestColor (0, 0, 19);
//gray
briefFgColorIndex [4] = paletteManager.ClosestColor (14, 14, 14);
briefBgColorIndex [4] = paletteManager.ClosestColor (0, 0, 0);
//yellow
briefFgColorIndex [5] = paletteManager.ClosestColor (54, 54, 0);
briefBgColorIndex [5] = paletteManager.ClosestColor (19, 19, 0);
//purple
briefFgColorIndex [6] = paletteManager.ClosestColor (0, 54, 54);
briefBgColorIndex [6] = paletteManager.ClosestColor (0, 19, 19);
//End D1X addition
}

//-----------------------------------------------------------------------------
// Return true if screen got aborted by user, else return false.
int ShowBriefingScreen (int nScreen, int bAllowKeys, short nLevel)
{
brief_palette_254_bash = 0;
if (gameOpts->gameplay.bSkipBriefingScreens) {
	console.printf (CON_DBG, "Skipping briefing screen [%s]\n", &briefingScreens [nScreen % MAX_BRIEFING_SCREENS].bs_name);
	return 0;
	}
if (gameStates.app.bD1Mission) {
	int pcxResult;
#if 1
	CBitmap bmBriefing;

	bmBriefing.Init ();
	if ((pcxResult = LoadBriefImg (briefingScreens [nScreen % MAX_BRIEFING_SCREENS].bs_name, &bmBriefing, 0)) != PCX_ERROR_NONE) {
#else
	if ((pcxResult = PcxReadFullScrImage (briefingScreens [nScreen % MAX_BRIEFING_SCREENS].bs_name, 1)) != PCX_ERROR_NONE) {
#endif
		console.printf (CON_DBG, "File '%s', PCX load error: %s (%i)\n  (It's a briefing screen.  Does this cause you pain?)\n", 
						briefingScreens [nScreen % MAX_BRIEFING_SCREENS].bs_name, pcx_errormsg (pcxResult), pcxResult);
		Int3 ();
		return 0;
		}
	paletteManager.LoadEffect (bmBriefing.Palette ());
	CCanvas::SetCurrent (NULL);
	bmBriefing.RenderFullScreen ();
	GrUpdate (0);
	bmBriefing.Destroy ();
	if (paletteManager.EnableEffect ())
		return 1;
	}
if (!ShowBriefingText (nScreen, nLevel))
	return 0;
if (paletteManager.DisableEffect ())
	return 1;
return 1;
}

//-----------------------------------------------------------------------------

void DoBriefingScreens (const char *filename, int nLevel)
{
	int	bAbortBriefing = 0;
	int	nCurBriefingScreen = 0;
	int	bEnding = strstr (filename, "endreg") || !stricmp (filename, gameData.missions.szEndingFilename);
	char	fnBriefing [FILENAME_LEN];

PrintLog ("Starting the briefing\n");
gameStates.render.bBriefing = 1;
RebuildRenderContext (1);
if (gameStates.app.bNostalgia)
	OglSetDrawBuffer (GL_FRONT, 0);
if (gameOpts->gameplay.bSkipBriefingScreens) {
	console.printf (CON_DBG, "Skipping all briefing screens.\n");
	gameStates.render.bBriefing = 0;
	return;
	}
strcpy (fnBriefing, *gameData.missions.szBriefingFilename ? gameData.missions.szBriefingFilename : filename);
console.printf (CON_DBG, "Trying briefing screen <%s>\n", fnBriefing);
PrintLog ("Looking for briefing screen '%s'\n", fnBriefing);
if (!*fnBriefing) {
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
		if	 ((fp = hogFileManager.Find (&hogFileManager.AltFiles (), "", fnBriefing, NULL))) {
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
CCanvas::SetCurrent (NULL);
gameStates.render.nShadowPass = 0;
console.printf (CON_DBG, "Playing briefing screen <%s>, level %d\n", fnBriefing, nLevel);
KeyFlush ();
if (gameStates.app.bD1Mission) {
	paletteManager.SetGame (paletteManager.Load (NULL, NULL, 1, 1, 1));
	LoadD1BitmapReplacements ();
	if (nLevel == 1) {
		while (!bAbortBriefing && 
				 (nCurBriefingScreen < MAX_BRIEFING_SCREENS) && 
				 (briefingScreens [nCurBriefingScreen].nLevel == ((gameStates.app.bD1Mission && bEnding) ? nLevel : 0))) {
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
delete [] szBriefingText;
szBriefingText = NULL;
KeyFlush ();
return;
}

//-----------------------------------------------------------------------------

int DefineBriefingBox (char **buf, tBriefingScreen *pBriefBuf)
{
	int i = 0, nScreen = GetNewMessageNum (buf);
	char name [20];

while (**buf != ' ') {
	name [i++] = **buf;
		(*buf)++;
	}
name [i] = '\0';   // slap a delimiter on this guy
*pBriefBuf = briefingScreens [nScreen % MAX_BRIEFING_SCREENS];
strcpy (pBriefBuf->bs_name, name);
pBriefBuf->nLevel = GetNewMessageNum (buf);
pBriefBuf->nMessage = GetNewMessageNum (buf);
pBriefBuf->text_ulx = GetNewMessageNum (buf);
pBriefBuf->text_uly = GetNewMessageNum (buf);
pBriefBuf->text_width = GetNewMessageNum (buf);
pBriefBuf->text_height = GetMessageNum (buf);  // NOTICE!!!
InitCharPos (pBriefBuf, 1);
return nScreen;
}

//-----------------------------------------------------------------------------

int GetNewMessageNum (char **message)
{
	int	nFrame = 0;

while (**message == ' ')
	 (*message)++;
while ((**message >= '0') && (**message <= '9')) {
	nFrame = 10*nFrame + **message-'0';
	 (*message)++;
	}
 (*message)++;
return nFrame;
}

//-----------------------------------------------------------------------------

