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

#ifndef _NETWORK_H
#define _NETWORK_H

#include "loadgame.h"
#include "multi.h"
#include "newmenu.h"

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

#define NETGAMEIPX                  1
#define NETGAMETCP                  2

#define PID_LITE_INFO       43 // 0x2B lite game info
#define PID_SEND_ALL_GAMEINFO 44 // 0x2C plz send more than lite only
#define PID_PLAYERSINFO     45 // 0x2D here's my name & personal data
#define PID_REQUEST         46 // 0x2E may i join, plz send sync
#define PID_SYNC            47 // 0x2F master says: enter mine now!
#define PID_PDATA           48 // 0x30
#define PID_ADDPLAYER       49

#define PID_DUMP            51 // 0x33 you can't join this game
#define PID_ENDLEVEL        52

#define PID_QUIT_JOINING    54
#define PID_OBJECT_DATA     55 // array of bots, players, powerups, ...
#define PID_GAME_LIST       56 // 0x38 give me the list of your games
#define PID_GAME_INFO       57 // 0x39 here's a game i've started
#define PID_PING_SEND       58
#define PID_PING_RETURN     59
#define PID_GAME_UPDATE     60 // inform about new tPlayer/team change
#define PID_ENDLEVEL_SHORT  61
#define PID_NAKED_PDATA     62
#define PID_GAME_PLAYERS    63
#define PID_NAMES_RETURN    64 // 0x40

#define PID_EXTRA_GAMEINFO	 65
#define PID_DOWNLOAD			 66
#define PID_UPLOAD			 67
#define PID_SEND_EXTRA_GAMEINFO 68

#define PID_TRACKER_ADD_SERVER		'S'
#define PID_TRACKER_GET_SERVERLIST	'R'

#ifdef _DEBUG
#	define UDP_SAFEMODE	0
#else
#	define UDP_SAFEMODE	0
#endif

// defines and other things for appletalk/ipx games on mac

#define APPLETALK_GAME  1
#define IPX_GAME        2
#define UDP_GAME			3
extern int nNetworkGameType;
extern int nNetworkGameSubType;

typedef struct tSequencePacket {
	ubyte           nType;
	int             nSecurity;
	ubyte           pad1[3];
	tNetPlayerInfo  player;
} __pack__ tSequencePacket;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//      bytes on byte boundries
//      shorts on even byte boundries
//      ints on even byte boundries

typedef struct tFrameInfo {
	ubyte       nType;                   // What nType of packet
	ubyte       pad[3];                 // Pad out length of tFrameInfo packet
	int         numpackets;
	vmsVector	obj_pos;
	vmsMatrix	obj_orient;
	vmsVector	phys_velocity;
	vmsVector	phys_rotvel;
	short       obj_segnum;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       nPlayer;
	ubyte       obj_renderType;
	ubyte       level_num;
	ubyte       data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ tFrameInfo;

// tFrameInfoShort is not aligned -- 01/18/96 -- MWA
// won't align because of tShortPos.  Shortpos needs
// to stay in current form.

typedef struct tFrameInfoShort {
	ubyte       nType;                   // What nType of packet
	ubyte       pad[3];                 // Pad out length of tFrameInfo packet
	int         numpackets;
	tShortPos   thepos;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       nPlayer;
	ubyte       obj_renderType;
	ubyte       level_num;
	ubyte       data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ tFrameInfoShort;

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
	char		nCaptureVirusLimit; 
	char		nCaptureTimeLimit; 
	char		bRevertRooms;
	char		bDoConquerWarning;
	char		nOverrideTextures;
	char		bBrightenRooms;
	char		bPlayerHandicap;
} tEntropyGameInfo;

#define MAX_MONSTERBALL_FORCES	25

typedef struct tMonsterballForce {
	ubyte		nWeaponId;
	ubyte		nForce;
} tMonsterballForce;

typedef struct tMonsterballInfo {
	char					nBonus;
	char					nSizeMod;
	tMonsterballForce forces [MAX_MONSTERBALL_FORCES];
} tMonsterballInfo;

typedef struct tHeadLightInfo {
	int bAvailable;
	int bDrainPower;
	int bBuiltIn;
}  tHeadLightInfo;

typedef struct tExtraGameInfo {
	ubyte   	nType;
	char		bFriendlyFire;
	char		bInhibitSuicide;
	char		bFixedRespawns;
	int		nSpawnDelay;
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
	char		nBossCount;
	char		bSmartWeaponSwitch;
	char		bFluidPhysics;
	char		nWeaponDropMode;
	tEntropyGameInfo entropy;
	char		bRotateLevels;
	char		bDisableReactor;
	char		bMouseLook;
	char		nWeaponIcons;
	char		bSafeUDP;
	char		bFastPitch;
	char		bUseSmoke;
	char		bUseLightnings;
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
	char		bTagOnlyHitObjs;
	char		bTowFlags;
	char		bUseHitAngles;
	char		bLightTrails;
	char		bTracers;
	char		bShockwaves;
	char		bCompetition;
	char		bFlickerLights;
	char		bCheckUDPPort;
	char		bSmokeGrenades;
	char		nMaxSmokeGrenades;
	char		nMslTurnSpeed;
	char		nCoopPenalty;
	char		bKillMissiles;
	char		bTripleFusion;
	char		bEnhancedShakers;
	char		nHitboxes;
	char		nRadar;
	char		nDrag;
	char		nSpotSize;
	char		nSpotStrength;
	int		nLightRange;
	tMonsterballInfo	monsterball;
	char		szGameName [NETGAME_NAME_LEN + 1];
	int		nSecurity;
	tHeadLightInfo	headlight;
} __pack__ tExtraGameInfo;

typedef struct tMpParams {
	char	szGameName [NETGAME_NAME_LEN + 1];
	char	szServerIpAddr [22];
	int	udpClientPort;
	ubyte	nLevel;
	ubyte	nGameType;
	ubyte	nGameMode;
	ubyte	nGameAccess;
	char	bShowPlayersOnAutomap;
	char	nDifficulty;
	int	nWeaponFilter;
	int	nReactorLife;
	ubyte	nMaxTime;
	ubyte	nKillGoal;
	ubyte	bInvul;
	ubyte	bMarkerView;
	ubyte	bAlwaysBright;
	ubyte	bBrightPlayers;
	ubyte bDarkness;
	ubyte bTeamDoors;
	ubyte bEnableCheats;
	ubyte	bShowAllNames;
	ubyte	bShortPackets;
	ubyte	nPPS;
	tMsnListEntry	mission;
} tMpParams;

extern tMpParams mpParams;

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
	int					bSendingExtras;
	int					bVerifyPlayerJoined;
	int					nPlayerJoiningExtras; 
	int					bRejoined;      
	int					bNewGame;       
	int					bSendObjects; 
	int					nSendObjNum;   
	int					bPlayerAdded;   
	int					bSendObjectMode; 
	int					bD2XData;
	tSequencePacket	playerRejoining;
	fix					nLastPacketTime [MAX_PLAYERS];
	int					bPacketUrgent;
	int					nGameType;
	int					nTotalMissedPackets;
	int					nTotalPacketsGot;
	tFrameInfo			mySyncPack;
	tFrameInfo			urgentSyncPack;
	ubyte					mySyncPackInited;       
	ushort				nMySegsCheckSum;
	tSequencePacket	mySeq;
	char					bWantPlayersInfo;
	char					bWaitingForPlayerInfo;
	fix					nStartWaitAllTime;
	int					bWaitAllChoice;
	fix					xPingReturnTime;
	int					bShowPingStats;
	int					tLastPingStat;
} tNetworkData;

extern tNetworkData networkData;

#if 1

static inline int EGIFlag (char bLocalFlag, char bMultiFlag, char bDefault, int bAllowLocalFlagOn, int bAllowLocalFlagOff)
{
if (!IsMultiGame)
	return bLocalFlag;
if (!gameStates.app.bHaveExtraGameInfo [1])	//host doesn't use d2x-xl or runs in pure D2 mode
	return bDefault;
if (bLocalFlag == bMultiFlag)
	return bMultiFlag;
if (bLocalFlag) {
	if (bAllowLocalFlagOn || IsCoopGame)
		return bLocalFlag;
	}
else {
	if (bAllowLocalFlagOff || IsCoopGame)
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

int NetworkStartGame();
void NetworkRejoinGame();
void NetworkLeaveGame();
int NetworkEndLevel(int *secret);
void NetworkEndLevelPoll2(int nitems, struct tMenuItem * menus, int * key, int citem);

extern tExtraGameInfo extraGameInfo [2];

int NetworkListen();
int NetworkLevelSync();
void NetworkSendEndLevelPacket();

int network_delete_extraObjects();
int network_find_max_net_players();
int NetworkObjnumIsPast(int nObject);
char * NetworkGetPlayerName(int nObject);
void NetworkSendEndLevelSub(int player_num);
void NetworkDisconnectPlayer(int playernum);

extern void NetworkDumpPlayer(ubyte * server, ubyte *node, int why);
extern void NetworkSendNetgameUpdate();

extern int GetMyNetRanking();

extern int NetGameType;
extern int Network_sendObjects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern fix LastPacketTime[MAX_PLAYERS];

// By putting an up-to-20-char-message into Network_message and
// setting Network_message_reciever to the tPlayer num you want to
// send it to (100 for broadcast) the next frame the tPlayer will
// get your message.

// Call once at the beginning of a frame
void NetworkDoFrame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void NetworkSendData(ubyte * ptr, int len, int urgent);

// returns 1 if hoard.ham available
extern int HoardEquipped();

typedef struct tPingStats {
	fix	launchTime;
	int	ping;
	int	sent;
	int	received;
} tPingStats;

extern tPingStats pingStats [MAX_PLAYERS];
extern fix xPingReturnTime;
extern int bShowPingStats, tLastPingStat;
extern char *pszRankStrings[];

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
 * identifies a netgame. It is a random number chosen by the network master
 * (the one that did "start netgame").
 */

// MWA -- these structures are aligned -- please save me sanity and
// headaches by keeping alignment if these are changed!!!!  Contact
// me for info.

typedef struct tEndLevelInfo {
	ubyte                               nType;
	ubyte                               nPlayer;
	sbyte                               connected;
	ubyte                               seconds_left;
	short											killMatrix [MAX_PLAYERS] [MAX_PLAYERS];
	short                               kills;
	short                               killed;
} tEndLevelInfo;

typedef struct tEndLevelInfoShort {
	ubyte                               nType;
	ubyte                               nPlayer;
	sbyte                               connected;
	ubyte                               seconds_left;
} tEndLevelInfoShort;

// WARNING!!! This is the top part of tNetgameInfo...if that struct changes,
//      this struct much change as well.  ie...they are aligned and the join system will
// not work without it.

// MWA  if this structure changes -- please make appropriate changes to receive_netgame_info
// code for macintosh in netmisc.c

typedef struct tLiteInfo {
	ubyte                           nType;
	int                             nSecurity;
	char                            szGameName [NETGAME_NAME_LEN+1];
	char                            szMissionTitle [MISSION_NAME_LEN+1];
	char                            szMissionName [9];
	int                             nLevel;
	ubyte                           gameMode;
	ubyte                           bRefusePlayers;
	ubyte                           difficulty;
	ubyte                           gameStatus;
	ubyte                           nNumPlayers;
	ubyte                           nMaxPlayers;
	ubyte                           nConnected;
	ubyte                           gameFlags;
	ubyte                           protocol_version;
	ubyte                           version_major;
	ubyte                           version_minor;
	ubyte                           teamVector;
} __pack__ tLiteInfo;

#define NETGAME_INFO_SIZE       sizeof(tNetgameInfo)
#define ALLNETPLAYERSINFO_SIZE  sizeof(tAllNetPlayersInfo)
#define LITE_INFO_SIZE          sizeof(tLiteInfo)
#define SEQUENCE_PACKET_SIZE    sizeof(tSequencePacket)
#define FRAME_INFO_SIZE         sizeof(tFrameInfo)
#define IPX_SHORT_INFO_SIZE     sizeof(tFrameInfoShort)
#define ENTROPY_INFO_SIZE       sizeof(tExtraGameInfo)

#define MAX_ACTIVE_NETGAMES     80

int  NetworkSendRequest(void);
int  NetworkChooseConnect();
int  NetworkSendGameListRequest();
void NetworkAddPlayer (tSequencePacket *p);
void NetworkSendGameInfo(tSequencePacket *their);
void ClipRank (char *rank);
void NetworkCheckForOldVersion(char pnum);
void NetworkInit(void);
void NetworkFlush();
int  NetworkWaitForAllInfo(int choice);
void NetworkSetGameMode(int gameMode);
void NetworkAdjustMaxDataSize();
int CanJoinNetgame(tNetgameInfo *game,tAllNetPlayersInfo *people);
void RestartNetSearching(tMenuItem * m);
void DeleteTimedOutNetGames (void);
void InitMonsterballSettings (tMonsterballInfo *monsterballP);
void InitEntropySettings (int i);
void NetworkSendExtraGameInfo (tSequencePacket *their);
char *iptos (char *pszIP, char *addr);

#define DUMP_CLOSED     0 // no new players allowed after game started
#define DUMP_FULL       1 // tPlayer cound maxed out
#define DUMP_ENDLEVEL   2
#define DUMP_DORK       3
#define DUMP_ABORTED    4
#define DUMP_CONNECTED  5 // never used
#define DUMP_LEVEL      6
#define DUMP_KICKED     7 // never used

extern tNetgameInfo activeNetGames [MAX_ACTIVE_NETGAMES];
extern tAllNetPlayersInfo activeNetPlayers [MAX_ACTIVE_NETGAMES];
extern tAllNetPlayersInfo *tmpPlayersInfo, tmpPlayersBase;
extern int nCoopPenalties [10];


#define COMPETITION	(IsMultiGame && !IsCoopGame && extraGameInfo [1].bCompetition)

#endif /* _NETWORK_H */
