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
#include "error.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "gameseq.h"
#include "fireball.h"
#include "network.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "menu.h"
#include "sounds.h"
#include "text.h"
#include "kmatrix.h"
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
#include "ipx.h"
#include "ipx_udp.h"
#include "ipx_drv.h"
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

#define SECURITY_CHECK	1

#define LHX (x)      (gameStates.menus.bHires?2* (x):x)
#define LHY (y)      (gameStates.menus.bHires? (24* (y))/10:y)

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

// MWAnetgame_info *TempNetInfo; 
tNetgameInfo TempNetInfo;

extern struct ipx_recv_data ipx_udpSrc;

extern void SetFunctionMode (int);
extern unsigned char ipx_MyAddress [10];

extern void MultiSendDropMarker (int tPlayer, vmsVector position, char messagenum, char text []);
extern void MultiSendKillGoalCounts ();

void NetworkProcessNakedPData (char *, int);
extern void MultiSendRobotControls (char);

int  NetworkListen ();
void NetworkUpdateNetGame ();
void NetworkSendObjects ();
void NetworkSendRejoinSync (int nPlayer);
void NetworkSendEndLevelShortSub (int from_player_num, int to_player);
void NetworkReadSyncPacket (tNetgameInfo * sp, int rsinit);
int  NetworkWaitForPlayerInfo ();
int  NetworkProcessPacket (ubyte *data, int length);
void NetworkProcessPData (char *data);
void NetworkReadObjectPacket (ubyte *data);
void NetworkReadEndLevelPacket (ubyte *data);
void NetworkReadEndLevelShortPacket (ubyte *data);
void NetworkPing (ubyte flat, int pnum);
void NetworkHandlePingReturn (ubyte pnum);
void NetworkProcessNamesReturn (char *data);
void NetworkSendPlayerNames (tSequencePacket *their);
void NetworkCountPowerupsInMine ();
void NetworkDoBigWait (int choice);
void NetworkSendExtras ();
void NetworkReadPDataPacket (tFrameInfo *pd);
void NetworkReadPDataShortPacket (tFrameInfoShort *pd);
int NetworkVerifyObjects (int nRemoteObjNum, int nLocalObjs);

void DoRefuseStuff (tSequencePacket *their);
int  GetNewPlayerNumber (tSequencePacket *their);
void SetAllAllowablesTo (int on);
void LogExtraGameInfo ();

#if 1
#	define	SET_INT(_b, _loc, _i)		*((int *) ((_b) + (_loc))) = INTEL_INT ((int) (_i)); (_loc) += 4
#	define	SET_SHORT(_b, _loc, _i)		*((short *) ((_b) + (_loc))) = INTEL_SHORT ((short) (_i)); (_loc) += 2
#	define	GET_INT(_b, _loc, _i)		(_i) = INTEL_INT (*((int *) ((_b) + (_loc)))); (_loc) += 4
#	define	GET_SHORT(_b, _loc, _i)		(_i) = INTEL_SHORT (*((short *) ((_b) + (_loc)))); (_loc) += 2
#else
#	define	SET_INT(_b, _loc, _i)		{int tmpi = INTEL_INT (_i); memcpy ((_b) + (_loc), &tmpi, 4); (_loc) += 4; }
#	define	SET_SHORT(_b, _loc, _i)		{int tmps = INTEL_SHORT (_i); memcpy ((_b) + (_loc), &tmpi, 2); (_loc) += 2; }
#endif
#define	SET_BYTE(_b, _loc, _i)			(_b) [(_loc)++] = (ubyte) (_i)
#define	SET_BYTES(_b, _loc, _p, _n)	memcpy ((_b) + (_loc), _p, _n); (_loc) += (_n)
#define	GET_BYTE(_b, _loc, _i)			(_i) = (_b) [(_loc)++]
#define	GET_BYTES(_b, _loc, _p, _n)	memcpy (_p, (_b) + (_loc), _n); (_loc) += (_n)

//------------------------------------------------------------------------------

tNetworkData networkData;

char *pszRankStrings []={
	" (unpatched) ", "Cadet ", "Ensign ", "Lieutenant ", "Lt.Commander ", 
   "Commander ", "Captain ", "Vice Admiral ", "Admiral ", "Demigod "
	};

//------------------------------------------------------------------------------

void InitNetworkData (void)
{
memset (&networkData, 0, sizeof (networkData));
networkData.nActiveGames = 0;
networkData.nActiveGames = 0;
networkData.nLastActiveGames = 0;
networkData.nPacketsPerSec = 10;
networkData.nNamesInfoSecurity = -1;
networkData.nMaxXDataSize = NET_XDATA_SIZE;
networkData.nNetLifeKills = 0;
networkData.nNetLifeKilled = 0;
networkData.bDebug = 0;
networkData.bActive = 0;
networkData.nStatus = 0;
networkData.bGamesChanged = 0;
networkData.nSocket = 0;
networkData.bAllowSocketChanges = 1;
networkData.nSecurityFlag = NETSECURITY_OFF;
networkData.nSecurityNum = 0;
networkData.bSendingExtras = 0;
networkData.bVerifyPlayerJoined = -1;
networkData.nPlayerJoiningExtras = -1;	// This is so we know who to send 'latecomer' packets to.
networkData.bRejoined = 0;					// Did WE rejoin this game?
networkData.bNewGame = 0;					// Is this the first level of a new game?
networkData.bSendObjects = 0;				// Are we in the process of sending gameData.objs.objects to a tPlayer?
networkData.nSendObjNum = -1;				// What tObject are we sending next?
networkData.bPlayerAdded = 0;				// Is this a new tPlayer or a returning tPlayer?
networkData.bSendObjectMode = 0;			// What nType of gameData.objs.objects are we sending, static or dynamic?
networkData.bPacketUrgent = 0;
networkData.nGameType = 0;
networkData.nTotalMissedPackets = 0;
networkData.nTotalPacketsGot = 0;
networkData.mySyncPackInited = 1;		// Set to 1 if the networkData.mySyncPack is zeroed.
networkData.nMySegsCheckSum = 0;
networkData.bWantPlayersInfo = 0;
networkData.bWaitingForPlayerInfo = 0;
}

//------------------------------------------------------------------------------

// reasons for a packet with nType PID_DUMP

extern void MultiSendStolenItems ();

int NetworkWaitForSync ();
extern void MultiSendWallStatus (int, ubyte, ubyte, ubyte);
extern void MultiSendWallStatusSpecific (int, int, ubyte, ubyte, ubyte);

extern void GameDisableCheats ();

#ifdef NETPROFILING
FILE *SendLogFile, *ReceiveLogFile;
int TTSent [100], TTRecv [100];
#endif

// following is network stuff for appletalk

void network_dump_appletalk_player (ubyte node, ushort net, ubyte socket, int why);

#define NETWORK_OEM 0x10

//------------------------------------------------------------------------------

char *iptos (char *pszIP, char *addr)
{
sprintf (pszIP, "%d.%d.%d.%d:%d", 
			addr [0], addr [1], addr [2], addr [3],
			ntohs (*((short *) (addr + 4))));
return pszIP;
}

//------------------------------------------------------------------------------

void ClipRank (char *rank)
{
 // This function insures no crashes when dealing with D2 1.0
if ((*rank < 0) || (*rank > 9))
	*rank = 0;
 }

//------------------------------------------------------------------------------

void NetworkInit (void)
{
	int nPlayerSave = gameData.multiplayer.nLocalPlayer;

GameDisableCheats ();
gameStates.multi.bIWasKicked=0;
gameStates.gameplay.bFinalBossIsDead=0;
networkData.nNamesInfoSecurity=-1;
#ifdef NETPROFILING
OpenSendLog ();
OpenReceiveLog (); 
#endif
memset (gameData.multiplayer.maxPowerupsAllowed, 0, sizeof (gameData.multiplayer.maxPowerupsAllowed ));
memset (gameData.multiplayer.powerupsInMine, 0, sizeof (gameData.multiplayer.powerupsInMine));
networkData.nTotalMissedPackets=0; networkData.nTotalPacketsGot=0;
memset (&netGame, 0, sizeof (tNetgameInfo));
memset (&netPlayers, 0, sizeof (tAllNetPlayersInfo));
memset (&networkData.mySeq, 0, sizeof (tSequencePacket));
networkData.mySeq.nType = PID_REQUEST;
memcpy (networkData.mySeq.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
networkData.mySeq.player.version_major=D2X_MAJOR;
networkData.mySeq.player.version_minor=D2X_MINOR | (IS_D2_OEM ? NETWORK_OEM : 0);
networkData.mySeq.player.rank=GetMyNetRanking ();	
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (networkData.mySeq.player.network.ipx.node, IpxGetMyLocalAddress (), 6);
	if (gameStates.multi.nGameType == UDP_GAME)
		* ((ushort *) (networkData.mySeq.player.network.ipx.node + 4)) = 
			htons (* ((ushort *) (networkData.mySeq.player.network.ipx.node + 4)));
//		if (gameStates.multi.nGameType == UDP_GAME)
//			memcpy (networkData.mySeq.player.network.ipx.node, ipx_LocalAddress + 4, 4);
	memcpy (networkData.mySeq.player.network.ipx.server, IpxGetMyServerAddress (), 4);
}
networkData.mySeq.player.computerType = DOS;
for (gameData.multiplayer.nLocalPlayer = 0; 
	  gameData.multiplayer.nLocalPlayer < MAX_NUM_NET_PLAYERS; 
	  gameData.multiplayer.nLocalPlayer++)
	InitPlayerStatsGame ();
gameData.multiplayer.nLocalPlayer = nPlayerSave;         
MultiNewGame ();
networkData.bNewGame = 1;
gameData.reactor.bDestroyed = 0;
NetworkFlush ();
netGame.nPacketsPerSec = mpParams.nPPS;
}

//------------------------------------------------------------------------------

int NetworkIAmMaster (void)
{
	// I am the lowest numbered tPlayer in this game?

	int i;

if (!IsMultiGame)
	return (gameData.multiplayer.nLocalPlayer == 0);
for (i = 0; i < gameData.multiplayer.nLocalPlayer; i++)
	if (gameData.multiplayer.players [i].connected)
		return 0;
return 1;
}

//------------------------------------------------------------------------------

int NetworkWhoIsMaster (void)
{
	// Who is the master of this game?

	int i;

	if (!(gameData.app.nGameMode & GM_NETWORK))
		return (gameData.multiplayer.nLocalPlayer == 0);

	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		if (gameData.multiplayer.players [i].connected)
			return i;
	return gameData.multiplayer.nLocalPlayer;
}

//------------------------------------------------------------------------------

int NetworkHowManyConnected ()
 {
  int num=0, i;
 
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		if (gameData.multiplayer.players [i].connected)
			num++;
   return (num);
 }

//------------------------------------------------------------------------------

inline int CmpNetPlayers (char *callsign1, char *callsign2, 
								  network_info *network1, network_info *network2)
{
if ((gameStates.multi.nGameType == IPX_GAME) ||
	 ((gameStates.multi.nGameType == UDP_GAME) && gameStates.multi.bCheckPorts)) {
	if (memcmp (network1, network2, sizeof (network_info)))
		return 1;
	}
else if (gameStates.multi.nGameType == UDP_GAME) {
	if (memcmp (network1, network2, extraGameInfo [1].bCheckUDPPort ? sizeof (network_info) : sizeof (network_info) - 2))	//bCheck the port too
		return 1;
	}
#ifdef MACINTOSH
else {
	if (network1->appletalk.node != network2->appletalk.node)
		return 1;
	if (network1->appletalk.net == network2->appletalk.net)
		return 1;
	}
#endif
if (callsign1 && callsign2 && stricmp (callsign1, callsign2))
	return 1;
#ifdef _DEBUG
HUDMessage (0, "'%s' not recognized", callsign1);
#endif
return 0;
}

//------------------------------------------------------------------------------

static inline void ResetPlayerTimeout (int nPlayer, fix t)
{
networkData.nLastPacketTime [nPlayer] = (t < 0) ? TimerGetApproxSeconds () : t;
}

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
networkData.mySyncPackInited = 0;
NetworkUpdateNetGame ();
return 0;
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

if (game->version_major == 0 && D2X_MAJOR>0) {
#if 1      
	con_printf (CONDBG, "Error:Can't join because version majors don't match!\n");
#endif
	return 0;
	}

if (game->version_major>0 && D2X_MAJOR == 0) {
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
		 return (2);
	if (game->bRefusePlayers)
		 return (3);
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
netPlayers.players [nPlayer].connected = 0;
if (networkData.bVerifyPlayerJoined == nPlayer)
	networkData.bVerifyPlayerJoined=-1;
//      CreatePlayerAppearanceEffect (&gameData.objs.objects [gameData.multiplayer.players [nPlayer].nObject]);
MultiMakePlayerGhost (nPlayer);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiDisconnect (nPlayer);
MultiStripRobots (nPlayer);
}
		
//------------------------------------------------------------------------------

void NetworkNewPlayer (tSequencePacket *their)
{
	int	nObject;
	int	pnum;

pnum = their->player.connected;
Assert (pnum >= 0);
Assert (pnum < gameData.multiplayer.nMaxPlayers);        
nObject = gameData.multiplayer.players [pnum].nObject;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordMultiConnect (pnum, pnum == gameData.multiplayer.nPlayers, their->player.callsign);
memcpy (gameData.multiplayer.players [pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
memcpy (netPlayers.players [pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
ClipRank ((char *) &their->player.rank);
netPlayers.players [pnum].rank = their->player.rank;
netPlayers.players [pnum].version_major = their->player.version_major;
netPlayers.players [pnum].version_minor = their->player.version_minor;
NetworkCheckForOldVersion ((char) pnum);

if (gameStates.multi.nGameType >= IPX_GAME) {
	if ((* (uint *)their->player.network.ipx.server) != 0)
		IpxGetLocalTarget (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			gameData.multiplayer.players [pnum].netAddress);
	else
		memcpy (gameData.multiplayer.players [pnum].netAddress, their->player.network.ipx.node, 6);
	}
memcpy (&netPlayers.players [pnum].network, &their->player.network, sizeof (network_info));
gameData.multiplayer.players [pnum].nPacketsGot = 0;
gameData.multiplayer.players [pnum].connected = 1;
gameData.multiplayer.players [pnum].netKillsTotal = 0;
gameData.multiplayer.players [pnum].netKilledTotal = 0;
memset (gameData.multigame.kills.matrix [pnum], 0, MAX_PLAYERS*sizeof (short)); 
gameData.multiplayer.players [pnum].score = 0;
gameData.multiplayer.players [pnum].flags = 0;
gameData.multiplayer.players [pnum].nKillGoalCount=0;
if (pnum == gameData.multiplayer.nPlayers) {
	gameData.multiplayer.nPlayers++;
	netGame.nNumPlayers = gameData.multiplayer.nPlayers;
	}
DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
ClipRank ((char *) &their->player.rank);
if (gameOpts->multi.bNoRankings)
	HUDInitMessage ("'%s' %s\n", their->player.callsign, TXT_JOINING);
else   
   HUDInitMessage ("%s'%s' %s\n", pszRankStrings [their->player.rank], their->player.callsign, TXT_JOINING);
MultiMakeGhostPlayer (pnum);
MultiSendScore ();
MultiSortKillList ();
//      CreatePlayerAppearanceEffect (&gameData.objs.objects [nObject]);
}

char bRefuseThisPlayer=0, bWaitForRefuseAnswer=0, bRefuseTeam;
char RefusePlayerName [12];
fix RefuseTimeLimit=0;

//------------------------------------------------------------------------------

void NetworkWelcomePlayer (tSequencePacket *their)
{
	// Add a tPlayer to a game already in progress
	ubyte local_address [6];
	int nPlayer;
	int i;

bWaitForRefuseAnswer=0;
if (FindArg ("-NoMatrixCheat")) {
	if ((their->player.version_minor & 0x0F) < 3) {
		NetworkDumpPlayer (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			DUMP_DORK);
		return;
		}
	}
if (HoardEquipped ()) {
// If hoard game, and this guy isn't D2 Christmas (v1.2), dump him
	if ((gameData.app.nGameMode & GM_HOARD) && ((their->player.version_minor & 0x0F)<2)) {
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
if ((gameStates.app.bEndLevelSequence) || (gameData.reactor.bDestroyed)) {
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
	if (networkData.bSendObjects)
		return;
	if (networkData.bSendingExtras && networkData.bVerifyPlayerJoined == -1)
		return;
	}
#if 0
if (networkData.bSendObjects || networkData.bSendingExtras) {
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
	if ((* (uint *)their->player.network.ipx.server) != 0)
		IpxGetLocalTarget (
			their->player.network.ipx.server, 
			their->player.network.ipx.node, 
			local_address);
	else
		memcpy (local_address, their->player.network.ipx.node, 6);
	}
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (gameStates.multi.nGameType == IPX_GAME) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, local_address, 6) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, their->player.callsign)) {
			nPlayer = i;
			break;
			}
		}
	else if (gameStates.multi.nGameType == UDP_GAME) {
		if (!memcmp (gameData.multiplayer.players [i].netAddress, local_address, extraGameInfo [1].bCheckUDPPort ? 6 : 4) &&
			 !stricmp (gameData.multiplayer.players [i].callsign, their->player.callsign)) {
			nPlayer = i;
			memcpy (gameData.multiplayer.players [i].netAddress, local_address, 6);
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
		if (gameData.app.nGameMode & GM_TEAM)
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
		HUDInitMessage ("%s'%s' %s", pszRankStrings [netPlayers.players [nPlayer].rank], gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	}
gameData.multiplayer.players [nPlayer].nKillGoalCount=0;
gameData.multiplayer.players [nPlayer].connected=0;
// Send updated gameData.objs.objects data to the new/returning tPlayer
networkData.playerRejoining = *their;
networkData.playerRejoining.player.connected = nPlayer;
networkData.bSendObjects = 1;
networkData.nSendObjNum = -1;
NetworkSendObjects ();
}

//------------------------------------------------------------------------------

int NetworkObjnumIsPast (int nObject)
{
	// determine whether or not a given tObject number has already been sent
	// to a re-joining player.
	int nPlayer = networkData.playerRejoining.player.connected;
	int obj_mode = !((gameData.multigame.nObjOwner [nObject] == -1) || (gameData.multigame.nObjOwner [nObject] == nPlayer));

if (!networkData.bSendObjects)
	return 0; // We're not sending gameData.objs.objects to a new tPlayer
if (obj_mode > networkData.bSendObjectMode)
	return 0;
else if (obj_mode < networkData.bSendObjectMode)
	return 1;
else if (nObject < networkData.nSendObjNum)
	return 1;
else
	return 0;
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
#	define OBJ_PACKETS_PER_FRAME 1
#else
#	define OBJ_PACKETS_PER_FRAME 1
#endif
extern void MultiSendActiveDoor (char);
extern void MultiSendDoorOpenSpecific (int, int, int, ubyte);

void NetworkSendDoorUpdates (int pnum)
{
	// Send door status when new tPlayer joins
	
	int i;
	tWall *wallP;
   
//   Assert (pnum>-1 && pnum<gameData.multiplayer.nPlayers);
for (i = 0, wallP = gameData.walls.walls; i < gameData.walls.nWalls; i++, wallP++) {
   if ((wallP->nType == WALL_DOOR) && 
		 ((wallP->state == WALL_DOOR_OPENING) || 
		  (wallP->state == WALL_DOOR_WAITING) || 
		  (wallP->state == WALL_DOOR_OPEN)))
		MultiSendDoorOpenSpecific (pnum, wallP->nSegment, wallP->nSide, wallP->flags);
	else if ((wallP->nType == WALL_BLASTABLE) && (wallP->flags & WALL_BLASTED))
		MultiSendDoorOpenSpecific (pnum, wallP->nSegment, wallP->nSide, wallP->flags);
	else if ((wallP->nType == WALL_BLASTABLE) && (wallP->hps != WALL_HPS))
		MultiSendHostageDoorStatus (i);
	else
		MultiSendWallStatusSpecific (pnum, i, wallP->nType, wallP->flags, wallP->state);
	}
}

//------------------------------------------------------------------------------

void NetworkSendMarkers ()
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

void NetworkProcessMonitorVector (int vector)
{
	int		i, j;
	int		tm, ec, bm;
	int		count = 0;
	tSegment	*segP = gameData.segs.segments ;
	tSide		*sideP;
	
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	for (j = 0, sideP = segP->sides; j < 6; j++, sideP++) {
		if (((tm = sideP->nOvlTex) != 0) &&
				((ec = gameData.pig.tex.pTMapInfo [tm].eclip_num) != -1) &&
				((bm = gameData.eff.pEffects [ec].nDestBm) != -1)) {
			if (vector & (1 << count))
				sideP->nOvlTex = bm;
			count++;
			Assert (count < 32);
			}
		}
	}
}

//------------------------------------------------------------------------------

int NetworkCreateMonitorVector (void)
{
	int h, i, j, k;
	int tm, ec;
	int num_blown_bitmaps = 0;
	int monitor_num = 0;
	#define NUM_BLOWN_BITMAPS 20
	int blown_bitmaps [NUM_BLOWN_BITMAPS];
	int vector = 0;
	tSegment *segP = gameData.segs.segments;
	tSide *sideP;

for (i = 0; i < gameData.eff.nEffects [gameStates.app.bD1Data]; i++) {
	if ((h = gameData.eff.pEffects[i].nDestBm) > 0) {
		for (j = 0; j < num_blown_bitmaps; j++)
			if (blown_bitmaps [j] == h)
				break;
		if (j == num_blown_bitmaps) {
			blown_bitmaps [num_blown_bitmaps++] = h;
			Assert (num_blown_bitmaps < NUM_BLOWN_BITMAPS);
			}
		}
	}               
		
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	for (j = 0, sideP = segP->sides; j < 6; j++, sideP++) {
		if ((tm = sideP->nOvlTex) != 0) {
			if (((ec = gameData.pig.tex.pTMapInfo [tm].eclip_num) != -1) &&
					(gameData.eff.pEffects[ec].nDestBm != -1)) {
				monitor_num++;
				Assert (monitor_num < 32);
				}
			else {
				for (k = 0; k < num_blown_bitmaps; k++) {
					if ((tm) == blown_bitmaps [k]) {
						vector |= (1 << monitor_num);
						monitor_num++;
						Assert (monitor_num < 32);
						break;
						}
					}
				}
			}
		}
	}
return (vector);
}

//------------------------------------------------------------------------------

void NetworkStopResync (tSequencePacket *their)
{
if (!CmpNetPlayers (networkData.playerRejoining.player.callsign, their->player.callsign, 
						  &networkData.playerRejoining.player.network, &their->player.network)) {
#if 1      
	con_printf (CONDBG, "Aborting resync for tPlayer %s.\n", their->player.callsign);
#endif
	networkData.bSendObjects = 0;
	networkData.bSendingExtras = 0;
	networkData.bRejoined=0;
	networkData.nPlayerJoiningExtras = -1;
	networkData.nSendObjNum = -1;
	}
}

//------------------------------------------------------------------------------

ubyte object_buffer [IPX_MAX_DATA_SIZE];

void NetworkSendObjects (void)
{
	short nRemoteObj;
	sbyte owner;
	int loc, i, h, t;

	static int objCount = 0;
	static int nFrame = 0;

	int objCount_frame = 0;
	int nPlayer = networkData.playerRejoining.player.connected;

	// Send clear gameData.objs.objects array tTrigger and send tPlayer num

Assert (networkData.bSendObjects != 0);
Assert (nPlayer >= 0);
Assert (nPlayer < gameData.multiplayer.nMaxPlayers);

if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
	// Endlevel started before we finished sending the goods, we'll
	// have to stop and try again after the level.

	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			networkData.playerRejoining.player.network.ipx.server, 
			networkData.playerRejoining.player.network.ipx.node, 
			DUMP_ENDLEVEL);
	networkData.bSendObjects = 0; 
	return;
	}
for (h = 0; h < OBJ_PACKETS_PER_FRAME; h++) { // Do more than 1 per frame, try to speed it up without
														  // over-stressing the receiver.
	objCount_frame = 0;
	memset (object_buffer, 0, IPX_MAX_DATA_SIZE);
	object_buffer [0] = PID_OBJECT_DATA;
	loc = 3;

	if (networkData.nSendObjNum == -1) {
		objCount = 0;
		networkData.bSendObjectMode = 0;
		SET_SHORT (object_buffer, loc, -1);            
		SET_BYTE (object_buffer, loc, nPlayer);                            
		/* Placeholder for nRemoteObj, not used here */          
		loc += 2;
		networkData.nSendObjNum = 0;
		objCount_frame = 1;
		nFrame = 0;
		}
	
	for (i = networkData.nSendObjNum; i <= gameData.objs.nLastObject; i++) {
		t = gameData.objs.objects [i].nType;
		if ((t != OBJ_POWERUP) && (t != OBJ_PLAYER) &&
			 (t != OBJ_CNTRLCEN) && (t != OBJ_GHOST) &&
			 (t != OBJ_ROBOT) && (t != OBJ_HOSTAGE) &&
			 ((t != OBJ_WEAPON) || (gameData.objs.objects [i].id != SMALLMINE_ID)))
			continue;
		if ((networkData.bSendObjectMode == 0) && 
			 ((gameData.multigame.nObjOwner [i] != -1) && (gameData.multigame.nObjOwner [i] != nPlayer)))
			continue;
		if ((networkData.bSendObjectMode == 1) && 
			 ((gameData.multigame.nObjOwner [i] == -1) || (gameData.multigame.nObjOwner [i] == nPlayer)))
			continue;

		if (((IPX_MAX_DATA_SIZE-1) - loc) < (sizeof (tObject)+5))
			break; // Not enough room for another tObject

		objCount_frame++;
		objCount++;

		nRemoteObj = ObjnumLocalToRemote ((short)i, &owner);
		Assert (owner == gameData.multigame.nObjOwner [i]);

		SET_SHORT (object_buffer, loc, i);      
		SET_BYTE (object_buffer, loc, owner);                                 
		SET_SHORT (object_buffer, loc, nRemoteObj); 
		SET_BYTES (object_buffer, loc, gameData.objs.objects + i, sizeof (tObject));
		if (gameStates.multi.nGameType >= IPX_GAME)
			SwapObject ((tObject *) (object_buffer + loc - sizeof (tObject)));
		}

	if (objCount_frame) {// Send any gameData.objs.objects we've buffered
		nFrame++;

		networkData.nSendObjNum = i;
		object_buffer [1] = objCount_frame;  
		object_buffer [2] = nFrame;

		Assert (loc <= IPX_MAX_DATA_SIZE);

		if (gameStates.multi.nGameType >= IPX_GAME)
			IPXSendInternetPacketData (
				object_buffer, 
				loc, 
				networkData.playerRejoining.player.network.ipx.server, 
				networkData.playerRejoining.player.network.ipx.node);

		// OLD IPXSendPacketData (object_buffer, loc, &networkData.playerRejoining.player.node);
		}

	if (i > gameData.objs.nLastObject) {
		if (networkData.bSendObjectMode == 0) {
			networkData.nSendObjNum = 0;
			networkData.bSendObjectMode = 1; // go to next mode
			}
		else {
			Assert (networkData.bSendObjectMode == 1); 

			nFrame++;
			// Send count so other tSide can make sure he got them all
			object_buffer [0] = PID_OBJECT_DATA;
			object_buffer [1] = 1;
			object_buffer [2] = nFrame;
			* (short *) (object_buffer+3) = INTEL_SHORT (-2);
			* (short *) (object_buffer+6) = INTEL_SHORT (objCount);
			//OLD IPXSendPacketData (object_buffer, 8, &networkData.playerRejoining.player.node);
			if (gameStates.multi.nGameType >= IPX_GAME)
				IPXSendInternetPacketData (object_buffer, 8, networkData.playerRejoining.player.network.ipx.server, networkData.playerRejoining.player.network.ipx.node);
			
		
			// Send sync packet which tells the tPlayer who he is and to start!
			NetworkSendRejoinSync (nPlayer);
			networkData.bVerifyPlayerJoined = nPlayer;

			// Turn off send tObject mode
			networkData.nSendObjNum = -1;
			networkData.bSendObjects = 0;
			objCount = 0;

			//if (!NetworkIAmMaster ())
			// Int3 ();  // Bad!! Get Jason.  Someone goofy is trying to get ahold of the game!

			networkData.bSendingExtras = 40; // start to send extras
			networkData.nPlayerJoiningExtras=nPlayer;

			return;
			} // mode == 1;
		} // i > gameData.objs.nLastObject
	} // For PACKETS_PER_FRAME
}

//------------------------------------------------------------------------------

extern void MultiSendPowerupUpdate ();

void NetworkSendRejoinSync (int nPlayer)
{
	int i, j;

gameData.multiplayer.players [nPlayer].connected = 1; // connect the new guy
ResetPlayerTimeout (nPlayer, -1);
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed) {
	// Endlevel started before we finished sending the goods, we'll
	// have to stop and try again after the level.

	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			networkData.playerRejoining.player.network.ipx.server, 
			networkData.playerRejoining.player.network.ipx.node, 
			DUMP_ENDLEVEL);
	networkData.bSendObjects = 0; 
	networkData.bSendingExtras=0;
	return;
	}
if (networkData.bPlayerAdded) {
	networkData.playerRejoining.nType = PID_ADDPLAYER;
	networkData.playerRejoining.player.connected = nPlayer;
	NetworkNewPlayer (&networkData.playerRejoining);

	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((i != nPlayer) && 
			 (i != gameData.multiplayer.nLocalPlayer) && 
			 (gameData.multiplayer.players [i].connected))
			if (gameStates.multi.nGameType >= IPX_GAME) {
				SendSequencePacket (
					networkData.playerRejoining, 
					netPlayers.players [i].network.ipx.server, 
					netPlayers.players [i].network.ipx.node, 
					gameData.multiplayer.players [i].netAddress);
			}
		}
	}       
// Send sync packet to the new guy
NetworkUpdateNetGame ();

// Fill in the kill list
for (j=0; j<MAX_PLAYERS; j++) {
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
		networkData.playerRejoining.player.network.ipx.server, 
		networkData.playerRejoining.player.network.ipx.node);
	SendNetPlayersPacket (
		networkData.playerRejoining.player.network.ipx.server, 
		networkData.playerRejoining.player.network.ipx.node);
	}
return;
}

//------------------------------------------------------------------------------

void ResendSyncDueToPacketLossForAllender ()
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
		networkData.playerRejoining.player.network.ipx.server, 
		networkData.playerRejoining.player.network.ipx.node);
	SendNetPlayersPacket (
		networkData.playerRejoining.player.network.ipx.server, 
		networkData.playerRejoining.player.network.ipx.node);
	}
}

//------------------------------------------------------------------------------

char * NetworkGetPlayerName (int nObject)
{
if (nObject < 0) 
	return NULL; 
if (gameData.objs.objects [nObject].nType != OBJ_PLAYER) 
	return NULL;
if (gameData.objs.objects [nObject].id >= MAX_PLAYERS) 
	return NULL;
if (gameData.objs.objects [nObject].id >= gameData.multiplayer.nPlayers) 
	return NULL;
return gameData.multiplayer.players [gameData.objs.objects [nObject].id].callsign;
}

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

void NetworkAddPlayer (tSequencePacket *pkt)
{
	tNetPlayerInfo	*playerP;
	
if (NetworkFindPlayer (&pkt->player) > -1)
	return;
playerP = netPlayers.players + gameData.multiplayer.nPlayers;
memcpy (&playerP->network, &pkt->player.network, sizeof (network_info));
ClipRank ((char *) &pkt->player.rank);
memcpy (playerP->callsign, pkt->player.callsign, CALLSIGN_LEN+1);
playerP->version_major = pkt->player.version_major;
playerP->version_minor = pkt->player.version_minor;
playerP->rank = pkt->player.rank;
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

void NetworkRemovePlayer (tSequencePacket *pkt)
{
	int i, j, pn = NetworkFindPlayer (&pkt->player);

if (pn < 0)
	return;

for (i = pn; i < gameData.multiplayer.nPlayers - 1; ) {
	j = i++;
	memcpy (&netPlayers.players [j].network, &netPlayers.players [i].network.ipx.node, 
			  sizeof (network_info));
	memcpy (netPlayers.players [j].callsign, netPlayers.players [i].callsign, CALLSIGN_LEN+1);
	netPlayers.players [j].version_major = netPlayers.players [i].version_major;
	netPlayers.players [j].version_minor = netPlayers.players [i].version_minor;
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

void NetworkDumpPlayer (ubyte * server, ubyte *node, int why)
{
	// Inform tPlayer that he was not chosen for the netgame
	tSequencePacket temp;

temp.nType = PID_DUMP;
memcpy (temp.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
temp.player.connected = why;
if (gameStates.multi.nGameType >= IPX_GAME)
	SendInternetSequencePacket (temp, server, node);
else
	Int3 ();
}

//------------------------------------------------------------------------------
// Send a broadcast request for game info

int NetworkSendGameListRequest ()
{
	tSequencePacket me;

#ifdef _DEBUG
memset (&me, 0, sizeof (me));
#endif
me.nType = PID_GAME_LIST;
memcpy (me.player.callsign, 
		  LOCALPLAYER.callsign, 
		  CALLSIGN_LEN+1);
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

void NetworkSendAllInfoRequest (char nType, int which_security)
{
	// Send a broadcast request for game info
	tSequencePacket me;

me.nSecurity = which_security;
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

void NetworkUpdateNetGame (void)
{
	// Update the netgame struct with current game variables

	int i, j;

netGame.nConnected = 0;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected)
		netGame.nConnected++;

// This is great: D2 1.0 and 1.1 ignore upper part of the gameFlags field of
//	the tLiteInfo struct when you're sitting on the join netgame screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.
if (HoardEquipped ()) {
	if (gameData.app.nGameMode & GM_MONSTERBALL)
		netGame.gameFlags |= NETGAME_FLAG_MONSTERBALL;
	else if (gameData.app.nGameMode & GM_ENTROPY)
		netGame.gameFlags |= NETGAME_FLAG_ENTROPY;
	else if (gameData.app.nGameMode & GM_HOARD) {
		netGame.gameFlags |= NETGAME_FLAG_HOARD;
		if (gameData.app.nGameMode & GM_TEAM)
			netGame.gameFlags |= NETGAME_FLAG_TEAM_HOARD;
		}
	}
if (networkData.nStatus == NETSTAT_STARTING)
	return;
netGame.nNumPlayers = gameData.multiplayer.nPlayers;
netGame.gameStatus = networkData.nStatus;
netGame.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	netPlayers.players [i].connected = gameData.multiplayer.players [i].connected;
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		netGame.kills [i][j] = gameData.multigame.kills.matrix [i][j];
	netGame.killed [i] = gameData.multiplayer.players [i].netKilledTotal;
	netGame.playerKills [i] = gameData.multiplayer.players [i].netKillsTotal;
	netGame.player_score [i] = gameData.multiplayer.players [i].score;
	netGame.playerFlags [i] = (gameData.multiplayer.players [i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
	}
netGame.teamKills [0] = gameData.multigame.kills.nTeam [0];
netGame.teamKills [1] = gameData.multigame.kills.nTeam [1];
netGame.nLevel = gameData.missions.nCurrentLevel;
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

extern fix ThisLevelTime;

void NetworkSendGameInfo (tSequencePacket *their)
{
	// Send game info to someone who requested it

	char oldType, old_status;
   fix timevar;
   int i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.nType;
old_status = netGame.gameStatus;
netGame.nType = PID_GAME_INFO;
netPlayers.nType = PID_PLAYERSINFO;
netPlayers.nSecurity = netGame.nSecurity;
netGame.version_major = D2X_MAJOR;
netGame.version_minor = D2X_MINOR;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.gameStatus = NETSTAT_ENDLEVEL;
if (netGame.xPlayTimeAllowed) {
	timevar=i2f (netGame.xPlayTimeAllowed*5*60);
	i=f2i (timevar-ThisLevelTime);
	if (i<30)
		netGame.gameStatus=NETSTAT_ENDLEVEL;
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
netGame.gameStatus = old_status;
//	if ((gameData.app.nGameMode & GM_ENTROPY) || extraGameInfo [0].bEnhancedCTF)
//make half-way sure the client gets this data ...
NetworkSendExtraGameInfo (their);
NetworkSendExtraGameInfo (their);
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
extraGameInfo [1].bHeadLights = egi1Save.bHeadLights;
extraGameInfo [1].bPowerupLights = egi1Save.bPowerupLights;
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
	char oldType, old_status, oldstatus;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.nType;
old_status = netGame.gameStatus;
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
netGame.gameStatus = old_status;
NetworkSendExtraGameInfo (their);
}       

//------------------------------------------------------------------------------

/* Send game info to all players in this game */
void NetworkSendNetgameUpdate ()
{
	char	oldType, old_status, szIP [30];
	int	i;

NetworkUpdateNetGame (); // Update the values in the netgame struct
oldType = netGame.nType;
old_status = netGame.gameStatus;
netGame.nType = PID_GAME_UPDATE;
if (gameStates.app.bEndLevelSequence || gameData.reactor.bDestroyed)
	netGame.gameStatus = NETSTAT_ENDLEVEL;
LogErr ("sending netgame update:\n");
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected) && (i != gameData.multiplayer.nLocalPlayer)) {
		if (gameStates.multi.nGameType >= IPX_GAME) {
			LogErr ("   %s (%s)\n", netPlayers.players [i].callsign, 
				iptos (szIP, (char *) netPlayers.players [i].network.ipx.node));
			SendLiteNetGamePacket (
				netPlayers.players [i].network.ipx.server, 
				netPlayers.players [i].network.ipx.node, 
				gameData.multiplayer.players [i].netAddress);
			}
		}
	}
netGame.nType = oldType;
netGame.gameStatus = old_status;
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
networkData.mySeq.nType = PID_REQUEST;
networkData.mySeq.player.connected = gameData.missions.nCurrentLevel;
if (gameStates.multi.nGameType >= IPX_GAME) {
	SendInternetSequencePacket (
		networkData.mySeq, 
		netPlayers.players [i].network.ipx.server, 
		netPlayers.players [i].network.ipx.node);
	}
return i;
}

//------------------------------------------------------------------------------

void DeleteActiveNetGame (int i)
{
if (i < --networkData.nActiveGames) {
	int h = networkData.nActiveGames - i;
	memcpy (activeNetGames + i, activeNetGames + i + 1, sizeof (tNetgameInfo) * h);
	memcpy (activeNetPlayers + i, activeNetPlayers + i + 1, sizeof (tAllNetPlayersInfo) * h);
	memcpy (nLastNetGameUpdate + i, nLastNetGameUpdate + i + 1, sizeof (int) * h);
	}	   	
networkData.bGamesChanged = 1;
}

//------------------------------------------------------------------------------

void DeleteTimedOutNetGames (void)
{
	int	i, t = SDL_GetTicks ();
	int	bPlaySound = 0;

for (i = networkData.nActiveGames; i > 0;)
	if (t - nLastNetGameUpdate [--i] > 10000) {
		DeleteActiveNetGame (i);
		bPlaySound = 1;
		}
if (bPlaySound)
	DigiPlaySample (SOUND_HUD_MESSAGE,F1_0);
}

//------------------------------------------------------------------------------

int FindActiveNetGame (char *pszGameName, int nSecurity)
{
	int	i;

for (i = 0; i < networkData.nActiveGames; i++) {
	if (!stricmp (activeNetGames [i].szGameName, pszGameName)
#if SECURITY_CHECK
		 && (activeNetGames [i].nSecurity == nSecurity)
#endif
		 )
		break;
	}
return i;
}

//------------------------------------------------------------------------------

int nSecurityCheck = 0;
	
void NetworkProcessGameInfo (ubyte *data)
{
	int i;
	tNetgameInfo *newGame = (tNetgameInfo *) data;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tNetgameInfo tmp_info;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveNetGamePacket (data, &tmp_info, 0); // get correctly aligned structure
	newGame = &tmp_info;
	}
#endif
networkData.bWaitingForPlayerInfo = 0;
#if SECURITY_CHECK
if (newGame->nSecurity != tmpPlayersInfo->nSecurity) {
	Int3 ();     // Get Jason
   return;     // If this first half doesn't go with the second half
   }
#endif
Assert (tmpPlayersInfo != NULL);
i = FindActiveNetGame (newGame->szGameName, newGame->nSecurity);
if (i == MAX_ACTIVE_NETGAMES) {
#if 1
	con_printf (CONDBG, "Too many netgames.\n");
#endif		
	return;
	}
if (i == networkData.nActiveGames) {
	if (newGame->nNumPlayers == 0)
		return;
	networkData.nActiveGames++;
	}
networkData.bGamesChanged = 1;
// MWA  memcpy (&activeNetGames [i], data, sizeof (tNetgameInfo);
nLastNetGameUpdate [i] = SDL_GetTicks ();
memcpy (activeNetGames + i, (ubyte *) newGame, sizeof (tNetgameInfo));
memcpy (activeNetPlayers + i, tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
if (nSecurityCheck)
#if SECURITY_CHECK
	if (activeNetGames [i].nSecurity == nSecurityCheck)
#endif
		nSecurityCheck = -1;
if (i == networkData.nActiveGames)
if (activeNetGames [i].nNumPlayers == 0) {	// Delete this game
	DeleteActiveNetGame (i);
	nSecurityCheck = 0;
	}
}

//------------------------------------------------------------------------------

void NetworkProcessLiteInfo (ubyte *data)
{
	int				i;
	tNetgameInfo	*actGameP;
	tLiteInfo		*newInfo = (tLiteInfo *)data;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tLiteInfo		tmp_info;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveNetGamePacket (data, (tNetgameInfo *)&tmp_info, 1);
	newInfo = &tmp_info;
	}
#endif

networkData.bGamesChanged = 1;
i = FindActiveNetGame (((tNetgameInfo *) newInfo)->szGameName, ((tNetgameInfo *) newInfo)->nSecurity);
if (i == MAX_ACTIVE_NETGAMES)
	return;
if (i == networkData.nActiveGames) {
	if (newInfo->nNumPlayers == 0)
		return;
	networkData.nActiveGames++;
	}
actGameP = activeNetGames + i;
memcpy (actGameP, (ubyte *) newInfo, sizeof (tLiteInfo));
nLastNetGameUpdate [i] = SDL_GetTicks ();
// See if this is really a Hoard/Entropy/Monsterball game
// If so, adjust all the data accordingly
if (HoardEquipped ()) {
	if (actGameP->gameFlags & (NETGAME_FLAG_HOARD | NETGAME_FLAG_ENTROPY | NETGAME_FLAG_MONSTERBALL)) {
		if ((actGameP->gameFlags & NETGAME_FLAG_MONSTERBALL) == NETGAME_FLAG_MONSTERBALL)
			actGameP->gameMode = NETGAME_MONSTERBALL; 
		else if (actGameP->gameFlags & NETGAME_FLAG_HOARD)
			actGameP->gameMode = NETGAME_HOARD;					  
		else if (actGameP->gameFlags & NETGAME_FLAG_ENTROPY)
			actGameP->gameMode = NETGAME_ENTROPY;					  
		actGameP->gameStatus = NETSTAT_PLAYING;
		if (actGameP->gameFlags & NETGAME_FLAG_TEAM_HOARD)
			actGameP->gameMode = NETGAME_TEAM_HOARD;					  
		if (actGameP->gameFlags & NETGAME_FLAG_REALLY_ENDLEVEL)
			actGameP->gameStatus = NETSTAT_ENDLEVEL;
		if (actGameP->gameFlags & NETGAME_FLAG_REALLY_FORMING)
			actGameP->gameStatus = NETSTAT_STARTING;
		}
	}
if (actGameP->nNumPlayers == 0)
	DeleteActiveNetGame (i);
}

//------------------------------------------------------------------------------

void NetworkProcessExtraGameInfo (ubyte *data)
{
	int	i;

ReceiveExtraGameInfoPacket (data, extraGameInfo + 1);
SetMonsterballForces ();
LogExtraGameInfo ();
gameStates.app.bHaveExtraGameInfo [1] = 1;
i = FindActiveNetGame (extraGameInfo [1].szGameName, extraGameInfo [1].nSecurity);
if (i < networkData.nActiveGames)
	activeExtraGameInfo [i] = extraGameInfo [1];
else
	memset (activeExtraGameInfo + i, 0, sizeof (activeExtraGameInfo [i]));
}

//------------------------------------------------------------------------------

void NetworkProcessDump (tSequencePacket *their)
{
	// Our request for join was denied.  Tell the user why.

	char temp [40];
	int i;

if (their->player.connected != 7)
	ExecMessageBox (NULL, NULL, 1, TXT_OK, NET_DUMP_STRINGS (their->player.connected));
else {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!stricmp (their->player.callsign, gameData.multiplayer.players [i].callsign)) {
			if (i!=NetworkWhoIsMaster ()) 
				HUDInitMessage (TXT_KICK_ATTEMPT, their->player.callsign);
			else {
				sprintf (temp, TXT_KICKED_YOU, their->player.callsign);
				ExecMessageBox (NULL, NULL, 1, TXT_OK, &temp);
				if (networkData.nStatus == NETSTAT_PLAYING) {
					gameStates.multi.bIWasKicked=1;
					MultiLeaveGame ();     
					}
				else
					networkData.nStatus = NETSTAT_MENU;
		      }
		   }
 		}
	}
}	

//------------------------------------------------------------------------------

void NetworkProcessRequest (tSequencePacket *their)
{
	// Player is ready to receieve a sync packet
	int i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!CmpNetPlayers (their->player.callsign, netPlayers.players [i].callsign, 
								&their->player.network, &netPlayers.players [i].network)) {
		gameData.multiplayer.players [i].connected = 1;
		break;
		}
	}                       
}

//------------------------------------------------------------------------------

int NetworkBadPacketSize (int nLength, int nExpectedLength, char *pszId)
{
if (nLength == nExpectedLength)
	return 0;
con_printf (CONDBG, "WARNING! Received invalid size for %s\n", pszId);
LogErr ("Networking: Bad size for %s\n", pszId);
#ifdef _DEBUG
HUDMessage (0, "invalid %s", pszId);
#endif
if (nLength == nExpectedLength - 4)
	LogErr ("   (Probably due to mismatched UDP/IP connection quality improvement settings)\n");
return 1;
}

//------------------------------------------------------------------------------

int NetworkBadSecurity (int nSecurity, char *pszId)
{
#if SECURITY_CHECK
if (nSecurity == netGame.nSecurity)
#endif
	return 0;
con_printf (CONDBG, "Bad security for %s\n", pszId);
LogErr ("Networking: Bad security for %s\n", pszId);
return 1;
}

//------------------------------------------------------------------------------

#define REFUSE_INTERVAL F1_0 * 8
extern void MultiResetObjectTexture (tObject *);

int NetworkProcessPacket (ubyte *data, int length)
{
	tSequencePacket *their = (tSequencePacket *)data;
	char pid = data [0];
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tSequencePacket tmp_packet;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveSequencePacket (data, &tmp_packet);
	their = &tmp_packet; // reassign their to point to correctly alinged structure
	}
#endif
if	((gameStates.multi.nGameType == UDP_GAME) &&
	 (pid != PID_ADDPLAYER) &&
	 (pid != PID_DOWNLOAD) &&
	 (pid != PID_ENDLEVEL) && 
	 (pid != PID_ENDLEVEL_SHORT) &&
	 (pid != PID_EXTRA_GAMEINFO) &&
	 (pid != PID_GAME_INFO) &&
	 (pid != PID_LITE_INFO) &&
	 (pid != PID_NAKED_PDATA) &&
	 (pid != PID_NAMES_RETURN) &&
	 (pid != PID_OBJECT_DATA) &&
	 (pid != PID_PDATA) &&
	 (pid != PID_UPLOAD) &&
	 (pid != PID_TRACKER_ADD_SERVER) &&
	 (pid != PID_TRACKER_GET_SERVERLIST)
	)
	{
	memcpy (&their->player.network.ipx.server, &ipx_udpSrc.src_network, 10);
	}
switch (pid) {
    //-------------------------------------------
	case PID_GAME_INFO:		// Jason L. says we can safely ignore this nType.
		con_printf (0, "received PID_GAME_INFO\n");
#ifdef _DEBUG
		pid = PID_GAME_INFO;
#endif
	 	break;
	
    //-------------------------------------------
	case PID_PLAYERSINFO:
		con_printf (0, "received PID_PLAYERSINFO\n");
		if (networkData.nStatus == NETSTAT_WAITING) {
			if (gameStates.multi.nGameType >= IPX_GAME)
				ReceiveNetPlayersPacket (data, &tmpPlayersBase);
			else
				memcpy (&tmpPlayersBase, data, sizeof (tAllNetPlayersInfo));
			if (NetworkBadSecurity (tmpPlayersBase.nSecurity, "PID_PLAYERSINFO"))
				break;
			con_printf (CONDBG, "Got a waiting PID_PLAYERSINFO!\n");
			if (NetworkBadPacketSize (length, ALLNETPLAYERSINFO_SIZE, "PID_PLAYERSINFO"))
				return 0;
			tmpPlayersInfo=&tmpPlayersBase;
			networkData.bWaitingForPlayerInfo=0;
			networkData.nSecurityNum=tmpPlayersInfo->nSecurity;
			networkData.nSecurityFlag=NETSECURITY_WAIT_FOR_SYNC;
		   }
     break;

    //-------------------------------------------
	case PID_LITE_INFO:
		con_printf (0, "received PID_LITEINFO\n");
		if (NetworkBadPacketSize (length, LITE_INFO_SIZE, "PID_LITE_INFO"))
		  return 0;
	 if (networkData.nStatus == NETSTAT_BROWSING)
		NetworkProcessLiteInfo (data);
    break;

    //-------------------------------------------
	case PID_GAME_LIST:
		con_printf (0, "received PID_GAME_LIST\n");
		// Someone wants a list of games
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_GAME_LIST"))
		  return 0;
		if (FindPlayerInBanList (their->player.callsign))
			break;
		if ((networkData.nStatus == NETSTAT_PLAYING) || 
			 (networkData.nStatus == NETSTAT_STARTING) || 
			 (networkData.nStatus == NETSTAT_ENDLEVEL))
			if (NetworkIAmMaster ())
				NetworkSendLiteInfo (their);
		break;

    //-------------------------------------------
	case PID_SEND_ALL_GAMEINFO:
		con_printf (0, "received PID_SEND_ALL_GAMEINFO\n");
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_SEND_ALL_GAMEINFO"))
		  return 0;

		if ((networkData.nStatus == NETSTAT_PLAYING) || 
			 (networkData.nStatus == NETSTAT_STARTING) || 
			 (networkData.nStatus == NETSTAT_ENDLEVEL))
			if (NetworkIAmMaster () && 
				 !NetworkBadSecurity (their->nSecurity, "PID_SEND_ALL_GAMEINFO"))
				NetworkSendGameInfo (their);
		break;
	
    //-------------------------------------------
	case PID_ADDPLAYER:
		con_printf (0, "received PID_ADDPLAYER\n");
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_ADDPLAYER"))
		  return 0;
		NetworkNewPlayer (their);
		break;                  

    //-------------------------------------------
	case PID_REQUEST:
		con_printf (0, "received PID_REQUEST\n");
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_REQUEST"))
		  return 0;
		if (FindPlayerInBanList (their->player.callsign))
			break;
		if (networkData.nStatus == NETSTAT_STARTING)  {
			// Someone wants to join our game!
			NetworkAddPlayer (their);
		   }
		else if (networkData.nStatus == NETSTAT_WAITING) {
			// Someone is ready to receive a sync packet
			NetworkProcessRequest (their);
		}
		else if (networkData.nStatus == NETSTAT_PLAYING) {
			// Someone wants to join a game in progress!
			if (netGame.bRefusePlayers)
				DoRefuseStuff (their);
		   else 
				NetworkWelcomePlayer (their);
		}
		break;

    //-------------------------------------------
	case PID_DUMP:  
		con_printf (0, "received PID_DUMP\n");
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_DUMP"))
		  return 0;
  		if (networkData.nStatus == NETSTAT_WAITING || networkData.nStatus == NETSTAT_PLAYING)
			NetworkProcessDump (their);
		break;

    //-------------------------------------------
	case PID_QUIT_JOINING:
		con_printf (0, "received PID_QUIT_JOINING\n");
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_QUIT_JOINING"))
		  return 0;
		if (networkData.nStatus == NETSTAT_STARTING)
			NetworkRemovePlayer (their);
		else if ((networkData.nStatus == NETSTAT_PLAYING) && (networkData.bSendObjects))
			NetworkStopResync (their);
		break;

    //-------------------------------------------
	case PID_SYNC:  
		con_printf (0, "received PID_SYNC\n");
		if (networkData.nStatus == NETSTAT_WAITING)  {
			if (gameStates.multi.nGameType >= IPX_GAME)
				ReceiveFullNetGamePacket (data, &TempNetInfo);
			else
				memcpy ((ubyte *)& (TempNetInfo), data, sizeof (tNetgameInfo));
			if (NetworkBadSecurity (TempNetInfo.nSecurity, "PID_SYNC"))
				break;
			if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_SYNC) {
#if SECURITY_CHECK
				if (TempNetInfo.nSecurity == tmpPlayersInfo->nSecurity) {
#else
					{
#endif
					NetworkReadSyncPacket (&TempNetInfo, 0);
					networkData.nSecurityFlag = 0;
					networkData.nSecurityNum = 0;
					}	
				}
			else {	
				networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_PLAYERS;
				networkData.nSecurityNum = TempNetInfo.nSecurity;
				if (NetworkWaitForPlayerInfo ())
					NetworkReadSyncPacket ((tNetgameInfo *)data, 0);
				networkData.nSecurityFlag = 0;
				networkData.nSecurityNum = 0;
				}
			}
		break;

    //-------------------------------------------
	case PID_EXTRA_GAMEINFO: 
		con_printf (0, "received PID_EXTRA_GAMEINFO\n");
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkProcessExtraGameInfo (data);
		break;

    //-------------------------------------------

	case PID_UPLOAD:
		con_printf (0, "received PID_UPLOAD\n");
		if (gameStates.multi.bServer || 
			 ((networkData.nStatus == NETSTAT_PLAYING) && NetworkIAmMaster ()))
			NetworkUpload (data);
		break;

	case PID_DOWNLOAD:
		con_printf (0, "received PID_DOWNLOAD\n");
		if (extraGameInfo [0].bAutoDownload) 
			NetworkDownload (data);
		break;

	case PID_TRACKER_GET_SERVERLIST:
	case PID_TRACKER_ADD_SERVER:
		con_printf (0, "received PID_TRACKER_*\n");
		ReceiveServerListFromTracker (data);
		break;

    //-------------------------------------------
	case PID_PDATA: 
		con_printf (0, "received PID_PDATA\n");
		if ((gameData.app.nGameMode&GM_NETWORK) && 
			 ((networkData.nStatus == NETSTAT_PLAYING)||
			  (networkData.nStatus == NETSTAT_ENDLEVEL) || 
			  (networkData.nStatus == NETSTAT_WAITING))) { 
			NetworkProcessPData ((char *)data);
		}
		break;

    //-------------------------------------------
   case PID_NAKED_PDATA:
		con_printf (0, "received PID_NAKED_PDATA\n");
		if ((gameData.app.nGameMode&GM_NETWORK) && 
			 ((networkData.nStatus == NETSTAT_PLAYING)||
			  (networkData.nStatus == NETSTAT_ENDLEVEL) || 
			  (networkData.nStatus == NETSTAT_WAITING))) 
			NetworkProcessNakedPData ((char *)data, length);
	   break;

    //-------------------------------------------
	case PID_OBJECT_DATA:
		con_printf (0, "received PID_OBJECT_DATA\n");
		if (networkData.nStatus == NETSTAT_WAITING) 
			NetworkReadObjectPacket (data);
		break;

    //-------------------------------------------
	case PID_ENDLEVEL:
		con_printf (0, "received PID_ENDLEVEL\n");
		if ((networkData.nStatus == NETSTAT_ENDLEVEL) || 
			 (networkData.nStatus == NETSTAT_PLAYING))
			NetworkReadEndLevelPacket (data);
#if 1				
		else
			con_printf (CONDBG, "Junked endlevel packet.\n");
#endif
		break;

    //-------------------------------------------
	case PID_ENDLEVEL_SHORT:
		con_printf (0, "received PID_ENDLEVEL_SHORT\n");
		if ((networkData.nStatus == NETSTAT_ENDLEVEL) || 
			 (networkData.nStatus == NETSTAT_PLAYING))
			NetworkReadEndLevelShortPacket (data);
#if 1				
		else
			con_printf (CONDBG, "Junked short endlevel packet!\n");
#endif
		break;

    //-------------------------------------------
	case PID_GAME_UPDATE:
		con_printf (0, "received PID_GAME_UPDATE\n");
		if (networkData.nStatus == NETSTAT_PLAYING) {
			if (gameStates.multi.nGameType >= IPX_GAME) {
				char pszIP [30];
				LogErr ("received netgame update from %s (%s)\n", 
						  their->player.callsign,
						  iptos (pszIP, (char *) their->player.network.ipx.node));
				ReceiveLiteNetGamePacket (data, &TempNetInfo);
				}
			else
				memcpy ((ubyte *)&TempNetInfo, data, sizeof (tLiteInfo));
			if (!NetworkBadSecurity (TempNetInfo.nSecurity, "PID_GAME_UPDATE"))
				memcpy (&netGame, (ubyte *)&TempNetInfo, sizeof (tLiteInfo));
			}
		if (gameData.app.nGameMode & GM_TEAM) {
			int i;

			for (i = 0; i < gameData.multiplayer.nPlayers; i++)
				if (gameData.multiplayer.players [i].connected)
		    		MultiResetObjectTexture (gameData.objs.objects + gameData.multiplayer.players [i].nObject);
		   ResetCockpit ();
			}
	   break;

    //-------------------------------------------
   case PID_PING_SEND:
		con_printf (0, "received PID_PING_SEND\n");
		NetworkPing (PID_PING_RETURN, data [1]);
		break;

    //-------------------------------------------
   case PID_PING_RETURN:
		con_printf (0, "received PID_PING_RETURN\n");
		NetworkHandlePingReturn (data [1]);  // data [1] is tPlayer who told us of their ping time
		break;

    //-------------------------------------------
   case PID_NAMES_RETURN:
		if ((networkData.nStatus == NETSTAT_BROWSING) && 
			 (networkData.nNamesInfoSecurity != -1))
		  NetworkProcessNamesReturn ((char *) data);
		break;

    //-------------------------------------------
	case PID_GAME_PLAYERS:
		// Someone wants a list of players in this game
		con_printf (0, "received PID_GAME_PLAYERS\n");
		if (NetworkBadPacketSize (length, SEQUENCE_PACKET_SIZE, "PID_GAME_PLAYERS"))
		  return 0;
		if ((networkData.nStatus == NETSTAT_PLAYING) || 
			 (networkData.nStatus == NETSTAT_STARTING) || 
			 (networkData.nStatus == NETSTAT_ENDLEVEL))
			if (NetworkIAmMaster () && 
				 !NetworkBadSecurity (their->nSecurity, "PID_GAME_PLAYERS"))
				NetworkSendPlayerNames (their);
		break;

	default:
#if 1				
		con_printf (CONDBG, "Ignoring invalid packet nType %d.\n", pid);
		LogErr ("Netgame: Ignoring invalid packet nType %d.\n", pid);
#endif
		Int3 (); // Invalid network packet nType, see ROB
	   return 0;
	}
return 1;
}

//------------------------------------------------------------------------------

#ifdef _DEBUG
void DumpSegments ()
{
	FILE * fp;

fp = fopen ("TEST.DMP", "wb");
fwrite (gameData.segs.segments, sizeof (tSegment)* (gameData.segs.nLastSegment+1), 1, fp);    
fclose (fp);
con_printf (CONDBG, "SS=%d\n", sizeof (tSegment));
}
#endif

//------------------------------------------------------------------------------

void NetworkReadEndLevelPacket (ubyte *data)
{
	// Special packet for end of level syncing
	int nPlayer;
	tEndLevelInfo *end = (tEndLevelInfo *)data;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	int i, j;

for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		end->killMatrix [i][j] = INTEL_SHORT (end->killMatrix [i][j]);
end->kills = INTEL_SHORT (end->kills);
end->killed = INTEL_SHORT (end->killed);
#endif
nPlayer = end->nPlayer;
Assert (nPlayer != gameData.multiplayer.nLocalPlayer);
if (nPlayer>=gameData.multiplayer.nPlayers) {              
	Int3 (); // weird, but it an happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}
if ((networkData.nStatus == NETSTAT_PLAYING) && (end->connected != 0))
	return; // Only accept disconnect packets if we're not out of the level yet
gameData.multiplayer.players [nPlayer].connected = end->connected;
memcpy (&gameData.multigame.kills.matrix [nPlayer][0], end->killMatrix, MAX_PLAYERS*sizeof (short));
gameData.multiplayer.players [nPlayer].netKillsTotal = end->kills;
gameData.multiplayer.players [nPlayer].netKilledTotal = end->killed;
if ((gameData.multiplayer.players [nPlayer].connected == 1) && (end->seconds_left < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = end->seconds_left;
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

void NetworkReadEndLevelShortPacket (ubyte *data)
{
	// Special packet for end of level syncing

	int nPlayer;
	tEndLevelInfoShort *end;       

end = (tEndLevelInfoShort *)data;
nPlayer = end->nPlayer;
Assert (nPlayer != gameData.multiplayer.nLocalPlayer);
if (nPlayer >= gameData.multiplayer.nPlayers) {              
	Int3 (); // weird, but it can happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}

if ((networkData.nStatus == NETSTAT_PLAYING) && (end->connected != 0))
	return; // Only accept disconnect packets if we're not out of the level yet
gameData.multiplayer.players [nPlayer].connected = end->connected;
if ((gameData.multiplayer.players [nPlayer].connected == 1) && (end->seconds_left < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = end->seconds_left;
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

void NetworkPackObjects (void)
{
// Switching modes, pack the tObject array
SpecialResetObjects ();
}                               

//------------------------------------------------------------------------------

int NetworkVerifyObjects (int nRemoteObjNum, int nLocalObjs)
{
	int i, bCoop = (gameData.app.nGameMode & GM_MULTI_COOP) != 0;
	int nPlayers, bHaveReactor = 0;

if (nRemoteObjNum - nLocalObjs > 10)
	return -1;
#if 0
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
#endif
	bHaveReactor = 1;	// multiplayer maps do not need a control center ...
for (i = 0, nPlayers = 0; i <= gameData.objs.nLastObject; i++) {
	if (gameData.objs.objects [i].nType == OBJ_GHOST)
		nPlayers++;
	else if (gameData.objs.objects [i].nType == OBJ_PLAYER) {
		if (!(i && bCoop))
			nPlayers++;
		}
	else if (gameData.objs.objects [i].nType == OBJ_COOP) {
		if (bCoop)
			nPlayers++;
		}
	if (nPlayers > gameData.multiplayer.nMaxPlayers)
		return 1;
#if 0
	if (gameData.objs.objects [i].nType == OBJ_CNTRLCEN)
		bHaveReactor = 1;
#endif
	}
return !bHaveReactor;
}

//------------------------------------------------------------------------------

void NetworkReadObjectPacket (ubyte *data)
{
	// Object from another net tPlayer we need to sync with

	short nObject, nRemoteObj;
	sbyte obj_owner;
	int nSegment, i;
	tObject *objP;

	static int my_pnum = 0;
	static int mode = 0;
	static int objectCount = 0;
	static int nFrame = 0;
	int nobj = data [1];
	int loc = 3;
	int nRemoteFrame = data [2];
	
nFrame++;
for (i = 0; i < nobj; i++) {
	GET_SHORT (data, loc, nObject);                   
	GET_BYTE (data, loc, obj_owner);                                          
	GET_SHORT (data, loc, nRemoteObj);
	if (nObject == -1) {
		// Clear tObject array
		InitObjects ();
		networkData.bRejoined = 1;
		my_pnum = obj_owner;
		ChangePlayerNumTo (my_pnum);
		mode = 1;
		objectCount = 0;
		nFrame = 1;
		}
	else if (nObject == -2) {
		// Special debug checksum marker for entire send
		if (mode == 1) {
			NetworkPackObjects ();
			mode = 0;
			}
#if 1				
		con_printf (CONDBG, "Objnum -2 found in frame local %d remote %d.\n", nFrame, nRemoteFrame);
		con_printf (CONDBG, "Got %d gameData.objs.objects, zF %d.\n", objectCount, nRemoteObj);
#endif
		if (nRemoteObj != objectCount) {
			Int3 ();
			}
		if (NetworkVerifyObjects (nRemoteObj, objectCount)) {
			// Failed to sync up 
			ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
			networkData.nStatus = NETSTAT_MENU;                          
			return;
			}
		nFrame = 0;
		}
	else {
		if (nFrame != nRemoteFrame)
			Int3 ();
#if 1				
		con_printf (CONDBG, "Got a nType 3 tObject packet!\n");
#endif
		objectCount++;
		if ((obj_owner == my_pnum) || (obj_owner == -1)) {
			if (mode != 1)
				Int3 (); // SEE ROB
			nObject = nRemoteObj;
			//if (nObject > gameData.objs.nLastObject)
			//{
			//      gameData.objs.nLastObject = nObject;
			//      gameData.objs.nObjects = gameData.objs.nLastObject+1;
			//}
			}
		else {
			if (mode == 1) {
				NetworkPackObjects ();
				mode = 0;
				}
			nObject = AllocObject ();
			}
		if (nObject != -1) {
			objP = gameData.objs.objects + nObject;
			if (objP->nSegment != -1)
				UnlinkObject (nObject);
			Assert (objP->nSegment == -1);
			Assert (nObject < MAX_OBJECTS);
			GET_BYTES (data, loc, objP, sizeof (tObject));
			if (gameStates.multi.nGameType >= IPX_GAME)
				SwapObject (objP);
			nSegment = objP->nSegment;
			objP->next = objP->prev = objP->nSegment = -1;
			objP->attachedObj = -1;
			if (nSegment > -1)
				LinkObject (OBJ_IDX (objP), nSegment);
			if ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_GHOST))
				RemapLocalPlayerObject (nObject, nRemoteObj);
			if (obj_owner == my_pnum) 
				MapObjnumLocalToLocal (nObject);
			else if (obj_owner != -1)
				MapObjnumLocalToRemote (nObject, nRemoteObj, obj_owner);
			else
				gameData.multigame.nObjOwner [nObject] = -1;
			}
		} // For a standard onbject
	} // For each tObject in packet
gameData.objs.nObjects = objectCount;
//gameData.objs.nLastObject = gameData.objs.nObjects - 1;
}
	
//------------------------------------------------------------------------------

void NetworkSetGameMode (int gameMode)
{
	gameData.multigame.kills.bShowList = 1;

if (gameMode == NETGAME_ANARCHY)
	;
else if (gameMode == NETGAME_TEAM_ANARCHY)
	gameData.app.nGameMode = GM_TEAM;
else if (gameMode == NETGAME_ROBOT_ANARCHY)
	gameData.app.nGameMode = GM_MULTI_ROBOTS;
else if (gameMode == NETGAME_COOPERATIVE) 
	gameData.app.nGameMode = GM_MULTI_COOP | GM_MULTI_ROBOTS;
else if (gameMode == NETGAME_CAPTURE_FLAG)
		gameData.app.nGameMode = GM_TEAM | GM_CAPTURE;
else if (HoardEquipped ()) {
	if (gameMode == NETGAME_HOARD)
		gameData.app.nGameMode = GM_HOARD;
	else if (gameMode == NETGAME_TEAM_HOARD)
		gameData.app.nGameMode = GM_HOARD | GM_TEAM;
	else if (gameMode == NETGAME_ENTROPY)
		gameData.app.nGameMode = GM_ENTROPY | GM_TEAM;
	else if (gameMode == NETGAME_MONSTERBALL)
		gameData.app.nGameMode = GM_MONSTERBALL | GM_TEAM;
	}
else
	Int3 ();
gameData.app.nGameMode |= GM_NETWORK;
if (gameData.app.nGameMode & GM_TEAM)
	gameData.multigame.kills.bShowList = 3;
}

//------------------------------------------------------------------------------

#if 0

static time_t	nQueryTimeout;

static void QueryPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
	time_t t;

if (NetworkListen () && (networkData.nActiveGames >= MAX_ACTIVE_NETGAMES))
	*key = -2;
else if (*key == KEY_ESC)
	*key = -3;
else if ((t = SDL_GetTicks () - nQueryTimeout) > 5000)
	*key = -4;
else {
	int v = (int) (t / 5);
	if (m [0].value != v) {
		m [0].value = v;
		m [0].rebuild = 1;
		}
	*key = 0;
	}
return;
}

#endif

//------------------------------------------------------------------------------

int GotTeamSpawnPos (void)
{
	int	i, j;

for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	j = FindSegByPoint (&gameData.multiplayer.playerInit [i].position.vPos, -1);
	gameData.multiplayer.playerInit [i].nSegType = (j < 0) ? SEGMENT_IS_NOTHING : gameData.segs.segment2s [j].special;
	switch (gameData.multiplayer.playerInit [i].nSegType) {
		case SEGMENT_IS_GOAL_BLUE:
		case SEGMENT_IS_TEAM_BLUE:
		case SEGMENT_IS_GOAL_RED:
		case SEGMENT_IS_TEAM_RED:
			break;
		default:
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------
// Find the proper initial spawn location for tPlayer i from team t

int TeamSpawnPos (int i)
{
	int	h, j, t = GetTeam (i);

// first find out how many players before tPlayer i are in the same team
// result stored in h
for (h = j = 0; j < i; j++)
	if (GetTeam (j) == t)
		h++;
// assign the spawn location # (h+1) to tPlayer i	
for (j = 0; j < gameData.multiplayer.nPlayerPositions; j++) {
	switch (gameData.multiplayer.playerInit [j].nSegType) {
		case SEGMENT_IS_GOAL_BLUE:
		case SEGMENT_IS_TEAM_BLUE:
			if (!t && (--h < 0))
				return j;
			break;
		case SEGMENT_IS_GOAL_RED:
		case SEGMENT_IS_TEAM_RED:
			if (t && (--h < 0))
				return j;
			break;
		}
	}
return -1;
}

//------------------------------------------------------------------------------

#define LOCAL_NODE \
	((gameStates.multi.bHaveLocalAddress && (gameStates.multi.nGameType == UDP_GAME)) ? \
	 ipx_LocalAddress + 4 : networkData.mySeq.player.network.ipx.node)


int CmpLocalPlayer (network_info *pNetwork, char *pszNetCallSign, char *pszLocalCallSign)
{
if (stricmp (pszNetCallSign, pszLocalCallSign))
	return 1;
#if 0
// if restoring a multiplayer game that had been played via UDP/IP, 
// tPlayer network addresses may have changed, so we have to rely on the callsigns
// This will cause problems if several players with identical callsigns participate
if (gameStates.multi.nGameType == UDP_GAME)
	return 0;
#endif
if (gameStates.multi.nGameType >= IPX_GAME) {
	return memcmp (pNetwork->ipx.node, LOCAL_NODE, extraGameInfo [1].bCheckUDPPort ? 6 : 4) ? 1 : 0;
	}
#ifdef MACINTOSH
if (pNetwork->appletalk.node != networkData.mySeq.player.network.appletalk.node)
	return 1;
#endif
return 0;
}

//------------------------------------------------------------------------------

void NetworkReadSyncPacket (tNetgameInfo * sp, int rsinit)
{
	int				i, j;
	char				szLocalCallSign [CALLSIGN_LEN+1];
	tNetPlayerInfo	*playerP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tNetgameInfo	tmp_info;

if ((gameStates.multi.nGameType >= IPX_GAME) && (sp != &netGame)) { // for macintosh -- get the values unpacked to our structure format
	ReceiveFullNetGamePacket ((ubyte *)sp, &tmp_info);
	sp = &tmp_info;
	}
#endif

if (rsinit)
	tmpPlayersInfo = &netPlayers;
	// This function is now called by all people entering the netgame.
if (sp != &netGame) {
	char *p = (char *) sp;
	ushort h;
	int i, s;
	for (i = 0, h = -1; i < sizeof (tNetgameInfo) - 1; i++, p++) {
		s = *((ushort *) p);
		if (s == networkData.nMySegsCheckSum) {
			h = i;
			break;
			}
		else if (((s / 256) + (s % 256) * 256) == networkData.nMySegsCheckSum) {
			h = i;
			break;
			}
		}
	memcpy (&netGame, sp, sizeof (tNetgameInfo));
	memcpy (&netPlayers, tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
	}
gameData.multiplayer.nPlayers = sp->nNumPlayers;
gameStates.app.nDifficultyLevel = sp->difficulty;
networkData.nStatus = sp->gameStatus;
//Assert (gameStates.app.nFunctionMode != FMODE_GAME);
// New code, 11/27
#if 1				
con_printf (1, "netGame.checksum = %d, calculated checksum = %d.\n", 
			   netGame.segments_checksum, networkData.nMySegsCheckSum);
#endif
if (netGame.segments_checksum != networkData.nMySegsCheckSum) {
	if (extraGameInfo [0].bAutoDownload)
		networkData.nStatus = NETSTAT_AUTODL;
	else {
		short nInMenu = gameStates.menus.nInMenu;
		gameStates.menus.nInMenu = 0;
		networkData.nStatus = NETSTAT_MENU;
		ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
		gameStates.menus.nInMenu = nInMenu;
		}
#if 1//def RELEASE
		return;
#endif
	}
// Discover my tPlayer number
memcpy (szLocalCallSign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
gameData.multiplayer.nLocalPlayer = -1;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	gameData.multiplayer.players [i].netKillsTotal = 0;

for (i = 0, playerP = tmpPlayersInfo->players; i < gameData.multiplayer.nPlayers; i++, playerP++) {
	if (!CmpLocalPlayer (&playerP->network, playerP->callsign, szLocalCallSign)) {
		if (gameData.multiplayer.nLocalPlayer != -1) {
			Int3 (); // Hey, we've found ourselves twice
			ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_DUPLICATE_PLAYERS);
			con_printf (CONDBG, TXT_FOUND_TWICE);
			networkData.nStatus = NETSTAT_MENU;
			return; 
			}
		ChangePlayerNumTo (i);
		}
	memcpy (gameData.multiplayer.players [i].callsign, playerP->callsign, CALLSIGN_LEN+1);
	if (gameStates.multi.nGameType >= IPX_GAME) {
#ifdef WORDS_NEED_ALIGNMENT
		uint server;
		memcpy (&server, playerP->network.ipx.server, 4);
		if (server != 0)
			IpxGetLocalTarget (
				(ubyte *)&server, 
				tmpPlayersInfo->players [i].network.ipx.node, 
				gameData.multiplayer.players [i].netAddress);
#else // WORDS_NEED_ALIGNMENT
		if ((* (uint *)tmpPlayersInfo->players [i].network.ipx.server) != 0)
			IpxGetLocalTarget (
				tmpPlayersInfo->players [i].network.ipx.server, 
				tmpPlayersInfo->players [i].network.ipx.node, 
				gameData.multiplayer.players [i].netAddress);
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy (gameData.multiplayer.players [i].netAddress, tmpPlayersInfo->players [i].network.ipx.node, 6);
		}
	gameData.multiplayer.players [i].nPacketsGot = 0;                             // How many packets we got from them
	gameData.multiplayer.players [i].nPacketsSent = 0;                            // How many packets we sent to them
	gameData.multiplayer.players [i].connected = playerP->connected;
	gameData.multiplayer.players [i].netKillsTotal = sp->playerKills [i];
	gameData.multiplayer.players [i].netKilledTotal = sp->killed [i];
	if (networkData.bRejoined || (i != gameData.multiplayer.nLocalPlayer))
		gameData.multiplayer.players [i].score = sp->player_score [i];
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		gameData.multigame.kills.matrix [i][j] = sp->kills [i][j];
	}

if (gameData.multiplayer.nLocalPlayer < 0) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PLAYER_REJECTED);
	networkData.nStatus = NETSTAT_MENU;
	return;
	}
if (networkData.bRejoined) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		gameData.multiplayer.players [i].netKilledTotal = sp->killed [i];
	NetworkProcessMonitorVector (sp->monitor_vector);
	LOCALPLAYER.timeLevel = sp->xLevelTime;
	}
gameData.multigame.kills.nTeam [0] = sp->teamKills [0];
gameData.multigame.kills.nTeam [1] = sp->teamKills [1];
LOCALPLAYER.connected = 1;
netPlayers.players [gameData.multiplayer.nLocalPlayer].connected = 1;
netPlayers.players [gameData.multiplayer.nLocalPlayer].rank=GetMyNetRanking ();
if (!networkData.bRejoined) {
	int	j, bGotTeamSpawnPos = (gameData.app.nGameMode & GM_TEAM) && GotTeamSpawnPos ();
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		if (bGotTeamSpawnPos) {
			j = TeamSpawnPos (i);
			if (j < 0)
				j = netGame.locations [i];
			}
		else
			j = netGame.locations [i];
		gameData.objs.objects [gameData.multiplayer.players [i].nObject].position.vPos = gameData.multiplayer.playerInit [j].position.vPos;
		gameData.objs.objects [gameData.multiplayer.players [i].nObject].position.mOrient = gameData.multiplayer.playerInit [j].position.mOrient;
		RelinkObject (gameData.multiplayer.players [i].nObject, gameData.multiplayer.playerInit [j].nSegment);
		}
	}
gameData.objs.objects [LOCALPLAYER.nObject].nType = OBJ_PLAYER;
networkData.nStatus = NETSTAT_PLAYING;
SetFunctionMode (FMODE_GAME);
MultiSortKillList ();
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
	if (gameData.app.nGameMode & GM_MULTI_COOP)
		netGame.locations [i] = i;
	else {
		do {
			np = d_rand () % gameData.multiplayer.nPlayerPositions;
			for (j = 0; j < i; j++) {
				if (netGame.locations [j] == np) {
					np =-1;
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
netGame.segments_checksum = networkData.nMySegsCheckSum;
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

int NetworkStartGame ()
{
	int i, bAutoRun;

if (gameStates.multi.nGameType >= IPX_GAME) {
	Assert (FRAME_INFO_SIZE < IPX_MAX_DATA_SIZE);
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
// LoadLevel (level); Old, no longer used.

netGame.difficulty = gameStates.app.nDifficultyLevel;
netGame.gameMode = mpParams.nGameMode;
netGame.gameStatus = NETSTAT_STARTING;
netGame.nNumPlayers = 0;
netGame.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
netGame.nLevel = mpParams.nLevel;
netGame.protocol_version = MULTI_PROTO_VERSION;

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
netGame.nSecurity=d_rand ();  // For syncing Netgames with tPlayer packets
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
networkData.nNamesInfoSecurity=-1;
networkData.bGamesChanged = 1;      
}

//------------------------------------------------------------------------------
/* Polling loop waiting for sync packet to start game
 * after having sent request
 */
void NetworkSyncPoll (int nitems, tMenuItem * menus, int * key, int citem)
{
	static fix t1 = 0;

NetworkListen ();
if (networkData.nStatus != NETSTAT_WAITING)  // Status changed to playing, exit the menu
	*key = -2;
else if (!networkData.bRejoined && (TimerGetApproxSeconds () > t1 + F1_0 * 2)) {
	int i;

	// Poll time expired, re-send request
	t1 = TimerGetApproxSeconds ();
#if 1				
	con_printf (CONDBG, "Re-sending join request.\n");
#endif
	i = NetworkSendRequest ();
	if (i < 0)
		*key = -2;
	}
}

//------------------------------------------------------------------------------

int NetworkWaitForSync (void)
{
	char					text [60];
	tMenuItem		m [2];
	int					i, choice;
	tSequencePacket	me;
	
networkData.nStatus = NETSTAT_WAITING;
memset (m, 0, sizeof (m));
m [0].nType = NM_TYPE_TEXT; 
m [0].text = text;
m [1].nType = NM_TYPE_TEXT; 
m [1].text = TXT_NET_LEAVE;
networkData.bRejoined = 0;
i = NetworkSendRequest ();
if (i < 0)
	return -1;
sprintf (m [0].text, "%s\n'%s' %s", TXT_NET_WAITING, netPlayers.players [i].callsign, TXT_NET_TO_ENTER);

do_menu:   

choice = ExecMenu (NULL, TXT_WAIT, 2, m, NetworkSyncPoll, NULL);
if (choice > -1)
	goto do_menu;
if (networkData.nStatus == NETSTAT_PLAYING)  
	return 0;
else if (networkData.nStatus == NETSTAT_AUTODL)
	if (DownloadMission (netGame.szMissionName))
		return 1;
#if 1				
con_printf (CONDBG, "Aborting join.\n");
#endif
me.nType = PID_QUIT_JOINING;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.ipx.node, IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.ipx.server, IpxGetMyServerAddress (), 4);
	SendInternetSequencePacket (me, netPlayers.players [0].network.ipx.server, netPlayers.players [0].network.ipx.node);
}
gameData.multiplayer.nPlayers = 0;
SetFunctionMode (FMODE_MENU);
gameData.app.nGameMode = GM_GAME_OVER;
return -1;     // they cancelled               
}

//------------------------------------------------------------------------------

void NetworkRequestPoll (int nitems, tMenuItem * menus, int * key, int citem)
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int num_ready = 0;

menus = menus;
citem = citem;
nitems = nitems;
key = key;
NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected == 1) || (gameData.multiplayer.players [i].connected == 0))
		num_ready++;
	}
if (num_ready == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	*key = -2;
}

//------------------------------------------------------------------------------

void NetworkWaitForRequests (void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	tMenuItem m [1];
	
networkData.nStatus = NETSTAT_WAITING;
memset (m, 0, sizeof (m));
m [0].nType=NM_TYPE_TEXT; 
m [0].text = TXT_NET_LEAVE;
networkData.nStatus = NETSTAT_WAITING;
NetworkFlush ();
LOCALPLAYER.connected = 1;

do_menu:

choice = ExecMenu (NULL, TXT_WAIT, 1, m, NetworkRequestPoll, NULL);        
if (choice == -1) {
	// User aborted
	choice = ExecMessageBox (NULL, NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
	if (choice == 2)
		return;
	if (choice != 0)
		goto do_menu;
		
	// User confirmed abort
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((gameData.multiplayer.players [i].connected != 0) && (i != gameData.multiplayer.nLocalPlayer)) {
			if (gameStates.multi.nGameType >= IPX_GAME)
				NetworkDumpPlayer (netPlayers.players [i].network.ipx.server, netPlayers.players [i].network.ipx.node, DUMP_ABORTED);
			}
		}
		longjmp (gameExitPoint, 0);  
	}
else if (choice != -2)
	goto do_menu;
}

//------------------------------------------------------------------------------

/* Do required syncing after each level, before starting new one */
int NetworkLevelSync (void)
{
	int result;
	networkData.mySyncPackInited = 0;

//networkData.nMySegsCheckSum = NetMiscCalcCheckSum (gameData.segs.segments, sizeof (tSegment)* (gameData.segs.nLastSegment+1);
NetworkFlush (); // Flush any old packets
if (!gameData.multiplayer.nPlayers)
	result = NetworkWaitForSync ();
else if (NetworkIAmMaster ()) {
	NetworkWaitForRequests ();
	NetworkSendSync ();
	result = (gameData.multiplayer.nLocalPlayer < 0) ? -1 : 0;
	}
else
	result = NetworkWaitForSync ();

NetworkCountPowerupsInMine ();

if (result < 0) {
	LOCALPLAYER.connected = 0;
	NetworkSendEndLevelPacket ();
	longjmp (gameExitPoint, 0);
	}
return (result);
}

//------------------------------------------------------------------------------

void NetworkCountPowerupsInMine (void)
 {
  int i;

memset (gameData.multiplayer.powerupsInMine, 0, sizeof (gameData.multiplayer.powerupsInMine));
for (i = 0; i <= gameData.objs.nLastObject; i++) {
	if (gameData.objs.objects [i].nType == OBJ_POWERUP) {
		gameData.multiplayer.powerupsInMine [gameData.objs.objects [i].id]++;
		if (MultiPowerupIs4Pack (gameData.objs.objects [i].id))
			gameData.multiplayer.powerupsInMine [gameData.objs.objects [i].id-1]+=4;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkAdjustMaxDataSize ()
{
networkData.nMaxXDataSize = netGame.bShortPackets ? NET_XDATA_SIZE : NET_XDATA_SIZE;
}

//------------------------------------------------------------------------------

#define ALL_INFO_REQUEST_INTERVAL F1_0*3

void NetworkWaitAllPoll (int nitems, tMenuItem * menus, int * key, int citem)
{
	static fix t1=0;

if (TimerGetApproxSeconds () > t1+ALL_INFO_REQUEST_INTERVAL) {
	NetworkSendAllInfoRequest (PID_SEND_ALL_GAMEINFO, nSecurityCheck);
	t1 = TimerGetApproxSeconds ();
	}
NetworkDoBigWait (networkData.bWaitAllChoice);  
if (nSecurityCheck == -1)
	*key=-2;
}
 
//------------------------------------------------------------------------------

int NetworkWaitForAllInfo (int choice)
 {
  int pick;
  
  tMenuItem m [2];

memset (m, 0, sizeof (m));
m [0].nType=NM_TYPE_TEXT; 
m [0].text = "Press Escape to cancel";
networkData.bWaitAllChoice=choice;
networkData.nStartWaitAllTime=TimerGetApproxSeconds ();
nSecurityCheck=activeNetGames [choice].nSecurity;
networkData.nSecurityFlag=0;

get_menu:

pick=ExecMenu (NULL, TXT_CONNECTING, 1, m, NetworkWaitAllPoll, NULL);
if (pick>-1 && nSecurityCheck!=-1)
	goto get_menu;
if (nSecurityCheck == -1) {   
	nSecurityCheck=0;     
	return 1;
	}
nSecurityCheck=0;      
return 0;
 }

//------------------------------------------------------------------------------

void NetworkDoBigWait (int choice)
{
	int size;
	ubyte packet [IPX_MAX_DATA_SIZE], *data;
	tAllNetPlayersInfo *temp_info;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tAllNetPlayersInfo info_struct;
#endif
  
while (0 < (size = IpxGetPacketData (packet))) {
	data = packet;

	switch (data [0]) {  
		case PID_GAME_INFO:
			if (gameStates.multi.nGameType >= IPX_GAME)
				ReceiveFullNetGamePacket (data, &TempNetInfo); 
			else
				memcpy ((ubyte *)&TempNetInfo, data, sizeof (tNetgameInfo));
#if SECURITY_CHECK
			if (TempNetInfo.nSecurity !=nSecurityCheck)
				break;
#endif
			if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_GAMEINFO) {
#if SECURITY_CHECK
				if (tmpPlayersInfo->nSecurity == TempNetInfo.nSecurity) {
#else
					{
#endif
#if SECURITY_CHECK
					if (tmpPlayersInfo->nSecurity == nSecurityCheck) {
#else
						{
#endif
						memcpy (&activeNetGames [choice], (ubyte *)&TempNetInfo, sizeof (tNetgameInfo));
						memcpy (&activeNetPlayers [choice], tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
						nSecurityCheck=-1;
						}
					}
				}
			else {
				networkData.nSecurityFlag=NETSECURITY_WAIT_FOR_PLAYERS;
				networkData.nSecurityNum=TempNetInfo.nSecurity;
				if (NetworkWaitForPlayerInfo ()) {
#if 1				
					con_printf (CONDBG, "HUH? Game=%d Player=%d\n", 
									networkData.nSecurityNum, tmpPlayersInfo->nSecurity);
#endif
					memcpy (&activeNetGames [choice], (ubyte *)&TempNetInfo, sizeof (tNetgameInfo));
					memcpy (&activeNetPlayers [choice], tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
					nSecurityCheck = -1;
					}
				networkData.nSecurityFlag = 0;
				networkData.nSecurityNum = 0;
				}
			break;

		case PID_EXTRA_GAMEINFO: 
			if (gameStates.multi.nGameType >= IPX_GAME) {
				NetworkProcessExtraGameInfo (data);
				}
			break;

		case PID_PLAYERSINFO:
			if (gameStates.multi.nGameType >= IPX_GAME) {
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
				temp_info = (tAllNetPlayersInfo *)data;
#else
				ReceiveNetPlayersPacket (data, &info_struct);
				temp_info = &info_struct;
#endif
				}
			else
				temp_info = (tAllNetPlayersInfo *)data;
#if SECURITY_CHECK
			if (temp_info->nSecurity!=nSecurityCheck) 
				break;     // If this isn't the guy we're looking for, move on
#endif
			memcpy (&tmpPlayersBase, (ubyte *)&temp_info, sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo=&tmpPlayersBase;
			networkData.bWaitingForPlayerInfo=0;
			networkData.nSecurityNum = tmpPlayersInfo->nSecurity;
			networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_GAMEINFO;
			break;

		default:
			break;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkLeaveGame ()
{ 
   int nsave;
		
NetworkDoFrame (1, 1);
#ifdef NETPROFILING
fclose (SendLogFile);
	fclose (ReceiveLogFile);
#endif
if ((NetworkIAmMaster ())) {
	while (networkData.bSendingExtras>1 && networkData.nPlayerJoiningExtras!=-1)
		NetworkSendExtras ();

	netGame.nNumPlayers = 0;
	nsave=gameData.multiplayer.nPlayers;
	gameData.multiplayer.nPlayers=0;
	NetworkSendGameInfo (NULL);
	gameData.multiplayer.nPlayers=nsave;
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

void NetworkFlush ()
{
	ubyte packet [IPX_MAX_DATA_SIZE];
	
if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return;
while (IpxGetPacketData (packet) > 0) 
	 ;
}

//------------------------------------------------------------------------------

int NetworkListen ()
{
	int size;
	ubyte packet [IPX_MAX_DATA_SIZE];
	int i, t, nPackets = 0, loopmax = 999;

CleanUploadDests ();
AddServerToTracker ();
if ((networkData.nStatus == NETSTAT_PLAYING) && netGame.bShortPackets && !networkData.bSendObjects)
	loopmax = gameData.multiplayer.nPlayers * netGame.nPacketsPerSec;

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
	for (i = nPackets = 0; (i < loopmax) && (SDL_GetTicks () - t < 50); i++) {
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

int NetworkWaitForPlayerInfo ()
{
	int size=0, retries=0;
	ubyte packet [IPX_MAX_DATA_SIZE];
	struct tAllNetPlayersInfo *TempInfo;
	fix basetime;
	ubyte id=0;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tAllNetPlayersInfo info_struct;
#endif

if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return 0;
#if 1				
if (!(gameData.app.nGameMode & GM_NETWORK) && (gameStates.app.nFunctionMode == FMODE_GAME))
	con_printf (CONDBG, "Calling NetworkWaitForPlayerInfo () when not in net game.\n");
#endif		
if (networkData.nStatus == NETSTAT_PLAYING) {
	Int3 (); //MY GOD! Get Jason...this is the source of many problems
	return 0;
	}
basetime=TimerGetApproxSeconds ();
while (networkData.bWaitingForPlayerInfo && (retries < 50) && 
	    (TimerGetApproxSeconds ()< (basetime+ (F1_0*5)))) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		size = IpxGetPacketData (packet);
		id = packet [0];
		}
	if ((size > 0) && (id == PID_PLAYERSINFO)) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
		ReceiveNetPlayersPacket (packet, &info_struct);
		TempInfo = &info_struct;
#else
		TempInfo= (tAllNetPlayersInfo *)packet;
#endif
		retries++;
		if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_PLAYERS) {
#if SECURITY_CHECK
			if (networkData.nSecurityNum != TempInfo->nSecurity)
				continue;
#endif
			memcpy (&tmpPlayersBase, (ubyte *)TempInfo, sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo=&tmpPlayersBase;
			networkData.nSecurityFlag=NETSECURITY_OFF;
			networkData.nSecurityNum=0;
			networkData.bWaitingForPlayerInfo=0;
			return 1;
			}
		else {
			networkData.nSecurityNum = TempInfo->nSecurity;
			networkData.nSecurityFlag=NETSECURITY_WAIT_FOR_GAMEINFO;
			memcpy (&tmpPlayersBase, (ubyte *)TempInfo, sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo=&tmpPlayersBase;
			networkData.bWaitingForPlayerInfo=0;
			return 1;
			}
		}
	}
return 0;
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
if (!networkData.mySyncPackInited) {
	networkData.mySyncPackInited = 1;
	memset (&networkData.mySyncPack, 0, sizeof (tFrameInfo));
	}
if (urgent)
	networkData.bPacketUrgent = 1;
if ((networkData.mySyncPack.data_size + len) > networkData.nMaxXDataSize) {
	bCheck = ptr [0];
	NetworkDoFrame (1, 0);
	if (networkData.mySyncPack.data_size != 0) {
#if 1				
	con_printf (CONDBG, "%d bytes were added to data by NetworkDoFrame!\n", networkData.mySyncPack.data_size);
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
	if (networkData.mySyncPack.data_size && !bD2XData && networkData.bD2XData)
		NetworkDoFrame (1, 0);
	networkData.bD2XData = bD2XData;
	}
Assert (networkData.mySyncPack.data_size + len <= networkData.nMaxXDataSize);
memcpy (networkData.mySyncPack.data + networkData.mySyncPack.data_size, ptr, len);
networkData.mySyncPack.data_size += len;
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
CreatePlayerAppearanceEffect (gameData.objs.objects + gameData.multiplayer.players [nPlayer].nObject);
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

fix xLastSendTime = 0;
fix xLastTimeoutCheck = 0;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
void SquishShortFrameInfo (tFrameInfoShort old_info, ubyte *data)
{
	int 	loc = 0;
	int 	tmpi;
	short tmps;
		
SET_BYTE (data, loc, old_info.nType);                                            
loc += 3;
SET_INT (data, loc, old_info.numpackets);
SET_BYTES (data, loc, old_info.thepos.bytemat, 9);                   
SET_SHORT (data, loc, old_info.thepos.xo);
SET_SHORT (data, loc, old_info.thepos.yo);
SET_SHORT (data, loc, old_info.thepos.zo);
SET_SHORT (data, loc, old_info.thepos.nSegment);
SET_SHORT (data, loc, old_info.thepos.velx);
SET_SHORT (data, loc, old_info.thepos.vely);
SET_SHORT (data, loc, old_info.thepos.velz);
SET_SHORT (data, loc, old_info.data_size);
SET_BYTE (data, loc, old_info.nPlayer);                                     
SET_BYTE (data, loc, old_info.obj_renderType);                               
SET_BYTE (data, loc, old_info.level_num);                                     
SET_BYTES (data, loc, old_info.data, old_info.data_size);
}
#endif

//------------------------------------------------------------------------------

char NakedBuf [NET_XDATA_SIZE+4];
int NakedPacketLen=0;
int NakedPacketDestPlayer=-1;

void NetworkDoFrame (int force, int listen)
{
	tFrameInfoShort ShortSyncPack;
	static fix LastEndlevel=0;
	int i;

if (!(gameData.app.nGameMode&GM_NETWORK)) 
	return;
if ((networkData.nStatus != NETSTAT_PLAYING) || (gameStates.app.bEndLevelSequence)) // Don't send postion during escape sequence...
	goto listen;

if (NakedPacketLen) {
	Assert (NakedPacketDestPlayer >- 1);
	if (gameStates.multi.nGameType >= IPX_GAME) 
		IPXSendPacketData ((ubyte *) NakedBuf, NakedPacketLen, 
								 netPlayers.players [NakedPacketDestPlayer].network.ipx.server, 
								 netPlayers.players [NakedPacketDestPlayer].network.ipx.node, 
								 gameData.multiplayer.players [NakedPacketDestPlayer].netAddress);
	NakedPacketLen = 0;
	NakedPacketDestPlayer = -1;
	}
if (bWaitForRefuseAnswer && TimerGetApproxSeconds ()> (RefuseTimeLimit+ (F1_0*12)))
	bWaitForRefuseAnswer=0;
xLastSendTime += gameData.time.xFrame;
xLastTimeoutCheck += gameData.time.xFrame;

// Send out packet PacksPerSec times per second maximum... unless they fire, then send more often...
if ((xLastSendTime > F1_0 / netGame.nPacketsPerSec) || 
	  (gameData.multigame.laser.bFired) || force || networkData.bPacketUrgent) {        
	if (LOCALPLAYER.connected) {
		int nObject = LOCALPLAYER.nObject;
		networkData.bPacketUrgent = 0;
		if (listen) {
			MultiSendRobotFrame (0);
			MultiSendFire ();              // Do firing if needed..
			}
		xLastSendTime = 0;
		if (netGame.bShortPackets) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
			ubyte send_data [IPX_MAX_DATA_SIZE];
#endif
			memset (&ShortSyncPack, 0, sizeof (ShortSyncPack));
			CreateShortPos (&ShortSyncPack.thepos, gameData.objs.objects+nObject, 0);
			ShortSyncPack.nType = PID_PDATA;
			ShortSyncPack.nPlayer = gameData.multiplayer.nLocalPlayer;
			ShortSyncPack.obj_renderType = gameData.objs.objects [nObject].renderType;
			ShortSyncPack.level_num = gameData.missions.nCurrentLevel;
			ShortSyncPack.data_size = networkData.mySyncPack.data_size;
			memcpy (ShortSyncPack.data, networkData.mySyncPack.data, networkData.mySyncPack.data_size);
			networkData.mySyncPack.numpackets = INTEL_INT (gameData.multiplayer.players [0].nPacketsSent++);
			ShortSyncPack.numpackets = networkData.mySyncPack.numpackets;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
			IpxSendGamePacket (
				(ubyte*)&ShortSyncPack, 
				sizeof (tFrameInfoShort) - networkData.nMaxXDataSize + networkData.mySyncPack.data_size);
#else
			SquishShortFrameInfo (ShortSyncPack, send_data);
			IpxSendGamePacket (
				(ubyte*)send_data, 
				IPX_SHORT_INFO_SIZE-networkData.nMaxXDataSize+networkData.mySyncPack.data_size);
#endif
			}
		else {// If long packets
				int send_data_size;

			networkData.mySyncPack.nType = PID_PDATA;
			networkData.mySyncPack.nPlayer = gameData.multiplayer.nLocalPlayer;
			networkData.mySyncPack.obj_renderType = gameData.objs.objects [nObject].renderType;
			networkData.mySyncPack.level_num = gameData.missions.nCurrentLevel;
			networkData.mySyncPack.obj_segnum = gameData.objs.objects [nObject].nSegment;
			networkData.mySyncPack.obj_pos = gameData.objs.objects [nObject].position.vPos;
			networkData.mySyncPack.obj_orient = gameData.objs.objects [nObject].position.mOrient;
			networkData.mySyncPack.phys_velocity = gameData.objs.objects [nObject].mType.physInfo.velocity;
			networkData.mySyncPack.phys_rotvel = gameData.objs.objects [nObject].mType.physInfo.rotVel;
			send_data_size = networkData.mySyncPack.data_size;                  // do this so correct size data is sent
	#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)                        // do the swap stuff
			if (gameStates.multi.nGameType >= IPX_GAME) {
				networkData.mySyncPack.obj_segnum = INTEL_SHORT (networkData.mySyncPack.obj_segnum);
				INTEL_VECTOR (&networkData.mySyncPack.obj_pos);
				INTEL_MATRIX (&networkData.mySyncPack.obj_orient);
				INTEL_VECTOR (&networkData.mySyncPack.phys_velocity);
				INTEL_VECTOR (&networkData.mySyncPack.phys_rotvel);
				networkData.mySyncPack.data_size = INTEL_SHORT (networkData.mySyncPack.data_size);
				}
	#endif
			networkData.mySyncPack.numpackets = INTEL_INT (gameData.multiplayer.players [0].nPacketsSent++);
			IpxSendGamePacket (
				(ubyte*)&networkData.mySyncPack, 
				sizeof (tFrameInfo) - networkData.nMaxXDataSize + send_data_size);
			}
		networkData.mySyncPack.data_size = 0;               // Start data over at 0 length.
		networkData.bD2XData = 0;
		if (gameData.reactor.bDestroyed) {
			if (gameStates.app.bPlayerIsDead)
				LOCALPLAYER.connected=3;
			if (TimerGetApproxSeconds () > (LastEndlevel+ (F1_0/2))) {
				NetworkSendEndLevelPacket ();
				LastEndlevel = TimerGetApproxSeconds ();
				}
			}
		}
	}

if (!listen)
	return;

if ((xLastTimeoutCheck > F1_0) && !(gameData.reactor.bDestroyed)) {
	fix approxTime = TimerGetApproxSeconds ();
// Check for tPlayer timeouts
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if ((i != gameData.multiplayer.nLocalPlayer) && 
			 ((gameData.multiplayer.players [i].connected == 1) || bDownloading [i])) {
			if ((networkData.nLastPacketTime [i] == 0) || 
				 (networkData.nLastPacketTime [i] > approxTime)) {
				ResetPlayerTimeout (i, approxTime);
				continue;
				}
#if 1//ndef _DEBUG
			if (gameOpts->multi.bTimeoutPlayers && 
				 (approxTime - networkData.nLastPacketTime [i]) > (15*F1_0))
				NetworkTimeoutPlayer (i);
#endif
			}
		}
	xLastTimeoutCheck = 0;
	}

listen:

if (!listen) {
	networkData.mySyncPack.data_size = 0;
	return;
	}
NetworkListen ();
if (networkData.bVerifyPlayerJoined!=-1 && !(gameData.app.nFrameCount & 63))
	ResendSyncDueToPacketLossForAllender (); // This will resend to network_player_rejoining
if (networkData.bSendObjects)
	NetworkSendObjects ();
if (networkData.bSendingExtras && (networkData.bVerifyPlayerJoined == -1))
	NetworkSendExtras ();
}

//------------------------------------------------------------------------------

int nMissedPackets = 0;
int nConsistencyErrorCount = 0;

void NetworkConsistencyError (void)
{
if (++nConsistencyErrorCount < 10)
	return;
SetFunctionMode (FMODE_MENU);
ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
SetFunctionMode (FMODE_GAME);
nConsistencyErrorCount = 0;
gameData.multigame.bQuitGame = 1;
gameData.multigame.menu.bLeave = 1;
MultiResetStuff ();
SetFunctionMode (FMODE_MENU);
}

//------------------------------------------------------------------------------

void NetworkProcessPData (char *data)
{
Assert (gameData.app.nGameMode & GM_NETWORK);
if (netGame.bShortPackets)
	NetworkReadPDataShortPacket ((tFrameInfoShort *)data);
else
	NetworkReadPDataPacket ((tFrameInfo *)data);
}

//------------------------------------------------------------------------------

void NetworkReadPDataPacket (tFrameInfo *pd)
{
	int nTheirPlayer;
	int theirObjNum;
	tObject * theirObjP = NULL;
	
// tFrameInfo should be aligned...for mac, make the necessary adjustments
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
if (gameStates.multi.nGameType >= IPX_GAME) {
	pd->numpackets = INTEL_INT (pd->numpackets);
	pd->obj_pos.p.x = INTEL_INT (pd->obj_pos.p.x);
	pd->obj_pos.p.y = INTEL_INT (pd->obj_pos.p.y);
	pd->obj_pos.p.z = INTEL_INT (pd->obj_pos.p.z);
	pd->obj_orient.rVec.p.x = (fix)INTEL_INT ((int)pd->obj_orient.rVec.p.x);
	pd->obj_orient.rVec.p.y = (fix)INTEL_INT ((int)pd->obj_orient.rVec.p.y);
	pd->obj_orient.rVec.p.z = (fix)INTEL_INT ((int)pd->obj_orient.rVec.p.z);
	pd->obj_orient.uVec.p.x = (fix)INTEL_INT ((int)pd->obj_orient.uVec.p.x);
	pd->obj_orient.uVec.p.y = (fix)INTEL_INT ((int)pd->obj_orient.uVec.p.y);
	pd->obj_orient.uVec.p.z = (fix)INTEL_INT ((int)pd->obj_orient.uVec.p.z);
	pd->obj_orient.fVec.p.x = (fix)INTEL_INT ((int)pd->obj_orient.fVec.p.x);
	pd->obj_orient.fVec.p.y = (fix)INTEL_INT ((int)pd->obj_orient.fVec.p.y);
	pd->obj_orient.fVec.p.z = (fix)INTEL_INT ((int)pd->obj_orient.fVec.p.z);
	pd->phys_velocity.p.x = (fix)INTEL_INT ((int)pd->phys_velocity.p.x);
	pd->phys_velocity.p.y = (fix)INTEL_INT ((int)pd->phys_velocity.p.y);
	pd->phys_velocity.p.z = (fix)INTEL_INT ((int)pd->phys_velocity.p.z);
	pd->phys_rotvel.p.x = (fix)INTEL_INT ((int)pd->phys_rotvel.p.x);
	pd->phys_rotvel.p.y = (fix)INTEL_INT ((int)pd->phys_rotvel.p.y);
	pd->phys_rotvel.p.z = (fix)INTEL_INT ((int)pd->phys_rotvel.p.z);
	pd->obj_segnum = INTEL_SHORT (pd->obj_segnum);
	pd->data_size = INTEL_SHORT (pd->data_size);
	}
#endif
nTheirPlayer = pd->nPlayer;
theirObjNum = gameData.multiplayer.players [pd->nPlayer].nObject;
if (nTheirPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if ((networkData.bVerifyPlayerJoined != -1) && (nTheirPlayer == networkData.bVerifyPlayerJoined))
	networkData.bVerifyPlayerJoined = -1;
if (!gameData.multigame.bQuitGame && (nTheirPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus!=NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->data_size > 0)
		// pass pd->data to some parser function....
		MultiProcessBigData ((char *) pd->data, pd->data_size);
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
if ((sbyte)pd->level_num != gameData.missions.nCurrentLevel) {
#if 1				
	con_printf (CONDBG, "Got frame packet from tPlayer %d wrong level %d!\n", pd->nPlayer, pd->level_num);
#endif
	return;
	}

theirObjP = gameData.objs.objects + theirObjNum;
//------------- Keep track of missed packets -----------------
gameData.multiplayer.players [nTheirPlayer].nPacketsGot++;
networkData.nTotalPacketsGot++;
ResetPlayerTimeout (nTheirPlayer, -1);
if  (pd->numpackets != gameData.multiplayer.players [nTheirPlayer].nPacketsGot) {
	int nMissedPackets = pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
	if ((pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot)>0)
		networkData.nTotalMissedPackets += pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
#if 1				
	if (nMissedPackets > 0)       
		con_printf (0, 
			"Missed %d packets from tPlayer #%d (%d total)\n", 
			pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot, 
			nTheirPlayer, 
			nMissedPackets);
	else
		con_printf (CONDBG, 
			"Got %d late packets from tPlayer #%d (%d total)\n", 
			gameData.multiplayer.players [nTheirPlayer].nPacketsGot-pd->numpackets, 
			nTheirPlayer, 
			nMissedPackets);
#endif
	gameData.multiplayer.players [nTheirPlayer].nPacketsGot = pd->numpackets;
	}
//------------ Read the tPlayer's ship's tObject info ----------------------
theirObjP->position.vPos = pd->obj_pos;
theirObjP->position.mOrient = pd->obj_orient;
theirObjP->mType.physInfo.velocity = pd->phys_velocity;
theirObjP->mType.physInfo.rotVel = pd->phys_rotvel;
if ((theirObjP->renderType != pd->obj_renderType) && (pd->obj_renderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nTheirPlayer);
RelinkObject (theirObjNum, pd->obj_segnum);
if (theirObjP->movementType == MT_PHYSICS)
	SetThrustFromVelocity (theirObjP);
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nTheirPlayer].connected) {
	gameData.multiplayer.players [nTheirPlayer].connected = 1;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nTheirPlayer);
	MultiMakeGhostPlayer (nTheirPlayer);
	CreatePlayerAppearanceEffect (gameData.objs.objects + theirObjNum);
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	ClipRank ((char *) &netPlayers.players [nTheirPlayer].rank);
	if (gameOpts->multi.bNoRankings)      
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s", pszRankStrings [netPlayers.players [nTheirPlayer].rank], gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra data at the end ---------------
if (pd->data_size > 0)
	// pass pd->data to some parser function....
	MultiProcessBigData ((char *) pd->data, pd->data_size);
}

//------------------------------------------------------------------------------

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
void GetShortFrameInfo (ubyte *old_info, tFrameInfoShort *new_info)
{
	int loc = 0;
	
GET_BYTE (old_info, loc, new_info->nType);
/* skip three for pad byte */                                       
loc += 3;
GET_INT (old_info, loc, new_info->numpackets);
GET_BYTES (old_info, loc, new_info->thepos.bytemat, 9);
GET_SHORT (old_info, loc, new_info->thepos.xo);
GET_SHORT (old_info, loc, new_info->thepos.yo);
GET_SHORT (old_info, loc, new_info->thepos.zo);
GET_SHORT (old_info, loc, new_info->thepos.nSegment);
GET_SHORT (old_info, loc, new_info->thepos.velx);
GET_SHORT (old_info, loc, new_info->thepos.vely);
GET_SHORT (old_info, loc, new_info->thepos.velz);
GET_SHORT (old_info, loc, new_info->data_size);
GET_BYTE (old_info, loc, new_info->nPlayer);
GET_BYTE (old_info, loc, new_info->obj_renderType);
GET_BYTE (old_info, loc, new_info->level_num);
GET_BYTES (old_info, loc, new_info->data, new_info->data_size);
}
#else

#define GetShortFrameInfo(old_info, new_info) \
	memcpy (new_info, old_info, sizeof (tFrameInfoShort))

#endif

//------------------------------------------------------------------------------

void NetworkReadPDataShortPacket (tFrameInfoShort *pd)
{
	int nTheirPlayer;
	int theirObjNum;
	tObject * theirObjP = NULL;
	tFrameInfoShort new_pd;

// short frame info is not aligned because of tShortPos.  The mac
// will call totally hacked and gross function to fix this up.

if (gameStates.multi.nGameType >= IPX_GAME)
	GetShortFrameInfo ((ubyte *)pd, &new_pd);
else
	memcpy (&new_pd, (ubyte *)pd, sizeof (tFrameInfoShort));
nTheirPlayer = new_pd.nPlayer;
theirObjNum = gameData.multiplayer.players [new_pd.nPlayer].nObject;
if (nTheirPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if (!gameData.multigame.bQuitGame && (nTheirPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus!=NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if (networkData.bVerifyPlayerJoined!=-1 && nTheirPlayer == networkData.bVerifyPlayerJoined) {
	// Hurray! Someone really really got in the game (I think).
   networkData.bVerifyPlayerJoined=-1;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (new_pd.data_size > 0) {
		// pass pd->data to some parser function....
		MultiProcessBigData ((char *) new_pd.data, new_pd.data_size);
		}
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
if ((sbyte)new_pd.level_num != gameData.missions.nCurrentLevel) {
#if 1				
	con_printf (CONDBG, "Got frame packet from tPlayer %d wrong level %d!\n", new_pd.nPlayer, new_pd.level_num);
#endif
	return;
	}
theirObjP = &gameData.objs.objects [theirObjNum];
//------------- Keep track of missed packets -----------------
gameData.multiplayer.players [nTheirPlayer].nPacketsGot++;
networkData.nTotalPacketsGot++;
networkData.nLastPacketTime [nTheirPlayer] = TimerGetApproxSeconds ();
if  (new_pd.numpackets != gameData.multiplayer.players [nTheirPlayer].nPacketsGot)      {
	int nMissedPackets = new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
	if ((new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot)>0)
		networkData.nTotalMissedPackets += new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
#if 1				
	if (nMissedPackets > 0)       
		con_printf (CONDBG, 
			"Missed %d packets from tPlayer #%d (%d total)\n", 
			new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot, 
			nTheirPlayer, 
			nMissedPackets);
	else
		con_printf (CONDBG, 
			"Got %d late packets from tPlayer #%d (%d total)\n", 
			gameData.multiplayer.players [nTheirPlayer].nPacketsGot-new_pd.numpackets, 
			nTheirPlayer, 
			nMissedPackets);
#endif
		gameData.multiplayer.players [nTheirPlayer].nPacketsGot = new_pd.numpackets;
	}
//------------ Read the tPlayer's ship's tObject info ----------------------
ExtractShortPos (theirObjP, &new_pd.thepos, 0);
if ((theirObjP->renderType != new_pd.obj_renderType) && (new_pd.obj_renderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nTheirPlayer);
if (theirObjP->movementType == MT_PHYSICS)
	SetThrustFromVelocity (theirObjP);
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nTheirPlayer].connected) {
	gameData.multiplayer.players [nTheirPlayer].connected = 1;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nTheirPlayer);
	MultiMakeGhostPlayer (nTheirPlayer);
	CreatePlayerAppearanceEffect (gameData.objs.objects + theirObjNum);
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	ClipRank ((char *) &netPlayers.players [nTheirPlayer].rank);
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s", pszRankStrings [netPlayers.players [nTheirPlayer].rank], gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra data at the end ---------------
if (new_pd.data_size>0) {
	// pass pd->data to some parser function....
	MultiProcessBigData ((char *) new_pd.data, new_pd.data_size);
	}
}

//------------------------------------------------------------------------------

extern void MultiSendLight (int, ubyte);
extern void MultiSendLightSpecific (int, int, ubyte);

void NetworkSendSmashLights (int pnum) 
 {
  // send the lights that have been blown out
  int i;

for (i = 0;i <= gameData.segs.nLastSegment; i++)
   if (gameData.render.lights.subtracted [i])
		MultiSendLightSpecific (pnum, i, gameData.render.lights.subtracted [i]);
 }

//------------------------------------------------------------------------------

extern void MultiSendTriggerSpecific (char pnum, char);

void NetworkSendFlyThruTriggers (int pnum) 
 {
  // send the fly thru triggers that have been disabled
  int i;

for (i = 0; i < gameData.trigs.nTriggers; i++)
	if (gameData.trigs.triggers [i].flags & TF_DISABLED)
		MultiSendTriggerSpecific ((char) pnum, (ubyte) i);
 }

//------------------------------------------------------------------------------

void NetworkSendPlayerFlags ()
{
int i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	MultiSendFlags ((char) i);
 }

//------------------------------------------------------------------------------

void NetworkPing (ubyte flag, int pnum)
{
	ubyte mybuf [2];

mybuf [0]=flag;
mybuf [1]=gameData.multiplayer.nLocalPlayer;
if (gameStates.multi.nGameType >= IPX_GAME)
	IPXSendPacketData (
			(ubyte *)mybuf, 2, 
		netPlayers.players [pnum].network.ipx.server, 
		netPlayers.players [pnum].network.ipx.node, 
		gameData.multiplayer.players [pnum].netAddress);
}

//------------------------------------------------------------------------------

void NetworkHandlePingReturn (ubyte pnum)
{
if ((pnum >= gameData.multiplayer.nPlayers) || !pingStats [pnum].launchTime) {
#if 1				
	 con_printf (CONDBG, "Got invalid PING RETURN from %s!\n", gameData.multiplayer.players [pnum].callsign);
#endif
   return;
	}
xPingReturnTime = TimerGetFixedSeconds ();
pingStats [pnum].ping = f2i (FixMul (xPingReturnTime - pingStats [pnum].launchTime, i2f (1000)));
if (!gameStates.render.cockpit.bShowPingStats)
	HUDInitMessage ("Ping time for %s is %d ms!", gameData.multiplayer.players [pnum].callsign, pingStats [pnum].ping);
pingStats [pnum].launchTime = 0;
pingStats [pnum].received++;
}
	
//------------------------------------------------------------------------------

void NetworkSendPing (ubyte pnum)
{
NetworkPing (PID_PING_SEND, pnum);
}  

//------------------------------------------------------------------------------

void DoRefuseStuff (tSequencePacket *their)
 {
  int i, new_player_num;

ClipRank ((char *) &their->player.rank);

for (i = 0; i < MAX_PLAYERS; i++)
	if (!strcmp (their->player.callsign, gameData.multiplayer.players [i].callsign)) {
		NetworkWelcomePlayer (their);
		return;
		}
if (FindPlayerInBanList (their->player.callsign))
	return;
if (!bWaitForRefuseAnswer) {
	DigiPlaySample (SOUND_HUD_JOIN_REQUEST, F1_0*2);           
	if (gameData.app.nGameMode & GM_TEAM) {
		if (!gameOpts->multi.bNoRankings)
			HUDInitMessage (
				"%s %s wants to join", 
				pszRankStrings [their->player.rank], their->player.callsign);
		HUDInitMessage (
			"%s joining. Alt-1 assigns to team %s. Alt-2 to team %s", 
			their->player.callsign, netGame.team_name [0], netGame.team_name [1]);
//                      HUDInitMessage ("Alt-1 to place on team %s!", netGame.team_name [0]);
//                      HUDInitMessage ("Alt-2 to place on team %s!", netGame.team_name [1]);
		}               
	else    
		HUDInitMessage (TXT_JOIN_ACCEPT, their->player.callsign);
	strcpy (RefusePlayerName, their->player.callsign);
	RefuseTimeLimit=TimerGetApproxSeconds ();   
	bRefuseThisPlayer=0;
	bWaitForRefuseAnswer=1;
	}
else {      
	if (strcmp (their->player.callsign, RefusePlayerName))
		return;
	if (bRefuseThisPlayer) {
		RefuseTimeLimit=0;
		bRefuseThisPlayer=0;
		bWaitForRefuseAnswer=0;
		if (gameData.app.nGameMode & GM_TEAM) {
			new_player_num=GetNewPlayerNumber (their);
			Assert (bRefuseTeam == 1 || bRefuseTeam == 2);        
			if (bRefuseTeam == 1)      
				netGame.teamVector &= (~ (1<<new_player_num));
			else
				netGame.teamVector |= (1<<new_player_num);
			NetworkWelcomePlayer (their);
			NetworkSendNetgameUpdate (); 
			}
		else
			NetworkWelcomePlayer (their);
		return;
		}
	if ((TimerGetApproxSeconds ()) > RefuseTimeLimit+REFUSE_INTERVAL) {
		RefuseTimeLimit=0;
		bRefuseThisPlayer=0;
		bWaitForRefuseAnswer=0;
		if (!strcmp (their->player.callsign, RefusePlayerName)) {
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

int GetNewPlayerNumber (tSequencePacket *their)
  {
	 int i;
	
if (gameData.multiplayer.nPlayers < gameData.multiplayer.nMaxPlayers)
	return (gameData.multiplayer.nPlayers);
else {
	// Slots are full but game is open, see if anyone is
	// disconnected and replace the oldest tPlayer with this new one
	int oldest_player = -1;
	fix oldestTime = TimerGetApproxSeconds ();

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

void NetworkSendExtras ()
{
Assert (networkData.nPlayerJoiningExtras >- 1);
if (!NetworkIAmMaster ()) {
#if 1				
  con_printf (CONDBG, "Hey! I'm not the master and I was gonna send info!\n");
#endif
	}
if (networkData.bSendingExtras == 40)
	NetworkSendFlyThruTriggers (networkData.nPlayerJoiningExtras);
if (networkData.bSendingExtras == 38)
	NetworkSendDoorUpdates (networkData.nPlayerJoiningExtras);
if (networkData.bSendingExtras == 35)
	NetworkSendMarkers ();
if (networkData.bSendingExtras == 30 && (gameData.app.nGameMode & GM_MULTI_ROBOTS))
	MultiSendStolenItems ();
if (networkData.bSendingExtras == 25 && (netGame.xPlayTimeAllowed || netGame.KillGoal))
	MultiSendKillGoalCounts ();
if (networkData.bSendingExtras == 20)
	NetworkSendSmashLights (networkData.nPlayerJoiningExtras);
if (networkData.bSendingExtras == 15)
	NetworkSendPlayerFlags ();    
if (networkData.bSendingExtras == 10)
	MultiSendPowerupUpdate ();  
networkData.bSendingExtras--;
if (!networkData.bSendingExtras) {
	networkData.nPlayerJoiningExtras = -1;
	memset (&networkData.playerRejoining, 0, sizeof (networkData.playerRejoining));
	}
}

//------------------------------------------------------------------------------

void NetworkSendNakedPacket (char *buf, short len, int who)
{
if (!(gameData.app.nGameMode&GM_NETWORK)) 
	return;
if (NakedPacketLen == 0) {
	NakedBuf [0]=PID_NAKED_PDATA;
	NakedBuf [1]=gameData.multiplayer.nLocalPlayer;
	NakedPacketLen=2;
	}
if (len + NakedPacketLen>networkData.nMaxXDataSize) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		IPXSendPacketData (
			 (ubyte *)NakedBuf, 
			NakedPacketLen, 
			netPlayers.players [who].network.ipx.server, 
			netPlayers.players [who].network.ipx.node, gameData.multiplayer.players [who].netAddress);
	NakedPacketLen=2;
	memcpy (&NakedBuf [NakedPacketLen], buf, len);     
	NakedPacketLen+=len;
	NakedPacketDestPlayer=who;
	}
else {
	memcpy (&NakedBuf [NakedPacketLen], buf, len);     
	NakedPacketLen+=len;
	NakedPacketDestPlayer=who;
	}
 }

//------------------------------------------------------------------------------

void NetworkProcessNakedPData (char *data, int len)
 {
   int pnum=data [1]; 
   Assert (data [0]=PID_NAKED_PDATA);

if (pnum < 0) {
#if 1				
   con_printf (CONDBG, "Naked packet is bad!\n");
#endif
	Int3 (); // This packet is bogus!!
	return;
	}

if (!gameData.multigame.bQuitGame && (pnum >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus != NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	MultiProcessBigData ((char *) data+2, len-2);
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
MultiProcessBigData ((char *) data+2, len-2);
 }

//------------------------------------------------------------------------------

#ifdef NETPROFILING
void OpenSendLog ()
 {
  int i;

SendLogFile= (FILE *)fopen ("sendlog.net", "w");
for (i = 0; i < 100; i++)
	TTSent [i] = 0;
 }

 //------------------------------------------------------------------------------

void OpenReceiveLog ()
 {
int i;

ReceiveLogFile= (FILE *)fopen ("recvlog.net", "w");
for (i = 0; i < 100; i++)
	TTRecv [i] = 0;
 }
#endif

//------------------------------------------------------------------------------

int GetMyNetRanking ()
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
if ((netPlayers.players [(int) pnum].version_major == 1) && 
	 !(netPlayers.players [(int) pnum].version_minor & 0x0F))
	netPlayers.players [ (int) pnum].rank = 0;
}

//------------------------------------------------------------------------------

void NetworkRequestPlayerNames (int n)
{
NetworkSendAllInfoRequest (PID_GAME_PLAYERS, activeNetGames [n].nSecurity);
networkData.nNamesInfoSecurity=activeNetGames [n].nSecurity;
}

//------------------------------------------------------------------------------

extern int ExecMenutiny2 (char * title, char * subtitle, int nitems, tMenuItem * item, void (*subfunction) (int nitems, tMenuItem * items, int * last_key, int citem));

void NetworkProcessNamesReturn (char *data)
 {
	tMenuItem m [15];
   char mtext [15][50], temp [50];
	int i, l, nInMenu, gnum, num = 0, count = 5, nPlayers;
   
if (networkData.nNamesInfoSecurity != (*(int *) (data+1))) {
#if 1				
  con_printf (CONDBG, "Bad security on names return!\n");
  con_printf (CONDBG, "NIS=%d data=%d\n", networkData.nNamesInfoSecurity, (*(int *) (data+1)));
#endif
	return;
	}
nPlayers = data [count++]; 
if (nPlayers == 255) {
	gameStates.multi.bSurfingNet = 0;	
	networkData.nNamesInfoSecurity = -1;
	ExecMessageBox (NULL, NULL, 1, "OK", "That game is refusing\nname requests.\n");
	gameStates.multi.bSurfingNet=1;
	return;
	}
Assert ((nPlayers > 0) && (nPlayers < MAX_NUM_NET_PLAYERS));
memset (m, 0, sizeof (m));
for (i = 0; i < 12; i++) {
	m [i].text = (char *)(mtext + i);
	m [i].nType = NM_TYPE_TEXT;		
	}
#if SECURITY_CHECK
for (gnum = -1, i = 0; i < networkData.nActiveGames; i++) {
	if (networkData.nNamesInfoSecurity == activeNetGames [i].nSecurity) {
		gnum = i;
		break;
		}
	}
if (gnum == -1) {
	gameStates.multi.bSurfingNet = 0;
	networkData.nNamesInfoSecurity = -1;
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_GAME_GONE);
	gameStates.multi.bSurfingNet = 1;
	return;
	}
#else
gnum = 0;
#endif
sprintf (mtext [num], TXT_GAME_PLRS, activeNetGames [gnum].szGameName); 
num++;
for (i = 0; i < nPlayers; i++) {
	l = data [count++];
	memcpy (temp, data + count, CALLSIGN_LEN + 1);
	count += CALLSIGN_LEN + 1;
	if (gameOpts->multi.bNoRankings)	
		sprintf (mtext [num], "%s", temp);
	else
		sprintf (mtext [num], "%s%s", pszRankStrings [l], temp);
	num++;	
	}
if (data [count] == 99) {
	sprintf (mtext [num++], " ");
	sprintf (mtext [num++], TXT_SHORT_PACKETS2, data [count+1]? TXT_ON : TXT_OFF);
	sprintf (mtext [num++], TXT_PPS2, data [count+2]);
	}
bAlreadyShowingInfo = 1;	
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
ExecMenutiny2 (NULL, NULL, num, m, NULL);
gameStates.menus.nInMenu = nInMenu;
bAlreadyShowingInfo = 0;	
}

//------------------------------------------------------------------------------

char bNameReturning=1;

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
buf [count++] = (char) netGame.nPacketsPerSec;
 
sendit:	   

IPXSendInternetPacketData ((ubyte *)buf, count, 
									their->player.network.ipx.server, 
									their->player.network.ipx.node);
}

//------------------------------------------------------------------------------

int HoardEquipped ()
{
	static int checked=-1;

if (checked == -1) {
	if (CFExist ("hoard.ham", gameFolders.szDataDir, 0))
		checked=1;
	else
		checked=0;
	}
return (checked);
}

//-----------------------------------------------------------------------------

void ResetPingStats (void)
{
memset (pingStats, 0, sizeof (pingStats));
networkData.tLastPingStat = 0;
}

//------------------------------------------------------------------------------

void InitMonsterballSettings (tMonsterballInfo *monsterballP)
{
	tMonsterballForce *forceP = monsterballP->forces;

// primary weapons
forceP [0].nWeaponId = LASER_ID; 
forceP [0].nForce = 10;
forceP [1].nWeaponId = LASER_ID + 1; 
forceP [1].nForce = 15;
forceP [2].nWeaponId = LASER_ID + 2; 
forceP [2].nForce = 20;
forceP [3].nWeaponId = LASER_ID + 3; 
forceP [3].nForce = 25;
forceP [4].nWeaponId = SPREADFIRE_ID; 
forceP [4].nForce = 20;
forceP [5].nWeaponId = VULCAN_ID; 
forceP [5].nForce = 10;
forceP [6].nWeaponId = PLASMA_ID; 
forceP [6].nForce = 30;
forceP [7].nWeaponId = FUSION_ID; 
forceP [7].nForce = 100;
// primary "super" weapons
forceP [8].nWeaponId = SUPERLASER_ID; 
forceP [8].nForce = 50;
forceP [9].nWeaponId = SUPERLASER_ID + 1; 
forceP [9].nForce = 60;
forceP [10].nWeaponId = HELIX_ID; 
forceP [10].nForce = 40;
forceP [11].nWeaponId = GAUSS_ID; 
forceP [11].nForce = 30;
forceP [12].nWeaponId = PHOENIX_ID; 
forceP [12].nForce = 60;
forceP [13].nWeaponId = OMEGA_ID; 
forceP [13].nForce = 30;
forceP [14].nWeaponId = FLARE_ID; 
forceP [14].nForce = 5;
// missiles
forceP [15].nWeaponId = CONCUSSION_ID; 
forceP [15].nForce = 50;
forceP [16].nWeaponId = HOMINGMSL_ID; 
forceP [16].nForce = 50;
forceP [17].nWeaponId = SMARTMSL_ID; 
forceP [17].nForce = 80;
forceP [18].nWeaponId = MEGAMSL_ID; 
forceP [18].nForce = 150;
// "super" missiles
forceP [19].nWeaponId = FLASHMSL_ID; 
forceP [19].nForce = 30;
forceP [20].nWeaponId = GUIDEDMSL_ID; 
forceP [20].nForce = 40;
forceP [21].nWeaponId = MERCURYMSL_ID; 
forceP [21].nForce = 70;
forceP [22].nWeaponId = EARTHSHAKER_ID; 
forceP [22].nForce = 200;
forceP [23].nWeaponId = EARTHSHAKER_MEGA_ID; 
forceP [23].nForce = 150;
// tPlayer ships
forceP [24].nWeaponId = 255;
forceP [24].nForce = 4;
monsterballP->nBonus = 1;
monsterballP->nSizeMod = 7;	// that is actually shield orb size * 3, because it's divided by 2, thus allowing for half sizes
}

//------------------------------------------------------------------------------

void InitEntropySettings (int i)
{
extraGameInfo [i].entropy.nEnergyFillRate = 25;
extraGameInfo [i].entropy.nShieldFillRate = 11;
extraGameInfo [i].entropy.nShieldDamageRate = 11;
extraGameInfo [i].entropy.nMaxVirusCapacity = 0; 
extraGameInfo [i].entropy.nBumpVirusCapacity = 2;
extraGameInfo [i].entropy.nBashVirusCapacity = 1; 
extraGameInfo [i].entropy.nVirusGenTime = 2; 
extraGameInfo [i].entropy.nVirusLifespan = 0; 
extraGameInfo [i].entropy.nVirusStability = 0;
extraGameInfo [i].entropy.nCaptureVirusLimit = 1; 
extraGameInfo [i].entropy.nCaptureTimeLimit = 1; 
extraGameInfo [i].entropy.bRevertRooms = 0;
extraGameInfo [i].entropy.bDoConquerWarning = 0;
extraGameInfo [i].entropy.nOverrideTextures = 2;
extraGameInfo [i].entropy.bBrightenRooms = 0;
extraGameInfo [i].entropy.bPlayerHandicap = 0;
}

//------------------------------------------------------------------------------

void InitExtraGameInfo (void)
{
	int	i;

for (i = 0; i < 2; i++) {
	extraGameInfo [i].nType = 0;
	extraGameInfo [i].bFriendlyFire = 1;
	extraGameInfo [i].bInhibitSuicide = 0;
	extraGameInfo [i].bFixedRespawns = 0;
	extraGameInfo [i].nSpawnDelay = 0;
	extraGameInfo [i].bEnhancedCTF = 0;
	extraGameInfo [i].bRadarEnabled = i ? 0 : 1;
	extraGameInfo [i].bPowerupsOnRadar = 0;
	extraGameInfo [i].nZoomMode = 0;
	extraGameInfo [i].bRobotsHitRobots = 0;
	extraGameInfo [i].bAutoDownload = 1;
	extraGameInfo [i].bAutoBalanceTeams = 0;
	extraGameInfo [i].bDualMissileLaunch = 0;
	extraGameInfo [i].bRobotsOnRadar = 0;
	extraGameInfo [i].grWallTransparency = (GR_ACTUAL_FADE_LEVELS * 10 + 3) / 6;
	extraGameInfo [i].nSpeedBoost = 10;
	extraGameInfo [i].bDropAllMissiles = 1;
	extraGameInfo [i].bImmortalPowerups = 0;
	extraGameInfo [i].bUseCameras = 1;
	extraGameInfo [i].nFusionPowerMod = 2;
	extraGameInfo [i].bWiggle = 1;
	extraGameInfo [i].bMultiBosses = 0;
	extraGameInfo [i].nBossCount = 0;
	extraGameInfo [i].bSmartWeaponSwitch = 0;
	extraGameInfo [i].bFluidPhysics = 0;
	extraGameInfo [i].nWeaponDropMode = 1;
	extraGameInfo [i].bRotateLevels = 0;
	extraGameInfo [i].bDisableReactor = 0;
	extraGameInfo [i].bMouseLook = i ? 0 : 1;
	extraGameInfo [i].nWeaponIcons = 0;
	extraGameInfo [i].bSafeUDP = 0;
	extraGameInfo [i].bFastPitch = i ? 0 : 1;
	extraGameInfo [i].bUseSmoke = 1;
	extraGameInfo [i].bDamageExplosions = 1;
	extraGameInfo [i].bThrusterFlames = 1;
	extraGameInfo [i].bShadows = 1;
	extraGameInfo [i].bRenderShield = 1;
	extraGameInfo [i].bTeleporterCams = 0;
	extraGameInfo [i].bDarkness = 0;
	extraGameInfo [i].bTeamDoors = 0;
	extraGameInfo [i].bEnableCheats = 0;
	extraGameInfo [i].bTargetIndicators = 0;
	extraGameInfo [i].bDamageIndicators = 0;
	extraGameInfo [i].bFriendlyIndicators = 0;
	extraGameInfo [i].bCloakedIndicators = 0;
	extraGameInfo [i].bMslLockIndicators = 0;
	extraGameInfo [i].bTagOnlyHitObjs = 0;
	extraGameInfo [i].bTowFlags = 1;
	extraGameInfo [i].bUseHitAngles = 0;
	extraGameInfo [i].bLightTrails = 0;
	extraGameInfo [i].bTracers = 0;
	extraGameInfo [i].bShockwaves = 0;
	extraGameInfo [i].bCompetition = 0;
	extraGameInfo [i].bPowerupLights = i;
	extraGameInfo [i].bCheckUDPPort = 1;
	extraGameInfo [i].bSmokeGrenades = 0;
	extraGameInfo [i].nMaxSmokeGrenades = 2;
	extraGameInfo [i].nCoopPenalty = 0;
	extraGameInfo [i].bShootMissiles = 0;
	extraGameInfo [i].bTripleFusion = 0;
	extraGameInfo [i].bEnhancedShakers = 0;
	extraGameInfo [i].nHitboxes = 0;
	extraGameInfo [i].nRadar = 0;
	extraGameInfo [i].nDrag = 10;
	extraGameInfo [i].nSpotSize = 2 - i;
	extraGameInfo [i].nSpotStrength = 2 - i;
	extraGameInfo [i].nLightRange = 0;
	InitEntropySettings (i);
	InitMonsterballSettings (&extraGameInfo [i].monsterball);
	}
}

//------------------------------------------------------------------------------

int InitAutoNetGame (void)
{
if (!gameData.multiplayer.autoNG.bValid)
	return 0;
if (gameData.multiplayer.autoNG.bHost) {
	CFUseAltHogFile (gameData.multiplayer.autoNG.szFile);
	strcpy (szAutoMission, gameData.multiplayer.autoNG.szMission);
	gameStates.app.bAutoRunMission = gameHogFiles.AltHogFiles.bInitialized;
	strncpy (mpParams.szGameName, gameData.multiplayer.autoNG.szName, sizeof (mpParams.szGameName));
	mpParams.nLevel = gameData.multiplayer.autoNG.nLevel;
	extraGameInfo [0].bEnhancedCTF = 0;
	switch (gameData.multiplayer.autoNG.uType) {
		case 0:
			mpParams.nGameMode = gameData.multiplayer.autoNG.bTeam ? NETGAME_TEAM_ANARCHY : NETGAME_ANARCHY;
			break;
		case 1:
			mpParams.nGameMode = NETGAME_COOPERATIVE;
			break;
		case 2:
			mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
			break;
		case 3:
			mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
			extraGameInfo [0].bEnhancedCTF = 1;
			break;
		case 4:
			if (HoardEquipped ())
				mpParams.nGameMode = gameData.multiplayer.autoNG.bTeam ? NETGAME_TEAM_HOARD : NETGAME_HOARD;
			else
				mpParams.nGameMode = gameData.multiplayer.autoNG.bTeam ? NETGAME_TEAM_ANARCHY : NETGAME_ANARCHY;
			break;
		case 5:
			mpParams.nGameMode = NETGAME_ENTROPY;
			break;
		case 6:
			mpParams.nGameMode = NETGAME_MONSTERBALL;
			break;
			}
	mpParams.nGameType = mpParams.nGameMode;
	}
else {
	memcpy (ipx_ServerAddress + 4, gameData.multiplayer.autoNG.ipAddr, sizeof (gameData.multiplayer.autoNG.ipAddr));
	mpParams.udpClientPort = gameData.multiplayer.autoNG.nPort;
	}	
return 1;
}
	
//------------------------------------------------------------------------------

void LogExtraGameInfo (void)
{
if (!gameStates.app.bHaveExtraGameInfo [1])
	LogErr ("No extra game info data available\n");
else {
	LogErr ("extra game info data:\n");
	LogErr ("   bFriendlyFire: %d\n", extraGameInfo [1].bFriendlyFire);
	LogErr ("   bInhibitSuicide: %d\n", extraGameInfo [1].bInhibitSuicide);
	LogErr ("   bFixedRespawns: %d\n", extraGameInfo [1].bFixedRespawns);
	LogErr ("   nSpawnDelay: %d\n", extraGameInfo [1].nSpawnDelay);
	LogErr ("   bEnhancedCTF: %d\n", extraGameInfo [1].bEnhancedCTF);
	LogErr ("   bRadarEnabled: %d\n", extraGameInfo [1].bRadarEnabled);
	LogErr ("   bPowerupsOnRadar: %d\n", extraGameInfo [1].bPowerupsOnRadar);
	LogErr ("   nZoomMode: %d\n", extraGameInfo [1].nZoomMode);
	LogErr ("   bRobotsHitRobots: %d\n", extraGameInfo [1].bRobotsHitRobots);
	LogErr ("   bAutoDownload: %d\n", extraGameInfo [1].bAutoDownload);
	LogErr ("   bAutoBalanceTeams: %d\n", extraGameInfo [1].bAutoBalanceTeams);
	LogErr ("   bDualMissileLaunch: %d\n", extraGameInfo [1].bDualMissileLaunch);
	LogErr ("   bRobotsOnRadar: %d\n", extraGameInfo [1].bRobotsOnRadar);
	LogErr ("   grWallTransparency: %d\n", extraGameInfo [1].grWallTransparency);
	LogErr ("   nSpeedBoost: %d\n", extraGameInfo [1].nSpeedBoost);
	LogErr ("   bDropAllMissiles: %d\n", extraGameInfo [1].bDropAllMissiles);
	LogErr ("   bImmortalPowerups: %d\n", extraGameInfo [1].bImmortalPowerups);
	LogErr ("   bUseCameras: %d\n", extraGameInfo [1].bUseCameras);
	LogErr ("   nFusionPowerMod: %d\n", extraGameInfo [1].nFusionPowerMod);
	LogErr ("   bWiggle: %d\n", extraGameInfo [1].bWiggle);
	LogErr ("   bMultiBosses: %d\n", extraGameInfo [1].bMultiBosses);
	LogErr ("   nBossCount: %d\n", extraGameInfo [1].nBossCount);
	LogErr ("   bSmartWeaponSwitch: %d\n", extraGameInfo [1].bSmartWeaponSwitch);
	LogErr ("   bFluidPhysics: %d\n", extraGameInfo [1].bFluidPhysics);
	LogErr ("   nWeaponDropMode: %d\n", extraGameInfo [1].nWeaponDropMode);
	LogErr ("   bDarkness: %d\n", extraGameInfo [1].bDarkness);
	LogErr ("   bTeamDoors: %d\n", extraGameInfo [1].bTeamDoors);
	LogErr ("   bEnableCheats: %d\n", extraGameInfo [1].bEnableCheats);
	LogErr ("   bRotateLevels: %d\n", extraGameInfo [1].bRotateLevels);
	LogErr ("   bDisableReactor: %d\n", extraGameInfo [1].bDisableReactor);
	LogErr ("   bMouseLook: %d\n", extraGameInfo [1].bMouseLook);
	LogErr ("   nWeaponIcons: %d\n", extraGameInfo [1].nWeaponIcons);
	LogErr ("   bSafeUDP: %d\n", extraGameInfo [1].bSafeUDP);
	LogErr ("   bFastPitch: %d\n", extraGameInfo [1].bFastPitch);
	LogErr ("   bUseSmoke: %d\n", extraGameInfo [1].bUseSmoke);
	LogErr ("   bDamageExplosions: %d\n", extraGameInfo [1].bDamageExplosions);
	LogErr ("   bThrusterFlames: %d\n", extraGameInfo [1].bThrusterFlames);
	LogErr ("   bShadows: %d\n", extraGameInfo [1].bShadows);
	LogErr ("   bRenderShield: %d\n", extraGameInfo [1].bRenderShield);
	LogErr ("   bTeleporterCams: %d\n", extraGameInfo [1].bTeleporterCams);
	LogErr ("   bEnableCheats: %d\n", extraGameInfo [1].bEnableCheats);
	LogErr ("   bTargetIndicators: %d\n", extraGameInfo [1].bTargetIndicators);
	LogErr ("   bDamageIndicators: %d\n", extraGameInfo [1].bDamageIndicators);
	LogErr ("   bFriendlyIndicators: %d\n", extraGameInfo [1].bFriendlyIndicators);
	LogErr ("   bCloakedIndicators: %d\n", extraGameInfo [1].bCloakedIndicators);
	LogErr ("   bMslLockIndicators: %d\n", extraGameInfo [1].bMslLockIndicators);
	LogErr ("   bTagOnlyHitObjs: %d\n", extraGameInfo [1].bTagOnlyHitObjs);
	LogErr ("   bTowFlags: %d\n", extraGameInfo [1].bTowFlags);
	LogErr ("   bUseHitAngles: %d\n", extraGameInfo [1].bUseHitAngles);
	LogErr ("   bLightTrails: %d\n", extraGameInfo [1].bLightTrails);
	LogErr ("   bTracers: %d\n", extraGameInfo [1].bTracers);
	LogErr ("   bShockwaves: %d\n", extraGameInfo [1].bShockwaves);
	LogErr ("   bSmokeGrenades: %d\n", extraGameInfo [1].bSmokeGrenades);
	LogErr ("   nMaxSmokeGrenades: %d\n", extraGameInfo [1].nMaxSmokeGrenades);
	LogErr ("   bCompetition: %d\n", extraGameInfo [1].bCompetition);
	LogErr ("   bFlickerLights: %d\n", extraGameInfo [1].bFlickerLights);
	LogErr ("   bCheckUDPPort: %d\n", extraGameInfo [1].bCheckUDPPort);
	LogErr ("   bSmokeGrenades: %d\n", extraGameInfo [1].bSmokeGrenades);
	LogErr ("   nMslTurnSpeed: %d\n", extraGameInfo [1].nMslTurnSpeed);
	LogErr ("   nCoopPenalty: %d\n", nCoopPenalties [extraGameInfo [1].nCoopPenalty]);
	LogErr ("   bShootMissiles: %d\n", extraGameInfo [1].bShootMissiles);
	LogErr ("   bTripleFusion: %d\n", extraGameInfo [1].bTripleFusion);
	LogErr ("   bEnhancedShakers: %d\n", extraGameInfo [1].bEnhancedShakers);
	LogErr ("   nHitboxes: %d\n", extraGameInfo [1].nHitboxes);
	LogErr ("   nRadar: %d\n", extraGameInfo [1].nRadar);
	LogErr ("   nDrag: %d\n", extraGameInfo [1].nDrag);
	LogErr ("   nSpotSize: %d\n", extraGameInfo [1].nSpotSize);
	LogErr ("   nSpotStrength: %d\n", extraGameInfo [1].nSpotStrength);
	LogErr ("   nLightRange: %d\n", extraGameInfo [1].nLightRange);
	LogErr ("entropy info data:\n");
	LogErr ("   nEnergyFillRate: %d\n", extraGameInfo [1].entropy.nEnergyFillRate);
	LogErr ("   nShieldFillRate: %d\n", extraGameInfo [1].entropy.nShieldFillRate);
	LogErr ("   nShieldDamageRate: %d\n", extraGameInfo [1].entropy.nShieldDamageRate);
	LogErr ("   nMaxVirusCapacity: %d\n", extraGameInfo [1].entropy.nMaxVirusCapacity); 
	LogErr ("   nBumpVirusCapacity: %d\n", extraGameInfo [1].entropy.nBumpVirusCapacity);
	LogErr ("   nBashVirusCapacity: %d\n", extraGameInfo [1].entropy.nBashVirusCapacity); 
	LogErr ("   nVirusGenTime: %d\n", extraGameInfo [1].entropy.nVirusGenTime); 
	LogErr ("   nVirusLifespan: %d\n", extraGameInfo [1].entropy.nVirusLifespan); 
	LogErr ("   nVirusStability: %d\n", extraGameInfo [1].entropy.nVirusStability);
	LogErr ("   nCaptureVirusLimit: %d\n", extraGameInfo [1].entropy.nCaptureVirusLimit); 
	LogErr ("   nCaptureTimeLimit: %d\n", extraGameInfo [1].entropy.nCaptureTimeLimit); 
	LogErr ("   bRevertRooms: %d\n", extraGameInfo [1].entropy.bRevertRooms);
	LogErr ("   bDoConquerWarning: %d\n", extraGameInfo [1].entropy.bDoConquerWarning);
	LogErr ("   nOverrideTextures: %d\n", extraGameInfo [1].entropy.nOverrideTextures);
	LogErr ("   bBrightenRooms: %d\n", extraGameInfo [1].entropy.bBrightenRooms);
	LogErr ("   bPlayerHandicap: %d\n", extraGameInfo [1].entropy.bPlayerHandicap);
	}
}

//------------------------------------------------------------------------------

