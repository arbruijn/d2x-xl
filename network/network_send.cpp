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

void NetworkSendDoorUpdates (int32_t nPlayer)
{
	// Send door status when new player joins

	CWall* wallP = WALLS.Buffer ();
   
//   Assert (nPlayer>-1 && nPlayer<N_PLAYERS);
for (int32_t i = 0; i < gameData.wallData.nWalls; i++, wallP++) {
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
  int32_t i, j;

for (i = j = 0; i < N_PLAYERS; i++, j++) {
   if (markerManager.Objects (j) != -1)
		MultiSendDropMarker (i, markerManager.Position (j), 0, markerManager.Message (j));
   if (markerManager.Objects (++j) != -1)
		MultiSendDropMarker (i, markerManager.Position (j), 0, markerManager.Message (j));
	}
 }

//------------------------------------------------------------------------------

void NetworkSendRejoinSync (int32_t nPlayer, tNetworkSyncInfo *syncInfoP)
{
	int32_t i, j;

CONNECT (nPlayer, CONNECT_PLAYING); // connect the new guy
ResetPlayerTimeout (nPlayer, -1);
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
	// Endlevel started before we finished sending the goods, we'll
	// have to stop and try again after the level.

	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (syncInfoP->player [1].player.network.Network (), syncInfoP->player [1].player.network.Node (), DUMP_ENDLEVEL);
	syncInfoP->nState = 0; 
	syncInfoP->nExtras = 0;
	return;
	}
if (networkData.bPlayerAdded) {
	syncInfoP->player [1].nType = PID_ADDPLAYER;
	syncInfoP->player [1].player.connected = nPlayer;
	NetworkNewPlayer (&syncInfoP->player [1]);

	for (i = 0; i < N_PLAYERS; i++) {
		if ((i != nPlayer) && (i != N_LOCALPLAYER) && PLAYER (i).IsConnected () && (gameStates.multi.nGameType >= IPX_GAME)) {
			SendSequencePacket (
				syncInfoP->player [1], 
				NETPLAYER (i).network.Network (), 
				NETPLAYER (i).network.Node (), 
				PLAYER (i).netAddress);
			}
		}       
	}
// Send sync packet to the new guy
NetworkUpdateNetGame ();

// Fill in the kill list
for (j = 0; j < MAX_PLAYERS; j++) {
	for (i = 0; i < MAX_PLAYERS; i++)
		*netGameInfo.Kills (j, i) = gameData.multigame.score.matrix [j][i];
	*netGameInfo.Killed (j) = PLAYER (j).netKilledTotal;
	*netGameInfo.PlayerKills (j) = PLAYER (j).netKillsTotal;
	*netGameInfo.PlayerScore (j) = PLAYER (j).score;
	}       
netGameInfo.SetLevelTime (LOCALPLAYER.timeLevel);
netGameInfo.SetMonitorVector (NetworkCreateMonitorVector ());
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (syncInfoP->player [1].player.network.Network (), syncInfoP->player [1].player.network.Node ());
	SendNetPlayersPacket (syncInfoP->player [1].player.network.Network (), syncInfoP->player [1].player.network.Node ());
	MultiSendMonsterball (1, 1);
	}
}

//------------------------------------------------------------------------------

#if 0

void ResendSyncDueToPacketLoss (void)
{
   int32_t i, j;

NetworkUpdateNetGame ();
// Fill in the kill list
for (j = 0; j < MAX_PLAYERS; j++) {
	for (i = 0; i < MAX_PLAYERS; i++)
		*netGameInfo.Kills () [j][i] = gameData.multigame.score.matrix [j][i];
	*netGameInfo.Killed () [j] = PLAYER (j).netKilledTotal;
	*netGameInfo.PlayerKills () [j] = PLAYER (j).netKillsTotal;
	*netGameInfo.PlayerScore () [j] = PLAYER (j).score;
	}       
netGameInfo.LevelTime () = LOCALPLAYER.timeLevel;
netGameInfo.MonitorVector () = NetworkCreateMonitorVector ();
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (
		networkData.syncInfo [0].player [1].player.network.Network (), 
		networkData.syncInfo [0].player [1].player.network.Node ());
	SendNetPlayersPacket (
		networkData.syncInfo [0].player [1].player.network.Network (), 
		networkData.syncInfo [0].player [1].player.network.Node ());
	}
}

#endif

//------------------------------------------------------------------------------

#if DBG

void TestXMLInfoRequest (uint8_t* serverAddress)
{
#if 0 //DBG
gameStates.multi.bTrackerCall = 2;
networkThread.Send ((uint8_t *) "FDescent Game Info Request", (int32_t) strlen ("FDescent Game Info Request") + 1, serverAddress, serverAddress + 4);
gameStates.multi.bTrackerCall = 0;
#endif
#if 1 //DBG
gameStates.multi.bTrackerCall = 2;
networkThread.Send ((uint8_t *) "GDescent Game Status Request", (int32_t) strlen ("GDescent Game Status Request") + 1, serverAddress, serverAddress + 4);
gameStates.multi.bTrackerCall = 0;
#endif
}

#endif

//------------------------------------------------------------------------------
// Send a broadcast request for game info

int32_t NetworkSendGameListRequest (int32_t bAutoLaunch)
{
	tPlayerSyncData me;

#if DBG
memset (&me, 0, sizeof (me));
#endif
me.nType = PID_GAME_LIST;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	me.player.network.SetNode (IpxGetMyLocalAddress ());
	me.player.network.SetNetwork (IpxGetMyServerAddress ());
	if (gameStates.multi.nGameType != UDP_GAME) 
		SendBroadcastPlayerSyncData (me);
	else {
		console.printf (0, "looking for netgames\n");
		uint8_t serverAddress [10];
		if (tracker.m_bUse && !bAutoLaunch) {
			if (!tracker.RequestServerList ())
				return 0;
			for (int32_t i = 0; tracker.GetServerFromList (i, serverAddress); i++) {
				SendInternetPlayerSyncData (me, serverAddress, serverAddress + 4);
#if DBG
				TestXMLInfoRequest (serverAddress);
#endif
				}
			}
		else {
			SendInternetPlayerSyncData (me, networkData.serverAddress.Network (), networkData.serverAddress.Node ());
			}
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void NetworkSendAllInfoRequest (char nType, int32_t nSecurity)
{
	// Send a broadcast request for game info
	tPlayerSyncData me;

me.nSecurity = nSecurity;
me.nType = nType;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	me.player.network.SetNode (IpxGetMyLocalAddress ());
	me.player.network.SetNetwork (IpxGetMyServerAddress ());
	SendBroadcastPlayerSyncData (me);
	}
}

//------------------------------------------------------------------------------

void NetworkSendEndLevelSub (int32_t nPlayer)
{
	CEndLevelInfo end;
	int32_t i;

// Send an endlevel packet for a player
#if 0 //DBG
if (!N_LOCALPLAYER)
	audio.PlaySound (SOUND_HOMING_WARNING);
#endif
*end.Type () = PID_ENDLEVEL;
*end.Player () = nPlayer;
end.SetConnected ((int8_t) PLAYER (nPlayer).GetConnected ());
*end.Kills () = INTEL_SHORT (PLAYER (nPlayer).netKillsTotal);
*end.Killed () = INTEL_SHORT (PLAYER (nPlayer).netKilledTotal);
memcpy (end.ScoreMatrix (), gameData.multigame.score.matrix [nPlayer], MAX_NUM_NET_PLAYERS * sizeof (int16_t));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
for (i = 0; i < MAX_PLAYERS; i++)
	for (int32_t j = 0; j < MAX_PLAYERS; j++)
		*end.ScoreMatrix (i, j) = INTEL_SHORT (*end.ScoreMatrix (i, j));
#endif
if (PLAYER (nPlayer).Connected (CONNECT_PLAYING)) {// Still playing
#if DBG
	BRP;
#endif
#if 0
	Assert (gameData.reactor.bDestroyed);
	*end.SecondsLeft () = gameData.reactor.countdown.nSecsLeft;
#endif
	}
for (i = 0; i < N_PLAYERS; i++) {       
	if ((i != N_LOCALPLAYER) && (i != nPlayer) && (PLAYER (i).IsConnected ())) {
		if (PLAYER (i).Connected (CONNECT_PLAYING))
			NetworkSendEndLevelShortSub (nPlayer, i);
		else if (gameStates.multi.nGameType >= IPX_GAME)
			networkThread.Send (
				reinterpret_cast<uint8_t*> (&end), sizeof (tEndLevelInfo), 
				NETPLAYER (i).network.Network (), 
				NETPLAYER (i).network.Node (), 
				PLAYER (i).netAddress);
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

void NetworkSendEndLevelShortSub (int32_t nSrcPlayer, int32_t nDestPlayer)
{
if (gameStates.multi.nGameType < IPX_GAME)
	return;
if (!PLAYER (nDestPlayer).IsConnected ())
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
eli.connected = PLAYER (nSrcPlayer).connected;
eli.secondsLeft = gameData.reactor.countdown.nSecsLeft;
networkThread.Send (
	reinterpret_cast<uint8_t*> (&eli), sizeof (tEndLevelInfoShort), 
	NETPLAYER (nDestPlayer).network.Network (), 
	NETPLAYER (nDestPlayer).network.Node (), 
	PLAYER (nDestPlayer).netAddress);
}

//------------------------------------------------------------------------------

void NetworkSendGameInfo (tPlayerSyncData *their)
{
	// Send game info to someone who requested it

	char oldType, oldStatus;
   fix timevar;
   int32_t i;


NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGameInfo.m_info.nType;
oldStatus = netGameInfo.m_info.gameStatus;
netGameInfo.m_info.nType = PID_GAME_INFO;
netPlayers [0].m_info.nType = PID_PLAYERSINFO;
netPlayers [0].m_info.nSecurity = netGameInfo.m_info.nSecurity;
netGameInfo.m_info.versionMajor = D2X_MAJOR;
netGameInfo.m_info.versionMinor = D2X_MINOR;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGameInfo.m_info.gameStatus = NETSTAT_ENDLEVEL;
if ((timevar = I2X (netGameInfo.GetPlayTimeAllowed () * 5 * 60))) {
	i = X2I (timevar - gameStates.app.xThisLevelTime);
	if (i < 30)
		netGameInfo.m_info.gameStatus = NETSTAT_ENDLEVEL;
	}       
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!their) {
		SendBroadcastFullNetGamePacket ();
		SendBroadcastNetPlayersPacket ();
		}
	else {
		SendInternetFullNetGamePacket (their->player.network.Network (), their->player.network.Node ());
		SendNetPlayersPacket (their->player.network.Network (), their->player.network.Node ());
		}
	}
netGameInfo.m_info.nType = oldType;
netGameInfo.m_info.gameStatus = oldStatus;
//	if (IsEntropyGame || extraGameInfo [0].bEnhancedCTF)
//make half-way sure the client gets this data ...
NetworkSendExtraGameInfo (their);
MultiSendMonsterball (1, 1);
}       

//------------------------------------------------------------------------------

void NetworkSendExtraGameInfo (tPlayerSyncData *their)
{
if (!IAmGameHost ())
	return;

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
extraGameInfo [1].nSpotSize = egi1Save.nSpotSize;
extraGameInfo [1].nCoopPenalty = egi1Save.nCoopPenalty;
extraGameInfo [1].bRadarEnabled = ((netGameInfo.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP) != 0);
extraGameInfo [1].bWiggle = 1;
extraGameInfo [1].nType = PID_EXTRA_GAMEINFO;
extraGameInfo [1].nVersion = EGI_DATA_VERSION;
memcpy (extraGameInfo [1].szGameName, mpParams.szGameName, sizeof (mpParams.szGameName));
extraGameInfo [1].nSecurity = netGameInfo.m_info.nSecurity;
gameStates.app.bHaveExtraGameInfo [1] = 1;
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!their)
		SendBroadcastExtraGameInfoPacket ();
	else
		SendInternetExtraGameInfoPacket (their->player.network.Network (), their->player.network.Node ());
	} 
SetMonsterballForces ();
MultiSendWeaponStates ();
}

//------------------------------------------------------------------------------

void NetworkSendXMLGameInfo (void)
{
if (IAmGameHost ()) {
	gameStates.multi.bTrackerCall = 2;
	char* szInfo = XMLGameInfo ();
	SendInternetXMLGameInfoPacket (szInfo, networkData.packetDest.Network (), networkData.packetDest.Node ());
	gameStates.multi.bTrackerCall = 0;
	}
}

//------------------------------------------------------------------------------

void NetworkSendLiteInfo (tPlayerSyncData *their)
{
	// Send game info to someone who requested it
	char oldType, oldStatus;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGameInfo.m_info.nType;
oldStatus = netGameInfo.m_info.gameStatus;
netGameInfo.m_info.nType = PID_LITE_INFO;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGameInfo.m_info.gameStatus = NETSTAT_ENDLEVEL;
// If hoard mode, make this game look closed even if it isn't
if (HoardEquipped ()) {
	if (gameData.app.GameMode (GM_HOARD | GM_ENTROPY)) {
		char oldStatus = netGameInfo.m_info.gameStatus;
		netGameInfo.m_info.gameStatus = NETSTAT_ENDLEVEL;
		netGameInfo.m_info.gameMode = NETGAME_CAPTURE_FLAG;
		if (oldStatus == NETSTAT_ENDLEVEL)
			netGameInfo.m_info.gameFlags |= NETGAME_FLAG_REALLY_ENDLEVEL;
		if (oldStatus == NETSTAT_STARTING)
			netGameInfo.m_info.gameFlags |= NETGAME_FLAG_REALLY_FORMING;
		}
	}
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!their)
		SendBroadcastLiteNetGamePacket ();
	else
		SendInternetLiteNetGamePacket (their->player.network.Network (), their->player.network.Node ());
	}  
//  Restore the pre-hoard mode
if (HoardEquipped ()) {
	if (gameData.app.GameMode (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL)) {
		if (IsEntropyGame)
 			netGameInfo.m_info.gameMode = NETGAME_ENTROPY;
		else if (gameData.app.GameMode (GM_MONSTERBALL))
 			netGameInfo.m_info.gameMode = NETGAME_MONSTERBALL;
		else if (IsTeamGame)
 			netGameInfo.m_info.gameMode = NETGAME_TEAM_HOARD;
		else
 			netGameInfo.m_info.gameMode = NETGAME_HOARD;
		netGameInfo.m_info.gameFlags &= ~(NETGAME_FLAG_REALLY_ENDLEVEL | NETGAME_FLAG_REALLY_FORMING | NETGAME_FLAG_TEAM_HOARD);
		}
	}
netGameInfo.m_info.nType = oldType;
netGameInfo.m_info.gameStatus = oldStatus;
NetworkSendExtraGameInfo (their);
}       

//------------------------------------------------------------------------------

/* Send game info to all players in this game */
void NetworkSendNetGameUpdate (void)
{
	char	oldType, oldStatus, szIP [30];
	int32_t	i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGameInfo.m_info.nType;
oldStatus = netGameInfo.m_info.gameStatus;
netGameInfo.m_info.nType = PID_GAME_UPDATE;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGameInfo.m_info.gameStatus = NETSTAT_ENDLEVEL;
PrintLog (1, "sending netgame update:\n");
for (i = 0; i < N_PLAYERS; i++) {
	if (PLAYER (i).IsConnected () && (i != N_LOCALPLAYER)) {
		if (gameStates.multi.nGameType >= IPX_GAME) {
			PrintLog (1, "%s (%s)\n", NETPLAYER (i).callsign, 
				iptos (szIP, reinterpret_cast<char*> (NETPLAYER (i).network.Node ())));
			SendLiteNetGamePacket (
				NETPLAYER (i).network.Network (), 
				NETPLAYER (i).network.Node (), 
				PLAYER (i).netAddress);
			}
		}
	}
netGameInfo.m_info.nType = oldType;
netGameInfo.m_info.gameStatus = oldStatus;
}       
			  
//------------------------------------------------------------------------------

int32_t NetworkSendRequest (void)
{
	// Send a request to join a game 'netGameInfo'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int32_t i;

if (netGameInfo.m_info.nNumPlayers < 1)
	return 1;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	if (NETPLAYER (i).IsConnected ())
	   break;
Assert (i < MAX_NUM_NET_PLAYERS);
networkData.thisPlayer.nType = PID_REQUEST;
networkData.thisPlayer.player.connected = missionManager.nCurrentLevel;
if (networkData.nJoinState != 2) {
	networkData.syncInfo [0].objs.nFrame = 0;
	networkData.syncInfo [0].objs.nFramesToSkip = 0;
	}
networkData.thisPlayer.nObjFramesToSkip = networkData.syncInfo [0].objs.nFramesToSkip;
networkData.bTraceFrames = 1;
ResetWalls (); // may have been changed by players transmitting game state changes like doors opening or exploding etc.
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetPlayerSyncData (networkData.thisPlayer, NETPLAYER (i).network.Network (), NETPLAYER (i).network.Node ());
	}
return i;
}

//------------------------------------------------------------------------------

void NetworkSendSync (int8_t nPlayer)
{
	int32_t i;

gameStates.app.SRand ();
	// Randomize their starting locations...
for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
	if (PLAYER (i).IsConnected ())
		CONNECT (i, CONNECT_PLAYING); // Get rid of endlevel connect statuses
if (IsCoopGame) {
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++)
		*netGameInfo.Locations (i) = i;
	}
else {	// randomize player positions
	int32_t h, j = gameData.multiplayer.nPlayerPositions, posTable [MAX_PLAYERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,13, 14, 15};

	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		h = Rand (j);	// compute random table index
		*netGameInfo.Locations (i) = posTable [h];	// pick position using random index
		if (h < --j)
			posTable [h] = posTable [j];	// remove picked position from position table
		}
	}
// Push current data into the sync packet
NetworkUpdateNetGame ();
netGameInfo.m_info.gameStatus = NETSTAT_PLAYING;
netGameInfo.m_info.nType = PID_SYNC;
netGameInfo.SetSegmentCheckSum (networkData.nSegmentCheckSum);
for (i = 0; i < N_PLAYERS; i++) {
	if (i == N_LOCALPLAYER)
		continue;
	if (nPlayer < 0) {
		if (!PLAYER (i).IsConnected ())
		continue;
		}
	else { // player nPlayer is (re-) joining and needs a sync packet
		if (i != nPlayer)
			continue;
		}
	if (gameStates.multi.nGameType >= IPX_GAME) {
	// Send several times, extras will be ignored
		SendInternetFullNetGamePacket (NETPLAYER (i).network.Network (), NETPLAYER (i).network.Node ());
		SendNetPlayersPacket (NETPLAYER (i).network.Network (), NETPLAYER (i).network.Node ());
		}
	}
if (nPlayer < 0)
	NetworkProcessSyncPacket (&netGameInfo, 1); // Read it myself, as if I had sent it
}

//------------------------------------------------------------------------------

void NetworkSendData (uint8_t * buf, int32_t len, int32_t bUrgent)
{
	int32_t	bD2XData;

#ifdef NETPROFILING
TTSent [buf [0]]++;  
fprintf (SendLogFile, "Packet nType: %d Len:%d Urgent:%d TT=%d\n", buf [0], len, bUrgent, TTSent [buf [0]]);
fflush (SendLogFile);
#endif
   
if (gameStates.app.bEndLevelSequence)
	return;
if (!networkData.bSyncPackInited) {
	networkData.bSyncPackInited = 1;
	networkData.SyncPack ().Reset ();
	}
// for IPX game, separate legacy and D2X message to avoid non D2X-XL participants losing data
// because they do not know the D2X data and hence cannot determine its length, thus getting out
// of sync when decompositing a multi-data packet.
if (gameStates.multi.nGameType == IPX_GAME) {
	bD2XData = (*buf > MULTI_MAX_TYPE_D2);
	if (bD2XData && (gameStates.app.bNostalgia > 1))
		return;
	if (networkData.SyncPack ().MsgDataSize () && !bD2XData && networkData.bD2XData)
		NetworkFlushData ();
	networkData.bD2XData = bD2XData;
	}
networkData.SyncPack ().AppendMessage (buf, len);
if (bUrgent)
	NetworkFlushData ();
}

//------------------------------------------------------------------------------
// send the lights that have been blown out

void NetworkSendSmashedLights (int32_t nPlayer) 
{
for (int32_t i = 0; i <= gameData.segData.nLastSegment; i++)
	if (gameData.render.lights.subtracted [i])
		MultiSendLightSpecific (nPlayer, i, gameData.render.lights.subtracted [i]);
}

//------------------------------------------------------------------------------

void NetworkSendFlyThruTriggers (int32_t nPlayer) 
 {
// send the fly thru triggers that have been disabled
for (int32_t i = 0; i < gameData.trigData.m_nTriggers [0]; i++)
	if (GEOTRIGGER (i)->m_info.flags & TF_DISABLED)
		MultiSendTriggerSpecific (nPlayer, (uint8_t) i);
 }

//------------------------------------------------------------------------------

void NetworkSendPlayerFlags (void)
{
for (int32_t i = 0; i < N_PLAYERS; i++)
	MultiSendFlags (i);
 }

//------------------------------------------------------------------------------

void CMineSyncData::Flush (void)
{
if ((gameStates.multi.nGameType >= IPX_GAME) && (Length () > 2) && (m_data.receiver >= 0)) {
	networkThread.Send (
		Buffer (), Length (), 
		NETPLAYER (m_data.receiver).network.Network (), 
		NETPLAYER (m_data.receiver).network.Node (), 
		PLAYER (m_data.receiver).netAddress);
	Reset ();
	}
}

//------------------------------------------------------------------------------

void CMineSyncData::SendMessage (uint8_t* msgBuf, int16_t msgLen, int32_t receiver)
{
if (!IsNetworkGame) 
	return;
if (Length () == 0) {
	*Buffer (0) = PID_MINE_DATA;
	*Buffer (1) = N_LOCALPLAYER;
	SetLength (2);
	}
if (msgLen + Length () > MaxDataSize () || (m_data.receiver != receiver)) {
	Flush ();
	SetLength (2);
	}
memcpy (Buffer (Length ()), msgBuf, msgLen);     
SetLength (Length () + msgLen);
m_data.receiver = receiver;
}

//------------------------------------------------------------------------------
// Send data with local game state changes without any additional information
// like in the sync packets periodically being sent by NetworkDoFrame()

void NetworkSendMineSyncPacket (uint8_t* msgBuf, int16_t msgLen, int32_t receiver)
{
mineSyncData.SendMessage (msgBuf, msgLen, receiver);
}

//------------------------------------------------------------------------------

char bNameReturning = 1;

void NetworkSendPlayerNames (tPlayerSyncData *their)
{
	int32_t nConnected = 0, count = 0, i;
	char buf [80];

if (!their) {
#if 1			
	console.printf (CON_DBG, "Got a player name without a return address! Get Jason\n");
#endif
	return;
	}
buf [0] = PID_NAMES_RETURN; 
count++;
*reinterpret_cast<int32_t*> (buf + 1) = netGameInfo.m_info.nSecurity; 
count += 4;
if (!bNameReturning) {
	buf [count++] = (char) 255; 
	goto sendit;
	}
for (i = 0; i < N_PLAYERS; i++)
	if (PLAYER (i).IsConnected ())
		nConnected++;
buf [count++] = nConnected; 
for (i = 0; i < N_PLAYERS; i++)
	if (PLAYER (i).IsConnected ()) {
		buf [count++] = NETPLAYER (i).rank; 
		memcpy (buf + count, NETPLAYER (i).callsign, CALLSIGN_LEN + 1);
		count += CALLSIGN_LEN + 1;
		}
buf [count++] = 99;
buf [count++] = netGameInfo.GetShortPackets ();	
buf [count++] = char (MinPPS ());
 
sendit:	   

networkThread.Send (reinterpret_cast<uint8_t*> (buf), count, their->player.network.Network (), their->player.network.Node ());
}

//------------------------------------------------------------------------------

