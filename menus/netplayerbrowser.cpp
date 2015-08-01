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

int32_t NetworkSelectTeams (void)
{
	CMenu		m;
	int32_t		choice;
	uint16_t	teamVector = 0;
	char		teamNames [2][CALLSIGN_LEN+1];
	int32_t		i, j;
	int32_t		playerIds [MAX_PLAYERS+2];
	char		szId [100];

// one time initialization
for (i = N_PLAYERS / 2; i < N_PLAYERS; i++) // Put first half of players on team A
	teamVector |= (1 << i);
sprintf (teamNames [0], "%s", TXT_BLUE);
sprintf (teamNames [1], "%s", TXT_RED);

for (;;) {
	m.Destroy ();
	m.Create (MAX_PLAYERS + 4);

	m.AddInput ("red team", teamNames [0], CALLSIGN_LEN);
	for (i = j = 0; i < N_PLAYERS; i++) {
		if (!(teamVector & (1 << i))) {
			sprintf (szId, "red player %d", ++j);
			m.AddMenu (szId, NETPLAYER (i).callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	m.AddInput ("blue team", teamNames [1], CALLSIGN_LEN); 
	for (i = 0; i < N_PLAYERS; i++) {
		if (teamVector & (1 << i)) {
			sprintf (szId, "blue player %d", ++j);
			m.AddMenu (szId, NETPLAYER (i).callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	m.AddText ("end of teams", "");
	m.AddMenu ("accept teams", TXT_ACCEPT, KEY_A);

	choice = m.Menu (NULL, TXT_TEAM_SELECTION, NULL, NULL);

	int32_t redTeam = m.IndexOf ("red team");
	int32_t blueTeam = m.IndexOf ("blue team");
	int32_t endOfTeams = m.IndexOf ("end of teams");
	if (choice == int32_t (m.ToS ()) - 1) {
		if ((endOfTeams - blueTeam < 2) || (blueTeam - redTeam < 2))
			TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_TEAM_MUST_ONE);
		netGameInfo.m_info.SetTeamVector (teamVector);
		strcpy (netGameInfo.m_info.szTeamName [0], teamNames [0]);
		strcpy (netGameInfo.m_info.szTeamName [1], teamNames [1]);
		NetworkSendGameInfo (NULL);
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

static bool GetSelectedPlayers (CMenu& m, int32_t nSavePlayers)
{
// Count number of players chosen
N_PLAYERS = 0;
for (int32_t i = 0; i < nSavePlayers; i++) {
	if (m [i].Value ()) 
		N_PLAYERS++;
	}
if (N_PLAYERS > netGameInfo.m_info.nMaxPlayers) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	N_PLAYERS = nSavePlayers;
	return true;
	}
#if !DBG
if (N_PLAYERS < 2) {
	InfoBox (TXT_WARNING, NULL, BG_STANDARD, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO);
#	if 0
	N_PLAYERS = nSavePlayers;
	return true;
#	endif
	}
#endif

#if !DBG
if (((netGameInfo.m_info.gameMode == NETGAME_TEAM_ANARCHY) ||
	  (netGameInfo.m_info.gameMode == NETGAME_CAPTURE_FLAG) || 
	  (netGameInfo.m_info.gameMode == NETGAME_TEAM_HOARD)) && 
	 (N_PLAYERS < 2)) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_NEED_2PLAYERS);
	N_PLAYERS = nSavePlayers;
#if 0	
	return true;
#endif	
	}
#endif
return false;
}

//------------------------------------------------------------------------------

int32_t AbortPlayerSelection (int32_t nSavePlayers)
{
for (int32_t i = 1; i < nSavePlayers; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (NETPLAYER (i).network.Network (), NETPLAYER (i).network.Node (), DUMP_ABORTED);
	}

netGameInfo.m_info.nNumPlayers = 0;
NetworkSendGameInfo (0); // Tell everyone we're bailing
IpxHandleLeaveGame (); // Tell the network driver we're bailing too
networkData.nStatus = NETSTAT_MENU;
return 0;
}

//------------------------------------------------------------------------------

int32_t NetworkSelectPlayers (int32_t bAutoRun)
{
	int32_t		i, j, choice = 1;
   CMenu		m (MAX_PLAYERS + 4);
   char		text [MAX_PLAYERS+4][45];
	char		title [200];
	int32_t		nSavePlayers;              //how may people would like to join

PrintLog (1, "Selecting netgame players\n");
NetworkAddPlayer (&networkData.pThislayer);
if (bAutoRun) {
	PrintLog (-1);
	return 1;
	}

m [0].Value () = 1;                         // Assume server will play...
if (gameOpts->multi.bNoRankings)
	sprintf (text [0], "%d. %-20s", 1, LOCALPLAYER.callsign);
else
	sprintf (text [0], "%d. %s%-20s", 1, pszRankStrings [NETPLAYER (N_LOCALPLAYER).rank], LOCALPLAYER.callsign);
m.AddCheck ("", text [0], 0);
for (i = 1; i < MAX_PLAYERS + 4; i++) {
	sprintf (text [i], "%d.  %-20s", i + 1, "");
	m.AddCheck ("", text [i], 0);
	}
sprintf (title, "%s %d %s", TXT_TEAM_SELECT, gameData.multiplayer.nMaxPlayers, TXT_TEAM_PRESS_ENTER);

do {
	gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_PLAYER_SELECT;
	j = m.Menu (NULL, title, NetworkStartPoll, &choice);
	nSavePlayers = N_PLAYERS;
	if (j < 0) {
		PrintLog (-1);
		return AbortPlayerSelection (nSavePlayers);
		}
	} while (GetSelectedPlayers (m, nSavePlayers));
// Remove players that aren't marked.
N_PLAYERS = 0;
for (i = 0; i < nSavePlayers; i++) {
	if (m [i].Value ()) {
		if (i > N_PLAYERS) {
			if (gameStates.multi.nGameType >= IPX_GAME) {
				NETPLAYER (N_PLAYERS).network.SetNode (NETPLAYER (i).network.Node ());
				NETPLAYER (N_PLAYERS).network.SetNetwork (NETPLAYER (i).network.Network ());
				}
			else {
				memcpy (&NETPLAYER (N_PLAYERS).network.AppleTalk (), 
						  &NETPLAYER (i).network.AppleTalk (), 
						  sizeof (tAppleTalkAddr));
				}
			memcpy (
				NETPLAYER (N_PLAYERS).callsign, 
				NETPLAYER (i).callsign, CALLSIGN_LEN+1);
			NETPLAYER (N_PLAYERS).versionMajor = NETPLAYER (i).versionMajor;
			NETPLAYER (N_PLAYERS).versionMinor = NETPLAYER (i).versionMinor;
			NETPLAYER (N_PLAYERS).rank = NETPLAYER (i).rank;
			ClipRank (reinterpret_cast<char*> (&NETPLAYER (N_PLAYERS).rank));
			NetworkCheckForOldVersion ((char)i);
			}
		CONNECT (N_PLAYERS, CONNECT_PLAYING);
		++N_PLAYERS;
		}
	else {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (NETPLAYER (i).network.Network (), NETPLAYER (i).network.Node (), DUMP_DORK);
		}
	}

for (i = N_PLAYERS; i < MAX_NUM_NET_PLAYERS; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		NETPLAYER (i).network.ResetNode ();
		NETPLAYER (i).network.ResetNetwork ();
	   }
	else {
		NETPLAYER (i).network.AppleTalk ().node = 0;
		NETPLAYER (i).network.AppleTalk ().net = 0;
		NETPLAYER (i).network.AppleTalk ().socket = 0;
	   }
	memset (NETPLAYER (i).callsign, 0, CALLSIGN_LEN+1);
	NETPLAYER (i).versionMajor = 0;
	NETPLAYER (i).versionMinor = 0;
	NETPLAYER (i).rank = 0;
	}
#if 1			
console.printf (CON_DBG, "Select teams: Game mode is %d\n", netGameInfo.m_info.gameMode);
#endif
if (netGameInfo.m_info.gameMode == NETGAME_TEAM_ANARCHY ||
	 netGameInfo.m_info.gameMode == NETGAME_CAPTURE_FLAG ||
	 netGameInfo.m_info.gameMode == NETGAME_TEAM_HOARD ||
	 netGameInfo.m_info.gameMode == NETGAME_ENTROPY ||
	 netGameInfo.m_info.gameMode == NETGAME_MONSTERBALL)
	if (!NetworkSelectTeams ()) {
		PrintLog (-1);
		return AbortPlayerSelection (nSavePlayers);
		}
PrintLog (-1);
return 1; 
}

//------------------------------------------------------------------------------

