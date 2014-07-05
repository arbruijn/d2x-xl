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

#include "descent.h"
#include "byteswap.h"
#include "timer.h"
#include "error.h"
#include "objeffects.h"
#include "ipx.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "multibot.h"
#include "netmenu.h"
#include "autodl.h"
#include "tracker.h"
#include "playerprofile.h"
#include "gamecntl.h"
#include "text.h"
#include "console.h"

#ifndef _WIN32
#	include "linux/include/ipx_udp.h"
#	include "linux/include/ipx_drv.h"
#endif

/*
The general proceedings of D2X-XL when establishing a UDP/IP communication between two peers is as follows:

The sender places the destination address (IP + port) right after the game data in the data packet. 
This happens in UDPSendPacket() in the following two lines:

	memcpy (buf + 8 + dataLen, &dest->sin_addr, 4);
	memcpy (buf + 12 + dataLen, &dest->sin_port, 2);

The receiver that way gets to know the IP + port it is sending on. These are not always to be determined by 
the sender itself, as it may sit behind a NAT or proxy, or be using port 0 (in which case the OS will chose 
a port for it). The sender's IP + port are stored in the global variable networkData.packetSource (happens in 
ipx_udp.c::UDPReceivePacket()), which is needed on some special occasions.

That's my mechanism to make every participant in a game reliably know about its own IP + port.

All this starts with a client connecting to a server: Consider this the bootstrapping part of establishing 
the communication. This always works, as the client knows the servers' IP + port (if it doesn't, no 
connection ;). The server receives a game info request from the client (containing the server IP + port 
after the game data) and thus gets to know its IP + port. It replies to the client, and puts the client's 
IP + port after the end of the game data.

There is a message nType where the server tells all participants about all other participants; that's how 
clients find out about each other in a game.

When the server receives a participation request from a client, it adds its IP + port to a table containing 
all participants of the game. When the server subsequently sends data to the client, it uses that IP + port. 

This however takes only part after the client has sent a game info request and received a game info from 
the server. When the server sends that game info, it hasn't added that client to the participants table. 
Therefore, some game data contains client address data. Unfortunately, this address data is not valid in 
UDP/IP communications, and this is where we need networkData.packetSource from above: It's contents is patched into the
game data address (happens in main/network.c::NetworkProcessPacket()) and now is used by the function 
returning the game info.
*/

/* the following are the possible packet identificators.
 * they are stored in the "nType" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */

int32_t nLastNetGameUpdate [MAX_ACTIVE_NETGAMES];
CNetGameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
tExtraGameInfo activeExtraGameInfo [MAX_ACTIVE_NETGAMES];
CAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
CAllNetPlayersInfo *playerInfoP, netPlayers [2];

int32_t nCoopPenalties [10] = {0, 1, 2, 3, 5, 10, 25, 50, 75, 90};

tExtraGameInfo extraGameInfo [3];

tMpParams mpParams = {
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
 {'1', '2', '7', '.', '0', '.', '0', '.', '1', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}, 
	{UDP_BASEPORT, UDP_BASEPORT}, 
	1, 0, 0, 0, 0, 2, (int32_t) 0xFFFFFFFF, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 10
	};

tPingStats pingStats [MAX_PLAYERS];

CNetGameInfo tempNetInfo;

const char *pszRankStrings []={
	" (unpatched) ", "Cadet ", "Ensign ", "Lieutenant ", "Lt.Commander ", 
   "Commander ", "Captain ", "Vice Admiral ", "Admiral ", "Demigod "
	};

//------------------------------------------------------------------------------

void ResetAllPlayerTimeouts (void)
{
	int32_t	i, t = SDL_GetTicks ();

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	ResetPlayerTimeout (i, t);
}

//------------------------------------------------------------------------------

int32_t NetworkEndLevel (int32_t *secret)
{
// Do whatever needs to be done between levels
*secret = 0;
//NetworkFlush ();
networkData.nStatus = NETSTAT_ENDLEVEL; // We are between levels
NetworkListen ();
NetworkSendEndLevelPacket ();
ResetAllPlayerTimeouts ();
networkData.bSyncPackInited = 0;
NetworkUpdateNetGame ();
return 0;
}

//------------------------------------------------------------------------------

int32_t NetworkStartGame (void)
{
	int32_t bAutoRun;

if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!networkData.bActive) {
		TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return 0;
		}
	}

NetworkInit ();
ChangePlayerNumTo (0);
if (NetworkFindGame ()) {
	TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_NET_FULL);
	return 0;
	}
bAutoRun = InitAutoNetGame ();
PrintLog (1, "Getting network game params\n");
if (0 > NetworkGetGameParams (bAutoRun)) {
	PrintLog (-1);
	return 0;
	}
gameData.multiplayer.nPlayers = 0;
netGameInfo.m_info.difficulty = gameStates.app.nDifficultyLevel;
netGameInfo.m_info.gameMode = mpParams.nGameMode;
netGameInfo.m_info.gameStatus = NETSTAT_STARTING;
netGameInfo.m_info.nNumPlayers = 0;
netGameInfo.m_info.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
netGameInfo.m_info.SetLevel (mpParams.nLevel);
netGameInfo.m_info.protocolVersion = MULTI_PROTO_VERSION;
strcpy (netGameInfo.m_info.szGameName, mpParams.szGameName);
networkData.nStatus = NETSTAT_STARTING;
// Have the network driver initialize whatever data it wants to
// store for this netgame.
// For mcast4, this randomly chooses a multicast session and port.
// Clients subscribe to this address when they call
// IpxHandleNetGameAuxData.
IpxInitNetGameAuxData (netGameInfo.AuxData ());
NetworkSetGameMode (netGameInfo.m_info.gameMode);
gameStates.app.SRand ();
netGameInfo.m_info.nSecurity = RandShort ();  // For syncing NetGames with player packets
downloadManager.Init ();
networkThread.Start ();
if (NetworkSelectPlayers (bAutoRun)) {
	missionManager.DeleteLevelStates ();
	missionManager.SaveLevelStates ();
	SetupPowerupFilter ();
	StartNewLevel (netGameInfo.m_info.GetLevel (), true);
	ResetAllPlayerTimeouts ();
	PrintLog (-1);
	return 1;
	}
else {
	gameData.app.SetGameMode (GM_GAME_OVER);
	PrintLog (-1);
	return 0;
	}
}

//------------------------------------------------------------------------------

void RestartNetSearching (CMenu& menu)
{
gameData.multiplayer.nPlayers = 0;
networkData.nActiveGames = 0;
memset (activeNetGames, 0, sizeof (activeNetGames));
InitNetGameMenu (menu, 0);
networkData.nNamesInfoSecurity = -1;
networkData.bGamesChanged = 1;      
}

//------------------------------------------------------------------------------

void NetworkLeaveGame (bool bDisconnect)
{ 
if (bDisconnect) { 
	NetworkDoFrame (1);
	#ifdef NETPROFILING
	fclose (SendLogFile);
		fclose (ReceiveLogFile);
	#endif
	if ((IAmGameHost ())) {
		bool bSyncExtras = true;

		while (bSyncExtras) {
			bSyncExtras = false;
			for (int16_t i = 0; i < networkData.nJoining; i++)
				if (networkData.sync [i].nExtras && (networkData.sync [i].nExtrasPlayer != -1)) {
					NetworkSyncExtras (networkData.sync + i);
					bSyncExtras  = true;
					}
			}

		netGameInfo.m_info.nNumPlayers = 0;
		int32_t h = gameData.multiplayer.nPlayers;
		gameData.multiplayer.nPlayers = 0;
		NetworkSendGameInfo (NULL);
		gameData.multiplayer.nPlayers = h;
		}
	}
CONNECT (N_LOCALPLAYER, CONNECT_DISCONNECTED);
NetworkSendEndLevelPacket ();
gameData.app.SetGameMode (GM_GAME_OVER);
IpxHandleLeaveGame ();
NetworkFlush ();
ChangePlayerNumTo (0);
if (gameStates.multi.nGameType != UDP_GAME)
	SavePlayerProfile ();
memcpy (extraGameInfo, extraGameInfo + 2, sizeof (extraGameInfo [0]));
gameData.multiplayer.nPlayers = 1;
}

//------------------------------------------------------------------------------

void NetworkFlush (void)
{
if ((gameStates.multi.nGameType >= IPX_GAME) && !networkData.bActive) 
	return;

if (networkThread.Available ())
	networkThread.FlushPackets ();
else {
	uint8_t packet [MAX_PACKET_SIZE];
	while (IpxGetPacketData (packet) > 0) 
		 ;
	}
}

//------------------------------------------------------------------------------

int32_t NetworkTimeoutPlayer (int32_t nPlayer, int32_t t)
{
if (!gameOpts->multi.bTimeoutPlayers)
	return 0;

CObject* objP = (LOCALPLAYER.connected == CONNECT_PLAYING) ? gameData.Object (gameData.multiplayer.players [nPlayer].nObject) : NULL;

if (gameData.multiplayer.players [nPlayer].TimedOut ()) {
	if (t - gameData.multiplayer.players [nPlayer].m_tDisconnect > TIMEOUT_KICK) { // drop player when he disconnected for 3 minutes
		gameData.multiplayer.players [nPlayer].callsign [0] = '\0';
		memset (gameData.multiplayer.players [nPlayer].netAddress, 0, sizeof (gameData.multiplayer.players [nPlayer].netAddress));
		if (objP)
			MultiDestroyPlayerShip (nPlayer);
		}
#if 0
	if (objP && (objP->Type () == OBJ_GHOST))
#endif
		return 0;
	}

// Remove a player from the game if we haven't heard from them in a long time.
NetworkDisconnectPlayer (nPlayer);
if ((LOCALPLAYER.connected == CONNECT_END_MENU) || (LOCALPLAYER.connected == CONNECT_ADVANCE_LEVEL)) {
	if ((gameStates.multi.nGameType != UDP_GAME) || IAmGameHost ())
		NetworkSendEndLevelSub (nPlayer);
	}
else if (objP) {
	objP->CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	HUDInitMessage ("%s %s", gameData.multiplayer.players [nPlayer].callsign, TXT_DISCONNECTING);
	}
#if !DBG
int32_t n = 0;
for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].IsConnected ()) 
		n++;
if (n == 1)
	MultiOnlyPlayerMsg (0);
#endif
return 1;
}

//------------------------------------------------------------------------------

int32_t NetworkListen (void)
{
#if DBG
if (!IsNetworkGame && (gameStates.app.nFunctionMode == FMODE_GAME))
	console.printf (CON_DBG, "Calling NetworkListen () when not in net game.\n");
#endif

if ((gameStates.multi.nGameType < IPX_GAME) || !networkData.bActive) 
	return -1;

networkData.bWaitingForPlayerInfo = 1;
networkData.nSecurityFlag = NETSECURITY_OFF;

if (networkThread.Available ())
	return networkThread.ProcessPackets ();

tracker.AddServer ();

int32_t nQueries = 
	((networkData.nStatus == NETSTAT_PLAYING) && netGameInfo.GetShortPackets () && !networkData.nJoining)
	? gameData.multiplayer.nPlayers * MinPPS ()
	: 999;

uint8_t packet [MAX_PACKET_SIZE];
int32_t nPackets = 0;
int32_t t = SDL_GetTicks ();
int32_t size;

for (; (size = IpxGetPacketData (packet)) && nQueries && (SDL_GetTicks () - t < 50); nQueries--)
	if (NetworkProcessPacket (packet, size))
		nPackets++;

return nPackets;
}

//------------------------------------------------------------------------------

void CSyncPackShort::Squish (void)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	int32_t 	i;
	
m_info.header.nPackets = INTEL_INT (m_info.header.nPackets);
for (int32_t i = 0; i < 3; i++) {
	m_info.objData.pos [i] = INTEL_SHORT (m_info.objData.pos [i]);
	m_info.objData.vel [i] = INTEL_SHORT (m_info.objData.vel [i]);
	}
m_info.objData.nSegment = INTEL_SHORT (m_info.objData.nSegment);
m_info.data.dataSize = INTEL_SHORT (m_info.data.dataSize);
#endif
}

//------------------------------------------------------------------------------

void CSyncPackLong::Squish (void)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
if (gameStates.multi.nGameType >= IPX_GAME) {
	m_info.header.nPackets = INTEL_INT (m_info.header.nPackets);
	INTEL_VECTOR (m_info.objData.pos);
	INTEL_MATRIX (m_info.objData.orient);
	INTEL_VECTOR (m_info.objData.vel);
	INTEL_VECTOR (m_info.objData.rotVel);
	m_info.objData.nSegment = INTEL_SHORT (m_info.objData.nSegment);
	m_info.data.dataSize = INTEL_SHORT (m_info.data.dataSize);
	}
#endif
}

//------------------------------------------------------------------------------

#if DBG

void ResetPlayerTimeout (int32_t nPlayer, fix t)
{
	static int32_t nDbgPlayer = -1;

if (nPlayer == nDbgPlayer)
	BRP;
networkData.nLastPacketTime [nPlayer] = (t < 0) ? (fix) SDL_GetTicks () : t;
}

#endif

//------------------------------------------------------------------------------

void NetworkAdjustPPS (void)
{
	static CTimeout to (10000);

if (to.Expired ()) {
	int32_t nMaxPing = 0;
	for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (i == N_LOCALPLAYER)
			continue;
		if (!gameData.multiplayer.players [i].Connected (CONNECT_PLAYING))
			continue;
		if (nMaxPing < pingStats [i].averagePing)
			nMaxPing = pingStats [i].averagePing;
		}
	mpParams.nMinPPS = Clamp (2000 / nMaxPing, MIN_PPS, MAX_PPS);
	}
}

//------------------------------------------------------------------------------

void CSyncPack::Prepare (void) 
{
SetType (PID_PDATA);
SetPlayer (N_LOCALPLAYER);
SetRenderType (OBJECTS [LOCALPLAYER.nObject].info.renderType);
SetLevel (missionManager.nCurrentLevel);
SetObjInfo (OBJECTS + LOCALPLAYER.nObject);
Squish ();
}

//------------------------------------------------------------------------------

void CSyncPack::Send (void) 
{
	static CTimeout toEndLevel (500);

if (IsMultiGame && LOCALPLAYER.IsConnected ()) {
	MultiSendRobotFrame (0);
	MultiSendFire (); // Do firing if needed..
	Prepare ();
	IpxSendGamePacket (reinterpret_cast<uint8_t*>(Info ()), Size ());
	++m_nPackets; 
	networkData.bD2XData = 0;
	if (gameData.reactor.bDestroyed) {
		if (gameStates.app.bPlayerIsDead)
			CONNECT (N_LOCALPLAYER, CONNECT_DIED_IN_MINE);
		if (toEndLevel.Expired ())
			NetworkSendEndLevelPacket ();
		}
	}
Reset ();
}

//------------------------------------------------------------------------------

void NetworkFlushData (void)
{
networkData.SyncPack ().Send (); 
}

//------------------------------------------------------------------------------

static int32_t SyncTimeout (void)
{
return I2X (1) / (networkThread.Available () ? networkThread.MinPPS () : MinPPS ());
}

//------------------------------------------------------------------------------

void NetworkDoFrame (int bFlush)
{
if (!IsNetworkGame) 
	return;
if ((networkData.nStatus == NETSTAT_PLAYING) && !gameStates.app.bEndLevelSequence) { // Don't send postion during escape sequence...
	nakedData.Flush ();
	if (networkData.refuse.bWaitForAnswer && TimerGetApproxSeconds () > (networkData.refuse.xTimeLimit + (I2X (12))))
		networkData.refuse.bWaitForAnswer = 0;
	networkData.xLastSendTime += gameData.time.xFrame;
	//networkData.xLastTimeoutCheck += gameData.time.xFrame;

	// Send out packet PacksPerSec times per second maximum... unless they fire, then send more often...
	if ((networkData.xLastSendTime >= SyncTimeout ())

#if !DBG
		 || (/*(networkData.xLastSendTime >= I2X (1) / MAX_PPS) &&*/ (gameData.multigame.laser.bFired || networkData.bPacketUrgent))
#endif
		) {
		networkThread.SetUrgent (gameData.multigame.laser.bFired || networkData.bPacketUrgent);
		NetworkFlushData ();
		}

	if (!networkThread.Available ())
		networkThread.CheckPlayerTimeouts ();
	//NetworkCheckPlayerTimeouts ();
	}
NetworkListen ();
#if 0
if ((networkData.sync [0].nPlayer != -1) && !(gameData.app.nFrameCount & 63))
	ResendSyncDueToPacketLoss (); // This will resend to network_player_rejoining
#endif
XMLGameInfoHandler ();
NetworkDoSyncFrame ();
//NetworkAdjustPPS ();
}

//------------------------------------------------------------------------------

void NetworkConsistencyError (void)
{
if (++networkData.nConsistencyErrorCount < 10)
	return;
SetFunctionMode (FMODE_MENU);
PauseGame ();
TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
ResumeGame ();
SetFunctionMode (FMODE_GAME);
networkData.nConsistencyErrorCount = 0;
gameData.multigame.bQuitGame = 1;
gameData.multigame.menu.bLeave = 1;
MultiResetStuff ();
SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

void NetworkPing (uint8_t flag, uint8_t nPlayer)
{
if (gameStates.multi.nGameType >= IPX_GAME) {
	uint8_t mybuf [3];
	mybuf [0] = flag;
	mybuf [1] = N_LOCALPLAYER;
	if (gameStates.multi.nGameType == UDP_GAME)
		mybuf [2] = LOCALPLAYER.connected;
	networkThread.Send (
		reinterpret_cast<uint8_t*> (mybuf), (gameStates.multi.nGameType == UDP_GAME) ? 3 : 2, 
		netPlayers [0].m_info.players [nPlayer].network.Network (), 
		netPlayers [0].m_info.players [nPlayer].network.Node (), 
		gameData.multiplayer.players [nPlayer].netAddress);
	}
}

//------------------------------------------------------------------------------

void NetworkHandlePingReturn (uint8_t nPlayer)
{
if ((nPlayer >= gameData.multiplayer.nPlayers) || !pingStats [nPlayer].launchTime) {
#if 1			
	 console.printf (CON_DBG, "Got invalid PING RETURN from %s!\n", gameData.multiplayer.players [nPlayer].callsign);
#endif
   return;
	}
if (pingStats [nPlayer].launchTime > 0) { // negative value suppresses display of returned ping on HUD
	//xPingReturnTime = TimerGetFixedSeconds ();
	pingStats [nPlayer].received++;
	pingStats [nPlayer].ping = SDL_GetTicks () - pingStats [nPlayer].launchTime; //X2I (FixMul (xPingReturnTime - pingStats [nPlayer].launchTime, I2X (1000)));
	pingStats [nPlayer].totalPing += pingStats [nPlayer].ping;
	pingStats [nPlayer].averagePing = pingStats [nPlayer].totalPing / pingStats [nPlayer].received;
	if (!gameStates.render.cockpit.bShowPingStats)
		HUDInitMessage ("Ping time for %s is %d ms!", gameData.multiplayer.players [nPlayer].callsign, pingStats [nPlayer].ping);
	pingStats [nPlayer].launchTime = 0;
	}
}

//------------------------------------------------------------------------------

void NetworkSendPing (uint8_t nPlayer)
{
NetworkPing (PID_PING_SEND, nPlayer);
}  

//------------------------------------------------------------------------------

#ifdef NETPROFILING

void OpenSendLog (void)
 {
  int32_t i;

SendLogFile = reinterpret_cast<FILE*> (fopen ("sendlog.net", "w"));
for (i = 0; i < 100; i++)
	TTSent [i] = 0;
 }

 //------------------------------------------------------------------------------

void OpenReceiveLog (void)
 {
int32_t i;

ReceiveLogFile= reinterpret_cast<FILE*> (fopen ("recvlog.net", "w"));
for (i = 0; i < 100; i++)
	TTRecv [i] = 0;
 }
#endif

//------------------------------------------------------------------------------

int32_t GetMyNetRanking (void)
 {
if (networkData.nNetLifeKills + networkData.nNetLifeKilled == 0)
	return 1;
int32_t rank = (int32_t) (((double) networkData.nNetLifeKills / 3000.0) * 8.0);
int32_t eff = (int32_t) ((double) ((double)networkData.nNetLifeKills / ((double) networkData.nNetLifeKilled + (double) networkData.nNetLifeKills)) * 100.0);
if (rank > 8)
	rank = 8;
if (eff < 0)
	eff = 0;
if (eff < 60)
	rank-= ((59 - eff) / 10);
if (rank < 0)
	rank = 0;
else if (rank > 8)
	rank = 8;

#if 1			
console.printf (CON_DBG, "Rank is %d (%s)\n", rank+1, pszRankStrings [rank+1]);
#endif
return rank + 1;
 }

//------------------------------------------------------------------------------

void NetworkCheckForOldVersion (uint8_t nPlayer)
{  
if ((netPlayers [0].m_info.players [(int32_t) nPlayer].versionMajor == 1) && 
	 !(netPlayers [0].m_info.players [(int32_t) nPlayer].versionMinor & 0x0F))
	netPlayers [0].m_info.players [(int32_t) nPlayer].rank = 0;
}

//------------------------------------------------------------------------------

