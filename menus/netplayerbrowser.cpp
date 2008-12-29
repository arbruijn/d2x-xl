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
#include "netmenu.h"
#include "menubackground.h"

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

//------------------------------------------------------------------------------

int NetworkSelectTeams (void)
{
	CMenu	m;
	int	choice, opt_team_b;
	ubyte teamVector = 0;
	char	teamNames [2][CALLSIGN_LEN+1];
	int	i;
	int	playerIds [MAX_PLAYERS+2];

	// One-time initialization

for (i = gameData.multiplayer.nPlayers/2; i < gameData.multiplayer.nPlayers; i++) // Put first half of players on team A
	teamVector |= (1 << i);
sprintf (teamNames [0], "%s", TXT_BLUE);
sprintf (teamNames [1], "%s", TXT_RED);

for (;;) {
	m.Destroy ();
	m.Create (MAX_PLAYERS + 4);

	m.AddInput (teamNames [0], CALLSIGN_LEN);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!(teamVector & (1 << i))) {
			m.AddMenu (netPlayers.players [i].callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	opt_team_b = m.AddInput (teamNames [1], CALLSIGN_LEN); 
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (teamVector & (1 << i)) {
			m.AddMenu (netPlayers.players [i].callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	m.AddText ("");
	m.AddMenu (TXT_ACCEPT, KEY_A);

	choice = m.Menu (NULL, TXT_TEAM_SELECTION, NULL, NULL);

	if (choice == m.ToS () - 1) {
		if ((m.ToS () - 2 - opt_team_b < 2) || (opt_team_b == 1)) 
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
		netGame.teamVector = teamVector;
		strcpy (netGame.szTeamName [0], teamNames [0]);
		strcpy (netGame.szTeamName [1], teamNames [1]);
		return 1;
		}
	else if ((choice > 0) && (choice < opt_team_b)) 
		teamVector |= (1 << playerIds [choice]);
	else if ((choice > opt_team_b) && (choice < int (m.ToS ()) - 2)) 
		teamVector &= ~ (1 << playerIds [choice]);
	else if (choice == -1)
		return 0;
	}
}

//------------------------------------------------------------------------------

static bool GetSelectedPlayers (CMenu& m, int nSavePlayers)
{
// Count number of players chosen
gameData.multiplayer.nPlayers = 0;
for (int i = 0; i < nSavePlayers; i++) {
	if (m [i].m_value) 
		gameData.multiplayer.nPlayers++;
	}
if (gameData.multiplayer.nPlayers > netGame.nMaxPlayers) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	gameData.multiplayer.nPlayers = nSavePlayers;
	return true;
	}
#if !DBG
if (gameData.multiplayer.nPlayers < 2) {
	MsgBox (TXT_WARNING, NULL, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO);
#	if 0
	gameData.multiplayer.nPlayers = nSavePlayers;
	return true;
#	endif
	}
#endif

#if !DBG
if (((netGame.gameMode == NETGAME_TEAM_ANARCHY) ||
	  (netGame.gameMode == NETGAME_CAPTURE_FLAG) || 
	  (netGame.gameMode == NETGAME_TEAM_HOARD)) && 
	 (gameData.multiplayer.nPlayers < 2)) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_NEED_2PLAYERS);
	gameData.multiplayer.nPlayers = nSavePlayers;
#if 0	
	return true;
#endif	
	}
#endif
return false;
}

//------------------------------------------------------------------------------

int AbortPlayerSelection (int nSavePlayers)
{
for (int i = 1; i < nSavePlayers; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			netPlayers.players [i].network.ipx.server, 
			netPlayers.players [i].network.ipx.node, 
			DUMP_ABORTED);
	}

netGame.nNumPlayers = 0;
NetworkSendGameInfo (0); // Tell everyone we're bailing
IpxHandleLeaveGame (); // Tell the network driver we're bailing too
networkData.nStatus = NETSTAT_MENU;
return 0;
}

//------------------------------------------------------------------------------

int NetworkSelectPlayers (int bAutoRun)
{
	int		i, j, choice = 1;
   CMenu		m (MAX_PLAYERS + 4);
   char		text [MAX_PLAYERS+4][45];
	char		title [50];
	int		nSavePlayers;              //how may people would like to join

NetworkAddPlayer (&networkData.thisPlayer);
if (bAutoRun)
	return 1;

for (i = 0; i < MAX_PLAYERS + 4; i++) {
	sprintf (text [i], "%d.  %-20s", i+1, "");
	m.AddCheck (text [i], 0);
	}
m [0].m_value = 1;                         // Assume server will play...
if (gameOpts->multi.bNoRankings)
	sprintf (text [0], "%d. %-20s", 1, LOCALPLAYER.callsign);
else
	sprintf (text [0], "%d. %s%-20s", 1, pszRankStrings [netPlayers.players [gameData.multiplayer.nLocalPlayer].rank], LOCALPLAYER.callsign);
sprintf (title, "%s %d %s", TXT_TEAM_SELECT, gameData.multiplayer.nMaxPlayers, TXT_TEAM_PRESS_ENTER);

do {
	gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_PLAYER_SELECT;
	j = m.Menu (NULL, title, NetworkStartPoll, &choice);
	nSavePlayers = gameData.multiplayer.nPlayers;
	if (j < 0)
		return AbortPlayerSelection (nSavePlayers);
	} while (GetSelectedPlayers (m, nSavePlayers));
// Remove players that aren't marked.
gameData.multiplayer.nPlayers = 0;
for (i = 0; i < nSavePlayers; i++) {
	if (m [i].m_value) {
		if (i > gameData.multiplayer.nPlayers) {
			if (gameStates.multi.nGameType >= IPX_GAME) {
				memcpy (netPlayers.players [gameData.multiplayer.nPlayers].network.ipx.node, 
				netPlayers.players [i].network.ipx.node, 6);
				memcpy (netPlayers.players [gameData.multiplayer.nPlayers].network.ipx.server, 
				netPlayers.players [i].network.ipx.server, 4);
				}
			else {
				netPlayers.players [gameData.multiplayer.nPlayers].network.appletalk.node = netPlayers.players [i].network.appletalk.node;
				netPlayers.players [gameData.multiplayer.nPlayers].network.appletalk.net = netPlayers.players [i].network.appletalk.net;
				netPlayers.players [gameData.multiplayer.nPlayers].network.appletalk.socket = netPlayers.players [i].network.appletalk.socket;
				}
			memcpy (
				netPlayers.players [gameData.multiplayer.nPlayers].callsign, 
				netPlayers.players [i].callsign, CALLSIGN_LEN+1);
			netPlayers.players [gameData.multiplayer.nPlayers].versionMajor = netPlayers.players [i].versionMajor;
			netPlayers.players [gameData.multiplayer.nPlayers].versionMinor = netPlayers.players [i].versionMinor;
			netPlayers.players [gameData.multiplayer.nPlayers].rank = netPlayers.players [i].rank;
			ClipRank (reinterpret_cast<char*> (&netPlayers.players [gameData.multiplayer.nPlayers].rank));
			NetworkCheckForOldVersion ((char)i);
			}
		gameData.multiplayer.players [gameData.multiplayer.nPlayers].connected = 1;
		gameData.multiplayer.nPlayers++;
		}
	else {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (netPlayers.players [i].network.ipx.server, netPlayers.players [i].network.ipx.node, DUMP_DORK);
		}
	}

for (i = gameData.multiplayer.nPlayers; i < MAX_NUM_NET_PLAYERS; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		memset (netPlayers.players [i].network.ipx.node, 0, 6);
		memset (netPlayers.players [i].network.ipx.server, 0, 4);
	   }
	else {
		netPlayers.players [i].network.appletalk.node = 0;
		netPlayers.players [i].network.appletalk.net = 0;
		netPlayers.players [i].network.appletalk.socket = 0;
	   }
	memset (netPlayers.players [i].callsign, 0, CALLSIGN_LEN+1);
	netPlayers.players [i].versionMajor = 0;
	netPlayers.players [i].versionMinor = 0;
	netPlayers.players [i].rank = 0;
	}
#if 1			
console.printf (CON_DBG, "Select teams: Game mode is %d\n", netGame.gameMode);
#endif
if (netGame.gameMode == NETGAME_TEAM_ANARCHY ||
	 netGame.gameMode == NETGAME_CAPTURE_FLAG ||
	 netGame.gameMode == NETGAME_TEAM_HOARD ||
	 netGame.gameMode == NETGAME_ENTROPY ||
	 netGame.gameMode == NETGAME_MONSTERBALL)
	if (!NetworkSelectTeams ())
		return AbortPlayerSelection (nSavePlayers);
return 1; 
}

//------------------------------------------------------------------------------

