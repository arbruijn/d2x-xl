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

#define EGI_DATA_VERSION				6

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
#define PID_PDATA							48 // 0x30
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
#define PID_NAKED_PDATA					62
#define PID_GAME_PLAYERS				63
#define PID_NAMES_RETURN				64 // 0x40

#define PID_EXTRA_GAMEINFO				65
#define PID_DOWNLOAD						66
#define PID_UPLOAD						67
#define PID_SEND_EXTRA_GAMEINFO		68
#define PID_MISSING_OBJ_FRAMES		69
#define PID_XML_GAMEINFO				70	// 'F'

#define PID_TRACKER_ADD_SERVER		'S'
#define PID_TRACKER_GET_SERVERLIST	'R'

#if DBG
#	define UDP_SAFEMODE	0
#else
#	define UDP_SAFEMODE	0
#endif

// defines and other things for appletalk/ipx games on mac
#if 0
extern int nNetworkGameType;
extern int nNetworkGameSubType;
#endif

typedef struct tSequencePacket {
	ubyte           nType;
	int             nSecurity;
	ubyte           pad1 [3];
	tNetPlayerInfo  player;
	ubyte           pad2 [3];
} __pack__ tSequencePacket;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//      bytes on byte boundaries
//      shorts on even byte boundaries
//      ints on even byte boundaries

typedef struct tFrameInfoLong {
	ubyte       nType;                   // What nType of packet
	ubyte       pad[3];                 // Pad out length of tFrameInfoLong packet
	int         nPackets;
	CFixVector	objPos;
	CFixMatrix	objOrient;
	CFixVector	physVelocity;
	CFixVector	physRotVel;
	short       nObjSeg;
	ushort      dataSize;          // Size of data appended to the net packet
	ubyte       nPlayer;
	ubyte       objRenderType;
	ubyte       nLevel;
	ubyte       data [NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ tFrameInfoLong;

// tFrameInfoShort is not aligned -- 01/18/96 -- MWA
// won't align because of tShortPos.  Shortpos needs
// to stay in current form.

typedef struct tFrameInfoShort {
	ubyte       nType;                   // What nType of packet
	ubyte       pad[3];                 // Pad out length of tFrameInfoLong packet
	int         nPackets;
	tShortPos   objPos;
	ushort      dataSize;          // Size of data appended to the net packet
	ubyte       nPlayer;
	ubyte       objRenderType;
	ubyte       nLevel;
	ubyte       data [NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ tFrameInfoShort;

typedef union tFrameInfo {
	tFrameInfoLong		l;
	tFrameInfoShort	s;
} tFrameInfo;

typedef struct tEntropyGameInfo {
	ushort	nEnergyFillRate;
	ushort	nShieldFillRate;
	ushort	nShieldDamageRate;
	ushort	nMaxVirusCapacity; 
	char		nBumpVirusCapacity;
	char		nBashVirusCapacity; 
	char		nVirusGenTime; 
	char		nVirusLifespan; 
	char		nVirusStability;
	char		nCaptureVirusThreshold; 
	char		nCaptureTimeThreshold; 
	char		bRevertRooms;
	char		bDoCaptureWarning;
	char		nOverrideTextures;
	char		bBrightenRooms;
	char		bPlayerHandicap;
} __pack__ tEntropyGameInfo;

#define MAX_MONSTERBALL_FORCES	25

typedef struct tMonsterballForce {
	ubyte		nWeaponId;
	short		nForce;
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
	uint					nGuns;
	uint					nDevice;
	ubyte					nMissiles [10];
} __pack__ tLoadoutInfo;

typedef struct tExtraGameInfo {
	ubyte   	nType;
	int		nVersion;

	char					szGameName [NETGAME_NAME_LEN + 1];
	tEntropyGameInfo	entropy;
	tMonsterballInfo	monsterball;
	tHeadlightInfo		headlight;
	tLoadoutInfo		loadout;

	char		bFriendlyFire;
	char		bInhibitSuicide;
	char		bFixedRespawns;
	char		bEnhancedCTF;
	char		bRadarEnabled;
	char		bPowerupsOnRadar;
	char		nZoomMode;
	char		bRobotsHitRobots;
	char		bAutoDownload;
	char		bAutoBalanceTeams;
	char		bDualMissileLaunch;
	char		bRobotsOnRadar;
	char		grWallTransparency;
	char		nSpeedBoost;
	char		bDropAllMissiles;
	char		bImmortalPowerups;
	char		bUseCameras;
	char		nFusionRamp;
	char		nOmegaRamp;
	char		bWiggle;
	char		bMultiBosses;
	char		nBossCount [2];
	char		bSmartWeaponSwitch;
	char		bFluidPhysics;
	char		nWeaponDropMode;
	char		bRotateLevels;
	char		bDisableReactor;
	char		bMouseLook;
	char		nWeaponIcons;
	char		bSafeUDP;
	char		bFastPitch;
	char		bUseParticles;
	char		bUseLightning;
	char		bDamageExplosions;
	char		bThrusterFlames;
	char		bShadows;
	char		bPlayerShield;
	char		bTeleporterCams;
	char		bDarkness;
	char		bPowerupLights;
	char		bBrightObjects;
	char		bTeamDoors;
	char		bEnableCheats;
	char		bTargetIndicators;
	char		bDamageIndicators;
	char		bFriendlyIndicators;
	char		bCloakedIndicators;
	char		bMslLockIndicators;
	char		bHideIndicators;
	char		bTagOnlyHitObjs;
	char		bTowFlags;
	char		bUseHitAngles;
	char		bLightTrails;
	char		bGatlingTrails;
	char		bTracers;
	char		bShockwaves;
	char		bCompetition;
	char		bFlickerLights;
	char		bCheckUDPPort;
	char		bSmokeGrenades;
	char		nMaxSmokeGrenades;
	char		nMslTurnSpeed;
	char		nMslStartSpeed;
	char		nCoopPenalty;
	char		bKillMissiles;
	char		bTripleFusion;
	char		bEnhancedShakers;
	char		bShowWeapons;
	char		bGatlingSpeedUp;
	char		bRotateMarkers;
	char		bAllowCustomWeapons;
	char		nHitboxes;
	char		nDamageModel;
	char		nRadar;
	char		nDrag;
	char		nSpotSize;
	char		nSpotStrength;

	int		nSpawnDelay;
	int		nLightRange;
	int		nSpeedScale;
	int		nSecurity;
	int		shipsAllowed [MAX_SHIP_TYPES];
} __pack__ tExtraGameInfo;

typedef struct tMpParams {
	char	szGameName [NETGAME_NAME_LEN + 1];
	char	szServerIpAddr [22];
	int	udpPorts [2];
	ubyte	nLevel;
	ubyte	nGameType;
	ubyte	nGameMode;
	ubyte	nGameAccess;
	char	bShowPlayersOnAutomap;
	char	nDifficulty;
	int	nWeaponFilter;
	int	nReactorLife;
	ubyte	nMaxTime;
	ubyte	nScoreGoal;
	ubyte	bInvul;
	ubyte	bMarkerView;
	ubyte	bIndestructibleLights;
	ubyte	bBrightPlayers;
	ubyte bDarkness;
	ubyte bTeamDoors;
	ubyte bEnableCheats;
	ubyte	bShowAllNames;
	ubyte	bShortPackets;
	ubyte	nPPS;
	tMsnListEntry	mission;
} __pack__ tMpParams;

extern tMpParams mpParams;

typedef struct tNetworkObjInfo {
	short	nObject;
	ubyte	nType;
	ubyte	nId;
} __pack__ tNetworkObjInfo;

extern tExtraGameInfo extraGameInfo [3];

//	-----------------------------------------------------------------------------

#if 1

static inline int EGIFlag (char bLocalFlag, char bMultiFlag, char bDefault, int bAllowLocalFlagOn, int bAllowLocalFlagOff)
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

static inline bool IsBuiltinWeapon (int nWeapon)
{
return (!IsMultiGame || gameStates.app.bHaveExtraGameInfo [IsMultiGame]) && 
		 (extraGameInfo [IsMultiGame].loadout.nGuns & HAS_FLAG (nWeapon));
}

//	-----------------------------------------------------------------------------

#define COLLISION_MODEL_SPHERES	0
#define COLLISION_MODEL_BOXES		2

static inline int CollisionModel (void)
{
return missionConfig.m_nCollisionModel ? COLLISION_MODEL_BOXES : EGI_FLAG (nHitboxes, 0, 0, extraGameInfo [0].nHitboxes);
}

//	-----------------------------------------------------------------------------

int NetworkStartGame (void);
void NetworkRejoinGame (void);
void NetworkLeaveGame (void);
int NetworkEndLevel (int *secret);
int NetworkEndLevelPoll2 (CMenu& menu, int& key, int nCurItem, int nState);

int NetworkListen (void);
int NetworkLevelSync (void);
void NetworkSendEndLevelPacket (void);

int network_delete_extraObjects (void);
int network_find_max_net_players (void);
char * NetworkGetPlayerName (int nObject);
void NetworkSendEndLevelSub (int player_num);
void NetworkDisconnectPlayer (int playernum);

extern void NetworkDumpPlayer (ubyte * server, ubyte *node, int why);
extern void NetworkSendNetGameUpdate (void);

extern int GetMyNetRanking (void);

extern int NetGameType;
extern int Network_sendObjects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern fix LastPacketTime [MAX_PLAYERS];

// By putting an up-to-20-char-message into Network_message and
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void NetworkDoFrame (int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void NetworkSendData (ubyte * ptr, int len, int urgent);

// returns 1 if hoard.ham available
extern int HoardEquipped (void);

typedef struct tPingStats {
	fix	launchTime;
	int	ping;
	int	totalPing;
	int	averagePing;
	int	sent;
	int	received;
} __pack__ tPingStats;

extern tPingStats pingStats [MAX_PLAYERS];
extern fix xPingReturnTime;
extern int bShowPingStats, tLastPingStat;
extern const char *pszRankStrings[];

void ResetPingStats (void);
void InitExtraGameInfo (void);
void InitNetworkData (void);
int InitAutoNetGame (void);

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
	ubyte                               nType;
	ubyte                               nPlayer;
	sbyte                               connected;
	ubyte                               secondsLeft;
} __pack__ tEndLevelInfoShort;

typedef struct tEndLevelInfoD2 : public tEndLevelInfoShort {
	short											scoreMatrix [MAX_PLAYERS_D2][MAX_PLAYERS_D2];
	short                               kills;
	short                               killed;
} __pack__ tEndLevelInfoD2;

typedef struct tEndLevelInfoD2X : public tEndLevelInfoShort  {
	short											scoreMatrix [MAX_PLAYERS_D2X][MAX_PLAYERS_D2X];
	short                               kills;
	short                               killed;
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

		inline ubyte* Type (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.nType : &m_info.d2.nType; }
		inline ubyte* Player (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.nPlayer : &m_info.d2.nPlayer; }
		inline sbyte* Connected (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.connected : &m_info.d2.connected; }
		inline ubyte* SecondsLeft (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.secondsLeft : &m_info.d2.secondsLeft; }
		inline short* ScoreMatrix (int i = 0, int j = 0) { return gameStates.app.bD2XLevel ? m_info.d2x.scoreMatrix [i] + j : m_info.d2.scoreMatrix [i] + j; }
		inline short* Kills (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.kills : &m_info.d2.kills; }
		inline short* Killed (void) { return gameStates.app.bD2XLevel ? &m_info.d2x.killed : &m_info.d2.killed; }
};

// WARNING!!! This is the top part of tNetGameInfo...if that struct changes,
//      this struct much change as well.  ie...they are aligned and the join system will
// not work without it.

// MWA  if this structure changes -- please make appropriate changes to receive_netgame_info
// code for macintosh in netmisc.c


#define NETGAME_INFO_SIZE       int (netGameInfo.Size ())
#define ALLNETPLAYERSINFO_SIZE  int (netPlayers [0].Size ())
#define LITE_INFO_SIZE          sizeof (tNetGameInfoLite)
#define SEQUENCE_PACKET_SIZE    sizeof (tSequencePacket)
#define FRAME_INFO_SIZE         sizeof (tFrameInfoLong)
#define IPX_SHORT_INFO_SIZE     sizeof (tFrameInfoShort)
#define ENTROPY_INFO_SIZE       sizeof (tExtraGameInfo)

#define MAX_ACTIVE_NETGAMES     80

int  NetworkSendRequest (void);
int  NetworkChooseConnect (void);
int  NetworkSendGameListRequest (int bAutoLaunch = 0);
void NetworkAddPlayer (tSequencePacket *p);
void NetworkSendGameInfo (tSequencePacket *their);
void ClipRank (char *rank);
void NetworkCheckForOldVersion (char pnum);
void NetworkInit (void);
void NetworkFlush (void);
int  NetworkWaitForAllInfo (int choice);
void NetworkSetGameMode (int gameMode);
void NetworkAdjustMaxDataSize (void);
int CanJoinNetGame (CNetGameInfo *game, CAllNetPlayersInfo *people);
void RestartNetSearching (CMenu& menu);
void DeleteTimedOutNetGames (void);
void InitMonsterballSettings (tMonsterballInfo *monsterballP);
void InitEntropySettings (int i);
void NetworkSendExtraGameInfo (tSequencePacket *their);
void NetworkSendXMLGameInfo (void);
void NetworkResetSyncStates (void);
void NetworkResetObjSync (short nObject);
void DeleteSyncData (short nConnection);
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
extern int nCoopPenalties [10];

#define COMPETITION	 (IsMultiGame && !IsCoopGame && extraGameInfo [1].bCompetition)

#define MAX_PAYLOAD_SIZE ((gameStates.multi.nGameType == UDP_GAME) ? UDP_PAYLOAD_SIZE : IPX_PAYLOAD_SIZE)

//------------------------------------------------------------------------------

#define MIN_PPS		5
#define MAX_PPS		100
#define DEFAULT_PPS	20

static inline short PacketsPerSec (void)
{
	int	i = netGame.GetPacketsPerSec ();

if ((i < MIN_PPS) || (i > MAX_PPS))
	netGame.SetPacketsPerSec (DEFAULT_PPS);
return netGame.GetPacketsPerSec ();
}

//------------------------------------------------------------------------------

#endif /* _NETWORK_H */
