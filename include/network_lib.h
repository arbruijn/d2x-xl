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

#include "multi.h"

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

typedef union tFrameInfo {
	tFrameInfoLong		longInfo;
	tFrameInfoShort	shortInfo;
	uint8_t				data [MAX_PACKET_SIZE];
} tFrameInfo;

class CSyncPack {
	public:
		uint32_t	m_nPackets;

	public:
		CSyncPack () {}
		~CSyncPack () {}
		virtual void Reset (void) { SetMsgDataSize (0); }
		virtual int32_t HeaderSize (void) = 0;
		virtual int32_t MaxDataSize (void) = 0;
		virtual tFrameInfoHeader& Header (void) = 0;
		virtual tFrameInfoData& Data (void) = 0;
		virtual void* Info (void) = 0;
		virtual void SetInfo (void* info) = 0;

		virtual uint8_t* Buffer (void) { return reinterpret_cast<uint8_t*> (this); }
		virtual int32_t Size (void) { return HeaderSize () + MsgDataSize (); }

		virtual uint8_t* MsgData (uint32_t offset = 0) { return Data ().msgData + offset; }
		virtual uint16_t MsgDataSize (void) { return Data ().dataSize; }
		virtual void SetMsgDataSize (uint16_t dataSize) { Data ().dataSize = dataSize; }

		virtual void SetType (uint8_t nType) { Header ().nType = nType; }
		virtual void SetPackets (int32_t nPackets) { Header ().nPackets = nPackets; }
		virtual void SetPlayer (uint8_t nPlayer) { Data ().nPlayer = nPlayer; }
		virtual void SetRenderType (uint8_t nRenderType) { Data ().nRenderType = nRenderType; }
		virtual void SetLevel (uint8_t nLevel) { Data ().nLevel = nLevel; }

		virtual uint8_t GetType (void) { return Header ().nType; }
		virtual int32_t GetPackets (void) { return Header ().nPackets; }
		virtual uint8_t GetPlayer (void) { return Data ().nPlayer; }
		virtual uint8_t GetRenderType (void) { return Data ().nRenderType; }
		virtual uint8_t GetLevel (void) { return Data ().nLevel; }

		virtual void SetObjInfo (CObject* objP) = 0;

		virtual bool Overflow (uint16_t msgLen) { return MsgDataSize () + msgLen > MaxDataSize (); }

		virtual int AppendMessage (uint8_t* msg, uint16_t msgLen) {
			if (Overflow (msgLen))
				Send ();
			memcpy (MsgData (MsgDataSize ()), msg, msgLen);
			SetMsgDataSize (MsgDataSize () + msgLen);
			return MsgDataSize ();
			}

		virtual void Squish (void) = 0;

		virtual void Prepare (void);
		virtual void Send (void);
	};


class CSyncPackLong : public CSyncPack {
	public:
		tFrameInfoLong m_info;

	public:
		CSyncPackLong () { Reset (); }
		~CSyncPackLong () {}
		virtual void* Info (void) { return &m_info; }
		virtual int32_t HeaderSize (void) { return sizeof (m_info) - UDP_PAYLOAD_SIZE; }
		virtual int32_t MaxDataSize (void) { return UDP_PAYLOAD_SIZE - HeaderSize (); }
		virtual tFrameInfoHeader& Header (void) { return m_info.header; }
		virtual tFrameInfoData& Data (void) { return m_info.data; }
		virtual void SetInfo (void* info) { memcpy (&m_info, info, HeaderSize () + reinterpret_cast<tFrameInfoLong*>(info)->data.dataSize); }
		virtual void SetObjInfo (CObject* objP) { CreateLongPos (&m_info.objData, objP); }
		virtual void Squish (void);

		inline CSyncPackLong& operator= (tFrameInfoLong& info) { 
			SetInfo (&info); 
			return *this;
			}
	};


class CSyncPackShort : public CSyncPack {
	public:
		tFrameInfoShort m_info;

	public:
		CSyncPackShort () { Reset (); }
		~CSyncPackShort () {}
		virtual void* Info (void) { return &m_info; }
		virtual int32_t HeaderSize (void) { return sizeof (m_info) - UDP_PAYLOAD_SIZE; }
		virtual int32_t MaxDataSize (void) { return NET_XDATA_SIZE; }
		virtual tFrameInfoHeader& Header (void) { return m_info.header; }
		virtual tFrameInfoData& Data (void) { return m_info.data; }
		virtual void SetInfo (void* info) { memcpy (&m_info, info, HeaderSize () + reinterpret_cast<tFrameInfoShort*>(info)->data.dataSize); }
		virtual void SetObjInfo (CObject* objP) { CreateShortPos (&m_info.objData, objP); }
		virtual void Squish (void);

		inline CSyncPackShort& operator= (tFrameInfoShort& info) { 
			SetInfo (&info); 
			return *this;
			}
	};

//------------------------------------------------------------------------------

typedef struct tNakedData {
	int32_t	nLength;
	int32_t	receiver;
   uint8_t	buffer [UDP_PAYLOAD_SIZE];
	} __pack__ tNakedData;

class CNakedData {
	public:
		tNakedData	m_data;

	public:
		CNakedData () { Reset (); }

		inline void Reset (void) {
			m_data.nLength = 0;
			m_data.receiver = -1;
			}

		inline int32_t Length (void) { return m_data.nLength; }

		inline int32_t SetLength (int nLength) { return m_data.nLength = nLength; }

		inline uint8_t* Buffer (int32_t offset = 0) { return m_data.buffer + offset; }

		inline int32_t HeaderSize (void) { return sizeof (m_data) - UDP_PAYLOAD_SIZE; }

		inline int32_t MaxDataSize (void) { return (gameStates.multi.nGameType == UDP_GAME) ? UDP_PAYLOAD_SIZE - HeaderSize () : NET_XDATA_SIZE + 4; }

		inline tNakedData& Data (void) { return m_data; }

		void Flush (void);

		void SendMessage (uint8_t* msgBuf, int16_t msgLen, int32_t receiver);
	};

extern CNakedData nakedData;

//------------------------------------------------------------------------------

typedef struct tRefuseData {
	char					bThisPlayer;
	char					bWaitForAnswer;
	char					bTeam;
	char					szPlayer [12];
	fix					xTimeLimit;
	} __pack__ tRefuseData;

typedef struct tSyncObjectsData {
	int32_t				nMode;
	int32_t				nCurrent;
	int32_t				nSent;
	uint16_t				nFrame;
	uint16_t				nFramesToSkip;
} __pack__ tSyncObjectsData;

typedef struct tNetworkSyncInfo {
	time_t				timeout;
	time_t				tLastJoined;
	tSequencePacket	player [2];
	int16_t				nPlayer;
	int16_t				nExtrasPlayer; 
	int16_t				nState;
	int16_t				nExtras;
	bool					bExtraGameInfo;
	bool					bAllowedPowerups;
	bool					bDeferredSync;
	tSyncObjectsData	objs;
} __pack__ tNetworkSyncInfo;

#include "ipx_drv.h"
#include "ipx_udp.h"
#include "timeout.h"

class CNetworkData {
	public:
		int32_t					nActiveGames;
		int32_t					nLastActiveGames;
		int32_t					nNamesInfoSecurity;
		int32_t					nMinPPS;
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
		int32_t					bPacketUrgent;
		int32_t					nGameType;
		int32_t					nTotalMissedPackets;
		int32_t					nTotalPacketsGot;
		int32_t					nMissedPackets;
		int32_t					nConsistencyErrorCount;
		uint8_t					bSyncPackInited;       
		uint8_t					bWantPlayersInfo;
		uint8_t					bWaitingForPlayerInfo;
		uint16_t					nSegmentCheckSum;
		fix						nStartWaitAllTime;
		fix						nLastPacketTime [MAX_PLAYERS];
		fix						xLastSendTime;
		fix						xLastTimeoutCheck;
		fix						xPingReturnTime;
		int32_t					tLastPingStat;
		int32_t					bWaitAllChoice;
		int32_t					bShowPingStats;
		int32_t					bHaveSync;
		int16_t					nPrevFrame;
		int32_t					bTraceFrames;
		int16_t					nJoining;
		int32_t					xmlGameInfoRequestTime;
		tRefuseData				refuse;
		tSequencePacket		thisPlayer;
		tNetworkSyncInfo		syncInfo [MAX_JOIN_REQUESTS];

		CNetworkAddress		localAddress;
		CNetworkAddress		serverAddress;
		CTimeout					toWaitAllPoll;
		CTimeout					toSyncPoll;
		CPacketAddress			packetSource;
		CSyncPackLong			longSyncPack;
		CSyncPackShort			shortSyncPack;

	public:
		CSyncPack& SyncPack (void) { 
			if (netGameInfo.GetShortPackets ()) 
				return shortSyncPack;
			else
				return longSyncPack; 
			}
};

extern CNetworkData networkData;

//------------------------------------------------------------------------------

typedef struct tIPToCountry {
	int32_t	minIP, maxIP;
	char		country [4];
} tIPToCountry;

//------------------------------------------------------------------------------

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
void NetworkWaitForRequests (void);

int32_t NetworkProcessPacket (uint8_t *data, int32_t nLength);

void NetworkSendDoorUpdates (int32_t nPlayer);
void NetworkSendPlayerFlags (void);
void NetworkSendFlyThruTriggers (int32_t nPlayer); 
void NetworkSendSmashedLights (int32_t nPlayer); 
void NetworkSendMarkers (void);
void NetworkSendRejoinSync (int32_t nPlayer, tNetworkSyncInfo *syncInfoP);
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
void NetworkSendNakedPacket (uint8_t* buf, int16_t len, int32_t who);
void NetworkSendPlayerNames (tSequencePacket *their);
void NetworkSendMissingObjFrames (void);

int32_t NetworkWaitForSync (void);
void NetworkDoSyncFrame (void);
void NetworkStopResync (tSequencePacket *their);
void NetworkUpdateNetGame (void);
void NetworkDoBigWait (int32_t choice);
void NetworkSyncExtras (tNetworkSyncInfo *syncInfoP);
tNetworkSyncInfo* FindJoiningPlayer (int16_t nPlayer);
int32_t NetworkObjnumIsPast(int32_t nObject, tNetworkSyncInfo *syncInfoP);

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
