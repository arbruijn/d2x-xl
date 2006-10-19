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

/*
 *
 * Prototypes for network management functions.
 *
 * Old Log:
 * Revision 1.7  1995/10/31  10:20:04  allender
 * shareware stuff
 *
 * Revision 1.6  1995/09/21  14:31:18  allender
 * new appltalk type packet which contains shortpos
 *
 * Revision 1.5  1995/09/18  08:07:08  allender
 * added function prototype to remove netgame NBP
 *
 * Revision 1.4  1995/08/31  15:51:55  allender
 * new prototypes for join and start games
 *
 * Revision 1.3  1995/07/26  17:02:29  allender
 * implemented and working on mac
 *
 * Revision 1.2  1995/06/02  07:42:34  allender
 * fixed prototype for NetworkEndLevelPoll2
 *
 * Revision 1.1  1995/05/16  16:00:15  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/21  14:58:09  john
 * *** empty log message ***
 *
 * Revision 2.1  1995/03/21  14:39:43  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:29:48  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.72  1995/02/11  14:24:39  rob
 * Added suppor for invul. cntrlcen.
 *
 * Revision 1.71  1995/02/08  19:18:43  rob
 * Added extern for segments checksum.
 *
 * Revision 1.70  1995/02/08  11:01:16  rob
 * Fixed a bug.
 *
 * Revision 1.69  1995/02/07  21:16:49  rob
 * Added flag for automap.
 *
 * Revision 1.68  1995/02/06  18:18:33  rob
 * Extern'ed Packet Urgent.
 *
 * Revision 1.67  1995/02/05  14:36:29  rob
 * Moved defines to network.h.
 *
 * Revision 1.66  1995/02/01  16:34:09  john
 * Linted.
 *
 * Revision 1.65  1995/01/30  21:14:35  rob
 * Fixed a bug in join menu.
 * Simplified mission load support.
 *
 * Revision 1.64  1995/01/30  18:19:40  rob
 * Added mission title to gameinfo packet.
 *
 * Revision 1.63  1995/01/28  17:02:39  rob
 * Fixed monitor syncing bug (minor 19).
 *
 * Revision 1.62  1995/01/27  11:15:25  rob
 * removed extern of variable no longer in network.c
 *
 * Revision 1.61  1995/01/26  12:17:34  rob
 * Added new param to NetworkDoFrame.
 *
 * Revision 1.60  1995/01/22  17:32:11  rob
 * Added mission support for network games.
 *
 * Revision 1.59  1995/01/17  17:10:33  rob
 * Added some flags to netgame struct.
 *
 * Revision 1.58  1995/01/12  18:57:15  rob
 * Added score resync stuff.
 *
 * Revision 1.57  1995/01/12  16:42:18  rob
 * Added new prototypes.
 *
 * Revision 1.56  1995/01/05  12:12:10  rob
 * Fixed a problem with packet size.
 *
 * Revision 1.55  1995/01/05  11:12:36  rob
 * Reduced packet size by 1 byte just to be safe.
 *
 * Revision 1.54  1995/01/04  21:39:32  rob
 * New framedata packet for registered.
 *
 * Revision 1.53  1995/01/04  08:47:04  rob
 * JOHN CHECKED IN FOR ROB !!!
 *
 * Revision 1.52  1994/12/30  20:09:07  rob
 * ADded a toggle for showing player names on HUD.
 *
 * Revision 1.51  1994/12/29  15:59:52  rob
 * Centralized network disconnects.
 *
 * Revision 1.50  1994/12/09  21:21:57  rob
 * rolled back changes.
 *
 * Revision 1.48  1994/12/05  22:59:27  rob
 * added prototype for send_endlevel_packet.
 *
 * Revision 1.47  1994/12/05  21:47:34  rob
 * Added a new field to netgame struct.
 *
 * Revision 1.46  1994/12/05  14:39:16  rob
 * Added new variable to indicate new net game starting.
 *
 * Revision 1.45  1994/12/04  15:37:18  rob
 * Fixed a typo.
 *
 * Revision 1.44  1994/12/04  15:30:42  rob
 * Added new fields to netgame struct.
 *
 * Revision 1.43  1994/12/03  18:03:19  rob
 * ADded team kill syncing.
 *
 * Revision 1.42  1994/12/01  21:21:27  rob
 * Added new system for object sync on rejoin.
 *
 * Revision 1.41  1994/11/29  13:07:33  rob
 * Changed structure defs to .h files.
 *
 * Revision 1.40  1994/11/22  17:10:48  rob
 * Fix for secret levels in network play mode.
 *
 * Revision 1.39  1994/11/18  16:36:31  rob
 * Added variable network_rejoined to enable random placement
 * of rejoining palyers.
 *
 * Revision 1.38  1994/11/12  19:51:13  rob
 * Changed prototype for NetworkSendData to pass
 * urgent flag.
 *
 * Revision 1.37  1994/11/10  21:48:26  rob
 * Changed net_endlevel to return an int.
 *
 * Revision 1.36  1994/11/10  20:32:49  rob
 * Added extern for LastPacketTime.
 *
 * Revision 1.35  1994/11/09  11:36:34  rob
 * ADded return value to NetworkLevelSync for success/fail.
 *
 * Revision 1.34  1994/11/08  21:22:31  rob
 * Added proto for network_endlevel_snyc
 *
 * Revision 1.33  1994/11/08  15:20:00  rob
 * added proto for ChangePlayerNumTo
 *
 * Revision 1.32  1994/11/07  17:45:40  rob
 * Added prototype for NetworkEndLevel (called from multi.c)
 *
 * Revision 1.31  1994/11/04  19:52:37  rob
 * Removed a function from network.h (network_show_player_list)
 *
 * Revision 1.30  1994/11/01  19:39:26  rob
 * Removed a couple of variables that should be externed from multi.c
 * (remnants of the changover)
 *
 * Revision 1.29  1994/10/31  19:18:24  rob
 * Changed prototype for NetworkDoFrame to add a parameter if you wish
 * to force the frame to be sent.  Important if this is a leave_game frame.
 *
 * Revision 1.28  1994/10/08  19:59:20  rob
 * Removed network message variables.
 *
 * Revision 1.27  1994/10/08  11:08:38  john
 * Moved network_delete_unused objects into multi.c,
 * took out network callsign stuff and used Matt's net
 * players[.].callsign stuff.
 *
 * Revision 1.26  1994/10/07  11:26:20  rob
 * Changed network_start_frame to NetworkDoFrame,.
 *
 *
 * Revision 1.25  1994/10/05  13:58:10  rob
 * Added a new function net_endlevel that is called after each level is
 * completed.  Currently it only does anything if SERIAL game is in effect.
 *
 * Revision 1.24  1994/10/04  19:34:57  rob
 * export network_find_max_net_players.
 *
 * Revision 1.23  1994/10/04  17:31:10  rob
 * Exported MaxNumNetPlayers.
 *
 * Revision 1.22  1994/10/03  22:57:30  matt
 * Took out redundant definition of callsign_len
 *
 * Revision 1.21  1994/10/03  15:12:48  rob
 * Boosted MAX_CREATE_OBJECTS to 15.
 *
 * Revision 1.20  1994/09/30  18:19:51  rob
 * Added two new variables for tracking object creation.
 *
 * Revision 1.19  1994/09/27  15:03:18  rob
 * Added prototype for network_delete_extra_objects used by modem.c
 *
 * Revision 1.18  1994/09/27  14:36:45  rob
 * Added two new varaibles for network/serial weapon firing.
 *
 * Revision 1.17  1994/09/07  17:10:57  john
 * Started adding code to sync powerups.
 *
 * Revision 1.16  1994/09/06  19:29:05  john
 * Added trial version of rejoin function.
 *
 * Revision 1.15  1994/08/26  13:01:54  john
 * Put high score system in.
 *
 * Revision 1.14  1994/08/25  18:12:04  matt
 * Made player's weapons and flares fire from the positions on the 3d model.
 * Also added support for quad lasers.
 *
 * Revision 1.13  1994/08/17  16:50:00  john
 * Added damaging fireballs, missiles.
 *
 * Revision 1.12  1994/08/16  12:25:22  john
 * Added hooks for sending messages.
 *
 * Revision 1.11  1994/08/13  12:20:18  john
 * Made the networking uise the Players array.
 *
 * Revision 1.10  1994/08/12  22:41:27  john
 * Took away Player_stats; add Players array.
 *
 * Revision 1.9  1994/08/12  03:10:22  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.8  1994/08/11  21:57:08  john
 * Moved network options to main menu.
 *
 * Revision 1.7  1994/08/10  11:29:20  john
 * *** empty log message ***
 *
 * Revision 1.6  1994/08/10  10:44:12  john
 * Made net players fire..
 *
 * Revision 1.5  1994/08/09  19:31:46  john
 * Networking changes.
 *
 * Revision 1.4  1994/08/05  16:30:26  john
 * Added capability to turn off network.
 *
 * Revision 1.3  1994/08/05  16:11:43  john
 * Psuedo working version of networking.
 *
 * Revision 1.2  1994/07/25  12:33:34  john
 * Network "pinging" in.
 *
 * Revision 1.1  1994/07/20  16:09:05  john
 * Initial revision
 *
 *
 */


#ifndef _NETWORK_H
#define _NETWORK_H

#include "gameseq.h"
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
#define PID_GAME_UPDATE     60 // inform about new player/team change
#define PID_ENDLEVEL_SHORT  61
#define PID_NAKED_PDATA     62
#define PID_GAME_PLAYERS    63
#define PID_NAMES_RETURN    64 // 0x40

#define PID_EXTRA_GAMEINFO	 65
#define PID_DOWNLOAD			 66
#define PID_UPLOAD			 67

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
extern int Network_game_type;
extern int Network_game_subtype;

typedef struct sequence_packet {
	ubyte           type;
	int             Security;
	ubyte           pad1[3];
	netplayer_info  player;
} __pack__ sequence_packet;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//      bytes on byte boundries
//      shorts on even byte boundries
//      ints on even byte boundries

typedef struct frame_info {
	ubyte       type;                   // What type of packet
	ubyte       pad[3];                 // Pad out length of frame_info packet
	int         numpackets;
	vms_vector  obj_pos;
	vms_matrix  obj_orient;
	vms_vector  phys_velocity;
	vms_vector  phys_rotvel;
	short       obj_segnum;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       playernum;
	ubyte       obj_render_type;
	ubyte       level_num;
	ubyte       data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ frame_info;

// short_frame_info is not aligned -- 01/18/96 -- MWA
// won't align because of shortpos.  Shortpos needs
// to stay in current form.

typedef struct short_frame_info {
	ubyte       type;                   // What type of packet
	ubyte       pad[3];                 // Pad out length of frame_info packet
	int         numpackets;
	shortpos    thepos;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       playernum;
	ubyte       obj_render_type;
	ubyte       level_num;
	ubyte       data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ short_frame_info;

typedef struct entropy_gameinfo {
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
} entropy_gameinfo;

#define MAX_MONSTERBALL_FORCES	25

typedef struct tMonsterballForce {
	ubyte		nWeaponId;
	ubyte		nForce;
} tMonsterballForce;

typedef struct monsterball_info {
	char					nBonus;
	char					nSizeMod;
	tMonsterballForce forces [MAX_MONSTERBALL_FORCES];

} monsterball_info;

typedef struct extra_gameinfo {
	ubyte   	type;
	char		bFriendlyFire;
	char		bInhibitSuicide;
	char		bFixedRespawns;
	long		nSpawnDelay;
	char		bEnhancedCTF;
	char		bRadarEnabled;
	char		bPowerUpsOnRadar;
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
	char		nFusionPowerMod;
	char		bWiggle;
	char		bMultiBosses;
	char		nBossCount;
	char		bSmartWeaponSwitch;
	char		bFluidPhysics;
	char		nWeaponDropMode;
	entropy_gameinfo entropy;
	char		bRotateLevels;
	char		bDisableReactor;
	char		bMouseLook;
	char		nWeaponIcons;
	char		bSafeUDP;
	char		bFastPitch;
	char		bUseSmoke;
	char		bDamageExplosions;
	char		bThrusterFlames;
	char		bShadows;
	char		bRenderShield;
	char		bTeleporterCams;
	char		bDarkness;
	char		bHeadLights;
	char		bPowerupLights;
	char		bTeamDoors;
	char		bEnableCheats;
	char		bTargetIndicators;
	char		bDamageIndicators;
	char		bFriendlyIndicators;
	char		bCloakedIndicators;
	char		bTowFlags;
	char		bUseHitAngles;
	char		bLightTrails;
	char		nSpotSize;
	char		nSpotStrength;
	monsterball_info	monsterball;
} __pack__ extra_gameinfo;

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
	mle	mission;
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
	sequence_packet	playerRejoining;
	fix					nLastPacketTime [MAX_PLAYERS];
	int					bPacketUrgent;
	int					nGameType;
	int					nTotalMissedPackets;
	int					nTotalPacketsGot;
	frame_info			mySyncPack;
	frame_info			urgentSyncPack;
	ubyte					mySyncPackInited;       
	ushort				nMySegsCheckSum;
	sequence_packet	mySeq;
	char					bWantPlayersInfo;
	char					bWaitingForPlayerInfo;
	fix					nStartWaitAllTime;
	int					bWaitAllChoice;
	fix					xPingReturnTime;
	int					bShowPingStats;
	int					tLastPingStat;
} tNetworkData;

extern tNetworkData networkData;

#if 0

static inline EGIFlag (char bLocalFlag, char bMultiFlag, char bDefault, int bAllowLocalFlag)
{
if (IsMultiGame)
	if (gameStates.app.bHaveExtraGameInfo [1])
		if (bMultiFlag)
			if (bAllowLocalFlag)
				return bLocalFlag;
			else
				return 1;
		else
			return 0;
	else
		return bDefault;
else
	return bLocalFlag;
}

#define	EGI_FLAG(_bFlag,_bLocal,_bDefault)	EGIFlag (extraGameInfo [0]._bFlag, extraGameInfo [1]._bFlag, _bDefault, _bLocal)

#else

#define	EGI_FLAG(_bFlag,_bLocal,_bDefault) \
			(IsMultiGame ? \
				gameStates.app.bHaveExtraGameInfo [1] ? \
					extraGameInfo [1]._bFlag ? \
						_bLocal ? \
							extraGameInfo [0]._bFlag \
							: 1 \
						: 0 \
					: _bDefault \
				: extraGameInfo [0]._bFlag)

#endif

int NetworkStartGame();
void NetworkRejoinGame();
void NetworkLeaveGame();
int NetworkEndLevel(int *secret);
void NetworkEndLevelPoll2(int nitems, struct newmenu_item * menus, int * key, int citem);

extern extra_gameinfo extraGameInfo [2];

int NetworkListen();
int NetworkLevelSync();
void NetworkSendEndLevelPacket();

int network_delete_extra_objects();
int network_find_max_net_players();
int NetworkObjnumIsPast(int objnum);
char * NetworkGetPlayerName(int objnum);
void NetworkSendEndLevelSub(int player_num);
void NetworkDisconnectPlayer(int playernum);

extern void NetworkDumpPlayer(ubyte * server, ubyte *node, int why);
extern void NetworkSendNetgameUpdate();

extern int GetMyNetRanking();

extern int NetGameType;
extern int Network_send_objects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern fix LastPacketTime[MAX_PLAYERS];

// By putting an up-to-20-char-message into Network_message and
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
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
/* The networkData.nSecurityNum and the "Security" field of the network structs
 * identifies a netgame. It is a random number chosen by the network master
 * (the one that did "start netgame").
 */

// MWA -- these structures are aligned -- please save me sanity and
// headaches by keeping alignment if these are changed!!!!  Contact
// me for info.

typedef struct endlevel_info {
	ubyte                               type;
	ubyte                               player_num;
	sbyte                               connected;
	ubyte                               seconds_left;
	short											killMatrix [MAX_PLAYERS] [MAX_PLAYERS];
	short                               kills;
	short                               killed;
} endlevel_info;

typedef struct endlevel_info_short {
	ubyte                               type;
	ubyte                               player_num;
	sbyte                               connected;
	ubyte                               seconds_left;
} endlevel_info_short;

// WARNING!!! This is the top part of netgame_info...if that struct changes,
//      this struct much change as well.  ie...they are aligned and the join system will
// not work without it.

// MWA  if this structure changes -- please make appropriate changes to receive_netgame_info
// code for macintosh in netmisc.c

typedef struct lite_info {
	ubyte                           type;
	int                             Security;
	char                            game_name [NETGAME_NAME_LEN+1];
	char                            mission_title [MISSION_NAME_LEN+1];
	char                            mission_name [9];
	int                             levelnum;
	ubyte                           gamemode;
	ubyte                           RefusePlayers;
	ubyte                           difficulty;
	ubyte                           game_status;
	ubyte                           numplayers;
	ubyte                           max_numplayers;
	ubyte                           numconnected;
	ubyte                           game_flags;
	ubyte                           protocol_version;
	ubyte                           version_major;
	ubyte                           version_minor;
	ubyte                           team_vector;
} __pack__ lite_info;

#define NETGAME_INFO_SIZE       sizeof(netgame_info)
#define ALLNETPLAYERSINFO_SIZE  sizeof(allNetPlayers_info)
#define LITE_INFO_SIZE          sizeof(lite_info)
#define SEQUENCE_PACKET_SIZE    sizeof(sequence_packet)
#define FRAME_INFO_SIZE         sizeof(frame_info)
#define IPX_SHORT_INFO_SIZE     sizeof(short_frame_info)
#define ENTROPY_INFO_SIZE       sizeof(extra_gameinfo)

#define MAX_ACTIVE_NETGAMES     80

int  NetworkSendRequest(void);
int  NetworkChooseConnect();
int  NetworkSendGameListRequest();
void NetworkAddPlayer (sequence_packet *p);
void NetworkSendGameInfo(sequence_packet *their);
void ClipRank(signed char *rank);
void NetworkCheckForOldVersion(char pnum);
void NetworkInit(void);
void NetworkFlush();
int  NetworkWaitForAllInfo(int choice);
void NetworkSetGameMode(int gamemode);
void NetworkAdjustMaxDataSize();
int CanJoinNetgame(netgame_info *game,allNetPlayers_info *people);
void RestartNetSearching(newmenu_item * m);
void DeleteTimedOutNetGames (void);
void InitMonsterballSettings (monsterball_info *monsterballP);
char *iptos (char *pszIP, char *addr);

#define DUMP_CLOSED     0 // no new players allowed after game started
#define DUMP_FULL       1 // player cound maxed out
#define DUMP_ENDLEVEL   2
#define DUMP_DORK       3
#define DUMP_ABORTED    4
#define DUMP_CONNECTED  5 // never used
#define DUMP_LEVEL      6
#define DUMP_KICKED     7 // never used

extern netgame_info activeNetGames [MAX_ACTIVE_NETGAMES];
extern allNetPlayers_info activeNetPlayers [MAX_ACTIVE_NETGAMES];
extern allNetPlayers_info *tmpPlayersInfo, tmpPlayersBase;

#endif /* _NETWORK_H */
