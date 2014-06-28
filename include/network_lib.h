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
#	define	NW_SET_INT(_b, _loc, _i)		*(reinterpret_cast<int *> ((_b) + (_loc))) = INTEL_INT ((int) (_i)); (_loc) += 4
#	define	NW_SET_SHORT(_b, _loc, _i)		*(reinterpret_cast<short *> ((_b) + (_loc))) = INTEL_SHORT ((short) (_i)); (_loc) += 2
#	define	NW_GET_INT(_b, _loc, _i)		(_i) = INTEL_INT (*(reinterpret_cast<int *> ((_b) + (_loc)))); (_loc) += 4
#	define	NW_GET_SHORT(_b, _loc, _i)		(_i) = INTEL_SHORT (*(reinterpret_cast<short *> ((_b) + (_loc)))); (_loc) += 2
#else
#	define	NW_SET_INT(_b, _loc, _i)	 {int tmpi = INTEL_INT (_i); memcpy ((_b) + (_loc), &tmpi, 4); (_loc) += 4; }
#	define	NW_SET_SHORT(_b, _loc, _i)	 {int tmps = INTEL_SHORT (_i); memcpy ((_b) + (_loc), &tmpi, 2); (_loc) += 2; }
#endif
#define	NW_SET_BYTE(_b, _loc, _i)			(_b) [(_loc)++] = (ubyte) (_i)
#define	NW_SET_BYTES(_b, _loc, _p, _n)	memcpy ((_b) + (_loc), _p, _n); (_loc) += (_n)
#define	NW_GET_BYTE(_b, _loc, _i)			(_i) = (_b) [(_loc)++]
#define	NW_GET_BYTES(_b, _loc, _p, _n)	memcpy (_p, (_b) + (_loc), _n); (_loc) += (_n)

//------------------------------------------------------------------------------

typedef struct tMissingObjFrames {
	ubyte					pid;
	ubyte					nPlayer;
	ushort				nFrame;
} __pack__ tMissingObjFrames;

typedef struct tRefuseData {
	char	bThisPlayer;
	char	bWaitForAnswer;
	char	bTeam;
	char	szPlayer [12];
	fix	xTimeLimit;
	} __pack__ tRefuseData;

typedef struct tSyncObjectsData {
	int					nMode;
	int					nCurrent;
	int					nSent;
	ushort				nFrame;
	tMissingObjFrames	missingFrames;
} __pack__ tSyncObjectsData;

typedef struct tNetworkSyncData {
	time_t				timeout;
	time_t				tLastJoined;
	tSequencePacket	player [2];
	short					nPlayer;
	short					nExtrasPlayer; 
	short					nState;
	short					nExtras;
	bool					bExtraGameInfo;
	bool					bAllowedPowerups;
	tSyncObjectsData	objs;
} __pack__ tNetworkSyncData;

#include "ipx_drv.h"
#include "ipx_udp.h"
#include "timeout.h"

typedef struct tNetworkData {
	ubyte					localAddress [10];
	ubyte					serverAddress [10];
	IPXRecvData_t		packetSource;
	int					nActiveGames;
	int					nLastActiveGames;
	int					nNamesInfoSecurity;
	int					nPacketsPerSec;
	int					nMaxXDataSize;
	int					nNetLifeKills;
	int					nNetLifeKilled;
	int					bDebug;
	int					bActive;
	int					nStatus;
	int					bGamesChanged;
	int					nPortOffset;
	int					bAllowSocketChanges;
	int					nSecurityFlag;
	int					nSecurityNum;
	int					nJoinState;
	int					bNewGame;       
	int					bPlayerAdded;   
	int					bD2XData;
	int					nSecurityCheck;
	fix					nLastPacketTime [MAX_PLAYERS];
	int					bPacketUrgent;
	int					nGameType;
	int					nTotalMissedPackets;
	int					nTotalPacketsGot;
	int					nMissedPackets;
	int					nConsistencyErrorCount;
	tFrameInfoLong		syncPack;
	tFrameInfoLong		urgentSyncPack;
	ubyte					bSyncPackInited;       
	ushort				nSegmentCheckSum;
	tSequencePacket	thisPlayer;
	char					bWantPlayersInfo;
	char					bWaitingForPlayerInfo;
	fix					nStartWaitAllTime;
	int					bWaitAllChoice;
	fix					xLastSendTime;
	fix					xLastTimeoutCheck;
	fix					xPingReturnTime;
	int					bShowPingStats;
	int					tLastPingStat;
	int					bHaveSync;
	short					nPrevFrame;
	int					bTraceFrames;
	tRefuseData			refuse;
	CTimeout				toSyncPoll;
	time_t				toWaitAllPoll;
	tNetworkSyncData	sync [MAX_JOIN_REQUESTS];
	short					nJoining;
	int					xmlGameInfoRequestTime;
} tNetworkData;

extern tNetworkData networkData;

//------------------------------------------------------------------------------

typedef struct tIPToCountry {
	int	minIP, maxIP;
	char  country [4];
} tIPToCountry;

//------------------------------------------------------------------------------

typedef struct tNakedData {
	int	nLength;
	int	nDestPlayer;
   char	buf [NET_XDATA_SIZE + 4];
	} __pack__ tNakedData;

extern tNakedData	nakedData;

extern CNetGameInfo tempNetInfo;

extern const char *pszRankStrings [];

extern int nLastNetGameUpdate [MAX_ACTIVE_NETGAMES];
#if 1
extern CNetGameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
extern tExtraGameInfo activeExtraGameInfo [MAX_ACTIVE_NETGAMES];
extern CAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
extern CAllNetPlayersInfo* playerInfoP;
#endif

//------------------------------------------------------------------------------

void InitPacketHandlers (void);
void LogExtraGameInfo (void);
int NetworkCreateMonitorVector (void);

int CmpNetPlayers (char *callsign1, char *callsign2, CNetworkInfo *network1, CNetworkInfo *network2);
int CmpLocalPlayer (CNetworkInfo *pNetwork, char *pszNetCallSign, char *pszLocalCallSign);
int NetworkWaitForPlayerInfo (void);
void NetworkCountPowerupsInMine (void);
int FindActiveNetGame (char *pszGameName, int nSecurity);
void DeleteActiveNetGame (int i);
int WhoIsGameHost (void);
void NetworkConsistencyError (void);
void NetworkPing (ubyte flag, ubyte nPlayer);
void NetworkHandlePingReturn (ubyte nPlayer);
void DoRefuseStuff (tSequencePacket *their);
int GotTeamSpawnPos (void);
int TeamSpawnPos (int i);
int NetworkHowManyConnected (void);
void NetworkAbortSync (void);
int NetworkVerifyObjects (int nRemoteObjNum, int nLocalObjs);
int NetworkVerifyPlayers (void);
void NetworkRequestPlayerNames (int);

void NetworkNewPlayer (tSequencePacket *their);
int CanJoinNetGame (CNetGameInfo *game, CAllNetPlayersInfo *people);
void NetworkWelcomePlayer (tSequencePacket *their);
void NetworkNewPlayer (tSequencePacket *their);
void NetworkAddPlayer (tSequencePacket *seqP);
void NetworkTimeoutPlayer (int nPlayer, int t);
void NetworkDisconnectPlayer (int nPlayer);
void NetworkRemovePlayer (tSequencePacket *seqP);
void DoRefuseStuff (tSequencePacket *their);
void NetworkDumpPlayer (ubyte * server, ubyte *node, int nReason);

void NetworkProcessSyncPacket (CNetGameInfo * sp, int rsinit);
void NetworkReadObjectPacket (ubyte *dataP);
void NetworkReadEndLevelPacket (ubyte *dataP);
void NetworkReadEndLevelShortPacket (ubyte *dataP);
void NetworkReadPDataLongPacket (tFrameInfoLong *pd);
void NetworkReadPDataShortPacket (tFrameInfoShort *pd);
void NetworkReadObjectPacket (ubyte *dataP);

void NetworkProcessMonitorVector (int vector);
void NetworkProcessGameInfo (ubyte *dataP);
void NetworkProcessLiteInfo (ubyte *dataP);
int NetworkProcessExtraGameInfo (ubyte *dataP);
void NetworkProcessDump (tSequencePacket *their);
void NetworkProcessRequest (tSequencePacket *their);
void NetworkProcessPData (char *dataP);
void NetworkProcessNakedPData (char *dataP, int len);
void NetworkProcessNamesReturn (char *dataP);
void NetworkProcessMissingObjFrames (char *dataP);
void NetworkWaitForRequests (void);

int NetworkProcessPacket (ubyte *dataP, int nLength);

void NetworkSendDoorUpdates (int nPlayer);
void NetworkSendPlayerFlags (void);
void NetworkSendFlyThruTriggers (int nPlayer); 
void NetworkSendSmashedLights (int nPlayer); 
void NetworkSendMarkers (void);
void NetworkSendRejoinSync (int nPlayer, tNetworkSyncData *syncP);
void ResendSyncDueToPacketLoss (void);
void NetworkSendFlyThruTriggers (int nPlayer); 
void NetworkSendAllInfoRequest (char nType, int nSecurity);
void NetworkSendEndLevelSub (int nPlayer);
void NetworkSendEndLevelPacket (void);
void NetworkSendEndLevelShortSub (int from_player_num, int to_player);
void NetworkSendGameInfo (tSequencePacket *their);
void NetworkSendExtraGameInfo (tSequencePacket *their);
void NetworkSendLiteInfo (tSequencePacket *their);
void NetworkSendXMLGameInfo (void);
void NetworkSendXMLGameInfo (void);
char* XMLGameInfo (void);
void NetworkSendNetGameUpdate (void);
int NetworkSendRequest (void);
void NetworkSendSync (void);
void NetworkSendData (ubyte * ptr, int len, int urgent);
void NetworkSendNakedPacket (char *buf, short len, int who);
void NetworkSendPlayerNames (tSequencePacket *their);
void NetworkSendMissingObjFrames (void);

int NetworkWaitForSync (void);
void NetworkDoSyncFrame (void);
void NetworkStopResync (tSequencePacket *their);
void NetworkUpdateNetGame (void);
void NetworkDoBigWait (int choice);
void NetworkSyncExtras (tNetworkSyncData *syncP);
tNetworkSyncData *FindJoiningPlayer (short nPlayer);
int NetworkObjnumIsPast(int nObject, tNetworkSyncData *syncP);

int XMLGameInfoHandler (ubyte *dataP = NULL, int nLength = 0);

void InitAddressFilter (void);

void NetworkSendPing (ubyte);

//------------------------------------------------------------------------------

#if DBG

void ResetPlayerTimeout (int nPlayer, fix t);

#else

static inline void ResetPlayerTimeout (int nPlayer, fix t)
{
networkData.nLastPacketTime [nPlayer] = (t < 0) ? (fix) SDL_GetTicks () : t;
}

#endif

//------------------------------------------------------------------------------

#endif //NETWORK_LIB_H
