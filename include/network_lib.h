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

#ifndef _NETWORK_LIB_H
#define _NETWORK_LIB_H

//------------------------------------------------------------------------------

#define SECURITY_CHECK		1
#define GET_MISSING_FRAMES 1
#define REFUSE_INTERVAL		I2X (8)

#define MAX_JOIN_REQUESTS	(MAX_PLAYERS - 1)


#if 1
#	define	NW_SET_INT(_b, _loc, _i)		*(reinterpret_cast<int32_t *> ((_b) + (_loc))) = INTEL_INT ((int32_t) (_i)); (_loc) += 4
#	define	NW_SET_SHORT(_b, _loc, _i)		*(reinterpret_cast<int16_t *> ((_b) + (_loc))) = INTEL_SHORT ((int16_t) (_i)); (_loc) += 2
#	define	NW_GET_INT(_b, _loc, _i)		(_i) = INTEL_INT (*(reinterpret_cast<int32_t *> ((_b) + (_loc)))); (_loc) += 4
#	define	NW_GET_SHORT(_b, _loc, _i)		(_i) = INTEL_SHORT (*(reinterpret_cast<int16_t *> ((_b) + (_loc)))); (_loc) += 2
#else
#	define	NW_SET_INT(_b, _loc, _i)	 {int32_t tmpi = INTEL_INT (_i); memcpy ((_b) + (_loc), &tmpi, 4); (_loc) += 4; }
#	define	NW_SET_SHORT(_b, _loc, _i)	 {int32_t tmps = INTEL_SHORT (_i); memcpy ((_b) + (_loc), &tmpi, 2); (_loc) += 2; }
#endif
#define	NW_SET_BYTE(_b, _loc, _i)			(_b) [(_loc)++] = (uint8_t) (_i)
#define	NW_SET_BYTES(_b, _loc, _p, _n)	memcpy ((_b) + (_loc), _p, _n); (_loc) += (_n)
#define	NW_GET_BYTE(_b, _loc, _i)			(_i) = (_b) [(_loc)++]
#define	NW_GET_BYTES(_b, _loc, _p, _n)	memcpy (_p, (_b) + (_loc), _n); (_loc) += (_n)

//------------------------------------------------------------------------------

typedef struct tMissingObjFrames {
	uint8_t					pid;
	uint8_t					nPlayer;
	uint16_t				nFrame;
} __pack__ tMissingObjFrames;

typedef struct tRefuseData {
	char	bThisPlayer;
	char	bWaitForAnswer;
	char	bTeam;
	char	szPlayer [12];
	fix	xTimeLimit;
	} __pack__ tRefuseData;

typedef struct tSyncObjectsData {
	int32_t					nMode;
	int32_t					nCurrent;
	int32_t					nSent;
	uint16_t				nFrame;
	tMissingObjFrames	missingFrames;
} __pack__ tSyncObjectsData;

typedef struct tNetworkSyncData {
	time_t				timeout;
	time_t				tLastJoined;
	tSequencePacket	player [2];
	int16_t					nPlayer;
	int16_t					nExtrasPlayer; 
	int16_t					nState;
	int16_t					nExtras;
	bool					bExtraGameInfo;
	bool					bAllowedPowerups;
	bool					bDeferredSync;
	tSyncObjectsData	objs;
} __pack__ tNetworkSyncData;

#include "ipx_drv.h"
#include "ipx_udp.h"
#include "timeout.h"

typedef struct tNetworkData {
	uint8_t					localAddress [10];
	uint8_t					serverAddress [10];
	CPacketAddress		packetSource;
	int32_t					nActiveGames;
	int32_t					nLastActiveGames;
	int32_t					nNamesInfoSecurity;
	int32_t					nPacketsPerSec;
	int32_t					nMaxXDataSize;
	int32_t					nNetLifeKills;
	int32_t					nNetLifeKilled;
	int32_t					bDebug;
	int32_t					bActive;
	int32_t					nStatus;
	int32_t					bGamesChanged;
	int32_t					nPortOffset;
	int32_t					bAllowSocketChanges;
	int32_t					nSecurityFlag;
	int32_t					nSecurityNum;
	int32_t					nJoinState;
	int32_t					bNewGame;       
	int32_t					bPlayerAdded;   
	int32_t					bD2XData;
	int32_t					nSecurityCheck;
	fix					nLastPacketTime [MAX_PLAYERS];
	int32_t					bPacketUrgent;
	int32_t					nGameType;
	int32_t					nTotalMissedPackets;
	int32_t					nTotalPacketsGot;
	int32_t					nMissedPackets;
	int32_t					nConsistencyErrorCount;
	tFrameInfoLong		syncPack;
	tFrameInfoLong		urgentSyncPack;
	uint8_t					bSyncPackInited;       
	uint16_t				nSegmentCheckSum;
	tSequencePacket	thisPlayer;
	char					bWantPlayersInfo;
	char					bWaitingForPlayerInfo;
	fix					nStartWaitAllTime;
	int32_t					bWaitAllChoice;
	fix					xLastSendTime;
	fix					xLastTimeoutCheck;
	fix					xPingReturnTime;
	int32_t					bShowPingStats;
	int32_t					tLastPingStat;
	int32_t					bHaveSync;
	int16_t					nPrevFrame;
	int32_t					bTraceFrames;
	tRefuseData			refuse;
	CTimeout				toSyncPoll;
	time_t				toWaitAllPoll;
	tNetworkSyncData	sync [MAX_JOIN_REQUESTS];
	int16_t					nJoining;
	int32_t					xmlGameInfoRequestTime;
} tNetworkData;

extern tNetworkData networkData;

//------------------------------------------------------------------------------

typedef struct tIPToCountry {
	int32_t	minIP, maxIP;
	char  country [4];
} tIPToCountry;

//------------------------------------------------------------------------------

typedef struct tNakedData {
	int32_t	nLength;
	int32_t	nDestPlayer;
   char	buf [NET_XDATA_SIZE + 4];
	} __pack__ tNakedData;

extern tNakedData	nakedData;

extern CNetGameInfo tempNetInfo;

extern const char *pszRankStrings [];

extern int32_t nLastNetGameUpdate [MAX_ACTIVE_NETGAMES];
#if 1
extern CNetGameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
extern tExtraGameInfo activeExtraGameInfo [MAX_ACTIVE_NETGAMES];
extern CAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
extern CAllNetPlayersInfo* playerInfoP;
#endif

//------------------------------------------------------------------------------

void InitPacketHandlers (void);
void LogExtraGameInfo (void);
int32_t NetworkCreateMonitorVector (void);

int32_t CmpNetPlayers (char *callsign1, char *callsign2, CNetworkInfo *network1, CNetworkInfo *network2);
int32_t CmpLocalPlayer (CNetworkInfo *pNetwork, char *pszNetCallSign, char *pszLocalCallSign);
int32_t NetworkWaitForPlayerInfo (void);
void NetworkCountPowerupsInMine (void);
int32_t FindActiveNetGame (char *pszGameName, int32_t nSecurity);
void DeleteActiveNetGame (int32_t i);
int32_t WhoIsGameHost (void);
void NetworkConsistencyError (void);
void NetworkPing (uint8_t flag, uint8_t nPlayer);
void NetworkHandlePingReturn (uint8_t nPlayer);
void DoRefuseStuff (tSequencePacket *their);
int32_t GotTeamSpawnPos (void);
int32_t TeamSpawnPos (int32_t i);
int32_t NetworkHowManyConnected (void);
void NetworkAbortSync (void);
int32_t NetworkVerifyObjects (int32_t nRemoteObjNum, int32_t nLocalObjs);
int32_t NetworkVerifyPlayers (void);
void NetworkRequestPlayerNames (int32_t);

void NetworkNewPlayer (tSequencePacket *their);
int32_t CanJoinNetGame (CNetGameInfo *game, CAllNetPlayersInfo *people);
void NetworkWelcomePlayer (tSequencePacket *their);
void NetworkNewPlayer (tSequencePacket *their);
void NetworkAddPlayer (tSequencePacket *seqP);
int32_t NetworkTimeoutPlayer (int32_t nPlayer, int32_t t);
void NetworkDisconnectPlayer (int32_t nPlayer);
void NetworkRemovePlayer (tSequencePacket *seqP);
void DoRefuseStuff (tSequencePacket *their);
void NetworkDumpPlayer (uint8_t * server, uint8_t *node, int32_t nReason);

void NetworkProcessSyncPacket (CNetGameInfo * sp, int32_t rsinit);
void NetworkReadObjectPacket (uint8_t *data);
void NetworkReadEndLevelPacket (uint8_t *data);
void NetworkReadEndLevelShortPacket (uint8_t *data);
void NetworkReadPDataLongPacket (tFrameInfoLong *pd);
void NetworkReadPDataShortPacket (tFrameInfoShort *pd);
void NetworkReadObjectPacket (uint8_t *data);

void NetworkProcessMonitorVector (int32_t vector);
void NetworkProcessGameInfo (uint8_t *data);
void NetworkProcessLiteInfo (uint8_t *data);
int32_t NetworkProcessExtraGameInfo (uint8_t *data);
void NetworkProcessDump (tSequencePacket *their);
void NetworkProcessRequest (tSequencePacket *their);
void NetworkProcessPData (uint8_t* data);
void NetworkProcessNakedPData (uint8_t* data, int32_t len);
void NetworkProcessNamesReturn (uint8_t* data);
void NetworkProcessMissingObjFrames (uint8_t* data);
void NetworkWaitForRequests (void);

int32_t NetworkProcessPacket (uint8_t *data, int32_t nLength);

void NetworkSendDoorUpdates (int32_t nPlayer);
void NetworkSendPlayerFlags (void);
void NetworkSendFlyThruTriggers (int32_t nPlayer); 
void NetworkSendSmashedLights (int32_t nPlayer); 
void NetworkSendMarkers (void);
void NetworkSendRejoinSync (int32_t nPlayer, tNetworkSyncData *syncP);
void ResendSyncDueToPacketLoss (void);
void NetworkSendFlyThruTriggers (int32_t nPlayer); 
void NetworkSendAllInfoRequest (char nType, int32_t nSecurity);
void NetworkSendEndLevelSub (int32_t nPlayer);
void NetworkSendEndLevelPacket (void);
void NetworkSendEndLevelShortSub (int32_t from_player_num, int32_t to_player);
void NetworkSendGameInfo (tSequencePacket *their);
void NetworkSendExtraGameInfo (tSequencePacket *their);
void NetworkSendLiteInfo (tSequencePacket *their);
void NetworkSendXMLGameInfo (void);
void NetworkSendXMLGameInfo (void);
char* XMLGameInfo (void);
void NetworkSendNetGameUpdate (void);
int32_t NetworkSendRequest (void);
void NetworkSendSync (void);
void NetworkSendData (uint8_t * ptr, int32_t len, int32_t urgent);
void NetworkSendNakedPacket (char *buf, int16_t len, int32_t who);
void NetworkSendPlayerNames (tSequencePacket *their);
void NetworkSendMissingObjFrames (void);

int32_t NetworkWaitForSync (void);
void NetworkDoSyncFrame (void);
void NetworkStopResync (tSequencePacket *their);
void NetworkUpdateNetGame (void);
void NetworkDoBigWait (int32_t choice);
void NetworkSyncExtras (tNetworkSyncData *syncP);
tNetworkSyncData *FindJoiningPlayer (int16_t nPlayer);
int32_t NetworkObjnumIsPast(int32_t nObject, tNetworkSyncData *syncP);

int32_t XMLGameInfoHandler (uint8_t *data = NULL, int32_t nLength = 0);

void InitAddressFilter (void);

void NetworkSendPing (uint8_t);

//------------------------------------------------------------------------------

#if DBG

void ResetPlayerTimeout (int32_t nPlayer, fix t);

#else

static inline void ResetPlayerTimeout (int32_t nPlayer, fix t)
{
networkData.nLastPacketTime [nPlayer] = (t < 0) ? (fix) SDL_GetTicks () : t;
}

#endif

//------------------------------------------------------------------------------

#endif //NETWORK_LIB_H
