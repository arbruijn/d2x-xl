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

#include "descent.h"
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
#include "console.h"

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

//------------------------------------------------------------------------------

int NetworkSelectTeams (void)
{
	CMenu		m;
	int		choice;
	ushort	teamVector = 0;
	char		teamNames [2][CALLSIGN_LEN+1];
	int		i, j;
	int		playerIds [MAX_PLAYERS+2];
	char		szId [100];

// one time initialization
for (i = gameData.multiplayer.nPlayers / 2; i < gameData.multiplayer.nPlayers; i++) // Put first half of players on team A
	teamVector |= (1 << i);
sprintf (teamNames [0], "%s", TXT_BLUE);
sprintf (teamNames [1], "%s", TXT_RED);

for (;;) {
	m.Destroy ();
	m.Create (MAX_PLAYERS + 4);

	m.AddInput ("red team", teamNames [0], CALLSIGN_LEN);
	for (i = j = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!(teamVector & (1 << i))) {
			sprintf (szId, "red player %d", ++j);
			m.AddMenu (szId, netPlayers [0].m_info.players [i].callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	m.AddInput ("blue team", teamNames [1], CALLSIGN_LEN); 
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (teamVector & (1 << i)) {
			sprintf (szId, "blue player %d", ++j);
			m.AddMenu (szId, netPlayers [0].m_info.players [i].callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	m.AddText ("end of teams", "");
	m.AddMenu ("accept teams", TXT_ACCEPT, KEY_A);

	choice = m.Menu (NULL, TXT_TEAM_SELECTION, NULL, NULL);

	int redTeam = m.IndexOf ("red team");
	int blueTeam = m.IndexOf ("blue team");
	int endOfTeams = m.IndexOf ("end of teams");
	if (choice == int (m.ToS ()) - 1) {
		if ((endOfTeams - blueTeam < 2) || (blueTeam - redTeam < 2))
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
		netGame.m_info.SetTeamVector (teamVector);
		strcpy (netGame.m_info.szTeamName [0], teamNames [0]);
		strcpy (netGame.m_info.szTeamName [1], teamNames [1]);
		return 1;
		}
	else if ((choice > redTeam) && (choice < blueTeam)) 
		teamVector |= (1 << playerIds [choice]);
	else if ((choice > blueTeam) && (choice < endOfTeams)) 
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
	if (m [i].Value ()) 
		gameData.multiplayer.nPlayers++;
	}
if (gameData.multiplayer.nPlayers > netGame.m_info.nMaxPlayers) {
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
if (((netGame.m_info.gameMode == NETGAME_TEAM_ANARCHY) ||
	  (netGame.m_info.gameMode == NETGAME_CAPTURE_FLAG) || 
	  (netGame.m_info.gameMode == NETGAME_TEAM_HOARD)) && 
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
			netPlayers [0].m_info.players [i].network.Server (), 
			netPlayers [0].m_info.players [i].network.Node (), 
			DUMP_ABORTED);
	}

netGame.m_info.nNumPlayers = 0;
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

PrintLog (1, "Selecting netgame players\n");
NetworkAddPlayer (&networkData.thisPlayer);
if (bAutoRun) {
	PrintLog (-1);
	return 1;
	}

m [0].Value () = 1;                         // Assume server will play...
if (gameOpts->multi.bNoRankings)
	sprintf (text [0], "%d. %-20s", 1, LOCALPLAYER.callsign);
else
	sprintf (text [0], "%d. %s%-20s", 1, pszRankStrings [netPlayers [0].m_info.players [gameData.multiplayer.nLocalPlayer].rank], LOCALPLAYER.callsign);
m.AddCheck ("", text [0], 0);
for (i = 1; i < MAX_PLAYERS + 4; i++) {
	sprintf (text [i], "%d.  %-20s", i + 1, "");
	m.AddCheck ("", text [i], 0);
	}
sprintf (title, "%s %d %s", TXT_TEAM_SELECT, gameData.multiplayer.nMaxPlayers, TXT_TEAM_PRESS_ENTER);

do {
	gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_PLAYER_SELECT;
	j = m.Menu (NULL, title, NetworkStartPoll, &choice);
	nSavePlayers = gameData.multiplayer.nPlayers;
	if (j < 0) {
		PrintLog (-1);
		return AbortPlayerSelection (nSavePlayers);
		}
	} while (GetSelectedPlayers (m, nSavePlayers));
// Remove players that aren't marked.
gameData.multiplayer.nPlayers = 0;
for (i = 0; i < nSavePlayers; i++) {
	if (m [i].Value ()) {
		if (i > gameData.multiplayer.nPlayers) {
			if (gameStates.multi.nGameType >= IPX_GAME) {
				memcpy (netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].network.Node (), 
						  netPlayers [0].m_info.players [i].network.Node (), 6);
				memcpy (netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].network.Server (), 
						  netPlayers [0].m_info.players [i].network.Server (), 4);
				}
			else {
				memcpy (&netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].network.AppleTalk (), 
						  &netPlayers [0].m_info.players [i].network.AppleTalk (), 
						  sizeof (tAppleTalkAddr));
				}
			memcpy (
				netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].callsign, 
				netPlayers [0].m_info.players [i].callsign, CALLSIGN_LEN+1);
			netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].versionMajor = netPlayers [0].m_info.players [i].versionMajor;
			netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].versionMinor = netPlayers [0].m_info.players [i].versionMinor;
			netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].rank = netPlayers [0].m_info.players [i].rank;
			ClipRank (reinterpret_cast<char*> (&netPlayers [0].m_info.players [gameData.multiplayer.nPlayers].rank));
			NetworkCheckForOldVersion ((char)i);
			}
		gameData.multiplayer.players [gameData.multiplayer.nPlayers].connected = CONNECT_PLAYING;
		gameData.multiplayer.nPlayers++;
		}
	else {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (netPlayers [0].m_info.players [i].network.Server (), netPlayers [0].m_info.players [i].network.Node (), DUMP_DORK);
		}
	}

for (i = gameData.multiplayer.nPlayers; i < MAX_NUM_NET_PLAYERS; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		memset (netPlayers [0].m_info.players [i].network.Node (), 0, 6);
		memset (netPlayers [0].m_info.players [i].network.Server (), 0, 4);
	   }
	else {
		netPlayers [0].m_info.players [i].network.AppleTalk ().node = 0;
		netPlayers [0].m_info.players [i].network.AppleTalk ().net = 0;
		netPlayers [0].m_info.players [i].network.AppleTalk ().socket = 0;
	   }
	memset (netPlayers [0].m_info.players [i].callsign, 0, CALLSIGN_LEN+1);
	netPlayers [0].m_info.players [i].versionMajor = 0;
	netPlayers [0].m_info.players [i].versionMinor = 0;
	netPlayers [0].m_info.players [i].rank = 0;
	}
#if 1			
console.printf (CON_DBG, "Select teams: Game mode is %d\n", netGame.m_info.gameMode);
#endif
if (netGame.m_info.gameMode == NETGAME_TEAM_ANARCHY ||
	 netGame.m_info.gameMode == NETGAME_CAPTURE_FLAG ||
	 netGame.m_info.gameMode == NETGAME_TEAM_HOARD ||
	 netGame.m_info.gameMode == NETGAME_ENTROPY ||
	 netGame.m_info.gameMode == NETGAME_MONSTERBALL)
	if (!NetworkSelectTeams ()) {
		PrintLog (-1);
		return AbortPlayerSelection (nSavePlayers);
		}
PrintLog (-1);
return 1; 
}

//------------------------------------------------------------------------------

