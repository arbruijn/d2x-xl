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

#include "descent.h"
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
#include "menu.h"
#include "movie.h"
#include "songs.h"
#include "mouse.h"
#include "gamepal.h"
#include "strutil.h"
#include "interp.h"
#include "ogl_lib.h"
#include "ogl_texcache.h"
#include "ogl_render.h"
#include "renderlib.h"
#include "briefings.h"
#include "menubackground.h"
#include "cockpit.h"
#include "config.h"
#ifdef __macosx__
# include <OpenGL/glu.h>
# undef GL_ARB_multitexture // hack!
#else
# include <GL/glu.h>
#endif

#define KEY_DELAY_DEFAULT       ((I2X (20))/1000)

int32_t NMCheckButtonPress (void);

//Begin D1X modification
#define MAX_BRIEFING_COLORS     9
//End D1X modification

// Descent 1 briefings
#define	SHAREWARE_ENDING_FILENAME	"ending.tex"

//	Can be set by -noscreens command line option.  Causes bypassing of all briefing screens.
int32_t	briefFgColorIndex [MAX_BRIEFING_COLORS], 
		briefBgColorIndex [MAX_BRIEFING_COLORS];

CBriefing briefing;

//-----------------------------------------------------------------------------

CRGBAColor briefFgColors [2][MAX_BRIEFING_COLORS] = {
	{{255, 255, 255, 255}, 
	 {0, 160, 0, 255}, 
	 {160, 132, 140, 255}, 
	 {32, 124, 216, 255}, 
	 {0, 0, 216, 255}, 
	 {56, 56, 56, 255}, 
	 {216, 216, 0, 255}, 
	 {0, 216, 216, 255}, 
	 {255, 255, 255, 255}}, 
	{{255, 255, 255, 255}, 
	 {0, 216, 0, 255}, 
	 {168, 152, 128, 255}, 
	 {252, 0, 0, 255}, 
	 {0, 0, 216, 255}, 
	 {56, 56, 56, 255}, 
	 {216, 216, 0, 255}, 
	 {0, 216, 216, 255}, 
	 {255, 255, 255, 255}}
	};

CRGBAColor briefBgColors [2][MAX_BRIEFING_COLORS] = {
	{{0, 0, 0, 255}, 
	 {0, 24, 255}, 
	 {20, 20, 20, 255}, 
	 {4, 16, 28, 255}, 
	 {0, 0, 76, 255}, 
	 {0, 0, 0, 255}, 
	 {76, 76, 0, 255}, 
	 {0, 76, 76, 255}, 
	 {255, 255, 255, 255}}, 
	{{0, 0, 0, 255}, 
	 {0, 76, 0, 255}, 
	 {56, 56, 56, 255}, 
	 {124, 0, 0, 255}, 
	 {0, 0, 76, 255}, 
	 {0, 0, 0, 255}, 
	 {76, 76, 0, 255}, 
	 {0, 76, 76, 255},
	 {255, 255, 255, 255}}
	};

CRGBAColor eraseColorRgb = {0, 0, 0, 255};

//-----------------------------------------------------------------------------

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
	{"brief01.pcx",   0,  1,  13, 140, 290,  59}, 
	{"brief02.pcx",   0,  2,  27,  34, 257, 177}, 
	{"brief03.pcx",   0,  3,  20,  22, 257, 177}, 
	{"brief02.pcx",   0,  4,  27,  34, 257, 177}, 

	{"moon01.pcx",    1,  5,  10,  10, 300, 170}, // level 1
	{"moon01.pcx",    2,  6,  10,  10, 300, 170}, // level 2
	{"moon01.pcx",    3,  7,  10,  10, 300, 170}, // level 3

	{"venus01.pcx",   4,  8,  15, 15, 300,  200}, // level 4
	{"venus01.pcx",   5,  9,  15, 15, 300,  200}, // level 5

	{"brief03.pcx",   6, 10,  20,  22, 257, 177}, 
	{"merc01.pcx",    6, 11,  10, 15, 300, 200},  // level 6
	{"merc01.pcx",    7, 12,  10, 15, 300, 200},  // level 7

	{"brief03.pcx",   8, 13,  20,  22, 257, 177}, 
	{"mars01.pcx",    8, 14,  10, 100, 300,  200}, // level 8
	{"mars01.pcx",    9, 15,  10, 100, 300,  200}, // level 9
	{"brief03.pcx",  10, 16,  20,  22, 257, 177}, 
	{"mars01.pcx",   10, 17,  10, 100, 300,  200}, // level 10

	{"jup01.pcx",    11, 18,  10, 40, 300,  200}, // level 11
	{"jup01.pcx",    12, 19,  10, 40, 300,  200}, // level 12
	{"brief03.pcx",  13, 20,  20,  22, 257, 177}, 
	{"jup01.pcx",    13, 21,  10, 40, 300,  200}, // level 13
	{"jup01.pcx",    14, 22,  10, 40, 300,  200}, // level 14

	{"saturn01.pcx", 15, 23,  10, 40, 300,  200}, // level 15
	{"brief03.pcx",  16, 24,  20,  22, 257, 177}, 
	{"saturn01.pcx", 16, 25,  10, 40, 300,  200}, // level 16
	{"brief03.pcx",  17, 26,  20,  22, 257, 177}, 
	{"saturn01.pcx", 17, 27,  10, 40, 300,  200}, // level 17

	{"uranus01.pcx", 18, 28,  100, 100, 300,  200}, // level 18
	{"uranus01.pcx", 19, 29,  100, 100, 300,  200}, // level 19
	{"uranus01.pcx", 20, 30,  100, 100, 300,  200}, // level 20
	{"uranus01.pcx", 21, 31,  100, 100, 300,  200}, // level 21

	{"neptun01.pcx", 22, 32,  10, 20, 300,  200}, // level 22
	{"neptun01.pcx", 23, 33,  10, 20, 300,  200}, // level 23
	{"neptun01.pcx", 24, 34,  10, 20, 300,  200}, // level 24

	{"pluto01.pcx",  25, 35,  10, 20, 300,  200}, // level 25
	{"pluto01.pcx",  26, 36,  10, 20, 300,  200}, // level 26
	{"pluto01.pcx",  27, 37,  10, 20, 300,  200}, // level 27

	{"aster01.pcx",  -1, 38,  10, 90, 300,  200}, // secret level -1
	{"aster01.pcx",  -2, 39,  10, 90, 300,  200}, // secret level -2
	{"aster01.pcx",  -3, 40,  10, 90, 300,  200}, // secret level -3

	{"end01.pcx",   SHAREWARE_ENDING_LEVEL_NUM,  1,  23, 40, 320, 200},   // shareware end
	{"end02.pcx",   REGISTERED_ENDING_LEVEL_NUM,  1,  5, 5, 300, 200},    // registered end
	{"end01.pcx",   REGISTERED_ENDING_LEVEL_NUM,  2,  23, 40, 320, 200},  // registered end
	{"end03.pcx",   REGISTERED_ENDING_LEVEL_NUM,  3,  5, 5, 300, 200}    // registered end
};

#define MAX_BRIEFING_SCREENS (sizeofa (briefingScreens))

#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      //  Adam: This is the nFrameber of frames in your new animating thing.
#define DOOR_DIV_INIT   6

//-----------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int32_t (__fastcall * pBriefingHandler) (void);
#else
typedef int32_t (* pBriefingHandler) (void);
#endif

typedef struct tBriefingHandlerInfo {
	int32_t					ch;
	int32_t					prevCh;
	pBriefingHandler	handlerP;
	} tBriefingHandlerInfo;

inline int32_t HandleA (void) { return briefing.HandleA (); }
inline int32_t HandleB (void) { return briefing.HandleB (); }
inline int32_t HandleC (void) { return briefing.HandleC (); }
inline int32_t HandleD (void) { return briefing.HandleD (); }
inline int32_t HandleF (void) { return briefing.HandleF (); }
inline int32_t HandleN (void) { return briefing.HandleN (); }
inline int32_t HandleO (void) { return briefing.HandleO (); }
inline int32_t HandleP (void) { return briefing.HandleP (); }
inline int32_t HandleR (void) { return briefing.HandleR (); }
inline int32_t HandleS (void) { return briefing.HandleS (); }
inline int32_t HandleT (void) { return briefing.HandleT (); }
inline int32_t HandleU (void) { return briefing.HandleU (); }
inline int32_t HandleZ (void) { return briefing.HandleZ (); }

inline int32_t HandleTAB (void) { return briefing.HandleTAB (); }
inline int32_t HandleBS (void) { return briefing.HandleBS (); }
inline int32_t HandleNEWL (void) { return briefing.HandleNEWL (); }
inline int32_t HandleSEMI (void) { return briefing.HandleSEMI (); }
inline int32_t HandleANY (void) { return briefing.HandleANY (); }

static tBriefingHandlerInfo briefingHandlers1 [] = {
	{'A', 0, ::HandleA},
	{'B', 0, ::HandleB},
	{'C', 0, ::HandleC},
	{'D', 0, ::HandleD},
	{'F', 0, ::HandleF},
	{'N', 0, ::HandleN},
	{'O', 0, ::HandleO},
	{'P', 0, ::HandleP},
	{'R', 0, ::HandleR},
	{'S', 0, ::HandleS},
	{'T', 0, ::HandleT},
	{'U', 0, ::HandleU},
	{'Z', 0, ::HandleZ}
	};

static tBriefingHandlerInfo briefingHandlers2 [] = {
	{'\t', 0, ::HandleTAB},
	{'\\', 0, ::HandleBS},
	{10, 0, ::HandleNEWL},
	{';', 10, ::HandleSEMI},
	{0, 0, ::HandleANY}
	};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CBriefingInfo::Init (void)
{ 
ch = -1;
prevCh = -1;
nHumChannel = -1;
nPrintingChannel = -1;
nBotChannel = -1;
nLineAdjustment = 1;
nDelayCount = KEY_DELAY_DEFAULT;
nRobot = -1;
nDoorDir = 1;
nCurrentColor = 1;
nFuncRes = 0;
nEraseColor = BLACK_RGBA;
*szBitmapName = '\0';
strcpy (szSpinningRobot, "rba.mve");
}

//-----------------------------------------------------------------------------

void CBriefingInfo::Setup (char* _message, int32_t _nLevel, int32_t _nScreen)
{
Init ();
message = _message;
nLevel = _nLevel;
nScreen = _nScreen;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CBriefing::Init (void)
{
#if 0
briefingHandlers1 [0].handlerP = &CBriefing::HandleA;
briefingHandlers1 [1].handlerP = &CBriefing::HandleB;
briefingHandlers1 [2].handlerP = &CBriefing::HandleC;
briefingHandlers1 [3].handlerP = &CBriefing::HandleD;
briefingHandlers1 [4].handlerP = &CBriefing::HandleF;
briefingHandlers1 [5].handlerP = &CBriefing::HandleN;
briefingHandlers1 [6].handlerP = &CBriefing::HandleO;
briefingHandlers1 [7].handlerP = &CBriefing::HandleP;
briefingHandlers1 [8].handlerP = &CBriefing::HandleR;
briefingHandlers1 [9].handlerP = &CBriefing::HandleS;
briefingHandlers1 [10].handlerP = &CBriefing::HandleT;
briefingHandlers1 [11].handlerP = &CBriefing::HandleU;
briefingHandlers1 [12].handlerP = &CBriefing::HandleZ;

briefingHandlers2 [0].handlerP = &CBriefing::HandleTAB;
briefingHandlers2 [0].handlerP = &CBriefing::HandleBS;
briefingHandlers2 [0].handlerP = &CBriefing::HandleNEWL;
briefingHandlers2 [0].handlerP = &CBriefing::HandleSEMI;
briefingHandlers2 [0].handlerP = &CBriefing::HandleANY;
#endif
}

//-----------------------------------------------------------------------------

int32_t CBriefing::StartSound (int32_t nChannel, int16_t nSound, fix nVolume, const char* pszWAV)
{
if (nChannel < 0) {
	int32_t bSound = gameStates.sound.bD1Sound;
	gameStates.sound.bD1Sound = 0;
	nChannel = audio.StartSound (audio.XlatSound (nSound), SOUNDCLASS_GENERIC, nVolume, 0xFFFF / 2, 1, -1, -1, -1, I2X (1), pszWAV);
	gameStates.sound.bD1Sound = bSound;
	}
return nChannel;
}

//-----------------------------------------------------------------------------

void CBriefing::StopSound (int32_t& nChannel)
{
if (nChannel > -1) {
	audio.StopSound (nChannel);
	nChannel = -1;
	}
}

//-----------------------------------------------------------------------------

int32_t CBriefing::StartHum (int32_t nLevel, int32_t nScreen, int32_t bExtraSounds)
{
if (bExtraSounds && (nScreen > 3)) {	//only play for the mission directives on a planet background
	char	szWAV [20];

	if (nLevel < 0)
		nLevel = 27 - nLevel;
 	sprintf (szWAV, "brf%02d.wav", nLevel);
	if (0 > (m_info.nHumChannel = StartSound (m_info.nHumChannel, SOUND_BRIEFING_HUM, I2X (4), szWAV)))
		return 0;
	songManager.StopAll ();
	G3_SLEEP (250);
	m_bAudioPlaying = 1;
	}
else {
	if (songManager.Playing ())
		return 0;
	m_info.nHumChannel = StartSound (m_info.nHumChannel, SOUND_BRIEFING_HUM, I2X (1) / 2, NULL);
	}
return m_info.nHumChannel >= 0;
}

//-----------------------------------------------------------------------------

tD1ExtraBotSound* CBriefing::FindExtraBotSound (int16_t nLevel, int16_t nBotSig)
{
	tD1ExtraBotSound	*p = extraBotSounds;
	int32_t				i = sizeof (extraBotSounds) / sizeof (tD1ExtraBotSound);

if (!gameStates.app.bD1Mission)
	return NULL;
for (; i; i--, p++)
	if ((p->nLevel == nLevel) && (p->nBotSig == nBotSig))
		return p;
return NULL;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::StartExtraBotSound (int32_t nChannel, int16_t nLevel, int16_t nBotSig)
{
	tD1ExtraBotSound*	p;

StopSound (nChannel);
if (!gameStates.app.bHaveExtraData)
	return -1;
if (!(p = FindExtraBotSound (nLevel, nBotSig)))
	return -1;
return StartSound (nChannel, -1, I2X (8), p->pszName);
}

//-----------------------------------------------------------------------------

int32_t CBriefing::RenderImage (char* pszImg)
{
	char	*ps, c = '0';

strcpy (m_szBackground, pszImg);
strlwr (m_szBackground);
if ((ps = strstr (m_szBackground, "b.pcx")))
	 c = 'b';
else if ((ps = strstr (m_szBackground, ".pcx"))) {
	c = *--ps;
	strcpy (--ps, "0b.pcx");
	*ps = c;
	}


if (strstr (m_szBackground, ".tga")) {
	CTGA tga (&m_background);
	if (!tga.Read (m_szBackground, gameFolders.game.szData [0], -1, 1.0, 0))
		return PCX_ERROR_OPENING;
	RenderElement (3);
	return PCX_ERROR_NONE;
	}
else {
	for (;;) {
		RenderElement (4);
		if (m_pcxError == PCX_ERROR_NONE)
			break;
		if (!ps)
			break;
		strcpy (ps, "00.pcx");
		*++ps = c;
		ps = NULL;
		} 
	}
return m_pcxError;
}

//-----------------------------------------------------------------------------

void CBriefing::InitCharPos (tBriefingScreen *pScreen, int32_t bRescale)
{
if (bRescale) {
	pScreen->textLeft = AdjustX (pScreen->textLeft);
	pScreen->textTop = AdjustY (pScreen->textTop);
	pScreen->textWidth = AdjustX (pScreen->textWidth);
	pScreen->textHeight = AdjustY (pScreen->textHeight);
	}
m_info.briefingTextX = pScreen->textLeft;
m_info.briefingTextY = gameStates.app.bD1Mission ? pScreen->textTop : pScreen->textTop - (8 * (1 + gameStates.menus.bHires));
}

//-----------------------------------------------------------------------------

void CBriefing::RenderBitmapFrame (int32_t bRedraw)
{
int32_t t = SDL_GetTicks ();
if (t - m_info.tAnimate < 10)
	return;
m_info.tAnimate = t;

	//	Only plot every nth frame.
if (!bRedraw && m_info.nDoorDivCount) {
	m_info.nDoorDivCount--;
	}

if (*m_info.szBitmapName) {
	char		*poundSignP;
	int32_t	nFrame, dig1, dig2;

	//	Set supertransparency color to black
	if (!bRedraw) {	//extract current bitmap frame number from bitmap name (<name>#<frame>)
		poundSignP = strchr (m_info.szBitmapName, '#');
		Assert (poundSignP != NULL);

		dig1 = poundSignP [1];
		dig2 = poundSignP [2];
		if (dig2 == 0)
			nFrame = dig1-'0';
		else
			nFrame = (dig1-'0')*10 + (dig2-'0');

		switch (m_info.nAnimatingBitmapType) {
			case 0:
				if (!m_info.nDoorDivCount) {
					nFrame += m_info.nDoorDir;
					if (nFrame > EXIT_DOOR_MAX) {
						nFrame = EXIT_DOOR_MAX;
						m_info.nDoorDir = -1;
						}
					else if (nFrame < 0) {
						nFrame = 0;
						m_info.nDoorDir = 1;
					}
				}
				break;
			case 1:
				if (!m_info.nDoorDivCount)
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
	tBitmapIndex bmi = PiggyFindBitmap (m_info.szBitmapName, gameStates.app.bD1Mission);
	if (0 > int16_t (bmi.index))
		return;
	m_bitmap = gameData.pigData.tex.bitmaps [gameStates.app.bD1Mission] + bmi.index;

	LoadTexture (bmi.index, 0, gameStates.app.bD1Mission);
	m_bitmap->SetStatic (1);

	RenderElement (8);
	//paletteManager.ResumeEffect ();
	if (!(bRedraw || m_info.nDoorDivCount)) {
#if 1
	m_info.nDoorDivCount = DOOR_DIV_INIT;
#else
	switch (m_info.nAnimatingBitmapType) {
		case 0:
			if (nFrame == EXIT_DOOR_MAX) {
				m_info.nDoorDir = -1;
				m_info.nDoorDivCount = DOOR_DIV_INIT;
				} 
			else if (nFrame == 0) {
				m_info.nDoorDir = 1;
				m_info.nDoorDivCount = DOOR_DIV_INIT;
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

void CBriefing::RenderBitmap (CBitmap* pBm)
{
m_bitmap = pBm;
RenderElement (7);
}

//-----------------------------------------------------------------------------

void CBriefing::RenderRobotFrame (void)
{
if (!ROBOTINFO (m_info.nRobot))
	return;

int32_t t = SDL_GetTicks ();
if (t - m_info.tAnimate < 10)
	return;

if (m_info.bInitAnimate) {
	paletteManager.Load ("", "", 0, 0, 1);
	OglCachePolyModelTextures (ROBOTINFO (m_info.nRobot)->nModel);
	//paletteManager.ResumeEffect ();
	m_info.bInitAnimate = SDL_GetTicks ();
	}
gameStates.render.bFullBright	= 1;

//ogl.m_states.bEnableScissor = 1;
RenderElement (6);
//ogl.m_states.bEnableScissor = 0;
gameStates.render.bFullBright = 0;
m_info.vRobotAngles.v.coord.h += 15 * (t - m_info.tAnimate);
m_info.tAnimate = t;
}

//-----------------------------------------------------------------------------

void CBriefing::RenderRobotMovie (void)
{
RenderElement (5);
}

//-----------------------------------------------------------------------------

void CBriefing::Animate (void)
{
if (m_info.bRobotPlaying)
	RenderRobotMovie ();
else if (m_info.nRobot != -1)
	RenderRobotFrame ();
else if (*m_info.szBitmapName)
	RenderBitmapFrame (0);
}

//-----------------------------------------------------------------------------

void CBriefing::InitSpinningRobot (void)
{
m_info.bInitAnimate = 1;
m_info.tAnimate = SDL_GetTicks ();
}

//------------------------------------------------------------------------------ 

void CBriefing::SetupAnimationCanvas (CCanvas* baseCanv)
{
int32_t x, y, w, h;
if (ogl.IsOculusRift ()) {
	y = gameData.renderData.window.Height (false) / 3;
	w = gameData.renderData.window.Height (false) - y;
	x = gameData.renderData.window.Width (false) - w;
	h = w = 9 * w / 10;
	}
else {
	x = RescaleX (138);
	y = RescaleY (55);
	w = RescaleX (163);
	h = RescaleY (136);
	}
AnimCanv ().Setup (baseCanv, x, y, w, h);
AnimCanv ().Activate ("CBriefing::SetupAnimationCanvas", baseCanv);
AnimCanv ().Clear (m_info.nEraseColor);
}

//------------------------------------------------------------------------------ 

float CBriefing::GetScale (void)
{
#if 0 //DBG
return ogl.IsSideBySideDevice () ? 0.5f : 1.0f;
#else
return (ogl.IsOculusRift () /*&& gameStates.app.bGameRunning*/) ? 0.5f : 1.0f;
#endif
}

//---------------------------------------------------------------------------

void CBriefing::RenderElement (int32_t nElement)
{
CFrameController fc;
for (fc.Begin (); fc.Continue (); fc.End ()) {
	CCanvas* baseCanv;
	if (!ogl.IsOculusRift ()) 
		baseCanv = &gameData.renderData.frame;
	else {
		int32_t w, h;
		cockpit->SetupSceneCenter (&gameData.renderData.frame, w, h);
		baseCanv = &gameData.renderData.window;
		}

	baseCanv->Activate ("CBriefing::RenderElement");
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetScale (fontManager.Scale () * GetScale ());
	gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
	switch (nElement) {
		case 0:
			fontManager.SetColorRGB (briefFgColors [gameStates.app.bD1Mission] + m_info.nCurrentColor, NULL);
			GrPrintF (NULL, Scaled (m_info.briefingTextX + 1), Scaled (m_info.briefingTextY), "_");
			break;

		case 1:
			fontManager.SetColorRGBi (m_info.nEraseColor, 1, 0, 0);
			GrPrintF (NULL, Scaled (m_info.briefingTextX + 1), Scaled (m_info.briefingTextY), "_");
		//	erase the character
			fontManager.SetColorRGB (briefBgColors [gameStates.app.bD1Mission] + m_info.nCurrentColor, NULL);
			GrPrintF (NULL, Scaled (m_info.briefingTextX), Scaled (m_info.briefingTextY), m_message);
			break;

		case 2:
			fontManager.SetColorRGB (briefFgColors [gameStates.app.bD1Mission] + m_info.nCurrentColor, NULL);
			GrPrintF (NULL, Scaled (m_info.briefingTextX + 1), Scaled (m_info.briefingTextY), m_message);
			break;

		case 3:
			m_background.RenderFullScreen ();
			break;

		case 4:
			m_pcxError = PcxReadFullScrImage (m_szBackground, gameStates.app.bD1Mission);
			break;

		case 5:
			AnimCanv ().Activate ("CBriefing::RenderElement (1)", baseCanv);
			movieManager.RotateRobot ();
			AnimCanv ().Deactivate ();
			break;

		case 6:
			if (ROBOTINFO (m_info.nRobot)) {
				SetupAnimationCanvas (baseCanv);
				DrawModelPicture (ROBOTINFO (m_info.nRobot)->nModel, &m_info.vRobotAngles);
				AnimCanv ().Deactivate ();
				glLineWidth (1);
				AnimCanv ().Deactivate ();
				}
			break;

		case 7:
			{
			CCanvas bitmapCanv;
			bitmapCanv.Setup (baseCanv, 220, 45, m_bitmap->Width (), m_bitmap->Height ());
			bitmapCanv.Activate ("CBriefing::RenderElement (2)", baseCanv);
			m_bitmap->RenderScaled (0, 0);
			bitmapCanv.Deactivate ();
			}
			break;

		case 8:
			{
			SetupAnimationCanvas (baseCanv);
			G3StartFrame (transformation, 1, 0, 0);
			CFixVector p = CFixVector::ZERO;
			SetupTransformation (transformation, p, CFixMatrix::IDENTITY, gameStates.render.xZoom, 1);
			GLint	depthFunc = ogl.GetDepthMode ();
			ogl.SetDepthMode (GL_ALWAYS);
			//AnimCanv ().Activate (baseCanv);
			p.v.coord.z = 2 * I2X (AnimCanv ().Width (false));
			ogl.RenderBitmap (m_bitmap, p, I2X (AnimCanv ().Width (false)), I2X (AnimCanv ().Height (false)), NULL, 1.0, 0);
			AnimCanv ().Deactivate ();
			ogl.SetDepthMode (depthFunc);
			G3EndFrame (transformation, 0);
			}

		default:
			// invalid call
			break;
		}
	baseCanv->Deactivate ();
	}
fontManager.SetScale (fontManager.Scale () / GetScale ());
}

//---------------------------------------------------------------------------
// Returns char width.
// If showRobotFlag set, then show a frame of the spinning robot.
// When delay is zero, the briefing rendering code is rebuilding the entire
// briefing screen up to the last currently visible character (required when
// dual buffering is enabled). In that case there is no delay.

int32_t CBriefing::PrintCharDelayed (int32_t delay)
{
m_message [0] = char (m_info.ch);
m_message [1] = 0;

int32_t	w, h, aw;
fontManager.SetCurrent (GAME_FONT);
fontManager.SetScale (fontManager.Scale () * GetScale ());
fontManager.Current ()->StringSize (m_message, w, h, aw);

static fix tText = 0;
if (!tText)
	tText = SDL_GetTicks ();

#if 0
Assert ((m_info.nCurrentColor >= 0) && (m_info.nCurrentColor < MAX_BRIEFING_COLORS));
#else
if (m_info.nCurrentColor < 0)
	m_info.nCurrentColor = 0;
else if (m_info.nCurrentColor >= MAX_BRIEFING_COLORS)
	m_info.nCurrentColor = MAX_BRIEFING_COLORS - 1;
#endif

//	Draw cursor if there is some delay and caller says to draw cursor
if (m_info.bFlashingCursor && !m_info.bRedraw) 
	RenderElement (0);

	fix t = SDL_GetTicks ();

if ((delay > 0) && !m_info.bRedraw) {
	delay = tText + (m_bAudioPlaying ? 110 : 1000 / 15);
	do {
		Animate ();
		} while ((t = SDL_GetTicks ()) < delay);
	tText = t;
	}

//	Erase cursor
if (m_info.bFlashingCursor && (delay > 0) && !m_info.bRedraw) 
	RenderElement (1);

//draw the character
RenderElement (2);
return w;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::LoadImage (char* szBriefScreen)
{
#if TRACE
console.printf (CON_DBG, "Loading new briefing <%s>\n", szBriefScreen);
#endif
strcpy (m_info.szCurImage, szBriefScreen);
if (paletteManager.DisableEffect ())
	return 0;
int32_t pcxResult = RenderImage (szBriefScreen);
if (pcxResult != PCX_ERROR_NONE) {
#if DBG
	static char szErrScreen [15] = "";

	if (strncmp (szErrScreen, szBriefScreen, sizeof (szErrScreen))) {
		Error ("Error loading briefing screen <%s>, \nPCX load error: %s (%i)\n", 
				 szBriefScreen, PcxErrorMsg (pcxResult), pcxResult);
		strncpy (szErrScreen, szBriefScreen, sizeof (szErrScreen));
		}
#endif
	}
SetColors ();
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::LoadImage (int32_t nScreen)
{
	char*	szBriefScreen;

szBriefScreen = gameStates.app.bD1Mission 
					 ? briefingScreens [nScreen % MAX_BRIEFING_SCREENS].szName
					 : m_info.szCurImage;
if (!*szBriefScreen)
	return 0;
glClear (GL_COLOR_BUFFER_BIT);
return LoadImage (szBriefScreen);
}

//-----------------------------------------------------------------------------

void CBriefing::FlashCursor (int32_t bCursor)
{
if (bCursor == 0)
	return;
if ((TimerGetFixedSeconds () % (I2X (1)/2)) > (I2X (1)/4))
	fontManager.SetColorRGB (briefFgColors [gameStates.app.bD1Mission] + m_info.nCurrentColor, NULL);
else
	fontManager.SetColorRGB (&eraseColorRgb, NULL);
GrPrintF (NULL, Scaled (m_info.briefingTextX + 1), Scaled (m_info.briefingTextY), "_");
//if (ogl.m_states.nDrawBuffer == GL_FRONT)
//	ogl.Update (0);
}

//-----------------------------------------------------------------------------

const char* CBriefing::NextPage (const char* message)
{
	const char	*pNextPage = strstr (message, "$P");
	const char	*pNextBrief = strstr (message, "$S");

if (pNextPage && pNextBrief)
	return ((pNextPage < pNextBrief) ? pNextPage : pNextBrief);
return (pNextPage ? pNextPage : pNextBrief);
return NULL;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::PageHasRobot (const char* message)
{
	const char	*pEnd = NextPage (message);
	const char	*pBot = strstr (message, "$R");

return pBot && (!pEnd || (pBot < pEnd));
}

//-----------------------------------------------------------------------------

int32_t CBriefing::InKey (void)
{
	int32_t	funcRes = KeyInKey ();

if (funcRes == KEY_PRINT_SCREEN) {
	gameStates.app.bSaveScreenShot = 1;
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

int32_t CBriefing::WaitForKeyPress (void)
{
	time_t t;
	int32_t keypress = 0;

m_info.bChattering = 0;
StopSound (m_info.nPrintingChannel);
if (!m_info.t0)
	m_info.t0 = SDL_GetTicks ();
do {		//	Wait for a key
	while ((t = SDL_GetTicks ()) - m_info.t0 < 10)
		;
	FlashCursor (m_info.bFlashingCursor);
	Animate ();
	m_info.t0 = t;
	keypress = InKey ();
	if (ogl.m_states.nDrawBuffer == GL_BACK)
		break;
	ogl.Update (0);
	} while (!keypress);
return keypress;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::ParseMessageInt (char*& pszMsg, bool bSkip)
{
	int32_t i = 0;

while (*pszMsg && (*pszMsg == ' '))
	 pszMsg++;
while ((*pszMsg >= '0') && (*pszMsg <= '9')) {
	i = 10 * i + *pszMsg - '0';
	pszMsg++;
	}
if (bSkip)
	while (*pszMsg && (*pszMsg++ != 10))
		;
else
	pszMsg++;
return i;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::DefineBox (void)
{
	int32_t i = 0, nScreen = ParseMessageInt (m_info.message);
	char name [20];

while (*m_info.message && (*m_info.message != ' ') && (i < int32_t (sizeof (name)) - 1)) {
	name [i++] = *m_info.message;
	m_info.message++;
	}
name [i] = '\0';   // slap a delimiter on this guy
m_info.curScreen = briefingScreens [nScreen % MAX_BRIEFING_SCREENS];
strcpy (m_info.curScreen.szName, name);
m_info.curScreen.nLevel = ParseMessageInt (m_info.message);
m_info.curScreen.nMessage = ParseMessageInt (m_info.message);
m_info.curScreen.textLeft = ParseMessageInt (m_info.message);
m_info.curScreen.textTop = ParseMessageInt (m_info.message);
m_info.curScreen.textWidth = ParseMessageInt (m_info.message);
m_info.curScreen.textHeight = ParseMessageInt (m_info.message, true);  // NOTICE!!!
InitCharPos (&m_info.curScreen, 1);
return nScreen;
}

//-----------------------------------------------------------------------------

void CBriefing::ParseMessageText (char* pszName)
{
while (*m_info.message && (*m_info.message == ' '))
	 m_info.message++;

while (*m_info.message && (*m_info.message != ' ') && (*m_info.message != 10)) {
	if (*m_info.message != '\n')
		*pszName++ = *m_info.message;
	m_info.message++;
	}
while (*m_info.message && (*m_info.message != 10))
	m_info.message++;
m_info.message++;
*pszName = '\0';
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleA (void)
{
m_info.nLineAdjustment = 1 - m_info.nLineAdjustment;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleB (void)
{
	char     szBitmap [32];
	CBitmap  bmGuy;
	CIFF		iff;
	int32_t      iff_error;

ParseMessageText (szBitmap);
strcat (szBitmap, ".bbm");
memset (&bmGuy, 0, sizeof (bmGuy));
iff_error = iff.ReadBitmap (szBitmap, &bmGuy, BM_LINEAR);
if (iff_error != IFF_NO_ERROR)
	return 0;
m_info.tAnimate = SDL_GetTicks ();
RenderBitmap (&bmGuy);
bmGuy.DestroyBuffer ();
m_info.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleC (void)
{
	char c = *m_info.message;

if ((c == 'F') || (c == 'B')) {
	m_info.message++;
	CRGBAColor* pColor = (c == 'F') ? briefFgColors [gameStates.app.bD1Mission] : briefBgColors [gameStates.app.bD1Mission];
	pColor->Red () = ParseMessageInt (m_info.message);
	pColor->Green () = ParseMessageInt (m_info.message);
	pColor->Blue () = ParseMessageInt (m_info.message);
	m_info.nCurrentColor = 0;
	}
else {
	m_info.nCurrentColor = ParseMessageInt (m_info.message);
#if 0
	Assert ((m_info.nCurrentColor >= 0) && (m_info.nCurrentColor < MAX_BRIEFING_COLORS));
#else
	if (m_info.nCurrentColor < 0)
		m_info.nCurrentColor = 0;
	else if (m_info.nCurrentColor >= MAX_BRIEFING_COLORS)
		m_info.nCurrentColor = MAX_BRIEFING_COLORS - 1;
#endif
	}
m_info.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleD (void)
{
m_info.nScreen = DefineBox ();
m_info.pScreen = &m_info.curScreen;
m_info.x = m_info.briefingTextX;
m_info.y = m_info.briefingTextY;
m_info.nLineAdjustment = 0;
m_info.prevCh = 10;                                   // read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleF (void)
{
m_info.bFlashingCursor = !m_info.bFlashingCursor;
m_info.prevCh = 10;
while (*m_info.message != 10)
	m_info.message++;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleN (void)
{
if (m_info.message > m_info.pj) {
	StopSound (m_info.nBotChannel);
	ParseMessageText (m_info.szBitmapName);
	strcat (m_info.szBitmapName, "#0");
	m_info.nAnimatingBitmapType = 0;
	m_info.tAnimate = SDL_GetTicks ();
	}
m_info.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleO (void)
{
if (m_info.message > m_info.pj) {
	ParseMessageText (m_info.szBitmapName);
	strcat (m_info.szBitmapName, "#0");
	m_info.nAnimatingBitmapType = 1;
	m_info.tAnimate = SDL_GetTicks ();
	}
m_info.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

static char szEndScreens [2][11] = {"end01b.pcx", "end01.pcx"};

int32_t CBriefing::HandleP (void)
{
if (!m_info.bGotZ) {
	Int3 (); // Hey ryan!!!!You gotta load a screen before you start printing to it!You know, $Z !!!
	LoadImage (szEndScreens [gameStates.menus.bHires]);
	m_info.bHaveScreen = 1;
	}
m_info.bNewPage = 1;
while (*m_info.message != 10)
	m_info.message++;	//	drop carriage return after special escape sequence
m_info.message++;
m_info.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleR (void)
{
if (m_info.message > m_info.pj) {
	if (m_info.bRobotPlaying) {
		movieManager.StopRobot ();
		m_info.bRobotPlaying = 0;
		}
	InitSpinningRobot ();
	if (!songManager.Playing ()) {
		m_info.nBotChannel = StartExtraBotSound (m_info.nBotChannel, (int16_t) m_info.nLevel, m_info.nBotSig++);
		if (m_info.nBotChannel >= 0)
			StopSound (m_info.nHumChannel);
		}
	}
if (gameStates.app.bD1Mission) {
	m_info.nRobot = ParseMessageInt (m_info.message);
	}
else {
	m_info.szSpinningRobot [2] = *m_info.message++; // ugly but proud
	if (m_info.message > m_info.pj) {
		AnimCanv ().Activate ("CBriefing::HandleR");
		m_info.bRobotPlaying = movieManager.StartRobot (m_info.szSpinningRobot);
		AnimCanv ().Deactivate ();
		if (m_info.bRobotPlaying) {
			RenderRobotMovie ();
			SetColors ();
			}
		}
	}
m_info.prevCh = 10;                           // read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleS (void)
{
int32_t keypress = WaitForKeyPress ();

if (!keypress)
	return -1;
if (keypress == KEY_ESC)
	m_info.nFuncRes = 1;
m_info.bFlashingCursor = 0;
return 0;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleT (void)
{
m_info.nTabStop = ParseMessageInt (m_info.message) * (1 + gameStates.menus.bHires);
m_info.prevCh = 10;							//	read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleU (void)
{
m_info.nScreen = ParseMessageInt (m_info.message);
m_info.curScreen = briefingScreens [m_info.nScreen % MAX_BRIEFING_SCREENS];
m_info.pScreen = &m_info.curScreen;
InitCharPos (m_info.pScreen, 1);
m_info.x = m_info.briefingTextX;
m_info.y = m_info.briefingTextY;
m_info.prevCh = 10;                                   // read to eoln
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleZ (void)
{
m_info.bGotZ = 1;
m_info.bDumbAdjust = 1;
int32_t i = 0;
while ((m_info.szBriefScreen [i] = *m_info.message) != '\n') {
	i++;
	m_info.message++;
	}
m_info.szBriefScreen [i] = 0;
while (*m_info.message != 10)    //  Get and drop eoln
	m_info.message++;
for (i = 0; m_info.szBriefScreen [i] != '.'; i++)
	m_info.szBriefScreenB [i] = m_info.szBriefScreen [i];
memcpy (m_info.szBriefScreenB + i, "b.pcx", sizeof ("b.pcx"));
i += sizeof ("b.pcx");
if ((gameStates.menus.bHires && CFile::Exist (m_info.szBriefScreenB, gameFolders.game.szData [0], 0)) || 
	 !CFile::Exist (m_info.szBriefScreen, gameFolders.game.szData [0], 0))
	LoadImage (m_info.szBriefScreenB);
else
	LoadImage (m_info.szBriefScreen);
m_info.bHaveScreen = 1;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleTAB (void)
{
if (m_info.briefingTextX - m_info.pScreen->textLeft < m_info.nTabStop)
	m_info.briefingTextX = m_info.pScreen->textLeft + m_info.nTabStop;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleBS (void)
{
m_info.prevCh = m_info.ch;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleNEWL (void)
{
if (m_info.prevCh != '\\') {
	m_info.prevCh = m_info.ch;
	if (m_info.bDumbAdjust == 0)
		m_info.briefingTextY += 8 * (gameStates.menus.bHires + 1);
	else
		m_info.bDumbAdjust--;
	m_info.briefingTextX = m_info.pScreen->textLeft;
	if (m_info.briefingTextY > m_info.pScreen->textTop + m_info.pScreen->textHeight) {
		LoadImage (m_info.nScreen);
		m_info.briefingTextX = m_info.pScreen->textLeft;
		m_info.briefingTextY = m_info.pScreen->textTop;
		}
	}
else {
	if (*m_info.message == 13)		//Can this happen? Above says m_info.ch==10
		m_info.message++;
	m_info.prevCh = m_info.ch;
	}
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleSEMI (void)
{
while (*m_info.message != 10)
	m_info.message++;
m_info.prevCh = 10;
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleANY (void)
{
if (!m_info.bGotZ) 
	LoadImage (szEndScreens [gameStates.menus.bHires]);
m_info.prevCh = m_info.ch;
m_info.bRedraw = !m_info.nDelayCount || (m_info.message <= m_info.pj);
if (!m_info.bRedraw) {
	m_info.nPrintingChannel = StartSound (m_info.nPrintingChannel, SOUND_BRIEFING_PRINTING, I2X (1), NULL);
	if (m_info.nPrintingChannel >= 0)
		m_info.bChattering = 1;
	}
m_info.briefingTextX += PrintCharDelayed (m_info.nDelayCount);
return 1;
}

//-----------------------------------------------------------------------------

char* CBriefing::SkipPage (void)
{
	char	ch;
	const char* pEnd = NextPage (m_info.message);

m_info.nScreen++;
while (*m_info.message && (!pEnd || (m_info.message < pEnd))) {
	ch = *m_info.message++;
	if (ch == '$') {
		ch = *m_info.message++;
		if (ch == 'D') {
			m_info.nScreen = DefineBox ();
			m_info.x = m_info.briefingTextX;
			m_info.y = m_info.briefingTextY;
			}
		else if (ch == 'U') {
			m_info.nScreen = ParseMessageInt (m_info.message);
			m_info.curScreen = briefingScreens [m_info.nScreen % MAX_BRIEFING_SCREENS];
			InitCharPos (&m_info.curScreen, 1);
			m_info.x = m_info.briefingTextX;
			m_info.y = m_info.briefingTextY;
			}
		}
	}
if (!m_info.message)
	return NULL;
ch = *m_info.message;
if (ch != '$')
	return m_info.message;
ch = *(++m_info.message);
if (ch == 'S')
	return NULL;
if (ch != 'P')
	do {
		m_info.message++;
		} while (::isdigit (*m_info.message) || ::isspace (*m_info.message));
return m_info.message;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleInput (void)
{
if (!m_info.bRedraw && m_info.nDelayCount) {
	int32_t keypress = InKey ();
	if (!keypress)
		return 1;
	if (keypress == KEY_ESC)
		return 0;
	if ((keypress == KEY_SPACEBAR) || (keypress == KEY_ENTER)) {
		StopSound (m_info.nBotChannel);
		m_info.nDelayCount = 0;
		m_info.bRedraw = 1;
		}
	if ((keypress == KEY_ALTED + KEY_ENTER) ||
		 (keypress == KEY_ALTED + KEY_PADENTER))
		ogl.ToggleFullScreen ();
	}
return 1;
}

//-----------------------------------------------------------------------------

int32_t CBriefing::HandleNewPage (void)
{
if (!m_info.bNewPage && (m_info.briefingTextY <= m_info.pScreen->textTop + m_info.pScreen->textHeight))
	return 1;
m_info.bNewPage = 0;
int32_t keypress = WaitForKeyPress ();
if (!keypress)
	return -1;
if (m_info.bRobotPlaying)
	movieManager.StopRobot ();
m_info.bRobotPlaying = 0;
m_info.nRobot = -1;
if (keypress == KEY_ESC)
	return 0;
if (m_info.bOnlyRobots) {
	*m_info.szBriefScreen = *m_info.szBriefScreenB = '\0';
	while (m_info.message && !PageHasRobot (m_info.message))
		if (!SkipPage ())
			return 0;
	if (!m_info.message)
		return 0;
	}
m_info.pi = m_info.message;
//if (ogl.m_states.nDrawBuffer == GL_FRONT) {
//	LoadImage (m_info.nScreen);
//	ogl.Update (0);
//	}
m_info.briefingTextX = m_info.pScreen->textLeft;
m_info.briefingTextY = m_info.pScreen->textTop;
m_info.nDelayCount = KEY_DELAY_DEFAULT;
return -1;
}

//-----------------------------------------------------------------------------
// Return true if message got aborted by user (pressed ESC), else return false.

int32_t CBriefing::ShowMessage (int32_t nScreen, char* message, int32_t nLevel)
{
	tBriefingHandlerInfo	*pInfo;
	int32_t					bSongPlaying = songManager.Playing (), bHumPlaying = 0;
	int32_t					h, i;

m_info.Setup (message, nLevel, nScreen);

//if (gameStates.app.bNostalgia)
//	ogl.SetDrawBuffer (GL_FRONT, 0);

m_info.bExtraSounds = gameStates.app.bHaveExtraData && gameStates.app.bD1Mission && 
							 (missionManager.nCurrentMission == missionManager.nBuiltInMission [1]);
m_info.bOnlyRobots = movieManager.m_bHaveExtras && m_info.bExtraSounds && (m_info.nLevel == 1) && (m_info.nScreen < 4);

fontManager.SetCurrent (GAME_FONT);

if (m_info.bOnlyRobots) {
	while (m_info.message && !PageHasRobot (m_info.message))
		SkipPage ();
	if (!m_info.message)
		goto done;
	}
m_info.pi = m_info.pj = m_info.message;

m_bAudioPlaying = 0;
bHumPlaying = StartHum (m_info.nLevel, m_info.nScreen, m_info.bExtraSounds);
m_info.curScreen = briefingScreens [m_info.nScreen % MAX_BRIEFING_SCREENS];
m_info.pScreen = &m_info.curScreen;
if (gameStates.app.bD1Mission)
	m_info.bGotZ = 1;
InitCharPos (m_info.pScreen, gameStates.app.bD1Mission);
m_info.x = m_info.briefingTextX;
m_info.y = m_info.briefingTextY;
*m_info.szBriefScreen = *m_info.szBriefScreenB = '\0';

redrawPage:

for (;;) {
	m_info.pj = m_info.message;
	ogl.Update (0);
	//if (ogl.m_states.nDrawBuffer == GL_FRONT)
	//	m_info.pi = m_info.message;
	//else 
		{
		m_info.message = m_info.pi;
		m_info.briefingTextX = m_info.x;
		m_info.briefingTextY = m_info.y;
		LoadImage (m_info.nScreen);
		m_info.curScreen = briefingScreens [m_info.nScreen % MAX_BRIEFING_SCREENS];
		m_info.pScreen = &m_info.curScreen;
		InitCharPos (m_info.pScreen, 1);
		}

	m_info.bRedraw = 1;
	m_info.bNewPage = 0;
	while (m_info.bRedraw) {
		m_info.ch = *m_info.message++;
		if (m_info.ch == '$') {
			m_info.ch = *m_info.message++;
			for (i = sizeofa (briefingHandlers1), pInfo = briefingHandlers1; i; i--, pInfo++)
				if ((m_info.ch == pInfo->ch) && (!pInfo->prevCh || (m_info.prevCh == pInfo->prevCh))) {
					h = pInfo->handlerP ();
					if (h < 0)
						goto redrawPage;
					if (!h)
						goto done;
					break;
					}
			}
		else {
			for (i = sizeofa (briefingHandlers2), pInfo = briefingHandlers2; i; i--, pInfo++)
				if (!pInfo->ch || ((m_info.ch == pInfo->ch) && (!pInfo->prevCh || (m_info.prevCh == pInfo->prevCh)))) {
					pInfo->handlerP ();
					break;
					}
			}
		if (!HandleInput ()) {
			m_info.nFuncRes = 1;
			goto done;
			}

		if (m_info.briefingTextX > m_info.pScreen->textLeft + m_info.pScreen->textWidth) {
			m_info.briefingTextX = m_info.pScreen->textLeft;
			m_info.briefingTextY += m_info.pScreen->textTop;
			}

		h = HandleNewPage ();
		if (h < 0)
			goto redrawPage;
		if (!h) {
			m_info.nFuncRes = 1;
			goto done;
			}
		}
	}

done:

if (m_info.bRobotPlaying) {
	movieManager.StopRobot ();
	m_info.bRobotPlaying = 0;
	}
StopSound (m_info.nPrintingChannel);
StopSound (m_info.nBotChannel);
if (bHumPlaying)
	StopSound (m_info.nHumChannel);
if (bSongPlaying && !songManager.Playing ())
	songManager.Play (SONG_BRIEFING, 1);
m_bAudioPlaying = 0;
return m_info.nFuncRes;
}

//-----------------------------------------------------------------------------
// Return a pointer to the start of text for screen #nScreen. Screens start with token '$S'.

char* CBriefing::GetMessage (int32_t nScreen)
{
	char* pszMsg = &m_info.briefingText [0];

for (int32_t i = 0; i < m_info.nBriefingTextLen; ) {
	if ((*pszMsg++ == '$') && (*pszMsg++ == 'S') && (ParseMessageInt (pszMsg) == nScreen))
		return pszMsg;
	i = int32_t (pszMsg - m_info.briefingText.Buffer ());
	}
return NULL;
}

//-----------------------------------------------------------------------------
//	Load Descent briefing text.
int32_t CBriefing::LoadImageText (char* filename, CCharArray& textBuffer)
{
	CFile cf;
	int32_t	len, i;
	int32_t	bHaveBinary;
	char	*pBuffer;
	
	static char fileModes [2][3] = {"rb", "rb"};

if (!strstr (filename, ".t"))
	strcat (filename, ".tex");
bHaveBinary = (strstr (filename, ".txb") != NULL);
if (!(cf.Open (filename, gameFolders.mods.szCurrent, fileModes [bHaveBinary], gameStates.app.bD1Mission) ||
	   cf.Open (filename, gameFolders.game.szData [0], fileModes [bHaveBinary], gameStates.app.bD1Mission))) {
	bHaveBinary = !bHaveBinary;
	strcpy (strstr (filename, ".t"), bHaveBinary ? ".txb" : ".tex");
	if (!(cf.Open (filename, gameFolders.mods.szCurrent, fileModes [bHaveBinary], gameStates.app.bD1Mission) ||
			cf.Open (filename, gameFolders.game.szData [0], fileModes [bHaveBinary], gameStates.app.bD1Mission))) {
		PrintLog (0, "can't open briefing '%s'!\n", filename);
		return 0;
		}
	}
if (bHaveBinary) {
	len = (int32_t) cf.Length ();
	if (!textBuffer.Create (len))
		return 0;
	pBuffer = &textBuffer [0];
	for (i = 0; i < len; i++, pBuffer++) {
		cf.Read (pBuffer, 1, 1);
			if (*pBuffer == 13)
				pBuffer--;
		}
	cf.Close ();
	for (i = 0, pBuffer = &textBuffer [0]; i < len; i++, pBuffer++)
		if (*pBuffer != '\n')
			*pBuffer = EncodeRotateLeft ((char) (EncodeRotateLeft (*pBuffer) ^ BITMAP_TBL_XOR));
	}
else {
	len = (int32_t) cf.Length ();
	if (!textBuffer.Create (len + 500))
		return 0;
	pBuffer = &textBuffer [0];
	for (i = 0; i < len; i++, pBuffer++) {
		cf.Read (pBuffer, 1, 1);
			if (*pBuffer == 13)
				pBuffer--;
		}
	*pBuffer = 0;
	cf.Close ();
	}
m_info.nBriefingTextLen = (int32_t) (pBuffer - textBuffer.Buffer ());
PrintLog (-1);
return 1;
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int32_t CBriefing::ShowText (int32_t nScreen, int16_t nLevel)
{
	char*	pszMsg;
	int32_t	i;

pszMsg = GetMessage (gameStates.app.bD1Mission ? briefingScreens [nScreen % MAX_BRIEFING_SCREENS].nMessage : nScreen);
if (!pszMsg)
	return (0);
SetColors ();
ogl.SetTexturing (true);
i = ShowMessage (nScreen, pszMsg, nLevel);
ogl.SetTexturing (false);
return i;
}

//-----------------------------------------------------------------------------
// Return true if screen got aborted by user, else return false.
int32_t CBriefing::LoadLevelScreen (int32_t nScreen, int16_t nLevel)
{
if (gameOpts->gameplay.bSkipBriefingScreens) {
	console.printf (CON_DBG, "Skipping briefing screen [%s]\n", &briefingScreens [nScreen % MAX_BRIEFING_SCREENS].szName);
	return 0;
	}
if (gameStates.app.bD1Mission) {
	int32_t pcxResult = RenderImage (briefingScreens [nScreen % MAX_BRIEFING_SCREENS].szName);
	if (pcxResult != PCX_ERROR_NONE) {
		console.printf (CON_DBG, "File '%s', PCX load error: %s (%i)\n  (It's a briefing gameData.renderData.screen.  Does this cause you pain?)\n", 
							 briefingScreens [nScreen % MAX_BRIEFING_SCREENS].szName, PcxErrorMsg (pcxResult), pcxResult);
		Int3 ();
		return 0;
		}
	ogl.Update (0);
	}
if (!ShowText (nScreen, nLevel))
	return 0;
return 1;
}

//-----------------------------------------------------------------------------

void CBriefing::SetColors (void)
{
//paletteManager.ResumeEffect ();

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

void CBriefing::Run (const char* filename, int32_t nLevel)
{
	int32_t	bAbortBriefing = 0;
	int32_t	nCurBriefingScreen = 0;
	int32_t	bEnding = strstr (filename, "endreg") || !stricmp (filename, missionManager.szBriefingFilename [1]);
	char	fnBriefing [FILENAME_LEN];

PrintLog (1, "Starting the briefing\n");
gameStates.render.nFlashScale = 0;
gameStates.render.bBriefing = 1;
if (gameOpts->gameplay.bSkipBriefingScreens) {
	console.printf (CON_DBG, "Skipping all briefing screens.\n");
	gameStates.render.bBriefing = 0;
	PrintLog (-1);
	return;
	}
strcpy (fnBriefing, ((nLevel != 0x7e) && *missionManager.szBriefingFilename [0]) ? missionManager.szBriefingFilename [0] : filename);
console.printf (CON_DBG, "Trying briefing screen <%s>\n", fnBriefing);
PrintLog (0, "Looking for briefing screen '%s'\n", fnBriefing);
if (!*fnBriefing) {
	gameStates.render.bBriefing = 0;
	PrintLog (-1);
	return;
	}

//if (gameStates.app.bNostalgia)
//	ogl.SetDrawBuffer (GL_FRONT, 0);
//else
	ogl.ChooseDrawBuffer ();

if (gameStates.app.bD1Mission && (missionManager.nCurrentMission != missionManager.nBuiltInMission [1])) {
	int32_t i;
	for (i = 0; i < 2; i++) {
		char* psz = strchr (fnBriefing, '.');
		if (psz)
			*psz = '\0';
		strcat (fnBriefing, i ? ".txb" : ".tex");
		FILE* fp = hogFileManager.Find (&hogFileManager.AltFiles (), "", fnBriefing, NULL);
		if	 (fp) {
			fclose (fp);
			break;
			}
		}
	if (i == 2) {
		gameStates.render.bBriefing = 0;
		return;
		}
	}

if (!LoadImageText (fnBriefing, m_info.briefingText)) {
	gameStates.render.bBriefing = 0;
	return;
	}

int32_t nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);

audio.StopAllChannels ();
audio.SetFxVolume (gameConfig.nAudioVolume [1], 1);
audio.SetVolumes ((gameConfig.nAudioVolume [0] * 32768) / 8, (gameConfig.nMidiVolume * 128) / 8);
songManager.Play (SONG_BRIEFING, 1);
SetScreenMode (SCREEN_MENU);
gameStates.render.nShadowPass = 0;
console.printf (CON_DBG, "Playing briefing screen <%s>, level %d\n", fnBriefing, nLevel);
m_info.Init ();
KeyFlush ();
if (gameStates.app.bD1Mission) {
	paletteManager.SetGame (paletteManager.Load (NULL, NULL, 1, 1, 1));
	LoadD1Textures ();
	if (nLevel == 1) {
		while (!bAbortBriefing && 
				 (nCurBriefingScreen < int32_t (MAX_BRIEFING_SCREENS)) && 
				 (briefingScreens [nCurBriefingScreen].nLevel == ((gameStates.app.bD1Mission && bEnding) ? nLevel : 0))) {
			bAbortBriefing = LoadLevelScreen (nCurBriefingScreen, (int16_t) nLevel);
			nCurBriefingScreen++;
			if (gameStates.app.bD1Mission && bEnding)
				break;
			}
		}
	if (!bAbortBriefing) {
		for (nCurBriefingScreen = 0; nCurBriefingScreen < int32_t (MAX_BRIEFING_SCREENS); nCurBriefingScreen++)
			if (briefingScreens [nCurBriefingScreen].nLevel == nLevel)
				if (LoadLevelScreen (nCurBriefingScreen, (int16_t) nLevel))
					break;
		}
	}
else
	LoadLevelScreen (nLevel, (int16_t) nLevel);
gameStates.render.bBriefing = 0;
m_info.briefingText.Destroy ();
KeyFlush ();
if (gameStates.app.bNostalgia)
	ogl.SetDrawBuffer (GL_BACK, 0);
gameData.SetStereoOffsetType (nOffsetSave);
backgroundManager.Draw ();
}

//-----------------------------------------------------------------------------

