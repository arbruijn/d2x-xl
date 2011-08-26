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
	nMissions = missionManager.BuildList (1, nNewMission);
	if (nMissions < 1)
		return -1;
	nDefaultMission = 0;
	for (i = 0; i < nMissions; i++) {
		msnNames.Push (missionManager [i].szMissionName);
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
	} while (!missionManager [nNewMission].nDescentVersion);
strncpy (gameConfig.szLastMission, msnNames [nNewMission] + (MsnHasGameVer (msnNames [nNewMission]) ? 4 : 0), sizeof (gameConfig.szLastMission));
gameConfig.szLastMission [sizeof (gameConfig.szLastMission) - 1] = '\0';
if (!missionManager.Load (nNewMission)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_MISSION_ERROR);
	return -1;
	}
gameStates.app.bD1Mission = (missionManager [nNewMission].nDescentVersion == 1);
missionManager.nLastMission = nNewMission;
if (bAnarchyOnly)
	*bAnarchyOnly = missionManager [nNewMission].bAnarchyOnly;
return nNewMission;
}

//------------------------------------------------------------------------------

int DifficultyMenu (void)
{
	int		i, choice = gameStates.app.nDifficultyLevel;
	CMenu		m (5);

for (i = 0; i < 5; i++)
	m.AddMenu ("difficulty", MENU_DIFFICULTY_TEXT (i), 0, "");
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
	nMissions = missionManager.BuildList (0, nFolder);
	if (nMissions < 1)
		return;
	for (i = 0; i < nMissions; i++) {
		m.Push (missionManager [i].szMissionName);
		if (!stricmp (m [i], gameConfig.szLastMission))
			nDefaultMission = i;
		}
	nMission = lb.ListBox (menuTitles [gameOpts->app.nVersionFilter], m, nDefaultMission);
	GameFlushInputs ();
	if (nMission == -1)
		return;         //abort!
	nFolder = nMission;
	}
while (!missionManager [nMission].nDescentVersion);
strcpy (gameConfig.szLastMission, m [nMission]);
if (!missionManager.Load (nMission)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_ERROR_MSNFILE); 
	return;
}
gameStates.app.bD1Mission = (missionManager [nMission].nDescentVersion == 1);
missionManager.nLastMission = nMission;
nNewLevel = 1;

PrintLog ("   getting highest level allowed to play\n");
nHighestPlayerLevel = GetHighestLevel ();

if (nHighestPlayerLevel > missionManager.nLastLevel)
	nHighestPlayerLevel = missionManager.nLastLevel;

if (nHighestPlayerLevel > 1) {
	CMenu	m (2);
	char	szInfo [80];
	char	szNumber [10];

	for (;;) {
		sprintf (szInfo, "%s %d", TXT_START_ANY_LEVEL, nHighestPlayerLevel);
		m.AddText ("", szInfo, 0);
		strcpy (szNumber, "1");
		m.AddInput ("start level", szNumber, 10, "");
		choice = m.Menu (NULL, TXT_SELECT_START_LEV);
		if ((choice == -1) || !*m.Text ("start level"))
			return;
		nNewLevel = m.ToInt ("start level");
		if ((nNewLevel > 0) && (nNewLevel <= nHighestPlayerLevel)) 
			break;
		m [0].SetText (const_cast<char*> (TXT_ENTER_TO_CONT));
		MsgBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
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

int NewGameMenuCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

if ((m = menu ["difficulty"])) {
	v = m->Value ();
	if (gameStates.app.nDifficultyLevel != v) {
		gameStates.app.nDifficultyLevel = v;
		gameData.bosses.InitGateIntervals ();
		sprintf (m->Text (), TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
		m->Rebuild ();
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void NewGameMenu (void)
{
	CMenu				m;
	int				nMission = missionManager.nLastMission, bMsnLoaded = 0;
	int				i, choice = 0, bBuiltIn, bEnableMod = gameOpts->app.bEnableMods;
	char				szDifficulty [50];
	char				szLevelText [32];
	char				szLevel [5];
#if DBG
	int				optLives;
	char				szLives [5];
#endif
	int				optShip = -1;

	static int		nPlayerMaxLevel = 1;
	static int		nLevel = 1;

if (gameStates.app.bNostalgia) {
	LegacyNewGameMenu ();
	return;
	}
gameStates.app.bD1Mission = 0;
gameStates.app.bD1Data = 0;
gameOpts->app.nVersionFilter = 3;
SetDataVersion (-1);
if (nMission < 0)
	gameFolders.szMsnSubDir [0] = '\0';
else if (gameOpts->app.bSinglePlayer) {
	if (!strstr (gameFolders.szMsnSubDir, "single/"))
		nMission = -1;
	gameFolders.szMsnSubDir [0] = '\0';
	}
if (nMission >= 0) {
	nPlayerMaxLevel = GetHighestLevel ();
	if (nPlayerMaxLevel > missionManager.nLastLevel)
		nPlayerMaxLevel = missionManager.nLastLevel;
	}
hogFileManager.UseMission ("");

for (;;) {
	m.Destroy ();
	m.Create (15);

	m.AddMenu ("mission selector", TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
	m.AddText ("mission name", (nMission < 0) ? TXT_NONE_SELECTED : missionManager [nMission].szMissionName, 0);
	if ((nMission >= 0) && (nPlayerMaxLevel > 1)) {
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, nPlayerMaxLevel);
		Assert (strlen (szLevelText) < 100);
		m.AddText ("level text", szLevelText, 0); 
		m ["level text"]->Rebuild ();
		sprintf (szLevel, "%d", nLevel);
		m.AddInput ("level number", szLevel, 4, HTX_MULTI_LEVEL);
		}

	if (nMission >= 0) {
		bBuiltIn = missionManager.IsBuiltIn (missionManager [nMission].szMissionName);
		gameOpts->app.bEnableMods = 1;
		MakeModFolders (bBuiltIn ? hogFileManager.m_files.MsnHogFiles.szName : missionManager [nMission].filename);
		gameOpts->app.bEnableMods = bEnableMod;
		if (gameStates.app.bHaveMod == bBuiltIn) {
			m.AddText ("", "");
			m.AddCheck ("enable mods", TXT_ENABLE_MODS, gameOpts->app.bEnableMods, KEY_O, HTX_ENABLE_MODS);
			}
		}

#if DBG
	m.AddText ("", "");
	m.AddText ("", "Initial Lives:");
	sprintf (szLives, "%d", gameStates.gameplay.nInitialLives);
	optLives = m.AddInput ("initial lives", szLives, 4);
#endif

	m.AddText ("", "                              ", 0);
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	m.AddSlider ("difficulty", szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	AddShipSelection (m, optShip);
	m.AddText ("", "");
	m.AddMenu ("loadout", TXT_LOADOUT_OPTION, KEY_B, HTX_MULTI_LOADOUT);

	if (nMission >= 0) {
		m.AddText ("", "");
		m.AddMenu ("launch game", TXT_LAUNCH_GAME, KEY_L, "");
		m ["launch game"]->m_bCentered = 1;
		}

	i = m.Menu (NULL, TXT_NEWGAME_MENUTITLE, NewGameMenuCallback, &choice);

	if (i < 0) {
		SetFunctionMode (FMODE_MENU);
		return;
		}
	GET_VAL (gameOpts->app.bEnableMods, "use mod");
	if (choice == m.IndexOf ("loadout"))
		LoadoutOptionsMenu ();
	else if (choice == m.IndexOf ("mission selector")) {
		i = SelectAndLoadMission (0, NULL);
		if (i >= 0) {
			bMsnLoaded = 1;
			nMission = i;
			nLevel = 1;
			PrintLog ("   getting highest level allowed to play\n");
			nPlayerMaxLevel = GetHighestLevel ();
			if (nPlayerMaxLevel > missionManager.nLastLevel)
				nPlayerMaxLevel = missionManager.nLastLevel;
			}
		}
	else if (choice == m.IndexOf ("level number")) {
		i = m.ToInt ("level number");
#if DBG
		if (!i || (i < -missionManager.nSecretLevels) || (i > nPlayerMaxLevel) || (i > missionManager.nLastLevel))
#else
		if ((i <= 0) || (i > nPlayerMaxLevel) || (i > missionManager.nLastLevel))
#endif
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
		else if (nLevel == i)
			break;
		else
			nLevel = i;
		}
	else if (nMission >= 0) {
		if (m.Available ("level number") && !(nLevel = m.ToInt ("level number"))) {
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
			nLevel = 1;
			}
		else
			break;
		}
#if DBG
	else {
		i = m.ToInt ("initial lives");
		if (i > 0)
			gameStates.gameplay.nInitialLives = min (i, 255);
		}
#endif
	}

i = m.Value ("difficulty");
if (gameStates.app.nDifficultyLevel != i) {
	gameStates.app.nDifficultyLevel = i;
	gameData.bosses.InitGateIntervals ();
	}
GetShipSelection (m, optShip);
SavePlayerProfile ();

paletteManager.DisableEffect ();
if (!bMsnLoaded)
	missionManager.Load (nMission);
if (!StartNewGame (nLevel))
	SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

static inline int MultiChoice (int nType, int bJoin)
{
return *(reinterpret_cast<int*> (&multiOpts) + 2 * nType + bJoin);
}

//------------------------------------------------------------------------------

int ExecMultiMenuOption (CMenu& m, int nChoice)
{
	int	bUDP = 0, bStart = 0;

tracker.m_bUse = 0;
if ((nChoice == multiOpts.nStartUdpTracker) ||(nChoice == multiOpts.nJoinUdpTracker)) {
	if (!DBG && gameStates.app.bNostalgia > 1)
		return 0;
	tracker.m_bUse = 1;
	bUDP = 1;
	bStart = (nChoice == multiOpts.nStartUdpTracker);
	}
else if ((nChoice == multiOpts.nStartUdp) || (nChoice == multiOpts.nJoinUdp)) {
	if (!DBG && gameStates.app.bNostalgia > 1)
		return 0;
	bUDP = 1;
	bStart = (nChoice == multiOpts.nStartUdp);
	}
else if ((nChoice == multiOpts.nStartIpx) || (nChoice == multiOpts.nStartKali) || (nChoice == multiOpts.nStartMCast4))
	bStart = 1;
else if ((nChoice != multiOpts.nJoinIpx) && (nChoice != multiOpts.nJoinKali) && (nChoice != multiOpts.nJoinMCast4))
	return 0;
gameOpts->app.bSinglePlayer = 0;
missionManager.Load (missionManager.nLastMission);
gameStates.multi.bServer [0] = 
gameStates.multi.bServer [1] = (nChoice & 1) == 0;
gameStates.app.bHaveExtraGameInfo [1] = gameStates.multi.bServer [0];
if (bUDP) {
	if (!(InitAutoNetGame () || NetworkGetIpAddr (bStart != 0)))
		return 0;
	gameStates.multi.nGameType = UDP_GAME;
	IpxSetDriver (IPX_DRIVER_UDP); 
	if (nChoice == multiOpts.nStartUdpTracker) {
		PrintLog ("   Looking for active trackers\n");
		int n = tracker.ActiveCount (1);
		if (n < -2) {
			if (n == -4)
				MsgBox (NULL, NULL, 1, TXT_OK, TXT_NO_TRACKERS);
			tracker.m_bUse = 0;
			return 0;
			}
		}
	}
else if ((nChoice == m.IndexOf ("start ipx game")) || (nChoice == m.IndexOf ("join ipx game"))) {
	gameStates.multi.nGameType = IPX_GAME;
	IpxSetDriver (IPX_DRIVER_IPX); 
	}
else if ((nChoice == m.IndexOf ("start kali game")) || (nChoice == m.IndexOf ("join kali game"))) {
	gameStates.multi.nGameType = IPX_GAME;
	IpxSetDriver (IPX_DRIVER_KALI); 
	}
else if ((nChoice == m.IndexOf ("start mcast4 game")) || (nChoice == m.IndexOf ("join mcast4 game"))) {
#if 0 //DBG
	NetworkGetIpAddr (bStart != 0, false);
#endif
	gameStates.multi.nGameType = IPX_GAME;
	IpxSetDriver (IPX_DRIVER_MCAST4); 
	}
if (bStart ? NetworkStartGame () : NetworkBrowseGames ()) {
	if (gameData.multiplayer.autoNG.bValid > 0)
		gameData.multiplayer.autoNG.bValid = -1;
	return 1;
	}
IpxClose ();
gameData.multiplayer.autoNG.bValid = 0;
return 0;
}

//------------------------------------------------------------------------------

int MultiplayerMenu (void)
{
	CMenu	m;
	int	choice = 0, i, nConnections = 0;
	int	nOldGameMode;

if ((gameStates.app.bNostalgia < 2) && (gameData.multiplayer.autoNG.bValid > 0)) {
	i = MultiChoice (gameData.multiplayer.autoNG.uConnect, !gameData.multiplayer.autoNG.bHost);
	if (i >= 0)
		return ExecMultiMenuOption (m, i);
	}

do {
	nOldGameMode = gameData.app.nGameMode;
	m.Destroy ();
	m.Create (15);
	if (DBG || gameStates.app.bNostalgia < 2) {
		m.AddMenu ("create game", TXT_CREATE_GAME, KEY_S, HTX_NETWORK_SERVER);
		m.AddMenu ("join game", TXT_JOIN_GAME, KEY_J, HTX_NETWORK_CLIENT);
		m.AddText ("", "");
		m.AddRadio ("ipx game", TXT_NGTYPE_IPX, 0, KEY_I, HTX_NETWORK_IPX);
		m.AddRadio ("udp game", TXT_NGTYPE_UDP, 0, KEY_U, HTX_NETWORK_UDP);
		m.AddRadio ("tracker game", TXT_NGTYPE_TRACKER, 0, KEY_T, HTX_NETWORK_TRACKER);
		m.AddRadio ("multicast game", TXT_NGTYPE_MCAST4, 0, KEY_M, HTX_NETWORK_MCAST);
#ifdef KALINIX
		m.AddRadio ("kali game", TXT_NGTYPE_KALI, 0, KEY_K, HTX_NETWORK_KALI);
#endif
		nConnections = m.ToS ();
		m [m.IndexOf ("ipx game") + NMCLAMP (gameStates.multi.nConnection, 0, nConnections - m.IndexOf ("ipx game"))].Value () = 1;
		}
	else {
#ifdef NATIVE_IPX
		m.AddMenu ("start ipx game", TXT_START_IPX_NET_GAME,  -1, HTX_NETWORK_IPX);
		m.AddMenu ("join ipx game", TXT_JOIN_IPX_NET_GAME, -1, HTX_NETWORK_IPX);
#endif //NATIVE_IPX
		m.AddMenu ("start mcast4 game", TXT_MULTICAST_START, KEY_M, HTX_NETWORK_MCAST);
		m.AddMenu ("join mcast4 game", TXT_MULTICAST_JOIN, KEY_N, HTX_NETWORK_MCAST);
#ifdef KALINIX
		m.AddMenu ("start kali game", TXT_KALI_START, KEY_K, HTX_NETWORK_KALI);
		m.AddMenu ("join kali game", TXT_KALI_JOIN, KEY_I, HTX_NETWORK_KALI);
#endif // KALINIX
		if (gameStates.app.bNostalgia > 2)
			multiOpts.nSerial = m.AddMenu ("serial game", TXT_MODEM_GAME2, KEY_G, HTX_NETWORK_MODEM);
		}
	i = m.Menu (NULL, TXT_MULTIPLAYER, NULL, &choice);
	if (i > -1) {      
		if (!DBG && gameStates.app.bNostalgia > 1)
			i = choice;
		else {
			for (gameStates.multi.nConnection = 0; 
				  gameStates.multi.nConnection < nConnections; 
				  gameStates.multi.nConnection++)
				if (m [m.IndexOf ("ipx game") + gameStates.multi.nConnection].Value ())
					break;
			i = MultiChoice (gameStates.multi.nConnection, choice == m.IndexOf ("join game"));
			}
		ExecMultiMenuOption (m, i);
		}
	if (nOldGameMode != gameData.app.nGameMode)
		break;          // leave menu
	} while (i > -1);
return 0;
}

//------------------------------------------------------------------------------
//eof
