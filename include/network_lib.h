/* $Id: network.h,v 1.12 2003/10/10 09:36:35 btb Exp $ */
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

#include "ipx_udp.h"
#include "ipx_drv.h"

//------------------------------------------------------------------------------

#define SECURITY_CHECK		1
#define GET_MISSING_FRAMES 1
#define REFUSE_INTERVAL		(F1_0 * 8)

#define MAX_JOIN_REQUESTS	(MAX_PLAYERS - 1)


#if 1
#	define	NW_SET_INT(_b, _loc, _i)		*((int *) ((_b) + (_loc))) = INTEL_INT ((int) (_i)); (_loc) += 4
#	define	NW_SET_SHORT(_b, _loc, _i)		*((short *) ((_b) + (_loc))) = INTEL_SHORT ((short) (_i)); (_loc) += 2
#	define	NW_GET_INT(_b, _loc, _i)		(_i) = INTEL_INT (*((int *) ((_b) + (_loc)))); (_loc) += 4
#	define	NW_GET_SHORT(_b, _loc, _i)		(_i) = INTEL_SHORT (*((short *) ((_b) + (_loc)))); (_loc) += 2
#else
#	define	NW_SET_INT(_b, _loc, _i)		{int tmpi = INTEL_INT (_i); memcpy ((_b) + (_loc), &tmpi, 4); (_loc) += 4; }
#	define	NW_SET_SHORT(_b, _loc, _i)		{int tmps = INTEL_SHORT (_i); memcpy ((_b) + (_loc), &tmpi, 2); (_loc) += 2; }
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
} tMissingObjFrames;

typedef struct tRefuseData {
	char	bThisPlayer;
	char	bWaitForAnswer;
	char	bTeam;
	char	szPlayer [12];
	fix	xTimeLimit;
	} tRefuseData;

typedef struct tSyncObjectsData {
	int					nMode;
	int					nCurrent;
	int					nSent;
	ushort				nFrame;
	tMissingObjFrames	missingFrames;
} tSyncObjectsData;

typedef struct tNetworkSyncData {
	time_t				timeout;
	time_t				tLastJoined;
	tSequencePacket	player [2];
	short					nPlayer;
	short					nExtrasPlayer; 
	short					nState;
	short					nExtras;
	bool					bExtraGameInfo;
	tSyncObjectsData	objs;
} tNetworkSyncData;

typedef struct tNetworkData {
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
	int					nSocket;
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
	tFrameInfo			syncPack;
	tFrameInfo			urgentSyncPack;
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
	time_t				toSyncPoll;
	time_t				toWaitAllPoll;
	tNetworkSyncData	sync [MAX_JOIN_REQUESTS];
	short					nJoining;
} tNetworkData;

extern tNetworkData networkData;

//------------------------------------------------------------------------------

typedef struct tNakedData {
	int	nLength;
	int	nDestPlayer;
   char	buf [NET_XDATA_SIZE + 4];
	} tNakedData;

extern tNakedData	nakedData;

extern tNetgameInfo tempNetInfo;

extern const char *pszRankStrings [];

extern struct ipx_recv_data ipx_udpSrc;

extern int nLastNetGameUpdate [MAX_ACTIVE_NETGAMES];
extern tNetgameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
extern tExtraGameInfo activeExtraGameInfo [MAX_ACTIVE_NETGAMES];
extern tAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
extern tAllNetPlayersInfo *tmpPlayersInfo, tmpPlayersBase;

//------------------------------------------------------------------------------

void InitPacketHandlers (void);
void LogExtraGameInfo (void);
int NetworkCreateMonitorVector (void);

int CmpNetPlayers (char *callsign1, char *callsign2, tNetworkInfo *network1, tNetworkInfo *network2);
int CmpLocalPlayer (tNetworkInfo *pNetwork, char *pszNetCallSign, char *pszLocalCallSign);
int NetworkWaitForPlayerInfo (void);
void NetworkCountPowerupsInMine (void);
int FindActiveNetGame (char *pszGameName, int nSecurity);
void DeleteActiveNetGame (int i);
int NetworkWhoIsMaster (void);
void NetworkConsistencyError (void);
void NetworkPing (ubyte flag, int nPlayer);
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
int CanJoinNetgame (tNetgameInfo *game, tAllNetPlayersInfo *people);
void NetworkWelcomePlayer (tSequencePacket *their);
void NetworkNewPlayer (tSequencePacket *their);
void NetworkAddPlayer (tSequencePacket *seqP);
void NetworkTimeoutPlayer (int nPlayer);
void NetworkDisconnectPlayer (int nPlayer);
void NetworkRemovePlayer (tSequencePacket *seqP);
void DoRefuseStuff (tSequencePacket *their);
void NetworkDumpPlayer (ubyte * server, ubyte *node, int nReason);

void NetworkReadSyncPacket (tNetgameInfo * sp, int rsinit);
void NetworkReadObjectPacket (ubyte *dataP);
void NetworkReadEndLevelPacket (ubyte *dataP);
void NetworkReadEndLevelShortPacket (ubyte *dataP);
void NetworkReadPDataPacket (tFrameInfo *pd);
void NetworkReadPDataShortPacket (tFrameInfoShort *pd);
void NetworkReadObjectPacket (ubyte *dataP);

void NetworkProcessMonitorVector (int vector);
void NetworkProcessGameInfo (ubyte *dataP);
void NetworkProcessLiteInfo (ubyte *dataP);
void NetworkProcessExtraGameInfo (ubyte *dataP);
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
int NetworkSendGameListRequest (void);
void NetworkSendAllInfoRequest (char nType, int nSecurity);
void NetworkSendEndLevelSub (int nPlayer);
void NetworkSendEndLevelPacket (void);
void NetworkSendEndLevelShortSub (int from_player_num, int to_player);
void NetworkSendGameInfo (tSequencePacket *their);
void NetworkSendExtraGameInfo (tSequencePacket *their);
void NetworkSendLiteInfo (tSequencePacket *their);
void NetworkSendNetgameUpdate (void);
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
void NetworkSyncExtras (void);
tNetworkSyncData *FindJoiningPlayer (short nPlayer);

void InitAddressFilter (void);

//------------------------------------------------------------------------------

static inline void ResetPlayerTimeout (int nPlayer, fix t)
{
networkData.nLastPacketTime [nPlayer] = (t < 0) ? (fix) SDL_GetTicks () : t;
}

//------------------------------------------------------------------------------

#endif //NETWORK_LIB_H
