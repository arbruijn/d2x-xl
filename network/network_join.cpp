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

#include "inferno.h"
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

//------------------------------------------------------------------------------

int NetworkFindPlayer (tNetPlayerInfo *playerP)
{
	int	i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (!CmpNetPlayers (NULL, NULL, &netPlayers.players [i].network, &playerP->network))
		return i;         // already got them
return -1;
}

//------------------------------------------------------------------------------

int GetNewPlayerNumber (tSequencePacket *their)
  {
	 int i;

if (gameData.multiplayer.nPlayers < gameData.multiplayer.nMaxPlayers)
	return (gameData.multiplayer.nPlayers);
else {
	// Slots are full but game is open, see if anyone is
	// disconnected and replace the oldest tPlayer with this new one
	int oldestPlayer = -1;
	fix oldestTime = gameStates.app.nSDLTicks;

	Assert (gameData.multiplayer.nPlayers == gameData.multiplayer.nMaxPlayers);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((!gameData.multiplayer.players [i].connected) && (networkData.nLastPacketTime [i] < oldestTime)) {
			oldestTime = networkData.nLastPacketTime [i];
			oldestPlayer = i;
			}
		}
	return (oldestPlayer);
	}
}

//------------------------------------------------------------------------------

int CanJoinNetgame (tNetgameInfo *game, tAllNetPlayersInfo *people)
{
	// Can this tPlayer rejoin a netgame in progress?

	int i, nNumPlayers;

if (game->gameStatus == NETSTAT_STARTING)
   return 1;
if (game->gameStatus != NETSTAT_PLAYING) {
#if 1      
	con_printf (CONDBG, "Error: Can't join because gameStatus !=NETSTAT_PLAYING\n");
#endif
	return 0;
    }

if (game->versionMajor == 0 && D2X_MAJOR>0) {
#if 1      
	con_printf (CONDBG, "Error:Can't join because version majors don't match!\n");
#endif
	return 0;
	}

if (game->versionMajor>0 && D2X_MAJOR == 0) {
#if 1      
	con_printf (CONDBG, "Error:Can't join because version majors2 don't match!\n");
#endif
	return 0;
	}
// Game is in progress, figure out if this guy can re-join it
nNumPlayers = game->nNumPlayers;

if (!(game->gameFlags & NETGAME_FLAG_CLOSED)) {
	// Look for tPlayer that is not connected
	if (game->nConnected == game->nMaxPlayers)
		 return 2;
	if (game->bRefusePlayers)
		 return 3;
	if (game->nNumPlayers < game->nMaxPlayers)
		return 1;
	if (game->nConnected<nNumPlayers)
		return 1;
	}
if (!people) {
	con_printf (CONDBG, "Error! Can't join because people == NULL!\n");
	return 0;
   }
// Search to see if we were already in this closed netgame in progress
for (i = 0; i < nNumPlayers; i++)
	if (!CmpNetPlayers (LOCALPLAYER.callsign, 
							  people->players [i].callsign, 
							  &networkData.thisPlayer.player.network, 
							  &people->players [i].network))
		return 1;
#if 1      
con_printf (CONDBG, "Error: Can't join because at end of list!\n");
#endif
return 0;
}

//------------------------------------------------------------------------------

void NetworkResetSyncStates (void)
{
networkData.nJoining = 0;
}

//------------------------------------------------------------------------------

void NetworkResetObjSync (short nObject)
{
for (short i = 0; i < networkData.nJoining; i++)
	if ((networkData.sync [i].nState == 1) && ((nObject < 0) || NetworkObjnumIsPast (nObject, networkData.sync + i)))
		networkData.sync [i].objs.nCurrent = -1;
}

//------------------------------------------------------------------------------

void NetworkDisconnectPlayer (int nPlayer)
{
	// A tPlayer has disconnected from the net game, take whatever steps are
	// necessary 

if (nPlayer == gameData.multiplayer.nLocalPlayer) {
	Int3 (); // Weird, see Rob
	return;
	}
gameData.multiplayer.players [nPlayer].connected = 0;
KillPlayerSmoke (nPlayer);
gameData.multiplayer.weaponStates [nPlayer].firing [0].nDuration =
gameData.multiplayer.weaponStates [nPlayer].firing [1].nDuration = 0;
KillPlayerBullets (OBJECTS + gameData.multiplayer.players [nPlayer].nObject);
KillGatlingSmoke (OBJECTS + gameData.multiplayer.players [nPlayer].nObject);
netPlayers.players [nPlayer].connected = 0;
for (short i = 0; i < networkData.nJoining; i++)
	if (networkData.sync [i].nPlayer == nPlayer)
		DeleteSyncData (i);
	NetworkResetSyncStates ();
// CreatePlayerAppearanceEffect (&OBJECTS [gameData.multiplayer.players [nPlayer].nObject]);
MultiMakePlayerGhost (nPlayer);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiDisconnect (nPlayer);
MultiStripRobots (nPlayer);
}
	
//------------------------------------------------------------------------------

void NetworkNewPlayer (tSequencePacket *their)
{
	int	nObject;
	int	nPlayer;

nPlayer = their->player.connected;
Assert (nPlayer >= 0);
Assert (nPlayer < gameData.multiplayer.nMaxPlayers);        
nObject = gameData.multiplayer.players [nPlayer].nObject;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiConnect (nPlayer, nPlayer == gameData.multiplayer.nPlayers, their->player.callsign);
memcpy (gameData.multiplayer.players [nPlayer].callsign, their->player.callsign, CALLSIGN_LEN+1);
memcpy (netPlayers.players [nPlayer].callsign, their->player.callsign, CALLSIGN_LEN+1);
ClipRank (reinterpret_cast<char*> (&their->player.rank));
netPlayers.players [nPlayer].rank = their->player.rank;
netPlayers.players [nPlayer].versionMajor = their->player.versionMajor;
netPlayers.players [nPlayer].versionMinor = their->player.versionMinor;
NetworkCheckForOldVersion ((char) nPlayer);

if (gameStates.multi.nGameType >= IPX_GAME) {
	if (*reinterpret_cast<uint*> (their->player.network.ipx.server) != 0)
		IpxGetLocalTarget (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			gameData.multiplayer.players [nPlayer].netAddress);
	else
		memcpy (gameData.multiplayer.players [nPlayer].netAddress, their->player.network.ipx.node, 6);
	}
memcpy (&netPlayers.players [nPlayer].network, &their->player.network, sizeof (tNetworkInfo));
gameData.multiplayer.players [nPlayer].nPacketsGot = 0;
gameData.multiplayer.players [nPlayer].connected = 1;
gameData.multiplayer.players [nPlayer].netKillsTotal = 0;
gameData.multiplayer.players [nPlayer].netKilledTotal = 0;
memset (gameData.multigame.kills.matrix [nPlayer], 0, MAX_PLAYERS * sizeof (short)); 
gameData.multiplayer.players [nPlayer].score = 0;
gameData.multiplayer.players [nPlayer].flags = 0;
gameData.multiplayer.players [nPlayer].nKillGoalCount = 0;
if (nPlayer == gameData.multiplayer.nPlayers) {
	gameData.multiplayer.nPlayers++;
	netGame.nNumPlayers = gameData.multiplayer.nPlayers;
	}
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
ClipRank (reinterpret_cast<char*> (&their->player.rank));
if (gameOpts->multi.bNoRankings)
	HUDInitMessage ("'%s' %s\n", their->player.callsign, TXT_JOINING);
else   
   HUDInitMessage ("%s'%s' %s\n", pszRankStrings [their->player.rank], their->player.callsign, TXT_JOINING);
MultiMakeGhostPlayer (nPlayer);
GetPlayerSpawn (GetRandomPlayerPosition (), OBJECTS + gameData.multiplayer.players [nPlayer].nObject);
MultiSendScore ();
MultiSortKillList ();
//      CreatePlayerAppearanceEffect (&OBJECTS [nObject]);
}

//------------------------------------------------------------------------------

static int FindNetworkPlayer (tSequencePacket *player, ubyte *newAddress)
{
	int	i;
	ubyte anyAddress [6];

if (gameStates.multi.nGameType == UDP_GAME) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, newAddress, extraGameInfo [1].bCheckUDPPort ? 6 : 4) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, player->player.callsign)) {
			memcpy (gameData.multiplayer.players [i].netAddress, newAddress, 6);
			return i;
			}
		}
	memset (&anyAddress, 0xFF, sizeof (newAddress));
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, anyAddress, 6) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, player->player.callsign)) {
			memcpy (gameData.multiplayer.players [i].netAddress, newAddress, 6);
			memcpy (netPlayers.players [i].network.ipx.node, newAddress, 6);
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

static int FindPlayerSlot (tSequencePacket *player)
{
if (netGame.gameFlags & NETGAME_FLAG_CLOSED)	{
	// Slots are open but game is closed
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			player->player.network.ipx.server, 
			player->player.network.ipx.node, 
			DUMP_CLOSED);
	return -1;
	}
if (gameData.multiplayer.nPlayers < gameData.multiplayer.nMaxPlayers) {
	// Add tPlayer in an open slot, game not full yet
	networkData.bPlayerAdded = 1;
	return gameData.multiplayer.nPlayers;
	}
// Slots are full but game is open, see if anyone is
// disconnected and replace the oldest tPlayer with this new one
int oldestPlayer = -1;
fix oldestTime = TimerGetApproxSeconds ();
Assert (gameData.multiplayer.nPlayers == gameData.multiplayer.nMaxPlayers);
for (int i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!gameData.multiplayer.players [i].connected && (networkData.nLastPacketTime [i] < oldestTime)) {
		oldestTime = networkData.nLastPacketTime [i];
		oldestPlayer = i;
		}
	}
if (oldestPlayer == -1) {
	// Everyone is still connected 
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			player->player.network.ipx.server, 
			player->player.network.ipx.node, 
			DUMP_FULL);
	return -1;
	}
networkData.bPlayerAdded = 1;
return oldestPlayer;
}

//------------------------------------------------------------------------------

static short FindJoiningPlayer (tSequencePacket *player)
{
for (short i = 0; i < networkData.nJoining; i++)
	if (!memcmp (&networkData.sync [i].player [1], player, sizeof (*player)))
		return i;
return -1;
}

//------------------------------------------------------------------------------

tNetworkSyncData *FindJoiningPlayer (short nPlayer)
{
for (short i = 0; i < networkData.nJoining; i++)
	if (networkData.sync [i].player [1].player.connected == nPlayer)
		return networkData.sync + i;
return NULL;
}

//------------------------------------------------------------------------------

void DeleteSyncData (short nConnection)
{
if (nConnection < --networkData.nJoining)
	memcpy (networkData.sync + nConnection, networkData.sync + networkData.nJoining, sizeof (tNetworkSyncData));
}

//------------------------------------------------------------------------------

static tNetworkSyncData *AcceptJoinRequest (tSequencePacket *player)
{
// Don't accept new players if we're ending this level.  Its safe to
// ignore since they'll request again later
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
#if 1      
	con_printf (CONDBG, "Ignored request from new tPlayer to join during endgame.\n");
#endif
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			player->player.network.ipx.server, 
			player->player.network.ipx.node, 
			DUMP_ENDLEVEL);
	return NULL; 
	}

if (player->player.connected != gameData.missions.nCurrentLevel) {
#if 1      
	con_printf (CONDBG, "Dumping tPlayer due to old level number.\n");
#endif
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			player->player.network.ipx.server, 
			player->player.network.ipx.node, 
			DUMP_LEVEL);
	return NULL;
	}

tNetworkSyncData *syncP;
short nConnection = FindJoiningPlayer (player);

if (nConnection < 0) {
	if (networkData.nJoining == MAX_JOIN_REQUESTS)
		return NULL;
	syncP = networkData.sync + networkData.nJoining++;
	}
else { //prevent flooding with connection attempts from the same player
	syncP = networkData.sync + nConnection;
	if (gameStates.app.nSDLTicks - syncP->tLastJoined < 2100)
		return NULL;
	syncP->tLastJoined = gameStates.app.nSDLTicks;
	}
return syncP;
}

//------------------------------------------------------------------------------
// Add a tPlayer to a game already in progress

void NetworkWelcomePlayer (tSequencePacket *player)
{
	int					nPlayer;
	ubyte					newAddress [6];
	tNetworkSyncData	*syncP;

networkData.refuse.bWaitForAnswer = 0;
if (FindArg ("-NoMatrixCheat")) {
	if ((player->player.versionMinor & 0x0F) < 3) {
		NetworkDumpPlayer (
			player->player.network.ipx.server, 
			player->player.network.ipx.node, 
			DUMP_DORK);
		return;
		}
	}
if (HoardEquipped ()) {
// If hoard game, and this guy isn't D2 Christmas (v1.2), dump him
	if ((gameData.app.nGameMode & GM_HOARD) && ((player->player.versionMinor & 0x0F) < 2)) {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (
				player->player.network.ipx.server, 
				player->player.network.ipx.node, 
				DUMP_DORK);
		return;
		}
	}
if (!(syncP = AcceptJoinRequest (player)))
	return;
memset (&syncP->player [1], 0, sizeof (tSequencePacket));
networkData.bPlayerAdded = 0;
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (*reinterpret_cast<uint*> (player->player.network.ipx.server) != 0)
		IpxGetLocalTarget (
			player->player.network.ipx.server, 
			player->player.network.ipx.node, 
			newAddress);
	else
		memcpy (newAddress, player->player.network.ipx.node, 6);
	}
if (0 > (nPlayer = FindNetworkPlayer (player, newAddress))) {
	// Player is new to this game
	if (0 > (nPlayer = FindPlayerSlot (player))) {
		DeleteSyncData (syncP - networkData.sync);
		return;
		}
	}
else {
	// Player is reconnecting
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nPlayer);
	networkData.bPlayerAdded = 0;
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s", pszRankStrings [netPlayers.players [nPlayer].rank], 
							 gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	}
if (IsTeamGame)
	ChoseTeam (nPlayer);
gameData.multiplayer.players [nPlayer].nKillGoalCount = 0;
gameData.multiplayer.players [nPlayer].connected = 0;
// Send updated OBJECTS data to the new/returning tPlayer
syncP->player [0] = *player;
syncP->player [1] = *player;
syncP->player [1].player.connected = nPlayer;
syncP->bExtraGameInfo = 0;
syncP->nState = 1;
syncP->objs.nCurrent = -1;
syncP->nExtras = 0;
syncP->timeout = 0;
syncP->tLastJoined = gameStates.app.nSDLTicks;
NetworkDoSyncFrame ();
}

//------------------------------------------------------------------------------

void NetworkAddPlayer (tSequencePacket *player)
{
	tNetPlayerInfo	*npiP;

if (NetworkFindPlayer (&player->player) > -1)
	return;
npiP = netPlayers.players + gameData.multiplayer.nPlayers;
memcpy (&npiP->network, &player->player.network, sizeof (tNetworkInfo));
ClipRank (reinterpret_cast<char*> (&player->player.rank));
memcpy (npiP->callsign, player->player.callsign, CALLSIGN_LEN+1);
npiP->versionMajor = player->player.versionMajor;
npiP->versionMinor = player->player.versionMinor;
npiP->rank = player->player.rank;
npiP->connected = 1;
NetworkCheckForOldVersion ((char) gameData.multiplayer.nPlayers);
gameData.multiplayer.players [gameData.multiplayer.nPlayers].nKillGoalCount = 0;
gameData.multiplayer.players [gameData.multiplayer.nPlayers].connected = 1;
ResetPlayerTimeout (gameData.multiplayer.nPlayers, -1);
gameData.multiplayer.nPlayers++;
netGame.nNumPlayers = gameData.multiplayer.nPlayers;
// Broadcast updated info
NetworkSendGameInfo (NULL);
}

//------------------------------------------------------------------------------

// One of the players decided not to join the game

void NetworkRemovePlayer (tSequencePacket *player)
{
	int i, j, pn = NetworkFindPlayer (&player->player);

if (pn < 0)
	return;

for (i = pn; i < gameData.multiplayer.nPlayers - 1; ) {
	j = i++;
	memcpy (&netPlayers.players [j].network, &netPlayers.players [i].network.ipx.node, 
			  sizeof (tNetworkInfo));
	memcpy (netPlayers.players [j].callsign, netPlayers.players [i].callsign, CALLSIGN_LEN+1);
	netPlayers.players [j].versionMajor = netPlayers.players [i].versionMajor;
	netPlayers.players [j].versionMinor = netPlayers.players [i].versionMinor;
   netPlayers.players [j].rank = netPlayers.players [i].rank;
	ClipRank (reinterpret_cast<char*> (&netPlayers.players [j].rank));
   NetworkCheckForOldVersion ((char) i);
	}
gameData.multiplayer.nPlayers--;
netGame.nNumPlayers = gameData.multiplayer.nPlayers;
// Broadcast new info
NetworkSendGameInfo (NULL);
}

//------------------------------------------------------------------------------

void DoRefuseStuff (tSequencePacket *their)
{
  int				i, nNewPlayer;

  static tTextIndex	joinMsgIndex;
  static char			szJoinMsg [200];

ClipRank (reinterpret_cast<char*> (&their->player.rank));

for (i = 0; i < MAX_PLAYERS; i++)
	if (!strcmp (their->player.callsign, gameData.multiplayer.players [i].callsign)) {
		NetworkWelcomePlayer (their);
		return;
		}
if (FindPlayerInBanList (their->player.callsign))
	return;
if (!networkData.refuse.bWaitForAnswer) {
	DigiPlaySample (SOUND_HUD_JOIN_REQUEST, F1_0*2);           
#if 0
	if (IsTeamGame) {
		if (gameOpts->multi.bNoRankings)
			HUDInitMessage ("%s joining", their->player.callsign);
		else
			HUDInitMessage ("%s %s wants to join", 
								 pszRankStrings [their->player.rank], their->player.callsign);
		HUDInitMessage ("Alt-1 assigns to team %s. Alt-2 to team %s", 
							 their->player.callsign, netGame.szTeamName [0], netGame.szTeamName [1]);
		}               
	else    
		HUDInitMessage (TXT_JOIN_ACCEPT, their->player.callsign);
#else
	if (IsTeamGame) {
		char szRank [20];

		if (gameOpts->multi.bNoRankings)
			*szRank = '\0';
		else
			sprintf (szRank, "%s ", pszRankStrings [their->player.rank]);
		sprintf (szJoinMsg, " \n  %s%s wants to join.  \n  Alt-1 assigns to team %s.  \n  Alt-2 to team %s.  \n ", 
					szRank, their->player.callsign, netGame.szTeamName [0], netGame.szTeamName [1]);
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
	gameData.messages [1].nStartTime = gameStates.app.nSDLTicks;
	gameData.messages [1].nEndTime = gameStates.app.nSDLTicks + 5000;
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
				netGame.teamVector &= ~(1 << nNewPlayer);
			else
				netGame.teamVector |= (1 << nNewPlayer);
			NetworkWelcomePlayer (their);
			NetworkSendNetgameUpdate (); 
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
					their->player.network.ipx.server, 
					their->player.network.ipx.node, 
					DUMP_DORK);
			}
		return;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkDumpPlayer (ubyte * server, ubyte *node, int nReason)
{
	// Inform tPlayer that he was not chosen for the netgame
	tSequencePacket temp;

temp.nType = PID_DUMP;
memcpy (temp.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
temp.player.connected = nReason;
if (gameStates.multi.nGameType >= IPX_GAME)
	SendInternetSequencePacket (temp, server, node);
else
	Int3 ();
}

//------------------------------------------------------------------------------

