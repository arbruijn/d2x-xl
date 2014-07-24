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

#ifndef _NETWORK_H
#define _NETWORK_H

#include "ipx_drv.h"
#include "ipx.h"
#include "multi.h"
#include "menu.h"
#include "loadgame.h"
#include "network_thread.h"

#define EGI_DATA_VERSION				8

#define NETWORK_OEM						0x10

#define NETSTAT_MENU                0
#define NETSTAT_PLAYING             1
#define NETSTAT_BROWSING            2
#define NETSTAT_WAITING             3
#define NETSTAT_STARTING            4
#define NETSTAT_ENDLEVEL            5
#define NETSTAT_AUTODL					6

#define CONNECT_DISCONNECTED        0
#define CONNECT_PLAYING             1
#define CONNECT_WAITING             2
#define CONNECT_DIED_IN_MINE        3
#define CONNECT_FOUND_SECRET        4
#define CONNECT_ESCAPE_TUNNEL       5
#define CONNECT_END_MENU            6
#define CONNECT_ADVANCE_LEVEL			7

#define NETGAMEIPX                  1
#define NETGAMETCP                  2

#define PID_LITE_INFO					43 // 0x2B lite game info
#define PID_SEND_ALL_GAMEINFO			44 // 0x2C plz send more than lite only
#define PID_PLAYERSINFO					45 // 0x2D here's my name & personal data
#define PID_REQUEST						46 // 0x2E may i join, plz send sync
#define PID_SYNC							47 // 0x2F master says: enter mine now!
#define PID_PLAYER_DATA					48 // 0x30
#define PID_ADDPLAYER					49

#define PID_DUMP							51 // 0x33 you can't join this game
#define PID_ENDLEVEL						52

#define PID_QUIT_JOINING				54
#define PID_OBJECT_DATA					55 // array of bots, players, powerups, ...
#define PID_GAME_LIST					56 // 0x38 give me the list of your games
#define PID_GAME_INFO					57 // 0x39 here's a game i've started
#define PID_PING_SEND					58
#define PID_PING_RETURN					59
#define PID_GAME_UPDATE					60 // inform about new player/team change
#define PID_ENDLEVEL_SHORT				61
#define PID_MINE_DATA					62
#define PID_GAME_PLAYERS				63
#define PID_NAMES_RETURN				64 // 0x40

#define PID_EXTRA_GAMEINFO				65
#define PID_DOWNLOAD						66
#define PID_UPLOAD						67

#define PID_RESEND_MESSAGE				68
#define PID_CONFIRM_MESSAGE			69

#define PID_XML_GAMEINFO				70	// 'F'

#define PID_TRACKER_ADD_SERVER		'S'
#define PID_TRACKER_GET_SERVERLIST	'R'

#define FIRST_PID							PID_LITE_INFO
#define LAST_PID							PID_TRACKER_GET_SERVERLIST

#define TIMEOUT_MSG_RESEND_COOP		300
#define TIMEOUT_MSG_RESEND_STD		150
#define TIMEOUT_MSG_RESEND_COMP		70

#define TIMEOUT_MSG_RESEND_COUNT		8

#define TIMEOUT_MSG_KEEP(TIMEOUT_MSG_RESEND)	TIMEOUT_MSG_RESEND * TIMEOUT_MSG_RESEND_COUNT + 10

#define TIMEOUT_MSG_KEEP_COOP			TIMEOUT_MSG_KEEP(TIMEOUT_MSG_RESEND_COOP)
#define TIMEOUT_MSG_KEEP_STD			TIMEOUT_MSG_KEEP(TIMEOUT_MSG_RESEND_STD)
#define TIMEOUT_MSG_KEEP_COMP			TIMEOUT_MSG_KEEP(TIMEOUT_MSG_RESEND_COMP)


#if DBG
#	define TIMEOUT_DISCONNECT			30000
#	define TIMEOUT_KICK					180000
#	define TIMEOUT_KEEP_MSG				3000
#	define TIMEOUT_RESEND_MSG			300
#else
#	define TIMEOUT_DISCONNECT			3000
#	define TIMEOUT_KICK					30000
#	define TIMEOUT_KEEP_MSG				1500
#	define TIMEOUT_RESEND_MSG			150
#endif


#if DBG
#	define UDP_SAFEMODE	0
#else
#	define UDP_SAFEMODE	0
#endif

#define NET_XDATA_SIZE					454

// defines and other things for appletalk/ipx games on mac
#if 0
extern int32_t nNetworkGameType;
extern int32_t nNetworkGameSubType;
#endif

typedef struct tPlayerSyncData {
	uint8_t           nType;
	int32_t           nSecurity;
	uint16_t				nObjFramesToSkip;
	uint8_t           pad1;
	tNetPlayerInfo		player;
	uint8_t           pad2 [3];
} __pack__ tPlayerSyncData;



// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//      bytes on byte boundaries
//      shorts on even byte boundaries
//      ints on even byte boundaries

typedef struct tFrameInfoHeader {
	uint8_t				nType;        
	uint8_t				pad [3];       
	int32_t				nPackets;
} __pack__ tFrameInfoHeader;

typedef struct tFrameInfoData {
	uint16_t				dataSize;          // Size of data appended to the net packet
	uint8_t				nPlayer;
	uint8_t				nRenderType;
	uint8_t				nLevel;
	uint8_t				msgData [UDP_PAYLOAD_SIZE];   // extra data to be tacked on the end
} __pack__ tFrameInfoData;

typedef struct tFrameInfoLong {
	tFrameInfoHeader	header;
	tLongPos				objData;
	tFrameInfoData		data;
} __pack__ tFrameInfoLong;

// tFrameInfoShort is not aligned -- 01/18/96 -- MWA
// won't align because of tShortPos. Shortpos needs to stay in current form.

typedef struct tFrameInfoShort {
	tFrameInfoHeader	header;
	tShortPos			objData;
	tFrameInfoData		data;
} __pack__ tFrameInfoShort;

typedef struct tEntropyGameInfo {
	uint16_t		nEnergyFillRate;
	uint16_t		nShieldFillRate;
	uint16_t		nShieldDamageRate;
	uint16_t		nMaxVirusCapacity; 
	char			nBumpVirusCapacity;
	char			nBashVirusCapacity; 
	char			nVirusGenTime; 
	char			nVirusLifespan; 
	char			nVirusStability;
	char			nCaptureVirusThreshold; 
	char			nCaptureTimeThreshold; 
	char			bRevertRooms;
	char			bDoCaptureWarning;
	char			nOverrideTextures;
	char			bBrightenRooms;
	char			bPlayerHandicap;
} __pack__ tEntropyGameInfo;

#define MAX_MONSTERBALL_FORCES	25

typedef struct tMonsterballForce {
	uint8_t		nWeaponId;
	int16_t		nForce;
} __pack__ tMonsterballForce;

typedef struct tMonsterballInfo {
	char					nBonus;
	char					nSizeMod;
	tMonsterballForce forces [MAX_MONSTERBALL_FORCES];
} __pack__ tMonsterballInfo;

typedef struct tHeadlightInfo {
	char	bAvailable;
	char	bDrainPower;
}  tHeadlightInfo;

typedef struct tLoadoutInfo {
	uint32_t					nGuns;
	uint32_t					nDevice;
	uint8_t					nMissiles [10];
} __pack__ tLoadoutInfo;

typedef struct tTimeoutInfo {
	int32_t					nDisconnectPlayer;
	int32_t					nKickPlayer;
	uint16_t					nKeepMessage;
	uint16_t					nResendMessage;
} __pack__ tTimeoutInfo;

typedef struct tExtraGameInfo {
	uint8_t   			nType;
	int32_t				nVersion;

	char					szGameName [NETGAME_NAME_LEN + 1];
	tEntropyGameInfo	entropy;
	tMonsterballInfo	monsterball;
	tHeadlightInfo		headlight;
	tLoadoutInfo		loadout;
	tTimeoutInfo		timeout;

	uint8_t				bFriendlyFire;
	uint8_t				bInhibitSuicide;
	uint8_t				bFixedRespawns;
	uint8_t				bEnhancedCTF;
	uint8_t				bRadarEnabled;
	uint8_t				bPowerupsOnRadar;
	uint8_t				nZoomMode;
	uint8_t				bRobotsHitRobots;
	uint8_t				bAutoDownload;
	uint8_t				bAutoBalanceTeams;
	uint8_t				bDualMissileLaunch;
	uint8_t				bRobotsOnRadar;
	uint8_t				grWallTransparency;
	uint8_t				nSpeedBoost;
	uint8_t				bDropAllMissiles;
	uint8_t				bImmortalPowerups;
	uint8_t				bUseCameras;
	uint8_t				nFusionRamp;
	uint8_t				nOmegaRamp;
	uint8_t				bWiggle;
	uint8_t				bMultiBosses;
	uint8_t				nBossCount [2];
	uint8_t				bSmartWeaponSwitch;
	uint8_t				bFluidPhysics;
	uint8_t				nWeaponDropMode;
	uint8_t				bRotateLevels;
	uint8_t				bDisableReactor;
	uint8_t				bMouseLook;
	uint8_t				nWeaponIcons;
	uint8_t				bSafeUDP;
	uint8_t				bFastPitch;
	uint8_t				bUseParticles;
	uint8_t				bUseLightning;
	uint8_t				bDamageExplosions;
	uint8_t				bThrusterFlames;
	uint8_t				bShadows;
	uint8_t				nShieldEffect;
	uint8_t				bTeleporterCams;
	uint8_t				bDarkness;
	uint8_t				bPowerupLights;
	uint8_t				bBrightObjects;
	uint8_t				bTeamDoors;
	uint8_t				bEnableCheats;
	uint8_t				bTargetIndicators;
	uint8_t				bDamageIndicators;
	uint8_t				bFriendlyIndicators;
	uint8_t				bCloakedIndicators;
	uint8_t				bMslLockIndicators;
	uint8_t				bHideIndicators;
	uint8_t				bTagOnlyHitObjs;
	uint8_t				bTowFlags;
	uint8_t				bUseHitAngles;
	uint8_t				bLightTrails;
	uint8_t				bGatlingTrails;
	uint8_t				bTracers;
	uint8_t				bShockwaves;
	uint8_t				bCompetition;
	uint8_t				bFlickerLights;
	uint8_t				bCheckUDPPort;
	uint8_t				bSmokeGrenades;
	uint8_t				nMaxSmokeGrenades;
	uint8_t				nMslTurnSpeed;
	uint8_t				nMslStartSpeed;
	uint8_t				nCoopPenalty;
	uint8_t				bKillMissiles;
	uint8_t				bTripleFusion;
	uint8_t				bEnhancedShakers;
	uint8_t				bShowWeapons;
	uint8_t				bGatlingSpeedUp;
	uint8_t				bRotateMarkers;
	uint8_t				bAllowCustomWeapons;
	uint8_t				nHitboxes;
	uint8_t				nDamageModel;
	uint8_t				nRadar;
	uint8_t				nDrag;
	uint8_t				nSpotSize;
	uint8_t				nSpotStrength;

	int32_t				nSpawnDelay;
	int32_t				nLightRange;
	int32_t				nSpeedScale;
	int32_t				nSecurity;
	int32_t				shipsAllowed [MAX_SHIP_TYPES];
} __pack__ tExtraGameInfo;

typedef struct tMpParams {
	char					szGameName [NETGAME_NAME_LEN + 1];
	char					szServerIpAddr [22];
	int32_t				udpPorts [2];
	uint8_t				nLevel;
	uint8_t				nGameType;
	uint8_t				nGameMode;
	uint8_t				nGameAccess;
	uint8_t				bShowPlayersOnAutomap;
	uint8_t				nDifficulty;
	int32_t				nWeaponFilter;
	int32_t				nReactorLife;
	uint8_t				nMaxTime;
	uint8_t				nScoreGoal;
	uint8_t				bInvul;
	uint8_t				bMarkerView;
	uint8_t				bIndestructibleLights;
	uint8_t				bBrightPlayers;
	uint8_t				bDarkness;
	uint8_t				bTeamDoors;
	uint8_t				bEnableCheats;
	uint8_t				bShowAllNames;
	uint8_t				bShortPackets;
	uint8_t				nMinPPS;
	tMsnListEntry		mission;
} __pack__ tMpParams;

extern tMpParams mpParams;

typedef struct tNetworkObjInfo {
	int16_t				nObject;
	uint8_t				nType;
	uint8_t				nId;
} __pack__ tNetworkObjInfo;

extern tExtraGameInfo extraGameInfo [3];

//	-----------------------------------------------------------------------------

#if 1

static inline int32_t EGIFlag (char bLocalFlag, char bMultiFlag, char bDefault, int32_t bAllowLocalFlagOn, int32_t bAllowLocalFlagOff)
{
if (!IsMultiGame || IsCoopGame)
	return bLocalFlag;
if (!gameStates.app.bHaveExtraGameInfo [1])	//host doesn't use d2x-xl or runs in pure D2 mode
	return bDefault;
if ((bLocalFlag != 0) == (bMultiFlag != 0))
	return bMultiFlag;
if (bLocalFlag) {
	if (bAllowLocalFlagOn)
		return bLocalFlag;
	}
else {
	if (bAllowLocalFlagOff)
		return bLocalFlag;
	}
return bMultiFlag;
}

#define	EGI_FLAG(_bFlag, _bAllowLocalOn, _bAllowLocalOff, _bDefault)	\
			EGIFlag (extraGameInfo [0]._bFlag, extraGameInfo [1]._bFlag, _bDefault, _bAllowLocalOn, _bAllowLocalOff)

#else

#define	EGI_FLAG(_bFlag, _bAllocLocalOn, _bAllowLocalOff, _bDefault) \
			 (IsMultiGame ? \
				gameStates.app.bHaveExtraGameInfo [1] ? \
					extraGameInfo [1]._bFlag ? \
						_bLocal ? \
							extraGameInfo [0]._bFlag \
							: extraGameInfo [1]._bFlag \
						: 0 \
					: _bDefault \
				: extraGameInfo [0]._bFlag)

#endif

//	-----------------------------------------------------------------------------

static inline bool IsBuiltinWeapon (int32_t nWeapon)
{
return (!IsMultiGame || gameStates.app.bHaveExtraGameInfo [IsMultiGame]) && 
		 (extraGameInfo [IsMultiGame].loadout.nGuns & HAS_FLAG (nWeapon));
}

//	-----------------------------------------------------------------------------

#define COLLISION_MODEL_SPHERES	0
#define COLLISION_MODEL_BOXES		2

static inline int32_t CollisionModel (void)
{
return missionConfig.m_nCollisionModel ? COLLISION_MODEL_BOXES : EGI_FLAG (nHitboxes, 0, 0, extraGameInfo [0].nHitboxes);
}

//	-----------------------------------------------------------------------------

int32_t NetworkStartGame (void);
void NetworkRejoinGame (void);
void NetworkLeaveGame (bool bDisconnect = true);
int32_t NetworkEndLevel (int32_t *secret);
int32_t NetworkEndLevelPoll2 (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState);

int32_t NetworkListen (void);
int32_t NetworkLevelSync (void);
void NetworkSendLifeSign (void);
void NetworkSendEndLevelPacket (void);

int32_t network_delete_extraObjects (void);
int32_t network_find_max_net_players (void);
char * NetworkGetPlayerName (int32_t nObject);
void NetworkSendEndLevelSub (int32_t player_num);
void NetworkDisconnectPlayer (int32_t playernum);

extern void NetworkDumpPlayer (uint8_t * server, uint8_t *node, int32_t why);
extern void NetworkSendNetGameUpdate (void);

extern int32_t GetMyNetRanking (void);

extern int32_t NetGameType;
extern int32_t Network_sendObjects;
extern int32_t Network_send_objnum;
extern int32_t PacketUrgent;
extern int32_t Network_rejoined;

extern int32_t Network_new_game;
extern int32_t Network_status;

extern fix LastPacketTime [MAX_PLAYERS];

// By putting an up-to-20-char-message into Network_message and
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void NetworkDoFrame (int bFlush = 0);
void NetworkFlushData (void);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void NetworkSendData (uint8_t * ptr, int32_t len, int32_t urgent);

// returns 1 if hoard.ham available
extern int32_t HoardEquipped (void);

typedef struct tPingStats {
	fix	launchTime;
	int32_t	ping;
	int32_t	totalPing;
	int32_t	averagePing;
	int32_t	sent;
	int32_t	received;
} __pack__ tPingStats;

extern tPingStats pingStats [MAX_PLAYERS];
extern fix xPingReturnTime;
extern int32_t bShowPingStats, tLastPingStat;
extern const char *pszRankStrings[];

void ResetPingStats (void);
void InitExtraGameInfo (void);
void InitNetworkData (void);
int32_t InitAutoNetGame (void);

#define NETGAME_ANARCHY				0
#define NETGAME_TEAM_ANARCHY		1
#define NETGAME_ROBOT_ANARCHY		2
#define NETGAME_COOPERATIVE		3
#define NETGAME_CAPTURE_FLAG		4
#define NETGAME_HOARD				5
#define NETGAME_TEAM_HOARD			6
#define NETGAME_ENTROPY				7
#define NETGAME_MONSTERBALL		8

/* The following are values for networkData.nSecurityFlag */
#define NETSECURITY_OFF                 0
#define NETSECURITY_WAIT_FOR_PLAYERS    1
#define NETSECURITY_WAIT_FOR_GAMEINFO   2
#define NETSECURITY_WAIT_FOR_SYNC       3
/* The networkData.nSecurityNum and the "nSecurity" field of the network structs
 * identifies a netgame. It is a Random number chosen by the network master
 * (the one that did "start netgame").
 */

// MWA -- these structures are aligned -- please save me sanity and
// headaches by keeping alignment if these are changed!!!!  Contact
// me for info.

typedef struct tEndLevelInfoShort {
	uint8_t                               nType;
	uint8_t                               nPlayer;
	int8_t                               connected;
	uint8_t                               secondsLeft;
} __pack__ tEndLevelInfoShort;

typedef struct tEndLevelInfoD2 : public tEndLevelInfoShort {
	int16_t											scoreMatrix [MAX_PLAYERS_D2][MAX_PLAYERS_D2];
	int16_t                               kills;
	int16_t                               killed;
} __pack__ tEndLevelInfoD2;

typedef struct tEndLevelInfoD2X : public tEndLevelInfoShort  {
	int16_t											scoreMatrix [MAX_PLAYERS_D2X][MAX_PLAYERS_D2X];
	int16_t                               kills;
	int16_t                               killed;
} __pack__ tEndLevelInfoD2X;

typedef union tEndLevelInfo {
	tEndLevelInfoD2							d2;
	tEndLevelInfoD2X							d2x;
} __pack__ tEndLevelInfo;

class CEndLevelInfo {
	public:
		tEndLevelInfo	m_info;

		CEndLevelInfo() { memset (&m_info, 0, sizeof (m_info)); }
		CEndLevelInfo (tEndLevelInfo* eli) { *this = *eli; }

		inline size_t Size (void) { return gameStates.app.bD2XLevel ? sizeof (m_info.d2x) : sizeof (m_info.d2); }

		tEndLevelInfo& operator= (tEndLevelInfo& other) {
			memcpy (&m_info, &other, Size ());
			return m_info;
			}

		tEndLevelInfo& operator= (CEndLevelInfo& other) {
			m_info = other.m_info;
			return m_info;
			}

		inline uint8_t* Type (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.nType : &m_info.d2.nType; }
		inline uint8_t* Player (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.nPlayer : &m_info.d2.nPlayer; }
		inline int8_t Connected (void) { return gameStates.app.bD2XLevel ? m_info.d2x.connected : m_info.d2.connected; }
		inline void SetConnected (int8_t nStatus) { 
			if (gameStates.app.bD2XLevel)
				m_info.d2x.connected = nStatus;
			else
				m_info.d2.connected = nStatus; 
			}
		inline uint8_t* SecondsLeft (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.secondsLeft : &m_info.d2.secondsLeft; }
		inline int16_t* ScoreMatrix (int32_t i = 0, int32_t j = 0) { return gameStates.app.bD2XLevel ? m_info.d2x.scoreMatrix [i] + j : m_info.d2.scoreMatrix [i] + j; }
		inline int16_t* Kills (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.kills : &m_info.d2.kills; }
		inline int16_t* Killed (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.killed : &m_info.d2.killed; }
};

// WARNING!!! This is the top part of tNetGameInfo...if that struct changes,
//      this struct much change as well.  ie...they are aligned and the join system will
// not work without it.

// MWA  if this structure changes -- please make appropriate changes to receive_netgame_info
// code for macintosh in netmisc.c


#define NETGAME_INFO_SIZE       int32_t (netGameInfo.Size ())
#define ALLNETPLAYERSINFO_SIZE  int32_t (netPlayers [0].Size ())
#define LITE_INFO_SIZE          sizeof (tNetGameInfoLite)
#define SEQUENCE_PACKET_SIZE    sizeof (tPlayerSyncData)
#define FRAME_INFO_SIZE         sizeof (tFrameInfoLong)
#define IPX_SHORT_INFO_SIZE     sizeof (tFrameInfoShort)
#define ENTROPY_INFO_SIZE       sizeof (tExtraGameInfo)

#define MAX_ACTIVE_NETGAMES     80

int32_t  NetworkSendRequest (void);
int32_t  NetworkChooseConnect (void);
int32_t  NetworkSendGameListRequest (int32_t bAutoLaunch = 0);
void NetworkSetTimeoutValues (void);
void NetworkAddPlayer (tPlayerSyncData *p);
void NetworkSendGameInfo (tPlayerSyncData *their);
void ClipRank (char *rank);
void NetworkCheckForOldVersion (uint8_t nPlayer);
void NetworkInit (void);
void NetworkFlush (void);
int32_t  NetworkWaitForAllInfo (int32_t choice);
void NetworkSetGameMode (int32_t gameMode);
int32_t CanJoinNetGame (CNetGameInfo *game, CAllNetPlayersInfo *people);
void RestartNetSearching (CMenu& menu);
void DeleteTimedOutNetGames (void);
void InitMonsterballSettings (tMonsterballInfo *monsterballP);
void InitEntropySettings (int32_t i);
void NetworkSendExtraGameInfo (tPlayerSyncData *their);
void NetworkSendXMLGameInfo (void);
void NetworkResetSyncStates (void);
void NetworkResetObjSync (int16_t nObject);
void DeleteSyncData (int16_t nConnection);
char *iptos (char *pszIP, char *addr);

#define DUMP_CLOSED     0 // no new players allowed after game started
#define DUMP_FULL       1 // player cound maxed out
#define DUMP_ENDLEVEL   2
#define DUMP_DORK       3
#define DUMP_ABORTED    4
#define DUMP_CONNECTED  5 // never used
#define DUMP_LEVEL      6
#define DUMP_KICKED     7 // never used

extern CNetGameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
extern CAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
extern CAllNetPlayersInfo* playerInfoP, netPlayers [2];
extern int32_t nCoopPenalties [10];

#define COMPETITION			(IsMultiGame && !IsCoopGame && extraGameInfo [1].bCompetition)
#define COMPETITION_LEVEL	(IsMultiGame ? extraGameInfo [1].bCompetition ? 2 : IsCoopGame ? 0 : 1 : 0)

#define MAX_PAYLOAD_SIZE ((gameStates.multi.nGameType == UDP_GAME) ? UDP_PAYLOAD_SIZE : IPX_PAYLOAD_SIZE)

//------------------------------------------------------------------------------

#define MIN_PPS		5
#define MAX_PPS		100
#define DEFAULT_PPS	20

static inline int16_t MinPPS (void)
{
	int32_t	nMinPPS = netGameInfo.GetMinPPS ();

if ((nMinPPS < MIN_PPS) || (nMinPPS > MAX_PPS))
	netGameInfo.SetMinPPS (DEFAULT_PPS);
return netGameInfo.GetMinPPS ();
}

//------------------------------------------------------------------------------

#endif /* _NETWORK_H */
