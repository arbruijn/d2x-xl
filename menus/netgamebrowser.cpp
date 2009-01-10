#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#define PATCH12

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "inferno.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "menu.h"
#include "key.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "text.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "autodl.h"
#include "tracker.h"
#include "gamefont.h"
#include "menubackground.h"
#include "netmenu.h"

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

char szGameList [MAX_ACTIVE_NETGAMES + 5][100];

//------------------------------------------------------------------------------

void InitNetgameMenuOption (CMenu& menu, int j)
{
CMenuItem* m = menu + j;
if (!m->m_text) {
	m->m_text = szGameList [j];
	m->m_nType = NM_TYPE_MENU;
	}
sprintf (m->m_text, "%2d.                                                     ", j - 1 - gameStates.multi.bUseTracker);
m->m_value = 0;
m->m_bRebuild = 1;
}

//------------------------------------------------------------------------------

void InitNetgameMenu (CMenu& menu, int i)
{
	int j;

for (j = i + 2 + gameStates.multi.bUseTracker; i < MAX_ACTIVE_NETGAMES; i++, j++)
	InitNetgameMenuOption (menu, j);
}

//------------------------------------------------------------------------------

extern int nTabs [];

char *PruneText (char *pszDest, char *pszSrc, int nSize, int nPos, int nVersion)
{
	int		lDots, lMax, l, tx, ty, ta;
	char*		psz;
	CFont*	curFont = CCanvas::Current ()->Font ();

if (gameOpts->menus.bShowLevelVersion && (nVersion >= 0)) {
	if (nVersion)
		sprintf (pszDest, "[%d] ", nVersion);
	else
		strcpy (pszDest, "[?] ");
	strncat (pszDest + 4, pszSrc, nSize - 4);
	}
else
	strncpy (pszDest, pszSrc, nSize);
pszDest [nSize - 1] = '\0';
if ((psz = strchr (pszDest, '\t')))
	*psz = '\0';
fontManager.SetCurrent (SMALL_FONT);
fontManager.Current ()->StringSize ("... ", lDots, ty, ta);
fontManager.Current ()->StringSize (pszDest, tx, ty, ta);
l = (int) strlen (pszDest);
lMax = LHX (nTabs [nPos]) - LHX (nTabs [nPos - 1]);
if (tx > lMax) {
	lMax -= lDots;
	do {
		pszDest [--l] = '\0';
		fontManager.Current ()->StringSize (pszDest, tx, ty, ta);
	} while (tx > lMax);
	strcat (pszDest, "...");
	}
fontManager.SetCurrent (curFont); 
return pszDest;
}

//------------------------------------------------------------------------------

const char *szModeLetters []  = 
 {"ANRCHY", 
	 "TEAM", 
	 "ROBO", 
	 "COOP", 
	 "FLAG", 
	 "HOARD", 
	 "TMHOARD", 
	 "ENTROPY",
	 "MONSTER"};

int NetworkJoinPoll (CMenu& menu, int& key, int nCurItem)
{
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int	t = SDL_GetTicks ();
	int	i, h = 2 + gameStates.multi.bUseTracker, osocket, nJoinStatus, bPlaySound = 0;
	const char	*psz;
	char	szOption [200];
	char	szTrackers [100];

if (gameStates.multi.bUseTracker) {
	i = ActiveTrackerCount (0);
	menu [1].m_color = ORANGE_RGBA;
	sprintf (szTrackers, TXT_TRACKERS_FOUND, i, (i == 1) ? "" : "s");
	if (strcmp (menu [1].m_text, szTrackers)) {
		strcpy (menu [1].m_text, szTrackers);
//			menu [1].x = (short) 0x8000;
		menu [1].m_bRebuild = 1;
		}
	}

if ((gameStates.multi.nGameType >= IPX_GAME) && networkData.bAllowSocketChanges) {
	osocket = networkData.nSocket;
	if (key == KEY_PAGEDOWN) { 
		networkData.nSocket--; 
		key = 0; 
		}
	else if (key == KEY_PAGEUP) { 
		networkData.nSocket++; 
		key = 0; 
		}
	if (networkData.nSocket > 99)
		networkData.nSocket = -99;
	else if (networkData.nSocket < -99)
		networkData.nSocket = 99;
	if (networkData.nSocket + IPX_DEFAULT_SOCKET > 0x8000)
		networkData.nSocket = 0x8000 - IPX_DEFAULT_SOCKET;
	if (networkData.nSocket + IPX_DEFAULT_SOCKET < 0)
		networkData.nSocket = IPX_DEFAULT_SOCKET;
	if (networkData.nSocket != osocket) {
		sprintf (menu [0].m_text, TXT_CURR_SOCK, 
					 (gameStates.multi.nGameType == IPX_GAME) ? "IPX" : "UDP", 
					 networkData.nSocket);
		menu [0].m_bRebuild = 1;
#if 1			
		console.printf (0, TXT_CHANGE_SOCK, networkData.nSocket);
#endif
		NetworkListen ();
		IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
		RestartNetSearching (menu);
		NetworkSendGameListRequest ();
		return nCurItem;
		}
	}
	// send a request for game info every 3 seconds
#if DBG
if (!networkData.nActiveGames)
#endif
	if (gameStates.multi.nGameType >= IPX_GAME) {
		if (t > t1 + 3000) {
			if (!NetworkSendGameListRequest ())
				return nCurItem;
			t1 = t;
			}
		}
NetworkListen ();
DeleteTimedOutNetGames ();
if (networkData.bGamesChanged || (networkData.nActiveGames != networkData.nLastActiveGames)) {
	networkData.bGamesChanged = 0;
	networkData.nLastActiveGames = networkData.nActiveGames;
#if 1			
	console.printf (CON_DBG, "Found %d netgames.\n", networkData.nActiveGames);
#endif
	// Copy the active games data into the menu options
	for (i = 0; i < networkData.nActiveGames; i++, h++) {
			int gameStatus = activeNetGames [i].gameStatus;
			int nplayers = 0;
			char szLevelName [20], szMissionName [50], szGameName [50];
			int nLevelVersion = gameOpts->menus.bShowLevelVersion ? FindMissionByName (activeNetGames [i].szMissionName, -1) : -1;

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		PruneText (szMissionName, activeNetGames [i].szMissionTitle, sizeof (szMissionName), 4, nLevelVersion);
		PruneText (szGameName, activeNetGames [i].szGameName, sizeof (szGameName), 1, -1);
		nplayers = activeNetGames [i].nConnected;
		if (activeNetGames [i].nLevel < 0)
			sprintf (szLevelName, "S%d", -activeNetGames [i].nLevel);
		else
			sprintf (szLevelName, "%d", activeNetGames [i].nLevel);
		if (gameStatus == NETSTAT_STARTING)
			psz = "Forming";
		else if (gameStatus == NETSTAT_PLAYING) {
			nJoinStatus = CanJoinNetgame (activeNetGames + i, NULL);

			if (nJoinStatus == 1)
				psz = "Open";
			else if (nJoinStatus == 2)
				psz = "Full";
			else if (nJoinStatus == 3)
				psz = "Restrict";
			else
				psz = "Closed";
			}
		else 
			psz = "Between";
		sprintf (szOption, "%2d.\t%s\t%s\t%d/%d\t%s\t%s\t%s", 
					i + 1, szGameName, szModeLetters [activeNetGames [i].gameMode], nplayers, 
					activeNetGames [i].nMaxPlayers, szMissionName, szLevelName, psz);
		Assert (strlen (szOption) < 100);
		if (strcmp (szOption, menu [h].m_text)) {
			memcpy (menu [h].m_text, szOption, 100);
			menu [h].m_bRebuild = 1;
			menu [h].m_bUnavailable = (nLevelVersion == 0);
			bPlaySound = 1;
			}
		}
	for (i = 3 + networkData.nActiveGames; i < MAX_ACTIVE_NETGAMES; i++, h++)
		InitNetgameMenuOption (menu, h);
	}
#if 0
else
	for (i = 3; i < networkData.nActiveGames; i++, h++)
		menu [h].m_value = SDL_GetTicks ();
for (i = 3 + networkData.nActiveGames; i < MAX_ACTIVE_NETGAMES; i++, h++)
	if (menu [h].m_value && (t - menu [h].m_value > 10000)) 
	 {
		InitNetgameMenuOption (menu, h);
		bPlaySound = 1;
		}
#endif
if (bPlaySound)
	audio.PlaySound (SOUND_HUD_MESSAGE);
return nCurItem;
}


//------------------------------------------------------------------------------

int NetworkBrowseGames (void)
{
	CMenu	m (MAX_ACTIVE_NETGAMES + 5);
	int	choice, i, bAutoRun = gameData.multiplayer.autoNG.bValid;
	char	callsign [CALLSIGN_LEN+1];

//PrintLog ("launching netgame browser\n");
memcpy (callsign, LOCALPLAYER.callsign, sizeof (callsign));
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!networkData.bActive) {
		MsgBox (NULL, NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return 0;
		}
	}
//PrintLog ("   NetworkInit\n");
NetworkInit ();
gameData.multiplayer.nPlayers = 0;
setjmp (gameExitPoint);
networkData.nJoining = 0; 
networkData.nJoinState = 0;
networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
//PrintLog ("   NetworkFlush\n");
NetworkFlush ();
//PrintLog ("   NetworkListen\n");
NetworkListen ();  // Throw out old info
//PrintLog ("   NetworkSendGameListRequest\n");
NetworkSendGameListRequest (); // broadcast a request for lists
networkData.nActiveGames = 0;
networkData.nLastActiveGames = 0;
memset (activeNetGames, 0, sizeof (activeNetGames));
memset (activeNetPlayers, 0, sizeof (activeNetPlayers));
if (!bAutoRun) {
	fontManager.SetColorRGBi (RGBA_PAL (15, 15, 23), 1, 0, 0);
	m.AddText (szGameList [0]);
	m.Top ()->m_bNoScroll = 1;
	m.Top ()->m_x = (short) 0x8000;	//centered
	if (gameStates.multi.nGameType >= IPX_GAME) {
		if (networkData.bAllowSocketChanges)
			sprintf (m.Top ()->m_text, TXT_CURR_SOCK, (gameStates.multi.nGameType == IPX_GAME) ? "IPX" : "UDP", networkData.nSocket);
		else
			*m.Top ()->m_text = '\0';
		}
	i = 1;
	if (gameStates.multi.bUseTracker) {
		m.AddText (szGameList [i]);
		strcpy (m.Top ()->m_text, TXT_0TRACKERS);
		m.Top ()->m_x = (short) 0x8000;
		m.Top ()->m_bNoScroll = 1;
		}
	m.AddText (szGameList [i]);
	strcpy (m.Top ()->m_text, TXT_GAME_BROWSER);
	m.Top ()->m_bNoScroll = 1;
	InitNetgameMenu (m, 0);
	}
networkData.bGamesChanged = 1;    

doMenu:

gameStates.app.nExtGameStatus = GAMESTAT_JOIN_NETGAME;
if (bAutoRun) {
	fix t, t0 = 0;
	do {
		if ((t = SDL_GetTicks ()) > t0 + I2X (1)) {
			t0 = t;
			NetworkSendGameListRequest ();
			}
		NetworkListen ();
		if (KeyInKey () == KEY_ESC)
			return 0;
		} while (!networkData.nActiveGames);
	choice = 0;
	}
else {
	gameStates.multi.bSurfingNet = 1;
//	NMLoadBackground (BackgroundName (BG_MENU), &bg, 0);             //load this here so if we abort after loading level, we restore the palette
//	paletteManager.LoadEffect ();
	choice = m.Menu (TXT_NETGAMES, NULL, NetworkJoinPoll, NULL, NULL, LHX (340), -1, 1);
//	backgroundManager.Remove ();
	gameStates.multi.bSurfingNet = 0;
	}
if (choice == -1) {
	ChangePlayerNumTo (0);
	memcpy (LOCALPLAYER.callsign, callsign, sizeof (callsign));
	networkData.nStatus = NETSTAT_MENU;
	return 0; // they cancelled               
	}               
choice -= (2 + gameStates.multi.bUseTracker);
if ((choice < 0) || (choice >= networkData.nActiveGames)) {
	//MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INVALID_CHOICE);
	goto doMenu;
	}

// Choice has been made and looks legit
if (AGI.gameStatus == NETSTAT_ENDLEVEL) {
	MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
	goto doMenu;
	}
if (AGI.protocolVersion != MULTI_PROTO_VERSION) {
	if (AGI.protocolVersion == 3) {
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INCOMPAT1);
		}
	else if (AGI.protocolVersion == 4) {
		}
	else {
		char	szFmt [200], szError [200];

		sprintf (szFmt, "%s%s", TXT_VERSION_MISMATCH, TXT_NETGAME_VERSIONS);
		sprintf (szError, szFmt, MULTI_PROTO_VERSION, AGI.protocolVersion);
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, szError);
		}
	goto doMenu;
	}

if (gameStates.multi.bUseTracker) {
	//PrintLog ("   getting server lists from trackers\n");
	GetServerFromList (choice);
	}
// Check for valid mission name
console.printf (CON_DBG, TXT_LOADING_MSN, AGI.szMissionName);
if (!(LoadMissionByName (AGI.szMissionName, -1) ||
		(DownloadMission (AGI.szMissionName) &&
		 LoadMissionByName (AGI.szMissionName, -1)))) {
	PrintLog ("Mission '%s' not found%s\n", AGI.szMissionName);
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
	goto doMenu;
	}
if (IS_D2_OEM && (AGI.nLevel > 8)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_OEM_ONLY8);
	goto doMenu;
	}
if (IS_MAC_SHARE && (AGI.nLevel > 4)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_SHARE_ONLY4);
	goto doMenu;
	}
if (!NetworkWaitForAllInfo (choice)) {
	MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_JOIN_ERROR);
	networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
	goto doMenu;
	}       

networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
  if (!CanJoinNetgame (activeNetGames + choice, activeNetPlayers + choice)) {
	if (AGI.nNumPlayers == AGI.nMaxPlayers)
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_GAME_FULL);
	else
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_IN_PROGRESS);
	goto doMenu;
	}
// Choice is valid, prepare to join in
memcpy (&netGame, activeNetGames + choice, sizeof (tNetgameInfo));
memcpy (&netPlayers, activeNetPlayers + choice, sizeof (tAllNetPlayersInfo));
gameStates.app.nDifficultyLevel = netGame.difficulty;
gameData.multiplayer.nMaxPlayers = netGame.nMaxPlayers;
ChangePlayerNumTo (1);
memcpy (LOCALPLAYER.callsign, callsign, sizeof (callsign));
// Handle the extra data for the network driver
// For the mcast4 driver, this is the game's multicast address, to
// which the driver subscribes.
if (IpxHandleNetGameAuxData (netGame.AuxData) < 0) {
	networkData.nStatus = NETSTAT_BROWSING;
	goto doMenu;
	}
NetworkSetGameMode (netGame.gameMode);
NetworkAdjustMaxDataSize ();
//PrintLog ("loading level\n");
StartNewLevel (netGame.nLevel, 0);
//PrintLog ("exiting netgame browser\n");
backgroundManager.Remove ();
return 1;         // look ma, we're in a game!!!
}

//------------------------------------------------------------------------------

