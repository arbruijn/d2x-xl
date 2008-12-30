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
#include "menu.h"
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
#include "menubackground.h"
#ifdef EDITOR
#	include "editor/editor.h"
#endif

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

//------------------------------------------------------------------------------

// Function Prototypes added after LINTING
int ExecMainMenuOption (int nChoice);
int ExecMultiMenuOption (int nChoice);

//returns the number of demo files on the disk
int NDCountDemos (void);

// ------------------------------------------------------------------------

int AutoDemoMenuCheck (CMenu& m, int& nLastKey, int nCurItem)
{
	int curtime;

PrintVersionInfo ();
// Don't allow them to hit ESC in the main menu.
if (nLastKey == KEY_ESC) 
	nLastKey = 0;
if (gameStates.app.bAutoDemos) {
	curtime = TimerGetApproxSeconds ();
	if (((gameStates.input.keys.xLastPressTime + I2X (/*2*/5)) < curtime)
#if DBG
		&& !gameData.speedtest.bOn
#endif	
		) {
		int nDemos = NDCountDemos ();
		for (;;) {
			if ((d_rand () % (nDemos+1)) == 0) {
				gameStates.video.nScreenMode = -1;
				movieManager.PlayIntro ();
				songManager.Play (SONG_TITLE, 1);
				nLastKey = -3; //exit menu to force rebuild even if not going to game mode. -3 tells menu system not to restore
				SetScreenMode (SCREEN_MENU);
				break;
				}
			else {
				gameStates.input.keys.xLastPressTime = curtime;                  // Reset timer so that disk won't thrash if no demos.
				NDStartPlayback (NULL);           // Randomly pick a file
				if (gameData.demo.nState == ND_STATE_PLAYBACK) {
					SetFunctionMode (FMODE_GAME);
					nLastKey = -3; //exit menu to get into game mode. -3 tells menu system not to restore
					break;
					}
				}
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------
//      Create the main menu.
int SetupMainMenu (CMenu& m)
{
memset (&mainOpts, 0xff, sizeof (mainOpts));
m.Destroy ();
m.Create (25);
#ifndef DEMO_ONLY
SetScreenMode (SCREEN_MENU);
#if 1
m.AddText ("", 0);
m.Top ()->m_bNoScroll = 1;
m.AddText ("", 0);
m.Top ()->m_bNoScroll = 1;
m.AddText ("", 0);
m.Top ()->m_bNoScroll = 1;
#endif
mainOpts.nNew = m.AddMenu (TXT_NEW_GAME1, KEY_N, HTX_MAIN_NEW);
if (!gameStates.app.bNostalgia)
	mainOpts.nSingle = m.AddMenu (TXT_NEW_SPGAME, KEY_I, HTX_MAIN_SINGLE);
mainOpts.nLoad = m.AddMenu (TXT_LOAD_GAME, KEY_L, HTX_MAIN_LOAD);
mainOpts.nMulti = m.AddMenu (TXT_MULTIPLAYER_, KEY_M, HTX_MAIN_MULTI);
if (gameStates.app.bNostalgia)
	mainOpts.nConfig = m.AddMenu (TXT_OPTIONS_, KEY_O, HTX_MAIN_CONF);
else
	mainOpts.nConfig = m.AddMenu (TXT_CONFIGURE, KEY_O, HTX_MAIN_CONF);
mainOpts.nPilots = m.AddMenu (TXT_CHANGE_PILOTS, KEY_P, HTX_MAIN_PILOT);
mainOpts.nDemo = m.AddMenu (TXT_VIEW_DEMO, KEY_D, HTX_MAIN_DEMO);
mainOpts.nScores = m.AddMenu (TXT_VIEW_SCORES, KEY_H, HTX_MAIN_SCORES);
#if 0
if (CFile::Exist ("orderd2.pcx", gameFolders.szDataDir, 0)) // SHAREWARE
	mainOpts.nOrder = m.AddMenu (TXT_ORDERING_INFO, -1, NULL);
#endif
mainOpts.nMovies = m.AddMenu (TXT_PLAY_MOVIES, KEY_V, HTX_MAIN_MOVIES);
if (!gameStates.app.bNostalgia)
	mainOpts.nSongs = m.AddMenu (TXT_PLAY_SONGS, KEY_S, HTX_MAIN_SONGS);
mainOpts.nCredits = m.AddMenu (TXT_CREDITS, KEY_C, HTX_MAIN_CREDITS);
#endif
mainOpts.nQuit = m.AddMenu (TXT_QUIT, KEY_Q, HTX_MAIN_QUIT);
return m.ToS ();
}

//------------------------------------------------------------------------------
//returns number of item chosen
int MainMenu (void) 
{
	CMenu	m;
	int	i, nChoice = 0, nOptions = 0;

IpxClose ();
//paletteManager.Load (MENU_PALETTE, NULL, 0, 1, 0);		//get correct palette

if (!LOCALPLAYER.callsign [0]) {
	SelectPlayer ();
	return 0;
	}
if (gameData.multiplayer.autoNG.bValid) {
	ExecMultiMenuOption (mainOpts.nMulti);
	return 0;
	}
PrintLog ("launching main menu\n");
do {
	nOptions = SetupMainMenu (m); // may have to change, eg, maybe selected pilot and no save games.
	gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds ();                // .. 20 seconds from now!
	if (nChoice < 0)
		nChoice = 0;
	gameStates.menus.bDrawCopyright = 1;
	i = m.Menu ("", NULL, AutoDemoMenuCheck, &nChoice, MenuPCXName ());
	if (gameStates.app.bNostalgia)
		gameOpts->app.nVersionFilter = 3;
	WritePlayerFile ();
	if (i > -1)
		ExecMainMenuOption (nChoice);
} while (gameStates.app.nFunctionMode == FMODE_MENU);
if (gameStates.app.nFunctionMode == FMODE_GAME)
	paletteManager.DisableEffect ();
FlushInput ();
StopPlayerMovement ();
return mainOpts.nChoice;
}

//------------------------------------------------------------------------------

static void PlayMenuMovie (void)
{
	int				h, i, j;
	CStack<char*>	m;
	char*				ps;

i = movieManager.m_nLibs;
for (h = j = 0; j < i; j++)
	if (j != 2)	//skip robot movies
		h += movieManager.m_libs [j].m_nMovies;
if (!h)
	return;
if (!m.Create (h))
	return;
for (i = j = 0; i < h; i++)
	if ((ps = movieManager.Cycle (i == 0, 0))) {
		if (j && !strcmp (ps, m [0]))
			break;
		m.Push (ps);
		}
i = ListBox (TXT_SELECT_MOVIE, m);
if (i > -1) {
	SDL_ShowCursor (0);
	if (strstr (m [i], "intro"))
		subTitles.Init ("intro.tex");
	else if (strstr (m [i], ENDMOVIE))
		subTitles.Init (ENDMOVIE ".tex");
	movieManager.Play (m [i], 1, 1, gameOpts->movies.bResize);
	SDL_ShowCursor (1);
	}
songManager.PlayCurrent (1);
}

//------------------------------------------------------------------------------

static void PlayMenuSong (void)
{
	int				h, i, j = 0;
	CStack<char*>	m (MAX_NUM_SONGS + 2);
	CFile				cf;
	char				szSongTitles [2][14] = {"- Descent 2 -", "- Descent 1 -"};

m.Push (szSongTitles [0]);
for (i = 0; i < gameData.songs.nTotalSongs; i++) {
	if (cf.Open (reinterpret_cast<char*> (gameData.songs.info [i].filename), gameFolders.szDataDir, "rb", i >= gameData.songs.nSongs [0])) {
		cf.Close ();
		if (i == gameData.songs.nSongs [0])
			m.Push (szSongTitles [1]);
		m.Push (gameData.songs.info [i].filename);
		}
	}
for (;;) {
	h = ListBox (TXT_SELECT_SONG, m);
	if (h < 0)
		return;
	if (!strstr (m [h], ".hmp"))
		continue;
	for (i = 0; i < gameData.songs.nTotalSongs; i++)
		if (gameData.songs.info [i].filename == m [h]) {
			songManager.Play (i, 0);
			return;
			}
	}
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
	CMenu	m (1);
	char	szLevel [10] = "";
	int	nLevel;

	m.AddInput (szLevel, sizeof (szLevel), NULL);
	m.Menu (NULL, "Enter level to load", NULL, NULL);
	nLevel = atoi (m [0].m_text);
	if (nLevel && (nLevel >= gameData.missions.nLastSecretLevel) && (nLevel <= gameData.missions.nLastLevel)) {
		paletteManager.DisableEffect ();
		StartNewGame (nLevel);
		}
	}
#endif
else if (nChoice == mainOpts.nMulti)
	MultiplayerMenu ();
else if (nChoice == mainOpts.nConfig) 
	ConfigMenu ();
else if (nChoice == mainOpts.nPilots) {
	gameStates.gfx.bOverride = 0;
	SelectPlayer ();
	}
else if (nChoice == mainOpts.nDemo) {
	char demoPath [FILENAME_LEN], demoFile [FILENAME_LEN];

	sprintf (demoPath, "%s%s*.dem", gameFolders.szDemoDir, *gameFolders.szDemoDir ? "/" : ""); 
	if (FileSelector (TXT_SELECT_DEMO, demoPath, demoFile, 1))
		NDStartPlayback (demoFile);
	}
else if (nChoice == mainOpts.nScores) {
	paletteManager.DisableEffect ();
	ScoresView (-1);
	}
else if (nChoice == mainOpts.nMovies)
	PlayMenuMovie ();
else if (nChoice == mainOpts.nSongs)
	PlayMenuSong ();
else if (nChoice == mainOpts.nCredits) {
	paletteManager.DisableEffect ();
	songManager.StopAll ();
	creditsManager.Show (NULL); 
	}
#ifdef EDITOR
else if (nChoice == mainOpts.nEditor) {
	SetFunctionMode (FMODE_EDITOR);
	InitCockpit ();
	}
#endif
else if (nChoice == mainOpts.nHelp) 
	DoShowHelp ();
else if (nChoice == mainOpts.nQuit) {
#ifdef EDITOR
	if (SafetyCheck ()) {
#endif
	paletteManager.DisableEffect ();
	SetFunctionMode (FMODE_EXIT);
#ifdef EDITOR
	}
#endif
	}
else if (nChoice == mainOpts.nOrder) 
	ShowOrderForm ();
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int QuitSaveLoadMenu (void)
{
	CMenu m (5);
	int	i, choice = 0, optQuit, optOptions, optLoad, optSave;

optQuit = m.AddMenu (TXT_QUIT_GAME, KEY_Q, HTX_QUIT_GAME);
optOptions = m.AddMenu (TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
optLoad = m.AddMenu (TXT_LOAD_GAME2, KEY_L, HTX_LOAD_GAME);
optSave = m.AddMenu (TXT_SAVE_GAME2, KEY_S, HTX_SAVE_GAME);
i = m.Menu (NULL, TXT_ABORT_GAME, NULL, &choice);
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
//eof
