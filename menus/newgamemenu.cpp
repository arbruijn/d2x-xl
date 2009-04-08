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
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
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

//------------------------------------------------------------------------------

int SelectAndLoadMission (int bMulti, int *bAnarchyOnly)
{
	int				i, j, nMissions, nDefaultMission, nNewMission = -1;
	CStack<char*>	msnNames (MAX_MISSIONS);
	CListBox			lb;

	static const char *menuTitles [4];

	menuTitles [0] = TXT_NEW_GAME;
	menuTitles [1] = TXT_NEW_D1GAME;
	menuTitles [2] = TXT_NEW_D2GAME;
	menuTitles [3] = TXT_NEW_GAME;

if (bAnarchyOnly)
	*bAnarchyOnly = 0;
do {
	msnNames.Reset ();
	nMissions = BuildMissionList (1, nNewMission);
	if (nMissions < 1)
		return -1;
	nDefaultMission = 0;
	for (i = 0; i < nMissions; i++) {
		msnNames.Push (gameData.missions.list [i].szMissionName);
		j = MsnHasGameVer (msnNames [i]) ? 4 : 0;
		if (!stricmp (msnNames [i] + j, gameConfig.szLastMission))
			nDefaultMission = i;
		}
	gameStates.app.nExtGameStatus = bMulti ? GAMESTAT_START_MULTIPLAYER_MISSION : GAMESTAT_SELECT_MISSION;
	gameOpts->app.nVersionFilter = NMCLAMP (gameOpts->app.nVersionFilter, 1, 3);
	nNewMission = lb.ListBox (bMulti ? TXT_MULTI_MISSION : menuTitles [gameOpts->app.nVersionFilter], msnNames, nDefaultMission);
	GameFlushInputs ();
	if (nNewMission == -1)
		return -1;      //abort!
	} while (!gameData.missions.list [nNewMission].nDescentVersion);
strcpy (gameConfig.szLastMission, msnNames [nNewMission]);
if (!LoadMission (nNewMission)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_MISSION_ERROR);
	return -1;
	}
gameStates.app.bD1Mission = (gameData.missions.list [nNewMission].nDescentVersion == 1);
gameData.missions.nLastMission = nNewMission;
if (bAnarchyOnly)
	*bAnarchyOnly = gameData.missions.list [nNewMission].bAnarchyOnly;
return nNewMission;
}

//------------------------------------------------------------------------------

int DifficultyMenu (void)
{
	int		i, choice = gameStates.app.nDifficultyLevel;
	CMenu		m (5);

for (i = 0; i < 5; i++)
	m.AddMenu ( MENU_DIFFICULTY_TEXT (i), 0, "");
i = m.Menu (NULL, TXT_DIFFICULTY_LEVEL, NULL, &choice);
if (i <= -1)
	return 0;
if (choice != gameStates.app.nDifficultyLevel) {       
	gameStates.app.nDifficultyLevel = choice;
	SavePlayerProfile ();
	}
return 1;
}

//------------------------------------------------------------------------------

void LegacyNewGameMenu (void)
{
	int				nNewLevel, nHighestPlayerLevel;
	int				nMissions;
	CStack<char*>	m (MAX_MISSIONS);
	int				i, choice = 0, nFolder = -1, nDefaultMission = 0;
	CListBox			lb;

	static int		nMission = -1;

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
hogFileManager.UseMission ("");
do {
	nMissions = BuildMissionList (0, nFolder);
	if (nMissions < 1)
		return;
	for (i = 0; i < nMissions; i++) {
		m.Push (gameData.missions.list [i].szMissionName);
		if (!stricmp (m [i], gameConfig.szLastMission))
			nDefaultMission = i;
		}
	nMission = lb.ListBox (menuTitles [gameOpts->app.nVersionFilter], m, nDefaultMission);
	GameFlushInputs ();
	if (nMission == -1)
		return;         //abort!
	nFolder = nMission;
	}
while (!gameData.missions.list [nMission].nDescentVersion);
strcpy (gameConfig.szLastMission, m [nMission]);
if (!LoadMission (nMission)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_ERROR_MSNFILE); 
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
	CMenu	m (2);
	char	szInfo [80];
	char	szNumber [10];

try_again:
	sprintf (szInfo, "%s %d", TXT_START_ANY_LEVEL, nHighestPlayerLevel);
	m.AddText (szInfo, 0);
	strcpy (szNumber, "1");
	m.AddInput (szNumber, 10, "");
	choice = m.Menu (NULL, TXT_SELECT_START_LEV);
	if ((choice == -1) || !m [1].m_text [0])
		return;
	nNewLevel = atoi (m [1].m_text);
	if ((nNewLevel <= 0) || (nNewLevel > nHighestPlayerLevel)) {
		m [0].SetText (const_cast<char*> (TXT_ENTER_TO_CONT));
		MsgBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		goto try_again;
	}
}

SavePlayerProfile ();
if (!DifficultyMenu ())
	return;
paletteManager.DisableEffect ();
if (!StartNewGame (nNewLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static int nOptDifficulty = -1;

int NewGameMenuCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

m = menu + nOptDifficulty;
v = m->m_value;
if (gameStates.app.nDifficultyLevel != v) {
	gameStates.app.nDifficultyLevel = 
	gameStates.app.nDifficultyLevel = v;
	gameData.bosses.InitGateIntervals ();
	sprintf (m->m_text, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	m->m_bRebuild = 1;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void NewGameMenu (void)
{
	CMenu				menu;
	int				optSelMsn, optMsnName, optLevelText, optLevel, optLaunch, optLoadout;
	int				nMission = gameData.missions.nLastMission, bMsnLoaded = 0;
	int				i, choice = 0;
	char				szDifficulty [50];
	char				szLevelText [32];
	char				szLevel [5];
#if DBG
	int				optLives;
	char				szLives [5];
#endif

	static int		nPlayerMaxLevel = 1;
	static int		nLevel = 0;

if (gameStates.app.bNostalgia) {
	LegacyNewGameMenu ();
	return;
	}
gameStates.app.bD1Mission = 0;
gameStates.app.bD1Data = 0;
gameOpts->app.nVersionFilter = 3;
SetDataVersion (-1);
if ((nMission < 0) || gameOpts->app.bSinglePlayer)
	gameFolders.szMsnSubDir [0] = '\0';
hogFileManager.UseMission ("");

for (;;) {
	menu.Destroy ();
	menu.Create (15);

	optSelMsn = menu.AddMenu (TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
	optMsnName = menu.AddText ((nMission < 0) ? TXT_NONE_SELECTED : gameData.missions.list [nMission].szMissionName, 0);
	if ((nMission >= 0) && (nPlayerMaxLevel > 1)) {
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, nPlayerMaxLevel);
		Assert (strlen (szLevelText) < 100);
		optLevelText = menu.AddText (szLevelText, 0); 
		menu [optLevelText].m_bRebuild = 1;
		sprintf (szLevel, "%d", nLevel);
		optLevel = menu.AddInput (szLevel, 4, HTX_MULTI_LEVEL);
		}
	else
		optLevel = -1;
#if DBG
	menu.AddText ("");
	menu.AddText ("Initial Lives:");
	sprintf (szLives, "%d", gameStates.gameplay.nInitialLives);
	optLives = menu.AddInput (szLives, 4);
#endif
	menu.AddText ("                              ", 0);
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	nOptDifficulty = menu.AddSlider (szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	menu.AddText ("", 0);
	optLoadout = menu.AddMenu (TXT_LOADOUT_OPTION, KEY_B, HTX_MULTI_LOADOUT);

	if (nMission >= 0) {
		menu.AddText ("", 0);
		optLaunch = menu.AddMenu (TXT_LAUNCH_GAME, KEY_L, "");
		menu [optLaunch].m_bCentered = 1;
		}
	else
		optLaunch = -1;

	i = menu.Menu (NULL, TXT_NEWGAME_MENUTITLE, NewGameMenuCallback, &choice);
	if (i < 0) {
		SetFunctionMode (FMODE_MENU);
		return;
		}
	if (choice == optLoadout)
		LoadoutOptionsMenu ();
	else if (choice == optSelMsn) {
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
		i = atoi (menu [optLevel].m_text);
#if DBG
		if (!i || (i < -gameData.missions.nSecretLevels) || (i > nPlayerMaxLevel))
#else
		if ((i <= 0) || (i > nPlayerMaxLevel))
#endif
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		else if (nLevel == i)
			break;
		else
			nLevel = i;
		}
	else if (nMission >= 0) {
		if ((optLevel > 0) && !(nLevel = atoi (menu [optLevel].m_text))) {
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
			nLevel = 1;
			}
		else
			break;
		}
#if DBG
	else {
		i = atoi (menu [optLives].m_text);
		if (i > 0)
			gameStates.gameplay.nInitialLives = min (i, 255);
		}
#endif
	}

i = menu [nOptDifficulty].m_value;
if (gameStates.app.nDifficultyLevel != i) {
	gameStates.app.nDifficultyLevel = i;
	gameData.bosses.InitGateIntervals ();
	}
SavePlayerProfile ();

paletteManager.DisableEffect ();
if (!bMsnLoaded)
	LoadMission (nMission);
if (!StartNewGame (nLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static inline int MultiChoice (int nType, int bJoin)
{
return *(reinterpret_cast<int*> (&multiOpts) + 2 * nType + bJoin);
}

//------------------------------------------------------------------------------

int ExecMultiMenuOption (int nChoice)
{
	int	bUDP = 0, bStart = 0;

tracker.m_bUse = 0;
if ((nChoice == multiOpts.nStartUdpTracker) ||(nChoice == multiOpts.nJoinUdpTracker)) {
	if (gameStates.app.bNostalgia > 1)
		return 0;
	tracker.m_bUse = 1;
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
if (bUDP && !(InitAutoNetGame () || NetworkGetIpAddr (bStart != 0)))
	return 0;
gameStates.multi.bServer = (nChoice & 1) == 0;
gameStates.app.bHaveExtraGameInfo [1] = gameStates.multi.bServer;
if (bUDP) {
	gameStates.multi.nGameType = UDP_GAME;
	IpxSetDriver (IPX_DRIVER_UDP); 
	if (nChoice == multiOpts.nStartUdpTracker) {
		int n = tracker.ActiveCount (1);
		if (n < -2) {
			if (n == -4)
				MsgBox (NULL, NULL, 1, TXT_OK, TXT_NO_TRACKERS);
			tracker.m_bUse = 0;
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

void MultiplayerMenu (void)
{
	CMenu	m;
	int	choice = 0, i, optCreate, optJoin = -1, optConn = -1, nConnections = 0;
	int	nOldGameMode;

if ((gameStates.app.bNostalgia < 2) && gameData.multiplayer.autoNG.bValid) {
	i = MultiChoice (gameData.multiplayer.autoNG.uConnect, !gameData.multiplayer.autoNG.bHost);
	if (i >= 0)
		ExecMultiMenuOption (i);
	}
else {
	do {
		nOldGameMode = gameData.app.nGameMode;
		m.Destroy ();
		m.Create (15);
		if (gameStates.app.bNostalgia < 2) {
			optCreate = m.AddMenu (TXT_CREATE_GAME, KEY_S, HTX_NETWORK_SERVER);
			optJoin = m.AddMenu (TXT_JOIN_GAME, KEY_J, HTX_NETWORK_CLIENT);
			m.AddText ("", 0);
			optConn = m.AddRadio (TXT_NGTYPE_IPX, 0, KEY_I, HTX_NETWORK_IPX);
			m.AddRadio (TXT_NGTYPE_UDP, 0, KEY_U, HTX_NETWORK_UDP);
			m.AddRadio (TXT_NGTYPE_TRACKER, 0, KEY_T, HTX_NETWORK_TRACKER);
			m.AddRadio (TXT_NGTYPE_MCAST4, 0, KEY_M, HTX_NETWORK_MCAST);
#ifdef KALINIX
			m.AddRadio (TXT_NGTYPE_KALI, 0, KEY_K, HTX_NETWORK_KALI);
#endif
			nConnections = m.ToS ();
			m [optConn + NMCLAMP (gameStates.multi.nConnection, 0, nConnections - optConn)].m_value = 1;
			}
		else {
#ifdef NATIVE_IPX
			multiOpts.nStartIpx = m.AddMenu (TXT_START_IPX_NET_GAME,  -1, HTX_NETWORK_IPX);
			multiOpts.nJoinIpx = m.AddMenu (TXT_JOIN_IPX_NET_GAME, -1, HTX_NETWORK_IPX);
#endif //NATIVE_IPX
			multiOpts.nStartMCast4 = m.AddMenu (TXT_MULTICAST_START, KEY_M, HTX_NETWORK_MCAST);
			multiOpts.nJoinMCast4 = m.AddMenu (TXT_MULTICAST_JOIN, KEY_N, HTX_NETWORK_MCAST);
#ifdef KALINIX
			multiOpts.nStartKali = m.AddMenu (TXT_KALI_START, KEY_K, HTX_NETWORK_KALI);
			multiOpts.nJoinKali = m.AddMenu (TXT_KALI_JOIN, KEY_I, HTX_NETWORK_KALI);
#endif // KALINIX
			if (gameStates.app.bNostalgia > 2)
				multiOpts.nSerial = m.AddMenu (TXT_MODEM_GAME2, KEY_G, HTX_NETWORK_MODEM);
			}
		i = m.Menu (NULL, TXT_MULTIPLAYER, NULL, &choice);
		if (i > -1) {      
			if (gameStates.app.bNostalgia > 1)
				i = choice;
			else {
				for (gameStates.multi.nConnection = 0; 
					  gameStates.multi.nConnection < nConnections; 
					  gameStates.multi.nConnection++)
					if (m [optConn + gameStates.multi.nConnection].m_value)
						break;
				i = MultiChoice (gameStates.multi.nConnection, choice == optJoin);
				}
			ExecMultiMenuOption (i);
			}
		if (nOldGameMode != gameData.app.nGameMode)
			break;          // leave menu
		} while (i > -1);
	}
}

//------------------------------------------------------------------------------
//eof
