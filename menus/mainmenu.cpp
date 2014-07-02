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
#include "descent.h"
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
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
#include "movie.h"
#include "gamepal.h"
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "rendermine.h"
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
#include "songs.h"

//------------------------------------------------------------------------------

// Function Prototypes added after LINTING
int32_t ExecMainMenuOption (CMenu& m, int32_t nChoice);
int32_t ExecMultiMenuOption (CMenu& m, int32_t nChoice);

//returns the number of demo files on the disk
int32_t NDCountDemos (void);

#if defined(_WIN32) || defined(__unix__)
int32_t CheckForUpdate (void);
#endif

// ------------------------------------------------------------------------

int32_t AutoDemoCallback (CMenu& m, int32_t& nLastKey, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	int32_t curtime;

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
		int32_t nDemos = NDCountDemos ();
		for (;;) {
			if ((RandShort () % (nDemos+1)) == 0) {
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
int32_t SetupMainMenu (CMenu& m)
{
m.Destroy ();
m.Create (25);
SetScreenMode (SCREEN_MENU);
m.AddText ("", "");
m.Top ()->m_bNoScroll = 1;
m.AddText ("", "");
m.Top ()->m_bNoScroll = 1;
m.AddText ("", "");
m.Top ()->m_bNoScroll = 1;
m.AddMenu ("new game", TXT_NEW_GAME1, KEY_N, HTX_MAIN_NEW);
if (!gameStates.app.bNostalgia)
	m.AddMenu ("new singleplayer game", TXT_NEW_SPGAME, KEY_I, HTX_MAIN_SINGLE);
m.AddMenu ("load game", TXT_LOAD_GAME, KEY_L, HTX_MAIN_LOAD);
m.AddMenu ("multiplayer game", TXT_MULTIPLAYER_, KEY_M, HTX_MAIN_MULTI);
m.AddMenu ("program settings", gameStates.app.bNostalgia ? TXT_OPTIONS_ : TXT_CONFIGURE, KEY_O, HTX_MAIN_CONF);
m.AddMenu ("choose pilot", TXT_CHANGE_PILOTS, KEY_P, HTX_MAIN_PILOT);
m.AddMenu ("view demo", TXT_VIEW_DEMO, KEY_D, HTX_MAIN_DEMO);
m.AddMenu ("view highscores", TXT_VIEW_SCORES, KEY_H, HTX_MAIN_SCORES);
m.AddMenu ("play movies", TXT_PLAY_MOVIES, KEY_V, HTX_MAIN_MOVIES);
if (!gameStates.app.bNostalgia)
	m.AddMenu ("play songs", TXT_PLAY_SONGS, KEY_S, HTX_MAIN_SONGS);
m.AddMenu ("view credits", TXT_CREDITS, KEY_C, HTX_MAIN_CREDITS);
#if defined(_WIN32) || defined(__unix__)
m.AddMenu ("check update", TXT_CHECK_FOR_UPDATE, KEY_U, HTX_CHECK_FOR_UPDATE);
#endif
m.AddMenu ("quit", TXT_QUIT, KEY_Q, HTX_MAIN_QUIT);
return m.ToS ();
}

//------------------------------------------------------------------------------
//returns number of item chosen
int32_t MainMenu (void) 
{
	CMenu	m;
	int32_t	i, nChoice = 0;

IpxClose ();
//paletteManager.Load (MENU_PALETTE, NULL, 0, 1, 0);		//get correct palette

if (!LOCALPLAYER.callsign [0]) {
	SelectPlayer ();
	return 0;
	}
if (gameData.multiplayer.autoNG.bValid > 0) {
	if (!MultiplayerMenu ())
		gameStates.app.nFunctionMode = FMODE_EXIT;
	return 0;
	}

PrintLog (1, "launching main menu\n");
do {
	SetupMainMenu (m); // may have to change, eg, maybe selected pilot and no save games.
	gameStates.input.keys.xLastPressTime = TimerGetFixedSeconds ();                // .. 20 seconds from now!
	if (nChoice < 0)
		nChoice = 0;
	gameStates.menus.bDrawCopyright = 1;
	gameStates.menus.nInMenu = 0;
	i = m.Menu ("", NULL, AutoDemoCallback, &nChoice, BG_TOPMENU, BG_STANDARD);
	if (gameStates.app.bNostalgia)
		gameOpts->app.nVersionFilter = 3;
	if (i > -1) {
		ExecMainMenuOption (m, nChoice);
		SavePlayerProfile ();
		}
} while (gameStates.app.nFunctionMode == FMODE_MENU);
PrintLog (-1);
if (gameStates.app.nFunctionMode == FMODE_GAME)
	paletteManager.DisableEffect ();
controls.FlushInput ();
return nChoice;
}

//------------------------------------------------------------------------------

static void PlayMenuMovie (void)
{
	int32_t				h, i, j;
	CStack<char*>	m;
	char*				ps;
	CListBox			lb;

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
i = lb.ListBox (TXT_SELECT_MOVIE, m);
if (i > -1) {
	SDL_ShowCursor (0);
	if (strstr (m [i], "intro"))
		subTitles.Init ("intro.tex");
	else if (strstr (m [i], ENDMOVIE))
		subTitles.Init (ENDMOVIE ".tex");
	movieManager.Play (m [i], 1, 1, gameOpts->movies.bResize);
	subTitles.Close ();
	SDL_ShowCursor (1);
	}
songManager.PlayCurrent (1);
}

//------------------------------------------------------------------------------

static void PlayMenuSong (void)
{
	int32_t				h, i;
	CStack<char*>	m (MAX_NUM_SONGS + 2);
	CFile				cf;
	char				szSongTitles [2][14] = {"- Descent 2 -", "- Descent 1 -"};
	CListBox			lb;

m.Push (szSongTitles [0]);
for (i = 0; i < songManager.TotalCount (); i++) {
	if (cf.Open (reinterpret_cast<char*> (songManager.SongData (i).filename), gameFolders.game.szData [0], "rb", i >= songManager.Count (0))) {
		cf.Close ();
		if (i == songManager.Count (0))
			m.Push (szSongTitles [1]);
		m.Push (songManager.SongData (i).filename);
		}
	}
for (;;) {
	h = lb.ListBox (TXT_SELECT_SONG, m);
	if (h < 0)
		return;
	if (!strstr (m [h], ".hmp"))
		continue;
	for (i = 0; i < songManager.TotalCount (); i++)
		if (songManager.SongData (i).filename == m [h]) {
			songManager.Play (i, 0);
			return;
			}
	}
}

//------------------------------------------------------------------------------

void ShowOrderForm (void);      // John didn't want this in inferno[HA] so I just externed it.

//returns flag, true means quit menu
int32_t ExecMainMenuOption (CMenu& m, int32_t nChoice) 
{
	CFileSelector	fs;

if (nChoice == m.IndexOf ("new game")) {
	gameOpts->app.bSinglePlayer = 0;
	NewGameMenu ();
	}
else if (nChoice == m.IndexOf ("new singleplayer game")) {
	gameOpts->app.bSinglePlayer = 1;
	NewGameMenu ();
	}
else if (nChoice == m.IndexOf ("load game")) {
	if (!saveGameManager.Load (0, 0, 0, NULL))
		SetFunctionMode (FMODE_MENU);
	}
#if 0 //DBG
else if (nChoice == m.IndexOf ("load direct")) {
	CMenu	m (1);
	char	szLevel [10] = "";
	int32_t	nLevel;

	m.AddInput ("level number", szLevel, sizeof (szLevel), NULL);
	m.Menu (NULL, "Enter level to load", NULL, NULL);
	nLevel = atoi (m [0].m_text);
	if (nLevel && (nLevel >= missionManager.nLastSecretLevel) && (nLevel <= missionManager.nLastLevel)) {
		paletteManager.DisableEffect ();
		StartNewGame (nLevel);
		}
	}
#endif
else if (nChoice == m.IndexOf ("multiplayer game"))
		MultiplayerMenu ();
else if (nChoice == m.IndexOf ("program settings")) 
	ConfigMenu ();
else if (nChoice == m.IndexOf ("choose pilot"))
	SelectPlayer ();
else if (nChoice == m.IndexOf ("view demo")) {
	char demoPath [FILENAME_LEN], demoFile [FILENAME_LEN];

	sprintf (demoPath, "%s*.dem", gameFolders.user.szDemos); 
	if (fs.FileSelector (TXT_SELECT_DEMO, demoPath, demoFile, 1))
		NDStartPlayback (demoFile);
	}
else if (nChoice == m.IndexOf ("view highscores")) {
	paletteManager.DisableEffect ();
	scoreManager.Show (-1);
	}
else if (nChoice == m.IndexOf ("play movies"))
	PlayMenuMovie ();
else if (nChoice == m.IndexOf ("play songs"))
	PlayMenuSong ();
else if (nChoice == m.IndexOf ("view credits")) {
	paletteManager.DisableEffect ();
	songManager.StopAll ();
	creditsRenderer.Show (NULL); 
	}
//else if (nChoice == m.IndexOf ("show help")) 
//	ShowHelp ();
//else if (nChoice == mainOpts.nOrder) 
//	ShowOrderForm ();
#if defined(_WIN32) || defined(__unix__)
else if (nChoice == m.IndexOf ("check update"))
	CheckForUpdate ();
#endif
else if (nChoice == m.IndexOf ("quit")) {
	paletteManager.DisableEffect ();
	SetFunctionMode (FMODE_EXIT);
	}
else
	return 0;
return 1;
}

//------------------------------------------------------------------------------

int32_t QuitSaveLoadMenu (void)
{
	CMenu m (5);
	int32_t	i;

	int32_t choice = 0;

m.AddMenu ("quit", TXT_QUIT_GAME, KEY_Q, HTX_QUIT_GAME);
m.AddMenu ("settings", TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
m.AddMenu ("load game", TXT_LOAD_GAME2, KEY_L, HTX_LOAD_GAME);
m.AddMenu ("save game", TXT_SAVE_GAME2, KEY_S, HTX_SAVE_GAME);
i = m.Menu (NULL, TXT_ABORT_GAME, NULL, &choice);
if (i == m.IndexOf ("quit"))
	return 0;
else if (i == m.IndexOf ("settings"))
	ConfigMenu ();
else if (i == m.IndexOf ("load game"))
	saveGameManager.Load (1, 0, 0, NULL);
else if (i == m.IndexOf ("save game"))
	saveGameManager.Save (0, 0, 0, NULL);
return 1;
}

//------------------------------------------------------------------------------
//eof
