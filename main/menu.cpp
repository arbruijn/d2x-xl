/* $Id: menu.c, v 1.34 2003/11/18 01:08:07 btb Exp $ */
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
#include "game.h"
#include "ipx.h"
#include "gr.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "bm.h"
#include "screens.h"
#include "mono.h"
#include "joy.h"
#include "vecmat.h"
#include "effects.h"
#include "slew.h"
#include "gamemine.h"
#include "gamesave.h"
#include "palette.h"
#include "args.h"
#include "newdemo.h"
#include "timer.h"
#include "sounds.h"
#include "loadgame.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#include "piggy.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "ipx.h"
#include "multi.h"
#include "scores.h"
#include "joydefs.h"
#include "modem.h"
#include "playsave.h"
#include "kconfig.h"
#include "briefings.h"
#include "credits.h"
#include "texmap.h"
#include "polyobj.h"
#include "state.h"
#include "mission.h"
#include "songs.h"
#include "config.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "powerup.h"
#include "strutil.h"
#include "reorder.h"
#include "digi.h"
#include "ai.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "cameras.h"
#include "texmerge.h"
#include "render.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "particles.h"
#include "laser.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#ifdef _WIN32
#	include "../network/win32/include/ipx_udp.h"
#else
#	include "../network/linux/include/ipx_udp.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

//------------------------------------------------------------------------------

//char *menu_difficulty_text [] = {"Trainee", "Rookie", "Hotshot", "Ace", "Insane"};
//char *menu_detail_text [] = {"Lowest", "Low", "Medium", "High", "Highest", "", "Custom..."};

#define MENU_NEW_GAME                   0
#define MENU_NEW_SP_GAME                1
#define MENU_GAME                       2 
#define MENU_LOAD_GAME                  3
#define MENU_SAVE_GAME                  4
#define MENU_MULTIPLAYER                5
#define MENU_CONFIG                     6
#define MENU_NEW_PLAYER                 7
#define MENU_DEMO_PLAY                  8
#define MENU_VIEW_SCORES                9
#define MENU_ORDER_INFO                 10
#define MENU_PLAY_MOVIE                 11
#define MENU_PLAY_SONG                  12
#define MENU_SHOW_CREDITS               13
#define MENU_QUIT                       14
#define MENU_EDITOR                     15
#define MENU_D2_MISSIONS					 16
#define MENU_D1_MISSIONS					 17
#define MENU_LOAD_LEVEL                 18
#define MENU_START_IPX_NETGAME          20
#define MENU_JOIN_IPX_NETGAME           21
#define MENU_REJOIN_NETGAME             22
#define MENU_DIFFICULTY                 23
#define MENU_START_SERIAL               24
#define MENU_HELP                       25
#define MENU_STOP_MODEM                 26
//#define MENU_START_TCP_NETGAME          26 // TCP/IP support was planned in Descent II, 
//#define MENU_JOIN_TCP_NETGAME           27 // but never realized.
#define MENU_START_APPLETALK_NETGAME    28
#define MENU_JOIN_APPLETALK_NETGAME     29
#define MENU_START_UDP_NETGAME          30 // UDP/IP support copied from d1x
#define MENU_JOIN_UDP_NETGAME           31
#define MENU_START_KALI_NETGAME         32 // Kali support copied from d1x
#define MENU_JOIN_KALI_NETGAME          33
#define MENU_START_MCAST4_NETGAME       34 // UDP/IP over multicast networks
#define MENU_JOIN_MCAST4_NETGAME        35
#define MENU_START_UDP_TRACKER_NETGAME  36 // UDP/IP support copied from d1x
#define MENU_JOIN_UDP_TRACKER_NETGAME   37

#define D2X_MENU_GAP 0

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
	int	nHWHeadLight;
	int	nMaxLightsPerFace;
	int	nMaxLightsPerPass;
	int	nMaxLightsPerObject;
	int	nLightmapQual;
	int	nGunColor;
	int	nObjectLight;
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
	int	nHeadLightAvailable;
} gplayOpts;

static struct {
	int	nUse;
	int	nPlayer;
	int	nRobots;
	int	nMissiles;
	int	nDebris;
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
#ifdef _DEBUG
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

static char *pszTexQual [4];
static char *pszMeshQual [5];
static char *pszLMapQual [5];
static char *pszRendQual [5];
static char *pszSmokeAmount [5];
static char *pszSmokeSize [4];
static char *pszSmokeLife [3];
static char *pszSmokeQual [3];
static char *pszSmokeAlpha [5];

#if DBG_SHADOWS
extern int	bZPass, bFrontCap, bRearCap, bFrontFaces, bBackFaces, bShadowVolume, bShadowTest, 
				bWallShadows, bSWCulling;
#endif

//------------------------------------------------------------------------------

//ADD_MENU ("Start netgame...", MENU_START_NETGAME, -1);
//ADD_MENU ("Send net message...", MENU_SEND_NET_MESSAGE, -1);

//unused - extern int last_joyTime;               //last time the joystick was used
extern void SetFunctionMode (int);
extern int UDPGetMyAddress ();
extern unsigned char ipx_ServerAddress [10];

void SoundMenu ();
void MiscellaneousMenu ();

// Function Prototypes added after LINTING
int ExecMainMenuOption (int nChoice);
int ExecMultiMenuOption (int nChoice);
void CustomDetailsMenu (void);
void NewGameMenu (void);
void MultiplayerMenu (void);
void IpxSetDriver (int ipx_driver);

//returns the number of demo files on the disk
int NDCountDemos ();

// ------------------------------------------------------------------------

void AutoDemoMenuCheck (int nitems, tMenuItem * items, int *nLastKey, int citem)
{
	int curtime;

PrintVersionInfo ();
// Don't allow them to hit ESC in the main menu.
if (*nLastKey == KEY_ESC) 
	*nLastKey = 0;
if (gameStates.app.bAutoDemos) {
	curtime = TimerGetApproxSeconds ();
	if (((gameStates.input.keys.xLastPressTime + i2f (/*2*/5)) < curtime)
#ifdef _DEBUG
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
	int opt = 0;

memset (&mainOpts, 0xff, sizeof (mainOpts));
#ifndef DEMO_ONLY
SetScreenMode (SCREEN_MENU);
#if 1
ADD_TEXT (opt, "", 0);
m [opt++].noscroll = 1;
ADD_TEXT (opt, "", 0);
m [opt++].noscroll = 1;
ADD_TEXT (opt, "", 0);
m [opt++].noscroll = 1;
#endif
ADD_MENU (opt, TXT_NEW_GAME1, KEY_N, HTX_MAIN_NEW);
mainOpts.nNew = opt++;
if (!gameStates.app.bNostalgia) {
	ADD_MENU (opt, TXT_NEW_SPGAME, KEY_I, HTX_MAIN_SINGLE);
	mainOpts.nSingle = opt++;
	}
ADD_MENU (opt, TXT_LOAD_GAME, KEY_L, HTX_MAIN_LOAD);
mainOpts.nLoad = opt++;
ADD_MENU (opt, TXT_MULTIPLAYER_, KEY_M, HTX_MAIN_MULTI);
mainOpts.nMulti = opt++;
if (gameStates.app.bNostalgia)
	ADD_MENU (opt, TXT_OPTIONS_, KEY_O, HTX_MAIN_CONF);
else
	ADD_MENU (opt, TXT_CONFIGURE, KEY_O, HTX_MAIN_CONF);
mainOpts.nConfig = opt++;
ADD_MENU (opt, TXT_CHANGE_PILOTS, KEY_P, HTX_MAIN_PILOT);
mainOpts.nPilots = opt++;
ADD_MENU (opt, TXT_VIEW_DEMO, KEY_D, HTX_MAIN_DEMO);
mainOpts.nDemo = opt++;
ADD_MENU (opt, TXT_VIEW_SCORES, KEY_H, HTX_MAIN_SCORES);
mainOpts.nScores = opt++;
if (CFExist ("orderd2.pcx", gameFolders.szDataDir, 0)) { // SHAREWARE
	ADD_MENU (opt, TXT_ORDERING_INFO, -1, NULL);
	mainOpts.nOrder = opt++;
	}
ADD_MENU (opt, TXT_PLAY_MOVIES, KEY_V, HTX_MAIN_MOVIES);
mainOpts.nMovies = opt++;
if (!gameStates.app.bNostalgia) {
	ADD_MENU (opt, TXT_PLAY_SONGS, KEY_S, HTX_MAIN_SONGS);
	mainOpts.nSongs = opt++;
	}
ADD_MENU (opt, TXT_CREDITS, KEY_C, HTX_MAIN_CREDITS);
mainOpts.nCredits = opt++;
#endif
ADD_MENU (opt, TXT_QUIT, KEY_Q, HTX_MAIN_QUIT);
mainOpts.nQuit = opt++;
#if 0
if (!IsMultiGame) {
	ADD_MENU ("  Load level...", MENU_LOAD_LEVEL , KEY_N);
#	ifdef EDITOR
	ADD_MENU (opt, "  Editor", KEY_E, NULL);
	nMenuChoice [opt++] = MENU_EDITOR;
#	endif
}
#endif
return opt;
}

//------------------------------------------------------------------------------

extern unsigned char ipx_ServerAddress [10];
extern unsigned char ipx_LocalAddress [10];

//------------------------------------------------------------------------------
//returns number of item chosen
int MainMenu () 
{
	tMenuItem m [25];
	int i, nChoice = 0, nOptions = 0;

IpxClose ();
memset (m, 0, sizeof (m));
//LoadPalette (MENU_PALETTE, NULL, 0, 1, 0);		//get correct palette

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
	Assert (nOptions <= sizeofa (m));
	i = ExecMenu2 ("", NULL, nOptions, m, AutoDemoMenuCheck, &nChoice, MENU_PCX_NAME ());
	if (gameStates.app.bNostalgia)
		gameOpts->app.nVersionFilter = 3;
	WritePlayerFile ();
	if (i > -1)
		ExecMainMenuOption (nChoice);
} while (gameStates.app.nFunctionMode == FMODE_MENU);
if (gameStates.app.nFunctionMode == FMODE_GAME)
	GrPaletteFadeOut (NULL, 32, 0);
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
if (!(m = (char **) D2_ALLOC (h * sizeof (char **))))
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
	CFILE cf;
	char	szSongTitles [2][14] = {"- Descent 2 -", "- Descent 1 -"};

m [j++] = szSongTitles [0];
for (i = 0; i < gameData.songs.nTotalSongs; i++) {
	if (CFOpen (&cf, (char *) gameData.songs.info [i].filename, gameFolders.szDataDir, "rb", i >= gameData.songs.nSongs [0])) {
		CFClose (&cf);
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
	if (nChoice == MENU_START_UDP_TRACKER_NETGAME) {
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

void ShowOrderForm (void);      // John didn't want this in inferno.h so I just externed it.

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
	if (!StateRestoreAll (0, 0, NULL))
		SetFunctionMode (FMODE_MENU);
	}
#ifdef _DEBUG
else if (nChoice == mainOpts.nLoadDirect) {
	tMenuItem	m [1];
	char			szLevel [10] = "";
	int			nLevel;

	ADD_INPUT (0, szLevel, sizeof (szLevel), NULL);
	ExecMenu (NULL, "Enter level to load", 1, m, NULL, NULL);
	nLevel = atoi (m->text);
	if (nLevel && (nLevel >= gameData.missions.nLastSecretLevel) && (nLevel <= gameData.missions.nLastLevel)) {
		GrPaletteFadeOut (NULL, 32, 0);
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
	GrPaletteFadeOut (NULL, 32, 0);
	ScoresView (-1);
	}
else if (nChoice == mainOpts.nMovies) {
	PlayMenuMovie ();
	}
else if (nChoice == mainOpts.nSongs) {
	PlayMenuSong ();
	}
else if (nChoice == mainOpts.nCredits) {
	GrPaletteFadeOut (NULL, 32, 0);
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
	GrPaletteFadeOut (NULL, 32, 0);
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

void CustomDetailsCallback (int nitems, tMenuItem * items, int *nLastKey, int citem)
{
	nitems = nitems;
	*nLastKey = *nLastKey;
	citem = citem;

gameStates.render.detail.nObjectComplexity = items [0].value;
gameStates.render.detail.nObjectDetail = items [1].value;
gameStates.render.detail.nWallDetail = items [2].value;
gameStates.render.detail.nWallRenderDepth = items [3].value;
gameStates.render.detail.nDebrisAmount = items [4].value;
if (!gameStates.app.bGameRunning)
	gameStates.sound.nSoundChannels = items [5].value;
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
	int	opt;
	int	i, choice = 0;
	tMenuItem m [DL_MAX];

do {
	opt = 0;
	memset (m, 0, sizeof (m));
	ADD_SLIDER (opt, TXT_OBJ_COMPLEXITY, gameStates.render.detail.nObjectComplexity, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	opt++;
	ADD_SLIDER (opt, TXT_OBJ_DETAIL, gameStates.render.detail.nObjectDetail, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	opt++;
	ADD_SLIDER (opt, TXT_WALL_DETAIL, gameStates.render.detail.nWallDetail, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	opt++;
	ADD_SLIDER (opt, TXT_WALL_RENDER_DEPTH, gameStates.render.detail.nWallRenderDepth, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	opt++;
	ADD_SLIDER (opt, TXT_DEBRIS_AMOUNT, gameStates.render.detail.nDebrisAmount, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
	opt++;
	if (!gameStates.app.bGameRunning) {
		ADD_SLIDER (opt, TXT_SOUND_CHANNELS, gameStates.sound.nSoundChannels, 0, NDL-1, 0, HTX_ONLINE_MANUAL);
		opt++;
		}
	ADD_TEXT (opt, TXT_LO_HI, 0);
	opt++;
	Assert (opt <= sizeof (m) / sizeof (m [0]));
	i = ExecMenu1 (NULL, TXT_DETAIL_CUSTOM, opt, m, CustomDetailsCallback, &choice);
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
gameOpts->render.color.nLightMapRange = 5;
gameOpts->render.color.bMix = 1;
gameOpts->render.color.bWalls = 1;
gameOpts->render.cameras.bFitToWall = 0;
gameOpts->render.cameras.nSpeed = 5000;
gameOpts->ogl.bSetGammaRamp = 0;
gameStates.ogl.nContrast = 8;
if (gameStates.app.nCompSpeed == 0) {
	gameOpts->render.color.bUseLightMaps = 0;
	gameOpts->render.nQuality = 1;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->ogl.bLightObjects = 0;
	gameOpts->movies.nQuality = 0;
	gameOpts->movies.bResize = 0;
	gameOpts->render.shadows.nClip = 0;
	gameOpts->render.shadows.nReach = 0;
	gameOpts->render.effects.nShrapnels = 0;
	gameOpts->render.smoke.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->ogl.nMaxLightsPerFace = 4;
	gameOpts->ogl.nMaxLightsPerPass = 4;
	gameOpts->ogl.nMaxLightsPerObject = 4;
	extraGameInfo [0].bShadows = 0;
	extraGameInfo [0].bUseSmoke = 0;
	extraGameInfo [0].bUseLightnings = 0;
	extraGameInfo [0].bUseCameras = 0;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 0;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 1) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseSmoke = 1;
	gameOpts->render.smoke.bPlayers = 0;
	gameOpts->render.smoke.bRobots = 1;
	gameOpts->render.smoke.bMissiles = 1;
	gameOpts->render.smoke.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 1;
	gameOpts->render.smoke.bStatic = 0;
	gameOpts->render.smoke.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
	gameOpts->render.smoke.nDens [0] =
	gameOpts->render.smoke.nDens [1] =
	gameOpts->render.smoke.nDens [2] =
	gameOpts->render.smoke.nDens [3] =
	gameOpts->render.smoke.nDens [4] = 0;
	gameOpts->render.smoke.nSize [0] =
	gameOpts->render.smoke.nSize [1] =
	gameOpts->render.smoke.nSize [2] =
	gameOpts->render.smoke.nSize [3] =
	gameOpts->render.smoke.nSize [4] = 1;
	gameOpts->render.smoke.nLife [0] =
	gameOpts->render.smoke.nLife [1] =
	gameOpts->render.smoke.nLife [2] =
	gameOpts->render.smoke.nLife [4] = 0;
	gameOpts->render.smoke.nLife [3] = 1;
	gameOpts->render.smoke.nAlpha [0] =
	gameOpts->render.smoke.nAlpha [1] =
	gameOpts->render.smoke.nAlpha [2] =
	gameOpts->render.smoke.nAlpha [3] =
	gameOpts->render.smoke.nAlpha [4] = 0;
	gameOpts->render.smoke.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 0;
	gameOpts->render.cockpit.bTextGauges = 1;
	gameOpts->render.nLightingMethod = 0;
	gameOpts->render.smoke.bAuxViews = 0;
	gameOpts->render.lightnings.bAuxViews = 0;
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
	extraGameInfo [0].bUseSmoke = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 0;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 0;
	}
else if (gameStates.app.nCompSpeed == 2) {
	gameOpts->render.nQuality = 2;
	extraGameInfo [0].bUseSmoke = 1;
	gameOpts->render.smoke.bPlayers = 0;
	gameOpts->render.smoke.bRobots = 1;
	gameOpts->render.smoke.bMissiles = 1;
	gameOpts->render.smoke.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 2;
	gameOpts->render.smoke.bStatic = 1;
	gameOpts->render.smoke.nDens [0] =
	gameOpts->render.smoke.nDens [1] =
	gameOpts->render.smoke.nDens [2] =
	gameOpts->render.smoke.nDens [3] =
	gameOpts->render.smoke.nDens [4] = 1;
	gameOpts->render.smoke.nSize [0] =
	gameOpts->render.smoke.nSize [1] =
	gameOpts->render.smoke.nSize [2] =
	gameOpts->render.smoke.nSize [3] =
	gameOpts->render.smoke.nSize [4] = 1;
	gameOpts->render.smoke.nLife [0] =
	gameOpts->render.smoke.nLife [1] =
	gameOpts->render.smoke.nLife [2] =
	gameOpts->render.smoke.nLife [4] = 0;
	gameOpts->render.smoke.nLife [3] = 1;
	gameOpts->render.smoke.nAlpha [0] =
	gameOpts->render.smoke.nAlpha [1] =
	gameOpts->render.smoke.nAlpha [2] =
	gameOpts->render.smoke.nAlpha [3] =
	gameOpts->render.smoke.nAlpha [4] = 0;
	gameOpts->render.smoke.bPlasmaTrails = 0;
	gameOpts->render.coronas.nStyle = 1;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.smoke.bAuxViews = 0;
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
	extraGameInfo [0].bUseSmoke = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 3) {
	gameOpts->render.nQuality = 3;
	extraGameInfo [0].bUseSmoke = 1;
	gameOpts->render.smoke.bPlayers = 1;
	gameOpts->render.smoke.bRobots = 1;
	gameOpts->render.smoke.bMissiles = 1;
	gameOpts->render.smoke.bCollisions = 0;
	gameOpts->render.effects.nShrapnels = 3;
	gameOpts->render.smoke.bStatic = 1;
	gameOpts->render.smoke.nDens [0] =
	gameOpts->render.smoke.nDens [1] =
	gameOpts->render.smoke.nDens [2] =
	gameOpts->render.smoke.nDens [3] =
	gameOpts->render.smoke.nDens [4] = 2;
	gameOpts->render.smoke.nSize [0] =
	gameOpts->render.smoke.nSize [1] =
	gameOpts->render.smoke.nSize [2] =
	gameOpts->render.smoke.nSize [3] =
	gameOpts->render.smoke.nSize [4] = 1;
	gameOpts->render.smoke.nLife [0] =
	gameOpts->render.smoke.nLife [1] =
	gameOpts->render.smoke.nLife [2] =
	gameOpts->render.smoke.nLife [4] = 0;
	gameOpts->render.smoke.nLife [3] = 1;
	gameOpts->render.smoke.nAlpha [0] =
	gameOpts->render.smoke.nAlpha [1] =
	gameOpts->render.smoke.nAlpha [2] =
	gameOpts->render.smoke.nAlpha [3] =
	gameOpts->render.smoke.nAlpha [4] = 0;
	gameOpts->render.smoke.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.cockpit.bTextGauges = 0;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.smoke.bAuxViews = 0;
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
	extraGameInfo [0].bUseSmoke = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
else if (gameStates.app.nCompSpeed == 4) {
	gameOpts->render.nQuality = 4;
	extraGameInfo [0].bUseSmoke = 1;
	gameOpts->render.smoke.bPlayers = 1;
	gameOpts->render.smoke.bRobots = 1;
	gameOpts->render.smoke.bMissiles = 1;
	gameOpts->render.smoke.bCollisions = 1;
	gameOpts->render.effects.nShrapnels = 4;
	gameOpts->render.smoke.bStatic = 1;
	gameOpts->render.smoke.nDens [0] =
	gameOpts->render.smoke.nDens [1] =
	gameOpts->render.smoke.nDens [2] =
	gameOpts->render.smoke.nDens [3] =
	gameOpts->render.smoke.nDens [4] = 3;
	gameOpts->render.smoke.nSize [0] =
	gameOpts->render.smoke.nSize [1] =
	gameOpts->render.smoke.nSize [2] =
	gameOpts->render.smoke.nSize [3] =
	gameOpts->render.smoke.nSize [4] = 1;
	gameOpts->render.smoke.nLife [0] =
	gameOpts->render.smoke.nLife [1] =
	gameOpts->render.smoke.nLife [2] =
	gameOpts->render.smoke.nLife [4] = 0;
	gameOpts->render.smoke.nLife [3] = 1;
	gameOpts->render.smoke.nAlpha [0] =
	gameOpts->render.smoke.nAlpha [1] =
	gameOpts->render.smoke.nAlpha [2] =
	gameOpts->render.smoke.nAlpha [3] =
	gameOpts->render.smoke.nAlpha [4] = 0;
	gameOpts->render.smoke.bPlasmaTrails = 1;
	gameOpts->render.coronas.nStyle = 2;
	gameOpts->render.nLightingMethod = 1;
	gameOpts->render.smoke.bAuxViews = 1;
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
	extraGameInfo [0].bUseSmoke = 1;
	extraGameInfo [0].bUseLightnings = 1;
	extraGameInfo [0].bPlayerShield = 1;
	extraGameInfo [0].bThrusterFlames = 1;
	extraGameInfo [0].bDamageExplosions = 1;
	}
}

//      -----------------------------------------------------------------------------

static char *pszCompSpeeds [5];

void PerformanceSettingsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem * m;
	int			v;

m = menus + performanceOpts.nUseCompSpeed;
v = m->value;
if (gameStates.app.bUseDefaults != v) {
	gameStates.app.bUseDefaults = v;
	*key = -2;
	return;
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
}

//      -----------------------------------------------------------------------------

void PerformanceSettingsMenu (void)
{
	int		i, opt, choice = gameStates.app.nDetailLevel;
	char		szCompSpeed [50];
	tMenuItem m [10];

pszCompSpeeds [0] = TXT_VERY_SLOW;
pszCompSpeeds [1] = TXT_SLOW;
pszCompSpeeds [2] = TXT_MEDIUM;
pszCompSpeeds [3] = TXT_FAST;
pszCompSpeeds [4] = TXT_VERY_FAST;

do {
	i = opt = 0;
	memset (m, 0, sizeof (m));
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_CHECK (opt, TXT_USE_DEFAULTS, gameStates.app.bUseDefaults, KEY_U, HTX_MISC_DEFAULTS);
	performanceOpts.nUseCompSpeed = opt++;
	if (gameStates.app.bUseDefaults) {
		sprintf (szCompSpeed + 1, TXT_COMP_SPEED, pszCompSpeeds [gameStates.app.nCompSpeed]);
		*szCompSpeed = *(TXT_COMP_SPEED - 1);
		ADD_SLIDER (opt, szCompSpeed + 1, gameStates.app.nCompSpeed, 0, 4, KEY_C, HTX_MISC_COMPSPEED);
		performanceOpts.nCompSpeed = opt++;
		}
	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, TXT_SETPERF_MENUTITLE, opt, m, PerformanceSettingsCallback, &choice);
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

static int ScreenResModeToMenuItem(int mode)
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

void ScreenResCallback (int nItems, tMenuItem *m, int *nLastKey, int citem)
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

#define ADD_RES_OPT(_t) {ADD_RADIO (opt, _t, 0, -1, 0, NULL); opt++;}
//{m [opt].nType = NM_TYPE_RADIO; m [opt].text = (_t); m [opt].key = -1; m [opt].value = 0; opt++;}

void ScreenResMenu ()
{
#	define N_SCREENRES_ITEMS (NUM_DISPLAY_MODES + 4)

	tMenuItem	m [N_SCREENRES_ITEMS];
	int				choice;
	int				i, j, key, opt = 0, nCustWOpt, nCustHOpt, nCustW, nCustH, bStdRes;
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
	opt = 0;
	memset (m, 0, sizeof (m));
	for (i = 0, j = NUM_DISPLAY_MODES; i < j; i++) {
		if (!(displayModeInfo [i].isAvailable = 
				 ((i < 2) || gameStates.menus.bHiresAvailable) && GrVideoModeOK (displayModeInfo [i].VGA_mode)))
				continue;
		if (displayModeInfo [i].isWideScreen && !displayModeInfo [i-1].isWideScreen) {
			ADD_TEXT (opt, TXT_WIDESCREEN_RES, 0);
			if (wideScreenOpt < 0)
				wideScreenOpt = opt;
			opt++;
			}
		sprintf (szMode [i], "%c. %dx%d", cShortCut, displayModeInfo [i].w, displayModeInfo [i].h);
		if (cShortCut == '9')
			cShortCut = 'A';
		else
			cShortCut++;
		ADD_RES_OPT (szMode [i]);
		}
	ADD_RADIO (opt, TXT_CUSTOM_SCRRES, 0, KEY_U, 0, HTX_CUSTOM_SCRRES);
	optCustRes = opt++;
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
		ADD_INPUT (opt, szCustX, 4, NULL);
		nCustWOpt = opt++;
		ADD_INPUT (opt, szCustY, 4, NULL);
		nCustHOpt = opt++;
		}
/*
	else
		nCustWOpt =
		nCustHOpt = -1;
*/
	choice = ScreenResModeToMenuItem(nDisplayMode);
	m [choice].value = 1;

	key = ExecMenu1 (NULL, TXT_SELECT_SCRMODE, opt, m, ScreenResCallback, &choice);
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
	int	i, nMissions, nDefaultMission, nNewMission = -1;
	char	*szMsnNames [MAX_MISSIONS];

	static char* menuTitles [4];

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
		if (!stricmp (szMsnNames [i], gameConfig.szLastMission))
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
	static char	*menuTitles [4];

menuTitles [0] = TXT_NEW_GAME;
menuTitles [1] = TXT_NEW_D1GAME;
menuTitles [2] = TXT_NEW_D2GAME;
menuTitles [3] = TXT_NEW_GAME;

gameStates.app.bD1Mission = 0;
gameStates.app.bD1Data = 0;
SetDataVersion (-1);
if ((nMission < 0) || gameOpts->app.bSinglePlayer)
	gameFolders.szMsnSubDir [0] = '\0';
CFUseAltHogFile ("");
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
		m [0].text = TXT_ENTER_TO_CONT;
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		goto try_again;
	}
}

WritePlayerFile ();
if (!DifficultyMenu ())
	return;
GrPaletteFadeOut (NULL, 32, 0);
if (!StartNewGame (nNewLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static int nOptVerFilter = -1;

void NewGameMenuCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
}

//------------------------------------------------------------------------------

void NewGameMenu ()
{
	tMenuItem		m [15];
	int				opt, optSelMsn, optMsnName, optLevelText, optLevel, optLaunch;
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
CFUseAltHogFile ("");
for (;;) {
	memset (m, 0, sizeof (m));
	opt = 0;

	ADD_MENU (opt, TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
	optSelMsn = opt++;
	ADD_TEXT (opt, (nMission < 0) ? TXT_NONE_SELECTED : gameData.missions.list [nMission].szMissionName, 0);
	optMsnName = opt++;
	if ((nMission >= 0) && (nPlayerMaxLevel > 1)) {
#if 0
		ADD_TEXT (opt, "", 0);
		opt++;
#endif
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, nPlayerMaxLevel);
		Assert (strlen (szLevelText) < 32);
		ADD_TEXT (opt, szLevelText, 0); 
		m [opt].rebuild = 1;
		optLevelText = opt++;
		sprintf (szLevel, "%d", nLevel);
		ADD_INPUT (opt, szLevel, 4, HTX_MULTI_LEVEL);
		optLevel = opt++;
		}
	else
		optLevel = -1;
	ADD_TEXT (opt, "                              ", 0);
	opt++;
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	ADD_SLIDER (opt, szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	gplayOpts.nDifficulty = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_RADIO (opt, TXT_PLAY_D1MISSIONS, 0, KEY_1, 1, HTX_LEVEL_VERSION_FILTER);
	nOptVerFilter = opt++;
	ADD_RADIO (opt, TXT_PLAY_D2MISSIONS, 0, KEY_2, 1, HTX_LEVEL_VERSION_FILTER);
	opt++;
	ADD_RADIO (opt, TXT_PLAY_ALL_MISSIONS, 0, KEY_A, 1, HTX_LEVEL_VERSION_FILTER);
	opt++;
	m [nOptVerFilter + gameOpts->app.nVersionFilter - 1].value = 1;
	if (nMission >= 0) {
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_MENU (opt, TXT_LAUNCH_GAME, KEY_L, "");
		m [opt].centered = 1;
		optLaunch = opt++;
		}
	else
		optLaunch = -1;

	Assert (opt <= sizeofa (m));
	i = ExecMenu1 (NULL, TXT_NEWGAME_MENUTITLE, opt, m, &NewGameMenuCallback, &choice);
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
GrPaletteFadeOut (NULL, 32, 0);
if (!bMsnLoaded)
	LoadMission (nMission);
if (!StartNewGame (nLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static int optBrightness = -1;
static int optContrast = -1;

//------------------------------------------------------------------------------

void options_menuset (int nitems, tMenuItem * items, int *nLastKey, int citem)
{
if (gameStates.app.bNostalgia) {
	if (citem == optBrightness)
		GrSetPaletteGamma (items [optBrightness].value);
	}
nitems++;		//kill warning
nLastKey++;		//kill warning
}

//------------------------------------------------------------------------------

static int	nCWSopt, nCWZopt, optTextGauges, optWeaponIcons, bShowWeaponIcons, 
				optIconAlpha, optTgtInd, optDmgInd, optHitInd, optMslLockInd;

static char *szCWS [4];

//------------------------------------------------------------------------------

void TgtIndOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
	return;
	}
m = menus + optDmgInd;
v = m->value;
if (v != extraGameInfo [0].bDamageIndicators) {
	extraGameInfo [0].bDamageIndicators = v;
	*key = -2;
	return;
	}
m = menus + optMslLockInd;
v = m->value;
if (v != extraGameInfo [0].bMslLockIndicators) {
	extraGameInfo [0].bMslLockIndicators = v;
	*key = -2;
	return;
	}
}

//------------------------------------------------------------------------------

void TgtIndOptionsMenu ()
{
	tMenuItem m [15];
	int	i, j, opt, choice = 0;
	int	optCloakedInd, optRotateInd;

do {
	memset (m, 0, sizeof (m));
	opt = 0;

	ADD_RADIO (opt, TXT_TGTIND_NONE, 0, KEY_A, 1, HTX_CPIT_TGTIND);
	optTgtInd = opt++;
	ADD_RADIO (opt, TXT_TGTIND_SQUARE, 0, KEY_R, 1, HTX_CPIT_TGTIND);
	opt++;
	ADD_RADIO (opt, TXT_TGTIND_TRIANGLE, 0, KEY_T, 1, HTX_CPIT_TGTIND);
	opt++;
	m [optTgtInd + extraGameInfo [0].bTargetIndicators].value = 1;
	if (extraGameInfo [0].bTargetIndicators) {
		ADD_CHECK (opt, TXT_CLOAKED_INDICATOR, extraGameInfo [0].bCloakedIndicators, KEY_C, HTX_CLOAKED_INDICATOR);
		optCloakedInd = opt++;
		}
	else
		optCloakedInd = -1;
	ADD_CHECK (opt, TXT_DMG_INDICATOR, extraGameInfo [0].bDamageIndicators, KEY_D, HTX_CPIT_DMGIND);
	optDmgInd = opt++;
	if (extraGameInfo [0].bTargetIndicators || extraGameInfo [0].bDamageIndicators) {
		ADD_CHECK (opt, TXT_HIT_INDICATOR, extraGameInfo [0].bTagOnlyHitObjs, KEY_T, HTX_HIT_INDICATOR);
		optHitInd = opt++;
		}
	else
		optHitInd = -1;
	ADD_CHECK (opt, TXT_MSLLOCK_INDICATOR, extraGameInfo [0].bMslLockIndicators, KEY_M, HTX_CPIT_MSLLOCKIND);
	optMslLockInd = opt++;
	if (extraGameInfo [0].bMslLockIndicators) {
		ADD_CHECK (opt, TXT_ROTATE_MSLLOCKIND, gameOpts->render.cockpit.bRotateMslLockInd, KEY_R, HTX_ROTATE_MSLLOCKIND);
		optRotateInd = opt++;
		}
	else
		optRotateInd = -1;
	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, TXT_TGTIND_MENUTITLE, opt, m, &TgtIndOptionsCallback, &choice);
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

void WeaponIconOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

m = menus + optWeaponIcons;
v = m->value;
if (v != bShowWeaponIcons) {
	bShowWeaponIcons = v;
	*key = -2;
	return;
	}
}

//------------------------------------------------------------------------------

void WeaponIconOptionsMenu (void)
{
	tMenuItem m [35];
	int	i, j, opt, choice = 0;
	int	optSmallIcons, optIconSort, optIconAmmo, optIconPos, optEquipIcons;

bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	memset (m, 0, sizeof (m));
	opt = 0;

	ADD_CHECK (opt, TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS);
	optWeaponIcons = opt++;
	if (bShowWeaponIcons && gameOpts->app.bExpertMode) {
		ADD_CHECK (opt, TXT_SHOW_EQUIPICONS, gameOpts->render.weaponIcons.bEquipment, KEY_Q, HTX_CPIT_EQUIPICONS);
		optEquipIcons = opt++;
		ADD_CHECK (opt, TXT_SMALL_WPNICONS, gameOpts->render.weaponIcons.bSmall, KEY_I, HTX_CPIT_SMALLICONS);
		optSmallIcons = opt++;
		ADD_CHECK (opt, TXT_SORT_WPNICONS, gameOpts->render.weaponIcons.nSort, KEY_T, HTX_CPIT_SORTICONS);
		optIconSort = opt++;
		ADD_CHECK (opt, TXT_AMMO_WPNICONS, gameOpts->render.weaponIcons.bShowAmmo, KEY_A, HTX_CPIT_ICONAMMO);
		optIconAmmo = opt++;
		optIconPos = opt;
		ADD_RADIO (opt, TXT_WPNICONS_TOP, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		opt++;
		ADD_RADIO (opt, TXT_WPNICONS_BTM, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		opt++;
		ADD_RADIO (opt, TXT_WPNICONS_LRB, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		opt++;
		ADD_RADIO (opt, TXT_WPNICONS_LRT, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		opt++;
		m [optIconPos + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].value = 1;
		ADD_SLIDER (opt, TXT_ICON_DIM, gameOpts->render.weaponIcons.alpha, 0, 8, KEY_D, HTX_CPIT_ICONDIM);
		optIconAlpha = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	else
		optEquipIcons =
		optSmallIcons =
		optIconSort =
		optIconPos =
		optIconAmmo = 
		optIconAlpha = -1;
	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, TXT_WPNICON_MENUTITLE, opt, m, &WeaponIconOptionsCallback, &choice);
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

void GaugeOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

m = menus + optTextGauges;
v = !m->value;
if (v != gameOpts->render.cockpit.bTextGauges) {
	gameOpts->render.cockpit.bTextGauges = v;
	*key = -2;
	return;
	}
}

//------------------------------------------------------------------------------

void GaugeOptionsMenu (void)
{
	tMenuItem m [10];
	int	i, opt, choice = 0;
	int	optScaleGauges, optFlashGauges, optShieldWarn, optObjectTally, optPlayerStats;

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_P, HTX_CPIT_GFXGAUGES);
	optTextGauges = opt++;
	if (!gameOpts->render.cockpit.bTextGauges && gameOpts->app.bExpertMode) {
		ADD_CHECK (opt, TXT_SCALE_GAUGES, gameOpts->render.cockpit.bScaleGauges, KEY_C, HTX_CPIT_SCALEGAUGES);
		optScaleGauges = opt++;
		ADD_CHECK (opt, TXT_FLASH_GAUGES, gameOpts->render.cockpit.bFlashGauges, KEY_F, HTX_CPIT_FLASHGAUGES);
		optFlashGauges = opt++;
		}
	else
		optScaleGauges =
		optFlashGauges = -1;
	ADD_CHECK (opt, TXT_SHIELD_WARNING, gameOpts->gameplay.bShieldWarning, KEY_W, HTX_CPIT_SHIELDWARN);
	optShieldWarn = opt++;
	ADD_CHECK (opt, TXT_OBJECT_TALLY, gameOpts->render.cockpit.bObjectTally, KEY_T, HTX_CPIT_OBJTALLY);
	optObjectTally = opt++;
	ADD_CHECK (opt, TXT_PLAYER_STATS, gameOpts->render.cockpit.bPlayerStats, KEY_S, HTX_CPIT_PLAYERSTATS);
	optPlayerStats = opt++;
	do {
		i = ExecMenu1 (NULL, TXT_GAUGES_MENUTITLE, opt, m, &GaugeOptionsCallback, &choice);
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

void CockpitOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

if (gameOpts->app.bExpertMode) {
	m = menus + nCWSopt;
	v = m->value;
	if (gameOpts->render.cockpit.nWindowSize != v) {
		gameOpts->render.cockpit.nWindowSize = v;
		m->text = szCWS [v];
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
}

//------------------------------------------------------------------------------

void CockpitOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, opt, 
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
	opt = 0;

	if (gameOpts->app.bExpertMode) {
		ADD_SLIDER (opt, szCWS [gameOpts->render.cockpit.nWindowSize], gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_S, HTX_CPIT_WINSIZE);
		nCWSopt = opt++;
		sprintf (szCockpitWindowZoom, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		ADD_SLIDER (opt, szCockpitWindowZoom, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);
		nCWZopt = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_TEXT (opt, TXT_AUXWIN_POSITION, 0);
		opt++;
		ADD_RADIO (opt, TXT_POS_BOTTOM, nPosition == 0, KEY_B, 10, HTX_AUXWIN_POSITION);
		optPosition = opt++;
		ADD_RADIO (opt, TXT_POS_TOP, nPosition == 1, KEY_T, 10, HTX_AUXWIN_POSITION);
		opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_TEXT (opt, TXT_AUXWIN_ALIGNMENT, 0);
		opt++;
		ADD_RADIO (opt, TXT_ALIGN_CORNERS, nAlignment == 0, KEY_O, 11, HTX_AUXWIN_ALIGNMENT);
		optAlignment = opt++;
		ADD_RADIO (opt, TXT_ALIGN_MIDDLE, nAlignment == 1, KEY_I, 11, HTX_AUXWIN_ALIGNMENT);
		opt++;
		ADD_RADIO (opt, TXT_ALIGN_CENTER, nAlignment == 2, KEY_E, 11, HTX_AUXWIN_ALIGNMENT);
		opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
		optHUD = opt++;
		ADD_CHECK (opt, TXT_SHOW_HUDMSGS, gameOpts->render.cockpit.bHUDMsgs, KEY_M, HTX_CPIT_SHOWHUDMSGS);
		optHUDMsgs = opt++;
		ADD_CHECK (opt, TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optReticle = opt++;
		if (gameOpts->input.mouse.bJoystick) {
			ADD_CHECK (opt, TXT_SHOW_MOUSEIND, gameOpts->render.cockpit.bMouseIndicator, KEY_O, HTX_CPIT_MOUSEIND);
			optMouseInd = opt++;
			}
		else
			optMouseInd = -1;
		}
	else
		optHUD =
		optHUDMsgs =
		optMouseInd = 
		optReticle = -1;
	ADD_CHECK (opt, TXT_EXTRA_PLRMSGS, gameOpts->render.cockpit.bSplitHUDMsgs, KEY_P, HTX_CPIT_SPLITMSGS);
	optSplitMsgs = opt++;
	ADD_CHECK (opt, TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
	optMissileView = opt++;
	ADD_CHECK (opt, TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_F, HTX_CPIT_GUIDEDVIEW);
	optGuided = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_MENU (opt, TXT_TGTIND_MENUCALL, KEY_T, "");
	optTgtInd = opt++;
	ADD_MENU (opt, TXT_WPNICON_MENUCALL, KEY_W, "");
	optWeaponIcons = opt++;
	ADD_MENU (opt, TXT_GAUGES_MENUCALL, KEY_G, "");
	optGauges = opt++;
	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, TXT_COCKPIT_OPTS, opt, m, &CockpitOptionsCallback, &choice);
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

static inline char *ContrastText (void)
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

char *pszCoronaInt [4];
char *pszCoronaQual [3];

void CoronaOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
}

//------------------------------------------------------------------------------

void CoronaOptionsMenu ()
{
	tMenuItem m [30];
	int	i, choice = 0, optTrailType;
	int	opt;
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
	opt = 0;

	ADD_CHECK (opt, TXT_RENDER_CORONAS, gameOpts->render.coronas.bUse, KEY_C, HTX_ADVRND_CORONAS);
	effectOpts.nCoronas = opt++;
	ADD_CHECK (opt, TXT_SHOT_CORONAS, gameOpts->render.coronas.bShots, KEY_S, HTX_SHOT_CORONAS);
	effectOpts.nShotCoronas = opt++;
	ADD_CHECK (opt, TXT_POWERUP_CORONAS, gameOpts->render.coronas.bPowerups, KEY_P, HTX_POWERUP_CORONAS);
	effectOpts.nPowerupCoronas = opt++;
	ADD_CHECK (opt, TXT_WEAPON_CORONAS, gameOpts->render.coronas.bWeapons, KEY_W, HTX_WEAPON_CORONAS);
	effectOpts.nWeaponCoronas = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_CHECK (opt, TXT_ADDITIVE_CORONAS, gameOpts->render.coronas.bAdditive, KEY_A, HTX_ADDITIVE_CORONAS);
	effectOpts.nAdditiveCoronas = opt++;
	ADD_CHECK (opt, TXT_ADDITIVE_OBJCORONAS, gameOpts->render.coronas.bAdditiveObjs, KEY_O, HTX_ADDITIVE_OBJCORONAS);
	effectOpts.nAdditiveObjCoronas = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;

	sprintf (szCoronaQual + 1, TXT_CORONA_QUALITY, pszCoronaQual [gameOpts->render.coronas.nStyle]);
	*szCoronaQual = *(TXT_CORONA_QUALITY - 1);
	ADD_SLIDER (opt, szCoronaQual + 1, gameOpts->render.coronas.nStyle, 0, 1 + gameStates.ogl.bDepthBlending, KEY_Q, HTX_CORONA_QUALITY);
	effectOpts.nCoronaStyle = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;

	sprintf (szCoronaInt + 1, TXT_CORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nIntensity]);
	*szCoronaInt = *(TXT_CORONA_INTENSITY - 1);
	ADD_SLIDER (opt, szCoronaInt + 1, gameOpts->render.coronas.nIntensity, 0, 3, KEY_I, HTX_CORONA_INTENSITY);
	effectOpts.nCoronaIntensity = opt++;
	sprintf (szObjCoronaInt + 1, TXT_OBJCORONA_INTENSITY, pszCoronaInt [gameOpts->render.coronas.nObjIntensity]);
	*szObjCoronaInt = *(TXT_OBJCORONA_INTENSITY - 1);
	ADD_SLIDER (opt, szObjCoronaInt + 1, gameOpts->render.coronas.nObjIntensity, 0, 3, KEY_N, HTX_CORONA_INTENSITY);
	effectOpts.nObjCoronaIntensity = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;

	ADD_CHECK (opt, TXT_RENDER_LGTTRAILS, extraGameInfo [0].bLightTrails, KEY_T, HTX_RENDER_LGTTRAILS);
	effectOpts.nLightTrails = opt++;
	if (extraGameInfo [0].bLightTrails) {
		ADD_RADIO (opt, TXT_SOLID_LIGHTTRAILS, 0, KEY_S, 2, HTX_LIGHTTRAIL_TYPE);
		optTrailType = opt++;
		ADD_RADIO (opt, TXT_PLASMA_LIGHTTRAILS, 0, KEY_P, 2, HTX_LIGHTTRAIL_TYPE);
		opt++;
		m [optTrailType + gameOpts->render.smoke.bPlasmaTrails].value = 1;
		}
	else
		optTrailType = -1;

	Assert (opt <= sizeofa (m));
	for (;;) {
		i = ExecMenu1 (NULL, TXT_CORONA_MENUTITLE, opt, m, CoronaOptionsCallback, &choice);
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
		gameOpts->render.smoke.bPlasmaTrails = (m [optTrailType].value == 0);
	} while (i == -2);
}

//------------------------------------------------------------------------------

char *pszExplShrapnels [5];

void EffectOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
}

//------------------------------------------------------------------------------

void EffectOptionsMenu ()
{
	tMenuItem m [30];
	int	i, j, choice = 0;
	int	opt;
	int	optTranspExpl, optThrusterFlame, optDmgExpl, optAutoTransp, optPlayerShields,
			optRobotShields, optShieldHits, optTracers, optExplBlast, optSparks;
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
	opt = 0;

	sprintf (szExplShrapnels + 1, TXT_EXPLOSION_SHRAPNELS, pszExplShrapnels [gameOpts->render.effects.nShrapnels]);
	*szExplShrapnels = *(TXT_EXPLOSION_SHRAPNELS - 1);
	ADD_SLIDER (opt, szExplShrapnels + 1, gameOpts->render.effects.nShrapnels, 0, 4, KEY_P, HTX_EXPLOSION_SHRAPNELS);
	effectOpts.nExplShrapnels = opt++;
	ADD_CHECK (opt, TXT_EXPLOSION_BLAST, gameOpts->render.effects.bExplBlasts, KEY_B, HTX_EXPLOSION_BLAST);
	optExplBlast = opt++;
	ADD_CHECK (opt, TXT_DMG_EXPL, extraGameInfo [0].bDamageExplosions, KEY_X, HTX_RENDER_DMGEXPL);
	optDmgExpl = opt++;
	ADD_RADIO (opt, TXT_NO_THRUSTER_FLAME, 0, KEY_F, 1, HTX_RENDER_THRUSTER);
	optThrusterFlame = opt++;
	ADD_RADIO (opt, TXT_2D_THRUSTER_FLAME, 0, KEY_2, 1, HTX_RENDER_THRUSTER);
	opt++;
	ADD_RADIO (opt, TXT_3D_THRUSTER_FLAME, 0, KEY_3, 1, HTX_RENDER_THRUSTER);
	opt++;
	m [optThrusterFlame + extraGameInfo [0].bThrusterFlames].value = 1;
	ADD_CHECK (opt, TXT_RENDER_SPARKS, gameOpts->render.effects.bEnergySparks, KEY_P, HTX_RENDER_SPARKS);
	optSparks = opt++;
	if (gameOpts->render.textures.bUseHires)
		optTranspExpl = -1;
	else {
		ADD_CHECK (opt, TXT_TRANSP_EFFECTS, gameOpts->render.effects.bTransparent, KEY_E, HTX_ADVRND_TRANSPFX);
		optTranspExpl = opt++;
		}
	ADD_CHECK (opt, TXT_AUTO_TRANSPARENCY, gameOpts->render.effects.bAutoTransparency, KEY_A, HTX_RENDER_AUTOTRANSP);
	optAutoTransp = opt++;
	ADD_CHECK (opt, TXT_RENDER_SHIELDS, extraGameInfo [0].bPlayerShield, KEY_P, HTX_RENDER_SHIELDS);
	optPlayerShields = opt++;
	ADD_CHECK (opt, TXT_ROBOT_SHIELDS, gameOpts->render.effects.bRobotShields, KEY_O, HTX_ROBOT_SHIELDS);
	optRobotShields = opt++;
	ADD_CHECK (opt, TXT_SHIELD_HITS, gameOpts->render.effects.bOnlyShieldHits, KEY_H, HTX_SHIELD_HITS);
	optShieldHits = opt++;
	ADD_CHECK (opt, TXT_RENDER_TRACERS, extraGameInfo [0].bTracers, KEY_T, HTX_RENDER_TRACERS);
	optTracers = opt++;
#if 0
	ADD_CHECK (opt, TXT_RENDER_SHKWAVES, extraGameInfo [0].bShockwaves, KEY_S, HTX_RENDER_SHKWAVES);
	optShockwaves = opt++;
#endif
	Assert (opt <= sizeofa (m));
	for (;;) {
		i = ExecMenu1 (NULL, TXT_EFFECT_MENUTITLE, opt, m, EffectOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	gameOpts->render.effects.bEnergySparks = m [optSparks].value;
	if ((gameOpts->render.effects.bEnergySparks != bEnergySparks) && gameStates.app.bGameRunning) {
		if (gameOpts->render.effects.bEnergySparks)
			AllocEnergySparks ();
		else
			FreeEnergySparks ();
		}
	GET_VAL (gameOpts->render.effects.bTransparent, optTranspExpl);
	gameOpts->render.effects.bAutoTransparency = m [optAutoTransp].value;
	gameOpts->render.effects.bExplBlasts = m [optExplBlast].value;
	extraGameInfo [0].bTracers = m [optTracers].value;
	extraGameInfo [0].bShockwaves = 0; //m [optShockwaves].value;
	extraGameInfo [0].bDamageExplosions = m [optDmgExpl].value;
	for (j = 0; j < 3; j++)
		if (m [optThrusterFlame + j].value) {
			extraGameInfo [0].bThrusterFlames = j;
			break;
			}
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

static char *pszRadarRange [3];

void AutomapOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem * m;
	int			v;

m = menus + automapOpts.nOptTextured;
v = m->value;
if (v != gameOpts->render.automap.bTextured) {
	gameOpts->render.automap.bTextured = v;
	*key = -2;
	return;
	}
if (!m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].value) {
	*key = -2;
	return;
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
}

//------------------------------------------------------------------------------

void AutomapOptionsMenu ()
{
	tMenuItem m [30];
	int	i, j, choice = 0;
	int	opt;
	int	optBright, optGrayOut, optShowRobots, optShowPowerups, optCoronas, optSmoke, optLightnings, optColor, optSkybox, optSparks;
	char	szRadarRange [50];

pszRadarRange [0] = TXT_SHORT;
pszRadarRange [1] = TXT_MEDIUM;
pszRadarRange [2] = TXT_FAR;
*szRadarRange = '\0';
do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_AUTOMAP_TEXTURED, gameOpts->render.automap.bTextured, KEY_T, HTX_AUTOMAP_TEXTURED);
	automapOpts.nOptTextured = opt++;
	if (gameOpts->render.automap.bTextured) {
		ADD_CHECK (opt, TXT_AUTOMAP_BRIGHT, gameOpts->render.automap.bBright, KEY_B, HTX_AUTOMAP_BRIGHT);
		optBright = opt++;
		ADD_CHECK (opt, TXT_AUTOMAP_GRAYOUT, gameOpts->render.automap.bGrayOut, KEY_Y, HTX_AUTOMAP_GRAYOUT);
		optGrayOut = opt++;
		ADD_CHECK (opt, TXT_AUTOMAP_CORONAS, gameOpts->render.automap.bCoronas, KEY_C, HTX_AUTOMAP_CORONAS);
		optCoronas = opt++;
		ADD_CHECK (opt, TXT_RENDER_SPARKS, gameOpts->render.automap.bSparks, KEY_P, HTX_RENDER_SPARKS);
		optSparks = opt++;
		ADD_CHECK (opt, TXT_AUTOMAP_SMOKE, gameOpts->render.automap.bSmoke, KEY_S, HTX_AUTOMAP_SMOKE);
		optSmoke = opt++;
		ADD_CHECK (opt, TXT_AUTOMAP_LIGHTNINGS, gameOpts->render.automap.bLightnings, KEY_S, HTX_AUTOMAP_LIGHTNINGS);
		optLightnings = opt++;
		ADD_CHECK (opt, TXT_AUTOMAP_SKYBOX, gameOpts->render.automap.bSkybox, KEY_B, HTX_AUTOMAP_SKYBOX);
		optSkybox = opt++;
		}
	else
		optSmoke =
		optLightnings =
		optCoronas =
		optSkybox =
		optBright =
		optSparks = -1;
	ADD_CHECK (opt, TXT_AUTOMAP_ROBOTS, extraGameInfo [0].bRobotsOnRadar, KEY_R, HTX_AUTOMAP_ROBOTS);
	optShowRobots = opt++;
	ADD_RADIO (opt, TXT_AUTOMAP_NO_POWERUPS, 0, KEY_D, 3, HTX_AUTOMAP_POWERUPS);
	optShowPowerups = opt++;
	ADD_RADIO (opt, TXT_AUTOMAP_POWERUPS, 0, KEY_P, 3, HTX_AUTOMAP_POWERUPS);
	opt++;
	if (extraGameInfo [0].nRadar) {
		ADD_RADIO (opt, TXT_RADAR_POWERUPS, 0, KEY_A, 3, HTX_AUTOMAP_POWERUPS);
		opt++;
		}
	m [optShowPowerups + extraGameInfo [0].bPowerupsOnRadar].value = 1;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_RADIO (opt, TXT_RADAR_OFF, 0, KEY_R, 1, HTX_AUTOMAP_RADAR);
	automapOpts.nOptRadar = opt++;
	ADD_RADIO (opt, TXT_RADAR_TOP, 0, KEY_T, 1, HTX_AUTOMAP_RADAR);
	opt++;
	ADD_RADIO (opt, TXT_RADAR_BOTTOM, 0, KEY_O, 1, HTX_AUTOMAP_RADAR);
	opt++;
	if (extraGameInfo [0].nRadar) {
		ADD_TEXT (opt, "", 0);
		opt++;
		sprintf (szRadarRange + 1, TXT_RADAR_RANGE, pszRadarRange [gameOpts->render.automap.nRange]);
		*szRadarRange = *(TXT_RADAR_RANGE - 1);
		ADD_SLIDER (opt, szRadarRange + 1, gameOpts->render.automap.nRange, 0, 2, KEY_A, HTX_RADAR_RANGE);
		automapOpts.nOptRadarRange = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_RADAR_WHITE, 0, KEY_W, 2, NULL);
		optColor = opt++;
		ADD_RADIO (opt, TXT_RADAR_BLACK, 0, KEY_L, 2, NULL);
		opt++;
		m [optColor + gameOpts->render.automap.nColor].value = 1;
		m [automapOpts.nOptRadar + extraGameInfo [0].nRadar].value = 1;
		}
	else
		automapOpts.nOptRadarRange =
		optColor = -1;
	Assert (opt <= sizeofa (m));
	for (;;) {
		i = ExecMenu1 (NULL, TXT_AUTOMAP_MENUTITLE, opt, m, AutomapOptionsCallback, &choice);
		if (i < 0)
			break;
		} 
	//gameOpts->render.automap.bTextured = m [automapOpts.nOptTextured].value;
	GET_VAL (gameOpts->render.automap.bBright, optBright);
	GET_VAL (gameOpts->render.automap.bGrayOut, optGrayOut);
	GET_VAL (gameOpts->render.automap.bCoronas, optCoronas);
	GET_VAL (gameOpts->render.automap.bSparks, optSparks);
	GET_VAL (gameOpts->render.automap.bSmoke, optSmoke);
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

void PowerupOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem * m;
	int			v;

m = menus + nOpt3D;
v = m->value;
if (v != gameOpts->render.powerups.b3D) {
	if ((gameOpts->render.powerups.b3D = v))
		ConvertAllPowerupsToWeapons ();
	*key = -2;
	return;
	}
}

//------------------------------------------------------------------------------

void PowerupOptionsMenu ()
{
	tMenuItem m [10];
	int	i, j, choice = 0;
	int	opt;
	int	optSpin;

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_3D_POWERUPS, gameOpts->render.powerups.b3D, KEY_D, HTX_3D_POWERUPS);
	nOpt3D = opt++;
	if (!gameOpts->render.powerups.b3D)
		optSpin = -1;
	else {
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_SPIN_OFF, 0, KEY_O, 1, NULL);
		optSpin = opt++;
		ADD_RADIO (opt, TXT_SPIN_SLOW, 0, KEY_S, 1, NULL);
		opt++;
		ADD_RADIO (opt, TXT_SPIN_MEDIUM, 0, KEY_M, 1, NULL);
		opt++;
		ADD_RADIO (opt, TXT_SPIN_FAST, 0, KEY_F, 1, NULL);
		opt++;
		m [optSpin + gameOpts->render.powerups.nSpin].value = 1;
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_POWERUP_MENUTITLE, opt, m, PowerupOptionsCallback, &choice);
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

static char *pszReach [4];
static char *pszClip [4];

void ShadowOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

m = menus + shadowOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bShadows) {
	extraGameInfo [0].bShadows = v;
	*key = -2;
	return;
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
			return;
			}
		m = menus + shadowOpts.nVolume;
		v = m->value;
		if (bShadowVolume != v) {
			bShadowVolume = v;
			m->rebuild = 1;
			*key = -2;
			return;
			}
		}
#endif
	}
}

//------------------------------------------------------------------------------

void ShadowOptionsMenu ()
{
	tMenuItem m [30];
	int	i, j, choice = 0;
	int	opt;
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
	opt = 0;
	if (extraGameInfo [0].bShadows) {
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	ADD_CHECK (opt, TXT_RENDER_SHADOWS, extraGameInfo [0].bShadows, KEY_W, HTX_ADVRND_SHADOWS);
	shadowOpts.nUse = opt++;
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
		ADD_SLIDER (opt, szMaxLightsPerFace + 1, gameOpts->render.shadows.nLights - 1, 0, MAX_SHADOW_LIGHTS, KEY_S, HTX_ADVRND_MAXLIGHTS);
		shadowOpts.nMaxLights = opt++;
		sprintf (szReach + 1, TXT_SHADOW_REACH, pszReach [gameOpts->render.shadows.nReach]);
		*szReach = *(TXT_SHADOW_REACH - 1);
		ADD_SLIDER (opt, szReach + 1, gameOpts->render.shadows.nReach, 0, 3, KEY_R, HTX_RENDER_SHADOWREACH);
		shadowOpts.nReach = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_TEXT (opt, TXT_CLIP_SHADOWS, 0);
		optClipShadows = ++opt;
		for (j = 0; j < 4; j++) {
			ADD_RADIO (opt, pszClip [j], gameOpts->render.shadows.nClip == j, 0, 1, HTX_CLIP_SHADOWS);
			opt++;
			}
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_PLAYER_SHADOWS, gameOpts->render.shadows.bPlayers, KEY_P, HTX_PLAYER_SHADOWS);
		optPlayerShadows = opt++;
		ADD_CHECK (opt, TXT_ROBOT_SHADOWS, gameOpts->render.shadows.bRobots, KEY_O, HTX_ROBOT_SHADOWS);
		optRobotShadows = opt++;
		ADD_CHECK (opt, TXT_MISSILE_SHADOWS, gameOpts->render.shadows.bMissiles, KEY_M, HTX_MISSILE_SHADOWS);
		optMissileShadows = opt++;
		ADD_CHECK (opt, TXT_POWERUP_SHADOWS, gameOpts->render.shadows.bPowerups, KEY_W, HTX_POWERUP_SHADOWS);
		optPowerupShadows = opt++;
		ADD_CHECK (opt, TXT_REACTOR_SHADOWS, gameOpts->render.shadows.bReactors, KEY_A, HTX_REACTOR_SHADOWS);
		optReactorShadows = opt++;
#if DBG_SHADOWS
		ADD_CHECK (opt, TXT_FAST_SHADOWS, gameOpts->render.shadows.bFast, KEY_F, HTX_FAST_SHADOWS);
		optFastShadows = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, "use Z-Pass algorithm", bZPass, 0, NULL);
		shadowOpts.nZPass = opt++;
		if (!bZPass) {
			ADD_CHECK (opt, "render front cap", bFrontCap, 0, NULL);
			optFrontCap = opt++;
			ADD_CHECK (opt, "render rear cap", bRearCap, 0, NULL);
			optRearCap = opt++;
			}
		ADD_CHECK (opt, "render shadow volume", bShadowVolume, 0, NULL);
		shadowOpts.nVolume = opt++;
		if (bShadowVolume) {
			ADD_CHECK (opt, "render front faces", bFrontFaces, 0, NULL);
			optFrontFaces = opt++;
			ADD_CHECK (opt, "render back faces", bBackFaces, 0, NULL);
			optBackFaces = opt++;
			}
		ADD_CHECK (opt, "render tWall shadows", bWallShadows, 0, NULL);
		optWallShadows = opt++;
		ADD_CHECK (opt, "software culling", bSWCulling, 0, NULL);
		optSWCulling = opt++;
		sprintf (szShadowTest, "test method: %d", bShadowTest);
		ADD_SLIDER (opt, szShadowTest, bShadowTest, 0, 6, KEY_S, NULL);
		shadowOpts.nTest = opt++;
#endif
		}
	for (;;) {
		i = ExecMenu1 (NULL, TXT_SHADOW_MENUTITLE, opt, m, &ShadowOptionsCallback, &choice);
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

void CameraOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

m = menus + camOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseCameras) {
	extraGameInfo [0].bUseCameras = v;
	*key = -2;
	return;
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
}

//------------------------------------------------------------------------------

void CameraOptionsMenu ()
{
	tMenuItem m [10];
	int	i, choice = 0;
	int	opt;
	int	bFSCameras = gameOpts->render.cameras.bFitToWall;
	int	optFSCameras, optTeleCams;
#if 0
	int checks;
#endif

	char szCameraFps [50];
	char szCameraSpeed [50];

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_USE_CAMS, extraGameInfo [0].bUseCameras, KEY_C, HTX_ADVRND_USECAMS);
	camOpts.nUse = opt++;
	if (extraGameInfo [0].bUseCameras && gameOpts->app.bExpertMode) {
		ADD_CHECK (opt, TXT_TELEPORTER_CAMS, extraGameInfo [0].bTeleporterCams, KEY_U, HTX_TELEPORTER_CAMS);
		optTeleCams = opt++;
		ADD_CHECK (opt, TXT_ADJUST_CAMS, gameOpts->render.cameras.bFitToWall, KEY_U, HTX_ADVRND_ADJUSTCAMS);
		optFSCameras = opt++;
		sprintf (szCameraFps + 1, TXT_CAM_REFRESH, gameOpts->render.cameras.nFPS);
		*szCameraFps = *(TXT_CAM_REFRESH - 1);
		ADD_SLIDER (opt, szCameraFps + 1, gameOpts->render.cameras.nFPS / 5, 0, 6, KEY_A, HTX_ADVRND_CAMREFRESH);
		camOpts.nFPS = opt++;
		sprintf (szCameraSpeed + 1, TXT_CAM_SPEED, gameOpts->render.cameras.nSpeed / 1000);
		*szCameraSpeed = *(TXT_CAM_SPEED - 1);
		ADD_SLIDER (opt, szCameraSpeed + 1, (gameOpts->render.cameras.nSpeed / 1000) - 1, 0, 9, KEY_D, HTX_ADVRND_CAMSPEED);
		camOpts.nSpeed = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	else {
		optTeleCams = -1;
		optFSCameras = -1;
		camOpts.nFPS = -1;
		camOpts.nSpeed = -1;
		}

	do {
		i = ExecMenu1 (NULL, TXT_CAMERA_MENUTITLE, opt, m, &CameraOptionsCallback, &choice);
	} while (i >= 0);

	if ((extraGameInfo [0].bUseCameras = m [camOpts.nUse].value)) {
		GET_VAL (extraGameInfo [0].bTeleporterCams, optTeleCams);
		GET_VAL (gameOpts->render.cameras.bFitToWall, optFSCameras);
		}
	if (bFSCameras != gameOpts->render.cameras.bFitToWall) {
		DestroyCameras ();
		CreateCameras ();
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

void SmokeOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			i, v;

m = menus + smokeOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseSmoke) {
	extraGameInfo [0].bUseSmoke = v;
	if (!v)
		FreePartList ();
	*key = -2;
	return;
	}
if (extraGameInfo [0].bUseSmoke) {
	m = menus + smokeOpts.nQuality;
	v = m->value;
	if (v != gameOpts->render.smoke.bSort) {
		gameOpts->render.smoke.bSort = v;
		sprintf (m->text, TXT_SMOKE_QUALITY, pszSmokeQual [v]);
		m->rebuild = 1;
		return;
		}
	m = menus + smokeOpts.nSyncSizes;
	v = m->value;
	if (v != gameOpts->render.smoke.bSyncSizes) {
		gameOpts->render.smoke.bSyncSizes = v;
		*key = -2;
		return;
		}
	m = menus + smokeOpts.nPlayer;
	v = m->value;
	if (gameOpts->render.smoke.bPlayers != v) {
		gameOpts->render.smoke.bPlayers = v;
		*key = -2;
		}
	m = menus + smokeOpts.nRobots;
	v = m->value;
	if (gameOpts->render.smoke.bRobots != v) {
		gameOpts->render.smoke.bRobots = v;
		*key = -2;
		}
	m = menus + smokeOpts.nMissiles;
	v = m->value;
	if (gameOpts->render.smoke.bMissiles != v) {
		gameOpts->render.smoke.bMissiles = v;
		*key = -2;
		}
	m = menus + smokeOpts.nDebris;
	v = m->value;
	if (gameOpts->render.smoke.bDebris != v) {
		gameOpts->render.smoke.bDebris = v;
		*key = -2;
		}
	if (gameOpts->render.smoke.bSyncSizes) {
		m = menus + smokeOpts.nDensity [0];
		v = m->value;
		if (gameOpts->render.smoke.nDens [0] != v) {
			gameOpts->render.smoke.nDens [0] = v;
			sprintf (m->text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
			m->rebuild = 1;
			}
		m = menus + smokeOpts.nSize [0];
		v = m->value;
		if (gameOpts->render.smoke.nSize [0] != v) {
			gameOpts->render.smoke.nSize [0] = v;
			sprintf (m->text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
			m->rebuild = 1;
			}
		m = menus + smokeOpts.nAlpha [0];
		v = m->value;
		if (v != gameOpts->render.smoke.nAlpha [0]) {
			gameOpts->render.smoke.nAlpha [0] = v;
			sprintf (m->text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
			m->rebuild = 1;
			}
		}
	else {
		for (i = 1; i < 5; i++) {
			if (smokeOpts.nDensity [i] >= 0) {
				m = menus + smokeOpts.nDensity [i];
				v = m->value;
				if (gameOpts->render.smoke.nDens [i] != v) {
					gameOpts->render.smoke.nDens [i] = v;
					sprintf (m->text, TXT_SMOKE_DENS, pszSmokeAmount [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nSize [i] >= 0) {
				m = menus + smokeOpts.nSize [i];
				v = m->value;
				if (gameOpts->render.smoke.nSize [i] != v) {
					gameOpts->render.smoke.nSize [i] = v;
					sprintf (m->text, TXT_SMOKE_SIZE, pszSmokeSize [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nLife [i] >= 0) {
				m = menus + smokeOpts.nLife [i];
				v = m->value;
				if (gameOpts->render.smoke.nLife [i] != v) {
					gameOpts->render.smoke.nLife [i] = v;
					sprintf (m->text, TXT_SMOKE_LIFE, pszSmokeLife [v]);
					m->rebuild = 1;
					}
				}
			if (smokeOpts.nAlpha [i] >= 0) {
				m = menus + smokeOpts.nAlpha [i];
				v = m->value;
				if (v != gameOpts->render.smoke.nAlpha [i]) {
					gameOpts->render.smoke.nAlpha [i] = v;
					sprintf (m->text, TXT_SMOKE_ALPHA, pszSmokeAlpha [v]);
					m->rebuild = 1;
					}
				}
			}
		}
	}
else
	DestroyAllSmoke ();
}

//------------------------------------------------------------------------------

static char szSmokeDens [5][50];
static char szSmokeSize [5][50];
static char szSmokeLife [5][50];
static char szSmokeAlpha [5][50];

int AddSmokeSliders (tMenuItem *m, int opt, int i)
{
sprintf (szSmokeDens [i] + 1, TXT_SMOKE_DENS, pszSmokeAmount [NMCLAMP (gameOpts->render.smoke.nDens [i], 0, 4)]);
*szSmokeDens [i] = *(TXT_SMOKE_DENS - 1);
ADD_SLIDER (opt, szSmokeDens [i] + 1, gameOpts->render.smoke.nDens [i], 0, 4, KEY_P, HTX_ADVRND_SMOKEDENS);
smokeOpts.nDensity [i] = opt++;
sprintf (szSmokeSize [i] + 1, TXT_SMOKE_SIZE, pszSmokeSize [NMCLAMP (gameOpts->render.smoke.nSize [i], 0, 3)]);
*szSmokeSize [i] = *(TXT_SMOKE_SIZE - 1);
ADD_SLIDER (opt, szSmokeSize [i] + 1, gameOpts->render.smoke.nSize [i], 0, 3, KEY_Z, HTX_ADVRND_PARTSIZE);
smokeOpts.nSize [i] = opt++;
if (i < 3)
	smokeOpts.nLife [i] = -1;
else {
	sprintf (szSmokeLife [i] + 1, TXT_SMOKE_LIFE, pszSmokeLife [NMCLAMP (gameOpts->render.smoke.nLife [i], 0, 3)]);
	*szSmokeLife [i] = *(TXT_SMOKE_LIFE - 1);
	ADD_SLIDER (opt, szSmokeLife [i] + 1, gameOpts->render.smoke.nLife [i], 0, 2, KEY_L, HTX_SMOKE_LIFE);
	smokeOpts.nLife [i] = opt++;
	}
sprintf (szSmokeAlpha [i] + 1, TXT_SMOKE_ALPHA, pszSmokeAlpha [NMCLAMP (gameOpts->render.smoke.nAlpha [i], 0, 4)]);
*szSmokeAlpha [i] = *(TXT_SMOKE_SIZE - 1);
ADD_SLIDER (opt, szSmokeAlpha [i] + 1, gameOpts->render.smoke.nAlpha [i], 0, 4, KEY_Z, HTX_ADVRND_SMOKEALPHA);
smokeOpts.nAlpha [i] = opt++;
return opt;
}

//------------------------------------------------------------------------------

void SmokeOptionsMenu ()
{
	tMenuItem m [40];
	int	i, j, choice = 0;
	int	opt;
	int	nOptSmokeLag, optStaticSmoke, optCollisions, optDisperse, optRotate = -1, optAuxViews = -1;
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
	opt = 0;
	nOptSmokeLag = optStaticSmoke = optCollisions = optDisperse = -1;

	ADD_CHECK (opt, TXT_USE_SMOKE, extraGameInfo [0].bUseSmoke, KEY_U, HTX_ADVRND_USESMOKE);
	smokeOpts.nUse = opt++;
	for (j = 1; j < 5; j++)
		smokeOpts.nSize [j] =
		smokeOpts.nDensity [j] = 
		smokeOpts.nAlpha [j] = -1;
	if (extraGameInfo [0].bUseSmoke) {
		if (gameOpts->app.bExpertMode) {
			sprintf (szSmokeQual + 1, TXT_SMOKE_QUALITY, pszSmokeQual [NMCLAMP (gameOpts->render.smoke.bSort, 0, 2)]);
			*szSmokeQual = *(TXT_SMOKE_QUALITY - 1);
			ADD_SLIDER (opt, szSmokeQual + 1, gameOpts->render.smoke.bSort, 0, 2, KEY_Q, HTX_ADVRND_SMOKEQUAL);
			smokeOpts.nQuality = opt++;
			ADD_CHECK (opt, TXT_SYNC_SIZES, gameOpts->render.smoke.bSyncSizes, KEY_M, HTX_ADVRND_SYNCSIZES);
			smokeOpts.nSyncSizes = opt++;
			if (gameOpts->render.smoke.bSyncSizes) {
				opt = AddSmokeSliders (m, opt, 0);
				for (j = 1; j < 5; j++) {
					gameOpts->render.smoke.nSize [j] = gameOpts->render.smoke.nSize [0];
					gameOpts->render.smoke.nDens [j] = gameOpts->render.smoke.nDens [0];
					gameOpts->render.smoke.nAlpha [j] = gameOpts->render.smoke.nAlpha [0];
					}
				}
			else {
				smokeOpts.nDensity [0] =
				smokeOpts.nSize [0] = -1;
				}
			if (!gameOpts->render.smoke.bSyncSizes && gameOpts->render.smoke.bPlayers) {
				ADD_TEXT (opt, "", 0);
				opt++;
				}
			ADD_CHECK (opt, TXT_SMOKE_PLAYERS, gameOpts->render.smoke.bPlayers, KEY_Y, HTX_ADVRND_PLRSMOKE);
			smokeOpts.nPlayer = opt++;
			if (gameOpts->render.smoke.bPlayers) {
				if (!gameOpts->render.smoke.bSyncSizes)
					opt = AddSmokeSliders (m, opt, 1);
				ADD_CHECK (opt, TXT_SMOKE_DECREASE_LAG, gameOpts->render.smoke.bDecreaseLag, KEY_R, HTX_ADVREND_DECSMOKELAG);
				nOptSmokeLag = opt++;
				ADD_TEXT (opt, "", 0);
				opt++;
				}
			else
				nOptSmokeLag = -1;
			ADD_CHECK (opt, TXT_SMOKE_ROBOTS, gameOpts->render.smoke.bRobots, KEY_O, HTX_ADVRND_BOTSMOKE);
			smokeOpts.nRobots = opt++;
			if (gameOpts->render.smoke.bRobots && !gameOpts->render.smoke.bSyncSizes) {
				opt = AddSmokeSliders (m, opt, 2);
				ADD_TEXT (opt, "", 0);
				opt++;
				}
			ADD_CHECK (opt, TXT_SMOKE_MISSILES, gameOpts->render.smoke.bMissiles, KEY_M, HTX_ADVRND_MSLSMOKE);
			smokeOpts.nMissiles = opt++;
			if (gameOpts->render.smoke.bMissiles && !gameOpts->render.smoke.bSyncSizes) {
				opt = AddSmokeSliders (m, opt, 3);
				ADD_TEXT (opt, "", 0);
				opt++;
				}
			ADD_CHECK (opt, TXT_SMOKE_DEBRIS, gameOpts->render.smoke.bDebris, KEY_D, HTX_ADVRND_DEBRISSMOKE);
			smokeOpts.nDebris = opt++;
			if (gameOpts->render.smoke.bDebris && !gameOpts->render.smoke.bSyncSizes) {
				opt = AddSmokeSliders (m, opt, 4);
				ADD_TEXT (opt, "", 0);
				opt++;
				}
			ADD_CHECK (opt, TXT_SMOKE_STATIC, gameOpts->render.smoke.bStatic, KEY_T, HTX_ADVRND_STATICSMOKE);
			optStaticSmoke = opt++;
#if 0
			ADD_CHECK (opt, TXT_SMOKE_COLLISION, gameOpts->render.smoke.bCollisions, KEY_I, HTX_ADVRND_SMOKECOLL);
			optCollisions = opt++;
#endif
			ADD_CHECK (opt, TXT_SMOKE_DISPERSE, gameOpts->render.smoke.bDisperse, KEY_D, HTX_ADVRND_SMOKEDISP);
			optDisperse = opt++;
			ADD_CHECK (opt, TXT_ROTATE_SMOKE, gameOpts->render.smoke.bRotate, KEY_R, HTX_ROTATE_SMOKE);
			optRotate = opt++;
			ADD_CHECK (opt, TXT_SMOKE_AUXVIEWS, gameOpts->render.smoke.bAuxViews, KEY_W, HTX_SMOKE_AUXVIEWS);
			optAuxViews = opt++;
			}
		}
	else
		nOptSmokeLag =
		smokeOpts.nPlayer =
		smokeOpts.nRobots =
		smokeOpts.nMissiles =
		smokeOpts.nDebris =
		optStaticSmoke =
		optCollisions =
		optDisperse = 
		optRotate = 
		optAuxViews = -1;

	Assert (opt <= sizeof (m) / sizeof (m [0]));
	do {
		i = ExecMenu1 (NULL, TXT_SMOKE_MENUTITLE, opt, m, &SmokeOptionsCallback, &choice);
		} while (i >= 0);
	if ((extraGameInfo [0].bUseSmoke = m [smokeOpts.nUse].value)) {
		GET_VAL (gameOpts->render.smoke.bPlayers, smokeOpts.nPlayer);
		GET_VAL (gameOpts->render.smoke.bRobots, smokeOpts.nRobots);
		GET_VAL (gameOpts->render.smoke.bMissiles, smokeOpts.nMissiles);
		GET_VAL (gameOpts->render.smoke.bDebris, smokeOpts.nDebris);
		GET_VAL (gameOpts->render.smoke.bStatic, optStaticSmoke);
#if 0
		GET_VAL (gameOpts->render.smoke.bCollisions, optCollisions);
#else
		gameOpts->render.smoke.bCollisions = 0;
#endif
		GET_VAL (gameOpts->render.smoke.bDisperse, optDisperse);
		GET_VAL (gameOpts->render.smoke.bRotate, optRotate);
		GET_VAL (gameOpts->render.smoke.bDecreaseLag, nOptSmokeLag);
		GET_VAL (gameOpts->render.smoke.bAuxViews, optAuxViews);
		//GET_VAL (gameOpts->render.smoke.bSyncSizes, smokeOpts.nSyncSizes);
		if (gameOpts->render.smoke.bSyncSizes) {
			for (j = 1; j < 4; j++) {
				gameOpts->render.smoke.nSize [j] = gameOpts->render.smoke.nSize [0];
				gameOpts->render.smoke.nDens [j] = gameOpts->render.smoke.nDens [0];
				}
			}
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

void AdvancedRenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
	v = (GR_ACTUAL_FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
}

//------------------------------------------------------------------------------

void LightOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
		return;
		}
	}
if (lightOpts.nLightmapQual > 0) {
	m = menus + lightOpts.nLightmapQual;
	v = m->value;
	if (gameOpts->render.nLightmapQuality != v) {
		gameOpts->render.nLightmapQuality = v;
		sprintf (m->text, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
		m->rebuild = 1;
		}
	}
if (lightOpts.nHWObjLighting >= 0) {
	m = menus + lightOpts.nHWObjLighting;
	v = m->value;
	if (v != gameOpts->ogl.bObjLighting) {
		gameOpts->ogl.bObjLighting = v;
		*key = -2;
		return;
		}
	}
if (lightOpts.nGunColor >= 0) {
	m = menus + lightOpts.nGunColor;
	v = m->value;
	if (v != gameOpts->render.color.bGunLight) {
		gameOpts->render.color.bGunLight = v;
		*key = -2;
		return;
		}
	}
if (lightOpts.nObjectLight >= 0) {
	m = menus + lightOpts.nObjectLight;
	v = m->value;
	if (v != gameOpts->ogl.bLightObjects) {
		gameOpts->ogl.bLightObjects = v;
		*key = -2;
		return;
		}
	}
if (lightOpts.nMaxLightsPerFace >= 0) {
	m = menus + lightOpts.nMaxLightsPerFace;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerFace) {
		gameOpts->ogl.nMaxLightsPerFace = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return;
		}
	}
if (lightOpts.nMaxLightsPerObject >= 0) {
	m = menus + lightOpts.nMaxLightsPerObject;
	v = m->value;
	if (v != gameOpts->ogl.nMaxLightsPerObject) {
		gameOpts->ogl.nMaxLightsPerObject = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [v]);
		m->rebuild = 1;
		return;
		}
	}
if (lightOpts.nMaxLightsPerPass >= 0) {
	m = menus + lightOpts.nMaxLightsPerPass;
	v = m->value + 1;
	if (v != gameOpts->ogl.nMaxLightsPerPass) {
		gameOpts->ogl.nMaxLightsPerPass = v;
		sprintf (m->text, TXT_MAX_LIGHTS_PER_PASS, v);
		m->rebuild = 1;
		return;
		}
	}
}

//------------------------------------------------------------------------------

void LightOptionsMenu (void)
{
	tMenuItem m [30];
	int	h, i, choice = 0, nLightRange = extraGameInfo [0].nLightRange;
	int	opt;
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
	opt = 0;
	optColorSat = 
	lightOpts.nMethod =
	lightOpts.nLightmapQual = 
	lightOpts.nMaxLightsPerFace = 
	lightOpts.nMaxLightsPerPass = 
	lightOpts.nMaxLightsPerObject = 
	lightOpts.nHWObjLighting =
	lightOpts.nHWHeadLight =
	lightOpts.nObjectLight = -1;
	if (!gameStates.app.bGameRunning) {
		if ((gameOpts->render.nLightingMethod == 2) && !(gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk))
			gameOpts->render.nLightingMethod = 1;
		ADD_RADIO (opt, TXT_STD_LIGHTING, gameOpts->render.nLightingMethod == 0, KEY_S, 1, NULL);
		lightOpts.nMethod = opt++;
		ADD_RADIO (opt, TXT_VERTEX_LIGHTING, gameOpts->render.nLightingMethod == 1, KEY_V, 1, HTX_VERTEX_LIGHTING);
		opt++;
		if (gameStates.render.bUsePerPixelLighting && gameStates.ogl.bShadersOk && gameStates.ogl.bPerPixelLightingOk) {
			ADD_RADIO (opt, TXT_PER_PIXEL_LIGHTING, gameOpts->render.nLightingMethod == 2, KEY_P, 1, HTX_PER_PIXEL_LIGHTING);
			opt++;
			}	
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	if (gameOpts->render.nLightingMethod) {
#if 0
		ADD_TEXT (opt, "", 0);
		opt++;
#endif
		if (gameOpts->render.nLightingMethod == 1) {
			if (gameStates.ogl.bHeadLight) {
				ADD_CHECK (opt, TXT_HW_HEADLIGHT, gameOpts->ogl.bHeadLight, KEY_H, HTX_HW_HEADLIGHT);
				lightOpts.nHWHeadLight = opt++;
				}
			ADD_CHECK (opt, TXT_OBJECT_HWLIGHTING, gameOpts->ogl.bObjLighting, KEY_A, HTX_OBJECT_HWLIGHTING);
			lightOpts.nHWObjLighting = opt++;
			if (!gameOpts->ogl.bObjLighting) {
				ADD_CHECK (opt, TXT_OBJECT_LIGHTING, gameOpts->ogl.bLightObjects, KEY_O, HTX_OBJECT_LIGHTING);
				lightOpts.nObjectLight = opt++;
				if (gameOpts->ogl.bLightObjects) {
					ADD_CHECK (opt, TXT_POWERUP_LIGHTING, gameOpts->ogl.bLightPowerups, KEY_P, HTX_POWERUP_LIGHTING);
					nPowerupLight = opt++;
					}
				else
					nPowerupLight = -1;
				}
			}
		h = sizeofa (nMaxLightsPerFaceTable);
		for (i = 0; i < h; i++)
			if (gameOpts->ogl.nMaxLightsPerObject < nMaxLightsPerFaceTable [i])
				break;
		gameOpts->ogl.nMaxLightsPerObject = i ? i - 1 : 0;
		sprintf (szMaxLightsPerObject + 1, TXT_MAX_LIGHTS_PER_OBJECT, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerObject]);
		*szMaxLightsPerObject = *(TXT_MAX_LIGHTS_PER_OBJECT - 1);
		ADD_SLIDER (opt, szMaxLightsPerObject + 1, gameOpts->ogl.nMaxLightsPerObject, 0, h - 1, KEY_I, HTX_MAX_LIGHTS_PER_OBJECT);
		lightOpts.nMaxLightsPerObject = opt++;

		if (gameOpts->render.nLightingMethod == 2) {
			h = sizeofa (nMaxLightsPerFaceTable);
			for (i = 0; i < h; i++)
				if (gameOpts->ogl.nMaxLightsPerFace < nMaxLightsPerFaceTable [i])
					break;
			gameOpts->ogl.nMaxLightsPerFace = i ? i - 1 : 0;
			sprintf (szMaxLightsPerFace + 1, TXT_MAX_LIGHTS_PER_FACE, nMaxLightsPerFaceTable [gameOpts->ogl.nMaxLightsPerFace]);
			*szMaxLightsPerFace = *(TXT_MAX_LIGHTS_PER_FACE - 1);
			ADD_SLIDER (opt, szMaxLightsPerFace + 1, gameOpts->ogl.nMaxLightsPerFace, 0, h - 1, KEY_I, HTX_MAX_LIGHTS_PER_FACE);
			lightOpts.nMaxLightsPerFace = opt++;
			sprintf (szMaxLightsPerPass + 1, TXT_MAX_LIGHTS_PER_PASS, gameOpts->ogl.nMaxLightsPerPass);
			*szMaxLightsPerPass = *(TXT_MAX_LIGHTS_PER_PASS - 1);
			ADD_SLIDER (opt, szMaxLightsPerPass + 1, gameOpts->ogl.nMaxLightsPerPass - 1, 0, 7, KEY_P, HTX_MAX_LIGHTS_PER_PASS);
			lightOpts.nMaxLightsPerPass = opt++;
			sprintf (szLightmapQual + 1, TXT_LMAP_QUALITY, pszLMapQual [gameOpts->render.nLightmapQuality]);
			*szLightmapQual = *(TXT_LMAP_QUALITY + 1);
			ADD_SLIDER (opt, szLightmapQual + 1, gameOpts->render.nLightmapQuality, 0, 4, KEY_G, HTX_LMAP_QUALITY);
			lightOpts.nLightmapQual = opt++;
			}

		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_FULL_COLORSAT, 0, KEY_F, 2, HTX_COLOR_SATURATION);
		optColorSat = opt++;
		ADD_RADIO (opt, TXT_LIMIT_COLORSAT, 0, KEY_L, 2, HTX_COLOR_SATURATION);
		opt++;
		ADD_RADIO (opt, TXT_NO_COLORSAT, 0, KEY_N, 2, HTX_COLOR_SATURATION);
		opt++;
		m [optColorSat + NMCLAMP (gameOpts->render.color.nSaturation, 0, 2)].value = 1;
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	if (gameOpts->render.nLightingMethod < 2) {
		ADD_CHECK (opt, TXT_USE_COLOR, gameOpts->render.color.bAmbientLight, KEY_C, HTX_RENDER_AMBICOLOR);
		optColoredLight = opt++;
		}
	if (gameOpts->render.nLightingMethod)
		lightOpts.nGunColor = -1;
	else {
		ADD_CHECK (opt, TXT_USE_WPNCOLOR, gameOpts->render.color.bGunLight, KEY_W, HTX_RENDER_WPNCOLOR);
		lightOpts.nGunColor = opt++;
		}
	optMixColors = 
	optPowerupLights = -1;
	if (gameOpts->app.bExpertMode) {
		if (!gameOpts->render.nLightingMethod && gameOpts->render.color.bGunLight) {
			ADD_CHECK (opt, TXT_MIX_COLOR, gameOpts->render.color.bMix, KEY_X, HTX_ADVRND_MIXCOLOR);
			optMixColors = opt++;
			}
		ADD_CHECK (opt, TXT_POWERUPLIGHTS, !extraGameInfo [0].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
		optPowerupLights = opt++;
		}
	ADD_CHECK (opt, TXT_FLICKERLIGHTS, extraGameInfo [0].bFlickerLights, KEY_F, HTX_FLICKERLIGHTS);
	optFlickerLights = opt++;
	if (gameOpts->render.bHiresModels) {
		ADD_CHECK (opt, TXT_BRIGHT_OBJECTS, extraGameInfo [0].bBrightObjects, KEY_B, HTX_BRIGHT_OBJECTS);
		optBrightObjects = opt++;
		}
	else
		optBrightObjects = -1;
	Assert (opt <= sizeofa (m));
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTING_MENUTITLE, opt, m, &LightOptionsCallback, &choice);
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
	if (optColoredLight >= 0)
		gameOpts->render.color.bAmbientLight = m [optColoredLight].value;
	if (lightOpts.nGunColor >= 0)
		gameOpts->render.color.bGunLight = m [lightOpts.nGunColor].value;
	if (gameOpts->app.bExpertMode) {
		if (gameStates.render.color.bLightMapsOk && gameOpts->render.color.bUseLightMaps)
			gameStates.ogl.nContrast = 8;
		if (gameOpts->render.color.bGunLight)
			GET_VAL (gameOpts->render.color.bMix, optMixColors);
#if EXPMODE_DEFAULTS
			else
				gameOpts->render.color.bMix = 1;
#endif
		if (optPowerupLights >= 0)
			extraGameInfo [0].bPowerupLights = !m [optPowerupLights].value;
		}
	extraGameInfo [0].bFlickerLights = m [optFlickerLights].value;
	GET_VAL (gameOpts->ogl.bHeadLight, lightOpts.nHWHeadLight);
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
if (gameStates.render.bPerPixelLighting = (gameStates.render.nLightingMethod == 2)) {
	gameStates.render.nMaxLightsPerPass = gameOpts->ogl.nMaxLightsPerPass;
	gameStates.render.nMaxLightsPerFace = gameOpts->ogl.nMaxLightsPerFace;
	}
gameStates.render.nMaxLightsPerObject = gameOpts->ogl.nMaxLightsPerObject;
gameStates.render.bAmbientColor = gameStates.render.bPerPixelLighting || gameOpts->render.color.bAmbientLight;
}

//------------------------------------------------------------------------------

static	char *pszLightningQuality [2];
static	char *pszLightningStyle [3];

void LightningOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

m = menus + lightningOpts.nUse;
v = m->value;
if (v != extraGameInfo [0].bUseLightnings) {
	if (!(extraGameInfo [0].bUseLightnings = v))
		DestroyAllLightnings (0);
	*key = -2;
	return;
	}
if (extraGameInfo [0].bUseLightnings) {
	m = menus + lightningOpts.nQuality;
	v = m->value;
	if (gameOpts->render.lightnings.nQuality != v) {
		gameOpts->render.lightnings.nQuality = v;
		sprintf (m->text, TXT_LIGHTNING_QUALITY, pszLightningQuality [v]);
		m->rebuild = 1;
		DestroyAllLightnings (0);
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
}

//------------------------------------------------------------------------------

void LightningOptionsMenu ()
{
	tMenuItem m [15];
	int	i, choice = 0;
	int	opt;
	int	optDamage, optExplosions, optPlayers, optRobots, optStatic, optRobotOmega, optPlasma, optAuxViews;
	char	szQuality [50], szStyle [100];

	pszLightningQuality [0] = TXT_LOW;
	pszLightningQuality [1] = TXT_HIGH;
	pszLightningStyle [0] = TXT_LIGHTNING_ERRATIC;
	pszLightningStyle [1] = TXT_LIGHTNING_JAGGY;
	pszLightningStyle [2] = TXT_LIGHTNING_SMOOTH;

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	lightningOpts.nQuality = 
	optDamage = 
	optExplosions = 
	optPlayers = 
	optRobots = 
	optStatic = 
	optRobotOmega = 
	optPlasma = 
	optAuxViews = -1;

	ADD_CHECK (opt, TXT_LIGHTNING_ENABLE, extraGameInfo [0].bUseLightnings, KEY_U, HTX_LIGHTNING_ENABLE);
	lightningOpts.nUse = opt++;
	if (extraGameInfo [0].bUseLightnings) {
		sprintf (szQuality + 1, TXT_LIGHTNING_QUALITY, pszLightningQuality [gameOpts->render.lightnings.nQuality]);
		*szQuality = *(TXT_LIGHTNING_QUALITY - 1);
		ADD_SLIDER (opt, szQuality + 1, gameOpts->render.lightnings.nQuality, 0, 1, KEY_R, HTX_LIGHTNING_QUALITY);
		lightningOpts.nQuality = opt++;
		sprintf (szStyle + 1, TXT_LIGHTNING_STYLE, pszLightningStyle [gameOpts->render.lightnings.nStyle]);
		*szStyle = *(TXT_LIGHTNING_STYLE - 1);
		ADD_SLIDER (opt, szStyle + 1, gameOpts->render.lightnings.nStyle, 0, 2, KEY_S, HTX_LIGHTNING_STYLE);
		lightningOpts.nStyle = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_PLASMA, gameOpts->render.lightnings.bPlasma, KEY_L, HTX_LIGHTNING_PLASMA);
		optPlasma = opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_DAMAGE, gameOpts->render.lightnings.bDamage, KEY_D, HTX_LIGHTNING_DAMAGE);
		optDamage = opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_EXPLOSIONS, gameOpts->render.lightnings.bExplosions, KEY_E, HTX_LIGHTNING_EXPLOSIONS);
		optExplosions = opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_PLAYERS, gameOpts->render.lightnings.bPlayers, KEY_P, HTX_LIGHTNING_PLAYERS);
		optPlayers = opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_ROBOTS, gameOpts->render.lightnings.bRobots, KEY_R, HTX_LIGHTNING_ROBOTS);
		optRobots = opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_STATIC, gameOpts->render.lightnings.bStatic, KEY_T, HTX_LIGHTNING_STATIC);
		optStatic = opt++;
		ADD_CHECK (opt, TXT_LIGHTNING_OMEGA, gameOpts->render.lightnings.bOmega, KEY_O, HTX_LIGHTNING_OMEGA);
		lightningOpts.nOmega = opt++;
		if (gameOpts->render.lightnings.bOmega) {
			ADD_CHECK (opt, TXT_LIGHTNING_ROBOT_OMEGA, gameOpts->render.lightnings.bRobotOmega, KEY_B, HTX_LIGHTNING_ROBOT_OMEGA);
			optRobotOmega = opt++;
			}
		ADD_CHECK (opt, TXT_LIGHTNING_AUXVIEWS, gameOpts->render.lightnings.bAuxViews, KEY_D, HTX_LIGHTNING_AUXVIEWS);
		optAuxViews = opt++;
		}
	Assert (opt <= sizeofa (m));
	for (;;) {
		i = ExecMenu1 (NULL, TXT_LIGHTNING_MENUTITLE, opt, m, &LightningOptionsCallback, &choice);
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
		}
	} while (i == -2);
if (!gameOpts->render.lightnings.bPlayers)
	DestroyPlayerLightnings ();
if (!gameOpts->render.lightnings.bRobots)
	DestroyRobotLightnings ();
if (!gameOpts->render.lightnings.bStatic)
	DestroyStaticLightnings ();
}

//------------------------------------------------------------------------------

void MovieOptionsMenu ()
{
	tMenuItem m [5];
	int	i, choice = 0;
	int	opt;
	int	optMovieQual, optMovieSize, optSubTitles;

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_MOVIE_SUBTTL, gameOpts->movies.bSubTitles, KEY_O, HTX_RENDER_SUBTTL);
	optSubTitles = opt++;
	if (gameOpts->app.bExpertMode) {
		ADD_CHECK (opt, TXT_MOVIE_QUAL, gameOpts->movies.nQuality, KEY_Q, HTX_RENDER_MOVIEQUAL);
		optMovieQual = opt++;
		ADD_CHECK (opt, TXT_MOVIE_FULLSCR, gameOpts->movies.bResize, KEY_U, HTX_RENDER_MOVIEFULL);
		optMovieSize = opt++;
		}
	else
		optMovieQual = 
		optMovieSize = -1;

	for (;;) {
		i = ExecMenu1 (NULL, TXT_MOVIE_OPTIONS, opt, m, NULL, &choice);
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

static char *pszShipColors [8];

void ShipRenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
}

//------------------------------------------------------------------------------

void ShipRenderOptionsMenu ()
{
	tMenuItem m [10];
	int	i, j, choice = 0;
	int	opt;
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
	opt = 0;
	ADD_CHECK (opt, TXT_SHIP_WEAPONS, extraGameInfo [0].bShowWeapons, KEY_W, HTX_SHIP_WEAPONS);
	shipRenderOpts.nWeapons = opt++;
	if (extraGameInfo [0].bShowWeapons) {
		ADD_CHECK (opt, TXT_SHIP_BULLETS, gameOpts->render.ship.bBullets, KEY_B, HTX_SHIP_BULLETS);
		optBullets = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_SHIP_WINGTIP_LASER, 0, KEY_A, 1, HTX_SHIP_WINGTIPS);
		optWingtips = opt++;
		ADD_RADIO (opt, TXT_SHIP_WINGTIP_SUPLAS, 0, KEY_U, 1, HTX_SHIP_WINGTIPS);
		opt++;
		ADD_RADIO (opt, TXT_SHIP_WINGTIP_SHORT, 0, KEY_S, 1, HTX_SHIP_WINGTIPS);
		opt++;
		ADD_RADIO (opt, TXT_SHIP_WINGTIP_LONG, 0, KEY_L, 1, HTX_SHIP_WINGTIPS);
		opt++;
		m [optWingtips + gameOpts->render.ship.nWingtip].value = 1;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_TEXT (opt, TXT_SHIPCOLOR_HEADER, 0);
		opt++;
		sprintf (szShipColor + 1, TXT_SHIPCOLOR, pszShipColors [gameOpts->render.ship.nColor]);
		*szShipColor = 0;
		ADD_SLIDER (opt, szShipColor + 1, gameOpts->render.ship.nColor, 0, 7, KEY_C, HTX_SHIPCOLOR);
		shipRenderOpts.nColor = opt++;
		}
	else
		optBullets =
		optWingtips =
		shipRenderOpts.nColor = -1;
	for (;;) {
		i = ExecMenu1 (NULL, TXT_SHIP_RENDERMENU, opt, m, ShipRenderOptionsCallback, &choice);
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

void RenderOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem	*m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + optBrightness;
	v = m->value;
	if (v != GrGetPaletteGamma ())
		GrSetPaletteGamma (v);
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
	v = (GR_ACTUAL_FADE_LEVELS * m->value + 5) / 10;
	if (extraGameInfo [0].grWallTransparency != v) {
		extraGameInfo [0].grWallTransparency = v;
		sprintf (m->text, TXT_WALL_TRANSP, m->value * 10, '%');
		m->rebuild = 1;
		}
	}
}

//------------------------------------------------------------------------------

void RenderOptionsMenu (void)
{
	tMenuItem m [40];
	int	h, i, choice = 0;
	int	opt;
	int	optSmokeOpts, optShadowOpts, optCameraOpts, optLightOpts, optMovieOpts,
			optAdvOpts, optEffectOpts, optPowerupOpts, optAutomapOpts, optLightningOpts,
			optUseGamma, optColoredWalls, optDepthSort, optCoronaOpts, optShipRenderOpts;
#ifdef _DEBUG
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
	opt = 0;
	optPowerupOpts = optAutomapOpts = -1;
	if (!gameStates.app.bNostalgia) {
		ADD_SLIDER (opt, TXT_BRIGHTNESS, GrGetPaletteGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = opt++;
		}
	if (gameOpts->render.nMaxFPS > 1)
		sprintf (szMaxFps + 1, TXT_FRAMECAP, gameOpts->render.nMaxFPS);
	else
		sprintf (szMaxFps + 1, TXT_NO_FRAMECAP);
	*szMaxFps = *(TXT_FRAMECAP - 1);
	ADD_SLIDER (opt, szMaxFps + 1, FindTableFps (gameOpts->render.nMaxFPS), 0, 15, KEY_F, HTX_RENDER_FRAMECAP);
	renderOpts.nMaxFPS = opt++;

	if (gameOpts->app.bExpertMode) {
		if (!(gameStates.render.color.bLightMapsOk && gameOpts->render.color.bUseLightMaps)) {
			sprintf (szContrast, TXT_CONTRAST, ContrastText ());
			ADD_SLIDER (opt, szContrast, gameStates.ogl.nContrast, 0, 16, KEY_C, HTX_ADVRND_CONTRAST);
			optContrast = opt++;
			}
		sprintf (szRendQual + 1, TXT_RENDQUAL, pszRendQual [gameOpts->render.nQuality]);
		*szRendQual = *(TXT_RENDQUAL - 1);
		ADD_SLIDER (opt, szRendQual + 1, gameOpts->render.nQuality, 0, 4, KEY_Q, HTX_ADVRND_RENDQUAL);
		renderOpts.nRenderQual = opt++;
		if (gameStates.app.bGameRunning)
			renderOpts.nTexQual =
			renderOpts.nMeshQual = 
			lightOpts.nLightmapQual = -1;
		else {
			sprintf (szTexQual + 1, TXT_TEXQUAL, pszTexQual [gameOpts->render.textures.nQuality]);
			*szTexQual = *(TXT_TEXQUAL + 1);
			ADD_SLIDER (opt, szTexQual + 1, gameOpts->render.textures.nQuality, 0, 3, KEY_U, HTX_ADVRND_TEXQUAL);
			renderOpts.nTexQual = opt++;
			if (gameOpts->render.nLightingMethod == 1) {
				sprintf (szMeshQual + 1, TXT_MESH_QUALITY, pszMeshQual [gameOpts->render.nMeshQuality]);
				*szMeshQual = *(TXT_MESH_QUALITY + 1);
				ADD_SLIDER (opt, szMeshQual + 1, gameOpts->render.nMeshQuality, 0, 4, KEY_O, HTX_MESH_QUALITY);
				renderOpts.nMeshQual = opt++;
				}
			else
				renderOpts.nMeshQual = -1;
			}
		ADD_TEXT (opt, "", 0);
		opt++;
		h = extraGameInfo [0].grWallTransparency * 10 / GR_ACTUAL_FADE_LEVELS;
		sprintf (szWallTransp + 1, TXT_WALL_TRANSP, h * 10, '%');
		*szWallTransp = *(TXT_WALL_TRANSP - 1);
		ADD_SLIDER (opt, szWallTransp + 1, h, 0, 10, KEY_T, HTX_ADVRND_WALLTRANSP);
		renderOpts.nWallTransp = opt++;
		ADD_CHECK (opt, TXT_COLOR_WALLS, gameOpts->render.color.bWalls, KEY_W, HTX_ADVRND_COLORWALLS);
		optColoredWalls = opt++;
		if (gameOpts->render.nPath)
			optDepthSort = -1;
		else {
			ADD_CHECK (opt, TXT_TRANSP_DEPTH_SORT, gameOpts->render.bDepthSort, KEY_D, HTX_TRANSP_DEPTH_SORT);
			optDepthSort = opt++;
			}
#if 0
		ADD_CHECK (opt, TXT_GAMMA_BRIGHT, gameOpts->ogl.bSetGammaRamp, KEY_V, HTX_ADVRND_GAMMA);
		optUseGamma = opt++;
#else
		optUseGamma = -1;
#endif
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_MENU (opt, TXT_LIGHTING_OPTIONS, KEY_L, HTX_RENDER_LIGHTINGOPTS);
		optLightOpts = opt++;
		ADD_MENU (opt, TXT_SMOKE_OPTIONS, KEY_S, HTX_RENDER_SMOKEOPTS);
		optSmokeOpts = opt++;
		ADD_MENU (opt, TXT_LIGHTNING_OPTIONS, KEY_I, HTX_LIGHTNING_OPTIONS);
		optLightningOpts = opt++;
		if (!(gameStates.app.bEnableShadows && gameStates.render.bHaveStencilBuffer))
			optShadowOpts = -1;
		else {
			ADD_MENU (opt, TXT_SHADOW_OPTIONS, KEY_A, HTX_RENDER_SHADOWOPTS);
			optShadowOpts = opt++;
			}
		ADD_MENU (opt, TXT_EFFECT_OPTIONS, KEY_E, HTX_RENDER_EFFECTOPTS);
		optEffectOpts = opt++;
		ADD_MENU (opt, TXT_CORONA_OPTIONS, KEY_O, HTX_RENDER_CORONAOPTS);
		optCoronaOpts = opt++;
		ADD_MENU (opt, TXT_CAMERA_OPTIONS, KEY_C, HTX_RENDER_CAMERAOPTS);
		optCameraOpts = opt++;
		ADD_MENU (opt, TXT_POWERUP_OPTIONS, KEY_P, HTX_RENDER_PRUPOPTS);
		optPowerupOpts = opt++;
		ADD_MENU (opt, TXT_AUTOMAP_OPTIONS, KEY_M, HTX_RENDER_AUTOMAPOPTS);
		optAutomapOpts = opt++;
		ADD_MENU (opt, TXT_SHIP_RENDEROPTIONS, KEY_H, HTX_RENDER_SHIPOPTS);
		optShipRenderOpts = opt++;
		ADD_MENU (opt, TXT_MOVIE_OPTIONS, KEY_M, HTX_RENDER_MOVIEOPTS);
		optMovieOpts = opt++;
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

#ifdef _DEBUG
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_CHECK (opt, "Draw wire frame", gameOpts->render.debug.bWireFrame, 0, NULL);
	optWireFrame = opt++;
	ADD_CHECK (opt, "Draw textures", gameOpts->render.debug.bTextures, 0, NULL);
	optTextures = opt++;
	ADD_CHECK (opt, "Draw walls", gameOpts->render.debug.bWalls, 0, NULL);
	optWalls = opt++;
	ADD_CHECK (opt, "Draw objects", gameOpts->render.debug.bObjects, 0, NULL);
	optObjects = opt++;
	ADD_CHECK (opt, "Dynamic Light", gameOpts->render.debug.bDynamicLight, 0, NULL);
	optDynLight = opt++;
#endif

	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, TXT_RENDER_OPTS, opt, m, &RenderOptionsCallback, &choice);
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
		GrSetPaletteGamma (m [optBrightness].value);
	if (gameOpts->app.bExpertMode) {
		gameOpts->render.color.bWalls = m [optColoredWalls].value;
		GET_VAL (gameOpts->render.bDepthSort, optDepthSort);
		GET_VAL (gameOpts->ogl.bSetGammaRamp, optUseGamma);
		if (gameStates.render.color.bLightMapsOk && gameOpts->render.color.bUseLightMaps)
			gameStates.ogl.nContrast = 8;
		else if (optContrast >= 0)
			gameStates.ogl.nContrast = m [optContrast].value;
		if (nRendQualSave != gameOpts->render.nQuality)
			SetRenderQuality ();
		}
#if EXPMODE_DEFAULTS
	else {
		gameOpts->render.nMaxFPS = 250;
		gameOpts->render.color.nLightMapRange = 5;
		gameOpts->render.color.bMix = 1;
		gameOpts->render.nQuality = 3;
		gameOpts->render.color.bWalls = 1;
		gameOpts->render.effects.bTransparent = 1;
		gameOpts->render.smoke.bPlayers = 0;
		gameOpts->render.smoke.bRobots =
		gameOpts->render.smoke.bMissiles = 1;
		gameOpts->render.smoke.bCollisions = 0;
		gameOpts->render.smoke.bDisperse = 0;
		gameOpts->render.smoke.nDens = 2;
		gameOpts->render.smoke.nSize = 3;
		gameOpts->render.cameras.bFitToWall = 0;
		gameOpts->render.cameras.nSpeed = 5000;
		gameOpts->render.cameras.nFPS = 0;
		gameOpts->movies.nQuality = 0;
		gameOpts->movies.bResize = 1;
		gameStates.ogl.nContrast = 8;
		gameOpts->ogl.bSetGammaRamp = 0;
		}
#endif
#ifdef _DEBUG
	gameOpts->render.debug.bWireFrame = m [optWireFrame].value;
	gameOpts->render.debug.bTextures = m [optTextures].value;
	gameOpts->render.debug.bObjects = m [optObjects].value;
	gameOpts->render.debug.bWalls = m [optWalls].value;
	gameOpts->render.debug.bDynamicLight = m [optDynLight].value;
#endif
	} while (i == -2);
}

//------------------------------------------------------------------------------

static char *pszGuns [] = {"Laser", "Vulcan", "Spreadfire", "Plasma", "Fusion", "Super Laser", "Gauss", "Helix", "Phoenix", "Omega"};
static char *pszDevices [] = {"Full Map", "Ammo Rack", "Converter", "Quad Lasers", "Afterburner", "Headlight", "Slow Motion", "Bullet Time"};
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

void LoadoutCallback (int nitems, tMenuItem * menus, int * key, int cItem)
{
	tMenuItem	*m = menus + cItem;
	int			v = m->value;

if (cItem == optGuns) {	//checked/unchecked lasers
	if (v != GetGunLoadoutFlag (0)) {
		SetGunLoadoutFlag (0, v);
		if (!v) {	//if lasers unchecked, also uncheck super lasers
			SetGunLoadoutFlag (5, 0);
			menus [optGuns + 5].value = 0;
			}
		}
	}
else if (cItem == optGuns + 5) {	//checked/unchecked super lasers
	if (v != GetGunLoadoutFlag (5)) {
		SetGunLoadoutFlag (5, v);
		if (v) {	// if super lasers checked, also check lasers
			SetGunLoadoutFlag (0, 1);
			menus [optGuns].value = 1;
			}
		}
	}
else if (cItem == optDevices + 6) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (6)) {
		SetDeviceLoadoutFlag (6, v);
		if (!v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (7, 0);
			menus [optDevices + 7].value = 0;
			}
		}
	}
else if (cItem == optDevices + 7) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (7)) {
		SetDeviceLoadoutFlag (7, v);
		if (v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (6, 1);
			menus [optDevices + 6].value = 1;
			}
		}
	}
}

//------------------------------------------------------------------------------

void LoadoutOptions (void)
{
	tMenuItem	m [25];
	int			i, opt = 0;

memset (m, 0, sizeof (m));
ADD_TEXT (opt, TXT_GUN_LOADOUT, 0);
opt++;
for (i = 0, optGuns = opt; i < sizeofa (pszGuns); i++, opt++)
	ADD_CHECK (opt, pszGuns [i], (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0, 0, HTX_GUN_LOADOUT);
ADD_TEXT (opt, "", 0);
opt++;
ADD_TEXT (opt, TXT_DEVICE_LOADOUT, 0);
opt++;
for (i = 0, optDevices = opt; i < sizeofa (pszDevices); i++, opt++)
	ADD_CHECK (opt, pszDevices [i], (extraGameInfo [0].loadout.nDevices & (nDeviceFlags [i])) != 0, 0, HTX_DEVICE_LOADOUT);
Assert (opt <= sizeofa (m));
do {
	i = ExecMenu1 (NULL, TXT_LOADOUT_MENUTITLE, opt, m, LoadoutCallback, 0);
	} while (i != -1);
extraGameInfo [0].loadout.nGuns = 0;
for (i = 0; i < sizeofa (pszGuns); i++) {
	if (m [optGuns + i].value)
		extraGameInfo [0].loadout.nGuns |= (1 << i);
	}
extraGameInfo [0].loadout.nDevices = 0;
for (i = 0; i < sizeofa (pszDevices); i++) {
	if (m [optDevices + i].value)
		extraGameInfo [0].loadout.nDevices |= nDeviceFlags [i];
	}
}

//------------------------------------------------------------------------------

static char *pszMslTurnSpeeds [3];
static char *pszMslStartSpeeds [4];

void GameplayOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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

m = menus + gplayOpts.nHeadLightAvailable;
v = m->value;
if (extraGameInfo [0].headlight.bAvailable != v) {
	extraGameInfo [0].headlight.bAvailable = v;
	*key = -2;
	return;
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
		return;
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
}

//------------------------------------------------------------------------------

#define D2X_MENU_GAP 0

void GameplayOptionsMenu (void)
{
	tMenuItem m [35];
	int	i, j, opt = 0, choice = 0;
	int	optFixedSpawn = -1, optSnipeMode = -1, optAutoSel = -1, optInventory = -1, 
			optDualMiss = -1, optDropAll = -1, optImmortal = -1, optMultiBosses = -1, optTripleFusion = -1,
			optEnhancedShakers = -1, optSmartWeaponSwitch = -1, optWeaponDrop = -1, optIdleAnims = -1, 
			optAwareness = -1, optHeadLightBuiltIn = -1, optHeadLightPowerDrain = -1, optHeadLightOnWhenPickedUp = -1,
			optRotateMarkers = -1, optLoadout;
	char	szRespawnDelay [60];
	char	szDifficulty [50], szMaxSmokeGrens [50];

do {
	memset (&m, 0, sizeof (m));
	opt = 0;
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	ADD_SLIDER (opt, szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	gplayOpts.nDifficulty = opt++;
	if (gameOpts->app.bExpertMode) {
		sprintf (szRespawnDelay + 1, TXT_RESPAWN_DELAY, (extraGameInfo [0].nSpawnDelay < 0) ? -1 : extraGameInfo [0].nSpawnDelay / 1000);
		*szRespawnDelay = *(TXT_RESPAWN_DELAY - 1);
		ADD_SLIDER (opt, szRespawnDelay + 1, extraGameInfo [0].nSpawnDelay / 5000 + 1, 0, 13, KEY_R, HTX_GPLAY_SPAWNDELAY);
		gplayOpts.nSpawnDelay = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_HEADLIGHT_AVAILABLE, extraGameInfo [0].headlight.bAvailable, KEY_A, HTX_HEADLIGHT_AVAILABLE);
		gplayOpts.nHeadLightAvailable = opt++;
		if (extraGameInfo [0].headlight.bAvailable) {
			ADD_CHECK (opt, TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadLightOnWhenPickedUp, KEY_O, HTX_MISC_HEADLIGHT);
			optHeadLightOnWhenPickedUp = opt++;
			ADD_CHECK (opt, TXT_HEADLIGHT_BUILTIN, extraGameInfo [0].headlight.bBuiltIn, KEY_L, HTX_HEADLIGHT_BUILTIN);
			optHeadLightBuiltIn = opt++;
			ADD_CHECK (opt, TXT_HEADLIGHT_POWERDRAIN, extraGameInfo [0].headlight.bDrainPower, KEY_W, HTX_HEADLIGHT_POWERDRAIN);
			optHeadLightPowerDrain = opt++;
			}
		ADD_CHECK (opt, TXT_USE_INVENTORY, gameOpts->gameplay.bInventory, KEY_V, HTX_GPLAY_INVENTORY);
		optInventory = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_MULTI_BOSSES, extraGameInfo [0].bMultiBosses, KEY_M, HTX_GPLAY_MULTIBOSS);
		optMultiBosses = opt++;
		ADD_CHECK (opt, TXT_IDLE_ANIMS, gameOpts->gameplay.bIdleAnims, KEY_D, HTX_GPLAY_IDLEANIMS);
		optIdleAnims = opt++;
		ADD_CHECK (opt, TXT_AI_AWARENESS, gameOpts->gameplay.nAIAwareness, KEY_I, HTX_GPLAY_AWARENESS);
		optAwareness = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_ALWAYS_RESPAWN, extraGameInfo [0].bImmortalPowerups, KEY_P, HTX_GPLAY_ALWAYSRESP);
		optImmortal = opt++;
		ADD_CHECK (opt, TXT_FIXED_SPAWN, extraGameInfo [0].bFixedRespawns, KEY_F, HTX_GPLAY_FIXEDSPAWN);
		optFixedSpawn = opt++;
		ADD_CHECK (opt, TXT_DROP_ALL, extraGameInfo [0].bDropAllMissiles, KEY_A, HTX_GPLAY_DROPALL);
		optDropAll = opt++;
		ADD_CHECK (opt, TXT_DROP_QUADSUPER, extraGameInfo [0].nWeaponDropMode, KEY_Q, HTX_GPLAY_DROPQUAD);
		optWeaponDrop = opt++;
		ADD_CHECK (opt, TXT_DUAL_LAUNCH, extraGameInfo [0].bDualMissileLaunch, KEY_U, HTX_GPLAY_DUALLAUNCH);
		optDualMiss = opt++;
		ADD_CHECK (opt, TXT_TRIPLE_FUSION, extraGameInfo [0].bTripleFusion, KEY_U, HTX_GPLAY_TRIFUSION);
		optTripleFusion = opt++;
		ADD_CHECK (opt, TXT_ENHANCED_SHAKERS, extraGameInfo [0].bEnhancedShakers, KEY_B, HTX_ENHANCED_SHAKERS);
		optEnhancedShakers = opt++;
		ADD_CHECK (opt, TXT_ROTATE_MARKERS, extraGameInfo [0].bRotateMarkers, KEY_K, HTX_ROTATE_MARKERS);
		optRotateMarkers = opt++;
		if (extraGameInfo [0].bSmokeGrenades) {
			ADD_TEXT (opt, "", 0);
			opt++;
			}
		ADD_CHECK (opt, TXT_GPLAY_SMOKEGRENADES, extraGameInfo [0].bSmokeGrenades, KEY_S, HTX_GPLAY_SMOKEGRENADES);
		gplayOpts.nSmokeGrens = opt++;
		if (extraGameInfo [0].bSmokeGrenades) {
			sprintf (szMaxSmokeGrens + 1, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
			*szMaxSmokeGrens = *(TXT_MAX_SMOKEGRENS - 1);
			ADD_SLIDER (opt, szMaxSmokeGrens + 1, extraGameInfo [0].nMaxSmokeGrenades - 1, 0, 3, KEY_X, HTX_GPLAY_MAXGRENADES);
			gplayOpts.nMaxSmokeGrens = opt++;
			}
		else
			gplayOpts.nMaxSmokeGrens = -1;
		}
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_CHECK (opt, TXT_SMART_WPNSWITCH, extraGameInfo [0].bSmartWeaponSwitch, KEY_W, HTX_GPLAY_SMARTSWITCH);
	optSmartWeaponSwitch = opt++;
	optAutoSel = opt;
	ADD_RADIO (opt, TXT_WPNSEL_NEVER, 0, KEY_N, 2, HTX_GPLAY_WSELNEVER);
	opt++;
	ADD_RADIO (opt, TXT_WPNSEL_EMPTY, 0, KEY_Y, 2, HTX_GPLAY_WSELEMPTY);
	opt++;
	ADD_RADIO (opt, TXT_WPNSEL_ALWAYS, 0, KEY_T, 2, HTX_GPLAY_WSELALWAYS);
	opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	optSnipeMode = opt;
	ADD_RADIO (opt, TXT_ZOOM_OFF, 0, KEY_D, 3, HTX_GPLAY_ZOOMOFF);
	opt++;
	ADD_RADIO (opt, TXT_ZOOM_FIXED, 0, KEY_X, 3, HTX_GPLAY_ZOOMFIXED);
	opt++;
	ADD_RADIO (opt, TXT_ZOOM_SMOOTH, 0, KEY_Z, 3, HTX_GPLAY_ZOOMSMOOTH);
	opt++;
	m [optAutoSel + NMCLAMP (gameOpts->gameplay.nAutoSelectWeapon, 0, 2)].value = 1;
	m [optSnipeMode + NMCLAMP (extraGameInfo [0].nZoomMode, 0, 2)].value = 1;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_MENU (opt, TXT_LOADOUT_OPTION, KEY_L, HTX_MULTI_LOADOUT);
	optLoadout = opt++;
	Assert (sizeofa (m) >= opt);
	for (;;) {
		if (0 > (i = ExecMenu1 (NULL, TXT_GAMEPLAY_OPTS, opt, m, &GameplayOptionsCallback, &choice)))
			break;
		if (choice == optLoadout)
			LoadoutOptions ();
		};
	} while (i == -2);
if (gameOpts->app.bExpertMode) {
	extraGameInfo [0].headlight.bAvailable = m [gplayOpts.nHeadLightAvailable].value;
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
	GET_VAL (gameOpts->gameplay.bHeadLightOnWhenPickedUp, optHeadLightOnWhenPickedUp);
	GET_VAL (extraGameInfo [0].headlight.bDrainPower, optHeadLightPowerDrain);
	GET_VAL (extraGameInfo [0].headlight.bBuiltIn, optHeadLightBuiltIn);
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
	gameOpts->gameplay.bHeadLightOnWhenPickedUp = 0;
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

static char *OmegaRampStr (void)
{
	static char szRamp [20];

if (extraGameInfo [0].nOmegaRamp == 0)
	return "1 sec";
sprintf (szRamp, "%d secs", nOmegaDuration [(int) extraGameInfo [0].nOmegaRamp]);
return szRamp;
}

//------------------------------------------------------------------------------

void PhysicsOptionsCallback (int nitems, tMenuItem * menus, int * key, int citem)
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
		return;
		}

	m = menus + nOptDebrisLife;
	v = m->value;
	if (gameOpts->render.nDebrisLife != v) {
		gameOpts->render.nDebrisLife = v;
		sprintf (m->text, TXT_DEBRIS_LIFE, nDebrisLife [v]);
		m->rebuild = -1;
		}
	}
}

//------------------------------------------------------------------------------

#define D2X_MENU_GAP 0

void PhysicsOptionsMenu (void)
{
	tMenuItem m [30];
	int	i, opt = 0, choice = 0;
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
	opt = 0;
	if (gameOpts->app.bExpertMode) {
		sprintf (szSpeedBoost + 1, TXT_SPEEDBOOST, extraGameInfo [0].nSpeedBoost * 10, '%');
		*szSpeedBoost = *(TXT_SPEEDBOOST - 1);
		ADD_SLIDER (opt, szSpeedBoost + 1, extraGameInfo [0].nSpeedBoost, 0, 10, KEY_B, HTX_GPLAY_SPEEDBOOST);
		physOpts.nSpeedboost = opt++;
		sprintf (szFusionRamp + 1, TXT_FUSION_RAMP, extraGameInfo [0].nFusionRamp * 50, '%');
		*szFusionRamp = *(TXT_FUSION_RAMP - 1);
		ADD_SLIDER (opt, szFusionRamp + 1, extraGameInfo [0].nFusionRamp - 2, 0, 6, KEY_U, HTX_FUSION_RAMP);
		physOpts.nFusionRamp = opt++;
		sprintf (szOmegaRamp + 1, TXT_OMEGA_RAMP, OmegaRampStr ());
		*szOmegaRamp = *(TXT_OMEGA_RAMP - 1);
		ADD_SLIDER (opt, szOmegaRamp + 1, extraGameInfo [0].nOmegaRamp, 0, 6, KEY_O, HTX_OMEGA_RAMP);
		physOpts.nOmegaRamp = opt++;
		sprintf (szMslTurnSpeed + 1, TXT_MSL_TURNSPEED, pszMslTurnSpeeds [(int) extraGameInfo [0].nMslTurnSpeed]);
		*szMslTurnSpeed = *(TXT_MSL_TURNSPEED - 1);
		ADD_SLIDER (opt, szMslTurnSpeed + 1, extraGameInfo [0].nMslTurnSpeed, 0, 2, KEY_T, HTX_GPLAY_MSL_TURNSPEED);
		physOpts.nMslTurnSpeed = opt++;
		sprintf (szMslStartSpeed + 1, TXT_MSL_STARTSPEED, pszMslStartSpeeds [(int) 3 - extraGameInfo [0].nMslStartSpeed]);
		*szMslStartSpeed = *(TXT_MSL_STARTSPEED - 1);
		ADD_SLIDER (opt, szMslStartSpeed + 1, 3 - extraGameInfo [0].nMslStartSpeed, 0, 3, KEY_S, HTX_MSL_STARTSPEED);
		physOpts.nMslStartSpeed = opt++;
		sprintf (szSlowmoSpeedup + 1, TXT_SLOWMOTION_SPEEDUP, (float) gameOpts->gameplay.nSlowMotionSpeedup / 2);
		*szSlowmoSpeedup = *(TXT_SLOWMOTION_SPEEDUP - 1);
		ADD_SLIDER (opt, szSlowmoSpeedup + 1, gameOpts->gameplay.nSlowMotionSpeedup, 0, 4, KEY_M, HTX_SLOWMOTION_SPEEDUP);
		physOpts.nSlomoSpeedup = opt++;
		sprintf (szDebrisLife + 1, TXT_DEBRIS_LIFE, nDebrisLife [gameOpts->render.nDebrisLife]);
		*szDebrisLife = *(TXT_DEBRIS_LIFE - 1);
		ADD_SLIDER (opt, szDebrisLife + 1, gameOpts->render.nDebrisLife, 0, 8, KEY_D, HTX_DEBRIS_LIFE);
		nOptDebrisLife = opt++;
		sprintf (szDrag + 1, TXT_PLAYER_DRAG, extraGameInfo [0].nDrag * 10, '%');
		*szDrag = *(TXT_PLAYER_DRAG - 1);
		ADD_SLIDER (opt, szDrag + 1, extraGameInfo [0].nDrag, 0, 10, KEY_G, HTX_PLAYER_DRAG);
		physOpts.nDrag = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		m [optAutoLevel + gameOpts->gameplay.nAutoLeveling].value = 1;
		if (!extraGameInfo [0].nDrag)
			optWiggle = -1;
		else {
			ADD_CHECK (opt, TXT_WIGGLE_SHIP, extraGameInfo [0].bWiggle, KEY_W, HTX_MISC_WIGGLE);
			optWiggle = opt++;
			}
		ADD_CHECK (opt, TXT_BOTS_HIT_BOTS, extraGameInfo [0].bRobotsHitRobots, KEY_O, HTX_GPLAY_BOTSHITBOTS);
		optRobHits = opt++;
		ADD_CHECK (opt, TXT_FLUID_PHYS, extraGameInfo [0].bFluidPhysics, KEY_Y, HTX_GPLAY_FLUIDPHYS);
		optFluidPhysics = opt++;
		ADD_CHECK (opt, TXT_USE_HITANGLES, extraGameInfo [0].bUseHitAngles, KEY_H, HTX_GPLAY_HITANGLES);
		optHitAngles = opt++;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_IMMUNE_MISSILES, 0, KEY_I, 1, HTX_KILL_MISSILES);
		optKillMissiles = opt++;
		ADD_RADIO (opt, TXT_OMEGA_KILLS_MISSILES, 0, KEY_K, 1, HTX_KILL_MISSILES);
		opt++;
		ADD_RADIO (opt, TXT_KILL_MISSILES, 0, KEY_A, 1, HTX_KILL_MISSILES);
		opt++;
		m [optKillMissiles + extraGameInfo [0].bKillMissiles].value = 1;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_AUTOLEVEL_NONE, 0, KEY_N, 2, HTX_AUTO_LEVELLING);
		optAutoLevel = opt++;
		ADD_RADIO (opt, TXT_AUTOLEVEL_SIDE, 0, KEY_S, 2, HTX_AUTO_LEVELLING);
		opt++;
		ADD_RADIO (opt, TXT_AUTOLEVEL_FLOOR, 0, KEY_F, 2, HTX_AUTO_LEVELLING);
		opt++;
		ADD_RADIO (opt, TXT_AUTOLEVEL_GLOBAL, 0, KEY_M, 2, HTX_AUTO_LEVELLING);
		opt++;
		m [optAutoLevel + NMCLAMP (gameOpts->gameplay.nAutoLeveling, 0, 3)].value = 1;
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_RADIO (opt, TXT_HIT_SPHERES, 0, KEY_W, 3, HTX_GPLAY_HITBOXES);
		optHitboxes = opt++;
		ADD_RADIO (opt, TXT_SIMPLE_HITBOXES, 0, KEY_W, 3, HTX_GPLAY_HITBOXES);
		opt++;
		ADD_RADIO (opt, TXT_COMPLEX_HITBOXES, 0, KEY_W, 3, HTX_GPLAY_HITBOXES);
		opt++;
		m [optHitboxes + NMCLAMP (extraGameInfo [0].nHitboxes, 0, 2)].value = 1;
		}
	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, TXT_PHYSICS_MENUTITLE, opt, m, &PhysicsOptionsCallback, &choice);
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
	int			i, choice = 0;

memset (m, 0, sizeof (m));
for (i = rtStaticVertLight; i <= rtPolyModel; i++)
	ADD_CHECK (i, GT (1060 + i), bUseMultiThreading [i], 0, HT (359 + i));
i = ExecMenu1 (NULL, TXT_MT_MENU_TITLE, 10, m, NULL, &choice);
for (i = rtStaticVertLight; i <= rtPolyModel; i++)
	bUseMultiThreading [i] = (m [i].value != 0);
}

//------------------------------------------------------------------------------

void ConfigMenu (void)
{
	tMenuItem	m [20];
	int			i, opt, choice = 0;
	int			optSound, optConfig, optJoyCal, optPerformance, optScrRes, optReorderPrim, optReorderSec, 
					optMiscellaneous, optMultiThreading = -1, optRender, optGameplay, optCockpit, optPhysics = -1;

do {
	memset (m, 0, sizeof (m));
	optRender = optGameplay = optCockpit = optPerformance = -1;
	opt = 0;
	ADD_MENU (opt, TXT_SOUND_MUSIC, KEY_M, HTX_OPTIONS_SOUND);
	optSound = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_MENU (opt, TXT_CONTROLS_, KEY_O, HTX_OPTIONS_CONFIG);
	strupr (m [opt].text);
	optConfig = opt++;
#if defined (_WIN32) || defined (__linux__)
	optJoyCal = -1;
#else
	ADD_MENU (opt, TXT_CAL_JOYSTICK, KEY_J, HTX_OPTIONS_CALSTICK);
	optJoyCal = opt++;
#endif
	ADD_TEXT (opt, "", 0);
	opt++;
	if (gameStates.app.bNostalgia) {
		ADD_SLIDER (opt, TXT_BRIGHTNESS, GrGetPaletteGamma (), 0, 16, KEY_B, HTX_RENDER_BRIGHTNESS);
		optBrightness = opt++;
		}
	
	if (gameStates.app.bNostalgia)
		ADD_MENU (opt, TXT_DETAIL_LEVELS, KEY_D, HTX_OPTIONS_DETAIL);
	else if (gameStates.app.bGameRunning)
		optPerformance = -1;
	else {
		ADD_MENU (opt, TXT_SETPERF_OPTION, KEY_E, HTX_PERFORMANCE_SETTINGS);
		optPerformance = opt++;
		}
	ADD_MENU (opt, TXT_SCREEN_RES, KEY_S, HTX_OPTIONS_SCRRES);
	optScrRes = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_MENU (opt, TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
	optReorderPrim = opt++;
	ADD_MENU (opt, TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
	optReorderSec = opt++;
	ADD_MENU (opt, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISCELLANEOUS, gameStates.app.bNostalgia ? KEY_T : KEY_I, HTX_OPTIONS_MISC);
	optMiscellaneous = opt++;
	if (!gameStates.app.bNostalgia) {
		ADD_MENU (opt, TXT_COCKPIT_OPTS2, KEY_C, HTX_OPTIONS_COCKPIT);
		optCockpit = opt++;
		ADD_MENU (opt, TXT_RENDER_OPTS2, KEY_R, HTX_OPTIONS_RENDER);
		optRender = opt++;
		if (IsMultiGame && !IsCoopGame) 
			optPhysics =
			optGameplay = -1;
		else {
			ADD_MENU (opt, TXT_GAMEPLAY_OPTS2, KEY_G, HTX_OPTIONS_GAMEPLAY);
			optGameplay = opt++;
			ADD_MENU (opt, TXT_PHYSICS_MENUCALL, KEY_Y, HTX_OPTIONS_PHYSICS);
			optPhysics = opt++;
			}
		if (gameStates.app.bMultiThreaded) {
			ADD_MENU (opt, TXT_MT_MENU_OPTION, KEY_U, HTX_MULTI_THREADING);
			optMultiThreading = opt++;
			}
		}

	i = ExecMenu1 (NULL, TXT_OPTIONS, opt, m, options_menuset, &choice);
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

for (h = sizeofa (detailData.nSoundChannels), i = 0; i < h; i++)
	if (gameStates.sound.digi.nMaxChannels < detailData.nSoundChannels [i])
		break;
return i - 1;
}

//------------------------------------------------------------------------------

void SoundMenuCallback (int nitems, tMenuItem * m, int *nLastKey, int citem)
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
	return;
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
	m [soundOpts.nMusicVol].text = gameStates.sound.bRedbookEnabled ? TXT_CD_VOLUME : TXT_MIDI_VOLUME;
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

 		gameConfig.nMidiVolume = m [soundOpts.nMusicVol].value;
		DigiSetMidiVolume ((gameConfig.nMidiVolume*128)/8);
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
citem++;		//kill warning
}

//------------------------------------------------------------------------------

void SoundMenu ()
{
   tMenuItem	m [20];
	char			szChannels [50], szVolume [50];
	int			i, opt, choice = 0, 
					optReverse, optShipSound = -1, optMissileSound = -1, optSpeedUpSound = -1, 
					bSongPlaying = (gameConfig.nMidiVolume > 0);

gameStates.sound.nSoundChannels = SoundChannelIndex ();
do {
	memset (m, 0, sizeof (m));
	opt = 0;
	soundOpts.nGatling = -1;
	ADD_SLIDER (opt, TXT_FX_VOLUME, gameConfig.nDigiVolume, 0, 8, KEY_F, HTX_ONLINE_MANUAL);
	soundOpts.nDigiVol = opt++;
	ADD_SLIDER (opt, gameStates.sound.bRedbookEnabled ? TXT_CD_VOLUME : TXT_MIDI_VOLUME, 
					gameStates.sound.bRedbookEnabled ? gameConfig.nRedbookVolume : gameConfig.nMidiVolume, 
					0, 8, KEY_M, HTX_ONLINE_MANUAL);
	soundOpts.nMusicVol = opt++;
	if (gameStates.app.bGameRunning || gameStates.app.bNostalgia)
		soundOpts.nChannels = -1;
	else {
		sprintf (szChannels + 1, TXT_SOUND_CHANNEL_COUNT, gameStates.sound.digi.nMaxChannels);
		*szChannels = *(TXT_SOUND_CHANNEL_COUNT - 1);
		ADD_SLIDER (opt, szChannels + 1, gameStates.sound.nSoundChannels, 0, 
						sizeofa (detailData.nSoundChannels) - 1, KEY_C, HTX_SOUND_CHANNEL_COUNT);  
		soundOpts.nChannels = opt++;
		}
	if (!gameStates.app.bNostalgia) {
		sprintf (szVolume + 1, TXT_CUSTOM_SOUNDVOL, gameOpts->sound.xCustomSoundVolume * 10, '%');
		*szVolume = *(TXT_CUSTOM_SOUNDVOL - 1);
		ADD_SLIDER (opt, szVolume + 1, gameOpts->sound.xCustomSoundVolume, 0, 
						10, KEY_C, HTX_CUSTOM_SOUNDVOL);  
		soundOpts.nVolume = opt++;
		}
	if (gameStates.app.bNostalgia || !gameOpts->sound.xCustomSoundVolume) 
		optShipSound = 
		optMissileSound =
		optSpeedUpSound =
		soundOpts.nGatling = -1;
	else {
		ADD_TEXT (opt, "", 0);
		opt++;
		ADD_CHECK (opt, TXT_SHIP_SOUND, gameOpts->sound.bShip, KEY_S, HTX_SHIP_SOUND);
		optShipSound = opt++;
		ADD_CHECK (opt, TXT_MISSILE_SOUND, gameOpts->sound.bMissiles, KEY_M, HTX_MISSILE_SOUND);
		optMissileSound = opt++;
		ADD_CHECK (opt, TXT_GATLING_SOUND, gameOpts->sound.bGatling, KEY_G, HTX_GATLING_SOUND);
		soundOpts.nGatling = opt++;
		if (gameOpts->sound.bGatling) {
			ADD_CHECK (opt, TXT_SPINUP_SOUND, extraGameInfo [0].bGatlingSpeedUp, KEY_U, HTX_SPINUP_SOUND);
			optSpeedUpSound = opt++;
			}
		}
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_CHECK (opt, TXT_REDBOOK_ENABLED, gameStates.sound.bRedbookPlaying, KEY_C, HTX_ONLINE_MANUAL);
	soundOpts.nRedbook = opt++;
	ADD_CHECK (opt, TXT_REVERSE_STEREO, gameConfig.bReverseChannels, KEY_R, HTX_ONLINE_MANUAL);
	optReverse = opt++;
	Assert (sizeofa (m) >= opt);
	i = ExecMenu1 (NULL, TXT_SOUND_OPTS, opt, m, SoundMenuCallback, &choice);
	gameStates.sound.bRedbookEnabled = m [soundOpts.nRedbook].value;
	gameConfig.bReverseChannels = m [optReverse].value;
} while (i == -2);

if (gameConfig.nMidiVolume < 1)   {
		DigiPlayMidiSong (NULL, NULL, NULL, 0, 0);
	}
else if (!bSongPlaying)
	SongsPlaySong (gameStates.sound.nCurrentSong, 1);
if (!gameStates.app.bNostalgia) {
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

void MiscellaneousCallback (int nitems, tMenuItem * menus, int * key, int citem)
{
	tMenuItem * m;
	int			v;

if (!gameStates.app.bNostalgia) {
	m = menus + performanceOpts.nUseCompSpeed;
	v = m->value;
	if (gameStates.app.bUseDefaults != v) {
		gameStates.app.bUseDefaults = v;
		*key = -2;
		return;
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
		return;
		}
	}
m = menus + miscOpts.nExpertMode;
v = m->value;
if (gameOpts->app.bExpertMode != v) {
	gameOpts->app.bExpertMode = v;
	*key = -2;
	return;
	}
m = menus + miscOpts.nAutoDl;
v = m->value;
if (extraGameInfo [0].bAutoDownload != v) {
	extraGameInfo [0].bAutoDownload = v;
	*key = -2;
	return;
	}
if (gameOpts->app.bExpertMode) {
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

//------------------------------------------------------------------------------

void MiscellaneousMenu ()
{
	tMenuItem m [20];
	int	i, opt, choice,
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
	i = opt = 0;
	memset (m, 0, sizeof (m));
	optReticle = optMissileView = optGuided = optSmartSearch = optLevelVer = optDemoFmt = -1;
	if (gameStates.app.bNostalgia) {
		ADD_CHECK (0, TXT_AUTO_LEVEL, gameOpts->gameplay.nAutoLeveling, KEY_L, HTX_MISC_AUTOLEVEL);
		optAutoLevel = opt++;
		ADD_CHECK (opt, TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optReticle = opt++;
		ADD_CHECK (opt, TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
		optMissileView = opt++;
		ADD_CHECK (opt, TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_G, HTX_CPIT_GUIDEDVIEW);
		optGuided = opt++;
		ADD_CHECK (opt, TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadLightOnWhenPickedUp, KEY_H, HTX_MISC_HEADLIGHT);
		optHeadlight = opt++;
		}
	else
		optHeadlight = 
		optAutoLevel = -1;
	ADD_CHECK (opt, TXT_ESCORT_KEYS, gameOpts->gameplay.bEscortHotKeys, KEY_K, HTX_MISC_ESCORTKEYS);
	optEscort = opt++;
#if 0
	ADD_CHECK (opt, TXT_FAST_RESPAWN, gameOpts->gameplay.bFastRespawn, KEY_F, HTX_MISC_FASTRESPAWN);
	optFastResp = opt++;
#endif
	ADD_CHECK (opt, TXT_USE_MACROS, gameOpts->multi.bUseMacros, KEY_M, HTX_MISC_USEMACROS);
	optUseMacros = opt++;
	if (!(gameStates.app.bNostalgia || gameStates.app.bGameRunning)) {
#if UDP_SAFEMODE
		ADD_CHECK (opt, TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MISC_UDPQUAL);
		optSafeUDP=opt++;
#endif
		}
	if (!gameStates.app.bNostalgia) {
		if (gameOpts->app.bExpertMode) {
			ADD_CHECK (opt, TXT_SMART_SEARCH, gameOpts->menus.bSmartFileSearch, KEY_S, HTX_MISC_SMARTSEARCH);
			optSmartSearch = opt++;
			ADD_CHECK (opt, TXT_SHOW_LVL_VERSION, gameOpts->menus.bShowLevelVersion, KEY_V, HTX_MISC_SHOWLVLVER);
			optLevelVer = opt++;
			}
		else
			optSmartSearch =
			optLevelVer = -1;
		ADD_CHECK (opt, TXT_EXPERT_MODE, gameOpts->app.bExpertMode, KEY_X, HTX_MISC_EXPMODE);
		miscOpts.nExpertMode = opt++;
		ADD_CHECK (opt, TXT_OLD_DEMO_FORMAT, gameOpts->demo.bOldFormat, KEY_C, HTX_OLD_DEMO_FORMAT);
		optDemoFmt = opt++;
		}
	if (gameStates.app.bNostalgia < 2) {
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			ADD_TEXT (opt, "", 0);
			opt++;
			}
		ADD_CHECK (opt, TXT_AUTODL_ENABLE, extraGameInfo [0].bAutoDownload, KEY_A, HTX_MISC_AUTODL);
		miscOpts.nAutoDl = opt++;
		if (extraGameInfo [0].bAutoDownload && gameOpts->app.bExpertMode) {
			sprintf (szDlTimeout + 1, TXT_AUTODL_TO, GetDlTimeoutSecs ());
			*szDlTimeout = *(TXT_AUTODL_TO - 1);
			ADD_SLIDER (opt, szDlTimeout + 1, GetDlTimeout (), 0, MaxDlTimeout (), KEY_T, HTX_MISC_AUTODLTO);  
			miscOpts.nDlTimeout = opt++;
			}
		ADD_TEXT (opt, "", 0);
		opt++;
		if (gameOpts->app.nScreenShotInterval)
			sprintf (szScreenShots + 1, TXT_SCREENSHOTS, screenShotIntervals [gameOpts->app.nScreenShotInterval]);
		else
			strcpy (szScreenShots + 1, TXT_NO_SCREENSHOTS);
		*szScreenShots = *(TXT_SCREENSHOTS - 1);
		ADD_SLIDER (opt, szScreenShots + 1, gameOpts->app.nScreenShotInterval, 0, 7, KEY_S, HTX_MISC_SCREENSHOTS);  
		miscOpts.nScreenshots = opt++;
		}
	Assert (sizeofa (m) >= opt);
	do {
		i = ExecMenu1 (NULL, gameStates.app.bNostalgia ? TXT_TOGGLES : TXT_MISC_TITLE, opt, m, MiscellaneousCallback, &choice);
	} while (i >= 0);
	if (gameStates.app.bNostalgia) {
		gameOpts->gameplay.nAutoLeveling = m [optAutoLevel].value;
		gameOpts->render.cockpit.bReticle = m [optReticle].value;
		gameOpts->render.cockpit.bMissileView = m [optMissileView].value;
		gameOpts->render.cockpit.bGuidedInMainView = m [optGuided].value;
		gameOpts->gameplay.bHeadLightOnWhenPickedUp = m [optHeadlight].value;
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
	int	i, choice = 0, opt, optQuit, optOptions, optLoad, optSave;

memset (m, 0, sizeof (m));
opt = 0;
ADD_MENU (opt, TXT_QUIT_GAME, KEY_Q, HTX_QUIT_GAME);
optQuit = opt++;
ADD_MENU (opt, TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
optOptions = opt++;
ADD_MENU (opt, TXT_LOAD_GAME2, KEY_L, HTX_LOAD_GAME);
optLoad = opt++;
ADD_MENU (opt, TXT_SAVE_GAME2, KEY_S, HTX_SAVE_GAME);
optSave = opt++;
i = ExecMenu1 (NULL, TXT_ABORT_GAME, opt, m, NULL, &choice);
if (!i)
	return 0;
if (i == optOptions)
	ConfigMenu ();
else if (i == optLoad)
	StateRestoreAll (1, 0, NULL);
else if (i == optSave)
	StateSaveAll (0, 0, NULL);
return 1;
}

//------------------------------------------------------------------------------

static inline int MultiChoice (int nType, int bJoin)
{
return *(((int *) &multiOpts) + 2 * nType + bJoin);
}

void MultiplayerMenu ()
{
	tMenuItem	m [15];
	int choice = 0, opt = 0, i, optCreate, optJoin = -1, optConn = -1, nConnections = 0;
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
		opt = 0;
		if (gameStates.app.bNostalgia < 2) {
			ADD_MENU (opt, TXT_CREATE_GAME, KEY_S, HTX_NETWORK_SERVER);
			optCreate = opt++;
			ADD_MENU (opt, TXT_JOIN_GAME, KEY_J, HTX_NETWORK_CLIENT);
			optJoin = opt++;
			ADD_TEXT (opt, "", 0);
			opt++;
			ADD_RADIO (opt, TXT_NGTYPE_IPX, 0, KEY_I, 0, HTX_NETWORK_IPX);
			optConn = opt++;
			ADD_RADIO (opt, TXT_NGTYPE_UDP, 0, KEY_U, 0, HTX_NETWORK_UDP);
			opt++;
			ADD_RADIO (opt, TXT_NGTYPE_TRACKER, 0, KEY_T, 0, HTX_NETWORK_TRACKER);
			opt++;
			ADD_RADIO (opt, TXT_NGTYPE_MCAST4, 0, KEY_M, 0, HTX_NETWORK_MCAST);
			opt++;
#ifdef KALINIX
			ADD_RADIO (opt, TXT_NGTYPE_KALI, 0, KEY_K, 0, HTX_NETWORK_KALI);
			opt++;
#endif
			nConnections = opt;
			m [optConn + NMCLAMP (gameStates.multi.nConnection, 0, nConnections - optConn)].value = 1;
			}
		else {
#ifdef NATIVE_IPX
			ADD_MENU (opt, TXT_START_IPX_NET_GAME,  -1, HTX_NETWORK_IPX);
			multiOpts.nStartIpx = opt++;
			ADD_MENU (opt, TXT_JOIN_IPX_NET_GAME, -1, HTX_NETWORK_IPX);
			multiOpts.nJoinIpx = opt++;
#endif //NATIVE_IPX
			ADD_MENU (opt, TXT_MULTICAST_START, KEY_M, HTX_NETWORK_MCAST);
			multiOpts.nStartMCast4 = opt++;
			ADD_MENU (opt, TXT_MULTICAST_JOIN, KEY_N, HTX_NETWORK_MCAST);
			multiOpts.nJoinMCast4 = opt++;
#ifdef KALINIX
			ADD_MENU (opt, TXT_KALI_START, KEY_K, HTX_NETWORK_KALI);
			multiOpts.nStartKali = opt++;
			ADD_MENU (opt, TXT_KALI_JOIN, KEY_I, HTX_NETWORK_KALI);
			multiOpts.nJoinKali = opt++;
#endif // KALINIX
			if (gameStates.app.bNostalgia > 2) {
				ADD_MENU (opt, TXT_MODEM_GAME2, KEY_G, HTX_NETWORK_MODEM);
				multiOpts.nSerial = opt++;
				}
			}
		i = ExecMenu1 (NULL, TXT_MULTIPLAYER, opt, m, NULL, &choice);
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
if (CFExist ((char *) MENU_PCX_FULL, gameFolders.szDataDir, 0))
	return (char *) MENU_PCX_FULL;
if (CFExist ((char *) MENU_PCX_OEM, gameFolders.szDataDir, 0))
	return (char *) MENU_PCX_OEM;
if (CFExist ((char *) MENU_PCX_SHAREWARE, gameFolders.szDataDir, 0))
	return (char *) MENU_PCX_SHAREWARE;
return (char *) MENU_PCX_MAC_SHARE;
}
//------------------------------------------------------------------------------
//eof
