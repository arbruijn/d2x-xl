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

#include "inferno.h"
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
#include "playsave.h"
#include "text.h"

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
a port for it). The sender's IP + port are stored in the global variable ipx_udpSrc (happens in 
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
UDP/IP communications, and this is where we need ipx_udpSrc from above: It's contents is patched into the
game data address (happens in main/network.c::NetworkProcessPacket()) and now is used by the function 
returning the game info.
*/

/* the following are the possible packet identificators.
 * they are stored in the "nType" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */

int nLastNetGameUpdate [MAX_ACTIVE_NETGAMES];
tNetgameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
tExtraGameInfo activeExtraGameInfo [MAX_ACTIVE_NETGAMES];
tAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
tAllNetPlayersInfo *tmpPlayersInfo, tmpPlayersBase;

int nCoopPenalties [10] = {0, 1, 2, 3, 5, 10, 25, 50, 75, 90};

tExtraGameInfo extraGameInfo [2];

tMpParams mpParams = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
	{'1', '2', '7', '.', '0', '.', '0', '.', '1', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}, 
	UDP_BASEPORT, 
	1, 0, 0, 0, 0, 2, 0xFFFFFFFF, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 10};

tPingStats pingStats [MAX_PLAYERS];

tNetgameInfo tempNetInfo;

const char *pszRankStrings []={
	" (unpatched) ", "Cadet ", "Ensign ", "Lieutenant ", "Lt.Commander ", 
   "Commander ", "Captain ", "Vice Admiral ", "Admiral ", "Demigod "
	};

tNakedData nakedData = {0, -1};

tNetworkData networkData;

//------------------------------------------------------------------------------

void ResetAllPlayerTimeouts (void)
{
	int	i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	ResetPlayerTimeout (i, -1);
}

//------------------------------------------------------------------------------

int NetworkEndLevel (int *secret)
{
	// Do whatever needs to be done between levels

	int	i;
	fix	t = TimerGetApproxSeconds ();

*secret=0;
//NetworkFlush ();
networkData.nStatus = NETSTAT_ENDLEVEL; // We are between levels
NetworkListen ();
NetworkSendEndLevelPacket ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) 
	ResetPlayerTimeout (i, t);
NetworkSendEndLevelPacket ();
NetworkSendEndLevelPacket ();
networkData.bSyncPackInited = 0;
NetworkUpdateNetGame ();
return 0;
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
void DumpSegments (void)
{
	FILE * fp;

fp = fopen ("TEST.DMP", "wb");
fwrite (gameData.segs.segments, sizeof (tSegment)* (gameData.segs.nLastSegment+1), 1, fp);    
fclose (fp);
con_printf (CONDBG, "SS=%d\n", sizeof (tSegment));
}
#endif

//------------------------------------------------------------------------------

int NetworkStartGame (void)
{
	int i, bAutoRun;

if (gameStates.multi.nGameType >= IPX_GAME) {
	Assert (FRAME_INFO_SIZE < DATALIMIT);
	if (!networkData.bActive) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return 0;
		}
	}

NetworkInit ();
ChangePlayerNumTo (0);
if (NetworkFindGame ()) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_NET_FULL);
	return 0;
	}
bAutoRun = InitAutoNetGame ();
if (0 > (i = NetworkGetGameParams (bAutoRun)))
	return 0;
gameData.multiplayer.nPlayers = 0;
netGame.difficulty = gameStates.app.nDifficultyLevel;
netGame.gameMode = mpParams.nGameMode;
netGame.gameStatus = NETSTAT_STARTING;
netGame.nNumPlayers = 0;
netGame.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
netGame.nLevel = mpParams.nLevel;
netGame.protocolVersion = MULTI_PROTO_VERSION;
strcpy (netGame.szGameName, mpParams.szGameName);
networkData.nStatus = NETSTAT_STARTING;
// Have the network driver initialize whatever data it wants to
// store for this netgame.
// For mcast4, this randomly chooses a multicast session and port.
// Clients subscribe to this address when they call
// IpxHandleNetGameAuxData.
IpxInitNetGameAuxData (netGame.AuxData);
NetworkSetGameMode (netGame.gameMode);
d_srand (TimerGetFixedSeconds ());
netGame.nSecurity = d_rand ();  // For syncing Netgames with tPlayer packets
if (NetworkSelectPlayers (bAutoRun)) {
	StartNewLevel (netGame.nLevel, 0);
	ResetAllPlayerTimeouts ();
	return 1;
	}
else {
	gameData.app.nGameMode = GM_GAME_OVER;
	return 0;
	}
}

//------------------------------------------------------------------------------

void RestartNetSearching (tMenuItem * m)
{
gameData.multiplayer.nPlayers = 0;
networkData.nActiveGames = 0;
memset (activeNetGames, 0, sizeof (tNetgameInfo) * MAX_ACTIVE_NETGAMES);
InitNetgameMenu (m, 0);
networkData.nNamesInfoSecurity = -1;
networkData.bGamesChanged = 1;      
}

//------------------------------------------------------------------------------

void NetworkLeaveGame (void)
{ 
   int nsave;
	
NetworkDoFrame (1, 1);
#ifdef NETPROFILING
fclose (SendLogFile);
	fclose (ReceiveLogFile);
#endif
if ((NetworkIAmMaster ())) {
	bool bSyncExtras = true;

	while (bSyncExtras) {
		bSyncExtras = false;
		for (short i = 0; i < networkData.nJoining; i++)
			if (networkData.sync [i].nExtras && (networkData.sync [i].nExtrasPlayer != -1)) {
				NetworkSyncExtras (networkData.sync + i);
				bSyncExtras  = true;
				}
		}

	netGame.nNumPlayers = 0;
	nsave = gameData.multiplayer.nPlayers;
	gameData.multiplayer.nPlayers = 0;
	NetworkSendGameInfo (NULL);
	gameData.multiplayer.nPlayers = nsave;
	}
LOCALPLAYER.connected = 0;
NetworkSendEndLevelPacket ();
ChangePlayerNumTo (0);
gameData.app.nGameMode = GM_GAME_OVER;
WritePlayerFile ();
ipx_handle_leave_game ();
NetworkFlush ();
}

//------------------------------------------------------------------------------

void NetworkFlush (void)
{
	ubyte packet [MAX_PACKETSIZE];

if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return;
while (IpxGetPacketData (packet) > 0) 
	 ;
}

//------------------------------------------------------------------------------

void NetworkTimeoutPlayer (int nPlayer)
{
	// Remove a tPlayer from the game if we haven't heard from them in 
	// a long time.
	int i, n = 0;

Assert (nPlayer < gameData.multiplayer.nPlayers);
Assert (nPlayer > -1);
NetworkDisconnectPlayer (nPlayer);
CreatePlayerAppearanceEffect (OBJECTS + gameData.multiplayer.players [nPlayer].nObject);
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
HUDInitMessage ("%s %s", gameData.multiplayer.players [nPlayer].callsign, TXT_DISCONNECTING);
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected) 
		n++;
#ifndef _DEBUG
if (n == 1)
	MultiOnlyPlayerMsg (0);
#endif
}

//------------------------------------------------------------------------------

int NetworkListen (void)
{
	int size;
	ubyte packet [MAX_PACKETSIZE];
	int i, t, nPackets = 0, nMaxLoops = 999;

CleanUploadDests ();
if (NetworkIAmMaster ())
	AddServerToTracker ();
if ((networkData.nStatus == NETSTAT_PLAYING) && netGame.bShortPackets && !networkData.nJoining)
	nMaxLoops = gameData.multiplayer.nPlayers * PacketsPerSec ();

if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return -1;
#if 1			
if (!(gameData.app.nGameMode & GM_NETWORK) && (gameStates.app.nFunctionMode == FMODE_GAME))
	con_printf (CONDBG, "Calling NetworkListen () when not in net game.\n");
#endif
networkData.bWaitingForPlayerInfo = 1;
networkData.nSecurityFlag = NETSECURITY_OFF;

t = SDL_GetTicks ();
if (gameStates.multi.nGameType >= IPX_GAME) {
	for (i = nPackets = 0; (i < nMaxLoops) && (SDL_GetTicks () - t < 50); i++) {
		size = IpxGetPacketData (packet);
		if (size <= 0)
			break;
		if (NetworkProcessPacket (packet, size))
			nPackets++;
		}
	}
return nPackets;
}

//------------------------------------------------------------------------------

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)

void SquishShortFrameInfo (tFrameInfoShort old_info, ubyte *data)
{
	int 	bufI = 0;
	int 	tmpi;
	short tmps;
	
NW_SET_BYTE (data, bufI, old_info.nType);                                            
bufI += 3;
NW_SET_INT (data, bufI, old_info.numpackets);
NW_SET_BYTES (data, bufI, old_info.thepos.bytemat, 9);                   
NW_SET_SHORT (data, bufI, old_info.thepos.xo);
NW_SET_SHORT (data, bufI, old_info.thepos.yo);
NW_SET_SHORT (data, bufI, old_info.thepos.zo);
NW_SET_SHORT (data, bufI, old_info.thepos.nSegment);
NW_SET_SHORT (data, bufI, old_info.thepos.velx);
NW_SET_SHORT (data, bufI, old_info.thepos.vely);
NW_SET_SHORT (data, bufI, old_info.thepos.velz);
NW_SET_SHORT (data, bufI, old_info.data_size);
NW_SET_BYTE (data, bufI, old_info.nPlayer);                                     
NW_SET_BYTE (data, bufI, old_info.obj_renderType);                               
NW_SET_BYTE (data, bufI, old_info.level_num);                                     
NW_SET_BYTES (data, bufI, old_info.data, old_info.data_size);
}

#endif

//------------------------------------------------------------------------------

void NetworkDoFrame (int bForce, int bListen)
{
	tFrameInfoShort ShortSyncPack;
	static fix xLastEndlevel = 0;
	int i;

if (!(gameData.app.nGameMode & GM_NETWORK)) 
	return;
if ((networkData.nStatus == NETSTAT_PLAYING) && !gameStates.app.bEndLevelSequence) { // Don't send postion during escape sequence...
	if (nakedData.nLength) {
		Assert (nakedData.nDestPlayer >- 1);
		if (gameStates.multi.nGameType >= IPX_GAME) 
			IPXSendPacketData ((ubyte *) nakedData.buf, nakedData.nLength, 
									netPlayers.players [nakedData.nDestPlayer].network.ipx.server, 
									netPlayers.players [nakedData.nDestPlayer].network.ipx.node, 
									gameData.multiplayer.players [nakedData.nDestPlayer].netAddress);
		nakedData.nLength = 0;
		nakedData.nDestPlayer = -1;
		}
	if (networkData.refuse.bWaitForAnswer && TimerGetApproxSeconds ()> (networkData.refuse.xTimeLimit+ (F1_0*12)))
		networkData.refuse.bWaitForAnswer=0;
	networkData.xLastSendTime += gameData.time.xFrame;
	networkData.xLastTimeoutCheck += gameData.time.xFrame;

	// Send out packet PacksPerSec times per second maximum... unless they fire, then send more often...
	if ((networkData.xLastSendTime > F1_0 / PacketsPerSec ()) || 
		(gameData.multigame.laser.bFired) || bForce || networkData.bPacketUrgent) {        
		if (LOCALPLAYER.connected) {
			int nObject = LOCALPLAYER.nObject;
			networkData.bPacketUrgent = 0;
			if (bListen) {
				MultiSendRobotFrame (0);
				MultiSendFire ();              // Do firing if needed..
				}
			networkData.xLastSendTime = 0;
			if (netGame.bShortPackets) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
				ubyte send_data [MAX_PACKETSIZE];
#endif
				memset (&ShortSyncPack, 0, sizeof (ShortSyncPack));
				CreateShortPos (&ShortSyncPack.thepos, OBJECTS+nObject, 0);
				ShortSyncPack.nType = PID_PDATA;
				ShortSyncPack.nPlayer = gameData.multiplayer.nLocalPlayer;
				ShortSyncPack.obj_renderType = OBJECTS [nObject].renderType;
				ShortSyncPack.level_num = gameData.missions.nCurrentLevel;
				ShortSyncPack.data_size = networkData.syncPack.data_size;
				memcpy (ShortSyncPack.data, networkData.syncPack.data, networkData.syncPack.data_size);
				networkData.syncPack.numpackets = INTEL_INT (gameData.multiplayer.players [0].nPacketsSent++);
				ShortSyncPack.numpackets = networkData.syncPack.numpackets;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
				IpxSendGamePacket (
					(ubyte*)&ShortSyncPack, 
					sizeof (tFrameInfoShort) - networkData.nMaxXDataSize + networkData.syncPack.data_size);
#else
				SquishShortFrameInfo (ShortSyncPack, send_data);
				IpxSendGamePacket (
					(ubyte*)send_data, 
					IPX_SHORT_INFO_SIZE-networkData.nMaxXDataSize+networkData.syncPack.data_size);
#endif
				}
			else {// If long packets
					int send_data_size;

				networkData.syncPack.nType = PID_PDATA;
				networkData.syncPack.nPlayer = gameData.multiplayer.nLocalPlayer;
				networkData.syncPack.obj_renderType = OBJECTS [nObject].renderType;
				networkData.syncPack.level_num = gameData.missions.nCurrentLevel;
				networkData.syncPack.obj_segnum = OBJECTS [nObject].nSegment;
				networkData.syncPack.obj_pos = OBJECTS [nObject].position.vPos;
				networkData.syncPack.obj_orient = OBJECTS [nObject].position.mOrient;
				networkData.syncPack.phys_velocity = OBJECTS [nObject].mType.physInfo.velocity;
				networkData.syncPack.phys_rotvel = OBJECTS [nObject].mType.physInfo.rotVel;
				send_data_size = networkData.syncPack.data_size;                  // do this so correct size data is sent
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)                        // do the swap stuff
				if (gameStates.multi.nGameType >= IPX_GAME) {
					networkData.syncPack.obj_segnum = INTEL_SHORT (networkData.syncPack.obj_segnum);
					INTEL_VECTOR (&networkData.syncPack.obj_pos);
					INTEL_MATRIX (&networkData.syncPack.obj_orient);
					INTEL_VECTOR (&networkData.syncPack.phys_velocity);
					INTEL_VECTOR (&networkData.syncPack.phys_rotvel);
					networkData.syncPack.data_size = INTEL_SHORT (networkData.syncPack.data_size);
					}
#endif
				networkData.syncPack.numpackets = INTEL_INT (gameData.multiplayer.players [0].nPacketsSent++);
				IpxSendGamePacket (
					(ubyte*)&networkData.syncPack, 
					sizeof (tFrameInfo) - networkData.nMaxXDataSize + send_data_size);
				}
			networkData.syncPack.data_size = 0;               // Start data over at 0 length.
			networkData.bD2XData = 0;
			if (gameData.reactor.bDestroyed) {
				if (gameStates.app.bPlayerIsDead)
					LOCALPLAYER.connected=3;
				if (TimerGetApproxSeconds () > (xLastEndlevel+ (F1_0/2))) {
					NetworkSendEndLevelPacket ();
					xLastEndlevel = TimerGetApproxSeconds ();
					}
				}
			}
		}

	if (!bListen)
		return;

	if ((networkData.xLastTimeoutCheck > F1_0) && !gameData.reactor.bDestroyed) {
		fix t = (fix) SDL_GetTicks ();
	// Check for tPlayer timeouts
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if ((i != gameData.multiplayer.nLocalPlayer) && 
				((gameData.multiplayer.players [i].connected == 1) || bDownloading [i])) {
				if ((networkData.nLastPacketTime [i] == 0) || 
					(networkData.nLastPacketTime [i] > t)) {
					ResetPlayerTimeout (i, t);
					continue;
					}
#if 1//ndef _DEBUG
				if (gameOpts->multi.bTimeoutPlayers && (t - networkData.nLastPacketTime [i] > 15000))
					NetworkTimeoutPlayer (i);
#endif
				}
			}
		networkData.xLastTimeoutCheck = 0;
		}
	}

if (!bListen) {
	networkData.syncPack.data_size = 0;
	return;
	}
NetworkListen ();
#if 0
if ((networkData.sync.nPlayer != -1) && !(gameData.app.nFrameCount & 63))
	ResendSyncDueToPacketLoss (); // This will resend to network_player_rejoining
#endif
NetworkDoSyncFrame ();
if (NetworkIAmMaster ())
	AddServerToTracker ();
}

//------------------------------------------------------------------------------

void NetworkConsistencyError (void)
{
if (++networkData.nConsistencyErrorCount < 10)
	return;
SetFunctionMode (FMODE_MENU);
ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
SetFunctionMode (FMODE_GAME);
networkData.nConsistencyErrorCount = 0;
gameData.multigame.bQuitGame = 1;
gameData.multigame.menu.bLeave = 1;
MultiResetStuff ();
SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

void NetworkPing (ubyte flag, int nPlayer)
{
	ubyte mybuf [2];

mybuf [0]=flag;
mybuf [1]=gameData.multiplayer.nLocalPlayer;
if (gameStates.multi.nGameType >= IPX_GAME)
	IPXSendPacketData (
			(ubyte *)mybuf, 2, 
		netPlayers.players [nPlayer].network.ipx.server, 
		netPlayers.players [nPlayer].network.ipx.node, 
		gameData.multiplayer.players [nPlayer].netAddress);
}

//------------------------------------------------------------------------------

void NetworkHandlePingReturn (ubyte nPlayer)
{
if ((nPlayer >= gameData.multiplayer.nPlayers) || !pingStats [nPlayer].launchTime) {
#if 1			
	 con_printf (CONDBG, "Got invalid PING RETURN from %s!\n", gameData.multiplayer.players [nPlayer].callsign);
#endif
   return;
	}
xPingReturnTime = TimerGetFixedSeconds ();
pingStats [nPlayer].ping = f2i (FixMul (xPingReturnTime - pingStats [nPlayer].launchTime, i2f (1000)));
if (!gameStates.render.cockpit.bShowPingStats)
	HUDInitMessage ("Ping time for %s is %d ms!", gameData.multiplayer.players [nPlayer].callsign, pingStats [nPlayer].ping);
pingStats [nPlayer].launchTime = 0;
pingStats [nPlayer].received++;
}

//------------------------------------------------------------------------------

void NetworkSendPing (ubyte pnum)
{
NetworkPing (PID_PING_SEND, pnum);
}  

//------------------------------------------------------------------------------

#ifdef NETPROFILING

void OpenSendLog (void)
 {
  int i;

SendLogFile= (FILE *)fopen ("sendlog.net", "w");
for (i = 0; i < 100; i++)
	TTSent [i] = 0;
 }

 //------------------------------------------------------------------------------

void OpenReceiveLog (void)
 {
int i;

ReceiveLogFile= (FILE *)fopen ("recvlog.net", "w");
for (i = 0; i < 100; i++)
	TTRecv [i] = 0;
 }
#endif

//------------------------------------------------------------------------------

int GetMyNetRanking (void)
 {
  int rank;
  int eff;

if (networkData.nNetLifeKills+networkData.nNetLifeKilled == 0)
	return 1;
rank = (int) (((double)networkData.nNetLifeKills/3000.0)*8.0);
eff = (int) ((double) ((double)networkData.nNetLifeKills/ ((double)networkData.nNetLifeKilled+ (double)networkData.nNetLifeKills))*100.0);
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
con_printf (CONDBG, "Rank is %d (%s)\n", rank+1, pszRankStrings [rank+1]);
#endif
return (rank+1);
 }

//------------------------------------------------------------------------------

void NetworkCheckForOldVersion (char pnum)
{  
if ((netPlayers.players [(int) pnum].versionMajor == 1) && 
	 !(netPlayers.players [(int) pnum].versionMinor & 0x0F))
	netPlayers.players [ (int) pnum].rank = 0;
}

//------------------------------------------------------------------------------

