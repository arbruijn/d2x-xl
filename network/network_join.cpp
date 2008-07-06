/* $Id: network.c, v 1.24 2003/10/12 09:38:48 btb Exp $ */
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

#ifdef RCS
static char rcsid [] = "$Id: network.c, v 1.24 2003/10/12 09:38:48 btb Exp $";
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
#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "inferno.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "mono.h"
#include "ipx.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "objsmoke.h"
#include "error.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "loadgame.h"
#include "fireball.h"
#include "network.h"
#include "network_lib.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "reactor.h"
#include "powerup.h"
#include "menu.h"
#include "sounds.h"
#include "text.h"
#include "highscores.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "switch.h"
#include "automap.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "playsave.h"
#include "cfile.h"
#include "autodl.h"
#include "tracker.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gameseg.h"
#include "hudmsg.h"
#include "vers_id.h"
#include "netmenu.h"
#include "banlist.h"
#include "collide.h"
#include "ipx.h"
#ifdef _WIN32
#	include "win32/include/ipx_udp.h"
#	include "win32/include/ipx_drv.h"
#else
#	include "linux/include/ipx_udp.h"
#	include "linux/include/ipx_drv.h"
#endif

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
	int oldest_player = -1;
	fix oldestTime = (fix) SDL_GetTicks ();

	Assert (gameData.multiplayer.nPlayers == gameData.multiplayer.nMaxPlayers);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((!gameData.multiplayer.players [i].connected) && (networkData.nLastPacketTime [i] < oldestTime)) {
			oldestTime = networkData.nLastPacketTime [i];
			oldest_player = i;
			}
		}
	return (oldest_player);
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
							  &networkData.mySeq.player.network, 
							  &people->players [i].network))
		return 1;
#if 1      
con_printf (CONDBG, "Error: Can't join because at end of list!\n");
#endif
return 0;
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
netPlayers.players [nPlayer].connected = 0;
if (networkData.bVerifyPlayerJoined == nPlayer)
	networkData.bVerifyPlayerJoined = -1;
//      CreatePlayerAppearanceEffect (&OBJECTS [gameData.multiplayer.players [nPlayer].nObject]);
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
ClipRank ((char *) &their->player.rank);
netPlayers.players [nPlayer].rank = their->player.rank;
netPlayers.players [nPlayer].versionMajor = their->player.versionMajor;
netPlayers.players [nPlayer].versionMinor = their->player.versionMinor;
NetworkCheckForOldVersion ((char) nPlayer);

if (gameStates.multi.nGameType >= IPX_GAME) {
	if ((* (uint *)their->player.network.ipx.server) != 0)
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
memset (gameData.multigame.kills.matrix [nPlayer], 0, MAX_PLAYERS*sizeof (short)); 
gameData.multiplayer.players [nPlayer].score = 0;
gameData.multiplayer.players [nPlayer].flags = 0;
gameData.multiplayer.players [nPlayer].nKillGoalCount=0;
if (nPlayer == gameData.multiplayer.nPlayers) {
	gameData.multiplayer.nPlayers++;
	netGame.nNumPlayers = gameData.multiplayer.nPlayers;
	}
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
ClipRank ((char *) &their->player.rank);
if (gameOpts->multi.bNoRankings)
	HUDInitMessage ("'%s' %s\n", their->player.callsign, TXT_JOINING);
else   
   HUDInitMessage ("%s'%s' %s\n", pszRankStrings [their->player.rank], their->player.callsign, TXT_JOINING);
MultiMakeGhostPlayer (nPlayer);
MultiSendScore ();
MultiSortKillList ();
//      CreatePlayerAppearanceEffect (&OBJECTS [nObject]);
}

//------------------------------------------------------------------------------

void NetworkWelcomePlayer (tSequencePacket *their)
{
	// Add a tPlayer to a game already in progress
	ubyte newAddress [6], anyAddress [6];
	int nPlayer;
	int i;

networkData.refuse.bWaitForAnswer = 0;
if (FindArg ("-NoMatrixCheat")) {
	if ((their->player.versionMinor & 0x0F) < 3) {
		NetworkDumpPlayer (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			DUMP_DORK);
		return;
		}
	}
if (HoardEquipped ()) {
// If hoard game, and this guy isn't D2 Christmas (v1.2), dump him
	if ((gameData.app.nGameMode & GM_HOARD) && ((their->player.versionMinor & 0x0F)<2)) {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (
				their->player.network.ipx.server, 
				their->player.network.ipx.node, 
				DUMP_DORK);
		return;
		}
	}
// Don't accept new players if we're ending this level.  Its safe to
// ignore since they'll request again later
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
#if 1      
	con_printf (CONDBG, "Ignored request from new tPlayer to join during endgame.\n");
#endif
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			DUMP_ENDLEVEL);
	return; 
	}
if (memcmp (&networkData.playerRejoining, their, sizeof (networkData.playerRejoining))) {
	if (networkData.nSyncState)
		return;
	if (!networkData.nSyncState && networkData.nSyncExtras && (networkData.bVerifyPlayerJoined != -1))
		return;
	}
#if 0
if (networkData.nSyncState || networkData.nSyncExtras) {
	// Ignore silently, we're already responding to someone and we can't
	// do more than one person at a time.  If we don't dump them they will
	// re-request in a few seconds.
	return;
}
#endif
if (their->player.connected != gameData.missions.nCurrentLevel) {
#if 1      
	con_printf (CONDBG, "Dumping tPlayer due to old level number.\n");
#endif
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			DUMP_LEVEL);
	return;
	}
nPlayer = -1;
memset (&networkData.playerRejoining, 0, sizeof (tSequencePacket));
networkData.bPlayerAdded = 0;
if (gameStates.multi.nGameType >= IPX_GAME) {
	if ((*(uint *) their->player.network.ipx.server) != 0)
		IpxGetLocalTarget (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			newAddress);
	else
		memcpy (newAddress, their->player.network.ipx.node, 6);
	}
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (gameStates.multi.nGameType == IPX_GAME) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, newAddress, 6) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, their->player.callsign)) {
			nPlayer = i;
			break;
			}
		}
	else if (gameStates.multi.nGameType == UDP_GAME) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, newAddress, extraGameInfo [1].bCheckUDPPort ? 6 : 4) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, their->player.callsign)) {
			nPlayer = i;
			memcpy (gameData.multiplayer.players [i].netAddress, newAddress, 6);
			break;
			}
		}
	}
if ((nPlayer == -1) && (gameStates.multi.nGameType == UDP_GAME)) {
	memset (&anyAddress, 0xFF, sizeof (newAddress));
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, anyAddress, 6) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, their->player.callsign)) {
			nPlayer = i;
			memcpy (gameData.multiplayer.players [i].netAddress, newAddress, 6);
			memcpy (netPlayers.players [i].network.ipx.node, newAddress, 6);
			break;
			}
		}
	}
if (nPlayer == -1) {
	// Player is new to this game
	if (!(netGame.gameFlags & NETGAME_FLAG_CLOSED) && 
		  (gameData.multiplayer.nPlayers < gameData.multiplayer.nMaxPlayers)) {
		// Add tPlayer in an open slot, game not full yet
		nPlayer = gameData.multiplayer.nPlayers;
		if (IsTeamGame)
			ChoseTeam (nPlayer);
		networkData.bPlayerAdded = 1;
		}
	else if (netGame.gameFlags & NETGAME_FLAG_CLOSED)	{
		// Slots are open but game is closed
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (
				their->player.network.ipx.server, 
				their->player.network.ipx.node, 
				DUMP_CLOSED);
		return;
		}
	else {
		// Slots are full but game is open, see if anyone is
		// disconnected and replace the oldest tPlayer with this new one
		int oldest_player = -1;
		fix oldestTime = TimerGetApproxSeconds ();
		Assert (gameData.multiplayer.nPlayers == gameData.multiplayer.nMaxPlayers);
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if ((!gameData.multiplayer.players [i].connected) && 
				 (networkData.nLastPacketTime [i] < oldestTime)) {
				oldestTime = networkData.nLastPacketTime [i];
				oldest_player = i;
				}
			}
		if (oldest_player == -1) {
			// Everyone is still connected 
			if (gameStates.multi.nGameType >= IPX_GAME)
				NetworkDumpPlayer (
					their->player.network.ipx.server, 
					their->player.network.ipx.node, 
					DUMP_FULL);
			return;
			}
		else {       
			// Found a slot!
			nPlayer = oldest_player;
			networkData.bPlayerAdded = 1;
			}
		}
	}
else {
	// Player is reconnecting
#if 0	
	if (gameData.multiplayer.players [nPlayer].connected)	{
#if 1      
		con_printf (CONDBG, "Extra REQUEST from tPlayer ignored.\n");
#endif
		return;
		}
#endif
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
gameData.multiplayer.players [nPlayer].nKillGoalCount = 0;
gameData.multiplayer.players [nPlayer].connected = 0;
// Send updated OBJECTS data to the new/returning tPlayer
networkData.playerRejoining = *their;
networkData.playerRejoining.player.connected = nPlayer;
networkData.bSyncExtraGameInfo = 0;
networkData.nSyncState = 1;
networkData.nSentObjs = -1;
networkData.nSyncExtras = 0;
networkData.toSyncFrame = 0;
networkData.joinSeq = *their;
NetworkDoSyncFrame ();
}

//------------------------------------------------------------------------------

void NetworkAddPlayer (tSequencePacket *seqP)
{
	tNetPlayerInfo	*playerP;

if (NetworkFindPlayer (&seqP->player) > -1)
	return;
playerP = netPlayers.players + gameData.multiplayer.nPlayers;
memcpy (&playerP->network, &seqP->player.network, sizeof (tNetworkInfo));
ClipRank ((char *) &seqP->player.rank);
memcpy (playerP->callsign, seqP->player.callsign, CALLSIGN_LEN+1);
playerP->versionMajor = seqP->player.versionMajor;
playerP->versionMinor = seqP->player.versionMinor;
playerP->rank = seqP->player.rank;
playerP->connected = 1;
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

void NetworkRemovePlayer (tSequencePacket *seqP)
{
	int i, j, pn = NetworkFindPlayer (&seqP->player);

if (pn < 0)
	return;

for (i = pn; i < gameData.multiplayer.nPlayers - 1; ) {
	j = i++;
	memcpy (&netPlayers.players [j].network, &netPlayers.players [i].network.ipx.node, 
			  sizeof (tNetworkInfo));
	memcpy (netPlayers.players [j].callsign, netPlayers.players [i].callsign, CALLSIGN_LEN+1);
	netPlayers.players [j].versionMajor = netPlayers.players [i].versionMajor;
	netPlayers.players [j].versionMinor = netPlayers.players [i].versionMinor;
   netPlayers.players [j].rank=netPlayers.players [i].rank;
	ClipRank ((char *) &netPlayers.players [j].rank);
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

ClipRank ((char *) &their->player.rank);

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
							 their->player.callsign, netGame.team_name [0], netGame.team_name [1]);
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
					szRank, their->player.callsign, netGame.team_name [0], netGame.team_name [1]);
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

