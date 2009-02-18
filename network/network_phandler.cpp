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
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "autodl.h"
#include "tracker.h"
#include "banlist.h"

//------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int ( __fastcall * pPacketHandler) (ubyte *dataP, int nLength);
#else
typedef int (* pPacketHandler) (ubyte *dataP, int nLength);
#endif

typedef struct tPacketHandlerInfo {
	pPacketHandler	packetHandler;
	const char				*pszInfo;
	int				nLength;
	short				nStatusFilter;
	} tPacketHandlerInfo;

static tPacketHandlerInfo packetHandlers [256];

static ubyte addressFilter [256];

void InitPacketHandlers (void);

//------------------------------------------------------------------------------
// Set a flag for packet types that do not carry a sender IP address
// All other packets need to be patched with the IP address retrieved when
// receiving the packet from the network adapter

void InitAddressFilter (void)
{
memset (addressFilter, 0, sizeof (addressFilter));
if (gameStates.multi.nGameType == UDP_GAME)
	 addressFilter [PID_LITE_INFO] =
	 addressFilter [PID_ADDPLAYER] =
	 addressFilter [PID_ENDLEVEL] = 
	 addressFilter [PID_ENDLEVEL_SHORT] =
	 addressFilter [PID_GAME_INFO] =
	 addressFilter [PID_NAKED_PDATA] =
	 addressFilter [PID_NAMES_RETURN] =
	 addressFilter [PID_OBJECT_DATA] =
	 addressFilter [PID_PDATA] =
	 addressFilter [PID_EXTRA_GAMEINFO] =
	 addressFilter [PID_DOWNLOAD] =
	 addressFilter [PID_UPLOAD] =
	 addressFilter [(int) PID_TRACKER_ADD_SERVER] =
	 addressFilter [(int) PID_TRACKER_GET_SERVERLIST] = 1;
}

//------------------------------------------------------------------------------

int NetworkBadPacketSize (int nLength, int nExpectedLength, const char *pszId)
{
if (!nExpectedLength || (nLength == nExpectedLength))
	return 0;
console.printf (CON_DBG, "WARNING! Received invalid size for %s\n", pszId);
PrintLog ("Networking: Bad size for %s\n", pszId);
#if DBG
HUDMessage (0, "invalid %s", pszId);
#endif
if (nLength == nExpectedLength - 4)
	PrintLog ("   (Probably due to mismatched UDP/IP connection quality improvement settings)\n");
return 1;
}

//------------------------------------------------------------------------------

int NetworkBadSecurity (int nSecurity, const char *pszId)
{
#if SECURITY_CHECK
if (nSecurity == netGame.nSecurity)
#endif
	return 0;
console.printf (CON_DBG, "Bad security for %s\n", pszId);
PrintLog ("Networking: Bad security for %s\n", pszId);
return 1;
}

//------------------------------------------------------------------------------

#define THEIR	reinterpret_cast<tSequencePacket*>(dataP)

int GameInfoHandler (ubyte *dataP, int nLength)
{
return 1;
}

//-------------------------------------------

int PlayersInfoHandler (ubyte *dataP, int nLength)
{
if (gameStates.multi.nGameType >= IPX_GAME)
	ReceiveNetPlayersPacket (dataP, &tmpPlayersBase);
else
	memcpy (&tmpPlayersBase, dataP, sizeof (tAllNetPlayersInfo));
if (NetworkBadSecurity (tmpPlayersBase.nSecurity, "PID_PLAYERSINFO"))
	return 0;
tmpPlayersInfo = &tmpPlayersBase;
networkData.bWaitingForPlayerInfo = 0;
networkData.nSecurityNum = tmpPlayersInfo->nSecurity;
networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_SYNC;
return 1;
}

//-------------------------------------------

int LiteInfoHandler (ubyte *dataP, int nLength)
{
NetworkProcessLiteInfo (dataP);
return 1;
}

//-------------------------------------------

int GameListHandler (ubyte *dataP, int nLength)
{
if (banList.Find (THEIR->player.callsign))
	return 0;
if (!NetworkIAmMaster ())
	return 0;
NetworkSendLiteInfo (THEIR);
return 1;
}

//-------------------------------------------

int AllGameInfoHandler (ubyte *dataP, int nLength)
{
if (NetworkBadSecurity (THEIR->nSecurity, "PID_SEND_ALL_GAMEINFO"))
	return 0;
if (!NetworkIAmMaster ())
	return 0;
NetworkSendGameInfo (THEIR);
return 1;
}

//-------------------------------------------

int AddPlayerHandler (ubyte *dataP, int nLength)
{
NetworkNewPlayer (THEIR);
return 1;
}

//-------------------------------------------

int RequestHandler (ubyte *dataP, int nLength)
{
if (banList.Find (THEIR->player.callsign))
	return 0;
if (networkData.nStatus == NETSTAT_STARTING) // Someone wants to join our game!
	NetworkAddPlayer (THEIR);	
else if (networkData.nStatus == NETSTAT_WAITING)	// Someone is ready to receive a sync packet
	NetworkProcessRequest (THEIR);	
else if (networkData.nStatus == NETSTAT_PLAYING) {		// Someone wants to join a game in progress!
	if (netGame.bRefusePlayers)
		DoRefuseStuff (THEIR);
	else 
		NetworkWelcomePlayer (THEIR);
	}
return 1;
}

//-------------------------------------------

int DumpHandler (ubyte *dataP, int nLength)
{
NetworkProcessDump (THEIR);
return 1;
}

//-------------------------------------------

int QuitJoiningHandler (ubyte *dataP, int nLength)
{
if (networkData.nStatus == NETSTAT_STARTING)
	NetworkRemovePlayer (THEIR);
else if (networkData.nStatus == NETSTAT_PLAYING) 
	NetworkStopResync (THEIR);
return 1;
}

//-------------------------------------------

int SyncHandler (ubyte *dataP, int nLength)
{
if (gameStates.multi.nGameType >= IPX_GAME)
	ReceiveFullNetGamePacket (dataP, &tempNetInfo);
else
	tempNetInfo = *reinterpret_cast<tNetgameInfo*> (dataP);
if (NetworkBadSecurity (tempNetInfo.nSecurity, "PID_SYNC"))
	return 0;
if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_SYNC) {
#if SECURITY_CHECK
	if (tempNetInfo.nSecurity == tmpPlayersInfo->nSecurity) {
#endif
		NetworkReadSyncPacket (&tempNetInfo, 0);
		networkData.nSecurityFlag = 0;
		networkData.nSecurityNum = 0;
#if SECURITY_CHECK
		}
#endif
	}
else {
	networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_PLAYERS;
	networkData.nSecurityNum = tempNetInfo.nSecurity;
	if (NetworkWaitForPlayerInfo ())
		NetworkReadSyncPacket (reinterpret_cast<tNetgameInfo*> (dataP), 0);
	networkData.nSecurityFlag = 0;
	networkData.nSecurityNum = 0;
	}
return 1;
}

//-------------------------------------------

int ExtraGameInfoHandler (ubyte *dataP, int nLength)
{
if (gameStates.multi.nGameType >= IPX_GAME)
	NetworkProcessExtraGameInfo (dataP);
return 1;
}

//-------------------------------------------

int UploadHandler (ubyte *dataP, int nLength)
{
if (NetworkIAmMaster ())
	downloadManager.Upload (dataP);
return 1;
}

//-------------------------------------------

int DownloadHandler (ubyte *dataP, int nLength)
{
if (extraGameInfo [0].bAutoDownload) 
	downloadManager.Download (dataP);
return 1;
}

//-------------------------------------------

int TrackerHandler (ubyte *dataP, int nLength)
{
tracker.ReceiveServerList (dataP);
return 1;
}

//-------------------------------------------

int PDataHandler (ubyte *dataP, int nLength)
{
if (IsNetworkGame)
	NetworkProcessPData (reinterpret_cast<char*> (dataP));
return 1;
}

//-------------------------------------------

int NakedPDataHandler (ubyte *dataP, int nLength)
{
if (IsNetworkGame)
	NetworkProcessNakedPData (reinterpret_cast<char*> (dataP), nLength);
return 1;
}

//-------------------------------------------

int ObjectDataHandler (ubyte *dataP, int nLength)
{
NetworkReadObjectPacket (dataP);
return 1;
}

//-------------------------------------------

int EndLevelHandler (ubyte *dataP, int nLength)
{
NetworkReadEndLevelPacket (dataP);
return 1;
}

//-------------------------------------------

int EndLevelShortHandler (ubyte *dataP, int nLength)
{
NetworkReadEndLevelShortPacket (dataP);
return 1;
}

//-------------------------------------------

int GameUpdateHandler (ubyte *dataP, int nLength)
{
if (NetworkBadSecurity (reinterpret_cast<tNetgameInfo*> (dataP)->nSecurity, "PID_GAME_UPDATE"))
	return 0;
if (networkData.nStatus == NETSTAT_PLAYING) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		ReceiveLiteNetGamePacket (dataP, &netGame);
	else
		memcpy (&netGame, dataP, sizeof (tLiteInfo));
	}
if (IsTeamGame) {
	int i;

	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		if (gameData.multiplayer.players [i].connected)
		   MultiResetObjectTexture (OBJECTS + gameData.multiplayer.players [i].nObject);
	}
return 1;
}

//-------------------------------------------
   
int PingSendHandler (ubyte *dataP, int nLength)
{
NetworkPing (PID_PING_RETURN, dataP [1]);
return 1;
}

//-------------------------------------------

int PingReturnHandler (ubyte *dataP, int nLength)
{
NetworkHandlePingReturn (dataP [1]);  // dataP [1] is CPlayerData who told us of THEIR ping time
return 1;
}

//-------------------------------------------

int NamesReturnHandler (ubyte *dataP, int nLength)
{
if (networkData.nNamesInfoSecurity != -1)
	NetworkProcessNamesReturn (reinterpret_cast<char*> (dataP));
return 1;
}

//-------------------------------------------

int GamePlayersHandler (ubyte *dataP, int nLength)
{
if (NetworkIAmMaster () && 
	 !NetworkBadSecurity (THEIR->nSecurity, "PID_GAME_PLAYERS"))
	NetworkSendPlayerNames (THEIR);
return 1;
}

//-------------------------------------------

int MissingObjFramesHandler (ubyte *dataP, int nLength)
{
NetworkProcessMissingObjFrames (reinterpret_cast<char*> (dataP));
return 1;
}

//------------------------------------------------------------------------------

void InitPacketHandler (ubyte pId, pPacketHandler packetHandler, const char *pszInfo, int nLength, short nStatusFilter)
{
	tPacketHandlerInfo	*piP = packetHandlers + pId;

piP->packetHandler = packetHandler;
piP->pszInfo = pszInfo;
piP->nLength = nLength;
piP->nStatusFilter = nStatusFilter;
}

//------------------------------------------------------------------------------

#define	PHINIT(_pId, _handler, _nLength, _nStatusFilter) \
			InitPacketHandler (_pId, _handler, #_pId, _nLength, _nStatusFilter)

void InitPacketHandlers (void)
{
PHINIT (PID_GAME_INFO, GameInfoHandler, 0, (short) 0xFFFF);
PHINIT (PID_PLAYERSINFO, PlayersInfoHandler, ALLNETPLAYERSINFO_SIZE, 1 << NETSTAT_WAITING);
PHINIT (PID_LITE_INFO, LiteInfoHandler, LITE_INFO_SIZE, 1 << NETSTAT_BROWSING);
PHINIT (PID_GAME_LIST, GameListHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_SEND_ALL_GAMEINFO, AllGameInfoHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_ADDPLAYER, AddPlayerHandler, SEQUENCE_PACKET_SIZE, (short) 0xFFFF);
PHINIT (PID_REQUEST, RequestHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_STARTING) | (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_DUMP, DumpHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_QUIT_JOINING, QuitJoiningHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_STARTING) | (1 << NETSTAT_PLAYING));
PHINIT (PID_SYNC, SyncHandler, 0, 1 << NETSTAT_WAITING);
PHINIT (PID_EXTRA_GAMEINFO, ExtraGameInfoHandler, 0, (short) 0xFFFF);
PHINIT (PID_UPLOAD, UploadHandler, 0, (1 << NETSTAT_STARTING) | (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_DOWNLOAD, DownloadHandler, 0, (short) 0xFFFF);
PHINIT (PID_TRACKER_GET_SERVERLIST, TrackerHandler, 0, (short) 0xFFFF);
PHINIT (PID_TRACKER_ADD_SERVER, TrackerHandler, 0, (short) 0xFFFF);
PHINIT (PID_PDATA, PDataHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_NAKED_PDATA, NakedPDataHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_OBJECT_DATA, ObjectDataHandler, 0, 1 << NETSTAT_WAITING);
PHINIT (PID_ENDLEVEL, EndLevelHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_ENDLEVEL_SHORT, EndLevelShortHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_GAME_UPDATE, GameUpdateHandler, 0, (short) 0xFFFF);
PHINIT (PID_PING_SEND, PingSendHandler, 0, (short) 0xFFFF);
PHINIT (PID_PING_RETURN, PingReturnHandler, 0, (short) 0xFFFF);
PHINIT (PID_NAMES_RETURN, NamesReturnHandler, 0, 1 << NETSTAT_BROWSING);
PHINIT (PID_GAME_PLAYERS, GamePlayersHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_MISSING_OBJ_FRAMES, MissingObjFramesHandler, 0, (1 << NETSTAT_WAITING) | (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
}

//------------------------------------------------------------------------------

int NetworkProcessPacket (ubyte *dataP, int nLength)
{
	ubyte						pId = dataP [0];
	tPacketHandlerInfo	*piP = packetHandlers + pId;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tSequencePacket tmpPacket;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveSequencePacket (dataP, &tmpPacket);
	dataP = reinterpret_cast<ubyte*>(&tmpPacket); // reassign THEIR to point to correctly alinged structure
	}
#endif

if (!piP->packetHandler)
	PrintLog ("invalid packet id %d\n", pId);
else if (!(piP->nStatusFilter & (1 << networkData.nStatus)))
	PrintLog ("invalid status %d for packet id %d\n", networkData.nStatus, pId);
else if (!NetworkBadPacketSize (nLength, piP->nLength, piP->pszInfo)) {
	console.printf (0, "received %s\n", piP->pszInfo);
	if (!addressFilter [pId])
		memcpy (&THEIR->player.network.ipx.server, &ipx_udpSrc.src_network, 10);
	return piP->packetHandler (dataP, nLength);
	}
return 0;
}

//------------------------------------------------------------------------------

