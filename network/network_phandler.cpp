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
#include "timeout.h"
#include "console.h"

//------------------------------------------------------------------------------

#if defined(_WIN32) && !DBG
typedef int32_t ( __fastcall * pPacketHandler) (uint8_t* data, int32_t nLength);
#else
typedef int32_t (* pPacketHandler) (uint8_t* data, int32_t nLength);
#endif

typedef struct tPacketHandlerInfo {
	pPacketHandler	packetHandler;
	const char*		pszInfo;
	int32_t				nLength;
	int16_t				nStatusFilter;
	} tPacketHandlerInfo;

static tPacketHandlerInfo packetHandlers [256];

static uint8_t addressFilter [256];

void InitPacketHandlers (void);

//------------------------------------------------------------------------------
// Set a flag for packet types that do not carry a sender IP address
// All other packets need to be patched with the IP address retrieved when
// receiving the packet from the network adapter

void InitAddressFilter (void)
{
memset (addressFilter, 0, sizeof (addressFilter));
//if (gameStates.multi.nGameType == UDP_GAME)
	 addressFilter [PID_LITE_INFO] =
	 addressFilter [PID_ADDPLAYER] =
	 addressFilter [PID_ENDLEVEL] = 
	 addressFilter [PID_ENDLEVEL_SHORT] =
	 addressFilter [PID_GAME_INFO] =
	 addressFilter [PID_MINE_DATA] =
	 addressFilter [PID_NAMES_RETURN] =
	 addressFilter [PID_OBJECT_DATA] =
	 addressFilter [PID_PLAYER_DATA] =
	 addressFilter [PID_PLAYERSINFO] =
	 addressFilter [PID_EXTRA_GAMEINFO] =
	 addressFilter [PID_DOWNLOAD] =
	 addressFilter [PID_UPLOAD] =
	 addressFilter [PID_XML_GAMEINFO] =
	 addressFilter [PID_RESEND_MESSAGE] =
	 addressFilter [PID_CONFIRM_MESSAGE] =
	 addressFilter [(int32_t) PID_TRACKER_ADD_SERVER] =
	 addressFilter [(int32_t) PID_TRACKER_GET_SERVERLIST] = 1;
}

//------------------------------------------------------------------------------
// Check whether the packet has the correct size. Works for packets with
// several messages packet together.

static int32_t NetworkBadCombinedPacketSize (uint8_t* data, int32_t nLength)
{
	int32_t	i = 0;

for (;;) {
	uint8_t pId = data [i];
	tPacketHandlerInfo* piP = packetHandlers + pId;
	if (!piP->packetHandler) {
		PrintLog (0, "invalid packet id %d\n", pId);
		return 0;
		}
	nLength -= piP->nLength;
	if (nLength == 0)
		return 1;
	if (nLength < 0) {
		PrintLog (0, "invalid packet size for packet type %d\n", pId);
		return 0;
		}
	i += piP->nLength;
	}
}

//------------------------------------------------------------------------------

int32_t NetworkBadPacketSize (int32_t nLength, int32_t nExpectedLength, const char *pszId)
{
if (!nExpectedLength || (nLength == nExpectedLength))
	return 0;
console.printf (CON_DBG, "WARNING! Received invalid size for %s\n", pszId);
PrintLog (0, "Networking: Bad size for %s\n", pszId);
#if DBG
HUDMessage (0, "invalid %s", pszId);
#endif
if (nLength == nExpectedLength - 4)
	PrintLog (0, "(Probably due to mismatched UDP/IP connection quality improvement settings)\n");
return 1;
}

//------------------------------------------------------------------------------

int32_t NetworkBadSecurity (int32_t nSecurity, const char *pszId)
{
#if SECURITY_CHECK
if (nSecurity == netGameInfo.m_info.nSecurity)
#endif
	return 0;
console.printf (CON_DBG, "Bad security for %s\n", pszId);
PrintLog (0, "Networking: Bad security id for %s\n", pszId);
return 1;
}

//------------------------------------------------------------------------------

int32_t IgnoreDataHandler (uint8_t* data, int32_t nLength)
{
return 1;
}

//------------------------------------------------------------------------------

int32_t GameInfoHandler (uint8_t* data, int32_t nLength)
{
return 1;
}

//------------------------------------------------------------------------------

int32_t XMLGameInfoHandler (uint8_t* data, int32_t nLength)
{
	static CTimeout to (200);

if (data && (networkData.xmlGameInfoRequestTime <= 0)) {
	if (to.Expired () && !strcmp ((char*) data + 1, "Descent Game Info Request")) {
		networkData.xmlGameInfoRequestTime = SDL_GetTicks ();
		for (int32_t i = 0; i < N_PLAYERS; i++) {
			if (i == N_LOCALPLAYER)
				pingStats [i].ping = 0;
			else {
				pingStats [i].ping = -1;
				pingStats [i].launchTime = -networkData.xmlGameInfoRequestTime; // negative value suppresses display of returned ping on HUD
				NetworkSendPing (i);
				}
			}
		}
	}
else if (networkData.xmlGameInfoRequestTime > 0) {
	// check whether all players have returned a ping response
	int32_t i;
	for (i = 0; i < N_PLAYERS; i++)
		if (pingStats [i].ping < 0)
			break;
	// send XML game status when all players have returned a ping response or 1 second has passed since the XML game status request has arrived
	if ((i == N_PLAYERS) || (SDL_GetTicks () - networkData.xmlGameInfoRequestTime > 1000)) { 
		NetworkSendXMLGameInfo ();
		networkData.xmlGameInfoRequestTime = -1;
		to.Start ();
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t PlayersInfoHandler (uint8_t* data, int32_t nLength)
{
if (gameStates.multi.nGameType >= IPX_GAME)
	ReceiveNetPlayersPacket (data, &netPlayers [1]);
else
	memcpy (&netPlayers [1].m_info, data, netPlayers [1].Size ());
if (NetworkBadSecurity (netPlayers [1].m_info.nSecurity, "PID_PLAYERSINFO"))
	return 0;
playerInfoP = &netPlayers [1];
networkData.bWaitingForPlayerInfo = 0;
networkData.nSecurityNum = playerInfoP->m_info.nSecurity;
networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_SYNC;
return 1;
}

//------------------------------------------------------------------------------

int32_t LiteInfoHandler (uint8_t* data, int32_t nLength)
{
NetworkProcessLiteInfo (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t GameListHandler (uint8_t* data, int32_t nLength)
{
if (banList.Find (reinterpret_cast<tPlayerSyncData*>(data)->player.callsign))
	return 0;
if (!IAmGameHost ())
	return 0;
NetworkSendLiteInfo (reinterpret_cast<tPlayerSyncData*>(data));
return 1;
}

//------------------------------------------------------------------------------

int32_t AllGameInfoHandler (uint8_t* data, int32_t nLength)
{
if (NetworkBadSecurity (reinterpret_cast<tPlayerSyncData*>(data)->nSecurity, "PID_SEND_ALL_GAMEINFO"))
	return 0;
if (!IAmGameHost ())
	return 0;
NetworkSendGameInfo (reinterpret_cast<tPlayerSyncData*>(data));
return 1;
}

//------------------------------------------------------------------------------

int32_t AddPlayerHandler (uint8_t* data, int32_t nLength)
{
NetworkNewPlayer (reinterpret_cast<tPlayerSyncData*>(data));
return 1;
}

//------------------------------------------------------------------------------

int32_t RequestHandler (uint8_t* data, int32_t nLength)
{
if (banList.Find (reinterpret_cast<tPlayerSyncData*>(data)->player.callsign))
	return 0;
if (networkData.nStatus == NETSTAT_STARTING) // Someone wants to join our game!
	NetworkAddPlayer (reinterpret_cast<tPlayerSyncData*>(data));	
else if (networkData.nStatus == NETSTAT_WAITING)	// Someone is ready to receive a sync packet
	NetworkProcessRequest (reinterpret_cast<tPlayerSyncData*>(data));	
else if (networkData.nStatus == NETSTAT_PLAYING) {		// Someone wants to join a game in progress!
	if (netGameInfo.m_info.bRefusePlayers)
		DoRefuseStuff (reinterpret_cast<tPlayerSyncData*>(data));
	else 
		NetworkWelcomePlayer (reinterpret_cast<tPlayerSyncData*>(data));
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t DumpHandler (uint8_t* data, int32_t nLength)
{
NetworkProcessDump (reinterpret_cast<tPlayerSyncData*>(data));
return 1;
}

//------------------------------------------------------------------------------

int32_t QuitJoiningHandler (uint8_t* data, int32_t nLength)
{
if (networkData.nStatus == NETSTAT_STARTING)
	NetworkRemovePlayer (reinterpret_cast<tPlayerSyncData*>(data));
else if (networkData.nStatus == NETSTAT_PLAYING) 
	NetworkStopResync (reinterpret_cast<tPlayerSyncData*>(data));
return 1;
}

//------------------------------------------------------------------------------

int32_t SyncHandler (uint8_t* data, int32_t nLength)
{
if (gameStates.multi.nGameType >= IPX_GAME)
	ReceiveFullNetGamePacket (data, &tempNetInfo);
else
	tempNetInfo = *reinterpret_cast<tNetGameInfo*> (data);
if (NetworkBadSecurity (tempNetInfo.m_info.nSecurity, "PID_SYNC"))
	return 0;
if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_SYNC) {
#if SECURITY_CHECK
	if (tempNetInfo.m_info.nSecurity == playerInfoP->m_info.nSecurity) {
#endif
		NetworkProcessSyncPacket (&tempNetInfo, 0);
		networkData.nSecurityFlag = 0;
		networkData.nSecurityNum = 0;
#if SECURITY_CHECK
		}
#endif
	}
else {
	networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_PLAYERS;
	networkData.nSecurityNum = tempNetInfo.m_info.nSecurity;
	if (NetworkWaitForPlayerInfo ())
		NetworkProcessSyncPacket (&tempNetInfo, 0);
	networkData.nSecurityFlag = 0;
	networkData.nSecurityNum = 0;
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t ExtraGameInfoHandler (uint8_t* data, int32_t nLength)
{
if (gameStates.multi.nGameType >= IPX_GAME)
	return NetworkProcessExtraGameInfo (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t UploadHandler (uint8_t* data, int32_t nLength)
{
if (IAmGameHost ())
	downloadManager.InitUpload (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t DownloadHandler (uint8_t* data, int32_t nLength)
{
if (extraGameInfo [0].bAutoDownload) 
	downloadManager.InitDownload (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t TrackerHandler (uint8_t* data, int32_t nLength)
{
tracker.ReceiveServerList (data);
return 1;
}

//------------------------------------------------------------------------------

int MultiProcessData (uint8_t* buf, int32_t len);

int32_t ResentMessageHandler (uint8_t* data, int32_t nLength)
{
MultiProcessData (data + 1, nLength - 1);
return 1;
}

//------------------------------------------------------------------------------

void MultiDoConfirmMessage (uint8_t* buf);

int32_t ConfirmMessageHandler (uint8_t* data, int32_t nLength)
{
MultiDoConfirmMessage (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t PlayerDataHandler (uint8_t* data, int32_t nLength)
{
if (IsNetworkGame)
	NetworkProcessPlayerData (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t MineDataHandler (uint8_t* data, int32_t nLength)
{
if (IsNetworkGame)
	NetworkProcessMineData (data, nLength);
return 1;
}

//------------------------------------------------------------------------------

int32_t ObjectDataHandler (uint8_t* data, int32_t nLength)
{
NetworkReadObjectPacket (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t EndLevelHandler (uint8_t* data, int32_t nLength)
{
NetworkReadEndLevelPacket (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t EndLevelShortHandler (uint8_t* data, int32_t nLength)
{
NetworkReadEndLevelShortPacket (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t GameUpdateHandler (uint8_t* data, int32_t nLength)
{
if (NetworkBadSecurity (reinterpret_cast<tNetGameInfo*> (data)->nSecurity, "PID_GAME_UPDATE"))
	return 0;
if (networkData.nStatus == NETSTAT_PLAYING) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		ReceiveLiteNetGamePacket (data, &netGameInfo);
	else
		memcpy (&netGameInfo, data, sizeof (tNetGameInfoLite));
	}
if (IsTeamGame) {
	for (int32_t i = 0; i < N_PLAYERS; i++)
		if (PLAYER (i).IsConnected ())
		   MultiSetObjectTextures (OBJECTS + PLAYER (i).nObject);
	}
return 1;
}

//------------------------------------------------------------------------------
   
int32_t PingSendHandler (uint8_t* data, int32_t nLength)
{
//CONNECT ((int32_t) data [1], (gameStates.multi.nGameType == UDP_GAME) ? data [2] : CONNECT_PLAYING);
NetworkPing (PID_PING_RETURN, data [1]);
ResetPlayerTimeout ((int32_t) data [1], -1);
return 1;
}

//------------------------------------------------------------------------------

int32_t PingReturnHandler (uint8_t* data, int32_t nLength)
{
//CONNECT ((int32_t) data [1], (gameStates.multi.nGameType == UDP_GAME) ? data [2] : CONNECT_PLAYING);
NetworkHandlePingReturn (data [1]);  // data [1] is CPlayerData who told us of reinterpret_cast<tPlayerSyncData*>(data) ping time
ResetPlayerTimeout ((int32_t) data [1], -1);
return 1;
}

//------------------------------------------------------------------------------

int32_t NamesReturnHandler (uint8_t* data, int32_t nLength)
{
if (networkData.nNamesInfoSecurity != -1)
	NetworkProcessNamesReturn (data);
return 1;
}

//------------------------------------------------------------------------------

int32_t GamePlayersHandler (uint8_t* data, int32_t nLength)
{
if (IAmGameHost () && 
	 !NetworkBadSecurity (reinterpret_cast<tPlayerSyncData*>(data)->nSecurity, "PID_GAME_PLAYERS"))
	NetworkSendPlayerNames (reinterpret_cast<tPlayerSyncData*>(data));
return 1;
}

//-----------------------------------------------------------------------------------------------------------------

void InitPacketHandler (uint8_t pId, pPacketHandler packetHandler, const char *pszInfo, int32_t nLength, int16_t nStatusFilter)
{
	tPacketHandlerInfo	*piP = packetHandlers + pId;

piP->packetHandler = packetHandler;
piP->pszInfo = pszInfo;
piP->nLength = nLength;
piP->nStatusFilter = nStatusFilter;
}

//-----------------------------------------------------------------------------------------------------------------

#define	PHINIT(_pId, _handler, _nLength, _nStatusFilter) \
			InitPacketHandler (_pId, _handler, #_pId, _nLength, _nStatusFilter)

void InitPacketHandlers (void)
{
PHINIT (PID_LITE_INFO, LiteInfoHandler, LITE_INFO_SIZE, 1 << NETSTAT_BROWSING);
PHINIT (PID_SEND_ALL_GAMEINFO, AllGameInfoHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_PLAYERSINFO, PlayersInfoHandler, ALLNETPLAYERSINFO_SIZE, (1 << NETSTAT_WAITING) | (1 << NETSTAT_PLAYING));
PHINIT (PID_REQUEST, RequestHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_STARTING) | (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_SYNC, SyncHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_PLAYER_DATA, PlayerDataHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_ADDPLAYER, AddPlayerHandler, SEQUENCE_PACKET_SIZE, (int16_t) 0xFFFF);
PHINIT (PID_DUMP, DumpHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_ENDLEVEL, EndLevelHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_QUIT_JOINING, QuitJoiningHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_STARTING) | (1 << NETSTAT_PLAYING));
PHINIT (PID_OBJECT_DATA, ObjectDataHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_GAME_LIST, GameListHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_GAME_INFO, GameInfoHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_PING_SEND, PingSendHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_PING_RETURN, PingReturnHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_GAME_UPDATE, GameUpdateHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_ENDLEVEL_SHORT, EndLevelShortHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_MINE_DATA, MineDataHandler, 0, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_GAME_PLAYERS, GamePlayersHandler, SEQUENCE_PACKET_SIZE, (1 << NETSTAT_PLAYING) | (1 << NETSTAT_STARTING) | (1 << NETSTAT_ENDLEVEL));
PHINIT (PID_NAMES_RETURN, NamesReturnHandler, 0, 1 << NETSTAT_BROWSING);
PHINIT (PID_EXTRA_GAMEINFO, ExtraGameInfoHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_DOWNLOAD, DownloadHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_UPLOAD, UploadHandler, 0, (1 << NETSTAT_STARTING) | (1 << NETSTAT_PLAYING) | (1 << NETSTAT_WAITING));
PHINIT (PID_XML_GAMEINFO, XMLGameInfoHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_TRACKER_GET_SERVERLIST, TrackerHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_TRACKER_ADD_SERVER, TrackerHandler, 0, (int16_t) 0xFFFF);
PHINIT (PID_RESEND_MESSAGE, ResentMessageHandler, 0, (1 << NETSTAT_PLAYING));
PHINIT (PID_CONFIRM_MESSAGE, ConfirmMessageHandler, 0, (1 << NETSTAT_PLAYING));
}

//-----------------------------------------------------------------------------------------------------------------

int32_t NetworkProcessSinglePacket (uint8_t* data, int32_t nLength)
{
	uint8_t					pId = data [0];
	tPacketHandlerInfo*	piP = packetHandlers + pId;
	int32_t					nFuncRes = 0;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tPlayerSyncData tmpPacket;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveSequencePacket (data, &tmpPacket);
	data = reinterpret_cast<uint8_t*>(&tmpPacket); // reassign reinterpret_cast<tPlayerSyncData*>(data) to point to correctly alinged structure
	}
#endif

if (!piP->packetHandler)
	PrintLog (0, "invalid packet id %d\n", pId);
else if (!(piP->nStatusFilter & (1 << networkData.nStatus)))
	PrintLog (0, "invalid status %d for packet id %d\n", networkData.nStatus, pId);
else if (!NetworkBadPacketSize (nLength, piP->nLength, piP->pszInfo)) {
	console.printf (0, "received %s\n", piP->pszInfo);
	if (!addressFilter [pId])	// patch the proper IP address into the packet header
		memcpy (&reinterpret_cast<tPlayerSyncData*>(data)->player.network, &networkData.packetSource.Address (), sizeof (tNetworkNode));
	nFuncRes = piP->packetHandler (data, nLength);
	}
return nFuncRes;
}

//-----------------------------------------------------------------------------------------------------------------

int32_t NetworkProcessPacket (uint8_t* data, int32_t nLength)
{
if (!networkThread.Available () || !(data [0] & 0x80))
	return NetworkProcessSinglePacket (data, nLength);

if (NetworkBadCombinedPacketSize (data, nLength)) 
	return 0;

tPacketHandlerInfo* piP = NULL;

int32_t nPackets = 0;
int32_t nProcessed = 0;

data [0] &= ~0x80;
networkThread.LockProcess ();
for (int32_t i = 0; i < nLength; i += piP->nLength) {
	++nPackets;
	piP = packetHandlers + data [i];
	if (NetworkProcessSinglePacket (data + i, piP->nLength))
		++nProcessed;
	}
networkThread.UnlockProcess ();
return nProcessed == nPackets;
}

//-----------------------------------------------------------------------------------------------------------------

