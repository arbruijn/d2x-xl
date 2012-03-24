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

#ifndef _MULTI_H
#define _MULTI_H

#define MAX_MESSAGE_LEN 35

#include "loadgame.h"
#include "piggy.h"

// What version of the multiplayer protocol is this?

#define MULTI_PROTO_VERSION 4

// Protocol versions:
//   1 Descent Shareware
//   2 Descent Registered/Commercial
//   3 Descent II Shareware
//   4 Descent II Commercial

// Save multiplayer games?

#define MULTI_SAVE

// How many simultaneous network players do we support?

#define APPLETALK_GAME  1
#define IPX_GAME        2
#define UDP_GAME			3

#define MAX_NUM_NET_PLAYERS			((gameStates.multi.nGameType == UDP_GAME) ? 16 : 8)

#define MULTI_POSITION					0
#define MULTI_REAPPEAR					1
#define MULTI_FIRE						2
#define MULTI_KILL						3
#define MULTI_REMOVE_OBJECT			4
#define MULTI_PLAYER_EXPLODE			5
#define MULTI_MESSAGE					6
#define MULTI_QUIT						7
#define MULTI_PLAY_SOUND				8
#define MULTI_BEGIN_SYNC				9
#define MULTI_CONTROLCEN				10
#define MULTI_ROBOT_CLAIM				11
#define MULTI_END_SYNC					12
#define MULTI_CLOAK						13
#define MULTI_ENDLEVEL_START			14
#define MULTI_DOOR_OPEN					15
#define MULTI_CREATE_EXPLOSION		16
#define MULTI_CONTROLCEN_FIRE			17
#define MULTI_PLAYER_DROP				18
#define MULTI_CREATE_POWERUP			19
#define MULTI_CONSISTENCY				20
#define MULTI_DECLOAK					21
#define MULTI_MENU_CHOICE				22
#define MULTI_ROBOT_POSITION			23
#define MULTI_ROBOT_EXPLODE			24
#define MULTI_ROBOT_RELEASE			25
#define MULTI_ROBOT_FIRE				26
#define MULTI_SCORE						27
#define MULTI_CREATE_ROBOT				28
#define MULTI_TRIGGER					29
#define MULTI_BOSS_ACTIONS				30
#define MULTI_CREATE_ROBOT_POWERUPS	31
#define MULTI_HOSTAGE_DOOR				32

#define MULTI_SAVE_GAME					33
#define MULTI_RESTORE_GAME				34

#define MULTI_REQ_PLAYER				35  // Someone requests my CPlayerData structure
#define MULTI_SEND_PLAYER				36  // Sending someone my CPlayerData structure
#define MULTI_MARKER						37
#define MULTI_DROP_WEAPON				38
#define MULTI_GUIDED						39
#define MULTI_STOLEN_ITEMS				40
#define MULTI_WALL_STATUS				41  // send to new players
#define MULTI_HEARTBEAT					42

#define MULTI_SCOREGOALS				43
#define MULTI_SEISMIC					44
#define MULTI_LIGHT						45
#define MULTI_START_TRIGGER			46
#define MULTI_FLAGS						47
#define MULTI_DROP_BLOB					48
#define MULTI_POWERUP_UPDATE			49
#define MULTI_ACTIVE_DOOR				50
#define MULTI_SOUND_FUNCTION			51
#define MULTI_CAPTURE_BONUS			52
#define MULTI_GOT_FLAG					53
#define MULTI_DROP_FLAG					54
#define MULTI_ROBOT_CONTROLS			55
#define MULTI_FINISH_GAME				56
#define MULTI_RANK						57
#define MULTI_MODEM_PING				58
#define MULTI_MODEM_PING_RETURN		59
#define MULTI_ORB_BONUS					60
#define MULTI_GOT_ORB					61
#define MULTI_DROP_ORB					62
#define MULTI_PLAY_BY_PLAY				63
#define MULTI_MAX_TYPE_D2				63

#define MULTI_RETURN_FLAG				64
#define MULTI_CONQUER_ROOM				65
#define MULTI_CONQUER_WARNING			66
#define MULTI_STOP_CONQUER_WARNING	67
#define MULTI_TELEPORT					68
#define MULTI_SET_TEAM					69
#define MULTI_START_TYPING				70
#define MULTI_QUIT_TYPING				71
#define MULTI_OBJ_TRIGGER				72
#define MULTI_PLAYER_SHIELDS			73
#define MULTI_INVUL						74
#define MULTI_DEINVUL					75
#define MULTI_WEAPONS					76
#define MULTI_MONSTERBALL				77
#define MULTI_CHEATING					78
#define MULTI_TRIGGER_EXT				79
#define MULTI_SYNC_KILLS				80
#define MULTI_COUNTDOWN					81
#define MULTI_PLAYER_WEAPONS			82
#define MULTI_SYNC_MONSTERBALL		83
#define MULTI_DROP_POWERUP				84
#define MULTI_CREATE_WEAPON			85
#define MULTI_AMMO						86
#define MULTI_FUSION_CHARGE			87
#define MULTI_PLAYER_THRUST			88
#define MULTI_MAX_TYPE					88

#define MAX_NET_CREATE_OBJECTS		40

#define MAX_MULTI_MESSAGE_LEN			120

// Exported functions

int ObjnumRemoteToLocal (int remote_obj, int owner);
int ObjnumLocalToRemote (int local_obj, sbyte *owner);
void MapObjnumLocalToRemote (int local, int remote, int owner);
void MapObjnumLocalToLocal (int nObject);
void ResetNetworkObjects ();

void MultiInitObjects (void);
void MultiShowPlayerList (void);
void MultiDoFrame (void);
void MultiCapObjects (void);

void MultiSendPlayerWeapons (void);
void MultiSendPlayerThrust (void);
void MultiSendFlags (char);
void MultiSendWeapons (int bForce);
void MultiSendAmmo (void);
void MultiSendMonsterball (int bForce, int bCreate);
void MultiSendFire (void);
void MultiSendDestroyReactor (int nObject, int nPlayer);
void MultiSendCountdown (void);
void MultiSendEndLevelStart (int);
void MultiSendPlayerExplode (char nType);
void MultiSendMessage (void);
void MultiSendPosition (int nObject);
void MultiSendReappear ();
void MultiSendKill (int nObject);
void MultiSendRemoveObj (int nObject);
void MultiSendQuit (int why);
void MultiSendDoorOpen (int nSegment, int nSide, ushort flags);
void MultiSendCreateExplosion (int nPlayer);
void MultiSendCtrlcenFire (CFixVector *to_target, int nGun, int nObject);
void MultiSendInvul (void);
void MultiSendDeInvul (void);
void MultiSendCloak (void);
void MultiSendDeCloak (void);
void MultiSendCreateWeapon (int nObject);
void MultiSendCreatePowerup (int powerupType, int nSegment, int nObject, const CFixVector *vPos);
void MultiSendDropPowerup (int powerupType, int nSegment, int nObject, const CFixVector *vPos, const CFixVector *vVel);
void MultiSendPlaySound (int nSound, fix volume);
void MultiSendAudioTaunt (int taunt_num);
void MultiSendScore (void);
void MultiSendTrigger (int nTrigger, int nObject);
void MultiSendObjTrigger (int CTrigger);
void MultiSendHostageDoorStatus (int wallnum);
void MultiSendNetPlayerStatsRequest (ubyte nPlayer);
void MultiSendDropWeapon (int nObject);
void MultiSendDropMarker (int nPlayer,CFixVector position,char messagenum,char text[]);
void MultiSendGuidedInfo (CObject *miss,char);
void MultiSendReturnFlagHome (short nObject);
void MultiSendCaptureBonus (char pnum);
void MultiSendShield (void);
void MultiSendCheating (void);
void MultiSendFusionCharge (void);

void MultiEndLevelScore (void);
void MultiPrepLevel (void);
int MultiEndLevel (int *secret);
int MultiMenuPoll (void);
void MultiLeaveGame (void);
void MultiProcessData (char *dat, int len);
void MultiProcessBigData (char *buf, int len);
void MultiDoDeath (int nObject);
void MultiSendMsgDialog (void);
int MultiDeleteExtraObjects (void);
void MultiMakeGhostPlayer (int nObject);
void MultiMakePlayerGhost (int nObject);
void MultiDefineMacro (int key);
void MultiSendMacro (int key);
int MultiGetKillList (int *plist);
void MultiNewGame (void);
void MultiSortKillList (void);
int MultiChooseMission (int *anarchy_only);
void MultiResetStuff (void);
void MultiSendConquerRoom (char owner, char prevOwner, char group);
void MultiSendConquerWarning ();
void MultiSendStopConquerWarning ();
void MultiSendData (char *buf, int len, int repeat);
void ChoseTeam (int nPlayer, bool bForce = false);
void AutoBalanceTeams ();

short GetTeam (int nPlayer);
bool SameTeam (int nPlayer1, int nPlayer2);

// Exported variables


extern int Network_active;
extern int NetlifeKills,NetlifeKilled;

extern int bUseMacros;

extern int message_length[MULTI_MAX_TYPE+1];

extern CShortArray scoreMatrix;
//extern short scoreMatrix[MAX_PLAYERS][MAX_PLAYERS];


extern void MultiMsgInputSub (int key);
extern void MultiSendMsgStart (char nMsg);
extern void MultiSendMsgQuit (void);
extern void MultiSendTyping (void);

extern int MultiPowerupIs4Pack (int);
extern void MultiSendOrbBonus (char pnum);
extern void MultiSendGotOrb (char pnum);
extern void MultiAddLifetimeKills (void);

extern void MultiSendTeleport (char pnum, short nSegment, char nSide);

extern int controlInvulTime;

#define N_PLAYER_SHIP_TEXTURES 6

extern tBitmapIndex mpTextureIndex [MAX_PLAYERS][N_PLAYER_SHIP_TEXTURES];

#define NETGAME_FLAG_CLOSED            1
#define NETGAME_FLAG_SHOW_ID           2
#define NETGAME_FLAG_SHOW_MAP          4
#define NETGAME_FLAG_HOARD             8
#define NETGAME_FLAG_TEAM_HOARD        16
#define NETGAME_FLAG_REALLY_ENDLEVEL   32
#define NETGAME_FLAG_REALLY_FORMING    64
#define NETGAME_FLAG_ENTROPY				128
#define NETGAME_FLAG_MONSTERBALL			 (NETGAME_FLAG_HOARD | NETGAME_FLAG_ENTROPY)	//ugly hack, but we only have a single byte ... :-/

#define NETGAME_NAME_LEN                15
#define NETGAME_AUX_SIZE                20  // Amount of extra data for the network protocol to store in the netgame packet

enum compType {DOS,WIN_32,WIN_95,MAC} __pack__ ;

// sigh...the socket structure member was moved away from it's friends.
// I'll have to create a union for appletalk network info with just
// the server and node members since I cannot change the order ot these
// members.

#ifdef _WIN32
#pragma pack (push, 1)
#endif

typedef union tPort {
	ubyte b [2];
	ushort s;
} __pack__ tPort;

typedef struct tIP {
	ubyte	ip [4];
	tPort	port;
} __pack__ tIP;

typedef union tNetworkNode {
	tIP	a;
	ubyte	v [6];
} tNetworkNode;

typedef struct tIpxAddr {
	ubyte   server [4];
	tNetworkNode	node;
} __pack__ tIpxAddr;

typedef struct tAppleTalkAddr {
	ushort  net;
	ubyte   node;
	ubyte   socket;
} __pack__ tAppleTalkAddr;


typedef union {
	public:
		tIpxAddr			ipx;
		tAppleTalkAddr	appletalk;

} __pack__ tNetworkInfo;

class CNetworkInfo {
	public:
		tNetworkInfo	m_info;

	public:
		inline ubyte* Server (void) { return m_info.ipx.server; }
		inline ubyte* Node (void) { return m_info.ipx.node.v; }
		inline ubyte* IP (void) { return m_info.ipx.node.a.ip; }
		inline ushort* Port (void) { return &m_info.ipx.node.a.port.s; }
		inline tAppleTalkAddr& AppleTalk (void) { return m_info.appletalk; }
		inline tNetworkInfo& operator= (tNetworkInfo& other) {
			m_info = other;
			return m_info;
			}
	};

//inline tNetworkInfo& operator= (tNetworkInfo& i, CNetworkInfo& j) { 
//	i = j.m_info;
//	return i;
//	}

#ifdef _WIN32
#pragma pack (pop)
#endif

typedef struct tNetPlayerInfo {
	char    callsign [CALLSIGN_LEN+1];
	CNetworkInfo network;
	ubyte   versionMajor;
	ubyte   versionMinor;
	ubyte   computerType;
	sbyte   connected;
	ushort  socket;
	ubyte   rank;

	int Connected (void) { return *callsign ? connected : 0; }
} __pack__ tNetPlayerInfo;


typedef struct tAllNetPlayersInfo
{
	char    nType;
	int     nSecurity;
	struct tNetPlayerInfo players [MAX_PLAYERS_D2X + 4];
} __pack__ tAllNetPlayersInfo;

class CAllNetPlayersInfo {
	public:
		tAllNetPlayersInfo	m_info;

	inline size_t Size (void) { return sizeof (m_info) - sizeof (m_info.players) + sizeof (tNetPlayerInfo) * (MAX_NUM_NET_PLAYERS + 4); }

	inline tAllNetPlayersInfo& operator= (tAllNetPlayersInfo& other) {
		memcpy (&m_info, &other, Size ());
		return m_info;
		}
	inline tAllNetPlayersInfo& operator= (CAllNetPlayersInfo& other) {
		m_info = other.m_info;
		return m_info;
		}
};

typedef struct tNetGameInfoD2 {
	int     locations [MAX_PLAYERS_D2];			// 32 bytes
	short   kills [MAX_PLAYERS_D2][MAX_PLAYERS_D2];	// 128 bytes
	ushort  nSegmentCheckSum;						// 2 bytes
	short   teamKills [2];							// 4 bytes
	short   killed [MAX_PLAYERS_D2];				// 16 bytes
	short   playerKills [MAX_PLAYERS_D2];		// 16 bytes
	int     nScoreGoal;								// 4 bytes
	fix     xPlayTimeAllowed;						// 4 bytes
	fix     xLevelTime;								// 4 bytes
	int     controlInvulTime;						// 4 bytes
	int     monitorVector;							// 4 bytes
	int     playerScore [MAX_PLAYERS_D2];		// 32 bytes
	ubyte   playerFlags [MAX_PLAYERS_D2];		// 8 bytes
	short   nPacketsPerSec;							// 2 bytes
	ubyte   bShortPackets;							// 1 bytes
// 279 bytes
// 355 bytes total
	ubyte   auxData[NETGAME_AUX_SIZE];  // Storage for protocol-specific data (e.g., multicast session and port)
} __pack__ tNetGameInfoD2;

typedef struct tNetGameInfoD2X {
	int     locations [MAX_PLAYERS_D2X];		// 32 bytes
	short   kills [MAX_PLAYERS_D2X][MAX_PLAYERS_D2X];	// 128 bytes
	ushort  nSegmentCheckSum;						// 2 bytes
	short   teamKills [2];							// 4 bytes
	short   killed [MAX_PLAYERS_D2X];			// 16 bytes
	short   playerKills [MAX_PLAYERS_D2X];		// 16 bytes
	int     nScoreGoal;								// 4 bytes
	fix     xPlayTimeAllowed;						// 4 bytes
	fix     xLevelTime;								// 4 bytes
	int     controlInvulTime;						// 4 bytes
	int     monitorVector;							// 4 bytes
	int     playerScore [MAX_PLAYERS_D2X];		// 32 bytes
	ubyte   playerFlags [MAX_PLAYERS_D2X];		// 8 bytes
	short   nPacketsPerSec;							// 2 bytes
	ubyte   bShortPackets;							// 1 bytes
	ubyte   auxData [NETGAME_AUX_SIZE];  // Storage for protocol-specific data (e.g., multicast session and port)
} __pack__ tNetGameInfoD2X;

typedef struct tNetGameInfoLite {
	public:
		ubyte                        nType;
		int                          nSecurity;
		char                         szGameName [NETGAME_NAME_LEN+1];
		char                         szMissionTitle [MISSION_NAME_LEN+1];
		char                         szMissionName [9];
		int                          nLevel;
		ubyte                        gameMode;
		ubyte                        bRefusePlayers;
		ubyte                        difficulty;
		ubyte                        gameStatus;
		ubyte                        nNumPlayers;
		ubyte                        nMaxPlayers;
		ubyte                        nConnected;
		ubyte                        gameFlags;
		ubyte                        protocolVersion;
		ubyte                        versionMajor;
		ubyte                        versionMinor;
		ubyte                        teamVector;

   public:
		inline int GetLevel (void) { return nLevel & 0xFFFF; }
		inline void SetLevel (int n) { nLevel = (nLevel & 0xFFFF0000) | (n & 0xFFFF); }
		ushort GetTeamVector (void);
		void SetTeamVector (ushort n);
		inline void AddTeamPlayer (int n) { SetTeamVector (GetTeamVector () | (1 << n)); }
		inline void RemoveTeamPlayer (int n) { SetTeamVector (GetTeamVector () & ~(1 << n)); }
} __pack__ tNetGameInfoLite;

typedef struct tNetGameInfo : tNetGameInfoLite {
// 72 bytes
// change the order of the bit fields for the mac compiler.
// doing so will mean I don't have to do screwy things to
// send this as network information

#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
	short DoMegas:1;		//first word
	short DoSmarts:1;
	short DoFusions:1;
	short DoHelix:1;
	short DoPhoenix:1;
	short DoAfterburner:1;
	short DoInvulnerability:1;
	short DoCloak:1;
	short DoGauss:1;
	short DoVulcan:1;
	short DoPlasma:1;
	short DoOmega:1;
	short DoSuperLaser:1;
	short DoProximity:1;
	short DoSpread:1;
	short DoSmartMine:1;
	short DoFlash:1;		//second word
	short DoGuided:1;
	short DoEarthShaker:1;
	short DoMercury:1;
	short bAllowMarkerView:1;
	short bIndestructibleLights:1;
	short DoAmmoRack:1;
	short DoConverter:1;
	short DoHeadlight:1;
	short DoHoming:1;
	short DoLaserUpgrade:1;
	short DoQuadLasers:1;
	short bShowAllNames:1;
	short BrightPlayers:1;
	short invul:1;
	short FriendlyFireOff:1;
#else
	short DoSmartMine:1;
	short DoSpread:1;
	short DoProximity:1;
	short DoSuperLaser:1;
	short DoOmega:1;
	short DoPlasma:1;
	short DoVulcan:1;
	short DoGauss:1;
	short DoCloak:1;
	short DoInvulnerability:1;
	short DoAfterburner:1;
	short DoPhoenix:1;
	short DoHelix:1;
	short DoFusions:1;
	short DoSmarts:1;
	short DoMegas:1;
	short FriendlyFireOff:1;
	short invul:1;
	short BrightPlayers:1;
	short bShowAllNames:1;
	short DoQuadLasers:1;
	short DoLaserUpgrade:1;
	short DoHoming:1;
	short DoHeadlight:1;
	short DoConverter:1;
	short DoAmmoRack:1;
	short bIndestructibleLights:1;
	short bAllowMarkerView:1;
	short DoMercury:1;
	short DoEarthShaker:1;
	short DoGuided:1;
	short DoFlash:1;
#endif

	char    szTeamName [2][CALLSIGN_LEN+1];	// 18 bytes
	union {
		tNetGameInfoD2		d2;
		tNetGameInfoD2X	d2x;
		} versionSpecific;
	} __pack__ tNetGameInfo;

class CNetGameInfo {
	public:
		tNetGameInfo	m_info;
		ubyte				m_server [10];

	public:
		CNetGameInfo() { memset (&m_info, 0, sizeof (m_info)); }
		CNetGameInfo (tNetGameInfo* ngi) { *this = *ngi; }

		inline size_t Size (void) { return (gameStates.multi.nGameType == UDP_GAME) ? sizeof (tNetGameInfo) : sizeof (tNetGameInfo) - sizeof (tNetGameInfoD2X) + sizeof (tNetGameInfoD2); }

		inline tNetGameInfo& operator= (tNetGameInfo& other) {
			memcpy (&m_info, &other, Size ());
			return m_info;
			}

		inline tNetGameInfo& operator= (CNetGameInfo& other) {
			m_info = other.m_info;
			return m_info;
			}

		inline int* Locations (int i = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.locations + i : m_info.versionSpecific.d2.locations + i; }
		inline short* Kills (int i = 0, int j = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.kills [i] + j : m_info.versionSpecific.d2.kills [i] + j; }
		inline short* TeamKills (int i = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.teamKills + i : m_info.versionSpecific.d2.teamKills + i; }
		inline short* Killed (int i = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.killed  + i : m_info.versionSpecific.d2.killed  + i; }
		inline short* PlayerKills (int i = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.playerKills  + i : m_info.versionSpecific.d2.playerKills  + i; }
		inline int* PlayerScore (int i = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.playerScore  + i : m_info.versionSpecific.d2.playerScore  + i; }
		inline ubyte* PlayerFlags (int i = 0) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.playerFlags  + i : m_info.versionSpecific.d2.playerFlags  + i; }
		inline ubyte* AuxData (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.auxData : m_info.versionSpecific.d2.auxData; }

		inline ushort GetSegmentCheckSum (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.nSegmentCheckSum : m_info.versionSpecific.d2.nSegmentCheckSum; }
		inline int GetScoreGoal (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.nScoreGoal : m_info.versionSpecific.d2.nScoreGoal; }
		inline fix GetPlayTimeAllowed (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.xPlayTimeAllowed : m_info.versionSpecific.d2.xPlayTimeAllowed; }
		inline fix GetLevelTime (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.xLevelTime : m_info.versionSpecific.d2.xLevelTime; }
		inline int GetControlInvulTime (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.controlInvulTime : m_info.versionSpecific.d2.controlInvulTime; }
		inline int GetMonitorVector (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.monitorVector : m_info.versionSpecific.d2.monitorVector; }
		inline short GetPacketsPerSec (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.nPacketsPerSec : m_info.versionSpecific.d2.nPacketsPerSec; }
		inline ubyte GetShortPackets (void) { return (gameStates.multi.nGameType == UDP_GAME) ? m_info.versionSpecific.d2x.bShortPackets : m_info.versionSpecific.d2.bShortPackets; }

		inline void SetSegmentCheckSum (ushort n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.nSegmentCheckSum = n; else m_info.versionSpecific.d2.nSegmentCheckSum = n; }
		inline void SetScoreGoal (int n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.nScoreGoal = n; else m_info.versionSpecific.d2.nScoreGoal = n; }
		inline void SetPlayTimeAllowed (fix n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.xPlayTimeAllowed = n; else m_info.versionSpecific.d2.xPlayTimeAllowed = n; }
		inline void SetLevelTime (fix n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.xLevelTime = n; else m_info.versionSpecific.d2.xLevelTime = n; }
		inline void SetControlInvulTime (int n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.controlInvulTime = n; else m_info.versionSpecific.d2.controlInvulTime = n; }
		inline void SetMonitorVector (int n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.monitorVector = n; else m_info.versionSpecific.d2.monitorVector = n; }
		inline void SetPacketsPerSec (short n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.nPacketsPerSec = n; else m_info.versionSpecific.d2.nPacketsPerSec = n; }
		inline void SetShortPackets (ubyte n) { if (gameStates.multi.nGameType == UDP_GAME) m_info.versionSpecific.d2x.bShortPackets = n; else m_info.versionSpecific.d2.bShortPackets = n; }
};

#define MAX_ROBOTS_CONTROLLED 5

typedef struct tMultiRobotData {
	int controlled [MAX_ROBOTS_CONTROLLED];
	int agitation [MAX_ROBOTS_CONTROLLED];
	fix controlledTime [MAX_ROBOTS_CONTROLLED];
	fix lastSendTime [MAX_ROBOTS_CONTROLLED];
	fix lastMsgTime [MAX_ROBOTS_CONTROLLED];
	int sendPending [MAX_ROBOTS_CONTROLLED];
	int fired [MAX_ROBOTS_CONTROLLED];
	sbyte fireBuf [MAX_ROBOTS_CONTROLLED][18+3];
} __pack__ tMultiRobotData;

extern CNetGameInfo netGame;
extern CAllNetPlayersInfo netPlayers [2];

int IAmGameHost (void);
void ChangePlayerNumTo (int nPlayer);
int SetLocalPlayer (CAllNetPlayersInfo* playerInfoP, int nPlayers, int nDefault = -1);

//how to encode missiles & flares in weapon packets
#define MISSILE_ADJUST  100
#define FLARE_ADJUST    127

int PingPlayer (int n);
int MultiProtectGame (void);
void SwitchTeam (int nPlayer, int bForce);
void SetTeam (int nPlayer, int team);
void MultiSendModemPing ();
int MultiFindGoalTexture (short t);
void MultiSetFlagPos (void);
void ResetPlayerPaths (void);
void SetPlayerPaths (void);
void MultiSyncKills (void);
void MultiAdjustPowerups (void);
void RemapLocalPlayerObject (int nLocalObj, int nRemoteObj);
void MultiOnlyPlayerMsg (int bMsgBox);

void MultiSendStolenItems ();
void MultiSendScoreGoalCounts ();
void MultiSendPowerupUpdate ();
void MultiSendDoorOpenSpecific (int nPlayer, int nSegment, int nSide, ushort flags);
void MultiSendWallStatusSpecific (int nPlayer, int wallnum, ubyte nType, ushort flags, ubyte state);
void MultiSendLightSpecific (int nPlayer, int nSegment, ubyte val);
void MultiSendTriggerSpecific (char nPlayer, ubyte trig);
void MultiSetObjectTextures (CObject *objP);
void MultiSendSeismic (fix start, fix end);
void MultiSendDropBlobs (char nPlayer);
void InitDefaultShipProps (void);
void SetupPowerupFilter (tNetGameInfo* infoP = NULL);

#endif /* _MULTI_H */
