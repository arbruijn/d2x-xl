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

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "inferno.h"
#include "ipx.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"
#include "joy.h"
#include "slew.h"
#include "args.h"
#include "hogfile.h"
#include "newdemo.h"
#include "timer.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "strutil.h"
#include "reorder.h"
#include "render.h"
#include "light.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#include "soundthreads.h"
#ifdef EDITOR
#	include "editor/editor.h"
#endif

#if DBG

const char *menuBgNames [4][2] = {
	{"menu.pcx", "menub.pcx"},
	{"menuo.pcx", "menuob.pcx"},
	{"menud.pcx", "menud.pcx"},
	{"menub.pcx", "menub.pcx"}
	};

#else

const char *menuBgNames [4][2] = {
	{"\x01menu.pcx", "\x01menub.pcx"},
	{"\x01menuo.pcx", "\x01menuob.pcx"},
	{"\x01menud.pcx", "\x01menud.pcx"},
	{"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

//------------------------------------------------------------------------------

static struct {
	int	nMaxFPS;
	int	nRenderQual;
	int	nTexQual;
	int	nMeshQual;
	int	nWallTransp;
} renderOpts;

static struct {
	int	nMethod;
	int	nHWObjLighting;
	int	nHWHeadlight;
	int	nMaxLightsPerFace;
	int	nMaxLightsPerPass;
	int	nMaxLightsPerObject;
	int	nLightmapQual;
	int	nGunColor;
	int	nObjectLight;
	int	nLightmaps;
} lightOpts;

static struct {
	int	nUse;
	int	nQuality;
	int	nStyle;
	int	nOmega;
} lightningOpts;

static struct {
	int	nSpeedboost;
	int	nFusionRamp;
	int	nOmegaRamp;
	int	nMslTurnSpeed;
	int	nMslStartSpeed;
	int	nSlomoSpeedup;
	int	nDrag;
} physOpts;

static struct {
	int	nDifficulty;
	int	nSpawnDelay;
	int	nSmokeGrens;
	int	nMaxSmokeGrens;
	int	nAIAggressivity;
	int	nHeadlightAvailable;
} gplayOpts;

static struct {
	int	nUse;
	int	nPlayer;
	int	nRobots;
	int	nMissiles;
	int	nDebris;
	int	nBubbles;
	int	nDensity [5];
	int	nLife [5];
	int	nSize [5];
	int	nAlpha [5];
	int	nSyncSizes;
	int	nQuality;
} smokeOpts;

static struct {
	int	nUse;
	int	nReach;
	int	nMaxLights;
#if DBG
	int	nZPass;
	int	nVolume;
	int	nTest;
#endif
} shadowOpts;

static struct {
	int	nUse;
	int	nSpeed;
	int	nFPS;
} camOpts;

static struct {
	int	nCoronas;
	int	nCoronaStyle;
	int	nShotCoronas;
	int	nWeaponCoronas;
	int	nPowerupCoronas;
	int	nAdditiveCoronas;
	int	nAdditiveObjCoronas;
	int	nCoronaIntensity;
	int	nObjCoronaIntensity;
	int	nLightTrails;
	int	nExplShrapnels;
	int	nSparks;
} effectOpts;

static struct {
	int	nOptTextured;
	int	nOptRadar;
	int	nOptRadarRange;
} automapOpts;

static struct {
	int	nDigiVol;
	int	nMusicVol;
	int	nRedbook;
	int	nChannels;
	int	nVolume;
	int	nGatling;
} soundOpts;

static struct {
	int	nDlTimeout;
	int	nAutoDl;
	int	nExpertMode;
	int	nScreenshots;
} miscOpts;

static struct {
	int	nUseCompSpeed;
	int	nCompSpeed;
} performanceOpts;

static struct {
	int	nWeapons;
	int	nColor;
} shipRenderOpts;

static int fpsTable [16] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 250};

static int nMaxLightsPerFaceTable [] = {3,4,5,6,7,8,12,16,20,24,32};

static const char *pszTexQual [4];
static const char *pszMeshQual [5];
static const char *pszLMapQual [5];
static const char *pszRendQual [5];
static const char *pszSmokeAmount [5];
static const char *pszSmokeSize [4];
static const char *pszSmokeLife [3];
static const char *pszSmokeQual [3];
static const char *pszSmokeAlpha [5];

#if DBG_SHADOWS
extern int	bZPass, bFrontCap, bRearCap, bFrontFaces, bBackFaces, bShadowVolume, bShadowTest, 
				bWallShadows, bSWCulling;
#endif

//------------------------------------------------------------------------------

void SoundMenu (void);
void MiscellaneousMenu (void);

// Function Prototypes added after LINTING
int ExecMainMenuOption (int nChoice);
int ExecMultiMenuOption (int nChoice);
void CustomDetailsMenu (void);
void NewGameMenu (void);
void MultiplayerMenu (void);
void IpxSetDriver (int ipx_driver);

//returns the number of demo files on the disk
int NDCountDemos (void);

// ------------------------------------------------------------------------

int AutoDemoMenuCheck (int nitems, tMenuItem * items, int *nLastKey, int nCurItem)
{
	int curtime;

PrintVersionInfo ();
// Don't allow them to hit ESC in the main menu.
if (*nLastKey == KEY_ESC) 
	*nLastKey = 0;
if (gameStates.app.bAutoDemos) {
	curtime = TimerGetApproxSeconds ();
	if (((gameStates.input.keys.xLastPressTime + I2X (/*2*/5)) < curtime)
#if DBG
		&& !gameData.speedtest.bOn
#endif	
		) {
		int nDemos = NDCountDemos ();

try_again:;

		if ((d_rand () % (nDemos+1)) == 0) {
				gameStates.video.nScreenMode = -1;
				PlayIntroMovie ();
				SongsPlaySong (SONG_TITLE, 1);
				*nLastKey = -3; //exit menu to force rebuild even if not going to game mode. -3 tells menu system not to restore
				SetScreenMode (SCREEN_MENU);
			}
		else {
			gameStates.input.keys.xLastPressTime = curtime;                  // Reset timer so that disk won't thrash if no demos.
			NDStartPlayback (NULL);           // Randomly pick a file
			if (gameData.demo.nState == ND_STATE_PLAYBACK) {
				SetFunctionMode (FMODE_GAME);
				*nLastKey = -3; //exit menu to get into game mode. -3 tells menu system not to restore
				}
			else
				goto try_again;	//keep trying until we get a demo that works
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------
//static int FirstTime = 1;

static struct {
	int	nNew;
	int	nSingle;
	int	nLoad;
	int	nLoadDirect;
	int	nMulti;
	int	nConfig;
	int	nPilots;
	int	nDemo;
	int	nScores;
	int	nMovies;
	int	nSongs;
	int	nCredits;
	int	nQuit;
	int	nOrder;
	int	nHelp;
	int	nChoice;
} mainOpts;

static struct {
	int	nStartIpx;
	int	nJoinIpx;
	int	nStartUdp;
	int	nJoinUdp;
	int	nStartUdpTracker;
	int	nJoinUdpTracker;
	int	nStartMCast4;
	int	nJoinMCast4;
	int	nStartKali;
	int	nJoinKali;
	int	nSerial;
} multiOpts = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1};

//      Create the main menu.
int CreateMainMenu (tMenuItem *m)
{
	int nOptions = 0;

memset (&mainOpts, 0xff, sizeof (mainOpts));
#ifndef DEMO_ONLY
SetScreenMode (SCREEN_MENU);
#if 1
ADD_TEXT (nOptions, "", 0);
m [nOptions++].noscroll = 1;
ADD_TEXT (nOptions, "", 0);
m [nOptions++].noscroll = 1;
ADD_TEXT (nOptions, "", 0);
m [nOptions++].noscroll = 1;
#endif
ADD_MENU (nOptions, TXT_NEW_GAME1, KEY_N, HTX_MAIN_NEW);
mainOpts.nNew = nOptions++;
if (!gameStates.app.bNostalgia) {
	ADD_MENU (nOptions, TXT_NEW_SPGAME, KEY_I, HTX_MAIN_SINGLE);
	mainOpts.nSingle = nOptions++;
	}
ADD_MENU (nOptions, TXT_LOAD_GAME, KEY_L, HTX_MAIN_LOAD);
mainOpts.nLoad = nOptions++;
ADD_MENU (nOptions, TXT_MULTIPLAYER_, KEY_M, HTX_MAIN_MULTI);
mainOpts.nMulti = nOptions++;
if (gameStates.app.bNostalgia)
	ADD_MENU (nOptions, TXT_OPTIONS_, KEY_O, HTX_MAIN_CONF);
else
	ADD_MENU (nOptions, TXT_CONFIGURE, KEY_O, HTX_MAIN_CONF);
mainOpts.nConfig = nOptions++;
ADD_MENU (nOptions, TXT_CHANGE_PILOTS, KEY_P, HTX_MAIN_PILOT);
mainOpts.nPilots = nOptions++;
ADD_MENU (nOptions, TXT_VIEW_DEMO, KEY_D, HTX_MAIN_DEMO);
mainOpts.nDemo = nOptions++;
ADD_MENU (nOptions, TXT_VIEW_SCORES, KEY_H, HTX_MAIN_SCORES);
mainOpts.nScores = nOptions++;
if (CFile::Exist ("orderd2.pcx", gameFolders.szDataDir, 0)) { // SHAREWARE
	ADD_MENU (nOptions, TXT_ORDERING_INFO, -1, NULL);
	mainOpts.nOrder = nOptions++;
	}
ADD_MENU (nOptions, TXT_PLAY_MOVIES, KEY_V, HTX_MAIN_MOVIES);
mainOpts.nMovies = nOptions++;
if (!gameStates.app.bNostalgia) {
	ADD_MENU (nOptions, TXT_PLAY_SONGS, KEY_S, HTX_MAIN_SONGS);
	mainOpts.nSongs = nOptions++;
	}
ADD_MENU (nOptions, TXT_CREDITS, KEY_C, HTX_MAIN_CREDITS);
mainOpts.nCredits = nOptions++;
#endif
ADD_MENU (nOptions, TXT_QUIT, KEY_Q, HTX_MAIN_QUIT);
mainOpts.nQuit = nOptions++;
#if 0
if (!IsMultiGame) {
	ADD_MENU ("  Load level...", MENU_LOAD_LEVEL , KEY_N);
#	ifdef EDITOR
	ADD_MENU (nOptions, "  Editor", KEY_E, NULL);
	nMenuChoice [nOptions++] = MENU_EDITOR;
#	endif
}
#endif
return nOptions;
}

//------------------------------------------------------------------------------
//returns number of item chosen
int MainMenu (void) 
{
	tMenuItem m [25];
	int i, nChoice = 0, nOptions = 0;

IpxClose ();
memset (m, 0, sizeof (m));
//paletteManager.Load (MENU_PALETTE, NULL, 0, 1, 0);		//get correct palette

if (!LOCALPLAYER.callsign [0]) {
	SelectPlayer ();
	return 0;
	}
if ((gameData.app.nGameMode & GM_SERIAL) || (gameData.app.nGameMode & GM_MODEM)) {
	ExecMultiMenuOption (multiOpts.nSerial);
	return 0;
	}
if (gameData.multiplayer.autoNG.bValid) {
	ExecMultiMenuOption (mainOpts.nMulti);
	return 0;
	}
PrintLog ("launching main menu\n");
do {
	nOptions = CreateMainMenu (m); // may have to change, eg, maybe selected pilot and no save games.
	gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds ();                // .. 20 seconds from now!
	if (nChoice < 0)
		nChoice = 0;
	gameStates.menus.bDrawCopyright = 1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	i = ExecMenu2 ("", NULL, nOptions, m, AutoDemoMenuCheck, &nChoice, MENU_PCX_NAME ());
	if (gameStates.app.bNostalgia)
		gameOpts->app.nVersionFilter = 3;
	WritePlayerFile ();
	if (i > -1)
		ExecMainMenuOption (nChoice);
} while (gameStates.app.nFunctionMode == FMODE_MENU);
if (gameStates.app.nFunctionMode == FMODE_GAME)
	paletteManager.FadeOut ();
FlushInput ();
StopPlayerMovement ();
return mainOpts.nChoice;
}

//------------------------------------------------------------------------------

static void PlayMenuMovie (void)
{
	int h, i, j;
	char **m, *ps;

i = GetNumMovieLibs ();
for (h = j = 0; j < i; j++)
	if (j != 2)	//skip robot movies
		h += GetNumMovies (j);
if (!h)
	return;
if (!(m = reinterpret_cast<char **> (D2_ALLOC (h * sizeof (char **)))))
	return;
for (i = j = 0; i < h; i++)
	if ((ps = CycleThroughMovies (i == 0, 0))) {
		if (j && !strcmp (ps, m [0]))
			break;
		m [j++] = ps;
		}
i = ExecMenuListBox (TXT_SELECT_MOVIE, j, m, 1, NULL);
if (i > -1) {
	SDL_ShowCursor (0);
	if (strstr (m [i], "intro"))
		InitSubTitles ("intro.tex");
	else if (strstr (m [i], ENDMOVIE))
		InitSubTitles (ENDMOVIE ".tex");
	PlayMovie (m [i], 1, 1, gameOpts->movies.bResize);
	SDL_ShowCursor (1);
	}
D2_FREE (m);
SongsPlayCurrentSong (1);
}

//------------------------------------------------------------------------------

static void PlayMenuSong (void)
{
	int h, i, j = 0;
	char * m [MAX_NUM_SONGS + 2];
	CFile cf;
	char	szSongTitles [2][14] = {"- Descent 2 -", "- Descent 1 -"};

m [j++] = szSongTitles [0];
for (i = 0; i < gameData.songs.nTotalSongs; i++) {
	if (cf.Open (reinterpret_cast<char*> (gameData.songs.info [i].filename), gameFolders.szDataDir, "rb", i >= gameData.songs.nSongs [0])) {
		cf.Close ();
		if (i == gameData.songs.nSongs [0])
			m [j++] = szSongTitles [1];
		m [j++] = gameData.songs.info [i].filename;
		}
	}
for (;;) {
	h = ExecMenuListBox (TXT_SELECT_SONG, j, m, 1, NULL);
	if (h < 0)
		return;
	if (!strstr (m [h], ".hmp"))
		continue;
	for (i = 0; i < gameData.songs.nTotalSongs; i++)
		if (gameData.songs.info [i].filename == m [h]) {
			SongsPlaySong (i, 0);
			return;
			}
	}
}

//------------------------------------------------------------------------------

int ExecMultiMenuOption (int nChoice)
{
	int	bUDP = 0, bStart = 0;

gameStates.multi.bUseTracker = 0;
if ((nChoice == multiOpts.nStartUdpTracker) ||(nChoice == multiOpts.nJoinUdpTracker)) {
	if (gameStates.app.bNostalgia > 1)
		return 0;
	gameStates.multi.bUseTracker = 1;
	bUDP = 1;
	bStart = (nChoice == multiOpts.nStartUdpTracker);
	}
else if ((nChoice == multiOpts.nStartUdp) || (nChoice == multiOpts.nJoinUdp)) {
	if (gameStates.app.bNostalgia > 1)
		return 0;
	bUDP = 1;
	bStart = (nChoice == multiOpts.nStartUdp);
	}
else if ((nChoice == multiOpts.nStartIpx) || (nChoice == multiOpts.nStartKali) || (nChoice == multiOpts.nStartMCast4))
	bStart = 1;
else if ((nChoice != multiOpts.nJoinIpx) && (nChoice != multiOpts.nJoinKali) &&	(nChoice != multiOpts.nJoinMCast4))
	return 0;
gameOpts->app.bSinglePlayer = 0;
LoadMission (gameData.missions.nLastMission);
if (bUDP && !(bStart || InitAutoNetGame () || NetworkGetIpAddr ()))
	return 0;
gameStates.multi.bServer = (nChoice & 1) == 0;
gameStates.app.bHaveExtraGameInfo [1] = gameStates.multi.bServer;
if (bUDP) {
	gameStates.multi.nGameType = UDP_GAME;
	IpxSetDriver (IPX_DRIVER_UDP); 
	if (nChoice == multiOpts.nStartUdpTracker) {
		int n = ActiveTrackerCount (1);
		if (n < -2) {
			if (n == -4)
				ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_NO_TRACKERS);
			gameStates.multi.bUseTracker = 0;
			return 0;
			}
		}
	}
else if ((nChoice == multiOpts.nStartIpx) || (nChoice == multiOpts.nJoinIpx)) {
	gameStates.multi.nGameType = IPX_GAME;
	IpxSetDriver (IPX_DRIVER_IPX); 
	}
else if ((nChoice == multiOpts.nStartKali) || (nChoice == multiOpts.nJoinKali)) {
	gameStates.multi.nGameType = IPX_GAME;
	IpxSetDriver (IPX_DRIVER_KALI); 
	}
else if ((nChoice == multiOpts.nStartMCast4) || (nChoice == multiOpts.nJoinMCast4)) {
	gameStates.multi.nGameType = IPX_GAME;
	IpxSetDriver (IPX_DRIVER_MCAST4); 
	}
if (bStart ? NetworkStartGame () : NetworkBrowseGames ())
	return 1;
IpxClose ();
return 0;
}

//------------------------------------------------------------------------------

void ShowOrderForm (void);      // John didn't want this in inferno[HA] so I just externed it.

//returns flag, true means quit menu
int ExecMainMenuOption (int nChoice) 
{
if (nChoice == mainOpts.nNew) {
	gameOpts->app.bSinglePlayer = 0;
	NewGameMenu ();
	}
else if (nChoice == mainOpts.nSingle) {
	gameOpts->app.bSinglePlayer = 1;
	NewGameMenu ();
	}
else if (nChoice == mainOpts.nLoad) {
	if (!saveGameHandler.Load (0, 0, 0, NULL))
		SetFunctionMode (FMODE_MENU);
	}
#if DBG
else if (nChoice == mainOpts.nLoadDirect) {
	tMenuItem	m [1];
	char			szLevel [10] = "";
	int			nLevel;

	ADD_INPUT (0, szLevel, sizeof (szLevel), NULL);
	ExecMenu (NULL, "Enter level to load", 1, m, NULL, NULL);
	nLevel = atoi (m->text);
	if (nLevel && (nLevel >= gameData.missions.nLastSecretLevel) && (nLevel <= gameData.missions.nLastLevel)) {
		paletteManager.FadeOut ();
		StartNewGame (nLevel);
		}
	}
#endif
else if (nChoice == mainOpts.nMulti) {
	MultiplayerMenu ();
	}
else if (nChoice == mainOpts.nConfig) {
	ConfigMenu ();
	}
else if (nChoice == mainOpts.nPilots) {
	gameStates.gfx.bOverride = 0;
	SelectPlayer ();
	}
else if (nChoice == mainOpts.nDemo) {
	char demoPath [FILENAME_LEN], demoFile [FILENAME_LEN];

	sprintf (demoPath, "%s%s*.dem", gameFolders.szDemoDir, *gameFolders.szDemoDir ? "/" : ""); 
	if (ExecMenuFileSelector (TXT_SELECT_DEMO, demoPath, demoFile, 1))
		NDStartPlayback (demoFile);
	}
else if (nChoice == mainOpts.nScores) {
	paletteManager.FadeOut ();
	ScoresView (-1);
	}
else if (nChoice == mainOpts.nMovies) {
	PlayMenuMovie ();
	}
else if (nChoice == mainOpts.nSongs) {
	PlayMenuSong ();
	}
else if (nChoice == mainOpts.nCredits) {
	paletteManager.FadeOut ();
	SongsStopAll ();
	ShowCredits (NULL); 
	}
#ifdef EDITOR
else if (nChoice == mainOpts.nEditor) {
	SetFunctionMode (FMODE_EDITOR);
	InitCockpit ();
	}
#endif
else if (nChoice == mainOpts.nHelp) {
	DoShowHelp ();
	}
else if (nChoice == mainOpts.nQuit) {
#ifdef EDITOR
	if (SafetyCheck ()) {
#endif
	paletteManager.FadeOut ();
	SetFunctionMode (FMODE_EXIT);
#ifdef EDITOR
	}
#endif
	}
else if (nChoice == mainOpts.nOrder) {
	ShowOrderForm ();
	}
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int DifficultyMenu ()
{
	int i, choice = gameStates.app.nDifficultyLevel;
	tMenuItem m [5];

memset (m, 0, sizeof (m));
for (i = 0; i < 5; i++)
	ADD_MENU (i, MENU_DIFFICULTY_TEXT (i), 0, "");
i = ExecMenu1 (NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, &choice);
if (i <= -1)
	return 0;
if (choice != gameStates.app.nDifficultyLevel) {       
	gameStates.app.nDifficultyLevel = choice;
	WritePlayerFile ();
	}
return 1;
}

//------------------------------------------------------------------------------

typedef struct tDetailData {
	ubyte		renderDepths [NUM_DETAIL_LEVELS - 1];
	sbyte		maxPerspectiveDepths [NUM_DETAIL_LEVELS - 1];
	sbyte		maxLinearDepths [NUM_DETAIL_LEVELS - 1];
	sbyte		maxLinearDepthObjects [NUM_DETAIL_LEVELS - 1];
	sbyte		maxDebrisObjects [NUM_DETAIL_LEVELS - 1];
	sbyte		maxObjsOnScreenDetailed [NUM_DETAIL_LEVELS - 1];
	sbyte		simpleModelThresholdScales [NUM_DETAIL_LEVELS - 1];
	sbyte		nSoundChannels [NUM_DETAIL_LEVELS - 1];
} tDetailData;

tDetailData	detailData = {
	{15, 31, 63, 127, 255},
	{ 1,  2,  3,   5,   8},
	{ 3,  5,  7,  10,  50},
	{ 1,  2,  3,   7,  20},
	{ 2,  4,  7,  10,  15},
	{ 2,  4,  7,  10,  15},
	{ 2,  4,  8,  16,  50},
	{ 2,  8, 16,  32,  64}};


//      -----------------------------------------------------------------------------
//      Set detail level based stuff.
//      Note: Highest detail level (gameStates.render.detail.nLevel == NUM_DETAIL_LEVELS-1) is custom detail level.
void InitDetailLevels (int nDetailLevel)
{
	Assert ((nDetailLevel >= 0) && (nDetailLevel < NUM_DETAIL_LEVELS));

if (nDetailLevel < NUM_DETAIL_LEVELS - 1) {
	gameStates.render.detail.nRenderDepth = detailData.renderDepths [nDetailLevel];
	gameStates.render.detail.nMaxPerspectiveDepth = detailData.maxPerspectiveDepths [nDetailLevel];
	gameStates.render.detail.nMaxLinearDepth = detailData.maxLinearDepths [nDetailLevel];
	gameStates.render.detail.nMaxLinearDepthObjects = detailData.maxLinearDepthObjects [nDetailLevel];
	gameStates.render.detail.nMaxDebrisObjects = detailData.maxDebrisObjects [nDetailLevel];
	gameStates.render.detail.nMaxObjectsOnScreenDetailed = detailData.maxObjsOnScreenDetailed [nDetailLevel];
	gameData.models.nSimpleModelThresholdScale = detailData.simpleModelThresholdScales [nDetailLevel];
	DigiSetMaxChannels (detailData.nSoundChannels [nDetailLevel]);
	//      Set custom menu defaults.
	gameStates.render.detail.nObjectComplexity = nDetailLevel;
	gameStates.render.detail.nWallRenderDepth = nDetailLevel;
	gameStates.render.detail.nObjectDetail = nDetailLevel;
	gameStates.render.detail.nWallDetail = nDetailLevel;
	gameStates.render.detail.nDebrisAmount = nDetailLevel;
	gameStates.sound.nSoundChannels = nDetailLevel;
	gameStates.render.detail.nLevel = nDetailLevel;
	}
}

//      -----------------------------------------------------------------------------

void DetailLevelMenu (void)
{
	int i, choice = gameStates.app.nDetailLevel;
	tMenuItem m [8];

#if 1
	char szMenuDetails [5][20];

memset (m, 0, sizeof (m));
for (i = 0; i < 5; i++) {
	sprintf (szMenuDetails [i], "%d. %s", i + 1, MENU_DETAIL_TEXT (i));
	ADD_MENU (i, szMenuDetails [i], 0, HTX_ONLINE_MANUAL);
	}
#else
memset (m, 0, sizeof (m));
for (i = 0; i < 5; i++)
	ADD_MENU (i, MENU_DETAIL_TEXT (i), 0, HTX_ONLINE_MANUAL);
#endif
ADD_TEXT (5, "", 0);
ADD_MENU (6, MENU_DETAIL_TEXT (5), KEY_C, HTX_ONLINE_MANUAL);
ADD_CHECK (7, TXT_HIRES_MOVIES, gameOpts->movies.bHires, KEY_S, HTX_ONLINE_MANUAL);
i = ExecMenu1 (NULL, TXT_DETAIL_LEVEL, NDL + 3, m, NULL, &choice);
if (i > -1) {
	switch (choice) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			gameStates.app.nDetailLevel = choice;
			InitDetailLevels (gameStates.app.nDetailLevel);
			break;
		case 6:
			gameStates.app.nDetailLevel = 5;
			CustomDetailsMenu ();
			break;
		}
	}
gameOpts->movies.bHires = m [7].value;
}

//      -----------------------------------------------------------------------------

int CustomDetailsCallback (int nitems, tMenuItem * items, int *nLastKey, int nCurItem)
{
	nitems = nitems;
	*nLastKey = *nLastKey;
	nCurItem = nCurItem;

gameStates.render.detail.nObjectComplexity = items [0].value;
gameStates.render.detail.nObjectDetail = items [1].value;
gameStates.render.detail.nWallDetail = items [2].value;
gameStates.render.detail.nWallRenderDepth = items [3].value;
gameStates.render.detail.nDebrisAmount = items [4].value;
if (!gameStates.app.bGameRunning)
	gameStates.sound.nSoundChannels = items [5].value;
return nCurItem;
}

// -----------------------------------------------------------------------------

void SetMaxCustomDetails (void)
{
gameStates.render.detail.nRenderDepth = detailData.renderDepths [NDL - 1];
gameStates.render.detail.nMaxPerspectiveDepth = detailData.maxPerspectiveDepths [NDL - 1];
gameStates.render.detail.nMaxLinearDepth = detailData.maxLinearDepths [NDL - 1];
gameStates.render.detail.nMaxDebrisObjects = detailData.maxDebrisObjects [NDL - 1];
gameStates.render.detail.nMaxObjectsOnScreenDetailed = detailData.maxObjsOnScreenDetailed [NDL - 1];
gameData.models.nSimpleModelThresholdScale = detailData.simpleModelThresholdScales [NDL - 1];
gameStates.render.detail.nMaxLinearDepthObjects = detailData.maxLinearDepthObjects [NDL - 1];
}

// -----------------------------------------------------------------------------

void InitCustomDetails (void)
{
gameStates.render.detail.nRenderDepth = detailData.renderDepths [gameStates.render.detail.nWallRenderDepth];
gameStates.render.detail.nMaxPerspectiveDepth = detailData.maxPerspectiveDepths [gameStates.render.detail.nWallDetail];
gameStates.render.detail.nMaxLinearDepth = detailData.maxLinearDepths [gameStates.render.detail.nWallDetail];
gameStates.render.detail.nMaxDebrisObjects = detailData.maxDebrisObjects [gameStates.render.detail.nDebrisAmount];
gameStates.render.detail.nMaxObjectsOnScreenDetailed = detailData.maxObjsOnScreenDetailed [gameStates.render.detail.nObjectComplexity];
gameData.models.nSimpleModelThresholdScale = detailData.simpleModelThresholdScales [gameStates.render.detail.nObjectComplexity];
gameStates.render.detail.nMaxLinearDepthObjects = detailData.maxLinearDepthObjects [gameStates.render.detail.nObjectDetail];
DigiSetMaxChannels (detailData.nSoundChannels [gameStates.sound.nSoundChannels]);
}

#define	DL_MAX	10

// -----------------------------------------------------------------------------

void CustomDetailsMenu (void)
{
	int	nOptions;
	int	i, choice = 0;
	tMenuItem m [DL_MAX];

do {
	nOptions = 0;
	memset (m, 0, sizeof (m));
	ADD_SLIDER (nOptions, TXT_OBJ_COMPLEXITY, gameStates.render.detail.nObjectComplexity, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	ADD_SLIDER (nOptions, TXT_OBJ_DETAIL, gameStates.render.detail.nObjectDetail, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	ADD_SLIDER (nOptions, TXT_WALL_DETAIL, gameStates.render.detail.nWallDetail, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	ADD_SLIDER (nOptions, TXT_WALL_RENDER_DEPTH, gameStates.render.detail.nWallRenderDepth, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	ADD_SLIDER (nOptions, TXT_DEBRIS_AMOUNT, gameStates.render.detail.nDebrisAmount, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	nOptions++;
	if (!gameStates.app.bGameRunning) {
		ADD_SLIDER (nOptions, TXT_SOUND_CHANNELS, gameStates.sound.nSoundChannels, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
		nOptions++;
		}
	ADD_TEXT (nOptions, TXT_LO_HI, 0);
	nOptions++;
	Assert (sizeofa (m) >= nOptions);
	i = ExecMenu1 (NULL, TXT_DETAIL_CUSTOM, nOptions, m, CustomDetailsCallback, &choice);
} while (i > -1);
InitCustomDetails ();
}

//------------------------------------------------------------------------------

void UseDefaultPerformanceSettings (void)
{
if (gameStates.app.bNostalgia || !gameStates.app.bUseDefaults)
	return;
	
SetMaxCustomDetails ();
gameOpts->render.nMaxFPS = 60;
gameOpts->render.effects.bTransparent = 1;
gameOpts->render.color.nLightmapRange = 5;
gameOpts->render.color.bMix = 1;
gameOpts->render.color.bWalls = 1;
gameOpts->render.cameras.bFitToWall = 0;
gameOpts->render.cameras.nSpeed = 5000;
gameOpts->ogl.bSetGammaRamp = 0;
gameStates.ogl.nContrast = 8;
if (gameStates.app.nCompSpeed == 0) {
	gameOpts->render.bUseLightmaps = 0;
	gameOpts->render.nQuality = 1;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 0;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	gameOpts->render.effects.nShrapnels = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->ogl.nMaxLightsPerFace = 4;
	gameOpts->ogl.nMaxLightsPerPass = 4;
	gameOpts->ogl.nMaxLightsPerObject = 4;
	extraGameInfo [0].bShadows = 0;
	extraGameInfo [0].bUseParticles = 0;
	extraGameInfo [0].bUseLightnings = 0;
	extraGameInfo [0].bUseCameras = 0;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 1) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 0;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 1;
	gameOpts->render.particles.bStatic = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 0;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 8;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 5;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 2) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 0;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 2;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 1;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 1;
	gameOpts->render.lightnings.bPlasma = 0;
	gameOpts->render.lightnings.bPlayers = 0;
	gameOpts->render.lightnings.bRobots = 0;
	gameOpts->render.lightnings.bDamage = 0;
	gameOpts->render.lightnings.bExplosions = 0;
	gameOpts->render.lightnings.bStatic = 0;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 12;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 3) {
	gameOpts->render.nQuality = 3;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 1;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 3;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 2;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 0;
	gameOpts->render.particles.bMonitors = 0;
	gameOpts->render.lightnings.bMonitors = 0;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 1;
	gameOpts->render.lightnings.bPlasma = 0;
	gameOpts->render.lightnings.bPlayers = 1;
	gameOpts->render.lightnings.bRobots = 1;
	gameOpts->render.lightnings.bDamage = 1;
	gameOpts->render.lightnings.bExplosions = 1;
	gameOpts->render.lightnings.bStatic = 1;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->ogl.nMaxLightsPerFace = 16;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 8;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 1;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 4) {
	gameOpts->render.nQuality = 4;
	extraGameInfo [0].bUseParticles = 1;
	gameOpts->render.particles.bPlayers = 1;
	gameOpts->render.particles.bRobots = 1;
	gameOpts->render.particles.bMissiles = 1;
	gameOpts->render.particles.bCollisions = 1;
	gameOpts->render.effects.nShrapnels = 4;
	gameOpts->render.particles.bStatic = 1;
	gameOpts->render.particles.nDens [0] =
	gameOpts->render.particles.nDens [1] =
	gameOpts->render.particles.nDens [2] =
	gameOpts->render.particles.nDens [3] =
	gameOpts->render.particles.nDens [4] = 3;
	gameOpts->render.particles.nSize [0] =
	gameOpts->render.particles.nSize [1] =
	gameOpts->render.particles.nSize [2] =
	gameOpts->render.particles.nSize [3] =
	gameOpts->render.particles.nSize [4] = 1;
	gameOpts->render.particles.nLife [0] =
	gameOpts->render.particles.nLife [1] =
	gameOpts->render.particles.nLife [2] =
	gameOpts->render.particles.nLife [4] = 0;
	gameOpts->render.particles.nLife [3] = 1;
	gameOpts->render.particles.nAlpha [0] =
	gameOpts->render.particles.nAlpha [1] =
	gameOpts->render.particles.nAlpha [2] =
	gameOpts->render.particles.nAlpha [3] =
	gameOpts->render.particles.nAlpha [4] = 0;
	gameOpts->render.particles.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.particles.bAuxViews = 1;
	gameOpts->render.particles.bMonitors = 1;
	gameOpts->render.lightnings.bMonitors = 1;
	gameOpts->render.lightnings.nQuality = 1;
	gameOpts->render.lightnings.nStyle = 2;
	gameOpts->render.lightnings.bPlasma = 1;
	gameOpts->render.lightnings.bPlayers = 1;
	gameOpts->render.lightnings.bRobots = 1;
	gameOpts->render.lightnings.bDamage = 1;
	gameOpts->render.lightnings.bExplosions = 1;
	gameOpts->render.lightnings.bStatic = 1;
	gameOpts->render.lightnings.bOmega = 1;
	gameOpts->render.lightnings.bAuxViews = 1;
	gameOpts->ogl.bLightObjects = 1;
	gameOpts->ogl.nMaxLightsPerFace = 20;
	gameOpts->ogl.nMaxLightsPerPass = 8;
	gameOpts->ogl.nMaxLightsPerObject = 16;
	extraGameInfo [0].bUseCameras = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.cameras.nFPS = 0;
	gameOpts->movies.nQuality = 1;
	gameOpts->movies.bResize = 1;
	gameOpts->render.shadows.nClip = 1;
	gameOpts->render.shadows.nReach = 1;
	extraGameInfo [0].bShadows = 1;
	extraGameInfo [0].bUseParticles = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
}

//      -----------------------------------------------------------------------------

static const char *pszCompSpeeds [5];

int PerformanceSettingsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem * m;
	int			v;

m = menus + performanceOpts.nUseCompSpeed;
v = m->value;
if (gameStates.app.bUseDefaults != v) {
	gameStates.app.bUseDefaults = v;
	*key = -2;
	return nCurItem;
	}
if (gameStates.app.bUseDefaults) {
	m = menus + performanceOpts.nCompSpeed;
	v = m->value;
	if (gameStates.app.nCompSpeed != v) {
		gameStates.app.nCompSpeed = v;
		sprintf (m->text, TXT_COMP_SPEED, pszCompSpeeds [v]);
		}
	m->rebuild = 1;
	}
return nCurItem;
}

//      -----------------------------------------------------------------------------

void PerformanceSettingsMenu (void)
{
	int		i, nOptions, choice = gameStates.app.nDetailLevel;
	char		szCompSpeed [50];
	tMenuItem m [10];

pszCompSpeeds [0] = TXT_VERY_SLOW;
pszCompSpeeds [1] = TXT_SLOW;
pszCompSpeeds [2] = TXT_MEDIUM;
pszCompSpeeds [3] = TXT_FAST;
pszCompSpeeds [4] = TXT_VERY_FAST;

do {
	i = nOptions = 0;
	memset (m, 0, sizeof (m));
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_CHECK (nOptions, TXT_USE_DEFAULTS, gameStates.app.bUseDefaults, KEY_U, HTX_MISC_DEFAULTS);
	performanceOpts.nUseCompSpeed = nOptions++;
	if (gameStates.app.bUseDefaults) {
		sprintf (szCompSpeed + 1, TXT_COMP_SPEED, pszCompSpeeds [gameStates.app.nCompSpeed]);
		*szCompSpeed = *(TXT_COMP_SPEED - 1);
		ADD_SLIDER (nOptions, szCompSpeed + 1, gameStates.app.nCompSpeed, 0, 4, KEY_C, HTX_MISC_COMPSPEED);
		performanceOpts.nCompSpeed = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_SETPERF_MENUTITLE, nOptions, m, PerformanceSettingsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);
UseDefaultPerformanceSettings ();
}

//------------------------------------------------------------------------------

static int wideScreenOpt, optCustRes, nDisplayMode;

static int ScreenResMenuItemToMode (int menuItem)
{
	int j;

if ((wideScreenOpt >= 0) && (menuItem > wideScreenOpt))
	menuItem--;
for (j = 0; j < NUM_DISPLAY_MODES; j++)
	if (displayModeInfo [j].isAvailable)
		if (--menuItem < 0)
			break;
return j;
}

//------------------------------------------------------------------------------

static int ScreenResModeToMenuItem (int mode)
{
	int j;
	int item = 0;

	for (j = 0; j < mode; j++)
		if (displayModeInfo [j].isAvailable)
			item++;

	if ((wideScreenOpt >= 0) && (mode >= wideScreenOpt))
		item++;

	return item;
}

//------------------------------------------------------------------------------

int ScreenResCallback (int nItems, tMenuItem *m, int *nLastKey, int nCurItem)
{
	int	i, j;

if (m [optCustRes].value != (nDisplayMode == NUM_DISPLAY_MODES)) 
	*nLastKey = -2;
for (i = 0; i < optCustRes; i++)
	if (m [i].value) {
		j = ScreenResMenuItemToMode(i);
		if ((j < NUM_DISPLAY_MODES) && (j != nDisplayMode)) {
			SetDisplayMode (j, 0);
			nDisplayMode = gameStates.video.nDisplayMode;
			*nLastKey = -2;
			}
		break;
		}
return nCurItem;
}

//------------------------------------------------------------------------------

int SwitchDisplayMode (int dir)
{
	int	i, h = NUM_DISPLAY_MODES;

for (i = 0; i < h; i++)
	displayModeInfo [i].isAvailable =
		 ((i < 2) || gameStates.menus.bHiresAvailable) && GrVideoModeOK (displayModeInfo [i].VGA_mode);
i = gameStates.video.nDisplayMode;
do {
	i += dir;
	if (i < 0)
		i = 0;
	else if (i >= h)
		i = h - 1;
	if (displayModeInfo [i].isAvailable) {
		SetDisplayMode (i, 0);
		return 1;
		}
	} while (i && (i < h - 1) && (i != gameStates.video.nDisplayMode));
return 0;
}

//------------------------------------------------------------------------------

#if VR_NONE
#   undef VR_NONE			//undef if != 0
#endif
#ifndef VR_NONE
#   define VR_NONE 0		//make sure VR_NONE is defined and 0 here
#endif

#define ADD_RES_OPT(_t) {ADD_RADIO (nOptions, _t, 0, -1, 0, NULL); nOptions++;}
//{m [nOptions].nType = NM_TYPE_RADIO; m [nOptions].text = (_t); m [nOptions].key = -1; m [nOptions].value = 0; nOptions++;}

void ScreenResMenu ()
{
#	define N_SCREENRES_ITEMS (NUM_DISPLAY_MODES + 4)

	tMenuItem	m [N_SCREENRES_ITEMS];
	int				choice;
	int				i, j, key, nOptions = 0, nCustWOpt, nCustHOpt, nCustW, nCustH, bStdRes;
	char				szMode [NUM_DISPLAY_MODES][20];
	char				cShortCut, szCustX [5], szCustY [5];

if ((gameStates.video.nDisplayMode == -1) || (gameStates.render.vr.nRenderMode != VR_NONE)) {				//special VR mode
	ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, 
			"You may not change screen\nresolution when VR modes enabled.");
	return;
	}
nDisplayMode = gameStates.video.nDisplayMode;
do {
	wideScreenOpt = -1;
	cShortCut = '1';
	nOptions = 0;
	memset (m, 0, sizeof (m));
	for (i = 0, j = NUM_DISPLAY_MODES; i < j; i++) {
		if (!(displayModeInfo [i].isAvailable = 
				 ((i < 2) || gameStates.menus.bHiresAvailable) && GrVideoModeOK (displayModeInfo [i].VGA_mode)))
				continue;
		if (displayModeInfo [i].isWideScreen && !displayModeInfo [i-1].isWideScreen) {
			ADD_TEXT (nOptions, TXT_WIDESCREEN_RES, 0);
			if (wideScreenOpt < 0)
				wideScreenOpt = nOptions;
			nOptions++;
			}
		sprintf (szMode [i], "%c. %dx%d", cShortCut, displayModeInfo [i].w, displayModeInfo [i].h);
		if (cShortCut == '9')
			cShortCut = 'A';
		else
			cShortCut++;
		ADD_RES_OPT (szMode [i]);
		}
	ADD_RADIO (nOptions, TXT_CUSTOM_SCRRES, 0, KEY_U, 0, HTX_CUSTOM_SCRRES);
	optCustRes = nOptions++;
	*szCustX = *szCustY = '\0';
	if (displayModeInfo [NUM_DISPLAY_MODES].w)
		sprintf (szCustX, "%d", displayModeInfo [NUM_DISPLAY_MODES].w);
	else
		*szCustX = '\0';
	if (displayModeInfo [NUM_DISPLAY_MODES].h)
		sprintf (szCustY, "%d", displayModeInfo [NUM_DISPLAY_MODES].h);
	else
		*szCustY = '\0';
	//if (nDisplayMode == NUM_DISPLAY_MODES) 
		{
		ADD_INPUT (nOptions, szCustX, 4, NULL);
		nCustWOpt = nOptions++;
		ADD_INPUT (nOptions, szCustY, 4, NULL);
		nCustHOpt = nOptions++;
		}
/*
	else
		nCustWOpt =
		nCustHOpt = -1;
*/
	choice = ScreenResModeToMenuItem(nDisplayMode);
	m [choice].value = 1;

	key = ExecMenu1 (NULL, TXT_SELECT_SCRMODE, nOptions, m, ScreenResCallback, &choice);
	if (key == -1)
		return;
	bStdRes = 0;
	if (m [optCustRes].value) {
		key = -2;
		nDisplayMode = NUM_DISPLAY_MODES;
		if ((nCustWOpt > 0) && (nCustHOpt > 0) &&
			 (0 < (nCustW = atoi (szCustX))) && (0 < (nCustH = atoi (szCustY)))) {
			i = NUM_DISPLAY_MODES;
			if (SetCustomDisplayMode (nCustW, nCustH))
				key = 0;
			else
				ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_ERROR_SCRMODE);
			}
		else
			continue;
		}
	else {
		for (i = 0; i <= optCustRes; i++)
			if (m [i].value) {
				bStdRes = 1;
				i = ScreenResMenuItemToMode(i);
				break;
				}
			if (!bStdRes)
				continue;
		}
	if (((i > 1) && !gameStates.menus.bHiresAvailable) || !GrVideoModeOK (displayModeInfo [i].VGA_mode)) {
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_ERROR_SCRMODE);
		return;
		}
	if (i == gameStates.video.nDisplayMode) {
		SetDisplayMode (i, 0);
		SetScreenMode (SCREEN_MENU);
		if (bStdRes)
			return;
		}
	}
	while (key == -2);
}

//------------------------------------------------------------------------------

int SelectAndLoadMission (int bMulti, int *bAnarchyOnly)
{
	int	i, j, nMissions, nDefaultMission, nNewMission = -1;
	char	*szMsnNames [MAX_MISSIONS];

	static const char *menuTitles [4];

	menuTitles [0] = TXT_NEW_GAME;
	menuTitles [1] = TXT_NEW_D1GAME;
	menuTitles [2] = TXT_NEW_D2GAME;
	menuTitles [3] = TXT_NEW_GAME;

if (bAnarchyOnly)
	*bAnarchyOnly = 0;
do {
	nMissions = BuildMissionList (1, nNewMission);
	if (nMissions < 1)
		return -1;
	nDefaultMission = 0;
	for (i = 0; i < nMissions; i++) {

		szMsnNames [i] = gameData.missions.list [i].szMissionName;
		j = MsnHasGameVer (szMsnNames [i]) ? 4 : 0;
		if (!stricmp (szMsnNames [i] + j, gameConfig.szLastMission))
			nDefaultMission = i;
		}
	gameStates.app.nExtGameStatus = bMulti ? GAMESTAT_START_MULTIPLAYER_MISSION : GAMESTAT_SELECT_MISSION;
	nNewMission = ExecMenuListBox1 (bMulti ? TXT_MULTI_MISSION : menuTitles [gameOpts->app.nVersionFilter], 
											  nMissions, szMsnNames, 1, nDefaultMission, NULL);
	GameFlushInputs ();
	if (nNewMission == -1)
		return -1;      //abort!
	} while (!gameData.missions.list [nNewMission].nDescentVersion);
strcpy (gameConfig.szLastMission, szMsnNames [nNewMission]);
if (!LoadMission (nNewMission)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_MISSION_ERROR);
	return -1;
	}
gameStates.app.bD1Mission = (gameData.missions.list [nNewMission].nDescentVersion == 1);
gameData.missions.nLastMission = nNewMission;
if (bAnarchyOnly)
	*bAnarchyOnly = gameData.missions.list [nNewMission].bAnarchyOnly;
return nNewMission;
}

//------------------------------------------------------------------------------

void LegacyNewGameMenu (void)
{
	int			nNewLevel, nHighestPlayerLevel;
	int			nMissions;
	char			*m [MAX_MISSIONS];
	int			i, choice = 0, nFolder = -1, nDefaultMission = 0;
	static int	nMission = -1;
	static const char	*menuTitles [4];

menuTitles [0] = TXT_NEW_GAME;
menuTitles [1] = TXT_NEW_D1GAME;
menuTitles [2] = TXT_NEW_D2GAME;
menuTitles [3] = TXT_NEW_GAME;

gameStates.app.bD1Mission = 0;
gameStates.app.bD1Data = 0;
SetDataVersion (-1);
if ((nMission < 0) || gameOpts->app.bSinglePlayer)
	gameFolders.szMsnSubDir [0] = '\0';
hogFileManager.UseAlt ("");
do {
	nMissions = BuildMissionList (0, nFolder);
	if (nMissions < 1)
		return;
	for (i = 0; i < nMissions; i++) {
		m [i] = gameData.missions.list [i].szMissionName;
		if (!stricmp (m [i], gameConfig.szLastMission))
			nDefaultMission = i;
		}
	nMission = ExecMenuListBox1 (menuTitles [gameOpts->app.nVersionFilter], nMissions, m, 1, nDefaultMission, NULL);
	GameFlushInputs ();
	if (nMission == -1)
		return;         //abort!
	nFolder = nMission;
	}
while (!gameData.missions.list [nMission].nDescentVersion);
strcpy (gameConfig.szLastMission, m [nMission]);
if (!LoadMission (nMission)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_ERROR_MSNFILE); 
	return;
}
gameStates.app.bD1Mission = (gameData.missions.list [nMission].nDescentVersion == 1);
gameData.missions.nLastMission = nMission;
nNewLevel = 1;

PrintLog ("   getting highest level allowed to play\n");
nHighestPlayerLevel = GetHighestLevel ();

if (nHighestPlayerLevel > gameData.missions.nLastLevel)
	nHighestPlayerLevel = gameData.missions.nLastLevel;

if (nHighestPlayerLevel > 1) {
	tMenuItem m [4];
	char szInfo [80];
	char szNumber [10];
	int nItems;

try_again:
	sprintf (szInfo, "%s %d", TXT_START_ANY_LEVEL, nHighestPlayerLevel);

	memset (m, 0, sizeof (m));
	ADD_TEXT (0, szInfo, 0);
	ADD_INPUT (1, szNumber, 10, "");
	nItems = 2;

	strcpy (szNumber, "1");
	choice = ExecMenu (NULL, TXT_SELECT_START_LEV, nItems, m, NULL, NULL);
	if ((choice == -1) || !m [1].text [0])
		return;
	nNewLevel = atoi (m [1].text);
	if ((nNewLevel <= 0) || (nNewLevel > nHighestPlayerLevel)) {
		m [0].text = reinterpret_cast<char*> (TXT_ENTER_TO_CONT);
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		goto try_again;
	}
}

WritePlayerFile ();
if (!DifficultyMenu ())
	return;
paletteManager.FadeOut ();
if (!StartNewGame (nNewLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static int nOptVerFilter = -1;

int NewGameMenuCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			i, v;

m = menus + gplayOpts.nDifficulty;
v = m->value;
if (gameStates.app.nDifficultyLevel != v) {
	gameStates.app.nDifficultyLevel = 
	gameStates.app.nDifficultyLevel = v;
	InitGateIntervals ();
	sprintf (m->text, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	m->rebuild = 1;
	}
for (i = 0; i < 3; i++)
	if (menus [nOptVerFilter + i].value) {
		gameOpts->app.nVersionFilter = i + 1;
		break;
		}
return nCurItem;
}

//------------------------------------------------------------------------------

void NewGameMenu ()
{
	tMenuItem		m [15];
	int				nOptions, optSelMsn, optMsnName, optLevelText, optLevel, optLaunch;
	int				nMission = gameData.missions.nLastMission, bMsnLoaded = 0;
	int				i, choice = 0;
	char				szDifficulty [50];
	char				szLevelText [32];
	char				szLevel [5];

	static int		nPlayerMaxLevel = 1;
	static int		nLevel = 1;

if (gameStates.app.bNostalgia) {
	LegacyNewGameMenu ();
	return;
	}
gameStates.app.bD1Mission = 0;
gameStates.app.bD1Data = 0;
SetDataVersion (-1);
if ((nMission < 0) || gameOpts->app.bSinglePlayer)
	gameFolders.szMsnSubDir [0] = '\0';
hogFileManager.UseAlt ("");
for (;;) {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	ADD_MENU (nOptions, TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
	optSelMsn = nOptions++;
	ADD_TEXT (nOptions, (nMission < 0) ? TXT_NONE_SELECTED : gameData.missions.list [nMission].szMissionName, 0);
	optMsnName = nOptions++;
	if ((nMission >= 0) && (nPlayerMaxLevel > 1)) {
#if 0
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
#endif
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, nPlayerMaxLevel);
		Assert (strlen (szLevelText) < 32);
		ADD_TEXT (nOptions, szLevelText, 0); 
		m [nOptions].rebuild = 1;
		optLevelText = nOptions++;
		sprintf (szLevel, "%d", nLevel);
		ADD_INPUT (nOptions, szLevel, 4, HTX_MULTI_LEVEL);
		optLevel = nOptions++;
		}
	else
		optLevel = -1;
	ADD_TEXT (nOptions, "                              ", 0);
	nOptions++;
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	ADD_SLIDER (nOptions, szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	gplayOpts.nDifficulty = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_RADIO (nOptions, TXT_PLAY_D1MISSIONS, 0, KEY_1, 1, HTX_LEVEL_VERSION_FILTER);
	nOptVerFilter = nOptions++;
	ADD_RADIO (nOptions, TXT_PLAY_D2MISSIONS, 0, KEY_2, 1, HTX_LEVEL_VERSION_FILTER);
	nOptions++;
	ADD_RADIO (nOptions, TXT_PLAY_ALL_MISSIONS, 0, KEY_A, 1, HTX_LEVEL_VERSION_FILTER);
	nOptions++;
	m [nOptVerFilter + gameOpts->app.nVersionFilter - 1].value = 1;
	if (nMission >= 0) {
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_MENU (nOptions, TXT_LAUNCH_GAME, KEY_L, "");
		m [nOptions].centered = 1;
		optLaunch = nOptions++;
		}
	else
		optLaunch = -1;

	Assert (sizeofa (m) >= (size_t) nOptions);
	i = ExecMenu1 (NULL, TXT_NEWGAME_MENUTITLE, nOptions, m, &NewGameMenuCallback, &choice);
	if (i < 0) {
		SetFunctionMode (FMODE_MENU);
		return;
		}
	if (choice == optSelMsn) {
		i = SelectAndLoadMission (0, NULL);
		if (i >= 0) {
			bMsnLoaded = 1;
			nMission = i;
			nLevel = 1;
			PrintLog ("   getting highest level allowed to play\n");
			nPlayerMaxLevel = GetHighestLevel ();
			if (nPlayerMaxLevel > gameData.missions.nLastLevel)
				nPlayerMaxLevel = gameData.missions.nLastLevel;
			}
		}
	else if (choice == optLevel) {
		i = atoi (m [optLevel].text);
		if ((i <= 0) || (i > nPlayerMaxLevel))
			ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		else if (nLevel == i)
			break;
		else
			nLevel = i;
		}
	else if (nMission >= 0)
		break;
	}

i = m [gplayOpts.nDifficulty].value;
if (gameStates.app.nDifficultyLevel != i) {
	gameStates.app.nDifficultyLevel = i;
	InitGateIntervals ();
	}
WritePlayerFile ();
if (optLevel > 0)
	nLevel = atoi (m [optLevel].text);
paletteManager.FadeOut ();
if (!bMsnLoaded)
	LoadMission (nMission);
if (!StartNewGame (nLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static int optBrightness = -1;
static int optContrast = -1;

//------------------------------------------------------------------------------

int ConfigMenuCallback (int nitems, tMenuItem * items, int *nLastKey, int nCurItem)
{
if (gameStates.app.bNostalgia) {
	if (nCurItem == optBrightness)
		paletteManager.SetGamma (items [optBrightness].value);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static int	nCWSopt, nCWZopt, optTextGauges, optWeaponIcons, bShowWeaponIcons, 
				optIconAlpha, optTgtInd, optDmgInd, optHitInd, optMslLockInd;

static const char *szCWS [4];

//------------------------------------------------------------------------------

int TgtIndOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v, j;

m = menus + optTgtInd;
v = m->value;
if (v != (extraGameInfo [0].bTargetIndicators == 0)) {
	for (j = 0; j < 3; j++)
		if (m [optTgtInd + j].value) {
			extraGameInfo [0].bTargetIndicators = j;
			break;
			}
	*key = -2;
	return nCurItem;
	}
m = menus + optDmgInd;
v = m->value;
if (v != extraGameInfo [0].bDamageIndicators) {
	extraGameInfo [0].bDamageIndicators = v;
	*key = -2;
	return nCurItem;
	}
m = menus + optMslLockInd;
v = m->value;
if (v != extraGameInfo [0].bMslLockIndicators) {
	extraGameInfo [0].bMslLockIndicators = v;
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void TgtIndOptionsMenu ()
{
	tMenuItem m [15];
	int	i, j, nOptions, choice = 0;
	int	optCloakedInd, optRotateInd;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	ADD_RADIO (nOptions, TXT_TGTIND_NONE, 0, KEY_A, 1, HTX_CPIT_TGTIND);
	optTgtInd = nOptions++;
	ADD_RADIO (nOptions, TXT_TGTIND_SQUARE, 0, KEY_R, 1, HTX_CPIT_TGTIND);
	nOptions++;
	ADD_RADIO (nOptions, TXT_TGTIND_TRIANGLE, 0, KEY_T, 1, HTX_CPIT_TGTIND);
	nOptions++;
	m [optTgtInd + extraGameInfo [0].bTargetIndicators].value = 1;
	if (extraGameInfo [0].bTargetIndicators) {
		ADD_CHECK (nOptions, TXT_CLOAKED_INDICATOR, extraGameInfo [0].bCloakedIndicators, KEY_C, HTX_CLOAKED_INDICATOR);
		optCloakedInd = nOptions++;
		}
	else
		optCloakedInd = -1;
	ADD_CHECK (nOptions, TXT_DMG_INDICATOR, extraGameInfo [0].bDamageIndicators, KEY_D, HTX_CPIT_DMGIND);
	optDmgInd = nOptions++;
	if (extraGameInfo [0].bTargetIndicators || extraGameInfo [0].bDamageIndicators) {
		ADD_CHECK (nOptions, TXT_HIT_INDICATOR, extraGameInfo [0].bTagOnlyHitObjs, KEY_T, HTX_HIT_INDICATOR);
		optHitInd = nOptions++;
		}
	else
		optHitInd = -1;
	ADD_CHECK (nOptions, TXT_MSLLOCK_INDICATOR, extraGameInfo [0].bMslLockIndicators, KEY_M, HTX_CPIT_MSLLOCKIND);
	optMslLockInd = nOptions++;
	if (extraGameInfo [0].bMslLockIndicators) {
		ADD_CHECK (nOptions, TXT_ROTATE_MSLLOCKIND, gameOpts->render.cockpit.bRotateMslLockInd, KEY_R, HTX_ROTATE_MSLLOCKIND);
		optRotateInd = nOptions++;
		}
	else
		optRotateInd = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_TGTIND_MENUTITLE, nOptions, m, &TgtIndOptionsCallback, &choice);
	} while (i >= 0);
	if (optTgtInd >= 0) {
		for (j = 0; j < 3; j++)
			if (m [optTgtInd + j].value) {
				extraGameInfo [0].bTargetIndicators = j;
				break;
				}
		GET_VAL (extraGameInfo [0].bCloakedIndicators, optCloakedInd);
		}
	GET_VAL (extraGameInfo [0].bDamageIndicators, optDmgInd);
	GET_VAL (extraGameInfo [0].bMslLockIndicators, optMslLockInd);
	GET_VAL (gameOpts->render.cockpit.bRotateMslLockInd, optRotateInd);
	GET_VAL (extraGameInfo [0].bTagOnlyHitObjs, optHitInd);
	} while (i == -2);
}

//------------------------------------------------------------------------------

int WeaponIconOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + optWeaponIcons;
v = m->value;
if (v != bShowWeaponIcons) {
	bShowWeaponIcons = v;
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void WeaponIconOptionsMenu (void)
{
	tMenuItem m [35];
	int	i, j, nOptions, choice = 0;
	int	optSmallIcons, optIconSort, optIconAmmo, optIconPos, optEquipIcons;

bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	ADD_CHECK (nOptions, TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS);
	optWeaponIcons = nOptions++;
	if (bShowWeaponIcons && gameOpts->app.bExpertMode) {
		ADD_CHECK (nOptions, TXT_SHOW_EQUIPICONS, gameOpts->render.weaponIcons.bEquipment, KEY_Q, HTX_CPIT_EQUIPICONS);
		optEquipIcons = nOptions++;
		ADD_CHECK (nOptions, TXT_SMALL_WPNICONS, gameOpts->render.weaponIcons.bSmall, KEY_I, HTX_CPIT_SMALLICONS);
		optSmallIcons = nOptions++;
		ADD_CHECK (nOptions, TXT_SORT_WPNICONS, gameOpts->render.weaponIcons.nSort, KEY_T, HTX_CPIT_SORTICONS);
		optIconSort = nOptions++;
		ADD_CHECK (nOptions, TXT_AMMO_WPNICONS, gameOpts->render.weaponIcons.bShowAmmo, KEY_A, HTX_CPIT_ICONAMMO);
		optIconAmmo = nOptions++;
		optIconPos = nOptions;
		ADD_RADIO (nOptions, TXT_WPNICONS_TOP, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		ADD_RADIO (nOptions, TXT_WPNICONS_BTM, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		ADD_RADIO (nOptions, TXT_WPNICONS_LRB, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		ADD_RADIO (nOptions, TXT_WPNICONS_LRT, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		m [optIconPos + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].value = 1;
		ADD_SLIDER (nOptions, TXT_ICON_DIM, gameOpts->render.weaponIcons.alpha, 0, 8, KEY_D, HTX_CPIT_ICONDIM);
		optIconAlpha = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		}
	else
		optEquipIcons =
		optSmallIcons =
		optIconSort =
		optIconPos =
		optIconAmmo = 
		optIconAlpha = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_WPNICON_MENUTITLE, nOptions, m, &WeaponIconOptionsCallback, &choice);
	} while (i >= 0);
	if (bShowWeaponIcons) {
		if (gameOpts->app.bExpertMode) {
			GET_VAL (gameOpts->render.weaponIcons.bEquipment, optEquipIcons);
			GET_VAL (gameOpts->render.weaponIcons.bSmall, optSmallIcons);
			GET_VAL (gameOpts->render.weaponIcons.nSort, optIconSort);
			GET_VAL (gameOpts->render.weaponIcons.bShowAmmo, optIconAmmo);
			if (optIconPos >= 0)
				for (j = 0; j < 4; j++)
					if (m [optIconPos + j].value) {
						extraGameInfo [0].nWeaponIcons = j + 1;
						break;
						}
			GET_VAL (gameOpts->render.weaponIcons.alpha, optIconAlpha);
			}
		else {
#if EXPMODE_DEFAULTS
			gameOpts->render.weaponIcons.bEquipment = 1;
			gameOpts->render.weaponIcons.bSmall = 1;
			gameOpts->render.weaponIcons.nSort = 1;
			gameOpts->render.weaponIcons.bShowAmmo = 1;
			gameOpts->render.weaponIcons.alpha = 3;
#endif
			}
		}
	else
		extraGameInfo [0].nWeaponIcons = 0;
	} while (i == -2);
}

//------------------------------------------------------------------------------

int GaugeOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + optTextGauges;
v = !m->value;
if (v != gameOpts->render.cockpit.bTextGauges) {
	gameOpts->render.cockpit.bTextGauges = v;
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void GaugeOptionsMenu (void)
{
	tMenuItem m [10];
	int	i, nOptions, choice = 0;
	int	optScaleGauges, optFlashGauges, optShieldWarn, optObjectTally, optPlayerStats;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	ADD_CHECK (nOptions, TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_P, HTX_CPIT_GFXGAUGES);
	optTextGauges = nOptions++;
	if (!gameOpts->render.cockpit.bTextGauges && gameOpts->app.bExpertMode) {
		ADD_CHECK (nOptions, TXT_SCALE_GAUGES, gameOpts->render.cockpit.bScaleGauges, KEY_C, HTX_CPIT_SCALEGAUGES);
		optScaleGauges = nOptions++;
		ADD_CHECK (nOptions, TXT_FLASH_GAUGES, gameOpts->render.cockpit.bFlashGauges, KEY_F, HTX_CPIT_FLASHGAUGES);
		optFlashGauges = nOptions++;
		}
	else
		optScaleGauges =
		optFlashGauges = -1;
	ADD_CHECK (nOptions, TXT_SHIELD_WARNING, gameOpts->gameplay.bShieldWarning, KEY_W, HTX_CPIT_SHIELDWARN);
	optShieldWarn = nOptions++;
	ADD_CHECK (nOptions, TXT_OBJECT_TALLY, gameOpts->render.cockpit.bObjectTally, KEY_T, HTX_CPIT_OBJTALLY);
	optObjectTally = nOptions++;
	ADD_CHECK (nOptions, TXT_PLAYER_STATS, gameOpts->render.cockpit.bPlayerStats, KEY_S, HTX_CPIT_PLAYERSTATS);
	optPlayerStats = nOptions++;
	do {
		i = ExecMenu1 (NULL, TXT_GAUGES_MENUTITLE, nOptions, m, &GaugeOptionsCallback, &choice);
	} while (i >= 0);
	if (!(gameOpts->render.cockpit.bTextGauges = !m [optTextGauges].value)) {
		if (gameOpts->app.bExpertMode) {
			GET_VAL (gameOpts->render.cockpit.bScaleGauges, optScaleGauges);
			GET_VAL (gameOpts->render.cockpit.bFlashGauges, optFlashGauges);
			GET_VAL (gameOpts->gameplay.bShieldWarning, optShieldWarn);
			GET_VAL (gameOpts->render.cockpit.bObjectTally, optObjectTally);
			GET_VAL (gameOpts->render.cockpit.bPlayerStats, optPlayerStats);
			}
		else {
#if EXPMODE_DEFAULTS
			gameOpts->render.cockpit.bScaleGauges = 1;
			gameOpts->render.cockpit.bFlashGauges = 1;
			gameOpts->gameplay.bShieldWarning = 0;
			gameOpts->render.cockpit.bObjectTally = 0;
			gameOpts->render.cockpit.bPlayerStats = 0;
#endif
			}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

int CockpitOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

if (gameOpts->app.bExpertMode) {
	m = menus + nCWSopt;
	v = m->value;
	if (gameOpts->render.cockpit.nWindowSize != v) {
		gameOpts->render.cockpit.nWindowSize = v;
		m->text = reinterpret_cast<char*> (szCWS [v]);
		m->rebuild = 1;
		}

	m = menus + nCWZopt;
	v = m->value;
	if (gameOpts->render.cockpit.nWindowZoom != v) {
		gameOpts->render.cockpit.nWindowZoom = v;
		sprintf (m->text, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CockpitOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, nOptions, 
			nPosition = gameOpts->render.cockpit.nWindowPos / 3, 
			nAlignment = gameOpts->render.cockpit.nWindowPos % 3, 
			choice = 0;
	int	optGauges, optHUD, optReticle, optGuided, optPosition, optAlignment,
			optMissileView, optMouseInd, optSplitMsgs, optHUDMsgs, optTgtInd, optWeaponIcons;

	char szCockpitWindowZoom [40];

szCWS [0] = TXT_CWS_SMALL;
szCWS [1] = TXT_CWS_MEDIUM;
szCWS [2] = TXT_CWS_LARGE;
szCWS [3] = TXT_CWS_HUGE;
optPosition = optAlignment = nCWSopt = nCWZopt = optTextGauges = optWeaponIcons = optIconAlpha = -1;
bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	if (gameOpts->app.bExpertMode) {
		ADD_SLIDER (nOptions, szCWS [gameOpts->render.cockpit.nWindowSize], gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_S, HTX_CPIT_WINSIZE);
		nCWSopt = nOptions++;
		sprintf (szCockpitWindowZoom, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		ADD_SLIDER (nOptions, szCockpitWindowZoom, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);
		nCWZopt = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_TEXT (nOptions, TXT_AUXWIN_POSITION, 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_POS_BOTTOM, nPosition == 0, KEY_B, 10, HTX_AUXWIN_POSITION);
		optPosition = nOptions++;
		ADD_RADIO (nOptions, TXT_POS_TOP, nPosition == 1, KEY_T, 10, HTX_AUXWIN_POSITION);
		nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_TEXT (nOptions, TXT_AUXWIN_ALIGNMENT, 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_ALIGN_CORNERS, nAlignment == 0, KEY_O, 11, HTX_AUXWIN_ALIGNMENT);
		optAlignment = nOptions++;
		ADD_RADIO (nOptions, TXT_ALIGN_MIDDLE, nAlignment == 1, KEY_I, 11, HTX_AUXWIN_ALIGNMENT);
		nOptions++;
		ADD_RADIO (nOptions, TXT_ALIGN_CENTER, nAlignment == 2, KEY_E, 11, HTX_AUXWIN_ALIGNMENT);
		nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
		optHUD = nOptions++;
		ADD_CHECK (nOptions, TXT_SHOW_HUDMSGS, gameOpts->render.cockpit.bHUDMsgs, KEY_M, HTX_CPIT_SHOWHUDMSGS);
		optHUDMsgs = nOptions++;
		ADD_CHECK (nOptions, TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optReticle = nOptions++;
		if (gameOpts->input.mouse.bJoystick) {
			ADD_CHECK (nOptions, TXT_SHOW_MOUSEIND, gameOpts->render.cockpit.bMouseIndicator, KEY_O, HTX_CPIT_MOUSEIND);
			optMouseInd = nOptions++;
			}
		else
			optMouseInd = -1;
		}
	else
		optHUD =
		optHUDMsgs =
		optMouseInd = 
		optReticle = -1;
	ADD_CHECK (nOptions, TXT_EXTRA_PLRMSGS, gameOpts->render.cockpit.bSplitHUDMsgs, KEY_P, HTX_CPIT_SPLITMSGS);
	optSplitMsgs = nOptions++;
	ADD_CHECK (nOptions, TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
	optMissileView = nOptions++;
	ADD_CHECK (nOptions, TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_F, HTX_CPIT_GUIDEDVIEW);
	optGuided = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_MENU (nOptions, TXT_TGTIND_MENUCALL, KEY_T, "");
	optTgtInd = nOptions++;
	ADD_MENU (nOptions, TXT_WPNICON_MENUCALL, KEY_W, "");
	optWeaponIcons = nOptions++;
	ADD_MENU (nOptions, TXT_GAUGES_MENUCALL, KEY_G, "");
	optGauges = nOptions++;
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_COCKPIT_OPTS, nOptions, m, &CockpitOptionsCallback, &choice);
		if (i < 0)
			break;
		if ((optTgtInd >= 0) && (i == optTgtInd))
			TgtIndOptionsMenu ();
		else if ((optWeaponIcons >= 0) && (i == optWeaponIcons))
			WeaponIconOptionsMenu ();
		else if ((optGauges >= 0) && (i == optGauges))
			GaugeOptionsMenu ();
	} while (i >= 0);
	GET_VAL (gameOpts->render.cockpit.bReticle, optReticle);
	GET_VAL (gameOpts->render.cockpit.bMissileView, optMissileView);
	GET_VAL (gameOpts->render.cockpit.bGuidedInMainView, optGuided);
	GET_VAL (gameOpts->render.cockpit.bMouseIndicator, optMouseInd);
	GET_VAL (gameOpts->render.cockpit.bHUD, optHUD);
	GET_VAL (gameOpts->render.cockpit.bHUDMsgs, optHUDMsgs);
	GET_VAL (gameOpts->render.cockpit.bSplitHUDMsgs, optSplitMsgs);
	if ((optAlignment >= 0) && (optPosition >= 0)) {
		for (nPosition = 0; nPosition < 2; nPosition++)
			if (m [optPosition + nPosition].value)
				break;
		for (nAlignment = 0; nAlignment < 3; nAlignment++)
			if (m [optAlignment + nAlignment].value)
				break;
		gameOpts->render.cockpit.nWindowPos = nPosition * 3 + nAlignment;
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static inline const char *ContrastText (void)
{
return (gameStates.ogl.nContrast == 8) ? TXT_STANDARD : 
		 (gameStates.ogl.nContrast < 8) ? TXT_LOW : 
		 TXT_HIGH;
}

//------------------------------------------------------------------------------

int FindTableFps (int nFps)
{
	int	i, j = 0, d, dMin = 0x7fffffff;

for (i = 0; i < 16; i++) {
	d = abs (nFps - fpsTable [i]);
	if (d < dMin) {
		j = i;
		dMin = d;
		}
	}
return j;
}

//------------------------------------------------------------------------------

static const char *pszCoronaInt [4];
static const char *pszCoronaQual [3];

int CoronaOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + effectOpts.nLightTrails;
v = m->value;
if (extraGameInfo [0].bLightTrails != v) {
	extraGameInfo [0].bLightTrails = v;
	*key = -2;
	}
m = menus + effectOpts.nCoronas;
v = m->value;
if (gameOpts->render.coronas.bUse != v) {
	gameOpts->render.coronas.bUse = v;
	*key = -2;
	}
m = menus + effectOpts.nShotCoronas;
v = m->value;
if (gameOpts->render.coronas.bShots != v) {
	gameOpts->render.coronas.bShots = v;
	*key = -2;
	}
if (effectOpts.nCoronaStyle >= 0) {
	m = menus + effectOpts.nCoronaStyle;
	v = m->value;
	if (gameOpts->render.coronas.nStyle != v) {
		gameOpts->render.coronas.nStyle = v;
		sprintf (m->text, TXT_CORONA_QUALITY, pszCoronaQual [v]);
		m->rebuild = -1;
		}
	}
if (effectOpts.nCoronaIntensity >= 0) {
	m = menus + effectOpts.nCoronaIntensity;
	v = m->value;
	if (gameOpts->render.coronas.nIntensity != v) {
		gameOpts->render.coronas.nIntensity = v;
		sprintf (m->text, TXT_CORONA_INTENSITY, pszCoronaInt [v]);
		m->rebuild = -1;
		}
	}
if (effectOpts.nObjCoronaIntensity >= 0) {
	m = menus + effectOpts.nObjCoronaIntensity;
	v = m->value;
	if (gameOpts->render.coronas.nObjIntensity != v) {
		gameOpts->render.coronas.nObjIntensity = v;
		sprintf (m->text, TXT_OBJCORONA_INTENSITY, pszCoronaInt [v]);
		m->rebuild = -1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CoronaOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, choice = 0, optTrailType;
	int	nOptions;
	char	szCoronaQual [50], szCoronaInt [50], szObjCoronaInt [50];

pszCoronaInt [0] = TXT_VERY_LOW;
pszCoronaInt [1] = TXT_LOW;
pszCoronaInt [2] = TXT_MEDIUM;
pszCoronaInt [3] = TXT_HIGH;

pszCoronaQual [0] = TXT_LOW;
pszCoronaQual [1] = TXT_MEDIUM;
pszCoronaQual [2] = TXT_HIGH;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	ADD_CHECK (nOptions, TXT_RENDER_CORONAS, gameOpts->render.coronas.bUse, KEY_C, HTX_ADVRND_CORONAS);
	effectOpts.nCoronas = nOptions++;
	ADD_CHECK (nOptions, TXT_SHOT_CORONAS, gameOpts->render.coronas.bShots, KEY_S, HTX_SHOT_CORONAS);
	effectOpts.nShotCoronas = nOptions++;
	ADD_CHECK (nOptions, TXT_POWERUP_CORONAS, gameOpts->render.coronas.bPowerups, KEY_P, HTX_POWERUP_CORONAS);
	effectOpts.nPowerupCoronas = nOptions++;
	ADD_CHECK (nOptions, TXT_WEAPON_CORONAS, gameOpts->render.coronas.bWeapons, KEY_W, HTX_WEAPON_CORONAS);
	effectOpts.nWeaponCoronas = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_CHECK (nOptions, TXT_ADDITIVE_CORONAS, gameOpts->render.coronas.bAdditive, KEY_A, HTX_ADDITIVE_CORONAS);
	effectOpts.nAdditiveCoronas = nOptions++;
	ADD_CHECK (nOptions, TXT_ADDITIVE_OBJCORONAS, gameOpts->render.coronas.bAdditiveObjs, KEY_O, HTX_ADDITIVE_OBJCORONAS);
	effectOpts.nAdditiveObjCoronas = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;

	sprintf (szCoronaQual + 1, TXT_CORONA_QUALITY, pszCoronaQual [gameOpts->render.coronas.nStyle]);
	*szCoronaQual = *(TXT_CORONA_QUALITY - 1);
	ADD_SLIDER (nOptions, szCoronaQual + 1, gameOpts->render.coronas.nStyle, 0, 1 + gameStates.ogl.bDepthBlending, KEY_Q, HTX_CORONA_QUALITY);
	effectOpts.nCoronaStyle = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;

	sprintf (szCoronaInt + 1, TXT_CORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nIntensity]);
	*szCoronaInt = *(TXT_CORONA_INTENSITY - 1);
	ADD_SLIDER (nOptions, szCoronaInt + 1, gameOpts->render.coronas.nIntensity, 0, 3, KEY_I, HTX_CORONA_INTENSITY);
	effectOpts.nCoronaIntensity = nOptions++;
	sprintf (szObjCoronaInt + 1, TXT_OBJCORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nObjIntensity]);
	*szObjCoronaInt = *(TXT_OBJCORONA_INTENSITY - 1);
	ADD_SLIDER (nOptions, szObjCoronaInt + 1, gameOpts->render.coronas.nObjIntensity, 0, 3, KEY_N, HTX_CORONA_INTENSITY);
	effectOpts.nObjCoronaIntensity = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;

	ADD_CHECK (nOptions, TXT_RENDER_LGTTRAILS, extraGameInfo [0].bLightTrails, KEY_T, HTX_RENDER_LGTTRAILS);
	effectOpts.nLightTrails = nOptions++;
	if (extraGameInfo [0].bLightTrails) {
		ADD_RADIO (nOptions, TXT_SOLID_LIGHTTRAILS, 0, KEY_S, 2, HTX_LIGHTTRAIL_TYPE);
		optTrailType = nOptions++;
		ADD_RADIO (nOptions, TXT_PLASMA_LIGHTTRAILS, 0, KEY_P, 2, HTX_LIGHTTRAIL_TYPE);
		nOptions++;
		m [optTrailType + gameOpts->render.particles.bPlasmaTrails].value = 1;
		}
	else
		optTrailType = -1;

	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_CORONA_MENUTITLE, nOptions, m, CoronaOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.coronas.bUse = m [effectOpts.nCoronas].value;
	gameOpts->render.coronas.bShots = m [effectOpts.nShotCoronas].value;
	gameOpts->render.coronas.bPowerups = m [effectOpts.nPowerupCoronas].value;
	gameOpts->render.coronas.bWeapons = m [effectOpts.nWeaponCoronas].value;
	gameOpts->render.coronas.bAdditive = m [effectOpts.nAdditiveCoronas].value;
	gameOpts->render.coronas.bAdditiveObjs = m [effectOpts.nAdditiveObjCoronas].value;
	if (optTrailType >= 0)
		gameOpts->render.particles.bPlasmaTrails = (m [optTrailType].value == 0);
	} while (i == -2);
}

//------------------------------------------------------------------------------

static const char *pszExplShrapnels [5];

int EffectOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + effectOpts.nExplShrapnels;
v = m->value;
if (gameOpts->render.effects.nShrapnels != v) {
	gameOpts->render.effects.nShrapnels = v;
	sprintf (m->text, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [v]);
	m->rebuild = -1;
	}
m = menus + effectOpts.nSparks;
v = m->value;
if (gameOpts->render.effects.bEnergySparks != v) {
	gameOpts->render.effects.bEnergySparks = v;
	*key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void EffectOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, j, choice = 0;
	int	nOptions;
	int	optTranspExpl, optThrusterFlame, optDmgExpl, optAutoTransp, optPlayerShields, optSoftParticles [3],
			optRobotShields, optShieldHits, optTracers, optGatlingTrails, optExplBlast, optMovingSparks;
#if 0
	int	optShockwaves;
#endif
	int	bEnergySparks = gameOpts->render.effects.bEnergySparks;
	char	szExplShrapnels [50];

pszExplShrapnels [0] = TXT_NONE;
pszExplShrapnels [1] = TXT_FEW;
pszExplShrapnels [2] = TXT_MEDIUM;
pszExplShrapnels [3] = TXT_MANY;
pszExplShrapnels [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	sprintf (szExplShrapnels + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
	*szExplShrapnels = *(TXT_EXPLOSION_SHRAPNELS - 1);
	ADD_SLIDER (nOptions, szExplShrapnels + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_P, HTX_EXPLOSION_SHRAPNELS);
	effectOpts.nExplShrapnels = nOptions++;
	ADD_CHECK (nOptions, TXT_EXPLOSION_BLAST, gameOpts->render.effects.bExplBlasts, KEY_B, HTX_EXPLOSION_BLAST);
	optExplBlast = nOptions++;
	ADD_CHECK (nOptions, TXT_DMG_EXPL, extraGameInfo [0].bDamageExplosions, KEY_X, HTX_RENDER_DMGEXPL);
	optDmgExpl = nOptions++;
	ADD_RADIO (nOptions, TXT_NO_THRUSTER_FLAME, 0, KEY_F, 1, HTX_RENDER_THRUSTER);
	optThrusterFlame = nOptions++;
	ADD_RADIO (nOptions, TXT_2D_THRUSTER_FLAME, 0, KEY_2, 1, HTX_RENDER_THRUSTER);
	nOptions++;
	ADD_RADIO (nOptions, TXT_3D_THRUSTER_FLAME, 0, KEY_3, 1, HTX_RENDER_THRUSTER);
	nOptions++;
	m [optThrusterFlame + extraGameInfo [0].bThrusterFlames].value = 1;
	ADD_CHECK (nOptions, TXT_RENDER_SPARKS, gameOpts->render.effects.bEnergySparks, KEY_P, HTX_RENDER_SPARKS);
	effectOpts.nSparks = nOptions++;
	if (gameOpts->render.effects.bEnergySparks) {
		ADD_CHECK (nOptions, TXT_MOVING_SPARKS, gameOpts->render.effects.bMovingSparks, KEY_V, HTX_MOVING_SPARKS);
		optMovingSparks = nOptions++;
		}
	else
		optMovingSparks = -1;
	if (gameOpts->render.textures.bUseHires)
		optTranspExpl = -1;
	else {
		ADD_CHECK (nOptions, TXT_TRANSP_EFFECTS, gameOpts->render.effects.bTransparent, KEY_E, HTX_ADVRND_TRANSPFX);
		optTranspExpl = nOptions++;
		}
	ADD_CHECK (nOptions, TXT_SOFT_SPRITES, (gameOpts->render.effects.bSoftParticles & 1) != 0, KEY_I, HTX_SOFT_SPRITES);
	optSoftParticles [0] = nOptions++;
	if (gameOpts->render.effects.bEnergySparks) {
		ADD_CHECK (nOptions, TXT_SOFT_SPARKS, (gameOpts->render.effects.bSoftParticles & 2) != 0, KEY_A, HTX_SOFT_SPARKS);
		optSoftParticles [1] = nOptions++;
		}
	else
		optSoftParticles [1] = -1; 
	if (extraGameInfo [0].bUseParticles) {
		ADD_CHECK (nOptions, TXT_SOFT_SMOKE, (gameOpts->render.effects.bSoftParticles & 4) != 0, KEY_O, HTX_SOFT_SMOKE);
		optSoftParticles [2] = nOptions++;
		}
	else
		optSoftParticles [2] = -1;
	ADD_CHECK (nOptions, TXT_AUTO_TRANSPARENCY, gameOpts->render.effects.bAutoTransparency, KEY_A, HTX_RENDER_AUTOTRANSP);
	optAutoTransp = nOptions++;
	ADD_CHECK (nOptions, TXT_RENDER_SHIELDS, extraGameInfo [0].bPlayerShield, KEY_P, HTX_RENDER_SHIELDS);
	optPlayerShields = nOptions++;
	ADD_CHECK (nOptions, TXT_ROBOT_SHIELDS, gameOpts->render.effects.bRobotShields, KEY_O, HTX_ROBOT_SHIELDS);
	optRobotShields = nOptions++;
	ADD_CHECK (nOptions, TXT_SHIELD_HITS, gameOpts->render.effects.bOnlyShieldHits, KEY_H, HTX_SHIELD_HITS);
	optShieldHits = nOptions++;
	ADD_CHECK (nOptions, TXT_RENDER_TRACERS, extraGameInfo [0].bTracers, KEY_T, HTX_RENDER_TRACERS);
	optTracers = nOptions++;
	ADD_CHECK (nOptions, TXT_GATLING_TRAILS, extraGameInfo [0].bGatlingTrails, KEY_G, HTX_GATLING_TRAILS);
	optGatlingTrails = nOptions++;
#if 0
	ADD_CHECK (nOptions, TXT_RENDER_SHKWAVES, extraGameInfo [0].bShockwaves, KEY_S, HTX_RENDER_SHKWAVES);
	optShockwaves = nOptions++;
#endif
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_EFFECT_MENUTITLE, nOptions, m, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.effects.bEnergySparks = m [effectOpts.nSparks].value;
	if ((gameOpts->render.effects.bEnergySparks != bEnergySparks) && gameStates.app.bGameRunning) {
		if (gameOpts->render.effects.bEnergySparks)
			AllocEnergySparks ();
		else
			FreeEnergySparks ();
		}
	GET_VAL (gameOpts->render.effects.bTransparent, optTranspExpl);
	for (j = 0; j < 3; j++)
	if (optSoftParticles [j] >= 0) {
		if (m [optSoftParticles [j]].value)
			gameOpts->render.effects.bSoftParticles |= 1 << j;
		else
			gameOpts->render.effects.bSoftParticles &= ~(1 << j);
		}
	GET_VAL (gameOpts->render.effects.bMovingSparks, optMovingSparks);
	gameOpts->render.effects.bAutoTransparency = m [optAutoTransp].value;
	gameOpts->render.effects.bExplBlasts = m [optExplBlast].value;
	extraGameInfo [0].bTracers = m [optTracers].value;
	extraGameInfo [0].bGatlingTrails = m [optGatlingTrails].value;
	extraGameInfo [0].bShockwaves = 0; //m [optShockwaves].value;
	extraGameInfo [0].bDamageExplosions = m [optDmgExpl].value;
	for (j = 0; j < 3; j++)
		if (m [optThrusterFlame + j].value) {
			extraGameInfo [0].bThrusterFlames = j;
			break;
			}
	extraGameInfo [0].bPlayerShield = m [optPlayerShields].value;
	gameOpts->render.effects.bRobotShields = m [optRobotShields].value;
	gameOpts->render.effects.bOnlyShieldHits = m [optShieldHits].value;
#if EXPMODE_DEFAULTS
	if (!gameOpts->app.bExpertMode) {
		gameOpts->render.effects.bTransparent = 1;
	gameOpts->render.effects.bAutoTransparency = 1;
	gameOpts->render.coronas.bUse = 0;
	gameOpts->render.coronas.bShots = 0;
	extraGameInfo [0].bLightTrails = 1;
	extraGameInfo [0].bTracers = 1;
	extraGameInfo [0].bShockwaves = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bPlayerShield = 1;
		}
#endif
	} while (i == -2);
SetDebrisCollisions ();
}

//------------------------------------------------------------------------------

static const char *pszRadarRange [3];

int AutomapOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem * m;
	int			v;

m = menus + automapOpts.nOptTextured;
v = m->value;
if (v != gameOpts->render.automap.bTextured) {
	gameOpts->render.automap.bTextured = v;
	*key = -2;
	return nCurItem;
	}
if (!m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].value) {
	*key = -2;
	return nCurItem;
	}
if (automapOpts.nOptRadarRange >= 0) {
	m = menus + automapOpts.nOptRadarRange;
	v = m->value;
	if (v != gameOpts->render.automap.nRange) {
		gameOpts->render.automap.nRange = v;
		sprintf (m->text, TXT_RADAR_RANGE, pszRadarRange [v]);
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void AutomapOptionsMenu ()
{
	tMenuItem m [30];
	int	i, j, choice = 0;
	int	nOptions;
	int	optBright, optGrayOut, optShowRobots, optShowPowerups, optCoronas, optSmoke, optLightnings, optColor, optSkybox, optSparks;
	char	szRadarRange [50];

pszRadarRange [0] = TXT_SHORT;
pszRadarRange [1] = TXT_MEDIUM;
pszRadarRange [2] = TXT_FAR;
*szRadarRange = '\0';
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	ADD_CHECK (nOptions, TXT_AUTOMAP_TEXTURED, gameOpts->render.automap.bTextured, KEY_T, HTX_AUTOMAP_TEXTURED);
	automapOpts.nOptTextured = nOptions++;
	if (gameOpts->render.automap.bTextured) {
		ADD_CHECK (nOptions, TXT_AUTOMAP_BRIGHT, gameOpts->render.automap.bBright, KEY_B, HTX_AUTOMAP_BRIGHT);
		optBright = nOptions++;
		ADD_CHECK (nOptions, TXT_AUTOMAP_GRAYOUT, gameOpts->render.automap.bGrayOut, KEY_Y, HTX_AUTOMAP_GRAYOUT);
		optGrayOut = nOptions++;
		ADD_CHECK (nOptions, TXT_AUTOMAP_CORONAS, gameOpts->render.automap.bCoronas, KEY_C, HTX_AUTOMAP_CORONAS);
		optCoronas = nOptions++;
		ADD_CHECK (nOptions, TXT_RENDER_SPARKS, gameOpts->render.automap.bSparks, KEY_P, HTX_RENDER_SPARKS);
		optSparks = nOptions++;
		ADD_CHECK (nOptions, TXT_AUTOMAP_SMOKE, gameOpts->render.automap.bParticles, KEY_S, HTX_AUTOMAP_SMOKE);
		optSmoke = nOptions++;
		ADD_CHECK (nOptions, TXT_AUTOMAP_LIGHTNINGS, gameOpts->render.automap.bLightnings, KEY_S, HTX_AUTOMAP_LIGHTNINGS);
		optLightnings = nOptions++;
		ADD_CHECK (nOptions, TXT_AUTOMAP_SKYBOX, gameOpts->render.automap.bSkybox, KEY_K, HTX_AUTOMAP_SKYBOX);
		optSkybox = nOptions++;
		}
	else
		optGrayOut =
		optSmoke =
		optLightnings =
		optCoronas =
		optSkybox =
		optBright =
		optSparks = -1;
	ADD_CHECK (nOptions, TXT_AUTOMAP_ROBOTS, extraGameInfo [0].bRobotsOnRadar, KEY_R, HTX_AUTOMAP_ROBOTS);
	optShowRobots = nOptions++;
	ADD_RADIO (nOptions, TXT_AUTOMAP_NO_POWERUPS, 0, KEY_D, 3, HTX_AUTOMAP_POWERUPS);
	optShowPowerups = nOptions++;
	ADD_RADIO (nOptions, TXT_AUTOMAP_POWERUPS, 0, KEY_P, 3, HTX_AUTOMAP_POWERUPS);
	nOptions++;
	if (extraGameInfo [0].nRadar) {
		ADD_RADIO (nOptions, TXT_RADAR_POWERUPS, 0, KEY_A, 3, HTX_AUTOMAP_POWERUPS);
		nOptions++;
		}
	m [optShowPowerups + extraGameInfo [0].bPowerupsOnRadar].value = 1;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_RADIO (nOptions, TXT_RADAR_OFF, 0, KEY_R, 1, HTX_AUTOMAP_RADAR);
	automapOpts.nOptRadar = nOptions++;
	ADD_RADIO (nOptions, TXT_RADAR_TOP, 0, KEY_T, 1, HTX_AUTOMAP_RADAR);
	nOptions++;
	ADD_RADIO (nOptions, TXT_RADAR_BOTTOM, 0, KEY_O, 1, HTX_AUTOMAP_RADAR);
	nOptions++;
	if (extraGameInfo [0].nRadar) {
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		sprintf (szRadarRange + 1, TXT_RADAR_RANGE, pszRadarRange [gameOpts->render.automap.nRange]);
		*szRadarRange = *(TXT_RADAR_RANGE - 1);
		ADD_SLIDER (nOptions, szRadarRange + 1, gameOpts->render.automap.nRange, 0, 2, KEY_A, HTX_RADAR_RANGE);
		automapOpts.nOptRadarRange = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_RADAR_WHITE, 0, KEY_W, 2, NULL);
		optColor = nOptions++;
		ADD_RADIO (nOptions, TXT_RADAR_BLACK, 0, KEY_L, 2, NULL);
		nOptions++;
		m [optColor + gameOpts->render.automap.nColor].value = 1;
		m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].value = 1;
		}
	else
		automapOpts.nOptRadarRange =
		optColor = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_AUTOMAP_MENUTITLE, nOptions, m, AutomapOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	//gameOpts->render.automap.bTextured = m [automapOpts.nOptTextured].value;
	GET_VAL (gameOpts->render.automap.bBright, optBright);
	GET_VAL (gameOpts->render.automap.bGrayOut, optGrayOut);
	GET_VAL (gameOpts->render.automap.bCoronas, optCoronas);
	GET_VAL (gameOpts->render.automap.bSparks, optSparks);
	GET_VAL (gameOpts->render.automap.bParticles, optSmoke);
	GET_VAL (gameOpts->render.automap.bLightnings, optLightnings);
	GET_VAL (gameOpts->render.automap.bSkybox, optSkybox);
	if (automapOpts.nOptRadarRange >= 0)
		gameOpts->render.automap.nRange = m [automapOpts.nOptRadarRange].value;
	extraGameInfo [0].bPowerupsOnRadar = m [optShowPowerups].value;
	extraGameInfo [0].bRobotsOnRadar = m [optShowRobots].value;
	for (j = 0; j < 2 + extraGameInfo [0].nRadar; j++)
		if (m [optShowPowerups + j].value) {
			extraGameInfo [0].bPowerupsOnRadar = j;
			break;
			}
	for (j = 0; j < 3; j++)
		if (m [automapOpts.nOptRadar + j].value) {
			extraGameInfo [0].nRadar = j;
			break;
			}
	if (optColor >= 0) {
		for (j = 0; j < 2; j++)
			if (m [optColor + j].value) {
				gameOpts->render.automap.nColor = j;
				break;
				}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static int nOpt3D;

int PowerupOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem * m;
	int			v;

m = menus + nOpt3D;
v = m->value;
if (v != gameOpts->render.powerups.b3D) {
	if ((gameOpts->render.powerups.b3D = v))
		ConvertAllPowerupsToWeapons ();
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void PowerupOptionsMenu ()
{
	tMenuItem m [10];
	int	i, j, choice = 0;
	int	nOptions;
	int	optSpin;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	ADD_CHECK (nOptions, TXT_3D_POWERUPS, gameOpts->render.powerups.b3D, KEY_D, HTX_3D_POWERUPS);
	nOpt3D = nOptions++;
	if (!gameOpts->render.powerups.b3D)
		optSpin = -1;
	else {
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_SPIN_OFF, 0, KEY_O, 1, NULL);
		optSpin = nOptions++;
		ADD_RADIO (nOptions, TXT_SPIN_SLOW, 0, KEY_S, 1, NULL);
		nOptions++;
		ADD_RADIO (nOptions, TXT_SPIN_MEDIUM, 0, KEY_M, 1, NULL);
		nOptions++;
		ADD_RADIO (nOptions, TXT_SPIN_FAST, 0, KEY_F, 1, NULL);
		nOptions++;
		m [optSpin + gameOpts->render.powerups.nSpin].value = 1;
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_POWERUP_MENUTITLE, nOptions, m, PowerupOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.powerups.b3D && (optSpin >= 0))
		for (j = 0; j < 4; j++)
			if (m [optSpin + j].value) {
				gameOpts->render.powerups.nSpin = j;
				break;
			}
	} while (i == -2);
}

//------------------------------------------------------------------------------

#if SHADOWS

static const char *pszReach [4];
static const char *pszClip [4];

int ShadowOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + shadowOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bShadows) {
	extraGameInfo [0].bShadows = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bShadows) {
	m = menus + shadowOpts.nMaxLights;
	v = m->value + 1;
	if (gameOpts->render.shadows.nLights != v) {
		gameOpts->render.shadows.nLights = v;
		sprintf (m->text, TXT_MAX_LIGHTS, gameOpts->render.shadows.nLights);
		m->rebuild = 1;
		}
	m = menus + shadowOpts.nReach;
	v = m->value;
	if (gameOpts->render.shadows.nReach != v) {
		gameOpts->render.shadows.nReach = v;
		sprintf (m->text, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		m->rebuild = 1;
		}
#if DBG_SHADOWS
	if (shadowOpts.nTest >= 0) {
		m = menus + shadowOpts.nTest;
		v = m->value;
		if (bShadowTest != v) {
			bShadowTest = v;
			sprintf (m->text, "Test mode: %d", bShadowTest);
			m->rebuild = 1;
			}
		m = menus + shadowOpts.nZPass;
		v = m->value;
		if (bZPass != v) {
			bZPass = v;
			m->rebuild = 1;
			*key = -2;
			return nCurItem;
			}
		m = menus + shadowOpts.nVolume;
		v = m->value;
		if (bShadowVolume != v) {
			bShadowVolume = v;
			m->rebuild = 1;
			*key = -2;
			return nCurItem;
			}
		}
#endif
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ShadowOptionsMenu ()
{
	tMenuItem m [30];
	int	i, j, choice = 0;
	int	nOptions;
	int	optClipShadows, optPlayerShadows, optRobotShadows, optMissileShadows, 
			optPowerupShadows, optReactorShadows;
	char	szMaxLightsPerFace [50], szReach [50];
#if DBG_SHADOWS
	char	szShadowTest [50];
	int	optFrontCap, optRearCap, optFrontFaces, optBackFaces, optSWCulling, optWallShadows,
			optFastShadows;
#endif

pszReach [0] = TXT_PRECISE;
pszReach [1] = TXT_SHORT;
pszReach [2] = TXT_MEDIUM;
pszReach [3] = TXT_LONG;

pszClip [0] = TXT_OFF;
pszClip [1] = TXT_FAST;
pszClip [2] = TXT_MEDIUM;
pszClip [3] = TXT_PRECISE;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	if (extraGameInfo [0].bShadows) {
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		}
	ADD_CHECK (nOptions, TXT_RENDER_SHADOWS, extraGameInfo [0].bShadows, KEY_W, HTX_ADVRND_SHADOWS);
	shadowOpts.nUse = nOptions++;
	optClipShadows =
	optPlayerShadows =
	optRobotShadows =
	optMissileShadows =
	optPowerupShadows = 
	optReactorShadows = -1;
#if DBG_SHADOWS
	shadowOpts.nZPass =
	optFrontCap =
	optRearCap =
	shadowOpts.nVolume =
	optFrontFaces =
	optBackFaces =
	optSWCulling =
	optWallShadows =
	optFastShadows =
	shadowOpts.nTest = -1;
#endif
	if (extraGameInfo [0].bShadows) {
		sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS, gameOpts->render.shadows.nLights);
		*szMaxLightsPerFace = *(TXT_MAX_LIGHTS - 1);
		ADD_SLIDER (nOptions, szMaxLightsPerFace + 1, gameOpts->render.shadows.nLights - 1, 0, MAX_SHADOW_LIGHTS, KEY_S, HTX_ADVRND_MAXLIGHTS);
		shadowOpts.nMaxLights = nOptions++;
		sprintf (szReach + 1, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		*szReach = *(TXT_SHADOW_REACH - 1);
		ADD_SLIDER (nOptions, szReach + 1, gameOpts->render.shadows.nReach, 0, 3, KEY_R, HTX_RENDER_SHADOWREACH);
		shadowOpts.nReach = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_TEXT (nOptions, TXT_CLIP_SHADOWS, 0);
		optClipShadows = ++nOptions;
		for (j = 0; j < 4; j++) {
			ADD_RADIO (nOptions, pszClip [j], gameOpts->render.shadows.nClip == j, 0, 1, HTX_CLIP_SHADOWS);
			nOptions++;
			}
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_PLAYER_SHADOWS, gameOpts->render.shadows.bPlayers, KEY_P, HTX_PLAYER_SHADOWS);
		optPlayerShadows = nOptions++;
		ADD_CHECK (nOptions, TXT_ROBOT_SHADOWS, gameOpts->render.shadows.bRobots, KEY_O, HTX_ROBOT_SHADOWS);
		optRobotShadows = nOptions++;
		ADD_CHECK (nOptions, TXT_MISSILE_SHADOWS, gameOpts->render.shadows.bMissiles, KEY_M, HTX_MISSILE_SHADOWS);
		optMissileShadows = nOptions++;
		ADD_CHECK (nOptions, TXT_POWERUP_SHADOWS, gameOpts->render.shadows.bPowerups, KEY_W, HTX_POWERUP_SHADOWS);
		optPowerupShadows = nOptions++;
		ADD_CHECK (nOptions, TXT_REACTOR_SHADOWS, gameOpts->render.shadows.bReactors, KEY_A, HTX_REACTOR_SHADOWS);
		optReactorShadows = nOptions++;
#if DBG_SHADOWS
		ADD_CHECK (nOptions, TXT_FAST_SHADOWS, gameOpts->render.shadows.bFast, KEY_F, HTX_FAST_SHADOWS);
		optFastShadows = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, "use Z-Pass algorithm", bZPass, 0, NULL);
		shadowOpts.nZPass = nOptions++;
		if (!bZPass) {
			ADD_CHECK (nOptions, "render front cap", bFrontCap, 0, NULL);
			optFrontCap = nOptions++;
			ADD_CHECK (nOptions, "render rear cap", bRearCap, 0, NULL);
			optRearCap = nOptions++;
			}
		ADD_CHECK (nOptions, "render shadow volume", bShadowVolume, 0, NULL);
		shadowOpts.nVolume = nOptions++;
		if (bShadowVolume) {
			ADD_CHECK (nOptions, "render front faces", bFrontFaces, 0, NULL);
			optFrontFaces = nOptions++;
			ADD_CHECK (nOptions, "render back faces", bBackFaces, 0, NULL);
			optBackFaces = nOptions++;
			}
		ADD_CHECK (nOptions, "render tWall shadows", bWallShadows, 0, NULL);
		optWallShadows = nOptions++;
		ADD_CHECK (nOptions, "software culling", bSWCulling, 0, NULL);
		optSWCulling = nOptions++;
		sprintf (szShadowTest, "test method: %d", bShadowTest);
		ADD_SLIDER (nOptions, szShadowTest, bShadowTest, 0, 6, KEY_S, NULL);
		shadowOpts.nTest = nOptions++;
#endif
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_SHADOW_MENUTITLE, nOptions, m, &ShadowOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	for (j = 0; j < 4; j++)
		if (m [optClipShadows + j].value) {
			gameOpts->render.shadows.nClip = j;
			break;
			}
	GET_VAL (gameOpts->render.shadows.bPlayers, optPlayerShadows);
	GET_VAL (gameOpts->render.shadows.bRobots, optRobotShadows);
	GET_VAL (gameOpts->render.shadows.bMissiles, optMissileShadows);
	GET_VAL (gameOpts->render.shadows.bPowerups, optPowerupShadows);
	GET_VAL (gameOpts->render.shadows.bReactors, optReactorShadows);
#if DBG_SHADOWS
	if (extraGameInfo [0].bShadows) {
		GET_VAL (gameOpts->render.shadows.bFast, optFastShadows);
		GET_VAL (bZPass, shadowOpts.nZPass);
		GET_VAL (bFrontCap, optFrontCap);
		GET_VAL (bRearCap, optRearCap);
		GET_VAL (bFrontFaces, optFrontFaces);
		GET_VAL (bBackFaces, optBackFaces);
		GET_VAL (bWallShadows, optWallShadows);
		GET_VAL (bSWCulling, optSWCulling);
		GET_VAL (bShadowVolume, shadowOpts.nVolume);
		}
#endif
	} while (i == -2);
}

#endif

//------------------------------------------------------------------------------

int CameraOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + camOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseCameras) {
	extraGameInfo [0].bUseCameras = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseCameras) {
	if (camOpts.nFPS >= 0) {
		m = menus + camOpts.nFPS;
		v = m->value * 5;
		if (gameOpts->render.cameras.nFPS != v) {
			gameOpts->render.cameras.nFPS = v;
			sprintf (m->text, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
			m->rebuild = 1;
			}
		}
	if (gameOpts->app.bExpertMode && (camOpts.nSpeed >= 0)) {
		m = menus + camOpts.nSpeed;
		v = (m->value + 1) * 1000;
		if (gameOpts->render.cameras.nSpeed != v) {
			gameOpts->render.cameras.nSpeed = v;
			sprintf (m->text, TXT_CAM_SPEED, v / 1000);
			m->rebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CameraOptionsMenu ()
{
	tMenuItem m [10];
	int	i, choice = 0;
	int	nOptions;
	int	bFSCameras = gameOpts->render.cameras.bFitToWall;
	int	optFSCameras, optTeleCams, optHiresCams;
#if 0
	int checks;
#endif

	char szCameraFps [50];
	char szCameraSpeed [50];

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	ADD_CHECK (nOptions, TXT_USE_CAMS, extraGameInfo [0].bUseCameras, KEY_C, HTX_ADVRND_USECAMS);
	camOpts.nUse = nOptions++;
	if (extraGameInfo [0].bUseCameras && gameOpts->app.bExpertMode) {
		if (gameStates.app.bGameRunning) 
			optHiresCams = -1;
		else {
			ADD_CHECK (nOptions, TXT_HIRES_CAMERAS, gameOpts->render.cameras.bHires, KEY_H, HTX_HIRES_CAMERAS);
			optHiresCams = nOptions++;
			}
		ADD_CHECK (nOptions, TXT_TELEPORTER_CAMS, extraGameInfo [0].bTeleporterCams, KEY_U, HTX_TELEPORTER_CAMS);
		optTeleCams = nOptions++;
		ADD_CHECK (nOptions, TXT_ADJUST_CAMS, gameOpts->render.cameras.bFitToWall, KEY_A, HTX_ADVRND_ADJUSTCAMS);
		optFSCameras = nOptions++;
		sprintf (szCameraFps + 1, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
		*szCameraFps = *(TXT_CAM_REFRESH - 1);
		ADD_SLIDER (nOptions, szCameraFps + 1, gameOpts->render.cameras.nFPS / 5, 0, 6, KEY_A, HTX_ADVRND_CAMREFRESH);
		camOpts.nFPS = nOptions++;
		sprintf (szCameraSpeed + 1, TXT_CAM_SPEED, gameOpts->render.cameras.nSpeed / 1000);
		*szCameraSpeed = *(TXT_CAM_SPEED - 1);
		ADD_SLIDER (nOptions, szCameraSpeed + 1, (gameOpts->render.cameras.nSpeed / 1000) - 1, 0, 9, KEY_D, HTX_ADVRND_CAMSPEED);
		camOpts.nSpeed = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		}
	else {
		optHiresCams = 
		optTeleCams = 
		optFSCameras =
		camOpts.nFPS =
		camOpts.nSpeed = -1;
		}

	do {
		i = ExecMenu1 (NULL, TXT_CAMERA_MENUTITLE, nOptions, m, &CameraOptionsCallback, &choice);
	} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = m [camOpts.nUse].value)) {
		GET_VAL (extraGameInfo [0].bTeleporterCams, optTeleCams);
		GET_VAL (gameOpts->render.cameras.bFitToWall, optFSCameras);
		if (!gameStates.app.bGameRunning)
			GET_VAL (gameOpts->render.cameras.bHires, optHiresCams);
		}
	if (bFSCameras != gameOpts->render.cameras.bFitToWall) {
		cameraManager.Destroy ();
		cameraManager.Create ();
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

int SmokeOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			i, v;

m = menus + smokeOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseParticles) {
	extraGameInfo [0].bUseParticles = v;
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseParticles) {
	m = menus + smokeOpts.nQuality;
	v = m->value;
	if (v != gameOpts->render.particles.bSort) {
		gameOpts->render.particles.bSort = v;
		sprintf (m->text, TXT_SMOKE_QUALITY, pszSmokeQual [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	m = menus + smokeOpts.nSyncSizes;
	v = m->value;
	if (v != gameOpts->render.particles.bSyncSizes) {
		gameOpts->render.particles.bSyncSizes = v;
		*key = -2;
		return nCurItem;
		}
	m = menus + smokeOpts.nPlayer;
	v = m->value;
	if (gameOpts->render.particles.bPlayers != v) {
		gameOpts->render.particles.bPlayers = v;
		*key = -2;
		}
	m = menus + smokeOpts.nRobots;
	v = m->value;
	if (gameOpts->render.particles.bRobots != v) {
		gameOpts->render.particles.bRobots = v;
		*key = -2;
		}
	m = menus + smokeOpts.nMissiles;
	v = m->value;
	if (gameOpts->render.particles.bMissiles != v) {
		gameOpts->render.particles.bMissiles = v;
		*key = -2;
		}
	m = menus + smokeOpts.nDebris;
	v = m->value;
	if (gameOpts->render.particles.bDebris != v) {
		gameOpts->render.particles.bDebris = v;
		*key = -2;
		}
	m = menus + smokeOpts.nBubbles;
	v = m->value;
	if (gameOpts->render.particles.bBubbles != v) {
		gameOpts->render.particles.bBubbles = v;
		*key = -2;
		}
	if (gameOpts->render.particles.bSyncSizes) {
		m = menus + smokeOpts.nDensity [0];
		v = m->value;
		if (gameOpts->render.particles.nDens [0] != v) {
			gameOpts->render.particles.nDens [0] = v;
			sprintf (m->text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
			m->rebuild = 1;
			}
		m = menus + smokeOpts.nSize [0];
		v = m->value;
		if (gameOpts->render.particles.nSize [0] != v) {
			gameOpts->render.particles.nSize [0] = v;
			sprintf (m->text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
			m->rebuild = 1;
			}
		m = menus + smokeOpts.nAlpha [0];
		v = m->value;
		if (v != gameOpts->render.particles.nAlpha [0]) {
			gameOpts->render.particles.nAlpha [0] = v;
			sprintf (m->text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
			m->rebuild = 1;
			}
		}
	else {
		for (i = 1; i < 5; i++) {
			if (smokeOpts.nDensity [i] >= 0) {
				m = menus + smokeOpts.nDensity [i];
				v = m->value;
				if (gameOpts->render.particles.nDens [i] != v) {
					gameOpts->render.particles.nDens [i] = v;
					sprintf (m->text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nSize [i] >= 0) {
				m = menus + smokeOpts.nSize [i];
				v = m->value;
				if (gameOpts->render.particles.nSize [i] != v) {
					gameOpts->render.particles.nSize [i] = v;
					sprintf (m->text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nLife [i] >= 0) {
				m = menus + smokeOpts.nLife [i];
				v = m->value;
				if (gameOpts->render.particles.nLife [i] != v) {
					gameOpts->render.particles.nLife [i] = v;
					sprintf (m->text, TXT_SMOKE_LIFE, pszSmokeLife [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nAlpha [i] >= 0) {
				m = menus + smokeOpts.nAlpha [i];
				v = m->value;
				if (v != gameOpts->render.particles.nAlpha [i]) {
					gameOpts->render.particles.nAlpha [i] = v;
					sprintf (m->text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
					m->rebuild = 1;
					}
				}
			}
		}
	}
else
	particleManager.Shutdown ();
return nCurItem;
}

//------------------------------------------------------------------------------

static char szSmokeDens [5][50];
static char szSmokeSize [5][50];
static char szSmokeLife [5][50];
static char szSmokeAlpha [5][50];

int AddSmokeSliders (tMenuItem *m, int nOptions, int i)
{
sprintf (szSmokeDens [i] + 1, TXT_SMOKE_DENS, pszSmokeAmount [NMCLAMP (gameOpts->render.particles.nDens [i], 0, 4)]);
*szSmokeDens [i] = *(TXT_SMOKE_DENS - 1);
ADD_SLIDER (nOptions, szSmokeDens [i] + 1, gameOpts->render.particles.nDens [i], 0, 4, KEY_P, HTX_ADVRND_SMOKEDENS);
smokeOpts.nDensity [i] = nOptions++;
sprintf (szSmokeSize [i] + 1, TXT_SMOKE_SIZE, pszSmokeSize [NMCLAMP (gameOpts->render.particles.nSize [i], 0, 3)]);
*szSmokeSize [i] = *(TXT_SMOKE_SIZE - 1);
ADD_SLIDER (nOptions, szSmokeSize [i] + 1, gameOpts->render.particles.nSize [i], 0, 3, KEY_Z, HTX_ADVRND_PARTSIZE);
smokeOpts.nSize [i] = nOptions++;
if (i < 3)
	smokeOpts.nLife [i] = -1;
else {
	sprintf (szSmokeLife [i] + 1, TXT_SMOKE_LIFE, pszSmokeLife [NMCLAMP (gameOpts->render.particles.nLife [i], 0, 3)]);
	*szSmokeLife [i] = *(TXT_SMOKE_LIFE - 1);
	ADD_SLIDER (nOptions, szSmokeLife [i] + 1, gameOpts->render.particles.nLife [i], 0, 2, KEY_L, HTX_SMOKE_LIFE);
	smokeOpts.nLife [i] = nOptions++;
	}
sprintf (szSmokeAlpha [i] + 1, TXT_SMOKE_ALPHA, pszSmokeAlpha [NMCLAMP (gameOpts->render.particles.nAlpha [i], 0, 4)]);
*szSmokeAlpha [i] = *(TXT_SMOKE_SIZE - 1);
ADD_SLIDER (nOptions, szSmokeAlpha [i] + 1, gameOpts->render.particles.nAlpha [i], 0, 4, KEY_Z, HTX_ADVRND_SMOKEALPHA);
smokeOpts.nAlpha [i] = nOptions++;
return nOptions;
}

//------------------------------------------------------------------------------

void SmokeOptionsMenu ()
{
	tMenuItem m [40];
	int	i, j, choice = 0;
	int	nOptions;
	int	nOptSmokeLag, optStaticParticles, optCollisions, optDisperse, 
			optRotate = -1, optAuxViews = -1, optMonitors = -1, optWiggle = -1, optWobble = -1;
	char	szSmokeQual [50];

pszSmokeSize [0] = TXT_SMALL;
pszSmokeSize [1] = TXT_MEDIUM;
pszSmokeSize [2] = TXT_LARGE;
pszSmokeSize [3] = TXT_VERY_LARGE;

pszSmokeAmount [0] = TXT_QUALITY_LOW;
pszSmokeAmount [1] = TXT_QUALITY_MED;
pszSmokeAmount [2] = TXT_QUALITY_HIGH;
pszSmokeAmount [3] = TXT_VERY_HIGH;
pszSmokeAmount [4] = TXT_EXTREME;

pszSmokeLife [0] = TXT_SHORT;
pszSmokeLife [1] = TXT_MEDIUM;
pszSmokeLife [2] = TXT_LONG;

pszSmokeQual [0] = TXT_STANDARD;
pszSmokeQual [1] = TXT_GOOD;
pszSmokeQual [2] = TXT_HIGH;

pszSmokeAlpha [0] = TXT_LOW;
pszSmokeAlpha [1] = TXT_MEDIUM;
pszSmokeAlpha [2] = TXT_HIGH;
pszSmokeAlpha [3] = TXT_VERY_HIGH;
pszSmokeAlpha [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	memset (&smokeOpts, 0xff, sizeof (smokeOpts));
	nOptions = 0;
	nOptSmokeLag = optStaticParticles = optCollisions = optDisperse = -1;

	ADD_CHECK (nOptions, TXT_USE_SMOKE, extraGameInfo [0].bUseParticles, KEY_U, HTX_ADVRND_USESMOKE);
	smokeOpts.nUse = nOptions++;
	for (j = 1; j < 5; j++)
		smokeOpts.nSize [j] =
		smokeOpts.nDensity [j] = 
		smokeOpts.nAlpha [j] = -1;
	if (extraGameInfo [0].bUseParticles) {
		if (gameOpts->app.bExpertMode) {
			sprintf (szSmokeQual + 1, TXT_SMOKE_QUALITY, pszSmokeQual [NMCLAMP (gameOpts->render.particles.bSort, 0, 2)]);
			*szSmokeQual = *(TXT_SMOKE_QUALITY - 1);
			ADD_SLIDER (nOptions, szSmokeQual + 1, gameOpts->render.particles.bSort, 0, 2, KEY_Q, HTX_ADVRND_SMOKEQUAL);
			smokeOpts.nQuality = nOptions++;
			ADD_CHECK (nOptions, TXT_SYNC_SIZES, gameOpts->render.particles.bSyncSizes, KEY_M, HTX_ADVRND_SYNCSIZES);
			smokeOpts.nSyncSizes = nOptions++;
			if (gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 0);
				for (j = 1; j < 5; j++) {
					gameOpts->render.particles.nSize [j] = gameOpts->render.particles.nSize [0];
					gameOpts->render.particles.nDens [j] = gameOpts->render.particles.nDens [0];
					gameOpts->render.particles.nAlpha [j] = gameOpts->render.particles.nAlpha [0];
					}
				}
			else {
				smokeOpts.nDensity [0] =
				smokeOpts.nSize [0] = -1;
				}
			if (!gameOpts->render.particles.bSyncSizes && gameOpts->render.particles.bPlayers) {
				ADD_TEXT (nOptions, "", 0);
				nOptions++;
				}
			ADD_CHECK (nOptions, TXT_SMOKE_PLAYERS, gameOpts->render.particles.bPlayers, KEY_Y, HTX_ADVRND_PLRSMOKE);
			smokeOpts.nPlayer = nOptions++;
			if (gameOpts->render.particles.bPlayers) {
				if (!gameOpts->render.particles.bSyncSizes)
					nOptions = AddSmokeSliders (m, nOptions, 1);
				ADD_CHECK (nOptions, TXT_SMOKE_DECREASE_LAG, gameOpts->render.particles.bDecreaseLag, KEY_R, HTX_ADVREND_DECSMOKELAG);
				nOptSmokeLag = nOptions++;
				ADD_TEXT (nOptions, "", 0);
				nOptions++;
				}
			else
				nOptSmokeLag = -1;
			ADD_CHECK (nOptions, TXT_SMOKE_ROBOTS, gameOpts->render.particles.bRobots, KEY_O, HTX_ADVRND_BOTSMOKE);
			smokeOpts.nRobots = nOptions++;
			if (gameOpts->render.particles.bRobots && !gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 2);
				ADD_TEXT (nOptions, "", 0);
				nOptions++;
				}
			ADD_CHECK (nOptions, TXT_SMOKE_MISSILES, gameOpts->render.particles.bMissiles, KEY_M, HTX_ADVRND_MSLSMOKE);
			smokeOpts.nMissiles = nOptions++;
			if (gameOpts->render.particles.bMissiles && !gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 3);
				ADD_TEXT (nOptions, "", 0);
				nOptions++;
				}
			ADD_CHECK (nOptions, TXT_SMOKE_DEBRIS, gameOpts->render.particles.bDebris, KEY_D, HTX_ADVRND_DEBRISSMOKE);
			smokeOpts.nDebris = nOptions++;
			if (gameOpts->render.particles.bDebris && !gameOpts->render.particles.bSyncSizes) {
				nOptions = AddSmokeSliders (m, nOptions, 4);
				ADD_TEXT (nOptions, "", 0);
				nOptions++;
				}
			ADD_CHECK (nOptions, TXT_SMOKE_STATIC, gameOpts->render.particles.bStatic, KEY_T, HTX_ADVRND_STATICSMOKE);
			optStaticParticles = nOptions++;
#if 0
			ADD_CHECK (nOptions, TXT_SMOKE_COLLISION, gameOpts->render.particles.bCollisions, KEY_I, HTX_ADVRND_SMOKECOLL);
			optCollisions = nOptions++;
#endif
			ADD_CHECK (nOptions, TXT_SMOKE_DISPERSE, gameOpts->render.particles.bDisperse, KEY_D, HTX_ADVRND_SMOKEDISP);
			optDisperse = nOptions++;
			ADD_CHECK (nOptions, TXT_ROTATE_SMOKE, gameOpts->render.particles.bRotate, KEY_R, HTX_ROTATE_SMOKE);
			optRotate = nOptions++;
			ADD_CHECK (nOptions, TXT_SMOKE_AUXVIEWS, gameOpts->render.particles.bAuxViews, KEY_W, HTX_SMOKE_AUXVIEWS);
			optAuxViews = nOptions++;
			ADD_CHECK (nOptions, TXT_SMOKE_MONITORS, gameOpts->render.particles.bMonitors, KEY_M, HTX_SMOKE_MONITORS);
			optMonitors = nOptions++;
			ADD_TEXT (nOptions, "", 0);
			nOptions++;
			ADD_CHECK (nOptions, TXT_SMOKE_BUBBLES, gameOpts->render.particles.bBubbles, KEY_B, HTX_SMOKE_BUBBLES);
			smokeOpts.nBubbles = nOptions++;
			if (gameOpts->render.particles.bBubbles) {
				ADD_CHECK (nOptions, TXT_WIGGLE_BUBBLES, gameOpts->render.particles.bWiggleBubbles, KEY_I, HTX_WIGGLE_BUBBLES);
				optWiggle = nOptions++;
				ADD_CHECK (nOptions, TXT_WOBBLE_BUBBLES, gameOpts->render.particles.bWobbleBubbles, KEY_I, HTX_WOBBLE_BUBBLES);
				optWobble = nOptions++;
				}
			}
		}
	else
		nOptSmokeLag =
		smokeOpts.nPlayer =
		smokeOpts.nRobots =
		smokeOpts.nMissiles =
		smokeOpts.nDebris =
		smokeOpts.nBubbles =
		optStaticParticles =
		optCollisions =
		optDisperse = 
		optRotate = 
		optAuxViews = 
		optMonitors = -1;

	Assert (sizeofa (m) >= nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_SMOKE_MENUTITLE, nOptions, m, &SmokeOptionsCallback, &choice);
		} while (i >= 0);
	if ((extraGameInfo [0].bUseParticles = m [smokeOpts.nUse].value)) {
		GET_VAL (gameOpts->render.particles.bPlayers, smokeOpts.nPlayer);
		GET_VAL (gameOpts->render.particles.bRobots, smokeOpts.nRobots);
		GET_VAL (gameOpts->render.particles.bMissiles, smokeOpts.nMissiles);
		GET_VAL (gameOpts->render.particles.bDebris, smokeOpts.nDebris);
		GET_VAL (gameOpts->render.particles.bStatic, optStaticParticles);
#if 0
		GET_VAL (gameOpts->render.particles.bCollisions, optCollisions);
#else
		gameOpts->render.particles.bCollisions = 0;
#endif
		GET_VAL (gameOpts->render.particles.bDisperse, optDisperse);
		GET_VAL (gameOpts->render.particles.bRotate, optRotate);
		GET_VAL (gameOpts->render.particles.bDecreaseLag, nOptSmokeLag);
		GET_VAL (gameOpts->render.particles.bAuxViews, optAuxViews);
		GET_VAL (gameOpts->render.particles.bMonitors, optMonitors);
		if (gameOpts->render.particles.bBubbles) {
			GET_VAL (gameOpts->render.particles.bWiggleBubbles, optWiggle);
			GET_VAL (gameOpts->render.particles.bWobbleBubbles, optWobble);
			}
		//GET_VAL (gameOpts->render.particles.bSyncSizes, smokeOpts.nSyncSizes);
		if (gameOpts->render.particles.bSyncSizes) {
			for (j = 1; j < 4; j++) {
				gameOpts->render.particles.nSize [j] = gameOpts->render.particles.nSize [0];
				gameOpts->render.particles.nDens [j] = gameOpts->render.particles.nDens [0];
				}
			}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

int AdvancedRenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

if (optContrast >= 0) {
	m = menus + optContrast;
	v = m->value;
	if (v != gameStates.ogl.nContrast) {
		gameStates.ogl.nContrast = v;
		sprintf (m->text, TXT_CONTRAST, ContrastText ());
		m->rebuild = 1;
		}
	}
if (gameOpts->app.bExpertMode) {
	m = menus + renderOpts.nRenderQual;
	v = m->value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->rebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menus + renderOpts.nTexQual;
		v = m->value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->rebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menus + renderOpts.nMeshQual;
		v = m->value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int LightOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

if (lightOpts.nMethod >= 0) {
	for (v = 0; v < 3; v++)
		if (menus [lightOpts.nMethod + v].value)
			break;
	if (v > 2)
		v = 0;
	if (v != gameOpts->render.nLightingMethod) {
		gameOpts->render.nLightingMethod = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nLightmapQual >= 0) {
	m = menus + lightOpts.nLightmapQual;
	v = m->value;
	if (gameOpts->render.nLightmapQuality != v) {
		gameOpts->render.nLightmapQuality = v;
		sprintf (m->text, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
		m->rebuild = 1;
		}
	}
if (lightOpts.nLightmaps >= 0) {
	m = menus + lightOpts.nLightmaps;
	v = m->value;
	if (v != gameOpts->render.bUseLightmaps) {
		gameOpts->render.bUseLightmaps = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nHWObjLighting >= 0) {
	m = menus + lightOpts.nHWObjLighting;
	v = m->value;
	if (v != gameOpts->ogl.bObjLighting) {
		gameOpts->ogl.bObjLighting = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nGunColor >= 0) {
	m = menus + lightOpts.nGunColor;
	v = m->value;
	if (v != gameOpts->render.color.bGunLight) {
		gameOpts->render.color.bGunLight = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nObjectLight >= 0) {
	m = menus + lightOpts.nObjectLight;
	v = m->value;
	if (v != gameOpts->ogl.bLightObjects) {
		gameOpts->ogl.bLightObjects = v;
		*key = -2;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerFace >= 0) {
	m = menus + lightOpts.nMaxLightsPerFace;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerFace) {
		gameOpts->ogl.nMaxLightsPerFace = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerObject >= 0) {
	m = menus + lightOpts.nMaxLightsPerObject;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerObject) {
		gameOpts->ogl.nMaxLightsPerObject = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return nCurItem;
		}
	}
if (lightOpts.nMaxLightsPerPass >= 0) {
	m = menus + lightOpts.nMaxLightsPerPass;
	v = m->value + 1;
	if (v != gameOpts->ogl.nMaxLightsPerPass) {
		gameOpts->ogl.nMaxLightsPerPass = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_PASS, v);
		m->rebuild = 1;
		return nCurItem;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static int LightTableIndex (int nValue)
{
	int i, h = (int) sizeofa (nMaxLightsPerFaceTable);

for (i = 0; i < h; i++)
	if (nValue < nMaxLightsPerFaceTable [i])
		break;
return i ? i - 1 : 0;
}

//------------------------------------------------------------------------------

void LightOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, choice = 0;
	int	nOptions;
	int	optColoredLight, optMixColors, optPowerupLights, optFlickerLights, optColorSat, optBrightObjects, nPowerupLight = -1;
#if 0
	int checks;
#endif

	char szMaxLightsPerFace [50];
	char szMaxLightsPerPass [50];
	char szMaxLightsPerObject [50];
	char szLightmapQual [50];

	pszLMapQual [0] = TXT_LOW;
	pszLMapQual [1] = TXT_MEDIUM;
	pszLMapQual [2] = TXT_HIGH;
	pszLMapQual [3] = TXT_VERY_HIGH;
	pszLMapQual [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	optColorSat = 
	optColoredLight =
	optMixColors = 
	optPowerupLights = -1;
	memset (&lightOpts, 0xff, sizeof (lightOpts));
	if (!gameStates.app.bGameRunning) {
		if ((gameOpts->render.nLightingMethod == 2) && 
			 !(gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk))
			gameOpts->render.nLightingMethod = 1;
		ADD_RADIO (nOptions, TXT_STD_LIGHTING, gameOpts->render.nLightingMethod == 0, KEY_S, 1, NULL);
		lightOpts.nMethod = nOptions++;
		ADD_RADIO (nOptions, TXT_VERTEX_LIGHTING, gameOpts->render.nLightingMethod == 1, KEY_V, 1, HTX_VERTEX_LIGHTING);
		nOptions++;
		if (gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk) {
			ADD_RADIO (nOptions, TXT_PER_PIXEL_LIGHTING, gameOpts->render.nLightingMethod == 2, KEY_P, 1, HTX_PER_PIXEL_LIGHTING);
			nOptions++;
			}	
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		}
	gameOpts->ogl.nMaxLightsPerObject = LightTableIndex (gameOpts->ogl.nMaxLightsPerObject);
	gameOpts->ogl.nMaxLightsPerFace = LightTableIndex (gameOpts->ogl.nMaxLightsPerFace);
	if (gameOpts->render.nLightingMethod) {
#if 0
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
#endif
		if (gameOpts->render.nLightingMethod == 1) {
			if (gameStates.ogl.bHeadlight) {
				ADD_CHECK (nOptions, TXT_HW_HEADLIGHT, gameOpts->ogl.bHeadlight, KEY_H, HTX_HW_HEADLIGHT);
				lightOpts.nHWHeadlight = nOptions++;
				}
			ADD_CHECK (nOptions, TXT_OBJECT_HWLIGHTING, gameOpts->ogl.bObjLighting, KEY_A, HTX_OBJECT_HWLIGHTING);
			lightOpts.nHWObjLighting = nOptions++;
			if (!gameOpts->ogl.bObjLighting) {
				ADD_CHECK (nOptions, TXT_OBJECT_LIGHTING, gameOpts->ogl.bLightObjects, KEY_B, HTX_OBJECT_LIGHTING);
				lightOpts.nObjectLight = nOptions++;
				if (gameOpts->ogl.bLightObjects) {
					ADD_CHECK (nOptions, TXT_POWERUP_LIGHTING, gameOpts->ogl.bLightPowerups, KEY_W, HTX_POWERUP_LIGHTING);
					nPowerupLight = nOptions++;
					}
				else
					nPowerupLight = -1;
				}
			}
		sprintf (szMaxLightsPerObject + 1, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject]);
		*szMaxLightsPerObject = *(TXT_MAX_LIGHTS_PER_OBJECT - 1);
		ADD_SLIDER (nOptions, szMaxLightsPerObject + 1, gameOpts->ogl.nMaxLightsPerObject, 0, (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_O, HTX_MAX_LIGHTS_PER_OBJECT);
		lightOpts.nMaxLightsPerObject = nOptions++;

		if (gameOpts->render.nLightingMethod == 2) {
			sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace]);
			*szMaxLightsPerFace = *(TXT_MAX_LIGHTS_PER_FACE - 1);
			ADD_SLIDER (nOptions, szMaxLightsPerFace + 1, gameOpts->ogl.nMaxLightsPerFace, 0,  (int) sizeofa (nMaxLightsPerFaceTable) - 1, KEY_A, HTX_MAX_LIGHTS_PER_FACE);
			lightOpts.nMaxLightsPerFace = nOptions++;
			sprintf (szMaxLightsPerPass + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			*szMaxLightsPerPass = *(TXT_MAX_LIGHTS_PER_PASS - 1);
			ADD_SLIDER (nOptions, szMaxLightsPerPass + 1, gameOpts->ogl.nMaxLightsPerPass - 1, 0, 7, KEY_S, HTX_MAX_LIGHTS_PER_PASS);
			lightOpts.nMaxLightsPerPass = nOptions++;
			}
		if (!gameStates.app.bGameRunning && 
			 ((gameOpts->render.nLightingMethod == 2) || ((gameOpts->render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps))) {
			sprintf (szLightmapQual + 1, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
			*szLightmapQual = *(TXT_LMAP_QUALITY + 1);
			ADD_SLIDER (nOptions, szLightmapQual + 1, gameOpts->render.nLightmapQuality, 0, 4, KEY_Q, HTX_LMAP_QUALITY);
			lightOpts.nLightmapQual = nOptions++;
			}

		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_FULL_COLORSAT, 0, KEY_F, 2, HTX_COLOR_SATURATION);
		optColorSat = nOptions++;
		ADD_RADIO (nOptions, TXT_LIMIT_COLORSAT, 0, KEY_L, 2, HTX_COLOR_SATURATION);
		nOptions++;
		ADD_RADIO (nOptions, TXT_NO_COLORSAT, 0, KEY_N, 2, HTX_COLOR_SATURATION);
		nOptions++;
		m [optColorSat + NMCLAMP (gameOpts->render.color.nSaturation, 0, 2)].value = 1;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		}
	if (gameOpts->render.nLightingMethod != 1)
		lightOpts.nLightmaps = -1;
	else {
		ADD_CHECK (nOptions, TXT_USE_LIGHTMAPS, gameOpts->render.bUseLightmaps, KEY_G, HTX_USE_LIGHTMAPS);
		lightOpts.nLightmaps = nOptions++;
		}
	if (gameOpts->render.nLightingMethod < 2) {
		ADD_CHECK (nOptions, TXT_USE_COLOR, gameOpts->render.color.bAmbientLight, KEY_C, HTX_RENDER_AMBICOLOR);
		optColoredLight = nOptions++;
		}
	if (!gameOpts->render.nLightingMethod) {
		ADD_CHECK (nOptions, TXT_USE_WPNCOLOR, gameOpts->render.color.bGunLight, KEY_W, HTX_RENDER_WPNCOLOR);
		lightOpts.nGunColor = nOptions++;
		if (gameOpts->app.bExpertMode) {
			if (!gameOpts->render.nLightingMethod && gameOpts->render.color.bGunLight) {
				ADD_CHECK (nOptions, TXT_MIX_COLOR, gameOpts->render.color.bMix, KEY_X, HTX_ADVRND_MIXCOLOR);
				optMixColors = nOptions++;
				}
			}
		}
	ADD_CHECK (nOptions, TXT_POWERUPLIGHTS, !extraGameInfo [0].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
	optPowerupLights = nOptions++;
	ADD_CHECK (nOptions, TXT_FLICKERLIGHTS, extraGameInfo [0].bFlickerLights, KEY_F, HTX_FLICKERLIGHTS);
	optFlickerLights = nOptions++;
	if (gameOpts->render.bHiresModels) {
		ADD_CHECK (nOptions, TXT_BRIGHT_OBJECTS, extraGameInfo [0].bBrightObjects, KEY_B, HTX_BRIGHT_OBJECTS);
		optBrightObjects = nOptions++;
		}
	else
		optBrightObjects = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTING_MENUTITLE, nOptions, m, &LightOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (gameOpts->render.nLightingMethod == 1) {
		if (lightOpts.nObjectLight >= 0) {
			gameOpts->ogl.bLightObjects = m [lightOpts.nObjectLight].value;
			if (nPowerupLight >= 0)
				gameOpts->ogl.bLightPowerups = m [nPowerupLight].value;
			}
		}
	if (gameOpts->render.nLightingMethod < 2) {
		if (optColoredLight >= 0)
			gameOpts->render.color.bAmbientLight = m [optColoredLight].value;
		if (lightOpts.nGunColor >= 0)
			gameOpts->render.color.bGunLight = m [lightOpts.nGunColor].value;
		if (gameOpts->app.bExpertMode) {
			if (gameStates.render.color.bLightmapsOk && gameOpts->render.color.bUseLightmaps)
			gameStates.ogl.nContrast = 8;
			if (gameOpts->render.color.bGunLight)
				GET_VAL (gameOpts->render.color.bMix, optMixColors);
#if EXPMODE_DEFAULTS
				else
					gameOpts->render.color.bMix = 1;
#endif
			}
		}
	if (optPowerupLights >= 0)
		extraGameInfo [0].bPowerupLights = !m [optPowerupLights].value;
	extraGameInfo [0].bFlickerLights = m [optFlickerLights].value;
	GET_VAL (gameOpts->ogl.bHeadlight, lightOpts.nHWHeadlight);
	GET_VAL (extraGameInfo [0].bBrightObjects, optBrightObjects);
	gameOpts->ogl.nMaxLightsPerFace = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace];
	gameOpts->ogl.nMaxLightsPerObject = nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject];
	} while (i == -2);
if (optColorSat >= 0) {
	for (i = 0; i < 3; i++)
		if (m [optColorSat + i].value) {
			gameOpts->render.color.nSaturation = i;
			break;
			}
	}
gameStates.render.nLightingMethod = gameOpts->render.nLightingMethod;
if (gameStates.render.nLightingMethod == 2)
	gameStates.render.bPerPixelLighting = 2;
else if ((gameStates.render.nLightingMethod == 1) && gameOpts->render.bUseLightmaps)
	gameStates.render.bPerPixelLighting = 1;
else
	gameStates.render.bPerPixelLighting = 0;
if (gameStates.render.bPerPixelLighting == 2) {
	gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
	gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
	}
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = gameStates.render.bPerPixelLighting || gameOpts->render.color.bAmbientLight;
}

//------------------------------------------------------------------------------

static const char *pszLightningQuality [2];
static const char *pszLightningStyle [3];

int LightningOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + lightningOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseLightnings) {
	if (!(extraGameInfo [0].bUseLightnings = v))
		lightningManager.Shutdown (0);
	*key = -2;
	return nCurItem;
	}
if (extraGameInfo [0].bUseLightnings) {
	m = menus + lightningOpts.nQuality;
	v = m->value;
	if (gameOpts->render.lightnings.nQuality != v) {
		gameOpts->render.lightnings.nQuality = v;
		sprintf (m->text, TXT_LIGHTNING_QUALITY, pszLightningQuality [v]);
		m->rebuild = 1;
		lightningManager.Shutdown (0);
		}
	m = menus + lightningOpts.nStyle;
	v = m->value;
	if (gameOpts->render.lightnings.nStyle != v) {
		gameOpts->render.lightnings.nStyle = v;
		sprintf (m->text, TXT_LIGHTNING_STYLE, pszLightningStyle [v]);
		m->rebuild = 1;
		}
	m = menus + lightningOpts.nOmega;
	v = m->value;
	if (gameOpts->render.lightnings.bOmega != v) {
		gameOpts->render.lightnings.bOmega = v;
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LightningOptionsMenu (void)
{
	tMenuItem m [15];
	int	i, choice = 0;
	int	nOptions;
	int	optDamage, optExplosions, optPlayers, optRobots, optStatic, optRobotOmega, optPlasma, optAuxViews, optMonitors;
	char	szQuality [50], szStyle [100];

	pszLightningQuality [0] = TXT_LOW;
	pszLightningQuality [1] = TXT_HIGH;
	pszLightningStyle [0] = TXT_LIGHTNING_ERRATIC;
	pszLightningStyle [1] = TXT_LIGHTNING_JAGGY;
	pszLightningStyle [2] = TXT_LIGHTNING_SMOOTH;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	lightningOpts.nQuality = 
	optDamage = 
	optExplosions = 
	optPlayers = 
	optRobots = 
	optStatic = 
	optRobotOmega = 
	optPlasma = 
	optAuxViews = 
	optMonitors = -1;

	ADD_CHECK (nOptions, TXT_LIGHTNING_ENABLE, extraGameInfo [0].bUseLightnings, KEY_U, HTX_LIGHTNING_ENABLE);
	lightningOpts.nUse = nOptions++;
	if (extraGameInfo [0].bUseLightnings) {
		sprintf (szQuality + 1, TXT_LIGHTNING_QUALITY, pszLightningQuality [gameOpts->render.lightnings.nQuality]);
		*szQuality = *(TXT_LIGHTNING_QUALITY - 1);
		ADD_SLIDER (nOptions, szQuality + 1, gameOpts->render.lightnings.nQuality, 0, 1, KEY_R, HTX_LIGHTNING_QUALITY);
		lightningOpts.nQuality = nOptions++;
		sprintf (szStyle + 1, TXT_LIGHTNING_STYLE, pszLightningStyle [gameOpts->render.lightnings.nStyle]);
		*szStyle = *(TXT_LIGHTNING_STYLE - 1);
		ADD_SLIDER (nOptions, szStyle + 1, gameOpts->render.lightnings.nStyle, 0, 2, KEY_S, HTX_LIGHTNING_STYLE);
		lightningOpts.nStyle = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_PLASMA, gameOpts->render.lightnings.bPlasma, KEY_L, HTX_LIGHTNING_PLASMA);
		optPlasma = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_DAMAGE, gameOpts->render.lightnings.bDamage, KEY_D, HTX_LIGHTNING_DAMAGE);
		optDamage = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_EXPLOSIONS, gameOpts->render.lightnings.bExplosions, KEY_E, HTX_LIGHTNING_EXPLOSIONS);
		optExplosions = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_PLAYERS, gameOpts->render.lightnings.bPlayers, KEY_P, HTX_LIGHTNING_PLAYERS);
		optPlayers = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_ROBOTS, gameOpts->render.lightnings.bRobots, KEY_R, HTX_LIGHTNING_ROBOTS);
		optRobots = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_STATIC, gameOpts->render.lightnings.bStatic, KEY_T, HTX_LIGHTNING_STATIC);
		optStatic = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_OMEGA, gameOpts->render.lightnings.bOmega, KEY_O, HTX_LIGHTNING_OMEGA);
		lightningOpts.nOmega = nOptions++;
		if (gameOpts->render.lightnings.bOmega) {
			ADD_CHECK (nOptions, TXT_LIGHTNING_ROBOT_OMEGA, gameOpts->render.lightnings.bRobotOmega, KEY_B, HTX_LIGHTNING_ROBOT_OMEGA);
			optRobotOmega = nOptions++;
			}
		ADD_CHECK (nOptions, TXT_LIGHTNING_AUXVIEWS, gameOpts->render.lightnings.bAuxViews, KEY_D, HTX_LIGHTNING_AUXVIEWS);
		optAuxViews = nOptions++;
		ADD_CHECK (nOptions, TXT_LIGHTNING_MONITORS, gameOpts->render.lightnings.bMonitors, KEY_M, HTX_LIGHTNING_MONITORS);
		optMonitors = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTNING_MENUTITLE, nOptions, m, &LightningOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if (extraGameInfo [0].bUseLightnings) {
		GET_VAL (gameOpts->render.lightnings.bPlasma, optPlasma);
		GET_VAL (gameOpts->render.lightnings.bDamage, optDamage);
		GET_VAL (gameOpts->render.lightnings.bExplosions, optExplosions);
		GET_VAL (gameOpts->render.lightnings.bPlayers, optPlayers);
		GET_VAL (gameOpts->render.lightnings.bRobots, optRobots);
		GET_VAL (gameOpts->render.lightnings.bStatic, optStatic);
		GET_VAL (gameOpts->render.lightnings.bRobotOmega, optRobotOmega);
		GET_VAL (gameOpts->render.lightnings.bAuxViews, optAuxViews);
		GET_VAL (gameOpts->render.lightnings.bMonitors, optMonitors);
		}
	} while (i == -2);
if (!gameOpts->render.lightnings.bPlayers)
	lightningManager.DestroyForPlayers ();
if (!gameOpts->render.lightnings.bRobots)
	lightningManager.DestroyForRobots ();
if (!gameOpts->render.lightnings.bStatic)
	lightningManager.DestroyStatic ();
}

//------------------------------------------------------------------------------

void MovieOptionsMenu ()
{
	tMenuItem m [5];
	int	i, choice = 0;
	int	nOptions;
	int	optMovieQual, optMovieSize, optSubTitles;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	ADD_CHECK (nOptions, TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_O, HTX_RENDER_SUBTTL);
	optSubTitles = nOptions++;
	if (gameOpts->app.bExpertMode) {
		ADD_CHECK (nOptions, TXT_MOVIE_QUAL, gameOpts->movies.nQuality, KEY_Q, HTX_RENDER_MOVIEQUAL);
		optMovieQual = nOptions++;
		ADD_CHECK (nOptions, TXT_MOVIE_FULLSCR, gameOpts->movies.bResize, KEY_U, HTX_RENDER_MOVIEFULL);
		optMovieSize = nOptions++;
		}
	else
		optMovieQual = 
		optMovieSize = -1;

	for (;;) {
		i = ExecMenu1 (NULL, TXT_MOVIE_OPTIONS, nOptions, m, NULL, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->movies.bSubTitles = m [optSubTitles].value;
	if (gameOpts->app.bExpertMode) {
		gameOpts->movies.nQuality = m [optMovieQual].value;
		gameOpts->movies.bResize = m [optMovieSize].value;
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static const char *pszShipColors [8];

int ShipRenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + shipRenderOpts.nWeapons;
v = m->value;
if (v != extraGameInfo [0].bShowWeapons) {
	extraGameInfo [0].bShowWeapons = v;
	*key = -2;
	}
if (shipRenderOpts.nColor >= 0) {
	m = menus + shipRenderOpts.nColor;
	v = m->value;
	if (v != gameOpts->render.ship.nColor) {
		gameOpts->render.ship.nColor = v;
		sprintf (m->text, TXT_SHIPCOLOR, pszShipColors [v]);
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void ShipRenderOptionsMenu (void)
{
	tMenuItem m [10];
	int	i, j, choice = 0;
	int	nOptions;
	int	optBullets, optWingtips;
	char	szShipColor [50];

pszShipColors [0] = TXT_SHIP_WHITE;
pszShipColors [1] = TXT_SHIP_BLUE;
pszShipColors [2] = TXT_RED;
pszShipColors [3] = TXT_SHIP_GREEN;
pszShipColors [4] = TXT_SHIP_YELLOW;
pszShipColors [5] = TXT_SHIP_PURPLE;
pszShipColors [6] = TXT_SHIP_ORANGE;
pszShipColors [7] = TXT_SHIP_CYAN;
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	ADD_CHECK (nOptions, TXT_SHIP_WEAPONS, extraGameInfo [0].bShowWeapons, KEY_W, HTX_SHIP_WEAPONS);
	shipRenderOpts.nWeapons = nOptions++;
	if (extraGameInfo [0].bShowWeapons) {
		ADD_CHECK (nOptions, TXT_SHIP_BULLETS, gameOpts->render.ship.bBullets, KEY_B, HTX_SHIP_BULLETS);
		optBullets = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_SHIP_WINGTIP_LASER, 0, KEY_A, 1, HTX_SHIP_WINGTIPS);
		optWingtips = nOptions++;
		ADD_RADIO (nOptions, TXT_SHIP_WINGTIP_SUPLAS, 0, KEY_U, 1, HTX_SHIP_WINGTIPS);
		nOptions++;
		ADD_RADIO (nOptions, TXT_SHIP_WINGTIP_SHORT, 0, KEY_S, 1, HTX_SHIP_WINGTIPS);
		nOptions++;
		ADD_RADIO (nOptions, TXT_SHIP_WINGTIP_LONG, 0, KEY_L, 1, HTX_SHIP_WINGTIPS);
		nOptions++;
		m [optWingtips + gameOpts->render.ship.nWingtip].value = 1;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_TEXT (nOptions, TXT_SHIPCOLOR_HEADER, 0);
		nOptions++;
		sprintf (szShipColor + 1, TXT_SHIPCOLOR, pszShipColors [gameOpts->render.ship.nColor]);
		*szShipColor = 0;
		ADD_SLIDER (nOptions, szShipColor + 1, gameOpts->render.ship.nColor, 0, 7, KEY_C, HTX_SHIPCOLOR);
		shipRenderOpts.nColor = nOptions++;
		}
	else
		optBullets =
		optWingtips =
		shipRenderOpts.nColor = -1;
	for (;;) {
		i = ExecMenu1 (NULL, TXT_SHIP_RENDERMENU, nOptions, m, ShipRenderOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	if ((extraGameInfo [0].bShowWeapons = m [shipRenderOpts.nWeapons].value)) {
		gameOpts->render.ship.bBullets = m [optBullets].value;
		for (j = 0; j < 4; j++)
			if (m [optWingtips + j].value) {
				gameOpts->render.ship.nWingtip = j;
				break;
				}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

int RenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + optBrightness;
	v = m->value;
	if (v != paletteManager.GetGamma ())
		paletteManager.SetGamma (v);
	}
m = menus + renderOpts.nMaxFPS;
v = fpsTable [m->value];
if (gameOpts->render.nMaxFPS != (v ? v : 1)) {
	gameOpts->render.nMaxFPS = v ? v : 1;
	if (v)
		sprintf (m->text, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (m->text, TXT_NO_FRAMECAP);
	m->rebuild = 1;
	}
if (gameOpts->app.bExpertMode) {
	if (optContrast >= 0) {
		m = menus + optContrast;
		v = m->value;
		if (v != gameStates.ogl.nContrast) {
			gameStates.ogl.nContrast = v;
			sprintf (m->text, TXT_CONTRAST, ContrastText ());
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nRenderQual;
	v = m->value;
	if (gameOpts->render.nQuality != v) {
		gameOpts->render.nQuality = v;
		sprintf (m->text, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		m->rebuild = 1;
		}
	if (renderOpts.nTexQual > 0) {
		m = menus + renderOpts.nTexQual;
		v = m->value;
		if (gameOpts->render.textures.nQuality != v) {
			gameOpts->render.textures.nQuality = v;
			sprintf (m->text, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			m->rebuild = 1;
			}
		}
	if (renderOpts.nMeshQual > 0) {
		m = menus + renderOpts.nMeshQual;
		v = m->value;
		if (gameOpts->render.nMeshQuality != v) {
			gameOpts->render.nMeshQuality = v;
			sprintf (m->text, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
			m->rebuild = 1;
			}
		}
	m = menus + renderOpts.nWallTransp;
	v = (FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	tMenuItem m [40];
	int	h, i, choice = 0;
	int	nOptions;
	int	optSmokeOpts, optShadowOpts, optCameraOpts, optLightOpts, optMovieOpts,
			optAdvOpts, optEffectOpts, optPowerupOpts, optAutomapOpts, optLightningOpts,
			optUseGamma, optColoredWalls, optDepthSort, optCoronaOpts, optShipRenderOpts;
#if DBG
	int	optWireFrame, optTextures, optObjects, optWalls, optDynLight;
#endif

	char szMaxFps [50];
	char szWallTransp [50];
	char szRendQual [50];
	char szTexQual [50];
	char szMeshQual [50];
	char szContrast [50];

	int nRendQualSave = gameOpts->render.nQuality;

	pszRendQual [0] = TXT_QUALITY_LOW;
	pszRendQual [1] = TXT_QUALITY_MED;
	pszRendQual [2] = TXT_QUALITY_HIGH;
	pszRendQual [3] = TXT_VERY_HIGH;
	pszRendQual [4] = TXT_QUALITY_MAX;

	pszTexQual [0] = TXT_QUALITY_LOW;
	pszTexQual [1] = TXT_QUALITY_MED;
	pszTexQual [2] = TXT_QUALITY_HIGH;
	pszTexQual [3] = TXT_QUALITY_MAX;

	pszMeshQual [0] = TXT_NONE;
	pszMeshQual [1] = TXT_SMALL;
	pszMeshQual [2] = TXT_MEDIUM;
	pszMeshQual [3] = TXT_HIGH;
	pszMeshQual [4] = TXT_EXTREME;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	optPowerupOpts = optAutomapOpts = -1;
	if (!gameStates.app.bNostalgia) {
		ADD_SLIDER (nOptions, TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = nOptions++;
		}
	if (gameOpts->render.nMaxFPS > 1)
		sprintf (szMaxFps + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (szMaxFps + 1, TXT_NO_FRAMECAP);
	*szMaxFps = *(TXT_FRAMECAP - 1);
	ADD_SLIDER (nOptions, szMaxFps + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, 15, KEY_F, HTX_RENDER_FRAMECAP);
	renderOpts.nMaxFPS = nOptions++;

	if (gameOpts->app.bExpertMode) {
		if ((gameOpts->render.nLightingMethod < 2) && !gameOpts->render.bUseLightmaps) {
			sprintf (szContrast, TXT_CONTRAST, ContrastText ());
			ADD_SLIDER (nOptions, szContrast, gameStates.ogl.nContrast, 0, 16, KEY_C, HTX_ADVRND_CONTRAST);
			optContrast = nOptions++;
			}
		sprintf (szRendQual + 1, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		*szRendQual = *(TXT_RENDQUAL - 1);
		ADD_SLIDER (nOptions, szRendQual + 1, gameOpts->render.nQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);
		renderOpts.nRenderQual = nOptions++;
		if (gameStates.app.bGameRunning)
			renderOpts.nTexQual =
			renderOpts.nMeshQual = 
			lightOpts.nLightmapQual = -1;
		else {
			sprintf (szTexQual + 1, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			*szTexQual = *(TXT_TEXQUAL + 1);
			ADD_SLIDER (nOptions, szTexQual + 1, gameOpts->render.textures.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);
			renderOpts.nTexQual = nOptions++;
			if ((gameOpts->render.nLightingMethod == 1) && !gameOpts->render.bUseLightmaps) {
				sprintf (szMeshQual + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
				*szMeshQual = *(TXT_MESH_QUALITY + 1);
				ADD_SLIDER (nOptions, szMeshQual + 1, gameOpts->render.nMeshQuality, 0, 4, KEY_O, HTX_MESH_QUALITY);
				renderOpts.nMeshQual = nOptions++;
				}
			else
				renderOpts.nMeshQual = -1;
			}
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		h = extraGameInfo [0].grWallTransparency * 10 / FADE_LEVELS;
		sprintf (szWallTransp + 1, TXT_WALL_TRANSP, h * 10, '%');
		*szWallTransp = *(TXT_WALL_TRANSP - 1);
		ADD_SLIDER (nOptions, szWallTransp + 1, h, 0, 10, KEY_T, HTX_ADVRND_WALLTRANSP);
		renderOpts.nWallTransp = nOptions++;
		ADD_CHECK (nOptions, TXT_COLOR_WALLS, gameOpts->render.color.bWalls, KEY_W, HTX_ADVRND_COLORWALLS);
		optColoredWalls = nOptions++;
		if (RENDERPATH)
			optDepthSort = -1;
		else {
			ADD_CHECK (nOptions, TXT_TRANSP_DEPTH_SORT, gameOpts->render.bDepthSort, KEY_D, HTX_TRANSP_DEPTH_SORT);
			optDepthSort = nOptions++;
			}
#if 0
		ADD_CHECK (nOptions, TXT_GAMMA_BRIGHT, gameOpts->ogl.bSetGammaRamp, KEY_V, HTX_ADVRND_GAMMA);
		optUseGamma = nOptions++;
#else
		optUseGamma = -1;
#endif
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_MENU (nOptions, TXT_LIGHTING_OPTIONS, KEY_L, HTX_RENDER_LIGHTINGOPTS);
		optLightOpts = nOptions++;
		ADD_MENU (nOptions, TXT_SMOKE_OPTIONS, KEY_S, HTX_RENDER_SMOKEOPTS);
		optSmokeOpts = nOptions++;
		ADD_MENU (nOptions, TXT_LIGHTNING_OPTIONS, KEY_I, HTX_LIGHTNING_OPTIONS);
		optLightningOpts = nOptions++;
		if (!(gameStates.app.bEnableShadows && gameStates.render.bHaveStencilBuffer))
			optShadowOpts = -1;
		else {
			ADD_MENU (nOptions, TXT_SHADOW_OPTIONS, KEY_A, HTX_RENDER_SHADOWOPTS);
			optShadowOpts = nOptions++;
			}
		ADD_MENU (nOptions, TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
		optEffectOpts = nOptions++;
		ADD_MENU (nOptions, TXT_CORONA_OPTIONS, KEY_O, HTX_RENDER_CORONAOPTS);
		optCoronaOpts = nOptions++;
		ADD_MENU (nOptions, TXT_CAMERA_OPTIONS, KEY_C, HTX_RENDER_CAMERAOPTS);
		optCameraOpts = nOptions++;
		ADD_MENU (nOptions, TXT_POWERUP_OPTIONS, KEY_P, HTX_RENDER_PRUPOPTS);
		optPowerupOpts = nOptions++;
		ADD_MENU (nOptions, TXT_AUTOMAP_OPTIONS, KEY_M, HTX_RENDER_AUTOMAPOPTS);
		optAutomapOpts = nOptions++;
		ADD_MENU (nOptions, TXT_SHIP_RENDEROPTIONS, KEY_H, HTX_RENDER_SHIPOPTS);
		optShipRenderOpts = nOptions++;
		ADD_MENU (nOptions, TXT_MOVIE_OPTIONS, KEY_M, HTX_RENDER_MOVIEOPTS);
		optMovieOpts = nOptions++;
		}
	else
		renderOpts.nRenderQual =
		renderOpts.nTexQual =
		renderOpts.nMeshQual =
		renderOpts.nWallTransp = 
		optUseGamma = 
		optColoredWalls =
		optDepthSort =
		optContrast =
		optLightOpts =
		optLightningOpts =
		optSmokeOpts =
		optShadowOpts =
		optEffectOpts =
		optCoronaOpts =
		optCameraOpts = 
		optMovieOpts = 
		optShipRenderOpts =
		optAdvOpts = -1;

#if DBG
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_CHECK (nOptions, "Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
	optWireFrame = nOptions++;
	ADD_CHECK (nOptions, "Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
	optTextures = nOptions++;
	ADD_CHECK (nOptions, "Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
	optWalls = nOptions++;
	ADD_CHECK (nOptions, "Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
	optObjects = nOptions++;
	ADD_CHECK (nOptions, "Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
	optDynLight = nOptions++;
#endif

	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_RENDER_OPTS, nOptions, m, &RenderOptionsCallback, &choice);
		if (i < 0)
			break;
		if (gameOpts->app.bExpertMode) {
			if ((optLightOpts >= 0) && (i == optLightOpts))
				i = -2, LightOptionsMenu ();
			else if ((optSmokeOpts >= 0) && (i == optSmokeOpts))
				i = -2, SmokeOptionsMenu ();
			else if ((optLightningOpts >= 0) && (i == optLightningOpts))
				i = -2, LightningOptionsMenu ();
			else if ((optShadowOpts >= 0) && (i == optShadowOpts))
				i = -2, ShadowOptionsMenu ();
			else if ((optEffectOpts >= 0) && (i == optEffectOpts))
				i = -2, EffectOptionsMenu ();
			else if ((optCoronaOpts >= 0) && (i == optCoronaOpts))
				i = -2, CoronaOptionsMenu ();
			else if ((optCameraOpts >= 0) && (i == optCameraOpts))
				i = -2, CameraOptionsMenu ();
			else if ((optPowerupOpts >= 0) && (i == optPowerupOpts))
				i = -2, PowerupOptionsMenu ();
			else if ((optAutomapOpts >= 0) && (i == optAutomapOpts))
				i = -2, AutomapOptionsMenu ();
			else if ((optMovieOpts >= 0) && (i == optMovieOpts))
				i = -2, MovieOptionsMenu ();
			else if ((optShipRenderOpts >= 0) && (i == optShipRenderOpts))
				i = -2, ShipRenderOptionsMenu ();
			}
		} while (i >= 0);
	if (!gameStates.app.bNostalgia)
		paletteManager.SetGamma (m [optBrightness].value);
	if (gameOpts->app.bExpertMode) {
		gameOpts->render.color.bWalls = m [optColoredWalls].value;
		GET_VAL (gameOpts->render.bDepthSort, optDepthSort);
		GET_VAL (gameOpts->ogl.bSetGammaRamp, optUseGamma);
		if (optContrast >= 0)
			gameStates.ogl.nContrast = m [optContrast].value;
		if (nRendQualSave != gameOpts->render.nQuality)
			SetRenderQuality ();
		}
#if EXPMODE_DEFAULTS
	else {
		gameOpts->render.nMaxFPS = 250;
		gameOpts->render.color.nLightmapRange = 5;
		gameOpts->render.color.bMix = 1;
		gameOpts->render.nQuality = 3;
		gameOpts->render.color.bWalls = 1;
		gameOpts->render.effects.bTransparent = 1;
		gameOpts->render.particles.bPlayers = 0;
		gameOpts->render.particles.bRobots =
		gameOpts->render.particles.bMissiles = 1;
		gameOpts->render.particles.bCollisions = 0;
		gameOpts->render.particles.bDisperse = 0;
		gameOpts->render.particles.nDens = 2;
		gameOpts->render.particles.nSize = 3;
		gameOpts->render.cameras.bFitToWall = 0;
		gameOpts->render.cameras.nSpeed = 5000;
		gameOpts->render.cameras.nFPS = 0;
		gameOpts->movies.nQuality = 0;
		gameOpts->movies.bResize = 1;
		gameStates.ogl.nContrast = 8;
		gameOpts->ogl.bSetGammaRamp = 0;
		}
#endif
#if DBG
	gameOpts->render.debug.bWireFrame = m [optWireFrame].value;
	gameOpts->render.debug.bTextures = m [optTextures].value;
	gameOpts->render.debug.bObjects = m [optObjects].value;
	gameOpts->render.debug.bWalls = m [optWalls].value;
	gameOpts->render.debug.bDynamicLight = m [optDynLight].value;
#endif
	} while (i == -2);
}

//------------------------------------------------------------------------------

static const char *pszGuns [] = {"Laser", "Vulcan", "Spreadfire", "Plasma", "Fusion", "Super Laser", "Gauss", "Helix", "Phoenix", "Omega"};
static const char *pszDevices [] = {"Full Map", "Ammo Rack", "Converter", "Quad Lasers", "Afterburner", "Headlight", "Slow Motion", "Bullet Time"};
static int nDeviceFlags [] = {PLAYER_FLAGS_FULLMAP, PLAYER_FLAGS_AMMO_RACK, PLAYER_FLAGS_CONVERTER, PLAYER_FLAGS_QUAD_LASERS, 
										PLAYER_FLAGS_AFTERBURNER, PLAYER_FLAGS_HEADLIGHT, PLAYER_FLAGS_SLOWMOTION, PLAYER_FLAGS_BULLETTIME};

static int optGuns, optDevices;

//------------------------------------------------------------------------------

static inline void SetGunLoadoutFlag (int i, int v)
{
if (v)
	extraGameInfo [0].loadout.nGuns |= (1 << i);
else
	extraGameInfo [0].loadout.nGuns &= ~(1 << i);
}

//------------------------------------------------------------------------------

static inline int GetGunLoadoutFlag (int i)
{
return (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0;
}

//------------------------------------------------------------------------------

static inline void SetDeviceLoadoutFlag (int i, int v)
{
if (v)
	extraGameInfo [0].loadout.nDevices |= nDeviceFlags [i];
else
	extraGameInfo [0].loadout.nDevices &= ~nDeviceFlags [i];
}

//------------------------------------------------------------------------------

static inline int GetDeviceLoadoutFlag (int i)
{
return (extraGameInfo [0].loadout.nDevices & nDeviceFlags [i]) != 0;
}

//------------------------------------------------------------------------------

int LoadoutCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m = menus + nCurItem;
	int			v = m->value;

if (nCurItem == optGuns) {	//checked/unchecked lasers
	if (v != GetGunLoadoutFlag (0)) {
		SetGunLoadoutFlag (0, v);
		if (!v) {	//if lasers unchecked, also uncheck super lasers
			SetGunLoadoutFlag (5, 0);
			menus [optGuns + 5].value = 0;
			}
		}
	}
else if (nCurItem == optGuns + 5) {	//checked/unchecked super lasers
	if (v != GetGunLoadoutFlag (5)) {
		SetGunLoadoutFlag (5, v);
		if (v) {	// if super lasers checked, also check lasers
			SetGunLoadoutFlag (0, 1);
			menus [optGuns].value = 1;
			}
		}
	}
else if (nCurItem == optDevices + 6) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (6)) {
		SetDeviceLoadoutFlag (6, v);
		if (!v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (7, 0);
			menus [optDevices + 7].value = 0;
			}
		}
	}
else if (nCurItem == optDevices + 7) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (7)) {
		SetDeviceLoadoutFlag (7, v);
		if (v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (6, 1);
			menus [optDevices + 6].value = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LoadoutOptions (void)
{
	tMenuItem	m [25];
	int			i, nOptions = 0;

memset (m, 0, sizeof (m));
ADD_TEXT (nOptions, TXT_GUN_LOADOUT, 0);
nOptions++;
for (i = 0, optGuns = nOptions; i < (int) sizeofa (pszGuns); i++, nOptions++)
	ADD_CHECK (nOptions, pszGuns [i], (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0, 0, HTX_GUN_LOADOUT);
ADD_TEXT (nOptions, "", 0);
nOptions++;
ADD_TEXT (nOptions, TXT_DEVICE_LOADOUT, 0);
nOptions++;
for (i = 0, optDevices = nOptions; i < (int) sizeofa (pszDevices); i++, nOptions++)
	ADD_CHECK (nOptions, pszDevices [i], (extraGameInfo [0].loadout.nDevices & (nDeviceFlags [i])) != 0, 0, HTX_DEVICE_LOADOUT);
Assert (sizeofa (m) >= (size_t) nOptions);
do {
	i = ExecMenu1 (NULL, TXT_LOADOUT_MENUTITLE, nOptions, m, LoadoutCallback, 0);
	} while (i != -1);
extraGameInfo [0].loadout.nGuns = 0;
for (i = 0; i < (int) sizeofa (pszGuns); i++) {
	if (m [optGuns + i].value)
		extraGameInfo [0].loadout.nGuns |= (1 << i);
	}
extraGameInfo [0].loadout.nDevices = 0;
for (i = 0; i < (int) sizeofa (pszDevices); i++) {
	if (m [optDevices + i].value)
		extraGameInfo [0].loadout.nDevices |= nDeviceFlags [i];
	}
AddPlayerLoadout ();
}

//------------------------------------------------------------------------------

static const char *pszMslTurnSpeeds [3];
static const char *pszMslStartSpeeds [4];
static const char *pszAggressivities [5];

int GameplayOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

m = menus + gplayOpts.nDifficulty;
v = m->value;
if (gameStates.app.nDifficultyLevel != v) {
	gameStates.app.nDifficultyLevel = v;
	if (!IsMultiGame) {
		gameStates.app.nDifficultyLevel = v;
		InitGateIntervals ();
		}
	sprintf (m->text, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	m->rebuild = 1;
	}

m = menus + gplayOpts.nHeadlightAvailable;
v = m->value;
if (extraGameInfo [0].headlight.bAvailable != v) {
	extraGameInfo [0].headlight.bAvailable = v;
	*key = -2;
	return nCurItem;
	}

if (gameOpts->app.bExpertMode) {
	m = menus + gplayOpts.nSpawnDelay;
	v = (m->value - 1) * 5;
	if (extraGameInfo [0].nSpawnDelay != v * 1000) {
		extraGameInfo [0].nSpawnDelay = v * 1000;
		sprintf (m->text, TXT_RESPAWN_DELAY, (v < 0) ? -1 : v);
		m->rebuild = 1;
		}

	m = menus + gplayOpts.nSmokeGrens;
	v = m->value;
	if (extraGameInfo [0].bSmokeGrenades != v) {
		extraGameInfo [0].bSmokeGrenades = v;
		*key = -2;
		return nCurItem;
		}

	m = menus + gplayOpts.nAIAggressivity;
	v = m->value;
	if (gameOpts->gameplay.nAIAggressivity != v) {
		gameOpts->gameplay.nAIAggressivity = v;
		sprintf (m->text, TXT_AI_AGGRESSIVITY, pszAggressivities [v]);
		m->rebuild = 1;
		}

	if (gplayOpts.nMaxSmokeGrens >= 0) {
		m = menus + gplayOpts.nMaxSmokeGrens;
		v = m->value + 1;
		if (extraGameInfo [0].nMaxSmokeGrenades != v) {
			extraGameInfo [0].nMaxSmokeGrenades = v;
			sprintf (m->text, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
			m->rebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void GameplayOptionsMenu (void)
{
	tMenuItem m [40];
	int	i, j, nOptions = 0, choice = 0;
	int	optFixedSpawn = -1, optSnipeMode = -1, optAutoSel = -1, optInventory = -1, 
			optDualMiss = -1, optDropAll = -1, optImmortal = -1, optMultiBosses = -1, optTripleFusion = -1,
			optEnhancedShakers = -1, optSmartWeaponSwitch = -1, optWeaponDrop = -1, optIdleAnims = -1, 
			optAwareness = -1, optHeadlightBuiltIn = -1, optHeadlightPowerDrain = -1, optHeadlightOnWhenPickedUp = -1,
			optRotateMarkers = -1, optLoadout, optUseD1AI = -1, optNoThief = -1;
	char	szRespawnDelay [60];
	char	szDifficulty [50], szMaxSmokeGrens [50], szAggressivity [50];

pszAggressivities [0] = TXT_STANDARD;
pszAggressivities [1] = TXT_MODERATE;
pszAggressivities [2] = TXT_MEDIUM;
pszAggressivities [3] = TXT_HIGH;
pszAggressivities [4] = TXT_VERY_HIGH;

do {
	memset (&m, 0, sizeof (m));
	nOptions = 0;
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	ADD_SLIDER (nOptions, szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	gplayOpts.nDifficulty = nOptions++;
	if (gameOpts->app.bExpertMode) {
		sprintf (szRespawnDelay + 1, TXT_RESPAWN_DELAY, (extraGameInfo [0].nSpawnDelay < 0) ? -1 : extraGameInfo [0].nSpawnDelay / 1000);
		*szRespawnDelay = *(TXT_RESPAWN_DELAY - 1);
		ADD_SLIDER (nOptions, szRespawnDelay + 1, extraGameInfo [0].nSpawnDelay / 5000 + 1, 0, 13, KEY_R, HTX_GPLAY_SPAWNDELAY);
		gplayOpts.nSpawnDelay = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_HEADLIGHT_AVAILABLE, extraGameInfo [0].headlight.bAvailable, KEY_A, HTX_HEADLIGHT_AVAILABLE);
		gplayOpts.nHeadlightAvailable = nOptions++;
		if (extraGameInfo [0].headlight.bAvailable) {
			ADD_CHECK (nOptions, TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_O, HTX_MISC_HEADLIGHT);
			optHeadlightOnWhenPickedUp = nOptions++;
			ADD_CHECK (nOptions, TXT_HEADLIGHT_BUILTIN, extraGameInfo [0].headlight.bBuiltIn, KEY_L, HTX_HEADLIGHT_BUILTIN);
			optHeadlightBuiltIn = nOptions++;
			ADD_CHECK (nOptions, TXT_HEADLIGHT_POWERDRAIN, extraGameInfo [0].headlight.bDrainPower, KEY_W, HTX_HEADLIGHT_POWERDRAIN);
			optHeadlightPowerDrain = nOptions++;
			}
		ADD_CHECK (nOptions, TXT_USE_INVENTORY, gameOpts->gameplay.bInventory, KEY_V, HTX_GPLAY_INVENTORY);
		optInventory = nOptions++;
		ADD_CHECK (nOptions, TXT_ROTATE_MARKERS, extraGameInfo [0].bRotateMarkers, KEY_K, HTX_ROTATE_MARKERS);
		optRotateMarkers = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_MULTI_BOSSES, extraGameInfo [0].bMultiBosses, KEY_M, HTX_GPLAY_MULTIBOSS);
		optMultiBosses = nOptions++;
		ADD_CHECK (nOptions, TXT_IDLE_ANIMS, gameOpts->gameplay.bIdleAnims, KEY_D, HTX_GPLAY_IDLEANIMS);
		optIdleAnims = nOptions++;
		ADD_CHECK (nOptions, TXT_SUPPRESS_THIEF, gameOpts->gameplay.bNoThief, KEY_T, HTX_SUPPRESS_THIEF);
		optNoThief = nOptions++;
		ADD_CHECK (nOptions, TXT_AI_AWARENESS, gameOpts->gameplay.nAIAwareness, KEY_I, HTX_AI_AWARENESS);
		optAwareness = nOptions++;
		sprintf (szAggressivity + 1, TXT_AI_AGGRESSIVITY, pszAggressivities [gameOpts->gameplay.nAIAggressivity]);
		*szAggressivity = *(TXT_AI_AGGRESSIVITY - 1);
		ADD_SLIDER (nOptions, szAggressivity + 1, gameOpts->gameplay.nAIAggressivity, 0, 4, KEY_G, HTX_AI_AGGRESSIVITY);
		gplayOpts.nAIAggressivity = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_ALWAYS_RESPAWN, extraGameInfo [0].bImmortalPowerups, KEY_P, HTX_GPLAY_ALWAYSRESP);
		optImmortal = nOptions++;
		ADD_CHECK (nOptions, TXT_FIXED_SPAWN, extraGameInfo [0].bFixedRespawns, KEY_F, HTX_GPLAY_FIXEDSPAWN);
		optFixedSpawn = nOptions++;
		ADD_CHECK (nOptions, TXT_DROP_ALL, extraGameInfo [0].bDropAllMissiles, KEY_A, HTX_GPLAY_DROPALL);
		optDropAll = nOptions++;
		ADD_CHECK (nOptions, TXT_DROP_QUADSUPER, extraGameInfo [0].nWeaponDropMode, KEY_Q, HTX_GPLAY_DROPQUAD);
		optWeaponDrop = nOptions++;
		ADD_CHECK (nOptions, TXT_DUAL_LAUNCH, extraGameInfo [0].bDualMissileLaunch, KEY_U, HTX_GPLAY_DUALLAUNCH);
		optDualMiss = nOptions++;
		ADD_CHECK (nOptions, TXT_TRIPLE_FUSION, extraGameInfo [0].bTripleFusion, KEY_U, HTX_GPLAY_TRIFUSION);
		optTripleFusion = nOptions++;
		ADD_CHECK (nOptions, TXT_ENHANCED_SHAKERS, extraGameInfo [0].bEnhancedShakers, KEY_B, HTX_ENHANCED_SHAKERS);
		optEnhancedShakers = nOptions++;
		ADD_CHECK (nOptions, TXT_USE_D1_AI, gameOpts->gameplay.bUseD1AI, KEY_I, HTX_USE_D1_AI);
		optUseD1AI = nOptions++;
		if (extraGameInfo [0].bSmokeGrenades) {
			ADD_TEXT (nOptions, "", 0);
			nOptions++;
			}
		ADD_CHECK (nOptions, TXT_GPLAY_SMOKEGRENADES, extraGameInfo [0].bSmokeGrenades, KEY_S, HTX_GPLAY_SMOKEGRENADES);
		gplayOpts.nSmokeGrens = nOptions++;
		if (extraGameInfo [0].bSmokeGrenades) {
			sprintf (szMaxSmokeGrens + 1, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
			*szMaxSmokeGrens = *(TXT_MAX_SMOKEGRENS - 1);
			ADD_SLIDER (nOptions, szMaxSmokeGrens + 1, extraGameInfo [0].nMaxSmokeGrenades - 1, 0, 3, KEY_X, HTX_GPLAY_MAXGRENADES);
			gplayOpts.nMaxSmokeGrens = nOptions++;
			}
		else
			gplayOpts.nMaxSmokeGrens = -1;
		}
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_CHECK (nOptions, TXT_SMART_WPNSWITCH, extraGameInfo [0].bSmartWeaponSwitch, KEY_W, HTX_GPLAY_SMARTSWITCH);
	optSmartWeaponSwitch = nOptions++;
	optAutoSel = nOptions;
	ADD_RADIO (nOptions, TXT_WPNSEL_NEVER, 0, KEY_N, 2, HTX_GPLAY_WSELNEVER);
	nOptions++;
	ADD_RADIO (nOptions, TXT_WPNSEL_EMPTY, 0, KEY_Y, 2, HTX_GPLAY_WSELEMPTY);
	nOptions++;
	ADD_RADIO (nOptions, TXT_WPNSEL_ALWAYS, 0, KEY_T, 2, HTX_GPLAY_WSELALWAYS);
	nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	optSnipeMode = nOptions;
	ADD_RADIO (nOptions, TXT_ZOOM_OFF, 0, KEY_D, 3, HTX_GPLAY_ZOOMOFF);
	nOptions++;
	ADD_RADIO (nOptions, TXT_ZOOM_FIXED, 0, KEY_X, 3, HTX_GPLAY_ZOOMFIXED);
	nOptions++;
	ADD_RADIO (nOptions, TXT_ZOOM_SMOOTH, 0, KEY_Z, 3, HTX_GPLAY_ZOOMSMOOTH);
	nOptions++;
	m [optAutoSel + NMCLAMP (gameOpts->gameplay.nAutoSelectWeapon, 0, 2)].value = 1;
	m [optSnipeMode + NMCLAMP (extraGameInfo [0].nZoomMode, 0, 2)].value = 1;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_MENU (nOptions, TXT_LOADOUT_OPTION, KEY_L, HTX_MULTI_LOADOUT);
	optLoadout = nOptions++;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		if (0 > (i = ExecMenu1 (NULL, TXT_GAMEPLAY_OPTS, nOptions, m, &GameplayOptionsCallback, &choice)))
			break;
		if (choice == optLoadout)
			LoadoutOptions ();
		};
	} while (i == -2);
if (gameOpts->app.bExpertMode) {
	extraGameInfo [0].headlight.bAvailable = m [gplayOpts.nHeadlightAvailable].value;
	extraGameInfo [0].bFixedRespawns = m [optFixedSpawn].value;
	extraGameInfo [0].bSmokeGrenades = m [gplayOpts.nSmokeGrens].value;
	extraGameInfo [0].bDualMissileLaunch = m [optDualMiss].value;
	extraGameInfo [0].bDropAllMissiles = m [optDropAll].value;
	extraGameInfo [0].bImmortalPowerups = m [optImmortal].value;
	extraGameInfo [0].bMultiBosses = m [optMultiBosses].value;
	extraGameInfo [0].bSmartWeaponSwitch = m [optSmartWeaponSwitch].value;
	extraGameInfo [0].bTripleFusion = m [optTripleFusion].value;
	extraGameInfo [0].bEnhancedShakers = m [optEnhancedShakers].value;
	extraGameInfo [0].bRotateMarkers = m [optRotateMarkers].value;
	extraGameInfo [0].nWeaponDropMode = m [optWeaponDrop].value;
	GET_VAL (gameOpts->gameplay.bInventory, optInventory);
	GET_VAL (gameOpts->gameplay.bIdleAnims, optIdleAnims);
	GET_VAL (gameOpts->gameplay.nAIAwareness, optAwareness);
	GET_VAL (gameOpts->gameplay.bUseD1AI, optUseD1AI);
	GET_VAL (gameOpts->gameplay.bNoThief, optNoThief);
	GET_VAL (gameOpts->gameplay.bHeadlightOnWhenPickedUp, optHeadlightOnWhenPickedUp);
	GET_VAL (extraGameInfo [0].headlight.bDrainPower, optHeadlightPowerDrain);
	GET_VAL (extraGameInfo [0].headlight.bBuiltIn, optHeadlightBuiltIn);
	}
else {
#if EXPMODE_DEFAULTS
	extraGameInfo [0].bFixedRespawns = 0;
	extraGameInfo [0].bDualMissileLaunch = 0;
	extraGameInfo [0].bDropAllMissiles = 0;
	extraGameInfo [0].bImmortalPowerups = 0;
	extraGameInfo [0].bMultiBosses = 1;
	extraGameInfo [0].bSmartWeaponSwitch = 0;
	extraGameInfo [0].nWeaponDropMode = 1;
	gameOpts->gameplay.bInventory = 0;
	gameOpts->gameplay.bIdleAnims = 0;
	gameOpts->gameplay.nAIAwareness = 0;
	gameOpts->gameplay.bHeadlightOnWhenPickedUp = 0;
#endif
	}
for (j = 0; j < 3; j++)
	if (m [optAutoSel + j].value) {
		gameOpts->gameplay.nAutoSelectWeapon = j;
		break;
		}
for (j = 0; j < 3; j++)
	if (m [optSnipeMode + j].value) {
		extraGameInfo [0].nZoomMode = j;
		break;
		}
if (!COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 4;
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

//------------------------------------------------------------------------------

static int nOptDebrisLife;

//------------------------------------------------------------------------------

static const char *OmegaRampStr (void)
{
	static char szRamp [20];

if (extraGameInfo [0].nOmegaRamp == 0)
	return "1 sec";
sprintf (szRamp, "%d secs", nOmegaDuration [(int) extraGameInfo [0].nOmegaRamp]);
return szRamp;
}

//------------------------------------------------------------------------------

int PhysicsOptionsCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem	*m;
	int			v;

if (gameOpts->app.bExpertMode) {
	m = menus + physOpts.nDrag;
	v = m->value;
	if (extraGameInfo [0].nDrag != v) {
		extraGameInfo [0].nDrag = v;
		sprintf (m->text, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		m->rebuild = 1;
		}

	m = menus + physOpts.nSpeedboost;
	v = m->value;
	if (extraGameInfo [0].nSpeedBoost != v) {
		extraGameInfo [0].nSpeedBoost = v;
		sprintf (m->text, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		m->rebuild = 1;
		}

	m = menus + physOpts.nFusionRamp;
	v = m->value + 2;
	if (extraGameInfo [0].nFusionRamp != v) {
		extraGameInfo [0].nFusionRamp = v;
		sprintf (m->text, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
		m->rebuild = 1;
		}

	m = menus + physOpts.nOmegaRamp;
	v = m->value;
	if (extraGameInfo [0].nOmegaRamp != v) {
		extraGameInfo [0].nOmegaRamp = v;
		SetMaxOmegaCharge ();
		sprintf (m->text, TXT_OMEGA_RAMP, OmegaRampStr ());
		m->rebuild = 1;
		}

	m = menus + physOpts.nMslTurnSpeed;
	v = m->value;
	if (extraGameInfo [0].nMslTurnSpeed != v) {
		extraGameInfo [0].nMslTurnSpeed = v;
		sprintf (m->text, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [v]);
		m->rebuild = 1;
		}

	m = menus + physOpts.nMslStartSpeed;
	v = m->value;
	if (extraGameInfo [0].nMslStartSpeed != 3 - v) {
		extraGameInfo [0].nMslStartSpeed = 3 - v;
		sprintf (m->text, TXT_MSL_STARTSPEED, pszMslStartSpeeds [v]);
		m->rebuild = 1;
		}

	m = menus + physOpts.nSlomoSpeedup;
	v = m->value + 4;
	if (gameOpts->gameplay.nSlowMotionSpeedup != v) {
		gameOpts->gameplay.nSlowMotionSpeedup = v;
		sprintf (m->text, TXT_SLOWMOTION_SPEEDUP, (float) v / 2);
		m->rebuild = 1;
		return nCurItem;
		}

	m = menus + nOptDebrisLife;
	v = m->value;
	if (gameOpts->render.nDebrisLife != v) {
		gameOpts->render.nDebrisLife = v;
		sprintf (m->text, TXT_DEBRIS_LIFE, nDebrisLife [v]);
		m->rebuild = -1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void PhysicsOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, nOptions = 0, choice = 0;
	int	optRobHits = -1, optWiggle = -1, optAutoLevel = -1,
			optFluidPhysics = -1, optHitAngles = -1, optKillMissiles = -1, optHitboxes = -1;
	char	szSpeedBoost [50], szMslTurnSpeed [50], szMslStartSpeed [50], szSlowmoSpeedup [50], 
			szFusionRamp [50], szOmegaRamp [50], szDebrisLife [50], szDrag [50];

pszMslTurnSpeeds [0] = TXT_SLOW;
pszMslTurnSpeeds [1] = TXT_MEDIUM;
pszMslTurnSpeeds [2] = TXT_STANDARD;

pszMslStartSpeeds [0] = TXT_VERY_SLOW;
pszMslStartSpeeds [1] = TXT_SLOW;
pszMslStartSpeeds [2] = TXT_MEDIUM;
pszMslStartSpeeds [3] = TXT_STANDARD;

do {
	memset (&m, 0, sizeof (m));
	nOptions = 0;
	if (gameOpts->app.bExpertMode) {
		sprintf (szSpeedBoost + 1, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		*szSpeedBoost = *(TXT_SPEEDBOOST - 1);
		ADD_SLIDER (nOptions, szSpeedBoost + 1, extraGameInfo [0].nSpeedBoost, 0, 10, KEY_B, HTX_GPLAY_SPEEDBOOST);
		physOpts.nSpeedboost = nOptions++;
		sprintf (szFusionRamp + 1, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
		*szFusionRamp = *(TXT_FUSION_RAMP - 1);
		ADD_SLIDER (nOptions, szFusionRamp + 1, extraGameInfo [0].nFusionRamp - 2, 0, 6, KEY_U, HTX_FUSION_RAMP);
		physOpts.nFusionRamp = nOptions++;
		sprintf (szOmegaRamp + 1, TXT_OMEGA_RAMP, OmegaRampStr ());
		*szOmegaRamp = *(TXT_OMEGA_RAMP - 1);
		ADD_SLIDER (nOptions, szOmegaRamp + 1, extraGameInfo [0].nOmegaRamp, 0, 6, KEY_O, HTX_OMEGA_RAMP);
		physOpts.nOmegaRamp = nOptions++;
		sprintf (szMslTurnSpeed + 1, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [(int) extraGameInfo [0].nMslTurnSpeed]);
		*szMslTurnSpeed = *(TXT_MSL_TURNSPEED - 1);
		ADD_SLIDER (nOptions, szMslTurnSpeed + 1, extraGameInfo [0].nMslTurnSpeed, 0, 2, KEY_T, HTX_GPLAY_MSL_TURNSPEED);
		physOpts.nMslTurnSpeed = nOptions++;
		sprintf (szMslStartSpeed + 1, TXT_MSL_STARTSPEED, pszMslStartSpeeds [(int) 3 - extraGameInfo [0].nMslStartSpeed]);
		*szMslStartSpeed = *(TXT_MSL_STARTSPEED - 1);
		ADD_SLIDER (nOptions, szMslStartSpeed + 1, 3 - extraGameInfo [0].nMslStartSpeed, 0, 3, KEY_S, HTX_MSL_STARTSPEED);
		physOpts.nMslStartSpeed = nOptions++;
		sprintf (szSlowmoSpeedup + 1, TXT_SLOWMOTION_SPEEDUP, (float) gameOpts->gameplay.nSlowMotionSpeedup / 2);
		*szSlowmoSpeedup = *(TXT_SLOWMOTION_SPEEDUP - 1);
		ADD_SLIDER (nOptions, szSlowmoSpeedup + 1, gameOpts->gameplay.nSlowMotionSpeedup, 0, 4, KEY_M, HTX_SLOWMOTION_SPEEDUP);
		physOpts.nSlomoSpeedup = nOptions++;
		sprintf (szDebrisLife + 1, TXT_DEBRIS_LIFE, nDebrisLife [gameOpts->render.nDebrisLife]);
		*szDebrisLife = *(TXT_DEBRIS_LIFE - 1);
		ADD_SLIDER (nOptions, szDebrisLife + 1, gameOpts->render.nDebrisLife, 0, 8, KEY_D, HTX_DEBRIS_LIFE);
		nOptDebrisLife = nOptions++;
		sprintf (szDrag + 1, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		*szDrag = *(TXT_PLAYER_DRAG - 1);
		ADD_SLIDER (nOptions, szDrag + 1, extraGameInfo [0].nDrag, 0, 10, KEY_G, HTX_PLAYER_DRAG);
		physOpts.nDrag = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		m [optAutoLevel + gameOpts->gameplay.nAutoLeveling].value = 1;
		if (!extraGameInfo [0].nDrag)
			optWiggle = -1;
		else {
			ADD_CHECK (nOptions, TXT_WIGGLE_SHIP, extraGameInfo [0].bWiggle, KEY_W, HTX_MISC_WIGGLE);
			optWiggle = nOptions++;
			}
		ADD_CHECK (nOptions, TXT_BOTS_HIT_BOTS, extraGameInfo [0].bRobotsHitRobots, KEY_O, HTX_GPLAY_BOTSHITBOTS);
		optRobHits = nOptions++;
		ADD_CHECK (nOptions, TXT_FLUID_PHYS, extraGameInfo [0].bFluidPhysics, KEY_Y, HTX_GPLAY_FLUIDPHYS);
		optFluidPhysics = nOptions++;
		ADD_CHECK (nOptions, TXT_USE_HITANGLES, extraGameInfo [0].bUseHitAngles, KEY_H, HTX_GPLAY_HITANGLES);
		optHitAngles = nOptions++;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_IMMUNE_MISSILES, 0, KEY_I, 1, HTX_KILL_MISSILES);
		optKillMissiles = nOptions++;
		ADD_RADIO (nOptions, TXT_OMEGA_KILLS_MISSILES, 0, KEY_K, 1, HTX_KILL_MISSILES);
		nOptions++;
		ADD_RADIO (nOptions, TXT_KILL_MISSILES, 0, KEY_A, 1, HTX_KILL_MISSILES);
		nOptions++;
		m [optKillMissiles + extraGameInfo [0].bKillMissiles].value = 1;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_AUTOLEVEL_NONE, 0, KEY_N, 2, HTX_AUTO_LEVELLING);
		optAutoLevel = nOptions++;
		ADD_RADIO (nOptions, TXT_AUTOLEVEL_SIDE, 0, KEY_S, 2, HTX_AUTO_LEVELLING);
		nOptions++;
		ADD_RADIO (nOptions, TXT_AUTOLEVEL_FLOOR, 0, KEY_F, 2, HTX_AUTO_LEVELLING);
		nOptions++;
		ADD_RADIO (nOptions, TXT_AUTOLEVEL_GLOBAL, 0, KEY_M, 2, HTX_AUTO_LEVELLING);
		nOptions++;
		m [optAutoLevel + NMCLAMP (gameOpts->gameplay.nAutoLeveling, 0, 3)].value = 1;
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_RADIO (nOptions, TXT_HIT_SPHERES, 0, KEY_W, 3, HTX_GPLAY_HITBOXES);
		optHitboxes = nOptions++;
		ADD_RADIO (nOptions, TXT_SIMPLE_HITBOXES, 0, KEY_W, 3, HTX_GPLAY_HITBOXES);
		nOptions++;
		ADD_RADIO (nOptions, TXT_COMPLEX_HITBOXES, 0, KEY_W, 3, HTX_GPLAY_HITBOXES);
		nOptions++;
		m [optHitboxes + NMCLAMP (extraGameInfo [0].nHitboxes, 0, 2)].value = 1;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_PHYSICS_MENUTITLE, nOptions, m, &PhysicsOptionsCallback, &choice);
		} while (i >= 0);
	} while (i == -2);
if (gameOpts->app.bExpertMode) {
	gameOpts->gameplay.nAutoLeveling = m [optAutoLevel].value;
	extraGameInfo [0].bRobotsHitRobots = m [optRobHits].value;
	extraGameInfo [0].bKillMissiles = m [optKillMissiles].value;
	extraGameInfo [0].bFluidPhysics = m [optFluidPhysics].value;
	extraGameInfo [0].bUseHitAngles = m [optHitAngles].value;
	if (optWiggle >= 0)
		extraGameInfo [0].bWiggle = m [optWiggle].value;
	for (i = 0; i < 3; i++)
		if (m [optKillMissiles + i].value) {
			extraGameInfo [0].bKillMissiles = i;
			break;
			}
	for (i = 0; i < 3; i++)
		if (m [optHitboxes + i].value) {
			extraGameInfo [0].nHitboxes = i;
			break;
			}
	for (i = 0; i < 4; i++)
		if (m [optAutoLevel + i].value) {
			gameOpts->gameplay.nAutoLeveling = i;
			break;
			}
	for (i = 0; i < 2; i++)
		if (gameStates.gameplay.slowmo [i].fSpeed > (float) gameOpts->gameplay.nSlowMotionSpeedup / 2)
			gameStates.gameplay.slowmo [i].fSpeed = (float) gameOpts->gameplay.nSlowMotionSpeedup / 2;
		else if ((gameStates.gameplay.slowmo [i].fSpeed > 1) && 
					(gameStates.gameplay.slowmo [i].fSpeed < (float) gameOpts->gameplay.nSlowMotionSpeedup / 2))
			gameStates.gameplay.slowmo [i].fSpeed = (float) gameOpts->gameplay.nSlowMotionSpeedup / 2;
	}
else {
#if EXPMODE_DEFAULTS
	extraGameInfo [0].bRobotsHitRobots = 0;
	extraGameInfo [0].bKillMissiles = 0;
	extraGameInfo [0].bFluidPhysics = 1;
	extraGameInfo [0].nHitboxes = 0;
	extraGameInfo [0].bWiggle = 0;
#endif
	}
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

//------------------------------------------------------------------------------

void MultiThreadingOptionsMenu (void)
{
	tMenuItem	m [10];
	int			h, i, bSound = gameData.app.bUseMultiThreading [rtSound], choice = 0;

	static int	menuToTask [rtTaskCount] = {0, 1, 1, 2, 2, 3, 4, 5};	//map menu entries to tasks
	static int	taskToMenu [6] = {0, 1, 3, 5, 6, 7};	//map tasks to menu entries

memset (m, 0, sizeof (m));
h = gameStates.app.bMultiThreaded ? 6 : 1;
for (i = 0; i < h; i++)
	ADD_CHECK (i, GT (1060 + i), gameData.app.bUseMultiThreading [taskToMenu [i]], -1, HT (359 + i));
i = ExecMenu1 (NULL, TXT_MT_MENU_TITLE, h, m, NULL, &choice);
h = gameStates.app.bMultiThreaded ? rtTaskCount : rtSound + 1;
for (i = rtSound; i < h; i++)
	gameData.app.bUseMultiThreading [i] = (m [menuToTask [i]].value != 0);
if (bSound != gameData.app.bUseMultiThreading [rtSound]) {
	if (bSound)
		EndSoundThread ();
	else
		StartSoundThread ();
	}
}

//------------------------------------------------------------------------------

void ConfigMenu (void)
{
	tMenuItem	m [20];
	int			i, nOptions, choice = 0;
	int			optSound, optConfig, optJoyCal, optPerformance, optScrRes, optReorderPrim, optReorderSec, 
					optMiscellaneous, optMultiThreading = -1, optRender, optGameplay, optCockpit, optPhysics = -1;

do {
	memset (m, 0, sizeof (m));
	optRender = optGameplay = optCockpit = optPerformance = -1;
	nOptions = 0;
	ADD_MENU (nOptions, TXT_SOUND_MUSIC, KEY_M, HTX_OPTIONS_SOUND);
	optSound = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_MENU (nOptions, TXT_CONTROLS_, KEY_O, HTX_OPTIONS_CONFIG);
	strupr (m [nOptions].text);
	optConfig = nOptions++;
#if defined (_WIN32) || defined (__linux__)
	optJoyCal = -1;
#else
	ADD_MENU (nOptions, TXT_CAL_JOYSTICK, KEY_J, HTX_OPTIONS_CALSTICK);
	optJoyCal = nOptions++;
#endif
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	if (gameStates.app.bNostalgia) {
		ADD_SLIDER (nOptions, TXT_BRIGHTNESS, paletteManager.GetGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = nOptions++;
		}
	
	if (gameStates.app.bNostalgia)
		ADD_MENU (nOptions, TXT_DETAIL_LEVELS, KEY_D, HTX_OPTIONS_DETAIL);
	else if (gameStates.app.bGameRunning)
		optPerformance = -1;
	else {
		ADD_MENU (nOptions, TXT_SETPERF_OPTION, KEY_E, HTX_PERFORMANCE_SETTINGS);
		optPerformance = nOptions++;
		}
	ADD_MENU (nOptions, TXT_SCREEN_RES, KEY_S, HTX_OPTIONS_SCRRES);
	optScrRes = nOptions++;
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_MENU (nOptions, TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
	optReorderPrim = nOptions++;
	ADD_MENU (nOptions, TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
	optReorderSec = nOptions++;
	ADD_MENU (nOptions, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISCELLANEOUS, gameStates.app.bNostalgia ? KEY_T : KEY_I, HTX_OPTIONS_MISC);
	optMiscellaneous = nOptions++;
	if (!gameStates.app.bNostalgia) {
		ADD_MENU (nOptions, TXT_COCKPIT_OPTS2, KEY_C, HTX_OPTIONS_COCKPIT);
		optCockpit = nOptions++;
		ADD_MENU (nOptions, TXT_RENDER_OPTS2, KEY_R, HTX_OPTIONS_RENDER);
		optRender = nOptions++;
		if (gameStates.app.bGameRunning && IsMultiGame && !IsCoopGame) 
			optPhysics =
			optGameplay = -1;
		else {
			ADD_MENU (nOptions, TXT_GAMEPLAY_OPTS2, KEY_G, HTX_OPTIONS_GAMEPLAY);
			optGameplay = nOptions++;
			ADD_MENU (nOptions, TXT_PHYSICS_MENUCALL, KEY_Y, HTX_OPTIONS_PHYSICS);
			optPhysics = nOptions++;
			}
		ADD_MENU (nOptions, TXT_MT_MENU_OPTION, KEY_U, HTX_MULTI_THREADING);
		optMultiThreading = nOptions++;
		}

	i = ExecMenu1 (NULL, TXT_OPTIONS, nOptions, m, ConfigMenuCallback, &choice);
	if (i >= 0) {
		if (i == optSound)
			SoundMenu ();		
		else if (i == optConfig)
			InputDeviceConfig ();		
		else if (i == optJoyCal)
			JoyDefsCalibrate ();	
		else if (i == optPerformance) {
			if (gameStates.app.bNostalgia)
				DetailLevelMenu (); 
			else
				PerformanceSettingsMenu ();
			}
		else if (i == optScrRes)
			ScreenResMenu ();	
		else if (i == optReorderPrim)
			ReorderPrimary ();		
		else if (i == optReorderSec)
			ReorderSecondary ();	
		else if (i == optMiscellaneous)
			MiscellaneousMenu ();		
		else if (!gameStates.app.bNostalgia) {
			if (i == optCockpit)
				CockpitOptionsMenu ();		
			else if (i == optRender)
				RenderOptionsMenu ();		
			else if ((optGameplay >= 0) && (i == optGameplay))
				GameplayOptionsMenu ();        
			else if ((optPhysics >= 0) && (i == optPhysics))
				PhysicsOptionsMenu ();        
			else if ((optMultiThreading >= 0) && (i == optMultiThreading))
				MultiThreadingOptionsMenu ();        
			}
		}
	} while (i > -1);
WritePlayerFile ();
}

//------------------------------------------------------------------------------

void SetRedbookVolume (int volume);

//------------------------------------------------------------------------------

int SoundChannelIndex (void)
{
	int	h, i;

for (h = (int) sizeofa (detailData.nSoundChannels), i = 0; i < h; i++)
	if (gameStates.sound.digi.nMaxChannels < detailData.nSoundChannels [i])
		break;
return i - 1;
}

//------------------------------------------------------------------------------

int SoundMenuCallback (int nitems, tMenuItem * m, int *nLastKey, int nCurItem)
{
	nitems = nitems;          
	*nLastKey = *nLastKey;

if (gameOpts->sound.bGatling != m [soundOpts.nGatling].value) {
	gameOpts->sound.bGatling = m [soundOpts.nGatling].value;
	*nLastKey = -2;
	}
if (gameConfig.nDigiVolume != m [soundOpts.nDigiVol].value) {
	gameConfig.nDigiVolume = m [soundOpts.nDigiVol].value;
	DigiSetFxVolume ((gameConfig.nDigiVolume*32768)/8);
	DigiPlaySampleOnce (SOUND_DROP_BOMB, F1_0);
	}
if ((soundOpts.nChannels >= 0) && (gameStates.sound.nSoundChannels != m [soundOpts.nChannels].value)) {
	gameStates.sound.nSoundChannels = m [soundOpts.nChannels].value;
	gameStates.sound.digi.nMaxChannels = detailData.nSoundChannels [gameStates.sound.nSoundChannels];
	sprintf (m [soundOpts.nChannels].text, TXT_SOUND_CHANNEL_COUNT, gameStates.sound.digi.nMaxChannels);
	m [soundOpts.nChannels].rebuild = 1;
	}
if ((soundOpts.nVolume >= 0) && (gameOpts->sound.xCustomSoundVolume != m [soundOpts.nVolume].value)) {
	if (!gameOpts->sound.xCustomSoundVolume || !m [soundOpts.nVolume].value)
		*nLastKey = -2;
	else
		m [soundOpts.nVolume].rebuild = 1;
	gameOpts->sound.xCustomSoundVolume = m [soundOpts.nVolume].value;
	sprintf (m [soundOpts.nVolume].text, TXT_CUSTOM_SOUNDVOL, gameOpts->sound.xCustomSoundVolume * 10, '%');
	return nCurItem;
	}
if (m [soundOpts.nRedbook].value != gameStates.sound.bRedbookEnabled) {
	if (m [soundOpts.nRedbook].value && !gameOpts->sound.bUseRedbook) {
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_REDBOOK_DISABLED);
		m [soundOpts.nRedbook].value = 0;
		m [soundOpts.nRedbook].rebuild = 1;
		}
	else if ((gameStates.sound.bRedbookEnabled = m [soundOpts.nRedbook].value)) {
		if (gameStates.app.nFunctionMode == FMODE_MENU)
			SongsPlaySong (SONG_TITLE, 1);
		else if (gameStates.app.nFunctionMode == FMODE_GAME)
			PlayLevelSong (gameData.missions.nCurrentLevel, gameStates.app.bGameRunning);
		else
			Int3 ();

		if (m [soundOpts.nRedbook].value && !gameStates.sound.bRedbookPlaying) {
			gameStates.sound.bRedbookEnabled = 0;
			gameStates.menus.nInMenu = 0;
			ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_MUSIC_NOCD);
			gameStates.menus.nInMenu = 1;
			m [soundOpts.nRedbook].value = 0;
			m [soundOpts.nRedbook].rebuild = 1;
			}
		}
	m [soundOpts.nMusicVol].text = gameStates.sound.bRedbookEnabled ? reinterpret_cast<char*> (TXT_CD_VOLUME) : reinterpret_cast<char*> (TXT_MIDI_VOLUME);
	m [soundOpts.nMusicVol].rebuild = 1;
	}

if (gameStates.sound.bRedbookEnabled) {
	if (gameConfig.nRedbookVolume != m [soundOpts.nMusicVol].value)   {
		gameConfig.nRedbookVolume = m [soundOpts.nMusicVol].value;
		SetRedbookVolume (gameConfig.nRedbookVolume);
		}
	}
else {
	if (gameConfig.nMidiVolume != m [soundOpts.nMusicVol].value) {
		int bSongPlaying = (gameConfig.nMidiVolume > 0);

		if (gameConfig.nMidiVolume * m [soundOpts.nMusicVol].value == 0) //=> midi gets either turned on or off
			*nLastKey = -2;
 		gameConfig.nMidiVolume = m [soundOpts.nMusicVol].value;
		DigiSetMidiVolume (128 * gameConfig.nMidiVolume / 8);
		if (gameConfig.nMidiVolume < 1)
			DigiPlayMidiSong (NULL, NULL, NULL, 1, 0);
		else if (!bSongPlaying) {
			//DigiStopAllChannels ();
			if (gameStates.app.bGameRunning)
				PlayLevelSong (gameData.missions.nCurrentLevel ? gameData.missions.nCurrentLevel : 1, 1);
			else
				SongsPlaySong (SONG_TITLE, 1);
			}
		}
	}
// don't enable redbook for a non-apple demo version of the shareware demo
return nCurItem;		//kill warning
}

//------------------------------------------------------------------------------

void SoundMenu (void)
{
   tMenuItem	m [20];
	char			szChannels [50], szVolume [50];
	int			i, nOptions, choice = 0, 
					optReverse, optShipSound = -1, optMissileSound = -1, optSpeedUpSound = -1, optFadeMusic = -1, 
					bSongPlaying = (gameConfig.nMidiVolume > 0);

gameStates.sound.nSoundChannels = SoundChannelIndex ();
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	soundOpts.nGatling = -1;
	ADD_SLIDER (nOptions, TXT_FX_VOLUME, gameConfig.nDigiVolume, 0, 8, KEY_F, HTX_ONLINE_MANUAL);
	soundOpts.nDigiVol = nOptions++;
	ADD_SLIDER (nOptions, gameStates.sound.bRedbookEnabled ? TXT_CD_VOLUME : TXT_MIDI_VOLUME, 
					gameStates.sound.bRedbookEnabled ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume, 
					0, 8, KEY_M, HTX_ONLINE_MANUAL);
	soundOpts.nMusicVol = nOptions++;
	if (gameStates.app.bGameRunning || gameStates.app.bNostalgia)
		soundOpts.nChannels = -1;
	else {
		sprintf (szChannels + 1, TXT_SOUND_CHANNEL_COUNT, gameStates.sound.digi.nMaxChannels);
		*szChannels = *(TXT_SOUND_CHANNEL_COUNT - 1);
		ADD_SLIDER (nOptions, szChannels + 1, gameStates.sound.nSoundChannels, 0, 
						(int) sizeofa (detailData.nSoundChannels) - 1, KEY_C, HTX_SOUND_CHANNEL_COUNT);  
		soundOpts.nChannels = nOptions++;
		}
	if (!gameStates.app.bNostalgia) {
		sprintf (szVolume + 1, TXT_CUSTOM_SOUNDVOL, gameOpts->sound.xCustomSoundVolume * 10, '%');
		*szVolume = *(TXT_CUSTOM_SOUNDVOL - 1);
		ADD_SLIDER (nOptions, szVolume + 1, gameOpts->sound.xCustomSoundVolume, 0, 
						10, KEY_C, HTX_CUSTOM_SOUNDVOL);  
		soundOpts.nVolume = nOptions++;
		}
	if (gameStates.app.bNostalgia || !gameOpts->sound.xCustomSoundVolume) 
		optShipSound = 
		optMissileSound =
		optSpeedUpSound =
		soundOpts.nGatling = -1;
	else {
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		ADD_CHECK (nOptions, TXT_SHIP_SOUND, gameOpts->sound.bShip, KEY_S, HTX_SHIP_SOUND);
		optShipSound = nOptions++;
		ADD_CHECK (nOptions, TXT_MISSILE_SOUND, gameOpts->sound.bMissiles, KEY_M, HTX_MISSILE_SOUND);
		optMissileSound = nOptions++;
		ADD_CHECK (nOptions, TXT_GATLING_SOUND, gameOpts->sound.bGatling, KEY_G, HTX_GATLING_SOUND);
		soundOpts.nGatling = nOptions++;
		if (gameOpts->sound.bGatling) {
			ADD_CHECK (nOptions, TXT_SPINUP_SOUND, extraGameInfo [0].bGatlingSpeedUp, KEY_U, HTX_SPINUP_SOUND);
			optSpeedUpSound = nOptions++;
			}
		}
	ADD_TEXT (nOptions, "", 0);
	nOptions++;
	ADD_CHECK (nOptions, TXT_REDBOOK_ENABLED, gameStates.sound.bRedbookPlaying, KEY_C, HTX_ONLINE_MANUAL);
	soundOpts.nRedbook = nOptions++;
	ADD_CHECK (nOptions, TXT_REVERSE_STEREO, gameConfig.bReverseChannels, KEY_R, HTX_ONLINE_MANUAL);
	optReverse = nOptions++;
	if (gameStates.sound.bRedbookEnabled || !gameConfig.nMidiVolume)
		optFadeMusic = -1;
	else {
		ADD_CHECK (nOptions, TXT_FADE_MUSIC, gameOpts->sound.bFadeMusic, KEY_F, HTX_FADE_MUSIC);
		optFadeMusic = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	i = ExecMenu1 (NULL, TXT_SOUND_OPTS, nOptions, m, SoundMenuCallback, &choice);
	gameStates.sound.bRedbookEnabled = m [soundOpts.nRedbook].value;
	gameConfig.bReverseChannels = m [optReverse].value;
} while (i == -2);

if (gameConfig.nMidiVolume < 1)   {
		DigiPlayMidiSong (NULL, NULL, NULL, 0, 0);
	}
else if (!bSongPlaying)
	SongsPlaySong (gameStates.sound.nCurrentSong, 1);
if (!gameStates.app.bNostalgia) {
	GET_VAL (gameOpts->sound.bFadeMusic, optFadeMusic);
	GET_VAL (gameOpts->sound.bShip, optShipSound);
	GET_VAL (gameOpts->sound.bMissiles, optMissileSound);
	GET_VAL (gameOpts->sound.bGatling, soundOpts.nGatling);
	GET_VAL (extraGameInfo [0].bGatlingSpeedUp, optSpeedUpSound);
	if (!(gameOpts->sound.bShip && gameOpts->sound.bGatling))
		DigiKillSoundLinkedToObject (LOCALPLAYER.nObject);
	}
}

//------------------------------------------------------------------------------

extern int screenShotIntervals [];

int MiscellaneousCallback (int nitems, tMenuItem * menus, int * key, int nCurItem)
{
	tMenuItem * m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + performanceOpts.nUseCompSpeed;
	v = m->value;
	if (gameStates.app.bUseDefaults != v) {
		gameStates.app.bUseDefaults = v;
		*key = -2;
		return nCurItem;
		}
	m = menus + miscOpts.nScreenshots;
	v = m->value;
	if (gameOpts->app.nScreenShotInterval != v) {
		gameOpts->app.nScreenShotInterval = v;
		if (v)
			sprintf (m->text, TXT_SCREENSHOTS, screenShotIntervals [v]);
		else
			strcpy (m->text, TXT_NO_SCREENSHOTS);
		m->rebuild = 1;
		*key = -2;
		return nCurItem;
		}
	m = menus + miscOpts.nExpertMode;
	v = m->value;
	if (gameOpts->app.bExpertMode != v) {
		gameOpts->app.bExpertMode = v;
		*key = -2;
		return nCurItem;
		}
	if (gameOpts->app.bExpertMode) {
		m = menus + miscOpts.nAutoDl;
		v = m->value;
		if (extraGameInfo [0].bAutoDownload != v) {
			extraGameInfo [0].bAutoDownload = v;
			*key = -2;
			return nCurItem;
			}
		if (extraGameInfo [0].bAutoDownload) {
			m = menus + miscOpts.nDlTimeout;
			v = m->value;
			if (GetDlTimeout () != v) {
				v = SetDlTimeout (v);
				sprintf (m->text, TXT_AUTODL_TO, GetDlTimeoutSecs ());
				m->rebuild = 1;
				}
			}
		}
	else
		SetDlTimeout (15);
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void MiscellaneousMenu (void)
{
	tMenuItem m [20];
	int	i, nOptions, choice,
#if 0
			optFastResp, 
#endif
			optHeadlight, optEscort, optUseMacros,	optAutoLevel,
			optReticle, optMissileView, optGuided, optSmartSearch, optLevelVer, optDemoFmt;
#if UDP_SAFEMODE
	int	optSafeUDP;
#endif
	char  szDlTimeout [50];
	char	szScreenShots [50];

do {
	i = nOptions = 0;
	memset (m, 0, sizeof (m));
	memset (&miscOpts, 0xff, sizeof (miscOpts));
	optReticle = optMissileView = optGuided = optSmartSearch = optLevelVer = optDemoFmt = -1;
	if (gameStates.app.bNostalgia) {
		ADD_CHECK (0, TXT_AUTO_LEVEL, gameOpts->gameplay.nAutoLeveling, KEY_L, HTX_MISC_AUTOLEVEL);
		optAutoLevel = nOptions++;
		ADD_CHECK (nOptions, TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optReticle = nOptions++;
		ADD_CHECK (nOptions, TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
		optMissileView = nOptions++;
		ADD_CHECK (nOptions, TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_G, HTX_CPIT_GUIDEDVIEW);
		optGuided = nOptions++;
		ADD_CHECK (nOptions, TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_H, HTX_MISC_HEADLIGHT);
		optHeadlight = nOptions++;
		}
	else
		optHeadlight = 
		optAutoLevel = -1;
	ADD_CHECK (nOptions, TXT_ESCORT_KEYS, gameOpts->gameplay.bEscortHotKeys, KEY_K, HTX_MISC_ESCORTKEYS);
	optEscort = nOptions++;
#if 0
	ADD_CHECK (nOptions, TXT_FAST_RESPAWN, gameOpts->gameplay.bFastRespawn, KEY_F, HTX_MISC_FASTRESPAWN);
	optFastResp = nOptions++;
#endif
	ADD_CHECK (nOptions, TXT_USE_MACROS, gameOpts->multi.bUseMacros, KEY_M, HTX_MISC_USEMACROS);
	optUseMacros = nOptions++;
	if (!(gameStates.app.bNostalgia || gameStates.app.bGameRunning)) {
#if UDP_SAFEMODE
		ADD_CHECK (nOptions, TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MISC_UDPQUAL);
		optSafeUDP=nOptions++;
#endif
		}
	if (!gameStates.app.bNostalgia) {
		if (gameOpts->app.bExpertMode) {
			ADD_CHECK (nOptions, TXT_SMART_SEARCH, gameOpts->menus.bSmartFileSearch, KEY_S, HTX_MISC_SMARTSEARCH);
			optSmartSearch = nOptions++;
			ADD_CHECK (nOptions, TXT_SHOW_LVL_VERSION, gameOpts->menus.bShowLevelVersion, KEY_V, HTX_MISC_SHOWLVLVER);
			optLevelVer = nOptions++;
			}
		else
			optSmartSearch =
			optLevelVer = -1;
		ADD_CHECK (nOptions, TXT_EXPERT_MODE, gameOpts->app.bExpertMode, KEY_X, HTX_MISC_EXPMODE);
		miscOpts.nExpertMode = nOptions++;
		ADD_CHECK (nOptions, TXT_OLD_DEMO_FORMAT, gameOpts->demo.bOldFormat, KEY_C, HTX_OLD_DEMO_FORMAT);
		optDemoFmt = nOptions++;
		}
	if (gameStates.app.bNostalgia < 2) {
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			ADD_TEXT (nOptions, "", 0);
			nOptions++;
			}
		ADD_CHECK (nOptions, TXT_AUTODL_ENABLE, extraGameInfo [0].bAutoDownload, KEY_A, HTX_MISC_AUTODL);
		miscOpts.nAutoDl = nOptions++;
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			sprintf (szDlTimeout + 1, TXT_AUTODL_TO, GetDlTimeoutSecs ());
			*szDlTimeout = *(TXT_AUTODL_TO - 1);
			ADD_SLIDER (nOptions, szDlTimeout + 1, GetDlTimeout (), 0, MaxDlTimeout (), KEY_T, HTX_MISC_AUTODLTO);  
			miscOpts.nDlTimeout = nOptions++;
			}
		ADD_TEXT (nOptions, "", 0);
		nOptions++;
		if (gameOpts->app.nScreenShotInterval)
			sprintf (szScreenShots + 1, TXT_SCREENSHOTS, screenShotIntervals [gameOpts->app.nScreenShotInterval]);
		else
			strcpy (szScreenShots + 1, TXT_NO_SCREENSHOTS);
		*szScreenShots = *(TXT_SCREENSHOTS - 1);
		ADD_SLIDER (nOptions, szScreenShots + 1, gameOpts->app.nScreenShotInterval, 0, 7, KEY_S, HTX_MISC_SCREENSHOTS);  
		miscOpts.nScreenshots = nOptions++;
		}
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISC_TITLE, nOptions, m, MiscellaneousCallback, &choice);
	} while (i >= 0);
	if (gameStates.app.bNostalgia) {
		gameOpts->gameplay.nAutoLeveling = m [optAutoLevel].value;
		gameOpts->render.cockpit.bReticle = m [optReticle].value;
		gameOpts->render.cockpit.bMissileView = m [optMissileView].value;
		gameOpts->render.cockpit.bGuidedInMainView = m [optGuided].value;
		gameOpts->gameplay.bHeadlightOnWhenPickedUp = m [optHeadlight].value;
		}
	gameOpts->gameplay.bEscortHotKeys = m [optEscort].value;
	gameOpts->multi.bUseMacros = m [optUseMacros].value;
	if (!gameStates.app.bNostalgia) {
		gameOpts->app.bExpertMode = m [miscOpts.nExpertMode].value;
		gameOpts->demo.bOldFormat = m [optDemoFmt].value;
		if (gameOpts->app.bExpertMode) {
#if UDP_SAFEMODE
			if (!gameStates.app.bGameRunning)
				GET_VAL (extraGameInfo [0].bSafeUDP, optSafeUDP);
#endif
#if 0
			GET_VAL (gameOpts->gameplay.bFastRespawn, optFastResp);
#endif
			GET_VAL (gameOpts->menus.bSmartFileSearch, optSmartSearch);
			GET_VAL (gameOpts->menus.bShowLevelVersion, optLevelVer);
			}
		else {
#if EXPMODE_DEFAULTS
			extraGameInfo [0].bWiggle = 1;
#if 0
			gameOpts->gameplay.bFastRespawn = 0;
#endif
			gameOpts->menus.bSmartFileSearch = 1;
			gameOpts->menus.bShowLevelVersion = 1;
#endif
			}
		}
	if (gameStates.app.bNostalgia > 1)
		extraGameInfo [0].bAutoDownload = 0;
	else
		extraGameInfo [0].bAutoDownload = m [miscOpts.nAutoDl].value;
	} while (i == -2);
}

//------------------------------------------------------------------------------

int QuitSaveLoadMenu (void)
{
	tMenuItem m [5];
	int	i, choice = 0, nOptions, optQuit, optOptions, optLoad, optSave;

memset (m, 0, sizeof (m));
nOptions = 0;
ADD_MENU (nOptions, TXT_QUIT_GAME, KEY_Q, HTX_QUIT_GAME);
optQuit = nOptions++;
ADD_MENU (nOptions, TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
optOptions = nOptions++;
ADD_MENU (nOptions, TXT_LOAD_GAME2, KEY_L, HTX_LOAD_GAME);
optLoad = nOptions++;
ADD_MENU (nOptions, TXT_SAVE_GAME2, KEY_S, HTX_SAVE_GAME);
optSave = nOptions++;
i = ExecMenu1 (NULL, TXT_ABORT_GAME, nOptions, m, NULL, &choice);
if (!i)
	return 0;
if (i == optOptions)
	ConfigMenu ();
else if (i == optLoad)
	saveGameHandler.Load (1, 0, 0, NULL);
else if (i == optSave)
	saveGameHandler.Save (0, 0, 0, NULL);
return 1;
}

//------------------------------------------------------------------------------

static inline int MultiChoice (int nType, int bJoin)
{
return *(reinterpret_cast<int*> (&multiOpts) + 2 * nType + bJoin);
}

void MultiplayerMenu ()
{
	tMenuItem	m [15];
	int choice = 0, nOptions = 0, i, optCreate, optJoin = -1, optConn = -1, nConnections = 0;
	int old_game_mode;

if ((gameStates.app.bNostalgia < 2) && gameData.multiplayer.autoNG.bValid) {
	i = MultiChoice (gameData.multiplayer.autoNG.uConnect, !gameData.multiplayer.autoNG.bHost);
	if (i >= 0)
		ExecMultiMenuOption (i);
	}
else {
	do {
		old_game_mode = gameData.app.nGameMode;
		memset (m, 0, sizeof (m));
		nOptions = 0;
		if (gameStates.app.bNostalgia < 2) {
			ADD_MENU (nOptions, TXT_CREATE_GAME, KEY_S, HTX_NETWORK_SERVER);
			optCreate = nOptions++;
			ADD_MENU (nOptions, TXT_JOIN_GAME, KEY_J, HTX_NETWORK_CLIENT);
			optJoin = nOptions++;
			ADD_TEXT (nOptions, "", 0);
			nOptions++;
			ADD_RADIO (nOptions, TXT_NGTYPE_IPX, 0, KEY_I, 0, HTX_NETWORK_IPX);
			optConn = nOptions++;
			ADD_RADIO (nOptions, TXT_NGTYPE_UDP, 0, KEY_U, 0, HTX_NETWORK_UDP);
			nOptions++;
			ADD_RADIO (nOptions, TXT_NGTYPE_TRACKER, 0, KEY_T, 0, HTX_NETWORK_TRACKER);
			nOptions++;
			ADD_RADIO (nOptions, TXT_NGTYPE_MCAST4, 0, KEY_M, 0, HTX_NETWORK_MCAST);
			nOptions++;
#ifdef KALINIX
			ADD_RADIO (nOptions, TXT_NGTYPE_KALI, 0, KEY_K, 0, HTX_NETWORK_KALI);
			nOptions++;
#endif
			nConnections = nOptions;
			m [optConn + NMCLAMP (gameStates.multi.nConnection, 0, nConnections - optConn)].value = 1;
			}
		else {
#ifdef NATIVE_IPX
			ADD_MENU (nOptions, TXT_START_IPX_NET_GAME,  -1, HTX_NETWORK_IPX);
			multiOpts.nStartIpx = nOptions++;
			ADD_MENU (nOptions, TXT_JOIN_IPX_NET_GAME, -1, HTX_NETWORK_IPX);
			multiOpts.nJoinIpx = nOptions++;
#endif //NATIVE_IPX
			ADD_MENU (nOptions, TXT_MULTICAST_START, KEY_M, HTX_NETWORK_MCAST);
			multiOpts.nStartMCast4 = nOptions++;
			ADD_MENU (nOptions, TXT_MULTICAST_JOIN, KEY_N, HTX_NETWORK_MCAST);
			multiOpts.nJoinMCast4 = nOptions++;
#ifdef KALINIX
			ADD_MENU (nOptions, TXT_KALI_START, KEY_K, HTX_NETWORK_KALI);
			multiOpts.nStartKali = nOptions++;
			ADD_MENU (nOptions, TXT_KALI_JOIN, KEY_I, HTX_NETWORK_KALI);
			multiOpts.nJoinKali = nOptions++;
#endif // KALINIX
			if (gameStates.app.bNostalgia > 2) {
				ADD_MENU (nOptions, TXT_MODEM_GAME2, KEY_G, HTX_NETWORK_MODEM);
				multiOpts.nSerial = nOptions++;
				}
			}
		i = ExecMenu1 (NULL, TXT_MULTIPLAYER, nOptions, m, NULL, &choice);
		if (i > -1) {      
			if (gameStates.app.bNostalgia > 1)
				i = choice;
			else {
				for (gameStates.multi.nConnection = 0; 
					  gameStates.multi.nConnection < nConnections; 
					  gameStates.multi.nConnection++)
					if (m [optConn + gameStates.multi.nConnection].value)
						break;
				i = MultiChoice (gameStates.multi.nConnection, choice == optJoin);
				}
			ExecMultiMenuOption (i);
			}
		if (old_game_mode != gameData.app.nGameMode)
			break;          // leave menu
		} while (i > -1);
	}
}

//------------------------------------------------------------------------------
/*
 * IpxSetDriver was called do_network_init and located in main/inferno
 * before the change which allows the user to choose the network driver
 * from the game menu instead of having to supply command line args.
 */
void IpxSetDriver (int ipx_driver)
{
	IpxClose ();

if (!FindArg ("-nonetwork")) {
	int nIpxError;
	int socket = 0, t;

	if ((t = FindArg ("-socket")))
		socket = atoi (pszArgList [t + 1]);
	ArchIpxSetDriver (ipx_driver);
	if ((nIpxError = IpxInit (IPX_DEFAULT_SOCKET + socket)) == IPX_INIT_OK) {
		networkData.bActive = 1;
		} 
	else {
#if 1 //TRACE
	switch (nIpxError) {
		case IPX_NOT_INSTALLED: 
			con_printf (CON_VERBOSE, "%s\n", TXT_NO_NETWORK); 
			break;
		case IPX_SOCKET_TABLE_FULL: 
			con_printf (CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET + socket); 
			break;
		case IPX_NO_LOW_DOS_MEM: 
			con_printf (CON_VERBOSE, "%s\n", TXT_MEMORY_IPX); 
			break;
		default: 
			con_printf (CON_VERBOSE, "%s %d", TXT_ERROR_IPX, nIpxError);
		}
		con_printf (CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
#endif
		networkData.bActive = 0;		// Assume no network
	}
	IpxReadUserFile ("descent.usr");
	IpxReadNetworkFile ("descent.net");
	} 
else {
#if 1 //TRACE
	con_printf (CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
#endif
	networkData.bActive = 0;		// Assume no network
	}
}

//------------------------------------------------------------------------------

void DoNewIPAddress ()
 {
  tMenuItem m [2];
  char		szIP [30];
  int			choice;

memset (m, 0, sizeof (m));
ADD_TEXT (0, "Enter an address or hostname:", 0);
ADD_INPUT (1, szIP, 50, NULL);
choice = ExecMenu (NULL, TXT_JOIN_TCP, 2, m, NULL, NULL);
if ((choice == -1) || !*m [1].text)
	return;
ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INV_ADDRESS);
}

//------------------------------------------------------------------------------

 char *MENU_PCX_NAME (void)
{
if (CFile::Exist (MENU_PCX_FULL, gameFolders.szDataDir, 0))
	return reinterpret_cast<char*> (MENU_PCX_FULL);
if (CFile::Exist (MENU_PCX_OEM, gameFolders.szDataDir, 0))
	return reinterpret_cast<char*> (MENU_PCX_OEM);
if (CFile::Exist (MENU_PCX_SHAREWARE, gameFolders.szDataDir, 0))
	return reinterpret_cast<char*> (MENU_PCX_SHAREWARE);
return reinterpret_cast<char*> (MENU_PCX_MAC_SHARE);
}
//------------------------------------------------------------------------------
//eof
