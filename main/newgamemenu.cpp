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
#include "menubackground.h"
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
	CMenuItem m [4];
	char szInfo [80];
	char szNumber [10];
	int nItems;

try_again:
	sprintf (szInfo, "%s %d", TXT_START_ANY_LEVEL, nHighestPlayerLevel);

	memset (m, 0, sizeof (m));
	m.AddText (0, szInfo, 0);
	m.AddInput (1, szNumber, 10, "");
	nItems = 2;

	strcpy (szNumber, "1");
	choice = ExecMenu (NULL, TXT_SELECT_START_LEV, nItems, m, NULL, NULL);
	if ((choice == -1) || !m [1].text [0])
		return;
	nNewLevel = atoi (m [1].text);
	if ((nNewLevel <= 0) || (nNewLevel > nHighestPlayerLevel)) {
		m [0].text = const_cast<char*> (TXT_ENTER_TO_CONT);
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		goto try_again;
	}
}

WritePlayerFile ();
if (!DifficultyMenu ())
	return;
paletteManager.DisableEffect ();
if (!StartNewGame (nNewLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static int nOptVerFilter = -1;

int NewGameMenuCallback (CMenu& m, int * key, int nCurItem)
{
	CMenuItem	*m;
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

void NewGameMenu (void)
{
	CMenu				m (15);
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

	m.AddMenu (nOptions, TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
	optSelMsn = nOptions++;
	m.AddText (nOptions, (nMission < 0) ? TXT_NONE_SELECTED : gameData.missions.list [nMission].szMissionName, 0);
	optMsnName = nOptions++;
	if ((nMission >= 0) && (nPlayerMaxLevel > 1)) {
#if 0
		m.AddText (nOptions, "", 0);
		nOptions++;
#endif
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, nPlayerMaxLevel);
		Assert (strlen (szLevelText) < 32);
		m.AddText (nOptions, szLevelText, 0); 
		m [nOptions].rebuild = 1;
		optLevelText = nOptions++;
		sprintf (szLevel, "%d", nLevel);
		m.AddInput (nOptions, szLevel, 4, HTX_MULTI_LEVEL);
		optLevel = nOptions++;
		}
	else
		optLevel = -1;
	m.AddText (nOptions, "                              ", 0);
	nOptions++;
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	m.AddSlider (nOptions, szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	gplayOpts.nDifficulty = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddRadio (nOptions, TXT_PLAY_D1MISSIONS, 0, KEY_1, 1, HTX_LEVEL_VERSION_FILTER);
	nOptVerFilter = nOptions++;
	m.AddRadio (nOptions, TXT_PLAY_D2MISSIONS, 0, KEY_2, 1, HTX_LEVEL_VERSION_FILTER);
	nOptions++;
	m.AddRadio (nOptions, TXT_PLAY_ALL_MISSIONS, 0, KEY_A, 1, HTX_LEVEL_VERSION_FILTER);
	nOptions++;
	m [nOptVerFilter + gameOpts->app.nVersionFilter - 1].value = 1;
	if (nMission >= 0) {
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddMenu (nOptions, TXT_LAUNCH_GAME, KEY_L, "");
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
paletteManager.DisableEffect ();
if (!bMsnLoaded)
	LoadMission (nMission);
if (!StartNewGame (nLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------
//eof
