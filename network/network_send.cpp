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

//------------------------------------------------------------------------------

void NetworkSendDoorUpdates (int nPlayer)
{
	// Send door status when new tPlayer joins

	int i;
	tWall *wallP;
   
//   Assert (nPlayer>-1 && nPlayer<gameData.multiplayer.nPlayers);
for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
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
  // send marker positions/text to new tPlayer
  int i, j;

for (i = j = 0; i < gameData.multiplayer.nPlayers; i++, j++) {
   if (gameData.marker.objects [j] != -1)
		MultiSendDropMarker (i, gameData.marker.point [j], 0, gameData.marker.szMessage [j]);
   if (gameData.marker.objects [++j] != -1)
		MultiSendDropMarker (i, gameData.marker.point [j], 1, gameData.marker.szMessage [j]);
	}
 }

//------------------------------------------------------------------------------

void NetworkSendRejoinSync (int nPlayer, tNetworkSyncData *syncP)
{
	int i, j;

gameData.multiplayer.players [nPlayer].connected = 1; // connect the new guy
ResetPlayerTimeout (nPlayer, -1);
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
	// Endlevel started before we finished sending the goods, we'll
	// have to stop and try again after the level.

	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			syncP->player [1].player.network.ipx.server, 
			syncP->player [1].player.network.ipx.node, 
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
			 (i != gameData.multiplayer.nLocalPlayer) && 
			 gameData.multiplayer.players [i].connected)
			if (gameStates.multi.nGameType >= IPX_GAME) {
				SendSequencePacket (
					syncP->player [1], 
					netPlayers.players [i].network.ipx.server, 
					netPlayers.players [i].network.ipx.node, 
					gameData.multiplayer.players [i].netAddress);
			}
		}
	}       
// Send sync packet to the new guy
NetworkUpdateNetGame ();

// Fill in the kill list
for (j = 0; j < MAX_PLAYERS; j++) {
	for (i = 0; i < MAX_PLAYERS; i++)
		netGame.kills [j][i] = gameData.multigame.kills.matrix [j][i];
	netGame.killed [j] = gameData.multiplayer.players [j].netKilledTotal;
	netGame.playerKills [j] = gameData.multiplayer.players [j].netKillsTotal;
	netGame.player_score [j] = gameData.multiplayer.players [j].score;
	}       
netGame.xLevelTime = LOCALPLAYER.timeLevel;
netGame.monitor_vector = NetworkCreateMonitorVector ();
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (
		syncP->player [1].player.network.ipx.server, 
		syncP->player [1].player.network.ipx.node);
	SendNetPlayersPacket (
		syncP->player [1].player.network.ipx.server, 
		syncP->player [1].player.network.ipx.node);
	}
return;
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
		netGame.kills [j][i] = gameData.multigame.kills.matrix [j][i];
	netGame.killed [j] = gameData.multiplayer.players [j].netKilledTotal;
	netGame.playerKills [j] = gameData.multiplayer.players [j].netKillsTotal;
	netGame.player_score [j] = gameData.multiplayer.players [j].score;
	}       
netGame.xLevelTime = LOCALPLAYER.timeLevel;
netGame.monitor_vector = NetworkCreateMonitorVector ();
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (
		networkData.sync.player [1].player.network.ipx.server, 
		networkData.sync.player [1].player.network.ipx.node);
	SendNetPlayersPacket (
		networkData.sync.player [1].player.network.ipx.server, 
		networkData.sync.player [1].player.network.ipx.node);
	}
}

#endif

//------------------------------------------------------------------------------
// Send a broadcast request for game info

int NetworkSendGameListRequest (void)
{
	tSequencePacket me;

#ifdef _DEBUG
memset (&me, 0, sizeof (me));
#endif
me.nType = PID_GAME_LIST;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN + 1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.ipx.node, IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.ipx.server, IpxGetMyServerAddress (), 4);
	if (gameStates.multi.nGameType != UDP_GAME) 
		SendBroadcastSequencePacket (me);
	else {
		int	i;

		con_printf (0, "looking for netgames\n");
		if (gameStates.multi.bUseTracker) {
			if (!RequestServerListFromTracker ())
				return 0;
			//memset (activeNetGames, 0, sizeof (activeNetGames));
			//networkData.nActiveGames = 0;
			for (i = 0; GetServerFromList (i); i++)
				SendInternetSequencePacket (me, ipx_ServerAddress, ipx_ServerAddress + 4);
			}
		else {
			//memset (activeNetGames, 0, sizeof (activeNetGames));
			//networkData.nActiveGames = 0;
			SendInternetSequencePacket (me, ipx_ServerAddress, ipx_ServerAddress + 4);
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
memcpy (me.player.callsign, 
			LOCALPLAYER.callsign, 
			CALLSIGN_LEN+1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.ipx.node, IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.ipx.server, IpxGetMyServerAddress (), 4);
	SendBroadcastSequencePacket (me);
	}
}

//------------------------------------------------------------------------------

void NetworkSendEndLevelSub (int nPlayer)
{
	tEndLevelInfo end;
	int i, j = 0;

	// Send an endlevel packet for a tPlayer
end.nType       = PID_ENDLEVEL;
end.nPlayer = nPlayer;
end.connected  = gameData.multiplayer.players [nPlayer].connected;
end.kills      = INTEL_SHORT (gameData.multiplayer.players [nPlayer].netKillsTotal);
end.killed     = INTEL_SHORT (gameData.multiplayer.players [nPlayer].netKilledTotal);
memcpy (end.killMatrix, gameData.multigame.kills.matrix [nPlayer], MAX_PLAYERS*sizeof (short));
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		end.killMatrix [i][j] = INTEL_SHORT (end.killMatrix [i][j]);
#else
j = j;          // to satisfy compiler
#endif
if (gameData.multiplayer.players [nPlayer].connected == 1) {// Still playing
	Assert (gameData.reactor.bDestroyed);
	end.seconds_left = gameData.reactor.countdown.nSecsLeft;
	}
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {       
	if ((i != gameData.multiplayer.nLocalPlayer) && 
		 (i != nPlayer) && 
		 (gameData.multiplayer.players [i].connected)) {
		if (gameData.multiplayer.players [i].connected == 1)
			NetworkSendEndLevelShortSub (nPlayer, i);
		else if (gameStates.multi.nGameType >= IPX_GAME)
			IPXSendPacketData (
				(ubyte *)&end, sizeof (tEndLevelInfo), 
			netPlayers.players [i].network.ipx.server, 
			netPlayers.players [i].network.ipx.node, gameData.multiplayer.players [i].netAddress);
		}
	}
}

//------------------------------------------------------------------------------

/* Send an updated endlevel status to other hosts */
void NetworkSendEndLevelPacket (void)
{
NetworkSendEndLevelSub (gameData.multiplayer.nLocalPlayer);
}

//------------------------------------------------------------------------------

/* Send an endlevel packet for a tPlayer */
void NetworkSendEndLevelShortSub (int from_player_num, int to_player)
{
	tEndLevelInfoShort end;

end.nType = PID_ENDLEVEL_SHORT;
end.nPlayer = from_player_num;
end.connected = gameData.multiplayer.players [from_player_num].connected;
end.seconds_left = gameData.reactor.countdown.nSecsLeft;

if (gameData.multiplayer.players [from_player_num].connected == 1) {// Still playing
	Assert (gameData.reactor.bDestroyed);
	}
if ((to_player != gameData.multiplayer.nLocalPlayer) && 
	 (to_player != from_player_num) && 
	 (gameData.multiplayer.players [to_player].connected)) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		IPXSendPacketData (
		 (ubyte *)&end, sizeof (tEndLevelInfoShort), 
		netPlayers.players [to_player].network.ipx.server, 
		netPlayers.players [to_player].network.ipx.node, gameData.multiplayer.players [to_player].netAddress);
	}
}

//------------------------------------------------------------------------------

void NetworkSendGameInfo (tSequencePacket *their)
{
	// Send game info to someone who requested it

	char oldType, oldStatus;
   fix timevar;
   int i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.nType;
oldStatus = netGame.gameStatus;
netGame.nType = PID_GAME_INFO;
netPlayers.nType = PID_PLAYERSINFO;
netPlayers.nSecurity = netGame.nSecurity;
netGame.versionMajor = D2X_MAJOR;
netGame.versionMinor = D2X_MINOR;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.gameStatus = NETSTAT_ENDLEVEL;
if (netGame.xPlayTimeAllowed) {
	timevar = I2X (netGame.xPlayTimeAllowed * 5 * 60);
	i = X2I (timevar - gameStates.app.xThisLevelTime);
	if (i < 30)
		netGame.gameStatus = NETSTAT_ENDLEVEL;
	}       
if (!their) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		SendBroadcastFullNetGamePacket ();
		SendBroadcastNetPlayersPacket ();
		}
	}
else if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetFullNetGamePacket (
		their->player.network.ipx.server, 
		their->player.network.ipx.node);
	SendNetPlayersPacket (
		their->player.network.ipx.server, 
		their->player.network.ipx.node);
	}
netGame.nType = oldType;
netGame.gameStatus = oldStatus;
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
extraGameInfo [1].nSpotSize = egi1Save.nSpotSize;
extraGameInfo [1].nCoopPenalty = egi1Save.nCoopPenalty;
extraGameInfo [1].bRadarEnabled = ((netGame.gameFlags & NETGAME_FLAG_SHOW_MAP) != 0);
extraGameInfo [1].bWiggle = 1;
extraGameInfo [1].nType = PID_EXTRA_GAMEINFO;
memcpy (extraGameInfo [1].szGameName, mpParams.szGameName, sizeof (mpParams.szGameName));
extraGameInfo [1].nSecurity = netGame.nSecurity;
gameStates.app.bHaveExtraGameInfo [1] = 1;
#if 1
if (!their) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		SendBroadcastExtraGameInfoPacket ();
		} // nothing to do for appletalk games I think....
	}
else {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		SendInternetExtraGameInfoPacket (
			their->player.network.ipx.server, 
			their->player.network.ipx.node);
		}
	} 
#endif 
SetMonsterballForces ();
}

//------------------------------------------------------------------------------

void NetworkSendLiteInfo (tSequencePacket *their)
{
	// Send game info to someone who requested it
	char oldType, oldStatus, oldstatus;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.nType;
oldStatus = netGame.gameStatus;
netGame.nType = PID_LITE_INFO;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.gameStatus = NETSTAT_ENDLEVEL;
// If hoard mode, make this game look closed even if it isn't
if (HoardEquipped ()) {
	if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
		oldstatus = netGame.gameStatus;
		netGame.gameStatus = NETSTAT_ENDLEVEL;
		netGame.gameMode = NETGAME_CAPTURE_FLAG;
		if (oldstatus == NETSTAT_ENDLEVEL)
			netGame.gameFlags|= NETGAME_FLAG_REALLY_ENDLEVEL;
		if (oldstatus == NETSTAT_STARTING)
			netGame.gameFlags|= NETGAME_FLAG_REALLY_FORMING;
		}
	}
if (!their) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		SendBroadcastLiteNetGamePacket ();
		}               // nothing to do for appletalk I think....
	}
else {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		SendInternetLiteNetGamePacket (
			their->player.network.ipx.server, 
			their->player.network.ipx.node);
		}
	}  
//  Restore the pre-hoard mode
if (HoardEquipped ()) {
	if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY | GM_MONSTERBALL)) {
		if (gameData.app.nGameMode & GM_ENTROPY)
 			netGame.gameMode = NETGAME_ENTROPY;
		else if (gameData.app.nGameMode & GM_MONSTERBALL)
 			netGame.gameMode = NETGAME_MONSTERBALL;
		else if (gameData.app.nGameMode & GM_TEAM)
 			netGame.gameMode = NETGAME_TEAM_HOARD;
		else
 			netGame.gameMode = NETGAME_HOARD;
		netGame.gameFlags &= ~(NETGAME_FLAG_REALLY_ENDLEVEL | NETGAME_FLAG_REALLY_FORMING | NETGAME_FLAG_TEAM_HOARD);
		}
	}
netGame.nType = oldType;
netGame.gameStatus = oldStatus;
NetworkSendExtraGameInfo (their);
}       

//------------------------------------------------------------------------------

/* Send game info to all players in this game */
void NetworkSendNetgameUpdate (void)
{
	char	oldType, oldStatus, szIP [30];
	int	i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.nType;
oldStatus = netGame.gameStatus;
netGame.nType = PID_GAME_UPDATE;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.gameStatus = NETSTAT_ENDLEVEL;
PrintLog ("sending netgame update:\n");
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected) && (i != gameData.multiplayer.nLocalPlayer)) {
		if (gameStates.multi.nGameType >= IPX_GAME) {
			PrintLog ("   %s (%s)\n", netPlayers.players [i].callsign, 
				iptos (szIP, (char *) netPlayers.players [i].network.ipx.node));
			SendLiteNetGamePacket (
				netPlayers.players [i].network.ipx.server, 
				netPlayers.players [i].network.ipx.node, 
				gameData.multiplayer.players [i].netAddress);
			}
		}
	}
netGame.nType = oldType;
netGame.gameStatus = oldStatus;
}       
			  
//------------------------------------------------------------------------------

int NetworkSendRequest (void)
{
	// Send a request to join a game 'netGame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

if (netGame.nNumPlayers < 1)
	return 1;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	if (netPlayers.players [i].connected)
	   break;
Assert (i < MAX_NUM_NET_PLAYERS);
networkData.thisPlayer.nType = PID_REQUEST;
networkData.thisPlayer.player.connected = gameData.missions.nCurrentLevel;
networkData.nJoinState = 0;
networkData.bHaveSync = 0;
networkData.sync [0].objs.nFrame = 0;
networkData.sync [0].objs.missingFrames.nFrame = 0;
networkData.bTraceFrames = 1;
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetSequencePacket (
		networkData.thisPlayer, 
		netPlayers.players [i].network.ipx.server, 
		netPlayers.players [i].network.ipx.node);
	}
return i;
}

//------------------------------------------------------------------------------

void NetworkSendSync (void)
{
	int i, j, np;

	// Randomize their starting locations...
d_srand (TimerGetFixedSeconds ());
for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	if (gameData.multiplayer.players [i].connected)
		gameData.multiplayer.players [i].connected = 1; // Get rid of endlevel connect statuses
	if (IsCoopGame)
		netGame.locations [i] = i;
	else {
		do {
			np = d_rand () % gameData.multiplayer.nPlayerPositions;
			for (j = 0; j < i; j++) {
				if (netGame.locations [j] == np) {
					np = -1;
					break;
					}
				}
			} while (np < 0);
		// np is a location that is not used anywhere else..
		netGame.locations [i] = np;
		}
	}
// Push current data into the sync packet
NetworkUpdateNetGame ();
netGame.gameStatus = NETSTAT_PLAYING;
netGame.nType = PID_SYNC;
netGame.nSegmentCheckSum = networkData.nSegmentCheckSum;
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((!gameData.multiplayer.players [i].connected) || (i == gameData.multiplayer.nLocalPlayer))
		continue;
	if (gameStates.multi.nGameType >= IPX_GAME) {
	// Send several times, extras will be ignored
		SendInternetFullNetGamePacket (
			netPlayers.players [i].network.ipx.server, 
			netPlayers.players [i].network.ipx.node);
		SendNetPlayersPacket (
			netPlayers.players [i].network.ipx.server, 
			netPlayers.players [i].network.ipx.node);
		}
	}
NetworkReadSyncPacket (&netGame, 1); // Read it myself, as if I had sent it
}

//------------------------------------------------------------------------------

void NetworkSendData (ubyte * ptr, int len, int urgent)
{
	char	bCheck;
	int	bD2XData;

#ifdef NETPROFILING
TTSent [ptr [0]]++;  
fprintf (SendLogFile, "Packet nType: %d Len:%d Urgent:%d TT=%d\n", ptr [0], len, urgent, TTSent [ptr [0]]);
fflush (SendLogFile);
#endif
   
if (gameStates.app.bEndLevelSequence)
	return;
if (!networkData.bSyncPackInited) {
	networkData.bSyncPackInited = 1;
	memset (&networkData.syncPack, 0, sizeof (tFrameInfo));
	}
if (urgent)
	networkData.bPacketUrgent = 1;
if ((networkData.syncPack.data_size + len) > networkData.nMaxXDataSize) {
	bCheck = ptr [0];
	NetworkDoFrame (1, 0);
	if (networkData.syncPack.data_size != 0) {
#if 1			
	con_printf (CONDBG, "%d bytes were added to data by NetworkDoFrame!\n", networkData.syncPack.data_size);
#endif
	Int3 ();
	}
//              Int3 ();         // Trying to send too much!
//              return;
#if 1			
	con_printf (CONDBG, "Packet overflow, sending additional packet, nType %d len %d.\n", ptr [0], len);
#endif
	Assert (bCheck == ptr [0]);
	}
// for IPX game, separate legacy and D2X message to avoid non D2X-XL participants losing data
// because they do not know the D2X data and hence cannot determine its length, thus getting out
// of sync when decompositing a multi-data packet.
if (gameStates.multi.nGameType == IPX_GAME) {
	bD2XData = (*ptr > MULTI_MAX_TYPE_D2);
	if (bD2XData && (gameStates.app.bNostalgia > 1))
		return;
	if (networkData.syncPack.data_size && !bD2XData && networkData.bD2XData)
		NetworkDoFrame (1, 0);
	networkData.bD2XData = bD2XData;
	}
Assert (networkData.syncPack.data_size + len <= networkData.nMaxXDataSize);
memcpy (networkData.syncPack.data + networkData.syncPack.data_size, ptr, len);
networkData.syncPack.data_size += len;
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

for (i = 0; i < gameData.trigs.nTriggers; i++)
	if (gameData.trigs.triggers [i].flags & TF_DISABLED)
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
	nakedData.buf [1] = gameData.multiplayer.nLocalPlayer;
	nakedData.nLength = 2;
	}
if (len + nakedData.nLength>networkData.nMaxXDataSize) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		IPXSendPacketData (
			(ubyte *) nakedData.buf, 
			nakedData.nLength, 
			netPlayers.players [who].network.ipx.server, 
			netPlayers.players [who].network.ipx.node, gameData.multiplayer.players [who].netAddress);
	nakedData.nLength=2;
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
	con_printf (CONDBG, "Got a tPlayer name without a return address! Get Jason\n");
#endif
	return;
	}
buf [0] = PID_NAMES_RETURN; 
count++;
(*(int *) (buf+1)) = netGame.nSecurity; 
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
		buf [count++] = netPlayers.players [i].rank; 
		memcpy (buf + count, netPlayers.players [i].callsign, CALLSIGN_LEN + 1);
		count += CALLSIGN_LEN + 1;
		}
buf [count++] = 99;
buf [count++] = netGame.bShortPackets;	
buf [count++] = (char) PacketsPerSec ();
 
sendit:	   

IPXSendInternetPacketData ((ubyte *)buf, count, 
									their->player.network.ipx.server, 
									their->player.network.ipx.node);
}

//------------------------------------------------------------------------------

void NetworkSendMissingObjFrames (void)
{
if (gameStates.multi.nGameType >= IPX_GAME) {
	networkData.sync [0].objs.missingFrames.pid = PID_MISSING_OBJ_FRAMES;
	networkData.sync [0].objs.missingFrames.nPlayer = gameData.multiplayer.nLocalPlayer;
	SendInternetMissingObjFramesPacket (ipx_ServerAddress, ipx_ServerAddress + 4);
	} 
}

//------------------------------------------------------------------------------

