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
#	include <conf.h>
#endif

#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "descent.h"
#include "args.h"
#include "timer.h"
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "multibot.h"
#include "objsmoke.h"
#include "newdemo.h"
#include "text.h"
#include "banlist.h"
#include "console.h"

//------------------------------------------------------------------------------

int32_t NetworkFindPlayer (tNetPlayerInfo *playerP)
{
	int32_t	i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (!CmpNetPlayers (NULL, NULL, &netPlayers [0].m_info.players [i].network, &playerP->network))
		return i;         // already got them
return -1;
}

//------------------------------------------------------------------------------

int32_t GetNewPlayerNumber (tPlayerSyncData *their)
  {
	 int32_t i;

if ((gameData.multiplayer.nPlayers < gameData.multiplayer.nMaxPlayers) && 
	 (gameData.multiplayer.nPlayers < gameData.multiplayer.nPlayerPositions))
	return gameData.multiplayer.nPlayers;
// Slots are full but game is open, see if anyone is
// disconnected and replace the oldest player with this new one
int32_t oldestPlayer = -1;
fix oldestTime = gameStates.app.nSDLTicks [0];

Assert (gameData.multiplayer.nPlayers == gameData.multiplayer.nMaxPlayers);
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((!gameData.multiplayer.players [i].connected) && (networkData.nLastPacketTime [i] < oldestTime)) {
		oldestTime = networkData.nLastPacketTime [i];
		oldestPlayer = i;
		}
	}
return oldestPlayer;
}

//------------------------------------------------------------------------------

int32_t CanJoinNetGame (CNetGameInfo *game, CAllNetPlayersInfo *people)
{
	// Can this CPlayerData rejoin a netgame in progress?

	int32_t i, nNumPlayers;

if (game->m_info.gameStatus == NETSTAT_STARTING)
   return 1;
if (game->m_info.gameStatus != NETSTAT_PLAYING) {
#if 1      
	console.printf (CON_DBG, "Error: Cannot join because gameStatus !=NETSTAT_PLAYING\n");
#endif
	return 0;
    }

if ((game->m_info.versionMajor == 0) && (D2X_MAJOR > 0)) {
#if 1      
	console.printf (CON_DBG, "Error: Cannot join because version majors don't match!\n");
#endif
	return 0;
	}

if ((game->m_info.versionMajor > 0) && (D2X_MAJOR == 0)) {
#if 1      
	console.printf (CON_DBG, "Error: Cannot join because version majors2 don't match!\n");
#endif
	return 0;
	}
// Game is in progress, figure out if this guy can re-join it
nNumPlayers = game->m_info.nNumPlayers;

if (!(game->m_info.gameFlags & NETGAME_FLAG_CLOSED)) {
	// Look for CPlayerData that is not connected
	if (game->m_info.nConnected == game->m_info.nMaxPlayers)
		 return 2;
	if (game->m_info.bRefusePlayers)
		 return 3;
	if (game->m_info.nNumPlayers < game->m_info.nMaxPlayers)
		return 1;
	if (game->m_info.nConnected < nNumPlayers)
		return 1;
	}
if (!people) {
	console.printf (CON_DBG, "Error! Can't join because people == NULL!\n");
	return 0;
   }
// Search to see if we were already in this closed netgame in progress
for (i = 0; i < nNumPlayers; i++)
	if (!CmpNetPlayers (LOCALPLAYER.callsign, 
							  people->m_info.players [i].callsign, 
							  &networkData.thisPlayer.player.network, 
							  &people->m_info.players [i].network))
		return 1;
#if 1      
console.printf (CON_DBG, "Error: Can't join because at end of list!\n");
#endif
return 0;
}

//------------------------------------------------------------------------------

void NetworkResetSyncStates (void)
{
networkData.nJoining = 0;
}

//------------------------------------------------------------------------------

void NetworkResetObjSync (int16_t nObject)
{
for (int16_t i = 0; i < networkData.nJoining; i++)
	if ((networkData.syncInfo [i].nState == 1) && ((nObject < 0) || NetworkObjnumIsPast (nObject, networkData.syncInfo + i)))
		networkData.syncInfo [i].objs.nCurrent = -1;
}

//------------------------------------------------------------------------------

void NetworkDisconnectPlayer (int32_t nPlayer)
{
CPlayerData& player = gameData.multiplayer.players [nPlayer];

player.m_tDisconnect = SDL_GetTicks ();
if (player.Connected (CONNECT_PLAYING)) {
	if (!player.m_nLevel || (player.m_nLevel == missionManager.nCurrentLevel))
		CONNECT (nPlayer, CONNECT_DISCONNECTED);
	KillPlayerSmoke (nPlayer);
	gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration =
	gameData.multiplayer.weaponStates [nPlayer].firing [1].nDuration = 0;
	KillPlayerBullets (gameData.Object (player.nObject));
	KillGatlingSmoke (gameData.Object (player.nObject));
	for (int16_t i = 0; i < networkData.nJoining; i++)
		if (networkData.syncInfo [i].nPlayer == nPlayer) {
			DeleteSyncData (i);
			NetworkResetSyncStates ();
			}
	// OBJECTS [player.nObject].CreateAppearanceEffect ();
	MultiMakePlayerGhost (nPlayer);
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiDisconnect (nPlayer);
	MultiStripRobots (nPlayer);
	}
}
	
//------------------------------------------------------------------------------

void NetworkNewPlayer (tPlayerSyncData *their)
{
	int32_t	nPlayer;

nPlayer = their->player.connected;
Assert (nPlayer >= 0);
Assert (nPlayer < gameData.multiplayer.nMaxPlayers);        
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiConnect (nPlayer, nPlayer == gameData.multiplayer.nPlayers, their->player.callsign);
memcpy (gameData.multiplayer.players [nPlayer].callsign, their->player.callsign, CALLSIGN_LEN + 1);
memcpy (netPlayers [0].m_info.players [nPlayer].callsign, their->player.callsign, CALLSIGN_LEN + 1);
memset (pingStats + nPlayer, 0, sizeof (pingStats [0]));
ClipRank (reinterpret_cast<char*> (&their->player.rank));
netPlayers [0].m_info.players [nPlayer].rank = their->player.rank;
netPlayers [0].m_info.players [nPlayer].versionMajor = their->player.versionMajor;
netPlayers [0].m_info.players [nPlayer].versionMinor = their->player.versionMinor;
NetworkCheckForOldVersion ((char) nPlayer);

if (gameStates.multi.nGameType >= IPX_GAME) {
	if (*reinterpret_cast<uint32_t*> (their->player.network.Network ()) != 0)
		IpxGetLocalTarget (
			their->player.network.Network (), 
			their->player.network.Node (), 
			gameData.multiplayer.players [nPlayer].netAddress);
	else
		memcpy (gameData.multiplayer.players [nPlayer].netAddress, their->player.network.Node (), 6);
	}
memcpy (&netPlayers [0].m_info.players [nPlayer].network, &their->player.network, sizeof (CNetworkInfo));
gameData.multiplayer.players [nPlayer].nPacketsGot = 0;
CONNECT (nPlayer, CONNECT_PLAYING);
gameData.multiplayer.players [nPlayer].netKillsTotal = 0;
gameData.multiplayer.players [nPlayer].netKilledTotal = 0;
memset (gameData.multigame.score.matrix [nPlayer], 0, MAX_NUM_NET_PLAYERS * sizeof (int16_t)); 
gameData.multiplayer.players [nPlayer].score = 0;
gameData.multiplayer.players [nPlayer].flags = 0;
gameData.multiplayer.players [nPlayer].nScoreGoalCount = 0;
if (nPlayer == gameData.multiplayer.nPlayers) {
	gameData.multiplayer.nPlayers++;
	netGameInfo.m_info.nNumPlayers = gameData.multiplayer.nPlayers;
	}
audio.PlaySound (SOUND_HUD_MESSAGE);
ClipRank (reinterpret_cast<char*> (&their->player.rank));
if (gameOpts->multi.bNoRankings)
	HUDInitMessage ("'%s' %s\n", their->player.callsign, TXT_JOINING);
else   
   HUDInitMessage ("%s'%s' %s\n", pszRankStrings [their->player.rank], their->player.callsign, TXT_JOINING);
MultiMakeGhostPlayer (nPlayer);
GetPlayerSpawn (GetRandomPlayerPosition (nPlayer), OBJECTS + gameData.multiplayer.players [nPlayer].nObject);
MultiSendScore ();
MultiSortKillList ();
// OBJECTS [nObject].CreateAppearanceEffect ();
}

//------------------------------------------------------------------------------

static int32_t FindNetworkPlayer (tPlayerSyncData *player, uint8_t *newAddress)
{
	int32_t	i;
	uint8_t anyAddress [6];

if (gameStates.multi.nGameType == UDP_GAME) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, newAddress, extraGameInfo [1].bCheckUDPPort ? 6 : 4) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, player->player.callsign)) {
			memcpy (gameData.multiplayer.players [i].netAddress, newAddress, 6);
			return i;
			}
		}
	memset (&anyAddress, 0xFF, sizeof (anyAddress));
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, anyAddress, 6) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, player->player.callsign)) {
			memcpy (gameData.multiplayer.players [i].netAddress, newAddress, 6);
			memcpy (netPlayers [0].m_info.players [i].network.Node (), newAddress, 6);
			return i;
			}
		}
	}
else {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) 
		if (gameStates.multi.nGameType == IPX_GAME) {
			if (!memcmp (gameData.multiplayer.players [i].netAddress, newAddress, 6) &&
				 !stricmp (gameData.multiplayer.players [i].callsign, player->player.callsign)) {
				return i;
				}
			}
	}
return -1;
}

//------------------------------------------------------------------------------

static int32_t FindPlayerSlot (tPlayerSyncData *player)
{
if (netGameInfo.m_info.gameFlags & NETGAME_FLAG_CLOSED) {
	// Slots are open but game is closed
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			player->player.network.Network (), 
			player->player.network.Node (), 
			DUMP_CLOSED);
	return -1;
	}
if ((gameData.multiplayer.nPlayers < gameData.multiplayer.nMaxPlayers) &&
	 (gameData.multiplayer.nPlayers < gameData.multiplayer.nPlayerPositions)) {
	// Add CPlayerData in an open slot, game not full yet
	networkData.bPlayerAdded = 1;
	return gameData.multiplayer.nPlayers;
	}
// Slots are full but game is open, see if anyone is
// disconnected and replace the oldest CPlayerData with this new one
int32_t oldestPlayer = -1;
fix oldestTime = TimerGetApproxSeconds ();
for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!gameData.multiplayer.players [i].IsConnected () && (networkData.nLastPacketTime [i] < oldestTime)) {
		oldestTime = networkData.nLastPacketTime [i];
		oldestPlayer = i;
		}
	}
if (oldestPlayer == -1) {
	// Everyone is still connected 
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			player->player.network.Network (), 
			player->player.network.Node (), 
			DUMP_FULL);
	return -1;
	}
networkData.bPlayerAdded = 1;
return oldestPlayer;
}

//------------------------------------------------------------------------------

static int16_t FindJoiningPlayer (tPlayerSyncData *player)
{
for (int16_t i = 0; i < networkData.nJoining; i++)
	if (!strcmp (networkData.syncInfo [i].player [1].player.callsign, player->player.callsign) && 
		(networkData.syncInfo [i].player [1].player.network == player->player.network))
		return i;
return -1;
}

//------------------------------------------------------------------------------

tNetworkSyncInfo *FindJoiningPlayer (int16_t nPlayer)
{
for (int16_t i = 0; i < networkData.nJoining; i++)
	if (networkData.syncInfo [i].player [1].player.connected == nPlayer)
		return networkData.syncInfo + i;
return NULL;
}

//------------------------------------------------------------------------------

void DeleteSyncData (int16_t nConnection)
{
if (nConnection < --networkData.nJoining)
	memcpy (networkData.syncInfo + nConnection, networkData.syncInfo + networkData.nJoining, sizeof (tNetworkSyncInfo));
memset (networkData.syncInfo + networkData.nJoining, 0, sizeof (tNetworkSyncInfo));
}

//------------------------------------------------------------------------------

static tNetworkSyncInfo *AcceptJoinRequest (tPlayerSyncData *player)
{
// Don't accept new players if we're ending this level.  Its safe to ignore since they'll request again later
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
#if 1      
console.printf (CON_DBG, "Ignored request from new player to join during endgame.\n");
#endif
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (player->player.network.Network (), player->player.network.Node (), DUMP_ENDLEVEL);
	return NULL; 
	}

if (player->player.connected != missionManager.nCurrentLevel) {
#if 1      
	console.printf (CON_DBG, "Dumping player due to old level number.\n");
#endif
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (player->player.network.Network (), player->player.network.Node (), DUMP_LEVEL);
	return NULL;
	}

tNetworkSyncInfo* syncInfoP;
int16_t nConnection = FindJoiningPlayer (player);

if (nConnection < 0) {
	if (networkData.nJoining == MAX_JOIN_REQUESTS)
		return NULL;
	syncInfoP = networkData.syncInfo + networkData.nJoining++;
	}
else { //prevent flooding with connection attempts from the same player
	syncInfoP = networkData.syncInfo + nConnection;
	if (gameStates.app.nSDLTicks [0] - syncInfoP->tLastJoined < 2100)
		return NULL;
	syncInfoP->tLastJoined = gameStates.app.nSDLTicks [0];
	}
return syncInfoP;
}

//------------------------------------------------------------------------------
// Add a player to a game already in progress

void NetworkWelcomePlayer (tPlayerSyncData *player)
{
	int32_t				nPlayer;
	uint8_t				newAddress [6];
	tNetworkSyncInfo*	syncInfoP;

networkData.refuse.bWaitForAnswer = 0;
if (FindArg ("-NoMatrixCheat")) {
	if ((player->player.versionMinor & 0x0F) < 3) {
		NetworkDumpPlayer (player->player.network.Network (), player->player.network.Node (), DUMP_DORK);
		return;
		}
	}
if (HoardEquipped ()) {
// If hoard game, and this guy isn't D2 Christmas (v1.2), dump him
	if (IsHoardGame && ((player->player.versionMinor & 0x0F) < 2)) {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (
				player->player.network.Network (), 
				player->player.network.Node (), 
				DUMP_DORK);
		return;
		}
	}
if (!(syncInfoP = AcceptJoinRequest (player)))
	return;
memset (&syncInfoP->player [1], 0, sizeof (tPlayerSyncData));
networkData.bPlayerAdded = 0;
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (player->player.network.GetNetwork () != 0)
		IpxGetLocalTarget (player->player.network.Network (), player->player.network.Node (), newAddress);
	else
		memcpy (newAddress, player->player.network.Node (), 6);
	}
if (0 > (nPlayer = FindNetworkPlayer (player, newAddress))) {
	// Player is new to this game
	if (0 > (nPlayer = FindPlayerSlot (player))) {
		DeleteSyncData (int16_t (syncInfoP - networkData.syncInfo));
		return;
		}
	gameData.multiplayer.bAdjustPowerupCap [nPlayer] = true;
	if (IsTeamGame)
		ChoseTeam (nPlayer, true);
	}
else {
	// Player is reconnecting
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nPlayer);
	networkData.bPlayerAdded = 0;
	audio.PlaySound (SOUND_HUD_MESSAGE);
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s", pszRankStrings [netPlayers [0].m_info.players [nPlayer].rank], 
							 gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	}
// Check whether this player is currently trying to sync and has a resync running
// The resync request requires an additional join request to make the game host accept the resync request
gameData.multiplayer.players [nPlayer].nScoreGoalCount = 0;
gameData.multiplayer.players [nPlayer].m_nLevel = missionManager.nCurrentLevel;
CONNECT (nPlayer, CONNECT_DISCONNECTED);
// Send updated OBJECTS data to the new/returning CPlayerData
syncInfoP->player [0] = 
syncInfoP->player [1] = *player;
syncInfoP->player [1].player.connected = nPlayer;
syncInfoP->bExtraGameInfo = 0;
syncInfoP->nState = 1;
syncInfoP->objs.nCurrent = -1;
syncInfoP->nExtras = 0;
syncInfoP->timeout = 0;
syncInfoP->objs.nFramesToSkip = (gameStates.multi.nGameType == UDP_GAME) ? player->nObjFramesToSkip : 0;
syncInfoP->tLastJoined = gameStates.app.nSDLTicks [0];
NetworkSendSync (nPlayer);
NetworkDoSyncFrame ();
}

//------------------------------------------------------------------------------

void NetworkAddPlayer (tPlayerSyncData* playerInfoP)
{
if (NetworkFindPlayer (&playerInfoP->player) > -1)
	return;
tNetPlayerInfo& playerInfo = netPlayers [0].m_info.players [gameData.multiplayer.nPlayers];
memcpy (&playerInfo.network, &playerInfoP->player.network, sizeof (CNetworkInfo));
ClipRank (reinterpret_cast<char*> (&playerInfoP->player.rank));
memcpy (playerInfo.callsign, playerInfoP->player.callsign, CALLSIGN_LEN + 1);
playerInfo.versionMajor = playerInfoP->player.versionMajor;
playerInfo.versionMinor = playerInfoP->player.versionMinor;
playerInfo.rank = playerInfoP->player.rank;
playerInfo.connected = CONNECT_PLAYING;
NetworkCheckForOldVersion ((char) gameData.multiplayer.nPlayers);
CPlayerData& player = gameData.multiplayer.players [gameData.multiplayer.nPlayers];
memcpy (player.callsign, playerInfo.callsign, CALLSIGN_LEN + 1);
player.nScoreGoalCount = 0;
CONNECT (gameData.multiplayer.nPlayers, CONNECT_PLAYING);
ResetPlayerTimeout (gameData.multiplayer.nPlayers, -1);
netGameInfo.m_info.nNumPlayers = ++gameData.multiplayer.nPlayers;
// Broadcast updated info
NetworkSendGameInfo (NULL);
}

//------------------------------------------------------------------------------

// One of the players decided not to join the game

void NetworkRemovePlayer (tPlayerSyncData *player)
{
	int32_t i, j, pn = NetworkFindPlayer (&player->player);

if (pn < 0)
	return;

for (i = pn; i < gameData.multiplayer.nPlayers - 1; ) {
	j = i++;
	memcpy (&netPlayers [0].m_info.players [j].network, &netPlayers [0].m_info.players [i].network, sizeof (CNetworkInfo));
	memcpy (netPlayers [0].m_info.players [j].callsign, netPlayers [0].m_info.players [i].callsign, CALLSIGN_LEN + 1);
	netPlayers [0].m_info.players [j].versionMajor = netPlayers [0].m_info.players [i].versionMajor;
	netPlayers [0].m_info.players [j].versionMinor = netPlayers [0].m_info.players [i].versionMinor;
   netPlayers [0].m_info.players [j].rank = netPlayers [0].m_info.players [i].rank;
	ClipRank (reinterpret_cast<char*> (&netPlayers [0].m_info.players [j].rank));
   NetworkCheckForOldVersion ((char) i);
	}
gameData.multiplayer.nPlayers--;
netGameInfo.m_info.nNumPlayers = gameData.multiplayer.nPlayers;
// Broadcast new info
NetworkSendGameInfo (NULL);
}

//------------------------------------------------------------------------------

void DoRefuseStuff (tPlayerSyncData *their)
{
  int32_t				i, nNewPlayer;

  static tTextIndex	joinMsgIndex;
  static char			szJoinMsg [200];

ClipRank (reinterpret_cast<char*> (&their->player.rank));

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	if (!strcmp (their->player.callsign, gameData.multiplayer.players [i].callsign)) {
		NetworkWelcomePlayer (their);
		return;
		}
if (banList.Find (const_cast<char*>(their->player.callsign)))
	return;
if (!networkData.refuse.bWaitForAnswer) {
	audio.PlaySound (SOUND_HUD_JOIN_REQUEST, SOUNDCLASS_GENERIC, I2X (2));           
#if 1
	if (IsTeamGame) {
		if (gameOpts->multi.bNoRankings)
			HUDInitMessage ("%s joining", their->player.callsign);
		else
			HUDInitMessage ("%s %s wants to join", 
								 pszRankStrings [their->player.rank], their->player.callsign);
		HUDInitMessage ("Alt-1 assigns to team %s. Alt-2 to team %s", netGameInfo.m_info.szTeamName [0], netGameInfo.m_info.szTeamName [1]);
		}               
	else    
		HUDInitMessage (TXT_JOIN_ACCEPT, their->player.callsign);
#endif
#if 1
	if (IsTeamGame) {
		char szRank [20];

		if (gameOpts->multi.bNoRankings)
			*szRank = '\0';
		else
			sprintf (szRank, "%s ", pszRankStrings [their->player.rank]);
		sprintf (szJoinMsg, " \n  %s%s wants to join.  \n  Alt-1 assigns to team %s.  \n  Alt-2 to team %s.  \n ", 
					szRank, their->player.callsign, netGameInfo.m_info.szTeamName [0], netGameInfo.m_info.szTeamName [1]);
		joinMsgIndex.nLines = 5;
		}
	else {
		sprintf (szJoinMsg, " \n  %s wants to join.  \n  Press F6 to accept.  \n ", their->player.callsign);
		joinMsgIndex.nLines = 4;
		}
	joinMsgIndex.pszText = szJoinMsg;
	joinMsgIndex.nId = 1;
	gameData.messages [1].nMessages = 1;
	gameData.messages [1].index = 
	gameData.messages [1].currentMsg = &joinMsgIndex;
	gameData.messages [1].nStartTime = gameStates.app.nSDLTicks [0];
	gameData.messages [1].nEndTime = gameStates.app.nSDLTicks [0] + 5000;
	gameData.messages [1].textBuffer = NULL;
	gameData.messages [1].bmP = NULL;
#endif
	strcpy (networkData.refuse.szPlayer, their->player.callsign);
	networkData.refuse.xTimeLimit = TimerGetApproxSeconds ();   
	networkData.refuse.bThisPlayer = 0;
	networkData.refuse.bWaitForAnswer = 1;
	}
else {      
	if (strcmp (their->player.callsign, networkData.refuse.szPlayer))
		return;
	if (networkData.refuse.bThisPlayer) {
		networkData.refuse.xTimeLimit = 0;
		networkData.refuse.bThisPlayer = 0;
		networkData.refuse.bWaitForAnswer = 0;
		if (IsTeamGame) {
			nNewPlayer = GetNewPlayerNumber (their);
			Assert (networkData.refuse.bTeam == 1 || networkData.refuse.bTeam == 2);        
			if (networkData.refuse.bTeam == 1)      
				netGameInfo.m_info.RemoveTeamPlayer (nNewPlayer);
			else
				netGameInfo.m_info.AddTeamPlayer (nNewPlayer);
			NetworkWelcomePlayer (their);
			NetworkSendNetGameUpdate (); 
			}
		else
			NetworkWelcomePlayer (their);
		return;
		}
	if ((TimerGetApproxSeconds ()) > networkData.refuse.xTimeLimit + REFUSE_INTERVAL) {
		networkData.refuse.xTimeLimit = 0;
		networkData.refuse.bThisPlayer = 0;
		networkData.refuse.bWaitForAnswer = 0;
		if (!strcmp (their->player.callsign, networkData.refuse.szPlayer)) {
			if (gameStates.multi.nGameType >= IPX_GAME)
				NetworkDumpPlayer (
					their->player.network.Network (), 
					their->player.network.Node (), 
					DUMP_DORK);
			}
		return;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkDumpPlayer (uint8_t * server, uint8_t *node, int32_t nReason)
{
	// Inform CPlayerData that he was not chosen for the netgame
	tPlayerSyncData temp;

temp.nType = PID_DUMP;
memcpy (temp.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
temp.player.connected = nReason;
if (gameStates.multi.nGameType >= IPX_GAME)
	SendInternetPlayerSyncData (temp, server, node);
else
	Int3 ();
}

//------------------------------------------------------------------------------

#if DBG

void CMultiplayerData::Connect (int32_t nPlayer, int8_t nStatus) 
{
	static int32_t nDbgPlayer = 1;
	static int32_t nDbgStatus = 1;

if (((nDbgPlayer != -1) || (nDbgStatus != -1)) && ((nDbgPlayer < 0) || (nPlayer == nDbgPlayer)) && ((nDbgStatus < 0) || (nStatus == nDbgStatus)))
	BRP;
players [nPlayer].Connect (nStatus);
}

#endif

//------------------------------------------------------------------------------

