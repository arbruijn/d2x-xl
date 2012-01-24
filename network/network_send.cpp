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
#include "timer.h"
#include "byteswap.h"
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "autodl.h"
#include "tracker.h"
#include "monsterball.h"
#include "marker.h"
#include "console.h"

//------------------------------------------------------------------------------

void NetworkSendDoorUpdates (int nPlayer)
{
	// Send door status when new player joins

	int i;
	CWall *wallP;
   
//   Assert (nPlayer>-1 && nPlayer<gameData.multiplayer.nPlayers);
for (i = 0, wallP = WALLS.Buffer (); i < gameData.walls.nWalls; i++, wallP++) {
   if ((wallP->nType == WALL_DOOR) && 
		 ((wallP->state == WALL_DOOR_OPENING) || 
		  (wallP->state == WALL_DOOR_WAITING) || 
		  (wallP->state == WALL_DOOR_OPEN)))
		MultiSendDoorOpenSpecific (nPlayer, wallP->nSegment, wallP->nSide, wallP->flags);
	else if ((wallP->nType == WALL_BLASTABLE) && (wallP->flags & WALL_BLASTED))
		MultiSendDoorOpenSpecific (nPlayer, wallP->nSegment, wallP->nSide, wallP->flags);
	else if ((wallP->nType == WALL_BLASTABLE) && (wallP->hps != WALL_HPS))
		MultiSendHostageDoorStatus (i);
	else
		MultiSendWallStatusSpecific (nPlayer, i, wallP->nType, wallP->flags, wallP->state);
	}
}

//------------------------------------------------------------------------------

void NetworkSendMarkers (void)
 {
  // send marker positions/text to new player
  int i, j;

for (i = j = 0; i < gameData.multiplayer.nPlayers; i++, j++) {
   if (markerManager.Objects (j) != -1)
		MultiSendDropMarker (i, markerManager.Position (j), 0, markerManager.Message (j));
   if (markerManager.Objects (++j) != -1)
		MultiSendDropMarker (i, markerManager.Position (j), 0, markerManager.Message (j));
	}
 }

//------------------------------------------------------------------------------

void NetworkSendRejoinSync (int nPlayer, tNetworkSyncData *syncP)
{
	int i, j;

CONNECT (nPlayer, CONNECT_PLAYING); // connect the new guy
ResetPlayerTimeout (nPlayer, -1);
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
	// Endlevel started before we finished sending the goods, we'll
	// have to stop and try again after the level.

	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			syncP->player [1].player.network.Server (), 
			syncP->player [1].player.network.Node (), 
			DUMP_ENDLEVEL);
	syncP->nState = 0; 
	syncP->nExtras = 0;
	return;
	}
if (networkData.bPlayerAdded) {
	syncP->player [1].nType = PID_ADDPLAYER;
	syncP->player [1].player.connected = nPlayer;
	NetworkNewPlayer (&syncP->player [1]);

	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((i != nPlayer) && 
			 (i != N_LOCALPLAYER) && 
			 gameData.multiplayer.players [i].connected &&
			 (gameStates.multi.nGameType >= IPX_GAME)) {
			SendSequencePacket (
				syncP->player [1], 
				netPlayers [0].m_info.players [i].network.Server (), 
				netPlayers [0].m_info.players [i].network.Node (), 
				gameData.multiplayer.players [i].netAddress);
			}
		}       
	}
// Send sync packet to the new guy
NetworkUpdateNetGame ();

// Fill in the kill list
for (j = 0; j < MAX_PLAYERS; j++) {
	for (i = 0; i < MAX_PLAYERS; i++)
		*netGame.Kills (j, i) = gameData.multigame.score.matrix [j][i];
	*netGame.Killed (j) = gameData.multiplayer.players [j].netKilledTotal;
	*netGame.PlayerKills (j) = gameData.multiplayer.players [j].netKillsTotal;
	*netGame.PlayerScore (j) = gameData.multiplayer.players [j].score;
	}       
netGame.SetLevelTime (LOCALPLAYER.timeLevel);
netGame.SetMonitorVector (NetworkCreateMonitorVector ());
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (syncP->player [1].player.network.Server (), syncP->player [1].player.network.Node ());
	SendNetPlayersPacket (syncP->player [1].player.network.Server (), syncP->player [1].player.network.Node ());
	MultiSendMonsterball (1, 1);
	}
}

//------------------------------------------------------------------------------

#if 0

void ResendSyncDueToPacketLoss (void)
{
   int i, j;

NetworkUpdateNetGame ();
// Fill in the kill list
for (j = 0; j < MAX_PLAYERS; j++) {
	for (i = 0; i < MAX_PLAYERS; i++)
		*netGame.Kills () [j][i] = gameData.multigame.score.matrix [j][i];
	*netGame.Killed () [j] = gameData.multiplayer.players [j].netKilledTotal;
	*netGame.PlayerKills () [j] = gameData.multiplayer.players [j].netKillsTotal;
	*netGame.PlayerScore () [j] = gameData.multiplayer.players [j].score;
	}       
netGame.LevelTime () = LOCALPLAYER.timeLevel;
netGame.MonitorVector () = NetworkCreateMonitorVector ();
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (
		networkData.sync [0].player [1].player.network.Server (), 
		networkData.sync [0].player [1].player.network.Node ());
	SendNetPlayersPacket (
		networkData.sync [0].player [1].player.network.Server (), 
		networkData.sync [0].player [1].player.network.Node ());
	}
}

#endif

//------------------------------------------------------------------------------

#if DBG

void TestXMLInfoRequest (ubyte* serverAddress)
{
#if 0 //DBG
gameStates.multi.bTrackerCall = 2;
IPXSendInternetPacketData((ubyte *) "FDescent Game Info Request", strlen ("FDescent Game Info Request") + 1, 
								  serverAddress, serverAddress + 4);
gameStates.multi.bTrackerCall = 0;
#endif
#if 1 //DBG
gameStates.multi.bTrackerCall = 2;
IPXSendInternetPacketData((ubyte *) "GDescent Game Status Request", strlen ("GDescent Game Status Request") + 1, 
								  serverAddress, serverAddress + 4);
gameStates.multi.bTrackerCall = 0;
#endif
}

#endif

//------------------------------------------------------------------------------
// Send a broadcast request for game info

int NetworkSendGameListRequest (int bAutoLaunch)
{
	tSequencePacket me;

#if DBG
memset (&me, 0, sizeof (me));
#endif
me.nType = PID_GAME_LIST;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.Node (), IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.Server (), IpxGetMyServerAddress (), 4);
	if (gameStates.multi.nGameType != UDP_GAME) 
		SendBroadcastSequencePacket (me);
	else {
		console.printf (0, "looking for netgames\n");
		ubyte serverAddress [10];
		if (tracker.m_bUse && !bAutoLaunch) {
			if (!tracker.RequestServerList ())
				return 0;
			for (int i = 0; tracker.GetServerFromList (i, serverAddress); i++) {
				SendInternetSequencePacket (me, serverAddress, serverAddress + 4);
#if DBG
				TestXMLInfoRequest (serverAddress);
#endif
				}
			}
		else {
			SendInternetSequencePacket (me, networkData.serverAddress, networkData.serverAddress + 4);
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void NetworkSendAllInfoRequest (char nType, int nSecurity)
{
	// Send a broadcast request for game info
	tSequencePacket me;

me.nSecurity = nSecurity;
me.nType = nType;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.Node (), IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.Server (), IpxGetMyServerAddress (), 4);
	SendBroadcastSequencePacket (me);
	}
}

//------------------------------------------------------------------------------

void NetworkSendEndLevelSub (int nPlayer)
{
	CEndLevelInfo end;
	int i;

	// Send an endlevel packet for a player
*end.Type () = PID_ENDLEVEL;
*end.Player () = nPlayer;
*end.Connected () = gameData.multiplayer.players [nPlayer].connected;
*end.Kills () = INTEL_SHORT (gameData.multiplayer.players [nPlayer].netKillsTotal);
*end.Killed () = INTEL_SHORT (gameData.multiplayer.players [nPlayer].netKilledTotal);
memcpy (end.ScoreMatrix (), gameData.multigame.score.matrix [nPlayer], MAX_NUM_NET_PLAYERS * sizeof (short));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
for (i = 0; i < MAX_PLAYERS; i++)
	for (int j = 0; j < MAX_PLAYERS; j++)
		*end.ScoreMatrix (i, j) = INTEL_SHORT (*end.ScoreMatrix (i, j));
#endif
if (gameData.multiplayer.players [nPlayer].connected == 1) {// Still playing
	Assert (gameData.reactor.bDestroyed);
	*end.SecondsLeft () = gameData.reactor.countdown.nSecsLeft;
	}
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {       
	if ((i != N_LOCALPLAYER) && (i != nPlayer) && (gameData.multiplayer.players [i].connected)) {
		if (gameData.multiplayer.players [i].connected == 1)
			NetworkSendEndLevelShortSub (nPlayer, i);
		else if (gameStates.multi.nGameType >= IPX_GAME)
			IPXSendPacketData (
				reinterpret_cast<ubyte*> (&end), sizeof (tEndLevelInfo), 
				netPlayers [0].m_info.players [i].network.Server (), 
				netPlayers [0].m_info.players [i].network.Node (), gameData.multiplayer.players [i].netAddress);
		}
	}
}

//------------------------------------------------------------------------------

/* Send an updated endlevel status to other hosts */
void NetworkSendEndLevelPacket (void)
{
NetworkSendEndLevelSub (N_LOCALPLAYER);
}

//------------------------------------------------------------------------------
// Tell player nDestPlayer that player nSrcPlayer is out of the level

void NetworkSendEndLevelShortSub (int nSrcPlayer, int nDestPlayer)
{
if (gameStates.multi.nGameType < IPX_GAME)
	return;
if (!gameData.multiplayer.players [nDestPlayer].connected)
	return;
if (nDestPlayer == N_LOCALPLAYER)
	return;
if (nDestPlayer == nSrcPlayer)
	return;
#if 1
if ((gameStates.multi.nGameType == UDP_GAME) && (nSrcPlayer != N_LOCALPLAYER) && !IAmGameHost ())
	return;
#endif

	tEndLevelInfoShort eli;

eli.nType = PID_ENDLEVEL_SHORT;
eli.nPlayer = nSrcPlayer;
eli.connected = gameData.multiplayer.players [nSrcPlayer].connected;
eli.secondsLeft = gameData.reactor.countdown.nSecsLeft;
IPXSendPacketData (
	reinterpret_cast<ubyte*> (&eli), sizeof (tEndLevelInfoShort), 
	netPlayers [0].m_info.players [nDestPlayer].network.Server (), 
	netPlayers [0].m_info.players [nDestPlayer].network.Node (), gameData.multiplayer.players [nDestPlayer].netAddress);
}

//------------------------------------------------------------------------------

void NetworkSendGameInfo (tSequencePacket *their)
{
	// Send game info to someone who requested it

	char oldType, oldStatus;
   fix timevar;
   int i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.m_info.nType;
oldStatus = netGame.m_info.gameStatus;
netGame.m_info.nType = PID_GAME_INFO;
netPlayers [0].m_info.nType = PID_PLAYERSINFO;
netPlayers [0].m_info.nSecurity = netGame.m_info.nSecurity;
netGame.m_info.versionMajor = D2X_MAJOR;
netGame.m_info.versionMinor = D2X_MINOR;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.m_info.gameStatus = NETSTAT_ENDLEVEL;
if ((timevar = I2X (netGame.GetPlayTimeAllowed () * 5 * 60))) {
	i = X2I (timevar - gameStates.app.xThisLevelTime);
	if (i < 30)
		netGame.m_info.gameStatus = NETSTAT_ENDLEVEL;
	}       
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!their) {
		SendBroadcastFullNetGamePacket ();
		SendBroadcastNetPlayersPacket ();
		}
	else {
		SendInternetFullNetGamePacket (their->player.network.Server (), their->player.network.Node ());
		SendNetPlayersPacket (their->player.network.Server (), their->player.network.Node ());
		}
	}
netGame.m_info.nType = oldType;
netGame.m_info.gameStatus = oldStatus;
//	if ((gameData.app.nGameMode & GM_ENTROPY) || extraGameInfo [0].bEnhancedCTF)
//make half-way sure the client gets this data ...
NetworkSendExtraGameInfo (their);
MultiSendMonsterball (1, 1);
}       

//------------------------------------------------------------------------------

void NetworkSendExtraGameInfo (tSequencePacket *their)
{
	tExtraGameInfo	egi1Save = extraGameInfo [1];

extraGameInfo [1] = extraGameInfo [0];
extraGameInfo [1].bCompetition = egi1Save.bCompetition;
extraGameInfo [1].bMouseLook = egi1Save.bMouseLook;
extraGameInfo [1].bFastPitch = egi1Save.bFastPitch;
extraGameInfo [1].bTeamDoors = egi1Save.bTeamDoors;
extraGameInfo [1].bEnableCheats = egi1Save.bEnableCheats;
extraGameInfo [1].bTargetIndicators = egi1Save.bTargetIndicators;
extraGameInfo [1].bDamageIndicators = egi1Save.bDamageIndicators;
extraGameInfo [1].bFriendlyIndicators = egi1Save.bFriendlyIndicators;
extraGameInfo [1].bMslLockIndicators = egi1Save.bMslLockIndicators;
extraGameInfo [1].bTagOnlyHitObjs = egi1Save.bTagOnlyHitObjs;
extraGameInfo [1].bTowFlags = egi1Save.bTowFlags;
extraGameInfo [1].bDualMissileLaunch = egi1Save.bDualMissileLaunch;
extraGameInfo [1].bDisableReactor = egi1Save.bDisableReactor;
extraGameInfo [1].bRotateLevels = egi1Save.bRotateLevels;
extraGameInfo [1].bDarkness = egi1Save.bDarkness;
extraGameInfo [1].headlight.bAvailable = egi1Save.headlight.bAvailable;
extraGameInfo [1].bPowerupLights = egi1Save.bPowerupLights;
extraGameInfo [1].bBrightObjects = egi1Save.bBrightObjects;
extraGameInfo [1].bAllowCustomWeapons = egi1Save.bAllowCustomWeapons;
extraGameInfo [1].nSpotSize = egi1Save.nSpotSize;
extraGameInfo [1].nCoopPenalty = egi1Save.nCoopPenalty;
extraGameInfo [1].bRadarEnabled = ((netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP) != 0);
extraGameInfo [1].bWiggle = 1;
extraGameInfo [1].nType = PID_EXTRA_GAMEINFO;
memcpy (extraGameInfo [1].szGameName, mpParams.szGameName, sizeof (mpParams.szGameName));
extraGameInfo [1].nSecurity = netGame.m_info.nSecurity;
gameStates.app.bHaveExtraGameInfo [1] = 1;
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!their)
		SendBroadcastExtraGameInfoPacket ();
	else
		SendInternetExtraGameInfoPacket (their->player.network.Server (), their->player.network.Node ());
	} 
SetMonsterballForces ();
MultiSendPlayerWeapons ();
}

//------------------------------------------------------------------------------

void NetworkSendXMLGameInfo (void)
{
if (IAmGameHost ()) {
	gameStates.multi.bTrackerCall = 2;
	char* szInfo = XMLGameInfo ();
	SendInternetXMLGameInfoPacket (szInfo, networkData.packetSource.src_network, networkData.packetSource.src_node);
	gameStates.multi.bTrackerCall = 0;
	}
}

//------------------------------------------------------------------------------

void NetworkSendLiteInfo (tSequencePacket *their)
{
	// Send game info to someone who requested it
	char oldType, oldStatus;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.m_info.nType;
oldStatus = netGame.m_info.gameStatus;
netGame.m_info.nType = PID_LITE_INFO;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.m_info.gameStatus = NETSTAT_ENDLEVEL;
// If hoard mode, make this game look closed even if it isn't
if (HoardEquipped ()) {
	if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
		char oldStatus = netGame.m_info.gameStatus;
		netGame.m_info.gameStatus = NETSTAT_ENDLEVEL;
		netGame.m_info.gameMode = NETGAME_CAPTURE_FLAG;
		if (oldStatus == NETSTAT_ENDLEVEL)
			netGame.m_info.gameFlags |= NETGAME_FLAG_REALLY_ENDLEVEL;
		if (oldStatus == NETSTAT_STARTING)
			netGame.m_info.gameFlags |= NETGAME_FLAG_REALLY_FORMING;
		}
	}
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!their)
		SendBroadcastLiteNetGamePacket ();
	else
		SendInternetLiteNetGamePacket (their->player.network.Server (), their->player.network.Node ());
	}  
//  Restore the pre-hoard mode
if (HoardEquipped ()) {
	if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL)) {
		if (gameData.app.nGameMode & GM_ENTROPY)
 			netGame.m_info.gameMode = NETGAME_ENTROPY;
		else if (gameData.app.nGameMode & GM_MONSTERBALL)
 			netGame.m_info.gameMode = NETGAME_MONSTERBALL;
		else if (gameData.app.nGameMode & GM_TEAM)
 			netGame.m_info.gameMode = NETGAME_TEAM_HOARD;
		else
 			netGame.m_info.gameMode = NETGAME_HOARD;
		netGame.m_info.gameFlags &= ~(NETGAME_FLAG_REALLY_ENDLEVEL | NETGAME_FLAG_REALLY_FORMING | NETGAME_FLAG_TEAM_HOARD);
		}
	}
netGame.m_info.nType = oldType;
netGame.m_info.gameStatus = oldStatus;
NetworkSendExtraGameInfo (their);
}       

//------------------------------------------------------------------------------

/* Send game info to all players in this game */
void NetworkSendNetGameUpdate (void)
{
	char	oldType, oldStatus, szIP [30];
	int	i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.m_info.nType;
oldStatus = netGame.m_info.gameStatus;
netGame.m_info.nType = PID_GAME_UPDATE;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.m_info.gameStatus = NETSTAT_ENDLEVEL;
PrintLog (1, "sending netgame update:\n");
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected) && (i != N_LOCALPLAYER)) {
		if (gameStates.multi.nGameType >= IPX_GAME) {
			PrintLog (1, "%s (%s)\n", netPlayers [0].m_info.players [i].callsign, 
				iptos (szIP, reinterpret_cast<char*> (netPlayers [0].m_info.players [i].network.Node ())));
			SendLiteNetGamePacket (
				netPlayers [0].m_info.players [i].network.Server (), 
				netPlayers [0].m_info.players [i].network.Node (), 
				gameData.multiplayer.players [i].netAddress);
			}
		}
	}
netGame.m_info.nType = oldType;
netGame.m_info.gameStatus = oldStatus;
}       
			  
//------------------------------------------------------------------------------

int NetworkSendRequest (void)
{
	// Send a request to join a game 'netGame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

if (netGame.m_info.nNumPlayers < 1)
	return 1;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	if (netPlayers [0].m_info.players [i].connected)
	   break;
Assert (i < MAX_NUM_NET_PLAYERS);
networkData.thisPlayer.nType = PID_REQUEST;
networkData.thisPlayer.player.connected = missionManager.nCurrentLevel;
networkData.nJoinState = 0;
networkData.bHaveSync = 0;
networkData.sync [0].objs.nFrame = 0;
networkData.sync [0].objs.missingFrames.nFrame = 0;
networkData.bTraceFrames = 1;
ResetWalls (); // may have been changed by players transmitting game state changes like doors opening or exploding etc.
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetSequencePacket (networkData.thisPlayer, netPlayers [0].m_info.players [i].network.Server (), netPlayers [0].m_info.players [i].network.Node ());
	}
return i;
}

//------------------------------------------------------------------------------

void NetworkSendSync (void)
{
	int i;

d_srand (gameStates.app.nRandSeed = TimerGetFixedSeconds ());
	// Randomize their starting locations...
for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
	if (gameData.multiplayer.players [i].connected)
		CONNECT (i, CONNECT_PLAYING); // Get rid of endlevel connect statuses
if (IsCoopGame) {
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
		*netGame.Locations (i) = i;
	}
else {	// randomize player positions
	int h, j = gameData.multiplayer.nPlayerPositions, posTable [MAX_PLAYERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,13, 14, 15};

	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		h = RandShort () % j;	// compute random table index
		*netGame.Locations (i) = posTable [h];	// pick position using random index
		if (h < --j)
			posTable [h] = posTable [j];	// remove picked position from position table
		}
	}
// Push current data into the sync packet
NetworkUpdateNetGame ();
netGame.m_info.gameStatus = NETSTAT_PLAYING;
netGame.m_info.nType = PID_SYNC;
netGame.SetSegmentCheckSum (networkData.nSegmentCheckSum);
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((!gameData.multiplayer.players [i].connected) || (i == N_LOCALPLAYER))
		continue;
	if (gameStates.multi.nGameType >= IPX_GAME) {
	// Send several times, extras will be ignored
		SendInternetFullNetGamePacket (netPlayers [0].m_info.players [i].network.Server (), netPlayers [0].m_info.players [i].network.Node ());
		SendNetPlayersPacket (netPlayers [0].m_info.players [i].network.Server (), netPlayers [0].m_info.players [i].network.Node ());
		}
	}
NetworkProcessSyncPacket (&netGame, 1); // Read it myself, as if I had sent it
}

//------------------------------------------------------------------------------

void NetworkSendData (ubyte * buf, int len, int bUrgent)
{
	int	bD2XData;

#ifdef NETPROFILING
TTSent [buf [0]]++;  
fprintf (SendLogFile, "Packet nType: %d Len:%d Urgent:%d TT=%d\n", buf [0], len, bUrgent, TTSent [buf [0]]);
fflush (SendLogFile);
#endif
   
if (gameStates.app.bEndLevelSequence)
	return;
if (!networkData.bSyncPackInited) {
	networkData.bSyncPackInited = 1;
	memset (&networkData.syncPack, 0, sizeof (tFrameInfoLong));
	}
if ((networkData.syncPack.dataSize + len) > networkData.nMaxXDataSize) {
	NetworkDoFrame (1, 0);
	if (networkData.syncPack.dataSize != 0) {
#if 0			
		console.printf (CON_DBG, "%d bytes were added to data by NetworkDoFrame!\n", networkData.syncPack.dataSize);
#endif
		Int3 ();
	}
#if 0
	console.printf (CON_DBG, "Packet overflow, sending additional packet, nType %d len %d.\n", buf [0], len);
#endif
	}
// for IPX game, separate legacy and D2X message to avoid non D2X-XL participants losing data
// because they do not know the D2X data and hence cannot determine its length, thus getting out
// of sync when decompositing a multi-data packet.
if (gameStates.multi.nGameType == IPX_GAME) {
	bD2XData = (*buf > MULTI_MAX_TYPE_D2);
	if (bD2XData && (gameStates.app.bNostalgia > 1))
		return;
	if (networkData.syncPack.dataSize && !bD2XData && networkData.bD2XData)
		NetworkDoFrame (1, 0);
	networkData.bD2XData = bD2XData;
	}
Assert (networkData.syncPack.dataSize + len <= networkData.nMaxXDataSize);
memcpy (networkData.syncPack.data + networkData.syncPack.dataSize, buf, len);
networkData.syncPack.dataSize += len;
if (bUrgent)
	networkData.bPacketUrgent = bUrgent;
}

//------------------------------------------------------------------------------

void NetworkSendSmashedLights (int nPlayer) 
 {
  // send the lights that have been blown out
  int i;

for (i = 0;i <= gameData.segs.nLastSegment; i++)
   if (gameData.render.lights.subtracted [i])
		MultiSendLightSpecific (nPlayer, i, gameData.render.lights.subtracted [i]);
 }

//------------------------------------------------------------------------------

void NetworkSendFlyThruTriggers (int nPlayer) 
 {
  // send the fly thru triggers that have been disabled
  int i;

for (i = 0; i < gameData.trigs.m_nTriggers; i++)
	if (TRIGGERS [i].m_info.flags & TF_DISABLED)
		MultiSendTriggerSpecific ((char) nPlayer, (ubyte) i);
 }

//------------------------------------------------------------------------------

void NetworkSendPlayerFlags (void)
{
int i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	MultiSendFlags ((char) i);
 }

//------------------------------------------------------------------------------

void NetworkSendNakedPacket (char *buf, short len, int who)
{
if (!(gameData.app.nGameMode & GM_NETWORK)) 
	return;
if (nakedData.nLength == 0) {
	nakedData.buf [0] = PID_NAKED_PDATA;
	nakedData.buf [1] = N_LOCALPLAYER;
	nakedData.nLength = 2;
	}
if (len + nakedData.nLength>networkData.nMaxXDataSize) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		IPXSendPacketData (
			reinterpret_cast<ubyte*> (nakedData.buf), 
			nakedData.nLength, 
			netPlayers [0].m_info.players [who].network.Server (), 
			netPlayers [0].m_info.players [who].network.Node (), gameData.multiplayer.players [who].netAddress);
	nakedData.nLength = 2;
	memcpy (&nakedData.buf [nakedData.nLength], buf, len);     
	nakedData.nLength+=len;
	nakedData.nDestPlayer=who;
	}
else {
	memcpy (&nakedData.buf [nakedData.nLength], buf, len);     
	nakedData.nLength+=len;
	nakedData.nDestPlayer=who;
	}
 }

//------------------------------------------------------------------------------

char bNameReturning = 1;

void NetworkSendPlayerNames (tSequencePacket *their)
{
	int nConnected = 0, count = 0, i;
	char buf [80];

if (!their) {
#if 1			
	console.printf (CON_DBG, "Got a player name without a return address! Get Jason\n");
#endif
	return;
	}
buf [0] = PID_NAMES_RETURN; 
count++;
*reinterpret_cast<int*> (buf + 1) = netGame.m_info.nSecurity; 
count+=4;
if (!bNameReturning) {
	buf [count++] = (char) 255; 
	goto sendit;
	}
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected)
		nConnected++;
buf [count++] = nConnected; 
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected) {
		buf [count++] = netPlayers [0].m_info.players [i].rank; 
		memcpy (buf + count, netPlayers [0].m_info.players [i].callsign, CALLSIGN_LEN + 1);
		count += CALLSIGN_LEN + 1;
		}
buf [count++] = 99;
buf [count++] = netGame.GetShortPackets ();	
buf [count++] = char (PacketsPerSec ());
 
sendit:	   

IPXSendInternetPacketData (reinterpret_cast<ubyte*> (buf), count, 
									their->player.network.Server (), 
									their->player.network.Node ());
}

//------------------------------------------------------------------------------

void NetworkSendMissingObjFrames (void)
{
if (gameStates.multi.nGameType >= IPX_GAME) {
	networkData.sync [0].objs.missingFrames.pid = PID_MISSING_OBJ_FRAMES;
	networkData.sync [0].objs.missingFrames.nPlayer = N_LOCALPLAYER;
	SendInternetMissingObjFramesPacket (networkData.serverAddress, networkData.serverAddress + 4);
	} 
}

//------------------------------------------------------------------------------

